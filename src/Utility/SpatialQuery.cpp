// -------------------------------------------------------------------------
// SpatialQuery.cpp
// -------------------------------------------------------------------------
#include "Utility/SpatialQuery.h"
#include "Utility/Ray.h"
#include "Core/Logger.h"

#include <algorithm>
#include <cmath>
#include <glm/gtx/norm.hpp>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Utility
{

    std::vector<std::shared_ptr<ISpatialObject>> SpatialQuery::findKNearest(
        const std::shared_ptr<SpatialPartitioning>& spatialPartitioning,
        const glm::vec3& point,
        size_t k,
        float maxDistance)
    {

        if (!spatialPartitioning)
        {
            Log::error("SpatialQuery::findKNearest: Invalid spatial partitioning");
            return {};
        }

        // Create a sphere with the maximum search distance
        auto sphereResults = spatialPartitioning->querySphere(point, maxDistance);

        // If we didn't find enough objects, we might need to expand our search
        if (sphereResults.size() < k && maxDistance < FLT_MAX / 2.0f)
        {
            // Try with a larger distance
            float expandedDistance = maxDistance * 2.0f;
            sphereResults = spatialPartitioning->querySphere(point, expandedDistance);
        }

        // Sort objects by distance to the reference point
        std::sort(sphereResults.begin(), sphereResults.end(),
                  [&point](const std::shared_ptr<ISpatialObject>& a, const std::shared_ptr<ISpatialObject>& b) {
                      float distA = length2(a->getBounds().getCenter() - point);
                      float distB = length2(b->getBounds().getCenter() - point);
                      return distA < distB;
                  }
        );

        // Return up to k objects
        if (sphereResults.size() > k)
        {
            sphereResults.resize(k);
        }

        return sphereResults;
    }

    std::vector<std::shared_ptr<ISpatialObject>> SpatialQuery::findAlongPath(
        const std::shared_ptr<SpatialPartitioning>& spatialPartitioning,
        const glm::vec3& start,
        const glm::vec3& end,
        float radius)
    {

        if (!spatialPartitioning)
        {
            Log::error("SpatialQuery::findAlongPath: Invalid spatial partitioning");
            return {};
        }

        // Calculate path direction and length
        glm::vec3 pathVector = end - start;
        float pathLength = glm::length(pathVector);

        if (pathLength < 0.001f)
        {
            // Start and end are essentially the same point, just do a sphere query
            return spatialPartitioning->querySphere(start, radius);
        }

        // Normalize direction
        glm::vec3 pathDirection = pathVector / pathLength;

        // Create a ray for the path
        Ray pathRay(start, pathDirection);

        // Get objects along the ray with some extra margin
        auto rayResults = spatialPartitioning->queryRay(pathRay, pathLength + radius);

        // Now filter to only include objects within the radius of the path
        std::vector<std::shared_ptr<ISpatialObject>> results;

        for (auto& obj : rayResults)
        {
            // Get the object's center
            glm::vec3 objCenter = obj->getBounds().getCenter();

            // Project onto the path
            float projectionT = glm::dot(objCenter - start, pathDirection);

            // Clamp to path length
            projectionT = std::clamp(projectionT, 0.0f, pathLength);

            // Get the closest point on the path
            glm::vec3 closestPointOnPath = start + pathDirection * projectionT;

            // Check if the object is within the radius
            float distanceToPath = glm::length(objCenter - closestPointOnPath);

            if (distanceToPath <= radius)
            {
                results.push_back(obj);
            }
        }

        return results;
    }

    std::vector<std::shared_ptr<ISpatialObject>> SpatialQuery::findWithPredicate(
        const std::shared_ptr<SpatialPartitioning>& spatialPartitioning,
        const AABB& bounds,
        const std::function<bool(const std::shared_ptr<ISpatialObject>&)>& predicate)
    {

        if (!spatialPartitioning)
        {
            Log::error("SpatialQuery::findWithPredicate: Invalid spatial partitioning");
            return {};
        }

        // Query objects in the bounds
        auto queryResults = spatialPartitioning->queryAABB(bounds);

        // Filter using the predicate
        std::vector<std::shared_ptr<ISpatialObject>> results;

        for (auto& obj : queryResults)
        {
            if (predicate(obj))
            {
                results.push_back(obj);
            }
        }

        return results;
    }

    std::shared_ptr<ISpatialObject> SpatialQuery::findClosestToRay(
        const std::shared_ptr<SpatialPartitioning>& spatialPartitioning,
        const Ray& ray,
        float maxDistance,
        float& outDistance)
    {

        if (!spatialPartitioning)
        {
            Log::error("SpatialQuery::findClosestToRay: Invalid spatial partitioning");
            outDistance = FLT_MAX;
            return nullptr;
        }

        // Query objects along the ray
        auto rayResults = spatialPartitioning->queryRay(ray, maxDistance);

        // Find the closest object
        std::shared_ptr<ISpatialObject> closestObject = nullptr;
        float closestDistance = FLT_MAX;

        for (auto& obj : rayResults)
        {
            // Get the object's bounds
            AABB bounds = obj->getBounds();

            // Perform a more precise ray-AABB intersection test
            float t;
            if (ray.intersectAABB(bounds, closestDistance, t))
            {
                closestObject = obj;
            }
        }

        outDistance = closestDistance;
        return closestObject;
    }

    std::vector<std::shared_ptr<ISpatialObject>> SpatialQuery::findVisible(
        const std::shared_ptr<SpatialPartitioning>& spatialPartitioning,
        const glm::vec3& point,
        const glm::vec3& direction,
        float angle,
        float maxDistance)
    {

        if (!spatialPartitioning)
        {
            Log::error("SpatialQuery::findVisible: Invalid spatial partitioning");
            return {};
        }

        // Create a frustum for the view
        // This is a simplified version that approximates the frustum with a sphere
        float halfAngleRad = angle * 0.5f * (3.14159f / 180.0f);
        float radius = maxDistance * std::sin(halfAngleRad);
        glm::vec3 coneEndPoint = point + direction * maxDistance;

        // First query objects in the sphere that encompasses the view cone
        auto sphereResults = spatialPartitioning->querySphere(
            point + direction * (maxDistance * 0.5f),
            maxDistance * 0.5f + radius
        );

        // Filter to only include objects within the view angle and not occluded
        std::vector<std::shared_ptr<ISpatialObject>> results;

        for (auto& obj : sphereResults)
        {
            // Get the object's center
            glm::vec3 objCenter = obj->getBounds().getCenter();
            glm::vec3 toObject = objCenter - point;
            float distanceToObject = glm::length(toObject);

            // Skip if beyond max distance
            if (distanceToObject > maxDistance)
            {
                continue;
            }

            // Check if within view angle
            if (distanceToObject > 0.001f)
            {
                glm::vec3 toObjectDir = toObject / distanceToObject;
                float cosAngle = glm::dot(direction, toObjectDir);
                float viewCosAngle = glm::cos(halfAngleRad);

                if (cosAngle < viewCosAngle)
                {
                    // Object is outside view angle
                    continue;
                }
            }

            // Check for occlusion using a ray cast
            Ray ray(point, toObject / distanceToObject);
            float closestDist;
            auto closestObj = findClosestToRay(spatialPartitioning, ray, distanceToObject * 0.99f, closestDist);

            if (!closestObj || closestObj->getID() == obj->getID())
            {
                // Object is not occluded or is the closest object
                results.push_back(obj);
            }
        }

        return results;
    }

    std::vector<std::shared_ptr<ISpatialObject>> SpatialQuery::findOverTime(
        const std::shared_ptr<SpatialPartitioning>& spatialPartitioning,
        const AABB& startBounds,
        const AABB& endBounds,
        int steps)
    {

        if (!spatialPartitioning)
        {
            Log::error("SpatialQuery::findOverTime: Invalid spatial partitioning");
            return {};
        }

        if (steps <= 0)
        {
            Log::error("SpatialQuery::findOverTime: Invalid step count");
            return {};
        }

        // Store all unique objects found
        std::unordered_map<uint64_t, std::shared_ptr<ISpatialObject>> objectMap;

        // Query at each interpolated step
        for (int i = 0; i <= steps; i++)
        {
            float t = static_cast<float>(i) / static_cast<float>(steps);

            // Interpolate bounds
            glm::vec3 minPoint = startBounds.getMin() * (1.0f - t) + endBounds.getMin() * t;
            glm::vec3 maxPoint = startBounds.getMax() * (1.0f - t) + endBounds.getMax() * t;

            AABB interpolatedBounds(minPoint, maxPoint);

            // Query objects at this step
            auto stepResults = spatialPartitioning->queryAABB(interpolatedBounds);

            // Add to the map
            for (auto& obj : stepResults)
            {
                objectMap[obj->getID()] = obj;
            }
        }

        // Convert map to vector
        std::vector<std::shared_ptr<ISpatialObject>> results;
        results.reserve(objectMap.size());

        for (auto& [id, obj] : objectMap)
        {
            results.push_back(obj);
        }

        return results;
    }

    std::vector<std::vector<std::vector<int>>> SpatialQuery::countInSubregions(
        const std::shared_ptr<SpatialPartitioning>& spatialPartitioning,
        const AABB& bounds,
        int divisions)
    {

        if (!spatialPartitioning)
        {
            Log::error("SpatialQuery::countInSubregions: Invalid spatial partitioning");
            return {};
        }

        if (divisions <= 0)
        {
            Log::error("SpatialQuery::countInSubregions: Invalid division count");
            return {};
        }

        // Initialize the 3D grid of counts
        std::vector<std::vector<std::vector<int>>> counts(divisions,
                                                          std::vector<std::vector<int>>(divisions,
                                                                                        std::vector<int>(divisions, 0)
                                                          )
        );

        // Calculate the size of each subregion
        glm::vec3 boundsSize = bounds.getSize();
        glm::vec3 subregionSize = boundsSize / static_cast<float>(divisions);

        // Query all objects in the overall bounds
        auto allObjects = spatialPartitioning->queryAABB(bounds);

        // Distribute objects to subregions
        for (auto& obj : allObjects)
        {
            // Get object bounds
            AABB objBounds = obj->getBounds();

            // Find which subregions the object intersects
            glm::vec3 minSubregion = (objBounds.getMin() - bounds.getMin()) / subregionSize;
            glm::vec3 maxSubregion = (objBounds.getMax() - bounds.getMin()) / subregionSize;

            // Clamp to valid subregion indices
            int minX = glm::max(0, glm::min(divisions - 1, static_cast<int>(minSubregion.x)));
            int minY = glm::max(0, glm::min(divisions - 1, static_cast<int>(minSubregion.y)));
            int minZ = glm::max(0, glm::min(divisions - 1, static_cast<int>(minSubregion.z)));

            int maxX = glm::max(0, glm::min(divisions - 1, static_cast<int>(maxSubregion.x)));
            int maxY = glm::max(0, glm::min(divisions - 1, static_cast<int>(maxSubregion.y)));
            int maxZ = glm::max(0, glm::min(divisions - 1, static_cast<int>(maxSubregion.z)));

            // Increment counts for all intersected subregions
            for (int x = minX; x <= maxX; x++)
            {
                for (int y = minY; y <= maxY; y++)
                {
                    for (int z = minZ; z <= maxZ; z++)
                    {
                        counts[x][y][z]++;
                    }
                }
            }
        }

        return counts;
    }

    AABB SpatialQuery::findHighestDensityRegion(
        const std::shared_ptr<SpatialPartitioning>& spatialPartitioning,
        const AABB& bounds,
        int divisions)
    {

        if (!spatialPartitioning)
        {
            Log::error("SpatialQuery::findHighestDensityRegion: Invalid spatial partitioning");
            return bounds;
        }

        // Get the count in each subregion
        auto counts = countInSubregions(spatialPartitioning, bounds, divisions);

        // Find the subregion with the highest count
        int maxCount = 0;
        int maxX = 0, maxY = 0, maxZ = 0;

        for (int x = 0; x < divisions; x++)
        {
            for (int y = 0; y < divisions; y++)
            {
                for (int z = 0; z < divisions; z++)
                {
                    if (counts[x][y][z] > maxCount)
                    {
                        maxCount = counts[x][y][z];
                        maxX = x;
                        maxY = y;
                        maxZ = z;
                    }
                }
            }
        }

        // Calculate the bounds of the highest density region
        glm::vec3 boundsSize = bounds.getSize();
        glm::vec3 subregionSize = boundsSize / static_cast<float>(divisions);

        glm::vec3 minPoint = bounds.getMin() + glm::vec3(
            maxX * subregionSize.x,
            maxY * subregionSize.y,
            maxZ * subregionSize.z
        );

        glm::vec3 maxPoint = minPoint + subregionSize;

        return AABB(minPoint, maxPoint);
    }

} // namespace PixelCraft::Utility