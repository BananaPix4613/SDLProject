// -------------------------------------------------------------------------
// GenerationParameters.cpp
// -------------------------------------------------------------------------
#include "Voxel/GenerationParameters.h"
#include "Utility/Serialization/SerializationUtility.h"
#include "Core/Logger.h"

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
            Core::warn("Min height greater than max height, swapping values");
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
        Core::warn("Preset '" + presetName + "' not found");
        return false;
    }

    Utility::Serialization::SerializationResult GenerationParameters::serialize(Utility::Serialization::Serializer& serializer) const
    {
        // Begin serializing the parameters
        auto result = serializer.beginObject(getTypeName());
        if (!result) return result;

        // Serialize basic properties
        result = serializer.writeField("terrainMode", static_cast<int>(m_terrainMode));
        if (!result) return result;

        result = serializer.writeField("minHeight", m_minHeight);
        if (!result) return result;

        result = serializer.writeField("maxHeight", m_maxHeight);
        if (!result) return result;

        result = serializer.writeField("waterLevel", m_waterLevel);
        if (!result) return result;

        result = serializer.writeField("waterEnabled", m_waterEnabled);
        if (!result) return result;

        result = serializer.writeField("cavesEnabled", m_cavesEnabled);
        if (!result) return result;

        result = serializer.writeField("caveDensity", m_caveDensity);
        if (!result) return result;

        result = serializer.writeField("caveSize", m_caveSize);
        if (!result) return result;

        result = serializer.writeField("terrainRoughness", m_terrainRoughness);
        if (!result) return result;

        result = serializer.writeField("terrainErosion", m_terrainErosion);
        if (!result) return result;

        result = serializer.writeField("featurePlacementPattern", static_cast<int>(m_featurePlacementPattern));
        if (!result) return result;

        result = serializer.writeField("featureDensity", m_featureDensity);
        if (!result) return result;

        result = serializer.writeField("biomeBlendDistance", m_biomeBlendDistance);
        if (!result) return result;

        result = serializer.writeField("featurePlacementControl", m_featurePlacementControl);
        if (!result) return result;

        result = serializer.writeField("distributionControl", m_distributionControl);
        if (!result) return result;

        // Serialize feature type enabled flags
        result = serializer.beginArray("enabledFeatureTypes", m_enabledFeatureTypes.size());
        if (!result) return result;

        for (const auto& [type, enabled] : m_enabledFeatureTypes)
        {
            result = serializer.writeField("type", type);
            if (!result) return result;

            result = serializer.writeField("enabled", enabled);
            if (!result) return result;
        }

        result = serializer.endArray();
        if (!result) return result;

        // Serialize feature type densities
        result = serializer.beginArray("featureTypeDensities", m_featureTypeDensities.size());
        if (!result) return result;

        for (const auto& [type, density] : m_featureTypeDensities)
        {
            result = serializer.writeField("type", type);
            if (!result) return result;

            result = serializer.writeField("density", density);
            if (!result) return result;
        }
        
        result = serializer.endArray();
        if (!result) return result;

        // Serialize custom parameters
        result = serializer.beginArray("floatParams", m_floatParams.size());
        if (!result) return result;

        for (const auto& [name, value] : m_floatParams)
        {
            result = serializer.writeField("name", name);
            if (!result) return result;

            result = serializer.writeField("value", value);
            if (!result) return result;
        }
        
        result = serializer.endArray();
        if (!result) return result;

        result = serializer.beginArray("intParams", m_intParams.size());
        if (!result) return result;

        for (const auto& [name, value] : m_intParams)
        {
            result = serializer.writeField("name", name);
            if (!result) return result;

            result = serializer.writeField("value", value);
            if (!result) return result;
        }
        
        result = serializer.endArray();
        if (!result) return result;

        result = serializer.beginArray("boolParams", m_boolParams.size());
        if (!result) return result;

        for (const auto& [name, value] : m_boolParams)
        {
            result = serializer.writeField("name", name);
            if (!result) return result;

            result = serializer.writeField("value", value);
            if (!result) return result;
        }
        
        result = serializer.endArray();
        if (!result) return result;

        result = serializer.beginArray("stringParams", m_stringParams.size());
        if (!result) return result;

        for (const auto& [name, value] : m_stringParams)
        {
            result = serializer.writeField("name", name);
            if (!result) return result;

            result = serializer.writeField("value", value);
            if (!result) return result;
        }
        
        result = serializer.endArray();
        if (!result) return result;

        result = serializer.beginArray("vec3Params", m_vec3Params.size());
        if (!result) return result;

        for (const auto& [name, value] : m_vec3Params)
        {
            result = serializer.writeField("name", name);
            if (!result) return result;

            result = serializer.writeField("value", value);
            if (!result) return result;
        }
        
        result = serializer.endArray();
        if (!result) return result;

        return serializer.endObject();
    }

    Utility::Serialization::SerializationResult GenerationParameters::deserialize(Utility::Serialization::Deserializer& deserializer)
    {
        // Reset to defaults first
        initializeDefaults();

        // Begin deserializing the parameters
        auto result = deserializer.beginObject(getTypeName());
        if (!result) return result;

        // Deserialize basic properties
        int tempMode;
        result = deserializer.readField("terrainMode", tempMode);
        if (!result) return result;
        m_terrainMode = static_cast<TerrainMode>(tempMode);

        result = deserializer.readField("minHeight", m_minHeight);
        if (!result) return result;

        result = deserializer.readField("maxHeight", m_maxHeight);
        if (!result) return result;

        result = deserializer.readField("waterLevel", m_waterLevel);
        if (!result) return result;

        result = deserializer.readField("waterEnabled", m_waterEnabled);
        if (!result) return result;

        result = deserializer.readField("cavesEnabled", m_cavesEnabled);
        if (!result) return result;

        result = deserializer.readField("caveDensity", m_caveDensity);
        if (!result) return result;

        result = deserializer.readField("caveSize", m_caveSize);
        if (!result) return result;

        result = deserializer.readField("terrainRoughness", m_terrainRoughness);
        if (!result) return result;

        result = deserializer.readField("terrainErosion", m_terrainErosion);
        if (!result) return result;

        int tempPattern;
        result = deserializer.readField("featurePlacementPattern", tempPattern);
        if (!result) return result;
        m_featurePlacementPattern = static_cast<PlacementPattern>(tempPattern);

        result = deserializer.readField("featureDensity", m_featureDensity);
        if (!result) return result;

        result = deserializer.readField("biomeBlendDistance", m_biomeBlendDistance);
        if (!result) return result;

        result = deserializer.readField("featurePlacementControl", m_featurePlacementControl);
        if (!result) return result;

        result = deserializer.readField("distributionControl", m_distributionControl);
        if (!result) return result;

        size_t arraySize;
        // Deseserialize feature type enabled flags
        result = deserializer.beginArray("enabledFeatureTypes", arraySize);
        if (!result) return result;

        for (size_t i = 0; i < arraySize; i++)
        {
            std::string type;
            result = deserializer.readField("type", type);
            if (!result) return result;

            bool enabled;
            result = deserializer.readField("enabled", enabled);
            if (!result) return result;

            m_enabledFeatureTypes[type] = enabled;
        }
        
        result = deserializer.endArray();
        if (!result) return result;

        // Deserialize feature type densities
        result = deserializer.beginArray("featureTypeDensities", arraySize);
        if (!result) return result;

        for (size_t i = 0; i < arraySize; i++)
        {
            std::string type;
            result = deserializer.readField("type", type);
            if (!result) return result;

            float density;
            result = deserializer.readField("density", density);
            if (!result) return result;

            m_featureTypeDensities[type] = density;
        }
        
        result = deserializer.endArray();
        if (!result) return result;

        // Deserialize custom parameters
        result = deserializer.beginArray("floatParams", arraySize);
        if (!result) return result;

        for (size_t i = 0; i < arraySize; i++)
        {
            std::string type;
            result = deserializer.readField("type", type);
            if (!result) return result;

            float value;
            result = deserializer.readField("value", value);
            if (!result) return result;

            m_floatParams[type] = value;
        }

        result = deserializer.endArray();
        if (!result) return result;

        result = deserializer.beginArray("intParams", arraySize);
        if (!result) return result;

        for (size_t i = 0; i < arraySize; i++)
        {
            std::string type;
            result = deserializer.readField("type", type);
            if (!result) return result;

            int value;
            result = deserializer.readField("value", value);
            if (!result) return result;

            m_intParams[type] = value;
        }
        
        result = deserializer.endArray();
        if (!result) return result;

        result = deserializer.beginArray("boolParams", arraySize);
        if (!result) return result;

        for (size_t i = 0; i < arraySize; i++)
        {
            std::string type;
            result = deserializer.readField("type", type);
            if (!result) return result;

            bool value;
            result = deserializer.readField("value", value);
            if (!result) return result;

            m_boolParams[type] = value;
        }
        
        result = deserializer.endArray();
        if (!result) return result;

        result = deserializer.beginArray("stringParams", arraySize);
        if (!result) return result;

        for (size_t i = 0; i < arraySize; i++)
        {
            std::string type;
            result = deserializer.readField("type", type);
            if (!result) return result;

            std::string value;
            result = deserializer.readField("value", value);
            if (!result) return result;

            m_stringParams[type] = value;
        }
        
        result = deserializer.endArray();
        if (!result) return result;

        result = deserializer.beginArray("vec3Params", arraySize);
        if (!result) return result;

        for (size_t i = 0; i < arraySize; i++)
        {
            std::string type;
            result = deserializer.readField("type", type);
            if (!result) return result;

            glm::vec3 value;
            result = deserializer.readField("value", value);
            if (!result) return result;

            m_vec3Params[type] = value;
        }
        
        result = deserializer.endArray();
        if (!result) return result;

        return deserializer.endObject();
    }

    void GenerationParameters::defineSchema(Utility::Serialization::Schema& schema)
    {
        schema.addField("terrainMode", Utility::Serialization::ValueType::Int64, true);
        schema.addField("minHeight", Utility::Serialization::ValueType::Float, true);
        schema.addField("maxHeight", Utility::Serialization::ValueType::Float, true);
        schema.addField("waterLevel", Utility::Serialization::ValueType::Float, true);
        schema.addField("waterEnabled", Utility::Serialization::ValueType::Bool, true);
        schema.addField("caveEnabled", Utility::Serialization::ValueType::Bool, true);
        schema.addField("caveDensity", Utility::Serialization::ValueType::Float, true);
        schema.addField("caveSize", Utility::Serialization::ValueType::Float, true);
        schema.addField("terrainNoiseLayer", Utility::Serialization::ValueType::String, true);
        schema.addField("caveNoiseLayer", Utility::Serialization::ValueType::String, true);
        schema.addField("featureNoiseLayer", Utility::Serialization::ValueType::String, true);
        schema.addField("biomeNoiseLayer", Utility::Serialization::ValueType::String, true);
        schema.addField("terrainRoughness", Utility::Serialization::ValueType::Float, true);
        schema.addField("terrainErosion", Utility::Serialization::ValueType::Float, true);
        schema.addField("featurePlacementPattern", Utility::Serialization::ValueType::Int64, true);
        schema.addField("featureDensity", Utility::Serialization::ValueType::Float, true);
        schema.addField("biomeBlendDistance", Utility::Serialization::ValueType::Float, true);
        schema.addField("featurePlacementControl", Utility::Serialization::ValueType::String, true);
        schema.addField("distributionControl", Utility::Serialization::ValueType::String, true);
        schema.addArrayField("enabledFeatureTypes", Utility::Serialization::ValueType::Bool, true);
        schema.addArrayField("featureTypeDensities", Utility::Serialization::ValueType::Float, true);
        schema.addArrayField("floatParams", Utility::Serialization::ValueType::Float, true);
        schema.addArrayField("intParams", Utility::Serialization::ValueType::Int64, true);
        schema.addArrayField("boolParams", Utility::Serialization::ValueType::Bool, true);
        schema.addArrayField("stringParams", Utility::Serialization::ValueType::String, true);
        schema.addArrayField("vec3Params", Utility::Serialization::ValueType::Vec3, true);
    }

    std::string GenerationParameters::getTypeName()
    {
        return "GenerationParameters";
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