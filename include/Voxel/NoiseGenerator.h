// -------------------------------------------------------------------------
// NoiseGenerator.h
// -------------------------------------------------------------------------
#pragma once

#include "Voxel/NoiseLayer.h"

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

/**
 * @brief Advanced noise generation for procedural content
 *
 * Provides multiple noise algorithms, layering capabilities, and transformation
 * options for generating complex terrain and other procedural elements.
 */
class NoiseGenerator {
  public:
    /**
     * @brief Constructor
     */
    NoiseGenerator();

    /**
     * @brief Destructor
     */
    virtual ~NoiseGenerator();

    /**
     * @brief Generate a noise value at specified coordinates
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @return The generated noise value [-1,1]
     */
    float generate(float x, float y, float z) const;

    /**
     * @brief Generate a noise value at specified coordinates
     * @parma position 3D position vector
     * @return The generated noise value [-1,1]
     */
    float generate(const glm::vec3 &position) const;

    /**
     * @brief Generate a 2D noise value at specified coordinates
     * @param x X coordinate
     * @param y Y coordinate
     * @return The generated noise value [-1,1]
     */
    float generate2D(float x, float y) const;

    /**
     * @brief Set the seed for noise generation
     * @param seed The new seed value
     */
    void setSeed(uint32_t seed);

    /**
     * @brief Get the current seed
     * @return The current seed value
     */
    uint32_t getSeed() const;

    /**
     * @brief Create a new noise layer
     * @param name Unique name for the layer
     * @return True if layer was created successfully
     */
    std::shared_ptr<NoiseLayer> createLayer(const std::string &name);

    /**
     * @brief Remove a noise layer
     * @param name Name of the layer to remove
     * @return True if layer was found and removed
     */
    bool removeLayer(int layerId);

    /**
     * @brief Get the names of all layers
     * @return Vector of layer names
     */
    std::vector<std::string> getLayerNames() const;

    /**
     * @brief Get the number of layers
     * @return The layer count
     */
    size_t getLayerCount() const;

    /**
     * @brief Set a custom modifier function to apply after noise generation
     * @param modifier Function that takes a noise value and returns a modified value
     */
    void setModifier(std::function<float(float)> modifier);

    /**
     * @brief Clear the custom modifier function
     */
    void clearModifier();

    /**
     * @brief Preset for terrain height map
     * @param mountainous If true, more dramatic elevation changes
     * @return True if preset was applied successfully
     */
    bool presetTerrain(bool mountainous = false);

    /**
     * @brief Preset for cave generation
     * @return True if preset was applied successfully
     */
    bool presetCaves();

    /**
     * @brief Preset for ore/resource distribution
     * @return True if preset was applied successfully
     */
    bool presetOreDistribution();

    /**
     * @brief Preset for biome blending
     * @return True if preset was applied successfully
     */
    bool presetBiomeBlend();

    /**
     * @brief Preset for detailed texture variation
     * @return True if preset was applied successfully
     */
    bool presetDetailTexture();

    /**
     * @brief Serialize the generator to the given serializer
     * @param serializer The serializer to use
     * @return SerializationResult with success/failure status
     */
    Utility::Serialization::SerializationResult serialize(Utility::Serialization::Serializer &serializer) const;

    /**
     * @brief Deserialize the generator from the given deserializer
     * @param deserializer The deserializer to use
     * @return SerializationResult with success/failure status
     */
    Utility::Serialization::SerializationResult deserialize(Utility::Serialization::Deserializer &deserializer);

    /**
     * @brief Define the schema for generator serialization
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
     * @brief Retrieve the next available layer ID
     */
    int getNextLayerId() const;

    /**
     * @brief Generate for a single coordinate
     * @param type Noise type to utilize
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @returns idk a float I guess
     */
    float generateSingle(NoiseLayer::NoiseType type, float x, float y, float z,
                         NoiseLayer::InterpolationType interpType = NoiseLayer::InterpolationType::Linear) const;

    /**
     * @brief Generate for a single coordinate with fractal
     * @param noiseType Noise type to utilize
     * @param fractalType Fractal type to utilize
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @param octaves The amount of octaves to utilize in fractal
     * @param persistence The persistence of the fractal
     * @param lacunarity The lacunarity of the fractal
     * @returns idk a float I guess
     */
    float generateFractal(NoiseLayer::NoiseType noiseType, NoiseLayer::FractalType fractalType, float x, float y,
                          float z, int octaves, float persistence, float lacunarity) const;

    /**
     * @brief Computes the warp for a single coordinate
     * @param type Warp type to compute for
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @param amplitude Amplitude of the warp to calculate with
     * @param frequency Frequency of the warp to calculate with
     * @returns idk a float I guess
     */
    float computeWarp(NoiseLayer::WarpType type, float x, float y, float z, float amplitude, float frequency) const;

    /**
     * @brief Interpolate between values by interpolation type
     * @param InterpolationType Interpolation type to utilize
     * @param a Starting value
     * @param b End value
     * @param t Time to step by
     * @returns Next interpolated value
     */
    float interpolate(NoiseLayer::InterpolationType type, float a, float b, float t) const;

    /**
     * @brief Combine two values together using a combine operation by weight
     * @param a First value
     * @param b Second value
     * @param op Combine operation to use
     * @param weight Weight to combine by
     */
    float combineValues(float a, float b, NoiseLayer::CombineOperation op, float weight) const;

    /**
     * @brief Apply scale and offset to particular point
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @param scale Vector scale to apply
     * @param offset Vector offset to apply
     * @returns Vector point with applied offset and scale
     */
    glm::vec3 applyScaleAndOffset(float x, float y, float z, const glm::vec3 &scale, const glm::vec3 &offset) const;

    /**
     * @brief Validate the parameters of the generator
     * @param layer Noise layer
     */
    void validateParameters();

    /**
     * @brief Helper function for value noise
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @return Calculated value noise
     */
    float valueNoise(int x, int y, int z) const;

    // Private member variables
    uint32_t m_seed = 42;

    std::unordered_map<int, std::shared_ptr<NoiseLayer>> m_layers;
    std::unordered_map<std::string, int> m_layerNameToId;
    std::function<float(float)> m_modifier;
};

} // namespace PixelCraft::Voxel