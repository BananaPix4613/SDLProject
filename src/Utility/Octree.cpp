// -------------------------------------------------------------------------
// Octree.cpp
// -------------------------------------------------------------------------
#include "Utility/Octree.h"
#include "Core/Application.h"
#include "Utility/DebugDraw.h"
#include "Core/Logger.h"
#include "Utility/Color.h"
#include "Utility/StringUtils.h"

using StringUtils = PixelCraft::Utility::StringUtils;
namespace Log = PixelCraft::Core;

namespace PixelCraft::Utility
{

    // OctreeNode implementation
    OctreeNode::OctreeNode(const AABB& bounds, int depth, OctreeNode* parent,
                           int maxDepth, int maxObjects, int minObjects)
    : SpatialNode(bounds, depth, parent)
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

    OctreeNode::~OctreeNode()
    {
        // Children are automatically cleaned up by unique_ptr
    }

    bool OctreeNode::insert(std::shared_ptr<ISpatialObject> object)
    {
        if (!object)
        {
            return false;
        }

        // Check if the object intersects this node
        if (!m_bounds.intersects(object->getBounds()))
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

    bool OctreeNode::remove(uint64_t objectID)
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

    bool OctreeNode::update(std::shared_ptr<ISpatialObject> object)
    {
        if (!object)
        {
            return false;
        }

        // Check if the object is still within this node
        if (!m_bounds.intersects(object->getBounds()))
        {
            // If not, we need to remove it and reinsert
            if (!remove(object->getID()))
            {
                return false;
            }

            // Find the nearest ancestor that contains the object
            SpatialNode* node = m_parent;
            while (node)
            {
                if (static_cast<OctreeNode*>(node)->m_bounds.intersects(object->getBounds()))
                {
                    return static_cast<OctreeNode*>(node)->insert(object);
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

        // If we reach here, the object wasn't found in this node or it's children
        return false;
    }

    void OctreeNode::queryAABB(const AABB& bounds, std::vector<std::shared_ptr<ISpatialObject>>& results) const
    {
        // Check if this node intersects the query bounds
        if (!m_bounds.intersects(bounds))
        {
            return;
        }

        // Add objects from this node that intersect the query bounds
        for (const auto& object : m_objects)
        {
            if (object->getBounds().intersects(bounds))
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

    void OctreeNode::querySphere(const glm::vec3& center, float radius, std::vector<std::shared_ptr<ISpatialObject>>& results) const
    {
        // Create a bounding box for the sphere
        AABB sphereAABB(
            center - glm::vec3(radius),
            center + glm::vec3(radius)
        );

        // Check if this node intersects the sphere's AABB
        if (!m_bounds.intersects(sphereAABB))
        {
            return;
        }

        // Add objects from this node that intersect the sphere
        for (const auto& object : m_objects)
        {
            // First do a quick AABB test
            if (object->getBounds().intersects(sphereAABB))
            {
                // Then do a more precise sphere test
                // We can approximate this by checking if the closest point on the AABB to the sphere center
                // is within the radius
                glm::vec3 closestPoint = object->getBounds().getClosestPoint(center);
                float distanceSquared = length2((closestPoint - center));

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
                    child->querySphere(center, radius, results);
                }
            }
        }
    }

    void OctreeNode::queryRay(const Ray& ray, float maxDistance, std::vector<std::shared_ptr<ISpatialObject>>& results) const
    {
        // Check if this node intersects the ray
        float t = 0.0f;
        if (!m_bounds.intersects(ray, t, maxDistance))
        {
            return;
        }

        // Add objects from this node that intersects the ray
        for (const auto& object : m_objects)
        {
            if (object->getBounds().intersects(ray, t, maxDistance))
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
                    child->queryRay(ray, maxDistance, results);
                }
            }
        }
    }

    void OctreeNode::queryFrustum(const Frustum& frustum, std::vector<std::shared_ptr<ISpatialObject>>& results) const
    {
        // Check if this node intersects the frustum
        if (!frustum.testAABB(m_bounds.getMin(), m_bounds.getMax()))
        {
            return;
        }

        // Add objects from this node that intersect the frustum
        for (const auto& object : m_objects)
        {
            if (frustum.testAABB(object->getBounds()))
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
                    child->queryFrustum(frustum, results);
                }
            }
        }
    }

    bool OctreeNode::isLeaf() const
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

    void OctreeNode::debugDraw(DebugDraw& debugDraw, bool drawObjects) const
    {
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

        debugDraw.drawBox(m_bounds, color.rgba(), false, 5.0f, false);

        // Draw contained objects if requested
        if (drawObjects)
        {
            for (const auto& object : m_objects)
            {
                debugDraw.drawBox(object->getBounds(), glm::vec4(0.2f, 0.2f, 0.8f, 0.5f), false, 5.0f, false);
            }
        }

        // Recursively draw children
        if (!isLeaf())
        {
            for (const auto& child : m_children)
            {
                if (child)
                {
                    child->debugDraw(debugDraw, drawObjects);
                }
            }
        }
    }

    OctreeNode* OctreeNode::getChild(int index) const
    {
        if (index >= 0 && index < 8)
        {
            return m_children[index].get();
        }
        return nullptr;
    }

    size_t OctreeNode::getObjectCount() const
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

    int OctreeNode::getMaxDepth() const
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

    size_t OctreeNode::getNodeCount() const
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

    void OctreeNode::split()
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

        // Create 8 children
        for (int i = 0; i < 8; i++)
        {
            AABB childBounds = calculateChildBounds(i);
            m_children[i] = std::make_unique<OctreeNode>(
                childBounds,
                m_depth + 1,
                this,
                m_maxDepth,
                m_maxObjects,
                m_minObjects
            );
        }

        // Redistribute objects to children
        std::vector<std::shared_ptr<ISpatialObject>> remainingObjects;
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

    void OctreeNode::merge()
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

                // Add the child's object to this node
                m_objects.insert(m_objects.end(), child->m_objects.begin(), child->m_objects.end());

                // Clear the child
                child.reset();
            }
        }
    }

    bool OctreeNode::containsObject(uint64_t objectID) const
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

    int OctreeNode::calculateChildIndex(const glm::vec3 &position) const
    {
        glm::vec3 center = m_bounds.getCenter();
        int index = 0;
        if (position.x >= center.x) index |= 1;
        if (position.y >= center.y) index |= 2;
        if (position.z >= center.z) index |= 4;
        return index;
    }

    AABB OctreeNode::calculateChildBounds(int index) const
    {
        glm::vec3 min = m_bounds.getMin();
        glm::vec3 max = m_bounds.getMax();
        glm::vec3 center = m_bounds.getCenter();

        // Calculate min and max for the child based on index
        glm::vec3 childMin, childMax;
        childMin.x = (index & 1) ? center.x : min.x;
        childMin.y = (index & 2) ? center.y : min.y;
        childMin.z = (index & 4) ? center.z : min.z;

        childMax.x = (index & 1) ? max.x : center.x;
        childMax.y = (index & 2) ? max.y : center.y;
        childMax.z = (index & 4) ? max.z : center.z;

        return AABB(childMin, childMax);
    }

    // Octree implementation
    Octree::Octree(const AABB& worldBounds, const SpatialPartitionConfig& config)
        : SpatialPartitioning(config), m_worldBounds(worldBounds)
    {
        // Constructor implementation
    }

    Octree::~Octree()
    {
        // Cleanup handled by shutdown()
    }

    bool Octree::initialize()
    {
        if (!SpatialPartitioning::initialize())
        {
            return false;
        }

        // Create the root node
        m_root = std::make_unique<OctreeNode>(
            m_worldBounds,
            0,              // depth
            nullptr,        // parent
            m_config.maxDepth,
            m_config.maxObjectsPerNode,
            m_config.minObjectsPerNode
        );

        Log::info(StringUtils::format("Octree initialized with world bounds: ({}, {}, {}) to ({}, {}, {})",
                                      m_worldBounds.getMin().x, m_worldBounds.getMin().y, m_worldBounds.getMin().z,
                                      m_worldBounds.getMax().x, m_worldBounds.getMax().y, m_worldBounds.getMax().z));

        return true;
    }

    void Octree::update(float deltaTime)
    {
        SpatialPartitioning::update(deltaTime);

        // If dynamic updates are enabled, we could perform periodic restructuring here
        if (!m_initialized || !m_config.dynamicTree)
        {
            return;
        }

        // Implementation for periodic restructuring or optimization
    }

    void Octree::render()
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

        // Draw octree
        if (m_root)
        {
            m_root->debugDraw(*debugDraw, true);
        }
    }

    void Octree::shutdown()
    {
        SpatialPartitioning::shutdown();

        // Clear all objects and the tree
        clear();
    }

    std::vector<std::string> Octree::getDependencies() const
    {
        return {
            "DebugDraw" // Only needed for visualization
        };
    }

    uint64_t Octree::insert(std::shared_ptr<ISpatialObject> object)
    {
        if (!m_initialized || !object)
        {
            return 0;
        }

        // Check if the object is within the world bounds
        if (!m_worldBounds.intersects(object->getBounds()))
        {
            Log::warn("Object outside world bounds, cannot insert: ID " + object->getID());
            return 0;
        }

        // Store the object in our map
        uint64_t id = object->getID();
        if (id == 0)
        {
            id = generateObjectID();
        }

        // Insert into the tree
        if (m_root && m_root->insert(object))
        {
            m_objectMap[id] = object;
            return id;
        }

        return 0;
    }

    bool Octree::remove(uint64_t objectID)
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
            // Remove from the map
            m_objectMap.erase(it);
            return true;
        }

        return false;
    }

    bool Octree::update(uint64_t objectID)
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

        // Update in the tree
        return m_root && m_root->update(it->second);
    }

    bool Octree::update(uint64_t objectID, const AABB& newBounds)
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

        auto updatedObject = std::make_shared<UpdatedObject>(it->second, newBounds);

        // Update in the tree
        if (m_root && m_root->update(updatedObject))
        {
            // Update the bounds in the original object if possible
            // This depends on the actual type of the object and whether it supports bounds updates

            // For now, we'll just keep the updated wrapper in our map
            it->second = updatedObject;
            return true;
        }

        return false;
    }

    void Octree::clear()
    {
        if (!m_initialized)
        {
            return;
        }

        // Clear the object map
        m_objectMap.clear();

        // Recreate the root node
        m_root = std::make_unique<OctreeNode>(
            m_worldBounds,
            0,              // depth
            nullptr,        // parent
            m_config.maxDepth,
            m_config.maxObjectsPerNode,
            m_config.minObjectsPerNode
        );

        Log::info("Octree cleared");
    }

    void Octree::setWorldBounds(const AABB& bounds)
    {
        m_worldBounds = bounds;

        // Recreate the tree with new bounds
        if (m_initialized)
        {
            // Save existing objects
            auto savedObjects = m_objectMap;

            // Clear the tree
            clear();

            // Reinsert all objects
            for (const auto& [id, object] : savedObjects)
            {
                if (m_worldBounds.intersects(object->getBounds()))
                {
                    m_root->insert(object);
                    m_objectMap[id] = object;
                }
                else
                {
                    Log::warn("Object outside new world bounds, not reinserted: ID " + id);
                }
            }

            Log::info(StringUtils::format("Octree world bounds updated: ({}, {}, {}) to ({}, {}, {})",
                                          m_worldBounds.getMin().x, m_worldBounds.getMin().y, m_worldBounds.getMin().z,
                                          m_worldBounds.getMax().x, m_worldBounds.getMax().y, m_worldBounds.getMax().z));
        }
    }

    const AABB& Octree::getWorldBounds() const
    {
        return m_worldBounds;
    }

    std::vector<std::shared_ptr<ISpatialObject>> Octree::queryAABB(const AABB& bounds) const
    {
        std::vector<std::shared_ptr<ISpatialObject>> results;

        if (m_initialized && m_root)
        {
            m_root->queryAABB(bounds, results);
        }

        return results;
    }

    std::vector<std::shared_ptr<ISpatialObject>> Octree::querySphere(const glm::vec3 &center, float radius) const
    {
        std::vector<std::shared_ptr<ISpatialObject>> results;

        if (m_initialized && m_root)
        {
            m_root->querySphere(center, radius, results);
        }

        return results;
    }

    std::vector<std::shared_ptr<ISpatialObject>> Octree::queryRay(const Ray &ray, float maxDistance) const
    {
        std::vector<std::shared_ptr<ISpatialObject>> results;

        if (m_initialized && m_root)
        {
            m_root->queryRay(ray, maxDistance, results);
        }

        return results;
    }

    std::vector<std::shared_ptr<ISpatialObject>> Octree::queryFrustum(const Frustum &frustum) const
    {
        std::vector<std::shared_ptr<ISpatialObject>> results;

        if (m_initialized && m_root)
        {
            m_root->queryFrustum(frustum, results);
        }

        return results;
    }

    size_t Octree::getObjectCount() const
    {
        return m_objectMap.size();
    }

    int Octree::getTreeDepth() const
    {
        if (m_initialized && m_root)
        {
            return m_root->getMaxDepth();
        }
        return 0;
    }

    size_t Octree::getNodeCount() const
    {
        if (m_initialized && m_root)
        {
            return m_root->getNodeCount();
        }
        return 0;
    }

} // namespace PixelCraft::Utility