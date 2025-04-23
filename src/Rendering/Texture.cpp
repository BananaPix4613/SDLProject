// -------------------------------------------------------------------------
// Texture.cpp
// -------------------------------------------------------------------------
#include "Rendering/Texture.h"
#include "Rendering/PaletteManager.h"
#include "Core/Logger.h"
#include <glad/glad.h>
#include <GL/glew.h>
#include <stb_image.h>
#include <algorithm>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Rendering
{

    Texture::Texture(const std::string& name)
        : Core::Resource(name)
        , m_textureID(0)
        , m_width(0)
        , m_height(0)
        , m_format(TextureFormat::RGBA8)
        , m_filterMode(TextureFilterMode::Nearest) // Default to nearest for pixel art
        , m_wrapMode(TextureWrapMode::ClampToEdge)
        , m_hasMipmaps(false)
        , m_pixelGridAligned(true)  // Default to pixel grid alignment for pixel art
        , m_hasPixelData(false)
        , m_channels(4)
        , m_multisampleCount(0)
    {
    }

    Texture::~Texture()
    {
        unload();
    }

    bool Texture::initialize()
    {
        if (isLoaded() || m_textureID != 0)
        {
            unload();
        }

        // Generate texture ID
        glGenTextures(1, &m_textureID);
        if (m_textureID == 0)
        {
            Log::error("Texture::initialize - Failed to generate texture ID");
            return false;
        }

        // Set default values
        m_width = 0;
        m_height = 0;
        m_format = TextureFormat::RGBA8;
        m_channels = 4;
        m_hasMipmaps = false;
        m_hasPixelData = false;
        m_multisampleCount = 0;
        m_pixelData.reset();
        m_loaded = true;

        return true;
    }

    bool Texture::initialize(int width, int height, TextureFormat format, int multisampleCount)
    {
        if (width <= 0 || height <= 0)
        {
            Log::error("Texture::createEmpty - Invalid dimensions: " +
                       std::to_string(width) + "x" + std::to_string(height));
            return false;
        }

        unload();

        m_width = width;
        m_height = height;
        m_format = format;
        m_channels = getFormatChannelCount(format);

        // Generate texture
        glGenTextures(1, &m_textureID);
        if (m_textureID == 0)
        {
            Log::error("Texture::createEmpty - Failed to generate texture ID");
            return false;
        }

        glBindTexture(GL_TEXTURE_2D, m_textureID);

        // Determine OpenGL format
        uint32_t internalFormat, dataFormat, dataType;
        convertFormat(format, internalFormat, dataFormat, dataType);

        // Allocate storage
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, dataType, nullptr);

        // Set texture parameters
        setTextureParameters();

        glBindTexture(GL_TEXTURE_2D, 0);

        // Allocate pixel data for CPU-side access
        m_pixelData.reset(new uint8_t[width * height * m_channels]);
        std::memset(m_pixelData.get(), 0, width * height * m_channels);
        m_hasPixelData = true;
        m_multisampleCount = multisampleCount;
        m_loaded = true;

        return true;
    }

    bool Texture::initialize(int width, int height, TextureFormat format)
    {
        if (width <= 0 || height <= 0)
        {
            Log::error("Texture::createEmpty - Invalid dimensions: " +
                       std::to_string(width) + "x" + std::to_string(height));
            return false;
        }

        unload();

        m_width = width;
        m_height = height;
        m_format = format;
        m_channels = getFormatChannelCount(format);

        // Generate texture
        glGenTextures(1, &m_textureID);
        if (m_textureID == 0)
        {
            Log::error("Texture::createEmpty - Failed to generate texture ID");
            return false;
        }

        glBindTexture(GL_TEXTURE_2D, m_textureID);

        // Determine OpenGL format
        uint32_t internalFormat, dataFormat, dataType;
        convertFormat(format, internalFormat, dataFormat, dataType);

        // Allocate storage
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, dataType, nullptr);

        // Set texture parameters
        setTextureParameters();

        glBindTexture(GL_TEXTURE_2D, 0);

        // Allocate pixel data for CPU-side access
        m_pixelData.reset(new uint8_t[width * height * m_channels]);
        std::memset(m_pixelData.get(), 0, width * height * m_channels);
        m_hasPixelData = true;
        m_loaded = true;

        return true;
    }

    bool Texture::initialize(int width, int height, TextureFormat format, const void* data)
    {
        if (width <= 0 || height <= 0 || data == nullptr)
        {
            Log::error("Texture::createFromData - Invalid parameters");
            return false;
        }

        unload();

        m_width = width;
        m_height = height;
        m_format = format;
        m_channels = getFormatChannelCount(format);

        // Generate texture
        glGenTextures(1, &m_textureID);
        if (m_textureID == 0)
        {
            Log::error("Texture::createFromData - Failed to generate texture ID");
            return false;
        }

        glBindTexture(GL_TEXTURE_2D, m_textureID);

        // Determine OpenGL format
        uint32_t internalFormat, dataFormat, dataType;
        convertFormat(format, internalFormat, dataFormat, dataType);

        // Upload data
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, dataType, data);

        // Set texture parameters
        setTextureParameters();

        glBindTexture(GL_TEXTURE_2D, 0);

        // Keep a copy of the pixel data for CPU-side access
        m_pixelData.reset(new uint8_t[width * height * m_channels]);
        std::memcpy(m_pixelData.get(), data, width * height * m_channels);
        m_hasPixelData = true;
        m_loaded = true;

        return true;
    }

    void Texture::setData(const void* data)
    {


        // Keep a copy of the pixel data for CPU-side access
        m_pixelData.reset(new uint8_t[m_width * m_height * m_channels]);
        std::memcpy(m_pixelData.get(), data, m_width * m_height * m_channels);
        m_hasPixelData = true;
    }

    bool Texture::load()
    {
        if (isLoaded())
        {
            return true;
        }

        std::string filepath = getPath();
        if (filepath.empty())
        {
            Log::error("Texture::load - Empty path for texture: " + getName());
            return false;
        }

        bool success = loadFromFile(filepath);
        if (success)
        {
            m_loaded = true;
        }

        return success;
    }

    void Texture::unload()
    {
        if (!isLoaded())
        {
            return;
        }

        if (m_textureID != 0)
        {
            glDeleteTextures(1, &m_textureID);
            m_textureID = 0;
        }

        m_pixelData.reset();
        m_hasPixelData = false;
        m_width = 0;
        m_height = 0;
        m_hasMipmaps = false;
        m_loaded = false;
    }

    bool Texture::onReload()
    {
        // Store current settings
        TextureFilterMode filterMode = m_filterMode;
        TextureWrapMode wrapMode = m_wrapMode;
        bool pixelGridAligned = m_pixelGridAligned;

        // Unload and reload
        unload();
        bool success = load();

        // Restore settings if successful
        if (success)
        {
            setFilterMode(filterMode);
            setWrapMode(wrapMode);
            setPixelGridAlignment(pixelGridAligned);
        }

        return success;
    }

    void Texture::bind(uint32_t textureUnit)
    {
        if (!isLoaded())
        {
            load();
        }

        if (m_textureID == 0)
        {
            Log::warn("Texture::bind - Invalid texture ID for: " + getName());
            return;
        }

        glActiveTexture(GL_TEXTURE0 + textureUnit);

        if (m_multisampleCount > 0)
        {
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_textureID);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, m_textureID);
        }
    }

    void Texture::unbind()
    {
        if (m_multisampleCount > 0)
        {
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    void Texture::setPixelGridAlignment(bool enabled)
    {
        m_pixelGridAligned = enabled;

        if (m_textureID != 0)
        {
            bind();

            if (enabled)
            {
                // For pixel art, we force nearest filtering
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                m_hasMipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST);
            }
            else
            {
                // Restore current filter mode
                setFilterMode(m_filterMode);
            }

            unbind();
        }
    }

    bool Texture::isPixelGridAligned() const
    {
        return m_pixelGridAligned;
    }

    void Texture::setFilterMode(TextureFilterMode mode)
    {
        m_filterMode = mode;

        if (m_textureID != 0 && !m_pixelGridAligned)
        {
            bind();

            GLint minFilter, magFilter;

            switch (mode)
            {
                case TextureFilterMode::Nearest:
                    minFilter = GL_NEAREST;
                    magFilter = GL_NEAREST;
                    break;
                case TextureFilterMode::Linear:
                    minFilter = GL_LINEAR;
                    magFilter = GL_LINEAR;
                    break;
                case TextureFilterMode::NearestMipmapNearest:
                    minFilter = GL_NEAREST_MIPMAP_NEAREST;
                    magFilter = GL_NEAREST;
                    break;
                case TextureFilterMode::LinearMipmapNearest:
                    minFilter = GL_LINEAR_MIPMAP_NEAREST;
                    magFilter = GL_LINEAR;
                    break;
                case TextureFilterMode::NearestMipmapLinear:
                    minFilter = GL_NEAREST_MIPMAP_LINEAR;
                    magFilter = GL_NEAREST;
                    break;
                case TextureFilterMode::LinearMipmapLinear:
                    minFilter = GL_LINEAR_MIPMAP_LINEAR;
                    magFilter = GL_LINEAR;
                    break;
                default:
                    minFilter = GL_NEAREST;
                    magFilter = GL_NEAREST;
            }

            // If mipmaps are not generated but mipmap filtering is requested, use non-mipmap version
            if (!m_hasMipmaps && (minFilter == GL_NEAREST_MIPMAP_NEAREST ||
                                  minFilter == GL_LINEAR_MIPMAP_NEAREST ||
                                  minFilter == GL_NEAREST_MIPMAP_LINEAR ||
                                  minFilter == GL_LINEAR_MIPMAP_LINEAR))
            {
                minFilter = (minFilter == GL_NEAREST_MIPMAP_NEAREST || minFilter == GL_NEAREST_MIPMAP_LINEAR)
                    ? GL_NEAREST : GL_LINEAR;
            }

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

            unbind();
        }
    }

    void Texture::setWrapMode(TextureWrapMode mode)
    {
        m_wrapMode = mode;

        if (m_textureID != 0)
        {
            bind();

            GLint wrapParam;
            switch (mode)
            {
                case TextureWrapMode::Repeat:
                    wrapParam = GL_REPEAT;
                    break;
                case TextureWrapMode::MirroredRepeat:
                    wrapParam = GL_MIRRORED_REPEAT;
                    break;
                case TextureWrapMode::ClampToEdge:
                    wrapParam = GL_CLAMP_TO_EDGE;
                    break;
                case TextureWrapMode::ClampToBorder:
                    wrapParam = GL_CLAMP_TO_BORDER;
                    break;
                default:
                    wrapParam = GL_CLAMP_TO_EDGE;
            }

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapParam);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapParam);

            unbind();
        }
    }

    TextureFilterMode Texture::getFilterMode() const
    {
        return m_filterMode;
    }

    TextureWrapMode Texture::getWrapMode() const
    {
        return m_wrapMode;
    }

    bool Texture::setMultisampleCount(int samples)
    {
        // Validate input
        if (samples < 0)
        {
            Log::error("Texture::setMultisampleCount - Invalid sample count: " +
                                std::to_string(samples));
            return false;
        }

        // No change needed
        if (m_multisampleCount == samples)
        {
            return true;
        }

        // Store old state
        int oldSamples = m_multisampleCount;
        m_multisampleCount = samples;

        // If we have dimensions, we need to recreate the texture
        if (m_width > 0 && m_height > 0)
        {
            // Multisampled textures can't have CPU-side data
            std::unique_ptr<uint8_t[]> oldData;

            if (m_hasPixelData && oldSamples == 0 && samples == 0)
            {
                // Preserve data only when staying non-multisampled
                oldData = std::move(m_pixelData);
            }

            // Recreate the texture with new sample count
            if (m_textureID != 0)
            {
                glDeleteTextures(1, &m_textureID);
                m_textureID = 0;
            }

            glGenTextures(1, &m_textureID);
            if (m_textureID == 0)
            {
                Log::error("Texture::setMultisampleCount - Failed to generate texture ID");
                return false;
            }

            // Determine OpenGL format
            uint32_t internalFormat, dataFormat, dataType;
            convertFormat(m_format, internalFormat, dataFormat, dataType);

            // Allocate texture storage based on new multisample status
            if (samples > 0)
            {
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_textureID);
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples,
                                        internalFormat, m_width, m_height, GL_TRUE);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, m_textureID);
                glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height,
                             0, dataFormat, dataType, nullptr);
            }

            // Set texture parameters (if not multisampled)
            if (samples == 0)
            {
                setTextureParameters();

                // Restore data if we had it before
                if (oldData)
                {
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height,
                                    dataFormat, dataType, oldData.get());
                    m_pixelData = std::move(oldData);
                    m_hasPixelData = true;
                }
                else
                {
                    // Create new pixel data for non-multisampled textures
                    m_pixelData.reset(new uint8_t[m_width * m_height * m_channels]);
                    std::memset(m_pixelData.get(), 0, m_width * m_height * m_channels);
                    m_hasPixelData = true;
                }
            }
            else
            {
                // Multisampled textures can't have CPU-side data
                m_pixelData.reset();
                m_hasPixelData = false;
            }

            unbind();
        }

        // Multisampled textures can't have mipmaps
        if (samples > 0)
        {
            m_hasMipmaps = false;
        }

        return true;
    }

    void Texture::generateMipmaps()
    {
        if (m_textureID == 0 || m_hasMipmaps || m_multisampleCount > 0)
        {
            if (m_multisampleCount > 0)
            {
                Log::warn("Texture::generateMipmaps - Cannot generate mipmaps for multisampled textures");
            }
            return;
        }

        bind();
        glGenerateMipmap(GL_TEXTURE_2D);
        m_hasMipmaps = true;
        unbind();

        // Update filter mode to account for mipmaps
        setFilterMode(m_filterMode);
    }

    bool Texture::hasMipmaps() const
    {
        return m_hasMipmaps;
    }

    bool Texture::resize(int width, int height, bool preserveData)
    {
        if (width <= 0 || height <= 0)
        {
            Log::error("Texture::setSize - Invalid dimensions: " +
                                std::to_string(width) + "x" + std::to_string(height));
            return false;
        }

        // Nothing to do if dimensions are the same
        if (m_width == width && m_height == height)
        {
            return true;
        }

        // Store old data if needed
        std::unique_ptr<uint8_t[]> oldData;
        int oldWidth = m_width;
        int oldHeight = m_height;
        int oldChannels = m_channels;

        if (preserveData && m_hasPixelData && m_width > 0 && m_height > 0)
        {
            oldData = std::move(m_pixelData);
        }

        // Create new texture storage
        if (m_textureID == 0 && !initialize())
        {
            return false;
        }

        bind();

        // Determine OpenGL format
        uint32_t internalFormat, dataFormat, dataType;
        convertFormat(m_format, internalFormat, dataFormat, dataType);

        // Allocate texture storage based on multisample status
        if (m_multisampleCount > 0)
        {
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_multisampleCount,
                                    internalFormat, width, height, GL_TRUE);
        }
        else
        {
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height,
                         0, dataFormat, dataType, nullptr);
        }

        m_width = width;
        m_height = height;

        // Set texture parameters
        setTextureParameters();

        // Restore data if needed
        if (oldData)
        {
            m_pixelData.reset(new uint8_t[width * height * m_channels]);
            m_hasPixelData = true;

            // Initialize new data to zero
            std::memset(m_pixelData.get(), 0, width * height * m_channels);

            // Copy old data, taking into account possibly different dimensions
            int copyWidth = std::min(width, oldWidth);
            int copyHeight = std::min(height, oldHeight);

            for (int y = 0; y < copyHeight; ++y)
            {
                for (int x = 0; x < copyWidth; ++x)
                {
                    int oldIndex = (y * oldWidth + x) * oldChannels;
                    int newIndex = (y * width + x) * m_channels;

                    // Copy available channels
                    for (int c = 0; c < std::min(oldChannels, m_channels); ++c)
                    {
                        m_pixelData[newIndex + c] = oldData[oldIndex + c];
                    }
                }
            }

            // Upload the restored data to GPU
            if (!m_multisampleCount)
            {  // Can't update multisampled textures directly
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height,
                                dataFormat, dataType, m_pixelData.get());
            }
        }
        else if (!m_multisampleCount)
        {
            // Create new empty pixel data (not for multisampled textures)
            m_pixelData.reset(new uint8_t[width * height * m_channels]);
            std::memset(m_pixelData.get(), 0, width * height * m_channels);
            m_hasPixelData = true;
        }

        unbind();

        // Reset mipmaps
        m_hasMipmaps = false;

        return true;
    }

    bool Texture::setFormat(TextureFormat format, bool preserveData)
    {
        if (m_format == format)
        {
            return true;
        }

        // Store old data if needed
        std::unique_ptr<uint8_t[]> oldData;
        int oldChannels = m_channels;

        if (preserveData && m_hasPixelData && m_width > 0 && m_height > 0)
        {
            oldData = std::move(m_pixelData);
        }

        // Update format
        m_format = format;
        m_channels = getFormatChannelCount(format);

        // Create new texture storage with new format
        if (m_width > 0 && m_height > 0)
        {
            if (m_textureID == 0 && !initialize())
            {
                return false;
            }

            bind();

            // Determine OpenGL format
            uint32_t internalFormat, dataFormat, dataType;
            convertFormat(m_format, internalFormat, dataFormat, dataType);

            // Allocate texture storage based on multisample status
            if (m_multisampleCount > 0)
            {
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_multisampleCount,
                                        internalFormat, m_width, m_height, GL_TRUE);
            }
            else
            {
                glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height,
                             0, dataFormat, dataType, nullptr);
            }

            // Set texture parameters
            setTextureParameters();

            // Restore data if needed
            if (oldData && !m_multisampleCount)
            {
                m_pixelData.reset(new uint8_t[m_width * m_height * m_channels]);
                m_hasPixelData = true;

                // Initialize new data to zero
                std::memset(m_pixelData.get(), 0, m_width * m_height * m_channels);

                // Convert and copy old data
                for (int y = 0; y < m_height; ++y)
                {
                    for (int x = 0; x < m_width; ++x)
                    {
                        int oldIndex = (y * m_width + x) * oldChannels;
                        int newIndex = (y * m_width + x) * m_channels;

                        // Extract color from old format
                        glm::vec4 color(0.0f, 0.0f, 0.0f, 1.0f);
                        if (oldChannels >= 1) color.r = oldData[oldIndex] / 255.0f;
                        if (oldChannels >= 2) color.g = oldData[oldIndex + 1] / 255.0f;
                        if (oldChannels >= 3) color.b = oldData[oldIndex + 2] / 255.0f;
                        if (oldChannels >= 4) color.a = oldData[oldIndex + 3] / 255.0f;

                        // Store color in new format
                        if (m_channels >= 1) m_pixelData[newIndex] = static_cast<uint8_t>(color.r * 255.0f);
                        if (m_channels >= 2) m_pixelData[newIndex + 1] = static_cast<uint8_t>(color.g * 255.0f);
                        if (m_channels >= 3) m_pixelData[newIndex + 2] = static_cast<uint8_t>(color.b * 255.0f);
                        if (m_channels >= 4) m_pixelData[newIndex + 3] = static_cast<uint8_t>(color.a * 255.0f);
                    }
                }

                // Upload the restored data to GPU
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height,
                                dataFormat, dataType, m_pixelData.get());
            }
            else if (!m_multisampleCount)
            {
                // Create new empty pixel data
                m_pixelData.reset(new uint8_t[m_width * m_height * m_channels]);
                std::memset(m_pixelData.get(), 0, m_width * m_height * m_channels);
                m_hasPixelData = true;
            }
            else
            {
                // Multisampled textures can't have CPU-side data
                m_pixelData.reset();
                m_hasPixelData = false;
            }

            unbind();
        }

        // Reset mipmaps
        m_hasMipmaps = false;

        return true;
    }

    void Texture::setPixel(int x, int y, const glm::vec4& color)
    {
        if (!isLoaded() || !m_hasPixelData || x < 0 || y < 0 || x >= m_width || y >= m_height || m_multisampleCount > 0)
        {
            if (m_multisampleCount > 0)
            {
                Log::warn("Texture::setPixel - Cannot set pixels on multisampled textures");
            }
            return;
        }

        // Calculate pixel index
        int index = (y * m_width + x) * m_channels;

        // Update pixel data
        if (m_channels >= 1) m_pixelData[index] = static_cast<uint8_t>(color.r * 255.0f);
        if (m_channels >= 2) m_pixelData[index + 1] = static_cast<uint8_t>(color.g * 255.0f);
        if (m_channels >= 3) m_pixelData[index + 2] = static_cast<uint8_t>(color.b * 255.0f);
        if (m_channels >= 4) m_pixelData[index + 3] = static_cast<uint8_t>(color.a * 255.0f);

        // Update the texture on GPU with this single pixel
        update(m_pixelData.get() + index, x, y, 1, 1);
    }

    glm::vec4 Texture::getPixel(int x, int y)
    {
        if (!isLoaded() || !m_hasPixelData || x < 0 || y < 0 || x >= m_width || y >= m_height || m_multisampleCount > 0)
        {
            if (m_multisampleCount > 0)
            {
                Log::warn("Texture::getPixel - Cannot get pixels from multisampled textures");
            }
            return glm::vec4(0.0f);
        }

        // Calculate pixel index
        int index = (y * m_width + x) * m_channels;

        // Read pixel data
        glm::vec4 color(0.0f, 0.0f, 0.0f, 1.0f);
        if (m_channels >= 1) color.r = m_pixelData[index] / 255.0f;
        if (m_channels >= 2) color.g = m_pixelData[index + 1] / 255.0f;
        if (m_channels >= 3) color.b = m_pixelData[index + 2] / 255.0f;
        if (m_channels >= 4) color.a = m_pixelData[index + 3] / 255.0f;

        return color;
    }

    void Texture::update(const void* data, int x, int y, int width, int height)
    {
        if (!isLoaded() || data == nullptr || m_multisampleCount > 0)
        {
            if (m_multisampleCount > 0)
            {
                Log::warn("Texture::update - Cannot update multisampled textures directly");
            }
            return;
        }

        // Use full dimensions if not specified
        if (width <= 0) width = m_width;
        if (height <= 0) height = m_height;

        // Validate coordinates
        x = std::clamp(x, 0, m_width - 1);
        y = std::clamp(y, 0, m_height - 1);
        width = std::min(width, m_width - x);
        height = std::min(height, m_height - y);

        bind();

        // Determine OpenGL format
        uint32_t internalFormat, dataFormat, dataType;
        convertFormat(m_format, internalFormat, dataFormat, dataType);

        // Update the texture data
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, dataFormat, dataType, data);

        // Update local copy if we have one
        if (m_hasPixelData && x == 0 && y == 0 && width == m_width && height == m_height)
        {
            // Full texture update
            std::memcpy(m_pixelData.get(), data, m_width * m_height * m_channels);
        }
        else if (m_hasPixelData)
        {
            // Partial update - copy region by region
            const uint8_t* srcData = static_cast<const uint8_t*>(data);
            for (int row = 0; row < height; ++row)
            {
                for (int col = 0; col < width; ++col)
                {
                    int srcIndex = (row * width + col) * m_channels;
                    int dstIndex = ((y + row) * m_width + (x + col)) * m_channels;

                    for (int c = 0; c < m_channels; ++c)
                    {
                        m_pixelData[dstIndex + c] = srcData[srcIndex + c];
                    }
                }
            }
        }

        // Regenerate mipmaps if necessary
        if (m_hasMipmaps)
        {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        unbind();
    }

    void Texture::applyPaletteMapping(std::shared_ptr<PaletteManager> paletteManager)
    {
        if (!isLoaded() || !m_hasPixelData || !paletteManager)
        {
            return;
        }

        // Process each pixel
        for (int y = 0; y < m_height; ++y)
        {
            for (int x = 0; x < m_width; ++x)
            {
                glm::vec4 color = getPixel(x, y);
                glm::vec4 mappedColor = paletteManager->findNearestColor(color);
                setPixel(x, y, mappedColor);
            }
        }
    }

    uint32_t Texture::getID() const
    {
        return m_textureID;
    }

    int Texture::getWidth() const
    {
        return m_width;
    }

    int Texture::getHeight() const
    {
        return m_height;
    }

    TextureFormat Texture::getFormat() const
    {
        return m_format;
    }

    bool Texture::loadFromFile(const std::string& filepath)
    {
        // Use stb_image to load the file
        int width, height, channels;
        stbi_set_flip_vertically_on_load(true);

        unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);
        if (!data)
        {
            Log::error("Texture::loadFromFile - Failed to load image: " + filepath);
            return false;
        }

        // Determine format based on channels
        TextureFormat format;
        switch (channels)
        {
            case 1: format = TextureFormat::R8; break;
            case 2: format = TextureFormat::RG8; break;
            case 3: format = TextureFormat::RGB8; break;
            case 4: format = TextureFormat::RGBA8; break;
            default: format = TextureFormat::RGBA8;
        }

        // Create texture from loaded data
        bool result = initialize(width, height, format, data);

        // Free the loaded data
        stbi_image_free(data);

        return result;
    }

    void Texture::setTextureParameters()
    {
        // Multisampled textures don't support texture parameters
        if (m_multisampleCount > 0)
        {
            return;
        }
        
        // Set filtering and wrapping modes
        setFilterMode(m_filterMode);
        setWrapMode(m_wrapMode);

        // Apply pixel grid alignment if needed
        setPixelGridAlignment(m_pixelGridAligned);
    }

    int Texture::getFormatChannelCount(TextureFormat format)
    {
        switch (format)
        {
            // Single channel formats
            case TextureFormat::R8:
            case TextureFormat::R16F:
            case TextureFormat::R32F:
            case TextureFormat::R8I:
            case TextureFormat::R16I:
            case TextureFormat::R8UI:
            case TextureFormat::R16UI:
            case TextureFormat::Depth16:
            case TextureFormat::Depth24:
            case TextureFormat::Depth32F:
            case TextureFormat::Stencil8:
                return 1;

                // Dual channel formats
            case TextureFormat::RG8:
            case TextureFormat::RG16F:
            case TextureFormat::RG32F:
            case TextureFormat::RG8I:
            case TextureFormat::RG16I:
            case TextureFormat::RG8UI:
            case TextureFormat::RG16UI:
                return 2;

                // Three channel formats
            case TextureFormat::RGB8:
            case TextureFormat::RGB16F:
            case TextureFormat::RGB32F:
            case TextureFormat::RGB8I:
            case TextureFormat::RGB16I:
            case TextureFormat::RGB8UI:
            case TextureFormat::RGB16UI:
            case TextureFormat::R11F_G11F_B10F:
            case TextureFormat::RGB9_E5:
            case TextureFormat::SRGB8:
            case TextureFormat::DXT1_RGB:
            case TextureFormat::ETC2_RGB:
                return 3;

                // Four channel formats
            case TextureFormat::RGBA8:
            case TextureFormat::RGBA16F:
            case TextureFormat::RGBA32F:
            case TextureFormat::RGBA8I:
            case TextureFormat::RGBA16I:
            case TextureFormat::RGBA8UI:
            case TextureFormat::RGBA16UI:
            case TextureFormat::RGB10_A2:
            case TextureFormat::SRGB8_A8:
            case TextureFormat::Depth24Stencil8:
            case TextureFormat::DXT1_RGBA:
            case TextureFormat::DXT3:
            case TextureFormat::DXT5:
            case TextureFormat::ETC2_RGBA:
            case TextureFormat::ASTC_4x4:
            case TextureFormat::ASTC_8x8:
                return 4;

            default:
                return 4;
        }
    }

    void Texture::convertFormat(TextureFormat format, uint32_t& internalFormat, uint32_t& dataFormat, uint32_t& dataType)
    {
        switch (format)
        {
            // Standard formats
            case TextureFormat::R8:
                internalFormat = GL_R8;
                dataFormat = GL_RED;
                dataType = GL_UNSIGNED_BYTE;
                break;
            case TextureFormat::RG8:
                internalFormat = GL_RG8;
                dataFormat = GL_RG;
                dataType = GL_UNSIGNED_BYTE;
                break;
            case TextureFormat::RGB8:
                internalFormat = GL_RGB8;
                dataFormat = GL_RGB;
                dataType = GL_UNSIGNED_BYTE;
                break;
            case TextureFormat::RGBA8:
                internalFormat = GL_RGBA8;
                dataFormat = GL_RGBA;
                dataType = GL_UNSIGNED_BYTE;
                break;

                // Float formats (16-bit)
            case TextureFormat::R16F:
                internalFormat = GL_R16F;
                dataFormat = GL_RED;
                dataType = GL_HALF_FLOAT;
                break;
            case TextureFormat::RG16F:
                internalFormat = GL_RG16F;
                dataFormat = GL_RG;
                dataType = GL_HALF_FLOAT;
                break;
            case TextureFormat::RGB16F:
                internalFormat = GL_RGB16F;
                dataFormat = GL_RGB;
                dataType = GL_HALF_FLOAT;
                break;
            case TextureFormat::RGBA16F:
                internalFormat = GL_RGBA16F;
                dataFormat = GL_RGBA;
                dataType = GL_HALF_FLOAT;
                break;

                // Float formats (32-bit)
            case TextureFormat::R32F:
                internalFormat = GL_R32F;
                dataFormat = GL_RED;
                dataType = GL_FLOAT;
                break;
            case TextureFormat::RG32F:
                internalFormat = GL_RG32F;
                dataFormat = GL_RG;
                dataType = GL_FLOAT;
                break;
            case TextureFormat::RGB32F:
                internalFormat = GL_RGB32F;
                dataFormat = GL_RGB;
                dataType = GL_FLOAT;
                break;
            case TextureFormat::RGBA32F:
                internalFormat = GL_RGBA32F;
                dataFormat = GL_RGBA;
                dataType = GL_FLOAT;
                break;

                // Special float formats
            case TextureFormat::R11F_G11F_B10F:
                internalFormat = GL_R11F_G11F_B10F;
                dataFormat = GL_RGB;
                dataType = GL_UNSIGNED_INT_10F_11F_11F_REV;
                break;
            case TextureFormat::RGB9_E5:
                internalFormat = GL_RGB9_E5;
                dataFormat = GL_RGB;
                dataType = GL_UNSIGNED_INT_5_9_9_9_REV;
                break;

                // Integer formats
            case TextureFormat::R8I:
                internalFormat = GL_R8I;
                dataFormat = GL_RED_INTEGER;
                dataType = GL_BYTE;
                break;
            case TextureFormat::RG8I:
                internalFormat = GL_RG8I;
                dataFormat = GL_RG_INTEGER;
                dataType = GL_BYTE;
                break;
            case TextureFormat::RGB8I:
                internalFormat = GL_RGB8I;
                dataFormat = GL_RGB_INTEGER;
                dataType = GL_BYTE;
                break;
            case TextureFormat::RGBA8I:
                internalFormat = GL_RGBA8I;
                dataFormat = GL_RGBA_INTEGER;
                dataType = GL_BYTE;
                break;
            case TextureFormat::R16I:
                internalFormat = GL_R16I;
                dataFormat = GL_RED_INTEGER;
                dataType = GL_SHORT;
                break;
            case TextureFormat::RG16I:
                internalFormat = GL_RG16I;
                dataFormat = GL_RG_INTEGER;
                dataType = GL_SHORT;
                break;
            case TextureFormat::RGB16I:
                internalFormat = GL_RGB16I;
                dataFormat = GL_RGB_INTEGER;
                dataType = GL_SHORT;
                break;
            case TextureFormat::RGBA16I:
                internalFormat = GL_RGBA16I;
                dataFormat = GL_RGBA_INTEGER;
                dataType = GL_SHORT;
                break;

                // Unsigned integer formats
            case TextureFormat::R8UI:
                internalFormat = GL_R8UI;
                dataFormat = GL_RED_INTEGER;
                dataType = GL_UNSIGNED_BYTE;
                break;
            case TextureFormat::RG8UI:
                internalFormat = GL_RG8UI;
                dataFormat = GL_RG_INTEGER;
                dataType = GL_UNSIGNED_BYTE;
                break;
            case TextureFormat::RGB8UI:
                internalFormat = GL_RGB8UI;
                dataFormat = GL_RGB_INTEGER;
                dataType = GL_UNSIGNED_BYTE;
                break;
            case TextureFormat::RGBA8UI:
                internalFormat = GL_RGBA8UI;
                dataFormat = GL_RGBA_INTEGER;
                dataType = GL_UNSIGNED_BYTE;
                break;
            case TextureFormat::R16UI:
                internalFormat = GL_R16UI;
                dataFormat = GL_RED_INTEGER;
                dataType = GL_UNSIGNED_SHORT;
                break;
            case TextureFormat::RG16UI:
                internalFormat = GL_RG16UI;
                dataFormat = GL_RG_INTEGER;
                dataType = GL_UNSIGNED_SHORT;
                break;
            case TextureFormat::RGB16UI:
                internalFormat = GL_RGB16UI;
                dataFormat = GL_RGB_INTEGER;
                dataType = GL_UNSIGNED_SHORT;
                break;
            case TextureFormat::RGBA16UI:
                internalFormat = GL_RGBA16UI;
                dataFormat = GL_RGBA_INTEGER;
                dataType = GL_UNSIGNED_SHORT;
                break;

                // Special formats
            case TextureFormat::RGB10_A2:
                internalFormat = GL_RGB10_A2;
                dataFormat = GL_RGBA;
                dataType = GL_UNSIGNED_INT_2_10_10_10_REV;
                break;

                // sRGB formats
            case TextureFormat::SRGB8:
                internalFormat = GL_SRGB8;
                dataFormat = GL_RGB;
                dataType = GL_UNSIGNED_BYTE;
                break;
            case TextureFormat::SRGB8_A8:
                internalFormat = GL_SRGB8_ALPHA8;
                dataFormat = GL_RGBA;
                dataType = GL_UNSIGNED_BYTE;
                break;

                // Depth/stencil formats
            case TextureFormat::Depth16:
                internalFormat = GL_DEPTH_COMPONENT16;
                dataFormat = GL_DEPTH_COMPONENT;
                dataType = GL_UNSIGNED_SHORT;
                break;
            case TextureFormat::Depth24:
                internalFormat = GL_DEPTH_COMPONENT24;
                dataFormat = GL_DEPTH_COMPONENT;
                dataType = GL_UNSIGNED_INT;
                break;
            case TextureFormat::Depth32F:
                internalFormat = GL_DEPTH_COMPONENT32F;
                dataFormat = GL_DEPTH_COMPONENT;
                dataType = GL_FLOAT;
                break;
            case TextureFormat::Stencil8:
                internalFormat = GL_STENCIL_INDEX8;
                dataFormat = GL_STENCIL_INDEX;
                dataType = GL_UNSIGNED_BYTE;
                break;
            case TextureFormat::Depth24Stencil8:
                internalFormat = GL_DEPTH24_STENCIL8;
                dataFormat = GL_DEPTH_STENCIL;
                dataType = GL_UNSIGNED_INT_24_8;
                break;

                // Compressed formats - DXT
            case TextureFormat::DXT1_RGB:
                internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
                dataFormat = GL_RGB;
                dataType = GL_UNSIGNED_BYTE;
                break;
            case TextureFormat::DXT1_RGBA:
                internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                dataFormat = GL_RGBA;
                dataType = GL_UNSIGNED_BYTE;
                break;
            case TextureFormat::DXT3:
                internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                dataFormat = GL_RGBA;
                dataType = GL_UNSIGNED_BYTE;
                break;
            case TextureFormat::DXT5:
                internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                dataFormat = GL_RGBA;
                dataType = GL_UNSIGNED_BYTE;
                break;

                // Compressed formats - ETC2
            case TextureFormat::ETC2_RGB:
                internalFormat = GL_COMPRESSED_RGB8_ETC2;
                dataFormat = GL_RGB;
                dataType = GL_UNSIGNED_BYTE;
                break;
            case TextureFormat::ETC2_RGBA:
                internalFormat = GL_COMPRESSED_RGBA8_ETC2_EAC;
                dataFormat = GL_RGBA;
                dataType = GL_UNSIGNED_BYTE;
                break;

                // Compressed formats - ASTC
            case TextureFormat::ASTC_4x4:
                internalFormat = GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
                dataFormat = GL_RGBA;
                dataType = GL_UNSIGNED_BYTE;
                break;
            case TextureFormat::ASTC_8x8:
                internalFormat = GL_COMPRESSED_RGBA_ASTC_8x8_KHR;
                dataFormat = GL_RGBA;
                dataType = GL_UNSIGNED_BYTE;
                break;

            default:
                internalFormat = GL_RGBA8;
                dataFormat = GL_RGBA;
                dataType = GL_UNSIGNED_BYTE;
                break;
        }
    }

} // namespace PixelCraft::Rendering