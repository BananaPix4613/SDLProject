// -------------------------------------------------------------------------
// StreamingManager.h
// -------------------------------------------------------------------------
#pragma once

#include "Core/Subsystem.h"
#include "Core/ThreadSafeQueue.h"
#include "Voxel/ChunkManager.h"
#include "Voxel/Chunk.h"
#include "Voxel/ChunkMesh.h"
#include "Core/ResourceManager.h"
#include "glm/glm.hpp"

#include <memory>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <unordered_map>
#include <condition_variable>
#include <functional>

namespace PixelCraft::Voxel
{

    // Forward declarations
    struct ChunkCoord;

    /**
     * @brief Priority-based streaming system for dynamic chunk loading
     * 
     * StreamingManager handles intelligent loading, unloading, and saving of voxel chunks
     * based on player position, memory constraints, and gameplay importance. It implements
     * a priority-based queue system with support for background processing and memory budgeting
     */
    class StreamingManager : public Core::Subsystem
    {
    public:
        /**
         * @brief Defines operations that can be performed on chunks
         */
        enum class ChunkOperation
        {
            Load,       ///< Load a chunk from storage
            Generate,   ///< Generate a new chunk
            Mesh,       ///< Generate mesh for a chunk
            Save,       ///< Save a chunk to storage
            Unload      ///< Unload a chunk from memory
        };

        /**
         * @brief Priority levels for chunk operations
         */
        enum class Priority
        {
            Critical = 0,   ///< Highest priority (camera position)
            High = 1,       ///< Important chunks (near player, gameplay)
            Medium = 2,     ///< Standard priority chunks
            Low = 3,        ///< Background loading with no urgency
            VeryLow = 4     ///< Only process when idle
        };

        /**
         * @brief Streaming task definition
         */
        struct StreamingTask
        {
            ChunkCoord coord;             ///< Coordinate of the chunk
            ChunkOperation operation;     ///< Operation to perform
            Priority priority;            ///< Task priority
            uint64_t timestamp;           ///< When the task was created
            size_t estimatedMemory;       ///< Estimated memory usage
            std::atomic<bool> canceled;   ///< Whether the task is canceled

            /**
             * @brief Default constructor
             */
            StreamingTask() = default;

            /**
             * @brief Copy constructor
             */
            StreamingTask(const StreamingTask& other)
                : coord(other.coord)
                , operation(other.operation)
                , priority(other.priority)
                , timestamp(other.timestamp)
                , estimatedMemory(other.estimatedMemory)
                , canceled(other.canceled.load())
            {
            }

            /**
             * @brief Assignment operator
             */
            StreamingTask& operator=(const StreamingTask& other);

            /**
             * @brief Comparison operator for priority queue
             */
            bool operator<(const StreamingTask& other) const;
        };

        /**
         * @brief Memory budget constraints
         */
        struct MemoryBudget
        {
            size_t maxChunkMemory;      ///< Maximum memory for chunks
            size_t maxMeshMemory;       ///< Maximum memory for chunk meshes
            size_t reserveMemory;       ///< Reserved memory for critical operations

            size_t currentChunkMemory;  ///< Current chunk memory usage
            size_t currentMeshMemory;   ///< Current mesh memory usage
        };

        /**
         * @brief Constructor
         */
        StreamingManager();

        /**
         * @brief Destructor
         */
        virtual ~StreamingManager();

        /**
         * @brief Initialize the streaming system
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Update the streaming system based on player position and priorities
         * @param deltaTime Time since last update
         */
        void update(float deltaTime) override;

        /**
         * @brief Render debug visualization (if applicable)
         */
        void render() override;

        /**
         * @brief Shut down the streaming system and release resources
         */
        void shutdown() override;

        /**
         * @brief Get the name of the subsystem
         * @return The subsystem name
         */
        std::string getName() const override
        {
            return "StreamingManager";
        }

        /**
         * @brief Get the subsystem dependencies
         * @return Vector of dependency subsystem names
         */
        std::vector<std::string> getDependencies() const override;

        /**
         * @brief Request a chunk to be loaded or generated with specified priority
         * @param coord Chunk coordinate
         * @param priority Operation priority
         * @return True if the request was accepted
         */
        bool requestChunk(const ChunkCoord& coord, Priority priority = Priority::Medium);

        /**
         * @brief Request chunk mesh generation with specified priority
         * @param coord Chunk coordinate
         * @param priority Operation priority
         * @return True if the request was accepted
         */
        bool requestChunkMesh(const ChunkCoord& coord, Priority priority = Priority::Medium);

        /**
         * @brief Request a chunk to be saved
         * @param coord Chunk coordinate
         * @param priority Operation priority
         * @return True if the request was accepted
         */
        bool requestChunkSave(const ChunkCoord& coord, Priority priority = Priority::Low);

        /**
         * @brief Request a chunk to be unloaded
         * @param coord Chunk coordinate
         * @param priority Operation priority
         * @return True if the request was accepted
         */
        bool requestChunkUnload(const ChunkCoord& coord, Priority priority = Priority::Low);

        /**
         * @brief Cancel all pending tasks for a specific chunk
         * @param coord Chunk coordinate
         * @return Number of tasks canceled
         */
        int cancelChunkTasks(const ChunkCoord& coord);

        /**
         * @brief Set the focus point for streaming operations (usually player position)
         * @param position World position to focus on
         * @param radius Primary streaming radius
         */
        void setFocusPoint(const glm::vec3& position, float radius);

        /**
         * @brief Add a secondary focus point (important gameplay area)
         * @param position World position of secondary focus
         * @param radius Streaming radius for this point
         * @param priority Priority for this region
         * @return ID of the focus point for later removal
         */
        int addFocusPoint(const glm::vec3& position, float radius, Priority priority = Priority::High);

        /**
         * @brief Remove a secondary focus point
         * @param id ID of the focus point to remove
         * @return True if the focus point was removed
         */
        bool removeFocusPoint(int focusPointId);

        /**
         * @brief Set memory budget constraints
         * @param chunkMemory Maximum memory for chunks (in bytes)
         * @param meshMemory Maximum memory for chunk meshes (in bytes)
         * @param reserveMemory Memory reserved for critical operations (in bytes)
         */
        void setMemoryBudget(size_t chunkMemory, size_t meshMemory, size_t reserveMemory);

        /**
         * @brief Get current memory usage statistics
         * @return Current memory budget state
         */
        const MemoryBudget& getMemoryUsage() const;

        /**
         * @brief Set the number of worker threads for background operations
         * @param numThreads Number of threads to use
         */
        void setWorkerThreadCount(int numThreads);

        /**
         * @brief Get the number of pending tasks
         * @return Total pending task count
         */
        size_t getPendingTaskCount() const;

        /**
         * @brief Get the number of active tasks
         * @return Currently executing task count
         */
        size_t getActiveTaskCount() const;

        /**
         * @brief Check if a chunk is being processed
         * @param coord Chunk coordinate
         * @return True if any task for this chunk is pending or active
         */
        bool isChunkProcessing(const ChunkCoord& coord) const;

    private:
        /**
         * @brief Secondary focus point definition
         */
        struct FocusPoint
        {
            int id;                   ///< Unique identifier
            glm::vec3 position;       ///< World position
            float radius;             ///< Influence radius
            Priority priority;        ///< Priority level
        };

        /**
         * @brief Worker thread function for processing tasks
         * @param threadIndex Index of this worker thread
         */
        void workerThread(int threadIndex);

        /**
         * @brief Evaluate chunks around focus points for loading/unloading
         */
        void updateChunkRequests();

        /**
         * @brief Process a single streaming task
         * @param task Task to process
         * @return True if the task was processed successfully
         */
        bool processTask(StreamingTask& task);

        /**
         * @brief Generate a new chunk
         * @param coord Chunk coordinate
         * @param task Task reference for cancellation check
         * @return True if generation was successful
         */
        bool generateChunk(const ChunkCoord& coord, StreamingTask& task);

        /**
         * @brief Load a chunk from storage
         * @param coord Chunk coordinate
         * @param task Task reference for cancellation check
         * @return True if loading was successful
         */
        bool loadChunk(const ChunkCoord& coord, StreamingTask& task);

        /**
         * @brief Generate mesh for a chunk
         * @param coord Chunk coordinate
         * @param task Task reference for cancellation check
         * @return True if mesh generation was successful
         */
        bool generateChunkMesh(const ChunkCoord& coord, StreamingTask& task);

        /**
         * @brief Save a chunk to storage
         * @param coord Chunk coordinate
         * @param task Task reference for cancellation check
         * @return True if saving was successful
         */
        bool saveChunk(const ChunkCoord& coord, StreamingTask& task);

        /**
         * @brief Unload a chunk from memory
         * @param coord Chunk coordinate
         * @param task Task reference for cancellation check
         * @return True if unloading was successful
         */
        bool unloadChunk(const ChunkCoord& coord, StreamingTask& task);

        /**
         * @brief Calculate priority based on distance to focus points
         * @param coord Chunk coordinate
         * @return Calculated priority level
         */
        Priority calculatePriority(const ChunkCoord& coord) const;

        /**
         * @brief Check if there is enough budget for a new resource
         * @param chunkMemory Required chunk memory (bytes)
         * @param meshMemory Required mesh memory (bytes)
         * @param isHighPriority Whether this is a high priority operation
         * @return True if budget allows the operation
         */
        bool checkMemoryBudget(size_t chunkMemory, size_t meshMemory, bool isHighPriority);

        /**
         * @brief Update memory usage statistics
         * @param chunkDelta Change in chunk memory (bytes, can be negative)
         * @param meshDelta Change in mesh memory (bytes, can be negative)
         */
        void updateMemoryUsage(int64_t chunkDelta, int64_t meshDelta);

        /**
         * @brief Calculate distance from chunk to nearest focus point
         * @param coord Chunk coordinate
         * @return Distance to nearest focus point
         */
        float calculateDistanceToFocus(const ChunkCoord& coord) const;

        /**
         * @brief Estimate memory requirements for a chunk
         * @param coord Chunk coordinate
         * @return Estimated memory size in bytes
         */
        size_t estimatedChunkMemory(const ChunkCoord& coord) const;

        /**
         * @brief Estimate memory requirements for a chunk mesh
         * @param coord Chunk coordinate
         * @return Estimated memory size in bytes
         */
        size_t estimateChunkMeshMemory(const ChunkCoord& coord) const;

        /**
         * @brief Add a task to the priority queue
         * @param coord Chunk coordinate
         * @param operation Operation type
         * @param priority Priority level
         * @return True if task was added successfully
         */
        bool addTask(const ChunkCoord& coord, ChunkOperation operation, Priority priority);

        /**
         * @brief Start worker threads
         * @param numThreads Number of threads to start
         */
        void startWorkerThreads(int numThreads);

        /**
         * @brief Stop all worker threads
         */
        void stopWorkerThreads();

        // Primary focus point
        glm::vec3 m_primaryFocus;
        float m_primaryRadius;

        // Secondary focus points
        std::vector<FocusPoint> m_focusPoints;
        int m_nextFocusPointId;

        // Task management
        std::priority_queue<StreamingTask> m_taskQueue;
        std::unordered_map<ChunkCoord, std::vector<StreamingTask*>> m_pendingTasksByChunk;
        std::vector<StreamingTask> m_activeTasks;
        mutable std::mutex m_taskMutex;

        // Memory budget
        MemoryBudget m_memoryBudget;
        mutable std::mutex m_memoryMutex;

        // Worker threads
        std::vector<std::thread> m_workerThreads;
        std::atomic<bool> m_shutdownThreads;
        std::condition_variable m_taskCondition;

        // Dependencies
        std::weak_ptr<ChunkManager> m_chunkManager;
        std::weak_ptr<Core::ResourceManager> m_resourceManager;

        // Statistics
        std::atomic<size_t> m_totalProcessedTasks;
        std::atomic<size_t> m_totalCanceledTasks;

        // Configuration
        float m_updateInterval;
        float m_timeSinceLastUpdate;
        int m_maxQueuedTasks;
        bool m_initialized;
    };

} // namespace PixelCraft::Voxel