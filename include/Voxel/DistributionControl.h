// -------------------------------------------------------------------------
// DistributionControl.h
// -------------------------------------------------------------------------
#pragma once

#include "Voxel/ChunkCoord.h"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <glm/glm.hpp>

namespace PixelCraft::Voxel
{

    /**
     * @brief Controls feature distribution and density
     * 
     * Manages density maps, clustering, and spatial distribution of features
     * within the procedurally generated world.
     */
    class DistributionControl
    {
    public:
        /**
         * @brief Distribution pattern type
         */
        enum class PatternType
        {
            Uniform,        // Uniformly distributed
            Clustered,      // Clustered around points
            Gradient,       // Following a gradient
            Stratified,     // Stratified random distribution
            Voronoi,        // Voronoi-based distribution
            BlueNoise,      // Blue noise distribution (visually pleasing)
            Fibonacci,      // Fibonacci spiral distribution
            Custom          // Custom distribution pattern
        };

        /**
         * @brief Constructor
         */
        DistributionControl();

        /**
         * @brief Destructor
         */
        virtual ~DistributionControl();

        /**
         * @brief Set the distribution pattern
         * @param pattern The distribution pattern to use
         */
        void setPattern(PatternType pattern);

        /**
         * @brief Get the current distribution pattern
         * @return The current pattern
         */
        PatternType getPattern() const;

        /**
         * @brief Set the global density factor
         * @param density Density factor [0.0-1.0]
         */
        void setGlobalDensity(float density);

        /**
         * @brief Get the global density factor
         * @return The global density factor
         */
        float getGlobalDensity() const;

        /**
         * @brief Set density for a specific feature type
         * @param featureType Type name of the feature
         * @param density Density factor [0.0-1.0]
         */
        void setFeatureDensity(const std::string& featureType, float density);

        /**
         * @brief Get density for a feature type
         * @param featureType Type name of the feature
         * @return The density factor, returns global density if not set
         */
        float getFeatureDensity(const std::string& featureType) const;

        /**
         * @brief Set the radius of influence for density
         * @param radius Influence radius in world units
         */
        void setInfluenceRadius(float radius);

        /**
         * @brief Get the influence radius
         * @return The influence radius
         */
        float getInfluenceRadius() const;

        /**
         * @brief Set the cluster size (when using clustered pattern)
         * @param size Size factor for clusters [0.1-10.0]
         */
        void setClusterSize(float size);

        /**
         * @brief Get the cluster size
         * @return The cluster size factor
         */
        float getClusterSize() const;

        /**
         * @brief Set the cluster spacing (when using clustered pattern)
         * @param spacing Spacing between clusters in world units
         */
        void setClusterSpacing(float spacing);

        /**
         * @brief Get the cluster spacing
         * @return The cluster spacing
         */
        float getClusterSpacing() const;

        /**
         * @brief Set cluster falloff factor (edge fading)
         * @param falloff Falloff factor [0.0-1.0]
         */
        void setClusterFalloff(float falloff);

        /**
         * @brief Get the cluster fallof factor
         * @return The cluster falloff factor
         */
        float getClusterFalloff() const;

        /**
         * @brief Set the seed for distribution patterns
         * @param seed Seed value
         */
        void setSeed(uint32_t seed);

        /**
         * @brief Get the current seed
         * @return The current seed value
         */
        uint32_t getSeed() const;

        /**
         * @brief Calculate density at a world position
         * @param position World position to check
         * @param featureType Type of feature (optional, uses global density if empty)
         * @return Density value [0.0-1.0]
         */
        float getDensityAt(const glm::vec3& position, const std::string& featureType = "") const;

        /**
         * @brief Add a density modifier region
         * @param centerPosition Center of the region
         * @param radius Radius of the region
         * @param multiplier Density multiplier [0.0-10.0]
         * @param featureType Type of feature affected (empty for all)
         * @return ID of the created region for later reference
         */
        int addDensityRegion(const glm::vec3& centerPosition, float radius, float multiplier,
                             const std::string& featureType = "");

        /**
         * @brief Remove a density modifier region
         * @param regionId ID of the region to remove
         * @return True if region was found and removed
         */
        bool removeDensityRegion(int regionId);

        /**
         * @brief Add a cluster center
         * @param position Center position of the cluster
         * @param strength Strength of the cluster [0.0-1.0]
         * @param featureType Type of feature affected (empty for all)
         * @return ID of the created cluster for later reference
         */
        int addCluster(const glm::vec3& position, float strength, const std::string& featureType = "");

        /**
         * @brief Remove a cluster
         * @param clusterId ID of the cluster to remove
         * @return True if cluster was found and removed
         */
        bool removeCluster(int clusterId);

        /**
         * @brief Clear all clusters
         */
        void clearClusters();

        /**
         * @brief Set density threshold for feature placement
         * @param threshold Minimum density required [0.0-1.0]
         * @param featureType Type of feature (empty for all)
         */
        void setDensityThreshold(float threshold, const std::string& featureType = "");

        /**
         * @brief Get density threshold for a feature type
         * @param featureType Type of feature
         * @return Threshold value, 0 if not set
         */
        float getDensityThreshold(const std::string& featureType) const;

        /**
         * @brief Generate sample points in a chunk based on distribution
         * @param chunkCoord Coordinate of the chunk
         * @param maxPoints Maximum number of points to generate
         * @param featureType Type of feature (for density calculation)
         * @return Vector of world positions for potential feature placement
         */
        std::vector<glm::vec3> generateDistributionPoints(const ChunkCoord& chunkCoord,
                                                          int maxPoints,
                                                          const std::string& featureType = "") const;

        /**
         * @brief Set noise-based distribution map
         * @param noiseLayerId ID of the noise layer to use
         * @param threshold Threshold value for density [0.0-1.0]
         * @param featureType Type of feature (empty for all)
         */
        void setNoiseDistribution(const std::string& noiseLayerId, float threshold,
                                  const std::string& featureType = "");

        /**
         * @brief Get noise layer ID for distribution
         * @param featureType Type of feature
         * @return Noise layer ID, empty if not set
         */
        std::string getNoiseDistribution(const std::string& featureType) const;

        /**
         * @brief Get noise threshold for distribution
         * @param featureType Type of feature
         * @return Threshold value, 0 if not set
         */
        float getNoiseThreshold(const std::string& featureType) const;

        /**
         * @brief Set a density gradient
         * @param startPosition Start position of the gradient
         * @param endPosition End position of the gradient
         * @param startDensity Density at start [0.0-1.0]
         * @param endDensity Density at end [0.0-1.0]
         * @param featureType Type of feature (empty for all)
         * @return ID of the created gradient for later reference
         */
        int setGradient(const glm::vec3& startPosition, const glm::vec3& endPosition,
                        float startDensity, float endDensity,
                        const std::string& featureType = "");

        /**
         * @brief Remove a gradient
         * @param gradientId ID of the gradient to remove
         * @return True if gradient was found and removed
         */
        bool removeGradient(int gradientId);

        /**
         * @brief Register a custom distribution function
         * @param name Unique name for the function
         * @param distributionFunc Function that takes a position and returns a density [0.0-1.0]
         * @return True if successfully registered
         */
        bool registerCustomDistribution(const std::string& name,
                                        std::function<float(const glm::vec3&, uint32_t)> distributionFunc);

        /**
         * @brief Set the active custom distribution
         * @param name Name of the registered custom distribution
         * @return True if distribution was found and set as active
         */
        bool setActiveCustomDistribution(const std::string& name);

        /**
         * @brief Get the name of the active custom distribution
         * @returns Name of the active custom distribution, empty if none
         */
        std::string getActiveCustomDistribution() const;

        /**
         * @brief Serialize the distribution configuration
         * @param serializer Serializer to serialize with
         */
        void serialize(ECS::Serializer& serializer) const;

        /**
         * @brief Deserialize the distribution configuration
         * @param deserializer Deserializer to deserialize with
         */
        void deserialize(ECS::Deserializer& deserializer);

    private:
        // Internal structures
        struct DensityRegion
        {
            int id;
            glm::vec3 position;
            float radius;
            float multiplier;
            std::string featureType;
        };

        struct Cluster
        {
            int id;
            glm::vec3 position;
            float strength;
            std::string featureType;
        };

        struct Gradient
        {
            int id;
            glm::vec3 startPosition;
            glm::vec3 endPosition;
            float startDensity;
            float endDensity;
            std::string featureType;
        };

        struct NoiseDistribution
        {
            std::string noiseLayerId;
            float threshold;
        };

        // Private helper methods
        float calculateBaseDensity(const glm::vec3& position, const std::string& featureType) const;
        float applyRegionModifiers(const glm::vec3& position, float baseDensity, const std::string& featureType) const;
        float applyClusterEffect(const glm::vec3& position, float density, const std::string& featureType) const;
        float applyGradientEffect(const glm::vec3& position, float density, const std::string& featureType) const;
        std::vector<glm::vec3> generatePointsUniform(const ChunkCoord& chunkCoord, int count) const;
        std::vector<glm::vec3> generatePointsClustered(const ChunkCoord& chunkCoord, int count) const;
        std::vector<glm::vec3> generatePointsStratified(const ChunkCoord& chunkCoord, int count) const;
        std::vector<glm::vec3> generatePointsVoronoi(const ChunkCoord& chunkCoord, int count) const;
        std::vector<glm::vec3> generatePointsBlueNoise(const ChunkCoord& chunkCoord, int count) const;
        std::vector<glm::vec3> generatePointsFibonacci(const ChunkCoord& chunkCoord, int count) const;

        // Private member variables
        PatternType m_pattern = PatternType::Uniform;
        float m_globalDensity = 0.5f;
        std::unordered_map<std::string, float> m_featureDensities;
        float m_influenceRadius = 16.0f;
        float m_clusterSize = 1.0f;
        float m_clusterSpacing = 32.0f;
        float m_clusterFalloff = 0.5f;
        uint32_t m_seed = 54321;

        std::vector<DensityRegion> m_densityRegions;
        std::vector<Cluster> m_clusters;
        std::vector<Gradient> m_gradients;
        std::unordered_map<std::string, NoiseDistribution> m_noiseDistributions;
        std::unordered_map<std::string, float> m_densityThresholds;

        int m_nextRegionId = 0;
        int m_nextClusterId = 0;
        int m_nextGradientId = 0;

        std::unordered_map<std::string, std::function<float(const glm::vec3&, uint32_t)>> m_customDistributions;
        std::string m_activeCustomDistribution;
    };

} // namespace PixelCraft::Voxel