// -------------------------------------------------------------------------
// CameraComponent.h
// -------------------------------------------------------------------------
#pragma once

#include "ECS/Component.h"
#include "ECS/ComponentRegistry.h"
#include "Rendering/Camera/Camera.h"
//#include "Utility/Serialization/SerializationUtility.h"

#include <memory>
#include <string>
#include <glm/glm.hpp>

namespace Camera = PixelCraft::Rendering::Camera;

namespace PixelCraft::ECS
{

    /**
     * @brief Component that provides camera functionality to an entity
     *
     * The CameraComponent wraps a camera object to provide view and projection
     * capabilities for an entity. It can be configured as perspective or orthographic.
     */
    class CameraComponent : public Component
    {
    public:
        /**
         * @brief Camera type enumeration
         */
        enum class CameraType
        {
            Perspective,   ///< Perspective projection
            Orthographic   ///< Orthographic projection
        };

        /**
         * @brief Default constructor
         */
        CameraComponent();

        /**
         * @brief Destructor
         */
        virtual ~CameraComponent();

        /**
         * @brief Initialize the component after creation
         */
        void initialize() override;

        /**
         * @brief Create a clone of this component
         * @return A unique pointer to a new CameraComponent instance
         */
        std::unique_ptr<Component> clone() override;

        /**
         * @brief Set the camera type
         * @param type Camera type (perspective or orthographic)
         */
        void setCameraType(CameraType type);

        /**
         * @brief Get the camera type
         * @return Current camera type
         */
        CameraType getCameraType() const;

        /**
         * @brief Set the field of view (for perspective camera)
         * @param fov Field of view in degrees
         */
        void setFieldOfView(float fov);

        /**
         * @brief Get the field of view
         * @return Field of view in degrees
         */
        float getFieldOfView() const;

        /**
         * @brief Set the orthographic size (for orthographic camera)
         * @param size Half-height of the orthographic view
         */
        void setOrthographicSize(float size);

        /**
         * @brief Get the orthographic size
         * @return Half-height of the orthographic view
         */
        float getOrthographicSize() const;

        /**
         * @brief Set the near clipping plane distance
         * @param nearPlane Distance to near clipping plane
         */
        void setNearPlane(float nearPlane);

        /**
         * @brief Get the near clipping plane distance
         * @return Distance to near clipping plane
         */
        float getNearPlane() const;

        /**
         * @brief Set the far clipping plane distance
         * @param farPlane Distance to far clipping plane
         */
        void setFarPlane(float farPlane);

        /**
         * @brief Get the far clipping plane distance
         * @return Distance to far clipping plane
         */
        float getFarPlane() const;

        /**
         * @brief Set the aspect ratio
         * @param aspectRatio Width divided by height
         */
        void setAspectRatio(float aspectRatio);

        /**
         * @brief Get the aspect ratio
         * @return Width divided by height
         */
        float getAspectRatio() const;

        /**
         * @brief Get the view matrix
         * @return Matrix transforming from world space to view space
         */
        const glm::mat4& getViewMatrix() const;

        /**
         * @brief Get the projection matrix
         * @return Matrix transforming from view space to clip space
         */
        const glm::mat4& getProjectionMatrix() const;

        /**
         * @brief Get the view-projection matrix
         * @return Combined view and projection matrix
         */
        const glm::mat4& getViewProjectionMatrix() const;

        /**
         * @brief Get the internal camera object
         * @return Reference to the camera
         */
        Camera::Camera& getCamera();

        /**
         * @brief Get the internal camera object (const version)
         * @return Const reference to the camera
         */
        const Camera::Camera& getCamera() const;

        /**
         * @brief Set this camera as the main camera
         * @param isMain True to set as main camera
         */
        void setMain(bool isMain);

        /**
         * @brief Check if this is the main camera
         * @return True if this is the main camera
         */
        bool isMain() const;

        // Override serialization methods
        Utility::Serialization::SerializationResult serialize(Utility::Serialization::Serializer& serializer) const override
        {
            // First serialize base component data
            auto result = Component::serialize(serializer);
            if (!result) return result;

            // Now serialize camera-specific data
            result = serializer.beginObject("CameraComponent");
            if (!result) return result;

            result = serializer.writeField("camera", m_camera);
            if (!result) return result;

            result = serializer.writeField("cameraType", m_cameraType);
            if (!result) return result;

            result = serializer.writeField("fov", m_fov);
            if (!result) return result;

            result = serializer.writeField("orthoSize", m_orthoSize);
            if (!result) return result;

            result = serializer.writeField("nearPlane", m_nearPlane);
            if (!result) return result;

            result = serializer.writeField("farPlane", m_farPlane);
            if (!result) return result;

            result = serializer.writeField("aspectRatio", m_aspectRatio);
            if (!result) return result;

            result = serializer.writeField("isMain", m_isMain);
            if (!result) return result;

            return serializer.endObject();
        }

        Utility::Serialization::SerializationResult deserialize(Utility::Serialization::Deserializer& deserializer) override
        {
            // First deserialize base component data
            auto result = Component::deserialize(deserializer);
            if (!result) return result;

            // Now deserialize camera-specific data
            result = deserializer.beginObject("CameraComponent");
            if (!result) return result;

            result = deserializer.readField("camera", m_camera);
            if (!result) return result;

            result = deserializer.readField("cameraType", m_cameraType);
            if (!result) return result;

            result = deserializer.readField("fov", m_fov);
            if (!result) return result;

            result = deserializer.readField("orthoSize", m_orthoSize);
            if (!result) return result;

            result = deserializer.readField("nearPlane", m_nearPlane);
            if (!result) return result;

            result = deserializer.readField("farPlane", m_farPlane);
            if (!result) return result;

            result = deserializer.readField("aspectRatio", m_aspectRatio);
            if (!result) return result;

            result = deserializer.readField("isMain", m_isMain);
            if (!result) return result;

            return deserializer.endObject();
        }

        static void defineSchema(Utility::Serialization::Schema& schema)
        {
            // Include base component fields
            Component::defineSchema(schema);

            // Add camera-specific fields
            schema.addField("camera", Utility::Serialization::ValueType::Object);
            schema.addField("cameraType", Utility::Serialization::ValueType::Object);
            schema.addField("fov", Utility::Serialization::ValueType::Float);
            schema.addField("orthoSize", Utility::Serialization::ValueType::Float);
            schema.addField("nearPlane", Utility::Serialization::ValueType::Float);
            schema.addField("farPlane", Utility::Serialization::ValueType::Float);
            schema.addField("aspectRatio", Utility::Serialization::ValueType::Float);
            schema.addField("isMain", Utility::Serialization::ValueType::Bool);
        }

        static std::string getTypeName()
        {
            return "CameraComponent";
        }

        // Type identification implementation
        DEFINE_COMPONENT_TYPE(CameraComponent, 2)

    private:
        /**
         * @brief Create the appropriate camera type
         */
        void createCamera();

        /**
         * @brief Update the render system main camera reference
         */
        void updateMainCameraReference();

        std::shared_ptr<Camera::Camera> m_camera;              ///< Internal camera object
        CameraType m_cameraType;                               ///< Camera type (perspective/orthographic)
        float m_fov;                                           ///< Field of view (for perspective)
        float m_orthoSize;                                     ///< Orthographic size (for orthographic)
        float m_nearPlane;                                     ///< Near clipping plane
        float m_farPlane;                                      ///< Far clipping plane
        float m_aspectRatio;                                   ///< Aspect ratio (width/height)
        bool m_isMain;                                         ///< Flag for main camera
    };

} // namespace PixelCraft::ECS