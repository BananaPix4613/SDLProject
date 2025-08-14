// -------------------------------------------------------------------------
// ClusterUpdateStage.h
// -------------------------------------------------------------------------
#pragma once

#include "Rendering/RenderStage.h"
#include "Rendering/ClusteredLightCuller.h"
#include "glm/glm.hpp"

#include <memory>

namespace PixelCraft::Rendering
{
    class LightManager;
    class ForwardPlusShaderManager;
    struct ClusterConfig;
}

namespace PixelCraft::ECS
{
    class Registry;
}

namespace PixelCraft::Rendering::Stages
{

    /**
     * @brief Render stage for updating clustered light culling data
     *
     * The ClusterUpdateStage prepares clustered lighting data for forward+ rendering
     * by updating light assignments to clusters based on camera changes and uploading
     * cluster buffer data to GPU for use by subsequent rendering stages. This stage
     * integrates with ForwardPlusShaderManager for coordinated cluster management.
     */
    class ClusterUpdateStage : public RenderStage
    {
    public:
        /**
         * @brief Constructor
         */
        ClusterUpdateStage();

        /**
         * @brief Destructor
         */
        virtual ~ClusterUpdateStage();

        /**
         * @brief Initialize the cluster update stage
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Clean up resources
         */
        void shutdown() override;

        /**
         * @brief Execute the cluster update pass
         * @param context Current render context
         */
        void execute(const std::shared_ptr<RenderContext> context) override;

        /**
         * @brief Get the stage type name
         * @return Type name string
         */
        std::string getTypeName() const override
        {
            return "ClusterUpdateStage";
        }

        /**
         * @brief Set the light manager for cluster calculations
         * @param lightManager Light manager instance
         */
        void setLightManager(std::shared_ptr<LightManager> lightManager);

        /**
         * @brief Set the ForwardPlusShaderManager for coordinated cluster management
         * @param shaderManager Forward+ shader manager instance
         */
        void setForwardPlusShaderManager(std::shared_ptr<ForwardPlusShaderManager> shaderManager);

        /**
         * @brief Get the ForwardPlusShaderManager
         * @return Forward+ shader manager instance
         */
        std::shared_ptr<ForwardPlusShaderManager> getForwardPlusShaderManager() const;

        /**
         * @brief Set the cluster dimensions for clustered lighting
         * @param dimensions Cluster grid dimensions (x, y, z)
         */
        void setClusterDimensions(const glm::ivec3& dimensions);

        /**
         * @brief Get the current cluster dimensions
         * @return Cluster grid dimensions
         */
        const glm::ivec3& getClusterDimensions() const;

        /**
         * @brief Configure the cluster culler with advanced settings
         * @param config Cluster configuration parameters
         */
        void configureClusterCuller(const ClusterConfig& config);

        /**
         * @brief Get performance metrics from the last cluster update
         * @return Time taken for clustering in milliseconds
         */
        float getLastClusterUpdateTime() const;

        /**
         * @brief Get the total number of light-cluster assignments from last update
         * @return Total light assignments
         */
        uint32_t getTotalLightAssignments() const;

    private:
        /**
         * @brief Update cluster data for the current frame
         * @param context Render context
         */
        void updateClusterData(const std::shared_ptr<RenderContext> context);

        /**
         * @brief Validate that clustering is properly configured
         * @return True if clustering is ready
         */
        bool validateClusterConfiguration() const;

        /**
         * @brief Log cluster performance metrics
         */
        void logPerformanceMetrics() const;

        /**
         * @brief Synchronize configuration with ForwardPlusShaderManager
         */
        void synchronizeShaderManagerConfig();

        /** Light manager for cluster operations */
        std::shared_ptr<LightManager> m_lightManager;

        /** Forward+ shader manager for coordinated cluster management */
        std::shared_ptr<ForwardPlusShaderManager> m_forwardPlusShaderManager;

        /** Cluster dimensions for clustered lighting */
        glm::ivec3 m_clusterDimensions;

        /** Whether cluster configuration has changed */
        bool m_clusterConfigDirty;

        /** Frame counter for performance tracking */
        uint32_t m_frameCounter;
    };

} // namespace PixelCraft::Rendering::Stages
