// -------------------------------------------------------------------------
// ClusteredLightCuller.h
// -------------------------------------------------------------------------
#pragma once

#include "Core/Logger.h"
#include "Rendering/Light.h"
#include "Rendering/RenderContext.h"
#include "Utility/Frustum.h"

#include <memory>
#include <vector>
#include <glm/glm.hpp>

namespace PixelCraft::Rendering
{
    /**
     * @brief Configuration for clustered light culling optimized for isometric voxel games
     */
    struct ClusterConfig
    {
        // Screen-space clusters (adjust based on target resolution)
        int clusterCountX = 32;  ///< Horizontal clusters
        int clusterCountY = 18;  ///< Vertical clusters (16:9 aspect)
        int clusterCountZ = 24;  ///< Depth clusters (more than typical due to orthographic)
        
        // Depth distribution
        float nearBias = 0.3f;   ///< Subtle bias toward camera
        float verticalStretch = 1.5f; ///< Account for isometric angle
        
        // Special handling
        bool enableAdaptiveDensity = true;   ///< Dynamic light assignment based on cluster density 
        bool enableVerticalParallax = true;  ///< Special handling for isometric depth perception
        
        // Performance limits
        int maxLightsPerCluster = 128;  ///< Maximum lights that can affect a single cluster
        float maxClusteringTime = 16.0f; ///< Maximum time in milliseconds for clustering per frame
    };

    /**
     * @brief Data structure for clustered lighting optimized for GPU consumption
     */
    struct ClusterData
    {
        glm::ivec3 dimensions;              ///< 3D dimensions of the cluster grid (x, y, z)
        glm::vec3 minBounds;                ///< Minimum view-space bounds of the cluster volume
        glm::vec3 maxBounds;                ///< Maximum view-space bounds of the cluster volume
        glm::vec3 clusterSize;              ///< Size of each cluster in view space
        std::vector<uint32_t> lightGrid;    ///< Grid data: start index and count packed into 32 bits
        std::vector<uint32_t> lightIndices; ///< Indices into the visible lights array
        uint32_t totalLightAssignments;     ///< Total number of light-cluster assignments
        float lastUpdateTime;               ///< Time taken for last clustering update in milliseconds
    };

    /**
     * @brief Clustered light culling system optimized for forward+ rendering with orthographic support
     * 
     * The ClusteredLightCuller manages 3D light clusters for efficient light culling in forward+ rendering.
     * It's specifically optimized for isometric voxel games with orthographic projection support,
     * featuring adaptive density management and vertical parallax handling.
     */
    class ClusteredLightCuller
    {
    public:
        /**
         * @brief Constructor with default configuration
         */
        ClusteredLightCuller();

        /**
         * @brief Constructor with custom configuration
         * @param config The cluster configuration to use
         */
        explicit ClusteredLightCuller(const ClusterConfig& config);

        /**
         * @brief Destructor
         */
        ~ClusteredLightCuller();

        /**
         * @brief Initialize the clustered light culler
         * @return True if initialization was successful
         */
        bool initialize();

        /**
         * @brief Shut down the culler and release resources
         */
        void shutdown();

        /**
         * @brief Configure the cluster grid settings
         * @param config The new cluster configuration
         */
        void configureGrid(const ClusterConfig& config);

        /**
         * @brief Update the light clusters based on current render context and visible lights
         * @param context The current render context
         * @param visibleLights The lights visible in the current view frustum
         */
        void update(const RenderContext& context, const std::vector<std::shared_ptr<Light>>& visibleLights);

        /**
         * @brief Get the current cluster data for GPU upload
         * @return Const reference to the cluster data structure
         */
        const ClusterData& getClusterData() const;

        /**
         * @brief Get the current cluster configuration
         * @return Const reference to the cluster configuration
         */
        const ClusterConfig& getConfig() const;

        /**
         * @brief Check if the culler has been properly initialized
         * @return True if initialized
         */
        bool isInitialized() const;

        /**
         * @brief Get performance statistics for the last clustering update
         * @return Time taken for last update in milliseconds
         */
        float getLastUpdateTime() const;

        /**
         * @brief Get the total number of light-cluster assignments
         * @return Total assignments made in the last update
         */
        uint32_t getTotalLightAssignments() const;

    private:
        /**
         * @brief Calculate optimal cluster dimensions based on scene complexity
         * @param lightCount Number of visible lights
         * @param frustum Current view frustum
         * @return Optimal cluster dimensions
         */
        glm::ivec3 calculateOptimalDimensions(int lightCount, const Utility::Frustum& frustum);

        /**
         * @brief Calculate cluster bounds in view space
         * @param context Current render context
         */
        void calculateClusterBounds(const RenderContext& context);

        /**
         * @brief Calculate depth slice position for a given cluster Z index
         * @param slice The Z slice index
         * @param totalSlices Total number of Z slices
         * @param nearPlane Near clipping plane distance
         * @param farPlane Far clipping plane distance
         * @return Depth position for the slice
         */
        float calculateDepthSlice(int slice, int totalSlices, float nearPlane, float farPlane) const;

        /**
         * @brief Assign lights to clusters in view space
         * @param context Current render context
         * @param visibleLights The lights to assign to clusters
         */
        void assignLightsToViewSpaceClusters(const RenderContext& context, 
                                             const std::vector<std::shared_ptr<Light>>& visibleLights);

        /**
         * @brief Test if a light affects a specific cluster
         * @param light The light to test
         * @param clusterIndex 3D index of the cluster
         * @param viewMatrix Current view matrix for transformations
         * @return True if the light affects the cluster
         */
        bool testLightClusterIntersection(const std::shared_ptr<Light>& light, 
                                          const glm::ivec3& clusterIndex,
                                          const glm::mat4& viewMatrix) const;

        /**
         * @brief Apply adaptive density optimization
         * @param visibleLights The lights being processed
         */
        void applyAdaptiveDensity(const std::vector<std::shared_ptr<Light>>& visibleLights);

        /**
         * @brief Apply vertical parallax compensation for isometric projection
         * @param context Current render context
         */
        void applyVerticalParallax(const RenderContext& context);

        /**
         * @brief Generate GPU-ready cluster data from internal structures
         */
        void generateGPUData();

        /**
         * @brief Clear all cluster assignments
         */
        void clearClusterAssignments();

        /**
         * @brief Get the flat array index for a 3D cluster coordinate
         * @param x X cluster index
         * @param y Y cluster index  
         * @param z Z cluster index
         * @return Flat array index
         */
        inline int getClusterIndex(int x, int y, int z) const
        {
            return z * (m_config.clusterCountX * m_config.clusterCountY) + 
                   y * m_config.clusterCountX + x;
        }

        /**
         * @brief Get 3D cluster coordinate from flat array index
         * @param index Flat array index
         * @return 3D cluster coordinate
         */
        inline glm::ivec3 getClusterCoordinate(int index) const
        {
            int z = index / (m_config.clusterCountX * m_config.clusterCountY);
            int remainder = index % (m_config.clusterCountX * m_config.clusterCountY);
            int y = remainder / m_config.clusterCountX;
            int x = remainder % m_config.clusterCountX;
            return glm::ivec3(x, y, z);
        }

        // Configuration and state
        ClusterConfig m_config;
        ClusterData m_clusterData;
        bool m_initialized;
        bool m_configDirty;

        // Performance tracking
        float m_lastUpdateStartTime;
        uint32_t m_frameCounter;

        // Temporary data structures for clustering
        std::vector<std::vector<uint32_t>> m_tempClusterLightLists;
    };

} // namespace PixelCraft::Rendering
