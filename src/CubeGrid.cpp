#include "CubeGrid.h"

GridChunk::GridChunk(const glm::ivec3& position)
    : chunkPosition(position), active(false)
{
    // Initialize all cubes to inactive
    for (int x = 0; x < CHUNK_SIZE; x++)
    {
        for (int y = 0; y < CHUNK_SIZE; y++)
        {
            for (int z = 0; z < CHUNK_SIZE; z++)
            {
                cubes[x][y][z].active = false;
            }
        }
    }
}

void GridChunk::setCube(int localX, int localY, int localZ, const Cube& cube)
{
    if (localX >= 0 && localX < CHUNK_SIZE &&
        localY >= 0 && localY < CHUNK_SIZE &&
        localZ >= 0 && localZ < CHUNK_SIZE)
    {
        cubes[localX][localY][localZ] = cube;

        // Set chunk to active if any cube is active
        if (cube.active)
        {
            active = true;
        }
        else
        {
            // Check if any other cube is still active
            active = hasAnyCubes();
        }
    }
}

const Cube& GridChunk::getCube(int localX, int localY, int localZ) const
{
    static Cube defaultCube;
    if (localX >= 0 && localX < CHUNK_SIZE &&
        localY >= 0 && localY < CHUNK_SIZE &&
        localZ >= 0 && localZ < CHUNK_SIZE)
    {
        return cubes[localX][localY][localZ];
    }
    return defaultCube;
}

bool GridChunk::hasAnyCubes() const
{
    for (int x = 0; x < CHUNK_SIZE; x++)
    {
        for (int y = 0; y < CHUNK_SIZE; y++)
        {
            for (int z = 0; z < CHUNK_SIZE; z++)
            {
                if (cubes[x][y][z].active)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

std::vector<glm::ivec3> GridChunk::getAffectedChunks(const glm::vec3& worldPos, float radius) const
{
    // Convert world position to chunk-local coordinates
    glm::vec3 localPos = worldPos - glm::vec3(
        chunkPosition.x * CHUNK_SIZE,
        chunkPosition.y * CHUNK_SIZE,
        chunkPosition.z * CHUNK_SIZE
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
        result.push_back(chunkPosition);
    }

    // TODO: Add logic to check neighboring chunks if radius extends beyond this chunk

    return result;
}

CubeGrid::CubeGrid(int initialSize, float gridSpacing)
    : spacing(gridSpacing),
      minBounds(0, 0, 0),
      maxBounds(0, 0, 0)
{
    // Initialize with a single chunk at origin
    getOrCreateChunk(glm::ivec3(0, 0, 0));

    // Create initial floor (can be done more efficiently)
    for (int x = -initialSize / 2; x < initialSize / 2; x++)
    {
        for (int z = -initialSize / 2; z < initialSize / 2; z++)
        {
            glm::vec3 position = calculatePosition(x, 0, z);
            Cube cube(position, glm::vec3(0.9f, 0.9f, 0.9f));
            cube.active = true;
            setCube(x, 0, z, cube);
        }
    }

    // Set initial bounds based on the floor
    minBounds = glm::ivec3(-initialSize / 2, 0, -initialSize / 2);
    maxBounds = glm::ivec3(initialSize / 2, 0, initialSize / 2);
}

CubeGrid::~CubeGrid()
{
    clear();
}

void CubeGrid::clear()
{
    // Delete all chunks
    for (auto& pair : chunks)
    {
        delete pair.second;
    }
    chunks.clear();

    // Reset bounds
    minBounds = glm::ivec3(0, 0, 0);
    maxBounds = glm::ivec3(0, 0, 0);
}

void CubeGrid::update(float deltaTime)
{
    // Add any grid update logic here
    // This could include animations, cube creation/destruction, etc.
}

void CubeGrid::setCube(int x, int y, int z, const Cube& cube)
{
    // Calculate which chunk this belongs to
    glm::ivec3 chunkPos(
        std::floor(float(x) / GridChunk::CHUNK_SIZE),
        std::floor(float(y) / GridChunk::CHUNK_SIZE),
        std::floor(float(z) / GridChunk::CHUNK_SIZE)
    );

    // Get or create the chunk
    GridChunk* chunk = getOrCreateChunk(chunkPos);

    // Calculate local coordinates within the chunk
    int localX = x - (chunkPos.x * GridChunk::CHUNK_SIZE);
    int localY = y - (chunkPos.y * GridChunk::CHUNK_SIZE);
    int localZ = z - (chunkPos.z * GridChunk::CHUNK_SIZE);

    // Set the cube in the chunk
    chunk->setCube(localX, localY, localZ, cube);

    // Update grid bounds if this is an active cube
    if (cube.active)
    {
        expandBounds(glm::ivec3(x, y, z));
    }

    // Clean up chunks with no active cubes
    if (!cube.active && !chunk->hasAnyCubes())
    {
        chunks.erase(chunkPos);
        delete chunk;
    }
}

const Cube& CubeGrid::getCube(int x, int y, int z) const
{
    static Cube defaultCube;

    // Calculate which chunk this belongs to
    glm::ivec3 chunkPos(
        std::floor(float(x) / GridChunk::CHUNK_SIZE),
        std::floor(float(y) / GridChunk::CHUNK_SIZE),
        std::floor(float(z) / GridChunk::CHUNK_SIZE)
    );

    // Find the chunk
    auto it = chunks.find(chunkPos);
    if (it == chunks.end())
    {
        return defaultCube;
    }

    // Calculate local coordinates within the chunk
    int localX = x - (chunkPos.x * GridChunk::CHUNK_SIZE);
    int localY = y - (chunkPos.y * GridChunk::CHUNK_SIZE);
    int localZ = z - (chunkPos.z * GridChunk::CHUNK_SIZE);

    return it->second->getCube(localX, localY, localZ);
}

bool CubeGrid::isCubeActive(int x, int y, int z) const
{
    return getCube(x, y, z).active;
}

GridChunk* CubeGrid::getOrCreateChunk(const glm::ivec3& chunkPos)
{
    auto it = chunks.find(chunkPos);
    if (it != chunks.end())
    {
        return it->second;
    }

    // Create new chunk
    GridChunk* newChunk = new GridChunk(chunkPos);
    chunks[chunkPos] = newChunk;
    return newChunk;
}

void CubeGrid::updateLoadedChunks(const glm::ivec3& centerGridPos, int viewDistance)
{
    // Convert grid position to chunk position
    glm::ivec3 centerChunkPos(
        std::floor(float(centerGridPos.x) / GridChunk::CHUNK_SIZE),
        std::floor(float(centerGridPos.y) / GridChunk::CHUNK_SIZE),
        std::floor(float(centerGridPos.z) / GridChunk::CHUNK_SIZE)
    );

    // Mark all chunks as unvisited
    std::unordered_set<glm::ivec3, Vec3Hash> visitedChunks;

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
                    auto it = chunks.find(chunkPos);
                    if (it != chunks.end())
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

glm::vec3 CubeGrid::calculatePosition(int x, int y, int z) const
{
    return glm::vec3(
        x * spacing,
        y * spacing,
        z * spacing
    );
}

glm::ivec3 CubeGrid::worldToGridCoordinates(const glm::vec3& worldPos) const
{
    return glm::ivec3(
        static_cast<int>(std::floor(worldPos.x / spacing)),
        static_cast<int>(std::floor(worldPos.y / spacing)),
        static_cast<int>(std::floor(worldPos.z / spacing))
    );
}

void CubeGrid::expandBounds(const glm::ivec3& pos)
{
    minBounds.x = std::min(minBounds.x, pos.x);
    minBounds.y = std::min(minBounds.y, pos.y);
    minBounds.z = std::min(minBounds.z, pos.z);

    maxBounds.x = std::max(maxBounds.x, pos.x);
    maxBounds.y = std::max(maxBounds.y, pos.y);
    maxBounds.z = std::max(maxBounds.z, pos.z);
}

int CubeGrid::getActiveChunkCount() const
{
    int count = 0;

    for (const auto& pair : chunks)
    {
        GridChunk* chunk = pair.second;

        if (chunk->hasAnyCubes())
        {
            count++;
        }
    }

    return count;
}

int CubeGrid::getTotalActiveCubeCount() const
{
    int totalActiveCubes = 0;

    for (const auto& pair : chunks)
    {
        GridChunk* chunk = pair.second;

        for (int x = 0; x < GridChunk::CHUNK_SIZE; x++)
        {
            for (int y = 0; y < GridChunk::CHUNK_SIZE; y++)
            {
                for (int z = 0; z < GridChunk::CHUNK_SIZE; z++)
                {
                    if (chunk->getCube(x, y, z).active)
                    {
                        totalActiveCubes++;
                    }
                }
            }
        }
    }

    return totalActiveCubes;
}
