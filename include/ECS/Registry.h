// -------------------------------------------------------------------------
// Registry.h
// -------------------------------------------------------------------------
#pragma once

#include <unordered_set>
#include <unordered_map>
#include <bitset>
#include <queue>
#include <memory>
#include <typeindex>
#include <mutex>
#include <type_traits>

#include "ECS/ComponentPool.h"

namespace PixelCraft::ECS
{

    // Forward declarations
    template <typename T> class ComponentPool;
    template <typename... Components> class View;

    // Constants
    constexpr uint32_t MAX_COMPONENT_TYPES = 64;
    constexpr uint32_t INVALID_ENTITY = 0;

    // Type definitions
    using ComponentMask = std::bitset<MAX_COMPONENT_TYPES>;
    using EntityID = uint32_t;

    /**
     * @brief Central registry for entities and components
     *
     * The Registry is the core of the ECS system, managing entity creation,
     * component storage, and providing efficient queries for entities with
     * specific component combinations.
     */
    class Registry
    {
    public:
        /**
         * @brief Constructor
         */
        Registry();

        /**
         * @brief Destructor
         */
        ~Registry();

        /**
         * @brief Create a new entity
         * @return The ID of the newly created entity
         *
         * Entity IDs are non-zero unsigned integers. Zero is reserved
         * as an invalid entity ID.
         */
        EntityID createEntity();

        /**
         * @brief Destroy an entity and all its components
         * @param entity Entity ID to destroy
         * @return True if the entity was successfully destroyed
         *
         * When an entity is destroyed, all its components are removed,
         * and its ID is recycled for future entity creation.
         */
        bool destroyEntity(EntityID entity);

        /**
         * @brief Check if an entity is valid
         * @param entity Entity ID to check
         * @return True if the entity exists in the registry
         */
        bool isValid(EntityID entity) const;

        /**
         * @brief Get the total number of entities
         * @return The number of active entities
         */
        size_t getEntityCount() const;

        /**
         * @brief Get a reference to the set of all entities
         * @return Const reference to the entity set
         */
        const std::unordered_set<EntityID>& getEntities() const;

        /**
         * @brief Add a component to an entity
         * @tparam T Component type to add
         * @tparam Args Constructor argument types
         * @param entity Entity ID to add component to
         * @param args Constructor arguments forwarded to component constructor
         * @return Pointer to the new component, or nullptr if entity is invalid
         *
         * If the entity already has a component of this type, the existing
         * component is returned instead of creating a new one.
         */
        template<typename T, typename... Args>
        T* addComponent(EntityID entity, Args&&... args)
        {
            if (!isValid(entity))
            {
                return nullptr;
            }

            // Register component type if not already registered
            ComponentTypeID typeID = registerComponentType<T>();

            // Get or create the component pool
            auto pool = getComponentPool<T>();

            // Create the component
            T* component = pool->create(entity, std::forward<Args>(args)...);

            // Update the entity's component mask
            m_entityMasks[entity].set(typeID);

            return component;
        }

        /**
         * @brief Get a component from an entity
         * @tparam T Component type to get
         * @param entity Entity ID to get component from
         * @return Pointer to the component, or nullptr if entity doesn't have it
         */
        template<typename T>
        T* getComponent(EntityID entity)
        {
            if (!isValid(entity))
            {
                return nullptr;
            }

            // Get the component pool
            auto pool = getComponentPool<T>();

            // Get the component
            return pool->get(entity);
        }

        /**
         * @brief Remove a component from an entity
         * @tparam T Component type to remove
         * @param entity Entity ID to remove component from
         * @return True if component was removed, false if entity didn't have it
         */
        template<typename T>
        bool removeComponent(EntityID entity)
        {
            if (!isValid(entity))
            {
                return false;
            }

            // Get the component type ID
            ComponentTypeID typeID = registerComponentType<T>();

            // Get the component pool
            auto pool = getComponentPool<T>();

            // Remove the component
            bool removed = pool->remove(entity);

            // Update the entity's component mask
            if (removed)
            {
                m_entityMasks[entity].reset(typeID);
            }

            return removed;
        }

        /**
         * @brief Check if an entity has a component
         * @tparam T Component type to check
         * @param entity Entity ID to check
         * @return True if entity has the component
         */
        template<typename T>
        bool hasComponent(EntityID entity) const
        {
            if (!isValid(entity))
            {
                return false;
            }

            // Get the component type ID
            ComponentTypeID typeID = s_componentTypes.at(std::type_index(typeid(T)));

            // Check the entity's component mask
            return m_entityMasks.at(entity).test(typeID);
        }

        /**
         * @brief Register a component type
         * @tparam T Component type to register
         * @return The component type ID
         *
         * This method is called automatically the first time a component
         * type is used. It assigns a unique ID to each component type.
         */
        template<typename T>
        static ComponentTypeID registerComponentType()
        {
            std::lock_guard<std::mutex> lock(s_componentTypesMutex);

            auto index = std::type_index(typeid(T));
            auto it = s_componentTypes.find(index);

            if (it != s_componentTypes.end())
            {
                return it->second;
            }

            ComponentTypeID id = s_nextComponentTypeID++;
            if (id >= MAX_COMPONENT_TYPES)
            {
                throw std::runtime_error("Maximum number of component types reached");
            }

            s_componentTypes[index] = id;
            return id;
        }

        /**
         * @brief Get the component mask for an entity
         * @param entity Entity ID to get mask for
         * @return The component mask, indicating which component types the entity has
         */
        const ComponentMask& getComponentMask(EntityID entity) const;

        /**
         * @brief Create a view for iterating entities with specific components
         * @tparam Components Component types to include in the view
         * @return A view object for iteration
         *
         * The view provides efficient iteration over all entities that have
         * all of the specified component types, with direct access to those
         * components.
         */
        template<typename... Components>
        View<Components...> view()
        {
            return View<Components...>(*this);
        }

        /**
         * @brief Get the component pool for a component type
         * @tparam T Component type to get pool for
         * @return Shared pointer to the component pool
         *
         * If the pool doesn't exist yet, it is created.
         */
        template<typename T>
        std::shared_ptr<ComponentPool<T>> getComponentPool()
        {
            auto typeIndex = std::type_index(typeid(T));
            auto it = m_componentPools.find(typeIndex);

            if (it != m_componentPools.end())
            {
                return std::static_pointer_cast<ComponentPool<T>>(it->second);
            }

            // Create a new component pool
            ComponentTypeID typeID = registerComponentType<T>();
            auto pool = std::make_shared<ComponentPool<T>>(typeID);
            m_componentPools[typeIndex] = pool;

            return pool;
        }

        /**
         * @brief Serialize the registry state
         * @param node Data node to serialize into
         */
        void serialize(Core::DataNode& node) const;

        /**
         * @brief Deserialize registry state
         * @param node Data node to deserialize from
         */
        void deserialize(const Core::DataNode& node);

        /**
         * @brief Clear all entities and components
         *
         * Removes all entities and components from the registry,
         * but keeps component type registrations.
         */
        void clear();

    private:
        /**
         * @brief Generate a new entity ID
         * @return A new unique entity ID
         */
        EntityID generateEntityID();

        /**
         * @brief Recycle an entity ID for future use
         * @param entity Entity ID to recycle
         */
        void recycleEntityID(EntityID entity);

        // Core ECS data
        std::unordered_set<EntityID> m_entities;
        std::unordered_map<EntityID, ComponentMask> m_entityMasks;
        std::unordered_map<std::type_index, std::shared_ptr<IComponentPool>> m_componentPools;

        // Entity ID generation
        EntityID m_nextEntityID;
        std::queue<EntityID> m_freeEntityIDs;

        // Component type registration
        static std::unordered_map<std::type_index, ComponentTypeID> s_componentTypes;
        static ComponentTypeID s_nextComponentTypeID;
        static std::mutex s_componentTypesMutex;
    };

} // namespace PixelCraft::ECS