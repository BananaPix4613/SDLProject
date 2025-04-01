#pragma once

#include "RenderSystem.h"
#include "CubeGrid.h"
#include <glm.hpp>
#include <vector>
#include <unordered_map>

// Cached rendering data for a single voxel chunk
struct ChunkRenderData
{
    unsigned int vao = 0;
    unsigned int vertexBuffer = 0;
    unsigned int instanceBuffer = 0;
    int instanceCount = 0;
    bool dirty = true;
    glm::ivec3 chunkPos;

    // Bounding volume for frustum culling
    glm::vec3 center = glm::vec3(0.0f);
    float radius = 0.0f;

    // Cached data
    std::vector<glm::mat4> instanceMatrices;
    std::vector<glm::vec3> instanceColors;

    // Constructor
    ChunkRenderData(const glm::ivec3& pos)
        : vao(0),
          vertexBuffer(0),
          instanceBuffer(0),
          instanceCount(0),
          dirty(true),
          chunkPos(pos),
          center(0.0f, 0.0f, 0.0f),
          radius(0.0f)
    {
        // Setup VAO/VBO when created
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vertexBuffer);
        glGenBuffers(1, &instanceBuffer);
    }

    // Clean up resources
    ~ChunkRenderData()
    {
        if (vao) glDeleteVertexArrays(1, &vao);
        if (vertexBuffer) glDeleteBuffers(1, &vertexBuffer);
        if (instanceBuffer) glDeleteBuffers(1, &instanceBuffer);
    }

    // Copy/move contructors deleted to avoid OpenGL resource duplication
    ChunkRenderData(const ChunkRenderData&) = delete;
    ChunkRenderData& operator=(const ChunkRenderData&) = delete;

    // Move constructor/assingment are allowed
    ChunkRenderData(ChunkRenderData&& other) noexcept
        : vao(other.vao), vertexBuffer(other.vertexBuffer),
          instanceBuffer(other.instanceBuffer), instanceCount(other.instanceCount),
          dirty(other.dirty), chunkPos(other.chunkPos),
          center(other.center), radius(other.radius),
          instanceMatrices(std::move(other.instanceMatrices)),
          instanceColors(std::move(other.instanceColors))
    {
        other.vao = 0;
        other.vertexBuffer = 0;
        other.instanceBuffer = 0;
    }

    ChunkRenderData& operator=(ChunkRenderData&& other) noexcept
    {
        if (this != &other)
        {
            // Clean up existing resources
            if (vao) glDeleteVertexArrays(1, &vao);
            if (vertexBuffer) glDeleteBuffers(1, &vertexBuffer);
            if (instanceBuffer) glDeleteBuffers(1, &instanceBuffer);

            // Move resources from other
            vao = other.vao;
            vertexBuffer = other.vertexBuffer;
            instanceBuffer = other.instanceBuffer;
            instanceCount = other.instanceCount;
            dirty = other.dirty;
            chunkPos = other.chunkPos;
            center = other.center;
            radius = other.radius;
            instanceMatrices = std::move(other.instanceMatrices);
            instanceColors = std::move(other.instanceColors);

            // Invalidate other's resources
            other.vao = 0;
            other.vertexBuffer = 0;
            other.instanceBuffer = 0;
        }
        return *this;
    }
};

// A renderable voxel grid object
class VoxelObject : public RenderableObject
{
public:
    VoxelObject(CubeGrid* grid);
    ~VoxelObject();

    void prepare(RenderContext& context) override;
    void render(RenderContext& context) override;

    // Set the materials for regular rendering and shadow mapping
    void setMaterial(Material* material) { this->material = material; }
    void setShadowMaterial(Material* material) { this->shadowMaterial = material; }

    // Set render parameters
    void setRenderDistance(float distance) { renderDistance = distance; }
    float getRenderDistance() const { return renderDistance; }

    void setLodEnabled(bool enabled) { lodEnabled = enabled; }
    bool getLodEnabled() const { return lodEnabled; }

    // Mark a specific chunk as needing update
    void markChunkDirty(const glm::ivec3& chunkPos);

    // Mark all chunks as dirty
    void markAllChunksDirty();

    // Called when the voxel grid changes
    void onGridChanged();

    // Get render statistics
    int getTotalChunks() const { return static_cast<int>(chunkRenderData.size()); }
    int getVisibleChunks() const { return visibleChunks; }
    int getVisibleCubes() const { return visibleChunks; }
    bool isChunkVisible(const glm::ivec3& chunkPos, const Frustum& frustum, const glm::vec3& cameraPosition) const;

private:
    // Reference to the voxel data
    CubeGrid* grid;

    // Materials for regular rendering and shadows
    Material* material = nullptr;
    Material* shadowMaterial = nullptr;

    // Mesh data for cube geometry
    unsigned int cubeMeshVao = 0;
    unsigned int cubeMeshVbo = 0;
    unsigned int cubeMeshEbo = 0;
    int cubeIndexCount = 0;

    // Chunk rendering data
    std::unordered_map<glm::ivec3, std::unique_ptr<ChunkRenderData>, Vec3Hash> chunkRenderData;

    // Rendering parameters
    float renderDistance = 500.0f;
    bool lodEnabled = false;

    // Statistics for monitoring
    int visibleChunks = 0;
    int visibleCubes = 0;
    int totalCubes = 0;

    // Initialization
    void initializeCubeMesh();

    // Update chunk data if needed
    void updateChunkData(ChunkRenderData* data, const GridChunk* chunk);

    // Render a single chunk
    void renderChunk(ChunkRenderData* data, RenderContext& context);

    // Calculate chunk visibility
    bool isChunkVisible(const ChunkRenderData* data, const RenderContext& context) const;
};