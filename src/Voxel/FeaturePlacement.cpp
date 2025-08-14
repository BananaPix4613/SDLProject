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
                                           const GenerationContext& context, const std::shared_ptr<Grid> grid) const
    {
        // Check if feature type is registered
        auto it = m_featureTypes.find(featureType);
        if (it == m_featureTypes.end())
        {
            Core::error("Feature type '" + featureType + "' not registered");
            return false;
        }

        auto& feature = it->second;

        // Check spacing from existing features
        if (feature.spacing > 0.0f && isNearExistingFeature(position, featureType, feature.spacing))
        {
            return false;
        }

        // Check standard constraints
        for (const auto& constraint : feature.constraints)
        {
            if (!checkConstraint(constraint, position, context, grid))
            {
                return false;
            }
        }

        // Check custom constraints
        for (const auto& customConstraint : feature.customConstraints)
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
            Core::error("Feature type '" + featureType + "' not registered");
            return false;
        }

        // Get the feature generator function
        const auto& generator = it->second.generator;
        if (!generator)
        {
            Core::error("No generator function for feature type '" + featureType + "'");
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
                                                                   const std::shared_ptr<Grid> grid) const
    {
        // Check if feature type is registered
        auto it = m_featureTypes.find(featureType);
        if (it == m_featureTypes.end())
        {
            Core::error("Feature type '" + featureType + "' not registered");
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
            Core::error("Feature type name cannot be empty");
            return false;
        }

        if (!generator)
        {
            Core::error("Generator function cannot be null");
            return false;
        }

        // Check if already registered
        if (m_featureTypes.find(featureType) != m_featureTypes.end())
        {
            Core::warn("Feature type '" + featureType + "' already registered");
            return false;
        }

        // Create new feature type info
        FeatureType type;
        type.generator = generator;

        m_featureTypes[featureType] = type;
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
            Core::error("Could not add constraint to missing feature type '" + featureType + "'");
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
            Core::error("Could not remove constraint from missing feature type '" + featureType + "'");
            return false;
        }

        auto& constraints = it->second.constraints;
        auto constraintIt = std::find_if(constraints.begin(), constraints.end(),
                                         [constraint](const FeatureConstraint& c) { return c.type == constraint; });

        if (constraintIt == constraints.end())
        {
            Core::warn("Constraint not found for feature type '" + featureType + "'");
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
            Core::error("Could not clear constraint from missing feature type '" + featureType + "'");
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
            Core::error("Could not get constraints from missing feature type '" + featureType + "'");
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
            Core::error("Could not get constraint params from missing feature type '" + featureType + "'");
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
        std::function<bool(const glm::vec3&, const GenerationContext&, const std::shared_ptr<Grid>)> constraint)
    {
        if (name.empty() || !constraint)
        {
            Core::error("Constraint name or function cannot be empty");
            return false;
        }

        auto it = m_featureTypes.find(featureType);
        if (it == m_featureTypes.end())
        {
            Core::error("Could not add custom constraint to missing feature type '" + featureType + "'");
            return false;
        }

        // Check if constraint already exists
        auto& customConstraints = it->second.customConstraints;
        auto existingIt = std::find_if(customConstraints.begin(), customConstraints.end(),
                                       [&name](const CustomConstraintFunc& c) { return c.name == name; });

        if (existingIt != customConstraints.end())
        {
            Core::warn("Custom constraint '" + name + "' already exists for feature type '" + featureType + "'");
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
            Core::error("Could not remove custom constraint from missing feature type '" + featureType + "'");
            return false;
        }

        auto& customConstraints = it->second.customConstraints;
        auto constraintIt = std::find_if(customConstraints.begin(), customConstraints.end(),
                                         [&name](const CustomConstraintFunc& c) { return c.name == name; });

        if (constraintIt == customConstraints.end())
        {
            Core::warn("Custom constraint '" + name + "' not found for feature type '" + featureType + "'");
            return false;
        }

        customConstraints.erase(constraintIt);
        return true;
    }

    bool FeaturePlacement::setFeatureSpacing(const std::string& featureType, float spacing)
    {
        auto it = m_featureTypes.find(featureType);
        if (it == m_featureTypes.end())
        {
            Core::error("Could not set spacing to missing feature type '" + featureType + "'");
            return false;
        }

        it->second.spacing = spacing;
        return true;
    }

    float FeaturePlacement::getFeatureSpacing(const std::string& featureType) const
    {
        auto it = m_featureTypes.find(featureType);
        if (it == m_featureTypes.end())
        {
            Core::error("Could not get spacing from missing feature type '" + featureType + "'");
            return 0.0f;
        }

        return it->second.spacing;
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

    void FeaturePlacement::setSeed(uint32_t seed)
    {
        m_seed = seed;
    }

    uint32_t FeaturePlacement::getSeed() const
    {
        return m_seed;
    }

    Utility::Serialization::SerializationResult FeaturePlacement::serialize(Utility::Serialization::Serializer& serializer) const
    {
        auto result = serializer.beginObject(getTypeName());
        if (!result) return result;

        // Serialize basic properties
        result =  serializer.writeField("seed", m_seed);
        if (!result) return result;

        // Serialize feature types
        result = serializer.beginArray("featureTypes", m_featureTypes.size());
        if (!result) return result;

        for (const auto& [typeName, typeInfo] : m_featureTypes)
        {
            // Begin serializing the entity
            result = serializer.beginObject("Feature");
            if (!result) return result;

            result = serializer.writeField("name", typeName);
            if (!result) return result;

            result = serializer.writeField("spacing", typeInfo.spacing);
            if (!result) return result;

            // Serialize constraints as an array
            result = serializer.beginArray("constraints", typeInfo.constraints.size());
            if (!result) return result;

            for (size_t i = 0; i < typeInfo.constraints.size(); i++)
            {
                const auto& constraint = typeInfo.constraints[i];
                result = serializer.beginObject("Constraint");
                if (!result) return result;

                result = serializer.writeField("type", static_cast<int>(constraint.type));
                if (!result) return result;

                // Serialize params
                result = serializer.beginArray("params", constraint.params.size());
                if (!result) return result;

                for (size_t j = 0; j < constraint.params.size(); j++)
                {
                    result = serializer.writeField("param", constraint.params[j]);
                    if (!result) return result;
                }

                result = serializer.endArray();
                if (!result) return result;

                result = serializer.endObject();
                if (!result) return result;
            }

            result = serializer.endArray();
            if (!result) return result;

            // Serialize custom constraints as an array
            result = serializer.beginArray("customConstraints", typeInfo.customConstraints.size());
            if (!result) return result;

            for (size_t i = 0; i < typeInfo.customConstraints.size(); i++)
            {
                result = serializer.writeField("name", typeInfo.customConstraints[i].name);
            }

            result = serializer.endArray();
            if (!result) return result;

            result = serializer.endObject();
            if (!result) return result;
        }

        result = serializer.endArray();
        if (!result) return result;

        // Serialize points of interest
        result = serializer.beginArray("pointsOfInterest", m_pointsOfInterest.size());
        if (!result) return result;

        for (size_t i = 0; i < m_pointsOfInterest.size(); i++)
        {
            const auto& poi = m_pointsOfInterest[i];
            result = serializer.beginObject("POI");
            if (!result) return result;

            result = serializer.writeField("position", poi.position);
            if (!result) return result;

            result = serializer.writeField("type", poi.type);
            if (!result) return result;

            result = serializer.writeField("radius", poi.radius);
            if (!result) return result;

            result = serializer.endObject();
            if (!result) return result;
        }

        result = serializer.endArray();
        if (!result) return result;

        // Placed features are not serialized (runtime state)
        return serializer.endObject();
    }

    Utility::Serialization::SerializationResult FeaturePlacement::deserialize(Utility::Serialization::Deserializer& deserializer)
    {
        // Clear existing data
        m_featureTypes.clear();
        m_pointsOfInterest.clear();
        m_placedFeatures.clear();

        auto result = deserializer.beginObject(getTypeName());
        if (!result) return result;

        // Serialize basic properties
        result = deserializer.readField("seed", m_seed);
        if (!result) return result;

        // Serialize feature types
        size_t featureSize;
        result = deserializer.beginArray("featureTypes", featureSize);
        if (!result) return result;

        for (size_t i = 0; i < featureSize; i++)
        {
            std::string typeName;
            FeatureType typeInfo;

            // Begin deserializing the feature type
            result = deserializer.beginObject("FeatureType");
            if (!result) return result;

            result = deserializer.readField("name", typeName);
            if (!result) return result;

            result = deserializer.readField("spacing", typeInfo.spacing);
            if (!result) return result;

            size_t constraintsSize;
            // Deserialize constraints as an array
            result = deserializer.beginArray("constraints", constraintsSize);
            if (!result) return result;

            for (size_t j = 0; j < constraintsSize; j++)
            {
                FeatureConstraint constraint;
                result = deserializer.beginObject("Constraint");
                if (!result) return result;

                int32_t constraintType;
                result = deserializer.readField("type", constraintType);
                if (!result) return result;
                constraint.type = static_cast<ConstraintType>(constraintType);

                // Deserialize params
                size_t paramsSize;
                result = deserializer.beginArray("params", paramsSize);
                if (!result) return result;

                for (size_t k = 0; k < paramsSize; k++)
                {
                    result = deserializer.readField("param", constraint.params[k]);
                    if (!result) return result;
                }

                result = deserializer.endArray();
                if (!result) return result;

                result = deserializer.endObject();
                if (!result) return result;

                typeInfo.constraints[j] = constraint;
            }

            result = deserializer.endArray();
            if (!result) return result;

            // Deserialize custom constraints as an array (only func names
            result = deserializer.beginArray("customConstraints", constraintsSize);
            if (!result) return result;

            for (size_t j = 0; j < constraintsSize; j++)
            {
                result = deserializer.readField("name", typeInfo.customConstraints[j].name);
                if (!result) return result;
            }

            result = deserializer.endArray();
            if (!result) return result;

            m_featureTypes[typeName] = typeInfo;

            result = deserializer.endObject();
            if (!result) return result;
        }

        result = deserializer.endArray();
        if (!result) return result;

        // Serialize points of interest
        size_t poiSize;
        result = deserializer.beginArray("pointsOfInterest", poiSize);
        if (!result) return result;

        for (size_t i = 0; i < poiSize; i++)
        {
            PointOfInterest poi;
            result = deserializer.beginObject("POI");
            if (!result) return result;

            result = deserializer.readField("position", poi.position);
            if (!result) return result;

            result = deserializer.readField("type", poi.type);
            if (!result) return result;

            result = deserializer.readField("radius", poi.radius);
            if (!result) return result;

            result = deserializer.endObject();
            if (!result) return result;

            m_pointsOfInterest[i] = poi;
        }

        result = deserializer.endArray();
        if (!result) return result;

        // Feature types and constraints need to be registered at runtime
        // because generator and constraint functions cannot be serialized
        return deserializer.endObject();
    }

    void FeaturePlacement::defineSchema(Utility::Serialization::Schema& schema)
    {
        schema.addField("seed", Utility::Serialization::ValueType::UInt32, true);
        schema.addArrayField("featureTypes", Utility::Serialization::ValueType::Object, true);
        schema.addArrayField("pointsOfInterest", Utility::Serialization::ValueType::Object, true);
    }

    std::string FeaturePlacement::getTypeName()
    {
        return "FeaturePlacement";
    }

    // Private helper methods

    bool FeaturePlacement::checkConstraint(const FeatureConstraint& constraint, const glm::vec3& position,
                                           const GenerationContext& context, const std::shared_ptr<Grid> grid) const
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

                Voxel v1 = grid->getVoxel(p1);
                Voxel v2 = grid->getVoxel(p2);
                Voxel v3 = grid->getVoxel(p3);
                Voxel v4 = grid->getVoxel(p4);

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
                        Voxel voxel = grid->getVoxel(checkPos);

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
                        Voxel voxel = grid->getVoxel(checkPos);

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
                // Requires params: [threshold, cutoff]
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