// -------------------------------------------------------------------------
// MemoryManager.cpp
// -------------------------------------------------------------------------
#include "Core/MemoryManager.h"
#include "Logger.h"
#include <algorithm>
#include <vector>

namespace PixelCraft::Core
{

    // Static instance for singleton pattern
    static std::unique_ptr<MemoryManager> s_instance = nullptr;

    MemoryManager::MemoryManager()
        : m_initialized(false)
    {
        // Initialize statistics
        m_stats.totalAllocated = 0;
        m_stats.totalFreed = 0;
        m_stats.currentUsage = 0;
        m_stats.peakUsage = 0;
        m_stats.totalCapacity = 0;
        m_stats.activePoolCount = 0;
        m_stats.allocationCount = 0;
        m_stats.deallocationCount = 0;
    }

    MemoryManager::~MemoryManager()
    {
        if (m_initialized)
        {
            shutdown();
        }
    }

    MemoryManager& MemoryManager::getInstance()
    {
        if (!s_instance)
        {
            s_instance = std::unique_ptr<MemoryManager>(new MemoryManager());
        }
        return *s_instance;
    }

    bool MemoryManager::initialize()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_initialized)
        {
            warn("MemoryManager: Already initialized");
            return true;
        }

        info("MemoryManager: Initializing memory management system");

        // Create some common pools by default for better performance
        createPool<int>(128);
        createPool<float>(128);
        createPool<double>(128);
        createPool<std::string>(64);

        m_initialized = true;
        return true;
    }

    void MemoryManager::shutdown()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_initialized)
        {
            return;
        }

        info("MemoryManager: Shutting down memory management system");

        // Check for memory leaks
        if (m_stats.currentUsage > 0)
        {
            warn("MemoryManager: Potential memory leak detected. " +
                 std::to_string(m_stats.currentUsage) + " bytes still allocated at shutdown");
        }

        // Output final statistics
        info("MemoryManager: Final statistics:");
        info("  - Total allocated: " + std::to_string(m_stats.totalAllocated) + " bytes");
        info("  - Total freed: " + std::to_string(m_stats.totalFreed) + " bytes");
        info("  - Peak usage: " + std::to_string(m_stats.peakUsage) + " bytes");
        info("  - Allocation operations: " + std::to_string(m_stats.allocationCount));
        info("  - Deallocation operations: " + std::to_string(m_stats.deallocationCount));

        // Clear all pools
        m_pools.clear();

        // Reset statistics
        m_stats.totalAllocated = 0;
        m_stats.totalFreed = 0;
        m_stats.currentUsage = 0;
        m_stats.peakUsage = 0;
        m_stats.totalCapacity = 0;
        m_stats.activePoolCount = 0;
        m_stats.allocationCount = 0;
        m_stats.deallocationCount = 0;

        m_initialized = false;
    }

    MemoryManager::AllocationStats MemoryManager::getAllocStats() const
    {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        return m_stats;
    }

    void MemoryManager::defragmentAll()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_initialized)
        {
            warn("MemoryManager: Cannot defragment, manager not initialized");
            return;
        }

        info("MemoryManager: Starting defragmentation of all memory pools");

        size_t poolCount = 0;
        for (auto& pair : m_pools)
        {
            pair.second->defragment();
            poolCount++;
        }

        info("MemoryManager: Defragmented " + std::to_string(poolCount) + " memory pools");
    }

    int MemoryManager::releaseUnused()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_initialized)
        {
            warn("MemoryManager: Cannot release unused pools, manager not initialized");
            return 0;
        }

        // Find pools with no used blocks
        std::vector<std::type_index> poolsToRemove;
        for (const auto& pair : m_pools)
        {
            if (pair.second->getUsedBlocks() == 0)
            {
                poolsToRemove.push_back(pair.first);
            }
        }

        // Remove unused pools
        for (const auto& typeIndex : poolsToRemove)
        {
            auto it = m_pools.find(typeIndex);
            if (it != m_pools.end())
            {
                // Update statistics before removing
                {
                    std::lock_guard<std::mutex> statsLock(m_statsMutex);
                    m_stats.totalCapacity -= it->second->getCapacity() * it->second->getObjectSize();
                    m_stats.activePoolCount--;
                }

                // Remove the pool
                m_pools.erase(it);
            }
        }

        if (!poolsToRemove.empty())
        {
            info("MemoryManager: Released " + std::to_string(poolsToRemove.size()) +
                 " unused memory pools");
        }

        return static_cast<int>(poolsToRemove.size());
    }
}