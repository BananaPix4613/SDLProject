// -------------------------------------------------------------------------
// Utility/Serialization/Deserializer.h
// -------------------------------------------------------------------------
#pragma once

#include "Utility/Serialization/SerializationTypes.h"
#include "Utility/Serialization/SchemaRegistry.h"
#include "Utility/Serialization/Schema.h"

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace PixelCraft::Utility::Serialization
{

    /**
     * @brief Base class for deserialization
     */
    class Deserializer
    {
    public:
        /**
         * @brief Constructor
         */
        Deserializer();

        /**
         * @brief Virtual destructor
         */
        virtual ~Deserializer();

        /**
         * @brief Begin reading an object
         * @param name Expected name of the object
         * @param schema Optional schema for the object
         * @return Result of the operation
         */
        virtual SerializationResult beginObject(const std::string& name, const Schema* schema = nullptr) = 0;

        /**
         * @brief End reading an object
         * @return Result of the operation
         */
        virtual SerializationResult endObject() = 0;

        /**
         * @brief Begin reading an array
         * @param name Expected name of the array
         * @param size Output parameter for array size
         * @return Result of the operation
         */
        virtual SerializationResult beginArray(const std::string& name, size_t& size) = 0;

        /**
         * @brief End reading an array
         * @return Result of the operation
         */
        virtual SerializationResult endArray() = 0;

        /**
         * @brief Check if the current field is null
         * @return True if the field is null
         */
        virtual bool isNull() = 0;

        /**
         * @brief Get the type of the current value
         * @return Value type
         */
        virtual ValueType getValueType() = 0;

        /**
         * @brief Read a field name
         * @param name Output parameter for field name
         * @return Result of the operation
         */
        virtual SerializationResult readFieldName(std::string& name) = 0;

        /**
         * @brief Read a boolean value
         * @param value Output parameter for boolean value
         * @return Result of the operation
         */
        virtual SerializationResult readBool(bool& value) = 0;

        /**
         * @brief Read an integer value
         * @param value Output parameter for integer value
         * @return Result of the operation
         */
        virtual SerializationResult readInt(int64_t& value) = 0;

        /**
         * @brief Read an unsigned integer value
         * @param value Output parameter for unsigned integer value
         * @return Result of the operation
         */
        virtual SerializationResult readUInt(uint64_t& value) = 0;

        /**
         * @brief Read a float value
         * @param value Output parameter for float value
         * @return Result of the operation
         */
        virtual SerializationResult readFloat(float& value) = 0;

        /**
         * @brief Read a double value
         * @param value Output parameter for double value
         * @return Result of the operation
         */
        virtual SerializationResult readDouble(double& value) = 0;

        /**
         * @brief Read a glm::vec2 value
         * @param value Output parameter for glm::vec2 value
         * @return Result of the operation
         */
        virtual SerializationResult readVec2(glm::vec2& value) = 0;

        /**
         * @brief Read a glm::ivec2 value
         * @param value Output parameter for glm::ivec2 value
         * @return Result of the operation
         */
        virtual SerializationResult readIVec2(glm::ivec2& value) = 0;

        /**
         * @brief Read a glm::vec3 value
         * @param value Output parameter for glm::vec3 value
         * @return Result of the operation
         */
        virtual SerializationResult readVec3(glm::vec3& value) = 0;

        /**
         * @brief Read a glm::ivec3 value
         * @param value Output parameter for glm::ivec3 value
         * @return Result of the operation
         */
        virtual SerializationResult readIVec3(glm::ivec3& value) = 0;

        /**
         * @brief Read a glm::vec4 value
         * @param value Output parameter for glm::vec4 value
         * @return Result of the operation
         */
        virtual SerializationResult readVec4(glm::vec4& value) = 0;

        /**
         * @brief Read a glm::ivec4 value
         * @param value Output parameter for glm::ivec4 value
         * @return Result of the operation
         */
        virtual SerializationResult readIVec4(glm::ivec4& value) = 0;

        /**
         * @brief Read a glm::mat3 value
         * @param value Output parameter for glm::mat3 value
         * @return Result of the operation
         */
        virtual SerializationResult readMat3(glm::mat3& value) = 0;

        /**
         * @brief Read a glm::mat4 value
         * @param value Output parameter for glm::mat4 value
         * @return Result of the operation
         */
        virtual SerializationResult readMat4(glm::mat4& value) = 0;

        /**
         * @brief Read a glm::quat value
         * @param value Output parameter for glm::quat value
         * @return Result of the operation
         */
        virtual SerializationResult readQuat(glm::quat& value) = 0;

        /**
         * @brief Read a string value
         * @param value Output parameter for string value
         * @return Result of the operation
         */
        virtual SerializationResult readString(std::string& value) = 0;

        /**
         * @brief Read binary data
         * @param data Output buffer for binary data
         * @param size Size of the buffer in bytes
         * @param actualSize Output parameter for actual size read
         * @return Result of the operation
         */
        virtual SerializationResult readBinary(void* data, size_t size, size_t& actualSize) = 0;

        /**
         * @brief Read an entity reference
         * @param entityId Output parameter for entity ID
         * @return Result of the operation
         */
        virtual SerializationResult readEntityRef(uint32_t& entityId) = 0;

        /**
         * @brief Read a resource reference
         * @param resourceName Output parameter for resource name
         * @return Result of the operation
         */
        virtual SerializationResult readResourceRef(std::string& resourceName) = 0;

        /**
         * @brief Find a field by name
         * @param name Field name to find
         * @return True if the field was found
         */
        virtual bool findField(const std::string& name) = 0;

        /**
         * @brief Check if a field exists
         * @param name Field name to check
         * @return True if the field exists
         */
        virtual bool hasField(const std::string& name) = 0;

        /**
         * @brief Skip the current value
         * @return Result of the operation
         */
        virtual SerializationResult skipValue() = 0;

        /**
         * @brief Read a field with a specified value
         * @param name Field name
         * @param value Output parameter for field value
         * @return Result of the operation
         */
        template<typename T>
        SerializationResult readField(const std::string& name, T& value)
        {
            if (!findField(name))
            {
                return SerializationResult("Field not found: " + name);
            }
            return readValue(value);
        }

        /**
         * @brief Read a value of a specified type
         * @param value Output parameter for value
         * @return Result of the operation
         */
        template<typename T>
        SerializationResult readValue(T& value)
        {
            // Reject const references
            static_assert(!std::is_const_v<T>, "Cannot deserialize into const references");
            
            // Default implementation delegates to schema-based deserialization
            static_assert(has_getTypeName<T>::value,
                "Type used with Deserializer::readValue must have a static getTypeName() method.");
            auto schema = SchemaRegistry::getInstance().getSchema(T::getTypeName());
            if (!schema)
            {
                return SerializationResult("No schema registered for type");
            }
            return schema->deserialize(&value, *this);
        }

        /**
         * @brief Get deserialization format
         * @return Format of the deserializer
         */
        SerializationFormat getFormat() const
        {
            return m_format;
        }

        /**
         * @brief Set entity resolver function
         * @param resolver Function to resolve entity references
         */
        void setEntityResolver(std::function<uint32_t(const std::string&)> resolver)
        {
            m_entityResolver = std::move(resolver);
        }

        /**
         * @brief Set resource resolver function
         * @param resolver Function to resolve resource references
         */
        void setResourceResolver(std::function<bool(const std::string&)> resolver)
        {
            m_resourceResolver = std::move(resolver);
        }

        /**
         * @brief Get the version info from the current stream
         * @return Version info
         */
        virtual VersionInfo getVersion() const = 0;

    protected:
        SerializationFormat m_format;
        std::vector<std::string> m_objectStack;
        std::function<uint32_t(const std::string&)> m_entityResolver;
        std::function<bool(const std::string&)> m_resourceResolver;
    };

    // Template specializations for primitive types
    template<> SerializationResult Deserializer::readValue<bool>(bool& value);
    template<> SerializationResult Deserializer::readValue<int8_t>(int8_t& value);
    template<> SerializationResult Deserializer::readValue<uint8_t>(uint8_t& value);
    template<> SerializationResult Deserializer::readValue<int16_t>(int16_t& value);
    template<> SerializationResult Deserializer::readValue<uint16_t>(uint16_t& value);
    template<> SerializationResult Deserializer::readValue<int32_t>(int32_t& value);
    template<> SerializationResult Deserializer::readValue<uint32_t>(uint32_t& value);
    template<> SerializationResult Deserializer::readValue<int64_t>(int64_t& value);
    template<> SerializationResult Deserializer::readValue<uint64_t>(uint64_t& value);
    template<> SerializationResult Deserializer::readValue<float>(float& value);
    template<> SerializationResult Deserializer::readValue<double>(double& value);
    template<> SerializationResult Deserializer::readValue<glm::vec2>(glm::vec2& value);
    template<> SerializationResult Deserializer::readValue<glm::ivec2>(glm::ivec2& value);
    template<> SerializationResult Deserializer::readValue<glm::vec3>(glm::vec3& value);
    template<> SerializationResult Deserializer::readValue<glm::ivec3>(glm::ivec3& value);
    template<> SerializationResult Deserializer::readValue<glm::vec4>(glm::vec4& value);
    template<> SerializationResult Deserializer::readValue<glm::ivec4>(glm::ivec4& value);
    template<> SerializationResult Deserializer::readValue<glm::mat3>(glm::mat3& value);
    template<> SerializationResult Deserializer::readValue<glm::mat4>(glm::mat4& value);
    template<> SerializationResult Deserializer::readValue<glm::quat>(glm::quat& value);
    template<> SerializationResult Deserializer::readValue<std::string>(std::string& value);

} // namespace PixelCraft::Utility::Serialization