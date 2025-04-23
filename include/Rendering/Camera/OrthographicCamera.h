// -------------------------------------------------------------------------
// OrthographicCamera.h
// -------------------------------------------------------------------------
#pragma once

#include "Rendering/Camera/Camera.h"

namespace PixelCraft::Rendering::Camera
{

    /**
     * @brief Orthographic camera with projection based on size or explicit boundaries
     */
    class OrthographicCamera : public Camera
    {
    public:
        /**
         * @brief Default constructor
         */
        OrthographicCamera();

        /**
         * @brief Constructor with initialization parameters
         * @param size The half-height of the camera view (width is determined by aspect ratio)
         * @param aspectRatio Aspect ratio (width/height)
         * @param nearPlane Near plane distance
         * @param farPlane Far plane distance
         */
        OrthographicCamera(float size, float aspectRatio, float nearPlane, float farPlane);

        /**
         * @brief Destructor
         */
        ~OrthographicCamera() override;

        /**
         * @brief Set the size of the orthographic projection
         * @param size The half-height of the camera view
         */
        void setSize(float size);

        /**
         * @brief Get the current size of the orthographic projection
         * @return The half-height of the camera view
         */
        float getSize() const;

        /**
         * @brief Set the explicit boundaries of the orthographic projection
         * @param left Left boundary
         * @param right Right boundary
         * @param bottom Bottom boundary
         * @param top Top boundary
         */
        void setRect(float left, float right, float bottom, float top);

    protected:
        /**
         * @brief Update the projection matrix for orthographic projection
         */
        void updateProjectionMatrix() const override;

    private:
        float m_size; // Half-height of the view
        mutable float m_left, m_right, m_bottom, m_top; // Explicit boundaries
        bool m_useExplicitRect; // Whether to use explicit boundaries or derive from size
    };

} // namespace PixelCraft::Rendering