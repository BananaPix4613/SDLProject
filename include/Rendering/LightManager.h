// -------------------------------------------------------------------------
// LightManager.h
// -------------------------------------------------------------------------
#pragma once

#include "Core/Subsystem.h"
#include "Core/Resource.h"
#include "Rendering/RenderContext.h"
#include "Rendering/RenderTarget.h"
#include "Rendering/Light.h"
#include "Rendering/ClusteredLightCuller.h"
#include "Utility/Frustum.h"

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

namespace PixelCraft::Rendering
{
    class ForwardPlusShaderManager;

    /**
     * @brief Manager for light sources and shadows in the rendering system
     *
     * The LightManager handles different types of light sources (directional, point, spot, area),
     * manages shadow mapping, implements clustered lighting for performance optimization via 
     * ClusteredLightCuller, performs light culling against the view frustum, and delegates 
     * GPU buffer updates to ForwardPlusShaderManager for proper forward+ integration.
     */
    class LightManager : public Core::Subsystem
    {
    public:
        /**
         * @brief Constructor
         */
        LightManager();

        /**
         * @brief Destructor
         */
        ~LightManager() override;

        /**
         * @brief Initialize the light manager
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Update the light manager
         * @param deltaTime Time elapsed since last update
         */
        void update(float deltaTime) override;

        /**
         * @brief Render light-related elements
         */
        void render() override;

        /**
         * @brief Shut down the light manager and release resources
         */
        void shutdown() override;

        /**
         * @brief Get the name of the subsystem
         * @return The subsystem name
         */
        std::string getName() const override
        {
            return "LightManager";
        }

        /**
         * @brief Get dependencies required by this subsystem
         * @return List of dependency subsystem names
         */
        std::vector<std::string> getDependencies() const override;

        /**
         * @brief Create a light of the specified type
         * @param type The type of light to create
         * @return A shared pointer to the created light
         */
        std::shared_ptr<Light> createLight(LightType type);

        /**
         * @brief Create a directional light (convenience method)
         * @return A shared pointer to the created directional light
         */
        std::shared_ptr<Light> createDirectionalLight();

        /**
         * @brief Create a point light (convenience method)
         * @return A shared pointer to the created point light
         */
        std::shared_ptr<Light> createPointLight();

        /**
         * @brief Create a spot light (convenience method)
         * @return A shared pointer to the created spot light
         */
        std::shared_ptr<Light> createSpotLight();

        /**
         * @brief Create an area light (convenience method)
         * @return A shared pointer to the created area light
         */
        std::shared_ptr<Light> createAreaLight();

        /**
         * @brief Remove a light from the manager
         * @param light The light to remove
         */
        void removeLight(std::shared_ptr<Light> light);

        /**
         * @brief Get all lights managed by this LightManager
         * @return A vector of all lights
         */
        const std::vector<std::shared_ptr<Light>>& getLights() const;

        /**
         * @brief Get lights that are currently visible in the view frustum
         * @return A vector of visible lights
         */
        const std::vector<std::shared_ptr<Light>>& getVisibleLights() const;

        /**
         * @brief Get the total number of lights managed by this LightManager
         * @return The number of lights
         */
        size_t getLightCount() const;

        /**
         * @brief Force an update of the light data for GPU buffers
         * 
         * This method ensures all light data is up to date and synchronized 
         * with the GPU buffers through ForwardPlusShaderManager.
         */
        void updateLightData();

        /**
         * @brief Configure the clustered light culler
         * @param config The cluster configuration to use
         */
        void configureClusterCuller(const ClusterConfig& config);

        /**
         * @brief Update the light clusters based on the current render context
         * @param context The current render context
         */
        void updateClusters(const std::shared_ptr<RenderContext> context);

        /**
         * @brief Get the current cluster data from the culler
         * @return The cluster data structure
         */
        const ClusterData& getClusterData() const;

        /**
         * @brief Get the clustered light culler instance
         * @return Pointer to the cluster culler
         */
        std::shared_ptr<ClusteredLightCuller> getClusterCuller();

        /**
         * @brief Check if clustering is enabled and initialized
         * @return True if clustering is available
         */
        bool isClusteringEnabled() const;

        /**
         * @brief Prepare shadow maps for all shadow-casting lights
         * @param context The current render context
         */
        void prepareShadowMaps(const RenderContext& context);

        /**
         * @brief Get the shadow map for a specific index
         * @param index The shadow map index
         * @return A shared pointer to the render target containing the shadow map
         */
        std::shared_ptr<RenderTarget> getShadowMap(size_t index) const;

        /**
         * @brief Setup cascaded shadow maps for directional lights
         * @param cascadeCount The number of cascades to use
         * @param splitLambda Lambda factor for cascade split calculation (0.0 to 1.0)
         */
        void setupCascadedShadowMaps(int cascadeCount, float splitLambda = 0.5f);

        /**
         * @brief Get the view-projection matrices for all cascades
         * @return A vector of cascade view-projection matrices
         */
        const std::vector<glm::mat4>& getCascadeViewProjections() const;

        /**
         * @brief Get the cascade split distances
         * @return A vector of cascade split distances
         */
        const std::vector<float>& getCascadeSplits() const;

        /**
         * @brief Update the list of visible lights based on the view frustum
         * @param frustum The current view frustum
         */
        void updateVisibleLights(const Utility::Frustum& frustum);

        /**
         * @brief Update the GPU buffers with the current light data
         * 
         * This method delegates to ForwardPlusShaderManager for proper forward+ buffer management.
         * It prepares light data and coordinates with the forward+ system for efficient GPU updates.
         */
        void updateGPUBuffers();

        /**
         * @brief Get the GPU buffer containing light data from ForwardPlusShaderManager
         * @return The light data buffer ID, or 0 if ForwardPlusShaderManager is not available
         */
        uint32_t getLightDataBuffer() const;

        /**
         * @brief Get the GPU buffer containing cluster data from ForwardPlusShaderManager
         * @return The cluster data buffer ID, or 0 if ForwardPlusShaderManager is not available
         */
        uint32_t getClusterDataBuffer() const;

        /**
         * @brief Set the ForwardPlusShaderManager for GPU buffer coordination
         * @param forwardPlusManager The ForwardPlusShaderManager instance
         */
        void setForwardPlusShaderManager(std::shared_ptr<ForwardPlusShaderManager> forwardPlusManager);

        /**
         * @brief Get the ForwardPlusShaderManager instance
         * @return The ForwardPlusShaderManager instance, or nullptr if not set
         */
        std::shared_ptr<ForwardPlusShaderManager> getForwardPlusShaderManager() const;

    private:
        /**
         * @brief Create or update a shadow map for the specified light
         * @param light The light that needs a shadow map
         *
         * Creates an appropriately sized and formatted shadow map render target
         * based on the light type (cubemap for point lights, 2D for others)
         * and registers it in the shadow map index.
         */
        void createShadowMapForLight(std::shared_ptr<Light> light);

        /**
         * @brief Calculate cascade split distances based on the render context
         * @param context The current render context
         *
         * Uses a logarithmic distribution controlled by m_cascadeSplitLambda
         * to calculate optimal cascade split distances and updates the
         * cascade view-projection matrices.
         */
        void calculateCascadeSplits(const RenderContext& context);

        // All managed lights
        std::vector<std::shared_ptr<Light>> m_lights;

        // Lights visible in the current view frustum
        std::vector<std::shared_ptr<Light>> m_visibleLights;

        // Light type-specific lists for optimization
        std::vector<std::shared_ptr<Light>> m_directionalLights;
        std::vector<std::shared_ptr<Light>> m_pointLights;
        std::vector<std::shared_ptr<Light>> m_spotLights;
        std::vector<std::shared_ptr<Light>> m_areaLights;

        // Clustered light culling
        std::shared_ptr<ClusteredLightCuller> m_clusterCuller;
        bool m_clusteringEnabled;

        // Forward+ integration
        std::shared_ptr<ForwardPlusShaderManager> m_forwardPlusShaderManager;

        // Shadow mapping
        std::vector<std::shared_ptr<RenderTarget>> m_shadowMaps;
        std::unordered_map<std::shared_ptr<Light>, size_t> m_lightShadowIndices;

        // Cascaded shadow mapping for directional lights
        int m_cascadeCount;
        float m_cascadeSplitLambda;
        std::vector<float> m_cascadeSplits;
        std::vector<glm::mat4> m_cascadeViewProjections;

        // Initialization state
        bool m_initialized;
    };

} // namespace PixelCraft::Rendering
