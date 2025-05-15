// -------------------------------------------------------------------------
// AABB.cpp
// -------------------------------------------------------------------------
#include "Utility/AABB.h"
#include "Utility/Ray.h"
#include "Utility/Frustum.h"
#include "Core/Logger.h"
#include "Utility/DebugDraw.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Utility
{

    AABB::AABB()
        : m_min(std::numeric_limits<float>::max())
        , m_max(-std::numeric_limits<float>::max())
    {
    }

    AABB::AABB(const glm::vec3& min, const glm::vec3& max)
        : m_min(min)
        , m_max(max)
    {
    }

    AABB::AABB(const glm::vec3& center, float halfExtentX, float halfExtentY, float halfExtentZ)
        : m_min(center.x - halfExtentX, center.y - halfExtentY, center.z - halfExtentZ)
        , m_max(center.x + halfExtentX, center.y + halfExtentY, center.z + halfExtentZ)
    {
    }

    AABB AABB::createFromPoints(const std::vector<glm::vec3>& points)
    {
        if (points.empty())
        {
            return AABB();
        }

        glm::vec3 min = points[0];
        glm::vec3 max = points[0];

        for (size_t i = 1; i < points.size(); ++i)
        {
            min.x = std::min(min.x, points[i].x);
            min.y = std::min(min.y, points[i].y);
            min.z = std::min(min.z, points[i].z);

            max.x = std::max(max.x, points[i].x);
            max.y = std::max(max.y, points[i].y);
            max.z = std::max(max.z, points[i].z);
        }

        return AABB(min, max);
    }

    AABB AABB::createFromSphere(const glm::vec3& center, float radius)
    {
        glm::vec3 extent(radius, radius, radius);
        return AABB(center - extent, center + extent);
    }

    AABB AABB::createFromTransformedAABB(const AABB& aabb, const glm::mat4& transform)
    {
        // Get the 8 corners of the source AABB
        std::array<glm::vec3, 8> corners = aabb.getAllCorners();

        // Transform each corner
        for (auto& corner : corners)
        {
            glm::vec4 transformedCorner = transform * glm::vec4(corner, 1.0f);
            corner = glm::vec3(transformedCorner) / transformedCorner.w;
        }

        // Find the min/max of the transformed corners
        glm::vec3 min = corners[0];
        glm::vec3 max = corners[0];

        for (size_t i = 1; i < 8; ++i)
        {
            min.x = std::min(min.x, corners[i].x);
            min.y = std::min(min.y, corners[i].y);
            min.z = std::min(min.z, corners[i].z);

            max.x = std::max(max.x, corners[i].x);
            max.y = std::max(max.y, corners[i].y);
            max.z = std::max(max.z, corners[i].z);
        }

        return AABB(min, max);
    }

    glm::vec3 AABB::getMin() const
    {
        return m_min;
    }

    glm::vec3 AABB::getMax() const
    {
        return m_max;
    }

    void AABB::setMin(const glm::vec3& min)
    {
        m_min = min;
    }

    void AABB::setMax(const glm::vec3& max)
    {
        m_max = max;
    }

    glm::vec3 AABB::getCenter() const
    {
        return (m_min + m_max) * 0.5f;
    }

    glm::vec3 AABB::getExtents() const
    {
        return (m_max - m_min) * 0.5f;
    }

    glm::vec3 AABB::getSize() const
    {
        return m_max - m_min;
    }

    float AABB::getVolume() const
    {
        if (!isValid())
        {
            return 0.0f;
        }

        glm::vec3 size = getSize();
        return size.x * size.y * size.z;
    }

    float AABB::getSurfaceArea() const
    {
        if (!isValid())
        {
            return 0.0f;
        }

        glm::vec3 size = getSize();
        return 2.0f * (size.x * size.y + size.x * size.z + size.y * size.z);
    }

    float AABB::getLongestAxis() const
    {
        glm::vec3 size = getSize();
        return std::max(std::max(size.x, size.y), size.z);
    }

    int AABB::getLongestAxisIndex() const
    {
        glm::vec3 size = getSize();
        if (size.x >= size.y && size.x >= size.z)
        {
            return 0; // X is longest
        }
        else if (size.y >= size.x && size.y >= size.z)
        {
            return 1; // Y is longest
        }
        else
        {
            return 2; // Z is longest
        }
    }

    float AABB::getShortestAxis() const
    {
        glm::vec3 size = getSize();
        return std::min(std::min(size.x, size.y), size.z);
    }

    int AABB::getShortestAxisIndex() const
    {
        glm::vec3 size = getSize();
        if (size.x <= size.y && size.x <= size.z)
        {
            return 0; // X is shortest
        }
        else if (size.y <= size.x && size.y <= size.z)
        {
            return 1; // Y is shortest
        }
        else
        {
            return 2; // Z is shortest
        }
    }

    glm::vec3 AABB::getCorner(int index) const
    {
        // Use binary pattern: 
        // bit 0 (LSB): x-coordinate (0 = min.x, 1 = max.x)
        // bit 1: y-coordinate (0 = min.y, 1 = max.y)
        // bit 2: z-coordinate (0 = min.z, 1 = max.z)
        return glm::vec3(
            (index & 1) ? m_max.x : m_min.x,
            (index & 2) ? m_max.y : m_min.y,
            (index & 4) ? m_max.z : m_min.z
        );
    }

    std::array<glm::vec3, 8> AABB::getAllCorners() const
    {
        std::array<glm::vec3, 8> corners;
        for (int i = 0; i < 8; ++i)
        {
            corners[i] = getCorner(i);
        }
        return corners;
    }

    void AABB::expand(float amount)
    {
        m_min -= glm::vec3(amount);
        m_max += glm::vec3(amount);
    }

    void AABB::expand(const glm::vec3& amount)
    {
        m_min -= amount;
        m_max += amount;
    }

    void AABB::translate(const glm::vec3& delta)
    {
        m_min += delta;
        m_max += delta;
    }

    void AABB::transform(const glm::mat4& transform)
    {
        *this = createFromTransformedAABB(*this, transform);
    }

    AABB AABB::merge(const AABB& other) const
    {
        return AABB(
            glm::min(m_min, other.m_min),
            glm::max(m_max, other.m_max)
        );
    }

    void AABB::include(const glm::vec3& point)
    {
        m_min = glm::min(m_min, point);
        m_max = glm::max(m_max, point);
    }

    void AABB::include(const AABB& other)
    {
        if (other.isValid())
        {
            include(other.m_min);
            include(other.m_max);
        }
    }

    bool AABB::isEmpty() const
    {
        return m_min.x >= m_max.x || m_min.y >= m_max.y || m_min.z >= m_max.z;
    }

    bool AABB::isValid() const
    {
        return m_min.x <= m_max.x && m_min.y <= m_max.y && m_min.z <= m_max.z;
    }

    bool AABB::contains(const glm::vec3& point) const
    {
        return point.x >= m_min.x && point.x <= m_max.x &&
            point.y >= m_min.y && point.y <= m_max.y &&
            point.z >= m_min.z && point.z <= m_max.z;
    }

    bool AABB::contains(const AABB& other) const
    {
        return other.m_min.x >= m_min.x && other.m_max.x <= m_max.x &&
            other.m_min.y >= m_min.y && other.m_max.y <= m_max.y &&
            other.m_min.z >= m_min.z && other.m_max.z <= m_max.z;
    }

    bool AABB::intersects(const AABB& other) const
    {
        return m_min.x <= other.m_max.x && m_max.x >= other.m_min.x &&
            m_min.y <= other.m_max.y && m_max.y >= other.m_min.y &&
            m_min.z <= other.m_max.z && m_max.z >= other.m_min.z;
    }

    bool AABB::intersects(const Ray& ray, float& tMin, float& tMax) const
    {
        // Implementation based on the slab method
        // Assumes Ray has getOrigin(), getDirection(), and getInvDirection() methods

        tMin = -std::numeric_limits<float>::infinity();
        tMax = std::numeric_limits<float>::infinity();

        // Get ray properties
        const glm::vec3& origin = ray.getOrigin();
        const glm::vec3& direction = ray.getDirection();
        const glm::vec3& invDirection = ray.getInvDirection();

        // For each axis
        for (int i = 0; i < 3; ++i)
        {
            if (std::abs(direction[i]) < std::numeric_limits<float>::epsilon())
            {
                // Ray is parallel to slab. Check if origin is within slab
                if (origin[i] < m_min[i] || origin[i] > m_max[i])
                {
                    return false;
                }
            }
            else
            {
                // Ray is not parallel - compute intersection distances
                float t1 = (m_min[i] - origin[i]) * invDirection[i];
                float t2 = (m_max[i] - origin[i]) * invDirection[i];

                // Ensure t1 <= t2
                if (t1 > t2)
                {
                    std::swap(t1, t2);
                }

                // Update tMin and tMax
                tMin = std::max(tMin, t1);
                tMax = std::min(tMax, t2);

                // Check for no intersection
                if (tMin > tMax)
                {
                    return false;
                }
            }
        }

        // If we get here, ray intersects AABB
        return true;
    }

    bool AABB::intersects(const Frustum& frustum) const
    {
        // This implementation requires the Frustum class to have a method
        // that tests against an AABB or its points.
        // Without knowing the exact Frustum API, we'll provide a skeleton.

        // Typical frustum-AABB test approaches:
        // 1. Test if any AABB corner is inside the frustum
        // 2. Test AABB against each frustum plane with early-out

        // NOTE: This is a placeholder implementation. 
        // Actual implementation depends on Frustum class interface.

        Log::warn("AABB::intersects(Frustum): Using placeholder implementation. Implement based on actual Frustum API.");

        // Assuming Frustum has a containsPoint method
        // auto corners = getAllCorners();
        // for (const auto& corner : corners) {
        //     if (frustum.containsPoint(corner)) {
        //         return true;
        //     }
        // }

        // Conservative fallback: assume intersection
        return true;
    }

    bool AABB::intersects(const glm::vec3& center, float radius) const
    {
        // Get closest point on AABB to sphere center
        glm::vec3 closestPoint = getClosestPoint(center);

        // Check if squared distance from closest point to sphere center is <= radius²
        float distanceSquared = glm::length2(closestPoint - center);
        return distanceSquared <= radius * radius;
    }

    glm::vec3 AABB::getClosestPoint(const glm::vec3& point) const
    {
        // Clamp point to AABB bounds
        return glm::clamp(point, m_min, m_max);
    }

    float AABB::getDistanceSquared(const glm::vec3& point) const
    {
        // If point is inside AABB, distance is 0
        if (contains(point))
        {
            return 0.0f;
        }

        // Otherwise, distance is from point to closest point on AABB
        glm::vec3 closestPoint = getClosestPoint(point);
        return glm::length2(point - closestPoint);
    }

    float AABB::getDistance(const glm::vec3& point) const
    {
        return std::sqrt(getDistanceSquared(point));
    }

    void AABB::debugDraw(const glm::vec3& color) const
    {
        if (!isValid())
        {
            return;
        }

        // Get all 8 corners of the AABB
        auto corners = getAllCorners();

        // Draw the 12 edges of the AABB
        // Assumes DebugDraw has a drawLine(start, end, color) method

        //// Bottom face
        //DebugDraw::drawLine(corners[0], corners[1], color);
        //DebugDraw::drawLine(corners[1], corners[3], color);
        //DebugDraw::drawLine(corners[3], corners[2], color);
        //DebugDraw::drawLine(corners[2], corners[0], color);

        //// Top face
        //DebugDraw::drawLine(corners[4], corners[5], color);
        //DebugDraw::drawLine(corners[5], corners[7], color);
        //DebugDraw::drawLine(corners[7], corners[6], color);
        //DebugDraw::drawLine(corners[6], corners[4], color);

        //// Connecting edges
        //DebugDraw::drawLine(corners[0], corners[4], color);
        //DebugDraw::drawLine(corners[1], corners[5], color);
        //DebugDraw::drawLine(corners[2], corners[6], color);
        //DebugDraw::drawLine(corners[3], corners[7], color);
    }

} // namespace PixelCraft::Utility