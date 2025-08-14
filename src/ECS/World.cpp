// -------------------------------------------------------------------------
// World.cpp
// -------------------------------------------------------------------------
#include "ECS/World.h"
#include "ECS/Registry.h"
#include "ECS/System.h"
#include "ECS/Scene.h"
#include "ECS/Entity.h"
#include "Utility/Serialization/Serializer.h"
#include "Utility/Serialization/Deserializer.h"
#include "Core/Logger.h"

namespace PixelCraft::ECS
{

    World::World() : m_savingEnabled(false), m_threadRunning(false), m_paused(false)
    {
        // Create the registry
        m_registry = std::make_shared<Registry>();
        
        // Initialize Subsystem base class members
        m_initialized = false;
        m_active = true;
    }

    World::~World()
    {
        // Shut down the background save thread if it's running
        if (m_threadRunning)
        {
            m_threadRunning = false;
            m_saveQueue.shutdown();

            // Wait for the thread to finish
            if (m_saveThread.joinable())
            {
                m_saveThread.join();
            }
        }

        // Acquire exclusive lock for cleanup
        std::unique_lock lock(m_worldMutex);

        // Clear all systems
        m_systems.clear();
        m_systemsByType.clear();

        // Clear the active scene and registry
        m_activeScene.reset();
        m_registry.reset();
    }

    bool World::initialize()
    {
        std::unique_lock lock(m_worldMutex);

        // Initialize all registered systems
        bool success = true;
        for (auto& system : m_systems)
        {
            if (!system->initialize())
            {
                Core::error("Failed to initialize system: " + system->getTypeName());
                success = false;
            }
        }

        m_initialized = success;
        return success;
    }

    void World::update(float deltaTime)
    {
        // Early out if paused
        if (m_paused)
        {
            return;
        }

        // Use a system execution lock to prevent concurrent update/render
        std::lock_guard executionLock(m_systemExecutionMutex);

        // Get a copy of the systems vector to avoid holding the world lock during update
        std::vector<std::shared_ptr<System>> systemsCopy;
        {
            std::shared_lock worldLock(m_worldMutex);
            systemsCopy = m_systems;
        }

        // Update all systems in priority order
        for (auto& system : m_systems)
        {
            if (system && system->isActive())
            {
                system->update(deltaTime);
            }
        }
    }

    void World::render()
    {
        // Early out if paused
        if (m_paused)
        {
            return;
        }

        // Use a system execution lock to prevent concurrent update/render
        std::lock_guard executionLock(m_systemExecutionMutex);

        // Get a copy of the systems vector to avoid holding the world lock during render
        std::vector<std::shared_ptr<System>> systemsCopy;
        {
            std::shared_lock worldLock(m_worldMutex);
            systemsCopy = m_systems;
        }

        // Call render on all rendering systems
        for (auto& system : m_systems)
        {
            if (system && system->isActive())
            {
                system->render();
            }
        }
    }

    void World::shutdown()
    {
        // Shut down the background save thread if it's running
        if (m_threadRunning)
        {
            m_threadRunning = false;
            m_saveQueue.shutdown();

            // Wait for the thread to finish
            if (m_saveThread.joinable())
            {
                m_saveThread.join();
            }
        }

        // Acquire exclusive lock for cleanup
        std::unique_lock lock(m_worldMutex);

        // Clear all systems
        m_systems.clear();
        m_systemsByType.clear();

        // Clear the active scene and registry
        m_activeScene.reset();
        m_registry.reset();

        m_initialized = false;
        m_active = false;
    }

    std::vector<std::string> World::getDependencies() const
    {
        return {"MemoryManager"};
    }

    EntityID World::createEntity(const std::string& name)
    {
        // Delegate to Registry
        EntityID entity = m_registry->createEntity(true); // Generate UUID by default

        // Set the entity name if provided
        if (!name.empty())
        {
            m_registry->setEntityName(entity, name);
        }

        // Notify systems about entity creation
        notifySystems("entityCreated");

        return entity;
    }

    bool World::destroyEntity(EntityID entity)
    {
        // Notify systems before destroying the entity
        notifySystems("EntityDestroyed");

        // Remove from active scene if present
        {
            std::shared_lock lock(m_worldMutex);
            if (m_activeScene)
            {
                m_activeScene->removeEntity(entity);
            }
        }

        // Delegate to Registry
        return m_registry->destroyEntity(entity);
    }

    void World::registerSystem(std::shared_ptr<System> system)
    {
        std::unique_lock lock(m_worldMutex);

        if (!system)
        {
            Core::error("World::registerSystem: Null system pointer");
            return;
        }

        // Set the system's world reference using weak_ptr
        system->setWorld(weak_from_this());

        // Add to systems list
        m_systems.push_back(system);

        // Add to type map for fast lookups
        auto typeIndex = system->getTypeIndex();
        m_systemsByType[typeIndex] = system;

        // Re-sort systems based on dependencies
        sortSystems();

        Core::info("Registered system: " + system->getTypeName());
    }

    Entity World::getEntity(EntityID id)
    {
        // Create and return an Entity object for the given ID
        return Entity(id, m_registry);
    }

    std::shared_ptr<Scene> World::loadScene(const std::string& path)
    {
        std::unique_lock lock(m_worldMutex);

        // Notify systems about scene unloading
        notifySystems("SceneUnloading");

        // Clear current scene data
        m_activeScene.reset();

        // Create new scene
        auto scene = std::make_shared<Scene>();
        scene->setWorld(weak_from_this());

        // Load the scene from file
        if (!scene->load(path))
        {
            Core::error("Failed to load scene: " + path);
            return nullptr;
        }

        // Set as active scene
        m_activeScene = scene;

        // Notify systems about scene loaded
        notifySystems("SceneLoaded");

        return scene;
    }

    bool World::saveScene(const std::string& path)
    {
        // Get the active scene
        std::shared_ptr<Scene> scene;
        {
            std::shared_lock lock(m_worldMutex);
            scene = m_activeScene;
        }

        if (!scene)
        {
            Core::error("World::saveScene: No active scene to save");
            return false;
        }

        // If background saving is enabled, queue the save operation
        if (m_savingEnabled)
        {
            SerializationTask task;
            task.type = SerializationTask::Type::SaveScene;
            task.path = path;
            m_saveQueue.push(task);
            return true;
        }

        // Otherwise perform save immediately
        return scene->save(path);
    }

    Utility::Serialization::SerializationResult World::serialize(Utility::Serialization::Serializer& serializer) const
    {
        std::shared_lock lock(m_worldMutex);

        // Begin serializing the world
        auto result = serializer.beginObject(getTypeName());
        if (!result) return result;

        // Write version info
        result = serializer.writeField("version", m_version);
        if (!result) return result;

        // Write active scene reference
        result = serializer.writeField("activeScenePath", m_activeScene->getPath());
        if (!result) return result;

        // Serialize the registry with all entities and components
        result = m_registry->serializeAll(serializer);
        if (!result) return result;

        result = serializer.beginArray("systems", m_systems.size());
        if (!result) return result;

        // Call serialize on all systems
        for (auto& system : m_systems)
        {
            if (system && system->shouldSerialize())
            {
                system->serialize(const_cast<World&>(*this), serializer);
            }
        }

        result = serializer.endArray();
        if (!result) return result;

        // End world object
        return serializer.endObject();
    }

    Utility::Serialization::SerializationResult World::deserialize(Utility::Serialization::Deserializer& deserializer)
    {
        std::unique_lock lock(m_worldMutex);

        // Begin deserializing the world
        auto result = deserializer.beginObject(getTypeName());
        if (!result) return result;

        // Read version info
        result = deserializer.readField("version", m_version);
        if (!result) return result;

        // Deserialize active scene path
        std::string scenePath;
        result = deserializer.readField("activeScenePath", scenePath);
        if (!result) return result;

        // Deserialize the registry with all entities and components
        result = m_registry->deserializeAll(deserializer);
        if (!result) return result;

        size_t arraySize;
        // Begin deserializing systems
        result = deserializer.beginArray("systems", arraySize);
        if (!result) return result;

        // Deserialize system-specific data
        for (auto& system : m_systems)
        {
            if (system && system->shouldSerialize())
            {
                system->deserialize(*this, deserializer);
            }
        }

        result = deserializer.endArray();
        if (!result) return result;

        // Load active scene if path is not empty
        if (!scenePath.empty())
        {
            loadScene(scenePath);
        }

        // End world object
        result = deserializer.endObject();
        if (!result) return result;

        // Notify systems about world deserialization
        notifySystems("worldDeserialized");

        return result;
    }

    void World::defineSchema(Utility::Serialization::Schema& schema)
    {
        schema.addField("version", Utility::Serialization::ValueType::UInt32);
        schema.addField("activeScenePath", Utility::Serialization::ValueType::String);
        schema.addField("registry", Utility::Serialization::ValueType::Object);
        schema.addArrayField("systems", Utility::Serialization::ValueType::Object);
    }

    std::string World::getTypeName()
    {
        return "World";
    }

    void World::enableBackgroundSaving(bool enable)
    {
        // If already in the requested state, do nothing
        if (m_savingEnabled == enable)
        {
            return;
        }

        m_savingEnabled = enable;

        if (enable)
        {
            // Start the background save thread if it's not running
            if (!m_threadRunning)
            {
                m_threadRunning = true;
                m_saveThread = std::thread(&World::backgroundSaveThread, this);
            }
        }
        else
        {
            // Signal the thread to stop and wait for it
            if (m_threadRunning)
            {
                m_threadRunning = false;
                m_saveQueue.shutdown();

                if (m_saveThread.joinable())
                {
                    m_saveThread.join();
                }
            }
        }
    }

    void World::backgroundSaveThread()
    {
        Core::info("Background save thread started");

        while (m_threadRunning)
        {
            SerializationTask task;

            // Wait for a task from the queue
            if (!m_saveQueue.waitAndPop(task))
            {
                break; // Queue was shut down
            }

            // Process the task based on its type
            switch (task.type)
            {
                case SerializationTask::Type::SaveScene:
                {
                    std::shared_ptr<Scene> scene;
                    {
                        std::shared_lock lock(m_worldMutex);
                        scene = m_activeScene;
                    }

                    if (scene)
                    {
                        // Use the scene's save method directly
                        scene->save(task.path);
                    }
                    break;
                }

                case SerializationTask::Type::SaveChunk:
                {
                    std::shared_ptr<Scene> scene;
                    {
                        std::shared_lock lock(m_worldMutex);
                        scene = m_activeScene;
                    }

                    if (scene)
                    {
                        scene->serializeChunk(task.chunkCoord);
                    }
                    break;
                }
            }
        }

        Core::info("Background save thread stopped");
    }

    void World::sortSystems()
    {
        // Check for cyclic dependencies first
        if (hasCyclicDependencies())
        {
            Core::error("Cyclic dependencies detected in systems");
            return;
        }

        // Sort systems based on priority and dependencies
        std::sort(m_systems.begin(), m_systems.end(), [](const std::shared_ptr<System>& a, const std::shared_ptr<System>& b) {
            // Higher priority comes first
            if (a->getPriority() != b->getPriority())
            {
                return a->getPriority() > b->getPriority();
            }

            // Check dependencies
            auto aDeps = a->getDependencies();
            auto bDeps = b->getDependencies();

            // If a depends on b, b should come first
            if (std::find(aDeps.begin(), aDeps.end(), b->getTypeName()) != aDeps.end())
            {
                return false;
            }

            // If b depends on a, a should come first
            if (std::find(bDeps.begin(), bDeps.end(), a->getTypeName()) != bDeps.end())
            {
                return true;
            }

            // Otherwise, order by name for stability
            return a->getTypeName() < b->getTypeName();
                  });
    }

    bool World::hasCyclicDependencies()
    {
        // Implementation of cycle detection algorithm (DFS)
        std::map<std::string, bool> visited;
        std::map<std::string, bool> inStack;

        // Initialize all systems as not visited
        for (const auto& system : m_systems)
        {
            visited[system->getTypeName()] = false;
            inStack[system->getTypeName()] = false;
        }

        // Helper function for DFS cycle detection
        std::function<bool(const std::string&)> hasCycle = [&](const std::string& systemName) -> bool {
            // Mark current system as visited and add to recursion stack
            visited[systemName] = true;
            inStack[systemName] = true;

            // Find the system by name
            auto system = std::find_if(m_systems.begin(), m_systems.end(), [&](const std::shared_ptr<System>& s) {
                return s->getTypeName() == systemName;
                                       });

            if (system != m_systems.end())
            {
                // Check all dependencies
                for (const auto& dependentName : (*system)->getDependencies())
                {
                    // If dependent is not visited, check if it leads to cycle
                    if (!visited[dependentName] && hasCycle(dependentName))
                    {
                        return true;
                    }
                    // If dependent is already in recursion stack, we found a cycle
                    else if (inStack[dependentName])
                    {
                        return true;
                    }
                }
            }

            // Remove from recursion stack
            inStack[systemName] = false;
            return false;
            };

        // Check for cycles starting from each system
        for (auto& system : m_systems)
        {
            if (!visited[system->getTypeName()] && hasCycle(system->getTypeName()))
            {
                return true;
            }
        }

        return false;
    }

    void World::notifySystems(const std::string& eventName)
    {
        // Get a copy of the systems vector to avoid holding the lock during event processing
        std::vector<std::shared_ptr<System>> systemsCopy;
        {
            std::shared_lock lock(m_worldMutex);
            systemsCopy = m_systems;
        }

        // Notify each system
        for (auto& system : systemsCopy)
        {
            system->onEvent(eventName);
        }
    }

} // namespace PixelCraft::ECS