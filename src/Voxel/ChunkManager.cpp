// -------------------------------------------------------------------------
// ChunkManager.cpp
// -------------------------------------------------------------------------
#include "Voxel/ChunkManager.h"
#include "Voxel/ChunkStorage.h"
#include "Voxel/StreamingManager.h"
#include "Core/Logger.h"
#include "Core/Application.h"
#include "Utility/Profiler.h"
#include "Utility/FileSystem.h"
#include "Utility/StringUtils.h"

#include <algorithm>
#include <chrono>

namespace Log = PixelCraft::Core;
using StringUtils = PixelCraft::Utility::StringUtils;

namespace PixelCraft::Voxel
{
    
    namespace
    {
        constexpr int DEFAULT_CHUNK_SIZE = 16;
        constexpr int MAX_CHUNKS_PER_FRAME = 4; // Maximum number of chunks to process in a single frame
    }

    ChunkManager::ChunkManager()
        : m_chunkSize(DEFAULT_CHUNK_SIZE)
        , m_saveThreadActive(false)
        , m_initialized(false)
    {
    }

    ChunkManager::~ChunkManager()
    {
        if (m_initialized)
        {
            shutdown();
        }
    }

    bool ChunkManager::initialize()
    {
        if (m_initialized)
        {
            Log::warn("ChunkManager already initialized");
            return true;
        }

        Log::info("Initializing ChunkManager subsystem");

        // Create chunk storage with default world path
        std::string worldPath = "worlds/default";
        m_chunkStorage = std::make_shared<ChunkStorage>(worldPath);

        if (!m_chunkStorage->initialize())
        {
            Log::error("Failed to initialize chunk storage");
            return false;
        }

        // Start save thread
        m_saveThreadActive = true;
        m_saveThread = std::thread(&ChunkManager::backgroundSaveThread, this);

        // Get reference to streaming manager if available
        auto app = Core::Application::getInstance();
        m_streamingManager = app->getSubsystem<StreamingManager>();

        m_initialized = true;
        Log::info("ChunkManager initialized with chunk size: " + m_chunkSize);

        return true;
    }

    void ChunkManager::update(float deltaTime)
    {
        if (!m_initialized)
        {
            return;
        }

        Utility::Profiler::getInstance()->beginSample("ChunkManager::update");

        // Process a limited number of dirty chunks per frame to avoid hitches
        int processedCount = 0;
        std::vector<ChunkCoord> chunksToProcess;

        {
            std::lock_guard<std::mutex> lock(m_dirtyChunksMutex);

            // Make a copy of a subset of dirty chunks to process
            for (const auto& coord : m_dirtyChunks)
            {
                chunksToProcess.push_back(coord);
                if (++processedCount >= MAX_CHUNKS_PER_FRAME)
                {
                    break;
                }
            }

            // Remove processed chunks from the dirty set
            for (const auto& coord : chunksToProcess)
            {
                m_dirtyChunks.erase(coord);
            }
        }

        // Add dirty chunks to save queue
        for (const auto& coord : chunksToProcess)
        {
            auto chunk = getChunk(coord);
            if (chunk && chunk->isDirty())
            {
                m_saveQueue.push(chunk);
            }
        }

        Utility::Profiler::getInstance()->endSample();
    }

    void ChunkManager::render()
    {
        if (!m_initialized)
        {
            return;
        }

        // Debug visualization could be added here
    }

    void ChunkManager::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        Log::info("Shutting down ChunkManager subsystem");

        // Save all dirty chunks
        saveModifiedChunks();

        // Stop save thread
        m_saveThreadActive = false;
        m_saveQueue.push(nullptr); // Push null to wake up thread

        if (m_saveThread.joinable())
        {
            m_saveThread.join();
        }

        // Clear all chunks
        {
            std::lock_guard<std::mutex> lock(m_chunksMutex);
            m_chunks.clear();
        }

        {
            std::lock_guard<std::mutex> lock(m_dirtyChunksMutex);
            m_dirtyChunks.clear();
        }

        m_initialized = false;
    }

    std::vector<std::string> ChunkManager::getDependencies() const
    {
        return {
            "ResourceManager"
        };
    }

    std::shared_ptr<Chunk> ChunkManager::loadChunk(const ChunkCoord& coord)
    {
        if (!m_initialized)
        {
            Log::error("ChunkManager not initialized");
            return nullptr;
        }

        Utility::Profiler::getInstance()->beginSample("ChunkManager::loadChunk");

        // Check if chunk is already loaded
        {
            std::lock_guard<std::mutex> lock(m_chunksMutex);
            auto it = m_chunks.find(coord);
            if (it != m_chunks.end())
            {
                Utility::Profiler::getInstance()->endSample();
                return it->second; // Already loaded
            }
        }

        // Create a new chunk
        auto chunk = std::make_shared<Chunk>(coord);
        if (!chunk->initialize(m_chunkSize))
        {
            Log::error("Failed to initialize chunk at " + coord.toString());
            Utility::Profiler::getInstance()->endSample();
            return nullptr;
        }

        // Try to load from storage
        bool loaded = false;
        if (m_chunkStorage && m_chunkStorage->chunkExists(coord))
        {
            loaded = m_chunkStorage->loadChunk(coord, *chunk);
            if (!loaded)
            {
                Log::warn("Failed to load chunk data for " + coord.toString() + ", will generate new chunk");
            }
        }

        // If we couldn't load from storage, it's a new chunk (already initialized with defaults)
        if (!loaded)
        {
            chunk->markDirty(); // Mark as dirty to save later
        }

        // Add to loaded chunks map
        {
            std::lock_guard<std::mutex> lock(m_chunksMutex);
            m_chunks[coord] = chunk;
        }

        // Update neighbors
        updateChunkNeighbors(chunk);

        // If this is a new chunk, add to dirty set for later saving
        if (!loaded)
        {
            std::lock_guard<std::mutex> lock(m_dirtyChunksMutex);
            m_dirtyChunks.insert(coord);
        }

        Utility::Profiler::getInstance()->endSample();
        return chunk;
    }

    bool ChunkManager::unloadChunk(const ChunkCoord& coord)
    {
        if (!m_initialized)
        {
            Log::error("ChunkManager not initialized");
            return false;
        }

        Utility::Profiler::getInstance()->beginSample("ChunkManager::unloadChunk");

        std::shared_ptr<Chunk> chunk;

        // Get and remove the chunk from the loaded map
        {
            std::lock_guard<std::mutex> lock(m_chunksMutex);
            auto it = m_chunks.find(coord);
            if (it == m_chunks.end())
            {
                Utility::Profiler::getInstance()->endSample();
                return false; // Not loaded
            }

            chunk = it->second;
            m_chunks.erase(it);
        }

        // If chunk is dirty, save it before unloading
        if (chunk->isDirty())
        {
            serializeChunk(*chunk);
        }

        // Remove from dirty chunks set
        {
            std::lock_guard<std::mutex> lock(m_dirtyChunksMutex);
            m_dirtyChunks.erase(coord);
        }

        // Update neighbors of adjacent chunks
        std::array<ChunkCoord, 6> neighborCoords = {
            ChunkCoord(coord.x - 1, coord.y, coord.z),
            ChunkCoord(coord.x + 1, coord.y, coord.z),
            ChunkCoord(coord.x, coord.y - 1, coord.z),
            ChunkCoord(coord.x, coord.y + 1, coord.z),
            ChunkCoord(coord.x, coord.y, coord.z - 1),
            ChunkCoord(coord.x, coord.y, coord.z + 1)
        };

        for (int i = 0; i < 6; i++)
        {
            auto neighborChunk = getChunk(neighborCoords[i]);
            if (neighborChunk)
            {
                // Update the neighbor to remove reference to this chunk
                neighborChunk->setNeighbor(i ^ 1, std::weak_ptr<Chunk>());
            }
        }

        Utility::Profiler::getInstance()->endSample();
        return true;
    }

    std::shared_ptr<Chunk>ChunkManager::createChunk(const ChunkCoord& coord)
    {
        if (!m_initialized)
        {
            Log::error("ChunkManager not initialized");
            return nullptr;
        }

        Utility::Profiler::getInstance()->beginSample("ChunkManager::createChunk");

        // Check if chunk already exists
        {
            std::lock_guard<std::mutex> lock(m_chunksMutex);
            auto it = m_chunks.find(coord);
            if (it != m_chunks.end())
            {
                Utility::Profiler::getInstance()->endSample();
                return it->second; // Already exists
            }
        }

        // Create and initialize the chunk
        auto chunk = std::make_shared<Chunk>(coord);
        if (!chunk->initialize(m_chunkSize))
        {
            Log::error("Failed to initialize chunk at " + coord.toString());
            Utility::Profiler::getInstance()->endSample();
            return nullptr;
        }

        // Add to loaded chunks map
        {
            std::lock_guard<std::mutex> lock(m_chunksMutex);
            m_chunks[coord] = chunk;
        }

        // Update neighbors
        updateChunkNeighbors(chunk);

        // Mark as dirty for saving
        {
            std::lock_guard<std::mutex> lock(m_dirtyChunksMutex);
            m_dirtyChunks.insert(coord);
            chunk->markDirty();
        }

        Utility::Profiler::getInstance()->endSample();
        return chunk;
    }

    std::shared_ptr<Chunk> ChunkManager::getChunk(const ChunkCoord& coord)
    {
        if (!m_initialized)
        {
            return nullptr;
        }

        std::lock_guard<std::mutex> lock(m_chunksMutex);
        auto it = m_chunks.find(coord);
        return it != m_chunks.end() ? it->second : nullptr;
    }

    bool ChunkManager::isChunkLoaded(const ChunkCoord& coord) const
    {
        if (!m_initialized)
        {
            return false;
        }

        std::lock_guard<std::mutex> lock(m_chunksMutex);
        return m_chunks.find(coord) != m_chunks.end();
    }

    bool ChunkManager::serializeChunk(const Chunk& chunk)
    {
        if (!m_initialized || !m_chunkStorage)
        {
            Log::error("ChunkManager or ChunkStorage not initialized");
            return false;
        }

        Utility::Profiler::getInstance()->beginSample("ChunkManager::serializeChunk");

        bool success = m_chunkStorage->saveChunk(chunk);

        if (success)
        {
            // Clear dirty flag if save was successful
            // Note: const_cast is safe here because we're only modifying the dirty flag
            const_cast<Chunk&>(chunk).clearDirty();

            // Remove from dirty set
            {
                std::lock_guard<std::mutex> lock(m_dirtyChunksMutex);
                m_dirtyChunks.erase(chunk.getCoord());
            }
        }
        else
        {
            Log::error("Failed to save chunk at " + chunk.getCoord().toString());
        }

        Utility::Profiler::getInstance()->endSample();
        return success;
    }

    bool ChunkManager::deserializeChunk(Chunk& chunk)
    {
        if (!m_initialized || !m_chunkStorage)
        {
            Log::error("ChunkManager or ChunkStorage not initialized");
            return false;
        }

        Utility::Profiler::getInstance()->beginSample("ChunkManager::deserializeChunk");

        bool success = m_chunkStorage->loadChunk(chunk.getCoord(), chunk);

        if (success)
        {
            // Update neighbors after loading
            auto chunkPtr = getChunk(chunk.getCoord());
            if (chunkPtr)
            {
                updateChunkNeighbors(chunkPtr);
            }
        }
        else
        {
            Log::error("Failed to load chunk at " + chunk.getCoord().toString());
        }

        Utility::Profiler::getInstance()->endSample();
        return success;
    }

    void ChunkManager::saveModifiedChunks()
    {
        if (!m_initialized)
        {
            return;
        }

        Utility::Profiler::getInstance()->beginSample("ChunkManager::saveModiferChunks");

        // Get all dirty chunks
        std::vector<ChunkCoord> dirtyChunkCoords;
        {
            std::lock_guard<std::mutex> lock(m_dirtyChunksMutex);
            dirtyChunkCoords.insert(dirtyChunkCoords.end(), m_dirtyChunks.begin(), m_dirtyChunks.end());
            m_dirtyChunks.clear();
        }

        // Save each dirty chunk
        for (const auto& coord : dirtyChunkCoords)
        {
            auto chunk = getChunk(coord);
            if (chunk && chunk->isDirty())
            {
                serializeChunk(*chunk);
            }
        }

        // Flush storage
        if (m_chunkStorage)
        {
            m_chunkStorage->flush();
        }

        Utility::Profiler::getInstance()->endSample();
    }

    void ChunkManager::backgroundSaveThread()
    {
        Log::debug("ChunkManager save thread started");

        while (m_saveThreadActive)
        {
            // Wait for a chunk to save
            std::shared_ptr<Chunk> chunk;
            m_saveQueue.waitAndPop(chunk);

            // Check for shutdown signal (null chunk) or if thread should stop
            if (!chunk || !m_saveThreadActive)
            {
                break;
            }

            // Save the chunk if it's still dirty
            if (chunk->isDirty())
            {
                serializeChunk(*chunk);
            }
        }

        Log::debug("ChunkManager save thread stopped");
    }

    std::vector<std::shared_ptr<Chunk>> ChunkManager::getChunksAroundPoint(const glm::vec3& position, float radius)
    {
        if (!m_initialized)
        {
            return {};
        }

        std::vector<std::shared_ptr<Chunk>> result;

        // Convert world position to chunk coordinates
        int chunkX = static_cast<int>(floor(position.x / m_chunkSize));
        int chunkY = static_cast<int>(floor(position.y / m_chunkSize));
        int chunkZ = static_cast<int>(floor(position.z / m_chunkSize));

        // Calculate chunk radius
        int chunkRadius = static_cast<int>(ceil(radius / m_chunkSize)) + 1;

        // Iterate through chunks in the radius
        for (int x = chunkX - chunkRadius; x <= chunkX + chunkRadius; x++)
        {
            for (int y = chunkY - chunkRadius; y <= chunkY + chunkRadius; y++)
            {
                for (int z = chunkZ - chunkRadius; z <= chunkZ + chunkRadius; z++)
                {
                    ChunkCoord coord(x, y, z);

                    // Calculate center of this chunk
                    glm::vec3 chunkCenter(
                        (x + 0.5f) * m_chunkSize,
                        (y + 0.5f) * m_chunkSize,
                        (z + 0.5f) * m_chunkSize
                    );

                    // Calculate distance to chunk center
                    float distance = glm::distance(position, chunkCenter);

                    // If chunk is within radius, add it to result
                    if (distance <= radius + m_chunkSize * 0.866f)
                    { // 0.866 = sqrt(3)/2 for diagonal
                        auto chunk = getChunk(coord);
                        if (chunk)
                        {
                            result.push_back(chunk);
                        }
                    }
                }
            }
        }

        return result;
    }

    bool ChunkManager::setWorldPath(const std::string& path)
    {
        if (!m_initialized)
        {
            Log::error("ChunkManager not initialized");
            return false;
        }

        // Save any modified chunks first
        saveModifiedChunks();

        // Clear all loaded chunks
        {
            std::lock_guard<std::mutex> lock(m_chunksMutex);
            m_chunks.clear();
        }

        {
            std::lock_guard<std::mutex> lock(m_dirtyChunksMutex);
            m_dirtyChunks.clear();
        }

        // Create new chunk storage
        auto newStorage = std::make_shared<ChunkStorage>(path);
        if (!newStorage->initialize())
        {
            Log::error("Failed to initialize chunk storage for path: " + path);
            return false;
        }

        // Replace the storage
        m_chunkStorage = newStorage;

        Log::info("ChunkStorage world path set to: " + path);
        return true;
    }

    const std::string& ChunkManager::getWorldPath() const
    {
        static std::string emptyPath;

        if (!m_initialized || !m_chunkStorage)
        {
            return emptyPath;
        }

        return m_chunkStorage->getWorldPath();
    }

    void ChunkManager::setChunkSize(int size)
    {
        if (size <= 0)
        {
            Log::error("Invalid chunk size: " + size);
            return;
        }

        if (m_initialized && !m_chunks.empty())
        {
            Log::error("Cannot change chunk size after chunks have been created");
            return;
        }

        m_chunkSize = size;
        Log::info("Chunk size set to " + size);
    }

    void ChunkManager::updateChunkNeighbors(std::shared_ptr<Chunk> chunk)
    {
        if (!chunk || !m_initialized)
        {
            return;
        }

        const ChunkCoord& coord = chunk->getCoord();

        // Define neighbor offsets and directions
        const std::array<std::pair<ChunkCoord, int>, 6> neighbors = {
            std::make_pair(ChunkCoord(coord.x - 1, coord.y, coord.z), 0), // -X
            std::make_pair(ChunkCoord(coord.x + 1, coord.y, coord.z), 1), // +X
            std::make_pair(ChunkCoord(coord.x, coord.y - 1, coord.z), 2), // -Y
            std::make_pair(ChunkCoord(coord.x, coord.y + 1, coord.z), 3), // +Y
            std::make_pair(ChunkCoord(coord.x, coord.y, coord.z - 1), 4), // -Z
            std::make_pair(ChunkCoord(coord.x, coord.y, coord.z + 1), 5)  // +Z
        };

        // Update neighbors
        for (const auto& [neighborCoord, direction] : neighbors)
        {
            auto neighborChunk = getChunk(neighborCoord);

            // Set neighbor reference in this chunk
            chunk->setNeighbor(direction, neighborChunk);

            // Set reference to this chunk in neighbor
            if (neighborChunk)
            {
                neighborChunk->setNeighbor(direction ^ 1, chunk); // Opposite direction
            }
        }

        // If neighbors changed, mark chunk as needing remesh
        chunk->markMeshDirty();
    }

    bool ChunkManager::generateChunkMesh(const ChunkCoord& coord, bool forceRegenerate)
    {
        auto chunk = getChunk(coord);
        if (!chunk)
        {
            return false;
        }

        // Update neighbors to ensure proper meshing
        updateChunkNeighbors(chunk);

        // Generate mesh
        return chunk->generateMesh(forceRegenerate);
    }

} // namespace PixelCraft::Voxel