#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <glm.hpp>
#include <functional>

// Forward declarations
class Shader;
class Texture;
class TextureManager;

// Enum for different palette generation methods
enum class PaletteGenerationMethod
{
    RGB_CUBE,           // Evenly distributed colors in RGB space
    HSV_DISTRIBUTED,    // Distributed across hue with varied saturation and value
    GRAYSCALE,          // Grayscale ramp
    CUSTOM              // Custom function-based generation
};

// Enum for dithering patterns
enum class DitheringPattern
{
    ORDERED_2X2,        // 2x2 ordered dithering (4 levels)
    ORDERED_4X4,        // 4x4 ordered dithering (Bayer matrix - 16 levels)
    ORDERED_8X8,        // 8x8 ordered dithering (64 levels)
    BLUE_NOISE,         // Blue noise dithering
    CUSTOM              // Custom dithering pattern from texture
};

/**
 * @class PaletteManager
 * @brief Manages color palettes and dithering for pixel art aesthetic
 *
 * The PaletteManager handles creation, loading, and application of color
 * palettes for pixel art rendering. It supports palette constraints, dithering
 * patterns, and dynamic palette transitions based on time of day or location.
 */
class PaletteManager
{
public:
    /**
     * @brief Constructor
     * @param textureManager Pointer to texture manager for resource handling
     */
    PaletteManager(TextureManager* textureManager);

    /**
     * @brief Destructor
     */
    ~PaletteManager();

    /**
     * @brief Load a predefined palette from file
     * @param paletteName Name to identify the palette
     * @param filePath Path to palette file (.png, .act, .pal, etc.)
     * @return True if palette was loaded successfully
     */
    bool loadPalette(const std::string& paletteName, const std::string& filePath);

    /**
     * @brief Create palette from an array of colors
     * @param paletteName Name to identify the palette
     * @param colors Vector of colors for the palette
     * @return True if palette was created successfully
     */
    bool createPalette(const std::string& paletteName, const std::vector<glm::vec4>& colors);

    /**
     * @brief Set the number of colors in the current palette
     * @param colors Number of colors in palette
     */
    void setPaletteSize(int colors);

    /**
     * @brief Generate a palette using one of the built-in methods
     * @param method Method to use for generation
     * @param paletteName Name to identify the generated palette
     * @param size Number of colors to generate
     * @return True if palette was generated successfully
     */
    bool generatePalette(PaletteGenerationMethod method, const std::string& paletteName, int size);

    /**
     * @brief Generate a custom palette using a color generation function
     * @param paletteName Name to identify the generated palette
     * @param size Number of colors to generate
     * @param colorFunc Function that generates a color based on normalized index (0-1)
     * @return True if palette was generated successfully
     */
    bool generateCustomPalette(const std::string& paletteName, int size,
                               std::function<glm::vec4(float)> colorFunc);

    /**
     * @brief Set the active palette
     * @param paletteName Name of the palette to activate
     * @return True if palette was found and activated
     */
    bool setActivePalette(const std::string& paletteName);

    /**
     * @brief Enable or disable palette constraint
     * @param enabled Whether to constrain colors to the palette
     */
    void enablePaletteConstraint(bool enabled);

    /**
     * @brief Check if palette constraint is enabled
     * @return True if palette constraint is enabled
     */
    bool isPaletteConstraintEnabled() const;

    /**
     * @brief Set the dithering pattern to use
     * @param pattern Dithering pattern type
     */
    void setDitheringPattern(DitheringPattern pattern);

    /**
     * @brief Set a custom dithering pattern from a texture
     * @param texture Texture containing the dithering pattern
     */
    void setCustomDitheringPattern(Texture* texture);

    /**
     * @brief Set the strength of dithering
     * @param strength Dithering strength (0.0 - 1.0)
     */
    void setDitheringStrength(float strength);

    /**
     * @brief Smoothly transition from current palette to target palette
     * @param targetPalette Name of target palette
     * @param duration Transition duration in seconds
     */
    void blendToPalette(const std::string& targetPalette, float duration);

    /**
     * @brief Add time-of-day variant of a palette
     * @param timeOfDay Time of day (0-24 hour format)
     * @param paletteName Name of palette to use at this time
     */
    void addTimeOfDayVariant(float timeOfDay, const std::string& paletteName);

    /**
     * @brief Update palette based on time of day
     * @param timeOfDay Current time of day (0-24 hour format)
     * @param deltaTime Time since last update in seconds
     */
    void update(float timeOfDay, float deltaTime);

    /**
     * @brief Bind palette resources to a shader
     * @param shader Shader to set uniform values on
     */
    void bindPaletteResources(Shader* shader);

    /**
     * @brief Get the current palette texture
     * @return Pointer to current palette texture
     */
    Texture* getPaletteTexture() const;

    /**
     * @brief Get the current dithering pattern texture
     * @return Pointer to current dithering pattern texture
     */
    Texture* getDitherPatternTexture() const;

    /**
     * @brief Get the number of colors in the current palette
     * @return Color count
     */
    int getPaletteSize() const;

    /**
     * @brief Get the current dithering strength
     * @return Dithering strength (0.0 - 1.0)
     */
    float getDitheringStrength() const;

    /**
     * @brief Get the raw color data for the current palette
     * @return Vector of palette colors
     */
    const std::vector<glm::vec4>& getCurrentPaletteColors() const;

    /**
     * @brief Get all available palette names
     * @return Vector of palette names
     */
    std::vector<std::string> getAvailablePalettes() const;

private:
    // Texture manager reference
    TextureManager* m_textureManager;

    // Palette storage
    std::unordered_map<std::string, std::vector<glm::vec4>> m_palettes;

    // Current palette data
    std::string m_currentPaletteName;
    std::vector<glm::vec4> m_currentPalette;
    int m_paletteSize;

    // Time of day variant mapping
    std::unordered_map<float, std::string> m_timeOfDayVariants;

    // Palette blending
    std::string m_targetPaletteName;
    std::vector<glm::vec4> m_targetPalette;
    float m_blendDuration;
    float m_blendProgress;
    bool m_isBlending;

    // Palette constraint
    bool m_paletteConstraintEnabled;

    // Dithering
    DitheringPattern m_ditheringPattern;
    float m_ditheringStrength;
    Texture* m_customDitherTexture;

    // Textures for GPU
    Texture* m_paletteTexture;
    Texture* m_ditherPatternTexture;

    // Helper methods
    void createPaletteTexture();
    void createDefaultDitherPatterns();
    void updatePaletteTexture();
    void generateDitherTexture(DitheringPattern pattern);
    void processPaletteBlending(float deltaTime);
    std::vector<glm::vec4> interpolatePalettes(const std::vector<glm::vec4>& palette1,
                                               const std::vector<glm::vec4>& palette2,
                                               float blend);
};