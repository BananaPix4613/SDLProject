#include "Ray.h"
#include <algorithm>
#include <limits>

Ray::Ray() :
    m_origin(0.0f), m_direction(0.0f, 0.0f, 1.0f)
{
    // Pre-compute inverse direction and signs for faster AABB intersection
    m_invDirection = 1.0f / m_direction;
    m_dirIsNeg[0] = m_direction.x < 0;
    m_dirIsNeg[1] = m_direction.y < 0;
    m_dirIsNeg[2] = m_direction.z < 0;
}

Ray::Ray(const glm::vec3& origin, const glm::vec3& direction) :
    m_origin(origin)
{
    // Normalize direction
    m_direction = glm::normalize(direction);

    // Pre-compute inverse direction and signs for faster AABB intersection
    m_invDirection = 1.0f / m_direction;
    m_dirIsNeg[0] = m_direction.x < 0;
    m_dirIsNeg[1] = m_direction.y < 0;
    m_dirIsNeg[2] = m_direction.z < 0;
}

const glm::vec3& Ray::getOrigin() const
{
    return m_origin;
}

const glm::vec3& Ray::getDirection() const
{
    return m_direction;
}

void Ray::setOrigin(const glm::vec3& origin)
{
    m_origin = origin;
}

void Ray::setDirection(const glm::vec3& direction)
{
    m_direction = glm::normalize(direction);

    // Update cached values
    m_invDirection = 1.0f / m_direction;
    m_dirIsNeg[0] = m_direction.x < 0;
    m_dirIsNeg[1] = m_direction.y < 0;
    m_dirIsNeg[2] = m_direction.z < 0;
}

glm::vec3 Ray::getPoint(float distance) const
{
    return m_origin + m_direction * distance;
}

bool Ray::intersectAABB(const glm::vec3& min, const glm::vec3& max, float& t) const
{
    // Optimized AABB test using pre-computed inverse direction
    float tmin = (min.x - m_origin.x) * m_invDirection.x;
    float tmax = (max.x - m_origin.x) * m_invDirection.x;

    if (m_dirIsNeg[0])
    {
        std::swap(tmin, tmax);
    }

    float tymin = (min.y - m_origin.y) * m_invDirection.y;
    float tymax = (max.y - m_origin.y) * m_invDirection.y;

    if (m_dirIsNeg[1])
    {
        std::swap(tymin, tymax);
    }

    if ((tmin > tymax) || (tymin > tmax))
        return false;

    if (tymin > tmin)
        tmin = tymin;
    if (tymax < tmax)
        tmax = tymax;

    float tzmin = (min.z - m_origin.z) * m_invDirection.z;
    float tzmax = (max.z - m_origin.z) * m_invDirection.z;

    if (m_dirIsNeg[2])
    {
        std::swap(tzmin, tzmax);
    }

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;

    if (tzmin > tmin)
        tmin = tzmin;
    if (tzmax < tmax)
        tmax = tzmax;

    // Check if intersection is behind the ray
    if (tmax < 0)
        return false;

    // Return closest intersection in front of the ray
    t = (tmin < 0) ? tmax : tmin;

    return true;
}

bool Ray::intersectSphere(const glm::vec3& center, float radius, float& t) const
{
    glm::vec3 oc = m_origin - center;

    float a = glm::dot(m_direction, m_direction);  // Always 1 if direction is normalized
    float b = 2.0f * glm::dot(oc, m_direction);
    float c = glm::dot(oc, oc) - radius * radius;

    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0)
        return false;

    float sqrtDiscriminant = sqrt(discriminant);

    // Try smaller root first (closest interaction)
    float t0 = (-b - sqrtDiscriminant) / (2.0f * a);

    // If behind the ray, try the farther intersection
    if (t0 < 0)
    {
        t0 = (-b + sqrtDiscriminant) / (2.0f * a);

        // If still behind the ray, no valid intersection
        if (t0 < 0)
            return false;
    }

    t = t0;
    return true;
}

bool Ray::intersectPlane(const glm::vec3& planePoint, const glm::vec3& planeNormal,
                         float& t, glm::vec3& intersectionPoint) const
{
    // Normalize the plane normal
    glm::vec3 normal = glm::normalize(planeNormal);

    // Calculate denominator
    float denom = glm::dot(normal, m_direction);

    // Check if ray is parallel to the plane
    if (std::abs(denom) < 1e-6f)
        return false;

    // Calculate distance to intersection
    t = glm::dot(planePoint - m_origin, normal) / denom;

    // Check if intersection is behind the ray
    if (t < 0)
        return false;

    // Calculate intersection point
    intersectionPoint = m_origin + m_direction * t;

    return true;
}

bool Ray::intersectTriangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
                            float& t, glm::vec2& barycentricCoords) const
{
    // Möller–Trumbore ray-triangle intersection algorithm

    // Edge vectors
    glm::vec3 e1 = v1 - v0;
    glm::vec3 e2 = v2 - v0;

    // Calculate determinant
    glm::vec3 p = glm::cross(m_direction, e2);
    float det = glm::dot(e1, p);

    // If determinant is near zero, ray lies in plane of triangle or ray is parallel to plane
    if (std::abs(det) < 1e-6f)
        return false;

    float invDet = 1.0f / det;

    // Calculate distance from v0 to ray origin
    glm::vec3 s = m_origin - v0;

    // Calculate u parameter
    float u = glm::dot(s, p) * invDet;

    // Check  if intersection is outside the triangle
    if (u < 0.0f || u > 1.0f)
        return false;

    // Calculate q and v parameter
    glm::vec3 q = glm::cross(s, e1);
    float v = glm::dot(m_direction, q) * invDet;

    // Check if intersection is outside the triangle
    if (v < 0.0f || u + v > 1.0f)
        return false;

    // Calculate t, ray intersection
    t = glm::dot(e2, q) * invDet;

    // Check if intersection is behind the ray
    if (t < 0.0f)
        return false;

    // Output barycentric coordinates
    barycentricCoords = glm::vec2(u, v);

    return true;
}

float Ray::distanceToPoint(const glm::vec3& point, float& t) const
{
    // Calculate parameter t along ray where closest point occurs
    t = glm::dot(point - m_origin, m_direction);

    // If t is negative, closest point is behind the ray
    if (t < 0)
    {
        // Closest point is ray origin
        t = 0;
        return glm::length(point - m_origin);
    }

    // Calculate closest point on ray
    glm::vec3 closestPoint = m_origin + m_direction * t;

    // Return distance from point to closest point on ray
    return glm::length(point - closestPoint);
}

float Ray::distanceToLineSegment(const glm::vec3& lineStart, const glm::vec3& lineEnd, float& t) const
{
    // Line segment direction
    glm::vec3 segDir = lineEnd - lineStart;
    float segLength = glm::length(segDir);

    // Handle degenerate case
    if (segLength < 1e-6f)
    {
        return distanceToPoint(lineStart, t);
    }

    // Normalize segment direction
    segDir /= segLength;

    // Calculate closest point between ray and line segment
    glm::vec3 v1 = m_origin - lineStart;
    glm::vec3 v2 = m_direction;
    glm::vec3 v3 = segDir;

    float d1 = glm::dot(v1, v2);
    float d2 = glm::dot(v1, v3);
    float d3 = glm::dot(v2, v3);

    float denominator = 1.0f - d3 * d3;

    // Handle parallel lines case
    if (std::abs(denominator) < 1e-6f)
    {
        // Find closest point on line segment to ray origin
        float t0 = d2 / segLength;
        t0 = std::max(0.0f, std::min(1.0f, t0));  // Clamp to segment

        // Distance from origin to point on segment
        glm::vec3 closestOnSegment = lineStart + segDir * t0 * segLength;
        float distOriginToSegment = glm::length(closestOnSegment - m_origin);

        // Find closest point on ray to segment start
        float tRay = std::max(0.0f, d1);
        glm::vec3 closestOnRay = m_origin + m_direction * tRay;
        float distRayToStart = glm::length(closestOnRay - lineStart);

        // Return the smaller distance
        if (distOriginToSegment < distRayToStart)
        {
            t = 0.0f;
            return distOriginToSegment;
        }
        else
        {
            t = tRay;
            return distRayToStart;
        }
    }

    // Calculate parameters
    float tray = (d3 * d2 - d1) / denominator;
    float tseg = (d1 * d3 - d2) / denominator;

    // Clamp to segment
    tseg = std::max(0.0f, std::min(1.0f, tseg));

    // Recalculate tray based on clamped tseg
    if (tseg <= 0.0f || tseg >= 1.0f)
    {
        glm::vec3 pointOnSeg = lineStart + segDir * tseg * segLength;
        glm::vec3 dir = pointOnSeg - m_origin;
        tray = glm::dot(dir, m_direction);
    }

    // If tray is negative, closest point is behind the ray
    if (tray < 0.0f)
    {
        tray = 0.0f;
    }

    // Calculate closest points
    glm::vec3 closestOnRay = m_origin + m_direction * tray;
    glm::vec3 closestOnSeg = lineStart + segDir * tseg * segLength;

    // Set output parameter
    t = tray;

    // Return distance between closest points
    return glm::length(closestOnRay - closestOnSeg);
}

bool Ray::intersectTriangleMesh(const std::vector<glm::vec3>& vertices,
                                const std::vector<unsigned int>& indices,
                                const std::vector<glm::mat4>* transforms,
                                float& t, int& triangleIndex) const
{
    t = std::numeric_limits<float>::max();
    triangleIndex = -1;
    bool hit = false;

    // Check if we're using instanced rendering with transforms
    bool useTransforms = transforms != nullptr && !transforms->empty();

    // Check each triangle
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        // Get vertex indices
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];

        // Check each transform (or just once if no transforms)
        for (size_t j = 0; j < (useTransforms ? transforms->size() : 1); j++)
        {
            // Get triangle vertices (transformed if needed)
            glm::vec3 v0, v1, v2;

            if (useTransforms)
            {
                // Apply transformation
                const glm::mat4& transform = (*transforms)[j];
                v0 = glm::vec3(transform * glm::vec4(vertices[i0], 1.0f));
                v1 = glm::vec3(transform * glm::vec4(vertices[i1], 1.0f));
                v2 = glm::vec3(transform * glm::vec4(vertices[i2], 1.0f));
            }
            else
            {
                // Use vertices directly
                v0 = vertices[i0];
                v1 = vertices[i1];
                v2 = vertices[i2];
            }

            // Test intersection
            float tHit;
            glm::vec2 baryCoords;

            if (intersectTriangle(v0, v1, v2, tHit, baryCoords))
            {
                // Keep track of closest intersection
                if (tHit < t)
                {
                    t = tHit;
                    triangleIndex = static_cast<int>(i / 3);
                    hit = true;
                }
            }
        }
    }

    return hit;
}