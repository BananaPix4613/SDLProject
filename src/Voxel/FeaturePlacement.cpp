// -------------------------------------------------------------------------
// FeaturePlacement.cpp
// -------------------------------------------------------------------------
#include "Voxel/FeaturePlacement.h"
#include "Voxel/Chunk.h"
#include "Voxel/Grid.h"
#include "Voxel/ProceduralGenerationSystem.h"
#include "Voxel/BiomeManager.h"
#include "Voxel/NoiseGenerator.h"
#include "Core/Logger.h"
#include "Utility/Random.h"
#include "Utility/Math.h"

#include <algorithm>
#include <random>
#include <cmath>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Voxel
{

    FeaturePlacement::FeaturePlacement()
    {
        // Nothing specific to initialize
    }

    FeaturePlacement::~FeaturePlacement()
    {
        // Clean up any resources
    }

    bool FeaturePlacement::canPlaceFeature(const glm::vec3& position, const std::string& featureType,
                                           const GenerationContext& context, const Grid& grid) const
    {
        // Check if feature type is registered
        auto it = m_featureTypes.find(featureType);
        if (it == m_featureTypes.end())
        {
            Log::error("Feature type '" + featureType + "' not registered");
            return false;
        }

        const auto& featureInfo = it->second;

        // Check spacing from existing features
        if (featureInfo.spacing > 0.0f && isNearExistingFeature(position, featureType, featureInfo.spacing))
        {
            return false;
        }

        // Check standard constraints
        for (const auto& constraint : featureInfo.constraints)
        {
            if (!checkConstraint(constraint, position, context, grid))
            {
                return false;
            }
        }

        // Check custom constraints
        for (const auto& customConstraint : featureInfo.customConstraints)
        {
            if (!customConstraint.func(position, context, grid))
            {
                return false;
            }
        }

        return true;
    }

    bool FeaturePlacement::placeFeature(const glm::vec3& position, const std::string& featureType,
                                        const GenerationContext& context, Chunk& chunk)
    {
        // Check if feature type is registered
        auto it = m_featureTypes.find(featureType);
        if (it == m_featureTypes.end())
        {
            Log::error("Feature type '" + featureType + "' not registered");
            return false;
        }

        // Get the feature generator function
        const auto& generator = it->second.generator;
        if (!generator)
        {
            Log::error("No generator function for feature type '" + featureType + "'");
            return false;
        }

        // Call the generator function
        bool success = generator(position, context, chunk);

        // If successful, record the placement
        if (success)
        {
            m_placedFeatures[chunk.getCoord()][featureType].push_back(position);
        }

        return success;
    }

    std::vector<glm::vec3> FeaturePlacement::findPlacementsInChunk(const ChunkCoord& chunkCoord,
                                                                   const std::string& featureType,
                                                                   int maxCount,
                                                                   const GenerationContext& context,
                                                                   const Grid& grid) const
    {
        // Check if feature type is registered
        auto it = m_featureTypes.find(featureType);
        if (it == m_featureTypes.end())
        {
            Log::error("Feature type '" + featureType + "' not registered");
            return {};
        }

        // Generate potential positions
        std::vector<glm::vec3> potentialPositions = generatePotentialPositions(chunkCoord, maxCount * 2, context);

        // Check each position against constraints
        std::vector<glm::vec3> validPositions;
        for (const auto& position : potentialPositions)
        {
            if (canPlaceFeature(position, featureType, context, grid))
            {
                validPositions.push_back(position);

                if (validPositions.size() >= static_cast<size_t>(maxCount))
                {
                    break;
                }
            }
        }

        return validPositions;
    }

    bool FeaturePlacement::registerFeatureType(const std::string& featureType,
                                               std::function<bool(const glm::vec3&, const GenerationContext&, Chunk&)> generator)
    {
        if (featureType.empty())
        {
            Log::error("Feature type name cannot be empty");
            return false;
        }

        if (!generator)
        {
            Log::error("Generator function cannot be null");
            return false;
        }

        // Check if already registered
        if (m_featureTypes.find(featureType) != m_featureTypes.end())
        {
            Log::warn("Feature type '" + featureType + "' already registered");
            return false;
        }

        // Create new feature type info
        FeatureTypeInfo info;
        info.generator = generator;

        m_featureTypes[featureType] = info;
        return true;
    }

    bool FeaturePlacement::hasFeatureType(const std::string& featureType) const
    {
        return m_featureTypes.find(featureType) != m_featureTypes.end();
    }

    std::vector<std::string> FeaturePlacement::getFeatureTypes() const
    {
        std::vector<std::string> types;
        types.reserve(m_featureTypes.size());

        for (const auto& [type, _] : m_featureTypes)
        {
            types.push_back(type);
        }

        return types;
    }

    bool FeaturePlacement::addConstraint(const std::string& featureType, ConstraintType constraint, const std::vector<float>& params)
    {
        auto it = m_featureTypes.find(featureType);
        if (it == m_featureTypes.end())
        {
            Log::error("Feature type '" + featureType + "' not registered");
            return false;
        }

        // Create constraint
        FeatureConstraint newConstraint;
        newConstraint.type = constraint;
        newConstraint.params = params;

        // Add to feature type
        it->second.constraints.push_back(newConstraint);
        return true;
    }

    bool FeaturePlacement::removeConstraint(const std::string& featureType, ConstraintType constraint)
    {
        auto it = m_featureTypes.find(featureType);
        if (it == m_featureTypes.end())
        {
            Log::error("Feature type '" + featureType + "' not registered");
            return false;
        }

        auto& constraints = it->second.constraints;
        auto constraintIt = std::find_if(constraints.begin(), constraints.end(),
                                         [constraint](const FeatureConstraint& c) { return c.type == constraint; });

        if (constraintIt == constraints.end())
        {
            Log::warn("Constraint not found for feature type '" + featureType + "'");
            return false;
        }

        constraints.erase(constraintIt);
        return true;
    }

    bool FeaturePlacement::clearConstraints(const std::string& featureType)
    {
        auto it = m_featureTypes.find(featureType);
        if (it == m_featureTypes.end())
        {
            Log::error("Feature type '" + featureType + "' not registered");
            return false;
        }

        it->second.constraints.clear();
        return true;
    }

    std::vector<FeaturePlacement::ConstraintType> FeaturePlacement::getConstraints(const std::string& featureType) const
    {
        auto it = m_featureTypes.find(featureType);
        if (it == m_featureTypes.end())
        {
            return {};
        }

        std::vector<ConstraintType> types;
        for (const auto& constraint : it->second.constraints)
        {
            types.push_back(constraint.type);
        }

        return types;
    }

    std::vector<float> FeaturePlacement::getConstraintParams(const std::string& featureType, ConstraintType constraint) const
    {
        auto it = m_featureTypes.find(featureType);
        if (it == m_featureTypes.end())
        {
            return {};
        }

        for (const auto& c : it->second.constraints)
        {
            if (c.type == constraint)
            {
                return c.params;
            }
        }

        return {};
    }

    bool FeaturePlacement::addCustomConstraint(const std::string& featureType, const std::string& name,
                                               std::function<bool(const glm::vec3&, const GenerationContext&, const Grid&)> constraint)
    {
        if (name.empty() || !constraint)
        {
            Log::error("Constraint name or function cannot be empty");
            return false;
        }

        auto it = m_featureTypes.find(featureType);
        if (it == m_featureTypes.end())
        {
            Log::error("Feature type '" + featureType + "' not registered");
            return false;
        }

        // Check if constraint already exists
        auto& customConstraints = it->second.customConstraints;
        auto existingIt = std::find_if(customConstraints.begin(), customConstraints.end(),
                                       [&name](const CustomConstraintFunc& c) { return c.name == name; });

        if (existingIt != customConstraints.end())
        {
            Log::warn("Custom constraint '" + name + "' already exists for feature type '" + featureType + "'");
            return false;
        }

        // Add new constraint
        CustomConstraintFunc customConstraint;
        customConstraint.name = name;
        customConstraint.func = constraint;

        customConstraints.push_back(customConstraint);
        return true;
    }

    bool FeaturePlacement::removeCustomConstraint(const std::string& featureType, const std::string& name)
    {
        auto it = m_featureTypes.find(featureType);
        if (it == m_featureTypes.end())
        {
            Log::error("Feature type '" + featureType + "' not registered");
            return false;
        }

        auto& customConstraints = it->second.customConstraints;
        auto constraintIt = std::find_if(customConstraints.begin(), customConstraints.end(),
                                         [&name](const CustomConstraintFunc& c) { return c.name == name; });

        if (constraintIt == customConstraints.end())
        {
            Log::warn("Custom constraint '" + name + "' not found for feature type '" + featureType + "'");
            return false;
        }

        customConstraints.erase(constraintIt);
        return true;
    }

    bool FeaturePlacement::registerPointOfInterest(const glm::vec3& position, const std::string& type, float radius)
    {
        PointOfInterest poi;
        poi.position = position;
        poi.type = type;
        poi.radius = radius;

        m_pointsOfInterest.push_back(poi);
        return true;
    }

    bool FeaturePlacement::removePointOfInterest(const glm::vec3& position, const std::string& type)
    {
        auto it = std::find_if(m_pointsOfInterest.begin(), m_pointsOfInterest.end(),
                               [&position, &type](const PointOfInterest& poi) {
                                   return poi.type == type && glm::distance(poi.position, position) < 0.1f;
                               });

        if (it == m_pointsOfInterest.end())
        {
            return false;
        }

        m_pointsOfInterest.erase(it);
        return true;
    }

    std::vector<glm::vec3> FeaturePlacement::findNearbyPointsOfInterest(const glm::vec3& position,
                                                                        float maxDistance,
                                                                        const std::string& type) const
    {
        std::vector<glm::vec3> result;

        for (const auto& poi : m_pointsOfInterest)
        {
            if (!type.empty() && poi.type != type)
            {
                continue;
            }

            float distance = glm::distance(position, poi.position);
            if (distance <= maxDistance)
            {
                result.push_back(poi.position);
            }
        }

        return result;
    }

    void FeaturePlacement::clearPointsOfInterest()
    {
        m_pointsOfInterest.clear();
    }

    void FeaturePlacement::setFeatureSpacing(const std::string& featureType, float spacing)
    {
        auto it = m_featureTypes.find(featureType);
        if (it == m_featureTypes.end())
        {
            Log::error("Feature type '" + featureType + "' not registered");
            return;
        }

        it->second.spacing = spacing;
    }

    float FeaturePlacement::getFeatureSpacing(const std::string& featureType) const
    {
        auto it = m_featureTypes.find(featureType);
        if (it == m_featureTypes.end())
        {
            return 0.0f;
        }

        return it->second.spacing;
    }

    void FeaturePlacement::setSeed(uint32_t seed)
    {
        m_seed = seed;
    }

    uint32_t FeaturePlacement::getSeed() const
    {
        return m_seed;
    }

    void FeaturePlacement::serialize(ECS::Serializer& serializer) const
    {
        // Serialize basic properties
        serializer.write(m_seed);

        // Serialize feature types
        serializer.beginArray("featureTypes", m_featureTypes.size());
        for (const auto& [typeName, typeInfo] : m_featureTypes)
        {
            serializer.beginObject("Feature");

            // Feature spacing
            serializer.write(typeInfo.spacing);

            serializer.beginArray("constraints", typeInfo.constraints.size());
            for (size_t i = 0; i < typeInfo.constraints.size(); i++)
            {
                const auto& constraint = typeInfo.constraints[i];
                serializer.beginObject("Constraint");

                serializer.write(static_cast<int>(constraint.type));

                // Serialize params
                serializer.beginArray("params", constraint.params.size());
                for (size_t j = 0; j < constraint.params.size(); j++)
                {
                    serializer.write(constraint.params[j]);
                }
                serializer.endArray();
                serializer.endObject();
            }
            serializer.endArray();

            // Custom contraints cannot be serialized (functions)
            // Just serialize their names for reference
            serializer.beginArray("customConstraints", typeInfo.customConstraints.size());
            for (size_t i = 0; i < typeInfo.customConstraints.size(); i++)
            {
                serializer.writeString(typeInfo.customConstraints[i].name);
            }
            serializer.endArray();
            serializer.endObject();
        }
        serializer.endArray();

        // Serialize points of interest
        serializer.beginArray("pointsOfInterest", m_pointsOfInterest.size());
        for (size_t i = 0; i < m_pointsOfInterest.size(); i++)
        {
            const auto& poi = m_pointsOfInterest[i];
            serializer.beginObject("POI");

            serializer.writeVec3(poi.position);
            serializer.writeString(poi.type);
            serializer.write(poi.radius);

            serializer.endObject();
        }
        serializer.endArray();

        // Placed features are not serialized (runtime state)
    }

    void FeaturePlacement::deserialize(ECS::Deserializer& deserializer)
    {
        // Clear existing data
        m_featureTypes.clear();
        m_pointsOfInterest.clear();
        m_placedFeatures.clear();

        // Deserialize basic properties
        deserializer.read(m_seed);

        //// Serialize feature types
        //size_t featureArraySize;
        //deserializer.beginArray("featureTypes", featureArraySize);
        //for (const auto& [typeName, typeInfo] : m_featureTypes)
        //{
        //    deserializer.beginObject("Feature");

        //    // Feature spacing
        //    deserializer.read(typeInfo.spacing);

        //    size_t constraintsArraySize;
        //    deserializer.beginArray("constraints", constraintsArraySize);
        //    for (size_t i = 0; i < typeInfo.constraints.size(); i++)
        //    {
        //        const FeatureConstraint& constraint;
        //        deserializer.beginObject("Constraint");

        //        int type;
        //        deserializer.read(type);
        //        constraint.type = static_cast<ConstraintType>(type);

        //        // Serialize params
        //        size_t paramsArraySize;
        //        deserializer.beginArray("params", paramsArraySize);
        //        for (size_t j = 0; j < constraint.params.size(); j++)
        //        {
        //            deserializer.read(constraint.params[j]);
        //        }
        //        deserializer.endArray();
        //        deserializer.endObject();
        //    }
        //    deserializer.endArray();

        //    // Custom contraints cannot be serialized (functions)
        //    // Just serialize their names for reference
        //    size_t customArraySize;
        //    deserializer.beginArray("customConstraints", customArraySize);
        //    for (size_t i = 0; i < typeInfo.customConstraints.size(); i++)
        //    {
        //        deserializer.readString(typeInfo.customConstraints[i].name);
        //    }
        //    deserializer.endArray();
        //    deserializer.endObject();
        //}
        //deserializer.endArray();

        // Serialize points of interest
        size_t poiArraySize;
        deserializer.beginArray("pointsOfInterest", poiArraySize);
        for (size_t i = 0; i < poiArraySize; i++)
        {
            PointOfInterest poi;
            deserializer.beginObject("POI");

            deserializer.readVec3(poi.position);
            deserializer.readString(poi.type);
            deserializer.read(poi.radius);

            deserializer.endObject();
            m_pointsOfInterest.push_back(poi);
        }
        deserializer.endArray();

        // Feature types and constraints need to be registered at runtime
        // because generator and constraint functions cannot be serialized
    }

    // Private helper methods

    bool FeaturePlacement::checkConstraint(const FeatureConstraint& constraint, const glm::vec3& position,
                                           const GenerationContext& context, const Grid& grid) const
    {
        switch (constraint.type)
        {
            case ConstraintType::None:
                return true;

            case ConstraintType::Elevation:
            {
                // Requires params: [minHeight, maxHeight]
                if (constraint.params.size() < 2)
                {
                    return true; // Invalid constraint params, allow placement
                }

                float minHeight = constraint.params[0];
                float maxHeight = constraint.params[1];

                return position.y >= minHeight && position.y <= maxHeight;
            }

            case ConstraintType::Distance:
            {
                // Requires params: [refX, refY, refZ, minDist, maxDist]
                if (constraint.params.size() < 5)
                {
                    return true; // Invalid constraint params, allow placement
                }

                glm::vec3 refPoint(constraint.params[0], constraint.params[1], constraint.params[2]);
                float minDist = constraint.params[3];
                float maxDist = constraint.params[4];

                float dist = glm::distance(position, refPoint);
                return dist >= minDist && dist <= maxDist;
            }

            case ConstraintType::BiomeType:
            {
                // Biome constraint, requires a biome manager
                if (!context.biomeManager)
                {
                    return true; // No biome manager, allow placement
                }

                // Requires params: [biomeId1, biomeId2, ...]
                if (constraint.params.empty())
                {
                    return true; // No biome IDs specified, allow placement
                }

                int biomeId = context.biomeManager->getBiomeAt(position, context);
                if (biomeId < 0)
                {
                    return false; // No biome at this position
                }

                // Check if biome ID is in the allowed list
                for (float allowedId : constraint.params)
                {
                    if (static_cast<int>(allowedId) == biomeId)
                    {
                        return true;
                    }
                }

                return false;
            }

            case ConstraintType::SlopeAngle:
            {
                // Requires params: [maxSlopeAngleDegrees]
                if (constraint.params.empty())
                {
                    return true; // Invalid constraint params, allow placement
                }

                float maxSlopeAngle = constraint.params[0];

                // Calculate terrain normal at this point
                // Approximate by sampling nearby points
                const float sampleDist = 1.0f;
                glm::vec3 p1 = position + glm::vec3(sampleDist, 0.0f, 0.0f);
                glm::vec3 p2 = position + glm::vec3(-sampleDist, 0.0f, 0.0f);
                glm::vec3 p3 = position + glm::vec3(0.0f, 0.0f, sampleDist);
                glm::vec3 p4 = position + glm::vec3(0.0f, 0.0f, -sampleDist);

                Voxel v1 = grid.getVoxel(p1);
                Voxel v2 = grid.getVoxel(p2);
                Voxel v3 = grid.getVoxel(p3);
                Voxel v4 = grid.getVoxel(p4);

                // Calculate height differences
                float dx = (v1.type == 0 ? -1.0f : 1.0f) - (v2.type == 0 ? -1.0f : 1.0f);
                float dz = (v3.type == 0 ? -1.0f : 1.0f) - (v4.type == 0 ? -1.0f : 1.0f);

                // Calculate normal
                glm::vec3 normal = glm::normalize(glm::vec3(-dx, 2.0f, -dz));

                // Calculate angle between normal and up vector
                float angle = glm::degrees(std::acos(glm::dot(normal, glm::vec3(0.0f, 1.0f, 0.0f))));

                return angle <= maxSlopeAngle;
            }

            case ConstraintType::NearWater:
            {
                // Requires params: [maxDistance]
                if (constraint.params.empty())
                {
                    return true; // Invalid constraint params, allow placement
                }

                float maxDistance = constraint.params[0];

                // Check surrounding points for water
                for (float dx = -maxDistance; dx <= maxDistance; dx += 1.0f)
                {
                    for (float dz = -maxDistance; dz <= maxDistance; dz += 1.0f)
                    {
                        glm::vec3 checkPos = position + glm::vec3(dx, 0.0f, dz);
                        Voxel voxel = grid.getVoxel(checkPos);

                        // Water type is 3
                        if (voxel.type == 3)
                        {
                            return true;
                        }
                    }
                }

                return false;
            }

            case ConstraintType::FarFromWater:
            {
                // Requires params: [minDistance]
                if (constraint.params.empty())
                {
                    return true; // Invalid constraint params, allow placement
                }

                float minDistance = constraint.params[0];

                // Check surrounding points for water
                for (float dx = -minDistance; dx <= minDistance; dx += 1.0f)
                {
                    for (float dz = -minDistance; dz <= minDistance; dz += 1.0f)
                    {
                        glm::vec3 checkPos = position + glm::vec3(dx, 0.0f, dz);
                        Voxel voxel = grid.getVoxel(checkPos);

                        // Water is type 3
                        if (voxel.type == 3)
                        {
                            return false;
                        }
                    }
                }

                return true;
            }

            case ConstraintType::NearFeature:
            {
                // Requires params: [maxDistance, featureTypeIndex1, featureTypeIndex2, ...]
                if (constraint.params.size() < 2)
                {
                    return true; // Invalid constraint params, allow placement
                }

                float maxDistance = constraint.params[0];

                // Check all points of interest for matching feature types
                for (const auto& poi : m_pointsOfInterest)
                {
                    // Check if the POI type matches any of the specified feature types
                    bool typeMatch = false;
                    for (size_t i = 1; i < constraint.params.size(); i++)
                    {
                        int typeIndex = static_cast<int>(constraint.params[i]);

                        // Convert type index to name (simplified)
                        std::string typeName = std::to_string(typeIndex);

                        if (poi.type == typeName)
                        {
                            typeMatch = true;
                            break;
                        }
                    }

                    if (typeMatch && glm::distance(position, poi.position) <= maxDistance)
                    {
                        return true;
                    }
                }

                return false;
            }

            case ConstraintType::FarFromFeature:
            {
                // Requires params: [minDistance, featureTypeIndex1, featureTypeIndex2, ...]
                if (constraint.params.size() < 2)
                {
                    return true; // Invalid constraint params, allow placement
                }

                float minDistance = constraint.params[0];

                // Check all points of interest for matching feature types
                for (const auto& poi : m_pointsOfInterest)
                {
                    // Check if the POI type matches any of the specified feature types
                    bool typeMatch = false;
                    for (size_t i = 1; i < constraint.params.size(); i++)
                    {
                        int typeIndex = static_cast<int>(constraint.params[i]);

                        // Convert type index to name (simplified)
                        std::string typeName = std::to_string(typeIndex);

                        if (poi.type == typeName)
                        {
                            typeMatch = true;
                            break;
                        }
                    }

                    if (typeMatch && glm::distance(position, poi.position) < minDistance)
                    {
                        return false;
                    }
                }

                return true;
            }

            case ConstraintType::NoiseThreshold:
            {
                // Requires params: [theshold, cutoff]
                if (constraint.params.size() < 2)
                {
                    return true; // Invalid constraint params, allow placement
                }

                if (!context.noiseGenerator)
                {
                    return true; // No noise generator, allow placement
                }

                float threshold = constraint.params[0];
                bool aboveThreshold = constraint.params[1] > 0.0f;

                float noiseValue = context.noiseGenerator->generate(position);

                return aboveThreshold ? (noiseValue >= threshold) : (noiseValue <= threshold);
            }

            case ConstraintType::Density:
            {
                // Requires params: [radius, maxCount]
                if (constraint.params.size() < 2)
                {
                    return true; // Invalid constraint params, allow placement
                }

                float radius = constraint.params[0];
                int maxCount = static_cast<int>(constraint.params[1]);

                // Convert world position to chunk space
                ChunkCoord chunkCoord = ChunkCoord::fromWorldPosition(position, context.chunkSize);

                // Check already placed features in this and neighboring chunks
                int count = 0;

                // Get chunks that could contain features within radius
                int chunkRadius = static_cast<int>(std::ceil(radius / context.chunkSize)) + 1;

                for (int dx = -chunkRadius; dx <= chunkRadius; dx++)
                {
                    for (int dy = -chunkRadius; dy <= chunkRadius; dy++)
                    {
                        for (int dz = -chunkRadius; dz <= chunkRadius; dz++)
                        {
                            ChunkCoord neighborCoord(chunkCoord.x + dx, chunkCoord.y + dy, chunkCoord.z + dz);

                            auto chunkIt = m_placedFeatures.find(neighborCoord);
                            if (chunkIt == m_placedFeatures.end())
                            {
                                continue;
                            }

                            // Count all features within radius
                            for (const auto& [_, positions] : chunkIt->second)
                            {
                                for (const auto& featurePos : positions)
                                {
                                    if (glm::distance(position, featurePos) <= radius)
                                    {
                                        count++;

                                        if (count >= maxCount)
                                        {
                                            return false;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                return true;
            }

            case ConstraintType::Custom:
                // Custom constraints are handled separately
                return true;

            default:
                return true;
        }
    }

    std::vector<glm::vec3> FeaturePlacement::generatePotentialPositions(const ChunkCoord& chunkCoord,
                                                                        int count,
                                                                        const GenerationContext& context) const
    {
        std::vector<glm::vec3> positions;
        positions.reserve(count);

        // Set up RNG with seed derived from chunk coords and global seed
        uint32_t chunkSeed = m_seed;
        chunkSeed = chunkSeed * 73856093 + chunkCoord.x * 19349663;
        chunkSeed = chunkSeed * 73856093 + chunkCoord.y * 83492791;
        chunkSeed = chunkSeed * 73856093 + chunkCoord.z * 25982993;

        std::mt19937 rng(chunkSeed);
        std::uniform_real_distribution<float> distChunk(0.0f, static_cast<float>(context.chunkSize));

        // Calculate chunk world position
        glm::vec3 chunkWorldPos = chunkCoord.toWorldPosition(context.chunkSize);

        for (int i = 0; i < count; i++)
        {
            // Generate random position within the chunk
            float x = distChunk(rng);
            float y = distChunk(rng);
            float z = distChunk(rng);

            // Convert to world space
            glm::vec3 worldPos = chunkWorldPos + glm::vec3(x, y, z);

            positions.push_back(worldPos);
        }

        return positions;
    }

    bool FeaturePlacement::isNearExistingFeature(const glm::vec3& position, const std::string& featureType, float spacing) const
    {
        // Convert world position to chunk space
        ChunkCoord chunkCoord = ChunkCoord::fromWorldPosition(position, 16); // Default chunk size

        // Check already placed features in this and neighboring chunks
        int chunkRadius = static_cast<int>(std::ceil(spacing / 16.0f)) + 1;

        for (int dx = -chunkRadius; dx <= chunkRadius; dx++)
        {
            for (int dy = -chunkRadius; dy <= chunkRadius; dy++)
            {
                for (int dz = -chunkRadius; dz <= chunkRadius; dz++)
                {
                    ChunkCoord neighborCoord(chunkCoord.x + dx, chunkCoord.y + dy, chunkCoord.z + dz);

                    auto chunkIt = m_placedFeatures.find(neighborCoord);
                    if (chunkIt == m_placedFeatures.end())
                    {
                        continue;
                    }

                    // Check if the same feature type is placed nearby
                    auto featureIt = chunkIt->second.find(featureType);
                    if (featureIt == chunkIt->second.end())
                    {
                        continue;
                    }

                    // Check distance to each placed feature
                    for (const auto& featurePos : featureIt->second)
                    {
                        if (glm::distance(position, featurePos) < spacing)
                        {
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

} // namespace PixelCraft::Voxel