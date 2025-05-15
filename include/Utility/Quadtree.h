// -------------------------------------------------------------------------
// Quadtree.h
// -------------------------------------------------------------------------
#pragma once

#include <array>
#include <memory>
#include <vector>
#include <unordered_map>

#include "Utility/SpatialPartitioning.h"
#include "glm/glm.hpp"

namespace PixelCraft::Utility
{

    /**
     * @brief 2D Axis-Aligned Bounding Box for quadtree
     */
    class AABB2D
    {
    public:
        /**
         * @brief Default constructor
         */
        AABB2D() : m_min(0.0f, 0.0f), m_max(0.0f, 0.0f)
        {
        }

        /**
         * @brief Constructor with min and max points
         * @param min The minimum point
         * @param max The maximum point
         */
        AABB2D(const glm::vec2& min, const glm::vec2& max) : m_min(min), m_max(max)
        {
        }

        /**
         * @brief Get the minimum point
         * @return The minimum point
         */
        const glm::vec2& getMin() const
        {
            return m_min;
        }

        /**
         * @brief Get the maximum point
         * @return The maximum point
         */
        const glm::vec2& getMax() const
        {
            return m_max;
        }

        /**
         * @brief Set the minimum point
         * @param min The minimum point
         */
        void setMin(const glm::vec2& min)
        {
            m_min = min;
        }

        /**
         * @brief Set the maximum point
         * @param max The maximum point
         */
        void setMax(const glm::vec2& max)
        {
            m_max = max;
        }

        /**
         * @brief Get the center of the bounding box
         * @return The center point
         */
        glm::vec2 getCenter() const
        {
            return (m_min + m_max) * 0.5f;
        }

        /**
         * @brief Get the extents (half-size) of the bounding box
         * @return The extents
         */
        glm::vec2 getExtents() const
        {
            return (m_max - m_min) * 0.5f;
        }

        /**
         * @brief Get the size of the bounding box
         * @return The size
         */
        glm::vec2 getSize() const
        {
            return m_max - m_min;
        }

        /**
         * @brief Get the area of the bounding box
         * @return The area
         */
        float getArea() const
        {
            glm::vec2 size = getSize();
            return size.x * size.y;
        }

        /**
         * @brief Get the perimeter of the bounding box
         * @return The perimeter
         */
        float getPerimeter() const
        {
            glm::vec2 size = getSize();
            return 2.0f * (size.x + size.y);
        }

        /**
         * @brief Check if the bounding box contains a point
         * @param point The point to check
         * @return True if the point is inside the bounding box
         */
        bool contains(const glm::vec2& point) const
        {
            return point.x >= m_min.x && point.x <= m_max.x &&
                point.y >= m_min.y && point.y <= m_max.y;
        }

        /**
         * @brief Check if the bounding box intersects another bounding box
         * @param other The other bounding box
         * @return True if the bounding boxes intersect
         */
        bool intersects(const AABB2D& other) const
        {
            return m_min.x <= other.m_max.x && m_max.x >= other.m_min.x &&
                m_min.y <= other.m_max.y && m_max.y >= other.m_min.y;
        }

        /**
         * @brief Convert from 3D AABB to 2D AABB (using X and Z coordinates)
         * @param aabb3D The 3D AABB
         * @return The 2D AABB
         */
        static AABB2D fromAABB(const AABB& aabb3D)
        {
            return AABB2D(
                glm::vec2(aabb3D.getMin().x, aabb3D.getMin().z),
                glm::vec2(aabb3D.getMax().x, aabb3D.getMax().z)
            );
        }

        /**
         * @brief Convert to 3D AABB with specified Y range
         * @param minY The minimum Y value
         * @param maxY The maximum Y value
         * @return The 3D AABB
         */
        AABB toAABB(float minY, float maxY) const
        {
            return AABB(
                glm::vec3(m_min.x, minY, m_min.y),
                glm::vec3(m_max.x, maxY, m_max.y)
            );
        }

    private:
        glm::vec2 m_min;    ///< Minimum point
        glm::vec2 m_max;    ///< Maximum point
    };

    /**
     * @brief Interface for objects that can be partitioned in 2D space
     */
    class ISpatialObject2D
    {
    public:
        virtual ~ISpatialObject2D() = default;

        /**
         * @brief Get the 2D bounding box of the object
         * @return The axis-aligned bounding box
         */
        virtual AABB2D getBounds2D() const = 0;

        /**
         * @brief Get the unique identifier for the object
         * @return The object's identifier
         */
        virtual uint64_t getID() const = 0;
    };

    /**
     * @brief Wrapper class to make 3D spatial objects compatible with 2D partitioning
     */
    class SpatialObject2DWrapper : public ISpatialObject2D
    {
    public:
        /**
         * @brief Constructor
         * @param object The 3D spatial object
         */
        SpatialObject2DWrapper(std::shared_ptr<ISpatialObject> object)
            : m_object(object)
        {
        }

        /**
         * @brief Get the 2D bounding box of the wrapped object
         * @return The 2D axis-aligned bounding box
         */
        AABB2D getBounds2D() const override
        {
            return AABB2D::fromAABB(m_object->getBounds());
        }

        /**
         * @brief Get the unique identifier for the object
         * @return The object's identifier
         */
        uint64_t getID() const override
        {
            return m_object->getID();
        }

        /**
         * @brief Get the wrapped 3D object
         * @return The object being wrapped
         */
        std::shared_ptr<ISpatialObject> getObject() const
        {
            return m_object;
        }

    private:
        std::shared_ptr<ISpatialObject> m_object;
    };

    /**
     * @brief Node in a quadtree spatial partitioning structure
     */
    class QuadtreeNode
    {
    public:
        /**
         * @brief Constructor
         * @param bounds The 2D bounding box of this node
         * @param depth The depth of this node in the tree
         * @param parent Pointer to the parent node (null for root)
         * @param maxDepth Maximum tree depth
         * @param maxObjects Maximum objects before splitting
         * @param minObjects Minimum objects before merging
         */
        QuadtreeNode(const AABB2D& bounds, int depth = 0, QuadtreeNode* parent = nullptr,
                     int maxDepth = 8, int maxObjects = 8, int minObjects = 2);

        /**
         * @brief Destructor
         */
        ~QuadtreeNode();

        /**
         * @brief Insert an object into the node
         * @param object The object to insert
         * @return True if the insertion was successful
         */
        bool insert(std::shared_ptr<ISpatialObject2D> object);

        /**
         * @brief Remove an object from the node
         * @param objectID The ID of the object to remove
         * @return True if the object was found and removed
         */
        bool remove(uint64_t objectID);

        /**
         * @brief Update an object's position in the tree
         * @param object The object to update
         * @return True if the update was successful
         */
        bool update(std::shared_ptr<ISpatialObject2D> object);

        /**
         * @brief Query objects within a 2D bounding box
         * @param bounds The query bounds
         * @param results Vector to store results
         */
        void queryAABB(const AABB2D& bounds, std::vector<std::shared_ptr<ISpatialObject2D>>& results) const;

        /**
         * @brief Query objects within a circle
         * @param center The center of the circle
         * @param radius The radius of the circle
         * @param results Vector to store results
         */
        void queryCircle(const glm::vec2& center, float radius, std::vector<std::shared_ptr<ISpatialObject2D>>& results) const;

        /**
         * @brief Query objects intersecting a 2D ray
         * @param origin The ray origin
         * @param direction The ray direction
         * @param maxDistance The maximum distance along the ray
         * @param results Vector to store results
         */
        void queryRay(const glm::vec2& origin, const glm::vec2& direction, float maxDistance, std::vector<std::shared_ptr<ISpatialObject2D>>& results) const;

        /**
         * @brief Check if this node is a leaf (has no children)
         * @return True if this is a leaf node
         */
        bool isLeaf() const;

        /**
         * @brief Visualize the quadtree for debugging
         * @param debugDraw Debug drawing utility
         * @param drawObjects Whether to draw contained objects
         * @param minY The minimum Y value for 3D visualization
         * @param maxY The maximum Y value for 3D visualization
         */
        void debugDraw(class DebugDraw& debugDraw, bool drawObjects = true, float minY = 0.0f, float maxY = 0.0f) const;

        /**
         * @brief Get the bounds of this node
         * @return The 2D bounding box
         */
        const AABB2D& getBounds() const
        {
            return m_bounds;
        }

        /**
         * @brief Get the depth of this node in the tree
         * @return The depth value
         */
        int getDepth() const
        {
            return m_depth;
        }

        /**
         * @brief Get the parent node
         * @return Pointer to the parent node (null for root)
         */
        QuadtreeNode* getParent() const
        {
            return m_parent;
        }

        /**
         * @brief Get the child at the specified index
         * @param index The child index (0-3)
         * @return Pointer to the child node
         */
        QuadtreeNode* getChild(int index) const;

        /**
         * @brief Get the number of objects in this node and its children
         * @return The total object count
         */
        size_t getObjectCount() const;

        /**
         * @brief Get the maximum depth of the subtree rooted at this node
         * @return The maximum depth
         */
        int getMaxDepth() const;

        /**
         * @brief Get the total number of nodes in the subtree rooted at this node
         * @return The total node count
         */
        size_t getNodeCount() const;

    private:
        AABB2D m_bounds;                                           ///< 2D bounding box of this node
        int m_depth;                                               ///< Depth in the tree
        QuadtreeNode* m_parent;                                    ///< Parent node (null for root)
        std::array<std::unique_ptr<QuadtreeNode>, 4> m_children;   ///< Child nodes
        std::vector<std::shared_ptr<ISpatialObject2D>> m_objects;  ///< Objects contained in this node
        int m_maxDepth;                                            ///< Maximum tree depth
        int m_maxObjects;                                          ///< Maximum objects before splitting
        int m_minObjects;                                          ///< Minimum objects before merging

        /**
         * @brief Split the node into 4 children
         */
        void split();

        /**
         * @brief Merge children back into this node
         */
        void merge();

        /**
         * @brief Check if the node contains an object with the given ID
         * @param objectID The ID to check
         * @return True if the object is in this node
         */
        bool containsObject(uint64_t objectID) const;

        /**
         * @brief Calculate the index of the child that would contain the given position
         * @param position The position to check
         * @return The child index (0-3)
         */
        int calculateChildIndex(const glm::vec2& position) const;

        /**
         * @brief Calculate the bounds of a child node
         * @param index The child index (0-3)
         * @return The child bounds
         */
        AABB2D calculateChildBounds(int index) const;
    };

    /**
     * @brief Quadtree implementation of spatial partitioning for 2D space
     */
    class Quadtree : public SpatialPartitioning
    {
    public:
        /**
         * @brief Constructor
         * @param worldBounds The 2D bounds of the entire world
         * @param config Configuration parameters
         */
        Quadtree(const AABB2D& worldBounds = AABB2D(glm::vec2(-1000.0f), glm::vec2(1000.0f)),
                 const SpatialPartitionConfig& config = SpatialPartitionConfig());

        /**
         * @brief Destructor
         */
        ~Quadtree() override;

        /**
         * @brief Initialize the quadtree
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Update the quadtree
         * @param deltaTime Time since last update
         */
        void update(float deltaTime) override;

        /**
         * @brief Render debug visualization
         */
        void render() override;

        /**
         * @brief Shut down the quadtree
         */
        void shutdown() override;

        /**
         * @brief Get the subsystem dependencies
         * @return Vector of dependency subsystem names
         */
        std::vector<std::string> getDependencies() const override;

        /**
         * @brief Insert an object into the quadtree
         * @param object The object to insert
         * @return The ID assigned to the object, or 0 if insertion failed
         */
        uint64_t insert(std::shared_ptr<ISpatialObject> object) override;

        /**
         * @brief Remove an object from the quadtree
         * @param objectID The ID of the object to remove
         * @return True if the object was found and removed
         */
        bool remove(uint64_t objectID) override;

        /**
         * @brief Update an object's position in the quadtree
         * @param objectID The ID of the object to update
         * @return True if the update was successful
         */
        bool update(uint64_t objectID) override;

        /**
         * @brief Update an object's position with a new bounding box
         * @param objectID The ID of the object to update
         * @param newBounds The new bounding box
         * @return True if the update was successful
         */
        bool update(uint64_t objectID, const AABB& newBounds) override;

        /**
         * @brief Clear all objects from the quadtree
         */
        void clear() override;

        /**
         * @brief Set the world bounds for the quadtree
         * @param bounds The new world bounds
         */
        void setWorldBounds(const AABB& bounds) override;

        /**
         * @brief Get the world bounds of the quadtree
         * @return The world bounds
         */
        const AABB& getWorldBounds() const override;

        /**
         * @brief Query objects that intersect the given bounding box
         * @param bounds The query bounds
         * @return Vector of objects within the bounds
         */
        std::vector<std::shared_ptr<ISpatialObject>> queryAABB(const AABB& bounds) const override;

        /**
         * @brief Query objects that intersect the given sphere
         * @param center The center of the sphere
         * @param radius The radius of the sphere
         * @return Vector of objects within the sphere
         */
        std::vector<std::shared_ptr<ISpatialObject>> querySphere(const glm::vec3& center, float radius) const override;

        /**
         * @brief Query objects that intersect the given ray
         * @param ray The ray to test
         * @param maxDistance The maximum distance along the ray
         * @return Vector of objects intersecting the ray
         */
        std::vector<std::shared_ptr<ISpatialObject>> queryRay(const Ray& ray, float maxDistance = FLT_MAX) const override;

        /**
         * @brief Query objects that are inside the given frustum
         * @param frustum The viewing frustum
         * @return Vector of objects inside the frustum
         */
        std::vector<std::shared_ptr<ISpatialObject>> queryFrustum(const Frustum& frustum) const override;

        /**
         * @brief Get the number of objects in the quadtree
         * @return The object count
         */
        size_t getObjectCount() const override;

        /**
         * @brief Get the depth of the quadtree
         * @return The maximum depth of any leaf node
         */
        int getTreeDepth() const override;

        /**
         * @brief Get the number of nodes in the quadtree
         * @return The node count
         */
        size_t getNodeCount() const override;

        /**
         * @brief Set the Y range for 3D visualization
         * @param minY The minimum Y value
         * @param maxY The maximum Y value
         */
        void setYRange(float minY, float maxY);

        /**
         * @brief Get the minimum Y value for 3D visualization
         * @return The minimum Y value
         */
        float getMinY() const
        {
            return m_minY;
        }

        /**
         * @brief Get the maximum Y value for 3D visualization
         * @return The maximum Y value
         */
        float getMaxY() const
        {
            return m_maxY;
        }

        /**
         * @brief Get the 2D world bounds
         * @return The 2D bounds
         */
        const AABB2D& getWorldBounds2D() const
        {
            return m_worldBounds2D;
        }

        /**
         * @brief Get the root node of the quadtree
         * @return The root node
         */
        const QuadtreeNode* getRootNode() const
        {
            return m_root.get();
        }

    private:
        std::unique_ptr<QuadtreeNode> m_root;                                       ///< Root node of the quadtree
        std::unordered_map<uint64_t, std::shared_ptr<ISpatialObject2D>> m_objectMap;   ///< Map of object IDs to 2D wrapper objects
        std::unordered_map<uint64_t, std::shared_ptr<ISpatialObject>> m_originalObjects; ///< Map of object IDs to original 3D objects
        AABB2D m_worldBounds2D;                                                     ///< 2D bounds of the entire world
        AABB m_worldBounds;                                                         ///< 3D bounds of the entire world
        float m_minY;                                                               ///< Minimum Y value for 3D visualization
        float m_maxY;                                                               ///< Maximum Y value for 3D visualization
    };

} // namespace PixelCraft::Utility