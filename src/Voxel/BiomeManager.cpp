// -------------------------------------------------------------------------
// BiomeManager.cpp
// -------------------------------------------------------------------------
#include "Voxel/BiomeManager.h"
#include "Voxel/Chunk.h"
#include "Voxel/ProceduralGenerationSystem.h"
#include "Voxel/GenerationParameters.h"
#include "Voxel/NoiseGenerator.h"
#include "Core/Logger.h"
#include "Utility/Random.h"
#include "Utility/Math.h"
#include "Utility/Serialization/SerializationUtility.h"

#include <algorithm>
#include <random>
#include <cmath>

namespace PixelCraft::Voxel
{

    BiomeManager::BiomeManager()
    {
        // Initialize some default biomes
        registerBiome("Plains");
        registerBiome("Forest");
        registerBiome("Desert");
        registerBiome("Mountains");
        registerBiome("Ocean");
    }

    BiomeManager::~BiomeManager()
    {
        // Nothing specific to clean up
    }

    std::shared_ptr<Biome> BiomeManager::registerBiome(const std::string& name)
    {
        // Check if already registered
        auto it = m_biomeNameToId.find(name);
        if (it != m_biomeNameToId.end())
        {
            Core::warn("Biome '" + name + "' already registered");
            return getBiome(it->second);
        }

        // Generate new ID
        int id = getNextBiomeId();

        // Create biome info
        auto biome = std::make_shared<Biome>(name, id);

        // Register it
        m_biomes[id] = biome;
        m_biomeNameToId[name] = id;

        return biome;
    }

    bool BiomeManager::removeBiome(int biomeId)
    {
        auto it = m_biomes.find(biomeId);
        if (it == m_biomes.end())
        {
            return false;
        }

        // Remove from name map first
        m_biomeNameToId.erase(it->second->getBiomeName());

        // Remove from biomes map
        m_biomes.erase(it);

        return true;
    }

    std::shared_ptr<Biome> BiomeManager::getBiome(int biomeId) const
    {
        auto it = m_biomes.find(biomeId);
        if (it == m_biomes.end())
        {
            return nullptr;
        }

        return it->second;
    }

    std::shared_ptr<Biome> BiomeManager::getBiome(const std::string& name) const
    {
        auto it = m_biomeNameToId.find(name);
        if (it == m_biomeNameToId.end())
        {
            return nullptr;
        }

        return getBiome(it->second);
    }

    std::vector<int> BiomeManager::getBiomeIds() const
    {
        std::vector<int> ids;
        ids.reserve(m_biomes.size());

        for (const auto& [id, _] : m_biomes)
        {
            ids.push_back(id);
        }

        return ids;
    }

    std::vector<std::string> BiomeManager::getBiomeNames() const
    {
        std::vector<std::string> names;
        names.reserve(m_biomes.size());

        for (const auto& [_, biome] : m_biomes)
        {
            names.push_back(biome->getBiomeName());
        }

        return names;
    }

    int BiomeManager::getBiomeAt(const glm::vec3& position, const GenerationContext& context) const
    {
        if (m_biomes.empty())
        {
            return -1;
        }

        // Get biome influences at this point
        auto influences = getBiomeInfluence(position, context);

        // Find the biome with the highest influence
        int maxBiomeId = -1;
        float maxInfluence = 0.0f;

        for (const auto& [biomeId, influence] : influences)
        {
            if (influence > maxInfluence)
            {
                maxInfluence = influence;
                maxBiomeId = biomeId;
            }
        }

        return maxBiomeId;
    }

    std::unordered_map<int, float> BiomeManager::getBiomeInfluence(const glm::vec3& position, const GenerationContext& context) const
    {
        std::unordered_map<int, float> influences;

        // If no biomes or no noise generator, return empty
        if (m_biomes.empty() || !context.noiseGenerator)
        {
            return influences;
        }

        // Get temperature and humidity at this position
        float temperature = 0.5f;
        float humidity = 0.5f;

        // Use temperature and humidity noise if specified
        if (!m_temperatureNoiseLayer.empty() && context.parameters)
        {
            std::shared_ptr<NoiseGenerator> temperatureNoise = nullptr;

            // Try to get the noise generator from the procedural generation system
            // Since we don't have direct access, we'll just use the context's noise generator
            temperatureNoise = context.noiseGenerator;

            if (temperatureNoise)
            {
                temperature = temperatureNoise->generate2D(position.x, position.z);
                temperature = temperature * 0.5f + 0.5f; // Normalize to [0,1]
            }
        }

        if (!m_humidityNoiseLayer.empty() && context.parameters)
        {
            std::shared_ptr<NoiseGenerator> humidityNoise = nullptr;

            // Try to get the noise generator from the procedural generation system
            humidityNoise = context.noiseGenerator;

            if (humidityNoise)
            {
                humidity = humidityNoise->generate2D(position.x + 1000.0f, position.z + 1000.0f);
                humidity = humidity * 0.5f + 0.5f; // Normalize to [0,1]
            }
        }

        // Calculate elevation (normalized to [0,1])
        float elevation = position.y;
        if (context.parameters)
        {
            float minHeight = context.parameters->getMinHeight();
            float maxHeight = context.parameters->getMaxHeight();

            if (maxHeight > minHeight)
            {
                elevation = (position.y - minHeight) / (maxHeight - minHeight);
                elevation = std::max(0.0f, std::min(1.0f, elevation));
            }
        }

        // Calculate influence for each enabled biome
        float totalInfluence = 0.0f;

        for (const auto& [id, biome] : m_biomes)
        {
            if (!biome->isBiomeEnabled())
            {
                continue;
            }

            float influence = calculateBiomeInfluence(id, position, temperature, humidity, elevation, context);

            if (influence > 0.0f)
            {
                influences[id] = influence;
                totalInfluence += influence;
            }
        }

        // Normalize influences
        if (totalInfluence > 0.0f)
        {
            for (auto& [_, influence] : influences)
            {
                influence /= totalInfluence;
            }
        }

        return influences;
    }

    void BiomeManager::setBlendDistance(float distance)
    {
        m_blendDistance = std::max(0.0f, distance);
    }

    float BiomeManager::getBlendDistance() const
    {
        return m_blendDistance;
    }

    bool BiomeManager::generateBiomeData(const ChunkCoord& chunkCoord, const GenerationContext& context, Chunk& chunk)
    {
        if (m_biomes.empty() || !context.noiseGenerator)
        {
            return false;
        }

        int chunkSize = chunk.getSize();

        // Get chunk world position
        glm::vec3 chunkWorldPos = chunkCoord.toWorldPosition(chunkSize);

        // For each voxel in the chunk
        for (int x = 0; x < chunkSize; x++)
        {
            for (int z = 0; z < chunkSize; z++)
            {
                // Sample at surface level (for efficiency, we don't need to process every voxel)
                // Find the highest non-air voxel
                int surfaceY = -1;

                for (int y = chunkSize - 1; y >= 0; y--)
                {
                    Voxel voxel = chunk.getVoxel(x, y, z);
                    if (voxel.type != 0)
                    { // Not air
                        surfaceY = y;
                        break;
                    }
                }

                if (surfaceY < 0)
                {
                    continue; // No surface found in this column
                }

                // Calculate world position
                glm::vec3 worldPos = chunkWorldPos + glm::vec3(x, surfaceY, z);

                // Get biome influences at this position
                auto influences = getBiomeInfluence(worldPos, context);

                // Apply biome-specific modifications to the voxel column
                for (int y = 0; y <= surfaceY; y++)
                {
                    Voxel voxel = chunk.getVoxel(x, y, z);

                    if (voxel.type == 0)
                    {
                        continue; // Skip air voxels
                    }

                    // Create a world position for this voxel
                    glm::vec3 voxelPos = chunkWorldPos + glm::vec3(x, y, z);

                    // Based on influences, determine voxel type
                    bool voxelModified = false;

                    // If it's the surface voxel, apply biome-specific surface block
                    if (y == surfaceY)
                    {
                        // Find the dominant biome
                        int dominantBiome = -1;
                        float maxInfluence = 0.0f;

                        for (const auto& [biomeId, influence] : influences)
                        {
                            if (influence > maxInfluence)
                            {
                                maxInfluence = influence;
                                dominantBiome = biomeId;
                            }
                        }

                        if (dominantBiome >= 0)
                        {
                            auto biome = getBiome(dominantBiome);
                            if (biome)
                            {
                                // Apply biome-specific surface block
                                // Example: use data value to store biome ID
                                voxel.data = static_cast<uint16_t>(dominantBiome);

                                // Apply based on voxel probabilities
                                // This is simplified and would be more complex in a real implementation
                                for (const auto& [voxelType, probability] : biome->getVoxelProbabilities())
                                {
                                    if (probability > 0.0f)
                                    {
                                        // Convert voxel type name to ID (simplified)
                                        uint16_t voxelTypeId = static_cast<uint16_t>(std::stoi(voxelType));

                                        // Apply with probability
                                        Utility::Random rand(context.seed);
                                        if (rand.getFloat(0.0f, 1.0f) < probability)
                                        {
                                            voxel.type = voxelTypeId;
                                            voxelModified = true;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Apply the modified voxel
                    if (voxelModified)
                    {
                        chunk.setVoxel(x, y, z, voxel);
                    }
                }
            }
        }

        return true;
    }

    void BiomeManager::setTemperatureNoiseLayer(const std::string& noiseLayerId)
    {
        m_temperatureNoiseLayer = noiseLayerId;
    }

    std::string BiomeManager::getTemperatureNoiseLayer() const
    {
        return m_temperatureNoiseLayer;
    }

    void BiomeManager::setHumidityNoiseLayer(const std::string& noiseLayerId)
    {
        m_humidityNoiseLayer = noiseLayerId;
    }

    std::string BiomeManager::getHumidityNoiseLayer() const
    {
        return m_humidityNoiseLayer;
    }

    bool BiomeManager::applyPreset(const std::string& presetName)
    {
        if (presetName == "default")
        {
            // Default is already set up in constructor
            return true;
        }
        else if (presetName == "realistic")
        {
            // Clear existing biomes
            m_biomes.clear();
            m_biomeNameToId.clear();

            // Create realistic biome distribution based on temperature and humidity
            std::shared_ptr<Biome> desert = registerBiome("Desert");
            desert->setBiomeTemperatureRange(0.7f, 1.0f);
            desert->setBiomeHumidityRange(0.0f, 0.3f);
            desert->setVoxelProbability("4", 0.9f); // Sand
            desert->setFeatureProbability("cactus", 0.1f);

            std::shared_ptr<Biome> savanna = registerBiome("Savanna");
            savanna->setBiomeTemperatureRange(0.7f, 1.0f);
            savanna->setBiomeHumidityRange(0.3f, 0.5f);
            savanna->setVoxelProbability("1", 0.8f); // Dirt/Grass
            savanna->setFeatureProbability("tree", 0.05f);

            std::shared_ptr<Biome> jungle = registerBiome("Jungle");
            jungle->setBiomeTemperatureRange(0.7f, 1.0f);
            jungle->setBiomeHumidityRange(0.5f, 1.0f);
            jungle->setVoxelProbability("1", 0.9f); // Dirt/Grass
            jungle->setFeatureProbability("tree", 0.4f);

            std::shared_ptr<Biome> plain = registerBiome("Plains");
            plain->setBiomeTemperatureRange(0.4f, 0.7f);
            plain->setBiomeHumidityRange(0.2f, 0.6f);
            plain->setVoxelProbability("1", 1.0f); // Dirt/Grass
            plain->setFeatureProbability("tree", 0.05f);

            std::shared_ptr<Biome> forest = registerBiome("Forest");
            forest->setBiomeTemperatureRange(0.4f, 0.7f);
            forest->setBiomeHumidityRange(0.6f, 1.0f);
            forest->setVoxelProbability("1", 1.0f); // Dirt/Grass
            forest->setFeatureProbability("tree", 0.3f);

            std::shared_ptr<Biome> taiga = registerBiome("Taiga");
            taiga->setBiomeTemperatureRange(0.1f, 0.4f);
            taiga->setBiomeHumidityRange(0.1f, 1.0f);
            taiga->setVoxelProbability("1", 0.9f); // Dirt/Grass
            taiga->setVoxelProbability("6", 0.1f); // Snow
            taiga->setFeatureProbability("pine", 0.2f);

            std::shared_ptr<Biome> tundra = registerBiome("Tundra");
            tundra->setBiomeTemperatureRange(0.0f, 0.1f);
            tundra->setBiomeHumidityRange(0.0f, 1.0f);
            tundra->setVoxelProbability("6", 0.9f); // Snow

            // Mountains can appear in any climate
            std::shared_ptr<Biome> mountains = registerBiome("Mountains");
            mountains->setBiomeElevationRange(0.7f, 1.0f);
            mountains->setVoxelProbability("5", 1.0f); // Stone

            // Ocean biome
            std::shared_ptr<Biome> ocean = registerBiome("Ocean");
            ocean->setBiomeElevationRange(0.0f, 0.4f);
            ocean->setVoxelProbability("2", 1.0f); // Sand/Gravel

            return true;
        }
        else if (presetName == "fantasy")
        {
            // Clear existing biomes
            m_biomes.clear();
            m_biomeNameToId.clear();

            // Create fantasy biomes
            std::shared_ptr<Biome> magicForest = registerBiome("MagicForest");
            magicForest->setVoxelProbability("8", 0.9f); // Custom block type
            magicForest->setFeatureProbability("crystal", 0.2f);

            std::shared_ptr<Biome> lavaLands = registerBiome("LavaLands");
            lavaLands->setVoxelProbability("9", 0.8f); // Custom block type
            lavaLands->setFeatureProbability("lava_vent", 0.1f);

            std::shared_ptr<Biome> crystalForests = registerBiome("CrystalForests");
            crystalForests->setVoxelProbability("10", 0.7f); // Custom block type
            crystalForests->setFeatureProbability("crystal_formation", 0.3f);

            std::shared_ptr<Biome> cloudRealm = registerBiome("CloudRealm");
            cloudRealm->setVoxelProbability("11", 1.0f); // Custom block type
            cloudRealm->setFeatureProbability("cloud_pillar", 0.15f);

            std::shared_ptr<Biome> abyss = registerBiome("Abyss");
            abyss->setVoxelProbability("12", 0.95f); // Custom block type
            abyss->setFeatureProbability("void_tear", 0.05f);

            return true;
        }

        // Preset not found
        Core::warn("Biome preset '" + presetName + "' not found");
        return false;
    }

    Utility::Serialization::SerializationResult BiomeManager::serialize(Utility::Serialization::Serializer& serializer) const
    {
        // Begin serializing the biome manager
        auto result = serializer.beginObject(getTypeName());
        if (!result) return result;

        // Serialize basic properties
        result = serializer.writeField("blendDistance", m_blendDistance);
        if (!result) return result;

        result = serializer.writeField("temperatureNoiseLayer", m_temperatureNoiseLayer);
        if (!result) return result;

        result = serializer.writeField("humidityNoiseLayer", m_humidityNoiseLayer);
        if (!result) return result;

        // Serialize biomes
        result = serializer.beginArray("biomes", m_biomes.size());
        if (!result) return result;

        for (const auto& [biomeId, biome] : m_biomes)
        {
            result = biome->serialize(serializer);
            if (!result) return result;
        }

        result = serializer.endArray();
        if (!result) return result;

        return serializer.endObject();
    }

    Utility::Serialization::SerializationResult BiomeManager::deserialize(Utility::Serialization::Deserializer& deserializer)
    {
        // Clear existing data
        m_biomes.clear();
        m_biomeNameToId.clear();

        // Begin deserializing the biome manager
        auto result = deserializer.beginObject(getTypeName());
        if (!result) return result;

        // Serialize basic properties
        result = deserializer.readField("blendDistance", m_blendDistance);
        if (!result) return result;

        result = deserializer.readField("temperatureNoiseLayer", m_temperatureNoiseLayer);
        if (!result) return result;

        result = deserializer.readField("humidityNoiseLayer", m_humidityNoiseLayer);
        if (!result) return result;

        // Serialize biomes
        size_t biomesSize;
        result = deserializer.beginArray("biomes", biomesSize);
        if (!result) return result;

        for (size_t i = 0; i < biomesSize; i++)
        {
            std::shared_ptr<Biome> biome;
            result = biome->deserialize(deserializer);
            if (!result) return result;

            // Register the biome
            m_biomes[biome->getBiomeId()] = biome;
            m_biomeNameToId[biome->getBiomeName()] = biome->getBiomeId();
        }

        result = deserializer.endArray();
        if (!result) return result;

        return deserializer.endObject();
    }

    void BiomeManager::defineSchema(Utility::Serialization::Schema& schema)
    {
        schema.addField("blendDistance", Utility::Serialization::ValueType::Float, true);
        schema.addField("temperatureNoiseLayer", Utility::Serialization::ValueType::String, true);
        schema.addField("humidityNoiseLayer", Utility::Serialization::ValueType::String, true);
        schema.addArrayField("biomes", Utility::Serialization::ValueType::Object, true);
    }

    std::string BiomeManager::getTypeName()
    {
        return "BiomeManager";
    }

    // Private helper methods

    int BiomeManager::getNextBiomeId() const
    {
        int maxId = 0;

        for (const auto& [id, _] : m_biomes)
        {
            maxId = std::max(maxId, id);
        }

        return maxId + 1;
    }

    float BiomeManager::calculateBiomeInfluence(int biomeId, const glm::vec3& position,
                                                float temperature, float humidity, float elevation,
                                                const GenerationContext& context) const
    {
        auto it = m_biomes.find(biomeId);
        if (it == m_biomes.end() || !it->second->isBiomeEnabled())
        {
            return 0.0f;
        }

        const auto& biome = it->second;

        // Calculate weight from temperature, humidity, and elevation
        float tempWeight = 1.0f;
        float humidityWeight = 1.0f;
        float elevationWeight = 1.0f;

        // Check temperature range
        if (temperature < biome->getBiomeTemperatureRange().x || temperature > biome->getBiomeTemperatureRange().y)
        {
            // Outside range, calculate falloff
            float distToRange = 0.0f;
            if (temperature < biome->getBiomeTemperatureRange().x)
            {
                distToRange = biome->getBiomeTemperatureRange().x - temperature;
            }
            else
            {
                distToRange = temperature - biome->getBiomeTemperatureRange().y;
            }

            // Apply falloff
            tempWeight = std::max(0.0f, 1.0f - distToRange * 4.0f);
        }

        // Check humidity range
        if (humidity < biome->getBiomeTemperatureRange().x || humidity > biome->getBiomeTemperatureRange().y)
        {
            // Outside range, calculate falloff
            float distToRange = 0.0f;
            if (humidity < biome->getBiomeHumidityRange().x)
            {
                distToRange = biome->getBiomeHumidityRange().x - humidity;
            }
            else
            {
                distToRange = humidity - biome->getBiomeHumidityRange().y;
            }

            // Apply falloff
            humidityWeight = std::max(0.0f, 1.0f - distToRange * 4.0f);
        }

        // Check elevation range
        if (elevation < biome->getBiomeElevationRange().x || elevation > biome->getBiomeElevationRange().y)
        {
            // Outside range, calculate falloff
            float distToRange = 0.0f;
            if (elevation < biome->getBiomeElevationRange().x)
            {
                distToRange = biome->getBiomeElevationRange().x - elevation;
            }
            else
            {
                distToRange = elevation - biome->getBiomeElevationRange().y;
            }

            // Apply falloff
            elevationWeight = std::max(0.0f, 1.0f - distToRange * 4.0f);
        }

        // Combine weights
        float combinedWeight = tempWeight * humidityWeight * elevationWeight;

        // Apply biome weight
        combinedWeight *= biome->getBiomeWeight();

        // Apply noise factor if specified
        std::string noiseLayerId;
        float threshold;
        biome->getBiomeNoise(noiseLayerId, threshold);
        if (!noiseLayerId.empty() && context.noiseGenerator)
        {
            float noise = context.noiseGenerator->generate(position);

            // Normalize noise to [0,1]
            noise = noise * 0.5f + 0.5f;

            // Apply threshold
            if (noise < threshold)
            {
                // Below threshold, calculate falloff
                float falloff = noise / threshold;
                combinedWeight *= falloff;
            }
        }

        return combinedWeight;
    }

} // namespace PixelCraft::Voxel