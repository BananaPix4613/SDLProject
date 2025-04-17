#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <glm.hpp>

// Forward declarations
class UIManager;
class Scene;
class Entity;
class Component;
class Camera;
class Material;
class CommandManager;
class EditorTool;
class SceneSerializer;
class VoxelEditor;
class GizmoRenderer;
class GridRenderer;
class RenderContext;
class ClusteredRenderSystem;
class CubeGrid;

/**
 * @enum EditMode
 * @brief Defines the current editing mode in the editor
 */
enum class EditMode
{
    SELECT,     // Selection mode for entities
    MOVE,       // Moving selected entities
    ROTATE,     // Rotating selected entities
    SCALE,      // Scaling selected entities
    VOXEL,      // Voxel editing mode
    TERRAIN,    // Terrain editing mode
    PAINT,      // Material/texture painting mode
    PLAY        // Play testing mode
};

/**
 * @class Editor
 * @brief Main editor system for the game engine
 *
 * The Editor provides functionality for editing and manipulating entities, components,
 * and the voxel world. It integrates with various subsystems like the CommandManager
 * for undo/redo support, SceneSerializer for saving/loading, and specialized editors
 * for specific tasks like VoxelEditor.
 */
class Editor
{
public:
    /**
     * @brief Constructor
     */
    Editor();

    /**
     * @brief Destructor
     */
    ~Editor();

    /**
     * @brief Initialize the editor
     * @param uiManager Pointer to UI manager
     * @param scene Pointer to scene
     * @param camera Pointer to camera
     * @param renderer Pointer to render system
     * @param grid Pointer to cube grid
     * @return True if initialization succeeded
     */
    bool initialize(UIManager* uiManager, Scene* scene, Camera* camera,
                    ClusteredRenderSystem* renderer, CubeGrid* grid);

    /**
     * @brief Update editor state
     * @param deltaTime Time elapsed since last update
     */
    void update(float deltaTime);

    /**
     * @brief Render editor UI and visualizations
     */
    void render();

    /**
     * @brief Process editor-specific input
     */
    void processInput();

    /**
     * @brief Set the current edit mode
     * @param mode New edit mode
     */
    void setEditMode(EditMode mode);

    /**
     * @brief Get the current edit mode
     * @return Current edit mode
     */
    EditMode getEditMode() const;

    /**
     * @brief Select an entity
     * @param entity Entity to select
     */
    void selectEntity(Entity* entity);

    /**
     * @brief Add an entity to the current selection
     * @param entity Entity to add to selection
     */
    void addEntityToSelection(Entity* entity);

    /**
     * @brief Clear the current entity selection
     */
    void clearSelection();

    /**
     * @brief Get the currently selected entity
     * @return Pointer to selected entity or nullptr if none selected
     */
    Entity* getSelectedEntity() const;

    /**
     * @brief Get all currently selected entities
     * @return Vector of selected entities
     */
    const std::vector<Entity*>& getSelectedEntities() const;

    /**
     * @brief Select entities by tag
     * @param tag Tag to select
     */
    void selectEntitiesByTag(const std::string& tag);

    /**
     * @brief Begin an undoable transaction
     * @param name Name of the transaction
     */
    void beginTransaction(const std::string& name);

    /**
     * @brief Commit the current transaction
     */
    void commitTransaction();

    /**
     * @brief Abort the current transaction
     */
    void abortTransaction();

    /**
     * @brief Undo the last transaction
     */
    void undo();

    /**
     * @brief Redo the last undone transaction
     */
    void redo();

    /**
     * @brief Save the current scene to a file
     * @param filename Path to save the scene to
     * @return True if save succeeded
     */
    bool saveScene(const std::string& filename);

    /**
     * @brief Load a scene from a file
     * @param filename Path to the scene file
     * @return True if load succeeded
     */
    bool loadScene(const std::string& filename);

    /**
     * @brief Create a new empty scene
     */
    void newScene();

    /**
     * @brief Create a new entity
     * @param name Name for the new entity
     * @return Pointer to the created entity
     */
    Entity* createEntity(const std::string& name = "Entity");

    /**
     * @brief Duplicate the selected entity
     * @return Pointer to the duplicated entity
     */
    Entity* duplicateSelectedEntity();

    /**
     * @brief Delete the selected entity
     */
    void deleteSelectedEntity();

    /**
     * @brief Get the scene being edited
     * @return Pointer to the scene
     */
    Scene* getScene() const;

    /**
     * @brief Get the editor camera
     * @return Pointer to the camera
     */
    Camera* getCamera() const;

    /**
     * @brief Set the editor camera
     * @param camera Pointer to the camera
     */
    void setCamera(Camera* camera);

    /**
     * @brief Get the voxel editor
     * @return Pointer to the voxel editor
     */
    VoxelEditor* getVoxelEditor() const;

    /**
     * @brief Set grid visibility
     * @param visible Whether the grid is visible
     */
    void setGridVisible(bool visible);

    /**
     * @brief Is grid visible
     * @return True if grid is visible
     */
    bool isGridVisible() const;

    /**
     * @brief Set the grid cell size
     * @param size Grid cell size
     */
    void setGridCellSize(float size);

    /**
     * @brief Get the grid cell size
     * @return Grid cell size
     */
    float getGridCellSize() const;

    /**
     * @brief Set the grid color
     * @param color Grid color
     */
    void setGridColor(const glm::vec3& color);

    /**
     * @brief Get the grid color
     * @return Grid color
     */
    glm::vec3 getGridColor() const;

    /**
     * @brief Focus camera on selected entity
     */
    void focusOnSelected();

    /**
     * @brief Set entity visibility
     * @param entity Entity to modify
     * @param visible Whether entity is visible
     */
    void setEntityVisible(Entity* entity, bool visible);

    /**
     * @brief Register an editor tool
     * @param tool Pointer to the tool
     */
    void registerTool(std::shared_ptr<EditorTool> tool);

    /**
     * @brief Get a registered tool by name
     * @param name Name of the tool
     * @return Pointer to the tool or nullptr if not found
     */
    EditorTool* getTool(const std::string& name) const;

    /**
     * @brief Toggle the active state of a tool
     * @param name Name of the tool
     * @param active Whether the tool should be active
     */
    void setToolActive(const std::string& name, bool active);

    /**
     * @brief Get the command manager
     * @return Pointer to the command manager
     */
    CommandManager* getCommandManager() const;

    /**
     * @brief Set editor grid visible
     * @param visible Whether grid is visible
     */
    void setShowGrid(bool visible);

    /**
     * @brief Set gizmos visible
     * @param visible Whether gizmos are visible
     */
    void setShowGizmos(bool visible);

    /**
     * @brief Is play mode active
     * @return True if in play mode
     */
    bool isPlayModeActive() const;

    /**
     * @brief Toggle play mode
     */
    void togglePlayMode();

    /**
     * @brief Set snap to grid enabled
     * @param enabled Whether snap to grid is enabled
     */
    void setSnapToGrid(bool enabled);

    /**
     * @brief Is snap to grid enabled
     * @return True if snap to grid is enabled
     */
    bool isSnapToGridEnabled() const;

    /**
     * @brief Set snap rotation enabled
     * @param enabled Whether snap rotation is enabled
     */
    void setSnapRotation(bool enabled);

    /**
     * @brief Is snap rotation enabled
     * @return True if snap rotation is enabled
     */
    bool isSnapRotationEnabled() const;

    /**
     * @brief Set rotation snap angle
     * @param degrees Angle in degrees for rotation snapping
     */
    void setRotationSnapAngle(float degrees);

    /**
     * @brief Get rotation snap angle
     * @return Angle in degrees for rotation snapping
     */
    float getRotationSnapAngle() const;

    /**
     * @brief Set position snap distance
     * @param distance Distance for position snapping
     */
    void setPositionSnapDistance(float distance);

    /**
     * @brief Get position snap distance
     * @return Distance for position snapping
     */
    float getPositionSnapDistance() const;

    /**
     * @brief Set scale snap value
     * @param value Scale for scale snapping
     */
    void setScaleSnapValue(float value);

    /**
     * @brief Get scale snap value
     * @return Value for scale snapping
     */
    float getScaleSnapValue() const;

    /**
     * @brief Set local transformation mode
     * @param useLocalTransform Whether to use local or world transformations
     */
    void setUseLocalTransform(bool useLocalTransform);

    /**
     * @brief Is using local transformation mode
     * @return True if using local transformations
     */
    bool isUsingLocalTransform() const;

    /**
     * @brief Register callback for entity selection
     * @param callback Function to call when selection changes
     * @return ID for the callback
     */
    int registerSelectionCallback(std::function<void(Entity*)> callback);

    /**
     * @brief Unregister a selection callback
     * @param id ID of the callback to remove
     */
    void unregisterSelectionCallback(int id);

private:
    // Core references
    UIManager* m_uiManager;
    Scene* m_scene;
    Camera* m_camera;
    ClusteredRenderSystem* m_renderer;
    CubeGrid* m_grid;

    // Editor state
    EditMode m_editMode;
    std::vector<Entity*> m_selectedEntities;
    bool m_playModeActive;

    // Snapping settings
    bool m_snapToGrid;
    bool m_snapRotation;
    float m_rotationSnapAngle;
    float m_positionSnapDistance;
    float m_scaleSnapValue;
    bool m_useLocalTransform;

    // Grid settings
    bool m_gridVisible;
    float m_gridCellSize;
    glm::vec3 m_gridColor;

    // SubEditors
    std::unique_ptr<VoxelEditor> m_voxelEditor;
    std::unique_ptr<CommandManager> m_commandManager;
    std::unique_ptr<SceneSerializer> m_sceneSerializer;
    std::unique_ptr<GizmoRenderer> m_gizmoRenderer;
    std::unique_ptr<GridRenderer> m_gridRenderer;

    // Tools
    std::unordered_map<std::string, std::shared_ptr<EditorTool>> m_tools;
    std::vector<EditorTool*> m_activeTools;

    // Selection callbacks
    struct SelectionCallback
    {
        int id;
        std::function<void(Entity*)> callback;
    };
    std::vector<SelectionCallback> m_selectionCallbacks;
    int m_nextCallbackId;

    // Temporary state for operations
    glm::vec3 m_lastMouseWorldPos;
    glm::vec3 m_transformStartPos;
    glm::quat m_transformStartRot;
    glm::vec3 m_transformStartScale;
    bool m_isDraggingGizmo;

    // UI Components
    void renderMainMenu();
    void renderToolbar();
    void renderSceneHierarchy();
    void renderInspector();
    void renderPropertiesPanel();
    void renderVoxelTools();
    void renderSettingsPanel();
    void renderViewportOverlay();
    void renderStatsPanel();
    void renderConsole();

    // Modal dialogs
    void showSaveDialog();
    void showLoadDialog();
    void showNewSceneDialog();
    void showSettingsDialog();
    void showAboutDialog();

    // Action handlers
    void handleSceneCreation();
    void handleEntityCreation();
    void handleEntityDeletion();
    void handleEntityDuplication();
    void handleModeChange(EditMode newMode);

    // Gizmo and transformation helpers
    bool handleGizmoInteraction();
    void updateGizmoTransformation();
    void applyTransformation();
    void cancelTransformation();

    // Selection helpers
    void notifySelectionChanged();
    Entity* pickEntityAtScreenPos(int x, int y);
    void selectEntitiesInRect(const glm::vec2& start, const glm::vec2& end);

    // Utility methods
    glm::vec3 screenToWorldRay(int screenX, int screenY);
    glm::vec3 snapPosition(const glm::vec3& position);
    glm::quat snapRotation(const glm::quat& rotation);
    glm::vec3 snapScale(const glm::vec3& scale);

    // Play mode helpers
    void enterPlayMode();
    void exitPlayMode();
    void saveSceneState();
    void restoreSceneState();
    std::string m_playModeStateFile;
};