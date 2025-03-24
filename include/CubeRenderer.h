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
    // Cube mesh data
    unsigned int cubeVAO, cubeVBO, cubeEBO;

    // Instancing data
    unsigned int instanceMatrixVBO, instanceColorVBO;
    std::vector<glm::mat4> instanceMatrices;
    std::vector<glm::vec3> instanceColors;
    unsigned int instanceCount;
    unsigned int maxInstances;

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