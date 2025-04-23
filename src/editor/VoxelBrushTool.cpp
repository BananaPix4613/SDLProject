#include "editor/VoxelBrushTool.h"
#include "Editor.h"
#include "LineBatchRenderer.h"
#include "Camera.h"
#include "UIManager.h"
#include "CubeGrid.h"
#include "CommandManager.h"
#include "Command.h"
#include "Ray.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

VoxelBrushTool::VoxelBrushTool() :
    EditorTool("Voxel Brush"), m_operation(BrushOperation::ADD),
    m_shape(BrushShape::CUBE), m_size(1), m_color(1.0f, 1.0f, 1.0f),
    m_isApplying(false), m_lastAppliedPosition(0, 0, 0),
    m_hoverPosition(0, 0, 0), m_grid(nullptr)
{
}

VoxelBrushTool::~VoxelBrushTool()
{
}

void VoxelBrushTool::activate()
{
    EditorTool::activate();

    // Get reference to the voxel grid
    if (m_editor && m_editor->getActiveScene())
    {
        // In a real implementation, this would get the grid from the scene
        // Here we assume it's accessible through the editor
        m_grid = m_editor->getCubeGrid();
    }

    m_isApplying = false;
}

void VoxelBrushTool::deactivate()
{
    EditorTool::deactivate();
    m_isApplying = false;
}

void VoxelBrushTool::update(float deltaTime)
{
    // Update hover position with mouse ray
    if (m_active && m_grid)
    {
        Ray ray = getMouseRay(m_lastMouseX, m_lastMouseY);
        glm::ivec3 hitPosition, adjacentPosition;
        glm::vec3 normal;

        if (raycastGrid(ray, hitPosition, adjacentPosition, normal))
        {
            // Determine hover position based on operation
            if (m_operation == BrushOperation::ADD)
            {
                // For adding, hover over the adjacent (empty) position
                m_hoverPosition = adjacentPosition;
            }
            else
            {
                // For removing or painting, hover over the hit position
                m_hoverPosition = hitPosition;
            }

            // Continue applying brush if mouse button is held down
            if (m_isApplying && m_hoverPosition != m_lastAppliedPosition)
            {
                applyBrush(m_hoverPosition);
                m_lastAppliedPosition = m_hoverPosition;
            }
        }
    }
}

void VoxelBrushTool::renderTool(Camera* camera)
{
    if (!m_grid || !m_active)
        return;

    // Get line renderer for visualizing the brush
    LineBatchRenderer* lineRenderer = LineBatchRenderer::getInstance();
    lineRenderer->begin(camera->getViewMatrix(), camera->getProjectionMatrix());

    // Get voxels affected by the brush at hover position
    std::vector<glm::ivec3> affectedVoxels = getCubesToModify(m_hoverPosition);

    // Draw wireframes for each affected voxel
    for (const glm::ivec3& voxelPos : affectedVoxels)
    {
        // Check if this position is valid
        bool isValid = true;

        // For add operation, position should be empty
        // For remove/paint operations, position should have a voxel
        if (m_operation == BrushOperation::ADD)
        {
            isValid = !m_grid->isCubeActive(voxelPos.x, voxelPos.y, voxelPos.z);
        }
        else
        {
            isValid = m_grid->isCubeActive(voxelPos.x, voxelPos.y, voxelPos.z);
        }

        if (!isValid)
            continue;

        // Get world position of the voxel
        glm::vec3 worldPos = m_grid->gridToWorldPosition(voxelPos.x, voxelPos.y, voxelPos.z);
        float voxelSize = m_grid->getSpacing();

        // Choose color based on operation
        glm::vec4 color;
        switch (m_operation)
        {
            case BrushOperation::ADD:
                color = glm::vec4(0.2f, 1.0f, 0.2f, 0.8f);  // Green for add
                break;

            case BrushOperation::REMOVE:
                color = glm::vec4(1.0f, 0.2f, 0.2f, 0.8f);  // Red for remove
                break;

            case BrushOperation::PAINT:
                color = glm::vec4(m_color, 0.8f);  // Use brush color for paint
                break;

            default:
                color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);  // White default
                break;
        }

        // Draw voxel wireframe
        float halfSize = voxelSize * 0.5f;
        glm::vec3 v[8];

        // Calculate corners
        v[0] = worldPos + glm::vec3(-halfSize, -halfSize, -halfSize);
        v[1] = worldPos + glm::vec3(halfSize, -halfSize, -halfSize);
        v[2] = worldPos + glm::vec3(halfSize, -halfSize, halfSize);
        v[3] = worldPos + glm::vec3(-halfSize, -halfSize, halfSize);
        v[4] = worldPos + glm::vec3(-halfSize, halfSize, -halfSize);
        v[5] = worldPos + glm::vec3(halfSize, halfSize, -halfSize);
        v[6] = worldPos + glm::vec3(halfSize, halfSize, halfSize);
        v[7] = worldPos + glm::vec3(-halfSize, halfSize, halfSize);

        // Draw edges
        // Bottom face
        lineRenderer->addLine(v[0], v[1], color, 1.0f);
        lineRenderer->addLine(v[1], v[2], color, 1.0f);
        lineRenderer->addLine(v[2], v[3], color, 1.0f);
        lineRenderer->addLine(v[3], v[0], color, 1.0f);

        // Top face
        lineRenderer->addLine(v[4], v[5], color, 1.0f);
        lineRenderer->addLine(v[5], v[6], color, 1.0f);
        lineRenderer->addLine(v[6], v[7], color, 1.0f);
        lineRenderer->addLine(v[7], v[4], color, 1.0f);

        // Connecting edges
        lineRenderer->addLine(v[0], v[4], color, 1.0f);
        lineRenderer->addLine(v[1], v[5], color, 1.0f);
        lineRenderer->addLine(v[2], v[6], color, 1.0f);
        lineRenderer->addLine(v[3], v[7], color, 1.0f);
    }

    lineRenderer->end();
}

void VoxelBrushTool::renderUI()
{
    UIManager* ui = getUIManager();
    if (!ui)
        return;

    if (ui->beginPanel("Voxel Brush"))
    {
        // Operation selector
        const std::vector<std::string> operations = {"Add", "Remove", "Paint"};
        int currentOp = static_cast<int>(m_operation);
        if (ui->combo("Operation", &currentOp, operations, 3))
        {
            setOperation(static_cast<BrushOperation>(currentOp));
        }

        // Shape selector
        const std::vector<std::string> shapes = {"Cube", "Sphere", "Cylinder"};
        int currentShape = static_cast<int>(m_shape);
        if (ui->combo("Shape", &currentShape, shapes, 3))
        {
            setShape(static_cast<BrushShape>(currentShape));
        }

        // Size slider
        int size = m_size;
        if (ui->dragInt("Size", &size, 1, 1, 10))
        {
            setSize(size);
        }

        // Color picker (only for paint mode)
        if (m_operation == BrushOperation::PAINT)
        {
            glm::vec3 color = m_color;
            if (ui->colorEdit3("Color", &color))
            {
                setColor(color);
            }
        }

        ui->text("Left-click: Apply brush");
        ui->text("Shift+Drag: Apply continuously");

        ui->endPanel();
    }
}

bool VoxelBrushTool::onMouseButton(int button, int action, int mods)
{
    EditorTool::onMouseButton(button, action, mods);

    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            // Don't handle if over UI
            if (getUIManager()->isAnyItemHovered())
                return false;

            // Apply brush at current hover position
            if (m_grid)
            {
                // Begin transaction for undo/redo
                m_editor->beginTransaction("Brush Operation");

                applyBrush(m_hoverPosition);
                m_isApplying = true;
                m_lastAppliedPosition = m_hoverPosition;

                return true;
            }
        }
        else if (action == GLFW_RELEASE)
        {
            if (m_isApplying)
            {
                m_isApplying = false;

                // Commit transaction
                m_editor->commitTransaction();

                return true;
            }
        }
    }

    return false;
}

bool VoxelBrushTool::onMouseMove(double xpos, double ypos)
{
    EditorTool::onMouseMove(xpos, ypos);

    // We use update() to track the hover position
    return m_isApplying;
}

bool VoxelBrushTool::onKeyboard(int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        // Number keys 1-3 for operations
        if (key == GLFW_KEY_1)
        {
            setOperation(BrushOperation::ADD);
            return true;
        }
        else if (key == GLFW_KEY_2)
        {
            setOperation(BrushOperation::REMOVE);
            return true;
        }
        else if (key == GLFW_KEY_3)
        {
            setOperation(BrushOperation::PAINT);
            return true;
        }

        // Keys for brush size
        else if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT)
        {
            setSize(std::max(1, m_size - 1));
            return true;
        }
        else if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD)
        {
            setSize(std::min(10, m_size + 1));
            return true;
        }

        // Tab key to cycle through shapes
        else if (key == GLFW_KEY_TAB)
        {
            int nextShape = (static_cast<int>(m_shape) + 1) % 3;
            setShape(static_cast<BrushShape>(nextShape));
            return true;
        }
    }

    return false;
}

void VoxelBrushTool::setOperation(BrushOperation operation)
{
    m_operation = operation;
}

BrushOperation VoxelBrushTool::getOperation() const
{
    return m_operation;
}

void VoxelBrushTool::setShape(BrushShape shape)
{
    m_shape = shape;
}

BrushShape VoxelBrushTool::getShape() const
{
    return m_shape;
}

void VoxelBrushTool::setSize(int size)
{
    m_size = std::max(1, size);
}

int VoxelBrushTool::getSize() const
{
    return m_size;
}

void VoxelBrushTool::setColor(const glm::vec3& color)
{
    m_color = glm::clamp(color, glm::vec3(0.0f), glm::vec3(1.0f));
}

const glm::vec3& VoxelBrushTool::getColor() const
{
    return m_color;
}

void VoxelBrushTool::applyBrush(const glm::ivec3& center)
{
    if (!m_grid)
        return;

    // Get the voxels affected by the brush
    std::vector<glm::ivec3> voxels = getCubesToModify(center);

    // Apply the appropriate operation to each affected voxel
    for (const glm::ivec3& pos : voxels)
    {
        // Check if position is valid for the operation
        bool isValid = true;

        switch (m_operation)
        {
            case BrushOperation::ADD:
                // Can only add to empty positions
                isValid = !m_grid->isCubeActive(pos.x, pos.y, pos.z);

                if (isValid)
                {
                    // Create a new cube with the brush color
                    Cube cube;
                    cube.active = true;
                    cube.color = m_color;
                    cube.position = glm::vec3(pos);

                    // Create command to add voxel
                    createVoxelCommand(pos, cube);
                }
                break;

            case BrushOperation::REMOVE:
                // Can only remove existing voxels
                isValid = m_grid->isCubeActive(pos.x, pos.y, pos.z);

                if (isValid)
                {
                    // Create command to remove voxel
                    removeVoxelCommand(pos);
                }
                break;

            case BrushOperation::PAINT:
                // Can only paint existing voxels
                isValid = m_grid->isCubeActive(pos.x, pos.y, pos.z);

                if (isValid)
                {
                    // Create command to paint voxel
                    paintVoxelCommand(pos, m_color);
                }
                break;
        }
    }
}

std::vector<glm::ivec3> VoxelBrushTool::getCubesToModify(const glm::ivec3& center) const
{
    std::vector<glm::ivec3> result;

    // Calculate the brush radius
    int radius = (m_size - 1) / 2;

    // Different shapes require different algorithms
    switch (m_shape)
    {
        case BrushShape::CUBE:
        {
            // Simple cube shape - all voxels within a cubic area
            for (int x = center.x - radius; x <= center.x + radius; x++)
            {
                for (int y = center.y - radius; y <= center.y + radius; y++)
                {
                    for (int z = center.z - radius; z <= center.z + radius; z++)
                    {
                        result.push_back(glm::ivec3(x, y, z));
                    }
                }
            }
            break;
        }

        case BrushShape::SPHERE:
        {
            // Sphere shape - voxels within a spherical radius
            float radiusSquared = (radius + 0.5f) * (radius + 0.5f);

            for (int x = center.x - radius; x <= center.x + radius; x++)
            {
                for (int y = center.y - radius; y <= center.y + radius; y++)
                {
                    for (int z = center.z - radius; z <= center.z + radius; z++)
                    {
                        // Calculate squared distance from center
                        float dx = static_cast<float>(x - center.x);
                        float dy = static_cast<float>(y - center.y);
                        float dz = static_cast<float>(z - center.z);
                        float distSquared = dx * dx + dy * dy + dz * dz;

                        // Add if within radius
                        if (distSquared <= radiusSquared)
                        {
                            result.push_back(glm::ivec3(x, y, z));
                        }
                    }
                }
            }
            break;
        }

        case BrushShape::CYLINDER:
        {
            // Cylinder shape - voxels within a cylindrical area (Y axis)
            float radiusSquared = (radius + 0.5f) * (radius + 0.5f);

            for (int x = center.x - radius; x <= center.x + radius; x++)
            {
                for (int y = center.y - radius; y <= center.y + radius; y++)
                {
                    for (int z = center.z - radius; z <= center.z + radius; z++)
                    {
                        // Calculate squared horizontal distance from center
                        float dx = static_cast<float>(x - center.x);
                        float dz = static_cast<float>(z - center.z);
                        float distSquared = dx * dx + dz * dz;

                        // Add if within radius (ignore Y)
                        if (distSquared <= radiusSquared)
                        {
                            result.push_back(glm::ivec3(x, y, z));
                        }
                    }
                }
            }
            break;
        }
    }

    return result;
}

bool VoxelBrushTool::raycastGrid(const Ray& ray, glm::ivec3& outHitPosition,
                                 glm::ivec3& outAdjacentPosition, glm::vec3& outNormal)
{
    if (!m_grid)
        return false;

    // Simple ray-voxel grid intersection
    // This is a simplified version - a more efficient algorithm would be used in practice

    // Get grid properties
    float voxelSize = m_grid->getSpacing();

    // Step along the ray at fixed intervals
    float maxDistance = 100.0f;  // Maximum raycast distance
    float stepSize = voxelSize * 0.5f;  // Step smaller than voxel size for accuracy

    for (float t = 0.0f; t < maxDistance; t += stepSize)
    {
        // Calculate current position along ray
        glm::vec3 worldPos = ray.m_origin + ray.m_direction * t;

        // Convert to grid coordinates
        glm::ivec3 gridPos = m_grid->worldToGridCoordinates(worldPos);

        // Check if this voxel exists
        if (m_grid->isCubeActive(gridPos.x, gridPos.y, gridPos.z))
        {
            // We hit a voxel
            outHitPosition = gridPos;

            // Calculate normal based on the face we hit
            // This is just an approximation - finding the exact hit face would require more work

            // Get center of the hit voxel
            glm::vec3 voxelCenter = m_grid->gridToWorldPosition(gridPos.x, gridPos.y, gridPos.z);

            // Calculate hit point on voxel
            glm::vec3 hitPoint = ray.m_origin + ray.m_direction * (t - stepSize);

            // Find closest face
            glm::vec3 localHit = hitPoint - voxelCenter;
            glm::vec3 absLocal = glm::abs(localHit);

            // Determine which face was hit (largest component)
            if (absLocal.x >= absLocal.y && absLocal.x >= absLocal.z)
            {
                // X face
                outNormal = glm::vec3(glm::sign(localHit.x), 0.0f, 0.0f);
            }
            else if (absLocal.y >= absLocal.x && absLocal.y >= absLocal.z)
            {
                // Y face
                outNormal = glm::vec3(0.0f, glm::sign(localHit.y), 0.0f);
            }
            else
            {
                // Z face
                outNormal = glm::vec3(0.0f, 0.0f, glm::sign(localHit.z));
            }

            // Calculate adjacent position (where a new voxel would be placed)
            outAdjacentPosition = gridPos + glm::ivec3(outNormal);

            return true;
        }
    }

    return false;
}

void VoxelBrushTool::createVoxelCommand(const glm::ivec3& position, const Cube& cube)
{
    if (!m_editor || !m_grid)
        return;

    // Create a command for adding a voxel
    class AddVoxelCommand : public Command
    {
    public:
        AddVoxelCommand(CubeGrid* grid, const glm::ivec3& pos, const Cube& c) :
            Command("Add Voxel"), grid(grid), position(pos), cube(c)
        {
        }

        bool execute() override
        {
            if (!grid) return false;
            grid->setCube(position.x, position.y, position.z, cube);
            return true;
        }

        bool undo() override
        {
            if (!grid) return false;

            // Store the current cube before removing it
            oldCube = grid->getCube(position.x, position.y, position.z);

            // Create an inactive cube to remove the voxel
            Cube emptyCube;
            emptyCube.active = false;
            grid->setCube(position.x, position.y, position.z, emptyCube);

            return true;
        }

        bool redo() override
        {
            return execute();
        }

    private:
        CubeGrid* grid;
        glm::ivec3 position;
        Cube cube;
        Cube oldCube;
    };

    // Create and execute the command
    CommandManager* cmdMgr = m_editor->getCommandManager();
    if (cmdMgr)
    {
        auto cmd = std::make_unique<AddVoxelCommand>(m_grid, position, cube);
        cmdMgr->executeCommand(std::move(cmd));
    }
    else
    {
        // Fallback if command manager is not available
        m_grid->setCube(position.x, position.y, position.z, cube);
    }
}

void VoxelBrushTool::removeVoxelCommand(const glm::ivec3& position)
{
    if (!m_editor || !m_grid)
        return;

    // Create a command for removing a voxel
    class RemoveVoxelCommand : public Command
    {
    public:
        RemoveVoxelCommand(CubeGrid* grid, const glm::ivec3& pos) :
            Command("Remove Voxel"), grid(grid), position(pos)
        {
            // Store the current cube for undo
            if (grid)
            {
                oldCube = grid->getCube(position.x, position.y, position.z);
            }
        }

        bool execute() override
        {
            if (!grid) return false;

            // Create an inactive cube to remove the voxel
            Cube emptyCube;
            emptyCube.active = false;
            grid->setCube(position.x, position.y, position.z, emptyCube);

            return true;
        }

        bool undo() override
        {
            if (!grid) return false;
            grid->setCube(position.x, position.y, position.z, oldCube);
            return true;
        }

        bool redo() override
        {
            return execute();
        }

    private:
        CubeGrid* grid;
        glm::ivec3 position;
        Cube oldCube;
    };

    // Create and execute the command
    CommandManager* cmdMgr = m_editor->getCommandManager();
    if (cmdMgr)
    {
        auto cmd = std::make_unique<RemoveVoxelCommand>(m_grid, position);
        cmdMgr->executeCommand(std::move(cmd));
    }
    else
    {
        // Fallback if command manager is not available
        Cube emptyCube;
        emptyCube.active = false;
        m_grid->setCube(position.x, position.y, position.z, emptyCube);
    }
}

void VoxelBrushTool::paintVoxelCommand(const glm::ivec3& position, const glm::vec3& color)
{
    if (!m_editor || !m_grid)
        return;

    // Create a command for painting a voxel
    class PaintVoxelCommand : public Command
    {
    public:
        PaintVoxelCommand(CubeGrid* grid, const glm::ivec3& pos, const glm::vec3& col) :
            Command("Paint Voxel"), grid(grid), position(pos), color(col)
        {
            // Store the current cube for undo
            if (grid)
            {
                oldCube = grid->getCube(position.x, position.y, position.z);
            }
        }

        bool execute() override
        {
            if (!grid) return false;

            // Get the current cube
            Cube cube = grid->getCube(position.x, position.y, position.z);

            // Only paint if the cube is active
            if (cube.active)
            {
                // Update color
                cube.color = color;
                grid->setCube(position.x, position.y, position.z, cube);
                return true;
            }

            return false;
        }

        bool undo() override
        {
            if (!grid) return false;
            grid->setCube(position.x, position.y, position.z, oldCube);
            return true;
        }

        bool redo() override
        {
            return execute();
        }

    private:
        CubeGrid* grid;
        glm::ivec3 position;
        glm::vec3 color;
        Cube oldCube;
    };

    // Create and execute the command
    CommandManager* cmdMgr = m_editor->getCommandManager();
    if (cmdMgr)
    {
        auto cmd = std::make_unique<PaintVoxelCommand>(m_grid, position, color);
        cmdMgr->executeCommand(std::move(cmd));
    }
    else
    {
        // Fallback if command manager is not available
        Cube cube = m_grid->getCube(position.x, position.y, position.z);
        if (cube.active)
        {
            cube.color = color;
            m_grid->setCube(position.x, position.y, position.z, cube);
        }
    }
}