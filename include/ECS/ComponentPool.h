// -------------------------------------------------------------------------
// ComponentPool.h
// -------------------------------------------------------------------------
#pragma once

#include <unordered_map>
#include <vector>
#include <queue>
#include <memory>
#include <cassert>
#include <cstdint>
#include <typeindex>
#include <mutex>

#include "Core/DataNode.h"
#include "Core/Logger.h"

namespace PixelCraft::ECS
{

    using EntityID = uint32_t;
    using ComponentTypeID = uint16_t;

    /**
     * @brief Interface for component type storage pools
     *
     * Provides a common interface for all component pools regardless of their template type,
     * allowing the Registry to store and manage heterogeneous component types through a
     * type-erased interface.
     */
    class IComponentPool
    {
    public:
        /**
         * @brief Virtual destructor
         */
        virtual ~IComponentPool() = default;

        /**
         * @brief Check if the pool contains a component for the specified entity
         * @param entity Entity ID to check
         * @return True if the entity has a component in this pool
         */
        virtual bool has(EntityID entity) const = 0;

        /**
         * @brief Remove a component from the specified entity
         * @param entity Entity ID to remove component from
         * @return True if a component was removed, false if entity had no component
         */
        virtual bool remove(EntityID entity) = 0;

        /**
         * @brief Get the size of the pool (number of components)
         * @return The number of components in the pool
         */
        virtual size_t size() const = 0;

        /**
         * @brief Clear all components from the pool
         */
        virtual void clear() = 0;

        /**
         * @brief Get raw pointer to component data for the specified entity
         * @param entity Entity ID to get component for
         * @return Void pointer to the component data, or nullptr if entity has no component
         */
        virtual void* getRaw(EntityID entity) = 0;

        /**
         * @brief Get const raw pointer to component data for the specified entity
         * @param entity Entity ID to get component for
         * @return Const void pointer to the component data, or nullptr if entity has no component
         */
        virtual const void* getRaw(EntityID entity) const = 0;

        /**
         * @brief Serialize all components in this pool
         * @param node Data node to serialize into
         */
        virtual void serialize(Core::DataNode& node) const = 0;

        /**
         * @brief Deserialize components from a data node
         * @param node Data node to deserialize from
         */
        virtual void deserialize(const Core::DataNode& node) = 0;

        /**
         * @brief Get the component type ID for this pool
         * @return The component type ID
         */
        virtual ComponentTypeID getComponentTypeID() const = 0;
    };

    /**
     * @brief Type-safe storage container for components of a specific type
     *
     * Implements a sparse set pattern for efficient entity-component association
     * with contiguous memory layout for component data to improve cache coherence.
     *
     * @tparam T The component type managed by this pool
     */
    template<typename T>
    class ComponentPool : public IComponentPool
    {
    public:
        /**
         * @brief Constructor
         * @param componentTypeID The ID for this component type
         */
        ComponentPool(ComponentTypeID componentTypeID)
            : m_componentTypeID(componentTypeID)
        {
            // Reserve some initial capacity to avoid early reallocations
            m_components.reserve(64);
            m_entityToIndexMap.reserve(64);
        }

        /**
         * @brief Destructor
         */
        ~ComponentPool() override = default;

        /**
         * @brief Create a new component for the specified entity
         *
         * If the entity already has a component in this pool, the existing component
         * is returned instead of creating a new one.
         *
         * @param entity Entity ID to create component for
         * @param args Arguments to forward to component constructor
         * @return Pointer to the new or existing component
         */
        template<typename... Args>
        T* create(EntityID entity, Args&&... args)
        {
            // If entity already has this component type, return the existing component
            if (auto it = m_entityToIndexMap.find(entity); it != m_entityToIndexMap.end())
            {
                return &m_components[it->second];
            }

            // Create the component
            size_t newIndex;

            // Reuse a free index if available
            if (!m_freeIndices.empty())
            {
                newIndex = m_freeIndices.front();
                m_freeIndices.pop();

                // Construct the component in-place at the free index
                m_components[newIndex] = T(std::forward<Args>(args)...);
            }
            else
            {
                // Create a new component at the end
                newIndex = m_components.size();
                m_components.emplace_back(std::forward<Args>(args)...);
            }

            // Map the entity to the component index
            m_entityToIndexMap[entity] = newIndex;
            m_indexToEntityMap[newIndex] = entity;

            return &m_components[newIndex];
        }

        /**
         * @brief Get the component for the specified entity
         * @param entity Entity ID to get component for
         * @return Pointer to the component, or nullptr if entity has no component
         */
        T* get(EntityID entity)
        {
            auto it = m_entityToIndexMap.find(entity);
            if (it == m_entityToIndexMap.end())
            {
                return nullptr;
            }

            return &m_components[it->second];
        }

        /**
         * @brief Get the component for the specified entity (const version)
         * @param entity Entity ID to get component for
         * @return Const pointer to the component, or nullptr if entity has no component
         */
        const T* get(EntityID entity) const
        {
            auto it = m_entityToIndexMap.find(entity);
            if (it == m_entityToIndexMap.end())
            {
                return nullptr;
            }

            return &m_components[it->second];
        }

        /**
         * @brief Check if the pool contains a component for the specified entity
         * @param entity Entity ID to check
         * @return True if the entity has a component in this pool
         */
        bool has(EntityID entity) const override
        {
            return m_entityToIndexMap.find(entity) != m_entityToIndexMap.end();
        }

        /**
         * @brief Remove a component from the specified entity
         * @param entity Entity ID to remove component from
         * @return True if a component was removed, false if entity had no component
         */
        bool remove(EntityID entity) override
        {
            auto it = m_entityToIndexMap.find(entity);
            if (it == m_entityToIndexMap.end())
            {
                return false;
            }

            size_t indexToRemove = it->second;

            // Add the index to the free list for reuse
            m_freeIndices.push(indexToRemove);

            // Remove the mappings
            m_entityToIndexMap.erase(entity);
            m_indexToEntityMap.erase(indexToRemove);

            return true;
        }
        
        /**
         * @brief Get the size of the pool (number of active components)
         * @return The number of active components in the pool
         */
        size_t size() const override
        {
            return m_entityToIndexMap.size();
        }

        /**
         * @brief Clear all components from the pool
         */
        void clear() override
        {
            m_components.clear();
            m_entityToIndexMap.clear();
            m_indexToEntityMap.clear();

            // Clear the free indices queue
            std::queue<size_t> empty;
            std::swap(m_freeIndices, empty);
        }

        /**
         * @brief Iterate over all components in the pool
         * @param func Function to call for each component
         */
        template<typename Func>
        void forEach(Func&& func)
        {
            for (auto& [entity, index] : m_entityToIndexMap)
            {
                func(entity, m_components[index]);
            }
        }

        /**
         * @brief Iterate over all components in the pool (const version)
         * @param func Function to call for each component
         */
        template<typename Func>
        void forEach(Func&& func) const
        {
            for (auto& [entity, index] : m_entityToIndexMap)
            {
                func(entity, m_components[index]);
            }
        }

        /**
         * @brief Get raw pointer to component data for the specified entity
         * @param entity Entity ID to get component for
         * @return Void pointer to the component data, or nullptr if entity has no component
         */
        void* getRaw(EntityID entity) override
        {
            return static_cast<void*>(get(entity));
        }

        /**
         * @brief Get const raw pointer to component data for the specified entity
         * @param entity Entity ID to get component for
         * @return Const void pointer to the component data, or nullptr if entity has no component
         */
        const void* getRaw(EntityID entity) const override
        {
            return static_cast<const void*>(get(entity));
        }

        /**
         * @brief Serialize all components in this pool
         * @param node Data node to serialize into
         */
        void serialize(Core::DataNode& node) const override
        {
            // Only serializes components if they implement the serialize method
            if constexpr (has_serialize_method<T>::value)
            {
                auto componentsNode = node.createChild("components");

                forEach([&](EntityID entity, const T& component) {
                    auto componentNode = componentsNode.createChild("component");
                    componentNode.setAttribute("entity", entity);

                    // Call the component's serialize method
                    component.serialize(componentNode);
                        });
            }
        }

        /**
         * @brief Deserialize components from a data node
         * @param node Data node to deserialize from
         */
        void deserialize(const Core::DataNode& node) override
        {
            // Only deserializes components if they implement the deserialize method
            if constexpr (has_deserialize_method<T>::value)
            {
                auto componentsNode = node.getChild("components");
                if (!componentsNode.isValid())
                {
                    return;
                }

                for (size_t i = 0; i < componentsNode.getChildCount(); ++i)
                {
                    auto componentNode = componentsNode.getChild(i);

                    EntityID entity = componentNode.getAttribute<EntityID>("entity", 0);
                    if (entity == 0)
                    {
                        Core::Logger::warn("ComponentPool: Invalid entity ID in deserialization");
                        continue;
                    }

                    // Create a new component for this entity
                    T* component = create(entity);

                    // Call the component's deserialize method
                    component->deserialize(componentNode);
                }
            }
        }

        /**
         * @brief Get the component type ID for this pool
         * @return The component type ID
         */
        ComponentTypeID getComponentTypeID() const override
        {
            return m_componentTypeID;
        }

    private:
        // SFINAE to check if T has serialize method
        template<typename U, typename = void>
        struct has_serialize_method : std::false_type
        {
        };

        template<typename U>
        struct has_serialize_method<U, std::void_t<decltype(std::declval<const U>().serialize(std::declval<Core::DataNode&>()))>> : std::true_type
        {
        };

        // SFINAE to check if T has deserialize method
        template<typename U, typename = void>
        struct has_deserialize_method : std::false_type
        {
        };

        template<typename U>
        struct has_deserialize_method<U, std::void_t<decltype(std::declval<U>().deserialize(std::declval<const Core::DataNode&>()))>> : std::true_type
        {
        };

        std::vector<T> m_components;                      // Dense array of components
        std::unordered_map<EntityID, size_t> m_entityToIndexMap;  // Maps entity IDs to component indices
        std::unordered_map<size_t, EntityID> m_indexToEntityMap;  // Maps component indices to entity IDs
        std::queue<size_t> m_freeIndices;                // Queue of free indices for reuse
        ComponentTypeID m_componentTypeID;               // Type ID for this component type
    };

} // namespace PixelCraft::ECS