// -------------------------------------------------------------------------
// Utility/Serialization/Deserializer.cpp
// -------------------------------------------------------------------------
#include "Utility/Serialization/Deserializer.h"
#include "Core/Logger.h"

namespace PixelCraft::Utility::Serialization
{

    Deserializer::Deserializer()
        : m_format(SerializationFormat::Binary)
    {
    }

    Deserializer::~Deserializer()
    {
    }

    // Template specializations for primitive types
    template<>
    SerializationResult Deserializer::readValue<bool>(bool& value)
    {
        return readBool(value);
    }

    template<>
    SerializationResult Deserializer::readValue<int8_t>(int8_t& value)
    {
        int64_t temp;
        auto result = readInt(temp);
        if (result) value = static_cast<int8_t>(temp);
        return result;
    }

    template<>
    SerializationResult Deserializer::readValue<uint8_t>(uint8_t& value)
    {
        uint64_t temp;
        auto result = readUInt(temp);
        if (result) value = static_cast<uint8_t>(temp);
        return result;
    }

    template<>
    SerializationResult Deserializer::readValue<int16_t>(int16_t& value)
    {
        int64_t temp;
        auto result = readInt(temp);
        if (result) value = static_cast<int16_t>(temp);
        return result;
    }

    template<>
    SerializationResult Deserializer::readValue<uint16_t>(uint16_t& value)
    {
        uint64_t temp;
        auto result = readUInt(temp);
        if (result) value = static_cast<uint16_t>(temp);
        return result;
    }

    template<>
    SerializationResult Deserializer::readValue<int32_t>(int32_t& value)
    {
        int64_t temp;
        auto result = readInt(temp);
        if (result) value = static_cast<int32_t>(temp);
        return result;
    }

    template<>
    SerializationResult Deserializer::readValue<uint32_t>(uint32_t& value)
    {
        uint64_t temp;
        auto result = readUInt(temp);
        if (result) value = static_cast<uint32_t>(temp);
        return result;
    }

    template<>
    SerializationResult Deserializer::readValue<int64_t>(int64_t& value)
    {
        return readInt(value);
    }

    template<>
    SerializationResult Deserializer::readValue<uint64_t>(uint64_t& value)
    {
        return readUInt(value);
    }

    template<>
    SerializationResult Deserializer::readValue<float>(float& value)
    {
        return readFloat(value);
    }

    template<>
    SerializationResult Deserializer::readValue<double>(double& value)
    {
        return readDouble(value);
    }

    template<>
    SerializationResult Deserializer::readValue<glm::vec2>(glm::vec2& value)
    {
        return readVec2(value);
    }

    template<>
    SerializationResult Deserializer::readValue<glm::ivec2>(glm::ivec2& value)
    {
        return readIVec2(value);
    }

    template<>
    SerializationResult Deserializer::readValue<glm::vec3>(glm::vec3& value)
    {
        return readVec3(value);
    }

    template<>
    SerializationResult Deserializer::readValue<glm::ivec3>(glm::ivec3& value)
    {
        return readIVec3(value);
    }

    template<>
    SerializationResult Deserializer::readValue<glm::vec4>(glm::vec4& value)
    {
        return readVec4(value);
    }

    template<>
    SerializationResult Deserializer::readValue<glm::ivec4>(glm::ivec4& value)
    {
        return readIVec4(value);
    }

    template<>
    SerializationResult Deserializer::readValue<glm::mat3>(glm::mat3& value)
    {
        return readMat3(value);
    }

    template<>
    SerializationResult Deserializer::readValue<glm::mat4>(glm::mat4& value)
    {
        return readMat4(value);
    }

    template<>
    SerializationResult Deserializer::readValue<glm::quat>(glm::quat& value)
    {
        return readQuat(value);
    }

    template<>
    SerializationResult Deserializer::readValue<std::string>(std::string& value)
    {
        return readString(value);
    }

} // namespace PixelCraft::Utility::Serialization