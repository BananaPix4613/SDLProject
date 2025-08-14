// -------------------------------------------------------------------------
// OrthographicCamera.cpp
// -------------------------------------------------------------------------
#include "Rendering/Cameras/OrthographicCamera.h"
#include "Utility/Serialization/SerializationUtility.h"
#include "Utility/Serialization/Serializer.h"
#include "Utility/Serialization/Deserializer.h"
#include "Core/Logger.h"

#include <glm/gtc/matrix_transform.hpp>

namespace PixelCraft::Rendering::Cameras
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
            Core::warn("OrthographicCamera::setSize: Invalid size (must be positive)");
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
            Core::warn("OrthographicCamera::setRect: Invalid boundaries (left must be less than right, bottom must be less than top)");
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

    Utility::Serialization::SerializationResult OrthographicCamera::serialize(Utility::Serialization::Serializer& serializer) const
    {
        // First serialize the base camera data
        auto result = Camera::serialize(serializer);
        if (!result) return result;

        // Then serialize orthographic-specific data
        result = serializer.writeField("size", m_size);
        if (!result) return result;

        result = serializer.writeField("useExplicitRect", m_useExplicitRect);
        if (!result) return result;

        if (m_useExplicitRect)
        {
            result = serializer.writeField("left", m_left);
            if (!result) return result;

            result = serializer.writeField("right", m_right);
            if (!result) return result;

            result = serializer.writeField("bottom", m_bottom);
            if (!result) return result;

            result = serializer.writeField("top", m_top);
            if (!result) return result;
        }

        return Utility::Serialization::SerializationResult();
    }

    Utility::Serialization::SerializationResult OrthographicCamera::deserialize(Utility::Serialization::Deserializer& deserializer)
    {
        // First deserialize the base camera data
        auto result = Camera::deserialize(deserializer);
        if (!result) return result;

        // Then deserialize orthographic-specific data
        result = deserializer.readField("size", m_size);
        if (!result) return result;

        result = deserializer.readField("useExplicitRect", m_useExplicitRect);
        if (!result) return result;

        if (m_useExplicitRect)
        {
            result = deserializer.readField("left", m_left);
            if (!result) return result;

            result = deserializer.readField("right", m_right);
            if (!result) return result;

            result = deserializer.readField("bottom", m_bottom);
            if (!result) return result;

            result = deserializer.readField("top", m_top);
            if (!result) return result;
        }

        // Mark projection matrix as dirty after deserialization
        m_projectionDirty = true;

        return Utility::Serialization::SerializationResult();
    }

    void OrthographicCamera::defineSchema(Utility::Serialization::Schema& schema)
    {
        // First define the base camera schema
        Camera::defineSchema(schema);

        // Then add orthographic-specific fields
        schema.addField("size", Utility::Serialization::ValueType::Float);
        schema.addField("useExplicitRect", Utility::Serialization::ValueType::Bool);
        schema.addField("left", Utility::Serialization::ValueType::Float);
        schema.addField("right", Utility::Serialization::ValueType::Float);
        schema.addField("bottom", Utility::Serialization::ValueType::Float);
        schema.addField("top", Utility::Serialization::ValueType::Float);
    }

    std::string OrthographicCamera::getTypeName()
    {
        return "OrthographicCamera";
    }

} // namespace PixelCraft::Rendering::Cameras
