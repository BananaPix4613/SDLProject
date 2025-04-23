#pragma once

#include <string>
#include <glm/glm.hpp>

// Forward declarations
class Editor;
class Camera;
class UIManager;
class Entity;

/**
 * @class EditorTool
 * @brief Base class for all editor tools
 * 
 * The EditorTool class provides a common interface for implementing
 * various editing tools in the engine. Each tool has a lifecycle
 * (activate/deativate), receives updates, handles input, and
 * can render UI and visualizations in the scene.
 */
class EditorTool
{
public:
    /**
     * @brief Constructor
     * @param name Name of the tool
     */
    EditorTool(const std::string& name);

    /**
     * @brief Virtual destructor
     */
    virtual ~EditorTool();

    /**
     * @brief Called when the tool is activated
     */
    virtual void activate();

    /**
     * @brief Called when the tool is deactivated
     */
    virtual void deactivate();

    /**
     * @brief Update the tool state
     * @param deltaTime Time elapsed since last update
     */
    virtual void update(float deltaTime);

    /**
     * @brief Render tool visualizations in the scene
     * @param camera Camera used for rendering
     */
    virtual void renderTool(Camera* camera);

    /**
     * @brief Render tool UI elements
     */
    virtual void renderUI();

    /**
     * @brief Handle mouse button events
     * @param button Mouse button
     * @param action Action (press, release)
     * @param mods Keyboard modifiers
     * @return True if the event was handled
     */
    virtual bool onMouseButton(int button, int action, int mods);

    /**
     * @brief Handle mouse movement events
     * @param xpos Mouse X position
     * @param ypos Mouse Y position
     * @return True if the event was handled
     */
    virtual bool onMouseMove(double xpos, double ypos);

    /**
     * @brief Handle mouse wheel events
     * @param xoffset Horizontal scroll amount
     * @param yoffset Vertical scroll amount
     * @return True if the event was handled
     */
    virtual bool onMouseScroll(double xoffset, double yoffset);

    /**
     * @brief Handle keyboard events
     * @param key Key code
     * @param scancode System-specific scancode
     * @param action Action (press, release, repeat)
     * @param mods Keyboard modifiers
     * @return True if the event was handled
     */
    virtual bool onKeyboard(int key, int scancode, int action, int mods);

    /**
     * @brief Get the tool name
     * @return Tool name
     */
    const std::string& getName() const;

    /**
     * @brief Check if the tool is active
     * @return True if the tool is active
     */
    bool isActive() const;

    /**
     * @brief Set the editor reference
     * @param editor Pointer to the editor
     */
    void setEditor(Editor* editor);

    /**
     * @brief Get the editor reference
     * @return Pointer to the editor
     */
    Editor* getEditor() const;

    /**
     * @brief Get the UI manager reference
     * @return Pointer to the UI manager
     */
    UIManager* getUIManager() const;

    /**
     * @brief Get the current camera
     * @return Pointer to the current camera
     */
    Camera* getCamera() const;

protected:
    std::string m_name;
    bool m_active;
    Editor* m_editor;

    // Last mouse position for tracking movement
    double m_lastMouseX;
    double m_lastMouseY;

    // Track button states
    bool m_mouseButtonDown[3];

    // Get ray from mouse position
    Ray getMouseRay(double xpos, double ypos) const;

    // Get entity under cursor
    Entity* getEntityUnderMouse(double xpos, double ypos) const;

    // Get world position from mouse position (intersection with plane)
    glm::vec3 getWorldPositionFromMouse(double xpos, double ypos, const glm::vec3& planeNormal = glm::vec3(0.0f, 1.0f, 0.0f), float planeDistance = 0.0f) const;

    // Snap position to grid
    glm::vec3 snapToGrid(const glm::vec3& position) const;

    // Snap position to pixel grid
    glm::vec3 snapToPixelGrid(const glm::vec3& position) const;
};