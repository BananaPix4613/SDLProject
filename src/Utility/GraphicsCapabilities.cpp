// -------------------------------------------------------------------------
// GraphicsCapabilities.cpp
// -------------------------------------------------------------------------
#include "Utility/GraphicsCapabilities.h"
#include "Core/Logger.h"

#include <algorithm>
#include <sstream>

namespace PixelCraft::Utility
{

    GraphicsCapabilities& GraphicsCapabilities::getInstance()
    {
        static GraphicsCapabilities instance;
        return instance;
    }

    bool GraphicsCapabilities::initialize()
    {
        if (m_initialized)
        {
            return true;
        }

        Core::info("Detecting graphics hardware capabilities...");

        try
        {
            detectBasicInfo();
            detectTextureCapabilities();
            detectFramebufferCapabilities();
            detectExtensions();

            m_initialized = true;
            logCapabilities();

            return true;
        }
        catch (const std::exception& e)
        {
            Core::error("Failed to detect graphics capabilities: " + std::string(e.what()));
            return false;
        }
    }

    void GraphicsCapabilities::detectBasicInfo()
    {
        const GLubyte* vendor = glGetString(GL_VENDOR);
        const GLubyte* renderer = glGetString(GL_RENDERER);
        const GLubyte* version = glGetString(GL_VERSION);

        m_capabilities.vendor = vendor ? reinterpret_cast<const char*>(vendor) : "Unknown";
        m_capabilities.renderer = renderer ? reinterpret_cast<const char*>(renderer) : "Unknown";
        m_capabilities.version = version ? reinterpret_cast<const char*>(version) : "Unknown";

        // Check for Intel graphics (known to have limitations)
        std::string rendererLower = m_capabilities.renderer;
        std::transform(rendererLower.begin(), rendererLower.end(), rendererLower.begin(), ::tolower);
        
        Core::info("Graphics Hardware Detected:");
        Core::info("  Vendor: " + m_capabilities.vendor);
        Core::info("  Renderer: " + m_capabilities.renderer);
        Core::info("  Version: " + m_capabilities.version);
    }

    void GraphicsCapabilities::detectTextureCapabilities()
    {
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_capabilities.maxTextureSize);
        
        // Check for array texture support
        if (hasExtension("GL_EXT_texture_array"))
        {
            glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &m_capabilities.maxTextureLayers);
        }
        else
        {
            m_capabilities.maxTextureLayers = 0;
        }

        // Check depth texture support
        m_capabilities.supportsDepthTextures = hasExtension("GL_ARB_depth_texture") || 
                                               hasExtension("GL_EXT_packed_depth_stencil");

        // Check float texture support
        m_capabilities.supportsFloatTextures = hasExtension("GL_ARB_texture_float") ||
                                               hasExtension("GL_ATI_texture_float");

        // Check anisotropic filtering support
        m_capabilities.supportsAnisotropicFiltering = hasExtension("GL_EXT_texture_filter_anisotropic");

        // Check seamless cubemap support
        m_capabilities.supportsSeamlessCubemaps = hasExtension("GL_ARB_seamless_cube_map");
    }

    void GraphicsCapabilities::detectFramebufferCapabilities()
    {
        glGetIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, &m_capabilities.maxFramebufferWidth);
        glGetIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT, &m_capabilities.maxFramebufferHeight);
        glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &m_capabilities.maxColorAttachments);

        // Check depth and stencil bit support
        glGetIntegerv(GL_DEPTH_BITS, &m_capabilities.maxDepthBits);
        glGetIntegerv(GL_STENCIL_BITS, &m_capabilities.maxStencilBits);

        // Check multisample support
        GLint maxSamples = 0;
        glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
        m_capabilities.supportsMultisample = (maxSamples > 1);
    }

    void GraphicsCapabilities::detectExtensions()
    {
        GLint numExtensions = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

        for (GLint i = 0; i < numExtensions; ++i)
        {
            const GLubyte* extension = glGetStringi(GL_EXTENSIONS, i);
            if (extension)
            {
                m_capabilities.extensions.insert(reinterpret_cast<const char*>(extension));
            }
        }
    }

    bool GraphicsCapabilities::hasExtension(const std::string& extension) const
    {
        return m_capabilities.extensions.find(extension) != m_capabilities.extensions.end();
    }

    int GraphicsCapabilities::getRecommendedShadowMapResolution() const
    {
        if (!m_initialized)
        {
            return 512; // Conservative default
        }

        // Base recommendation on hardware capabilities
        int maxTexSize = m_capabilities.maxTextureSize;
        std::string rendererLower = m_capabilities.renderer;
        std::transform(rendererLower.begin(), rendererLower.end(), rendererLower.begin(), ::tolower);

        // Intel integrated graphics - use lower resolutions
        if (rendererLower.find("intel") != std::string::npos && 
            (rendererLower.find("uhd") != std::string::npos || 
             rendererLower.find("hd graphics") != std::string::npos))
        {
            return std::min(512, maxTexSize);
        }
        
        // NVIDIA/AMD discrete graphics - can handle higher resolutions
        if (rendererLower.find("nvidia") != std::string::npos || 
            rendererLower.find("radeon") != std::string::npos ||
            rendererLower.find("geforce") != std::string::npos)
        {
            return std::min(2048, maxTexSize);
        }

        // Conservative default for unknown hardware
        return std::min(1024, maxTexSize);
    }

    bool GraphicsCapabilities::supportsAdvancedShadowMapping() const
    {
        if (!m_initialized)
        {
            return false;
        }

        // Check for requirements for advanced shadow mapping
        return m_capabilities.supportsDepthTextures &&
               m_capabilities.supportsFloatTextures &&
               m_capabilities.maxTextureSize >= 1024 &&
               m_capabilities.maxColorAttachments >= 4;
    }

    void GraphicsCapabilities::logCapabilities() const
    {
        if (!m_initialized)
        {
            Core::warn("Graphics capabilities not initialized");
            return;
        }

        Core::info("Graphics Capabilities Summary:");
        Core::info("  Max Texture Size: " + std::to_string(m_capabilities.maxTextureSize));
        Core::info("  Max Framebuffer Size: " + std::to_string(m_capabilities.maxFramebufferWidth) + 
                   "x" + std::to_string(m_capabilities.maxFramebufferHeight));
        Core::info("  Max Color Attachments: " + std::to_string(m_capabilities.maxColorAttachments));
        Core::info("  Depth Bits: " + std::to_string(m_capabilities.maxDepthBits));
        Core::info("  Stencil Bits: " + std::to_string(m_capabilities.maxStencilBits));
        Core::info("  Supports Depth Textures: " + std::string(m_capabilities.supportsDepthTextures ? "Yes" : "No"));
        Core::info("  Supports Float Textures: " + std::string(m_capabilities.supportsFloatTextures ? "Yes" : "No"));
        Core::info("  Supports Multisample: " + std::string(m_capabilities.supportsMultisample ? "Yes" : "No"));
        Core::info("  Recommended Shadow Resolution: " + std::to_string(getRecommendedShadowMapResolution()));
        Core::info("  Advanced Shadow Mapping: " + std::string(supportsAdvancedShadowMapping() ? "Supported" : "Not Supported"));
    }

} // namespace PixelCraft::Utility