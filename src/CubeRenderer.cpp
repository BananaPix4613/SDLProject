#include "CubeRenderer.h"
#include "Application.h"
#include "ext/matrix_transform.hpp"

CubeRenderer::CubeRenderer(CubeGrid *cubeGrid, Application* application)
    : grid(cubeGrid), app(application)
{
    
}

CubeRenderer::~CubeRenderer()
{
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
}

void CubeRenderer::initialize()
{
    float cubeVertices[] = {
        // positions          // normals           // texture coords
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,

         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f
    };

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Texture coords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void CubeRenderer::render(Shader &shader)
{
    shader.use();

    // Reset visible cube counter in the Application
    int visibleCount = 0;

    // Loop through the grid and render active cubes
    const auto& gridData = grid->getGrid();
    for (int x = 0; x < grid->getSize(); x++)
    {
        for (int y = 0; y < grid->getSize(); y++)
        {
            for (int z = 0; z < grid->getSize(); z++)
            {
                const Cube& cube = gridData[x][y][z];

                if (cube.active)
                {
                    // Check if the cube is visible before rendering
                    if (!app->isFrustumCullingEnabled() || isCubeVisible(x, y, z))
                    {
                        // Set model matrix for this cube
                        glm::mat4 model = glm::mat4(1.0f);
                        model = glm::translate(model, cube.position);
                        model = glm::scale(model, glm::vec3(1.0f)); // Uniform scaling

                        shader.setMat4("model", model);
                        shader.setVec3("color", cube.color);

                        // Draw the cube
                        glBindVertexArray(cubeVAO);
                        glDrawArrays(GL_TRIANGLES, 0, 36);

                        // Increment visible cube counter
                        visibleCount++;
                    }
                }
            }
        }
    }

    // Update the Application's visible cube count variable
    app->setVisibleCubeCount(visibleCount);

    glBindVertexArray(0);
}

void CubeRenderer::renderDepthOnly(Shader& depthShader)
{
    // For shadow mapping, we may want to disable frustum culling
    // or use the light's frustum instead
    // For simplicity, we'll just render all cubes for the shadow pass

    for (int x = 0; x < grid->getSize(); x++)
    {
        for (int y = 0; y < grid->getSize(); y++)
        {
            for (int z = 0; z < grid->getSize(); z++)
            {
                if (grid->isCubeActive(x, y, z))
                {
                    glm::vec3 position = grid->getCube(x, y, z).position;
                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, position);
                    model = glm::scale(model, glm::vec3(1.0f)); // Cube size

                    depthShader.setMat4("model", model);

                    // Draw the cube (only positions needed for depth)
                    glBindVertexArray(cubeVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }
        }
    }
}

bool CubeRenderer::isCubeVisible(int x, int y, int z) const
{
    return app->isCubeVisible(x, y, z);
}
