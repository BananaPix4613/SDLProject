// -------------------------------------------------------------------------
// NoiseGenerator.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <glm/glm.hpp>

namespace PixelCraft::Voxel
{

    /**
     * @brief Advanced noise generation for procedural content
     * 
     * Provides multiple noise algorithms, layering capabilities, and transformation
     * options for generating complex terrain and other procedural elements.
     */
    class NoiseGenerator
    {
    public:
        /**
         * @brief Types of noise algorithms
         */
        enum class NoiseType
        {
            Perlin,
            Simplex,
            Worley,
            Value,
            Cubic,
            WhiteNoise,
            Ridged,
            Billow,
            Voronoi,
            Domain
        };

        /**
         * @brief Fractal types for noise detail
         */
        enum class FractalType
        {
            None,
            FBM,         // Fractal Brownian Motion
            Rigid,       // Rigid multi-fractal
            Billow,      // Billowy noise
            MultiFractal,// Multi-fractal
            HybridMulti, // Hybrid multi-fractal
            DeCarpentier // De Carpentier's voxel terrain formula
        };

        /**
         * @brief Interpolation methods for noise
         */
        enum class InterpolationType
        {
            Linear,
            Cubic,
            Quintic,
            Hermite
        };

        /**
         * @brief Domain warp method
         */
        enum class WarpType
        {
            None,
            Basic,
            Derivative,
            Fractal
        };

        /**
         * @brief Noise layer combining operations
         */
        enum class CombineOperation
        {
            Add,
            Subtract,
            Multiply,
            Divide,
            Min,
            Max,
            Power,
            Average,
            Blend
        };

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
        float generate(const glm::vec3& position) const;

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
         * @brief Set the noise type
         * @param type The noise algorithm type to use
         */
        void setNoiseType(NoiseType type);

        /**
         * @brief Get the current noise type
         * @return The current noise algorithm type
         */
        NoiseType getNoiseType() const;

        /**
         * @brief Set the fractal type
         * @param type The fractal algorithm to use
         */
        void setFractalType(FractalType type);

        /**
         * @brief Get the current fractal type
         * @return The current fractal type
         */
        FractalType getFractalType() const;

        /**
         * @brief Set the number of octaves for fractal noise
         * @param octaves Number of octaves [1-10]
         */
        void setOctaves(int octaves);

        /**
         * @brief Get the current number of octaves
         * @return The current octave count
         */
        int getOctaves() const;

        /**
         * @brief Set the lacunarity (frequency multiplier between octaves)
         * @param lacunarity The lacunarity value [1.0-4.0]
         */
        void setLacunarity(float lacunarity);

        /**
         * @brief Get the current lacunarity
         * @return The current lacunarity value
         */
        float getLacunarity() const;

        /**
         * @brief Set the persistence (amplitude multiplier between octaves)
         * @param persistence The persistence value [0.0-1.0]
         */
        void setPersistence(float persistence);

        /**
         * @brief Get the current persistence
         * @return The current persistence value
         */
        float getPersistence() const;

        /**
         * @brief Set the frequency of the noise
         * @param frequency The frequency value
         */
        void setFrequency(float frequency);

        /**
         * @brief Get the current frequency
         * @return The current frequency value
         */
        float getFrequency() const;

        /**
         * @brief Set the amplitude of the noise
         * @param amplitude The amplitude value
         */
        void setAmplitude(float amplitude);

        /**
         * @brief Get the current amplitude
         * @return The current amplitude value
         */
        float getAmplitude() const;

        /**
         * @brief Set a scale factor for the coordinates
         * @param scale The scale vector (separate scale for each axis)
         */
        void setScale(const glm::vec3& scale);

        /**
         * @brief Get the current scale factor
         * @return The current scale vector
         */
        glm::vec3 getScale() const;

        /**
         * @brief Set an offset for the coordinates
         * @param offset The offset vector
         */
        void setOffset(const glm::vec3& offset);

        /**
         * @brief Get the current offset
         * @return The current offset vector
         */
        glm::vec3 getOffset() const;

        /**
         * @brief Enable or disable domain warping
         * @param enable True to enable, false to disable
         */
        void enableWarp(bool enable);

        /**
         * @brief Check if warping is enabled
         * @return True if enabled
         */
        bool isWarpEnabled() const;

        /**
         * @brief Set the domain warp type
         * @param type The warp type to use
         */
        void setWarpType(WarpType type);

        /**
         * @brief Get the current warp type
         * @return The current warp type
         */
        WarpType getWarpType() const;

        /**
         * @brief Set the warp amplitude
         * @param amplitude The warp amplitude
         */
        void setWarpAmplitude(float amplitude);

        /**
         * @brief Get the current warp amplitude
         * @return The current warp amplitude
         */
        float getWarpAmplitude() const;

        /**
         * @brief Set the warp frequency
         * @param frequency The warp frequency
         */
        void setWarpFrequency(float frequency);

        /**
         * @brief Get the current warp frequency
         * @return The current warp frequency
         */
        float getWarpFrequency() const;

        /**
         * @brief Set the interpolation method
         * @param interp The interpolation type
         */
        void setInterpolation(InterpolationType interp);

        /**
         * @brief Get the current interpolation method
         * @return The current interpolation type
         */
        InterpolationType getInterpolation() const;

        /**
         * @brief Create a new noise layer
         * @param name Unique name for the layer
         * @return True if layer was created successfully
         */
        bool createLayer(const std::string& name);

        /**
         * @brief Remove a noise layer
         * @param name Name of the layer to remove
         * @return True if layer was found and removed
         */
        bool removeLayer(const std::string& name);

        /**
         * @brief Set a layer's enabled state
         * @param name Name of the layer
         * @param enabled True to enable, false to disable
         * @return True if layer was found
         */
        bool setLayerEnabled(const std::string& name, bool enabled);

        /**
         * @brief Check if a layer is enabled
         * @param name Name of the layer
         * @return True if layer exists and is enabled
         */
        bool isLayerEnabled(const std::string& name) const;

        /**
         * @brief Set a layer's noise type
         * @param name Name of the layer
         * @param type The noise type to use
         * @return True if layer was found
         */
        bool setLayerNoiseType(const std::string& name, NoiseType type);

        /**
         * @brief Set a layer's fractal type
         * @param name Name of the layer
         * @param type The fractal type to use
         * @return True if layer was found
         */
        bool setLayerFractalType(const std::string& name, FractalType type);

        /**
         * @brief Set a layer's frequency
         * @param name Name of the layer
         * @param frequency The frequency value
         * @return True if layer was found
         */
        bool setLayerFrequency(const std::string& name, float frequency);

        /**
         * @brief Set a layer's amplitude
         * @param name Name of the layer
         * @param amplitude The amplitude value
         * @return True if layer was found
         */
        bool setLayerAmplitude(const std::string& name, float amplitude);

        /**
         * @brief Set a layer's octaves
         * @param name Name of the layer
         * @param octaves The octave count
         * @return True if layer was found
         */
        bool setLayerOctaves(const std::string& name, int octaves);

        /**
         * @brief Set a layer's persistence
         * @param name Name of the layer
         * @param persistence The persistence value
         * @return True if layer was found
         */
        bool setLayerPersistence(const std::string& name, float persistence);

        /**
         * @brief Set a layer's lacunarity
         * @param name Name of the layer
         * @param lacunarity The lacunarity value
         * @return True if layer was found
         */
        bool setLayerLacunarity(const std::string& name, float lacunarity);

        /**
         * @brief Set a layer's offset
         * @param name Name of the layer
         * @param offset The offset vector
         * @return True if layer was found
         */
        bool setLayerOffset(const std::string& name, const glm::vec3& offset);

        /**
         * @brief Set a layer's scale
         * @param name Name of the layer
         * @param scale The scale vector
         * @return True if layer was found
         */
        bool setLayerScale(const std::string& name, const glm::vec3& scale);

        /**
         * @brief Set a layer's weight in the final result
         * @param name Name of the layer
         * @param weight The weight value [0.0-1.0]
         * @return True if layer was found
         */
        bool setLayerWeight(const std::string& name, float weight);

        /**
         * @brief Get a layer's weight
         * @param name Name of the layer
         * @param weight Output parameter for the weight value
         * @return True if layer was found
         */
        bool getLayerWeight(const std::string& name, float& weight) const;

        /**
         * @brief Set the operation used to combine this layer with previous layers
         * @param name Name of the layer
         * @param operation The combine operation
         * @return True if layer was found
         */
        bool setLayerCombineOperation(const std::string& name, CombineOperation operation);

        /**
         * @brief Get a layer's combine operation
         * @param name Name of the layer
         * @param operation Output parameter for the operation
         * @return True if layer was found
         */
        bool getLayerCombineOperation(const std::string& name, CombineOperation& operation) const;

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
         * @brief Serialize the generator configuration
         * @param serializer The serializer to serialize with
         */
        void serialize(ECS::Serializer& serializer) const;

        /**
         * @brief Deserialize the generator configuration
         * @param deserializer The deserializer to deserialize with
         */
        void deserialize(ECS::Deserializer& deserializer);

    private:
        // Internal structure for a noise layer
        struct NoiseLayer
        {
            std::string name;
            bool enabled = true;
            NoiseType noiseType = NoiseType::Perlin;
            FractalType fractalType = FractalType::FBM;
            InterpolationType interpolation = InterpolationType::Quintic;
            CombineOperation combineOp = CombineOperation::Add;
            float frequency = 1.0f;
            float amplitude = 1.0f;
            int octaves = 4;
            float persistence = 0.5f;
            float lacunarity = 2.0f;
            glm::vec3 offset = glm::vec3(0.0f);
            glm::vec3 scale = glm::vec3(1.0f);
            float weight = 1.0f;
            WarpType warpType = WarpType::None;
            float warpAmplitude = 1.0f;
            float warpFrequency = 1.0f;
            bool warpEnabled = false;
        };

        // Private helper methods
        /**
         * @brief Generate for a single coordinate
         * @param type Noise type to utilize
         * @param x X coordinate
         * @param y Y coordinate
         * @param z Z coordinate
         * @returns idk a float I guess
         */
        float generateSingle(NoiseType type, float x, float y, float z) const;

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
        float generateFractal(NoiseType noiseType, FractalType fractalType, float x, float y, float z,
                              int octaves, float persistence, float lacunarity) const;

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
        float computeWarp(WarpType type, float x, float y, float z, float amplitude, float frequency) const;

        /**
         * @brief Interpolate between values by interpolation type
         * @param InterpolationType Interpolation type to utilize
         * @param a Starting value
         * @param b End value
         * @param t Time to step by
         * @returns Next interpolated value
         */
        float interpolate(InterpolationType type, float a, float b, float t) const;

        /**
         * @brief Combine two values together using a combine operation by weight
         * @param a First value
         * @param b Second value
         * @param op Combine operation to use
         * @param weight Weight to combine by
         */
        float combineValues(float a, float b, CombineOperation op, float weight) const;

        /**
         * @brief Apply scale and offset to particular point
         * @param x X coordinate
         * @param y Y coordinate
         * @param z Z coordinate
         * @param scale Vector scale to apply
         * @param offset Vector offset to apply
         * @returns Vector point with applied offset and scale
         */
        glm::vec3 applyScaleAndOffset(float x, float y, float z, const glm::vec3& scale, const glm::vec3& offset) const;

        /**
         * @brief Validate the parameters of the generator
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
        NoiseType m_noiseType = NoiseType::Perlin;
        FractalType m_fractalType = FractalType::FBM;
        InterpolationType m_interpolation = InterpolationType::Quintic;
        int m_octaves = 4;
        float m_lacunarity = 2.0f;
        float m_persistence = 0.5f;
        float m_frequency = 1.0f;
        float m_amplitude = 1.0f;
        glm::vec3 m_scale = glm::vec3(1.0f);
        glm::vec3 m_offset = glm::vec3(0.0f);
        bool m_warpEnabled = false;
        WarpType m_warpType = WarpType::None;
        float m_warpAmplitude = 1.0f;
        float m_warpFrequency = 1.0f;

        std::vector<NoiseLayer> m_layers;
        std::unordered_map<std::string, size_t> m_layerIndices;
        std::function<float(float)> m_modifier;
    };

} // namespace PixelCraft::Voxel