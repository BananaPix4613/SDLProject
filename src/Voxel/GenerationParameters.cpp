// -------------------------------------------------------------------------
// GenerationParameters.cpp
// -------------------------------------------------------------------------
#include "Voxel/GenerationParameters.h"
#include "Core/Logger.h"

namespace Log = PixelCraft::Core;

namespace PixelCraft::Voxel
{

    GenerationParameters::GenerationParameters()
    {
        initializeDefaults();
    }

    GenerationParameters::~GenerationParameters()
    {
        // Nothing specific to clean up
    }

    void GenerationParameters::setTerrainMode(TerrainMode mode)
    {
        m_terrainMode = mode;
    }

    GenerationParameters::TerrainMode GenerationParameters::getTerrainMode() const
    {
        return m_terrainMode;
    }

    void GenerationParameters::setHeightRange(float min, float max)
    {
        if (min > max)
        {
            Log::warn("Min height greater than max height, swapping values");
            std::swap(min, max);
        }

        m_minHeight = min;
        m_maxHeight = max;
    }

    float GenerationParameters::getMinHeight() const
    {
        return m_minHeight;
    }

    float GenerationParameters::getMaxHeight() const
    {
        return m_maxHeight;
    }

    void GenerationParameters::setWaterLevel(float level)
    {
        m_waterLevel = level;
    }

    float GenerationParameters::getWaterLevel() const
    {
        return m_waterLevel;
    }

    void GenerationParameters::enableWater(bool enable)
    {
        m_waterEnabled = enable;
    }

    bool GenerationParameters::isWaterEnabled() const
    {
        return m_waterEnabled;
    }

    void GenerationParameters::enableCaves(bool enable)
    {
        m_cavesEnabled = enable;
    }

    bool GenerationParameters::areCavesEnabled() const
    {
        return m_cavesEnabled;
    }

    void GenerationParameters::setCaveDensity(float density)
    {
        m_caveDensity = std::max(0.0f, std::min(1.0f, density));
    }

    float GenerationParameters::getCaveDensity() const
    {
        return m_caveDensity;
    }

    void GenerationParameters::setCaveSize(float size)
    {
        m_caveSize = std::max(0.1f, std::min(10.0f, size));
    }

    float GenerationParameters::getCaveSize() const
    {
        return m_caveSize;
    }

    void GenerationParameters::setTerrainNoiseLayer(const std::string& layerId)
    {
        m_terrainNoiseLayer = layerId;
    }

    std::string GenerationParameters::getTerrainNoiseLayer() const
    {
        return m_terrainNoiseLayer;
    }

    void GenerationParameters::setCaveNoiseLayer(const std::string& layerId)
    {
        m_caveNoiseLayer = layerId;
    }

    std::string GenerationParameters::getCaveNoiseLayer() const
    {
        return m_caveNoiseLayer;
    }

    void GenerationParameters::setFeatureNoiseLayer(const std::string& layerId)
    {
        m_featureNoiseLayer = layerId;
    }

    std::string GenerationParameters::getFeatureNoiseLayer() const
    {
        return m_featureNoiseLayer;
    }

    void GenerationParameters::setBiomeNoiseLayer(const std::string& layerId)
    {
        m_biomeNoiseLayer = layerId;
    }

    std::string GenerationParameters::getBiomeNoiseLayer() const
    {
        return m_biomeNoiseLayer;
    }

    void GenerationParameters::setTerrainRoughness(float roughness)
    {
        m_terrainRoughness = std::max(0.0f, std::min(1.0f, roughness));
    }

    float GenerationParameters::getTerrainRoughness() const
    {
        return m_terrainRoughness;
    }

    void GenerationParameters::setTerrainErosion(float erosion)
    {
        m_terrainErosion = std::max(0.0f, std::min(1.0f, erosion));
    }

    float GenerationParameters::getTerrainErosion() const
    {
        return m_terrainErosion;
    }

    void GenerationParameters::setFeaturePlacementPattern(PlacementPattern pattern)
    {
        m_featurePlacementPattern = pattern;
    }

    GenerationParameters::PlacementPattern GenerationParameters::getFeaturePlacementPattern() const
    {
        return m_featurePlacementPattern;
    }

    void GenerationParameters::setFeatureDensity(float density)
    {
        m_featureDensity = std::max(0.0f, std::min(1.0f, density));
    }

    float GenerationParameters::getFeatureDensity() const
    {
        return m_featureDensity;
    }

    void GenerationParameters::setBiomeBlendDistance(float distance)
    {
        m_biomeBlendDistance = std::max(0.0f, distance);
    }

    float GenerationParameters::getBiomeBlendDistance() const
    {
        return m_biomeBlendDistance;
    }

    void GenerationParameters::setFeaturePlacementControl(const std::string& controlId)
    {
        m_featurePlacementControl = controlId;
    }

    std::string GenerationParameters::getFeaturePlacementControl() const
    {
        return m_featurePlacementControl;
    }

    void GenerationParameters::setDistributionControl(const std::string& controlId)
    {
        m_distributionControl = controlId;
    }

    std::string GenerationParameters::getDistributionControl() const
    {
        return m_distributionControl;
    }

    void GenerationParameters::enableFeatureType(const std::string& featureType, bool enable)
    {
        m_enabledFeatureTypes[featureType] = enable;
    }

    bool GenerationParameters::isFeatureTypeEnabled(const std::string& featureType) const
    {
        auto it = m_enabledFeatureTypes.find(featureType);
        if (it != m_enabledFeatureTypes.end())
        {
            return it->second;
        }

        // Default to enabled if not specifically set
        return true;
    }

    void GenerationParameters::setFeatureTypeDensity(const std::string& featureType, float density)
    {
        m_featureTypeDensities[featureType] = std::max(0.0f, std::min(1.0f, density));
    }

    float GenerationParameters::getFeatureTypeDensity(const std::string& featureType, float defaultValue) const
    {
        auto it = m_featureTypeDensities.find(featureType);
        if (it != m_featureTypeDensities.end())
        {
            return it->second;
        }

        return defaultValue;
    }

    void GenerationParameters::setFloatParam(const std::string& name, float value)
    {
        m_floatParams[name] = value;
    }

    float GenerationParameters::getFloatParam(const std::string& name, float defaultValue) const
    {
        auto it = m_floatParams.find(name);
        if (it != m_floatParams.end())
        {
            return it->second;
        }

        return defaultValue;
    }

    void GenerationParameters::setIntParam(const std::string& name, int value)
    {
        m_intParams[name] = value;
    }

    int GenerationParameters::getIntParam(const std::string& name, int defaultValue) const
    {
        auto it = m_intParams.find(name);
        if (it != m_intParams.end())
        {
            return it->second;
        }

        return defaultValue;
    }

    void GenerationParameters::setBoolParam(const std::string& name, bool value)
    {
        m_boolParams[name] = value;
    }

    bool GenerationParameters::getBoolParam(const std::string& name, bool defaultValue) const
    {
        auto it = m_boolParams.find(name);
        if (it != m_boolParams.end())
        {
            return it->second;
        }

        return defaultValue;
    }

    void GenerationParameters::setStringParam(const std::string& name, const std::string& value)
    {
        m_stringParams[name] = value;
    }

    std::string GenerationParameters::getStringParam(const std::string& name, const std::string& defaultValue) const
    {
        auto it = m_stringParams.find(name);
        if (it != m_stringParams.end())
        {
            return it->second;
        }

        return defaultValue;
    }

    void GenerationParameters::setVec3Param(const std::string& name, const glm::vec3& value)
    {
        m_vec3Params[name] = value;
    }

    glm::vec3 GenerationParameters::getVec3Param(const std::string& name, const glm::vec3& defaultValue) const
    {
        auto it = m_vec3Params.find(name);
        if (it != m_vec3Params.end())
        {
            return it->second;
        }

        return defaultValue;
    }

    bool GenerationParameters::applyPreset(const std::string& presetName)
    {
        // Reset to defaults first
        initializeDefaults();

        if (presetName == "default")
        {
            // Default parameters already set
            return true;
        }
        else if (presetName == "flat")
        {
            m_terrainMode = TerrainMode::Flat;
            m_minHeight = 0.0f;
            m_maxHeight = 100.0f;
            m_waterLevel = 50.0f;
            return true;
        }
        else if (presetName == "hills")
        {
            m_terrainMode = TerrainMode::HeightMap;
            m_minHeight = 0.0f;
            m_maxHeight = 128.0f;
            m_waterLevel = 60.0f;
            m_terrainRoughness = 0.3f;
            m_terrainErosion = 0.1f;
            m_terrainNoiseLayer = "terrain";
            return true;
        }
        else if (presetName == "mountains")
        {
            m_terrainMode = TerrainMode::Mountains;
            m_minHeight = 0.0f;
            m_maxHeight = 196.0f;
            m_waterLevel = 50.0f;
            m_terrainRoughness = 0.7f;
            m_terrainErosion = 0.3f;
            m_terrainNoiseLayer = "terrain";
            setBoolParam("snow_caps", true);
            return true;
        }
        else if (presetName == "islands")
        {
            m_terrainMode = TerrainMode::Islands;
            m_minHeight = 10.0f;
            m_maxHeight = 120.0f;
            m_waterLevel = 60.0f;
            m_waterEnabled = true;
            m_terrainRoughness = 0.4f;
            m_terrainNoiseLayer = "terrain";
            return true;
        }
        else if (presetName == "caves")
        {
            m_terrainMode = TerrainMode::Volumetric;
            m_minHeight = 0.0f;
            m_maxHeight = 128.0f;
            m_waterLevel = 40.0f;
            m_cavesEnabled = true;
            m_caveDensity = 0.6f;
            m_caveSize = 1.5f;
            m_terrainNoiseLayer = "terrain";
            m_caveNoiseLayer = "caves";
            return true;
        }
        else if (presetName == "desert")
        {
            m_terrainMode = TerrainMode::Desert;
            m_minHeight = 0.0f;
            m_maxHeight = 100.0f;
            m_waterLevel = 30.0f;
            m_waterEnabled = false;
            m_cavesEnabled = false;
            m_terrainRoughness = 0.2f;
            m_terrainErosion = 0.05f;
            m_terrainNoiseLayer = "terrain";
            setBoolParam("use_dunes", true);
            setFloatParam("dune_scale", 40.0f);
            return true;
        }
        else if (presetName == "archipelago")
        {
            m_terrainMode = TerrainMode::Islands;
            m_minHeight = 0.0f;
            m_maxHeight = 80.0f;
            m_waterLevel = 50.0f;
            m_waterEnabled = true;
            m_terrainRoughness = 0.3f;
            m_terrainNoiseLayer = "terrain";
            setFloatParam("island_density", 0.3f);
            return true;
        }
        else if (presetName == "jungle")
        {
            m_terrainMode = TerrainMode::HeightMap;
            m_minHeight = 0.0f;
            m_maxHeight = 160.0f;
            m_waterLevel = 40.0f;
            m_cavesEnabled = true;
            m_caveDensity = 0.3f;
            m_terrainRoughness = 0.5f;
            m_terrainErosion = 0.2f;
            m_terrainNoiseLayer = "terrain";
            m_featureDensity = 0.8f;
            setBoolParam("dense_vegetation", true);
            return true;
        }
        else if (presetName == "canyon")
        {
            m_terrainMode = TerrainMode::HeightMap;
            m_minHeight = 0.0f;
            m_maxHeight = 200.0f;
            m_waterLevel = 20.0f;
            m_terrainRoughness = 0.6f;
            m_terrainErosion = 0.8f;
            m_terrainNoiseLayer = "terrain";
            setBoolParam("use_canyons", true);
            setFloatParam("canyon_depth", 100.0f);
            setFloatParam("canyon_width", 40.0f);
            return true;
        }

        // Preset not found
        Log::warn("Preset '" + presetName + "' not found");
        return false;
    }

    void GenerationParameters::serialize(ECS::Serializer& serializer) const
    {
        // Serialize basic properties
        serializer.write(static_cast<int>(m_terrainMode));
        serializer.write(m_minHeight);
        serializer.write(m_maxHeight);
        serializer.write(m_waterLevel);
        serializer.write(m_waterEnabled);
        serializer.write(m_cavesEnabled);
        serializer.write(m_caveDensity);
        serializer.write(m_caveSize);
        serializer.writeString(m_terrainNoiseLayer);
        serializer.writeString(m_caveNoiseLayer);
        serializer.writeString(m_featureNoiseLayer);
        serializer.writeString(m_biomeNoiseLayer);
        serializer.write(m_terrainRoughness);
        serializer.write(m_terrainErosion);
        serializer.write(static_cast<int>(m_featurePlacementPattern));
        serializer.write(m_featureDensity);
        serializer.write(m_biomeBlendDistance);
        serializer.writeString(m_featurePlacementControl);
        serializer.writeString(m_distributionControl);

        // Serialize feature type enabled flags
        serializer.beginArray("enabledFeatureTypes", m_enabledFeatureTypes.size());
        for (const auto& [type, enabled] : m_enabledFeatureTypes)
        {
            serializer.writeString(type);
            serializer.write(enabled);
        }
        serializer.endArray();

        // Serialize feature type densities
        serializer.beginArray("featureTypeDensities", m_featureTypeDensities.size());
        for (const auto& [type, density] : m_featureTypeDensities)
        {
            serializer.writeString(type);
            serializer.write(density);
        }
        serializer.endArray();

        // Serialize custom parameters
        serializer.beginArray("floatParams", m_floatParams.size());
        for (const auto& [name, value] : m_floatParams)
        {
            serializer.writeString(name);
            serializer.write(value);
        }
        serializer.endArray();

        serializer.beginArray("intParams", m_intParams.size());
        for (const auto& [name, value] : m_intParams)
        {
            serializer.writeString(name);
            serializer.write(value);
        }
        serializer.endArray();

        serializer.beginArray("boolParams", m_boolParams.size());
        for (const auto& [name, value] : m_boolParams)
        {
            serializer.writeString(name);
            serializer.write(value);
        }
        serializer.endArray();

        serializer.beginArray("stringParams", m_stringParams.size());
        for (const auto& [name, value] : m_stringParams)
        {
            serializer.writeString(name);
            serializer.writeString(value);
        }
        serializer.endArray();

        serializer.beginArray("vec3Params", m_vec3Params.size());
        for (const auto& [name, value] : m_vec3Params)
        {
            serializer.writeString(name);
            serializer.writeVec3(value);
        }
        serializer.endArray();
    }

    void GenerationParameters::deserialize(ECS::Deserializer& deserializer)
    {
        // Reset to defaults first
        initializeDefaults();

        // Deserialize basic properties
        int tempTerrainMode;
        deserializer.read(tempTerrainMode);
        m_terrainMode = static_cast<TerrainMode>(tempTerrainMode);

        deserializer.read(m_minHeight);
        deserializer.read(m_maxHeight);
        deserializer.read(m_waterLevel);
        deserializer.read(m_waterEnabled);
        deserializer.read(m_cavesEnabled);
        deserializer.read(m_caveDensity);
        deserializer.read(m_caveSize);
        deserializer.readString(m_terrainNoiseLayer);
        deserializer.readString(m_caveNoiseLayer);
        deserializer.readString(m_featureNoiseLayer);
        deserializer.readString(m_biomeNoiseLayer);
        deserializer.read(m_terrainRoughness);
        deserializer.read(m_terrainErosion);

        int tempPattern;
        deserializer.read(tempPattern);
        m_featurePlacementPattern = static_cast<PlacementPattern>(tempPattern);

        deserializer.read(m_featureDensity);
        deserializer.read(m_biomeBlendDistance);
        deserializer.readString(m_featurePlacementControl);
        deserializer.readString(m_distributionControl);

        size_t arraySize;
        // Deseserialize feature type enabled flags
        deserializer.beginArray("enabledFeatureTypes", arraySize);
        for (int i = 0; i < arraySize; i++)
        {
            std::string type;
            deserializer.readString(type);

            bool enabled;
            deserializer.read(enabled);

            m_enabledFeatureTypes[type] = enabled;
        }
        deserializer.endArray();

        // Deserialize feature type densities
        deserializer.beginArray("featureTypeDensities", arraySize);
        for (int i = 0; i < arraySize; i++)
        {
            std::string type;
            deserializer.readString(type);

            float density;
            deserializer.read(density);

            m_featureTypeDensities[type] = density;
        }
        deserializer.endArray();

        // Deserialize custom parameters
        deserializer.beginArray("floatParams", arraySize);
        for (int i = 0; i < arraySize; i++)
        {
            std::string type;
            deserializer.readString(type);

            float value;
            deserializer.read(value);

            m_floatParams[type] = value;
        }
        deserializer.endArray();

        deserializer.beginArray("intParams", arraySize);
        for (int i = 0; i < arraySize; i++)
        {
            std::string type;
            deserializer.readString(type);

            int value;
            deserializer.read(value);

            m_intParams[type] = value;
        }
        deserializer.endArray();

        deserializer.beginArray("boolParams", arraySize);
        for (int i = 0; i < arraySize; i++)
        {
            std::string type;
            deserializer.readString(type);

            bool value;
            deserializer.read(value);

            m_boolParams[type] = value;
        }
        deserializer.endArray();

        deserializer.beginArray("stringParams", arraySize);
        for (int i = 0; i < arraySize; i++)
        {
            std::string type;
            deserializer.readString(type);

            std::string value;
            deserializer.readString(value);

            m_stringParams[type] = value;
        }
        deserializer.endArray();

        deserializer.beginArray("vec3Params", arraySize);
        for (int i = 0; i < arraySize; i++)
        {
            std::string type;
            deserializer.readString(type);

            glm::vec3 value;
            deserializer.readVec3(value);

            m_vec3Params[type] = value;
        }
        deserializer.endArray();
    }

    void GenerationParameters::initializeDefaults()
    {
        // Basic properties
        m_terrainMode = TerrainMode::HeightMap;
        m_minHeight = 0.0f;
        m_maxHeight = 128.0f;
        m_waterLevel = 60.0f;
        m_waterEnabled = true;
        m_cavesEnabled = true;
        m_caveDensity = 0.5f;
        m_caveSize = 1.0f;
        m_terrainNoiseLayer = "terrain";
        m_caveNoiseLayer = "caves";
        m_featureNoiseLayer = "";
        m_biomeNoiseLayer = "";
        m_terrainRoughness = 0.5f;
        m_terrainErosion = 0.1f;
        m_featurePlacementPattern = PlacementPattern::Random;
        m_featureDensity = 0.5f;
        m_biomeBlendDistance = 8.0f;
        m_featurePlacementControl = "default";
        m_distributionControl = "default";

        // Clear all maps
        m_enabledFeatureTypes.clear();
        m_featureTypeDensities.clear();
        m_floatParams.clear();
        m_intParams.clear();
        m_boolParams.clear();
        m_stringParams.clear();
        m_vec3Params.clear();

        // Set common feature types to enabled by default
        m_enabledFeatureTypes["tree"] = true;
        m_enabledFeatureTypes["rock"] = true;
        m_enabledFeatureTypes["cave"] = true;
        m_enabledFeatureTypes["ore"] = true;

        // Set default densities for common feature types
        m_featureTypeDensities["tree"] = 0.5f;
        m_featureTypeDensities["rock"] = 0.3f;
        m_featureTypeDensities["cave"] = 0.2f;
        m_featureTypeDensities["ore"] = 0.1f;
    }

} // namespace PixelCraft::Voxel