// -------------------------------------------------------------------------
// Transform.cpp
// -------------------------------------------------------------------------
#include "Utility/Transform.h"
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace PixelCraft::Utility
{

    Transform::Transform()
        : m_position(0.0f)
        , m_rotation(1.0f, 0.0f, 0.0f, 0.0f)  // Identity quaternion
        , m_scale(1.0f)
        , m_matrixDirty(true)
    {
    }

    Transform::Transform(const glm::vec3& position)
        : m_position(position)
        , m_rotation(1.0f, 0.0f, 0.0f, 0.0f)  // Identity quaternion
        , m_scale(1.0f)
        , m_matrixDirty(true)
    {
    }

    Transform::Transform(const glm::vec3& position, const glm::quat& rotation)
        : m_position(position)
        , m_rotation(rotation)
        , m_scale(1.0f)
        , m_matrixDirty(true)
    {
        // Ensure rotation quaternion is normalized
        m_rotation = glm::normalize(m_rotation);
    }

    Transform::Transform(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale)
        : m_position(position)
        , m_rotation(rotation)
        , m_scale(scale)
        , m_matrixDirty(true)
    {
        // Ensure rotation quaternion is normalized
        m_rotation = glm::normalize(m_rotation);
    }

    Transform::Transform(const glm::mat4& matrix)
        : m_matrixDirty(true)
    {
        setFromMatrix(matrix);
    }

    void Transform::setPosition(const glm::vec3& position)
    {
        m_position = position;
        markDirty();
    }

    void Transform::setPosition(float x, float y, float z)
    {
        m_position = glm::vec3(x, y, z);
        markDirty();
    }

    const glm::vec3& Transform::getPosition() const
    {
        return m_position;
    }

    void Transform::translate(const glm::vec3& delta)
    {
        m_position += delta;
        markDirty();
    }

    void Transform::translate(float x, float y, float z)
    {
        m_position += glm::vec3(x, y, z);
        markDirty();
    }

    void Transform::setRotation(const glm::quat& rotation)
    {
        m_rotation = glm::normalize(rotation); // Ensure quaternion is normalized
        markDirty();
    }

    void Transform::setRotation(const glm::vec3& eulerAngles)
    {
        // Convert Euler angles (in radians) to quaternion
        m_rotation = glm::quat(eulerAngles);
        // Ensure quaternion is normalized
        m_rotation = glm::normalize(m_rotation);
        markDirty();
    }

    void Transform::setRotation(float yaw, float pitch, float roll)
    {
        // Create rotation quaternion from Euler angles (in radians)
        // Note: GLM expects the angles in the order: pitch, yaw, roll
        m_rotation = glm::quat(glm::vec3(pitch, yaw, roll));
        // Ensure quaternion is normalized
        m_rotation = glm::normalize(m_rotation);
        markDirty();
    }

    void Transform::setRotationFromAxisAngle(const glm::vec3& axis, float angle)
    {
        m_rotation = glm::angleAxis(angle, glm::normalize(axis));
        markDirty();
    }

    const glm::quat& Transform::getRotation() const
    {
        return m_rotation;
    }

    glm::vec3 Transform::getEulerAngles() const
    {
        // Convert quaternion to Euler angles (in radians)
        return glm::eulerAngles(m_rotation);
    }

    void Transform::rotate(const glm::quat& rotation)
    {
        // Apply rotation (quaternion multiplication)
        m_rotation = glm::normalize(rotation * m_rotation);
        markDirty();
    }

    void Transform::rotate(const glm::vec3& eulerAngles)
    {
        // Create rotation quaternion from Euler angles and apply it
        glm::quat rot = glm::quat(eulerAngles);
        m_rotation = glm::normalize(rot * m_rotation);
        markDirty();
    }

    void Transform::rotate(float yaw, float pitch, float roll)
    {
        // Create rotation quaternion from Euler angles and apply it
        glm::quat rot = glm::quat(glm::vec3(pitch, yaw, roll));
        m_rotation = glm::normalize(rot * m_rotation);
        markDirty();
    }

    void Transform::rotateAround(const glm::vec3& point, const glm::quat& rotation)
    {
        // 1. Translate position relative to the rotation point
        glm::vec3 relPos = m_position - point;

        // 2. Rotate the relative position
        relPos = rotation * relPos;

        // 3. Translate back to world space
        m_position = point + relPos;

        // 4. Apply rotation to current rotation
        m_rotation = glm::normalize(rotation * m_rotation);

        markDirty();
    }

    void Transform::setScale(const glm::vec3& scale)
    {
        m_scale = scale;
        markDirty();
    }

    void Transform::setScale(float uniformScale)
    {
        m_scale = glm::vec3(uniformScale);
        markDirty();
    }

    void Transform::setScale(float x, float y, float z)
    {
        m_scale = glm::vec3(x, y, z);
        markDirty();
    }

    const glm::vec3& Transform::getScale() const
    {
        return m_scale;
    }

    void Transform::scaleBy(const glm::vec3& scale)
    {
        m_scale *= scale;
        markDirty();
    }

    void Transform::scaleBy(float scale)
    {
        m_scale *= scale;
        markDirty();
    }

    glm::vec3 Transform::getForward() const
    {
        // Forward is -Z axis rotated by quaternion
        return glm::normalize(m_rotation * glm::vec3(0.0f, 0.0f, -1.0f));
    }

    glm::vec3 Transform::getRight() const
    {
        // Right is +X axis rotated by quaternion
        return glm::normalize(m_rotation * glm::vec3(1.0f, 0.0f, 0.0f));
    }

    glm::vec3 Transform::getUp() const
    {
        // Up is +Y axis rotated by quaternion
        return glm::normalize(m_rotation * glm::vec3(0.0f, 1.0f, 0.0f));
    }

    const glm::mat4& Transform::getMatrix() const
    {
        if (m_matrixDirty)
        {
            updateMatrix();
        }
        return m_matrix;
    }

    glm::mat4 Transform::getInverseMatrix() const
    {
        // For a TRS (translation-rotation-scale) matrix, the inverse can be computed efficiently
        // if we know the components. This is more efficient than inverting the matrix directly.

        // Inverse scale (reciprocal)
        glm::vec3 invScale = glm::vec3(
            m_scale.x != 0.0f ? 1.0f / m_scale.x : 0.0f,
            m_scale.y != 0.0f ? 1.0f / m_scale.y : 0.0f,
            m_scale.z != 0.0f ? 1.0f / m_scale.z : 0.0f
        );

        // Inverse rotation (conjugate of quaternion)
        glm::quat invRotation = glm::conjugate(m_rotation);

        // Build the inverse scale matrix
        glm::mat4 invScaleMat = glm::scale(glm::mat4(1.0f), invScale);

        // Build the inverse rotation matrix
        glm::mat4 invRotMat = glm::mat4_cast(invRotation);

        // Transform the negated position by inverse rotation and scale
        glm::vec3 invPos = -(invRotation * (invScale * m_position));

        // Build the inverse translation matrix
        glm::mat4 invTransMat = glm::translate(glm::mat4(1.0f), invPos);

        // Combine the inverse matrices in reverse order
        // For a TRS matrix, the inverse is S^-1 * R^-1 * T^-1
        return invScaleMat * invRotMat * invTransMat;
    }

    void Transform::setFromMatrix(const glm::mat4& matrix)
    {
        decompose(matrix, m_position, m_rotation, m_scale);
        m_matrixDirty = true;
    }

    void Transform::decompose(const glm::mat4& matrix, glm::vec3& position, glm::quat& rotation, glm::vec3& scale)
    {
        // Extract translation
        position = glm::vec3(matrix[3]);

        // Extract the rotation and scale
        glm::mat3 rotScaleMat(matrix);

        // Extract scale by calculating the length of each axis
        scale.x = glm::length(glm::vec3(rotScaleMat[0]));
        scale.y = glm::length(glm::vec3(rotScaleMat[1]));
        scale.z = glm::length(glm::vec3(rotScaleMat[2]));

        // Determine if the matrix includes reflection/shear
        // We do this by checking if the cross product of the x and y axes
        // points in the same direction as the z axis
        glm::vec3 columnX = glm::vec3(rotScaleMat[0]);
        glm::vec3 columnY = glm::vec3(rotScaleMat[1]);
        glm::vec3 columnZ = glm::vec3(rotScaleMat[2]);

        float determinant = glm::determinant(rotScaleMat);
        if (determinant < 0.0f)
        {
            // Matrix includes reflection, adjust scale and one axis
            scale.x = -scale.x;
            columnX = -columnX;
        }

        // Remove scaling from rotation matrix
        if (scale.x != 0.0f) columnX /= scale.x;
        if (scale.y != 0.0f) columnY /= scale.y;
        if (scale.z != 0.0f) columnZ /= scale.z;

        // Reconstruct the normalized rotation matrix
        glm::mat3 rotMat(columnX, columnY, columnZ);

        // Convert rotation matrix to quaternion
        rotation = glm::quat_cast(rotMat);
        rotation = glm::normalize(rotation); // Ensure the quaternion is normalized
    }

    glm::vec3 Transform::transformPoint(const glm::vec3& point) const
    {
        // Transform point: M * point
        // We can do this more efficiently by directly applying TRS components

        // Apply scale
        glm::vec3 transformed = point * m_scale;

        // Apply rotation
        transformed = m_rotation * transformed;

        // Apply translation
        transformed += m_position;

        return transformed;
    }

    glm::vec3 Transform::transformDirection(const glm::vec3& direction) const
    {
        // Transform direction vector: only apply rotation
        return m_rotation * direction;
    }

    glm::vec3 Transform::inverseTransformPoint(const glm::vec3& point) const
    {
        // Inverse transform point: M^-1 * point
        // We can do this more efficiently by directly applying inverse TRS components

        // Apply inverse translation
        glm::vec3 transformed = point - m_position;

        // Apply inverse rotation (conjugate quaternion)
        transformed = glm::conjugate(m_rotation) * transformed;

        // Apply inverse scale
        transformed.x = (m_scale.x != 0.0f) ? transformed.x / m_scale.x : 0.0f;
        transformed.y = (m_scale.y != 0.0f) ? transformed.y / m_scale.y : 0.0f;
        transformed.z = (m_scale.z != 0.0f) ? transformed.z / m_scale.z : 0.0f;

        return transformed;
    }

    glm::vec3 Transform::inverseTransformDirection(const glm::vec3& direction) const
    {
        // Inverse transform direction: only apply inverse rotation
        return glm::conjugate(m_rotation) * direction;
    }

    Transform Transform::combinedWith(const Transform& parent) const
    {
        // Combine transforms: parent * this
        Transform result;

        // Scale = parent.scale * this.scale
        result.m_scale = parent.m_scale * m_scale;

        // Rotation = parent.rotation * this.rotation
        result.m_rotation = glm::normalize(parent.m_rotation * m_rotation);

        // Position = parent.position + (parent.rotation * (parent.scale * this.position))
        result.m_position = parent.m_position + parent.m_rotation * (parent.m_scale * m_position);

        result.markDirty();
        return result;
    }

    Transform Transform::relativeTo(const Transform& parent) const
    {
        // Compute this transform relative to the parent transform
        Transform result;

        // Inverse scale
        glm::vec3 invParentScale = glm::vec3(
            parent.m_scale.x != 0.0f ? 1.0f / parent.m_scale.x : 0.0f,
            parent.m_scale.y != 0.0f ? 1.0f / parent.m_scale.y : 0.0f,
            parent.m_scale.z != 0.0f ? 1.0f / parent.m_scale.z : 0.0f
        );

        // Inverse rotation
        glm::quat invParentRotation = glm::conjugate(parent.m_rotation);

        // Position = parent^-1 * this.position
        result.m_position = invParentRotation * (invParentScale * (m_position - parent.m_position));

        // Rotation = parent^-1 * this.rotation
        result.m_rotation = glm::normalize(invParentRotation * m_rotation);

        // Scale = this.scale / parent.scale
        result.m_scale = m_scale * invParentScale;

        result.markDirty();
        return result;
    }

    Transform Transform::lerp(const Transform& a, const Transform& b, float t)
    {
        // Clamp t to [0,1]
        t = glm::clamp(t, 0.0f, 1.0f);

        // Linear interpolation of position and scale
        glm::vec3 position = glm::mix(a.m_position, b.m_position, t);
        glm::vec3 scale = glm::mix(a.m_scale, b.m_scale, t);

        // Spherical linear interpolation of rotation
        glm::quat rotation = glm::slerp(a.m_rotation, b.m_rotation, t);

        // Create and return the interpolated transform
        return Transform(position, rotation, scale);
    }

    void Transform::lookAt(const glm::vec3& target, const glm::vec3& up)
    {
        // Compute look direction
        glm::vec3 direction = glm::normalize(target - m_position);

        // Edge case: target is very close to position
        if (glm::length(direction) < 1e-6f)
        {
            return;
        }

        // Construct a rotation matrix that looks in the direction of the target
        glm::mat4 lookMatrix = glm::lookAt(m_position, target, up);

        // Extract rotation quaternion from the look matrix
        // Note that lookAt returns a view matrix which is the inverse of what we want
        // We need to take the upper-left 3x3 of the matrix, which is the rotation part
        glm::mat3 rotMat(lookMatrix);
        rotMat = glm::transpose(rotMat); // Transpose to convert from view to model

        // Convert the rotation matrix to a quaternion
        m_rotation = glm::normalize(glm::quat_cast(rotMat));

        markDirty();
    }

    void Transform::reset()
    {
        m_position = glm::vec3(0.0f);
        m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion
        m_scale = glm::vec3(1.0f);
        markDirty();
    }

    void Transform::markDirty()
    {
        m_matrixDirty = true;
    }

    void Transform::updateMatrix() const
    {
        // Build transformation matrix: Translation * Rotation * Scale
        m_matrix = glm::translate(glm::mat4(1.0f), m_position) *
            glm::mat4_cast(m_rotation) *
            glm::scale(glm::mat4(1.0f), m_scale);

        m_matrixDirty = false;
    }

} // namespace PixelCraft::Utility