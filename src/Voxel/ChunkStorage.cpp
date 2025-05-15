// -------------------------------------------------------------------------
// ChunkStorage.cpp
// -------------------------------------------------------------------------
#include "Voxel/ChunkStorage.h"
#include "Voxel/Chunk.h"
#include "Core/Logger.h"
#include "Utility/FileSystem.h"
#include "Utility/Profiler.h"
#include "Utility/StringUtils.h"

#include <algorithm>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <zlib.h>

using StringUtils = PixelCraft::Utility::StringUtils;
namespace fs = std::filesystem;
namespace Log = PixelCraft::Core;

namespace PixelCraft::Voxel
{

    namespace
    {
        constexpr size_t DEFAULT_METADATA_CACHE_SIZE = 1000;
        constexpr int DEFAULT_COMPRESSION_LEVEL = 6;
        constexpr char FILE_MAGIC[] = "PCVX"; // PixelCraft Voxel
        constexpr uint32_t FILE_VERSION = 1;
        constexpr int REGION_SIZE = 32; // 32x32 chunks per region

        // Utility function to compute region indices
        std::pair<int, int> getRegionIndices(const ChunkCoord& coord)
        {
            int regionX = coord.x < 0 ? (coord.x - REGION_SIZE + 1) / REGION_SIZE : coord.x / REGION_SIZE;
            int regionZ = coord.z < 0 ? (coord.z - REGION_SIZE + 1) / REGION_SIZE : coord.z / REGION_SIZE;
            return {regionX, regionZ};
        }
    }

    ChunkStorage::ChunkStorage(const std::string& worldPath, Format format)
        : m_worldPath(worldPath)
        , m_format(format)
        , m_compressionLevel(DEFAULT_COMPRESSION_LEVEL)
        , m_metadataCacheSize(DEFAULT_METADATA_CACHE_SIZE)
        , m_initialized(false)
    {
    }

    ChunkStorage::~ChunkStorage()
    {
        if (m_initialized)
        {
            flush();
        }
    }

    bool ChunkStorage::initialize()
    {
        if (m_initialized)
        {
            Log::warn("ChunkStorage already initialized");
            return true;
        }

        // Ensure directories exist
        if (!ensureDirectoriesExist())
        {
            Log::error("Failed to create world directories: " + m_worldPath);
            return false;
        }

        m_initialized = true;
        Log::info("ChunkStorage initialized for world: " + m_worldPath);

        return true;
    }

    bool ChunkStorage::saveChunk(const Chunk& chunk)
    {
        using namespace Utility::Serialization;

        const ChunkCoord& coord = chunk.getCoord();
        std::string chunkPath = getChunkFilePath(coord);

        // Create directory if needed
        std::string dirPath = Utility::FileSystem::getDirectoryPath(chunkPath);
        if (!Utility::FileSystem::directoryExists(dirPath))
        {
            if (!Utility::FileSystem::createDirectory(dirPath))
            {
                Log::error("Failed to create directory: " + dirPath);
                return false;
            }
        }

        // Serialize chunk to file
        auto result = SerializationUtility::serializeToFile(chunk, chunkPath);
        if (!result)
        {
            Log::error("Failed to serialize chunk: " + result.error);
            return false;
        }

        // Update metadata cache
        if (m_metadataCacheSize > 0)
        {
            std::lock_guard<std::mutex> lock(m_metadataCacheMutex);
            ChunkMetadata metadata;
            metadata.lastModified = Utility::FileSystem::getFileModificationTime(chunkPath);
            metadata.size = Utility::FileSystem::getFileSize(chunkPath);
            m_metadataCache[coord] = metadata;
        }

        return true;
    }

    bool ChunkStorage::loadChunk(const ChunkCoord& coord, Chunk& chunk)
    {
        using namespace Utility::Serialization;

        std::string chunkPath = getChunkFilePath(coord);

        // Check if file exists
        if (!Utility::FileSystem::fileExists(chunkPath))
        {
            return false;
        }

        // Deserialize chunk from file
        auto result = SerializationUtility::deserializeFromFile(chunk, chunkPath);
        if (!result)
        {
            Log::error("Failed to deserialize chunk: " + result.error);
            return false;
        }

        // Update metadata cache
        if (m_metadataCacheSize > 0)
        {
            std::lock_guard<std::mutex> lock(m_metadataCacheMutex);
            ChunkMetadata metadata;
            metadata.lastModified = Utility::FileSystem::getFileModificationTime(chunkPath);
            metadata.size = Utility::FileSystem::getFileSize(chunkPath);
            m_metadataCache[coord] = metadata;
        }

        return true;
    }

    bool ChunkStorage::chunkExists(const ChunkCoord& coord) const
    {
        if (!m_initialized)
        {
            return false;
        }

        std::string filepath = getChunkFilePath(coord);
        return Utility::FileSystem::fileExists(filepath);
    }

    bool ChunkStorage::deleteChunk(const ChunkCoord& coord)
    {
        if (!m_initialized)
        {
            Log::error("ChunkStorage not initialized");
            return false;
        }

        std::string filepath = getChunkFilePath(coord);

        if (!Utility::FileSystem::fileExists(filepath))
        {
            return true; // Already doesn't exist
        }

        bool success = Utility::FileSystem::deleteFile(filepath);

        if (success)
        {
            // Remove from caches
            {
                std::lock_guard<std::mutex> lock(m_metadataCacheMutex);
                m_metadataCache.erase(coord);
            }

            {
                std::lock_guard<std::mutex> lock(m_pendingWritesMutex);
                m_pendingWrites.erase(coord);
            }
        }
        else
        {
            Log::error("Failed to delete chunk file: " + filepath);
        }

        return success;
    }

    void ChunkStorage::setMetadataCacheSize(size_t size)
    {
        std::lock_guard<std::mutex> lock(m_metadataCacheMutex);

        m_metadataCacheSize = size;

        // If the new size is smaller than current cache size, trim the cache
        if (m_metadataCache.size() > size)
        {
            // This is a simple approach - in a real implementation,
            // you might want to remove least recently used entries first
            while (m_metadataCache.size() > size)
            {
                m_metadataCache.erase(m_metadataCache.begin());
            }
        }
    }

    void ChunkStorage::flush()
    {
        // Currently no buffered writes to flush
        // This method would be ued if we implemented a write buffer
    }

    std::vector<ChunkCoord> ChunkStorage::getAllChunkCoords() const
    {
        if (!m_initialized)
        {
            return {};
        }

        std::vector<ChunkCoord> coords;

        try
        {
            std::string chunksDir = m_worldPath + "/chunks";
            if (!fs::exists(chunksDir))
            {
                return coords;
            }

            // Recursively iterate through all files in the chunks directory
            for (const auto& entry : fs::recursive_directory_iterator(chunksDir))
            {
                if (fs::is_regular_file(entry) && entry.path().extension() == ".chunk")
                {
                    // Parse filename to extract coordinates
                    std::string filename = entry.path().filename().string();

                    // Format is expected to be "c.x.y.z.chunk"
                    size_t pos1 = filename.find('.');
                    size_t pos2 = filename.find('.', pos1 + 1);
                    size_t pos3 = filename.find('.', pos2 + 1);
                    size_t pos4 = filename.find('.', pos3 + 1);

                    if (pos1 != std::string::npos && pos2 != std::string::npos &&
                        pos3 != std::string::npos && pos4 != std::string::npos)
                    {
                        try
                        {
                            int x = std::stoi(filename.substr(pos1 + 1, pos2 - pos1 - 1));
                            int y = std::stoi(filename.substr(pos2 + 1, pos3 - pos2 - 1));
                            int z = std::stoi(filename.substr(pos3 + 1, pos4 - pos3 - 1));

                            coords.emplace_back(x, y, z);
                        }
                        catch (const std::exception& e)
                        {
                            Log::warn("Invalid chunk filename format: " + filename);
                        }
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            Log::error("Exception while enumerating chunks: " + std::string(e.what()));
        }

        return coords;
    }

    bool ChunkStorage::getChunkMetadata(const ChunkCoord& coord, int64_t& lastModified, size_t& size) const
    {
        if (!m_initialized)
        {
            return false;
        }

        // Check cache first
        {
            std::lock_guard<std::mutex> lock(m_metadataCacheMutex);
            auto it = m_metadataCache.find(coord);
            if (it != m_metadataCache.end())
            {
                lastModified = it->second.lastModified;
                size = it->second.size;
                return true;
            }
        }

        // Cache miss, get from filesystem
        std::string filepath = getChunkFilePath(coord);

        struct stat fileStat;
        if (stat(filepath.c_str(), &fileStat) != 0)
        {
            return false;
        }

        lastModified = fileStat.st_mtime;
        size = fileStat.st_size;

        // Cache the result
        cacheMetadata(coord, lastModified, size);

        return true;
    }

    void ChunkStorage::setCompressionLevel(int level)
    {
        // Ensure level is in valid range
        m_compressionLevel = glm::max(0, glm::min(9, level));
    }

    size_t ChunkStorage::getTotalStorageSize() const
    {
        if (!m_initialized)
        {
            return 0;
        }

        size_t totalSize = 0;

        try
        {
            std::string chunksDir = m_worldPath + "/chunks";
            if (!fs::exists(chunksDir))
            {
                return 0;
            }

            // Recursively iterate through all files in the chunks directory
            for (const auto& entry : fs::recursive_directory_iterator(chunksDir))
            {
                if (fs::is_regular_file(entry) && entry.path().extension() == ".chunk")
                {
                    totalSize += fs::file_size(entry.path());
                }
            }
        }
        catch (const std::exception& e)
        {
            Log::error("Exception while calculating storage size: " + std::string(e.what()));
        }

        return totalSize;
    }

    bool ChunkStorage::repairChunk(const ChunkCoord& coord)
    {
        // Basic repair functionality - check if file is valid
        if (!m_initialized)
        {
            return false;
        }

        std::string filepath = getChunkFilePath(coord);

        if (!Utility::FileSystem::fileExists(filepath))
        {
            return false; // File doesn't exist, can't repair
        }

        // Try to validate the file
        if (isValidChunkFile(filepath))
        {
            return true; // File is already valid
        }
        
        // If we have a backup, try to restore from it
        std::string backupPath = filepath + ".bak";
        if (Utility::FileSystem::fileExists(backupPath))
        {
            bool success = Utility::FileSystem::copyFile(backupPath, filepath);

            if (success && isValidChunkFile(filepath))
            {
                Log::info("Repaired chunk file from backup: " + filepath);
                return true;
            }
        }

        // If repair failed, delete the corrupted file
        Utility::FileSystem::deleteFile(filepath);
        Log::warn("Deleted corrupted chunk file that couldn't be repaired: " + filepath);

        return false;
    }

    void ChunkStorage::forEachChunkInRegion(const ChunkCoord& minCoord, const ChunkCoord& maxCoord,
                                            std::function<void(const ChunkCoord&)> callback) const
    {
        if (!m_initialized || !callback)
        {
            return;
        }

        // Iterate through all potential chunk coordinates in the region
        for (int x = minCoord.x; x <= maxCoord.x; x++)
        {
            for (int y = minCoord.y; y <= maxCoord.y; y++)
            {
                for (int z = minCoord.z; z <= maxCoord.z; z++)
                {
                    ChunkCoord coord(x, y, z);

                    // Check if chunk exists before calling the callback
                    if (chunkExists(coord))
                    {
                        callback(coord);
                    }
                }
            }
        }
    }

    std::string ChunkStorage::getChunkFilePath(const ChunkCoord& coord) const
    {
        // Create a directory structure based on chunk coordinates
        // to avoid having too many files in a single directory

        auto [regionX, regionZ] = getRegionIndices(coord);

        std::string directory = m_worldPath + "/chunks/" +
            std::to_string(regionX) + "." +
            std::to_string(regionZ);

        // Create filename from coordinates
        std::string filename = "c." +
            std::to_string(coord.x) + "." +
            std::to_string(coord.y) + "." +
            std::to_string(coord.z) + ".chunk";

        return directory + "/" + filename;
    }

    std::string ChunkStorage::getRegionFilePath(const ChunkCoord& coord) const
    {
        auto [regionX, regionZ] = getRegionIndices(coord);

        return m_worldPath + "/regions/r." +
            std::to_string(regionX) + "." +
            std::to_string(regionZ) + ".region";
    }

    bool ChunkStorage::ensureDirectoriesExist() const
    {
        try
        {
            // Create world directory if it doesn't exist
            if (!fs::exists(m_worldPath))
            {
                fs::create_directories(m_worldPath);
            }

            // Create chunks directory
            std::string chunksDir = m_worldPath + "/chunks";
            if (!fs::exists(chunksDir))
            {
                fs::create_directories(chunksDir);
            }

            // Create regions directory (for future region-based storage)
            std::string regionsDir = m_worldPath + "/regions";
            if (!fs::exists(regionsDir))
            {
                fs::create_directories(regionsDir);
            }

            return true;
        }
        catch (const std::exception& e)
        {
            Log::error("Failed to create directories: " + std::string(e.what()));
            return false;
        }
    }

    bool ChunkStorage::isValidChunkFile(const std::string& filepath) const
    {
        try
        {
            // Open file for reading
            std::ifstream file(filepath, std::ios::binary);
            if (!file.is_open())
            {
                return false;
            }

            // Check file size
            file.seekg(0, std::ios::end);
            std::streampos fileSize = file.tellg();
            file.seekg(0, std::ios::beg);

            if (fileSize < 20)
            { // Minimum size for a valid header
                return false;
            }

            // Read and verify file header
            char magic[4];
            file.read(magic, 4);
            if (memcmp(magic, FILE_MAGIC, 4) != 0)
            {
                return false;
            }

            // Read version
            uint32_t version;
            file.read(reinterpret_cast<char*>(&version), sizeof(version));

            if (version > FILE_VERSION)
            {
                return false;
            }

            // If we get here, the file seems valid
            return true;
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    void ChunkStorage::cacheMetadata(const ChunkCoord& coord, int64_t lastModified, size_t size) const
    {
        std::lock_guard<std::mutex> lock(m_metadataCacheMutex);

        // If cache is full, remove oldest entry
        if (m_metadataCache.size() >= m_metadataCacheSize && m_metadataCache.find(coord) == m_metadataCache.end())
        {
            m_metadataCache.erase(m_metadataCache.begin());
        }

        // Add to cache
        ChunkMetadata metadata;
        metadata.lastModified = lastModified;
        metadata.size = size;

        m_metadataCache[coord] = metadata;
    }

} // namespace PixelCraft::Voxel