// -------------------------------------------------------------------------
// ParameterValue.cpp
// -------------------------------------------------------------------------
#include "Utility/Serialization/ParameterValue.h"
#include "Utility/Serialization/SerializationUtility.h"

namespace PixelCraft::Utility::Serialization
{

    ValueType ParameterValue::getType() const
    {
        return std::visit([](const auto& value) -> ValueType {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, bool>) return ValueType::Bool;
            else if constexpr (std::is_same_v<T, int8_t>) return ValueType::Int8;
            else if constexpr (std::is_same_v<T, uint8_t>) return ValueType::UInt8;
            else if constexpr (std::is_same_v<T, int16_t>) return ValueType::Int16;
            else if constexpr (std::is_same_v<T, uint16_t>) return ValueType::UInt16;
            else if constexpr (std::is_same_v<T, int32_t>) return ValueType::Int32;
            else if constexpr (std::is_same_v<T, uint32_t>) return ValueType::UInt32;
            else if constexpr (std::is_same_v<T, int64_t>) return ValueType::Int64;
            else if constexpr (std::is_same_v<T, uint64_t>) return ValueType::UInt64;
            else if constexpr (std::is_same_v<T, float>) return ValueType::Float;
            else if constexpr (std::is_same_v<T, double>) return ValueType::Double;
            else if constexpr (std::is_same_v<T, std::string>) return ValueType::String;
            else if constexpr (std::is_same_v<T, glm::vec2>) return ValueType::Vec2;
            else if constexpr (std::is_same_v<T, glm::vec3>) return ValueType::Vec3;
            else if constexpr (std::is_same_v<T, glm::vec4>) return ValueType::Vec4;
            else if constexpr (std::is_same_v<T, glm::ivec2>) return ValueType::IVec2;
            else if constexpr (std::is_same_v<T, glm::ivec3>) return ValueType::IVec3;
            else if constexpr (std::is_same_v<T, glm::ivec4>) return ValueType::IVec4;
            else if constexpr (std::is_same_v<T, glm::mat3>) return ValueType::Mat3;
            else if constexpr (std::is_same_v<T, glm::mat4>) return ValueType::Mat4;
            else if constexpr (std::is_same_v<T, glm::quat>) return ValueType::Quat;
            else return ValueType::Null;
                          }, m_value);
    }

    SerializationResult ParameterValue::serialize(Serializer& serializer) const
    {
        // Write the type first
        auto type = getType();
        auto result = serializer.writeUInt(static_cast<uint64_t>(type));
        if (!result.success) return result;

        // Write the value based on type
        return std::visit([&serializer](const auto& value) -> SerializationResult {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, bool>)
            {
                return serializer.writeBool(value);
            }
            else if constexpr (std::is_same_v<T, int8_t>)
            {
                return serializer.writeInt(value);
            }
            else if constexpr (std::is_same_v<T, uint8_t>)
            {
                return serializer.writeUInt(value);
            }
            else if constexpr (std::is_same_v<T, int16_t>)
            {
                return serializer.writeInt(value);
            }
            else if constexpr (std::is_same_v<T, uint16_t>)
            {
                return serializer.writeUInt(value);
            }
            else if constexpr (std::is_same_v<T, int32_t>)
            {
                return serializer.writeInt(value);
            }
            else if constexpr (std::is_same_v<T, uint32_t>)
            {
                return serializer.writeUInt(value);
            }
            else if constexpr (std::is_same_v<T, int64_t>)
            {
                return serializer.writeInt(value);
            }
            else if constexpr (std::is_same_v<T, uint64_t>)
            {
                return serializer.writeUInt(value);
            }
            else if constexpr (std::is_same_v<T, float>)
            {
                return serializer.writeFloat(value);
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                return serializer.writeDouble(value);
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                return serializer.writeString(value);
            }
            else if constexpr (std::is_same_v<T, glm::vec2>)
            {
                return serializer.writeVec2(value);
            }
            else if constexpr (std::is_same_v<T, glm::ivec2>)
            {
                return serializer.writeIVec2(value);
            }
            else if constexpr (std::is_same_v<T, glm::vec3>)
            {
                return serializer.writeVec3(value);
            }
            else if constexpr (std::is_same_v<T, glm::ivec3>)
            {
                return serializer.writeIVec3(value);
            }
            else if constexpr (std::is_same_v<T, glm::vec4>)
            {
                return serializer.writeVec4(value);
            }
            else if constexpr (std::is_same_v<T, glm::ivec4>)
            {
                return serializer.writeIVec4(value);
            }
            else if constexpr (std::is_same_v<T, glm::mat3>)
            {
                return serializer.writeMat3(value);
            }
            else if constexpr (std::is_same_v<T, glm::mat4>)
            {
                return serializer.writeMat4(value);
            }
            else if constexpr (std::is_same_v<T, glm::quat>)
            {
                return serializer.writeQuat(value);
            }
            else
            {
                return SerializationResult{false, "Unsupported parameter type"};
            }
                          }, m_value);
    }

    SerializationResult ParameterValue::deserialize(Deserializer& deserializer)
    {
        // Read the type first
        uint64_t typeValue;
        auto result = deserializer.readUInt(typeValue);
        if (!result.success) return result;

        auto type = static_cast<ValueType>(typeValue);

        // Read the value based on type
        switch (type)
        {
            case ValueType::Bool:
            {
                bool val;
                result = deserializer.readBool(val);
                if (result.success) m_value = val;
                break;
            }
            case ValueType::Int8:
            {
                int64_t val;
                result = deserializer.readInt(val);
                if (result.success) m_value = static_cast<int8_t>(val);
                break;
            }
            case ValueType::UInt8:
            {
                uint64_t val;
                result = deserializer.readUInt(val);
                if (result.success) m_value = static_cast<uint8_t>(val);
                break;
            }
            case ValueType::Int16:
            {
                int64_t val;
                result = deserializer.readInt(val);
                if (result.success) m_value = static_cast<int16_t>(val);
                break;
            }
            case ValueType::UInt16:
            {
                uint64_t val;
                result = deserializer.readUInt(val);
                if (result.success) m_value = static_cast<uint16_t>(val);
                break;
            }
            case ValueType::Int32:
            {
                int64_t val;
                result = deserializer.readInt(val);
                if (result.success) m_value = static_cast<int32_t>(val);
                break;
            }
            case ValueType::UInt32:
            {
                uint64_t val;
                result = deserializer.readUInt(val);
                if (result.success) m_value = static_cast<uint32_t>(val);
                break;
            }
            case ValueType::Int64:
            {
                int64_t val;
                result = deserializer.readInt(val);
                if (result.success) m_value = static_cast<int64_t>(val);
                break;
            }
            case ValueType::UInt64:
            {
                uint64_t val;
                result = deserializer.readUInt(val);
                if (result.success) m_value = static_cast<uint64_t>(val);
                break;
            }
            case ValueType::Float:
            {
                float val;
                result = deserializer.readFloat(val);
                if (result.success) m_value = val;
                break;
            }
            case ValueType::Double:
            {
                double val;
                result = deserializer.readDouble(val);
                if (result.success) m_value = val;
                break;
            }
            case ValueType::String:
            {
                std::string val;
                result = deserializer.readString(val);
                if (result.success) m_value = val;
                break;
            }
            case ValueType::Vec2:
            {
                glm::vec2 val;
                result = deserializer.readVec2(val);
                if (result.success) m_value = val;
                break;
            }
            case ValueType::IVec2:
            {
                glm::ivec2 val;
                result = deserializer.readIVec2(val);
                if (result.success) m_value = val;
                break;
            }
            case ValueType::Vec3:
            {
                glm::vec3 val;
                result = deserializer.readVec3(val);
                if (result.success) m_value = val;
                break;
            }
            case ValueType::IVec3:
            {
                glm::ivec3 val;
                result = deserializer.readIVec3(val);
                if (result.success) m_value = val;
                break;
            }
            case ValueType::Vec4:
            {
                glm::vec4 val;
                result = deserializer.readVec4(val);
                if (result.success) m_value = val;
                break;
            }
            case ValueType::IVec4:
            {
                glm::ivec4 val;
                result = deserializer.readIVec4(val);
                if (result.success) m_value = val;
                break;
            }
            case ValueType::Mat3:
            {
                glm::mat3 val;
                result = deserializer.readMat3(val);
                if (result.success) m_value = val;
                break;
            }
            case ValueType::Mat4:
            {
                glm::mat4 val;
                result = deserializer.readMat4(val);
                if (result.success) m_value = val;
                break;
            }
            case ValueType::Quat:
            {
                glm::quat val;
                result = deserializer.readQuat(val);
                if (result.success) m_value = val;
                break;
            }
            default:
                return SerializationResult{"Unknown parameter type"};
        }

        return result;
    }

} // namespace PixelCraft::Utility::Serialization