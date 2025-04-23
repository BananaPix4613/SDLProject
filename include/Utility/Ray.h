// -------------------------------------------------------------------------
// Ray.h
// -------------------------------------------------------------------------
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <memory>

namespace PixelCraft::Utility
{

    // Forward declarations
    class AABB;

    /**
     * @brief Ray class for raycasting operations in physics, rendering, and user interaction

     * Represents a ray with origin and direction, providing optimized intersection tests
     * with various primitives, transformation operations, distance calculations, and
     * debug visualization.
     */
    class Ray
    {
    public:
        /**
         * @brief Default constructor creating a ray along positive Z-axis
         */
        Ray();

        /**
         * @brief Constructor with origin and direction
         * @param origin The origin point of the ray
         * @param direction The direction vector of the ray (will be normalized)
         */
        Ray(const glm::vec3& origin, const glm::vec3& direction);

        /**
         * @brief Get the ray's origin point
         * @return The origin point
         */
        const glm::vec3& getOrigin() const
        {
            return m_origin;
        }

        /**
         * @brief Get the ray's direction vector
         * @return The normalized direction vector
         */
        const glm::vec3& getDirection() const
        {
            return m_direction;
        }

        /**
         * @brief Set the ray's origin point
         * @param origin The new origin point
         */
        void setOrigin(const glm::vec3& origin);

        /**
         * @brief Set the ray's direction vector
         * @param direction The new direction vector (will be normalized)
         */
        void setDirection(const glm::vec3& direction);

        /**
         * @brief Get the inverse of the ray's direction vector
         * Used for optimized intersection tests
         * @return The inverse direction vector (1/dx, 1/dy, 1/dz)
         */
        const glm::vec3& getInvDirection() const
        {
            return m_invDirection;
        }

        /**
         * @brief Get signs of direction components for optimized AABB intersection
         * @return Vector with 1 for negative components, 0 for positive components
         */
        const glm::ivec3& getDirIsNeg() const
        {
            return m_dirIsNeg;
        }

        /**
         * @brief Get a point along the ray at distance t
         * @param t Distance along the ray
         * @return The point at origin + t * direction
         */
        glm::vec3 getPoint(float t) const
        {
            return m_origin + m_direction * t;
        }

        /**
         * @brief Transform the ray by a 4x4 matrix
         * @param matrix The transformation matrix
         * @return A new transformed ray
         */
        Ray transform(const glm::mat4& matrix) const;

        /**
         * @brief Test intersection with an axis-aligned bounding box
         * Uses the optimized slab method with cached inverse direction
         * @param aabb The axis-aligned bounding box to test against
         * @param tMin [out] Near intersection distance if hit
         * @param tMax [out] Far intersection distance if hit
         * @return True if the ray intersects the AABB
         */
        bool intersectAABB(const AABB& aabb, float& tMin, float& tMax) const;

        /**
         * @brief Test intersection with a sphere
         * @param center The center of the sphere
         * @param radius The radius of the sphere
         * @param t [out] Distance along the ray to the intersection point
         * @return True if the ray intersects the sphere
         */
        bool intersectSphere(const glm::vec3& center, float radius, float& t) const;

        /**
         * @brief Test intersection with a plane defined by normal and distance
         * @param normal The plane normal (must be normalized)
         * @param distance The plane distance from origin
         * @param t [out] Distance along the ray to the intersection point
         * @return True if the ray intersects the plane
         */
        bool intersectPlane(const glm::vec3& normal, float distance, float& t) const;

        /**
         * @brief Test intersection with a plane defined by normal and point
         * @param normal The plane normal (must be normalized)
         * @param point A point on the plane
         * @param t [out] Distance along the ray to the intersection point
         * @return True if the ray intersects the plane
         */
        bool intersectPlane(const glm::vec3& normal, const glm::vec3& point, float& t) const;

        /**
         * @brief Test intersection with a triangle using Möller–Trumbore algorithm
         * @param v0 First vertex of the triangle
         * @param v1 Second vertex of the triangle
         * @param v2 Third vertex of the triangle
         * @param t [out] Distance along the ray to the intersection point
         * @param u [out] Barycentric coordinate of the intersection
         * @param v [out] Barycentric coordinate of the intersection
         * @param backfaceCulling Whether to ignore intersections with back faces
         * @return True if the ray intersects the triangle
         */
        bool intersectTriangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
                               float& t, float& u, float& v, bool backfaceCulling = false) const;

        /**
         * @brief Test intersection with a triangle mesh
         * @param vertices Array of vertex positions
         * @param indices Array of triangle indices
         * @param t [out] Distance along the ray to the closest intersection point
         * @param triangleIndex [out] Index of the intersected triangle
         * @return True if the ray intersects any triangle in the mesh
         */
        bool intersectTriangleMesh(const std::vector<glm::vec3>& vertices,
                                   const std::vector<unsigned int>& indices,
                                   float& t, int& triangleIndex) const;

        /**
         * @brief Calculate distance from a point to the ray
         * @param point The point to calculate distance to
         * @return The shortest distance from the point to the ray
         */
        float distanceToPoint(const glm::vec3& point) const;

        /**
         * @brief Calculate distance from a line segment to the ray
         * @param start Start point of the line segment
         * @param end End point of the line segment
         * @param t [out] Parameter along the ray for closest point
         * @param closestPoint [out] Closest point on the line segment to the ray
         * @return The shortest distance from the ray to the line segment
         */
        float distanceToLineSegment(const glm::vec3& start, const glm::vec3& end,
                                    float& t, glm::vec3& closestPoint) const;

        /**
         * @brief Calculate signed distance from the ray origin to a plane
         * @param normal The plane normal (must be normalized)
         * @param distance The plane distance from origin
         * @return The signed distance from ray origin to plane
         */
        float distanceToPlane(const glm::vec3& normal, float distance) const;

        /**
         * @brief Calculate signed distance from the ray origin to a plane
         * @param normal The plane normal (must be normalized)
         * @param point A point on the plane
         * @return The signed distance from ray origin to plane
         */
        float distanceToPlane(const glm::vec3& normal, const glm::vec3& point) const;

        /**
         * @brief Set ray differentials for texture mapping
         * Ray differentials represent how the ray changes with respect to
         * screen-space coordinates, used for texture filtering.
         * @param dPdx Ray origin differential in x direction
         * @param dPdy Ray origin differential in y direction
         * @param dDdx Ray direction differential in x direction
         * @param dDdy Ray direction differential in y direction
         */
        void setDifferentials(const glm::vec3& dPdx, const glm::vec3& dPdy,
                              const glm::vec3& dDdx, const glm::vec3& dDdy);

        /**
         * @brief Check if the ray has differentials set
         * @return True if ray differentials are available
         */
        bool hasDifferentials() const
        {
            return m_hasDifferentials;
        }

        /**
         * @brief Get the ray origin differential in x direction
         * @return The ray origin differential in x direction
         */
        glm::vec3 getDPdx() const
        {
            return m_dPdx;
        }

        /**
         * @brief Get the ray origin differential in y direction
         * @return The ray origin differential in y direction
         */
        glm::vec3 getDPdy() const
        {
            return m_dPdy;
        }

        /**
         * @brief Get the ray direction differential in x direction
         * @return The ray direction differential in x direction
         */
        glm::vec3 getDDdx() const
        {
            return m_dDdx;
        }

        /**
         * @brief Get the ray direction differential in y direction
         * @return The ray direction differential in y direction
         */
        glm::vec3 getDDdy() const
        {
            return m_dDdy;
        }

        /**
         * @brief Draw the ray for debugging purposes
         * @param length Length of the ray to draw
         * @param color Color of the ray
         */
        void debugDraw(float length = 100.0f, const glm::vec3& color = glm::vec3(1.0f, 1.0f, 0.0f)) const;

    private:
        glm::vec3 m_origin;      ///< Ray origin point
        glm::vec3 m_direction;   ///< Normalized ray direction

        // Cached values for optimized intersection tests
        glm::vec3 m_invDirection;  ///< Inverse of direction (1/dx, 1/dy, 1/dz)
        glm::ivec3 m_dirIsNeg;     ///< Signs of direction components (1 if negative, 0 if positive)

        // Ray differentials for texture filtering
        bool m_hasDifferentials;   ///< Whether differentials are available
        glm::vec3 m_dPdx;          ///< Ray origin differential in x
        glm::vec3 m_dPdy;          ///< Ray origin differential in y
        glm::vec3 m_dDdx;          ///< Ray direction differential in x
        glm::vec3 m_dDdy;          ///< Ray direction differential in y

        /**
         * @brief Update cached values after changing origin or direction
         */
        void updateCachedValues();
    };

} // namespace PixelCraft::Utility