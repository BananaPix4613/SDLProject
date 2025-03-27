#include "VoxelObject.h"
#include <gtc/matrix_transform.hpp>
#include <iostream>

VoxelObject::VoxelObject(CubeGrid* grid)
    : grid(grid)
{
    initializeCubeMesh();
}

VoxelObject::~VoxelObject()
{
    // Clean up cube mesh resources
    if (cubeMeshVao) glDeleteVertexArrays(1, &cubeMeshVao);
    if (cubeMeshVbo) glDeleteBuffers(1, &cubeMeshVbo);
    if (cubeMeshEbo) glDeleteBuffers(1, &cubeMeshEbo);

    // ChunkRenderData destructors will clean up their resources
    chunkRenderData.clear();
}

void VoxelObject::initializeCubeMesh()
{
    // Cube vertices with position, normal, and texture coordinates
    float vertices[] = {
        // positions           // normals          // tex coords
        -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,  0.0f, 0.0f, // front face
         0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,  0.0f, 1.0f,

        -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,  0.0f, 0.0f, // back face
         0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,  0.0f, 1.0f,

        -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // left face
        -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

         0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // right face
         0.5f,  0.5f, -0.5f,   1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,   1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,  0.0f, 1.0f, // bottom face
         0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,  0.0f, 0.0f,

        -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // top face
         0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,  0.0f, 0.0f
    };

    // Indices for all 6 faces (36 indices, 12 triangles)
    unsigned int indices[] = {
        2, 1, 0, 0, 3, 2,       // front face
        4, 5, 6, 6, 7, 4,       // back face
        8, 9, 10, 10, 11, 8,    // left face
        14, 13, 12, 12, 15, 14, // right face
        16, 17, 18, 18, 19, 16, // bottom face
        22, 21, 20, 20, 23, 22  // top face
    };

    cubeIndexCount = 36; // 6 faces * 2 triangles * 3 vertices

    // Create and bind VAO
    glGenVertexArrays(1, &cubeMeshVao);
    glGenBuffers(1, &cubeMeshVbo);
    glGenBuffers(1, &cubeMeshEbo);

    glBindVertexArray(cubeMeshVao);

    // Set up vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, cubeMeshVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Set up element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeMeshEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

    // Texture coordinate attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    // Unbind VAO
    glBindVertexArray(0);
}

void VoxelObject::prepare(RenderContext& context)
{
    if (!grid) return;

    // Reset statistics
    visibleChunks = 0;
    visibleCubes = 0;
    totalCubes = 0;

    // Get camera position for distance culling
    glm::vec3 cameraPos = context.camera->getPosition();

    // Get all chunks from the grid
    const auto& chunks = grid->getChunks();

    // Process each chunk
    for (const auto& pair : chunks)
    {
        const glm::ivec3& chunkPos = pair.first;
        GridChunk* chunk = pair.second;

        // Skip inactive chunks
        if (!chunk->isActive()) continue;

        // Get or create render data for this chunk
        auto it = chunkRenderData.find(chunkPos);
        if (it == chunkRenderData.end())
        {
            auto [newIt, inserted] = chunkRenderData.emplace(
                chunkPos, std::make_unique<ChunkRenderData>(chunkPos));
            it = newIt;
            it->second->dirty = true; // New chunk needs update
        }

        ChunkRenderData* renderData = it->second.get();

        // Update the chunk's rendering data if needed
        if (renderData->dirty)
        {
            updateChunkData(renderData, chunk);

            // Update bounding sphere for frustum culling
            // Calculate center of chunk in world coordinates
            float chunkSize = GridChunk::CHUNK_SIZE * grid->getSpacing();
            renderData->center = glm::vec3(
                chunkPos.x * chunkSize + chunkSize * 0.5f,
                chunkPos.y * chunkSize + chunkSize * 0.5f,
                chunkPos.z * chunkSize + chunkSize * 0.5f
            );

            // Calculate radius as half of the diagonal
            renderData->radius = chunkSize * 0.866f; // sqrt(3)/2 * chunkSize
        }

        // Check if the chunk visible
        if (isChunkVisible(renderData, context))
        {
            visibleChunks++;
            visibleCubes += renderData->instanceCount;
        }

        totalCubes += renderData->instanceCount;
    }

    // Optional: Clean up chunks that no longer exist in the grid
    std::vector<glm::ivec3> chunksToRemove;

    for (const auto& pair : chunkRenderData)
    {
        const glm::ivec3& pos = pair.first;
        if (chunks.find(pos) == chunks.end())
        {
            chunksToRemove.push_back(pos);
        }
    }

    for (const auto& pos : chunksToRemove)
    {
        chunkRenderData.erase(pos);
    }
}

void VoxelObject::render(RenderContext& context)
{
    if (!grid || !material) return;

    // Bind shared properties for all chunks
    Material* currentMaterial = context.overrideMaterial ? context.overrideMaterial : material;
    currentMaterial->bind();

    // Set common uniforms
    Shader* shader = currentMaterial->getShader();
    if (shader)
    {
        shader->setMat4("view", context.viewMatrix);
        shader->setMat4("projection", context.projectionMatrix);

        // Set shadow mapping parameters if needed
        if (context.enableShadows && context.shadowMapTexture)
        {
            shader->setBool("enableShadows", true);
            shader->setMat4("lightSpaceMatrix", context.lightSpaceMatrix);

            // Bind shadow map texture
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, context.shadowMapTexture);
            shader->setInt("shadowMap", 0);
        }
        else
        {
            shader->setBool("enableShadows", false);
        }
    }

    // Prepare for instanced rendering
    glBindVertexArray(cubeMeshVao);

    // Render each visible chunk
    for (const auto& pair : chunkRenderData)
    {
        ChunkRenderData* renderData = pair.second.get();

        // Skip empty chunks
        if (renderData->instanceCount == 0) continue;

        // Skip chunks that are not visible
        if (!isChunkVisible(renderData, context)) continue;

        // Render the chunk
        renderChunk(renderData, context);
    }

    // Clean up state
    glBindVertexArray(0);
}

void VoxelObject::updateChunkData(ChunkRenderData* data, const GridChunk* chunk)
{
    if (!data || !chunk) return;

    // Get chunk position
    const glm::ivec3& chunkPos = chunk->getPosition();

    // Clear previous instance data
    data->instanceMatrices.clear();
    data->instanceColors.clear();

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

                // Calculate model matrix for this cube
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, cube.position);
                model = glm::scale(model, glm::vec3(grid->getSpacing()));

                // Add to instance data
                data->instanceMatrices.push_back(model);
                data->instanceColors.push_back(cube.color);
            }
        }
    }

    // Update instance count
    data->instanceCount = static_cast<int>(data->instanceMatrices.size());

    // Update GPU buffers if we have instances
    if (data->instanceCount > 0)
    {
        // Bind VAO
        glBindVertexArray(data->vao);

        // Set up base vertex attributes using the shared cube mesh
        glBindBuffer(GL_ARRAY_BUFFER, cubeMeshVbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeMeshEbo);

        // Position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

        // Normal attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        
        // Texture coordinate attribute
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

        // Update instance matrix buffer
        glBindBuffer(GL_ARRAY_BUFFER, data->instanceBuffer);

        // Interleave matrices and colors for better cache coherency
        std::vector<float> instanceData;
        instanceData.reserve(data->instanceCount * (16 + 3)); // 16 floats per matrix + 3 floats for color

        for (int i = 0; i < data->instanceCount; i++)
        {
            // Add matrix data (16 floats)
            const glm::mat4& mat = data->instanceMatrices[i];
            for (int row = 0; row < 4; row++)
            {
                for (int col = 0; col < 4; col++)
                {
                    instanceData.push_back(mat[row][col]);
                }
            }

            // Add color data (3 floats)
            const glm::vec3& color = data->instanceColors[i];
            instanceData.push_back(color.r);
            instanceData.push_back(color.g);
            instanceData.push_back(color.b);
        }

        // Upload to GPU
        glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(float), instanceData.data(), GL_DYNAMIC_DRAW);
        
        // Set up instance attributes
        // Matrix columns (4 vec4 attributes)
        for (int i = 0; i < 4; i++)
        {
            glEnableVertexAttribArray(3 + i);
            glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, (16 + 3) * sizeof(float),
                                 (void*)(i * 4 * sizeof(float)));
            glVertexAttribDivisor(3 + i, 1); // Advance per instance
        }

        // Color attribute
        glEnableVertexAttribArray(7);
        glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, (16 + 3) * sizeof(float),
                             (void*)(16 * sizeof(float)));
        glVertexAttribDivisor(7, 1); // Advance per instance

        // Unbind VAO
        glBindVertexArray(0);
    }

    // Mark as updated
    data->dirty = false;
}

void VoxelObject::renderChunk(ChunkRenderData* data, RenderContext& context)
{
    if (!data || data->instanceCount == 0) return;

    // Bind this chunk's VAO with instance data
    glBindVertexArray(data->vao);

    // Draw instanced cubes
    glDrawElementsInstanced(GL_TRIANGLES, cubeIndexCount, GL_UNSIGNED_INT, 0, data->instanceCount);
}

bool VoxelObject::isChunkVisible(const ChunkRenderData* data, const RenderContext& context) const
{
    if (!data) return false;

    // Always visible if frustum culling is disabled
    if (!context.enableFrustumCulling) return true;

    // Distance culling
    if (context.camera)
    {
        float distance = glm::distance(data->center, context.camera->getPosition());
        if (distance > renderDistance) return false;
    }

    // Frustum culling
    return !context.frustum.isSphereOutside(data->center, data->radius);
}

void VoxelObject::markChunkDirty(const glm::ivec3& chunkPos)
{
    auto it = chunkRenderData.find(chunkPos);
    if (it != chunkRenderData.end())
    {
        it->second->dirty = true;
    }
}

void VoxelObject::markAllChunksDirty()
{
    for (auto& pair : chunkRenderData)
    {
        pair.second->dirty = true;
    }
}

void VoxelObject::onGridChanged()
{
    // When grid changes significantly, mark all chunks dirty
    markAllChunksDirty();
}