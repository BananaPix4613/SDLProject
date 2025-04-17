// Implementation for template class

// GridChunk implementation
template <typename T>
GridChunk<T>::GridChunk(const glm::ivec3& position)
    : m_chunkPosition(position), m_active(false)
{
    // Initialize with empty cells
    m_cells.resize(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE);
}

template <typename T>
void GridChunk<T>::setCell(int localX, int localY, int localZ, const T& value)
{
    if (localX >= 0 && localX < CHUNK_SIZE &&
        localY >= 0 && localY < CHUNK_SIZE &&
        localZ >= 0 && localZ < CHUNK_SIZE)
    {
        int index = coordsToIndex(localX, localY, localZ);
        m_cells[index] = value;

        // Update active state based on cell activity
        m_active = hasAnyCells();
    }
}

template <typename T>
const T& GridChunk<T>::getCell(int localX, int localY, int localZ) const
{
    static T defaultValue;
    if (localX >= 0 && localX < CHUNK_SIZE &&
        localY >= 0 && localY < CHUNK_SIZE &&
        localZ >= 0 && localZ < CHUNK_SIZE)
    {
        int index = coordsToIndex(localX, localY, localZ);
        return m_cells[index];
    }
    return defaultValue;
}

template <typename T>
bool GridChunk<T>::isCellActive(int localX, int localY, int localZ) const
{
    // Base implementation - override in specialized chunks
    return false;
}

template <typename T>
bool GridChunk<T>::hasAnyCells() const
{
    for (int x = 0; x < CHUNK_SIZE; x++)
    {
        for (int y = 0; y < CHUNK_SIZE; y++)
        {
            for (int z = 0; z < CHUNK_SIZE; z++)
            {
                if (isCellActive(x, y, z))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

template <typename T>
std::vector<glm::ivec3> GridChunk<T>::getAffectedChunks(const glm::vec3& worldPos, float radius) const
{
    // Convert world position to chunk-local coordinates
    glm::vec3 localPos = worldPos - glm::vec3(
        m_chunkPosition.x * CHUNK_SIZE,
        m_chunkPosition.y * CHUNK_SIZE,
        m_chunkPosition.z * CHUNK_SIZE
    );

    std::vector<glm::ivec3> result;

    // Check if this chunk is affected
    glm::vec3 chunkMin(0.0f);
    glm::vec3 chunkMax(CHUNK_SIZE - 1);

    // Simple AABB test with radius
    if (localPos.x + radius >= chunkMin.x && localPos.x - radius <= chunkMax.x &&
        localPos.y + radius >= chunkMin.y && localPos.y - radius <= chunkMax.y &&
        localPos.z + radius >= chunkMin.z && localPos.z - radius <= chunkMax.z)
    {
        result.push_back(m_chunkPosition);
    }

    // Check neighboring chunks if radius extends beyond this chunk
    // TODO: Implement proper chunk neighborhood detection

    return result;
}

template <typename T>
int GridChunk<T>::coordsToIndex(int x, int y, int z) const
{
    return (z * CHUNK_SIZE * CHUNK_SIZE) + (y * CHUNK_SIZE) + x;
}

// Grid implementation
template <typename T>
Grid<T>::Grid(float gridSpacing) :
    m_spacing(gridSpacing),
    m_minBounds(0, 0, 0),
    m_maxBounds(0, 0, 0)
{
    // Initialize with a single chunk at origin
    getOrCreateChunk(glm::ivec3(0, 0, 0));
}

template <typename T>
Grid<T>::~Grid()
{
    clear();
}

template <typename T>
void Grid<T>::clear()
{
    // Delete all chunks
    for (auto& pair : m_chunks)
    {
        delete pair.second;
    }
    m_chunks.clear();
    
    // Reset bounds
    m_minBounds = glm::ivec3(0, 0, 0);
    m_maxBounds = glm::ivec3(0, 0, 0);
}

template <typename T>
void Grid<T>::update(float deltaTime)
{
    // Base implementation - override in specialized grids
}

template <typename T>
void Grid<T>::setCell(int x, int y, int z, const T& value)
{
    // Calculate which chunk this belongs to
    glm::ivec3 chunkPos(
        std::floor(float(x) / GridChunk<T>::CHUNK_SIZE),
        std::floor(float(y) / GridChunk<T>::CHUNK_SIZE),
        std::floor(float(z) / GridChunk<T>::CHUNK_SIZE)
    );

    // Get or create the chunk
    GridChunk<T>* chunk = getOrCreateChunk(chunkPos);

    // Calculate local coordinates within the chunk
    int localX = x - (chunkPos.x * GridChunk<T>::CHUNK_SIZE);
    int localY = y - (chunkPos.y * GridChunk<T>::CHUNK_SIZE);
    int localZ = z - (chunkPos.z * GridChunk<T>::CHUNK_SIZE);

    // Set the cell in the chunk
    chunk->setCell(localX, localY, localZ, value);

    // Update grid bounds
    expandBounds(glm::ivec3(x, y, z));

    // Clean up chunks with no active cells
    if (!chunk->hasAnyCells())
    {
        unloadChunk(chunkPos);
    }
}

template <typename T>
const T& Grid<T>::getCell(int x, int y, int z) const
{
    // Calculate which chunk this belongs to
    glm::ivec3 chunkPos(
        std::floor(float(x) / GridChunk<T>::CHUNK_SIZE),
        std::floor(float(y) / GridChunk<T>::CHUNK_SIZE),
        std::floor(float(z) / GridChunk<T>::CHUNK_SIZE)
    );

    // Find the chunk
    auto it = m_chunks.find(chunkPos);
    if (it == m_chunks.end())
    {
        return defaultValue;
    }

    // Calculate local coordinates within the chunk
    int localX = x - (chunkPos.x * GridChunk<T>::CHUNK_SIZE);
    int localY = y - (chunkPos.y * GridChunk<T>::CHUNK_SIZE);
    int localZ = z - (chunkPos.z * GridChunk<T>::CHUNK_SIZE);

    return it->second->getCell(localX, localY, localZ);
}

template <typename T>
bool Grid<T>::isCellActive(int x, int y, int z) const
{
    // Calculate which chunk this belongs to
    glm::ivec3 chunkPos(
        std::floor(float(x) / GridChunk<T>::CHUNK_SIZE),
        std::floor(float(y) / GridChunk<T>::CHUNK_SIZE),
        std::floor(float(z) / GridChunk<T>::CHUNK_SIZE)
    );

    // Find the chunk
    auto it = m_chunks.find(chunkPos);
    if (it == m_chunks.end())
    {
        return false;
    }

    // Calculate local coordinates within the chunk
    int localX = x - (chunkPos.x * GridChunk<T>::CHUNK_SIZE);
    int localY = y - (chunkPos.y * GridChunk<T>::CHUNK_SIZE);
    int localZ = z - (chunkPos.z * GridChunk<T>::CHUNK_SIZE);

    return it->second->isCellActive(localX, localY, localZ);
}

template <typename T>
GridChunk<T>* Grid<T>::getOrCreateChunk(const glm::ivec3& chunkPos)
{
    auto it = m_chunks.find(chunkPos);
    if (it != m_chunks.end())
    {
        return it->second;
    }

    // Create new chunk
    GridChunk<T>* newChunk = createChunk(chunkPos);
    m_chunks[chunkPos] = newChunk;
    return newChunk;
}

template <typename T>
GridChunk<T>* Grid<T>::createChunk(const glm::ivec3& position)
{
    return new GridChunk<T>(position);
}

template <typename T>
void Grid<T>::updateLoadedChunks(const glm::ivec3& centerGridPos, int viewDistance)
{
    // Convert grid position to chunk position
    glm::ivec3 centerChunkPos(
        std::floor(float(centerGridPos.x) / GridChunk<T>::CHUNK_SIZE),
        std::floor(float(centerGridPos.y) / GridChunk<T>::CHUNK_SIZE),
        std::floor(float(centerGridPos.z) / GridChunk<T>::CHUNK_SIZE)
    );

    // Mark all chunks as unvisited
    std::unordered_set<glm::ivec3, Vec3Hash, Vec3Equal> visitedChunks;

    // Load chunks in view distance
    for (int x = -viewDistance; x <= viewDistance; x++)
    {
        for (int y = -viewDistance; y <= viewDistance; y++)
        {
            for (int z = -viewDistance; z <= viewDistance; z++)
            {
                glm::ivec3 chunkPos = centerChunkPos + glm::ivec3(x, y, z);

                // Use distance check for circular loading
                float distSq = x * x + y * y + z * z;
                if (distSq <= viewDistance * viewDistance)
                {
                    // Ensure the chunk exists (but don't create if empty)
                    auto it = m_chunks.find(chunkPos);
                    if (it != m_chunks.end())
                    {
                        visitedChunks.insert(chunkPos);
                    }
                    else if (x == 0 && y == 0 && z == 0)
                    {
                        // Always create the center chunk
                        getOrCreateChunk(chunkPos);
                        visitedChunks.insert(chunkPos);
                    }
                }
            }
        }
    }

    // Optional: Unload chunks outside view distance
    // This would require additional logic to save/serialize unloaded chunks
}

template <typename T>
bool Grid<T>::unloadChunk(const glm::ivec3& chunkPos)
{
    auto it = m_chunks.find(chunkPos);
    if (it != m_chunks.end())
    {
        delete it->second;
        m_chunks.erase(it);
        return true;
    }
    return false;
}

template <typename T>
glm::vec3 Grid<T>::gridToWorldPosition(int x, int y, int z) const
{
    return glm::vec3(
        x * m_spacing,
        y * m_spacing,
        z * m_spacing
    );
}

template <typename T>
glm::ivec3 Grid<T>::worldToGridCoordinates(const glm::vec3& worldPos) const
{
    return glm::ivec3(
        static_cast<int>(std::floor(worldPos.x / m_spacing)),
        static_cast<int>(std::floor(worldPos.y / m_spacing)),
        static_cast<int>(std::floor(worldPos.z / m_spacing))
    );
}

template <typename T>
void Grid<T>::expandBounds(const glm::ivec3& pos)
{
    m_minBounds.x = std::min(m_minBounds.x, pos.x);
    m_minBounds.y = std::min(m_minBounds.y, pos.y);
    m_minBounds.z = std::min(m_minBounds.z, pos.z);

    m_maxBounds.x = std::max(m_maxBounds.x, pos.x);
    m_maxBounds.y = std::max(m_maxBounds.y, pos.y);
    m_maxBounds.z = std::max(m_maxBounds.z, pos.z);
}

template <typename T>
int Grid<T>::getActiveChunkCount() const
{
    int count = 0;
    for (const auto& pair : m_chunks)
    {
        if (pair.second->isActive())
        {
            count++;
        }
    }
    return count;
}

template <typename T>
int Grid<T>::getTotalActiveCellCount() const
{
    int count = 0;
    for (const auto& pair : m_chunks)
    {
        GridChunk<T>* chunk = pair.second;
        for (int x = 0; x < GridChunk<T>::CHUNK_SIZE; x++)
        {
            for (int y = 0; y < GridChunk<T>::CHUNK_SIZE; y++)
            {
                for (int z = 0; z < GridChunk<T>::CHUNK_SIZE; z++)
                {
                    if (chunk->isCellActive(x, y, z))
                    {
                        count++;
                    }
                }
            }
        }
    }
    return count;
}

template <typename T>
std::vector<std::tuple<glm::ivec3, T>> Grid<T>::querySphere(const glm::vec3& center, float radius) const
{
    std::vector<std::tuple<glm::ivec3, T>> result;

    // Convert to grid coordinates
    glm::ivec3 gridCenter = worldToGridCoordinates(center);
    int gridRadius = static_cast<int>(std::ceil(radius / m_spacing));

    // Define the box bounds for initial broad check
    glm::ivec3 minCoord = gridCenter - glm::ivec3(gridRadius);
    glm::ivec3 maxCoord = gridCenter + glm::ivec3(gridRadius);

    // Iterate through potential cells
    for (int x = minCoord.x; x <= maxCoord.x; x++)
    {
        for (int y = minCoord.y; y <= maxCoord.y; y++)
        {
            for (int z = minCoord.z; z <= maxCoord.z; z++)
            {
                glm::ivec3 cellPos(x, y, z);
                glm::vec3 worldPos = gridToWorldPosition(x, y, z);

                // Check distance from center to cell center
                float distSq = glm::distance2(center, worldPos);
                if (distSq <= radius * radius && isCellActive(x, y, z))
                {
                    result.push_back(std::make_tuple(cellPos, getCell(x, y, z)));
                }
            }
        }
    }

    return result;
}

template <typename T>
std::vector<std::tuple<glm::ivec3, T>> Grid<T>::queryBox(const glm::vec3& min, const glm::vec3& max) const
{
    std::vector<std::tuple<glm::ivec3, T>> result;

    // Convert to grid coordinates
    glm::ivec3 minCoord = worldToGridCoordinates(min);
    glm::ivec3 maxCoord = worldToGridCoordinates(max);

    // Iterate through the box
    for (int x = minCoord.x; x <= maxCoord.x; x++)
    {
        for (int y = minCoord.y; y <= maxCoord.y; y++)
        {
            for (int z = minCoord.z; z <= maxCoord.z; z++)
            {
                if (isCellActive(x, y, z))
                {
                    glm::ivec3 cellPos(x, y, z);
                    result.push_back(std::make_tuple(cellPos, getCell(x, y, z)));
                }
            }
        }
    }
    
    return result;
}

template <typename T>
void Grid<T>::forEachCell(const std::function<void(const glm::ivec3&, const T&)>& func) const
{
    for (const auto& pair : m_chunks)
    {
        GridChunk<T>* chunk = pair.second;
        glm::ivec3 chunkPos = chunk->getPosition();

        for (int x = 0; x < GridChunk<T>::CHUNK_SIZE; x++)
        {
            for (int y = 0; y < GridChunk<T>::CHUNK_SIZE; y++)
            {
                for (int z = 0; z < GridChunk<T>::CHUNK_SIZE; z++)
                {
                    // Calculate global coordinates
                    glm::ivec3 cellPos(
                        chunkPos.x * GridChunk<T>::CHUNK_SIZE + x,
                        chunkPos.y * GridChunk<T>::CHUNK_SIZE + y,
                        chunkPos.z * GridChunk<T>::CHUNK_SIZE + z
                    );

                    func(cellPos, chunk->getCell(x, y, z));
                }
            }
        }
    }
}

template <typename T>
void Grid<T>::forEachActiveCell(const std::function<void(const glm::ivec3&, const T&)>& func) const
{
    for (const auto& pair : m_chunks)
    {
        GridChunk<T>* chunk = pair.second;
        glm::ivec3 chunkPos = chunk->getPosition();

        for (int x = 0; x < GridChunk<T>::CHUNK_SIZE; x++)
        {
            for (int y = 0; y < GridChunk<T>::CHUNK_SIZE; y++)
            {
                for (int z = 0; z < GridChunk<T>::CHUNK_SIZE; z++)
                {
                    if (chunk->isCellActive(x, y, z))
                    {
                        // Calculate global coordinates
                        glm::ivec3 cellPos(
                            chunkPos.x * GridChunk<T>::CHUNK_SIZE + x,
                            chunkPos.y * GridChunk<T>::CHUNK_SIZE + y,
                            chunkPos.z * GridChunk<T>::CHUNK_SIZE + z
                        );

                        func(cellPos, chunk->getCell(x, y, z));
                    }
                }
            }
        }
    }
}