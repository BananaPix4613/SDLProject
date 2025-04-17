#include "LineBatchRenderer.h"
#include <cmath>
#include <algorithm>
#include <ext/scalar_constants.hpp>
#include <gtc/quaternion.hpp>

// Shader sources for line rendering
static const char* s_lineVertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec4 aColor;

uniform mat4 uViewProjection;
uniform float uDepthBias;

out vec4 vColor;

void main()
{
    vec4 viewPos = uViewProjection * vec4(aPosition, 1.0);
    
    // Apply depth bias to avoid z-fighting with other geometry
    viewPos.z -= uDepthBias;
    
    gl_position = viewPos;
    vColor = aColor;
}
)";

static const char* s_lineFragmentShaderSrc = R"(
#version 330 core
in vec4 vColor;
out vec4 FragColor;

void main()
{
    FragColor = vColor;
}
)";

// Static instance
LineBatchRenderer* LineBatchRenderer::s_instance = nullptr;

LineBatchRenderer* LineBatchRenderer::getInstance()
{
    if (!s_instance)
    {
        s_instance = new LineBatchRenderer();
    }

    return s_instance;
}

LineBatchRenderer::LineBatchRenderer() :
    m_vao(0), m_vbo(0), m_batchStarted(false), m_maxVertices(10000),
    m_depthBias(0.00001f), m_depthTest(true)
{
}

LineBatchRenderer::~LineBatchRenderer()
{
    shutdown();
}

bool LineBatchRenderer::initialize()
{
    // Create shader
    m_shader = std::make_unique<Shader>();

    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &s_lineVertexShaderSrc, nullptr);
    glCompileShader(vertexShader);

    // Check for vertex shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        // Handle error
        glDeleteShader(vertexShader);
        return false;
    }

    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &s_lineFragmentShaderSrc, nullptr);
    glCompileShader(fragmentShader);

    // Check for fragment shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        // Handle error
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    // Link shaders into program
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // Check for linking errors
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        // Handle error
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(program);
        return false;
    }

    // Clean up shader objects
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Store program in shader
    m_shader->setProgram(program);
    
    // Create vertex array and buffer
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    // Allocate buffer with maximum vertices
    glBufferData(GL_ARRAY_BUFFER, m_maxVertices * sizeof(LineVertex), nullptr, GL_DYNAMIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)offsetof(LineVertex, position));

    // Color attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)offsetof(LineVertex, color));

    // Unbind
    glBindVertexArray(0);

    // Preallocate vertex array
    m_vertices.reserve(m_maxVertices);

    return true;
}

void LineBatchRenderer::begin(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
{
    if (m_batchStarted)
    {
        end();
    }

    m_viewMatrix = viewMatrix;
    m_projectionMatrix = projectionMatrix;
    m_vertices.clear();
    m_batchStarted = true;
}

void LineBatchRenderer::addLine(const glm::vec3& start, const glm::vec3& end,
                                const glm::vec4& color, float width)
{
    if (!m_batchStarted)
        return;

    // Check if we have room for 2 more vertices
    if (m_vertices.size() + 2 > m_maxVertices)
    {
        // Flush current batch to make room
        this->end();
        begin(m_viewMatrix, m_projectionMatrix);
    }

    // Add line vertices
    LineVertex v1, v2;
    v1.position = start;
    v1.color = color;

    v2.position = end;
    v2.color = color;

    m_vertices.push_back(v1);
    m_vertices.push_back(v2);
}

void LineBatchRenderer::addLineStrip(const glm::vec3* points, int count,
                                     const glm::vec4& color, float width)
{
    if (!m_batchStarted || count < 2 || !points)
        return;

    // Add lines connecting adjacent points
    for (int i = 0; i < count - 1; i++)
    {
        addLine(points[i], points[i + 1], color, width);
    }
}

void LineBatchRenderer::addLineLoop(const glm::vec3* points, int count,
                                    const glm::vec4& color, float width)
{
    if (!m_batchStarted || count < 2 || !points)
        return;

    // Add lines connecting adjacent points and closing the loop
    for (int i = 0; i < count - 1; i++)
    {
        addLine(points[i], points[i + 1], color, width);
    }

    // Connect last point back to first point
    addLine(points[count - 1], points[0], color, width);
}

void LineBatchRenderer::addCircle(const glm::vec3& center, float radius, const glm::vec3& normal,
                                  const glm::vec4& color, int segments, float width)
{
    if (!m_batchStarted || segments < 3)
        return;

    // Build coordinate system from normal
    glm::vec3 tangent, bitangent;
    buildCoordinateSystem(glm::normalize(normal), tangent, bitangent);

    // Generate points around the circle
    std::vector<glm::vec3> points;
    points.reserve(segments);

    for (int i = 0; i < segments; i++)
    {
        float angle = 2.0f * glm::pi<float>() * static_cast<float>(i) / static_cast<float>(segments);
        float x = std::cos(angle);
        float y = std::sin(angle);

        // Calculate point on the circle
        glm::vec3 point = center + radius * (tangent * x + bitangent * y);
        points.push_back(point);
    }

    // Add as line loop
    addLineLoop(points.data(), segments, color, width);
}

void LineBatchRenderer::addRectangle(const glm::vec3& center, const glm::vec2& size,
                                     const glm::quat& rotation, const glm::vec4& color, float width)
{
    if (!m_batchStarted)
        return;

    // Calculate half size
    glm::vec2 halfSize = size * 0.5f;

    // Calculate corner points in local space
    glm::vec3 corners[4] = {
        glm::vec3(-halfSize.x, -halfSize.y, 0.0f),  // Bottom-left
        glm::vec3(halfSize.x, -halfSize.y, 0.0f),  // Bottom-right
        glm::vec3(halfSize.x, halfSize.y, 0.0f),  // Top-right
        glm::vec3(-halfSize.x, halfSize.y, 0.0f)   // Top-left
    };

    // Transform corners to world space
    for (int i = 0; i < 4; i++)
    {
        corners[i] = center + rotation * corners[i];
    }

    // Add as line loop
    addLineLoop(corners, 4, color, width);
}

void LineBatchRenderer::addBox(const glm::vec3& center, const glm::vec3& size,
                               const glm::quat& rotation, const glm::vec4& color, float width)
{
    if (!m_batchStarted)
        return;

    // Calculate half size
    glm::vec3 halfSize = size * 0.5f;

    // Calculate corner points in local space
    glm::vec3 corners[8] = {
        glm::vec3(-halfSize.x, -halfSize.y, -halfSize.z),  // Bottom-left-back
        glm::vec3(halfSize.x, -halfSize.y, -halfSize.z),  // Bottom-right-back
        glm::vec3(halfSize.x, halfSize.y, -halfSize.z),  // Top-right-back
        glm::vec3(-halfSize.x, halfSize.y, -halfSize.z),  // Top-left-back
        glm::vec3(-halfSize.x, -halfSize.y, halfSize.z),  // Bottom-left-front
        glm::vec3(halfSize.x, -halfSize.y, halfSize.z),  // Bottom-right-front
        glm::vec3(halfSize.x, halfSize.y, halfSize.z),  // Top-right-front
        glm::vec3(-halfSize.x, halfSize.y, halfSize.z)   // Top-left-front
    };

    // Transform corners to world space
    for (int i = 0; i < 8; i++)
    {
        corners[i] = center + rotation * corners[i];
    }

    // Add lines for box edges (12 edges total)

    // Bottom face
    addLine(corners[0], corners[1], color, width);
    addLine(corners[1], corners[2], color, width);
    addLine(corners[2], corners[3], color, width);
    addLine(corners[3], corners[0], color, width);

    // Top face
    addLine(corners[4], corners[5], color, width);
    addLine(corners[5], corners[6], color, width);
    addLine(corners[6], corners[7], color, width);
    addLine(corners[7], corners[4], color, width);

    // Connecting edges
    addLine(corners[0], corners[4], color, width);
    addLine(corners[1], corners[5], color, width);
    addLine(corners[2], corners[6], color, width);
    addLine(corners[3], corners[7], color, width);
}

void LineBatchRenderer::addSphere(const glm::vec3& center, float radius, const glm::vec4& color,
                                  int rings, int segments, float width)
{
    if (!m_batchStarted || rings < 2 || segments < 3)
        return;

    // Create rings around the three main axes
    // XZ rings (around Y axis)
    for (int i = 0; i <= rings; i++)
    {
        float phi = glm::pi<float>() * static_cast<float>(i) / static_cast<float>(rings);
        float y = radius * std::cos(phi);
        float ringRadius = radius * std::sin(phi);

        // Add horizontal ring at this height
        addCircle(
            center + glm::vec3(0.0f, y, 0.0f),  // Ring center
            ringRadius,                         // Ring radius
            glm::vec3(0.0f, 1.0f, 0.0f),        // Normal along Y axis
            color,
            segments,
            width
        );
    }

    // XY rings (around Z axis)
    for (int i = 0; i <= rings; i++)
    {
        float phi = glm::pi<float>() * static_cast<float>(i) / static_cast<float>(rings);
        float z = radius * std::cos(phi);
        float ringRadius = radius * std::sin(phi);

        // Add vertical ring parallel to XY plane
        addCircle(
            center + glm::vec3(0.0f, 0.0f, z),  // Ring center
            ringRadius,                         // Ring radius
            glm::vec3(0.0f, 0.0f, 1.0f),        // Normal along Z axis
            color,
            segments,
            width
        );
    }

    // YZ rings (around X axis)
    for (int i = 0; i <= rings; i++)
    {
        float phi = glm::pi<float>() * static_cast<float>(i) / static_cast<float>(rings);
        float x = radius * std::cos(phi);
        float ringRadius = radius * std::sin(phi);

        // Add vertical ring parallel to YZ plane
        addCircle(
            center + glm::vec3(x, 0.0f, 0.0f),  // Ring center
            ringRadius,                         // Ring radius
            glm::vec3(1.0f, 0.0f, 0.0f),        // Normal along X axis
            color,
            segments,
            width
        );
    }
}

void LineBatchRenderer::addAxes(const glm::vec3& position, const glm::quat& rotation,
                                float size, float width)
{
    if (!m_batchStarted)
        return;

    // Calculate axis directions
    glm::vec3 xAxis = rotation * glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 yAxis = rotation * glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 zAxis = rotation * glm::vec3(0.0f, 0.0f, 1.0f);

    // Add colored axis lines
    addLine(position, position + xAxis * size, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), width); // X axis (red)
    addLine(position, position + yAxis * size, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), width); // Y axis (green)
    addLine(position, position + zAxis * size, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), width); // Z axis (blue)
}

void LineBatchRenderer::addGrid(const glm::vec3& center, const glm::vec2& size, int divisions,
                                const glm::vec4& color, int plane, float width)
{
    if (!m_batchStarted || divisions < 1)
        return;

    // Calculate half size
    glm::vec2 halfSize = size * 0.5f;

    // Calculate cell size
    glm::vec2 cellSize = size / static_cast<float>(divisions);

    // Select axes based on plane
    glm::vec3 axes[3];

    switch (plane)
    {
        case 0: // YZ plane (X = constant)
            axes[0] = glm::vec3(0.0f, 1.0f, 0.0f);
            axes[1] = glm::vec3(0.0f, 0.0f, 1.0f);
            axes[2] = glm::vec3(1.0f, 0.0f, 0.0f);
            break;

        case 1: // XZ plane (Y = constant)
            axes[0] = glm::vec3(1.0f, 0.0f, 0.0f);
            axes[1] = glm::vec3(0.0f, 0.0f, 1.0f);
            axes[2] = glm::vec3(0.0f, 1.0f, 0.0f);
            break;

        case 2: // XY plane (Z = constant)
        default:
            axes[0] = glm::vec3(1.0f, 0.0f, 0.0f);
            axes[1] = glm::vec3(0.0f, 1.0f, 0.0f);
            axes[2] = glm::vec3(0.0f, 0.0f, 1.0f);
            break;
    }

    // Draw grid lines
    for (int i = 0; i <= divisions; ++i)
    {
        // Calculate normalized coordinate (-0.5 to 0.5)
        float t = static_cast<float>(i) / static_cast<float>(divisions) - 0.5f;

        // First axis lines
        glm::vec3 start = center + axes[0] * (-halfSize.x) + axes[1] * (t * size.y);
        glm::vec3 end = center + axes[0] * (halfSize.x) + axes[1] * (t * size.y);
        addLine(start, end, color, width);

        // Second axis lines
        start = center + axes[0] * (t * size.x) + axes[1] * (-halfSize.y);
        end = center + axes[0] * (t * size.x) + axes[1] * (halfSize.y);
        addLine(start, end, color, width);
    }
}

void LineBatchRenderer::addRay(const glm::vec3& origin, const glm::vec3& direction, float length,
                               const glm::vec4& color, float width)
{
    if (!m_batchStarted)
        return;
    
    glm::vec3 normalizedDir = glm::normalize(direction);
    glm::vec3 end = origin + normalizedDir * length;
    
    addLine(origin, end, color, width);
}

void LineBatchRenderer::end()
{
    if (!m_batchStarted || m_vertices.empty())
    {
        m_batchStarted = false;
        return;
    }
    
    // Activate shader
    m_shader->use();
    
    // Set uniforms
    glm::mat4 viewProj = m_projectionMatrix * m_viewMatrix;
    m_shader->setMat4("uViewProjection", viewProj);
    m_shader->setFloat("uDepthBias", m_depthBias);
    
    // Set OpenGL state for line rendering
    GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    
    if (m_depthTest && !depthTestWasEnabled)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else if (!m_depthTest && depthTestWasEnabled)
    {
        glDisable(GL_DEPTH_TEST);
    }
    
    // Line width state may be ignored on some platforms
    glLineWidth(1.0f);
    
    // Bind vertex array
    glBindVertexArray(m_vao);
    
    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertices.size() * sizeof(LineVertex), m_vertices.data());
    
    // Draw lines
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_vertices.size()));
    
    // Restore OpenGL state
    if (depthTestWasEnabled && !m_depthTest)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else if (!depthTestWasEnabled && m_depthTest)
    {
        glDisable(GL_DEPTH_TEST);
    }
    
    // Unbind
    glBindVertexArray(0);
    glUseProgram(0);
    
    // Clear batch state
    m_batchStarted = false;
}

void LineBatchRenderer::buildCoordinateSystem(const glm::vec3& normal, glm::vec3& tangent, glm::vec3& bitangent)
{
    if (std::abs(normal.x) > std::abs(normal.y))
    {
        // Use up vector as reference if normal is more aligned with x than y
        tangent = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), normal));
    }
    else
    {
        // Use right vector as reference if normal is more aligned with y than x
        tangent = glm::normalize(glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), normal));
    }
    
    // Calculate bitangent from normal and tangent
    bitangent = glm::normalize(glm::cross(normal, tangent));
    
    // Re-orthogonalize tangent (in case normal wasn't perfectly normalized)
    tangent = glm::normalize(glm::cross(bitangent, normal));
}