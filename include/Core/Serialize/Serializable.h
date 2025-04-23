// -------------------------------------------------------------------------
// Serializable.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <type_traits>

namespace PixelCraft::Core::Serialize
{

    // Forward declarations
    class DataNode;

    /**
     * @brief Interface for objects that can be serialized in the engine.
     *
     * The Serializable interface defines the contract for objects that can be serialized
     * and deserialized. Classes implementing this interface can be saved to and loaded
     * from DataNode structures, allowing for persistent storage and runtime modifications.
     */
    class Serializable
    {
    public:
        /**
         * @brief Virtual destructor
         */
        virtual ~Serializable() = default;

        /**
         * @brief Serialize this object to the provided data node
         * @param node The data node to serialize into
         */
        virtual void serialize(DataNode& node) const = 0;

        /**
         * @brief Deserialize this object from the provided data node
         * @param node The data node to deserialize from
         */
        virtual void deserialize(const DataNode& node) = 0;

        /**
         * @brief Get the serialization version of this object
         * @return The serialization version number
         *
         * This allows for backward compatibility when the serialization format changes.
         * Default implementation returns 1 (initial version).
         */
        virtual uint32_t getSerializationVersion() const;

        /**
         * @brief Validate the object state after deserialization
         * @return True if the object is in a valid state, false otherwise
         *
         * This method is called after deserialize() to ensure the object is in a valid state.
         * Default implementation returns true (validation successful).
         */
        virtual bool validateAfterDeserialization();
    };

    /**
     * @brief Registry for serializable types that enables runtime type creation
     *
     * This singleton class maintains a registry of serializable types that can be
     * instantiated at runtime given their type name. This is useful for systems
     * that need to create objects based on serialized data.
     */
    class SerializationTypeRegistry
    {
    public:
        /**
         * @brief Function type for creating serializable instances
         */
        using CreateFunc = std::function<std::shared_ptr<Serializable>()>;

        /**
         * @brief Get the singleton instance
         * @return Reference to the singleton instance
         */
        static SerializationTypeRegistry& getInstance();

        /**
         * @brief Register a serializable type
         * @param typeName The name of the type to register
         * @param createFunc Factory function to create instances of the type
         * @return True if registration was successful, false if the type was already registered
         */
        bool registerType(const std::string& typeName, CreateFunc createFunc);

        /**
         * @brief Create an instance of a registered type
         * @param typeName The name of the type to create
         * @return Shared pointer to the created instance, or nullptr if the type is not registered
         */
        std::shared_ptr<Serializable> createInstance(const std::string& typeName);

        /**
         * @brief Check if a type is registered
         * @param typeName The name of the type to check
         * @return True if the type is registered, false otherwise
         */
        bool isTypeRegistered(const std::string& typeName) const;

        /**
         * @brief Get a list of all registered type names
         * @return Vector of registered type names
         */
        std::vector<std::string> getRegisteredTypes() const;

    private:
        /**
         * @brief Private constructor to enforce singleton pattern
         */
        SerializationTypeRegistry() = default;

        /**
         * @brief Private destructor to enforce singleton pattern
         */
        ~SerializationTypeRegistry() = default;

        /**
         * @brief Map of type names to factory functions
         */
        std::unordered_map<std::string, CreateFunc> m_typeRegistry;

        /**
         * @brief Mutex for thread-safe access to the registry
         */
        mutable std::mutex m_registryMutex;
    };

    /**
     * @brief Convenience macro for registering serializable types
     *
     * This macro creates a static variable that registers the type with the
     * SerializationTypeRegistry when the program starts.
     *
     * @param TypeName The name of the type to register
     */
#define REGISTER_SERIALIZABLE_TYPE(TypeName) \
    namespace { \
        static bool TypeName##_registered = PixelCraft::Core::SerializationTypeRegistry::getInstance().registerType( \
            #TypeName, \
            []() -> std::shared_ptr<PixelCraft::Core::Serializable> { return std::make_shared<TypeName>(); } \
        ); \
    }

     // Type traits for detecting shared_ptr
    template<typename T>
    struct is_shared_ptr : std::false_type
    {
    };

    template<typename T>
    struct is_shared_ptr<std::shared_ptr<T>> : std::true_type
    {
    };

    template<typename T>
    constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

    // Type trait for detecting serialize method
    template<typename T, typename = void>
    struct has_serialize_method : std::false_type
    {
    };

    template<typename T>
    struct has_serialize_method<T, std::void_t<decltype(std::declval<const T>().serialize(std::declval<DataNode&>()))>> : std::true_type
    {
    };

    template<typename T>
    constexpr bool has_serialize_method_v = has_serialize_method<T>::value;

    // Type trait for detecting deserialize method
    template<typename T, typename = void>
    struct has_deserialize_method : std::false_type
    {
    };

    template<typename T>
    struct has_deserialize_method<T, std::void_t<decltype(std::declval<T>().deserialize(std::declval<const DataNode&>()))>> : std::true_type
    {
    };

    template<typename T>
    constexpr bool has_deserialize_method_v = has_deserialize_method<T>::value;

    /**
     * @brief Helper template for common serialization tasks
     *
     * This class provides utility methods for serializing common data structures
     * like vectors and maps.
     *
     * @tparam T The type of elements being serialized
     */
    template<typename T>
    class SerializationHelper
    {
    public:
        /**
         * @brief Serialize a vector of objects
         * @param node The data node to serialize into
         * @param name The name of the vector in the data node
         * @param vec The vector to serialize
         */
        static void serializeVector(DataNode& node, const std::string& name, const std::vector<T>& vec);

        /**
         * @brief Deserialize a vector of objects
         * @param node The data node to deserialize from
         * @param name The name of the vector in the data node
         * @param vec The vector to populate with deserialized objects
         */
        static void deserializeVector(const DataNode& node, const std::string& name, std::vector<T>& vec);

        /**
         * @brief Serialize a map of objects
         * @param node The data node to serialize into
         * @param name The name of the map in the data node
         * @param map The map to serialize
         */
        static void serializeMap(DataNode& node, const std::string& name, const std::unordered_map<std::string, T>& map);

        /**
         * @brief Deserialize a map of objects
         * @param node The data node to deserialize from
         * @param name The name of the map in the data node
         * @param map The map to populate with deserialized objects
         */
        static void deserializeMap(const DataNode& node, const std::string& name, std::unordered_map<std::string, T>& map);

        /**
         * @brief Serialize a serializable object
         * @param node The data node to serialize into
         * @param name The name of the object in the data node
         * @param object The object to serialize
         */
        static void serializeObject(DataNode& node, const std::string& name, const Serializable* object);

        /**
         * @brief Deserialize a serializable object
         * @param node The data node to deserialize from
         * @param name The name of the object in the data node
         * @param object The object to populate with deserialized data
         */
        static void deserializeObject(const DataNode& node, const std::string& name, Serializable* object);
    };

} // namespace PixelCraft::Core::Serialize

// Include template implementations
#include "Core/Serializable.inl"