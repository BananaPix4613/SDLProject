// -------------------------------------------------------------------------
// Utility/Serialization/Serializer.cpp
// -------------------------------------------------------------------------
#include "Utility/Serialization/Serializer.h"
#include "Core/Logger.h"

namespace PixelCraft::Utility::Serialization
{

    Serializer::Serializer()
        : m_format(SerializationFormat::Binary)
    {
    }

    Serializer::~Serializer()
    {
    }

    // Template specializations for primitive types
    template<>
    SerializationResult Serializer::writeValue<bool>(const bool& value)
    {
        return writeBool(value);
    }

    template<>
    SerializationResult Serializer::writeValue<int8_t>(const int8_t& value)
    {
        return writeInt(value);
    }

    template<>
    SerializationResult Serializer::writeValue<uint8_t>(const uint8_t& value)
    {
        return writeUInt(value);
    }

    template<>
    SerializationResult Serializer::writeValue<int16_t>(const int16_t& value)
    {
        return writeInt(value);
    }

    template<>
    SerializationResult Serializer::writeValue<uint16_t>(const uint16_t& value)
    {
        return writeUInt(value);
    }

    template<>
    SerializationResult Serializer::writeValue<int32_t>(const int32_t& value)
    {
        return writeInt(value);
    }

    template<>
    SerializationResult Serializer::writeValue<uint32_t>(const uint32_t& value)
    {
        return writeUInt(value);
    }

    template<>
    SerializationResult Serializer::writeValue<int64_t>(const int64_t& value)
    {
        return writeInt(value);
    }

    template<>
    SerializationResult Serializer::writeValue<uint64_t>(const uint64_t& value)
    {
        return writeUInt(value);
    }

    template<>
    SerializationResult Serializer::writeValue<float>(const float& value)
    {
        return writeFloat(value);
    }

    template<>
    SerializationResult Serializer::writeValue<double>(const double& value)
    {
        return writeDouble(value);
    }

    template<>
    SerializationResult Serializer::writeValue<std::string>(const std::string& value)
    {
        return writeString(value);
    }

} // namespace PixelCraft::Utility::Serialization