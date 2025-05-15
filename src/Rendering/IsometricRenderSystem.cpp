// -------------------------------------------------------------------------
// IsometricRenderSystem.cpp
// -------------------------------------------------------------------------
#include "Rendering/IsometricRenderSystem.h"
#include "Core/Application.h"
#include "Core/Logger.h"
#include "Utility/SpatialPartitionFactory.h"
#include "Utility/DebugDraw.h"
#include "Utility/Math.h"
#include "Utility/Profiler.h"
#include "Rendering/Renderable.h"
#include "Rendering/Material.h"
#include "Rendering/Shader.h"
#include "Rendering/RenderTarget.h"
#include <Utility/Color.h>
#include <Utility/Transform.h>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Rendering
{

    // Helper function to convert degrees to radians
    float degreesToRadians(float degrees)
    {
        return degrees * (3.14159f / 180.0f);
    }

    IsometricRenderSystem::IsometricRenderSystem()
        : m_isometricAngle(45.0f)
        , m_pixelGridAlignmentEnabled(true)
        , m_depthSortingEnabled(true)
        , m_debugVisualizationEnabled(false)
        , m_initialized(false)
    {
        // Initialize stats
        m_stats.visibleObjects = 0;
        m_stats.drawCalls = 0;
        m_stats.triangles = 0;
        m_stats.cullingTime = 0.0f;
        m_stats.renderTime = 0.0f;
    }

    IsometricRenderSystem::~IsometricRenderSystem()
    {
        if (m_initialized)
        {
            shutdown();
        }
    }

    bool IsometricRenderSystem::initialize()
    {
        if (m_initialized)
        {
            Log::warn("IsometricRenderSystem already initialized");
            return true;
        }

        Log::info("Initializing IsometricRenderSystem");

        // Get the application instance
        auto& app = Core::Application::getInstance();

        // Get the main render system
        m_renderSystem = app.getSubsystem<RenderSystem>();
        if (!m_renderSystem)
        {
            Log::error("Failed to get RenderSystem");
            return false;
        }

        // Create isometric rendering pipeline
        m_isometricPipeline = createIsometricPipeline();
        if (!m_isometricPipeline)
        {
            Log::error("Failed to create isometric rendering pipeline");
            return false;
        }

        // Set it as the active pipeline
        m_renderSystem->setActivePipeline("IsometricPipeline");

        // Create orthographic camera
        m_camera = std::make_shared<PixelCraft::Rendering::Camera::OrthographicCamera>();
        if (!m_camera)
        {
            Log::error("Failed to create orthographic camera");
            return false;
        }

        // Configure the camera
        setupIsometricCamera();

        // Set the main camera
        m_renderSystem->setMainCamera(m_camera);

        // Create spatial partitioning for scene objects
        auto factory = app.getSubsystem<Utility::SpatialPartitionFactory>();
        if (!factory)
        {
            Log::error("Failed to get SpatialPartitionFactory");
            return false;
        }

        // Configure spatial partitioning for isometric view
        Utility::SpatialPartitionConfig config;
        config.maxDepth = 8;
        config.maxObjectsPerNode = 16;
        config.dynamicTree = true;

        // For isometric games, a quadtree if often more efficient since the viewport is fixed
        // and objects are primarily distributes across a 2D plane
        m_spatialTree = factory->createQuadtree(
            PixelCraft::Utility::AABB::AABB(glm::vec3(-1000.0f, -1000.0f, -1000.0f), glm::vec3(1000.0f, 1000.0f, 1000.0f)),
            config
        );

        if (!m_spatialTree)
        {
            Log::error("Failed to create spatial partitioning");
            return false;
        }

        m_initialized = true;
        return true;
    }

    void IsometricRenderSystem::update(float deltaTime)
    {
        if (!m_initialized)
        {
            return;
        }

        // Update camera if needed
        m_camera.update();

        // Update renderables if they have moved
        for (auto& [id, renderable] : m_renderables)
        {
            if (renderable->hasTransformChanged())
            {
                updateRenderable(id);
                renderable->clearTransformChanged();
            }
        }
    }

    void IsometricRenderSystem::render()
    {
        if (!m_initialized)
        {
            return;
        }

        Utility::Profiler::beginSample("IsometricRenderSystem.render");

        // Perform frustum culling
        Utility::Profiler::beginSample("IsometricRenderSystem.culling");
        auto visibleRenderables = getVisibleRenderables();
        m_stats.visibleObjects = static_cast<int>(visibleRenderables.size());
        Utility::Profiler::endSample();
        m_stats.cullingTime = Utility::Profiler::getSample("IsometricRenderSystem.culling").lastTime;

        // Sort transparent objects if enabled
        if (m_depthSortingEnabled)
        {
            sortByDepth(visibleRenderables);
        }

        // Apply pixel grid alignment if enabled
        if (m_pixelGridAlignmentEnabled)
        {
            for (auto& renderable : visibleRenderables)
            {
                applyPixelGridAlignment(renderable);
            }
        }

        // Reset render stats
        m_stats.drawCalls = 0;
        m_stats.triangles = 0;

        // Render visible objects
        Utility::Profiler::beginSample("IsometricRenderSystem.rendering");
        for (auto& renderable : visibleRenderables)
        {
            m_renderSystem->submitRenderable(renderable);
            m_stats.drawCalls++;
            m_stats.triangles += renderable->getTriangleCount();
        }

        // Execute the render pipeline
        m_renderSystem->render();
        Utility::Profiler::endSample();
        m_stats.renderTime = Utility::Profiler::getSample("IsometricRenderSystem.rendering").lastTime;

        // Render debug visualization if enabled
        if (m_debugVisualizationEnabled)
        {
            renderDebugVisualization();
        }

        // Collect render stats
        collectRenderStats();

        Utility::Profiler::endSample();
    }

    void IsometricRenderSystem::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        Log::info("Shutting down IsometricRenderSystem");

        // Clear renderables
        m_renderables.clear();

        // Clear spatial tree
        if (m_spatialTree)
        {
            m_spatialTree->clear();
        }

        m_initialized = false;
    }

    std::vector<std::string> IsometricRenderSystem::getDependencies() const
    {
        return {
            "RenderSystem",
            "SpatialPartitionFactory",
            "DebugDraw"
        };
    }

    uint64_t IsometricRenderSystem::registerRenderable(std::shared_ptr<Renderable> renderable)
    {
        if (!m_initialized || !renderable)
        {
            return 0;
        }

        // Generate ID if not already assigned
        uint64_t id = renderable->getID();
        if (id == 0)
        {
            // Use the spatial tree's ID generator
            id = m_spatialTree->insert(renderable);
            renderable->setID(id);
        }
        else
        {
            // Use the existing ID
            m_spatialTree->insert(renderable);
        }

        // Add to renderables map
        m_renderables[id] = renderable;

        Log::debug("Registered renderable with ID " + id);
        return id;
    }

    bool IsometricRenderSystem::unregisterRenderable(uint64_t id)
    {
        if (!m_initialized || id == 0)
        {
            return false;
        }

        // Remove from renderables map
        auto it = m_renderables.find(id);
        if (it == m_renderables.end())
        {
            return false;
        }

        // Remove from spatial tree
        m_spatialTree->remove(id);

        // Remove from map
        m_renderables.erase(it);

        Log::debug("Unregistered renderable with ID {}", id);
        return true;
    }

    bool IsometricRenderSystem::updateRenderable(uint64_t id)
    {
        if (!m_initialized || id == 0)
        {
            return false;
        }

        // Find the renderable
        auto it = m_renderables.find(id);
        if (it == m_renderables.end())
        {
            return false;
        }

        // Update in spatial tree
        return m_spatialTree->update(id);
    }

    void IsometricRenderSystem::setMainCamera(std::shared_ptr<OrthographicCamera> camera)
    {
        if (!m_initialized || !camera)
        {
            return;
        }

        m_camera = camera;
        setupIsometricCamera();

        // Update in render system
        m_renderSystem->setMainCamera(m_camera);
    }

    std::shared_ptr<OrthographicCamera> IsometricRenderSystem::getMainCamera() const
    {
        return m_camera;
    }

    void IsometricRenderSystem::setIsometricAngle(float angle)
    {
        m_isometricAngle = angle;

        if (m_initialized && m_camera)
        {
            setupIsometricCamera();
        }
    }

    float IsometricRenderSystem::getIsometricAngle() const
    {
        return m_isometricAngle;
    }

    void IsometricRenderSystem::setPixelGridAlignment(bool enabled)
    {
        m_pixelGridAlignmentEnabled = enabled;
    }

    bool IsometricRenderSystem::isPixelGridAlignmentEnabled() const
    {
        return m_pixelGridAlignmentEnabled;
    }

    void IsometricRenderSystem::setDepthSortingEnabled(bool enabled)
    {
        m_depthSortingEnabled = enabled;
    }

    bool IsometricRenderSystem::isDepthSortingEnabled() const
    {
        return m_depthSortingEnabled;
    }

    void IsometricRenderSystem::setDebugVisualizationEnabled(bool enabled)
    {
        m_debugVisualizationEnabled = enabled;

        if (m_initialized && m_spatialTree)
        {
            m_spatialTree->setDebugDrawEnabled(enabled);
        }
    }

    bool IsometricRenderSystem::isDebugVisualizationEnabled() const
    {
        return m_debugVisualizationEnabled;
    }

    std::string IsometricRenderSystem::getRenderStats() const
    {
        std::string stats = "Isometric Render Stats:\n";
        stats += "Visible Objects: " + std::to_string(m_stats.visibleObjects) + "\n";
        stats += "Draw Calls: " + std::to_string(m_stats.drawCalls) + "\n";
        stats += "Triangles: " + std::to_string(m_stats.triangles) + "\n";
        stats += "Culling Time: " + std::to_string(m_stats.cullingTime) + " ms\n";
        stats += "Render Time: " + std::to_string(m_stats.renderTime) + " ms\n";

        if (m_spatialTree)
        {
            stats += "Spatial Tree Depth: " + std::to_string(m_spatialTree->getTreeDepth()) + "\n";
            stats += "Spatial Tree Nodes: " + std::to_string(m_spatialTree->getNodeCount()) + "\n";
        }

        return stats;
    }

    std::shared_ptr<RenderPipeline> IsometricRenderSystem::createIsometricPipeline()
    {
        // Create forward rendering pipeline for isometric view
        m_renderSystem->createRenderPipeline("IsometricPipeline", m_isometricPipeline);
        auto pipeline = m_renderSystem->getRenderPipeline("IsometricPipeline");
        if (!pipeline)
        {
            return nullptr;
        }

        // Create a render target for the main output
        auto renderTarget = std::make_shared<RenderTarget>();
        renderTarget->initialize(
            //m_renderSystem->getViewport().x,
            //m_renderSystem->getViewport().y
        );

        // Set the output render target
        pipeline->setOutput(renderTarget);

        // Create shaders for isometric rendering
        auto shaderManager = m_renderSystem->getShaderManager();
        auto pixelArtShader = shaderManager->createStandardShader("PixelArt");

        // Create stages for the isometric pipeline

        // 1. Clear stage
        auto clearStage = pipeline->addStage<RenderStage>("Clear");
        clearStage->setOutput(renderTarget);
        clearStage->setParameter("clearColor", glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
        clearStage->setParameter("clearDepth", 1.0f);
        clearStage->setParameter("clearStencil", 0);

        // 2. Geometry stage for opaque objects
        auto opaqueStage = pipeline->addStage<GeometryStage>("OpaqueGeometry");
        opaqueStage->setOutput(renderTarget);
        opaqueStage->setShader(pixelArtShader);
        opaqueStage->setParameter("renderOpaqueOnly", true);
        opaqueStage->setParameter("depthTest", true);
        opaqueStage->setParameter("depthWrite", true);
        opaqueStage->setParameter("sortObjects", false);

        // 3. Geometry stage for transparent objects
        auto transparentStage = pipeline->addStage<GeometryStage>("TransparentGeometry");
        transparentStage->setOutput(renderTarget);
        transparentStage->setShader(pixelArtShader);
        transparentStage->setParameter("renderTransparentOnly", true);
        transparentStage->setParameter("depthTest", true);
        transparentStage->setParameter("depthWrite", false);
        transparentStage->setParameter("sortObjects", true);

        // 4. Post-processing stage for pixel art
        auto postProcessStage = pipeline->addStage<RenderStage>("PixelArtPostProcess");
        postProcessStage->setInput(renderTarget);
        postProcessStage->setOutput(nullptr); // Final output

        // Add specialized pixel art shaders if needed
        // ...

        return pipeline;
    }

    void IsometricRenderSystem::setupIsometricCamera()
    {
        if (!m_camera)
        {
            return;
        }

        // Get the current viewport size
        glm::vec2 viewportSize = m_renderSystem->getViewport();

        // Set the aspect ratio
        float aspectRatio = viewportSize.x / viewportSize.y;
        m_camera->setAspectRatio(aspectRatio);

        // Set orthographic size (half-height)
        float orthoSize = 10.0f;
        m_camera->setSize(orthoSize);

        // Set near and far planes
        m_camera->setNearPlane(-1000.0f);
        m_camera->setFarPlane(1000.0f);

        // Set up isometric view rotation
        // Typical isometric view is achieved with specific rotation
        // around X and Y axes
        glm::quat rotation = quat::identity();

        // First rotate around X axis (looking down from above)
        float xAngle = 35.264f; // Classic isometric angle (atan(1/sqrt(2)))
        rotation = glm::fromAxisAngle(glm::vec3(1.0f, 0.0f, 0.0f), degreesToRadians(xAngle));

        // Then rotate around Y axis by 45 degrees
        glm::quat yRot = glm::fromAxisAngle(vec3(0.0f, 1.0f, 0.0f), degreesToRadians(m_isometricAngle));
        rotation = yRot * rotation;

        m_camera->setRotation(rotation);

        // Set position based on what you want to look at
        // For now, we'll set it to look at the origin from a distance
        glm::vec3 forward = rotation * vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 position = vec3(0.0f) - forward * 100.0f;
        m_camera->setPosition(position);

        // Update the camera's view matrix
        m_camera->update();
    }

    std::vector<std::shared_ptr<Renderable>> IsometricRenderSystem::getVisibleRenderables()
    {
        std::vector<std::shared_ptr<Renderable>> visibleRenderables;

        if (!m_camera || !m_spatialTree)
        {
            return visibleRenderables;
        }

        // Get the camera's frustum for culling
        Utility::Frustum frustum = m_camera->getFrustum();

        // Query visible objects from spatial partitioning
        auto visibleObjects = m_spatialTree->queryFrustum(frustum);

        // Convert to renderables
        visibleRenderables.reserve(visibleObjects.size());
        for (auto& obj : visibleObjects)
        {
            // Since we know all objects in our tree are Renderables,
            // we can safely static_cast
            auto renderable = std::static_pointer_cast<Renderable>(obj);
            visibleRenderables.push_back(renderable);
        }

        return visibleRenderables;
    }

    void IsometricRenderSystem::sortByDepth(std::vector<std::shared_ptr<Renderable>>& renderables)
    {
        if (!m_camera || renderables.empty())
        {
            return;
        }

        // Get camera position and forward direction for depth calculation
        glm::vec3 cameraPos = m_camera->getPosition();
        glm::vec3 cameraForward = m_camera->getForward();

        // Sort renderables by depth (distance from camera plane)
        std::sort(renderables.begin(), renderables.end(),
                  [&](const std::shared_ptr<Renderable>& a, const std::shared_ptr<Renderable>& b) {
                      // Get object centers
                      glm::vec3 centerA = a->getBounds().getCenter();
                      glm::vec3 centerB = b->getBounds().getCenter();

                      // Calculate depth (distance along camera forward direction)
                      float depthA = glm::dot(centerA - cameraPos, cameraForward);
                      float depthB = glm::dot(centerB - cameraPos, cameraForward);

                      // Sort from back to front for transparent objects
                      return depthA > depthB;
                  }
        );
    }

    void IsometricRenderSystem::applyPixelGridAlignment(std::shared_ptr<Renderable> renderable)
    {
        if (!renderable)
        {
            return;
        }

        // Get renderable's world position
        glm::vec3 position = renderable->getTransform().getPosition();

        // Convert to screen space
        glm::vec3 screenPos = m_camera->worldToScreenPoint(position);

        // Round to pixel grid
        screenPos.x = std::floor(screenPos.x + 0.5f);
        screenPos.y = std::floor(screenPos.y + 0.5f);

        // Convert back to world space
        glm::vec3 alignedPos = m_camera->screenToWorldPoint(screenPos);

        // Update renderable's position
        Transform transform = renderable->getTransform();
        transform.setPosition(alignedPos);
        renderable->setTransform(transform);
    }

    void IsometricRenderSystem::renderDebugVisualization()
    {
        if (!m_initialized)
        {
            return;
        }

        auto& app = Core::Application::getInstance();
        auto debugDraw = app.getSubsystem<Utility::DebugDraw>();
        if (!debugDraw)
        {
            return;
        }

        // Draw camera frustum
        if (m_camera)
        {
            debugDraw->drawFrustum(m_camera->getFrustum(), Color(0.0f, 1.0f, 0.0f, 0.5f));
        }

        // Draw world grid (aligned to isometric view)
        float gridSize = 100.0f;
        float cellSize = 1.0f;
        int cells = static_cast<int>(gridSize / cellSize);

        glm::vec3 gridColor(0.5f, 0.5f, 0.5f);
        for (int i = -cells / 2; i <= cells / 2; ++i)
        {
            float pos = i * cellSize;

            // X axis lines
            debugDraw->drawLine(
                glm::vec3(pos, 0.0f, -gridSize / 2.0f),
                glm::vec3(pos, 0.0f, gridSize / 2.0f),
                Color(gridColor, 0.3f)
            );

            // Z axis lines
            debugDraw->drawLine(
                glm::vec3(-gridSize / 2.0f, 0.0f, pos),
                glm::vec3(gridSize / 2.0f, 0.0f, pos),
                Color(gridColor, 0.3f)
            );
        }

        // Draw coordinate axes
        debugDraw->drawAxes(Transform(), 5.0f);

        // Draw stats text
        std::string statsText = getRenderStats();
        debugDraw->draw2DText(10, 10, statsText, Color(1.0f, 1.0f, 0.0f, 1.0f));
    }

    void IsometricRenderSystem::collectRenderStats()
    {
        // Stats collection already done during rendering
    }

} // namespace PixelCraft::Rendering