// -------------------------------------------------------------------------
// NoiseGenerator.cpp
// -------------------------------------------------------------------------
#include "Voxel/NoiseGenerator.h"
#include "Voxel/NoiseLayer.h"
#include "Core/Logger.h"
#include "Utility/Math.h"
#include "Utility/Random.h"
#include "Utility/Serialization/SerializationUtility.h"

#include <algorithm>
#include <random>
#include <cmath>

namespace PixelCraft::Voxel
{

    NoiseGenerator::NoiseGenerator()
    {
        // Nothing to initialize
    }

    NoiseGenerator::~NoiseGenerator()
    {
        // Nothing to clean up
    }

    float NoiseGenerator::generate(float x, float y, float z) const
    {
        if (m_layers.empty())
        {
            Core::error("NoiseGenerator::generate: Missing noise layers for noise generation");
            return 0.0f;
        }

        float result = 0.0f;
        float totalWeight = 0.0f;

        // Start with the first layer as the base
        bool firstLayer = true;

        for (auto [id, layer] : m_layers)
        {
            if (!layer->isLayerEnabled())
            {
                continue;
            }

            // Apply scale and offset to coordinates
            glm::vec3 pos = applyScaleAndOffset(x, y, z, layer->getLayerScale(), layer->getLayerOffset());

            // Apply domain warping if enabled
            if (layer->isWarpEnabled())
            {
                float warpX = computeWarp(layer->getWarpType(), pos.x, pos.y, pos.z, layer->getWarpAmplitude(), layer->getWarpFrequency());
                float warpY = computeWarp(layer->getWarpType(), pos.x + 123.456f, pos.y + 789.012f, pos.z + 345.678f, layer->getWarpAmplitude(), layer->getWarpFrequency());
                float warpZ = computeWarp(layer->getWarpType(), pos.x + 901.234f, pos.y + 567.890f, pos.z + 123.456f, layer->getWarpAmplitude(), layer->getWarpFrequency());

                pos.x += warpX;
                pos.y += warpY;
                pos.z += warpZ;
            }

            // Apply the layer's scale and offset
            glm::vec3 layerPos = applyScaleAndOffset(pos.x, pos.y, pos.z, layer->getLayerScale(), layer->getLayerOffset());

            // Apply the layer's warping if enabled
            if (layer->isWarpEnabled())
            {
                float warpX = computeWarp(layer->getWarpType(), layerPos.x, layerPos.y, layerPos.z,
                                            layer->getWarpAmplitude(), layer->getWarpFrequency());
                float warpY = computeWarp(layer->getWarpType(), layerPos.x + 123.456f, layerPos.y + 789.012f,
                                            layerPos.z + 345.678f, layer->getWarpAmplitude(), layer->getWarpFrequency());
                float warpZ = computeWarp(layer->getWarpType(), layerPos.x + 901.234f, layerPos.y + 567.890f,
                                            layerPos.z + 123.456f, layer->getWarpAmplitude(), layer->getWarpFrequency());

                layerPos.x += warpX;
                layerPos.y += warpY;
                layerPos.z += warpZ;
            }

            // Generate noise value for this layer
            float layerValue;
            if (layer->getLayerFractalType() == NoiseLayer::FractalType::None)
            {
                layerValue = generateSingle(layer->getLayerNoiseType(), layerPos.x * layer->getLayerFrequency(),
                                            layerPos.y * layer->getLayerFrequency(), layerPos.z * layer->getLayerFrequency());
            }
            else
            {
                layerValue = generateFractal(layer->getLayerNoiseType(), layer->getLayerFractalType(),
                                                layerPos.x * layer->getLayerFrequency(), layerPos.y * layer->getLayerFrequency(),
                                                layerPos.z * layer->getLayerFrequency(), layer->getLayerOctaves(),
                                                layer->getLayerPersistence(), layer->getLayerLacunarity());
            }

            // Scale by amplitude
            layerValue *= layer->getLayerAmplitude();

            // Combine with previous result
            if (firstLayer)
            {
                result = layerValue;
                firstLayer = false;
            }
            else
            {
                result = combineValues(result, layerValue, layer->getLayerCombineOperation(), layer->getLayerWeight());
            }

            totalWeight += layer->getLayerWeight();
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

    std::shared_ptr<NoiseLayer> NoiseGenerator::createLayer(const std::string& name)
    {
        if (name.empty())
        {
            Core::error("Layer name cannot be empty");
            return nullptr;
        }

        // Check if layer already exists
        if (m_layerNameToId.find(name) != m_layerNameToId.end())
        {
            Core::warn("Layer '" + name + "' already exists");
            return nullptr;
        }

        // Generate new ID
        int id = getNextLayerId();

        // Create new layer with default values
        auto layer = std::make_shared<NoiseLayer>(name, true);

        // Add to layers list
        m_layers[id] = layer;
        m_layerNameToId[name] = id;

        return layer;
    }

    bool NoiseGenerator::removeLayer(int layerId)
    {
        auto it = m_layers.find(layerId);
        if (it == m_layers.end())
        {
            return false;
        }

        // Remove from name map first
        m_layerNameToId.erase(it->second->getLayerName());

        // Remove from layers map
        m_layers.erase(it);

        return true;
    }

    std::vector<std::string> NoiseGenerator::getLayerNames() const
    {
        std::vector<std::string> names;
        names.reserve(m_layers.size());

        for (auto [_, layer] : m_layers)
        {
            names.push_back(layer->getLayerName());
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
        m_layerNameToId.clear();

        // Create base terrain layer
        std::shared_ptr<NoiseLayer> base = createLayer("base");
        base->setLayerNoiseType(NoiseLayer::NoiseType::Perlin);
        base->setLayerFractalType(NoiseLayer::FractalType::FBM);
        base->setLayerOctaves(4);
        base->setLayerPersistence(0.5f);
        base->setLayerLacunarity(2.0f);
        base->setLayerFrequency(0.01f);
        base->setLayerAmplitude(1.0f);
        base->setLayerWeight(1.0f);

        // Create detail layer
        std::shared_ptr<NoiseLayer> detail = createLayer("detail");
        detail->setLayerNoiseType(NoiseLayer::NoiseType::Perlin);
        detail->setLayerFractalType(NoiseLayer::FractalType::FBM);
        detail->setLayerOctaves(6);
        detail->setLayerPersistence(0.4f);
        detail->setLayerLacunarity(2.5f);
        detail->setLayerFrequency(0.04f);
        detail->setLayerAmplitude(0.2f);
        detail->setLayerCombineOperation(NoiseLayer::CombineOperation::Add);
        detail->setLayerWeight(0.5f);

        if (mountainous)
        {
            // Create ridge layer for mountains
            std::shared_ptr<NoiseLayer> ridges = createLayer("ridges");
            ridges->setLayerNoiseType(NoiseLayer::NoiseType::Ridged);
            ridges->setLayerFractalType(NoiseLayer::FractalType::FBM);
            ridges->setLayerOctaves(3);
            ridges->setLayerPersistence(0.7f);
            ridges->setLayerLacunarity(2.3f);
            ridges->setLayerFrequency(0.008f);
            ridges->setLayerAmplitude(0.8f);
            ridges->setLayerCombineOperation(NoiseLayer::CombineOperation::Add);
            ridges->setLayerWeight(0.7f);

            // Add warping to make mountains more interesting
            ridges->setWarpEnabled(true);
            ridges->setWarpType(NoiseLayer::WarpType::Fractal);
            ridges->setWarpAmplitude(20.0f);
            ridges->setWarpFrequency(0.005f);
        }

        return true;
    }

    bool NoiseGenerator::presetCaves()
    {
        // Clear existing layers
        m_layers.clear();
        m_layerNameToId.clear();

        // Create base cave layer
        std::shared_ptr<NoiseLayer> base = createLayer("base");
        base->setLayerNoiseType(NoiseLayer::NoiseType::Perlin);
        base->setLayerFractalType(NoiseLayer::FractalType::FBM);
        base->setLayerOctaves(3);
        base->setLayerPersistence(0.4f);
        base->setLayerLacunarity(2.0f);
        base->setLayerFrequency(0.05f);
        base->setLayerAmplitude(1.0f);
        base->setLayerWeight(1.0f);

        // Create cave shape layer
        std::shared_ptr<NoiseLayer> tunnels = createLayer("tunnels");
        tunnels->setLayerNoiseType(NoiseLayer::NoiseType::Worley);
        tunnels->setLayerFractalType(NoiseLayer::FractalType::None);
        tunnels->setLayerFrequency(0.03f);
        tunnels->setLayerAmplitude(1.0f);
        tunnels->setLayerCombineOperation(NoiseLayer::CombineOperation::Multiply);
        tunnels->setLayerWeight(0.8f);

        // Add warping for more natural cave systems
        tunnels->setWarpEnabled(true);
        tunnels->setWarpType(NoiseLayer::WarpType::Basic);
        tunnels->setWarpAmplitude(5.0f);
        tunnels->setWarpFrequency(0.02f);

        return true;
    }

    bool NoiseGenerator::presetOreDistribution()
    {
        // Clear existing layers
        m_layers.clear();
        m_layerNameToId.clear();

        // Create base layer for overall distribution
        std::shared_ptr<NoiseLayer> distribution = createLayer("distribution");
        distribution->setLayerNoiseType(NoiseLayer::NoiseType::Perlin);
        distribution->setLayerFractalType(NoiseLayer::FractalType::FBM);
        distribution->setLayerOctaves(2);
        distribution->setLayerPersistence(0.5f);
        distribution->setLayerLacunarity(2.0f);
        distribution->setLayerFrequency(0.1f);
        distribution->setLayerAmplitude(1.0f);
        distribution->setLayerWeight(1.0f);

        // Create vein layer for ore veins
        std::shared_ptr<NoiseLayer> veins = createLayer("veins");
        veins->setLayerNoiseType(NoiseLayer::NoiseType::Worley);
        veins->setLayerFractalType(NoiseLayer::FractalType::None);
        veins->setLayerFrequency(0.2f);
        veins->setLayerAmplitude(1.0f);
        veins->setLayerCombineOperation(NoiseLayer::CombineOperation::Multiply);
        veins->setLayerWeight(0.6f);

        // Create detail layer for smaller deposits
        std::shared_ptr<NoiseLayer> deposits = createLayer("deposits");
        deposits->setLayerNoiseType(NoiseLayer::NoiseType::Value);
        deposits->setLayerFractalType(NoiseLayer::FractalType::FBM);
        deposits->setLayerOctaves(3);
        deposits->setLayerPersistence(0.3f);
        deposits->setLayerLacunarity(2.5f);
        deposits->setLayerFrequency(0.3f);
        deposits->setLayerAmplitude(0.5f);
        deposits->setLayerCombineOperation(NoiseLayer::CombineOperation::Add);
        deposits->setLayerWeight(0.4f);

        return true;
    }

    bool NoiseGenerator::presetBiomeBlend()
    {
        // Clear existing layers
        m_layers.clear();
        m_layerNameToId.clear();

        // Create temperature layer
        std::shared_ptr<NoiseLayer> temperature = createLayer("temperature");
        temperature->setLayerNoiseType(NoiseLayer::NoiseType::Perlin);
        temperature->setLayerFractalType(NoiseLayer::FractalType::FBM);
        temperature->setLayerOctaves(3);
        temperature->setLayerPersistence(0.5f);
        temperature->setLayerLacunarity(2.0f);
        temperature->setLayerFrequency(0.01f);
        temperature->setLayerAmplitude(1.0f);
        temperature->setLayerWeight(1.0f);

        // Create humidity layer
        std::shared_ptr<NoiseLayer> humidity = createLayer("humidity");
        humidity->setLayerNoiseType(NoiseLayer::NoiseType::Perlin);
        humidity->setLayerFractalType(NoiseLayer::FractalType::FBM);
        humidity->setLayerOctaves(3);
        humidity->setLayerPersistence(0.5f);
        humidity->setLayerLacunarity(2.0f);
        humidity->setLayerFrequency(0.01f);
        humidity->setLayerAmplitude(1.0f);
        humidity->setLayerOffset(glm::vec3(123.0f, 456.0f, 789.0f)); // Offset to make it different from temperature
        humidity->setLayerCombineOperation(NoiseLayer::CombineOperation::Blend);
        humidity->setLayerWeight(1.0f);

        // Create altitude variation layer
        std::shared_ptr<NoiseLayer> altitude = createLayer("altitude");
        altitude->setLayerNoiseType(NoiseLayer::NoiseType::Perlin);
        altitude->setLayerFractalType(NoiseLayer::FractalType::FBM);
        altitude->setLayerOctaves(4);
        altitude->setLayerPersistence(0.6f);
        altitude->setLayerLacunarity(2.2f);
        altitude->setLayerFrequency(0.02f);
        altitude->setLayerAmplitude(0.7f);
        altitude->setLayerOffset(glm::vec3(987.0f, 654.0f, 321.0f));
        altitude->setLayerCombineOperation(NoiseLayer::CombineOperation::Multiply);
        altitude->setLayerWeight(0.5f);

        return true;
    }

    bool NoiseGenerator::presetDetailTexture()
    {
        // Clear existing layers
        m_layers.clear();
        m_layerNameToId.clear();

        // Create base detail layer
        std::shared_ptr<NoiseLayer> base = createLayer("base");
        base->setLayerNoiseType(NoiseLayer::NoiseType::Value);
        base->setLayerFractalType(NoiseLayer::FractalType::FBM);
        base->setLayerOctaves(5);
        base->setLayerPersistence(0.5f);
        base->setLayerLacunarity(2.0f);
        base->setLayerFrequency(0.2f);
        base->setLayerAmplitude(1.0f);
        base->setLayerWeight(1.0f);

        // Create fine detail layer
        std::shared_ptr<NoiseLayer> fine = createLayer("fine");
        fine->setLayerNoiseType(NoiseLayer::NoiseType::Perlin);
        fine->setLayerFractalType(NoiseLayer::FractalType::FBM);
        fine->setLayerOctaves(7);
        fine->setLayerPersistence(0.6f);
        fine->setLayerLacunarity(2.5f);
        fine->setLayerFrequency(0.5f);
        fine->setLayerAmplitude(0.3f);
        fine->setLayerCombineOperation(NoiseLayer::CombineOperation::Add);
        fine->setLayerWeight(0.4f);

        // Create spots layer
        std::shared_ptr<NoiseLayer> spots = createLayer("spots");
        spots->setLayerNoiseType(NoiseLayer::NoiseType::Worley);
        spots->setLayerFractalType(NoiseLayer::FractalType::None);
        spots->setLayerFrequency(0.4f);
        spots->setLayerAmplitude(0.5f);
        spots->setLayerCombineOperation(NoiseLayer::CombineOperation::Multiply);
        spots->setLayerWeight(0.3f);

        return true;
    }

    Utility::Serialization::SerializationResult NoiseGenerator::serialize(Utility::Serialization::Serializer& serializer) const
    {
        auto result = serializer.beginObject(getTypeName());
        if (!result) return result;

        // Serialize seed
        result = serializer.writeField("seed", m_seed);
        if (!result) return result;

        // Serialize layers
        result = serializer.beginArray("layers", m_layers.size());
        if (!result) return result;

        for (auto& [_, layer] : m_layers)
        {
            result = layer->serialize(serializer);
            if (!result) return result;
        }

        result = serializer.endArray();
        if (!result) return result;

        return serializer.endObject();
    }

    Utility::Serialization::SerializationResult NoiseGenerator::deserialize(Utility::Serialization::Deserializer& deserializer)
    {
        // Clear existing data
        m_layers.clear();
        m_layerNameToId.clear();

        auto result = deserializer.beginObject(getTypeName());
        if (!result) return result;

        // Deserialize seed
        result = deserializer.readField("seed", m_seed);
        if (!result) return result;

        // Deserialize layers
        size_t arraySize;
        result = deserializer.beginArray("layers", arraySize);
        if (!result) return result;

        for (size_t i = 0; i < arraySize; i++)
        {
            std::shared_ptr<NoiseLayer> layer;

            result = layer->deserialize(deserializer);
            if (!result) return result;
        }

        result = deserializer.endArray();
        if (!result) return result;

        return deserializer.endObject();
    }

    void NoiseGenerator::defineSchema(Utility::Serialization::Schema& schema)
    {
        schema.addField("seed", Utility::Serialization::ValueType::UInt32, true);
        schema.addArrayField("layers", Utility::Serialization::ValueType::Object, true);
    }

    std::string NoiseGenerator::getTypeName()
    {
        return "NoiseGenerator";
    }

    int NoiseGenerator::getNextLayerId() const
    {
        int maxId = 0;

        for (const auto& [id, _] : m_layers)
        {
            maxId = std::max(maxId, id);
        }

        return maxId + 1;
    }

    // Private helper methods

    float NoiseGenerator::generateSingle(NoiseLayer::NoiseType type, float x, float y, float z, NoiseLayer::InterpolationType interpType) const
    {
        switch (type)
        {
            case NoiseLayer::NoiseType::Perlin:
                return Utility::Math::perlinNoise(x, y, z);

            case NoiseLayer::NoiseType::Simplex:
                return Utility::Math::simplexNoise(x, y, z);

            case NoiseLayer::NoiseType::Worley:
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

            case NoiseLayer::NoiseType::Value:
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

                switch (interpType)
                {
                    case NoiseLayer::InterpolationType::Linear:
                        tx = sx;
                        ty = sy;
                        tz = sz;
                        break;

                    case NoiseLayer::InterpolationType::Cubic:
                        tx = sx * sx * (3 - 2 * sx);
                        ty = sy * sy * (3 - 2 * sy);
                        tz = sz * sz * (3 - 2 * sz);
                        break;

                    case NoiseLayer::InterpolationType::Quintic:
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
                    interpType,
                    interpolate(
                        interpType,
                        interpolate(interpType, n000, n100, tx),
                        interpolate(interpType, n010, n110, tx),
                        ty
                    ),
                    interpolate(
                        interpType,
                        interpolate(interpType, n001, n101, tx),
                        interpolate(interpType, n011, n111, tx),
                        ty
                    ),
                    tz
                );
            }

            case NoiseLayer::NoiseType::Cubic:
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
                    interpType,
                    interpolate(
                        interpType,
                        interpolate(interpType, n000, n100, tx),
                        interpolate(interpType, n010, n110, tx),
                        ty
                    ),
                    interpolate(
                        interpType,
                        interpolate(interpType, n001, n101, tx),
                        interpolate(interpType, n011, n111, tx),
                        ty
                    ),
                    tz
                );
            }

            case NoiseLayer::NoiseType::WhiteNoise:
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

            case NoiseLayer::NoiseType::Ridged:
            {
                // Ridged multi-fractal noise based on Perlin
                float noise = std::abs(Utility::Math::perlinNoise(x, y, z));
                return 2.0f * (0.5f - noise);
            }
            
            case NoiseLayer::NoiseType::Billow:
            {
                // Billow noise based on Perlin
                float noise = std::abs(Utility::Math::perlinNoise(x, y, z));
                return 2.0f * noise - 1.0f;
            }

            case NoiseLayer::NoiseType::Voronoi:
            {
                // Voronoi noise implementation (similar to Worley but different distance metric)
                constexpr int primes[3] = {15731, 789221, 1376312589};

                // Determine cell coordinates
                int cellX = static_cast<int>(std::floor(x));
                int cellY = static_cast<int>(std::floor(y));
                int cellZ = static_cast<int>(std::floor(z));

                float minDist = 1000.0f;
                glm::vec3 closestPoint(0, 0, 0);

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

            case NoiseLayer::NoiseType::Domain:
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

    float NoiseGenerator::generateFractal(NoiseLayer::NoiseType noiseType, NoiseLayer::FractalType fractalType, float x, float y, float z,
                                          int octaves, float persistence, float lacunarity) const
    {
        float result = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float maxValue = 0.0f;

        switch (fractalType)
        {
            case NoiseLayer::FractalType::FBM:
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

            case NoiseLayer::FractalType::Rigid:
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

            case NoiseLayer::FractalType::Billow:
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

            case NoiseLayer::FractalType::MultiFractal:
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

            case NoiseLayer::FractalType::HybridMulti:
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

            case NoiseLayer::FractalType::DeCarpentier:
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

    float NoiseGenerator::computeWarp(NoiseLayer::WarpType type, float x, float y, float z, float amplitude, float frequency) const
    {
        switch (type)
        {
            case NoiseLayer::WarpType::Basic:
            {
                // Basic domain warping using Perlin noise
                return Utility::Math::perlinNoise(x * frequency, y * frequency, z * frequency) * amplitude;
            }

            case NoiseLayer::WarpType::Derivative:
            {
                // Derivative-based warping
                float epsilon = 0.01f;
                float dx = Utility::Math::perlinNoise(x + epsilon, y, z) - Utility::Math::perlinNoise(x - epsilon, y, z);
                float dy = Utility::Math::perlinNoise(x, y + epsilon, z) - Utility::Math::perlinNoise(x, y - epsilon, z);
                float dz = Utility::Math::perlinNoise(x, y, z + epsilon) - Utility::Math::perlinNoise(x, y, z - epsilon);

                return (dx + dy + dz) * amplitude / 3.0f;
            }

            case NoiseLayer::WarpType::Fractal:
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

    float NoiseGenerator::interpolate(NoiseLayer::InterpolationType type, float a, float b, float t) const
    {
        switch (type)
        {
            case NoiseLayer::InterpolationType::Linear:
                return a + t * (b - a);

            case NoiseLayer::InterpolationType::Cubic:
            {
                float t2 = t * t;
                return a + t2 * (3.0f - 2.0f * t) * (b - a);
            }

            case NoiseLayer::InterpolationType::Quintic:
            {
                float t2 = t * t;
                float t3 = t2 * t;
                return a + t3 * (t * (t * 6.0f - 15.0f) + 10.0f) * (b - a);
            }

            case NoiseLayer::InterpolationType::Hermite:
            {
                float t2 = t * t;
                float t3 = t2 * t;
                float h1 = 2.0f * t3 - 3.0f * t2 + 1.0f;
                float h2 = -2.0f * t3 + 3.0f * t2;
                return a * h1 + b * h2;
            }

            default:
                return a + t * (b - a);
        }
    }

    float NoiseGenerator::combineValues(float a, float b, NoiseLayer::CombineOperation op, float weight) const
    {
        switch (op)
        {
            case NoiseLayer::CombineOperation::Add:
                return a + b * weight;

            case NoiseLayer::CombineOperation::Subtract:
                return a - b * weight;

            case NoiseLayer::CombineOperation::Multiply:
                return a * (1.0f + b * weight);

            case NoiseLayer::CombineOperation::Divide:
                if (std::abs(b) < 0.0001f)
                {
                    return a;
                }
                return a / (1.0f + std::abs(b) * weight);

            case NoiseLayer::CombineOperation::Min:
                return std::min(a, b);

            case NoiseLayer::CombineOperation::Max:
                return std::max(a, b);

            case NoiseLayer::CombineOperation::Power:
                return std::pow(std::abs(a), 1.0f + b * weight) * (a < 0 ? -1.0f : 1.0f);

            case NoiseLayer::CombineOperation::Average:
                return (a + b * weight) / (1.0f + weight);

            case NoiseLayer::CombineOperation::Blend:
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
        // Validate all layers
        for (auto& [id, layer] : m_layers)
        {
            layer->setLayerOctaves(std::max(1, std::min(10, layer->getLayerOctaves())));
            layer->setLayerPersistence(std::max(0.0f, std::min(1.0f, layer->getLayerPersistence())));
            layer->setLayerLacunarity(std::max(1.0f, std::min(4.0f, layer->getLayerLacunarity())));
            layer->setLayerWeight(std::max(0.0f, std::min(1.0f, layer->getLayerWeight())));

            // Ensure scale is non-zero
            glm::vec3 layerScale = layer->getLayerScale();
            if (layer->getLayerScale().x < 0.0001f) layer->setLayerScale(glm::vec3(0.0001f, layerScale.y, layerScale.z));
            if (layer->getLayerScale().y < 0.0001f) layer->setLayerScale(glm::vec3(layerScale.x, 0.0001f, layerScale.z));
            if (layer->getLayerScale().z < 0.0001f) layer->setLayerScale(glm::vec3(layerScale.x, layerScale.y, 0.0001f));
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