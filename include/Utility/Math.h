// -------------------------------------------------------------------------
// Math.h
// -------------------------------------------------------------------------
#pragma once

#include <random>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace PixelCraft::Utility
{

    /**
     * @brief A utility class providing common mathematical functions and constants for game development.
     *
     * The Math class offers a wide range of optimized mathematical operations beyond the standard library,
     * including interpolation, angle manipulation, random number generation, noise functions, vector and
     * matrix operations, geometry utilities, and SIMD-optimized routines.
     */
    class Math
    {
    public:
        // Constants
        static constexpr float PI = 3.14159265358979323846f;
        static constexpr float TWO_PI = PI * 2.0f;
        static constexpr float HALF_PI = PI * 0.5f;
        static constexpr float DEG_TO_RAD = PI / 180.0f;
        static constexpr float RAD_TO_DEG = 180.0f / PI;
        static constexpr float EPSILON = 1.0e-6f;

        /**
         * @brief Convert an angle from degrees to radians.
         *
         * @param degrees The angle in degrees
         * @return The angle converted to radians
         */
        static inline float degreesToRadians(float degrees)
        {
            return degrees * DEG_TO_RAD;
        }

        /**
         * @brief Convert an angle from radians to degrees.
         *
         * @param radians The angle in radians
         * @return The angle converted to degrees
         */
        static inline float radiansToDegrees(float radians)
        {
            return radians * RAD_TO_DEG;
        }

        /**
         * @brief Linearly interpolate between two scalar values.
         *
         * @param a Starting value
         * @param b Ending value
         * @param t Interpolation factor (0.0 to 1.0)
         * @return The interpolated value
         */
        static float lerp(float a, float b, float t);

        /**
         * @brief Linearly interpolate between two 2D vectors.
         *
         * @param a Starting vector
         * @param b Ending vector
         * @param t Interpolation factor (0.0 to 1.0)
         * @return The interpolated vector
         */
        static glm::vec2 lerp(const glm::vec2& a, const glm::vec2& b, float t);

        /**
         * @brief Linearly interpolate between two 3D vectors.
         *
         * @param a Starting vector
         * @param b Ending vector
         * @param t Interpolation factor (0.0 to 1.0)
         * @return The interpolated vector
         */
        static glm::vec3 lerp(const glm::vec3& a, const glm::vec3& b, float t);

        /**
         * @brief Linearly interpolate between two 4D vectors.
         *
         * @param a Starting vector
         * @param b Ending vector
         * @param t Interpolation factor (0.0 to 1.0)
         * @return The interpolated vector
         */
        static glm::vec4 lerp(const glm::vec4& a, const glm::vec4& b, float t);

        /**
         * @brief Perform a smooth step interpolation between two values.
         *
         * This produces a smooth transition that accelerates and decelerates,
         * following a Hermite curve (3t² - 2t³).
         *
         * @param a Starting value
         * @param b Ending value
         * @param t Interpolation factor (0.0 to 1.0)
         * @return The interpolated value
         */
        static float smoothStep(float a, float b, float t);

        /**
         * @brief Perform a smoother step interpolation between two values.
         *
         * This produces an even smoother transition than smoothStep,
         * following the curve (6t^5 - 15t^4 + 10t^3).
         *
         * @param a Starting value
         * @param b Ending value
         * @param t Interpolation factor (0.0 to 1.0)
         * @return The interpolated value
         */
        static float smootherStep(float a, float b, float t);

        /**
         * @brief Clamp a float value between a minimum and maximum value.
         *
         * @param value The value to clamp
         * @param min The minimum value
         * @param max The maximum value
         * @return The clamped value
         */
        static float clamp(float value, float min, float max);

        /**
         * @brief Clamp an integer value between a minimum and maximum value.
         *
         * @param value The value to clamp
         * @param min The minimum value
         * @param max The maximum value
         * @return The clamped value
         */
        static int clamp(int value, int min, int max);

        /**
         * @brief Clamp a float value to the range [0, 1].
         *
         * @param value The value to saturate
         * @return The saturated value
         */
        static inline float saturate(float value)
        {
            return clamp(value, 0.0f, 1.0f);
        }

        /**
         * @brief Wrap an angle to the range [-PI, PI].
         *
         * @param angle The angle in radians to wrap
         * @return The wrapped angle
         */
        static float wrapAngle(float angle);

        /**
         * @brief Calculate the signed angle between two 2D vectors.
         *
         * @param a The first vector
         * @param b The second vector
         * @return The angle in radians between the vectors
         */
        static float angleBetween(const glm::vec2& a, const glm::vec2& b);

        /**
         * @brief Calculate the angle between two 3D vectors.
         *
         * @param a The first vector
         * @param b The second vector
         * @return The angle in radians between the vectors
         */
        static float angleBetween(const glm::vec3& a, const glm::vec3& b);

        /**
         * @brief Generate a random float within a specified range.
         *
         * @param min The minimum value
         * @param max The maximum value
         * @return A random float between min and max
         */
        static float randomRange(float min, float max);

        /**
         * @brief Generate a random integer within a specified range.
         *
         * @param min The minimum value
         * @param max The maximum value
         * @return A random integer between min and max (inclusive)
         */
        static int randomRange(int min, int max);

        /**
         * @brief Generate a random 2D unit vector.
         *
         * @return A random 2D direction vector with unit length
         */
        static glm::vec2 randomDirection2D();

        /**
         * @brief Generate a random 3D unit vector.
         *
         * @return A random 3D direction vector with unit length
         */
        static glm::vec3 randomDirection3D();

        /**
         * @brief Generate a random point on the surface of a unit sphere.
         *
         * @return A random position vector on the unit sphere
         */
        static glm::vec3 randomPointOnSphere();

        /**
         * @brief Generate a random point within a unit sphere.
         *
         * @return A random position vector inside the unit sphere
         */
        static glm::vec3 randomPointInSphere();

        /**
         * @brief Generate a 1D Perlin noise value.
         *
         * @param x The x-coordinate
         * @return Noise value in the range [-1, 1]
         */
        static float perlinNoise(float x);

        /**
         * @brief Generate a 2D Perlin noise value.
         *
         * @param x The x-coordinate
         * @param y The y-coordinate
         * @return Noise value in the range [-1, 1]
         */
        static float perlinNoise(float x, float y);

        /**
         * @brief Generate a 3D Perlin noise value.
         *
         * @param x The x-coordinate
         * @param y The y-coordinate
         * @param z The z-coordinate
         * @return Noise value in the range [-1, 1]
         */
        static float perlinNoise(float x, float y, float z);

        /**
         * @brief Generate a 2D Simplex noise value.
         *
         * @param x The x-coordinate
         * @param y The y-coordinate
         * @return Noise value in the range [-1, 1]
         */
        static float simplexNoise(float x, float y);

        /**
         * @brief Generate a 3D Simplex noise value.
         *
         * @param x The x-coordinate
         * @param y The y-coordinate
         * @param z The z-coordinate
         * @return Noise value in the range [-1, 1]
         */
        static float simplexNoise(float x, float y, float z);

        /**
         * @brief Generate fractal noise by summing multiple octaves of noise.
         *
         * @param x The x-coordinate
         * @param y The y-coordinate
         * @param octaves Number of octaves to sum
         * @param persistence How much each octave contributes
         * @return Noise value in the range [-1, 1]
         */
        static float fractalNoise(float x, float y, int octaves, float persistence);

        /**
         * @brief Project a vector onto a plane defined by its normal.
         *
         * @param vector The vector to project
         * @param planeNormal The normal of the plane
         * @return The projected vector
         */
        static glm::vec3 projectOnPlane(const glm::vec3& vector, const glm::vec3& planeNormal);

        /**
         * @brief Reflect a vector off a surface with the specified normal.
         *
         * @param vector The vector to reflect
         * @param normal The surface normal
         * @return The reflected vector
         */
        static glm::vec3 reflect(const glm::vec3& vector, const glm::vec3& normal);

        /**
         * @brief Calculate the normal of a triangle.
         *
         * @param a First vertex position
         * @param b Second vertex position
         * @param c Third vertex position
         * @return The triangle normal
         */
        static glm::vec3 calculateNormal(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c);

        /**
         * @brief Calculate the tangent vector for a triangle.
         *
         * @param p1 First vertex position
         * @param p2 Second vertex position
         * @param p3 Third vertex position
         * @param uv1 First vertex UV
         * @param uv2 Second vertex UV
         * @param uv3 Third vertex UV
         * @return The triangle tangent
         */
        static glm::vec3 calculateTangent(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
                                          const glm::vec2& uv1, const glm::vec2& uv2, const glm::vec2& uv3);

        /**
         * @brief Compare two float values with an epsilon tolerance.
         *
         * @param a First value
         * @param b Second value
         * @param epsilon The comparison tolerance (default: EPSILON)
         * @return True if the values are equal within the tolerance
         */
        static bool equals(float a, float b, float epsilon = EPSILON);

        /**
         * @brief Compare two 2D vectors with an epsilon tolerance.
         *
         * @param a First vector
         * @param b Second vector
         * @param epsilon The comparison tolerance (default: EPSILON)
         * @return True if the vectors are equal within the tolerance
         */
        static bool equals(const glm::vec2& a, const glm::vec2& b, float epsilon = EPSILON);

        /**
         * @brief Compare two 3D vectors with an epsilon tolerance.
         *
         * @param a First vector
         * @param b Second vector
         * @param epsilon The comparison tolerance (default: EPSILON)
         * @return True if the vectors are equal within the tolerance
         */
        static bool equals(const glm::vec3& a, const glm::vec3& b, float epsilon = EPSILON);

        /**
         * @brief Create a transformation matrix from position, rotation, and scale.
         *
         * @param position The position vector
         * @param rotation The rotation quaternion
         * @param scale The scale vector
         * @return The combined transformation matrix
         */
        static glm::mat4 createTransformMatrix(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale);

        /**
         * @brief Decompose a transformation matrix into position, rotation, and scale.
         *
         * @param transform The transformation matrix to decompose
         * @param position Output parameter for the position
         * @param rotation Output parameter for the rotation
         * @param scale Output parameter for the scale
         */
        static void decomposeTransform(const glm::mat4& transform, glm::vec3& position, glm::quat& rotation, glm::vec3& scale);

        /**
         * @brief Transform a point by a 4x4 matrix.
         *
         * @param matrix The transformation matrix
         * @param point The point to transform
         * @return The transformed point
         */
        static glm::vec3 transformPoint(const glm::mat4& matrix, const glm::vec3& point);

        /**
         * @brief Transform a direction vector by a 4x4 matrix.
         *
         * Unlike transformPoint, this ignores translation.
         *
         * @param matrix The transformation matrix
         * @param direction The direction vector to transform
         * @return The transformed direction
         */
        static glm::vec3 transformDirection(const glm::mat4& matrix, const glm::vec3& direction);

    private:
        // Internal random number generator
        static std::mt19937 s_randomEngine;
        static std::uniform_real_distribution<float> s_uniformDist;

        // Perlin noise helper tables
        static constexpr int NOISE_PERM_SIZE = 256;
        static int s_permutation[NOISE_PERM_SIZE * 2];
        static float s_gradients2D[NOISE_PERM_SIZE * 2][2];
        static float s_gradients3D[NOISE_PERM_SIZE * 2][3];

        // Initialize the noise tables
        static void initializeNoise();

        // Return a value from the permutation table
        static int perm(int i);

        // Helper methods for noise functions
        static float grad(int hash, float x);
        static float grad(int hash, float x, float y);
        static float grad(int hash, float x, float y, float z);
    };

} // namespace PixelCraft::Utility