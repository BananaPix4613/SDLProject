// -------------------------------------------------------------------------
// FileChunkStorage.cpp
// -------------------------------------------------------------------------
#include "Voxel/FileChunkStorage.h"
#include "Core/Logger.h"
#include "ECS/Serializer.h"
#include "ECS/Deserializer.h"
#include "Utility/FileSystem.h"
#include "Utility/StringUtils.h"

#include <fstream>
#include <sstream>

namespace Log = PixelCraft::Core;
using fs = PixelCraft::Utility::FileSystem;
using StringUtils = PixelCraft::Utility::StringUtils;

namespace PixelCraft::Voxel
{

    FileChunkStorage::FileChunkStorage(const std::string& basePath)
        : m_basePath(fs::normalizePath(basePath))
    {
        // Ensure the base directory exists
        ensureDirectoryExists(m_basePath);
    }

    FileChunkStorage::~FileChunkStorage()
    {
        // Nothing to clean up
    }

    bool FileChunkStorage::saveChunk(const Chunk& chunk)
    {
        // Get the file path for this chunk
        std::string path = getChunkPath(chunk.getCoord());

        // Ensure the directory exists
        if (!ensureDirectoryExists(fs::getDirectoryPath(path)))
        {
            Log::error("Failed to create directory for chunk: " + path);
            return false;
        }

        // Open the file for writing
        std::ofstream file(path, std::ios::binary);
        if (!file)
        {
            Log::error("Failed to open chunk file for writing: " + path);
            return false;
        }

        try
        {
            // Create a serializer for the file
            Serializer serializer(file);

            // Serialize the chunk data
            const_cast<Chunk&>(chunk).serialize(serializer);

            return true;
        }
        catch (const std::exception& e)
        {
            Log::error(StringUtils::format("Error serializing chunk: ", e.what()));
            return false;
        }
    }

    bool FileChunkStorage::loadChunk(Chunk& chunk)
    {
        // Get the file path for this chunk
        std::string path = getChunkPath(chunk.getCoord());

        // Check if the file exists
        if (!fs::fileExists(path))
        {
            return false;
        }

        // Open the file for reading
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            Log::error("Failed to open chunk file for reading: " + path);
            return false;
        }

        try
        {
            // Create a deserializer for the file
            Deserializer deserializer(file);

            // Deserialize the chunk data
            chunk.deserialize(deserializer);

            return true;
        }
        catch (const std::exception& e)
        {
            Log::error(StringUtils::format("Error deserializing chunk: ", e.what()));
            return false;
        }
    }

    bool FileChunkStorage::chunkExists(const ChunkCoord& coord)
    {
        std::string path = getChunkPath(coord);
        return fs::fileExists(path);
    }

    bool FileChunkStorage::deleteChunk(const ChunkCoord& coord)
    {
        std::string path = getChunkPath(coord);

        if (!fs::fileExists(path))
        {
            // File doesn't exist, consider it "deleted"
            return true;
        }

        if (!fs::deleteFile(path))
        {
            Log::error("Failed to delete chunk file: " + path);
            return false;
        }

        return true;
    }

    std::string FileChunkStorage::getChunkPath(const ChunkCoord& coord) const
    {
        // Create a region-based file structure for better file system performance
        // A region contains 32x32 chunks
        const int REGION_SIZE = 32;

        int regionX = std::floor(static_cast<float>(coord.x()) / REGION_SIZE);
        int regionZ = std::floor(static_cast<float>(coord.z()) / REGION_SIZE);

        // Create a path in the format: basePath/regionX.regionZ/x.y.z.chunk
        std::stringstream ss;
        ss << m_basePath << "/"
            << "r." << regionX << "." << regionZ << "/"
            << "c." << coord.x() << "." << coord.y() << "." << coord.z() << ".chunk";

        return fs::normalizePath(ss.str());
    }

    bool FileChunkStorage::ensureDirectoryExists(const std::string& path) const
    {
        if (fs::directoryExists(path))
        {
            return true;
        }

        return fs::createDirectory(path);
    }

} // namespace PixelCraft::Voxel