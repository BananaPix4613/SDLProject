// -------------------------------------------------------------------------
// DataNode.cpp
// -------------------------------------------------------------------------
#include "Core/Serialize/DataNode.h"

namespace PixelCraft::Core::Serialize
{

    DataNode::DataNode()
        : m_type(Type::Null)
        , m_value(std::monostate{})
    {
    }

    DataNode::DataNode(bool value)
        : m_type(Type::Bool)
        , m_value(value)
    {
    }

    DataNode::DataNode(int value)
        : m_type(Type::Int)
        , m_value(value)
    {
    }

    DataNode::DataNode(float value)
        : m_type(Type::Float)
        , m_value(value)
    {
    }

    DataNode::DataNode(const std::string& value)
        : m_type(Type::String)
        , m_value(value)
    {
    }

    DataNode::DataNode(const char* value)
        : m_type(Type::String)
        , m_value(std::string(value))
    {
    }

    DataNode::DataNode(const DataNode& other)
        : m_type(other.m_type)
        , m_value(other.m_value)
    {
    }

    DataNode::DataNode(DataNode&& other) noexcept
        : m_type(other.m_type)
        , m_value(std::move(other.m_value))
    {
        other.m_type = Type::Null;
        other.m_value = std::monostate{};
    }

    DataNode::~DataNode()
    {
    }

    DataNode& DataNode::operator=(const DataNode& other)
    {
        if (this != &other)
        {
            m_type = other.m_type;
            m_value = other.m_value;
        }
        return *this;
    }

    DataNode& DataNode::operator=(DataNode&& other) noexcept
    {
        if (this != &other)
        {
            m_type = other.m_type;
            m_value = std::move(other.m_value);
            other.m_type = Type::Null;
            other.m_value = std::monostate{};
        }
        return *this;
    }

    DataNode::Type DataNode::getType() const
    {
        return m_type;
    }

    void DataNode::setType(Type type)
    {
        if (m_type == type)
        {
            return;
        }

        switch (type)
        {
            case Type::Null:
                m_value = std::monostate{};
                break;
            case Type::Bool:
                m_value = false;
                break;
            case Type::Int:
                m_value = 0;
                break;
            case Type::Float:
                m_value = 0.0f;
                break;
            case Type::String:
                m_value = std::string();
                break;
            case Type::Array:
                m_value = std::vector<DataNode>();
                break;
            case Type::Object:
                m_value = std::unordered_map<std::string, DataNode>();
                break;
        }

        m_type = type;
    }

    bool DataNode::isNull() const
    {
        return m_type == Type::Null;
    }

    bool DataNode::isBool() const
    {
        return m_type == Type::Bool;
    }

    bool DataNode::isInt() const
    {
        return m_type == Type::Int;
    }

    bool DataNode::isFloat() const
    {
        return m_type == Type::Float;
    }

    bool DataNode::isString() const
    {
        return m_type == Type::String;
    }

    bool DataNode::isArray() const
    {
        return m_type == Type::Array;
    }

    bool DataNode::isObject() const
    {
        return m_type == Type::Object;
    }

    DataNode& DataNode::operator[](size_t index)
    {
        if (!isArray())
        {
            throw std::runtime_error("DataNode is not an array");
        }

        auto& array = std::get<std::vector<DataNode>>(m_value);

        if (index >= array.size())
        {
            throw std::runtime_error("Array index out of range");
        }

        return array[index];
    }

    const DataNode& DataNode::operator[](size_t index) const
    {
        if (!isArray())
        {
            throw std::runtime_error("DataNode is not an array");
        }

        const auto& array = std::get<std::vector<DataNode>>(m_value);

        if (index >= array.size())
        {
            throw std::runtime_error("Array index out of range");
        }

        return array[index];
    }

    DataNode& DataNode::operator[](const std::string& key)
    {
        if (!isObject())
        {
            // Convert to object if null
            if (isNull())
            {
                setType(Type::Object);
            }
            else
            {
                throw std::runtime_error("DataNode is not an object");
            }
        }

        auto& object = std::get<std::unordered_map<std::string, DataNode>>(m_value);
        return object[key];
    }

    const DataNode& DataNode::operator[](const std::string& key) const
    {
        if (!isObject())
        {
            throw std::runtime_error("DataNode is not an object");
        }

        const auto& object = std::get<std::unordered_map<std::string, DataNode>>(m_value);

        auto it = object.find(key);
        if (it == object.end())
        {
            throw std::runtime_error("Object key not found: " + key);
        }

        return it->second;
    }

    void DataNode::addChild(DataNode&& node)
    {
        if (!isArray())
        {
            if (isNull())
            {
                setType(Type::Array);
            }
            else
            {
                throw std::runtime_error("DataNode is not an array");
            }
        }

        auto& array = std::get<std::vector<DataNode>>(m_value);
        array.push_back(std::move(node));
    }

    void DataNode::addChild(const DataNode& node)
    {
        if (!isArray())
        {
            if (isNull())
            {
                setType(Type::Array);
            }
            else
            {
                throw std::runtime_error("DataNode is not an array");
            }
        }

        auto& array = std::get<std::vector<DataNode>>(m_value);
        array.push_back(node);
    }

    void DataNode::addChild(const std::string& key, DataNode&& node)
    {
        if (!isObject())
        {
            if (isNull())
            {
                setType(Type::Object);
            }
            else
            {
                throw std::runtime_error("DataNode is not an object");
            }
        }

        auto& object = std::get<std::unordered_map<std::string, DataNode>>(m_value);
        object[key] = std::move(node);
    }

    void DataNode::addChild(const std::string& key, const DataNode& node)
    {
        if (!isObject())
        {
            if (isNull())
            {
                setType(Type::Object);
            }
            else
            {
                throw std::runtime_error("DataNode is not an object");
            }
        }

        auto& object = std::get<std::unordered_map<std::string, DataNode>>(m_value);
        object[key] = node;
    }

    bool DataNode::removeChild(size_t index)
    {
        if (!isArray())
        {
            throw std::runtime_error("DataNode is not an array");
        }

        auto& array = std::get<std::vector<DataNode>>(m_value);

        if (index >= array.size())
        {
            return false;
        }

        array.erase(array.begin() + index);
        return true;
    }

    bool DataNode::removeChild(const std::string& key)
    {
        if (!isObject())
        {
            throw std::runtime_error("DataNode is not an object");
        }

        auto& object = std::get<std::unordered_map<std::string, DataNode>>(m_value);
        return object.erase(key) > 0;
    }

    bool DataNode::hasKey(const std::string& key) const
    {
        if (!isObject())
        {
            throw std::runtime_error("DataNode is not an object");
        }

        const auto& object = std::get<std::unordered_map<std::string, DataNode>>(m_value);
        return object.find(key) != object.end();
    }

    size_t DataNode::size() const
    {
        if (isArray())
        {
            const auto& array = std::get<std::vector<DataNode>>(m_value);
            return array.size();
        }
        else if (isObject())
        {
            const auto& object = std::get<std::unordered_map<std::string, DataNode>>(m_value);
            return object.size();
        }
        else
        {
            throw std::runtime_error("DataNode is not an array or object");
        }
    }

    const std::vector<DataNode>& DataNode::getArrayElements() const
    {
        if (!isArray())
        {
            throw std::runtime_error("DataNode is not an array");
        }

        return std::get<std::vector<DataNode>>(m_value);
    }

    const std::unordered_map<std::string, DataNode>& DataNode::getObjectElements() const
    {
        if (!isObject())
        {
            throw std::runtime_error("DataNode is not an object");
        }

        return std::get<std::unordered_map<std::string, DataNode>>(m_value);
    }

    std::vector<std::string> DataNode::getKeys() const
    {
        if (!isObject())
        {
            throw std::runtime_error("DataNode is not an object");
        }

        const auto& object = std::get<std::unordered_map<std::string, DataNode>>(m_value);

        std::vector<std::string> keys;
        keys.reserve(object.size());

        for (const auto& [key, _] : object)
        {
            keys.push_back(key);
        }

        return keys;
    }

    void DataNode::clear()
    {
        if (isArray())
        {
            auto& array = std::get<std::vector<DataNode>>(m_value);
            array.clear();
        }
        else if (isObject())
        {
            auto& object = std::get<std::unordered_map<std::string, DataNode>>(m_value);
            object.clear();
        }
        else
        {
            throw std::runtime_error("DataNode is not an array or object");
        }
    }

} // namespace PixelCraft::Core