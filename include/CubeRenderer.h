#pragma once

#include "CubeGrid.h"
#include "Frustum.h"
#include <Shader.h>
#include <glad/glad.h>
#include <vector>

class Application; // Forward declaration

class CubeRenderer
{
private:
    // Core rendering objects
    unsigned int cubeVAO, cubeVBO, cubeEBO;
    unsigned int instanceMatrixVBO, instanceColorVBO;

    // Grid and application references
    CubeGrid* grid;
    Application* app;

    // Rendering parameters
    float maxViewDistance;
    bool enableFrustumCulling;

    // Stats
    int visibleCubeCount;

public:
    CubeRenderer(CubeGrid* cubeGrid, Application* application);
    ~CubeRenderer();

    void initialize();
    void render(Shader& shader);
    void renderShadowMap(Shader& depthShader);

    // Settings
    void setMaxViewDistance(float distance) { maxViewDistance = distance; }
    void setEnableFrustumCulling(bool enable) { enableFrustumCulling = enable; }

    // Stats
    int getVisibleCubeCount() const { return visibleCubeCount; }
};