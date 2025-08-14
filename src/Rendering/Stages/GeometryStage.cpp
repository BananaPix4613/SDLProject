// -------------------------------------------------------------------------
// GeometryStage.cpp
// -------------------------------------------------------------------------
#include "Rendering/Stages/GeometryStage.h"
#include "Rendering/Shader.h"
#include "Rendering/RenderContext.h"
#include "Rendering/RenderSystem.h"
#include "Rendering/ForwardPlusShaderManager.h"
#include "Rendering/Material.h"
#include "Rendering/Mesh.h"
#include "Core/Application.h"
#include "Core/Logger.h"
#include "Utility/Profiler.h"
#include "Utility/AABB.h"
#include "ECS/Registry.h"
#include "ECS/View.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshRendererComponent.h"

#include <algorithm>
#include <unordered_map>
#include <glad/glad.h>

namespace PixelCraft::Rendering::Stages
{

    GeometryStage::GeometryStage()
        : RenderStage("GeometryStage")
    {
    }

    GeometryStage::~GeometryStage()
    {
        if (isInitialized())
        {
            shutdown();
        }
    }

    bool GeometryStage::initialize()
    {
        if (isInitialized())
        {
            return true;
        }

        Core::info("Initializing GeometryStage with ForwardPlusShaderManager integration");

        // Verify ForwardPlusShaderManager is available
        if (!m_forwardPlusShaderManager)
        {
            Core::warn("GeometryStage: No ForwardPlusShaderManager set during initialization");
        }

        setInitialized(true);
        Core::info("GeometryStage initialized successfully with forward+ integration");
        return true;
    }

    void GeometryStage::shutdown()
    {
        if (!isInitialized())
        {
            return;
        }

        Core::info("Shutting down GeometryStage");

        m_registry.reset();
        m_forwardPlusShaderManager.reset();

        setInitialized(false);
    }

    void GeometryStage::execute(const std::shared_ptr<RenderContext> context)
    {
        if (!isInitialized() || !isEnabled())
        {
            Core::error("GeometryStage: Not initialized or enabled");
            return;
        }

        // Validate render context
        if (!context)
        {
            Core::error("GeometryStage: Invalid render context provided");
            return;
        }

        PROFILE_SCOPE("GeometryStage::execute");

        // Reset frame statistics
        m_frameDrawCalls = 0;
        m_frameTriangles = 0;

        // Bind output render target (SceneTarget)
        bindOutput();

        // Validate that output binding was successful before proceeding
        GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
        {
            Core::error("GeometryStage: Output framebuffer is not complete after binding (status: " + std::to_string(framebufferStatus) + ") - skipping geometry rendering");
            return;
        }

        // Clear any OpenGL errors from previous operations
        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
        {
            Core::error("GeometryStage: OpenGL error before setup (" + std::to_string(error) + ") - clearing and continuing");
        }

        // Clear only color, shared depth is preserved automatically
        if (hasParameter("clearColor"))
        {
            bool clearColor = getParameter<bool>("clearColor", true);
            clearOutput(clearColor, false, false); // Don't clear shared depth
        }

        // Setup render state - this should now be safe
        setupRenderState();

        // Render geometry with forward+ shading
        // Early-z rejection works perfectly since we're using the same depth texture
        // that was written to in the DepthPrepass stage
        renderGeometry(context);

        // Restore render state
        restoreRenderState();

        // Unbind output render target
        unbindOutput();
    }

    void GeometryStage::setForwardPlusShaderManager(std::shared_ptr<ForwardPlusShaderManager> shaderManager)
    {
        m_forwardPlusShaderManager = shaderManager;
    }

    std::shared_ptr<ForwardPlusShaderManager> GeometryStage::getForwardPlusShaderManager() const
    {
        return m_forwardPlusShaderManager;
    }

    void GeometryStage::setRegistry(std::shared_ptr<ECS::Registry> registry)
    {
        m_registry = registry;
    }

    void GeometryStage::setRenderSystem(RenderSystem* renderSystem)
    {
        m_renderSystem = renderSystem;
    }

    void GeometryStage::setRenderOpaque(bool opaque)
    {
        m_renderOpaque = opaque;
        if (opaque)
        {
            m_renderTransparent = false;
        }
    }

    void GeometryStage::setRenderTransparent(bool transparent)
    {
        m_renderTransparent = transparent;
        if (transparent)
        {
            m_renderOpaque = false;
        }
    }

    void GeometryStage::setDepthTest(bool enabled)
    {
        m_depthTest = enabled;
    }

    void GeometryStage::setDepthWrite(bool enabled)
    {
        m_depthWrite = enabled;
    }

    void GeometryStage::setUseInstancing(bool enabled)
    {
        m_useInstancing = enabled;
    }

    void GeometryStage::setSortObjects(bool enabled)
    {
        m_sortObjects = enabled;
    }

    void GeometryStage::renderGeometry(const std::shared_ptr<RenderContext> context)
    {
        PROFILE_FUNCTION();

        // Validate context
        if (!context)
        {
            Core::error("GeometryStage::renderGeometry: Invalid render context");
            return;
        }

        // Track last bound material and mesh for optimization
        std::shared_ptr<Material> lastMaterial;
        std::shared_ptr<Mesh> lastMesh;

        // Render ECS entities with forward+ shading
        if (m_registry)
        {
            renderECSEntities(context, lastMaterial, lastMesh);
        }

        // Render non-entity geometry with forward+ shading
        renderSystemGeometry(context, lastMaterial, lastMesh);

        // Unbind last material and mesh
        if (lastMaterial)
        {
            lastMaterial->unbind();
        }
        if (lastMesh)
        {
            lastMesh->unbind();
        }
    }

    void GeometryStage::renderECSEntities(const std::shared_ptr<RenderContext> context,
                                          std::shared_ptr<Material>& lastMaterial,
                                          std::shared_ptr<Mesh>& lastMesh)
    {
        PROFILE_SCOPE("GeometryStage::renderECSEntities");

        // Validate inputs
        if (!context)
        {
            Core::error("GeometryStage::renderECSEntities: Invalid render context");
            return;
        }

        if (!m_registry)
        {
            Core::warn("GeometryStage::renderECSEntities: No registry available");
            return;
        }

        auto view = m_registry->view<ECS::TransformComponent, ECS::MeshRendererComponent>();

        // Pre-collect render data for efficient sorting
        std::vector<RenderItem> renderItems;
        renderItems.reserve(view.size());

        glm::vec3 cameraPosition = context->getCameraPosition();

        // Collect all renderable entities in one pass
        for (auto [entity, transform, meshRenderer] : view)
        {
            // Validate ECS components
            if (!transform || !meshRenderer)
            {
                Core::warn("GeometryStage: Invalid components for entity " + std::to_string(entity));
                continue;
            }

            // Early rejection - skip invisible objects
            if (!meshRenderer->isVisible())
            {
                continue;
            }

            auto material = meshRenderer->getMaterial();
            auto mesh = meshRenderer->getMesh();

            // Validate resources
            if (!material || !mesh)
            {
                Core::warn("GeometryStage: Entity " + std::to_string(entity) + " has invalid material or mesh");
                continue;
            }

            // Determine transparency
            bool isOpaque = (material->getBlendMode() == BlendMode::Opaque);
            bool isTransparent = (material->getBlendMode() == BlendMode::Transparent);

            // Skip based on what we're rendering this pass
            if ((isOpaque && !m_renderOpaque) || (isTransparent && !m_renderTransparent))
            {
                continue;
            }

            // Get transform data - validate transform component
            glm::mat4 transformMatrix;
            glm::vec3 position;
            try
            {
                transformMatrix = transform->getWorldTransform().getMatrix();
                position = transform->getWorldTransform().getPosition();
            }
            catch (const std::exception& e)
            {
                Core::error("GeometryStage: Failed to get transform for entity " + std::to_string(entity) + ": " + e.what());
                continue;
            }

            // Frustum culling - skip objects outside view frustum
            try
            {
                Utility::AABB worldBounds = mesh->getBounds();
                worldBounds.transform(transformMatrix);
                if (!context->getFrustum().testAABB(worldBounds))
                {
                    continue;
                }
            }
            catch (const std::exception& e)
            {
                Core::warn("GeometryStage: Failed frustum culling for entity " + std::to_string(entity) + ": " + e.what());
                // Continue rendering even if frustum culling fails
            }

            // Calculate distance for sorting
            float distanceFromCamera = glm::length(position - cameraPosition);

            // Store render item
            renderItems.emplace_back(RenderItem{
                entity,
                transform,
                meshRenderer,
                position,
                distanceFromCamera,
                material,
                mesh
            });
        }

        // Efficient sorting for forward+ rendering
        if (m_sortObjects && !renderItems.empty())
        {
            try
            {
                sortRenderItems(renderItems);
            }
            catch (const std::exception& e)
            {
                Core::warn("GeometryStage: Failed to sort render items: " + std::string(e.what()));
                // Continue with unsorted items
            }
        }

        Core::debug("GeometryStage: Rendering " + std::to_string(renderItems.size()) + " ECS entities with forward+ shading");

        // Render sorted items with forward+ shading
        if (m_useInstancing)
        {
            renderItemsInstanced(renderItems, context);
        }
        else
        {
            for (const auto& item : renderItems)
            {
                try
                {
                    renderItem(item, context, lastMaterial, lastMesh);
                }
                catch (const std::exception& e)
                {
                    Core::error("GeometryStage: Failed to render entity " + std::to_string(item.entity) + ": " + e.what());
                    // Continue with next item
                }
            }
        }
    }

    void GeometryStage::renderSystemGeometry(const std::shared_ptr<RenderContext> context,
                                             std::shared_ptr<Material>& lastMaterial,
                                             std::shared_ptr<Mesh>& lastMesh)
    {
        PROFILE_SCOPE("GeometryStage::renderSystemGeometry");

        // Validate context
        if (!context)
        {
            Core::error("GeometryStage::renderSystemGeometry: Invalid render context");
            return;
        }

        // Get the main render system to access submitted renderables
        auto app = Core::Application::getInstance();
        if (!app)
        {
            Core::error("Cannot get Application instance in GeometryStage");
            return;
        }

        auto renderSystem = app->getSubsystem<RenderSystem>();
        if (!renderSystem)
        {
            Core::warn("GeometryStage: No RenderSystem reference - voxel geometry will not be rendered");
            return;
        }

        // Render opaque objects from the render system queue with forward+ shading
        if (m_renderOpaque)
        {
            const auto& opaqueQueue = renderSystem->getOpaqueQueue();
            Core::debug("GeometryStage: Rendering " + std::to_string(opaqueQueue.size()) + " opaque render objects with forward+ shading");

            for (const auto& renderable : opaqueQueue)
            {
                try
                {
                    renderObject(renderable, lastMaterial, lastMesh, context);
                }
                catch (const std::exception& e)
                {
                    Core::error("GeometryStage: Failed to render opaque object: " + std::string(e.what()));
                    // Continue with next object
                }
            }
        }

        // Render transparent objects from the render system queue with forward+ shading
        if (m_renderTransparent)
        {
            const auto& transparentQueue = renderSystem->getTransparentQueue();
            Core::debug("GeometryStage: Rendering " + std::to_string(transparentQueue.size()) + " transparent render objects with forward+ shading");

            for (const auto& renderable : transparentQueue)
            {
                try
                {
                    renderObject(renderable, lastMaterial, lastMesh, context);
                }
                catch (const std::exception& e)
                {
                    Core::error("GeometryStage: Failed to render transparent object: " + std::string(e.what()));
                    // Continue with next object
                }
            }
        }
    }

    void GeometryStage::renderItem(const RenderItem& item, 
                                  const std::shared_ptr<RenderContext> context,
                                  std::shared_ptr<Material>& lastMaterial,
                                  std::shared_ptr<Mesh>& lastMesh)
    {
        // Validate inputs
        if (!context)
        {
            Core::error("GeometryStage::renderItem: Invalid render context");
            return;
        }

        if (!item.material || !item.mesh || !item.transform)
        {
            Core::warn("GeometryStage::renderItem: Invalid item resources");
            return;
        }

        // Bind material if different from last
        if (item.material != lastMaterial)
        {
            if (lastMaterial)
            {
                lastMaterial->unbind();
            }
            
            item.material->bind();
            lastMaterial = item.material;

            // Validate shader before use
            auto shader = item.material->getShader();
            if (!shader)
            {
                Core::error("GeometryStage::renderItem: Material has no shader");
                return;
            }

            // Update common uniforms
            context->bindShaderUniforms(shader);
            
            // Bind forward+ cluster data if available
            if (m_forwardPlusShaderManager && m_forwardPlusShaderManager->isForwardPlusReady())
            {
                try
                {
                    m_forwardPlusShaderManager->bindClusterData(shader, context);
                    m_forwardPlusShaderManager->updateClusterUniforms(shader, context);
                }
                catch (const std::exception& e)
                {
                    Core::warn("GeometryStage: Failed to bind forward+ cluster data: " + std::string(e.what()));
                    // Continue rendering without forward+ features
                }
            }
        }

        // Set model matrix - validate shader availability
        if (lastMaterial && lastMaterial->getShader())
        {
            try
            {
                lastMaterial->getShader()->setMat4("modelMatrix", item.transform->getWorldTransform().getMatrix());
            }
            catch (const std::exception& e)
            {
                Core::error("GeometryStage: Failed to set model matrix: " + std::string(e.what()));
                return;
            }
        }
        else
        {
            Core::error("GeometryStage::renderItem: No valid shader available");
            return;
        }

        // Bind mesh if different from last
        if (item.mesh != lastMesh)
        {
            if (lastMesh)
            {
                lastMesh->unbind();
            }
            item.mesh->bind();
            lastMesh = item.mesh;
        }

        // Draw mesh with forward+ shading and track statistics
        try
        {
            item.mesh->draw();
            trackDrawCall(item.mesh);
        }
        catch (const std::exception& e)
        {
            Core::error("GeometryStage: Failed to draw mesh: " + std::string(e.what()));
        }
    }

    void GeometryStage::renderItemsInstanced(const std::vector<RenderItem>& items, 
                                            const std::shared_ptr<RenderContext> context)
    {
        PROFILE_SCOPE("GeometryStage::renderItemsInstanced");

        if (items.empty() || !context)
        {
            return;
        }

        // Group items by mesh and material for instancing
        std::unordered_map<std::pair<std::shared_ptr<Mesh>, std::shared_ptr<Material>>,
                           std::vector<glm::mat4>,
                           PairHash> instanceGroups;

        for (const auto& item : items)
        {
            // Validate item
            if (!item.mesh || !item.material || !item.transform)
            {
                Core::warn("GeometryStage: Skipping invalid item in instanced rendering");
                continue;
            }

            try
            {
                auto key = std::make_pair(item.mesh, item.material);
                instanceGroups[key].push_back(item.transform->getWorldTransform().getMatrix());
            }
            catch (const std::exception& e)
            {
                Core::warn("GeometryStage: Failed to add item to instance group: " + std::string(e.what()));
            }
        }

        // Render each instance group with forward+ shading
        for (const auto& [meshMaterialPair, transforms] : instanceGroups)
        {
            auto [mesh, material] = meshMaterialPair;

            // Validate resources
            if (!mesh || !material)
            {
                Core::warn("GeometryStage: Invalid mesh or material in instance group");
                continue;
            }

            if (transforms.size() == 1)
            {
                // Single instance - use regular rendering
                RenderItem singleItem = items[0];
                for (const auto& item : items)
                {
                    if (item.mesh == mesh && item.material == material)
                    {
                        singleItem = item;
                        break;
                    }
                }

                std::shared_ptr<Material> lastMaterial;
                std::shared_ptr<Mesh> lastMesh;
                renderItem(singleItem, context, lastMaterial, lastMesh);
            }
            else
            {
                // Multiple instances - use instanced rendering with forward+ shading
                try
                {
                    material->bind();
                    
                    auto shader = material->getShader();
                    if (!shader)
                    {
                        Core::error("GeometryStage: Material has no shader for instanced rendering");
                        continue;
                    }

                    context->bindShaderUniforms(shader);
                    
                    // Bind forward+ cluster data if available
                    if (m_forwardPlusShaderManager && m_forwardPlusShaderManager->isForwardPlusReady())
                    {
                        try
                        {
                            m_forwardPlusShaderManager->bindClusterData(shader, context);
                            m_forwardPlusShaderManager->updateClusterUniforms(shader, context);
                        }
                        catch (const std::exception& e)
                        {
                            Core::warn("GeometryStage: Failed to bind forward+ data for instanced rendering: " + std::string(e.what()));
                        }
                    }

                    // TODO: Upload transform matrices to instance buffer
                    // For now, render individually (instancing requires additional buffer setup)
                    for (const glm::mat4& transform : transforms)
                    {
                        try
                        {
                            shader->setMat4("modelMatrix", transform);
                            mesh->bind();
                            mesh->draw();
                            trackDrawCall(mesh);
                            mesh->unbind();
                        }
                        catch (const std::exception& e)
                        {
                            Core::error("GeometryStage: Failed to render instance: " + std::string(e.what()));
                        }
                    }

                    material->unbind();
                }
                catch (const std::exception& e)
                {
                    Core::error("GeometryStage: Failed instanced rendering: " + std::string(e.what()));
                }
            }
        }
    }

    void GeometryStage::renderObject(const RenderableObject& renderable,
                                     std::shared_ptr<Material>& lastMaterial,
                                     std::shared_ptr<Mesh>& lastMesh,
                                     const std::shared_ptr<RenderContext> context)
    {
        // Validate inputs
        if (!context)
        {
            Core::error("GeometryStage::renderObject: Invalid render context");
            return;
        }

        if (!renderable.mesh || !renderable.material)
        {
            Core::warn("GeometryStage::renderObject: Invalid renderable resources");
            return;
        }

        // Bind material if different from last
        if (renderable.material != lastMaterial)
        {
            if (lastMaterial)
            {
                lastMaterial->unbind();
            }
            
            renderable.material->bind();
            lastMaterial = renderable.material;
            
            auto shader = renderable.material->getShader();
            if (!shader)
            {
                Core::error("GeometryStage::renderObject: Material has no shader");
                return;
            }

            try
            {
                // Update common uniforms
                context->bindShaderUniforms(shader);

                // Update material uniforms
                shader->setFloat("metallic", renderable.material->getMetallic());
                shader->setFloat("roughness", renderable.material->getRoughness());
                
                // Bind forward+ cluster data if available
                if (m_forwardPlusShaderManager && m_forwardPlusShaderManager->isForwardPlusReady())
                {
                    m_forwardPlusShaderManager->bindClusterData(shader, context);
                    m_forwardPlusShaderManager->updateClusterUniforms(shader, context);
                }
            }
            catch (const std::exception& e)
            {
                Core::error("GeometryStage: Failed to setup material uniforms: " + std::string(e.what()));
                return;
            }
        }

        // Set model matrix from renderable transform
        if (lastMaterial && lastMaterial->getShader())
        {
            try
            {
                lastMaterial->getShader()->setMat4("modelMatrix", renderable.transform);
            }
            catch (const std::exception& e)
            {
                Core::error("GeometryStage: Failed to set model matrix for renderable: " + std::string(e.what()));
                return;
            }
        }
        else
        {
            Core::error("GeometryStage::renderObject: No valid shader available");
            return;
        }

        // Bind mesh if different from last
        if (renderable.mesh != lastMesh)
        {
            if (lastMesh)
            {
                lastMesh->unbind();
            }
            renderable.mesh->bind();
            lastMesh = renderable.mesh;
        }

        // Before draw call - debug logging
        Core::debug("GeometryStage: About to draw mesh - VAO bound: " +
                    std::to_string(renderable.mesh && renderable.mesh->isLoaded()) + ", Shader bound: " +
                    std::to_string(renderable.material && renderable.material->getShader() && renderable.material->getShader()->isValid()));

        // Draw mesh with forward+ shading and track statistics
        try
        {
            renderable.mesh->draw();
            trackDrawCall(renderable.mesh);
        }
        catch (const std::exception& e)
        {
            Core::error("GeometryStage: Failed to draw renderable mesh: " + std::string(e.what()));
        }

        // After draw call - check for GL errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
        {
            Core::error("OpenGL error after draw: " + std::to_string(error));
        }
    }

    void GeometryStage::setupRenderState()
    {
        try
        {
            // First, ensure we have a valid OpenGL context and framebuffer state
            GLenum error = glGetError(); // Clear any existing errors
            
            // Check if we have a valid current context
            GLint currentFramebuffer = 0;
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFramebuffer);
            error = glGetError();
            if (error != GL_NO_ERROR)
            {
                Core::error("GeometryStage: Cannot query framebuffer binding - OpenGL context may be invalid (error: " + std::to_string(error) + ")");
                return;
            }

            // Check framebuffer completeness if we're not using the default framebuffer
            if (currentFramebuffer != 0)
            {
                GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
                {
                    Core::error("GeometryStage: Cannot setup render state - framebuffer is not complete (status: " + std::to_string(framebufferStatus) + ")");
                    return;
                }
            }
            
            // Check for depth buffer availability - should be present with shared depth texture
            bool hasDepthBuffer = false;
            if (getOutput())
            {
                auto depthTexture = getOutput()->getDepthTexture();
                hasDepthBuffer = (depthTexture != nullptr);
                Core::debug("GeometryStage: Output render target depth texture check - " +
                           std::string(hasDepthBuffer ? "FOUND" : "NOT FOUND") +
                           (depthTexture ? " (ID: " + std::to_string(depthTexture->getID()) + ")" : ""));
            }
            else if (currentFramebuffer == 0)
            {
                // Default framebuffer - for completeness, but forward+ uses custom targets
                GLint depthBits = 0;
                glGetIntegerv(GL_DEPTH_BITS, &depthBits);
                hasDepthBuffer = (depthBits > 0);
                Core::debug("GeometryStage: Default framebuffer depth bits: " + std::to_string(depthBits));
            }
            
            // Get current depth test state before modifying it
            GLboolean currentDepthTest = glIsEnabled(GL_DEPTH_TEST);
            
            // Depth test - only modify state if needed and we have valid context
            bool shouldEnableDepthTest = (m_depthTest && hasDepthBuffer);
            
            // CRITICAL: Only modify depth test state if we have a depth buffer
            // This prevents GL_INVALID_OPERATION errors when no depth buffer exists
            if (hasDepthBuffer)
            {
                if (shouldEnableDepthTest && !currentDepthTest)
                {
                    // Safe to enable - we have depth buffer and it's currently disabled
                    glEnable(GL_DEPTH_TEST);
                    glDepthFunc(GL_LEQUAL);
                }
                else if (!shouldEnableDepthTest && currentDepthTest)
                {
                    // Safe to disable - we have depth buffer and it's currently enabled
                    glDisable(GL_DEPTH_TEST);
                }
            }
            
            // Log warning about missing depth buffer but don't try to change OpenGL state
            if (m_depthTest && !hasDepthBuffer)
            {
                Core::error("GeometryStage: Depth test requested but no depth buffer available in render target - leaving depth test state unchanged");
            }

            // Depth write - only set if depth test is enabled or will be enabled
            if (shouldEnableDepthTest || currentDepthTest)
            {
                glDepthMask(m_depthWrite ? GL_TRUE : GL_FALSE);
            }

            // Face culling
            if (!glIsEnabled(GL_CULL_FACE))
            {
                glEnable(GL_CULL_FACE);
            }
            glCullFace(GL_BACK);

            // Transparency settings
            GLboolean currentBlend = glIsEnabled(GL_BLEND);
            if (m_renderTransparent && !currentBlend)
            {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
            else if (!m_renderTransparent && currentBlend)
            {
                glDisable(GL_BLEND);
            }
        }
        catch (const std::exception& e)
        {
            Core::error("GeometryStage: Failed to setup render state: " + std::string(e.what()));
        }
    }

    void GeometryStage::restoreRenderState()
    {
        try
        {
            // Clear any existing errors first
            GLenum error = glGetError();
            
            // Check if we have a valid OpenGL context
            GLint currentFramebuffer = 0;
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFramebuffer);
            error = glGetError();
            if (error != GL_NO_ERROR)
            {
                Core::error("GeometryStage: Cannot restore render state - OpenGL context invalid (error: " + std::to_string(error) + ")");
                return;
            }
            
            // Check framebuffer completeness if we're not using the default framebuffer
            if (currentFramebuffer != 0)
            {
                GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
                {
                    Core::error("GeometryStage: Cannot restore render state - framebuffer is not complete (status: " + std::to_string(framebufferStatus) + ")");
                    return;
                }
            }

            // Check depth buffer availability more safely
            bool hasDepthBuffer = false;
            if (currentFramebuffer != 0)
            {
                // Custom framebuffer - check for depth attachment
                GLint hasDepthAttachment = 0;
                glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 
                                                     GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &hasDepthAttachment);
                error = glGetError();
                if (error == GL_NO_ERROR)
                {
                    hasDepthBuffer = (hasDepthAttachment != GL_NONE);
                }
                else
                {
                    Core::warn("GeometryStage: Could not query depth attachment, assuming no depth buffer");
                    hasDepthBuffer = false;
                }
            }
            else
            {
                // Default framebuffer - check depth bits
                GLint depthBits = 0;
                glGetIntegerv(GL_DEPTH_BITS, &depthBits);
                error = glGetError();
                hasDepthBuffer = (error == GL_NO_ERROR && depthBits > 0);
            }

            // Get current OpenGL state before modifying
            GLboolean currentDepthTest = glIsEnabled(GL_DEPTH_TEST);
            GLboolean currentBlend = glIsEnabled(GL_BLEND);
            
            // CRITICAL: Only modify depth test state if we have a depth buffer
            // This prevents GL_INVALID_OPERATION errors when no depth buffer exists
            if (hasDepthBuffer)
            {
                if (!currentDepthTest)
                {
                    // Safe to enable - we have depth buffer and it's currently disabled
                    glEnable(GL_DEPTH_TEST);
                    glDepthMask(GL_TRUE);
                }
                else
                {
                    // We have a depth buffer and depth test is enabled - just ensure depth mask is correct
                    glDepthMask(GL_TRUE);
                }
            }
            else if (!hasDepthBuffer && currentDepthTest)
            {
                // Dangerous situation: depth test is enabled but no depth buffer
                // Rather than trying to disable (which causes GL_INVALID_OPERATION),
                // we'll leave the state alone and log a warning
                Core::debug("GeometryStage: No depth attachment found, but depth test is enabled - leaving state unchanged to avoid OpenGL errors");
            }
            
            // Only disable blend if it's currently enabled
            if (currentBlend)
            {
                glDisable(GL_BLEND);
            }
        }
        catch (const std::exception& e)
        {
            Core::error("GeometryStage: Failed to restore render state: " + std::string(e.what()));
        }
    }

    void GeometryStage::sortRenderItems(std::vector<RenderItem>& items)
    {
        PROFILE_FUNCTION();

        if (items.empty())
        {
            return;
        }

        try
        {
            if (m_renderTransparent)
            {
                // Sort transparent objects back-to-front for proper blending
                std::sort(items.begin(), items.end(),
                    [](const RenderItem& a, const RenderItem& b) {
                        return a.distanceFromCamera > b.distanceFromCamera;
                    });
            }
            else if (m_renderOpaque)
            {
                // Sort opaque objects by material first (reduce state changes), then front-to-back for early Z rejection
                std::sort(items.begin(), items.end(),
                    [](const RenderItem& a, const RenderItem& b) {
                        // First sort by material to reduce state changes
                        if (a.material != b.material)
                        {
                            return a.material < b.material;
                        }
                        // Then sort front-to-back for early Z rejection
                        return a.distanceFromCamera < b.distanceFromCamera;
                    });
            }
        }
        catch (const std::exception& e)
        {
            Core::error("GeometryStage: Failed to sort render items: " + std::string(e.what()));
            // Continue with unsorted items
        }
    }

    void GeometryStage::trackDrawCall(const std::shared_ptr<Mesh>& mesh, uint32_t instanceCount)
    {
        if (!mesh)
        {
            return;
        }

        try
        {
            // Increment draw call count
            m_frameDrawCalls++;

            // Calculate triangle count based on mesh data and primitive type
            uint32_t triangles = 0;

            if (mesh->getPrimitiveType() == PrimitiveType::Triangles)
            {
                if (mesh->getIndexCount() > 0)
                {
                    // Indexed geometry
                    triangles = mesh->getIndexCount() / 3;
                }
                else
                {
                    // Non-indexed geometry
                    triangles = mesh->getVertexCount() / 3;
                }
            }
            else if (mesh->getPrimitiveType() == PrimitiveType::TriangleStrip)
            {
                if (mesh->getIndexCount() > 0)
                {
                    triangles = mesh->getIndexCount() > 2 ? mesh->getIndexCount() - 2 : 0;
                }
                else
                {
                    triangles = mesh->getVertexCount() > 2 ? mesh->getVertexCount() - 2 : 0;
                }
            }
            else if (mesh->getPrimitiveType() == PrimitiveType::TriangleFan)
            {
                if (mesh->getIndexCount() > 0)
                {
                    triangles = mesh->getIndexCount() > 2 ? mesh->getIndexCount() - 2 : 0;
                }
                else
                {
                    triangles = mesh->getVertexCount() > 2 ? mesh->getVertexCount() - 2 : 0;
                }
            }
            // Other primitive types (points, lines) don't contribute to triangle count

            // Multiply by instance count for instanced rendering
            m_frameTriangles += triangles * instanceCount;
        }
        catch (const std::exception& e)
        {
            Core::warn("GeometryStage: Failed to track draw call statistics: " + std::string(e.what()));
            // Just increment draw calls if triangle calculation fails
            m_frameDrawCalls++;
        }
    }

} // namespace PixelCraft::Rendering::Stages
