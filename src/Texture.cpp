#include "Texture.h"
#include <stb_image.h>
#include <stb_image_write.h>
#include <iostream>
#include <algorithm>
#include <cmath>

// Static creation methods

Texture* Texture::create2D(const std::string& path, bool generateMipmaps, bool sRGB)
{
    Texture* texture = new Texture();

    // Load image using stb_image
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

    if (!data)
    {
        std::cerr << "Failed to load texture: " << path << std::endl;
        delete texture;
        return nullptr;
    }

    // Set texture properties
    texture->width = width;
    texture->height = height;
    texture->channels = channels;
    texture->mipmapsGenerated = false;
    texture->sRGB = sRGB;

    // Generate OpenGL texture
    glGenTextures(1, &texture->textureID);
    glBindTexture(GL_TEXTURE_2D, texture->textureID);

    // Determine format based on channels
    GLenum internalFormat, format;
    if (channels == 1)
    {
        internalFormat = GL_R8;
        format = GL_RED;
    }
    else if (channels == 3)
    {
        internalFormat = sRGB ? GL_SRGB8 : GL_RGB8;
        format = GL_RGB;
    }
    else if (channels == 4)
    {
        internalFormat = sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        format = GL_RGBA;
    }
    else
    {
        std::cerr << "Unsupported number of channels: " << channels << std::endl;
        stbi_image_free(data);
        delete texture;
        return nullptr;
    }

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);

    // Generate mipmaps if requested
    if (generateMipmaps)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
        texture->mipmapsGenerated = true;
    }

    // Set default filter and wrap nodes for pixel art
    texture->setFilterMode(TextureFilterMode::NEAREST);
    texture->setWrapMode(TextureWrapMode::REPEAT);
    texture->pixelGridAligned = true;

    // Create a copy of data for CPU-side operations
    size_t dataSize = width * height * channels;
    texture->textureData = std::make_unique<unsigned char[]>(dataSize);
    memcpy(texture->textureData.get(), data, dataSize);
    texture->dataIsDirty = false;

    // Free the original data
    stbi_image_free(data);

    return texture;
}

Texture* Texture::createFromMemory(unsigned char* data, int width, int height, int channels,
                                   bool generateMipmaps, bool sRGB)
{
    if (!data)
    {
        std::cerr << "Null data provided to createFromMemory" << std::endl;
        return nullptr;
    }

    Texture* texture = new Texture();

    // Set texture properties
    texture->width = width;
    texture->height = height;
    texture->channels = channels;
    texture->mipmapsGenerated = false;
    texture->sRGB = sRGB;

    // Generate OpenGL texture
    glGenTextures(1, &texture->textureID);
    glBindTexture(GL_TEXTURE_2D, texture->textureID);

    // Determine format based on channels
    GLenum internalFormat, format;
    if (channels == 1)
    {
        internalFormat = GL_R8;
        format = GL_RED;
    }
    else if (channels == 3)
    {
        internalFormat = sRGB ? GL_SRGB8 : GL_RGB8;
        format = GL_RGB;
    }
    else if (channels == 4)
    {
        internalFormat = sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        format = GL_RGBA;
    }
    else
    {
        std::cerr << "Unsupported number of channels: " << channels << std::endl;
        delete texture;
        return nullptr;
    }

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);

    // Generate mipmaps if requested
    if (generateMipmaps)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
        texture->mipmapsGenerated = true;
    }

    // Set default filter and wrap nodes for pixel art
    texture->setFilterMode(TextureFilterMode::NEAREST);
    texture->setWrapMode(TextureWrapMode::REPEAT);
    texture->pixelGridAligned = true;

    // Create a copy of data for CPU-side operations
    size_t dataSize = width * height * channels;
    texture->textureData = std::make_unique<unsigned char[]>(dataSize);
    memcpy(texture->textureData.get(), data, dataSize);
    texture->dataIsDirty = false;

    return texture;
}

Texture* Texture::createEmpty(int width, int height, int channels, bool generateMipmaps, bool sRGB)
{
    Texture* texture = new Texture();

    // Set texture properties
    texture->width = width;
    texture->height = height;
    texture->channels = channels;
    texture->mipmapsGenerated = false;
    texture->sRGB = sRGB;

    // Generate OpenGL texture
    glGenTextures(1, &texture->textureID);
    glBindTexture(GL_TEXTURE_2D, texture->textureID);

    // Determine format based on channels
    GLenum internalFormat, format;
    if (channels == 1)
    {
        internalFormat = GL_R8;
        format = GL_RED;
    }
    else if (channels == 3)
    {
        internalFormat = sRGB ? GL_SRGB8 : GL_RGB8;
        format = GL_RGB;
    }
    else if (channels == 4)
    {
        internalFormat = sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        format = GL_RGBA;
    }
    else
    {
        std::cerr << "Unsupported number of channels: " << channels << std::endl;
        delete texture;
        return nullptr;
    }

    // Create empty texture
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);

    // Generate mipmaps if requested
    if (generateMipmaps)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
        texture->mipmapsGenerated = true;
    }

    // Set default filter and wrap nodes for pixel art
    texture->setFilterMode(TextureFilterMode::NEAREST);
    texture->setWrapMode(TextureWrapMode::REPEAT);
    texture->pixelGridAligned = true;

    // Allocate memory for CPU-side operations
    size_t dataSize = width * height * channels;
    texture->textureData = std::make_unique<unsigned char[]>(dataSize);
    memset(texture->textureData.get(), 0, dataSize);
    texture->dataIsDirty = true;

    return texture;
}

Texture::Texture() :
    textureID(0), width(0), height(0), channels(0),
    mipmapsGenerated(false), pixelGridAligned(true),
    filterMode(TextureFilterMode::NEAREST),
    wrapMode(TextureWrapMode::REPEAT),
    sRGB(false), dataIsDirty(false)
{
}

Texture::~Texture()
{
    if (textureID)
    {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }
}

void Texture::bind(unsigned int slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, textureID);
}

void Texture::setFilterMode(TextureFilterMode mode)
{
    if (filterMode == mode)
    {
        return;
    }

    filterMode = mode;
    updateTextureParameters();
}

void Texture::setWrapMode(TextureWrapMode mode)
{
    if (wrapMode == mode)
    {
        return;
    }

    wrapMode = mode;
    updateTextureParameters();
}

void Texture::setPixelGridAlignment(bool aligned)
{
    pixelGridAligned = aligned;

    // If we're setting pixel grid alignment, update filtering to nearest
    if (aligned)
    {
        setFilterMode(TextureFilterMode::NEAREST);
    }
}

void Texture::generateMipmaps()
{
    if (mipmapsGenerated)
    {
        return;
    }

    glBindTexture(GL_TEXTURE_2D, textureID);
    glGenerateMipmap(GL_TEXTURE_2D);
    mipmapsGenerated = true;

    // Update filtering based on current mode to account for mipmaps
    updateTextureParameters();
}

void Texture::resize(int newWidth, int newHeight)
{
    if (width == newWidth && height == newHeight)
    {
        return;
    }

    // Save old properties
    int oldWidth = width;
    int oldHeight = height;

    // Create a new texture with the new dimensions
    GLuint newTextureID;
    glGenTextures(1, &newTextureID);
    glBindTexture(GL_TEXTURE_2D, newTextureID);
    
    // Determine format based on channels
    GLenum internalFormat, format;
    if (channels == 1)
    {
        internalFormat = GL_R8;
        format = GL_RED;
    }
    else if (channels == 3)
    {
        internalFormat = sRGB ? GL_SRGB8 : GL_RGB8;
        format = GL_RGB;
    }
    else if (channels == 4)
    {
        internalFormat = sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        format = GL_RGBA;
    }
    else
    {
        std::cerr << "Unsupported number of channels in resize: " << channels << std::endl;
        return;
    }

    // Allocate new texture storage
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, newWidth, newHeight, 0, format, GL_UNSIGNED_BYTE, nullptr);

    // If we have data, resize it
    if (textureData)
    {
        // Allocate new data buffer
        std::unique_ptr<unsigned char[]> newData = std::make_unique<unsigned char[]>(newWidth * newHeight * channels);

        // Simple nearest-neighbor scaling for pixel art consistency
        for (int y = 0; y < newHeight; y++)
        {
            for (int x = 0; x < newWidth; x++)
            {
                int srcX = static_cast<int>(x * static_cast<float>(oldWidth) / newWidth);
                int srcY = static_cast<int>(y * static_cast<float>(oldHeight) / newHeight);

                for (int c = 0; c < channels; c++)
                {
                    int srcIdx = (srcY * oldWidth + srcX) * channels + c;
                    int dstIdx = (y * newWidth + x) * channels + c;
                    newData[dstIdx] = textureData[srcIdx];
                }
            }
        }

        // Upload new data
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, newWidth, newHeight, format, GL_UNSIGNED_BYTE, newData.get());

        // Replace old data with new
        textureData = std::move(newData);
    }

    // Apply texture parameters
    updateTextureParameters();

    // Generate mipmaps if needed
    if (mipmapsGenerated)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    // Clean up old texture
    glDeleteTextures(1, &textureID);
    textureID = newTextureID;

    // Update dimensions
    width = newWidth;
    height = newHeight;
    dataIsDirty = false;
}

void Texture::updateTextureParameters()
{
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Set texture wrapping
    GLint wrapParam;
    switch (wrapMode)
    {
        case TextureWrapMode::REPEAT:
            wrapParam = GL_REPEAT;
            break;
        case TextureWrapMode::CLAMP_TO_EDGE:
            wrapParam = GL_CLAMP_TO_EDGE;
            break;
        case TextureWrapMode::MIRRORED_REPEAT:
            wrapParam = GL_MIRRORED_REPEAT;
            break;
        default:
            wrapParam = GL_REPEAT;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapParam);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapParam);

    // Set texture filtering
    GLint minFilter, magFilter;

    switch (filterMode)
    {
        case TextureFilterMode::NEAREST:
            minFilter = GL_NEAREST;
            magFilter = GL_NEAREST;
            break;
        case TextureFilterMode::BILINEAR:
            minFilter = GL_LINEAR;
            magFilter = GL_LINEAR;
            break;
        case TextureFilterMode::NEAREST_WITH_MIPMAPS:
            minFilter = mipmapsGenerated ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
            magFilter = GL_NEAREST;
            break;
        case TextureFilterMode::BILINEAR_WITH_MIPMAPS:
            minFilter = mipmapsGenerated ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
            magFilter = GL_LINEAR;
            break;
        case TextureFilterMode::PIXEL_ART_OPTIMIZED:
            // Special case for pixel art - nearest for mag, but allow mipmapping for min
            minFilter = mipmapsGenerated ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST;
            magFilter = GL_NEAREST;
            break;
        default:
            minFilter = GL_NEAREST;
            magFilter = GL_NEAREST;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
}

void Texture::setData(unsigned char* data, int newWidth, int newHeight, int newChannels)
{
    if (!data)
    {
        std::cerr << "Null data provided to setData" << std::endl;
        return;
    }

    // If dimensions or format changed, recreate the texture
    if (newWidth != width || newHeight != height || newChannels != channels)
    {
        // Delete existing texture
        if (textureID)
        {
            glDeleteTextures(1, &textureID);
        }

        // Update properties
        width = newWidth;
        height = newHeight;
        channels = newChannels;

        // Generate new texture
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        // Determine format based on channels
        GLenum internalFormat, format;
        if (channels == 1)
        {
            internalFormat = GL_R8;
            format = GL_RED;
        }
        else if (channels == 3)
        {
            internalFormat = sRGB ? GL_SRGB8 : GL_RGB8;
            format = GL_RGB;
        }
        else if (channels == 4)
        {
            internalFormat = sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
            format = GL_RGBA;
        }
        else
        {
            std::cerr << "Unsupported number of channels: " << channels << std::endl;
            return;
        }

        // Upload new data
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);

        // Set parameters
        updateTextureParameters();

        // Generate mipmaps if needed
        if (mipmapsGenerated)
        {
            glGenerateMipmap(GL_TEXTURE_2D);
        }
    }
    else
    {
        // Just update the data
        glBindTexture(GL_TEXTURE_2D, textureID);

        GLenum format;
        if (channels == 1)
        {
            format = GL_RED;
        }
        else if (channels == 3)
        {
            format = GL_RGB;
        }
        else if (channels == 4)
        {
            format = GL_RGBA;
        }
        else
        {
            std::cerr << "Unsupported number of channels: " << channels << std::endl;
            return;
        }

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);

        // Update mipmaps if needed
        if (mipmapsGenerated)
        {
            glGenerateMipmap(GL_TEXTURE_2D);
        }
    }

    // Update CPU copy of data
    size_t dataSize = width * height * channels;
    if (!textureData || width * height * channels != dataSize)
    {
        textureData = std::make_unique<unsigned char[]>(dataSize);
    }
    memcpy(textureData.get(), data, dataSize);
    dataIsDirty = false;
}

void Texture::setPixel(int x, int y, const glm::vec4& color)
{
    if (x < 0 || x >= width || y < 0 || y >= height)
    {
        return; // Out of bounds
    }

    // Ensure we have CPU data
    if (!textureData)
    {
        downloadFromGPU();
    }

    // Set the pixel data
    int index = (y * width + x) * channels;

    for (int c = 0; c < channels && c < 4; c++)
    {
        float value = glm::clamp(color[c], 0.0f, 1.0f);
        textureData[index + c] = static_cast<unsigned char>(value * 255.0f);
    }

    dataIsDirty = true;
}

glm::vec4 Texture::getPixel(int x, int y) const
{
    if (x < 0 || x >= width || y < 0 || y >= height)
    {
        return glm::vec4(0.0f); // Out of bounds
    }

    // Ensure we have CPU data
    if (!textureData)
    {
        const_cast<Texture*>(this)->downloadFromGPU();
    }

    // Get the pixel color
    int index = (y * width + x) * channels;
    glm::vec4 color(0.0f, 0.0f, 0.0f, 1.0f);

    for (int c = 0; c < channels && c < 4; c++)
    {
        color[c] = textureData[index + c] / 255.0f;
    }

    return color;
}

void Texture::applyPaletteMapping(const std::vector<glm::vec4>& palette, bool dithering)
{
    if (palette.empty())
    {
        return;
    }

    // Ensure we have CPU data
    if (!textureData)
    {
        downloadFromGPU();
    }

    // Simple ordered dithering matrix (4x4)
    const float ditherMatrix[4][4] = {
        {0.0f, 0.5f, 0.125f, 0.625f},
        {0.75f, 0.25f, 0.875f, 0.375f},
        {0.1875f, 0.6875f, 0.0625f, 0.5625f},
        {0.9375f, 0.4375f, 0.8125f, 0.3125f}
    };

    // Process each pixel
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int index = (y * width + x) * channels;

            // Read original color
            glm::vec4 originalColor(0.0f, 0.0f, 0.0f, 1.0f);
            for (int c = 0; c < channels && c < 4; c++)
            {
                originalColor[c] = textureData[index + c] / 255.0f;
            }

            // Apply dithering offset if enabled
            if (dithering)
            {
                int ditherX = x % 4;
                int ditherY = y % 4;
                float ditherValue = ditherMatrix[ditherY][ditherX] - 0.5f;

                // Apply dither to color components
                for (int c = 0; c < 3; c++)
                { // Don't dither alpha
                    originalColor[c] = glm::clamp(originalColor[c] + ditherValue * 0.1f, 0.0f, 1.0f);
                }
            }

            // Find closest color in palette
            float minDistance = std::numeric_limits<float>::max();
            glm::vec4 closestColor(0.0f);

            for (const auto& paletteColor : palette)
            {
                // Calculate color distance (ignoring alpha)
                float distance = glm::distance(
                    glm::vec3(originalColor.r, originalColor.g, originalColor.b),
                    glm::vec3(paletteColor.r, paletteColor.g, paletteColor.b)
                );

                if (distance < minDistance)
                {
                    minDistance = distance;
                    closestColor = paletteColor;
                    // Use original alpha
                    if (channels > 3)
                    {
                        closestColor.a = originalColor.a;
                    }
                }
            }

            // Write palette color back
            for (int c = 0; c < channels && c < 4; c++)
            {
                textureData[index + c] = static_cast<unsigned char>(closestColor[c] * 255.0f);
            }
        }
    }

    // Upload modified data to GPU
    uploadToGPU();
}

void Texture::uploadToGPU()
{
    if (!dataIsDirty || !textureData)
    {
        return;
    }

    glBindTexture(GL_TEXTURE_2D, textureID);

    GLenum format;
    if (channels == 1)
    {
        format = GL_RED;
    }
    else if (channels == 3)
    {
        format = GL_RGB;
    }
    else if (channels == 4)
    {
        format = GL_RGBA;
    }
    else
    {
        std::cerr << "Unsupported number of channels for upload: " << channels << std::endl;
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, textureData.get());

    // Update mipmaps if needed
    if (mipmapsGenerated)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    dataIsDirty = false;
}

void Texture::downloadFromGPU()
{
    // Allocate memory for texture data if needed
    if (!textureData)
    {
        textureData = std::make_unique<unsigned char[]>(width * height * channels);
    }

    glBindTexture(GL_TEXTURE_2D, textureID);

    GLenum format;
    if (channels == 1)
    {
        format = GL_RED;
    }
    else if (channels == 3)
    {
        format = GL_RGB;
    }
    else if (channels == 4)
    {
        format = GL_RGBA;
    }
    else
    {
        std::cerr << "Unsupported number of channels for download: " << channels << std::endl;
        return;
    }

    // Read pixels from GPU
    GLint previousFramebuffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);

    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureID, 0);

    glReadPixels(0, 0, width, height, format, GL_UNSIGNED_BYTE, textureData.get());

    // Clean up
    glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
    glDeleteFramebuffers(1, &fbo);

    dataIsDirty = false;
}

void Texture::saveToFile(const std::string& path)
{
    // Ensure we have up-to-date CPU data
    if (!textureData || dataIsDirty)
    {
        downloadFromGPU();
    }

    // Write image to file
    stbi_flip_vertically_on_write(true);
    int result;

    // Determine file type from extension
    size_t extPos = path.find_last_of('.');
    if (extPos == std::string::npos)
    {
        std::cerr << "No file extension in path: " << path << std::endl;
        return;
    }

    std::string ext = path.substr(extPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == "png")
    {
        result = stbi_write_png(path.c_str(), width, height, channels, textureData.get(), width * channels);
    }
    else if (ext == "jpg" || ext == "jpeg")
    {
        result = stbi_write_jpg(path.c_str(), width, height, channels, textureData.get(), 90);
    }
    else if (ext == "bmp")
    {
        result = stbi_write_bmp(path.c_str(), width, height, channels, textureData.get());
    }
    else if (ext == "tga")
    {
        result = stbi_write_tga(path.c_str(), width, height, channels, textureData.get());
    }
    else
    {
        std::cerr << "Unsupported file extension: " << ext << std::endl;
        return;
    }

    if (!result)
    {
        std::cerr << "Failed to save texture to: " << path << std::endl;
    }
}

unsigned int Texture::getID() const
{
    return textureID;
}

int Texture::getWidth() const
{
    return width;
}

int Texture::getHeight() const
{
    return height;
}

int Texture::getChannels() const
{
    return channels;
}

TextureFilterMode Texture::getFilterMode() const
{
    return filterMode;
}

TextureWrapMode Texture::getWrapMode() const
{
    return wrapMode;
}

bool Texture::isPixelGridAligned() const
{
    return pixelGridAligned;
}

bool Texture::hasMipmaps() const
{
    return mipmapsGenerated;
}

unsigned char* Texture::getData()
{
    if (!textureData || dataIsDirty)
    {
        downloadFromGPU();
    }
    return textureData.get();
}

const unsigned char* Texture::getData() const
{
    if (!textureData || dataIsDirty)
    {
        const_cast<Texture*>(this)->downloadFromGPU();
    }
    return textureData.get();
}