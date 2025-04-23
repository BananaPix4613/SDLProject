// -------------------------------------------------------------------------
// PerspectiveCamera.cpp
// -------------------------------------------------------------------------
#include "Rendering/Camera/PerspectiveCamera.h"
#include "Core/Logger.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Rendering::Camera
{

    PerspectiveCamera::PerspectiveCamera()
        : Camera()
        , m_fov(glm::radians(60.0f)) // Default 60 degrees FOV
    {
        updateProjectionMatrix();
    }

    PerspectiveCamera::PerspectiveCamera(float fov, float aspectRatio, float nearPlane, float farPlane)
        : Camera()
        , m_fov(fov)
    {
        setAspectRatio(aspectRatio);
        setNearPlane(nearPlane);
        setFarPlane(farPlane);
        updateProjectionMatrix();
    }

    PerspectiveCamera::~PerspectiveCamera()
    {
        // Nothing to clean up
    }

    void PerspectiveCamera::setFOV(float fov)
    {
        if (fov <= 0.0f || fov >= glm::radians(180.0f))
        {
            Log::warn("PerspectiveCamera::setFOV: Invalid field of view (must be between 0 and 180 degrees)");
            return;
        }

        m_fov = fov;
        m_projectionDirty = true;
    }

    float PerspectiveCamera::getFOV() const
    {
        return m_fov;
    }

    void PerspectiveCamera::updateProjectionMatrix() const
    {
        // Create a perspective projection matrix
        m_projectionMatrix = glm::perspective(m_fov, m_aspectRatio, m_nearPlane, m_farPlane);
        m_projectionDirty = false;
    }

} // namespace PixelCraft::Rendering