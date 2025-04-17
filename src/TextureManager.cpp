#include "Texture.h"
#include <iostream>

TextureManager::TextureManager() :
    defaultFilterMode(TextureFilterMode::NEAREST),
    defaultWrapMode(TextureWrapMode::REPEAT)
{
    initializeDefaultTextures();
}

TextureManager::~TextureManager()
{
    releaseAll();
}

Texture* TextureManager::getTexture(const std::string& path, bool generateMipmaps, bool sRGB)
{
    // Check if the texture is already loaded
    auto it = textureCache.find(path);
    if (it != textureCache.end())
    {
        return it->second.get();
    }

    // Load the texture
    Texture* texture = Texture::create2D(path, generateMipmaps, sRGB);
    if (!texture)
    {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return nullptr;
    }

    // Apply default settings
    texture->setFilterMode(defaultFilterMode);
    texture->setWrapMode(defaultWrapMode);

    // Add to cache
    textureCache[path] = std::unique_ptr<Texture>(texture);
    return texture;
}

Texture* TextureManager::createTexture(int width, int height, int channels, bool generateMipmaps, bool sRGB)
{
    Texture* texture = Texture::createEmpty(width, height, channels, generateMipmaps, sRGB);
    if (!texture)
    {
        std::cerr << "Failed to create empty texture" << std::endl;
        return nullptr;
    }

    // Apply default settings
    texture->setFilterMode(defaultFilterMode);
    texture->setWrapMode(defaultWrapMode);

    // Add to owned textures
    ownedTextures.push_back(std::unique_ptr<Texture>(texture));
    return texture;
}

void TextureManager::releaseTexture(Texture* texture)
{
    // Remove from cache
    for (auto it = textureCache.begin(); it != textureCache.end(); )
    {
        if (it->second.get() == texture)
        {
            it = textureCache.erase(it);
        }
        else
        {
            it++;
        }
    }

    // Remove from owned textures
    for (auto it = ownedTextures.begin(); it != ownedTextures.end(); )
    {
        if (it->get() == texture)
        {
            it = ownedTextures.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void TextureManager::releaseUnused()
{
    // This is a simple implementation - could be enhanced with reference counting
    // Currently, it doesn't actually do anything since we don't track usage
}

void TextureManager::releaseAll()
{
    textureCache.clear();
    ownedTextures.clear();

    // Re-initialize default textures
    defaultAlbedo.reset();
    defaultNormal.reset();
    defaultMetallicRoughness.reset();

    initializeDefaultTextures();
}

Texture* TextureManager::getDefaultAlbedo()
{
    return defaultAlbedo.get();
}

Texture* TextureManager::getDefaultNormal()
{
    return defaultNormal.get();
}

Texture* TextureManager::getDefaultMetallicRoughness()
{
    return defaultMetallicRoughness.get();
}

void TextureManager::initializeDefaultTextures()
{
    // Create default white texture (albedo)
    defaultAlbedo.reset(Texture::createEmpty(4, 4, 4, false, false));
    createDefaultTexture(defaultAlbedo, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // Create default normal map (pointing up)
    defaultNormal.reset(Texture::createEmpty(4, 4, 4, false, false));
    createDefaultTexture(defaultNormal, glm::vec4(0.5f, 0.5f, 1.0f, 1.0f));

    // Create default metallic-roughness map (non-metallic, medium roughness)
    defaultMetallicRoughness.reset(Texture::createEmpty(4, 4, 4, false, false));
    createDefaultTexture(defaultMetallicRoughness, glm::vec4(0.0f, 0.5f, 0.0f, 1.0f));
}

void TextureManager::setDefaultFilterMode(TextureFilterMode mode)
{
    defaultFilterMode = mode;
}

void TextureManager::setDefaultWrapMode(TextureWrapMode mode)
{
    defaultWrapMode = mode;
}

size_t TextureManager::getTextureCount() const
{
    return textureCache.size() + ownedTextures.size();
}

size_t TextureManager::getTotalTextureMemory() const
{
    size_t totalMemory = 0;

    // Calculate memory for cached textures
    for (const auto& pair : textureCache)
    {
        totalMemory += calculateTextureMemory(pair.second.get());
    }

    // Calculate memory for owned textures
    for (const auto& texture : ownedTextures)
    {
        totalMemory += calculateTextureMemory(texture.get());
    }

    return totalMemory;
}

void TextureManager::createDefaultTexture(std::unique_ptr<Texture>& texture, const glm::vec4& color)
{
    if (!texture)
    {
        return;
    }

    // Fill texture with solid color
    int width = texture->getWidth();
    int height = texture->getHeight();
    int channels = texture->getChannels();

    std::unique_ptr<unsigned char[]> data = std::make_unique<unsigned char[]>(width * height * channels);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int index = (y * width + x) * channels;

            for (int c = 0; c < channels && c < 4; c++)
            {
                data[index + c] = static_cast<unsigned char>(color[c] * 255.0f);
            }
        }
    }

    // Set texture data
    texture->setData(data.get(), width, height, channels);
}

unsigned int TextureManager::calculateTextureMemory(const Texture* texture) const
{
    if (!texture)
    {
        return 0;
    }

    unsigned int baseSize = texture->getWidth() * texture->getHeight() * texture->getChannels();

    // If mipmaps are enabled, account for the full mipmap chain
    if (texture->hasMipmaps())
    {
        // Mipmap series is a geometric series with r=1/4 (each level is 1/4 the size of the previous)
        // Sum of geometric series with r=1/4: S = a/(1-r) = a/(1-0.25) = a/0.75 = a*4/3
        return static_cast<unsigned int>(baseSize * 4.0f / 3.0f);
    }

    return baseSize;
}