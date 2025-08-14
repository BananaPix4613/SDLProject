// -------------------------------------------------------------------------
// World.h
// -------------------------------------------------------------------------
#pragma once

#include "Core/Subsystem.h"
#include "ECS/ComponentTypes.h"
#include "Utility/ThreadSafeQueue.h"
#include "Voxel/Chunk.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <typeindex>
#include <vector>

namespace PixelCraft::Utility::Serialization
{
    class Serializer;
    class Deserializer;
    struct SerializationResult;
    class Schema;
}

namespace PixelCraft::ECS
{

    // Forward declarations
    class Registry;
    class System;
    class Scene;
    class Entity;

    /**
     * @brief Serialization task structure for background saving
     */
    struct SerializationTask
    {
        enum class Type
        {
            SaveScene,
            SaveChunk
        };

        Type type;
        std::string path;
        Voxel::ChunkCoord chunkCoord;
    };

    /**
     * @brief Thread safety documentation for World
     *
     * The World class is thread-safe for the following operations:
     * - Multiple threads can read world state concurrently
     * - Entity creation/destruction is thread-safe
     * - System registration and execution has controlled concurrency
     * - Scene loading/saving is handled with proper synchronization
     *
     * Locking strategy:
     * - Shared mutex for world state access (highest level lock)
     * - System execution mutex for update/render cycles
     * - Task queue for background saving operations
     *
     * Thread safety guarantees:
     * - System update/render is never concurrent with system registration/removal
     * - Scene loading/unloading is never concurrent with world updates
     * - Entity operations delegate thread safety to Registry
     */
    class World : public Core::Subsystem, public std::enable_shared_from_this<World>
    {
    public:
        /**
         * @brief Constructor
         */
        World();

        /**
         * @brief Destructor, ensures proper cleanup of background threads
         */
        ~World() override;

        /**
         * @brief Initialize the World and its systems
         * @return True if initialization was successful
         * @thread_safety Thread-safe, acquires world lock
         */
        bool initialize() override;

        /**
         * @brief Update all systems
         * @param deltaTime Time elapsed since last update
         * @thread_safety Thread-safe, acquires system execution lock
         */
        void update(float deltaTime) override;

        /**
         * @brief Call render on all rendering-related systems
         * @thread_safety Thread-safe, acquires system execution lock
         */
        void render() override;

        /**
         * @brief Shutdown the world and cleanup resources
         */
        void shutdown() override;

        /**
         * @brief Get the name of the subsystem
         * @return The subsystem name as a string
         */
        std::string getName() const override
        {
            return "World";
        }

        /**
         * @brief Get the dependencies of this subsystem
         * @return Vector of the subsystem names that this subsystem depends on
         */
        std::vector<std::string> getDependencies() const override;

        /**
         * @brief Create a new entity in the world
         * @param name Optional name for the entity
         * @return ID of the created entity
         * @thread_safety Thread-safe, delegates to Registry
         */
        EntityID createEntity(const std::string &name = "");

        /**
         * @brief Destroy an entity and all its components
         * @param entity ID of the entity to destroy
         * @return True if entity was successfully destroyed
         * @thread_safety Thread-safe, delegates to Registry
         */
        bool destroyEntity(EntityID entity);

        /**
         * @brief Register a system with the world
         * @param system Shared pointer to the system to register
         * @thread_safety Thread-safe, acquires world lock
         */
        void registerSystem(std::shared_ptr<System> system);

        /**
         * @brief Get a system by type
         * @tparam T Type of system to retrieve
         * @return Shared pointer to the system, or nullptr if not found
         * @thread_safety Thread-safe, acquires shared world lock
         */
        template <typename T> std::shared_ptr<T> getSystem()
        {
            std::shared_lock lock(m_worldMutex);

            auto typeIndex = std::type_index(typeid(T));
            auto it = m_systemsByType.find(typeIndex);
            if (it != m_systemsByType.end())
            {
                return std::dynamic_pointer_cast<T>(it->second.lock());
            }

            return nullptr;
        }

        /**
         * @brief Get an entity by ID
         * @param id Entity ID to retrieve
         * @return Entity object for the specified ID
         * @thread_safety Thread-safe, delegates to Registry
         */
        Entity getEntity(EntityID id);

        /**
         * @brief Load a scene from a file
         * @param path Path to the scene file
         * @return Shared pointer to the loaded scene, or nullptr on failure
         * @thread_safety Thread-safe, acquires world lock
         */
        std::shared_ptr<Scene> loadScene(const std::string &path);

        /**
         * @brief Save the current scene to a file
         * @param path Path where the scene should be saved
         * @return True if saving was successful
         * @thread_safety Thread-safe, acquires shared world lock
         */
        bool saveScene(const std::string &path);

        /**
         * @brief Serialize the world to the given serializer
         * @param serializer The serializer to use
         * @return SerializationResult with success/failure status
         * @thread_safety Thread-safe, acquires shared world lock
         */
        Utility::Serialization::SerializationResult serialize(Utility::Serialization::Serializer &serializer) const;

        /**
         * @brief Deserialize the world from the given deserializer
         * @param deserializer The deserializer to use
         * @return SerializationResult with success/failure status
         * @thread_safety Thread-safe, acquires world lock
         */
        Utility::Serialization::SerializationResult deserialize(Utility::Serialization::Deserializer &deserializer);

        /**
         * @brief Define the schema for World serialization
         * @param schema The schema to define
         */
        static void defineSchema(Utility::Serialization::Schema &schema);

        /**
         * @brief Get the type name for serialization
         * @return The type name string
         */
        static std::string getTypeName();

        /**
         * @brief Get the registry that stores components
         * @return Shared pointer to the registry
         * @thread_safety Thread-safe, no locks needed (immutable after initialization)
         */
        std::shared_ptr<Registry> getRegistry() const
        {
            return m_registry;
        }

        /**
         * @brief Enable or disable background saving
         * @param enable True to enable background saving, false to disable
         * @thread_safety Thread-safe, atomic operation
         */
        void enableBackgroundSaving(bool enable);

        /**
         * @brief Check if background saving is enabled
         * @return True if background saving is enabled
         * @thread_safety Thread-safe, atomic operation
         */
        bool isBackgroundSavingEnabled() const
        {
            return m_savingEnabled;
        }

        /**
         * @brief Get the active scene
         * @return Shared pointer to the active scene
         * @thread_safety Thread-safe, acquires shared world lock
         */
        std::shared_ptr<Scene> getActiveScene() const
        {
            std::shared_lock lock(m_worldMutex);
            return m_activeScene;
        }

        /**
         * @brief Pause world updates
         * @thread_safety Thread-safe, atomic operation
         */
        void pause()
        {
            m_paused = true;
        }

        /**
         * @brief Resume world updates
         * @thread_safety Thread-safe, atomic operation
         */
        void resume()
        {
            m_paused = false;
        }

        /**
         * @brief Check if the world is paused
         * @return True if the world is paused
         * @thread_safety Thread-safe, atomic operation
         */
        bool isPaused() const
        {
            return m_paused;
        }

    private:
        // Core ECS data
        std::shared_ptr<Registry> m_registry;                             ///< Registry for storing components
        std::vector<std::shared_ptr<System>> m_systems;                   ///< List of registered systems
        std::map<std::type_index, std::weak_ptr<System>> m_systemsByType; ///< Map of systems by type
        std::shared_ptr<Scene> m_activeScene;                             ///< Currently active scene
        uint32_t m_version;                                               ///< World version for serialization

        // Background saving
        bool m_savingEnabled{false};                             ///< Flag for background saving
        Utility::ThreadSafeQueue<SerializationTask> m_saveQueue; ///< Queue for serialization tasks
        std::thread m_saveThread;                                ///< Background thread for saving
        std::atomic<bool> m_threadRunning{false};                ///< Flag for thread running state

        // Synchronization
        mutable std::shared_mutex m_worldMutex; ///< Mutex for world state access
        std::mutex m_systemExecutionMutex;      ///< Mutex for system execution
        std::atomic<bool> m_paused{false};      ///< Flag for paused state

        /**
         * @brief Background thread function for processing save tasks
         */
        void backgroundSaveThread();

        /**
         * @brief Sort systems based on dependencies and priorities
         * @thread_safety Assumes m_worldMutex is already locked
         */
        void sortSystems();

        /**
         * @brief Check if the system dependencies contain cycles
         * @return True if cycles are detected
         * @thread_safety Assumes m_worldMutex is already locked
         */
        bool hasCyclicDependencies();

        /**
         * @brief Notify all systems of an event
         * @param eventName Name of the event to broadcast
         * @thread_safety Thread-safe, acquires shared world lock
         */
        void notifySystems(const std::string &eventName);
    };

} // namespace PixelCraft::ECS