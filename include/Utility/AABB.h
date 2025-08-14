// -------------------------------------------------------------------------
// AABB.h
// -------------------------------------------------------------------------
#pragma once

#include "Utility/Ray.h"
#include "Utility/Frustum.h"
#include "Utility/Color.h"
#include "Utility/DebugDraw.h"
#include "Utility/Serialization/SerializationUtility.h"
#include "Core/Logger.h"

#include <vector>
#include <array>
#include <glm/glm.hpp>

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
         * @brief Default constructor, creates an invalid AABB
         */
        AABB()
            : m_min(glm::vec3(std::numeric_limits<float>::max()))
            , m_max(glm::vec3(-std::numeric_limits<float>::max()))
        {
        }

        /**
         * @brief Construct from min and max points
         * @param min The minimum point of the AABB (smallest values in each dimension)
         * @param max The maximum point of the AABB (largest values in each dimension)
         */
        AABB(const glm::vec3& min, const glm::vec3& max)
            : m_min(min)
            , m_max(max)
        {
        }

        /**
         * @brief Construct from center and half extents
         * @param center The center point of the AABB
         * @param halfExtentX The half-size in X dimension
         * @param halfExtentY The half-size in Y dimension
         * @param halfExtentZ The half-size in Z dimension
         */
        AABB(const glm::vec3& center, float halfExtentX, float halfExtentY, float halfExtentZ)
            : m_min(center.x - halfExtentX, center.y - halfExtentY, center.z - halfExtentZ)
            , m_max(center.x + halfExtentX, center.y + halfExtentY, center.z + halfExtentZ)
        {
        }

        /**
         * @brief Create an AABB that contains all given points
         * @param points A collection of points to enclose
         * @return AABB containing all points
         */
        static AABB createFromPoints(const std::vector<glm::vec3>& points)
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

        /**
         * @brief Create an AABB that contains a sphere
         * @param center The center of the sphere
         * @param radius The radius of the sphere
         * @return AABB containing the sphere
         */
        static AABB createFromSphere(const glm::vec3& center, float radius)
        {
            glm::vec3 extent(radius, radius, radius);
            return AABB(center - extent, center + extent);
        }

        /**
         * @brief Create an AABB that contains a transformed AABB
         * @param aabb The source AABB
         * @param matrix The transformation matrix to apply
         * @return New AABB containing the transformed source AABB
         */
        static AABB createFromTransformedAABB(const AABB& aabb, const glm::mat4& matrix)
        {
            // Get the 8 corners of the source AABB
            std::array<glm::vec3, 8> corners = aabb.getAllCorners();

            // Transform each corner
            for (auto& corner : corners)
            {
                glm::vec4 transformedCorner = matrix * glm::vec4(corner, 1.0f);
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

        /**
         * @brief Get the minimum point of the AABB
         * @return The minimum point (smallest values in each dimension)
         */
        const glm::vec3& getMin() const
        {
            return m_min;
        }

        /**
         * @brief Get the maximum point of the AABB
         * @return The maximum point (largest values in each dimension)
         */
        const glm::vec3& getMax() const
        {
            return m_max;
        }

        /**
         * @brief Set the minimum point of the AABB
         * @param min The new minimum point
         */
        void setMin(const glm::vec3& min)
        {
            m_min = min;
        }

        /**
         * @brief Set the maximum point of the AABB
         * @param max The new maximum point
         */
        void setMax(const glm::vec3& max)
        {
            m_max = max;
        }

        /**
         * @brief Get the center point of the AABB
         * @return The center point
         */
        glm::vec3 getCenter() const
        {
            return (m_min + m_max) * 0.5f;
        }

        /**
         * @brief Get the half-extents of the AABB
         * @return Half-sizes in each dimension
         */
        glm::vec3 getExtents() const
        {
            return (m_max - m_min) * 0.5f;
        }

        /**
         * @brief Get the full size of the AABB
         * @return Full sizes in each dimension
         */
        glm::vec3 getSize() const
        {
            return m_max - m_min;
        }

        /**
         * @brief Get the volume of the AABB
         * @return The volume
         */
        float getVolume() const
        {
            glm::vec3 size = getSize();
            return size.x * size.y * size.z;
        }

        /**
         * @brief Get the surface area of the AABB
         * @return The surface area
         */
        float getSurfaceArea() const
        {
            glm::vec3 size = getSize();
            return 2.0f * (size.x * size.y + size.x * size.z + size.y * size.z);
        }

        /**
         * @brief Get the length of the longest axis
         * @return The length of the longest axis
         */
        float getLongestAxis() const
        {
            glm::vec3 size = getSize();
            return std::max(std::max(size.x, size.y), size.z);
        }

        /**
         * @brief Get the index of the longest axis (0=X, 1=Y, 2=Z)
         * @return The index of the longest axis
         */
        int getLongestAxisIndex() const
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

        /**
         * @brief Get the length of the shortest axis
         * @return The length of the shortest axis
         */
        float getShortestAxis() const
        {
            glm::vec3 size = getSize();
            return std::min(std::min(size.x, size.y), size.z);
        }

        /**
         * @brief Get the index of the shortest axis (0=X, 1=Y, 2=Z)
         * @return The index of the shortest axis
         */
        int getShortestAxisIndex() const
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

        /**
         * @brief Get a specific corner of the AABB
         * @param index Corner index (0-7, binary pattern corresponds to (x,y,z))
         * @return The corner point
         */
        glm::vec3 getCorner(int index) const
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

        /**
         * @brief Get all corners of the AABB
         * @return Array of all 8 corner points
         */
        std::array<glm::vec3, 8> getAllCorners() const
        {
            std::array<glm::vec3, 8> corners;
            for (int i = 0; i < 8; ++i)
            {
                corners[i] = getCorner(i);
            }
            return corners;
        }

        /**
         * @brief Expand the AABB in all directions by a fixed amount
         * @param amount The distance to expand in each direction
         */
        void expand(float amount)
        {
            m_min -= glm::vec3(amount);
            m_max += glm::vec3(amount);
        }

        /**
         * @brief Expand the AABB by different amounts in each direction
         * @param amount The distances to expand in each direction
         */
        void expand(const glm::vec3& amount)
        {
            m_min -= amount;
            m_max += amount;
        }

        /**
         * @brief Translate the AABB by an offset
         * @param offset The offset to translate by
         */
        void translate(const glm::vec3& offset)
        {
            m_min += offset;
            m_max += offset;
        }

        /**
         * @brief Apply a transformation to the AABB
         * @param matrix The transformation matrix
         */
        void transform(const glm::mat4& matrix)
        {
            *this = createFromTransformedAABB(*this, matrix);
        }

        /**
         * @brief Create a new AABB that contains both this AABB and another
         * @param other The other AABB to merge with
         * @return New AABB containing both AABBs
         */
        void merge(const AABB& other)
        {
            m_min = glm::min(m_min, other.m_min);
            m_max = glm::max(m_max, other.m_max);
        }

        /**
         * @brief Expand the AABB to include a point
         * @param point The point to include
         */
        void include(const glm::vec3& point)
        {
            m_min = glm::min(m_min, point);
            m_max = glm::max(m_max, point);
        }

        /**
         * @brief Expand the AABB to include another AABB
         * @param other The AABB to include
         */
        void include(const AABB& other)
        {
            if (other.isValid())
            {
                include(other.m_min);
                include(other.m_max);
            }
        }

        /**
         * @brief Check if the AABB is empty (zero volume)
         * @return True if the AABB is empty
         */
        bool isEmpty() const
        {
            return m_min.x > m_max.x || m_min.y > m_max.y || m_min.z > m_max.z;
        }

        /**
         * @brief Check if the AABB is valid (min <= max in all dimensions)
         * @return True if the AABB is valid
         */
        bool isValid() const
        {
            return m_min.x <= m_max.x && m_min.y <= m_max.y && m_min.z <= m_max.z;
        }

        /**
         * @brief Check if the AABB contains a point
         * @param point The point to test
         * @return True if the point is inside or on the AABB
         */
        bool contains(const glm::vec3& point) const
        {
            return point.x >= m_min.x && point.x <= m_max.x &&
                point.y >= m_min.y && point.y <= m_max.y &&
                point.z >= m_min.z && point.z <= m_max.z;
        }

        /**
         * @brief Check if the AABB fully contains another AABB
         * @param other The AABB to test
         * @return True if the other AABB is fully contained
         */
        bool contains(const AABB& other) const
        {
            return other.m_min.x >= m_min.x && other.m_max.x <= m_max.x &&
                other.m_min.y >= m_min.y && other.m_max.y <= m_max.y &&
                other.m_min.z >= m_min.z && other.m_max.z <= m_max.z;
        }

        /**
         * @brief Check if the AABB intersects with another AABB
         * @param other The AABB to test
         * @return True if the AABBs intersect
         */
        bool intersects(const AABB& other) const
        {
            return m_min.x <= other.m_max.x && m_max.x >= other.m_min.x &&
                m_min.y <= other.m_max.y && m_max.y >= other.m_min.y &&
                m_min.z <= other.m_max.z && m_max.z >= other.m_min.z;
        }

        /**
         * @brief Check if the AABB intersects with a ray and find intersection distances
         * @param ray The ray to test
         * @param tMin Output parameter for near intersection distance
         * @param tMax Output parameter for far intersection distance
         * @return True if the ray intersects the AABB
         */
        bool intersects(const Ray& ray, float& tMin, float& tMax) const
        {
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

        /**
         * @brief Check if the AABB intersects with a frustum
         * @param frustum The frustum to test
         * @return True if the AABB intersects the frustum
         */
        bool intersects(const Frustum& frustum) const
        {
            auto corners = getAllCorners();
            for (const auto& corner : corners) {
                if (frustum.testPoint(corner)) {
                    return true;
                }
            }

            return false;
        }

        /**
         * @brief Check if the AABB intersects with a sphere
         * @param center The center point of the sphere
         * @param radius The radius of the sphere
         * @return True if the AABB intersects the sphere
         */
        bool intersects(const glm::vec3& center, float radius) const
        {
            // Get closest point on AABB to sphere center
            glm::vec3 closestPoint = getClosestPoint(center);

            // Check if squared distance from closest point to sphere center is <= radius�
            float distanceSquared = glm::dot(closestPoint - center, closestPoint - center);
            return distanceSquared <= radius * radius;
        }

        /**
         * @brief Get the closest point on the AABB to a given point
         * @param point The reference point
         * @return The closest point on the AABB surface or inside
         */
        glm::vec3 getClosestPoint(const glm::vec3& point) const
        {
            return glm::clamp(point, m_min, m_max);
        }

        /**
         * @brief Get the squared distance from a point to the AABB
         * @param point The reference point
         * @return Squared distance (0 if point is inside)
         */
        float getDistanceSquared(const glm::vec3& point) const
        {
            glm::vec3 closest = getClosestPoint(point);
            glm::vec3 diff = point - closest;
            return glm::dot(diff, diff);
        }

        /**
         * @brief Get the distance from a point to the AABB
         * @param point The reference point
         * @return Distance (0 if point is inside)
         */
        float getDistance(const glm::vec3& point) const
        {
            return glm::sqrt(getDistanceSquared(point));
        }

        /**
         * @brief Draw the AABB for debugging purposes
         * @param color The color to use for drawing
         */
        void debugDraw(const Color& color = Color(1.0f, 1.0f, 1.0f)) const
        {
            if (!isValid())
            {
                return;
            }

            // Get all 8 corners of the AABB
            auto corners = getAllCorners();

            // Draw the 12 edges of the AABB
            // Assumes DebugDraw has a drawLine(start, end, color) method

            // Bottom face
            Utility::drawLine(corners[0], corners[1], color);
            Utility::drawLine(corners[1], corners[3], color);
            Utility::drawLine(corners[3], corners[2], color);
            Utility::drawLine(corners[2], corners[0], color);

            // Top face
            Utility::drawLine(corners[4], corners[5], color);
            Utility::drawLine(corners[5], corners[7], color);
            Utility::drawLine(corners[7], corners[6], color);
            Utility::drawLine(corners[6], corners[4], color);

            // Connecting edges
            Utility::drawLine(corners[0], corners[4], color);
            Utility::drawLine(corners[1], corners[5], color);
            Utility::drawLine(corners[2], corners[6], color);
            Utility::drawLine(corners[3], corners[7], color);
        }

        /**
         * @brief Serialize the biome configuration to the given serializer
         * @param serializer The serializer to use
         * @return SerializationResult with success/failure status
         */
        Utility::Serialization::SerializationResult serialize(Utility::Serialization::Serializer& serializer) const
        {
            // Begin serializing the AABB
            auto result = serializer.beginObject(getTypeName());
            if (!result) return result;

            result = serializer.writeField("min", m_min);
            if (!result) return result;

            result = serializer.writeField("max", m_max);
            if (!result) return result;

            return serializer.endObject();
        }

        /**
         * @brief Deserialize the biome configuration from the given deserializer
         * @param deserializer The deserializer to use
         * @return SerializationResult with success/failure status
         */
        Utility::Serialization::SerializationResult deserialize(Utility::Serialization::Deserializer& deserializer)
        {
            // Begin deserializing the AABB
            auto result = deserializer.beginObject(getTypeName());
            if (!result) return result;

            result = deserializer.readField("min", m_min);
            if (!result) return result;

            result = deserializer.readField("max", m_max);
            if (!result) return result;

            return deserializer.endObject();
        }

        /**
         * @brief Define the schema for biome info serialization
         * @param schema The schema to define
         */
        static void defineSchema(Utility::Serialization::Schema& schema)
        {
            schema.addField("min", Utility::Serialization::ValueType::Vec3, true);
            schema.addField("max", Utility::Serialization::ValueType::Vec3, true);
        }

        /**
         * @brief Get the type name for serialization
         * @return The type name string
         */
        static std::string getTypeName()
        {
            return "AABB";
        }

    private:
        glm::vec3 m_min; ///< Minimum corner of the box
        glm::vec3 m_max; ///< Maximum corner of the box
    };

} // namespace PixelCraft::Utility