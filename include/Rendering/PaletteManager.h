// -------------------------------------------------------------------------
// PaletteManager.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

#include "Core/Resource.h"
#include "Rendering/Texture.h"
#include "Rendering/Shader.h"

namespace PixelCraft::Rendering
{

    enum class DitheringPattern
    {
        None,
        Bayer2x2,
        Bayer4x4,
        Bayer8x8,
        BlueNoise,
        WhiteNoise,
        Ordered,
        ErrorDiffusion
    };

    /**
     * @brief Manages color palettes for pixel art rendering
     *
     * The PaletteManager handles loading, saving, and managing color palettes
     * for pixel art rendering. It supports palette constraints, dithering, palette
     * swapping, time-of-day variations, and integration with the shader system.
     */
    class PaletteManager
    {
    public:
        /**
         * @brief Constructs a new PaletteManager with default settings
         */
        PaletteManager();

        /**
         * @brief Destructor that ensures proper cleanup
         */
        ~PaletteManager();

        /**
         * @brief Initializes the palette manager and creates default palettes
         * @return True if initialization was successful
         */
        bool initialize();

        /**
         * @brief Releases all resources and resets the manager state
         */
        void shutdown();

        /**
         * @brief Loads a palette from a file (supports PAL, GPL, and HEX formats)
         * @param name Unique identifier for the palette
         * @param filepath Path to the palette file
         * @return True if the palette was loaded successfully
         */
        bool loadPalette(const std::string& name, const std::string& filepath);

        /**
         * @brief Creates a palette from an array of colors
         * @param name Unique identifier for the palette
         * @param colors Vector of RGBA colors (0.0-1.0 range)
         * @return True if the palette was created successfully
         */
        bool createPalette(const std::string& name, const std::vector<glm::vec4>& colors);

        /**
         * @brief Saves a palette to a file (supports PAL, GPL, and HEX formats)
         * @param name The palette to save
         * @param filepath Destination file path
         * @return True if the palette was saved successfully
         */
        bool savePalette(const std::string& name, const std::string& filepath);

        /**
         * @brief Removes a palette from memory
         * @param name The palette to delete
         * @return True if the palette was deleted successfully
         */
        bool deletePalette(const std::string& name);

        /**
         * @brief Checks if a palette with the given name exists
         * @param name The palette name to check
         * @return True if the palette exists
         */
        bool hasPalette(const std::string& name) const;

        /**
         * @brief Gets the array of colors in a palette
         * @param name The palette name
         * @return Const reference to the array of colors
         */
        const std::vector<glm::vec4>& getPaletteColors(const std::string& name) const;

        /**
         * @brief Gets a palette as a texture for use in shaders
         * @param name The palette name
         * @return Shared pointer to the palette texture
         */
        std::shared_ptr<Texture> getPaletteTexture(const std::string& name);

        /**
         * @brief Gets the names of all available palettes
         * @return Vector of palette names
         */
        std::vector<std::string> getPaletteNames() const;

        /**
         * @brief Sets the active palette for rendering
         * @param name The palette to set as active
         */
        void setActivePalette(const std::string& name);

        /**
         * @brief Gets the name of the currently active palette
         * @return The active palette name
         */
        const std::string& getActivePalette() const;

        /**
         * @brief Enables or disables palette constraint for rendering
         * @param enabled Whether to enable palette constraint
         */
        void enablePaletteConstraint(bool enabled);

        /**
         * @brief Checks if palette constraint is enabled
         * @return True if palette constraint is enabled
         */
        bool isPaletteConstraintEnabled() const;

        /**
         * @brief Sets the strength of the palette constraint
         * @param strength Value between 0.0 (no constraint) and 1.0 (full constraint)
         */
        void setConstraintStrength(float strength);

        /**
         * @brief Gets the current constraint strength
         * @return The constraint strength value
         */
        float getConstraintStrength() const;

        /**
         * @brief Sets the dithering pattern to use for color transitions
         * @param pattern The dithering pattern to use
         */
        void setDitheringPattern(DitheringPattern pattern);

        /**
         * @brief Sets the strength of the dithering effect
         * @param strength Value between 0.0 (no dithering) and 1.0 (full dithering)
         */
        void setDitheringStrength(float strength);

        /**
         * @brief Gets the current dithering pattern
         * @return The current dithering pattern
         */
        DitheringPattern getDitheringPattern() const;

        /**
         * @brief Gets the current dithering strength
         * @return The dithering strength value
         */
        float getDitheringStrength() const;

        /**
         * @brief Generates a palette algorithmically
         * @param name Name for the new palette
         * @param colorCount Number of colors to generate
         * @param includeTransparency Whether to include a transparent color
         * @return True if the palette was generated successfully
         */
        bool generatePalette(const std::string& name, int colorCount, bool includeTransparency = false);

        /**
         * @brief Extracts a color palette from an image file
         * @param name Name for the new palette
         * @param imagePath Path to the source image
         * @param maxColors Maximum number of colors to extract
         * @return True if the palette was extracted successfully
         */
        bool extractPaletteFromImage(const std::string& name, const std::string& imagePath, int maxColors = 16);

        /**
         * @brief Adds a color to an existing palette
         * @param name The palette to modify
         * @param color The color to add
         * @return True if the color was added successfully
         */
        bool addColorToPalette(const std::string& name, const glm::vec4& color);

        /**
         * @brief Removes a color from a palette
         * @param name The palette to modify
         * @param index The index of the color to remove
         * @return True if the color was removed successfully
         */
        bool removeColorFromPalette(const std::string& name, int index);

        /**
         * @brief Updates a color in a palette
         * @param name The palette to modify
         * @param index The index of the color to update
         * @param color The new color value
         * @return True if the color was updated successfully
         */
        bool updateColorInPalette(const std::string& name, int index, const glm::vec4& color);

        /**
         * @brief Finds the nearest color in a palette to a given color
         * @param color The color to match
         * @param paletteName The palette to search (uses active palette if empty)
         * @return The nearest color in the palette
         */
        glm::vec4 findNearestColor(const glm::vec4& color, const std::string& paletteName = "") const;

        /**
         * @brief Finds the index of the nearest color in a palette
         * @param color The color to match
         * @param paletteName The palette to search (uses active palette if empty)
         * @return The index of the nearest color, or -1 if not found
         */
        int findNearestColorIndex(const glm::vec4& color, const std::string& paletteName = "") const;

        /**
         * @brief Adds a time-of-day variant of a palette
         * @param basePalette The base palette name
         * @param timeOfDay Time value (0.0-24.0) for the variant
         * @param variantPalette The palette to use at this time
         */
        void addTimeOfDayVariant(const std::string& basePalette, float timeOfDay, const std::string& variantPalette);

        /**
         * @brief Updates the time of day to blend between variants
         * @param timeOfDay Current time value (0.0-24.0)
         */
        void updateTimeOfDay(float timeOfDay);

        /**
         * @brief Starts a smooth transition to another palette
         * @param targetPalette The palette to blend to
         * @param duration Duration of the blend in seconds
         */
        void blendToPalette(const std::string& targetPalette, float duration);

        /**
         * @brief Updates the palette blending process
         * @param deltaTime Time elapsed since last update in seconds
         */
        void updatePaletteBlend(float deltaTime);

        /**
         * @brief Gets the current blend progress
         * @return Value between 0.0 (start) and 1.0 (complete)
         */
        float getPaletteBlendProgress() const;

        /**
         * @brief Binds palette textures to a shader
         * @param shader The shader to bind textures to
         */
        void bindPaletteTextures(std::shared_ptr<Shader> shader);

    private:
        // Palette storage
        std::unordered_map<std::string, std::vector<glm::vec4>> m_palettes;
        std::unordered_map<std::string, std::shared_ptr<Texture>> m_paletteTextures;

        // Active palette tracking
        std::string m_activePalette;
        std::string m_targetPalette;
        float m_blendDuration;
        float m_blendProgress;

        // Palette constraint settings
        bool m_constraintEnabled;
        float m_constraintStrength;

        // Dithering settings
        DitheringPattern m_ditheringPattern;
        float m_ditheringStrength;
        std::shared_ptr<Texture> m_ditheringTexture;

        // Time of day variants
        std::unordered_map<std::string, std::unordered_map<float, std::string>> m_timeVariants;

        /**
         * @brief Updates or creates the texture for a palette
         * @param name The palette name
         */
        void updatePaletteTexture(const std::string& name);

        /**
         * @brief Creates textures for all dithering patterns
         */
        void createDitheringTextures();

        /**
         * @brief Helper method to create a dithering texture from pattern data
         * @param pattern The dithering pattern data
         * @param width Width of the pattern
         * @param height Height of the pattern
         */
        void createDitheringTexture(const std::vector<float>& pattern, int width, int height);

        /**
         * @brief Calculates the perceptual distance between two colors
         * @param a First color
         * @param b Second color
         * @return The distance value (lower means more similar)
         */
        float colorDistance(const glm::vec4& a, const glm::vec4& b) const;
    };

} // namespace PixelCraft::Rendering