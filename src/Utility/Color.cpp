// -------------------------------------------------------------------------
// Color.cpp
// -------------------------------------------------------------------------
#include "Utility/Color.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <random>
#include <stdexcept>

namespace PixelCraft::Utility
{

    // Define static constant colors
    const Color Color::Black(0.0f, 0.0f, 0.0f, 1.0f);
    const Color Color::White(1.0f, 1.0f, 1.0f, 1.0f);
    const Color Color::Red(1.0f, 0.0f, 0.0f, 1.0f);
    const Color Color::Green(0.0f, 1.0f, 0.0f, 1.0f);
    const Color Color::Blue(0.0f, 0.0f, 1.0f, 1.0f);
    const Color Color::Yellow(1.0f, 1.0f, 0.0f, 1.0f);
    const Color Color::Cyan(0.0f, 1.0f, 1.0f, 1.0f);
    const Color Color::Magenta(1.0f, 0.0f, 1.0f, 1.0f);
    const Color Color::Transparent(0.0f, 0.0f, 0.0f, 0.0f);
    const Color Color::Gray(0.5f, 0.5f, 0.5f, 1.0f);
    const Color Color::Orange(1.0f, 0.5f, 0.0f, 1.0f);
    const Color Color::Purple(0.5f, 0.0f, 0.5f, 1.0f);
    const Color Color::Brown(0.6f, 0.4f, 0.2f, 1.0f);
    const Color Color::Pink(1.0f, 0.75f, 0.8f, 1.0f);
    const Color Color::Lime(0.75f, 1.0f, 0.0f, 1.0f);
    const Color Color::Teal(0.0f, 0.5f, 0.5f, 1.0f);
    const Color Color::Navy(0.0f, 0.0f, 0.5f, 1.0f);
    const Color Color::Olive(0.5f, 0.5f, 0.0f, 1.0f);
    const Color Color::Maroon(0.5f, 0.0f, 0.0f, 1.0f);
    const Color Color::Silver(0.75f, 0.75f, 0.75f, 1.0f);

    Color::Color() : m_r(0.0f), m_g(0.0f), m_b(0.0f), m_a(1.0f)
    {
    }

    Color::Color(float r, float g, float b, float a) : m_r(r), m_g(g), m_b(b), m_a(a)
    {
        clamp();
    }

    Color::Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
        : m_r(r / 255.0f), m_g(g / 255.0f), m_b(b / 255.0f), m_a(a / 255.0f)
    {
        // No need to clamp as division by 255 ensures range [0,1]
    }

    Color::Color(uint32_t rgba)
    {
        setFromInt(rgba);
    }

    Color Color::fromHSV(float h, float s, float v, float a)
    {
        Color result;
        result.setHSV(h, s, v);
        result.setA(a);
        return result;
    }

    Color Color::fromHSL(float h, float s, float l, float a)
    {
        Color result;
        result.setHSL(h, s, l);
        result.setA(a);
        return result;
    }

    Color Color::fromString(const std::string& hexString)
    {
        if (hexString.empty())
        {
            return Color::Black;
        }

        // Skip # if present
        size_t start = (hexString[0] == '#') ? 1 : 0;
        size_t len = hexString.length() - start;

        uint32_t value = 0;
        try
        {
            value = std::stoul(hexString.substr(start), nullptr, 16);
        }
        catch (const std::exception&)
        {
            return Color::Black; // Return black on invalid input
        }

        // Format detection
        if (len == 3)
        {
            // #RGB format, expand to #RRGGBB
            uint8_t r = ((value & 0xF00) >> 8) * 17; // * 17 to expand 0xR to 0xRR
            uint8_t g = ((value & 0x0F0) >> 4) * 17;
            uint8_t b = (value & 0x00F) * 17;
            return Color(r, g, b);
        }
        else if (len == 4)
        {
            // #RGBA format, expand to #RRGGBBAA
            uint8_t r = ((value & 0xF000) >> 12) * 17;
            uint8_t g = ((value & 0x0F00) >> 8) * 17;
            uint8_t b = ((value & 0x00F0) >> 4) * 17;
            uint8_t a = (value & 0x000F) * 17;
            return Color(r, g, b, a);
        }
        else if (len == 6)
        {
            // #RRGGBB format
            uint8_t r = (value & 0xFF0000) >> 16;
            uint8_t g = (value & 0x00FF00) >> 8;
            uint8_t b = (value & 0x0000FF);
            return Color(r, g, b);
        }
        else if (len == 8)
        {
            // #RRGGBBAA format
            uint8_t r = (value & 0xFF000000) >> 24;
            uint8_t g = (value & 0x00FF0000) >> 16;
            uint8_t b = (value & 0x0000FF00) >> 8;
            uint8_t a = (value & 0x000000FF);
            return Color(r, g, b, a);
        }

        return Color::Black; // Default for invalid formats
    }

    Color Color::lerp(const Color& a, const Color& b, float t)
    {
        // Clamp t to [0,1]
        t = std::max(0.0f, std::min(1.0f, t));

        return Color(
            a.m_r + (b.m_r - a.m_r) * t,
            a.m_g + (b.m_g - a.m_g) * t,
            a.m_b + (b.m_b - a.m_b) * t,
            a.m_a + (b.m_a - a.m_a) * t
        );
    }

    Color Color::random(float alpha)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> dis(0.0f, 1.0f);

        return Color(dis(gen), dis(gen), dis(gen), alpha);
    }

    float Color::r() const
    {
        return m_r;
    }
    float Color::g() const
    {
        return m_g;
    }
    float Color::b() const
    {
        return m_b;
    }
    float Color::a() const
    {
        return m_a;
    }

    void Color::setR(float r)
    {
        m_r = std::max(0.0f, std::min(1.0f, r));
    }
    void Color::setG(float g)
    {
        m_g = std::max(0.0f, std::min(1.0f, g));
    }
    void Color::setB(float b)
    {
        m_b = std::max(0.0f, std::min(1.0f, b));
    }
    void Color::setA(float a)
    {
        m_a = std::max(0.0f, std::min(1.0f, a));
    }

    uint8_t Color::rByte() const
    {
        return static_cast<uint8_t>(m_r * 255.0f);
    }
    uint8_t Color::gByte() const
    {
        return static_cast<uint8_t>(m_g * 255.0f);
    }
    uint8_t Color::bByte() const
    {
        return static_cast<uint8_t>(m_b * 255.0f);
    }
    uint8_t Color::aByte() const
    {
        return static_cast<uint8_t>(m_a * 255.0f);
    }

    void Color::setRByte(uint8_t r)
    {
        m_r = r / 255.0f;
    }
    void Color::setGByte(uint8_t g)
    {
        m_g = g / 255.0f;
    }
    void Color::setBByte(uint8_t b)
    {
        m_b = b / 255.0f;
    }
    void Color::setAByte(uint8_t a)
    {
        m_a = a / 255.0f;
    }

    glm::vec3 Color::rgb() const
    {
        return glm::vec3(m_r, m_g, m_b);
    }

    glm::vec4 Color::rgba() const
    {
        return glm::vec4(m_r, m_g, m_b, m_a);
    }

    void Color::setRGB(const glm::vec3& rgb)
    {
        m_r = rgb.r;
        m_g = rgb.g;
        m_b = rgb.b;
        clamp();
    }

    void Color::setRGBA(const glm::vec4& rgba)
    {
        m_r = rgba.r;
        m_g = rgba.g;
        m_b = rgba.b;
        m_a = rgba.a;
        clamp();
    }

    uint32_t Color::toInt() const
    {
        uint32_t r = static_cast<uint32_t>(m_r * 255.0f) & 0xFF;
        uint32_t g = static_cast<uint32_t>(m_g * 255.0f) & 0xFF;
        uint32_t b = static_cast<uint32_t>(m_b * 255.0f) & 0xFF;
        uint32_t a = static_cast<uint32_t>(m_a * 255.0f) & 0xFF;

        return (r << 24) | (g << 16) | (b << 8) | a;
    }

    void Color::setFromInt(uint32_t rgba)
    {
        m_r = ((rgba >> 24) & 0xFF) / 255.0f;
        m_g = ((rgba >> 16) & 0xFF) / 255.0f;
        m_b = ((rgba >> 8) & 0xFF) / 255.0f;
        m_a = (rgba & 0xFF) / 255.0f;
    }

    glm::vec3 Color::toHSV() const
    {
        float r = m_r;
        float g = m_g;
        float b = m_b;

        float max = std::max(std::max(r, g), b);
        float min = std::min(std::min(r, g), b);
        float delta = max - min;

        // Hue calculation
        float h = 0.0f;
        if (delta > 0.0f)
        {
            if (max == r)
            {
                h = 60.0f * std::fmod(((g - b) / delta), 6.0f);
            }
            else if (max == g)
            {
                h = 60.0f * (((b - r) / delta) + 2.0f);
            }
            else if (max == b)
            {
                h = 60.0f * (((r - g) / delta) + 4.0f);
            }

            if (h < 0.0f)
            {
                h += 360.0f;
            }
        }

        // Saturation calculation
        float s = (max > 0.0f) ? (delta / max) : 0.0f;

        // Value is simply the max
        float v = max;

        return glm::vec3(h, s, v);
    }

    glm::vec3 Color::toHSL() const
    {
        float r = m_r;
        float g = m_g;
        float b = m_b;

        float max = std::max(std::max(r, g), b);
        float min = std::min(std::min(r, g), b);
        float delta = max - min;

        // Lightness is the average of max and min
        float l = (max + min) / 2.0f;

        // Hue calculation
        float h = 0.0f;
        if (delta > 0.0f)
        {
            if (max == r)
            {
                h = 60.0f * std::fmod(((g - b) / delta), 6.0f);
            }
            else if (max == g)
            {
                h = 60.0f * (((b - r) / delta) + 2.0f);
            }
            else if (max == b)
            {
                h = 60.0f * (((r - g) / delta) + 4.0f);
            }

            if (h < 0.0f)
            {
                h += 360.0f;
            }
        }

        // Saturation calculation
        float s = 0.0f;
        if (delta > 0.0f)
        {
            s = delta / (1.0f - std::abs(2.0f * l - 1.0f));
        }

        return glm::vec3(h, s, l);
    }

    void Color::setHSV(float h, float s, float v)
    {
        // Clamp inputs
        h = std::fmod(h, 360.0f);
        if (h < 0.0f) h += 360.0f;
        s = std::max(0.0f, std::min(1.0f, s));
        v = std::max(0.0f, std::min(1.0f, v));

        // HSV to RGB conversion
        float c = v * s; // Chroma
        float hp = h / 60.0f; // H'
        float x = c * (1.0f - std::abs(std::fmod(hp, 2.0f) - 1.0f));
        float m = v - c;

        float r, g, b;
        if (hp <= 1.0f)
        {
            r = c;
            g = x;
            b = 0.0f;
        }
        else if (hp <= 2.0f)
        {
            r = x;
            g = c;
            b = 0.0f;
        }
        else if (hp <= 3.0f)
        {
            r = 0.0f;
            g = c;
            b = x;
        }
        else if (hp <= 4.0f)
        {
            r = 0.0f;
            g = x;
            b = c;
        }
        else if (hp <= 5.0f)
        {
            r = x;
            g = 0.0f;
            b = c;
        }
        else
        {
            r = c;
            g = 0.0f;
            b = x;
        }

        m_r = r + m;
        m_g = g + m;
        m_b = b + m;
        // No need to clamp as the calculations should ensure range [0,1]
    }

    float Color::hueToRGB(float p, float q, float t)
    {
        if (t < 0.0f) t += 1.0f;
        if (t > 1.0f) t -= 1.0f;

        if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
        if (t < 1.0f / 2.0f) return q;
        if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;

        return p;
    }

    void Color::setHSL(float h, float s, float l)
    {
        // Clamp inputs
        h = std::fmod(h, 360.0f);
        if (h < 0.0f) h += 360.0f;
        s = std::max(0.0f, std::min(1.0f, s));
        l = std::max(0.0f, std::min(1.0f, l));

        // HSL to RGB conversion
        // If saturation is 0, the color is a shade of gray
        if (s == 0.0f)
        {
            m_r = m_g = m_b = l;
            return;
        }

        float q = (l < 0.5f) ? (l * (1.0f + s)) : (l + s - (l * s));
        float p = 2.0f * l - q;
        float hNorm = h / 360.0f; // Normalize hue to [0,1]

        m_r = hueToRGB(p, q, hNorm + 1.0f / 3.0f);
        m_g = hueToRGB(p, q, hNorm);
        m_b = hueToRGB(p, q, hNorm - 1.0f / 3.0f);
        // No need to clamp as hueToRGB ensures range [0,1]
    }

    std::string Color::toHexString(bool includeAlpha) const
    {
        std::stringstream ss;
        ss << '#';

        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(rByte());
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(gByte());
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(bByte());

        if (includeAlpha)
        {
            ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(aByte());
        }

        return ss.str();
    }

    std::string Color::toString() const
    {
        std::stringstream ss;
        if (m_a == 1.0f)
        {
            ss << "RGB(" << static_cast<int>(rByte()) << ", "
                << static_cast<int>(gByte()) << ", "
                << static_cast<int>(bByte()) << ")";
        }
        else
        {
            ss << "RGBA(" << static_cast<int>(rByte()) << ", "
                << static_cast<int>(gByte()) << ", "
                << static_cast<int>(bByte()) << ", "
                << m_a << ")";
        }
        return ss.str();
    }

    Color Color::withAlpha(float alpha) const
    {
        return Color(m_r, m_g, m_b, std::max(0.0f, std::min(1.0f, alpha)));
    }

    Color Color::saturate(float amount) const
    {
        glm::vec3 hsl = toHSL();
        hsl.y = std::min(1.0f, hsl.y + amount);

        Color result;
        result.setHSL(hsl.x, hsl.y, hsl.z);
        result.setA(m_a);
        return result;
    }

    Color Color::desaturate(float amount) const
    {
        glm::vec3 hsl = toHSL();
        hsl.y = std::max(0.0f, hsl.y - amount);

        Color result;
        result.setHSL(hsl.x, hsl.y, hsl.z);
        result.setA(m_a);
        return result;
    }

    Color Color::lighten(float amount) const
    {
        glm::vec3 hsl = toHSL();
        hsl.z = std::min(1.0f, hsl.z + amount);

        Color result;
        result.setHSL(hsl.x, hsl.y, hsl.z);
        result.setA(m_a);
        return result;
    }

    Color Color::darken(float amount) const
    {
        glm::vec3 hsl = toHSL();
        hsl.z = std::max(0.0f, hsl.z - amount);

        Color result;
        result.setHSL(hsl.x, hsl.y, hsl.z);
        result.setA(m_a);
        return result;
    }

    Color Color::complementary() const
    {
        glm::vec3 hsl = toHSL();
        // Add 180 degrees to hue for complementary color
        hsl.x = std::fmod(hsl.x + 180.0f, 360.0f);

        Color result;
        result.setHSL(hsl.x, hsl.y, hsl.z);
        result.setA(m_a);
        return result;
    }

    Color Color::blend(const Color& other, float factor) const
    {
        return lerp(*this, other, factor);
    }

    Color Color::multiply(const Color& other) const
    {
        return Color(
            m_r * other.m_r,
            m_g * other.m_g,
            m_b * other.m_b,
            m_a * other.m_a
        );
    }

    Color Color::screen(const Color& other) const
    {
        return Color(
            1.0f - (1.0f - m_r) * (1.0f - other.m_r),
            1.0f - (1.0f - m_g) * (1.0f - other.m_g),
            1.0f - (1.0f - m_b) * (1.0f - other.m_b),
            m_a * other.m_a
        );
    }

    Color Color::overlay(const Color& other) const
    {
        auto overlayComponent = [](float a, float b) -> float {
            return (a <= 0.5f) ? (2.0f * a * b) : (1.0f - 2.0f * (1.0f - a) * (1.0f - b));
            };

        return Color(
            overlayComponent(m_r, other.m_r),
            overlayComponent(m_g, other.m_g),
            overlayComponent(m_b, other.m_b),
            m_a * other.m_a
        );
    }

    std::vector<Color> Color::analogous(float angle) const
    {
        glm::vec3 hsl = toHSL();

        // Clamp angle to reasonable range
        angle = std::max(1.0f, std::min(90.0f, angle));

        // Create colors with hues on either side of the original
        std::vector<Color> result;
        result.push_back(*this);

        Color color1, color2;

        // Add angle to hue, wrapping around 360 degrees
        float hue1 = std::fmod(hsl.x + angle, 360.0f);
        color1.setHSL(hue1, hsl.y, hsl.z);
        color1.setA(m_a);
        result.push_back(color1);

        // Subtract angle from hue, wrapping around 360 degrees
        float hue2 = std::fmod(hsl.x - angle + 360.0f, 360.0f);
        color2.setHSL(hue2, hsl.y, hsl.z);
        color2.setA(m_a);
        result.push_back(color2);

        return result;
    }

    std::vector<Color> Color::triadic() const
    {
        glm::vec3 hsl = toHSL();

        // Create colors with hues 120 degrees apart
        std::vector<Color> result;
        result.push_back(*this);

        Color color1, color2;

        float hue1 = std::fmod(hsl.x + 120.0f, 360.0f);
        color1.setHSL(hue1, hsl.y, hsl.z);
        color1.setA(m_a);
        result.push_back(color1);

        float hue2 = std::fmod(hsl.x + 240.0f, 360.0f);
        color2.setHSL(hue2, hsl.y, hsl.z);
        color2.setA(m_a);
        result.push_back(color2);

        return result;
    }

    std::vector<Color> Color::tetradic() const
    {
        glm::vec3 hsl = toHSL();

        // Create a rectangle on the color wheel (90 degrees apart)
        std::vector<Color> result;
        result.push_back(*this);

        Color color1, color2, color3;

        float hue1 = std::fmod(hsl.x + 90.0f, 360.0f);
        color1.setHSL(hue1, hsl.y, hsl.z);
        color1.setA(m_a);
        result.push_back(color1);

        float hue2 = std::fmod(hsl.x + 180.0f, 360.0f);
        color2.setHSL(hue2, hsl.y, hsl.z);
        color2.setA(m_a);
        result.push_back(color2);

        float hue3 = std::fmod(hsl.x + 270.0f, 360.0f);
        color3.setHSL(hue3, hsl.y, hsl.z);
        color3.setA(m_a);
        result.push_back(color3);

        return result;
    }

    std::vector<Color> Color::monochromatic(int steps) const
    {
        if (steps < 2) steps = 2;

        glm::vec3 hsl = toHSL();
        std::vector<Color> result;

        // Vary lightness to create monochromatic colors
        for (int i = 0; i < steps; ++i)
        {
            float t = static_cast<float>(i) / (steps - 1);
            float l = t * 0.8f + 0.1f; // Range from 0.1 to 0.9

            Color color;
            color.setHSL(hsl.x, hsl.y, l);
            color.setA(m_a);
            result.push_back(color);
        }

        return result;
    }

    bool Color::operator==(const Color& other) const
    {
        const float epsilon = 0.00001f;
        return std::abs(m_r - other.m_r) < epsilon &&
            std::abs(m_g - other.m_g) < epsilon &&
            std::abs(m_b - other.m_b) < epsilon &&
            std::abs(m_a - other.m_a) < epsilon;
    }

    bool Color::operator!=(const Color& other) const
    {
        return !(*this == other);
    }

    Color Color::operator+(const Color& other) const
    {
        return Color(
            m_r + other.m_r,
            m_g + other.m_g,
            m_b + other.m_b,
            m_a + other.m_a
        );
    }

    Color Color::operator-(const Color& other) const
    {
        return Color(
            m_r - other.m_r,
            m_g - other.m_g,
            m_b - other.m_b,
            m_a - other.m_a
        );
    }

    Color Color::operator*(float scalar) const
    {
        return Color(
            m_r * scalar,
            m_g * scalar,
            m_b * scalar,
            m_a * scalar
        );
    }

    Color Color::operator/(float scalar) const
    {
        if (std::abs(scalar) < 0.00001f)
        {
            // Prevent division by zero
            return *this;
        }

        float invScalar = 1.0f / scalar;
        return *this * invScalar;
    }

    void Color::clamp()
    {
        m_r = std::max(0.0f, std::min(1.0f, m_r));
        m_g = std::max(0.0f, std::min(1.0f, m_g));
        m_b = std::max(0.0f, std::min(1.0f, m_b));
        m_a = std::max(0.0f, std::min(1.0f, m_a));
    }

} // namespace PixelCraft::Utility