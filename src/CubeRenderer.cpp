#include "CubeRenderer.h"
#include "Application.h"
#include "ext/matrix_transform.hpp"
#include <iostream>

CubeRenderer::CubeRenderer(CubeGrid *cubeGrid, Application* application)
    : grid(cubeGrid), app(application), maxViewDistance(500.0f),
      enableFrustumCulling(true), visibleCubeCount(0)
{
    
}

CubeRenderer::~CubeRenderer()
{
    // Clean up OpenGL resources
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &cubeEBO);
    glDeleteBuffers(1, &instanceMatrixVBO);
    glDeleteBuffers(1, &instanceColorVBO);
}

void CubeRenderer::initialize()
{
    std::cout << "Initializing cube renderer..." << std::endl;

    // Cube vertices with reduced data - eliminating duplicates
    float vertices[] = {
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

    // Generate and set up VAO, VBO and EBO
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glGenBuffers(1, &cubeEBO);

    // Bind VAO first
    glBindVertexArray(cubeVAO);

    // Set up vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // CRITICAL: Set up element buffer while VAO is bound
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Create instance data buffer
    glGenBuffers(1, &instanceMatrixVBO);
    glGenBuffers(1, &instanceColorVBO);

    // Bind VAO to associate the instance buffers with it
    glBindVertexArray(cubeVAO);

    // Set up matrix attributes - initially with null data, size will be updated in reder
    glBindBuffer(GL_ARRAY_BUFFER, instanceMatrixVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * 100, NULL, GL_DYNAMIC_DRAW); // Reserve space for 100 initially

    // Set up matrix attribute pointers
    for (unsigned int i = 0; i < 4; i++)
    {
        glEnableVertexAttribArray(3 + i);
        glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                              (void*)(sizeof(glm::vec4) * i));
        glVertexAttribDivisor(3 + i, 1);
    }

    // Set up color attributes
    glBindBuffer(GL_ARRAY_BUFFER, instanceColorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 100, NULL, GL_DYNAMIC_DRAW); // Reserve space for 100 initially

    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glVertexAttribDivisor(7, 1);

    // Unbind VAO
    glBindVertexArray(0);
}

void CubeRenderer::render(Shader &shader)
{
    // Start with visible cube count at 0
    visibleCubeCount = 0;

    // Build list of model matrices from grid data
    std::vector<glm::mat4> instanceMatrices;
    std::vector<glm::vec3> instanceColors;

    // Get camera position for distance culling
    glm::vec3 cameraPos = app->getCamera()->getPosition();

    // Process each chunk
    const auto& chunks = grid->getChunks();
    for (const auto& pair : chunks)
    {
        GridChunk* chunk = pair.second;
        const glm::ivec3& chunkPos = chunk->getPosition();

        // Skip inactive chunks
        if (!chunk->isActive()) continue;

        // Calculate grid coordinates for chunk
        int baseX = chunkPos.x * GridChunk::CHUNK_SIZE;
        int baseY = chunkPos.y * GridChunk::CHUNK_SIZE;
        int baseZ = chunkPos.z * GridChunk::CHUNK_SIZE;

        // Process each cube in the chunk
        for (int x = 0; x < GridChunk::CHUNK_SIZE; x++)
        {
            for (int y = 0; y < GridChunk::CHUNK_SIZE; y++)
            {
                for (int z = 0; z < GridChunk::CHUNK_SIZE; z++)
                {
                    const Cube& cube = chunk->getCube(x, y, z);

                    // Skip inactive cubes
                    if (!cube.active) continue;

                    // Distance culling
                    //float distance = glm::distance(cube.position, cameraPos);
                    //if (distance > maxViewDistance) continue;

                    // Skip frustum culling for now since we know that works
                    // if (enableFrustumCulling && !app->isCubeVisible(baseX + x, baseY + y, baseZ + z))

                    // Add to instance data
                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, cube.position);
                    instanceMatrices.push_back(model);
                    instanceColors.push_back(cube.color);

                    // Count visible cubes
                    visibleCubeCount++;
                }
            }
        }
    }

    // Skip rendering if no visible cubes
    if (visibleCubeCount == 0) return;

    //std::cout << "Rendering " << visibleCubeCount << " cubes" << std::endl;

    // CREATE COMPLETELY NEW RESOURCES
    unsigned int testVAO, testVBO, testEBO, instanceMatrixVBO, instanceColorVBO;

    // Create VAO and buffers
    glGenVertexArrays(1, &testVAO);
    glGenBuffers(1, &testVBO);
    glGenBuffers(1, &testEBO);
    glGenBuffers(1, &instanceMatrixVBO);
    glGenBuffers(1, &instanceColorVBO);

    // Bind the VAO first
    glBindVertexArray(testVAO);

    // Cube vertices with reduced data - eliminating duplicates
    float vertices[] = {
        // positions           // normals          // tex coords
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // 0
        0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, // 1
        0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, // 2
        -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, // 3

        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // 4
        0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, // 5
        0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // 6
        -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // 7

        -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // 8 (same as 7)
        -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // 9 (same as 3)
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // 10 (same as 0)
        -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // 11 (same as 4)

        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // 12 (same as 6)
        0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // 13 (same as 2)
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // 14 (same as 1)
        0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // 15 (same as 5)

        -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // 16 (same as 0)
        0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, // 17 (same as 1)
        0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // 18 (same as 5)
        -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, // 19 (same as 4)

        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // 20 (same as 3)
        0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // 21 (same as 2)
        0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // 22 (same as 6)
        -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f  // 23 (same as 7)
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

    // Upload vertices
    glBindBuffer(GL_ARRAY_BUFFER, testVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Upload indices - WHILE VAO IS BOUND
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, testEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Set up vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Upload instance matrices
    glBindBuffer(GL_ARRAY_BUFFER, instanceMatrixVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceMatrices.size() * sizeof(glm::mat4),
                 instanceMatrices.data(), GL_STATIC_DRAW);

    // Set up matrix attributes
    for (unsigned int i = 0; i < 4; i++)
    {
        glEnableVertexAttribArray(3 + i);
        glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                              (void*)(sizeof(glm::vec4) * i));
        glVertexAttribDivisor(3 + i, 1);
    }

    // Upload instance colors
    glBindBuffer(GL_ARRAY_BUFFER, instanceColorVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceColors.size() * sizeof(glm::vec3),
                 instanceColors.data(), GL_STATIC_DRAW);

    // Set up color attribute
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glVertexAttribDivisor(7, 1);

    // Activate shader
    shader.use();
    shader.setMat4("view", app->getCamera()->getViewMatrix());
    shader.setMat4("projection", app->getCamera()->getProjectionMatrix());
    
    //// Bind VAO
    //glBindVertexArray(cubeVAO);

    //// Update instance matrix buffer with actual data
    //glBindBuffer(GL_ARRAY_BUFFER, instanceMatrixVBO);
    //glBufferData(GL_ARRAY_BUFFER, instanceMatrices.size() * sizeof(glm::mat4),
    //             instanceMatrices.data(), GL_STATIC_DRAW);

    //// Update instance color buffer
    //glBindBuffer(GL_ARRAY_BUFFER, instanceColorVBO);
    //glBufferData(GL_ARRAY_BUFFER, instanceColors.size() * sizeof(glm::vec3),
    //             instanceColors.data(), GL_STATIC_DRAW);

    //// Draw all instances
    //glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0, visibleCubeCount);

    // Check for errors
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        std::cout << "OpenGL error during rendering: 0x" << std::hex << err << std::dec << std::endl;
    }

    // Draw the instances
    glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, visibleCubeCount);

    // Clean up
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &testVAO);
    glDeleteBuffers(1, &testVBO);
    glDeleteBuffers(1, &testEBO);
    glDeleteBuffers(1, &instanceMatrixVBO);
    glDeleteBuffers(1, &instanceColorVBO);
}

void CubeRenderer::renderShadowMap(Shader& depthShader)
{
    // Similar implementation to render() but simplified for depth-only
    std::vector<glm::mat4> instanceMatrices;

    // Collect all visible cubes
    const auto& chunks = grid->getChunks();
    for (const auto& pair : chunks)
    {
        GridChunk* chunk = pair.second;
        if (!chunk->isActive()) continue;

        for (int x = 0; x < GridChunk::CHUNK_SIZE; x++)
        {
            for (int y = 0; y < GridChunk::CHUNK_SIZE; y++)
            {
                for (int z = 0; z < GridChunk::CHUNK_SIZE; z++)
                {
                    const Cube& cube = chunk->getCube(x, y, z);
                    if (cube.active)
                    {
                        glm::mat4 model = glm::mat4(1.0f);
                        model = glm::translate(model, cube.position);
                        instanceMatrices.push_back(model);
                    }
                }
            }
        }
    }

    // Skip if no cubes to render
    int instanceCount = static_cast<int>(instanceMatrices.size());
    if (instanceCount == 0) return;

    // Activate shader
    depthShader.use();

    // Create instance matrix buffer
    unsigned int instanceMatrixVBO;
    glGenBuffers(1, &instanceMatrixVBO);

    // Bind VAO
    glBindVertexArray(cubeVAO);

    // Set up instance matrix buffer
    glBindBuffer(GL_ARRAY_BUFFER, instanceMatrixVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceCount * sizeof(glm::mat4),
                 instanceMatrices.data(), GL_STATIC_DRAW);

    // Set matrix attributes
    for (unsigned int i = 0; i < 4; i++)
    {
        glEnableVertexAttribArray(3 + i);
        glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                              (void*)(sizeof(glm::vec4) * i));
        glVertexAttribDivisor(3 + i, 1);
    }

    // Draw all instances
    glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0, instanceCount);

    // Clean up
    glBindVertexArray(0);
    glDeleteBuffers(1, &instanceMatrixVBO);
}
