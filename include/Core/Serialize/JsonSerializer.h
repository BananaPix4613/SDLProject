// -------------------------------------------------------------------------
// JsonSerializer.h
// -------------------------------------------------------------------------
#pragma once

#include "Core/Serialize/Serializer.h"

namespace PixelCraft::Core::Serialize
{

    /**
     * @brief Implementation of the Serializer interface for JSON format
     *
     * The JsonSerializer class provides serialization and deserialization of
     * DataNode hierarchies to and from JSON format. It uses a third-party
     * JSON library (nlohmann::json) for the underlying JSON operations.
     */
    class JsonSerializer : public Serializer
    {
    public:
        /**
         * @brief Constructor
         */
        JsonSerializer();

        /**
         * @brief Destructor
         */
        ~JsonSerializer() override;

        /**
         * @brief Initialize the JSON serializer
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Clean up resources used by the JSON serializer
         */
        void shutdown() override;

        /**
         * @brief Serialize a DataNode into JSON
         * @param node The DataNode to serialize
         * @return True if serialization was successful
         */
        bool serialize(const DataNode& node) override;

        /**
         * @brief Deserialize JSON into a DataNode
         * @param node The DataNode to populate with deserialized data
         * @return True if deserialization was successful
         */
        bool deserialize(DataNode& node) override;

        /**
         * @brief Write the JSON data to a file
         * @param filePath Path to the output file
         * @return True if the write operation was successful
         */
        bool writeToFile(const std::string& filePath) override;

        /**
         * @brief Read JSON data from a file
         * @param filePath Path to the input file
         * @return True if the read operation was successful
         */
        bool readFromFile(const std::string& filePath) override;

        /**
         * @brief Write the JSON data to an output stream
         * @param stream The output stream to write to
         * @return True if the write operation was successful
         */
        bool writeToStream(std::ostream& stream) override;

        /**
         * @brief Read JSON data from an input stream
         * @param stream The input stream to read from
         * @return True if the read operation was successful
         */
        bool readFromStream(std::istream& stream) override;

        /**
         * @brief Convert the JSON data to a string representation
         * @return String representation of the JSON data
         */
        std::string toString() override;

        /**
         * @brief Parse JSON data from a string
         * @param data The string containing JSON data
         * @return True if the parsing was successful
         */
        bool fromString(const std::string& data) override;

        /**
         * @brief Check if an error occurred during the last JSON operation
         * @return True if an error occurred
         */
        bool hasError() const override;

        /**
         * @brief Get the error message if an error occurred
         * @return Error message string
         */
        std::string getErrorMessage() const override;

        /**
         * @brief Enable or disable pretty-printing of JSON output
         * @param enabled True to enable pretty-printing, false to disable
         */
        void setPrettyPrint(bool enabled);

        /**
         * @brief Check if pretty-printing is enabled
         * @return True if pretty-printing is enabled
         */
        bool isPrettyPrintEnabled() const;

        /**
         * @brief Set the indentation level for pretty-printing
         * @param level Number of spaces for each indentation level
         */
        void setIndentLevel(int level);

        /**
         * @brief Get the current indentation level
         * @return Number of spaces for each indentation level
         */
        int getIndentLevel() const;

    private:
        // Internal JSON representation (using nlohmann::json)
        void* m_jsonDocument;

        // JSON formatting options
        bool m_prettyPrint;
        int m_indentLevel;

        // Error handling
        std::string m_errorMessage;

        /**
         * @brief Helper method to serialize a DataNode into a JSON value
         * @param node The DataNode to serialize
         * @param jsonValue Pointer to the JSON value to populate
         */
        void serializeNode(const DataNode& node, void* jsonValue);

        /**
         * @brief Helper method to deserialize a JSON value into a DataNode
         * @param node The DataNode to populate
         * @param jsonValue Pointer to the JSON value to read from
         */
        void deserializeNode(DataNode& node, const void* jsonValue);
    };

} // namespace PixelCraft::Core::Serialize