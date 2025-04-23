// -------------------------------------------------------------------------
// Ray.cpp
// -------------------------------------------------------------------------
#include "Utility/Ray.h"
#include "Utility/AABB.h"
#include "Core/Logger.h"
#include "Utility/DebugDraw.h"

#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/transform.hpp>
#include <limits>
#include <glm/gtx/norm.hpp>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Utility
{

    Ray::Ray()
        : m_origin(0.0f, 0.0f, 0.0f)
        , m_direction(0.0f, 0.0f, 1.0f)
        , m_hasDifferentials(false)
    {
        updateCachedValues();
    }

    Ray::Ray(const glm::vec3& origin, const glm::vec3& direction)
        : m_origin(origin)
        , m_direction(glm::normalize(direction))
        , m_hasDifferentials(false)
    {
        updateCachedValues();
    }

    void Ray::setOrigin(const glm::vec3& origin)
    {
        m_origin = origin;
    }

    void Ray::setDirection(const glm::vec3& direction)
    {
        // Don't normalize if given a zero vector to avoid NaNs
        if (glm::length2(direction) > std::numeric_limits<float>::epsilon())
        {
            m_direction = glm::normalize(direction);
            updateCachedValues();
        }
        else
        {
            Log::warn("Ray::setDirection called with zero-length vector");
        }
    }

    void Ray::updateCachedValues()
    {
        // Calculate inverse direction components
        m_invDirection.x = 1.0f / m_direction.x;
        m_invDirection.y = 1.0f / m_direction.y;
        m_invDirection.z = 1.0f / m_direction.z;

        // Store signs of direction components for optimized AABB test
        m_dirIsNeg.x = (m_direction.x < 0.0f) ? 1 : 0;
        m_dirIsNeg.y = (m_direction.y < 0.0f) ? 1 : 0;
        m_dirIsNeg.z = (m_direction.z < 0.0f) ? 1 : 0;
    }

    Ray Ray::transform(const glm::mat4& matrix) const
    {
        // Transform origin point by full matrix
        glm::vec4 originTransformed = matrix * glm::vec4(m_origin, 1.0f);
        glm::vec3 newOrigin = glm::vec3(originTransformed) / originTransformed.w;

        // Transform direction by rotation part of matrix (upper 3x3)
        // Direction is a vector, not a point, so we don't apply translation
        glm::vec3 newDirection = glm::mat3(matrix) * m_direction;

        return Ray(newOrigin, newDirection);
    }

    bool Ray::intersectAABB(const AABB& aabb, float& tMin, float& tMax) const
    {
        // Get AABB bounds
        const glm::vec3& boxMin = aabb.getMin();
        const glm::vec3& boxMax = aabb.getMax();

        // Optimized slab method using precomputed inverse direction
        float tx1 = (boxMin.x - m_origin.x) * m_invDirection.x;
        float tx2 = (boxMax.x - m_origin.x) * m_invDirection.x;

        tMin = std::min(tx1, tx2);
        tMax = std::max(tx1, tx2);

        float ty1 = (boxMin.y - m_origin.y) * m_invDirection.y;
        float ty2 = (boxMax.y - m_origin.y) * m_invDirection.y;

        tMin = std::max(tMin, std::min(ty1, ty2));
        tMax = std::min(tMax, std::max(ty1, ty2));

        float tz1 = (boxMin.z - m_origin.z) * m_invDirection.z;
        float tz2 = (boxMax.z - m_origin.z) * m_invDirection.z;

        tMin = std::max(tMin, std::min(tz1, tz2));
        tMax = std::min(tMax, std::max(tz1, tz2));

        return tMax >= tMin && tMax > 0.0f;
    }

    bool Ray::intersectSphere(const glm::vec3& center, float radius, float& t) const
    {
        // Calculate vector from ray origin to sphere center
        glm::vec3 oc = m_origin - center;

        // Quadratic coefficients
        float a = glm::dot(m_direction, m_direction); // Always 1 for normalized direction
        float b = 2.0f * glm::dot(m_direction, oc);
        float c = glm::dot(oc, oc) - radius * radius;

        // Calculate discriminant
        float discriminant = b * b - 4.0f * a * c;

        if (discriminant < 0.0f)
        {
            // No intersection
            return false;
        }

        // Calculate closest intersection
        float sqrtDiscriminant = std::sqrt(discriminant);
        float t0 = (-b - sqrtDiscriminant) / (2.0f * a);
        float t1 = (-b + sqrtDiscriminant) / (2.0f * a);

        // Check for valid intersections (in front of ray origin)
        if (t0 > 0.0f)
        {
            t = t0;
            return true;
        }

        if (t1 > 0.0f)
        {
            t = t1;
            return true;
        }

        // Sphere is behind ray origin
        return false;
    }

    bool Ray::intersectPlane(const glm::vec3& normal, float distance, float& t) const
    {
        // Calculate dot product between ray direction and plane normal
        float denom = glm::dot(normal, m_direction);

        // Check if ray is parallel to plane
        if (std::abs(denom) < std::numeric_limits<float>::epsilon())
        {
            return false;
        }

        // Calculate intersection distance
        t = -(glm::dot(normal, m_origin) + distance) / denom;

        // Check if intersection is in front of ray origin
        return t >= 0.0f;
    }

    bool Ray::intersectPlane(const glm::vec3& normal, const glm::vec3& point, float& t) const
    {
        // Convert to plane equation form Ax + By + Cz + D = 0
        // where D = -dot(normal, point)
        float distance = -glm::dot(normal, point);

        // Use the other plane intersection method
        return intersectPlane(normal, distance, t);
    }

    bool Ray::intersectTriangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
                                float& t, float& u, float& v, bool backfaceCulling) const
    {
        // Implementation of Möller–Trumbore algorithm

        // Edge vectors
        glm::vec3 e1 = v1 - v0;
        glm::vec3 e2 = v2 - v0;

        // Calculate determinant
        glm::vec3 h = glm::cross(m_direction, e2);
        float det = glm::dot(e1, h);

        // Check backface culling
        if (backfaceCulling)
        {
            if (det < std::numeric_limits<float>::epsilon())
            {
                return false;
            }
        }
        else
        {
            // Check if ray is parallel to triangle
            if (std::abs(det) < std::numeric_limits<float>::epsilon())
            {
                return false;
            }
        }

        float invDet = 1.0f / det;

        // Calculate u parameter
        glm::vec3 s = m_origin - v0;
        u = invDet * glm::dot(s, h);

        // Check if intersection is outside triangle (u coordinate)
        if (u < 0.0f || u > 1.0f)
        {
            return false;
        }

        // Calculate v parameter
        glm::vec3 q = glm::cross(s, e1);
        v = invDet * glm::dot(m_direction, q);

        // Check if intersection is outside triangle (u+v coordinate)
        if (v < 0.0f || u + v > 1.0f)
        {
            return false;
        }

        // Calculate t parameter (distance along ray)
        t = invDet * glm::dot(e2, q);

        // Check if intersection is in front of ray origin
        return t > 0.0f;
    }

    bool Ray::intersectTriangleMesh(const std::vector<glm::vec3>& vertices,
                                    const std::vector<unsigned int>& indices,
                                    float& t, int& triangleIndex) const
    {
        // Initialize results
        t = std::numeric_limits<float>::max();
        triangleIndex = -1;
        bool hit = false;

        // Iterate through all triangles in the mesh
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            // Get triangle vertices
            const glm::vec3& v0 = vertices[indices[i]];
            const glm::vec3& v1 = vertices[indices[i + 1]];
            const glm::vec3& v2 = vertices[indices[i + 2]];

            // Test intersection with triangle
            float tTriangle;
            float u, v;
            if (intersectTriangle(v0, v1, v2, tTriangle, u, v))
            {
                // If closer than previous hits, update result
                if (tTriangle < t)
                {
                    t = tTriangle;
                    triangleIndex = static_cast<int>(i / 3);
                    hit = true;
                }
            }
        }

        return hit;
    }

    float Ray::distanceToPoint(const glm::vec3& point) const
    {
        // Vector from ray origin to point
        glm::vec3 v = point - m_origin;

        // Project onto ray direction
        float projDist = glm::dot(v, m_direction);

        // Calculate closest point on ray
        glm::vec3 closestPoint;
        if (projDist < 0.0f)
        {
            // Point projects behind ray origin
            closestPoint = m_origin;
        }
        else
        {
            // Point projects onto ray
            closestPoint = m_origin + m_direction * projDist;
        }

        // Return distance to closest point
        return glm::length(point - closestPoint);
    }

    float Ray::distanceToLineSegment(const glm::vec3& start, const glm::vec3& end,
                                     float& t, glm::vec3& closestPoint) const
    {
        // Calculate segment direction and length
        glm::vec3 segmentDir = end - start;
        float segmentLength = glm::length(segmentDir);

        // Handle degenerate segment
        if (segmentLength < std::numeric_limits<float>::epsilon())
        {
            t = 0.0f;
            closestPoint = start;
            return distanceToPoint(start);
        }

        // Normalize segment direction
        segmentDir /= segmentLength;

        // Compute ray-segment closest points
        // Parameterize ray as p(t) = origin + t * direction
        // Parameterize segment as q(s) = start + s * direction

        // Closest points satisfy:
        // (p(t) - q(s)) dot direction = 0
        // (p(t) - q(s)) dot segmentDir = 0

        // Set up the linear system
        float dirDotSegDir = glm::dot(m_direction, segmentDir);
        glm::vec3 w0 = m_origin - start;
        float a = 1.0f;                             // dot(direction, direction) = 1 (normalized)
        float b = -dirDotSegDir;
        float c = dirDotSegDir;
        float d = -1.0f;                            // -dot(segmentDir, segmentDir) = -1 (normalized)
        float e = glm::dot(w0, m_direction);
        float f = -glm::dot(w0, segmentDir);

        // Compute determinant
        float det = a * d - b * c;

        // Check if ray and segment are parallel
        if (std::abs(det) < std::numeric_limits<float>::epsilon())
        {
            // Find closest point on segment to ray origin
            float dotProduct = glm::dot(w0, segmentDir);
            float segmentParam = -dotProduct;

            // Clamp to segment bounds
            segmentParam = glm::clamp(segmentParam, 0.0f, segmentLength);

            // Calculate closest point on segment
            closestPoint = start + segmentDir * segmentParam;

            // Parameter along ray
            glm::vec3 closestVec = closestPoint - m_origin;
            t = glm::dot(closestVec, m_direction);
            t = std::max(0.0f, t);  // Ensure t is positive

            // Return distance
            return glm::length(getPoint(t) - closestPoint);
        }

        // Solve linear system
        float invDet = 1.0f / det;
        float tRay = (d * e - b * f) * invDet;
        float tSegment = (a * f - c * e) * invDet;

        // Clamp segment parameter to valid range [0, segmentLength]
        tSegment = glm::clamp(tSegment, 0.0f, segmentLength);

        // Recalculate ray parameter using clamped segment parameter
        closestPoint = start + segmentDir * tSegment;
        glm::vec3 closestVec = closestPoint - m_origin;
        t = glm::dot(closestVec, m_direction);
        t = std::max(0.0f, t);  // Ensure t is positive

        // Return distance between closest points
        return glm::length(getPoint(t) - closestPoint);
    }

    float Ray::distanceToPlane(const glm::vec3& normal, float distance) const
    {
        // Calculate signed distance from ray origin to plane
        return glm::dot(normal, m_origin) + distance;
    }

    float Ray::distanceToPlane(const glm::vec3& normal, const glm::vec3& point) const
    {
        // Convert to plane equation form
        float planeDistance = -glm::dot(normal, point);

        // Use the other distance method
        return distanceToPlane(normal, planeDistance);
    }

    void Ray::setDifferentials(const glm::vec3& dPdx, const glm::vec3& dPdy,
                               const glm::vec3& dDdx, const glm::vec3& dDdy)
    {
        m_dPdx = dPdx;
        m_dPdy = dPdy;
        m_dDdx = dDdx;
        m_dDdy = dDdy;
        m_hasDifferentials = true;
    }

    void Ray::debugDraw(float length, const glm::vec3& color) const
    {
        // Draw ray as a line from origin to origin + direction * length
        glm::vec3 endpoint = m_origin + m_direction * length;

        // Use the engine's debug drawing system
        DebugDraw::drawLine(m_origin, endpoint, color);

        // Draw a small sphere at the origin to indicate the ray's start point
        DebugDraw::drawSphere(m_origin, 0.1f, glm::vec3(1.0f, 0.0f, 0.0f));
    }

} // namespace PixelCraft::Utility