#include "MemoryManager.h"
#include <algorithm>
#include <cassert>
#include <iostream>

/**
 * @class MemoryPool
 * @brief Manages a contiguous block of memory for general purpose allocation
 */
class MemoryManager::MemoryPool
{
public:
    MemoryPool(size_t size)
        : m_memory(nullptr), m_size(size), m_used(0)
    {
        m_memory = static_cast<uint8_t*>(malloc(size));

        if (!m_memory)
        {
            throw std::bad_alloc();
        }

        // Initialize free blocks list with entire pool
        m_freeBlocks.emplace_back(0, size);
    }

    ~MemoryPool()
    {
        if (m_memory)
        {
            free(m_memory);
            m_memory = nullptr;
        }
    }

    void* allocate(size_t size, size_t alignment = 8)
    {
        // Find a free block that fits the requested size
        for (auto it = m_freeBlocks.begin(); it != m_freeBlocks.end(); it++)
        {
            size_t blockOffset = it->first;
            size_t blockSize = it->second;

            // Calculate alignment padding
            size_t padding = 0;
            size_t alignedOffset = blockOffset;

            if (alignedOffset % alignment != 0)
            {
                padding = alignment - (alignedOffset % alignment);
            }

            if (blockSize >= size + padding)
            {
                // Found a suitable block

                // Remove or resize the free block
                if (padding + size == blockSize)
                {
                    // Exact fit, remove the block
                    m_freeBlocks.erase(it);
                }
                else
                {
                    // Partial use, resize the block
                    it->first = blockOffset + padding + size;
                    it->second = blockSize - (padding + size);
                }

                // Update usage stats
                m_used += padding + size;

                // Return pointer to allocated memory
                return m_memory + blockOffset + padding;
            }
        }

        // No suitable block found
        return nullptr;
    }

    void deallocate(void* ptr)
    {
        if (!ptr || ptr < m_memory || ptr >= m_memory + m_size)
        {
            return; // Not part of this pool
        }

        size_t offset = static_cast<uint8_t*>(ptr) - m_memory;

        // Find the size of this allocation
        // In a real implementation, we would need to store allocation metadata
        // Here we'll have to scan the free blocks to find adjacent blocks

        // Find insertion point for the new free block
        auto it = m_freeBlocks.begin();
        while (it != m_freeBlocks.end() && it->first < offset)
        {
            it++;
        }

        // Calculate size based on the next free block
        size_t size;
        if (it != m_freeBlocks.end())
        {
            size = it->first - offset;
        }
        else
        {
            size = (m_memory + m_size) - (m_memory + offset);
        }

        // Update usage stats
        m_used -= size;

        // Insert new free block
        auto inserted = m_freeBlocks.emplace(it, offset, size);

        // Coalesce with previous block if contiguous
        if (inserted != m_freeBlocks.begin())
        {
            auto prev = std::prev(inserted);
            if (prev->first + prev->second == offset)
            {
                prev->second += size;
                m_freeBlocks.erase(inserted);
                inserted = prev;
            }
        }

        // Coalesce with next block if contiguous
        auto next = std::next(inserted);
        if (next != m_freeBlocks.end() && inserted->first + inserted->second == next->first)
        {
            inserted->second += next->second;
            m_freeBlocks.erase(next);
        }
    }

    void defragment()
    {
        // In a real implementation, defragmentation would involve
        // moving allocated memory blocks to eliminate gaps
        // This is complex and requires maintaining allocation metadata

        // Here we'll just coalesce adjacent free blocks as a basic operation
        if (m_freeBlocks.size() <= 1)
        {
            return; // Nothing to defragment
        }

        std::sort(m_freeBlocks.begin(), m_freeBlocks.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });

        for (auto it = m_freeBlocks.begin(); it != m_freeBlocks.end(); )
        {
            auto next = std::next(it);
            if (next != m_freeBlocks.end() && it->first + it->second == next->first)
            {
                // Merge blocks
                it->second += next->second;
                it = m_freeBlocks.erase(next);
            }
            else
            {
                it++;
            }
        }
    }

    size_t getSize() const
    {
        return m_size;
    }
    size_t getUsed() const
    {
        return m_used;
    }
    size_t getFree() const
    {
        return m_size - m_used;
    }

private:
    uint8_t* m_memory;
    size_t m_size;
    size_t m_used;
    std::vector<std::pair<size_t, size_t>> m_freeBlocks; // offset, size
};

/**
 * @class TypePool
 * @brief Specialized pool for objects of a specific type
 */
class MemoryManager::TypePool
{
public:
    TypePool(size_t objectSize, size_t initialCount, size_t growSize)
        : m_objectSize(objectSize), m_growSize(growSize), m_allocatedCount(0)
    {
        // Ensure minimum alignment
        m_objectSize = (objectSize + 7) & ~7;

        // Create initial pool
        expandPool(initialCount);
    }

    ~TypePool()
    {
        for (auto& chunk : m_chunks)
        {
            free(chunk.memory);
        }
    }

    void* allocate()
    {
        // Check free list first
        if (!m_freeList.empty())
        {
            void* ptr = m_freeList.back();
            m_freeList.pop_back();
            m_allocatedCount++;
            return ptr;
        }

        // Need to expand the pool
        expandPool(m_growSize);

        // Now we should have free blocks
        if (!m_freeList.empty())
        {
            void* ptr = m_freeList.back();
            m_freeList.pop_back();
            m_allocatedCount++;
            return ptr;
        }

        // Something went wrong
        return nullptr;
    }

    void deallocate(void* ptr)
    {
        // Validate pointer is from this pool
        bool valid = false;
        for (const auto& chunk : m_chunks)
        {
            if (ptr >= chunk.memory && ptr < static_cast<uint8_t*>(chunk.memory) + chunk.size)
            {
                valid = true;
                break;
            }
        }

        if (!valid)
        {
            return; // Not from this pool
        }

        // Add to free list
        m_freeList.push_back(ptr);
        m_allocatedCount--;
    }

    size_t getObjectSize() const
    {
        return m_objectSize;
    }
    size_t getAllocatedCount() const
    {
        return m_allocatedCount;
    }
    size_t getTotalCapacity() const
    {
        size_t total = 0;
        for (const auto& chunk : m_chunks)
        {
            total += chunk.count;
        }
        return total;
    }

    size_t getTotalReserved() const
    {
        size_t total = 0;
        for (const auto& chunk : m_chunks)
        {
            total += chunk.size;
        }
        return total;
    }

private:
    struct Chunk
    {
        void* memory;
        size_t size;
        size_t count;
    };

    void expandPool(size_t count)
    {
        if (count == 0)
        {
            return;
        }

        // Allocate new chunk
        size_t chunkSize = m_objectSize * count;
        void* memory = malloc(chunkSize);

        if (!memory)
        {
            throw std::bad_alloc();
        }

        // Add to chunks list
        m_chunks.push_back({memory, chunkSize, count});

        // Add objects to free list
        for (size_t i = 0; i < count; i++)
        {
            void* ptr = static_cast<uint8_t*>(memory) + (i * m_objectSize);
            m_freeList.push_back(ptr);
        }
    }

    size_t m_objectSize;
    size_t m_growSize;
    size_t m_allocatedCount;
    std::vector<Chunk> m_chunks;
    std::vector<void*> m_freeList;
};

// MemoryManager implementation
MemoryManager::MemoryManager()
    : m_memoryLimit(0)
{
    // Initialize statistics
    m_stats.totalAllocated = 0;
    m_stats.totalReserved = 0;
    m_stats.peakUsage = 0;
    m_stats.poolCount = 0;
    m_stats.activeAllocations = 0;
}

MemoryManager::~MemoryManager()
{
    // Type pools will be cleaned up by their destructors
    m_typePools.clear();

    // General pool will be cleaned up by its destructor
    m_generalPool.reset();

    // Check for memory leaks
    if (m_stats.activeAllocations > 0)
    {
        std::cerr << "WARNING: " << m_stats.activeAllocations
            << " allocations still active at MemoryManager destruction!" << std::endl;
    }
}

bool MemoryManager::initialize(size_t poolSize)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    try
    {
        // Create general memory pool
        m_generalPool = std::make_unique<MemoryPool>(poolSize);

        // Update stats
        m_stats.totalReserved = poolSize;
        m_stats.poolCount = 1;

        return true;
    }
    catch (const std::bad_alloc&)
    {
        std::cerr << "ERROR: Failed to allocate general memory pool of size "
            << poolSize << " bytes" << std::endl;
        return false;
    }
}

void MemoryManager::defragment(bool aggressive)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Defragment general pool
    if (m_generalPool)
    {
        m_generalPool->defragment();
    }

    if (aggressive)
    {
        // In aggressive mode, we could:
        // 1. Merge small type pools
        // 2. Compact partially filled pools
        // 3. Run a more thorough general pool defragmentation

        // For this implementation, we'll just release unused memory
        releaseUnused(true);
    }

    updateStats();
}

MemoryStats MemoryManager::getStats() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats;
}

void MemoryManager::releaseUnused(bool aggressive)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // For a real implementation, this would involve:
    // 1. Identifying pools with low utilization
    // 2. Potentially re-allocating and moving objects
    // 3. Freeing memory back to the system

    // For this simplified version, we'll just log what would happen
    size_t totalUnused = 0;

    if (m_generalPool)
    {
        totalUnused += m_generalPool->getFree();
    }

    for (const auto& [type, pool] : m_typePools)
    {
        size_t poolUnused = pool->getTotalReserved() -
            (pool->getAllocatedCount() * pool->getObjectSize());
        totalUnused += poolUnused;
    }

    std::cout << "Memory release would free " << totalUnused << " bytes" << std::endl;

    // In a full implementation, we'd actually release the memory here
    // For now, just update stats
    m_stats.totalReserved -= (aggressive ? totalUnused : totalUnused / 2);
    updateStats();
}

void MemoryManager::setMemoryLimit(size_t limitInBytes)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_memoryLimit = limitInBytes;
}

bool MemoryManager::isApproachingLimit(float thresholdPercent) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_memoryLimit == 0)
    {
        return false; // No limit set
    }

    float percentUsed = (static_cast<float>(m_stats.totalAllocated) / m_memoryLimit) * 100.0f;
    return percentUsed >= thresholdPercent;
}

void MemoryManager::updateStats()
{
    // Update total allocated
    size_t totalAllocated = 0;

    if (m_generalPool)
    {
        totalAllocated += m_generalPool->getUsed();
    }

    for (const auto& [type, pool] : m_typePools)
    {
        totalAllocated += pool->getAllocatedCount() * pool->getObjectSize();
    }

    m_stats.totalAllocated = totalAllocated;

    // Update peak usage
    if (totalAllocated > m_stats.peakUsage)
    {
        m_stats.peakUsage = totalAllocated;
    }
}

void* MemoryManager::allocateRaw(size_t size, std::type_index typeIndex)
{
    // Check if we have a type-specific pool
    auto it = m_typePools.find(typeIndex);
    if (it != m_typePools.end())
    {
        // Allocate from type pool
        return it->second->allocate();
    }

    // Otherwise, allocate from general pool
    if (m_generalPool)
    {
        return m_generalPool->allocate(size);
    }

    return nullptr;
}

void MemoryManager::deallocateRaw(void* ptr)
{
    if (!ptr)
    {
        return;
    }

    // Find which pool this pointer belongs to
    auto it = m_allocations.find(ptr);
    if (it != m_allocations.end())
    {
        std::type_index typeIndex = it->second;

        // Check if it's in a type pool
        auto poolIt = m_typePools.find(typeIndex);
        if (poolIt != m_typePools.end())
        {
            poolIt->second->deallocate(ptr);
            return;
        }
    }

    // Otherwise, try the general pool
    if (m_generalPool)
    {
        m_generalPool->deallocate(ptr);
    }
}