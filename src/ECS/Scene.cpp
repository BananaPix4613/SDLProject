// -------------------------------------------------------------------------
// Scene.cpp
// -------------------------------------------------------------------------
#include "ECS/Scene.h"
#include "ECS/World.h"
#include "ECS/Entity.h"
#include "Core/Logger.h"
#include "Utility/FileSystem.h"
#include "Utility/StringUtils.h"
#include "Utility/Serialization/SerializationUtility.h"

// Components
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/MeshRendererComponent.h"

#include <filesystem>
#include <fstream>
#include <sstream>

using fs = PixelCraft::Utility::FileSystem;
using StringUtils = PixelCraft::Utility::StringUtils;

namespace PixelCraft::ECS
{

    Scene::Scene(const std::string& name)
        : m_name(name)
        , m_mainCamera(INVALID_ENTITY_ID)
    {
        Core::info("Created scene: " + name);
    }

    Scene::~Scene()
    {
        // Save any unsaved data
        saveChunks();

        Core::info("Destroyed scene: " + m_name);
    }

    bool Scene::load(const std::string& path)
    {
        m_path = path;
        Core::info("Loading scene from: " + path);

        if (!fs::fileExists(path))
        {
            Core::error("Scene file does not exist: " + path);
            return false;
        }

        Utility::Serialization::SerializationUtility::registerSchemaIfNeeded<Scene>();

        Scene tempScene(fs::getFileNameWithoutExtension(path));
        tempScene.setWorld(m_world);

        auto result = Utility::Serialization::SerializationUtility::deserializeFromFile(tempScene, path);
        if (!result)
        {
            Core::error("Failed to load scene: " + result.error);
            return false;
        }

        // Copy data from temp scene
        m_name = tempScene.m_name;
        m_path = tempScene.m_path;
        m_entitiesByName = tempScene.m_entitiesByName;
        m_entitiesByTag = tempScene.m_entitiesByTag;
        m_mainCamera = tempScene.m_mainCamera;
        m_rootEntities = tempScene.m_rootEntities;
        m_dirtyChunks = tempScene.m_dirtyChunks;

        // Load chunks separately (they're stored in individual files)
        loadChunks();

        return true;
    }

    bool Scene::save(const std::string& path)
    {
        if (path.empty())
        {
            if (m_path.empty())
            {
                Core::error("Cannot save scene: No path specified");
                return false;
            }
        }
        else
        {
            m_path = path;
        }

        Core::info("Saving scene to: " + m_path);

        if (!ensureSceneDirectoryExists())
        {
            Core::error("Failed to create scene directory for: " + m_path);
            return false;
        }

        Utility::Serialization::SerializationUtility::registerSchemaIfNeeded<Scene>();

        auto result = Utility::Serialization::SerializationUtility::serializeToFile(*this, m_path);
        if (!result)
        {
            Core::error("Failed to save scene: " + result.error);
            return false;
        }

        // Save modified chunks individually (these are still stored separately)
        saveChunks();

        return true;
    }

    EntityID Scene::instantiate(const std::string& prefabPath)
    {
        auto world = m_world.lock();
        if (!world)
        {
            Core::error("Cannot instantiate prefab: No world reference");
            return INVALID_ENTITY_ID;
        }

        Core::info("Instantiating prefab: " + prefabPath);

        if (!fs::fileExists(prefabPath))
        {
            Core::error("Prefab file does not exist: " + prefabPath);
            return INVALID_ENTITY_ID;
        }

        // Create a new entity
        EntityID entityId = world->createEntity(fs::getFileNameWithoutExtension(prefabPath));
        if (entityId == INVALID_ENTITY_ID)
        {
            Core::error("Failed to create entity for prefab: " + prefabPath);
            return INVALID_ENTITY_ID;
        }

        Entity entity(entityId, world->getRegistry());

        // Use serialization to load the prefab
        auto result = Utility::Serialization::SerializationUtility::deserializeFromFile(entity, prefabPath);
        if (!result)
        {
            Core::error("Failed to deserialize prefab: " + result.error);
            world->destroyEntity(entityId);
            return INVALID_ENTITY_ID;
        }

        // Add the entity to the scene
        addEntity(entityId);
        return entityId;
    }

    EntityID Scene::findEntityByName(const std::string& name)
    {
        auto it = m_entitiesByName.find(name);
        if (it != m_entitiesByName.end())
        {
            return it->second;
        }
        return INVALID_ENTITY_ID;
    }

    std::vector<EntityID> Scene::findEntitiesByTag(const std::string& tag)
    {
        auto it = m_entitiesByTag.find(tag);
        if (it != m_entitiesByTag.end())
        {
            return it->second;
        }
        return std::vector<EntityID>();
    }

    void Scene::setMainCamera(EntityID cameraEntity)
    {
        // Check if the entity exists in our world
        auto world = m_world.lock();
        if (world)
        {
            Entity entity(cameraEntity, world->getRegistry());
            if (entity.isValid())
            {
                m_mainCamera = cameraEntity;
                Core::info("Set main camera to entity: " + entity.getName());
            }
            else
            {
                Core::warn("Cannot set main camera: Invalid entity ID");
            }
        }
        else
        {
            Core::warn("Cannot set main camera: No world reference");
        }
    }

    bool Scene::serializeChunk(const Voxel::ChunkCoord& coord)
    {
        auto it = m_chunks.find(coord);
        if (it == m_chunks.end())
        {
            Core::warn(StringUtils::format("Cannot serialize chunk: Chunk not found at coordinates ({}, {}, {})",
                                          coord.x, coord.y, coord.z));
            return false;
        }

        if (!ensureSceneDirectoryExists())
        {
            Core::error("Failed to create scene directory for chunk serialization");
            return false;
        }

        // Get the path for this chunk
        std::string chunkPath = getChunkPath(coord);

        // Use the modern serialization method directly
        auto result = Utility::Serialization::SerializationUtility::serializeToFile(*(it->second), chunkPath);

        if (!result)
        {
            Core::error("Failed to serialize chunk: " + result.error);
            return false;
        }

        // Remove from dirty chunks if successful
        m_dirtyChunks.erase(coord);

        Core::debug(StringUtils::format("Serialized chunk at ({}, {}, {}) to {}",
                                       coord.x, coord.y, coord.z, chunkPath));

        return true;
    }

    bool Scene::deserializeChunk(Voxel::ChunkCoord& coord)
    {
        auto world = m_world.lock();
        if (!world)
        {
            Core::error("Cannot deserialize chunk: No world reference");
            return false;
        }

        std::string chunkPath = getChunkPath(coord);
        if (!fs::fileExists(chunkPath))
        {
            Core::warn("Chunk file does not exist: " + chunkPath);
            return false;
        }

        // Create or get the chunk
        auto chunk = getChunk(coord, true);
        if (!chunk)
        {
            Core::error("Failed to create chunk for deserialization");
            return false;
        }

        // Use the modern serialization method directly
        auto result = Utility::Serialization::SerializationUtility::deserializeFromFile(*chunk, chunkPath);

        if (!result)
        {
            Core::error("Failed to deserialize chunk: " + result.error);
            return false;
        }

        // Remove from dirty chunks
        m_dirtyChunks.erase(coord);

        Core::debug(StringUtils::format("Deserialized chunk at ({}, {}, {}) from {}",
                                       coord.x, coord.y, coord.z, chunkPath));

        return true;
    }

    void Scene::markChunkDirty(const Voxel::ChunkCoord& coord)
    {
        m_dirtyChunks.insert(coord);

        // Also mark the chunk as dirty internally
        auto it = m_chunks.find(coord);
        if (it != m_chunks.end())
        {
            it->second->markDirty();
        }
    }

    bool Scene::isChunkDirty(const Voxel::ChunkCoord& coord) const
    {
        return m_dirtyChunks.find(coord) != m_dirtyChunks.end();
    }

    void Scene::addEntity(EntityID entity)
    {
        auto world = m_world.lock();
        if (!world)
        {
            Core::error("Cannot add entity to scene: No world reference");
            return;
        }

        Entity e(entity, world->getRegistry());
        if (!e.isValid())
        {
            Core::error("Cannot add invalid entity to scene");
            return;
        }

        // Register the entity name
        registerEntityName(entity, e.getName());

        // Add to root entities (unless it has a parent, which would be handled by the parent)
        m_rootEntities.push_back(entity);

        Core::debug("Added entity " + e.getName() + " to scene");
    }

    bool Scene::removeEntity(EntityID entity)
    {
        auto world = m_world.lock();
        if (!world)
        {
            Core::error("Cannot remove entity from scene: No world reference");
            return false;
        }

        Entity e(entity, world->getRegistry());
        if (!e.isValid())
        {
            Core::error("Cannot remove invalid entity from scene");
            return false;
        }

        // Unregister the entity name
        unregisterEntityName(entity, e.getName());

        // Remove from root entities
        auto it = std::find(m_rootEntities.begin(), m_rootEntities.end(), entity);
        if (it != m_rootEntities.end())
        {
            m_rootEntities.erase(it);
        }

        // If this was the main camera, clear it
        if (m_mainCamera == entity)
        {
            m_mainCamera = INVALID_ENTITY_ID;
        }

        // Remove any tags
        for (auto& tagPair : m_entitiesByTag)
        {
            auto& entities = tagPair.second;
            entities.erase(std::remove(entities.begin(), entities.end(), entity), entities.end());
        }

        Core::debug("Removed entity " + e.getName() + " from scene");

        return true;
    }

    void Scene::registerEntityName(EntityID entity, const std::string& name)
    {
        if (name.empty())
        {
            return;
        }

        // Add to name map
        m_entitiesByName[name] = entity;
    }

    void Scene::unregisterEntityName(EntityID entity, const std::string& name)
    {
        if (name.empty())
        {
            return;
        }

        // Remove from name map
        auto it = m_entitiesByName.find(name);
        if (it != m_entitiesByName.end() && it->second == entity)
        {
            m_entitiesByName.erase(it);
        }
    }

    std::shared_ptr<Voxel::Chunk> Scene::getChunk(const Voxel::ChunkCoord& coord, bool createIfMissing)
    {
        auto it = m_chunks.find(coord);
        if (it != m_chunks.end())
        {
            return it->second;
        }

        if (!createIfMissing)
        {
            return nullptr;
        }

        // Create a new chunk
        auto chunk = std::make_shared<Voxel::Chunk>(coord);
        m_chunks[coord] = chunk;

        // Mark as dirty since it's new
        markChunkDirty(coord);

        return chunk;
    }

    void Scene::addTag(EntityID entity, const std::string& tag)
    {
        auto& entities = m_entitiesByTag[tag];
        if (std::find(entities.begin(), entities.end(), entity) == entities.end())
        {
            entities.push_back(entity);
        }
    }

    void Scene::removeTag(EntityID entity, const std::string& tag)
    {
        auto it = m_entitiesByTag.find(tag);
        if (it != m_entitiesByTag.end())
        {
            auto& entities = it->second;
            entities.erase(std::remove(entities.begin(), entities.end(), entity), entities.end());

            // Remove the tag entry is no entities have this tag
            if (entities.empty())
            {
                m_entitiesByTag.erase(it);
            }
        }
    }

    bool Scene::hasTag(EntityID entity, const std::string& tag) const
    {
        auto it = m_entitiesByTag.find(tag);
        if (it == m_entitiesByTag.end())
        {
            return false;
        }

        const auto& entities = it->second;
        return std::find(entities.begin(), entities.end(), entity) != entities.end();
    }

    Utility::Serialization::SerializationResult Scene::serialize(Utility::Serialization::Serializer& serializer) const
    {
        auto result = serializer.beginObject(getTypeName());
        if (!result) return result;

        // Write basic scene properties
        result = serializer.writeField("name", m_name);
        if (!result) return result;

        result = serializer.writeField("path", m_path);
        if (!result) return result;

        result = serializer.writeField("mainCamera", static_cast<uint32_t>(m_mainCamera));
        if (!result) return result;

        // Write entity name map
        result = serializer.beginArray("entitiesByName", m_entitiesByName.size());
        if (!result) return result;

        for (const auto& [name, entity] : m_entitiesByName)
        {
            result = serializer.writeField("entityName", name);
            if (!result) return result;

            result = serializer.writeField("entityId", static_cast<uint32_t>(entity));
            if (!result) return result;
        }

        result = serializer.endArray();
        if (!result) return result;

        // Write entity tag map
        result = serializer.beginArray("entitiesByTag", m_entitiesByTag.size());
        if (!result) return result;

        for (const auto& [tag, entities] : m_entitiesByTag)
        {
            result = serializer.writeField("entityTag", tag);
            if (!result) return result;

            result = serializer.beginArray("entities", entities.size());
            if (!result) return result;

            for (auto entity : entities)
            {
                result = serializer.writeField("entityId", static_cast<uint32_t>(entity));
                if (!result) return result;
            }

            result = serializer.endArray();
            if (!result) return result;
        }

        result = serializer.endArray();
        if (!result) return result;

        // Write dirty chunks
        result = serializer.beginArray("dirtyChunks", m_dirtyChunks.size());
        if (!result) return result;

        for (const auto& chunkCoord : m_dirtyChunks)
        {
            result = chunkCoord.serialize(serializer);
            if (!result) return result;
        }

        return serializer.endObject();
    }

    Utility::Serialization::SerializationResult Scene::deserialize(Utility::Serialization::Deserializer& deserializer)
    {
        m_entitiesByName.clear();
        m_entitiesByTag.clear();
        m_dirtyChunks.clear();

        auto result = deserializer.beginObject(getTypeName());
        if (!result) return result;

        // Write basic scene properties
        result = deserializer.readField("name", m_name);
        if (!result) return result;

        result = deserializer.readField("path", m_path);
        if (!result) return result;

        uint32_t mainCamera;
        result = deserializer.readField("mainCamera", mainCamera);
        if (!result) return result;
        m_mainCamera = static_cast<EntityID>(mainCamera);

        size_t arraySize;

        // Write entity name map
        result = deserializer.beginArray("entitiesByName", arraySize);
        if (!result) return result;

        for (size_t i = 0; i < arraySize; i++)
        {
            std::string name;
            result = deserializer.readField("entityName", name);
            if (!result) return result;

            uint32_t entity;
            result = deserializer.readField("entityId", entity);
            if (!result) return result;

            m_entitiesByName[name] = static_cast<EntityID>(entity);
        }

        result = deserializer.endArray();
        if (!result) return result;

        // Write entity tag map
        result = deserializer.beginArray("entitiesByTag", arraySize);
        if (!result) return result;

        for (size_t i = 0; i < arraySize; i++)
        {
            std::string tag;
            result = deserializer.readField("entityTag", tag);
            if (!result) return result;

            size_t entitiesArraySize;
            result = deserializer.beginArray("entities", entitiesArraySize);
            if (!result) return result;

            std::vector<EntityID> entities;
            for (size_t j = 0; j < entitiesArraySize; j++)
            {
                uint32_t entity;
                result = deserializer.readField("entityId", entity);
                if (!result) return result;

                entities.push_back(static_cast<EntityID>(entity));
            }

            result = deserializer.endArray();
            if (!result) return result;

            m_entitiesByTag.insert(std::make_pair(tag, entities));
        }

        result = deserializer.endArray();
        if (!result) return result;

        // Write dirty chunks
        result = deserializer.beginArray("dirtyChunks", arraySize);
        if (!result) return result;

        for (size_t i = 0; i < arraySize; i++)
        {
            Voxel::ChunkCoord chunkCoord;
            result = chunkCoord.deserialize(deserializer);
            if (!result) return result;

            m_dirtyChunks.insert(chunkCoord);
        }

        return deserializer.endObject();
    }

    void Scene::defineSchema(Utility::Serialization::Schema& schema)
    {
        schema.addField("name", Utility::Serialization::ValueType::String, true);
        schema.addField("path", Utility::Serialization::ValueType::String, true);
        schema.addField("mainCamera", Utility::Serialization::ValueType::UInt32, true);
        schema.addField("entitiesByName", Utility::Serialization::ValueType::Object, true);
        schema.addField("entitiesByTag", Utility::Serialization::ValueType::Object, true);
        schema.addField("dirtyChunks", Utility::Serialization::ValueType::Object, true);
    }

    bool Scene::loadEntities(const std::string& path)
    {
        std::string entityPath = path;
        if (fs::getPathExtension(path) != ".scene")
        {
            entityPath = path + ".entities";
        }
        else
        {
            entityPath = fs::getFileNameWithoutExtension(path) + ".entities";
        }

        if (!fs::fileExists(entityPath))
        {
            Core::warn("Entity file does not exist: " + entityPath);
            return false;
        }

        auto world = m_world.lock();
        if (!world)
        {
            Core::error("Cannot load entities: No world reference");
            return false;
        }

        // Create the container and deserialize
        EntityContainer container(this, world);
        auto result = Utility::Serialization::SerializationUtility::deserializeFromFile(container, entityPath);

        if (!result)
        {
            Core::error("Failed to load entities: " + result.error);
            return false;
        }

        Core::info(StringUtils::format("Loaded {} entities from {}", m_rootEntities.size(), entityPath));

        return true;
    }

    bool Scene::saveEntities(const std::string& path)
    {
        std::string entityPath = path;
        if (fs::getPathExtension(path) != ".scene")
        {
            entityPath = path + ".entities";
        }
        else
        {
            entityPath = fs::getFileNameWithoutExtension(path) + ".entities";
        }

        auto world = m_world.lock();
        if (!world)
        {
            Core::error("Cannot save entities: No world reference");
            return false;
        }

        // Create the container and serialize
        EntityContainer container(this, world);
        auto result = Utility::Serialization::SerializationUtility::serializeToFile(container, entityPath);

        if (!result)
        {
            Core::error("Failed to save entities: " + result.error);
            return false;
        }

        Core::info(StringUtils::format("Saved {} entities to {}", m_rootEntities.size(), entityPath));

        return true;
    }

    bool Scene::loadChunks()
    {
        // Determine the chunks directory
        std::string chunkDir = getSceneDirectory() + "/chunks";

        // Check if the chunks directory exists
        if (!fs::directoryExists(chunkDir))
        {
            Core::warn("Chunks directory does not exist: " + chunkDir);
            return false;
        }

        // List all chunk files in the directory
        auto chunkFiles = fs::listFiles(chunkDir, "*.chunk");

        // Load each chunk
        for (const auto& fileInfo : chunkFiles)
        {
            // Parse coordinates from filename
            std::string filename = fs::getFileNameWithoutExtension(fileInfo.name);
            int x, y, z;
            if (sscanf_s(filename.c_str(), "chunk_%d_%d_%d", &x, &y, &z) == 3)
            {
                Voxel::ChunkCoord coord(x, y, z);
                deserializeChunk(coord);
            }
        }

        Core::info(StringUtils::format("Loaded {} chunks from {}", chunkFiles.size(), chunkDir));

        return !chunkFiles.empty();
    }

    bool Scene::saveChunks()
    {
        // Copy dirty chunks to avoid iterator invalidation
        auto dirtyChunks = m_dirtyChunks;

        bool success = true;
        for (const auto& coord : dirtyChunks)
        {
            success = serializeChunk(coord);
            if (!success) return success;
        }

        return success;
    }

    std::string Scene::getChunkPath(const Voxel::ChunkCoord& coord) const
    {
        // Create the chunk filename
        std::string chunkFilename = "chunk_" +
            std::to_string(coord.x) + "_" +
            std::to_string(coord.y) + "_" +
            std::to_string(coord.z) + ".chunk";

        // Return the full path
        return getSceneDirectory() + "/chunks/" + chunkFilename;
    }

    void Scene::updateChunkMesh(const Voxel::ChunkCoord& coord)
    {
        // Mark the chunk as dirty
        markChunkDirty(coord);

        // This would trigger mesh regeneration in a real implementation
        // For now, just log it
        Core::debug(StringUtils::format("Chunk mesh update triggered for ({}, {}, {})",
                                       coord.x, coord.y, coord.z));
    }

    std::string Scene::getSceneDirectory() const
    {
        // Get the directory containing the scene
        std::string baseDir = fs::getDirectoryPath(m_path);

        // If the path doesn't have an extension, it's probably a directory itself
        if (fs::getPathExtension(m_path).empty())
        {
            return m_path;
        }

        // Create a scene directory based on the filename
        std::string sceneName = fs::getFileNameWithoutExtension(m_path);
        return baseDir + "/" + sceneName + "_data";
    }

    bool Scene::ensureSceneDirectoryExists() const
    {
        std::string sceneDir = getSceneDirectory();
        if (!fs::directoryExists(sceneDir))
        {
            if (!fs::createDirectory(sceneDir))
            {
                Core::error("Failed to create scene directory: " + sceneDir);
                return false;
            }
        }

        // Also ensure the chunks directory exists
        std::string chunkDir = sceneDir + "/chunks";
        if (!fs::directoryExists(chunkDir))
        {
            if (!fs::createDirectory(chunkDir))
            {
                Core::error("Failed to create chunks directory: " + chunkDir);
                return false;
            }
        }

        return true;
    }

    Utility::Serialization::SerializationResult EntityContainer::serialize(Utility::Serialization::Serializer& serializer)
    {
        auto result = serializer.beginObject("RootEntities");
        if (!result) return result;

        // Write root entities array
        result = serializer.beginArray("rootEntities", m_scene->getRootEntities().size());
        if (!result) return result;

        for (auto entityId : m_scene->getRootEntities())
        {
            Entity entity(entityId, m_world->getRegistry());
            if (entity.isValid())
            {
                result = serializer.writeField("entity_" + std::to_string(entityId), entity);
                if (!result) return result;
            }
        }

        result = serializer.endArray();
        if (!result) return result;

        // Write main camera
        result = serializer.writeField("mainCamera", static_cast<uint32_t>(m_scene->getMainCamera()));
        if (!result) return result;

        return serializer.endObject();
    }

    Utility::Serialization::SerializationResult EntityContainer::deserialize(Utility::Serialization::Deserializer& deserializer)
    {
        // Clear any existing data
        m_scene->clearRootEntities();

        auto result = deserializer.beginObject("RootEntities");
        if (!result) return result;

        // Read root entities
        size_t arraySize;
        result = deserializer.beginArray("entities", arraySize);
        if (!result) return result;

        for (size_t i = 0; i < arraySize; i++)
        {
            EntityID newId = m_world->createEntity();
            Entity entity(newId, m_world->getRegistry());

            // Deserialize the entity
            result = entity.deserialize(deserializer);
            if (!result) return result;

            // Add to root entities
            m_scene->insertRootEntities(newId);

            // Register entity name
            m_scene->registerEntityName(newId, entity.getName());
        }

        // Read main camera ID
        uint32_t mainCamera;
        result = deserializer.readField("mainCamera", mainCamera);
        if (!result) return result;

        m_scene->setMainCamera(mainCamera);

        return deserializer.endObject();
    }

    void EntityContainer::defineSchema(Utility::Serialization::Schema& schema)
    {
        schema.addArrayField("entities", Utility::Serialization::ValueType::Object);
        schema.addField("camera", Utility::Serialization::ValueType::Object);
    }

} // namespace PixelCraft::ECS