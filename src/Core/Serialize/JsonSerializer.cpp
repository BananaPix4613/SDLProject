// -------------------------------------------------------------------------
// JsonSerializer.cpp
// -------------------------------------------------------------------------
#include "Core/Serialize/JsonSerializer.h"
#include "Core/Serialize/DataNode.h"
#include "Core/Logger.h"
#include "Utility/FileSystem.h"

// Include nlohmann/json header library
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace PixelCraft::Core::Serialize
{

    JsonSerializer::JsonSerializer()
        : Serializer(SerializationFormat::JSON)
        , m_jsonDocument(nullptr)
        , m_prettyPrint(true)
        , m_indentLevel(4)
    {
    }

    JsonSerializer::~JsonSerializer()
    {
        shutdown();
    }

    bool JsonSerializer::initialize()
    {
        try
        {
            // Create a new JSON document
            m_jsonDocument = new json();
            return true;
        }
        catch (const std::exception& e)
        {
            m_errorMessage = "Failed to initialize JSON serializer: " + std::string(e.what());
            error(m_errorMessage);
            return false;
        }
    }

    void JsonSerializer::shutdown()
    {
        if (m_jsonDocument)
        {
            delete static_cast<json*>(m_jsonDocument);
            m_jsonDocument = nullptr;
        }
    }

    bool JsonSerializer::serialize(const DataNode& node)
    {
        try
        {
            if (!m_jsonDocument)
            {
                m_errorMessage = "JSON serializer not initialized";
                error(m_errorMessage);
                return false;
            }

            json* root = static_cast<json*>(m_jsonDocument);
            *root = json::object(); // Reset the document

            // Serialize the root node into the JSON document
            serializeNode(node, root);
            return true;
        }
        catch (const std::exception& e)
        {
            m_errorMessage = "Failed to serialize node: " + std::string(e.what());
            error(m_errorMessage);
            return false;
        }
    }

    bool JsonSerializer::deserialize(DataNode& node)
    {
        try
        {
            if (!m_jsonDocument)
            {
                m_errorMessage = "JSON serializer not initialized";
                error(m_errorMessage);
                return false;
            }

            json* root = static_cast<json*>(m_jsonDocument);

            // Deserialize the JSON document into the root node
            deserializeNode(node, root);
            return true;
        }
        catch (const std::exception& e)
        {
            m_errorMessage = "Failed to deserialize data: " + std::string(e.what());
            error(m_errorMessage);
            return false;
        }
    }

    bool JsonSerializer::writeToFile(const std::string& filePath)
    {
        try
        {
            if (!m_jsonDocument)
            {
                m_errorMessage = "JSON serializer not initialized";
                error(m_errorMessage);
                return false;
            }

            json* root = static_cast<json*>(m_jsonDocument);

            // Create parent directories if they don't exist
            fs::path path(filePath);
            if (!fs::exists(path.parent_path()))
            {
                fs::create_directories(path.parent_path());
            }

            // Open file for writing
            std::ofstream file(filePath);
            if (!file.is_open())
            {
                m_errorMessage = "Failed to open file for writing: " + filePath;
                error(m_errorMessage);
                return false;
            }

            // Write to file with appropriate formatting
            if (m_prettyPrint)
            {
                file << root->dump(m_indentLevel);
            }
            else
            {
                file << root->dump();
            }

            file.close();
            return true;
        }
        catch (const std::exception& e)
        {
            m_errorMessage = "Failed to write to file: " + std::string(e.what());
            error(m_errorMessage);
            return false;
        }
    }

    bool JsonSerializer::readFromFile(const std::string& filePath)
    {
        try
        {
            if (!m_jsonDocument)
            {
                m_errorMessage = "JSON serializer not initialized";
                error(m_errorMessage);
                return false;
            }

            // Check if file exists
            if (!fs::exists(filePath))
            {
                m_errorMessage = "File does not exist: " + filePath;
                error(m_errorMessage);
                return false;
            }

            // Open file for reading
            std::ifstream file(filePath);
            if (!file.is_open())
            {
                m_errorMessage = "Failed to open file for reading: " + filePath;
                error(m_errorMessage);
                return false;
            }

            json* root = static_cast<json*>(m_jsonDocument);

            // Parse JSON from file
            try
            {
                file >> *root;
            }
            catch (const json::parse_error& e)
            {
                m_errorMessage = "JSON parse error: " + std::string(e.what());
                error(m_errorMessage);
                return false;
            }

            file.close();
            return true;
        }
        catch (const std::exception& e)
        {
            m_errorMessage = "Failed to read from file: " + std::string(e.what());
            error(m_errorMessage);
            return false;
        }
    }

    bool JsonSerializer::writeToStream(std::ostream& stream)
    {
        try
        {
            if (!m_jsonDocument)
            {
                m_errorMessage = "JSON serializer not initialized";
                error(m_errorMessage);
                return false;
            }

            json* root = static_cast<json*>(m_jsonDocument);

            // Write to stream with appropriate formatting
            if (m_prettyPrint)
            {
                stream << root->dump(m_indentLevel);
            }
            else
            {
                stream << root->dump();
            }

            return true;
        }
        catch (const std::exception& e)
        {
            m_errorMessage = "Failed to write to stream: " + std::string(e.what());
            error(m_errorMessage);
            return false;
        }
    }

    bool JsonSerializer::readFromStream(std::istream& stream)
    {
        try
        {
            if (!m_jsonDocument)
            {
                m_errorMessage = "JSON serializer not initialized";
                error(m_errorMessage);
                return false;
            }

            json* root = static_cast<json*>(m_jsonDocument);

            // Parse JSON from stream
            try
            {
                stream >> *root;
            }
            catch (const json::parse_error& e)
            {
                m_errorMessage = "JSON parse error: " + std::string(e.what());
                error(m_errorMessage);
                return false;
            }

            return true;
        }
        catch (const std::exception& e)
        {
            m_errorMessage = "Failed to read from stream: " + std::string(e.what());
            error(m_errorMessage);
            return false;
        }
    }

    std::string JsonSerializer::toString()
    {
        try
        {
            if (!m_jsonDocument)
            {
                m_errorMessage = "JSON serializer not initialized";
                error(m_errorMessage);
                return "";
            }

            json* root = static_cast<json*>(m_jsonDocument);

            // Convert JSON to string with appropriate formatting
            if (m_prettyPrint)
            {
                return root->dump(m_indentLevel);
            }
            else
            {
                return root->dump();
            }
        }
        catch (const std::exception& e)
        {
            m_errorMessage = "Failed to convert to string: " + std::string(e.what());
            error(m_errorMessage);
            return "";
        }
    }

    bool JsonSerializer::fromString(const std::string& data)
    {
        try
        {
            if (!m_jsonDocument)
            {
                m_errorMessage = "JSON serializer not initialized";
                error(m_errorMessage);
                return false;
            }

            json* root = static_cast<json*>(m_jsonDocument);

            // Parse JSON from string
            try
            {
                *root = json::parse(data);
            }
            catch (const json::parse_error& e)
            {
                m_errorMessage = "JSON parse error: " + std::string(e.what());
                error(m_errorMessage);
                return false;
            }

            return true;
        }
        catch (const std::exception& e)
        {
            m_errorMessage = "Failed to parse from string: " + std::string(e.what());
            error(m_errorMessage);
            return false;
        }
    }

    bool JsonSerializer::hasError() const
    {
        return !m_errorMessage.empty();
    }

    std::string JsonSerializer::getErrorMessage() const
    {
        return m_errorMessage;
    }

    void JsonSerializer::setPrettyPrint(bool enabled)
    {
        m_prettyPrint = enabled;
    }

    bool JsonSerializer::isPrettyPrintEnabled() const
    {
        return m_prettyPrint;
    }

    void JsonSerializer::setIndentLevel(int level)
    {
        m_indentLevel = level;
    }

    int JsonSerializer::getIndentLevel() const
    {
        return m_indentLevel;
    }

    void JsonSerializer::serializeNode(const DataNode& node, void* jsonValue)
    {
        json* jsonObj = static_cast<json*>(jsonValue);

        switch (node.getType())
        {
            case DataNode::Type::Object:
            {
                *jsonObj = json::object();
                const auto& elements = node.getObjectElements();
                for (const auto& [key, childNode] : elements)
                {
                    (*jsonObj)[key] = json();
                    serializeNode(childNode, &(*jsonObj)[key]);
                }
                break;
            }
            case DataNode::Type::Array:
            {
                *jsonObj = json::array();
                const auto& elements = node.getArrayElements();
                for (size_t i = 0; i < elements.size(); ++i)
                {
                    jsonObj->push_back(json());
                    serializeNode(elements[i], &(*jsonObj)[i]);
                }
                break;
            }
            case DataNode::Type::String:
                *jsonObj = node.getValue<std::string>();
                break;
            case DataNode::Type::Int:
                *jsonObj = node.getValue<int>();
                break;
            case DataNode::Type::Float:
                *jsonObj = node.getValue<float>();
                break;
            case DataNode::Type::Bool:
                *jsonObj = node.getValue<bool>();
                break;
            case DataNode::Type::Null:
                *jsonObj = nullptr;
                break;
            default:
                // Unsupported type
                warn("Unsupported DataNode type encountered during JSON serialization");
                *jsonObj = nullptr;
                break;
        }
    }

    void JsonSerializer::deserializeNode(DataNode& node, const void* jsonValue)
    {
        const json* jsonObj = static_cast<const json*>(jsonValue);

        if (jsonObj->is_object())
        {
            node.setType(DataNode::Type::Object);
            for (auto it = jsonObj->begin(); it != jsonObj->end(); ++it)
            {
                DataNode childNode;
                deserializeNode(childNode, &it.value());
                node.addChild(it.key(), std::move(childNode));
            }
        }
        else if (jsonObj->is_array())
        {
            node.setType(DataNode::Type::Array);
            for (size_t i = 0; i < jsonObj->size(); i++)
            {
                DataNode childNode;
                deserializeNode(childNode, &(*jsonObj)[i]);
                node.addChild(std::move(childNode));
            }
        }
        else if (jsonObj->is_string())
        {
            node.setValue(jsonObj->get<std::string>());
        }
        else if (jsonObj->is_number_integer())
        {
            node.setValue(jsonObj->get<int>());
        }
        else if (jsonObj->is_number_float())
        {
            node.setValue(jsonObj->get<float>());
        }
        else if (jsonObj->is_boolean())
        {
            node.setValue(jsonObj->get<bool>());
        }
        else if (jsonObj->is_null())
        {
            node.setType(DataNode::Type::Null);
        }
        else
        {
            // Unsupported type
            warn("Unsupported JSON type encountered during deserialization");
            node.setType(DataNode::Type::Null);
        }
    }

} // namespace PixelCraft::Core::Serialize