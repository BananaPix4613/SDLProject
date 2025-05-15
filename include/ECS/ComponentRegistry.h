// -------------------------------------------------------------------------
// ComponentRegistry.h
// -------------------------------------------------------------------------
#pragma once

#include "ECS/ComponentTypes.h"
#include "ECS/ComponentPool.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <typeindex>

namespace PixelCraft::ECS
{

    /**
     * @brief Registry for component types
     */
    class ComponentRegistry
    {
    public:
        /**
         * @brief Get singleton instance
         * @return Reference to the registry
         */
        static ComponentRegistry& getInstance()
        {
            static ComponentRegistry instance;
            return instance;
        }

        /**
         * @brief Register a component type
         * @tparam T Component type
         * @param name Type name
         * @return Component type ID
         */
        template<typename T>
        ComponentTypeID registerComponent(const std::string& name)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            std::type_index typeIndex(typeid(T));

            // Check if already registered
            auto it = m_typeIndices.find(typeIndex);
            if (it != m_typeIndices.end())
            {
                return it->second;
            }

            // Register new type
            ComponentTypeID typeID = m_nextTypeID++;
            m_typeIndices[typeIndex] = typeID;
            m_typeNames[typeID] = name;

            // Register creation function
            m_poolFactories[typeID] = []() -> std::shared_ptr<IComponentPool> {
                return std::make_shared<ComponentPool<T>>();
                };

            return typeID;
        }

        /**
         * @brief Get component type ID
         * @tparam T Component type
         * @return Component type ID
         */
        template<typename T>
        static ComponentTypeID getComponentTypeID()
        {
            static ComponentTypeID typeID = getInstance().registerComponent<T>(T::getTypeName());
            return typeID;
        }

        /**
         * @brief Get component type name
         * @param typeID Component type ID
         * @return Type name
         */
        std::string getComponentTypeName(ComponentTypeID typeID) const
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            auto it = m_typeNames.find(typeID);
            if (it != m_typeNames.end())
            {
                return it->second;
            }

            return "Unknown";
        }

        /**
         * @brief Get component type ID by name
         * @param name Type name
         * @return Component type ID or INVALID_COMPONENT_TYPE
         */
        ComponentTypeID getComponentTypeIDByName(const std::string& name) const
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            for (const auto& [id, typeName] : m_typeNames)
            {
                if (typeName == name)
                {
                    return id;
                }
            }

            return INVALID_COMPONENT_TYPE;
        }

        /**
         * @brief Create a component pool for a type
         * @param typeID Component type ID
         * @return Shared pointer to the pool or nullptr
         */
        std::shared_ptr<IComponentPool> createPool(ComponentTypeID typeID)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            auto it = m_poolFactories.find(typeID);
            if (it != m_poolFactories.end())
            {
                return it->second();
            }

            return nullptr;
        }

    private:
        ComponentRegistry() = default;
        ~ComponentRegistry() = default;

        ComponentRegistry(const ComponentRegistry&) = delete;
        ComponentRegistry& operator=(const ComponentRegistry&) = delete;

        mutable std::mutex m_mutex;
        ComponentTypeID m_nextTypeID = 0;
        std::unordered_map<std::type_index, ComponentTypeID> m_typeIndices;
        std::unordered_map<ComponentTypeID, std::string> m_typeNames;
        std::unordered_map<ComponentTypeID, std::function<std::shared_ptr<IComponentPool>()>> m_poolFactories;
    };

} // namespace PixelCraft::ECS