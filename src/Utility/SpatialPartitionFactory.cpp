// -------------------------------------------------------------------------
// SpatialPartitionFactory.cpp
// -------------------------------------------------------------------------
#include "Utility/SpatialPartitionFactory.h"
#include "Core/Application.h"
#include "Core/Logger.h"

namespace Log = PixelCraft::Core;

namespace PixelCraft::Utility
{

    SpatialPartitionFactory::SpatialPartitionFactory()
        : m_initialized(false)
    {
        // Constructor implementation
    }

    SpatialPartitionFactory::~SpatialPartitionFactory()
    {
        if (m_initialized)
        {
            shutdown();
        }
    }

    bool SpatialPartitionFactory::initialize()
    {
        if (m_initialized)
        {
            Log::warn("SpatialPartitionFactory already initialized");
            return true;
        }

        Log::info("Initializing SpatialPartitionFactory subsystem");

        m_initialized = true;
        return true;
    }

    void SpatialPartitionFactory::update(float deltaTime)
    {
        if (!m_initialized)
        {
            return;
        }

        // Nothing to update
    }

    void SpatialPartitionFactory::render()
    {
        if (!m_initialized)
        {
            return;
        }

        // Nothing to render
    }

    void SpatialPartitionFactory::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        Log::info("Shutting down SpatialPartitionFactory subsystem");

        m_initialized = false;
    }

    std::vector<std::string> SpatialPartitionFactory::getDependencies() const
    {
        return {};
    }

    std::shared_ptr<SpatialPartitioning> SpatialPartitionFactory::create(
        PartitionType type,
        const AABB& worldBounds,
        const SpatialPartitionConfig& config)
    {

        switch (type)
        {
            case PartitionType::Octree:
                return createOctree(worldBounds, config);
            case PartitionType::Quadtree:
                return createQuadtree(worldBounds, config);
            default:
                Log::error("Unknown partition type");
                return nullptr;
        }
    }

    std::shared_ptr<Octree> SpatialPartitionFactory::createOctree(
        const AABB& worldBounds,
        const SpatialPartitionConfig& config)
    {

        auto octree = std::make_shared<Octree>(worldBounds, config);
        octree->initialize();
        return octree;
    }

    std::shared_ptr<Quadtree> SpatialPartitionFactory::createQuadtree(
        const AABB& worldBounds,
        const SpatialPartitionConfig& config)
    {

        auto quadtree = std::make_shared<Quadtree>(AABB2D::fromAABB(worldBounds), config);
        quadtree->initialize();
        return quadtree;
    }

    std::shared_ptr<SpatialPartitioning> SpatialPartitionFactory::createTerrainPartitioning(
        const AABB& worldBounds,
        const SpatialPartitionConfig& config)
    {

        // For terrain, quadtrees are often more efficient since terrrain is usually
        // height-mapped and thus primarily varies in X and Z dimensions

        // Create a specialized config for terrain
        SpatialPartitionConfig terrainConfig = config;
        terrainConfig.maxDepth = 10;         // Deeper for more granularity at leaf level
        terrainConfig.maxObjectsPerNode = 4;  // Fewer objects per node to improve query performance

        auto quadtree = createQuadtree(worldBounds, terrainConfig);

        // Set Y range based on world bounds
        quadtree->setYRange(worldBounds.getMin().y, worldBounds.getMax().y);

        return quadtree;
    }

    std::shared_ptr<SpatialPartitioning> SpatialPartitionFactory::createUrbanPartitioning(
        const AABB& worldBounds,
        const SpatialPartitionConfig& config)
    {

        // Urban environments often benefit from octrees due to vertical stacking
        // of structures (buildings of different heights)

        // Create a specialized config for urban environments
        SpatialPartitionConfig urbanConfig = config;
        urbanConfig.maxDepth = 8;             // Balanced depth
        urbanConfig.maxObjectsPerNode = 8;    // Standard value
        urbanConfig.looseness = 1.2f;         // Slightly loose bounds to reduce updating frequency

        return createOctree(worldBounds, urbanConfig);
    }

    std::shared_ptr<SpatialPartitioning> SpatialPartitionFactory::createIndoorPartitioning(
        const AABB& worldBounds,
        const SpatialPartitionConfig& config)
    {

        // Indoor environments benefit from octrees due to complex 3D structure

        // Create a specialized config for indoor environments
        SpatialPartitionConfig indoorConfig = config;
        indoorConfig.maxDepth = 6;             // Lower depth for smaller environments
        indoorConfig.maxObjectsPerNode = 16;   // More objects per node for tightly packed areas
        indoorConfig.looseness = 1.0f;         // Tight bounds for precise spatial queries

        return createOctree(worldBounds, indoorConfig);
    }

    std::shared_ptr<SpatialPartitioning> SpatialPartitionFactory::createDynamicObjectPartitioning(
        const AABB& worldBounds,
        const SpatialPartitionConfig& config)
    {

        // Dynamic objects benefit from octrees with optimized parameters for frequent updates

        // Create a specialized config for dynamic objects
        SpatialPartitionConfig dynamicConfig = config;
        dynamicConfig.maxDepth = 6;             // Lower depth to reduce update overhead
        dynamicConfig.maxObjectsPerNode = 12;   // More objects per node to reduce tree restructuring
        dynamicConfig.looseness = 1.5f;         // Looser bounds to reduce update frequency
        dynamicConfig.dynamicTree = true;       // Enable dynamic tree optimization

        return createOctree(worldBounds, dynamicConfig);
    }

} // namespace PixelCraft::Utility