#include once

#include <glm.hpp>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "Command.h"

// Forward declarations
template <typename T> class Grid;
class CubeGrid;
class Cube;
class Camera;
class SceneSerializer;
class EditorTool;
class Ray;
class CommandManager;
class LineBatchRenderer;
class GizmoRenderer;
class UIManager;

/**
 * @enum BrushShape
 * @brief Defines different brush shapes for voxel painting
 */
enum class BrushShape
{
    CUBE,       // Cubic brush
    SPHERE,     // Spherical brush
    CYLINDER,   // Cylindrical brush
    CUSTOM      // Custom brush shape from template
};

/**
 * @enum VoxelOperation
 * @brief Defines different operations for voxel editing
 */
enum class VoxelOperation
{
    ADD,        // Add voxels
    REMOVE,     // Remove voxels
    PAINT,      // Change voxel appearance
    SELECT,     // Select voxels
};

/**
 * @class VoxelEditor
 * @brief Specialized editor for voxel grid manipulation
 * 
 * The VoxelEditor provides tools for creating, removing, and modifying voxels
 * within a Grid<T> system. It handles brush-based operations, selections,
 * and specialized voxel manipulation commands.
 */
class VoxelEditor
{
public:
    /**
     * @brief Constructor
     */
    VoxelEditor();

    /**
     * @brief Destructor
     */
    ~VoxelEditor();

    /**
     * @brief Initialize the voxel editor
     * @param grid Pointer to the grid to edit
     * @return True if initialization succeeded
     */
    template <typename T>
    bool initialize(Grid<T>* grid);

    /**
     * @brief Specialized initialization for CubeGrid
     * @param grid Pointer to the CubeGrid to edit
     * @return True if initialization succeeded
     */
    bool initialize(CubeGrid* grid);

    /**
     * @brief Update editor state based on input and time
     * @param deltaTime Time elapsed since last update
     */
    void update(float deltaTime);

    /**
     * @brief Render editor UI and voxel highlighting
     */
    void render();

    /**
     * @brief Set the current voxel editor
     * @param color RGB color for voxels
     */
    void setCurrentColor(const glm::vec3& color);

    /**
     * @brief Get the current voxel color
     * @return Current voxel color
     */
    glm::vec3 getCurrentColor() const;

    /**
     * @brief Set the current material index
     * @param materialIndex Index of material to apply
     */
    void setCurrentMaterial(int materialIndex);

    /**
     * @brief Get the current material index
     * @return Current material index
     */
    int getCurrentMaterial() const;

    /**
     * @brief Set the brush size
     * @param size Size of the brush in grid cells
     */
    void setBrushSize(int size);

    /**
     * @brief Get the current brush size
     * @return Current brush size
     */
    int getBrushSize() const;

    /**
     * @brief Set the brush shape
     * @param shape Brush shape to use
     */
    void setBrushShape(BrushShape shape);

    /**
     * @brief Get the current brush shape
     * @return Current brush shape
     */
    BrushShape getBrushShape() const;

    /**
     * @brief Set the current operation
     * @param operation Operation to perform
     */
    void setOperation(VoxelOperation operation);

    /**
     * @brief Get the current operation
     * @return Current operation
     */
    VoxelOperation getOperation() const;

    /**
     * @brief Place a voxel at the specified grid position
     * @param position Grid coordinates for voxel placement
     * @return True if the voxel was placed successfully
     */
    bool placeVoxel(const glm::ivec3& position);

    /**
     * @brief Remove a voxel at the specified grid position
     * @param position Grid coordinates for voxel removal
     * @return True if the voxel was removed successfully
     */
    bool removeVoxel(const glm::ivec3& position);

    /**
     * @brief Paint/modify a voxel at the specified grid position
     * @param position Grid coordinates for voxel modification
     * @return True if the voxel was modified successfully
     */
    bool paintVoxel(const glm::ivec3& position);

    /**
     * @brief Apply the current operation using the brush at the specified position
     * @param centerPosition Grid coordinates for brush center
     * @return Number of voxels affected
     */
    int applyBrush(const glm::ivec3& centerPosition);

    /**
     * @brief Fill a volume with voxels
     * @param start Starting grid coordinates
     * @param end Ending grid coordinates
     * @return Number of voxels affected
     */
    int fillArea(const glm::ivec3& start, const glm::ivec3& end);

    /**
     * @brief Select voxels in an area
     * @param start Starting grid coordinates
     * @param end Ending grid coordinates
     * @return Number of voxels selected
     */
    int selectArea(const glm::ivec3& start, const glm::ivec3& end);

    /**
     * @brief Clear the current voxel selection
     */
    void clearSelection();

    /**
     * @brief Get the current voxel selection
     * @return Vector of grid coordinates for selected voxels
     */
    const std::vector<glm::ivec3>& getSelection() const;

    /**
     * @brief Save the current grid state to a file
     * @param filename Path to save the grid to
     * @return True if save succeeded
     */
    bool saveGridState(const std::string& filename);

    /**
     * @brief Load a grid state from a file
     * @param filename Path to the grid file
     * @return True if load succeeded
     */
    bool loadGridState(const std::string& filename);

    /**
     * @brief Create a custom brush from a template
     * @param templateGrid Grid containing the template pattern
     * @param centerOffset Offset from bottom corner to actual center
     * @return True if brush was created successfully
     */
    template <typename T>
    bool createCustomBrush(const Grid<T>* templateGrid, const glm::ivec3& centerOffset);

    /**
     * @brief Create a custom brush from a file
     * @param filename Path to the brush template file
     * @return True if brush was loaded successfully
     */
    bool loadCustomBrush(const std::string& filename);

    /**
     * @brief Set the camera used for ray casting
     * @param camera Pointer to the camera
     */
    void setCamera(Camera* camera);

    /**
     * @brief Set the command manager for undo/redo support
     * @param commandManager Pointer to the command manager
     */
    void setCommandManager(CommandManager* commandManager);

    /**
     * @brief Enable or disable symmetry mode
     * @param enable Whether symmetry is enabled
     * @param axis Axis of symmetry (0=X, 1=Y, 2=Z)
     */
    void setSymmetry(bool enable, int axis = 0);

    /**
     * @brief Get current symmetry settings
     * @param outAxis Output parameter for symmetry axis
     * @return True if symmetry is enabled
     */
    bool getSymmetry(int& outAxis) const;

    /**
     * @brief Delete all selected voxels
     */
    void deleteSelectedVoxels();

    /**
     * @brief Paint all selected voxels with current color and material
     */
    void paintSelectedVoxels();

private:
    // Pointer to the grid being edited (using void* for type erasure in header)
    void* m_grid;
    bool m_isCubeGrid;

    // Editor state
    glm::vec3 m_currentColor;
    int m_currentMaterialIndex;
    int m_brushSize;
    BrushShape m_brushShape;
    VoxelOperation m_currentOperation;
    Camera* m_camera;
    CommandManager* m_commandManager;

    // Custom brush data
    std::vector<glm::ivec3> m_customBrushVoxels;
    glm::ivec3 m_customBrushCenter;

    // Selection
    std::vector<glm::ivec3> m_selectedVoxels;

    // Symmetry settings
    bool m_symmetryEnabled;
    int m_symmetryAxis;

    // Serializer for grid operations
    SceneSerializer* m_serializer;

    /**
     * @brief Generate voxel positions for the current brush
     * @param center Center position of the brush
     * @return Vector of grid positions covered by the brush
     */
    std::vector<glm::ivec3> generateBrushVoxels(const glm::ivec3& center);

    /**
     * @brief Get the symmetrical position for a given position
     * @param position Original position
     * @return Symmetrical position
     */
    glm::ivec3 getSymmetricalPosition(const glm::ivec3& position);

    /**
     * @brief Render brush preview at current mouse position
     */
    void renderBrushPreview();

    /**
     * @brief Render selection highlights
     */
    void renderSelection();

    /**
     * @brief Cast a ray into the grid and find the hit position
     * @param ray Ray to cast
     * @param outPosition Output parameter for hit position
     * @return True if ray hit a voxel or surface
     */
    bool raycastGrid(const Ray& ray, glm::ivec3& outPosition);

    /**
     * @brief Convert world position to grid position
     * @param worldPos World space position
     * @return Grid position
     */
    glm::ivec3 worldToGridPosition(const glm::vec3& worldPos);

    /**
     * @brief Convert grid position to world position
     * @param gridPos Grid position
     * @return World space position
     */
    glm::vec3 gridToWorldPosition(const glm::ivec3& gridPos);

    /**
     * @brief Check if a voxel is active at the given position
     * @param position Grid position to check
     * @return True if position contains an active voxel
     */
    bool isVoxelActive(const glm::ivec3& position);

    /**
     * @brief Toggle selection state of a voxel
     * @param position Grid position of voxel
     */
    void toggleVoxelSelection(const glm::ivec3& position);

    /**
     * @brief Add a voxel to the selection
     * @param position Grid position of voxel
     */
    void addToSelection(const glm::ivec3& position);

    // Command classes for undo/redo
    class PlaceVoxelCommand;
    class RemoveVoxelCommand;
    class PaintVoxelCommand;
    class BrushCommand;
    class FillCommand;
};