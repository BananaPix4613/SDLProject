// -------------------------------------------------------------------------
// PostProcessStage.cpp
// -------------------------------------------------------------------------
#include "Rendering/Stages/PostProcessStage.h"
#include "Core/Logger.h"
#include "Rendering/Shader.h"
#include "Rendering/RenderContext.h"
#include "Rendering/RenderTarget.h"
#include "Rendering/Mesh.h"
#include "Rendering/PaletteManager.h"
#include "Utility/Profiler.h"

namespace PixelCraft::Rendering::Stages
{

    PostProcessStage::PostProcessStage()
        : RenderStage("PostProcessStage")
        , m_enabledEffects(0)
        , m_paletteMappingShader(nullptr)
        , m_ditheringShader(nullptr)
        , m_toneMappingShader(nullptr)
        , m_bloomShader(nullptr)
        , m_fxaaShader(nullptr)
        , m_pixelPerfectShader(nullptr)
        , m_fullscreenQuad(nullptr)
        , m_paletteManager(nullptr)
        , m_ditheringStrength(0.5f)
        , m_bloomThreshold(1.0f)
        , m_bloomIntensity(0.8f)
        , m_exposure(1.0f)
        , m_pixelScale(1)
        , m_tempTarget1(nullptr)
        , m_tempTarget2(nullptr)
        , m_bloomTarget(nullptr)
        , m_profilingMarker("PostProcessStage")
    {
    }

    PostProcessStage::~PostProcessStage()
    {
        shutdown();
    }

    bool PostProcessStage::initialize()
    {
        if (isInitialized())
        {
            Core::warn("PostProcessStage already initialized");
            return true;
        }

        Core::info("Initializing PostProcessStage");

        // Create fullscreen quad
        m_fullscreenQuad = std::make_shared<Mesh>("FullscreenQuad");

        // Create fullscreen quad geometry
        // Each vertex: position(x,y) + texcoord(u,v) = 4 floats
        float quadVertices[] = {
            -1.0f, -1.0f, 0.0f, 0.0f,  // bottom-left
             1.0f, -1.0f, 1.0f, 0.0f,  // bottom-right  
             1.0f,  1.0f, 1.0f, 1.0f,  // top-right
            -1.0f,  1.0f, 0.0f, 1.0f   // top-left
        };

        uint32_t quadIndices[] = {
            0, 1, 2,
            0, 2, 3
        };

        // Set up vertex attributes correctly
        std::vector<VertexAttribute> attributes;

        VertexAttribute posAttr;
        posAttr.semantic = VertexAttributeSemantic::Position;
        posAttr.location = 0;
        posAttr.size = 2;                      // 2 components (x, y)
        posAttr.type = 0x1406;                 // GL_FLOAT
        posAttr.normalized = false;
        posAttr.stride = 4 * sizeof(float);    // Total vertex size
        posAttr.offset = 0;                    // Position starts at beginning

        VertexAttribute texAttr;
        texAttr.semantic = VertexAttributeSemantic::TexCoord0;
        texAttr.location = 1;
        texAttr.size = 2;                      // 2 components (u, v)
        texAttr.type = 0x1406;                 // GL_FLOAT
        texAttr.normalized = false;
        texAttr.stride = 4 * sizeof(float);    // Total vertex size
        texAttr.offset = 2 * sizeof(float);    // TexCoord starts after position

        attributes.push_back(posAttr);
        attributes.push_back(texAttr);

        // Create the mesh with correct data
        if (!m_fullscreenQuad->createFromData(quadVertices, 4, 4 * sizeof(float), attributes, quadIndices, 6))
        {
            Core::error("PostProcessStage: Failed to create fullscreen quad mesh");
            return false;
        }

        // Create intermediate render targets
        createIntermediateTargets();

        Core::info("PostProcessStage initialized successfully");
        setInitialized(true);
        return true;
    }

    void PostProcessStage::shutdown()
    {
        if (!isInitialized())
        {
            return;
        }

        Core::info("Shutting down PostProcessStage");

        m_paletteMappingShader.reset();
        m_ditheringShader.reset();
        m_toneMappingShader.reset();
        m_bloomShader.reset();
        m_fxaaShader.reset();
        m_pixelPerfectShader.reset();
        m_fullscreenQuad.reset();
        m_paletteManager.reset();
        m_tempTarget1.reset();
        m_tempTarget2.reset();
        m_bloomTarget.reset();

        setInitialized(false);
    }

    void PostProcessStage::execute(const std::shared_ptr<RenderContext> context)
    {
        if (!isInitialized() || !isEnabled())
        {
            Core::warn("PostProcessStage: Not initialized or enabled");
            return;
        }

        PROFILE_SCOPE("PostProcessStage::execute");

        // Start with input texture
        auto currentInput = getInput(0);
        if (!currentInput)
        {
            Core::warn("PostProcessStage: No input texture - skipping post-processing");
            return;
        }

        // If no effects are enabled, just pass through the input
        if (m_enabledEffects == 0)
        {
            Core::debug("PostProcessStage: No effects enabled, passing through input");
            
            // Handle passthrough case
            bindOutput();
            blitFromInput(currentInput);
            unbindOutput();
            return;
        }

        Core::debug("PostProcessStage: Processing with enabled effects");

        // Resize intermediate targets to match input if needed
        ensureIntermediateTargetsSize(currentInput->getWidth(), currentInput->getHeight());

        // Effect chaining - each effect reads from previous effect's output
        std::shared_ptr<RenderTarget> currentOutput = m_tempTarget1;
        bool isFirstEffect = true;

        // Apply effects in order, properly chaining them
        if (isEffectEnabled(EffectType::Bloom))
        {
            auto input = isFirstEffect ? currentInput : currentInput;
            applyBloom(input, m_bloomTarget);
            // Note: Bloom result stored separately for later additive blending
            isFirstEffect = false;
        }

        if (isEffectEnabled(EffectType::ToneMapping))
        {
            auto input = isFirstEffect ? currentInput : currentInput;
            applyToneMapping(input, currentOutput);

            // Swap buffers for next effect
            std::swap(currentInput, currentOutput);
            currentOutput = (currentOutput == m_tempTarget1) ? m_tempTarget2 : m_tempTarget1;
            isFirstEffect = false;
        }

        if (isEffectEnabled(EffectType::PaletteMapping))
        {
            auto input = isFirstEffect ? getInput(0) : currentInput;
            applyPaletteMapping(input, currentOutput);

            // Swap buffers for next effect
            std::swap(currentInput, currentOutput);
            currentOutput = (currentOutput == m_tempTarget1) ? m_tempTarget2 : m_tempTarget1;
            isFirstEffect = false;
        }

        if (isEffectEnabled(EffectType::Dithering))
        {
            auto input = isFirstEffect ? getInput(0) : currentInput;
            applyDithering(input, currentOutput);

            // Swap buffers for next effect
            std::swap(currentInput, currentOutput);
            currentOutput = (currentOutput == m_tempTarget1) ? m_tempTarget2 : m_tempTarget1;
            isFirstEffect = false;
        }

        if (isEffectEnabled(EffectType::FXAA))
        {
            auto input = isFirstEffect ? getInput(0) : currentInput;
            applyFXAA(input, currentOutput);

            // Swap buffers for next effect
            std::swap(currentInput, currentOutput);
            currentOutput = (currentOutput == m_tempTarget1) ? m_tempTarget2 : m_tempTarget1;
            isFirstEffect = false;
        }

        // Final output handling with proper binding
        bindOutput();

        if (isEffectEnabled(EffectType::PixelPerfect))
        {
            auto input = isFirstEffect ? getInput(0) : currentInput;
            // PixelPerfect is applied directly to the bound output
            applyPixelPerfectToOutput(input);
        }
        else
        {
            // Copy final result to output
            auto finalResult = isFirstEffect ? getInput(0) : currentInput;
            blitFromInput(finalResult);
        }

        // Unbind output render target
        unbindOutput();
    }

    void PostProcessStage::enableEffect(EffectType effect)
    {
        m_enabledEffects |= (1 << static_cast<uint32_t>(effect));
    }

    void PostProcessStage::disableEffect(EffectType effect)
    {
        m_enabledEffects &= ~(1 << static_cast<uint32_t>(effect));
    }

    bool PostProcessStage::isEffectEnabled(EffectType effect) const
    {
        return (m_enabledEffects & (1 << static_cast<uint32_t>(effect))) != 0;
    }

    void PostProcessStage::setPaletteManager(std::shared_ptr<PaletteManager> paletteManager)
    {
        m_paletteManager = paletteManager;
    }

    void PostProcessStage::setPaletteMappingShader(std::shared_ptr<Shader> shader)
    {
        m_paletteMappingShader = shader;
        if (shader)
        {
            Core::debug("PostProcessStage: Set palette mapping shader");
        }
    }

    void PostProcessStage::setDitheringShader(std::shared_ptr<Shader> shader)
    {
        m_ditheringShader = shader;
        if (shader)
        {
            Core::debug("PostProcessStage: Set dithering shader");
        }
    }

    void PostProcessStage::setToneMappingShader(std::shared_ptr<Shader> shader)
    {
        m_toneMappingShader = shader;
        if (shader)
        {
            Core::debug("PostProcessStage: Set tone mapping shader");
        }
    }

    void PostProcessStage::setBloomShader(std::shared_ptr<Shader> shader)
    {
        m_bloomShader = shader;
        if (shader)
        {
            Core::debug("PostProcessStage: Set bloom shader");
        }
    }

    void PostProcessStage::setFXAAShader(std::shared_ptr<Shader> shader)
    {
        m_fxaaShader = shader;
        if (shader)
        {
            Core::debug("PostProcessStage: Set FXAA shader");
        }
    }

    void PostProcessStage::setPixelPerfectShader(std::shared_ptr<Shader> shader)
    {
        m_pixelPerfectShader = shader;
        if (shader)
        {
            Core::debug("PostProcessStage: Set pixel perfect shader");
        }
    }

    void PostProcessStage::setDitheringStrength(float strength)
    {
        m_ditheringStrength = std::clamp(strength, 0.0f, 1.0f);
    }

    float PostProcessStage::getDitheringStrength() const
    {
        return m_ditheringStrength;
    }

    void PostProcessStage::setBloomThreshold(float threshold)
    {
        m_bloomThreshold = std::max(0.0f, threshold);
    }

    float PostProcessStage::getBloomThreshold() const
    {
        return m_bloomThreshold;
    }

    void PostProcessStage::setBloomIntensity(float intensity)
    {
        m_bloomIntensity = std::max(0.0f, intensity);
    }

    float PostProcessStage::getBloomIntensity() const
    {
        return m_bloomIntensity;
    }

    void PostProcessStage::setExposure(float exposure)
    {
        m_exposure = std::max(0.0f, exposure);
    }

    float PostProcessStage::getExposure() const
    {
        return m_exposure;
    }

    void PostProcessStage::setPixelScale(int scale)
    {
        m_pixelScale = std::max(1, scale);
    }

    int PostProcessStage::getPixelScale() const
    {
        return m_pixelScale;
    }

    void PostProcessStage::applyPaletteMapping(std::shared_ptr<RenderTarget> input, std::shared_ptr<RenderTarget> output)
    {
        if (!m_paletteMappingShader || !m_paletteManager)
        {
            Core::warn("PostProcessStage::applyPaletteMapping: Missing shader or palette manager");
            return;
        }

        PROFILE_SCOPE("PostProcessStage::applyPaletteMapping");

        output->bind();
        m_paletteMappingShader->bind();

        // Bind input texture
        if (input)
        {
            input->getColorTexture(0)->bind(0);
            m_paletteMappingShader->setInt("inputTexture", 0);
        }

        // Bind palette textures
        m_paletteManager->bindPaletteTextures(m_paletteMappingShader);

        renderFullscreenQuad();

        m_paletteMappingShader->unbind();
        output->unbind();
    }

    void PostProcessStage::applyDithering(std::shared_ptr<RenderTarget> input, std::shared_ptr<RenderTarget> output)
    {
        if (!m_ditheringShader)
        {
            Core::warn("PostProcessStage::applyDithering: Missing dithering shader");
            return;
        }

        PROFILE_SCOPE("PostProcessStage::applyDithering");

        output->bind();
        m_ditheringShader->bind();

        // Bind input texture
        if (input)
        {
            input->getColorTexture(0)->bind(0);
            m_ditheringShader->setInt("inputTexture", 0);
        }

        m_ditheringShader->setFloat("ditheringStrength", m_ditheringStrength);

        renderFullscreenQuad();

        m_ditheringShader->unbind();
        output->unbind();
    }

    void PostProcessStage::applyToneMapping(std::shared_ptr<RenderTarget> input, std::shared_ptr<RenderTarget> output)
    {
        if (!m_toneMappingShader)
        {
            Core::warn("PostProcessStage::applyToneMapping: Missing tone mapping shader");
            return;
        }

        PROFILE_SCOPE("PostProcessStage::applyToneMapping");

        output->bind();
        m_toneMappingShader->bind();

        if (input)
        {
            input->getColorTexture(0)->bind(0);
            m_toneMappingShader->setInt("inputTexture", 0);
        }

        m_toneMappingShader->setFloat("exposure", m_exposure);

        renderFullscreenQuad();

        m_toneMappingShader->unbind();
        output->unbind();
    }

    void PostProcessStage::applyBloom(std::shared_ptr<RenderTarget> input, std::shared_ptr<RenderTarget> output)
    {
        if (!m_bloomShader || !output)
        {
            Core::warn("PostProcessStage::applyBloom: Missing bloom shader or target");
            return;
        }

        PROFILE_SCOPE("PostProcessStage::applyBloom");

        // Extract bright pixels
        output->bind();
        m_bloomShader->bind();

        if (input)
        {
            input->getColorTexture(0)->bind(0);
            m_bloomShader->setInt("inputTexture", 0);
        }

        m_bloomShader->setFloat("threshold", m_bloomThreshold);
        m_bloomShader->setFloat("intensity", m_bloomIntensity);

        renderFullscreenQuad();

        m_bloomShader->unbind();
        output->unbind();
    }

    void PostProcessStage::applyFXAA(std::shared_ptr<RenderTarget> input, std::shared_ptr<RenderTarget> output)
    {
        if (!m_fxaaShader)
        {
            Core::warn("PostProcessStage::applyFXAA: Missing FXAA shader");
            return;
        }

        PROFILE_SCOPE("PostProcessStage::applyFXAA");

        output->bind();
        m_fxaaShader->bind();

        if (input)
        {
            input->getColorTexture(0)->bind(0);
            m_fxaaShader->setInt("inputTexture", 0);
        }

        renderFullscreenQuad();

        m_fxaaShader->unbind();
        output->unbind();
    }

    void PostProcessStage::applyPixelPerfectToOutput(std::shared_ptr<RenderTarget> input)
    {
        if (!m_pixelPerfectShader || !input)
        {
            Core::warn("PostProcessStage::applyPixelPerfectToOutput: Missing shader or input");
            blitFromInput(input); // Fallback to simple copy
            return;
        }

        PROFILE_SCOPE("PostProcessStage::applyPixelPerfectToOutput");

        // The output is already bound from bindOutput()
        glClear(GL_COLOR_BUFFER_BIT);

        m_pixelPerfectShader->bind();

        input->getColorTexture(0)->bind(0);
        m_pixelPerfectShader->setInt("inputTexture", 0);
        m_pixelPerfectShader->setInt("pixelScale", m_pixelScale);

        renderFullscreenQuad();

        m_pixelPerfectShader->unbind();
    }

    void PostProcessStage::renderFullscreenQuad()
    {
        if (!m_fullscreenQuad)
        {
            Core::error("PostProcessStage::renderFullscreenQuad: No fullscreen quad mesh available");
            return;
        }

        // Save current OpenGL state
        GLboolean depthTestEnabled;
        GLboolean cullFaceEnabled;
        glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
        glGetBooleanv(GL_CULL_FACE, &cullFaceEnabled);

        // Disable depth testing and culling for fullscreen quad
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        m_fullscreenQuad->bind();
        m_fullscreenQuad->draw();
        m_fullscreenQuad->unbind();

        // Restore previous OpenGL state
        if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
        if (cullFaceEnabled) glEnable(GL_CULL_FACE);
    }

    void PostProcessStage::blitFromInput(std::shared_ptr<RenderTarget> source)
    {
        if (!source)
        {
            Core::error("PostProcessStage::blitFromInput: No source target");
            return;
        }

        PROFILE_SCOPE("PostProcessStage::blitFromInput");

        // Use a simple blit shader or OpenGL blit
        // For now, implement as simple fullscreen textured quad

        // Disable depth testing for final blit
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        // Clear the currently bound output
        glClear(GL_COLOR_BUFFER_BIT);

        // Bind source texture
        source->getColorTexture(0)->bind(0);

        // Use a simple copy shader or immediate mode rendering
        // For simplicity, create a basic copy operation

        // Create a simple passthrough shader if we don't have one
        if (!m_copyShader)
        {
            // This would be created during initialization in a real implementation
            Core::warn("PostProcessStage: No copy shader available, using blit operation");

            // Fallback to OpenGL blit
            glBindFramebuffer(GL_READ_FRAMEBUFFER, source->getFramebufferID());
            // GL_DRAW_FRAMEBUFFER is already bound from bindOutput()

            glBlitFramebuffer(
                0, 0, source->getWidth(), source->getHeight(),
                0, 0, source->getWidth(), source->getHeight(), // Assume same size
                GL_COLOR_BUFFER_BIT,
                GL_NEAREST
            );

            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        }
        else
        {
            // Use proper shader-based copy
            m_copyShader->bind();
            m_copyShader->setInt("inputTexture", 0);
            renderFullscreenQuad();
            m_copyShader->unbind();
        }

        // Restore depth testing
        glEnable(GL_DEPTH_TEST);
    }

    void PostProcessStage::createIntermediateTargets()
    {
        Core::info("Creating intermediate render targets for post-processing");
        
        // These will be resized as needed based on input
        m_tempTarget1 = std::make_shared<RenderTarget>(1920, 1080);
        if (!m_tempTarget1->initialize())
        {
            Core::error("Failed to initialize temp target 1");
            return;
        }
        m_tempTarget1->addColorAttachment(TextureFormat::RGBA8);

        m_tempTarget2 = std::make_shared<RenderTarget>(1920, 1080);
        if (!m_tempTarget2->initialize())
        {
            Core::error("Failed to initialize temp target 2");
            return;
        }
        m_tempTarget2->addColorAttachment(TextureFormat::RGBA8);

        m_bloomTarget = std::make_shared<RenderTarget>(960, 540); // Half resolution for bloom
        if (!m_bloomTarget->initialize())
        {
            Core::error("Failed to initialize bloom target");
            return;
        }
        m_bloomTarget->addColorAttachment(TextureFormat::RGBA16F);
        
        Core::info("Successfully created intermediate render targets");
    }

    void PostProcessStage::ensureIntermediateTargetsSize(int width, int height)
    {
        // Check if temp targets need resizing
        if (m_tempTarget1 && (m_tempTarget1->getWidth() != width || m_tempTarget1->getHeight() != height))
        {
            Core::debug("PostProcessStage: Resizing intermediate targets to " +
                        std::to_string(width) + "x" + std::to_string(height));
            
            m_tempTarget1->resize(width, height);
            m_tempTarget2->resize(width, height);

            // Bloom target can be half resolution for performance
            m_bloomTarget->resize(width / 2, height / 2);
        }
    }

} // namespace PixelCraft::Rendering::Stages
