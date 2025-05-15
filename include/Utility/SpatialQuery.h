// -------------------------------------------------------------------------
// SpatialQuery.h
// -------------------------------------------------------------------------
#pragma once

#include "Utility/SpatialPartitioning.h"
#include "glm/glm.hpp"

#include <functional>
#include <vector>

namespace PixelCraft::Utility
{

    /**
     * @brief Utility class for advanced spatial queries
     * 
     * This class provides additional query functionality that goes beyond
     * the basic spatial partitioning queries, such as k-nearest neighbors,
     * path queries, and custom predicate filtering.
     */
    class SpatialQuery
    {
    public:
        /**
         * @brief Find the k-nearest objects to a point
         * @param spatialPartitioning The spatial partitioning structure to query
         * @param point The reference point
         * @param k The number of nearest neighbors to find
         * @param maxDistance Maximum search distance (optional)
         * @return Vector of nearest objects sorted by distance
         */
        static std::vector<std::shared_ptr<ISpatialObject>> findKNearest(
            const std::shared_ptr<SpatialPartitioning>& spatialPartitioning,
            const glm::vec3& point,
            size_t k,
            float maxDistance = FLT_MAX);

        /**
         * @brief Find objects along a path
         * @param partialPartitioning The spatial partitioning structure to query
         * @param start The start point
         * @param end The end point
         * @param radius The search radius around the path
         * @return Vector of objects along the path
         */
        static std::vector<std::shared_ptr<ISpatialObject>> findAlongPath(
            const std::shared_ptr<SpatialPartitioning>& spatialPartitioning,
            const glm::vec3& start,
            const glm::vec3& end,
            float radius);

        /**
         * @brief Find objects that match a predicate
         * @param spatialPartitioning The spatial partitioning structure to query
         * @param bounds The bounds to search within
         * @param predicate The predicate function to filter objects
         * @return Vector of objects that match the predicate
         */
        static std::vector<std::shared_ptr<ISpatialObject>> findWithPredicate(
            const std::shared_ptr<SpatialPartitioning>& spatialPartitioning,
            const AABB& bounds,
            const std::function<bool(const std::shared_ptr<ISpatialObject>&)>& predicate);

        /**
         * @brief Find objects of a specific type
         * @tparam T The type of object to find
         * @param spatialPartitioning The spatial partitioning structure to query
         * @param bounds The bounds to search within
         * @return Vector of objects of the specified type
         */
        template<typename T>
        static std::vector<std::shared_ptr<T>> findByType(
            const std::shared_ptr<SpatialPartitioning>& spatialPartitioning,
            const AABB& bounds)
        {

            std::vector<std::shared_ptr<T>> results;

            // Get all objects in the bounds
            auto objects = spatialPartitioning->queryAABB(bounds);

            // Filter by type
            for (auto& obj : objects)
            {
                // Try to cast to the desired type
                auto typed = std::dynamic_pointer_cast<T>(obj);
                if (typed)
                {
                    results.push_back(typed);
                }

                // Special case for SpatialObjectWrapper
                auto wrapper = std::dynamic_pointer_cast<SpatialObjectWrapper<T>>(obj);
                if (wrapper)
                {
                    results.push_back(wrapper->getObject());
                }
            }

            return results;
        }

        /**
         * @brief Find the object closest to a ray
         * @param spatialPartitioning The spatial partitioning structure to query
         * @param ray The ray to query
         * @param maxDistance Maximum search distance along the ray
         * @param outDistance Output parameter for the closest distance
         * @return The closest object, or nullptr if none found
         */
        static std::shared_ptr<ISpatialObject> findClosestToRay(
            const std::shared_ptr<SpatialPartitioning>& spatialPartitioning,
            const Ray& ray,
            float maxDistance,
            float& outDistance);

        /**
         * @brief Find objects visible from a point, considering oclusion
         * @param spatialPartitioning The spatial partitioning structure to query
         * @param point The viewpoint
         * @param direction The view direction
         * @param angle The view angle in degrees
         * @param maxDistance Maximum view distance
         * @return Vector of visible objects
         */
        static std::vector<std::shared_ptr<ISpatialObject>> findVisible(
            const std::shared_ptr<SpatialPartitioning>& spatialPartitioning,
            const glm::vec3& point,
            const glm::vec3& direction,
            float angle,
            float maxDistance);

        /**
         * @brief Find objects in a region over time
         * @param spatialPartitioning The spatial partitioning structure to query
         * @param startBounds The region bounds at start time
         * @param endPoints The region bounds at end time
         * @param steps Number of interpolation steps
         * @return Vector of objects that intersect the region during any step
         */
        static std::vector<std::shared_ptr<ISpatialObject>> findOverTime(
            const std::shared_ptr<SpatialPartitioning>& spatialPartitioning,
            const AABB& startBounds,
            const AABB& endBounds,
            int steps);

        /**
         * @brief Count objects in various subregions of a bound
         * @param spatialPartitioning The spatial partitioning structure to query
         * @param bounds The overall bounds to divide
         * @param divisions Number of divisions along each axis
         * @return 3D grid of object counts in each subregion
         */
        static std::vector<std::vector<std::vector<int>>> countInSubregions(
            const std::shared_ptr<SpatialPartitioning>& spatialPartitioning,
            const AABB& bounds,
            int divisions);

        /**
         * @brief Find the region with the highest object density
         * @param spatialPartitioning The spatial partitioning structure to query
         * @param bounds The overall bounds to search
         * @param divisions Number of divisions along each axis
         * @return The bounds of the highest density region
         */
        static AABB findHighestDensityRegion(
            const std::shared_ptr<SpatialPartitioning>& spatialPartitioning,
            const AABB& bounds,
            int divisions);
    };

} // namespace PixelCraft::Utility