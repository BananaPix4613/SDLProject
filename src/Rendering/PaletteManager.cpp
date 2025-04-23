// -------------------------------------------------------------------------
// PaletteManager.cpp
// -------------------------------------------------------------------------
#include "Rendering/PaletteManager.h"
#include "Core/Logger.h"
#include "Utility/FileSystem.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <limits>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Rendering
{

    PaletteManager::PaletteManager()
        : m_activePalette("")
        , m_targetPalette("")
        , m_blendDuration(0.0f)
        , m_blendProgress(0.0f)
        , m_constraintEnabled(false)
        , m_constraintStrength(1.0f)
        , m_ditheringPattern(DitheringPattern::None)
        , m_ditheringStrength(0.0f)
    {
    }

    PaletteManager::~PaletteManager()
    {
        shutdown();
    }

    bool PaletteManager::initialize()
    {
        Log::info("Initializing PaletteManager");

        // Create default grayscale palette
        std::vector<glm::vec4> grayscalePalette;
        for (int i = 0; i < 16; i++)
        {
            float v = i / 15.0f;
            grayscalePalette.push_back(glm::vec4(v, v, v, 1.0f));
        }
        createPalette("Grayscale", grayscalePalette);

        // Create default color palette (16 colors)
        std::vector<glm::vec4> defaultPalette = {
            // Basic colors
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),      // Black
            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),      // White
            glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),      // Red
            glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),      // Green
            glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),      // Blue
            glm::vec4(1.0f, 1.0f, 0.0f, 1.0f),      // Yellow
            glm::vec4(1.0f, 0.0f, 1.0f, 1.0f),      // Magenta
            glm::vec4(0.0f, 1.0f, 1.0f, 1.0f),      // Cyan
            // Additional colors
            glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),      // Gray
            glm::vec4(0.5f, 0.0f, 0.0f, 1.0f),      // Dark red
            glm::vec4(0.0f, 0.5f, 0.0f, 1.0f),      // Dark green
            glm::vec4(0.0f, 0.0f, 0.5f, 1.0f),      // Dark blue
            glm::vec4(0.5f, 0.5f, 0.0f, 1.0f),      // Dark yellow
            glm::vec4(0.5f, 0.0f, 0.5f, 1.0f),      // Dark magenta
            glm::vec4(0.0f, 0.5f, 0.5f, 1.0f),      // Dark cyan
            glm::vec4(0.8f, 0.4f, 0.2f, 1.0f)       // Orange
        };
        createPalette("Default", defaultPalette);

        // Set Default as the active palette
        setActivePalette("Default");

        // Create dithering textures
        createDitheringTextures();

        return true;
    }

    void PaletteManager::shutdown()
    {
        Log::info("Shutting down PaletteManager");

        m_palettes.clear();
        m_paletteTextures.clear();
        m_activePalette = "";
        m_targetPalette = "";
        m_timeVariants.clear();
    }

    bool PaletteManager::loadPalette(const std::string& name, const std::string& filepath)
    {
        Log::info("Loading palette '" + name + "' from " + filepath);

        if (m_palettes.find(name) != m_palettes.end())
        {
            Log::warn("Palette '" + name + "' already exists and will be overwritten");
        }

        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open())
        {
            Log::error("Failed to open palette file: " + filepath);
            return false;
        }

        // Detect file format based on extension
        std::string extension = filepath.substr(filepath.find_last_of(".") + 1);
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        std::vector<glm::vec4> colors;

        if (extension == "pal")
        {
            // JASC-PAL format
            std::string line;

            // Read header
            std::getline(file, line); // JASC-PAL
            std::getline(file, line); // Version

            // Read color count
            std::getline(file, line);
            int colorCount = std::stoi(line);

            // Read colors
            for (int i = 0; i < colorCount; i++)
            {
                std::getline(file, line);
                std::stringstream ss(line);

                int r, g, b;
                ss >> r >> g >> b;

                colors.push_back(glm::vec4(
                    r / 255.0f,
                    g / 255.0f,
                    b / 255.0f,
                    1.0f
                ));
            }
        }
        else if (extension == "hex")
        {
            // Hex color codes, one per line
            std::string line;
            while (std::getline(file, line))
            {
                if (line.empty() || line[0] == '#' && line.length() == 1) continue; // Skip empty lines and comments

                // Remove any whitespace
                line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());

                // Remove # if present
                if (line[0] == '#') line = line.substr(1);

                // Parse hex code
                unsigned int hexColor = std::stoul(line, nullptr, 16);

                // Determine if it's RGB or RGBA based on length
                if (line.length() <= 6)
                { // RGB
                    float r = ((hexColor >> 16) & 0xFF) / 255.0f;
                    float g = ((hexColor >> 8) & 0xFF) / 255.0f;
                    float b = (hexColor & 0xFF) / 255.0f;
                    colors.push_back(glm::vec4(r, g, b, 1.0f));
                }
                else
                { // RGBA
                    float r = ((hexColor >> 24) & 0xFF) / 255.0f;
                    float g = ((hexColor >> 16) & 0xFF) / 255.0f;
                    float b = ((hexColor >> 8) & 0xFF) / 255.0f;
                    float a = (hexColor & 0xFF) / 255.0f;
                    colors.push_back(glm::vec4(r, g, b, a));
                }
            }
        }
        else if (extension == "gpl")
        {
            // GIMP Palette format
            std::string line;

            // Skip header
            while (std::getline(file, line) && line.substr(0, 5) != "Name:")
            {
                // Keep reading until we find the Name: line or reach EOF
            }

            // Read colors
            while (std::getline(file, line))
            {
                if (line.empty() || line[0] == '#') continue; // Skip empty lines and comments

                std::stringstream ss(line);
                int r, g, b;
                ss >> r >> g >> b;

                // Skip any color name after RGB values
                colors.push_back(glm::vec4(
                    r / 255.0f,
                    g / 255.0f,
                    b / 255.0f,
                    1.0f
                ));
            }
        }
        else
        {
            Log::error("Unsupported palette file format: " + extension);
            return false;
        }

        if (colors.empty())
        {
            Log::error("No colors found in palette file: " + filepath);
            return false;
        }

        // Store the palette
        m_palettes[name] = colors;

        // Create a texture for the palette
        updatePaletteTexture(name);

        // If no active palette is set, make this one active
        if (m_activePalette.empty())
        {
            m_activePalette = name;
        }

        Log::info("Loaded palette '" + name + "' with " + std::to_string(colors.size()) + " colors");
        return true;
    }

    bool PaletteManager::createPalette(const std::string& name, const std::vector<glm::vec4>& colors)
    {
        if (colors.empty())
        {
            Log::error("Cannot create empty palette: " + name);
            return false;
        }

        if (m_palettes.find(name) != m_palettes.end())
        {
            Log::warn("Palette '" + name + "' already exists and will be overwritten");
        }

        m_palettes[name] = colors;

        // Create a texture for the palette
        updatePaletteTexture(name);

        // If no active palette is set, make this one active
        if (m_activePalette.empty())
        {
            m_activePalette = name;
        }

        Log::info("Created palette '" + name + "' with " + std::to_string(colors.size()) + " colors");
        return true;
    }

    bool PaletteManager::savePalette(const std::string& name, const std::string& filepath)
    {
        if (!hasPalette(name))
        {
            Log::error("Cannot save non-existent palette: " + name);
            return false;
        }

        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open())
        {
            Log::error("Failed to open file for writing: " + filepath);
            return false;
        }

        // Detect file format based on extension
        std::string extension = filepath.substr(filepath.find_last_of(".") + 1);
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        const auto& colors = m_palettes[name];

        if (extension == "pal")
        {
            // JASC-PAL format
            file << "JASC-PAL" << std::endl;
            file << "0100" << std::endl;
            file << colors.size() << std::endl;

            for (const auto& color : colors)
            {
                file << static_cast<int>(color.r * 255.0f) << " "
                    << static_cast<int>(color.g * 255.0f) << " "
                    << static_cast<int>(color.b * 255.0f) << std::endl;
            }
        }
        else if (extension == "hex")
        {
            // Hex color codes, one per line
            for (const auto& color : colors)
            {
                char hexColor[10];
                snprintf(hexColor, sizeof(hexColor), "#%02X%02X%02X%02X",
                         static_cast<int>(color.r * 255.0f),
                         static_cast<int>(color.g * 255.0f),
                         static_cast<int>(color.b * 255.0f),
                         static_cast<int>(color.a * 255.0f));
                file << hexColor << std::endl;
            }
        }
        else if (extension == "gpl")
        {
            // GIMP Palette format
            file << "GIMP Palette" << std::endl;
            file << "Name: " << name << std::endl;
            file << "Columns: " << std::min(16, static_cast<int>(colors.size())) << std::endl;
            file << "#" << std::endl;

            for (size_t i = 0; i < colors.size(); i++)
            {
                const auto& color = colors[i];
                file << static_cast<int>(color.r * 255.0f) << " "
                    << static_cast<int>(color.g * 255.0f) << " "
                    << static_cast<int>(color.b * 255.0f) << "\tColor " << i << std::endl;
            }
        }
        else
        {
            Log::error("Unsupported palette file format: " + extension);
            return false;
        }

        Log::info("Saved palette '" + name + "' to " + filepath);
        return true;
    }

    bool PaletteManager::deletePalette(const std::string& name)
    {
        if (!hasPalette(name))
        {
            Log::error("Cannot delete non-existent palette: " + name);
            return false;
        }

        // Remove the palette
        m_palettes.erase(name);

        // Remove the texture if it exists
        auto textureIt = m_paletteTextures.find(name);
        if (textureIt != m_paletteTextures.end())
        {
            m_paletteTextures.erase(textureIt);
        }

        // If this was the active palette, clear the active palette
        if (m_activePalette == name)
        {
            m_activePalette = "";

            // Set a new active palette if any exist
            if (!m_palettes.empty())
            {
                m_activePalette = m_palettes.begin()->first;
            }
        }

        // If this was the target palette, clear the target palette
        if (m_targetPalette == name)
        {
            m_targetPalette = "";
            m_blendProgress = 0.0f;
            m_blendDuration = 0.0f;
        }

        // Remove any time variants that use this palette
        for (auto it = m_timeVariants.begin(); it != m_timeVariants.end();)
        {
            auto& variants = it->second;

            // Remove variants that use this palette
            for (auto varIt = variants.begin(); varIt != variants.end();)
            {
                if (varIt->second == name)
                {
                    varIt = variants.erase(varIt);
                }
                else
                {
                    ++varIt;
                }
            }

            // If base palette has no variants left, remove it
            if (variants.empty())
            {
                it = m_timeVariants.erase(it);
            }
            else
            {
                ++it;
            }
        }

        Log::info("Deleted palette: " + name);
        return true;
    }

    bool PaletteManager::hasPalette(const std::string& name) const
    {
        return m_palettes.find(name) != m_palettes.end();
    }

    const std::vector<glm::vec4>& PaletteManager::getPaletteColors(const std::string& name) const
    {
        static const std::vector<glm::vec4> emptyPalette;

        if (!hasPalette(name))
        {
            Log::error("Palette not found: " + name);
            return emptyPalette;
        }

        return m_palettes.at(name);
    }

    std::shared_ptr<Texture> PaletteManager::getPaletteTexture(const std::string& name)
    {
        if (!hasPalette(name))
        {
            Log::error("Palette not found: " + name);
            return nullptr;
        }

        // Create the texture if it doesn't exist
        if (m_paletteTextures.find(name) == m_paletteTextures.end())
        {
            updatePaletteTexture(name);
        }

        return m_paletteTextures[name];
    }

    std::vector<std::string> PaletteManager::getPaletteNames() const
    {
        std::vector<std::string> names;
        names.reserve(m_palettes.size());

        for (const auto& pair : m_palettes)
        {
            names.push_back(pair.first);
        }

        return names;
    }

    void PaletteManager::setActivePalette(const std::string& name)
    {
        if (!hasPalette(name))
        {
            Log::error("Cannot set non-existent palette as active: " + name);
            return;
        }

        m_activePalette = name;
        m_targetPalette = "";
        m_blendProgress = 0.0f;
        m_blendDuration = 0.0f;

        Log::info("Active palette set to: " + name);
    }

    const std::string& PaletteManager::getActivePalette() const
    {
        return m_activePalette;
    }

    void PaletteManager::enablePaletteConstraint(bool enabled)
    {
        m_constraintEnabled = enabled;
    }

    bool PaletteManager::isPaletteConstraintEnabled() const
    {
        return m_constraintEnabled;
    }

    void PaletteManager::setConstraintStrength(float strength)
    {
        m_constraintStrength = std::max(0.0f, std::min(1.0f, strength));
    }

    float PaletteManager::getConstraintStrength() const
    {
        return m_constraintStrength;
    }

    void PaletteManager::setDitheringPattern(DitheringPattern pattern)
    {
        if (m_ditheringPattern != pattern)
        {
            m_ditheringPattern = pattern;
            createDitheringTextures();
        }
    }

    void PaletteManager::setDitheringStrength(float strength)
    {
        m_ditheringStrength = std::max(0.0f, std::min(1.0f, strength));
    }

    DitheringPattern PaletteManager::getDitheringPattern() const
    {
        return m_ditheringPattern;
    }

    float PaletteManager::getDitheringStrength() const
    {
        return m_ditheringStrength;
    }

    bool PaletteManager::generatePalette(const std::string& name, int colorCount, bool includeTransparency)
    {
        if (colorCount <= 0)
        {
            Log::error("Cannot generate palette with zero or negative color count");
            return false;
        }

        std::vector<glm::vec4> colors;
        colors.reserve(colorCount);

        // Generate colors with evenly distributed hues and varying saturation/value
        for (int i = 0; i < colorCount; i++)
        {
            float hue = i * (360.0f / colorCount);
            float saturation = 0.7f + 0.3f * ((i % 3) / 2.0f); // Alternate between 0.7 and 1.0
            float value = 0.7f + 0.3f * ((i % 2)); // Alternate between 0.7 and 1.0

            // Convert HSV to RGB
            float c = value * saturation;
            float x = c * (1 - std::abs(std::fmod(hue / 60.0f, 2) - 1));
            float m = value - c;

            float r, g, b;
            if (hue < 60)
            {
                r = c; g = x; b = 0;
            }
            else if (hue < 120)
            {
                r = x; g = c; b = 0;
            }
            else if (hue < 180)
            {
                r = 0; g = c; b = x;
            }
            else if (hue < 240)
            {
                r = 0; g = x; b = c;
            }
            else if (hue < 300)
            {
                r = x; g = 0; b = c;
            }
            else
            {
                r = c; g = 0; b = x;
            }

            colors.push_back(glm::vec4(r + m, g + m, b + m, 1.0f));
        }

        // Add black and white if there's room
        if (colorCount >= 3)
        {
            colors[0] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); // Black
            colors[1] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); // White
        }

        // Add transparent color if requested
        if (includeTransparency)
        {
            colors.push_back(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
        }

        // Create the palette
        return createPalette(name, colors);
    }

    bool PaletteManager::extractPaletteFromImage(const std::string& name, const std::string& imagePath, int maxColors)
    {
        // Load the image data
        // Note: In a real implementation, this would use the engine's resource system
        // For demonstration purposes, we'll use a placeholder implementation

        Log::info("Extracting palette from image: " + imagePath);

        // Placeholder for image loading (would use the engine's Resource system)
        auto imageResource = std::make_shared<Core::Resource>();
        /*if (!imageResource->load(imagePath))
        {
            Log::error("Failed to load image for palette extraction: " + imagePath);
            return false;
        }*/

        // Placeholder implementation for color quantization
        // In a real implementation, this would use median cut, k-means clustering,
        // or another color quantization algorithm

        // Simulated colors for demonstration (please replace with actual implementation)
        std::vector<glm::vec4> extractedColors = {
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), // Black
            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), // White
            glm::vec4(0.8f, 0.1f, 0.1f, 1.0f), // Red
            glm::vec4(0.1f, 0.7f, 0.2f, 1.0f), // Green
            glm::vec4(0.2f, 0.3f, 0.8f, 1.0f), // Blue
            glm::vec4(0.9f, 0.8f, 0.2f, 1.0f), // Yellow
            glm::vec4(0.8f, 0.3f, 0.7f, 1.0f), // Magenta
            glm::vec4(0.3f, 0.8f, 0.9f, 1.0f)  // Cyan
        };

        // Limit to maxColors
        if (extractedColors.size() > static_cast<size_t>(maxColors))
        {
            extractedColors.resize(maxColors);
        }

        // Create the palette
        return createPalette(name, extractedColors);
    }

    bool PaletteManager::addColorToPalette(const std::string& name, const glm::vec4& color)
    {
        if (!hasPalette(name))
        {
            Log::error("Cannot add color to non-existent palette: " + name);
            return false;
        }

        m_palettes[name].push_back(color);

        // Update the texture
        updatePaletteTexture(name);

        return true;
    }

    bool PaletteManager::removeColorFromPalette(const std::string& name, int index)
    {
        if (!hasPalette(name))
        {
            Log::error("Cannot remove color from non-existent palette: " + name);
            return false;
        }

        auto& colors = m_palettes[name];

        if (index < 0 || index >= static_cast<int>(colors.size()))
        {
            Log::error("Color index out of range: " + std::to_string(index));
            return false;
        }

        colors.erase(colors.begin() + index);

        // Update the texture
        updatePaletteTexture(name);

        return true;
    }

    bool PaletteManager::updateColorInPalette(const std::string& name, int index, const glm::vec4& color)
    {
        if (!hasPalette(name))
        {
            Log::error("Cannot update color in non-existent palette: " + name);
            return false;
        }

        auto& colors = m_palettes[name];

        if (index < 0 || index >= static_cast<int>(colors.size()))
        {
            Log::error("Color index out of range: " + std::to_string(index));
            return false;
        }

        colors[index] = color;

        // Update the texture
        updatePaletteTexture(name);

        return true;
    }

    glm::vec4 PaletteManager::findNearestColor(const glm::vec4& color, const std::string& paletteName) const
    {
        int index = findNearestColorIndex(color, paletteName);

        // Use the active palette if none specified
        std::string palette = paletteName.empty() ? m_activePalette : paletteName;

        if (index < 0 || !hasPalette(palette))
        {
            return color; // Return original color if no palette or index found
        }

        const auto& colors = m_palettes.at(palette);
        return colors[index];
    }

    int PaletteManager::findNearestColorIndex(const glm::vec4& color, const std::string& paletteName) const
    {
        // Use the active palette if none specified
        std::string palette = paletteName.empty() ? m_activePalette : paletteName;

        if (!hasPalette(palette))
        {
            Log::error("Palette not found: " + palette);
            return -1;
        }

        const auto& colors = m_palettes.at(palette);
        if (colors.empty())
        {
            return -1;
        }

        // Find the nearest color by distance
        float minDistance = std::numeric_limits<float>::max();
        int minIndex = 0;

        for (size_t i = 0; i < colors.size(); i++)
        {
            float distance = colorDistance(color, colors[i]);

            if (distance < minDistance)
            {
                minDistance = distance;
                minIndex = static_cast<int>(i);
            }
        }

        return minIndex;
    }

    void PaletteManager::addTimeOfDayVariant(const std::string& basePalette, float timeOfDay, const std::string& variantPalette)
    {
        if (!hasPalette(basePalette))
        {
            Log::error("Base palette not found: " + basePalette);
            return;
        }

        if (!hasPalette(variantPalette))
        {
            Log::error("Variant palette not found: " + variantPalette);
            return;
        }

        // Clamp time of day to 0-24 range
        float clampedTime = std::max(0.0f, std::min(24.0f, timeOfDay));

        // Add the variant
        m_timeVariants[basePalette][clampedTime] = variantPalette;

        Log::info("Added time variant for palette '" + basePalette + "' at time " +
                  std::to_string(clampedTime) + " using palette '" + variantPalette + "'");
    }

    void PaletteManager::updateTimeOfDay(float timeOfDay)
    {
        // If no active palette or no variants, nothing to do
        if (m_activePalette.empty() || m_timeVariants.empty())
        {
            return;
        }

        // Check if the active palette has time variants
        auto it = m_timeVariants.find(m_activePalette);
        if (it == m_timeVariants.end())
        {
            return;
        }

        // Clamp time of day to 0-24 range
        float clampedTime = std::max(0.0f, std::min(24.0f, timeOfDay));

        // Find the surrounding time points
        const auto& variants = it->second;
        if (variants.empty())
        {
            return;
        }

        // If we have an exact match for this time
        auto exactMatch = variants.find(clampedTime);
        if (exactMatch != variants.end())
        {
            // Check if we're not already using this palette
            if (m_activePalette != exactMatch->second)
            {
                // Just switch to that palette directly
                setActivePalette(exactMatch->second);
            }
            return;
        }

        // Find the two closest time points
        float prevTime = -1.0f;
        float nextTime = -1.0f;
        std::string prevPalette;
        std::string nextPalette;

        for (const auto& entry : variants)
        {
            float t = entry.first;

            if (t <= clampedTime && (prevTime < 0.0f || t > prevTime))
            {
                prevTime = t;
                prevPalette = entry.second;
            }

            if (t >= clampedTime && (nextTime < 0.0f || t < nextTime))
            {
                nextTime = t;
                nextPalette = entry.second;
            }
        }

        // Handle wrap-around case
        if (prevTime < 0.0f)
        {
            // Find the maximum time
            for (const auto& entry : variants)
            {
                if (entry.first > prevTime)
                {
                    prevTime = entry.first;
                    prevPalette = entry.second;
                }
            }
        }

        if (nextTime < 0.0f)
        {
            // Find the minimum time
            nextTime = 24.0f;
            for (const auto& entry : variants)
            {
                if (entry.first < nextTime)
                {
                    nextTime = entry.first;
                    nextPalette = entry.second;
                }
            }
        }

        // Calculate blend factor
        float blendFactor;
        if (prevTime > nextTime)
        {
            // Handle wrap-around case (prev = 22, next = 2, time = 23)
            float totalTime = (24.0f - prevTime) + nextTime;
            float currentOffset;
            if (clampedTime < prevTime && clampedTime < nextTime)
            {
                // We're in the wraparound region (e.g. time = 1)
                currentOffset = (clampedTime + 24.0f - prevTime);
            }
            else
            {
                // Normal case
                currentOffset = (clampedTime - prevTime);
            }
            blendFactor = currentOffset / totalTime;
        }
        else
        {
            float totalTime = nextTime - prevTime;
            blendFactor = (totalTime > 0.0f) ? ((clampedTime - prevTime) / totalTime) : 0.0f;
        }

        // Create blended palette
        if (!prevPalette.empty() && !nextPalette.empty())
        {
            // Create a blend of the two palettes
            std::string blendedName = "TimeBlend_" + prevPalette + "_" + nextPalette;

            const auto& prevColors = m_palettes[prevPalette];
            const auto& nextColors = m_palettes[nextPalette];

            // Only blend if the palettes have the same number of colors
            if (prevColors.size() == nextColors.size())
            {
                std::vector<glm::vec4> blendedColors;
                blendedColors.reserve(prevColors.size());

                for (size_t i = 0; i < prevColors.size(); i++)
                {
                    blendedColors.push_back(
                        glm::mix(prevColors[i], nextColors[i], blendFactor)
                    );
                }

                // Create or update the blended palette
                createPalette(blendedName, blendedColors);

                // Set as active palette without disrupting time-of-day system
                std::string oldActive = m_activePalette;
                m_activePalette = blendedName;
                // Restore original palette in time variants to prevent recursion
                m_timeVariants[oldActive] = variants;
            }
            else
            {
                // If palettes have different sizes, just use the closer palette
                if (blendFactor < 0.5f)
                {
                    setActivePalette(prevPalette);
                }
                else
                {
                    setActivePalette(nextPalette);
                }
            }
        }
    }

    void PaletteManager::blendToPalette(const std::string& targetPalette, float duration)
    {
        if (!hasPalette(targetPalette))
        {
            Log::error("Target palette not found: " + targetPalette);
            return;
        }

        if (m_activePalette.empty())
        {
            // If no active palette, just set the target as active immediately
            setActivePalette(targetPalette);
            return;
        }

        // Don't blend to the same palette
        if (m_activePalette == targetPalette)
        {
            return;
        }

        m_targetPalette = targetPalette;
        m_blendDuration = std::max(0.1f, duration);
        m_blendProgress = 0.0f;

        Log::info("Starting palette blend from '" + m_activePalette + "' to '" + m_targetPalette +
                  "' over " + std::to_string(m_blendDuration) + " seconds");
    }

    void PaletteManager::updatePaletteBlend(float deltaTime)
    {
        // If no blend in progress, return
        if (m_targetPalette.empty() || m_blendDuration <= 0.0f)
        {
            return;
        }

        // Update blend progress
        m_blendProgress += deltaTime / m_blendDuration;

        // If blend complete, set target as active
        if (m_blendProgress >= 1.0f)
        {
            setActivePalette(m_targetPalette);
            return;
        }

        // Create a blend palette
        std::string blendName = "Blend_" + m_activePalette + "_" + m_targetPalette;

        const auto& sourceColors = m_palettes[m_activePalette];
        const auto& targetColors = m_palettes[m_targetPalette];

        // If the palettes have different sizes, use the smaller one's size
        size_t blendSize = std::min(sourceColors.size(), targetColors.size());

        std::vector<glm::vec4> blendedColors;
        blendedColors.reserve(blendSize);

        for (size_t i = 0; i < blendSize; i++)
        {
            blendedColors.push_back(
                glm::mix(sourceColors[i], targetColors[i], m_blendProgress)
            );
        }

        // Create or update the blended palette
        createPalette(blendName, blendedColors);

        // Set as active palette (without resetting the blend)
        std::string oldActive = m_activePalette;
        std::string oldTarget = m_targetPalette;
        float oldProgress = m_blendProgress;
        float oldDuration = m_blendDuration;

        m_activePalette = blendName;

        // Restore blend state
        m_targetPalette = oldTarget;
        m_blendProgress = oldProgress;
        m_blendDuration = oldDuration;
    }

    float PaletteManager::getPaletteBlendProgress() const
    {
        return m_blendProgress;
    }

    void PaletteManager::bindPaletteTextures(std::shared_ptr<Shader> shader)
    {
        if (!shader)
        {
            Log::error("Cannot bind palette textures to null shader");
            return;
        }

        // Make sure the active palette has a texture
        if (!m_activePalette.empty() && m_paletteTextures.find(m_activePalette) == m_paletteTextures.end())
        {
            updatePaletteTexture(m_activePalette);
        }

        // Bind the active palette texture
        if (!m_activePalette.empty() && m_paletteTextures.find(m_activePalette) != m_paletteTextures.end())
        {
            shader->setUniform("u_PaletteTexture", m_paletteTextures[m_activePalette]);
            shader->setUniform("u_PaletteSize", static_cast<int>(m_palettes[m_activePalette].size()));
        }

        // Bind the dithering texture if enabled
        if (m_ditheringPattern != DitheringPattern::None && m_ditheringTexture)
        {
            shader->setUniform("u_DitheringTexture", m_ditheringTexture);
            shader->setUniform("u_DitheringStrength", m_ditheringStrength);
        }

        // Set other palette-related uniforms
        shader->setUniform("u_PaletteConstraintEnabled", m_constraintEnabled ? 1 : 0);
        shader->setUniform("u_PaletteConstraintStrength", m_constraintStrength);
    }

    void PaletteManager::updatePaletteTexture(const std::string& name)
    {
        if (!hasPalette(name))
        {
            Log::error("Cannot update texture for non-existent palette: " + name);
            return;
        }

        const auto& colors = m_palettes[name];
        if (colors.empty())
        {
            Log::error("Cannot create texture for empty palette: " + name);
            return;
        }

        // Create a 1D texture from the palette colors
        auto texture = std::make_shared<Texture>();
        texture->initialize(static_cast<int>(colors.size()), 1, TextureFormat::RGBA8); // RGBA format

        // Create color data for the texture
        std::vector<unsigned char> textureData;
        textureData.reserve(colors.size() * 4);

        for (const auto& color : colors)
        {
            textureData.push_back(static_cast<unsigned char>(color.r * 255.0f));
            textureData.push_back(static_cast<unsigned char>(color.g * 255.0f));
            textureData.push_back(static_cast<unsigned char>(color.b * 255.0f));
            textureData.push_back(static_cast<unsigned char>(color.a * 255.0f));
        }

        // Update the texture with the color data
        texture->setData(textureData.data());
        texture->setFilterMode(TextureFilterMode::Nearest);
        texture->setWrapMode(TextureWrapMode::ClampToEdge);

        // Store the texture
        m_paletteTextures[name] = texture;
    }

    void PaletteManager::createDitheringTextures()
    {
        // Generate the appropriate dithering texture based on the current pattern
        switch (m_ditheringPattern)
        {
            case DitheringPattern::Bayer2x2:
            {
                std::vector<float> pattern = {
                    0.0f / 4.0f, 2.0f / 4.0f,
                    3.0f / 4.0f, 1.0f / 4.0f
                };
                createDitheringTexture(pattern, 2, 2);
                break;
            }
            case DitheringPattern::Bayer4x4:
            {
                std::vector<float> pattern = {
                    0.0f / 16.0f, 8.0f / 16.0f, 2.0f / 16.0f, 10.0f / 16.0f,
                    12.0f / 16.0f, 4.0f / 16.0f, 14.0f / 16.0f, 6.0f / 16.0f,
                    3.0f / 16.0f, 11.0f / 16.0f, 1.0f / 16.0f, 9.0f / 16.0f,
                    15.0f / 16.0f, 7.0f / 16.0f, 13.0f / 16.0f, 5.0f / 16.0f
                };
                createDitheringTexture(pattern, 4, 4);
                break;
            }
            case DitheringPattern::Bayer8x8:
            {
                // Generate 8x8 Bayer matrix
                std::vector<float> pattern(64);

                // Construct 8x8 matrix from 4x4 matrix
                for (int y = 0; y < 8; y++)
                {
                    for (int x = 0; x < 8; x++)
                    {
                        int i = y * 8 + x;
                        int quadrant = (y / 4) * 2 + (x / 4);
                        int subIndex = (y % 4) * 4 + (x % 4);

                        // Use 4x4 pattern values and shift based on quadrant
                        float value = (0.0f / 16.0f + 8.0f / 16.0f + 2.0f / 16.0f + 10.0f / 16.0f +
                                       12.0f / 16.0f + 4.0f / 16.0f + 14.0f / 16.0f + 6.0f / 16.0f +
                                       3.0f / 16.0f + 11.0f / 16.0f + 1.0f / 16.0f + 9.0f / 16.0f +
                                       15.0f / 16.0f + 7.0f / 16.0f + 13.0f / 16.0f + 5.0f / 16.0f) / 16.0f;

                        // Set value based on the 8x8 matrix pattern
                        pattern[i] = value / 4.0f + quadrant / 4.0f;
                    }
                }
                createDitheringTexture(pattern, 8, 8);
                break;
            }
            case DitheringPattern::BlueNoise:
            {
                // In a real implementation, you would load a blue noise texture
                // For now, create a placeholder with random values
                std::vector<float> pattern(16 * 16);
                for (int i = 0; i < 16 * 16; i++)
                {
                    pattern[i] = static_cast<float>(rand()) / RAND_MAX;
                }
                createDitheringTexture(pattern, 16, 16);
                break;
            }
            case DitheringPattern::WhiteNoise:
            {
                // In a real implementation, you would generate white noise
                // For now, create a placeholder with random values
                std::vector<float> pattern(16 * 16);
                for (int i = 0; i < 16 * 16; i++)
                {
                    pattern[i] = static_cast<float>(rand()) / RAND_MAX;
                }
                createDitheringTexture(pattern, 16, 16);
                break;
            }
            case DitheringPattern::Ordered:
            {
                // Ordered dithering pattern (similar to Bayer but with different values)
                std::vector<float> pattern = {
                    0.00f, 0.50f, 0.25f, 0.75f,
                    0.75f, 0.25f, 0.50f, 0.00f,
                    0.50f, 0.00f, 0.75f, 0.25f,
                    0.25f, 0.75f, 0.00f, 0.50f
                };
                createDitheringTexture(pattern, 4, 4);
                break;
            }
            case DitheringPattern::ErrorDiffusion:
            {
                // For error diffusion, we need special handling in the shader
                // Just create a simple pattern for now
                std::vector<float> pattern = {
                    0.00f, 0.00f, 0.00f, 0.00f,
                    0.00f, 0.00f, 0.00f, 0.00f,
                    0.00f, 0.00f, 0.00f, 0.00f,
                    0.00f, 0.00f, 0.00f, 0.00f
                };
                createDitheringTexture(pattern, 4, 4);
                break;
            }
            case DitheringPattern::None:
            default:
                // No dithering, clear the texture
                m_ditheringTexture = nullptr;
                break;
        }
    }

    void PaletteManager::createDitheringTexture(const std::vector<float>& pattern, int width, int height)
    {
        // Convert the float pattern to a texture
        auto texture = std::make_shared<Texture>();
        texture->initialize(width, height, TextureFormat::R8); // R format

        // Convert float values [0,1] to byte values [0,255]
        std::vector<unsigned char> textureData;
        textureData.reserve(pattern.size());

        for (float value : pattern)
        {
            textureData.push_back(static_cast<unsigned char>(value * 255.0f));
        }

        // Update the texture with the pattern data
        texture->setData(textureData.data());
        texture->setFilterMode(TextureFilterMode::Nearest);
        texture->setWrapMode(TextureWrapMode::Repeat);

        // Store the texture
        m_ditheringTexture = texture;
    }

    float PaletteManager::colorDistance(const glm::vec4& a, const glm::vec4& b) const
    {
        // Perceptual color distance using weighted Euclidean distance
        // These weights approximate human visual perception
        const float rWeight = 0.299f;
        const float gWeight = 0.587f;
        const float bWeight = 0.114f;

        // Handle alpha differently - only consider it significant if the difference is large
        float alphaDiff = (std::abs(a.a - b.a) > 0.2f) ? std::abs(a.a - b.a) : 0.0f;

        // Handle fully transparent colors as a special case
        if (a.a < 0.01f && b.a < 0.01f)
        {
            return 0.0f; // Both fully transparent, consider them identical
        }

        // If one is transparent and the other isn't, they're very different
        if ((a.a < 0.01f && b.a > 0.01f) || (a.a > 0.01f && b.a < 0.01f))
        {
            return 2.0f; // Maximum distance for colors with different transparency
        }

        // Color component differences
        float rDiff = a.r - b.r;
        float gDiff = a.g - b.g;
        float bDiff = a.b - b.b;

        // Weighted Euclidean distance
        return std::sqrt(
            rWeight * rDiff * rDiff +
            gWeight * gDiff * gDiff +
            bWeight * bDiff * bDiff +
            alphaDiff * alphaDiff
        );
    }

} // namespace PixelCraft::Rendering