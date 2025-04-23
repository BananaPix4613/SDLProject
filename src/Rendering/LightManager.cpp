// -------------------------------------------------------------------------
// LightManager.cpp
// -------------------------------------------------------------------------
#include "Rendering/LightManager.h"
#include "Core/Logger.h"
#include "Core/Application.h"
#include "Rendering/Shader.h"
#include "Utility/Math.h"
#include "glm/glm.hpp"
#include "gtx/transform.hpp"

namespace Log = PixelCraft::Core;

namespace PixelCraft::Rendering
{

    //-------------------------------------------------------------------------
    // Light implementation
    //-------------------------------------------------------------------------

    Light::Light(LightType type)
        : m_type(type)
        , m_position(0.0f, 0.0f, 0.0f)
        , m_direction(0.0f, -1.0f, 0.0f)
        , m_color(1.0f, 1.0f, 1.0f)
        , m_intensity(1.0f)
        , m_range(10.0f)
        , m_castShadows(false)
        , m_spotInnerAngle(glm::radians(30.0f))
        , m_spotOuterAngle(glm::radians(45.0f))
        , m_areaSize(1.0f, 1.0f)
        , m_shadowBias(0.005f)
        , m_shadowResolution(1024)
    {
        // Normalize the direction vector
        if (glm::length(m_direction) > 0.0f)
        {
            m_direction = glm::normalize(m_direction);
        }
    }

    Light::~Light()
    {
        // No special cleanup needed
    }

    void Light::setPosition(const glm::vec3& position)
    {
        m_position = position;
    }

    void Light::setDirection(const glm::vec3& direction)
    {
        m_direction = glm::normalize(direction);
    }

    void Light::setColor(const glm::vec3& color)
    {
        m_color = color;
    }

    void Light::setIntensity(float intensity)
    {
        m_intensity = glm::max(0.0f, intensity);
    }

    void Light::setRange(float range)
    {
        m_range = glm::max(0.1f, range);
    }

    void Light::setCastShadows(bool castShadows)
    {
        m_castShadows = castShadows;
    }

    void Light::setSpotAngle(float innerAngle, float outerAngle)
    {
        m_spotInnerAngle = glm::clamp(innerAngle, 0.0f, glm::radians(175.0f));
        m_spotOuterAngle = glm::clamp(outerAngle, m_spotInnerAngle, glm::radians(175.0f));
    }

    void Light::setAreaSize(const glm::vec2& size)
    {
        m_areaSize = glm::max(size, glm::vec2(0.01f));
    }

    LightType Light::getType() const
    {
        return m_type;
    }

    const glm::vec3& Light::getPosition() const
    {
        return m_position;
    }

    const glm::vec3& Light::getDirection() const
    {
        return m_direction;
    }

    const glm::vec3& Light::getColor() const
    {
        return m_color;
    }

    float Light::getIntensity() const
    {
        return m_intensity;
    }

    float Light::getRange() const
    {
        return m_range;
    }

    bool Light::getCastShadows() const
    {
        return m_castShadows;
    }

    void Light::setShadowBias(float bias)
    {
        m_shadowBias = glm::clamp(bias, 0.0f, 0.1f);
    }

    void Light::setShadowResolution(int resolution)
    {
        // Ensure resolution is a power of two
        int powerOfTwo = 1;
        while (powerOfTwo < resolution && powerOfTwo < 8192)
        {
            powerOfTwo *= 2;
        }
        m_shadowResolution = powerOfTwo;
    }

    glm::mat4 Light::getViewMatrix() const
    {
        switch (m_type)
        {
            case LightType::Directional:
            {
                // For directional lights, create a view matrix looking in the light direction
                const glm::vec3 up = glm::abs(m_direction.y) > 0.999f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
                return glm::lookAt(glm::vec3(0.0f), m_direction, up);
            }
            case LightType::Point:
            case LightType::Spot:
            {
                // For point and spot lights, create a view matrix from the light position
                const glm::vec3 target = m_position + m_direction;
                const glm::vec3 up = glm::abs(m_direction.y) > 0.999f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
                return glm::lookAt(m_position, target, up);
            }
            case LightType::Area:
            {
                // For area lights, create a view matrix from the light position
                const glm::vec3 target = m_position + m_direction;
                const glm::vec3 up = glm::abs(m_direction.y) > 0.999f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
                return glm::lookAt(m_position, target, up);
            }
            default:
                return glm::mat4(1.0f);
        }
    }

    glm::mat4 Light::getProjectionMatrix() const
    {
        switch (m_type)
        {
            case LightType::Directional:
                // Orthographic projection for directional lights
                return glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.1f, 100.0f);

            case LightType::Point:
                // For point lights, we would typically use cubemap projections
                // This is a placeholder - actual implementation would handle all six faces
                return glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, m_range);

            case LightType::Spot:
            {
                // For spot lights, use a perspective projection
                float aspectRatio = 1.0f;
                float fovY = 2.0f * m_spotOuterAngle;
                return glm::perspective(fovY, aspectRatio, 0.1f, m_range);
            }

            case LightType::Area:
                // For area lights, use an orthographic projection
                return glm::ortho(-m_areaSize.x * 0.5f, m_areaSize.x * 0.5f,
                                  -m_areaSize.y * 0.5f, m_areaSize.y * 0.5f,
                                  0.1f, 50.0f);

            default:
                return glm::mat4(1.0f);
        }
    }

    LightData Light::packLightData() const
    {
        LightData data;
        data.position = m_position;
        data.range = m_range;
        data.color = m_color;
        data.intensity = m_intensity;
        data.direction = m_direction;
        data.spotAngle = m_type == LightType::Spot ? cos(m_spotOuterAngle) : -1.0f;
        data.areaSize = m_areaSize;
        data.shadowIndex = 0; // Will be set by LightManager
        data.type = m_type;
        data.castShadows = m_castShadows;
        return data;
    }

    //-------------------------------------------------------------------------
    // LightManager implementation
    //-------------------------------------------------------------------------

    LightManager::LightManager()
        : m_clustersDirty(true)
        , m_cascadeCount(4)
        , m_cascadeSplitLambda(0.5f)
        , m_lightDataBuffer(0)
        , m_clusterDataBuffer(0)
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
            Log::warn("LightManager already initialized");
            return true;
        }

        Log::info("Initializing LightManager subsystem");

        // Initialize the cluster data with default values
        m_clusterData.dimensions = glm::ivec3(16, 8, 24); // Default cluster grid size
        m_clusterData.minBounds = glm::vec3(-50.0f, -50.0f, -50.0f);
        m_clusterData.maxBounds = glm::vec3(50.0f, 50.0f, 50.0f);

        // Allocate space for the light grid and indices
        const int totalClusters = m_clusterData.dimensions.x * m_clusterData.dimensions.y * m_clusterData.dimensions.z;
        m_clusterData.lightGrid.resize(totalClusters, 0);
        m_clusterData.lightIndices.reserve(1024); // Reserve space for light indices

        // Create GPU buffers for light data and cluster data
        // In a real implementation, these would be created using the graphics API
        m_lightDataBuffer = 1; // Placeholder
        m_clusterDataBuffer = 2; // Placeholder

        m_initialized = true;
        return true;
    }

    void LightManager::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        Log::info("Shutting down LightManager subsystem");

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

        // Release GPU buffers
        // In a real implementation, these would be deleted using the graphics API
        m_lightDataBuffer = 0;
        m_clusterDataBuffer = 0;

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

        // Mark clusters as dirty
        m_clustersDirty = true;

        return light;
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

        // Mark clusters as dirty
        m_clustersDirty = true;
    }

    const std::vector<std::shared_ptr<Light>>& LightManager::getLights() const
    {
        return m_lights;
    }

    const std::vector<std::shared_ptr<Light>>& LightManager::getVisibleLights() const
    {
        return m_visibleLights;
    }

    void LightManager::setupClusters(const glm::ivec3& dimensions)
    {
        m_clusterData.dimensions = glm::max(dimensions, glm::ivec3(1));

        // Resize the light grid for the new dimensions
        const int totalClusters = m_clusterData.dimensions.x * m_clusterData.dimensions.y * m_clusterData.dimensions.z;
        m_clusterData.lightGrid.resize(totalClusters, 0);

        // Mark clusters as dirty to trigger a rebuild
        m_clustersDirty = true;
    }

    void LightManager::updateClusters(const RenderContext& context)
    {
        if (!m_clustersDirty && !m_visibleLights.empty())
        {
            return;
        }

        // Update the cluster bounds based on the view frustum
        // For simplicity, we'll use the camera frustum corners to determine bounds
        const auto& frustum = context.getFrustum();
        const auto& corners = frustum.getCorners();

        // Calculate min/max bounds from the frustum corners
        glm::vec3 minBounds = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 maxBounds = glm::vec3(std::numeric_limits<float>::lowest());

        for (const auto& corner : corners)
        {
            minBounds = glm::min(minBounds, corner);
            maxBounds = glm::max(maxBounds, corner);
        }

        m_clusterData.minBounds = minBounds;
        m_clusterData.maxBounds = maxBounds;

        // Clear existing light assignments
        std::fill(m_clusterData.lightGrid.begin(), m_clusterData.lightGrid.end(), 0);
        m_clusterData.lightIndices.clear();

        // Assign lights to clusters
        assignLightsToClusters();

        // Update the cluster buffer
        updateGPUBuffers();

        m_clustersDirty = false;
    }

    const ClusterData& LightManager::getClusterData() const
    {
        return m_clusterData;
    }

    void LightManager::prepareShadowMaps(const RenderContext& context)
    {
        // Create shadow maps for all shadow-casting lights
        for (auto& light : m_visibleLights)
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

    const std::vector<glm::mat4>& LightManager::getCascadeViewProjections() const
    {
        return m_cascadeViewProjections;
    }

    const std::vector<float>& LightManager::getCascadeSplits() const
    {
        return m_cascadeSplits;
    }

    void LightManager::updateVisibleLights(const Utility::Frustum& frustum)
    {
        m_visibleLights.clear();

        // Directional lights are always visible
        for (auto& light : m_directionalLights)
        {
            m_visibleLights.push_back(light);
        }

        // Test point lights against the frustum
        for (auto& light : m_pointLights)
        {
            if (frustum.testSphere(light->getPosition(), light->getRange()))
            {
                m_visibleLights.push_back(light);
            }
        }

        // Test spot lights against the frustum
        for (auto& light : m_spotLights)
        {
            if (frustum.testSphere(light->getPosition(), light->getRange()))
            {
                m_visibleLights.push_back(light);
            }
        }

        // Test area lights against the frustum
        for (auto& light : m_areaLights)
        {
            if (frustum.testSphere(light->getPosition(), glm::length(glm::vec3(light->getAreaSize().x, light->getAreaSize().y, 0.0f))))
            {
                m_visibleLights.push_back(light);
            }
        }

        // Mark clusters as dirty since visible lights changed
        m_clustersDirty = true;
    }

    void LightManager::updateGPUBuffers()
    {
        // Prepare light data for GPU
        std::vector<LightData> lightDataArray;
        lightDataArray.reserve(m_visibleLights.size());

        for (size_t i = 0; i < m_visibleLights.size(); ++i)
        {
            auto& light = m_visibleLights[i];
            LightData data = light->packLightData();

            // Set shadow index if this light has a shadow map
            auto it = m_lightShadowIndices.find(light);
            if (it != m_lightShadowIndices.end())
            {
                data.shadowIndex = static_cast<uint32_t>(it->second);
            }

            lightDataArray.push_back(data);
        }

        // In a real implementation, the light data would be uploaded to GPU buffers here
        // For example:
        // glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_lightDataBuffer);
        // glBufferData(GL_SHADER_STORAGE_BUFFER, lightDataArray.size() * sizeof(LightData), lightDataArray.data(), GL_DYNAMIC_DRAW);

        // Similarly for cluster data
        // glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_clusterDataBuffer);
        // ...
    }

    uint32_t LightManager::getLightDataBuffer() const
    {
        return m_lightDataBuffer;
    }

    uint32_t LightManager::getClusterDataBuffer() const
    {
        return m_clusterDataBuffer;
    }

    void LightManager::assignLightsToClusters()
    {
        // Grid dimensions
        const glm::ivec3& dims = m_clusterData.dimensions;
        const glm::vec3& minBounds = m_clusterData.minBounds;
        const glm::vec3& maxBounds = m_clusterData.maxBounds;
        const glm::vec3 boundsSize = maxBounds - minBounds;

        // Cluster size
        const glm::vec3 clusterSize = boundsSize / glm::vec3(dims);

        // Reserve space for light indices (worst case: all lights affect all clusters)
        m_clusterData.lightIndices.clear();

        // For each cluster, find lights that affect it
        const int totalClusters = dims.x * dims.y * dims.z;

        for (int clusterIdx = 0; clusterIdx < totalClusters; ++clusterIdx)
        {
            // Calculate 3D indices from flat index
            int z = clusterIdx / (dims.x * dims.y);
            int y = (clusterIdx - z * dims.x * dims.y) / dims.x;
            int x = clusterIdx - z * dims.x * dims.y - y * dims.x;

            // Calculate cluster bounds
            glm::vec3 clusterMin = minBounds + glm::vec3(x, y, z) * clusterSize;
            glm::vec3 clusterMax = clusterMin + clusterSize;

            // Center of the cluster
            glm::vec3 clusterCenter = (clusterMin + clusterMax) * 0.5f;

            // Record starting index for this cluster's lights
            uint32_t startIndex = static_cast<uint32_t>(m_clusterData.lightIndices.size());
            uint32_t lightCount = 0;

            // Add directional lights (which affect all clusters)
            for (auto& light : m_directionalLights)
            {
                if (std::find(m_visibleLights.begin(), m_visibleLights.end(), light) != m_visibleLights.end())
                {
                    m_clusterData.lightIndices.push_back(
                        static_cast<uint32_t>(std::distance(m_visibleLights.begin(),
                                                            std::find(m_visibleLights.begin(), m_visibleLights.end(), light))));
                    lightCount++;
                }
            }

            // Check point lights
            for (auto& light : m_pointLights)
            {
                if (std::find(m_visibleLights.begin(), m_visibleLights.end(), light) == m_visibleLights.end())
                {
                    continue;
                }

                float radius = light->getRange();
                float radiusSq = radius * radius;

                // Simplified test: distance from point to AABB
                float distSq = 0.0f;

                for (int i = 0; i < 3; i++)
                {
                    float v = light->getPosition()[i];
                    if (v < clusterMin[i]) distSq += (clusterMin[i] - v) * (clusterMin[i] - v);
                    if (v > clusterMax[i]) distSq += (v - clusterMax[i]) * (v - clusterMax[i]);
                }

                if (distSq <= radiusSq)
                {
                    m_clusterData.lightIndices.push_back(
                        static_cast<uint32_t>(std::distance(m_visibleLights.begin(),
                                                            std::find(m_visibleLights.begin(), m_visibleLights.end(), light))));
                    lightCount++;
                }
            }

            // Check spot lights (similar to point lights but also check direction)
            for (auto& light : m_spotLights)
            {
                if (std::find(m_visibleLights.begin(), m_visibleLights.end(), light) == m_visibleLights.end())
                {
                    continue;
                }

                float radius = light->getRange();
                float radiusSq = radius * radius;

                // First, quick sphere test
                float distSq = 0.0f;
                for (int i = 0; i < 3; i++)
                {
                    float v = light->getPosition()[i];
                    if (v < clusterMin[i]) distSq += (clusterMin[i] - v) * (clusterMin[i] - v);
                    if (v > clusterMax[i]) distSq += (v - clusterMax[i]) * (v - clusterMax[i]);
                }

                if (distSq <= radiusSq)
                {
                    // Check if cluster center is within the spotlight cone
                    glm::vec3 dirToCluster = glm::normalize(clusterCenter - light->getPosition());
                    float cosAngle = glm::dot(light->getDirection(), dirToCluster);

                    if (cosAngle > glm::cos(light->getType() == LightType::Spot ? glm::radians(45.0f) : glm::pi<float>()))
                    {
                        m_clusterData.lightIndices.push_back(
                            static_cast<uint32_t>(std::distance(m_visibleLights.begin(),
                                                                std::find(m_visibleLights.begin(), m_visibleLights.end(), light))));
                        lightCount++;
                    }
                }
            }

            // Check area lights (simplified)
            for (auto& light : m_areaLights)
            {
                if (std::find(m_visibleLights.begin(), m_visibleLights.end(), light) == m_visibleLights.end())
                {
                    continue;
                }

                // Simplified as sphere test
                float extent = glm::max(light->getAreaSize().x, light->getAreaSize().y) * 0.5f;
                float maxDist = extent + glm::length(clusterSize) * 0.5f;

                if (glm::distance(light->getPosition(), clusterCenter) <= maxDist)
                {
                    m_clusterData.lightIndices.push_back(
                        static_cast<uint32_t>(std::distance(m_visibleLights.begin(),
                                                            std::find(m_visibleLights.begin(), m_visibleLights.end(), light))));
                    lightCount++;
                }
            }

            // Update grid entry with start index and count
            m_clusterData.lightGrid[clusterIdx] = (startIndex & 0xFFFFFF) | (lightCount << 24);
        }
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

        // Create a shadow map based on light type
        const int resolution = light->getType() == LightType::Directional ? 2048 : 1024;
        const bool cubemap = light->getType() == LightType::Point;

        // In a real implementation, this would create the appropriate render target
        // with depth textures for shadow mapping
        std::shared_ptr<RenderTarget> shadowMap = std::make_shared<RenderTarget>();
        // shadowMap->initialize(resolution, resolution, TextureFormat::DEPTH_32F, cubemap);

        // Add shadow map to the list and record its index
        size_t index = m_shadowMaps.size();
        m_shadowMaps.push_back(shadowMap);
        m_lightShadowIndices[light] = index;
    }

    void LightManager::calculateCascadeSplits(const RenderContext& context)
    {
        if (m_cascadeCount <= 0 || m_directionalLights.empty())
        {
            return;
        }

        // Get near and far planes from the camera
        const float nearClip = context.getNearClip();
        const float farClip = context.getFarClip();
        const float clipRange = farClip - nearClip;
        const float minZ = nearClip;
        const float maxZ = nearClip + clipRange;

        // Calculate split depths using logarithmic distribution
        for (int i = 0; i <= m_cascadeCount; ++i)
        {
            float p = static_cast<float>(i) / static_cast<float>(m_cascadeCount);
            float log = minZ * std::pow(maxZ / minZ, p);
            float uniform = minZ + clipRange * p;
            float d = m_cascadeSplitLambda * log + (1.0f - m_cascadeSplitLambda) * uniform;
            m_cascadeSplits[i] = d;
        }

        // Get the main directional light
        auto& mainLight = m_directionalLights[0];

        // Get the light view matrix
        glm::mat4 lightViewMatrix = mainLight->getViewMatrix();

        // For each cascade, calculate the view-projection matrix
        for (int i = 0; i < m_cascadeCount; ++i)
        {
            float nearSplit = m_cascadeSplits[i];
            float farSplit = m_cascadeSplits[i + 1];

            // Calculate bounding sphere for this cascade
            glm::vec3 center = glm::vec3(0.0f);
            float radius = 0.0f;

            // In a real implementation, this would compute the bounding sphere
            // for the view frustum between nearSplit and farSplit

            // Radius of the sphere is the distance from the center to the furthest corner
            radius = 10.0f + i * 15.0f; // Simplified for demonstration

            // Create an orthographic projection for this cascade
            glm::mat4 lightProjMatrix = glm::ortho(
                -radius, radius,
                -radius, radius,
                -radius, radius);

            // Store the cascade view-projection matrix
            m_cascadeViewProjections[i] = lightProjMatrix * lightViewMatrix;
        }
    }

} // namespace PixelCraft::Rendering