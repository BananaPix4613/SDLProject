// -------------------------------------------------------------------------
// ClusteredLightCuller.cpp
// -------------------------------------------------------------------------
#include "Rendering/ClusteredLightCuller.h"
#include "Core/Logger.h"
#include "Utility/Math.h"

#include <algorithm>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

namespace PixelCraft::Rendering
{

    ClusteredLightCuller::ClusteredLightCuller()
        : m_initialized(false)
        , m_configDirty(true)
        , m_lastUpdateStartTime(0.0f)
        , m_frameCounter(0)
    {
        // Initialize with default configuration
        m_config = ClusterConfig{};
    }

    ClusteredLightCuller::ClusteredLightCuller(const ClusterConfig &config)
        : m_config(config)
        , m_initialized(false)
        , m_configDirty(true)
        , m_lastUpdateStartTime(0.0f)
        , m_frameCounter(0)
    {
    }

    ClusteredLightCuller::~ClusteredLightCuller()
    {
        if (m_initialized)
        {
            shutdown();
        }
    }

    bool ClusteredLightCuller::initialize()
    {
        if (m_initialized)
        {
            Core::warn("ClusteredLightCuller already initialized");
            return true;
        }

        Core::info("Initializing ClusteredLightCuller with dimensions " + std::to_string(m_config.clusterCountX) + "x" +
                std::to_string(m_config.clusterCountY) + "x" + std::to_string(m_config.clusterCountZ));

        // Initialize cluster data with configured dimensions
        m_clusterData.dimensions = glm::ivec3(m_config.clusterCountX, m_config.clusterCountY, m_config.clusterCountZ);
        m_clusterData.minBounds = glm::vec3(-1.0f);
        m_clusterData.maxBounds = glm::vec3(1.0f);
        m_clusterData.clusterSize = glm::vec3(0.0f);
        m_clusterData.totalLightAssignments = 0;
        m_clusterData.lastUpdateTime = 0.0f;

        // Allocate space for the light grid
        const int totalClusters = m_config.clusterCountX * m_config.clusterCountY * m_config.clusterCountZ;
        m_clusterData.lightGrid.resize(totalClusters, 0);
        m_clusterData.lightIndices.reserve(totalClusters * 4); // Initial estimate

        // Initialize temporary cluster light lists
        m_tempClusterLightLists.resize(totalClusters);

        m_initialized = true;
        m_configDirty = false;

        Core::info("ClusteredLightCuller initialized successfully with " + std::to_string(totalClusters) +
                " total clusters");
        return true;
    }

    void ClusteredLightCuller::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        Core::info("Shutting down ClusteredLightCuller");

        // Clear all data structures
        m_clusterData.lightGrid.clear();
        m_clusterData.lightIndices.clear();
        m_tempClusterLightLists.clear();

        m_initialized = false;
    }

    void ClusteredLightCuller::configureGrid(const ClusterConfig &config)
    {
        m_config = config;
        m_configDirty = true;

        if (m_initialized)
        {
            Core::info("Reconfiguring ClusteredLightCuller to " + std::to_string(m_config.clusterCountX) + "x" +
                    std::to_string(m_config.clusterCountY) + "x" + std::to_string(m_config.clusterCountZ));

            // Resize data structures
            m_clusterData.dimensions = glm::ivec3(config.clusterCountX, config.clusterCountY, config.clusterCountZ);
            const int totalClusters = config.clusterCountX * config.clusterCountY * config.clusterCountZ;

            m_clusterData.lightGrid.resize(totalClusters, 0);
            m_tempClusterLightLists.resize(totalClusters);

            m_configDirty = false;
        }
    }

    void ClusteredLightCuller::update(const RenderContext &context,
                                    const std::vector<std::shared_ptr<Light>> &visibleLights)
    {
        if (!m_initialized)
        {
            Core::error("ClusteredLightCuller::update called before initialization");
            return;
        }

        // Start performance timer
        auto startTime = std::chrono::high_resolution_clock::now();

        // Reconfigure if needed
        if (m_configDirty)
        {
            initialize();
        }

        // Clear previous cluster assignments
        clearClusterAssignments();

        // Calculate cluster bounds in view space
        calculateClusterBounds(context);

        // Apply adaptive density if enabled
        if (m_config.enableAdaptiveDensity)
        {
            applyAdaptiveDensity(visibleLights);
        }

        // Apply vertical parallax compensation if enabled
        if (m_config.enableVerticalParallax)
        {
            applyVerticalParallax(context);
        }

        // Assign lights to view-space clusters
        assignLightsToViewSpaceClusters(context, visibleLights);

        // Generate GPU-ready data
        generateGPUData();

        // Calculate update time
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        m_clusterData.lastUpdateTime = duration.count() / 1000.0f; // Convert to milliseconds

        m_frameCounter++;

        // Log performance warnings if clustering takes too long
        if (m_clusterData.lastUpdateTime > m_config.maxClusteringTime)
        {
            Core::warn("ClusteredLightCuller update took " + std::to_string(m_clusterData.lastUpdateTime) +
                    "ms (limit: " + std::to_string(m_config.maxClusteringTime) + "ms) with " +
                    std::to_string(visibleLights.size()) + " lights");
        }
    }

    const ClusterData &ClusteredLightCuller::getClusterData() const
    {
        return m_clusterData;
    }

    const ClusterConfig &ClusteredLightCuller::getConfig() const
    {
        return m_config;
    }

    bool ClusteredLightCuller::isInitialized() const
    {
        return m_initialized;
    }

    float ClusteredLightCuller::getLastUpdateTime() const
    {
        return m_clusterData.lastUpdateTime;
    }

    uint32_t ClusteredLightCuller::getTotalLightAssignments() const
    {
        return m_clusterData.totalLightAssignments;
    }

    glm::ivec3 ClusteredLightCuller::calculateOptimalDimensions(int lightCount, const Utility::Frustum& frustum)
    {
        (void)frustum;  // Currently unused

        // For now, use the configured dimensions
        // In a more advanced implementation, this could adapt based on light density and frustum size
        glm::ivec3 optimal = glm::ivec3(m_config.clusterCountX, m_config.clusterCountY, m_config.clusterCountZ);

        if (m_config.enableAdaptiveDensity && lightCount > 0)
        {
            // Simple adaptive scaling based on light count
            float scaleFactor = glm::clamp(static_cast<float>(lightCount) / 50.0f, 0.5f, 2.0f);

            optimal.x = static_cast<int>(m_config.clusterCountX * scaleFactor);
            optimal.y = static_cast<int>(m_config.clusterCountY * scaleFactor);
            // Keep Z dimension stable for orthographic projection
            optimal.z = m_config.clusterCountZ;

            // Ensure minimum dimensions
            optimal = glm::max(optimal, glm::ivec3(8, 4, 12));
            // Ensure maximum dimensions for performance
            optimal = glm::min(optimal, glm::ivec3(64, 36, 48));
        }

        return optimal;
    }

    void ClusteredLightCuller::calculateClusterBounds(const RenderContext &context)
    {
        // Get the view frustum corners in world space
        const auto &frustum = context.getFrustum();
        const auto &corners = frustum.getCorners();

        // Transform frustum corners to view space
        const glm::mat4 &viewMatrix = context.getViewMatrix();

        glm::vec3 minBounds = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 maxBounds = glm::vec3(std::numeric_limits<float>::lowest());

        for (const auto &corner : corners)
        {
            glm::vec4 viewSpaceCorner = viewMatrix * glm::vec4(corner, 1.0f);
            glm::vec3 viewPos = glm::vec3(viewSpaceCorner) / viewSpaceCorner.w;

            minBounds = glm::min(minBounds, viewPos);
            maxBounds = glm::max(maxBounds, viewPos);
        }

        // Apply vertical stretch for isometric compensation
        if (m_config.enableVerticalParallax)
        {
            float yCenter = (minBounds.y + maxBounds.y) * 0.5f;
            float yExtent = (maxBounds.y - minBounds.y) * 0.5f * m_config.verticalStretch;
            minBounds.y = yCenter - yExtent;
            maxBounds.y = yCenter + yExtent;
        }

        m_clusterData.minBounds = minBounds;
        m_clusterData.maxBounds = maxBounds;
        m_clusterData.clusterSize = (maxBounds - minBounds) / glm::vec3(m_clusterData.dimensions);
    }

    float ClusteredLightCuller::calculateDepthSlice(int slice, int totalSlices, float nearPlane, float farPlane) const
    {
        if (totalSlices <= 1)
        {
            return nearPlane;
        }

        // Linear distribution with near bias for orthographic projection
        float linearT = static_cast<float>(slice) / static_cast<float>(totalSlices - 1);

        // Apply near bias using power function
        float biasedT = glm::pow(linearT, 1.0f + m_config.nearBias);

        // Mix between near and far plane
        return glm::mix(nearPlane, farPlane, biasedT);
    }

    void ClusteredLightCuller::assignLightsToViewSpaceClusters(const RenderContext &context,
                                                               const std::vector<std::shared_ptr<Light>> &visibleLights)
    {
        const glm::mat4 &viewMatrix = context.getViewMatrix();
        const glm::ivec3 &dims = m_clusterData.dimensions;
        const glm::vec3 &minBounds = m_clusterData.minBounds;
        const glm::vec3 &clusterSize = m_clusterData.clusterSize;

        uint32_t totalAssignments = 0;

        // For each visible light
        for (size_t lightIdx = 0; lightIdx < visibleLights.size(); ++lightIdx)
        {
            const auto &light = visibleLights[lightIdx];

            // Transform light position to view space
            glm::vec4 viewSpacePos = viewMatrix * glm::vec4(light->getPosition(), 1.0f);
            glm::vec3 lightViewPos = glm::vec3(viewSpacePos) / viewSpacePos.w;

            // Handle different light types
            switch (light->getType())
            {
            case LightType::Directional:
                {
                    // Directional lights affect all clusters
                    for (int z = 0; z < dims.z; ++z)
                    {
                        for (int y = 0; y < dims.y; ++y) 
                        {
                            for (int x = 0; x < dims.x; ++x)
                            {
                                int clusterIdx = getClusterIndex(x, y, z);
                                m_tempClusterLightLists[clusterIdx].push_back(static_cast<uint32_t>(lightIdx));
                                totalAssignments++;
                            }
                        }
                    }
                    break;
                }

            case LightType::Point:
                {
                    float lightRange = light->getRange();

                    // Calculate which clusters this point light can affect
                    glm::vec3 lightMin = lightViewPos - glm::vec3(lightRange);
                    glm::vec3 lightMax = lightViewPos + glm::vec3(lightRange);

                    // Convert to cluster coordinates
                    glm::ivec3 minCluster = glm::max(glm::ivec3(0), glm::ivec3((lightMin - minBounds) / clusterSize));
                    glm::ivec3 maxCluster = glm::min(dims - 1, glm::ivec3((lightMax - minBounds) / clusterSize));

                    for (int z = minCluster.z; z <= maxCluster.z; ++z)
                    {
                        for (int y = minCluster.y; y <= maxCluster.y; ++y)
                        {
                            for (int x = minCluster.x; x <= maxCluster.x; ++x)
                            {
                                // Calculate cluster center
                                glm::vec3 clusterCenter = minBounds + (glm::vec3(x, y, z) + 0.5f) * clusterSize;

                                // Test if light affects this cluster
                                float distance = glm::distance(lightViewPos, clusterCenter);
                                float clusterRadius = glm::length(clusterSize) * 0.5f;

                                if (distance <= lightRange + clusterRadius)
                                {
                                    int clusterIdx = getClusterIndex(x, y, z);
                                    m_tempClusterLightLists[clusterIdx].push_back(static_cast<uint32_t>(lightIdx));
                                    totalAssignments++;
                                }
                            }
                        }
                    }
                    break;
                }

            case LightType::Spot:
                {
                    float lightRange = light->getRange();

                    // Transform light direction to view space
                    glm::vec4 viewSpaceDir = viewMatrix * glm::vec4(light->getDirection(), 0.0f);
                    glm::vec3 lightViewDir = glm::normalize(glm::vec3(viewSpaceDir));

                    // Calculate potential affected area (simplified as sphere first)
                    glm::vec3 lightMin = lightViewPos - glm::vec3(lightRange);
                    glm::vec3 lightMax = lightViewPos + glm::vec3(lightRange);

                    glm::ivec3 minCluster = glm::max(glm::ivec3(0), glm::ivec3((lightMin - minBounds) / clusterSize));
                    glm::ivec3 maxCluster = glm::min(dims - 1, glm::ivec3((lightMax - minBounds) / clusterSize));

                    for (int z = minCluster.z; z <= maxCluster.z; ++z)
                    {
                        for (int y = minCluster.y; y <= maxCluster.y; ++y)
                        {
                            for (int x = minCluster.x; x <= maxCluster.x; ++x)
                            {
                                glm::vec3 clusterCenter = minBounds + (glm::vec3(x, y, z) + 0.5f) * clusterSize;

                                // Distance test
                                float distance = glm::distance(lightViewPos, clusterCenter);
                                float clusterRadius = glm::length(clusterSize) * 0.5f;

                                if (distance <= lightRange + clusterRadius)
                                {
                                    // Cone test (simplified)
                                    glm::vec3 dirToCluster = glm::normalize(clusterCenter - lightViewPos);
                                    float cosAngle = glm::dot(lightViewDir, dirToCluster);

                                    // Use a generous cone angle for spot lights
                                    if (cosAngle > 0.5f) // Approximately 60 degree cone
                                    {
                                        int clusterIdx = getClusterIndex(x, y, z);
                                        m_tempClusterLightLists[clusterIdx].push_back(static_cast<uint32_t>(lightIdx));
                                        totalAssignments++;
                                    }
                                }
                            }
                        }
                    }
                    break;
                }

            case LightType::Area:
                {
                    // Simplified area light handling - treat as point light with extended range
                    glm::vec2 areaSize = light->getAreaSize();
                    float effectiveRange = glm::max(areaSize.x, areaSize.y) * 0.5f + light->getRange();

                    glm::vec3 lightMin = lightViewPos - glm::vec3(effectiveRange);
                    glm::vec3 lightMax = lightViewPos + glm::vec3(effectiveRange);

                    glm::ivec3 minCluster = glm::max(glm::ivec3(0), glm::ivec3((lightMin - minBounds) / clusterSize));
                    glm::ivec3 maxCluster = glm::min(dims - 1, glm::ivec3((lightMax - minBounds) / clusterSize));

                    for (int z = minCluster.z; z <= maxCluster.z; ++z)
                    {
                        for (int y = minCluster.y; y <= maxCluster.y; ++y)
                        {
                            for (int x = minCluster.x; x <= maxCluster.x; ++x)
                            {
                                glm::vec3 clusterCenter = minBounds + (glm::vec3(x, y, z) + 0.5f) * clusterSize;
                                float distance = glm::distance(lightViewPos, clusterCenter);
                                float clusterRadius = glm::length(clusterSize) * 0.5f;

                                if (distance <= effectiveRange + clusterRadius)
                                {
                                    int clusterIdx = getClusterIndex(x, y, z);
                                    m_tempClusterLightLists[clusterIdx].push_back(static_cast<uint32_t>(lightIdx));
                                    totalAssignments++;
                                }
                            }
                        }
                    }
                    break;
                }
            }
        }

        m_clusterData.totalLightAssignments = totalAssignments;
    }

    bool ClusteredLightCuller::testLightClusterIntersection(const std::shared_ptr<Light>& light,
                                                            const glm::ivec3& clusterIndex,
                                                            const glm::mat4& viewMatrix) const
    {
        // This method provides more detailed intersection testing if needed
        // For now, the main assignment logic is in assignLightsToViewSpaceClusters
        (void)light;  // Currently unused
        (void)clusterIndex;  // Currently unused
        (void)viewMatrix;  // Currently unused
        return true;
    }

    void ClusteredLightCuller::applyAdaptiveDensity(const std::vector<std::shared_ptr<Light>> &visibleLights)
    {
        // Adjust cluster dimensions based on light density
        if (visibleLights.size() > 100)
        {
            // High light density - might need more clusters
            // This is a placeholder for more sophisticated adaptive logic
        }
        else if (visibleLights.size() < 10)
        {
            // Low light density - can use fewer clusters
            // This is a placeholder for more sophisticated adaptive logic
        }
    }

    void ClusteredLightCuller::applyVerticalParallax(const RenderContext &context)
    {
        // Apply isometric-specific adjustments
        // The main adjustment is already applied in calculateClusterBounds()
        // This method can be used for additional per-frame adjustments if needed
        (void)context;  // Currently unused
    }

    void ClusteredLightCuller::generateGPUData()
    {
        // Clear existing GPU data
        m_clusterData.lightIndices.clear();

        const int totalClusters = m_clusterData.dimensions.x * m_clusterData.dimensions.y * m_clusterData.dimensions.z;

        // Build the light grid and indices
        for (int clusterIdx = 0; clusterIdx < totalClusters; ++clusterIdx)
        {
            const auto &lightList = m_tempClusterLightLists[clusterIdx];

            // Limit lights per cluster for performance
            uint32_t lightCount =
                glm::min(static_cast<uint32_t>(lightList.size()), static_cast<uint32_t>(m_config.maxLightsPerCluster));

            uint32_t startIndex = static_cast<uint32_t>(m_clusterData.lightIndices.size());

            // Add light indices to the global list
            for (uint32_t i = 0; i < lightCount; ++i)
            {
                m_clusterData.lightIndices.push_back(lightList[i]);
            }

            // Pack start index and count into grid entry
            // Lower 24 bits for start index, upper 8 bits for count
            m_clusterData.lightGrid[clusterIdx] = (startIndex & 0x00FFFFFF) | ((lightCount & 0xFF) << 24);
        }
    }

    void ClusteredLightCuller::clearClusterAssignments()
    {
        // Clear temporary cluster light lists
        for (auto &clusterLights : m_tempClusterLightLists)
        {
            clusterLights.clear();
        }

        // Reset cluster data
        std::fill(m_clusterData.lightGrid.begin(), m_clusterData.lightGrid.end(), 0);
        m_clusterData.lightIndices.clear();
        m_clusterData.totalLightAssignments = 0;
    }

} // namespace PixelCraft::Rendering