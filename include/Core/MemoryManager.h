// -------------------------------------------------------------------------
// MemoryManager.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <mutex>
#include <typeindex>
#include <atomic>
#include "Core/MemoryPool.h"

namespace PixelCraft::Core
{

    /**
     * @brief Central memory management system with pooling, defragmentation, and diagnostics
     * 
     * Manages multiple typed memory pools, provides custom allocators for smart pointers,
     * and tracks memory usage throughout the engine.
     */
    class MemoryManager
    {
    public:
        /**
         * @brief Get the singleton instance
         * @return Reference to the MemoryManager instance
         */
        static MemoryManager& getInstance();

        /**
         * @brief Initialize the memory manager
         * @return True if initialization was successful
         */
        bool initialize();

        /**
         * @brief Shut down the memory manager and release all memory
         */
        void shutdown();

        /**
         * @brief Create a memory pool for a specific type
         * @tparam T Type of objects to be allocated from the pool
         * @param initialCapacity Initial number of objects the pool can hold
         * @return Pointer to the created memory pool
         */
        template<typename T>
        MemoryPool* createPool(size_t initialCapacity = 64)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            auto typeIndex = std::type_index(typeid(T));
            auto it = m_pools.find(typeIndex);

            // Return existing pool if already created
            if (it != m_pools.end())
            {
                return it->second.get();
            }

            // Create new pool
            auto pool = std::make_unique<MemoryPool>(sizeof(T), initialCapacity);
            MemoryPool* poolPtr = pool.get();
            m_pools[typeIndex] = std::move(pool);

            // Update statistics
            {
                std::lock_guard<std::mutex> statsLock(m_statsMutex);
                m_stats.activePoolCount++;
                m_stats.totalCapacity += initialCapacity * sizeof(T);
            }

            return poolPtr;
        }

        /**
         * @brief Get an existing memory pool for a specific type
         * @tparam T Type of objects allocated from the pool
         * @return Pointer to the memory pool or nullptr if not found
         */
        template<typename T>
        MemoryPool* getPool()
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            auto typeIndex = std::type_index(typeid(T));
            auto it = m_pools.find(typeIndex);

            return (it != m_pools.end()) ? it->second.get() : nullptr;
        }

        /**
         * @brief Allocate a single object of type T from its pool
         * @tparam T Type of object to allocate
         * @return Pointer to the allocated object (nullptr if allocation failed)
         */
        template<typename T>
        T* allocate()
        {
            MemoryPool* pool = getPool<T>();
            if (!pool)
            {
                pool = createPool<T>();
            }

            void* memory = pool->allocate();
            if (!memory)
            {
                return nullptr;
            }

            // Update statistics
            {
                std::lock_guard<std::mutex> statsLock(m_statsMutex);
                size_t size = sizeof(T);
                m_stats.totalAllocated += size;
                m_stats.currentUsage += size;
                m_stats.peakUsage = std::max(m_stats.peakUsage, m_stats.currentUsage);
                m_stats.allocationCount++;
            }

            // Construct the object in-place
            return new(memory) T();
        }

        /**
         * @brief Deallocate an object previously allocated from a pool
         * @tparam T Type of object to deallocate
         * @param ptr Pointer to the object
         */
        template<typename T>
        void deallocate(T* ptr)
        {
            if (!ptr) return;

            // Call destructor
            ptr->~T();

            // Return memory to the pool
            MemoryPool* pool = getPool<T>();
            if (pool)
            {
                pool->deallocate(ptr);

                // Update statistics
                {
                    std::lock_guard<std::mutex> statsLock(m_statsMutex);
                    m_stats.totalFreed += sizeof(T);
                    m_stats.currentUsage -= sizeof(T);
                    m_stats.deallocationCount++;
                }
            }
        }

        /**
         * @brief Structure for tracking memory allocation statistics
         */
        struct AllocationStats
        {
            size_t totalAllocated{0};     // Total bytes allocated since startup
            size_t totalFreed{0};         // Total bytes freed since startup
            size_t currentUsage{0};       // Current memory usage in bytes
            size_t peakUsage{0};          // Peak memory usage in bytes
            size_t totalCapacity{0};      // Total capacity of all pools in bytes
            int activePoolCount{0};       // Number of active memory pools
            int allocationCount{0};       // Number of allocation operations
            int deallocationCount{0};     // Number of deallocation operations
        };

        /**
         * @brief Get current memory allocation statistics
         * @return Statistics structure with allocation data
         */
        AllocationStats getAllocStats() const;

        /**
         * @brief Defragment all memory pools
         * 
         * This is a potentially expensive operation and should be used during
         * loading screens or other non-performance-critical moments.
         */
        void defragmentAll();

        /**
         * @brief Release all unused memory pools
         * @return Number of pools released
         */
        int releaseUnused();

        /**
         * @brief Custom deleter for use with smart pointers
         * @tparam T Type of object to delete
         */
        template<typename T>
        struct PoolDeleter
        {
            void operator()(T* ptr) const
            {
                MemoryManager::getInstance().deallocate<T>(ptr);
            }
        };

        /**
         * @brief Create a shared_ptr using pool allocation
         * @tparam T Type of object to create
         * @tparam Args Constructor argument types
         * @param args Constructor arguments
         * @return shared_ptr to the newly created object
         */
        template<typename T, typename... Args>
        std::shared_ptr<T> makeShared(Args&&... args)
        {
            T* ptr = allocate<T>();
            if (!ptr) return nullptr;

            // Placement new with constructor arguments
            new(ptr) T(std::forward<Args>(args)...);

            // Create shared_ptr with custom deleter
            return std::shared_ptr<T>(ptr, PoolDeleter<T>())
        }

        /**
         * @brief Create a unique_ptr using pool allocation
         * @tparam T Type of object to create
         * @tparam Args Constructor argument types
         * @param args Constructor arguments
         * @return unique_ptr to the newly created object
         */
        template<typename T, typename... Args>
        std::unique_ptr<T, PoolDeleter<T>> makeUnique(Args&&... args)
        {
            T* ptr = allocate<T>();
            if (!ptr) return nullptr;

            // Placement new with constructor arguments
            new(ptr) T(std::forward<Args>(args)...);

            // Create unique_ptr with custom deleter
            return std::unique_ptr<T, PoolDeleter<T>>(ptr, PoolDeleter<T>());
        }

    private:
        /**
         * @brief Private constructor for singleton pattern
         */
        MemoryManager();

        /**
         * @brief Private destructor for singleton pattern
         */
        ~MemoryManager();

        // Delete copy/move constructors and assignment operators
        MemoryManager(const MemoryManager&) = delete;
        MemoryManager& operator=(const MemoryManager&) = delete;
        MemoryManager(MemoryManager&&) = delete;
        MemoryManager& operator=(MemoryManager&&) = delete;

        std::unordered_map<std::type_index, std::unique_ptr<MemoryPool>> m_pools;
        mutable std::mutex m_mutex;
        mutable std::mutex m_statsMutex;
        bool m_initialized;
        AllocationStats m_stats;
    };

    /**
     * @brief Convenience function to create a shared_ptr using the memory manager
     * @tparam T Type of object to create
     * @tparam Args Constructor argument types
     * @param args Constructor arguments
     * @return shared_ptr to he newly created object
     */
    template<typename T, typename... Args>
    std::shared_ptr<T> makeShared(Args&&... args)
    {
        return MemoryManager::getInstance().makeShared<T>(std::forward<Args>(args)...);
    }

    /**
     * @brief Convenience function to create a unique_ptr using the memory manager
     * @tparam T Type of object to create
     * @tparam Args Constructor argument types
     * @param args Constructor arguments
     * @return unique_ptr to the newly created object
     */
    template<typename T, typename... Args>
    std::unique_ptr<T, MemoryManager::PoolDeleter<T>> makeUnique(Args&&... args)
    {
        return MemoryManager::getInstance().makeUnique<T>(std::forward<Args>(args)...);
    }

} // namespace PixelCraft::Core