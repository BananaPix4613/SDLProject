// -------------------------------------------------------------------------
// LightManager.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

#include "Core/Subsystem.h"
#include "Core/Resource.h"
#include "Rendering/RenderContext.h"
#include "Rendering/RenderTarget.h"
#include "Utility/Frustum.h"

namespace PixelCraft::Rendering
{

    /**
     * @brief Types of light sources supported by the engine
     */
    enum class LightType
    {
        Directional,  ///< Directional light (sun, moon)
        Point,        ///< Point light (omnidirectional)
        Spot,         ///< Spot light (cone)
        Area          ///< Area light (rectangular)
    };

    /**
     * @brief Data structure for light properties optimized for GPU use
     *
     * This structure is designed to be directly uploaded to a GPU buffer
     * for efficient use in shaders.
     */
    struct LightData
    {
        glm::vec3 position;    ///< World position of the light
        float range;           ///< Maximum range/radius of the light
        glm::vec3 color;       ///< RGB color of the light
        float intensity;       ///< Brightness multiplier
        glm::vec3 direction;   ///< Forward direction of directional/spot lights
        float spotAngle;       ///< Cosine of outer spot angle (for spot lights)
        glm::vec2 areaSize;    ///< Width and height of area lights
        uint32_t shadowIndex;  ///< Index to shadow map in shadow array
        LightType type;        ///< Type of light
        bool castShadows;      ///< Whether this light casts shadows
    };

    /**
     * @brief Light source with customizable properties
     *
     * Represents a single light source in the scene with properties
     * specific to each light type (directional, point, spot, area).
     */
    class Light
    {
    public:
        /**
         * @brief Constructor
         * @param type The type of light to create
         */
        Light(LightType type);

        /**
         * @brief Virtual destructor
         */
        virtual ~Light();

        // Light properties
        /**
         * @brief Set the world position of the light
         * @param position The new position
         */
        void setPosition(const glm::vec3& position);

        /**
         * @brief Set the direction the light is pointing
         * @param direction The new direction (will be normalized)
         */
        void setDirection(const glm::vec3& direction);

        /**
         * @brief Set the RGB color of the light
         * @param color The new color
         */
        void setColor(const glm::vec3& color);

        /**
         * @brief Set the brightness multiplier of the light
         * @param intensity The new intensity (must be non-negative)
         */
        void setIntensity(float intensity);

        /**
         * @brief Set the maximum range/radius of the light
         * @param range The new range (must be positive)
         */
        void setRange(float range);

        /**
         * @brief Set whether this light casts shadows
         * @param castShadows True if the light should cast shadows
         */
        void setCastShadows(bool castShadows);

        // Type-specific setters
        /**
         * @brief Set the inner and outer angles for spot lights
         * @param innerAngle The inner cone angle in radians
         * @param outerAngle The outer cone angle in radians
         */
        void setSpotAngle(float innerAngle, float outerAngle);

        /**
         * @brief Set the dimensions for area lights
         * @param size Width and height of the area light
         */
        void setAreaSize(const glm::vec2& size);

        // Getters
        /**
         * @brief Get the type of this light
         * @return The light type
         */
        LightType getType() const;

        /**
         * @brief Get the world position of the light
         * @return The light position
         */
        const glm::vec3& getPosition() const;

        /**
         * @brief Get the direction the light is pointing
         * @return The normalized light direction
         */
        const glm::vec3& getDirection() const;

        /**
         * @brief Get the RGB color of the light
         * @return The light color
         */
        const glm::vec3& getColor() const;

        /**
         * @brief Get the brightness multiplier of the light
         * @return The light intensity
         */
        float getIntensity() const;

        /**
         * @brief Get the maximum range/radius of the light
         * @return The light range
         */
        float getRange() const;

        /**
         * @brief Check if this light casts shadows
         * @return True if the light casts shadows
         */
        bool getCastShadows() const;

        /**
         * @brief Get the area light dimensions
         * @return Width and height of the area light
         */
        const glm::vec2& getAreaSize() const;

        // Utility methods
        /**
         * @brief Get the view matrix for shadow mapping
         * @return View matrix from the light's perspective
         */
        glm::mat4 getViewMatrix() const;

        /**
         * @brief Get the projection matrix for shadow mapping
         * @return Projection matrix appropriate for the light type
         */
        glm::mat4 getProjectionMatrix() const;

        // Shadow parameters
        /**
         * @brief Set the shadow bias to prevent shadow acne
         * @param bias The shadow depth bias value
         */
        void setShadowBias(float bias);

        /**
         * @brief Set the resolution of this light's shadow map
         * @param resolution The shadow map resolution (will be rounded to power of 2)
         */
        void setShadowResolution(int resolution);

        /**
         * @brief Pack light data for shader use
         * @return Packed light data structure for GPU
         */
        LightData packLightData() const;

    private:
        LightType m_type;
        glm::vec3 m_position;
        glm::vec3 m_direction;
        glm::vec3 m_color;
        float m_intensity;
        float m_range;
        bool m_castShadows;

        // Spot light properties
        float m_spotInnerAngle;
        float m_spotOuterAngle;

        // Area light properties
        glm::vec2 m_areaSize;

        // Shadow properties
        float m_shadowBias;
        int m_shadowResolution;
    };

    /**
     * @brief Data structure for clustered lighting
     *
     * Contains information about the 3D grid of clusters and the
     * assignment of lights to each cluster.
     */
    struct ClusterData
    {
        glm::ivec3 dimensions;           ///< 3D dimensions of the cluster grid (x, y, z)
        glm::vec3 minBounds;             ///< Minimum world bounds of the cluster volume
        glm::vec3 maxBounds;             ///< Maximum world bounds of the cluster volume
        std::vector<uint32_t> lightGrid;  ///< Grid data: start index and count packed into 32 bits
        std::vector<uint32_t> lightIndices; ///< Indices into the visible lights array
    };

    /**
     * @brief Manager for light sources and shadows in the rendering system
     *
     * The LightManager handles different types of light sources (directional, point, spot, area),
     * manages shadow mapping, implements clustered lighting for performance optimization,
     * performs light culling against the view frustum, and provides efficient GPU buffer updates.
     */
    class LightManager
    {
    public:
        /**
         * @brief Constructor
         */
        LightManager();

        /**
         * @brief Destructor
         */
        ~LightManager();

        /**
         * @brief Initialize the light manager
         * @return True if initialization was successful
         */
        bool initialize();

        /**
         * @brief Shut down the light manager and release resources
         */
        void shutdown();

        /**
         * @brief Create a light of the specified type
         * @param type The type of light to create
         * @return A shared pointer to the created light
         */
        std::shared_ptr<Light> createLight(LightType type);

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
         * @brief Setup light clustering with the specified dimensions
         * @param dimensions The dimensions of the cluster grid (x, y, z)
         */
        void setupClusters(const glm::ivec3& dimensions);

        /**
         * @brief Update the light clusters based on the current render context
         * @param context The current render context
         */
        void updateClusters(const RenderContext& context);

        /**
         * @brief Get the current cluster data
         * @return The cluster data structure
         */
        const ClusterData& getClusterData() const;

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
         */
        void updateGPUBuffers();

        /**
         * @brief Get the GPU buffer containing light data
         * @return The light data buffer ID
         */
        uint32_t getLightDataBuffer() const;

        /**
         * @brief Get the GPU buffer containing cluster data
         * @return The cluster data buffer ID
         */
        uint32_t getClusterDataBuffer() const;

    private:
        /**
         * @brief Assign lights to clusters based on their positions and ranges
         *
         * This method performs spatial tests to determine which lights affect each
         * cluster in the 3D grid, and builds the data structures for efficient
         * lookup in shaders.
         */
        void assignLightsToClusters();

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

        // Clustering
        ClusterData m_clusterData;
        bool m_clustersDirty;

        // Shadow mapping
        std::vector<std::shared_ptr<RenderTarget>> m_shadowMaps;
        std::unordered_map<std::shared_ptr<Light>, size_t> m_lightShadowIndices;

        // Cascaded shadow mapping for directional lights
        int m_cascadeCount;
        float m_cascadeSplitLambda;
        std::vector<float> m_cascadeSplits;
        std::vector<glm::mat4> m_cascadeViewProjections;

        // GPU buffers
        uint32_t m_lightDataBuffer;
        uint32_t m_clusterDataBuffer;

        // Initialization state
        bool m_initialized;
    };

} // namespace PixelCraft::Rendering