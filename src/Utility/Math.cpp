// -------------------------------------------------------------------------
// Math.cpp
// -------------------------------------------------------------------------
#include "Utility/Math.h"
#include <cmath>
#include <ctime>
#include <algorithm>
#include <gtc/matrix_transform.hpp>
#include <gtx/quaternion.hpp>

#if defined(__SSE__)
#include <immintrin.h>
#endif
#include <random>

namespace PixelCraft::Utility
{

    // Initialize static members
    std::mt19937 Math::s_randomEngine(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_real_distribution<float> Math::s_uniformDist(0.0f, 1.0f);

    int Math::s_permutation[NOISE_PERM_SIZE * 2];
    float Math::s_gradients2D[NOISE_PERM_SIZE * 2][2];
    float Math::s_gradients3D[NOISE_PERM_SIZE * 2][3];

    // Initialize noise tables on first use
    bool s_noiseInitialized = false;

    float Math::lerp(float a, float b, float t)
    {
        return a + t * (b - a);
    }

    glm::vec2 Math::lerp(const glm::vec2& a, const glm::vec2& b, float t)
    {
#if defined(__SSE__)
        // SIMD optimized implementation for vec2
        __m128 va = _mm_set_ps(0, 0, a.y, a.x);
        __m128 vb = _mm_set_ps(0, 0, b.y, b.x);
        __m128 vt = _mm_set_ps1(t);

        // a + t * (b - a)
        __m128 result = _mm_add_ps(va, _mm_mul_ps(vt, _mm_sub_ps(vb, va)));

        // Extract the results
        float resArray[4];
        _mm_storeu_ps(resArray, result);

        return glm::vec2(resArray[0], resArray[1]);
#else
        return a + t * (b - a);
#endif
    }

    glm::vec3 Math::lerp(const glm::vec3& a, const glm::vec3& b, float t)
    {
#if defined(__SSE__)
        // SIMD optimized implementation for vec3
        __m128 va = _mm_set_ps(0, a.z, a.y, a.x);
        __m128 vb = _mm_set_ps(0, b.z, b.y, b.x);
        __m128 vt = _mm_set_ps1(t);

        // a + t * (b - a)
        __m128 result = _mm_add_ps(va, _mm_mul_ps(vt, _mm_sub_ps(vb, va)));

        // Extract the results
        float resArray[4];
        _mm_storeu_ps(resArray, result);

        return glm::vec3(resArray[0], resArray[1], resArray[2]);
#else
        return a + t * (b - a);
#endif
    }

    glm::vec4 Math::lerp(const glm::vec4& a, const glm::vec4& b, float t)
    {
#if defined(__SSE__)
        // SIMD optimized implementation for vec4
        __m128 va = _mm_set_ps(a.w, a.z, a.y, a.x);
        __m128 vb = _mm_set_ps(b.w, b.z, b.y, b.x);
        __m128 vt = _mm_set_ps1(t);

        // a + t * (b - a)
        __m128 result = _mm_add_ps(va, _mm_mul_ps(vt, _mm_sub_ps(vb, va)));

        // Extract the results
        float resArray[4];
        _mm_storeu_ps(resArray, result);

        return glm::vec4(resArray[0], resArray[1], resArray[2], resArray[3]);
#else
        return a + t * (b - a);
#endif
    }

    float Math::smoothStep(float a, float b, float t)
    {
        // Clamp t to [0, 1]
        t = saturate(t);

        // Apply smoothstep formula: 3t² - 2t³
        t = t * t * (3.0f - 2.0f * t);

        // Interpolate between a and b
        return a + t * (b - a);
    }

    float Math::smootherStep(float a, float b, float t)
    {
        // Clamp t to [0, 1]
        t = saturate(t);

        // Apply Ken Perlin's improved smoothstep: 6t^5 - 15t^4 + 10t^3
        t = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);

        // Interpolate between a and b
        return a + t * (b - a);
    }

    float Math::clamp(float value, float min, float max)
    {
        return std::max(min, std::min(value, max));
    }

    int Math::clamp(int value, int min, int max)
    {
        return std::max(min, std::min(value, max));
    }

    float Math::wrapAngle(float angle)
    {
        // Wrap the angle to (-PI, PI]
        angle = std::fmod(angle, TWO_PI);
        if (angle <= -PI) angle += TWO_PI;
        if (angle > PI) angle -= TWO_PI;
        return angle;
    }

    float Math::angleBetween(const glm::vec2& a, const glm::vec2& b)
    {
        // Calculate the signed angle between two 2D vectors
        float dot = a.x * b.x + a.y * b.y;
        float det = a.x * b.y - a.y * b.x;
        return std::atan2(det, dot);
    }

    float Math::angleBetween(const glm::vec3& a, const glm::vec3& b)
    {
        // Calculate the angle between two 3D vectors
        float lenProduct = glm::length(a) * glm::length(b);

        // Protect against zero vectors
        if (lenProduct < EPSILON) return 0.0f;

        float dot = glm::dot(a, b) / lenProduct;

        // Clamp to prevent floating-point errors
        dot = clamp(dot, -1.0f, 1.0f);

        return std::acos(dot);
    }

    float Math::randomRange(float min, float max)
    {
        return min + s_uniformDist(s_randomEngine) * (max - min);
    }

    int Math::randomRange(int min, int max)
    {
        std::uniform_int_distribution<int> intDist(min, max);
        return intDist(s_randomEngine);
    }

    glm::vec2 Math::randomDirection2D()
    {
        float angle = randomRange(0.0f, TWO_PI);
        return glm::vec2(std::cos(angle), std::sin(angle));
    }

    glm::vec3 Math::randomDirection3D()
    {
        // Generate a random point on a unit sphere
        float z = randomRange(-1.0f, 1.0f);
        float t = randomRange(0.0f, TWO_PI);
        float r = std::sqrt(1.0f - z * z);
        float x = r * std::cos(t);
        float y = r * std::sin(t);
        return glm::vec3(x, y, z);
    }

    glm::vec3 Math::randomPointOnSphere()
    {
        // This is the same as randomDirection3D()
        return randomDirection3D();
    }

    glm::vec3 Math::randomPointInSphere()
    {
        // Generate a random direction and scale by random radius with cubic distribution
        float radius = std::cbrt(randomRange(0.0f, 1.0f)); // Cubic root for uniform distribution
        return randomDirection3D() * radius;
    }

    // Initialize the noise tables
    void Math::initializeNoise()
    {
        if (s_noiseInitialized) return;

        // Initialize permutation table with a random sequence
        for (int i = 0; i < NOISE_PERM_SIZE; ++i)
        {
            s_permutation[i] = i;
        }

        // Shuffle permutation table
        for (int i = NOISE_PERM_SIZE - 1; i > 0; --i)
        {
            int j = randomRange(0, i);
            std::swap(s_permutation[i], s_permutation[j]);

            // Mirror into the second half
            s_permutation[i + NOISE_PERM_SIZE] = s_permutation[i];
        }

        // Generate random gradient vectors for 2D noise
        for (int i = 0; i < NOISE_PERM_SIZE * 2; ++i)
        {
            float angle = randomRange(0.0f, TWO_PI);
            s_gradients2D[i][0] = std::cos(angle);
            s_gradients2D[i][1] = std::sin(angle);
        }

        // Generate random gradient vectors for 3D noise
        for (int i = 0; i < NOISE_PERM_SIZE * 2; ++i)
        {
            glm::vec3 grad = randomDirection3D();
            s_gradients3D[i][0] = grad.x;
            s_gradients3D[i][1] = grad.y;
            s_gradients3D[i][2] = grad.z;
        }

        s_noiseInitialized = true;
    }

    int Math::perm(int i)
    {
        if (!s_noiseInitialized) initializeNoise();
        return s_permutation[i & (NOISE_PERM_SIZE - 1)];
    }

    float Math::grad(int hash, float x)
    {
        int h = hash & 15;
        float grad = 1.0f + (h & 7);  // Gradient value 1-8
        if (h & 8) grad = -grad;      // Random sign
        return (grad * x);            // Multiply the gradient with the distance
    }

    float Math::grad(int hash, float x, float y)
    {
        if (!s_noiseInitialized) initializeNoise();

        int index = hash & (NOISE_PERM_SIZE * 2 - 1);
        return s_gradients2D[index][0] * x + s_gradients2D[index][1] * y;
    }

    float Math::grad(int hash, float x, float y, float z)
    {
        if (!s_noiseInitialized) initializeNoise();

        int index = hash & (NOISE_PERM_SIZE * 2 - 1);
        return s_gradients3D[index][0] * x + s_gradients3D[index][1] * y + s_gradients3D[index][2] * z;
    }

    float Math::perlinNoise(float x)
    {
        if (!s_noiseInitialized) initializeNoise();

        int X = static_cast<int>(std::floor(x)) & 255;
        x -= std::floor(x);

        float u = smootherStep(0.0f, 1.0f, x);

        return lerp(grad(perm(X), x), grad(perm(X + 1), x - 1), u) * 2.0f;
    }

    float Math::perlinNoise(float x, float y)
    {
        if (!s_noiseInitialized) initializeNoise();

        // Integer part of coordinates
        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;

        // Fractional part of coordinates
        x -= std::floor(x);
        y -= std::floor(y);

        // Compute fade curves
        float u = smootherStep(0.0f, 1.0f, x);
        float v = smootherStep(0.0f, 1.0f, y);

        // Hash coordinates of the 4 square corners
        int A = perm(X) + Y;
        int B = perm(X + 1) + Y;
        int AA = perm(A);
        int AB = perm(A + 1);
        int BA = perm(B);
        int BB = perm(B + 1);

        // Blend the gradients
        float g1 = grad(AA, x, y);
        float g2 = grad(BA, x - 1, y);
        float g3 = grad(AB, x, y - 1);
        float g4 = grad(BB, x - 1, y - 1);

        float result = lerp(
            lerp(g1, g2, u),
            lerp(g3, g4, u),
            v);

        // Scale to range [-1, 1]
        return result;
    }

    float Math::perlinNoise(float x, float y, float z)
    {
        if (!s_noiseInitialized) initializeNoise();

        // Integer part of coordinates
        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;
        int Z = static_cast<int>(std::floor(z)) & 255;

        // Fractional part of coordinates
        x -= std::floor(x);
        y -= std::floor(y);
        z -= std::floor(z);

        // Compute fade curves
        float u = smootherStep(0.0f, 1.0f, x);
        float v = smootherStep(0.0f, 1.0f, y);
        float w = smootherStep(0.0f, 1.0f, z);

        // Hash coordinates of the 8 cube corners
        int A = perm(X) + Y;
        int B = perm(X + 1) + Y;
        int AA = perm(A) + Z;
        int AB = perm(A + 1) + Z;
        int BA = perm(B) + Z;
        int BB = perm(B + 1) + Z;

        // Blend the gradients of the 8 corners
        float g1 = grad(perm(AA), x, y, z);
        float g2 = grad(perm(BA), x - 1, y, z);
        float g3 = grad(perm(AB), x, y - 1, z);
        float g4 = grad(perm(BB), x - 1, y - 1, z);
        float g5 = grad(perm(AA + 1), x, y, z - 1);
        float g6 = grad(perm(BA + 1), x - 1, y, z - 1);
        float g7 = grad(perm(AB + 1), x, y - 1, z - 1);
        float g8 = grad(perm(BB + 1), x - 1, y - 1, z - 1);

        float result = lerp(
            lerp(
                lerp(g1, g2, u),
                lerp(g3, g4, u),
                v),
            lerp(
                lerp(g5, g6, u),
                lerp(g7, g8, u),
                v),
            w);

        // Scale to range [-1, 1]
        return result;
    }

    float Math::simplexNoise(float x, float y)
    {
        if (!s_noiseInitialized) initializeNoise();

        const float F2 = 0.366025403f; // F2 = 0.5*(sqrt(3.0)-1.0)
        const float G2 = 0.211324865f; // G2 = (3.0-sqrt(3.0))/6.0

        // Skew the input space to determine which simplex cell we're in
        float s = (x + y) * F2;
        float xs = x + s;
        float ys = y + s;
        int i = static_cast<int>(std::floor(xs));
        int j = static_cast<int>(std::floor(ys));

        float t = static_cast<float>(i + j) * G2;
        float X0 = i - t; // Unskew the cell origin back to (x,y) space
        float Y0 = j - t;
        float x0 = x - X0; // The x,y distances from the cell origin
        float y0 = y - Y0;

        // Determine which simplex we are in
        int i1, j1; // Offsets for second (middle) corner of simplex in (i,j) coords
        if (x0 > y0)
        { // lower triangle, XY order: (0,0)->(1,0)->(1,1)
            i1 = 1;
            j1 = 0;
        }
        else
        { // upper triangle, YX order: (0,0)->(0,1)->(1,1)
            i1 = 0;
            j1 = 1;
        }

        // A step of (1,0) in (i,j) means a step of (1-G2,G2) in (x,y), and
        // a step of (0,1) in (i,j) means a step of (G2,1-G2) in (x,y).
        float x1 = x0 - i1 + G2; // Offsets for middle corner in (x,y) unskewed coords
        float y1 = y0 - j1 + G2;
        float x2 = x0 - 1.0f + 2.0f * G2; // Offsets for last corner in (x,y) unskewed coords
        float y2 = y0 - 1.0f + 2.0f * G2;

        // Work out the hashed gradient indices of the three simplex corners
        int ii = i & 255;
        int jj = j & 255;
        int gi0 = perm(ii + perm(jj)) % 12;
        int gi1 = perm(ii + i1 + perm(jj + j1)) % 12;
        int gi2 = perm(ii + 1 + perm(jj + 1)) % 12;

        // Calculate contribution from the three corners
        float t0 = 0.5f - x0 * x0 - y0 * y0;
        float n0 = (t0 < 0.0f) ? 0.0f : (t0 * t0 * t0 * t0 * grad(gi0, x0, y0));

        float t1 = 0.5f - x1 * x1 - y1 * y1;
        float n1 = (t1 < 0.0f) ? 0.0f : (t1 * t1 * t1 * t1 * grad(gi1, x1, y1));

        float t2 = 0.5f - x2 * x2 - y2 * y2;
        float n2 = (t2 < 0.0f) ? 0.0f : (t2 * t2 * t2 * t2 * grad(gi2, x2, y2));

        // Add contributions from each corner to get the final noise value.
        // The result is scaled to return values in the interval [-1,1].
        return 70.0f * (n0 + n1 + n2);
    }

    float Math::simplexNoise(float x, float y, float z)
    {
        if (!s_noiseInitialized) initializeNoise();

        const float F3 = 1.0f / 3.0f;
        const float G3 = 1.0f / 6.0f;

        // Skew the input space to determine which simplex cell we're in
        float s = (x + y + z) * F3;
        int i = static_cast<int>(std::floor(x + s));
        int j = static_cast<int>(std::floor(y + s));
        int k = static_cast<int>(std::floor(z + s));

        float t = (i + j + k) * G3;
        float X0 = i - t; // Unskew the cell origin back to (x,y,z) space
        float Y0 = j - t;
        float Z0 = k - t;

        float x0 = x - X0; // The x,y,z distances from the cell origin
        float y0 = y - Y0;
        float z0 = z - Z0;

        // Determine which simplex we are in
        int i1, j1, k1; // Offsets for second corner of simplex in (i,j,k) coords
        int i2, j2, k2; // Offsets for third corner of simplex in (i,j,k) coords

        if (x0 >= y0)
        {
            if (y0 >= z0)
            { // X Y Z order
                i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0;
            }
            else if (x0 >= z0)
            { // X Z Y order
                i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1;
            }
            else
            { // Z X Y order
                i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1;
            }
        }
        else
        { // x0<y0
            if (y0 < z0)
            { // Z Y X order
                i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1;
            }
            else if (x0 < z0)
            { // Y Z X order
                i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1;
            }
            else
            { // Y X Z order
                i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0;
            }
        }

        // A step of (1,0,0) in (i,j,k) means a step of (1-G3,G3,G3) in (x,y,z),
        // a step of (0,1,0) in (i,j,k) means a step of (G3,1-G3,G3) in (x,y,z), and
        // a step of (0,0,1) in (i,j,k) means a step of (G3,G3,1-G3) in (x,y,z).
        float x1 = x0 - i1 + G3; // Offsets for second corner in (x,y,z) coords
        float y1 = y0 - j1 + G3;
        float z1 = z0 - k1 + G3;
        float x2 = x0 - i2 + 2.0f * G3; // Offsets for third corner in (x,y,z) coords
        float y2 = y0 - j2 + 2.0f * G3;
        float z2 = z0 - k2 + 2.0f * G3;
        float x3 = x0 - 1.0f + 3.0f * G3; // Offsets for last corner in (x,y,z) coords
        float y3 = y0 - 1.0f + 3.0f * G3;
        float z3 = z0 - 1.0f + 3.0f * G3;

        // Work out the hashed gradient indices of the four simplex corners
        int ii = i & 255;
        int jj = j & 255;
        int kk = k & 255;
        int gi0 = perm(ii + perm(jj + perm(kk))) % 12;
        int gi1 = perm(ii + i1 + perm(jj + j1 + perm(kk + k1))) % 12;
        int gi2 = perm(ii + i2 + perm(jj + j2 + perm(kk + k2))) % 12;
        int gi3 = perm(ii + 1 + perm(jj + 1 + perm(kk + 1))) % 12;

        // Calculate contribution from the four corners
        float t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
        float n0 = (t0 < 0.0f) ? 0.0f : (t0 * t0 * t0 * t0 * grad(gi0, x0, y0, z0));

        float t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
        float n1 = (t1 < 0.0f) ? 0.0f : (t1 * t1 * t1 * t1 * grad(gi1, x1, y1, z1));

        float t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
        float n2 = (t2 < 0.0f) ? 0.0f : (t2 * t2 * t2 * t2 * grad(gi2, x2, y2, z2));

        float t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
        float n3 = (t3 < 0.0f) ? 0.0f : (t3 * t3 * t3 * t3 * grad(gi3, x3, y3, z3));

        // Add contributions from each corner to get the final noise value.
        // The result is scaled to return values in the interval [-1,1].
        return 32.0f * (n0 + n1 + n2 + n3);
    }

    float Math::fractalNoise(float x, float y, int octaves, float persistence)
    {
        if (!s_noiseInitialized) initializeNoise();

        float total = 0.0f;
        float frequency = 1.0f;
        float amplitude = 1.0f;
        float maxValue = 0.0f;  // Used for normalizing result to -1.0 - 1.0

        for (int i = 0; i < octaves; ++i)
        {
            total += perlinNoise(x * frequency, y * frequency) * amplitude;

            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= 2.0f;
        }

        return total / maxValue;
    }

    glm::vec3 Math::projectOnPlane(const glm::vec3& vector, const glm::vec3& planeNormal)
    {
        // Project vector onto plane: v - (v·n)n where n is normalized
        float normalLength = glm::length(planeNormal);

        // Handle near-zero normals
        if (normalLength < EPSILON) return vector;

        glm::vec3 normalizedNormal = planeNormal / normalLength;
        float dot = glm::dot(vector, normalizedNormal);

        return vector - (dot * normalizedNormal);
    }

    glm::vec3 Math::reflect(const glm::vec3& vector, const glm::vec3& normal)
    {
        // Reflect vector: v - 2(v·n)n where n is normalized
        float normalLength = glm::length(normal);

        // Handle near-zero normals
        if (normalLength < EPSILON) return vector;

        glm::vec3 normalizedNormal = normal / normalLength;
        float dot = glm::dot(vector, normalizedNormal);

        return vector - (2.0f * dot * normalizedNormal);
    }

    glm::vec3 Math::calculateNormal(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
    {
        // Calculate vectors along two edges of the triangle
        glm::vec3 edge1 = b - a;
        glm::vec3 edge2 = c - a;

        // Cross product of the two edges gives us the normal
        glm::vec3 normal = glm::cross(edge1, edge2);

        // Normalize the result
        float length = glm::length(normal);
        if (length > EPSILON)
        {
            return normal / length;
        }

        // Fallback if the triangle is degenerate
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }

    glm::vec3 Math::calculateTangent(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
                                     const glm::vec2& uv1, const glm::vec2& uv2, const glm::vec2& uv3)
    {
        // Calculate edges of the triangle in both 3D space and UV space
        glm::vec3 edge1 = p2 - p1;
        glm::vec3 edge2 = p3 - p1;
        glm::vec2 deltaUV1 = uv2 - uv1;
        glm::vec2 deltaUV2 = uv3 - uv1;

        // Solve the linear system to find the tangent
        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        glm::vec3 tangent;
        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        // Normalize the result
        return glm::normalize(tangent);
    }

    bool Math::equals(float a, float b, float epsilon)
    {
        return std::abs(a - b) <= epsilon;
    }

    bool Math::equals(const glm::vec2& a, const glm::vec2& b, float epsilon)
    {
        return equals(a.x, b.x, epsilon) && equals(a.y, b.y, epsilon);
    }

    bool Math::equals(const glm::vec3& a, const glm::vec3& b, float epsilon)
    {
        return equals(a.x, b.x, epsilon) && equals(a.y, b.y, epsilon) && equals(a.z, b.z, epsilon);
    }

    glm::mat4 Math::createTransformMatrix(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale)
    {
        // Create transformation matrix in TRS order (scale, then rotate, then translate)
        glm::mat4 translateMat = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 rotateMat = glm::toMat4(rotation);
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);

        // Combine the transformations (order matters!)
        return translateMat * rotateMat * scaleMat;
    }

    void Math::decomposeTransform(const glm::mat4& transform, glm::vec3& position, glm::quat& rotation, glm::vec3& scale)
    {
        // Extract the position from the last column
        position = glm::vec3(transform[3]);

        // Extract the scale
        scale.x = glm::length(glm::vec3(transform[0]));
        scale.y = glm::length(glm::vec3(transform[1]));
        scale.z = glm::length(glm::vec3(transform[2]));

        // Create rotation matrix by removing scale from the upper 3x3 portion
        glm::mat3 rotMat(
            glm::vec3(transform[0]) / scale.x,
            glm::vec3(transform[1]) / scale.y,
            glm::vec3(transform[2]) / scale.z
        );

        // Convert to quaternion
        rotation = glm::quat_cast(rotMat);
    }

    glm::vec3 Math::transformPoint(const glm::mat4& matrix, const glm::vec3& point)
    {
        // Transform a point (applying translation)
        glm::vec4 transformedPoint = matrix * glm::vec4(point, 1.0f);
        return glm::vec3(
            transformedPoint.x / transformedPoint.w,
            transformedPoint.y / transformedPoint.w,
            transformedPoint.z / transformedPoint.w
        );
    }

    glm::vec3 Math::transformDirection(const glm::mat4& matrix, const glm::vec3& direction)
    {
        // Transform a direction (ignoring translation)
        return glm::vec3(matrix * glm::vec4(direction, 0.0f));
    }

} // namespace PixelCraft::Utility