// -------------------------------------------------------------------------
// CameraAnimator.cpp
// -------------------------------------------------------------------------
#include "Rendering/Camera/CameraAnimator.h"
#include "Core/Logger.h"

#include <glm/gtx/quaternion.hpp>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Rendering::Camera
{

    CameraAnimator::CameraAnimator(std::shared_ptr<Camera> camera)
        : m_camera(camera)
        , m_playing(false)
        , m_currentTime(0.0f)
        , m_duration(1.0f)
        , m_animatingPosition(false)
        , m_startPosition(0.0f)
        , m_targetPosition(0.0f)
        , m_animatingRotation(false)
        , m_startRotation(1.0f, 0.0f, 0.0f, 0.0f)
        , m_targetRotation(1.0f, 0.0f, 0.0f, 0.0f)
        , m_animatingTarget(false)
        , m_startTarget(0.0f)
        , m_targetTarget(0.0f)
    {
        if (!m_camera)
        {
            Log::warn("CameraAnimator: Created with null camera");
        }
    }

    CameraAnimator::~CameraAnimator()
    {
        // Nothing to clean up
    }

    void CameraAnimator::setTargetPosition(const glm::vec3& position, float duration)
    {
        if (!m_camera)
        {
            return;
        }

        if (duration <= 0.0f)
        {
            Log::warn("CameraAnimator::setTargetPosition: Invalid duration (must be positive)");
            return;
        }

        m_startPosition = m_camera->getPosition();
        m_targetPosition = position;
        m_animatingPosition = true;
        m_duration = duration;
        m_currentTime = 0.0f;
    }

    void CameraAnimator::setTargetRotation(const glm::quat& rotation, float duration)
    {
        if (!m_camera)
        {
            return;
        }

        if (duration <= 0.0f)
        {
            Log::warn("CameraAnimator::setTargetRotation: Invalid duration (must be positive)");
            return;
        }

        m_startRotation = m_camera->getRotation();
        m_targetRotation = glm::normalize(rotation);
        m_animatingRotation = true;
        m_duration = duration;
        m_currentTime = 0.0f;
    }

    void CameraAnimator::setTargetLookAt(const glm::vec3& target, float duration)
    {
        if (!m_camera)
        {
            return;
        }

        if (duration <= 0.0f)
        {
            Log::warn("CameraAnimator::setTargetLookAt: Invalid duration (must be positive)");
            return;
        }

        m_startTarget = m_camera->getTarget();
        m_targetTarget = target;
        m_animatingTarget = true;
        m_duration = duration;
        m_currentTime = 0.0f;
    }

    void CameraAnimator::animateTo(const glm::vec3& position, const glm::quat& rotation, float duration)
    {
        if (!m_camera)
        {
            return;
        }

        if (duration <= 0.0f)
        {
            Log::warn("CameraAnimator::animateTo: Invalid duration (must be positive)");
            return;
        }

        m_startPosition = m_camera->getPosition();
        m_targetPosition = position;
        m_animatingPosition = true;

        m_startRotation = m_camera->getRotation();
        m_targetRotation = glm::normalize(rotation);
        m_animatingRotation = true;

        m_animatingTarget = false;

        m_duration = duration;
        m_currentTime = 0.0f;
    }

    void CameraAnimator::animateTo(const Camera& targetCamera, float duration)
    {
        if (!m_camera)
        {
            return;
        }

        if (duration <= 0.0f)
        {
            Log::warn("CameraAnimator::animateTo: Invalid duration (must be positive)");
            return;
        }

        m_startPosition = m_camera->getPosition();
        m_targetPosition = targetCamera.getPosition();
        m_animatingPosition = true;

        m_startRotation = m_camera->getRotation();
        m_targetRotation = targetCamera.getRotation();
        m_animatingRotation = true;

        m_animatingTarget = false;

        m_duration = duration;
        m_currentTime = 0.0f;
    }

    void CameraAnimator::play()
    {
        m_playing = true;
    }

    void CameraAnimator::pause()
    {
        m_playing = false;
    }

    void CameraAnimator::stop()
    {
        m_playing = false;
        m_currentTime = 0.0f;
        m_animatingPosition = false;
        m_animatingRotation = false;
        m_animatingTarget = false;
    }

    bool CameraAnimator::isPlaying() const
    {
        return m_playing;
    }

    float CameraAnimator::getProgress() const
    {
        if (m_duration <= 0.0f)
        {
            return 1.0f;
        }

        return glm::clamp(m_currentTime / m_duration, 0.0f, 1.0f);
    }

    void CameraAnimator::update(float deltaTime)
    {
        if (!m_camera || !m_playing)
        {
            return;
        }

        // Update time
        m_currentTime += deltaTime;

        // Check if animation is complete
        if (m_currentTime >= m_duration)
        {
            // Apply final values
            if (m_animatingPosition)
            {
                m_camera->setPosition(m_targetPosition);
            }

            if (m_animatingRotation)
            {
                m_camera->setRotation(m_targetRotation);
            }

            if (m_animatingTarget)
            {
                m_camera->lookAt(m_targetTarget);
            }

            // Animation complete
            m_playing = false;
            return;
        }

        // Calculate interpolation factor with easing
        float t = easeInOut(m_currentTime / m_duration);

        // Interpolate values
        if (m_animatingPosition)
        {
            glm::vec3 position = glm::mix(m_startPosition, m_targetPosition, t);
            m_camera->setPosition(position);
        }

        if (m_animatingRotation)
        {
            glm::quat rotation = glm::slerp(m_startRotation, m_targetRotation, t);
            m_camera->setRotation(rotation);
        }

        if (m_animatingTarget)
        {
            glm::vec3 target = glm::mix(m_startTarget, m_targetTarget, t);
            m_camera->lookAt(target);
        }
    }

    float CameraAnimator::easeInOut(float t) const
    {
        // Smooth cubic easing: 3t^2 - 2t^3
        return t * t * (3.0f - 2.0f * t);
    }

} // namespace PixelCraft::Rendering