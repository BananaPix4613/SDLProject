// -------------------------------------------------------------------------
// LightManager.cpp
// -------------------------------------------------------------------------
#include "Rendering/LightManager.h"
#include "Core/Application.h"
#include "Core/Logger.h"
#include "Rendering/ForwardPlusShaderManager.h"
#include "Rendering/RenderSystem.h"
#include "Rendering/Shader.h"
#include "Utility/Math.h"
#include "Utility/AABB.h"
#include "Utility/GraphicsCapabilities.h"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <cmath>

namespace PixelCraft::Rendering
{

    LightManager::LightManager()
        : m_clusteringEnabled(false)
        , m_cascadeCount(4)
        , m_cascadeSplitLambda(0.5f)
        , m_initialized(false)
    {
    }

    LightManager::~LightManager()
    {
        if (m_initialized)
        {
            shutdown();
        }
    }

    bool LightManager::initialize()
    {
        if (m_initialized)
        {
            Core::warn("LightManager already initialized");
            return true;
        }

        Core::info("Initializing LightManager subsystem");

        // Initialize the clustered light culler with default configuration
        m_clusterCuller = std::make_unique<ClusteredLightCuller>();
        if (!m_clusterCuller->initialize())
        {
            Core::error("Failed to initialize ClusteredLightCuller");
            return false;
        }
        m_clusteringEnabled = true;

        // Get ForwardPlusShaderManager for buffer coordination
        m_forwardPlusShaderManager = Core::Application::getInstance()->getSubsystem<Rendering::ForwardPlusShaderManager>();
        if (m_forwardPlusShaderManager)
        {
            // Set up integration with ForwardPlusShaderManager
            m_forwardPlusShaderManager->setClusteredLightCuller(m_clusterCuller);
            Core::info("LightManager: Integrated with ForwardPlusShaderManager");
        }
        else
        {
            Core::warn("LightManager: ForwardPlusShaderManager not found - GPU buffer management will be limited");
        }

        m_initialized = true;

        Core::info("LightManager initialized successfully with clustering enabled");
        return true;
    }

    std::vector<std::string> LightManager::getDependencies() const
    {
        return {"MemoryManager", "ForwardPlusShaderManager"};
    }

    void LightManager::update(float deltaTime)
    {
        (void)deltaTime;  // Currently unused

        // Update light clusters if clustering is enabled
        if (m_clusteringEnabled && m_clusterCuller)
        {
            auto renderSystem = Core::Application::getInstance()->getSubsystem<Rendering::RenderSystem>();
            if (renderSystem)
            {
                updateClusters(std::make_shared<RenderContext>(renderSystem->getRenderContext()));
            }
        }

        // Update shadow maps for all shadow-casting lights
        for (const auto &light : m_lights)
        {
            if (light->getCastShadows())
            {
                createShadowMapForLight(light);
            }
        }

        // Update GPU buffers with current light data
        updateGPUBuffers();
    }

    void LightManager::render()
    {
        // Render debug visualization of lights, light volumes, etc.
    }

    void LightManager::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        Core::info("Shutting down LightManager subsystem");

        // Shutdown cluster culler
        if (m_clusterCuller)
        {
            m_clusterCuller->shutdown();
            m_clusterCuller.reset();
        }
        m_clusteringEnabled = false;

        // Clear all lights
        m_lights.clear();
        m_visibleLights.clear();
        m_directionalLights.clear();
        m_pointLights.clear();
        m_spotLights.clear();
        m_areaLights.clear();

        // Release shadow maps
        m_shadowMaps.clear();
        m_lightShadowIndices.clear();

        // Release ForwardPlusShaderManager reference
        m_forwardPlusShaderManager.reset();

        m_initialized = false;
    }

    std::shared_ptr<Light> LightManager::createLight(LightType type)
    {
        auto light = std::make_shared<Light>(type);

        // Add the light to the appropriate lists
        m_lights.push_back(light);

        switch (type)
        {
        case LightType::Directional:
            m_directionalLights.push_back(light);
            break;
        case LightType::Point:
            m_pointLights.push_back(light);
            break;
        case LightType::Spot:
            m_spotLights.push_back(light);
            break;
        case LightType::Area:
            m_areaLights.push_back(light);
            break;
        }

        return light;
    }

    std::shared_ptr<Light> LightManager::createDirectionalLight()
    {
        return createLight(LightType::Directional);
    }

    std::shared_ptr<Light> LightManager::createPointLight()
    {
        return createLight(LightType::Point);
    }

    std::shared_ptr<Light> LightManager::createSpotLight()
    {
        return createLight(LightType::Spot);
    }

    std::shared_ptr<Light> LightManager::createAreaLight()
    {
        return createLight(LightType::Area);
    }

    void LightManager::removeLight(std::shared_ptr<Light> light)
    {
        if (!light)
        {
            return;
        }

        // Remove from light shadow indices if it had a shadow
        auto shadowIt = m_lightShadowIndices.find(light);
        if (shadowIt != m_lightShadowIndices.end())
        {
            m_lightShadowIndices.erase(shadowIt);
        }

        // Remove from the appropriate type-specific list
        switch (light->getType())
        {
        case LightType::Directional:
            {
                auto it = std::find(m_directionalLights.begin(), m_directionalLights.end(), light);
                if (it != m_directionalLights.end())
                {
                    m_directionalLights.erase(it);
                }
                break;
            }
        case LightType::Point:
            {
                auto it = std::find(m_pointLights.begin(), m_pointLights.end(), light);
                if (it != m_pointLights.end())
                {
                    m_pointLights.erase(it);
                }
                break;
            }
        case LightType::Spot:
            {
                auto it = std::find(m_spotLights.begin(), m_spotLights.end(), light);
                if (it != m_spotLights.end())
                {
                    m_spotLights.erase(it);
                }
                break;
            }
        case LightType::Area:
            {
                auto it = std::find(m_areaLights.begin(), m_areaLights.end(), light);
                if (it != m_areaLights.end())
                {
                    m_areaLights.erase(it);
                }
                break;
            }
        }

        // Remove from visible lights if present
        auto visIt = std::find(m_visibleLights.begin(), m_visibleLights.end(), light);
        if (visIt != m_visibleLights.end())
        {
            m_visibleLights.erase(visIt);
        }

        // Remove from main light list
        auto it = std::find(m_lights.begin(), m_lights.end(), light);
        if (it != m_lights.end())
        {
            m_lights.erase(it);
        }
    }

    const std::vector<std::shared_ptr<Light>> &LightManager::getLights() const
    {
        return m_lights;
    }

    const std::vector<std::shared_ptr<Light>> &LightManager::getVisibleLights() const
    {
        return m_visibleLights;
    }

    size_t LightManager::getLightCount() const
    {
        return m_lights.size();
    }

    void LightManager::updateLightData()
    {
        if (!m_initialized)
        {
            Core::warn("LightManager not initialized - cannot update light data");
            return;
        }

        Core::debug("LightManager: Forcing light data update with " + std::to_string(m_lights.size()) + " total lights, " +
                    std::to_string(m_visibleLights.size()) + " visible");

        // Force an immediate GPU buffer update
        updateGPUBuffers();

        Core::debug("LightManager: Light data update complete");
    }

    void LightManager::configureClusterCuller(const ClusterConfig &config)
    {
        if (!m_clusterCuller)
        {
            Core::error("ClusterCuller not initialized - cannot configure");
            return;
        }

        m_clusterCuller->configureGrid(config);
        Core::info("ClusterCuller configured with " + std::to_string(config.clusterCountX) + "x" +
                std::to_string(config.clusterCountY) + "x" + std::to_string(config.clusterCountZ) + " dimensions");
    }

    void LightManager::updateClusters(const std::shared_ptr<RenderContext> context)
    {
        if (!m_clusteringEnabled || !m_clusterCuller || !context)
        {
            return;
        }

        // Update the cluster culler with current context and visible lights
        m_clusterCuller->update(*context, m_visibleLights);

        // Update GPU buffers through ForwardPlusShaderManager
        updateGPUBuffers();
    }

    const ClusterData &LightManager::getClusterData() const
    {
        if (!m_clusterCuller)
        {
            // Return a default/empty cluster data if culler is not available
            static ClusterData emptyData;
            return emptyData;
        }

        return m_clusterCuller->getClusterData();
    }

    std::shared_ptr<ClusteredLightCuller> LightManager::getClusterCuller()
    {
        if (!m_clusterCuller)
        {
            throw std::runtime_error("ClusterCuller not initialized");
        }
        return m_clusterCuller;
    }

    bool LightManager::isClusteringEnabled() const
    {
        return m_clusteringEnabled && m_clusterCuller && m_clusterCuller->isInitialized();
    }

    void LightManager::prepareShadowMaps(const RenderContext &context)
    {
        // Create shadow maps for all shadow-casting lights
        for (auto &light : m_visibleLights)
        {
            if (light->getCastShadows())
            {
                createShadowMapForLight(light);
            }
        }

        // For directional lights, calculate cascaded shadow maps
        if (!m_directionalLights.empty() && m_directionalLights[0]->getCastShadows() && m_cascadeCount > 0)
        {
            calculateCascadeSplits(context);
        }
    }

    std::shared_ptr<RenderTarget> LightManager::getShadowMap(size_t index) const
    {
        if (index < m_shadowMaps.size())
        {
            return m_shadowMaps[index];
        }
        return nullptr;
    }

    void LightManager::setupCascadedShadowMaps(int cascadeCount, float splitLambda)
    {
        m_cascadeCount = glm::max(1, cascadeCount);
        m_cascadeSplitLambda = glm::clamp(splitLambda, 0.0f, 1.0f);

        // Resize cascade data structures
        m_cascadeSplits.resize(m_cascadeCount + 1);
        m_cascadeViewProjections.resize(m_cascadeCount);
    }

    const std::vector<glm::mat4> &LightManager::getCascadeViewProjections() const
    {
        return m_cascadeViewProjections;
    }

    const std::vector<float> &LightManager::getCascadeSplits() const
    {
        return m_cascadeSplits;
    }

    void LightManager::updateVisibleLights(const Utility::Frustum &frustum)
    {
        m_visibleLights.clear();

        // Directional lights are always visible
        for (auto &light : m_directionalLights)
        {
            m_visibleLights.push_back(light);
        }

        // Test point lights against the frustum
        for (auto &light : m_pointLights)
        {
            if (frustum.testSphere(light->getPosition(), light->getRange()))
            {
                m_visibleLights.push_back(light);
            }
        }

        // Test spot lights against the frustum
        for (auto &light : m_spotLights)
        {
            if (frustum.testSphere(light->getPosition(), light->getRange()))
            {
                m_visibleLights.push_back(light);
            }
        }

        // Test area lights against the frustum
        for (auto &light : m_areaLights)
        {
            if (frustum.testSphere(light->getPosition(),
                glm::length(glm::vec3(light->getAreaSize().x, light->getAreaSize().y, 0.0f))))
            {
                m_visibleLights.push_back(light);
            }
        }
    }

    void LightManager::updateGPUBuffers()
    {
        if (!m_initialized)
        {
            Core::warn("LightManager not initialized - cannot update GPU buffers");
            return;
        }

        // Prepare light data for GPU
        std::vector<LightData> lightDataArray;
        lightDataArray.reserve(m_visibleLights.size());

        for (size_t i = 0; i < m_visibleLights.size(); ++i)
        {
            auto &light = m_visibleLights[i];
            LightData data = light->packLightData();

            // Set shadow index if this light has a shadow map
            auto it = m_lightShadowIndices.find(light);
            if (it != m_lightShadowIndices.end())
            {
                data.metadata.x = static_cast<float>(it->second);
            }
            else
            {
                data.metadata.x = -1.0f; // Invalid shadow index
            }

            lightDataArray.push_back(data);
        }

        // Delegate GPU buffer updates to ForwardPlusShaderManager
        if (m_forwardPlusShaderManager && m_forwardPlusShaderManager->isForwardPlusReady())
        {
            // Use the integrated forward+ buffer management
            if (m_clusteringEnabled && m_clusterCuller)
            {
                // Update all forward+ buffers with cluster data
                const auto &clusterData = m_clusterCuller->getClusterData();
                if (!m_forwardPlusShaderManager->updateForwardPlusBuffers(lightDataArray, &clusterData))
                {
                    Core::error("LightManager: Failed to update forward+ buffers through ForwardPlusShaderManager");
                }
            }
            else
            {
                // Update only visible lights buffer
                if (!m_forwardPlusShaderManager->updateVisibleLights(lightDataArray))
                {
                    Core::error("LightManager: Failed to update visible lights through ForwardPlusShaderManager");
                }
            }
        }
        else
        {
            // Fallback: Log that forward+ integration is not available
            Core::warn("LightManager: ForwardPlusShaderManager not available - GPU buffers not updated");

            if (!lightDataArray.empty())
            {
                Core::info("LightManager: Would update " + std::to_string(lightDataArray.size()) + " lights");
            }
        }
    }

    uint32_t LightManager::getLightDataBuffer() const
    {
        if (m_forwardPlusShaderManager)
        {
            return m_forwardPlusShaderManager->getVisibleLightsBuffer();
        }
        return 0;
    }

    uint32_t LightManager::getClusterDataBuffer() const
    {
        if (m_forwardPlusShaderManager)
        {
            return m_forwardPlusShaderManager->getClusterDataBuffer();
        }
        return 0;
    }

    void LightManager::setForwardPlusShaderManager(std::shared_ptr<ForwardPlusShaderManager> forwardPlusManager)
    {
        m_forwardPlusShaderManager = forwardPlusManager;

        if (m_forwardPlusShaderManager && m_clusterCuller)
        {
            // Set up integration with the cluster culler
            m_forwardPlusShaderManager->setClusteredLightCuller(m_clusterCuller);
            Core::info("LightManager: ForwardPlusShaderManager integration established");
        }
    }

    std::shared_ptr<ForwardPlusShaderManager> LightManager::getForwardPlusShaderManager() const
    {
        return m_forwardPlusShaderManager;
    }

    void LightManager::createShadowMapForLight(std::shared_ptr<Light> light)
    {
        if (!light || !light->getCastShadows())
        {
            return;
        }

        // Check if the light already has a shadow map
        auto it = m_lightShadowIndices.find(light);
        if (it != m_lightShadowIndices.end())
        {
            return; // Shadow map already exists
        }

        // Use graphics capabilities to determine appropriate shadow map settings
        auto& capabilities = Utility::GraphicsCapabilities::getInstance();
        if (!capabilities.initialize())
        {
            Core::error("LightManager: Failed to initialize graphics capabilities");
            return;
        }

        // Check if shadow mapping is supported at all
        if (!capabilities.supportsDepthTextures())
        {
            Core::warn("LightManager: Hardware does not support depth textures - disabling shadow casting for light");
            light->setCastShadows(false);
            return;
        }

        // Determine shadow map characteristics based on light type and hardware capabilities
        int baseResolution = capabilities.getRecommendedShadowMapResolution();
        int resolution;
        bool needsCubemap = false;
        
        switch (light->getType())
        {
        case LightType::Directional:
            // Directional lights can use higher resolution if hardware supports it
            resolution = capabilities.supportsAdvancedShadowMapping() ? 
                        std::min(2048, capabilities.getMaxTextureSize()) : baseResolution;
            break;
        case LightType::Point:
            resolution = baseResolution;
            needsCubemap = true; // Point lights need cubemap for omnidirectional shadows
            break;
        case LightType::Spot:
            resolution = baseResolution;
            break;
        case LightType::Area:
            resolution = baseResolution;
            break;
        default:
            Core::warn("LightManager: Unknown light type for shadow map creation");
            return;
        }

        // Try multiple fallback resolutions if the preferred one fails
        std::vector<int> fallbackResolutions = {resolution, resolution/2, resolution/4, 512, 256};
        bool shadowMapCreated = false;
        
        for (int testResolution : fallbackResolutions)
        {
            if (testResolution <= 0 || testResolution > capabilities.getMaxTextureSize()) continue;
            
            // Create the shadow map render target with current test resolution
            auto shadowMap = std::make_shared<RenderTarget>(testResolution, testResolution, false);
            
            if (!shadowMap->initialize())
            {
                Core::warn("LightManager: Failed to initialize shadow map render target with resolution " + std::to_string(testResolution));
                continue;
            }

            // Try multiple depth formats, starting with the most preferred
            std::vector<TextureFormat> depthFormats = {
                TextureFormat::Depth32F,
                TextureFormat::Depth24Stencil8,
                TextureFormat::Depth16
            };
            
            bool depthAttachmentCreated = false;
            for (auto format : depthFormats)
            {
                if (shadowMap->setDepthAttachment(format))
                {
                    depthAttachmentCreated = true;
                    break;
                }
                Core::warn("LightManager: Failed to create depth attachment with format " + std::to_string(static_cast<int>(format)));
            }
            
            if (!depthAttachmentCreated)
            {
                Core::warn("LightManager: Could not create any depth attachment for shadow map with resolution " + std::to_string(testResolution));
                continue;
            }

            // Validate the shadow map framebuffer
            if (!shadowMap->validate())
            {
                Core::warn("LightManager: Shadow map framebuffer validation failed for resolution " + std::to_string(testResolution));
                continue;
            }

            // Success! Add shadow map to the list and record its index
            size_t shadowIndex = m_shadowMaps.size();
            m_shadowMaps.push_back(shadowMap);
            m_lightShadowIndices[light] = shadowIndex;

            Core::info("LightManager: Created shadow map " + std::to_string(shadowIndex) + 
                       " for " + (needsCubemap ? "cubemap " : "") + "light with resolution " + 
                       std::to_string(testResolution) + "x" + std::to_string(testResolution));
            shadowMapCreated = true;
            break;
        }
        
        if (!shadowMapCreated)
        {
            Core::error("LightManager: Failed to create shadow map for light after trying all fallback resolutions");
            // Disable shadow casting for this light to prevent continued failures
            light->setCastShadows(false);
            Core::warn("LightManager: Disabled shadow casting for problematic light");
        }
    }

    void LightManager::calculateCascadeSplits(const RenderContext &context)
    {
        if (m_cascadeCount <= 0 || m_directionalLights.empty())
        {
            return;
        }

        // Get near and far planes from the camera context
        const float nearPlane = context.getNearPlane();
        const float farPlane = context.getFarPlane();
        const float clipRange = farPlane - nearPlane;

        if (clipRange <= 0.0f)
        {
            Core::warn("LightManager: Invalid clip range for cascade calculation");
            return;
        }

        // Calculate cascade split distances using logarithmic/uniform blend
        for (int i = 0; i <= m_cascadeCount; ++i)
        {
            float p = static_cast<float>(i) / static_cast<float>(m_cascadeCount);
            
            // Logarithmic distribution
            float logSplit = nearPlane * std::pow(farPlane / nearPlane, p);
            
            // Uniform distribution
            float uniformSplit = nearPlane + clipRange * p;
            
            // Blend between logarithmic and uniform based on lambda
            float blendedSplit = m_cascadeSplitLambda * logSplit + (1.0f - m_cascadeSplitLambda) * uniformSplit;
            
            m_cascadeSplits[i] = blendedSplit;
        }

        // Get the main directional light for shadow mapping
        auto mainLight = m_directionalLights[0];
        const glm::mat4 lightViewMatrix = mainLight->getViewMatrix();

        // Calculate view-projection matrix for each cascade
        for (int cascadeIndex = 0; cascadeIndex < m_cascadeCount; ++cascadeIndex)
        {
            float nearSplit = m_cascadeSplits[cascadeIndex];
            float farSplit = m_cascadeSplits[cascadeIndex + 1];

            // Create a temporary perspective projection matrix for this cascade
            glm::mat4 cascadeProjection = glm::perspective(
                glm::radians(45.0f), // Use a reasonable FOV - could be extracted from actual camera
                1.0f, // Assume square aspect ratio for now
                nearSplit,
                farSplit
            );

            // Create a frustum for this cascade by combining the cascade projection with camera view
            glm::mat4 cascadeViewProjection = cascadeProjection * context.getViewMatrix();
            
            // Create a frustum and extract its corners
            Utility::Frustum cascadeFrustum;
            cascadeFrustum.update(cascadeViewProjection);
            
            // Get the 8 corners of the frustum for this cascade
            const auto& frustumCorners = cascadeFrustum.getCorners();
            
            // Convert corners array to vector for AABB creation
            std::vector<glm::vec3> cornerPoints(frustumCorners.begin(), frustumCorners.end());
            
            // Create an AABB that contains all frustum corners in world space
            Utility::AABB worldBounds = Utility::AABB::createFromPoints(cornerPoints);
            
            // Transform the AABB to light view space
            Utility::AABB lightSpaceBounds = Utility::AABB::createFromTransformedAABB(worldBounds, lightViewMatrix);
            
            // Get the bounds in light space
            glm::vec3 minBounds = lightSpaceBounds.getMin();
            glm::vec3 maxBounds = lightSpaceBounds.getMax();
            
            // Extend the Z bounds to include potential shadow casters outside the frustum
            const float zExtension = glm::abs(maxBounds.z - minBounds.z);
            minBounds.z -= zExtension;
            maxBounds.z += zExtension * 0.5f;

            // Create orthographic projection matrix for this cascade
            glm::mat4 lightProjection = glm::ortho(
                minBounds.x, maxBounds.x,
                minBounds.y, maxBounds.y,
                minBounds.z, maxBounds.z
            );

            // Store the combined view-projection matrix for this cascade
            m_cascadeViewProjections[cascadeIndex] = lightProjection * lightViewMatrix;
        }

        Core::debug("LightManager: Calculated " + std::to_string(m_cascadeCount) + 
                    " cascade splits from " + std::to_string(nearPlane) + 
                    " to " + std::to_string(farPlane));
    }

} // namespace PixelCraft::Rendering