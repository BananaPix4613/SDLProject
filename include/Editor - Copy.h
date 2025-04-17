#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <Entity.h>
#include <Scene.h>
#include <Camera.h>

// Forward declarations
class UIManager;
class CommandManager;
class Command;
class EditorTool;
template <typename T> class Grid;
class CubeGrid;
class LineBatchRenderer;
class GizmoRenderer;

/**
 * @enum EditMode
 * @brief Defines different editor modes for different types of editing operations
 */
enum class EditMode
{
    ENTITY,     // Entity creation, selection, and property editing
    VOXEL,      // Voxel placement and editing
    TERRAIN,    // Terrain height and texture editing
    LIGHTING,   // Light placement and parameter adjustment
    PLAYTEST    // In-editor gameplay testing
};

/**
 * @class Editor
 * @brief Main editor system for managing editing tools, selection, and user operations
 * 
 * The Editor class is the central hub for all editor functionality, providing
 * entity selection, editing mode management, undo/redo support, and tool coordination.
 */
class Editor
{
public:
    /**
     * @brief Initialize the editor system
     * @return True if initialization succeeded
     */
    bool initialize();

    /**
     * @brief Update editor state based on input and time
     * @param deltaTime Time elapsed since last update
     */
    void update(float deltaTime);

    /**
     * @brief Render editor UI and gizmos
     */
    void render();

    /**
     * @brief Set the active editing mode
     * @param mode The editing mode to activate
     */
    void setEditMode(EditMode mode);

    /**
     * @brief Get the current editing mode
     * @return Current editing mode
     */
    EditMode getEditMode() const;

    /**
     * @brief Select an entity for editing
     * @param entity Pointer to entity to select (nullptr to clear selection)
     */
    void selectEntity(Entity* entity);

    /**
     * @brief Get the currently selected entity
     * @return Pointers to selected entity, or nullptr if nothing selected
     */
    Entity* getSelectedEntity() const;

    /**
     * @brief Select multiple entities
     * @param entities Vector of entity pointers to select
     */
    void selectEntities(const std::vector<Entity*>& entities);

    /**
     * @brief Get all selected entities
     * @return Vector of currently selected entities
     */
    const std::vector<Entity*>& getSelectedEntities() const;

    /**
     * @brief Clear all entity selections
     */
    void clearSelection();

    /**
     * @brief Begin a transaction for grouping undo/redo operations
     * @param name Name of the transaction
     */
    void beginTransaction(const std::string& name);

    /**
     * @brief Commit the current transaction to the undo stack
     */
    void commitTransaction();

    /**
     * @brief Abort the current transaction without committing
     */
    void abortTransaction();

    /**
     * @brief Undo the last committed transaction
     */
    void undo();

    /**
     * @brief Redo the last undone transaction
     */
    void redo();

    /**
     * @brief Get reference to the active scene
     * @return Pointer to the current scene
     */
    Scene* getActiveScene() const;

    /**
     * @brief Set the active scene for editing
     * @param scene Pointer to scene to edit
     */
    void setActiveScene(Scene* scene);

    /**
     * @brief Create a new empty scene
     * @param name Name for the new scene
     * @return Pointer to the newly created scene
     */
    Scene* createNewScene(const std::string& name);

    /**
     * @brief Get the editor camera
     * @return Pointer to the editor camera
     */
    Camera* getEditorCamera() const;

    /**
     * @brief Set grid snap settings
     * @param enable Whether grid snapping is enabled
     * @param size Grid cell size for snapping
     */
    void setGridSnap(bool enable, float size = 1.0f);

    /**
     * @brief Get the current grid snap settings
     * @param outEnabled Output parameter for snap enabled state
     * @return Grid snap size
     */
    float getGridSnap(bool& outEnabled) const;

    /**
     * @brief Set pixel grid alignment for precise pixel art editing
     * @param enable Whether pixel grid alignment is enabled
     * @param pixelSize Virtual pixel size for snapping
     */
    void setPixelGridAlignment(bool enable, int pixelSize = 1);

    /**
     * @brief Get the current pixel grid alignment settings
     * @param outEnabled Output parameter for alignment enabled state
     * @return Virtual pixel size
     */
    int getPixelGridAlignment(bool& outEnabled) const;

    /**
     * @brief Register an editor tool
     * @param tool Pointer to the tool is register
     * @param mode Edit mode this tool is associated with
     */
    void registerTool(EditorTool* tool, EditMode mode);

    /**
     * @brief Set active tool for current edit mode
     * @param toolName Name of the tool to activate
     * @return True if tool was found and activated
     */
    bool setActiveTool(const std::string& toolName);

    /**
     * @brief Get the active tool name
     * @return Name of currently active tool
     */
    std::string getActiveToolName() const;

    /**
     * @brief Show the editor UI for a specific component
     * @param component Pointer to component to edit
     * @return True if component UI was shown
     */
    bool showComponentEditor(Component* component);

    /**
     * @brief Save the current scene
     * @param filename Path to save the scene to
     * @return True if save succeeded
     */
    bool saveScene(const std::string& filename);

    /**
     * @brief Save the current scene
     * @param filename Path to save the scene to
     * @return True if save succeeded
     */
    bool loadScene(const std::string& filename);

    /**
     * @brief Get the UI manager reference
     * @return Pointer to the UI manager
     */
    UIManager* getUIManager() const;

    /**
     * @brief Set whether to show the grid
     * @param show Whether to show the grid
     */
    void showGrid(bool show);

    /**
     * @brief Check if grid is currently visible
     * @return True if grid is visible
     */
    bool isGridVisible() const;

    /**
     * @brief Check if mouse is currently over UI
     * @return True if mouse is over UI element
     */
    bool isMouseOverUI() const;

    /**
     * @brief Check if a key is currently pressed
     * @param keyCode GLFW key code
     * @return True if key is pressed
     */
    bool isKeyPressed(int keyCode) const;

    /**
     * @brief Check if a mouse button is pressed
     * @param button Mouse button (0 = left, 1 = right, 2 = middle)
     * @return True if button is pressed
     */
    bool isMouseButtonPressed(int button) const;

    /**
     * @brief Get current mouse position
     * @return Mouse position in screen coordinates
     */
    glm::vec2 getMousePosition() const;

    /**
     * @brief Get mouse delta since last frame
     * @return Mouse movement delta
     */
    glm::vec2 getMouseDelta() const;

private:
    // Core editor state
    EditMode m_currentEditMode;
    Scene* m_activeScene;
    Camera* m_editorCamera;
    Entity* m_selectedEntity;
    std::vector<Entity*> m_selectedEntities;

    // Editor tools
    std::unordered_map<EditMode, std::vector<EditorTool*>> m_tools;
    EditorTool* m_activeTool;

    // Grid settings
    bool m_gridSnapEnabled;
    float m_gridSnapSize;
    bool m_pixelGridEnabled;
    int m_pixelGridSize;
    bool m_showGrid;

    // Undo/redo system
    CommandManager* m_commandManager;
    bool m_inTransaction;
    std::string m_currentTransactionName;

    /**
     * @brief Handle entity selection via ray cast
     */
    void handleSelectionInput();

    /**
     * @brief Update the active tool
     * @param deltaTime Time elapsed since last update
     */
    void updateActiveTool(float deltaTime);

    /**
     * @brief Render the editor grid
     */
    void renderEditorGrid();

    /**
     * @brief Render gizmos for selected entities
     */
    void renderSelectionGizmos();

    /**
     * @brief Initialize default tools for each edit mode
     */
    void initializeDefaultTools();
};