// -------------------------------------------------------------------------
// Random.h
// -------------------------------------------------------------------------
#pragma once

#include "Utility/Color.h"
#include <random>
#include <memory>
#include <vector>
#include <functional>
#include <thread>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace PixelCraft::Utility
{

    /**
     * @brief Provides robust random number generation capabilities for various game development needs
     *
     * The Random class offers high-quality random number generation with support for
     * different distributions and common game development use cases such as random
     * vectors, directions, positions, and color generation. It supports deterministic
     * seeding for reproducible results and thread-safe operation.
     */
    class Random
    {
    public:
        /**
         * @brief Constructor with optional seed
         * @param seed Initial seed value (0 means seed from time)
         */
        Random(uint32_t seed = 0);

        /**
         * @brief Set the random number generator seed
         * @param seed The seed value to use
         */
        void seed(uint32_t seed);

        /**
         * @brief Seed the random number generator with current time
         */
        void seedFromTime();

        /**
         * @brief Get the current seed value
         * @return The current seed value
         */
        uint32_t getSeed() const;

        /**
         * @brief Get a random integer covering the full int32_t range
         * @return Random integer
         */
        int32_t getInt();

        /**
         * @brief Get a random integer in the specified range [min, max]
         * @param min Minimum value (inclusive)
         * @param max Maximum value (inclusive)
         * @return Random integer in the range [min, max]
         */
        int32_t getInt(int32_t min, int32_t max);

        /**
         * @brief Get a random unsigned integer covering the full uint32_t range
         * @return Random unsigned integer
         */
        uint32_t getUint();

        /**
         * @brief Get a random unsigned integer in the specified range [min, max]
         * @param min Minimum value (inclusive)
         * @param max Maximum value (inclusive)
         * @return Random unsigned integer in the range [min, max]
         */
        uint32_t getUint(uint32_t min, uint32_t max);

        /**
         * @brief Get a random float in the range [0.0, 1.0)
         * @return Random float
         */
        float getFloat();

        /**
         * @brief Get a random float in the specified range [min, max)
         * @param min Minimum value (inclusive)
         * @param max Maximum value (exclusive)
         * @return Random float in the range [min, max)
         */
        float getFloat(float min, float max);

        /**
         * @brief Get a random double in the range [0.0, 1.0)
         * @return Random double
         */
        double getDouble();

        /**
         * @brief Get a random double in the specified range [min, max)
         * @param min Minimum value (inclusive)
         * @param max Maximum value (exclusive)
         * @return Random double in the range [min, max)
         */
        double getDouble(double min, double max);

        /**
         * @brief Get a random boolean with 50% probability
         * @return Random boolean
         */
        bool getBool();

        /**
         * @brief Get a random boolean with specified probability
         * @param probability Probability of returning true (0.0 to 1.0)
         * @return Random boolean
         */
        bool getBool(float probability);

        /**
         * @brief Get a random 2D vector with components in the range [0.0, 1.0)
         * @return Random 2D vector
         */
        glm::vec2 getVec2();

        /**
         * @brief Get a random 2D vector with components in the specified range [min, max)
         * @param min Minimum component value (inclusive)
         * @param max Maximum component value (exclusive)
         * @return Random 2D vector
         */
        glm::vec2 getVec2(float min, float max);

        /**
         * @brief Get a random 2D vector with components in the specified range [min.x, max.x), [min.y, max.y)
         * @param min Minimum component values (inclusive)
         * @param max Maximum component values (exclusive)
         * @return Random 2D vector
         */
        glm::vec2 getVec2(const glm::vec2& min, const glm::vec2& max);

        /**
         * @brief Get a random 3D vector with components in the range [0.0, 1.0)
         * @return Random 3D vector
         */
        glm::vec3 getVec3();

        /**
         * @brief Get a random 3D vector with components in the specified range [min, max)
         * @param min Minimum component value (inclusive)
         * @param max Maximum component value (exclusive)
         * @return Random 3D vector
         */
        glm::vec3 getVec3(float min, float max);

        /**
         * @brief Get a random 3D vector with components in the specified range [min.x, max.x), [min.y, max.y), [min.z, max.z)
         * @param min Minimum component values (inclusive)
         * @param max Maximum component values (exclusive)
         * @return Random 3D vector
         */
        glm::vec3 getVec3(const glm::vec3& min, const glm::vec3& max);

        /**
         * @brief Get a random 4D vector with components in the range [0.0, 1.0)
         * @return Random 4D vector
         */
        glm::vec4 getVec4();

        /**
         * @brief Get a random 4D vector with components in the specified range [min, max)
         * @param min Minimum component value (inclusive)
         * @param max Maximum component value (exclusive)
         * @return Random 4D vector
         */
        glm::vec4 getVec4(float min, float max);

        /**
         * @brief Get a random 4D vector with components in the specified range [min.x, max.x), [min.y, max.y), [min.z, max.z), [min.w, max.w)
         * @param min Minimum component values (inclusive)
         * @param max Maximum component values (exclusive)
         * @return Random 4D vector
         */
        glm::vec4 getVec4(const glm::vec4& min, const glm::vec4& max);

        /**
         * @brief Get a random 2D unit direction vector
         * @return Normalized random 2D direction vector
         */
        glm::vec2 getDir2D();

        /**
         * @brief Get a random 3D unit direction vector
         * @return Normalized random 3D direction vector
         */
        glm::vec3 getDir3D();

        /**
         * @brief Get a random point inside a circle of specified radius
         * @param radius The radius of the circle
         * @return Random point inside the circle
         */
        glm::vec2 getPointInCircle(float radius = 1.0f);

        /**
         * @brief Get a random point on the perimeter of a circle of specified radius
         * @param radius The radius of the circle
         * @return Random point on the circle perimeter
         */
        glm::vec2 getPointOnCircle(float radius = 1.0f);

        /**
         * @brief Get a random point inside a sphere of specified radius
         * @param radius The radius of the sphere
         * @return Random point inside the sphere
         */
        glm::vec3 getPointInSphere(float radius = 1.0f);

        /**
         * @brief Get a random point on the surface of a sphere of specified radius
         * @param radius The radius of the sphere
         * @return Random point on the sphere surface
         */
        glm::vec3 getPointOnSphere(float radius = 1.0f);

        /**
         * @brief Get a random point inside a cube of specified size
         * @param size The size of the cube (edge length)
         * @return Random point inside the cube
         */
        glm::vec3 getPointInCube(float size = 1.0f);

        /**
         * @brief Get a random point inside a box of specified dimensions
         * @param dimensions The dimensions of the box
         * @return Random point inside the box
         */
        glm::vec3 getPointInBox(const glm::vec3& dimensions);

        /**
         * @brief Get a random angle in the range [0, 2π)
         * @return Random angle in radians
         */
        float getAngle();

        /**
         * @brief Get a random quaternion representing a rotation
         * @return Random quaternion (normalized)
         */
        glm::quat getQuat();

        /**
         * @brief Get a random value from a normal distribution
         * @param mean Mean value of the distribution
         * @param stdDev Standard deviation of the distribution
         * @return Random value from the normal distribution
         */
        float getNormal(float mean = 0.0f, float stdDev = 1.0f);

        /**
         * @brief Get a random value from an exponential distribution
         * @param lambda Rate parameter of the distribution
         * @return Random value from the exponential distribution
         */
        float getExponential(float lambda = 1.0f);

        /**
         * @brief Get a random color with optional random alpha
         * @param randomAlpha Whether to randomize the alpha component
         * @return Random color
         */
        Color getColor(bool randomAlpha = false);

        /**
         * @brief Get a random color using the HSV color model
         * @param minHue Minimum hue value (degrees)
         * @param maxHue Maximum hue value (degrees)
         * @param minSat Minimum saturation value [0.0, 1.0]
         * @param maxSat Maximum saturation value [0.0, 1.0]
         * @param minVal Minimum value/brightness [0.0, 1.0]
         * @param maxVal Maximum value/brightness [0.0, 1.0]
         * @param alpha Alpha value [0.0, 1.0]
         * @return Random color in the specified HSV range
         */
        Color getColorHSV(float minHue = 0.0f, float maxHue = 360.0f,
                          float minSat = 0.0f, float maxSat = 1.0f,
                          float minVal = 0.0f, float maxVal = 1.0f,
                          float alpha = 1.0f);

        /**
         * @brief Get a weighted random item from a container
         * @tparam T Type of items in the container
         * @param items Container of items to choose from
         * @param weights Container of weights corresponding to the items
         * @return Randomly selected item based on weights
         * @throws std::invalid_argument if containers are empty or have different sizes
         */
        template<typename T>
        T getWeightedRandom(const std::vector<T>& items, const std::vector<float>& weights);

        /**
         * @brief Choose a random item from a container
         * @tparam Container Container type
         * @param container Container to choose from
         * @return Reference to the randomly selected item
         * @throws std::invalid_argument if container is empty
         */
        template<typename Container>
        auto& choose(Container& container);

        /**
         * @brief Shuffle the elements of a container randomly
         * @tparam Container Container type
         * @param container Container to shuffle
         */
        template<typename Container>
        void shuffle(Container& container);

        /**
         * @brief Sample a specified number of elements from a container without replacement
         * @tparam Container Container type
         * @param container Container to sample from
         * @param count Number of elements to sample
         * @return Vector containing the sampled elements
         */
        template<typename Container>
        std::vector<typename Container::value_type> sample(const Container& container, size_t count);

        /**
         * @brief Test if an event with the given probability occurs
         * @param probability Probability of the event [0.0, 1.0]
         * @return True if the event occurs, false otherwise
         */
        bool chance(float probability);

        /**
         * @brief Simulate rolling a die with the specified number of sides
         * @param sides Number of sides on the die
         * @return Result of the die roll [1, sides]
         */
        int roll(int sides);

        /**
         * @brief Simulate rolling multiple dice and summing the results
         * @param count Number of dice to roll
         * @param sides Number of sides on each die
         * @return Sum of the dice rolls
         */
        int rollDice(int count, int sides);

        /**
         * @brief Get the global random number generator instance
         * @return Reference to the global random instance
         */
        static Random& getInstance();

    private:
        std::mt19937 m_engine;
        uint32_t m_seed;

        // Thread-local random engine
        static thread_local std::mt19937 tl_engine;

        // Standard distributions
        std::uniform_real_distribution<float> m_floatDist;
        std::uniform_real_distribution<double> m_doubleDist;
        std::uniform_int_distribution<int32_t> m_intDist;
        std::normal_distribution<float> m_normalDist;
    };

    // Template method implementations
    template<typename T>
    T Random::getWeightedRandom(const std::vector<T>& items, const std::vector<float>& weights)
    {
        if (items.empty() || weights.empty() || items.size() != weights.size())
        {
            throw std::invalid_argument("Invalid items or weights for weighted random selection");
        }

        // Calculate sum of weights
        float sum = 0.0f;
        for (float w : weights)
        {
            sum += w;
        }

        // Get a random value between 0 and sum
        float value = getFloat() * sum;

        // Find the item that corresponds to the random value
        float cumulative = 0.0f;
        for (size_t i = 0; i < items.size(); ++i)
        {
            cumulative += weights[i];
            if (value < cumulative)
            {
                return items[i];
            }
        }

        // Fallback (should not happen unless floating point precision issues)
        return items.back();
    }

    template<typename Container>
    auto& Random::choose(Container& container)
    {
        if (container.empty())
        {
            throw std::invalid_argument("Cannot choose from empty container");
        }

        auto it = container.begin();
        std::advance(it, getInt(0, static_cast<int32_t>(container.size() - 1)));
        return *it;
    }

    template<typename Container>
    void Random::shuffle(Container& container)
    {
        std::shuffle(container.begin(), container.end(), m_engine);
    }

    template<typename Container>
    std::vector<typename Container::value_type> Random::sample(const Container& container, size_t count)
    {
        if (container.empty())
        {
            return std::vector<typename Container::value_type>();
        }

        if (count >= container.size())
        {
            // If requesting all or more than available, return a copy of the whole container
            return std::vector<typename Container::value_type>(container.begin(), container.end());
        }

        // Create a copy to shuffle
        std::vector<typename Container::value_type> result(container.begin(), container.end());

        // Shuffle and take the first 'count' elements
        shuffle(result);
        result.resize(count);

        return result;
    }

    // Global convenience functions
    /**
     * @brief Get a random integer in the specified range [min, max]
     * @param min Minimum value (inclusive)
     * @param max Maximum value (inclusive)
     * @return Random integer in the range [min, max]
     */
    int32_t randomInt(int32_t min, int32_t max);

    /**
     * @brief Get a random float in the specified range [min, max)
     * @param min Minimum value (inclusive)
     * @param max Maximum value (exclusive)
     * @return Random float in the range [min, max)
     */
    float randomFloat(float min = 0.0f, float max = 1.0f);

    /**
     * @brief Get a random boolean with specified probability
     * @param probability Probability of returning true (0.0 to 1.0)
     * @return Random boolean
     */
    bool randomBool(float probability = 0.5f);

    /**
     * @brief Get a random 3D unit direction vector
     * @return Normalized random 3D direction vector
     */
    glm::vec3 randomDir3D();

    /**
     * @brief Get a random color
     * @return Random color
     */
    Color randomColor();

} // namespace PixelCraft::Utility