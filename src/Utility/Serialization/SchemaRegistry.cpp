// -------------------------------------------------------------------------
// Utility/Serialization/SchemaRegistry.cpp
// -------------------------------------------------------------------------
#include "Utility/Serialization/SchemaRegistry.h"
#include "Core/Logger.h"

namespace Log = PixelCraft::Core;

namespace PixelCraft::Utility::Serialization
{

    SchemaRegistry& SchemaRegistry::getInstance()
    {
        static SchemaRegistry instance;
        return instance;
    }

    void SchemaRegistry::registerSchema(std::shared_ptr<Schema> schema)
    {
        if (!schema)
        {
            Log::error("Attempted to register null schema");
            return;
        }

        std::lock_guard<std::mutex> lock(m_mutex);

        const std::string& typeName = schema->getTypeName();

        if (m_schemas.find(typeName) != m_schemas.end())
        {
            Log::warn("Schema already registered for type: " + typeName + ". Replacing.");
        }

        m_schemas[typeName] = schema;
        Log::debug("Registered schema for type: " + typeName);
    }

    std::shared_ptr<Schema> SchemaRegistry::getSchema(const std::string& typeName) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_schemas.find(typeName);
        if (it != m_schemas.end())
        {
            return it->second;
        }

        return nullptr;
    }

    bool SchemaRegistry::hasSchema(const std::string& typeName) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_schemas.find(typeName) != m_schemas.end();
    }

    std::vector<std::string> SchemaRegistry::getAllSchemaNames() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::vector<std::string> names;
        names.reserve(m_schemas.size());

        for (const auto& pair : m_schemas)
        {
            names.push_back(pair.first);
        }

        return names;
    }

} // namespace PixelCraft::Utility::Serialization