// -------------------------------------------------------------------------
// Utility/Serialization/Schema.cpp
// -------------------------------------------------------------------------
#include "Utility/Serialization/Schema.h"
#include "Utility/Serialization/Serializer.h"
#include "Utility/Serialization/Deserializer.h"
#include "Core/Logger.h"

namespace PixelCraft::Utility::Serialization
{

    Schema::Schema(const std::string& typeName, const VersionInfo& version)
        : m_typeName(typeName), m_version(version)
    {
    }

    Schema::~Schema()
    {
    }

    void Schema::addField(const std::string& name, ValueType type, bool required, size_t offset)
    {
        Field field;
        field.name = name;
        field.type = type;
        field.required = required;
        field.offset = offset;
        field.typeName = "";

        m_fields.push_back(field);
    }

    void Schema::addComplexField(const std::string& name, const std::string& typeName, bool required, size_t offset)
    {
        Field field;
        field.name = name;
        field.type = ValueType::Object;
        field.required = required;
        field.offset = offset;
        field.typeName = typeName;

        m_fields.push_back(field);
    }

    void Schema::addArrayField(const std::string& name, ValueType elementType, bool required, size_t offset)
    {
        Field field;
        field.name = name;
        field.type = ValueType::Array;
        field.required = required;
        field.offset = offset;
        // Store element type encoded in the typeName for primitive types
        field.typeName = std::to_string(static_cast<int>(elementType));

        m_fields.push_back(field);
    }

    void Schema::addComplexArrayField(const std::string& name, const std::string& elementTypeName, bool required, size_t offset)
    {
        Field field;
        field.name = name;
        field.type = ValueType::Array;
        field.required = required;
        field.offset = offset;
        // Prefix with 'c:' to indicate a complex type array
        field.typeName = "c:" + elementTypeName;

        m_fields.push_back(field);
    }

    const Schema::Field* Schema::getField(const std::string& name) const
    {
        for (const auto& field : m_fields)
        {
            if (field.name == name)
            {
                return &field;
            }
        }
        return nullptr;
    }

    SerializationResult Schema::validateField(const Field& field, ValueType actualType) const
    {
        if (field.type != actualType)
        {
            return SerializationResult("Type mismatch for field '" + field.name +
                                       "'. Expected " + std::to_string(static_cast<int>(field.type)) +
                                       ", got " + std::to_string(static_cast<int>(actualType)));
        }
        return SerializationResult();
    }

    SerializationResult Schema::serialize(const void* obj, Serializer& serializer) const
    {
        if (!m_serializeFunc)
        {
            return SerializationResult("No serialization function registered for type: " + m_typeName);
        }

        return m_serializeFunc(obj, serializer);
    }

    SerializationResult Schema::deserialize(void* obj, Deserializer& deserializer) const
    {
        if (!m_deserializeFunc)
        {
            return SerializationResult("No deserialization function registered for type: " + m_typeName);
        }

        return m_deserializeFunc(obj, deserializer);
    }

} // namespace PixelCraft::Utility::Serialization