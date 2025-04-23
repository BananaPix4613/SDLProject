// -------------------------------------------------------------------------
// PerspectiveCamera.h
// -------------------------------------------------------------------------
#pragma once

#include "Rendering/Camera/Camera.h"

namespace PixelCraft::Rendering::Camera
{

    /**
     * @brief Perspective camera with projection based on field of view
     */
    class PerspectiveCamera : public Camera
    {
    public:
        /**
         * @brief Default constructor
         */
        PerspectiveCamera();

        /**
         * @brief Constructor with initialization parameters
         * @param fov Field of view in radians
         * @param aspectRatio Aspect ratio (width/height)
         * @param nearPlane Near plane distance
         * @param farPlane Far plane distance
         */
        PerspectiveCamera(float fov, float aspectRatio, float nearPlane, float farPlane);

        /**
         * @brief Destructor
         */
        ~PerspectiveCamera() override;

        /**
         * @brief Set the field of view
         * @param fov Field of view in radians
         */
        void setFOV(float fov);

        /**
         * @brief Get the current field of view
         * @return Field of view in radians
         */
        float getFOV() const;

    protected:
        /**
         * @brief Update the projection matrix for perspective projection
         */
        void updateProjectionMatrix() const override;

    private:
        float m_fov; // Field of view in radians
    };

} // namespace PixelCraft::Rendering