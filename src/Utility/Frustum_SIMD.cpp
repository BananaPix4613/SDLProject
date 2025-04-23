// -------------------------------------------------------------------------
// Frustum_SIMD.cpp - SIMD-optimized implementation of sphere culling
// -------------------------------------------------------------------------
#include "Utility/Frustum.h"

#ifdef __SSE__
#include <immintrin.h>

namespace PixelCraft::Utility
{

    // SIMD-optimized version of testSphereIntersection
    // This can replace the original implementation in Frustum.cpp
    IntersectionType Frustum::testSphereIntersection(const glm::vec3& center, float radius) const
    {
        // Optimization using SSE intrinsics
        __m128 centerX = _mm_set1_ps(center.x);
        __m128 centerY = _mm_set1_ps(center.y);
        __m128 centerZ = _mm_set1_ps(center.z);
        __m128 radiusVec = _mm_set1_ps(radius);
        __m128 negRadiusVec = _mm_set1_ps(-radius);

        // Process planes in groups of 4 for SIMD efficiency
        __m128 anyOutside = _mm_setzero_ps();
        __m128 allInside = _mm_set1_ps(1.0f);

        // Process first 4 planes
        {
            __m128 normalX = _mm_setr_ps(
                m_planes[0].getNormal().x,
                m_planes[1].getNormal().x,
                m_planes[2].getNormal().x,
                m_planes[3].getNormal().x
            );

            __m128 normalY = _mm_setr_ps(
                m_planes[0].getNormal().y,
                m_planes[1].getNormal().y,
                m_planes[2].getNormal().y,
                m_planes[3].getNormal().y
            );

            __m128 normalZ = _mm_setr_ps(
                m_planes[0].getNormal().z,
                m_planes[1].getNormal().z,
                m_planes[2].getNormal().z,
                m_planes[3].getNormal().z
            );

            __m128 distances = _mm_setr_ps(
                m_planes[0].getDistance(),
                m_planes[1].getDistance(),
                m_planes[2].getDistance(),
                m_planes[3].getDistance()
            );

            // Calculate dot product (normalX * centerX + normalY * centerY + normalZ * centerZ)
            __m128 dotX = _mm_mul_ps(normalX, centerX);
            __m128 dotY = _mm_mul_ps(normalY, centerY);
            __m128 dotZ = _mm_mul_ps(normalZ, centerZ);

            // Sum dot products and add distance to get signed distance
            __m128 signedDist = _mm_add_ps(_mm_add_ps(dotX, dotY), _mm_add_ps(dotZ, distances));

            // Test if any distance < -radius (outside)
            __m128 outsideMask = _mm_cmplt_ps(signedDist, negRadiusVec);
            anyOutside = _mm_or_ps(anyOutside, outsideMask);

            // Test if any distance < radius (not fully inside)
            __m128 intersectMask = _mm_cmplt_ps(signedDist, radiusVec);
            allInside = _mm_andnot_ps(intersectMask, allInside);
        }

        // Process remaining 2 planes
        {
            __m128 normalX = _mm_setr_ps(
                m_planes[4].getNormal().x,
                m_planes[5].getNormal().x,
                0.0f,
                0.0f
            );

            __m128 normalY = _mm_setr_ps(
                m_planes[4].getNormal().y,
                m_planes[5].getNormal().y,
                0.0f,
                0.0f
            );

            __m128 normalZ = _mm_setr_ps(
                m_planes[4].getNormal().z,
                m_planes[5].getNormal().z,
                0.0f,
                0.0f
            );

            __m128 distances = _mm_setr_ps(
                m_planes[4].getDistance(),
                m_planes[5].getDistance(),
                0.0f,
                0.0f
            );

            // Calculate dot product
            __m128 dotX = _mm_mul_ps(normalX, centerX);
            __m128 dotY = _mm_mul_ps(normalY, centerY);
            __m128 dotZ = _mm_mul_ps(normalZ, centerZ);

            // Sum dot products and add distance
            __m128 signedDist = _mm_add_ps(_mm_add_ps(dotX, dotY), _mm_add_ps(dotZ, distances));

            // Mask to only consider the first 2 values
            __m128 mask = _mm_setr_ps(1.0f, 1.0f, 0.0f, 0.0f);

            // Test if any distance < -radius (outside)
            __m128 outsideMask = _mm_and_ps(_mm_cmplt_ps(signedDist, negRadiusVec), mask);
            anyOutside = _mm_or_ps(anyOutside, outsideMask);

            // Test if any distance < radius (not fully inside)
            __m128 intersectMask = _mm_and_ps(_mm_cmplt_ps(signedDist, radiusVec), mask);
            allInside = _mm_andnot_ps(intersectMask, allInside);
        }

        // Check if any plane has the sphere completely outside
        int outsideResult = _mm_movemask_ps(anyOutside);
        if (outsideResult != 0)
        {
            return IntersectionType::Outside;
        }

        // Check if all planes have the sphere completely inside
        int insideResult = _mm_movemask_ps(allInside);
        if (insideResult == 0xF)
        {
            return IntersectionType::Inside;
        }

        // If we get here, the sphere intersects the frustum
        return IntersectionType::Intersects;
    }

} // namespace PixelCraft::Utility

#endif // __SSE__