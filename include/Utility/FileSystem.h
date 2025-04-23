// -------------------------------------------------------------------------
// FileSystem.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <atomic>
#include <ctime>

namespace PixelCraft::Utility
{

    /**
     * @brief Enumeration of file access modes
     */
    enum class FileMode
    {
        Read,
        Write,
        Append,
        ReadBinary,
        WriteBinary,
        AppendBinary
    };

    /**
     * @brief File operation result codes
     */
    enum class FileResult
    {
        Success,
        NotFound,
        AccessDenied,
        InvalidPath,
        AlreadyExists,
        Unknown
    };

    /**
     * @brief File information structure
     */
    struct FileInfo
    {
        std::string path;
        std::string name;
        std::string extension;
        size_t size;
        bool isDirectory;
        time_t lastModified;
    };

    /**
     * @brief Platform-independent file system utility class
     *
     * Provides unified interface for file and directory operations across platforms
     * with support for asynchronous operations, path manipulation, virtual file system
     * and file watching for hot-reloading.
     */
    class FileSystem
    {
    public:
        /**
         * @brief Reads a text file into a string
         * @param path The path to the file
         * @param outContent The string that will receive the file content
         * @return Result of the operation
         */
        static FileResult readFile(const std::string& path, std::string& outContent);

        /**
         * @brief Reads a binary file into a byte vector
         * @param path The path to the file
         * @param outData The vector that will receive the file data
         * @return Result of the operation
         */
        static FileResult readFileBinary(const std::string& path, std::vector<uint8_t>& outData);

        /**
         * @brief Writes a string to a text file
         * @param path The path to the file
         * @param content The content to write
         * @return Result of the operation
         */
        static FileResult writeFile(const std::string& path, const std::string& content);

        /**
         * @brief Writes a byte vector to a binary file
         * @param path The path to the file
         * @param data The data to write
         * @return Result of the operation
         */
        static FileResult writeFileBinary(const std::string& path, const std::vector<uint8_t>& data);

        /**
         * @brief Appends a string to a text file
         * @param path The path to the file
         * @param content The content to append
         * @return Result of the operation
         */
        static FileResult appendFile(const std::string& path, const std::string& content);

        /**
         * @brief Appends a byte vector to a binary file
         * @param path The path to the file
         * @param data The data to append
         * @return Result of the operation
         */
        static FileResult appendFileBinary(const std::string& path, const std::vector<uint8_t>& data);

        /**
         * @brief Checks if a file exists
         * @param path The path to check
         * @return True if the file exists, false otherwise
         */
        static bool fileExists(const std::string& path);

        /**
         * @brief Deletes a file
         * @param path The path to the file to delete
         * @return True if the file was deleted, false otherwise
         */
        static bool deleteFile(const std::string& path);

        /**
         * @brief Copies a file from source to destination
         * @param source The source file path
         * @param destination The destination file path
         * @param overwrite If true, overwrites existing destination
         * @return True if the file was copied, false otherwise
         */
        static bool copyFile(const std::string& source, const std::string& destination, bool overwrite = false);

        /**
         * @brief Moves a file from source to destination
         * @param source The source file path
         * @param destination The destination file path
         * @param overwrite If true, overwrites existing destination
         * @return True if the file was moved, false otherwise
         */
        static bool moveFile(const std::string& source, const std::string& destination, bool overwrite = false);

        /**
         * @brief Gets the size of a file in bytes
         * @param path The path to the file
         * @return The size of the file in bytes, or 0 if the file doesn't exist
         */
        static size_t getFileSize(const std::string& path);

        /**
         * @brief Gets the last modification time of a file
         * @param path The path to the file
         * @return The last modification time as a time_t value, or 0 if the file doesn't exist
         */
        static time_t getFileModificationTime(const std::string& path);

        /**
         * @brief Creates a directory
         * @param path The path of the directory to create
         * @return True if the directory was created, false otherwise
         */
        static bool createDirectory(const std::string& path);

        /**
         * @brief Deletes a directory
         * @param path The path of the directory to delete
         * @param recursive If true, deletes all contents recursively
         * @return True if the directory was deleted, false otherwise
         */
        static bool deleteDirectory(const std::string& path, bool recursive = false);

        /**
         * @brief Checks if a directory exists
         * @param path The path to check
         * @return True if the directory exists, false otherwise
         */
        static bool directoryExists(const std::string& path);

        /**
         * @brief Lists all files in a directory
         * @param directory The directory path
         * @param filter Optional wildcard filter (e.g., "*.png")
         * @return Vector of FileInfo structures for each file
         */
        static std::vector<FileInfo> listFiles(const std::string& directory, const std::string& filter = "*");

        /**
         * @brief Lists all subdirectories in a directory
         * @param directory The directory path
         * @return Vector of FileInfo structures for each subdirectory
         */
        static std::vector<FileInfo> listDirectories(const std::string& directory);

        /**
         * @brief Gets the extension of a file path
         * @param path The file path
         * @return The extension including the dot (e.g., ".png")
         */
        static std::string getPathExtension(const std::string& path);

        /**
         * @brief Gets the filename from a path
         * @param path The file path
         * @return The filename with extension
         */
        static std::string getFileName(const std::string& path);

        /**
         * @brief Gets the filename without extension
         * @param path The file path
         * @return The filename without extension
         */
        static std::string getFileNameWithoutExtension(const std::string& path);

        /**
         * @brief Gets the directory part of a path
         * @param path The file path
         * @return The directory part of the path
         */
        static std::string getDirectoryPath(const std::string& path);

        /**
         * @brief Normalizes a path by removing redundant separators and up-level references
         * @param path The path to normalize
         * @return The normalized path
         */
        static std::string normalizePath(const std::string& path);

        /**
         * @brief Combines two paths
         * @param path1 The first path
         * @param path2 The second path
         * @return The combined path
         */
        static std::string combinePaths(const std::string& path1, const std::string& path2);

        /**
         * @brief Adds a file watcher that triggers a callback when the file changes
         * @param path The path to watch
         * @param callback The function to call when the file changes
         * @return A watcher ID that can be used to remove the watcher
         */
        static uint64_t addFileWatcher(const std::string& path, std::function<void(const std::string&)> callback);

        /**
         * @brief Removes a file watcher
         * @param watcherId The ID of the watcher to remove
         */
        static void removeFileWatcher(uint64_t watcherId);

        /**
         * @brief Mounts a real directory to a virtual path
         * @param virtualPath The virtual path prefix
         * @param realPath The real directory path
         */
        static void mountDirectory(const std::string& virtualPath, const std::string& realPath);

        /**
         * @brief Unmounts a virtual path
         * @param virtualPath The virtual path to unmount
         */
        static void unmountDirectory(const std::string& virtualPath);

        /**
         * @brief Resolves a virtual path to a real path
         * @param virtualPath The virtual path to resolve
         * @return The corresponding real path
         */
        static std::string resolveVirtualPath(const std::string& virtualPath);

        /**
         * @brief Asynchronously reads a text file
         * @param path The path to the file
         * @return Future containing the file content as a string
         */
        static std::future<std::string> readFileAsync(const std::string& path);

        /**
         * @brief Asynchronously reads a binary file
         * @param path The path to the file
         * @return Future containing the file content as a byte vector
         */
        static std::future<std::vector<uint8_t>> readFileBinaryAsync(const std::string& path);

        /**
         * @brief Initializes the FileSystem subsystem
         * @return True if initialization was successful
         */
        static bool initialize();

        /**
         * @brief Shuts down the FileSystem subsystem
         */
        static void shutdown();

    private:
        // Prevent instantiation
        FileSystem() = delete;
        ~FileSystem() = delete;
        FileSystem(const FileSystem&) = delete;
        FileSystem& operator=(const FileSystem&) = delete;

        // Implementation details
        static std::unordered_map<std::string, std::string> s_mountPoints;
        static std::unordered_map<uint64_t, std::pair<std::string, std::function<void(const std::string&)>>> s_fileWatchers;
        static uint64_t s_nextWatcherId;
        static std::mutex s_fileSystemMutex;
        static bool s_initialized;

        // Platform-specific implementations
        /**
         * @brief Platform-specific file reading implementation
         * @param path The path to the file
         * @param buffer The buffer to read into
         * @param bufferSize The size of the buffer
         * @param bytesRead Will be set to the number of bytes read
         * @param mode The file access mode
         * @return Result of the operation
         */
        static FileResult platformReadFile(const std::string& path, void* buffer, size_t bufferSize, size_t& bytesRead, FileMode mode);

        /**
         * @brief Platform-specific file writing implementation
         * @param path The path to the file
         * @param buffer The buffer to write from
         * @param bufferSize The size of the buffer
         * @param mode The file access mode
         * @return Result of the operation
         */
        static FileResult platformWriteFile(const std::string& path, const void* buffer, size_t bufferSize, FileMode mode);

        // File watching thread
        static std::thread s_watcherThread;
        static std::atomic<bool> s_watcherRunning;

        /**
         * @brief Thread function for file watching
         */
        static void fileWatcherThreadFunc();

        // Helper methods
        /**
         * @brief Resolves a path, handling virtual paths
         * @param path The path to resolve
         * @return The resolved path
         */
        static std::string resolvePathInternal(const std::string& path);

        /**
         * @brief Checks if a file is accessible with the given mode
         * @param path The path to check
         * @param mode The access mode to check
         * @return Result indicating if the file is accessible
         */
        static FileResult checkFileAccess(const std::string& path, FileMode mode);

        /**
         * @brief Ensures a directory exists, creating it if necessary
         * @param path The directory path
         * @return True if the directory exists or was created
         */
        static bool ensureDirectoryExists(const std::string& path);
    };

} // namespace PixelCraft::Utility