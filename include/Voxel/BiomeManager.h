// -------------------------------------------------------------------------
// BiomeManager.h
// -------------------------------------------------------------------------
#pragma once

#include "Voxel/Biome.h"
#include "Voxel/ChunkCoord.h"

#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace PixelCraft::Utility::Serialization {
class Serializer;
class Deserializer;
struct SerializationResult;
class Schema;
} // namespace PixelCraft::Utility::Serialization

namespace PixelCraft::Voxel {

// Forward declarations
class Chunk;
struct GenerationContext;

/**
 * @brief Manages biome types and transitions
 *
 * Handles biome definition, placement, and blending between different biome
 * types during procedural generation.
 */
class BiomeManager {
  public:
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
    std::shared_ptr<Biome> registerBiome(const std::string &name);

    /**
     * @brief Remove a biome type
     * @param biomeId ID of the biome to remove
     * @return True if biome was found and removed
     */
    bool removeBiome(int biomeId);

    /**
     * @brief Get biome by ID
     * @param biomeId ID of the biome
     * @return Biome, nullptr if not found
     */
    std::shared_ptr<Biome> getBiome(int biomeId) const;

    /**
     * @brief Get biome by name
     * @param name Name of the biome
     * @return Biome, nullptr if not found
     */
    std::shared_ptr<Biome> getBiome(const std::string &name) const;

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
     * @brief Get biome at a position
     * @param position World position to check
     * @param context Generation context
     * @return Biome ID, -1 if no biome found
     */
    int getBiomeAt(const glm::vec3 &position, const GenerationContext &context) const;

    /**
     * @brief Get biome influence at a position
     * @param position World position to check
     * @param context Generation context
     * @return Map of biome IDs to influence values [0.0-1.0]
     */
    std::unordered_map<int, float> getBiomeInfluence(const glm::vec3 &position, const GenerationContext &context) const;

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
    bool generateBiomeData(const ChunkCoord &chunkCoord, const GenerationContext &context, Chunk &chunk);

    /**
     * @brief Set a temperature noise layer
     * @param noiseLayerId ID of the noise layer
     */
    void setTemperatureNoiseLayer(const std::string &noiseLayerId);

    /**
     * @brief Get the temperature noise layer ID
     * @return The temperature noise layer ID
     */
    std::string getTemperatureNoiseLayer() const;

    /**
     * @brief Set a humidity noise layer
     * @param noiseLayerId ID of the noise layer
     */
    void setHumidityNoiseLayer(const std::string &noiseLayerId);

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
    bool applyPreset(const std::string &presetName);

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
    float calculateBiomeInfluence(int biomeId, const glm::vec3 &position, float temperature, float humidity,
                                  float elevation, const GenerationContext &context) const;

    // Member variables
    std::unordered_map<int, std::shared_ptr<Biome>> m_biomes;
    std::unordered_map<std::string, int> m_biomeNameToId;
    std::string m_temperatureNoiseLayer;
    std::string m_humidityNoiseLayer;
    float m_blendDistance = 8.0f;
};

} // namespace PixelCraft::Voxel