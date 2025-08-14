// -------------------------------------------------------------------------
// MemoryManager.h
// -------------------------------------------------------------------------
#pragma once

#include "Core/Application.h"
#include "Core/Subsystem.h"
#include "Core/MemoryPool.h"

#include <memory>
#include <unordered_map>
#include <string>
#include <mutex>
#include <typeindex>
#include <atomic>
#include <vector>
#include <functional>

// In namespace PixelCraft
namespace PixelCraft::Core
{

    /**
     * @brief Central memory management system with pooling, defragmentation, and diagnostics
     * 
     * Manages multiple typed memory pools, provides custom allocators for smart pointers,
     * and tracks memory usage throughout the engine.
     */
    class MemoryManager : public Subsystem
    {
    public:
        /**
         * @brief Constructor
         */
        MemoryManager();

        /**
         * @brief Destructor
         */
        virtual ~MemoryManager();

        /**
         * @brief Initialize the memory manager
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Update the memory manager (periodic cleanup, defragmentation checks)
         * @param deltaTime Time since last update
         */
        void update(float deltaTime) override;

        /**
         * @brief Render debug information if enabled
         */
        void render() override;

        /**
         * @brief Shut down the memory manager and release all memory
         */
        void shutdown() override;

        /**
         * @brief Get the name of this subsystem
         * @return The subsystem name
         */
        std::string getName() const override
        {
            return "MemoryManager";
        }

        /**
         * @brief Get dependencies for this subsystem
         * @return Empty vector (no dependencies)
         */
        std::vector<std::string> getDependencies() const override;

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
         * @brief Allocate variable-sized raw memory with tracking
         * @param size Number of bytes to allocate
         * @param alignment Memory alignment requirement (default: 16 bytes)
         * @return Pointer to allocated memory or nullptr if allocation failed
         */
        void* allocateRaw(size_t size, size_t alignment = 16);

        /**
         * @brief Deallocate variable-sized raw memory
         * @param ptr Pointer to memory previously allocated with allocateRaw
         * @param size Size of the allocation (must match original allocation size)
         */
        void deallocateRaw(void* ptr, size_t size);

        /**
         * @brief Allocate pixel data with specific tracking
         * @param width Width of the texture
         * @param height Height of the texture  
         * @param channels Number of color channels
         * @return Pointer to allocated pixel data or nullptr if allocation failed
         */
        uint8_t* allocatePixelData(int width, int height, int channels);

        /**
         * @brief Deallocate pixel data
         * @param ptr Pointer to pixel data
         * @param width Width of the texture
         * @param height Height of the texture
         * @param channels Number of color channels
         */
        void deallocatePixelData(uint8_t* ptr, int width, int height, int channels);

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

            // Raw allocation tracking
            size_t rawAllocated{0};       // Total raw bytes allocated
            size_t rawFreed{0};           // Total raw bytes freed
            size_t rawCurrentUsage{0};    // Current raw memory usage
            int rawAllocationCount{0};    // Number of raw allocation operations
            int rawDeallocationCount{0};  // Number of raw deallocation operations

            // Pixel data specific tracking
            size_t pixelDataAllocated{0}; // Total pixel data bytes allocated
            size_t pixelDataFreed{0};     // Total pixel data bytes freed
            size_t pixelDataCurrentUsage{0}; // Current pixel data usage
            int pixelDataAllocationCount{0}; // Number of pixel data allocations
            int pixelDataDeallocationCount{0}; // Number of pixel data deallocations
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

            // Create shared_ptr with lambda deleter
            return std::shared_ptr<T>(ptr, [this](T* p) {
                this->deallocate<T>(p);
            });
        }

        /**
         * @brief Create a unique_ptr using pool allocation
         * @tparam T Type of object to create
         * @tparam Args Constructor argument types
         * @param args Constructor arguments
         * @return unique_ptr to the newly created object
         */
        template<typename T, typename... Args>
        std::unique_ptr<T, std::function<void(T*)>> makeUnique(Args&&... args)
        {
            T* ptr = allocate<T>();
            if (!ptr) return nullptr;

            // Placement new with constructor arguments
            new(ptr) T(std::forward<Args>(args)...);

            // Create unique_ptr with lambda deleter
            auto deleter = [this](T* p) {
                this->deallocate<T>(p);
            };
            return std::unique_ptr<T, std::function<void(T*)>>(ptr, deleter);
        }

        /**
         * @brief Create a unique_ptr for pixel data with proper cleanup
         * @param width Width of the texture
         * @param height Height of the texture
         * @param channels Number of color channels
         * @return unique_ptr to allocated pixel data
         */
        std::unique_ptr<uint8_t[], std::function<void(uint8_t*)>> makePixelDataUnique(int width, int height, int channels);

    private:
        /**
         * @brief Internal method to create a memory pool without acquiring locks
         * @tparam T Type of objects to be allocated from the pool
         * @param initialCapacity Initial number of objects the pool can hold
         * @return Pointer to the created memory pool
         */
        template<typename T>
        MemoryPool* createPoolInternal(size_t initialCapacity = 64)
        {
            // Internal method that assumes mutex is already locked
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
         * @brief Find the best size pool for a given allocation size
         * @param size Requested allocation size
         * @return Pool size or 0 if no suitable pool exists
         */
        size_t findBestPoolSize(size_t size);

        /**
         * @brief Get or create a raw memory pool for a given size
         * @param poolSize Size of objects in the pool
         * @return Pointer to the memory pool
         */
        MemoryPool* getRawMemoryPool(size_t poolSize);

        std::unordered_map<std::type_index, std::unique_ptr<MemoryPool>> m_pools;
        std::unordered_map<size_t, std::unique_ptr<MemoryPool>> m_rawPools;
        std::vector<size_t> m_poolSizes;
        mutable std::mutex m_mutex;
        mutable std::mutex m_statsMutex;
        bool m_initialized;
        AllocationStats m_stats;

        // Update tracking
        float m_lastDefragmentTime;
        float m_defragmentInterval;
        bool m_enableDebugRendering;
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
        return Application::getInstance()->getSubsystem<MemoryManager>()->makeShared<T>(std::forward<Args>(args)...);
    }

    /**
     * @brief Convenience function to create a unique_ptr using the memory manager
     * @tparam T Type of object to create
     * @tparam Args Constructor argument types
     * @param args Constructor arguments
     * @return unique_ptr to the newly created object
     */
    template<typename T, typename... Args>
    std::unique_ptr<T, std::function<void(T*)>> makeUnique(Args&&... args)
    {
        return Application::getSubsystem<MemoryManager>()->makeUnique<T>(std::forward<Args>(args)...);
    }

} // namespace PixelCraft::Core