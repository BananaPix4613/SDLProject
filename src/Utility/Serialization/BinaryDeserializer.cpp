// -------------------------------------------------------------------------
// Utility/Serialization/BinaryDeserializer.cpp
// -------------------------------------------------------------------------
#include "Utility/Serialization/BinaryDeserializer.h"
#include "Core/Logger.h"
#include <cstring>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Utility::Serialization
{

    // Magic header for binary serialization format
    static const uint32_t BINARY_MAGIC = 0x42534552; // "BSER"

    BinaryDeserializer::BinaryDeserializer(std::istream& stream)
        : m_stream(stream), m_headerRead(false)
    {
        m_format = SerializationFormat::Binary;

        // Read and validate format header
        if (m_stream)
        {
            uint32_t magic;
            m_stream.read(reinterpret_cast<char*>(&magic), sizeof(magic));

            if (m_stream && magic == BINARY_MAGIC)
            {
                m_headerRead = true;

                // Read version info
                m_stream.read(reinterpret_cast<char*>(&m_version.major), sizeof(m_version.major));
                m_stream.read(reinterpret_cast<char*>(&m_version.minor), sizeof(m_version.minor));
                m_stream.read(reinterpret_cast<char*>(&m_version.patch), sizeof(m_version.patch));
            }
            else
            {
                Log::error("Invalid binary serialization format header");
            }
        }
    }

    BinaryDeserializer::~BinaryDeserializer()
    {
    }

    SerializationResult BinaryDeserializer::beginObject(const std::string& name, const Schema* schema)
    {
        if (!m_stream)
        {
            return SerializationResult("Stream is not valid");
        }

        if (!m_headerRead)
        {
            return SerializationResult("Invalid serialization format");
        }

        // If name is not empty, find the field
        if (!name.empty() && !findField(name))
        {
            return SerializationResult("Field not found: " + name);
        }

        // Read schema name
        uint32_t schemaNameLen;
        m_stream.read(reinterpret_cast<char*>(&schemaNameLen), sizeof(schemaNameLen));
        if (!m_stream)
        {
            return SerializationResult(getStreamErrorMessage());
        }

        std::string schemaName;
        if (schemaNameLen > 0)
        {
            schemaName.resize(schemaNameLen);
            m_stream.read(&schemaName[0], schemaNameLen);
            if (!m_stream)
            {
                return SerializationResult(getStreamErrorMessage());
            }

            // Read schema version
            VersionInfo version;
            m_stream.read(reinterpret_cast<char*>(&version.major), sizeof(version.major));
            m_stream.read(reinterpret_cast<char*>(&version.minor), sizeof(version.minor));
            m_stream.read(reinterpret_cast<char*>(&version.patch), sizeof(version.patch));
            if (!m_stream)
            {
                return SerializationResult(getStreamErrorMessage());
            }

            // Validate schema if provided
            if (schema && schema->getTypeName() != schemaName)
            {
                return SerializationResult("Schema mismatch: expected " + schema->getTypeName() +
                                           ", got " + schemaName);
            }
        }

        // Read object size
        uint32_t objectSize;
        m_stream.read(reinterpret_cast<char*>(&objectSize), sizeof(objectSize));
        if (!m_stream)
        {
            return SerializationResult(getStreamErrorMessage());
        }

        // Calculate end position
        std::streampos currentPos = m_stream.tellg();
        std::streampos endPos = currentPos + static_cast<std::streamoff>(objectSize);

        // Create new object state
        ObjectState state;
        state.endPos = endPos;

        // Read field positions
        uint32_t fieldCount;
        m_stream.read(reinterpret_cast<char*>(&fieldCount), sizeof(fieldCount));
        if (!m_stream)
        {
            return SerializationResult(getStreamErrorMessage());
        }

        for (uint32_t i = 0; i < fieldCount; ++i)
        {
            // Read field name
            std::string fieldName;
            uint8_t flag;
            m_stream.read(reinterpret_cast<char*>(&flag), sizeof(flag));
            if (!m_stream)
            {
                return SerializationResult(getStreamErrorMessage());
            }

            if (flag == 0)
            {
                // New string
                uint32_t length;
                m_stream.read(reinterpret_cast<char*>(&length), sizeof(length));
                if (!m_stream)
                {
                    return SerializationResult(getStreamErrorMessage());
                }

                if (length > 0)
                {
                    fieldName.resize(length);
                    m_stream.read(&fieldName[0], length);
                    if (!m_stream)
                    {
                        return SerializationResult(getStreamErrorMessage());
                    }
                }

                // Add to string cache
                uint32_t cacheId = static_cast<uint32_t>(m_stringCache.size());
                m_stringCache[cacheId] = fieldName;
            }
            else
            {
                // Cached string reference
                uint32_t cacheId;
                m_stream.read(reinterpret_cast<char*>(&cacheId), sizeof(cacheId));
                if (!m_stream)
                {
                    return SerializationResult(getStreamErrorMessage());
                }

                auto it = m_stringCache.find(cacheId);
                if (it != m_stringCache.end())
                {
                    fieldName = it->second;
                }
                else
                {
                    return SerializationResult("Invalid string cache reference: " + std::to_string(cacheId));
                }
            }

            // Read field position
            std::streampos fieldPos;
            m_stream.read(reinterpret_cast<char*>(&fieldPos), sizeof(fieldPos));
            if (!m_stream)
            {
                return SerializationResult(getStreamErrorMessage());
            }

            // Add to field map
            state.fields[fieldName] = fieldPos;
        }

        // Push to stack
        m_objectStack.push(state);

        // Push object name to stack
        m_objectNameStack.push_back(name);

        return SerializationResult();
    }

    SerializationResult BinaryDeserializer::endObject()
    {
        if (!m_stream)
        {
            return SerializationResult("Stream is not valid");
        }

        if (m_objectStack.empty())
        {
            return SerializationResult("No object to end");
        }

        // Seek to end of object
        m_stream.seekg(m_objectStack.top().endPos);
        if (!m_stream)
        {
            return SerializationResult(getStreamErrorMessage());
        }

        // Pop object from stack
        m_objectStack.pop();

        // Pop object name from name stack
        if (!m_objectNameStack.empty())
        {
            m_objectNameStack.pop_back();
        }

        return SerializationResult();
    }

    SerializationResult BinaryDeserializer::beginArray(const std::string& name, size_t& size)
    {
        if (!m_stream)
        {
            return SerializationResult("Stream is not valid");
        }

        // If name is not empty, find the field
        if (!name.empty() && !findField(name))
        {
            return SerializationResult("Field not found: " + name);
        }

        // Read value header
        ValueHeader header;
        auto result = readValueHeader(header);
        if (!result) return result;

        // Validate type
        if (header.type != ValueType::Array)
        {
            return SerializationResult("Expected array, got " + std::to_string(static_cast<int>(header.type)));
        }

        // Read array size
        uint32_t arraySize;
        m_stream.read(reinterpret_cast<char*>(&arraySize), sizeof(arraySize));
        if (!m_stream)
        {
            return SerializationResult(getStreamErrorMessage());
        }

        size = arraySize;

        // Read element type
        uint16_t elementType;
        m_stream.read(reinterpret_cast<char*>(&elementType), sizeof(elementType));
        if (!m_stream)
        {
            return SerializationResult(getStreamErrorMessage());
        }

        // Create array state
        ArrayState state;
        state.size = size;
        state.count = 0;
        state.startPos = m_stream.tellg();

        // Push to stack
        m_arrayStack.push(state);

        return SerializationResult();
    }

    SerializationResult BinaryDeserializer::endArray()
    {
        if (!m_stream)
        {
            return SerializationResult("Stream is not valid");
        }

        if (m_arrayStack.empty())
        {
            return SerializationResult("No array to end");
        }

        // Pop array from stack
        m_arrayStack.pop();

        return SerializationResult();
    }

    bool BinaryDeserializer::isNull()
    {
        if (!m_stream || !m_headerRead)
        {
            return false;
        }

        return m_currentHeader.type == ValueType::Null;
    }

    ValueType BinaryDeserializer::getValueType()
    {
        if (!m_stream || !m_headerRead)
        {
            return ValueType::Null;
        }

        // If we haven't read the header yet, read it now
        if (!m_headerRead)
        {
            ValueHeader header;
            readValueHeader(header);
            m_currentHeader = header;
        }

        return m_currentHeader.type;
    }

    SerializationResult BinaryDeserializer::readFieldName(std::string& name)
    {
        if (!m_stream)
        {
            return SerializationResult("Stream is not valid");
        }

        if (m_objectStack.empty())
        {
            return SerializationResult("No object context");
        }

        // Read field flag (cache or new)
        uint8_t flag;
        m_stream.read(reinterpret_cast<char*>(&flag), sizeof(flag));
        if (!m_stream)
        {
            return SerializationResult(getStreamErrorMessage());
        }

        if (flag == 0)
        {
            // New string
            uint32_t length;
            m_stream.read(reinterpret_cast<char*>(&length), sizeof(length));
            if (!m_stream)
            {
                return SerializationResult(getStreamErrorMessage());
            }

            if (length > 0)
            {
                name.resize(length);
                m_stream.read(&name[0], length);
                if (!m_stream)
                {
                    return SerializationResult(getStreamErrorMessage());
                }
            }
            else
            {
                name.clear();
            }

            // Add to string cache
            uint32_t cacheId = static_cast<uint32_t>(m_stringCache.size());
            m_stringCache[cacheId] = name;
        }
        else
        {
            // Cached string reference
            uint32_t cacheId;
            m_stream.read(reinterpret_cast<char*>(&cacheId), sizeof(cacheId));
            if (!m_stream)
            {
                return SerializationResult(getStreamErrorMessage());
            }

            auto it = m_stringCache.find(cacheId);
            if (it != m_stringCache.end())
            {
                name = it->second;
            }
            else
            {
                return SerializationResult("Invalid string cache reference: " + std::to_string(cacheId));
            }
        }

        m_currentField = name;
        return SerializationResult();
    }

    SerializationResult BinaryDeserializer::readBool(bool& value)
    {
        auto result = verifyValueType(ValueType::Bool);
        if (!result) return result;

        // Read boolean value (1 byte)
        uint8_t boolValue;
        m_stream.read(reinterpret_cast<char*>(&boolValue), sizeof(boolValue));
        if (!m_stream)
        {
            return SerializationResult(getStreamErrorMessage());
        }

        value = (boolValue != 0);

        // Update array state if needed
        if (!m_arrayStack.empty())
        {
            m_arrayStack.top().count++;
        }

        return SerializationResult();
    }

    SerializationResult BinaryDeserializer::readInt(int64_t& value)
    {
        auto result = verifyValueType(ValueType::Int8);
        if (!result) result = verifyValueType(ValueType::Int16);
        if (!result) result = verifyValueType(ValueType::Int32);
        if (!result) result = verifyValueType(ValueType::Int64);
        if (!result) return result;

        // Read integer value based on type
        switch (m_currentHeader.type)
        {
            case ValueType::Int8:
            {
                int8_t val;
                result = readRawData(&val, sizeof(val));
                value = val;
                break;
            }
            case ValueType::Int16:
            {
                int16_t val;
                result = readRawData(&val, sizeof(val));
                value = val;
                break;
            }
            case ValueType::Int32:
            {
                int32_t val;
                result = readRawData(&val, sizeof(val));
                value = val;
                break;
            }
            case ValueType::Int64:
            {
                result = readRawData(&value, sizeof(value));
                break;
            }
            default:
                return SerializationResult("Unexpected value type");
        }

        if (!result) return result;

        // Update array state if needed
        if (!m_arrayStack.empty())
        {
            m_arrayStack.top().count++;
        }

        return SerializationResult();
    }

    SerializationResult BinaryDeserializer::readUInt(uint64_t& value)
    {
        auto result = verifyValueType(ValueType::UInt8);
        if (!result) result = verifyValueType(ValueType::UInt16);
        if (!result) result = verifyValueType(ValueType::UInt32);
        if (!result) result = verifyValueType(ValueType::UInt64);
        if (!result) return result;

        // Read unsigned integer value based on type
        switch (m_currentHeader.type)
        {
            case ValueType::UInt8:
            {
                uint8_t val;
                result = readRawData(&val, sizeof(val));
                value = val;
                break;
            }
            case ValueType::UInt16:
            {
                uint16_t val;
                result = readRawData(&val, sizeof(val));
                value = val;
                break;
            }
            case ValueType::UInt32:
            {
                uint32_t val;
                result = readRawData(&val, sizeof(val));
                value = val;
                break;
            }
            case ValueType::UInt64:
            {
                result = readRawData(&value, sizeof(value));
                break;
            }
            default:
                return SerializationResult("Unexpected value type");
        }

        if (!result) return result;

        // Update array state if needed
        if (!m_arrayStack.empty())
        {
            m_arrayStack.top().count++;
        }

        return SerializationResult();
    }

    SerializationResult BinaryDeserializer::readFloat(float& value)
    {
        auto result = verifyValueType(ValueType::Float);
        if (!result) return result;

        // Read float value
        result = readRawData(&value, sizeof(value));
        if (!result) return result;

        // Update array state if needed
        if (!m_arrayStack.empty())
        {
            m_arrayStack.top().count++;
        }

        return SerializationResult();
    }

    SerializationResult BinaryDeserializer::readDouble(double& value)
    {
        auto result = verifyValueType(ValueType::Double);
        if (!result) return result;

        // Read double value
        result = readRawData(&value, sizeof(value));
        if (!result) return result;

        // Update array state if needed
        if (!m_arrayStack.empty())
        {
            m_arrayStack.top().count++;
        }

        return SerializationResult();
    }

    SerializationResult BinaryDeserializer::readString(std::string& value)
    {
        auto result = verifyValueType(ValueType::String);
        if (!result) return result;

        // Resize string to header size
        value.resize(m_currentHeader.size);

        // Read string data
        if (m_currentHeader.size > 0)
        {
            m_stream.read(&value[0], m_currentHeader.size);
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

        return SerializationResult();
    }

    SerializationResult BinaryDeserializer::readBinary(void* data, size_t size, size_t& actualSize)
    {
        auto result = verifyValueType(ValueType::Binary);
        if (!result) return result;

        // Check if buffer is large enough
        actualSize = m_currentHeader.size;
        if (size < actualSize)
        {
            return SerializationResult("Buffer too small: required " + std::to_string(actualSize) +
                                       " bytes, got " + std::to_string(size));
        }

        // Read binary data
        if (actualSize > 0)
        {
            m_stream.read(static_cast<char*>(data), actualSize);
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

        return SerializationResult();
    }

    SerializationResult BinaryDeserializer::readEntityRef(uint64_t& entityId)
    {
        auto result = verifyValueType(ValueType::EntityRef);
        if (!result) return result;

        // Read entity reference as string
        std::string entityName;
        entityName.resize(m_currentHeader.size);

        if (m_currentHeader.size > 0)
        {
            m_stream.read(&entityName[0], m_currentHeader.size);
            if (!m_stream)
            {
                return SerializationResult(getStreamErrorMessage());
            }
        }

        // Resolve entity ID
        if (m_entityResolver)
        {
            entityId = m_entityResolver(entityName);
        }
        else
        {
            // Default to parsing as number if no resolver
            try
            {
                entityId = std::stoull(entityName);
            }
            catch (const std::exception& e)
            {
                return SerializationResult("Failed to parse entity ID: " + entityName);
            }
        }

        // Update array state if needed
        if (!m_arrayStack.empty())
        {
            m_arrayStack.top().count++;
        }

        return SerializationResult();
    }

    SerializationResult BinaryDeserializer::readResourceRef(std::string& resourceName)
    {
        auto result = verifyValueType(ValueType::ResourceRef);
        if (!result) return result;

        // Read resource reference as string
        resourceName.resize(m_currentHeader.size);

        if (m_currentHeader.size > 0)
        {
            m_stream.read(&resourceName[0], m_currentHeader.size);
            if (!m_stream)
            {
                return SerializationResult(getStreamErrorMessage());
            }
        }

        // Validate resource reference if resolver is set
        if (m_resourceResolver && !resourceName.empty())
        {
            if (!m_resourceResolver(resourceName))
            {
                Log::warn("Resource not found: " + resourceName);
            }
        }

        // Update array state if needed
        if (!m_arrayStack.empty())
        {
            m_arrayStack.top().count++;
        }

        return SerializationResult();
    }

    bool BinaryDeserializer::findField(const std::string& name)
    {
        if (!m_stream || m_objectStack.empty())
        {
            return false;
        }

        // Get object state
        const ObjectState& state = m_objectStack.top();

        // Look up field position
        auto it = state.fields.find(name);
        if (it == state.fields.end())
        {
            return false;
        }

        // Seek to field position
        m_stream.seekg(it->second);
        if (!m_stream)
        {
            return false;
        }

        m_currentField = name;
        m_headerRead = false; // Force re-read of value header
        return true;
    }

    bool BinaryDeserializer::hasField(const std::string& name)
    {
        if (!m_stream || m_objectStack.empty())
        {
            return false;
        }

        // Get object state
        const ObjectState& state = m_objectStack.top();

        // Check if field exists
        return state.fields.find(name) != state.fields.end();
    }

    SerializationResult BinaryDeserializer::skipValue()
    {
        if (!m_stream)
        {
            return SerializationResult("Stream is not valid");
        }

        // Read value header if needed
        if (!m_headerRead)
        {
            ValueHeader header;
            auto result = readValueHeader(header);
            if (!result) return result;

            m_currentHeader = header;
        }

        // Skip based on type
        switch (m_currentHeader.type)
        {
            case ValueType::Null:
            case ValueType::Bool:
                // Single byte, already read in header
                break;

            case ValueType::Int8:
            case ValueType::UInt8:
                m_stream.seekg(1, std::ios::cur);
                break;

            case ValueType::Int16:
            case ValueType::UInt16:
                m_stream.seekg(2, std::ios::cur);
                break;

            case ValueType::Int32:
            case ValueType::UInt32:
            case ValueType::Float:
                m_stream.seekg(4, std::ios::cur);
                break;

            case ValueType::Int64:
            case ValueType::UInt64:
            case ValueType::Double:
                m_stream.seekg(8, std::ios::cur);
                break;

            case ValueType::String:
            case ValueType::Binary:
            case ValueType::EntityRef:
            case ValueType::ResourceRef:
                m_stream.seekg(m_currentHeader.size, std::ios::cur);
                break;

            case ValueType::Array:
                // Skip the entire array by reading array size and elements
            {
                uint32_t arraySize;
                m_stream.read(reinterpret_cast<char*>(&arraySize), sizeof(arraySize));
                if (!m_stream)
                {
                    return SerializationResult(getStreamErrorMessage());
                }

                // Skip element type
                m_stream.seekg(sizeof(uint16_t), std::ios::cur);
                if (!m_stream)
                {
                    return SerializationResult(getStreamErrorMessage());
                }

                // Skip each element
                for (uint32_t i = 0; i < arraySize; ++i)
                {
                    auto result = skipValue();
                    if (!result) return result;
                }
            }
            break;

            case ValueType::Object:
                // Skip to end of object
            {
                uint32_t objectSize;
                m_stream.read(reinterpret_cast<char*>(&objectSize), sizeof(objectSize));
                if (!m_stream)
                {
                    return SerializationResult(getStreamErrorMessage());
                }

                m_stream.seekg(objectSize, std::ios::cur);
                if (!m_stream)
                {
                    return SerializationResult(getStreamErrorMessage());
                }
            }
            break;
        }

        if (!m_stream)
        {
            return SerializationResult(getStreamErrorMessage());
        }

        // Update array state if needed
        if (!m_arrayStack.empty())
        {
            m_arrayStack.top().count++;
        }

        m_headerRead = false;
        return SerializationResult();
    }

    VersionInfo BinaryDeserializer::getVersion() const
    {
        return m_version;
    }

    SerializationResult BinaryDeserializer::readValueHeader(ValueHeader& header)
    {
        if (!m_stream)
        {
            return SerializationResult("Stream is not valid");
        }

        // Read value type
        uint16_t typeValue;
        m_stream.read(reinterpret_cast<char*>(&typeValue), sizeof(typeValue));
        if (!m_stream)
        {
            return SerializationResult(getStreamErrorMessage());
        }

        header.type = static_cast<ValueType>(typeValue);

        // Read value size if needed
        if (header.type != ValueType::Null && header.type != ValueType::Bool)
        {
            m_stream.read(reinterpret_cast<char*>(&header.size), sizeof(header.size));
            if (!m_stream)
            {
                return SerializationResult(getStreamErrorMessage());
            }
        }
        else
        {
            header.size = 0;
        }

        m_headerRead = true;
        m_currentHeader = header;
        return SerializationResult();
    }

    SerializationResult BinaryDeserializer::readRawData(void* data, size_t size)
    {
        if (!m_stream)
        {
            return SerializationResult("Stream is not valid");
        }

        m_stream.read(static_cast<char*>(data), size);
        if (!m_stream)
        {
            return SerializationResult(getStreamErrorMessage());
        }

        return SerializationResult();
    }

    std::string BinaryDeserializer::getStreamErrorMessage() const
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

    SerializationResult BinaryDeserializer::verifyValueType(ValueType expectedType)
    {
        if (!m_stream)
        {
            return SerializationResult("Stream is not valid");
        }

        // Read header if needed
        if (!m_headerRead)
        {
            ValueHeader header;
            auto result = readValueHeader(header);
            if (!result) return result;

            m_currentHeader = header;
        }

        // Check if type matches
        if (m_currentHeader.type != expectedType)
        {
            return SerializationResult("Type mismatch: expected " + std::to_string(static_cast<int>(expectedType)) +
                                       ", got " + std::to_string(static_cast<int>(m_currentHeader.type)));
        }

        return SerializationResult();
    }

} // namespace PixelCraft::Utility::Serialization