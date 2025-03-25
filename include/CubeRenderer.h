#pragma once

#include "CubeGrid.h"
#include "Frustum.h"
#include <Shader.h>
#include <glad/glad.h>
#include <vector>
#include <unordered_map>

class Application; // Forward declaration

// Cached instance data for a chunk
struct ChunkInstanceData {
    std::vector<glm::mat4> matrices;
    std::vector<glm::vec3> colors;
};

class CubeRenderer
{
private:
    // Cube mesh data
    unsigned int cubeVAO, cubeVBO, cubeEBO;

    // Instancing data
    unsigned int instanceMatrixVBO, instanceColorVBO;
    std::vector<glm::mat4> instanceMatrices;
    std::vector<glm::vec3> instanceColors;
    unsigned int instanceCount;
    unsigned int maxInstances;

    // Chunk instance cache for faster rendering
    std::unordered_map<glm::ivec3, ChunkInstanceData, Vec3Hash, Vec3Equal> chunkInstanceCache;
    bool cacheNeedsUpdate = true;

    // Rendering options
    bool useInstanceCache = true;
    bool enablePerCubeCulling = true;
    float maxViewDistance = 500.0f;
    int batchSize = 10000; // Number of instances to render in one batch

    // Debug info
    int chunksUpdatedThisFrame = 0;

    CubeGrid* grid;
    Application* app; // Reference to the application for frustum culling

    // Private render methods
    void renderLegacy(Shader& shader);
    void renderChunked(Shader& shader);
    void renderCurrentBatch(Shader& shader);

public:
    CubeRenderer(CubeGrid* cubeGrid, Application* application);
    ~CubeRenderer();

    void initialize();
    void render(Shader& shader);
    void renderDepthOnly(Shader& depthShader);

    // Cache management
    void updateChunkInstanceCache();
    void markCacheForUpdate();

    // Settings
    void setRenderSettings(bool useCache, bool perCubeCulling, float viewDist, int batch);

    int getChunkUpdatesThisFrame() const { return chunksUpdatedThisFrame; }

    // Helper method for frustum culling
    bool isCubeVisible(int x, int y, int z) const;
};