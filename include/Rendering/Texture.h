// -------------------------------------------------------------------------
// Texture.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "Core/Resource.h"

namespace PixelCraft::Rendering
{

    /**
     * @brief Defines texture filtering modes for magnification and minification
     */
    enum class TextureFilterMode
    {
        Nearest,               ///< Nearest neighbor filtering (pixel art)
        Linear,                ///< Linear filtering (smoother appearance)
        NearestMipmapNearest,  ///< Nearest filtering with nearest mipmap level
        LinearMipmapNearest,   ///< Linear filtering with nearest mipmap level
        NearestMipmapLinear,   ///< Nearest filtering with linear mipmap interpolation
        LinearMipmapLinear     ///< Linear filtering with linear mipmap interpolation (trilinear)
    };

    /**
     * @brief Defines texture edge handling modes
     */
    enum class TextureWrapMode
    {
        Repeat,          ///< Texture repeats at boundaries
        MirroredRepeat,  ///< Texture mirrors at boundaries
        ClampToEdge,     ///< Texture coordinates are clamped to edge
        ClampToBorder    ///< Texture coordinates beyond boundary use border color
    };

    /**
     * @brief Defines internal texture formats for storage
     */
    enum class TextureFormat
    {
        // Standard formats
        R8,              ///< 8-bit single channel (red)
        RG8,             ///< 8-bit dual channel (red, green)
        RGB8,            ///< 8-bit three channel (red, green, blue)
        RGBA8,           ///< 8-bit four channel (red, green, blue, alpha)

        // Float formats (16-bit)
        R16F,            ///< 16-bit float single channel
        RG16F,           ///< 16-bit float dual channel
        RGB16F,          ///< 16-bit float three channel
        RGBA16F,         ///< 16-bit float four channel

        // Float formats (32-bit)
        R32F,            ///< 32-bit float single channel
        RG32F,           ///< 32-bit float dual channel
        RGB32F,          ///< 32-bit float three channel
        RGBA32F,         ///< 32-bit float four channel

        // Special float formats
        R11F_G11F_B10F,  ///< Packed format with 11 bits each for R and G, 10 bits for B
        RGB9_E5,         ///< Shared exponent format

        // Integer formats
        R8I,             ///< 8-bit signed integer, single channel
        RG8I,            ///< 8-bit signed integer, dual channel
        RGB8I,           ///< 8-bit signed integer, three channel
        RGBA8I,          ///< 8-bit signed integer, four channel
        R16I,            ///< 16-bit signed integer, single channel
        RG16I,           ///< 16-bit signed integer, dual channel
        RGB16I,          ///< 16-bit signed integer, three channel
        RGBA16I,         ///< 16-bit signed integer, four channel

        // Unsigned integer formats
        R8UI,            ///< 8-bit unsigned integer, single channel
        RG8UI,           ///< 8-bit unsigned integer, dual channel
        RGB8UI,          ///< 8-bit unsigned integer, three channel
        RGBA8UI,         ///< 8-bit unsigned integer, four channel
        R16UI,           ///< 16-bit unsigned integer, single channel
        RG16UI,          ///< 16-bit unsigned integer, dual channel
        RGB16UI,         ///< 16-bit unsigned integer, three channel
        RGBA16UI,        ///< 16-bit unsigned integer, four channel

        // Special formats
        RGB10_A2,        ///< 10-bit per RGB component with 2-bit alpha

        // sRGB formats
        SRGB8,           ///< 8-bit sRGB
        SRGB8_A8,        ///< 8-bit sRGB with 8-bit alpha

        // Depth/stencil formats
        Depth16,         ///< 16-bit depth component
        Depth24,         ///< 24-bit depth component
        Depth32F,        ///< 32-bit float depth component
        Stencil8,        ///< 8-bit stencil component
        Depth24Stencil8, ///< 24-bit depth with 8-bit stencil

        // Compressed formats - DXT
        DXT1_RGB,        ///< DXT1 compressed (no alpha)
        DXT1_RGBA,       ///< DXT1 compressed (1-bit alpha)
        DXT3,            ///< DXT3 compressed (explicit alpha)
        DXT5,            ///< DXT5 compressed (interpolated alpha)

        // Compressed formats - ETC2
        ETC2_RGB,        ///< ETC2 compressed RGB
        ETC2_RGBA,       ///< ETC2 compressed RGBA

        // Compressed formats - ASTC
        ASTC_4x4,        ///< ASTC compressed 4x4 blocks
        ASTC_8x8         ///< ASTC compressed 8x8 blocks
    };

    /**
     * @brief 2D image resource with pixel art optimizations
     *
     * Texture class represents 2D image resources that can be loaded from files,
     * created from raw data, or generated procedurally. It provides special
     * handling for pixel art with features like pixel-perfect filtering and
     * palette mapping.
     */
    class Texture : public Core::Resource
    {
    public:
        /**
         * @brief Constructor
         * @param name Resource name for identification and tracking
         */
        Texture(const std::string& name);

        /**
         * @brief Destructor
         *
         * Automatically releases GPU resources if not already unloaded
         */
        virtual ~Texture();

        /**
         * @brief Initializes the texture with default settings
         * @return True if initialization was successful, false otherwise
         *
         * Creates an empty texture handle without allocating storage.
         */
        bool initialize();

        /**
         * @brief Creates an empty texture with specified dimensions and format
         * @param width Width of the texture in pixels
         * @param height Height of the texture in pixels
         * @param format Internal format for texture storage (default: RGBA8)
         * @return True if creation was successful, false otherwise
         *
         * Creates a new texture with uninitialized content. Useful for render targets
         * or textures that will be filled with data later.
         */
        bool initialize(int width, int height, TextureFormat format = TextureFormat::RGBA8);

        /**
         * @brief Creates an empty texture with specified dimensions and format
         * @param width Width of the texture in pixels
         * @param height Height of the texture in pixels
         * @param format Internal format for texture storage (default: RGBA8)
         * @param multisampleCount The multisample count
         * @return True if creation was successful, false otherwise
         *
         * Creates a new texture with uninitialized content. Useful for render targets
         * or textures that will be filled with data later.
         */
        bool initialize(int width, int height, TextureFormat format, int multisampleCount);

        /**
         * @brief Creates a texture from provided pixel data
         * @param width Width of the texture in pixels
         * @param height Height of the texture in pixels
         * @param format Internal format for texture storage
         * @param data Pointer to the raw pixel data
         * @return True if creation was successful, false otherwise
         *
         * The data pointer should point to contiguous memory with size at least
         * width × height × channels bytes, where channels depends on the format.
         */
        bool initialize(int width, int height, TextureFormat format, const void* data);

        /**
         * @brief Sets the raw pixel data of the texture
         * @param data Pointer to the raw pixel data
         */
        void setData(const void* data);

        /**
         * @brief Loads the texture from its source file path
         * @return True if loading was successful, false otherwise
         *
         * Uses the resource path to load image data using stb_image library.
         * Supports PNG, JPEG, BMP, TGA, and other common formats.
         */
        bool load() override;

        /**
         * @brief Releases all texture resources from GPU and CPU memory
         *
         * Deletes the OpenGL texture and releases any pixel data stored on the CPU.
         */
        void unload() override;

        /**
         * @brief Reloads the texture from its source while preserving settings
         * @return True if reloading was successful, false otherwise
         *
         * Used for hot-reloading when the source file changes. Preserves
         * filtering mode, wrap mode, and pixel grid alignment settings.
         */
        bool onReload() override;

        /**
         * @brief Binds the texture to the specified texture unit for rendering
         * @param textureUnit The OpenGL texture unit to bind to (default: 0)
         *
         * Automatically loads the texture if not already loaded.
         */
        void bind(uint32_t textureUnit = 0);

        /**
         * @brief Unbinds the texture from the current GL context
         *
         * Binds texture ID 0 to the current active texture unit.
         */
        void unbind();

        /**
         * @brief Sets pixel grid alignment mode for pixel art
         * @param enabled True to enable pixel-perfect filtering, false for standard filtering
         *
         * When enabled, forces nearest-neighbor filtering regardless of the filter mode
         * to maintain sharp pixel edges for pixel art.
         */
        void setPixelGridAlignment(bool enabled);

        /**
         * @brief Gets the current pixel grid alignment mode
         * @return True if pixel grid alignment is enabled, false otherwise
         */
        bool isPixelGridAligned() const;

        /**
         * @brief Sets the texture filtering mode for magnification and minification
         * @param mode The filtering mode to use
         *
         * Has no effect if pixel grid alignment is enabled, which forces nearest filtering.
         */
        void setFilterMode(TextureFilterMode mode);

        /**
         * @brief Sets the texture wrapping mode for texture coordinates
         * @param mode The wrapping mode to use
         *
         * Controls how texture coordinates outside the [0,1] range are handled.
         */
        void setWrapMode(TextureWrapMode mode);

        /**
         * @brief Gets the current texture filtering mode
         * @return The current filtering mode
         */
        TextureFilterMode getFilterMode() const;

        /**
         * @brief Gets the current texture wrapping mode
         * @return The current wrapping mode
         */
        TextureWrapMode getWrapMode() const;

        /**
         * @brief Sets the multisample count for antialiasing
         * @param samples Number of samples (0 = no multisampling)
         * @return True if setting was successful, false otherwise
         */
        bool setMultisampleCount(int samples);

        /**
         * @brief Gets the current multisample count
         * @return Number of multisample samples (0 if not multisampled)
         */
        int getMultisampleCount() const
        {
            return m_multisampleCount;
        }

        /**
         * @brief Generates mipmaps for the texture
         *
         * Creates a full mipmap chain for the texture to improve rendering quality
         * at different scales. Only has an effect if a mipmap filtering mode is set.
         */
        void generateMipmaps();

        /**
         * @brief Checks if mipmaps have been generated for this texture
         * @return True if mipmaps exist, false otherwise
         */
        bool hasMipmaps() const;

        /**
         * @brief Sets or changes the texture dimensions
         * @param width New width in pixels
         * @param height New height in pixels
         * @param preserveData Whether to attempt to preserve existing pixel data
         * @return True if resizing was successful, false otherwise
         */
        bool resize(int width, int height, bool preserveData = false);

        /**
         * @brief Sets or changes the texture format
         * @param format New internal texture format
         * @param preserveData Whether to attempt to preserve existing pixel data
         * @return True if format change was successful, false otherwise
         */
        bool setFormat(TextureFormat format, bool preserveData = false);

        /**
         * @brief Sets the color of a specific pixel
         * @param x X-coordinate of the pixel
         * @param y Y-coordinate of the pixel
         * @param color The color to set in RGBA format (0.0-1.0 range)
         *
         * Updates both CPU cache and GPU texture. Useful for pixel-by-pixel modification
         * or procedural texture generation.
         */
        void setPixel(int x, int y, const glm::vec4& color);

        /**
         * @brief Gets the color of a specific pixel
         * @param x X-coordinate of the pixel
         * @param y Y-coordinate of the pixel
         * @return The pixel color in RGBA format (0.0-1.0 range)
         *
         * Reads from CPU cache if available. If not, returns black.
         */
        glm::vec4 getPixel(int x, int y);

        /**
         * @brief Updates a region of the texture with new pixel data
         * @param data Pointer to the new pixel data
         * @param x X-coordinate of the region's top-left corner (default: 0)
         * @param y Y-coordinate of the region's top-left corner (default: 0)
         * @param width Width of the region to update (default: 0 for full width)
         * @param height Height of the region to update (default: 0 for full height)
         *
         * More efficient than setting individual pixels for large regions.
         * If width or height is 0, uses the full texture dimensions.
         */
        void update(const void* data, int x = 0, int y = 0, int width = 0, int height = 0);

        /**
         * @brief Maps texture colors to the nearest colors in the provided palette
         * @param paletteManager The palette manager containing color constraints
         *
         * Used for pixel art to enforce a consistent color palette. This operation
         * modifies the texture's pixel data.
         */
        void applyPaletteMapping(std::shared_ptr<class PaletteManager> paletteManager);

        /**
         * @brief Gets the OpenGL texture ID
         * @return The OpenGL texture handle
         */
        uint32_t getID() const;

        /**
         * @brief Gets the texture width
         * @return Width in pixels
         */
        int getWidth() const;

        /**
         * @brief Gets the texture height
         * @return Height in pixels
         */
        int getHeight() const;

        /**
         * @brief Gets the texture format
         * @return The internal format of the texture
         */
        TextureFormat getFormat() const;

    private:
        uint32_t m_textureID;               ///< OpenGL texture handle
        int m_width;                         ///< Texture width in pixels
        int m_height;                        ///< Texture height in pixels
        TextureFormat m_format;              ///< Internal texture format
        TextureFilterMode m_filterMode;      ///< Current filtering mode
        TextureWrapMode m_wrapMode;          ///< Current wrapping mode
        bool m_hasMipmaps;                   ///< Whether mipmaps have been generated
        bool m_pixelGridAligned;             ///< Whether pixel-perfect mode is enabled
        bool m_hasPixelData;                 ///< Whether CPU-side pixel data is available
        std::unique_ptr<uint8_t[]> m_pixelData; ///< CPU-side pixel data cache
        int m_channels;                      ///< Number of color channels
        int m_multisampleCount;              ///< Number of multisample samples (0 = no multisampling)

        /**
         * @brief Loads texture data from a file
         * @param filepath Path to the image file
         * @return True if loading was successful, false otherwise
         *
         * Internal implementation used by load() method.
         */
        bool loadFromFile(const std::string& filepath);

        /**
         * @brief Sets texture parameters based on current settings
         *
         * Applies filter mode, wrap mode, and other parameters to the bound texture.
         */
        void setTextureParameters();

        /**
         * @brief Gets the number of color channels for a format
         * @param format The texture format to check
         * @return Number of color channels (1-4)
         */
        int getFormatChannelCount(TextureFormat format);

        /**
         * @brief Converts texture format enum to OpenGL format constants
         * @param format The texture format to convert
         * @param internalFormat [out] OpenGL internal format value
         * @param dataFormat [out] OpenGL data format value
         * @param dataType [out] OpenGL data type value
         *
         * Maps PixelCraft texture formats to the appropriate OpenGL constants.
         */
        void convertFormat(TextureFormat format, uint32_t& internalFormat, uint32_t& dataFormat, uint32_t& dataType);
    };

} // namespace PixelCraft::Rendering