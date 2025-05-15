// -------------------------------------------------------------------------
// FeaturePlacement.h
// -------------------------------------------------------------------------
#pragma once

#include "Voxel/ChunkCoord.h"

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <glm/glm.hpp>

namespace PixelCraft::Voxel
{

    // Forward declarations
    class Chunk;
    class Grid;
    struct GenerationContext;

    /**
     * @brief Constraint-based feature placement system
     * 
     * Manages the placement of features (structures, objects, etc.) in a voxel world
     * with support for constraints, patterns, and distribution control.
     */
    class FeaturePlacement
    {
    public:
        /**
         * @brief Feature constraint type
         */
        enum class ConstraintType
        {
            None,               // No constraint
            Elevation,          // Based on elevation (height)
            Distance,           // Based on distance from a point
            BiomeType,          // Based on biome
            SlopeAngle,         // Based on terrain slope
            NearWater,          // Near water bodies
            FarFromWater,       // Away from water bodies
            NearFeature,        // Near another feature
            FarFromFeature,     // Away from another feature
            NoiseThreshold,     // Based on noise value threshold
            Density,            // Based on feature density in area
            Custom              // Custom constraint
        };

        /**
         * @brief Constructor
         */
        FeaturePlacement();

        /**
         * @brief Destructor
         */
        virtual ~FeaturePlacement();

        /**
         * @brief Check if a feature can be placed at a position
         * @param position World position to check
         * @param featureType Type of the feature to place
         * @param context Generation context
         * @param grid Reference to the grid for queries
         * @return True if placement is allowed
         */
        bool canPlaceFeature(const glm::vec3& position, const std::string& featureType,
                             const GenerationContext& context, const Grid& grid) const;

        /**
         * @brief Place a feature at a position
         * @param position World position for placement
         * @param featureType Type of the feature to place
         * @param context Generation context
         * @param chunk Reference to the chunk for modification
         * @return True if placement was successful
         */
        bool placeFeature(const glm::vec3& position, const std::string& featureType,
                          const GenerationContext& context, Chunk& chunk);

        /**
         * @brief Find valid feature placements in a chunk
         * @param chunkCoord Coordinate of the chunk
         * @param featureType Type of the feature to place
         * @param maxCount Maximum number of placements to find
         * @param context Generation context
         * @param grid Reference to the grid for queries
         * @return Vector of valid world positions for feature placement
         */
        std::vector<glm::vec3> findPlacementsInChunk(const ChunkCoord& chunkCoord,
                                                     const std::string& featureType,
                                                     int maxCount,
                                                     const GenerationContext& context,
                                                     const Grid& grid) const;

        /**
         * @brief Register a feature type
         * @param featureType Unique name for the feature type
         * @param generator Function to generate the feature
         * @return True if successfully registered
         */
        bool registerFeatureType(const std::string& featureType,
                                 std::function<bool(const glm::vec3&, const GenerationContext&, Chunk&)> generator);

        /**
         * @brief Check if a feature type is registered
         * @param featureType Name of the feature type
         * @return True if feature type is registered
         */
        bool hasFeatureType(const std::string& featureType) const;

        /**
         * @brief Get all registered feature types
         * @return Vector of feature type names
         */
        std::vector<std::string> getFeatureTypes() const;

        /**
         * @brief Add a constraint for a feature type
         * @param featureType Name of the feature type
         * @param constraint Type of constraint to add
         * @param params Parameters for the constraint (depends on type)
         * @return True if successful
         */
        bool addConstraint(const std::string& featureType, ConstraintType constraint, const std::vector<float>& params = {});

        /**
         * @brief Remove a constraint from a feature type
         * @param featureType Name of the feature type
         * @param constraint Type of constraint to remove
         * @return True if constraint was found and removed
         */
        bool removeConstraint(const std::string& featureType, ConstraintType constraint);

        /**
         * @brief Clear all constraints for a feature type
         * @param featureType Name of the feature type
         * @return True if feature type was found
         */
        bool clearConstraints(const std::string& featureType);

        /**
         * @brief Get constraints for a feature type
         * @param featureType Name of the feature type
         * @return Vector of constraint types
         */
        std::vector<ConstraintType> getConstraints(const std::string& featureType) const;

        /**
         * @brief Get parameters for a specific constraint
         * @param featureType Name of the feature type
         * @param constraint Type of constraint
         * @return Vector of parameter values, empty if not found
         */
        std::vector<float> getConstraintParams(const std::string& featureType, ConstraintType constraint) const;

        /**
         * @brief Add a custom constraint
         * @param featureType Name of the feature type
         * @param name Unique name for the constraint
         * @param constraint Function implementing the constraint
         * @return True if successfully added
         */
        bool addCustomConstraint(const std::string& featureType, const std::string& name,
                                 std::function<bool(const glm::vec3&, const GenerationContext&, const Grid&)> constraint);

        /**
         * @brief Remove a custom constraint
         * @param featureType Name of the feature type
         * @param name Name of the custom constraint
         * @return True if constraint was found and removed
         */
        bool removeCustomConstraint(const std::string& featureType, const std::string& name);

        /**
         * @brief Register a point of interest (landmark)
         * @param position World position of the point of interest
         * @param type Type of the point of interest
         * @param radius Influence radius
         * @return True if successfully registered
         */
        bool registerPointOfInterest(const glm::vec3& position, const std::string& type, float radius);

        /**
         * @brief Remove a point of interest
         * @param position World position of the point of interest
         * @param type Type of the point of interest
         * @return True if point was found and removed
         */
        bool removePointOfInterest(const glm::vec3& position, const std::string& type);

        /**
         * @brief Find nearby points of interest
         * @param position World position to search from
         * @param maxDistance Maximum search distance
         * @param type Type of the point of interest to find, empty for any
         * @return Vector of positions of matching points of interest
         */
        std::vector<glm::vec3> findNearbyPointsOfInterest(const glm::vec3& position,
                                                          float maxDistance,
                                                          const std::string& type = "") const;

        /**
         * @brief Clear all registered points of interest
         */
        void clearPointsOfInterest();

        /**
         * @brief Set minimum spacing between features of the same type
         * @param featureType Name of the feature type
         * @param spacing Minimum spacing in world units
         */
        void setFeatureSpacing(const std::string& featureType, float spacing);

        /**
         * @brief Get minimum spacing for a feature type
         * @param featureType Name of the feature type
         * @return Minimum spacing, 0 if not found
         */
        float getFeatureSpacing(const std::string& featureType) const;

        /**
         * @brief Set the feature placement seed
         * @param seed Seed value for random placement
         */
        void setSeed(uint32_t seed);

        /**
         * @brief Get the current seed
         * @return The current seed value
         */
        uint32_t getSeed() const;

        /**
         * @brief Serialize the placement configuration
         * @param serialier Serializer to serialize with
         */
        void serialize(ECS::Serializer& serializer) const;

        /**
         * @brief Deserialize the placement configuration
         * @param deserializer Deserializer to deserialize with
         */
        void deserialize(ECS::Deserializer& deserializer);

    private:
        // Internal structures
        struct FeatureConstraint
        {
            ConstraintType type;
            std::vector<float> params;
        };

        struct CustomConstraintFunc
        {
            std::string name;
            std::function<bool(const glm::vec3&, const GenerationContext&, const Grid&)> func;
        };

        struct PointOfInterest
        {
            glm::vec3 position;
            std::string type;
            float radius;
        };

        struct FeatureTypeInfo
        {
            std::vector<FeatureConstraint> constraints;
            std::vector<CustomConstraintFunc> customConstraints;
            std::function<bool(const glm::vec3&, const GenerationContext&, Chunk&)> generator;
            float spacing = 0.0f;
        };

        /**
         * @brief Check a constraint to evaludate validity of a position
         * @param constraint Constraint to check
         * @param position Position to evaluate constraint at
         * @param context Generation context for reference
         * @param grid Reference to grid for queries
         * @return True if constraint is valid
         */
        bool checkConstraint(const FeatureConstraint& constraint, const glm::vec3& position,
                             const GenerationContext& context, const Grid& grid) const;

        /**
         * @brief Generate potential positions within a chunk
         * @param chunkCoord Chunk destination to generate positions in
         * @param count Amount of positions to generate
         * @param context Generation context for reference
         * @return Vector of positions within the chunk
         */
        std::vector<glm::vec3> generatePotentialPositions(const ChunkCoord& chunkCoord,
                                                          int count,
                                                          const GenerationContext& context) const;

        /**
         * @brief Check if a position is near an existing feature
         * @param position Position to evaluate
         * @param featureType Feature type to check for nearby
         * @param spacing Spacing from position to check for features
         * @return True if position is near an existing feature
         */
        bool isNearExistingFeature(const glm::vec3& position, const std::string& featureType, float spacing) const;

        // Private member variables
        uint32_t m_seed = 12345;
        std::unordered_map<std::string, FeatureTypeInfo> m_featureTypes;
        std::vector<PointOfInterest> m_pointsOfInterest;
        std::unordered_map<ChunkCoord, std::unordered_map<std::string, std::vector<glm::vec3>>> m_placedFeatures;
    };

} // namespace PixelCraft::Voxel