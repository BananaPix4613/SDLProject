// -------------------------------------------------------------------------
// ShadowStage.cpp
// -------------------------------------------------------------------------
#include "Rendering/Stages/ShadowStage.h"
#include "Core/Logger.h"
#include "Rendering/Shader.h"
#include "Rendering/RenderContext.h"
#include "Rendering/RenderTarget.h"
#include "Rendering/LightManager.h"
#include "Utility/Profiler.h"
#include "Utility/Math.h"

#include <algorithm>

namespace PixelCraft::Rendering::Stages
{

    ShadowStage::ShadowStage()
        : RenderStage("ShadowStage")
        , m_shadowShader(nullptr)
        , m_lightManager(nullptr)
        , m_registry(nullptr)
        , m_shadowMapResolution(1024)
        , m_shadowBias(0.005f)
        , m_cascadeCount(3)
        , m_cascadeSplitLambda(0.5f)
        , m_softShadowsEnabled(false)
        , m_profilingMarker("ShadowMapping")
    {
    }

    ShadowStage::~ShadowStage()
    {
        shutdown();
    }

    bool ShadowStage::initialize()
    {
        if (isInitialized())
        {
            Core::warn("ShadowStage already initialized");
            return true;
        }

        Core::info("Initializing ShadowStage");

        // Shadow maps will be created and managed by LightManager
        setInitialized(true);
        return true;
    }

    void ShadowStage::shutdown()
    {
        if (!isInitialized())
        {
            return;
        }

        Core::info("Shutting down ShadowStage");

        m_shadowShader.reset();
        m_lightManager.reset();
        m_registry.reset();
        
        // Clear local shadow map containers (they're managed by LightManager now)
        m_shadowMaps.clear();
        m_cubeShadowMaps.clear();

        setInitialized(false);
    }

    void ShadowStage::execute(const std::shared_ptr<RenderContext> context)
    {
        if (!isInitialized() || !isEnabled())
        {
            Core::warn("ShadowStage: Not initialized or enabled");
            return;
        }

        PROFILE_SCOPE("ShadowStage::execute");

        if (!m_shadowShader || !m_lightManager || !m_registry)
        {
            Core::warn("ShadowStage: Missing shader, light manager, or registry");
            return;
        }

        // Bind output for consistency (even though we manage our own targets)
        bindOutput();

        // Configure LightManager with our shadow settings
        configureLightManagerShadows();

        // Let LightManager prepare shadow maps (this will call our implemented methods)
        m_lightManager->prepareShadowMaps(*context);

        // Get visible shadow-casting lights from LightManager
        const auto& visibleLights = m_lightManager->getVisibleLights();

        // Setup shadow render state
        setupShadowRenderState();

        // Render shadow maps for each visible shadow-casting light
        for (const auto& light : visibleLights)
        {
            if (light->getCastShadows())
            {
                renderLightShadowMap(light, context);
            }
        }

        // Restore render state
        restoreRenderState();

        // Unbind output for consistency
        unbindOutput();
    }

    void ShadowStage::setShadowShader(std::shared_ptr<Shader> shader)
    {
        m_shadowShader = shader;
    }

    std::shared_ptr<Shader> ShadowStage::getShadowShader() const
    {
        return m_shadowShader;
    }

    void ShadowStage::setLightManager(std::shared_ptr<LightManager> lightManager)
    {
        m_lightManager = lightManager;
        
        // Configure LightManager with our current settings
        if (m_lightManager)
        {
            configureLightManagerShadows();
        }
    }

    void ShadowStage::setRegistry(std::shared_ptr<ECS::Registry> registry)
    {
        m_registry = registry;
    }

    void ShadowStage::setShadowMapResolution(int resolution)
    {
        m_shadowMapResolution = resolution;
        // No need to clear shadow maps here - LightManager handles this
        Core::info("ShadowStage: Set shadow map resolution to " + std::to_string(resolution));
    }

    int ShadowStage::getShadowMapResolution() const
    {
        return m_shadowMapResolution;
    }

    void ShadowStage::setShadowBias(float bias)
    {
        m_shadowBias = bias;
    }

    float ShadowStage::getShadowBias() const
    {
        return m_shadowBias;
    }

    void ShadowStage::setCascadeCount(int cascadeCount)
    {
        m_cascadeCount = std::clamp(cascadeCount, 1, 8);
        
        // Update LightManager with new cascade settings
        if (m_lightManager)
        {
            configureLightManagerShadows();
        }
    }

    int ShadowStage::getCascadeCount() const
    {
        return m_cascadeCount;
    }

    void ShadowStage::setCascadeSplitLambda(float lambda)
    {
        m_cascadeSplitLambda = std::clamp(lambda, 0.0f, 1.0f);
        
        // Update LightManager with new cascade settings
        if (m_lightManager)
        {
            configureLightManagerShadows();
        }
    }

    float ShadowStage::getCascadeSplitLambda() const
    {
        return m_cascadeSplitLambda;
    }

    void ShadowStage::setSoftShadowsEnabled(bool enabled)
    {
        m_softShadowsEnabled = enabled;
    }

    bool ShadowStage::isSoftShadowsEnabled() const
    {
        return m_softShadowsEnabled;
    }

    void ShadowStage::configureLightManagerShadows()
    {
        if (!m_lightManager)
        {
            return;
        }

        // Configure LightManager's cascaded shadow map settings
        m_lightManager->setupCascadedShadowMaps(m_cascadeCount, m_cascadeSplitLambda);
        
        Core::debug("ShadowStage: Configured LightManager with " + std::to_string(m_cascadeCount) + 
                   " cascades and lambda " + std::to_string(m_cascadeSplitLambda));
    }

    void ShadowStage::renderLightShadowMap(std::shared_ptr<Light> light, const std::shared_ptr<RenderContext> context)
    {
        switch (light->getType())
        {
            case LightType::Directional:
                renderDirectionalShadowMap(light, context);
                break;
            case LightType::Point:
                renderPointShadowMap(light, context);
                break;
            case LightType::Spot:
                renderSpotShadowMap(light, context);
                break;
            case LightType::Area:
                // Area lights can use spot light shadow mapping for now
                renderSpotShadowMap(light, context);
                break;
        }
    }

    void ShadowStage::renderDirectionalShadowMap(std::shared_ptr<Light> light, const std::shared_ptr<RenderContext> context)
    {
        // Check if light still has shadow casting enabled (might have been disabled due to errors)
        if (!light->getCastShadows())
        {
            Core::debug("ShadowStage: Light has shadow casting disabled, skipping shadow map generation");
            return;
        }

        // Use LightManager's cascade view-projection matrices
        const auto& cascadeViewProjections = m_lightManager->getCascadeViewProjections();
        const auto& cascadeSplits = m_lightManager->getCascadeSplits();

        bool anyCascadeRendered = false;
        for (int cascade = 0; cascade < m_cascadeCount; ++cascade)
        {
            // Get shadow map from LightManager instead of creating our own
            auto shadowMap = getShadowMapFromLightManager(light, cascade);
            if (!shadowMap)
            {
                // Only warn once per frame to avoid log spam
                static std::set<std::pair<std::shared_ptr<Light>, int>> warnedCascades;
                auto cascadeKey = std::make_pair(light, cascade);
                if (warnedCascades.find(cascadeKey) == warnedCascades.end())
                {
                    Core::warn("ShadowStage: Failed to get shadow map for cascade " + std::to_string(cascade) + " - disabling further warnings for this cascade");
                    warnedCascades.insert(cascadeKey);
                }
                continue;
            }

            // Bind shadow map with error checking
            shadowMap->bind();
            GLenum error = glGetError();
            if (error != GL_NO_ERROR)
            {
                Core::error("ShadowStage: OpenGL error when binding shadow map for cascade " + std::to_string(cascade) + ": " + std::to_string(error));
                shadowMap->unbind();
                continue;
            }

            shadowMap->clear(false, true, false);
            error = glGetError();
            if (error != GL_NO_ERROR)
            {
                Core::error("ShadowStage: OpenGL error when clearing shadow map for cascade " + std::to_string(cascade) + ": " + std::to_string(error));
                shadowMap->unbind();
                continue;
            }

            // Use pre-calculated cascade view-projection matrix from LightManager
            if (cascade < static_cast<int>(cascadeViewProjections.size()))
            {
                glm::mat4 lightViewProj = cascadeViewProjections[cascade];

                // Bind shader and set uniforms with error checking
                if (m_shadowShader)
                {
                    m_shadowShader->bind();
                    error = glGetError();
                    if (error != GL_NO_ERROR)
                    {
                        Core::error("ShadowStage: OpenGL error when binding shadow shader for cascade " + std::to_string(cascade) + ": " + std::to_string(error));
                        shadowMap->unbind();
                        continue;
                    }

                    m_shadowShader->setMat4("lightViewProj", lightViewProj);
                    m_shadowShader->setFloat("cascadeIndex", static_cast<float>(cascade));

                    // Set cascade split distances for shader
                    if (cascade < static_cast<int>(cascadeSplits.size()) - 1)
                    {
                        m_shadowShader->setFloat("cascadeNear", cascadeSplits[cascade]);
                        m_shadowShader->setFloat("cascadeFar", cascadeSplits[cascade + 1]);
                    }

                    // Render shadow casters
                    renderShadowCasters(lightViewProj);

                    m_shadowShader->unbind();
                    anyCascadeRendered = true;
                }
                else
                {
                    Core::warn("ShadowStage: No shadow shader available for cascade " + std::to_string(cascade));
                }
            }

            shadowMap->unbind();
            error = glGetError();
            if (error != GL_NO_ERROR)
            {
                Core::error("ShadowStage: OpenGL error when unbinding shadow map for cascade " + std::to_string(cascade) + ": " + std::to_string(error));
            }
        }

        // If no cascades could be rendered, disable shadow casting to prevent further issues
        if (!anyCascadeRendered && light->getCastShadows())
        {
            Core::warn("ShadowStage: No cascades could be rendered for directional light, disabling shadow casting");
            light->setCastShadows(false);
        }
    }

    void ShadowStage::renderPointShadowMap(std::shared_ptr<Light> light, const std::shared_ptr<RenderContext> context)
    {
        // Get shadow map from LightManager
        auto shadowMap = getShadowMapFromLightManager(light);
        if (!shadowMap)
        {
            Core::warn("ShadowStage: Failed to get point light shadow map");
            return;
        }

        // Point lights require cube map shadows (6 faces)
        // For now, we'll render a single face as a simplified implementation
        shadowMap->bind();
        shadowMap->clear(false, true, false);

        // Calculate light view-projection matrix
        glm::mat4 lightView = light->getViewMatrix();
        glm::mat4 lightProj = light->getProjectionMatrix();
        glm::mat4 lightViewProj = lightProj * lightView;

        // Bind shader and set uniforms
        m_shadowShader->bind();
        m_shadowShader->setMat4("lightViewProj", lightViewProj);
        m_shadowShader->setVec3("lightPosition", light->getPosition());
        m_shadowShader->setFloat("lightRange", light->getRange());

        // Render shadow casters
        renderShadowCasters(lightViewProj);

        m_shadowShader->unbind();
        shadowMap->unbind();
    }

    void ShadowStage::renderSpotShadowMap(std::shared_ptr<Light> light, const std::shared_ptr<RenderContext> context)
    {
        // Get shadow map from LightManager
        auto shadowMap = getShadowMapFromLightManager(light);
        if (!shadowMap)
        {
            Core::warn("ShadowStage: Failed to get spot light shadow map");
            return;
        }

        shadowMap->bind();
        shadowMap->clear(false, true, false);

        // Calculate light view-projection matrix
        glm::mat4 lightView = light->getViewMatrix();
        glm::mat4 lightProj = light->getProjectionMatrix();
        glm::mat4 lightViewProj = lightProj * lightView;

        // Bind shader and set uniforms
        m_shadowShader->bind();
        m_shadowShader->setMat4("lightViewProj", lightViewProj);
        m_shadowShader->setVec3("lightPosition", light->getPosition());
        m_shadowShader->setVec3("lightDirection", light->getDirection());
        m_shadowShader->setFloat("lightRange", light->getRange());
        m_shadowShader->setFloat("spotInnerAngle", light->getInnerConeAngle());
        m_shadowShader->setFloat("spotOuterAngle", light->getOuterConeAngle());

        // Render shadow casters
        renderShadowCasters(lightViewProj);

        m_shadowShader->unbind();
        shadowMap->unbind();
    }

    void ShadowStage::renderShadowCasters(const glm::mat4& lightViewProj)
    {
        (void)lightViewProj;  // Currently unused
        // TODO: Implement shadow caster rendering when ECS components and/or normal geometry are available
        // This would iterate through entities with shadow-casting mesh renderers
        /*
        if (m_registry) {
            m_registry->view<TransformComponent, MeshRendererComponent>().each([&](auto entity, auto& transform, auto& meshRenderer) {
                if (meshRenderer.castShadows && meshRenderer.mesh) {
                    glm::mat4 modelMatrix = transform.getWorldMatrix();
                    m_shadowShader->setMat4("modelMatrix", modelMatrix);
                    
                    meshRenderer.mesh->bind();
                    meshRenderer.mesh->draw();
                    meshRenderer.mesh->unbind();
                }
            });
        }
        */
        
        // Placeholder for now - just log that we would render shadow casters
        Core::debug("ShadowStage: Would render shadow casters here");
    }

    std::shared_ptr<RenderTarget> ShadowStage::getShadowMapFromLightManager(std::shared_ptr<Light> light, int cascadeIndex)
    {
        if (!m_lightManager)
        {
            return nullptr;
        }

        // For directional lights with cascades, we need to determine the shadow map index
        // This is a simplified approach - in a full implementation, LightManager would
        // provide a more direct way to get cascade-specific shadow maps
        
        const auto& lights = m_lightManager->getLights();
        size_t lightIndex = 0;
        bool found = false;
        
        for (size_t i = 0; i < lights.size(); ++i)
        {
            if (lights[i] == light)
            {
                lightIndex = i;
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            return nullptr;
        }

        // For directional lights, cascade index affects the shadow map index
        if (light->getType() == LightType::Directional && cascadeIndex >= 0)
        {
            lightIndex = lightIndex * m_cascadeCount + cascadeIndex;
        }

        return m_lightManager->getShadowMap(lightIndex);
    }

    void ShadowStage::setupShadowRenderState()
    {
        // Enable depth testing
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);

        // Disable color writing for shadow mapping
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

        // Enable polygon offset to reduce shadow acne
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(m_shadowBias, m_shadowBias);

        // Enable front-face culling for shadow mapping to reduce peter-panning
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
    }

    void ShadowStage::restoreRenderState()
    {
        // Re-enable color writing
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        // Disable polygon offset
        glDisable(GL_POLYGON_OFFSET_FILL);

        // Restore back-face culling
        glCullFace(GL_BACK);
    }

} // namespace PixelCraft::Rendering::Stages
