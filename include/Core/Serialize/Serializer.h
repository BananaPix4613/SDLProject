// -------------------------------------------------------------------------
// Serializer.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <iostream>

namespace PixelCraft::Core::Serialize
{

    // Forward declarations
    class DataNode;

    /**
     * @brief Enumeration of supported serialization formats
     */
    enum class SerializationFormat
    {
        JSON,
        Binary,
        XML,
        YAML
    };

    /**
     * @brief Abstract base class that defines the interface for different serialization formats
     *
     * The Serializer class provides a common interface for serializing and deserializing
     * DataNode hierarchies in various formats. It supports file-based and stream-based
     * operations, as well as string conversion for serialized data.
     */
    class Serializer
    {
    public:
        /**
         * @brief Constructor
         * @param format The serialization format to use
         */
        Serializer(SerializationFormat format);

        /**
         * @brief Virtual destructor
         */
        virtual ~Serializer() = default;

        /**
         * @brief Get the serialization format
         * @return The serialization format
         */
        SerializationFormat getFormat() const;

        /**
         * @brief Initialize the serializer
         * @return True if initialization was successful
         */
        virtual bool initialize() = 0;

        /**
         * @brief Clean up resources used by the serializer
         */
        virtual void shutdown() = 0;

        /**
         * @brief Serialize a DataNode into the internal representation
         * @param node The DataNode to serialize
         * @return True if serialization was successful
         */
        virtual bool serialize(const DataNode& node) = 0;

        /**
         * @brief Deserialize from the internal representation into a DataNode
         * @param node The DataNode to populate with deserialized data
         * @return True if deserialization was successful
         */
        virtual bool deserialize(DataNode& node) = 0;

        /**
         * @brief Write the serialized data to a file
         * @param filePath Path to the output file
         * @return True if the write operation was successful
         */
        virtual bool writeToFile(const std::string& filePath) = 0;

        /**
         * @brief Read serialized data from a file
         * @param filePath Path to the input file
         * @return True if the read operation was successful
         */
        virtual bool readFromFile(const std::string& filePath) = 0;

        /**
         * @brief Write the serialized data to an output stream
         * @param stream The output stream to write to
         * @return True if the write operation was successful
         */
        virtual bool writeToStream(std::ostream& stream) = 0;

        /**
         * @brief Read serialized data from an input stream
         * @param stream The input stream to read from
         * @return True if the read operation was successful
         */
        virtual bool readFromStream(std::istream& stream) = 0;

        /**
         * @brief Convert the serialized data to a string representation
         * @return String representation of the serialized data
         */
        virtual std::string toString() = 0;

        /**
         * @brief Parse serialized data from a string
         * @param data The string containing serialized data
         * @return True if the parsing was successful
         */
        virtual bool fromString(const std::string& data) = 0;

        /**
         * @brief Check if an error occurred during the last operation
         * @return True if an error occurred
         */
        virtual bool hasError() const = 0;

        /**
         * @brief Get the error message if an error occurred
         * @return Error message string
         */
        virtual std::string getErrorMessage() const = 0;

        /**
         * @brief Factory method to create a serializer for the specified format
         * @param format The serialization format to use
         * @return A unique pointer to the created serializer instance
         */
        static std::unique_ptr<Serializer> createSerializer(SerializationFormat format);

    protected:
        SerializationFormat m_format; ///< The serialization format used by this serializer
    };

} // namespace PixelCraft::Core::Serialize