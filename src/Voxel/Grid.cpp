// -------------------------------------------------------------------------
// Grid.cpp
// -------------------------------------------------------------------------
#include "Voxel/Grid.h"
#include "Core/Logger.h"
#include "Core/Application.h"
#include "Utility/Profiler.h"
#include "Utility/Ray.h"

#include <algorithm>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Voxel
{

    Grid::Grid(int chunkSize)
        : m_chunkSize(chunkSize)
        , m_defaultVoxel(0, 0)
        , m_initialized(false)
    {
        // Initialize bounds to a large volume
        m_bounds = Utility::AABB(
            glm::vec3(-1000.0f, -1000.0f, -1000.0f),
            glm::vec3(1000.0f, 1000.0f, 1000.0f)
        );
    }

    Grid::~Grid()
    {
        if (m_initialized)
        {
            shutdown();
        }
    }

    bool Grid::initialize()
    {
        if (m_initialized)
        {
            Log::warn("Grid already initialized");
            return true;
        }

        Log::info("Initializing voxel grid with chunk size: " + m_chunkSize);

        // Get the ChunkManager from the application
        auto app = Core::Application::getInstance();
        m_chunkManager = app->getSubsystem<ChunkManager>();

        if (!m_chunkManager)
        {
            Log::error("Failed to initialize Grid: ChunkManager not found");
            return false;
        }

        m_initialized = true;
        return true;
    }

    void Grid::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        Log::info("Shutting down voxel grid");

        // Release reference to chunks
        m_chunkManager.reset();

        m_initialized = false;
    }

    Voxel Grid::getVoxel(const glm::vec3& worldPosition) const
    {
        if (!m_initialized)
        {
            return m_defaultVoxel;
        }

        // Convert world position to chunk and local coordinates
        ChunkCoord chunkCoord;
        int localX, localY, localZ;
        worldToChunkAndLocal(worldPosition, chunkCoord, localX, localY, localZ);

        // Get the chunk
        auto chunk = getChunk(chunkCoord);
        if (!chunk)
        {
            return m_defaultVoxel;
        }

        // Get the voxel from the chunk
        return chunk->getVoxel(localX, localY, localZ);
    }

    Voxel Grid::getVoxel(int x, int y, int z) const
    {
        if (!m_initialized)
        {
            return m_defaultVoxel;
        }

        // Convert grid coordinates to chunk and local coordinates
        ChunkCoord chunkCoord;
        int localX, localY, localZ;
        gridToChunkAndLocal(x, y, z, chunkCoord, localX, localY, localZ);

        // Get the chunk
        auto chunk = getChunk(chunkCoord);
        if (!chunk)
        {
            return m_defaultVoxel;
        }

        // Get the voxel from the chunk
        return chunk->getVoxel(localX, localY, localZ);
    }

    bool Grid::setVoxel(const glm::vec3& worldPosition, const Voxel& voxel)
    {
        if (!m_initialized)
        {
            return false;
        }

        // Check if the position is within the grid bounds
        if (!m_bounds.contains(worldPosition))
        {
            return false;
        }

        // Convert world position to chunk and local coordinates
        ChunkCoord chunkCoord;
        int localX, localY, localZ;
        worldToChunkAndLocal(worldPosition, chunkCoord, localX, localY, localZ);

        // Get or create the chunk
        auto chunk = getChunk(chunkCoord);
        if (!chunk)
        {
            chunk = createChunk(chunkCoord);
            if (!chunk)
            {
                return false;
            }
        }

        // Set the voxel in the chunk
        return chunk->setVoxel(localX, localY, localZ, voxel);
    }

    bool Grid::setVoxel(int x, int y, int z, const Voxel& voxel)
    {
        if (!m_initialized)
        {
            return false;
        }

        // Convert grid position to world position to check bounds
        glm::vec3 worldPos(
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z)
        );

        // Check if the position is within the grid bounds
        if (!m_bounds.contains(worldPos))
        {
            return false;
        }

        // Convert grid coordinates to chunk and local coordinates
        ChunkCoord chunkCoord;
        int localX, localY, localZ;
        gridToChunkAndLocal(x, y, z, chunkCoord, localX, localY, localZ);

        // Get or create the chunk
        auto chunk = getChunk(chunkCoord);
        if (!chunk)
        {
            chunk = createChunk(chunkCoord);
            if (!chunk)
            {
                return false;
            }
        }

        // Set the voxel in the chunk
        return chunk->setVoxel(localX, localY, localZ, voxel);
    }

    std::shared_ptr<Chunk> Grid::getChunk(const ChunkCoord& coord) const
    {
        if (!m_initialized || !m_chunkManager)
        {
            return nullptr;
        }

        return m_chunkManager->getChunk(coord);
    }

    std::shared_ptr<Chunk> Grid::getChunkAt(const glm::vec3& worldPosition) const
    {
        if (!m_initialized || !m_chunkManager)
        {
            return nullptr;
        }

        // Convert world position to chunk coordinates
        ChunkCoord coord = ChunkCoord::fromWorldPosition(worldPosition, m_chunkSize);

        return getChunk(coord);
    }

    std::shared_ptr<Chunk> Grid::createChunk(const ChunkCoord& coord)
    {
        if (!m_initialized || !m_chunkManager)
        {
            return nullptr;
        }

        return m_chunkManager->createChunk(coord);
    }

    bool Grid::unloadChunk(const ChunkCoord& coord)
    {
        if (!m_initialized || !m_chunkManager)
        {
            return false;
        }

        return m_chunkManager->unloadChunk(coord);
    }

    bool Grid::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
                       glm::vec3& hitPosition, glm::vec3& hitNormal, Voxel& hitVoxel)
    {
        if (!m_initialized)
        {
            return false;
        }

        Utility::Profiler::getInstance()->beginSample("Grid::raycast");

        // Create ray and initialize DDA algorithm parameters
        Utility::Ray ray;
        ray.setOrigin(origin);
        ray.setDirection(direction);

        // Transform ray to grid space
        glm::vec3 gridOrigin = origin;
        glm::vec3 gridDir = direction;

        // Ray starting parameters
        glm::vec3 voxelPos = glm::floor(gridOrigin);

        // Direction of the ray (sign of each component)
        glm::ivec3 step(
            gridDir.x > 0.0f ? 1 : -1,
            gridDir.y > 0.0f ? 1 : -1,
            gridDir.z > 0.0f ? 1 : -1
        );

        // Calculate delta distance for each axis
        glm::vec3 deltaDist(
            std::abs(gridDir.x) < 1e-6f ? FLT_MAX : std::abs(1.0f / gridDir.x),
            std::abs(gridDir.y) < 1e-6f ? FLT_MAX : std::abs(1.0f / gridDir.y),
            std::abs(gridDir.z) < 1e-6f ? FLT_MAX : std::abs(1.0f / gridDir.z)
        );

        // Calculate distance to next voxel boundary
        glm::vec3 sideDist;
        if (gridDir.x > 0.0f)
        {
            sideDist.x = (std::floor(gridOrigin.x) + 1.0f - gridOrigin.x) * deltaDist.x;
        }
        else
        {
            sideDist.x = (gridOrigin.x - std::floor(gridOrigin.x)) * deltaDist.x;
        }

        if (gridDir.y > 0.0f)
        {
            sideDist.y = (std::floor(gridOrigin.y) + 1.0f - gridOrigin.y) * deltaDist.y;
        }
        else
        {
            sideDist.y = (gridOrigin.y - std::floor(gridOrigin.y)) * deltaDist.y;
        }

        if (gridDir.z > 0.0f)
        {
            sideDist.z = (std::floor(gridOrigin.z) + 1.0f - gridOrigin.z) * deltaDist.z;
        }
        else
        {
            sideDist.z = (gridOrigin.z - std::floor(gridOrigin.z)) * deltaDist.z;
        }

        // Perform DDA algorithm
        glm::ivec3 normal(0, 0, 0);
        float distance = 0.0f;

        while (distance < maxDistance)
        {
            // Determine which voxel boundary to cross next
            if (sideDist.x < sideDist.y && sideDist.x < sideDist.z)
            {
                // X boundary
                voxelPos.x += step.x;
                distance = sideDist.x;
                sideDist.x += deltaDist.x;
                normal = glm::ivec3(-step.x, 0, 0);
            }
            else if (sideDist.y < sideDist.z)
            {
                // Y boundary
                voxelPos.y += step.y;
                distance = sideDist.y;
                sideDist.y += deltaDist.y;
                normal = glm::ivec3(0, -step.y, 0);
            }
            else
            {
                // Z boundary
                voxelPos.z += step.z;
                distance = sideDist.z;
                sideDist.z += deltaDist.z;
                normal = glm::ivec3(0, 0, -step.z);
            }

            // Check if we've gone out of bounds
            if (!m_bounds.contains(glm::vec3(voxelPos)))
            {
                Utility::Profiler::getInstance()->endSample();
                return false;
            }

            // Get the voxel at the current position
            Voxel voxel = getVoxel(static_cast<int>(voxelPos.x),
                                   static_cast<int>(voxelPos.y),
                                   static_cast<int>(voxelPos.z));

            // If not empty, we've hit something
            if (!voxel.isEmpty())
            {
                hitPosition = origin + direction * distance;
                hitNormal = glm::vec3(normal);
                hitVoxel = voxel;

                Utility::Profiler::getInstance()->endSample();
                return true;
            }
        }

        Utility::Profiler::getInstance()->endSample();
        return false;
    }

    bool Grid::serialize(ECS::Serializer& serializer) const
    {
        if (!m_initialized)
        {
            return false;
        }

        // Begin grid object
        serializer.beginObject("Grid");

        // Write grid properties
        serializer.write(m_chunkSize);

        // Write bounds
        serializer.writeVec3(m_bounds.getMin());
        serializer.writeVec3(m_bounds.getMax());

        // Write default voxel
        serializer.write(m_defaultVoxel.type);
        serializer.write(m_defaultVoxel.data);

        // Write loaded chunks
        auto loadedChunks = getLoadedChunkCoords();
        serializer.beginArray("chunks", loadedChunks.size());

        for (const auto& coord : loadedChunks)
        {
            auto chunk = getChunk(coord);
            if (chunk)
            {
                chunk->serialize(serializer);
            }
        }

        serializer.endArray();

        // End grid object
        serializer.endObject();

        return true;
    }

    bool Grid::deserialize(ECS::Deserializer& deserializer)
    {
        if (!m_initialized)
        {
            if (!initialize())
            {
                return false;
            }
        }

        // Begin grid object
        if (!deserializer.beginObject("Grid"))
        {
            return false;
        }

        // Read grid properties
        deserializer.read(m_chunkSize);

        // Read bounds
        glm::vec3 boundsMin, boundsMax;
        deserializer.readVec3(boundsMin);
        deserializer.readVec3(boundsMax);
        m_bounds = Utility::AABB(boundsMin, boundsMax);

        // Read default voxel
        deserializer.read(m_defaultVoxel.type);
        deserializer.read(m_defaultVoxel.data);

        // Read chunks
        size_t chunkCount;
        if (deserializer.beginArray("chunks", chunkCount))
        {
            for (size_t i = 0; i < chunkCount; i++)
            {
                glm::vec3 coord;
                deserializer.readVec3(coord);
                ChunkCoord chunkCoord(coord.x, coord.y, coord.z);

                auto chunk = createChunk(chunkCoord);
                if (chunk)
                {
                    chunk->deserialize(deserializer);
                }
            }

            deserializer.endArray();
        }

        // End grid object
        deserializer.endObject();

        return true;
    }

    std::vector<ChunkCoord> Grid::getLoadedChunkCoords() const
    {
        if (!m_initialized || !m_chunkManager)
        {
            return {};
        }

        // Get all loaded chunks from the chunk manager
        const auto& chunks = m_chunkManager->getLoadedChunks();

        // Extract coordinates
        std::vector<ChunkCoord> coords;
        coords.reserve(chunks.size());

        for (const auto& pair : chunks)
        {
            coords.push_back(pair.first);
        }

        return coords;
    }

    size_t Grid::getLoadedChunkCount() const
    {
        if (!m_initialized || !m_chunkManager)
        {
            return 0;
        }

        return m_chunkManager->getLoadedChunks().size();
    }

    std::vector<std::shared_ptr<Chunk>> Grid::getChunksInRadius(const glm::vec3& worldPosition, float radius)
    {
        if (!m_initialized || !m_chunkManager)
        {
            return {};
        }

        return m_chunkManager->getChunksAroundPoint(worldPosition, radius);
    }

    void Grid::worldToGrid(const glm::vec3& worldPos, int& gridX, int& gridY, int& gridZ) const
    {
        // World to grid is a straight conversion since we use 1:1 scale
        gridX = static_cast<int>(std::floor(worldPos.x));
        gridY = static_cast<int>(std::floor(worldPos.y));
        gridZ = static_cast<int>(std::floor(worldPos.z));
    }

    void Grid::gridToChunkAndLocal(int gridX, int gridY, int gridZ, ChunkCoord& chunkCoord,
                                   int& localX, int& localY, int& localZ) const
    {
        // Calculate chunk coordinate by dividing by chunk size
        chunkCoord.x = gridX < 0 ? (gridX + 1) / m_chunkSize - 1 : gridX / m_chunkSize;
        chunkCoord.y = gridY < 0 ? (gridY + 1) / m_chunkSize - 1 : gridY / m_chunkSize;
        chunkCoord.z = gridZ < 0 ? (gridZ + 1) / m_chunkSize - 1 : gridZ / m_chunkSize;

        // Calculate local coordinates within the chunk
        localX = gridX - chunkCoord.x * m_chunkSize;
        localY = gridY - chunkCoord.y * m_chunkSize;
        localZ = gridZ - chunkCoord.z * m_chunkSize;
    }

    void Grid::worldToChunkAndLocal(const glm::vec3& worldPos, ChunkCoord& chunkCoord,
                                    int& localX, int& localY, int& localZ) const
    {
        // Convert to grid coordinates first
        int gridX, gridY, gridZ;
        worldToGrid(worldPos, gridX, gridY, gridZ);

        // Then convert to chunk and local coordinates
        gridToChunkAndLocal(gridX, gridY, gridZ, chunkCoord, localX, localY, localZ);
    }

} // namespace PixelCraft::Voxel