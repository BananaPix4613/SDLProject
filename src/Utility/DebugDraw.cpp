// -------------------------------------------------------------------------
// DebugDraw.cpp
// -------------------------------------------------------------------------
#include "Utility/DebugDraw.h"
#include "Core/Logger.h"
#include "Core/Application.h"
#include "Rendering/RenderSystem.h"

#include <algorithm>
#include <cmath>
#include <mutex>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Utility
{

    // Static members initialization
    DebugDraw* DebugDraw::s_instance = nullptr;
    std::mutex DebugDraw::s_mutex;

    // Singleton accessor
    DebugDraw& DebugDraw::getInstance()
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_instance == nullptr)
        {
            s_instance = new DebugDraw();
        }
        return *s_instance;
    }

    DebugDraw::DebugDraw()
        : m_lineVAO(0)
        , m_lineVBO(0)
        , m_textVAO(0)
        , m_textVBO(0)
        , m_billboardVAO(0)
        , m_billboardVBO(0)
        , m_currentTime(0.0f)
        , m_initialized(false)
    {
    }

    DebugDraw::~DebugDraw()
    {
        if (m_initialized)
        {
            shutdown();
        }
    }

    bool DebugDraw::initialize()
    {
        if (m_initialized)
        {
            Log::warn("DebugDraw already initialized");
            return true;
        }

        Log::info("Initializing DebugDraw subsystem");

        // Get the render system from the application
        auto renderSystem = Core::Application::getInstance().getSubsystem<Rendering::RenderSystem>();
        if (!renderSystem)
        {
            Log::error("DebugDraw initialization failed: RenderSystem not found");
            return false;
        }

        // Load the debug shader
        m_shader = renderSystem->getResourceManager()->createResource<Rendering::Shader>("DebugDraw");
        if (!m_shader)
        {
            Log::error("DebugDraw initialization failed: Failed to create shader");
            return false;
        }

        // Set shader source
        const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec4 aColor;
        
        uniform mat4 uViewProjection;
        uniform mat4 uModel;
        
        out vec4 vertexColor;
        
        void main() {
            gl_Position = uViewProjection * uModel * vec4(aPos, 1.0);
            vertexColor = aColor;
        }
    )";

        const char* fragmentShaderSource = R"(
        #version 330 core
        in vec4 vertexColor;
        
        out vec4 FragColor;
        
        void main() {
            FragColor = vertexColor;
        }
    )";

        // Compile shader
        if (!m_shader->compileFromSource(vertexShaderSource, fragmentShaderSource))
        {
            Log::error("DebugDraw initialization failed: Failed to compile shader");
            return false;
        }

        // Create VAO and VBO for lines
        glGenVertexArrays(1, &m_lineVAO);
        glGenBuffers(1, &m_lineVBO);

        glBindVertexArray(m_lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);

        // Position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (void*)0);

        // Color attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (void*)(sizeof(float) * 3));

        glBindVertexArray(0);

        // Create VAO and VBO for text
        glGenVertexArrays(1, &m_textVAO);
        glGenBuffers(1, &m_textVBO);

        // Create VAO and VBO for billboards
        glGenVertexArrays(1, &m_billboardVAO);
        glGenBuffers(1, &m_billboardVBO);

        // Load font texture
        m_fontTexture = renderSystem->getResourceManager()->loadResource<Rendering::Texture>("Fonts/DebugFont.png");
        if (!m_fontTexture)
        {
            Log::warn("DebugDraw: Failed to load debug font texture");
            // Continue without text rendering capability
        }

        // Create primitive meshes
        createPrimitiveMeshes();

        m_initialized = true;
        return true;
    }

    void DebugDraw::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        Log::info("Shutting down DebugDraw subsystem");

        // Delete OpenGL resources
        glDeleteVertexArrays(1, &m_lineVAO);
        glDeleteBuffers(1, &m_lineVBO);

        glDeleteVertexArrays(1, &m_textVAO);
        glDeleteBuffers(1, &m_textVBO);

        glDeleteVertexArrays(1, &m_billboardVAO);
        glDeleteBuffers(1, &m_billboardVBO);

        // Clear all debug primitives
        clear();

        // Reset primitive meshes
        m_boxMesh.reset();
        m_sphereMesh.reset();
        m_cylinderMesh.reset();
        m_coneMesh.reset();

        // Reset shader and font texture
        m_shader.reset();
        m_fontTexture.reset();

        m_initialized = false;
    }

    void DebugDraw::begin()
    {
        if (!m_initialized)
        {
            return;
        }

        // Clear single-frame primitives
        m_lines.erase(
            std::remove_if(m_lines.begin(), m_lines.end(),
                           [this](const DebugLine& line) {
                               return line.duration <= 0.0f ||
                                   (line.duration > 0.0f && (m_currentTime - line.creationTime) >= line.duration);
                           }),
            m_lines.end()
        );

        m_texts.erase(
            std::remove_if(m_texts.begin(), m_texts.end(),
                           [this](const DebugText& text) {
                               return text.duration <= 0.0f ||
                                   (text.duration > 0.0f && (m_currentTime - text.creationTime) >= text.duration);
                           }),
            m_texts.end()
        );

        m_shapes.erase(
            std::remove_if(m_shapes.begin(), m_shapes.end(),
                           [this](const DebugShape& shape) {
                               return shape.duration <= 0.0f ||
                                   (shape.duration > 0.0f && (m_currentTime - shape.creationTime) >= shape.duration);
                           }),
            m_shapes.end()
        );

        m_lines2D.erase(
            std::remove_if(m_lines2D.begin(), m_lines2D.end(),
                           [this](const DebugLine& line) {
                               return line.duration <= 0.0f ||
                                   (line.duration > 0.0f && (m_currentTime - line.creationTime) >= line.duration);
                           }),
            m_lines2D.end()
        );

        m_texts2D.erase(
            std::remove_if(m_texts2D.begin(), m_texts2D.end(),
                           [this](const DebugText& text) {
                               return text.duration <= 0.0f ||
                                   (text.duration > 0.0f && (m_currentTime - text.creationTime) >= text.duration);
                           }),
            m_texts2D.end()
        );
    }

    void DebugDraw::flush()
    {
        if (!m_initialized)
        {
            return;
        }

        auto camera = m_camera.lock();
        if (!camera)
        {
            Log::warn("DebugDraw::flush - No camera set");
            return;
        }

        // Render 3D elements first
        renderLines();
        renderShapes();
        renderTexts();

        // Then render 2D elements (screen-space)
        render2DElements();
    }

    void DebugDraw::clear()
    {
        m_lines.clear();
        m_texts.clear();
        m_shapes.clear();
        m_lines2D.clear();
        m_texts2D.clear();
    }

    void DebugDraw::drawLine(const glm::vec3& start, const glm::vec3& end,
                             const glm::vec4& color, float thickness,
                             float duration, bool depthTest)
    {
        if (!m_initialized)
        {
            return;
        }

        DebugLine line;
        line.start = start;
        line.end = end;
        line.color = color;
        line.thickness = thickness;
        line.duration = duration;
        line.depthTest = depthTest;
        line.creationTime = m_currentTime;

        m_lines.push_back(line);
    }

    void DebugDraw::drawRay(const glm::vec3& origin, const glm::vec3& direction,
                            float length, const glm::vec4& color, float thickness,
                            float duration, bool depthTest)
    {
        if (!m_initialized)
        {
            return;
        }

        glm::vec3 normalizedDir = glm::normalize(direction);
        glm::vec3 end = origin + normalizedDir * length;
        drawLine(origin, end, color, thickness, duration, depthTest);
    }

    void DebugDraw::drawLineStrip(const std::vector<glm::vec3>& points,
                                  const glm::vec4& color, float thickness,
                                  float duration, bool depthTest)
    {
        if (!m_initialized || points.size() < 2)
        {
            return;
        }

        for (size_t i = 0; i < points.size() - 1; ++i)
        {
            drawLine(points[i], points[i + 1], color, thickness, duration, depthTest);
        }
    }

    void DebugDraw::drawBox(const glm::vec3& center, const glm::vec3& dimensions,
                            const glm::quat& rotation, const glm::vec4& color,
                            bool wireframe, float duration, bool depthTest)
    {
        if (!m_initialized)
        {
            return;
        }

        DebugShape shape;
        shape.type = DebugShape::Type::Box;
        shape.position = center;
        shape.rotation = rotation;
        shape.scale = dimensions * 0.5f; // Half extents
        shape.color = color;
        shape.wireframe = wireframe;
        shape.duration = duration;
        shape.depthTest = depthTest;
        shape.creationTime = m_currentTime;

        m_shapes.push_back(shape);
    }

    void DebugDraw::drawBox(const AABB& aabb, const glm::vec4& color,
                            bool wireframe, float duration, bool depthTest)
    {
        if (!m_initialized)
        {
            return;
        }

        glm::vec3 center = (aabb.getMin() + aabb.getMax()) * 0.5f;
        glm::vec3 dimensions = aabb.getMax() - aabb.getMin();
        drawBox(center, dimensions, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), color, wireframe, duration, depthTest);
    }

    void DebugDraw::drawSphere(const glm::vec3& center, float radius,
                               const glm::vec4& color, bool wireframe,
                               int segments, float duration, bool depthTest)
    {
        if (!m_initialized)
        {
            return;
        }

        DebugShape shape;
        shape.type = DebugShape::Type::Sphere;
        shape.position = center;
        shape.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        shape.scale = glm::vec3(radius);
        shape.color = color;
        shape.wireframe = wireframe;
        shape.duration = duration;
        shape.depthTest = depthTest;
        shape.creationTime = m_currentTime;

        m_shapes.push_back(shape);
    }

    void DebugDraw::drawCylinder(const glm::vec3& base, const glm::vec3& top,
                                 float radius, const glm::vec4& color, bool wireframe,
                                 int segments, float duration, bool depthTest)
    {
        if (!m_initialized)
        {
            return;
        }

        // Calculate position, scale, and rotation
        glm::vec3 center = (base + top) * 0.5f;
        glm::vec3 direction = top - base;
        float height = glm::length(direction);

        // Create a rotation from the default up vector (0,1,0) to the direction
        glm::vec3 defaultUp(0.0f, 1.0f, 0.0f);
        direction = glm::normalize(direction);

        glm::quat rotation;
        float dot = glm::dot(defaultUp, direction);

        if (dot >= 0.999f)
        {
            // Vectors are parallel, no rotation needed
            rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }
        else if (dot <= -0.999f)
        {
            // Vectors are opposite, rotate 180 degrees around X axis
            rotation = glm::quat(0.0f, 1.0f, 0.0f, 0.0f);
        }
        else
        {
            // Normal case, create rotation from cross product
            glm::vec3 axis = glm::cross(defaultUp, direction);
            axis = glm::normalize(axis);
            float angle = acos(dot);
            rotation = glm::angleAxis(angle, axis);
        }

        DebugShape shape;
        shape.type = DebugShape::Type::Cylinder;
        shape.position = center;
        shape.rotation = rotation;
        shape.scale = glm::vec3(radius, height * 0.5f, radius);
        shape.color = color;
        shape.wireframe = wireframe;
        shape.duration = duration;
        shape.depthTest = depthTest;
        shape.creationTime = m_currentTime;

        m_shapes.push_back(shape);
    }

    void DebugDraw::drawCone(const glm::vec3& apex, const glm::vec3& direction,
                             float length, float radius, const glm::vec4& color,
                             bool wireframe, int segments, float duration, bool depthTest)
    {
        if (!m_initialized)
        {
            return;
        }

        // Calculate position, scale, and rotation
        glm::vec3 normalizedDir = glm::normalize(direction);
        glm::vec3 base = apex + normalizedDir * length;
        glm::vec3 center = apex + normalizedDir * (length * 0.5f);

        // Create a rotation from the default up vector (0,1,0) to the direction
        glm::vec3 defaultUp(0.0f, 1.0f, 0.0f);

        glm::quat rotation;
        float dot = glm::dot(defaultUp, normalizedDir);

        if (dot >= 0.999f)
        {
            // Vectors are parallel, no rotation needed
            rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }
        else if (dot <= -0.999f)
        {
            // Vectors are opposite, rotate 180 degrees around X axis
            rotation = glm::quat(0.0f, 1.0f, 0.0f, 0.0f);
        }
        else
        {
            // Normal case, create rotation from cross product
            glm::vec3 axis = glm::cross(defaultUp, normalizedDir);
            axis = glm::normalize(axis);
            float angle = acos(dot);
            rotation = glm::angleAxis(angle, axis);
        }

        DebugShape shape;
        shape.type = DebugShape::Type::Cone;
        shape.position = center;
        shape.rotation = rotation;
        shape.scale = glm::vec3(radius, length * 0.5f, radius);
        shape.color = color;
        shape.wireframe = wireframe;
        shape.duration = duration;
        shape.depthTest = depthTest;
        shape.creationTime = m_currentTime;

        m_shapes.push_back(shape);
    }

    void DebugDraw::drawCapsule(const glm::vec3& start, const glm::vec3& end,
                                float radius, const glm::vec4& color, bool wireframe,
                                int segments, float duration, bool depthTest)
    {
        if (!m_initialized)
        {
            return;
        }

        // Calculate position, scale, and rotation
        glm::vec3 center = (start + end) * 0.5f;
        glm::vec3 direction = end - start;
        float height = glm::length(direction);

        // Create a rotation from the default up vector (0,1,0) to the direction
        glm::vec3 defaultUp(0.0f, 1.0f, 0.0f);
        direction = glm::normalize(direction);

        glm::quat rotation;
        float dot = glm::dot(defaultUp, direction);

        if (dot >= 0.999f)
        {
            // Vectors are parallel, no rotation needed
            rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }
        else if (dot <= -0.999f)
        {
            // Vectors are opposite, rotate 180 degrees around X axis
            rotation = glm::quat(0.0f, 1.0f, 0.0f, 0.0f);
        }
        else
        {
            // Normal case, create rotation from cross product
            glm::vec3 axis = glm::cross(defaultUp, direction);
            axis = glm::normalize(axis);
            float angle = acos(dot);
            rotation = glm::angleAxis(angle, axis);
        }

        DebugShape shape;
        shape.type = DebugShape::Type::Capsule;
        shape.position = center;
        shape.rotation = rotation;
        shape.scale = glm::vec3(radius, (height * 0.5f) + radius, radius);
        shape.color = color;
        shape.wireframe = wireframe;
        shape.duration = duration;
        shape.depthTest = depthTest;
        shape.creationTime = m_currentTime;

        m_shapes.push_back(shape);
    }

    void DebugDraw::drawArrow(const glm::vec3& start, const glm::vec3& end,
                              float headSize, const glm::vec4& color,
                              float thickness, float duration, bool depthTest)
    {
        if (!m_initialized)
        {
            return;
        }

        // Draw the shaft of the arrow
        drawLine(start, end, color, thickness, duration, depthTest);

        // Calculate the direction of the arrow
        glm::vec3 direction = glm::normalize(end - start);

        // Calculate the length of the arrow
        float length = glm::distance(start, end);

        // Only draw the head if the arrow has some length
        if (length > 0.0001f)
        {
            // Calculate the size of the head
            float actualHeadSize = length * headSize;

            // Calculate perpendicular vectors to create the arrow head
            glm::vec3 right;
            if (std::abs(direction.y) < 0.99f)
            {
                right = glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f));
            }
            else
            {
                right = glm::cross(direction, glm::vec3(1.0f, 0.0f, 0.0f));
            }
            right = glm::normalize(right);

            glm::vec3 up = glm::cross(right, direction);
            up = glm::normalize(up);

            // Calculate the positions of the arrow head points
            glm::vec3 headBase = end - direction * actualHeadSize;
            glm::vec3 headRight = headBase + right * actualHeadSize * 0.5f;
            glm::vec3 headLeft = headBase - right * actualHeadSize * 0.5f;
            glm::vec3 headUp = headBase + up * actualHeadSize * 0.5f;
            glm::vec3 headDown = headBase - up * actualHeadSize * 0.5f;

            // Draw the arrow head
            drawLine(end, headRight, color, thickness, duration, depthTest);
            drawLine(end, headLeft, color, thickness, duration, depthTest);
            drawLine(end, headUp, color, thickness, duration, depthTest);
            drawLine(end, headDown, color, thickness, duration, depthTest);
        }
    }

    void DebugDraw::drawCircle(const glm::vec3& center, float radius,
                               const glm::vec3& normal, const glm::vec4& color,
                               int segments, float duration, bool depthTest)
    {
        if (!m_initialized)
        {
            return;
        }

        // Calculate two perpendicular vectors in the plane
        glm::vec3 normalizedNormal = glm::normalize(normal);

        glm::vec3 tangent;
        if (std::abs(normalizedNormal.x) < std::abs(normalizedNormal.y))
        {
            tangent = glm::vec3(0.0f, normalizedNormal.z, -normalizedNormal.y);
        }
        else
        {
            tangent = glm::vec3(normalizedNormal.z, 0.0f, -normalizedNormal.x);
        }
        tangent = glm::normalize(tangent);

        glm::vec3 bitangent = glm::cross(normalizedNormal, tangent);
        bitangent = glm::normalize(bitangent);

        // Create circle points
        std::vector<glm::vec3> points;
        points.reserve(segments + 1);

        for (int i = 0; i <= segments; ++i)
        {
            float angle = 2.0f * glm::pi<float>() * static_cast<float>(i) / static_cast<float>(segments);
            float x = radius * std::cos(angle);
            float y = radius * std::sin(angle);
            glm::vec3 point = center + tangent * x + bitangent * y;
            points.push_back(point);
        }

        // Draw the circle as a line strip
        drawLineStrip(points, color, 1.0f, duration, depthTest);
    }

    void DebugDraw::drawFrustum(const Frustum& frustum, const glm::vec4& color,
                                float duration, bool depthTest)
    {
        if (!m_initialized)
        {
            return;
        }

        // Get the corners of the frustum
        const auto& corners = frustum.getCorners();

        // Draw the near plane
        drawLine(corners[0], corners[1], color, 1.0f, duration, depthTest);
        drawLine(corners[1], corners[2], color, 1.0f, duration, depthTest);
        drawLine(corners[2], corners[3], color, 1.0f, duration, depthTest);
        drawLine(corners[3], corners[0], color, 1.0f, duration, depthTest);

        // Draw the far plane
        drawLine(corners[4], corners[5], color, 1.0f, duration, depthTest);
        drawLine(corners[5], corners[6], color, 1.0f, duration, depthTest);
        drawLine(corners[6], corners[7], color, 1.0f, duration, depthTest);
        drawLine(corners[7], corners[4], color, 1.0f, duration, depthTest);

        // Connect near and far planes
        drawLine(corners[0], corners[4], color, 1.0f, duration, depthTest);
        drawLine(corners[1], corners[5], color, 1.0f, duration, depthTest);
        drawLine(corners[2], corners[6], color, 1.0f, duration, depthTest);
        drawLine(corners[3], corners[7], color, 1.0f, duration, depthTest);
    }

    void DebugDraw::drawGrid(const glm::vec3& center, const glm::vec2& size,
                             float cellSize, const glm::vec4& color, bool xzPlane,
                             float duration, bool depthTest)
    {
        if (!m_initialized)
        {
            return;
        }

        // Calculate grid dimensions
        int numCellsX = static_cast<int>(size.x / cellSize);
        int numCellsY = static_cast<int>(size.y / cellSize);

        float halfWidth = size.x * 0.5f;
        float halfHeight = size.y * 0.5f;

        // Calculate start positions
        glm::vec3 startPos = center - glm::vec3(halfWidth, 0.0f, halfHeight);

        // Draw grid lines
        if (xzPlane)
        {
            // X lines (along Z axis)
            for (int i = 0; i <= numCellsX; ++i)
            {
                float x = startPos.x + static_cast<float>(i) * cellSize;
                drawLine(
                    glm::vec3(x, center.y, startPos.z),
                    glm::vec3(x, center.y, startPos.z + size.y),
                    color, 1.0f, duration, depthTest
                );
            }

            // Z lines (along X axis)
            for (int i = 0; i <= numCellsY; ++i)
            {
                float z = startPos.z + static_cast<float>(i) * cellSize;
                drawLine(
                    glm::vec3(startPos.x, center.y, z),
                    glm::vec3(startPos.x + size.x, center.y, z),
                    color, 1.0f, duration, depthTest
                );
            }
        }
        else
        {
            // X lines (along Y axis)
            for (int i = 0; i <= numCellsX; ++i)
            {
                float x = startPos.x + static_cast<float>(i) * cellSize;
                drawLine(
                    glm::vec3(x, startPos.z, center.z),
                    glm::vec3(x, startPos.z + size.y, center.z),
                    color, 1.0f, duration, depthTest
                );
            }

            // Y lines (along X axis)
            for (int i = 0; i <= numCellsY; ++i)
            {
                float y = startPos.z + static_cast<float>(i) * cellSize;
                drawLine(
                    glm::vec3(startPos.x, y, center.z),
                    glm::vec3(startPos.x + size.x, y, center.z),
                    color, 1.0f, duration, depthTest
                );
            }
        }
    }

    void DebugDraw::drawAxes(const glm::vec3& position, float size,
                             float duration, bool depthTest)
    {
        if (!m_initialized)
        {
            return;
        }

        // X axis (red)
        drawLine(position, position + glm::vec3(size, 0.0f, 0.0f),
                 glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, duration, depthTest);

        // Y axis (green)
        drawLine(position, position + glm::vec3(0.0f, size, 0.0f),
                 glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), 1.0f, duration, depthTest);

        // Z axis (blue)
        drawLine(position, position + glm::vec3(0.0f, 0.0f, size),
                 glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), 1.0f, duration, depthTest);
    }

    void DebugDraw::drawText(const std::string& text, const glm::vec3& position,
                             const glm::vec4& color, float size, bool billboard,
                             float duration, bool depthTest)
    {
        if (!m_initialized)
        {
            return;
        }

        DebugText debugText;
        debugText.text = text;
        debugText.position = position;
        debugText.color = color;
        debugText.size = size;
        debugText.billboard = billboard;
        debugText.duration = duration;
        debugText.depthTest = depthTest;
        debugText.creationTime = m_currentTime;

        m_texts.push_back(debugText);
    }

    void DebugDraw::draw2DText(const std::string& text, const glm::vec2& position,
                               const glm::vec4& color, float size, float duration)
    {
        if (!m_initialized)
        {
            return;
        }

        DebugText debugText;
        debugText.text = text;
        debugText.position = glm::vec3(position, 0.0f);
        debugText.color = color;
        debugText.size = size;
        debugText.billboard = false;
        debugText.duration = duration;
        debugText.depthTest = false;
        debugText.creationTime = m_currentTime;

        m_texts2D.push_back(debugText);
    }

    void DebugDraw::drawRect2D(const glm::vec2& position, const glm::vec2& size,
                               const glm::vec4& color, bool filled, float duration)
    {
        if (!m_initialized)
        {
            return;
        }

        if (filled)
        {
            // For filled rectangles, we would normally use a quad or triangle mesh
            // For simplicity, we'll just use multiple lines to simulate a filled rectangle
            float stepSize = 1.0f; // 1 pixel step
            for (float y = 0.0f; y < size.y; y += stepSize)
            {
                drawLine2D(
                    glm::vec2(position.x, position.y + y),
                    glm::vec2(position.x + size.x, position.y + y),
                    color, 1.0f, duration
                );
            }
        }
        else
        {
            // For wireframe, just draw the outline
            drawLine2D(position, glm::vec2(position.x + size.x, position.y), color, 1.0f, duration);
            drawLine2D(glm::vec2(position.x + size.x, position.y), position + size, color, 1.0f, duration);
            drawLine2D(position + size, glm::vec2(position.x, position.y + size.y), color, 1.0f, duration);
            drawLine2D(glm::vec2(position.x, position.y + size.y), position, color, 1.0f, duration);
        }
    }

    void DebugDraw::drawCircle2D(const glm::vec2& center, float radius,
                                 const glm::vec4& color, bool filled,
                                 int segments, float duration)
    {
        if (!m_initialized)
        {
            return;
        }

        // Create circle points
        std::vector<glm::vec2> points;
        points.reserve(segments + 1);

        for (int i = 0; i <= segments; ++i)
        {
            float angle = 2.0f * glm::pi<float>() * static_cast<float>(i) / static_cast<float>(segments);
            float x = radius * std::cos(angle);
            float y = radius * std::sin(angle);
            points.push_back(center + glm::vec2(x, y));
        }

        if (filled)
        {
            // For filled circles, we would normally use a triangle fan
            // For simplicity, we'll draw lines from center to each point on the perimeter
            for (int i = 0; i < segments; ++i)
            {
                drawLine2D(center, points[i], color, 1.0f, duration);
                drawLine2D(points[i], points[i + 1], color, 1.0f, duration);
            }
        }
        else
        {
            // For wireframe, just draw the outline
            for (int i = 0; i < segments; ++i)
            {
                drawLine2D(points[i], points[i + 1], color, 1.0f, duration);
            }
        }
    }

    void DebugDraw::drawLine2D(const glm::vec2& start, const glm::vec2& end,
                               const glm::vec4& color, float thickness, float duration)
    {
        if (!m_initialized)
        {
            return;
        }

        DebugLine line;
        line.start = glm::vec3(start, 0.0f);
        line.end = glm::vec3(end, 0.0f);
        line.color = color;
        line.thickness = thickness;
        line.duration = duration;
        line.depthTest = false;
        line.creationTime = m_currentTime;

        m_lines2D.push_back(line);
    }

    void DebugDraw::setCamera(std::weak_ptr<Camera> camera)
    {
        m_camera = camera;
    }

    void DebugDraw::update(float deltaTime)
    {
        if (!m_initialized)
        {
            return;
        }

        m_currentTime += deltaTime;
    }

    void DebugDraw::renderLines()
    {
        if (m_lines.empty())
        {
            return;
        }

        // Get camera for view/projection matrices
        auto camera = m_camera.lock();
        if (!camera)
        {
            return;
        }

        glm::mat4 viewProj = camera->getProjectionMatrix() * camera->getViewMatrix();

        // Set up shader
        m_shader->bind();
        m_shader->setUniform("uViewProjection", viewProj);
        m_shader->setUniform("uModel", glm::mat4(1.0f));

        // Setup line rendering
        glBindVertexArray(m_lineVAO);

        // We'll sort lines by depth test flag to minimize state changes
        std::vector<DebugLine> depthTestLines;
        std::vector<DebugLine> noDepthTestLines;

        for (const auto& line : m_lines)
        {
            if (line.depthTest)
            {
                depthTestLines.push_back(line);
            }
            else
            {
                noDepthTestLines.push_back(line);
            }
        }

        // Render depth-tested lines first
        if (!depthTestLines.empty())
        {
            // Enable depth testing
            glEnable(GL_DEPTH_TEST);

            // Prepare line data
            std::vector<float> lineData;
            lineData.reserve(depthTestLines.size() * 14); // 7 floats per vertex, 2 vertices per line

            for (const auto& line : depthTestLines)
            {
                // Start point
                lineData.push_back(line.start.x);
                lineData.push_back(line.start.y);
                lineData.push_back(line.start.z);
                lineData.push_back(line.color.r);
                lineData.push_back(line.color.g);
                lineData.push_back(line.color.b);
                lineData.push_back(line.color.a);

                // End point
                lineData.push_back(line.end.x);
                lineData.push_back(line.end.y);
                lineData.push_back(line.end.z);
                lineData.push_back(line.color.r);
                lineData.push_back(line.color.g);
                lineData.push_back(line.color.b);
                lineData.push_back(line.color.a);
            }

            // Upload line data
            glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
            glBufferData(GL_ARRAY_BUFFER, lineData.size() * sizeof(float), lineData.data(), GL_DYNAMIC_DRAW);

            // Draw lines
            glLineWidth(1.0f); // Note: Line width is limited on modern hardware
            glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(depthTestLines.size() * 2));
        }

        // Then render lines without depth testing
        if (!noDepthTestLines.empty())
        {
            // Disable depth testing
            glDisable(GL_DEPTH_TEST);

            // Prepare line data
            std::vector<float> lineData;
            lineData.reserve(noDepthTestLines.size() * 14); // 7 floats per vertex, 2 vertices per line

            for (const auto& line : noDepthTestLines)
            {
                // Start point
                lineData.push_back(line.start.x);
                lineData.push_back(line.start.y);
                lineData.push_back(line.start.z);
                lineData.push_back(line.color.r);
                lineData.push_back(line.color.g);
                lineData.push_back(line.color.b);
                lineData.push_back(line.color.a);

                // End point
                lineData.push_back(line.end.x);
                lineData.push_back(line.end.y);
                lineData.push_back(line.end.z);
                lineData.push_back(line.color.r);
                lineData.push_back(line.color.g);
                lineData.push_back(line.color.b);
                lineData.push_back(line.color.a);
            }

            // Upload line data
            glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
            glBufferData(GL_ARRAY_BUFFER, lineData.size() * sizeof(float), lineData.data(), GL_DYNAMIC_DRAW);

            // Draw lines
            glLineWidth(1.0f);
            glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(noDepthTestLines.size() * 2));
        }

        // Clean up
        glBindVertexArray(0);
        m_shader->unbind();

        // Restore default state
        glEnable(GL_DEPTH_TEST);
    }

    void DebugDraw::renderShapes()
    {
        if (m_shapes.empty())
        {
            return;
        }

        // Get camera for view/projection matrices
        auto camera = m_camera.lock();
        if (!camera)
        {
            return;
        }

        glm::mat4 viewProj = camera->getProjectionMatrix() * camera->getViewMatrix();

        // Set up shader
        m_shader->bind();
        m_shader->setUniform("uViewProjection", viewProj);

        // We'll sort shapes by type and depth test flag to minimize state changes
        // This is a simple approach - a more sophisticated renderer would use more advanced batching

        // Process each shape type
        for (int type = 0; type < static_cast<int>(DebugShape::Type::Frustum) + 1; ++type)
        {
            auto shapeType = static_cast<DebugShape::Type>(type);

            // Get the mesh for this shape type
            std::shared_ptr<Rendering::Mesh> mesh;
            switch (shapeType)
            {
                case DebugShape::Type::Box:
                    mesh = m_boxMesh;
                    break;
                case DebugShape::Type::Sphere:
                    mesh = m_sphereMesh;
                    break;
                case DebugShape::Type::Cylinder:
                    mesh = m_cylinderMesh;
                    break;
                case DebugShape::Type::Cone:
                    mesh = m_coneMesh;
                    break;
                case DebugShape::Type::Capsule:
                    // Capsules are special - we need to handle them differently
                    // For now, we'll skip them
                    continue;
                case DebugShape::Type::Arrow:
                    // Arrows are drawn using lines, not meshes
                    continue;
                case DebugShape::Type::Frustum:
                    // Frustums are drawn using lines, not meshes
                    continue;
                default:
                    continue;
            }

            if (!mesh)
            {
                continue;
            }

            // Bind the mesh
            mesh->bind();

            // Render depth-tested shapes first, then non-depth-tested
            for (int depthPass = 0; depthPass < 2; ++depthPass)
            {
                bool depthTest = (depthPass == 0);

                // Set depth test state
                if (depthTest)
                {
                    glEnable(GL_DEPTH_TEST);
                }
                else
                {
                    glDisable(GL_DEPTH_TEST);
                }

                // Render each shape of this type and depth test setting
                for (const auto& shape : m_shapes)
                {
                    if (shape.type == shapeType && shape.depthTest == depthTest)
                    {
                        // Calculate model matrix
                        glm::mat4 model = glm::translate(glm::mat4(1.0f), shape.position);
                        model = model * glm::mat4_cast(shape.rotation);
                        model = glm::scale(model, shape.scale);

                        // Set uniforms
                        m_shader->setUniform("uModel", model);
                        m_shader->setUniform("uColor", shape.color);

                        // Set wireframe mode
                        if (shape.wireframe)
                        {
                            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                        }
                        else
                        {
                            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                        }

                        // Draw the mesh
                        mesh->draw();
                    }
                }
            }

            // Unbind the mesh
            mesh->unbind();
        }

        // Clean up
        m_shader->unbind();

        // Restore default state
        glEnable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    void DebugDraw::renderTexts()
    {
        if (m_texts.empty() || !m_fontTexture)
        {
            return;
        }

        // Get camera for view/projection matrices
        auto camera = m_camera.lock();
        if (!camera)
        {
            return;
        }

        glm::mat4 viewProj = camera->getProjectionMatrix() * camera->getViewMatrix();
        glm::mat4 view = camera->getViewMatrix();
        glm::mat4 proj = camera->getProjectionMatrix();

        // Set up shader for text rendering
        // Note: In a real implementation, we would use a specific text shader with font texture
        m_shader->bind();
        m_shader->setUniform("uViewProjection", viewProj);

        // Bind the font texture
        m_fontTexture->bind(0);
        m_shader->setUniform("uFontTexture", 0);

        // Setup text rendering
        glBindVertexArray(m_textVAO);

        // Enable alpha blending for text
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // We'll sort texts by depth test flag to minimize state changes
        std::vector<DebugText> depthTestTexts;
        std::vector<DebugText> noDepthTestTexts;

        for (const auto& text : m_texts)
        {
            if (text.depthTest)
            {
                depthTestTexts.push_back(text);
            }
            else
            {
                noDepthTestTexts.push_back(text);
            }
        }

        // Render each text
        // Note: This is a simplified implementation - a real text renderer would be more complex

        // Render depth-tested texts first
        if (!depthTestTexts.empty())
        {
            glEnable(GL_DEPTH_TEST);

            for (const auto& text : depthTestTexts)
            {
                // Calculate model matrix
                glm::mat4 model = glm::translate(glm::mat4(1.0f), text.position);

                if (text.billboard)
                {
                    // Extract rotation from view matrix to billboard the text
                    glm::mat3 rotMat(view);
                    rotMat = glm::transpose(rotMat);
                    glm::quat rotation = glm::quat_cast(rotMat);
                    model = model * glm::mat4_cast(rotation);
                }

                model = glm::scale(model, glm::vec3(text.size));

                // Set uniforms
                m_shader->setUniform("uModel", model);
                m_shader->setUniform("uColor", text.color);

                // Draw the text
                // For simplicity, we're just drawing a quad
                // In a real implementation, we'd generate vertices for each character
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
        }

        // Then render texts without depth testing
        if (!noDepthTestTexts.empty())
        {
            glDisable(GL_DEPTH_TEST);

            for (const auto& text : noDepthTestTexts)
            {
                // Calculate model matrix
                glm::mat4 model = glm::translate(glm::mat4(1.0f), text.position);

                if (text.billboard)
                {
                    // Extract rotation from view matrix to billboard the text
                    glm::mat3 rotMat(view);
                    rotMat = glm::transpose(rotMat);
                    glm::quat rotation = glm::quat_cast(rotMat);
                    model = model * glm::mat4_cast(rotation);
                }

                model = glm::scale(model, glm::vec3(text.size));

                // Set uniforms
                m_shader->setUniform("uModel", model);
                m_shader->setUniform("uColor", text.color);

                // Draw the text
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
        }

        // Clean up
        glBindVertexArray(0);
        m_fontTexture->unbind();
        m_shader->unbind();

        // Restore default state
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }

    void DebugDraw::render2DElements()
    {
        if (m_lines2D.empty() && m_texts2D.empty())
        {
            return;
        }

        // Get the viewport dimensions
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);

        // Create an orthographic projection matrix
        glm::mat4 orthoProj = glm::ortho(
            0.0f, static_cast<float>(viewport[2]),
            0.0f, static_cast<float>(viewport[3]),
            -1.0f, 1.0f
        );

        // Set up shader
        m_shader->bind();
        m_shader->setUniform("uViewProjection", orthoProj);
        m_shader->setUniform("uModel", glm::mat4(1.0f));

        // Disable depth testing for 2D elements
        glDisable(GL_DEPTH_TEST);

        // Render 2D lines
        if (!m_lines2D.empty())
        {
            glBindVertexArray(m_lineVAO);

            std::vector<float> lineData;
            lineData.reserve(m_lines2D.size() * 14); // 7 floats per vertex, 2 vertices per line

            for (const auto& line : m_lines2D)
            {
                // Start point
                lineData.push_back(line.start.x);
                lineData.push_back(line.start.y);
                lineData.push_back(line.start.z);
                lineData.push_back(line.color.r);
                lineData.push_back(line.color.g);
                lineData.push_back(line.color.b);
                lineData.push_back(line.color.a);

                // End point
                lineData.push_back(line.end.x);
                lineData.push_back(line.end.y);
                lineData.push_back(line.end.z);
                lineData.push_back(line.color.r);
                lineData.push_back(line.color.g);
                lineData.push_back(line.color.b);
                lineData.push_back(line.color.a);
            }

            // Upload line data
            glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
            glBufferData(GL_ARRAY_BUFFER, lineData.size() * sizeof(float), lineData.data(), GL_DYNAMIC_DRAW);

            // Draw lines
            glLineWidth(1.0f);
            glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_lines2D.size() * 2));

            glBindVertexArray(0);
        }

        // Render 2D texts
        if (!m_texts2D.empty() && m_fontTexture)
        {
            // Enable alpha blending for text
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            m_fontTexture->bind(0);
            m_shader->setUniform("uFontTexture", 0);

            glBindVertexArray(m_textVAO);

            for (const auto& text : m_texts2D)
            {
                // Calculate model matrix
                glm::mat4 model = glm::translate(glm::mat4(1.0f), text.position);
                model = glm::scale(model, glm::vec3(text.size));

                // Set uniforms
                m_shader->setUniform("uModel", model);
                m_shader->setUniform("uColor", text.color);

                // Draw the text
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }

            glBindVertexArray(0);
            m_fontTexture->unbind();

            glDisable(GL_BLEND);
        }

        // Clean up
        m_shader->unbind();

        // Restore default state
        glEnable(GL_DEPTH_TEST);
    }

    void DebugDraw::createPrimitiveMeshes()
    {
        // Get the render system
        auto renderSystem = Core::Application::getInstance().getSubsystem<Rendering::RenderSystem>();
        if (!renderSystem)
        {
            Log::error("DebugDraw::createPrimitiveMeshes - RenderSystem not found");
            return;
        }

        auto resourceManager = renderSystem->getResourceManager();
        if (!resourceManager)
        {
            Log::error("DebugDraw::createPrimitiveMeshes - ResourceManager not found");
            return;
        }

        // Create a box mesh
        m_boxMesh = resourceManager->createResource<Rendering::Mesh>("DebugBox");
        if (m_boxMesh)
        {
            // Box vertices, normals, UVs, and indices would be set here
            // For simplicity, we're not showing the full implementation
            m_boxMesh->createBox(glm::vec3(1.0f, 1.0f, 1.0f));
        }

        // Create a sphere mesh
        m_sphereMesh = resourceManager->createResource<Rendering::Mesh>("DebugSphere");
        if (m_sphereMesh)
        {
            m_sphereMesh->createSphere(1.0f, 16, 16);
        }

        // Create a cylinder mesh
        m_cylinderMesh = resourceManager->createResource<Rendering::Mesh>("DebugCylinder");
        if (m_cylinderMesh)
        {
            m_cylinderMesh->createCylinder(1.0f, 1.0f, 16);
        }

        // Create a cone mesh
        m_coneMesh = resourceManager->createResource<Rendering::Mesh>("DebugCone");
        if (m_coneMesh)
        {
            m_coneMesh->createCone(1.0f, 1.0f, 16);
        }
    }

    // Global convenience functions
    void drawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color)
    {
        DebugDraw::getInstance().drawLine(start, end, color);
    }

    void drawBox(const glm::vec3& center, const glm::vec3& dimensions, const glm::vec4& color)
    {
        DebugDraw::getInstance().drawBox(center, dimensions, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), color);
    }

    void drawSphere(const glm::vec3& center, float radius, const glm::vec4& color)
    {
        DebugDraw::getInstance().drawSphere(center, radius, color);
    }

    void drawText(const std::string& text, const glm::vec3& position, const glm::vec4& color)
    {
        DebugDraw::getInstance().drawText(text, position, color);
    }

} // namespace PixelCraft::Utility