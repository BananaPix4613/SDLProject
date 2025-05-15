// -------------------------------------------------------------------------
// Utility/Serialization/BinaryDeserializer.h
// -------------------------------------------------------------------------
#pragma once

#include "Utility/Serialization/Deserializer.h"

#include <istream>
#include <stack>
#include <unordered_map>

namespace PixelCraft::Utility::Serialization
{

    /**
     * @brief Binary format deserializer
     */
    class BinaryDeserializer : public Deserializer
    {
    public:
        /**
         * @brief Constructor with input stream
         * @param stream Input stream for serialized data
         */
        explicit BinaryDeserializer(std::istream& stream);

        /**
         * @brief Destructor
         */
        ~BinaryDeserializer() override;

        /**
         * @brief Begin reading an object
         * @param name Expected name of the object
         * @param schema Optional schema for the object
         * @return Result of the operation
         */
        SerializationResult beginObject(const std::string& name, const Schema* schema = nullptr) override;

        /**
         * @brief End reading an object
         * @return Result of the operation
         */
        SerializationResult endObject() override;

        /**
         * @brief Begin reading an array
         * @param name Expected name of the array
         * @param size Output parameter for array size
         * @return Result of the operation
         */
        SerializationResult beginArray(const std::string& name, size_t& size) override;

        /**
         * @brief End reading an array
         * @return Result of the operation
         */
        SerializationResult endArray() override;

        /**
         * @brief Check if the current field is null
         * @return True if the field is null
         */
        bool isNull() override;

        /**
         * @brief Get the type of the current value
         * @return Value type
         */
        ValueType getValueType() override;

        /**
         * @brief Read a field name
         * @param name Output parameter for field name
         * @return Result of the operation
         */
        SerializationResult readFieldName(std::string& name) override;

        /**
         * @brief Read a boolean value
         * @param value Output parameter for boolean value
         * @return Result of the operation
         */
        SerializationResult readBool(bool& value) override;

        /**
         * @brief Read an integer value
         * @param value Output parameter for integer value
         * @return Result of the operation
         */
        SerializationResult readInt(int64_t& value) override;

        /**
         * @brief Read an unsigned integer value
         * @param value Output parameter for unsigned integer value
         * @return Result of the operation
         */
        SerializationResult readUInt(uint64_t& value) override;

        /**
         * @brief Read a float value
         * @param value Output parameter for float value
         * @return Result of the operation
         */
        SerializationResult readFloat(float& value) override;

        /**
         * @brief Read a double value
         * @param value Output parameter for double value
         * @return Result of the operation
         */
        SerializationResult readDouble(double& value) override;

        /**
         * @brief Read a string value
         * @param value Output parameter for string value
         * @return Result of the operation
         */
        SerializationResult readString(std::string& value) override;

        /**
         * @brief Read binary data
         * @param data Output buffer for binary data
         * @param size Size of the buffer in bytes
         * @param actualSize Output parameter for actual size read
         * @return Result of the operation
         */
        SerializationResult readBinary(void* data, size_t size, size_t& actualSize) override;

        /**
         * @brief Read an entity reference
         * @param entityId Output parameter for entity ID
         * @return Result of the operation
         */
        SerializationResult readEntityRef(uint64_t& entityId) override;

        /**
         * @brief Read a resource reference
         * @param resourceName Output parameter for resource name
         * @return Result of the operation
         */
        SerializationResult readResourceRef(std::string& resourceName) override;

        /**
         * @brief Find a field by name
         * @param name Field name to find
         * @return True if the field was found
         */
        bool findField(const std::string& name) override;

        /**
         * @brief Check if a field exists
         * @param name Field name to check
         * @return True if the field exists
         */
        bool hasField(const std::string& name) override;

        /**
         * @brief Skip the current value
         * @return Result of the operation
         */
        SerializationResult skipValue() override;

        /**
         * @brief Get the version info from the current stream
         * @return Version info
         */
        VersionInfo getVersion() const override;

    private:
        /**
         * @brief Structure for value header
         */
        struct ValueHeader
        {
            ValueType type;
            uint32_t size;
        };

        /**
         * @brief Container type for tracking array state
         */
        struct ArrayState
        {
            size_t size;
            size_t count;
            std::istream::pos_type startPos;
        };

        /**
         * @brief Container type for tracking object state
         */
        struct ObjectState
        {
            std::unordered_map<std::string, std::istream::pos_type> fields;
            std::istream::pos_type endPos;
        };

        /**
         * @brief Read a value header
         * @param header Output parameter for header
         * @return Result of the operation
         */
        SerializationResult readValueHeader(ValueHeader& header);

        /**
         * @brief Read raw data from the stream
         * @param data Output buffer
         * @param size Size to read in bytes
         * @return Result of the operation
         */
        SerializationResult readRawData(void* data, size_t size);

        /**
         * @brief Get human-readable error for stream error
         * @return Error message
         */
        std::string getStreamErrorMessage() const;

        /**
         * @brief Verify the current value is of the expected type
         * @param expectedType Expected type
         * @return Result of the operation
         */
        SerializationResult verifyValueType(ValueType expectedType);

        std::istream& m_stream;
        std::stack<ObjectState> m_objectStack;
        std::vector<std::string> m_objectNameStack;
        std::stack<ArrayState> m_arrayStack;
        ValueHeader m_currentHeader;
        std::string m_currentField;
        bool m_headerRead;
        VersionInfo m_version;
        std::unordered_map<uint32_t, std::string> m_stringCache;
    };

} // namespace PixelCraft::Utility::Serialization