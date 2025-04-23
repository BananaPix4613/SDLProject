// -------------------------------------------------------------------------
// AABB.h
// -------------------------------------------------------------------------
#pragma once

#include <vector>
#include <array>
#include <glm/glm.hpp>

// Forward declarations
namespace PixelCraft
{
    namespace Utility
    {
        class Ray;
        class Frustum;
    }
}

namespace PixelCraft::Utility
{

    /**
     * @brief Axis-Aligned Bounding Box for efficient collision detection and spatial partitioning
     *
     * An AABB represents a box in 3D space with faces aligned to the coordinate axes,
     * defined by minimum and maximum points in each dimension.
     */
    class AABB
    {
    public:
        /**
         * @brief Default constructor, creates an invalid (empty) AABB
         */
        AABB();

        /**
         * @brief Construct from min and max points
         * @param min The minimum point of the box (smallest values in each dimension)
         * @param max The maximum point of the box (largest values in each dimension)
         */
        AABB(const glm::vec3& min, const glm::vec3& max);

        /**
         * @brief Construct from center and half extents
         * @param center The center point of the box
         * @param halfExtentX The half-size in X dimension
         * @param halfExtentY The half-size in Y dimension
         * @param halfExtentZ The half-size in Z dimension
         */
        AABB(const glm::vec3& center, float halfExtentX, float halfExtentY, float halfExtentZ);

        /**
         * @brief Create an AABB that contains all given points
         * @param points A collection of points to enclose
         * @return AABB containing all points
         */
        static AABB createFromPoints(const std::vector<glm::vec3>& points);

        /**
         * @brief Create an AABB that contains a sphere
         * @param center The center of the sphere
         * @param radius The radius of the sphere
         * @return AABB containing the sphere
         */
        static AABB createFromSphere(const glm::vec3& center, float radius);

        /**
         * @brief Create an AABB that contains a transformed AABB
         * @param aabb The source AABB
         * @param transform The transformation matrix to apply
         * @return New AABB containing the transformed source AABB
         */
        static AABB createFromTransformedAABB(const AABB& aabb, const glm::mat4& transform);

        /**
         * @brief Get the minimum point of the box
         * @return The minimum point (smallest values in each dimension)
         */
        glm::vec3 getMin() const;

        /**
         * @brief Get the maximum point of the box
         * @return The maximum point (largest values in each dimension)
         */
        glm::vec3 getMax() const;

        /**
         * @brief Set the minimum point of the box
         * @param min The new minimum point
         */
        void setMin(const glm::vec3& min);

        /**
         * @brief Set the maximum point of the box
         * @param max The new maximum point
         */
        void setMax(const glm::vec3& max);

        /**
         * @brief Get the center point of the box
         * @return The center point
         */
        glm::vec3 getCenter() const;

        /**
         * @brief Get the half-extents of the box
         * @return Half-sizes in each dimension
         */
        glm::vec3 getExtents() const;

        /**
         * @brief Get the full size of the box
         * @return Full sizes in each dimension
         */
        glm::vec3 getSize() const;

        /**
         * @brief Get the volume of the box
         * @return The volume
         */
        float getVolume() const;

        /**
         * @brief Get the surface area of the box
         * @return The surface area
         */
        float getSurfaceArea() const;

        /**
         * @brief Get the length of the longest axis
         * @return The length of the longest axis
         */
        float getLongestAxis() const;

        /**
         * @brief Get the index of the longest axis (0=X, 1=Y, 2=Z)
         * @return The index of the longest axis
         */
        int getLongestAxisIndex() const;

        /**
         * @brief Get the length of the shortest axis
         * @return The length of the shortest axis
         */
        float getShortestAxis() const;

        /**
         * @brief Get the index of the shortest axis (0=X, 1=Y, 2=Z)
         * @return The index of the shortest axis
         */
        int getShortestAxisIndex() const;

        /**
         * @brief Get a specific corner of the box
         * @param index Corner index (0-7, binary pattern corresponds to (x,y,z))
         * @return The corner point
         */
        glm::vec3 getCorner(int index) const;

        /**
         * @brief Get all corners of the box
         * @return Array of all 8 corner points
         */
        std::array<glm::vec3, 8> getAllCorners() const;

        /**
         * @brief Expand the box in all directions by a fixed amount
         * @param amount The distance to expand in each direction
         */
        void expand(float amount);

        /**
         * @brief Expand the box by different amounts in each direction
         * @param amount The distances to expand in each direction
         */
        void expand(const glm::vec3& amount);

        /**
         * @brief Move the box by a vector
         * @param delta The translation vector
         */
        void translate(const glm::vec3& delta);

        /**
         * @brief Apply a transformation to the box
         * @param transform The transformation matrix
         */
        void transform(const glm::mat4& transform);

        /**
         * @brief Create a new AABB that contains both this box and another
         * @param other The other AABB to merge with
         * @return New AABB containing both boxes
         */
        AABB merge(const AABB& other) const;

        /**
         * @brief Expand the box to include a point
         * @param point The point to include
         */
        void include(const glm::vec3& point);

        /**
         * @brief Expand the box to include another AABB
         * @param other The AABB to include
         */
        void include(const AABB& other);

        /**
         * @brief Check if the box is empty (zero volume)
         * @return True if the box is empty
         */
        bool isEmpty() const;

        /**
         * @brief Check if the box is valid (min <= max in all dimensions)
         * @return True if the box is valid
         */
        bool isValid() const;

        /**
         * @brief Check if the box contains a point
         * @param point The point to test
         * @return True if the point is inside or on the box
         */
        bool contains(const glm::vec3& point) const;

        /**
         * @brief Check if the box fully contains another AABB
         * @param other The AABB to test
         * @return True if the other box is fully contained
         */
        bool contains(const AABB& other) const;

        /**
         * @brief Check if the box intersects with another AABB
         * @param other The AABB to test
         * @return True if the boxes intersect
         */
        bool intersects(const AABB& other) const;

        /**
         * @brief Check if the box intersects with a ray and find intersection distances
         * @param ray The ray to test
         * @param tMin Output parameter for near intersection distance
         * @param tMax Output parameter for far intersection distance
         * @return True if the ray intersects the box
         */
        bool intersects(const Ray& ray, float& tMin, float& tMax) const;

        /**
         * @brief Check if the box intersects with a frustum
         * @param frustum The frustum to test
         * @return True if the box intersects the frustum
         */
        bool intersects(const Frustum& frustum) const;

        /**
         * @brief Check if the box intersects with a sphere
         * @param center The center point of the sphere
         * @param radius The radius of the sphere
         * @return True if the box intersects the sphere
         */
        bool intersects(const glm::vec3& center, float radius) const;

        /**
         * @brief Get the closest point on the box to a given point
         * @param point The reference point
         * @return The closest point on the box surface or inside
         */
        glm::vec3 getClosestPoint(const glm::vec3& point) const;

        /**
         * @brief Get the squared distance from a point to the box
         * @param point The reference point
         * @return Squared distance (0 if point is inside)
         */
        float getDistanceSquared(const glm::vec3& point) const;

        /**
         * @brief Get the distance from a point to the box
         * @param point The reference point
         * @return Distance (0 if point is inside)
         */
        float getDistance(const glm::vec3& point) const;

        /**
         * @brief Draw the box for debugging purposes
         * @param color The color to use for drawing
         */
        void debugDraw(const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f)) const;

    private:
        glm::vec3 m_min; ///< Minimum point (smallest values in each dimension)
        glm::vec3 m_max; ///< Maximum point (largest values in each dimension)
    };

} // namespace PixelCraft::Utility