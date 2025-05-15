// -------------------------------------------------------------------------
// Scene.cpp
// -------------------------------------------------------------------------
#include "ECS/Scene.h"
#include "ECS/World.h"
#include "ECS/Entity.h"
#include "ECS/Serializer.h"
#include "ECS/FlatBufferSerializer.h"
#include "ECS/Deserializer.h"
#include "Core/Logger.h"
#include "Utility/FileSystem.h"
#include "Utility/StringUtils.h"

// Components
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/CameraComponent.h"

// Schemas
#include <ECS/Schemas/scene_generated.h>
#include <ECS/Schemas/entity_generated.h>

#include <filesystem>
#include <fstream>
#include <sstream>

using fs = PixelCraft::Utility::FileSystem;
using StringUtils = PixelCraft::Utility::StringUtils;
namespace Log = PixelCraft::Core;

namespace PixelCraft::ECS
{

    Scene::Scene(const std::string& name)
        : m_name(name)
        , m_mainCamera(INVALID_ENTITY_ID)
        , m_schemaVersion(1) // Start with version 1
    {
        Log::info("Created scene: " + name);
    }

    Scene::~Scene()
    {
        // Save any unsaved data if we have a valid path
        if (!m_path.empty())
        {
            saveModifiedChunks();
        }

        Log::info("Destroyed scene: " + m_name);
    }

    bool Scene::load(const std::string& path)
    {
        m_path = path;

        Log::info("Loading scene from: " + path);

        // Check if the file exists
        if (!fs::fileExists(path))
        {
            Log::error("Scene file does not exist: " + path);
            return false;
        }

        // Clear any existing data
        m_entitiesByName.clear();
        m_entitiesByTag.clear();
        m_rootEntities.clear();
        m_chunks.clear();
        m_dirtyChunks.clear();

        // Load entities and chunks
        bool entitiesLoaded = loadEntities(path);
        bool chunksLoaded = loadChunks(path);

        // Return success if at least one of them succeeded
        return entitiesLoaded || chunksLoaded;
    }

    bool Scene::save(const std::string& path)
    {
        if (path.empty())
        {
            if (m_path.empty())
            {
                Log::error("Cannot save scene: No path specified");
                return false;
            }
        }
        else
        {
            m_path = path;
        }

        Log::info("Saving scene to: " + m_path);

        // Make sure the directory exists
        if (!ensureSceneDirectoryExists())
        {
            Log::error("Failed to create scene directory for: " + m_path);
            return false;
        }

        // Save entities and chunks
        bool entitiesSaved = saveEntities(m_path);
        bool chunksSaved = saveChunks(m_path);

        // Save modified chunks individually
        saveModifiedChunks();

        // Return success if at least one of them succeeded
        return entitiesSaved || chunksSaved;
    }

    EntityID Scene::instantiate(const std::string& prefabPath)
    {
        // Check if we have a valid world
        auto world = m_world.lock();
        if (!world)
        {
            Log::error("Cannot instantiate prefab: No world reference");
            return INVALID_ENTITY_ID;
        }

        Log::info("Instantiating prefab: " + prefabPath);

        // Check if the prefab file exists
        if (!fs::fileExists(prefabPath))
        {
            Log::error("Prefab file does not exist: " + prefabPath);
            return INVALID_ENTITY_ID;
        }

        // Load the prefab file
        std::vector<uint8_t> prefabData;
        auto result = fs::readFileBinary(prefabPath, prefabData);
        if (result != Utility::FileResult::Success)
        {
            Log::error("Failed to read prefab file: " + prefabPath);
            return INVALID_ENTITY_ID;
        }

        // Create a new entity
        EntityID entityId = world->createEntity(fs::getFileNameWithoutExtension(prefabPath));
        if (entityId == INVALID_ENTITY_ID)
        {
            Log::error("Failed to create entity for prefab: " + prefabPath);
            return INVALID_ENTITY_ID;
        }

        // Deserialize the prefab data into the entity
        std::istringstream dataStream(std::string(prefabData.begin(), prefabData.end()));
        Deserializer deserializer(dataStream);
        deserializer.setRegistry(world->getRegistry());

        Entity entity(entityId, world->getRegistry());
        entity.deserialize(deserializer);

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
                Log::info("Set main camera to entity: " + entity.getName());
            }
            else
            {
                Log::warn("Cannot set main camera: Invalid entity ID");
            }
        }
        else
        {
            Log::warn("Cannot set main camera: No world reference");
        }
    }

    std::vector<uint8_t> Scene::serializeToFlatBuffer()
    {
        // Use FlatBufferSerializer for efficient zero-copy serialization
        FlatBufferSerializer serializer;
        auto& builder = serializer.createBuilder();

        Log::info(StringUtils::format("Serializing scene '{}' to FlatBuffer", m_name));

        // Convert root entity vector
        std::vector<uint32_t> rootEntitiesVec(m_rootEntities.begin(), m_rootEntities.end());
        auto rootEntitiesVector = builder.CreateVector(rootEntitiesVec);

        // Create entity name map entries
        std::vector<flatbuffers::Offset<EntityNameMapEntry>> entityNameEntries;
        for (const auto& [name, entityId] : m_entitiesByName)
        {
            auto nameOffset = builder.CreateString(name);
            entityNameEntries.push_back(CreateEntityNameMapEntry(builder, nameOffset, entityId));
        }
        auto entityNameMapVector = builder.CreateVector(entityNameEntries);

        // Create entity tag map entries
        std::vector<flatbuffers::Offset<EntityTagMapEntry>> entityTagEntries;
        for (const auto& [tag, entities] : m_entitiesByTag)
        {
            auto tagOffset = builder.CreateString(tag);
            std::vector<uint32_t> entityIds(entities.begin(), entities.end());
            auto entitiesVector = builder.CreateVector(entityIds);
            entityTagEntries.push_back(CreateEntityTagMapEntry(builder, tagOffset, entitiesVector));
        }
        auto entityTagMapVector = builder.CreateVector(entityTagEntries);

        // Create dirty chunk coordinates
        std::vector<flatbuffers::Offset<flatbuffers::Vector<int32_t>>> dirtyChunksVectors;
        for (const auto& coord : m_dirtyChunks)
        {
            std::vector<int32_t> coordVec = {coord.x, coord.y, coord.z};
            dirtyChunksVectors.push_back(builder.CreateVector(coordVec));
        }
        auto dirtyChunksVector = builder.CreateVector(dirtyChunksVectors);

        // Create name and path strings
        auto nameOffset = builder.CreateString(m_name);
        auto pathOffset = builder.CreateString(m_path);

        // Create the scene FlatBuffer
        auto sceneOffset = CreateSceneData(
            builder,
            nameOffset,
            pathOffset,
            m_schemaVersion,
            m_mainCamera,
            rootEntitiesVector,
            entityNameMapVector,
            entityTagMapVector,
            dirtyChunksVector
        );

        // Finish the buffer with the "SCNE" identifier
        builder.Finish(sceneOffset, "SCNE");

        // Get the serialized buffer
        return serializer.finishBuffer("SCNE");
    }

    bool Scene::deserializeFromFlatBuffer(const uint8_t* data, size_t size)
    {
        // Validate input parameters
        if (!data || size == 0)
        {
            Log::error("Invalid FlatBuffer data provided for deserialization");
            return false;
        }

        // Check for a valid world
        auto world = m_world.lock();
        if (!world)
        {
            Log::error("Cannot deserialize scene: No world reference");
            return false;
        }

        // Use custom verification for the buffer
        flatbuffers::Verifier verifier(data, size);
        if (!flatbuffers::BufferHasIdentifier(data, "SCNE"))
        {
            Log::error("Invalid scene flatbuffer identifier");
            return false;
        }

        if (!verifier.VerifyBuffer<SceneData>(nullptr))
        {
            Log::error("Invalid scene flatbuffer format or corrupted data");
            return false;
        }

        Log::info("Deserializing scene from FlatBuffer");

        // Get the root scene object from the buffer
        auto sceneData = GetSceneData(data);

        // Extract basic scene information
        m_name = sceneData->name()->str();
        m_path = sceneData->path()->str();
        m_schemaVersion = sceneData->schema_version();
        m_mainCamera = sceneData->main_camera();

        // Clear existing data
        m_rootEntities.clear();
        m_entitiesByName.clear();
        m_entitiesByTag.clear();
        m_dirtyChunks.clear();

        // Create entity lookup map for any previously existing entities
        std::unordered_map<EntityID, Entity> existingEntities;

        // Get the registry for component operations
        auto registry = world->getRegistry();

        // Load entity data
        if (sceneData->root_entities())
        {
            auto rootEntities = sceneData->root_entities();
            for (uint32_t i = 0; i < rootEntities->size(); i++)
            {
                EntityID entityId = rootEntities->Get(i);

                // Try to find this entity in the scene storage
                // In a real implementation, we would have a separate array with entity data in the FlatBuffer
                std::string entityPath = getSceneDirectory() + "/entities/" + std::to_string(entityId) + ".entity";

                if (fs::fileExists(entityPath))
                {
                    // Load entity from file
                    std::vector<uint8_t> entityData;
                    auto result = fs::readFileBinary(entityPath, entityData);

                    if (result == Utility::FileResult::Success)
                    {
                        // Verify this is entity data
                        if (entityData.size() > 8 && memcmp(entityData.data() + 4, "ENTY", 4) == 0)
                        {
                            // Create a new entity with the same ID if possible
                            EntityID newId = world->createEntity();
                            Entity newEntity(newId, registry);

                            // Deserialize the entity data
                            if (newEntity.isValid())
                            {
                                // Use the entity's FlatBuffer deserialization
                                const uint8_t* entityBuffer = entityData.data();
                                auto entityFlatBuffer = GetEntityData(entityBuffer);

                                // Set entity name
                                if (entityFlatBuffer->name())
                                {
                                    newEntity.setName(entityFlatBuffer->name()->str());
                                }

                                // Create and add components
                                if (entityFlatBuffer->components())
                                {
                                    auto components = entityFlatBuffer->components();
                                    for (uint32_t j = 0; j < components->size(); j++)
                                    {
                                        auto componentEntry = components->Get(j);

                                        // Get component type and data
                                        ComponentType type = componentEntry->type();
                                        const void* componentData = componentEntry->data();

                                        // Create and deserialize component based on type
                                        switch (type)
                                        {
                                            case ComponentType_Transform:
                                            {
                                                // Create TransformComponent
                                                auto& transform = newEntity.addComponent<TransformComponent>();

                                                // Use FlatBufferSerializer to deserialize
                                                FlatBufferSerializer compSerializer;
                                                compSerializer.deserializeTransformComponent(componentData, transform);
                                                break;
                                            }
                                            case ComponentType_Camera:
                                            {
                                                // Create CameraComponent
                                                auto& camera = newEntity.addComponent<CameraComponent>();

                                                // Use FlatBufferSerializer to deserialize
                                                FlatBufferSerializer compSerializer;
                                                compSerializer.deserializeComponent(componentData, camera);

                                                if (camera.isMain())
                                                {
                                                    setMainCamera(newId);
                                                }
                                                break;
                                            }
                                            // Add cases for other component types as needed
                                        }
                                    }
                                }

                                // Add entity tags
                                if (entityFlatBuffer->tags())
                                {
                                    auto tags = entityFlatBuffer->tags();
                                    for (uint32_t j = 0; j < tags->size(); j++)
                                    {
                                        addTag(newId, tags->Get(j)->str());
                                    }
                                }

                                // Add to root entities
                                m_rootEntities.push_back(newId);

                                // Store the entity
                                existingEntities[entityId] = newEntity;

                                // Register the entity name
                                registerEntityName(newId, newEntity.getName());
                            }
                        }
                    }
                }
                else
                {
                    // Entity data not found, create a new placeholder entity
                    EntityID newId = world->createEntity("Entity_" + std::to_string(entityId));
                    Entity newEntity(newId, registry);

                    if (newEntity.isValid())
                    {
                        // Add to root entities
                        m_rootEntities.push_back(newId);

                        // Store the entity
                        existingEntities[entityId] = newEntity;

                        // register the entity name
                        registerEntityName(newId, newEntity.getName());
                    }
                }
            }
        }

        // Deserialize entity name map
        if (sceneData->entity_name_map())
        {
            auto nameMap = sceneData->entity_name_map();
            for (uint32_t i = 0; i < nameMap->size(); i++)
            {
                auto entry = nameMap->Get(i);
                EntityID oldId = entry->entity_id();

                // Find the corresponding new entity ID
                auto it = existingEntities.find(oldId);
                if (it != existingEntities.end())
                {
                    m_entitiesByName[entry->name()->str()] = it->second.getID();
                }
            }
        }

        // Deserialize entity tag map
        if (sceneData->entity_tag_map())
        {
            auto tagMap = sceneData->entity_tag_map();
            for (uint32_t i = 0; i < tagMap->size(); i++)
            {
                auto entry = tagMap->Get(i);
                std::string tag = entry->tag()->str();

                auto& entities = m_entitiesByTag[tag];
                entities.clear();

                auto tagEntities = entry->entities();
                for (uint32_t j = 0; j < tagEntities->size(); j++)
                {
                    EntityID oldId = tagEntities->Get(j);

                    // find the corresponding new entity ID
                    auto it = existingEntities.find(oldId);
                    if (it != existingEntities.end())
                    {
                        entities.push_back(it->second.getID());
                    }
                }
            }
        }

        // Update main camera reference if needed
        if (m_mainCamera != INVALID_ENTITY_ID)
        {
            auto it = existingEntities.find(m_mainCamera);
            if (it != existingEntities.end())
            {
                m_mainCamera = it->second.getID();
            }
            else
            {
                m_mainCamera = INVALID_ENTITY_ID;
            }
        }

        // Process parent-child relationships for transforms
        for (auto& [oldId, entity] : existingEntities)
        {
            if (entity.hasComponent<TransformComponent>())
            {
                auto& transform = entity.getComponent<TransformComponent>();

                // Get the parent ID
                EntityID oldParentId = transform.getParent();
                if (oldParentId != INVALID_ENTITY_ID)
                {
                    // Find the new parent ID
                    auto parentIt = existingEntities.find(oldParentId);
                    if (parentIt != existingEntities.end())
                    {
                        // Set the new parent
                        transform.setParent(parentIt->second.getID());
                    }
                }
            }
        }

        // Deserialize dirty chunks
        if (sceneData->dirty_chunks())
        {
            auto dirtyChunks = sceneData->dirty_chunks();
            for (uint32_t i = 0; i < dirtyChunks->size(); i++)
            {
                auto coordVec = dirtyChunks->Get(i);
                if (coordVec->size() >= 3)
                {
                    Voxel::ChunkCoord coord(coordVec->Get(0), coordVec->Get(1), coordVec->Get(2));
                    m_dirtyChunks.insert(coord);
                }
            }
        }

        Log::info(StringUtils::format("Successfully deserialized scene '{}' from FlatBuffer", m_name));
        return true;
    }

    std::vector<uint8_t> Scene::serializeEntityToFlatBuffer(EntityID entity)
    {
        auto world = m_world.lock();
        if (!world)
        {
            Log::error("Cannot serialize entity to FlatBuffer: No world reference");
            return {};
        }

        Entity e(entity, world->getRegistry());
        if (!e.isValid())
        {
            Log::error("Cannot serialize invalid entity to FlatBuffer");
            return {};
        }

        // Use FlatBufferSerializer for zero-copy serialization
        FlatBufferSerializer serializer;
        auto& builder = serializer.createBuilder();

        // Create name string
        auto nameOffset = builder.CreateString(e.getName());

        // Create UUID string
        auto uuidOffset = builder.CreateString(e.getUUID().toString());

        // Collect entity tags
        std::vector<std::string> tags;
        for (const auto& tagPair : m_entitiesByTag)
        {
            const auto& entities = tagPair.second;
            if (std::find(entities.begin(), entities.end(), entity) != entities.end())
            {
                tags.push_back(tagPair.first);
            }
        }

        // Create tags vector
        auto tagsOffset = builder.CreateVectorOfStrings(tags);

        // Collect and serialize components
        std::vector<flatbuffers::Offset<ComponentEntry>> componentEntries;

        // Check for TransformComponent
        if (e.hasComponent<TransformComponent>())
        {
            auto& transform = e.getComponent<TransformComponent>();
            auto transformOffset = serializer.serializeTransformComponent(transform);
            componentEntries.push_back(CreateComponentEntry(builder, ComponentType_Transform, transformOffset));
        }

        // Check for CameraComponent
        if (e.hasComponent<CameraComponent>())
        {
            auto& camera = e.getComponent<CameraComponent>();
            auto cameraOffset = serializer.serializeComponent(camera);
            componentEntries.push_back(CreateComponentEntry(builder, ComponentType_Camera, cameraOffset));
        }

        // Add other component types here as needed
        // Example:
        // if (e.hasComponent<MeshRendererComponent>()) {
        //     auto& meshRenderer = e.getComponent<MeshRendererComponent>();
        //     auto meshRendererOffset = serializer.serializeMeshRendererComponent(meshRenderer);
        //     componentEntries.push_back(CreateComponentEntry(builder, ComponentType_MeshRenderer, meshRendererOffset));
        // }

        auto componentsOffset = builder.CreateVector(componentEntries);

        // Create the entity FlatBuffer
        auto entityOffset = CreateEntityData(
            builder,
            entity,                 // id
            nameOffset,             // name
            uuidOffset,             // uuid
            componentsOffset,       // components
            tagsOffset              // tags
        );

        // Finish the buffer with the "ENTY" identifier
        builder.Finish(entityOffset, "ENTY");

        // Return the serialized data
        return serializer.finishBuffer("ENTY");
    }

    bool Scene::serializeChunk(const Voxel::ChunkCoord& coord)
    {
        auto it = m_chunks.find(coord);
        if (it == m_chunks.end())
        {
            Log::warn(StringUtils::format("Cannot serialize chunk: Chunk not found at coordinates ({}, {}, {})",
                                          coord.x, coord.y, coord.z));
            return false;
        }

        // Ensure the scene directory exists
        if (!ensureSceneDirectoryExists())
        {
            Log::error("Failed to create scene directory for chunk serialization");
            return false;
        }

        // Get the path for this chunk
        std::string chunkPath = getChunkPath(coord);

        // Open the file
        std::ofstream file(chunkPath, std::ios::binary);
        if (!file.is_open())
        {
            Log::error("Failed to open file for chunk serialization: " + chunkPath);
            return false;
        }

        // Create a serializer
        Serializer serializer(file);

        // Serialize the chunk
        it->second->serialize(serializer);

        // Remove from dirty chunks if successful
        m_dirtyChunks.erase(coord);

        Log::debug(StringUtils::format("Serialized chunk at ({}, {}, {}) to {}",
                                       coord.x, coord.y, coord.z, chunkPath));

        return true;
    }

    bool Scene::deserializeChunk(Voxel::ChunkCoord& coord)
    {
        // Ensure we have a world reference
        auto world = m_world.lock();
        if (!world)
        {
            Log::error("Cannot deserialize chunk: No world reference");
            return false;
        }

        // Get the path for this chunk
        std::string chunkPath = getChunkPath(coord);

        // Check if the file exists
        if (!fs::fileExists(chunkPath))
        {
            Log::warn("Chunk file does not exist: " + chunkPath);
            return false;
        }

        // Create or get the chunk
        auto chunk = getChunk(coord, true);
        if (!chunk)
        {
            Log::error("Failed to create chunk for deserialization");
            return false;
        }

        // Open the file
        std::ifstream file(chunkPath, std::ios::binary);
        if (!file.is_open())
        {
            Log::error("Failed to open file for chunk deserialization: " + chunkPath);
            return false;
        }

        // Create a deserializer
        Deserializer deserializer(file);
        deserializer.setRegistry(world->getRegistry());

        // Deserialize the chunk
        chunk->deserialize(deserializer);

        // Remove from dirty chunks
        m_dirtyChunks.erase(coord);

        Log::debug(StringUtils::format("Deserialized chunk at ({}, {}, {}) from {}",
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

    void Scene::saveModifiedChunks()
    {
        // Copy dirty chunks to avoid iterator invalidation
        auto dirtyChunks = m_dirtyChunks;

        for (const auto& coord : dirtyChunks)
        {
            serializeChunk(coord);
        }
    }

    void Scene::addEntity(EntityID entity)
    {
        auto world = m_world.lock();
        if (!world)
        {
            Log::error("Cannot add entity to scene: No world reference");
            return;
        }

        Entity e(entity, world->getRegistry());
        if (!e.isValid())
        {
            Log::error("Cannot add invalid entity to scene");
            return;
        }

        // Register the entity name
        registerEntityName(entity, e.getName());

        // Add to root entities (unless it has a parent, which would be handled by the parent)
        m_rootEntities.push_back(entity);

        Log::debug("Added entity " + e.getName() + " to scene");
    }

    bool Scene::removeEntity(EntityID entity)
    {
        auto world = m_world.lock();
        if (!world)
        {
            Log::error("Cannot remove entity from scene: No world reference");
            return false;
        }

        Entity e(entity, world->getRegistry());
        if (!e.isValid())
        {
            Log::error("Cannot remove invalid entity from scene");
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

        Log::debug("Removed entity " + e.getName() + " from scene");

        return true;
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

    bool Scene::loadEntities(const std::string& path)
    {
        // Determine the entity file path
        std::string entityPath = path;
        if (fs::getPathExtension(path) != ".scene")
        {
            entityPath = path + ".entities";
        }
        else
        {
            entityPath = fs::getFileNameWithoutExtension(path) + ".entities";
        }

        // Check if the entity file exists
        if (!fs::fileExists(entityPath))
        {
            Log::warn("Entity file does not exist: " + entityPath);
            return false;
        }

        // Ensure we have a world reference
        auto world = m_world.lock();
        if (!world)
        {
            Log::error("Cannot load entities: No world reference");
            return false;
        }

        // Read the file
        std::vector<uint8_t> data;
        auto result = fs::readFileBinary(entityPath, data);
        if (result != Utility::FileResult::Success)
        {
            Log::error("Failed to read entity file: " + entityPath);
            return false;
        }

        // Deserialize the entity data
        std::string dataStr(data.begin(), data.end());
        std::istringstream stream(dataStr);
        Deserializer deserializer(stream);
        deserializer.setRegistry(world->getRegistry());

        // Deserialize root entities
        size_t entityCount = 0;
        if (deserializer.beginArray("rootEntities", entityCount))
        {
            m_rootEntities.resize(entityCount);
            for (size_t i = 0; i < entityCount; i++)
            {
                m_rootEntities[i] = deserializeEntity(deserializer);
            }
            deserializer.endArray();
        }

        // Deserialize camera reference
        deserializer.read(m_mainCamera);

        Log::info(StringUtils::format("Loaded {} entities from {}", entityCount, entityPath));

        return true;
    }

    bool Scene::saveEntities(const std::string& path)
    {
        // Determine the entity file path
        std::string entityPath = path;
        if (fs::getPathExtension(path) != ".scene")
        {
            entityPath = path + ".entities";
        }
        else
        {
            entityPath = fs::getFileNameWithoutExtension(path) + ".entities";
        }

        // Ensure we have a world reference
        auto world = m_world.lock();
        if (!world)
        {
            Log::error("Cannot save entities: No world reference");
            return false;
        }

        // Open the file
        std::ofstream file(entityPath, std::ios::binary);
        if (!file.is_open())
        {
            Log::error("Failed to open file for entity serialization: " + entityPath);
            return false;
        }

        // Create a serializer
        Serializer serializer(file);
        serializer.setRegistry(world->getRegistry());

        // Serialize root entities
        serializer.beginArray("rootEntities", m_rootEntities.size());
        for (const auto& entityId : m_rootEntities)
        {
            serializeEntity(entityId, serializer);
        }
        serializer.endArray();

        // Serialize camera reference
        serializer.write(m_mainCamera);

        Log::info(StringUtils::format("Saved {} entities to {}", m_rootEntities.size(), entityPath));

        return true;
    }

    bool Scene::loadChunks(const std::string& path)
    {
        // Determine the chunks directory
        std::string chunkDir = getSceneDirectory() + "/chunks";

        // Check if the chunks directory exists
        if (!fs::directoryExists(chunkDir))
        {
            Log::warn("Chunks directory does not exist: " + chunkDir);
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
            if (sscanf(filename.c_str(), "chunk_%d_%d_%d", &x, &y, &z) == 3)
            {
                Voxel::ChunkCoord coord(x, y, z);
                deserializeChunk(coord);
            }
        }

        Log::info(StringUtils::format("Loaded {} chunks from {}", chunkFiles.size(), chunkDir));

        return !chunkFiles.empty();
    }

    bool Scene::saveChunks(const std::string& path)
    {
        // Just save all modified chunks
        saveModifiedChunks();
        return true;
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

    void Scene::serializeEntity(EntityID entity, Serializer& serializer)
    {
        auto world = m_world.lock();
        if (!world)
        {
            return;
        }

        Entity e(entity, world->getRegistry());
        if (!e.isValid())
        {
            return;
        }

        // Begin entity object
        serializer.beginObject("Entity");

        // Write entity ID
        serializer.write(entity);

        // Write entity name
        serializer.writeString(e.getName());

        // Write UUID
        std::string uuidStr = e.getUUID().toString();
        serializer.writeString(uuidStr);

        // Write components (handled by entity)
        e.serialize(serializer);

        // Write tags
        std::vector<std::string> tags;
        for (const auto& tagPair : m_entitiesByTag)
        {
            const auto& entities = tagPair.second;
            if (std::find(entities.begin(), entities.end(), entity) != entities.end())
            {
                tags.push_back(tagPair.first);
            }
        }

        serializer.beginArray("tags", tags.size());
        for (const auto& tag : tags)
        {
            serializer.writeString(tag);
        }
        serializer.endArray();

        // End entity object
        serializer.endObject();
    }

    EntityID Scene::deserializeEntity(Deserializer& deserializer)
    {
        auto world = m_world.lock();
        if (!world)
        {
            return INVALID_ENTITY_ID;
        }

        // Begin entity object
        if (!deserializer.beginObject("Entity"))
        {
            return INVALID_ENTITY_ID;
        }

        // Read entity ID
        EntityID entityId;
        deserializer.read(entityId);

        // Read entity name
        std::string name;
        deserializer.readString(name);

        // Read UUID
        std::string uuidStr;
        deserializer.readString(uuidStr);

        // Create the entity with the correct UUID
        entityId = world->createEntity(name);
        Entity entity(entityId, world->getRegistry());

        // Deserialize components
        entity.deserialize(deserializer);

        // Read tags
        size_t tagCount = 0;
        if (deserializer.beginArray("tags", tagCount))
        {
            for (size_t i = 0; i < tagCount; i++)
            {
                std::string tag;
                deserializer.readString(tag);
                addTag(entityId, tag);
            }
            deserializer.endArray();
        }

        // Register the entity name
        registerEntityName(entityId, name);

        // End entity object
        deserializer.endObject();

        return entityId;
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

    void Scene::updateChunkMesh(const Voxel::ChunkCoord& coord)
    {
        // Mark the chunk as dirty
        markChunkDirty(coord);

        // This would trigger mesh regeneration in a real implementation
        // For now, just log it
        Log::debug(StringUtils::format("Chunk mesh update triggered for ({}, {}, {})",
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
                Log::error("Failed to create scene directory: " + sceneDir);
                return false;
            }
        }

        // Also ensure the chunks directory exists
        std::string chunkDir = sceneDir + "/chunks";
        if (!fs::directoryExists(chunkDir))
        {
            if (!fs::createDirectory(chunkDir))
            {
                Log::error("Failed to create chunks directory: " + chunkDir);
                return false;
            }
        }

        return true;
    }

} // namespace PixelCraft::ECS