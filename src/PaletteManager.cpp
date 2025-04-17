#include "PaletteManager.h"
#include "Shader.h"
#include "Texture.h"
#include "Texture.h"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <gtc/constants.hpp>

// Constructor
PaletteManager::PaletteManager(TextureManager* textureManager) :
    m_textureManager(textureManager),
    m_currentPaletteName(""),
    m_paletteSize(16),
    m_targetPaletteName(""),
    m_blendDuration(0.0f),
    m_blendProgress(0.0f),
    m_isBlending(false),
    m_paletteConstraintEnabled(false),
    m_ditheringPattern(DitheringPattern::ORDERED_4X4),
    m_ditheringStrength(0.5f),
    m_customDitherTexture(nullptr),
    m_paletteTexture(nullptr),
    m_ditherPatternTexture(nullptr)
{
    // Create default palettes if texture manager is available
    if (m_textureManager)
    {
        // Generate a default RGB palette
        generatePalette(PaletteGenerationMethod::RGB_CUBE, "Default", 16);
        setActivePalette("Default");

        // Create default dither patterns
        createDefaultDitherPatterns();
    }
}

// Destructor
PaletteManager::~PaletteManager()
{
    // Texture manager is responsible for texture cleanup
    // All textures are owned by the texture manager
}

// Load a palette from file
bool PaletteManager::loadPalette(const std::string& paletteName, const std::string& filePath)
{
    if (!m_textureManager)
    {
        std::cerr << "Cannot load palette: TextureManager not available" << std::endl;
        return false;
    }

    // Load the palette texture
    Texture* paletteImg = m_textureManager->getTexture(filePath);
    if (!paletteImg)
    {
        std::cerr << "Failed to load palette image: " << filePath << std::endl;
        return false;
    }

    // Extract colors from the palette image
    std::vector<glm::vec4> colors;

    // Different formats might need different extraction logic
    // Here we assume a horizontal strip of colors
    int width = paletteImg->getWidth();
    int height = paletteImg->getHeight();

    // If it's a large image, sample colors proportionally
    int maxColors = 256;  // Maximum palette size
    int step = std::max(1, width / maxColors);

    for (int x = 0; x < width; x += step)
    {
        // Sample from middle row for more reliable color
        int y = height / 2;
        glm::vec4 color = paletteImg->getPixel(x, y);
        colors.push_back(color);

        // Stop if we reached maximum colors
        if (colors.size() >= maxColors)
        {
            break;
        }
    }

    // If palette is too small, duplicate the last color
    if (colors.size() < 4)
    {
        std::cerr << "Warning: Palette has fewer than 4 colors, duplicating last color" << std::endl;
        while (colors.size() < 4)
        {
            colors.push_back(colors.back());
        }
    }

    // Create the palette
    return createPalette(paletteName, colors);
}

// Create a palette from an array of colors
bool PaletteManager::createPalette(const std::string& paletteName, const std::vector<glm::vec4>& colors)
{
    if (colors.empty())
    {
        std::cerr << "Cannot create palette: No colors provided" << std::endl;
        return false;
    }

    // Store the palette
    m_palettes[paletteName] = colors;

    // If this is the first palette, set it as active
    if (m_currentPaletteName.empty())
    {
        setActivePalette(paletteName);
    }

    return true;
}

// Set the number of colors in the current palette
void PaletteManager::setPaletteSize(int colors)
{
    m_paletteSize = std::max(2, std::min(256, colors));

    // Update palette texture
    updatePaletteTexture();
}

// Generate a palette using one of the built-in methods
bool PaletteManager::generatePalette(PaletteGenerationMethod method, const std::string& paletteName, int size)
{
    size = std::max(2, std::min(256, size));
    std::vector<glm::vec4> colors;
    colors.reserve(size);

    switch (method)
    {
        case PaletteGenerationMethod::RGB_CUBE:
        {
            // Create an RGB cube distribution
            int cubeSize = static_cast<int>(std::ceil(std::pow(size, 1.0f / 3.0f)));
            for (int r = 0; r < cubeSize && colors.size() < size; r++)
            {
                for (int g = 0; g < cubeSize && colors.size() < size; g++)
                {
                    for (int b = 0; b < cubeSize && colors.size() < size; b++)
                    {
                        float rf = r / float(cubeSize - 1);
                        float gf = g / float(cubeSize - 1);
                        float bf = b / float(cubeSize - 1);
                        colors.push_back(glm::vec4(rf, gf, bf, 1.0f));
                    }
                }
            }
            break;
        }

        case PaletteGenerationMethod::HSV_DISTRIBUTED:
        {
            // Distribute colors around the hue circle with varying saturation and value
            for (int i = 0; i < size; i++)
            {
                float hue = i / float(size) * 360.0f;
                float sat = 0.7f + 0.3f * ((i % 3) / 2.0f);
                float val = 0.7f + 0.3f * (((i / 3) % 3) / 2.0f);

                // Convert HSV to RGB
                float c = val * sat;
                float x = c * (1 - std::abs(std::fmod(hue / 60.0f, 2) - 1));
                float m = val - c;

                glm::vec3 rgb;
                if (hue < 60) rgb = glm::vec3(c, x, 0);
                else if (hue < 120) rgb = glm::vec3(x, c, 0);
                else if (hue < 180) rgb = glm::vec3(0, c, x);
                else if (hue < 240) rgb = glm::vec3(0, x, c);
                else if (hue < 300) rgb = glm::vec3(x, 0, c);
                else rgb = glm::vec3(c, 0, x);

                rgb += glm::vec3(m);
                colors.push_back(glm::vec4(rgb, 1.0f));
            }
            break;
        }

        case PaletteGenerationMethod::GRAYSCALE:
        {
            // Create a grayscale ramp
            for (int i = 0; i < size; i++)
            {
                float gray = i / float(size - 1);
                colors.push_back(glm::vec4(gray, gray, gray, 1.0f));
            }
            break;
        }

        case PaletteGenerationMethod::CUSTOM:
        {
            // Custom generation is handled by generateCustomPalette()
            std::cerr << "Use generateCustomPalette() for custom palette generation" << std::endl;
            return false;
        }
    }

    // Create the palette
    return createPalette(paletteName, colors);
}

// Generate a custom palette using a color generation function
bool PaletteManager::generateCustomPalette(const std::string& paletteName, int size,
                                           std::function<glm::vec4(float)> colorFunc)
{
    if (!colorFunc)
    {
        std::cerr << "Cannot generate custom palette: Invalid color function" << std::endl;
        return false;
    }

    size = std::max(2, std::min(256, size));
    std::vector<glm::vec4> colors;
    colors.reserve(size);

    // Generate colors using the provided function
    for (int i = 0; i < size; i++)
    {
        float t = i / float(size - 1);
        colors.push_back(colorFunc(t));
    }

    // Create the palette
    return createPalette(paletteName, colors);
}

// Set the active palette
bool PaletteManager::setActivePalette(const std::string& paletteName)
{
    auto it = m_palettes.find(paletteName);
    if (it == m_palettes.end())
    {
        std::cerr << "Palette not found: " << paletteName << std::endl;
        return false;
    }

    m_currentPaletteName = paletteName;
    m_currentPalette = it->second;

    // Update the palette texture
    updatePaletteTexture();

    return true;
}

// Enable or disable palette constraint
void PaletteManager::enablePaletteConstraint(bool enabled)
{
    m_paletteConstraintEnabled = enabled;
}

// Check if palette constraint is enabled
bool PaletteManager::isPaletteConstraintEnabled() const
{
    return m_paletteConstraintEnabled;
}

// Set the dithering pattern to use
void PaletteManager::setDitheringPattern(DitheringPattern pattern)
{
    m_ditheringPattern = pattern;

    // Generate the appropriate dither texture
    generateDitherTexture(pattern);
}

// Set a custom dithering pattern from a texture
void PaletteManager::setCustomDitheringPattern(Texture* texture)
{
    if (texture)
    {
        m_customDitherTexture = texture;
        m_ditheringPattern = DitheringPattern::CUSTOM;
        m_ditherPatternTexture = texture;
    }
}

// Set the strength of dithering
void PaletteManager::setDitheringStrength(float strength)
{
    m_ditheringStrength = std::max(0.0f, std::min(1.0f, strength));
}

// Smoothly transition from current palette to target palette
void PaletteManager::blendToPalette(const std::string& targetPalette, float duration)
{
    auto it = m_palettes.find(targetPalette);
    if (it == m_palettes.end())
    {
        std::cerr << "Target palette not found: " << targetPalette << std::endl;
        return;
    }

    m_targetPaletteName = targetPalette;
    m_targetPalette = it->second;
    m_blendDuration = std::max(0.01f, duration);
    m_blendProgress = 0.0f;
    m_isBlending = true;
}

// Add time-of-day variant of a palette
void PaletteManager::addTimeOfDayVariant(float timeOfDay, const std::string& paletteName)
{
    // Normalize time of day to 0-24 range
    timeOfDay = std::fmod(timeOfDay, 24.0f);
    if (timeOfDay < 0)
    {
        timeOfDay += 24.0f;
    }

    // Check if the palette exists
    if (m_palettes.find(paletteName) == m_palettes.end())
    {
        std::cerr << "Palette not found for time of day variant: " << paletteName << std::endl;
        return;
    }

    // Add the time of day variant
    m_timeOfDayVariants[timeOfDay] = paletteName;
}

// Update palette based on time of day
void PaletteManager::update(float timeOfDay, float deltaTime)
{
    // Process palette blending if active
    if (m_isBlending)
    {
        processPaletteBlending(deltaTime);
    }

    // Check for time of day variants
    if (!m_timeOfDayVariants.empty() && !m_isBlending)
    {
        // Find the two closest time points
        float closestLower = -1.0f;
        float closestHigher = -1.0f;

        for (const auto& variant : m_timeOfDayVariants)
        {
            float variantTime = variant.first;

            if (variantTime <= timeOfDay && (closestLower < 0 || variantTime > closestLower))
            {
                closestLower = variantTime;
            }

            if (variantTime >= timeOfDay && (closestHigher < 0 || variantTime < closestHigher))
            {
                closestHigher = variantTime;
            }
        }

        // Handle wrap-around for 24-hour time
        if (closestLower < 0)
        {
            // No lower time, use highest time (wrap around from previous day)
            float highestTime = -1.0f;
            for (const auto& variant : m_timeOfDayVariants)
            {
                if (variant.first > highestTime)
                {
                    highestTime = variant.first;
                }
            }

            if (highestTime >= 0)
            {
                closestLower = highestTime - 24.0f; // Adjust for previous day
            }
        }

        if (closestHigher < 0)
        {
            // No higher time, use lowest time (wrap around to next day)
            float lowestTime = 24.0f;
            for (const auto& variant : m_timeOfDayVariants)
            {
                if (variant.first < lowestTime)
                {
                    lowestTime = variant.first;
                }
            }

            if (lowestTime < 24.0f)
            {
                closestHigher = lowestTime + 24.0f; // Adjust for next day
            }
        }

        // If we found two valid times, blend between them
        if (closestLower >= 0 && closestHigher >= 0)
        {
            float normalizedLower = std::fmod(closestLower, 24.0f);
            if (normalizedLower < 0)
            {
                normalizedLower += 24.0f;
            }

            float normalizedHigher = std::fmod(closestHigher, 24.0f);
            if (normalizedHigher < 0)
            {
                normalizedHigher += 24.0f;
            }

            std::string lowerPalette = m_timeOfDayVariants[normalizedLower];
            std::string higherPalette = m_timeOfDayVariants[normalizedHigher];

            // Calculate blend factor
            float blendRange = closestHigher - closestLower;
            float blend = (timeOfDay - closestLower) / blendRange;
            blend = std::max(0.0f, std::min(1.0f, blend));

            // Create blended palette
            std::vector<glm::vec4> lowerColors = m_palettes[lowerPalette];
            std::vector<glm::vec4> higherColors = m_palettes[higherPalette];

            m_currentPalette = interpolatePalettes(lowerColors, higherColors, blend);
            updatePaletteTexture();
        }
    }
}

// Bind palette resources to a shader
void PaletteManager::bindPaletteResources(Shader* shader)
{
    if (!shader) return;

    // Ensure textures are created
    if (!m_paletteTexture || !m_ditherPatternTexture)
    {
        createPaletteTexture();
        generateDitherTexture(m_ditheringPattern);
    }

    // Set shader uniforms
    shader->setBool("usePalette", m_paletteConstraintEnabled);
    shader->setInt("paletteSize", m_paletteSize);
    shader->setFloat("ditherStrength", m_ditheringStrength);

    // Bind textures (assuming standard texture units, adjust if needed)
    if (m_paletteTexture)
    {
        m_paletteTexture->bind(5); // Use texture unit 5 for palette
        shader->setInt("paletteTexture", 5);
    }

    if (m_ditherPatternTexture)
    {
        m_ditherPatternTexture->bind(6); // Use texture unit 6 for dither pattern
        shader->setInt("ditherPattern", 6);
    }
}

// Get the current palette texture
Texture* PaletteManager::getPaletteTexture() const
{
    return m_paletteTexture;
}

// Get the current dithering pattern texture
Texture* PaletteManager::getDitherPatternTexture() const
{
    return m_ditherPatternTexture;
}

// Get the number of colors in the current palette
int PaletteManager::getPaletteSize() const
{
    return m_paletteSize;
}

// Get the current dithering strength
float PaletteManager::getDitheringStrength() const
{
    return m_ditheringStrength;
}

// Get the raw color data for the current palette
const std::vector<glm::vec4>& PaletteManager::getCurrentPaletteColors() const
{
    return m_currentPalette;
}

// Get all available palette names
std::vector<std::string> PaletteManager::getAvailablePalettes() const
{
    std::vector<std::string> names;
    names.reserve(m_palettes.size());

    for (const auto& pair : m_palettes)
    {
        names.push_back(pair.first);
    }

    return names;
}

// Update the palette texture with current palette data
void PaletteManager::updatePaletteTexture()
{
    if (!m_textureManager || m_currentPalette.empty()) return;

    // Resize palette to match palette size settings
    std::vector<glm::vec4> resizedPalette;
    resizedPalette.reserve(m_paletteSize);

    if (m_currentPalette.size() == m_paletteSize)
    {
        // No resize needed
        resizedPalette = m_currentPalette;
    }
    else if (m_currentPalette.size() < m_paletteSize)
    {
        // Interpolate to add more colors
        resizedPalette = m_currentPalette;

        // Fill remaining slots with interpolated colors
        while (resizedPalette.size() < m_paletteSize)
        {
            std::vector<glm::vec4> additionalColors;

            for (size_t i = 0; i < resizedPalette.size() - 1 && resizedPalette.size() + additionalColors.size() < m_paletteSize; i++)
            {
                glm::vec4 interpolated = glm::mix(resizedPalette[i], resizedPalette[i + 1], 0.5f);
                additionalColors.push_back(interpolated);
            }

            // Insert new colors between existing ones
            std::vector<glm::vec4> combined;
            combined.reserve(resizedPalette.size() + additionalColors.size());

            for (size_t i = 0; i < resizedPalette.size() - 1; i++)
            {
                combined.push_back(resizedPalette[i]);
                if (i < additionalColors.size())
                {
                    combined.push_back(additionalColors[i]);
                }
            }

            combined.push_back(resizedPalette.back());
            resizedPalette = combined;
        }
    }
    else
    {
        // Downsample to fewer colors
        float ratio = static_cast<float>(m_currentPalette.size() - 1) / (m_paletteSize - 1);

        for (int i = 0; i < m_paletteSize; i++)
        {
            float index = i * ratio;
            int lower = static_cast<int>(index);
            int upper = std::min(static_cast<int>(std::ceil(index)), static_cast<int>(m_currentPalette.size() - 1));

            if (lower == upper)
            {
                resizedPalette.push_back(m_currentPalette[lower]);
            }
            else
            {
                float t = index - lower;
                resizedPalette.push_back(glm::mix(m_currentPalette[lower], m_currentPalette[upper], t));
            }
        }
    }

    // Create or update the texture
    if (m_paletteTexture)
    {
        // Update existing texture
        unsigned char* data = new unsigned char[m_paletteSize * 4];

        for (int i = 0; i < m_paletteSize; i++)
        {
            data[i * 4 + 0] = static_cast<unsigned char>(resizedPalette[i].r * 255.0f);
            data[i * 4 + 1] = static_cast<unsigned char>(resizedPalette[i].g * 255.0f);
            data[i * 4 + 2] = static_cast<unsigned char>(resizedPalette[i].b * 255.0f);
            data[i * 4 + 3] = static_cast<unsigned char>(resizedPalette[i].a * 255.0f);
        }

        m_paletteTexture->setData(data, m_paletteSize, 1, 4);
        delete[] data;
    }
    else
    {
        // Create new texture
        createPaletteTexture();
    }
}

// Create a 1D texture for the palette
void PaletteManager::createPaletteTexture()
{
    if (!m_textureManager) return;

    // Create texture data
    unsigned char* data = new unsigned char[m_paletteSize * 4];

    // Fill with current palette or default colors
    for (int i = 0; i < m_paletteSize; i++)
    {
        if (i < m_currentPalette.size())
        {
            data[i * 4 + 0] = static_cast<unsigned char>(m_currentPalette[i].r * 255.0f);
            data[i * 4 + 1] = static_cast<unsigned char>(m_currentPalette[i].g * 255.0f);
            data[i * 4 + 2] = static_cast<unsigned char>(m_currentPalette[i].b * 255.0f);
            data[i * 4 + 3] = static_cast<unsigned char>(m_currentPalette[i].a * 255.0f);
        }
        else
        {
            // Use a gradient for missing colors
            float t = static_cast<float>(i) / m_paletteSize;
            data[i * 4 + 0] = static_cast<unsigned char>(t * 255.0f);
            data[i * 4 + 1] = static_cast<unsigned char>(t * 255.0f);
            data[i * 4 + 2] = static_cast<unsigned char>(t * 255.0f);
            data[i * 4 + 3] = 255;
        }
    }

    // Create 1D texture
    m_paletteTexture = m_textureManager->createTexture(m_paletteSize, 1, 4);
    if (m_paletteTexture)
    {
        m_paletteTexture->setData(data, m_paletteSize, 1, 4);
        m_paletteTexture->setFilterMode(TextureFilterMode::NEAREST);
        m_paletteTexture->setWrapMode(TextureWrapMode::CLAMP_TO_EDGE);
    }

    delete[] data;
}

// Create default dither patterns
void PaletteManager::createDefaultDitherPatterns()
{
    // Default is 4x4 Bayer matrix dithering
    generateDitherTexture(DitheringPattern::ORDERED_4X4);
}

// Generate a dither texture based on the pattern type
void PaletteManager::generateDitherTexture(DitheringPattern pattern)
{
    if (!m_textureManager) return;

    // For custom pattern, just use the existing texture
    if (pattern == DitheringPattern::CUSTOM && m_customDitherTexture)
    {
        m_ditherPatternTexture = m_customDitherTexture;
        return;
    }

    // Define pattern size and data
    int size = 4; // Default size
    std::vector<float> patternData;

    switch (pattern)
    {
        case DitheringPattern::ORDERED_2X2:
        {
            size = 2;
            // 2x2 Bayer matrix
            float values[2][2] = {
                {0.0f, 0.5f},
                {0.75f, 0.25f}
            };

            patternData.resize(size * size);
            for (int y = 0; y < size; y++)
            {
                for (int x = 0; x < size; x++)
                {
                    patternData[y * size + x] = values[y][x];
                }
            }
            break;
        }

        case DitheringPattern::ORDERED_4X4:
        {
            size = 4;
            // 4x4 Bayer matrix
            float values[4][4] = {
                {0.0f, 0.5f, 0.125f, 0.625f},
                {0.75f, 0.25f, 0.875f, 0.375f},
                {0.1875f, 0.6875f, 0.0625f, 0.5625f},
                {0.9375f, 0.4375f, 0.8125f, 0.3125f}
            };

            patternData.resize(size * size);
            for (int y = 0; y < size; y++)
            {
                for (int x = 0; x < size; x++)
                {
                    patternData[y * size + x] = values[y][x];
                }
            }
            break;
        }

        case DitheringPattern::ORDERED_8X8:
        {
            size = 8;
            // 8x8 Bayer matrix
            patternData.resize(size * size);

            // Generate 8x8 matrix (recursive from 4x4)
            float values4x4[4][4] = {
                {0.0f, 0.5f, 0.125f, 0.625f},
                {0.75f, 0.25f, 0.875f, 0.375f},
                {0.1875f, 0.6875f, 0.0625f, 0.5625f},
                {0.9375f, 0.4375f, 0.8125f, 0.3125f}
            };

            for (int y = 0; y < 8; y++)
            {
                for (int x = 0; x < 8; x++)
                {
                    int quadrant_x = x / 4;
                    int quadrant_y = y / 4;
                    int sub_x = x % 4;
                    int sub_y = y % 4;

                    float value = values4x4[sub_y][sub_x];

                    // Adjust based on quadrant
                    if (quadrant_x == 1 && quadrant_y == 0) value += 0.03125f;
                    else if (quadrant_x == 0 && quadrant_y == 1) value += 0.0625f;
                    else if (quadrant_x == 1 && quadrant_y == 1) value += 0.09375f;

                    patternData[y * 8 + x] = value;
                }
            }
            break;
        }

        case DitheringPattern::BLUE_NOISE:
        {
            size = 32; // Blue noise typically uses larger textures
            patternData.resize(size * size);

            // Simple blue noise approximation using value noise and filtering
            // Note: Real blue noise requires more sophisticated algorithms

            // Start with random noise
            for (int i = 0; i < size * size; i++)
            {
                patternData[i] = static_cast<float>(rand()) / RAND_MAX;
            }

            // Apply simple frequency-domain filter (approximate)
            // In a real implementation, this would use Fourier transforms

            // Blur the noise slightly to create blue noise characteristics
            std::vector<float> tempData = patternData;
            for (int y = 0; y < size; y++)
            {
                for (int x = 0; x < size; x++)
                {
                    float sum = 0.0f;
                    int count = 0;

                    for (int dy = -2; dy <= 2; dy++)
                    {
                        for (int dx = -2; dx <= 2; dx++)
                        {
                            int nx = (x + dx + size) % size;
                            int ny = (y + dy + size) % size;

                            // Distance-based weight
                            float dist = std::sqrt(dx * dx + dy * dy);
                            if (dist <= 2.0f)
                            {
                                float weight = 1.0f - dist / 2.0f;
                                sum += tempData[ny * size + nx] * weight;
                                count += weight;
                            }
                        }
                    }

                    patternData[y * size + x] = sum / count;
                }
            }

            // Normalize values to 0-1 range
            float minVal = *std::min_element(patternData.begin(), patternData.end());
            float maxVal = *std::max_element(patternData.begin(), patternData.end());
            float range = maxVal - minVal;

            for (int i = 0; i < size * size; i++)
            {
                patternData[i] = (patternData[i] - minVal) / range;
            }
            break;
        }

        default:
            // Default to 4x4 ordered dithering
            return generateDitherTexture(DitheringPattern::ORDERED_4X4);
    }

    // Create texture from pattern data
    unsigned char* textureData = new unsigned char[size * size];
    for (int i = 0; i < size * size; i++)
    {
        textureData[i] = static_cast<unsigned char>(patternData[i] * 255.0f);
    }

    // Create or update texture
    if (m_ditherPatternTexture)
    {
        // Check if we need to create a new texture due to size change
        if (m_ditherPatternTexture->getWidth() != size || m_ditherPatternTexture->getHeight() != size)
        {
            // Size changed, create new texture
            m_ditherPatternTexture = m_textureManager->createTexture(size, size, 1);
        }

        if (m_ditherPatternTexture)
        {
            m_ditherPatternTexture->setData(textureData, size, size, 1);
        }
    }
    else
    {
        // Create new texture
        m_ditherPatternTexture = m_textureManager->createTexture(size, size, 1);
        if (m_ditherPatternTexture)
        {
            m_ditherPatternTexture->setData(textureData, size, size, 1);
        }
    }

    // Set texture properties
    if (m_ditherPatternTexture)
    {
        m_ditherPatternTexture->setFilterMode(TextureFilterMode::NEAREST);
        m_ditherPatternTexture->setWrapMode(TextureWrapMode::REPEAT);
    }

    delete[] textureData;
}

// Process palette blending
void PaletteManager::processPaletteBlending(float deltaTime)
{
    if (!m_isBlending) return;

    // Update blend progress
    m_blendProgress += deltaTime / m_blendDuration;

    // Check if blending is complete
    if (m_blendProgress >= 1.0f)
    {
        // Blending complete, set target palette as current
        m_currentPaletteName = m_targetPaletteName;
        m_currentPalette = m_targetPalette;
        m_isBlending = false;

        // Update palette texture
        updatePaletteTexture();
        return;
    }

    // Calculate smooth blend factor (ease in/out)
    float t = m_blendProgress;
    float smoothT = t * t * (3.0f - 2.0f * t); // Smoothstep function

    // Interpolate between current and target palettes
    std::vector<glm::vec4> blendedPalette = interpolatePalettes(m_currentPalette, m_targetPalette, smoothT);

    // Update current palette with blended version
    m_currentPalette = blendedPalette;

    // Update palette texture
    updatePaletteTexture();
}

// Interpolate between two palettes
std::vector<glm::vec4> PaletteManager::interpolatePalettes(const std::vector<glm::vec4>& palette1,
                                                           const std::vector<glm::vec4>& palette2,
                                                           float blend)
{
    // Ensure blend is in valid range
    blend = std::max(0.0f, std::min(1.0f, blend));

    // Handle empty paalettes
    if (palette1.empty()) return palette2;
    if (palette2.empty()) return palette1;

    // Determine output size (use larger palette)
    size_t outputSize = std::max(palette1.size(), palette2.size());
    std::vector<glm::vec4> result(outputSize);

    // Interpolate each color
    for (size_t i = 0; i < outputSize; i++)
    {
        // Calculate normalized positions
        float pos1 = (palette1.size() > 1) ? i / float(palette1.size() - 1) : 0.0f;
        float pos2 = (palette2.size() > 1) ? i / float(palette2.size() - 1) : 0.0f;

        // Get colors from each palette at corresponding positions
        glm::vec4 color1, color2;

        if (palette1.size() == 1)
        {
            color1 = palette1[0];
        }
        else
        {
            float index1 = pos1 * (palette1.size() - 1);
            int lower1 = static_cast<int>(index1);
            int upper1 = std::min(lower1 + 1, static_cast<int>(palette1.size() - 1));
            float t1 = index1 - lower1;

            color1 = glm::mix(palette1[lower1], palette1[upper1], t1);
        }

        if (palette2.size() > 1)
        {
            color2 = palette2[0];
        }
        else
        {
            float index2 = pos2 * (palette2.size() - 1);
            int lower2 = static_cast<int>(palette2.size() - 1);
            int upper2 = std::min(lower2 + 1, static_cast<int>(palette2.size() - 1));
            float t2 = index2 - lower2;

            color2 = glm::mix(palette2[lower2], palette2[upper2], t2);
        }

        // Interpolate between the two colors
        result[i] = glm::mix(color1, color2, blend);
    }

    return result;
}