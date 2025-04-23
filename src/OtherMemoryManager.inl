// Template method implementations for MemoryManager

template<typename T, typename... Args>
T* MemoryManager::allocate(Args&&... args)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Allocate raw memory
    void* memory = allocateRaw(sizeof(T), std::type_index(typeid(T)));

    if (!memory)
    {
        return nullptr;
    }

    // Construct the object in-place
    T* object = new (memory) T(std::forward<Args>(args)...);

    // Update allocation tracking
    m_allocations[memory] = std::type_index(typeid(T));

    // Update statistics
    m_stats.activeAllocations++;

    // Record per-type allocation
    std::string typeName = typeid(T).name();
    m_stats.typeAllocationMap[typeName] += sizeof(T);

    return object;
}

template<typename T>
void MemoryManager::deallocate(T* ptr)
{
    if (!ptr)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // Call destructor
    ptr->~T();

    // Update statistics
    m_stats.activeAllocations--;

    // Update type statistics
    std::string typeName = typeid(T).name();
    m_stats.typeAllocationMap[typeName] -= sizeof(T);

    // Return memory to pool
    deallocateRaw(static_cast<void*>(ptr));

    // Remove from tracking
    m_allocations.erase(static_cast<void*>(ptr));
}

template<typename T>
void MemoryManager::registerTypePool(size_t initialCount, size_t growSize)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::type_index typeIndex(typeid(T));

    // Check if pool already exists
    if (m_typePools.find(typeIndex) != m_typePools.end())
    {
        return; // Already registered
    }

    // Create new type pool
    size_t objectSize = sizeof(T);
    m_typePools[typeIndex] = std::make_unique<TypePool>(objectSize, initialCount, growSize);

    // Update stats
    m_stats.poolCount++;
    m_stats.totalReserved += objectSize * initialCount;

    updateStats();
}