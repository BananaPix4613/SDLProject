// -------------------------------------------------------------------------
// Camera.cpp
// -------------------------------------------------------------------------
#include "Rendering/Camera/Camera.h"
#include "Core/Logger.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Rendering::Camera
{

    Camera::Camera()
        : m_position(0.0f, 0.0f, 0.0f)
        , m_rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)) // Identity quaternion
        , m_target(0.0f, 0.0f, -1.0f)
        , m_aspectRatio(1.0f)
        , m_nearPlane(0.1f)
        , m_farPlane(1000.0f)
        , m_viewMatrix(1.0f)
        , m_projectionMatrix(1.0f)
        , m_viewProjectionMatrix(1.0f)
        , m_viewDirty(true)
        , m_projectionDirty(true)
    {
    }

    Camera::~Camera()
    {
        // Nothing to clean up
    }

    const glm::mat4& Camera::getViewMatrix() const
    {
        if (m_viewDirty)
        {
            updateViewMatrix();
        }
        return m_viewMatrix;
    }

    const glm::mat4& Camera::getProjectionMatrix() const
    {
        if (m_projectionDirty)
        {
            updateProjectionMatrix();
        }
        return m_projectionMatrix;
    }

    const glm::mat4& Camera::getViewProjectionMatrix() const
    {
        // Ensure view and projection matrices are up to date
        if (m_viewDirty || m_projectionDirty)
        {
            updateViewProjectionMatrix();
        }
        return m_viewProjectionMatrix;
    }

    const Utility::Frustum& Camera::getFrustum() const
    {
        // Ensure view-projection matrix is up to date
        if (m_viewDirty || m_projectionDirty)
        {
            updateViewProjectionMatrix();
            updateFrustum();
        }
        return m_frustum;
    }

    void Camera::setPosition(const glm::vec3& position)
    {
        m_position = position;
        m_viewDirty = true;
    }

    const glm::vec3& Camera::getPosition() const
    {
        return m_position;
    }

    void Camera::setRotation(const glm::quat& rotation)
    {
        m_rotation = glm::normalize(rotation);
        m_viewDirty = true;
    }

    const glm::quat& Camera::getRotation() const
    {
        return m_rotation;
    }

    void Camera::lookAt(const glm::vec3& target, const glm::vec3& up)
    {
        if (glm::distance(m_position, target) < 0.0001f)
        {
            Log::warn("Camera::lookAt: Target is too close to camera position");
            return;
        }

        // Calculate the view matrix directly
        glm::mat4 lookAtMatrix = glm::lookAt(m_position, target, up);

        // Extract rotation from the view matrix
        m_rotation = glm::quat_cast(glm::inverse(lookAtMatrix));
        m_target = target;

        m_viewDirty = true;
    }

    void Camera::setTarget(const glm::vec3& target)
    {
        m_target = target;
        // We don't update the rotation here - use lookAt if that's needed
    }

    const glm::vec3& Camera::getTarget() const
    {
        return m_target;
    }

    glm::vec3 Camera::getForward() const
    {
        // Forward is negative Z in view space
        return m_rotation * glm::vec3(0.0f, 0.0f, -1.0f);
    }

    glm::vec3 Camera::getRight() const
    {
        // Right is positive X in view space
        return m_rotation * glm::vec3(1.0f, 0.0f, 0.0f);
    }

    glm::vec3 Camera::getUp() const
    {
        // Up is positive Y in view space
        return m_rotation * glm::vec3(0.0f, 1.0f, 0.0f);
    }

    void Camera::move(const glm::vec3& offset)
    {
        m_position += offset;
        m_viewDirty = true;
    }

    void Camera::moveForward(float distance)
    {
        move(getForward() * distance);
    }

    void Camera::moveRight(float distance)
    {
        move(getRight() * distance);
    }

    void Camera::moveUp(float distance)
    {
        move(getUp() * distance);
    }

    void Camera::rotate(const glm::quat& rotation)
    {
        m_rotation = glm::normalize(rotation * m_rotation);
        m_viewDirty = true;
    }

    void Camera::rotateYaw(float angle)
    {
        // Rotate around world up axis (0,1,0)
        glm::quat q = glm::angleAxis(angle, glm::vec3(0.0f, 1.0f, 0.0f));
        rotate(q);
    }

    void Camera::rotatePitch(float angle)
    {
        // Rotate around local right axis
        glm::vec3 right = getRight();
        glm::quat q = glm::angleAxis(angle, right);
        rotate(q);
    }

    void Camera::rotateRoll(float angle)
    {
        // Rotate around local forward axis
        glm::vec3 forward = getForward();
        glm::quat q = glm::angleAxis(angle, forward);
        rotate(q);
    }

    void Camera::setAspectRatio(float aspectRatio)
    {
        if (aspectRatio <= 0.0f)
        {
            Log::warn("Camera::setAspectRatio: Invalid aspect ratio (must be positive)");
            return;
        }

        m_aspectRatio = aspectRatio;
        m_projectionDirty = true;
    }

    float Camera::getAspectRatio() const
    {
        return m_aspectRatio;
    }

    void Camera::setNearPlane(float nearPlane)
    {
        if (nearPlane <= 0.0f)
        {
            Log::warn("Camera::setNearPlane: Invalid near plane (must be positive)");
            return;
        }

        m_nearPlane = nearPlane;
        m_projectionDirty = true;
    }

    float Camera::getNearPlane() const
    {
        return m_nearPlane;
    }

    void Camera::setFarPlane(float farPlane)
    {
        if (farPlane <= m_nearPlane)
        {
            Log::warn("Camera::setFarPlane: Invalid far plane (must be greater than near plane)");
            return;
        }

        m_farPlane = farPlane;
        m_projectionDirty = true;
    }

    float Camera::getFarPlane() const
    {
        return m_farPlane;
    }

    void Camera::update()
    {
        // This base implementation just ensures matrices are up to date
        if (m_viewDirty || m_projectionDirty)
        {
            updateViewProjectionMatrix();
            updateFrustum();
        }
    }

    Utility::Ray Camera::screenPointToRay(const glm::vec2& screenPoint) const
    {
        // Ensure matrices are up to date
        getViewProjectionMatrix();

        // Convert from screen space [0,1] to normalized device coordinates [-1,1]
        glm::vec4 rayStart_NDC(
            screenPoint.x * 2.0f - 1.0f,
            screenPoint.y * 2.0f - 1.0f,
            -1.0f, // Near plane
            1.0f
        );

        glm::vec4 rayEnd_NDC(
            screenPoint.x * 2.0f - 1.0f,
            screenPoint.y * 2.0f - 1.0f,
            0.0f, // Far plane
            1.0f
        );

        // Inverse transform from NDC to world space
        glm::mat4 invVP = glm::inverse(m_viewProjectionMatrix);
        glm::vec4 rayStart_World = invVP * rayStart_NDC;
        glm::vec4 rayEnd_World = invVP * rayEnd_NDC;

        // Perspective division
        rayStart_World /= rayStart_World.w;
        rayEnd_World /= rayEnd_World.w;

        // Direction vector
        glm::vec3 rayDir_World = glm::normalize(glm::vec3(rayEnd_World) - glm::vec3(rayStart_World));

        return Utility::Ray(glm::vec3(rayStart_World), rayDir_World);
    }

    glm::vec3 Camera::screenToWorldPoint(const glm::vec2& screenPoint, float depth) const
    {
        // Ensure matrices are up to date
        getViewProjectionMatrix();

        // Clamp depth between 0 and 1
        depth = glm::clamp(depth, 0.0f, 1.0f);

        // Convert from screen space [0,1] to normalized device coordinates [-1,1]
        glm::vec4 point_NDC(
            screenPoint.x * 2.0f - 1.0f,
            screenPoint.y * 2.0f - 1.0f,
            depth * 2.0f - 1.0f,
            1.0f
        );

        // Inverse transform from NDC to world space
        glm::mat4 invVP = glm::inverse(m_viewProjectionMatrix);
        glm::vec4 point_World = invVP * point_NDC;

        // Perspective division
        if (point_World.w != 0.0f)
        {
            point_World /= point_World.w;
        }

        return glm::vec3(point_World);
    }

    glm::vec2 Camera::worldToScreenPoint(const glm::vec3& worldPoint) const
    {
        // Ensure matrices are up to date
        getViewProjectionMatrix();

        // Transform world point to clip space
        glm::vec4 clipSpace = m_viewProjectionMatrix * glm::vec4(worldPoint, 1.0f);

        // Perspective division to get NDC coordinates
        glm::vec3 ndcSpace;
        if (clipSpace.w != 0.0f)
        {
            ndcSpace = glm::vec3(clipSpace) / clipSpace.w;
        }
        else
        {
            ndcSpace = glm::vec3(clipSpace);
        }

        // Convert from NDC [-1,1] to screen coordinates [0,1]
        glm::vec2 screenPoint;
        screenPoint.x = (ndcSpace.x + 1.0f) * 0.5f;
        screenPoint.y = (ndcSpace.y + 1.0f) * 0.5f;

        return screenPoint;
    }

    void Camera::updateViewMatrix() const
    {
        // Build the view matrix from position and rotation
        glm::mat4 rotationMatrix = glm::mat4_cast(m_rotation);
        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), -m_position);

        m_viewMatrix = rotationMatrix * translationMatrix;
        m_viewDirty = false;
    }

    void Camera::updateProjectionMatrix() const
    {
        // This is implemented by derived classes
        m_projectionDirty = false;
    }

    void Camera::updateViewProjectionMatrix() const
    {
        // Ensure view and projection matrices are up to date
        if (m_viewDirty)
        {
            updateViewMatrix();
        }
        if (m_projectionDirty)
        {
            updateProjectionMatrix();
        }

        // Multiply projection and view matrices
        m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
    }

    void Camera::updateFrustum() const
    {
        // Extract frustum planes from the view-projection matrix
        m_frustum.extractPlanes(m_viewProjectionMatrix);
    }

} // namespace PixelCraft::Rendering