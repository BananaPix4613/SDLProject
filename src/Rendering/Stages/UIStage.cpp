// -------------------------------------------------------------------------
// UIStage.cpp
// -------------------------------------------------------------------------
#include "Rendering/Stages/UIStage.h"
#include "Core/Logger.h"
#include "Rendering/Shader.h"
#include "Rendering/RenderContext.h"
#include "Utility/Profiler.h"

namespace PixelCraft::Rendering::Stages
{

    UIStage::UIStage()
        : RenderStage("UIStage")
        , m_uiShader(nullptr)
        , m_registry(nullptr)
        , m_screenWidth(1920)
        , m_screenHeight(1080)
        , m_depthTestEnabled(false)
        , m_profilingMarker("UIStage")
    {
    }

    UIStage::~UIStage()
    {
        shutdown();
    }

    bool UIStage::initialize()
    {
        if (isInitialized())
        {
            Core::warn("UIStage already initialized");
            return true;
        }

        Core::info("Initializing UIStage");
        setInitialized(true);
        return true;
    }

    void UIStage::shutdown()
    {
        if (!isInitialized())
        {
            return;
        }

        Core::info("Shutting down UIStage");
        m_uiShader.reset();
        m_registry.reset();
        setInitialized(false);
    }

    void UIStage::execute(const std::shared_ptr<RenderContext> context)
    {
        if (!isInitialized() || !isEnabled())
        {
            Core::warn("UIStage: Not initialized or enabled");
            return;
        }

        PROFILE_SCOPE("UIStage::execute");

        if (!m_uiShader || !m_registry)
        {
            Core::warn("UIStage: Missing shader or registry");
            return;
        }

        // CURRENTLY DISABLED: UI rendering is not implemented yet

        bindOutput();
        setupUIRenderState();
        setupUIProjection();

        m_uiShader->bind();

        // TODO: Render UI entities
        /*
        m_registry->view<TransformComponent, UIComponent>().each([&](auto entity, auto& transform, auto& ui) {
            if (!ui.visible) return;

            // Set model matrix
            m_uiShader->setMat4("modelMatrix", transform.getWorldMatrix());

            // Bind UI element properties
            if (ui.texture) {
                ui.texture->bind(0);
                m_uiShader->setInt("uiTexture", 0);
            }

            m_uiShader->setVec4("uiColor", ui.color);

            // Render UI mesh/quad
            if (ui.mesh) {
                ui.mesh->bind();
                ui.mesh->draw();
                ui.mesh->unbind();
            }
        });
        */

        m_uiShader->unbind();
        restoreRenderState();
    }

    void UIStage::setUIShader(std::shared_ptr<Shader> shader)
    {
        m_uiShader = shader;
    }

    std::shared_ptr<Shader> UIStage::getUIShader() const
    {
        return m_uiShader;
    }

    void UIStage::setRegistry(std::shared_ptr<ECS::Registry> registry)
    {
        m_registry = registry;
    }

    void UIStage::setScreenDimensions(int width, int height)
    {
        m_screenWidth = width;
        m_screenHeight = height;
    }

    int UIStage::getScreenWidth() const
    {
        return m_screenWidth;
    }

    int UIStage::getScreenHeight() const
    {
        return m_screenHeight;
    }

    void UIStage::setDepthTestEnabled(bool enabled)
    {
        m_depthTestEnabled = enabled;
    }

    bool UIStage::isDepthTestEnabled() const
    {
        return m_depthTestEnabled;
    }

    void UIStage::setupUIRenderState()
    {
        // Configure depth testing
        if (m_depthTestEnabled)
        {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_FALSE); // Don't write to depth buffer
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }

        // Enable blending for UI transparency
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Disable face culling for UI
        glDisable(GL_CULL_FACE);
    }

    void UIStage::restoreRenderState()
    {
        // Re-enable depth testing
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);

        // Disable blending
        glDisable(GL_BLEND);

        // Re-enable face culling
        glEnable(GL_CULL_FACE);
    }

    void UIStage::setupUIProjection()
    {
        // Create orthographic projection for UI
        glm::mat4 uiProjection = glm::ortho(
            0.0f, static_cast<float>(m_screenWidth),
            0.0f, static_cast<float>(m_screenHeight),
            -1.0f, 1.0f
        );

        glm::mat4 uiView = glm::mat4(1.0f); // Identity view matrix

        if (m_uiShader)
        {
            m_uiShader->setMat4("projectionMatrix", uiProjection);
            m_uiShader->setMat4("viewMatrix", uiView);
        }
    }

} // namespace PixelCraft::Rendering::Stages