// -------------------------------------------------------------------------
// NoiseGenerator.cpp
// -------------------------------------------------------------------------
#include "Voxel/NoiseGenerator.h"
#include "Core/Logger.h"
#include "Utility/Math.h"
#include "Utility/Random.h"
#include "ECS/Serializer.h"
#include "ECS/Deserializer.h"

#include <algorithm>
#include <random>
#include <cmath>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Voxel
{

    NoiseGenerator::NoiseGenerator()
    {
        // Initialize with default values
        validateParameters();
    }

    NoiseGenerator::~NoiseGenerator()
    {
        // Nothing to clean up
    }

    float NoiseGenerator::generate(float x, float y, float z) const
    {
        // Apply scale and offset to coordinates
        glm::vec3 pos = applyScaleAndOffset(x, y, z, m_scale, m_offset);

        // Apply domain warping if enabled
        if (m_warpEnabled)
        {
            float warpX = computeWarp(m_warpType, pos.x, pos.y, pos.z, m_warpAmplitude, m_warpFrequency);
            float warpY = computeWarp(m_warpType, pos.x + 123.456f, pos.y + 789.012f, pos.z + 345.678f, m_warpAmplitude, m_warpFrequency);
            float warpZ = computeWarp(m_warpType, pos.x + 901.234f, pos.y + 567.890f, pos.z + 123.456f, m_warpAmplitude, m_warpFrequency);

            pos.x += warpX;
            pos.y += warpY;
            pos.z += warpZ;
        }

        // If there are layers, use them
        if (!m_layers.empty())
        {
            float result = 0.0f;
            float totalWeight = 0.0f;

            // Start with the first layer as the base
            bool firstLayer = true;

            for (const auto& layer : m_layers)
            {
                if (!layer.enabled)
                {
                    continue;
                }

                // Apply the layer's scale and offset
                glm::vec3 layerPos = applyScaleAndOffset(pos.x, pos.y, pos.z, layer.scale, layer.offset);

                // Apply the layer's warping if enabled
                if (layer.warpEnabled)
                {
                    float warpX = computeWarp(layer.warpType, layerPos.x, layerPos.y, layerPos.z,
                                              layer.warpAmplitude, layer.warpFrequency);
                    float warpY = computeWarp(layer.warpType, layerPos.x + 123.456f, layerPos.y + 789.012f,
                                              layerPos.z + 345.678f, layer.warpAmplitude, layer.warpFrequency);
                    float warpZ = computeWarp(layer.warpType, layerPos.x + 901.234f, layerPos.y + 567.890f,
                                              layerPos.z + 123.456f, layer.warpAmplitude, layer.warpFrequency);

                    layerPos.x += warpX;
                    layerPos.y += warpY;
                    layerPos.z += warpZ;
                }

                // Generate noise value for this layer
                float layerValue;
                if (layer.fractalType == FractalType::None)
                {
                    layerValue = generateSingle(layer.noiseType, layerPos.x * layer.frequency,
                                                layerPos.y * layer.frequency, layerPos.z * layer.frequency);
                }
                else
                {
                    layerValue = generateFractal(layer.noiseType, layer.fractalType,
                                                 layerPos.x * layer.frequency, layerPos.y * layer.frequency,
                                                 layerPos.z * layer.frequency, layer.octaves,
                                                 layer.persistence, layer.lacunarity);
                }

                // Scale by amplitude
                layerValue *= layer.amplitude;

                // Combine with previous result
                if (firstLayer)
                {
                    result = layerValue;
                    firstLayer = false;
                }
                else
                {
                    result = combineValues(result, layerValue, layer.combineOp, layer.weight);
                }

                totalWeight += layer.weight;
            }

            // Normalize if total weight is not zero
            if (totalWeight > 0.0f && !firstLayer)
            {
                result /= totalWeight;
            }

            // Apply custom modifier if set
            if (m_modifier)
            {
                result = m_modifier(result);
            }

            return result;
        }

        // No layers, use the base settings
        float result;

        // Scale coordinates by frequency
        pos.x *= m_frequency;
        pos.y *= m_frequency;
        pos.z *= m_frequency;

        // Generate noise value
        if (m_fractalType == FractalType::None)
        {
            result = generateSingle(m_noiseType, pos.x, pos.y, pos.z);
        }
        else
        {
            result = generateFractal(m_noiseType, m_fractalType, pos.x, pos.y, pos.z,
                                     m_octaves, m_persistence, m_lacunarity);
        }

        // Scale by amplitude
        result *= m_amplitude;

        // Apply custom modifier if set
        if (m_modifier)
        {
            result = m_modifier(result);
        }

        return result;
    }

    float NoiseGenerator::generate(const glm::vec3& position) const
    {
        return generate(position.x, position.y, position.z);
    }

    float NoiseGenerator::generate2D(float x, float y) const
    {
        // Just use the 3D generator with z=0
        return generate(x, y, 0.0f);
    }

    void NoiseGenerator::setSeed(uint32_t seed)
    {
        m_seed = seed;
    }

    uint32_t NoiseGenerator::getSeed() const
    {
        return m_seed;
    }

    void NoiseGenerator::setNoiseType(NoiseType type)
    {
        m_noiseType = type;
    }

    NoiseGenerator::NoiseType NoiseGenerator::getNoiseType() const
    {
        return m_noiseType;
    }

    void NoiseGenerator::setFractalType(FractalType type)
    {
        m_fractalType = type;
    }

    NoiseGenerator::FractalType NoiseGenerator::getFractalType() const
    {
        return m_fractalType;
    }

    void NoiseGenerator::setOctaves(int octaves)
    {
        m_octaves = std::max(1, std::min(10, octaves));
    }

    int NoiseGenerator::getOctaves() const
    {
        return m_octaves;
    }

    void NoiseGenerator::setLacunarity(float lacunarity)
    {
        m_lacunarity = std::max(1.0f, std::min(4.0f, lacunarity));
    }

    float NoiseGenerator::getLacunarity() const
    {
        return m_lacunarity;
    }

    void NoiseGenerator::setPersistence(float persistence)
    {
        m_persistence = std::max(0.0f, std::min(1.0f, persistence));
    }

    float NoiseGenerator::getPersistence() const
    {
        return m_persistence;
    }

    void NoiseGenerator::setFrequency(float frequency)
    {
        m_frequency = frequency;
    }

    float NoiseGenerator::getFrequency() const
    {
        return m_frequency;
    }

    void NoiseGenerator::setAmplitude(float amplitude)
    {
        m_amplitude = amplitude;
    }

    float NoiseGenerator::getAmplitude() const
    {
        return m_amplitude;
    }

    void NoiseGenerator::setScale(const glm::vec3& scale)
    {
        m_scale = scale;
    }

    glm::vec3 NoiseGenerator::getScale() const
    {
        return m_scale;
    }

    void NoiseGenerator::setOffset(const glm::vec3& offset)
    {
        m_offset = offset;
    }

    glm::vec3 NoiseGenerator::getOffset() const
    {
        return m_offset;
    }

    void NoiseGenerator::enableWarp(bool enable)
    {
        m_warpEnabled = enable;
    }

    bool NoiseGenerator::isWarpEnabled() const
    {
        return m_warpEnabled;
    }

    void NoiseGenerator::setWarpType(WarpType type)
    {
        m_warpType = type;
    }

    NoiseGenerator::WarpType NoiseGenerator::getWarpType() const
    {
        return m_warpType;
    }

    void NoiseGenerator::setWarpAmplitude(float amplitude)
    {
        m_warpAmplitude = amplitude;
    }

    float NoiseGenerator::getWarpAmplitude() const
    {
        return m_warpAmplitude;
    }

    void NoiseGenerator::setWarpFrequency(float frequency)
    {
        m_warpFrequency = frequency;
    }

    float NoiseGenerator::getWarpFrequency() const
    {
        return m_warpFrequency;
    }

    void NoiseGenerator::setInterpolation(InterpolationType interp)
    {
        m_interpolation = interp;
    }

    NoiseGenerator::InterpolationType NoiseGenerator::getInterpolation() const
    {
        return m_interpolation;
    }

    bool NoiseGenerator::createLayer(const std::string& name)
    {
        if (name.empty())
        {
            Log::error("Layer name cannot be empty");
            return false;
        }

        // Check if layer already exists
        if (m_layerIndices.find(name) != m_layerIndices.end())
        {
            Log::warn("Layer '" + name + "' already exists");
            return false;
        }

        // Create new layer with default values
        NoiseLayer layer;
        layer.name = name;

        // Add to layers list
        m_layers.push_back(layer);
        m_layerIndices[name] = m_layers.size() - 1;

        return true;
    }

    bool NoiseGenerator::removeLayer(const std::string& name)
    {
        auto it = m_layerIndices.find(name);
        if (it == m_layerIndices.end())
        {
            Log::warn("Layer '" + name + "' not found");
            return false;
        }

        size_t index = it->second;

        // Remove the layer
        m_layers.erase(m_layers.begin() + index);
        m_layerIndices.erase(it);

        // Update indices of remaining layers
        for (auto& [layerName, layerIndex] : m_layerIndices)
        {
            if (layerIndex > index)
            {
                layerIndex--;
            }
        }

        return true;
    }

    bool NoiseGenerator::setLayerEnabled(const std::string& name, bool enabled)
    {
        auto it = m_layerIndices.find(name);
        if (it == m_layerIndices.end())
        {
            Log::warn("Layer '" + name + "' not found");
            return false;
        }

        m_layers[it->second].enabled = enabled;
        return true;
    }

    bool NoiseGenerator::isLayerEnabled(const std::string& name) const
    {
        auto it = m_layerIndices.find(name);
        if (it == m_layerIndices.end())
        {
            return false;
        }

        return m_layers[it->second].enabled;
    }

    bool NoiseGenerator::setLayerNoiseType(const std::string& name, NoiseType type)
    {
        auto it = m_layerIndices.find(name);
        if (it == m_layerIndices.end())
        {
            Log::warn("Layer '" + name + "' not found");
            return false;
        }

        m_layers[it->second].noiseType = type;
        return true;
    }

    bool NoiseGenerator::setLayerFractalType(const std::string& name, FractalType type)
    {
        auto it = m_layerIndices.find(name);
        if (it == m_layerIndices.end())
        {
            Log::warn("Layer '" + name + "' not found");
            return false;
        }

        m_layers[it->second].fractalType = type;
        return true;
    }

    bool NoiseGenerator::setLayerFrequency(const std::string& name, float frequency)
    {
        auto it = m_layerIndices.find(name);
        if (it == m_layerIndices.end())
        {
            Log::warn("Layer '" + name + "' not found");
            return false;
        }

        m_layers[it->second].frequency = frequency;
        return true;
    }

    bool NoiseGenerator::setLayerAmplitude(const std::string& name, float amplitude)
    {
        auto it = m_layerIndices.find(name);
        if (it == m_layerIndices.end())
        {
            Log::warn("Layer '" + name + "' not found");
            return false;
        }

        m_layers[it->second].amplitude = amplitude;
        return true;
    }

    bool NoiseGenerator::setLayerOctaves(const std::string& name, int octaves)
    {
        auto it = m_layerIndices.find(name);
        if (it == m_layerIndices.end())
        {
            Log::warn("Layer '" + name + "' not found");
            return false;
        }

        m_layers[it->second].octaves = std::max(1, std::min(10, octaves));
        return true;
    }

    bool NoiseGenerator::setLayerPersistence(const std::string& name, float persistence)
    {
        auto it = m_layerIndices.find(name);
        if (it == m_layerIndices.end())
        {
            Log::warn("Layer '" + name + "' not found");
            return false;
        }

        m_layers[it->second].persistence = std::max(0.0f, std::min(1.0f, persistence));
        return true;
    }

    bool NoiseGenerator::setLayerLacunarity(const std::string& name, float lacunarity)
    {
        auto it = m_layerIndices.find(name);
        if (it == m_layerIndices.end())
        {
            Log::warn("Layer '" + name + "' not found");
            return false;
        }

        m_layers[it->second].lacunarity = std::max(1.0f, std::min(4.0f, lacunarity));
        return true;
    }

    bool NoiseGenerator::setLayerOffset(const std::string& name, const glm::vec3& offset)
    {
        auto it = m_layerIndices.find(name);
        if (it == m_layerIndices.end())
        {
            Log::warn("Layer '" + name + "' not found");
            return false;
        }

        m_layers[it->second].offset = offset;
        return true;
    }

    bool NoiseGenerator::setLayerScale(const std::string& name, const glm::vec3& scale)
    {
        auto it = m_layerIndices.find(name);
        if (it == m_layerIndices.end())
        {
            Log::warn("Layer '" + name + "' not found");
            return false;
        }

        m_layers[it->second].scale = scale;
        return true;
    }

    bool NoiseGenerator::setLayerWeight(const std::string& name, float weight)
    {
        auto it = m_layerIndices.find(name);
        if (it == m_layerIndices.end())
        {
            Log::warn("Layer '" + name + "' not found");
            return false;
        }

        m_layers[it->second].weight = std::max(0.0f, std::min(1.0f, weight));
        return true;
    }

    bool NoiseGenerator::getLayerWeight(const std::string& name, float& weight) const
    {
        auto it = m_layerIndices.find(name);
        if (it == m_layerIndices.end())
        {
            return false;
        }

        weight = m_layers[it->second].weight;
        return true;
    }

    bool NoiseGenerator::setLayerCombineOperation(const std::string& name, CombineOperation operation)
    {
        auto it = m_layerIndices.find(name);
        if (it == m_layerIndices.end())
        {
            Log::warn("Layer '" + name + "' not found");
            return false;
        }

        m_layers[it->second].combineOp = operation;
        return true;
    }

    bool NoiseGenerator::getLayerCombineOperation(const std::string& name, CombineOperation& operation) const
    {
        auto it = m_layerIndices.find(name);
        if (it == m_layerIndices.end())
        {
            return false;
        }

        operation = m_layers[it->second].combineOp;
        return true;
    }

    std::vector<std::string> NoiseGenerator::getLayerNames() const
    {
        std::vector<std::string> names;
        names.reserve(m_layers.size());

        for (const auto& layer : m_layers)
        {
            names.push_back(layer.name);
        }

        return names;
    }

    size_t NoiseGenerator::getLayerCount() const
    {
        return m_layers.size();
    }

    void NoiseGenerator::setModifier(std::function<float(float)> modifier)
    {
        m_modifier = modifier;
    }

    void NoiseGenerator::clearModifier()
    {
        m_modifier = nullptr;
    }

    bool NoiseGenerator::presetTerrain(bool mountainous)
    {
        // Clear existing layers
        m_layers.clear();
        m_layerIndices.clear();

        // Set base properties
        m_noiseType = NoiseType::Perlin;
        m_fractalType = FractalType::FBM;
        m_octaves = 4;
        m_persistence = 0.5f;
        m_lacunarity = 2.0f;
        m_frequency = 0.01f;
        m_amplitude = 1.0f;

        // Create base terrain layer
        createLayer("base");
        setLayerNoiseType("base", NoiseType::Perlin);
        setLayerFractalType("base", FractalType::FBM);
        setLayerOctaves("base", 4);
        setLayerPersistence("base", 0.5f);
        setLayerLacunarity("base", 2.0f);
        setLayerFrequency("base", 0.01f);
        setLayerAmplitude("base", 1.0f);
        setLayerWeight("base", 1.0f);

        // Create detail layer
        createLayer("detail");
        setLayerNoiseType("detail", NoiseType::Perlin);
        setLayerFractalType("detail", FractalType::FBM);
        setLayerOctaves("detail", 6);
        setLayerPersistence("detail", 0.4f);
        setLayerLacunarity("detail", 2.5f);
        setLayerFrequency("detail", 0.04f);
        setLayerAmplitude("detail", 0.2f);
        setLayerCombineOperation("detail", CombineOperation::Add);
        setLayerWeight("detail", 0.5f);

        if (mountainous)
        {
            // Create ridge layer for mountains
            createLayer("ridges");
            setLayerNoiseType("ridges", NoiseType::Ridged);
            setLayerFractalType("ridges", FractalType::FBM);
            setLayerOctaves("ridges", 3);
            setLayerPersistence("ridges", 0.7f);
            setLayerLacunarity("ridges", 2.3f);
            setLayerFrequency("ridges", 0.008f);
            setLayerAmplitude("ridges", 0.8f);
            setLayerCombineOperation("ridges", CombineOperation::Add);
            setLayerWeight("ridges", 0.7f);

            // Add warping to make mountains more interesting
            enableWarp(true);
            setWarpType(WarpType::Fractal);
            setWarpAmplitude(20.0f);
            setWarpFrequency(0.005f);
        }

        return true;
    }

    bool NoiseGenerator::presetCaves()
    {
        // Clear existing layers
        m_layers.clear();
        m_layerIndices.clear();

        // Set base properties
        m_noiseType = NoiseType::Perlin;
        m_fractalType = FractalType::FBM;
        m_octaves = 3;
        m_persistence = 0.4f;
        m_lacunarity = 2.0f;
        m_frequency = 0.05f;
        m_amplitude = 1.0f;

        // Create base cave layer
        createLayer("base");
        setLayerNoiseType("base", NoiseType::Perlin);
        setLayerFractalType("base", FractalType::FBM);
        setLayerOctaves("base", 3);
        setLayerPersistence("base", 0.4f);
        setLayerLacunarity("base", 2.0f);
        setLayerFrequency("base", 0.05f);
        setLayerAmplitude("base", 1.0f);
        setLayerWeight("base", 1.0f);

        // Create cave shape layer
        createLayer("tunnels");
        setLayerNoiseType("tunnels", NoiseType::Worley);
        setLayerFractalType("tunnels", FractalType::None);
        setLayerFrequency("tunnels", 0.03f);
        setLayerAmplitude("tunnels", 1.0f);
        setLayerCombineOperation("tunnels", CombineOperation::Multiply);
        setLayerWeight("tunnels", 0.8f);

        // Add warping for more natural cave systems
        enableWarp(true);
        setWarpType(WarpType::Basic);
        setWarpAmplitude(5.0f);
        setWarpFrequency(0.02f);

        return true;
    }

    bool NoiseGenerator::presetOreDistribution()
    {
        // Clear existing layers
        m_layers.clear();
        m_layerIndices.clear();

        // Set base properties
        m_noiseType = NoiseType::Perlin;
        m_fractalType = FractalType::FBM;
        m_octaves = 2;
        m_persistence = 0.5f;
        m_lacunarity = 2.0f;
        m_frequency = 0.1f;
        m_amplitude = 1.0f;

        // Create base layer for overall distribution
        createLayer("distribution");
        setLayerNoiseType("distribution", NoiseType::Perlin);
        setLayerFractalType("distribution", FractalType::FBM);
        setLayerOctaves("distribution", 2);
        setLayerPersistence("distribution", 0.5f);
        setLayerLacunarity("distribution", 2.0f);
        setLayerFrequency("distribution", 0.1f);
        setLayerAmplitude("distribution", 1.0f);
        setLayerWeight("distribution", 1.0f);

        // Create vein layer for ore veins
        createLayer("veins");
        setLayerNoiseType("veins", NoiseType::Worley);
        setLayerFractalType("veins", FractalType::None);
        setLayerFrequency("veins", 0.2f);
        setLayerAmplitude("veins", 1.0f);
        setLayerCombineOperation("veins", CombineOperation::Multiply);
        setLayerWeight("veins", 0.6f);

        // Create detail layer for smaller deposits
        createLayer("deposits");
        setLayerNoiseType("deposits", NoiseType::Value);
        setLayerFractalType("deposits", FractalType::FBM);
        setLayerOctaves("deposits", 3);
        setLayerPersistence("deposits", 0.3f);
        setLayerLacunarity("deposits", 2.5f);
        setLayerFrequency("deposits", 0.3f);
        setLayerAmplitude("deposits", 0.5f);
        setLayerCombineOperation("deposits", CombineOperation::Add);
        setLayerWeight("deposits", 0.4f);

        return true;
    }

    bool NoiseGenerator::presetBiomeBlend()
    {
        // Clear existing layers
        m_layers.clear();
        m_layerIndices.clear();

        // Set base properties
        m_noiseType = NoiseType::Perlin;
        m_fractalType = FractalType::FBM;
        m_octaves = 3;
        m_persistence = 0.5f;
        m_lacunarity = 2.0f;
        m_frequency = 0.01f;
        m_amplitude = 1.0f;

        // Create temperature layer
        createLayer("temperature");
        setLayerNoiseType("temperature", NoiseType::Perlin);
        setLayerFractalType("temperature", FractalType::FBM);
        setLayerOctaves("temperature", 3);
        setLayerPersistence("temperature", 0.5f);
        setLayerLacunarity("temperature", 2.0f);
        setLayerFrequency("temperature", 0.01f);
        setLayerAmplitude("temperature", 1.0f);
        setLayerWeight("temperature", 1.0f);

        // Create humidity layer
        createLayer("humidity");
        setLayerNoiseType("humidity", NoiseType::Perlin);
        setLayerFractalType("humidity", FractalType::FBM);
        setLayerOctaves("humidity", 3);
        setLayerPersistence("humidity", 0.5f);
        setLayerLacunarity("humidity", 2.0f);
        setLayerFrequency("humidity", 0.01f);
        setLayerAmplitude("humidity", 1.0f);
        setLayerOffset("humidity", glm::vec3(123.0f, 456.0f, 789.0f)); // Offset to make it different from temperature
        setLayerCombineOperation("humidity", CombineOperation::Blend);
        setLayerWeight("humidity", 1.0f);

        // Create altitude variation layer
        createLayer("altitude");
        setLayerNoiseType("altitude", NoiseType::Perlin);
        setLayerFractalType("altitude", FractalType::FBM);
        setLayerOctaves("altitude", 4);
        setLayerPersistence("altitude", 0.6f);
        setLayerLacunarity("altitude", 2.2f);
        setLayerFrequency("altitude", 0.02f);
        setLayerAmplitude("altitude", 0.7f);
        setLayerOffset("altitude", glm::vec3(987.0f, 654.0f, 321.0f));
        setLayerCombineOperation("altitude", CombineOperation::Multiply);
        setLayerWeight("altitude", 0.5f);

        return true;
    }

    bool NoiseGenerator::presetDetailTexture()
    {
        // Clear existing layers
        m_layers.clear();
        m_layerIndices.clear();

        // Set base properties
        m_noiseType = NoiseType::Value;
        m_fractalType = FractalType::FBM;
        m_octaves = 5;
        m_persistence = 0.5f;
        m_lacunarity = 2.0f;
        m_frequency = 0.2f;
        m_amplitude = 1.0f;

        // Create base detail layer
        createLayer("base");
        setLayerNoiseType("base", NoiseType::Value);
        setLayerFractalType("base", FractalType::FBM);
        setLayerOctaves("base", 5);
        setLayerPersistence("base", 0.5f);
        setLayerLacunarity("base", 2.0f);
        setLayerFrequency("base", 0.2f);
        setLayerAmplitude("base", 1.0f);
        setLayerWeight("base", 1.0f);

        // Create fine detail layer
        createLayer("fine");
        setLayerNoiseType("fine", NoiseType::Perlin);
        setLayerFractalType("fine", FractalType::FBM);
        setLayerOctaves("fine", 7);
        setLayerPersistence("fine", 0.6f);
        setLayerLacunarity("fine", 2.5f);
        setLayerFrequency("fine", 0.5f);
        setLayerAmplitude("fine", 0.3f);
        setLayerCombineOperation("fine", CombineOperation::Add);
        setLayerWeight("fine", 0.4f);

        // Create spots layer
        createLayer("spots");
        setLayerNoiseType("spots", NoiseType::Worley);
        setLayerFractalType("spots", FractalType::None);
        setLayerFrequency("spots", 0.4f);
        setLayerAmplitude("spots", 0.5f);
        setLayerCombineOperation("spots", CombineOperation::Multiply);
        setLayerWeight("spots", 0.3f);

        return true;
    }

    void NoiseGenerator::serialize(ECS::Serializer& serializer) const
    {
        // Serialize base properties
        serializer.write(m_seed);
        serializer.write(static_cast<int>(m_noiseType));
        serializer.write(static_cast<int>(m_fractalType));
        serializer.write(static_cast<int>(m_interpolation));
        serializer.write(m_octaves);
        serializer.write(m_lacunarity);
        serializer.write(m_persistence);
        serializer.write(m_frequency);
        serializer.write(m_amplitude);
        serializer.writeVec3(m_scale);
        serializer.writeVec3(m_offset);
        serializer.write(m_warpEnabled);
        serializer.write(static_cast<int>(m_warpType));
        serializer.write(m_warpAmplitude);
        serializer.write(m_warpFrequency);

        // Serialize layers
        serializer.beginArray("layers", m_layers.size());
        for (size_t i = 0; i < m_layers.size(); i++)
        {
            const auto& layer = m_layers[i];

            serializer.writeString(layer.name);
            serializer.write(layer.enabled);
            serializer.write(static_cast<int>(layer.noiseType));
            serializer.write(static_cast<int>(layer.fractalType));
            serializer.write(static_cast<int>(layer.interpolation));
            serializer.write(static_cast<int>(layer.combineOp));
            serializer.write(layer.frequency);
            serializer.write(layer.amplitude);
            serializer.write(layer.octaves);
            serializer.write(layer.persistence);
            serializer.write(layer.lacunarity);
            serializer.writeVec3(layer.offset);
            serializer.writeVec3(layer.scale);
            serializer.write(layer.weight);
            serializer.write(static_cast<int>(layer.warpType));
            serializer.write(layer.warpAmplitude);
            serializer.write(layer.warpFrequency);
            serializer.write(layer.warpEnabled);
        }

        serializer.endArray();
    }

    void NoiseGenerator::deserialize(ECS::Deserializer& deserializer)
    {
        // Clear existing data
        m_layers.clear();
        m_layerIndices.clear();

        // Deserialize base properties
        deserializer.read(m_seed);

        int tempNoiseType;
        deserializer.read(tempNoiseType);
        m_noiseType = static_cast<NoiseType>(tempNoiseType);

        int tempFractalType;
        deserializer.read(tempFractalType);
        m_fractalType = static_cast<FractalType>(tempFractalType);

        int tempInterpolation;
        deserializer.read(tempInterpolation);
        m_interpolation = static_cast<InterpolationType>(tempInterpolation);

        deserializer.read(m_octaves);
        deserializer.read(m_lacunarity);
        deserializer.read(m_persistence);
        deserializer.read(m_frequency);
        deserializer.read(m_amplitude);
        deserializer.readVec3(m_scale);
        deserializer.readVec3(m_offset);
        deserializer.read(m_warpEnabled);

        int tempWarpType;
        deserializer.read(tempWarpType);
        m_warpType = static_cast<WarpType>(tempWarpType);

        deserializer.read(m_warpAmplitude);
        deserializer.read(m_warpFrequency);

        // Deserialize layers
        size_t layersSize;
        deserializer.beginArray("layers", layersSize);

        for (int i = 0; i < layersSize; i++)
        {
            NoiseLayer layer;

            deserializer.readString(layer.name);
            deserializer.read(layer.enabled);

            int noiseType;
            deserializer.read(noiseType);
            layer.noiseType = static_cast<NoiseType>(noiseType);

            int fractalType;
            deserializer.read(fractalType);
            layer.fractalType = static_cast<FractalType>(fractalType);

            int interpolationType;
            deserializer.read(interpolationType);
            layer.interpolation = static_cast<InterpolationType>(interpolationType);

            int combineOperation;
            deserializer.read(combineOperation);
            layer.combineOp = static_cast<CombineOperation>(combineOperation);

            deserializer.read(layer.frequency);
            deserializer.read(layer.amplitude);
            deserializer.read(layer.amplitude);
            deserializer.read(layer.octaves);
            deserializer.read(layer.persistence);
            deserializer.read(layer.lacunarity);
            deserializer.readVec3(layer.offset);
            deserializer.readVec3(layer.scale);
            deserializer.read(layer.weight);

            int tempWarpType;
            deserializer.read(tempWarpType);
            layer.warpType = static_cast<WarpType>(tempWarpType);

            deserializer.read(layer.warpAmplitude);
            deserializer.read(layer.warpFrequency);
            deserializer.read(layer.warpEnabled);

            m_layers.push_back(layer);
            m_layerIndices[layer.name] = i;
        }

        deserializer.endArray();
        validateParameters();
    }

    // Private helper methods

    float NoiseGenerator::generateSingle(NoiseType type, float x, float y, float z) const
    {
        switch (type)
        {
            case NoiseType::Perlin:
                return Utility::Math::perlinNoise(x, y, z);

            case NoiseType::Simplex:
                return Utility::Math::simplexNoise(x, y, z);

            case NoiseType::Worley:
            {
                // Simple Worley (cellular) noise implementation
                constexpr int primes[3] = {15731, 789221, 1376312589};

                // Determine cell coordinates
                int cellX = static_cast<int>(std::floor(x));
                int cellY = static_cast<int>(std::floor(y));
                int cellZ = static_cast<int>(std::floor(z));

                float minDist = 1000.0f;

                // Search neighboring cells
                for (int i = -1; i <= 1; i++)
                {
                    for (int j = -1; j <= 1; j++)
                    {
                        for (int k = -1; k <= 1; k++)
                        {
                            int neighborX = cellX + i;
                            int neighborY = cellY + j;
                            int neighborZ = cellZ + k;

                            // Generate feature point for this cell
                            uint32_t cellSeed = m_seed;
                            cellSeed = cellSeed * primes[0] + neighborX * primes[1];
                            cellSeed = cellSeed * primes[0] + neighborY * primes[1];
                            cellSeed = cellSeed * primes[0] + neighborZ * primes[1];

                            // Use the seed to generate a feature point
                            float featureX = neighborX + (cellSeed % 1000) / 1000.0f;
                            cellSeed = cellSeed * primes[0];
                            float featureY = neighborY + (cellSeed % 1000) / 1000.0f;
                            cellSeed = cellSeed * primes[0];
                            float featureZ = neighborZ + (cellSeed % 1000) / 1000.0f;

                            // Compute distance to feature point
                            float dx = x - featureX;
                            float dy = y - featureY;
                            float dz = z - featureZ;
                            float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

                            minDist = std::min(minDist, dist);
                        }
                    }
                }

                // Convert to [-1, 1] range
                return minDist * 2.0f - 1.0f;
            }

            case NoiseType::Value:
            {
                // Value noise implementation
                int x0 = static_cast<int>(std::floor(x));
                int y0 = static_cast<int>(std::floor(y));
                int z0 = static_cast<int>(std::floor(z));
                int x1 = x0 + 1;
                int y1 = y0 + 1;
                int z1 = z0 + 1;

                float sx = x - static_cast<float>(x0);
                float sy = y - static_cast<float>(y0);
                float sz = z - static_cast<float>(z0);

                // Get values at corners
                float n000 = valueNoise(x0, y0, z0);
                float n100 = valueNoise(x1, y0, z0);
                float n010 = valueNoise(x0, y1, z0);
                float n110 = valueNoise(x1, y1, z0);
                float n001 = valueNoise(x0, y0, z1);
                float n101 = valueNoise(x1, y0, z1);
                float n011 = valueNoise(x0, y1, z1);
                float n111 = valueNoise(x1, y1, z1);

                // Interpolate
                float tx, ty, tz;

                switch (m_interpolation)
                {
                    case InterpolationType::Linear:
                        tx = sx;
                        ty = sy;
                        tz = sz;
                        break;

                    case InterpolationType::Cubic:
                        tx = sx * sx * (3 - 2 * sx);
                        ty = sy * sy * (3 - 2 * sy);
                        tz = sz * sz * (3 - 2 * sz);
                        break;

                    case InterpolationType::Quintic:
                        tx = sx * sx * sx * (sx * (sx * 6 - 15) + 10);
                        ty = sy * sy * sy * (sy * (sy * 6 - 15) + 10);
                        tz = sz * sz * sz * (sz * (sz * 6 - 15) + 10);
                        break;

                    default:
                        tx = sx;
                        ty = sy;
                        tz = sz;
                }

                return interpolate(
                    m_interpolation,
                    interpolate(
                        m_interpolation,
                        interpolate(m_interpolation, n000, n100, tx),
                        interpolate(m_interpolation, n010, n110, tx),
                        ty
                    ),
                    interpolate(
                        m_interpolation,
                        interpolate(m_interpolation, n001, n101, tx),
                        interpolate(m_interpolation, n011, n111, tx),
                        ty
                    ),
                    tz
                );
            }

            case NoiseType::Cubic:
            {
                // Cubic noise (similar to value noise but with cubic interpolation)
                int x0 = static_cast<int>(std::floor(x));
                int y0 = static_cast<int>(std::floor(y));
                int z0 = static_cast<int>(std::floor(z));

                float sx = x - static_cast<float>(x0);
                float sy = y - static_cast<float>(y0);
                float sz = z - static_cast<float>(z0);

                float tx = sx * sx * (3.0f - 2.0f * sx);
                float ty = sy * sy * (3.0f - 2.0f * sy);
                float tz = sz * sz * (3.0f - 2.0f * sz);

                // Get values at corners
                float n000 = valueNoise(x0, y0, z0);
                float n100 = valueNoise(x0 + 1, y0, z0);
                float n010 = valueNoise(x0, y0 + 1, z0);
                float n110 = valueNoise(x0 + 1, y0 + 1, z0);
                float n001 = valueNoise(x0, y0, z0 + 1);
                float n101 = valueNoise(x0 + 1, y0, z0 + 1);
                float n011 = valueNoise(x0, y0 + 1, z0 + 1);
                float n111 = valueNoise(x0 + 1, y0 + 1, z0 + 1);

                // Interpolate
                return interpolate(
                    m_interpolation,
                    interpolate(
                        m_interpolation,
                        interpolate(m_interpolation, n000, n100, tx),
                        interpolate(m_interpolation, n010, n110, tx),
                        ty
                    ),
                    interpolate(
                        m_interpolation,
                        interpolate(m_interpolation, n001, n101, tx),
                        interpolate(m_interpolation, n011, n111, tx),
                        ty
                    ),
                    tz
                );
            }

            case NoiseType::WhiteNoise:
            {
                // Simple white noise implementation
                uint32_t seed = m_seed;
                int ix = static_cast<int>(std::floor(x));
                int iy = static_cast<int>(std::floor(y));
                int iz = static_cast<int>(std::floor(z));

                // Combine coordinates and seed
                uint32_t h = seed + ix * 374761393 + iy * 668265263 + iz * 198491317;
                h ^= (h >> 13);
                h *= 1274126177;
                h ^= (h >> 16);

                // Convert to float in range [-1, 1]
                return (static_cast<float>(h % 2000000000) / 1000000000.0f) - 1.0f;
            }

            case NoiseType::Ridged:
            {
                // Ridged multi-fractal noise based on Perlin
                float noise = std::abs(Utility::Math::perlinNoise(x, y, z));
                return 2.0f * (0.5f - noise);
            }
            
            case NoiseType::Billow:
            {
                // Billow noise based on Perlin
                float noise = std::abs(Utility::Math::perlinNoise(x, y, z));
                return 2.0f * noise - 1.0f;
            }

            case NoiseType::Voronoi:
            {
                // Voronoi noise implementation (similar to Worley but different distance metric)
                constexpr int primes[3] = {15731, 789221, 1376312589};

                // Determine cell coordinates
                int cellX = static_cast<int>(std::floor(x));
                int cellY = static_cast<int>(std::floor(y));
                int cellZ = static_cast<int>(std::floor(z));

                float minDist = 1000.0f;
                glm::vec3 closestPoint;

                // Search neighboring cells
                for (int i = -1; i <= 1; i++)
                {
                    for (int j = -1; j <= 1; j++)
                    {
                        for (int k = -1; k <= 1; k++)
                        {
                            int neighborX = cellX + i;
                            int neighborY = cellY + j;
                            int neighborZ = cellZ + k;

                            // Generate feature point for this cell
                            uint32_t cellSeed = m_seed;
                            cellSeed = cellSeed * primes[0] + neighborX * primes[1];
                            cellSeed = cellSeed * primes[0] + neighborY * primes[1];
                            cellSeed = cellSeed * primes[0] + neighborZ * primes[1];

                            // Use the seed to generate a feature point
                            float featureX = neighborX + (cellSeed % 1000) / 1000.0f;
                            cellSeed = cellSeed * primes[0];
                            float featureY = neighborY + (cellSeed % 1000) / 1000.0f;
                            cellSeed = cellSeed * primes[0];
                            float featureZ = neighborZ + (cellSeed % 1000) / 1000.0f;

                            // Compute Manhattan distance to feature point
                            float dist = std::abs(x - featureX) + std::abs(y - featureY) + std::abs(z - featureZ);

                            if (dist < minDist)
                            {
                                minDist = dist;
                                closestPoint = glm::vec3(featureX, featureY, featureZ);
                            }
                        }
                    }
                }

                // Use the closest point to generate a value in [-1, 1]
                uint32_t pointSeed = m_seed;
                pointSeed = pointSeed * primes[0] + static_cast<int>(closestPoint.x * 1000) * primes[1];
                pointSeed = pointSeed * primes[0] + static_cast<int>(closestPoint.y * 1000) * primes[1];
                pointSeed = pointSeed * primes[0] + static_cast<int>(closestPoint.z * 1000) * primes[1];

                // Convert to float in range [-1, 1]
                return (static_cast<float>(pointSeed % 2000000000) / 1000000000.0f) - 1.0f;
            }

            case NoiseType::Domain:
            {
                // Domain warping noise based on Perlin
                float warpX = Utility::Math::perlinNoise(x + 123.4f, y + 567.8f, z + 901.2f);
                float warpY = Utility::Math::perlinNoise(x + 345.6f, y + 789.0f, z + 123.4f);
                float warpZ = Utility::Math::perlinNoise(x + 678.9f, y + 123.4f, z + 567.8f);

                return Utility::Math::perlinNoise(
                    x + warpX * 0.5f,
                    y + warpY * 0.5f,
                    z + warpZ * 0.5f
                );
            }

            default:
                return 0.0;
        }
    }

    float NoiseGenerator::generateFractal(NoiseType noiseType, FractalType fractalType, float x, float y, float z,
                                          int octaves, float persistence, float lacunarity) const
    {
        float result = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float maxValue = 0.0f;

        switch (fractalType)
        {
            case FractalType::FBM:
            {
                // Fractal Brownian Motion
                for (int i = 0; i < octaves; i++)
                {
                    result += generateSingle(noiseType, x * frequency, y * frequency, z * frequency) * amplitude;
                    maxValue += amplitude;
                    amplitude *= persistence;
                    frequency *= lacunarity;
                }

                // Normalize the result
                return result / maxValue;
            }

            case FractalType::Rigid:
            {
                // Rigid multi-fractal
                for (int i = 0; i < octaves; i++)
                {
                    float noise = 1.0f - std::abs(generateSingle(noiseType, x * frequency, y * frequency, z * frequency));
                    noise = noise * noise;
                    result += noise * amplitude;
                    maxValue += amplitude;
                    amplitude *= persistence;
                    frequency *= lacunarity;
                }

                // Normalize the result
                return result / maxValue;
            }

            case FractalType::Billow:
            {
                // Billowy noise
                for (int i = 0; i < octaves; i++)
                {
                    float noise = std::abs(generateSingle(noiseType, x * frequency, y * frequency, z * frequency));
                    result += (noise * 2.0f - 1.0f) * amplitude;
                    maxValue += amplitude;
                    amplitude *= persistence;
                    frequency *= lacunarity;
                }

                // Normalize the result
                return result / maxValue;
            }

            case FractalType::MultiFractal:
            {
                // Multi-fractal
                result = 1.0f;
                for (int i = 0; i < octaves; i++)
                {
                    result *= (generateSingle(noiseType, x * frequency, y * frequency, z * frequency) + 1.0f) * amplitude * 0.5f + 0.5f;
                    frequency *= lacunarity;
                }

                return result * 2.0f - 1.0f;
            }

            case FractalType::HybridMulti:
            {
                // Hybrid multi-fractal
                result = generateSingle(noiseType, x, y, z) + 0.5f;
                float weight = result;

                for (int i = 1; i < octaves; i++)
                {
                    if (weight > 1.0f) weight = 1.0f;

                    float signal = (generateSingle(noiseType, x * frequency, y * frequency, z * frequency) + 0.5f) * amplitude;
                    result += weight * signal;
                    weight *= signal * 2.0f;

                    frequency *= lacunarity;
                    amplitude *= persistence;
                }

                return result * 2.0f - 1.0f;
            }

            case FractalType::DeCarpentier:
            {
                // De Carpentier voxel terrain formula
                float offset = 1.0f;
                float gain = 2.0f;

                result = generateSingle(noiseType, x, y, z);
                float signal = result;
                float weight = 1.0f;

                for (int i = 1; i < octaves; i++)
                {
                    weight = std::min(1.0f, std::max(0.0f, offset - signal * gain));

                    signal = generateSingle(noiseType, x * frequency, y * frequency, z * frequency);
                    result += weight * signal * amplitude;

                    frequency *= lacunarity;
                    amplitude *= persistence;
                }

                return result;
            }

            default:
                return generateSingle(noiseType, x, y, z);
        }
    }

    float NoiseGenerator::computeWarp(WarpType type, float x, float y, float z, float amplitude, float frequency) const
    {
        switch (type)
        {
            case WarpType::Basic:
            {
                // Basic domain warping using Perlin noise
                return Utility::Math::perlinNoise(x * frequency, y * frequency, z * frequency) * amplitude;
            }

            case WarpType::Derivative:
            {
                // Derivative-based warping
                float epsilon = 0.01f;
                float dx = Utility::Math::perlinNoise(x + epsilon, y, z) - Utility::Math::perlinNoise(x - epsilon, y, z);
                float dy = Utility::Math::perlinNoise(x, y + epsilon, z) - Utility::Math::perlinNoise(x, y - epsilon, z);
                float dz = Utility::Math::perlinNoise(x, y, z + epsilon) - Utility::Math::perlinNoise(x, y, z - epsilon);

                return (dx + dy + dz) * amplitude / 3.0f;
            }

            case WarpType::Fractal:
            {
                // Fractal warping
                float result = 0.0f;
                float amp = amplitude;
                float freq = frequency;

                for (int i = 0; i < 3; i++)
                {
                    result += Utility::Math::perlinNoise(x * freq, y * freq, z * freq) * amp;
                    freq *= 2.0f;
                    amp *= 0.5f;
                }

                return result;
            }

            default:
                return 0.0f;
        }
    }

    float NoiseGenerator::interpolate(InterpolationType type, float a, float b, float t) const
    {
        switch (type)
        {
            case InterpolationType::Linear:
                return a + t * (b - a);

            case InterpolationType::Cubic:
            {
                float t2 = t * t;
                float t3 = t2 * t;
                return a + t2 * (3.0f - 2.0f * t) * (b - a);
            }

            case InterpolationType::Quintic:
            {
                float t2 = t * t;
                float t3 = t2 * t;
                return a + t3 * (t * (t * 6.0f - 15.0f) + 10.0f) * (b - a);
            }

            case InterpolationType::Hermite:
            {
                float t2 = t * t;
                float t3 = t2 * t;
                float h1 = 2.0f * t3 - 3.0f * t2 + 1.0f;
                float h2 = -2.0f * t3 + 3.0f * t2;
                return a * h2 + b * h2;
            }

            default:
                return a + t * (b - a);
        }
    }

    float NoiseGenerator::combineValues(float a, float b, CombineOperation op, float weight) const
    {
        switch (op)
        {
            case CombineOperation::Add:
                return a + b * weight;

            case CombineOperation::Subtract:
                return a - b * weight;

            case CombineOperation::Multiply:
                return a * (1.0f + b * weight);

            case CombineOperation::Divide:
                if (std::abs(b) < 0.0001f)
                {
                    return a;
                }
                return a / (1.0f + std::abs(b) * weight);

            case CombineOperation::Min:
                return std::min(a, b);

            case CombineOperation::Max:
                return std::max(a, b);

            case CombineOperation::Power:
                return std::pow(std::abs(a), 1.0f + b * weight) * (a < 0 ? -1.0f : 1.0f);

            case CombineOperation::Average:
                return (a + b * weight) / (1.0f + weight);

            case CombineOperation::Blend:
                float t = (b + 1.0f) * 0.5f * weight; // Map b from [-1,1] to [0,1]
                t = std::max(0.0f, std::min(1.0f, t));
                return a * (1.0f - t) + b * t;
        }

        return a;
    }

    glm::vec3 NoiseGenerator::applyScaleAndOffset(float x, float y, float z, const glm::vec3& scale, const glm::vec3& offset) const
    {
        return glm::vec3(
            x * scale.x + offset.x,
            y * scale.y + offset.y,
            z * scale.z + offset.z
        );
    }

    void NoiseGenerator::validateParameters()
    {
        // Ensure parameters are within valid ranges
        m_octaves = std::max(1, std::min(10, m_octaves));
        m_persistence = std::max(0.0f, std::min(1.0f, m_persistence));
        m_lacunarity = std::max(1.0f, std::min(4.0f, m_lacunarity));

        // Ensure scale is non-zero
        if (m_scale.x < 0.0001f) m_scale.x = 0.0001f;
        if (m_scale.y < 0.0001f) m_scale.y = 0.0001f;
        if (m_scale.z < 0.0001f) m_scale.z = 0.0001f;

        // Validate all layers
        for (auto& layer : m_layers)
        {
            layer.octaves = std::max(1, std::min(10, layer.octaves));
            layer.persistence = std::max(0.0f, std::min(1.0f, layer.persistence));
            layer.lacunarity = std::max(1.0f, std::min(4.0f, layer.lacunarity));
            layer.weight = std::max(0.0f, std::min(1.0f, layer.weight));

            if (layer.scale.x < 0.0001f) layer.scale.x = 0.0001f;
            if (layer.scale.y < 0.0001f) layer.scale.y = 0.0001f;
            if (layer.scale.z < 0.0001f) layer.scale.z = 0.0001f;
        }
    }

    // Helper function for value noise
    float NoiseGenerator::valueNoise(int x, int y, int z) const
    {
        uint32_t n = x + y * 57 + z * 131;
        n = (n << 13) ^ n;
        n = n * (n * n * 15731 + 789221) + 1376312589;
        return 1.0f - ((n & 0x7fffffff) / 1073741824.0f);
    }

} // namespace PixelCraft::Voxel