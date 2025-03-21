#pragma once

#include "CubeGrid.h"
#include "Frustum.h"
#include <Shader.h>
#include <glad/glad.h>

class Application; // Forward declaration

class CubeRenderer
{
private:
    unsigned int cubeVAO, cubeVBO;
    CubeGrid* grid;
    Application* app; // Reference to the application for frustum culling

public:
    CubeRenderer(CubeGrid* cubeGrid, Application* application);
    ~CubeRenderer();

    void initialize();
    void render(Shader& shader);
    void renderDepthOnly(Shader& depthShader);

    // Helper method for frustum culling
    bool isCubeVisible(int x, int y, int z) const;
};