// -------------------------------------------------------------------------
// EntityMetadata.cpp
// -------------------------------------------------------------------------
#include "ECS/EntityMetadata.h"
#include "Core/Logger.h"

namespace Log = PixelCraft::Core;

namespace PixelCraft::ECS
{

    void EntityMetadata::registerEntity(EntityID entity, bool generateUUID)
    {
        std::unique_lock lock(m_metadataMutex);

        // Add entity to metadata storage
        m_entityNames[entity] = ""; // Empty name by default

        // Generate UUID if requested
        if (generateUUID)
        {
            this->generateUUID(entity);
        }
        else
        {
            // Just mark that this entity should have a UUID generated when needed
            m_entitiesNeedingUUIDs.insert(entity);
        }
    }

    void EntityMetadata::removeEntity(EntityID entity)
    {
        std::unique_lock lock(m_metadataMutex);

        if (!entityExists(entity))
        {
            return;
        }

        // Remove from name storage
        const std::string& name = m_entityNames[entity];
        if (!name.empty())
        {
            m_nameToEntity.erase(name);
        }
        m_entityNames.erase(entity);

        // Remove from UUID storage
        auto uuidIt = m_entityUUIDs.find(entity);
        if (uuidIt != m_entityUUIDs.end())
        {
            m_uuidToEntity.erase(uuidIt->second);
            m_entityUUIDs.erase(uuidIt);
        }
        m_entitiesNeedingUUIDs.erase(entity);

        // Remove from tag storage
        auto tagsIt = m_entityTags.find(entity);
        if (tagsIt != m_entityTags.end())
        {
            for (const auto& tag : m_entityTags[entity])
            {
                m_tagToEntities[tag].erase(entity);
                if (m_tagToEntities[tag].empty())
                {
                    m_tagToEntities.erase(tag);
                }
            }
            m_entityTags.erase(entity);
        }

        // Remove from hierarchy storage
        auto parentIt = m_entityParents.find(entity);
        if (parentIt != m_entityParents.end())
        {
            EntityID parent = parentIt->second;
            m_entityChildren[parent].erase(entity);
            if (m_entityChildren[parent].empty())
            {
                m_entityChildren.erase(parent);
            }

            m_entityParents.erase(entity);
        }

        auto childrenIt = m_entityChildren.find(entity);
        if (childrenIt != m_entityChildren.end())
        {
            for (EntityID child : childrenIt->second)
            {
                m_entityParents.erase(child);
            }
            m_entityChildren.erase(entity);
        }

        // Remove from active state storage
        m_inactiveEntities.erase(entity);
    }

    void EntityMetadata::clear()
    {
        std::unique_lock lock(m_metadataMutex);

        m_entityNames.clear();
        m_nameToEntity.clear();
        m_entityUUIDs.clear();
        m_uuidToEntity.clear();
        m_entitiesNeedingUUIDs.clear();
        m_entityTags.clear();
        m_tagToEntities.clear();
        m_entityParents.clear();
        m_entityChildren.clear();
        m_inactiveEntities.clear();
    }

    bool EntityMetadata::setName(EntityID entity, const std::string& name)
    {
        if (name.empty())
        {
            return false;
        }

        std::unique_lock lock(m_metadataMutex);

        if (!entityExists(entity))
        {
            return false;
        }

        // Check if name is already in use by another entity
        auto nameIt = m_nameToEntity.find(name);
        if (nameIt != m_nameToEntity.end() && nameIt->second != entity)
        {
            return false;
        }

        // Remove old name mapping if present
        const std::string& oldName = m_entityNames[entity];
        if (!oldName.empty())
        {
            m_nameToEntity.erase(oldName);
        }

        // Set new name
        m_entityNames[entity] = name;
        m_nameToEntity[name] = entity;

        return true;
    }

    std::string EntityMetadata::getName(EntityID entity) const
    {
        std::shared_lock lock(m_metadataMutex);

        auto it = m_entityNames.find(entity);
        if (it == m_entityNames.end())
        {
            return "";
        }

        return it->second;
    }

    bool EntityMetadata::setUUID(EntityID entity, const UUID& uuid)
    {
        if (uuid.isNull())
        {
            return false;
        }

        std::unique_lock lock(m_metadataMutex);

        if (!entityExists(entity))
        {
            return false;
        }

        // Check if UUID is already in use by another entity
        auto uuidIt = m_uuidToEntity.find(uuid);
        if (uuidIt != m_uuidToEntity.end() && uuidIt->second != entity)
        {
            return false;
        }

        // Remove old UUID mapping if present
        auto oldUuidIt = m_entityUUIDs.find(entity);
        if (oldUuidIt != m_entityUUIDs.end())
        {
            m_uuidToEntity.erase(oldUuidIt->second);
        }

        // Remove entity needing UUID if present
        auto it = m_entitiesNeedingUUIDs.find(entity);
        if (it != m_entitiesNeedingUUIDs.end())
        {
            m_entitiesNeedingUUIDs.erase(entity);
        }

        // Add new UUID
        m_entityUUIDs[entity] = uuid;
        m_uuidToEntity[uuid] = entity;

        return true;
    }

    UUID EntityMetadata::getUUID(EntityID entity) const
    {
        std::shared_lock lock(m_metadataMutex);

        if (!entityExists(entity))
        {
            return UUID::createNull();
        }

        auto it = m_entityUUIDs.find(entity);
        if (it == m_entityUUIDs.end())
        {
            return UUID::createNull();
        }

        return it->second;
    }

    EntityID EntityMetadata::getEntityByUUID(const UUID& uuid) const
    {
        if (uuid.isNull())
        {
            return INVALID_ENTITY_ID;
        }

        std::shared_lock lock(m_metadataMutex);

        auto it = m_uuidToEntity.find(uuid);
        if (it == m_uuidToEntity.end())
        {
            return INVALID_ENTITY_ID;
        }

        return it->second;
    }

    EntityID EntityMetadata::findEntityByName(const std::string& name) const
    {
        if (name.empty())
        {
            return INVALID_ENTITY_ID;
        }

        std::shared_lock lock(m_metadataMutex);

        auto it = m_nameToEntity.find(name);
        if (it == m_nameToEntity.end())
        {
            return INVALID_ENTITY_ID;
        }

        return it->second;
    }

    bool EntityMetadata::addTag(EntityID entity, const std::string& tag)
    {
        if (tag.empty())
        {
            return false;
        }

        std::unique_lock lock(m_metadataMutex);

        if (!entityExists(entity))
        {
            return false;
        }

        // Add tag to entity
        m_entityTags[entity].insert(tag);

        // Add entity to tag
        m_tagToEntities[tag].insert(entity);

        return true;
    }

    bool EntityMetadata::removeTag(EntityID entity, const std::string& tag)
    {
        if (tag.empty())
        {
            return false;
        }

        std::unique_lock lock(m_metadataMutex);

        if (!entityExists(entity))
        {
            return false;
        }

        auto tagIt = m_entityTags.find(entity);
        if (tagIt == m_entityTags.end() || tagIt->second.find(tag) == tagIt->second.end())
        {
            return false;
        }

        // Remove tag from entity
        tagIt->second.erase(tag);
        if (tagIt->second.empty())
        {
            m_entityTags.erase(entity);
        }

        // Remove entity from tag
        m_tagToEntities[tag].erase(entity);
        if (m_tagToEntities[tag].empty())
        {
            m_tagToEntities.erase(tag);
        }

        return true;
    }

    bool EntityMetadata::hasTag(EntityID entity, const std::string& tag) const
    {
        if (tag.empty())
        {
            return false;
        }

        std::shared_lock lock(m_metadataMutex);

        if (!entityExists(entity))
        {
            return false;
        }

        auto tagIt = m_entityTags.find(entity);
        if (tagIt == m_entityTags.end())
        {
            return false;
        }

        return tagIt->second.find(tag) != tagIt->second.end();
    }

    std::vector<EntityID> EntityMetadata::findEntitiesByTag(const std::string& tag) const
    {
        std::shared_lock lock(m_metadataMutex);

        std::vector<EntityID> result;
        auto it = m_tagToEntities.find(tag);
        if (it != m_tagToEntities.end())
        {
            result.insert(result.end(), it->second.begin(), it->second.end());
        }

        return result;
    }

    std::vector<std::string> EntityMetadata::getAllTagsForEntity(EntityID entity) const
    {
        std::shared_lock lock(m_metadataMutex);

        std::vector<std::string> result;
        auto it = m_entityTags.find(entity);
        if (it != m_entityTags.end())
        {
            result.insert(result.end(), it->second.begin(), it->second.end());
        }

        return result;
    }

    void EntityMetadata::setEntityNeedsUUID(EntityID entity, bool needsUUID)
    {
        std::unique_lock lock(m_metadataMutex);

        if (!entityExists(entity))
        {
            return;
        }

        if (needsUUID)
        {
            m_entitiesNeedingUUIDs.insert(entity);

            // Generate UUID if not already present
            if (m_entityUUIDs.find(entity) == m_entityUUIDs.end())
            {
                generateUUID(entity);
            }
        }
        else
        {
            m_entitiesNeedingUUIDs.erase(entity);
        }
    }

    bool EntityMetadata::entityNeedsUUID(EntityID entity) const
    {
        std::shared_lock lock(m_metadataMutex);

        if (!entityExists(entity))
        {
            return false;
        }

        return m_entitiesNeedingUUIDs.find(entity) != m_entitiesNeedingUUIDs.end();
    }

    bool EntityMetadata::setParent(EntityID entity, EntityID parent)
    {
        std::unique_lock lock(m_metadataMutex);

        if (!entityExists(entity))
        {
            return false;
        }

        if (parent != INVALID_ENTITY_ID && !entityExists(parent))
        {
            return false;
        }

        // Prevent circular hierarchies
        if (parent != INVALID_ENTITY_ID)
        {
            EntityID current = parent;
            while (current != INVALID_ENTITY_ID)
            {
                if (current == entity)
                {
                    return false;
                }

                auto it = m_entityParents.find(current);
                if (it == m_entityParents.end())
                {
                    break;
                }

                current = it->second;
            }
        }

        // Remove from old parent
        auto oldParentIt = m_entityParents.find(entity);
        if (oldParentIt != m_entityParents.end())
        {
            EntityID oldParent = oldParentIt->second;
            m_entityChildren[oldParent].erase(entity);
            if (m_entityChildren[oldParent].empty())
            {
                m_entityChildren.erase(oldParent);
            }
        }

        // Set new parent
        if (parent != INVALID_ENTITY_ID)
        {
            m_entityParents[entity] = parent;
            m_entityChildren[parent].insert(entity);
        }
        else
        {
            m_entityParents.erase(entity);
        }

        return true;
    }

    EntityID EntityMetadata::getParent(EntityID entity) const
    {
        std::shared_lock lock(m_metadataMutex);

        if (!entityExists(entity))
        {
            return INVALID_ENTITY_ID;
        }

        auto it = m_entityParents.find(entity);
        if (it == m_entityParents.end())
        {
            return INVALID_ENTITY_ID;
        }

        return it->second;
    }

    std::vector<EntityID> EntityMetadata::getChildren(EntityID entity) const
    {
        std::shared_lock lock(m_metadataMutex);

        if (!entityExists(entity))
        {
            return {};
        }

        std::vector<EntityID> result;
        auto it = m_entityChildren.find(entity);
        if (it != m_entityChildren.end())
        {
            result.insert(result.end(), it->second.begin(), it->second.end());
        }

        return result;
    }

    bool EntityMetadata::setActive(EntityID entity, bool active)
    {
        std::unique_lock lock(m_metadataMutex);

        if (!entityExists(entity))
        {
            return false;
        }

        if (active)
        {
            m_inactiveEntities.erase(entity);
        }
        else
        {
            m_inactiveEntities.insert(entity);
        }

        return true;
    }

    bool EntityMetadata::isActive(EntityID entity) const
    {
        std::shared_lock lock(m_metadataMutex);

        if (!entityExists(entity))
        {
            return false;
        }

        return m_inactiveEntities.find(entity) == m_inactiveEntities.end();
    }

    bool EntityMetadata::entityExists(EntityID entity) const
    {
        // Assumes metadata mutex is already locked
        return m_entityNames.find(entity) != m_entityNames.end();
    }

    void EntityMetadata::generateUUID(EntityID entity)
    {
        // Assumes metadata mutex is already locked

        // Generate a random UUID
        UUID uuid = UUID::createRandom();

        // Ensure it's unique
        while (m_uuidToEntity.find(uuid) != m_uuidToEntity.end())
        {
            uuid = UUID::createRandom();
        }

        // Store the UUID
        m_entityUUIDs[entity] = uuid;
        m_uuidToEntity[uuid] = entity;
    }

} // namespace PixelCraft::ECS