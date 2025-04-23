// -------------------------------------------------------------------------
// Random.cpp
// -------------------------------------------------------------------------
#include "Utility/Random.h"
#include "Core/Logger.h"
#include <ctime>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <mutex>
#include <glm/gtc/constants.hpp>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Utility
{

    // Initialize thread-local engine
    thread_local std::mt19937 Random::tl_engine;

    // Singleton instance management
    namespace
    {
        std::unique_ptr<Random> s_instance;
        std::once_flag s_onceFlag;

        void createInstance()
        {
            s_instance = std::make_unique<Random>();
        }
    }

    Random::Random(uint32_t seed)
        : m_floatDist(0.0f, 1.0f),
        m_doubleDist(0.0, 1.0),
        m_intDist(std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max()),
        m_normalDist(0.0f, 1.0f)
    {
        if (seed == 0)
        {
            seedFromTime();
        }
        else
        {
            this->seed(seed);
        }
    }

    void Random::seed(uint32_t seed)
    {
        m_seed = seed;
        m_engine.seed(seed);

        // For thread-local engine, use the base seed plus a hash of the thread ID
        // to ensure different threads get different sequences
        tl_engine.seed(seed + std::hash<std::thread::id>{}(std::this_thread::get_id()));
    }

    void Random::seedFromTime()
    {
        uint32_t timeSeed = static_cast<uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        seed(timeSeed);
    }

    uint32_t Random::getSeed() const
    {
        return m_seed;
    }

    int32_t Random::getInt()
    {
        return m_intDist(m_engine);
    }

    int32_t Random::getInt(int32_t min, int32_t max)
    {
        if (min > max)
        {
            std::swap(min, max);
        }

        std::uniform_int_distribution<int32_t> dist(min, max);
        return dist(m_engine);
    }

    uint32_t Random::getUint()
    {
        return static_cast<uint32_t>(m_engine());
    }

    uint32_t Random::getUint(uint32_t min, uint32_t max)
    {
        if (min > max)
        {
            std::swap(min, max);
        }

        std::uniform_int_distribution<uint32_t> dist(min, max);
        return dist(m_engine);
    }

    float Random::getFloat()
    {
        return m_floatDist(m_engine);
    }

    float Random::getFloat(float min, float max)
    {
        if (min > max)
        {
            std::swap(min, max);
        }

        return min + (max - min) * getFloat();
    }

    double Random::getDouble()
    {
        return m_doubleDist(m_engine);
    }

    double Random::getDouble(double min, double max)
    {
        if (min > max)
        {
            std::swap(min, max);
        }

        return min + (max - min) * getDouble();
    }

    bool Random::getBool()
    {
        return getFloat() < 0.5f;
    }

    bool Random::getBool(float probability)
    {
        probability = std::clamp(probability, 0.0f, 1.0f);
        return getFloat() < probability;
    }

    glm::vec2 Random::getVec2()
    {
        return glm::vec2(
            getFloat(),
            getFloat()
        );
    }

    glm::vec2 Random::getVec2(float min, float max)
    {
        return glm::vec2(
            getFloat(min, max),
            getFloat(min, max)
        );
    }

    glm::vec2 Random::getVec2(const glm::vec2& min, const glm::vec2& max)
    {
        return glm::vec2(
            getFloat(min.x, max.x),
            getFloat(min.y, max.y)
        );
    }

    glm::vec3 Random::getVec3()
    {
        return glm::vec3(
            getFloat(),
            getFloat(),
            getFloat()
        );
    }

    glm::vec3 Random::getVec3(float min, float max)
    {
        return glm::vec3(
            getFloat(min, max),
            getFloat(min, max),
            getFloat(min, max)
        );
    }

    glm::vec3 Random::getVec3(const glm::vec3& min, const glm::vec3& max)
    {
        return glm::vec3(
            getFloat(min.x, max.x),
            getFloat(min.y, max.y),
            getFloat(min.z, max.z)
        );
    }

    glm::vec4 Random::getVec4()
    {
        return glm::vec4(
            getFloat(),
            getFloat(),
            getFloat(),
            getFloat()
        );
    }

    glm::vec4 Random::getVec4(float min, float max)
    {
        return glm::vec4(
            getFloat(min, max),
            getFloat(min, max),
            getFloat(min, max),
            getFloat(min, max)
        );
    }

    glm::vec4 Random::getVec4(const glm::vec4& min, const glm::vec4& max)
    {
        return glm::vec4(
            getFloat(min.x, max.x),
            getFloat(min.y, max.y),
            getFloat(min.z, max.z),
            getFloat(min.w, max.w)
        );
    }

    glm::vec2 Random::getDir2D()
    {
        // Generate a random angle and convert to a unit vector
        float angle = getFloat(0.0f, 2.0f * glm::pi<float>());
        return glm::vec2(std::cos(angle), std::sin(angle));
    }

    glm::vec3 Random::getDir3D()
    {
        // Uniform distribution on a sphere
        // Use the method described by Marsaglia (1972)
        float z = getFloat(-1.0f, 1.0f);
        float phi = getFloat(0.0f, 2.0f * glm::pi<float>());
        float r = std::sqrt(1.0f - z * z);

        return glm::vec3(
            r * std::cos(phi),
            r * std::sin(phi),
            z
        );
    }

    glm::vec2 Random::getPointInCircle(float radius)
    {
        // Generate a point in the unit circle with uniform distribution
        // Use polar coordinates with adjusted radius for uniform distribution
        float r = radius * std::sqrt(getFloat());
        float theta = getFloat(0.0f, 2.0f * glm::pi<float>());

        return glm::vec2(
            r * std::cos(theta),
            r * std::sin(theta)
        );
    }

    glm::vec2 Random::getPointOnCircle(float radius)
    {
        // Generate a random angle and convert to point on circle
        float angle = getFloat(0.0f, 2.0f * glm::pi<float>());

        return glm::vec2(
            radius * std::cos(angle),
            radius * std::sin(angle)
        );
    }

    glm::vec3 Random::getPointInSphere(float radius)
    {
        // Generate a point in the unit sphere with uniform distribution
        // Use spherical coordinates with adjusted radius for uniform distribution
        float r = radius * std::cbrt(getFloat());  // Cube root for uniform distribution
        float theta = getFloat(0.0f, 2.0f * glm::pi<float>());
        float phi = std::acos(2.0f * getFloat() - 1.0f);

        return glm::vec3(
            r * std::sin(phi) * std::cos(theta),
            r * std::sin(phi) * std::sin(theta),
            r * std::cos(phi)
        );
    }

    glm::vec3 Random::getPointOnSphere(float radius)
    {
        // Generate a random direction and scale to radius
        return radius * getDir3D();
    }

    glm::vec3 Random::getPointInCube(float size)
    {
        float halfSize = size * 0.5f;
        return getVec3(-halfSize, halfSize);
    }

    glm::vec3 Random::getPointInBox(const glm::vec3& dimensions)
    {
        glm::vec3 halfDim = dimensions * 0.5f;
        return getVec3(-halfDim, halfDim);
    }

    float Random::getAngle()
    {
        return getFloat(0.0f, 2.0f * glm::pi<float>());
    }

    glm::quat Random::getQuat()
    {
        // Generate a random rotation quaternion
        // Based on "Uniform Random Rotations" by Ken Shoemake
        float u1 = getFloat();
        float u2 = getFloat(0.0f, 2.0f * glm::pi<float>());
        float u3 = getFloat(0.0f, 2.0f * glm::pi<float>());

        float sq1 = std::sqrt(1.0f - u1);
        float sq2 = std::sqrt(u1);

        return glm::quat(
            sq1 * std::sin(u2),
            sq1 * std::cos(u2),
            sq2 * std::sin(u3),
            sq2 * std::cos(u3)
        );
    }

    float Random::getNormal(float mean, float stdDev)
    {
        // Use standard normal distribution and transform to desired parameters
        return mean + stdDev * m_normalDist(m_engine);
    }

    float Random::getExponential(float lambda)
    {
        std::exponential_distribution<float> dist(lambda);
        return dist(m_engine);
    }

    Color Random::getColor(bool randomAlpha)
    {
        if (randomAlpha)
        {
            return Color(getFloat(), getFloat(), getFloat(), getFloat());
        }
        else
        {
            return Color(getFloat(), getFloat(), getFloat(), 1.0f);
        }
    }

    Color Random::getColorHSV(float minHue, float maxHue,
                              float minSat, float maxSat,
                              float minVal, float maxVal,
                              float alpha)
    {
        // Generate random HSV values in the specified ranges
        float h = getFloat(minHue, maxHue);
        float s = getFloat(minSat, maxSat);
        float v = getFloat(minVal, maxVal);

        // Ensure hue is in the range [0, 360)
        h = std::fmod(h, 360.0f);
        if (h < 0.0f) h += 360.0f;

        // HSV to RGB conversion
        float c = v * s;
        float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
        float m = v - c;

        float r, g, b;

        if (h < 60.0f)
        {
            r = c; g = x; b = 0.0f;
        }
        else if (h < 120.0f)
        {
            r = x; g = c; b = 0.0f;
        }
        else if (h < 180.0f)
        {
            r = 0.0f; g = c; b = x;
        }
        else if (h < 240.0f)
        {
            r = 0.0f; g = x; b = c;
        }
        else if (h < 300.0f)
        {
            r = x; g = 0.0f; b = c;
        }
        else
        {
            r = c; g = 0.0f; b = x;
        }

        return Color(r + m, g + m, b + m, alpha);
    }

    bool Random::chance(float probability)
    {
        return getBool(probability);
    }

    int Random::roll(int sides)
    {
        if (sides <= 0)
        {
            Log::error("Random::roll: Cannot roll a die with non-positive number of sides");
            return 0;
        }
        return getInt(1, sides);
    }

    int Random::rollDice(int count, int sides)
    {
        if (count <= 0 || sides <= 0)
        {
            Log::error("Random::rollDice: Cannot roll non-positive count or sides");
            return 0;
        }

        int sum = 0;
        for (int i = 0; i < count; ++i)
        {
            sum += roll(sides);
        }
        return sum;
    }

    Random& Random::getInstance()
    {
        std::call_once(s_onceFlag, createInstance);
        return *s_instance;
    }

    // Global convenience functions
    int32_t randomInt(int32_t min, int32_t max)
    {
        return Random::getInstance().getInt(min, max);
    }

    float randomFloat(float min, float max)
    {
        return Random::getInstance().getFloat(min, max);
    }

    bool randomBool(float probability)
    {
        return Random::getInstance().getBool(probability);
    }

    glm::vec3 randomDir3D()
    {
        return Random::getInstance().getDir3D();
    }

    Color randomColor()
    {
        return Random::getInstance().getColor();
    }

} // namespace PixelCraft::Utility