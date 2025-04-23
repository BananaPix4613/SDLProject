// -------------------------------------------------------------------------
// Component.cpp
// -------------------------------------------------------------------------
#include "Component.h"
#include "Core/Logger.h"

namespace Log = PixelCraft::Core;

namespace PixelCraft::ECS
{

    // Initialize static members of ComponentFactory
    std::unordered_map<std::string, ComponentTypeID> ComponentFactory::s_typeIDs;
    std::unordered_map<ComponentTypeID, std::string> ComponentFactory::s_typeNames;
    std::unordered_map<ComponentTypeID, ComponentFactory::CreateComponentFunc> ComponentFactory::s_factories;
    std::mutex ComponentFactory::s_registryMutex;

    // Component implementation
    Component::Component()
        : m_owner(0), m_enabled(true), m_initialized(false)
    {
    }

    Component::~Component()
    {
        if (m_initialized)
        {
            onDestroy();
        }
    }

    bool Component::initialize()
    {
        if (m_initialized)
        {
            Log::warn("Component already initialized");
            return true;
        }

        m_initialized = true;
        return true;
    }

    void Component::start()
    {
        // Default implementation does nothing
    }

    void Component::update(float deltaTime)
    {
        // Default implementation does nothing
    }

    void Component::render()
    {
        // Default implementation does nothing
    }

    void Component::onDestroy()
    {
        // Default implementation does nothing
        m_initialized = false;
    }

    void Component::setOwner(EntityID owner)
    {
        m_owner = owner;
    }

    EntityID Component::getOwner() const
    {
        return m_owner;
    }

    void Component::serialize(Core::DataNode& node) const
    {
        // Serialize common component properties
        node.setValue("enabled", m_enabled);
        node.setValue("typeID", getTypeID());
        node.setValue("typeName", getTypeName());
    }

    void Component::deserialize(const Core::DataNode& node)
    {
        // Deserialize common component properties
        m_enabled = node.getValue("enabled", true);

        // Type information is validated elsewhere (likely in the entity that owns this component)
    }

    void Component::setEnabled(bool enabled)
    {
        m_enabled = enabled;
    }

    bool Component::isEnabled() const
    {
        return m_enabled;
    }

    bool Component::isInitialized() const
    {
        return m_initialized;
    }

    // ComponentFactory implementation
    bool ComponentFactory::registerComponent(const std::string& name, ComponentTypeID typeID, CreateComponentFunc createFunc)
    {
        // Thread-safe registration
        std::lock_guard<std::mutex> lock(s_registryMutex);

        // Check if already registered
        if (s_typeIDs.find(name) != s_typeIDs.end())
        {
            Log::warn("Component type already registered: " + name);
            return false;
        }

        if (s_typeNames.find(typeID) != s_typeNames.end())
        {
            Log::warn("Component type ID already in use: " + std::to_string(typeID));
            return false;
        }

        if (!createFunc)
        {
            Log::error("Invalid creation function for component: " + name);
            return false;
        }

        // Register the component
        s_typeIDs[name] = typeID;
        s_typeNames[typeID] = name;
        s_factories[typeID] = createFunc;

        Log::debug("Registered component type: " + name + " (ID: " + std::to_string(typeID) + ")");
        return true;
    }

    ComponentPtr ComponentFactory::createComponent(const std::string& name)
    {
        // Thread-safe lookup
        std::lock_guard<std::mutex> lock(s_registryMutex);

        auto it = s_typeIDs.find(name);
        if (it == s_typeIDs.end())
        {
            Log::error("Unknown component type: " + name);
            return nullptr;
        }

        return createComponent(it->second);
    }

    ComponentPtr ComponentFactory::createComponent(ComponentTypeID typeID)
    {
        CreateComponentFunc createFunc;

        {
            // Thread-safe lookup
            std::lock_guard<std::mutex> lock(s_registryMutex);

            auto it = s_factories.find(typeID);
            if (it == s_factories.end())
            {
                Log::error("Unknown component type ID: " + std::to_string(typeID));
                return nullptr;
            }

            createFunc = it->second;
        }

        try
        {
            ComponentPtr component = createFunc();
            if (!component)
            {
                Log::error("Failed to create component of type ID: " + std::to_string(typeID));
                return nullptr;
            }

            if (!component->initialize())
            {
                Log::error("Failed to initialize component of type ID: " + std::to_string(typeID));
                return nullptr;
            }

            return component;
        }
        catch (const std::exception& e)
        {
            Log::error("Exception while creating component of type ID: " + std::to_string(typeID) + " - " + e.what());
            return nullptr;
        }
    }

    ComponentTypeID ComponentFactory::getComponentTypeID(const std::string& name)
    {
        // Thread-safe lookup
        std::lock_guard<std::mutex> lock(s_registryMutex);

        auto it = s_typeIDs.find(name);
        if (it == s_typeIDs.end())
        {
            return 0; // 0 as invalid/unknown type ID
        }

        return it->second;
    }

    std::string ComponentFactory::getComponentTypeName(ComponentTypeID typeID)
    {
        // Thread-safe lookup
        std::lock_guard<std::mutex> lock(s_registryMutex);

        auto it = s_typeNames.find(typeID);
        if (it == s_typeNames.end())
        {
            return "";
        }

        return it->second;
    }

    std::vector<std::string> ComponentFactory::getRegisteredComponentNames()
    {
        // Thread-safe access
        std::lock_guard<std::mutex> lock(s_registryMutex);

        std::vector<std::string> names;
        names.reserve(s_typeIDs.size());

        for (const auto& pair : s_typeIDs)
        {
            names.push_back(pair.first);
        }

        return names;
    }

    bool ComponentFactory::isComponentRegistered(const std::string& name)
    {
        // Thread-safe lookup
        std::lock_guard<std::mutex> lock(s_registryMutex);

        return s_typeIDs.find(name) != s_typeIDs.end();
    }

    bool ComponentFactory::isComponentRegistered(ComponentTypeID typeID)
    {
        // Thread-safe lookup
        std::lock_guard<std::mutex> lock(s_registryMutex);

        return s_typeNames.find(typeID) != s_typeNames.end();
    }

} // namespace PixelCraft::ECS