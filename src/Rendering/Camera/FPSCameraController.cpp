// -------------------------------------------------------------------------
// FPSCameraController.cpp
// -------------------------------------------------------------------------
#include "Rendering/Camera/FPSCameraController.h"
#include "Core/Logger.h"
#include "Core/Application.h"
#include "Input/InputSystem.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtx/norm.hpp>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Rendering::Camera
{

    FPSCameraController::FPSCameraController(std::shared_ptr<Camera> camera)
        : m_camera(camera)
        , m_moveSpeed(5.0f)  // 5 units per second
        , m_rotationSpeed(0.005f)  // Radians per pixel
        , m_forward(false)
        , m_backward(false)
        , m_left(false)
        , m_right(false)
        , m_up(false)
        , m_down(false)
        , m_yaw(0.0f)
        , m_pitch(0.0f)
    {
        if (!m_camera)
        {
            Log::warn("FPSCameraController: Created with null camera");
        }
        else
        {
            // Extract initial yaw and pitch from camera rotation
            glm::vec3 forward = m_camera->getForward();
            m_yaw = atan2(forward.x, forward.z);
            m_pitch = -asin(forward.y);
        }
    }

    FPSCameraController::~FPSCameraController()
    {
        // Nothing to clean up
    }

    void FPSCameraController::setCamera(std::shared_ptr<Camera> camera)
    {
        m_camera = camera;

        if (m_camera)
        {
            // Extract yaw and pitch from camera rotation
            glm::vec3 forward = m_camera->getForward();
            m_yaw = atan2(forward.x, forward.z);
            m_pitch = -asin(forward.y);
        }
    }

    std::shared_ptr<Camera> FPSCameraController::getCamera() const
    {
        return m_camera;
    }

    void FPSCameraController::setMoveSpeed(float speed)
    {
        if (speed <= 0.0f)
        {
            Log::warn("FPSCameraController::setMoveSpeed: Invalid speed (must be positive)");
            return;
        }

        m_moveSpeed = speed;
    }

    float FPSCameraController::getMoveSpeed() const
    {
        return m_moveSpeed;
    }

    void FPSCameraController::setRotationSpeed(float speed)
    {
        if (speed <= 0.0f)
        {
            Log::warn("FPSCameraController::setRotationSpeed: Invalid speed (must be positive)");
            return;
        }

        m_rotationSpeed = speed;
    }

    float FPSCameraController::getRotationSpeed() const
    {
        return m_rotationSpeed;
    }

    void FPSCameraController::handleKeyboard(float deltaTime)
    {
        if (!m_camera)
        {
            return;
        }

        // Get the Input system from the application
        auto inputSystem = Core::Application::getInstance().getSubsystem<Input::InputSystem>();
        if (!inputSystem)
        {
            return;
        }

        // Check key states for movement
        m_forward = inputSystem->isKeyPressed(Input::KeyCode::W) ||
            inputSystem->isKeyPressed(Input::KeyCode::UP);

        m_backward = inputSystem->isKeyPressed(Input::KeyCode::S) ||
            inputSystem->isKeyPressed(Input::KeyCode::DOWN);

        m_left = inputSystem->isKeyPressed(Input::KeyCode::A) ||
            inputSystem->isKeyPressed(Input::KeyCode::LEFT);

        m_right = inputSystem->isKeyPressed(Input::KeyCode::D) ||
            inputSystem->isKeyPressed(Input::KeyCode::RIGHT);

        m_up = inputSystem->isKeyPressed(Input::KeyCode::SPACE);

        m_down = inputSystem->isKeyPressed(Input::KeyCode::LEFT_SHIFT) ||
            inputSystem->isKeyPressed(Input::KeyCode::RIGHT_SHIFT);
    }

    void FPSCameraController::handleMouseMotion(float deltaX, float deltaY)
    {
        if (!m_camera)
        {
            return;
        }

        // Update yaw and pitch based on mouse movement
        m_yaw -= deltaX * m_rotationSpeed;
        m_pitch -= deltaY * m_rotationSpeed;

        // Clamp pitch to avoid gimbal lock
        const float pitchLimit = glm::half_pi<float>() - 0.01f;
        m_pitch = glm::clamp(m_pitch, -pitchLimit, pitchLimit);
    }

    void FPSCameraController::update(float deltaTime)
    {
        if (!m_camera)
        {
            return;
        }

        // Apply rotation
        glm::quat quatPitch = glm::angleAxis(m_pitch, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::quat quatYaw = glm::angleAxis(m_yaw, glm::vec3(0.0f, 1.0f, 0.0f));

        glm::quat orientation = quatYaw * quatPitch;
        m_camera->setRotation(orientation);

        // Apply movement
        glm::vec3 movement(0.0f);

        if (m_forward)
        {
            movement += m_camera->getForward();
        }
        if (m_backward)
        {
            movement -= m_camera->getForward();
        }
        if (m_right)
        {
            movement += m_camera->getRight();
        }
        if (m_left)
        {
            movement -= m_camera->getRight();
        }
        if (m_up)
        {
            movement += glm::vec3(0.0f, 1.0f, 0.0f);  // World up, not camera up
        }
        if (m_down)
        {
            movement -= glm::vec3(0.0f, 1.0f, 0.0f);  // World down, not camera down
        }

        // Normalize and apply movement
        if (glm::length2(movement) > 0.0f)
        {
            movement = glm::normalize(movement) * m_moveSpeed * deltaTime;
            m_camera->move(movement);
        }
    }

} // namespace PixelCraft::Rendering