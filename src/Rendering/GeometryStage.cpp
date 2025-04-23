// -------------------------------------------------------------------------
// GeometryStage.cpp
// -------------------------------------------------------------------------
#include "Rendering/GeometryStage.h"
#include "Core/Logger.h"
#include "Core/Profiler.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MaterialComponent.h"
#include "Rendering/Mesh.h"
#include "Rendering/Material.h"
#include "glm/glm.hpp"

// OpenGL headers
#include <glad/glad.h>
#include <algorithm>
#include <gtx/norm.hpp>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Rendering
{

    GeometryStage::GeometryStage()
        : RenderStage("GeometryStage")
        , m_opaqueOnly(true)
        , m_transparentOnly(false)
        , m_depthTest(true)
        , m_depthWrite(true)
        , m_useInstancing(true)
        , m_sortObjects(true)
        , m_profilingMarker(0)
    {
    }

    GeometryStage::~GeometryStage()
    {
        if (m_shader || m_registry)
        {
            shutdown();
        }
    }

    bool GeometryStage::initialize()
    {
        Log::info("Initializing GeometryStage");

        // Set default parameters
        setParameter("opaqueOnly", m_opaqueOnly);
        setParameter("transparentOnly", m_transparentOnly);
        setParameter("depthTest", m_depthTest);
        setParameter("depthWrite", m_depthWrite);
        setParameter("useInstancing", m_useInstancing);
        setParameter("sortObjects", m_sortObjects);

        // Register profiling marker
        //m_profilingMarker = Core::Profiler::registerMarker("GeometryStage");

        return true;
    }

    void GeometryStage::shutdown()
    {
        Log::info("Shutting down GeometryStage");

        m_shader = nullptr;
        m_registry = nullptr;
    }

    void GeometryStage::execute(const RenderContext& context)
    {
        if (!m_enabled)
        {
            return;
        }

        // Start profiling
        //Core::Profiler::beginMarker(m_profilingMarker);

        if (!m_shader)
        {
            Log::error("GeometryStage::execute - No shader set");
            //Core::Profiler::endMarker(m_profilingMarker);
            return;
        }

        if (!m_registry)
        {
            Log::error("GeometryStage::execute - No registry set");
            //Core::Profiler::endMarker(m_profilingMarker);
            return;
        }

        // Update parameters from the parameter map
        m_opaqueOnly = getParameter("opaqueOnly", m_opaqueOnly);
        m_transparentOnly = getParameter("transparentOnly", m_transparentOnly);
        m_depthTest = getParameter("depthTest", m_depthTest);
        m_depthWrite = getParameter("depthWrite", m_depthWrite);
        m_useInstancing = getParameter("useInstancing", m_useInstancing);
        m_sortObjects = getParameter("sortObjects", m_sortObjects);

        // Bind output render target
        bindOutput();

        // Set viewport
        resetViewport();

        // Setup render state
        setupRenderState();

        // Bind shader
        m_shader->bind();

        // Set common shader uniforms from context
        m_shader->setUniform("viewMatrix", context.getViewMatrix());
        m_shader->setUniform("projMatrix", context.getProjectionMatrix());
        m_shader->setUniform("viewProjMatrix", context.getViewProjectionMatrix());
        m_shader->setUniform("cameraPosition", context.getCameraPosition());

        // Pass custom parameters to shader
        for (const auto& [name, param] : m_parameters)
        {
            struct ShaderParamVisitor
            {
                Shader* shader;
                const std::string& name;

                void operator()(int value)
                {
                    shader->setUniform(name, value);
                }
                void operator()(float value)
                {
                    shader->setUniform(name, value);
                }
                void operator()(bool value)
                {
                    shader->setUniform(name, value ? 1 : 0);
                }
                void operator()(const glm::vec2& value)
                {
                    shader->setUniform(name, value);
                }
                void operator()(const glm::vec3& value)
                {
                    shader->setUniform(name, value);
                }
                void operator()(const glm::vec4& value)
                {
                    shader->setUniform(name, value);
                }
                void operator()(const glm::mat4& value)
                {
                    shader->setUniform(name, value);
                }
            };

            ShaderParamVisitor visitor{m_shader.get(), name};
            std::visit(visitor, param.value);
        }

        // Get all entities with mesh, transform, and material components
        auto meshEntities = m_registry->view<ECS::MeshComponent, ECS::TransformComponent, ECS::MaterialComponent>();

        // Copy to vector for sorting
        std::vector<ECS::EntityID> renderQueue(meshEntities.begin(), meshEntities.end());

        // Sort entities if needed
        if (m_sortObjects)
        {
            if (m_transparentOnly)
            {
                // Sort transparent objects back-to-front
                std::sort(renderQueue.begin(), renderQueue.end(),
                          [this, &context](ECS::EntityID a, ECS::EntityID b) {
                              const auto& transformA = m_registry->getComponent<ECS::TransformComponent>(a);
                              const auto& transformB = m_registry->getComponent<ECS::TransformComponent>(b);

                              float distA = glm::length2(transformA.getWorldPosition() - context.getCameraPosition());
                              float distB = glm::length2(transformB.getWorldPosition() - context.getCameraPosition());

                              // Sort back-to-front (furthest first)
                              return distA > distB;
                          });
            }
            else
            {
                // Sort opaque objects by material to minimize state changes
                std::sort(renderQueue.begin(), renderQueue.end(),
                          [this](ECS::EntityID a, ECS::EntityID b) {
                              const auto& materialA = m_registry->getComponent<ECS::MaterialComponent>(a);
                              const auto& materialB = m_registry->getComponent<ECS::MaterialComponent>(b);

                              // Sort by material ID to batch similar materials
                              return materialA.material->getID() < materialB.material->getID();
                          });
            }
        }

        // Render each entity
        for (ECS::EntityID entity : renderQueue)
        {
            renderObject(context, entity);
        }

        // Unbind shader
        m_shader->unbind();

        // Restore render state
        restoreRenderState();

        // End profiling
        //Core::Profiler::endMarker(m_profilingMarker);
    }

    void GeometryStage::setShader(std::shared_ptr<Shader> shader)
    {
        m_shader = shader;
    }

    std::shared_ptr<Shader> GeometryStage::getShader() const
    {
        return m_shader;
    }

    void GeometryStage::setRegistry(std::shared_ptr<ECS::Registry> registry)
    {
        m_registry = registry;
    }

    void GeometryStage::renderObject(const RenderContext& context, ECS::EntityID entity)
    {
        // Get components
        auto& meshComp = m_registry->getComponent<ECS::MeshComponent>(entity);
        auto& transformComp = m_registry->getComponent<ECS::TransformComponent>(entity);
        auto& materialComp = m_registry->getComponent<ECS::MaterialComponent>(entity);

        // Skip if mesh or material is not valid
        if (!meshComp.mesh || !materialComp.material)
        {
            return;
        }

        // Skip based on transparency settings
        if (m_opaqueOnly && materialComp.material->isTransparent())
        {
            return;
        }

        if (m_transparentOnly && !materialComp.material->isTransparent())
        {
            return;
        }

        // Frustum culling - skip if object is outside view frustum
        if (meshComp.mesh->getAABB().isValid())
        {
            auto worldAABB = meshComp.mesh->getAABB().transform(transformComp.getWorldMatrix());
            if (!context.getFrustum().testAABB(worldAABB.getMin(), worldAABB.getMax()))
            {
                return;
            }
        }

        // Get world transform
        glm::mat4 modelMatrix = transformComp.getWorldMatrix();

        // Set model matrix
        m_shader->setUniform("modelMatrix", modelMatrix);

        // Normal matrix (inverse transpose of the model matrix)
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
        m_shader->setUniform("normalMatrix", normalMatrix);

        // Bind material and its textures
        materialComp.material->bind(m_shader);

        // Render mesh
        meshComp.mesh->bind();

        // Use instancing if available and enabled
        if (m_useInstancing && meshComp.instanceCount > 1 && meshComp.instanceBuffer)
        {
            meshComp.mesh->drawInstanced(meshComp.instanceCount);
        }
        else
        {
            meshComp.mesh->draw();
        }

        meshComp.mesh->unbind();

        // Unbind material
        materialComp.material->unbind();
    }

    void GeometryStage::setupRenderState()
    {
        // Configure depth testing
        if (m_depthTest)
        {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }

        // Configure depth writing
        glDepthMask(m_depthWrite ? GL_TRUE : GL_FALSE);

        // Configure culling (always enable for opaque, configurable for transparent)
        if (m_transparentOnly)
        {
            bool cullBackfaces = getParameter("cullBackfaces", false);
            if (cullBackfaces)
            {
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
            }
            else
            {
                glDisable(GL_CULL_FACE);
            }
        }
        else
        {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        }

        // Configure blending based on render mode
        if (m_transparentOnly)
        {
            glEnable(GL_BLEND);

            // Get blend mode from parameters or use default alpha blending
            int blendMode = getParameter("blendMode", 0);
            switch (blendMode)
            {
                case 0: // Alpha blending
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    break;
                case 1: // Additive
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                    break;
                case 2: // Multiplicative
                    glBlendFunc(GL_DST_COLOR, GL_ZERO);
                    break;
                case 3: // Premultiplied alpha
                    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                    break;
                default:
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    break;
            }
        }
        else
        {
            glDisable(GL_BLEND);
        }
    }

    void GeometryStage::restoreRenderState()
    {
        // Restore default states
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glDisable(GL_BLEND);
    }

} // namespace PixelCraft::Rendering