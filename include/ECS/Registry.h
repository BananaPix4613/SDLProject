// -------------------------------------------------------------------------
// Registry.h
// -------------------------------------------------------------------------
#pragma once

#include "ECS/UUID.h"
#include "ECS/ComponentPool.h"
#include "ECS/ComponentRegistry.h"
#include "ECS/EntityMetadata.h"
#include "Utility/Serialization/SerializationUtility.h"

#include <memory>
#include <map>
#include <unordered_map>
#include <vector>
#include <shared_mutex>

namespace PixelCraft::ECS
{

    // Forward declarations
    class Entity;
    class Component;

    template <typename... Components>
    class ComponentView;

    /**
     * @brief Thread safety documentation for Registry
     *
     * The Registry class is thread-safe for the following operations:
     * - Multiple readers can access entity metadata concurrently
     * - Multiple readers can access components concurrently
     * - Writers obtain exclusive access to the registry for entity creation/destruction
     * - Component access obtains locks on specific component pools
     *
     * Locking strategy uses shared_mutex for read-heavy operations and
     * regular mutex for entity creation/destruction.
     *
     * Thread safety guarantees:
     * - Entity creation/destruction is thread-safe
     * - Component addition/removal is thread-safe
     * - Entity metadata operations are thread-safe
     * - Component queries are thread-safe
     */
    class Registry : public std::enable_shared_from_this<Registry>
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
         * @param generateUUID Whether to generate a UUID for this entity
         * @return The ID of the newly created entity
         * @thread_safety Thread-safe, acquires entity lock
         */
        EntityID createEntity(bool generateUUID = false);

        /**
         * @brief Create a new entity with name
         * @param name The name of the entity
         * @param generateUUID Whether to generate a UUID for this entity
         * @return The ID of the newly created entity
         * @thread_safety Thread-safe, acquires entity lock
         */
        EntityID createEntity(const std::string& name, bool generateUUID = true);

        /**
         * @brief Destroy an entity and all its components
         * @param entity The entity ID to destroy
         * @return True if the entity was successfully destroyed
         * @thread_safety Thread-safe, acquires entity and component locks
         */
        bool destroyEntity(EntityID entity);

        /**
         * @brief Check if an entity is valid
         * @param entity The entity ID to check
         * @return True if the entity exists in the registry
         * @thread_safety Thread-safe, acquires shared entity lock
         */
        bool isValid(EntityID entity) const;

        /**
         * @brief Add a component to an entity
         * @tparam T Component type to add
         * @tparam Args Constructor argument types
         * @param entity Entity ID to add the component to
         * @param args Constructor arguments for the component
         * @return Reference to the added component
         * @thread_safety Thread-safe, acquires component lock
         */
        template<typename T, typename... Args>
        T& addComponent(EntityID entity, Args&&... args);

        /**
         * @brief Remove a component from an entity
         * @tparam T Component type to remove
         * @param entity Entity ID to remove the component from
         * @return True if the component was removed successfully
         * @thread_safety Thread-safe, acquires component lock
         */
        template<typename T>
        bool removeComponent(EntityID entity);

        /**
         * @brief Get a component from an entity
         * @tparam T Component type to get
         * @param entity Entity ID to get the component from
         * @return Reference to the component
         * @thread_safety Thread-safe, acquires shared component lock
         * @throws std::out_of_range if the entity doesn't have this component
         */
        template<typename T>
        T& getComponent(EntityID entity);

        /**
         * @brief Get a const component from an entity
         * @tparam T Component type to get
         * @param entity Entity ID to get the component from
         * @return Const reference to the component
         * @thread_safety Thread-safe, acquires shared component lock
         * @throws std::out_of_range if the entity doesn't have this component
         */
        template <typename T>
        const T& getComponent(EntityID entity) const;

        /**
         * @brief Check if an entity has a component
         * @tparam T Component type to check for
         * @param entity Entity ID to check
         * @return True if the entity has the component
         * @thread_safety Thread-safe, acquires shared component lock
         */
        template<typename T>
        bool hasComponent(EntityID entity) const;

        /**
         * @brief Get a view of entities with the specified component types
         * @tparam Components Component types to include in the view
         * @return View of entities with the specified components
         * @thread_safety Thread-safe, acquires shared component lock
         */
        template<typename... Components>
        ComponentView<Components...> view();

        /**
         * @brief Serialize an entity to binary format
         * @param entity Entity ID to serialize
         * @param serializer Serializer to use
         * @return Serialization result with success
         * @thread_safety Thread-safe, acquires shared locks
         */
        Utility::Serialization::SerializationResult serialize(EntityID entity, Utility::Serialization::Serializer& serializer);

        /**
         * @brief Deserialize an entity from binary format
         * @param entity Entity ID to deserialize into
         * @param deserializer Deserializer to use
         * @return Serialization result with success
         * @thread_safety Thread-safe, acquires exclusive locks
         */
        Utility::Serialization::SerializationResult deserialize(EntityID entity, Utility::Serialization::Deserializer& deserializer);

        /**
         * @brief Serialize all entities to binary format
         * @param serializer Serializer to use
         * @return Serialization result with success
         * @thread_safety Thread-safe, acquires shared locks
         */
        Utility::Serialization::SerializationResult serializeAll(Utility::Serialization::Serializer& serializer);

        /**
         * @brief Deserialize all entities from binary format
         * @param deserializer Deserializer to use
         * @return Serialization result with success
         * @thread_safety Thread-safe, acquires exclusive locks
         */
        Utility::Serialization::SerializationResult deserializeAll(Utility::Serialization::Deserializer& deserializer);

        /**
         * @brief Get the UUID for an entity
         * @param entity Entity ID to get the UUID for
         * @return UUID of the entity
         * @thread_safety Thread-safe, delegates to EntityMetadata
         */
        UUID getEntityUUID(EntityID entity) const;

        /**
         * @brief Set the UUID for an entity
         * @param entity Entity ID to set the UUID for
         * @param uuid UUID to assign
         * @thread_safety Thread-safe, delegates to EntityMetadata
         */
        void setEntityUUID(EntityID entity, const UUID& uuid);

        /**
         * @brief Get an entity by its UUID
         * @param uuid UUID to look up
         * @return Entity ID, or 0 if not found
         * @thread_safety Thread-safe, delegates to EntityMetadata
         */
        EntityID getEntityByUUID(const UUID& uuid) const;

        /**
         * @brief Set the name of an entity
         * @param entity Entity ID to name
         * @param name Name to assign
         * @thread_safety Thread-safe, delegates to EntityMetadata
         */
        void setEntityName(EntityID entity, const std::string& name);

        /**
         * @brief Get the name of an entity
         * @param entity Entity ID to get the name for
         * @return Name of the entity, or empty string if not set
         * @thread_safety Thread-safe, delegates to EntityMetadata
         */
        std::string getEntityName(EntityID entity) const;

        /**
         * @brief Find an entity by name
         * @param name name to search for
         * @return Entity ID, or INVALID_ENTITY_ID if not found
         * @thread_safety Thread-safe, delegates to EntityMetadata
         */
        EntityID findEntityByName(const std::string& name) const;

        /**
         * @brief Add a tag to an entity
         * @param entity Entity ID to tag
         * @param tag Tag to add
         * @thread_safety Thread-safe, delegates to EntityMetadata
         */
        void addTag(EntityID entity, const std::string& tag);

        /**
         * @brief Remove a tag from an entity
         * @param entity Entity ID to untag
         * @param tag Tag to remove
         * @thread_safety Thread-safe, delegates to EntityMetadata
         */
        void removeTag(EntityID entity, const std::string& tag);

        /**
         * @brief Check if an entity has a tag
         * @param entity Entity ID to check
         * @param tag Tag to check for
         * @return True if the entity has the tag
         * @thread_safety Thread-safe, delegates to EntityMetadata
         */
        bool hasTag(EntityID entity, const std::string& tag) const;

        /**
         * @brief Find entities by tag
         * @param tag Tag to search for
         * @return Vector of entity IDs with the tag
         * @thread_safety Thread-safe, delegates to EntityMetadata
         */
        std::vector<EntityID> findEntitiesByTag(const std::string& tag) const;

        /**
         * @brief Enable or disable UUID generation for an entity
         * @param entity Entity ID
         * @param needsUUID Whether the entity needs a UUID
         * @thread_safety Thread-safe, delegates to EntityMetadata
         */
        void setEntityNeedsUUID(EntityID entity, bool needsUUID);

        /**
         * @brief Check if an entity has UUID generation enabled
         * @param entity Entity ID
         * @return True if UUID generation is enabled
         * @thread_safety Thread-safe, delegates to EntityMetadata
         */
        bool entityNeedsUUID(EntityID entity) const;

        /**
         * @brief Set an entity's parent
         * @param entity Entity ID
         * @param parent Parent entity ID
         * @return True if successful
         * @thread_safety Thread-safe, delegates to EntityMetadata
         */
        bool setEntityParent(EntityID entity, EntityID parent);

        /**
         * @brief Get an entity's parent
         * @param entity Entity ID
         * @return Parent entity ID, or INVALID_ENTITY_ID if no parent
         * @thread_safety Thread-safe, delegates to EntityMetadata
         */
        EntityID getEntityParent(EntityID entity) const;

        /**
         * @brief Get an entity's children
         * @param entity Entity ID
         * @return Vector of child entity IDs
         * @thread_safety Thread-safe, delegates to EntityMetadata
         */
        std::vector<EntityID> getEntityChildren(EntityID entity) const;

        /**
         * @brief Set an entity's active state
         * @param entity Entity ID
         * @param active True to set active, false to set inactive
         * @return True if successful
         * @thread_safety Thread-safe, delegates to EntityMetadata
         */
        bool setEntityActive(EntityID entity, bool active);

        /**
         * @brief Check if an entity is active
         * @param entity Entity ID
         * @return True if the entity is active
         * @thread_safety Thread-safe, delegates to EntityMetadata
         */
        bool isEntityActive(EntityID entity) const;

        /**
         * @brief Get the entity metadata manager
         * @return Reference to the entity metadata manager
         * @thread_safety This method does not provide thread safety for the returned reference
         */
        EntityMetadata& getEntityMetadata();

        /**
         * @brief Get the entity metadata manager (const version)
         * @return Const reference to the entity metadata manager
         * @thread_safety This method does not provide thread safety for the returned reference
         */
        const EntityMetadata& getEntityMetadata() const;

        /**
         * @brief Get all entities
         * @return Const reference to the entity set
         * @thread_safety Thread-safe, acquires shared entity lock
         */
        const std::vector<EntityID>& getEntities() const;

        /**
         * @brief Get the component mask for an entity
         * @param entity Entity ID
         * @return Component mask for the entity
         * @thread_safety Thread-safe, acquires shared component lock
         */
        const ComponentMask& getEntityMask(EntityID entity) const;

        /**
         * @brief Get all component pools
         * @return Map of component type IDs to component pools
         * @thread_safety Thread-safe, acquires shared component lock
         */
        const std::map<ComponentTypeID, std::shared_ptr<IComponentPool>>& getAllComponentPools() const;

        /**
         * @brief Get the number of entities in the registry
         * @return Entity count
         * @thread_safety Thread-safe, acquires shared entity lock
         */
        size_t getEntityCount() const;

        /**
         * @brief Get raw component data for an entity
         * @param entity The entity ID to get the component for
         * @param typeID The component type ID
         * @return Pointer to component data or nullptr if not found
         * @thread_safety Thread-safe, acquires shared component lock
         */
        void* getComponentRaw(EntityID entity, ComponentTypeID typeID);

        /**
         * @brief Remove all components from an entity
         * @param entity Entity ID
         * @thread_safety Thread-safe, acquires exclusive component lock
         */
        void removeAllComponents(EntityID entity);

    private:
        // Mutex for entity operations
        mutable std::shared_mutex m_entityMutex;

        // Mutex for component operations
        mutable std::shared_mutex m_componentMutex;

        // Entity storage
        std::vector<EntityID> m_entities;
        std::map<EntityID, ComponentMask> m_entityMasks;
        EntityID m_nextEntityID = 1;

        // Component storage
        std::map<ComponentTypeID, std::shared_ptr<IComponentPool>> m_componentPools;

        // Entity metadata manager
        EntityMetadata m_entityMetadata;

        /**
         * @brief Get or create a component pool
         * @tparam T Component type
         * @return Shared pointer to the component pool
         * @thread_safety Assumes m_componentMutex is already locked
         */
        template <typename T>
        std::shared_ptr<ComponentPool<T>> getComponentPool();

        /**
         * @brief Register a component type
         * @tparam T Component type
         * @return Component type ID
         * @thread_safety Thread-safe, uses ComponentRegistry
         */
        template <typename T>
        static ComponentTypeID registerComponentType();
    };

    // Template implementation for addComponent
    template <typename T, typename... Args>
    T& Registry::addComponent(EntityID entity, Args&&... args)
    {
        // First validate entity with a shared lock
        {
            std::shared_lock entityLock(m_entityMutex);
            if (!isValid(entity))
            {
                throw std::runtime_error("Invalid entity");
            }
        }

        // Register component type
        ComponentTypeID typeID = registerComponentType<T>();

        // Now acquire exclusive lock for component operations
        std::unique_lock componentLock(m_componentMutex);

        // Get or create the component pool
        auto pool = getComponentPool<T>();

        // Create the component
        T& component = pool->create(entity, std::forward<Args>(args)...);

        // Update the entity's component mask
        m_entityMasks[entity].set(typeID);

        return component;
    }

    // Template implementation for removeComponent
    template <typename T>
    bool Registry::removeComponent(EntityID entity)
    {
        // First validate entity with a shared lock
        {
            std::shared_lock entityLock(m_entityMutex);
            if (!isValid(entity))
            {
                return false;
            }
        }

        // Get the component type ID
        ComponentTypeID typeID = registerComponentType<T>();

        // Acquire exclusive lock for component operations
        std::unique_lock componentLock(m_componentMutex);

        // Get the component pool
        auto pool = getComponentPool<T>();

        // Remove the component
        if (pool->destroy(entity))
        {
            // Update the entity's component mask
            m_entityMasks[entity].reset(typeID);
            return true;
        }

        return false;
    }

    // Template implementation for getComponent
    template <typename T>
    T& Registry::getComponent(EntityID entity)
    {
        // First validate entity with a shared lock
        {
            std::shared_lock entityLock(m_entityMutex);
            if (!isValid(entity))
            {
                throw std::runtime_error("Invalid entity");
            }
        }

        // Acquire shared lock for component access
        std::shared_lock componentLock(m_componentMutex);

        auto pool = getComponentPool<T>();
        return pool->get(entity);
    }

    // Template implementation for const getComponent
    template <typename T>
    const T& Registry::getComponent(EntityID entity) const
    {
        // First validate entity with a shared lock
        {
            std::shared_lock entityLock(m_entityMutex);
            if (!isValid(entity))
            {
                throw std::runtime_error("Invalid entity");
            }
        }

        // Acquire shared lock for component access
        std::shared_lock componentLock(m_componentMutex);

        // Need to cast away const here to use the pool
        // This is safe because we return a const reference
        auto registry = const_cast<Registry*>(this);
        auto pool = registry->getComponentPool<T>();
        return pool->get(entity);
    }

    // Template implementation for hasComponent
    template <typename T>
    bool Registry::hasComponent(EntityID entity) const
    {
        // First validate entity with a shared lock
        {
            std::shared_lock entityLock(m_entityMutex);
            if (!isValid(entity))
            {
                throw false;
            }
        }

        // Get the component type ID
        ComponentTypeID typeID = registerComponentType<T>();

        // Check if the entity's mask has the bit set
        return m_entityMasks.at(entity)[typeID];
    }

    // Template implementation for view
    template <typename... Components>
    ComponentView<Components...> Registry::view()
    {
        return ComponentView<Components...>(*this);
    }

    // Template implementation for getComponentPool
    template <typename T>
    std::shared_ptr<ComponentPool<T>> Registry::getComponentPool()
    {
        // Note: This assumes m_componentMutex is already locked

        // Get the component type ID
        ComponentTypeID typeID = registerComponentType<T>();

        // Check if pool exists
        auto it = m_componentPools.find(typeID);
        if (it != m_componentPools.end())
        {
            return std::static_pointer_cast<ComponentPool<T>>(it->second);
        }

        // Create new pool
        auto pool = std::make_shared<ComponentPool<T>>();
        m_componentPools[typeID] = pool;

        return pool;
    }

    // Template implementation for registerComponentType
    template <typename T>
    ComponentTypeID Registry::registerComponentType()
    {
        return ComponentRegistry::getComponentTypeID<T>();
    }

} // namespace PixelCraft::ECS