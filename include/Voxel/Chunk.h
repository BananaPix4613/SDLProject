// -------------------------------------------------------------------------
// Chunk.h
// -------------------------------------------------------------------------
#pragma once

#include "Voxel/ChunkCoord.h"
#include "Voxel/Voxel.h"
#include "Utility/Serialization/SerializationUtility.h"
#include "Utility/AABB.h"
#include "ECS/Serializer.h"
#include "ECS/Deserializer.h"

#include <memory>
#include <vector>
#include <atomic>

namespace PixelCraft::Voxel
{

    // Forward declarations
    class ChunkMesh;
    struct Voxel;

    /**
     * @brief Chunk representing a fixed-size volume of voxels
     *
     * A Chunk is a 3D grid of voxels with fixed dimensions that forms
     * the basic unit of the voxel world. Chunks are loaded, unloaded,
     * and streamed based on player position and gameplay needs.
     */
    class Chunk
    {
    public:
        /**
         * @brief Default constructor
         */
        Chunk();

        /**
         * @brief Constructor with explicit coordinate
         * @param coord Chunk coordinate
         */
        explicit Chunk(const ChunkCoord& coord);

        /**
         * @brief Destructor
         */
        ~Chunk();

        /**
         * @brief Initialize the chunk data structures
         * @param chunkSize Size of the chunk in each dimension (typically 16 or 32)
         * @return True if initialization was successful
         */
        bool initialize(int chunkSize = 16);

        /**
         * @brief Get the chunk coordinate
         * @return Chunk coordinate
         */
        const ChunkCoord& getCoord() const
        {
            return m_coord;
        }

        /**
         * @brief Set the chunk coordinate
         * @param coord New chunk coordinate
         */
        void setCoord(const ChunkCoord& coord)
        {
            m_coord = coord;
        }

        /**
         * @brief Get size of the chunk in voxels
         * @return Chunk size
         */
        int getSize() const
        {
            return m_size;
        }

        /**
         * @brief Get voxel at the specified local position
         * @param x Local X coordinate
         * @param y Local Y coordinate
         * @param z Local Z coordinate
         * @return Reference to the voxel at the position
         */
        const Voxel& getVoxel(int x, int y, int z) const;

        /**
         * @brief Set voxel at the specified local position
         * @param x Local X coordinate
         * @param y Local Y coordinate
         * @param z Local Z coordinate
         * @param voxel Voxel to set
         * @return True if the voxel was changed
         */
        bool setVoxel(int x, int y, int z, const Voxel& voxel);

        /**
         * @brief Get voxel at the specified local position with bounds checking
         * @param x Local X coordinate
         * @param y Local Y coordinate
         * @param z Local Z coordinate
         * @param defaultVoxel Default voxel to return if position is out of bounds
         * @return Voxel at the position or default voxel
         */
        const Voxel& getVoxelSafe(int x, int y, int z, const Voxel& defaultVoxel) const;

        /**
         * @brief Check if a local position is valid
         * @param x Local X coordinate
         * @param y Local Y coordinate
         * @param z Local Z coordinate
         * @return True if the position is within chunk bounds
         */
        bool isValidPosition(int x, int y, int z) const;

        /**
         * @brief Fill the entire chunk with a specific voxel
         * @param voxel Voxel to fill with
         */
        void fill(const Voxel& voxel);

        /**
         * @brief Generate mesh for the chunk
         * @param forceRegenerate Force mesh regeneration even if not dirty
         * @return True if mesh generation was successful
         */
        bool generateMesh(bool forceRegenerate = false);

        /**
         * @brief Get the chunk mesh
         * @return Shared pointer to the chunk mesh
         */
        std::shared_ptr<ChunkMesh> getMesh() const
        {
            return m_mesh;
        }

        /**
         * @brief Get the chunk's bounding box in world space
         * @return Axis-aligned bounding box
         */
        const Utility::AABB& getBounds() const
        {
            return m_bounds;
        }

        /**
         * @brief Check if the chunk is dirty (needs saving)
         * @return True if the chunk has been modified since last save
         */
        bool isDirty() const
        {
            return m_dirty;
        }

        /**
         * @brief Mark the chunk as dirty
         */
        void markDirty()
        {
            m_dirty = true;
        }

        /**
         * @brief Clear the dirty flag
         */
        void clearDirty()
        {
            m_dirty = false;
        }

        /**
         * @brief Check if the chunk has a generated mesh
         * @return True if the chunk has a valid mesh
         */
        bool hasMesh() const
        {
            return m_mesh != nullptr && m_meshGenerated;
        }

        /**
         * @brief Check if the chunk is completely empty
         * @return True if the chunk contains only empty voxels
         */
        bool isEmpty() const
        {
            return m_empty;
        }

        /**
         * @brief Get the memory usage of this chunk
         * @return Memory usage in bytes
         */
        size_t getMemoryUsage() const;

        /**
         * @brief Set a neighbor chunk
         * @param direction Direction index (0-5 for -x, +x, -y, +y, -z, +z)
         * @param chunk Neighbor chunk
         */
        void setNeighbor(int direction, std::weak_ptr<Chunk> chunk);

        /**
         * @brief Get a neighbor chunk
         * @param direction Direction index (0-5 for -x, +x, -y, +y, -z, +z)
         * @return Weak pointer to the neighbor chunk
         */
        std::weak_ptr<Chunk> getNeighbor(int direction) const;

        /**
         * @brief Notify neighbors that this chunk has changed
         */
        void notifyNeighbors();

        /**
         * @brief Set the visibility distance for LOD calculation
         * @param distance Distance from viewer
         */
        void setVisibilityDistance(float distance)
        {
            m_visibilityDistance = distance;
        }

        /**
         * @brief Get the visibility distance
         * @return Current visibility distance
         */
        float getVisibilityDistance() const
        {
            return m_visibilityDistance;
        }

        /**
         * @brief Check if the chunk needs to be re-meshed
         * @return True if mesh needs to be regenerated
         */
        bool needsRemesh() const
        {
            return m_meshDirty;
        }

        /**
         * @brief Mark the mesh as dirty, requiring regeneration
         */
        void markMeshDirty()
        {
            m_meshDirty = true;
        }

        /**
         * @brief Cancel current mesh generation if in progress
         */
        void cancelMeshGeneration();

        /**
         * @brief Update ambient occlusion and lighting
         */
        void updateLighting();

        // Serialization methods
        static void defineSchema(Utility::Serialization::Schema& schema)
        {
            schema.addField("coord", Utility::Serialization::ValueType::Object);
            schema.addField("size", Utility::Serialization::ValueType::Int32);
            schema.addField("voxels", Utility::Serialization::ValueType::Binary);
            schema.addField("empty", Utility::Serialization::ValueType::Bool);
        }

        Utility::Serialization::SerializationResult serialize(Utility::Serialization::Serializer& serializer) const
        {
            using namespace Utility::Serialization;

            auto result = serializer.beginObject("Chunk");
            if (!result) return result;

            // Serialize chunk coordinate
            result = serializer.writeField("coord", m_coord);
            if (!result) return result;

            // Serialize chunk size
            result = serializer.writeField("size", m_size);
            if (!result) return result;

            // Serialize voxel data as binary blob
            if (m_voxels && !m_empty)
            {
                size_t dataSize = m_size * m_size * m_size * sizeof(Voxel);
                result = serializer.writeFieldName("voxels");
                if (!result) return result;

                result = serializer.writeBinary(m_voxels.get(), dataSize);
                if (!result) return result;
            }
            else
            {
                // Just write an empty binary field if the chunk is empty
                result = serializer.writeFieldName("voxels");
                if (!result) return result;

                result = serializer.writeBinary(nullptr, 0);
                if (!result) return result;
            }

            // Serialize empty flag
            result = serializer.writeField("empty", m_empty);
            if (!result) return result;

            return serializer.endObject();
        }

        Utility::Serialization::SerializationResult deserialize(Utility::Serialization::Deserializer& deserializer)
        {
            using namespace Utility::Serialization;

            auto result = deserializer.beginObject("Chunk");
            if (!result) return result;

            // Read chunk coordinate
            result = deserializer.readField("coord", m_coord);
            if (!result) return result;

            // Read chunk size
            int32_t size;
            result = deserializer.readField("size", size);
            if (!result) return result;

            // Initialize chunk with the read size if needed
            if (m_size != size || !m_voxels)
            {
                initialize(size);
            }

            // Read empty flag
            result = deserializer.readField("empty", m_empty);
            if (!result) return result;

            // Read voxel data if not empty
            if (!m_empty && deserializer.hasField("voxels"))
            {
                size_t dataSize = m_size * m_size * m_size * sizeof(Voxel);
                size_t actualSize = 0;

                result.success = deserializer.findField("voxels");
                if (!result) return result;

                // Ensure voxel array is allocated
                if (!m_voxels)
                {
                    m_voxels = std::make_unique<Voxel[]>(m_size * m_size * m_size);
                }

                result = deserializer.readBinary(m_voxels.get(), dataSize, actualSize);
                if (!result) return result;

                if (actualSize > 0 && actualSize != dataSize)
                {
                    return SerializationResult("Voxel data size mismatch: expected " +
                                               std::to_string(dataSize) + ", got " +
                                               std::to_string(actualSize));
                }
            }
            else
            {
                // If empty or no voxel data, ensure the chunk is marked as empty
                m_empty = true;

                // Fill with empty voxels if the array exists
                if (m_voxels)
                {
                    Voxel emptyVoxel;
                    fill(emptyVoxel);
                }
            }

            // Update bounds based on coordinates and size
            m_bounds = Utility::AABB(
                glm::vec3(m_coord.x * m_size, m_coord.y * m_size, m_coord.z * m_size),
                glm::vec3((m_coord.x + 1) * m_size, (m_coord.y + 1) * m_size, (m_coord.z + 1) * m_size)
            );

            // Mark chunk as dirty to regenerate mesh if needed
            m_dirty = true;
            m_meshDirty = true;

            return deserializer.endObject();
        }

        static std::string getTypeName()
        {
            return "Chunk";
        }

    private:
        /**
         * @brief Calculate index for voxel at position
         * @param x Local X coordinate
         * @param y Local Y coordinate
         * @param z Local Z coordinate
         * @return Array index for the voxel
         */
        inline size_t voxelIndex(int x, int y, int z) const
        {
            return (z * m_size * m_size) + (y * m_size) + x;
        }

        /**
         * @brief Update the empty flag based on voxel content
         */
        void updateEmptyState();

        /**
         * @brief Update the world-space bounding box
         */
        void updateBounds();

        /**
         * @brief Check if a voxel is exposed (visible from outside)
         * @param x Local X coordinate
         * @param y Local Y coordinate
         * @param z Local Z coordinate
         * @return True if the voxel is visible from at least one side
         */
        bool isVoxelExposed(int x, int y, int z) const;

        // Basic properties
        ChunkCoord m_coord;       ///< Position in chunk grid
        int m_size;               ///< Size in each dimension
        Utility::AABB m_bounds;   ///< World-space bounding box

        // Voxel data
        std::unique_ptr<Voxel[]> m_voxels;  ///< Voxel data array

        // Mesh and rendering
        std::shared_ptr<ChunkMesh> m_mesh;  ///< Generated mesh for rendering
        float m_visibilityDistance;         ///< Distance from viewer for LOD

        // Neighbors for meshing and physics
        std::array<std::weak_ptr<Chunk>, 6> m_neighbors;  ///< Adjacent chunks

        // State flags
        bool m_initialized;    ///< Whether chunk is initialized
        bool m_empty;          ///< Whether chunk contains only empty voxels
        bool m_dirty;          ///< Whether chunk needs saving
        bool m_meshDirty;      ///< Whether mesh needs regeneration
        bool m_meshGenerated;  ///< Whether mesh is generated

        // Threading support
        std::atomic<bool> m_meshGenerationCanceled;  ///< Flag to cancel mesh generation
    };

} // namespace PixelCraft::Voxel