// -------------------------------------------------------------------------
// RenderTarget.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "Core/Resource.h"
#include "Rendering/Texture.h"

namespace PixelCraft::Rendering
{

    /**
     * @brief Enumeration of possible framebuffer attachment points
     */
    enum class RenderTargetAttachment
    {
        Color0,
        Color1,
        Color2,
        Color3,
        Color4,
        Color5,
        Color6,
        Color7,
        Depth,
        Stencil,
        DepthStencil
    };

    /**
     * @brief Framebuffer for off-screen rendering, post-processing, and advanced rendering techniques
     */
    class RenderTarget
    {
    public:
        /**
         * @brief Constructor
         * @param width Width of the render target in pixels
         * @param height Height of the render target in pixels
         * @param multisampled Whether to use multisampling for anti-aliasing
         */
        RenderTarget(int width, int height, bool multisampled = false);

        /**
         * @brief Destructor
         */
        ~RenderTarget();

        /**
         * @brief Initialize the render target and create framebuffer
         * @return True if initialization was successful
         */
        bool initialize();

        /**
         * @brief Clean up resources and delete framebuffer
         */
        void shutdown();

        /**
         * @brief Check if the render target has been initialized
         * @return True if initialized
         */
        bool isInitialized() const;

        /**
         * @brief Add a color attachment to the render target
         * @param format Texture format to use for the attachment
         * @param index Index of the color attachment (0-7)
         * @return True if successful
         */
        bool addColorAttachment(TextureFormat format, int index = 0);

        /**
         * @brief Add a depth attachment to the render target
         * @param format Texture format to use for the depth attachment
         * @return True if successful
         */
        bool setDepthAttachment(TextureFormat format = TextureFormat::Depth24);

        /**
         * @brief Add a stencil attachment to the render target
         * @return True if successful
         */
        bool setStencilAttachment();

        /**
         * @brief Add a combined depth-stencil attachment to the render target
         * @return True if successful
         */
        bool setDepthStencilAttachment();

        /**
         * @brief Validate that the framebuffer is complete and ready for use
         * @return True if the framebuffer is valid and complete
         */
        bool validate();

        /**
         * @brief Bind the framebuffer for rendering
         */
        void bind();

        /**
         * @brief Unbind the framebuffer and return to default framebuffer
         */
        void unbind();

        /**
         * @brief Clear the contents of the framebuffer
         * @param clearColor Whether to clear the color buffers
         * @param clearDepth Whether to clear the depth buffer
         * @param clearStencil Whether to clear the stencil buffer
         */
        void clear(bool clearColor = true, bool clearDepth = true, bool clearStencil = false);

        /**
         * @brief Clear a specific color attachment with a color
         * @param index Index of the color attachment to clear
         * @param color Color to clear with
         */
        void clearColor(int index, const glm::vec4& color);

        /**
         * @brief Get a color attachment texture
         * @param index Index of the color attachment
         * @return Shared pointer to the texture
         */
        std::shared_ptr<Texture> getColorTexture(int index = 0);

        /**
         * @brief Get the depth attachment texture
         * @return Shared pointer to the texture
         */
        std::shared_ptr<Texture> getDepthTexture();

        /**
         * @brief Get the stencil attachment texture
         * @return Shared pointer to the texture
         */
        std::shared_ptr<Texture> getStencilTexture();

        /**
         * @brief Get the combined depth-stencil attachment texture
         * @return Shared pointer to the texture
         */
        std::shared_ptr<Texture> getDepthStencilTexture();

        /**
         * @brief Copy contents from this render target to another
         * @param target Target render target to blit to
         * @param colorBuffer Whether to copy color buffers
         * @param depthBuffer Whether to copy depth buffer
         * @param stencilBuffer Whether to copy stencil buffer
         */
        void blit(std::shared_ptr<RenderTarget> target,
                  bool colorBuffer = true, bool depthBuffer = false, bool stencilBuffer = false);

        /**
         * @brief Resize the render target and all attachments
         * @param width New width in pixels
         * @param height New height in pixels
         */
        void resize(int width, int height);

        /**
         * @brief Read pixels from the render target into CPU memory
         * @param x X coordinate to start reading from
         * @param y Y coordinate to start reading from
         * @param width Width of the region to read
         * @param height Height of the region to read
         * @param data Pointer to memory to store the pixel data
         * @param format Format to read the pixels in
         */
        void readPixels(int x, int y, int width, int height, void* data,
                        TextureFormat format = TextureFormat::RGBA8);

        /**
         * @brief Get the OpenGL framebuffer ID
         * @return OpenGL framebuffer ID
         */
        uint32_t getFramebufferID() const;

        /**
         * @brief Get the width of the render target
         * @return Width in pixels
         */
        int getWidth() const;

        /**
         * @brief Get the height of the render target
         * @return Height in pixels
         */
        int getHeight() const;

        /**
         * @brief Check if the render target is multisampled
         * @return True if multisampled
         */
        bool isMultisampled() const;

        /**
         * @brief Get the multisample count
         * @return Number of samples, or 1 if not multisampled
         */
        int getMultisampleCount() const;

    private:
        /**
         * @brief Create and bind textures to attachment points
         * @return True if successful
         */
        bool createAttachments();

        /**
         * @brief Resolve multisampled textures to regular textures
         */
        void resolveMultisampledTextures();

        /**
         * @brief Check the framebuffer status
         * @return True if the framebuffer is complete
         */
        bool checkStatus();

        uint32_t m_framebufferID;
        int m_width;
        int m_height;
        bool m_multisampled;
        int m_multisampleCount;
        bool m_initialized;

        std::vector<std::shared_ptr<Texture>> m_colorTextures;
        std::shared_ptr<Texture> m_depthTexture;
        std::shared_ptr<Texture> m_stencilTexture;
        std::shared_ptr<Texture> m_depthStencilTexture;

        uint32_t m_resolveFramebufferID; // For multisampled targets
        std::vector<std::shared_ptr<Texture>> m_resolveColorTextures;
    };

} // namespace PixelCraft::Rendering