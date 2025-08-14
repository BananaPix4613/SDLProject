// -------------------------------------------------------------------------
// VelocityStage.cpp
// -------------------------------------------------------------------------

#include "Rendering/Stages/VelocityStage.h"
#include "Rendering/RenderContext.h"
#include "Rendering/Shader.h"
#include "ECS/Registry.h"
#include "ECS/View.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshRendererComponent.h"
#include "Utility/Profiler.h"
#include "Core/Logger.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>

namespace PixelCraft::Rendering::Stages
{

    VelocityStage::VelocityStage()
        : RenderStage("VelocityStage")
        , m_velocityShader(nullptr)
        , m_cameraVelocityShader(nullptr)
        , m_registry(nullptr)
        , m_previousViewProjectionMatrix(1.0f)
        , m_cameraMotionEnabled(true)
        , m_velocityScale(1.0f)
        , m_ndcEncoding(false)
        , m_maxVelocity(64.0f)
    {
    }

    VelocityStage::~VelocityStage()
    {
        shutdown();
    }

    bool VelocityStage::initialize()
    {
        Core::info("Initializing VelocityStage");
        
        // Clear previous transforms
        m_previousTransforms.clear();

        setInitialized(true);
        Core::info("VelocityStage initialized successfully");
        return true;
    }

    void VelocityStage::shutdown()
    {
        Core::info("Shutting down VelocityStage");
        
        m_velocityShader.reset();
        m_cameraVelocityShader.reset();
        m_registry.reset();
        m_previousTransforms.clear();

        setInitialized(false);
    }

    void VelocityStage::execute(const std::shared_ptr<RenderContext> context)
    {
        if (!isInitialized() || !isEnabled())
        {
            Core::warn("UIStage: Not initialized or enabled");
            return;
        }

        PROFILE_SCOPE("VelocityStage::execute");

        if (!context || !m_velocityShader || !m_registry)
        {
            return;
        }

        // Setup render state for velocity generation
        setupVelocityRenderState();

        // Render dynamic object velocity
        renderDynamicVelocity(context);

        // Generate camera motion velocity if enabled
        if (m_cameraMotionEnabled)
        {
            generateCameraVelocity(context);
        }

        // Restore render state
        restoreRenderState();
    }

    void VelocityStage::setVelocityShader(std::shared_ptr<Shader> shader)
    {
        m_velocityShader = shader;
    }

    std::shared_ptr<Shader> VelocityStage::getVelocityShader() const
    {
        return m_velocityShader;
    }

    void VelocityStage::setRegistry(std::shared_ptr<ECS::Registry> registry)
    {
        m_registry = registry;
    }

    void VelocityStage::setPreviousViewProjectionMatrix(const glm::mat4& matrix)
    {
        m_previousViewProjectionMatrix = matrix;
    }

    const glm::mat4& VelocityStage::getPreviousViewProjectionMatrix() const
    {
        return m_previousViewProjectionMatrix;
    }

    void VelocityStage::setCameraMotionEnabled(bool enabled)
    {
        m_cameraMotionEnabled = enabled;
    }

    bool VelocityStage::isCameraMotionEnabled() const
    {
        return m_cameraMotionEnabled;
    }

    void VelocityStage::setVelocityScale(float scale)
    {
        m_velocityScale = scale;
    }

    float VelocityStage::getVelocityScale() const
    {
        return m_velocityScale;
    }

    void VelocityStage::setNDCEncoding(bool enabled)
    {
        m_ndcEncoding = enabled;
    }

    bool VelocityStage::isNDCEncodingEnabled() const
    {
        return m_ndcEncoding;
    }

    void VelocityStage::setMaxVelocity(float maxVelocity)
    {
        m_maxVelocity = maxVelocity;
    }

    float VelocityStage::getMaxVelocity() const
    {
        return m_maxVelocity;
    }

    void VelocityStage::renderDynamicVelocity(const std::shared_ptr<RenderContext> context)
    {
        if (!m_velocityShader || !m_registry)
        {
            return;
        }

        m_velocityShader->bind();

        // Set uniforms
        m_velocityShader->setMat4("u_previousViewProjection", m_previousViewProjectionMatrix);
        m_velocityShader->setFloat("u_velocityScale", m_velocityScale);
        m_velocityShader->setFloat("u_maxVelocity", m_maxVelocity);
        m_velocityShader->setInt("u_ndcEncoding", m_ndcEncoding ? 1 : 0);

        // Render all entities with mesh renderers
        auto view = m_registry->view<ECS::TransformComponent, ECS::MeshRendererComponent>();
        
        for (auto [entity, transform, meshRenderer] : view)
        {
            auto mesh = meshRenderer->getMesh();
            if (!mesh)
            {
                continue;
            }

            // Get current transform matrix
            glm::mat4 currentTransform = transform->getWorldTransform().getMatrix();

            // Get previous transform matrix
            glm::mat4 previousTransform = currentTransform;
            auto it = m_previousTransforms.find(entity);
            if (it != m_previousTransforms.end())
            {
                previousTransform = it->second;
            }

            // Set transform uniforms
            m_velocityShader->setMat4("u_currentTransform", currentTransform);
            m_velocityShader->setMat4("u_previousTransform", previousTransform);

            // Bind and draw the mesh
            mesh->bind();
            mesh->draw();
            mesh->unbind();

            // Update previous transform for next frame
            updatePreviousTransform(entity);
        }
    }

    void VelocityStage::generateCameraVelocity(const std::shared_ptr<RenderContext> context)
    {
        if (!m_cameraVelocityShader)
        {
            return;
        }

        // Camera velocity generation would be implemented here
        // This is a placeholder for camera motion vector generation
    }

    void VelocityStage::setupVelocityRenderState()
    {
        // Enable depth testing but disable depth writes
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        
        // Disable blending for velocity buffer
        glDisable(GL_BLEND);
        
        // Enable back-face culling
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }

    void VelocityStage::restoreRenderState()
    {
        // Restore default render state
        glDepthMask(GL_TRUE);
        glDisable(GL_CULL_FACE);
    }

    void VelocityStage::updatePreviousTransform(ECS::EntityID entity)
    {
        if (!m_registry)
        {
            return;
        }

        if (m_registry->hasComponent<ECS::TransformComponent>(entity))
        {
            auto& transform = m_registry->getComponent<ECS::TransformComponent>(entity);
            m_previousTransforms[entity] = transform.getWorldTransform().getMatrix();
        }
    }

} // namespace PixelCraft::Rendering::Stages
