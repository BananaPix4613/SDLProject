#pragma once

#include "EditorTool.h"
#include <vector>

/**
 * @class SelectionTool
 * @brief Tool for selecting entities in the scene
 * 
 * Allows users to select entities by clicking on them in the viewport,
 * handles multi-selection with modifier keys, and selection rectangle.
 */
class SelectionTool : public EditorTool
{
public:
    /**
     * @brief Constructor
     */
    SelectionTool();

    /**
     * @brief Destructor
     */
    ~SelectionTool() override;

    /**
     * @brief Handle tool activation
     */
    void activate() override;

    /**
     * @brief Handle tool deactivation
     */
    void deactivate() override;

    /**
     * @brief Update selection tool
     * @param deltaTime Time elapsed since last update
     */
    void update(float deltaTime) override;

    /**
     * @brief Render selection visualization
     * @param camera Camera used for rendering
     */
    void renderTool(Camera* camera) override;

    /**
     * @brief Render selection UI elements
     */
    void renderUI() override;

    /**
     * @brief Handle mouse button events
     * @param button Mouse button
     * @param action Action (press, release)
     * @param mods Keyboard modifiers
     * @return True if the event was handled
     */
    bool onMouseButton(int button, int action, int mods) override;

    /**
     * @brief Handle mouse movement events
     * @param xpos Mouse X position
     * @param ypos Mouse Y position
     * @return True if the event was handled
     */
    bool onMouseMove(double xpos, double ypos) override;

    /**
     * @brief Handle keyboard events
     * @param key Key code
     * @param scancode System-specific scancode
     * @param action Action (press, release, repeat)
     * @param mods Keyboard modifiers
     * @return True if the event was handled
     */
    bool onKeyboard(int key, int scancode, int action, int mods) override;

private:
    // Selection state
    bool m_isSelecting;
    bool m_isBoxSelecting;
    glm::vec2 m_selectionStart;
    glm::vec2 m_selectionEnd;

    // Entities in current box selection
    std::vector<Entity*> m_boxSelectedEntities;

    // Helper methods
    void performSelection(double xpos, double ypos, bool additive);
    void performBoxSelection();
    void selectAllEntities();
    void deselectAllEntities();
    void deleteSelectedEntities();
    void duplicateSelectedEntities();
};