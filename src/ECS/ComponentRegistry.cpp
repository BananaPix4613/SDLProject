// -------------------------------------------------------------------------
// ComponentRegistry.cpp
// -------------------------------------------------------------------------
#include "ECS/ComponentRegistry.h"
#include "ECS/Component.h"
#include "Core/Logger.h"

namespace PixelCraft::ECS
{

    // Initialize static members
    ComponentTypeID ComponentRegistry::s_nextComponentTypeID = 1; // Reserve 0 as invalid ID
    std::unordered_map<std::type_index, ComponentTypeID> ComponentRegistry::s_componentTypeIDs;
    std::unordered_map<ComponentTypeID, std::type_index> ComponentRegistry::s_typeIndices;
    std::unordered_map<ComponentTypeID, std::string> ComponentRegistry::s_componentNames;
    std::unordered_map<std::string, ComponentTypeID> ComponentRegistry::s_componentTypeIDsByName;
    std::unordered_map<ComponentTypeID, ComponentRegistry::CreateComponentFunc> ComponentRegistry::s_componentFactories;
    std::unordered_map<ComponentTypeID, bool> ComponentRegistry::s_serializableComponents;
    std::unordered_map<std::type_index, void*> ComponentRegistry::s_componentPools;
    std::mutex ComponentRegistry::s_registryMutex;

    ComponentRegistry& ComponentRegistry::getInstance()
    {
        static ComponentRegistry instance;
        return instance;
    }

    std::string ComponentRegistry::getComponentName(ComponentTypeID typeID)
    {
        std::lock_guard<std::mutex> lock(s_registryMutex);
        auto it = s_componentNames.find(typeID);
        if (it != s_componentNames.end())
        {
            return it->second;
        }
        return ""; // Empty string if not found
    }

    ComponentTypeID ComponentRegistry::getComponentTypeID(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(s_registryMutex);
        auto it = s_componentTypeIDsByName.find(name);
        if (it != s_componentTypeIDsByName.end())
        {
            return it->second;
        }
        return INVALID_COMPONENT_TYPE_ID; // Invalid ID if not found
    }

    std::unique_ptr<Component> ComponentRegistry::createComponent(ComponentTypeID typeID)
    {
        std::lock_guard<std::mutex> lock(s_registryMutex);
        auto it = s_componentFactories.find(typeID);
        if (it != s_componentFactories.end())
        {
            return it->second();
        }

        Log::error("ComponentRegistry: Failed to create component with ID " +
                   std::to_string(typeID) + " - type not registered");
        return nullptr;
    }
    
    std::unique_ptr<Component> ComponentRegistry::createComponent(const std::string& name)
    {
        ComponentTypeID typeID = getComponentTypeID(name);
        if (typeID == INVALID_COMPONENT_TYPE_ID)
        {
            Log::error("ComponentRegistry: Failed to create component named " +
                       name + " - type not registered");
            return nullptr;
        }
        return createComponent(typeID);
    }

    bool ComponentRegistry::isSerializable(ComponentTypeID typeID)
    {
        std::lock_guard<std::mutex> lock(s_registryMutex);
        auto it = s_serializableComponents.find(typeID);
        if (it != s_serializableComponents.end())
        {
            return it->second;
        }
        return false; // Default to false if not explicitly set
    }

    void ComponentRegistry::setSerializable(ComponentTypeID typeID, bool serializable)
    {
        std::lock_guard<std::mutex> lock(s_registryMutex);

        // Verify the component type exists
        if (s_componentNames.find(typeID) == s_componentNames.end())
        {
            Log::warn("ComponentRegistry: Cannot set serializability for unknown component type ID " +
                      std::to_string(typeID));
            return;
        }

        s_serializableComponents[typeID] = serializable;
    }

} // namespace PixelCraft::ECS