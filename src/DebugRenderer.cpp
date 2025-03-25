#include "DebugRenderer.h"
#include "CubeGrid.h"
#include <gtc/matrix_transform.hpp>

DebugRenderer::DebugRenderer()
    : quadVAO(0), quadVBO(0), lineVAO(0), lineVBO(0),
      debugShader(nullptr), lineShader(nullptr),
      showChunkBoundaries(false), showBoundingBoxes(false), showGridLines(false)
{
    
}

DebugRenderer::~DebugRenderer()
{
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteVertexArrays(1, &lineVAO);
    glDeleteBuffers(1, &lineVBO);

    delete debugShader;
    delete lineShader;
}

void DebugRenderer::initialize()
{
    // Setup buffers for the quad
    float quadVertices[] = {
        // positions        // texture coords
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    // Setup buffers for lines
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));

    // Initialize debug shaders
    debugShader = new Shader("shaders/DebugVertexShader.glsl", "shaders/DebugFragmentShader.glsl");
    lineShader = new Shader("shaders/LineVertexShader.glsl", "shaders/LineFragmentShader.glsl");

    // Initialize line buffer for grids and boundaries
    updateLineMeshes();
}

void DebugRenderer::updateLineMeshes()
{
    // This will be called when the grid or visualization settings changge
    std::vector<float> lineData;

    // Generate lines for grid visualization if enabled
    if (showGridLines)
    {
        // TBD: Generate grid lines based on active grid bounds
    }

    // Reserve space for chunk boundaries - will be populated during rendering
    chunkBoundaryLines.clear();

    // Upload initial data
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, lineData.size() * sizeof(float), lineData.data(), GL_DYNAMIC_DRAW);
}

// Render a texture to the screen (for debug purposes)
void DebugRenderer::renderDebugTexture(GLuint textureID, float x, float y, float width, float height, bool isDepthTexture)
{
    debugShader->use();
    debugShader->setBool("isDepthTexture", isDepthTexture);

    // Setup viewport transform to render in a corner of the screen
    glViewport(x, y, width, height);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    debugShader->setInt("debugTexture", 0);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void DebugRenderer::renderChunkBoundaries(const glm::mat4& view, const glm::mat4& projection, const CubeGrid* grid)
{
    if (!showChunkBoundaries) return;

    // Get all active chunks
    const auto& chunks = grid->getChunks();

    // Clear previous chunk lines
    chunkBoundaryLines.clear();

    // Line color for chunk boundaries
    glm::vec3 chunkColor(1.0f, 0.5f, 0.0f); // Orange

    // Generate lines for each chunk
    for (const auto& pair : chunks)
    {
        const glm::ivec3& chunkPos = pair.first;
        GridChunk* chunk = pair.second;

        // Skip inactive chunks
        if (!chunk->isActive()) continue;

        // Calculate chunk bounds in world coordinates
        float spacing = grid->getSpacing();
        float chunkSize = GridChunk::CHUNK_SIZE * spacing;

        glm::vec3 minCorner(
            chunkPos.x * chunkSize,
            chunkPos.y * chunkSize,
            chunkPos.z * chunkSize
        );

        glm::vec3 maxCorner = minCorner + glm::vec3(chunkSize);

        // Add lines for the 12 edges of the chunk's bounding box
        addBoxToLines(minCorner, maxCorner, chunkColor, chunkBoundaryLines);
    }

    // Upload chunk boundary lines to GPU
    if (!chunkBoundaryLines.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, chunkBoundaryLines.size() * sizeof(float),
                     chunkBoundaryLines.data(), GL_DYNAMIC_DRAW);

        // Render lines
        lineShader->use();
        lineShader->setMat4("view", view);
        lineShader->setMat4("projection", projection);

        glBindVertexArray(lineVAO);
        glLineWidth(1.5f); // Thicker lines for better visibility
        glDrawArrays(GL_LINES, 0, chunkBoundaryLines.size() / 6);
        glLineWidth(1.0f); // Reset line width
        glBindVertexArray(0);
    }
}

void DebugRenderer::renderBoundingBoxes(const glm::mat4& view, const glm::mat4& projection,
                                        const std::vector<std::pair<glm::vec3, glm::vec3>>& boxes)
{
    if (!showBoundingBoxes || boxes.empty()) return;

    // Generate lines for all bounding boxes
    std::vector<float> boxLines;

    for (const auto& box : boxes)
    {
        const glm::vec3& minCorner = box.first;
        const glm::vec3& maxCorner = box.second;

        // Different color for bounding boxes
        glm::vec3 boxColor(0.0f, 1.0f, 1.0f); // Cyan

        addBoxToLines(minCorner, maxCorner, boxColor, boxLines);
    }

    // Upload and render
    if (!boxLines.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, boxLines.size() * sizeof(float),
                     boxLines.data(), GL_DYNAMIC_DRAW);

        // Render lines
        lineShader->use();
        lineShader->setMat4("view", view);
        lineShader->setMat4("projection", projection);

        glBindVertexArray(lineVAO);
        glLineWidth(1.0f);
        glDrawArrays(GL_LINES, 0, boxLines.size() / 6);
        glBindVertexArray(0);
    }
}

void DebugRenderer::renderGridLines(const glm::mat4& view, const glm::mat4& projection,
                                    const glm::ivec3& minBounds, const glm::ivec3& maxBounds,
                                    float spacing)
{
    if (!showGridLines) return;

    // Generate grid lines
    std::vector<float> gridLines;
    glm::vec3 gridColor(0.5f, 0.5f, 0.5f); // Gray

    // X-axis lines
    for (int z = minBounds.z; z <= maxBounds.z; z++)
    {
        for (int y = minBounds.y; y <= maxBounds.y; y++)
        {
            glm::vec3 start(minBounds.x * spacing, y * spacing, z * spacing);
            glm::vec3 end(maxBounds.x * spacing, y * spacing, z * spacing);

            // Add line: start vertex, color, end vertex, color
            gridLines.push_back(start.x);
            gridLines.push_back(start.y);
            gridLines.push_back(start.z);
            gridLines.push_back(gridColor.r);
            gridLines.push_back(gridColor.g);
            gridLines.push_back(gridColor.b);

            gridLines.push_back(end.x);
            gridLines.push_back(end.y);
            gridLines.push_back(end.z);
            gridLines.push_back(gridColor.r);
            gridLines.push_back(gridColor.g);
            gridLines.push_back(gridColor.b);
        }
    }

    // Y-axis lines
    for (int z = minBounds.z; z <= maxBounds.z; z++)
    {
        for (int x = minBounds.x; x <= maxBounds.x; x++)
        {
            glm::vec3 start(x * spacing, minBounds.y * spacing, z * spacing);
            glm::vec3 end(x * spacing, maxBounds.y * spacing, z * spacing);

            // Add line: start vertex, color, end vertex, color
            gridLines.push_back(start.x);
            gridLines.push_back(start.y);
            gridLines.push_back(start.z);
            gridLines.push_back(gridColor.r);
            gridLines.push_back(gridColor.g);
            gridLines.push_back(gridColor.b);

            gridLines.push_back(end.x);
            gridLines.push_back(end.y);
            gridLines.push_back(end.z);
            gridLines.push_back(gridColor.r);
            gridLines.push_back(gridColor.g);
            gridLines.push_back(gridColor.b);
        }
    }

    // Z-axis lines
    for (int y = minBounds.y; y <= maxBounds.y; y++)
    {
        for (int x = minBounds.x; x <= maxBounds.x; x++)
        {
            glm::vec3 start(x * spacing, y * spacing, minBounds.z * spacing);
            glm::vec3 end(x * spacing, y * spacing, maxBounds.z * spacing);

            // Add line: start vertex, color, end vertex, color
            gridLines.push_back(start.x);
            gridLines.push_back(start.y);
            gridLines.push_back(start.z);
            gridLines.push_back(gridColor.r);
            gridLines.push_back(gridColor.g);
            gridLines.push_back(gridColor.b);

            gridLines.push_back(end.x);
            gridLines.push_back(end.y);
            gridLines.push_back(end.z);
            gridLines.push_back(gridColor.r);
            gridLines.push_back(gridColor.g);
            gridLines.push_back(gridColor.b);
        }
    }

    // Upload and render
    if (!gridLines.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, gridLines.size() * sizeof(float),
                     gridLines.data(), GL_DYNAMIC_DRAW);

        // Render with alpha blending for less visual clutter
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        lineShader->use();
        lineShader->setMat4("view", view);
        lineShader->setMat4("projection", projection);

        glBindVertexArray(lineVAO);
        glLineWidth(0.5f); // Thin lines for grid
        glDrawArrays(GL_LINES, 0, gridLines.size() / 6);
        glBindVertexArray(0);

        glDisable(GL_BLEND);
    }
}

void DebugRenderer::setShowChunkBoundaries(bool show)
{
    showChunkBoundaries = show;
}

void DebugRenderer::setShowBoundingBoxes(bool show)
{
    showBoundingBoxes = show;
}

void DebugRenderer::setShowGridLines(bool show)
{
    showGridLines = show;
    // Update mesh if enabling grid lines
    if (show)
    {
        updateLineMeshes();
    }
}

void DebugRenderer::addBoxToLines(const glm::vec3& minCorner, const glm::vec3& maxCorner,
                                  const glm::vec3& color, std::vector<float>& lines)
{
    // Bottom face
    addLineToBuffer(minCorner.x, minCorner.y, minCorner.z,
                    maxCorner.x, minCorner.y, minCorner.z, color, lines);
    addLineToBuffer(maxCorner.x, minCorner.y, minCorner.z,
                    maxCorner.x, minCorner.y, maxCorner.z, color, lines);
    addLineToBuffer(maxCorner.x, minCorner.y, maxCorner.z,
                    minCorner.x, minCorner.y, maxCorner.z, color, lines);
    addLineToBuffer(minCorner.x, minCorner.y, maxCorner.z,
                    minCorner.x, minCorner.y, minCorner.z, color, lines);

    // Top face
    addLineToBuffer(minCorner.x, maxCorner.y, minCorner.z,
                    maxCorner.x, maxCorner.y, minCorner.z, color, lines);
    addLineToBuffer(maxCorner.x, maxCorner.y, minCorner.z,
                    maxCorner.x, maxCorner.y, maxCorner.z, color, lines);
    addLineToBuffer(maxCorner.x, maxCorner.y, maxCorner.z,
                    minCorner.x, maxCorner.y, maxCorner.z, color, lines);
    addLineToBuffer(minCorner.x, maxCorner.y, maxCorner.z,
                    minCorner.x, maxCorner.y, minCorner.z, color, lines);

    // Connecting lines
    addLineToBuffer(minCorner.x, minCorner.y, minCorner.z,
                    minCorner.x, maxCorner.y, minCorner.z, color, lines);
    addLineToBuffer(maxCorner.x, minCorner.y, minCorner.z,
                    maxCorner.x, maxCorner.y, minCorner.z, color, lines);
    addLineToBuffer(maxCorner.x, minCorner.y, maxCorner.z,
                    maxCorner.x, maxCorner.y, maxCorner.z, color, lines);
    addLineToBuffer(minCorner.x, minCorner.y, maxCorner.z,
                    minCorner.x, maxCorner.y, maxCorner.z, color, lines);
}

void DebugRenderer::addLineToBuffer(float x1, float y1, float z1,
                                    float x2, float y2, float z2,
                                    const glm::vec3& color, std::vector<float>& buffer)
{
    // Vertex 1
    buffer.push_back(x1);
    buffer.push_back(y1);
    buffer.push_back(z1);
    buffer.push_back(color.r);
    buffer.push_back(color.g);
    buffer.push_back(color.b);

    // Vertex 2
    buffer.push_back(x2);
    buffer.push_back(y2);
    buffer.push_back(z2);
    buffer.push_back(color.r);
    buffer.push_back(color.g);
    buffer.push_back(color.b);
}