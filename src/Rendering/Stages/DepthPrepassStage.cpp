// -------------------------------------------------------------------------
// DepthPrepassStage.cpp
// -------------------------------------------------------------------------
#include "Rendering/Stages/DepthPrepassStage.h"
#include "Core/Logger.h"
#include "Rendering/Shader.h"
#include "Rendering/RenderSystem.h"
#include "Rendering/RenderContext.h"
#include "Rendering/RenderTarget.h"
#include "Rendering/Renderable.h"
#include "Utility/Profiler.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshRendererComponent.h"
#include "ECS/Registry.h"
#include "ECS/View.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

// Debug macro for OpenGL error checking
#ifdef _DEBUG
#define CHECK_GL_ERROR(operation) \
    do { \
        operation; \
        GLenum error = glGetError(); \
        if (error != GL_NO_ERROR) { \
            Core::error("DepthPrepassStage: OpenGL error in " #operation ": " + std::to_string(error)); \
        } \
    } while(0)
#else
#define CHECK_GL_ERROR(operation) operation
#endif

namespace PixelCraft::Rendering::Stages
{

    DepthPrepassStage::DepthPrepassStage()
        : RenderStage("DepthPrepassStage")
        , m_depthShader(nullptr)
        , m_registry(nullptr)
        , m_renderSystem(nullptr)
        , m_materialFilterMode(MaterialFilterMode::OpaqueOnly)
        , m_alphaThreshold(0.5f)
        , m_depthBias(0.0f)
        , m_orthographicOptimization(false)
        , m_earlyZRejection(true)
        , m_depthTest(true)
        , m_depthWrite(true)
    {
    }

    DepthPrepassStage::~DepthPrepassStage()
    {
        shutdown();
    }

    bool DepthPrepassStage::initialize()
    {
        if (isInitialized())
        {
            Core::warn("DepthPrepassStage already initialized");
            return true;
        }

        Core::info("Initializing DepthPrepassStage for forward+ rendering");

        if (!m_depthShader)
        {
            Core::warn("No depth shader provided for DepthPrepassStage - stage will be skipped");
        }
        else
        {
            Core::info("DepthPrepassStage: Using depth shader for Z-prepass optimization");
        }

        if (!m_renderSystem)
        {
            Core::warn("DepthPrepassStage: No RenderSystem set - voxel geometry will not be rendered");
        }

        if (!m_registry)
        {
            Core::warn("DepthPrepassStage: No Registry set - ECS entities will not be rendered");
        }

        setInitialized(true);
        return true;
    }

    void DepthPrepassStage::shutdown()
    {
        if (!isInitialized())
        {
            return;
        }

        Core::info("Shutting down DepthPrepassStage");

        m_depthShader.reset();
        m_registry.reset();
        m_renderSystem = nullptr;

        setInitialized(false);
    }

    void DepthPrepassStage::execute(const std::shared_ptr<RenderContext> context)
    {
        if (!isInitialized() || !isEnabled())
        {
            Core::warn("DepthPrepassStage: Not initialized or enabled");
            return;
        }

        PROFILE_SCOPE("DepthPrepassStage::execute");

        if (!m_depthShader)
        {
            Core::warn("DepthPrepassStage: Missing shader - skipping");
            return;
        }

        // Bind output (depth buffer)
        bindOutput();

        // Clear depth buffer only (preserve color for forward+ rendering)
        clearOutput(false, true, false);

        // Setup render state for depth-only rendering with projection-specific optimizations
        setupDepthRenderState(*context);

        // Bind depth shader
        m_depthShader->bind();

        // Set view and projection matrices
        context->bindShaderUniforms(m_depthShader);

        // Set alpha threshold if rendering alpha-tested materials
        if (m_materialFilterMode == MaterialFilterMode::AlphaTestedOnly || 
            m_materialFilterMode == MaterialFilterMode::Both)
        {
            m_depthShader->setFloat("alphaThreshold", m_alphaThreshold);
        }

        // Render ECS entities by depth only
        if (m_registry)
        {
            renderECSEntityDepth(context);
        }

        // Render non-entity geometry by depth only
        if (m_renderSystem)
        {
            renderGeometryDepth(context);
        }

        // Unbind shader
        m_depthShader->unbind();

        // Restore render state with early-z optimization if enabled
        restoreRenderState();

        // Unbind output render target
        unbindOutput();
    }

    void DepthPrepassStage::setDepthShader(std::shared_ptr<Shader> shader)
    {
        m_depthShader = shader;
    }

    std::shared_ptr<Shader> DepthPrepassStage::getDepthShader() const
    {
        return m_depthShader;
    }

    void DepthPrepassStage::setRegistry(std::shared_ptr<ECS::Registry> registry)
    {
        m_registry = registry;
    }

    void DepthPrepassStage::setRenderSystem(std::shared_ptr<RenderSystem> renderSystem)
    {
        m_renderSystem = renderSystem;
    }

    void DepthPrepassStage::setMaterialFilterMode(MaterialFilterMode mode)
    {
        m_materialFilterMode = mode;
    }

    MaterialFilterMode DepthPrepassStage::getMaterialFilterMode() const
    {
        return m_materialFilterMode;
    }

    void DepthPrepassStage::setOpaqueOnly(bool opaqueOnly)
    {
        // Legacy compatibility - convert to new material filter mode
        m_materialFilterMode = opaqueOnly ? MaterialFilterMode::OpaqueOnly : MaterialFilterMode::Both;
    }

    bool DepthPrepassStage::isOpaqueOnly() const
    {
        // Legacy compatibility - return true only if filtering to opaque only
        return m_materialFilterMode == MaterialFilterMode::OpaqueOnly;
    }

    void DepthPrepassStage::setAlphaThreshold(float threshold)
    {
        m_alphaThreshold = glm::clamp(threshold, 0.0f, 1.0f);
    }

    float DepthPrepassStage::getAlphaThreshold() const
    {
        return m_alphaThreshold;
    }

    void DepthPrepassStage::setDepthBias(float bias)
    {
        m_depthBias = bias;
    }

    float DepthPrepassStage::getDepthBias() const
    {
        return m_depthBias;
    }

    void DepthPrepassStage::setOrthographicOptimization(bool enable)
    {
        m_orthographicOptimization = enable;
    }

    bool DepthPrepassStage::isOrthographicOptimizationEnabled() const
    {
        return m_orthographicOptimization;
    }

    void DepthPrepassStage::setEarlyZRejection(bool enable)
    {
        m_earlyZRejection = enable;
    }

    bool DepthPrepassStage::isEarlyZRejectionEnabled() const
    {
        return m_earlyZRejection;
    }

    void DepthPrepassStage::renderECSEntityDepth(const std::shared_ptr<RenderContext> context)
    {
        if (!m_registry)
        {
            return;
        }

        auto view = m_registry->view<ECS::TransformComponent, ECS::MeshRendererComponent>();
        size_t renderedCount = 0;

        // Collect all renderable entities in one pass
        for (auto [entity, transform, meshRenderer] : view)
        {
            // Early rejection - skip invisible objects
            if (!meshRenderer->isVisible())
            {
                continue;
            }

            if (!meshRenderer->getMesh())
            {
                continue;
            }

            // Apply material filtering based on blend mode
            auto material = meshRenderer->getMaterial();
            if (material && !shouldRenderMaterial(material))
            {
                continue;
            }

            // Convert to renderable object and render
            RenderableObject renderable;
            renderable.mesh = meshRenderer->getMesh();
            renderable.material = meshRenderer->getMaterial();
            renderable.transform = transform->getWorldTransform().getMatrix();
            renderable.bounds = meshRenderer->getMesh()->getBounds();

            renderObjectDepth(renderable, context);
            renderedCount++;
        }

        Core::debug("DepthPrepassStage: Rendered " + std::to_string(renderedCount) + " ECS entities");
    }

    void DepthPrepassStage::renderGeometryDepth(const std::shared_ptr<RenderContext> context)
    {
        if (!m_renderSystem)
        {
            return;
        }

        size_t renderedCount = 0;

        // Render opaque voxel geometry (always included for depth prepass)
        if (m_materialFilterMode == MaterialFilterMode::OpaqueOnly || 
            m_materialFilterMode == MaterialFilterMode::Both)
        {
            const auto& opaqueQueue = m_renderSystem->getOpaqueQueue();
            
            for (const auto& renderable : opaqueQueue)
            {
                // Additional material filtering
                if (renderable.material && !shouldRenderMaterial(renderable.material))
                {
                    continue;
                }

                renderObjectDepth(renderable, context);
                renderedCount++;
            }
        }

        // Render transparent voxel geometry if alpha testing is enabled
        if (m_materialFilterMode == MaterialFilterMode::AlphaTestedOnly || 
            m_materialFilterMode == MaterialFilterMode::Both)
        {
            const auto& transparentQueue = m_renderSystem->getTransparentQueue();
            
            for (const auto& renderable : transparentQueue)
            {
                // Only render alpha-tested transparent materials, not fully transparent ones
                if (renderable.material && shouldRenderMaterial(renderable.material))
                {
                    renderObjectDepth(renderable, context);
                    renderedCount++;
                }
            }
        }

        Core::debug("DepthPrepassStage: Rendered " + std::to_string(renderedCount) + " voxel objects from render queues");
    }

    void DepthPrepassStage::renderObjectDepth(const RenderableObject& renderable,
                                              const std::shared_ptr<RenderContext> context)
    {
        if (!renderable.mesh)
        {
            return;
        }

        // Set model matrix uniform
        m_depthShader->setMat4("modelMatrix", renderable.transform);

        // Handle alpha-tested materials by binding their textures
        if (renderable.material && (m_materialFilterMode == MaterialFilterMode::AlphaTestedOnly || 
                                    m_materialFilterMode == MaterialFilterMode::Both))
        {
            // Check if this is an alpha-tested material
            if (renderable.material->getBlendMode() == BlendMode::Transparent)
            {
                // Bind albedo texture for alpha testing if available
                auto albedoTexture = renderable.material->getTexture(TextureSlot::Albedo);
                if (albedoTexture)
                {
                    albedoTexture->bind(0);
                    m_depthShader->setInt("albedoTexture", 0);
                    m_depthShader->setBool("hasAlbedoTexture", true);
                }
                else
                {
                    m_depthShader->setBool("hasAlbedoTexture", false);
                }
            }
        }

        // Render mesh
        renderable.mesh->bind();
        renderable.mesh->draw();
        renderable.mesh->unbind();
    }

    void DepthPrepassStage::setupDepthRenderState(const RenderContext& context)
    {
        // Enable depth testing and writing
        CHECK_GL_ERROR(glEnable(GL_DEPTH_TEST));
        CHECK_GL_ERROR(glDepthFunc(GL_LESS));
        CHECK_GL_ERROR(glDepthMask(GL_TRUE));

        // Disable color writing (depth-only pass)
        CHECK_GL_ERROR(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE));

        // Set depth bias if needed for shadow mapping compatibility
        if (m_depthBias > 0.0f)
        {
            CHECK_GL_ERROR(glEnable(GL_POLYGON_OFFSET_FILL));
            CHECK_GL_ERROR(glPolygonOffset(m_depthBias, m_depthBias));
        }

        // Enable back-face culling for better performance
        CHECK_GL_ERROR(glEnable(GL_CULL_FACE));
        CHECK_GL_ERROR(glCullFace(GL_BACK));

        // Apply orthographic projection optimizations if enabled
        if (m_orthographicOptimization && isOrthographicProjection(context))
        {
            // For orthographic projection, we can be more aggressive with depth precision
            // and potentially use different culling strategies for isometric views
            Core::info("DepthPrepassStage: Applying orthographic projection optimizations");
            
            // For isometric/orthographic views, we might want to disable front face culling
            // in some cases to handle edge cases with thin geometry
            if (getParameter<bool>("disableCullingForOrthographic", false))
            {
                CHECK_GL_ERROR(glDisable(GL_CULL_FACE));
            }
        }

        // Handle alpha testing for transparent materials
        if (m_materialFilterMode == MaterialFilterMode::AlphaTestedOnly || 
            m_materialFilterMode == MaterialFilterMode::Both)
        {
            // Alpha testing will be handled in the shader using discard
            // No additional OpenGL state needed for modern alpha testing
        }
    }

    void DepthPrepassStage::restoreRenderState()
    {
        // Re-enable color writing
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        // Disable polygon offset if it was enabled
        if (m_depthBias > 0.0f)
        {
            CHECK_GL_ERROR(glDisable(GL_POLYGON_OFFSET_FILL));
        }

        // Setup early-z rejection optimization for subsequent passes
        if (m_earlyZRejection)
        {
            // Keep depth testing enabled but set to EQUAL for early depth rejection
            // This allows the GPU to reject fragments that don't match the depth buffer
            CHECK_GL_ERROR(glDepthFunc(GL_EQUAL));
            CHECK_GL_ERROR(glDepthMask(GL_FALSE)); // Disable depth writing for subsequent passes
            
            Core::info("DepthPrepassStage: Early-z rejection enabled for forward+ rendering");
        }
        else
        {
            // Standard depth testing for subsequent passes
            CHECK_GL_ERROR(glDepthFunc(GL_LESS));
            CHECK_GL_ERROR(glDepthMask(GL_TRUE));
        }

        // Restore face culling to default state
        CHECK_GL_ERROR(glEnable(GL_CULL_FACE));
        CHECK_GL_ERROR(glCullFace(GL_BACK));
    }

    bool DepthPrepassStage::shouldRenderMaterial(std::shared_ptr<Material> material) const
    {
        if (!material)
        {
            // No material means render as opaque
            return m_materialFilterMode == MaterialFilterMode::OpaqueOnly || 
                   m_materialFilterMode == MaterialFilterMode::Both;
        }

        BlendMode blendMode = material->getBlendMode();

        switch (m_materialFilterMode)
        {
            case MaterialFilterMode::OpaqueOnly:
                return blendMode == BlendMode::Opaque;

            case MaterialFilterMode::AlphaTestedOnly:
                // Alpha-tested materials typically use Transparent blend mode with alpha testing
                return blendMode == BlendMode::Transparent;

            case MaterialFilterMode::Both:
                // Render both opaque and alpha-tested, but skip fully transparent materials
                return blendMode == BlendMode::Opaque || blendMode == BlendMode::Transparent;

            default:
                return false;
        }
    }

    bool DepthPrepassStage::isOrthographicProjection(const RenderContext& context) const
    {
        // Check if the projection matrix represents an orthographic projection
        // In an orthographic projection, the bottom row should be [0, 0, 0, 1]
        const glm::mat4& projMatrix = context.getProjectionMatrix();
        
        // Check the w-component transformation elements
        // For orthographic: m[3][0] = m[3][1] = m[3][2] = 0, m[3][3] = 1
        // For perspective: m[3][2] = -1, others are 0
        const float epsilon = 1e-6f;
        
        bool isOrtho = (std::abs(projMatrix[3][0]) < epsilon) &&
                       (std::abs(projMatrix[3][1]) < epsilon) &&
                       (std::abs(projMatrix[3][2]) < epsilon) &&
                       (std::abs(projMatrix[3][3] - 1.0f) < epsilon);

        return isOrtho;
    }

} // namespace PixelCraft::Rendering::Stages
