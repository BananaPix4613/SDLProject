// -------------------------------------------------------------------------
// Utility/Serialization/BinarySerializer.h
// -------------------------------------------------------------------------
#pragma once

#include "Utility/Serialization/Serializer.h"

#include <ostream>
#include <stack>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace PixelCraft::Utility::Serialization
{

    /**
     * @brief Binary format serializer
     */
    class BinarySerializer : public Serializer
    {
    public:
        /**
         * @brief Constructor with output stream
         * @param stream Output stream for serialized data
         */
        explicit BinarySerializer(std::ostream& stream);

        /**
         * @brief Destructor
         */
        ~BinarySerializer() override;

        /**
         * @brief Begin writing an object
         * @param name Name of the object
         * @param schema Optional schema for the object
         * @return Result of the operation
         */
        SerializationResult beginObject(const std::string& name, const Schema* schema = nullptr) override;

        /**
         * @brief End writing an object
         * @return Result of the operation
         */
        SerializationResult endObject() override;

        /**
         * @brief Begin writing an array
         * @param name Name of the array
         * @param size Size of the array
         * @param elementType Optional type of array elements
         * @return Result of the operation
         */
        SerializationResult beginArray(const std::string& name, size_t size, ValueType elementType = ValueType::Null) override;

        /**
         * @brief End writing an array
         * @return Result of the operation
         */
        SerializationResult endArray() override;

        /**
         * @brief Write a field name for the next value
         * @param name Field name
         * @return Result of the operation
         */
        SerializationResult writeFieldName(const std::string& name) override;

        /**
         * @brief Write a null value
         * @return Result of the operation
         */
        SerializationResult writeNull() override;

        /**
         * @brief Write a boolean value
         * @param value Boolean value
         * @return Result of the operation
         */
        SerializationResult writeBool(bool value) override;

        /**
         * @brief Write an integer value
         * @param value Integer value
         * @return Result of the operation
         */
        SerializationResult writeInt(int64_t value) override;

        /**
         * @brief Write an unsigned integer value
         * @param value Unsigned integer value
         * @return Result of the operation
         */
        SerializationResult writeUInt(uint64_t value) override;

        /**
         * @brief Write a float value
         * @param value Float value
         * @return Result of the operation
         */
        SerializationResult writeFloat(float value) override;

        /**
         * @brief Write a double value
         * @param value Double value
         * @return Result of the operation
         */
        SerializationResult writeDouble(double value) override;

        /**
         * @brief Write a string value
         * @param value String value
         * @return Result of the operation
         */
        SerializationResult writeString(const std::string& value) override;

        /**
         * @brief Write a glm::vec2 value
         * @param value glm::vec2 value
         * @return Result of the operation
         */
        SerializationResult writeVec2(const glm::vec2& value);

        /**
         * @brief Write a glm::ivec2 value
         * @param value glm::ivec2 value
         * @return Result of the operation
         */
        SerializationResult writeIVec2(const glm::ivec2& value);

        /**
         * @brief Write a glm::vec3 value
         * @param value glm::vec3 value
         * @return Result of the operation
         */
        SerializationResult writeVec3(const glm::vec3& value);

        /**
         * @brief Write a glm::ivec3 value
         * @param value glm::ivec3 value
         * @return Result of the operation
         */
        SerializationResult writeIVec3(const glm::ivec3& value);

        /**
         * @brief Write a glm::vec4 value
         * @param value glm::vec4 value
         * @return Result of the operation
         */
        SerializationResult writeVec4(const glm::vec4& value);

        /**
         * @brief Write a glm::ivec4 value
         * @param value glm::ivec4 value
         * @return Result of the operation
         */
        SerializationResult writeIVec4(const glm::ivec4& value);
        
        /**
         * @brief Write a glm::mat3 value
         * @param value glm::mat3 value
         * @return Result of the operation
         */
        SerializationResult writeMat3(const glm::mat3& value);

        /**
         * @brief Write a glm::mat4 value
         * @param value glm::mat4 value
         * @return Result of the operation
         */
        SerializationResult writeMat4(const glm::mat4& value);

        /**
         * @brief Write a glm::quat value
         * @param value glm::quat value
         * @return Result of the operation
         */
        SerializationResult writeQuat(const glm::quat& value);

        /**
         * @brief Write binary data
         * @param data Binary data
         * @param size Size of data in bytes
         * @return Result of the operation
         */
        SerializationResult writeBinary(const void* data, size_t size) override;

        /**
         * @brief Write an entity reference
         * @param entityId Entity ID
         * @return Result of the operation
         */
        SerializationResult writeEntityRef(uint32_t entityId) override;

        /**
         * @brief Write a resource reference
         * @param resourceName Resource name
         * @return Result of the operation
         */
        SerializationResult writeResourceRef(const std::string& resourceName) override;

        /**
         * @brief Set compression level (0-9, 0=none, 9=max)
         * @param level Compression level
         */
        void setCompressionLevel(int level);

        /**
         * @brief Write version information
         * @param version Version info to write
         * @return Result of the operation
         */
        SerializationResult writeVersion(const VersionInfo& version);

    private:
        /**
         * @brief Container type for tracking array state
         */
        struct ArrayState
        {
            size_t size;
            size_t count;
            ValueType elementType;
        };

        /**
         * @brief Write a raw value without field name
         * @param type Value type
         * @param data Data pointer
         * @param size Data size in bytes
         * @return Result of the operation
         */
        SerializationResult writeRawValue(ValueType type, const void* data, size_t size);

        /**
         * @brief Write a header for the current value
         * @param type Value type
         * @param size Size of the value data in bytes
         * @return Result of the operation
         */
        SerializationResult writeValueHeader(ValueType type, uint32_t size);

        /**
         * @brief Get human-readable error for stream error
         * @return Error message
         */
        std::string getStreamErrorMessage() const;

        std::ostream& m_stream;
        std::string m_currentField;
        std::stack<ArrayState> m_arrayStack;
        std::unordered_map<std::string, uint32_t> m_stringCache;
        int m_compressionLevel;
        bool m_headerWritten;
        bool m_fieldNameWritten;
    };

} // namespace PixelCraft::Utility::Serialization