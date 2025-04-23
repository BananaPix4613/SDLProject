// -------------------------------------------------------------------------
// DataNode.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <stdexcept>

namespace PixelCraft::Core::Serialize
{

    /**
     * @brief A flexible data container for hierarchical data representation
     *
     * DataNode is a versatile container class that can store various data types
     * and form hierarchical structures. It is used as the primary data structure
     * for serialization and configuration in the engine.
     */
    class DataNode
    {
    public:
        /**
         * @brief Enumeration of supported data types
         */
        enum class Type
        {
            Null,    ///< Null value
            Bool,    ///< Boolean value
            Int,     ///< Integer value
            Float,   ///< Floating-point value
            String,  ///< String value
            Array,   ///< Ordered collection of DataNodes
            Object   ///< Key-value collection of DataNodes
        };

        /**
         * @brief Default constructor (creates a null node)
         */
        DataNode();

        /**
         * @brief Constructor for boolean values
         * @param value Boolean value
         */
        explicit DataNode(bool value);

        /**
         * @brief Constructor for integer values
         * @param value Integer value
         */
        explicit DataNode(int value);

        /**
         * @brief Constructor for floating-point values
         * @param value Floating-point value
         */
        explicit DataNode(float value);

        /**
         * @brief Constructor for string values
         * @param value String value
         */
        explicit DataNode(const std::string& value);

        /**
         * @brief Constructor for string literals
         * @param value String literal
         */
        explicit DataNode(const char* value);

        /**
         * @brief Copy constructor
         * @param other DataNode to copy
         */
        DataNode(const DataNode& other);

        /**
         * @brief Move constructor
         * @param other DataNode to move
         */
        DataNode(DataNode&& other) noexcept;

        /**
         * @brief Destructor
         */
        ~DataNode();

        /**
         * @brief Copy assignment operator
         * @param other DataNode to copy
         * @return Reference to this DataNode
         */
        DataNode& operator=(const DataNode& other);

        /**
         * @brief Move assignment operator
         * @param other DataNode to move
         * @return Reference to this DataNode
         */
        DataNode& operator=(DataNode&& other) noexcept;

        /**
         * @brief Get the type of this node
         * @return Node type
         */
        Type getType() const;

        /**
         * @brief Set the type of this node
         * @param type The type to set
         */
        void setType(Type type);

        /**
         * @brief Check if this node is null
         * @return True if the node is null
         */
        bool isNull() const;

        /**
         * @brief Check if this node is a boolean
         * @return True if the node is a boolean
         */
        bool isBool() const;

        /**
         * @brief Check if this node is an integer
         * @return True if the node is an integer
         */
        bool isInt() const;

        /**
         * @brief Check if this node is a floating-point number
         * @return True if the node is a floating-point number
         */
        bool isFloat() const;

        /**
         * @brief Check if this node is a string
         * @return True if the node is a string
         */
        bool isString() const;

        /**
         * @brief Check if this node is an array
         * @return True if the node is an array
         */
        bool isArray() const;

        /**
         * @brief Check if this node is an object
         * @return True if the node is an object
         */
        bool isObject() const;

        /**
         * @brief Get the value of this node as a specific type
         * @tparam T The type to retrieve
         * @return The value as type T
         * @throws std::runtime_error if the node is not of the requested type
         */
        template<typename T>
        T getValue() const
        {
            if constexpr (std::is_same_v<T, bool>)
            {
                if (!isBool())
                {
                    throw std::runtime_error("DataNode is not a boolean");
                }
                return std::get<bool>(m_value);
            }
            else if constexpr (std::is_same_v<T, int>)
            {
                if (!isInt())
                {
                    throw std::runtime_error("DataNode is not an integer");
                }
                return std::get<int>(m_value);
            }
            else if constexpr (std::is_same_v<T, float>)
            {
                if (!isFloat())
                {
                    throw std::runtime_error("DataNode is not a float");
                }
                return std::get<float>(m_value);
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                if (!isString())
                {
                    throw std::runtime_error("DataNode is not a string");
                }
                return std::get<std::string>(m_value);
            }
            else
            {
                static_assert(std::is_same_v<T, bool> ||
                              std::is_same_v<T, int> ||
                              std::is_same_v<T, float> ||
                              std::is_same_v<T, std::string>,
                              "Unsupported type for DataNode::getValue()");
                return T();
            }
        }

        /**
         * @brief Set the value of this node
         * @tparam T The type of value to set
         * @param value The value to set
         */
        template<typename T>
        void setValue(const T& value)
        {
            if constexpr (std::is_same_v<T, bool>)
            {
                m_type = Type::Bool;
                m_value = value;
            }
            else if constexpr (std::is_same_v<T, int>)
            {
                m_type = Type::Int;
                m_value = value;
            }
            else if constexpr (std::is_same_v<T, float>)
            {
                m_type = Type::Float;
                m_value = value;
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                m_type = Type::String;
                m_value = value;
            }
            else if constexpr (std::is_same_v<T, const char*>)
            {
                m_type = Type::String;
                m_value = std::string(value);
            }
            else
            {
                static_assert(std::is_same_v<T, bool> ||
                              std::is_same_v<T, int> ||
                              std::is_same_v<T, float> ||
                              std::is_same_v<T, std::string> ||
                              std::is_same_v<T, const char*>,
                              "Unsupported type for DataNode::setValue()");
            }
        }

        /**
         * @brief Get an array element by index
         * @param index Index of the element to retrieve
         * @return Reference to the element
         * @throws std::runtime_error if the node is not an array or index is out of range
         */
        DataNode& operator[](size_t index);

        /**
         * @brief Get an array element by index (const version)
         * @param index Index of the element to retrieve
         * @return Const reference to the element
         * @throws std::runtime_error if the node is not an array or index is out of range
         */
        const DataNode& operator[](size_t index) const;

        /**
         * @brief Get an object element by key
         * @param key Key of the element to retrieve
         * @return Reference to the element
         * @throws std::runtime_error if the node is not an object
         */
        DataNode& operator[](const std::string& key);

        /**
         * @brief Get an object element by key (const version)
         * @param key Key of the element to retrieve
         * @return Const reference to the element
         * @throws std::runtime_error if the node is not an object or key not found
         */
        const DataNode& operator[](const std::string& key) const;

        /**
         * @brief Add a child node to an array
         * @param node Node to add
         * @throws std::runtime_error if the node is not an array
         */
        void addChild(DataNode&& node);

        /**
         * @brief Add a child node to an array
         * @param node Node to add
         * @throws std::runtime_error if the node is not an array
         */
        void addChild(const DataNode& node);

        /**
         * @brief Add a child node to an object with the specified key
         * @param key Key for the child node
         * @param node Node to add
         * @throws std::runtime_error if the node is not an object
         */
        void addChild(const std::string& key, DataNode&& node);

        /**
         * @brief Add a child node to an object with the specified key
         * @param key Key for the child node
         * @param node Node to add
         * @throws std::runtime_error if the node is not an object
         */
        void addChild(const std::string& key, const DataNode& node);

        /**
         * @brief Remove a child node from an array
         * @param index Index of the node to remove
         * @return True if the node was removed
         * @throws std::runtime_error if the node is not an array
         */
        bool removeChild(size_t index);

        /**
         * @brief Remove a child node from an object
         * @param key Key of the node to remove
         * @return True if the node was removed
         * @throws std::runtime_error if the node is not an object
         */
        bool removeChild(const std::string& key);

        /**
         * @brief Check if an object contains a key
         * @param key Key to check
         * @return True if the key exists
         * @throws std::runtime_error if the node is not an object
         */
        bool hasKey(const std::string& key) const;

        /**
         * @brief Get the size of an array or object
         * @return Number of children
         * @throws std::runtime_error if the node is not an array or object
         */
        size_t size() const;

        /**
         * @brief Get all array elements
         * @return Vector of array elements
         * @throws std::runtime_error if the node is not an array
         */
        const std::vector<DataNode>& getArrayElements() const;

        /**
         * @brief Get all object elements
         * @return Map of object elements
         * @throws std::runtime_error if the node is not an object
         */
        const std::unordered_map<std::string, DataNode>& getObjectElements() const;

        /**
         * @brief Get all keys in an object
         * @return Vector of keys
         * @throws std::runtime_error if the node is not an object
         */
        std::vector<std::string> getKeys() const;

        /**
         * @brief Clear all array or object elements
         * @throws std::runtime_error if the node is not an array or object
         */
        void clear();

    private:
        Type m_type;                                              ///< The type of this node
        std::variant<std::monostate,                              ///< Null value
            bool,                                         ///< Boolean value
            int,                                          ///< Integer value
            float,                                        ///< Floating-point value
            std::string,                                  ///< String value
            std::vector<DataNode>,                        ///< Array value
            std::unordered_map<std::string, DataNode>     ///< Object value
        > m_value;                                    ///< The value of this node
    };

} // namespace PixelCraft::Core::Serialize