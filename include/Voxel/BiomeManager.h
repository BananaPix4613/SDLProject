// -------------------------------------------------------------------------
// BiomeManager.h
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

    // Forward declarations
    class Chunk;
    struct GenerationContext;

    /**
     * @brief Manages biome types and transitions
     * 
     * Handles biome definition, placement, and blending between different biome
     * types during procedural generation.
     */
    class BiomeManager
    {
    public:
        /**
         * @brief Structure representing biome parameters
         */
        struct BiomeInfo
        {
            std::string name;
            int id;
            std::string noiseLayerId;
            float threshold;
            float weight;
            bool enabled;
            glm::vec2 temperatureRange;  // Min/max temperature for this biome
            glm::vec2 humidityRange;     // Min/max humidity for this biome
            glm::vec2 elevationRange;    // Min/max elevation for this biome
            std::unordered_map<std::string, float> voxelProbabilities;  // Voxel type to probability mapping
            std::unordered_map<std::string, float> featureProbabilities; // Feature type to probability mapping
        };

        /**
         * @brief Constructor
         */
        BiomeManager();

        /**
         * @brief Destructor
         */
        virtual ~BiomeManager();

        /**
         * @brief Register a new biome type
         * @param name Unique name for the biome
         * @return Biome ID if successful, -1 if failed
         */
        int registerBiome(const std::string& name);

        /**
         * @brief Remove a biome type
         * @param biomeId ID of the biome to remove
         * @return True if biome was found and removed
         */
        bool removeBiome(int biomeId);

        /**
         * @brief Get biome information by ID
         * @param biomeId ID of the biome
         * @return Biome info, nullptr if not found
         */
        const BiomeInfo* getBiomeInfo(int biomeId) const;

        /**
         * @brief Get biome information by name
         * @param name Name of the biome
         * @return Biome info, nullptr if not found
         */
        const BiomeInfo* getBiomeInfo(const std::string& name) const;

        /**
         * @brief Get all registered biome IDs
         * @return Vector of biome IDs
         */
        std::vector<int> getBiomeIds() const;

        /**
         * @brief Get all registered biome names
         * @return Vector of biome names
         */
        std::vector<std::string> getBiomeNames() const;

        /**
         * @brief Set biome noise layer
         * @param biomeId ID of the biome
         * @param noiseLayerId ID of the noise layer
         * @param threshold Threshold value for biome boundary [0.0-1.0]
         * @return True if biome was found
         */
        bool setBiomeNoise(int biomeId, const std::string& noiseLayerId, float threshold);

        /**
         * @brief Set temperature range for a biome
         * @param biomeId ID of the biome
         * @param minTemp Minimum temperature
         * @param maxTemp Maximum temperature
         * @return True if biome was found
         */
        bool setBiomeTemperatureRange(int biomeId, float minTemp, float maxTemp);

        /**
         * @brief Set humidity range for a biome
         * @param biomeId ID of the biome
         * @param minHumidity Minimum humidity
         * @param maxHumidity Maximum humidity
         * @return True if biome was found
         */
        bool setBiomeHumidityRange(int biomeId, float minHumidity, float maxHumidity);

        /**
         * @brief Set elevation range for a biome
         * @param biomeId ID of the biome
         * @param minElevation Minimum elevation
         * @param maxElevation Maximum elevation
         * @return True if biome was found
         */
        bool setBiomeElevationRange(int biomeId, float minElevation, float maxElevation);

        /**
         * @brief Set probability for a voxel type in a biome
         * @param biomeId ID of the biome
         * @param voxelType Type of voxel
         * @param probability Probability [0.0-1.0]
         * @return True if biome was found
         */
        bool setVoxelProbability(int biomeId, const std::string& voxelType, float probability);

        /**
         * @brief Get probability for a voxel type in a biome
         * @param biomeId ID of the biome
         * @param voxelType Type of voxel
         * @return Probability value, 0 if not found
         */
        float getVoxelProbability(int biomeId, const std::string& voxelType) const;

        /**
         * @brief Set probability for a feature type in a biome
         * @param biomeId ID of the biome
         * @param featureType Type of feature
         * @param probability Probability [0.0-1.0]
         * @return True if biome was found
         */
        bool setFeatureProbability(int biomeId, const std::string& featureType, float probability);

        /**
         * @brief Get probability for a feature type in a biome
         * @param biomeId ID of the biome
         * @param featureType Type of feature
         * @return Probability value, 0 if not found
         */
        float getFeatureProbability(int biomeId, const std::string& featureType) const;

        /**
         * @brief Set biome weight (importance during blending)
         * @param biomeId ID of the biome
         * @param weight Weight value [0.0-1.0]
         * @return True if biome was found
         */
        bool setBiomeWeight(int biomeId, float weight);

        /**
         * @brief Get biome weight
         * @param biomeId ID of the biome
         * @return Weight value, 0 if not found
         */
        float getBiomeWeight(int biomeId) const;

        /**
         * @brief Enable or disable a biome
         * @param biomeId ID of the biome
         * @param enabled True to enable, false to disable
         * @return True if biome was found
         */
        bool setBiomeEnabled(int biomeId, bool enabled);

        /**
         * @brief Check if a biome is enabled
         * @param biomeId ID of the biome
         * @return True if biome is enabled
         */
        bool isBiomeEnabled(int biomeId) const;

        /**
         * @brief Get biome at a position
         * @param position World position to check
         * @param context Generation context
         * @return Biome ID, -1 if no biome found
         */
        int getBiomeAt(const glm::vec3& position, const GenerationContext& context) const;

        /**
         * @brief Get biome influence at a position
         * @param position World position to check
         * @param context Generation context
         * @return Map of biome IDs to influence values [0.0-1.0]
         */
        std::unordered_map<int, float> getBiomeInfluence(const glm::vec3& position, const GenerationContext& context) const;

        /**
         * @brief Set biome blend distance
         * @param distance Blend distance in world units
         */
        void setBlendDistance(float distance);

        /**
         * @brief Get biomes blend distance
         * @return The blend distance
         */
        float getBlendDistance() const;

        /**
         * @brief Generate biome data for a chunk
         * @param chunkCoord Coordinate of the chunk
         * @param context Generation context
         * @param chunk Reference to the chunk for modification
         * @return True if generation was successful
         */
        bool generateBiomeData(const ChunkCoord& chunkCoord, const GenerationContext& context, Chunk& chunk);

        /**
         * @brief Set a temperature noise layer
         * @param noiseLayerId ID of the noise layer
         */
        void setTemperatureNoiseLayer(const std::string& noiseLayerId);

        /**
         * @brief Get the temperature noise layer ID
         * @return The temperature noise layer ID
         */
        std::string getTemperatureNoiseLayer() const;

        /**
         * @brief Set a humidity noise layer
         * @param noiseLayerId ID of the noise layer
         */
        void setHumidityNoiseLayer(const std::string& noiseLayerId);

        /**
         * @brief Get the humidity noise layer ID
         * @return The humidity noise layer ID
         */
        std::string getHumidityNoiseLayer() const;

        /**
         * @brief Apply a preset biome configuration
         * @param presetName Name of the preset
         * @return True if preset was found and applied
         */
        bool applyPreset(const std::string& presetName);

        /**
         * @brief Serialize the biome configuration
         * @param serializer Serializer to serialize with
         */
        void serialize(ECS::Serializer& serializer) const;

        /**
         * @brief Deserialize the biome configuration
         * @param deserializer Deserializer to deserialize with
         */
        void deserialize(ECS::Deserializer& deserializer);

    private:
        /**
         * @brief Retrieve the next available biome ID
         */
        int getNextBiomeId() const;

        /**
         * @brief Calculate the biome influence in a particular position
         * @param biomeId ID of the biome to evaluate
         * @param position Position to check biome influence
         * @param temperature Biome temperature
         * @param humidity Biome humidity
         * @param elevation Biome elevation
         * @param context Reference to generation context
         */
        float calculateBiomeInfluence(int biomeId, const glm::vec3& position,
                                      float temperature, float humidity, float elevation,
                                      const GenerationContext& context) const;

        // Member variables
        std::unordered_map<int, BiomeInfo> m_biomes;
        std::unordered_map<std::string, int> m_biomeNameToId;
        std::string m_temperatureNoiseLayer;
        std::string m_humidityNoiseLayer;
        float m_blendDistance = 8.0f;
    };

} // namespace PixelCraft::Voxel