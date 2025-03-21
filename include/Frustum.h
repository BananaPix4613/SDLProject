#pragma once

#include <glm.hpp>
#include <vector>

// Represents a plane in 3D space using normal + distance representation
struct Plane
{
    glm::vec3 normal;
    float distance;

    Plane() : normal(0.0f), distance(0.0f) {}

    Plane(const glm::vec3& n, float d) : normal(glm::normalize(n)), distance(d) {}

    // Create a plane from 3 points
    Plane(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
    {
        normal = glm::normalize(glm::cross(b - a, c - a));
        distance = -glm::dot(normal, a);
    }

    // Calculate distance from a point to the plane
    float getSignedDistanceToPoint(const glm::vec3& point) const
    {
        return glm::dot(normal, point) + distance;
    }
};

// Represents a view frustum with 6 planes (near, far, left, right, top, bottom)
class Frustum
{
public:
    enum PlaneID { NEAR, FAR, LEFT, RIGHT, TOP, BOTTOM };
    std::vector<Plane> planes;

    Frustum()
    {
        planes.resize(6);
    }

    // Extract frustum planes from view-projection matrix
    void extractFromMatrix(const glm::mat4& viewProjection)
    {
        // Left plane
        planes[LEFT].normal.x = viewProjection[0][3] + viewProjection[0][0];
        planes[LEFT].normal.y = viewProjection[1][3] + viewProjection[1][0];
        planes[LEFT].normal.z = viewProjection[2][3] + viewProjection[2][0];
        planes[LEFT].distance = viewProjection[3][3] + viewProjection[3][0];

        // Right plane
        planes[RIGHT].normal.x = viewProjection[0][3] - viewProjection[0][0];
        planes[RIGHT].normal.y = viewProjection[1][3] - viewProjection[1][0];
        planes[RIGHT].normal.z = viewProjection[2][3] - viewProjection[2][0];
        planes[RIGHT].distance = viewProjection[3][3] - viewProjection[3][0];

        // Bottom plane
        planes[BOTTOM].normal.x = viewProjection[0][3] + viewProjection[0][1];
        planes[BOTTOM].normal.y = viewProjection[1][3] + viewProjection[1][1];
        planes[BOTTOM].normal.z = viewProjection[2][3] + viewProjection[2][1];
        planes[BOTTOM].distance = viewProjection[3][3] + viewProjection[3][1];

        // Top plane
        planes[TOP].normal.x = viewProjection[0][3] - viewProjection[0][1];
        planes[TOP].normal.y = viewProjection[1][3] - viewProjection[1][1];
        planes[TOP].normal.z = viewProjection[2][3] - viewProjection[2][1];
        planes[TOP].distance = viewProjection[3][3] - viewProjection[3][1];

        // Near plane
        planes[NEAR].normal.x = viewProjection[0][3] + viewProjection[0][2];
        planes[NEAR].normal.y = viewProjection[1][3] + viewProjection[1][2];
        planes[NEAR].normal.z = viewProjection[2][3] + viewProjection[2][2];
        planes[NEAR].distance = viewProjection[3][3] + viewProjection[3][2];

        // Far plane
        planes[FAR].normal.x = viewProjection[0][3] - viewProjection[0][2];
        planes[FAR].normal.y = viewProjection[1][3] - viewProjection[1][2];
        planes[FAR].normal.z = viewProjection[2][3] - viewProjection[2][2];
        planes[FAR].distance = viewProjection[3][3] - viewProjection[3][2];

        // Normalize all plane normals
        for (auto& plane : planes)
        {
            float length = glm::length(plane.normal);
            plane.normal /= length;
            plane.distance /= length;
        }
    }

    // Check if a point is inside the frustum
    bool isPointInside(const glm::vec3& point) const
    {
        for (const auto& plane : planes)
        {
            if (plane.getSignedDistanceToPoint(point) < 0)
            {
                return false;
            }
        }
        return true;
    }

    // Check if a sphere is fully outside the frustum
    bool isSphereOutside(const glm::vec3& center, float radius) const
    {
        for (const auto& plane : planes)
        {
            if (plane.getSignedDistanceToPoint(center) < -radius)
            {
                return true; // Sphere is fully outside this plane
            }
        }
        return false; // Sphere is at least partially inside the frustum
    }

    // Check if a cube is fully outside the frustum
    // This uses the sphere test first (as a quick rejection) then tests corners
    bool isBoxOutside(const glm::vec3& min, const glm::vec3& max) const
    {
        // Compute box center and radius (for sphere test)
        glm::vec3 center = (min + max) * 0.5f;
        float radius = glm::distance(center, max);

        // Quick sphere test first
        if (isSphereOutside(center, radius))
        {
            return true;
        }

        // Test all 8 corners of the box against each plane
        for (const auto& plane : planes)
        {
            int outside = 0;
            outside += (plane.getSignedDistanceToPoint(glm::vec3(min.x, min.y, min.z)) < 0.0f) ? 1 : 0;
            outside += (plane.getSignedDistanceToPoint(glm::vec3(max.x, min.y, min.z)) < 0.0f) ? 1 : 0;
            outside += (plane.getSignedDistanceToPoint(glm::vec3(min.x, max.y, min.z)) < 0.0f) ? 1 : 0;
            outside += (plane.getSignedDistanceToPoint(glm::vec3(max.x, max.y, min.z)) < 0.0f) ? 1 : 0;
            outside += (plane.getSignedDistanceToPoint(glm::vec3(min.x, min.y, max.z)) < 0.0f) ? 1 : 0;
            outside += (plane.getSignedDistanceToPoint(glm::vec3(max.x, min.y, max.z)) < 0.0f) ? 1 : 0;
            outside += (plane.getSignedDistanceToPoint(glm::vec3(min.x, max.y, max.z)) < 0.0f) ? 1 : 0;
            outside += (plane.getSignedDistanceToPoint(glm::vec3(max.x, max.y, max.z)) < 0.0f) ? 1 : 0;

            if (outside == 8)
            {
                return true; // All corners are outside this plane
            }
        }

        return false; // At least some part of the box is visible
    }

    // Helper for testing individual cubes in the grid
    bool isCubeVisible(const glm::vec3& center, float size) const
    {
        glm::vec3 halfSize(size * 0.5f);
        return !isBoxOutside(center - halfSize, center + halfSize);
    }
};