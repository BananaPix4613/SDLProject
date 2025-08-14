// -------------------------------------------------------------------------
// GraphicsCapabilities.h
// -------------------------------------------------------------------------
#pragma once

#include <glad/glad.h>
#include <string>
#include <unordered_set>

namespace PixelCraft::Utility
{
    /**
     * @brief Graphics hardware capability detection and management
     * 
     * This class provides methods to detect and query graphics hardware capabilities,
     * allowing the engine to adapt its rendering features based on available hardware.
     */
    class GraphicsCapabilities
    {
    public:
        struct CapabilityInfo
        {
            std::string vendor;
            std::string renderer;
            std::string version;
            GLint maxTextureSize;
            GLint maxTextureLayers;
            GLint maxFramebufferWidth;
            GLint maxFramebufferHeight;
            GLint maxColorAttachments;
            GLint maxDepthBits;
            GLint maxStencilBits;
            bool supportsFloatTextures;
            bool supportsDepthTextures;
            bool supportsMultisample;
            bool supportsAnisotropicFiltering;
            bool supportsSeamlessCubemaps;
            std::unordered_set<std::string> extensions;
        };

        /**
         * @brief Get the singleton instance
         */
        static GraphicsCapabilities& getInstance();

        /**
         * @brief Initialize capabilities detection (must be called with valid OpenGL context)
         */
        bool initialize();

        /**
         * @brief Get capability information
         */
        const CapabilityInfo& getCapabilities() const { return m_capabilities; }

        /**
         * @brief Check if a specific extension is supported
         */
        bool hasExtension(const std::string& extension) const;

        /**
         * @brief Get maximum supported texture size
         */
        GLint getMaxTextureSize() const { return m_capabilities.maxTextureSize; }

        /**
         * @brief Check if depth textures are supported
         */
        bool supportsDepthTextures() const { return m_capabilities.supportsDepthTextures; }

        /**
         * @brief Check if float textures are supported
         */
        bool supportsFloatTextures() const { return m_capabilities.supportsFloatTextures; }

        /**
         * @brief Check if multisampling is supported
         */
        bool supportsMultisample() const { return m_capabilities.supportsMultisample; }

        /**
         * @brief Get recommended shadow map resolution based on hardware
         */
        int getRecommendedShadowMapResolution() const;

        /**
         * @brief Check if hardware is capable of advanced shadow mapping
         */
        bool supportsAdvancedShadowMapping() const;

        /**
         * @brief Log capabilities information
         */
        void logCapabilities() const;

    private:
        GraphicsCapabilities() = default;
        ~GraphicsCapabilities() = default;
        GraphicsCapabilities(const GraphicsCapabilities&) = delete;
        GraphicsCapabilities& operator=(const GraphicsCapabilities&) = delete;

        void detectBasicInfo();
        void detectTextureCapabilities();
        void detectFramebufferCapabilities();
        void detectExtensions();
        bool checkExtensionSupport(const std::string& extension);

        CapabilityInfo m_capabilities;
        bool m_initialized = false;
    };

} // namespace PixelCraft::Utility