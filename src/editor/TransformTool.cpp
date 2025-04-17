#include "editor/TransformTool.h"
#include "Editor.h"
#include "Camera.h"
#include "UIManager.h"
#include "Entity.h"
#include "Scene.h"
#include "CommandManager.h"
#include "Command.h"
#include <algorithm>

TransformTool::TransformTool() :
    EditorTool("Transform"), m_transformMode(TransformMode::TRANSLATE),
    m_transformSpace(TransformSpace::WORLD), m_dragging(false),
    m_activeDragAxis(-1)
{
}

TransformTool::~TransformTool()
{
}

void TransformTool::activate()
{
    EditorTool::activate();
    m_dragging = false;
    m_activeDragAxis = -1;
}

void TransformTool::deactivate()
{
    EditorTool::deactivate();

    // End any in-progress transform
    if (m_dragging)
    {
        endTransform();
    }
}

void TransformTool::update(float deltaTime)
{
    // Transform tool doesn't need continuous updates
}

void TransformTool::renderTool(Camera* camera)
{
    // Only render if we have selected entities
    if (!m_editor || m_editor->getSelectedEntities().empty())
        return;

    // Render the appropriate gizmo based on transform mode
    switch (m_transformMode)
    {
        case TransformMode::TRANSLATE:
            renderTranslateGizmo(camera);
            break;

        case TransformMode::ROTATE:
            renderRotateGizmo(camera);
            break;

        case TransformMode::SCALE:
            renderScaleGizmo(camera);
            break;
    }
}

void TransformTool::renderUI()
{
    UIManager* ui = getUIManager();
    if (!ui)
        return;

    if (ui->beginPanel("Transform Tool"))
    {
        // Transform mode selector
        const char* modes[] = {"Translate", "Rotate", "Scale"};
        int currentMode = static_cast<int>(m_transformMode);
        if (ui->combo("Mode", currentMode, modes, 3))
        {
            setTransformMode(static_cast<TransformMode>(currentMode));
        }

        // Transform space selector
        const char* spaces[] = {"Local", "World", "Screen"};
        int currentSpace = static_cast<int>(m_transformSpace);
        if (ui->combo("Space", currentSpace, spaces, 3))
        {
            setTransformSpace(static_cast<TransformSpace>(currentSpace));
        }

        // Grid snap settings
        bool gridSnapEnabled;
        float gridSnapSize = m_editor->getGridSnap(gridSnapEnabled);
        if (ui->checkbox("Grid Snap", gridSnapEnabled))
        {
            m_editor->setGridSnap(gridSnapEnabled, gridSnapSize);
        }

        if (gridSnapEnabled)
        {
            if (ui->dragFloat("Snap Size", gridSnapSize, 0.1f, 0.1f, 10.0f))
            {
                m_editor->setGridSnap(gridSnapEnabled, gridSnapSize);
            }
        }

        // Pixel grid settings
        bool pixelGridEnabled;
        int pixelGridSize = m_editor->getPixelGridAlignment(pixelGridEnabled);
        if (ui->checkbox("Pixel Snap", pixelGridEnabled))
        {
            m_editor->setPixelGridAlignment(pixelGridEnabled, pixelGridSize);
        }

        if (pixelGridEnabled)
        {
            if (ui->dragInt("Pixel Size", pixelGridSize, 1, 1, 16))
            {
                m_editor->setPixelGridAlignment(pixelGridEnabled, pixelGridSize);
            }
        }

        // Show transform info for selected entity
        Entity* selectedEntity = m_editor->getSelectedEntity();
        if (selectedEntity)
        {
            ui->separator();
            ui->setSceneTexture("Selected Entity: " + selectedEntity->getName());

            // Position
            glm::vec3 position = selectedEntity->getPosition();
            if (ui->dragFloat3("Position", position, 0.1f))
            {
                // Create a command for the change
                m_editor->beginTransaction("Change Position");
                selectedEntity->setPosition(position);
                m_editor->commitTransaction();
            }

            // Rotation (Euler angles)
            glm::vec3 rotation = selectedEntity->setRotationEuler();
            if (ui->dragFloat3("Rotation", rotation, 1.0f))
            {
                // Create a command for the change
                m_editor->beginTransaction("Change Rotation");
                selectedEntity->setRotationEuler(rotation);
                m_editor->commitTransaction();
            }

            // Scale
            glm::vec3 scale = selectedEntity->getScale();
            if (ui->dragFloat3("Scale", scale, 0.1f, 0.01f))
            {
                // Create a command for the change
                m_editor->beginTransaction("Change Scale");
                selectedEntity->setScale(scale);
                m_editor->commitTransaction();
            }
        }

        ui->endPanel();
    }
}

bool TransformTool::onMouseButton(int button, int action, int mods)
{
    EditorTool::onMouseButton(button, action, mods);

    // Only handle left mouse button
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            // Don't start dragging if over UI
            if (getUIManager()->isMouseOverUI())
                return false;

            // Check if mouse is over a gizmo axis
            Ray ray = getMouseRay(m_lastMouseX, m_lastMouseY);
            int axis = getHoveredAxis(ray);

            if (axis >= 0)
            {
                // Start dragging
                m_dragging = true;
                m_activeDragAxis = axis;
                beginTransform();
                return true;
            }
        }
        else if (action == GLFW_RELEASE)
        {
            // End dragging
            if (m_dragging)
            {
                m_dragging = false;
                endTransform();
                return true;
            }
        }
    }

    return false;
}

bool TransformTool::onMouseMove(double xpos, double ypos)
{
    EditorTool::onMouseMove(xpos, ypos);

    // Update trasform if dragging
    if (m_dragging)
    {
        // Calculate drag delta
        glm::vec3 dragDelta = glm::vec3(0.0f);

        // Project mouse movement to appropriate axis or plane
        Camera* camera = getCamera();
        if (camera)
        {
            // Get screen delta
            glm::vec2 mouseDelta = glm::vec2(xpos - m_lastMouseX, ypos - m_lastMouseY);

            // Convert to world space delta based on active axis
            if (m_activeDragAxis >= 0 && m_activeDragAxis <= 2)
            {
                // Single axis: X, Y, or Z
                glm::vec3 axisDir = glm::vec3(0.0f);
                axisDir[m_activeDragAxis] = 1.0f;

                // Transform axis to appropriate space
                if (m_transformSpace == TransformSpace::LOCAL && m_editor->getSelectedEntity())
                {
                    axisDir = glm::mat3(m_editor->getSelectedEntity()->getRotation()) * axisDir;
                }
                else if (m_transformSpace == TransformSpace::SCREEN)
                {
                    // For screen space, use camera axes
                    if (m_activeDragAxis == 0) axisDir = camera->getRight();
                    else if (m_activeDragAxis == 1) axisDir = camera->getUp();
                    else axisDir = -camera->getForward();
                }

                // Project mouse movement along axis
                float axisDelta = (mouseDelta.x + mouseDelta.y) * 0.01f;
                dragDelta = axisDir * axisDelta;
            }
            else if (m_activeDragAxis >= 3 && m_activeDragAxis <= 5)
            {
                // Plane: XY, YZ, or XZ
                glm::vec3 normal = glm::vec3(0.0f);
                normal[m_activeDragAxis - 3] = 1.0f;

                // Transform normal to appropriate space
                if (m_transformSpace == TransformSpace::LOCAL && m_editor->getSelectedEntity())
                {
                    normal = glm::mat3(m_editor->getSelectedEntity()->getRotation()) * normal;
                }
                else if (m_transformSpace == TransformSpace::SCREEN)
                {
                    // For screen space, use camera normal
                    normal = camera->getForward();
                }

                // Get world positions
                glm::vec3 rayOrigin = getWorldPositionFromMouse(m_lastMouseX, m_lastMouseY, normal, 0);
                glm::vec3 rayNewOrigin = getWorldPositionFromMouse(xpos, ypos, normal, 0);

                // Calculate delta
                dragDelta = rayNewOrigin - rayOrigin;
            }
        }

        // Apply the transformation
        updateTransform(dragDelta);
        return true;
    }

    return false;
}

bool TransformTool::onKeyboard(int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        // Shortcut keys for transform modes
        if (key == GLFW_KEY_G || key == GLFW_KEY_T)
        {
            // G or T for translate mode
            setTransformMode(TransformMode::TRANSLATE);
            return true;
        }
        else if (key == GLFW_KEY_R)
        {
            // R for rotate mode
            setTransformMode(TransformMode::ROTATE);
            return true;
        }
        else if (key == GLFW_KEY_S)
        {
            // S for scale mode
            setTransformMode(TransformMode::SCALE);
            return true;
        }
        else if (key == GLFW_KEY_SPACE)
        {
            // Space to toggle local/world space
            setTransformSpace(m_transformSpace == TransformSpace::WORLD ?
                              TransformSpace::LOCAL : TransformSpace::WORLD);
            return true;
        }
    }

    return false;
}

void TransformTool::setTransformMode(TransformMode mode)
{
    m_transformMode = mode;
}

TransformMode TransformTool::getTransformMode() const
{
    return  m_transformMode;
}

void TransformTool::setTransformSpace(TransformSpace space)
{
    m_transformSpace = space;
}

TransformSpace TransformTool::getTransformSpace() const
{
    return m_transformSpace;
}

void TransformTool::beginTransform()
{
    if (!m_editor || m_editor->getSelectedEntities().empty())
        return;

    // Store original transforms for all selected entities
    m_entityTransforms.clear();
    for (Entity* entity : m_editor->getSelectedEntities())
    {
        if (entity)
        {
            EntityTransform transform;
            transform.entity = entity;
            transform.originalPosition = entity->getPosition();
            transform.originalRotation = entity->getRotation();
            transform.originalScale = entity->getScale();
            m_entityTransforms.push_back(transform);
        }
    }

    // Start a transaction for undo/redo
    m_editor->beginTransaction("Transform Entities");
}

void TransformTool::updateTransform(const glm::vec3& delta)
{
    if (m_entityTransforms.empty())
        return;

    // Apply transformation based on mode
    switch (m_transformMode)
    {
        case TransformMode::TRANSLATE:
            applyTranslation(delta);
            break;

        case TransformMode::ROTATE:
        {
            // For rotation, convert delta to angle around axis
            float angle = glm::length(delta) * 100.0f;
            glm::vec3 axis = glm::normalize(delta);

            // If axis is zero length, use Y axis
            if (glm::length(axis) < 0.001f)
            {
                axis = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            applyRotation(axis, angle);
            break;
        }

        case TransformMode::SCALE:
        {
            // For scaling, calculate scale factor based on delta
            glm::vec3 scale = glm::vec3(1.0f) + delta;

            // If using a single axis, only scale that axis
            if (m_activeDragAxis >= 0 && m_activeDragAxis <= 2)
            {
                scale = glm::vec3(1.0f);
                scale[m_activeDragAxis] = 1.0f + glm::length(delta) * (delta[m_activeDragAxis] >= 0 ? 1.0f : -1.0f);
            }

            applyScale(scale);
            break;
        }
    }
}

void TransformTool::endTransform()
{
    // Commit the transaction
    if (m_editor)
    {
        m_editor->commitTransaction();
    }

    m_entityTransforms.clear();
    m_activeDragAxis = -1;
}

void TransformTool::applyTranslation(const glm::vec3& delta)
{
    // Skip if no delta
    if (glm::length(delta) < 0.0001f)
        return;

    // Apply translation to all selected entities
    for (const EntityTransform& transform : m_entityTransforms)
    {
        if (transform.entity)
        {
            glm::vec3 newPosition = transform.originalPosition + delta;

            // Apply grid snapping if enabled
            bool gridSnapEnabled;
            float gridSize = m_editor->getGridSnap(gridSnapEnabled);
            if (gridSnapEnabled && gridSize > 0.0f)
            {
                newPosition = snapToGrid(newPosition);
            }

            // Apply pixel grid snapping if enabled
            bool pixelGridEnabled;
            int pixelSize = m_editor->getPixelGridAlignment(pixelGridEnabled);
            if (pixelGridEnabled && pixelSize > 0)
            {
                newPosition = snapToPixelGrid(newPosition);
            }

            transform.entity->setPosition(newPosition);
        }
    }
}

void TransformTool::applyRotation(const glm::vec3& axis, float angle)
{
    // Skip if angle is too small
    if (std::abs(angle) < 0.0001f)
        return;

    // Create rotation quaternion
    glm::quat rotation = glm::angleAxis(glm::radians(angle), glm::normalize(axis));

    // Apply rotation to all selected entities
    for (const EntityTransform& transform : m_entityTransforms)
    {
        if (transform.entity)
        {
            // Rotation depends on transform space
            glm::quat newRotation;

            if (m_transformSpace == TransformSpace::LOCAL)
            {
                // Local space: post-multiply
                newRotation = transform.originalRotation * rotation;
            }
            else
            {
                // World space: pre-multiply
                newRotation = rotation * transform.originalRotation;
            }

            transform.entity->setRotation(newRotation);
        }
    }
}

void TransformTool::applyScale(const glm::vec3& scale)
{
    // Apply scaling to all selected entities
    for (const EntityTransform& transform : m_entityTransforms)
    {
        if (transform.entity)
        {
            glm::vec3 newScale = transform.originalScale * scale;

            // Ensure scale doesn't go negative or too small
            newScale = glm::max(newScale, glm::vec3(0.01f));

            transform.entity->setScale(newScale);
        }
    }
}

int TransformTool::getHoveredAxis(const Ray& ray) const
{
    // No selection, no axis
    if (!m_editor || m_editor->getSelectedEntities().empty())
        return -1;

    // Get gizmo position and rotation
    glm::vec3 gizmoPos = getGizmoPosition();
    glm::quat gizmoRot = getGizmoRotation();

    // Define axis colors for visualization
    glm::vec3 axisColors[3] = {
        glm::vec3(1.0f, 0.0f, 0.0f),  // X - Red
        glm::vec3(0.0f, 1.0f, 0.0f),  // Y - Green
        glm::vec3(0.0f, 0.0f, 1.0f)   // Z - Blue
    };

    // Axis directions based on gizmo rotation
    glm::vec3 axes[3] = {
        gizmoRot * glm::vec3(1.0f, 0.0f, 0.0f),  // X
        gizmoRot * glm::vec3(0.0f, 1.0f, 0.0f),  // Y
        gizmoRot * glm::vec3(0.0f, 0.0f, 1.0f)   // Z
    };

    // Define axis line segments
    float axisLength = 1.0f;
    glm::vec3 axisStart = gizmoPos;
    glm::vec3 axisEnds[3] = {
        axisStart + axes[0] * axisLength,
        axisStart + axes[1] * axisLength,
        axisStart + axes[2] * axisLength
    };

    // Check for intersection with each axis
    float closestDistance = 0.05f;  // Threshold distance
    int closestAxis = -1;

    for (int i = 0; i < 3; i++)
    {
        // Calculate closest distance from ray to axis line segment
        float t;
        float dist = ray.distanceToLineSegment(axisStart, axisEnds[i], t);

        if (dist < closestDistance)
        {
            closestDistance = dist;
            closestAxis = i;
        }
    }

    // If using a translate tool, also check for plane handles (XY, YZ, XZ)
    if (m_transformMode == TransformMode::TRANSLATE)
    {
        // Define planes at the end of each axis
        float planeSize = 0.25f;

        // Define plane corner points for each plane
        glm::vec3 planeCorners[3][4] = {
            // XY plane (at Z)
            {
                axisStart + axes[0] * planeSize + axes[1] * planeSize,
                axisStart + axes[0] * planeSize - axes[1] * planeSize,
                axisStart - axes[0] * planeSize - axes[1] * planeSize,
                axisStart - axes[0] * planeSize + axes[1] * planeSize
            },
            // YZ plane (at X)
            {
                axisStart + axes[1] * planeSize + axes[2] * planeSize,
                axisStart + axes[1] * planeSize - axes[2] * planeSize,
                axisStart - axes[1] * planeSize - axes[2] * planeSize,
                axisStart - axes[1] * planeSize + axes[2] * planeSize
            },
            // XZ plane (at Y)
            {
                axisStart + axes[0] * planeSize + axes[2] * planeSize,
                axisStart + axes[0] * planeSize - axes[2] * planeSize,
                axisStart - axes[0] * planeSize - axes[2] * planeSize,
                axisStart - axes[0] * planeSize + axes[2] * planeSize
            }
        };

        // Check each plane
        for (int i = 0; i < 3; i++)
        {
            // Calculate plane normal (perpendicular to the other two axes)
            glm::vec3 normal = axes[i];

            // Check if ray intersects this plane
            float t;
            glm::vec3 intersectionPoint;

            if (ray.intersectPlane(axisStart, normal, t, intersectionPoint))
            {
                // Check if intersection is within the plane rectangle
                // This would require projecting the intersection point onto the plane
                // and checking if it's within the bounds

                // Simplified: just check if it's close enough to the center
                if (glm::distance(intersectionPoint, axisStart) < planeSize * 1.5f)
                {
                    // Return 3 + plane index (3=XY, 4=YZ, 5=XZ)
                    return 3 + i;
                }
            }
        }
    }

    return closestAxis;
}

glm::vec3 TransformTool::getGizmoPosition() const
{
    if (!m_editor || m_editor->getSelectedEntities().empty())
        return glm::vec3(0.0f);

    // Use the centroid of all selected entities
    glm::vec3 center(0.0f);
    int count = 0;

    for (Entity* entity : m_editor->getSelectedEntities())
    {
        if (entity)
        {
            center += entity->getWorldPosition();
            count++;
        }
    }

    if (count > 0)
    {
        center /= static_cast<float>(count);
    }

    return center;
}

glm::quat TransformTool::getGizmoRotation() const
{
    if (!m_editor || !m_editor->getSelectedEntity())
        return glm::quat();

    // Use the rotation of the primary selected entity
    Entity* entity = m_editor->getSelectedEntity();

    if (m_transformSpace == TransformSpace::LOCAL)
    {
        return entity->getWorldRotation();
    }
    else if (m_transformSpace == TransformSpace::SCREEN && getCamera())
    {
        // Align with camera for screen space
        Camera* camera = getCamera();

        glm::vec3 forward = -camera->getForward();
        glm::vec3 right = camera->getRight();
        glm::vec3 up = camera->getUp();

        // Create rotation matrix from these vectors
        glm::mat3 rotMatrix(right, up, forward);
        return glm::quat_cast(rotMatrix);
    }

    // World space is identity rotation
    return glm::quat();
}

void TransformTool::renderTranslateGizmo(Camera* camera)
{
    // Define common properties
    glm::vec3 gizmoPos = getGizmoPosition();
    glm::quat gizmoRot = getGizmoRotation();
    float axisLength = 1.0f;
    float handleSize = 0.1f;

    // Axis colors
    glm::vec4 axisColors[3] = {
        glm::vec4(1.0f, 0.2f, 0.2f, 1.0f),  // X - Red
        glm::vec4(0.2f, 1.0f, 0.2f, 1.0f),  // Y - Green
        glm::vec4(0.2f, 0.2f, 1.0f, 1.0f)   // Z - Blue
    };

    // Get line renderer
    LineBatchRenderer* lineRenderer = LineBatchRenderer::getInstance();
    lineRenderer->begin(camera->getViewMatrix(), camera->getProjectionMatrix());

    // Axis directions based on gizmo rotation
    glm::vec3 axes[3] = {
        gizmoRot * glm::vec3(1.0f, 0.0f, 0.0f),  // X
        gizmoRot * glm::vec3(0.0f, 1.0f, 0.0f),  // Y
        gizmoRot * glm::vec3(0.0f, 0.0f, 1.0f)   // Z
    };

    // Draw each axis
    for (int i = 0; i < 3; i++)
    {
        // Highlight active axis
        glm::vec4 color = axisColors[i];
        if (m_dragging && m_activeDragAxis == i)
        {
            color = glm::vec4(1.0f, 1.0f, 0.2f, 1.0f);  // Yellow highlight
        }

        // Draw axis line
        lineRenderer->addLine(
            gizmoPos,
            gizmoPos + axes[i] * axisLength,
            color,
            2.0f  // Line width
        );

        // Draw arrow head (simplified as a small cone of lines)
        glm::vec3 arrowTip = gizmoPos + axes[i] * axisLength;
        float coneBaseRadius = handleSize * 0.5f;
        int coneSegments = 8;

        for (int j = 0; j < coneSegments; j++)
        {
            float angle1 = j * 2.0f * glm::pi<float>() / coneSegments;
            float angle2 = (j + 1) * 2.0f * glm::pi<float>() / coneSegments;

            // Calculate perpendicular axes
            glm::vec3 perpAxis1, perpAxis2;

            if (std::abs(glm::dot(axes[i], glm::vec3(0.0f, 1.0f, 0.0f))) < 0.99f)
            {
                perpAxis1 = glm::normalize(glm::cross(axes[i], glm::vec3(0.0f, 1.0f, 0.0f)));
            }
            else
            {
                perpAxis1 = glm::normalize(glm::cross(axes[i], glm::vec3(1.0f, 0.0f, 0.0f)));
            }

            perpAxis2 = glm::normalize(glm::cross(axes[i], perpAxis1));

            // Calculate cone base points
            glm::vec3 basePoint1 = arrowTip - axes[i] * handleSize +
                perpAxis1 * coneBaseRadius * std::cos(angle1) +
                perpAxis2 * coneBaseRadius * std::sin(angle1);

            glm::vec3 basePoint2 = arrowTip - axes[i] * handleSize +
                perpAxis1 * coneBaseRadius * std::cos(angle2) +
                perpAxis2 * coneBaseRadius * std::sin(angle2);

            // Draw cone lines
            lineRenderer->addLine(arrowTip, basePoint1, color, 1.0f);
            lineRenderer->addLine(basePoint1, basePoint2, color, 1.0f);
        }
    }

    // Draw plane handles (squares at the end of each axis)
    if (!m_dragging || m_activeDragAxis >= 3)
    {
        float planeSize = 0.25f;

        // Define plane colors
        glm::vec4 planeColors[3] = {
            glm::vec4(1.0f, 1.0f, 0.2f, 0.5f),  // XY - Yellow
            glm::vec4(1.0f, 0.2f, 1.0f, 0.5f),  // YZ - Purple
            glm::vec4(0.2f, 1.0f, 1.0f, 0.5f)   // XZ - Cyan
        };

        for (int i = 0; i < 3; i++)
        {
            // Skip inactive planes if dragging
            if (m_dragging && m_activeDragAxis != i + 3)
                continue;

            // Get plane indices (the two axes that define this plane)
            int axis1 = (i + 1) % 3;
            int axis2 = (i + 2) % 3;

            // Get color, highlight if active
            glm::vec4 color = planeColors[i];
            if (m_dragging && m_activeDragAxis == i + 3)
            {
                color = glm::vec4(1.0f, 1.0f, 1.0f, 0.7f);  // White highlight
            }

            // Define plane corner points
            glm::vec3 planeCorners[4] = {
                gizmoPos + axes[axis1] * planeSize + axes[axis2] * planeSize,
                gizmoPos + axes[axis1] * planeSize - axes[axis2] * planeSize,
                gizmoPos - axes[axis1] * planeSize - axes[axis2] * planeSize,
                gizmoPos - axes[axis1] * planeSize + axes[axis2] * planeSize
            };

            // Draw plane outline
            for (int j = 0; j < 4; j++)
            {
                lineRenderer->addLine(
                    planeCorners[j],
                    planeCorners[(j + 1) % 4],
                    color,
                    1.5f  // Line width
                );
            }
        }
    }

    lineRenderer->end();
}

void TransformTool::renderRotateGizmo(Camera* camera)
{
    // Define common properties
    glm::vec3 gizmoPos = getGizmoPosition();
    glm::quat gizmoRot = getGizmoRotation();
    float radius = 1.0f;
    int circleSegments = 32;

    // Axis colors
    glm::vec4 axisColors[3] = {
        glm::vec4(1.0f, 0.2f, 0.2f, 1.0f),  // X - Red
        glm::vec4(0.2f, 1.0f, 0.2f, 1.0f),  // Y - Green
        glm::vec4(0.2f, 0.2f, 1.0f, 1.0f)   // Z - Blue
    };

    // Get line renderer
    LineBatchRenderer* lineRenderer = LineBatchRenderer::getInstance();
    lineRenderer->begin(camera->getViewMatrix(), camera->getProjectionMatrix());

    // Axis directions based on gizmo rotation
    glm::vec3 axes[3] = {
        gizmoRot * glm::vec3(1.0f, 0.0f, 0.0f),  // X
        gizmoRot * glm::vec3(0.0f, 1.0f, 0.0f),  // Y
        gizmoRot * glm::vec3(0.0f, 0.0f, 1.0f)   // Z
    };

    // Draw rotation circles for each axis
    for (int i = 0; i < 3; i++)
    {
        // Highlight active axis
        glm::vec4 color = axisColors[i];
        if (m_dragging && m_activeDragAxis == i)
        {
            color = glm::vec4(1.0f, 1.0f, 0.2f, 1.0f);  // Yellow highlight
        }

        // Get perpendicular axes to form the circle plane
        glm::vec3 circleAxis = axes[i];
        glm::vec3 perpAxis1, perpAxis2;

        if (std::abs(glm::dot(circleAxis, glm::vec3(0.0f, 1.0f, 0.0f))) < 0.99f)
        {
            perpAxis1 = glm::normalize(glm::cross(circleAxis, glm::vec3(0.0f, 1.0f, 0.0f)));
        }
        else
        {
            perpAxis1 = glm::normalize(glm::cross(circleAxis, glm::vec3(1.0f, 0.0f, 0.0f)));
        }

        perpAxis2 = glm::normalize(glm::cross(circleAxis, perpAxis1));

        // Draw circle segments
        for (int j = 0; j < circleSegments; j++)
        {
            float angle1 = j * 2.0f * glm::pi<float>() / circleSegments;
            float angle2 = (j + 1) * 2.0f * glm::pi<float>() / circleSegments;

            glm::vec3 point1 = gizmoPos + perpAxis1 * radius * std::cos(angle1) +
                perpAxis2 * radius * std::sin(angle1);

            glm::vec3 point2 = gizmoPos + perpAxis1 * radius * std::cos(angle2) +
                perpAxis2 * radius * std::sin(angle2);

            lineRenderer->addLine(point1, point2, color, 1.5f);
        }
    }

    lineRenderer->end();
}

void TransformTool::renderScaleGizmo(Camera* camera)
{
    // Define common properties
    glm::vec3 gizmoPos = getGizmoPosition();
    glm::quat gizmoRot = getGizmoRotation();
    float axisLength = 1.0f;
    float handleSize = 0.1f;

    // Axis colors
    glm::vec4 axisColors[3] = {
        glm::vec4(1.0f, 0.2f, 0.2f, 1.0f),  // X - Red
        glm::vec4(0.2f, 1.0f, 0.2f, 1.0f),  // Y - Green
        glm::vec4(0.2f, 0.2f, 1.0f, 1.0f)   // Z - Blue
    };

    // Get line renderer
    LineBatchRenderer* lineRenderer = LineBatchRenderer::getInstance();
    lineRenderer->begin(camera->getViewMatrix(), camera->getProjectionMatrix());

    // Axis directions based on gizmo rotation
    glm::vec3 axes[3] = {
        gizmoRot * glm::vec3(1.0f, 0.0f, 0.0f),  // X
        gizmoRot * glm::vec3(0.0f, 1.0f, 0.0f),  // Y
        gizmoRot * glm::vec3(0.0f, 0.0f, 1.0f)   // Z
    };

    // Draw each axis with cube handles
    for (int i = 0; i < 3; i++)
    {
        // Highlight active axis
        glm::vec4 color = axisColors[i];
        if (m_dragging && m_activeDragAxis == i)
        {
            color = glm::vec4(1.0f, 1.0f, 0.2f, 1.0f);  // Yellow highlight
        }

        // Draw axis line
        lineRenderer->addLine(
            gizmoPos,
            gizmoPos + axes[i] * axisLength,
            color,
            2.0f  // Line width
        );

        // Draw cube handle at end of axis
        glm::vec3 cubeCenter = gizmoPos + axes[i] * axisLength;
        float cubeHalfSize = handleSize * 0.5f;

        // Get perpendicular axes
        glm::vec3 perpAxis1, perpAxis2;

        if (std::abs(glm::dot(axes[i], glm::vec3(0.0f, 1.0f, 0.0f))) < 0.99f)
        {
            perpAxis1 = glm::normalize(glm::cross(axes[i], glm::vec3(0.0f, 1.0f, 0.0f)));
        }
        else
        {
            perpAxis1 = glm::normalize(glm::cross(axes[i], glm::vec3(1.0f, 0.0f, 0.0f)));
        }

        perpAxis2 = glm::normalize(glm::cross(axes[i], perpAxis1));

        // Calculate cube corners
        std::vector<glm::vec3> corners;

        for (int xi = -1; xi <= 1; xi += 2)
        {
            for (int yi = -1; yi <= 1; yi += 2)
            {
                for (int zi = -1; zi <= 1; zi += 2)
                {
                    // Convert to float for proper vector scaling
                    float x = static_cast<float>(xi);
                    float y = static_cast<float>(yi);
                    float z = static_cast<float>(zi);

                    corners.push_back(
                        cubeCenter +
                        axes[i] * glm::vec3(x * cubeHalfSize) +
                        perpAxis1 * y * cubeHalfSize +
                        perpAxis2 * z * cubeHalfSize
                    );
                }
            }
        }

        // Draw cube edges (12 edges in a cube)
        // Front face
        lineRenderer->addLine(corners[0], corners[1], color, 1.5f);
        lineRenderer->addLine(corners[1], corners[3], color, 1.5f);
        lineRenderer->addLine(corners[3], corners[2], color, 1.5f);
        lineRenderer->addLine(corners[2], corners[0], color, 1.5f);

        // Back face
        lineRenderer->addLine(corners[4], corners[5], color, 1.5f);
        lineRenderer->addLine(corners[5], corners[7], color, 1.5f);
        lineRenderer->addLine(corners[7], corners[6], color, 1.5f);
        lineRenderer->addLine(corners[6], corners[4], color, 1.5f);

        // Connecting edges
        lineRenderer->addLine(corners[0], corners[4], color, 1.5f);
        lineRenderer->addLine(corners[1], corners[5], color, 1.5f);
        lineRenderer->addLine(corners[2], corners[6], color, 1.5f);
        lineRenderer->addLine(corners[3], corners[7], color, 1.5f);
    }

    // Draw center box for uniform scaling
    glm::vec4 centerColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    if (m_dragging && m_activeDragAxis == -2)  // -2 is for center scaling
    {
        centerColor = glm::vec4(1.0f, 1.0f, 0.2f, 1.0f);  // Yellow highlight
    }

    float centerBoxSize = handleSize * 0.75f;

    // Draw center box
    // Calculate box corners in local space then transform
    std::vector<glm::vec3> centerCorners;
    for (int x = -1; x <= 1; x += 2)
    {
        for (int y = -1; y <= 1; y += 2)
        {
            for (int z = -1; z <= 1; z += 2)
            {
                glm::vec3 localCorner(x * centerBoxSize, y * centerBoxSize, z * centerBoxSize);

                // Transform by gizmo rotation
                glm::vec3 rotatedCorner = gizmoRot * localCorner;

                // Add to position
                centerCorners.push_back(gizmoPos + rotatedCorner);
            }
        }
    }

    // Draw center box edges
    // Front face
    lineRenderer->addLine(centerCorners[0], centerCorners[1], centerColor, 1.5f);
    lineRenderer->addLine(centerCorners[1], centerCorners[3], centerColor, 1.5f);
    lineRenderer->addLine(centerCorners[3], centerCorners[2], centerColor, 1.5f);
    lineRenderer->addLine(centerCorners[2], centerCorners[0], centerColor, 1.5f);

    // Back face
    lineRenderer->addLine(centerCorners[4], centerCorners[5], centerColor, 1.5f);
    lineRenderer->addLine(centerCorners[5], centerCorners[7], centerColor, 1.5f);
    lineRenderer->addLine(centerCorners[7], centerCorners[6], centerColor, 1.5f);
    lineRenderer->addLine(centerCorners[6], centerCorners[4], centerColor, 1.5f);

    // Connecting edges
    lineRenderer->addLine(centerCorners[0], centerCorners[4], centerColor, 1.5f);
    lineRenderer->addLine(centerCorners[1], centerCorners[5], centerColor, 1.5f);
    lineRenderer->addLine(centerCorners[2], centerCorners[6], centerColor, 1.5f);
    lineRenderer->addLine(centerCorners[3], centerCorners[7], centerColor, 1.5f);

    lineRenderer->end();
}