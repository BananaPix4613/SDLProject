// -------------------------------------------------------------------------
// BinarySerializer.h
// -------------------------------------------------------------------------
#pragma once

#include "Core/Serialize/Serializer.h"
#include <vector>
#include <cstdint>

namespace PixelCraft::Core::Serialize
{

    /**
     * @brief Implementation of the Serializer interface for binary format
     *
     * The BinarySerializer class provides serialization and deserialization of
     * DataNode hierarchies to and from a binary format. It includes support for
     * compression and version information for compatibility.
     */
    class BinarySerializer : public Serializer
    {
    public:
        /**
         * @brief Constructor
         */
        BinarySerializer();

        /**
         * @brief Destructor
         */
        ~BinarySerializer() override;

        /**
         * @brief Initialize the binary serializer
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Clean up resources used by the binary serializer
         */
        void shutdown() override;

        /**
         * @brief Serialize a DataNode into binary format
         * @param node The DataNode to serialize
         * @return True if serialization was successful
         */
        bool serialize(const DataNode& node) override;

        /**
         * @brief Deserialize binary data into a DataNode
         * @param node The DataNode to populate with deserialized data
         * @return True if deserialization was successful
         */
        bool deserialize(DataNode& node) override;

        /**
         * @brief Write the binary data to a file
         * @param filePath Path to the output file
         * @return True if the write operation was successful
         */
        bool writeToFile(const std::string& filePath) override;

        /**
         * @brief Read binary data from a file
         * @param filePath Path to the input file
         * @return True if the read operation was successful
         */
        bool readFromFile(const std::string& filePath) override;

        /**
         * @brief Write the binary data to an output stream
         * @param stream The output stream to write to
         * @return True if the write operation was successful
         */
        bool writeToStream(std::ostream& stream) override;

        /**
         * @brief Read binary data from an input stream
         * @param stream The input stream to read from
         * @return True if the read operation was successful
         */
        bool readFromStream(std::istream& stream) override;

        /**
         * @brief Convert the binary data to a string representation (Base64 encoding)
         * @return String representation of the binary data
         */
        std::string toString() override;

        /**
         * @brief Parse binary data from a string (Base64 encoding)
         * @param data The string containing encoded binary data
         * @return True if the parsing was successful
         */
        bool fromString(const std::string& data) override;

        /**
         * @brief Check if an error occurred during the last binary operation
         * @return True if an error occurred
         */
        bool hasError() const override;

        /**
         * @brief Get the error message if an error occurred
         * @return Error message string
         */
        std::string getErrorMessage() const override;

        /**
         * @brief Enable or disable compression of binary data
         * @param enabled True to enable compression, false to disable
         */
        void setCompression(bool enabled);

        /**
         * @brief Check if compression is enabled
         * @return True if compression is enabled
         */
        bool isCompressionEnabled() const;

        /**
         * @brief Set the binary format version
         * @param version Version number for compatibility
         */
        void setVersion(uint32_t version);

        /**
         * @brief Get the binary format version
         * @return Current version number
         */
        uint32_t getVersion() const;

    private:
        // Binary data buffer
        std::vector<uint8_t> m_buffer;

        // Read position for deserialization
        size_t m_readPosition;

        // Binary format options
        bool m_compression;
        uint32_t m_version;

        // Error handling
        std::string m_errorMessage;

        /**
         * @brief Helper method to serialize a DataNode into binary data
         * @param node The DataNode to serialize
         */
        void serializeNode(const DataNode& node);

        /**
         * @brief Helper method to deserialize binary data into a DataNode
         * @param node The DataNode to populate
         */
        void deserializeNode(DataNode& node);

        /**
         * @brief Write a value of type T to the binary buffer
         * @tparam T The type of value to write
         * @param value The value to write
         */
        template<typename T>
        void write(const T& value)
        {
            size_t size = sizeof(T);
            size_t currentSize = m_buffer.size();
            m_buffer.resize(currentSize + size);
            std::memcpy(m_buffer.data() + currentSize, &value, size);
        }

        /**
         * @brief Read a value of type T from the binary buffer
         * @tparam T The type of value to read
         * @return The read value
         */
        template<typename T>
        T read()
        {
            T value;
            if (m_readPosition + sizeof(T) <= m_buffer.size())
            {
                std::memcpy(&value, m_buffer.data() + m_readPosition, sizeof(T));
                m_readPosition += sizeof(T);
            }
            else
            {
                m_errorMessage = "Buffer read position out of bounds";
                error(m_errorMessage);
                // Return a zero-initialized value
                std::memset(&value, 0, sizeof(T));
            }
            return value;
        }

        /**
         * @brief Write a string to the binary buffer
         * @param str The string to write
         */
        void writeString(const std::string& str);

        /**
         * @brief Read a string from the binary buffer
         * @return The read string
         */
        std::string readString();

        /**
         * @brief Compress the binary data if compression is enabled
         */
        void compressBuffer();

        /**
         * @brief Decompress the binary data if compression is enabled
         * @return True if decompression was successful
         */
        bool decompressBuffer();
    };

} // namespace PixelCraft::Core::Serialize