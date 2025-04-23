// -------------------------------------------------------------------------
// MemoryPool.h
// -------------------------------------------------------------------------
#pragma once

#include <cstddef>
#include <vector>
#include <mutex>
#include <atomic>
#include <memory>

namespace PixelCraft::Core
{

    /**
     * @brief Type-specific memory pool for efficient allocation and deallocation
     * 
     * Provides a contiguous block of memory for fixed-size objects with
     * minimal overhead. Supports defragmentation, reserving capacity, and
     * tracking allocation statistics.
     */
    class MemoryPool
    {
    public:
        /**
         * @brief Constructor
         * @param objectSize Size of each object in bytes
         * @param initialCapacity Initial number of objects the pool can hold
         */
        MemoryPool(size_t objectSize, size_t initialCapacity);

        /**
         * @brief Destructor
         */
        ~MemoryPool();

        /**
         * @brief Allocate a block of memory from the pool
         * @return Pointer to allocated memory or nullptr if allocation failed
         */
        void* allocate();

        /**
         * @brief Return a block of memory to the pool
         * @param ptr Pointer to memory previously allocated by this pool
         */
        void deallocate(void* ptr);

        /**
         * @brief Get the number of currently used blocks
         * @return Number of allocated blocks
         */
        size_t getUsedBlocks() const;

        /**
         * @brief Get the number of free blocks
         * @return Number of unallocated blocks
         */
        size_t getFreeBlocks() const;

        /**
         * @brief Get the total capacity of the pool
         * @return Total number of blocks (used + free)
         */
        size_t getCapacity() const;

        /**
         * @brief Get the size of each object in the pool
         * @return Size in bytes of each object
         */
        size_t getObjectSize() const;

        /**
         * @brief Ensure the pool has at least the specified capacity
         * @param newCapacity Minimum number of objects the pool should hold
         */
        void reserve(size_t newCapacity);

        /**
         * @brief Defragment the memory pool to consolidate free blocks
         * 
         * This operation may be expensive and should be used sparingly,
         * typically during loading screens or other non-performance-critical moments
         */
        void defragment();

        /**
         * @brief Check if a pointer belongs to this memory pool
         * @param ptr Pointer to check
         * @return True if the pointer was allocated from this pool
         */
        bool owns(void* ptr) const;

        /**
         * @brief Structure for tracking memory pool statistics
         */
        struct PoolStats
        {
            size_t objectSize;        // Size of each object in bytes
            size_t capacity;          // Total capacity in objects
            size_t usedBlocks;        // Number of allocated objects
            size_t freeBlocks;        // Number of available objects
            size_t memoryUsage;       // Total memory usage in bytes
            size_t fragmentationRatio;// Fragmentation percentage (0-100)
        };

        /**
         * @brief Get statistics about the memory pool
         * @return PoolStats structure with current statistics
         */
        PoolStats getStats() const;

    private:
        /**
         * @brief Grow the pool by allocating more memory
         * @param additionalCapacity Number of objects to add
         * @return True if growth was successful
         */
        bool grow(size_t additionalCapacity);

        /**
         * @brief Check if the pointer is aligned and within the pool's memory range
         * @param ptr Pointer to validate
         * @return True if the pointer is valid for this pool
         */
        bool isValidPointer(void* ptr) const;

        size_t m_objectSize;               // Size of each object in bytes
        size_t m_capacity;                 // Total numbers of objects the pool can hold
        std::atomic<size_t> m_usedBlocks;  // Number of currently allocated blocks

        unsigned char* m_memory;           // Raw memory block
        std::vector<void*> m_freeBlocks;   // Stack of free block pointers

        mutable std::mutex m_mutex;        // Mutex for thread safety
        bool m_initialized;                // Flag to track initialization status
    };

} // namespace PixelCraft::Core