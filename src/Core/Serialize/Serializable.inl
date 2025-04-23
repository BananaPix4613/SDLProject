// -------------------------------------------------------------------------
// Serializable.inl
// -------------------------------------------------------------------------
#pragma once

namespace PixelCraft::Core::Serialize
{

    // Helper function to check if a type is a basic type that can be directly serialized
    template<typename T>
    constexpr bool isBasicType()
    {
        return std::is_arithmetic_v<T> || std::is_same_v<T, std::string>;
    }

    template<typename T>
    void SerializationHelper<T>::serializeVector(DataNode& node, const std::string& name, const std::vector<T>& vec)
    {
        auto& arrayNode = node.createChild(name);
        arrayNode.setType(DataNode::Type::Array);

        for (const auto& item : vec)
        {
            auto& itemNode = arrayNode.createChild();

            // Handle different types
            if constexpr (isBasicType<T>())
            {
                // Basic type that can be directly serialized
                itemNode.setValue(item);
            }
            else if constexpr (std::is_base_of_v<Serializable, T>)
            {
                // Serializable object
                item.serialize(itemNode);
            }
            else if constexpr (is_shared_ptr_v<T> && std::is_base_of_v<Serializable, typename T::element_type>)
            {
                // Shared pointer to Serializable
                if (item)
                {
                    item->serialize(itemNode);
                    // Store type information for deserialization
                    itemNode.setAttribute("type", typeid(*item).name());
                }
                else
                {
                    // Null pointer
                    itemNode.setAttribute("null", true);
                }
            }
            else if constexpr (has_serialize_method_v<T>)
            {
                // Custom type with serialize method
                item.serialize(itemNode);
            }
            else
            {
                // Error: unsupported type
                static_assert(isBasicType<T>() || std::is_base_of_v<Serializable, T> ||
                              (is_shared_ptr_v<T> && std::is_base_of_v<Serializable, typename T::element_type>) ||
                              has_serialize_method_v<T>,
                              "Type must be serializable");
            }
        }
    }

    template<typename T>
    void SerializationHelper<T>::deserializeVector(const DataNode& node, const std::string& name, std::vector<T>& vec)
    {
        const DataNode* arrayNode = node.findChild(name);
        if (!arrayNode || arrayNode->getType() != DataNode::Type::Array)
        {
            Logger::warn("Failed to deserialize vector '" + name + "': array node not found or invalid type");
            return;
        }

        vec.clear();
        vec.reserve(arrayNode->getChildCount());

        for (size_t i = 0; i < arrayNode->getChildCount(); ++i)
        {
            const DataNode& itemNode = arrayNode->getChild(i);

            // Handle different types
            if constexpr (isBasicType<T>())
            {
                // Basic type that can be directly deserialized
                T value;
                if (itemNode.getValue(value))
                {
                    vec.push_back(value);
                }
            }
            else if constexpr (std::is_base_of_v<Serializable, T>)
            {
                // Serializable object
                T item;
                item.deserialize(itemNode);
                vec.push_back(item);
            }
            else if constexpr (is_shared_ptr_v<T> && std::is_base_of_v<Serializable, typename T::element_type>)
            {
                // Shared pointer to Serializable
                bool isNull = false;
                if (itemNode.getAttribute("null", isNull) && isNull)
                {
                    // Null pointer
                    vec.push_back(nullptr);
                }
                else
                {
                    std::string typeName;
                    if (itemNode.getAttribute("type", typeName))
                    {
                        // Create instance of the correct type
                        auto instance = SerializationTypeRegistry::getInstance().createInstance(typeName);
                        if (instance)
                        {
                            instance->deserialize(itemNode);
                            vec.push_back(std::static_pointer_cast<typename T::element_type>(instance));
                        }
                        else
                        {
                            // Fallback to default type if the registered type can't be created
                            auto defaultInstance = std::make_shared<typename T::element_type>();
                            defaultInstance->deserialize(itemNode);
                            vec.push_back(defaultInstance);
                        }
                    }
                    else
                    {
                        // No type information, create default type
                        auto instance = std::make_shared<typename T::element_type>();
                        instance->deserialize(itemNode);
                        vec.push_back(instance);
                    }
                }
            }
            else if constexpr (has_deserialize_method_v<T>)
            {
                // Custom type with deserialize method
                T item;
                item.deserialize(itemNode);
                vec.push_back(item);
            }
            else
            {
                // Error: unsupported type
                static_assert(isBasicType<T>() || std::is_base_of_v<Serializable, T> ||
                              (is_shared_ptr_v<T> && std::is_base_of_v<Serializable, typename T::element_type>) ||
                              has_deserialize_method_v<T>,
                              "Type must be deserializable");
            }
        }
    }

    template<typename T>
    void SerializationHelper<T>::serializeMap(DataNode& node, const std::string& name, const std::unordered_map<std::string, T>& map)
    {
        auto& mapNode = node.createChild(name);
        mapNode.setType(DataNode::Type::Object);

        for (const auto& [key, value] : map)
        {
            auto& valueNode = mapNode.createChild(key);

            // Handle different types
            if constexpr (isBasicType<T>())
            {
                // Basic type that can be directly serialized
                valueNode.setValue(value);
            }
            else if constexpr (std::is_base_of_v<Serializable, T>)
            {
                // Serializable object
                value.serialize(valueNode);
            }
            else if constexpr (is_shared_ptr_v<T> && std::is_base_of_v<Serializable, typename T::element_type>)
            {
                // Shared pointer to Serializable
                if (value)
                {
                    value->serialize(valueNode);
                    // Store type information for deserialization
                    valueNode.setAttribute("type", typeid(*value).name());
                }
                else
                {
                    // Null pointer
                    valueNode.setAttribute("null", true);
                }
            }
            else if constexpr (has_serialize_method_v<T>)
            {
                // Custom type with serialize method
                value.serialize(valueNode);
            }
            else
            {
                // Error: unsupported type
                static_assert(isBasicType<T>() || std::is_base_of_v<Serializable, T> ||
                              (is_shared_ptr_v<T> && std::is_base_of_v<Serializable, typename T::element_type>) ||
                              has_serialize_method_v<T>,
                              "Type must be serializable");
            }
        }
    }

    template<typename T>
    void SerializationHelper<T>::deserializeMap(const DataNode& node, const std::string& name, std::unordered_map<std::string, T>& map)
    {
        const DataNode* mapNode = node.findChild(name);
        if (!mapNode || mapNode->getType() != DataNode::Type::Object)
        {
            Logger::warn("Failed to deserialize map '" + name + "': map node not found or invalid type");
            return;
        }

        map.clear();

        for (size_t i = 0; i < mapNode->getChildCount(); ++i)
        {
            const DataNode& valueNode = mapNode->getChild(i);
            std::string key = valueNode.getName();

            // Handle different types
            if constexpr (isBasicType<T>())
            {
                // Basic type that can be directly deserialized
                T value;
                if (valueNode.getValue(value))
                {
                    map[key] = value;
                }
            }
            else if constexpr (std::is_base_of_v<Serializable, T>)
            {
                // Serializable object
                T value;
                value.deserialize(valueNode);
                map[key] = value;
            }
            else if constexpr (is_shared_ptr_v<T> && std::is_base_of_v<Serializable, typename T::element_type>)
            {
                // Shared pointer to Serializable
                bool isNull = false;
                if (valueNode.getAttribute("null", isNull) && isNull)
                {
                    // Null pointer
                    map[key] = nullptr;
                }
                else
                {
                    std::string typeName;
                    if (valueNode.getAttribute("type", typeName))
                    {
                        // Create instance of the correct type
                        auto instance = SerializationTypeRegistry::getInstance().createInstance(typeName);
                        if (instance)
                        {
                            instance->deserialize(valueNode);
                            map[key] = std::static_pointer_cast<typename T::element_type>(instance);
                        }
                        else
                        {
                            // Fallback to default type if the registered type can't be created
                            auto defaultInstance = std::make_shared<typename T::element_type>();
                            defaultInstance->deserialize(valueNode);
                            map[key] = defaultInstance;
                        }
                    }
                    else
                    {
                        // No type information, create default type
                        auto instance = std::make_shared<typename T::element_type>();
                        instance->deserialize(valueNode);
                        map[key] = instance;
                    }
                }
            }
            else if constexpr (has_deserialize_method_v<T>)
            {
                // Custom type with deserialize method
                T value;
                value.deserialize(valueNode);
                map[key] = value;
            }
            else
            {
                // Error: unsupported type
                static_assert(isBasicType<T>() || std::is_base_of_v<Serializable, T> ||
                              (is_shared_ptr_v<T> && std::is_base_of_v<Serializable, typename T::element_type>) ||
                              has_deserialize_method_v<T>,
                              "Type must be deserializable");
            }
        }
    }

    template<typename T>
    void SerializationHelper<T>::serializeObject(DataNode& node, const std::string& name, const Serializable* object)
    {
        if (!object)
        {
            Logger::warn("Attempt to serialize null object '" + name + "'");
            return;
        }

        auto& objectNode = node.createChild(name);
        object->serialize(objectNode);

        // Store serialization version
        objectNode.setAttribute("version", object->getSerializationVersion());

        // Store type information for polymorphic deserialization
        objectNode.setAttribute("type", typeid(*object).name());
    }

    template<typename T>
    void SerializationHelper<T>::deserializeObject(const DataNode& node, const std::string& name, Serializable* object)
    {
        if (!object)
        {
            Logger::warn("Attempt to deserialize into null object '" + name + "'");
            return;
        }

        const DataNode* objectNode = node.findChild(name);
        if (!objectNode)
        {
            Logger::warn("Failed to deserialize object '" + name + "': node not found");
            return;
        }

        // Check serialization version
        uint32_t version = 0;
        if (objectNode->getAttribute("version", version))
        {
            // Handle version differences here if needed
            if (version > object->getSerializationVersion())
            {
                Logger::warn("Deserializing from newer version (" + std::to_string(version) +
                             ") into older version (" + std::to_string(object->getSerializationVersion()) +
                             ") of object '" + name + "'");
            }
        }

        // Deserialize the object
        object->deserialize(*objectNode);

        // Validate after deserialization
        if (!object->validateAfterDeserialization())
        {
            Logger::error("Validation failed after deserializing object '" + name + "'");
        }
    }

} // namespace PixelCraft::Core::Serialize