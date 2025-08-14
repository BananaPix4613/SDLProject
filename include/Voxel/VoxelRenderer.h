// -------------------------------------------------------------------------
// VoxelRenderer.h
// -------------------------------------------------------------------------
#pragma once

#include "Core/Subsystem.h"
#include "Voxel/ChunkCoord.h"

#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace PixelCraft::Rendering
{
    class Material;
}

namespace PixelCraft::Voxel
{

    // Forward declarations
    class ChunkMesh;
    class Grid;
    class ChunkManager;

    /**
     * @brief Optimized renderer for voxel grids
     */
    class VoxelRenderer : public Core::Subsystem
    {
    public:
        /**
         * @brief Constructor
         */
        VoxelRenderer();

        /**
         * @brief Destructor
         */
        virtual ~VoxelRenderer();

        /**
         * @brief Initialize the voxel renderer
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Update the renderer
         * @param deltaTime Time since last update
         */
        void update(float deltaTime) override;

        /**
         * @brief Render the voxel grid
         */
        void render() override;

        /**
         * @brief Shut down the renderer
         */
        void shutdown() override;

        /**
         * @brief Get the name of the subsystem
         * @return The subsystem name
         */
        std::string getName() const override
        {
            return "VoxelRenderer";
        }

        /**
         * @brief Get the module's dependencies
         * @return Vector of dependency subsystem names
         */
        std::vector<std::string> getDependencies() const override;

        /**
         * @brief Set the grid to render
         * @param grid The grid to render
         */
        void setGrid(std::shared_ptr<Grid> grid);

        /**
         * @brief Get the current grid
         * @return The current grid
         */
        std::shared_ptr<Grid> getGrid() const;

        /**
         * @brief Set the material to use for rendering
         * @param material The material to use
         */
        void setMaterial(std::shared_ptr<Rendering::Material> material);

        /**
         * @brief Get the current material
         * @return The current material
         */
        std::shared_ptr<Rendering::Material> getMaterial() const;

        /**
         * @brief Update a specific chunk
         * @param coord The coordinates of the chunk to update
         */
        void updateChunk(const ChunkCoord& coord);

        /**
         * @brief Set the render distance
         * @param distance The render distance
         */
        void setRenderDistance(float distance);

        /**
         * @brief Get the render distance
         * @return The render distance
         */
        float getRenderDistance() const;

        /**
         * @brief Set the LOD distances
         * @param distances Vector of distances for each LOD level
         */
        void setLODLevels(const std::vector<float>& distances);

        /**
         * @brief Get the LOD distances
         * @return Vector of LOD distances
         */
        const std::vector<float>& getLODLevels() const;

        /**
         * @brief Get the number of visible chunks
         * @return The visible chunk count
         */
        size_t getVisibleChunkCount() const;

        /**
         * @brief Get the total triangle count
         * @return The total triangle count
         */
        size_t getTotalTriangleCount() const;

        /**
         * @brief Enable or disable frustum culling
         * @param enabled Whether culling is enabled
         */
        void setCullingEnabled(bool enabled);

        /**
         * @brief Check if culling is enabled
         * @return Whether culling is enabled
         */
        bool isCullingEnabled() const;

        /**
         * @brief Enable or disable occlusion culling
         * @param enabled Whether occlusion culling is enabled
         */
        void setOcclusionCullingEnabled(bool enabled);

        /**
         * @brief Check if occlusion culling is enabled
         * @return Whether occlusion culling is enabled
         */
        bool isOcclusionCullingEnabled() const;

    private:
        std::shared_ptr<Grid> m_grid;                                      ///< The grid to render
        std::shared_ptr<Rendering::Material> m_material;                   ///< Material for rendering
        std::unordered_map<ChunkCoord, std::shared_ptr<ChunkMesh>, ChunkCoordHash> m_chunkMeshes; ///< Chunk meshes
        std::unordered_map<ChunkCoord, uint64_t, ChunkCoordHash> m_renderableIds;          ///< Renderable IDs
        std::vector<ChunkCoord> m_visibleChunks;                           ///< Currently visible chunks
        std::vector<float> m_lodLevels;                                    ///< LOD distance levels
        float m_renderDistance;                                            ///< Maximum render distance
        bool m_occlusionCullingEnabled;                                    ///< Whether occlusion culling is enabled
        size_t m_renderedChunkCount;                                       ///< Number of chunks rendered
        size_t m_totalTriangleCount;                                       ///< Total number of triangles
        bool m_initialized;                                                ///< Whether the renderer is initialized

        /**
         * @brief Update visible chunks
         */
        void updateVisibleChunks();

        /**
         * @brief Register chunk meshes with the render system
         */
        void registerChunkMeshes();

        /**
         * @brief Create a renderable from a chunk mesh
         * @param coord The chunk coordinate
         * @param mesh The chunk mesh
         * @return The created renderable ID
         */
        uint64_t createRenderable(const ChunkCoord& coord, std::shared_ptr<ChunkMesh> mesh);

        /**
         * @brief Unregister all renderables
         */
        void unregisterAllRenderables();

        /**
         * @brief Ensure all visible chunks with meshes are registered as renderables
         */
        void ensureVisibleChunksRegistered();
    };

} // namespace PixelCraft::Voxel
