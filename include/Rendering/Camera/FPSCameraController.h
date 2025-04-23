// -------------------------------------------------------------------------
// FPSCameraController.h
// -------------------------------------------------------------------------
#pragma once

#include "Rendering/Camera/Camera.h"
#include <memory>

namespace PixelCraft::Rendering::Camera
{

    /**
     * @brief Controller for first-person camera movement
     */
    class FPSCameraController
    {
    public:
        /**
         * @brief Constructor
         * @param camera The camera to control
         */
        FPSCameraController(std::shared_ptr<Camera> camera);

        /**
         * @brief Destructor
         */
        ~FPSCameraController();

        /**
         * @brief Set the camera to control
         * @param camera The new camera
         */
        void setCamera(std::shared_ptr<Camera> camera);

        /**
         * @brief Get the currently controlled camera
         * @return The camera
         */
        std::shared_ptr<Camera> getCamera() const;

        /**
         * @brief Set the movement speed
         * @param speed The new movement speed in units per second
         */
        void setMoveSpeed(float speed);

        /**
         * @brief Get the current movement speed
         * @return The movement speed in units per second
         */
        float getMoveSpeed() const;

        /**
         * @brief Set the rotation speed
         * @param speed The new rotation speed in radians per pixel
         */
        void setRotationSpeed(float speed);

        /**
         * @brief Get the current rotation speed
         * @return The rotation speed in radians per pixel
         */
        float getRotationSpeed() const;

        /**
         * @brief Handle keyboard input
         * @param deltaTime Time since last update in seconds
         */
        void handleKeyboard(float deltaTime);

        /**
         * @brief Handle mouse motion
         * @param deltaX Horizontal mouse movement in pixels
         * @param deltaY Vertical mouse movement in pixels
         */
        void handleMouseMotion(float deltaX, float deltaY);

        /**
         * @brief Update the controller
         * @param deltaTime Time since last update in seconds
         */
        void update(float deltaTime);

    private:
        std::shared_ptr<Camera> m_camera;

        float m_moveSpeed;
        float m_rotationSpeed;

        bool m_forward;
        bool m_backward;
        bool m_left;
        bool m_right;
        bool m_up;
        bool m_down;

        float m_yaw;
        float m_pitch;
    };

} // namespace PixelCraft::Rendering