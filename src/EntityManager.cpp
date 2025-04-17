#include "EntityManager.h"
#include "Entity.h"
#include "Component.h"
#include "Scene.h"
#include "EventSystem.h"
#include <algorithm>
#include <cassert>

// EntityManager constructor
EntityManager::EntityManager(Scene* scene, EventSystem* eventSystem) :
    m_scene(scene),
    m_eventSystem(eventSystem),
    m_nextEntityId(1), // Start from 1, reserve 0 as invalid
    m_nextCallbackId(1)
{
}

// EntityManager destructor
EntityManager::~EntityManager()
{
    destroyAllEntities(true);
}

// Initialize the entity manager
bool EntityManager::initialize()
{
    // Clear any existing data
    m_entities.clear();
    m_nameIndex.clear();
    m_tagIndex.clear();
    m_componentEntityIndex.clear();
    m_entityGenerations.clear();
    m_destroyQueue.clear();
    m_archetypes.clear();
    m_prefabs.clear();
    m_prefabBuilders.clear();
    m_entityCreatedCallbacks.clear();
    m_entityDestroyedCallbacks.clear();

    // Reset entity ID counter
    m_nextEntityId.store(1);
    m_nextCallbackId = 1;

    return true;
}

// Update all entities
void EntityManager::update(float deltaTime)
{
    // Process any pending entity destruction first
    processPendingDestruction();

    // Create a list of entities to update to avoid iterator invalidation if
    // entities are created or destroyed during update
    std::vector<Entity*> entitiesToUpdate;

    {
        // Lock for thread safety during read
        std::shared_lock<std::shared_mutex> lock(m_entityMutex);

        // Reserve space for efficiency
        entitiesToUpdate.reserve(m_entities.size());

        // Collect active entities
        for (const auto& pair : m_entities)
        {
            Entity* entity = pair.second.get();
            if (entity && entity->isActive())
            {
                entitiesToUpdate.push_back(entity);
            }
        }
    }

    // Update each entity
    for (Entity* entity : entitiesToUpdate)
    {
        entity->update(deltaTime);
    }
}

// Create a new entity
Entity* EntityManager::createEntity(const std::string& name)
{
    // Generate a new entity ID
    EntityID entityId = generateEntityId();

    // Create a new entity
    std::unique_ptr<Entity> entity = std::make_unique<Entity>(m_scene, name);
    entity->m_id = entityId;
    Entity* entityPtr = entity.get();

    // Add to entities map and indices
    {
        // Lock for thread safety during write
        std::unique_lock<std::shared_mutex> lock(m_entityMutex);

        // Store the entity
        m_entities[entityId] = std::move(entity);

        // Update name index
        if (!name.empty())
        {
            m_nameIndex[name].push_back(entityId);
        }

        // Update generation map
        m_entityGenerations[entityId] = 1;  // First generation
    }

    // Notify about entity creation
    notifyEntityCreated(entityPtr);

    return entityPtr;
}

// Create a new entity at a specified position
Entity* EntityManager::createEntityAt(const glm::vec3& position, const std::string& name)
{
    Entity* entity = createEntity(name);
    entity->setPosition(position);
    return entity;
}

// Create a new entity as a child of another entity
Entity* EntityManager::createChildEntity(Entity* parent, const std::string& name)
{
    if (!parent)
    {
        return createEntity(name);
    }

    Entity* childEntity = createEntity(name);
    childEntity->setParent(parent);
    return childEntity;
}

// Create an entity based on an archetype
Entity* EntityManager::createEntityFromArchetype(const EntityArchetype& archetype, const std::string& name)
{
    // Use provided name or default to archetype name
    std::string entityName = name.empty() ? archetype.name : name;
    Entity* entity = createEntity(entityName);

    // Add components based on archetype
    for (const std::type_index& typeIndex : archetype.componentTypes)
    {
        // This is just a placeholder, as we can't directly create components from type_index
        // In a real implementation, you would register component factories with the EntityManager
        // and use them here to create the appropriate components

        // For now, we'll assume the Entity class has a way to create components by type
        // entity->createComponentByType(typeIndex);
    }

    return entity;
}

// Create a batch of entities based on an archetype
std::vector<Entity*> EntityManager::createEntitiesFromArchetype(const EntityArchetype& archetype,
                                                                int count,
                                                                const std::string& namePrefix)
{
    std::vector<Entity*> entities;
    entities.reserve(count);

    for (int i = 0; i < count; i++)
    {
        std::string name = namePrefix.empty() ? archetype.name : namePrefix + "_" + std::to_string(i);
        Entity* entity = createEntityFromArchetype(archetype, name);
        entities.push_back(entity);
    }

    return entities;
}

// Create an entity based on a prefab
Entity* EntityManager::instantiatePrefab(const EntityPrefab& prefab, const std::string& name)
{
    // Use provided name or default to prefab name
    std::string entityName = name.empty() ? prefab.name : name;
    Entity* entity = createEntity(entityName);

    // Set entity properties from prefab
    entity->setTag(prefab.tag);
    entity->setActive(prefab.active);

    // Add components based on prefab
    for (const auto& prefabComponent : prefab.components)
    {
        // Create component using factory
        Component* component = prefabComponent.factory();
        if (component)
        {
            // Add to entity (this will call entity->addComponent which handles settings m_entity)
            entity->addExistingComponent(component);

            // Initialize component with prefab data
            prefabComponent.initializer(component);
        }
    }

    return entity;
}

// Create an entity based on a named prefab
Entity* EntityManager::instantiatePrefab(const std::string& prefabName, const std::string& name)
{
    // Check if we have a registered prefab
    auto it = m_prefabs.find(prefabName);
    if (it != m_prefabs.end())
    {
        return instantiatePrefab(it->second, name);
    }

    // Check if we have a dynamic prefab builder
    auto builderIt = m_prefabBuilders.find(prefabName);
    if (builderIt != m_prefabBuilders.end())
    {
        // Build the prefab and instantiate it
        EntityPrefab prefab = builderIt->second();
        return instantiatePrefab(prefab, name);
    }

    // Prefab not found
    return nullptr;
}

// Register a prefab for future instantiation
void EntityManager::registerPrefab(const EntityPrefab& prefab)
{
    m_prefabs[prefab.name] = prefab;
}

// Register a prefab with a custom builder function
void EntityManager::registerPrefab(const std::string& prefabName,
                                   std::function<EntityPrefab()> builder)
{
    m_prefabBuilders[prefabName] = builder;
}

// Destroy an entity
bool EntityManager::destroyEntity(Entity* entity)
{
    if (!entity)
    {
        return false;
    }

    return destroyEntity(entity->m_id);
}

// Destroy an entity by ID
bool EntityManager::destroyEntity(EntityID entityId)
{
    // Queue the entity for destruction
    {
        std::lock_guard<std::mutex> lock(m_destroyQueueMutex);
        m_destroyQueue.push_back(entityId);
    }

    return true;
}

// Destroy an entity using its handle
bool EntityManager::destroyEntity(const EntityHandle& handle)
{
    // Verify handle is valid
    if (!isHandleValid(handle))
    {
        return false;
    }

    return destroyEntity(handle.id);
}

// Destroy all entities matching a query
int EntityManager::destroyEntities(const EntityQuery& query)
{
    std::vector<Entity*> entitiesToDestroy = queryEntities(query);

    // Queue all matched entities for destruction
    {
        std::lock_guard<std::mutex> lock(m_destroyQueueMutex);
        for (Entity* entity : entitiesToDestroy)
        {
            m_destroyQueue.push_back(entity->m_id);
        }
    }

    return static_cast<int>(entitiesToDestroy.size());
}

// Destroy all entities
int EntityManager::destroyAllEntities(bool immediate)
{
    std::vector<EntityID> entityIds;

    // Get all entity IDs
    {
        std::shared_lock<std::shared_mutex> lock(m_entityMutex);
        entityIds.reserve(m_entities.size());

        for (const auto& pair : m_entities)
        {
            entityIds.push_back(pair.first);
        }
    }

    int count = static_cast<int>(entityIds.size());

    if (immediate)
    {
        // Directly destroy all entitites
        for (EntityID entityId : entityIds)
        {
            // Get the entity
            Entity* entity = getEntity(entityId);
            if (entity)
            {
                // Notify before destruction
                notifyEntityDestroyed(entity);

                // Remove from indices
                removeFromIndices(entityId);
            }
        }

        // Clear all entities
        std::unique_lock<std::shared_mutex> lock(m_entityMutex);
        m_entities.clear();
    }
    else
    {
        // Queue all entities for destruction
        std::lock_guard<std::mutex> lock(m_destroyQueueMutex);
        m_destroyQueue.insert(m_destroyQueue.end(), entityIds.begin(), entityIds.end());
    }

    return count;
}

// Process pending entity destruction
void EntityManager::processPendingDestruction()
{
    std::vector<EntityID> entitiesToDestroy;

    // Get the destroy queue
    {
        std::lock_guard<std::mutex> lock(m_destroyQueueMutex);
        entitiesToDestroy.swap(m_destroyQueue);
    }

    // Nothing to do if queue is empty
    if (entitiesToDestroy.empty())
    {
        return;
    }

    // Destroy all queued entities
    for (EntityID entityId : entitiesToDestroy)
    {
        // Get the entity
        Entity* entity = getEntity(entityId);
        if (entity)
        {
            // Notify before destruction
            notifyEntityDestroyed(entity);

            // Remove from indices
            removeFromIndices(entityId);

            // Increment the generation for this ID
            {
                std::unique_lock<std::shared_mutex> lock(m_entityMutex);
                auto genIt = m_entityGenerations.find(entityId);
                if (genIt != m_entityGenerations.end())
                {
                    genIt->second++;
                }

                // Remove from entities map (this will destroy the entity)
                m_entities.erase(entityId);
            }
        }
    }
}

// Get entity by ID
Entity* EntityManager::getEntity(EntityID entityId) const
{
    std::shared_lock<std::shared_mutex> lock(m_entityMutex);

    auto it = m_entities.find(entityId);
    return (it != m_entities.end()) ? it->second.get() : nullptr;
}

// Get entity by handle
Entity* EntityManager::getEntity(const EntityHandle& handle) const
{
    // Verify handle validity first
    if (!isHandleValid(handle))
    {
        return nullptr;
    }

    return getEntity(handle.id);
}

// Get entity by name
Entity* EntityManager::getEntityByName(const std::string& name) const
{
    std::shared_lock<std::shared_mutex> lock(m_entityMutex);

    auto it = m_nameIndex.find(name);
    if (it != m_nameIndex.end() && !it->second.empty())
    {
        // Return the first entity with this name
        EntityID entityId = it->second.front();
        auto entityIt = m_entities.find(entityId);
        if (entityIt != m_entities.end())
        {
            return entityIt->second.get();
        }
    }

    return nullptr;
}

// Find all entities with a specific tag
std::vector<Entity*> EntityManager::getEntitiesByTag(const std::string& tag) const
{
    std::vector<Entity*> result;

    std::shared_lock<std::shared_mutex> lock(m_entityMutex);

    auto it = m_tagIndex.find(tag);
    if (it != m_tagIndex.end())
    {
        result.reserve(it->second.size());

        for (EntityID entityId : it->second)
        {
            auto entityIt = m_entities.find(entityId);
            if (entityIt != m_entities.end())
            {
                result.push_back(entityIt->second.get());
            }
        }
    }

    return result;
}

// Query entities based on a set of criteria
std::vector<Entity*> EntityManager::queryEntities(const EntityQuery& query) const
{
    std::vector<Entity*> candidateEntities;
    size_t minEntityCount = SIZE_MAX;
    std::type_index smallestComponentType = query.requiredComponents.front();

    // Start with all entities if no required components specified,
    // or use the smallest component index as a starting point for better performance
    {
        std::shared_lock<std::shared_mutex> lock(m_entityMutex);

        if (query.requiredComponents.empty())
        {
            // Include all entities as candidates
            candidateEntities.reserve(m_entities.size());
            for (const auto& pair : m_entities)
            {
                candidateEntities.push_back(pair.second.get());
            }
        }
        else
        {
            // Find the component type with the fewest entities
            for (const std::type_index& componentType : query.requiredComponents)
            {
                auto it = m_componentEntityIndex.find(componentType);
                if (it != m_componentEntityIndex.end())
                {
                    if (it->second.size() < minEntityCount)
                    {
                        minEntityCount = it->second.size();
                        smallestComponentType = componentType;
                    }
                }
                else
                {
                    // If any required component has no entities, result is empty
                    return {};
                }
            }

            // Start with entities having the least common component
            auto it = m_componentEntityIndex.find(smallestComponentType);
            if (it != m_componentEntityIndex.end())
            {
                candidateEntities.reserve(it->second.size());
                for (EntityID entityId : it->second)
                {
                    auto entityIt = m_entities.find(entityId);
                    if (entityIt != m_entities.end())
                    {
                        candidateEntities.push_back(entityIt->second.get());
                    }
                }
            }
        }
    }

    // Filter entities based on query criteria
    std::vector<Entity*> result;
    result.reserve(candidateEntities.size());

    for (Entity* entity : candidateEntities)
    {
        // Skip inactive entities if activeOnly is true
        if (query.activeOnly && !entity->isActive())
        {
            continue;
        }

        // Filter by name if specified
        if (!query.name.empty() && entity->getName() != query.name)
        {
            continue;
        }

        // Filter by tag if specified
        if (!query.tag.empty() && entity->getTag() != query.tag)
        {
            continue;
        }

        // Check for required components
        bool hasAllRequired = true;
        for (const std::type_index& componentType : query.requiredComponents)
        {
            if (componentType != smallestComponentType && !entity->hasComponentOfType(componentType))
            {
                hasAllRequired = false;
                break;
            }
        }

        if (!hasAllRequired)
        {
            continue;
        }

        // Check for excluded components
        bool hasExcluded = false;
        for (const std::type_index& componentType : query.excludedComponents)
        {
            if (entity->hasComponentOfType(componentType))
            {
                hasExcluded = true;
                break;
            }
        }

        if (hasExcluded)
        {
            continue;
        }

        // Entity passed all filters, add to result
        result.push_back(entity);
    }

    return result;
}

// Execute a function on all entities matching a query
void EntityManager::forEachEntity(const EntityQuery& query,
                                  std::function<void(Entity*)> func)
{
    if (!func)
    {
        return;
    }

    std::vector<Entity*> matchingEntities = queryEntities(query);

    for (Entity* entity : matchingEntities)
    {
        func(entity);
    }
}

// Count entities matching a query
int EntityManager::countEntities(const EntityQuery& query) const
{
    return static_cast<int>(queryEntities(query).size());
}

// Get all entities in the manager
std::vector<Entity*> EntityManager::getAllEntities() const
{
    std::vector<Entity*> result;

    std::shared_lock<std::shared_mutex> lock(m_entityMutex);

    result.reserve(m_entities.size());
    for (const auto& pair : m_entities)
    {
        result.push_back(pair.second.get());
    }

    return result;
}

// Get the total number of entities
size_t EntityManager::getEntityCount() const
{
    std::shared_lock<std::shared_mutex> lock(m_entityMutex);
    return m_entities.size();
}

// Create an entity handle from an entity
EntityHandle EntityManager::createHandle(Entity* entity) const
{
    EntityHandle handle;
    handle.id = 0;
    handle.generation = 0;

    if (entity)
    {
        handle.id = entity->m_id;

        std::shared_lock<std::shared_mutex> lock(m_entityMutex);
        auto it = m_entityGenerations.find(handle.id);
        if (it != m_entityGenerations.end())
        {
            handle.generation = it->second;
        }
    }

    return handle;
}

// Check if an entity handle is valid
bool EntityManager::isHandleValid(const EntityHandle& handle) const
{
    if (handle.id == 0)
    {
        return false;
    }

    std::shared_lock<std::shared_mutex> lock(m_entityMutex);

    auto genIt = m_entityGenerations.find(handle.id);
    if (genIt == m_entityGenerations.end())
    {
        return false;
    }

    return genIt->second == handle.generation;
}

// Subscribe to entity creation events
int EntityManager::subscribeToEntityCreated(std::function<void(Entity*)> callback)
{
    if (!callback)
    {
        return -1;
    }

    int callbackId;

    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        callbackId = m_nextCallbackId++;

        m_entityCreatedCallbacks.push_back({callbackId, callback});
    }

    return callbackId;
}

// Subscribe to entity destruction events
int EntityManager::subscribeToEntityDestroyed(std::function<void(Entity*)> callback)
{
    if (!callback)
    {
        return -1;
    }

    int callbackId;

    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        callbackId = m_nextCallbackId++;

        m_entityDestroyedCallbacks.push_back({callbackId, callback});
    }

    return callbackId;
}

// Unsubscribe from entity creation events
void EntityManager::unsubscribeFromEntityCreated(int subscriptionId)
{
    std::lock_guard<std::mutex> lock(m_callbackMutex);

    auto it = std::remove_if(
        m_entityCreatedCallbacks.begin(),
        m_entityCreatedCallbacks.end(),
        [subscriptionId](const CallbackData& data) { return data.id == subscriptionId; }
    );

    m_entityCreatedCallbacks.erase(it, m_entityCreatedCallbacks.end());
}

// Unsubscribe from entity destruction events
void EntityManager::unsubscribeFromEntityDestroyed(int subscriptionId)
{
    std::lock_guard<std::mutex> lock(m_callbackMutex);

    auto it = std::remove_if(
        m_entityDestroyedCallbacks.begin(),
        m_entityDestroyedCallbacks.end(),
        [subscriptionId](const CallbackData& data) { return data.id == subscriptionId; }
    );

    m_entityDestroyedCallbacks.erase(it, m_entityDestroyedCallbacks.end());
}

// Check if an entity exists
bool EntityManager::entityExists(EntityID entityId) const
{
    std::shared_lock<std::shared_mutex> lock(m_entityMutex);
    return m_entities.find(entityId) != m_entities.end();
}

// Check if an entity exists and is valid
bool EntityManager::entityExists(const EntityHandle& handle) const
{
    return isHandleValid(handle) && entityExists(handle.id);
}

// Set the parent scene
void EntityManager::setScene(Scene* scene)
{
    m_scene = scene;
}

// Get the parent scene
Scene* EntityManager::getScene() const
{
    return m_scene;
}

// Get the event system
EventSystem* EntityManager::getEventSystem() const
{
    return m_eventSystem;
}

// Notify that a component was added to an entity
void EntityManager::onComponentAdded(Entity* entity, const std::type_index& componentType)
{
    if (!entity)
    {
        return;
    }

    // Update component index
    updateComponentIndices(entity, componentType, true);

    // Dispatch component added event if needed
    if (m_eventSystem)
    {
        // Example: m_eventSystem->dispatch<ComponentAddedEvent>(entity, componentType);
    }
}

// Notify that a component was removed from an entity
void EntityManager::onComponentRemoved(Entity* entity, const std::type_index& componentType)
{
    if (!entity)
    {
        return;
    }

    // Update component index
    updateComponentIndices(entity, componentType, false);

    // Dispatch component removed event if needed
    if (m_eventSystem)
    {
        // Example: m_eventSystem->dispatch<ComponentRemovedEvent>(entity, componentType);
    }
}

// Create a new entity archetype
EntityArchetype EntityManager::createArchetype(const std::string& name)
{
    EntityArchetype archetype;
    archetype.name = name;
    return archetype;
}

// Register an entity archetype
void EntityManager::registerArchetype(const EntityArchetype& archetype)
{
    if (archetype.name.empty())
    {
        return;
    }

    m_archetypes[archetype.name] = archetype;
}

// Get a registered archetype by name
const EntityArchetype* EntityManager::getArchetype(const std::string& name) const
{
    auto it = m_archetypes.find(name);
    return (it != m_archetypes.end()) ? &it->second : nullptr;
}

// Create a new prefab builder
EntityPrefab EntityManager::createPrefab(const std::string& name)
{
    EntityPrefab prefab;
    prefab.name = name;
    return prefab;
}

// Get a registered prefab by name
const EntityPrefab* EntityManager::getPrefab(const std::string& name) const
{
    auto it = m_prefabs.find(name);
    return (it != m_prefabs.end()) ? &it->second : nullptr;
}

// Duplicate an existing entity
Entity* EntityManager::duplicateEntity(Entity* sourceEntity, const std::string& newName)
{
    if (!sourceEntity)
    {
        return nullptr;
    }

    // Create a new entity with the same name or a provided name
    std::string entityName = newName.empty() ? sourceEntity->getName() : newName;
    Entity* newEntity = createEntity(entityName);

    // Copy base properties
    newEntity->setTag(sourceEntity->getTag());
    newEntity->setActive(sourceEntity->isActive());
    newEntity->setPosition(sourceEntity->getPosition());
    newEntity->setRotation(sourceEntity->getRotation());
    newEntity->setScale(sourceEntity->getScale());

    // Clone components
    // This would require a component cloning mechanism
    // For now, we'll assume the Entity class has a way to clone components
    // sourceEntity->cloneComponentsTo(newEntity);

    return newEntity;
}

// Set the active state of multiple entities
void EntityManager::setEntitiesActive(const std::vector<Entity*>& entities, bool active)
{
    for (Entity* entity : entities)
    {
        if (entity)
        {
            entity->setActive(active);
        }
    }
}

// Generate a new unique entity ID
EntityID EntityManager::generateEntityId()
{
    return m_nextEntityId.fetch_add(1);
}

// Update component indices
void EntityManager::updateComponentIndices(Entity* entity, const std::type_index& componentType, bool added)
{
    if (!entity)
    {
        return;
    }

    std::unique_lock<std::shared_mutex> lock(m_entityMutex);

    if (added)
    {
        // Add entity to component index
        m_componentEntityIndex[componentType].push_back(entity->m_id);
    }
    else
    {
        // Remove entity from component index
        auto& entitiesWithComponent = m_componentEntityIndex[componentType];
        entitiesWithComponent.erase(
            std::remove(entitiesWithComponent.begin(), entitiesWithComponent.end(), entity->m_id),
            entitiesWithComponent.end()
        );
    }
}

// Update name index
void EntityManager::updateNameIndex(Entity* entity, const std::string& oldName, const std::string& newName)
{
    if (!entity)
    {
        return;
    }

    std::unique_lock<std::shared_mutex> lock(m_entityMutex);

    // Remove from old name index if name changed
    if (!oldName.empty())
    {
        auto& entitiesWithOldName = m_nameIndex[oldName];
        entitiesWithOldName.erase(
            std::remove(entitiesWithOldName.begin(), entitiesWithOldName.end(), entity->m_id),
            entitiesWithOldName.end()
            );

        // Clean up if empty
        if (entitiesWithOldName.empty())
        {
            m_nameIndex.erase(oldName);
        }
    }

    // Add to new name index
    if (!newName.empty())
    {
        m_nameIndex[newName].push_back(entity->m_id);
    }
}

// Update tag index
void EntityManager::updateTagIndex(Entity* entity, const std::string& oldTag, const std::string& newTag)
{
    if (!entity)
    {
        return;
    }

    std::unique_lock<std::shared_mutex> lock(m_entityMutex);

    // remove from old tag index if tag changed
    if (!oldTag.empty())
    {
        auto& entitiesWithOldTag = m_tagIndex[oldTag];
        entitiesWithOldTag.erase(
            std::remove(entitiesWithOldTag.begin(), entitiesWithOldTag.end(), entity->m_id),
            entitiesWithOldTag.end()
        );

        // Clean up if empty
        if (entitiesWithOldTag.empty())
        {
            m_tagIndex.erase(oldTag);
        }
    }

    // Add to new tag index
    if (!newTag.empty())
    {
        m_tagIndex[newTag].push_back(entity->m_id);
    }
}

// Remove an entity from all indices
void EntityManager::removeFromIndices(EntityID entityId)
{
    std::unique_lock<std::shared_mutex> lock(m_entityMutex);

    // Get the entity before we remove it from indices
    auto entityIt = m_entities.find(entityId);
    if (entityIt == m_entities.end())
    {
        return;
    }

    Entity* entity = entityIt->second.get();

    // Remove from name index
    std::string name = entity->getName();
    if (!name.empty())
    {
        auto nameIt = m_nameIndex.find(name);
        if (nameIt != m_nameIndex.end())
        {
            auto& entities = nameIt->second;
            entities.erase(std::remove(entities.begin(), entities.end(), entityId), entities.end());

            if (entities.empty())
            {
                m_nameIndex.erase(nameIt);
            }
        }
    }

    // Remove from tag index
    std::string tag = entity->getTag();
    if (!tag.empty())
    {
        auto tagIt = m_tagIndex.find(tag);
        if (tagIt != m_tagIndex.end())
        {
            auto& entities = tagIt->second;
            entities.erase(std::remove(entities.begin(), entities.end(), entityId), entities.end());

            if (entities.empty())
            {
                m_tagIndex.erase(tagIt);
            }
        }
    }

    // Remove from all component indices
    for (auto& pair : m_componentEntityIndex)
    {
        auto& entities = pair.second;
        entities.erase(std::remove(entities.begin(), entities.end(), entityId), entities.end());
    }

    // Clean up empty component indices
    for (auto it = m_componentEntityIndex.begin(); it != m_componentEntityIndex.end(); )
    {
        if (it->second.empty())
        {
            it = m_componentEntityIndex.erase(it);
        }
        else
        {
            it++;
        }
    }
}

// Notify about entity creation
void EntityManager::notifyEntityCreated(Entity* entity)
{
    // Dispatch through event system if available
    if (m_eventSystem && entity)
    {
        // Example: m_eventSystem->dispatch<EntityCreatedEvent>(entity);
    }

    // Call registered callbacks
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    for (const auto& callback : m_entityCreatedCallbacks)
    {
        callback.callback(entity);
    }
}

// Notify about entity destruction
void EntityManager::notifyEntityDestroyed(Entity* entity)
{
    // Dispatch through event system if available
    if (m_eventSystem && entity)
    {
        // Example: m_eventSystem->dispatch<EntityDestroyedEvent>(entity);
    }

    // Call registered callbacks
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    for (const auto& callback : m_entityDestroyedCallbacks)
    {
        callback.callback(entity);
    }
}