// -------------------------------------------------------------------------
// Grid.h
// -------------------------------------------------------------------------
#pragma once

#include "Voxel/ChunkManager.h"
#include "Voxel/Voxel.h"
#include "Utility/AABB.h"
#include "glm/glm.hpp"

#include <memory>
#include <vector>
#include <unordered_map>

namespace PixelCraft::Voxel
{

    /**
     * @brief 3D voxel grid with chunking and serialization
     * 
     * Grid provides a high-level interface to voxel data, abstracting away the
     * chunk-based management and providing world-space coordinate access.
     */
    class Grid
    {
    public:
        /**
         * @brief Constructor
         * @param chunkSize Size of each chunk in voxels (typically 16 or 32)
         */
        Grid(int chunkSize = 16);

        /**
         * @brief Destructor
         */
        ~Grid();

        /**
         * @brief Initialize the grid
         * @return True if initialization was successful
         */
        bool initialize();

        /**
         * @brief Shut down the grid and release resources
         */
        void shutdown();

        /**
         * @brief Get voxel at world position
         * @param worldPosition Position in world space
         * @return Voxel at the position or default voxel if none exists
         */
        Voxel getVoxel(const glm::vec3& worldPosition) const;

        /**
         * @brief Get voxel at grid coordinates
         * @param x X coordinate in grid space
         * @param y Y coordinate in grid space
         * @param z Z coordinate in grid space
         * @return Voxel at the position or default voxel if none exists
         */
        Voxel getVoxel(int x, int y, int z) const;

        /**
         * @brief Set voxel at world position
         * @param worldPosition Position in world space
         * @param voxel Voxel to set
         * @return True if the voxel was set successfully
         */
        bool setVoxel(const glm::vec3& worldPosition, const Voxel& voxel);

        /**
         * @brief Set voxel at grid coordinates
         * @param x X coordinate in grid space
         * @param y Y coordinate in grid space
         * @param z Z coordinate in grid space
         * @param voxel Voxel to set
         * @return True if the voxel was set successfully
         */
        bool setVoxel(int x, int y, int z, const Voxel& voxel);

        /**
         * @brief Get chunk at specified coordinates
         * @param coord Chunk coordinates
         * @return Shared pointer to the chunk or nullptr if not loaded
         */
        std::shared_ptr<Chunk> getChunk(const ChunkCoord& coord) const;

        /**
         * @brief Get chunk at specified world position
         * @param worldPosition Position in world space
         * @return Shared pointer to the chunk or nullptr if not loaded
         */
        std::shared_ptr<Chunk> getChunkAt(const glm::vec3& worldPosition) const;

        /**
         * @brief Create a chunk at specified coordinates
         * @param coord Chunk coordinates
         * @return Shared pointer to the created chunk
         */
        std::shared_ptr<Chunk> createChunk(const ChunkCoord& coord);

        /**
         * @brief Unload a chunk
         * @param coord Chunk coordinates
         * @return True if the chunk was unloaded
         */
        bool unloadChunk(const ChunkCoord& coord);

        /**
         * @brief Get the bounding box of the grid
         * @return Axis-aligned bounding box
         */
        const Utility::AABB& getBounds() const
        {
            return m_bounds;
        }

        /**
         * @brief Set the bounding box of the grid
         * @param bounds New bounding box
         */
        void setBounds(const Utility::AABB& bounds)
        {
            m_bounds = bounds;
        }

        /**
         * @brief Get the chunk size
         * @return Size of each chunk in voxels
         */
        int getChunkSize() const
        {
            return m_chunkSize;
        }

        /**
         * @brief Perform a raycast through the voxel grid
         * @param origin Ray origin
         * @param direction Ray direction (normalized)
         * @param maxDistance Maximum ray distance
         * @param hitPosition Output parameter for hit position
         * @param hitNormal Output parameter for hit normal
         * @param hitVoxel Output parameter for hit voxel data
         * @return True if the ray hit a voxel
         */
        bool raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
                     glm::vec3& hitPosition, glm::vec3& hitNormal, Voxel& hitVoxel);

        /**
         * @brief Serialize the grid
         * @param serializer Serializer to write to
         * @return True if serialization was successful
         */
        bool serialize(ECS::Serializer& serializer) const;

        /**
         * @brief Deserialize the grid
         * @param deserializer Deserializer to read from
         * @return True if deserialization was successful
         */
        bool deserialize(ECS::Deserializer& deserializer);

        /**
         * @brief Get a list of all loaded chunk coordinates
         * @return Vector of loaded chunk coordinates
         */
        std::vector<ChunkCoord> getLoadedChunkCoords() const;

        /**
         * @brief Get the number of loaded chunks
         * @return Number of loaded chunks
         */
        size_t getLoadedChunkCount() const;

        /**
         * @brief Set the default voxel for empty space
         * @param voxel Default voxel
         */
        void getDefaultVoxel(const Voxel& voxel)
        {
            m_defaultVoxel = voxel;
        }

        /**
         * @brief Get the default voxel
         * @return Default voxel
         */
        const Voxel& getDefaultVoxel() const
        {
            return m_defaultVoxel;
        }

        /**
         * @brief Get all chunks within a radius of a world position
         * @param worldPosition Center position
         * @param radius Radius to search
         * @return Vector of chunks
         */
        std::vector<std::shared_ptr<Chunk>> getChunksInRadius(const glm::vec3& worldPosition, float radius);

    private:
        /**
         * @brief Convert world position to grid coordinates
         * @param worldPos World position
         * @param gridX Output X coordinate
         * @param gridY Output Y coordinate
         * @param gridZ Output Z coordinate
         */
        void worldToGrid(const glm::vec3& worldPos, int& gridX, int& gridY, int& gridZ) const;

        /**
         * @brief Convert grid coordinates to chunk coordinates and local position
         * @param gridX Grid X coordinate
         * @param gridY Grid Y coordinate
         * @param gridZ Grid Z coordinate
         * @param chunkCoord Output chunk coordinates
         * @param localX Output local X coordinate within chunk
         * @param localY Output local Y coordinate within chunk
         * @param localZ Output local Z coordinate within chunk
         */
        void gridToChunkAndLocal(int gridX, int gridY, int gridZ, ChunkCoord& chunkCoord,
                                 int& localX, int& localY, int& localZ) const;

        /**
         * @brief Convert world position directly to chunk coordinates and local position
         * @param worldPos World position
         * @param chunkCoord Output chunk coordinates
         * @param localX Output local X coordinate within chunk
         * @param localY Output local Y coordinate within chunk
         * @param localZ Output local Z coordinate within chunk
         */
        void worldToChunkAndLocal(const glm::vec3& worldPos, ChunkCoord& chunkCoord,
                                  int& localX, int& localY, int& localZ) const;

        // Chunk management
        std::shared_ptr<ChunkManager> m_chunkManager;
        int m_chunkSize;

        // Grid bounds
        Utility::AABB m_bounds;

        // Default voxel for empty space
        Voxel m_defaultVoxel;

        // State
        bool m_initialized;
    };

} // namespace PixelCraft::Voxel