// -------------------------------------------------------------------------
// NoiseLayer.cpp
// -------------------------------------------------------------------------
#pragma once

#include "Voxel/NoiseLayer.h"
#include "Utility/Serialization/SerializationUtility.h"

namespace PixelCraft::Voxel
{

    void NoiseLayer::setLayerName(const std::string& name)
    {
        m_name = name;
    }

    const std::string& NoiseLayer::getLayerName() const
    {
        return m_name;
    }

    void NoiseLayer::setLayerEnabled(bool enabled)
    {
        m_enabled = enabled;
    }

    bool NoiseLayer::isLayerEnabled() const
    {
        return m_enabled;
    }

    void NoiseLayer::setLayerNoiseType(NoiseType type)
    {
        m_noiseType = type;
    }

    NoiseLayer::NoiseType NoiseLayer::getLayerNoiseType() const
    {
        return m_noiseType;
    }

    void NoiseLayer::setLayerFractalType(FractalType type)
    {
        m_fractalType = type;
    }

    NoiseLayer::FractalType NoiseLayer::getLayerFractalType() const
    {
        return m_fractalType;
    }

    void NoiseLayer::setLayerFrequency(float frequency)
    {
        m_frequency = frequency;
    }

    float NoiseLayer::getLayerFrequency() const
    {
        return m_frequency;
    }

    void NoiseLayer::setLayerAmplitude(float amplitude)
    {
        m_amplitude = amplitude;
    }

    float NoiseLayer::getLayerAmplitude() const
    {
        return m_amplitude;
    }

    void NoiseLayer::setLayerOctaves(int octaves)
    {
        m_octaves = octaves;
    }

    int NoiseLayer::getLayerOctaves() const
    {
        return m_octaves;
    }

    void NoiseLayer::setLayerPersistence(float persistence)
    {
        m_persistence = persistence;
    }

    float NoiseLayer::getLayerPersistence() const
    {
        return m_persistence;
    }

    void NoiseLayer::setLayerLacunarity(float lacunarity)
    {
        m_lacunarity = lacunarity;
    }

    float NoiseLayer::getLayerLacunarity() const
    {
        return m_lacunarity;
    }

    void NoiseLayer::setLayerOffset(const glm::vec3& offset)
    {
        m_offset = offset;
    }

    const glm::vec3& NoiseLayer::getLayerOffset() const
    {
        return m_offset;
    }

    void NoiseLayer::setLayerScale(const glm::vec3& scale)
    {
        m_scale = scale;
    }

    const glm::vec3& NoiseLayer::getLayerScale() const
    {
        return m_scale;
    }

    void NoiseLayer::setLayerWeight(float weight)
    {
        m_weight = weight;
    }

    float NoiseLayer::getLayerWeight() const
    {
        return m_weight;
    }

    void NoiseLayer::setWarpType(WarpType type)
    {
        m_warpType = type;
    }

    NoiseLayer::WarpType NoiseLayer::getWarpType() const
    {
        return m_warpType;
    }

    void NoiseLayer::setWarpAmplitude(float amplitude)
    {
        m_warpAmplitude = amplitude;
    }

    float NoiseLayer::getWarpAmplitude() const
    {
        return m_warpAmplitude;
    }

    void NoiseLayer::setWarpFrequency(float frequency)
    {
        m_warpFrequency = frequency;
    }

    float NoiseLayer::getWarpFrequency() const
    {
        return m_warpFrequency;
    }

    void NoiseLayer::setWarpEnabled(bool enabled)
    {
        m_warpEnabled = enabled;
    }

    bool NoiseLayer::isWarpEnabled() const
    {
        return m_warpEnabled;
    }

    void NoiseLayer::setLayerCombineOperation(CombineOperation operation)
    {
        m_combineOp = operation;
    }

    NoiseLayer::CombineOperation NoiseLayer::getLayerCombineOperation() const
    {
        return m_combineOp;
    }

    Utility::Serialization::SerializationResult NoiseLayer::serialize(Utility::Serialization::Serializer& serializer) const
    {
        // Begin serializing the entity
        auto result = serializer.beginObject(getTypeName());
        if (!result) return result;

        result = serializer.writeField("id", m_id);
        if (!result) return result;

        result = serializer.writeField("name", m_name);
        if (!result) return result;

        result = serializer.writeField("enabled", m_enabled);
        if (!result) return result;

        result = serializer.writeField("noiseType", static_cast<int>(m_noiseType));
        if (!result) return result;

        result = serializer.writeField("fractalType", static_cast<int>(m_fractalType));
        if (!result) return result;

        result = serializer.writeField("interpolation", static_cast<int>(m_interpolation));
        if (!result) return result;

        result = serializer.writeField("combineOp", static_cast<int>(m_combineOp));
        if (!result) return result;

        result = serializer.writeField("frequency", m_frequency);
        if (!result) return result;

        result = serializer.writeField("amplitude", m_amplitude);
        if (!result) return result;

        result = serializer.writeField("octaves", m_octaves);
        if (!result) return result;

        result = serializer.writeField("persistence", m_persistence);
        if (!result) return result;

        result = serializer.writeField("lacunarity", m_lacunarity);
        if (!result) return result;

        result = serializer.writeField("offset", m_offset);
        if (!result) return result;

        result = serializer.writeField("scale", m_scale);
        if (!result) return result;

        result = serializer.writeField("weight", m_weight);
        if (!result) return result;

        result = serializer.writeField("warpType", static_cast<int>(m_warpType));
        if (!result) return result;

        result = serializer.writeField("warpAmplitude", m_warpAmplitude);
        if (!result) return result;

        result = serializer.writeField("warpFrequency", m_warpFrequency);
        if (!result) return result;

        result = serializer.writeField("warpEnabled", m_warpEnabled);
        if (!result) return result;

        // End entity object
        return serializer.endObject();
    }

    Utility::Serialization::SerializationResult NoiseLayer::deserialize(Utility::Serialization::Deserializer& deserializer)
    {
        // Begin deserializing the entity
        auto result = deserializer.beginObject(getTypeName());
        if (!result) return result;

        result = deserializer.readField("id", m_id);
        if (!result) return result;

        result = deserializer.readField("name", m_name);
        if (!result) return result;

        result = deserializer.readField("enabled", m_enabled);
        if (!result) return result;

        int32_t noiseType;
        result = deserializer.readField("noiseType", noiseType);
        if (!result) return result;
        m_noiseType = static_cast<NoiseType>(noiseType);

        int32_t fractalType;
        result = deserializer.readField("fractalType", fractalType);
        if (!result) return result;
        m_fractalType = static_cast<FractalType>(fractalType);

        int32_t interpolation;
        result = deserializer.readField("interpolation", interpolation);
        if (!result) return result;
        m_interpolation = static_cast<InterpolationType>(interpolation);

        int32_t combineOp;
        result = deserializer.readField("combineOp", combineOp);
        if (!result) return result;
        m_combineOp = static_cast<CombineOperation>(combineOp);

        result = deserializer.readField("frequency", m_frequency);
        if (!result) return result;

        result = deserializer.readField("amplitude", m_amplitude);
        if (!result) return result;

        result = deserializer.readField("octaves", m_octaves);
        if (!result) return result;

        result = deserializer.readField("persistence", m_persistence);
        if (!result) return result;

        result = deserializer.readField("lacunarity", m_lacunarity);
        if (!result) return result;

        result = deserializer.readField("offset", m_offset);
        if (!result) return result;

        result = deserializer.readField("scale", m_scale);
        if (!result) return result;

        result = deserializer.readField("weight", m_weight);
        if (!result) return result;

        int32_t warpType;
        result = deserializer.readField("warpType", warpType);
        if (!result) return result;
        m_warpType = static_cast<WarpType>(warpType);

        result = deserializer.readField("warpAmplitude", m_warpAmplitude);
        if (!result) return result;

        result = deserializer.readField("warpFrequency", m_warpFrequency);
        if (!result) return result;

        result = deserializer.readField("warpEnabled", m_warpEnabled);
        if (!result) return result;

        // End entity object
        return deserializer.endObject();
    }

    void NoiseLayer::defineSchema(Utility::Serialization::Schema& schema)
    {
        schema.addField("id", Utility::Serialization::ValueType::Int64, true);
        schema.addField("name", Utility::Serialization::ValueType::String, true);
        schema.addField("enabled", Utility::Serialization::ValueType::Bool, true);
        schema.addField("noiseType", Utility::Serialization::ValueType::UInt16, true);
        schema.addField("fractalType", Utility::Serialization::ValueType::UInt16, true);
        schema.addField("interpolation", Utility::Serialization::ValueType::UInt16, true);
        schema.addField("combineOp", Utility::Serialization::ValueType::UInt16, true);
        schema.addField("frequency", Utility::Serialization::ValueType::Float, true);
        schema.addField("amplitude", Utility::Serialization::ValueType::Float, true);
        schema.addField("octaves", Utility::Serialization::ValueType::Int64, true);
        schema.addField("persistence", Utility::Serialization::ValueType::Float, true);
        schema.addField("lacunarity", Utility::Serialization::ValueType::Float, true);
        schema.addField("offset", Utility::Serialization::ValueType::Vec3, true);
        schema.addField("scale", Utility::Serialization::ValueType::Vec3, true);
        schema.addField("weight", Utility::Serialization::ValueType::Float, true);
        schema.addField("warpType", Utility::Serialization::ValueType::UInt16, true);
        schema.addField("warpAmplitude", Utility::Serialization::ValueType::Float, true);
        schema.addField("warpFrequency", Utility::Serialization::ValueType::Float, true);
        schema.addField("warpEnabled", Utility::Serialization::ValueType::Bool, true);
    }

    std::string NoiseLayer::getTypeName()
    {
        return "NoiseLayer";
    }

} // namespace PixelCraft::Voxel