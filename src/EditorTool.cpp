#include "EditorTool.h"
#include "Editor.h"
#include "Camera.h"
#include "UIManager.h"
#include "Entity.h"
#include "Scene.h"
#include "Ray.h"

EditorTool::EditorTool(const std::string& name) :
    m_name(name), m_active(false), m_editor(nullptr),
    m_lastMouseX(0.0), m_lastMouseY(0.0)
{
    // Initialize mouse button states
    for (int i = 0; i < 3; i++)
    {
        m_mouseButtonDown[i] = false;
    }
}

EditorTool::~EditorTool()
{
}

void EditorTool::activate()
{
    m_active = true;

    // Reset mouse button states
    for (int i = 0; i < 3; i++)
    {
        m_mouseButtonDown[i] = false;
    }
}

void EditorTool::deactivate()
{
    m_active = false;
}

void EditorTool::update(float deltaTime)
{
    // Base implementation does nothing
}

void EditorTool::renderTool(Camera* camera)
{
    // Base implementation does nothing
}

void EditorTool::renderUI()
{
    // Base implementation does nothing
}

bool EditorTool::onMouseButton(int button, int action, int mods)
{
    // Track button state for buttons 0-2 (left, right, middle)
    if (button >= 0 && button < 3)
    {
        m_mouseButtonDown[button] = (action == GLFW_PRESS);
    }

    // Base implementation doesn't handle any buttons
    return false;
}

bool EditorTool::onMouseMove(double xpos, double ypos)
{
    // Track mouse movement
    m_lastMouseX = xpos;
    m_lastMouseY = xpos;

    // Base implementation doesn't handle mouse movement
    return false;
}

bool EditorTool::onMouseScroll(double xoffset, double yoffset)
{
    // Base implementation doesn't handle mouse scroll
    return false;
}

bool EditorTool::onKeyboard(int key, int scancode, int action, int mods)
{
    // Base implementation doesn't handle keyboard input
    return false;
}

const std::string& EditorTool::getName() const
{
    return m_name;
}

bool EditorTool::isActive() const
{
    return m_active;
}

void EditorTool::setEditor(Editor* editor)
{
    m_editor = editor;
}

Editor* EditorTool::getEditor() const
{
    return m_editor;
}

UIManager* EditorTool::getUIManager() const
{
    return m_editor ? m_editor->getUIManager() : nullptr;
}

Camera* EditorTool::getCamera() const
{
    return m_editor ? m_editor->getEditorCamera() : nullptr;
}

Ray EditorTool::getMouseRay(double xpos, double ypos) const
{
    if (!getCamera())
        return Ray();

    // Convert screen coordinates to normalized device coordinates
    glm::vec2 screenSize = getUIManager()->getViewportSize();
    float ndcX = (2.0f * xpos / screenSize.x) - 1.0f;
    float ndcY = 1.0f - (2.0f * ypos / screenSize.y);

    return getCamera()->screenPointToRay(glm::vec2(ndcX, ndcY));
}

Entity* EditorTool::getEntityUnderMouse(double xpos, double ypos) const
{
    if (!m_editor || !m_editor->getActiveScene())
        return nullptr;

    Ray ray = getMouseRay(xpos, ypos);
    return m_editor->getActiveScene()->raycastEntity(ray);
}

glm::vec3 EditorTool::getWorldPositionFromMouse(double xpos, double ypos, const glm::vec3& planeNormal, float planeDistance) const
{
    Ray ray = getMouseRay(xpos, ypos);

    // Calculate intersection with plane
    float denom = glm::dot(ray.direction, planeNormal);

    if (std::abs(denom) > 0.0001f)
    {
        float t = -(glm::dot(ray.origin, planeNormal) + planeDistance) / denom;
        if (t >= 0)
        {
            return ray.origin + ray.direction * t;
        }
    }

    // Default to a point at a fixed distance along the ray if no intersection
    return ray.origin + ray.direction * 10.0f;
}

glm::vec3 EditorTool::snapToGrid(const glm::vec3& position) const
{
    if (!m_editor)
        return position;

    bool gridSnapEnabled;
    float gridSize = m_editor->getGridSnap(gridSnapEnabled);

    if (!gridSnapEnabled || gridSize <= 0.0f)
        return position;

    // Snap to grid
    return glm::vec3(
        std::round(position.x / gridSize) * gridSize,
        std::round(position.y / gridSize) * gridSize,
        std::round(position.z / gridSize) * gridSize
    );
}

glm::vec3 EditorTool::snapToPixelGrid(const glm::vec3& position) const
{
    if (!m_editor)
        return position;

    bool pixelGridEnabled;
    int pixelSize = m_editor->getPixelGridAlignment(pixelGridEnabled);

    if (!pixelGridEnabled || pixelSize <= 0)
        return position;

    // Get camera for proper projection
    Camera* camera = getCamera();
    if (!camera)
        return position;

    // Project position to screen space
    glm::vec3 screenPos = camera->worldToScreenPoint(position);

    // Snap to pixel grid in screen space
    screenPos.x = std::round(screenPos.x / pixelSize) * pixelSize;
    screenPos.y = std::round(screenPos.y / pixelSize) * pixelSize;

    // Project back to world space
    return camera->screenToWorldPoint(screenPos);
}