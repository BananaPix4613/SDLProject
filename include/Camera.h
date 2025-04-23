#pragma once

#include <glm/glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

// Abstract camera base class
class Camera
{
public:
    virtual ~Camera() = default;

    virtual glm::mat4 getViewMatrix() const = 0;
    virtual glm::mat4 getProjectionMatrix() const = 0;

    virtual glm::vec3 getPosition() const = 0;
    virtual void setPosition(const glm::vec3& position) = 0;

    virtual glm::vec3 getTarget() const = 0;
    virtual void setTarget(const glm::vec3& target) = 0;

    virtual float getAspectRatio() const = 0;
    virtual void setAspectRatio(float aspect) = 0;
};

// Isometric camera for voxel rendering
class IsometricCamera : public Camera
{
public:
    IsometricCamera(float aspect = 1.0f)
        : position(30.0f, 30.0f, 30.0f),
          target(0.0f, 0.0f, 0.0f),
          up(0.0f, 1.0f, 0.0f),
          aspectRatio(aspect),
          zoom(1.0f),
          orthoSize(20.0f)
    {

    }

    glm::mat4 getViewMatrix() const override
    {
        return glm::lookAt(position, target, up);
    }

    glm::mat4 getProjectionMatrix() const override
    {
        float size = orthoSize / zoom;
        return glm::ortho(
            -size * aspectRatio, size * aspectRatio,
            -size, size,
            -100.0f, 100.0f
        );
    }

    glm::vec3 getPosition() const override
    {
        return position;
    }

    void setPosition(const glm::vec3& pos) override
    {
        position = pos;
    }

    glm::vec3 getTarget() const override
    {
        return target;
    }

    void setTarget(const glm::vec3& tgt) override
    {
        target = tgt;
    }

    float getAspectRatio() const override
    {
        return aspectRatio;
    }

    void setAspectRatio(float aspect) override
    {
        aspectRatio = aspect;
    }

    // Isometric-specific methods
    void setZoom(float zoomLevel)
    {
        zoom = glm::clamp(zoomLevel, 0.1f, 10.0f);
    }

    float getZoom() const
    {
        return zoom;
    }

    void setOrthoSize(float size)
    {
        orthoSize = size;
    }

    float getOrthoSize() const
    {
        return orthoSize;
    }

    void rotate(float angle)
    {
        // Create a rotation matrix around the Y axis
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));

        // Get the direction vector from target to position
        glm::vec3 direction = position - target;

        // Apply rotation to the direction vector
        glm::vec4 rotatedDirection = rotationMatrix * glm::vec4(direction, 0.0f);

        // Calculate new position
        position = target + glm::vec3(rotatedDirection);
    }

    void pan(const glm::vec3& direction)
    {
        target += direction;
        position += direction;
    }

private:
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;
    float aspectRatio;
    float zoom;
    float orthoSize;
};

// Free-look/orbiting camera
class FreeCamera : public Camera
{
public:
    enum CameraMovement
    {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN
    };

    FreeCamera(float aspect = 1.0f)
        : position(0.0f, 0.0f, 5.0f),
          //target(0.0f, 0.0f, 0.0f),
          up(0.0f, 1.0f, 0.0f),
          yaw(-90.0f),
          pitch(0.0f),
          fov(45.0f),
          aspectRatio(aspect),
          nearPlane(0.1f),
          farPlane(1000.0f),
          mouseSensitivity(0.1f),
          speed(2.5f)
    {
        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() const override
    {
        return glm::lookAt(position, position + front, up);
    }

    glm::mat4 getProjectionMatrix() const override
    {
        return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    }

    glm::vec3 getPosition() const override
    {
        return position;
    }

    void setPosition(const glm::vec3& pos) override
    {
        position = pos;
    }

    glm::vec3 getTarget() const override
    {
        return position + front;
    }

    void setTarget(const glm::vec3& tgt) override
    {
        // Calculate the direction to target
        glm::vec3 direction = glm::normalize(tgt - position);

        // Extract yaw and pitch from direction vector
        pitch = glm::degrees(asin(direction.y));
        yaw = glm::degrees(atan2(direction.z, direction.z));

        // Update camera vectors
        updateCameraVectors();
    }

    float getAspectRatio() const override
    {
        return aspectRatio;
    }

    void setAspectRatio(float aspect) override
    {
        aspectRatio = aspect;
    }

    // FreeCamera-specific methods
    void setFOV(float fovDegrees)
    {
        fov = glm::clamp(fovDegrees, 1.0f, 90.0f);
    }

    float getFOV() const
    {
        return fov;
    }

    void setYawPitch(float newYaw, float newPitch)
    {
        yaw = newYaw;
        pitch = glm::clamp(newPitch, -89.0f, 89.0f); // Prevent flipping
        updateCameraVectors();
    }

    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true)
    {
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        yaw += xoffset;
        pitch += yoffset;

        // Constrain pitch to avoid flipping
        if (constrainPitch)
        {
            pitch = glm::clamp(pitch, -89.0f, 89.0f);
        }

        // Update camera vectors
        updateCameraVectors();
    }

    void processKeyboard(CameraMovement direction, float deltaTime)
    {
        float velocity = speed * deltaTime;

        if (direction == FORWARD)
        {
            position += front * velocity;
        }
        if (direction == BACKWARD)
        {
            position -= front * velocity;
        }
        if (direction == LEFT)
        {
            position -= right * velocity;
        }
        if (direction == RIGHT)
        {
            position += right * velocity;
        }
        if (direction == UP)
        {
            position += up * velocity;
        }
        if (direction == DOWN)
        {
            position -= up * velocity;
        }
    }

    void setSpeed(float newSpeed)
    {
        speed = newSpeed;
    }

    float getSpeed() const
    {
        return speed;
    }

    void setMouseSensitivity(float sensitivity)
    {
        mouseSensitivity = sensitivity;
    }

    float getMouseSensitivity() const
    {
        return mouseSensitivity;
    }

    void setNearFarPlanes(float fnear, float ffar)
    {
        nearPlane = fnear;
        farPlane = ffar;
    }

    void getNearFarPlanes(float& fnear, float& ffar)
    {
        fnear = nearPlane;
        ffar = farPlane;
    }

private:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    // Euler angles
    float yaw;
    float pitch;

    // Camera options
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;
    float mouseSensitivity;
    float speed;

    void updateCameraVectors()
    {
        // Calculate the new front vector
        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        front = glm::normalize(newFront);

        // Recalculate the right and up vectors
        right = glm::normalize(glm::cross(front, worldUp));
        up = glm::normalize(glm::cross(right, front));
    }
};

// ArcballCamera for orbiting around a target
class ArcballCamera : public Camera
{
public:
    ArcballCamera(float aspect = 1.0f)
        : target(0.0f, 0.0f, 0.0f),
          distance(5.0f),
          minDistance(0.1f),
          maxDistance(100.0f),
          yaw(0.0f),
          pitch(0.0f),
          minPitch(-89.0f),
          maxPitch(89.0f),
          aspectRatio(aspect),
          fov(45.0f),
          nearPlane(0.1f),
          farPlane(1000.0f),
          mouseSensitivity(0.25f)
    {
        updateCameraPosition();
    }

private:
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;
    glm::vec3 right;

    float distance;
    float minDistance;
    float maxDistance;

    float yaw;
    float pitch;
    float minPitch;
    float maxPitch;

    float aspectRatio;
    float fov;
    float nearPlane;
    float farPlane;
    float mouseSensitivity;

    void updateCameraPosition()
    {
        // Calculate new camera position based on spherical coordinates
        float x = distance * cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        float y = distance * sin(glm::radians(pitch));
        float z = distance * sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        // Set position relative to target
        position = target + glm::vec3(x, y, z);

        // Update right and up vectors
        // Forward vector (from camera to target)
        glm::vec3 forward = glm::normalize(target - position);

        // Right vector (perpendicular to forward and world up)
        right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

        // Camera up vector (perpendicular to forward and right)
        up = glm::normalize(glm::cross(right, forward));
    }
};