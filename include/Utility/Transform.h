// -------------------------------------------------------------------------
// Transform.h
// -------------------------------------------------------------------------
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace PixelCraft::Utility
{

    /**
     * @brief Encapsulates position, rotation, and scale for spatial transformations.
     *
     * The Transform class provides functionality for handling 3D transformations including:
     * - Position, rotation and scale management
     * - Matrix generation for rendering and physics
     * - Support for hierarchical transformations
     * - Interpolation between different transforms
     * - Conversion between different rotation representations
     * - Efficient matrix generation using dirty flags
     * - Local and world space operations
     */
    class Transform
    {
    public:
        /**
         * @brief Default constructor.
         * Initializes position to origin, rotation to identity, and scale to (1,1,1).
         */
        Transform();

        /**
         * @brief Constructor with position.
         * @param position The initial position
         */
        Transform(const glm::vec3& position);

        /**
         * @brief Constructor with position and rotation.
         * @param position The initial position
         * @param rotation The initial rotation as quaternion
         */
        Transform(const glm::vec3& position, const glm::quat& rotation);

        /**
         * @brief Constructor with position, rotation and scale.
         * @param position The initial position
         * @param rotation The initial rotation as quaternion
         * @param scale The initial scale
         */
        Transform(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale);

        /**
         * @brief Constructor from a transformation matrix.
         * Decomposes the matrix into position, rotation, and scale.
         * @param matrix The transformation matrix
         */
        Transform(const glm::mat4& matrix);

        /**
         * @brief Sets the position.
         * @param position The new position
         */
        void setPosition(const glm::vec3& position);

        /**
         * @brief Sets the position using individual components.
         * @param x The x-coordinate
         * @param y The y-coordinate
         * @param z The z-coordinate
         */
        void setPosition(float x, float y, float z);

        /**
         * @brief Gets the current position.
         * @return The current position
         */
        const glm::vec3& getPosition() const;

        /**
         * @brief Translates the position by a delta vector.
         * @param delta The translation delta
         */
        void translate(const glm::vec3& delta);

        /**
         * @brief Translates the position by individual delta components.
         * @param x The x delta
         * @param y The y delta
         * @param z The z delta
         */
        void translate(float x, float y, float z);

        /**
         * @brief Sets the rotation using a quaternion.
         * @param rotation The new rotation as quaternion
         */
        void setRotation(const glm::quat& rotation);

        /**
         * @brief Sets the rotation using Euler angles (in radians).
         * @param eulerAngles The Euler angles in radians (pitch, yaw, roll)
         */
        void setRotation(const glm::vec3& eulerAngles);

        /**
         * @brief Sets the rotation using individual Euler angles (in radians).
         * @param yaw The yaw angle in radians (rotation around Y-axis)
         * @param pitch The pitch angle in radians (rotation around X-axis)
         * @param roll The roll angle in radians (rotation around Z-axis)
         */
        void setRotation(float yaw, float pitch, float roll);

        /**
         * @brief Sets the rotation from an axis-angle representation.
         * @param axis The rotation axis (should be normalized)
         * @param angle The rotation angle in radians
         */
        void setRotationFromAxisAngle(const glm::vec3& axis, float angle);

        /**
         * @brief Gets the current rotation as quaternion.
         * @return The current rotation quaternion
         */
        const glm::quat& getRotation() const;

        /**
         * @brief Gets the current rotation as Euler angles (in radians).
         * @return The current rotation as Euler angles in radians
         */
        glm::vec3 getEulerAngles() const;

        /**
         * @brief Applies an additional rotation.
         * @param rotation The rotation to apply as quaternion
         */
        void rotate(const glm::quat& rotation);

        /**
         * @brief Applies an additional rotation specified by Euler angles.
         * @param eulerAngles The Euler angles in radians (pitch, yaw, roll)
         */
        void rotate(const glm::vec3& eulerAngles);

        /**
         * @brief Applies an additional rotation specified by individual Euler angles.
         * @param yaw The yaw angle in radians (rotation around Y-axis)
         * @param pitch The pitch angle in radians (rotation around X-axis)
         * @param roll The roll angle in radians (rotation around Z-axis)
         */
        void rotate(float yaw, float pitch, float roll);

        /**
         * @brief Rotates around a specific point.
         * @param point The point to rotate around
         * @param rotation The rotation to apply as quaternion
         */
        void rotateAround(const glm::vec3& point, const glm::quat& rotation);

        /**
         * @brief Sets the scale.
         * @param scale The new scale
         */
        void setScale(const glm::vec3& scale);

        /**
         * @brief Sets a uniform scale for all axes.
         * @param uniformScale The uniform scale factor to apply to all axes
         */
        void setScale(float uniformScale);

        /**
         * @brief Sets the scale using individual components.
         * @param x The x-scale
         * @param y The y-scale
         * @param z The z-scale
         */
        void setScale(float x, float y, float z);

        /**
         * @brief Gets the current scale.
         * @return The current scale
         */
        const glm::vec3& getScale() const;

        /**
         * @brief Scales by a vector.
         * @param scale The scale factors to apply
         */
        void scaleBy(const glm::vec3& scale);

        /**
         * @brief Scales uniformly by a scalar.
         * @param scale The uniform scale factor to apply
         */
        void scaleBy(float scale);

        /**
         * @brief Gets the forward direction vector based on rotation.
         * @return The forward direction vector (normalized)
         */
        glm::vec3 getForward() const;

        /**
         * @brief Gets the right direction vector based on rotation.
         * @return The right direction vector (normalized)
         */
        glm::vec3 getRight() const;

        /**
         * @brief Gets the up direction vector based on rotation.
         * @return The up direction vector (normalized)
         */
        glm::vec3 getUp() const;

        /**
         * @brief Gets the transformation matrix.
         * Updates the cached matrix if dirty.
         * @return The transformation matrix
         */
        const glm::mat4& getMatrix() const;

        /**
         * @brief Gets the inverse transformation matrix.
         * @return The inverse transformation matrix
         */
        glm::mat4 getInverseMatrix() const;

        /**
         * @brief Sets the transform from a matrix.
         * Decomposes the matrix into position, rotation, and scale.
         * @param matrix The transformation matrix
         */
        void setFromMatrix(const glm::mat4& matrix);

        /**
         * @brief Decomposes a matrix into position, rotation, and scale.
         * @param matrix The matrix to decompose
         * @param[out] position The resulting position
         * @param[out] rotation The resulting rotation
         * @param[out] scale The resulting scale
         */
        static void decompose(const glm::mat4& matrix, glm::vec3& position, glm::quat& rotation, glm::vec3& scale);

        /**
         * @brief Transforms a point from local space to world space.
         * @param point The point in local space
         * @return The transformed point in world space
         */
        glm::vec3 transformPoint(const glm::vec3& point) const;

        /**
         * @brief Transforms a direction vector from local space to world space.
         * Does not apply scale or translation, only rotation.
         * @param direction The direction in local space
         * @return The transformed direction in world space
         */
        glm::vec3 transformDirection(const glm::vec3& direction) const;

        /**
         * @brief Transforms a point from world space to local space.
         * @param point The point in world space
         * @return The transformed point in local space
         */
        glm::vec3 inverseTransformPoint(const glm::vec3& point) const;

        /**
         * @brief Transforms a direction vector from world space to local space.
         * Does not apply scale or translation, only rotation.
         * @param direction The direction in world space
         * @return The transformed direction in local space
         */
        glm::vec3 inverseTransformDirection(const glm::vec3& direction) const;

        /**
         * @brief Combines this transform with a parent transform.
         * @param parent The parent transform
         * @return The combined transform
         */
        Transform combinedWith(const Transform& parent) const;

        /**
         * @brief Gets this transform relative to a parent transform.
         * @param parent The parent transform
         * @return The relative transform
         */
        Transform relativeTo(const Transform& parent) const;

        /**
         * @brief Linearly interpolates between two transforms.
         * @param a The starting transform
         * @param b The ending transform
         * @param t The interpolation parameter (0.0 to 1.0)
         * @return The interpolated transform
         */
        static Transform lerp(const Transform& a, const Transform& b, float t);

        /**
         * @brief Sets the rotation to look at a target point.
         * @param target The target point to look at
         * @param up The up direction (default is Y-up)
         */
        void lookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));

        /**
         * @brief Resets the transform to identity.
         * Sets position to origin, rotation to identity, and scale to (1,1,1).
         */
        void reset();

    private:
        // Position, rotation, and scale components
        glm::vec3 m_position;   ///< The position component
        glm::quat m_rotation;   ///< The rotation component as quaternion
        glm::vec3 m_scale;      ///< The scale component

        // Cached transformation matrix with dirty flag
        mutable glm::mat4 m_matrix;      ///< Cached transformation matrix
        mutable bool m_matrixDirty;      ///< Flag indicating if matrix needs to be updated

        /**
         * @brief Marks the matrix as dirty, requiring an update.
         */
        void markDirty();

        /**
         * @brief Updates the cached transformation matrix.
         */
        void updateMatrix() const;
    };

} // namespace PixelCraft::Utility