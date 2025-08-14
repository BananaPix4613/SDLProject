// -------------------------------------------------------------------------
// VoxelRenderer.cpp
// -------------------------------------------------------------------------
#include "Voxel/VoxelRenderer.h"
#include "Voxel/ChunkMesh.h"
#include "Voxel/Chunk.h"
#include "Voxel/Grid.h"
#include "Voxel/ChunkManager.h"
#include "Core/Logger.h"
#include "Core/Application.h"
#include "Rendering/Material.h"
#include "Rendering/Renderable.h"
#include "Rendering/RenderSystem.h"
#include "Rendering/Cameras/Camera.h"
#include "Rendering/Cameras/OrthographicCamera.h"
#include "Utility/Frustum.h"
#include "Utility/AABB.h"

namespace PixelCraft::Voxel
{

    VoxelRenderer::VoxelRenderer()
        : m_renderDistance(100.0f)
        , m_occlusionCullingEnabled(false)
        , m_renderedChunkCount(0)
        , m_totalTriangleCount(0)
        , m_initialized(false)
    {
        // Set default LOD levels
        m_lodLevels = {32.0f, 64.0f, 128.0f};
    }

    VoxelRenderer::~VoxelRenderer()
    {
        if (m_initialized)
        {
            shutdown();
        }
    }

    bool VoxelRenderer::initialize()
    {
        if (m_initialized)
        {
            Core::warn("VoxelRenderer already initialized");
            return true;
        }

        Core::info("Initializing VoxelRenderer subsystem");

        // Create default material if none exists
        if (!m_material)
        {
            auto renderSystem = Core::Application::getInstance()->getSubsystem<Rendering::RenderSystem>();
            if (renderSystem)
            {
                m_material = renderSystem->createMaterial("DefaultVoxelMaterial");
                if (m_material)
                {
                    // Try to get existing shader from ShaderManager
                    auto shader = renderSystem->getShader("voxel_basic");
                    if (shader)
                    {
                        // Set basic shader properties for testing
                        m_material->setShader(shader);
                        m_material->setBaseColor(glm::vec3(0.8f, 0.8f, 0.8f));
                    }
                }
            }
        }

        // Ensure any existing chunks are registered as renderables
        if (m_grid)
        {
            ensureVisibleChunksRegistered();
        }

        m_initialized = true;
        return true;
    }

    void VoxelRenderer::update(float deltaTime)
    {
        (void)deltaTime;  // Currently unused

        if (!m_initialized || !m_grid)
        {
            return;
        }

        // Only update visible chunks occasionally, not every frame
        static int updateCounter = 0;
        if (updateCounter++ % 60 == 0)  // Update every 60 frames (~1 second at 60fps)
        {
            updateVisibleChunks();
        }

        // Update stats
        m_renderedChunkCount = m_chunkMeshes.size();
        m_totalTriangleCount = 0;

        // Calculate total triangle count
        for (const auto& [coord, mesh] : m_chunkMeshes)
        {
            if (mesh)
            {
                m_totalTriangleCount += mesh->getIndexCount() / 3;
            }
        }
    }

    void VoxelRenderer::render()
    {
        if (!m_initialized || !m_grid)
        {
            return;
        }
        
        auto renderSystem = Core::Application::getInstance()->getSubsystem<Rendering::RenderSystem>();
        if (!renderSystem)
        {
            return;
        }

        // Ensure all visible chunks with meshes are registered as renderables
        ensureVisibleChunksRegistered();

        // Log stats periodically
        static int frameCount = 0;
        if (++frameCount % 120 == 0)  // Every 2 seconds
        {
            Core::info("VoxelRenderer: " + std::to_string(m_renderableIds.size()) +
                      " chunks registered, " + std::to_string(m_chunkMeshes.size()) +
                      " cached meshes, " + std::to_string(m_totalTriangleCount) + " triangles");
        }

        // Log first few frames for debugging
        if (frameCount <= 5)
        {
            Core::info("VoxelRenderer: render() called - frame " + std::to_string(frameCount));
        }
    }

    void VoxelRenderer::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        Core::info("Shutting down VoxelRenderer subsystem");

        // Unregister all renderables first
        unregisterAllRenderables();

        // Clear material reference early
        if (m_material)
        {
            Core::debug("VoxelRenderer: Releasing material reference during shutdown");
            m_material.reset();
        }

        // Clear cached data
        m_chunkMeshes.clear();
        m_visibleChunks.clear();
        m_renderableIds.clear();

        // Clear grid reference
        m_grid.reset();

        m_initialized = false;
    }

    std::vector<std::string> VoxelRenderer::getDependencies() const
    {
        return {"MemoryManager", "RenderSystem"};
    }

    void VoxelRenderer::setGrid(std::shared_ptr<Grid> grid)
    {
        m_grid = grid;

        // Clear cached data when grid changes
        m_chunkMeshes.clear();
        m_visibleChunks.clear();

        // Unregister existing renderables
        unregisterAllRenderables();
    }

    std::shared_ptr<Grid> VoxelRenderer::getGrid() const
    {
        return m_grid;
    }

    void VoxelRenderer::setMaterial(std::shared_ptr<Rendering::Material> material)
    {
        m_material = material;
    }

    std::shared_ptr<Rendering::Material> VoxelRenderer::getMaterial() const
    {
        return m_material;
    }

    void VoxelRenderer::updateChunk(const ChunkCoord& coord)
    {
        if (!m_grid)
        {
            return;
        }

        Core::info("VoxelRenderer: Updating chunk at " + coord.toString());

        // Get chunk from ChunkManager directly
        auto chunk = m_grid->getChunk(coord);
        if (!chunk || !chunk->hasMesh())
        {
            Core::warn("VoxelRenderer: Chunk at " + coord.toString() + " has no mesh");
            return;
        }

        auto chunkMesh = chunk->getMesh();
        if (!chunkMesh)
        {
            Core::warn("VoxelRenderer: Chunk mesh is null for " + coord.toString());
            return;
        }

        // Cache the mesh
        m_chunkMeshes[coord] = chunkMesh;

        // Unregister existing renderable if it exists
        auto it = m_renderableIds.find(coord);
        if (it != m_renderableIds.end())
        {
            auto renderSystem = Core::Application::getInstance()->getSubsystem<Rendering::RenderSystem>();
            if (renderSystem)
            {
                renderSystem->unregisterRenderable(it->second);
                m_renderableIds.erase(it);
                Core::info("VoxelRenderer: Unregistered old renderable for chunk " + coord.toString());
            }
        }

        // Create and register new renderable
        uint64_t id = createRenderable(coord, chunkMesh);
        if (id != 0)
        {
            m_renderableIds[coord] = id;
            Core::info("VoxelRenderer: Registered new renderable for chunk " + coord.toString() + 
                      " with ID " + std::to_string(id));
        }
        else
        {
            Core::error("VoxelRenderer: Failed to create renderable for chunk " + coord.toString());
        }
    }

    void VoxelRenderer::setRenderDistance(float distance)
    {
        m_renderDistance = distance;
    }

    float VoxelRenderer::getRenderDistance() const
    {
        return m_renderDistance;
    }

    void VoxelRenderer::setLODLevels(const std::vector<float>& distances)
    {
        m_lodLevels = distances;
    }

    const std::vector<float>& VoxelRenderer::getLODLevels() const
    {
        return m_lodLevels;
    }

    size_t VoxelRenderer::getVisibleChunkCount() const
    {
        return m_renderedChunkCount;
    }

    size_t VoxelRenderer::getTotalTriangleCount() const
    {
        return m_totalTriangleCount;
    }

    void VoxelRenderer::setOcclusionCullingEnabled(bool enabled)
    {
        m_occlusionCullingEnabled = enabled;
    }

    bool VoxelRenderer::isOcclusionCullingEnabled() const
    {
        return m_occlusionCullingEnabled;
    }

    void VoxelRenderer::updateVisibleChunks()
    {
        if (!m_grid)
        {
            return;
        }

        auto renderSystem = Core::Application::getInstance()->getSubsystem<Rendering::RenderSystem>();
        if (!renderSystem)
        {
            Core::error("VoxelRenderer: Could not get render system subsystem");
            return;
        }

        m_visibleChunks.clear();

        // Get all loaded chunks
        const auto& chunkCoords = m_grid->getLoadedChunkCoords();
        
        for (const auto& coord : chunkCoords)
        {
            auto chunk = m_grid->getChunk(coord);

            if (!chunk)
                continue;

            // TODO: Implement occlusion-based culling
            // Add to visible chunks
            m_visibleChunks.push_back(coord);

            // Cache the mesh if available
            if (chunk->hasMesh() && chunk->getMesh())
            {
                m_chunkMeshes[coord] = chunk->getMesh();
            }
        }

        Core::info("VoxelRenderer: Found " + std::to_string(m_visibleChunks.size()) + " visible chunks");
    }

    void VoxelRenderer::registerChunkMeshes()
    {
        auto renderSystem = Core::Application::getInstance()->getSubsystem<Rendering::RenderSystem>();
        if (!renderSystem)
        {
            return;
        }

        // Register visible chunk meshes with the render system
        for (const auto& coord : m_visibleChunks)
        {
            auto it = m_chunkMeshes.find(coord);
            if (it != m_chunkMeshes.end() && it->second)
            {
                // Create or update renderable
                uint64_t id = createRenderable(coord, it->second);
                if (id != 0)
                {
                    m_renderableIds[coord] = id;
                }
            }
        }
    }

    uint64_t VoxelRenderer::createRenderable(const ChunkCoord& coord, std::shared_ptr<ChunkMesh> mesh)
    {
        auto renderSystem = Core::Application::getInstance()->getSubsystem<Rendering::RenderSystem>();
        if (!renderSystem || !mesh || !m_material)
        {
            Core::error("VoxelRenderer: Cannot create renderable - missing dependencies");
            return 0;
        }

        auto renderMesh = mesh->getRenderMesh();
        if (!renderMesh)
        {
            Core::error("VoxelRenderer: No render mesh available for chunk " + coord.toString());
            return 0;
        }

        // Create a renderable object
        auto renderable = std::make_shared<Rendering::Renderable>();
        renderable->mesh = renderMesh;
        renderable->material = m_material;

        // Set transform based on chunk position
        glm::vec3 worldPos = coord.toWorldPosition(m_grid->getChunkSize());
        renderable->transform = glm::translate(glm::mat4(1.0f), worldPos);

        // Set bounds for culling
        Utility::AABB chunkBounds = mesh->getBounds();
        if (!chunkBounds.isValid())
        {
            // Create a default bounding box if mesh bounds are invalid
            glm::vec3 chunkSize(static_cast<float>(m_grid->getChunkSize()));
            chunkBounds = Utility::AABB(worldPos, worldPos + chunkSize);
            Core::warn("VoxelRenderer: Using default bounds for chunk " + coord.toString());
        }
        renderable->bounds = chunkBounds;

        // Set layer (default for now)
        renderable->layer = static_cast<uint32_t>(Rendering::RenderLayer::Default);

        // Set shadow properties
        renderable->castShadows = true;
        renderable->receiveShadows = true;

        // Register with the renderer
        uint64_t id = renderSystem->registerRenderable(renderable);
        
        if (id == 0)
        {
            Core::error("VoxelRenderer: Failed to register renderable with IsometricRenderSystem");
        }
        
        return id;
    }

    void VoxelRenderer::ensureVisibleChunksRegistered()
    {
        if (!m_grid)
        {
            return;
        }

        // Get all loaded chunks from the grid
        const auto& loadedChunks = m_grid->getLoadedChunkCoords();
        
        for (const auto& coord : loadedChunks)
        {
            // Check if this chunk already has a registered renderable
            if (m_renderableIds.find(coord) != m_renderableIds.end())
            {
                continue; // Already registered
            }

            // Get the chunk and check if it has a mesh
            auto chunk = m_grid->getChunk(coord);
            if (!chunk || !chunk->hasMesh())
            {
                continue; // No mesh to render
            }

            auto chunkMesh = chunk->getMesh();
            if (!chunkMesh)
            {
                continue; // Invalid mesh
            }

            // Cache the mesh
            m_chunkMeshes[coord] = chunkMesh;

            // Create and register the renderable
            uint64_t id = createRenderable(coord, chunkMesh);
            if (id != 0)
            {
                m_renderableIds[coord] = id;
                Core::info("VoxelRenderer: Auto-registered renderable for chunk " + coord.toString() + 
                          " with ID " + std::to_string(id));
            }
        }
    }

    void VoxelRenderer::unregisterAllRenderables()
    {
        auto renderSystem = Core::Application::getInstance()->getSubsystem<Rendering::RenderSystem>();
        if (!renderSystem)
        {
            return;
        }

        // Add exception handling to prevent unhandled exceptions during destruction
        try
        {
            // Unregister all renderables
            for (const auto& pair : m_renderableIds)
            {
                Core::info("Unregistered renderable with ID: " + std::to_string(pair.second));
                renderSystem->unregisterRenderable(pair.second);
            }
        }
        catch (const std::exception& e)
        {
            Core::error("VoxelRenderer: Exception during renderable cleanup: " + std::string(e.what()));
        }
        catch (...)
        {
            Core::error("VoxelRenderer: Unknown exception during renderable cleanup");
        }

        // Clear the map regardless of exceptions
        m_renderableIds.clear();
    }

} // namespace PixelCraft::Voxel
