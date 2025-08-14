// -------------------------------------------------------------------------
// ClusterUpdateStage.cpp
// -------------------------------------------------------------------------
#include "Rendering/Stages/ClusterUpdateStage.h"
#include "Core/Logger.h"
#include "Rendering/RenderContext.h"
#include "Rendering/LightManager.h"
#include "Rendering/ForwardPlusShaderManager.h"
#include "Rendering/RenderSystem.h"
#include "Utility/Profiler.h"

namespace PixelCraft::Rendering::Stages
{

    ClusterUpdateStage::ClusterUpdateStage()
        : RenderStage("ClusterUpdateStage")
        , m_lightManager(nullptr)
        , m_forwardPlusShaderManager(nullptr)
        , m_clusterDimensions(32, 18, 24)  // Default optimized for isometric voxel games
        , m_clusterConfigDirty(true)
        , m_frameCounter(0)
    {
    }

    ClusterUpdateStage::~ClusterUpdateStage()
    {
        shutdown();
    }

    bool ClusterUpdateStage::initialize()
    {
        if (isInitialized())
        {
            Core::warn("ClusterUpdateStage already initialized");
            return true;
        }

        Core::info("Initializing ClusterUpdateStage for forward+ clustering with ForwardPlusShaderManager integration");
        
        // Reset frame counter
        m_frameCounter = 0;
        
        // Validate dependencies
        if (!m_lightManager)
        {
            Core::warn("ClusterUpdateStage: No LightManager set during initialization");
        }
        
        if (!m_forwardPlusShaderManager)
        {
            Core::warn("ClusterUpdateStage: No ForwardPlusShaderManager set during initialization");
        }
        
        setInitialized(true);
        return true;
    }

    void ClusterUpdateStage::shutdown()
    {
        if (!isInitialized())
        {
            return;
        }

        Core::info("Shutting down ClusterUpdateStage");
        m_lightManager.reset();
        m_forwardPlusShaderManager.reset();
        setInitialized(false);
    }

    void ClusterUpdateStage::execute(const std::shared_ptr<RenderContext> context)
    {
        if (!isInitialized() || !isEnabled())
        {
            Core::warn("ClusterUpdateStage: Not initialized or enabled");
            return;
        }

        PROFILE_SCOPE("ClusterUpdateStage::execute");

        if (!validateClusterConfiguration())
        {
            Core::error("ClusterUpdateStage: Invalid cluster configuration");
            return;
        }

        // Synchronize configuration with ForwardPlusShaderManager
        if (m_clusterConfigDirty && m_forwardPlusShaderManager)
        {
            synchronizeShaderManagerConfig();
        }

        // Update cluster data for the current frame
        updateClusterData(context);

        // Update frame counter and log performance metrics periodically
        m_frameCounter++;
        if (m_frameCounter % 60 == 0) // Log every 60 frames
        {
            logPerformanceMetrics();
        }

        Core::debug("ClusterUpdateStage: Updated clustered lighting data for forward+ pipeline");
    }

    void ClusterUpdateStage::setLightManager(std::shared_ptr<LightManager> lightManager)
    {
        m_lightManager = lightManager;
        m_clusterConfigDirty = true;
    }

    void ClusterUpdateStage::setForwardPlusShaderManager(std::shared_ptr<ForwardPlusShaderManager> shaderManager)
    {
        m_forwardPlusShaderManager = shaderManager;
        m_clusterConfigDirty = true;
    }

    std::shared_ptr<ForwardPlusShaderManager> ClusterUpdateStage::getForwardPlusShaderManager() const
    {
        return m_forwardPlusShaderManager;
    }

    void ClusterUpdateStage::setClusterDimensions(const glm::ivec3& dimensions)
    {
        if (m_clusterDimensions != dimensions)
        {
            m_clusterDimensions = dimensions;
            m_clusterConfigDirty = true;
            Core::info("ClusterUpdateStage: Cluster dimensions updated to (" +
                       std::to_string(dimensions.x) + ", " +
                       std::to_string(dimensions.y) + ", " +
                       std::to_string(dimensions.z) + ")");
        }
    }

    const glm::ivec3& ClusterUpdateStage::getClusterDimensions() const
    {
        return m_clusterDimensions;
    }

    void ClusterUpdateStage::configureClusterCuller(const ClusterConfig& config)
    {
        if (!m_lightManager)
        {
            Core::error("ClusterUpdateStage: Cannot configure cluster culler without light manager");
            return;
        }

        if (!m_lightManager->isClusteringEnabled())
        {
            Core::error("ClusterUpdateStage: Clustering not enabled in light manager");
            return;
        }

        m_lightManager->configureClusterCuller(config);
        m_clusterConfigDirty = false;
        
        Core::info("ClusterUpdateStage: Configured cluster culler with " +
                   std::to_string(config.clusterCountX) + "x" +
                   std::to_string(config.clusterCountY) + "x" +
                   std::to_string(config.clusterCountZ) + " clusters");

        // Update cluster dimensions to match
        m_clusterDimensions = glm::ivec3(config.clusterCountX, config.clusterCountY, config.clusterCountZ);
        
        // Synchronize with ForwardPlusShaderManager
        if (m_forwardPlusShaderManager)
        {
            synchronizeShaderManagerConfig();
        }
    }

    float ClusterUpdateStage::getLastClusterUpdateTime() const
    {
        if (!m_lightManager || !m_lightManager->isClusteringEnabled())
        {
            return 0.0f;
        }

        return m_lightManager->getClusterCuller()->getLastUpdateTime();
    }

    uint32_t ClusterUpdateStage::getTotalLightAssignments() const
    {
        if (!m_lightManager || !m_lightManager->isClusteringEnabled())
        {
            return 0;
        }

        return m_lightManager->getClusterCuller()->getTotalLightAssignments();
    }

    void ClusterUpdateStage::updateClusterData(const std::shared_ptr<RenderContext> context)
    {
        PROFILE_SCOPE("ClusterUpdateStage::updateClusterData");

        if (!m_lightManager)
        {
            Core::warn("ClusterUpdateStage: Missing light manager");
            return;
        }

        // Configure cluster culler if dimensions changed
        if (m_clusterConfigDirty && m_lightManager->isClusteringEnabled())
        {
            ClusterConfig config;
            config.clusterCountX = m_clusterDimensions.x;
            config.clusterCountY = m_clusterDimensions.y;
            config.clusterCountZ = m_clusterDimensions.z;
            
            // Use defaults optimized for isometric voxel games
            config.nearBias = 0.3f;
            config.verticalStretch = 1.5f;
            config.enableAdaptiveDensity = true;
            config.enableVerticalParallax = true;
            config.maxLightsPerCluster = 128;
            config.maxClusteringTime = 16.0f;
            
            configureClusterCuller(config);
        }

        // Update clusters through the light manager
        // This will call the ClusteredLightCuller::update() method internally
        // and handle GPU buffer uploads
        m_lightManager->updateClusters(context);

        // Notify ForwardPlusShaderManager that cluster data has been updated
        if (m_forwardPlusShaderManager && m_forwardPlusShaderManager->isForwardPlusReady())
        {
            // ForwardPlusShaderManager will handle its own buffer updates based on the 
            // ClusteredLightCuller data when bindClusterData() is called during rendering
            Core::debug("ClusterUpdateStage: Coordinated cluster data update with ForwardPlusShaderManager");
        }

        Core::debug("ClusterUpdateStage: Cluster data updated for frame " + std::to_string(m_frameCounter));
    }

    bool ClusterUpdateStage::validateClusterConfiguration() const
    {
        if (!m_lightManager)
        {
            Core::error("ClusterUpdateStage: Light manager not set");
            return false;
        }

        if (!m_lightManager->isClusteringEnabled())
        {
            Core::error("ClusterUpdateStage: Clustering not enabled in light manager");
            return false;
        }

        if (m_clusterDimensions.x <= 0 || m_clusterDimensions.y <= 0 || m_clusterDimensions.z <= 0)
        {
            Core::error("ClusterUpdateStage: Invalid cluster dimensions (" +
                        std::to_string(m_clusterDimensions.x) + ", " +
                        std::to_string(m_clusterDimensions.y) + ", " +
                        std::to_string(m_clusterDimensions.z) + ")");
            return false;
        }

        return true;
    }

    void ClusterUpdateStage::logPerformanceMetrics() const
    {
        if (!m_lightManager || !m_lightManager->isClusteringEnabled())
        {
            return;
        }

        float updateTime = getLastClusterUpdateTime();
        uint32_t lightAssignments = getTotalLightAssignments();
        
        Core::info("ClusterUpdateStage Performance - Frame: " +
                   std::to_string(m_frameCounter) + ", Update Time: " +
                   std::to_string(updateTime) + "ms, Light Assignments: " +
                   std::to_string(lightAssignments));

        // Warn if performance is degrading
        if (updateTime > 10.0f)
        {
            Core::warn("ClusterUpdateStage: Cluster update taking " +
                       std::to_string(updateTime) + "ms (target: <10ms)");
        }

        if (lightAssignments > 10000)
        {
            Core::warn("ClusterUpdateStage: High light assignment count: " + std::to_string(lightAssignments) +
                       " (consider reducing light count or cluster density)");
        }
    }

    void ClusterUpdateStage::synchronizeShaderManagerConfig()
    {
        if (!m_forwardPlusShaderManager)
        {
            return;
        }

        // Create ForwardPlusShaderConfig from current cluster settings
        ForwardPlusShaderConfig shaderConfig;
        shaderConfig.clusterDimensions = m_clusterDimensions;
        shaderConfig.maxLightsPerCluster = 128; // Default matching cluster culler
        shaderConfig.enableAdaptiveDensity = true;
        shaderConfig.enableVerticalParallax = true;
        shaderConfig.enableDebugVisualization = false;

        // Apply configuration to ForwardPlusShaderManager
        m_forwardPlusShaderManager->configureShaders(shaderConfig);
        
        // Connect the clustered light culler if available
        if (m_lightManager && m_lightManager->isClusteringEnabled())
        {
            m_forwardPlusShaderManager->setClusteredLightCuller(m_lightManager->getClusterCuller());
        }
        
        Core::debug("ClusterUpdateStage: Synchronized configuration with ForwardPlusShaderManager");
        m_clusterConfigDirty = false;
    }

} // namespace PixelCraft::Rendering::Stages
