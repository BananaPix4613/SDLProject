// -------------------------------------------------------------------------
// CameraComponent.cpp
// -------------------------------------------------------------------------
#include "ECS/Components/CameraComponent.h"
#include "Core/Application.h"
#include "Rendering/RenderSystem.h"
#include "Rendering/Cameras/OrthographicCamera.h"
#include "Rendering/Cameras/PerspectiveCamera.h"

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
            std::shared_ptr<Rendering::RenderSystem> renderSystem = Core::Application::getInstance()->getSubsystem<Rendering::RenderSystem>();
            if (renderSystem)
            {
                renderSystem->setMainCamera(nullptr);
            }
        }
    }

    void CameraComponent::initialize()
    {
        Component::initialize();
        m_typeId = 2;
        m_typeName = "CameraComponent";

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
            auto* perspectiveCamera = static_cast<Rendering::Cameras::PerspectiveCamera*>(m_camera.get());
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
            auto* orthoCamera = static_cast<Rendering::Cameras::OrthographicCamera*>(m_camera.get());
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

    Rendering::Cameras::Camera& CameraComponent::getCamera()
    {
        if (!m_camera)
        {
            createCamera();
        }
        return *m_camera;
    }

    const Rendering::Cameras::Camera& CameraComponent::getCamera() const
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

    void CameraComponent::createCamera()
    {
        if (m_cameraType == CameraType::Perspective)
        {
            auto perspectiveCamera = std::make_shared<Rendering::Cameras::PerspectiveCamera>();
            perspectiveCamera->setFOV(m_fov);
            perspectiveCamera->setAspectRatio(m_aspectRatio);
            perspectiveCamera->setNearPlane(m_nearPlane);
            perspectiveCamera->setFarPlane(m_farPlane);
            m_camera = perspectiveCamera;
        }
        else
        { // Orthographic
            auto orthoCamera = std::make_shared<Rendering::Cameras::OrthographicCamera>();
            orthoCamera->setSize(m_orthoSize);
            orthoCamera->setAspectRatio(m_aspectRatio);
            orthoCamera->setNearPlane(m_nearPlane);
            orthoCamera->setFarPlane(m_farPlane);
            m_camera = orthoCamera;
        }
    }

    void CameraComponent::updateMainCameraReference()
    {
        auto renderSystem = Core::Application::getInstance()->getSubsystem<Rendering::RenderSystem>();
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

    namespace
    {
        // Register the CameraComponent with the component registry
        bool registerCameraComponent()
        {
            ComponentRegistry::registerComponent<CameraComponent>("CameraComponent");
            return true;
        }

        const bool registered = registerCameraComponent();
    }

} // namespace PixelCraft::ECS