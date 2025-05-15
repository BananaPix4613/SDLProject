// -------------------------------------------------------------------------
// Chunk.cpp
// -------------------------------------------------------------------------
#include "Voxel/Chunk.h"
#include "Voxel/ChunkMesh.h"
#include "Voxel/Voxel.h"
#include "ECS/Serializer.h"
#include "ECS/Deserializer.h"
#include "Core/Logger.h"
#include "Utility/StringUtils.h"
#include "Utility/Profiler.h"
#include "glm/glm.hpp"

#include <algorithm>
#include <cstring>

using StringUtils = PixelCraft::Utility::StringUtils;
namespace Log = PixelCraft::Core;

namespace PixelCraft::Voxel
{

    namespace
    {
        // Default empty voxel (for out-of-bounds access)
        const Voxel EMPTY_VOXEL = {0, 0}; // Assuming Voxel has type and data fields

        // Neighbor offsets for the 6 directions (-x, +x, -y, +y, -z, +z)
        constexpr int OFFSETS[6][3] = {
            {-1, 0, 0},  // -X
            {1, 0, 0},   // +X
            {0, -1, 0},  // -Y
            {0, 1, 0},   // +Y
            {0, 0, -1},  // -Z
            {0, 0, 1}    // +Z
        };

        // Convert a direction to the opposite direction
        constexpr int oppositeDirection(int dir)
        {
            return dir ^ 1; // Flips the lowest bit
        }
    }

    Chunk::Chunk()
        : m_coord(0, 0, 0)
        , m_size(0)
        , m_voxels(nullptr)
        , m_visibilityDistance(0.5f)
        , m_initialized(false)
        , m_empty(true)
        , m_dirty(false)
        , m_meshDirty(false)
        , m_meshGenerated(false)
        , m_meshGenerationCanceled(false)
    {
        // Clear neighbor array
        for (auto& neighbor : m_neighbors)
        {
            neighbor.reset();
        }
    }

    Chunk::Chunk(const ChunkCoord& coord)
        : m_coord(coord)
        , m_size(0)
        , m_voxels(nullptr)
        , m_visibilityDistance(0.0f)
        , m_initialized(false)
        , m_empty(true)
        , m_dirty(false)
        , m_meshDirty(false)
        , m_meshGenerated(false)
        , m_meshGenerationCanceled(false)
    {
        // Clear neighbor array
        for (auto& neighbor : m_neighbors)
        {
            neighbor.reset();
        }
    }

    Chunk::~Chunk()
    {
        // Cancel any ongoing mesh generation
        cancelMeshGeneration();
    }

    bool Chunk::initialize(int chunkSize)
    {
        if (m_initialized)
        {
            Log::warn("Chunk already initialized");
            return true;
        }

        if (chunkSize <= 0)
        {
            Log::error("Invalid chunk size: " + chunkSize);
            return false;
        }

        m_size = chunkSize;

        // Allocate voxel data
        size_t voxelCount = static_cast<size_t>(m_size * m_size * m_size);
        m_voxels = std::make_unique<Voxel[]>(voxelCount);

        // Initialize all voxels to empty
        for (size_t i = 0; i < voxelCount; i++)
        {
            m_voxels[i] = EMPTY_VOXEL;
        }

        // Update bounding box
        updateBounds();

        m_initialized = true;
        m_empty = true; // Initially empty
        m_dirty = false;
        m_meshDirty = true; // Need to generate mesh

        return true;
    }

    const Voxel& Chunk::getVoxel(int x, int y, int z) const
    {
        if (!m_initialized || !isValidPosition(x, y, z))
        {
            return EMPTY_VOXEL;
        }

        return m_voxels[voxelIndex(x, y, z)];
    }

    bool Chunk::setVoxel(int x, int y, int z, const Voxel& voxel)
    {
        if (!m_initialized || !isValidPosition(x, y, z))
        {
            return false;
        }

        size_t index = voxelIndex(x, y, z);

        // Check if the voxel is actually changing
        if (memcmp(&m_voxels[index], &voxel, sizeof(Voxel)) == 0)
        {
            return false; // No change
        }

        // Update the voxel
        m_voxels[index] = voxel;

        // Mark chunk as modified
        m_dirty = true;
        m_meshDirty = true;

        // Update empty state if adding a non-empty voxel
        if (m_empty && voxel.type != 0)
        {
            m_empty = false;
        }
        else if (!m_empty && voxel.type == 0)
        {
            // If setting to empty, we need to check if the entire chunk is now empty
            updateEmptyState();
        }

        // Notify neighbors if this voxel is on a chunk boundary
        bool isOnBoundary = (x == 0 || x == m_size - 1 ||
                             y == 0 || y == m_size - 1 ||
                             z == 0 || z == m_size - 1);

        if (isOnBoundary)
        {
            notifyNeighbors();
        }

        return true;
    }

    const Voxel& Chunk::getVoxelSafe(int x, int y, int z, const Voxel& defaultVoxel) const
    {
        if (!m_initialized || !isValidPosition(x, y, z))
        {
            return defaultVoxel;
        }

        return m_voxels[voxelIndex(x, y, z)];
    }

    bool Chunk::isValidPosition(int x, int y, int z) const
    {
        return x >= 0 && x < m_size &&
            y >= 0 && y < m_size &&
            z >= 0 && z < m_size;
    }

    void Chunk::fill(const Voxel& voxel)
    {
        if (!m_initialized)
        {
            return;
        }

        size_t voxelCount = static_cast<size_t>(m_size * m_size * m_size);

        for (size_t i = 0; i < voxelCount; i++)
        {
            m_voxels[i] = voxel;
        }

        m_empty = (voxel.type == 0);
        m_dirty = true;
        m_meshDirty = true;

        // Notify neighbors as all boundaries have changed
        notifyNeighbors();
    }

    bool Chunk::generateMesh(bool forceRegenerate)
    {
        if (!m_initialized)
        {
            return false;
        }

        // If empty, clear existing mesh and return
        if (m_empty)
        {
            m_mesh.reset();
            m_meshGenerated = true;
            m_meshDirty = false;
            return true;
        }

        // Skip if mesh is up to date and not forced
        if (!m_meshDirty && !forceRegenerate && m_mesh && m_meshGenerated)
        {
            return true;
        }

        Utility::Profiler::getInstance()->beginSample("Chunk::generateMesh");

        // Reset cancellation flag
        m_meshGenerationCanceled = false;

        // Create mesh if it doesn't exist
        if (!m_mesh)
        {
            m_mesh = std::make_shared<ChunkMesh>(m_coord);
        }

        // Generate mesh
        bool success = false;

        // Check if we have all required neighbors for seamless meshing
        bool hasAllNeighbors = true;
        for (const auto& neighbor : m_neighbors)
        {
            if (neighbor.expired())
            {
                hasAllNeighbors = false;
                break;
            }
        }

        if (hasAllNeighbors)
        {
            // Full quality meshing with all neighbors
            success = m_mesh->generate(*this, m_neighbors, [this]() { return m_meshGenerationCanceled.load(); });
        }
        else
        {
            // Simple meshing without all neighbors
            success = m_mesh->generateSimple(*this, [this]() { return m_meshGenerationCanceled.load();  });
        }

        // Check if canceled
        if (m_meshGenerationCanceled)
        {
            Utility::Profiler::getInstance()->endSample();
            return false;
        }

        if (success)
        {
            m_meshGenerated = true;
            m_meshDirty = false;
        }

        Utility::Profiler::getInstance()->endSample();
        return success;
    }

    bool Chunk::serialize(ECS::Serializer& serializer) const
    {
        if (!m_initialized)
        {
            return false;
        }

        Utility::Profiler::getInstance()->beginSample("Chunk::serialize");

        // Begin chunk object
        serializer.beginObject("Chunk");

        // Write basic properties
        serializer.write(m_size);
        serializer.writeVec3(glm::vec3(m_coord.x, m_coord.y, m_coord.z));
        serializer.write(m_empty);

        // Write voxel data (skip if empty)
        if (!m_empty)
        {
            size_t voxelCount = static_cast<size_t>(m_size * m_size * m_size);
            serializer.beginArray("voxels", voxelCount);

            // Write all voxels
            for (size_t i = 0; i < voxelCount; i++)
            {
                serializer.write(m_voxels[i].type);
                serializer.write(m_voxels[i].data);
            }

            serializer.endArray();
        }

        // End chunk object
        serializer.endObject();

        Utility::Profiler::getInstance()->endSample();
        return true;
    }

    bool Chunk::deserialize(ECS::Deserializer& deserializer)
    {
        if (!deserializer.beginObject("Chunk"))
        {
            return false;
        }

        Utility::Profiler::getInstance()->beginSample("Chunk::deserialize");

        // Read basic properties
        int size;
        deserializer.read(size);

        // Read empty flag
        bool isEmpty;
        deserializer.read(isEmpty);

        // Read voxel data
        if (!isEmpty)
        {
            size_t voxelCount;
            if (deserializer.beginArray("voxels", voxelCount))
            {
                // Ensure array size matches what we expect
                if (voxelCount != static_cast<size_t>(m_size * m_size * m_size))
                {
                    deserializer.endArray();
                    deserializer.endObject();
                    return false;
                }

                // Read all voxels
                for (size_t i = 0; i < voxelCount; i++)
                {
                    deserializer.read(m_voxels[i].type);
                    deserializer.read(m_voxels[i].data);
                }

                deserializer.endArray();
            }
        }
        else
        {
            // Fill with empty voxels
            fill(Voxel(0, 0));
        }

        // End chunk object
        deserializer.endObject();

        // Update state
        updateEmptyState();
        m_dirty = false;
        m_meshDirty = true;

        // Update bounds
        updateBounds();

        Utility::Profiler::getInstance()->endSample();
        return true;
    }

    size_t Chunk::getMemoryUsage() const
    {
        size_t usage = 0;

        // Voxel data
        if (m_voxels)
        {
            usage += static_cast<size_t>(m_size * m_size * m_size) * sizeof(Voxel);
        }

        // Mesh data
        if (m_mesh)
        {
            usage += m_mesh->getMemoryUsage();
        }

        // Other member variables
        usage += sizeof(*this);

        return usage;
    }

    void Chunk::setNeighbor(int direction, std::weak_ptr<Chunk> chunk)
    {
        if (direction >= 0 && direction < 6)
        {
            m_neighbors[direction] = chunk;

            // If we have a new neighbor, out mesh may need updating at the boundary
            m_meshDirty = true;
        }
    }

    std::weak_ptr<Chunk> Chunk::getNeighbor(int direction) const
    {
        if (direction >= 0 && direction < 6)
        {
            return m_neighbors[direction];
        }

        return std::weak_ptr<Chunk>();
    }

    void Chunk::notifyNeighbors()
    {
        // Mark neighboring chunks' meshes as dirty if they exist
        for (int i = 0; i < 6; i++)
        {
            auto neighbor = m_neighbors[i].lock();
            if (neighbor)
            {
                neighbor->markMeshDirty();
            }
        }
    }

    void Chunk::cancelMeshGeneration()
    {
        m_meshGenerationCanceled = true;
    }

    void Chunk::updateLighting()
    {
        if (!m_initialized)
        {
            return;
        }

        // This would be a more complex implementation in a real engine
        // For now, just mark mesh as dirty to trigger regeneration with lighting
        m_meshDirty = true;
    }

    void Chunk::updateEmptyState()
    {
        if (!m_initialized)
        {
            m_empty = true;
            return;
        }

        m_empty = true;

        size_t voxelCount = static_cast<size_t>(m_size * m_size * m_size);
        for (size_t i = 0; i < voxelCount; i++)
        {
            if (m_voxels[i].type != 0)
            {
                m_empty = false;
                return;
            }
        }
    }

    void Chunk::updateBounds()
    {
        // Calculate world-space bounds based on chunk coordinates and size
        float worldSize = static_cast<float>(m_size);
        glm::vec3 minCorner(
            static_cast<float>(m_coord.x) * worldSize,
            static_cast<float>(m_coord.y) * worldSize,
            static_cast<float>(m_coord.z) * worldSize
        );

        glm::vec3 maxCorner = minCorner + glm::vec3(worldSize);

        m_bounds = Utility::AABB(minCorner, maxCorner);
    }

    bool Chunk::isVoxelExposed(int x, int y, int z) const
    {
        // If the voxel is empty, it's not exposed
        if (getVoxel(x, y, z).type == 0)
        {
            return false;
        }

        // Check the six adjacent voxels
        for (int i = 0; i < 6; i++)
        {
            int nx = x + OFFSETS[i][0];
            int ny = y + OFFSETS[i][1];
            int nz = z + OFFSETS[i][2];

            // If out of bounds, check the neighboring chunk
            if (!isValidPosition(nx, ny, nz))
            {
                // Get the direction and neighboring chunk
                auto neighbor = m_neighbors[i].lock();

                if (!neighbor)
                {
                    // If no neighbor, consider the voxel exposed from this side
                    return true;
                }

                // Convert to neighboring chunk's coordinate space
                int nnx = nx, nny = ny, nnz = nz;

                if (nx < 0) nnx = m_size - 1;
                else if (nx >= m_size) nnx = 0;

                if (ny < 0) nny = m_size - 1;
                else if (ny >= m_size) nny = 0;

                if (nz < 0) nnz = m_size - 1;
                else if (nz >= m_size) nnz = 0;

                // Check the voxel in the neighboring chunk
                if (neighbor->getVoxel(nnx, nny, nnz).type == 0)
                {
                    return true; // Exposed on the side
                }
            }
            else
            {
                // If the adjacent voxel is empty, this voxel is exposed
                if (getVoxel(nx, ny, nz).type == 0)
                {
                    return true;
                }
            }
        }

        // Not exposed on any side
        return false;
    }

} // namespace PixelCraft::Voxel