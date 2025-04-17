#include "Editor.h"
#include "UIManager.h"
#include "Scene.h"
#include "Entity.h"
#include "Component.h"
#include "Camera.h"
#include "VoxelEditor.h"
#include "CommandManager.h"
#include "SceneSerializer.h"
#include "CubeGrid.h"
#include "EditorTool.h"
#include "GizmoRenderer.h"
#include "GridRenderer.h"
#include "RenderContext.h"
#include "ClusteredRenderSystem.h"
#include "Ray.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

// Constructor
Editor::Editor() :
    m_uiManager(nullptr),
    m_scene(nullptr),
    m_camera(nullptr),
    m_renderer(nullptr),
    m_grid(nullptr),
    m_editMode(EditMode::SELECT),
    m_playModeActive(false),
    m_snapToGrid(false),
    m_snapRotation(false),
    m_rotationSnapAngle(15.0f),
    m_positionSnapDistance(1.0f),
    m_scaleSnapValue(0.25f),
    m_useLocalTransform(true),
    m_gridVisible(true),
    m_gridCellSize(1.0f),
    m_gridColor(0.5f, 0.5f, 0.5f),
    m_nextCallbackId(1),
    m_isDraggingGizmo(false),
    m_playModeStateFile("temp_playmode_scene.json")
{
}

// Destructor
Editor::~Editor()
{
    // Clean up temporary files
    if (std::filesystem::exists(m_playModeStateFile))
    {
        std::filesystem::remove(m_playModeStateFile);
    }
}

// Initialize the editor
bool Editor::initialize(UIManager* uiManager, Scene* scene, Camera* camera,
                        ClusteredRenderSystem* renderer, CubeGrid* grid)
{
    m_uiManager = uiManager;
    m_scene = scene;
    m_camera = camera;
    m_renderer = renderer;
    m_grid = grid;

    // Initialize sub-editors and managers
    m_commandManager = std::make_unique<CommandManager>();
    m_sceneSerializer = std::make_unique<SceneSerializer>();
    m_gridRenderer = std::make_unique<GridRenderer>();
    m_gizmoRenderer = std::make_unique<GizmoRenderer>();

    // Initialize voxel editor if we have a grid
    if (m_grid)
    {
        m_voxelEditor = std::make_unique<VoxelEditor>();
        m_voxelEditor->initialize(m_grid);
        m_voxelEditor->setCamera(m_camera);
        m_voxelEditor->setCommandManager(m_commandManager.get());
    }

    // Set up grid renderer
    m_gridRenderer->initialize(m_gridCellSize, m_gridColor);

    // Set up gizmo renderer
    m_gizmoRenderer->initialize(m_camera);

    return true;
}

// Update editor state
void Editor::update(float deltaTime)
{
    // Skip if in play mode
    if (m_playModeActive)
        return;

    // Process input for editor
    processInput();

    // Update active tools
    for (auto* tool : m_activeTools)
    {
        if (tool)
        {
            tool->update(deltaTime);
        }
    }

    // Update voxel editor if in voxel mode
    if (m_editMode == EditMode::VOXEL && m_voxelEditor)
    {
        m_voxelEditor->update(deltaTime);
    }

    // Update gizmo transformation if dragging
    if (m_isDraggingGizmo)
    {
        updateGizmoTransformation();
    }
}

// Render editor UI and visualizations
void Editor::render()
{
    // Don't show editor UI in play mode (except for overlay)
    if (m_playModeActive)
    {
        renderViewportOverlay();
        return;
    }

    // Start ImGui frame via the UIManager
    m_uiManager->beginFrame();

    // Render main dockspace
    m_uiManager->beginDockspace("EditorDockspace");

    // Render all UI panels
    renderMainMenu();
    renderToolbar();
    renderSceneHierarchy();
    renderInspector();
    renderPropertiesPanel();

    // Render editor mode-specific panels
    if (m_editMode == EditMode::VOXEL)
    {
        renderVoxelTools();
    }

    renderSettingsPanel();
    renderStatsPanel();
    renderConsole();
    renderViewportOverlay();

    // Render any active tools UI
    for (auto* tool : m_activeTools)
    {
        if (tool)
        {
            tool->renderUI(m_uiManager);
        }
    }

    // End dockspace
    m_uiManager->endDockspace();

    // Render 3D visualizations
    renderGridAndGizmos();

    // End ImGui frame
    m_uiManager->endFrame();
}

// Process editor-specific input
void Editor::processInput()
{
    // Handle editor keyboard shortcuts
    if (m_uiManager->isKeyPressed('Z') && m_uiManager->isKeyModifiers(UIManager::KeyModifier::CTRL))
    {
        if (m_uiManager->isKeyModifier(UIManager::KeyModifier::SHIFT))
        {
            redo();
        }
        else
        {
            undo();
        }
    }

    // Delete selected entity
    if (m_uiManager->isKeyPressed(UIManager::Key::DELETE) && !m_selectedEntities.empty())
    {
        deleteSelectedEntity();
    }

    // Duplicate selected entity
    if (m_uiManager->isKeyPressed('D') && m_uiManager->isKeyModifier(UIManager::KeyModifier::CTRL))
    {
        duplicateSelectedEntity();
    }

    // Save scene
    if (m_uiManager->isKeyPressed('S') && m_uiManager->isKeyModifier(UIManager::KeyModifier::CTRL))
    {
        if (m_uiManager->isKeyModifier(UIManager::KeyModifier::SHIFT))
        {
            showSaveDialog();
        }
        else
        {
            saveScene("quicksave.scene");
        }
    }

    // Load scene
    if (m_uiManager->isKeyPressed('O') && m_uiManager->isKeyModifier(UIManager::KeyModifier::CTRL))
    {
        showLoadDialog();
    }

    // New scene
    if (m_uiManager->isKeyPressed('N') && m_uiManager->isKeyModifier(UIManager::KeyModifier::CTRL))
    {
        showNewSceneDialog();
    }

    // Toggle play mode
    if (m_uiManager->isKeyPressed(UIManager::Key::F5))
    {
        togglePlayMode();
    }

    // Focus on selected entity
    if (m_uiManager->isKeyPressed('F') && !m_selectedEntities.empty())
    {
        focusOnSelected();
    }

    // Edit mode shortcuts
    if (m_uiManager->isKeyPressed('1')) setEditMode(EditMode::SELECT);
    if (m_uiManager->isKeyPressed('2')) setEditMode(EditMode::MOVE);
    if (m_uiManager->isKeyPressed('3')) setEditMode(EditMode::ROTATE);
    if (m_uiManager->isKeyPressed('4')) setEditMode(EditMode::SCALE);
    if (m_uiManager->isKeyPressed('5')) setEditMode(EditMode::VOXEL);
    if (m_uiManager->isKeyPressed('6')) setEditMode(EditMode::TERRAIN);
    if (m_uiManager->isKeyPressed('7')) setEditMode(EditMode::PAINT);

    // Process viewport input (entity selection, gizmo manipulation)
    if (m_uiManager->isMouseButtonPressed(0) && m_uiManager->isHoveringPanel("Viewport"))
    {
        // Handle gizmo interaction first
        if (!handleGizmoInteraction())
        {
            // If not interacting with gizmo, do entity selection
            int mouseX, mouseY;
            m_uiManager->getMousePosition(mouseX, mouseY);

            // Get entity under cursor
            Entity* pickedEntity = pickEntityAtScreenPos(mouseX, mouseY);

            // Select the entity
            if (pickedEntity)
            {
                if (m_uiManager->isKeyModifier(UIManager::KeyModifier::SHIFT))
                {
                    // Add to selection
                    addEntityToSelection(pickedEntity);
                }
                else
                {
                    // Replace selection
                    selectEntity(pickedEntity);
                }
            }
            else if (!m_uiManager->isKeyModifier(UIManager::KeyModifier::SHIFT))
            {
                // Clear selection if clicked on nothing without SHIFT
                clearSelection();
            }
        }
    }
    else if (m_uiManager->isMouseButtonReleased(0) && m_isDraggingGizmo)
    {
        // Finalize transformation when releasing mouse after gizmo drag
        m_isDraggingGizmo = false;
        applyTransformation();
    }
}

// Set the current edit mode
void Editor::setEditMode(EditMode mode)
{
    // Skip if same mode
    if (m_editMode == mode)
        return;

    // Handle exiting previous mode
    switch (m_editMode)
    {
        case EditMode::VOXEL:
            // Exit voxel mode
            if (m_voxelEditor)
            {
                // Save any pending voxel edits
            }
            break;
        default:
            break;
    }

    // Set new mode
    m_editMode = mode;

    // Handle entering new mode
    switch (m_editMode)
    {
        case EditMode::VOXEL:
            // Enter voxel mode
            if (m_voxelEditor)
            {
                // Setup voxel editor
            }
            break;
        default:
            break;
    }

    // Notify tools about mode change
    for (auto* tool : m_activeTools)
    {
        if (tool)
        {
            tool->onEditModeChanged(m_editMode);
        }
    }
}

// Get the current edit mode
EditMode Editor::getEditMode() const
{
    return m_editMode;
}

// Select an entity
void Editor::selectEntity(Entity* entity)
{
    // Clear current selection
    m_selectedEntities.clear();

    // Add entity if valid
    if (entity)
    {
        m_selectedEntities.push_back(entity);
    }

    // Notify about selection change
    notifySelectionChanged();
}

// Add an entity to the current selection
void Editor::addEntityToSelection(Entity* entity)
{
    // Skip if already selected
    auto it = std::find(m_selectedEntities.begin(), m_selectedEntities.end(), entity);
    if (it != m_selectedEntities.end())
        return;

    // Add to selection
    m_selectedEntities.push_back(entity);

    // Notify about selection change
    notifySelectionChanged();
}

// Clear the current entity selection
void Editor::clearSelection()
{
    // Skip if already empty
    if (m_selectedEntities.empty())
        return;

    // Clear selection
    m_selectedEntities.clear();

    // Notify about selection change
    notifySelectionChanged();
}

// Get the currently selected entity
Entity* Editor::getSelectedEntity() const
{
    if (m_selectedEntities.empty())
        return nullptr;

    return m_selectedEntities[0];
}

// Get all currently selected entities
const std::vector<Entity*>& Editor::getSelectedEntities() const
{
    return m_selectedEntities;
}

// Select entities by tag
void Editor::selectEntitiesByTag(const std::string& tag)
{
    // Skip if no scene
    if (!m_scene)
        return;

    // Clear current selection
    m_selectedEntities.clear();

    // Find entities with tag
    auto entitiesWithTag = m_scene->findEntitiesByTag(tag);

    // Add to selection
    m_selectedEntities = entitiesWithTag;

    // Notify about selection change
    notifySelectionChanged();
}

// Begin an undoable transaction
void Editor::beginTransaction(const std::string& name)
{
    if (m_commandManager)
    {
        m_commandManager->beginTransaction(name);
    }
}

// Commit the current transaction
void Editor::commitTransaction()
{
    if (m_commandManager)
    {
        m_commandManager->commitTransaction();
    }
}

// Abort the current transaction
void Editor::abortTransaction()
{
    if (m_commandManager)
    {
        m_commandManager->abortTransaction();
    }
}

// Undo the last transaction
void Editor::undo()
{
    if (m_commandManager)
    {
        m_commandManager->undo();
    }
}

// Redo the last undone transaction
void Editor::redo()
{
    if (m_commandManager)
    {
        m_commandManager->redo();
    }
}

// Save the current scene to a file
bool Editor::saveScene(const std::string& filename)
{
    if (m_scene && m_sceneSerializer)
    {
        return m_sceneSerializer->saveScene(filename, m_scene);
    }
    return false;
}

// Load a scene from a file
bool Editor::loadScene(const std::string& filename)
{
    if (m_scene && m_sceneSerializer)
    {
        // Clear selection
        clearSelection();

        // Load scene
        return m_sceneSerializer->loadScene(filename, m_scene);
    }
    return false;
}

// Create a new empty scene
void Editor::newScene()
{
    // Clear selection
    clearSelection();

    // Clear scene
    if (m_scene)
    {
        m_scene->clear();
    }

    // Setup default scene (camera, light, etc.)
    handleSceneCreation();
}

// Create a new entity
Entity* Editor::createEntity(const std::string& name)
{
    if (m_scene)
    {
        return m_scene->createEntity(name);
    }
    return nullptr;
}

// Duplicate the selected entity
Entity* Editor::duplicateSelectedEntity()
{
    Entity* selectedEntity = getSelectedEntity();
    if (!selectedEntity || !m_scene)
        return nullptr;

    // Start transaction
    beginTransaction("Duplicate Entity");

    // Duplicate entity
    Entity* newEntity = m_scene->createEntity(selectedEntity->getName() + "_copy");

    // Clone components
    selectedEntity->cloneComponentsTo(newEntity);

    // Set position offset
    newEntity->setPosition(selectedEntity->getPosition() + glm::vec3(2.0f, 0.0f, 0.0f));

    // End transaction
    commitTransaction();

    // Select the new entity
    selectEntity(newEntity);

    return newEntity;
}

// Delete the selected entity
void Editor::deleteSelectedEntity()
{
    if (m_selectedEntities.empty() || !m_scene)
        return;

    // Start transaction
    beginTransaction("Delete Entity");

    // Store the indices for deletion in reverse order to avoid index shifting
    std::vector<Entity*> entitiesToDelete = m_selectedEntities;

    // Clear selection before deletion
    clearSelection();

    // Delete entities
    for (auto* entity : entitiesToDelete)
    {
        m_scene->getEntityManager()->destroyEntity(entity);
    }

    // End transaction
    commitTransaction();
}

// Get the scene being edited
Scene* Editor::getScene() const
{
    return m_scene;
}

// Get the editor camera
Camera* Editor::getCamera() const
{
    return m_camera;
}

// Set the editor camera
void Editor::setCamera(Camera* camera)
{

}

VoxelEditor* Editor::getVoxelEditor() const
{
    return nullptr;
}

void Editor::setGridVisible(bool visible)
{
}

bool Editor::isGridVisible() const
{
    return false;
}

void Editor::setGridCellSize(float size)
{
}

float Editor::getGridCellSize() const
{
    return 0.0f;
}

void Editor::setGridColor(const glm::vec3& color)
{
}

glm::vec3 Editor::getGridColor() const
{
    return glm::vec3();
}

void Editor::focusOnSelected()
{
}

void Editor::setEntityVisible(Entity* entity, bool visible)
{
}

void Editor::registerTool(std::shared_ptr<EditorTool> tool)
{
}

EditorTool* Editor::getTool(const std::string& name) const
{
    return nullptr;
}

void Editor::setToolActive(const std::string& name, bool active)
{
}

CommandManager* Editor::getCommandManager() const
{
    return nullptr;
}

void Editor::setShowGrid(bool visible)
{
}

void Editor::setShowGizmos(bool visible)
{
}

bool Editor::isPlayModeActive() const
{
    return false;
}

void Editor::togglePlayMode()
{
}

void Editor::setSnapToGrid(bool enabled)
{
}

bool Editor::isSnapToGridEnabled() const
{
    return false;
}

void Editor::setSnapRotation(bool enabled)
{
}

bool Editor::isSnapRotationEnabled() const
{
    return false;
}

void Editor::setRotationSnapAngle(float degrees)
{
}

float Editor::getRotationSnapAngle() const
{
    return 0.0f;
}

void Editor::setPositionSnapDistance(float distance)
{
}

float Editor::getPositionSnapDistance() const
{
    return 0.0f;
}

void Editor::setScaleSnapValue(float value)
{
}

float Editor::getScaleSnapValue() const
{
    return 0.0f;
}

void Editor::setUseLocalTransform(bool useLocalTransform)
{
}

bool Editor::isUsingLocalTransform() const
{
    return false;
}

int Editor::registerSelectionCallback(std::function<void(Entity*)> callback)
{
    return 0;
}

void Editor::unregisterSelectionCallback(int id)
{
}

void Editor::renderMainMenu()
{
}

void Editor::renderToolbar()
{
}

void Editor::renderSceneHierarchy()
{
}

void Editor::renderInspector()
{
}

// Properties panel rendering
void Editor::renderPropertiesPanel()
{
    if (!m_uiManager->beginPanel("Properties", UIManager::PanelFlags::DEFAULT))
        return;

    // Render general editor properties
    if (m_uiManager->beginSection("Transform Settings", true))
    {
        // Snap settings
        bool snapToGrid = m_snapToGrid;
        if (m_uiManager->checkbox("Snap to Grid", &snapToGrid))
        {
            setSnapToGrid(snapToGrid);
        }

        if (snapToGrid)
        {
            float snapDistance = m_positionSnapDistance;
            if (m_uiManager->dragFloat("Grid Size", &snapDistance, 0.1f, 0.1f, 10.0f))
            {
                setPositionSnapDistance(snapDistance);
            }
        }

        bool snapRotation = m_snapRotation;
        if (m_uiManager->checkbox("Snap Rotation", &snapRotation))
        {
            setSnapRotation(snapRotation);
        }

        if (snapRotation)
        {
            float snapAngle = m_rotationSnapAngle;
            if (m_uiManager->dragFloat("Angle Snap", &snapAngle, 1.0f, 1.0f, 90.0f))
            {
                setRotationSnapAngle(snapAngle);
            }
        }

        bool localTransform = m_useLocalTransform;
        if (m_uiManager->checkbox("Local Transform", &localTransform))
        {
            setUseLocalTransform(localTransform);
        }

        m_uiManager->endSection();
    }

    if (m_uiManager->beginSection("Grid Settings", true))
    {
        bool gridVisible = m_gridVisible;
        if (m_uiManager->checkbox("Show Grid", &gridVisible))
        {
            setGridVisible(gridVisible);
        }

        float gridSize = m_gridCellSize;
        if (m_uiManager->dragFloat("Grid Size", &gridSize, 0.1f, 0.1f, 10.0f))
        {
            setGridCellSize(gridSize);
        }

        glm::vec3 gridColor = m_gridColor;
        if (m_uiManager->colorEdit3("Grid Color", &gridColor.x))
        {
            setGridColor(gridColor);
        }

        m_uiManager->endSection();
    }

    m_uiManager->endPanel();
}

// Voxel tools rendering
void Editor::renderVoxelTools()
{
    if (!m_voxelEditor || !m_uiManager->beginPanel("Voxel Tools", UIManager::PanelFlags::DEFAULT))
        return;

    // Voxel operations
    const std::vector<std::string> operations = {"Add", "Remove", "Paint", "Select"};
    int currentOp = static_cast<int>(m_voxelEditor->getOperation());

    if (m_uiManager->combo("Operation", &currentOp, operations, 4))
    {
        m_voxelEditor->setOperation(static_cast<VoxelOperation>(currentOp));
    }

    // Brush shape
    const std::vector<std::string> shapes = {"Cube", "Sphere", "Cylinder", "Custom"};
    int currentShape = static_cast<int>(m_voxelEditor->getBrushShape());

    if (m_uiManager->combo("Brush Shape", &currentShape, shapes, 4))
    {
        m_voxelEditor->setBrushShape(static_cast<BrushShape>(currentShape));
    }

    // Brush size
    int brushSize = m_voxelEditor->getBrushSize();
    if (m_uiManager->dragInt("Brush Size", &brushSize, 1, 1, 20))
    {
        m_voxelEditor->setBrushSize(brushSize);
    }

    // Color selection (only show if in Add or Paint mode)
    VoxelOperation operation = m_voxelEditor->getOperation();
    if (operation == VoxelOperation::ADD || operation == VoxelOperation::PAINT)
    {
        glm::vec3 color = m_voxelEditor->getCurrentColor();
        if (m_uiManager->colorEdit3("Voxel Color", &color))
        {
            m_voxelEditor->setCurrentColor(color);
        }
    }

    // Material selection if supported
    int materialIndex = m_voxelEditor->getCurrentMaterial();
    if (m_uiManager->dragInt("Material", &materialIndex, 1, 0, 10))
    {
        m_voxelEditor->setCurrentMaterial(materialIndex);
    }

    // Symmetry options
    bool symmetryEnabled = false;
    int symmetryAxis = 0;
    symmetryEnabled = m_voxelEditor->getSymmetry(symmetryAxis);

    if (m_uiManager->checkbox("Symmetry", &symmetryEnabled))
    {
        m_voxelEditor->setSymmetry(symmetryEnabled, symmetryAxis);
    }

    if (symmetryEnabled)
    {
        const std::vector<std::string> axes = {"X Axis", "Y Axis", "Z Axis"};
        if (m_uiManager->combo("Symmetry Axis", &symmetryAxis, axes, 3))
        {
            m_voxelEditor->setSymmetry(symmetryEnabled, symmetryAxis);
        }
    }

    // Selection operations
    if (operation == VoxelOperation::SELECT)
    {
        m_uiManager->separator();

        // Get current selection
        const std::vector<glm::ivec3>& selection = m_voxelEditor->getSelection();
        m_uiManager->text("Selected: " + std::to_string(selection.size()) + " voxels");

        if (!selection.empty())
        {
            if (m_uiManager->button("Clear Selection"))
            {
                m_voxelEditor->clearSelection();
            }

            if (m_uiManager->button("Delete Selected"))
            {
                m_voxelEditor->deleteSelectedVoxels();
            }

            if (m_uiManager->button("Paint Selected"))
            {
                m_voxelEditor->paintSelectedVoxels();
            }
        }
    }

    // Save/Load buttons
    m_uiManager->separator();

    m_uiManager->beginHorizontal();
    if (m_uiManager->button("Save Voxels", {100, 0}))
    {
        // Show save voxel dialog
        const char* filename = "voxels.vox"; // Would be from a file dialog
        m_voxelEditor->saveGridState(filename);
    }

    if (m_uiManager->button("Load Voxels", {100, 0}))
    {
        // Show load voxel dialog
        const char* filename = "voxels.vox"; // Would be from a file dialog
        m_voxelEditor->loadGridState(filename);
    }
    m_uiManager->endHorizontal();

    m_uiManager->endPanel();
}

// Settings panel rendering
void Editor::renderSettingsPanel()
{
    if (!m_uiManager->beginPanel("Settings", UIManager::PanelFlags::DEFAULT))
        return;

    if (m_uiManager->beginSection("Display", true))
    {
        // Display settings
        bool vsync = true; // This would come from renderer
        if (m_uiManager->checkbox("VSync", &vsync))
        {
            // Set vsync
        }

        int msaaSamples = 4; // This would come from renderer
        const std::vector<std::string> msaaOptions = {"Off", "2x", "4x", "8x"};
        if (m_uiManager->combo("MSAA", &msaaSamples, msaaOptions, 4))
        {
            // Set MSAA
        }

        bool hdr = true; // This would come from renderer
        if (m_uiManager->checkbox("HDR", &hdr))
        {
            // Set HDR
        }

        m_uiManager->endSection();
    }

    if (m_uiManager->beginSection("Camera", true))
    {
        // Camera settings
        if (m_camera)
        {
            float fov = 45.0f;
            if (auto* freeCamera = dynamic_cast<FreeCamera*>(m_camera))
            {
                fov = freeCamera->getFOV();
                if (m_uiManager->dragFloat("FOV", &fov, 1.0f, 10.0f, 120.0f))
                {
                    freeCamera->setFOV(fov);
                }

                float speed = freeCamera->getSpeed();
                if (m_uiManager->dragFloat("Movement Speed", &speed, 0.1f, 0.1f, 20.0f))
                {
                    freeCamera->setSpeed(speed);
                }
            }
            else if (auto* isoCamera = dynamic_cast<IsometricCamera*>(m_camera))
            {
                float zoom = isoCamera->getZoom();
                if (m_uiManager->dragFloat("Zoom", &zoom, 0.1f, 0.1f, 10.0f))
                {
                    isoCamera->setZoom(zoom);
                }

                float orthoSize = isoCamera->getOrthoSize();
                if (m_uiManager->dragFloat("Ortho Size", &orthoSize, 1.0f, 1.0f, 100.0f))
                {
                    isoCamera->setOrthoSize(orthoSize);
                }
            }

            float nearPlane = 0.1f;
            float farPlane = 1000.0f;
            // Get from camera if available

            if (m_uiManager->dragFloat("Near Plane", &nearPlane, 0.1f, 0.01f, 10.0f))
            {
                // Set near plane
            }

            if (m_uiManager->dragFloat("Far Plane", &farPlane, 1.0f, 10.0f, 10000.0f))
            {
                // Set far plane
            }
        }

        m_uiManager->endSection();
    }

    if (m_uiManager->beginSection("Rendering", true))
    {
        // Rendering settings
        if (m_renderer)
        {
            int shadowResolution = 2048;
            const std::vector<std::string> shadowOptions = {"Off", "Low (512)", "Medium (1024)", "High (2048)", "Ultra (4096)"};
            int shadowIndex = 3; // High

            if (m_uiManager->combo("Shadows", &shadowIndex, shadowOptions, 5))
            {
                // Set shadow resolution
                switch (shadowIndex)
                {
                    case 0: shadowResolution = 0; break;    // Off
                    case 1: shadowResolution = 512; break;  // Low
                    case 2: shadowResolution = 1024; break; // Medium
                    case 3: shadowResolution = 2048; break; // High
                    case 4: shadowResolution = 4096; break; // Ultra
                }
            }

            // Could expose other renderer settings here
        }

        m_uiManager->endSection();
    }

    m_uiManager->endPanel();
}

// Viewport overlay rendering
void Editor::renderViewportOverlay()
{
    if (!m_uiManager->beginViewport("Viewport"))
        return;

    // Display mode indicator in top right
    if (m_playModeActive)
    {
        m_uiManager->setViewportOverlayText("PLAY MODE", {1.0f, 0.0f}, {1.0f, 0.2f, 0.2f, 1.0f});
    }

    // Display current edit mode in top left
    const char* modeNames[] = {
        "Select Mode", "Move Mode", "Rotate Mode", "Scale Mode",
        "Voxel Mode", "Terrain Mode", "Paint Mode", "Play Mode"
    };

    m_uiManager->setViewportOverlayText(modeNames[static_cast<int>(m_editMode)], {0.0f, 0.0f});

    // Only render editing overlays in edit mode
    if (!m_playModeActive)
    {
        // Display selection info
        if (!m_selectedEntities.empty())
        {
            std::string selectionText = "Selected: ";
            selectionText += std::to_string(m_selectedEntities.size()) + " entities";
            m_uiManager->setViewportOverlayText(selectionText, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 0.7f});
        }

        // Display transform snap status
        if (m_snapToGrid)
        {
            m_uiManager->setViewportOverlayText("Grid Snap: ON", {1.0f, 1.0f}, {0.2f, 1.0f, 0.2f, 0.7f});
        }
    }

    m_uiManager->endViewport();
}

// Stats panel rendering
void Editor::renderStatsPanel()
{
    if (!m_uiManager->beginPanel("Statistics", UIManager::PanelFlags::DEFAULT))
        return;

    if (m_renderer)
    {
        m_uiManager->text("FPS: " + std::to_string(60)); // This would come from FPS counter
        m_uiManager->text("Draw Calls: " + std::to_string(123)); // This would come from renderer
        m_uiManager->text("Triangles: " + std::to_string(456789)); // This would come from renderer

        m_uiManager->separator();

        // Renderer-specific stat
        m_uiManager->text("Active Clusters: " + std::to_string(m_renderer->getActiveClusterCount()));
        m_uiManager->text("Visible Lights: " + std::to_string(m_renderer->getVisibleLightCount()));
    }

    if (m_scene)
    {
        m_uiManager->separator();

        m_uiManager->text("Entities: " + std::to_string(m_scene->getEntityManager()->getEntityCount()));
        // Could display more scene stats
    }

    if (m_grid)
    {
        m_uiManager->separator();

        m_uiManager->text("Active Chunks: " + std::to_string(m_grid->getActiveChunkCount()));
        m_uiManager->text("Active Cubes: " + std::to_string(m_grid->getTotalActiveCubeCount()));
    }

    if (m_commandManager)
    {
        m_uiManager->separator();

        m_uiManager->text("Undo Stack: " + std::to_string(m_commandManager->getUndoStackSize()));
        m_uiManager->text("Redo Stack: " + std::to_string(m_commandManager->getRedoStackSize()));
    }

    m_uiManager->endPanel();
}

// Console rendering
void Editor::renderConsole()
{
    if (!m_uiManager->beginPanel("Console", UIManager::PanelFlags::DEFAULT))
        return;

    // In a real implementation, this would show log messages
    static std::vector<std::string> logMessages = {
        "Engine initialized successfully.",
        "Loaded scene 'Main.scene'.",
        "WARNING: Missing texture 'missing_texture.png'.",
        "Rendered 142 entities with 287 draw calls."
    };

    // Controls
    m_uiManager->beginHorizontal();
    if (m_uiManager->button("Clear"))
    {
        logMessages.clear();
    }

    bool showWarnings = true;
    bool showErrors = true;
    bool showInfo = true;

    m_uiManager->checkbox("Info", &showInfo);
    m_uiManager->checkbox("Warnings", &showWarnings);
    m_uiManager->checkbox("Errors", &showErrors);

    static char filterBuffer[256] = "";
    m_uiManager->inputText("##ConsoleFilter", filterBuffer, 256, "Filter...");

    m_uiManager->endHorizontal();

    m_uiManager->separator();

    // Log messages
    for (const auto& message : logMessages)
    {
        // Filter messages based on type and filter text
        bool isWarning = message.find("WARNING") != std::string::npos;
        bool isError = message.find("ERROR") != std::string::npos;
        bool isInfo = !isWarning && !isError;

        bool matchesFilter = true;
        if (filterBuffer[0] != '\0')
        {
            matchesFilter = message.find(filterBuffer) != std::string::npos;
        }

        bool shouldShow = (isInfo && showInfo) ||
            (isWarning && showWarnings) ||
            (isError && showErrors);

        if (shouldShow && matchesFilter)
        {
            glm::vec4 color(1.0f);

            if (isWarning)
            {
                color = {1.0f, 0.8f, 0.0f, 1.0f}; // Yellow
            }
            else if (isError)
            {
                color = {1.0f, 0.2f, 0.2f, 1.0f}; // Red
            }

            m_uiManager->textColored(message, color);
        }
    }

    // Add text input for command line?
    static char commandBuffer[256] = "";
    if (m_uiManager->inputTextWithHint("##CommandLine", "Enter command...", commandBuffer, 256, UIManager::InputTextFlags::ENTER_RETURNS_TRUE))
    {
        // Execute command
        std::string command = commandBuffer;
        logMessages.push_back("> " + command);

        // Clear command buffer
        commandBuffer[0] = '\0';
    }

    m_uiManager->endPanel();
}

// Dialog implementations
void Editor::showSaveDialog()
{
    // In a real implementation, this would show a file dialog
    // For now, we'll use a simple popup
    m_uiManager->openPopup("SaveSceneDialog");
}

void Editor::showLoadDialog()
{
    // In a real implementation, this would show a file dialog
    // For now, we'll use a simple popup
    m_uiManager->openPopup("LoadSceneDialog");
}

void Editor::showNewSceneDialog()
{
    m_uiManager->openPopup("NewSceneDialog");
}

void Editor::showSettingsDialog()
{
    m_uiManager->openPopup("SettingsDialog");
}

void Editor::showAboutDialog()
{
    m_uiManager->openPopup("AboutDialog");
}

// Action handlers
void Editor::handleSceneCreation()
{
    // Create default scene entities

    // Create a camera
    Entity* cameraEntity = createEntity("Main Camera");
    // Add camera component
    // Position camera

    // Create a light
    Entity* lightEntity = createEntity("Directional Light");
    // Add light component
    // Position light

    // Create a ground plane
    Entity* groundEntity = createEntity("Ground");
    // Add mesh component
    // Position ground
}

void Editor::handleEntityCreation()
{
    // Create entity and select it
    Entity* entity = createEntity("NewEntity");
    selectEntity(entity);
}

void Editor::handleEntityDeletion()
{
    // Delete selected entities
    if (!m_selectedEntities.empty())
    {
        deleteSelectedEntity();
    }
}

void Editor::handleEntityDuplication()
{
    // Duplicate selected entity
    if (!m_selectedEntities.empty())
    {
        duplicateSelectedEntity();
    }
}

void Editor::handleModeChange(EditMode newMode)
{
    setEditMode(newMode);
}

bool Editor::handleGizmoInteraction()
{
    if (!m_gizmoRenderer || m_selectedEntities.empty())
        return false;

    // Check if mouse if over gizmo
    int mouseX, mouseY;
    m_uiManager->getMousePosition(mouseX, mouseY);

    // Convert to ray
    glm::vec3 rayOrigin, rayDirection;
    screenToWorldRay(mouseX, mouseY, rayOrigin, rayDirection);

    // Check gizmo intersection

    return false;
}

void Editor::updateGizmoTransformation()
{
}

void Editor::applyTransformation()
{
}

void Editor::cancelTransformation()
{
}

void Editor::notifySelectionChanged()
{
}

Entity* Editor::pickEntityAtScreenPos(int x, int y)
{
    return nullptr;
}

void Editor::selectEntitiesInRect(const glm::vec2& start, const glm::vec2& end)
{
}

glm::vec3 Editor::screenToWorldRay(int screenX, int screenY)
{
    return glm::vec3();
}

glm::vec3 Editor::snapPosition(const glm::vec3& position)
{
    return glm::vec3();
}

glm::quat Editor::snapRotation(const glm::quat& rotation)
{
    return glm::quat();
}

glm::vec3 Editor::snapScale(const glm::vec3& scale)
{
    return glm::vec3();
}

void Editor::enterPlayMode()
{
}

void Editor::exitPlayMode()
{
}

void Editor::saveSceneState()
{
}

void Editor::restoreSceneState()
{
}
