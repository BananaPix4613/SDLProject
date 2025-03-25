#ifndef DEBUG_RENDERER_H
#define DEBUG_RENDERER_H

#include <glad/glad.h>
#include <glm.hpp>
#include <utility>
#include <vector>
#include "Shader.h"

// Forward declaration
class CubeGrid;

class DebugRenderer
{
public:
    DebugRenderer();
    ~DebugRenderer();

    void initialize();

    // Texture debug view
    void renderDebugTexture(GLuint textureID, float x, float y, float width, float height, bool isDepthTexture = true);

    // Chunk visualization
    void renderChunkBoundaries(const glm::mat4& view, const glm::mat4& projection, const CubeGrid* grid);

    // Bounding box visualization
    void renderBoundingBoxes(const glm::mat4& view, const glm::mat4& projection,
                             const std::vector<std::pair<glm::vec3, glm::vec3>>& boxes);

    // Grid line visualization
    void renderGridLines(const glm::mat4& view, const glm::mat4& projection,
                         const glm::ivec3& minBounds, const glm::ivec3& maxBounds,
                         float spacing);

    // Update graphics resources
    void updateLineMeshes();

    // Visualization
    void setShowChunkBoundaries(bool show);
    void setShowBoundingBoxes(bool show);
    void setShowGridLines(bool show);

private:
    // OpenGL objects
    unsigned int quadVAO, quadVBO;
    unsigned int lineVAO, lineVBO;

    // Shaders
    Shader* debugShader;
    Shader* lineShader;

    // Visualization flags
    bool showChunkBoundaries;
    bool showBoundingBoxes;
    bool showGridLines;

    // Cached line data for chunk boundaries
    std::vector<float> chunkBoundaryLines;

    // Helper methods for adding geometric primitives
    void addBoxToLines(const glm::vec3& minCorner, const glm::vec3& maxCorner,
                       const glm::vec3& color, std::vector<float>& lines);

    void addLineToBuffer(float x1, float y1, float z1,
                         float x2, float y2, float z2,
                         const glm::vec3& color, std::vector<float>& buffer);
};

#endif // DEBUG_RENDERER_H