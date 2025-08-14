// -------------------------------------------------------------------------
// RenderSystem.cpp
// -------------------------------------------------------------------------
#include "Rendering/RenderSystem.h"
#include "Core/Logger.h"
#include "Core/Application.h"
#include "Core/Window.h"
#include "ECS/World.h"
#include "Rendering/LightManager.h"
#include "Rendering/PaletteManager.h"
#include "Rendering/ShaderManager.h"
#include "Rendering/Renderable.h"
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"
#include "Rendering/Material.h"
#include "Rendering/Mesh.h"
#include "Rendering/RenderPipeline.h"
#include "Rendering/RenderPipelineDebugger.h"
#include "Rendering/Stages/ClusterUpdateStage.h"
#include "Rendering/Stages/DepthPrepassStage.h"
#include "Rendering/Stages/ShadowStage.h"
#include "Rendering/Stages/GeometryStage.h"
#include "Rendering/Stages/TransparencyStage.h"
#include "Rendering/Stages/PostProcessStage.h"
#include "Rendering/Stages/UIStage.h"
#include "Rendering/Cameras/OrthographicCamera.h"
#include "Utility/Frustum.h"
#include "Utility/DebugDraw.h"
#include "Utility/SpatialPartitionFactory.h"
#include "Utility/Math.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>

namespace PixelCraft::Rendering
{

    RenderSystem::RenderSystem()
        : m_viewportX(0)
        , m_viewportY(0)
        , m_viewportWidth(800)
        , m_viewportHeight(600)
        , m_isometricAngle(45.0f)
        , m_pixelGridAlignmentEnabled(true)
        , m_depthSortingEnabled(true)
        , m_debugVisualizationEnabled(true)
        , m_drawCallCount(0)
        , m_triangleCount(0)
        , m_visibleObjectCount(0)
        , m_initialized(false)
    {
    }

    RenderSystem::~RenderSystem()
    {
        if (m_initialized)
        {
            shutdown();
        }
    }

    bool RenderSystem::initialize()
    {
        Core::info("Initializing RenderSystem with forward+ pipeline");

        if (m_initialized)
        {
            Core::warn("RenderSystem already initialized");
            return true;
        }

        // Initialize clustered light culler with isometric-optimized configuration
        ClusterConfig clusterConfig;
        clusterConfig.clusterCountX = 32;
        clusterConfig.clusterCountY = 18;
        clusterConfig.clusterCountZ = 24;
        clusterConfig.nearBias = 1.3f;
        clusterConfig.enableAdaptiveDensity = true;
        clusterConfig.enableVerticalParallax = true;

        m_clusteredLightCuller = std::make_unique<ClusteredLightCuller>(clusterConfig);
        if (!m_clusteredLightCuller->initialize())
        {
            Core::error("Failed to initialize ClusteredLightCuller");
            return false;
        }

        // Initialize spatial partitioning for isometric view
        // Use generous bounds that can be expanded dynamically
        m_spatialTree = std::make_shared<Utility::Quadtree>(
            Utility::AABB2D(glm::vec2(-1000.0f), glm::vec2(1000.0f))
        );

        if (!m_spatialTree->initialize())
        {
            Core::error("Failed to initialize spatial partitioning quadtree");
            return false;
        }

        // Initialize subsystems
        m_lightManager = std::make_shared<LightManager>();
        if (!m_lightManager->initialize())
        {
            Core::error("Failed to initialize LightManager");
            return false;
        }

        m_paletteManager = std::make_shared<PaletteManager>();
        if (!m_paletteManager->initialize())
        {
            Core::error("Failed to initialize PaletteManager");
            return false;
        }

        // Create and initialize forward+ pipeline
        m_forwardPlusPipeline = std::make_shared<RenderPipeline>();
        if (!m_forwardPlusPipeline->initialize())
        {
            Core::error("Failed to initialize forward+ render pipeline");
            return false;
        }

        // Configure pipeline stages with required dependencies
        configurePipelineStages();

        // Setup pipeline debugging
        setupPipelineDebugging();

        // Pipeline debugging
        quickDebugSetup();

        // Clear render queues
        clearRenderQueue();

        // Set initial OpenGL state
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        m_initialized = true;
        
        Core::info("RenderSystem initialized successfully with forward+ pipeline");
        return true;
    }

    void RenderSystem::update(float deltaTime)
    {
        if (!m_initialized)
        {
            return;
        }

        // Update subsystems
        if (m_lightManager)
        {
            m_lightManager->update(deltaTime);
        }

        if (m_paletteManager)
        {
            m_paletteManager->update(deltaTime);
        }

        // Update render context if camera has changed
        if (m_mainCamera)
        {
            m_renderContext.setViewMatrix(m_mainCamera->getViewMatrix());
            m_renderContext.setProjectionMatrix(m_mainCamera->getProjectionMatrix());
            m_renderContext.setCameraPosition(m_mainCamera->getPosition());
            m_renderContext.setCameraDirection(m_mainCamera->getForward());
            m_renderContext.updateMatrices();
            m_renderContext.updateFrustum();
        }

        // Update render context with current viewport
        m_renderContext.setViewport(m_viewportX, m_viewportY, m_viewportWidth, m_viewportHeight);
        m_renderContext.setRenderSize(m_viewportWidth, m_viewportHeight);

        // Clear render statistics for the new frame
        m_drawCallCount = 0;
        m_triangleCount = 0;
        m_visibleObjectCount = 0;

        // Clear render queues before processing new objects
        clearRenderQueue();

        // Process registered renderables through spatial partitioning
        auto visibleRenderables = getVisibleRenderables();

        // Sort by depth if enabled
        if (m_depthSortingEnabled)
        {
            sortByDepth(visibleRenderables);
        }

        // Submit each visible renderable to the render queues
        for (const auto& renderable : visibleRenderables)
        {
            // Apply pixel grid alignment if enabled
            if (m_pixelGridAlignmentEnabled)
            {
                applyPixelGridAlignment(renderable);
            }

            // Convert to RenderableObject and submit
            RenderableObject obj = convertToRenderableObject(renderable);
            submitRenderable(obj);
        }

        // Update clustered light culler with visible lights
        if (m_clusteredLightCuller && m_lightManager && m_mainCamera)
        {
            const auto& frustum = m_renderContext.getFrustum();
            m_lightManager->updateVisibleLights(frustum);
            const auto& visibleLights = m_lightManager->getVisibleLights();
            m_clusteredLightCuller->update(m_renderContext, visibleLights);
        }

        // Update the pipeline
        if (m_forwardPlusPipeline)
        {
            m_forwardPlusPipeline->update(deltaTime);
        }

        m_visibleObjectCount = static_cast<int>(visibleRenderables.size());
    }

    void RenderSystem::render()
    {
        if (!m_initialized)
        {
            return;
        }

        // Set viewport
        glViewport(m_viewportX, m_viewportY, m_viewportWidth, m_viewportHeight);

        // Clear the frame buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Perform frustum culling on render queues
        cullObjects();

        // Sort render queues for optimal rendering
        sortRenderQueues();

        // Create shared render context for pipeline
        auto pipelineContext = std::make_shared<RenderContext>(m_renderContext);

        // Execute the render pipeline
        m_forwardPlusPipeline->render(pipelineContext);

        // Debug visualization
        if (m_debugVisualizationEnabled)
        {
            renderDebugVisualization();
        }

        // Update render stats
        updateRenderStats();
    }

    void RenderSystem::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        Core::info("Shutting down RenderSystem");

        // Clear render queues
        clearRenderQueue();

        // Clear registered renderables
        if (m_spatialTree)
        {
            m_spatialTree->clear();
        }
        m_registeredRenderables.clear();

        // Shutdown pipeline
        if (m_forwardPlusPipeline)
        {
            m_forwardPlusPipeline->shutdown();
            m_forwardPlusPipeline.reset();
        }

        // Shutdown clustered light culler
        if (m_clusteredLightCuller)
        {
            m_clusteredLightCuller->shutdown();
            m_clusteredLightCuller.reset();
        }

        // Shutdown subsystems
        if (m_paletteManager)
        {
            m_paletteManager->shutdown();
            m_paletteManager.reset();
        }

        if (m_lightManager)
        {
            m_lightManager->shutdown();
            m_lightManager.reset();
        }

        m_initialized = false;
        Core::info("RenderSystem shutdown complete");
    }

    std::vector<std::string> RenderSystem::getDependencies() const
    {
        return {"MemoryManager", "Window", "ShaderManager"};
    }

    void RenderSystem::submitRenderable(const RenderableObject& renderable)
    {
        // Skip incomplete renderables
        if (!renderable.mesh || !renderable.material)
        {
            return;
        }

        // Determine which queue to use based on the renderable's layer and material
        if (renderable.layer & static_cast<uint32_t>(RenderLayer::UI))
        {
            m_uiQueue.push_back(renderable);
        }
        else if (renderable.layer & static_cast<uint32_t>(RenderLayer::Transparent) ||
                (renderable.material && renderable.material->getBlendMode() == BlendMode::Transparent))
        {
            m_transparentQueue.push_back(renderable);
        }
        else
        {
            m_opaqueQueue.push_back(renderable);
        }
    }

    uint64_t RenderSystem::registerRenderable(std::shared_ptr<Renderable> renderable)
    {
        if (!renderable)
        {
            Core::error("Attempted to register null renderable");
            return 0;
        }

        // Generate a unique ID
        static uint64_t nextId = 1;
        uint64_t id = nextId++;

        // Store renderable
        m_registeredRenderables[id] = renderable;

        // Add to spatial tree if it has valid bounds
        if (renderable->bounds.isValid() && m_spatialTree)
        {
            // Create a spatial wrapper for the renderable
            auto spatialWrapper = std::make_shared<Utility::SpatialObjectWrapper<Renderable>>(renderable, id);
            m_spatialTree->insert(spatialWrapper);
        }

        return id;
    }

    bool RenderSystem::unregisterRenderable(uint64_t id)
    {
        auto it = m_registeredRenderables.find(id);
        if (it == m_registeredRenderables.end())
        {
            return false;
        }

        // Remove from spatial tree
        if (m_spatialTree)
        {
            m_spatialTree->remove(id);
        }

        // Remove from registered renderables
        m_registeredRenderables.erase(it);

        return true;
    }

    bool RenderSystem::updateRenderable(uint64_t id)
    {
        auto it = m_registeredRenderables.find(id);
        if (it == m_registeredRenderables.end())
        {
            return false;
        }

        // Update renderable in spatial tree
        auto renderable = it->second;
        if (renderable->bounds.isValid() && m_spatialTree)
        {
            m_spatialTree->update(id, renderable->bounds);
        }

        return true;
    }

    void RenderSystem::clearRenderQueue()
    {
        m_opaqueQueue.clear();
        m_transparentQueue.clear();
        m_uiQueue.clear();
    }

    void RenderSystem::setMainCamera(const std::shared_ptr<Rendering::Cameras::Camera> camera)
    {
        m_mainCamera = camera;

        // Update render context with camera information
        if (m_mainCamera)
        {
            m_renderContext.setViewMatrix(m_mainCamera->getViewMatrix());
            m_renderContext.setProjectionMatrix(m_mainCamera->getProjectionMatrix());
            m_renderContext.setCameraPosition(m_mainCamera->getPosition());
            m_renderContext.setCameraDirection(m_mainCamera->getForward());
            m_renderContext.setNearPlane(m_mainCamera->getNearPlane());
            m_renderContext.setFarPlane(m_mainCamera->getFarPlane());
            m_renderContext.updateMatrices();
            m_renderContext.updateFrustum();

            // If it's an orthographic camera, set up isometric view
            auto orthoCamera = std::dynamic_pointer_cast<Rendering::Cameras::OrthographicCamera>(camera);
            if (orthoCamera)
            {
                setupIsometricCamera();
            }
        }
    }

    std::shared_ptr<Rendering::Cameras::Camera> RenderSystem::getMainCamera() const
    {
        return m_mainCamera;
    }

    std::shared_ptr<RenderPipeline> RenderSystem::getRenderPipeline() const
    {
        return m_forwardPlusPipeline;
    }

    void RenderSystem::setViewport(int x, int y, int width, int height)
    {
        m_viewportX = x;
        m_viewportY = y;
        m_viewportWidth = width;
        m_viewportHeight = height;

        // Apply viewport immediately if rendering system is active
        if (m_initialized)
        {
            glViewport(x, y, width, height);
        }

        // Update render context
        m_renderContext.setViewport(x, y, width, height);
        m_renderContext.setRenderSize(width, height);

        // Resize pipeline render targets
        if (m_forwardPlusPipeline)
        {
            m_forwardPlusPipeline->resize(width, height);
        }
    }

    void RenderSystem::setIsometricAngle(float angle)
    {
        m_isometricAngle = angle;

        if (m_initialized && m_mainCamera)
        {
            setupIsometricCamera();
        }
    }

    float RenderSystem::getIsometricAngle() const
    {
        return m_isometricAngle;
    }

    void RenderSystem::setPixelGridAlignment(bool enabled)
    {
        m_pixelGridAlignmentEnabled = enabled;
    }

    bool RenderSystem::isPixelGridAlignmentEnabled() const
    {
        return m_pixelGridAlignmentEnabled;
    }

    void RenderSystem::setDepthSortingEnabled(bool enabled)
    {
        m_depthSortingEnabled = enabled;
    }

    bool RenderSystem::isDepthSortingEnabled() const
    {
        return m_depthSortingEnabled;
    }

    void RenderSystem::setDebugVisualizationEnabled(bool enabled)
    {
        m_debugVisualizationEnabled = enabled;
    }

    bool RenderSystem::isDebugVisualizationEnabled() const
    {
        return m_debugVisualizationEnabled;
    }

    void RenderSystem::toggleDebugging(bool enabled)
    {
        m_forwardPlusPipeline->setDebuggingEnabled(enabled);

        if (enabled)
        {
            Core::info("Render debugging enabled - images will be saved to debug folder");
        }
        else
        {
            Core::info("Render debugging disabled");
        }
    }

    std::shared_ptr<Shader> RenderSystem::getShader(const std::string& name)
    {
        // Delegate to ShaderManager
        auto shaderManager = Core::Application::getInstance()->getSubsystem<ShaderManager>();
        if (shaderManager)
        {
            return shaderManager->getShader(name);
        }
        return nullptr;
    }

    std::shared_ptr<Material> RenderSystem::createMaterial(const std::string& name)
    {
        return std::make_shared<Material>(name);
    }

    std::shared_ptr<Mesh> RenderSystem::createMesh(const std::string& name)
    {
        return std::make_shared<Mesh>(name);
    }

    LightManager& RenderSystem::getLightManager()
    {
        return *m_lightManager;
    }

    PaletteManager& RenderSystem::getPaletteManager()
    {
        return *m_paletteManager;
    }

    ClusteredLightCuller& RenderSystem::getClusteredLightCuller()
    {
        return *m_clusteredLightCuller;
    }

    void RenderSystem::drawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color)
    {
        // Delegate to DebugDraw subsystem
        auto debugDraw = Core::Application::getInstance()->getSubsystem<Utility::DebugDraw>();
        if (debugDraw)
        {
            debugDraw->drawLine(start, end, Utility::Color(color.r, color.g, color.b, 1.0f));
        }
    }

    void RenderSystem::drawBox(const Utility::AABB& box, const glm::vec3& color)
    {
        auto debugDraw = Core::Application::getInstance()->getSubsystem<Utility::DebugDraw>();
        if (debugDraw)
        {
            debugDraw->drawBox(box, Utility::Color(color.r, color.g, color.b, 1.0f));
        }
    }

    void RenderSystem::drawSphere(const glm::vec3& center, float radius, const glm::vec3& color)
    {
        auto debugDraw = Core::Application::getInstance()->getSubsystem<Utility::DebugDraw>();
        if (debugDraw)
        {
            debugDraw->drawSphere(center, radius, Utility::Color(color.r, color.g, color.b, 1.0f));
        }
    }

    int RenderSystem::getDrawCallCount() const
    {
        return m_drawCallCount;
    }

    int RenderSystem::getTriangleCount() const
    {
        return m_triangleCount;
    }

    int RenderSystem::getVisibleObjectCount() const
    {
        return m_visibleObjectCount;
    }

    std::string RenderSystem::getRenderStats() const
    {
        std::string stats = "Forward+ Render Stats:\n";
        stats += "Visible Objects: " + std::to_string(m_visibleObjectCount) + "\n";
        stats += "Draw Calls: " + std::to_string(m_drawCallCount) + "\n";
        stats += "Triangles: " + std::to_string(m_triangleCount) + "\n";
        stats += "Registered Objects: " + std::to_string(m_registeredRenderables.size()) + "\n";

        return stats;
    }

    const RenderContext& RenderSystem::getRenderContext() const
    {
        return m_renderContext;
    }

    const std::vector<RenderableObject>& RenderSystem::getOpaqueQueue() const
    {
        return m_opaqueQueue;
    }

    const std::vector<RenderableObject>& RenderSystem::getTransparentQueue() const
    {
        return m_transparentQueue;
    }

    const std::vector<RenderableObject>& RenderSystem::getUIQueue() const
    {
        return m_uiQueue;
    }

    void RenderSystem::sortRenderQueues()
    {
        if (!m_mainCamera)
        {
            return;
        }

        const glm::vec3& cameraPos = m_mainCamera->getPosition();

        // Sort opaque objects front to back (for early z-culling)
        std::sort(m_opaqueQueue.begin(), m_opaqueQueue.end(),
                  [&cameraPos](const RenderableObject& a, const RenderableObject& b) {
                      float distA = glm::distance(cameraPos, a.bounds.getCenter());
                      float distB = glm::distance(cameraPos, b.bounds.getCenter());
                      return distA < distB; // Front to back
                  });

        // Sort transparent objects back to front (for proper blending)
        std::sort(m_transparentQueue.begin(), m_transparentQueue.end(),
                  [&cameraPos](const RenderableObject& a, const RenderableObject& b) {
                      float distA = glm::distance(cameraPos, a.bounds.getCenter());
                      float distB = glm::distance(cameraPos, b.bounds.getCenter());
                      return distA > distB; // Back to front
                  });

        // Sort UI objects by layer
        std::sort(m_uiQueue.begin(), m_uiQueue.end(),
                  [](const RenderableObject& a, const RenderableObject& b) {
                      return a.layer < b.layer;
                  });
    }

    void RenderSystem::cullObjects()
    {
        if (!m_mainCamera)
        {
            return;
        }

        // Get the frustum from the render context
        const Utility::Frustum& frustum = m_renderContext.getFrustum();

        // Temporary storage for culled objects
        std::vector<RenderableObject> culledOpaque;
        std::vector<RenderableObject> culledTransparent;

        // UI objects are not culled

        // Cull opaque objects
        for (const auto& renderable : m_opaqueQueue)
        {
            if (frustum.testAABB(renderable.bounds))
            {
                culledOpaque.push_back(renderable);
            }
        }

        // Cull transparent objects
        for (const auto& renderable : m_transparentQueue)
        {
            if (frustum.testAABB(renderable.bounds))
            {
                culledTransparent.push_back(renderable);
            }
        }

        // Replace the original queues with the culled ones
        m_opaqueQueue = std::move(culledOpaque);
        m_transparentQueue = std::move(culledTransparent);
    }

    void RenderSystem::updateRenderStats()
    {
        // Reset statistics
        m_drawCallCount = 0;
        m_triangleCount = 0;
        
        // Aggregate stats from pipeline stages
        if (m_forwardPlusPipeline)
        {
            for (const auto& stage : m_forwardPlusPipeline->getStages())
            {
                if (stage)
                {
                    m_drawCallCount += stage->getFrameDrawCalls();
                    m_triangleCount += stage->getFrameTriangles();
                }
            }
        }
        
        // Update visible object count from render queues
        m_visibleObjectCount = static_cast<int>(m_opaqueQueue.size() +
                                                m_transparentQueue.size() +
                                                m_uiQueue.size());
    }

    RenderableObject RenderSystem::convertToRenderableObject(const std::shared_ptr<Renderable>& renderable)
    {
        RenderableObject obj;

        // Copy properties from Renderable to RenderableObject
        obj.mesh = renderable->mesh;
        obj.material = renderable->material;
        obj.transform = renderable->transform;
        obj.bounds = renderable->bounds;
        obj.layer = renderable->layer;
        obj.castShadows = renderable->castShadows;
        obj.receiveShadows = renderable->receiveShadows;

        return obj;
    }

    void RenderSystem::setupIsometricCamera()
    {
        auto orthoCamera = std::dynamic_pointer_cast<Rendering::Cameras::OrthographicCamera>(m_mainCamera);
        if (!orthoCamera)
        {
            return;
        }

        // Convert angle to radians
        float angleRadians = glm::radians(m_isometricAngle);

        // Calculate rotation quaternion
        // For isometric view, we typically rotate 45 degrees around Y axis and 30 degrees around X
        glm::quat rotation = glm::angleAxis(angleRadians, glm::vec3(0.0f, 1.0f, 0.0f)) *
                             glm::angleAxis(glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        // Set camera rotation
        orthoCamera->setRotation(rotation);
    }

    std::vector<std::shared_ptr<Renderable>> RenderSystem::getVisibleRenderables()
    {
        std::vector<std::shared_ptr<Renderable>> visibleRenderables;

        if (!m_mainCamera || !m_spatialTree)
        {
            return visibleRenderables;
        }

        // Get camera frustum
        const Utility::Frustum& frustum = m_renderContext.getFrustum();

        // Query spatial tree for potentially visible objects
        auto visibleObjects = m_spatialTree->queryFrustum(frustum);

        // Convert spatial objects back to renderables
        for (const auto& obj : visibleObjects)
        {
            auto it = m_registeredRenderables.find(obj->getID());
            if (it != m_registeredRenderables.end())
            {
                visibleRenderables.push_back(it->second);
            }
        }

        // Fallback: If spatial tree query returns nothing but we have renderables,
        // manually test each renderable against the frustum
        if (visibleRenderables.empty() && !m_registeredRenderables.empty())
        {
            for (const auto& [id, renderable] : m_registeredRenderables)
            {
                if (renderable->bounds.isValid())
                {
                    visibleRenderables.push_back(renderable);
                }
            }
        }

        return visibleRenderables;
    }

    void RenderSystem::sortByDepth(std::vector<std::shared_ptr<Renderable>>& renderables)
    {
        if (!m_mainCamera)
        {
            return;
        }

        // Get camera position
        glm::vec3 cameraPos = m_mainCamera->getPosition();

        // Sort by distance from camera (back to front for transparency)
        std::sort(renderables.begin(), renderables.end(),
                [&cameraPos](const std::shared_ptr<Renderable>& a, const std::shared_ptr<Renderable>& b) {
                    glm::vec3 posA = a->getPosition();
                    glm::vec3 posB = b->getPosition();

                    float distA = glm::distance(cameraPos, posA);
                    float distB = glm::distance(cameraPos, posB);

                    return distA > distB; // Further objects first (back to front)
                }
        );
    }

    void RenderSystem::applyPixelGridAlignment(std::shared_ptr<Renderable> renderable)
    {
        auto orthoCamera = std::dynamic_pointer_cast<Rendering::Cameras::OrthographicCamera>(m_mainCamera);
        if (!orthoCamera || !renderable)
        {
            return;
        }

        // Get object position in world space
        glm::vec3 position = renderable->getPosition();

        // Convert to screen space
        glm::vec2 screenPos = orthoCamera->worldToScreenPoint(position);

        // Round to integer pixels
        screenPos = glm::round(screenPos);

        // Convert back to world space
        glm::vec3 alignedPos = orthoCamera->screenToWorldPoint(screenPos);

        // Update position
        renderable->setPosition(alignedPos);
    }

    void RenderSystem::renderDebugVisualization()
    {
        if (!m_mainCamera)
        {
            return;
        }

        // Draw camera frustum
        const Utility::Frustum& frustum = m_renderContext.getFrustum();

        // Draw frustum lines
        auto corners = frustum.getCorners();
        for (int i = 0; i < 4; i++)
        {
            int j = (i + 1) % 4;

            // Near plane
            drawLine(corners[i], corners[j], glm::vec3(0.0f, 1.0f, 0.0f));

            // Far plane
            drawLine(corners[i + 4], corners[j + 4], glm::vec3(0.0f, 1.0f, 0.0f));

            // Edges
            drawLine(corners[i], corners[i + 4], glm::vec3(0.0f, 1.0f, 0.0f));
        }

        // Draw registered renderable bounds
        for (const auto& [id, renderable] : m_registeredRenderables)
        {
            if (renderable->bounds.isValid())
            {
                drawBox(renderable->bounds, glm::vec3(1.0f, 0.5f, 0.0f));
            }
        }
    }

    void RenderSystem::setupPipelineDebugging()
    {
        // Enable global debugging
        m_forwardPlusPipeline->setDebuggingEnabled(true);

        // Get the debugger for more advanced configuration
        auto debugger = m_forwardPlusPipeline->getDebugger();

        // Set output directory for debug images
        debugger->setOutputDirectory("debug_renders/");

        // Capture only every 10th frame to avoid too many files
        debugger->setCaptureInterval(10);

        // Limit to 100 frames total
        debugger->setMaxFramesToCapture(100);

        // Enable debugging for specific stages you want to inspect

        // 1. Check cluster update stage (won't capture images but will log info)
        m_forwardPlusPipeline->enableStageDebugging("ClusterUpdateStage", false, false);

        // 2. Check depth prepass - this should show white where geometry is rendered
        m_forwardPlusPipeline->enableStageDebugging("DepthPrepassStage", false, true);  // Only depth

        // 3. Check geometry stage - this should show your lit scene
        m_forwardPlusPipeline->enableStageDebugging("GeometryStage", true, false);      // Only color

        // 4. Check transparency stage if you have transparent objects
        m_forwardPlusPipeline->enableStageDebugging("TransparencyStage", true, false);  // Only color

        // 5. Check post-processing output
        m_forwardPlusPipeline->enableStageDebugging("PostProcessStage", true, false);   // Only color

        Core::info("Pipeline debugging configured - check debug_renders/ folder for output");
    }

    void RenderSystem::diagnosePipelineIssues()
    {
        auto debugger = m_forwardPlusPipeline->getDebugger();

        // After running a few frames, check what was captured
        Core::info(debugger->getCaptureStatistics());

        // Common issues to look for:
    
        // 1. If DepthPrepassStage depth images are all black:
        //    - No geometry is being rendered in depth prepass
        //    - Check if your depth shader is working
        //    - Check if ECS entities have valid meshes and transforms
        
        // 2. If GeometryStage color images are all black:
        //    - Depth prepass might have failed (no early-Z working)
        //    - Forward+ lighting data might not be bound correctly
        //    - Shaders might not be receiving correct uniforms
        //    - Materials might not be bound correctly
        
        // 3. If PostProcessStage input looks good but output is black:
        //    - Post-processing shaders might have issues
        //    - Tone mapping might be incorrect
        //    - Final blit to screen might be failing
    }

    void RenderSystem::quickDebugSetup()
    {
        // Enable debugging with default settings for all major stages
        m_forwardPlusPipeline->setDebuggingEnabled(true);

        // Capture every frame for first 5 frames to see immediate results
        auto debugger = m_forwardPlusPipeline->getDebugger();
        debugger->setCaptureInterval(1);
        debugger->setMaxFramesToCapture(5);
        debugger->setOutputDirectory("quick_debug/");

        // Enable all important stages
        m_forwardPlusPipeline->enableStageDebugging("DepthPrepassStage", true, true);    // Both color and depth
        m_forwardPlusPipeline->enableStageDebugging("GeometryStage", true, true);        // Both color and depth
        m_forwardPlusPipeline->enableStageDebugging("TransparencyStage", true, false);   // Color only
        m_forwardPlusPipeline->enableStageDebugging("PostProcessStage", true, false);    // Color only

        Core::info("Quick debug setup complete - will capture first 5 frames");
    }

    void RenderSystem::configureAdvancedDebugging()
    {
        auto debugger = m_forwardPlusPipeline->getDebugger();

        // Create custom configuration for a specific stage
        StageDebugConfig customConfig;
        customConfig.enabled = true;
        customConfig.captureColor = true;
        customConfig.captureDepth = true;
        customConfig.captureStencil = false;
        customConfig.colorAttachmentIndex = 0;  // Capture first color attachment
        customConfig.outputPath = "custom_debug/geometry/";
        customConfig.filenamePrefix = "geo_stage";

        debugger->configureStage("GeometryStage", customConfig);

        // You can also capture multiple color attachments if your render targets have them
        StageDebugConfig mrtConfig;
        mrtConfig.enabled = true;
        mrtConfig.captureColor = true;
        mrtConfig.colorAttachmentIndex = 1;  // Capture second color attachment
        mrtConfig.outputPath = "custom_debug/geometry_mrt/";
        mrtConfig.filenamePrefix = "geo_mrt";

        debugger->configureStage("GeometryStage_MRT", mrtConfig);
    }

    void RenderSystem::collectRenderStats()
    {
        // Update stats
        m_visibleObjectCount = static_cast<int>(m_opaqueQueue.size() +
                                                m_transparentQueue.size() +
                                                m_uiQueue.size());
    }

    void RenderSystem::configurePipelineStages()
    {
        // Get ECS World to access registry
        auto ecsWorld = Core::Application::getInstance()->getSubsystem<ECS::World>();
        auto registry = ecsWorld ? ecsWorld->getRegistry() : nullptr;

        // Configure each stage with necessary dependencies
        for (const auto& stage : m_forwardPlusPipeline->getStages())
        {
            // ClusterUpdateStage needs clustered light culler
            if (auto clusterStage = std::dynamic_pointer_cast<Stages::ClusterUpdateStage>(stage))
            {
                clusterStage->setClusterDimensions(m_clusteredLightCuller->getClusterData().dimensions);
                clusterStage->setLightManager(m_lightManager);
            }

            // DepthPrepassStage needs registry
            else if (auto depthStage = std::dynamic_pointer_cast<Stages::DepthPrepassStage>(stage))
            {
                depthStage->setRegistry(registry);
            }

            // ShadowStage needs light manager and registry
            else if (auto shadowStage = std::dynamic_pointer_cast<Stages::ShadowStage>(stage))
            {
                shadowStage->setLightManager(m_lightManager);
                shadowStage->setRegistry(registry);
            }

            // GeometryStage needs registry and render system
            else if (auto geoStage = std::dynamic_pointer_cast<Stages::GeometryStage>(stage))
            {
                geoStage->setRegistry(registry);
                geoStage->setRenderSystem(this);
            }

            // TransparencyStage needs registry
            else if (auto transStage = std::dynamic_pointer_cast<Stages::TransparencyStage>(stage))
            {
                transStage->setRegistry(registry);
            }

            // PostProcessStage needs palette manager
            else if (auto postStage = std::dynamic_pointer_cast<Stages::PostProcessStage>(stage))
            {
                postStage->setPaletteManager(m_paletteManager);
            }

            // UIStage needs registry
            else if (auto uiStage = std::dynamic_pointer_cast<Stages::UIStage>(stage))
            {
                uiStage->setRegistry(registry);
            }
        }
    }

} // namespace PixelCraft::Rendering
