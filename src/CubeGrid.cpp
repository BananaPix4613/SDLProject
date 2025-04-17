#include "CubeGrid.h"

// CubeChunk Implementation
CubeChunk::CubeChunk(const glm::ivec3& position)
    : GridChunk<Cube>(position)
{
    // Initialize with empty cubes
}

bool CubeChunk::isCellActive(int localX, int localY, int localZ) const
{
    if (localX >= 0 && localX < CHUNK_SIZE &&
        localY >= 0 && localY < CHUNK_SIZE &&
        localZ >= 0 && localZ < CHUNK_SIZE)
    {
        int index = coordsToIndex(localX, localY, localZ);
        return m_cells[index].active;
    }
    return false;
}

// CubeGrid Implementation
CubeGrid::CubeGrid(int initialSize, float gridSpacing)
    : Grid<Cube>(gridSpacing)
{
    createInitialFloor(initialSize);
}

CubeGrid::~CubeGrid()
{
    // Base class destructor will handle cleanup
}

void CubeGrid::setCube(int x, int y, int z, const Cube& cube)
{
    // Use base class implementation
    setCell(x, y, z, cube);
}

const Cube& CubeGrid::getCube(int x, int y, int z) const
{
    // Use baes class implementation
    return getCell(x, y, z);
}

bool CubeGrid::isCubeActive(int x, int y, int z) const
{
    // Use base class implementation
    return isCellActive(x, y, z);
}

std::vector<Cube> CubeGrid::getVisibleCubesInFrustum(const glm::mat4& viewProj) const
{
    std::vector<Cube> visibleCubes;

    // Simple implementation for now, can be optimized with actual frustum culling
    forEachActiveCell([&](const glm::ivec3& pos, const Cube& cube) {
        glm::vec3 worldPos = gridToWorldPosition(pos.x, pos.y, pos.z);
        glm::vec4 clipPos = viewProj * glm::vec4(worldPos, 1.0f);

        // Simple frustum check (can be improved)
        if (clipPos.w > 0 &&
            clipPos.x >= -clipPos.w && clipPos.x <= clipPos.w &&
            clipPos.y >= -clipPos.w && clipPos.y <= clipPos.w &&
            clipPos.z >= -clipPos.w && clipPos.z <= clipPos.w)
        {
            visibleCubes.push_back(cube);
        }
                      });

    return visibleCubes;
}

std::vector<std::tuple<glm::ivec3, Cube>> CubeGrid::querySphere(const glm::vec3& center, float radius) const
{
    // Specialized implementation that optimizes for Cube properties
    std::vector<std::tuple<glm::ivec3, Cube>> result;

    // Convert to grid coordinates
    glm::ivec3 gridCenter = worldToGridCoordinates(center);
    int gridRadius = static_cast<int>(std::ceil(radius / m_spacing));

    // Define the box bounds for initial broad check
    glm::ivec3 minCoord = gridCenter - glm::ivec3(gridRadius);
    glm::ivec3 maxCoord = gridCenter + glm::ivec3(gridRadius);

    // Pre-compute squared radius for faster distance checks
    float radiusSq = radius * radius;

    // Gather potentially affected chunks first (optimization)
    std::unordered_set<glm::ivec3, Vec3Hash, Vec3Equal> affectedChunks;

    glm::ivec3 minChunk(
        std::floor(float(minCoord.x) / GridChunk<Cube>::CHUNK_SIZE),
        std::floor(float(minCoord.y) / GridChunk<Cube>::CHUNK_SIZE),
        std::floor(float(minCoord.z) / GridChunk<Cube>::CHUNK_SIZE)
    );

    glm::ivec3 maxChunk(
        std::floor(float(maxCoord.x) / GridChunk<Cube>::CHUNK_SIZE),
        std::floor(float(maxCoord.y) / GridChunk<Cube>::CHUNK_SIZE),
        std::floor(float(maxCoord.z) / GridChunk<Cube>::CHUNK_SIZE)
    );

    // Iterate through chunks in the query area
    for (int cx = minChunk.x; cx <= maxChunk.x; cx++)
    {
        for (int cy = minChunk.y; cy <= maxChunk.y; cy++)
        {
            for (int cz = minChunk.z; cz <= maxChunk.z; cz++)
            {
                glm::ivec3 chunkPos(cx, cy, cz);
                auto it = m_chunks.find(chunkPos);
                if (it != m_chunks.end() && it->second->isActive())
                {
                    // Calculate chunk bounds in world space
                    glm::vec3 chunkMin = gridToWorldPosition(
                        cx * GridChunk<Cube>::CHUNK_SIZE,
                        cy * GridChunk<Cube>::CHUNK_SIZE,
                        cz * GridChunk<Cube>::CHUNK_SIZE
                    );
                    glm::vec3 chunkMax = gridToWorldPosition(
                        (cx + 1) * GridChunk<Cube>::CHUNK_SIZE - 1,
                        (cy + 1) * GridChunk<Cube>::CHUNK_SIZE - 1,
                        (cz + 1) * GridChunk<Cube>::CHUNK_SIZE - 1
                    );

                    // Check if chunk intersects with sphere
                    // Simple AABB vs Sphere test
                    float distSq = 0.0f;

                    // Check each axis for closest point
                    for (int i = 0; i < 3; i++)
                    {
                        float v = center[i];
                        if (v < chunkMin[i]) distSq += (chunkMin[i] - v) * (chunkMin[i] - v);
                        else if (v > chunkMax[i]) distSq += (v - chunkMax[i]) * (v - chunkMax[i]);
                    }

                    // If chunk is within radius, check cells inside the chunk
                    if (distSq <= radiusSq)
                    {
                        GridChunk<Cube>* chunk = it->second;

                        // Check individual cells in the chunk
                        for (int lx = 0; lx < GridChunk<Cube>::CHUNK_SIZE; lx++)
                        {
                            for (int ly = 0; ly < GridChunk<Cube>::CHUNK_SIZE; ly++)
                            {
                                for (int lz = 0; lz < GridChunk<Cube>::CHUNK_SIZE; lz++)
                                {
                                    if (chunk->isCellActive(lx, ly, lz))
                                    {
                                        // Calculate global position
                                        int gx = cx * GridChunk<Cube>::CHUNK_SIZE + lx;
                                        int gy = cy * GridChunk<Cube>::CHUNK_SIZE + ly;
                                        int gz = cz * GridChunk<Cube>::CHUNK_SIZE + lz;

                                        // Get world position
                                        glm::vec3 worldPos = gridToWorldPosition(gx, gy, gz);

                                        // Check distance from center to cell
                                        float cellDistSq = glm::distance2(center, worldPos);
                                        if (cellDistSq <= radiusSq)
                                        {
                                            result.push_back(std::make_tuple(
                                                glm::ivec3(gx, gy, gz),
                                                chunk->getCell(lx, ly, lz)
                                            ));
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return result;
}

void CubeGrid::createInitialFloor(int size)
{
    // Create initial floor
    for (int x = -size / 2; x < size / 2; x++)
    {
        for (int z = -size / 2; z < size / 2; z++)
        {
            glm::vec3 position = gridToWorldPosition(x, 0, z);
            Cube cube(position, glm::vec3(0.9f, 0.9f, 0.9f));
            cube.active = true;
            setCube(x, 0, z, cube);
        }
    }

    // Set initial bounds based on the floor
    m_minBounds = glm::ivec3(-size / 2, 0, -size / 2);
    m_maxBounds = glm::ivec3(size / 2, 0, size / 2);
}

GridChunk<Cube>* CubeGrid::createChunk(const glm::ivec3& position)
{
    // Create the specialized chunk type
    return new CubeChunk(position);
}