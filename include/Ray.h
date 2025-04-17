#pragma once

#include <glm.hpp>
#include <vector>

/**
 * @class Ray
 * @brief Represents a ray in 3D space for raycasting operations
 * 
 * The Ray class provides methods for raycasting against various primitives
 * and calculating closest distances to lines and planes. It's used extensively
 * by editor tools for selection and manipulation.
 */
class Ray
{
public:
    /**
     * @brief Default constructor
     */
    Ray();

    /**
     * @brief Constructor with origin and direction
     * @param origin Ray starting point
     * @param direction Ray direction (will be normalized)
     */
    Ray(const glm::vec3& origin, const glm::vec3& direction);

    /**
     * @brief Get ray origin
     * @return Origin point
     */
    const glm::vec3& getOrigin() const;

    /**
     * @brief Get ray direction
     * @return Normalized direction vector
     */
    const glm::vec3& getDirection() const;

    /**
     * @brief Set ray origin
     * @param origin New origin point
     */
    void setOrigin(const glm::vec3& origin);

    /**
     * @brief Set ray direction
     * @param direction New direction (will be normalized)
     */
    void setDirection(const glm::vec3& direction);

    /**
     * @brief Get point at distance along ray
     * @param distance Distance along ray
     * @return Point at specified distance
     */
    glm::vec3 getPoint(float distance) const;

    /**
     * @brief Check ray intersection with an axis-aligned bounding box
     * @param min Minimum corner of the box
     * @param max Maximum corner of the box
     * @param t [out] Distance to intersection if hit
     * @return True if ray intersects the box
     */
    bool intersectAABB(const glm::vec3& min, const glm::vec3& max, float& t) const;

    /**
     * @brief Check ray intersection with a sphere
     * @param center Sphere center
     * @param radius Sphere radius
     * @param t [out] Distance to intersection if hit
     * @return True if ray intersects with the sphere
     */
    bool intersectSphere(const glm::vec3& center, float radius, float& t) const;

    /**
     * @brief Check ray intersection with a plane
     * @param planePoint Point on the plane
     * @param planeNormal Plane normal (will be normalized)
     * @param t [out] Distance to intersection if hit
     * @param intersectionPoint [out] Point of intersection
     * @return True if ray intersects the plane
     */
    bool intersectPlane(const glm::vec3& planePoint, const glm::vec3& planeNormal,
                        float& t, glm::vec3& intersectionPoint) const;

    /**
     * @brief Check ray interseciton with a triangle
     * @param v0 First vertex
     * @param v1 Second vertex
     * @param v2 Third vertex
     * @param t [out] Distance to intersection if hit
     * @param barycentricCoords [out] Barycentric coordinates of hit point
     * @return True if ray intersects the triangle
     */
    bool intersectTriangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
                           float& t, glm::vec2& barycentricCoords) const;

    /**
     * @brief Calculate closest distance from ray to a point
     * @param point Point to check
     * @param t [out] Parameter along ray where closest point occurs
     * @return Distance from ray to point
     */
    float distanceToPoint(const glm::vec3& point, float& t) const;

    /**
     * @brief Calculate closest distance from ray to a line segment
     * @param lineStart Start point of line segment
     * @param lineEnd End point of line segment
     * @param t [out] Parameter along ray where closest point occurs
     * @return Distance from ray to line segment
     */
    float distanceToLineSegment(const glm::vec3& lineStart, const glm::vec3& lineEnd, float& t) const;

    /**
     * @brief Check if ray intersects multiple triangles
     * @param vertices Vertex array
     * @param indices Index array defining triangles
     * @param transforms Optional array of transforms to apply to vertices
     * @param t [out] Distance to closest intersection
     * @param triangleIndex [out] Index of intersected triangle
     * @return True if ray intersects any triangle
     */
    bool intersectTriangleMesh(const std::vector<glm::vec3>& vertices,
                               const std::vector<unsigned int>& indices,
                               const std::vector<glm::mat4>* transforms,
                               float& t, int& triangleIndex) const;

public:
    glm::vec3 m_origin;      // Starting point of the ray
    glm::vec3 m_direction;   // Normalized direction of the ray

    // Cached inverse direction for faster AABB intersection tests
    glm::vec3 m_invDirection;
    int m_dirIsNeg[3];       // Sign of direction components (optimization for AABB tests)
};