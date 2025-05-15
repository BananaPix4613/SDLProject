// -------------------------------------------------------------------------
// StreamingManager.cpp
// -------------------------------------------------------------------------
#include "Voxel/StreamingManager.h"
#include "Core/Logger.h"
#include "Core/Application.h"
#include "Utility/Profiler.h"
#include "Utility/Math.h"
#include "Utility/StringUtils.h"
#include "Voxel/ChunkStorage.h"

#include <algorithm>
#include <chrono>

using StringUtils = PixelCraft::Utility::StringUtils;
namespace Log = PixelCraft::Core;

namespace PixelCraft::Voxel
{

    namespace
    {
        // Constants
        constexpr size_t DEFAULT_CHUNK_MEMORY = 512 * 1024 * 1024;  // 512 MB
        constexpr size_t DEFAULT_MESH_MEMORY = 256 * 1024 * 1024;   // 256 MB
        constexpr size_t DEFAULT_RESERVE_MEMORY = 64 * 1024 * 1024; // 64 MB
        constexpr float DEFAULT_UPDATE_INTERVAL = 0.5f;             // 500ms
        constexpr int DEFAULT_WORKER_THREADS = 4;
        constexpr int MAX_QUEUED_TASKS = 1000;

        // Helper to convert ChunkCoord to vec3 for distance calculation
        glm::vec3 coordToWorldPosition(const ChunkCoord& coord)
        {
            // Assuming ChunkCoord has x, y, z and we have a constant chunk size
            constexpr int CHUNK_SIZE = 16; // This should match your actual chunk size
            return glm::vec3(coord.x * CHUNK_SIZE, coord.y * CHUNK_SIZE, coord.z * CHUNK_SIZE);
        }

        // Default estimation of chunk memory (should be replaced with more accurate calculations)
        constexpr size_t ESTIMATED_CHUNK_MEMORY = 64 * 1024;      // 64 KB per chunk
        constexpr size_t ESTIMATED_CHUNK_MESH_MEMORY = 128 * 1024; // 128 KB per mesh
    }

    StreamingManager::StreamingTask& StreamingManager::StreamingTask::operator=(const StreamingTask& other)
    {
        if (this != &other)
        {
            coord = other.coord;
            operation = other.operation;
            priority = other.priority;
            timestamp = other.timestamp;
            estimatedMemory = other.estimatedMemory;
            canceled.store(other.canceled.load());
        }
        return *this;
    }

    bool StreamingManager::StreamingTask::operator<(const StreamingTask& other) const
    {
        // Lower priority value = higher priority
        if (priority != other.priority)
        {
            return priority > other.priority; // Note the reversed comparison for priority queue
        }

        // If same priority, use timestamp (older tasks first)
        return timestamp > other.timestamp;
    }

    StreamingManager::StreamingManager()
        : m_primaryFocus(0.0f)
        , m_primaryRadius(0.0f)
        , m_nextFocusPointId(0)
        , m_shutdownThreads(false)
        , m_totalProcessedTasks(0)
        , m_totalCanceledTasks(0)
        , m_updateInterval(DEFAULT_UPDATE_INTERVAL)
        , m_timeSinceLastUpdate(0.0f)
        , m_maxQueuedTasks(MAX_QUEUED_TASKS)
        , m_initialized(false)
    {
        // Initialize memory budget with defaults
        m_memoryBudget.maxChunkMemory = DEFAULT_CHUNK_MEMORY;
        m_memoryBudget.maxMeshMemory = DEFAULT_MESH_MEMORY;
        m_memoryBudget.reserveMemory = DEFAULT_RESERVE_MEMORY;
        m_memoryBudget.currentChunkMemory = 0;
        m_memoryBudget.currentMeshMemory = 0;
    }

    StreamingManager::~StreamingManager()
    {
        if (m_initialized)
        {
            shutdown();
        }
    }

    bool StreamingManager::initialize()
    {
        if (m_initialized)
        {
            Log::warn("StreamingManager already initialized");
            return true;
        }

        Log::info("Initializing StreamingManager subsystem");

        // Get dependencies
        auto app = Core::Application::getInstance();
        m_chunkManager = app->getSubsystem<ChunkManager>();
        m_resourceManager = app->getSubsystem<Core::ResourceManager>();

        if (m_chunkManager.expired())
        {
            Log::error("StreamingManager initialization failed: ChunkManager not found");
            return false;
        }

        if (m_resourceManager.expired())
        {
            Log::error("StreamingManager initialization failed: ResourceManager not found");
            return false;
        }

        // Start worker threads
        startWorkerThreads(DEFAULT_WORKER_THREADS);

        m_initialized = true;
        return true;
    }

    void StreamingManager::update(float deltaTime)
    {
        if (!m_initialized)
        {
            return;
        }

        Utility::Profiler::getInstance()->beginSample("StreamingManager::update");

        // Accumulate time since last full update
        m_timeSinceLastUpdate += deltaTime;

        // Check if it's time for a full update of chunk requests
        if (m_timeSinceLastUpdate >= m_updateInterval)
        {
            updateChunkRequests();
            m_timeSinceLastUpdate = 0.0f;
        }

        // Process any completed tasks and update statistics
        std::lock_guard<std::mutex> lock(m_taskMutex);

        // Remove completed or canceled active tasks
        m_activeTasks.erase(
            std::remove_if(m_activeTasks.begin(), m_activeTasks.end(),
                           [](const StreamingTask& task) {
                               return task.canceled.load();
                           }),
            m_activeTasks.end()
        );

        Utility::Profiler::getInstance()->endSample();
    }

    void StreamingManager::render()
    {
        if (!m_initialized)
        {
            return;
        }

        // Debug visualization can be added here if needed
        // For example, visualizing loaded chunks with different colors based on priority
    }

    void StreamingManager::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        Log::info("Shutting down StreamingManager subsystem");

        // Stop all worker threads
        stopWorkerThreads();

        // Clear all pending tasks
        std::lock_guard<std::mutex> lock(m_taskMutex);
        while (!m_taskQueue.empty())
        {
            m_taskQueue.pop();
        }
        m_pendingTasksByChunk.clear();
        m_activeTasks.clear();

        m_initialized = false;
    }

    std::vector<std::string> StreamingManager::getDependencies() const
    {
        return {
            "ChunkManager",
            "ResourceManager"
        };
    }

    bool StreamingManager::requestChunk(const ChunkCoord& coord, Priority priority)
    {
        if (!m_initialized)
        {
            return false;
        }

        // Check if the chunk is already loaded
        auto chunkManager = m_chunkManager.lock();
        if (chunkManager && chunkManager->isChunkLoaded(coord))
        {
            return true; // Already loaded, no need to request
        }

        // Check if there's already a pending task for this chunk
        if (isChunkProcessing(coord))
        {
            // We could update priority here if needed
            return true;
        }

        // Determine if we need to load or generate
        ChunkOperation operation = ChunkOperation::Generate;

        // Check if the chunk exists on disk before deciding to generate
        if (chunkManager)
        {
            auto chunkStorage = chunkManager->getChunkStorage().get();
            if (chunkStorage && chunkStorage->chunkExists(coord))
            {
                operation = ChunkOperation::Load;
            }
        }

        return addTask(coord, operation, priority);
    }

    bool StreamingManager::requestChunkMesh(const ChunkCoord& coord, Priority priority)
    {
        if (!m_initialized)
        {
            return false;
        }

        // Ensure the chunk is loaded first
        auto chunkManager = m_chunkManager.lock();
        if (!chunkManager || !chunkManager->isChunkLoaded(coord))
        {
            // Request the chunk first, then mesh will be generated after
            return requestChunk(coord, priority);
        }

        return addTask(coord, ChunkOperation::Mesh, priority);
    }

    bool StreamingManager::requestChunkSave(const ChunkCoord& coord, Priority priority)
    {
        if (!m_initialized)
        {
            return false;
        }

        auto chunkManager = m_chunkManager.lock();
        if (!chunkManager || !chunkManager->isChunkLoaded(coord))
        {
            return false; // Can't save what isn't loaded
        }

        return addTask(coord, ChunkOperation::Save, priority);
    }

    bool StreamingManager::requestChunkUnload(const ChunkCoord& coord, Priority priority)
    {
        if (!m_initialized)
        {
            return false;
        }

        auto chunkManager = m_chunkManager.lock();
        if (!chunkManager || !chunkManager->isChunkLoaded(coord))
        {
            return true; // Already unloaded
        }

        return addTask(coord, ChunkOperation::Unload, priority);
    }

    int StreamingManager::cancelChunkTasks(const ChunkCoord& coord)
    {
        if (!m_initialized)
        {
            return 0;
        }

        int canceledCount = 0;

        std::lock_guard<std::mutex> lock(m_taskMutex);

        // Cancel any pending tasks for this chunk
        auto it = m_pendingTasksByChunk.find(coord);
        if (it != m_pendingTasksByChunk.end())
        {
            for (auto* task : it->second)
            {
                task->canceled = true;
                canceledCount++;
            }
            m_pendingTasksByChunk.erase(it);
        }

        // Mark any active tasks for this chunk as cancelled
        for (auto& task : m_activeTasks)
        {
            if (task.coord == coord && !task.canceled)
            {
                task.canceled = true;
                canceledCount++;
            }
        }

        if (canceledCount > 0)
        {
            m_totalCanceledTasks += canceledCount;
            m_taskCondition.notify_all(); // Wake up worker threads to check for cancellation
        }

        return canceledCount;
    }

    void StreamingManager::setFocusPoint(const glm::vec3& position, float radius)
    {
        m_primaryFocus = position;
        m_primaryRadius = radius;

        // This change should trigger a re-evaluation of priorities
        m_timeSinceLastUpdate = m_updateInterval; // Force update on next frame
    }

    int StreamingManager::addFocusPoint(const glm::vec3& position, float radius, Priority priority)
    {
        FocusPoint focusPoint;
        focusPoint.id = m_nextFocusPointId++;
        focusPoint.position = position;
        focusPoint.radius = radius;
        focusPoint.priority = priority;

        m_focusPoints.push_back(focusPoint);

        // Force update on next frame
        m_timeSinceLastUpdate = m_updateInterval;

        return focusPoint.id;
    }

    bool StreamingManager::removeFocusPoint(int focusPointId)
    {
        auto it = std::find_if(m_focusPoints.begin(), m_focusPoints.end(),
                               [focusPointId](const FocusPoint& fp) { return fp.id == focusPointId; });

        if (it != m_focusPoints.end())
        {
            m_focusPoints.erase(it);
            return true;
        }

        return false;
    }

    void StreamingManager::setMemoryBudget(size_t chunkMemory, size_t meshMemory, size_t reserveMemory)
    {
        std::lock_guard<std::mutex> lock(m_memoryMutex);

        m_memoryBudget.maxChunkMemory = chunkMemory;
        m_memoryBudget.maxMeshMemory = meshMemory;
        m_memoryBudget.reserveMemory = reserveMemory;

        // Log current memory usage vs budget
        Log::info(StringUtils::format("StreamingManager memory budget set: Chunk={} MB, Mesh={} MB, Reserved={} MB",
                                      chunkMemory / (1024 * 1024), meshMemory / (1024 * 1024), reserveMemory / (1024 * 1024)));
    }

    const StreamingManager::MemoryBudget& StreamingManager::getMemoryUsage() const
    {
        std::lock_guard<std::mutex> lock(m_memoryMutex);
        return m_memoryBudget;
    }

    void StreamingManager::setWorkerThreadCount(int numThreads)
    {
        if (numThreads == static_cast<int>(m_workerThreads.size()))
        {
            return; // No change needed
        }

        // stop existing threads
        stopWorkerThreads();

        // Start new threads
        startWorkerThreads(numThreads);
    }

    size_t StreamingManager::getPendingTaskCount() const
    {
        std::lock_guard<std::mutex> lock(m_taskMutex);
        return m_taskQueue.size();
    }

    size_t StreamingManager::getActiveTaskCount() const
    {
        std::lock_guard<std::mutex> lock(m_taskMutex);
        return m_activeTasks.size();
    }

    bool StreamingManager::isChunkProcessing(const ChunkCoord& coord) const
    {
        std::lock_guard<std::mutex> lock(m_taskMutex);

        // Check pending tasks
        if (m_pendingTasksByChunk.find(coord) != m_pendingTasksByChunk.end())
        {
            return true;
        }

        // Check active tasks
        for (const auto& task : m_activeTasks)
        {
            if (task.coord == coord && !task.canceled)
            {
                return true;
            }
        }

        return false;
    }

    void StreamingManager::workerThread(int threadIndex)
    {
        Log::debug("StreamingManager worker thread " + std::to_string(threadIndex) + " started");

        while (!m_shutdownThreads)
        {
            StreamingTask task;
            bool hasTask = false;

            {
                std::unique_lock<std::mutex> lock(m_taskMutex);

                // Wait for a task or shutdown signal
                m_taskCondition.wait(lock, [this] {
                    return m_shutdownThreads || !m_taskQueue.empty();
                                     });

                if (m_shutdownThreads)
                {
                    break;
                }

                if (!m_taskQueue.empty())
                {
                    // Get a copy of the top task
                    task = StreamingTask(m_taskQueue.top());
                    m_taskQueue.pop();

                    // Check if task is canceled
                    if (task.canceled)
                    {
                        continue;
                    }

                    // Remove from pending map
                    auto it = m_pendingTasksByChunk.find(task.coord);
                    if (it != m_pendingTasksByChunk.end())
                    {
                        auto taskIt = std::find_if(it->second.begin(), it->second.end(),
                                                   [&task](const StreamingTask* t) {
                                                       return t->timestamp == task.timestamp &&
                                                           t->operation == task.operation;
                                                   });
                        if (taskIt != it->second.end())
                        {
                            it->second.erase(taskIt);
                        }

                        if (it->second.empty())
                        {
                            m_pendingTasksByChunk.erase(it);
                        }
                    }

                    // Add to active tasks
                    m_activeTasks.push_back(task);
                    hasTask = true;
                }
            }

            if (hasTask)
            {
                // Process the task outside the lokc
                std::string operationName;
                switch (task.operation)
                {
                    case ChunkOperation::Load: operationName = "Load"; break;
                    case ChunkOperation::Generate: operationName = "Generate"; break;
                    case ChunkOperation::Mesh: operationName = "Mesh"; break;
                    case ChunkOperation::Save: operationName = "Save"; break;
                    case ChunkOperation::Unload: operationName = "Unload"; break;
                }

                Utility::Profiler::getInstance()->beginSample("StreamingManager::" + operationName);

                bool success = processTask(task);

                Utility::Profiler::getInstance()->endSample();

                if (success)
                {
                    m_totalProcessedTasks++;
                }
            }
        }

        Log::debug("StreamingManager worker thread " + std::to_string(threadIndex) + " stopped");
    }

    void StreamingManager::updateChunkRequests()
    {
        if (!m_initialized)
        {
            return;
        }

        Utility::Profiler::getInstance()->beginSample("StreamingManager::updateChunkRequests");

        auto chunkManager = m_chunkManager.lock();
        if (!chunkManager)
        {
            Utility::Profiler::getInstance()->endSample();
            return;
        }

        // Get chunks around primary focus point
        auto chunksInRadius = chunkManager->getChunksAroundPoint(m_primaryFocus, m_primaryRadius);

        // Determine chunks to load
        for (const auto& chunk : chunksInRadius)
        {
            ChunkCoord coord = chunk->getCoord();
            Priority priority = calculatePriority(coord);

            // Request chunk and mesh based on priority
            requestChunk(coord, priority);
            requestChunkMesh(coord, priority);
        }

        // Determine chunks to unload (those too far from any focus point
        for (const auto& [coord, chunk] : chunkManager->getLoadedChunks())
        {
            float distance = calculateDistanceToFocus(coord);

            // If chunk is too far from all focus points, schedule for unloading
            if (distance > m_primaryRadius * 1.5f)
            { // Add buffer to avoid thrashing
                requestChunkUnload(coord, Priority::Low);
            }
        }

        Utility::Profiler::getInstance()->endSample();
    }

    bool StreamingManager::processTask(StreamingTask& task)
    {
        if (task.canceled)
        {
            return false;
        }

        // Process based on operation type
        bool success = false;

        switch (task.operation)
        {
            case ChunkOperation::Load:
                success = loadChunk(task.coord, task);
                break;

            case ChunkOperation::Generate:
                success = generateChunk(task.coord, task);
                break;

            case ChunkOperation::Mesh:
                success = generateChunkMesh(task.coord, task);
                break;

            case ChunkOperation::Save:
                success = saveChunk(task.coord, task);
                break;

            case ChunkOperation::Unload:
                success = unloadChunk(task.coord, task);
                break;
        }

        return success;
    }

    bool StreamingManager::generateChunk(const ChunkCoord& coord, StreamingTask& task)
    {
        if (task.canceled)
        {
            return false;
        }

        auto chunkManager = m_chunkManager.lock();
        if (!chunkManager)
        {
            return false;
        }

        // Check memory budget
        if (!checkMemoryBudget(task.estimatedMemory, 0, task.priority <= Priority::High))
        {
            // Requeue with lower priority if not critical
            if (task.priority > Priority::Critical)
            {
                task.priority = static_cast<Priority>(static_cast<int>(task.priority) + 1);
                addTask(coord, ChunkOperation::Generate, task.priority);
            }
            return false;
        }

        // Generate the chunk
        auto chunk = chunkManager->createChunk(coord);
        if (!chunk)
        {
            Log::error(StringUtils::format("Failed to generate chunk at {},{},{}", coord.x, coord.y, coord.z));
            return false;
        }

        // Update memory usage
        updateMemoryUsage(task.estimatedMemory, 0);

        // Request mesh generation automatically
        requestChunkMesh(coord, task.priority);

        return true;
    }

    bool StreamingManager::loadChunk(const ChunkCoord& coord, StreamingTask& task)
    {
        if (task.canceled)
        {
            return false;
        }

        auto chunkManager = m_chunkManager.lock();
        if (!chunkManager)
        {
            return false;
        }

        // Check memory budget
        if (!checkMemoryBudget(task.estimatedMemory, 0, task.priority <= Priority::High))
        {
            // Requeue with lower priority if not critical
            if (task.priority > Priority::Critical)
            {
                task.priority = static_cast<Priority>(static_cast<int>(task.priority) + 1);
                addTask(coord, ChunkOperation::Load, task.priority);
            }
            return false;
        }

        // Load the chunk
        auto chunk = chunkManager->loadChunk(coord);
        if (!chunk)
        {
            Log::error(StringUtils::format("Failed to load chunk at {},{},{}", coord.x, coord.y, coord.z));

            // If loading failed but chunk should exist, try generating instead
            auto chunkStorage = chunkManager->getChunkStorage().get();
            if (chunkStorage && chunkStorage->chunkExists(coord))
            {
                Log::warn("Chunk file exists but failed to load, attempting generation");
                return generateChunk(coord, task);
            }

            return false;
        }

        // Update memory usage
        updateMemoryUsage(task.estimatedMemory, 0);

        // Request mesh generation automatically
        requestChunkMesh(coord, task.priority);

        return true;
    }

    bool StreamingManager::generateChunkMesh(const ChunkCoord& coord, StreamingTask& task)
    {
        if (task.canceled)
        {
            return false;
        }

        auto chunkManager = m_chunkManager.lock();
        if (!chunkManager)
        {
            return false;
        }

        // Check if chunk is loaded
        auto chunk = chunkManager->getChunk(coord);
        if (!chunk)
        {
            Log::debug(StringUtils::format("Cannot generate mesh for unloaded chunk at {},{},{}", coord.x, coord.y, coord.z));
            return false;
        }

        // Check memory budget
        if (!checkMemoryBudget(0, task.estimatedMemory, task.priority <= Priority::High))
        {
            // Requeue with lower priority if not critical
            if (task.priority > Priority::Critical)
            {
                task.priority = static_cast<Priority>(static_cast<int>(task.priority) + 1);
                addTask(coord, ChunkOperation::Mesh, task.priority);
            }
            return false;
        }

        // Generate mesh
        bool success = chunk->generateMesh();

        if (success)
        {
            // Update memory usage
            updateMemoryUsage(0, task.estimatedMemory);
        }

        return success;
    }

    bool StreamingManager::saveChunk(const ChunkCoord& coord, StreamingTask& task)
    {
        if (task.canceled)
        {
            return false;
        }

        auto chunkManager = m_chunkManager.lock();
        if (!chunkManager)
        {
            return false;
        }

        // Check if chunk is loaded
        auto chunk = chunkManager->getChunk(coord);
        if (!chunk)
        {
            Log::debug(StringUtils::format("Cannot save unloaded chunk at {},{},{}", coord.x, coord.y, coord.z));
            return false;
        }

        // Save the chunk
        return chunkManager->serializeChunk(*chunk);
    }

    bool StreamingManager::unloadChunk(const ChunkCoord& coord, StreamingTask& task)
    {
        if (task.canceled)
        {
            return false;
        }

        auto chunkManager = m_chunkManager.lock();
        if (!chunkManager)
        {
            return false;
        }

        // Check if chunk is loaded
        auto chunk = chunkManager->getChunk(coord);
        if (!chunk)
        {
            return true; // Already unloaded
        }

        // Save if modified
        if (chunk->isDirty())
        {
            saveChunk(coord, task);
        }

        // Calculate memory to free
        size_t chunkMemory = chunk->getMemoryUsage();
        size_t meshMemory = chunk->getMesh() ? chunk->getMesh()->getMemoryUsage() : 0;

        // Unload the chunk
        bool success = chunkManager->unloadChunk(coord);

        if (success)
        {
            // Update memory usage (negative to reduce)
            updateMemoryUsage(-static_cast<int64_t>(chunkMemory), -static_cast<int64_t>(meshMemory));
        }

        return success;
    }

    StreamingManager::Priority StreamingManager::calculatePriority(const ChunkCoord& coord) const
    {
        float distance = calculateDistanceToFocus(coord);

        // Convert distance to priority based on thresholds
        if (distance < m_primaryRadius * 0.3f)
        {
            return Priority::Critical;
        }
        else if (distance < m_primaryRadius * 0.6f)
        {
            return Priority::High;
        }
        else if (distance < m_primaryRadius * 0.9f)
        {
            return Priority::Medium;
        }
        else
        {
            return Priority::Low;
        }
    }

    bool StreamingManager::checkMemoryBudget(size_t chunkMemory, size_t meshMemory, bool isHighPriority)
    {
        std::lock_guard<std::mutex> lock(m_memoryMutex);

        // Calculate total memory required
        size_t totalChunkRequired = m_memoryBudget.currentChunkMemory + chunkMemory;
        size_t totalMeshRequired = m_memoryBudget.currentMeshMemory + meshMemory;

        // High priority operations can use reserved memory
        size_t chunkLimit = m_memoryBudget.maxChunkMemory;
        size_t meshLimit = m_memoryBudget.maxMeshMemory;

        if (isHighPriority)
        {
            chunkLimit += m_memoryBudget.reserveMemory / 2;
            meshLimit += m_memoryBudget.reserveMemory / 2;
        }

        // Check if we have enough budget
        return (totalChunkRequired <= chunkLimit) && (totalMeshRequired <= meshLimit);
    }

    void StreamingManager::updateMemoryUsage(int64_t chunkDelta, int64_t meshDelta)
    {
        std::lock_guard<std::mutex> lock(m_memoryMutex);

        // Update current usage
        if (chunkDelta > 0)
        {
            m_memoryBudget.currentChunkMemory += chunkDelta;
        }
        else
        {
            m_memoryBudget.currentChunkMemory -= std::min(m_memoryBudget.currentChunkMemory, static_cast<size_t>(-chunkDelta));
        }

        if (meshDelta > 0)
        {
            m_memoryBudget.currentMeshMemory += meshDelta;
        }
        else
        {
            m_memoryBudget.currentMeshMemory -= std::min(m_memoryBudget.currentMeshMemory, static_cast<size_t>(-meshDelta));
        }
    }

    float StreamingManager::calculateDistanceToFocus(const ChunkCoord& coord) const
    {
        glm::vec3 chunkWorldPos = coordToWorldPosition(coord);

        // Start with distance to primary focus
        float minDistance = glm::distance(chunkWorldPos, m_primaryFocus);

        // Check all secondary focus points
        for (const auto& focusPoint : m_focusPoints)
        {
            float dist = glm::distance(chunkWorldPos, focusPoint.position);
            minDistance = glm::min(minDistance, dist);
        }

        return minDistance;
    }

    size_t StreamingManager::estimatedChunkMemory(const ChunkCoord& coord) const
    {
        // In a real implementation, this might consider chunk contents or terrain complexity
        // For simplicity, we use a constant estimation
        return ESTIMATED_CHUNK_MEMORY;
    }

    size_t StreamingManager::estimateChunkMeshMemory(const ChunkCoord& coord) const
    {
        // In a real implementation, this might vary based on chunk contents
        // For simplicity, we use a constant estimation
        return ESTIMATED_CHUNK_MESH_MEMORY;
    }

    bool StreamingManager::addTask(const ChunkCoord& coord, ChunkOperation operation, Priority priority)
    {
        std::lock_guard<std::mutex> lock(m_taskMutex);

        // Check if we have too many tasks queued
        if (m_taskQueue.size() >= static_cast<size_t>(m_maxQueuedTasks) && priority > Priority::High)
        {
            return false;
        }

        // Create the task
        StreamingTask task;
        task.coord = coord;
        task.operation = operation;
        task.priority = priority;
        task.timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        task.canceled = false;

        // Estimate memory requirements
        if (operation == ChunkOperation::Load || operation == ChunkOperation::Generate)
        {
            task.estimatedMemory = estimatedChunkMemory(coord);
        }
        else if (operation == ChunkOperation::Mesh)
        {
            task.estimatedMemory = estimateChunkMeshMemory(coord);
        }
        else
        {
            task.estimatedMemory = 0;
        }

        // Add the task to the queue
        m_taskQueue.push(task);

        // Store a pointer to this task in the pending map
        // This is a simplification that works because we have direct access to the task
        // In a real implementation, you'd need a more robust way to track tasks by coordinate
        if (!m_taskQueue.empty())
        {
            StreamingTask* taskPtr = const_cast<StreamingTask*>(&m_taskQueue.top());
            m_pendingTasksByChunk[coord].push_back(taskPtr);
        }

        // Notify one worker thread
        m_taskCondition.notify_one();

        return true;
    }

    void StreamingManager::startWorkerThreads(int numThreads)
    {
        m_shutdownThreads = false;

        for (int i = 0; i < numThreads; i++)
        {
            m_workerThreads.emplace_back(&StreamingManager::workerThread, this, i);
        }

        Log::info("StreamingManager started " + std::to_string(numThreads) + " worker threads");
    }

    void StreamingManager::stopWorkerThreads()
    {
        // Signal threads to stop
        m_shutdownThreads = true;
        m_taskCondition.notify_all();

        // Wait for all threads to finish
        for (auto& thread : m_workerThreads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }

        m_workerThreads.clear();
        Log::info("StreamingManager stopped all worker threads");
    }

} // namespace PixelCraft::Voxel