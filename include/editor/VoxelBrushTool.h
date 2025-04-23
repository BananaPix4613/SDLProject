#pragma once

#include "EditorTool.h"
#include <glm/glm.hpp>
#include <vector>

// Forward declarations
class CubeGrid;
class Cube;

/**
 * @enum BrushOperation
 * @brief Operations that can be performed with the voxel brush
 */
enum class BrushOperation
{
    ADD,        // Add voxels
    REMOVE,     // Remove voxels
    PAINT       // Change voxel colors
};

/**
 * @enum BrushShape
 * @brief Shapes available for the voxel brush
 */
enum class BrushShape
{
    CUBE,       // Cubic brush
    SPHERE,     // Spherical brush
    CYLINDER    // Cylinder brush
};

/**
 * @class VoxelBrushTool
 * @brief Tool for adding, removing, and painting voxels
 * 
 * Provides a configurable brush for manipulating voxels in the grid
 * with support for different shapes, sizes, and operations.
 */
class VoxelBrushTool : public EditorTool
{
public:
    /**
     * @brief Constructor
     */
    VoxelBrushTool();

    /**
     * @brief Destructor
     */
    ~VoxelBrushTool() override;

    /**
     * @brief Handle tool activation
     */
    void activate() override;

    /**
     * @brief Handle tool deactivation
     */
    void deactivate() override;

    /**
     * @brief Update voxel brush tool
     * @param deltaTime Time elapsed since last update
     */
    void update(float deltaTime) override;

    /**
     * @brief Render brush preview
     * @param camera Camera used for rendering
     */
    void renderTool(Camera* camera) override;

    /**
     * @brief Render brush UI controls
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

    /**
     * @brief Set the brush operation
     * @param operation New brush operation
     */
    void setOperation(BrushOperation operation);

    /**
     * @brief Get the current brush operation
     * @return Current brush operation
     */
    BrushOperation getOperation() const;

    /**
     * @brief Set the brush shape
     * @param shape New brush shape
     */
    void setShape(BrushShape shape);

    /**
     * @brief Get the current brush shape
     * @return Current brush shape
     */
    BrushShape getShape() const;

    /**
     * @brief Set the brush size
     * @param size New brush size
     */
    void setSize(int size);

    /**
     * @brief Get the current brush size
     * @return Current brush size
     */
    int getSize() const;

    /**
     * @brief Set the brush color
     * @param color New brush color
     */
    void setColor(const glm::vec3& color);

    /**
     * @brief Get the current brush color
     * @return Current brush color
     */
    const glm::vec3& getColor() const;

private:
    // Brush settings
    BrushOperation m_operation;
    BrushShape m_shape;
    int m_size;
    glm::vec3 m_color;

    // Brush state
    bool m_isApplying;
    glm::ivec3 m_lastAppliedPosition;
    glm::ivec3 m_hoverPosition;

    // Reference to the voxel grid
    CubeGrid* m_grid;
    // Helper methods
    void applyBrush(const glm::ivec3& center);
    std::vector<glm::ivec3> getCubesToModify(const glm::ivec3& center) const;
    bool raycastGrid(const Ray& ray, glm::ivec3& outHitPosition, glm::ivec3& outAdjacentPosition, glm::vec3& outNormal);
    void createVoxelCommand(const glm::ivec3& position, const Cube& cube);
    void removeVoxelCommand(const glm::ivec3& position);
    void paintVoxelCommand(const glm::ivec3& position, const glm::vec3& color);
};