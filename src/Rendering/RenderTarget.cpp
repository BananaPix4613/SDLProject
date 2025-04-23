// -------------------------------------------------------------------------
// RenderTarget.cpp
// -------------------------------------------------------------------------
#include "Rendering/RenderTarget.h"
#include "Core/Logger.h"

// OpenGL headers
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Rendering
{

    RenderTarget::RenderTarget(int width, int height, bool multisampled)
        : m_width(width)
        , m_height(height)
        , m_multisampled(multisampled)
        , m_multisampleCount(4) // Default multisample count
        , m_initialized(false)
        , m_framebufferID(0)
        , m_resolveFramebufferID(0)
    {
    }

    RenderTarget::~RenderTarget()
    {
        if (m_initialized)
        {
            shutdown();
        }
    }

    bool RenderTarget::initialize()
    {
        if (m_initialized)
        {
            Log::warn("RenderTarget already initialized");
            return true;
        }

        Log::info("Initializing RenderTarget");

        // Generate framebuffer
        glGenFramebuffers(1, &m_framebufferID);

        // If multisampled, also create resolve framebuffer
        if (m_multisampled)
        {
            glGenFramebuffers(1, &m_resolveFramebufferID);
        }

        // Initialize empty vectors for color attachments
        m_colorTextures.resize(8);
        if (m_multisampled)
        {
            m_resolveColorTextures.resize(8);
        }

        m_initialized = true;
        return true;
    }

    void RenderTarget::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        Log::info("Shutting down RenderTarget");

        // Delete framebuffers
        if (m_framebufferID != 0)
        {
            glDeleteFramebuffers(1, &m_framebufferID);
            m_framebufferID = 0;
        }

        if (m_resolveFramebufferID != 0)
        {
            glDeleteFramebuffers(1, &m_resolveFramebufferID);
            m_resolveFramebufferID = 0;
        }

        // Clear texture references
        m_colorTextures.clear();
        m_resolveColorTextures.clear();
        m_depthTexture = nullptr;
        m_stencilTexture = nullptr;
        m_depthStencilTexture = nullptr;

        m_initialized = false;
    }

    bool RenderTarget::isInitialized() const
    {
        return m_initialized;
    }

    bool RenderTarget::addColorAttachment(TextureFormat format, int index)
    {
        if (!m_initialized)
        {
            Log::error("RenderTarget not initialized");
            return false;
        }

        if (index < 0 || index >= 8)
        {
            Log::error("Invalid color attachment index: " + std::to_string(index));
            return false;
        }

        // Create texture attachment
        bind();

        // Create multisampled texture if needed
        if (m_multisampled)
        {
            // Create multisampled texture
            auto msTexture = std::make_shared<Texture>();
            msTexture->initialize(m_width, m_height, format, m_multisampleCount);

            // Attach to framebuffer
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index,
                                   GL_TEXTURE_2D_MULTISAMPLE, msTexture->getID(), 0);

            m_colorTextures[index] = msTexture;

            // Also create resolve texture
            auto resolveTexture = std::make_shared<Texture>();
            resolveTexture->initialize(m_width, m_height, format);

            // Bind resolve framebuffer and attach texture
            glBindFramebuffer(GL_FRAMEBUFFER, m_resolveFramebufferID);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index,
                                   GL_TEXTURE_2D, resolveTexture->getID(), 0);

            m_resolveColorTextures[index] = resolveTexture;

            // Rebind main framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferID);
        }
        else
        {
            // Create normal texture
            auto texture = std::make_shared<Texture>();
            texture->initialize(m_width, m_height, format);

            // Attach to framebuffer
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index,
                                   GL_TEXTURE_2D, texture->getID(), 0);

            m_colorTextures[index] = texture;
        }

        // Setup draw buffers (necessary for MRT)
        GLenum drawBuffers[8] = {GL_NONE};
        int numAttachments = 0;

        for (int i = 0; i < 8; i++)
        {
            if (m_colorTextures[i])
            {
                drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
                numAttachments++;
            }
        }

        if (numAttachments > 0)
        {
            glDrawBuffers(numAttachments, drawBuffers);
        }

        unbind();
        return true;
    }

    bool RenderTarget::setDepthAttachment(TextureFormat format)
    {
        if (!m_initialized)
        {
            Log::error("RenderTarget not initialized");
            return false;
        }

        // Create depth texture
        bind();

        if (m_multisampled)
        {
            // Create multisampled depth texture
            auto depthTexture = std::make_shared<Texture>();
            depthTexture->initialize(m_width, m_height, format, m_multisampleCount);

            // Attach to framebuffer
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                   GL_TEXTURE_2D_MULTISAMPLE, depthTexture->getID(), 0);

            m_depthTexture = depthTexture;

            // Bind resolve framebuffer and attach normal depth texture
            auto resolveDepthTexture = std::make_shared<Texture>();
            resolveDepthTexture->initialize(m_width, m_height, format);

            glBindFramebuffer(GL_FRAMEBUFFER, m_resolveFramebufferID);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                   GL_TEXTURE_2D, resolveDepthTexture->getID(), 0);

            // Switch back to main framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferID);
        }
        else
        {
            // Create normal depth texture
            auto depthTexture = std::make_shared<Texture>();
            depthTexture->initialize(m_width, m_height, format);

            // Attach to framebuffer
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                   GL_TEXTURE_2D, depthTexture->getID(), 0);

            m_depthTexture = depthTexture;
        }

        unbind();
        return true;
    }

    bool RenderTarget::setStencilAttachment()
    {
        if (!m_initialized)
        {
            Log::error("RenderTarget not initialized");
            return false;
        }

        // Create stencil texture
        bind();

        if (m_multisampled)
        {
            // Create multisampled stencil texture
            auto stencilTexture = std::make_shared<Texture>();
            stencilTexture->initialize(m_width, m_height, TextureFormat::Stencil8, m_multisampleCount);

            // Attach to framebuffer
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                   GL_TEXTURE_2D_MULTISAMPLE, stencilTexture->getID(), 0);

            m_stencilTexture = stencilTexture;

            // Bind resolve framebuffer and attach normal stencil texture
            auto resolveStencilTexture = std::make_shared<Texture>();
            resolveStencilTexture->initialize(m_width, m_height, TextureFormat::Stencil8);

            glBindFramebuffer(GL_FRAMEBUFFER, m_resolveFramebufferID);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                   GL_TEXTURE_2D, resolveStencilTexture->getID(), 0);

            // Switch back to main framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferID);
        }
        else
        {
            // Create normal stencil texture
            auto stencilTexture = std::make_shared<Texture>();
            stencilTexture->initialize(m_width, m_height, TextureFormat::Stencil8);

            // Attach to framebuffer
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                   GL_TEXTURE_2D, stencilTexture->getID(), 0);

            m_stencilTexture = stencilTexture;
        }

        unbind();
        return true;
    }

    bool RenderTarget::setDepthStencilAttachment()
    {
        if (!m_initialized)
        {
            Log::error("RenderTarget not initialized");
            return false;
        }

        // Create combined depth-stencil texture
        bind();

        if (m_multisampled)
        {
            // Create multisampled depth-stencil texture
            auto depthStencilTexture = std::make_shared<Texture>();
            depthStencilTexture->initialize(m_width, m_height, TextureFormat::Depth24Stencil8, m_multisampleCount);

            // Attach to framebuffer
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                   GL_TEXTURE_2D_MULTISAMPLE, depthStencilTexture->getID(), 0);

            m_depthStencilTexture = depthStencilTexture;

            // Bind resolve framebuffer and attach normal depth-stencil texture
            auto resolveDepthStencilTexture = std::make_shared<Texture>();
            resolveDepthStencilTexture->initialize(m_width, m_height, TextureFormat::Depth24Stencil8);

            glBindFramebuffer(GL_FRAMEBUFFER, m_resolveFramebufferID);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                   GL_TEXTURE_2D, resolveDepthStencilTexture->getID(), 0);

            // Switch back to main framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferID);
        }
        else
        {
            // Create normal depth-stencil texture
            auto depthStencilTexture = std::make_shared<Texture>();
            depthStencilTexture->initialize(m_width, m_height, TextureFormat::Depth24Stencil8);

            // Attach to framebuffer
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                   GL_TEXTURE_2D, depthStencilTexture->getID(), 0);

            m_depthStencilTexture = depthStencilTexture;
        }

        unbind();
        return true;
    }

    bool RenderTarget::validate()
    {
        if (!m_initialized)
        {
            Log::error("RenderTarget not initialized");
            return false;
        }

        bind();
        bool status = checkStatus();
        unbind();

        return status;
    }

    void RenderTarget::bind()
    {
        if (!m_initialized)
        {
            Log::error("RenderTarget not initialized");
            return;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferID);
        glViewport(0, 0, m_width, m_height);
    }

    void RenderTarget::unbind()
    {
        // If multisampled, resolve to resolve framebuffer
        if (m_multisampled && m_resolveFramebufferID != 0)
        {
            resolveMultisampledTextures();
        }

        // Bind default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void RenderTarget::clear(bool clearColor, bool clearDepth, bool clearStencil)
    {
        if (!m_initialized)
        {
            Log::error("RenderTarget not initialized");
            return;
        }

        bind();

        GLbitfield clearFlags = 0;
        if (clearColor) clearFlags |= GL_COLOR_BUFFER_BIT;
        if (clearDepth) clearFlags |= GL_DEPTH_BUFFER_BIT;
        if (clearStencil) clearFlags |= GL_STENCIL_BUFFER_BIT;

        glClear(clearFlags);

        unbind();
    }

    void RenderTarget::clearColor(int index, const glm::vec4& color)
    {
        if (!m_initialized)
        {
            Log::error("RenderTarget not initialized");
            return;
        }

        if (index < 0 || index >= 8 || !m_colorTextures[index])
        {
            Log::error("Invalid color attachment index: " + std::to_string(index));
            return;
        }

        bind();

        // Store current clear color
        GLfloat currentClearColor[4];
        glGetFloatv(GL_COLOR_CLEAR_VALUE, currentClearColor);

        // Set new clear color
        glClearBufferfv(GL_COLOR, index, &color[0]);

        // Restore original clear color
        glClearColor(currentClearColor[0], currentClearColor[1], currentClearColor[2], currentClearColor[3]);

        unbind();
    }

    std::shared_ptr<Texture> RenderTarget::getColorTexture(int index)
    {
        if (!m_initialized)
        {
            Log::error("RenderTarget not initialized");
            return nullptr;
        }

        if (index < 0 || index >= 8)
        {
            Log::error("Invalid color attachment index: " + std::to_string(index));
            return nullptr;
        }

        // For multisampled textures, return the resolved texture
        if (m_multisampled && m_resolveColorTextures[index])
        {
            return m_resolveColorTextures[index];
        }

        return m_colorTextures[index];
    }

    std::shared_ptr<Texture> RenderTarget::getDepthTexture()
    {
        return m_depthTexture;
    }

    std::shared_ptr<Texture> RenderTarget::getStencilTexture()
    {
        return m_stencilTexture;
    }

    std::shared_ptr<Texture> RenderTarget::getDepthStencilTexture()
    {
        return m_depthStencilTexture;
    }

    void RenderTarget::blit(std::shared_ptr<RenderTarget> target,
                            bool colorBuffer, bool depthBuffer, bool stencilBuffer)
    {
        if (!m_initialized || !target || !target->isInitialized())
        {
            Log::error("RenderTarget not initialized for blit operation");
            return;
        }

        GLbitfield mask = 0;
        if (colorBuffer) mask |= GL_COLOR_BUFFER_BIT;
        if (depthBuffer) mask |= GL_DEPTH_BUFFER_BIT;
        if (stencilBuffer) mask |= GL_STENCIL_BUFFER_BIT;

        // Use resolve framebuffer as source if multisampled
        uint32_t srcFramebuffer = m_multisampled ? m_resolveFramebufferID : m_framebufferID;

        // Blit from source to destination
        glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFramebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target->getFramebufferID());

        glBlitFramebuffer(
            0, 0, m_width, m_height,
            0, 0, target->getWidth(), target->getHeight(),
            mask, GL_NEAREST
        );

        // Unbind framebuffers
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    void RenderTarget::resize(int width, int height)
    {
        if (!m_initialized)
        {
            Log::error("RenderTarget not initialized");
            return;
        }

        if (width <= 0 || height <= 0)
        {
            Log::error("Invalid dimensions for resize: " + std::to_string(width) + "x" + std::to_string(height));
            return;
        }

        // If dimensions haven't changed, do nothing
        if (width == m_width && height == m_height)
        {
            return;
        }

        // Store texture formats and recreate
        std::vector<std::pair<bool, TextureFormat>> colorFormats(8, {false, TextureFormat::RGBA8});
        bool hasDepth = false;
        bool hasStencil = false;
        bool hasDepthStencil = false;
        TextureFormat depthFormat = TextureFormat::Depth24;
        TextureFormat stencilFormat = TextureFormat::Stencil8;
        TextureFormat depthStencilFormat = TextureFormat::Depth24Stencil8;

        // Save existing texture formats
        for (int i = 0; i < 8; i++)
        {
            if (m_colorTextures[i])
            {
                colorFormats[i] = {true, m_colorTextures[i]->getFormat()};
            }
        }

        if (m_depthTexture)
        {
            hasDepth = true;
            depthFormat = m_depthTexture->getFormat();
        }

        if (m_stencilTexture)
        {
            hasStencil = true;
            stencilFormat = m_stencilTexture->getFormat();
        }

        if (m_depthStencilTexture)
        {
            hasDepthStencil = true;
            depthStencilFormat = m_depthStencilTexture->getFormat();
        }

        // Update dimensions
        m_width = width;
        m_height = height;

        // Recreate framebuffer with new size
        shutdown();
        initialize();

        // Recreate attachments
        for (int i = 0; i < 8; i++)
        {
            if (colorFormats[i].first)
            {
                addColorAttachment(colorFormats[i].second, i);
            }
        }

        if (hasDepth)
        {
            setDepthAttachment(depthFormat);
        }

        if (hasStencil)
        {
            setStencilAttachment();
        }

        if (hasDepthStencil)
        {
            setDepthStencilAttachment();
        }
    }

    void RenderTarget::readPixels(int x, int y, int width, int height, void* data, TextureFormat format)
    {
        if (!m_initialized)
        {
            Log::error("RenderTarget not initialized");
            return;
        }

        // Ensure x, y, width, height are valid
        if (x < 0 || y < 0 || width <= 0 || height <= 0 ||
            x + width > m_width || y + height > m_height)
        {
            Log::error("Invalid parameters for readPixels");
            return;
        }

        // Determine the GL format and type based on the TextureFormat
        GLenum glFormat = GL_RGBA;
        GLenum glType = GL_UNSIGNED_BYTE;

        switch (format)
        {
            case TextureFormat::R8:
                glFormat = GL_RED;
                glType = GL_UNSIGNED_BYTE;
                break;
            case TextureFormat::RG8:
                glFormat = GL_RG;
                glType = GL_UNSIGNED_BYTE;
                break;
            case TextureFormat::RGB8:
                glFormat = GL_RGB;
                glType = GL_UNSIGNED_BYTE;
                break;
            case TextureFormat::RGBA8:
                glFormat = GL_RGBA;
                glType = GL_UNSIGNED_BYTE;
                break;
            case TextureFormat::R32F:
                glFormat = GL_RED;
                glType = GL_FLOAT;
                break;
            case TextureFormat::RG32F:
                glFormat = GL_RG;
                glType = GL_FLOAT;
                break;
            case TextureFormat::RGB32F:
                glFormat = GL_RGB;
                glType = GL_FLOAT;
                break;
            case TextureFormat::RGBA32F:
                glFormat = GL_RGBA;
                glType = GL_FLOAT;
                break;
            default:
                Log::error("Unsupported format for readPixels");
                return;
        }

        // Bind the appropriate framebuffer
        uint32_t fbID = m_multisampled ? m_resolveFramebufferID : m_framebufferID;
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbID);

        // Set read buffer to first color attachment
        glReadBuffer(GL_COLOR_ATTACHMENT0);

        // Perform pixel read
        glReadPixels(x, y, width, height, glFormat, glType, data);

        // Unbind framebuffer
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }

    uint32_t RenderTarget::getFramebufferID() const
    {
        return m_framebufferID;
    }

    int RenderTarget::getWidth() const
    {
        return m_width;
    }

    int RenderTarget::getHeight() const
    {
        return m_height;
    }

    bool RenderTarget::isMultisampled() const
    {
        return m_multisampled;
    }

    int RenderTarget::getMultisampleCount() const
    {
        return m_multisampled ? m_multisampleCount : 1;
    }

    void RenderTarget::resolveMultisampledTextures()
    {
        if (!m_multisampled || m_resolveFramebufferID == 0)
        {
            return;
        }

        // For each color attachment, blit from multisampled to resolve
        for (int i = 0; i < 8; i++)
        {
            if (m_colorTextures[i] && m_resolveColorTextures[i])
            {
                glBindFramebuffer(GL_READ_FRAMEBUFFER, m_framebufferID);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_resolveFramebufferID);

                glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
                glDrawBuffer(GL_COLOR_ATTACHMENT0 + i);

                glBlitFramebuffer(
                    0, 0, m_width, m_height,
                    0, 0, m_width, m_height,
                    GL_COLOR_BUFFER_BIT, GL_NEAREST
                );
            }
        }

        // Resolve depth if present
        if (m_depthTexture)
        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, m_framebufferID);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_resolveFramebufferID);

            glBlitFramebuffer(
                0, 0, m_width, m_height,
                0, 0, m_width, m_height,
                GL_DEPTH_BUFFER_BIT, GL_NEAREST
            );
        }

        // Resolve stencil if present (stencil can't be blitted separately, only with depth)
        if (m_depthStencilTexture)
        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, m_framebufferID);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_resolveFramebufferID);

            glBlitFramebuffer(
                0, 0, m_width, m_height,
                0, 0, m_width, m_height,
                GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST
            );
        }

        // Reset framebuffer bindings
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    bool RenderTarget::checkStatus()
    {
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            switch (status)
            {
                case GL_FRAMEBUFFER_UNDEFINED:
                    Log::error("Framebuffer undefined");
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                    Log::error("Framebuffer incomplete attachment");
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                    Log::error("Framebuffer incomplete missing attachment");
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                    Log::error("Framebuffer incomplete draw buffer");
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                    Log::error("Framebuffer incomplete read buffer");
                    break;
                case GL_FRAMEBUFFER_UNSUPPORTED:
                    Log::error("Framebuffer unsupported");
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                    Log::error("Framebuffer incomplete multisample");
                    break;
                default:
                    Log::error("Framebuffer error: Unknown status");
                    break;
            }
            return false;
        }

        return true;
    }

} // namespace PixelCraft::Rendering