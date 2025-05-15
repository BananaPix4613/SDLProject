// -------------------------------------------------------------------------
// Utility/Serialization/BinarySerializer.cpp
// -------------------------------------------------------------------------
#include "Utility/Serialization/BinarySerializer.h"
#include "Core/Logger.h"
#include <cstring>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Utility::Serialization
{

    // Magic header for binary serialization format
    static const uint32_t BINARY_MAGIC = 0x42534552; // "BSER"

    BinarySerializer::BinarySerializer(std::ostream& stream)
        : m_stream(stream), m_compressionLevel(0), m_headerWritten(false), m_fieldNameWritten(false)
    {
        m_format = SerializationFormat::Binary;

        // Write format header
        if (m_stream)
        {
            m_stream.write(reinterpret_cast<const char*>(&BINARY_MAGIC), sizeof(BINARY_MAGIC));
            m_headerWritten = m_stream.good();
        }
    }

    BinarySerializer::~BinarySerializer()
    {
    }

    SerializationResult BinarySerializer::beginObject(const std::string& name, const Schema* schema)
    {
        if (!m_stream)
        {
            return SerializationResult("Stream is not valid");
        }

        // If we have a field name to write, write it now
        if (!name.empty() && !m_fieldNameWritten)
        {
            auto result = writeFieldName(name);
            if (!result) return result;
        }

        // Write schema name if available
        if (schema)
        {
            std::string schemaName = schema->getTypeName();
            uint32_t schemaNameLen = static_cast<uint32_t>(schemaName.length());
            m_stream.write(reinterpret_cast<const char*>(&schemaNameLen), sizeof(schemaNameLen));
            if (!m_stream)
            {
                return SerializationResult(getStreamErrorMessage());
            }

            if (schemaNameLen > 0)
            {
                m_stream.write(schemaName.c_str(), schemaNameLen);
                if (!m_stream)
                {
                    return SerializationResult(getStreamErrorMessage());
                }
            }

            // Write schema version
            const VersionInfo& version = schema->getVersion();
            m_stream.write(reinterpret_cast<const char*>(&version.major), sizeof(version.major));
            m_stream.write(reinterpret_cast<const char*>(&version.minor), sizeof(version.minor));
            m_stream.write(reinterpret_cast<const char*>(&version.patch), sizeof(version.patch));
            if (!m_stream)
            {
                return SerializationResult(getStreamErrorMessage());
            }
        }
        else
        {
            // No schema, write empty schema name
            uint32_t schemaNameLen = 0;
            m_stream.write(reinterpret_cast<const char*>(&schemaNameLen), sizeof(schemaNameLen));
            if (!m_stream)
            {
                return SerializationResult(getStreamErrorMessage());
            }
        }

        // Write placeholder for object size, to be filled in when endObject is called
        uint32_t objectSizePlaceholder = 0;
        std::streampos objectSizePos = m_stream.tellp();
        m_stream.write(reinterpret_cast<const char*>(&objectSizePlaceholder), sizeof(objectSizePlaceholder));
        if (!m_stream)
        {
            return SerializationResult(getStreamErrorMessage());
        }

        // Push object name to stack
        m_objectStack.push_back(name);

        m_fieldNameWritten = false;
        return SerializationResult();
    }

    SerializationResult BinarySerializer::endObject()
    {
        if (!m_stream)
        {
            return SerializationResult("Stream is not valid");
        }

        if (m_objectStack.empty())
        {
            return SerializationResult("No object to end");
        }

        // Pop object from stack
        m_objectStack.pop_back();

        m_fieldNameWritten = false;
        return SerializationResult();
    }

    SerializationResult BinarySerializer::beginArray(const std::string& name, size_t size, ValueType elementType)
    {
        if (!m_stream)
        {
            return SerializationResult("Stream is not valid");
        }

        // If we have a field name to write, write it now
        if (!name.empty() && !m_fieldNameWritten)
        {
            auto result = writeFieldName(name);
            if (!result) return result;
        }

        // Write array header
        uint32_t sizeValue = static_cast<uint32_t>(size);
        m_stream.write(reinterpret_cast<const char*>(&sizeValue), sizeof(sizeValue));
        if (!m_stream)
        {
            return SerializationResult(getStreamErrorMessage());
        }

        // Write element type
        uint16_t typeValue = static_cast<uint16_t>(elementType);
        m_stream.write(reinterpret_cast<const char*>(&typeValue), sizeof(typeValue));
        if (!m_stream)
        {
            return SerializationResult(getStreamErrorMessage());
        }

        // Push array state to stack
        ArrayState state;
        state.size = size;
        state.count = 0;
        state.elementType = elementType;
        m_arrayStack.push(state);

        m_fieldNameWritten = false;
        return SerializationResult();
    }

    SerializationResult BinarySerializer::endArray()
    {
        if (!m_stream)
        {
            return SerializationResult("Stream is not valid");
        }

        if (m_arrayStack.empty())
        {
            return SerializationResult("No array to end");
        }

        // Check if all elements were written
        ArrayState& state = m_arrayStack.top();
        if (state.count != state.size)
        {
            Log::warn("Array size mismatch: expected " + std::to_string(state.size) +
                      " elements, but wrote " + std::to_string(state.count));
        }

        // Pop array from stack
        m_arrayStack.pop();

        m_fieldNameWritten = false;
        return SerializationResult();
    }

    SerializationResult BinarySerializer::writeFieldName(const std::string& name)
    {
        if (!m_stream)
        {
            return SerializationResult("Stream is not valid");
        }

        if (m_fieldNameWritten)
        {
            return SerializationResult("Field name already written");
        }

        // Check string cache first
        auto it = m_stringCache.find(name);
        if (it != m_stringCache.end())
        {
            // Write cached string reference
            uint8_t flag = 1; // 1 indicates cached string
            m_stream.write(reinterpret_cast<const char*>(&flag), sizeof(flag));
            if (!m_stream)
            {
                return SerializationResult(getStreamErrorMessage());
            }

            m_stream.write(reinterpret_cast<const char*>(&it->second), sizeof(it->second));
            if (!m_stream)
            {
                return SerializationResult(getStreamErrorMessage());
            }
        }
        else
        {
            // Write new string
            uint8_t flag = 0; // 0 indicates new string
            m_stream.write(reinterpret_cast<const char*>(&flag), sizeof(flag));
            if (!m_stream)
            {
                return SerializationResult(getStreamErrorMessage());
            }

            uint32_t length = static_cast<uint32_t>(name.length());
            m_stream.write(reinterpret_cast<const char*>(&length), sizeof(length));
            if (!m_stream)
            {
                return SerializationResult(getStreamErrorMessage());
            }

            if (length > 0)
            {
                m_stream.write(name.c_str(), length);
                if (!m_stream)
                {
                    return SerializationResult(getStreamErrorMessage());
                }
            }

            // Add to string cache
            uint32_t cacheId = static_cast<uint32_t>(m_stringCache.size());
            m_stringCache[name] = cacheId;
        }

        m_currentField = name;
        m_fieldNameWritten = true;
        return SerializationResult();
    }

    SerializationResult BinarySerializer::writeNull()
    {
        return writeRawValue(ValueType::Null, nullptr, 0);
    }

    SerializationResult BinarySerializer::writeBool(bool value)
    {
        return writeRawValue(ValueType::Bool, &value, sizeof(value));
    }

    SerializationResult BinarySerializer::writeInt(int64_t value)
    {
        return writeRawValue(ValueType::Int64, &value, sizeof(value));
    }

    SerializationResult BinarySerializer::writeUInt(uint64_t value)
    {
        return writeRawValue(ValueType::UInt64, &value, sizeof(value));
    }

    SerializationResult BinarySerializer::writeFloat(float value)
    {
        return writeRawValue(ValueType::Float, &value, sizeof(value));
    }

    SerializationResult BinarySerializer::writeDouble(double value)
    {
        return writeRawValue(ValueType::Double, &value, sizeof(value));
    }

    SerializationResult BinarySerializer::writeString(const std::string& value)
    {
        auto result = writeValueHeader(ValueType::String, static_cast<uint32_t>(value.length()));
        if (!result) return result;

        if (!value.empty())
        {
            m_stream.write(value.c_str(), value.length());
            if (!m_stream)
            {
                return SerializationResult(getStreamErrorMessage());
            }
        }

        // Update array state if needed
        if (!m_arrayStack.empty())
        {
            m_arrayStack.top().count++;
        }

        m_fieldNameWritten = false;
        return SerializationResult();
    }

    SerializationResult BinarySerializer::writeBinary(const void* data, size_t size)
    {
        auto result = writeValueHeader(ValueType::Binary, static_cast<uint32_t>(size));
        if (!result) return result;

        if (size > 0 && data != nullptr)
        {
            m_stream.write(static_cast<const char*>(data), size);
            if (!m_stream)
            {
                return SerializationResult(getStreamErrorMessage());
            }
        }

        // Update array state if needed
        if (!m_arrayStack.empty())
        {
            m_arrayStack.top().count++;
        }

        m_fieldNameWritten = false;
        return SerializationResult();
    }

    SerializationResult BinarySerializer::writeEntityRef(uint64_t entityId)
    {
        std::string entityName;

        if (m_entityResolver)
        {
            entityName = m_entityResolver(entityId);
        }
        else
        {
            // Default to stringified ID if no resolver
            entityName = std::to_string(entityId);
        }

        auto result = writeValueHeader(ValueType::EntityRef, static_cast<uint32_t>(entityName.length()));
        if (!result) return result;

        if (!entityName.empty())
        {
            m_stream.write(entityName.c_str(), entityName.length());
            if (!m_stream)
            {
                return SerializationResult(getStreamErrorMessage());
            }
        }

        // Update array state if needed
        if (!m_arrayStack.empty())
        {
            m_arrayStack.top().count++;
        }

        m_fieldNameWritten = false;
        return SerializationResult();
    }

    SerializationResult BinarySerializer::writeResourceRef(const std::string& resourceName)
    {
        auto result = writeValueHeader(ValueType::ResourceRef, static_cast<uint32_t>(resourceName.length()));
        if (!result) return result;

        if (!resourceName.empty())
        {
            m_stream.write(resourceName.c_str(), resourceName.length());
            if (!m_stream)
            {
                return SerializationResult(getStreamErrorMessage());
            }
        }

        // Update array state if needed
        if (!m_arrayStack.empty())
        {
            m_arrayStack.top().count++;
        }

        m_fieldNameWritten = false;
        return SerializationResult();
    }

    void BinarySerializer::setCompressionLevel(int level)
    {
        m_compressionLevel = level;
        if (m_compressionLevel < 0) m_compressionLevel = 0;
        if (m_compressionLevel > 9) m_compressionLevel = 9;
    }

    SerializationResult BinarySerializer::writeVersion(const VersionInfo& version)
    {
        if (!m_stream)
        {
            return SerializationResult("Stream is not valid");
        }

        // Write version information
        m_stream.write(reinterpret_cast<const char*>(&version.major), sizeof(version.major));
        m_stream.write(reinterpret_cast<const char*>(&version.minor), sizeof(version.minor));
        m_stream.write(reinterpret_cast<const char*>(&version.patch), sizeof(version.patch));

        if (!m_stream)
        {
            return SerializationResult(getStreamErrorMessage());
        }

        return SerializationResult();
    }

    SerializationResult BinarySerializer::writeRawValue(ValueType type, const void* data, size_t size)
    {
        auto result = writeValueHeader(type, static_cast<uint32_t>(size));
        if (!result) return result;

        if (size > 0 && data != nullptr)
        {
            m_stream.write(static_cast<const char*>(data), size);
            if (!m_stream)
            {
                return SerializationResult(getStreamErrorMessage());
            }
        }

        // Update array state if needed
        if (!m_arrayStack.empty())
        {
            m_arrayStack.top().count++;
        }

        m_fieldNameWritten = false;
        return SerializationResult();
    }

    SerializationResult BinarySerializer::writeValueHeader(ValueType type, uint32_t size)
    {
        if (!m_stream)
        {
            return SerializationResult("Stream is not valid");
        }

        // Write value type
        uint16_t typeValue = static_cast<uint16_t>(type);
        m_stream.write(reinterpret_cast<const char*>(&typeValue), sizeof(typeValue));
        if (!m_stream)
        {
            return SerializationResult(getStreamErrorMessage());
        }

        // Write value size if needed
        if (type != ValueType::Null && type != ValueType::Bool)
        {
            m_stream.write(reinterpret_cast<const char*>(&size), sizeof(size));
            if (!m_stream)
            {
                return SerializationResult(getStreamErrorMessage());
            }
        }

        return SerializationResult();
    }

    std::string BinarySerializer::getStreamErrorMessage() const
    {
        std::string message = "Stream error: ";

        if (m_stream.bad())
        {
            message += "bad bit set (irrecoverable error)";
        }
        else if (m_stream.fail())
        {
            message += "fail bit set (recoverable error)";
        }
        else if (m_stream.eof())
        {
            message += "end of file reached";
        }
        else
        {
            message += "unknown error";
        }

        return message;
    }

} // namespace PixelCraft::Utility::Serialization