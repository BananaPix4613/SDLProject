#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <typeindex>
#include <string>

/**
 * @struct MemoryStats
 * @brief Contains statistics about memory usage in the engine
 */
struct MemoryStats
{
    size_t totalAllocated;       // Total bytes allocated
    size_t totalReserved;        // Total bytes reserved in pools
    size_t peakUsage;            // Peak memory usage
    size_t poolCount;            // Number of memory pools
    size_t activeAllocations;    // Number of active allocations

    std::unordered_map<std::string, size_t> typeAllocationMap; // Bytes per type
};

/**
 * @class MemoryManager
 * @brief Manager memory allocation and pooling for the game engine
 * 
 * Provides efficient memory allocation using pool-based strategies,
 * defragmentation capabilities, and detailed memory usage tracking.
 * Optimized for frequent creation and destruction of game objects,
 * components, and rendering resources.
*/
class MemoryManager
{
public:
    /**
     * @brief Constructor
     */
    MemoryManager();

    /**
     * @brief Destructor
     */
    ~MemoryManager();

    /**
     * @brief Initialize the memory manager with the specified pool size
     * @param poolSize Initial size of the general memory pool in bytes
     * @return True if initialization was successful
     */
    bool initialize(size_t poolSize);

    /**
     * @brief Allocate an object of type T from the appropriate memory pool
     * @tparam T Type of object to allocate
     * @tparam Args Constructor argument types
     * @param args Constructor arguments
     * @return Pointer to newly allocated object, or nullptr on failure
     */
    template<typename T, typename... Args>
    T* allocate(Args&&... args);

    /**
     * @brief Return an object to its memory pool
     * @tparam T Type of object to deallocate
     * @param ptr Pointer to the object
     */
    template<typename T>
    void deallocate(T* ptr);

    /**
     * @brief Defragment memory to reduce fragmentation and optimize layout
     * @param aggressive If true, performs a more thorough but slower defragmentation
     */
    void defragment(bool aggressive = false);

    /**
     * @brief Get statistics about memory usage
     * @return Current memory statistics
     */
    MemoryStats getStats() const;

    /**
     * @brief Register a type-specific memory pool
     * @tparam T Type to create a dedicated pool for
     * @param initialCount Initial number of objects to reserve
     * @param growSize Number of objects to add when pool is exhausted
     */
    template<typename T>
    void registerTypePool(size_t initialCount, size_t growSize);

    /**
     * @brief Release unused memory back to the system
     * @param aggressive If true, more aggressively releases memory at cost of future allocations
     */
    void releaseUnused(bool aggressive = false);

    /**
     * @brief Set a memory limit for the application
     * @param limitInBytes Maximum memory usage in bytes (0 for no limit)
     */
    void setMemoryLimit(size_t limitInBytes);

    /**
     * @brief Check if memory usage is approaching the set limit
     * @param thresholdPercent Percentage of limit to trigger warning (0-100)
     * @return True if usage exceeds the specified threshold percentage of limit
     */
    bool isApproachingLimit(float thresholdPercent = 90.0f) const;

private:
    // Forward declartion for implementation details
    class MemoryPool;
    class TypePool;

    // Memory pools - general and type-specific
    std::unique_ptr<MemoryPool> m_generalPool;
    std::unordered_map<std::type_index, std::unique_ptr<TypePool>> m_typePools;

    // Allocation tracking
    std::unordered_map<void*, std::type_index> m_allocations;

    // Statistics
    MemoryStats m_stats;
    size_t m_memoryLimit;

    // Thread safety
    mutable std::mutex m_mutex;

    // Internal methods
    void updateStats();
    void* allocateRaw(size_t size, std::type_index typeIndex);
    void deallocateRaw(void* ptr);

    // Helper classes implementations will be in the CPP file
};

// Template method implementations
#include "MemoryManager.inl"