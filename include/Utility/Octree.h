// -------------------------------------------------------------------------
// Octree.h
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
     * @brief Node in an octree spatial partitioning structure
     */
    class OctreeNode : public SpatialNode
    {
    public:
        /**
         * @brief Constructor
         * @param bounds The bounding box of this node
         * @param depth The depth of this node in the tree
         * @param parent Pointer to the parent node (null for root)
         * @param maxDepth Maximum tree depth
         * @param maxObjects Maximum objects before splitting
         * @param minObjects Minimum objects before merging
         */
        OctreeNode(const AABB& bounds, int depth = 0, OctreeNode* parent = nullptr,
                   int maxDepth = 8, int maxObjects = 8, int minObjects = 2);

        /**
         * @brief Destructor
         */
        ~OctreeNode() override;

        /**
         * @brief Insert an object into the node
         * @param object The object to insert
         * @return True if the insertion was successful
         */
        bool insert(std::shared_ptr<ISpatialObject> object) override;

        /**
         * @brief Remove an object from the node
         * @param objectID The ID of the object to remove
         * @return True if the object was found and removed
         */
        bool remove(uint64_t objectID) override;

        /**
         * @brief Update an object's position in the tree
         * @param object The object to update
         * @return True if the update was successful
         */
        bool update(std::shared_ptr<ISpatialObject> object) override;

        /**
         * @brief Query objects within a bounding box
         * @param bounds The query bounds
         * @param results Vector to store results
         */
        void queryAABB(const AABB& bounds, std::vector<std::shared_ptr<ISpatialObject>>& results) const override;

        /**
         * @brief Query objects within a sphere
         * @param center The center of the sphere
         * @param radius The radius of the sphere
         * @param results Vector to store results
         */
        void querySphere(const glm::vec3& center, float radius, std::vector<std::shared_ptr<ISpatialObject>>& results) const override;

        /**
         * @brief Query objects intersecting a ray
         * @param ray The ray to test
         * @param maxDistance The maximum distance along the ray
         * @param results Vector to store results
         */
        void queryRay(const Ray& ray, float maxDistance, std::vector<std::shared_ptr<ISpatialObject>>& results) const override;

        /**
         * @brief Query objects inside a frustum
         * @param frustum The viewing frustum
         * @param results Vector to store results
         */
        void queryFrustum(const Frustum& frustum, std::vector<std::shared_ptr<ISpatialObject>>& results) const override;

        /**
         * @brief Check if this node is a leaf (has no children)
         * @return True if this is a leaf node
         */
        bool isLeaf() const override;

        /**
         * @brief Visualize the octree for debugging
         * @param debugDraw Debug drawing utility
         * @param drawObjects Whether to draw contained objects
         */
        void debugDraw(DebugDraw& debugDraw, bool drawObjects = true) const override;

        /**
         * @brief Get the child at the specified index
         * @param index The child index (0-7)
         * @return Pointer to the child node
         */
        OctreeNode* getChild(int index) const;

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
        std::array<std::unique_ptr<OctreeNode>, 8> m_children;     ///< Child nodes
        std::vector<std::shared_ptr<ISpatialObject>> m_objects;    ///< Objects contained in this node
        int m_maxDepth;                                           ///< Maximum tree depth
        int m_maxObjects;                                         ///< Maximum objects before splitting
        int m_minObjects;                                         ///< Minimum objects before merging

        /**
         * @brief Split the node into 8 children
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
        bool containsObject(uint64_t objectID) const override;

        /**
         * @brief Calculate the index of the child that would contain the given position
         * @param position The position to check
         * @return The child index (0-7)
         */
        int calculateChildIndex(const glm::vec3& position) const;

        /**
         * @brief Calculate the bounds of a child node
         * @param index The child index (0-7)
         * @return The child bounds
         */
        AABB calculateChildBounds(int index) const;
    };

    /**
     * @brief Octree implementation of spatial partitioning for 3D space
     */
    class Octree : public SpatialPartitioning
    {
    public:
        /**
         * @brief Constructor
         * @param worldBounds The bounds of the entire world
         * @param config Configuration parameters
         */
        Octree(const AABB& worldBounds = AABB(glm::vec3(-1000.0f), glm::vec3(1000.0f)),
               const SpatialPartitionConfig& config = SpatialPartitionConfig());

        /**
         * @brief Destructor
         */
        ~Octree() override;

        /**
         * @brief Initialize the octree
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Update the octree
         * @param deltaTime Time since last update
         */
        void update(float deltaTime) override;

        /**
         * @brief Render debug visualization
         */
        void render() override;

        /**
         * @brief Shut down the octree
         */
        void shutdown() override;

        /**
         * @brief Get the subsystem dependencies
         * @return Vector of dependency subsystem names
         */
        std::vector<std::string> getDependencies() const override;

        /**
         * @brief Insert an object into the octree
         * @param object The object to insert
         * @return The ID assigned to the object, or 0 if insertion failed
         */
        uint64_t insert(std::shared_ptr<ISpatialObject> object) override;

        /**
         * @brief Remove an object from the octree
         * @param objectID The ID of the object to remove
         * @return True if the object was found and removed
         */
        bool remove(uint64_t objectID) override;

        /**
         * @brief Update an object's position in the octree
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
         * @brief Clear all objects from the octree
         */
        void clear() override;

        /**
         * @brief Set the world bounds for the octree
         * @param bounds The new world bounds
         */
        void setWorldBounds(const AABB& bounds) override;

        /**
         * @brief Get the world bounds of the octree
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
         * @brief Get the number of objects in the octree
         * @return The object count
         */
        size_t getObjectCount() const override;

        /**
         * @brief Get the depth of the octree
         * @return The maximum depth of any leaf node
         */
        int getTreeDepth() const override;

        /**
         * @brief Get the number of nodes in the octree
         * @return The node count
         */
        size_t getNodeCount() const override;

        /**
         * @brief Get the root node of the octree
         * @return The root node
         */
        const OctreeNode* getRootNode() const
        {
            return m_root.get();
        }

    private:
        std::unique_ptr<OctreeNode> m_root;                              ///< Root node of the octree
        std::unordered_map<uint64_t, std::shared_ptr<ISpatialObject>> m_objectMap;  ///< Map of the object IDs to objects
        AABB m_worldBounds;                                             ///< Bounds of the entire world
    };

} // namespace PixelCraft::Utility