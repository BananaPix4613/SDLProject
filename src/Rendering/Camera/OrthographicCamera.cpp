// -------------------------------------------------------------------------
// OrthographicCamera.cpp
// -------------------------------------------------------------------------
#include "Rendering/Camera/OrthographicCamera.h"
#include "Core/Logger.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Rendering::Camera
{

    OrthographicCamera::OrthographicCamera()
        : Camera()
        , m_size(5.0f) // Default size
        , m_left(0.0f)
        , m_right(0.0f)
        , m_bottom(0.0f)
        , m_top(0.0f)
        , m_useExplicitRect(false)
    {
        updateProjectionMatrix();
    }

    OrthographicCamera::OrthographicCamera(float size, float aspectRatio, float nearPlane, float farPlane)
        : Camera()
        , m_size(size)
        , m_left(0.0f)
        , m_right(0.0f)
        , m_bottom(0.0f)
        , m_top(0.0f)
        , m_useExplicitRect(false)
    {
        setAspectRatio(aspectRatio);
        setNearPlane(nearPlane);
        setFarPlane(farPlane);
        updateProjectionMatrix();
    }

    OrthographicCamera::~OrthographicCamera()
    {
        // Nothing to clean up
    }

    void OrthographicCamera::setSize(float size)
    {
        if (size <= 0.0f)
        {
            Log::warn("OrthographicCamera::setSize: Invalid size (must be positive)");
            return;
        }

        m_size = size;
        m_useExplicitRect = false;
        m_projectionDirty = true;
    }

    float OrthographicCamera::getSize() const
    {
        return m_size;
    }

    void OrthographicCamera::setRect(float left, float right, float bottom, float top)
    {
        if (left >= right || bottom >= top)
        {
            Log::warn("OrthographicCamera::setRect: Invalid boundaries (left must be less than right, bottom must be less than top)");
            return;
        }

        m_left = left;
        m_right = right;
        m_bottom = bottom;
        m_top = top;
        m_useExplicitRect = true;
        m_projectionDirty = true;
    }

    void OrthographicCamera::updateProjectionMatrix() const
    {
        if (m_useExplicitRect)
        {
            // Use explicit boundaries
            m_projectionMatrix = glm::ortho(m_left, m_right, m_bottom, m_top, m_nearPlane, m_farPlane);
        }
        else
        {
            // Calculate boundaries from size and aspect ratio
            float halfHeight = m_size;
            float halfWidth = halfHeight * m_aspectRatio;

            m_left = -halfWidth;
            m_right = halfWidth;
            m_bottom = -halfHeight;
            m_top = halfHeight;

            m_projectionMatrix = glm::ortho(m_left, m_right, m_bottom, m_top, m_nearPlane, m_farPlane);
        }

        m_projectionDirty = false;
    }

} // namespace PixelCraft::Rendering