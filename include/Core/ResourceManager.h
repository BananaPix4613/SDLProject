// -------------------------------------------------------------------------
// ResourceManager.h
// -------------------------------------------------------------------------
#pragma once

#include "Core/Resource.h"
#include "Core/ThreadSafeQueue.h"
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <thread>
#include <functional>
#include <atomic>
#include <filesystem>

namespace PixelCraft::Core
{

    /**
     * @brief Unified resource loading, caching, and hot-reloading
     */
    class ResourceManager
    {
    public:
        /**
         * @brief Get the singleton instance
         * @return Reference to the ResourceManager instance
         */
        static ResourceManager& getInstance();

        /**
         * @brief Initialize the resource manager
         * @return True if initialization was successful
         */
        bool initialize();

        /**
         * @brief Shut down the resource manager and release resources
         */
        void shutdown();

        /**
         * @brief Load a resource of the specified type
         * @param path The file path of the resource
         * @return Shared pointer to the loaded resource
         */
        template<typename T>
        std::shared_ptr<T> load(const std::string& path)
        {
            static_assert(std::is_base_of<Resource, T>::value, "T must derive from Resource");

            std::type_index typeIndex(typeid(T));

            // Check if already loaded
            {
                std::lock_guard<std::mutex> lock(m_resourceMutex);
                auto resourceMap = m_resources.find(typeIndex);
                if (resourceMap != m_resources.end())
                {
                    auto resource = resourceMap->second.find(path);
                    if (resource != resourceMap->second.end())
                    {
                        resource->second->addRef();
                        return std::static_pointer_cast<T>(resource->second);
                    }
                }
            }

            // Create and load the resources
            std::shared_ptr<T> resource = std::make_shared<T>(path);
            resource->addRef(); // Initial reference

            if (m_asyncLoadingEnabled)
            {
                // Queue for async loading
                ResourceTask task;
                task.path = path;
                task.typeIndex = typeIndex;
                task.resource = resource;
                task.callback = nullptr;

                m_asyncQueue.push(task);
            }
            else
            {
                // Load synchronously
                if (!resource->load())
                {
                    return nullptr;
                }

                // Store file modification time for hot reloading
                if (std::filesystem::exists(path))
                {
                    std::lock_guard<std::mutex> lock(m_resourceMutex);
                    m_fileTimes[path] = std::filesystem::last_write_time(path);
                }
            }

            // Store in the resource map
            {
                std::lock_guard<std::mutex> lock(m_resourceMutex);
                m_resources[typeIndex][path] = resource;
            }

            return resource;
        }

        /**
         * @brief Load a resource asynchronously
         * @param path The file path of the resource
         * @param callback Function to call when loading completes
         */
        template<typename T>
        void loadAsync(const std::string& path, std::function<void(std::shared_ptr<T>)> callback)
        {
            static_assert(std::is_base_of<Resource, T>::value, "T must derive from Resource");

            std::type_index typeIndex(typeid(T));

            // Check if already loaded
            {
                std::lock_guard<std::mutex> lock(m_resourceMutex);
                auto resourceMap = m_resources.find(typeIndex);
                if (resourceMap != m_resources.end())
                {
                    auto resource = resourceMap->second.find(path);
                    if (resource != resourceMap->second.end())
                    {
                        resource->second->addRef();
                        if (callback)
                        {
                            callback(static_cast<T>(resource->second));
                        }
                        return;
                    }
                }
            }

            // Create the resource
            std::shared_ptr<T> resource = std::make_shared<T>(path);
            resource->addRef(); // Initial reference

            // Store in the resource map
            {
                std::lock_guard<std::mutex> lock(m_resourceMutex);
                m_resources[typeIndex][path] = resource;
            }

            // Queue for async loading
            ResourceTask task;
            task.path = path;
            task.typeIndex = typeIndex;
            task.resource = resource;
            if (callback)
            {
                task.callback = [callback, resource](std::shared_ptr<Resource>) {
                    callback(std::static_pointer_cast<T>(resource));
                    };
            }

            m_asyncQueue.push(task);
        }

        /**
         * @brief Unload a resource and release memory
         * @param path The file path of the resource to unload
         */
        template<typename T>
        void unload(const std::string& path)
        {
            static_assert(std::is_base_of<Resource, T>::value, "T must derive from Resource");

            std::type_index typeIndex(typeid(T));
            std::shared_ptr<Resource> resourceToUnload;

            {
                std::lock_guard<std::mutex> lock(m_resourceMutex);
                auto resourceMap = m_resources.find(typeIndex);
                if (resourceMap == m_resources.end())
                {
                    return;
                }

                auto resource = resourceMap->second.find(path);
                if (resource == resourceMap->second.end())
                {
                    return;
                }

                // Decrement reference count
                int refCount = resource->second->releaseRef();

                // If no more reference, unload and remove from map
                if (refCount <= 0)
                {
                    resourceToUnload = resource->second;
                    resourceMap->second.erase(path);

                    // If no more resources of this type, remove the type map
                    if (resourceMap->second.empty())
                    {
                        m_resources.erase(typeIndex);
                    }
                }
            }

            // Actually unload the resource outside the lock to prevent deadlocks
            if (resourceToUnload)
            {
                resourceToUnload->unload();
            }
        }

        /**
         * @brief Get a resource without loading it
         * @param path The file path of the resource
         * @return Shared pointer to the resource or nullptr if not found
         */
        template<typename T>
        std::shared_ptr<T> getResource(const std::string& path)
        {
            static_assert(std::is_base_of<Resource, T>::value, "T must derive from Resource");

            std::type_index typeIndex(typeid(T));

            std::lock_guard<std::mutex> lock(m_resourceMutex);
            auto resourceMap = m_resources.find(typeIndex);
            if (resourceMap == m_resources.end())
            {
                return nullptr;
            }

            auto resource = resourceMap->second.find(path);
            if (resource == resourceMap->second.end())
            {
                return nullptr;
            }

            return std::static_pointer_cast<T>(resource->second);
        }

        /**
         * @brief Create a new resource without a file path
         * @param name The name of the resource
         * @return Shared pointer to the created resource
         */
        template<typename T>
        std::shared_ptr<T> createResource(const std::string& name)
        {
            static_assert(std::is_base_of<Resource, T>::value, "T must derive from Resource");

            std::type_index typeIndex(typeid(T));

            // Create the resource with a virtual path
            std::string virtualPath = "memory://" + name;

            // Check if already exists
            {
                std::lock_guard<std::mutex> lock(m_resourceMutex);
                auto resourceMap = m_resources.find(typeIndex);
                if (resourceMap != m_resources.end())
                {
                    auto resource = resourceMap->second.find(virtualPath);
                    if (resource != resourceMap->second.end())
                    {
                        return std::static_pointer_cast<T>(resource->second);
                    }
                }
            }

            // Create the resource
            std::shared_ptr<T> resource = std::make_shared<T>(virtualPath);
            resource->addRef(); // Initial reference

            // Store in the resource map
            {
                std::lock_guard<std::mutex> lock(m_resourceMutex);
                m_resources[typeIndex][virtualPath] = resource;
            }

            return resource;
        }

        /**
         * @brief Check for modified resources and reload them
         */
        void reloadModified();

        /**
         * @brief Enable or disable asynchronous loading
         * @param enabled True to enable async loading, false for synchronous
         */
        void setAsyncLoading(bool enabled);

    private:
        /**
         * @brief Constructor (private for singleton)
         */
        ResourceManager();

        /**
         * @brief Destructor
         */
        ~ResourceManager();

        // Prevent copying
        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;

        /**
         * @brief Async loading thread function
         */
        void asyncLoadingThreadFunc();

        // Type-specific resource maps
        std::unordered_map<std::type_index, std::unordered_map<std::string, std::shared_ptr<Resource>>> m_resources;

        // File modification times for hot reloading
        std::unordered_map<std::string, std::filesystem::file_time_type> m_fileTimes;

        // Async loading queue
        struct ResourceTask
        {
            std::string path;
            std::type_index typeIndex = typeid(void); // Default to void type
            std::shared_ptr<Resource> resource;
            std::function<void(std::shared_ptr<Resource>)> callback;
        };

        ThreadSafeQueue<ResourceTask> m_asyncQueue;
        std::thread m_loadingThread;
        std::atomic<bool> m_asyncLoadingEnabled;
        std::atomic<bool> m_threadRunning;

        // Mutex for resource map access
        mutable std::mutex m_resourceMutex;
    };

} // namespace PixelCraft::Core