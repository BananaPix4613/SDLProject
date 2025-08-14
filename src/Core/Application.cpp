// -------------------------------------------------------------------------
// Application.cpp
// -------------------------------------------------------------------------
#include "Core/Application.h"

// Core headers
#include "Core/EventSystem.h"
#include "Core/Logger.h"
#include "Core/ResourceManager.h"
#include "Core/TimeManager.h"
#include "Core/MemoryManager.h"
#include "Core/Window.h"

// Debug headers
#include "Debug/RenderingDebugger.h"

// ECS headers
#include "ECS/Component.h"
#include "ECS/ComponentRegistry.h"
#include "ECS/UUID.h"
#include "ECS/World.h"
#include "ECS/Components/TransformComponent.h"

// Rendering headers
#include "Rendering/LightManager.h"
#include "Rendering/Material.h"
#include "Rendering/RenderSystem.h"
#include "Rendering/RenderPipelineVerifier.h"
#include "Rendering/ShaderManager.h"
#include "Rendering/ForwardPlusShaderManager.h"
#include "Rendering/Cameras/OrthographicCamera.h"

// Utility headers
#include "Utility/Serialization/SchemaRegistry.h"
#include "Utility/GraphicsCapabilities.h"

// Voxel headers
#include "Voxel/ChunkManager.h"
#include "Voxel/GenerationParameters.h"
#include "Voxel/Grid.h"
#include "Voxel/ProceduralGenerationSystem.h"
#include "Voxel/StreamingManager.h"
#include "Voxel/VoxelRenderer.h"

// In namespace PixelCraft
namespace PixelCraft::Core
{

    // Initialize static members
    std::mutex Application::m_instanceMutex;
    Application* Application::s_instance = nullptr;

    Application* Application::getInstance()
    {
        std::lock_guard<std::mutex> lock(m_instanceMutex);
        if (!s_instance)
        {
            s_instance = new Application();
        }
        return s_instance;
    }

    Application::Application()
        : m_isRunning(false), m_deltaTime(0.0f)
    {
    }

    bool Application::initialize()
    {
        Core::info("Initializing PixelCraft Engine");

        Core::info("Registering core subsystems");

        // Register core subsystems (non-rendering systems first)
        auto eventSystem = std::make_shared<EventSystem>();
        if (!registerSubsystem<EventSystem>(eventSystem))
        {
            Core::error("Failed to register EventSystem");
            return false;
        }

        auto configManager = std::make_shared<ConfigManager>();
        if (!registerSubsystem<ConfigManager>(configManager))
        {
            Core::error("Failed to register ConfigManager");
            return false;
        }

        auto timeManager = std::make_shared<TimeManager>();
        if (!registerSubsystem<TimeManager>(timeManager))
        {
            Core::error("Failed to register TimeManager");
            return false;
        }

        auto memoryManager = std::make_shared<MemoryManager>();
        if (!registerSubsystem<MemoryManager>(memoryManager))
        {
            Core::error("Failed to register MemoryManager");
            return false;
        }

        // Register the ECS World subsystem (must be early as other systems depend on it)
        auto world = std::make_shared<ECS::World>();
        if (!registerSubsystem<ECS::World>(world))
        {
            Core::error("Failed to register World");
            return false;
        }

        // Register the Window subsystem (must be before rendering systems)
        auto window = std::make_shared<Window>();
        if (!registerSubsystem<Window>(window))
        {
            Core::error("Failed to register Window");
            return false;
        }

        // Register the shader manager (needed before render systems)
        auto shaderManager = std::make_shared<Rendering::ShaderManager>();
        if (!registerSubsystem<Rendering::ShaderManager>(shaderManager))
        {
            Core::error("Failed to register ShaderManager");
            return false;
        }

        // Register the forward plus shader manager (needed before forward+ render systems)
        auto forwardPlusShaderManager = std::make_shared<Rendering::ForwardPlusShaderManager>();
        if (!registerSubsystem<Rendering::ForwardPlusShaderManager>(forwardPlusShaderManager))
        {
            Core::error("Failed to register ForwardPlusShaderManager");
            return false;
        }

        // Register the resource manager
        auto resourceManager = std::make_shared<Core::ResourceManager>();
        if (!registerSubsystem<Core::ResourceManager>(resourceManager))
        {
            Core::error("Failed to register ResourceManager");
            return false;
        }

        // Initialize serialization system before voxel systems
        if (!initializeSerializationSystem())
        {
            Core::error("Failed to initialize serialization system");
            return false;
        }

        Core::info("Registering voxel subsystems");

        // Register the voxel systems
        auto chunkManager = std::make_shared<Voxel::ChunkManager>();
        if (!registerSubsystem<Voxel::ChunkManager>(chunkManager))
        {
            Core::error("Failed to register ChunkManager");
            return false;
        }

        auto streamingManager = std::make_shared<Voxel::StreamingManager>();
        if (!registerSubsystem<Voxel::StreamingManager>(streamingManager))
        {
            Core::error("Failed to register StreamingManager");
            return false;
        }

        auto proceduralGenSystem = std::make_shared<Voxel::ProceduralGenerationSystem>();
        if (!registerSubsystem<Voxel::ProceduralGenerationSystem>(proceduralGenSystem))
        {
            Core::error("Failed to register ProceduralGenerationSystem");
            return false;
        }

        // Register rendering systems but defer their initialization
        auto renderSystem = std::make_shared<Rendering::RenderSystem>();
        if (!registerSubsystem<Rendering::RenderSystem>(renderSystem))
        {
            Core::error("Failed to register RenderSystem");
            return false;
        }

        auto voxelRenderer = std::make_shared<Voxel::VoxelRenderer>();
        if (!registerSubsystem<Voxel::VoxelRenderer>(voxelRenderer))
        {
            Core::error("Failed to register VoxelRenderer");
            return false;
        }

        // Initialize subsystems in dependency order, but EXCLUDE rendering systems for now
        if (!sortSubsystemDependencies())
        {
            Core::error("Failed to sort subsystem dependencies");
            return false;
        }

        // Initialize all NON-RENDERING subsystems first
        std::set<std::type_index> initializedSystems;
        std::set<std::type_index> renderingSystems = {
            std::type_index(typeid(Rendering::RenderSystem)),
            std::type_index(typeid(Voxel::VoxelRenderer))
        };

        for (const auto& typeIndex : m_initializationOrder)
        {
            // Skip rendering systems for now
            if (renderingSystems.find(typeIndex) != renderingSystems.end())
            {
                Core::info("Deferring initialization of rendering system: " + m_subsystems[typeIndex]->getName());
                continue;
            }

            if (!initializeSubsystem(typeIndex, initializedSystems))
            {
                Core::error("Failed to initialize subsystem: " + m_subsystems[typeIndex]->getName());
                return false;
            }
        }

        // Initialize graphics capabilities detection after OpenGL context is available
        Core::info("Detecting graphics hardware capabilities");
        auto& graphicsCapabilities = Utility::GraphicsCapabilities::getInstance();
        if (!graphicsCapabilities.initialize())
        {
            Core::error("Failed to initialize graphics capabilities detection");
            return false;
        }

        // Create basic shaders before initializing rendering systems
        Core::info("Creating basic shaders before rendering system initialization");
        if (!createBasicShaders())
        {
            Core::error("Failed to create basic shaders");
            return false;
        }

        // Initialize the rendering systems with shaders available
        Core::info("Initializing rendering systems with shaders available");
        for (const auto& typeIndex : renderingSystems)
        {
            if (m_subsystems.find(typeIndex) != m_subsystems.end())
            {
                if (!initializeSubsystem(typeIndex, initializedSystems))
                {
                    Core::error("Failed to initialize rendering system: " + m_subsystems[typeIndex]->getName());
                    return false;
                }
            }
        }

        if (!setupCamera())
        {
            Core::error("Failed to setup camera");
            return false;
        }

        createBasicMaterials();

        if (!setupTestVoxelWorld())
        {
            Core::error("Failed to setup test voxel world");
            return false;
        }

        Core::info("Application initialization complete");
        return true;
    }

    bool Application::initializeSerializationSystem()
    {
        using namespace PixelCraft::Utility::Serialization;

        info("Initializing serialization system");

        try
        {
            auto& registry = SchemaRegistry::getInstance();

            // Register Voxel module types
            registry.registerType<Voxel::Voxel>("Voxel", {1, 0, 0});
            registry.registerType<Voxel::ChunkCoord>("ChunkCoord", {1, 0, 0});
            registry.registerType<Voxel::Chunk>("Chunk", {1, 0, 0});

            // Register ECS types
            registry.registerType<ECS::Component>("Component", {1, 0, 0});
            registry.registerType<ECS::TransformComponent>("TransformComponent", {1, 0, 0});

            // Register utility types
            registry.registerType<Utility::AABB>("AABB", {1, 0, 0});
            registry.registerType<ECS::UUID>("UUID", {1, 0, 0});

            info("Serialization system initialized with " +
                 std::to_string(registry.getAllSchemaNames().size()) +
                 " registered types");

            return true;
        }
        catch (const std::exception& e)
        {
            error("Exception during serialization system initialization: " + std::string(e.what()));
            return false;
        }
    }

    bool Application::sortSubsystemDependencies()
    {
        Core::info("Sorting subsystem dependencies");

        // Clear any existing order
        m_initializationOrder.clear();

        // Build dependency graph
        std::unordered_map<std::string, std::type_index> nameToType;
        std::unordered_map<std::type_index, std::vector<std::type_index>> dependencyGraph;
        std::unordered_map<std::type_index, int> inDegree;

        // First pass: map names to types and initialize in-degree
        for (const auto& [typeIndex, subsystem] : m_subsystems)
        {
            nameToType.emplace(subsystem->getName(), typeIndex);
            inDegree.emplace(typeIndex, 0);
        }

        // Second pass: build dependency graph
        for (const auto& [typeIndex, subsystem] : m_subsystems)
        {
            auto dependencies = subsystem->getDependencies();

            for (const auto& depName : dependencies)
            {
                auto depIt = nameToType.find(depName);
                if (depIt != nameToType.end())
                {
                    // Add edge: dependency -> current subsystem
                    dependencyGraph[depIt->second].push_back(typeIndex);
                    inDegree[typeIndex]++;
                }
                else
                {
                    Core::warn("Subsystem '" + subsystem->getName() +
                              "' depends on unknown subsystem '" + depName + "'");
                }
            }
        }

        // Topological sort using Kahn's algorithm
        std::queue<std::type_index> queue;

        // Find all nodes with no incoming edges
        for (const auto& [typeIndex, degree] : inDegree)
        {
            if (degree == 0)
            {
                queue.push(typeIndex);
            }
        }
        
        // Process nodes
        while (!queue.empty())
        {
            auto current = queue.front();
            queue.pop();

            m_initializationOrder.push_back(current);

            // Reduce in-degree for all dependents
            if (dependencyGraph.find(current) != dependencyGraph.end())
            {
                for (const auto& dependent : dependencyGraph[current])
                {
                    inDegree[dependent]--;
                    if (inDegree[dependent] == 0)
                    {
                        queue.push(dependent);
                    }
                }
            }
        }

        // Check if all subsystems were processed (no cycles)
        if (m_initializationOrder.size() != m_subsystems.size())
        {
            Core::error("Dependency cycle detected! Processed " +
                       std::to_string(m_initializationOrder.size()) +
                       " out of " + std::to_string(m_subsystems.size()) + " subsystems");

            // Log which subsystems weren't processed
            for (const auto& [typeIndex, subsystem] : m_subsystems)
            {
                if (std::find(m_initializationOrder.begin(),
                              m_initializationOrder.end(),
                              typeIndex) == m_initializationOrder.end())
                {
                    Core::error("Subsystem not in order: " + subsystem->getName());
                }
            }

            return false;
        }

        // Log the initialization order
        Core::info("Initialization order:");
        for (size_t i = 0; i < m_initializationOrder.size(); ++i)
        {
            auto subsystem = m_subsystems[m_initializationOrder[i]];
            Core::info("  " + std::to_string(i + 1) + ". " + subsystem->getName());
        }

        return true;
    }

    bool Application::initializeSubsystem(const std::type_index& typeIndex, std::set<std::type_index>& initializedSystems)
    {
        // Check if already initialized
        if (initializedSystems.find(typeIndex) != initializedSystems.end())
        {
            return true;
        }
        
        auto subsystem = m_subsystems[typeIndex];
        info("Initializing subsystem: " + subsystem->getName());

        // Legitimate dependencies first
        auto dependencies = subsystem->getDependencies();
        for (const auto& depName : dependencies)
        {
            // Find the subsystem with this name
            auto it = std::find_if(m_subsystems.begin(), m_subsystems.end(),
                                   [&depName](const auto& pair) {
                                       return pair.second->getName() == depName;
                                   });

            if (it == m_subsystems.end())
            {
                warn("Dependency not found: " + depName + " for subsystem " + subsystem->getName());
                continue;
            }

            if (!initializeSubsystem(it->first, initializedSystems))
            {
                return false;
            }
        }

        // Initialize the subsystem
        if (!subsystem->initialize())
        {
            error("Failed to initialize subsystem: " + subsystem->getName());
            return false;
        }

        initializedSystems.insert(typeIndex);
        return true;
    }

    bool Application::setupTestVoxelWorld()
    {
        Core::info("Setting up test voxel world");

        // Get required subsystems
        auto chunkManager = getSubsystem<Voxel::ChunkManager>();
        if (!chunkManager)
        {
            Core::error("ChunkManager not found");
            return false;
        }

        // Get VoxelRenderer
        auto voxelRenderer = getSubsystem<Voxel::VoxelRenderer>();
        if (!voxelRenderer)
        {
            Core::error("VoxelRenderer not found");
            return false;
        }

        // Get RenderSystem to access LightManager
        auto renderSystem = getSubsystem<Rendering::RenderSystem>();
        if (!renderSystem)
        {
            Core::error("RenderSystem not found");
            return false;
        }

        // Create voxel grid
        m_voxelGrid = std::make_shared<Voxel::Grid>(16); // 16x16x16 chunks
        if (!m_voxelGrid->initialize())
        {
            Core::error("Failed to initialize voxel grid");
            return false;
        }
        
        // Set the grid in VoxelRenderer
        voxelRenderer->setGrid(m_voxelGrid);

        // Get the voxel material
        auto voxelMaterial = voxelRenderer->getMaterial();

        // Add test lights

        Core::info("Setting up test lights for forward+ rendering");

        auto& lightManager = renderSystem->getLightManager();

        // 1. Add a main directional light (sun)
        auto directionalLight = lightManager.createDirectionalLight();
        if (directionalLight)
        {
            directionalLight->setDirection(glm::vec3(0.5f, -1.0f, 0.3f));
            directionalLight->setColor(glm::vec3(1.0f, 0.95f, 0.8f)); // Warm sunlight
            directionalLight->setIntensity(2.0f);
            directionalLight->setCastShadows(true);
            Core::info("Created directional light (sun)");
        }

        // 2. Add a point light above the voxel platform
        auto pointLight = lightManager.createPointLight();
        if (pointLight)
        {
            pointLight->setPosition(glm::vec3(0.0f, 5.0f, 0.0f)); // Above the platform
            pointLight->setColor(glm::vec3(1.0f, 0.8f, 0.6f)); // Warm orange light
            pointLight->setIntensity(3.0f);
            pointLight->setRange(15.0f);
            pointLight->setAttenuation(1.0f, 0.1f, 0.01f); // constant, linear, quadratic
            Core::info("Created point light above platform");
        }

        // 3. Add a second point light to the side for better illumination
        auto sideLight = lightManager.createPointLight();
        if (sideLight)
        {
            sideLight->setPosition(glm::vec3(8.0f, 3.0f, -8.0f)); // Side lighting
            sideLight->setColor(glm::vec3(0.6f, 0.8f, 1.0f)); // Cool blue light
            sideLight->setIntensity(1.5f);
            sideLight->setRange(12.0f);
            sideLight->setAttenuation(1.0f, 0.15f, 0.02f);
            Core::info("Created side point light");
        }

        // 4. Add a spot light for dramatic lighting
        auto spotLight = lightManager.createSpotLight();
        if (spotLight)
        {
            spotLight->setPosition(glm::vec3(-5.0f, 8.0f, 5.0f)); // Above and to the side
            spotLight->setDirection(glm::vec3(1.0f, -1.5f, -1.0f)); // Pointing toward platform
            spotLight->setColor(glm::vec3(1.0f, 1.0f, 0.9f)); // Bright white
            spotLight->setIntensity(4.0f);
            spotLight->setRange(20.0f);
            spotLight->setInnerConeAngle(glm::radians(15.0f)); // 15 degree inner cone
            spotLight->setOuterConeAngle(glm::radians(30.0f)); // 30 degree outer cone
            spotLight->setAttenuation(1.0f, 0.05f, 0.005f);
            Core::info("Created spot light for dramatic effect");
        }

        Core::info("Created " + std::to_string(lightManager.getLightCount()) + " test lights");

        // Force light manager to update its data
        lightManager.updateLightData();

        // Original voxel creation

        // Create voxel types
        Voxel::Voxel stoneCube(1, 0);  // Type 1 = stone, data 0
        Voxel::Voxel grassCube(2, 0);  // Type 2 = grass
        Voxel::Voxel airCube(0, 0);    // Type 0 = air/empty

        // Create test geometry and track affected chunks
        auto chunkCoordComparator = [](const glm::ivec3& a, const glm::ivec3& b)
        {
            if (a.x != b.x) return a.x < b.x;
            if (a.y != b.y) return a.y < b.y;
            return a.z < b.z;
        };
        std::set<glm::ivec3, decltype(chunkCoordComparator)> affectedChunkCoords(chunkCoordComparator);
        
        // Create a small platform (10x10x2)
        for (int x = -5; x <= 5; x++)
        {
            for (int z = -5; z <= 5; z++)
            {
                m_voxelGrid->setVoxel(x, 0, z, stoneCube);
                m_voxelGrid->setVoxel(x, 1, z, grassCube);
                
                // Track which chunk this voxel belongs to
                glm::ivec3 chunkCoord(
                    x >> 4,  // x / 16
                    0,       // y / 16
                    z >> 4   // z / 16
                );
                affectedChunkCoords.insert(chunkCoord);
            }
        }

        // Create chunks in ChunkManager and generate meshes
        for (const auto& chunkCoord : affectedChunkCoords)
        {
            // Create chunk in ChunkManager
            auto chunk = chunkManager->createChunk(Voxel::ChunkCoord(chunkCoord));
            if (!chunk)
            {
                Core::error("Failed to create chunk at " + std::to_string(chunkCoord.x) + 
                           "," + std::to_string(chunkCoord.y) + 
                           "," + std::to_string(chunkCoord.z));
                continue;
            }

            // Transfer voxel data from Grid to Chunk
            int chunkSize = 16;
            glm::ivec3 chunkWorldPos = chunkCoord * chunkSize;
            
            for (int x = 0; x < chunkSize; x++)
            {
                for (int y = 0; y < chunkSize; y++)
                {
                    for (int z = 0; z < chunkSize; z++)
                    {
                        glm::ivec3 worldPos = chunkWorldPos + glm::ivec3(x, y, z);
                        Voxel::Voxel voxel = m_voxelGrid->getVoxel(worldPos);

                        if (voxel.type != 0)
                        {
                            chunk->setVoxel(x, y, z, voxel);
                        }
                    }
                }
            }

            // Generate mesh for the chunk
            chunkManager->generateChunkMesh(Voxel::ChunkCoord(chunkCoord));

            // Update the chunk in VoxelRenderer
            voxelRenderer->updateChunk(Voxel::ChunkCoord(chunkCoord));

            Core::info("Created and meshed chunk at " + std::to_string(chunkCoord.x) +
                      "," + std::to_string(chunkCoord.y) +
                      "," + std::to_string(chunkCoord.z));
        }

        Core::info("Test voxel world setup complete");
        return true;
    }

    bool Application::setupCamera()
    {
        Core::info("Setting up isometric camera");

        // Create orthographic camera for isometric view
        m_camera = std::make_shared<Rendering::Cameras::OrthographicCamera>();

        // Set up isometric view parameters - much smaller for close-up view
        float size = 10.0f; // Half-height of view
        m_camera->setSize(size);
        m_camera->setAspectRatio(16.0f / 9.0f); // Adjust to your window aspect ratio
        m_camera->setNearPlane(0.1f);
        m_camera->setFarPlane(100.0f);

        // Voxels are at (-5 to 5, 0 to 1, -5 to 5), so position camera to see them
        glm::vec3 cameraPos = glm::vec3(10.0f, 15.0f, 10.0f);
        glm::vec3 target = glm::vec3(0.0f, 0.5f, 0.0f); // Look at center of voxel platform

        m_camera->setPosition(cameraPos);
        m_camera->lookAt(target);

        // Set camera in render systems
        auto renderSystem = getSubsystem<Rendering::RenderSystem>();
        if (renderSystem)
        {
            renderSystem->setMainCamera(m_camera);
        }

        Core::info("Camera setup complete");
        return true;
    }

    void Application::createBasicMaterials()
    {
        Core::info("Creating basic materials");

        auto renderSystem = getSubsystem<Rendering::RenderSystem>();
        auto forwardPlusShaderManager = getSubsystem<Rendering::ForwardPlusShaderManager>();

        // Materials use forward+ shaders
        auto voxelShader = forwardPlusShaderManager->getForwardPlusShader("voxel_basic");
        auto voxelMaterial = renderSystem->createMaterial("voxel");
        if (voxelMaterial)
        {
            // Set basic PBR properties for a blocky, pixel art look
            voxelMaterial->setBaseColor(glm::vec3(0.8f, 0.8f, 0.8f));
            voxelMaterial->setMetallic(0.0f);
            voxelMaterial->setRoughness(0.9f); // High roughness for matte look
            voxelMaterial->setBlendMode(Rendering::BlendMode::Opaque);
            voxelMaterial->setCullMode(Rendering::CullMode::Back);
            voxelMaterial->setDepthTest(true);
            voxelMaterial->setDepthWrite(true);

            // Set the shader
            voxelMaterial->setShader(voxelShader); // Now with cluster support

            // Set in voxel renderer
            auto voxelRenderer = getSubsystem<Voxel::VoxelRenderer>();
            if (voxelRenderer)
            {
                voxelRenderer->setMaterial(voxelMaterial);
            }
        }

        // Transparent materials use forward+ transparency shader - but clean up immediately after creation
        auto transparencyShader = forwardPlusShaderManager->getForwardPlusShader("transparency");
        auto glassMaterial = renderSystem->createMaterial("glass");
        if (glassMaterial)
        {
            glassMaterial->setShader(transparencyShader);  // Transparent with lighting!
            
            // Immediately destroy the glass material since it's just for testing
            // This prevents it from staying in memory and causing leaks
            glassMaterial.reset();
        }

        Core::info("Basic materials created");
    }

    bool Application::createBasicShaders()
    {
        Core::info("Creating basic shaders for rendering");

        auto shaderManager = getSubsystem<Rendering::ShaderManager>();
        auto forwardPlusShaderManager = getSubsystem<Rendering::ForwardPlusShaderManager>();

        // Create forward+ enabled shaders (need cluster data)
        auto voxelShader = forwardPlusShaderManager->createForwardPlusShader("voxel_basic",
                                                                             "assets/shaders/voxel_basic.vert",
                                                                             "assets/shaders/voxel_basic.frag",
                                                                             Rendering::ForwardPlusShaderType::Opaque);

        auto transparencyShader = forwardPlusShaderManager->createForwardPlusShader("transparency",
                                                                                    "assets/shaders/transparency.vert",
                                                                                    "assets/shaders/transparency.frag",
                                                                                    Rendering::ForwardPlusShaderType::Transparent);

        // Create regular shaders (no lighting calculations needed)
        auto depthShader = shaderManager->createStandardShader("voxel_depth",
                                                               "assets/shaders/voxel_depth.vert",
                                                               "assets/shaders/voxel_depth.frag");

        auto shadowShader = shaderManager->createStandardShader("shadow_mapping",
                                                                "assets/shaders/shadow_mapping.vert",
                                                                "assets/shaders/shadow_mapping.frag");

        auto skyboxShader = shaderManager->createStandardShader("skybox",
                                                                "assets/shaders/skybox.vert",
                                                                "assets/shaders/skybox.frag");

        auto velocityShader = shaderManager->createStandardShader("velocity",
                                                                  "assets/shaders/velocity.vert",
                                                                  "assets/shaders/velocity.frag");

        auto uiShader = shaderManager->createStandardShader("ui",
                                                            "assets/shaders/ui.vert",
                                                            "assets/shaders/ui.frag");

        // All post-processing shaders are regular (no lighting)
        auto toneMappingShader = shaderManager->createStandardShader("postprocess_tonemap",
                                                                     "assets/shaders/postprocess_tonemap.vert",
                                                                     "assets/shaders/postprocess_tonemap.frag");

        auto paletteMappingShader = shaderManager->createStandardShader("postprocess_palette",
                                                                        "assets/shaders/postprocess_palette.vert",
                                                                        "assets/shaders/postprocess_palette.frag");

        auto ditheringShader = shaderManager->createStandardShader("postprocess_dither",
                                                                   "assets/shaders/postprocess_dither.vert",
                                                                   "assets/shaders/postprocess_dither.frag");

        auto bloomShader = shaderManager->createStandardShader("postprocess_bloom",
                                                               "assets/shaders/postprocess_bloom.vert",
                                                               "assets/shaders/postprocess_bloom.frag");

        auto fxaaShader = shaderManager->createStandardShader("postprocess_fxaa",
                                                              "assets/shaders/postprocess_fxaa.vert",
                                                              "assets/shaders/postprocess_fxaa.frag");

        auto pixelPerfectShader = shaderManager->createStandardShader("postprocess_pixelperfect",
                                                                      "assets/shaders/postprocess_pixelperfect.vert",
                                                                      "assets/shaders/postprocess_pixelperfect.frag");

        Core::info("Basic shaders created successfully");
        return true;
    }

    void Application::debugRenderingPipeline()
    {
        // auto renderSystem = getSubsystem<Rendering::RenderSystem>();
        // if (!renderSystem)
        // {
        //     Core::error("No render system for debugging");
        //     return;
        // }

        // // Check if we have an active pipeline
        // auto pipeline = renderSystem->getRenderPipeline(renderSystem->getActivePipeline());
        // if (!pipeline)
        // {
        //     Core::error("No active render pipeline");
        //     return;
        // }

        // // Log pipeline stages
        // Core::info("Active pipeline: " + pipeline->getName());
        // auto stages = pipeline->getStages();
        // Core::info("Pipeline has " + std::to_string(stages.size()) + " stages:");

        // for (const auto& stage : stages)
        // {
        //     Core::info("  - " + stage->getName() + " (" + stage->getTypeName() + ")");
        //     Core::info("    Enabled: " + std::string(stage->isEnabled() ? "YES" : "NO"));
        //     Core::info("    Initialized: " + std::string(stage->isInitialized() ? "YES" : "NO"));
        //     Core::info("    Has Output: " + std::string(stage->getOutput() ? "YES" : "NO"));
        // }

        // // Check render queues
        // Core::info("Render queue sizes:");
        // Core::info("  - Opaque: " + std::to_string(renderSystem->getOpaqueQueue().size()));
        // Core::info("  - Transparent: " + std::to_string(renderSystem->getTransparentQueue().size()));
        // Core::info("  - UI: " + std::to_string(renderSystem->getUIQueue().size()));

        // // Check camera
        // if (m_camera)
        // {
        //     auto pos = m_camera->getPosition();
        //     Core::info("Camera position: " + std::to_string(pos.x) + ", " +
        //               std::to_string(pos.y) + ", " + std::to_string(pos.z));
        // }
        // else
        // {
        //     Core::error("No main camera set");
        // }
    }

    void Application::debugVoxelRenderer()
    {
        auto app = Core::Application::getInstance();
        auto voxelRenderer = app->getSubsystem<Voxel::VoxelRenderer>();
        
        if (!voxelRenderer)
        {
            Core::error("VoxelRenderer not found!");
            return;
        }
        
        Core::info("=== VOXEL RENDERER DEBUG ===");
        Core::info("Grid set: " + std::string(voxelRenderer->getGrid() ? "YES" : "NO"));
        Core::info("Material set: " + std::string(voxelRenderer->getMaterial() ? "YES" : "NO"));
        Core::info("Visible chunks: " + std::to_string(voxelRenderer->getVisibleChunkCount()));
        Core::info("Total triangles: " + std::to_string(voxelRenderer->getTotalTriangleCount()));
        Core::info("Render distance: " + std::to_string(voxelRenderer->getRenderDistance()));
        
        if (voxelRenderer->getMaterial())
        {
            auto mat = voxelRenderer->getMaterial();
            Core::info("Material has shader: " + std::string(mat->getShader() ? "YES" : "NO"));
        }
    }

    void Application::run()
    {
        Core::info("Starting main loop");
        m_isRunning = true;

        // Get configuration values for the main loop
        auto configManager = getSubsystem<ConfigManager>();
        if (!configManager)
        {
            Core::error("ConfigManager not found, cannot run main loop");
            return;
        }

        bool useFixedTimestep = configManager->get<bool>("engine.useFixedTimestep", false);
        float fixedTimestep = configManager->get<float>("engine.fixedTimestep", 1.0f / 60.0f);
        float maxFrameTime = configManager->get<float>("engine.maxFrameTime", 0.25f);

        using clock = std::chrono::high_resolution_clock;
        auto lastFrameTime = clock::now();
        float accumulatedTime = 0.0f;

        // Get window subsystem to check for window close
        auto window = getSubsystem<Window>();

        while (m_isRunning && (!window || !window->shouldClose()))
        {
            auto currentFrameTime = clock::now();
            float frameTime = std::chrono::duration<float>(currentFrameTime - lastFrameTime).count();
            lastFrameTime = currentFrameTime;

            // Cap extremely long frames (e.g., after debugging pause)
            frameTime = std::min(frameTime, maxFrameTime);

            if (useFixedTimestep)
            {
                // Fixed timestep update
                accumulatedTime += frameTime;
                while (accumulatedTime >= fixedTimestep)
                {
                    m_deltaTime = fixedTimestep;
                    update(m_deltaTime);
                    accumulatedTime -= fixedTimestep;
                }
            }
            else
            {
                // Variable timestep update
                m_deltaTime = frameTime;
                update(m_deltaTime);
            }

            // Debug before rendering
            auto renderSystem = getSubsystem<Rendering::RenderSystem>();
            Rendering::RenderPipelineVerifier::runDiagnostics(renderSystem);

            //debugRenderingPipeline();

            debugVoxelRenderer();

            // Always render once per frame
            render();

            glfwSwapBuffers(window->getHandle());
            glfwPollEvents();

            // Optional frame rate limiter
            bool limitFrameRate = configManager->get<bool>("engine.limitFrameRate", false);
            if (limitFrameRate)
            {
                float targetFPS = configManager->get<float>("engine.targetFPS", 60.0f);
                float targetFrameTime = 1.0f / targetFPS;
                float sleepTime = targetFrameTime - frameTime;

                if (sleepTime > 0.0f)
                {
                    std::this_thread::sleep_for(std::chrono::duration<float>(sleepTime));
                }
            }
        }

        info("Main loop ended");
    }

    void Application::update(float deltaTime)
    {
        // Update all subsystems in initialization order
        for (const auto& typeIndex : m_initializationOrder)
        {
            auto subsystem = m_subsystems[typeIndex];
            subsystem->update(deltaTime);
        }

        // Update camera if we have input handling
    }

    void Application::render()
    {
        // Run progressive tests
        // if (Debug::RenderingDebugger::runProgressiveTests(nullptr))
        // {
        //     // Still in test mode
        //     return;
        // }

        // Get the window to ensure we have correct viewport
        auto window = getSubsystem<Window>();
        if (!window)
        {
            Core::error("No window subsystem available for rendering");
            return;
        }

        // Get actual window dimensions
        int width, height;
        window->getSize(width, height);

        // Set viewport
        glViewport(0, 0, width, height);

        // Clear with a visible color to verify rendering
        //glClearColor(0.2f, 0.3f, 0.4f, 1.0f);  // Dark blue-gray
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update and propagate camera matrices
        if (m_camera)
        {
            m_camera->update();

            auto renderSystem = getSubsystem<Rendering::RenderSystem>();
            if (renderSystem)
            {
                renderSystem->setMainCamera(m_camera);
                renderSystem->setViewport(0, 0, width, height);

                // Get the render context and update it
                auto& context = const_cast<Rendering::RenderContext&>(renderSystem->getRenderContext());
                context.setViewMatrix(m_camera->getViewMatrix());
                context.setProjectionMatrix(m_camera->getProjectionMatrix());
                context.setCameraPosition(m_camera->getPosition());
                context.setCameraDirection(m_camera->getForward());
                context.updateMatrices();
                context.updateFrustum();
            }
        }

        // Render all subsystems
        for (const auto& typeIndex : m_initializationOrder)
        {
            auto it = m_subsystems.find(typeIndex);
            if (it != m_subsystems.end() && it->second)
            {
                it->second->render();
            }
        }

        // Check for OpenGL errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
        {
            static int errorCount = 0;
            if (errorCount++ < 10) // Only log first 10 errors
            {
                std::stringstream ss;
                ss << "OpenGL error in render loop: 0x" << std::hex << error;
                Core::error(ss.str());
            }
        }
    }

    void Application::shutdown()
    {
        info("Shutting down PixelCraft Engine");

        // Shutdown subsystems in reverse dependency order
        for (auto it = m_initializationOrder.rbegin(); it != m_initializationOrder.rend(); it++)
        {
            auto subsystem = m_subsystems[*it];
            info("Shutting down subsystem: " + subsystem->getName());
            subsystem->shutdown();
        }

        m_subsystems.clear();
        m_initializationOrder.clear();
    }

    void Application::quit()
    {
        m_isRunning = false;
    }

    float Application::getDeltaTime() const
    {
        return m_deltaTime;
    }

} // namespace PixelCraft::Core::Application