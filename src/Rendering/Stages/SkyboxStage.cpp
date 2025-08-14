// -------------------------------------------------------------------------
// SkyboxStage.cpp
// -------------------------------------------------------------------------
#include "Rendering/Stages/SkyboxStage.h"
#include "Rendering/Shader.h"
#include "Rendering/RenderContext.h"
#include "Rendering/Texture.h"
#include "Rendering/Mesh.h"
#include "Core/Application.h"
#include "Core/Logger.h"
#include "Utility/Profiler.h"
#include "Utility/Color.h"

namespace PixelCraft::Rendering::Stages
{

    SkyboxStage::SkyboxStage()
        : RenderStage("SkyboxStage")
    {
    }

    SkyboxStage::~SkyboxStage()
    {
        if (isInitialized())
        {
            shutdown();
        }
    }

    bool SkyboxStage::initialize()
    {
        if (isInitialized())
        {
            return true;
        }

        Core::info("Initializing SkyboxStage");

        // Create skybox mesh
        createSkyboxMesh();

        setInitialized(true);
        return true;
    }

    void SkyboxStage::shutdown()
    {
        if (!isInitialized())
        {
            return;
        }

        Core::info("Shutting down SkyboxStage");

        m_skyboxShader.reset();
        m_cubemapTexture.reset();
        m_equirectangularTexture.reset();
        m_skyboxMesh.reset();

        setInitialized(false);
    }

    void SkyboxStage::execute(const std::shared_ptr<RenderContext> context)
    {
        if (!isInitialized() || !isEnabled())
        {
            Core::warn("SkyboxStage: Not initialized or enabled");
            return;
        }

        PROFILE_SCOPE("SkyboxStage::execute");

        if (!m_skyboxShader || !m_skyboxMesh)
        {
            Core::warn("SkyboxStage: Missing skybox shader or mesh");
            return;
        }

        // Check if we have appropriate texture for current mode
        if (m_mode == SkyboxMode::Cubemap && !m_cubemapTexture)
        {
            return;
        }
        if (m_mode == SkyboxMode::Equirectangular && !m_equirectangularTexture)
        {
            return;
        }

        // Bind output
        bindOutput();

        // Setup render state
        setupSkyboxRenderState();

        // Update rotation if enabled
        if (m_rotationEnabled)
        {
            m_currentRotation += m_rotationSpeed * Core::Application::getInstance()->getDeltaTime();
        }

        // Render skybox
        renderSkybox(context);

        // Restore render state
        restoreSkyboxRenderState();
    }

    void SkyboxStage::setSkyboxMode(SkyboxMode mode)
    {
        m_mode = mode;
    }

    SkyboxStage::SkyboxMode SkyboxStage::getSkyboxMode() const
    {
        return m_mode;
    }

    void SkyboxStage::setSkyboxShader(std::shared_ptr<Shader> shader)
    {
        m_skyboxShader = shader;
    }

    std::shared_ptr<Shader> SkyboxStage::getSkyboxShader() const
    {
        return m_skyboxShader;
    }

    void SkyboxStage::setCubemapTexture(std::shared_ptr<Texture> cubemap)
    {
        m_cubemapTexture = cubemap;
    }

    void SkyboxStage::setEquirectangularTexture(std::shared_ptr<Texture> texture)
    {
        m_equirectangularTexture = texture;
    }

    void SkyboxStage::setGradientColors(const Utility::Color& topColor, const Utility::Color& bottomColor)
    {
        m_gradientTopColor = topColor;
        m_gradientBottomColor = bottomColor;
    }

    void SkyboxStage::setSunDirection(const glm::vec3& direction)
    {
        m_sunDirection = glm::normalize(direction);
    }

    void SkyboxStage::setSunProperties(const Utility::Color& color, float intensity)
    {
        m_sunColor = color;
        m_sunIntensity = intensity;
    }

    void SkyboxStage::setAtmosphereProperties(float rayleighScattering, float mieScattering, float atmosphereHeight)
    {
        m_rayleighScattering = rayleighScattering;
        m_mieScattering = mieScattering;
        m_atmosphereHeight = atmosphereHeight;
    }

    void SkyboxStage::setExposure(float exposure)
    {
        m_exposure = exposure;
    }

    float SkyboxStage::getExposure() const
    {
        return m_exposure;
    }

    void SkyboxStage::setRotation(bool enabled, float speed)
    {
        m_rotationEnabled = enabled;
        m_rotationSpeed = speed;
    }

    void SkyboxStage::renderSkybox(const std::shared_ptr<RenderContext> context)
    {
        m_skyboxShader->bind();

        // Remove translation from view matrix
        glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(context->getViewMatrix()));

        // Apply rotation if enabled
        if (m_rotationEnabled)
        {
            viewNoTranslation = glm::rotate(viewNoTranslation, m_currentRotation, glm::vec3(0.0f, 1.0f, 0.0f));
        }

        // Set common uniforms
        m_skyboxShader->setMat4("viewMatrix", viewNoTranslation);
        m_skyboxShader->setMat4("projMatrix", context->getProjectionMatrix());
        m_skyboxShader->setFloat("exposure", m_exposure);

        // Set mode-specific uniforms and textures
        switch (m_mode)
        {
            case SkyboxMode::Cubemap:
                m_skyboxShader->setInt("skyboxTexture", 0);
                m_cubemapTexture->bind(0);
                break;

            case SkyboxMode::Equirectangular:
                m_skyboxShader->setInt("equirectangularMap", 0);
                m_equirectangularTexture->bind(0);
                break;

            case SkyboxMode::Gradient:
                m_skyboxShader->setUniform("gradientTop", m_gradientTopColor);
                m_skyboxShader->setUniform("gradientBottom", m_gradientBottomColor);
                break;

            case SkyboxMode::Procedural:
                m_skyboxShader->setVec3("sunDirection", m_sunDirection);
                m_skyboxShader->setUniform("sunColor", m_sunColor);
                m_skyboxShader->setFloat("sunIntensity", m_sunIntensity);
                m_skyboxShader->setFloat("rayleighScattering", m_rayleighScattering);
                m_skyboxShader->setFloat("mieScattering", m_mieScattering);
                m_skyboxShader->setFloat("atmosphereHeight", m_atmosphereHeight);
                m_skyboxShader->setVec3("cameraPosition", context->getCameraPosition());
                break;
        }

        // Render skybox cube
        m_skyboxMesh->bind();
        m_skyboxMesh->draw();
        m_skyboxMesh->unbind();

        m_skyboxShader->unbind();
    }

    void SkyboxStage::createSkyboxMesh()
    {
        // Skybox vertices
        float vertices[] = {
            // Back face
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,
            // Front face
            -1.0f, -1.0f, 1.0f,
            1.0f, -1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f
        };

        // Skybox indices
        uint32_t indices[] = {
            // Back face
            0, 1, 2, 2, 3, 0,
            // Front face
            4, 5, 6, 6, 7, 4,
            // Left face
            7, 3, 0, 0, 4, 7,
            // Right face
            1, 5, 6, 6, 2, 1,
            // Bottom face
            0, 1, 5, 5, 4, 0,
            // Top face
            3, 2, 6, 6, 7, 3
        };

        // Create mesh
        m_skyboxMesh = std::make_shared<Mesh>("SkyboxMesh");

        std::vector<VertexAttribute> attributes;

        Rendering::VertexAttribute posAttr;
        posAttr.semantic = Rendering::VertexAttributeSemantic::Position;
        posAttr.location = 0;
        posAttr.size = 3;
        posAttr.type = 0x1406; // GL_FLOAT
        posAttr.normalized = false;
        posAttr.stride = sizeof(float);
        posAttr.offset = sizeof(float);
        posAttr.name = "position";
        attributes.push_back(posAttr);

        attributes.push_back(posAttr);

        m_skyboxMesh->createFromData(vertices, 8, sizeof(float), attributes, indices, 36);
    }

    void SkyboxStage::setupSkyboxRenderState()
    {
        // Depth test at far plane
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);

        // No face culling for skybox
        glDisable(GL_CULL_FACE);

        // No blending
        glDisable(GL_BLEND);
    }

    void SkyboxStage::restoreSkyboxRenderState()
    {
        // Restore defaults
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
    }

} // namespace PixelCraft::Rendering::Stages