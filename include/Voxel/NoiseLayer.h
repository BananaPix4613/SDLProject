// -------------------------------------------------------------------------
// NoiseLayer.h
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
class NoiseLayer {
  public:
    /**
     * @brief Types of noise algorithms
     */
    enum class NoiseType { Perlin, Simplex, Worley, Value, Cubic, WhiteNoise, Ridged, Billow, Voronoi, Domain };

    /**
     * @brief Fractal types for noise detail
     */
    enum class FractalType {
        None,
        FBM,          // Fractal Brownian Motion
        Rigid,        // Rigid multi-fractal
        Billow,       // Billowy noise
        MultiFractal, // Multi-fractal
        HybridMulti,  // Hybrid multi-fractal
        DeCarpentier  // De Carpentier's voxel terrain formula
    };

    /**
     * @brief Interpolation methods for noise
     */
    enum class InterpolationType { Linear, Cubic, Quintic, Hermite };

    /**
     * @brief Domain warp method
     */
    enum class WarpType { None, Basic, Derivative, Fractal };

    /**
     * @brief Noise layer combining operations
     */
    enum class CombineOperation { Add, Subtract, Multiply, Divide, Min, Max, Power, Average, Blend };

    NoiseLayer() : m_name("") {}

    NoiseLayer(const std::string &name, bool enabled = true, NoiseType noiseType = NoiseType::Perlin,
               FractalType fractalType = FractalType::FBM, InterpolationType interpolation = InterpolationType::Quintic,
               CombineOperation combineOp = CombineOperation::Add, float frequency = 1.0f, float amplitude = 1.0f,
               int octaves = 4, float persistence = 0.5f, float lacunarity = 2.0f, glm::vec3 offset = glm::vec3(0.0f),
               glm::vec3 scale = glm::vec3(1.0f), float weight = 1.0f, WarpType warpType = WarpType::None,
               float warpAmplitude = 1.0f, float warpFrequency = 1.0f, bool warpEnabled = false) {
        m_name = name;
        m_enabled = enabled;
        m_noiseType = noiseType;
        m_fractalType = fractalType;
        m_interpolation = interpolation;
        m_combineOp = combineOp;
        m_frequency = frequency;
        m_amplitude = amplitude;
        m_octaves = octaves;
        m_persistence = persistence;
        m_lacunarity = lacunarity;
        m_offset = offset;
        m_scale = scale;
        m_weight = weight;
        m_warpType = warpType;
        m_warpAmplitude = warpAmplitude;
        m_warpFrequency = warpFrequency;
        m_warpEnabled = warpEnabled;
    }

    /**
     * @brief Set a layer's name
     * @param name Layer's name
     */
    void setLayerName(const std::string &name);

    /**
     * @brief Get a layer's name
     * @return Layer's name
     */
    const std::string &getLayerName() const;

    /**
     * @brief Set a layer's enabled state
     * @param enabled True to enable, false to disable
     */
    void setLayerEnabled(bool enabled);

    /**
     * @brief Check if a layer is enabled
     * @return True if layer exists and is enabled
     */
    bool isLayerEnabled() const;

    /**
     * @brief Set a layer's noise type
     * @param type The noise type to use
     */
    void setLayerNoiseType(NoiseType type);

    /**
     * @brief Get a layer's noise type
     * @return Layer's noise type
     */
    NoiseType getLayerNoiseType() const;

    /**
     * @brief Set a layer's fractal type
     * @param type The fractal type to use
     */
    void setLayerFractalType(FractalType type);

    /**
     * @brief Get a layer's fractal type
     * @return Layer's fractal type
     */
    FractalType getLayerFractalType() const;

    /**
     * @brief Set a layer's frequency
     * @param frequency The frequency value
     */
    void setLayerFrequency(float frequency);

    /**
     * @brief Get a layer's frequency
     * @return Layer's frequency
     */
    float getLayerFrequency() const;

    /**
     * @brief Set a layer's amplitude
     * @param amplitude The amplitude value
     */
    void setLayerAmplitude(float amplitude);

    /**
     * @brief Get a layer's amplitude
     * @param Layer's amplitude
     */
    float getLayerAmplitude() const;

    /**
     * @brief Set a layer's octaves
     * @param octaves The octave count
     */
    void setLayerOctaves(int octaves);

    /**
     * @brief Get a layer's octaves
     * @return Layer's octaves
     */
    int getLayerOctaves() const;

    /**
     * @brief Set a layer's persistence
     * @param persistence The persistence value
     */
    void setLayerPersistence(float persistence);

    /**
     * @brief Get a layer's persistence
     * @return Layer's persistence
     */
    float getLayerPersistence() const;

    /**
     * @brief Set a layer's lacunarity
     * @param lacunarity The lacunarity value
     */
    void setLayerLacunarity(float lacunarity);

    /**
     * @brief Get a layer's lacunarity
     * @return Layer's lacunarity
     */
    float getLayerLacunarity() const;

    /**
     * @brief Set a layer's offset
     * @param offset The offset vector
     */
    void setLayerOffset(const glm::vec3 &offset);

    /**
     * @brief Get a layer's offset
     * @return Layer's offset
     */
    const glm::vec3 &getLayerOffset() const;

    /**
     * @brief Set a layer's scale
     * @param scale The scale vector
     */
    void setLayerScale(const glm::vec3 &scale);

    /**
     * @brief Get a layer's scale
     * @return Layer's scale
     */
    const glm::vec3 &getLayerScale() const;

    /**
     * @brief Set a layer's weight in the final result
     * @param weight The weight value [0.0-1.0]
     */
    void setLayerWeight(float weight);

    /**
     * @brief Get a layer's weight
     * @return Layer's weight value
     */
    float getLayerWeight() const;

    /**
     * @brief Set a layer's warp type
     * @param type The warp type
     */
    void setWarpType(WarpType type);

    /**
     * @brief Get a layer's warp type
     * @return Layer's warp type
     */
    WarpType getWarpType() const;

    /**
     * @brief Set a layer's warp amplitude
     * @param amplitude Warp amplitude
     */
    void setWarpAmplitude(float amplitude);

    /**
     * @brief Get a layer's warp amplitude
     * @return Layer's warp amplitude
     */
    float getWarpAmplitude() const;

    /**
     * @brief Set a layer's warp frequency
     * @param frequency Warp frequency
     */
    void setWarpFrequency(float frequency);

    /**
     * @brief Get a layer's warp frequency
     * @return Layer's warp frequency
     */
    float getWarpFrequency() const;

    /**
     * @brief Set a layer's warp enabled
     * @param enabled Warp enabled
     */
    void setWarpEnabled(bool enabled);

    /**
     * @brief Get a layer's warp enabled
     * @return Layer's warp enabled
     */
    bool isWarpEnabled() const;

    /**
     * @brief Set the operation used to combine this layer with previous layers
     * @param operation The combine operation
     */
    void setLayerCombineOperation(CombineOperation operation);

    /**
     * @brief Get a layer's combine operation
     * @return Layer's combine operation
     */
    CombineOperation getLayerCombineOperation() const;

    /**
     * @brief Serialize the layer configuration to the given serializer
     * @param serializer The serializer to use
     * @return SerializationResult with success/failure status
     */
    Utility::Serialization::SerializationResult serialize(Utility::Serialization::Serializer &serializer) const;

    /**
     * @brief Deserialize the layer configuration from the given deserializer
     * @param deserializer The deserializer to use
     * @return SerializationResult with success/failure status
     */
    Utility::Serialization::SerializationResult deserialize(Utility::Serialization::Deserializer &deserializer);

    /**
     * @brief Define the schema for layer info serialization
     * @param schema The schema to define
     */
    static void defineSchema(Utility::Serialization::Schema &schema);

    /**
     * @brief Get the type name for serialization
     * @return The type name string
     */
    static std::string getTypeName();

  private:
    std::string m_name;                                             ///< Name of the noise layer
    int m_id = 0;                                                   ///< ID of the noise layer
    bool m_enabled = true;                                          ///< Whether the noise layer is enabled or not
    NoiseType m_noiseType = NoiseType::Perlin;                      ///< Noise type for the noise layer
    FractalType m_fractalType = FractalType::FBM;                   ///< Fractal type for the noise layer
    InterpolationType m_interpolation = InterpolationType::Quintic; ///< Interpolation type for the noise layer
    CombineOperation m_combineOp = CombineOperation::Add;           ///< Combine operation for the noise layer
    float m_frequency = 1.0f;                                       ///< Frequency of the noise layer
    float m_amplitude = 1.0f;                                       ///< Amplitude of the noise layer
    int m_octaves = 4;                                              ///< Octaves of the noise layer
    float m_persistence = 0.5f;                                     ///< Persistence of the noise layer
    float m_lacunarity = 2.0f;                                      ///< Lacunarity of the noise layer
    glm::vec3 m_offset = glm::vec3(0.0f);                           ///< Offset of the noise layer
    glm::vec3 m_scale = glm::vec3(1.0f);                            ///< Scale of the noise layer
    float m_weight = 1.0f;                                          ///< Weight of the noise layer
    WarpType m_warpType = WarpType::None;                           ///< Warp type of the noise layer
    float m_warpAmplitude = 1.0f;                                   ///< Warp amplitude of the noise layer
    float m_warpFrequency = 1.0f;                                   ///< Warp frequency of the noise layer
    bool m_warpEnabled = false;                                     ///< Warp enabled of the noise layer
};

} // namespace PixelCraft::Voxel