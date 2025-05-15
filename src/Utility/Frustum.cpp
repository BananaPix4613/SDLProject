// -------------------------------------------------------------------------
// Frustum.cpp
// -------------------------------------------------------------------------
#include "Utility/Frustum.h"
#include "Utility/AABB.h"
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

namespace PixelCraft::Utility
{

    //-----------------------------------------------------------------------------
    // Plane Implementation
    //-----------------------------------------------------------------------------

    Plane::Plane() : m_normal(0.0f, 1.0f, 0.0f), m_distance(0.0f)
    {
    }

    Plane::Plane(const glm::vec3& normal, float distance) : m_normal(normal), m_distance(distance)
    {
    }

    Plane::Plane(const glm::vec3& normal, const glm::vec3& point) : m_normal(normal)
    {
        m_distance = -glm::dot(normal, point);
    }

    Plane::Plane(float a, float b, float c, float d) : m_normal(a, b, c), m_distance(d)
    {
    }

    void Plane::normalize()
    {
        float magnitude = glm::length(m_normal);
        if (magnitude > 0.0f)
        {
            float invMagnitude = 1.0f / magnitude;
            m_normal *= invMagnitude;
            m_distance *= invMagnitude;
        }
    }

    float Plane::getSignedDistance(const glm::vec3& point) const
    {
        return glm::dot(m_normal, point) + m_distance;
    }

    const glm::vec3& Plane::getNormal() const
    {
        return m_normal;
    }

    float Plane::getDistance() const
    {
        return m_distance;
    }

    void Plane::setNormal(const glm::vec3& normal)
    {
        m_normal = normal;
    }

    void Plane::setDistance(float distance)
    {
        m_distance = distance;
    }

    //-----------------------------------------------------------------------------
    // Frustum Implementation
    //-----------------------------------------------------------------------------

    Frustum::Frustum() : m_planesNormalized(false)
    {
        initialize();
    }

    void Frustum::initialize()
    {
        // Initialize with a default unit frustum
        m_planes[static_cast<int>(FrustumPlane::Near)] = Plane(glm::vec3(0.0f, 0.0f, -1.0f), -1.0f);
        m_planes[static_cast<int>(FrustumPlane::Far)] = Plane(glm::vec3(0.0f, 0.0f, 1.0f), -1.0f);
        m_planes[static_cast<int>(FrustumPlane::Left)] = Plane(glm::vec3(1.0f, 0.0f, 0.0f), -1.0f);
        m_planes[static_cast<int>(FrustumPlane::Right)] = Plane(glm::vec3(-1.0f, 0.0f, 0.0f), -1.0f);
        m_planes[static_cast<int>(FrustumPlane::Bottom)] = Plane(glm::vec3(0.0f, 1.0f, 0.0f), -1.0f);
        m_planes[static_cast<int>(FrustumPlane::Top)] = Plane(glm::vec3(0.0f, -1.0f, 0.0f), -1.0f);

        // Initialize corners
        m_corners[0] = glm::vec3(-1.0f, -1.0f, -1.0f);
        m_corners[1] = glm::vec3(1.0f, -1.0f, -1.0f);
        m_corners[2] = glm::vec3(1.0f, 1.0f, -1.0f);
        m_corners[3] = glm::vec3(-1.0f, 1.0f, -1.0f);
        m_corners[4] = glm::vec3(-1.0f, -1.0f, 1.0f);
        m_corners[5] = glm::vec3(1.0f, -1.0f, 1.0f);
        m_corners[6] = glm::vec3(1.0f, 1.0f, 1.0f);
        m_corners[7] = glm::vec3(-1.0f, 1.0f, 1.0f);

        m_planesNormalized = true;
    }

    void Frustum::update(const glm::mat4& viewProjection)
    {
        extractPlanes(viewProjection);

        // Also calculate corners - for this we need the inverse view-projection matrix
        glm::mat4 invViewProj = glm::inverse(viewProjection);
        extractCorners(invViewProj);
    }

    void Frustum::extractPlanes(const glm::mat4& matrix)
    {
        // Extract frustum planes from view-projection matrix
        // Method: Gribb/Hartmann method (efficient extraction)

        // Left plane
        glm::vec4 row3 = glm::row(matrix, 3);
        glm::vec4 row0 = glm::row(matrix, 0);
        m_planes[static_cast<int>(FrustumPlane::Left)] = Plane(
            row3.x + row0.x,
            row3.y + row0.y,
            row3.z + row0.z,
            row3.w + row0.w
        );

        // Right plane
        m_planes[static_cast<int>(FrustumPlane::Right)] = Plane(
            row3.x - row0.x,
            row3.y - row0.y,
            row3.z - row0.z,
            row3.w - row0.w
        );

        // Bottom plane
        glm::vec4 row1 = glm::row(matrix, 1);
        m_planes[static_cast<int>(FrustumPlane::Bottom)] = Plane(
            row3.x + row1.x,
            row3.y + row1.y,
            row3.z + row1.z,
            row3.w + row1.w
        );

        // Top plane
        m_planes[static_cast<int>(FrustumPlane::Top)] = Plane(
            row3.x - row1.x,
            row3.y - row1.y,
            row3.z - row1.z,
            row3.w - row1.w
        );

        // Near plane
        glm::vec4 row2 = glm::row(matrix, 2);
        m_planes[static_cast<int>(FrustumPlane::Near)] = Plane(
            row3.x + row2.x,
            row3.y + row2.y,
            row3.z + row2.z,
            row3.w + row2.w
        );

        // Far plane
        m_planes[static_cast<int>(FrustumPlane::Far)] = Plane(
            row3.x - row2.x,
            row3.y - row2.y,
            row3.z - row2.z,
            row3.w - row2.w
        );

        // Normalize all planes
        for (auto& plane : m_planes)
        {
            plane.normalize();
        }

        m_planesNormalized = true;
    }

    void Frustum::extractCorners(const glm::mat4& invViewProj)
    {
        // Frustum corners in NDC space
        static const glm::vec4 ndcCorners[8] = {
            // Near face (z = -1)
            glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f), // Bottom-left
            glm::vec4(1.0f, -1.0f, -1.0f, 1.0f),  // Bottom-right
            glm::vec4(1.0f, 1.0f, -1.0f, 1.0f),   // Top-right
            glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f),  // Top-left

            // Far face (z = 1)
            glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),  // Bottom-left
            glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),   // Bottom-right
            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),    // Top-right
            glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f)    // Top-left
        };

        // Transform NDC corners to world space
        for (int i = 0; i < 8; ++i)
        {
            glm::vec4 worldCorner = invViewProj * ndcCorners[i];
            // Perspective divide
            worldCorner /= worldCorner.w;
            m_corners[i] = glm::vec3(worldCorner);
        }
    }

    const Plane& Frustum::getPlane(FrustumPlane planeType) const
    {
        return m_planes[static_cast<int>(planeType)];
    }

    Plane& Frustum::getPlane(FrustumPlane planeType)
    {
        return m_planes[static_cast<int>(planeType)];
    }

    const std::array<Plane, 6>& Frustum::getPlanes() const
    {
        return m_planes;
    }

    const std::array<glm::vec3, 8>& Frustum::getCorners() const
    {
        return m_corners;
    }

    bool Frustum::testPoint(const glm::vec3& point) const
    {
        // For a point to be inside the frustum, it must be on the positive
        // side of all planes (or on the plane)
        for (const auto& plane : m_planes)
        {
            if (plane.getSignedDistance(point) < 0.0f)
            {
                return false;
            }
        }
        return true;
    }

    bool Frustum::testSphere(const glm::vec3& center, float radius) const
    {
        // For a sphere to be inside or intersect, the distance to each plane
        // must be greater than -radius
        for (const auto& plane : m_planes)
        {
            float distance = plane.getSignedDistance(center);
            if (distance < -radius)
            {
                return false;
            }
        }
        return true;
    }

    IntersectionType Frustum::testSphereIntersection(const glm::vec3& center, float radius) const
    {
        // SIMD optimization potential here with intrinsics

        bool fullyInside = true;

        for (const auto& plane : m_planes)
        {
            float distance = plane.getSignedDistance(center);

            // Outside
            if (distance < -radius)
            {
                return IntersectionType::Outside;
            }

            // Intersecting
            if (distance < radius)
            {
                fullyInside = false;
            }
        }

        return fullyInside ? IntersectionType::Inside : IntersectionType::Intersects;
    }

    bool Frustum::testAABB(const glm::vec3& min, const glm::vec3& max) const
    {
        // Quick test - if any plane has the box completely on the negative side,
        // the box is outside the frustum
        for (const auto& plane : m_planes)
        {
            const glm::vec3& normal = plane.getNormal();

            // Find the corner of the box that is furthest in the direction of the plane normal
            glm::vec3 p;
            p.x = (normal.x > 0.0f) ? max.x : min.x;
            p.y = (normal.y > 0.0f) ? max.y : min.y;
            p.z = (normal.z > 0.0f) ? max.z : min.z;

            // If this corner is outside, the entire box is outside
            if (plane.getSignedDistance(p) < 0.0f)
            {
                return false;
            }
        }

        // If we get here, the box is either intersecting or inside the frustum
        return true;
    }

    bool Frustum::testAABB(const AABB& bounds) const
    {
        if (bounds.isValid())
        {
            return false;
        }

        glm::vec3 min = bounds.getMin();
        glm::vec3 max = bounds.getMax();

        // Quick test - if any plane has the box completely on the negative side,
        // the box is outside the frustum
        for (const auto& plane : m_planes)
        {
            const glm::vec3& normal = plane.getNormal();

            // Find the corner of the box that is furthest in the direction of the plane normal
            glm::vec3 p;
            p.x = (normal.x > 0.0f) ? max.x : min.x;
            p.y = (normal.y > 0.0f) ? max.y : min.y;
            p.z = (normal.z > 0.0f) ? max.z : min.z;

            // If this corner is outside, the entire box is outside
            if (plane.getSignedDistance(p) < 0.0f)
            {
                return false;
            }
        }

        // If we get here, the box is either intersecting or inside the frustum
        return true;
    }

    IntersectionType Frustum::testAABBIntersection(const glm::vec3& min, const glm::vec3& max) const
    {
        // More detailed test than testAABB
        bool fullyInside = true;

        for (const auto& plane : m_planes)
        {
            const glm::vec3& normal = plane.getNormal();

            // Find the positive vertex (p-vertex)
            glm::vec3 p;
            p.x = (normal.x > 0.0f) ? max.x : min.x;
            p.y = (normal.y > 0.0f) ? max.y : min.y;
            p.z = (normal.z > 0.0f) ? max.z : min.z;

            // Find the negative vertex (n-vertex)
            glm::vec3 n;
            n.x = (normal.x > 0.0f) ? min.x : max.x;
            n.y = (normal.y > 0.0f) ? min.y : max.y;
            n.z = (normal.z > 0.0f) ? min.z : max.z;

            // Check p-vertex
            if (plane.getSignedDistance(p) < 0.0f)
            {
                return IntersectionType::Outside;
            }

            // Check n-vertex (if n-vertex is outside, the box is intersecting)
            if (plane.getSignedDistance(n) < 0.0f)
            {
                fullyInside = false;
            }
        }

        return fullyInside ? IntersectionType::Inside : IntersectionType::Intersects;
    }

    void Frustum::debugDraw(const glm::vec3& color) const
    {
        // Note: This implementation depends on the DebugDraw utility
        // A reference implementation is provided but actual usage would require
        // the DebugDraw system to be integrated

        // Draw near face
        /*
        DebugDraw::drawLine(m_corners[0], m_corners[1], color);
        DebugDraw::drawLine(m_corners[1], m_corners[2], color);
        DebugDraw::drawLine(m_corners[2], m_corners[3], color);
        DebugDraw::drawLine(m_corners[3], m_corners[0], color);

        // Draw far face
        DebugDraw::drawLine(m_corners[4], m_corners[5], color);
        DebugDraw::drawLine(m_corners[5], m_corners[6], color);
        DebugDraw::drawLine(m_corners[6], m_corners[7], color);
        DebugDraw::drawLine(m_corners[7], m_corners[4], color);

        // Draw edges connecting near and far faces
        DebugDraw::drawLine(m_corners[0], m_corners[4], color);
        DebugDraw::drawLine(m_corners[1], m_corners[5], color);
        DebugDraw::drawLine(m_corners[2], m_corners[6], color);
        DebugDraw::drawLine(m_corners[3], m_corners[7], color);
        */

        // For now, let's just use comments as a placeholder
        // The actual implementation would call the DebugDraw system 
        // once it's integrated
    }

} // namespace PixelCraft::Utility