// -------------------------------------------------------------------------
// Scene.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>

#include "ECS/Types.h"
#include "Voxel/Chunk.h"

namespace PixelCraft::ECS
{

    // Forward declarations
    class World;
    class Entity;

    /**
     * @brief Scene class manages entity hierarchies and serialization in an ECS architecture
     *
     * The Scene is responsible for:
     * - Storing and managing a collection of entities
     * - Loading and saving scene data
     * - Handling prefab instantiation for entity templates
     * - Tracking important entities like the main camera
     * - Implementing chunk-based serialization for voxel worlds
     * - Supporting FlatBuffers for efficient binary serialization
     * - Enabling efficient tracking and saving of modified chunks
     */
    class Scene : public std::enable_shared_from_this<Scene>
    {
    public:
        /**
         * @brief Constructor
         * @param name Optional name for the scene
         */
        Scene(const std::string& name = "");

        /**
         * @brief Destructor
         */
        ~Scene();

        /**
         * @brief Load a scene from a file
         * @param path Path to the scene file
         * @return True if the scene was loaded successfully
         */
        bool load(const std::string& path);

        /**
         * @brief Save a scene to a file
         * @param path Path where the scene will be saved
         * @return True if the scene was saved successfully
         */
        bool save(const std::string& path);

        /**
         * @brief Instantiate a prefab as an entity in the scene
         * @param prefabPath Path to the prefab file
         * @return ID of the instantiated entity
         */
        EntityID instantiate(const std::string& prefabPath);

        /**
         * @brief Find an entity by its name
         * @param name Name of the entity to find
         * @return ID of the found entity, or INVALID_ENTITY_ID if not found
         */
        EntityID findEntityByName(const std::string& name);

        /**
         * @brief Find all entities with a specific tag
         * @param tag Tag to search for
         * @return Vector of entity IDs with the matching tag
         */
        std::vector<EntityID> findEntitiesByTag(const std::string& tag);

        /**
         * @brief Get the main camrea entity
         * @return ID of the main camera entity
         */
        EntityID getMainCamera() const
        {
            return m_mainCamera;
        }

        /**
         * @brief Set the main camera entity
         * @param cameraEntity ID of the entity to set as the main camera
         */
        void setMainCamera(EntityID cameraEntity);

        /**
         * @brief Get the scene's name
         * @return The name of the scene
         */
        const std::string& getName() const
        {
            return m_name;
        }

        /**
         * @brief Set the scene's name
         * @param name The name to set
         */
        void setName(const std::string& name)
        {
            m_name = name;
        }

        /**
         * @brief Get the scene's file path
         * @return The path of the scene file
         */
        const std::string& getPath() const
        {
            return m_path;
        }

        /**
         * @brief Set the scene's file path
         * @param path The path to set
         */
        void setPath(const std::string& path)
        {
            m_path = path;
        }

        /**
         * @brief Serialize the scene to a FlatBuffer
         * @return Vector of bytes containing the serialized scene
         */
        std::vector<uint8_t> serializeToFlatBuffer();

        /**
         * @brief Deserialize the scene from a FlatBuffer
         * @param data Pointer to the serialized data
         * @param size Size of the serialized data in bytes
         * @return True if deserialization was successful
         */
        bool deserializeFromFlatBuffer(const uint8_t* data, size_t size);

        /**
         * @brief Serialize a specific entity to a FlatBuffer
         * @param entity Entity ID to serialize
         * @return Vector of bytes containing the serialized entity
         */
        std::vector<uint8_t> serializeEntityToFlatBuffer(EntityID entity);

        /**
         * @brief Serialize a specific chunk
         * @param coord Coordinates of the chunk to serialize
         * @return True if the chunk was serialized successfully
         */
        bool serializeChunk(const Voxel::ChunkCoord& coord);

        /**
         * @brief Deserialize a specific chunk
         * @param coord Coordinates of the chunk to deserialize
         * @return True if the chunk was deserialized successfully
         */
        bool deserializeChunk(Voxel::ChunkCoord& coord);

        /**
         * @brief Mark a chunk as modified (dirty)
         * @param coord Coordinates of the chunk to mark
         */
        void markChunkDirty(const Voxel::ChunkCoord& coord);

        /**
         * @brief Check if a chunk is marked as modified
         * @param coord Coordinates of the chunk to check
         * @return True if the chunk is marked as dirty
         */
        bool isChunkDirty(const Voxel::ChunkCoord& coord) const;

        /**
         * @brief Save all modified chunks
         */
        void saveModifiedChunks();

        /**
         * @brief Get the root entities (entities without parents)
         * @return Vector of root entity IDs
         */
        const std::vector<EntityID>& getRootEntities() const
        {
            return m_rootEntities;
        }

        /**
         * @brief Get the schema version used for serialization
         * @return The schema version number
         */
        uint32_t getSchemaVersion() const
        {
            return m_schemaVersion;
        }

        /**
         * @brief Set the schema version for serialization
         * @param version The schema version number to set
         */
        void setSchemaVersion(uint32_t version)
        {
            m_schemaVersion = version;
        }

        /**
         * @brief Set the world reference for this scene
         * @param world Weak pointer to the world
         */
        void setWorld(std::weak_ptr<World> world)
        {
            m_world = world;
        }

        /**
         * @brief Get the world this scene belongs to
         * @return Shared poitner to the world
         */
        std::shared_ptr<World> getWorld() const
        {
            return m_world.lock();
        }

        /**
         * @brief Add an entity to the scene
         * @param entity Entity to add
         */
        void addEntity(EntityID entity);

        /**
         * @brief Remove an entity from the scene
         * @param entity Entity to remove
         * @return True if the entity was removed
         */
        bool removeEntity(EntityID entity);

        /**
         * @brief Get a chunk by its coordinates
         * @param coord Coordinates of the chunk
         * @param createIfMissing If true, create the chunk if it doesn't exist
         * @return Shared pointer to the chunk, or nullptr if not found
         */
        std::shared_ptr<Voxel::Chunk> getChunk(const Voxel::ChunkCoord& coord, bool createIfMissing = false);

        /**
         * @brief Add a tag to an entity
         * @param entity Entity to add the tag to
         * @param tag Tag to add
         */
        void addTag(EntityID entity, const std::string& tag);

        /**
         * @brief Remove a tag from an entity
         * @param entity Entity to remove the tag from
         * @param tag Tag to remove
         */
        void removeTag(EntityID entity, const std::string& tag);

        /**
         * @brief Check if an entity has a specific tag
         * @param entity Entity to check
         * @param tag Tag to look for
         * @return True if the entity has the tag
         */
        bool hasTag(EntityID entity, const std::string& tag) const;

    private:
        // Scene data
        std::string m_name;                                  ///< Scene name
        std::string m_path;                                  ///< Scene file path
        std::map<std::string, EntityID> m_entitiesByName;    ///< Mapping of entity names to IDs
        std::map<std::string, std::vector<EntityID>> m_entitiesByTag; ///< Mapping of tags to entity IDs
        EntityID m_mainCamera;                               ///< Main camera entity ID
        std::vector<EntityID> m_rootEntities;                ///< Root-level entities (no parent)

        // Chunk data
        std::unordered_map<Voxel::ChunkCoord, std::shared_ptr<Voxel::Chunk>> m_chunks; ///< Map of chunk coordinates to chunks
        std::set<Voxel::ChunkCoord> m_dirtyChunks;                  ///< Set of dirty chunk coordinates

        // Version information
        uint32_t m_schemaVersion;                            ///< Schema version for backward compatibility

        // World reference
        std::weak_ptr<World> m_world;                        ///< Weak reference to the world this scene belongs to

        // Helper methods
        /**
         * @brief Load entities from a file
         * @param path Path to the entities file
         * @return True if entities were loaded successfully
         */
        bool loadEntities(const std::string& path);

        /**
         * @brief Save entities to a file
         * @param path Path to save the entities to
         * @return True if entities were saved successfully
         */
        bool saveEntities(const std::string& path);

        /**
         * @brief Load chunks from a file or directory
         * @param path Path to the chunks file or directory
         * @return True if chunks were loaded successfully
         */
        bool loadChunks(const std::string& path);

        /**
         * @brief Save chunks to files
         * @param path Path to save the chunks to
         * @return True if chunks were saved successfully
         */
        bool saveChunks(const std::string& path);

        /**
         * @brief Get the file path for a specific chunk
         * @param coord Coordinates of the chunk
         * @return File path for the chunk
         */
        std::string getChunkPath(const Voxel::ChunkCoord& coord) const;

        /**
         * @brief Serialize an entity to a serialize
         * @param entity Entity to serialize
         * @param serializer Serializer to use
         */
        void serializeEntity(EntityID entity, Serializer& serializer);

        /**
         * @brief Deserialize an entity from a deserializer
         * @param deserializer Deserializer to use
         * @return ID of the deserialized entity
         */
        EntityID deserializeEntity(Deserializer& deserializer);

        /**
         * @brief Add an entity to the entity name map
         * @param entity Entity ID
         * @param name Entity name
         */
        void registerEntityName(EntityID entity, const std::string& name);

        /**
         * @brief Remove an entity from the entity name map
         * @param entity Entity ID
         * @param name Entity name
         */
        void unregisterEntityName(EntityID entity, const std::string& name);

        /**
         * @brief Update chunk mesh and neighboring chunks
         * @param coord Coordinates of the chunk to update
         */
        void updateChunkMesh(const Voxel::ChunkCoord& coord);

        /**
         * @brief Generate a scene directory path from the scene path
         * @return Directory path for scene data
         */
        std::string getSceneDirectory() const;

        /**
         * @brief Ensure the scene directory exists
         * @return True if directory exists or was created successfully
         */
        bool ensureSceneDirectoryExists() const;
    };

} // namespace PixelCraft::ECS