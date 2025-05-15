// -------------------------------------------------------------------------
// Camera.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Utility/Frustum.h"
#include "Utility/Ray.h"

namespace PixelCraft::Rendering::Camera
{

    /**
     * @brief Abstract base camera class that defines view and projection transformations
     */
    class Camera
    {
    public:
        /**
         * @brief Constructor
         */
        Camera();

        /**
         * @brief Virtual destructor
         */
        virtual ~Camera();

        /**
         * @brief Get the view matrix for this camera
         * @return The view matrix
         */
        virtual const glm::mat4& getViewMatrix() const;

        /**
         * @brief Set the view matrix for this camera
         * @param The view matrix to use
         */
        virtual void setViewMatrix(const glm::mat4& viewMatrix)
        {
            m_viewMatrix = viewMatrix;
        }

        /**
         * @brief Get the projection matrix for this camera
         * @return The projection matrix
         */
        virtual const glm::mat4& getProjectionMatrix() const;

        /**
         * @brief Set the projection matrix for this camera
         * @param The projection matrix to use
         */
        virtual void setProjectionMatrix(const glm::mat4& projectionMatrix)
        {
            m_projectionMatrix = projectionMatrix;
        }

        /**
         * @brief Get the combined view-projection matrix
         * @return The view-projection matrix
         */
        virtual const glm::mat4& getViewProjectionMatrix() const;

        /**
         * @brief Set the view projection matrix for this camera
         * @param The view projection matrix to use
         */
        virtual void setViewProjectionMatrix(const glm::mat4& viewProjectionMatrix)
        {
            m_viewProjectionMatrix = viewProjectionMatrix;
        }

        /**
         * @brief Get the frustum for this camera
         * @return The frustum
         */
        virtual const Utility::Frustum& getFrustum() const;

        /**
         * @brief Set the position of the camera
         * @param position The new position
         */
        virtual void setPosition(const glm::vec3& position);

        /**
         * @brief Get the current position of the camera
         * @return The current position
         */
        virtual const glm::vec3& getPosition() const;

        /**
         * @brief Set the rotation of the camera
         * @param rotation The new rotation as a quaternion
         */
        virtual void setRotation(const glm::quat& rotation);

        /**
         * @brief Get the current rotation of the camera
         * @return The current rotation as a quaternion
         */
        virtual const glm::quat& getRotation() const;

        /**
         * @brief Orient the camera to look at a specific target
         * @param target The point to look at
         * @param up The up direction vector, defaults to world up (0,1,0)
         */
        virtual void lookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));

        /**
         * @brief Set the target point the camera is looking at
         * @param target The target point
         */
        virtual void setTarget(const glm::vec3& target);

        /**
         * @brief Get the target point the camera is looking at
         * @return The target point
         */
        virtual const glm::vec3& getTarget() const;

        /**
         * @brief Get the forward direction vector of the camera
         * @return The forward direction vector
         */
        virtual glm::vec3 getForward() const;

        /**
         * @brief Get the right direction vector of the camera
         * @return The right direction vector
         */
        virtual glm::vec3 getRight() const;

        /**
         * @brief Get the up direction vector of the camera
         * @return The up direction vector
         */
        virtual glm::vec3 getUp() const;

        /**
         * @brief Move the camera by the given offset
         * @param offset The offset to move by
         */
        virtual void move(const glm::vec3& offset);

        /**
         * @brief Move the camera forward by the given distance
         * @param distance The distance to move
         */
        virtual void moveForward(float distance);

        /**
         * @brief Move the camera right by the given distance
         * @param distance The distance to move
         */
        virtual void moveRight(float distance);

        /**
         * @brief Move the camera up by the given distance
         * @param distance The distance to move
         */
        virtual void moveUp(float distance);

        /**
         * @brief Rotate the camera by the given quaternion
         * @param rotation The rotation to apply
         */
        virtual void rotate(const glm::quat& rotation);

        /**
         * @brief Rotate the camera around the world up axis (yaw)
         * @param angle The angle to rotate by in radians
         */
        virtual void rotateYaw(float angle);

        /**
         * @brief Rotate the camera around its local right axis (pitch)
         * @param angle The angle to rotate by in radians
         */
        virtual void rotatePitch(float angle);

        /**
         * @brief Rotate the camera around its local forward axis (roll)
         * @param angle The angle to rotate by in radians
         */
        virtual void rotateRoll(float angle);

        /**
         * @brief Set the aspect ratio of the camera
         * @param aspectRatio The new aspect ratio (width/height)
         */
        virtual void setAspectRatio(float aspectRatio);

        /**
         * @brief Get the current aspect ratio of the camera
         * @return The current aspect ratio
         */
        virtual float getAspectRatio() const;

        /**
         * @brief Set the near clipping plane distance
         * @param nearPlane The new near plane distance
         */
        virtual void setNearPlane(float nearPlane);

        /**
         * @brief Get the current near clipping plane distance
         * @return The current near plane distance
         */
        virtual float getNearPlane() const;

        /**
         * @brief Set the far clipping plane distance
         * @param farPlane The new far plane distance
         */
        virtual void setFarPlane(float farPlane);

        /**
         * @brief Get the current far clipping plane distance
         * @return The current far plane distance
         */
        virtual float getFarPlane() const;

        /**
         * @brief Update the camera state
         * This method should be called once per frame to ensure
         * the camera's internal state is up to date
         */
        virtual void update();

        /**
         * @brief Cast a ray from a screen point into the world
         * @param screenPoint The screen point in normalized device coordinates (0,0 to 1,1)
         * @return A ray from the camera through the given screen point
         */
        virtual Utility::Ray screenPointToRay(const glm::vec2& screenPoint) const;

        /**
         * @brief Convert a screen point to a world point at the given depth
         * @param screenPoint The screen point in normalized device coordinates (0,0 to 1,1)
         * @param depth The depth value (0.0 for near plane, 1.0 for far plane)
         * @return The world point
         */
        virtual glm::vec3 screenToWorldPoint(const glm::vec2& screenPoint, float depth = 1.0f) const;

        /**
         * @brief Convert a world point to a screen point
         * @param worldPoint The world point
         * @return The screen point in normalized device coordinates (0,0 to 1,1)
         */
        virtual glm::vec2 worldToScreenPoint(const glm::vec3& worldPoint) const;

    protected:
        /**
         * @brief Update the view matrix if it's dirty
         * This is called internally by getViewMatrix() when needed
         */
        virtual void updateViewMatrix() const;

        /**
         * @brief Update the projection matrix if it's dirty
         * This is called internally by getProjectionMatrix() when needed
         */
        virtual void updateProjectionMatrix() const;

        /**
         * @brief Update the view-projection matrix if it's dirty
         * This is called internally by getViewProjectionMatrix() when needed
         */
        virtual void updateViewProjectionMatrix() const;

        /**
         * @brief Update the frustum if it's dirty
         * This is called internally by getFrustum() when needed
         */
        virtual void updateFrustum() const;

        // Camera position and orientation
        glm::vec3 m_position;
        glm::quat m_rotation;
        glm::vec3 m_target;

        // Camera properties
        float m_aspectRatio;
        float m_nearPlane;
        float m_farPlane;

        // Cached matrices
        mutable glm::mat4 m_viewMatrix;
        mutable glm::mat4 m_projectionMatrix;
        mutable glm::mat4 m_viewProjectionMatrix;
        mutable Utility::Frustum m_frustum;

        // Dirty flags for lazy evaluation
        mutable bool m_viewDirty;
        mutable bool m_projectionDirty;
    };

} // namespace PixelCraft::Rendering