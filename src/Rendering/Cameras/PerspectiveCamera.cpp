// -------------------------------------------------------------------------
// PerspectiveCamera.cpp
// -------------------------------------------------------------------------
#include "Rendering/Cameras/PerspectiveCamera.h"
#include "Utility/Serialization/SerializationUtility.h"
#include "Utility/Serialization/Serializer.h"
#include "Utility/Serialization/Deserializer.h"
#include "Core/Logger.h"

#include <glm/gtc/matrix_transform.hpp>

namespace PixelCraft::Rendering::Cameras
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
            Core::warn("PerspectiveCamera::setFOV: Invalid field of view (must be between 0 and 180 degrees)");
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

    Utility::Serialization::SerializationResult PerspectiveCamera::serialize(Utility::Serialization::Serializer& serializer) const
    {
        // First serialize the base camera data
        auto result = Camera::serialize(serializer);
        if (!result) return result;

        // Then serialize perspective-specific data
        result = serializer.writeField("fov", m_fov);
        if (!result) return result;

        return Utility::Serialization::SerializationResult();
    }

    Utility::Serialization::SerializationResult PerspectiveCamera::deserialize(Utility::Serialization::Deserializer& deserializer)
    {
        // First deserialize the base camera data
        auto result = Camera::deserialize(deserializer);
        if (!result) return result;

        // Then deserialize perspective-specific data
        result = deserializer.readField("fov", m_fov);
        if (!result) return result;

        // Mark projection matrix as dirty after deserialization
        m_projectionDirty = true;

        return Utility::Serialization::SerializationResult();
    }

    void PerspectiveCamera::defineSchema(Utility::Serialization::Schema& schema)
    {
        // First define the base camera schema
        Camera::defineSchema(schema);

        // Then add perspective-specific fields
        schema.addField("fov", Utility::Serialization::ValueType::Float);
    }

    std::string PerspectiveCamera::getTypeName()
    {
        return "PerspectiveCamera";
    }

} // namespace PixelCraft::Rendering::Cameras
