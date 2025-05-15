// -------------------------------------------------------------------------
// DistributionControl.cpp
// -------------------------------------------------------------------------
#include "Voxel/DistributionControl.h"
#include "Core/Logger.h"
#include "Utility/Random.h"
#include "Utility/Math.h"
#include "ECS/Serializer.h"
#include "ECS/Deserializer.h"

#include <algorithm>
#include <random>
#include <cmath>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Voxel
{

    DistributionControl::DistributionControl()
    {
        // Nothing specific to initialize
    }

    DistributionControl::~DistributionControl()
    {
        // Clean up any resources
    }

    void DistributionControl::setPattern(PatternType pattern)
    {
        m_pattern = pattern;
    }

    DistributionControl::PatternType DistributionControl::getPattern() const
    {
        return m_pattern;
    }

    void DistributionControl::setGlobalDensity(float density)
    {
        m_globalDensity = std::max(0.0f, std::min(1.0f, density));
    }

    float DistributionControl::getGlobalDensity() const
    {
        return m_globalDensity;
    }

    void DistributionControl::setFeatureDensity(const std::string& featureType, float density)
    {
        m_featureDensities[featureType] = std::max(0.0f, std::min(1.0f, density));
    }

    float DistributionControl::getFeatureDensity(const std::string& featureType) const
    {
        auto it = m_featureDensities.find(featureType);
        if (it != m_featureDensities.end())
        {
            return it->second;
        }

        return m_globalDensity;
    }

    void DistributionControl::setInfluenceRadius(float radius)
    {
        m_influenceRadius = std::max(0.0f, radius);
    }

    float DistributionControl::getInfluenceRadius() const
    {
        return m_influenceRadius;
    }

    void DistributionControl::setClusterSize(float size)
    {
        m_clusterSize = std::max(0.1f, std::min(10.0f, size));
    }

    float DistributionControl::getClusterSize() const
    {
        return m_clusterSize;
    }

    void DistributionControl::setClusterSpacing(float spacing)
    {
        m_clusterSpacing = std::max(0.0f, spacing);
    }

    float DistributionControl::getClusterSpacing() const
    {
        return m_clusterSpacing;
    }

    void DistributionControl::setClusterFalloff(float falloff)
    {
        m_clusterFalloff = std::max(0.0f, std::min(1.0f, falloff));
    }

    float DistributionControl::getClusterFalloff() const
    {
        return m_clusterFalloff;
    }

    void DistributionControl::setSeed(uint32_t seed)
    {
        m_seed = seed;
    }

    uint32_t DistributionControl::getSeed() const
    {
        return m_seed;
    }

    float DistributionControl::getDensityAt(const glm::vec3& position, const std::string& featureType) const
    {
        // Calculate base density
        float density = calculateBaseDensity(position, featureType);

        // Apply region modifiers
        density = applyRegionModifiers(position, density, featureType);

        // Apply cluster effects
        density = applyClusterEffect(position, density, featureType);

        // Apply gradient effects
        density = applyGradientEffect(position, density, featureType);

        // Apply custom distribution function if set
        if (!m_activeCustomDistribution.empty())
        {
            auto it = m_customDistributions.find(m_activeCustomDistribution);
            if (it != m_customDistributions.end())
            {
                density = it->second(position, m_seed);
            }
        }

        // Ensure result is in valid range
        return std::max(0.0f, std::min(1.0f, density));
    }

    int DistributionControl::addDensityRegion(const glm::vec3& centerPosition, float radius, float multiplier,
                                              const std::string& featureType)
    {
        DensityRegion region;
        region.id = m_nextRegionId++;
        region.position = centerPosition;
        region.radius = radius;
        region.multiplier = multiplier;
        region.featureType = featureType;

        m_densityRegions.push_back(region);
        return region.id;
    }

    bool DistributionControl::removeDensityRegion(int regionId)
    {
        auto it = std::find_if(m_densityRegions.begin(), m_densityRegions.end(),
                               [regionId](const DensityRegion& region) { return region.id == regionId; });

        if (it == m_densityRegions.end())
        {
            return false;
        }

        m_densityRegions.erase(it);
        return true;
    }

    int DistributionControl::addCluster(const glm::vec3& position, float strength, const std::string& featureType)
    {
        Cluster cluster;
        cluster.id = m_nextClusterId++;
        cluster.position = position;
        cluster.strength = std::max(0.0f, std::min(1.0f, strength));
        cluster.featureType = featureType;

        m_clusters.push_back(cluster);
        return cluster.id;
    }

    bool DistributionControl::removeCluster(int clusterId)
    {
        auto it = std::find_if(m_clusters.begin(), m_clusters.end(),
                               [clusterId](const Cluster& cluster) { return cluster.id == clusterId; });

        if (it == m_clusters.end())
        {
            return false;
        }

        m_clusters.erase(it);
        return true;
    }

    void DistributionControl::clearClusters()
    {
        m_clusters.clear();
        m_nextClusterId = 0;
    }

    void DistributionControl::setDensityThreshold(float threshold, const std::string& featureType)
    {
        m_densityThresholds[featureType] = std::max(0.0f, std::min(1.0f, threshold));
    }

    float DistributionControl::getDensityThreshold(const std::string& featureType) const
    {
        auto it = m_densityThresholds.find(featureType);
        if (it != m_densityThresholds.end())
        {
            return it->second;
        }

        return 0.0f;
    }

    std::vector<glm::vec3> DistributionControl::generateDistributionPoints(const ChunkCoord& chunkCoord,
                                                                           int maxPoints,
                                                                           const std::string& featureType) const
    {
        // Generate points based on the selected pattern
        std::vector<glm::vec3> points;

        switch (m_pattern)
        {
            case PatternType::Uniform:
                points = generatePointsUniform(chunkCoord, maxPoints);
                break;

            case PatternType::Clustered:
                points = generatePointsClustered(chunkCoord, maxPoints);
                break;

            case PatternType::Stratified:
                points = generatePointsStratified(chunkCoord, maxPoints);
                break;

            case PatternType::Voronoi:
                points = generatePointsVoronoi(chunkCoord, maxPoints);
                break;

            case PatternType::BlueNoise:
                points = generatePointsBlueNoise(chunkCoord, maxPoints);
                break;

            case PatternType::Fibonacci:
                points = generatePointsFibonacci(chunkCoord, maxPoints);
                break;

            default:
                points = generatePointsUniform(chunkCoord, maxPoints);
                break;
        }

        // Filter points based on density threshold
        if (!featureType.empty())
        {
            // Get density threshold for this feature type
            float threshold = getDensityThreshold(featureType);

            if (threshold > 0.0f)
            {
                std::vector<glm::vec3> filteredPoints;
                filteredPoints.reserve(points.size());

                for (const auto& point : points)
                {
                    float density = getDensityAt(point, featureType);
                    if (density >= threshold)
                    {
                        filteredPoints.push_back(point);
                    }
                }

                return filteredPoints;
            }
        }

        return points;
    }

    void DistributionControl::setNoiseDistribution(const std::string& noiseLayerId, float threshold,
                                                   const std::string& featureType)
    {
        NoiseDistribution dist;
        dist.noiseLayerId = noiseLayerId;
        dist.threshold = threshold;

        m_noiseDistributions[featureType] = dist;
    }

    std::string DistributionControl::getNoiseDistribution(const std::string& featureType) const
    {
        auto it = m_noiseDistributions.find(featureType);
        if (it != m_noiseDistributions.end())
        {
            return it->second.noiseLayerId;
        }

        return "";
    }

    float DistributionControl::getNoiseThreshold(const std::string& featureType) const
    {
        auto it = m_noiseDistributions.find(featureType);
        if (it != m_noiseDistributions.end())
        {
            return it->second.threshold;
        }

        return 0.0f;
    }

    int DistributionControl::setGradient(const glm::vec3& startPosition, const glm::vec3& endPosition,
                                         float startDensity, float endDensity,
                                         const std::string& featureType)
    {
        Gradient gradient;
        gradient.id = m_nextGradientId++;
        gradient.startPosition = startPosition;
        gradient.endPosition = endPosition;
        gradient.startDensity = startDensity;
        gradient.endDensity = endDensity;
        gradient.featureType = featureType;

        m_gradients.push_back(gradient);
        return gradient.id;
    }

    bool DistributionControl::removeGradient(int gradientId)
    {
        auto it = std::find_if(m_gradients.begin(), m_gradients.end(),
                               [gradientId](const Gradient& gradient) { return gradient.id == gradientId; });

        if (it == m_gradients.end())
        {
            return false;
        }

        m_gradients.erase(it);
        return true;
    }

    bool DistributionControl::registerCustomDistribution(const std::string& name,
                                                         std::function<float(const glm::vec3&, uint32_t)> distributionFunc)
    {
        if (name.empty() || !distributionFunc)
        {
            Log::error("Distribution name or function cannot be empty");
            return false;
        }

        // Check if already registered
        if (m_customDistributions.find(name) != m_customDistributions.end())
        {
            Log::warn("Custom distribution '" + name + "' already registered");
            return false;
        }

        m_customDistributions[name] = distributionFunc;
        return true;
    }

    bool DistributionControl::setActiveCustomDistribution(const std::string& name)
    {
        if (name.empty())
        {
            m_activeCustomDistribution.clear();
            return true;
        }

        auto it = m_customDistributions.find(name);
        if (it == m_customDistributions.end())
        {
            Log::error("Custom distribution '" + name + "' not found");
            return false;
        }

        m_activeCustomDistribution = name;
        return true;
    }

    std::string DistributionControl::getActiveCustomDistribution() const
    {
        return m_activeCustomDistribution;
    }

    void DistributionControl::serialize(ECS::Serializer& serializer) const
    {
        // Serialize basic properties
        serializer.write(static_cast<int>(m_pattern));
        serializer.write(m_globalDensity);
        serializer.write(m_influenceRadius);
        serializer.write(m_clusterSize);
        serializer.write(m_clusterSpacing);
        serializer.write(m_clusterFalloff);
        serializer.write(m_seed);
        serializer.writeString(m_activeCustomDistribution);

        // Serialize feature densities
        serializer.beginArray("featureDensitites", m_featureDensities.size());
        for (const auto& [type, density] : m_featureDensities)
        {
            serializer.write(density);
        }
        serializer.endArray();

        // Serialize density regions
        serializer.beginArray("densityRegions", m_densityRegions.size());
        for (size_t i = 0; i < m_densityRegions.size(); i++)
        {
            const auto& region = m_densityRegions[i];

            serializer.write(region.id);
            serializer.writeVec3(region.position);
            serializer.write(region.radius);
            serializer.write(region.multiplier);
            serializer.writeString(region.featureType);
        }
        serializer.endArray();

        // Serialize clusters
        serializer.beginArray("clusters", m_clusters.size());
        for (size_t i = 0; i < m_clusters.size(); i++)
        {
            const auto& cluster = m_clusters[i];

            serializer.write(cluster.id);
            serializer.writeVec3(cluster.position);
            serializer.write(cluster.strength);
            serializer.writeString(cluster.featureType);
        }
        serializer.endArray();

        // Serialize gradients
        serializer.beginArray("gradients", m_gradients.size());
        for (size_t i = 0; i < m_gradients.size(); i++)
        {
            const auto& gradient = m_gradients[i];

            serializer.write(gradient.id);
            serializer.writeVec3(gradient.startPosition);
            serializer.writeVec3(gradient.endPosition);
            serializer.write(gradient.startDensity);
            serializer.write(gradient.endDensity);
            serializer.writeString(gradient.featureType);
        }
        serializer.endArray();

        // Serialize noise distributions
        serializer.beginArray("noiseDistributions", m_noiseDistributions.size());
        for (const auto& [type, dist] : m_noiseDistributions)
        {
            serializer.writeString(type);
            serializer.writeString(dist.noiseLayerId);
            serializer.write(dist.threshold);
        }
        serializer.endArray();

        // Serialize density thresholds
        serializer.beginArray("densityThresholds", m_densityThresholds.size());
        for (const auto& [type, threshold] : m_densityThresholds)
        {
            serializer.writeString(type);
            serializer.write(threshold);
        }
        serializer.endArray();

        // Custom distribution functions cannot be serialized
        // Just serialize the names for reference
        serializer.beginArray("customDistributionNames", m_customDistributions.size());
        for (const auto& [name, _] : m_customDistributions)
        {
            serializer.writeString(name);
        }
        serializer.endArray();
    }

    void DistributionControl::deserialize(ECS::Deserializer& deserializer)
    {
        // Clear existing data
        m_densityRegions.clear();
        m_clusters.clear();
        m_gradients.clear();
        m_noiseDistributions.clear();
        m_densityThresholds.clear();
        m_featureDensities.clear();
        m_activeCustomDistribution.clear();

        // Deserialize basic properties
        int pattern;
        deserializer.read(pattern);
        m_pattern = static_cast<PatternType>(pattern);

        deserializer.read(m_globalDensity);
        deserializer.read(m_influenceRadius);
        deserializer.read(m_clusterSize);
        deserializer.read(m_clusterSpacing);
        deserializer.read(m_clusterFalloff);
        deserializer.read(m_seed);
        deserializer.readString(m_activeCustomDistribution);

        size_t arraySize;

        // Deserialize feature densities
        deserializer.beginArray("featureDensitites", arraySize);
        for (int i = 0; i < arraySize; i++)
        {
            std::string type;
            deserializer.readString(type);

            float density;
            deserializer.read(density);

            m_featureDensities[type] = density;
        }
        deserializer.endArray();

        // Deserialize density regions
        deserializer.beginArray("densityRegions", arraySize);
        for (size_t i = 0; i < m_densityRegions.size(); i++)
        {
            DensityRegion region;

            deserializer.read(region.id);
            deserializer.readVec3(region.position);
            deserializer.read(region.radius);
            deserializer.read(region.multiplier);
            deserializer.readString(region.featureType);

            m_densityRegions.push_back(region);
            m_nextRegionId = std::max(m_nextRegionId, region.id + 1);
        }
        deserializer.endArray();

        // Deserialize clusters
        deserializer.beginArray("clusters", arraySize);
        for (int i = 0; i < arraySize; i++)
        {
            Cluster cluster;

            deserializer.read(cluster.id);
            deserializer.readVec3(cluster.position);
            deserializer.read(cluster.strength);
            deserializer.readString(cluster.featureType);

            m_clusters.push_back(cluster);
        }
        deserializer.endArray();

        // Deserialize gradients
        deserializer.beginArray("gradients", arraySize);
        for (int i = 0; i < arraySize; i++)
        {
            Gradient gradient;

            deserializer.read(gradient.id);
            deserializer.readVec3(gradient.startPosition);
            deserializer.readVec3(gradient.endPosition);
            deserializer.read(gradient.startDensity);
            deserializer.read(gradient.endDensity);
            deserializer.readString(gradient.featureType);

            m_gradients.push_back(gradient);
            m_nextGradientId = std::max(m_nextGradientId, gradient.id + 1);
        }
        deserializer.endArray();

        // Deserialize noise distributions
        deserializer.beginArray("noiseDistributions", arraySize);
        for (int i = 0; i < arraySize; i++)
        {
            std::string type;
            deserializer.readString(type);

            NoiseDistribution dist;
            deserializer.readString(dist.noiseLayerId);
            deserializer.read(dist.threshold);

            m_noiseDistributions[type] = dist;
        }
        deserializer.endArray();

        // Deserialize density thresholds
        deserializer.beginArray("densityThresholds", arraySize);
        for (int i = 0; i < arraySize; i++)
        {
            std::string type;
            deserializer.readString(type);

            float threshold;
            deserializer.read(threshold);

            m_densityThresholds[type] = threshold;
        }
        deserializer.endArray();

        // Custom distribution functions need to be registered at runtime
        // because they cannot be serialized
    }

    // Private helper methods

    float DistributionControl::calculateBaseDensity(const glm::vec3& position, const std::string& featureType) const
    {
        float density = m_globalDensity;

        // Apply feature-specific density if available
        if (!featureType.empty())
        {
            auto it = m_featureDensities.find(featureType);
            if (it != m_featureDensities.end())
            {
                density = it->second;
            }
        }

        return density;
    }

    float DistributionControl::applyRegionModifiers(const glm::vec3& position, float baseDensity, const std::string& featureType) const
    {
        float density = baseDensity;

        // Apply density region modifiers
        for (const auto& region : m_densityRegions)
        {
            // Skip if not applicable to this feature type
            if (!region.featureType.empty() && region.featureType != featureType)
            {
                continue;
            }

            float distance = glm::distance(position, region.position);
            if (distance <= region.radius)
            {
                // Linear falloff from center to edge
                float falloff = 1.0f - (distance / region.radius);

                // Apply region multiplier
                density *= 1.0f + (region.multiplier - 1.0f) * falloff;
            }
        }

        return density;
    }

    float DistributionControl::applyClusterEffect(const glm::vec3& position, float density, const std::string& featureType) const
    {
        float result = density;

        // Apply cluster effects
        for (const auto& cluster : m_clusters)
        {
            // Skip if not applicable to this feature type
            if (!cluster.featureType.empty() && cluster.featureType != featureType)
            {
                continue;
            }

            float distance = glm::distance(position, cluster.position);
            float clusterRadius = m_clusterSize * m_influenceRadius;

            if (distance <= clusterRadius)
            {
                // Calculate falloff
                float falloff = 1.0f - std::pow(distance / clusterRadius, m_clusterFalloff);

                // Increase density based on cluster strength
                result = std::max(result, cluster.strength * falloff);
            }
        }

        return result;
    }

    float DistributionControl::applyGradientEffect(const glm::vec3& position, float density, const std::string& featureType) const
    {
        float result = density;

        // Apply gradient effects
        for (const auto& gradient : m_gradients)
        {
            // Skip if not applicable to this feature type
            if (!gradient.featureType.empty() && gradient.featureType != featureType)
            {
                continue;
            }
            
            // Calculate distance along gradient line
            glm::vec3 gradientDir = gradient.endPosition - gradient.startPosition;
            float gradientLength = glm::length(gradientDir);

            if (gradientLength < 0.0001f)
            {
                continue; // Skip zero-length gradients
            }

            gradientDir = glm::normalize(gradientDir);

            // Project position onto gradient line
            glm::vec3 toPos = position - gradient.startPosition;
            float projectionDist = glm::dot(toPos, gradientDir);

            // Calculate how far along the gradient we are [0-1]
            float gradientT = glm::clamp(projectionDist / gradientLength, 0.0f, 1.0f);

            // Interpolate between start and end densisies
            float gradientDensity = glm::mix(gradient.startDensity, gradient.endDensity, gradientT);

            // Blend with current density (stronger wins)
            result = std::max(result, gradientDensity);
        }

        return result;
    }

    std::vector<glm::vec3> DistributionControl::generatePointsUniform(const ChunkCoord& chunkCoord, int count) const
    {
        std::vector<glm::vec3> points;
        points.reserve(count);

        // Set up RNG with seed derived from chunk coords and global seed
        uint32_t chunkSeed = m_seed;
        chunkSeed = chunkSeed * 73856093 + chunkCoord.x * 19349663;
        chunkSeed = chunkSeed * 73856093 + chunkCoord.y * 83492791;
        chunkSeed = chunkSeed * 73856093 + chunkCoord.z * 25982993;

        std::mt19937 rng(chunkSeed);
        std::uniform_real_distribution<float> distChunk(0.0f, 16.0f); // Assuming 16x16x16 chunks

        // Calculate chunk world position
        glm::vec3 chunkWorldPos = chunkCoord.toWorldPosition(16); // Assuming 16x16x16 chunks

        for (int i = 0; i < count; i++)
        {
            // Generate random position within the chunk
            float x = distChunk(rng);
            float y = distChunk(rng);
            float z = distChunk(rng);

            // Convert to world space
            glm::vec3 worldPos = chunkWorldPos + glm::vec3(x, y, z);

            points.push_back(worldPos);
        }

        return points;
    }

    std::vector<glm::vec3> DistributionControl::generatePointsClustered(const ChunkCoord& chunkCoord, int count) const
    {
        std::vector<glm::vec3> points;
        points.reserve(count);

        // Set up RNG with seed derived from chunk coords and global seed
        uint32_t chunkSeed = m_seed;
        chunkSeed = chunkSeed * 73856093 + chunkCoord.x * 19349663;
        chunkSeed = chunkSeed * 73856093 + chunkCoord.y * 83492791;
        chunkSeed = chunkSeed * 73856093 + chunkCoord.z * 25982993;

        std::mt19937 rng(chunkSeed);
        std::uniform_real_distribution<float> distChunk(0.0f, 16.0f); // Assuming 16x16x16 chunks
        std::uniform_real_distribution<float> distCluster(0.0f, m_clusterSize * 8.0f);
        std::uniform_int_distribution<int> distClusterCount(1, 3);

        // Calculate chunk world position
        glm::vec3 chunkWorldPos = chunkCoord.toWorldPosition(16); // Assuming 16x16x16 chunks

        // Generate cluster centers
        int clusterCount = distClusterCount(rng);
        std::vector<glm::vec3> clusterCenters;

        for (int i = 0; i < clusterCount; i++)
        {
            float x = distChunk(rng);
            float y = distChunk(rng);
            float z = distChunk(rng);

            clusterCenters.push_back(chunkWorldPos + glm::vec3(x, y, z));
        }

        // Generate points around cluster centers
        int pointsPerCluster = count / clusterCount;

        for (int i = 0; i < clusterCount; i++)
        {
            for (int j = 0; j < pointsPerCluster; j++)
            {
                // Generate point with radial distribution around cluster center
                float angle = distChunk(rng) * glm::two_pi<float>();
                float distance = distCluster(rng);

                float dx = std::cos(angle) * distance;
                float dz = std::sin(angle) * distance;
                float dy = distCluster(rng) - distCluster(rng); // More vertical spread in the middle

                glm::vec3 offset(dx, dy, dz);
                glm::vec3 worldPos = clusterCenters[i] + offset;

                points.push_back(worldPos);
            }
        }

        // Add remaining points
        int remaining = count - (pointsPerCluster * clusterCount);
        for (int i = 0; i < remaining; i++)
        {
            int clusterIndex = i % clusterCount;

            // Generate point with radial distribution around cluster center
            float angle = distChunk(rng) * glm::two_pi<float>();
            float distance = distCluster(rng);

            float dx = std::cos(angle) * distance;
            float dz = std::sin(angle) * distance;
            float dy = distCluster(rng) - distCluster(rng);

            glm::vec3 offset(dx, dy, dz);
            glm::vec3 worldPos = clusterCenters[clusterIndex] + offset;

            points.push_back(worldPos);
        }

        return points;
    }

    std::vector<glm::vec3> DistributionControl::generatePointsStratified(const ChunkCoord& chunkCoord, int count) const
    {
        std::vector<glm::vec3> points;

        // Set up RNG with seed derived from chunk coords and global seed
        uint32_t chunkSeed = m_seed;
        chunkSeed = chunkSeed * 73856093 + chunkCoord.x * 19349663;
        chunkSeed = chunkSeed * 73856093 + chunkCoord.y * 83492791;
        chunkSeed = chunkSeed * 73856093 + chunkCoord.z * 25982993;

        std::mt19937 rng(chunkSeed);
        std::uniform_real_distribution<float> jitter(0.0f, 1.0f);

        // Calculate chunk world position
        glm::vec3 chunkWorldPos = chunkCoord.toWorldPosition(16); // Assuming 16x16x16 chunks

        // Determine grid size
        int gridSize = static_cast<int>(std::ceil(std::cbrt(count)));
        float cellSize = 16.0f / gridSize;

        // Generate one point per grid cell with jitter
        for (int x = 0; x < gridSize; x++)
        {
            for (int y = 0; y < gridSize; y++)
            {
                for (int z = 0; z < gridSize; z++)
                {
                    // Skip if we already have enough points
                    if (points.size() >= static_cast<size_t>(count))
                    {
                        break;
                    }

                    // Calculate base position
                    float baseX = x * cellSize;
                    float baseY = y * cellSize;
                    float baseZ = z * cellSize;

                    // Add jitter
                    float jitterX = jitter(rng) * cellSize;
                    float jitterY = jitter(rng) * cellSize;
                    float jitterZ = jitter(rng) * cellSize;

                    // Final position
                    float worldX = chunkWorldPos.x + baseX + jitterX;
                    float worldY = chunkWorldPos.y + baseY + jitterY;
                    float worldZ = chunkWorldPos.z + baseZ + jitterZ;

                    points.push_back(glm::vec3(worldX, worldY, worldZ));
                }
            }
        }

        return points;
    }

    std::vector<glm::vec3> DistributionControl::generatePointsVoronoi(const ChunkCoord& chunkCoord, int count) const
    {
        // For this simplified implementation, generate random points
        // that will be used as Voronoi cell centers
        std::vector<glm::vec3> cellCenters = generatePointsUniform(chunkCoord, count);

        // In a full implementation, we'd generate more points and select
        // those that are closest to Voronoi cell centers

        return cellCenters;
    }

    std::vector<glm::vec3> DistributionControl::generatePointsBlueNoise(const ChunkCoord& chunkCoord, int count) const
    {
        // Blue noise generation is complex, so this is a simplified version
        // that uses rejection sampling based on minimum distance
        std::vector<glm::vec3> points;

        // Set up RNG with seed derived from chunk coords and global seed
        uint32_t chunkSeed = m_seed;
        chunkSeed = chunkSeed * 73856093 + chunkCoord.x * 19349663;
        chunkSeed = chunkSeed * 73856093 + chunkCoord.y * 83492791;
        chunkSeed = chunkSeed * 73856093 + chunkCoord.z * 25982993;

        std::mt19937 rng(chunkSeed);
        std::uniform_real_distribution<float> distChunk(0.0f, 16.0f); // Assuming 16x16x16 chunks

        // Calculate chunk world position
        glm::vec3 chunkWorldPos = chunkCoord.toWorldPosition(16); // Assuming 16x16x16 chunks

        // Minimum distance between points (based on count)
        float minDist = 16.0f / std::cbrt(count) * 0.9f;

        // Maximum attempts before giving up on a point
        const int maxAttempts = 30;

        // Generate points
        int attempts = 0;
        while (points.size() < static_cast<size_t>(count) && attempts < maxAttempts * count)
        {
            // Generate candidate point
            float x = distChunk(rng);
            float y = distChunk(rng);
            float z = distChunk(rng);

            glm::vec3 candidate = chunkWorldPos + glm::vec3(x, y, z);

            // Check if it's far enough from existing points
            bool tooClose = false;
            for (const auto& point : points)
            {
                if (glm::distance(candidate, point) < minDist)
                {
                    tooClose = true;
                    break;
                }
            }

            if (!tooClose)
            {
                points.push_back(candidate);
            }

            attempts++;
        }

        // If we don't have enough points, just generate random ones
        while (points.size() < static_cast<size_t>(count))
        {
            float x = distChunk(rng);
            float y = distChunk(rng);
            float z = distChunk(rng);

            points.push_back(chunkWorldPos + glm::vec3(x, y, z));
        }

        return points;
    }

    std::vector<glm::vec3> DistributionControl::generatePointsFibonacci(const ChunkCoord& chunkCoord, int count) const
    {
        std::vector<glm::vec3> points;
        points.reserve(count);

        // Fibonacci spherical distribution constants
        const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f; // Golden ratio

        // Set up RNG with seed derived from chunk coords and global seed
        uint32_t chunkSeed = m_seed;
        chunkSeed = chunkSeed * 73856093 + chunkCoord.x * 19349663;
        chunkSeed = chunkSeed * 73856093 + chunkCoord.y * 83492791;
        chunkSeed = chunkSeed * 73856093 + chunkCoord.z * 25982993;

        std::mt19937 rng(chunkSeed);
        std::uniform_real_distribution<float> distRadius(0.0f, 8.0f); // Radius within chunk
        std::uniform_real_distribution<float> distOffset(0.0f, 16.0f); // Center position within chunk

        // Calculate chunk world position
        glm::vec3 chunkWorldPos = chunkCoord.toWorldPosition(16); // Assuming 16x16x16 chunks

        // Generate random center within chunk
        float centerX = distOffset(rng);
        float centerY = distOffset(rng);
        float centerZ = distOffset(rng);

        glm::vec3 center = chunkWorldPos + glm::vec3(centerX, centerY, centerZ);

        for (int i = 0; i < count; i++)
        {
            // Generate point using Fibonacci spherical distribution
            float y = 1.0f - (2.0f * i) / (count - 1);
            float radius = distRadius(rng);

            float theta = 2.0f * glm::pi<float>() * i * phi;

            float x = std::sqrt(1.0f - y * y) * std::cos(theta);
            float z = std::sqrt(1.0f - y * y) * std::sin(theta);

            // Scale by radius and add center
            glm::vec3 point = center + glm::vec3(x, y, z) * radius;

            // Ensure point is within chunk bounds
            glm::vec3 minBound = chunkWorldPos;
            glm::vec3 maxBound = chunkWorldPos + glm::vec3(16.0f);

            point.x = glm::clamp(point.x, minBound.x, maxBound.x);
            point.y = glm::clamp(point.y, minBound.y, maxBound.y);
            point.z = glm::clamp(point.z, minBound.z, maxBound.z);

            points.push_back(point);
        }

        return points;
    }

} // namespace PixelCraft::Voxel