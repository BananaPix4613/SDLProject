// -------------------------------------------------------------------------
// GenerationParameters.h
// -------------------------------------------------------------------------
#pragma once

#include "ECS/Serializer.h"
#include "ECS/Deserializer.h"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

namespace PixelCraft::Voxel
{

    /**
     * @brief Configuration parameters for procedural generation
     * 
     * Holds all configuration parameters for procedural generation, allowing for
     * serialization and reuse of generation settings.
     */
    class GenerationParameters
    {
    public:
        /**
         * @brief Constructor
         */
        GenerationParameters();

        /**
         * @brief Destructor
         */
        virtual ~GenerationParameters();

        /**
         * @brief Terrain generation mode
         */
        enum class TerrainMode
        {
            Flat,           // Flat terrain at fixed height
            HeightMap,      // Height map based terrain
            Volumetric,     // Full 3D volumetric terrain with caves
            Islands,        // Floating islands
            Mountains,      // Mountainous terrain
            Desert,         // Desert with dunes
            Custom          // Custom generation mode
        };

        /**
         * @brief Feature placement pattern
         */
        enum class PlacementPattern
        {
            Random,         // Random distribution
            Grid,           // Regular grid
            Clustered,      // Clustering of features
            Biome,          // Biome-based distribution
            Landmark,       // Near landmarks
            EdgeClustered,  // Clustered near edges
            Custom          // Custom placement pattern
        };

        /**
         * @brief Set the terrain generation mode
         * @param mode The terrain mode to use
         */
        void setTerrainMode(TerrainMode mode);

        /**
         * @brief Get the current terrain mode
         * @return The current terrain mode
         */
        TerrainMode getTerrainMode() const;

        /**
         * @brief Set the height range for terrain
         * @param min Minimum height
         * @param max Maximum height
         */
        void setHeightRange(float min, float max);

        /**
         * @brief Get the minimum terrain height
         * @return The minimum height
         */
        float getMinHeight() const;

        /**
         * @brief Get the maximum terrain height
         * @return The maximum height
         */
        float getMaxHeight() const;

        /**
         * @brief Set water level
         * @param level The water level height
         */
        void setWaterLevel(float level);

        /**
         * @brief Get the water level
         * @return The water level height
         */
        float getWaterLevel() const;

        /**
         * @brief Enable or disable water
         * @param enable True to enable water, false to disable
         */
        void enableWater(bool enable);

        /**
         * @brief Check if water is enabled
         * @return True if water is enabled
         */
        bool isWaterEnabled() const;

        /**
         * @brief Enable or disable caves
         * @param enable True to enable caves, false to disable
         */
        void enableCaves(bool enable);

        /**
         * @brief Check if caves are enabled
         * @return True if caves are enabled
         */
        bool areCavesEnabled() const;

        /**
         * @brief Set the cave density factor
         * @param density Density factor [0.0-1.0]
         */
        void setCaveDensity(float density);

        /**
         * @brief Get the cave density factor
         * @return The cave density factor
         */
        float getCaveDensity() const;

        /**
         * @brief Set the cave size
         * @param size Size factor for caves [0.1-10.0]
         */
        void setCaveSize(float size);

        /**
         * @brief Get the cave size
         * @return The cave size factor
         */
        float getCaveSize() const;

        /**
         * @brief Set the noise layer to use for terrain height
         * @param layerId ID of the noise layer
         */
        void setTerrainNoiseLayer(const std::string& layerId);

        /**
         * @brief Get the terrain noise layer ID
         * @return The terrain noise layer ID
         */
        std::string getTerrainNoiseLayer() const;

        /**
         * @brief Set the noise layer to use for cave generation
         * @param layerId ID of the noise layer
         */
        void setCaveNoiseLayer(const std::string& layerId);

        /**
         * @brief Get the cave noise layer ID
         * @return The cave noise layer ID
         */
        std::string getCaveNoiseLayer() const;

        /**
         * @brief Set the noise layer to use for feature placement
         * @param layerId ID of the noise layer
         */
        void setFeatureNoiseLayer(const std::string& layerId);

        /**
         * @brief Get the feature noise layer ID
         * @return The feature noise layer ID
         */
        std::string getFeatureNoiseLayer() const;

        /**
         * @brief Set the noise layer to use for biome distribution
         * @param layerId ID of the noise layer
         */
        void setBiomeNoiseLayer(const std::string& layerId);

        /**
         * @brief Get the biome noise layer ID
         * @return The biome noise layer ID
         */
        std::string getBiomeNoiseLayer() const;

        /**
         * @brief Set the terrain roughness
         * @param roughness Roughness factor [0.0-1.0]
         */
        void setTerrainRoughness(float roughness);

        /**
         * @brief Get the terrain roughness
         * @return The terrain roughness factor
         */
        float getTerrainRoughness() const;

        /**
         * @brief Set the terrain erosion factor
         * @param erosion Erosion factor [0.0-1.0]
         */
        void setTerrainErosion(float erosion);

        /**
         * @brief Get the terrain erosion factor
         * @return The terrain erosion factor
         */
        float getTerrainErosion() const;

        /**
         * @brief Set the feature placement pattern
         * @param pattern The placement pattern to use
         */
        void setFeaturePlacementPattern(PlacementPattern pattern);

        /**
         * @brief Get the feature placement pattern
         * @return The feature placement pattern
         */
        PlacementPattern getFeaturePlacementPattern() const;

        /**
         * @brief Set the feature density
         * @param density Density factor [0.0-1.0]
         */
        void setFeatureDensity(float density);

        /**
         * @brief Get the feature density
         * @return The feature density factor
         */
        float getFeatureDensity() const;

        /**
         * @brief Set the biome blend distance
         * @param distance Blend distance in world units
         */
        void setBiomeBlendDistance(float distance);

        /**
         * @brief Get the biome blend distance
         * @return The biome blend distance
         */
        float getBiomeBlendDistance() const;

        /**
         * @brief Set the feature placement control to use
         * @param controlId ID of the feature placement control
         */
        void setFeaturePlacementControl(const std::string& controlId);

        /**
         * @brief Get the feature placement control ID
         * @return The feature placement control ID
         */
        std::string getFeaturePlacementControl() const;

        /**
         * @brief Set the distribution control to use
         * @param controlId ID of the distribution control
         */
        void setDistributionControl(const std::string& controlId);

        /**
         * @brief Get the distribution control ID
         * @return The distribution control ID
         */
        std::string getDistributionControl() const;

        /**
         * @brief Enable or disable a specific feature type
         * @param featureType Type name of the feature
         * @param enable True to enable, false to disable
         */
        void enableFeatureType(const std::string& featureType, bool enable);

        /**
         * @brief Check if a feature type is enabled
         * @param featureType Type name of the feature
         * @return True if the feature type is enabled
         */
        bool isFeatureTypeEnabled(const std::string& featureType) const;

        /**
         * @brief Set the density for a specific feature type
         * @param featureType Type name of the feature
         * @param density Density factor [0.0-1.0]
         */
        void setFeatureTypeDensity(const std::string& featureType, float density);

        /**
         * @brief Get the density for a feature type
         * @param featureType Type name of the feature
         * @param defaultValue Default value if not found
         * @return The feature type density
         */
        float getFeatureTypeDensity(const std::string& featureType, float defaultValue = 0.5f) const;

        /**
         * @brief Set a float parameter
         * @param name name of the parameter
         * @param value Value to set
         */
        void setFloatParam(const std::string& name, float value);

        /**
         * @brief Get a float parameter
         * @param name Name of the parameter
         * @param defaultValue Default value if not found
         * @return The parameter value
         */
        float getFloatParam(const std::string& name, float defaultValue = 0.0f) const;

        /**
         * @brief Set an integer parameter
         * @param name Name of the parameter
         * @param value Value to set
         */
        void setIntParam(const std::string& name, int value);

        /**
         * @brief Get an integer parameter
         * @param name Name of the parameter
         * @param defaultValue Default value if not found
         * @return The parameter value
         */
        int getIntParam(const std::string& name, int defaultValue = 0) const;

        /**
         * @brief Set a boolean parameter
         * @param name Name of the parameter
         * @param value Value to set
         */
        void setBoolParam(const std::string& name, bool value);

        /**
         * @brief Get a boolean parameter
         * @param name Name of the parameter
         * @param defaultValue Default value if not found
         * @return The parameter value
         */
        bool getBoolParam(const std::string& name, bool defaultValue = false) const;

        /**
         * @brief Set a string parameter
         * @param name Name of the parameter
         * @param value Value to set
         */
        void setStringParam(const std::string& name, const std::string& value);

        /**
         * @brief Get a string parameter
         * @param name Name of the parameter
         * @param defaultValue Default value if not found
         * @return The parameter value
         */
        std::string getStringParam(const std::string& name, const std::string& defaultValue = "") const;

        /**
         * @brief Set a vector parameter
         * @param name Name of the parameter
         * @param value Value to set
         */
        void setVec3Param(const std::string& name, const glm::vec3& value);

        /**
         * @brief Get a vector parameter
         * @param name Name of the parameter
         * @param defaultValue Default value if not found
         * @return The parameter value
         */
        glm::vec3 getVec3Param(const std::string& name, const glm::vec3& defaultValue = glm::vec3(0.0f)) const;

        /**
         * @brief Apply preset parameters for a specific terrain type
         * @param presetName Name of the preset
         * @return True if preset was applied successfully
         */
        bool applyPreset(const std::string& presetName);

        /**
         * @brief Serialize the parameters
         * @param serializer Serializer to serialize with
         */
        void serialize(ECS::Serializer& serializer) const;

        /**
         * @brief Deserialize the parameters
         * @param deserializer Deserializer to deserialize with
         */
        void deserialize(ECS::Deserializer& deserializer);

    private:
        /**
         * @brief Initializes default values
         */
        void initializeDefaults();

        // Member variables
        TerrainMode m_terrainMode = TerrainMode::HeightMap;
        float m_minHeight = 0.0f;
        float m_maxHeight = 100.0f;
        float m_waterLevel = 50.0f;
        bool m_waterEnabled = true;
        bool m_cavesEnabled = true;
        float m_caveDensity = 0.5f;
        float m_caveSize = 1.0f;
        std::string m_terrainNoiseLayer;
        std::string m_caveNoiseLayer;
        std::string m_featureNoiseLayer;
        std::string m_biomeNoiseLayer;
        float m_terrainRoughness = 0.5f;
        float m_terrainErosion = 0.1f;
        PlacementPattern m_featurePlacementPattern = PlacementPattern::Random;
        float m_featureDensity = 0.5f;
        float m_biomeBlendDistance = 8.0f;
        std::string m_featurePlacementControl;
        std::string m_distributionControl;

        std::unordered_map<std::string, bool> m_enabledFeatureTypes;
        std::unordered_map<std::string, float> m_featureTypeDensities;
        std::unordered_map<std::string, float> m_floatParams;
        std::unordered_map<std::string, int> m_intParams;
        std::unordered_map<std::string, bool> m_boolParams;
        std::unordered_map<std::string, std::string> m_stringParams;
        std::unordered_map<std::string, glm::vec3> m_vec3Params;
    };

} // namespace PixelCraft::Voxel