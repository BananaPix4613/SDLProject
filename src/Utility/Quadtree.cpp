// -------------------------------------------------------------------------
// Quadtree.cpp
// -------------------------------------------------------------------------
#include "Utility/Quadtree.h"
#include "Core/Application.h"
#include "Utility/DebugDraw.h"
#include "Utility/StringUtils.h"
#include "Utility/Ray.h"
#include "Core/Logger.h"

#include <cmath>
#include <Utility/Color.h>

using StringUtils = PixelCraft::Utility::StringUtils;
namespace Log = PixelCraft::Core;

namespace PixelCraft::Utility
{

    // QuadtreeNode implementation
    QuadtreeNode::QuadtreeNode(const AABB2D &bounds, int depth, QuadtreeNode *parent,
                               int maxDepth, int maxObjects, int minObjects)
        : m_bounds(bounds)
        , m_depth(depth)
        , m_parent(parent)
        , m_maxDepth(maxDepth)
        , m_maxObjects(maxObjects)
        , m_minObjects(minObjects)
    {
        // Initialized with no children
        for (auto& child : m_children)
        {
            child = nullptr;
        }
    }

    QuadtreeNode::~QuadtreeNode()
    {
        // Children are automatically cleaned up by unique_ptr
    }

    bool QuadtreeNode::insert(std::shared_ptr<ISpatialObject2D> object)
    {
        if (!object)
        {
            return false;
        }

        // Check if the object intersects this node
        if (!m_bounds.intersects(object->getBounds2D()))
        {
            return false;
        }

        // If this is a leaf node and it hasn't reached capacity, add the object here
        if (isLeaf())
        {
            if (static_cast<int>(m_objects.size()) < m_maxObjects || m_depth >= m_maxDepth)
            {
                m_objects.push_back(object);
                return true;
            }

            // Otherwise, we need to split and redistribute
            split();
        }

        // Try to insert into children
        bool inserted = false;
        for (auto& child : m_children)
        {
            if (child && child->insert(object))
            {
                inserted = true;
                break;
            }
        }

        // If no child could contain it, add to this node
        if (!inserted)
        {
            m_objects.push_back(object);
        }

        return true;
    }

    bool QuadtreeNode::remove(uint64_t objectID)
    {
        // Remove from this node's objects
        for (auto it = m_objects.begin(); it != m_objects.end(); it++)
        {
            if ((*it)->getID() == objectID)
            {
                m_objects.erase(it);

                // Check if we should merge
                if (!isLeaf())
                {
                    size_t totalObjects = getObjectCount();
                    if (totalObjects <= static_cast<size_t>(m_minObjects))
                    {
                        merge();
                    }
                }

                return true;
            }
        }

        // Try to remove from children
        if (!isLeaf())
        {
            for (auto& child : m_children)
            {
                if (child && child->remove(objectID))
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool QuadtreeNode::update(std::shared_ptr<ISpatialObject2D> object)
    {
        if (!object)
        {
            return false;
        }

        // Check if the object is still within this node
        if (!m_bounds.intersects(object->getBounds2D()))
        {
            // If not, we need to remove it and reinsert
            if (!remove(object->getID()))
            {
                return false;
            }

            // Find the nearest ancestor that contains the object
            QuadtreeNode* node = m_parent;
            while (node)
            {
                if (node->m_bounds.intersects(object->getBounds2D()))
                {
                    return node->insert(object);
                }
                node = node->getParent();
            }

            return false;
        }

        // Check if this object is in this node's list
        for (auto it = m_objects.begin(); it != m_objects.end(); it++)
        {
            if ((*it)->getID() == object->getID())
            {
                // Update the object
                *it = object;
                return true;
            }
        }

        // Check children
        if (!isLeaf())
        {
            for (auto& child : m_children)
            {
                if (child && child->update(object))
                {
                    return true;
                }
            }
        }

        // If we reach here, the object wasn't found in this node or its children
        return false;
    }

    void QuadtreeNode::queryAABB(const AABB2D& bounds, std::vector<std::shared_ptr<ISpatialObject2D>>& results) const
    {
        // Check if this node intersects the query bounds
        if (!m_bounds.intersects(bounds))
        {
            return;
        }

        // Add objects from this node that intersect the query bounds
        for (const auto& object : m_objects)
        {
            if (object->getBounds2D().intersects(bounds))
            {
                results.push_back(object);
            }
        }

        // Recursively check children
        if (!isLeaf())
        {
            for (const auto& child : m_children)
            {
                if (child)
                {
                    child->queryAABB(bounds, results);
                }
            }
        }
    }

    void QuadtreeNode::queryCircle(const glm::vec2 &center, float radius, std::vector<std::shared_ptr<ISpatialObject2D>> &results) const
    {
        // Create a bounding box for the circle
        AABB2D circleAABB(
            center - glm::vec2(radius),
            center + glm::vec2(radius)
        );

        // Check if this node intersects the circle's AABB
        if (!m_bounds.intersects(circleAABB))
        {
            return;
        }

        // Add objects from this node that intersect the circle
        for (const auto& object : m_objects)
        {
            // First do a quick AABB test
            AABB2D objectBounds = object->getBounds2D();
            if (objectBounds.intersects(circleAABB))
            {
                // Then do a more precise circle test
                // We can approximate this by checking if the closest point on the AABB to the circle center
                // is within the radius
                glm::vec2 objectMin = objectBounds.getMin();
                glm::vec2 objectMax = objectBounds.getMax();

                // Compute closest point on AABB to circle center
                glm::vec2 closestPoint;
                closestPoint.x = std::max(objectMin.x, std::min(center.x, objectMax.x));
                closestPoint.y = std::max(objectMin.y, std::min(center.y, objectMax.y));

                // Check if the closest point is within the circle
                float distanceSquared = length2(closestPoint - center);
                if (distanceSquared <= radius * radius)
                {
                    results.push_back(object);
                }
            }
        }

        // Recursively check children
        if (!isLeaf())
        {
            for (const auto& child : m_children)
            {
                if (child)
                {
                    child->queryCircle(center, radius, results);
                }
            }
        }
    }

    void QuadtreeNode::queryRay(const glm::vec2& origin, const glm::vec2& direction, float maxDistance, std::vector<std::shared_ptr<ISpatialObject2D>>& results) const
    {
        // Check if this node's AABB intersects the ray
        // This is a simplified 2D ray-AABB test
        glm::vec2 nodeMin = m_bounds.getMin();
        glm::vec2 nodeMax = m_bounds.getMax();

        // Calculate inverse direction to avoid division by zero
        glm::vec2 invDir(
            direction.x != 0.0f ? 1.0f / direction.x : std::numeric_limits<float>::max(),
            direction.y != 0.0f ? 1.0f / direction.y : std::numeric_limits<float>::max()
        );

        // Calculate intersections with x and y slabs
        glm::vec2 t0 = (nodeMin - origin) * invDir;
        glm::vec2 t1 = (nodeMax - origin) * invDir;

        // Find the near and far intersections
        glm::vec2 tMin = min(t0, t1);
        glm::vec2 tMax = max(t0, t1);

        float tNear = std::max(tMin.x, tMin.y);
        float tFar = std::min(tMax.x, tMax.y);

        // Check if there is an intersection
        if (tNear > tFar || tFar < 0 || tNear > maxDistance)
        {
            return;
        }

        // Add objects from this node that intersect the ray
        for (const auto& object : m_objects)
        {
            // Perform the same ray-AABB test for the object
            AABB2D objectBounds = object->getBounds2D();
            glm::vec2 objMin = objectBounds.getMin();
            glm::vec2 objMax = objectBounds.getMax();

            glm::vec2 objT0 = (objMin - origin) * invDir;
            glm::vec2 objT1 = (objMax - origin) * invDir;

            glm::vec2 objTMin = min(objT0, objT1);
            glm::vec2 objTMax = max(objT0, objT1);

            float objTNear = std::max(objTMin.x, objTMin.y);
            float objTFar = std::min(objTMax.x, objTMax.y);

            if (objTNear <= objTFar && objTFar >= 0 && objTNear <= maxDistance)
            {
                results.push_back(object);
            }
        }

        // Recursively check children
        if (!isLeaf())
        {
            for (const auto& child : m_children)
            {
                if (child)
                {
                    child->queryRay(origin, direction, maxDistance, results);
                }
            }
        }
    }

    bool QuadtreeNode::isLeaf() const
    {
        // Check if any children are non-null
        for (const auto& child : m_children)
        {
            if (child)
            {
                return false;
            }
        }
        return true;
    }

    void QuadtreeNode::debugDraw(DebugDraw& debugDraw, bool drawObjects, float minY, float maxY) const
    {
        // Convert 2D bounds to 3D for visualization
        AABB aabb = m_bounds.toAABB(minY, maxY);

        // Draw this node's bounding box
        Color color;
        if (isLeaf())
        {
            // Leaf nodes are darker
            color = Color(0.2f, 0.8f, 0.2f, 0.3f);
        }
        else
        {
            // Internal nodes are lighter
            color = Color(0.8f, 0.2f, 0.2f, 0.2f);
        }

        debugDraw.drawBox(aabb, color.rgba());

        // Draw contained objects if requested
        if (drawObjects)
        {
            for (const auto& object : m_objects)
            {
                AABB2D bounds2D = object->getBounds2D();
                AABB objAABB = bounds2D.toAABB(minY, maxY);
                debugDraw.drawBox(objAABB, glm::vec4(0.2f, 0.2f, 0.8f, 0.5f));
            }
        }

        // Recursively draw children
        if (!isLeaf())
        {
            for (const auto& child : m_children)
            {
                if (child)
                {
                    child->debugDraw(debugDraw, drawObjects, minY, maxY);
                }
            }
        }
    }

    QuadtreeNode* QuadtreeNode::getChild(int index) const
    {
        if (index >= 0 && index < 4)
        {
            return m_children[index].get();
        }
        return nullptr;
    }

    size_t QuadtreeNode::getObjectCount() const
    {
        size_t count = m_objects.size();

        if (!isLeaf())
        {
            for (const auto& child : m_children)
            {
                if (child)
                {
                    count += child->getObjectCount();
                }
            }
        }

        return count;
    }

    int QuadtreeNode::getMaxDepth() const
    {
        if (isLeaf())
        {
            return m_depth;
        }

        int maxChildDepth = m_depth;
        for (const auto& child : m_children)
        {
            if (child)
            {
                int childDepth = child->getMaxDepth();
                maxChildDepth = std::max(maxChildDepth, childDepth);
            }
        }

        return maxChildDepth;
    }

    size_t QuadtreeNode::getNodeCount() const
    {
        size_t count = 1; // This node

        if (!isLeaf())
        {
            for (const auto& child : m_children)
            {
                if (child)
                {
                    count += child->getNodeCount();
                }
            }
        }

        return count;
    }

    void QuadtreeNode::split()
    {
        // Don't split if we're at max depth
        if (m_depth >= m_maxDepth)
        {
            return;
        }

        // Don't split if we're already split
        if (!isLeaf())
        {
            return;
        }

        // Create 4 children
        for (int i = 0; i < 4; i++)
        {
            AABB2D childBounds = calculateChildBounds(i);
            m_children[i] = std::make_unique<QuadtreeNode>(
                childBounds,
                m_depth + 1,
                this,
                m_maxDepth,
                m_maxObjects,
                m_minObjects
            );
        }

        // Redistribute objects to children
        std::vector<std::shared_ptr<ISpatialObject2D>> remainingObjects;
        for (auto& object : m_objects)
        {
            // Try to insert into a child
            bool inserted = false;
            for (auto& child : m_children)
            {
                if (child->insert(object))
                {
                    inserted = true;
                    break;
                }
            }

            // If the object couldn't be inserted into any child, keep it in this node
            if (!inserted)
            {
                remainingObjects.push_back(object);
            }
        }

        // Replace objects with those that couldn't be inserted into children
        m_objects = std::move(remainingObjects);
    }

    void QuadtreeNode::merge()
    {
        // Don't merge if we're a leaf
        if (isLeaf())
        {
            return;
        }

        // Collect all objects from children
        for (auto& child : m_children)
        {
            if (child)
            {
                // Recursively merge the child first
                child->merge();

                // Add the child's objects to this node
                m_objects.insert(m_objects.end(), child->m_objects.begin(), child->m_objects.end());

                // Clear the child
                child.reset();
            }
        }
    }

    bool QuadtreeNode::containsObject(uint64_t objectID) const
    {
        // Check if the object is in this node
        for (const auto& object : m_objects)
        {
            if (object->getID() == objectID)
            {
                return true;
            }
        }

        // Check children
        if (!isLeaf())
        {
            for (const auto& child : m_children)
            {
                if (child && child->containsObject(objectID))
                {
                    return true;
                }
            }
        }

        return false;
    }

    int QuadtreeNode::calculateChildIndex(const glm::vec2& position) const
    {
        glm::vec2 center = m_bounds.getCenter();
        int index = 0;
        if (position.x >= center.x) index |= 1;
        if (position.y >= center.y) index |= 2;
        return index;
    }

    AABB2D QuadtreeNode::calculateChildBounds(int index) const
    {
        glm::vec2 min = m_bounds.getMin();
        glm::vec2 max = m_bounds.getMax();
        glm::vec2 center = m_bounds.getCenter();

        // Calculate min and max for the child based on index
        glm::vec2 childMin, childMax;
        childMin.x = (index & 1) ? center.x : min.x;
        childMin.y = (index & 2) ? center.y : min.y;

        childMax.x = (index & 1) ? max.x : center.x;
        childMax.y = (index & 2) ? max.y : center.y;

        return AABB2D(childMin, childMax);
    }

    // Quadtree implementation
    Quadtree::Quadtree(const AABB2D& worldBounds, const SpatialPartitionConfig& config)
        : SpatialPartitioning(config)
        , m_worldBounds2D(worldBounds)
        , m_worldBounds(worldBounds.toAABB(-1000.0f, 1000.0f))
        , m_minY(-1000.0f)
        , m_maxY(1000.0f)
    {
        // Constructor implementation
    }

    Quadtree::~Quadtree()
    {
        // Cleanup handled by shutdown()
    }

    bool Quadtree::initialize()
    {
        if (!SpatialPartitioning::initialize())
        {
            return false;
        }

        // Create the root node
        m_root = std::make_unique<QuadtreeNode>(
            m_worldBounds2D,
            0,              // depth
            nullptr,        // parent
            m_config.maxDepth,
            m_config.maxObjectsPerNode,
            m_config.minObjectsPerNode
        );

        Log::info(StringUtils::format("Quadtree initialized with world bounds: ({}, {}) to ({}, {})",
                                      m_worldBounds2D.getMin().x, m_worldBounds2D.getMin().y,
                                      m_worldBounds2D.getMax().x, m_worldBounds2D.getMax().y));

        return true;
    }

    void Quadtree::update(float deltaTime)
    {
        SpatialPartitioning::update(deltaTime);

        // If dynamic updates are enabled, we could perform periodic restructuring here
        if (!m_initialized || !m_config.dynamicTree)
        {
            return;
        }

        // Implementation for periodic restructuring or optimization
    }

    void Quadtree::render()
    {
        if (!m_initialized || !m_debugDrawEnabled)
        {
            return;
        }

        // Get the debug drawing utility
        auto& app = Core::Application::getInstance();

        auto debugDraw = app.getSubsystem<DebugDraw>();
        if (!debugDraw)
        {
            return;
        }

        // Draw the quadtree
        if (m_root)
        {
            m_root->debugDraw(*debugDraw, true, m_minY, m_maxY);
        }
    }

    void Quadtree::shutdown()
    {
        SpatialPartitioning::shutdown();

        // Clear all objects and the tree
        clear();
    }

    std::vector<std::string> Quadtree::getDependencies() const
    {
        return {
            "DebugDraw" // Only needed for visualization
        };
    }

    uint64_t Quadtree::insert(std::shared_ptr<ISpatialObject> object)
    {
        if (!m_initialized || !object)
        {
            return 0;
        }

        // Create a 2D wrapper for the 3D object
        auto wrapper = std::make_shared<SpatialObject2DWrapper>(object);

        // Check if the object is within the world bounds
        if (!m_worldBounds2D.intersects(wrapper->getBounds2D()))
        {
            Log::warn("Object outside world bounds, cannot insert: ID " + object->getID());
            return 0;
        }

        // Store the object in our maps
        uint64_t id = object->getID();
        if (id == 0)
        {
            id = generateObjectID();
        }

        // Insert into the tree
        if (m_root && m_root->insert(wrapper))
        {
            m_objectMap[id] = wrapper;
            m_originalObjects[id] = object;
            return id;
        }

        return 0;
    }

    bool Quadtree::remove(uint64_t objectID)
    {
        if (!m_initialized || objectID == 0)
        {
            return false;
        }

        // Look up the object
        auto it = m_objectMap.find(objectID);
        if (it == m_objectMap.end())
        {
            return false;
        }

        // Remove from the tree
        if (m_root && m_root->remove(objectID))
        {
            // Remove from the maps
            m_objectMap.erase(it);
            m_originalObjects.erase(objectID);
            return true;
        }

        return false;
    }

    bool Quadtree::update(uint64_t objectID)
    {
        if (!m_initialized || objectID == 0)
        {
            return false;
        }

        // Look up the original object
        auto origIt = m_originalObjects.find(objectID);
        if (origIt == m_originalObjects.end())
        {
            return false;
        }

        // Create a new wrapper with the updated bounds
        auto wrapper = std::make_shared<SpatialObject2DWrapper>(origIt->second);

        // Update in the tree
        if (m_root && m_root->update(wrapper))
        {
            // Update our map
            m_objectMap[objectID] = wrapper;
            return true;
        }

        return false;
    }

    bool Quadtree::update(uint64_t objectID, const AABB& newBounds)
    {
        if (!m_initialized || objectID == 0)
        {
            return false;
        }

        // Look up the original object
        auto origIt = m_originalObjects.find(objectID);
        if (origIt == m_originalObjects.end())
        {
            return false;
        }

        // Create a temporary wrapper with the new bounds
        class UpdatedObject : public ISpatialObject
        {
        public:
            UpdatedObject(std::shared_ptr<ISpatialObject> original, const AABB& newBounds)
                : m_original(original), m_newBounds(newBounds)
            {
            }

            AABB getBounds() const override
            {
                return m_newBounds;
            }
            uint64_t getID() const override
            {
                return m_original->getID();
            }

        private:
            std::shared_ptr<ISpatialObject> m_original;
            AABB m_newBounds;
        };

        auto updatedObject = std::make_shared<UpdatedObject>(origIt->second, newBounds);

        // Create a 2D wrapper
        auto wrapper = std::make_shared<SpatialObject2DWrapper>(updatedObject);

        // Update in the tree
        if (m_root && m_root->update(wrapper))
        {
            // Update our maps
            m_objectMap[objectID] = wrapper;
            m_originalObjects[objectID] = updatedObject;
            return true;
        }

        return false;
    }

    void Quadtree::clear()
    {
        if (!m_initialized)
        {
            return;
        }

        // Clear the maps
        m_objectMap.clear();
        m_originalObjects.clear();

        // Recreate the root node
        m_root = std::make_unique<QuadtreeNode>(
            m_worldBounds2D,
            0,              // depth
            nullptr,        // parent
            m_config.maxDepth,
            m_config.maxObjectsPerNode,
            m_config.minObjectsPerNode
        );

        Log::info("Quadtree cleared");
    }

    void Quadtree::setWorldBounds(const AABB& bounds)
    {
        // Extract 2D bounds from 3D bounds (using X and Z coordinates)
        m_worldBounds2D = AABB2D::fromAABB(bounds);
        m_worldBounds = bounds;

        // Update Y range for visualization
        m_minY = bounds.getMin().y;
        m_maxY = bounds.getMax().y;

        // Recreate the tree with new bounds
        if (m_initialized)
        {
            // Save existing objects
            auto savedObjects = m_originalObjects;

            // Clear the tree
            clear();

            // Reinsert all objects
            for (const auto& [id, object] : savedObjects)
            {
                // Create a 2D wrapper
                auto wrapper = std::make_shared<SpatialObject2DWrapper>(object);

                if (m_worldBounds2D.intersects(wrapper->getBounds2D()))
                {
                    m_root->insert(wrapper);
                    m_objectMap[id] = wrapper;
                    m_originalObjects[id] = object;
                }
                else
                {
                    Log::warn("Object outside new world bounds, not reinserted: ID " + id);
                }
            }

            Log::info(StringUtils::format("Quadtree world bounds updated: ({}, {}) to ({}, {})",
                                          m_worldBounds2D.getMin().x, m_worldBounds2D.getMin().y,
                                          m_worldBounds2D.getMax().x, m_worldBounds2D.getMax().y));
        }
    }

    const AABB& Quadtree::getWorldBounds() const
    {
        return m_worldBounds;
    }

    std::vector<std::shared_ptr<ISpatialObject>> Quadtree::queryAABB(const AABB& bounds) const
    {
        // Extract 2D bounds
        AABB2D bounds2D = AABB2D::fromAABB(bounds);

        // Query the tree
        std::vector<std::shared_ptr<ISpatialObject2D>> wrapperResults;
        if (m_initialized && m_root)
        {
            m_root->queryAABB(bounds2D, wrapperResults);
        }

        // Convert to original objects
        std::vector<std::shared_ptr<ISpatialObject>> results;
        for (const auto& wrapper : wrapperResults)
        {
            // Get the original 3D object
            auto id = wrapper->getID();
            auto it = m_originalObjects.find(id);
            if (it != m_originalObjects.end())
            {
                // Check if it actually intersects the 3D bounds
                if (it->second->getBounds().intersects(bounds))
                {
                    results.push_back(it->second);
                }
            }
        }

        return results;
    }

    std::vector<std::shared_ptr<ISpatialObject>> Quadtree::querySphere(const glm::vec3& center, float radius) const
    {
        // Perform a 2D circle query
        glm::vec2 center2D(center.x, center.z);

        std::vector<std::shared_ptr<ISpatialObject2D>> wrapperResults;
        if (m_initialized && m_root)
        {
            m_root->queryCircle(center2D, radius, wrapperResults);
        }

        // Convert to original objects
        std::vector<std::shared_ptr<ISpatialObject>> results;
        for (const auto& wrapper : wrapperResults)
        {
            // Get the original 3D object
            auto id = wrapper->getID();
            auto it = m_originalObjects.find(id);
            if (it != m_originalObjects.end())
            {
                // Perform a more precise 3D sphere test
                glm::vec3 closestPoint = it->second->getBounds().getClosestPoint(center);
                float distanceSquared = length2(closestPoint - center);

                if (distanceSquared <= radius * radius)
                {
                    results.push_back(it->second);
                }
            }
        }

        return results;
    }

    std::vector<std::shared_ptr<ISpatialObject>> Quadtree::queryRay(const Ray& ray, float maxDistance) const
    {
        // Extract 2D ray components (using X and Z coordinates)
        glm::vec2 origin2D(ray.getOrigin().x, ray.getOrigin().z);
        glm::vec2 direction2D(ray.getDirection().x, ray.getDirection().z);

        // Normalize the 2D direction
        float length = direction2D.length();
        if (length > 0.0001f)
        {
            direction2D /= length;
        }

        // Query the tree
        std::vector<std::shared_ptr<ISpatialObject2D>> wrapperResults;
        if (m_initialized && m_root)
        {
            m_root->queryRay(origin2D, direction2D, maxDistance, wrapperResults);
        }

        // Convert to original objects
        std::vector<std::shared_ptr<ISpatialObject>> results;
        for (const auto& wrapper : wrapperResults)
        {
            // Get the original 3D object
            auto id = wrapper->getID();
            auto it = m_originalObjects.find(id);
            if (it != m_originalObjects.end())
            {
                // Perform a more precise 3D ray test
                float t = 0.0f;
                if (it->second->getBounds().intersects(ray, t, maxDistance))
                {
                    results.push_back(it->second);
                }
            }
        }

        return results;
    }

    std::vector<std::shared_ptr<ISpatialObject>> Quadtree::queryFrustum(const Frustum& frustum) const
    {
        // For frustum, we'll convert the frustum corners to a 2D AABB
        std::array<glm::vec3, 8> corners = frustum.getCorners();

        // Find 2D bounds of the frustum corners
        glm::vec2 min(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        glm::vec2 max(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

        for (const auto& corner : corners)
        {
            min.x = std::min(min.x, corner.x);
            min.y = std::min(min.y, corner.z);
            max.x = std::max(max.x, corner.x);
            max.y = std::max(max.y, corner.z);
        }

        AABB2D frustumAABB2D(min, max);

        // Query the tree
        std::vector<std::shared_ptr<ISpatialObject2D>> wrapperResults;
        if (m_initialized && m_root)
        {
            m_root->queryAABB(frustumAABB2D, wrapperResults);
        }

        // Convert to original objects
        std::vector<std::shared_ptr<ISpatialObject>> results;
        for (const auto& wrapper : wrapperResults)
        {
            // Get the original 3D object
            auto id = wrapper->getID();
            auto it = m_originalObjects.find(id);
            if (it != m_originalObjects.end())
            {
                // Perform a more precise 3D frustum test
                if (frustum.testAABB(it->second->getBounds()))
                {
                    results.push_back(it->second);
                }
            }
        }

        return results;
    }

    size_t Quadtree::getObjectCount() const
    {
        return m_originalObjects.size();
    }

    int Quadtree::getTreeDepth() const
    {
        if (m_initialized && m_root)
        {
            return m_root->getMaxDepth();
        }
        return 0;
    }

    size_t Quadtree::getNodeCount() const
    {
        if (m_initialized && m_root)
        {
            return m_root->getNodeCount();
        }
        return 0;
    }

    void Quadtree::setYRange(float minY, float maxY)
    {
        m_minY = minY;
        m_maxY = maxY;

        // Update 3D world bounds
        m_worldBounds = m_worldBounds2D.toAABB(minY, maxY);
    }

} // QuadtreeNode implementation