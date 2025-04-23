// -------------------------------------------------------------------------
// Color.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <vector>
#include <string>
#include <glm/glm.hpp>

namespace PixelCraft::Utility
{

    /**
     * @brief Color representation and manipulation class
     *
     * Provides storage and manipulation of RGBA color values with conversions
     * between different color spaces and color manipulation operations.
     */
    class Color
    {
    public:
        /**
         * @brief Default constructor
         * Creates a black, fully opaque color (0, 0, 0, 1)
         */
        Color();

        /**
         * @brief Constructor from float components
         * @param r Red component (0.0-1.0)
         * @param g Green component (0.0-1.0)
         * @param b Blue component (0.0-1.0)
         * @param a Alpha component (0.0-1.0)
         */
        Color(float r, float g, float b, float a = 1.0f);

        /**
         * @brief Constructor from byte components
         * @param r Red component (0-255)
         * @param g Green component (0-255)
         * @param b Blue component (0-255)
         * @param a Alpha component (0-255)
         */
        Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

        /**
         * @brief Constructor from 32-bit integer RGBA
         * @param rgba Color in 0xRRGGBBAA format
         */
        Color(uint32_t rgba);

        /**
         * @brief Create a color from HSV values
         * @param h Hue component (0.0-360.0)
         * @param s Saturation component (0.0-1.0)
         * @param v Value component (0.0-1.0)
         * @param a Alpha component (0.0-1.0)
         * @return The resulting color
         */
        static Color fromHSV(float h, float s, float v, float a = 1.0f);

        /**
         * @brief Create a color from HSL values
         * @param h Hue component (0.0-360.0)
         * @param s Saturation component (0.0-1.0)
         * @param l Lightness component (0.0-1.0)
         * @param a Alpha component (0.0-1.0)
         * @return The resulting color
         */
        static Color fromHSL(float h, float s, float l, float a = 1.0f);

        /**
         * @brief Create a color from a hex string
         * @param hexString Color in "#RRGGBB" or "#RRGGBBAA" format
         * @return The resulting color
         */
        static Color fromString(const std::string& hexString);

        /**
         * @brief Linearly interpolate between two colors
         * @param a First color
         * @param b Second color
         * @param t Interpolation factor (0.0-1.0)
         * @return The interpolated color
         */
        static Color lerp(const Color& a, const Color& b, float t);

        /**
         * @brief Create a random color
         * @param alpha Alpha value for the random color (0.0-1.0)
         * @return A random color
         */
        static Color random(float alpha = 1.0f);

        /**
         * @brief Get the red component (float)
         * @return Red component (0.0-1.0)
         */
        float r() const;

        /**
         * @brief Get the green component (float)
         * @return Green component (0.0-1.0)
         */
        float g() const;

        /**
         * @brief Get the blue component (float)
         * @return Blue component (0.0-1.0)
         */
        float b() const;

        /**
         * @brief Get the alpha component (float)
         * @return Alpha component (0.0-1.0)
         */
        float a() const;

        /**
         * @brief Set the red component (float)
         * @param r Red component (0.0-1.0)
         */
        void setR(float r);

        /**
         * @brief Set the green component (float)
         * @param g Green component (0.0-1.0)
         */
        void setG(float g);

        /**
         * @brief Set the blue component (float)
         * @param b Blue component (0.0-1.0)
         */
        void setB(float b);

        /**
         * @brief Set the alpha component (float)
         * @param a Alpha component (0.0-1.0)
         */
        void setA(float a);

        /**
         * @brief Get the red component (byte)
         * @return Red component (0-255)
         */
        uint8_t rByte() const;

        /**
         * @brief Get the green component (byte)
         * @return Green component (0-255)
         */
        uint8_t gByte() const;

        /**
         * @brief Get the blue component (byte)
         * @return Blue component (0-255)
         */
        uint8_t bByte() const;

        /**
         * @brief Get the alpha component (byte)
         * @return Alpha component (0-255)
         */
        uint8_t aByte() const;

        /**
         * @brief Set the red component (byte)
         * @param r Red component (0-255)
         */
        void setRByte(uint8_t r);

        /**
         * @brief Set the green component (byte)
         * @param g Green component (0-255)
         */
        void setGByte(uint8_t g);

        /**
         * @brief Set the blue component (byte)
         * @param b Blue component (0-255)
         */
        void setBByte(uint8_t b);

        /**
         * @brief Set the alpha component (byte)
         * @param a Alpha component (0-255)
         */
        void setAByte(uint8_t a);

        /**
         * @brief Get the RGB components as a vec3
         * @return RGB components (0.0-1.0)
         */
        glm::vec3 rgb() const;

        /**
         * @brief Get the RGBA components as a vec4
         * @return RGBA components (0.0-1.0)
         */
        glm::vec4 rgba() const;

        /**
         * @brief Set the RGB components from a vec3
         * @param rgb RGB components (0.0-1.0)
         */
        void setRGB(const glm::vec3& rgb);

        /**
         * @brief Set the RGBA components from a vec4
         * @param rgba RGBA components (0.0-1.0)
         */
        void setRGBA(const glm::vec4& rgba);

        /**
         * @brief Convert the color to a 32-bit integer
         * @return Color in 0xRRGGBBAA format
         */
        uint32_t toInt() const;

        /**
         * @brief Set the color from a 32-bit integer
         * @param rgba Color in 0xRRGGBBAA format
         */
        void setFromInt(uint32_t rgba);

        /**
         * @brief Convert the color to HSV space
         * @return HSV values (hue: 0-360, saturation: 0-1, value: 0-1)
         */
        glm::vec3 toHSV() const;

        /**
         * @brief Convert the color to HSL space
         * @return HSL values (hue: 0-360, saturation: 0-1, lightness: 0-1)
         */
        glm::vec3 toHSL() const;

        /**
         * @brief Set the color from HSV values
         * @param h Hue component (0.0-360.0)
         * @param s Saturation component (0.0-1.0)
         * @param v Value component (0.0-1.0)
         */
        void setHSV(float h, float s, float v);

        /**
         * @brief Set the color from HSL values
         * @param h Hue component (0.0-360.0)
         * @param s Saturation component (0.0-1.0)
         * @param l Lightness component (0.0-1.0)
         */
        void setHSL(float h, float s, float l);

        /**
         * @brief Convert the color to a hex string
         * @param includeAlpha Whether to include the alpha component
         * @return Hex string in "#RRGGBB" or "#RRGGBBAA" format
         */
        std::string toHexString(bool includeAlpha = false) const;

        /**
         * @brief Convert the color to a human-readable string
         * @return String representation of the color
         */
        std::string toString() const;

        /**
         * @brief Create a copy of this color with a different alpha
         * @param alpha New alpha value (0.0-1.0)
         * @return New color with the specified alpha
         */
        Color withAlpha(float alpha) const;

        /**
         * @brief Increase the saturation of the color
         * @param amount Amount to increase saturation by (0.0-1.0)
         * @return Saturated color
         */
        Color saturate(float amount) const;

        /**
         * @brief Decrease the saturation of the color
         * @param amount Amount to decrease saturation by (0.0-1.0)
         * @return Desaturated color
         */
        Color desaturate(float amount) const;

        /**
         * @brief Lighten the color
         * @param amount Amount to lighten by (0.0-1.0)
         * @return Lightened color
         */
        Color lighten(float amount) const;

        /**
         * @brief Darken the color
         * @param amount Amount to darken by (0.0-1.0)
         * @return Darkened color
         */
        Color darken(float amount) const;

        /**
         * @brief Get the complementary color
         * @return Complementary color (opposite on the color wheel)
         */
        Color complementary() const;

        /**
         * @brief Blend with another color using linear interpolation
         * @param other Color to blend with
         * @param factor Blend factor (0.0-1.0)
         * @return Blended color
         */
        Color blend(const Color& other, float factor) const;

        /**
         * @brief Multiply blend mode
         * @param other Color to blend with
         * @return Resulting color from multiply blend
         */
        Color multiply(const Color& other) const;

        /**
         * @brief Screen blend mode
         * @param other Color to blend with
         * @return Resulting color from screen blend
         */
        Color screen(const Color& other) const;

        /**
         * @brief Overlay blend mode
         * @param other Color to blend with
         * @return Resulting color from overlay blend
         */
        Color overlay(const Color& other) const;

        /**
         * @brief Generate analogous colors
         * @param angle Angle between analogous colors (degrees)
         * @return Vector of analogous colors
         */
        std::vector<Color> analogous(float angle = 30.0f) const;

        /**
         * @brief Generate triadic color harmony
         * @return Vector of triadic colors (including this color)
         */
        std::vector<Color> triadic() const;

        /**
         * @brief Generate tetradic color harmony
         * @return Vector of tetradic colors (including this color)
         */
        std::vector<Color> tetradic() const;

        /**
         * @brief Generate monochromatic color variations
         * @param steps Number of variations to generate
         * @return Vector of monochromatic colors
         */
        std::vector<Color> monochromatic(int steps = 5) const;

        /**
         * @brief Equality operator
         * @param other Color to compare with
         * @return True if colors are equal
         */
        bool operator==(const Color& other) const;

        /**
         * @brief Inequality operator
         * @param other Color to compare with
         * @return True if colors are not equal
         */
        bool operator!=(const Color& other) const;

        /**
         * @brief Addition operator
         * @param other Color to add
         * @return Sum of colors (clamped to valid range)
         */
        Color operator+(const Color& other) const;

        /**
         * @brief Subtraction operator
         * @param other Color to subtract
         * @return Difference of colors (clamped to valid range)
         */
        Color operator-(const Color& other) const;

        /**
         * @brief Scalar multiplication operator
         * @param scalar Value to multiply by
         * @return Scaled color (clamped to valid range)
         */
        Color operator*(float scalar) const;

        /**
         * @brief Scalar division operator
         * @param scalar Value to divide by
         * @return Divided color (clamped to valid range)
         */
        Color operator/(float scalar) const;

        // Predefined colors
        static const Color Black;
        static const Color White;
        static const Color Red;
        static const Color Green;
        static const Color Blue;
        static const Color Yellow;
        static const Color Cyan;
        static const Color Magenta;
        static const Color Transparent;
        static const Color Gray;
        static const Color Orange;
        static const Color Purple;
        static const Color Brown;
        static const Color Pink;
        static const Color Lime;
        static const Color Teal;
        static const Color Navy;
        static const Color Olive;
        static const Color Maroon;
        static const Color Silver;

    private:
        float m_r, m_g, m_b, m_a;

        /**
         * @brief Helper method for HSL/HSV conversions
         * @param p First parameter
         * @param q Second parameter
         * @param t Third parameter
         * @return Resulting component value
         */
        static float hueToRGB(float p, float q, float t);

        /**
         * @brief Clamp color components to valid range [0,1]
         */
        void clamp();
    };

} // namespace PixelCraft::Utility