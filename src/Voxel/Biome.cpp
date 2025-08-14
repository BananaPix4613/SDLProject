// -------------------------------------------------------------------------
// Biome.h
// -------------------------------------------------------------------------
#pragma once

#include "Voxel/Biome.h"
#include "Utility/Serialization/SerializationUtility.h"

namespace PixelCraft::Voxel
{

    const std::string& Biome::getBiomeName() const
    {
        return m_name;
    }

    int Biome::getBiomeId() const
    {
        return m_id;
    }

    void Biome::setBiomeNoise(const std::string& noiseLayerId, float threshold)
    {
        m_noiseLayerId = noiseLayerId;
        m_threshold = threshold;
    }

    void Biome::getBiomeNoise(std::string& noiseLayerId, float& threshold)
    {
        noiseLayerId = m_noiseLayerId;
        threshold = m_threshold;
    }

    void Biome::setBiomeTemperatureRange(float minTemp, float maxTemp)
    {
        m_temperatureRange = glm::vec2(minTemp, maxTemp);
    }

    glm::vec2 Biome::getBiomeTemperatureRange()
    {
        return m_temperatureRange;
    }

    void Biome::setBiomeHumidityRange(float minHumidity, float maxHumidity)
    {
        m_humidityRange = glm::vec2(minHumidity, maxHumidity);
    }

    glm::vec2 Biome::getBiomeHumidityRange()
    {
        return m_humidityRange;
    }

    void Biome::setBiomeElevationRange(float minElevation, float maxElevation)
    {
        m_elevationRange = glm::vec2(minElevation, maxElevation);
    }

    glm::vec2 Biome::getBiomeElevationRange()
    {
        return m_elevationRange;
    }

    void Biome::setVoxelProbability(const std::string& voxelType, float probability)
    {
        m_voxelProbabilities[voxelType] = std::max(0.0f, std::min(1.0f, probability));
    }

    float Biome::getVoxelProbability(const std::string& voxelType) const
    {
        auto& probs = m_voxelProbabilities;
        auto probIt = probs.find(voxelType);
        if (probIt == probs.end())
        {
            return 0.0f;
        }

        return probIt->second;
    }

    std::unordered_map<std::string, float> Biome::getVoxelProbabilities() const
    {
        return m_voxelProbabilities;
    }

    void Biome::setFeatureProbability(const std::string& featureType, float probability)
    {
        m_featureProbabilities[featureType] = std::max(0.0f, std::min(1.0f, probability));
    }

    float Biome::getFeatureProbability(const std::string& featureType) const
    {
        auto& probs = m_featureProbabilities;
        auto probIt = probs.find(featureType);
        if (probIt == probs.end())
        {
            return 0.0f;
        }

        return probIt->second;
    }

    void Biome::setBiomeWeight(float weight)
    {
        m_weight = std::max(0.0f, weight);
    }

    float Biome::getBiomeWeight() const
    {
        return m_weight;
    }

    void Biome::setBiomeEnabled(bool enabled)
    {
        m_enabled = enabled;
    }

    bool Biome::isBiomeEnabled() const
    {
        return m_enabled;
    }

    Utility::Serialization::SerializationResult Biome::serialize(Utility::Serialization::Serializer& serializer) const
    {
        // Begin serializing the entity
        auto result = serializer.beginObject(getTypeName());
        if (!result) return result;

        result = serializer.writeField("id", m_id);
        if (!result) return result;

        result = serializer.writeField("name", m_name);
        if (!result) return result;

        result = serializer.writeField("noiseLayerId", m_noiseLayerId);
        if (!result) return result;

        result = serializer.writeField("threshold", m_threshold);
        if (!result) return result;

        result = serializer.writeField("weight", m_weight);
        if (!result) return result;

        result = serializer.writeField("enabled", m_enabled);
        if (!result) return result;

        result = serializer.writeField("temperatureRange", m_temperatureRange);
        if (!result) return result;

        result = serializer.writeField("humidityRange", m_humidityRange);
        if (!result) return result;

        result = serializer.writeField("elevationRange", m_elevationRange);
        if (!result) return result;

        // Serialize voxel probabilities as an array
        result = serializer.beginArray("voxelProbabilities", m_voxelProbabilities.size());
        if (!result) return result;

        for (auto [voxelType, probability] : m_voxelProbabilities)
        {
            result = serializer.writeField("voxelType", voxelType);
            if (!result) return result;

            result = serializer.writeField("probability", probability);
            if (!result) return result;
        }

        result = serializer.endArray();
        if (!result) return result;

        // Serialize feature probabilities as an array
        result = serializer.beginArray("featureProbabilities", m_featureProbabilities.size());
        if (!result) return result;

        for (auto [featureType, probability] : m_featureProbabilities)
        {
            result = serializer.writeField("featureType", featureType);
            if (!result) return result;

            result = serializer.writeField("probability", probability);
            if (!result) return result;
        }

        result = serializer.endArray();
        if (!result) return result;

        // End entity object
        return serializer.endObject();
    }

    Utility::Serialization::SerializationResult Biome::deserialize(Utility::Serialization::Deserializer& deserializer)
    {
        // Clear data
        m_voxelProbabilities.clear();
        m_featureProbabilities.clear();

        // Begin serializing the entity
        auto result = deserializer.beginObject(getTypeName());
        if (!result) return result;

        result = deserializer.readField("id", m_id);
        if (!result) return result;

        result = deserializer.readField("name", m_name);
        if (!result) return result;

        result = deserializer.readField("noiseLayerId", m_noiseLayerId);
        if (!result) return result;

        result = deserializer.readField("threshold", m_threshold);
        if (!result) return result;

        result = deserializer.readField("weight", m_weight);
        if (!result) return result;

        result = deserializer.readField("enabled", m_enabled);
        if (!result) return result;

        result = deserializer.readField("temperatureRange", m_temperatureRange);
        if (!result) return result;

        result = deserializer.readField("humidityRange", m_humidityRange);
        if (!result) return result;

        result = deserializer.readField("elevationRange", m_elevationRange);
        if (!result) return result;

        size_t arraySize;

        // Serialize voxel probabilities as an array
        result = deserializer.beginArray("voxelProbabilities", arraySize);
        if (!result) return result;

        for (size_t i = 0; i < arraySize; i++)
        {
            std::string voxelType;
            result = deserializer.readField("voxelType", voxelType);
            if (!result) return result;

            float probability;
            result = deserializer.readField("probability", probability);
            if (!result) return result;

            m_voxelProbabilities[voxelType] = probability;
        }

        result = deserializer.endArray();
        if (!result) return result;

        // Serialize feature probabilities as an array
        result = deserializer.beginArray("featureProbabilities", arraySize);
        if (!result) return result;

        for (size_t i = 0; i < arraySize; i++)
        {
            std::string featureType;
            result = deserializer.readField("featureType", featureType);
            if (!result) return result;

            float probability;
            result = deserializer.readField("probability", probability);
            if (!result) return result;

            m_featureProbabilities[featureType] = probability;
        }

        result = deserializer.endArray();
        if (!result) return result;

        // End entity object
        return deserializer.endObject();
    }

    void Biome::defineSchema(Utility::Serialization::Schema& schema)
    {
        schema.addField("id", Utility::Serialization::ValueType::Int64, true);
        schema.addField("name", Utility::Serialization::ValueType::String, true);
        schema.addField("noiseLayerId", Utility::Serialization::ValueType::String, true);
        schema.addField("threshold", Utility::Serialization::ValueType::Float, true);
        schema.addField("weight", Utility::Serialization::ValueType::Float, true);
        schema.addField("enabled", Utility::Serialization::ValueType::Bool, true);
        schema.addField("temperatureRange", Utility::Serialization::ValueType::Vec2, true);
        schema.addField("humidityRange", Utility::Serialization::ValueType::Vec2, true);
        schema.addField("elevationRange", Utility::Serialization::ValueType::Vec2, true);
        schema.addArrayField("voxelProbabilities", Utility::Serialization::ValueType::Object, true);
        schema.addArrayField("featureProbabilities", Utility::Serialization::ValueType::Object, true);
    }

    std::string Biome::getTypeName()
    {
        return "Biome";
    }

} // namespace PixelCraft::Voxel