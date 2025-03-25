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

    // Initialize chunk instance caches
    updateChunkInstanceCache();
}

void CubeRenderer::updateChunkInstanceCache()
{
    // Reset counter
    chunksUpdatedThisFrame = 0;

    // Clear existing cache if needed
    chunkInstanceCache.clear();

    // Loop through all chunks and create cached instance data
    const auto& chunks = grid->getChunks();
    for (const auto& pair : chunks)
    {
        GridChunk* chunk = pair.second;
        const glm::ivec3& chunkPos = chunk->getPosition();

        // Skip inactive chunks
        if (!chunk->isActive()) continue;

        // Create instance data for this chunk
        ChunkInstanceData cacheData;

        // Loop through all cubes in this chunk
        for (int x = 0; x < GridChunk::CHUNK_SIZE; x++)
        {
            for (int y = 0; y < GridChunk::CHUNK_SIZE; y++)
            {
                for (int z = 0; z < GridChunk::CHUNK_SIZE; z++)
                {
                    const Cube& cube = chunk->getCube(x, y, z);

                    if (cube.active)
                    {
                        // Calculate model matrix
                        glm::mat4 model = glm::mat4(1.0f);
                        model = glm::translate(model, cube.position);
                        model = glm::scale(model, glm::vec3(1.0f));

                        // Add to instance data cache
                        cacheData.matrices.push_back(model);
                        cacheData.colors.push_back(cube.color);
                    }
                }
            }
        }

        // Only add chunks with active cubes
        if (!cacheData.matrices.empty())
        {
            chunkInstanceCache[chunkPos] = cacheData;
            chunksUpdatedThisFrame++; // Count this chunk as updated
        }
    }
}

void CubeRenderer::render(Shader &shader)
{
    shader.use();

    // Reset instance count and visible cube tracking
    instanceCount = 0;

    // Reset visible cube count at the beginning of render
    app->setVisibleCubeCount(0);

    // Determine if we're using instanced rendering based on shader name
    bool usingInstanced = shader.ID == app->getInstancedShaderID();

    if (!usingInstanced)
    {
        // Legacy non-instanced rendering - less efficient for large worlds
        renderLegacy(shader);
        return;
    }

    // Efficient chunk-based instanced rendering
    renderChunked(shader);
}

void CubeRenderer::renderLegacy(Shader& shader)
{
    // For backward compatibility - render entire grid without chunks
    // This is less efficient, so should be a fallback option

    const auto& chunks = grid->getChunks();
    int totalCubes = 0;

    for (const auto& pair : chunks)
    {
        GridChunk* chunk = pair.second;
        const glm::ivec3& chunkPos = chunk->getPosition();

        // Skip processing if chunk is inactive
        if (!chunk->isActive()) continue;

        // Calculate global grid coordinates for this chunk
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
                    // Global grid coordinates
                    int gridX = baseX + x;
                    int gridY = baseY + y;
                    int gridZ = baseZ + z;

                    const Cube& cube = chunk->getCube(x, y, z);

                    if (cube.active)
                    {
                        // Skip if outside frustum
                        if (app->isFrustumCullingEnabled() && !app->isCubeVisible(gridX, gridY, gridZ))
                        {
                            continue;
                        }

                        totalCubes++;

                        // Set up model matrix
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

    // Update stats
    app->setVisibleCubeCount(totalCubes);
}

void CubeRenderer::renderChunked(Shader& shader)
{
    // Efficient chunk-based rendering
    instanceCount = 0;

    // Option 1: Use the chunk instance cache
    if (useInstanceCache)
    {
        // If cache needs update
        if (cacheNeedsUpdate)
        {
            updateChunkInstanceCache();
            cacheNeedsUpdate = false;
        }

        // Get camera position to determine chunk visibility
        glm::vec3 cameraPos = app->getCameraPosition();

        // Frustum culling at chunk level
        const Frustum& frustum = app->getViewFrustum();

        // Loop through cached chunks
        for (const auto& pair : chunkInstanceCache)
        {
            const glm::ivec3& chunkPos = pair.first;
            const ChunkInstanceData& chunkData = pair.second;

            // Skip empty chunks
            if (chunkData.matrices.empty()) continue;

            // Calculate chunk center position
            glm::vec3 chunkCenter(
                (chunkPos.x * GridChunk::CHUNK_SIZE + GridChunk::CHUNK_SIZE / 2) * grid->getSpacing(),
                (chunkPos.y * GridChunk::CHUNK_SIZE + GridChunk::CHUNK_SIZE / 2) * grid->getSpacing(),
                (chunkPos.z * GridChunk::CHUNK_SIZE + GridChunk::CHUNK_SIZE / 2) * grid->getSpacing()
            );

            // Check distance to chunk
            float distanceToChunk = glm::distance(chunkCenter, cameraPos);

            // Skip chunks beyond view distance
            if (distanceToChunk > maxViewDistance) continue;

            // Check if chunk is in frustum (approximate with bounding box)
            float chunkSize = GridChunk::CHUNK_SIZE * grid->getSpacing();
            if (app->isFrustumCullingEnabled())
            {
                glm::vec3 chunkMin = chunkCenter - glm::vec3(chunkSize / 2);
                glm::vec3 chunkMax = chunkCenter + glm::vec3(chunkSize / 2);

                if (frustum.isBoxOutside(chunkMin, chunkMax))
                {
                    continue;
                }
            }

            // This chunk is visible - add its instances
            int chunkInstanceCount = chunkData.matrices.size();

            // Skip if there's no room for more instances
            if (instanceCount + chunkInstanceCount > maxInstances)
            {
                // Render current batch
                renderCurrentBatch(shader);
                instanceCount = 0;
            }

            // Add chunk instances to batch
            for (int i = 0; i < chunkInstanceCount; i++)
            {
                instanceMatrices[instanceCount] = chunkData.matrices[i];
                instanceColors[instanceCount] = chunkData.colors[i];
                instanceCount++;
            }
        }
    }
    else
    {
        // Option 2: Direct rendering from chunks (more flexible but potentially slower)
        const auto& chunks = grid->getChunks();

        for (const auto& pair : chunks)
        {
            GridChunk* chunk = pair.second;
            const glm::ivec3& chunkPos = chunk->getPosition();

            // Skip inactive chunks
            if (!chunk->isActive()) continue;

            // Calculate chunk center position
            glm::vec3 chunkCenter(
                (chunkPos.x * GridChunk::CHUNK_SIZE + GridChunk::CHUNK_SIZE / 2) * grid->getSpacing(),
                (chunkPos.y * GridChunk::CHUNK_SIZE + GridChunk::CHUNK_SIZE / 2) * grid->getSpacing(),
                (chunkPos.z * GridChunk::CHUNK_SIZE + GridChunk::CHUNK_SIZE / 2) * grid->getSpacing()
            );

            // Skip chunks beyond view distance
            float distanceToChunk = glm::distance(chunkCenter, app->getCameraPosition());
            if (distanceToChunk > maxViewDistance) continue;

            // Frustum culling at chunk level
            if (app->isFrustumCullingEnabled())
            {
                float chunkSize = GridChunk::CHUNK_SIZE * grid->getSpacing();
                glm::vec3 chunkMin = chunkCenter - glm::vec3(chunkSize / 2);
                glm::vec3 chunkMax = chunkCenter + glm::vec3(chunkSize / 2);

                if (app->getViewFrustum().isBoxOutside(chunkMin, chunkMax))
                {
                    continue;
                }
            }

            // Calculate global grid coordinates for this chunk
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
                        // Global grid coordinates
                        int gridX = baseX + x;
                        int gridY = baseY + y;
                        int gridZ = baseZ + z;

                        const Cube& cube = chunk->getCube(x, y, z);

                        if (cube.active && instanceCount < maxInstances)
                        {
                            // Skip outside frustum - optional per-cube culling
                            if (app->isFrustumCullingEnabled() && enablePerCubeCulling &&
                                !app->isCubeVisible(gridX, gridY, gridZ))
                            {
                                continue;
                            }

                            // Create model matrix for this cube
                            glm::mat4 model = glm::mat4(1.0f);
                            model = glm::translate(model, cube.position);
                            model = glm::scale(model, glm::vec3(1.0f));

                            // Add to instance data
                            instanceMatrices[instanceCount] = model;
                            instanceColors[instanceCount] = cube.color;
                            instanceCount++;

                            // If we've reached the batch limit, render current batch
                            if (instanceCount >= batchSize)
                            {
                                renderCurrentBatch(shader);
                                instanceCount = 0;
                            }
                        }
                    }
                }
            }
        }
    }

    // Render any remaining instances
    if (instanceCount > 0)
    {
        renderCurrentBatch(shader);
    }
}

void CubeRenderer::renderCurrentBatch(Shader& shader)
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

    // Add to the running total of visible cubes (accumulate)
    app->setVisibleCubeCount(app->getVisibleCubeCount() + instanceCount);
}

void CubeRenderer::renderDepthOnly(Shader& depthShader)
{
    // Modify to use chunks for shadow mapping
    // The approach is similar to render() but simplified for depth-only

    // Determine if we're using instanced rendering based on shader ID
    bool usingInstanced = depthShader.ID == app->getInstancedShadowShaderID();

    // Reset instance count
    instanceCount = 0;

    // Get all active chunks
    const auto& chunks = grid->getChunks();

    if (usingInstanced)
    {
        // Use instance rendering for shadow mapping
        for (const auto& pair : chunks)
        {
            GridChunk* chunk = pair.second;

            // Skip inactive chunks
            if (!chunk->isActive()) continue;

            // Process cubes in this chunk
            for (int x = 0; x < GridChunk::CHUNK_SIZE; x++)
            {
                for (int y = 0; y < GridChunk::CHUNK_SIZE; y++)
                {
                    for (int z = 0; z < GridChunk::CHUNK_SIZE; z++)
                    {
                        const Cube& cube = chunk->getCube(x, y, z);

                        if (cube.active && instanceCount < maxInstances)
                        {
                            // Create model matrix for this cube
                            glm::mat4 model = glm::mat4(1.0f);
                            model = glm::translate(model, cube.position);
                            model = glm::scale(model, glm::vec3(1.0f));

                            // Add to instance data
                            instanceMatrices[instanceCount] = model;
                            instanceCount++;

                            // If we've reached batch limit, render current batch
                            if (instanceCount >= batchSize)
                            {
                                // Update instance matrices
                                glBindBuffer(GL_ARRAY_BUFFER, instanceMatrixVBO);
                                glBufferSubData(GL_ARRAY_BUFFER, 0, instanceCount * sizeof(glm::mat4), instanceMatrices.data());

                                // Draw all cubes in batch
                                depthShader.use();
                                glBindVertexArray(cubeVAO);
                                glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, instanceCount);
                                glBindVertexArray(0);

                                // Reset for next batch
                                instanceCount = 0;
                            }
                        }
                    }
                }
            }
        }
        // Render any remaining instances
        if (instanceCount > 0)
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
    else
    {
        // Non-instanced rendering path (legacy)
        for (const auto& pair : chunks)
        {
            GridChunk* chunk = pair.second;

            // Skip inactive chunks
            if (!chunk->isActive()) continue;

            // Process cubes in this chunk
            for (int x = 0; x < GridChunk::CHUNK_SIZE; x++)
            {
                for (int y = 0; y < GridChunk::CHUNK_SIZE; y++)
                {
                    for (int z = 0; z < GridChunk::CHUNK_SIZE; z++)
                    {
                        const Cube& cube = chunk->getCube(x, y, z);

                        if (cube.active)
                        {
                            // Set up model matrix
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
    }
}

void CubeRenderer::markCacheForUpdate()
{
    cacheNeedsUpdate = true;
}

void CubeRenderer::setRenderSettings(bool useCache, bool perCubeCulling, float viewDist, int batch)
{
    useInstanceCache = useCache;
    enablePerCubeCulling = perCubeCulling;
    maxViewDistance = viewDist;
    batchSize = batch;
}

bool CubeRenderer::isCubeVisible(int x, int y, int z) const
{
    return app->isCubeVisible(x, y, z);
}
