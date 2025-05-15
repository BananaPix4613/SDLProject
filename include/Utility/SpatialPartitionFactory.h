// -------------------------------------------------------------------------
// SpatialPartitionFactory.h
// -------------------------------------------------------------------------
#pragma once

#include "Utility/SpatialPartitioning.h"
#include "Utility/Octree.h"
#include "Utility/Quadtree.h"
#include "Core/Subsystem.h"
#include "glm/glm.hpp"

namespace PixelCraft::Utility
{

    // Forward declarations
    class AABB;

    /**
     * @brief Factory for creating appropriate spatial partitioning structures
     * 
     * This factory provides a simplified interface for creating the most appropriate
     * spatial partitioning structure based on the dimensionality of the data and
     * other configuration parameters.
     */
    class SpatialPartitionFactory : public Core::Subsystem
    {
    public:
        /**
         * @brief Type of spatial partitioning structure
         */
        enum class PartitionType
        {
            Octree,     ///< 3D spatial partitioning (octree)
            Quadtree    ///< 2D spatial partitioning (quadtree)
        };

        /**
         * @brief Constructor
         */
        SpatialPartitionFactory();

        /**
         * @brief Destructor
         */
        ~SpatialPartitionFactory();

        /**
         * @brief Initialize the factory
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Update the factory
         * @param deltaTime Time since last update
         */
        void update(float deltaTime) override;

        /**
         * @brief Render debug visualization
         */
        void render() override;

        /**
         * @brief Shut down the factory
         */
        void shutdown() override;

        /**
         * @brief Get the name of the subsystem
         * @return The subsystem name
         */
        std::string getName() const override
        {
            return "SpatialPartitionFactory";
        }

        /**
         * @brief Get the subsystem dependencies
         * @return Vector of dependency subsystem names
         */
        std::vector<std::string> getDependencies() const override;

        /**
         * @brief Create a spatial partitioning structure
         * @param type The type of the structure to create
         * @param worldBounds The bounds of the entire world
         * @param config Configuration parameters
         * @return The created spatial partitioning structure
         */
        std::shared_ptr<SpatialPartitioning> create(
            PartitionType type,
            const AABB& worldBounds = AABB(glm::vec3(-1000.0f), glm::vec3(1000.0f)),
            const SpatialPartitionConfig& config = SpatialPartitionConfig());

        /**
         * @brief Create an octree
         * @param worldBounds The bounds of the entire world
         * @param config Configuration parameters
         * @return The created octree
         */
        std::shared_ptr<Octree> createOctree(
            const AABB& worldBounds = AABB(glm::vec3(-1000.0f), glm::vec3(1000.0f)),
            const SpatialPartitionConfig& config = SpatialPartitionConfig());

        /**
         * @brief Create a quadtree
         * @param worldBounds The bounds of the entire world
         * @param config Configuration parameters
         * @return The created quadtree
         */
        std::shared_ptr<Quadtree> createQuadtree(
            const AABB& worldBounds = AABB(glm::vec3(-1000.0f), glm::vec3(1000.0f)),
            const SpatialPartitionConfig& config = SpatialPartitionConfig());

        /**
         * @brief Create a spatial partitioning structure appropriate for terrain/heightmaps
         * @param worldBounds The bounds of the terrain
         * @param config Configuration parameters
         * @return The created spatial partitioning structure (usually a quadtree)
         */
        std::shared_ptr<SpatialPartitioning> createTerrainPartitioning(
            const AABB& worldBounds,
            const SpatialPartitionConfig& config = SpatialPartitionConfig());

        /**
         * @brief Create a spatial partitioning structure for urban environments
         * @param worldBounds The bounds of the urban area
         * @param config Configuration parameters
         * @return The created spatial partitioning structure
         */
        std::shared_ptr<SpatialPartitioning> createUrbanPartitioning(
            const AABB& worldBounds,
            const SpatialPartitionConfig& config = SpatialPartitionConfig());
        
        /**
         * @brief Create a spatial partitioning structure for indoor environments
         * @param worldBounds The bounds of the indoor area
         * @param config Configuration parameters
         * @return The created spatial partitioning structure
         */
        std::shared_ptr<SpatialPartitioning> createIndoorPartitioning(
            const AABB& worldBounds,
            const SpatialPartitionConfig& config = SpatialPartitionConfig());

        /**
         * @brief Create a spatial partitioning structure for dynamic objects
         * @param worldBounds The bounds of the entire world
         * @param config Configuration parameters
         * @return The created spatial partitioning structure
         */
        std::shared_ptr<SpatialPartitioning> createDynamicObjectPartitioning(
            const AABB& worldBounds,
            const SpatialPartitionConfig& config = SpatialPartitionConfig());

    private:
        bool m_initialized = false;
    };

} // namespace PixelCraft::Utility