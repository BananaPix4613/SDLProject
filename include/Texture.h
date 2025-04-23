#pragma once

#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <unordered_map>

// Forward declarations
class TextureManager;

// Enum for pixel art-friendly filtering modes
enum class TextureFilterMode
{
    NEAREST,                // Perfect pixel boundaries - no filtering
    BILINEAR,               // Standard bilinear filtering
    NEAREST_WITH_MIPMAPS,   // Pixel perfect with mipmapping for distance
    BILINEAR_WITH_MIPMAPS,  // Standard trilinear filtering
    PIXEL_ART_OPTIMIZED     // Special filtering designed for pixel art aesthetics
};

// Enum for texture wrapping modes
enum class TextureWrapMode
{
    REPEAT,
    CLAMP_TO_EDGE,
    MIRRORED_REPEAT
};

// Texture class with special handling for pixel art
class Texture
{
public:
    // Creation methods
    static Texture* create2D(const std::string& path, bool generateMipmaps = false, bool sRGB = false);
    static Texture* createFromMemory(unsigned char* data, int width, int height, int channels,
                                     bool generatedMipmaps = false, bool sRGB = false);
    static Texture* createEmpty(int width, int height, int channels = 4,
                                bool generateMipmaps = false, bool sRGB = false);

    ~Texture();

    // Bind texture to specified slot
    void bind(unsigned int slot = 0) const;

    // Pixel art specific settings
    void setFilterMode(TextureFilterMode mode);
    void setWrapMode(TextureWrapMode mode);
    void setPixelGridAlignment(bool aligned);

    // Utility functions
    void generateMipmaps();
    void resize(int width, int height);

    // Data manipulation
    void setData(unsigned char* data, int width, int height, int channels);
    void setPixel(int x, int y, const glm::vec4& color);
    glm::vec4 getPixel(int x, int y) const;

    // Palette handling
    void applyPaletteMapping(const std::vector<glm::vec4>& palette, bool dithering = false);

    // Getters
    unsigned int getID() const;
    int getWidth() const;
    int getHeight() const;
    int getChannels() const;
    TextureFilterMode getFilterMode() const;
    TextureWrapMode getWrapMode() const;
    bool isPixelGridAligned() const;
    bool hasMipmaps() const;

    // Get underlying texture data
    unsigned char* getData();
    const unsigned char* getData() const;

    // Screenshot and debug utility
    void saveToFile(const std::string& path);

private:
    Texture();  // Private constructor, use factory methods

    unsigned int textureID;
    int width;
    int height;
    int channels;
    bool mipmapsGenerated;
    bool pixelGridAligned;
    TextureFilterMode filterMode;
    TextureWrapMode wrapMode;
    bool sRGB;

    // Cached texture data for CPU operations
    std::unique_ptr<unsigned char[]> textureData;
    bool dataIsDirty;

    // Helper methods
    void updateTextureParameters();
    void uploadToGPU();
    void downloadFromGPU();

    // Allow TextureManager to manage this texture
    friend class TextureManager;
};

// Texture manager for resource handling
class TextureManager
{
public:
    TextureManager();
    ~TextureManager();

    // Get or load a texture
    Texture* getTexture(const std::string& path, bool generateMipmaps = false, bool sRGB = false);

    // Create a new texture
    Texture* createTexture(int width, int height, int channels = 4,
                           bool generateMipmaps = false, bool sRGB = false);

    // Release resources
    void releaseTexture(Texture* texture);
    void releaseUnused();
    void releaseAll();

    // Default textures
    Texture* getDefaultAlbedo();
    Texture* getDefaultNormal();
    Texture* getDefaultMetallicRoughness();

    // Initialize default textures
    void initializeDefaultTextures();

    // Global texture settings
    void setDefaultFilterMode(TextureFilterMode mode);
    void setDefaultWrapMode(TextureWrapMode mode);

    // Statistics
    size_t getTextureCount() const;
    size_t getTotalTextureMemory() const;

private:
    std::unordered_map<std::string, std::unique_ptr<Texture>> textureCache;
    std::vector<std::unique_ptr<Texture>> ownedTextures;

    // Default textures
    std::unique_ptr<Texture> defaultAlbedo;
    std::unique_ptr<Texture> defaultNormal;
    std::unique_ptr<Texture> defaultMetallicRoughness;

    // Default settings
    TextureFilterMode defaultFilterMode;
    TextureWrapMode defaultWrapMode;

    // Helper methods
    void createDefaultTexture(std::unique_ptr<Texture>& texture, const glm::vec4& color);
    unsigned int calculateTextureMemory(const Texture* texture) const;
};