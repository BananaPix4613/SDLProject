// -------------------------------------------------------------------------
// Frustum.h
// -------------------------------------------------------------------------
#pragma once

#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>

namespace PixelCraft::Utility
{

    // Forward declarations
    class AABB;

    /**
     * @brief Enumeration of frustum planes
     */
    enum class FrustumPlane
    {
        Near,
        Far,
        Left,
        Right,
        Top,
        Bottom
    };

    /**
     * @brief Represents a plane in 3D space (ax + by + cz + d = 0)
     */
    class Plane
    {
    public:
        /**
         * @brief Default constructor creates a plane at the origin with normal (0,1,0)
         */
        Plane();

        /**
         * @brief Creates a plane with given normal and distance from origin
         * @param normal The plane normal vector (should be normalized)
         * @param distance Distance from origin along the normal
         */
        Plane(const glm::vec3& normal, float distance);

        /**
         * @brief Creates a plane with given normal that passes through the specified point
         * @param normal The plane normal vector (should be normalized)
         * @param point A point that lies on the plane
         */
        Plane(const glm::vec3& normal, const glm::vec3& point);

        /**
         * @brief Creates a plane with equation ax + by + cz + d = 0
         * @param a The x component of the normal
         * @param b The y component of the normal
         * @param c The z component of the normal
         * @param d The negative distance from origin along the normal
         */
        Plane(float a, float b, float c, float d);

        /**
         * @brief Normalizes the plane equation
         */
        void normalize();

        /**
         * @brief Calculates the signed distance from a point to the plane
         * @param point The point to calculate distance to
         * @return Signed distance (positive if in front of the plane)
         */
        float getSignedDistance(const glm::vec3& point) const;

        /**
         * @brief Get the plane normal
         * @return The plane normal vector (normalized)
         */
        const glm::vec3& getNormal() const;

        /**
         * @brief Get the plane distance from origin
         * @return Distance from the origin along the normal
         */
        float getDistance() const;

        /**
         * @brief Set the plane normal
         * @param normal The new normal vector (should be normalized)
         */
        void setNormal(const glm::vec3& normal);

        /**
         * @brief Set the plane distance from origin
         * @param distance The new distance from origin along the normal
         */
        void setDistance(float distance);

        /**
         * @brief Get the x component of the normal (a coefficient)
         * @return The x component of the normal
         */
        float a() const
        {
            return m_normal.x;
        }

        /**
         * @brief Get the y component of the normal (b coefficient)
         * @return The y component of the normal
         */
        float b() const
        {
            return m_normal.y;
        }

        /**
         * @brief Get the z component of the normal (c coefficient)
         * @return The z component of the normal
         */
        float c() const
        {
            return m_normal.z;
        }

        /**
         * @brief Get the d coefficient (negative distance)
         * @return The d coefficient of the plane equation
         */
        float d() const
        {
            return m_distance;
        }

    private:
        glm::vec3 m_normal;   ///< The plane normal vector
        float m_distance;     ///< Distance from origin along the normal
    };

    /**
     * @brief Intersection test result types
     */
    enum class IntersectionType
    {
        Inside,     ///< Object is fully inside frustum
        Outside,    ///< Object is fully outside frustum
        Intersects  ///< Object intersects frustum boundary
    };

    /**
     * @brief Represents a view frustum for efficient culling operations
     */
    class Frustum
    {
    public:
        /**
         * @brief Default constructor
         */
        Frustum();

        /**
         * @brief Initialize the frustum with default values
         */
        void initialize();

        /**
         * @brief Update the frustum from a view-projection matrix
         * @param viewProjection The combined view-projection matrix
         */
        void update(const glm::mat4& viewProjection);

        /**
         * @brief Extract frustum planes from a matrix
         * @param matrix The view-projection matrix to extract planes from
         */
        void extractPlanes(const glm::mat4& matrix);

        /**
         * @brief Extract frustum corners from an inverse view-projection matrix
         * @param invViewProj The inverse view-projection matrix
         */
        void extractCorners(const glm::mat4& invViewProj);

        /**
         * @brief Get a specific frustum plane
         * @param planeType The plane to retrieve
         * @return Const reference to the requested plane
         */
        const Plane& getPlane(FrustumPlane planeType) const;

        /**
         * @brief Get a specific frustum plane for modification
         * @param planeType The plane to retrieve
         * @return Non-const reference to the requested plane
         */
        Plane& getPlane(FrustumPlane planeType);

        /**
         * @brief Get all frustum planes
         * @return Const reference to array of all planes
         */
        const std::array<Plane, 6>& getPlanes() const;

        /**
         * @brief Get all frustum corners
         * @return Const reference to array of corner points
         */
        const std::array<glm::vec3, 8>& getCorners() const;

        /**
         * @brief Test if a point is inside the frustum
         * @param point The point to test
         * @return True if point is inside or on the frustum
         */
        bool testPoint(const glm::vec3& point) const;

        /**
         * @brief Test if a sphere is inside or intersects the frustum
         * @param center The center of the sphere
         * @param radius The radius of the sphere
         * @return True if sphere is at least partially inside the frustum
         */
        bool testSphere(const glm::vec3& center, float radius) const;

        /**
         * @brief Detailed intersection test for a sphere
         * @param center The center of the sphere
         * @param radius The radius of the sphere
         * @return IntersectionType indicating the sphere's relation to the frustum
         */
        IntersectionType testSphereIntersection(const glm::vec3& center, float radius) const;

        /**
         * @brief Test if an AABB is inside or intersects the frustum
         * @param min The minimum corner of the AABB
         * @param max The maximum corner of the AABB
         * @return True if AABB is at least partially inside the frustum
         */
        bool testAABB(const glm::vec3& min, const glm::vec3& max) const;

        /**
         * @brief Test if an AABB is inside or intersects the frustum
         * @param bounds Reference to the AABB
         * @return True if AABB is at least partially inside the frustum
         */
        bool testAABB(const AABB& bounds) const;

        /**
         * @brief Detailed intersection test for an AABB
         * @param min The minimum corner of the AABB
         * @param max The maximum corner of the AABB
         * @return IntersectionType indicating the AABB's relation to the frustum
         */
        IntersectionType testAABBIntersection(const glm::vec3& min, const glm::vec3& max) const;

        /**
         * @brief Draw debug visualization of the frustum
         * @param color The color to use for debug drawing
         */
        void debugDraw(const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f)) const;

    private:
        std::array<Plane, 6> m_planes;  ///< Frustum planes (Near, Far, Left, Right, Top, Bottom)
        std::array<glm::vec3, 8> m_corners; ///< Frustum corners

        // Corner indices reference:
        // Near face: 0=bottom-left, 1=bottom-right, 2=top-right, 3=top-left
        // Far face: 4=bottom-left, 5=bottom-right, 6=top-right, 7=top-left

        bool m_planesNormalized; ///< Flag indicating if planes are normalized
    };

} // namespace PixelCraft::Utility