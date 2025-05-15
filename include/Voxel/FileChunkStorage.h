// -------------------------------------------------------------------------
// FileChunkStorage.h
// -------------------------------------------------------------------------
#pragma once

#include <string>
#include <filesystem>

namespace PixelCraft::Voxel
{

    // Forward declarations
    class Chunk;
    class ChunkStorage;

    /**
     * @brief File-based implementation of ChunkStorage for persisting chunks
     *
     * FileChunkStorage saves chunks as individual files in a specified directory
     * with subdirectories organized by region for improved file system performance.
     */
    class FileChunkStorage : public ChunkStorage
    {
    public:
        /**
         * @brief Constructor
         * @param basePath Base directory path where chunk files will be stored
         */
        FileChunkStorage(const std::string& basePath);

        /**
         * @brief Destructor
         */
        ~FileChunkStorage() override;

        /**
         * @brief Save a chunk to a file
         * @param chunk The chunk to save
         * @return True if the save was successful
         */
        bool saveChunk(const Chunk& chunk) override;

        /**
         * @brief Load a chunk from a file
         * @param chunk The chunk to load data into (must have valid coordinates)
         * @return True if the load was successful
         */
        bool loadChunk(Chunk& chunk) override;

        /**
         * @brief Check if a chunk file exists
         * @param coord Coordinates of the chunk to check
         * @return True if the chunk file exists
         */
        bool chunkExists(const ChunkCoord& coord) override;

        /**
         * @brief Delete a chunk file
         * @param coord Coordinates of the chunk to delete
         * @return True if the deletion was successful or file didn't exist
         */
        bool deleteChunk(const ChunkCoord& coord) override;

    private:
        // Base path for chunk storage
        std::string m_basePath;

        /**
         * @brief Get the file path for a chunk
         * @param coord Coordinates of the chunk
         * @return Full path to the chunk file
         */
        std::string getChunkPath(const ChunkCoord& coord) const;

        /**
         * @brief Ensure the directory structure exists for a chunk path
         * @param path Path to check/create
         * @return True if the directory exists or was created successfully
         */
        bool ensureDirectoryExists(const std::string& path) const;
    };

} // namespace PixelCraft::Voxel