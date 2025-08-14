// -------------------------------------------------------------------------
// SpatialPartitioning.cpp
// -------------------------------------------------------------------------
#include "Utility/SpatialPartitioning.h"

#include "Core/Application.h"
#include "Core/Logger.h"
#include "Utility/DebugDraw.h"

namespace PixelCraft::Utility
{

    // SpatialNode implementation
    SpatialNode::SpatialNode(const AABB& bounds, int depth, SpatialNode* parent)
        : m_bounds(bounds), m_depth(depth), m_parent(parent)
    {
        // Constructor implementation
    }

    SpatialNode::~SpatialNode()
    {
        // Destructor implementation
    }

    // SpatialPartitioning implementation
    SpatialPartitioning::SpatialPartitioning(const SpatialPartitionConfig& config)
        : m_config(config), m_initialized(false), m_debugDrawEnabled(false), m_nextObjectID(0)
    {
        // Constructor implementation
    }

    SpatialPartitioning::~SpatialPartitioning()
    {
        if (m_initialized)
        {
            shutdown();
        }
    }

    bool SpatialPartitioning::initialize()
    {
        if (m_initialized)
        {
            Core::warn("SpatialPartitioning already initialized");
            return true;
        }

        Core::info("Initializing SpatialPartitioning subsystem");

        // Validate configuration
        if (m_config.maxDepth <= 0)
        {
            Core::error("Utility::SpatialPartitioning: Invalid maxDepth value (must be > 0): " + m_config.maxDepth);
            m_config.maxDepth = 8;
        }

        if (m_config.maxObjectsPerNode <= 0)
        {
            Core::error("Utility::SpatialPartitioning: Invalid maxObjectsPerNode value (must be > 0): " + m_config.maxObjectsPerNode);
            m_config.maxObjectsPerNode = 8;
        }

        if (m_config.minObjectsPerNode <= 0)
        {
            Core::error("Utility::SpatialPartitioning: Invalid minObjectsPerNode value (must be > 0): " + m_config.minObjectsPerNode);
            m_config.minObjectsPerNode = 2;
        }

        if (m_config.minObjectsPerNode >= m_config.maxObjectsPerNode)
        {
            Core::error("Utility::SpatialPartitioning: minObjectsPerNode (" + std::to_string(m_config.minObjectsPerNode) +
                       ") must be less than maxObjectsPerNode (" + std::to_string(m_config.maxObjectsPerNode) + ")");
            m_config.minObjectsPerNode = m_config.maxObjectsPerNode / 2;
        }

        if (m_config.looseness <= 0.0f)
        {
            Core::error("Utility::SpatialPartitioning: Invalid looseness value (must be > 0): " + std::to_string(m_config.looseness));
            m_config.looseness = 1.0f;
        }

        // Specific initialization to be implemented by derived classes

        m_initialized = true;
        return true;
    }

    void SpatialPartitioning::update(float deltaTime)
    {
        (void)deltaTime;  // Currently unused

        if (!m_initialized)
        {
            return;
        }

        // Generic update logic
        // (Specific implementation in derived classes)
    }

    void SpatialPartitioning::render()
    {
        if (!m_initialized || !m_debugDrawEnabled)
        {
            return;
        }

        auto debugDraw = Core::Application::getInstance()->getSubsystem<DebugDraw>();
        if (!debugDraw)
        {
            return;
        }

        // Debug visualization to be implemented by derived classes
    }
    void SpatialPartitioning::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        Core::info("Shutting down SpatialPartitioning subsystem");

        // Clear all objects
        clear();

        m_initialized = false;
    }

    uint64_t SpatialPartitioning::insert(std::shared_ptr<ISpatialObject> object)
    {
        if (!m_initialized || !object)
        {
            return 0;
        }

        // This is a base class implementation - derived classes should override
        // this with actual insertion logic
        Core::warn("SpatialPartitioning::insert called on base class - no objects will be inserted");
        return 0;
    }

    bool SpatialPartitioning::remove(uint64_t objectID)
    {
        if (!m_initialized || objectID == 0)
        {
            return false;
        }

        // This is a base class implementation - derived classes should override
        // this with actual removal logic
        Core::warn("SpatialPartitioning::remove called on base class - no objects will be removed");
        return false;
    }

    bool SpatialPartitioning::update(uint64_t objectID)
    {
        if (!m_initialized || objectID == 0)
        {
            return false;
        }

        // This is a base class implementation - derived classes should override
        // this with actual update logic
        Core::warn("SpatialPartitioning::update called on base class - no objects will be updated");
        return false;
    }

    bool SpatialPartitioning::update(uint64_t objectID, const AABB &newBounds)
    {
        (void)newBounds;  // Currently unused

        if (!m_initialized || objectID == 0)
        {
            return false;
        }

        // This is a base class implementation - derived classes should override
        // this with actual update logic
        Core::warn("SpatialPartitioning::update with new bounds called on base class - no objects will be updated");
        return false;
    }

    void SpatialPartitioning::clear()
    {
        if (!m_initialized)
        {
            return;
        }

        // This is a base class implementation - derived classes should override
        // this with actual clear logic
        Core::warn("SpatialPartitioning::clear called on base class - no objects will be cleared");
    }

    void SpatialPartitioning::setWorldBounds(const AABB &bounds)
    {
        (void)bounds;  // Currently unused

        if (!m_initialized)
        {
            return;
        }

        // This is a base class implementation - derived classes should override
        // this with actual logic to set world bounds
        Core::warn("SpatialPartitioning::setWorldBounds called on base class - world bounds will not be set");
    }

    const AABB& SpatialPartitioning::getWorldBounds() const
    {
        // This is a base class implementation - derived classes should override
        // this with actual logic to get world bounds
        static AABB emptyBounds;
        Core::warn("SpatialPartitioning::getWorldBounds called on base class - returning empty bounds");
        return emptyBounds;
    }

    std::vector<std::shared_ptr<ISpatialObject>> SpatialPartitioning::queryAABB(
        const AABB &bounds) const
    {
        (void)bounds;  // Currently unused

        if (!m_initialized)
        {
            return {};
        }

        // This is a base class implementation - derived classes should override
        // this with actual query logic
        Core::warn("SpatialPartitioning::queryAABB called on base class - no objects will be returned");
        return {};
    }

    std::vector<std::shared_ptr<ISpatialObject>> SpatialPartitioning::
    querySphere(const glm::vec3 &center, float radius) const
    {
        (void)center;  // Currently unused
        (void)radius;  // Currently unused

        if (!m_initialized)
        {
            return {};
        }

        // This is a base class implementation - derived classes should override
        // this with actual query logic
        Core::warn("SpatialPartitioning::querySphere called on base class - no objects will be returned");
        return {};
    }

    std::vector<std::shared_ptr<ISpatialObject>> SpatialPartitioning::queryRay(
        const Ray &ray, float maxDistance) const
    {
        (void)ray;  // Currently unused
        (void)maxDistance;  // Currently unused

        if (!m_initialized)
        {
            return {};
        }

        // This is a base class implementation - derived classes should override
        // this with actual query logic
        Core::warn("SpatialPartitioning::queryRay called on base class - no objects will be returned");
        return {};
    }

    std::vector<std::shared_ptr<ISpatialObject>> SpatialPartitioning::
    queryFrustum(const Frustum &frustum) const
    {
        (void)frustum;  // Currently unused

        if (!m_initialized)
        {
            return {};
        }

        // This is a base class implementation - derived classes should override
        // this with actual query logic
        Core::warn("SpatialPartitioning::queryFrustum called on base class - no objects will be returned");
        return {};
    }

    size_t SpatialPartitioning::getObjectCount() const
    {
        if (!m_initialized)
        {
            return 0;
        }

        // This is a base class implementation - derived classes should override
        // this with actual count logic
        Core::warn("SpatialPartitioning::getObjectCount called on base class - returning 0");
        return 0;
    }

    int SpatialPartitioning::getTreeDepth() const
    {
        if (!m_initialized)
        {
            return 0;
        }

        // This is a base class implementation - derived classes should override
        // this with actual depth calculation
        Core::warn("SpatialPartitioning::getTreeDepth called on base class - returning 0");
        return 0;
    }

    size_t SpatialPartitioning::getNodeCount() const
    {
        if (!m_initialized)
        {
            return 0;
        }

        // This is a base class implementation - derived classes should override
        // this with actual node count calculation
        Core::warn("SpatialPartitioning::getNodeCount called on base class - returning 0");
        return 0;
    }

}// namespace PixelCraft::Utility