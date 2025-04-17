#include "editor/SelectionTool.h"
#include "Editor.h"
#include "Camera.h"
#include "UIManager.h"
#include "Entity.h"
#include "Scene.h"
#include "CommandManager.h"
#include "Command.h"
#include <algorithm>

SelectionTool::SelectionTool() :
    EditorTool("Selection"), m_isSelecting(false), m_isBoxSelecting(false)
{
}

SelectionTool::~SelectionTool()
{
}

void SelectionTool::activate()
{
    EditorTool::activate();
    m_isSelecting = false;
    m_isBoxSelecting = false;
}

void SelectionTool::deactivate()
{
    EditorTool::deactivate();
    m_isSelecting = false;
    m_isBoxSelecting = false;
}

void SelectionTool::update(float deltaTime)
{
    // Selection tool doesn't need to update every frame
}

void SelectionTool::renderTool(Camera* camera)
{
    // Render selection rectangle if box selecting
    if (m_isBoxSelecting)
    {
        UIManager* ui = getUIManager();
        if (ui)
        {
            // Draw selection rectangle
            ui->drawRect(
                m_selectionStart,
                m_selectionEnd - m_selectionStart,
                glm::vec4(0.2f, 0.5f, 0.9f, 0.2f),  // Fill color
                glm::vec4(0.2f, 0.6f, 1.0f, 0.8f),  // Border color
                1.0f  // Border width
            );
        }
    }
}

void SelectionTool::renderUI()
{
    UIManager* ui = getUIManager();
    if (!ui)
        return;

    if (ui->beginPanel("Selection Tool"))
    {
        ui->text("Left-click: Select entity");
        ui->text("Shift+Left-click: Add to selection");
        ui->text("Ctrl+Left-click: Toggle selection");
        ui->text("Left-drag: Box selection");
        ui->text("Ctrl+A: Select all");
        ui->text("Delete: Delete selected entities");
        ui->text("Ctrl+D: Duplicate selected entities");

        if (ui->button("Select All"))
        {
            selectAllEntities();
        }

        if (ui->button("Deselect All"))
        {
            deselectAllEntities();
        }

        if (ui->button("Delete Selected"))
        {
            deleteSelectedEntities();
        }

        if (ui->button("Duplicate Selected"))
        {
            duplicateSelectedEntities();
        }

        ui->endPanel();
    }
}

bool SelectionTool::onMouseButton(int button, int action, int mods)
{
    EditorTool::onMouseButton(button, action, mods);

    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            // Start selection
            m_isSelecting = true;
            m_isBoxSelecting = false;
            m_selectionStart = glm::vec2(m_lastMouseX, m_lastMouseY);
            m_selectionEnd = m_selectionStart;

            // If not over UI, perform selection
            if (!getUIManager()->isMouseOverUI())
            {
                bool additive = (mods & GLFW_MOD_SHIFT) != 0;
                bool toggle = (mods & GLFW_MOD_CONTROL) != 0;

                performSelection(m_lastMouseX, m_lastMouseY, additive || toggle);
                return true;
            }
        }
        else if (action == GLFW_RELEASE)
        {
            if (m_isSelecting)
            {
                // Complete box selection if active
                if (m_isBoxSelecting)
                {
                    performBoxSelection();
                }

                m_isSelecting = false;
                m_isBoxSelecting = false;
                return true;
            }
        }
    }

    return false;
}

bool SelectionTool::onMouseMove(double xpos, double ypos)
{
    EditorTool::onMouseMove(xpos, ypos);

    // Update box selection
    if (m_isSelecting)
    {
        // Update selection end point
        m_selectionEnd = glm::vec2(xpos, ypos);

        // Check if we should start box selection
        if (!m_isBoxSelecting)
        {
            // Start box selection if mouse moved enough
            float threshold = 4.0f;  // pixels
            if (glm::length(m_selectionEnd - m_selectionStart) > threshold)
            {
                m_isBoxSelecting = true;
                m_boxSelectedEntities.clear();
            }
        }

        return true;
    }

    return false;
}

bool SelectionTool::onKeyboard(int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        // Keyboard shortcuts
        if (key == GLFW_KEY_A && (mods & GLFW_MOD_CONTROL))
        {
            // Ctrl+A: Select all
            selectAllEntities();
            return true;
        }
        else if (key == GLFW_KEY_D && (mods & GLFW_MOD_CONTROL))
        {
            // Ctrl+D: Duplicate selected
            duplicateSelectedEntities();
            return true;
        }
        else if (key == GLFW_KEY_DELETE)
        {
            // Delete: Delete selected
            deleteSelectedEntities();
            return true;
        }
        else if (key == GLFW_KEY_ESCAPE)
        {
            // Escape: Deselect all
            deselectAllEntities();
            return true;
        }
    }

    return false;
}

void SelectionTool::performSelection(double xpos, double ypos, bool additive)
{
    if (!m_editor || !m_editor->getActiveScene())
        return;

    // Cast ray from mouse position
    Ray ray = getMouseRay(xpos, ypos);
    Entity* hitEntity = m_editor->getActiveScene()->raycastEntity(ray);

    // If nothing hit, deselect all if not additive
    if (!hitEntity)
    {
        if (!additive)
        {
            deselectAllEntities();
        }
        return;
    }

    // Get current selection
    const std::vector<Entity*>& currentSelection = m_editor->getSelectedEntities();

    // Check if entity is already selected
    auto it = std::find(currentSelection.begin(), currentSelection.end(), hitEntity);
    bool alreadySelected = (it != currentSelection.end());

    if (additive)
    {
        // Add to or toggle selection
        if (alreadySelected)
        {
            // Toggle: Remove from selection
            std::vector<Entity*> newSelection = currentSelection;
            newSelection.erase(std::remove(newSelection.begin(), newSelection.end(), hitEntity), newSelection.end());
            m_editor->selectEntities(newSelection);
        }
        else
        {
            // Add to selection
            std::vector<Entity*> newSelection = currentSelection;
            newSelection.push_back(hitEntity);
            m_editor->selectEntities(newSelection);
        }
    }
    else
    {
        // Set as the only selected entity
        m_editor->selectEntity(hitEntity);
    }
}

void SelectionTool::performBoxSelection()
{
    if (!m_editor || !m_editor->getActiveScene())
        return;

    // Normalize selection rectangle
    glm::vec2 min = glm::min(m_selectionStart, m_selectionEnd);
    glm::vec2 max = glm::max(m_selectionStart, m_selectionEnd);

    // Get all entities in scene
    Scene* scene = m_editor->getActiveScene();
    std::vector<Entity*> allEntities = scene->getAllEntities();

    // Check which entities are in the selection rectangle
    std::vector<Entity*> selectedEntities;
    Camera* camera = getCamera();

    for (Entity* entity : allEntities)
    {
        if (!entity)
            continue;

        // Project entity position to screen space
        glm::vec2 screenPos = camera->worldToScreenPoint(entity->getWorldPosition());

        // Check if inside selection rectangle
        if (screenPos.x >= min.x && screenPos.x <= max.x &&
            screenPos.y >= min.y && screenPos.y <= max.y)
        {
            selectedEntities.push_back(entity);
        }
    }

    // Update selection
    m_editor->getSelectedEntities(selectedEntities);
}

void SelectionTool::selectAllEntities()
{
    if (!m_editor || !m_editor->getActiveScene())
        return;

    // Get all entities in scene
    Scene* scene = m_editor->getActiveScene();
    std::vector<Entity*> allEntities = scene->getAllEntities();

    // Update selection
    m_editor->selectEntities(allEntities);
}

void SelectionTool::deselectAllEntities()
{
    if (m_editor)
    {
        m_editor->clearSelection();
    }
}

void SelectionTool::deleteSelectedEntities()
{
    if (!m_editor || !m_editor->getSelectedEntities().empty())
        return;

    Scene* scene = m_editor->getActiveScene();
    if (!scene)
        return;

    // Start a transaction for undo/redo
    m_editor->beginTransaction("Delete Entities");

    // Delete each selected entity
    for (Entity* entity : m_editor->getSelectedEntities())
    {
        if (entity)
        {
            scene->destroyEntity(entity);
        }
    }

    // Clear selection after deletion
    m_editor->clearSelection();

    // Commit the transaction
    m_editor->commitTransaction();
}

void SelectionTool::duplicateSelectedEntities()
{
    if (!m_editor || m_editor->getSelectedEntities().empty())
        return;

    Scene* scene = m_editor->getActiveScene();
    if (!scene)
        return;

    // Start a transaction for undo/redo
    m_editor->beginTransaction("Duplicate Entities");

    // Duplicate each selected entity and collect new entities
    std::vector<Entity*> newEntities;
    for (Entity* entity : m_editor->getSelectedEntities())
    {
        if (entity)
        {
            // Clone entity with slight offset
            Entity* newEntity = scene->cloneEntity(entity);
            if (newEntity)
            {
                // Offset the position slightly
                glm::vec3 position = newEntity->getPosition();
                position.x += 1.0f;
                position.z += 1.0f;
                newEntity->setPosition(position);

                newEntities.push_back(newEntity);
            }
        }
    }

    // Select the new entities
    m_editor->selectEntities(newEntities);

    // Commit the transaction
    m_editor->commitTransaction();
}