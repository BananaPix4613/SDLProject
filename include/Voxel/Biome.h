// -------------------------------------------------------------------------
// Biome.h
// -------------------------------------------------------------------------
#pragma once

#include "glm/glm.hpp"

#include <string>
#include <unordered_map>

namespace PixelCraft::Utility::Serialization {
class Serializer;
class Deserializer;
struct SerializationResult;
class Schema;
} // namespace PixelCraft::Utility::Serialization

namespace PixelCraft::Voxel {

/**
 * @brief Contains biome definition and serialization methods
 */
class Biome {
  public:
    Biome()
        : m_name(""), m_id(0), m_threshold(0.5f), m_weight(1.0f), m_enabled(true),
          m_temperatureRange(glm::vec2(0.0f, 1.0f)), m_humidityRange(glm::vec2(0.0f, 1.0f)),
          m_elevationRange(glm::vec2(0.0f, 1.0f)) {}

    Biome(const std::string &name, int id)
        : m_name(name), m_id(id), m_threshold(0.5f), m_weight(1.0f), m_enabled(true),
          m_temperatureRange(glm::vec2(0.0f, 1.0f)), m_humidityRange(glm::vec2(0.0f, 1.0f)),
          m_elevationRange(glm::vec2(0.0f, 1.0f)) {}

    /**
     * @brief Get name of the biome
     * @returns Name of the biome
     */
    const std::string &getBiomeName() const;

    /**
     * @brief Get ID of the biome
     * @returns The ID of the biome
     */
    int getBiomeId() const;

    /**
     * @brief Set biome noise layer
     * @param noiseLayerId ID of the noise layer
     * @param threshold Threshold value for biome boundary [0.0-1.0]
     */
    void setBiomeNoise(const std::string &noiseLayerId, float threshold);

    /**
     * @brief Get biome noise layer
     * @param noiseLayerId Output ID of the noise layer
     * @param threshold Output threshold value for biome boundary [0.0-1.0]
     */
    void getBiomeNoise(std::string &noiseLayerId, float &threshold);

    /**
     * @brief Set biome temperature range
     * @param minTemp Minimum temperature
     * @param maxTemp Maximum temperature
     */
    void setBiomeTemperatureRange(float minTemp, float maxTemp);

    /**
     * @brief Get biome temperature range
     * @return Biome temperature range
     */
    glm::vec2 getBiomeTemperatureRange();

    /**
     * @brief Set biome humidity range
     * @param minHumidity Minimum humidity
     * @param maxHumidity Maximum humidity
     */
    void setBiomeHumidityRange(float minHumidity, float maxHumidity);

    /**
     * @brief Get biome humidity range
     * @return Biome humidity range
     */
    glm::vec2 getBiomeHumidityRange();

    /**
     * @brief Set biome elevation range
     * @param minElevation Minimum elevation
     * @param maxElevation Maximum elevation
     */
    void setBiomeElevationRange(float minElevation, float maxElevation);

    /**
     * @brief Get biome elevation range
     * @return Biome elevation range
     */
    glm::vec2 getBiomeElevationRange();

    /**
     * @brief Set probability for a biome voxel type
     * @param voxelType Type of voxel
     * @param probability Probability [0.0-1.0]
     */
    void setVoxelProbability(const std::string &voxelType, float probability);

    /**
     * @brief Get probability for a biome voxel type
     * @param voxelType Type of voxel
     * @return Probability value, 0 if not found
     */
    float getVoxelProbability(const std::string &voxelType) const;

    /**
     * @brief Get all probabilities for a biome
     * @return Map of probabilities
     */
    std::unordered_map<std::string, float> getVoxelProbabilities() const;

    /**
     * @brief Set probability for a biome feature type
     * @param featureType Type of feature
     * @param probability Probability [0.0-1.0]
     */
    void setFeatureProbability(const std::string &featureType, float probability);

    /**
     * @brief Get probability for a biome feature type
     * @param featureType Type of feature
     * @return Probability value, 0 if not found
     */
    float getFeatureProbability(const std::string &featureType) const;

    /**
     * @brief Set biome weight (importance during blending)
     * @param weight Weight value [0.0-1.0]
     */
    void setBiomeWeight(float weight);

    /**
     * @brief Get biome weight
     * @return Weight value, 0 if not found
     */
    float getBiomeWeight() const;

    /**
     * @brief Enable or disable biome
     * @param enabled True to enable, false to disable
     */
    void setBiomeEnabled(bool enabled);

    /**
     * @brief Check if biome is enabled
     * @return True if biome is enabled
     */
    bool isBiomeEnabled() const;

    /**
     * @brief Serialize the biome configuration to the given serializer
     * @param serializer The serializer to use
     * @return SerializationResult with success/failure status
     */
    Utility::Serialization::SerializationResult serialize(Utility::Serialization::Serializer &serializer) const;

    /**
     * @brief Deserialize the biome configuration from the given deserializer
     * @param deserializer The deserializer to use
     * @return SerializationResult with success/failure status
     */
    Utility::Serialization::SerializationResult deserialize(Utility::Serialization::Deserializer &deserializer);

    /**
     * @brief Define the schema for biome info serialization
     * @param schema The schema to define
     */
    static void defineSchema(Utility::Serialization::Schema &schema);

    /**
     * @brief Get the type name for serialization
     * @return The type name string
     */
    static std::string getTypeName();

  private:
    std::string m_name;                                            ///< Name for this biome
    int m_id;                                                      ///< ID for this biome
    std::string m_noiseLayerId;                                    ///< Noise layer ID for this biome
    float m_threshold;                                             ///< Threshold for this biome to take influence
    float m_weight;                                                ///< Weight of influence for this biome
    bool m_enabled;                                                ///< Whether the biome is enabled or not
    glm::vec2 m_temperatureRange;                                  ///< Min/max temperature for this biome
    glm::vec2 m_humidityRange;                                     ///< Min/max humidity for this biome
    glm::vec2 m_elevationRange;                                    ///< Min/max elevation for this biome
    std::unordered_map<std::string, float> m_voxelProbabilities;   ///< Voxel type to probability mapping
    std::unordered_map<std::string, float> m_featureProbabilities; ///< Feature type to probability mapping
};

} // namespace PixelCraft::Voxel