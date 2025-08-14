// -------------------------------------------------------------------------
// DebugDraw.h
// -------------------------------------------------------------------------
#pragma once

#include "Core/Subsystem.h"
#include "Utility/Color.h"

#include <glm/gtx/quaternion.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace PixelCraft::Rendering {
class Mesh;
class Shader;
class Texture;

namespace Cameras {
class Camera;
}
} // namespace PixelCraft::Rendering

namespace PixelCraft::Utility {

// Forward declarations
class AABB;
class Frustum;

/**
 * @brief Debug line primitive for immediate-mode rendering
 */
struct DebugLine {
    glm::vec3 start;    // Starting position of the line
    glm::vec3 end;      // Ending position of the line
    Color color;        // RGBA color
    float thickness;    // Line thickness
    float duration;     // How long the line should stay visible (0 = single frame)
    bool depthTest;     // Whether the line should be depth tested
    float creationTime; // When the line was created
};

/**
 * @brief Debug text primitive for immediate-mode rendering
 */
struct DebugText {
    std::string text;   // Text string to display
    glm::vec3 position; // 3D position of the text
    Color color;        // RGBA color
    float size;         // Text size
    bool billboard;     // Whether to align with camera
    float duration;     // How long the text should stay visible (0 = single frame)
    bool depthTest;     // Whether the text should be depth tested
    float creationTime; // When the text was created
};

/**
 * @brief Debug shape primitive for immediate-mode rendering
 */
struct DebugShape {
    /**
     * @brief Types of debug shapes supported
     */
    enum class Type { Box, Sphere, Cylinder, Cone, Capsule, Arrow, Frustum };

    Type type;          // Shape type
    glm::vec3 position; // 3D position of the shape
    glm::quat rotation; // Rotation of the shape
    glm::vec3 scale;    // Scale of the shape
    Color color;        // RGBA color
    bool wireframe;     // Whether to render as wireframe
    int segments;       // Amount of segments in circular shapes
    float duration;     // How long the shape should stay visible (0 = single frame)
    bool depthTest;     // Whether the shape should be depth tested
    float creationTime; // When the shape was created
};

/**
 * @brief Immediate-mode debug drawing utility for visualization
 *
 * The DebugDraw class provides a simple and efficient way to render
 * debug visuals like lines, shapes, and text. These visuals can persist
 * for specified durations or appear for a single frame.
 */
class DebugDraw : public Core::Subsystem {
  public:
    /**
     * @brief Constructor
     */
    DebugDraw();

    /**
     * @brief Destructor
     */
    virtual ~DebugDraw();

    /**
     * @brief Initialize the debug drawing system
     * @return True if initialization was successful
     */
    bool initialize() override;

    /**
     * @brief Update the debug drawing system
     * @param deltaTime Time since last update
     */
    void update(float deltaTime) override;

    /**
     * @brief Render debug info (if enabled)
     */
    void render() override;

    /**
     * @brief Shut down the debug drawing system and release resources
     */
    void shutdown() override;

    /**
     * @brief Get the name of the subsystem
     * @return The subsystem name
     */
    std::string getName() const override { return "DebugDraw"; }

    /**
     * @brief Get dependencies required by this subsystem
     * @return List of dependency subsystem names
     */
    std::vector<std::string> getDependencies() const override;

    /**
     * @brief Begin a new debug drawing frame
     * Must be called before any drawing in a frame
     */
    void begin();

    /**
     * @brief Flush all debug drawings to the renderer
     * Must be called after all drawing in a frame
     */
    void flush();

    /**
     * @brief Clear all debug drawing primitives
     */
    void clear();

    /**
     * @brief Draw a line between two points
     * @param start Starting position
     * @param end Ending position
     * @param color RGBA color (default: white)
     * @param thickness Line thickness (default: 1.0)
     * @param duration How long the line should stay visible in seconds (0 = single frame)
     * @param depthTest Whether the line should be depth tested (default: true)
     */
    void drawLine(const glm::vec3 &start, const glm::vec3 &end, const Color &color = Color::White,
                  float thickness = 1.0f, float duration = 0.0f, bool depthTest = true);

    /**
     * @brief Draw a ray starting from an origin point
     * @param origin Ray origin position
     * @param direction Ray direction (will be normalized)
     * @param length Ray length
     * @param color RGBA color (default: white)
     * @param thickness Line thickness (default: 1.0)
     * @param duration How long the ray should stay visible in seconds (0 = single frame)
     * @param depthTest Whether the ray should be depth tested (default: true)
     */
    void drawRay(const glm::vec3 &origin, const glm::vec3 &direction, float length = 1.0f,
                 const Color &color = Color::White, float thickness = 1.0f, float duration = 0.0f,
                 bool depthTest = true);

    /**
     * @brief Draw a sequence of connected lines
     * @param points Vector of points to connect
     * @param color RGBA color (default: white)
     * @param thickness Line thickness (default: 1.0)
     * @param duration How long the lines should stay visible in seconds (0 = single frame)
     * @param depthTest Whether the lines should be depth tested (default: true)
     */
    void drawLineStrip(const std::vector<glm::vec3> &points, const Color &color = Color::White, float thickness = 1.0f,
                       float duration = 0.0f, bool depthTest = true);

    /**
     * @brief Draw a box shape
     * @param center Center position of the box
     * @param dimensions Width, height, and depth of the box
     * @param rotation Rotation of the box
     * @param color RGBA color (default: white)
     * @param wireframe Whether to render as wireframe (default: true)
     * @param duration How long the box should stay visible in seconds (0 = single frame)
     * @param depthTest Whether the box should be depth tested (default: true)
     */
    void drawBox(const glm::vec3 &center, const glm::vec3 &dimensions,
                 const glm::quat &rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), const Color &color = Color::White,
                 bool wireframe = true, float duration = 0.0f, bool depthTest = true);

    /**
     * @brief Draw a box from an axis-aligned bounding box
     * @param aabb The axis-aligned bounding box
     * @param color RGBA color (default: white)
     * @param wireframe Whether to render as wireframe (default: true)
     * @param duration How long the box should stay visible in seconds (0 = single frame)
     * @param depthTest Whether the box should be depth tested (default: true)
     */
    void drawBox(const AABB &aabb, const Color &color = Color::White, bool wireframe = true, float duration = 0.0f,
                 bool depthTest = true);

    /**
     * @brief Draw a sphere shape
     * @param center Center position of the sphere
     * @param radius Radius of the sphere
     * @param color RGBA color (default: white)
     * @param wireframe Whether to render as wireframe (default: true)
     * @param segments Number of segments for sphere tessellation (default: 16)
     * @param duration How long the sphere should stay visible in seconds (0 = single frame)
     * @param depthTest Whether the sphere should be depth tested (default: true)
     */
    void drawSphere(const glm::vec3 &center, float radius, const Color &color = Color::White, bool wireframe = true,
                    int segments = 16, float duration = 0.0f, bool depthTest = true);

    /**
     * @brief Draw a cylinder shape
     * @param base Center of the base of the cylinder
     * @param top Center of the top of the cylinder
     * @param radius Radius of the cylinder
     * @param color RGBA color (default: white)
     * @param wireframe Whether to render as wireframe (default: true)
     * @param segments Number of segments for cylinder tessellation (default: 16)
     * @param duration How long the cylinder should stay visible in seconds (0 = single frame)
     * @param depthTest Whether the cylinder should be depth tested (default: true)
     */
    void drawCylinder(const glm::vec3 &base, const glm::vec3 &top, float radius, const Color &color = Color::White,
                      bool wireframe = true, int segments = 16, float duration = 0.0f, bool depthTest = true);

    /**
     * @brief Draw a cone shape
     * @param apex Apex position of the cone
     * @param direction Direction the cone points (will be normalized)
     * @param length Length of the cone
     * @param radius Radius of the cone base
     * @param color RGBA color (default: white)
     * @param wireframe Whether to render as wireframe (default: true)
     * @param segments Number of segments for cone tessellation (default: 16)
     * @param duration How long the cone should stay visible in seconds (0 = single frame)
     * @param depthTest Whether the cone should be depth tested (default: true)
     */
    void drawCone(const glm::vec3 &apex, const glm::vec3 &direction, float length, float radius,
                  const Color &color = Color::White, bool wireframe = true, int segments = 16, float duration = 0.0f,
                  bool depthTest = true);

    /**
     * @brief Draw a capsule shape
     * @param start Center of the first hemisphere
     * @param end Center of the second hemisphere
     * @param radius Radius of the capsule
     * @param color RGBA color (default: white)
     * @param wireframe Whether to render as wireframe (default: true)
     * @param segments Number of segments for capsule tessellation (default: 16)
     * @param duration How long the capsule should stay visible in seconds (0 = single frame)
     * @param depthTest Whether the capsule should be depth tested (default: true)
     */
    void drawCapsule(const glm::vec3 &start, const glm::vec3 &end, float radius, const Color &color = Color::White,
                     bool wireframe = true, int segments = 16, float duration = 0.0f, bool depthTest = true);

    /**
     * @brief Draw an arrow
     * @param start Start position of the arrow
     * @param end End position of the arrow
     * @param headSize Size of the arrow head relative to the length
     * @param color RGBA color (default: white)
     * @param thickness Line thickness of the arrow shaft (default: 1.0)
     * @param duration How long the arrow should stay visible in seconds (0 = single frame)
     * @param depthTest Whether the arrow should be depth tested (default: true)
     */
    void drawArrow(const glm::vec3 &start, const glm::vec3 &end, float headSize = 0.2f,
                   const Color &color = Color::White, float thickness = 1.0f, float duration = 0.0f,
                   bool depthTest = true);

    /**
     * @brief Draw a circle
     * @param center Center position of the circle
     * @param radius Radius of the circle
     * @param normal Normal vector defining the plane of the circle
     * @param color RGBA color (default: white)
     * @param segments Number of segments for circle tessellation (default: 32)
     * @param duration How long the circle should stay visible in seconds (0 = single frame)
     * @param depthTest Whether the circle should be depth tested (default: true)
     */
    void drawCircle(const glm::vec3 &center, float radius, const glm::vec3 &normal = glm::vec3(0.0f, 1.0f, 0.0f),
                    const Color &color = Color::White, int segments = 32, float duration = 0.0f, bool depthTest = true);

    /**
     * @brief Draw a view frustum
     * @param frustum The view frustum to draw
     * @param color RGBA color (default: white)
     * @param duration How long the frustum should stay visible in seconds (0 = single frame)
     * @param depthTest Whether the frustum should be depth tested (default: true)
     */
    void drawFrustum(const Frustum &frustum, const Color &color = Color::White, float duration = 0.0f,
                     bool depthTest = true);

    /**
     * @brief Draw a grid
     * @param center Center position of the grid
     * @param size X and Y size of the grid
     * @param cellSize Size of each grid cell
     * @param color RGBA color (default: gray)
     * @param xzPlane Whether to draw the grid on the XZ plane (true) or XY plane (false)
     * @param duration How long the grid should stay visible in seconds (0 = single frame)
     * @param depthTest Whether the grid should be depth tested (default: true)
     */
    void drawGrid(const glm::vec3 &center, const glm::vec2 &size, float cellSize,
                  const Color &color = Color(0.5f, 0.5f, 0.5f, 1.0f), bool xzPlane = true, float duration = 0.0f,
                  bool depthTest = true);

    /**
     * @brief Draw coordinate axes
     * @param position Origin position of the axes
     * @param size Size/length of the axes
     * @param duration How long the axes should stay visible in seconds (0 = single frame)
     * @param depthTest Whether the axes should be depth tested (default: true)
     */
    void drawAxes(const glm::vec3 &position, float size = 1.0f, float duration = 0.0f, bool depthTest = true);

    /**
     * @brief Draw 3D text
     * @param text Text string to display
     * @param position 3D position of the text
     * @param color RGBA color (default: white)
     * @param size Text size
     * @param billboard Whether to align text with camera (default: true)
     * @param duration How long the text should stay visible in seconds (0 = single frame)
     * @param depthTest Whether the text should be depth tested (default: true)
     */
    void drawText(const std::string &text, const glm::vec3 &position, const Color &color = Color::White,
                  float size = 1.0f, bool billboard = true, float duration = 0.0f, bool depthTest = true);

    /**
     * @brief Draw 2D text on screen space
     * @param text Text string to display
     * @param position 2D screen position (0,0 is bottom-left)
     * @param color RGBA color (default: white)
     * @param size Text size
     * @param duration How long the text should stay visible in seconds (0 = single frame)
     */
    void draw2DText(const std::string &text, const glm::vec2 &position, const Color &color = Color::White,
                    float size = 1.0f, float duration = 0.0f);

    /**
     * @brief Draw a 2D rectangle on screen space
     * @param position Bottom-left position of the rectangle
     * @param size Width and height of the rectangle
     * @param color RGBA color (default: white)
     * @param filled Whether to draw a filled rectangle (default: false)
     * @param duration How long the rectangle should stay visible in seconds (0 = single frame)
     */
    void drawRect2D(const glm::vec2 &position, const glm::vec2 &size, const Color &color = Color::White,
                    bool filled = false, float duration = 0.0f);

    /**
     * @brief Draw a 2D circle on screen space
     * @param center Center position of the circle
     * @param radius Radius of the circle
     * @param color RGBA color (default: white)
     * @param filled Whether to draw a filled circle (default: false)
     * @param segments Number of segments for circle tessellation (default: 32)
     * @param duration How long the circle should stay visible in seconds (0 = single frame)
     */
    void drawCircle2D(const glm::vec2 &center, float radius, const Color &color = Color::White, bool filled = false,
                      int segments = 32, float duration = 0.0f);

    /**
     * @brief Draw a 2D line on screen space
     * @param start Start position of the line
     * @param end End position of the line
     * @param color RGBA color (default: white)
     * @param thickness Line thickness (default: 1.0)
     * @param duration How long the line should stay visible in seconds (0 = single frame)
     */
    void drawLine2D(const glm::vec2 &start, const glm::vec2 &end, const Color &color = Color::White,
                    float thickness = 1.0f, float duration = 0.0f);

    /**
     * @brief Set the camera used for debug drawing
     * @param camera Weak pointer to the camera
     */
    void setCamera(std::weak_ptr<Rendering::Cameras::Camera> camera);

  private:
    /**
     * @brief Render accumulated lines
     */
    void renderLines();

    /**
     * @brief Render accumulated shapes
     */
    void renderShapes();

    /**
     * @brief Render accumulated texts
     */
    void renderTexts();

    /**
     * @brief Render accumulated 2D elements
     */
    void render2DElements();

    /**
     * @brief Create primitive meshes for rendering
     */
    void createPrimitiveMeshes();

    /**
     * @brief Check if the DebugDraw system is initialized
     * @return True if initialized
     */
    bool isInitialized() const { return m_initialized; }

    // Shader for debug rendering
    std::shared_ptr<Rendering::Shader> m_shader;

    // OpenGL resources
    uint32_t m_lineVAO;
    uint32_t m_lineVBO;

    uint32_t m_textVAO;
    uint32_t m_textVBO;

    uint32_t m_billboardVAO;
    uint32_t m_billboardVBO;

    std::shared_ptr<Rendering::Mesh> m_boxMesh;
    std::shared_ptr<Rendering::Mesh> m_sphereMesh;
    std::shared_ptr<Rendering::Mesh> m_cylinderMesh;
    std::shared_ptr<Rendering::Mesh> m_coneMesh;

    // Debug drawing storage
    std::vector<DebugLine> m_lines;
    std::vector<DebugText> m_texts;
    std::vector<DebugShape> m_shapes;

    // 2D elements
    std::vector<DebugLine> m_lines2D;
    std::vector<DebugText> m_texts2D;

    std::weak_ptr<Rendering::Cameras::Camera> m_camera; ///< Camera reference
    std::shared_ptr<Rendering::Texture> m_fontTexture;  ///< Font texture for text rendering
    float m_currentTime;                                ///< Current time for duration calculations
    bool m_initialized = false;                         ///< Initialization flag
    static std::mutex s_mutex;                          ///< Mutex for thread safety
};

// Global convenience functions
/**
 * @brief Draw a line between two points (global convenience function)
 * @param start Starting position
 * @param end Ending position
 * @param color RGBA color (default: white)
 */
void drawLine(const glm::vec3 &start, const glm::vec3 &end, const Color &color = Color::White);

/**
 * @brief Draw a box shape (global convenience function)
 * @param center Center position of the box
 * @param dimensions Width, height, and depth of the box
 * @param color RGBA color (default: white)
 */
void drawBox(const glm::vec3 &center, const glm::vec3 &dimensions, const Color &color = Color::White);

/**
 * @brief Draw a sphere shape (global convenience function)
 * @param center Center position of the sphere
 * @param radius Radius of the sphere
 * @param color RGBA color (default: white)
 */
void drawSphere(const glm::vec3 &center, float radius, const Color &color = Color::White);

/**
 * @brief Draw 3D text (global convenience function)
 * @param text Text string to display
 * @param position 3D position of the text
 * @param color RGBA color (default: white)
 */
void drawText(const std::string &text, const glm::vec3 &position, const Color &color = Color::White);

} // namespace PixelCraft::Utility