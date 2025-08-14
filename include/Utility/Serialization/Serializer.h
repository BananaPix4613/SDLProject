// -------------------------------------------------------------------------
// Utility/Serialization/Serializer.h
// -------------------------------------------------------------------------
#pragma once

#include "Utility/Serialization/SchemaRegistry.h"
#include "Utility/Serialization/SerializationTypes.h"

#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <string>
#include <vector>

namespace PixelCraft::Utility::Serialization 
{

    /**
     * @brief Base class for serialization
     */
    class Serializer 
    {
    public:
        /**
         * @brief Constructor
         */
        Serializer();

        /**
         * @brief Virtual destructor
         */
        virtual ~Serializer();

        /**
         * @brief Begin writing an object
         * @param name Name of the object
         * @param schema Optional schema for the object
         * @return Result of the operation
         */
        virtual SerializationResult beginObject(const std::string &name, const Schema *schema = nullptr) = 0;

        /**
         * @brief End writing an object
         * @return Result of the operation
         */
        virtual SerializationResult endObject() = 0;

        /**
         * @brief Begin writing an array
         * @param name Name of the array
         * @param size Size of the array
         * @param elementType Optional type of array elements
         * @return Result of the operation
         */
        virtual SerializationResult beginArray(const std::string &name, size_t size,
                                            ValueType elementType = ValueType::Null) = 0;

        /**
         * @brief End writing an array
         * @return Result of the operation
         */
        virtual SerializationResult endArray() = 0;

        /**
         * @brief Write a field name for the next value
         * @param name Field name
         * @return Result of the operation
         */
        virtual SerializationResult writeFieldName(const std::string &name) = 0;

        /**
         * @brief Write a null value
         * @return Result of the operation
         */
        virtual SerializationResult writeNull() = 0;

        /**
         * @brief Write a boolean value
         * @param value Boolean value
         * @return Result of the operation
         */
        virtual SerializationResult writeBool(bool value) = 0;

        /**
         * @brief Write an integer value
         * @param value Integer value
         * @return Result of the operation
         */
        virtual SerializationResult writeInt(int64_t value) = 0;

        /**
         * @brief Write an unsigned integer value
         * @param value Unsigned integer value
         * @return Result of the operation
         */
        virtual SerializationResult writeUInt(uint64_t value) = 0;

        /**
         * @brief Write a float value
         * @param value Float value
         * @return Result of the operation
         */
        virtual SerializationResult writeFloat(float value) = 0;

        /**
         * @brief Write a double value
         * @param value Double value
         * @return Result of the operation
         */
        virtual SerializationResult writeDouble(double value) = 0;

        /**
         * @brief Write a glm::vec2 value
         * @param value glm::vec2 value
         * @return Result of the operation
         */
        virtual SerializationResult writeVec2(const glm::vec2 &value) = 0;

        /**
         * @brief Write a glm::ivec2 value
         * @param value glm::ivec2 value
         * @return Result of the operation
         */
        virtual SerializationResult writeIVec2(const glm::ivec2 &value) = 0;

        /**
         * @brief Write a glm::vec3 value
         * @param value glm::vec3 value
         * @return Result of the operation
         */
        virtual SerializationResult writeVec3(const glm::vec3 &value) = 0;

        /**
         * @brief Write a glm::ivec3 value
         * @param value glm::ivec3 value
         * @return Result of the operation
         */
        virtual SerializationResult writeIVec3(const glm::ivec3 &value) = 0;

        /**
         * @brief Write a glm::vec4 value
         * @param value glm::vec4 value
         * @return Result of the operation
         */
        virtual SerializationResult writeVec4(const glm::vec4 &value) = 0;

        /**
         * @brief Write a glm::ivec4 value
         * @param value glm::ivec4 value
         * @return Result of the operation
         */
        virtual SerializationResult writeIVec4(const glm::ivec4 &value) = 0;

        /**
         * @brief Write a glm::mat3 value
         * @param value glm::mat3 value
         * @return Result of the operation
         */
        virtual SerializationResult writeMat3(const glm::mat3 &value) = 0;

        /**
         * @brief Write a glm::mat4 value
         * @param value glm::mat4 value
         * @return Result of the operation
         */
        virtual SerializationResult writeMat4(const glm::mat4 &value) = 0;

        /**
         * @brief Write a glm::quat value
         * @param value glm::quat value
         * @return Result of the operation
         */
        virtual SerializationResult writeQuat(const glm::quat &value) = 0;

        /**
         * @brief Write a string value
         * @param value String value
         * @return Result of the operation
         */
        virtual SerializationResult writeString(const std::string &value) = 0;

        /**
         * @brief Write binary data
         * @param data Binary data
         * @param size Size of data in bytes
         * @return Result of the operation
         */
        virtual SerializationResult writeBinary(const void *data, size_t size) = 0;

        /**
         * @brief Write an entity reference
         * @param entityId Entity ID
         * @return Result of the operation
         */
        virtual SerializationResult writeEntityRef(uint32_t entityId) = 0;

        /**
         * @brief Write a resource reference
         * @param resourceName Resource name
         * @return Result of the operation
         */
        virtual SerializationResult writeResourceRef(const std::string &resourceName) = 0;

        /**
         * @brief Write a field with a specified value
         * @param name Field name
         * @param value Field value
         * @return Result of the operation
         */
        template <typename T> SerializationResult writeField(const std::string &name, const T &value)
        {
            auto result = writeFieldName(name);
            if (!result)
                return result;
            return writeValue(value);
        }

        /**
         * @brief Write a value of a specified type
         * @param value Value to write
         * @return Result of the operation
         */
        template <typename T> SerializationResult writeValue(const T &value)
        {
            // Default implementation delegates to schema-based serialization
            static_assert(has_getTypeName<T>::value || dependent_false<T>::value,
                        "Type used with Serializer::writeValue must have a static getTypeName() method. "
                        "Check the static_assert instantiation for the attempted type.");
            auto schema = SchemaRegistry::getInstance().getSchema(T::getTypeName());
            if (!schema)
            {
                return SerializationResult("No schema registered for type");
            }
            return schema->serialize(&value, *this);
        }

        /**
         * @brief Template catch for writing char to convert to string
         * @param value Char value to write
         * @return Result of the operation
         */
        template <size_t N> SerializationResult writeValue(const char (&value)[N]);

        /**
         * @brief Get serialization format
         * @return Format of the serializer
         */
        SerializationFormat getFormat() const
        {
            return m_format;
        }

        /**
         * @brief Set entity resolver function
         * @param resolver Function to resolve entity references
         */
        void setEntityResolver(std::function<std::string(uint32_t)> resolver)
        {
            m_entityResolver = std::move(resolver);
        }

        /**
         * @brief Set resource resolver function
         * @param resolver Function to resolve resource references
         */
        void setResourceResolver(std::function<bool(const std::string &)> resolver)
        {
            m_resourceResolver = std::move(resolver);
        }

    protected:
        SerializationFormat m_format;
        std::vector<std::string> m_objectStack;
        std::function<std::string(uint32_t)> m_entityResolver;
        std::function<bool(const std::string &)> m_resourceResolver;
    };

    // Template specializations for primitive types
    template <> SerializationResult Serializer::writeValue<bool>(const bool &value);
    template <> SerializationResult Serializer::writeValue<int8_t>(const int8_t &value);
    template <> SerializationResult Serializer::writeValue<uint8_t>(const uint8_t &value);
    template <> SerializationResult Serializer::writeValue<int16_t>(const int16_t &value);
    template <> SerializationResult Serializer::writeValue<uint16_t>(const uint16_t &value);
    template <> SerializationResult Serializer::writeValue<int32_t>(const int32_t &value);
    template <> SerializationResult Serializer::writeValue<uint32_t>(const uint32_t &value);
    template <> SerializationResult Serializer::writeValue<int64_t>(const int64_t &value);
    template <> SerializationResult Serializer::writeValue<uint64_t>(const uint64_t &value);
    template <> SerializationResult Serializer::writeValue<float>(const float &value);
    template <> SerializationResult Serializer::writeValue<double>(const double &value);
    template <> SerializationResult Serializer::writeValue<glm::vec2>(const glm::vec2 &value);
    template <> SerializationResult Serializer::writeValue<glm::ivec2>(const glm::ivec2 &value);
    template <> SerializationResult Serializer::writeValue<glm::vec3>(const glm::vec3 &value);
    template <> SerializationResult Serializer::writeValue<glm::ivec3>(const glm::ivec3 &value);
    template <> SerializationResult Serializer::writeValue<glm::vec4>(const glm::vec4 &value);
    template <> SerializationResult Serializer::writeValue<glm::ivec4>(const glm::ivec4 &value);
    template <> SerializationResult Serializer::writeValue<glm::mat3>(const glm::mat3 &value);
    template <> SerializationResult Serializer::writeValue<glm::mat4>(const glm::mat4 &value);
    template <> SerializationResult Serializer::writeValue<glm::quat>(const glm::quat &value);
    template <> SerializationResult Serializer::writeValue<std::string>(const std::string &value);

} // namespace PixelCraft::Utility::Serialization