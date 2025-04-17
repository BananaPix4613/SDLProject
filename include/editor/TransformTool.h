#pragma once

#include "EditorTool.h"
#include <glm.hpp>
#include <vector>

/**
 * @enum TransformMode
 * @brief Modes for the transform tool
 */
enum class TransformMode
{
    TRANSLATE,  // Move entities
    ROTATE,     // Rotate entities
    SCALE,      // Scale entities
};

/**
 * @enum TransformSpace
 * @brief Coordinate spaces for transformations
 */
enum class TransformSpace
{
    LOCAL,      // Local space (relative to entity)
    WORLD,      // World space (global coordinates)
    SCREEN,     // Screen space (camera-aligned)
};

/**
 * @class TransformTool
 * @brief Tool for transforming (moving, rotating, scaling) entities
 * 
 * Provides gizmos and UI controls for precise entity transformation
 * with support for snapping to grid and pixel-perfect positioning.
 */
class TransformTool : public EditorTool
{
public:
    /**
     * @brief Constructor
     */
    TransformTool();

    /**
     * @brief Destructor
     */
    ~TransformTool();

    /**
     * @brief Handle tool activation
     */
    void activate() override;

    /**
     * @brief Handle tool deactivation
     */
    void deactivate() override;

    /**
     * @brief Update transform tool
     * @param deltaTime Time elapsed since last update
     */
    void update(float deltaTime) override;

    /**
     * @brief Render transformation gizmos
     * @param camera Camera used for rendering
     */
    void renderTool(Camera* camera) override;

    /**
     * @brief Render transform UI elements
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
     * @brief Set the transform mode
     * @param mode New transform mode
     */
    void setTransformMode(TransformMode mode);

    /**
     * @brief Get the current transform transform mode
     * @return Current transform mode
     */
    TransformMode getTransformMode() const;

    /**
     * @brief Set the transform coordinate space
     * @param space New transform space
     */
    void setTransformSpace(TransformSpace space);

    /**
     * @brief Get the current transform space
     * @return Current transform space
     */
    TransformSpace getTransformSpace() const;

private:
    // Transform settings
    TransformMode m_transformMode;
    TransformSpace m_transformSpace;

    // Gizmo state
    bool m_dragging;
    int m_activeDragAxis;  // 0 = X, 1 = Y, 2 = Z, 3 = XY plane, 4 = YZ plane, 5 = XZ plane
    glm::vec3 m_dragStartPosition;
    glm::vec3 m_dragStartScale;
    glm::quat m_dragStartRotation;

    // Original transform values for all selected entities
    struct EntityTransform
    {
        Entity* entity;
        glm::vec3 originalPosition;
        glm::vec3 originalScale;
        glm::quat originalRotation;
    };
    std::vector<EntityTransform> m_entityTransforms;

    // Helper methods
    void beginTransform();
    void updateTransform(const glm::vec3& delta);
    void endTransform();

    void applyTranslation(const glm::vec3& delta);
    void applyRotation(const glm::vec3& axis, float angle);
    void applyScale(const glm::vec3& scale);

    int getHoveredAxis(const Ray& ray) const;
    glm::vec3 getGizmoPosition() const;
    glm::quat getGizmoRotation() const;

    void renderTranslateGizmo(Camera* camera);
    void renderRotateGizmo(Camera* camera);
    void renderScaleGizmo(Camera* camera);
};