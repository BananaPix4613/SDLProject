// -------------------------------------------------------------------------
// FileSystem.cpp
// -------------------------------------------------------------------------
#include "Utility/FileSystem.h"
#include "Core/Logger.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <regex>
#include <chrono>

#ifdef _WIN32
#include <Windows.h>
#include <direct.h>
#define PATH_SEPARATOR "\\"
#else
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#define PATH_SEPARATOR "/"
#endif

namespace Log = PixelCraft::Core;
namespace fs = std::filesystem;

namespace PixelCraft::Utility
{

    // Static member initialization
    std::unordered_map<std::string, std::string> FileSystem::s_mountPoints;
    std::unordered_map<uint64_t, std::pair<std::string, std::function<void(const std::string&)>>> FileSystem::s_fileWatchers;
    uint64_t FileSystem::s_nextWatcherId = 1;
    std::mutex FileSystem::s_fileSystemMutex;
    std::thread FileSystem::s_watcherThread;
    std::atomic<bool> FileSystem::s_watcherRunning(false);
    bool FileSystem::s_initialized = false;

    bool FileSystem::initialize()
    {
        if (s_initialized)
        {
            Log::warn("FileSystem already initialized");
            return true;
        }

        Log::info("Initializing FileSystem utility");

        // Initialize mount points with default paths
        s_mountPoints["assets"] = "assets";
        s_mountPoints["cache"] = "cache";
        s_mountPoints["user"] = "userdata";

        // Start file watcher thread
        s_watcherRunning = true;
        s_watcherThread = std::thread(fileWatcherThreadFunc);

        s_initialized = true;
        return true;
    }

    void FileSystem::shutdown()
    {
        if (!s_initialized)
        {
            return;
        }

        Log::info("Shutting down FileSystem utility");

        // Stop file watcher thread
        s_watcherRunning = false;
        if (s_watcherThread.joinable())
        {
            s_watcherThread.join();
        }

        // Clear all mount points and watchers
        std::lock_guard<std::mutex> lock(s_fileSystemMutex);
        s_mountPoints.clear();
        s_fileWatchers.clear();

        s_initialized = false;
    }

    // File Reading Functions
    FileResult FileSystem::readFile(const std::string& path, std::string& outContent)
    {
        try
        {
            std::lock_guard<std::mutex> lock(s_fileSystemMutex);

            // Resolve virtual path if needed
            std::string resolvedPath = resolvePathInternal(path);

            // Check if file exists and is accessible
            FileResult accessResult = checkFileAccess(resolvedPath, FileMode::Read);
            if (accessResult != FileResult::Success)
            {
                return accessResult;
            }

            // Open file
            std::ifstream file(resolvedPath);
            if (!file.is_open())
            {
                Log::error("Failed to open file for reading: " + resolvedPath);
                return FileResult::AccessDenied;
            }

            // Read content
            std::stringstream buffer;
            buffer << file.rdbuf();
            outContent = buffer.str();

            return FileResult::Success;
        }
        catch (const std::exception& e)
        {
            Log::error("Exception in readFile: " + std::string(e.what()));
            return FileResult::Unknown;
        }
    }

    FileResult FileSystem::readFileBinary(const std::string& path, std::vector<uint8_t>& outData)
    {
        try
        {
            std::lock_guard<std::mutex> lock(s_fileSystemMutex);

            // Resolve virtual path if needed
            std::string resolvedPath = resolvePathInternal(path);

            // Check if file exists and is accessible
            FileResult accessResult = checkFileAccess(resolvedPath, FileMode::ReadBinary);
            if (accessResult != FileResult::Success)
            {
                return accessResult;
            }

            // Get file size
            size_t fileSize = getFileSize(resolvedPath);
            if (fileSize == 0)
            {
                // Empty file, return success with empty vector
                outData.clear();
                return FileResult::Success;
            }

            // Resize output vector
            outData.resize(fileSize);

            // Read data
            std::ifstream file(resolvedPath, std::ios::binary);
            if (!file.is_open())
            {
                Log::error("Failed to open file for binary reading: " + resolvedPath);
                return FileResult::AccessDenied;
            }

            file.read(reinterpret_cast<char*>(outData.data()), fileSize);

            if (file.fail() && !file.eof())
            {
                Log::error("Failed to read binary file: " + resolvedPath);
                return FileResult::Unknown;
            }

            return FileResult::Success;
        }
        catch (const std::exception& e)
        {
            Log::error("Exception in readFileBinary: " + std::string(e.what()));
            return FileResult::Unknown;
        }
    }

    // File Writing Functions
    FileResult FileSystem::writeFile(const std::string& path, const std::string& content)
    {
        try
        {
            std::lock_guard<std::mutex> lock(s_fileSystemMutex);

            // Resolve virtual path if needed
            std::string resolvedPath = resolvePathInternal(path);

            // Ensure directory exists
            std::string directory = getDirectoryPath(resolvedPath);
            if (!ensureDirectoryExists(directory))
            {
                Log::error("Failed to create directory for file: " + resolvedPath);
                return FileResult::InvalidPath;
            }

            // Open file
            std::ofstream file(resolvedPath);
            if (!file.is_open())
            {
                Log::error("Failed to open file for writing: " + resolvedPath);
                return FileResult::AccessDenied;
            }

            // Write content
            file << content;

            if (file.fail())
            {
                Log::error("Failed to write to file: " + resolvedPath);
                return FileResult::Unknown;
            }

            return FileResult::Success;
        }
        catch (const std::exception& e)
        {
            Log::error("Exception in writeFile: " + std::string(e.what()));
            return FileResult::Unknown;
        }
    }

    FileResult FileSystem::writeFileBinary(const std::string& path, const std::vector<uint8_t>& data)
    {
        try
        {
            std::lock_guard<std::mutex> lock(s_fileSystemMutex);

            // Resolve virtual path if needed
            std::string resolvedPath = resolvePathInternal(path);

            // Ensure directory exists
            std::string directory = getDirectoryPath(resolvedPath);
            if (!ensureDirectoryExists(directory))
            {
                Log::error("Failed to create directory for file: " + resolvedPath);
                return FileResult::InvalidPath;
            }

            // Open file
            std::ofstream file(resolvedPath, std::ios::binary);
            if (!file.is_open())
            {
                Log::error("Failed to open file for binary writing: " + resolvedPath);
                return FileResult::AccessDenied;
            }

            // Write data
            if (!data.empty())
            {
                file.write(reinterpret_cast<const char*>(data.data()), data.size());
            }

            if (file.fail())
            {
                Log::error("Failed to write binary data to file: " + resolvedPath);
                return FileResult::Unknown;
            }

            return FileResult::Success;
        }
        catch (const std::exception& e)
        {
            Log::error("Exception in writeFileBinary: " + std::string(e.what()));
            return FileResult::Unknown;
        }
    }

    FileResult FileSystem::appendFile(const std::string& path, const std::string& content)
    {
        try
        {
            std::lock_guard<std::mutex> lock(s_fileSystemMutex);

            // Resolve virtual path if needed
            std::string resolvedPath = resolvePathInternal(path);

            // Ensure directory exists
            std::string directory = getDirectoryPath(resolvedPath);
            if (!ensureDirectoryExists(directory))
            {
                Log::error("Failed to create directory for file: " + resolvedPath);
                return FileResult::InvalidPath;
            }

            // Open file in append mode
            std::ofstream file(resolvedPath, std::ios::app);
            if (!file.is_open())
            {
                Log::error("Failed to open file for appending: " + resolvedPath);
                return FileResult::AccessDenied;
            }

            // Append content
            file << content;

            if (file.fail())
            {
                Log::error("Failed to append to file: " + resolvedPath);
                return FileResult::Unknown;
            }

            return FileResult::Success;
        }
        catch (const std::exception& e)
        {
            Log::error("Exception in appendFile: " + std::string(e.what()));
            return FileResult::Unknown;
        }
    }

    FileResult FileSystem::appendFileBinary(const std::string& path, const std::vector<uint8_t>& data)
    {
        try
        {
            std::lock_guard<std::mutex> lock(s_fileSystemMutex);

            // Resolve virtual path if needed
            std::string resolvedPath = resolvePathInternal(path);

            // Ensure directory exists
            std::string directory = getDirectoryPath(resolvedPath);
            if (!ensureDirectoryExists(directory))
            {
                Log::error("Failed to create directory for file: " + resolvedPath);
                return FileResult::InvalidPath;
            }

            // Open file in binary append mode
            std::ofstream file(resolvedPath, std::ios::binary | std::ios::app);
            if (!file.is_open())
            {
                Log::error("Failed to open file for binary appending: " + resolvedPath);
                return FileResult::AccessDenied;
            }

            // Append data
            if (!data.empty())
            {
                file.write(reinterpret_cast<const char*>(data.data()), data.size());
            }

            if (file.fail())
            {
                Log::error("Failed to append binary data to file: " + resolvedPath);
                return FileResult::Unknown;
            }

            return FileResult::Success;
        }
        catch (const std::exception& e)
        {
            Log::error("Exception in appendFileBinary: " + std::string(e.what()));
            return FileResult::Unknown;
        }
    }

    // File Operation Functions
    bool FileSystem::fileExists(const std::string& path)
    {
        try
        {
            std::lock_guard<std::mutex> lock(s_fileSystemMutex);

            // Resolve virtual path if needed
            std::string resolvedPath = resolvePathInternal(path);

            return fs::exists(resolvedPath) && fs::is_regular_file(resolvedPath);
        }
        catch (const std::exception& e)
        {
            Log::error("Exception in fileExists: " + std::string(e.what()));
            return false;
        }
    }

    bool FileSystem::deleteFile(const std::string& path)
    {
        try
        {
            std::lock_guard<std::mutex> lock(s_fileSystemMutex);

            // Resolve virtual path if needed
            std::string resolvedPath = resolvePathInternal(path);

            if (!fileExists(resolvedPath))
            {
                Log::warn("Cannot delete file, it doesn't exist: " + resolvedPath);
                return false;
            }

            return fs::remove(resolvedPath);
        }
        catch (const std::exception& e)
        {
            Log::error("Exception in deleteFile: " + std::string(e.what()));
            return false;
        }
    }

    bool FileSystem::copyFile(const std::string& source, const std::string& destination, bool overwrite)
    {
        try
        {
            std::lock_guard<std::mutex> lock(s_fileSystemMutex);

            // Resolve virtual paths if needed
            std::string resolvedSource = resolvePathInternal(source);
            std::string resolvedDestination = resolvePathInternal(destination);

            if (!fileExists(resolvedSource))
            {
                Log::error("Source file does not exist: " + resolvedSource);
                return false;
            }

            // Check if destination exists and we're not overwriting
            if (fileExists(resolvedDestination) && !overwrite)
            {
                Log::error("Destination file already exists and overwrite is false: " + resolvedDestination);
                return false;
            }

            // Ensure destination directory exists
            std::string destDir = getDirectoryPath(resolvedDestination);
            if (!ensureDirectoryExists(destDir))
            {
                Log::error("Failed to create destination directory: " + destDir);
                return false;
            }

            // Copy file
            fs::copy_options options = overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::none;
            fs::copy_file(resolvedSource, resolvedDestination, options);

            return true;
        }
        catch (const std::exception& e)
        {
            Log::error("Exception in copyFile: " + std::string(e.what()));
            return false;
        }
    }

    bool FileSystem::moveFile(const std::string& source, const std::string& destination, bool overwrite)
    {
        try
        {
            std::lock_guard<std::mutex> lock(s_fileSystemMutex);

            // Resolve virtual paths if needed
            std::string resolvedSource = resolvePathInternal(source);
            std::string resolvedDestination = resolvePathInternal(destination);

            if (!fileExists(resolvedSource))
            {
                Log::error("Source file does not exist: " + resolvedSource);
                return false;
            }

            // Check if destination exists and we're not overwriting
            if (fileExists(resolvedDestination) && !overwrite)
            {
                Log::error("Destination file already exists and overwrite is false: " + resolvedDestination);
                return false;
            }

            // Ensure destination directory exists
            std::string destDir = getDirectoryPath(resolvedDestination);
            if (!ensureDirectoryExists(destDir))
            {
                Log::error("Failed to create destination directory: " + destDir);
                return false;
            }

            // If destination exists and overwrite is true, delete destination first
            if (fileExists(resolvedDestination) && overwrite)
            {
                if (!deleteFile(resolvedDestination))
                {
                    Log::error("Failed to delete existing destination file: " + resolvedDestination);
                    return false;
                }
            }

            // Move file
            fs::rename(resolvedSource, resolvedDestination);

            return true;
        }
        catch (const std::exception& e)
        {
            Log::error("Exception in moveFile: " + std::string(e.what()));
            return false;
        }
    }

    size_t FileSystem::getFileSize(const std::string& path)
    {
        try
        {
            std::lock_guard<std::mutex> lock(s_fileSystemMutex);

            // Resolve virtual path if needed
            std::string resolvedPath = resolvePathInternal(path);

            if (!fileExists(resolvedPath))
            {
                return 0;
            }

            return static_cast<size_t>(fs::file_size(resolvedPath));
        }
        catch (const std::exception& e)
        {
            Log::error("Exception in getFileSize: " + std::string(e.what()));
            return 0;
        }
    }

    time_t FileSystem::getFileModificationTime(const std::string& path)
    {
        try
        {
            std::lock_guard<std::mutex> lock(s_fileSystemMutex);

            // Resolve virtual path if needed
            std::string resolvedPath = resolvePathInternal(path);

            if (!fileExists(resolvedPath))
            {
                return 0;
            }

            auto lastWriteTime = fs::last_write_time(resolvedPath);
            auto systemTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                lastWriteTime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());

            return std::chrono::system_clock::to_time_t(systemTime);
        }
        catch (const std::exception& e)
        {
            Log::error("Exception in getFileModificationTime: " + std::string(e.what()));
            return 0;
        }
    }

    // Directory Operation Functions
    bool FileSystem::createDirectory(const std::string& path)
    {
        try
        {
            std::lock_guard<std::mutex> lock(s_fileSystemMutex);

            // Resolve virtual path if needed
            std::string resolvedPath = resolvePathInternal(path);

            // If directory already exists, return true
            if (directoryExists(resolvedPath))
            {
                return true;
            }

            return fs::create_directories(resolvedPath);
        }
        catch (const std::exception& e)
        {
            Log::error("Exception in createDirectory: " + std::string(e.what()));
            return false;
        }
    }

    bool FileSystem::deleteDirectory(const std::string& path, bool recursive)
    {
        try
        {
            std::lock_guard<std::mutex> lock(s_fileSystemMutex);

            // Resolve virtual path if needed
            std::string resolvedPath = resolvePathInternal(path);

            if (!directoryExists(resolvedPath))
            {
                Log::warn("Cannot delete directory, it doesn't exist: " + resolvedPath);
                return false;
            }

            if (recursive)
            {
                // Use std::filesystem to remove recursively
                return fs::remove_all(resolvedPath) > 0;
            }
            else
            {
                // Check if directory is empty
                if (!fs::is_empty(resolvedPath))
                {
                    Log::error("Cannot delete non-empty directory without recursive flag: " + resolvedPath);
                    return false;
                }

                return fs::remove(resolvedPath);
            }
        }
        catch (const std::exception& e)
        {
            Log::error("Exception in deleteDirectory: " + std::string(e.what()));
            return false;
        }
    }

    bool FileSystem::directoryExists(const std::string& path)
    {
        try
        {
            std::lock_guard<std::mutex> lock(s_fileSystemMutex);

            // Resolve virtual path if needed
            std::string resolvedPath = resolvePathInternal(path);

            return fs::exists(resolvedPath) && fs::is_directory(resolvedPath);
        }
        catch (const std::exception& e)
        {
            Log::error("Exception in directoryExists: " + std::string(e.what()));
            return false;
        }
    }

    std::vector<FileInfo> FileSystem::listFiles(const std::string& directory, const std::string& filter)
    {
        std::vector<FileInfo> result;

        try
        {
            std::lock_guard<std::mutex> lock(s_fileSystemMutex);

            // Resolve virtual path if needed
            std::string resolvedPath = resolvePathInternal(directory);

            if (!directoryExists(resolvedPath))
            {
                Log::error("Directory does not exist: " + resolvedPath);
                return result;
            }

            // Prepare filter as regex
            std::regex filterRegex;

            if (filter != "*")
            {
                // Convert wildcard to regex
                std::string regexStr = filter;
                // Replace . with \. (escape dot)
                size_t pos = 0;
                while ((pos = regexStr.find(".", pos)) != std::string::npos)
                {
                    regexStr.replace(pos, 1, "\\.");
                    pos += 2;
                }

                // Replace * with .*
                pos = 0;
                while ((pos = regexStr.find("*", pos)) != std::string::npos)
                {
                    regexStr.replace(pos, 1, ".*");
                    pos += 2;
                }

                // Replace ? with .
                pos = 0;
                while ((pos = regexStr.find("?", pos)) != std::string::npos)
                {
                    regexStr.replace(pos, 1, ".");
                    pos += 1;
                }

                filterRegex = std::regex(regexStr, std::regex_constants::icase);
            }

            // Iterate directory
            for (const auto& entry : fs::directory_iterator(resolvedPath))
            {
                if (!entry.is_regular_file())
                {
                    continue;
                }

                std::string fileName = entry.path().filename().string();

                // Apply filter
                if (filter != "*" && !std::regex_match(fileName, filterRegex))
                {
                    continue;
                }

                FileInfo info;
                info.path = entry.path().string();
                info.name = fileName;
                info.extension = entry.path().extension().string();
                info.size = static_cast<size_t>(entry.file_size());
                info.isDirectory = false;

                auto lastWriteTime = entry.last_write_time();
                auto systemTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    lastWriteTime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
                info.lastModified = std::chrono::system_clock::to_time_t(systemTime);

                result.push_back(info);
            }
        }
        catch (const std::exception& e)
        {
            Log::error("Exception in listFiles: " + std::string(e.what()));
        }

        return result;
    }

    std::vector<FileInfo> FileSystem::listDirectories(const std::string& directory)
    {
        std::vector<FileInfo> result;

        try
        {
            std::lock_guard<std::mutex> lock(s_fileSystemMutex);

            // Resolve virtual path if needed
            std::string resolvedPath = resolvePathInternal(directory);

            if (!directoryExists(resolvedPath))
            {
                Log::error("Directory does not exist: " + resolvedPath);
                return result;
            }

            // Iterate directory
            for (const auto& entry : fs::directory_iterator(resolvedPath))
            {
                if (!entry.is_directory())
                {
                    continue;
                }

                FileInfo info;
                info.path = entry.path().string();
                info.name = entry.path().filename().string();
                info.extension = ""; // Directories don't have extensions
                info.size = 0; // Size doesn't apply to directories
                info.isDirectory = true;

                auto lastWriteTime = entry.last_write_time();
                auto systemTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    lastWriteTime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
                info.lastModified = std::chrono::system_clock::to_time_t(systemTime);

                result.push_back(info);
            }
        }
        catch (const std::exception& e)
        {
            Log::error("Exception in listDirectories: " + std::string(e.what()));
        }

        return result;
    }

    // Path Utility Functions
    std::string FileSystem::getPathExtension(const std::string& path)
    {
        try
        {
            fs::path fsPath(normalizePath(path));
            return fsPath.extension().string();
        }
        catch (const std::exception& e)
        {
            Log::error("Exception in getPathExtension: " + std::string(e.what()));
            return "";
        }
    }

    std::string FileSystem::getFileName(const std::string& path)
    {
        try
        {
            fs::path fsPath(normalizePath(path));
            return fsPath.filename().string();
        }
        catch (const std::exception& e)
        {
            Log::error("Exception in getFileName: " + std::string(e.what()));
            return "";
        }
    }

    std::string FileSystem::getFileNameWithoutExtension(const std::string& path)
    {
        try
        {
            fs::path fsPath(normalizePath(path));
            std::string filename = fsPath.filename().string();
            std::string extension = fsPath.extension().string();

            if (!extension.empty())
            {
                return filename.substr(0, filename.length() - extension.length());
            }

            return filename;
        }
        catch (const std::exception& e)
        {
            Log::error("Exception in getFileNameWithoutExtension: " + std::string(e.what()));
            return "";
        }
    }

    std::string FileSystem::getDirectoryPath(const std::string& path)
    {
        try
        {
            fs::path fsPath(normalizePath(path));
            return fsPath.parent_path().string();
        }
        catch (const std::exception& e)
        {
            Log::error("Exception in getDirectoryPath: " + std::string(e.what()));
            return "";
        }
    }

    std::string FileSystem::normalizePath(const std::string& path)
    {
        try
        {
            // Replace backslashes with forward slashes
            std::string normalized = path;
            std::replace(normalized.begin(), normalized.end(), '\\', '/');

            // Remove duplicate slashes
            size_t pos = 0;
            while ((pos = normalized.find("//", pos)) != std::string::npos)
            {
                normalized.replace(pos, 2, "/");
            }

            // Normalize path using filesystem
            fs::path fsPath(normalized);
            return fsPath.lexically_normal().string();
        }
        catch (const std::exception& e)
        {
            Log::error("Exception in normalizePath: " + std::string(e.what()));
            return path;
        }
    }

    std::string FileSystem::combinePaths(const std::string& path1, const std::string& path2)
    {
        try
        {
            fs::path p1(normalizePath(path1));
            fs::path p2(normalizePath(path2));
            return (p1 / p2).lexically_normal().string();
        }
        catch (const std::exception& e)
        {
            Log::error("Exception in combinePaths: " + std::string(e.what()));
            return path1 + PATH_SEPARATOR + path2;
        }
    }

    // File Watching Functions
    uint64_t FileSystem::addFileWatcher(const std::string& path, std::function<void(const std::string&)> callback)
    {
        std::lock_guard<std::mutex> lock(s_fileSystemMutex);

        if (!s_initialized)
        {
            Log::error("FileSystem not initialized, cannot add file watcher");
            return 0;
        }

        // Resolve virtual path if needed
        std::string resolvedPath = resolvePathInternal(path);

        // Check if file exists
        if (!fileExists(resolvedPath) && !directoryExists(resolvedPath))
        {
            Log::error("Cannot watch non-existent path: " + resolvedPath);
            return 0;
        }

        // Generate watcher ID
        uint64_t watcherId = s_nextWatcherId++;

        // Add to watchers map
        s_fileWatchers[watcherId] = std::make_pair(resolvedPath, callback);

        Log::info("Added file watcher for: " + resolvedPath);
        return watcherId;
    }

    void FileSystem::removeFileWatcher(uint64_t watcherId)
    {
        std::lock_guard<std::mutex> lock(s_fileSystemMutex);

        auto it = s_fileWatchers.find(watcherId);
        if (it != s_fileWatchers.end())
        {
            Log::info("Removed file watcher for: " + it->second.first);
            s_fileWatchers.erase(it);
        }
    }

    void FileSystem::fileWatcherThreadFunc()
    {
        if (!s_initialized)
        {
            return;
        }

        Log::info("File watcher thread started");

        std::unordered_map<std::string, time_t> lastModifiedTimes;

        // Initialize last modified times
        {
            std::lock_guard<std::mutex> lock(s_fileSystemMutex);
            for (const auto& watcher : s_fileWatchers)
            {
                if (fileExists(watcher.second.first))
                {
                    lastModifiedTimes[watcher.second.first] = getFileModificationTime(watcher.second.first);
                }
            }
        }

        while (s_watcherRunning)
        {
            // Check each watched file for changes
            std::vector<std::pair<std::string, std::function<void(const std::string&)>>> triggeredWatchers;

            {
                std::lock_guard<std::mutex> lock(s_fileSystemMutex);
                for (const auto& watcher : s_fileWatchers)
                {
                    const std::string& path = watcher.second.first;

                    if (fileExists(path))
                    {
                        time_t currentModifiedTime = getFileModificationTime(path);

                        // Check if we have a previous time recorded
                        auto it = lastModifiedTimes.find(path);
                        if (it != lastModifiedTimes.end())
                        {
                            // Compare times
                            if (currentModifiedTime > it->second)
                            {
                                // File was modified
                                triggeredWatchers.push_back(watcher.second);
                                it->second = currentModifiedTime; // Update last modified time
                            }
                        }
                        else
                        {
                            // First time seeing this file
                            lastModifiedTimes[path] = currentModifiedTime;
                        }
                    }
                }
            }

            // Trigger callbacks outside the lock
            for (const auto& watcher : triggeredWatchers)
            {
                try
                {
                    watcher.second(watcher.first);
                }
                catch (const std::exception& e)
                {
                    Log::error("Exception in file watcher callback: " + std::string(e.what()));
                }
            }

            // Sleep for a while to avoid high CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        Log::info("File watcher thread stopped");
    }

    // Virtual File System Functions
    void FileSystem::mountDirectory(const std::string& virtualPath, const std::string& realPath)
    {
        std::lock_guard<std::mutex> lock(s_fileSystemMutex);

        // Normalize paths
        std::string normalizedVirtualPath = normalizePath(virtualPath);
        std::string normalizedRealPath = normalizePath(realPath);

        // Check if real path exists
        if (!directoryExists(normalizedRealPath))
        {
            Log::warn("Cannot mount non-existent directory: " + normalizedRealPath);
            // Try to create the directory
            if (!createDirectory(normalizedRealPath))
            {
                Log::error("Failed to create mount point directory: " + normalizedRealPath);
                return;
            }
        }

        // Add/update mount point
        s_mountPoints[normalizedVirtualPath] = normalizedRealPath;
        Log::info("Mounted directory: " + normalizedVirtualPath + " -> " + normalizedRealPath);
    }

    void FileSystem::unmountDirectory(const std::string& virtualPath)
    {
        std::lock_guard<std::mutex> lock(s_fileSystemMutex);

        // Normalize path
        std::string normalizedVirtualPath = normalizePath(virtualPath);

        // Check if mount point exists
        auto it = s_mountPoints.find(normalizedVirtualPath);
        if (it != s_mountPoints.end())
        {
            Log::info("Unmounted directory: " + normalizedVirtualPath);
            s_mountPoints.erase(it);
        }
        else
        {
            Log::warn("Cannot unmount non-existent mount point: " + normalizedVirtualPath);
        }
    }

    std::string FileSystem::resolveVirtualPath(const std::string& virtualPath)
    {
        std::lock_guard<std::mutex> lock(s_fileSystemMutex);
        return resolvePathInternal(virtualPath);
    }

    // Asynchronous Operations
    std::future<std::string> FileSystem::readFileAsync(const std::string& path)
    {
        return std::async(std::launch::async, [path]() {
            std::string content;
            FileResult result = readFile(path, content);

            if (result != FileResult::Success)
            {
                throw std::runtime_error("Failed to read file: " + path);
            }

            return content;
                          });
    }

    std::future<std::vector<uint8_t>> FileSystem::readFileBinaryAsync(const std::string& path)
    {
        return std::async(std::launch::async, [path]() {
            std::vector<uint8_t> data;
            FileResult result = readFileBinary(path, data);

            if (result != FileResult::Success)
            {
                throw std::runtime_error("Failed to read binary file: " + path);
            }

            return data;
                          });
    }

    // Private Helper Methods
    std::string FileSystem::resolvePathInternal(const std::string& path)
    {
        // Normalize path first
        std::string normalizedPath = normalizePath(path);

        // Check for virtual path format (e.g., "virtual://assets/textures/file.png")
        if (normalizedPath.find("virtual://") == 0)
        {
            // Extract virtual part
            normalizedPath = normalizedPath.substr(10); // Remove "virtual://"

            // Find first directory separator
            size_t slashPos = normalizedPath.find('/');
            if (slashPos == std::string::npos)
            {
                // No slash, treat the whole string as mount point
                auto it = s_mountPoints.find(normalizedPath);
                if (it != s_mountPoints.end())
                {
                    return it->second;
                }

                // Mount point not found
                Log::error("Virtual path mount point not found: " + normalizedPath);
                return normalizedPath;
            }

            // Extract mount point name
            std::string mountPoint = normalizedPath.substr(0, slashPos);
            std::string remainingPath = normalizedPath.substr(slashPos + 1);

            // Look up mount point
            auto it = s_mountPoints.find(mountPoint);
            if (it != s_mountPoints.end())
            {
                // Combine real path with remaining path
                return combinePaths(it->second, remainingPath);
            }

            // Mount point not found
            Log::error("Virtual path mount point not found: " + mountPoint);
            return normalizedPath;
        }

        return normalizedPath;
    }

    FileResult FileSystem::checkFileAccess(const std::string& path, FileMode mode)
    {
        // Check if file exists
        if (!fileExists(path))
        {
            Log::error("File does not exist: " + path);
            return FileResult::NotFound;
        }

        // Check access rights based on mode
        bool readable = false;
        bool writable = false;

        try
        {
            // Use filesystem permissions to check access
            fs::perms permissions = fs::status(path).permissions();

            // Check owner permissions (assuming running as owner)
            if ((permissions & fs::perms::owner_read) != fs::perms::none)
            {
                readable = true;
            }

            if ((permissions & fs::perms::owner_write) != fs::perms::none)
            {
                writable = true;
            }
        }
        catch (const std::exception& e)
        {
            Log::error("Exception checking file permissions: " + std::string(e.what()));
            return FileResult::Unknown;
        }

        // Check based on mode
        switch (mode)
        {
            case FileMode::Read:
            case FileMode::ReadBinary:
                if (!readable)
                {
                    Log::error("No read permission for file: " + path);
                    return FileResult::AccessDenied;
                }
                break;

            case FileMode::Write:
            case FileMode::WriteBinary:
            case FileMode::Append:
            case FileMode::AppendBinary:
                if (!writable)
                {
                    Log::error("No write permission for file: " + path);
                    return FileResult::AccessDenied;
                }
                break;
        }

        return FileResult::Success;
    }

    bool FileSystem::ensureDirectoryExists(const std::string& path)
    {
        // If directory already exists, return true
        if (directoryExists(path))
        {
            return true;
        }

        return createDirectory(path);
    }

    // Platform-specific implementations
    FileResult FileSystem::platformReadFile(const std::string& path, void* buffer, size_t bufferSize, size_t& bytesRead, FileMode mode)
    {
        // This function would contain platform-specific file reading code
        // For now, we'll use std::fstream which is cross-platform

        try
        {
            std::ios_base::openmode openMode = std::ios::in;
            if (mode == FileMode::ReadBinary)
            {
                openMode |= std::ios::binary;
            }

            std::ifstream file(path, openMode);
            if (!file.is_open())
            {
                return FileResult::AccessDenied;
            }

            file.read(static_cast<char*>(buffer), bufferSize);
            bytesRead = file.gcount();

            if (file.fail() && !file.eof())
            {
                return FileResult::Unknown;
            }

            return FileResult::Success;
        }
        catch (const std::exception&)
        {
            return FileResult::Unknown;
        }
    }

    FileResult FileSystem::platformWriteFile(const std::string& path, const void* buffer, size_t bufferSize, FileMode mode)
    {
        // This function would contain platform-specific file writing code
        // For now, we'll use std::fstream which is cross-platform

        try
        {
            std::ios_base::openmode openMode;

            switch (mode)
            {
                case FileMode::Write:
                    openMode = std::ios::out;
                    break;
                case FileMode::WriteBinary:
                    openMode = std::ios::out | std::ios::binary;
                    break;
                case FileMode::Append:
                    openMode = std::ios::out | std::ios::app;
                    break;
                case FileMode::AppendBinary:
                    openMode = std::ios::out | std::ios::app | std::ios::binary;
                    break;
                default:
                    return FileResult::Unknown;
            }

            std::ofstream file(path, openMode);
            if (!file.is_open())
            {
                return FileResult::AccessDenied;
            }

            file.write(static_cast<const char*>(buffer), bufferSize);

            if (file.fail())
            {
                return FileResult::Unknown;
            }

            return FileResult::Success;
        }
        catch (const std::exception&)
        {
            return FileResult::Unknown;
        }
    }

} // namespace PixelCraft::Utility