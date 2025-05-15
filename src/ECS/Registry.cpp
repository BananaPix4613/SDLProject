// -------------------------------------------------------------------------
// Registry.cpp
// -------------------------------------------------------------------------
#include "ECS/Registry.h"
#include "ECS/Entity.h"
#include "ECS/ComponentRegistry.h"
#include "Core/Logger.h"

namespace Log = PixelCraft::Core;

namespace PixelCraft::ECS
{

    Registry::Registry() : m_nextEntityID(1)
    {
        // Constructor constructor
    }

    Registry::~Registry()
    {
        // Acquire exclusive locks for cleanup
        std::unique_lock entityLock(m_entityMutex);
        std::unique_lock componentLock(m_componentMutex);

        // Clean up all entities and components
        for (EntityID entity : m_entities)
        {
            // No need to call removeAllComponents since we're cleaning up everything
            for (auto& poolPair : m_componentPools)
            {
                poolPair.second->destroyComponent(entity);
            }
        }

        m_entities.clear();
        m_entityMasks.clear();
        m_componentPools.clear();
        m_entityMetadata.clear();
    }

    EntityID Registry::createEntity(bool generateUUID)
    {
        std::unique_lock entityLock(m_entityMutex);

        EntityID entity = m_nextEntityID++;

        // Add to entity list
        m_entities.push_back(entity);

        // Initialize component mask
        m_entityMasks[entity] = ComponentMask();

        // Register in entity metadata
        m_entityMetadata.registerEntity(entity, generateUUID);

        return entity;
    }

    EntityID Registry::createEntity(const std::string& name, bool generateUUID)
    {
        // First create the entity
        EntityID entity = createEntity(generateUUID);

        // Set entity name if provided
        if (!name.empty())
        {
            setEntityName(entity, name);
        }

        return entity;
    }

    bool Registry::destroyEntity(EntityID entity)
    {
        // First validate entity
        {
            std::shared_lock entityLock(m_entityMutex);
            if (!isValid(entity))
            {
                return false;
            }
        }

        // First remove all components
        removeAllComponents(entity);

        // Then remove from entity collections
        {
            std::unique_lock entityLock(m_entityMutex);

            // Remove entity from the registry
            auto it = std::find(m_entities.begin(), m_entities.end(), entity);
            if (it != m_entities.end())
            {
                m_entities.erase(it);
            }

            m_entityMasks.erase(entity);
        }

        // Remove from entity metadata
        m_entityMetadata.removeEntity(entity);

        return true;
    }
    
    bool Registry::isValid(EntityID entity) const
    {
        std::shared_lock entityLock(m_entityMutex);
        return std::find(m_entities.begin(), m_entities.end(), entity) != m_entities.end();
    }

    Utility::Serialization::SerializationResult Registry::serialize(EntityID entity, Utility::Serialization::Serializer& serializer)
    {
        std::shared_lock entityLock(m_entityMutex);
        std::shared_lock componentLock(m_componentMutex);

        // Validate entity
        if (!isValid(entity))
        {
            Log::error("Cannot serialize invalid entity: " + std::to_string(entity));
            return;
        }

        auto result = serializer.beginObject("Entity");
        if (!result)
        {
            Log::error("Failed to begin entity serialization: " + result.error);
            return;
        }

        // Serialize entity ID
        result = serializer.writeField("id", entity);
        if (!result) return;

        // Serialize entity metadata (UUID, name, etc.)
        UUID uuid = m_entityMetadata.getUUID(entity);
        if (uuid != UUID())
        {
            result = serializer.writeField("uuid", uuid);
            if (!result) return;
        }

        std::string name = m_entityMetadata.getName(entity);
        if (!name.empty())
        {
            result = serializer.writeField("name", name);
            if (!result) return;
        }

        // Serialize components
        auto& mask = m_entityMasks.at(entity);

        // Count serializable components
        std::vector<ComponentTypeID> componentsToSerialize;
        for (const auto& [typeID, pool] : m_componentPools)
        {
            if (mask.test(typeID))
            {
                componentsToSerialize.push_back(typeID);
            }
        }

        // Serialize components array
        result = serializer.beginArray("components", componentsToSerialize.size());
        if (!result) return;

        for (ComponentTypeID typeID : componentsToSerialize)
        {
            auto it = m_componentPools.find(typeID);
            if (it != m_componentPools.end() && it->second)
            {
                void* componentData = it->second->getComponentRaw(entity);
                if (componentData)
                {
                    // Serialize component type
                    result = serializer.writeField("typeID", typeID);
                    if (!result) return;

                    // Let component pool handle serialization
                    result = it->second->serializeComponent(entity, serializer);
                    if (!result)
                    {
                        Log::warn("Failed to serialize component " +
                                           std::to_string(typeID) + ": " + result.error);
                    }
                }
            }
        }

        result = serializer.endArray();
        if (!result) return;

        return serializer.endObject();
    }

    Utility::Serialization::SerializationResult Registry::deserialize(EntityID entity, Utility::Serialization::Deserializer& deserializer)
    {
        std::unique_lock entityLock(m_entityMutex);
        std::unique_lock componentLock(m_componentMutex);

        // Validate entity exists or create it
        if (!isValid(entity))
        {
            entity = createEntity();
        }

        auto result = deserializer.beginObject("Entity");
        if (!result)
        {
            Log::error("Failed to begin entity deserialization: " + result.error);
            return;
        }

        // Read UUID if present
        if (deserializer.hasField("uuid"))
        {
            UUID uuid;
            result = deserializer.readField("uuid", uuid);
            if (result)
            {
                m_entityMetadata.setUUID(entity, uuid);
            }
        }

        // Read name if present
        if (deserializer.hasField("name"))
        {
            std::string name;
            result = deserializer.readField("name", name);
            if (result)
            {
                m_entityMetadata.setName(entity, name);
            }
        }

        // Read components if present
        if (deserializer.hasField("components"))
        {
            size_t componentCount = 0;
            result = deserializer.beginArray("components", componentCount);
            if (!result) return;

            for (size_t i = 0; i < componentCount; ++i)
            {
                // Read component type ID
                ComponentTypeID typeID;
                result = deserializer.readField("typeID", typeID);
                if (!result)
                {
                    deserializer.skipValue();
                    continue;
                }

                // Find or create component pool
                auto it = m_componentPools.find(typeID);
                if (it == m_componentPools.end() || !it->second)
                {
                    // Try to create pool using registry
                    auto pool = ComponentRegistry::getInstance().createPool(typeID);
                    if (!pool)
                    {
                        Log::error("Unknown component type: " + std::to_string(typeID));
                        deserializer.skipValue();
                        continue;
                    }
                    m_componentPools[typeID] = pool;
                    it = m_componentPools.find(typeID);
                }

                // Deserialize component
                result = it->second->deserializeComponent(entity, deserializer);
                if (result)
                {
                    // Update component mask
                    m_entityMasks[entity].set(typeID);
                }
                else
                {
                    Log::warn("Failed to deserialize component " +
                                       std::to_string(typeID) + ": " + result.error);
                }
            }

            result = deserializer.endArray();
        }

        return deserializer.endObject();
    }

    Utility::Serialization::SerializationResult Registry::serializeAll(Utility::Serialization::Serializer& serializer)
    {
        std::shared_lock entityLock(m_entityMutex);

        auto result = serializer.beginObject("Registry");
        if (!result) return;

        // Serialize entities count
        result = serializer.writeField("entityCount", m_entities.size());
        if (!result) return;

        // Serialize entities array
        result = serializer.beginArray("entities", m_entities.size());
        if (!result) return;

        for (EntityID entity : m_entities)
        {
            if (isValid(entity))
            {
                // Release locks during individual entity serialization
                entityLock.unlock();
                serialize(entity, serializer);
                entityLock.lock();
            }
        }

        serializer.endArray();
        return serializer.endObject();
    }

    Utility::Serialization::SerializationResult Registry::deserializeAll(Utility::Serialization::Deserializer& deserializer)
    {
        auto result = deserializer.beginObject("Registry");
        if (!result) return;

        // Read entity count
        size_t entityCount = 0;
        result = deserializer.readField("entityCount", entityCount);

        // Read entities array
        if (deserializer.hasField("entities"))
        {
            size_t arraySize = 0;
            result = deserializer.beginArray("entities", arraySize);
            if (!result) return;

            for (size_t i = 0; i < arraySize; ++i)
            {
                // Create a new entity for each array entry
                EntityID entity = createEntity();

                // Deserialize entity data
                deserialize(entity, deserializer);
            }

            deserializer.endArray();
        }

        return deserializer.endObject();
    }

    UUID Registry::getEntityUUID(EntityID entity) const
    {
        return m_entityMetadata.getUUID(entity);
    }

    void Registry::setEntityUUID(EntityID entity, const UUID& uuid)
    {
        m_entityMetadata.setUUID(entity, uuid);
    }

    EntityID Registry::getEntityByUUID(const UUID& uuid) const
    {
        return m_entityMetadata.getEntityByUUID(uuid);
    }

    void Registry::setEntityName(EntityID entity, const std::string& name)
    {
        m_entityMetadata.setName(entity, name);
    }

    std::string Registry::getEntityName(EntityID entity) const
    {
        return m_entityMetadata.getName(entity);
    }

    EntityID Registry::findEntityByName(const std::string& name) const
    {
        return m_entityMetadata.findEntityByName(name);
    }

    void Registry::addTag(EntityID entity, const std::string& tag)
    {
        m_entityMetadata.addTag(entity, tag);
    }

    void Registry::removeTag(EntityID entity, const std::string& tag)
    {
        m_entityMetadata.removeTag(entity, tag);
    }

    bool Registry::hasTag(EntityID entity, const std::string& tag) const
    {
        return m_entityMetadata.hasTag(entity, tag);
    }

    std::vector<EntityID> Registry::findEntitiesByTag(const std::string& tag) const
    {
        return m_entityMetadata.findEntitiesByTag(tag);
    }

    void Registry::setEntityNeedsUUID(EntityID entity, bool needsUUID)
    {
        m_entityMetadata.setEntityNeedsUUID(entity, needsUUID);
    }

    bool Registry::entityNeedsUUID(EntityID entity) const
    {
        return m_entityMetadata.entityNeedsUUID(entity);
    }

    bool Registry::setEntityParent(EntityID entity, EntityID parent)
    {
        return m_entityMetadata.setParent(entity, parent);
    }

    EntityID Registry::getEntityParent(EntityID entity) const
    {
        return m_entityMetadata.getParent(entity);
    }

    std::vector<EntityID> Registry::getEntityChildren(EntityID entity) const
    {
        return m_entityMetadata.getChildren(entity);
    }

    bool Registry::setEntityActive(EntityID entity, bool active)
    {
        return m_entityMetadata.setActive(entity, active);
    }

    bool Registry::isEntityActive(EntityID entity) const
    {
        return m_entityMetadata.isActive(entity);
    }

    EntityMetadata& Registry::getEntityMetadata()
    {
        return m_entityMetadata;
    }

    const EntityMetadata& Registry::getEntityMetadata() const
    {
        return m_entityMetadata;
    }

    const std::vector<EntityID>& Registry::getEntities() const
    {
        std::shared_lock entityLock(m_entityMutex);
        return m_entities;
    }

    const ComponentMask& Registry::getEntityMask(EntityID entity) const
    {
        std::shared_lock componentLock(m_componentMutex);

        static ComponentMask emptyMask;
        auto it = m_entityMasks.find(entity);
        if (it == m_entityMasks.end())
        {
            return emptyMask;
        }
        return it->second;
    }

    const std::map<ComponentTypeID, std::shared_ptr<IComponentPool>>& Registry::getAllComponentPools() const
    {
        std::shared_lock componentLock(m_componentMutex);
        return m_componentPools;
    }

    size_t Registry::getEntityCount() const
    {
        std::shared_lock entityLock(m_entityMutex);
        return m_entities.size();
    }

    void* Registry::getComponentRaw(EntityID entity, ComponentTypeID typeID)
    {
        std::shared_lock componentLock(m_componentMutex);

        auto it = m_componentPools.find(typeID);
        if (it == m_componentPools.end())
        {
            return nullptr;
        }

        auto& pool = it->second;

        try
        {
            return pool->getComponentRaw(entity);
        }
        catch (const std::out_of_range&)
        {
            return nullptr;
        }
    }

    void Registry::removeAllComponents(EntityID entity)
    {
        // First validate entity
        {
            std::shared_lock entityLock(m_entityMutex);
            if (!isValid(entity))
            {
                return;
            }
        }

        // Acquire exclusive lock for component operations
        std::unique_lock componentLock(m_componentMutex);

        // Get the entity's component mask
        auto maskIt = m_entityMasks.find(entity);
        if (maskIt == m_entityMasks.end())
        {
            return;
        }

        ComponentMask& mask = maskIt->second;

        // Iterate through all component types
        for (ComponentTypeID typeID = 0; typeID < MAX_COMPONENT_TYPES; ++typeID)
        {
            if (mask[typeID])
            {
                // Get the component pool
                auto it = m_componentPools.find(typeID);
                if (it != m_componentPools.end())
                {
                    // Remove the component
                    it->second->destroyComponent(entity);
                }
            }
        }

        // Clear the component mask
        mask.reset();
    }

} // namespace PixelCraft::ECS