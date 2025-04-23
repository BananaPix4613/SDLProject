#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <glad/glad.h>
#include "Shader.h"

/**
 * @class LineBatchRenderer
 * @brief Utility class for batch rendering lines and wireframes
 * 
 * The LineBatchRenderer provides an efficient way to render debug lines,
 * gizmos, wireframes, and other visual aids for the editor. It uses batching
 * to minimize draw calls and state changes.
 */
class LineBatchRenderer
{
public:
    /**
     * @brief Get singleton instance
     * @return Reference to the singleton instance
     */
    static LineBatchRenderer* getInstance();

    /**
     * @brief Initialize the renderer
     * @return True if initialization was successful
     */
    bool initialize();

    /**
     * @brief Begin a new line batch with the specified view and projection matrices
     * @param viewMatrix Camera view matrix
     * @param projectionMatrix Camera projection matrix
     */
    void begin(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

    /**
     * @brief Add a line to the batch
     * @param start Start point
     * @param end End point
     * @param color Line color (RGBA)
     * @param width Line width in pixels
     */
    void addLine(const glm::vec3& start, const glm::vec3& end,
                 const glm::vec4& color, float width = 1.0f);

    /**
     * @brief Add a line strip to the batch
     * @param points Array of points
     * @param count Number of points
     * @param color Line color (RGBA)
     * @param width Line width in pixels
     */
    void addLineStrip(const glm::vec3* points, int count,
                      const glm::vec4& color, float width = 1.0f);

    /**
     * @brief Add a line loop to the batch
     * @param points Array of points
     * @param count Number of points
     * @param color Line color (RGBA)
     * @param width Line width in pixels
     */
    void addLineLoop(const glm::vec3* points, int count,
                     const glm::vec4& color, float width = 1.0f);

    /**
     * @brief Add a circle to the batch
     * @param center Circle center
     * @param radius Circle radius
     * @param normal Circle normal (determines plane orientation)
     * @param color Circle color (RGBA)
     * @param segments Number of line segments to use
     * @param width Line width in pixels
     */
    void addCircle(const glm::vec3& center, float radius, const glm::vec3& normal,
                   const glm::vec4& color, int segments = 32, float width = 1.0f);

    /**
     * @brief Add a rectangle to the batch
     * @param center Rectangle center
     * @param size Rectangle size (width, height)
     * @param rotation Rectangle rotation quaternion
     * @param color Rectangle color (RGBA)
     * @param width Line width in pixels
     */
    void addRectangle(const glm::vec3& center, const glm::vec2& size,
                      const glm::quat& rotation, const glm::vec4& color, float width = 1.0f);

    /**
     * @brief Add a 3D box to the batch
     * @param center Box center
     * @param size Box size (width, height, depth)
     * @param rotation Box rotation quaternion
     * @param color Box color (RGBA)
     * @param width Line width in pixels
     */
    void addBox(const glm::vec3& center, const glm::vec3& size,
                const glm::quat& rotation, const glm::vec4& color, float width = 1.0f);

    /**
     * @brief Add a sphere wireframe to the batch
     * @param center Sphere center
     * @param radius Sphere radius
     * @param color Sphere color (RGBA)
     * @param rings Number of horizontal rings
     * @param segments Number of segments per ring
     * @param width Line width in pixels
     */
    void addSphere(const glm::vec3& center, float radius, const glm::vec4& color,
                   int rings = 8, int segments = 16, float width = 1.0f);

    /**
     * @brief Add a coordinate axes gizmo to the batch
     * @param position Gizmo position
     * @param rotation Gizmo rotation quaternion
     * @param size Axis length
     * @param width Line width in pixels
     */
    void addAxes(const glm::vec3& position, const glm::quat& rotation = glm::quat(),
                 float size = 1.0f, float width = 1.0f);

    /**
     * @brief Add a grid to the batch
     * @param center Grid center
     * @param size Grid size (width, height)
     * @param divisions Number of cell divisions
     * @param color Grid color (RGBA)
     * @param plane Grid plane (XY,  YZ, XZ)
     * @param width Line width in pixels
     */
    void addGrid(const glm::vec3& center, const glm::vec2& size, int divisions,
                 const glm::vec4& color, int plane = 2, float width = 1.0f);

    /**
     * @brief Add a ray to the batch
     * @param origin Ray origin
     * @param direction Ray direction
     * @param length Ray length
     * @param color Ray color (RGBA)
     * @param width Line width in pixels
     */
    void addRay(const glm::vec3& origin, const glm::vec3& direction, float length,
                const glm::vec4& color, float width = 1.0f);

    /**
     * @brief Render all lines in the batch and clear it
     */
    void end();

    /**
     * @brief Clean up resources
     */
    void shutdown();

private:
    // Singleton implementation
    LineBatchRenderer();
    ~LineBatchRenderer();
    LineBatchRenderer(const LineBatchRenderer&) = delete;
    LineBatchRenderer& operator=(const LineBatchRenderer&) = delete;

    static LineBatchRenderer* s_instance;

    // Line vertex structure
    struct LineVertex
    {
        glm::vec3 position;
        glm::vec4 color;
    };

    // OpenGL handles
    GLuint m_vao;
    GLuint m_vbo;

    // Shader
    std::unique_ptr<Shader> m_shader;

    // View and projection matrices
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;

    // Line batch data
    std::vector<LineVertex> m_vertices;
    bool m_batchStarted;

    // Line rendering state
    int m_maxVertices;
    float m_depthBias;
    bool m_depthTest;

    // Helper methods for coordinate system conversions
    void buildCoordinateSystem(const glm::vec3& normal, glm::vec3& tangent, glm::vec3& bitangent);
};