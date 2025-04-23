// -------------------------------------------------------------------------
// MemoryPool.cpp
// -------------------------------------------------------------------------
#include "Core/MemoryPool.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <new>
#include "Core/Logger.h"

namespace PixelCraft::Core
{

    MemoryPool::MemoryPool(size_t objectSize, size_t initialCapacity)
        : m_objectSize(std::max(objectSize, sizeof(void*)))  // Ensure minimum size to store free list pointers
        , m_capacity(initialCapacity)
        , m_usedBlocks(0)
        , m_memory(nullptr)
        , m_initialized(false)
    {
        // Align object size to the platform's alignment requirements
        constexpr size_t alignment = alignof(std::max_align_t);
        m_objectSize = (m_objectSize + alignment - 1) & ~(alignment - 1);

        // Allocate the memory block
        try
        {
            m_memory = new unsigned char[m_objectSize * m_capacity];
        }
        catch (const std::bad_alloc&)
        {
            error("MemoryPool: Failed to allocate memory block of size " +
                  std::to_string(m_objectSize * m_capacity) + " bytes");
            m_capacity = 0;
            return;
        }

        // Initialize the free block list
        m_freeBlocks.reserve(m_capacity);
        for (size_t i = 0; i < m_capacity; i++)
        {
            m_freeBlocks.push_back(m_memory + (i * m_objectSize));
        }

        m_initialized = true;
        debug("MemoryPool: Created pool with object size " + std::to_string(m_objectSize) +
              " bytes and capacity " + std::to_string(m_capacity) + " objects");
    }

    MemoryPool::~MemoryPool()
    {
        if (m_memory)
        {
            // Report leak if blocks are still allocated when the pool is destroyed
            if (m_usedBlocks > 0)
            {
                warn("MemoryPool: Destroying pool with " + std::to_string(m_usedBlocks) +
                     " objects still allocated (memory leak)");
            }

            delete[] m_memory;
            m_memory = nullptr;
        }

        m_freeBlocks.clear();
        m_initialized = false;
    }

    void* MemoryPool::allocate()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_initialized)
        {
            error("MemoryPool: Attempted to allocate from uninitialized pool");
            return nullptr;
        }

        // If no free blocks, attempt to grow the pool
        if (m_freeBlocks.empty())
        {
            // Grow by 50% of current capacity (minimum 16 blocks)
            size_t growSize = std::max(m_capacity / 2, static_cast<size_t>(16));
            if (!grow(growSize))
            {
                error("MemoryPool: Failed to allocate; pool is full and cannot grow");
                return nullptr;
            }
        }

        // Get a block from the free list
        void* block = m_freeBlocks.back();
        m_freeBlocks.pop_back();

        // Zero out the memory block for safety
        std::memset(block, 0, m_objectSize);

        m_usedBlocks++;
        return block;
    }

    void MemoryPool::deallocate(void* ptr)
    {
        if (!ptr) return;

        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_initialized)
        {
            error("MemoryPool: Attempted to deallocate from unitialized pool");
            return;
        }

        // Validate the pointer
        if (!isValidPointer(ptr))
        {
            error("MemoryPool: Attempted to deallocate invalid pointer");
            return;
        }

        // Add the block back to the free list
        m_freeBlocks.push_back(ptr);
        m_usedBlocks--;
    }

    size_t MemoryPool::getUsedBlocks() const
    {
        return m_usedBlocks.load();
    }

    size_t MemoryPool::getFreeBlocks() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_freeBlocks.size();
    }

    size_t MemoryPool::getCapacity() const
    {
        return m_capacity;
    }

    size_t MemoryPool::getObjectSize() const
    {
        return m_objectSize;
    }

    void MemoryPool::reserve(size_t newCapacity)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (newCapacity <= m_capacity)
        {
            return; // Already have enough capacity
        }

        size_t additionalCapacity = newCapacity - m_capacity;
        grow(additionalCapacity);
    }

    void MemoryPool::defragment()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_initialized || m_freeBlocks.empty())
        {
            return;
        }

        // Sort free blocks by address for better cache locality
        std::sort(m_freeBlocks.begin(), m_freeBlocks.end());

        // Remove duplicates if any (shouldn't happen, but safety check)
        auto newEnd = std::unique(m_freeBlocks.begin(), m_freeBlocks.end());
        if (newEnd != m_freeBlocks.end())
        {
            warn("MemoryPool: Found duplicate free blocks during defragmentation");
            m_freeBlocks.erase(newEnd, m_freeBlocks.end());
        }

        debug("MemoryPool: Defragmented pool, organized " +
              std::to_string(m_freeBlocks.size()) + " free blocks");
    }

    bool MemoryPool::owns(void* ptr) const
    {
        return isValidPointer(ptr);
    }

    MemoryPool::PoolStats MemoryPool::getStats() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        PoolStats stats;
        stats.objectSize = m_objectSize;
        stats.capacity = m_capacity;
        stats.usedBlocks = m_usedBlocks;
        stats.freeBlocks = m_freeBlocks.size();
        stats.memoryUsage = m_objectSize * m_capacity;

        // Calculate fragmentation ratio
        if (m_freeBlocks.empty() || m_capacity == 0)
        {
            stats.fragmentationRatio = 0;
        }
        else
        {
            // Sort free blocks temporarily to check for contiguity
            std::vector<void*> sortedFreeBlocks = m_freeBlocks;
            std::sort(sortedFreeBlocks.begin(), sortedFreeBlocks.end());

            // Count "islands" of free blocks
            size_t islands = 1;
            for (size_t i = 1; i < sortedFreeBlocks.size(); i++)
            {
                unsigned char* prev = static_cast<unsigned char*>(sortedFreeBlocks[i - 1]);
                unsigned char* curr = static_cast<unsigned char*>(sortedFreeBlocks[i]);

                if (curr != prev + m_objectSize)
                {
                    islands++;
                }
            }

            // Fragmentation ratio: 0% = one contiguous block, 100% = maximally fragmented
            stats.fragmentationRatio = (islands == 1) ? 0 :
                static_cast<size_t>((static_cast<double>(islands) / sortedFreeBlocks.size()) * 100.0);
        }

        return stats;
    }

    bool MemoryPool::grow(size_t additionalCapacity)
    {
        if (additionalCapacity == 0) return true;

        // Allocate a new, larger block
        size_t newCapacity = m_capacity + additionalCapacity;
        size_t newSize = m_objectSize * newCapacity;

        unsigned char* newMemory = nullptr;
        try
        {
            newMemory = new unsigned char[newSize];
        }
        catch (const std::bad_alloc&)
        {
            error("MemoryPool: Failed to grow pool by " +
                  std::to_string(additionalCapacity) + " objects");
            return false;
        }

        // Copy existing memory to the new block
        if (m_memory)
        {
            std::memcpy(newMemory, m_memory, m_objectSize * m_capacity);
        }

        // Update free list with new blocks
        m_freeBlocks.reserve(m_freeBlocks.size() + additionalCapacity);
        for (size_t i = 0; i < additionalCapacity; i++)
        {
            m_freeBlocks.push_back(newMemory + ((m_capacity + i) * m_objectSize));
        }

        // Release old memory and update state
        if (m_memory)
        {
            delete[] m_memory;
        }

        m_memory = newMemory;
        m_capacity = newCapacity;

        debug("MemoryPool: Grew pool to " + std::to_string(m_capacity) +
              " objects (" + std::to_string(newSize) + " bytes)");

        return true;
    }

    bool MemoryPool::isValidPointer(void* ptr) const
    {
        if (!m_initialized || !m_memory || !ptr)
        {
            return false;
        }

        // Check if pointer is within the pool's memory range
        unsigned char* charPtr = static_cast<unsigned char*>(ptr);
        if (charPtr < m_memory || charPtr >= m_memory + (m_capacity * m_objectSize))
        {
            return false;
        }

        // Check alignment (must be at exact object boundary)
        size_t offset = charPtr - m_memory;
        return (offset % m_objectSize) == 0;
    }

} // namespace PixelCraft::Core