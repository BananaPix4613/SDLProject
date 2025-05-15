// -------------------------------------------------------------------------
// EntityMetadata.h
// -------------------------------------------------------------------------
#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <set>
#include <mutex>
#include <shared_mutex>

#include "ECS/Types.h"
#include "ECS/UUID.h"

namespace PixelCraft::ECS
{

    /**
     * @brief Container for entity metadata
     * 
     * Provides centralized storage for all entity-related data including:
     * - UUIDs
     * - Names
     * - Tags
     * - Parent-child relationships
     * - Active status
     */
    class EntityMetadata
    {
    public:
        /**
         * @brief Default constructor
         */
        EntityMetadata() = default;

        /**
         * @brief Destructor
         */
        ~EntityMetadata() = default;

        /**
         * @brief Register a new entity
         * @param entity Entity ID to register
         * @param generateUUID Whether to generate a UUID for this entity
         * @thread_safety Thread-safe, acquires metadata lock
         */
        void registerEntity(EntityID entity, bool generateUUID = false);

        /**
         * @brief Remove an entity and all its metadata
         * @param entity Entity ID to remove
         * @thread_safety Thread-safe, acquires metadata lock
         */
        void removeEntity(EntityID entity);

        /**
         * @brief Clear all entity metadata
         * @thread_safety Thread-safe, acquires metadata lock
         */
        void clear();

        /**
         * @brief Set the name of an entity
         * @param entity Entity ID to name
         * @param name Name to assign
         * @return Result<void> indicating success or failure
         * @thread_safety Thread-safe, acquires metadata lock
         */
        bool setName(EntityID entity, const std::string& name);

        /**
         * @brief Get the name of an entity
         * @param entity Entity ID to get the name for
         * @return Result containing entity name or error
         * @thread_safety Thread-safe, acquires metadata lock
         */
        std::string getName(EntityID entity) const;

        /**
         * @brief Set the UUID for an entity
         * @param entity Entity ID to set the UUID for
         * @param uuid UUID to assign
         * @return Result<void> indicating success or failure
         * @thread_safety Thread-safe, acquires metadata lock
         */
        bool setUUID(EntityID entity, const UUID& uuid);

        /**
         * @brief Get the UUID for an entity
         * @param entity Entity ID to get the UUID for
         * @return Result containing UUID or error
         * @thread_safety Thread-safe, acquires shared metadata lock
         */
        UUID getUUID(EntityID entity) const;

        /**
         * @brief Get an entity by its UUID
         * @param uuid UUID to look up
         * @return Result containing entity ID or error
         * @thread_safety Thread-safe, acquires shared metadata lock
         */
        EntityID getEntityByUUID(const UUID& uuid) const;

        /**
         * @brief Find an entity by name
         * @param name Name to search for
         * @return Result containing entity ID or error
         * @thread_safety Thread-safe, acquires shared metadata lock
         */
        EntityID findEntityByName(const std::string& name) const;

        /**
         * @brief Add a tag to an entity
         * @param entity Entity ID to tag
         * @param tag Tag to add
         * @return Result<void> indicating success or failure
         * @thread_safety Thread-safe, acquires metadata lock
         */
        bool addTag(EntityID entity, const std::string& tag);

        /**
         * @brief Remove a tag from an entity
         * @param entity Entity ID to untag
         * @param tag Tag to remove
         * @return Result<void> indicating success or failure
         * @thread_safety Thread-safe, acquires metadata lock
         */
        bool removeTag(EntityID entity, const std::string& tag);

        /**
         * @brief Check if an entity has a tag
         * @param entity Entity ID to check
         * @param tag Tag to check for
         * @return Result containing boolean or error
         * @thread_safety Thread-safe, acquires shared metadata lock
         */
        bool hasTag(EntityID entity, const std::string& tag) const;

        /**
         * @brief Find entities by tag
         * @param tag Tag to search for
         * @return Vector of entity IDs with the tag
         */
        std::vector<EntityID> findEntitiesByTag(const std::string& tag) const;

        /**
         * @brief Get all tags for an entity
         * @param entity Entity ID to get tags for
         * @return Vector of tags assigned to the entity
         * @thread_safety Thread-safe, acquires shared mutex
         */
        std::vector<std::string> getAllTagsForEntity(EntityID entity) const;

        /**
         * @brief Enable or disable UUID generation for an entity
         * @param entity Entity ID
         * @param needsUUID True to enable UUID generation
         */
        void setEntityNeedsUUID(EntityID entity, bool needsUUID);

        /**
         * @brief Check if an entity has UUID generation enabled
         * @param entity Entity ID
         * @return True if UUID generation is enabled
         */
        bool entityNeedsUUID(EntityID entity) const;

        /**
         * @brief Set an entity's parent
         * @param entity Entity ID
         * @param parent Parent entity ID
         * @return Result<void> indicating success or failure
         * @thread_safety Thread-safe, acquires metadata lock
         */
        bool setParent(EntityID entity, EntityID parent);

        /**
         * @brief Get an entity's parent
         * @param entity Entity ID
         * @return Result containing parent entity ID or error
         * @thread_safety Thread-safe, acquires shared metadata lock
         */
        EntityID getParent(EntityID entity) const;

        /**
         * @brief Get an entity's children
         * @param entity Entity ID
         * @return Result containing vector of child entity IDs or error
         * @thread_safety Thread-safe, acquires shared metadata lock
         */
        std::vector<EntityID> getChildren(EntityID entity) const;

        /**
         * @brief Set an entity's active state
         * @param entity Entity ID
         * @param active True to set active, false to set inactive
         * @return Result<void> indicating success or failure
         * @thread_safety Thread-safe, acquires metadata lock
         */
        bool setActive(EntityID entity, bool active);

        /**
         * @brief Check if an entity is active
         * @param entity Entity ID
         * @return Result containing boolean or error
         * @thread_safety Thread-safe, acquires shared metadata lock
         */
        bool isActive(EntityID entity) const;

    private:
        // Thread safety
        mutable std::shared_mutex m_metadataMutex;

        // Entity name storage
        std::unordered_map<EntityID, std::string> m_entityNames;
        std::unordered_map<std::string, EntityID> m_nameToEntity;

        // UUID management
        std::unordered_map<EntityID, UUID> m_entityUUIDs;
        std::unordered_map<UUID, EntityID> m_uuidToEntity;
        std::unordered_set<EntityID> m_entitiesNeedingUUIDs;

        // Tag storage
        std::unordered_map<EntityID, std::set<std::string>> m_entityTags;
        std::unordered_map<std::string, std::set<EntityID>> m_tagToEntities;

        // Hierarchy storage
        std::unordered_map<EntityID, EntityID> m_entityParents;
        std::unordered_map<EntityID, std::set<EntityID>> m_entityChildren;

        // Active state storage
        std::set<EntityID> m_inactiveEntities; // Entities not in this set are active

        /**
         * @brief Check if an entity exists
         * @param entity Entity ID to check
         * @return True if the entity exists in metadata
         * @thread_safety Assumes metadata mutex is already locked
         */
        bool entityExists(EntityID entity) const;

        /**
         * @brief Generate a UUID for an entity
         * @param entity Entity ID to generate UUID for
         * @thread_safety Assumes metadata mutex is already locked
         */
        void generateUUID(EntityID entity);
    };

} // namespace PixelCraft::ECS