// -------------------------------------------------------------------------
// ChunkMesh.h
// -------------------------------------------------------------------------
#pragma once

#include "Voxel/ChunkCoord.h"
#include "Rendering/Mesh.h"
#include "Utility/AABB.h"

#include <memory>
#include <vector>
#include <array>
#include <functional>

namespace PixelCraft::Voxel
{

    // Forward declarations
    class Chunk;

    /**
     * @brief Mesh generation and storage for voxel chunks
     * 
     * ChunkMesh handles generating optimized meshes for rendering voxel chunks,
     * including greedy meshing optimization and efficient GPU storage;
     */
    class ChunkMesh
    {
    public:
        /**
         * @brief Constructor
         * @param coord Chunk coordinate
         */
        explicit ChunkMesh(const ChunkCoord& coord);

        /**
         * @brief Destructor
         */
        ~ChunkMesh();

        /**
         * @brief Generate mesh for a chunk with full neighboring chunks
         * @param chunk Chunk to generate mesh for
         * @param neighbors Array of 6 neighbor chunks for seamless meshing
         * @param cancelCheck Optional function to check if generation should be canceled
         * @return True if generation was successful
         */
        bool generate(const Chunk& chunk,
                      const std::array<std::weak_ptr<Chunk>, 6>& neighbors,
                      std::function<bool()> cancelCheck = nullptr);

        /**
         * @brief Generate mesh without requiring all neighbors
         * @param chunk Chunk to generate mesh for
         * @param cancelCheck Optional function to check if generation should be canceled
         * @return True if generation was successful
         */
        bool generateSimple(const Chunk& chunk, std::function<bool()> cancelCheck = nullptr);

        /**
         * @brief Get the render mesh
         * @return Shared pointer to the render mesh
         */
        std::shared_ptr<Rendering::Mesh> getRenderMesh() const
        {
            return m_renderMesh;
        }

        /**
         * @brief Get the chunk coordinate
         * @return Chunk coordinate
         */
        const ChunkCoord& getCoord() const
        {
            return m_coord;
        }

        /**
         * @brief Get the memory usage of this mesh
         * @return Memory usage in bytes
         */
        size_t getMemoryUsage() const;

        /**
         * @brief Get the vertex count of the mesh
         * @return Number of vertices
         */
        size_t getVertexCount() const;

        /**
         * @brief Get the index count of the mesh
         * @return Number of indices
         */
        size_t getIndexCount() const;

        /**
         * @brief Update the bounds based on chunk coordinates
         */
        void updateBounds();

        /**
         * @brief Get the bounding box of the mesh
         * @return Axis-aligned bounding box
         */
        const Utility::AABB& getBounds() const
        {
            return m_bounds;
        }

        /**
         * @brief Update the level of detail based on distance
         * @param distance Distance from viewer
         * @return True if the LOD was updated
         */
        bool updateLOD(float distance);

        /**
         * @brief Create all LOD levels for this mesh
         * @return True if LOD creation was successful
         */
        bool createLODs();

        /**
         * @brief Get the current LOD level
         * @return Current LOD level (0 = full detail)
         */
        int getLODLevel()
        {
            return m_currentLOD;
        }

    private:
        /**
         * @brief Vertex data structure for the chunk mesh
         */
        struct Vertex
        {
            glm::vec3 position;     ///< Vertex position
            glm::vec3 normal;       ///< Normal vector
            glm::vec2 texCoord;     ///< Texture coordinates
            uint32_t color;         ///< Vertex color (RGBA)
            uint16_t material;      ///< Material ID
            uint16_t occlusion;     ///< Ambient occlusion value
        };

        /**
         * @brief Clear mesh data
         */
        void clear();

        /**
         * @brief Helper for voxel ray casting
         * @param chunk Chunk the ray is cast to
         * @param neighbors Neighbors of the chunk
         * @param origin Origin ray is cast from
         * @param direction Direction ray is cast toward
         * @param maxDistance Maximum distance ray is cast
         * @param hitDistance [out] Distance ray hits voxel
         * @return True if voxel is hit, false otherwise
         */
        bool raycastVoxel(const Chunk& chunk,
                          const std::array<std::weak_ptr<Chunk>, 6>& neighbors,
                          const glm::vec3& origin,
                          const glm::vec3& direction,
                          float maxDistance,
                          float& hitDistance);

        /**
         * @brief Generate hemisphere ray directions
         * @param normal Normal of hemisphere
         * @param sampleCount Count of sample rays to cast
         * @return Vector of rays cast from hemisphere
         */
        std::vector<glm::vec3> generateHemisphereRays(const glm::vec3& normal, int sampleCount);

        /**
         * @brief Update the rendering mesh from internal data
         * @return True if the update was successful
         */
        bool updateRenderMesh();

        /**
         * @brief Generate mesh using greedy meshing algorithm
         * @param chunk Chunk to generate mesh for
         * @param neighbors Array of neighbor chunks
         * @param cancelCheck Function to check for cancellation
         * @return True if generation was successful
         */
        bool generateGreedyMesh(const Chunk& chunk,
                                const std::array<std::weak_ptr<Chunk>, 6>& neighbors,
                                std::function<bool()> cancelCheck);

        /**
         * @brief Generate meh using simple cubes for each voxel
         * @param chunk Chunk to generate mesh for
         * @param cancelCheck Function to check for cancellation
         * @return True if generation was successful
         */
        bool generateCubeMesh(const Chunk& chunk, std::function<bool()> cancelCheck);

        /**
         * @brief Simplifies a mesh using quadric error metrics
         * @param vertices Original mesh vertices
         * @param indices Original mesh indicies
         * @param outVertices [out] Simplified vertices
         * @param outIndices [out] Simplified indices
         * @param targetRatio Ratio of vertices to remove
         */
        void simplifyMesh(const std::vector<Vertex>& vertices,
                          const std::vector<uint32_t>& indices,
                          std::vector<Vertex>& outVertices,
                          std::vector<uint32_t>& outIndices,
                          float targetRatio);

        /**
         * @brief Calculate ambient occlusion for vertices
         * @param chunk Chunk to calculate occlusion for
         * @param neighbors Array of neighbor chunks
         */
        void calculateAmbientOcclusion(const Chunk& chunk,
                                       const std::array<std::weak_ptr<Chunk>, 6>& neighbors);

        // Mesh data
        std::vector<Vertex> m_vertices;       ///< Vertex data
        std::vector<uint32_t> m_indices;      ///< Index data

        // Render mesh
        std::shared_ptr<Rendering::Mesh> m_renderMesh;  ///< Render mesh

        // LOD support
        int m_currentLOD;                     ///< Current level of detail
        std::vector<std::shared_ptr<Rendering::Mesh>> m_lodMeshes;  ///< LOD meshes

        // Properties
        ChunkCoord m_coord;                   ///< Chunk coordinate
        Utility::AABB m_bounds;               ///< Bounding box
        bool m_dirty;                         ///< Whether mesh needs update
    };

} // namespace PixelCraft::Voxel