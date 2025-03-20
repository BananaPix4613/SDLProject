#pragma once

#include "CubeGrid.h"
#include <Shader.h>
#include <glad/glad.h>

class CubeRenderer
{
private:
    unsigned int cubeVAO, cubeVBO;
    CubeGrid* grid;

public:
    CubeRenderer(CubeGrid* cubeGrid);
    ~CubeRenderer();

    void initialize();
    void render(Shader& shader);
    void renderDepthOnly(Shader& depthShader);
};