#include "CubeRenderer.h"
#include "Application.h"
#include "ext/matrix_transform.hpp"

CubeRenderer::CubeRenderer(CubeGrid *cubeGrid, Application* application)
    : grid(cubeGrid), app(application), maxInstances(10000), instanceCount(0)
{
    
}

CubeRenderer::~CubeRenderer()
{
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &cubeEBO);
    glDeleteBuffers(1, &instanceMatrixVBO);
    glDeleteBuffers(1, &instanceColorVBO);
}

void CubeRenderer::initialize()
{
    // Cube vertices with reduced data - eliminating duplicates
    float cubeVertices[] = {
        // positions           // normals          // tex coords
        -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,  0.0f, 0.0f, // 0
         0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,  1.0f, 0.0f, // 1
         0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,  1.0f, 1.0f, // 2
        -0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,  0.0f, 1.0f, // 3

        -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,  0.0f, 0.0f, // 4
         0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,  1.0f, 0.0f, // 5
         0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,  1.0f, 1.0f, // 6
        -0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,  0.0f, 1.0f, // 7

        -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 8 (same as 7)
        -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 9 (same as 3)
        -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 10 (same as 0)
        -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 11 (same as 4)

         0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 12 (same as 6)
         0.5f,  0.5f, -0.5f,   1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 13 (same as 2)
         0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 14 (same as 1)
         0.5f, -0.5f,  0.5f,   1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 15 (same as 5)

        -0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,  0.0f, 1.0f, // 16 (same as 0)
         0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,  1.0f, 1.0f, // 17 (same as 1)
         0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,  1.0f, 0.0f, // 18 (same as 5)
        -0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,  0.0f, 0.0f, // 19 (same as 4)

        -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 20 (same as 3)
         0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,  1.0f, 1.0f, // 21 (same as 2)
         0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 22 (same as 6)
        -0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,  0.0f, 0.0f  // 23 (same as 7)
    };

    // Indices for the cube faces
    unsigned int indices[] = {
        2, 1, 0, 0, 3, 2,       // Front face
        4, 5, 6, 6, 7, 4,       // Back face
        8, 9, 10, 10, 11, 8,    // Left face
        14, 13, 12, 12, 15, 14, // Right face
        16, 17, 18, 18, 19, 16, // Bottom face
        22, 21, 20, 20, 23, 22  // Top face
    };

    // Create VAO, VBO and EBO
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glGenBuffers(1, &cubeEBO);

    glBindVertexArray(cubeVAO);

    // Bind VBO and upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    // Bind EBO and upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Texture coords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Create buffers for instance data
    instanceMatrices.resize(maxInstances);
    instanceColors.resize(maxInstances);

    // Create VBO for model matrices (one per instance)
    glGenBuffers(1, &instanceMatrixVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceMatrixVBO);
    glBufferData(GL_ARRAY_BUFFER, maxInstances * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);

    // Model matrix (4 vec4s)
    // Layout for instanced model matrix
    for (unsigned int i = 0; i < 4; i++)
    {
        glEnableVertexAttribArray(3 + i);
        glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
            (void*)(sizeof(glm::vec4) * i));
        glVertexAttribDivisor(3 + i, 1); // Tell OpenGL this is an instanced attribute
    }

    // Create VBO for colors (one per instance)
    glGenBuffers(1, &instanceColorVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceColorVBO);
    glBufferData(GL_ARRAY_BUFFER, maxInstances * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

    // Color attribute
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glVertexAttribDivisor(7, 1); // Tell OpenGL this is an instanced attribute

    glBindVertexArray(0);
}

void CubeRenderer::render(Shader &shader)
{
    shader.use();

    // Reset instance count
    instanceCount = 0;

    // Determine if we're using instanced rendering based on shader name
    // This is a simple way to check - could be improved with a dedicated flag
    bool usingInstanced = shader.ID == app->getInstancedShaderID();

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
                    // Check if the cube is visible before adding to instance
                    if (!app->isFrustumCullingEnabled() || isCubeVisible(x, y, z))
                    {
                        if (usingInstanced)
                        {
                            // Instanced rendering path
                            if (instanceCount < maxInstances)
                            {
                                // Create model matrix for this cube
                                glm::mat4 model = glm::mat4(1.0f);
                                model = glm::translate(model, cube.position);
                                model = glm::scale(model, glm::vec3(1.0f)); // Uniform scaling

                                // Add to instance data
                                instanceMatrices[instanceCount] = model;
                                instanceColors[instanceCount] = cube.color;
                                instanceCount++;
                            }
                        }
                        else
                        {
                            // Non-instanced rendering path (legacy mode)
                            glm::mat4 model = glm::mat4(1.0f);
                            model = glm::translate(model, cube.position);
                            model = glm::scale(model, glm::vec3(1.0f));

                            shader.setMat4("model", model);
                            shader.setVec3("color", cube.color);

                            glBindVertexArray(cubeVAO);
                            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
                        }
                    }
                }
            }
        }
    }

    // If we have instances to render
    if (instanceCount > 0 && usingInstanced)
    {
        // Update instance data
        glBindBuffer(GL_ARRAY_BUFFER, instanceMatrixVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, instanceCount * sizeof(glm::mat4), instanceMatrices.data());
        
        glBindBuffer(GL_ARRAY_BUFFER, instanceColorVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, instanceCount * sizeof(glm::vec3), instanceColors.data());

        // Draw all cubes in one call
        glBindVertexArray(cubeVAO);
        glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, instanceCount);
        glBindVertexArray(0);
    }

    // Update the Application's visible cube count
    app->setVisibleCubeCount(instanceCount);
}

void CubeRenderer::renderDepthOnly(Shader& depthShader)
{
    // For shadow mapping pass, we'll use the same instancing approach
    // but we might want to include all cubes or use a different frustum

    // Determine if we're using instanced rendering based on shader name
    bool usingInstanced = depthShader.ID == app->getInstancedShadowShaderID();

    // Reset instance count
    instanceCount = 0;

    // Loop through all active cubes and prepare for rendering
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
                    if (usingInstanced)
                    {
                        // Instanced rendering path
                        if (instanceCount < maxInstances)
                        {
                            // Create model matrix for this cube
                            glm::mat4 model = glm::mat4(1.0f);
                            model = glm::translate(model, cube.position);
                            model = glm::scale(model, glm::vec3(1.0f));

                            // Add to instance data
                            instanceMatrices[instanceCount] = model;
                            instanceCount++;
                        }
                    }
                    else
                    {
                        // Non-instanced rendering path
                        glm::mat4 model = glm::mat4(1.0f);
                        model = glm::translate(model, cube.position);
                        model = glm::scale(model, glm::vec3(1.0f));

                        depthShader.setMat4("model", model);

                        glBindVertexArray(cubeVAO);
                        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
                    }
                }
            }
        }
    }

    // If we have instances to render and we're using instanced rendering
    if (instanceCount > 0 && usingInstanced)
    {
        // Update instance matrices
        glBindBuffer(GL_ARRAY_BUFFER, instanceMatrixVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, instanceCount * sizeof(glm::mat4), instanceMatrices.data());

        // Draw all cubes in one instanced call
        depthShader.use();
        glBindVertexArray(cubeVAO);
        glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, instanceCount);
        glBindVertexArray(0);
    }
}

bool CubeRenderer::isCubeVisible(int x, int y, int z) const
{
    return app->isCubeVisible(x, y, z);
}
