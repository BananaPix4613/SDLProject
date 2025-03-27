#pragma once

#include "RenderSystem.h"
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <vector>
#include <memory>

// Forward declarations
class Camera;

// Shadow rendering stage
class ShadowStage : public RenderStage
{
public:
    ShadowStage() : RenderStage("Shadow") {}

    void initialize() override
    {
        // Initialize shadow map resources
        if (!shadowMapTarget)
        {
            shadowMapTarget = std::make_unique<RenderTarget>(2048, 2048);
        }

        // Create shadow mapping shader
        shadowShader = new Shader("shaders/ShadowVert.glsl", "shaders/ShadowFrag.glsl");
    }

    void execute(RenderContext& context) override
    {
        if (!shadowMapTarget) return;

        // Calculate light position and view
        glm::vec3 lightDir = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
        glm::vec3 lightPos = -lightDir * 30.0f; // Position far away in the opposite direction
        glm::vec3 lightTarget = glm::vec3(0.0f);

        // Calculate light view matrix
        glm::mat4 lightView = glm::lookAt(lightPos, lightTarget, glm::vec3(0.0f, 1.0f, 0.0f));

        // Calculate orthographic projection for the light
        float orthoSize = 40.0f;
        glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize,
                                               -orthoSize, orthoSize,
                                               1.0f, 100.0f);

        // Combined light space matrix
        context.lightSpaceMatrix = lightProjection * lightView;

        // Bind shadow map framebuffer
        shadowMapTarger->bind();
        glClear(GL_DEPTH_BUFFER_BIT);

        // Configure for shadow map rendering
        glCullFace(GL_FRONT); // Avoid peter-panning
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(2.0f, 4.0f);

        // Set up shadow shader
        shadowShader->use();
        shadowShader->setMat4("lightSpaceMatrix", context.lightSpaceMatrix);

        // Render shadow-casting objects
        // This would typically iterate though scene objects
        // that cast shadows and render them with the shadow shader
        
        // Reset state
        glCullFace(GL_BACK);
        glDisable(GL_POLYGON_OFFSET_FILL);

        // Store shadow map texture for use in other stages
        context.shadowMapTexture = shadowMapTarget->getDepthTexture();
    }

    void setShadowMapResolution(int resolution)
    {
        if (shadowMapTarget)
        {
            shadowMapTarget->resize(resolution, resolution);
        }
    }

private:
    std::unique_ptr<RenderTarget> shadowMapTarget;
    Shader* shadowShader = nullptr;
};

// Skybox rendering stage
class SkyboxStage : public RenderStage
{
public:
    SkyboxStage() : RenderStage("Skybox") {}

    void initialize() override
    {
        // Initialize skybox resources
        initializeSkyboxMesh();
        initializeSkyboxTexture();

        // Create skybox shader
        skyboxShader = new Shader("shaders/SkyboxVert.glsl", "shaders/SkyboxFrag.glsl");
    }

    void execute(RenderContext& context) override
    {
        if (!skyboxShader || skyboxVAO == 0) return;

        // Save depth state
        GLboolean depthFunc;
        glGetBooleanv(GL_DEPTH_FUNC, &depthFunc);

        // Chance depth function to less-equal for skybox
        glDepthFunc(GL_LEQUAL);

        // Set up skybox shader
        skyboxShader->use();
        skyboxShader->setMat4("view", glm::mat4(glm::mat3(context.viewMatrix))); // Remove translation
        skyboxShader->setMat4("projection", context.projectionMatrix);

        // Bind skybox texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
        skyboxShader->setInt("skybox", 0);

        // Render skybox cube
        glBindVertexArray(skyboxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        // Restore depth state
        glDepthFunc(depthFunc);
    }

private:
    unsigned int skyboxVAO = 0;
    unsigned int skyboxVBO = 0;
    unsigned int skyboxTexture = 0;
    Shader* skyboxShader = nullptr;

    void initializeSkyboxMesh()
    {
        float skyboxVertices[] = {
            // positions          
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f
        };

        // Create VAO, VBO
        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);

        glBindVertexarray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

        glBindVertexArray(0);
    }

    void initializeSkyboxTexture()
    {
        // This is a placeholder - in a real implementation, you would load
        // actual skybox textures from files
        glGenTextures(1, &skyboxTexture);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);

        // Generate a default skybox (blue gradient)
        const int size = 64;
        unsigned char* data = new unsigned char[size * size * 3];

        // Fill with blue gradient
        for (int y = 0; y < size; y++)
        {
            for (int x = 0; x < size; x++)
            {
                int idx = (y * size + x) * 3;
                float t = (float)y / size;

                // Sky blue to darker blue gradient
                data[idx] = static_cast<unsigned char>(173 * (1.0f - t));
                data[idx + 1] = static_cast<unsigned char>(216 * (1.0f - t * 0.5f));
                data[idx + 2] = static_cast<unsigned char>(230 * (1.0f - t * 0.3f));
            }
        }

        // Upload same texture to all 6 faces
        for (unsigned int i = 0; i < 6; i++)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                         size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        }

        delete[] data;

        // Set filtering parameters
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
};

// Debug visualization stage for wireframes, bounds, etc.
class DebugStage : public RenderStage
{
public:
    DebugStage() : RenderStage("Debug") {}

    void initialize() override
    {
        // Create line shader for debug drawing
        lineShader = new Shader("shaders/LineVert.glsl", "shaders/LineFrag.glsl");

        // Set up debug drawing resources
        initializeDebugResources();
    }

    void execute(RenderContext& context) override
    {
        if (!lineShader) return;

        // Skip if debug visualization is disabled
        if (!context.showWireframe && !showFrustumWireframe &&
            !showGrid && !showChunkBoundaries)
        {
            return;
        }

        // Set up line shader
        lineShader->use();
        lineShader->setMat4("view", context.viewMatrix);
        lineShader->setMat4("projection", context.projectionMatrix);

        // Render frustum wireframe if enabled
        if (showFrustumWireframe)
        {
            renderFrustumWireframe(context);
        }

        // Render grid if enabled
        if (showGrid)
        {
            renderGrid(context);
        }

        // Render chunk boundaries if enabled
        if (showChunkBoundaries)
        {
            renderChunkBoundaries(context);
        }

        // Render boudning boxes if enabled
        if (showBoundingBoxes && !boundingBoxes.empty())
        {
            renderBoundingBoxes(context);
        }
    }

    // Settings
    void setShowFrustumWireframe(bool show) { showFrustumWireframe = show; }
    void setShowGrid(bool show) { showGrid = show; }
    void setShowChunkBoundaries(bool show) { showChunkBoundaries = show; }
    void setShowBoundingBoxes(bool show) { showBoundingBoxes = show; }

    // Add a bounding box to render
    void addBoundingBox(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f))
    {
        BoundingBox box;
        box.min = min;
        box.max = max;
        box.color = color;
        boundingBoxes.push_back(box);
    }

    // Clear all bounding boxes
    void clearBoundingBoxes()
    {
        boundingBoxes.clear();
    }

    // Set grid parameters
    void setGridParameters(const glm::ivec3& min, const glm::ivec3& max, float spacing)
    {
        gridMinBounds = min;
        gridMaxBounds = max;
        gridSpacing = spacing;
        gridNeedsUpdate = true;
    }

    // Set chunk data for rendering boundaries
    void setChunkData(const std::unorderered_map<glm::ivec3, GridChunk*, Vec3Hash>& chunks, float spacing)
    {
        // Store chunks reference
        this->setChunkData = &chunks;
        this->chunkSpacing = spacing;
        chunkBoundariesNeedUpdate = true;
    }

private:
    // Shader for debug visualization
    Shader* lineShader = nullptr;

    // Configuration flags
    bool showFrustumWireframe = false;
    bool showGrid = false;
    bool showChunkBoundaries = false;
    bool showBoundingBoxes = false;

    // OpenGL resources
    unsigned int lineVAO = 0;
    unsigned int lineVBO = 0;

    // Grid data
    glm::ivec3 gridMinBounds = glm::ivec3(-10, 0, -10);
    glm::ivec3 gridMaxBounds = glm::ivec3(10, 10, 10);
    float gridSpacing = 1.0f;
    bool gridNeedsUpdate = true;

    // Bounding box data
    struct BoundingBox
    {
        glm::vec3 min;
        glm::vec3 max;
        glm::vec3 color;
    };
    std::vector<BoundingBox> boundingBoxes;

    // Line buffers
    std::vector<float> frustumLineBuffer;
    std::vector<float> gridLineBuffer;
    std::vector<float> chunkBoundaryLineBuffer;
    std::vector<float> boundingBoxLineBuffer;

    void initializeDebugResources()
    {
        // Create VAO, VBO for lines
        glGenVertexArrays(1, &lineVAO);
        glGenBuffers(1, &lineVBO);

        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);

        // Position attribute (3 floats)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

        // Color attribute (3 floats)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

        glBindVertexArray(0);
    }

    void renderFrustumWireframe(const RenderContext& context)
    {
        // Extract frustum corners from view-projection matrix
        std::vector<glm::vec3> corners = getFrustumCorners(context.viewMatrix, context.projectionMatrix);

        // Build line buffer for frustum wireframe
        frustumLineBuffer.clear();

        // Define line segments for the frustum (8 corners, 12 edges)
        // Lines for near face
        addLineToBuffer(corners[0], corners[1], glm::vec3(1.0f, 0.0f, 0.0f), frustumLineBuffer);
        addLineToBuffer(corners[1], corners[2], glm::vec3(1.0f, 0.0f, 0.0f), frustumLineBuffer);
        addLineToBuffer(corners[2], corners[3], glm::vec3(1.0f, 0.0f, 0.0f), frustumLineBuffer);
        addLineToBuffer(corners[3], corners[0], glm::vec3(1.0f, 0.0f, 0.0f), frustumLineBuffer);

        // Lines for far face
        addLineToBuffer(corners[4], corners[5], glm::vec3(1.0f, 0.0f, 0.0f), frustumLineBuffer);
        addLineToBuffer(corners[5], corners[6], glm::vec3(1.0f, 0.0f, 0.0f), frustumLineBuffer);
        addLineToBuffer(corners[6], corners[7], glm::vec3(1.0f, 0.0f, 0.0f), frustumLineBuffer);
        addLineToBuffer(corners[7], corners[4], glm::vec3(1.0f, 0.0f, 0.0f), frustumLineBuffer);

        // Lines connected near and far faces
        addLineToBuffer(corners[0], corners[4], glm::vec3(1.0f, 0.0f, 0.0f), frustumLineBuffer);
        addLineToBuffer(corners[1], corners[5], glm::vec3(1.0f, 0.0f, 0.0f), frustumLineBuffer);
        addLineToBuffer(corners[2], corners[6], glm::vec3(1.0f, 0.0f, 0.0f), frustumLineBuffer);
        addLineToBuffer(corners[3], corners[7], glm::vec3(1.0f, 0.0f, 0.0f), frustumLineBuffer);

        // Upload and render
        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, frustumLineBuffer.size() * sizeof(float), frustumLineBuffer.data(), GL_DYNAMIC_DRAW);

        glLineWidth(2.0f); // Thicker lines for frustum
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(frustumLineBuffer.size() / 6));
        glLineWidth(1.0f); // Reset line width
    }

    void renderGrid(const RenderContext& context)
    {
        // Update grid lines if needed
        if (gridNeedsUpdate)
        {
            updateGridLines();
            gridNeedsUpdate = false;
        }

        // Render grid lines
        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, gridLineBuffer.size() * sizeof(float), gridLineBuffer.data(), GL_DYNAMIC_DRAW);

        glLineWidth(1.0f);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(gridLineBuffer.size() / 6));
    }

    void renderChunkBoundaries(const RenderContext& context)
    {
        // Update chunk boundaries if needed
        if (chunkBoundariesNeedUpdate)
        {
            updateChunkBoundaries();
            chunkBoundariesNeedUpdate = false;
        }

        // Render chunk boundaries
        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, chunkBoundaryLineBuffer.size() * sizeof(float), chunkBoundaryLineBuffer.data(), GL_DYNAMIC_DRAW);

        glLineWidth(1.5f); // Slightly thicker lines for chunk boundaries
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(chunkBoundaryLineBuffer.size() / 6));
        glLineWidth(1.0f); // Reset line width
    }

    void renderBoundingBoxes(const RenderContext& context)
    {
        // Update bounding box lines
        updateBoundingBoxLines();

        // Render bounding boxes
        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, boundingBoxLineBuffer.size() * sizeof(float), boundingBoxLineBuffer.data(), GL_DYNAMIC_DRAW);

        glLineWidth(1.0f);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(boudningBoxLineBuffer.size() / 6));
    }

    // Helper to update grid lines
    void updateGridLines()
    {
        gridLineBuffer.clear();
        glm::vec3 gridColor(0.5f, 0.5f, 0.5f); // Gray

        // X-axis lines (along Z)
        for (int y = gridMinBounds.y; y <= gridMaxBounds.y; y++)
        {
            for (int x = gridMinBounds.x; x <= gridMaxBounds.x; x++)
            {
                glm::vec3 start(x * gridSpacing, y * gridSpacing, gridMinBounds.z * gridSpacing);
                glm::vec3 end(x * gridSpacing, y * gridSpacing, gridMaxBounds.z * gridSpacing);
                addLineToBuffer(start, end, gridColor, gridLinebuffer);
            }
        }

        // Z-axis lines (along X)
        for (int y = gridMinBounds.y; y <= gridMaxBounds.y; y++)
        {
            for (int z = gridMinBounds.z; z <= gridMaxBounds.z; z++)
            {
                glm::vec3 start(gridMinBounds.x * gridSpacing, y * gridSpacing, z * gridSpacing);
                glm::vec3 end(gridMaxBounds.x * gridSpacing, y * gridSpacing, z * gridSpacing);
                addLineToBuffer(start, end, gridColor, gridLinebuffer);
            }
        }

        // Y-axis lines (vertical)
        for (int x = gridMinBounds.x; x <= gridMaxBounds.x; x++)
        {
            for (int z = gridMinBounds.z; z <= gridMaxBounds.z; z++)
            {
                glm::vec3 start(x * gridSpacing, gridMinBounds.y * gridSpacing, z * gridSpacing);
                glm::vec3 end(x * gridSpacing, gridMaxBounds.y * gridSpacing, z * gridSpacing);
                addLineToBuffer(start, end, gridColor, gridLinebuffer);
            }
        }
    }

    // Helper to update chunk boundaries
    void updateChunkBoundaries()
    {
        chunkBoundaryLineBuffer.clear();

        if (!chunks) return;

        // Line color for chunk boundaries
        glm::vec3 chunkColor(1.0f, 0.5f, 0.0f); // Orange

        // Generate lines for each chunk
        for (const auto& pair : *chunks)
        {
            const glm::ivec3& chunkPos = pair.first;
            GridChunk* chunk = pair.second;

            // Skip inactive chunks
            if (!chunk->isActive()) continue;

            // Calculate chunk bounds in world coordinates
            float chunkSize = GridChunk::CHUNK_SIZE * chunkSpacing;

            glm::vec3 minCorner(
                chunkPos.x * chunkSize,
                chunkPos.y * chunkSize,
                chunkPos.z * chunkSize
            );

            glm::vec3 maxCorner = minCorner + glm::vec3(chunkSize);

            // Add lines for the chunk's bounding box (12 edges)
            addBoxToLines(minCorner, maxCorner, chunkColor, chunkBoundaryLineBuffer);
        }
    }

    // Helper to udpate bounding box lines
    void updateBoundingBoxLines()
    {
        boundingBoxLineBuffer.clear();

        for (const auto& box : boundingBoxes)
        {
            addBoxToLines(box.min, box.max, box.color, boundingBoxLineBuffer);
        }
    }

    // Helper to extract frustum corners from matrices
    std::vector<glm::vec3> getFrustumCorners(const glm::mat4& view, const glm::mat4 projection)
    {
        std::vector<glm::vec3> corners(8);

        // Get the inverse of the view-projection matrix
        glm::mat4 invViewProj = glm::inverse(projection * view);

        // Define corners in NDC space (-1 to 1 cube)
        const glm::vec4 corners_ndc[8] = {
            // Near plane
            glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f), // Bottom left
            glm::vec4(1.0f, -1.0f, -1.0f, 1.0f),  // Bottom right
            glm::vec4(1.0f, 1.0f, -1.0f, 1.0f),   // Top right
            glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f),  // Top left

            // Far plane
            glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),  // Bottom left
            glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),   // Bottom right
            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),    // Top right
            glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f)    // Top left
        };

        // Transform to world space
        for (int i = 0; i < 8; i++)
        {
            glm::vec4 worldPos = invViewProj * corners_ndc[i];
            worldPos /= worldPos.w; // Perspective divide
            corners[i] = glm::vec3(worldPos);
        }

        return corners;
    }

    // Helper to add a box to the line buffer
    void addBoxToLines(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color, std::vector<float>& buffer)
    {
        // Bottom face
        addLineToBuffer(
            glm::vec3(min.x, min.y, min.z),
            glm::vec3(max.x, min.y, min.z),
            color, buffer);
        addLineToBuffer(
            glm::vec3(max.x, min.y, min.z),
            glm::vec3(max.x, min.y, max.z),
            color, buffer);
        addLineToBuffer(
            glm::vec3(max.x, min.y, max.z),
            glm::vec3(min.x, min.y, max.z),
            color, buffer);
        addLineToBuffer(
            glm::vec3(min.x, min.y, max.z),
            glm::vec3(min.x, min.y, min.z),
            color, buffer);

        // Top face
        addLineToBuffer(
            glm::vec3(min.x, max.y, min.z),
            glm::vec3(max.x, max.y, min.z),
            color, buffer);
        addLineToBuffer(
            glm::vec3(max.x, max.y, min.z),
            glm::vec3(max.x, max.y, max.z),
            color, buffer);
        addLineToBuffer(
            glm::vec3(max.x, max.y, max.z),
            glm::vec3(min.x, max.y, max.z),
            color, buffer);
        addLineToBuffer(
            glm::vec3(min.x, max.y, max.z),
            glm::vec3(min.x, max.y, min.z),
            color, buffer);

        // Connecting edges
        addLineToBuffer(
            glm::vec3(min.x, min.y, min.z),
            glm::vec3(min.x, max.y, min.z),
            color, buffer);
        addLineToBuffer(
            glm::vec3(max.x, min.y, min.z),
            glm::vec3(max.x, max.y, min.z),
            color, buffer);
        addLineToBuffer(
            glm::vec3(max.x, min.y, max.z),
            glm::vec3(max.x, max.y, max.z),
            color, buffer);
        addLineToBuffer(
            glm::vec3(min.x, min.y, max.z),
            glm::vec3(min.x, max.y, max.z),
            color, buffer);
    }

    // Helper to add a line to the buffer
    void addLineToBuffer(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, std::vector<float>& buffer)
    {
        // Start point
        buffer.push_back(start.x);
        buffer.push_back(start.y);
        buffer.push_back(start.z);
        buffer.push_back(color.r);
        buffer.push_back(color.g);
        buffer.push_back(color.b);

        // End point
        buffer.push_back(end.x);
        buffer.push_back(end.y);
        buffer.push_back(end.z);
        buffer.push_back(color.r);
        buffer.push_back(color.g);
        buffer.push_back(color.b);
    }
};