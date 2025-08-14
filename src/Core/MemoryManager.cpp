// -------------------------------------------------------------------------
// MemoryManager.cpp
// -------------------------------------------------------------------------
#include "Core/MemoryManager.h"
#include "Core/Logger.h"

#include <algorithm>
#include <vector>
#include <cstdlib>
#include <cstring>

// In namespace PixelCraft
namespace PixelCraft::Core
{

    MemoryManager::MemoryManager()
        : Subsystem()
        , m_initialized(false)
        , m_lastDefragmentTime(0.0f)
        , m_defragmentInterval(30.0f)
        , m_enableDebugRendering(false)
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
        m_stats.rawAllocated = 0;
        m_stats.rawFreed = 0;
        m_stats.rawCurrentUsage = 0;
        m_stats.rawAllocationCount = 0;
        m_stats.rawDeallocationCount = 0;
        m_stats.pixelDataAllocated = 0;
        m_stats.pixelDataFreed = 0;
        m_stats.pixelDataCurrentUsage = 0;
        m_stats.pixelDataAllocationCount = 0;
        m_stats.pixelDataDeallocationCount = 0;

        // Initialize common pool sizes for raw allocations (powers of 2)
        m_poolSizes = {
            64,       // 64 bytes
            256,      // 256 bytes
            1024,     // 1 KB
            4096,     // 4 KB
            16384,    // 16 KB
            65536,    // 64 KB
            262144,   // 256 KB
            1048576,  // 1 MB
            4194304   // 4 MB
        };
    }

    MemoryManager::~MemoryManager()
    {
        if (m_initialized)
        {
            shutdown();
        }
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
        // Use internal method that doesn't acquire locks (we already have the lock)
        createPoolInternal<int>(128);
        createPoolInternal<float>(128);
        createPoolInternal<double>(128);
        createPoolInternal<std::string>(64);

        // Create raw memory pools for common sizes
        for (size_t poolSize : m_poolSizes)
        {
            auto pool = std::make_unique<MemoryPool>(poolSize, 32); // Start with 32 blocks per pool
            m_rawPools[poolSize] = std::move(pool);

            debug("MemoryManager: Created raw memory pool for " + std::to_string(poolSize) + " byte objects");
        }

        m_initialized = true;
        return true;
    }

    void MemoryManager::update(float deltaTime)
    {
        if (!m_initialized)
        {
            return;
        }

        m_lastDefragmentTime += deltaTime;

        // Periodic defragmentation
        if (m_lastDefragmentTime >= m_defragmentInterval)
        {
            // Only defragment if we have significant memory usage
            if (m_stats.currentUsage > 1024 * 1024) // 1MB threshold
            {
                debug("MemoryManager: Running periodic defragmentation");
                defragmentAll();
            }

            m_lastDefragmentTime = 0.0f;
        }
    }

    void MemoryManager::render()
    {
        if (!m_initialized || !m_enableDebugRendering)
        {
            return;
        }

        // Debug rendering would go here - for now, just log periodic stats
        static float lastStatsTime = 0.0f;
        lastStatsTime += 0.016f; // Assuming ~60 FPS

        if (lastStatsTime >= 5.0f) // Log every 5 seconds
        {
            auto stats = getAllocStats();
            debug("MemoryManager Stats - Current: " + std::to_string(stats.currentUsage) +
                  " bytes, Peak: " + std::to_string(stats.peakUsage) +
                  " bytes, Pools: " + std::to_string(stats.activePoolCount));
            lastStatsTime = 0.0f;
        }
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
            warn("MemoryManager: Potential memory leak detected:");
            warn("  - Pool memory: " + std::to_string(m_stats.currentUsage) + " bytes");
            warn("  - Raw memory: " + std::to_string(m_stats.rawCurrentUsage) + " bytes");
            warn("  - Pixel data: " + std::to_string(m_stats.pixelDataCurrentUsage) + " bytes");
        }

        // Output final statistics
        info("MemoryManager: Final statistics:");
        info("  - Pool allocated: " + std::to_string(m_stats.totalAllocated) + " bytes");
        info("  - Pool freed: " + std::to_string(m_stats.totalFreed) + " bytes");
        info("  - Raw allocated: " + std::to_string(m_stats.rawAllocated) + " bytes");
        info("  - Raw freed: " + std::to_string(m_stats.rawFreed) + " bytes");
        info("  - Pixel data allocated: " + std::to_string(m_stats.pixelDataAllocated) + " bytes");
        info("  - Pixel data freed: " + std::to_string(m_stats.pixelDataFreed) + " bytes");
        info("  - Peak usage: " + std::to_string(m_stats.peakUsage) + " bytes");
        info("  - Pool operations: " + std::to_string(m_stats.allocationCount) + "/" + std::to_string(m_stats.deallocationCount));
        info("  - Raw operations: " + std::to_string(m_stats.rawAllocationCount) + "/" + std::to_string(m_stats.rawDeallocationCount));
        info("  - Pixel operations: " + std::to_string(m_stats.pixelDataAllocationCount) + "/" + std::to_string(m_stats.pixelDataDeallocationCount));

        // Clear all pools
        m_pools.clear();
        m_rawPools.clear();

        // Reset statistics
        m_stats = AllocationStats{};

        m_initialized = false;
    }

    std::vector<std::string> MemoryManager::getDependencies() const
    {
        // MemoryManager has no dependencies
        return {};
    }

    void* MemoryManager::allocateRaw(size_t size, size_t alignment)
    {
        if (size == 0)
        {
            return nullptr;
        }

        void* ptr = nullptr;

        // Try to use a pool for common sizes
        size_t poolSize = findBestPoolSize(size);
        if (poolSize > 0)
        {
            MemoryPool* pool = getRawMemoryPool(poolSize);
            if (pool)
            {
                ptr = pool->allocate();
                if (ptr)
                {
                    // Zero out the memory
                    std::memset(ptr, 0, size);
                }
            }
        }

        // Fall back to aligned allocation for large of unusual sizes
        if (!ptr)
        {
            // Use aligned_alloc if available, otherwise use malloc with manual alignment
            #if defined(_MSC_VER)
                ptr = _aligned_malloc(size, alignment);
            #elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
                ptr = aligned_alloc(alignment, ((size + alignment - 1) / alignment) * alignment);
            #else
                // Manual alignment for older systems
                void* raw = std::malloc(size + alignment - 1 + sizeof(void*));
                if (raw)
                {
                    ptr = reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(raw) + sizeof(void*) + alignment - 1) & ~(alignment - 1));
                    *(reinterpret_cast<void**>(ptr) - 1) = raw;
                }
            #endif

            if (ptr)
            {
                std::memset(ptr, 0, size);
            }
        }

        if (ptr)
        {
            // Update statistics
            std::lock_guard<std::mutex> statsLock(m_statsMutex);
            m_stats.rawAllocated += size;
            m_stats.rawCurrentUsage += size;
            m_stats.peakUsage = std::max(m_stats.peakUsage, m_stats.currentUsage + m_stats.rawCurrentUsage + m_stats.pixelDataCurrentUsage);
            m_stats.rawAllocationCount++;
        }

        return ptr;
    }

    void MemoryManager::deallocateRaw(void* ptr, size_t size)
    {
        if (!ptr)
        {
            return;
        }

        // Try to return to a pool first
        size_t poolSize = findBestPoolSize(size);
        if (poolSize > 0)
        {
            MemoryPool* pool = getRawMemoryPool(poolSize);
            if (pool && pool->owns(ptr))
            {
                pool->deallocate(ptr);

                // Update statistics
                std::lock_guard<std::mutex> statsLock(m_statsMutex);
                m_stats.rawFreed += size;
                m_stats.rawCurrentUsage -= size;
                m_stats.rawDeallocationCount++;
                return;
            }
        }

        // Fall back to system deallocation
        #if defined(_MSC_VER)
            _aligned_free(ptr);
        #elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
            std::free(ptr);
        #else
            // Manual alignment cleanup
            void* raw = *(reinterpret_cast<void**>(ptr) - 1);
            std::free(raw);
        #endif

        // Update statistics
        std::lock_guard<std::mutex> statsLock(m_statsMutex);
        m_stats.rawFreed += size;
        m_stats.rawCurrentUsage -= size;
        m_stats.rawDeallocationCount++;
    }

    uint8_t* MemoryManager::allocatePixelData(int width, int height, int channels)
    {
        if (width <= 0 || height <= 0 || channels <= 0)
        {
            return nullptr;
        }

        size_t size = static_cast<size_t>(width) * height * channels;
        uint8_t* ptr = static_cast<uint8_t*>(allocateRaw(size, 16)); // 16-byte alignment for SIMD

        if (ptr)
        {
            // Additional pixel data tracking
            std::lock_guard<std::mutex> statsLock(m_statsMutex);
            m_stats.pixelDataAllocated += size;
            m_stats.pixelDataCurrentUsage += size;
            m_stats.pixelDataAllocationCount++;

            debug("MemoryManager: Allocated " + std::to_string(size) +
                  " bytes for pixel data (" + std::to_string(width) + "x" +
                  std::to_string(height) + "x" + std::to_string(channels) + ")");
        }

        return ptr;
    }

    void MemoryManager::deallocatePixelData(uint8_t* ptr, int width, int height, int channels)
    {
        if (!ptr)
        {
            return;
        }

        size_t size = static_cast<size_t>(width) * height * channels;

        // Additional pixel data tracking
        {
            std::lock_guard<std::mutex> statsLock(m_statsMutex);
            m_stats.pixelDataFreed += size;
            m_stats.pixelDataCurrentUsage -= size;
            m_stats.pixelDataDeallocationCount++;
        }

        deallocateRaw(ptr, size);

        debug("MemoryManager: Deallocated " + std::to_string(size) +
              " bytes of pixel data (" + std::to_string(width) + "x" +
              std::to_string(height) + "x" + std::to_string(channels) + ")");
    }

    std::unique_ptr<uint8_t[], std::function<void(uint8_t*)>>
    MemoryManager::makePixelDataUnique(int width, int height, int channels)
    {
        uint8_t* ptr = allocatePixelData(width, height, channels);
        if (!ptr)
        {
            return nullptr;
        }

        // Create lambda deleter that captures the dimensions
        auto deleter = [this, width, height, channels](uint8_t* p) {
            this->deallocatePixelData(p, width, height, channels);
        };

        return std::unique_ptr<uint8_t[], std::function<void(uint8_t*)>>(ptr, deleter);
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

        // Defragment typed pools
        for (auto& pair : m_pools)
        {
            pair.second->defragment();
            poolCount++;
        }

        // Defragment raw memory pools
        for (auto& pair : m_rawPools)
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

        // Find typed pools with no used blocks
        std::vector<std::type_index> poolsToRemove;
        for (const auto& pair : m_pools)
        {
            if (pair.second->getUsedBlocks() == 0)
            {
                poolsToRemove.push_back(pair.first);
            }
        }

        // Find raw pools with no used blocks
        std::vector<size_t> rawPoolsToRemove;
        for (const auto& pair : m_rawPools)
        {
            if (pair.second->getUsedBlocks() == 0)
            {
                rawPoolsToRemove.push_back(pair.first);
            }
        }

        // Remove unused typed pools
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

        // Remove unused raw pools
        for (const auto& poolSize : rawPoolsToRemove)
        {
            auto it = m_rawPools.find(poolSize);
            if (it != m_rawPools.end())
            {
                // Update statistics before removing
                {
                    std::lock_guard<std::mutex> statsLock(m_statsMutex);
                    m_stats.totalCapacity -= it->second->getCapacity() * it->second->getObjectSize();
                }

                // Remove the pool
                m_rawPools.erase(it);
            }
        }

        int totalRemoved = static_cast<int>(poolsToRemove.size() + rawPoolsToRemove.size());

        if (!poolsToRemove.empty())
        {
            info("MemoryManager: Released " + std::to_string(totalRemoved) + " unused memory pools");
        }

        return totalRemoved;
    }

    size_t MemoryManager::findBestPoolSize(size_t size)
    {
        // Find the smallest pool size that can accommodate the requested size
        for (size_t poolSize : m_poolSizes)
        {
            if (poolSize >= size)
            {
                return poolSize;
            }
        }

        // No suitable pool size found
        return 0;
    }

    MemoryPool* MemoryManager::getRawMemoryPool(size_t poolSize)
    {
        auto it = m_rawPools.find(poolSize);
        if (it != m_rawPools.end())
        {
            return it->second.get();
        }

        // Create a new pool if it doesn't exist and is a valid size
        auto sizeIt = std::find(m_poolSizes.begin(), m_poolSizes.end(), poolSize);
        if (sizeIt != m_poolSizes.end())
        {
            auto pool = std::make_unique<MemoryPool>(poolSize, 32);
            MemoryPool* poolPtr = pool.get();
            m_rawPools[poolSize] = std::move(pool);

            debug("MemoryManager: Created new raw memory pool for " + std::to_string(poolSize) + " byte objects");

            return poolPtr;
        }

        return nullptr;
    }

} // namespace PixelCraft::Core
