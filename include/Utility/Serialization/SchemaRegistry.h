// -------------------------------------------------------------------------
// Utility/Serialization/SchemaRegistry.h
// -------------------------------------------------------------------------
#pragma once

#include "Utility/Serialization/Schema.h"

#include <memory>
#include <unordered_map>
#include <mutex>

namespace PixelCraft::Utility::Serialization
{

    /**
     * @brief Registry for serialization schemas
     */
    class SchemaRegistry
    {
    public:
        /**
         * @brief Get singleton instance
         */
        static SchemaRegistry& getInstance();

        /**
         * @brief Register a schema
         */
        void registerSchema(std::shared_ptr<Schema> schema);

        /**
         * @brief Get a schema by type name
         */
        std::shared_ptr<Schema> getSchema(const std::string& typeName) const;

        /**
         * @brief Check if a schema exists
         */
        bool hasSchema(const std::string& typeName) const;

        /**
         * @brief Register a type with auto-generated schema
         */
        template<typename T>
        void registerType(const std::string& typeName, const VersionInfo& version)
        {
            auto schema = std::make_shared<Schema>(typeName, version);
            T::defineSchema(*schema);
            schema->registerFunctions<T>(
                [](const T& obj, Serializer& s) { return obj.serialize(s); },
                [](T& obj, Deserializer& d) { return obj.deserialize(d); }
            );
            registerSchema(schema);
        }

        /**
         * @brief Get all registered schema names
         */
        std::vector<std::string> getAllSchemaNames() const;

    private:
        // Private constructor for singleton
        SchemaRegistry() = default;

        std::unordered_map<std::string, std::shared_ptr<Schema>> m_schemas;
        mutable std::mutex m_mutex;
    };

} // namespace PixelCraft::Utility::Serialization