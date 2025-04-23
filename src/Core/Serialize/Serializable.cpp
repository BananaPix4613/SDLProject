// -------------------------------------------------------------------------
// Serializable.cpp
// -------------------------------------------------------------------------
#include "Core/Serialize/Serializable.h"
#include "Core/Logger.h"

namespace PixelCraft::Core::Serialize
{

    // Serializable implementation
    uint32_t Serializable::getSerializationVersion() const
    {
        return 1; // Default to version 1
    }

    bool Serializable::validateAfterDeserialization()
    {
        return true; // Default to valid state
    }

    // SerializationTypeRegistry implementation
    SerializationTypeRegistry& SerializationTypeRegistry::getInstance()
    {
        static SerializationTypeRegistry instance;
        return instance;
    }

    bool SerializationTypeRegistry::registerType(const std::string& typeName, CreateFunc createFunc)
    {
        std::lock_guard<std::mutex> lock(m_registryMutex);

        // Check if type already registered
        if (m_typeRegistry.find(typeName) != m_typeRegistry.end())
        {
            warn("Type '" + typeName + "' is already registered in SerializationTypeRegistry");
            return false;
        }

        // Register type
        m_typeRegistry[typeName] = createFunc;
        debug("Registered serializable type: " + typeName);
        return true;
    }

    std::shared_ptr<Serializable> SerializationTypeRegistry::createInstance(const std::string& typeName)
    {
        std::lock_guard<std::mutex> lock(m_registryMutex);

        // Find type
        auto it = m_typeRegistry.find(typeName);
        if (it == m_typeRegistry.end())
        {
            error("Type '" + typeName + "' is not registered in SerializationTypeRegistry");
            return nullptr;
        }

        // Create instance
        try
        {
            return it->second();
        }
        catch (const std::exception& e)
        {
            error("Failed to create instance of type '" + typeName + "': " + e.what());
            return nullptr;
        }
    }

    bool SerializationTypeRegistry::isTypeRegistered(const std::string& typeName) const
    {
        std::lock_guard<std::mutex> lock(m_registryMutex);
        return m_typeRegistry.find(typeName) != m_typeRegistry.end();
    }

    std::vector<std::string> SerializationTypeRegistry::getRegisteredTypes() const
    {
        std::lock_guard<std::mutex> lock(m_registryMutex);

        std::vector<std::string> types;
        types.reserve(m_typeRegistry.size());

        for (const auto& pair : m_typeRegistry)
        {
            types.push_back(pair.first);
        }

        return types;
    }

} // namespace PixelCraft::Core::Serialize