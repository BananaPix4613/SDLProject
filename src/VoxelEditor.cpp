#include "VoxelEditor.h"
#include "GridCore.h"
#include "CubeGrid.h"
#include "Camera.h"
#include "CommandManager.h"
#include "SceneSerializer.h"
#include "Ray.h"
#include "LineBatchRenderer.h"
#include "GizmoRenderer.h"
#include "UIManager.h"
#include <algorithm>
#include <cmath>
#include <filesystem>

// VoxelEditor implementation

VoxelEditor::VoxelEditor() :
    m_grid(nullptr),
    m_isCubeGrid(false),
    m_currentColor(1.0f, 1.0f, 1.0f),
    m_currentMaterialIndex(0),
    m_brushSize(1),
    m_brushShape(BrushShape::CUBE),
    m_currentOperation(VoxelOperation::ADD),
    m_camera(nullptr),
    m_commandManager(nullptr),
    m_symmetryEnabled(false),
    m_symmetryAxis(0),
    m_serializer(nullptr)
{
    m_serializer = new SceneSerializer();
}

VoxelEditor::~VoxelEditor()
{
    delete m_serializer;
}

template <typename T>
bool VoxelEditor::initialize(Grid<T>* grid)
{
    if (!grid)
    {
        return false;
    }

    m_grid = static_cast<void*>(grid);
    m_isCubeGrid = false;
    m_selectedVoxels.clear();

    return true;
}

bool VoxelEditor::initialize(CubeGrid* grid)
{
    if (!grid)
    {
        return false;
    }

    m_grid = static_cast<void*>(grid);
    m_isCubeGrid = true;
    m_selectedVoxels.clear();

    return true;
}

void VoxelEditor::update(float deltaTime)
{
    if (!m_grid || !m_camera)
    {
        return;
    }

    // Check for input if UI isn't capturing it
    UIManager* uiManager = UIManager::getInstance();
    if (uiManager && !uiManager->isMouseOverUI())
    {
        // Handle mouse input for voxel editing
        if (uiManager->isMousePressed(0)) // Left mouse button
        {
            // Get mouse position
            glm::vec2 mousePos = uiManager->getMousePosition();

            // Cast ray from camera through mouse position
            Ray ray = m_camera->screenPointToRay(mousePos);

            // Find the grid position the ray hits
            glm::ivec3 hitPos;
            if (raycastGrid(ray, hitPos))
            {
                // Apply operation based on current settings
                if (m_currentOperation == VoxelOperation::ADD)
                {
                    applyBrush(hitPos);
                }
                else if (m_currentOperation == VoxelOperation::REMOVE)
                {
                    applyBrush(hitPos);
                }
                else if (m_currentOperation == VoxelOperation::PAINT)
                {
                    applyBrush(hitPos);
                }
                else if (m_currentOperation == VoxelOperation::SELECT)
                {
                    // For selection, we'll select or deselect single voxels first
                    toggleVoxelSelection(hitPos);
                }
            }
        }
        else if (uiManager->isMousePressed(1)) // Right mouse button
        {
            // Right click could be used for a secondary operation
            // For example, remove voxels regardless of current operation

            // Get mouse position
            glm::vec2 mousePos = uiManager->getMousePosition();

            // Cast ray from camera through mouse position
            Ray ray = m_camera->screenPointToRay(mousePos);

            // Find the grid position the ray hits
            glm::ivec3 hitPos;
            if (raycastGrid(ray, hitPos))
            {
                // Always remove with right click
                VoxelOperation prevOp = m_currentOperation;
                m_currentOperation = VoxelOperation::REMOVE;
                applyBrush(hitPos);
                m_currentOperation = prevOp;
            }
        }
    }
}

void VoxelEditor::render()
{
    if (!m_grid || !m_camera)
    {
        return;
    }

    // Render brush preview
    renderBrushPreview();

    // Render selection
    renderSelection();

    // Render UI for the editor
    UIManager* uiManager = UIManager::getInstance();
    if (uiManager)
    {
        if (uiManager->beginPanel("Voxel Editor"))
        {
            // Operations section
            uiManager->beginSection("Operation");

            static const char* operations[] = {"Add", "Remove", "Paint", "Select"};
            int currentOp = static_cast<int>(m_currentOperation);
            if (uiManager->comboBox("Operation", currentOp, operations, 4))
            {
                m_currentOperation = static_cast<VoxelOperation>(currentOp);
            }

            uiManager->endSection();

            // Brush settings section
            uiManager->beginSection("Brush Settings");

            static const char* shapes[] = {"Cube", "Sphere", "Cylinder", "Custom"};
            int currentShape = static_cast<int>(m_brushShape);
            if (uiManager->comboBox("Brush Shape", currentShape, shapes, 4))
            {
                m_brushShape = static_cast<BrushShape>(currentShape);
            }

            int brushSize = m_brushSize;
            if (uiManager->dragInt("Brush Size", brushSize, 1, 1, 10))
            {
                m_brushSize = brushSize;
            }

            // Only show color/material when in Add or Paint mode
            if (m_currentOperation == VoxelOperation::ADD ||
                m_currentOperation == VoxelOperation::PAINT)
            {
                glm::vec3 color = m_currentColor;
                if (uiManager->colorEdit3("Color", color))
                {
                    m_currentColor = color;
                }

                int materialIndex = m_currentMaterialIndex;
                if (uiManager->dragInt("Material", materialIndex, 1, 0, 10))
                {
                    m_currentMaterialIndex = materialIndex;
                }
            }

            bool symmetry = m_symmetryEnabled;
            if (uiManager->checkBox("Symmetry", symmetry))
            {
                m_symmetryEnabled = symmetry;
            }

            if (m_symmetryEnabled)
            {
                static const char* axes[] = {"X", "Y", "Z"};
                int axis = m_symmetryAxis;
                if (uiManager->comboBox("Axis", axis, axes, 3))
                {
                    m_symmetryAxis = axis;
                }
            }

            uiManager->endSection();

            // Selection section (only when in Select mode)
            if (m_currentOperation == VoxelOperation::SELECT)
            {
                uiManager->beginSection("Selection");

                uiManager->text("Selected Voxels: %d", m_selectedVoxels.size());

                if (uiManager->button("Clear Selection"))
                {
                    clearSelection();
                }

                if (!m_selectedVoxels.empty())
                {
                    if (uiManager->button("Delete Selected"))
                    {
                        deleteSelectedVoxels();
                    }

                    if (uiManager->button("Paint Selected"))
                    {
                        paintSelectedVoxels();
                    }
                }

                uiManager->endSection();
            }

            // File operations section
            uiManager->beginSection("File Operations");

            static char filenameBuffer[256] = "voxels.grid";
            uiManager->inputText("Filename", filenameBuffer, 256);

            if (uiManager->button("Save Grid"))
            {
                saveGridState(filenameBuffer);
            }

            uiManager->sameLine();

            if (uiManager->button("Load Grid"))
            {
                loadGridState(filenameBuffer);
            }

            uiManager->endSection();

            uiManager->endPanel();
        }
    }
}

void VoxelEditor::setCurrentColor(const glm::vec3& color)
{
    m_currentColor = color;
}

glm::vec3 VoxelEditor::getCurrentColor() const
{
    return m_currentColor;
}

void VoxelEditor::setCurrentMaterial(int materialIndex)
{
    m_currentMaterialIndex = materialIndex;
}

int VoxelEditor::getCurrentMaterial() const
{
    return m_currentMaterialIndex;
}

void VoxelEditor::setBrushSize(int size)
{
    m_brushSize = std::max(1, size);
}

int VoxelEditor::getBrushSize() const
{
    return m_brushSize;
}

void VoxelEditor::setBrushShape(BrushShape shape)
{
    m_brushShape = shape;
}

BrushShape VoxelEditor::getBrushShape() const
{
    return m_brushShape;
}

void VoxelEditor::setOperation(VoxelOperation operation)
{
    m_currentOperation = operation;
}

VoxelOperation VoxelEditor::getOperation() const
{
    return m_currentOperation;
}

bool VoxelEditor::placeVoxel(const glm::ivec3& position)
{
    if (!m_grid)
    {
        return false;
    }

    if (m_isCubeGrid)
    {
        CubeGrid* grid = static_cast<CubeGrid*>(m_grid);

        // Create a new cube with current properties
        Cube cube;
        cube.color = m_currentColor;
        cube.materialIndex = m_currentMaterialIndex;

        // Set the cube in the grid
        grid->setCube(position.x, position.y, position.z, cube);
        return true;
    }
    else
    {
        // Handle the case for generic Grid<T>
        // This would require knowledge of T
        return false;
    }
}

bool VoxelEditor::removeVoxel(const glm::ivec3& position)
{
    if (!m_grid)
    {
        return false;
    }

    if (m_isCubeGrid)
    {
        CubeGrid* grid = static_cast<CubeGrid*>(m_grid);

        // Check if the position has an active cube
        if (grid->isCubeActive(position.x, position.y, position.z))
        {
            // Create an empty cube (this is how we remove in our system)
            Cube emptyCube;
            emptyCube.color = glm::vec3(0.0f);
            emptyCube.materialIndex = -1;  // Invalid material

            // Set the empty cube to remove the existing one
            grid->setCube(position.x, position.y, position.z, emptyCube);
            return true;
        }
    }
    else
    {
        // Handle the case for generic Grid<T>
        // This would require knowledge of T
    }

    return false;
}

bool VoxelEditor::paintVoxel(const glm::ivec3& position)
{
    if (!m_grid)
    {
        return false;
    }

    if (m_isCubeGrid)
    {
        CubeGrid* grid = static_cast<CubeGrid*>(m_grid);

        // Check if the position has an active cube
        if (grid->isCubeActive(position.x, position.y, position.z))
        {
            // Get the existing cube
            Cube cube = grid->getCube(position.x, position.y, position.z);

            // Update its properties
            cube.color = m_currentColor;
            cube.materialIndex = m_currentMaterialIndex;

            // Set the updated cube
            grid->setCube(position.x, position.y, position.z, cube);
            return true;
        }
    }
    else
    {
        // Handle the case for generic Grid<T>
        // This would require knowledge of T
    }

    return false;
}

int VoxelEditor::applyBrush(const glm::ivec3& centerPosition)
{
    if (!m_grid)
    {
        return 0;
    }

    // Start a transaction for undo/redo
    if (m_commandManager)
    {
        std::string opName;
        switch (m_currentOperation)
        {
            case VoxelOperation::ADD: opName = "Add Voxels"; break;
            case VoxelOperation::REMOVE: opName = "Remove Voxels"; break;
            case VoxelOperation::PAINT: opName = "Paint Voxels"; break;
            case VoxelOperation::SELECT: opName = "Select Voxels"; break;
        }

        m_commandManager->beginTransaction(opName);
    }

    // Generate brush voxel positions
    std::vector<glm::ivec3> brushVoxels = generateBrushVoxels(centerPosition);

    // Count of affected voxels
    int affectedCount = 0;

    // Apply operation to each voxel in the brush
    for (const glm::ivec3& pos : brushVoxels)
    {
        bool affected = false;

        // Apply the operation
        switch (m_currentOperation)
        {
            case VoxelOperation::ADD:
                affected = placeVoxel(pos);
                // If symmetry is enabled, place a mirrored voxel
                if (affected && m_symmetryEnabled)
                {
                    placeVoxel(getSymmetricalPosition(pos));
                }
                break;

            case VoxelOperation::REMOVE:
                affected = removeVoxel(pos);
                // If symmetry is enabled, remove the mirrored voxel
                if (affected && m_symmetryEnabled)
                {
                    removeVoxel(getSymmetricalPosition(pos));
                }
                break;

            case VoxelOperation::PAINT:
                affected = paintVoxel(pos);
                // If symmetry is enabled, paint the mirrored voxel
                if (affected && m_symmetryEnabled)
                {
                    paintVoxel(getSymmetricalPosition(pos));
                }
                break;

            case VoxelOperation::SELECT:
                // For brush selection, we'll add all voxels to the selection
                // This differs from the individual toggle behavior
                if (isVoxelActive(pos))
                {
                    addToSelection(pos);
                    affected = true;
                }
                break;
        }

        if (affected)
        {
            affectedCount++;
        }
    }

    // Commit the transaction for undo/redo
    if (m_commandManager)
    {
        if (affectedCount > 0)
        {
            m_commandManager->commitTransaction();
        }
        else
        {
            m_commandManager->abortTransaction();
        }
    }

    return affectedCount;
}

int VoxelEditor::fillArea(const glm::ivec3& start, const glm::ivec3& end)
{
    if (!m_grid)
    {
        return 0;
    }

    // Start a transaction for undo/redo
    if (m_commandManager)
    {
        m_commandManager->beginTransaction("Fill Area");
    }

    // Determine min and max coordinates
    glm::ivec3 min = glm::min(start, end);
    glm::ivec3 max = glm::max(start, end);

    // Count of affected voxels
    int affectedCount = 0;

    // Apply operation to each voxel in the area
    for (int x = min.x; x <= max.x; x++)
    {
        for (int y = min.y; y <= max.y; y++)
        {
            for (int z = min.z; z <= max.z; z++)
            {
                glm::ivec3 pos(x, y, z);
                bool affected = false;

                // Apply the operation
                switch (m_currentOperation)
                {
                    case VoxelOperation::ADD:
                        affected = placeVoxel(pos);
                        break;

                    case VoxelOperation::REMOVE:
                        affected = removeVoxel(pos);
                        break;

                    case VoxelOperation::PAINT:
                        affected = paintVoxel(pos);
                        break;

                    case VoxelOperation::SELECT:
                        if (isVoxelActive(pos))
                        {
                            addToSelection(pos);
                            affected = true;
                        }
                        break;
                }

                if (affected)
                {
                    affectedCount++;
                }
            }
        }
    }

    // Commit the transaction for undo/redo
    if (m_commandManager)
    {
        if (affectedCount > 0)
        {
            m_commandManager->commitTransaction();
        }
        else
        {
            m_commandManager->abortTransaction();
        }
    }

    return affectedCount;
}

int VoxelEditor::selectArea(const glm::ivec3& start, const glm::ivec3& end)
{
    if (!m_grid)
    {
        return 0;
    }

    // Determine min and max coordinates
    glm::ivec3 min = glm::min(start, end);
    glm::ivec3 max = glm::max(start, end);

    // Count of selected voxels
    int selectedCount = 0;

    // Clear existing selection if shift isn't held
    UIManager* uiManager = UIManager::getInstance();
    bool addToExisting = uiManager && (uiManager->isKeyPressed(GLFW_KEY_LEFT_SHIFT) ||
                                       uiManager->isKeyPressed(GLFW_KEY_RIGHT_SHIFT));

    if (!addToExisting)
    {
        clearSelection();
    }

    // Select each active voxel in the area
    for (int x = min.x; x <= max.x; x++)
    {
        for (int y = min.y; y <= max.y; y++)
        {
            for (int z = min.z; z <= max.z; z++)
            {
                glm::ivec3 pos(x, y, z);

                if (isVoxelActive(pos))
                {
                    addToSelection(pos);
                    selectedCount++;
                }
            }
        }
    }

    return selectedCount;
}

void VoxelEditor::clearSelection()
{
    m_selectedVoxels.clear();
}

const std::vector<glm::ivec3>& VoxelEditor::getSelection() const
{
    return m_selectedVoxels;
}

bool VoxelEditor::saveGridState(const std::string& filename)
{
    if (!m_grid || !m_serializer)
    {
        return false;
    }

    if (m_isCubeGrid)
    {
        CubeGrid* grid = static_cast<CubeGrid*>(m_grid);

        // Create a simple scene just to hold the grid
        Scene tempScene("GridExport");
        tempScene.setGrid(grid);

        // Save the scene (which includes the grid)
        return m_serializer->saveScene(filename, &tempScene);
    }
    else
    {
        // Generic Grid<T> saving would need type information
        return false;
    }
}

bool VoxelEditor::loadGridState(const std::string& filename)
{
    if (!m_grid || !m_serializer)
    {
        return false;
    }

    // Check if file exists
    if (!std::filesystem::exists(filename))
    {
        return false;
    }

    if (m_isCubeGrid)
    {
        CubeGrid* grid = static_cast<CubeGrid*>(m_grid);

        // Create a temporary scene to load into
        Scene tempScene("GridImport");

        // Load the scene
        if (!m_serializer->loadScene(filename, &tempScene))
        {
            return false;
        }

        // Extract the grid from the loaded scene
        CubeGrid* loadedGrid = tempScene.getGrid();
        if (!loadedGrid)
        {
            return false;
        }

        // Copy grid data to our grid
        // This is a simplified implementation - a real one would handle grid properties

        // Clear existing grid
        // (this is a simplification - real implementation would need grid reset method)

        // Get grid bounds
        const glm::ivec3& minBounds = loadedGrid->getMinBounds();
        const glm::ivec3& maxBounds = loadedGrid->getMaxBounds();

        // Copy active cubes
        loadedGrid->forEachActiveCell([&](const glm::ivec3& pos, const Cube& cube) {
            grid->setCube(pos.x, pos.y, pos.z, cube);
                                      });

        return true;
    }
    else
    {
        // Generic Grid<T> loading would need type information
        return false;
    }
}

template <typename T>
bool VoxelEditor::createCustomBrush(const Grid<T>* templateGrid, const glm::ivec3& centerOffset)
{
    if (!templateGrid)
    {
        return false;
    }

    // Clear existing custom brush
    m_customBrushVoxels.clear();
    m_customBrushCenter = centerOffset;

    // Copy active voxels from the template grid
    templateGrid->forEachActiveCell([&](const glm::ivec3& pos, const T& value) {
        m_customBrushVoxels.push_back(pos);
                                    });

    // Set brush shape to custom
    m_brushShape = BrushShape::CUSTOM;

    return !m_customBrushVoxels.empty();
}

bool VoxelEditor::loadCustomBrush(const std::string& filename)
{
    if (!m_serializer)
    {
        return false;
    }

    // Check if file exists
    if (!std::filesystem::exists(filename))
    {
        return false;
    }

    // Create a temporary grid to load into
    CubeGrid tempGrid(16, 1.0f);

    // Create a simple scene just for loading
    Scene tempScene("BrushImport");
    tempScene.setGrid(&tempGrid);

    // Load the scene (which includes the grid)
    if (!m_serializer->loadScene(filename, &tempScene))
    {
        return false;
    }

    // Calculate the center of the brush
    const glm::ivec3& minBounds = tempGrid.getMinBounds();
    const glm::ivec3& maxBounds = tempGrid.getMaxBounds();
    glm::ivec3 center = (minBounds + maxBounds) / 2;

    // Set up the custom brush
    m_customBrushVoxels.clear();
    m_customBrushCenter = center;

    // Copy active voxels from the template grid
    tempGrid.forEachActiveCell([&](const glm::ivec3& pos, const Cube& cube) {
        m_customBrushVoxels.push_back(pos);
                               });

    // Set brush shape to custom
    m_brushShape = BrushShape::CUSTOM;

    return !m_customBrushVoxels.empty();
}

void VoxelEditor::setCamera(Camera* camera)
{
    m_camera = camera;
}

void VoxelEditor::setCommandManager(CommandManager* commandManager)
{
    m_commandManager = commandManager;
}

void VoxelEditor::setSymmetry(bool enable, int axis)
{
    m_symmetryEnabled = enable;
    m_symmetryAxis = glm::clamp(axis, 0, 2);  // 0=X, 1=Y, 2=Z
}

bool VoxelEditor::getSymmetry(int& outAxis) const
{
    outAxis = m_symmetryAxis;
    return m_symmetryEnabled;
}

// Private implementation methods

std::vector<glm::ivec3> VoxelEditor::generateBrushVoxels(const glm::ivec3& center)
{
    std::vector<glm::ivec3> voxels;

    switch (m_brushShape)
    {
        case BrushShape::CUBE:
        {
            // Generate a cube-shaped brush
            int radius = m_brushSize / 2;

            for (int x = -radius; x <= radius; x++)
            {
                for (int y = -radius; y <= radius; y++)
                {
                    for (int z = -radius; z <= radius; z++)
                    {
                        voxels.push_back(center + glm::ivec3(x, y, z));
                    }
                }
            }
            break;
        }

        case BrushShape::SPHERE:
        {
            // Generate a sphere-shaped brush
            float radiusSquared = (m_brushSize * 0.5f) * (m_brushSize * 0.5f);
            int radius = (m_brushSize + 1) / 2;

            for (int x = -radius; x <= radius; x++)
            {
                for (int y = -radius; y <= radius; y++)
                {
                    for (int z = -radius; z <= radius; z++)
                    {
                        float distSquared = x * x + y * y + z * z;
                        if (distSquared <= radiusSquared)
                        {
                            voxels.push_back(center + glm::ivec3(x, y, z));
                        }
                    }
                }
            }
            break;
        }

        case BrushShape::CYLINDER:
        {
            // Generate a cylinder-shaped brush (axis is Y)
            float radiusSquared = (m_brushSize * 0.5f) * (m_brushSize * 0.5f);
            int radius = (m_brushSize + 1) / 2;
            int height = m_brushSize;

            for (int x = -radius; x <= radius; x++)
            {
                for (int y = -height / 2; y <= height / 2; y++)
                {
                    for (int z = -radius; z <= radius; z++)
                    {
                        float distSquared = x * x + z * z;
                        if (distSquared <= radiusSquared)
                        {
                            voxels.push_back(center + glm::ivec3(x, y, z));
                        }
                    }
                }
            }
            break;
        }

        case BrushShape::CUSTOM:
        {
            // Use the custom brush pattern
            for (const glm::ivec3& offset : m_customBrushVoxels)
            {
                voxels.push_back(center + (offset - m_customBrushCenter));
            }
            break;
        }
    }

    return voxels;
}

glm::ivec3 VoxelEditor::getSymmetricalPosition(const glm::ivec3& position)
{
    if (!m_symmetryEnabled)
    {
        return position;
    }

    // Get the mirror plane position
    // For simplicity, we'll use world origin as the mirror plane
    // In a real implementation, this might be configurable

    glm::ivec3 result = position;

    // Mirror across the appropriate axis
    switch (m_symmetryAxis)
    {
        case 0: // X-axis
            result.x = -position.x;
            break;

        case 1: // Y-axis
            result.y = -position.y;
            break;

        case 2: // Z-axis
            result.z = -position.z;
            break;
    }

    return result;
}

void VoxelEditor::renderBrushPreview()
{
    if (!m_grid || !m_camera)
    {
        return;
    }

    // Only show brush preview when mouse is over the scene
    UIManager* uiManager = UIManager::getInstance();
    if (!uiManager || uiManager->isMouseOverUI())
    {
        return;
    }

    // Get mouse position
    glm::vec2 mousePos = uiManager->getMousePosition();

    // Cast ray from camera
    Ray ray = m_camera->screenPointToRay(mousePos);

    // Find the grid position the ray hits
    glm::ivec3 hitPos;
    if (!raycastGrid(ray, hitPos))
    {
        return;
    }

    // Generate brush voxels for preview
    std::vector<glm::ivec3> brushVoxels = generateBrushVoxels(hitPos);

    // Add symmetrical voxels if enabled
    if (m_symmetryEnabled)
    {
        std::vector<glm::ivec3> symmetricalVoxels;
        for (const glm::ivec3& pos : brushVoxels)
        {
            symmetricalVoxels.push_back(getSymmetricalPosition(pos));
        }

        // Append symmetrical voxels
        brushVoxels.insert(brushVoxels.end(), symmetricalVoxels.begin(), symmetricalVoxels.end());
    }

    // Set preview color based on operation
    glm::vec4 previewColor;
    switch (m_currentOperation)
    {
        case VoxelOperation::ADD:
            previewColor = glm::vec4(0.0f, 1.0f, 0.0f, 0.3f);  // Green
            break;

        case VoxelOperation::REMOVE:
            previewColor = glm::vec4(1.0f, 0.0f, 0.0f, 0.3f);  // Red
            break;

        case VoxelOperation::PAINT:
            previewColor = glm::vec4(m_currentColor, 0.7f);    // Current color
            break;

        case VoxelOperation::SELECT:
            previewColor = glm::vec4(1.0f, 1.0f, 0.0f, 0.3f);  // Yellow
            break;
    }

    // Render the brush preview as wireframe boxes
    for (const glm::ivec3& pos : brushVoxels)
    {
        // Convert grid position to world position
        glm::vec3 worldPos = gridToWorldPosition(pos);

        // Create a transform matrix for the voxel
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), worldPos);

        // Get the grid spacing for voxel size
        float spacing = 1.0f;
        if (m_isCubeGrid)
        {
            spacing = static_cast<CubeGrid*>(m_grid)->getSpacing();
        }

        // Calculate voxel size (slightly smaller than actual size to avoid z-fighting)
        glm::vec3 voxelSize(spacing * 0.98f);

        // Render the voxel as a wireframe box
        GizmoRenderer* gizmoRenderer = GizmoRenderer::getInstance();
        gizmoRenderer->drawWireBox(transform, voxelSize, previewColor);
    }
}

void VoxelEditor::renderSelection()
{
    if (m_selectedVoxels.empty() || !m_grid)
    {
        return;
    }

    // Render the selected voxels
    for (const glm::ivec3& pos : m_selectedVoxels)
    {
        // Convert grid position to world position
        glm::vec3 worldPos = gridToWorldPosition(pos);

        // Create a transform matrix for the voxel
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), worldPos);

        // Get the grid spacing for voxel size
        float spacing = 1.0f;
        if (m_isCubeGrid)
        {
            spacing = static_cast<CubeGrid*>(m_grid)->getSpacing();
        }

        // Calculate voxel size (slightly larger than actual size)
        glm::vec3 voxelSize(spacing * 1.02f);

        // Render the voxel as a wireframe box
        GizmoRenderer* gizmoRenderer = GizmoRenderer::getInstance();
        gizmoRenderer->drawWireBox(transform, voxelSize, glm::vec4(1.0f, 1.0f, 0.0f, 0.8f));
    }
}

bool VoxelEditor::raycastGrid(const Ray& ray, glm::ivec3& outPosition)
{
    if (!m_grid)
    {
        return false;
    }

    // This is a simplified implementation that uses a fixed step distance
    // A real implementation would use more sophisticated grid-ray intersection

    float stepDistance = 0.1f;
    float maxDistance = 1000.0f;
    glm::vec3 currentPos = ray.origin;

    for (float t = 0.0f; t < maxDistance; t += stepDistance)
    {
        currentPos = ray.origin + ray.direction * t;

        // Convert to grid position
        glm::ivec3 gridPos = worldToGridPosition(currentPos);

        // Check if this grid cell is active
        if (isVoxelActive(gridPos))
        {
            outPosition = gridPos;
            return true;
        }
    }

    // No intersection found, try to find a position adjacent to an active voxel
    // This allows placing voxels on the surface of the existing structure

    currentPos = ray.origin;
    glm::ivec3 lastGridPos = worldToGridPosition(currentPos);

    for (float t = 0.0f; t < maxDistance; t += stepDistance)
    {
        currentPos = ray.origin + ray.direction * t;
        glm::ivec3 gridPos = worldToGridPosition(currentPos);

        // If we moved to a new grid cell
        if (gridPos != lastGridPos)
        {
            // Check if the new position is active
            if (isVoxelActive(gridPos))
            {
                // Return the last position (the empty one adjacent to the active one)
                outPosition = lastGridPos;
                return true;
            }

            // Check all 6 adjacent cells of the previous position
            static const glm::ivec3 directions[] = {
                glm::ivec3(1, 0, 0), glm::ivec3(-1, 0, 0),
                glm::ivec3(0, 1, 0), glm::ivec3(0, -1, 0),
                glm::ivec3(0, 0, 1), glm::ivec3(0, 0, -1)
            };

            for (int i = 0; i < 6; i++)
            {
                glm::ivec3 adjacentPos = lastGridPos + directions[i];
                if (isVoxelActive(adjacentPos))
                {
                    // Found an active adjacent cell, return the last position
                    outPosition = lastGridPos;
                    return true;
                }
            }

            lastGridPos = gridPos;
        }
    }

    return false;
}

glm::ivec3 VoxelEditor::worldToGridPosition(const glm::vec3& worldPos)
{
    if (!m_grid)
    {
        return glm::ivec3(0);
    }

    if (m_isCubeGrid)
    {
        CubeGrid* grid = static_cast<CubeGrid*>(m_grid);
        return grid->worldToGridCoordinates(worldPos);
    }
    else
    {
        // Generic implementation
        // This assumes a grid spacing of 1.0
        return glm::ivec3(
            static_cast<int>(floor(worldPos.x + 0.5f)),
            static_cast<int>(floor(worldPos.y + 0.5f)),
            static_cast<int>(floor(worldPos.z + 0.5f))
        );
    }
}

glm::vec3 VoxelEditor::gridToWorldPosition(const glm::ivec3& gridPos)
{
    if (!m_grid)
    {
        return glm::vec3(0.0f);
    }

    if (m_isCubeGrid)
    {
        CubeGrid* grid = static_cast<CubeGrid*>(m_grid);
        return grid->gridToWorldPosition(gridPos.x, gridPos.y, gridPos.z);
    }
    else
    {
        // Generic implementation
        // This assumes a grid spacing of 1.0
        return glm::vec3(gridPos);
    }
}

bool VoxelEditor::isVoxelActive(const glm::ivec3& position)
{
    if (!m_grid)
    {
        return false;
    }

    if (m_isCubeGrid)
    {
        CubeGrid* grid = static_cast<CubeGrid*>(m_grid);
        return grid->isCubeActive(position.x, position.y, position.z);
    }
    else
    {
        // Generic implementation would require knowledge of T
        return false;
    }
}

void VoxelEditor::toggleVoxelSelection(const glm::ivec3& position)
{
    if (!isVoxelActive(position))
    {
        return;
    }

    // Check if position is already in selection
    auto it = std::find(m_selectedVoxels.begin(), m_selectedVoxels.end(), position);

    if (it != m_selectedVoxels.end())
    {
        // Remove from selection
        m_selectedVoxels.erase(it);
    }
    else
    {
        // Add to selection
        m_selectedVoxels.push_back(position);
    }
}

void VoxelEditor::addToSelection(const glm::ivec3& position)
{
    if (!isVoxelActive(position))
    {
        return;
    }

    // Check if position is already in selection
    auto it = std::find(m_selectedVoxels.begin(), m_selectedVoxels.end(), position);

    if (it == m_selectedVoxels.end())
    {
        // Add to selection
        m_selectedVoxels.push_back(position);
    }
}

void VoxelEditor::deleteSelectedVoxels()
{
    if (m_selectedVoxels.empty() || !m_grid)
    {
        return;
    }

    // Start a transaction for undo/redo
    if (m_commandManager)
    {
        m_commandManager->beginTransaction("Delete Selected Voxels");
    }

    // Remove each selected voxel
    for (const glm::ivec3& pos : m_selectedVoxels)
    {
        removeVoxel(pos);
    }

    // Commit the transaction
    if (m_commandManager)
    {
        m_commandManager->commitTransaction();
    }

    // Clear the selection
    clearSelection();
}

void VoxelEditor::paintSelectedVoxels()
{
    if (m_selectedVoxels.empty() || !m_grid)
    {
        return;
    }

    // Start a transaction for undo/redo
    if (m_commandManager)
    {
        m_commandManager->beginTransaction("Paint Selected Voxels");
    }

    // Paint each selected voxel
    for (const glm::ivec3& pos : m_selectedVoxels)
    {
        paintVoxel(pos);
    }

    // Commit the transaction
    if (m_commandManager)
    {
        m_commandManager->commitTransaction();
    }
}

// Command class implementations for undo/redo

class VoxelEditor::PlaceVoxelCommand : public Command
{
public:
    PlaceVoxelCommand(VoxelEditor* editor, const glm::ivec3& position,
                      const glm::vec3& color, int materialIndex)
        : m_editor(editor)
        , m_position(position)
        , m_color(color)
        , m_materialIndex(materialIndex)
        , m_hadPreviousVoxel(false)
        , m_previousColor(0.0f)
        , m_previousMaterialIndex(0)
    {
        // Store the previous state for undo
        if (m_editor && m_editor->m_isCubeGrid)
        {
            CubeGrid* grid = static_cast<CubeGrid*>(m_editor->m_grid);
            m_hadPreviousVoxel = grid->isCubeActive(position.x, position.y, position.z);

            if (m_hadPreviousVoxel)
            {
                Cube cube = grid->getCube(position.x, position.y, position.z);
                m_previousColor = cube.color;
                m_previousMaterialIndex = cube.materialIndex;
            }
        }
    }

    virtual void execute() override
    {
        if (m_editor)
        {
            // Save current editor state
            glm::vec3 prevColor = m_editor->m_currentColor;
            int prevMaterial = m_editor->m_currentMaterialIndex;

            // Set the specified color and material
            m_editor->m_currentColor = m_color;
            m_editor->m_currentMaterialIndex = m_materialIndex;

            // Place the voxel
            m_editor->placeVoxel(m_position);

            // Restore editor state
            m_editor->m_currentColor = prevColor;
            m_editor->m_currentMaterialIndex = prevMaterial;
        }
    }

    virtual void undo() override
    {
        if (m_editor)
        {
            if (m_hadPreviousVoxel)
            {
                // Restore previous voxel
                glm::vec3 prevColor = m_editor->m_currentColor;
                int prevMaterial = m_editor->m_currentMaterialIndex;

                m_editor->m_currentColor = m_previousColor;
                m_editor->m_currentMaterialIndex = m_previousMaterialIndex;

                m_editor->placeVoxel(m_position);

                m_editor->m_currentColor = prevColor;
                m_editor->m_currentMaterialIndex = prevMaterial;
            }
            else
            {
                // Remove the voxel
                m_editor->removeVoxel(m_position);
            }
        }
    }

private:
    VoxelEditor* m_editor;
    glm::ivec3 m_position;
    glm::vec3 m_color;
    int m_materialIndex;

    // Previous state for undo
    bool m_hadPreviousVoxel;
    glm::vec3 m_previousColor;
    int m_previousMaterialIndex;
};

class VoxelEditor::RemoveVoxelCommand : public Command
{
public:
    RemoveVoxelCommand(VoxelEditor* editor, const glm::ivec3& position)
        : m_editor(editor)
        , m_position(position)
        , m_hadPreviousVoxel(false)
        , m_previousColor(0.0f)
        , m_previousMaterialIndex(0)
    {
        // Store the previous state for undo
        if (m_editor && m_editor->m_isCubeGrid)
        {
            CubeGrid* grid = static_cast<CubeGrid*>(m_editor->m_grid);
            m_hadPreviousVoxel = grid->isCubeActive(position.x, position.y, position.z);

            if (m_hadPreviousVoxel)
            {
                Cube cube = grid->getCube(position.x, position.y, position.z);
                m_previousColor = cube.color;
                m_previousMaterialIndex = cube.materialIndex;
            }
        }
    }

    virtual void execute() override
    {
        if (m_editor)
        {
            m_editor->removeVoxel(m_position);
        }
    }

    virtual void undo() override
    {
        if (m_editor && m_hadPreviousVoxel)
        {
            // Restore previous voxel
            glm::vec3 prevColor = m_editor->m_currentColor;
            int prevMaterial = m_editor->m_currentMaterialIndex;

            m_editor->m_currentColor = m_previousColor;
            m_editor->m_currentMaterialIndex = m_previousMaterialIndex;

            m_editor->placeVoxel(m_position);

            m_editor->m_currentColor = prevColor;
            m_editor->m_currentMaterialIndex = prevMaterial;
        }
    }

private:
    VoxelEditor* m_editor;
    glm::ivec3 m_position;

    // Previous state for undo
    bool m_hadPreviousVoxel;
    glm::vec3 m_previousColor;
    int m_previousMaterialIndex;
};

class VoxelEditor::PaintVoxelCommand : public Command
{
public:
    PaintVoxelCommand(VoxelEditor* editor, const glm::ivec3& position,
                      const glm::vec3& color, int materialIndex)
        : m_editor(editor)
        , m_position(position)
        , m_color(color)
        , m_materialIndex(materialIndex)
        , m_previousColor(0.0f)
        , m_previousMaterialIndex(0)
    {
        // Store the previous state for undo
        if (m_editor && m_editor->m_isCubeGrid)
        {
            CubeGrid* grid = static_cast<CubeGrid*>(m_editor->m_grid);
            if (grid->isCubeActive(position.x, position.y, position.z))
            {
                Cube cube = grid->getCube(position.x, position.y, position.z);
                m_previousColor = cube.color;
                m_previousMaterialIndex = cube.materialIndex;
            }
        }
    }

    virtual void execute() override
    {
        if (m_editor)
        {
            // Save current editor state
            glm::vec3 prevColor = m_editor->m_currentColor;
            int prevMaterial = m_editor->m_currentMaterialIndex;

            // Set the specified color and material
            m_editor->m_currentColor = m_color;
            m_editor->m_currentMaterialIndex = m_materialIndex;

            // Paint the voxel
            m_editor->paintVoxel(m_position);

            // Restore editor state
            m_editor->m_currentColor = prevColor;
            m_editor->m_currentMaterialIndex = prevMaterial;
        }
    }

    virtual void undo() override
    {
        if (m_editor)
        {
            // Restore previous voxel appearance
            glm::vec3 prevColor = m_editor->m_currentColor;
            int prevMaterial = m_editor->m_currentMaterialIndex;

            m_editor->m_currentColor = m_previousColor;
            m_editor->m_currentMaterialIndex = m_previousMaterialIndex;

            m_editor->paintVoxel(m_position);

            m_editor->m_currentColor = prevColor;
            m_editor->m_currentMaterialIndex = prevMaterial;
        }
    }

private:
    VoxelEditor* m_editor;
    glm::ivec3 m_position;
    glm::vec3 m_color;
    int m_materialIndex;

    // Previous state for undo
    glm::vec3 m_previousColor;
    int m_previousMaterialIndex;
};