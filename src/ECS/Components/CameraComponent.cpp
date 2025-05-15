// -------------------------------------------------------------------------
// CameraComponent.cpp
// -------------------------------------------------------------------------
#include "ECS/Components/CameraComponent.h"
#include "Core/Application.h"
#include "ECS/Serializer.h"
#include "ECS/Deserializer.h"
#include "Rendering/RenderSystem.h"
#include "Rendering/Camera/OrthographicCamera.h"
#include "Rendering/Camera/PerspectiveCamera.h"

namespace Log = PixelCraft::Core;

namespace PixelCraft::ECS
{

    CameraComponent::CameraComponent()
        : m_cameraType(CameraType::Perspective)
        , m_fov(60.0f)
        , m_orthoSize(5.0f)
        , m_nearPlane(0.1f)
        , m_farPlane(1000.0f)
        , m_aspectRatio(16.0f / 9.0f)
        , m_isMain(false)
    {
    }

    CameraComponent::~CameraComponent()
    {
        if (m_isMain)
        {
            // If this was the main camera, clear it in the render system
            std::shared_ptr<Rendering::RenderSystem> renderSystem = Core::Application::getInstance().getSubsystem<Rendering::RenderSystem>();
            if (renderSystem)
            {
                renderSystem->setMainCamera(nullptr);
            }
        }
    }

    void CameraComponent::initialize()
    {
        Component::initialize();

        // Create the appropriate camera type
        createCamera();

        // If this is the main camera, update the render system
        if (m_isMain)
        {
            updateMainCameraReference();
        }
    }

    std::unique_ptr<Component> CameraComponent::clone()
    {
        auto clone = std::make_unique<CameraComponent>();

        // Copy camera properties
        clone->m_cameraType = m_cameraType;
        clone->m_fov = m_fov;
        clone->m_orthoSize = m_orthoSize;
        clone->m_nearPlane = m_nearPlane;
        clone->m_farPlane = m_farPlane;
        clone->m_aspectRatio = m_aspectRatio;
        clone->m_isMain = false; // Don't clone the main flag (there can only be one main camera)

        // Create the camera object with the cloned properties
        clone->createCamera();

        return clone;
    }

    void CameraComponent::setCameraType(CameraType type)
    {
        if (m_cameraType != type)
        {
            m_cameraType = type;
            createCamera();
        }
    }

    CameraComponent::CameraType CameraComponent::getCameraType() const
    {
        return m_cameraType;
    }

    void CameraComponent::setFieldOfView(float fov)
    {
        m_fov = fov;

        if (m_cameraType == CameraType::Perspective && m_camera)
        {
            auto* perspectiveCamera = static_cast<Camera::PerspectiveCamera*>(m_camera.get());
            perspectiveCamera->setFOV(m_fov);
        }
    }

    float CameraComponent::getFieldOfView() const
    {
        return m_fov;
    }

    void CameraComponent::setOrthographicSize(float size)
    {
        m_orthoSize = size;

        if (m_cameraType == CameraType::Orthographic && m_camera)
        {
            auto* orthoCamera = static_cast<Camera::OrthographicCamera*>(m_camera.get());
            orthoCamera->setSize(m_orthoSize);
        }
    }

    float CameraComponent::getOrthographicSize() const
    {
        return m_orthoSize;
    }

    void CameraComponent::setNearPlane(float nearPlane)
    {
        m_nearPlane = nearPlane;

        if (m_camera)
        {
            m_camera->setNearPlane(m_nearPlane);
        }
    }

    float CameraComponent::getNearPlane() const
    {
        return m_nearPlane;
    }

    void CameraComponent::setFarPlane(float farPlane)
    {
        m_farPlane = farPlane;

        if (m_camera)
        {
            m_camera->setFarPlane(m_farPlane);
        }
    }

    float CameraComponent::getFarPlane() const
    {
        return m_farPlane;
    }

    void CameraComponent::setAspectRatio(float aspectRatio)
    {
        m_aspectRatio = aspectRatio;

        if (m_camera)
        {
            m_camera->setAspectRatio(m_aspectRatio);
        }
    }

    float CameraComponent::getAspectRatio() const
    {
        return m_aspectRatio;
    }

    const glm::mat4& CameraComponent::getViewMatrix() const
    {
        if (m_camera)
        {
            return m_camera->getViewMatrix();
        }

        static const glm::mat4 identity(1.0f);
        return identity;
    }

    const glm::mat4& CameraComponent::getProjectionMatrix() const
    {
        if (m_camera)
        {
            return m_camera->getProjectionMatrix();
        }

        static const glm::mat4 identity(1.0f);
        return identity;
    }

    const glm::mat4& CameraComponent::getViewProjectionMatrix() const
    {
        if (m_camera)
        {
            return m_camera->getViewProjectionMatrix();
        }

        static const glm::mat4 identity(1.0f);
        return identity;
    }

    Camera::Camera& CameraComponent::getCamera()
    {
        if (!m_camera)
        {
            createCamera();
        }
        return *m_camera;
    }

    const Camera::Camera& CameraComponent::getCamera() const
    {
        if (!m_camera)
        {
            // This is a bit of a hack, but we need to create the camera on-demand
            // even in const methods
            const_cast<CameraComponent*>(this)->createCamera();
        }
        return *m_camera;
    }

    void CameraComponent::setMain(bool isMain)
    {
        if (m_isMain != isMain)
        {
            m_isMain = isMain;

            updateMainCameraReference();
        }
    }

    bool CameraComponent::isMain() const
    {
        return m_isMain;
    }

    void CameraComponent::serialize(Serializer& serializer)
    {
        // Serialize base component properties
        Component::serialize(serializer);

        // Serialize camera properties
        serializer.beginObject("camera");
        serializer.write(static_cast<int>(m_cameraType));
        serializer.write(m_fov);
        serializer.write(m_orthoSize);
        serializer.write(m_nearPlane);
        serializer.write(m_farPlane);
        serializer.write(m_aspectRatio);
        serializer.write(m_isMain);
        serializer.endObject();
    }

    void CameraComponent::deserialize(Deserializer& deserializer)
    {
        // Deserialize base component properties
        Component::deserialize(deserializer);

        // Deserialize camera properties
        if (deserializer.beginObject("camera"))
        {
            int cameraType;
            deserializer.read(cameraType);
            m_cameraType = static_cast<CameraType>(cameraType);

            deserializer.read(m_fov);
            deserializer.read(m_orthoSize);
            deserializer.read(m_nearPlane);
            deserializer.read(m_farPlane);
            deserializer.read(m_aspectRatio);
            deserializer.read(m_isMain);
            deserializer.endObject();
        }

        // Recreate the camera with the deserialized settings
        createCamera();

        // Update main camera reference if needed
        if (m_isMain)
        {
            updateMainCameraReference();
        }
    }

    void CameraComponent::createCamera()
    {
        if (m_cameraType == CameraType::Perspective)
        {
            auto perspectiveCamera = std::make_shared<Camera::PerspectiveCamera>();
            perspectiveCamera->setFOV(m_fov);
            perspectiveCamera->setAspectRatio(m_aspectRatio);
            perspectiveCamera->setNearPlane(m_nearPlane);
            perspectiveCamera->setFarPlane(m_farPlane);
            m_camera = perspectiveCamera;
        }
        else
        { // Orthographic
            auto orthoCamera = std::make_shared<Camera::OrthographicCamera>();
            orthoCamera->setSize(m_orthoSize);
            orthoCamera->setAspectRatio(m_aspectRatio);
            orthoCamera->setNearPlane(m_nearPlane);
            orthoCamera->setFarPlane(m_farPlane);
            m_camera = orthoCamera;
        }
    }

    void CameraComponent::updateMainCameraReference()
    {
        auto renderSystem = Core::Application::getInstance().getSubsystem<Rendering::RenderSystem>();
        if (renderSystem)
        {
            if (m_isMain)
            {
                renderSystem->setMainCamera(m_camera);
            }
            else if (renderSystem->getMainCamera() == m_camera)
            {
                // Only clear if this camera was the main one
                renderSystem->setMainCamera(nullptr);
            }
        }
    }

    std::unique_ptr<FlatBuffers::Schema> CameraComponent::createSchema()
    {
        // This implementation creates the FlatBuffers schema
        // for efficient serialization of the CameraComponent
        class CameraSchema : public FlatBuffers::Schema
        {
        public:
            std::string getDefinition() const override
            {
                return R"(
namespace PixelCraft.ECS;

table CameraComponentData {
  camera_type:uint8 = 0;  // 0 = Perspective, 1 = Orthographic
  fov:float = 60.0;
  ortho_size:float = 5.0;
  near_plane:float = 0.1;
  far_plane:float = 1000.0;
  aspect_ratio:float = 1.777778;  // 16:9 default
  is_main:bool = false;
}

root_type CameraComponentData;
            )";
            }

            std::string getIdentifier() const override
            {
                return "CAMR"; // PixelCraft Camera FlatBuffer
            }

            uint32_t getVersion() const override
            {
                return 1; // Initial schema version
            }
        };

        return std::make_unique<CameraSchema>();
    }

    namespace
    {
        // Register the CameraComponent with the component registry
        const bool registered = ComponentRegistry::registerComponentType<CameraComponent>("CameraComponent");
    }

} // namespace PixelCraft::ECS