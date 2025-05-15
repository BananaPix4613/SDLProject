// -------------------------------------------------------------------------
// ChunkManager.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <set>
#include <mutex>
#include <thread>
#include <atomic>

#include "Core/Subsystem.h"
#include "Core/ThreadSafeQueue.h"
#include "Voxel/Chunk.h"
#include "glm/glm.hpp"

namespace PixelCraft::Voxel
{

    // Forward declarations
    class ChunkStorage;
    class StreamingManager;

    /**
     * @brief Manages voxel chunks with efficient serialization
     *
     * ChunkManager handles the lifecycle of chunks in the voxel world, including
     * loading, unloading, storage, and coordination with the streaming system.
     */
    class ChunkManager : public Core::Subsystem
    {
    public:
        /**
         * @brief Constructor
         */
        ChunkManager();

        /**
         * @brief Destructor
         */
        virtual ~ChunkManager();

        /**
         * @brief Initialize the chunk management system
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Update the chunk system
         * @param deltaTime Time since last update
         */
        void update(float deltaTime) override;

        /**
         * @brief Render debug visualization (if applicable)
         */
        void render() override;

        /**
         * @brief Shut down the chunk system and release resources
         */
        void shutdown() override;

        /**
         * @brief Get the name of the subsystem
         * @return The subsystem name
         */
        std::string getName() const override
        {
            return "ChunkManager";
        }

        /**
         * @brief Get the subsystem dependencies
         * @return Vector of dependency subsystem names
         */
        std::vector<std::string> getDependencies() const override;

        /**
         * @brief Load a chunk from storage or create it if not found
         * @param coord Chunk coordinate
         * @return Shared pointer to the loaded chunk
         */
        std::shared_ptr<Chunk> loadChunk(const ChunkCoord& coord);

        /**
         * @brief Unload a chunk from memory
         * @param coord Chunk coordinate
         * @return True if the chunk was unloaded
         */
        bool unloadChunk(const ChunkCoord& coord);

        /**
         * @brief Create a new chunk at the specified coordinate
         * @param coord Chunk coordinate
         * @return Shared pointer to the created chunk
         */
        std::shared_ptr<Chunk> createChunk(const ChunkCoord& coord);

        /**
         * @brief Get a loaded chunk
         * @param coord Chunk coordinate
         * @return Shared pointer to the chunk, or nullptr if not loaded
         */
        std::shared_ptr<Chunk> getChunk(const ChunkCoord& coord);

        /**
         * @brief Check if a chunk is loaded
         * @param coord Chunk coordinate
         * @return True if the chunk is loaded
         */
        bool isChunkLoaded(const ChunkCoord& coord) const;

        /**
         * @brief Serialize a chunk to storage
         * @param chunk Chunk to serialize
         * @return True if serialization was successful
         */
        bool serializeChunk(const Chunk& chunk);

        /**
         * @brief Deserialize a chunk from storage
         * @param chunk Chunk to deserialize
         * @return True if deserialization was successful
         */
        bool deserializeChunk(Chunk& chunk);

        /**
         * @brief Save all modified chunks
         */
        void saveModifiedChunks();

        /**
         * @brief Background thread for saving chunks
         */
        void backgroundSaveThread();

        /**
         * @brief Get chunks around a point within a radius
         * @param position World position
         * @param radius Distance radius
         * @return Vector of chunks within the radius
         */
        std::vector<std::shared_ptr<Chunk>> getChunksAroundPoint(const glm::vec3& position, float radius);

        /**
         * @brief Get map of all loaded chunks
         * @return Map of chunk coordinates to chunk pointers
         */
        const std::unordered_map<ChunkCoord, std::shared_ptr<Chunk>>& getLoadedChunks() const
        {
            return m_chunks;
        }

        /**
         * @brief Get the chunk storage
         * @return Pointer to chunk storage
         */
        std::shared_ptr<ChunkStorage> getChunkStorage() const
        {
            return m_chunkStorage;
        }

        /**
         * @brief Set the world path for chunk storage
         * @param path Path to world directory
         * @return True if world was set successfully
         */
        bool setWorldPath(const std::string& path);

        /**
         * @brief Get the world path
         * @return Path to world directory
         */
        const std::string& getWorldPath() const;

        /**
         * @brief Set chunk size
         * @param size Size of each chunk (typically 16 or 32)
         */
        void setChunkSize(int size);

        /**
         * @brief Get chunk size
         * @return Size of each chunk
         */
        int getChunkSize() const
        {
            return m_chunkSize;
        }

        /**
         * @brief Update neighbor pointers for a chunk
         * @param chunk Chunk to update neighbors for
         */
        void updateChunkNeighbors(std::shared_ptr<Chunk> chunk);

        /**
         * @brief Generate mesh for a chunk
         * @param coord Chunk coordinate
         * @param forceRegenerate Force mesh regeneration even if not dirty
         * @return True if mesh generation was successful
         */
        bool generateChunkMesh(const ChunkCoord& coord, bool forceRegenerate = false);

    private:
        // Chunk storage
        std::unordered_map<ChunkCoord, std::shared_ptr<Chunk>> m_chunks;
        std::set<ChunkCoord> m_dirtyChunks;
        std::shared_ptr<ChunkStorage> m_chunkStorage;

        // Chunk properties
        int m_chunkSize;

        // Saving thread
        Core::ThreadSafeQueue<std::shared_ptr<Chunk>> m_saveQueue;
        std::thread m_saveThread;
        std::atomic<bool> m_saveThreadActive;

        // Thread safety
        mutable std::mutex m_chunksMutex;
        mutable std::mutex m_dirtyChunksMutex;

        // Streaming integration
        std::weak_ptr<StreamingManager> m_streamingManager;

        // Status
        bool m_initialized;
    };

} // namespace PixelCraft::Voxel