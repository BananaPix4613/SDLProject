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
#include "ECS/Serializer.h"
#include "ECS/Deserializer.h"

#include <algorithm>
#include <random>
#include <cmath>

namespace Log = PixelCraft::Core;

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

    int BiomeManager::registerBiome(const std::string& name)
    {
        // Check if already registered
        auto it = m_biomeNameToId.find(name);
        if (it != m_biomeNameToId.end())
        {
            Log::warn("Biome '" + name + "' already registered");
            return it->second;
        }

        // Generate new ID
        int id = getNextBiomeId();

        // Create biome info
        BiomeInfo info;
        info.name = name;
        info.id = id;
        info.threshold = 0.5f;
        info.weight = 1.0f;
        info.enabled = true;
        info.temperatureRange = glm::vec2(0.0f, 1.0f);
        info.humidityRange = glm::vec2(0.0f, 1.0f);
        info.elevationRange = glm::vec2(0.0f, 1.0f);

        // Register it
        m_biomes[id] = info;
        m_biomeNameToId[name] = id;

        return id;
    }

    bool BiomeManager::removeBiome(int biomeId)
    {
        auto it = m_biomes.find(biomeId);
        if (it == m_biomes.end())
        {
            return false;
        }

        // Remove from name map first
        m_biomeNameToId.erase(it->second.name);

        // Remove from biomes map
        m_biomes.erase(it);

        return true;
    }

    const BiomeManager::BiomeInfo* BiomeManager::getBiomeInfo(int biomeId) const
    {
        auto it = m_biomes.find(biomeId);
        if (it == m_biomes.end())
        {
            return nullptr;
        }

        return &(it->second);
    }

    const BiomeManager::BiomeInfo* BiomeManager::getBiomeInfo(const std::string& name) const
    {
        auto it = m_biomeNameToId.find(name);
        if (it == m_biomeNameToId.end())
        {
            return nullptr;
        }

        return getBiomeInfo(it->second);
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

        for (const auto& [_, info] : m_biomes)
        {
            names.push_back(info.name);
        }

        return names;
    }

    bool BiomeManager::setBiomeNoise(int biomeId, const std::string& noiseLayerId, float threshold)
    {
        auto it = m_biomes.find(biomeId);
        if (it == m_biomes.end())
        {
            Log::error("Biome ID " + std::to_string(biomeId) + " not found");
            return false;
        }

        it->second.noiseLayerId = noiseLayerId;
        it->second.threshold = threshold;
        return true;
    }

    bool BiomeManager::setBiomeTemperatureRange(int biomeId, float minTemp, float maxTemp)
    {
        auto it = m_biomes.find(biomeId);
        if (it == m_biomes.end())
        {
            Log::error("Biome ID " + std::to_string(biomeId) + " not found");
            return false;
        }

        it->second.temperatureRange = glm::vec2(minTemp, maxTemp);
        return true;
    }

    bool BiomeManager::setBiomeHumidityRange(int biomeId, float minHumidity, float maxHumidity)
    {
        auto it = m_biomes.find(biomeId);
        if (it == m_biomes.end())
        {
            Log::error("Biome ID " + std::to_string(biomeId) + " not found");
            return false;
        }

        it->second.humidityRange = glm::vec2(minHumidity, maxHumidity);
        return true;
    }

    bool BiomeManager::setBiomeElevationRange(int biomeId, float minElevation, float maxElevation)
    {
        auto it = m_biomes.find(biomeId);
        if (it == m_biomes.end())
        {
            Log::error("Biome ID " + std::to_string(biomeId) + " not found");
            return false;
        }

        it->second.elevationRange = glm::vec2(minElevation, maxElevation);
        return true;
    }

    bool BiomeManager::setVoxelProbability(int biomeId, const std::string& voxelType, float probability)
    {
        auto it = m_biomes.find(biomeId);
        if (it == m_biomes.end())
        {
            Log::error("Biome ID " + std::to_string(biomeId) + " not found");
            return false;
        }

        it->second.voxelProbabilities[voxelType] = std::max(0.0f, std::min(1.0f, probability));
        return true;
    }

    float BiomeManager::getVoxelProbability(int biomeId, const std::string& voxelType) const
    {
        auto it = m_biomes.find(biomeId);
        if (it == m_biomes.end())
        {
            return 0.0f;
        }

        auto& probs = it->second.voxelProbabilities;
        auto probIt = probs.find(voxelType);
        if (probIt == probs.end())
        {
            return 0.0f;
        }

        return probIt->second;
    }

    bool BiomeManager::setFeatureProbability(int biomeId, const std::string& featureType, float probability)
    {
        auto it = m_biomes.find(biomeId);
        if (it == m_biomes.end())
        {
            Log::error("Biome ID " + std::to_string(biomeId) + " not found");
            return false;
        }

        it->second.featureProbabilities[featureType] = std::max(0.0f, std::min(1.0f, probability));
        return true;
    }

    float BiomeManager::getFeatureProbability(int biomeId, const std::string& featureType) const
    {
        auto it = m_biomes.find(biomeId);
        if (it == m_biomes.end())
        {
            return 0.0f;
        }

        auto& probs = it->second.featureProbabilities;
        auto probIt = probs.find(featureType);
        if (probIt == probs.end())
        {
            return 0.0f;
        }

        return probIt->second;
    }

    bool BiomeManager::setBiomeWeight(int biomeId, float weight)
    {
        auto it = m_biomes.find(biomeId);
        if (it == m_biomes.end())
        {
            Log::error("Biome ID " + std::to_string(biomeId) + " not found");
            return false;
        }

        it->second.weight = std::max(0.0f, weight);
        return true;
    }

    float BiomeManager::getBiomeWeight(int biomeId) const
    {
        auto it = m_biomes.find(biomeId);
        if (it == m_biomes.end())
        {
            return 0.0f;
        }

        return it->second.weight;
    }

    bool BiomeManager::setBiomeEnabled(int biomeId, bool enabled)
    {
        auto it = m_biomes.find(biomeId);
        if (it == m_biomes.end())
        {
            Log::error("Biome ID " + std::to_string(biomeId) + " not found");
            return false;
        }

        it->second.enabled = enabled;
        return true;
    }

    bool BiomeManager::isBiomeEnabled(int biomeId) const
    {
        auto it = m_biomes.find(biomeId);
        if (it == m_biomes.end())
        {
            return false;
        }

        return it->second.enabled;
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

        for (const auto& [biomeId, biomeInfo] : m_biomes)
        {
            if (!biomeInfo.enabled)
            {
                continue;
            }

            float influence = calculateBiomeInfluence(biomeId, position, temperature, humidity, elevation, context);

            if (influence > 0.0f)
            {
                influences[biomeId] = influence;
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
                            auto biomeInfo = getBiomeInfo(dominantBiome);
                            if (biomeInfo)
                            {
                                // Apply biome-specific surface block
                                // Example: use data value to store biome ID
                                voxel.data = static_cast<uint16_t>(dominantBiome);

                                // Apply based on voxel probabilities
                                // This is simplified and would be more complex in a real implementation
                                for (const auto& [voxelType, probability] : biomeInfo->voxelProbabilities)
                                {
                                    if (probability > 0.0f)
                                    {
                                        // Convert voxel type name to ID (simplified)
                                        uint16_t voxelTypeId = std::stoi(voxelType);

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
            int desertId = registerBiome("Desert");
            setBiomeTemperatureRange(desertId, 0.7f, 1.0f);
            setBiomeHumidityRange(desertId, 0.0f, 0.3f);
            setVoxelProbability(desertId, "4", 0.9f); // Sand
            setFeatureProbability(desertId, "cactus", 0.1f);

            int savannaId = registerBiome("Savanna");
            setBiomeTemperatureRange(savannaId, 0.7f, 1.0f);
            setBiomeHumidityRange(savannaId, 0.3f, 0.5f);
            setVoxelProbability(savannaId, "1", 0.8f); // Dirt/Grass
            setFeatureProbability(savannaId, "tree", 0.05f);

            int jungleId = registerBiome("Jungle");
            setBiomeTemperatureRange(jungleId, 0.7f, 1.0f);
            setBiomeHumidityRange(jungleId, 0.5f, 1.0f);
            setVoxelProbability(jungleId, "1", 0.9f); // Dirt/Grass
            setFeatureProbability(jungleId, "tree", 0.4f);

            int plainId = registerBiome("Plains");
            setBiomeTemperatureRange(jungleId, 0.4f, 0.7f);
            setBiomeHumidityRange(plainId, 0.2f, 0.6f);
            setVoxelProbability(plainId, "1", 1.0f); // Dirt/Grass
            setFeatureProbability(plainId, "tree", 0.05f);

            int forestId = registerBiome("Forest");
            setBiomeTemperatureRange(forestId, 0.4f, 0.7f);
            setBiomeHumidityRange(forestId, 0.6f, 1.0f);
            setVoxelProbability(forestId, "1", 1.0f); // Dirt/Grass
            setFeatureProbability(forestId, "tree", 0.3f);

            int taigaId = registerBiome("Taiga");
            setBiomeTemperatureRange(taigaId, 0.1f, 0.4f);
            setBiomeHumidityRange(taigaId, 0.1f, 1.0f);
            setVoxelProbability(taigaId, "1", 0.9f); // Dirt/Grass
            setVoxelProbability(taigaId, "6", 0.1f); // Snow
            setFeatureProbability(taigaId, "pine", 0.2f);

            int tundraId = registerBiome("Tundra");
            setBiomeTemperatureRange(tundraId, 0.0f, 0.1f);
            setBiomeHumidityRange(tundraId, 0.0f, 1.0f);
            setVoxelProbability(tundraId, "6", 0.9f); // Snow

            // Mountains can appear in any climate
            int mountainsId = registerBiome("Mountains");
            setBiomeElevationRange(mountainsId, 0.7f, 1.0f);
            setVoxelProbability(mountainsId, "5", 1.0f); // Stone

            // Ocean biome
            int oceanId = registerBiome("Ocean");
            setBiomeElevationRange(oceanId, 0.0f, 0.4f);
            setVoxelProbability(oceanId, "2", 1.0f); // Sand/Gravel

            return true;
        }
        else if (presetName == "fantasy")
        {
            // Clear existing biomes
            m_biomes.clear();
            m_biomeNameToId.clear();

            // Create fantasy biomes
            int magicForestId = registerBiome("MagicForest");
            setVoxelProbability(magicForestId, "8", 0.9f); // Custom block type
            setFeatureProbability(magicForestId, "crystal", 0.2f);

            int lavaLandsId = registerBiome("LavaLands");
            setVoxelProbability(lavaLandsId, "9", 0.8f); // Custom block type
            setFeatureProbability(lavaLandsId, "lava_vent", 0.1f);

            int crystalForestsId = registerBiome("CrystalForests");
            setVoxelProbability(crystalForestsId, "10", 0.7f); // Custom block type
            setFeatureProbability(crystalForestsId, "crystal_formation", 0.3f);

            int cloudRealmId = registerBiome("CloudRealm");
            setVoxelProbability(cloudRealmId, "11", 1.0f); // Custom block type
            setFeatureProbability(cloudRealmId, "cloud_pillar", 0.15f);

            int abyssId = registerBiome("Abyss");
            setVoxelProbability(abyssId, "12", 0.95f); // Custom block type
            setFeatureProbability(abyssId, "void_tear", 0.05f);

            return true;
        }

        // Preset not found
        Log::warn("Biome preset '" + presetName + "' not found");
        return false;
    }

    void BiomeManager::serialize(ECS::Serializer& serializer) const
    {
        // Serialize basic properties
        serializer.write(m_blendDistance);
        serializer.writeString(m_temperatureNoiseLayer);
        serializer.writeString(m_humidityNoiseLayer);

        // Serialize biomes
        serializer.beginArray("biomes", m_biomes.size());
        for (const auto& [biomeId, biomeInfo] : m_biomes)
        {
            serializer.write(biomeInfo.id);
            serializer.writeString(biomeInfo.name);
            serializer.writeString(biomeInfo.noiseLayerId);
            serializer.write(biomeInfo.threshold);
            serializer.write(biomeInfo.weight);
            serializer.write(biomeInfo.enabled);
            serializer.writeVec2(biomeInfo.temperatureRange);
            serializer.writeVec2(biomeInfo.humidityRange);
            serializer.writeVec2(biomeInfo.elevationRange);

            // Serialize voxel probabilities
            serializer.beginArray("voxelProbabilities", biomeInfo.voxelProbabilities.size());
            for (const auto& [voxelType, probability] : biomeInfo.voxelProbabilities)
            {
                serializer.writeString(voxelType);
                serializer.write(probability);
            }
            serializer.endArray();

            // Serialize feature probabilities
            serializer.beginArray("featureProbabilities", biomeInfo.featureProbabilities.size());
            for (const auto& [featureType, probability] : biomeInfo.featureProbabilities)
            {
                serializer.writeString(featureType);
                serializer.write(probability);
            }
            serializer.endArray();
        }
        serializer.endArray();
    }

    void BiomeManager::deserialize(ECS::Deserializer& deserializer)
    {
        // Clear existing data
        m_biomes.clear();
        m_biomeNameToId.clear();

        // Deserialize basic properties
        deserializer.read(m_blendDistance);
        deserializer.readString(m_temperatureNoiseLayer);
        deserializer.readString(m_humidityNoiseLayer);

        // Serialize biomes
        size_t biomesSize;
        deserializer.beginArray("biomes", biomesSize);
        for (int i = 0; i < biomesSize; i++)
        {
            BiomeInfo biomeInfo;

            deserializer.read(biomeInfo.id);
            deserializer.readString(biomeInfo.name);
            deserializer.readString(biomeInfo.noiseLayerId);
            deserializer.read(biomeInfo.threshold);
            deserializer.read(biomeInfo.weight);
            deserializer.read(biomeInfo.enabled);
            deserializer.readVec2(biomeInfo.temperatureRange);
            deserializer.readVec2(biomeInfo.humidityRange);
            deserializer.readVec2(biomeInfo.elevationRange);

            // Serialize voxel probabilities
            size_t arraySize;
            deserializer.beginArray("voxelProbabilities", arraySize);
            for (int i = 0; i < arraySize; i++)
            {
                std::string voxelType;
                deserializer.readString(voxelType);

                float voxelProbability;
                deserializer.read(voxelProbability);

                biomeInfo.voxelProbabilities[voxelType] = voxelProbability;
            }
            deserializer.endArray();

            // Serialize feature probabilities
            deserializer.beginArray("featureProbabilities", arraySize);
            for (int i = 0; i < arraySize; i++)
            {
                std::string featureType;
                deserializer.readString(featureType);

                float featureProbability;
                deserializer.read(featureProbability);

                biomeInfo.featureProbabilities[featureType] = featureProbability;
            }
            deserializer.endArray();

            // Register the biome
            m_biomes[biomeInfo.id] = biomeInfo;
            m_biomeNameToId[biomeInfo.name] = biomeInfo.id;
        }
        deserializer.endArray();
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
        if (it == m_biomes.end() || !it->second.enabled)
        {
            return 0.0f;
        }

        const auto& biomeInfo = it->second;

        // Calculate weight from temperature, humidity, and elevation
        float tempWeight = 1.0f;
        float humidityWeight = 1.0f;
        float elevationWeight = 1.0f;

        // Check temperature range
        if (temperature < biomeInfo.temperatureRange.x || temperature > biomeInfo.temperatureRange.y)
        {
            // Outside range, calculate falloff
            float distToRange = 0.0f;
            if (temperature < biomeInfo.temperatureRange.x)
            {
                distToRange = biomeInfo.temperatureRange.x - temperature;
            }
            else
            {
                distToRange = temperature - biomeInfo.temperatureRange.y;
            }

            // Apply falloff
            tempWeight = std::max(0.0f, 1.0f - distToRange * 4.0f);
        }

        // Check humidity range
        if (humidity < biomeInfo.humidityRange.x || humidity > biomeInfo.humidityRange.y)
        {
            // Outside range, calculate falloff
            float distToRange = 0.0f;
            if (humidity < biomeInfo.humidityRange.x)
            {
                distToRange = biomeInfo.humidityRange.x - humidity;
            }
            else
            {
                distToRange = humidity - biomeInfo.humidityRange.y;
            }

            // Apply falloff
            humidityWeight = std::max(0.0f, 1.0f - distToRange * 4.0f);
        }

        // Check elevation range
        if (elevation < biomeInfo.elevationRange.x || elevation > biomeInfo.elevationRange.y)
        {
            // Outside range, calculate falloff
            float distToRange = 0.0f;
            if (elevation < biomeInfo.elevationRange.x)
            {
                distToRange = biomeInfo.elevationRange.x - elevation;
            }
            else
            {
                distToRange = elevation - biomeInfo.elevationRange.y;
            }

            // Apply falloff
            elevationWeight = std::max(0.0f, 1.0f - distToRange * 4.0f);
        }

        // Combine weights
        float combinedWeight = tempWeight * humidityWeight * elevationWeight;

        // Apply biome weight
        combinedWeight *= biomeInfo.weight;

        // Apply noise factor if specified
        if (!biomeInfo.noiseLayerId.empty() && context.noiseGenerator)
        {
            float noise = context.noiseGenerator->generate(position);

            // Normalize noise to [0,1]
            noise = noise * 0.5f + 0.5f;

            // Apply threshold
            if (noise < biomeInfo.threshold)
            {
                // Below threshold, calculate falloff
                float falloff = noise / biomeInfo.threshold;
                combinedWeight *= falloff;
            }
        }

        return combinedWeight;
    }

} // namespace PixelCraft::Voxel