// -------------------------------------------------------------------------
// Serializer.cpp
// -------------------------------------------------------------------------
#include "Core/Serialize/Serializer.h"
#include "Core/Serialize/JsonSerializer.h"
#include "Core/Serialize/BinarySerializer.h"
#include "Core/Logger.h"

namespace PixelCraft::Core::Serialize
{

    Serializer::Serializer(SerializationFormat format)
        : m_format(format)
    {
    }

    SerializationFormat Serializer::getFormat() const
    {
        return m_format;
    }

    std::unique_ptr<Serializer> Serializer::createSerializer(SerializationFormat format)
    {
        switch (format)
        {
            case SerializationFormat::JSON:
                return std::make_unique<JsonSerializer>();
            case SerializationFormat::Binary:
                return std::make_unique<BinarySerializer>();
            case SerializationFormat::XML:
                warn("XML serialization is not implemented yet.");
                return nullptr;
            case SerializationFormat::YAML:
                warn("YAML serialization is not implemented yet.");
                return nullptr;
            default:
                error("Unknown serialization format.");
                return nullptr;
        }
    }

} // namespace PixelCraft::Core::Serialize