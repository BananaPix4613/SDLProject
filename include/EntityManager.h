#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <mutex>
#include <typeindex>
#include <atomic>
#include <glm.hpp>
#include <shared_mutex>
#include <unordered_set>

// Forward declarations
class Entity;
class Component;
class Scene;
class EventSystem;
struct EntityPrefab;

/**
 * @typedef EntityID
 * @brief Unique identifier for entities
 */
using EntityID = uint64_t;

/**
 * @typedef EntityHandle
 * @brief Stable handle to an entity, consisting of ID and generation
 */
struct EntityHandle
{
    EntityID id;         // Unique entity ID
    uint32_t generation; // Generation counter for ID reuse detection

    // Comparison operators
    bool operator==(const EntityHandle& other) const
    {
        return id == other.id && generation == other.generation;
    }

    bool operator!=(const EntityHandle& other) const
    {
        return !(*this == other);
    }

    // Convenience function to check if handle is valid
    bool isValid() const
    {
        return id != 0;
    }
};

// Hash function for EntityHandle
namespace std
{
    template<>
    struct hash<EntityHandle>
    {
        size_t operator()(const EntityHandle& handle) const
        {
            // Combine hash of id and generation
            return std::hash<EntityID>()(handle.id) ^
                (std::hash<uint32_t>()(handle.generation) << 1);
        }
    };
}

/**
 * @struct EntityQuery
 * @brief Filter criteria for entity queries
 */
struct EntityQuery
{
    std::string name;                                 // Entity name (empty for any)
    std::string tag;                                  // Entity tag (empty for any)
    std::vector<std::type_index> requiredComponents;  // Components that must be present
    std::vector<std::type_index> excludedComponents;  // Components that must not be present
    bool activeOnly = true;                          // Whether to include only active entities

    // Add a required component type
    template<typename T>
    EntityQuery& with()
    {
        requiredComponents.push_back(std::type_index(typeid(T)));
        return *this;
    }

    // Add an excluded component type
    template<typename T>
    EntityQuery& without()
    {
        excludedComponents.push_back(std::type_index(typeid(T)));
        return *this;
    }

    // Set name filter
    EntityQuery& withName(const std::string& entityName)
    {
        name = entityName;
        return *this;
    }

    // Set tag filter
    EntityQuery& withTag(const std::string& entityTag)
    {
        tag = entityTag;
        return *this;
    }

    // Configure whether to include inactive entities
    EntityQuery& includeInactive(bool include)
    {
        activeOnly = !include;
        return *this;
    }

    // Static factory method for a new query
    static EntityQuery create()
    {
        return EntityQuery();
    }
};

/**
 * @struct EntityArchetype
 * @brief Blueprint for creating entities with a specific component configuration
 */
struct EntityArchetype
{
    std::string name;                           // Archetype name
    std::vector<std::type_index> componentTypes; // Component types in this archetype

    // Add a component type to the archetype
    template<typename T>
    EntityArchetype& with()
    {
        componentTypes.push_back(std::type_index(typeid(T)));
        return *this;
    }
};

/**
 * @struct EntityPrefab
 * @brief Template for creating entities with specific components and initial values
 */
struct EntityPrefab
{
    std::string name;                              // Prefab name
    std::string tag;                               // Default tag for entities
    bool active = true;                            // Default active state

    // Template components with serialized data
    struct PrefabComponent
    {
        std::type_index typeIndex;                // Component type
        std::function<Component* ()> factory;      // Factory function to create component
        std::function<void(Component*)> initializer; // Function to initialize component data
    };

    std::vector<PrefabComponent> components;      // Components in this prefab

    // Add a component type to the prefab with initializer function
    template<typename T>
    EntityPrefab& withComponent(std::function<void(T*)> initializer = nullptr)
    {
        PrefabComponent prefabComponent;
        prefabComponent.typeIndex = std::type_index(typeid(T));

        // Factory to create component
        prefabComponent.factory = []() -> Component* {
            return new T();
            };

        // Initializer to configure component if provided
        if (initializer)
        {
            prefabComponent.initializer = [initializer](Component* component) {
                initializer(static_cast<T*>(component));
                };
        }
        else
        {
            prefabComponent.initializer = [](Component*) {};
        }

        components.push_back(prefabComponent);
        return *this;
    }
};

/**
 * @class EntityManager
 * @brief Central system for entity management and lifecycle control
 * 
 * The EntityManager is responsible for creating, destroying, and tracking
 * all entities in the game. It provides efficient entity queries, handles
 * entity lifecycle events, and supports prefab instantiation.
 */
class EntityManager
{
public:
    /**
     * @brief Constructor
     * @param scene Parent scene to manage entities for
     * @param eventSystem Event system for entity lifecycle events
     */
    EntityManager(Scene* scene, EventSystem* eventSystem);

    /**
     * @brief Destructor
     */
    ~EntityManager();

    /**
     * @brief Initialize the entity manager
     */
    bool initialize();

    /**
     * @brief Update all entities
     * @param deltaTime Time elapsed since last update
     */
    void update(float deltaTime);

    /**
     * @brief Create a new entity
     * @param name Optional name for the entity
     * @return Pointer to the created entity
     */
    Entity* createEntity(const std::string& name = "Entity");

    /**
     * @brief Create a new entity at a specific position
     * @param position Initial position for the entity
     * @param name Optional name for the entity
     * @return Pointer to the created entity
     */
    Entity* createEntityAt(const glm::vec3& position, const std::string& name = "Entity");

    /**
     * @brief Create a new entity as a child of another entity
     * @param parent Parent entity
     * @param name Optional name for the entity
     * @return Pointer to the created entity
     */
    Entity* createChildEntity(Entity* parent, const std::string& name = "Entity");

    /**
     * @brief Create an entity based on an archetype
     * @param archetype Entity archetype to use as a template
     * @param name Optional name for the entity (overrides archetype name)
     * @return Pointer to the created entity
     */
    Entity* createEntityFromArchetype(const EntityArchetype& archetype, const std::string& name = "");

    /**
     * @brief Create a batch of entities based on an archetype
     * @param archetype Entity archetype to use as a template
     * @param count Number of entities to create
     * @param namePrefix Optional prefix for entity names
     * @return Vector of pointers to the created entities
     */
    std::vector<Entity*> createEntitiesFromArchetype(const EntityArchetype& archetype,
                                                     int count,
                                                     const std::string& namePrefix = "");

    /**
     * @brief Create an entity based on a prefab
     * @param prefab Entity prefab to use as a template
     * @param name Optional name for the entity (overrides prefab name)
     * @return Pointer to the created entity
     */
    Entity* instantiatePrefab(const EntityPrefab& prefab, const std::string& name = "");

    /**
     * @brief Create an entity based on a named prefab
     * @param prefabName Name of the prefab to instantiate
     * @param name Optional name for the entity (overrides prefab name)
     * @return Pointer to the created entity or nullptr if prefab not found
     */
    Entity* instantiatePrefab(const std::string& prefabName, const std::string& name = "");

    /**
     * @brief Register a prefab for future instantiation
     * @param prefab Prefab to register
     */
    void registerPrefab(const EntityPrefab& prefab);

    /**
     * @brief Register a prefab with a custom builder function
     * @param prefabName Name of the prefab
     * @param builder Function that builds and renames the prefab
     */
    void registerPrefab(const std::string& prefabName,
                        std::function<EntityPrefab()> builder);

    /**
     * @brief Destroy an entity
     * @param entity Pointer to the entity to destroy
     * @return True if entity was successfully destroyed
     */
    bool destroyEntity(Entity* entity);

    /**
     * @brief Destroy an entity by ID
     * @param entityId ID of the entity to destroy
     * @return True if entity was successfully destroyed
     */
    bool destroyEntity(EntityID entityId);

    /**
     * @brief Destroy an entity using its handle
     * @param handle Handle of the entity to destroy
     * @return True if an entity was successfully destroyed
     */
    bool destroyEntity(const EntityHandle& handle);

    /**
     * @brief Destroy all entities matching a query
     * @param query Query defining which entities to destroy
     * @return Number of entities destroyed
     */
    int destroyEntities(const EntityQuery& query);

    /**
     * @brief Destroy all entities immediately
     * @param immediate Whether to bypass and destroy queue
     * @return Number of entities destroyed
     */
    int destroyAllEntities(bool immediate = false);

    /**
     * @brief Process pending entity destruction
     */
    void processPendingDestruction();

    /**
     * @brief Get entity by ID
     * @param entityId ID of the entity to retrieve
     * @return Pointer to the entity or nullptr if not found
     */
    Entity* getEntity(EntityID entityId) const;

    /**
     * @brief Get entity by handle
     * @param handle Handle of the entity to retrieve
     * @return Pointer to the entity or nullptr if not found or handle invalid
     */
    Entity* getEntity(const EntityHandle& handle) const;

    /**
     * @brief Get entity by name
     * @param name Name of the entity to retrieve
     * @return Pointer to the first entity with matching name or nullptr if not found
     */
    Entity* getEntityByName(const std::string& name) const;

    /**
     * @brief Find all entities with a specific tag
     * @param tag Tag to search for
     * @return Vector of pointers to entities with matching tag
     */
    std::vector<Entity*> getEntitiesByTag(const std::string& tag) const;

    /**
     * @brief Get all entities with a specific component type
     * @tparam T Component type to search for
     * @return Vector of pointers to entities with the component
     */
    template<typename T>
    std::vector<Entity*> getEntitiesWithComponent() const
    {
        std::vector<Entity*> result;
        std::type_index typeIndex = std::type_index(typeid(T));

        // Lock for thread safety during read
        std::_Shared_ptr_spin_lock<std::shared_mutex> lock(m_entityMutex);

        // Find component type in index, if it exists
        auto it = m_componentEntityIndex.find(typeIndex);
        if (it != m_componentEntityIndex.end())
        {
            // Reserve space for efficiency
            result.reserve(it->second.size());

            // Get entities and filter active if needed
            for (EntityID entityId : it->second)
            {
                auto entityId = m_entities.find(entityId);
                if (entityId != m_entities.end())
                {
                    result.push_back(entityIt->second.get());
                }
            }
        }

        return result;
    }

    /**
     * @brief Query entities based on a set of criteria
     * @param query Query defining which entities to return
     * @return Vector of pointers to entities matching the query
     */
    std::vector<Entity*> queryEntities(const EntityQuery& query) const;

    /**
     * @brief Execute a function on all entities matching a query
     * @param query Query defining which entities to process
     * @param func Function to execute on each matching entity
     */
    void forEachEntity(const EntityQuery& query,
                       std::function<void(Entity*)> func);

    /**
     * @brief Count entities matching a query
     * @param query Query defining which entities to count
     * @return Number of matching entities
     */
    int countEntities(const EntityQuery& query) const;

    /**
     * @brief Get all entities in the manager
     * @return Vector of pointers to all entities
     */
    std::vector<Entity*> getAllEntities() const;

    /**
     * @brief Get the total number of entities
     * @return Entity count
     */
    size_t getEntityCount() const;

    /**
     * @brief Create an entity handle from an entity
     * @param entity Entity to create handle for
     * @return Handle to the entity
     */
    EntityHandle createHandle(Entity* entity) const;

    /**
     * @brief Check if an entity handle is valid
     * @param handle Handle to check
     * @return True if the handle points to a valid entity
     */
    bool isHandleValid(const EntityHandle& handle) const;

    /**
     * @brief Subscribe to entity creation events
     * @param callback Function to call when an entity is created
     * @return Subscription ID for unsubscribing
     */
    int subscribeToEntityCreated(std::function<void(Entity*)> callback);

    /**
     * @brief Subscribe to entity destruction events
     * @param callback Function to call when an entity is about to be destroyed
     * @return Subscription ID for unsubscribing
     */
    int subscribeToEntityDestroyed(std::function<void(Entity*)> callback);

    /**
     * @brief Unsubscribe from entity creation events
     * @param subscriptionId ID returned from subscribeToEntityCreated
     */
    void unsubscribeFromEntityCreated(int subscriptionId);

    /**
     * @brief Unsubscribe from entity destruction events
     * @param subscriptionId ID returned from subscribeToEntityDestroyed
     */
    void unsubscribeFromEntityDestroyed(int subscriptionId);

    /**
     * @brief Register a component type with the entity manager
     * @tparam T Component type to register
     */
    template<typename T>
    void registerComponentType()
    {
        std::type_index typeIndex = std::type_index(typeid(T));
        m_registeredComponentTypes.insert(typeIndex);
    }

    /**
     * @brief Check if an entity exists
     * @param entityId ID of the entity to check
     * @return True if the entity exists
     */
    bool entityExists(EntityID entityId) const;

    /**
     * @brief Check if an entity exists and is valid
     * @param handle Handle of the entity to check
     * @return True if the entity exists and handle is valid
     */
    bool entityExists(const EntityHandle& handle) const;

    /**
     * @brief Set the parent scene
     * @param scene New parent scene
     */
    void setScene(Scene* scene);

    /**
     * @brief Get the parent scene
     * @return Pointer to the parent scene
     */
    Scene* getScene() const;

    /**
     * @brief Get the event system
     * @return Pointer to the event system
     */
    EventSystem* getEventSystem() const;

    /**
     * @brief Notify that a component was added to an entity
     * @param entity Entity that received the component
     * @param componentType Type index of the added component
     */
    void onComponentAdded(Entity* entity, const std::type_index& componentType);

    /**
     * @brief Notify that a component was removed from an entity
     * @param entity Entity that lost the component
     * @param componentType Type index of the removed component
     */
    void onComponentRemoved(Entity* entity, const std::type_index& componentType);

    /**
     * @brief Create a new entity archetype
     * @param name Name for the archetype
     * @return New entity archetype
     */
    EntityArchetype createArchetype(const std::string& name);

    /**
     * @brief Register an entity archetype
     * @param archetype Archetype to register
     */
    void registerArchetype(const EntityArchetype& archetype);

    /**
     * @brief Get a registered archetype by name
     * @param name Name of the archetype
     * @return Pointer to the archetype or nullptr if not found
     */
    const EntityArchetype* getArchetype(const std::string& name) const;

    /**
     * @brief Create a new prefab builder
     * @param name Name for the prefab
     * @return New entity prefab
     */
    EntityPrefab createPrefab(const std::string& name);

    /**
     * @brief Get a registered prefab by name
     * @param name Name of the prefab
     * @return Pointer to the prefab or nullptr if not found
     */
    const EntityPrefab* getPrefab(const std::string& name) const;

    /**
     * @brief Duplicate an existing entity
     * @param sourceEntity Entity to duplicate
     * @param newName optional name for the new entity
     * @return Pointer to the duplicate entity
     */
    Entity* duplicateEntity(Entity* sourceEntity, const std::string& newName = "");

    /**
     * @brief Set the active state of multiple entities
     * @param entities Entities to modify
     * @param active New active state
     */
    void setEntitiesActive(const std::vector<Entity*>& entities, bool active);

private:
    // Parent scene and event system
    Scene* m_scene;
    EventSystem* m_eventSystem;

    // Entity storage and indexing
    std::unordered_map<EntityID, std::unique_ptr<Entity>> m_entities;
    std::unordered_map<std::string, std::vector<EntityID>> m_nameIndex;
    std::unordered_map<std::string, std::vector<EntityID>> m_tagIndex;
    std::unordered_map<std::type_index, std::vector<EntityID>> m_componentEntityIndex;

    // Thread safety
    mutable std::shared_mutex m_entityMutex;

    // Entity ID generation
    std::atomic<EntityID> m_nextEntityId;
    std::unordered_map<EntityID, uint32_t> m_entityGenerations;

    // Registered component types
    std::unordered_set<std::type_index> m_registeredComponentTypes;

    // Entity destroy queue
    std::vector<EntityID> m_destroyQueue;
    std::mutex m_destroyQueueMutex;

    // Archetypes and prefabs
    std::unordered_map<std::string, EntityArchetype> m_archetypes;
    std::unordered_map<std::string, EntityPrefab> m_prefabs;
    std::unordered_map<std::string, std::function<EntityPrefab()>> m_prefabBuilders;

    // Entity event callbacks
    struct CallbackData
    {
        int id;
        std::function<void(Entity*)> callback;
    };

    std::vector<CallbackData> m_entityCreatedCallbacks;
    std::vector<CallbackData> m_entityDestroyedCallbacks;
    int m_nextCallbackId;
    std::mutex m_callbackMutex;

    // Private helper methods
    EntityID generateEntityId();
    void updateComponentIndices(Entity* entity, const std::type_index& componentType, bool added);
    void updateNameIndex(Entity* entity, const std::string& oldName, const std::string& newName);
    void updateTagIndex(Entity* entity, const std::string& oldTag, const std::string& newTag);
    void removeFromIndices(EntityID entityId);
    void notifyEntityCreated(Entity* entity);
    void notifyEntityDestroyed(Entity* entity);
};