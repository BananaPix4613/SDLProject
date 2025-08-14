// -------------------------------------------------------------------------
// TransparencyStage.cpp
// -------------------------------------------------------------------------
#include "Rendering/Stages/TransparencyStage.h"
#include "Rendering/Shader.h"
#include "Rendering/RenderContext.h"
#include "Rendering/RenderTarget.h"
#include "Rendering/Material.h"
#include "Rendering/Mesh.h"
#include "Rendering/ForwardPlusShaderManager.h"
#include "Rendering/ClusteredLightCuller.h"
#include "Core/Logger.h"
#include "Utility/Profiler.h"
#include "ECS/Registry.h"
#include "ECS/View.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshRendererComponent.h"

#include <algorithm>

namespace PixelCraft::Rendering::Stages
{

    TransparencyStage::TransparencyStage()
        : RenderStage("TransparencyStage")
    {
    }

    TransparencyStage::~TransparencyStage()
    {
        if (isInitialized())
        {
            shutdown();
        }
    }

    bool TransparencyStage::initialize()
    {
        if (isInitialized())
        {
            return true;
        }

        Core::info("Initializing TransparencyStage with ForwardPlusShaderManager integration");

        // Verify ForwardPlusShaderManager is available
        if (!m_forwardPlusShaderManager)
        {
            Core::warn("TransparencyStage: No ForwardPlusShaderManager set during initialization");
        }

        // Create OIT resources if needed
        if (m_technique != TransparencyTechnique::AlphaBlending)
        {
            createOITResources();
        }

        setInitialized(true);
        Core::info("TransparencyStage initialized successfully with forward+ integration");
        return true;
    }

    void TransparencyStage::shutdown()
    {
        if (!isInitialized())
        {
            return;
        }

        Core::info("Shutting down TransparencyStage");

        // Clean up resources
        m_transparencyShader.reset();
        m_compositeShader.reset();
        m_registry.reset();
        m_forwardPlusShaderManager.reset();
        m_depthBuffer.reset();
        m_accumulationTarget.reset();
        m_revealageTarget.reset();
        m_depthLayers.clear();

        setInitialized(false);
    }

    void TransparencyStage::execute(const std::shared_ptr<RenderContext> context)
    {
        if (!isInitialized() || !isEnabled())
        {
            Core::warn("TransparencyStage: Not initialized or enabled");
            return;
        }

        PROFILE_SCOPE("TransparencyStage::execute");

        if (!m_transparencyShader || !m_registry)
        {
            Core::warn("TransparencyStage: Missing transparency shader or registry");
            return;
        }

        // Check if forward+ resources are available
        if (!m_forwardPlusShaderManager || !m_forwardPlusShaderManager->isForwardPlusReady())
        {
            Core::warn("TransparencyStage: Forward+ resources not available, skipping transparency pass");
            return;
        }

        // Collect transparent objects
        auto transparentItems = collectTransparentItems(context);
        if (transparentItems.empty())
        {
            return;
        }

        // Sort objects based on technique
        if (m_technique == TransparencyTechnique::AlphaBlending)
        {
            sortTransparentItems(transparentItems);
        }

        // Render based on technique
        switch (m_technique)
        {
            case TransparencyTechnique::AlphaBlending:
                renderAlphaBlended(context, transparentItems);
                break;
            case TransparencyTechnique::WeightedBlended:
                renderWeightedBlended(context, transparentItems);
                break;
            case TransparencyTechnique::DepthPeeling:
            case TransparencyTechnique::DualDepthPeeling:
                renderDepthPeeling(context);
                break;
        }
    }

    void TransparencyStage::setForwardPlusShaderManager(std::shared_ptr<ForwardPlusShaderManager> shaderManager)
    {
        m_forwardPlusShaderManager = shaderManager;
    }

    std::shared_ptr<ForwardPlusShaderManager> TransparencyStage::getForwardPlusShaderManager() const
    {
        return m_forwardPlusShaderManager;
    }

    void TransparencyStage::setTransparencyTechnique(TransparencyTechnique technique)
    {
        if (m_technique != technique)
        {
            m_technique = technique;
            if (isInitialized() && technique != TransparencyTechnique::AlphaBlending)
            {
                createOITResources();
            }
        }
    }

    TransparencyStage::TransparencyTechnique TransparencyStage::getTransparencyTechnique() const
    {
        return m_technique;
    }

    void TransparencyStage::setTransparencyShader(std::shared_ptr<Shader> shader)
    {
        m_transparencyShader = shader;
    }

    std::shared_ptr<Shader> TransparencyStage::getTransparencyShader() const
    {
        return m_transparencyShader;
    }

    void TransparencyStage::setRegistry(std::shared_ptr<ECS::Registry> registry)
    {
        m_registry = registry;
    }

    void TransparencyStage::setDepthBuffer(std::shared_ptr<RenderTarget> depthBuffer)
    {
        m_depthBuffer = depthBuffer;
    }

    std::shared_ptr<RenderTarget> TransparencyStage::getDepthBuffer() const
    {
        return m_depthBuffer;
    }
    
    void TransparencyStage::setMaxDepthLayers(int layers)
    {
        m_maxDepthLayers = std::max(1, layers);
        if (isInitialized() && (m_technique == TransparencyTechnique::DepthPeeling ||
                                m_technique == TransparencyTechnique::DualDepthPeeling))
        {
            createOITResources();
        }
    }

    int TransparencyStage::getMaxDepthLayers() const
    {
        return m_maxDepthLayers;
    }

    void TransparencyStage::setAlphaToCoverageEnabled(bool enabled)
    {
        m_alphaToCoverageEnabled = enabled;
    }

    bool TransparencyStage::isAlphaToCoverageEnabled() const
    {
        return m_alphaToCoverageEnabled;
    }

    void TransparencyStage::setAlphaDiscardThreshold(float threshold)
    {
        m_alphaDiscardThreshold = threshold;
    }

    float TransparencyStage::getAlphaDiscardThreshold() const
    {
        return m_alphaDiscardThreshold;
    }

    void TransparencyStage::setSoftParticlesEnabled(bool enabled)
    {
        m_softParticlesEnabled = enabled;
    }

    bool TransparencyStage::isSoftParticlesEnabled() const
    {
        return m_softParticlesEnabled;
    }

    void TransparencyStage::renderAlphaBlended(const std::shared_ptr<RenderContext> context,
                                               const std::vector<TransparentRenderItem>& items)
    {
        PROFILE_SCOPE("TransparencyStage::renderAlphaBlended");

        // Bind output
        bindOutput();

        // Setup render state for forward+ transparency
        setupTransparencyRenderState(BlendMode::Transparent);

        // Bind transparency shader and set up forward+ uniforms
        m_transparencyShader->bind();
        context->bindShaderUniforms(m_transparencyShader);
        
        // Bind forward+ cluster data using ForwardPlusShaderManager
        if (m_forwardPlusShaderManager)
        {
            m_forwardPlusShaderManager->bindClusterData(m_transparencyShader, context);
            m_forwardPlusShaderManager->updateClusterUniforms(m_transparencyShader, context);
        }
        
        setupDepthTesting(m_transparencyShader);

        // Render each transparent object
        std::shared_ptr<Rendering::Material> lastMaterial;
        std::shared_ptr<Rendering::Mesh> lastMesh;

        for (const auto& item : items)
        {
            // Bind material if different
            if (item.material != lastMaterial)
            {
                if (lastMaterial)
                {
                    lastMaterial->unbind();
                }
                item.material->bind();
                lastMaterial = item.material;

                // Set transparency-specific uniforms
                m_transparencyShader->setFloat("alphaDiscardThreshold", m_alphaDiscardThreshold);
                
                // Bind depth buffer for soft particles
                if (m_softParticlesEnabled && m_depthBuffer)
                {
                    m_transparencyShader->setInt("depthTexture", 10);
                    m_depthBuffer->getDepthTexture()->bind(10);
                }

                // Set cluster index for this object's position
                int clusterIndex = calculateClusterIndex(item.position, context);
                m_transparencyShader->setInt("objectClusterIndex", clusterIndex);
            }

            // Set model matrix and normal matrix
            glm::mat4 modelMatrix = item.transform->getWorldTransform().getMatrix();
            m_transparencyShader->setMat4("modelMatrix", modelMatrix);
            
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
            m_transparencyShader->setMat3("normalMatrix", normalMatrix);

            // Bind mesh if different
            if (item.mesh != lastMesh)
            {
                if (lastMesh)
                {
                    lastMesh->unbind();
                }
                item.mesh->bind();
                lastMesh = item.mesh;
            }

            // Draw mesh
            item.mesh->draw();
        }

        // Cleanup
        if (lastMaterial)
        {
            lastMaterial->unbind();
        }
        if (lastMesh)
        {
            lastMesh->unbind();
        }

        m_transparencyShader->unbind();
        restoreRenderState();

        // Unbind output render target
        unbindOutput();
    }

    void TransparencyStage::renderWeightedBlended(const std::shared_ptr<RenderContext> context,
                                                  const std::vector<TransparentRenderItem>& items)
    {
        PROFILE_SCOPE("TransparencyStage::renderWeightedBlended");

        if (!m_compositeShader || !m_accumulationTarget || !m_revealageTarget)
        {
            Core::error("TransparencyStage: OIT resources not initialized");
            return;
        }

        // Phase 1: Render to accumulation and revealage buffers
        m_accumulationTarget->bind();
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        m_revealageTarget->bind();
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Bind both targets for MRT
        GLuint attachments[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, attachments);

        // Setup OIT blend state
        glEnable(GL_BLEND);
        glBlendFunci(0, GL_ONE, GL_ONE); // Accumulation
        glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR); // Revealage
        glDepthMask(GL_FALSE);

        // Enable depth testing but don't write
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        // Bind cluster data and render transparent objects
        m_transparencyShader->bind();
        context->bindShaderUniforms(m_transparencyShader);
        
        // Bind forward+ cluster data using ForwardPlusShaderManager
        if (m_forwardPlusShaderManager)
        {
            m_forwardPlusShaderManager->bindClusterData(m_transparencyShader, context);
            m_forwardPlusShaderManager->updateClusterUniforms(m_transparencyShader, context);
        }
        
        setupDepthTesting(m_transparencyShader);

        for (const auto& item : items)
        {
            // Bind material textures
            item.material->bind();

            // Set model matrix and cluster index
            glm::mat4 modelMatrix = item.transform->getWorldTransform().getMatrix();
            m_transparencyShader->setMat4("modelMatrix", modelMatrix);
            
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
            m_transparencyShader->setMat3("normalMatrix", normalMatrix);

            int clusterIndex = calculateClusterIndex(item.position, context);
            m_transparencyShader->setInt("objectClusterIndex", clusterIndex);

            // Draw
            item.mesh->bind();
            item.mesh->draw();
            item.mesh->unbind();

            item.material->unbind();
        }

        m_transparencyShader->unbind();

        // Phase 2: Composite with forward+ aware blending
        bindOutput();
        setupTransparencyRenderState(BlendMode::Transparent);

        m_compositeShader->bind();
        m_compositeShader->setInt("accumulationTexture", 0);
        m_compositeShader->setInt("revealageTexture", 1);

        // Bind depth buffer for proper depth testing
        if (m_depthBuffer)
        {
            m_compositeShader->setInt("sceneDepthTexture", 2);
            m_depthBuffer->getDepthTexture()->bind(2);
        }

        m_accumulationTarget->getColorTexture(0)->bind(0);
        m_revealageTarget->getColorTexture(0)->bind(1);

        renderFullscreenQuad();

        m_compositeShader->unbind();
        restoreRenderState();

        // Unbind output render target
        unbindOutput();
    }

    void TransparencyStage::renderDepthPeeling(const std::shared_ptr<RenderContext> context)
    {
        PROFILE_SCOPE("TransparencyStage::renderDepthPeeling");

        if (m_depthLayers.empty())
        {
            Core::error("TransparencyStage: Depth peeling resources not initialized");
            // Fallback to alpha blending
            auto transparentItems = collectTransparentItems(context);
            sortTransparentItems(transparentItems);
            renderAlphaBlended(context, transparentItems);
            return;
        }

        auto transparentItems = collectTransparentItems(context);
        if (transparentItems.empty())
        {
            return;
        }

        // Depth peeling implementation with forward+ lighting
        for (int layer = 0; layer < m_maxDepthLayers; ++layer)
        {
            // Bind current depth layer
            m_depthLayers[layer]->bind();
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Setup depth testing for this layer
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_GREATER); // Render next depth layer
            glDepthMask(GL_TRUE);

            // Bind shader and forward+ data
            m_transparencyShader->bind();
            context->bindShaderUniforms(m_transparencyShader);
            
            // Bind forward+ cluster data using ForwardPlusShaderManager
            if (m_forwardPlusShaderManager)
            {
                m_forwardPlusShaderManager->bindClusterData(m_transparencyShader, context);
                m_forwardPlusShaderManager->updateClusterUniforms(m_transparencyShader, context);
            }

            // Set layer-specific uniforms
            m_transparencyShader->setInt("currentLayer", layer);
            if (layer > 0)
            {
                m_transparencyShader->setInt("previousDepthTexture", 10);
                m_depthLayers[layer - 1]->getDepthTexture()->bind(10);
            }

            // Render all transparent objects for this layer
            for (const auto& item : transparentItems)
            {
                item.material->bind();

                glm::mat4 modelMatrix = item.transform->getWorldTransform().getMatrix();
                m_transparencyShader->setMat4("modelMatrix", modelMatrix);
                
                glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
                m_transparencyShader->setMat3("normalMatrix", normalMatrix);

                int clusterIndex = calculateClusterIndex(item.position, context);
                m_transparencyShader->setInt("objectClusterIndex", clusterIndex);

                item.mesh->bind();
                item.mesh->draw();
                item.mesh->unbind();

                item.material->unbind();
            }

            m_transparencyShader->unbind();

            // Unbind each depth layer properly
            m_depthLayers[layer]->unbind();
        }

        // Composite all layers
        bindOutput();
        setupTransparencyRenderState(BlendMode::Transparent);

        if (m_compositeShader)
        {
            m_compositeShader->bind();
            m_compositeShader->setInt("numLayers", m_maxDepthLayers);

            for (int i = 0; i < m_maxDepthLayers; ++i)
            {
                std::string textureName = "layerTexture" + std::to_string(i);
                m_compositeShader->setInt(textureName, i);
                m_depthLayers[i]->getColorTexture(0)->bind(i);
            }

            renderFullscreenQuad();
            m_compositeShader->unbind();
        }

        restoreRenderState();

        // Unbind output render target
        unbindOutput();
    }

    std::vector<TransparentRenderItem> TransparencyStage::collectTransparentItems(const std::shared_ptr<RenderContext> context)
    {
        PROFILE_SCOPE("TransparencyStage::collectTransparentItems");

        std::vector<TransparentRenderItem> transparentItems;
        auto view = m_registry->view<ECS::TransformComponent, ECS::MeshRendererComponent>();

        glm::vec3 cameraPosition = context->getCameraPosition();

        // Single pass collection with forward+ optimization
        for (auto [entity, transform, meshRenderer] : view)
        {
            // Early rejection
            if (!meshRenderer->isVisible() || !meshRenderer->getMaterial())
            {
                continue;
            }

            // Check if transparent
            if (meshRenderer->getMaterial()->getBlendMode() == BlendMode::Opaque)
            {
                continue;
            }

            auto mesh = meshRenderer->getMesh();
            if (!mesh)
            {
                continue;
            }

            // Get transform data
            glm::mat4 transformMatrix = transform->getWorldTransform().getMatrix();
            glm::vec3 position = transform->getWorldTransform().getPosition();

            // Frustum culling - skip objects outside view frustum
            Utility::AABB worldBounds = mesh->getBounds();
            worldBounds.transform(transformMatrix);
            if (!context->getFrustum().testAABB(worldBounds))
            {
                continue; // Skip objects outside frustum
            }

            // Calculate distance for sorting
            float distanceFromCamera = glm::length(position - cameraPosition);

            // Store transparent item with all needed data
            transparentItems.emplace_back(TransparentRenderItem{
                entity,
                transform,     // Store pointer from view (safe during this frame)
                meshRenderer,  // Store pointer from view (safe during this frame)
                position,
                distanceFromCamera,
                meshRenderer->getMaterial(),
                mesh
            });
        }

        Core::debug("TransparencyStage: Collected " + std::to_string(transparentItems.size()) + " transparent objects for forward+ rendering");
        return transparentItems;
    }

    void TransparencyStage::sortTransparentItems(std::vector<TransparentRenderItem>& items)
    {
        PROFILE_SCOPE("TransparencyStage::sortTransparentItems");

        if (items.empty())
        {
            return;
        }

        // Sort back-to-front for proper alpha blending
        std::sort(items.begin(), items.end(),
                  [](const TransparentRenderItem& a, const TransparentRenderItem& b) {
                      return a.distanceFromCamera > b.distanceFromCamera;
                  });
    }

    void TransparencyStage::createOITResources()
    {
        if (!getOutput())
        {
            Core::error("TransparencyStage: No output target set");
            return;
        }

        int width = getOutput()->getWidth();
        int height = getOutput()->getHeight();

        switch (m_technique)
        {
            case TransparencyTechnique::WeightedBlended:
                // Create accumulation buffer (RGBA16F)
                m_accumulationTarget = std::make_shared<RenderTarget>(width, height, false);
                m_accumulationTarget->initialize();
                m_accumulationTarget->addColorAttachment(TextureFormat::RGBA16F, 0);

                // Create revealage buffer (R8)
                m_revealageTarget = std::make_shared<RenderTarget>(width, height, false);
                m_revealageTarget->initialize();
                m_revealageTarget->addColorAttachment(TextureFormat::R8, 0);
                break;

            case TransparencyTechnique::DepthPeeling:
            case TransparencyTechnique::DualDepthPeeling:
                // Create depth layers
                m_depthLayers.clear();
                m_depthLayers.reserve(m_maxDepthLayers);

                for (int i = 0; i < m_maxDepthLayers; ++i)
                {
                    auto depthLayer = std::make_shared<RenderTarget>(width, height, false);
                    depthLayer->initialize();
                    depthLayer->addColorAttachment(TextureFormat::RGBA8, 0);
                    depthLayer->setDepthAttachment(TextureFormat::Depth24);
                    m_depthLayers.push_back(depthLayer);
                }
                break;

            default:
                break;
        }

        Core::info("TransparencyStage: Created OIT resources for technique " + std::to_string(static_cast<int>(m_technique)));
    }

    void TransparencyStage::setupTransparencyRenderState(BlendMode blendMode)
    {
        // Enable depth testing but don't write to depth buffer
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL); // Use LEQUAL to properly test against Z-prepass
        glDepthMask(GL_FALSE); // Don't write to depth buffer

        glEnable(GL_BLEND);
        switch (blendMode)
        {
            case BlendMode::Transparent:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                break;
            case BlendMode::Additive:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                break;
            case BlendMode::Multiply:
                glBlendFunc(GL_DST_COLOR, GL_ZERO);
                break;
            case BlendMode::Screen:
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
                break;
            default:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                break;
        }

        if (m_alphaToCoverageEnabled)
        {
            glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        }

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }

    void TransparencyStage::restoreRenderState()
    {
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        if (m_alphaToCoverageEnabled)
        {
            glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        }
    }

    void TransparencyStage::setupDepthTesting(std::shared_ptr<Shader> shader)
    {
        if (!shader || !m_depthBuffer)
        {
            return;
        }

        // Bind Z-prepass depth buffer for depth testing
        shader->setInt("sceneDepthTexture", 15);
        m_depthBuffer->getDepthTexture()->bind(15);

        // Set depth testing parameters
        shader->setBool("enableDepthTesting", true);
        shader->setFloat("depthBias", 0.0001f); // Small bias to avoid z-fighting
    }

    int TransparencyStage::calculateClusterIndex(const glm::vec3& worldPos, const std::shared_ptr<RenderContext> context) const
    {
        // Use ForwardPlusShaderManager's ClusteredLightCuller if available
        if (m_forwardPlusShaderManager && m_forwardPlusShaderManager->getClusteredLightCuller())
        {
            auto culler = m_forwardPlusShaderManager->getClusteredLightCuller();
            const auto& clusterData = culler->getClusterData();
            
            // Transform world position to view space
            glm::vec3 viewPos = context->worldToViewSpace(worldPos);
            
            // Calculate cluster indices
            glm::vec3 clusterPos = (viewPos - clusterData.minBounds) / clusterData.clusterSize;
            
            int x = static_cast<int>(clusterPos.x);
            int y = static_cast<int>(clusterPos.y);
            int z = static_cast<int>(clusterPos.z);
            
            // Check bounds
            if (x < 0 || x >= clusterData.dimensions.x ||
                y < 0 || y >= clusterData.dimensions.y ||
                z < 0 || z >= clusterData.dimensions.z)
            {
                return -1;
            }
            
            // Calculate flat index
            return z * (clusterData.dimensions.x * clusterData.dimensions.y) + 
                   y * clusterData.dimensions.x + x;
        }
        
        return -1; // No clustering available
    }

    void TransparencyStage::renderFullscreenQuad()
    {
        // Save current OpenGL state
        GLboolean depthTestEnabled;
        GLboolean cullFaceEnabled;
        glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
        glGetBooleanv(GL_CULL_FACE, &cullFaceEnabled);

        // Disable depth testing and culling for fullscreen quad
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        // Create temporary fullscreen triangle (more efficient than quad)
        GLuint vao, vbo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        // Fullscreen triangle vertices
        float vertices[] = {
            -1.0f, -1.0f,
             3.0f, -1.0f,
            -1.0f,  3.0f
        };

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

        // Draw triangle
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // Cleanup
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);

        // Restore previous OpenGL state
        if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
        if (cullFaceEnabled) glEnable(GL_CULL_FACE);
    }

} // namespace PixelCraft::Rendering::Stages
