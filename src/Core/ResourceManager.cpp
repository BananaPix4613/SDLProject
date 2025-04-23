// -------------------------------------------------------------------------
// ResourceManager.cpp
// -------------------------------------------------------------------------
#include "Core/ResourceManager.h"
#include "Core/Logger.h"
#include <chrono>
#include <vector>

namespace PixelCraft::Core
{

    ResourceManager::ResourceManager()
        : m_asyncLoadingEnabled(false), m_threadRunning(false)
    {
    }

    ResourceManager::~ResourceManager()
    {
        shutdown();
    }

    ResourceManager& ResourceManager::getInstance()
    {
        static ResourceManager instance;
        return instance;
    }

    bool ResourceManager::initialize()
    {
        info("Initializing ResourceManager");

        // Start async loading thread if enabled
        if (m_asyncLoadingEnabled)
        {
            m_threadRunning = true;
            m_loadingThread = std::thread(&ResourceManager::asyncLoadingThreadFunc, this);
        }

        return true;
    }

    void ResourceManager::shutdown()
    {
        info("Shutting down ResourceManager");

        // Stop async loading thread
        if (m_threadRunning)
        {
            m_threadRunning = false;
            m_asyncQueue.shutdown();
            if (m_loadingThread.joinable())
            {
                m_loadingThread.join();
            }
        }

        // Unload all resources
        std::vector<std::shared_ptr<Resource>> resourcesToUnload;

        {
            std::lock_guard<std::mutex> lock(m_resourceMutex);
            for (auto& typeMap : m_resources)
            {
                for (auto& resource : typeMap.second)
                {
                    resourcesToUnload.push_back(resource.second);
                }
            }

            m_resources.clear();
            m_fileTimes.clear();
        }

        // Unload resources outside the lock
        for (auto& resource : resourcesToUnload)
        {
            resource->unload();
        }
    }

    void ResourceManager::setAsyncLoading(bool enabled)
    {
        if (enabled == m_asyncLoadingEnabled)
        {
            return;
        }

        m_asyncLoadingEnabled = enabled;

        if (enabled && !m_threadRunning)
        {
            // Start the loading thread
            m_threadRunning = true;
            m_loadingThread = std::thread(&ResourceManager::asyncLoadingThreadFunc, this);
        }
        else if (!enabled && m_threadRunning)
        {
            // Stop the loading thread
            m_threadRunning = false;
            m_asyncQueue.shutdown();
            if (m_loadingThread.joinable())
            {
                m_loadingThread.join();
            }
        }
    }

    void ResourceManager::asyncLoadingThreadFunc()
    {
        info("Starting async resource loading thread");

        while (m_threadRunning)
        {
            ResourceTask task;
            m_asyncQueue.waitAndPop(task);

            // Check if we're shutting down
            if (!m_threadRunning)
            {
                break;
            }

            // Load the resource
            bool success = false;
            if (task.resource)
            {
                success = task.resource->load();
            }

            // Store file modification time for hot reloading
            if (success && !task.path.empty() && task.path.find("memory://") != 0)
            {
                try
                {
                    if (std::filesystem::exists(task.path))
                    {
                        std::lock_guard<std::mutex> lock(m_resourceMutex);
                        m_fileTimes[task.path] = std::filesystem::last_write_time(task.path);
                    }
                }
                catch (const std::exception& e)
                {
                    warn("Failed to get modification time for " + task.path + ": " + e.what());
                }
            }

            // Call the callback if provided
            if (task.callback)
            {
                task.callback(task.resource);
            }

            // If loading failed, remove from resource map
            if (!success)
            {
                std::lock_guard<std::mutex> lock(m_resourceMutex);
                auto resourceMap = m_resources.find(task.typeIndex);
                if (resourceMap != m_resources.end())
                {
                    resourceMap->second.erase(task.path);

                    // If no more resources of this type, remove the type map
                    if (resourceMap->second.empty())
                    {
                        m_resources.erase(task.typeIndex);
                    }
                }
            }
        }

        info("Async resource loading thread stopped");
    }

    void ResourceManager::reloadModified()
    {
        std::vector<std::pair<std::string, std::shared_ptr<Resource>>> resourcesToReload;

        // First, check for modified files
        {
            std::lock_guard<std::mutex> lock(m_resourceMutex);

            for (const auto& typeMap : m_resources)
            {
                for (const auto& resource : typeMap.second)
                {
                    const std::string& path = resource.first;

                    // Skip memory resources
                    if (path.find("memory://") == 0)
                    {
                        continue;
                    }

                    try
                    {
                        // Check if file exists
                        if (!std::filesystem::exists(path))
                        {
                            continue;
                        }

                        // Get last modified time
                        auto lastWriteTime = std::filesystem::last_write_time(path);

                        // Check if file has been modified
                        auto it = m_fileTimes.find(path);
                        if (it == m_fileTimes.end() || lastWriteTime > it->second)
                        {
                            // File has been modified, add to reload list
                            resourcesToReload.push_back({path, resource.second});

                            // Update modification time
                            m_fileTimes[path] = lastWriteTime;
                        }
                    }
                    catch (const std::exception& e)
                    {
                        warn("Failed to check modification time for " + path + ": " + e.what());
                    }
                }
            }
        }

        // Now reload the modified resources
        for (const auto& resource : resourcesToReload)
        {
            info("Hot reloading resource: " + resource.first);
            resource.second->onReload();
        }
    }

} // namespace PixelCraft::Core