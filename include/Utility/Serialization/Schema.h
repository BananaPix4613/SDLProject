// -------------------------------------------------------------------------
// Utility/Serialization/Schema.h
// -------------------------------------------------------------------------
#pragma once

#include "Utility/Serialization/SerializationTypes.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace PixelCraft::Utility::Serialization
{

    // Forward declarations
    class Serializer;
    class Deserializer;

    /**
     * @brief Schema definition for serializable types
     */
    class Schema
    {
    public:
        /**
         * @brief Field descriptor for schema
         */
        struct Field
        {
            std::string name;     ///< Field name
            ValueType type;       ///< Field type
            bool required;        ///< Whether field is required
            size_t offset;        ///< Memory offset within object (for direct serialization)
            std::string typeName; ///< Name of complex type if applicable
        };

        /**
         * @brief Constructor with version
         */
        Schema(const std::string& typeName, const VersionInfo& version);

        /**
         * @brief Destructor
         */
        virtual ~Schema();

        /**
         * @brief Add a field to the schema
         */
        void addField(const std::string& name, ValueType type, bool required = true, size_t offset = 0);

        /**
         * @brief Add a complex field to the schema
         */
        void addComplexField(const std::string& name, const std::string& typeName, bool required = true, size_t offset = 0);

        /**
         * @brief Add an array field to the schema
         */
        void addArrayField(const std::string& name, ValueType elementType, bool required = true, size_t offset = 0);

        /**
         * @brief Add a complex array field to the schema
         */
        void addComplexArrayField(const std::string& name, const std::string& elementTypeName, bool required = true, size_t offset = 0);

        /**
         * @brief Get schema version
         */
        const VersionInfo& getVersion() const
        {
            return m_version;
        }

        /**
         * @brief Get schema type name
         */
        const std::string& getTypeName() const
        {
            return m_typeName;
        }

        /**
         * @brief Get all fields
         */
        const std::vector<Field>& getFields() const
        {
            return m_fields;
        }

        /**
         * @brief Get a field by name
         */
        const Field* getField(const std::string& name) const;

        /**
         * @brief Validate field value against schema
         */
        SerializationResult validateField(const Field& field, ValueType actualType) const;

        /**
         * @brief Register serialization functions for this type
         */
        template<typename T>
        void registerFunctions(
            std::function<SerializationResult(const T&, Serializer&)> serializeFunc,
            std::function<SerializationResult(T&, Deserializer&)> deserializeFunc)
        {
            // Store type-erased functions
            m_serializeFunc = [serializeFunc](const void* obj, Serializer& s) -> SerializationResult {
                return serializeFunc(*static_cast<const T*>(obj), s);
                };

            m_deserializeFunc = [deserializeFunc](void* obj, Deserializer& d) -> SerializationResult {
                return deserializeFunc(*static_cast<T*>(obj), d);
                };
        }

        /**
         * @brief Serialize an object using registered function
         */
        SerializationResult serialize(const void* obj, Serializer& serializer) const;

        /**
         * @brief Deserialize an object using registered function
         */
        SerializationResult deserialize(void* obj, Deserializer& deserializer) const;

    private:
        std::string m_typeName;
        VersionInfo m_version;
        std::vector<Field> m_fields;

        // Type-erased serialization functions
        std::function<SerializationResult(const void*, Serializer&)> m_serializeFunc;
        std::function<SerializationResult(void*, Deserializer&)> m_deserializeFunc;
    };

} // namespace PixelCraft::Utility::Serialization