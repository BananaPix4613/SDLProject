// -------------------------------------------------------------------------
// RenderSystem.cpp
// -------------------------------------------------------------------------
#include "Rendering/RenderSystem.h"
#include "Core/Logger.h"
#include "Core/Application.h"
#include "Core/ResourceManager.h"
#include "Rendering/RenderContext.h"
#include "Rendering/LightManager.h"
#include "Rendering/PaletteManager.h"
#include "Utility/LineBatchRenderer.h"
#include "Utility/FileSystem.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace PixelCraft::Rendering
{

    RenderSystem::RenderSystem()
        : m_viewportX(0)
        , m_viewportY(0)
        , m_viewportWidth(0)
        , m_viewportHeight(0)
        , m_drawCallCount(0)
        , m_triangleCount(0)
        , m_visibleObjectCount(0)
    {
        // Constructor implementation
        Log::debug("RenderSystem constructor called");
    }

    RenderSystem::~RenderSystem()
    {
        if (m_initialized)
        {
            shutdown();
        }
        Log::debug("RenderSystem destroyed");
    }

    bool RenderSystem::initialize()
    {
        if (m_initialized)
        {
            Log::warn("RenderSystem already initialized");
            return true;
        }

        Log::info("Initializing RenderSystem");

        // Initialize subsystems
        m_lightManager = std::make_unique<LightManager>();
        if (!m_lightManager->initialize())
        {
            Log::error("Failed to initialize LightManager");
            return false;
        }

        m_paletteManager = std::make_unique<PaletteManager>();
        if (!m_paletteManager->initialize())
        {
            Log::error("Failed to initialize PaletteManager");
            return false;
        }

        // Initialize debug renderer
        m_debugRenderer = std::make_unique<Utility::LineBatchRenderer>();
        if (!m_debugRenderer->initialize())
        {
            Log::error("Failed to initialize debug renderer");
            return false;
        }

        // Set default viewport to match window size
        App* app = Core::Application::getInstance();
        int width, height;
        Core::Application::getInstance().getWindowSize(width, height);
        setViewport(0, 0, width, height);

        // Initialize render context
        m_renderContext = RenderContext();
        m_renderContext.updateMatrices();

        // Clear render queues
        clearRenderQueue();

        // Set up default rendering pipeline
        auto defaultPipeline = std::make_shared<RenderPipeline>();
        if (!defaultPipeline->initialize())
        {
            Log::error("Failed to initialize default render pipeline");
            return false;
        }

        createRenderPipeline("Default", defaultPipeline);
        setActivePipeline("Default");

        m_initialized = true;
        Log::info("RenderSystem initialized successfully");
        return true;
    }

    void RenderSystem::update(float deltaTime)
    {
        if (!m_initialized)
        {
            return;
        }

        // Update light manager
        m_lightManager->update(deltaTime);

        // Update debug renderer
        m_debugRenderer->update(deltaTime);

        // Update active render pipeline
        auto pipeline = getRenderPipeline(m_activePipeline);
        if (pipeline)
        {
            pipeline->update(deltaTime);
        }

        // Clear render statistics for the new frame
        m_drawCallCount = 0;
        m_triangleCount = 0;
        m_visibleObjectCount = 0;
    }

    void RenderSystem::render()
    {
        if (!m_initialized)
        {
            return;
        }

        // Set viewport
        // In a real implementation, this would call the appropriate graphics API
        // to set the actual viewport dimensions

        // Update frustum for culling
        m_renderContext.updateMatrices();

        // Perform frustum culling
        cullObjects();

        // Sort render queues for optimal rendering
        sortRenderQueues();

        // Get active pipeline
        auto pipeline = getRenderPipeline(m_activePipeline);
        if (!pipeline)
        {
            Log::error("No active render pipeline set");
            return;
        }

        // Configure pipeline with current render context
        pipeline->setRenderContext(m_renderContext);

        // Render opaque objects first
        submitQueueToRenderer(m_opaqueQueue);

        // Render transparent objects (back to front)
        submitQueueToRenderer(m_transparentQueue);

        // Render UI elements
        submitQueueToRenderer(m_uiQueue);

        // Render debug visualizations
        m_debugRenderer->render(m_renderContext);

        // Update render statistics
        updateRenderStats();
    }

    void RenderSystem::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        Log::info("Shutting down RenderSystem");

        // Clear render queues
        clearRenderQueue();

        // Clear pipelines
        m_pipelines.clear();
        m_activePipeline = "";

        // Shutdown subsystems in reverse order of initialization
        if (m_debugRenderer)
        {
            m_debugRenderer->shutdown();
            m_debugRenderer.reset();
        }

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
        Log::info("RenderSystem shutdown complete");
    }

    std::vector<std::string> RenderSystem::getDependencies() const
    {
        // Return dependencies
        return {
            "ResourceManager",
            "EventSystem"
        };
    }

    void RenderSystem::createRenderPipeline(const std::string& name, std::shared_ptr<RenderPipeline> pipeline)
    {
        if (m_pipelines.find(name) != m_pipelines.end())
        {
            Log::warn("Render pipeline '" + name + "' already exists, replacing");
        }

        m_pipelines[name] = pipeline;
        Log::debug("Created render pipeline: " + name);
    }

    std::shared_ptr<RenderPipeline> RenderSystem::getRenderPipeline(const std::string& name)
    {
        auto it = m_pipelines.find(name);
        if (it == m_pipelines.end())
        {
            Log::warn("Render pipeline '" + name + "' not found");
            return nullptr;
        }

        return it->second;
    }

    void RenderSystem::setActivePipeline(const std::string& name)
    {
        if (m_pipelines.find(name) == m_pipelines.end())
        {
            Log::error("Cannot set active pipeline: '" + name + "' not found");
            return;
        }

        m_activePipeline = name;
        Log::debug("Active pipeline set to: " + name);
    }

    const std::string& RenderSystem::getActivePipeline() const
    {
        return m_activePipeline;
    }

    void RenderSystem::submitRenderable(const RenderableObject& renderable)
    {
        if (!renderable.mesh || !renderable.material)
        {
            // Skip incomplete renderables
            return;
        }

        // Determine which queue to use based on the object's layer
        if (renderable.layer & static_cast<uint32_t>(RenderLayer::UI))
        {
            m_uiQueue.push_back(renderable);
        }
        else if (renderable.material->isTransparent())
        {
            m_transparentQueue.push_back(renderable);
        }
        else
        {
            m_opaqueQueue.push_back(renderable);
        }
    }

    void RenderSystem::clearRenderQueue()
    {
        m_opaqueQueue.clear();
        m_transparentQueue.clear();
        m_uiQueue.clear();
    }

    void RenderSystem::setMainCamera(const Camera& camera)
    {
        // Update render context with camera information
        m_renderContext.setViewMatrix(camera.getViewMatrix());
        m_renderContext.setProjectionMatrix(camera.getProjectionMatrix());
        m_renderContext.setCameraPosition(camera.getPosition());
        m_renderContext.updateMatrices();
    }

    void RenderSystem::setViewport(int x, int y, int width, int height)
    {
        m_viewportX = x;
        m_viewportY = y;
        m_viewportWidth = width;
        m_viewportHeight = height;

        // Update render context with new viewport dimensions
        m_renderContext.setRenderSize(width, height);

        // In a real implementation, this would call the appropriate graphics API
        // to set the actual viewport dimensions

        Log::debug("Viewport set to: " + std::to_string(x) + ", " +
                   std::to_string(y) + ", " + std::to_string(width) +
                   ", " + std::to_string(height));
    }

    std::shared_ptr<Shader> RenderSystem::createShader(const std::string& name)
    {
        // Get the resource manager
        auto* resourceManager = Core::Application::getInstance()->getSubsystem<Core::ResourceManager>();
        if (!resourceManager)
        {
            Log::error("Failed to get ResourceManager for shader creation");
            return nullptr;
        }

        // Create or get the shader resource
        auto shader = resourceManager->createResource<Shader>(name);
        if (!shader)
        {
            Log::error("Failed to create shader: " + name);
            return nullptr;
        }

        return shader;
    }

    std::shared_ptr<Texture> RenderSystem::createTexture(const std::string& name)
    {
        // Get the resource manager
        auto* resourceManager = Core::Application::getInstance()->getSubsystem<Core::ResourceManager>();
        if (!resourceManager)
        {
            Log::error("Failed to get ResourceManager for texture creation");
            return nullptr;
        }

        // Create or get the texture resource
        auto texture = resourceManager->createResource<Texture>(name);
        if (!texture)
        {
            Log::error("Failed to create texture: " + name);
            return nullptr;
        }

        return texture;
    }

    std::shared_ptr<Material> RenderSystem::createMaterial(const std::string& name)
    {
        // Get the resource manager
        auto* resourceManager = Core::Application::getInstance()->getSubsystem<Core::ResourceManager>();
        if (!resourceManager)
        {
            Log::error("Failed to get ResourceManager for material creation");
            return nullptr;
        }

        // Create or get the material resource
        auto material = resourceManager->createResource<Material>(name);
        if (!material)
        {
            Log::error("Failed to create material: " + name);
            return nullptr;
        }

        return material;
    }

    std::shared_ptr<Mesh> RenderSystem::createMesh(const std::string& name)
    {
        // Get the resource manager
        auto* resourceManager = Core::Application::getInstance()->getSubsystem<Core::ResourceManager>();
        if (!resourceManager)
        {
            Log::error("Failed to get ResourceManager for mesh creation");
            return nullptr;
        }

        // Create or get the mesh resource
        auto mesh = resourceManager->createResource<Mesh>(name);
        if (!mesh)
        {
            Log::error("Failed to create mesh: " + name);
            return nullptr;
        }

        return mesh;
    }

    LightManager& RenderSystem::getLightManager()
    {
        return *m_lightManager;
    }

    PaletteManager& RenderSystem::getPaletteManager()
    {
        return *m_paletteManager;
    }

    void RenderSystem::drawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color)
    {
        if (m_debugRenderer)
        {
            m_debugRenderer->drawLine(start, end, color);
        }
    }

    void RenderSystem::drawBox(const Utility::AABB& box, const glm::vec3& color)
    {
        if (m_debugRenderer)
        {
            // Get the min and max points of the AABB
            const glm::vec3& min = box.getMin();
            const glm::vec3& max = box.getMax();

            // Draw the 12 edges of the box
            // Bottom face
            drawLine(glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, min.y, min.z), color);
            drawLine(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, min.y, max.z), color);
            drawLine(glm::vec3(max.x, min.y, max.z), glm::vec3(min.x, min.y, max.z), color);
            drawLine(glm::vec3(min.x, min.y, max.z), glm::vec3(min.x, min.y, min.z), color);

            // Top face
            drawLine(glm::vec3(min.x, max.y, min.z), glm::vec3(max.x, max.y, min.z), color);
            drawLine(glm::vec3(max.x, max.y, min.z), glm::vec3(max.x, max.y, max.z), color);
            drawLine(glm::vec3(max.x, max.y, max.z), glm::vec3(min.x, max.y, max.z), color);
            drawLine(glm::vec3(min.x, max.y, max.z), glm::vec3(min.x, max.y, min.z), color);

            // Connecting edges
            drawLine(glm::vec3(min.x, min.y, min.z), glm::vec3(min.x, max.y, min.z), color);
            drawLine(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, max.y, min.z), color);
            drawLine(glm::vec3(max.x, min.y, max.z), glm::vec3(max.x, max.y, max.z), color);
            drawLine(glm::vec3(min.x, min.y, max.z), glm::vec3(min.x, max.y, max.z), color);
        }
    }

    void RenderSystem::drawSphere(const glm::vec3& center, float radius, const glm::vec3& color)
    {
        if (m_debugRenderer)
        {
            // Draw a sphere using lines
            // We'll approximate a sphere by drawing three circles in the XY, XZ, and YZ planes
            const int segments = 32;
            const float step = 2.0f * 3.14159f / segments;

            // XY plane circle
            for (int i = 0; i < segments; ++i)
            {
                float angle1 = i * step;
                float angle2 = (i + 1) * step;

                glm::vec3 p1 = center + radius * glm::vec3(std::cos(angle1), std::sin(angle1), 0.0f);
                glm::vec3 p2 = center + radius * glm::vec3(std::cos(angle2), std::sin(angle2), 0.0f);
                drawLine(p1, p2, color);
            }

            // XZ plane circle
            for (int i = 0; i < segments; ++i)
            {
                float angle1 = i * step;
                float angle2 = (i + 1) * step;

                glm::vec3 p1 = center + radius * glm::vec3(std::cos(angle1), 0.0f, std::sin(angle1));
                glm::vec3 p2 = center + radius * glm::vec3(std::cos(angle2), 0.0f, std::sin(angle2));
                drawLine(p1, p2, color);
            }

            // YZ plane circle
            for (int i = 0; i < segments; ++i)
            {
                float angle1 = i * step;
                float angle2 = (i + 1) * step;

                glm::vec3 p1 = center + radius * glm::vec3(0.0f, std::cos(angle1), std::sin(angle1));
                glm::vec3 p2 = center + radius * glm::vec3(0.0f, std::cos(angle2), std::sin(angle2));
                drawLine(p1, p2, color);
            }
        }
    }

    void RenderSystem::takeScreenshot(const std::string& filepath)
    {
        // In a real implementation, this would:
        // 1. Read the framebuffer pixels
        // 2. Create an image from the pixel data
        // 3. Save the image to the specified filepath

        Log::info("Taking screenshot: " + filepath);

        // Placeholder implementation
        // In a real engine, this would use the graphics API to read pixels and save them

        // Example pseudo-code:
        // unsigned char* pixels = new unsigned char[m_viewportWidth * m_viewportHeight * 4];
        // glReadPixels(0, 0, m_viewportWidth, m_viewportHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        // Utility::saveImage(filepath, pixels, m_viewportWidth, m_viewportHeight, 4);
        // delete[] pixels;

        Log::info("Screenshot saved to: " + filepath);
    }

    void RenderSystem::renderToTexture(std::shared_ptr<Texture> texture)
    {
        if (!texture)
        {
            Log::error("Cannot render to null texture");
            return;
        }

        // In a real implementation, this would:
        // 1. Create a framebuffer object
        // 2. Attach the texture as a color attachment
        // 3. Set the framebuffer as the render target
        // 4. Render the scene
        // 5. Reset to the default framebuffer

        Log::debug("Rendering to texture: " + texture->getName());

        // Placeholder implementation
        // In a real engine, this would use framebuffer objects to render the scene to a texture

        // Example pseudo-code:
        // GLuint fbo;
        // glGenFramebuffers(1, &fbo);
        // glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->getTextureID(), 0);
        // glViewport(0, 0, texture->getWidth(), texture->getHeight());
        // render();
        // glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // glViewport(m_viewportX, m_viewportY, m_viewportWidth, m_viewportHeight);
        // glDeleteFramebuffers(1, &fbo);
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

    void RenderSystem::sortRenderQueues()
    {
        // Sort opaque objects front to back (for early z-culling)
        std::sort(m_opaqueQueue.begin(), m_opaqueQueue.end(),
                  [this](const RenderableObject& a, const RenderableObject& b) {
                      // Get the distance from the camera to the object centers
                      float distA = glm::length(m_renderContext.getCameraPosition() - a.bounds.getCenter());
                      float distB = glm::length(m_renderContext.getCameraPosition() - b.bounds.getCenter());

                      // Sort front to back (smaller distance first)
                      return distA < distB;
                  });

        // Sort transparent objects back to front (for proper blending)
        std::sort(m_transparentQueue.begin(), m_transparentQueue.end(),
                  [this](const RenderableObject& a, const RenderableObject& b) {
                      // Get the distance from the camera to the object centers
                      float distA = glm::length(m_renderContext.getCameraPosition() - a.bounds.getCenter());
                      float distB = glm::length(m_renderContext.getCameraPosition() - b.bounds.getCenter());

                      // Sort back to front (larger distance first)
                      return distA > distB;
                  });

        // Sort UI objects by layer
        std::sort(m_uiQueue.begin(), m_uiQueue.end(),
                  [](const RenderableObject& a, const RenderableObject& b) {
                      return a.layer < b.layer;
                  });
    }

    void RenderSystem::submitQueueToRenderer(const std::vector<RenderableObject>& queue)
    {
        if (queue.empty())
        {
            return;
        }

        // In a real implementation, this would iterate through the queue
        // and render each object using its material and mesh

        // Get the active pipeline
        auto pipeline = getRenderPipeline(m_activePipeline);
        if (!pipeline)
        {
            Log::error("No active render pipeline set");
            return;
        }

        // Track the current material to minimize state changes
        std::shared_ptr<Material> currentMaterial = nullptr;

        // Render all objects in the queue
        for (const auto& renderable : queue)
        {
            // Skip objects without mesh or material
            if (!renderable.mesh || !renderable.material)
            {
                continue;
            }

            // Bind material if different from current
            if (currentMaterial != renderable.material)
            {
                if (currentMaterial)
                {
                    currentMaterial->unbind();
                }

                renderable.material->bind();
                currentMaterial = renderable.material;
            }

            // Set object-specific parameters
            currentMaterial->setParameter("model", renderable.transform);

            // Set shadow parameters if applicable
            if (renderable.receiveShadows)
            {
                m_lightManager->bindShadowMaps(*currentMaterial);
            }

            // Bind and draw the mesh
            renderable.mesh->bind();
            renderable.mesh->draw();
            renderable.mesh->unbind();

            // Update statistics
            m_drawCallCount++;
            m_triangleCount += renderable.mesh->getTriangleCount();
        }

        // Unbind the last material
        if (currentMaterial)
        {
            currentMaterial->unbind();
        }
    }

    void RenderSystem::cullObjects()
    {
        // Get the frustum from the render context
        const Utility::Frustum& frustum = m_renderContext.getFrustum();

        // Temporary storage for culled objects
        std::vector<RenderableObject> culledOpaque;
        std::vector<RenderableObject> culledTransparent;

        // UI objects are not culled as they are in screen space

        // Cull opaque objects
        culledOpaque.reserve(m_opaqueQueue.size());
        for (const auto& renderable : m_opaqueQueue)
        {
            if (frustum.testAABB(renderable.bounds.getMin(), renderable.bounds.getMax()))
            {
                culledOpaque.push_back(renderable);
            }
        }

        // Cull transparent objects
        culledTransparent.reserve(m_transparentQueue.size());
        for (const auto& renderable : m_transparentQueue)
        {
            if (frustum.testAABB(renderable.bounds.getMin(), renderable.bounds.getMax()))
            {
                culledTransparent.push_back(renderable);
            }
        }

        // Update queues with culled objects
        m_opaqueQueue = std::move(culledOpaque);
        m_transparentQueue = std::move(culledTransparent);

        // Update visible object count
        m_visibleObjectCount = m_opaqueQueue.size() + m_transparentQueue.size() + m_uiQueue.size();
    }

    void RenderSystem::updateRenderStats()
    {
        // In a real implementation, this would update engine profiling metrics
        // or debug information for monitoring performance

        // Example: Update FPS counter, memory usage, etc.
    }

} // namespace PixelCraft::Rendering