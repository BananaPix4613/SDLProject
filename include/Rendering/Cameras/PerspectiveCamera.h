// -------------------------------------------------------------------------
// PerspectiveCamera.h
// -------------------------------------------------------------------------
#pragma once

#include "Rendering/Cameras/Camera.h"

namespace PixelCraft::Utility::Serialization
{
    class Serializer;
    class Deserializer;
    struct SerializationResult;
    class Schema;
} // namespace PixelCraft::Utility::Serialization

namespace PixelCraft::Rendering::Cameras
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

        /**
         * @brief Serialize the perspective camera configuration to the given serializer
         * @param serializer The serializer to use
         * @return SerializationResult with success/failure status
         */
        Utility::Serialization::SerializationResult
        serialize(Utility::Serialization::Serializer &serializer) const override;

        /**
         * @brief Deserialize the perspective camera configuration from the given deserializer
         * @param deserializer The deserializer to use
         * @return SerializationResult with success/failure status
         */
        Utility::Serialization::SerializationResult
        deserialize(Utility::Serialization::Deserializer &deserializer) override;

        /**
         * @brief Define the schema for perspective camera serialization
         * @param schema The schema to define
         */
        static void defineSchema(Utility::Serialization::Schema &schema);

        /**
         * @brief Get the type name for serialization
         * @return The type name string
         */
        static std::string getTypeName();

    protected:
        /**
         * @brief Update the projection matrix for perspective projection
         */
        void updateProjectionMatrix() const override;

    private:
        float m_fov; // Field of view in radians
    };

} // namespace PixelCraft::Rendering::Cameras
