// -------------------------------------------------------------------------
// ChunkStorage.h
// -------------------------------------------------------------------------
#pragma once

#include "Core/Subsystem.h"
#include "Voxel/ChunkCoord.h"

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <functional>

namespace PixelCraft::Voxel
{

    // Forward declarations
    class Chunk;

    /**
     * @brief Persistent storage system for voxel chunks
     *
     * ChunkStorage manages saving and loading chunks from disk, with
     * support for region-based file organization, caching, and compression.
     */
    class ChunkStorage
    {
    public:
        /**
         * @brief Storage format options
         */
        enum class Format
        {
            Binary,         ///< Raw binary format
            Compressed,     ///< Compressed binary format
            FlatBuffer,     ///< FlatBuffers serialization
        };

        /**
         * @brief Constructor
         * @param worldPath Path to the world directory
         * @param format Storage format to use
         */
        ChunkStorage(const std::string& worldPath, Format format = Format::Compressed);

        /**
         * @brief Destructor
         */
        ~ChunkStorage();

        /**
         * @brief Initialize the storage system
         * @return True if initialization was successful
         */
        bool initialize();

        /**
         * @brief Save a chunk to disk
         * @param chunk Chunk to save
         * @return True if saving was successful
         */
        bool saveChunk(const Chunk& chunk);

        /**
         * @brief Load a chunk from disk
         * @param coord Chunk coordinate
         * @param chunk Chunk to populate
         * @return True if loading was successful
         */
        bool loadChunk(const ChunkCoord& coord, Chunk& chunk);

        /**
         * @brief Check if a chunk exists on disk
         * @param coord Chunk coordinate
         * @return True if the chunk exists
         */
        bool chunkExists(const ChunkCoord& coord) const;

        /**
         * @brief Delete a chunk from disk
         * @param coord Chunk coordinate
         * @return True if deletion was successful
         */
        bool deleteChunk(const ChunkCoord& coord);

        /**
         * @brief Get the path to the world directory
         * @return World directory path
         */
        const std::string& getWorldPath() const
        {
            return m_worldPath;
        }

        /**
         * @brief Get the storage format being used
         * @return Storage format
         */
        Format getFormat() const
        {
            return m_format;
        }

        /**
         * @brief Set the maximum size of the metadata cache
         * @param size Cache size in number of entries
         */
        void setMetadataCacheSize(size_t size);

        /**
         * @brief Flush all pending writes to disk
         */
        void flush();

        /**
         * @brief Get a list of all chunk coordinates present in storage
         * @return Vector of chunk coordinates
         */
        std::vector<ChunkCoord> getAllChunkCoords() const;

        /**
         * @brief Get chunk metadata without loading the full chunk
         * @param coord Chunk coordinate
         * @param lastModified Output parameter for last modification time
         * @param size Output parameter for chunk file size
         * @return True if metadata was retrieved successfully
         */
        bool getChunkMetadata(const ChunkCoord& coord, int64_t& lastModified, size_t& size) const;

        /**
         * @brief Set compression level for compressed storage format
         * @param level Compression level (0-9, where 0 is no compression and 9 is maximum)
         */
        void setCompressionLevel(int level);

        /**
         * @brief Get the total size of all stored chunks
         * @return Total size in bytes
         */
        size_t getTotalStorageSize() const;

        /**
         * @brief Repair corrupted chunks if possible
         * @param coord Chunk coordinate
         * @return True if repair was successful or not needed
         */
        bool repairChunk(const ChunkCoord& coord);

        /**
         * @brief Iterate through all chunks in a region
         * @param minCoord Minimum corner of the region
         * @param maxCoord Maximum corner of the region
         * @param callback Function to call for each existing chunk
         */
        void forEachChunkInRegion(const ChunkCoord& minCoord, const ChunkCoord& maxCoord,
                                  std::function<void(const ChunkCoord&)> callback) const;

    private:
        /**
         * @brief Get the file path for a chunk
         * @param coord Chunk coordinate
         * @return Path to the chunk file
         */
        std::string getChunkFilePath(const ChunkCoord& coord) const;

        /**
         * @brief Get the region file path for a coordinate
         * @param coord Chunk coordinate
         * @return Path to the region file
         */
        std::string getRegionFilePath(const ChunkCoord& coord) const;

        /**
         * @brief Create directory structure for world if it doesn't exist
         * @return True if directories exist or were created successfully
         */
        bool ensureDirectoriesExist() const;

        /**
         * @brief Check if a chunk file is valid
         * @param filepath Path to the chunk file
         * @return True if the file exists and has valid format
         */
        bool isValidChunkFile(const std::string& filepath) const;

        /**
         * @brief Cache chunk metadata
         * @param coord Chunk coordinate
         * @param lastModified Last modification time
         * @param size File size
         */
        void cacheMetadata(const ChunkCoord& coord, int64_t lastModified, size_t size) const;

        // World path and format
        std::string m_worldPath;
        Format m_format;

        // Compression settings
        int m_compressionLevel;

        // Pending writes tracking
        std::unordered_map<ChunkCoord, bool> m_pendingWrites;
        mutable std::mutex m_pendingWritesMutex;

        // Metadata cache
        struct ChunkMetadata
        {
            int64_t lastModified;
            size_t size;
        };
        mutable std::unordered_map<ChunkCoord, ChunkMetadata> m_metadataCache;
        mutable std::mutex m_metadataCacheMutex;
        size_t m_metadataCacheSize;

        // Region file cache
        struct RegionData
        {
            int64_t lastAccess;
            std::vector<ChunkCoord> chunks;
        };
        mutable std::unordered_map<std::string, RegionData> m_regionCache;
        mutable std::mutex m_regionCacheMutex;

        bool m_initialized;
    };

} // namespace PixelCraft::Voxel