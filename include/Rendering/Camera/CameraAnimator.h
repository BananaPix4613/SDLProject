// -------------------------------------------------------------------------
// CameraAnimator.h
// -------------------------------------------------------------------------
#pragma once

#include "Rendering/Camera/Camera.h"
#include <memory>

namespace PixelCraft::Rendering::Camera
{

    /**
     * @brief Animator for smooth camera transitions
     */
    class CameraAnimator
    {
    public:
        /**
         * @brief Constructor
         * @param camera The camera to animate
         */
        CameraAnimator(std::shared_ptr<Camera> camera);

        /**
         * @brief Destructor
         */
        ~CameraAnimator();

        /**
         * @brief Set a target position for animation
         * @param position The target position
         * @param duration The animation duration in seconds
         */
        void setTargetPosition(const glm::vec3& position, float duration = 1.0f);

        /**
         * @brief Set a target rotation for animation
         * @param rotation The target rotation
         * @param duration The animation duration in seconds
         */
        void setTargetRotation(const glm::quat& rotation, float duration = 1.0f);

        /**
         * @brief Set a target look-at point for animation
         * @param target The point to look at
         * @param duration The animation duration in seconds
         */
        void setTargetLookAt(const glm::vec3& target, float duration = 1.0f);

        /**
         * @brief Animate to a position and rotation
         * @param position The target position
         * @param rotation The target rotation
         * @param duration The animation duration in seconds
         */
        void animateTo(const glm::vec3& position, const glm::quat& rotation, float duration = 1.0f);

        /**
         * @brief Animate to match another camera
         * @param targetCamera The camera to match
         * @param duration The animation duration in seconds
         */
        void animateTo(const Camera& targetCamera, float duration = 1.0f);

        /**
         * @brief Start playing the animation
         */
        void play();

        /**
         * @brief Pause the animation
         */
        void pause();

        /**
         * @brief Stop the animation and reset
         */
        void stop();

        /**
         * @brief Check if the animation is currently playing
         * @return True if playing, false otherwise
         */
        bool isPlaying() const;

        /**
         * @brief Get the current animation progress
         * @return Progress as a value between 0.0 and 1.0
         */
        float getProgress() const;

        /**
         * @brief Update the animation
         * @param deltaTime Time since last update in seconds
         */
        void update(float deltaTime);

    private:
        /**
         * @brief Apply smooth easing to a linear progress value
         * @param t Linear progress (0.0 to 1.0)
         * @return Eased progress
         */
        float easeInOut(float t) const;

        std::shared_ptr<Camera> m_camera;

        bool m_playing;
        float m_currentTime;
        float m_duration;

        bool m_animatingPosition;
        glm::vec3 m_startPosition;
        glm::vec3 m_targetPosition;

        bool m_animatingRotation;
        glm::quat m_startRotation;
        glm::quat m_targetRotation;

        bool m_animatingTarget;
        glm::vec3 m_startTarget;
        glm::vec3 m_targetTarget;
    };

} // namespace PixelCraft::Rendering