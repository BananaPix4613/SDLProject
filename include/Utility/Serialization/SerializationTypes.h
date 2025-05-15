// -------------------------------------------------------------------------
// Utility/Serialization/SerializationTypes.h
// -------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace PixelCraft::Utility::Serialization
{

    /**
     * @brief Enumeration of basic serializable types
     */
    enum class ValueType : uint16_t
    {
        Null = 0,
        Bool = 1,
        Int8 = 2,
        UInt8 = 3,
        Int16 = 4,
        UInt16 = 5,
        Int32 = 6,
        UInt32 = 7,
        Int64 = 8,
        UInt64 = 9,
        Float = 10,
        Double = 11,
        String = 12,
        Array = 13,
        Object = 14,
        Binary = 15,
        UUID = 16,
        EntityRef = 17,
        ResourceRef = 18
    };

    /**
     * @brief Format of serialized data
     */
    enum class SerializationFormat : uint8_t
    {
        Binary = 0,
        JSON = 1,
        XML = 2,
        FlatBuffers = 3
    };

    /**
     * @brief Result of serialization or deserialization operation
     */
    struct SerializationResult
    {
        bool success = false;
        std::string error;

        SerializationResult() : success(true)
        {
        }
        SerializationResult(const std::string& errorMsg) : success(false), error(errorMsg)
        {
        }

        operator bool() const
        {
            return success;
        }
    };

    /**
     * @brief Version information for serialization
     */
    struct VersionInfo
    {
        uint32_t major = 1;
        uint32_t minor = 0;
        uint32_t patch = 0;

        bool isCompatibleWith(const VersionInfo& other) const
        {
            return major == other.major && minor >= other.minor;
        }
    };

} // namespace PixelCraft::Utility::Serialization