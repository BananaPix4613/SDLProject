// -------------------------------------------------------------------------
// SpatialPartitioning.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <type_traits>

#include "Core/Subsystem.h"
#include "Utility/AABB.h"
#include "glm/glm.hpp"

namespace PixelCraft::Utility
{

    /**
     * @brief Interface for objects that can be spatially partitioned
     */
    class ISpatialObject
    {
    public:
        virtual ~ISpatialObject() = default;

        /**
         * @brief Get the bounding box of the object
         * @return The axis-aligned bounding box
         */
        virtual AABB getBounds() const = 0;

        /**
         * @brief Get the unique identifier for the object
         * @return The object's identifier
         */
        virtual uint64_t getID() const = 0;
    };

    /**
     * @brief Wrapper class to make any object with a getBounds() method compatible with spatial partitioning
     * @tparam T The type of object to wrap
     */
    template<typename T>
    class SpatialObjectWrapper : public ISpatialObject
    {
    public:
        /**
         * @brief Constructor
         * @param object The object to wrap
         * @param id Unique identifier for this object
         */
        SpatialObjectWrapper(std::shared_ptr<T> object, uint64_t id)
            : m_object(object), m_id(id)
        {
        }

        /**
         * @brief Get the bounding box of the wrapped object
         * @return The axis-aligned bounding box
         */
        AABB getBounds() const override
        {
            return m_object->getBounds();
        }

        /**
         * @brief Get the unique identifier for the object
         * @return The object's identifier
         */
        uint64_t getID() const override
        {
            return m_id;
        }

        /**
         * @brief Get the wrapped object
         * @return The object being wrapped
         */
        std::shared_ptr<T> getObject() const
        {
            return m_object;
        }

    private:
        std::shared_ptr<T> m_object;
        uint64_t m_id;
    };

    /**
     * @brief Base class for spatial partitioning nodes
     */
    class SpatialNode
    {
    public:
        /**
         * @brief Constructor
         * @param bounds The bounding box of this node
         * @param depth The depth of this node in the tree
         * @param parent Pointer to the parent node (null for root)
         */
        SpatialNode(const AABB& bounds, int depth = 0, SpatialNode* parent = nullptr);

        /**
         * @brief Destructor
         */
        virtual ~SpatialNode();

        /**
         * @brief Insert an object into the node
         * @param object The object to insert
         * @return True if the insertion was successful
         */
        virtual bool insert(std::shared_ptr<ISpatialObject> object) = 0;

        /**
         * @brief Remove an object from the node
         * @param objectID The ID of the object to remove
         * @return True if the object was found and removed
         */
        virtual bool remove(uint64_t objectID) = 0;

        /**
         * @brief Update an object's position in the tree
         * @param object The object to update
         * @return True if the update was successful
         */
        virtual bool update(std::shared_ptr<ISpatialObject> object) = 0;

        /**
         * @brief Query objects within a bounding box
         * @param bounds The query bounds
         * @param results Vector to store results
         */
        virtual void queryAABB(const AABB& bounds, std::vector<std::shared_ptr<ISpatialObject>>& results) const = 0;

        /**
         * @brief Query objects within a sphere
         * @param center The center of the sphere
         * @param radius The radius of the sphere
         * @param results Vector to store results
         */
        virtual void querySphere(const glm::vec3& center, float radius, std::vector<std::shared_ptr<ISpatialObject>>& results) const = 0;

        /**
         * @brief Query objects intersecting a ray
         * @param ray The ray to test
         * @param maxDistance The maximum distance along the ray
         * @param results Vector to store results
         */
        virtual void queryRay(const Ray& ray, float maxDistance, std::vector<std::shared_ptr<ISpatialObject>>& results) const = 0;

        /**
         * @brief Query objects inside a frustum
         * @param frustum The viewing frustum
         * @param results Vector to store results
         */
        virtual void queryFrustum(const Frustum& frustum, std::vector<std::shared_ptr<ISpatialObject>>& results) const = 0;

        /**
         * @brief Get the bounding box of this node
         * @return The axis-aligned bounding box
         */
        const AABB& getBounds() const
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
        SpatialNode* getParent() const
        {
            return m_parent;
        }

        /**
         * @brief Check if this node is a leaf (has no children)
         * @return True if this is a leaf node
         */
        virtual bool isLeaf() const = 0;

        /**
         * @brief Visualize the spatial partition for debugging
         * @param debugDraw Debug drawing utility
         * @param drawObjects Whether to draw contained objects
         */
        virtual void debugDraw(class DebugDraw& debugDraw, bool drawObjects = true) const = 0;

    protected:
        AABB m_bounds;                  ///< Bounding box of this node
        int m_depth;                    ///< Depth in the tree
        SpatialNode* m_parent;          ///< Parent node (null for root)

        /**
         * @brief Check if the node contains an object with the given ID
         * @param objectID The ID to check
         * @return True if the object is in this node
         */
        virtual bool containsObject(uint64_t objectID) const = 0;
    };

    /**
     * @brief Parameters for configuring a spatial partitioning structure
     */
    struct SpatialPartitionConfig
    {
        int maxDepth = 8;               ///< Maximum tree depth
        int maxObjectsPerNode = 8;      ///< Maximum objects before splitting
        int minObjectsPerNode = 2;      ///< Minimum objects before merging
        float looseness = 1.5f;         ///< Looseness factor for node bounds (1.0 = tight)
        bool dynamicTree = true;        ///< Whether the tree supports dynamic updates
    };

    /**
     * @brief Base class for spatial partitioning data structures
     */
    class SpatialPartitioning : public Core::Subsystem
    {
    public:
        /**
         * @brief Constructor
         * @param config Configuration parameters
         */
        SpatialPartitioning(const SpatialPartitionConfig& config = SpatialPartitionConfig());

        /**
         * @brief Destructor
         */
        virtual ~SpatialPartitioning();

        /**
         * @brief Initialize the spatial partitioning system
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Update the spatial partitioning system
         * @param deltaTime Time since last update
         */
        void update(float deltaTime) override;

        /**
         * @brief Render debug visualization
         */
        void render() override;

        /**
         * @brief Shut down the spatial partitioning system
         */
        void shutdown() override;

        /**
         * @brief Get the name of the subsystem
         * @return The subsystem name
         */
        std::string getName() const override
        {
            return "SpatialPartitioning";
        }

        /**
         * @brief Insert an object into the partitioning structure
         * @param object The object to insert
         * @return The ID assigned to the object, or 0 if insertion failed
         */
        virtual uint64_t insert(std::shared_ptr<ISpatialObject> object);

        /**
         * @brief Template method to wrap and insert any object with a getBounds() method
         * @tparam T The type of object
         * @param object The object to insert
         * @return The ID assigned to the object, or 0 if insertion failed
         */
        template<typename T>
        uint64_t insertObject(std::shared_ptr<T> object)
        {
            static_assert(std::is_member_function_pointer<decltype(&T::getBounds)>::value,
                          "Type must have a getBounds() method returning AABB");

            uint64_t id = generateObjectID();
            auto wrapper = std::make_shared<SpatialObjectWrapper<T>>(object, id);
            if (insert(wrapper))
            {
                return id;
            }
            return 0;
        }

        /**
         * @brief Remove an object from the partitioning structure
         * @param objectID The ID of the object to remove
         * @return True if the object was found and removed
         */
        virtual bool remove(uint64_t objectID);

        /**
         * @brief Update an object's position in the structure
         * @param objectID The ID of the object to update
         * @return True if the update was successful
         */
        virtual bool update(uint64_t objectID);

        /**
         * @brief Update an object's position with a new bounding box
         * @param objectID The ID of the object to update
         * @param newBounds The new bounding box
         * @return True if the update was successful
         */
        virtual bool update(uint64_t objectID, const AABB& newBounds);

        /**
         * @brief Clear all objects from the partitioning structure
         */
        virtual void clear();

        /**
         * @brief Set the world bounds for the partitioning structure
         * @param bounds The new world bounds
         */
        virtual void setWorldBounds(const AABB& bounds);

        /**
         * @brief Get the world bounds of the partitioning structure
         * @return The world bounds
         */
        virtual const AABB& getWorldBounds() const;

        /**
         * @brief Query objects that intersect the given bounding box
         * @param bounds The query bounds
         * @return Vector of objects within the bounds
         */
        virtual std::vector<std::shared_ptr<ISpatialObject>> queryAABB(const AABB& bounds) const;

        /**
         * @brief Query objects that intersect the given sphere
         * @param center The center of the sphere
         * @param radius The radius of the sphere
         * @return Vector of objects within the sphere
         */
        virtual std::vector<std::shared_ptr<ISpatialObject>> querySphere(const glm::vec3& center, float radius) const;

        /**
         * @brief Query objects that intersect the given ray
         * @param ray The ray to test
         * @param maxDistance The maximum distance along the ray
         * @return Vector of objects intersecting the ray
         */
        virtual std::vector<std::shared_ptr<ISpatialObject>> queryRay(const Ray& ray, float maxDistance = FLT_MAX) const;

        /**
         * @brief Query objects that are inside the given frustum
         * @param frustum The viewing frustum
         * @return Vector of objects inside the frustum
         */
        virtual std::vector<std::shared_ptr<ISpatialObject>> queryFrustum(const Frustum& frustum) const;

        /**
         * @brief Set whether debug visualization is enabled
         * @param enabled Whether debug visualization is enabled
         */
        void setDebugDrawEnabled(bool enabled)
        {
            m_debugDrawEnabled = enabled;
        }

        /**
         * @brief Check if debug visualization is enabled
         * @return True if debug visualization is enabled
         */
        bool isDebugDrawEnabled() const
        {
            return m_debugDrawEnabled;
        }

        /**
         * @brief Get the number of objects in the partitioning structure
         * @return The object count
         */
        virtual size_t getObjectCount() const;

        /**
         * @brief Get the depth of the partitioning structure
         * @return The maximum depth of any leaf node
         */
        virtual int getTreeDepth() const;

        /**
         * @brief Get the number of nodes in the partitioning structure
         * @return The node count
         */
        virtual size_t getNodeCount() const;

        /**
         * @brief Get the configuration parameters
         * @return The configuration parameters
         */
        const SpatialPartitionConfig& getConfig() const
        {
            return m_config;
        }

    protected:
        SpatialPartitionConfig m_config;    ///< Configuration parameters
        bool m_initialized = false;         ///< Whether the system has been initialized
        bool m_debugDrawEnabled = false;    ///< Whether debug visualization is enabled

        /**
         * @brief Generate a unique ID for a new object
         * @return The generated ID
         */
        uint64_t generateObjectID()
        {
            return m_nextObjectID++;
        }

    private:
        uint64_t m_nextObjectID = 0;        ///< Counter for generating unique object IDs
    };

} // namespace PixelCraft::Utility