// -------------------------------------------------------------------------
// RenderStage.cpp
// -------------------------------------------------------------------------
#include "Rendering/RenderStage.h"
#include "Rendering/RenderTarget.h"

#include <glad/glad.h>

namespace PixelCraft::Rendering
{

    // -------------------------------------------------------------------------
    // StageConfig Implementation
    // -------------------------------------------------------------------------

    Utility::Serialization::SerializationResult StageConfig::serialize(Utility::Serialization::Serializer& serializer) const
    {
        auto result = serializer.beginObject("StageConfig");
        if (!result) return result;

        result = serializer.writeField("name", name);
        if (!result) return result;

        result = serializer.writeField("type", type);
        if (!result) return result;

        result = serializer.writeField("enabled", enabled);
        if (!result) return result;

        result = serializer.writeField("outputTarget", outputTarget);
        if (!result) return result;

        result = serializer.beginArray("inputTargets", inputTargets.size());
        if (!result) return result;

        for (const auto& target : inputTargets)
        {
            result = serializer.writeString(target);
            if (!result) return result;
        }

        result = serializer.endArray();
        if (!result) return result;

        result = serializer.beginObject("parameters");
        if (!result) return result;

        for (const auto& [paramName, paramValue] : parameters)
        {
            result = serializer.writeFieldName(paramName);
            if (!result.success) return result;

            result = paramValue.serialize(serializer);
            if (!result.success) return result;
        }

        result = serializer.endObject();
        if (!result.success) return result;

        return serializer.endObject();
    }

    Utility::Serialization::SerializationResult StageConfig::deserialize(Utility::Serialization::Deserializer& deserializer)
    {
        auto result = deserializer.beginObject("StageConfig");
        if (!result) return result;

        result = deserializer.readField("name", name);
        if (!result) return result;

        result = deserializer.readField("type", type);
        if (!result) return result;

        result = deserializer.readField("enabled", enabled);
        if (!result) return result;

        result = deserializer.readField("outputTarget", outputTarget);
        if (!result) return result;

        size_t inputCount;
        result = deserializer.beginArray("inputTargets", inputCount);
        if (!result) return result;

        inputTargets.clear();
        inputTargets.reserve(inputCount);
        for (size_t i = 0; i < inputCount; ++i)
        {
            std::string target;
            result = deserializer.readString(target);
            if (!result) return result;
            inputTargets.push_back(target);
        }

        result = deserializer.endArray();
        if (!result) return result;

        result = deserializer.beginObject("parameters");
        if (!result) return result;
        parameters.clear();

        // Read all parameter fields
        std::string fieldName;
        while (deserializer.hasField(""))
        {
            result = deserializer.readFieldName(fieldName);
            if (!result) break;

            Utility::Serialization::ParameterValue paramValue;
            result = paramValue.deserialize(deserializer);
            if (!result) return result;

            parameters[fieldName] = paramValue;
        }

        result = deserializer.endObject();
        if (!result) return result;

        return deserializer.endObject();
    }

    void StageConfig::defineSchema(Utility::Serialization::Schema& schema)
    {
        schema.addField("name", Utility::Serialization::ValueType::String, true);
        schema.addField("type", Utility::Serialization::ValueType::String, true);
        schema.addField("enabled", Utility::Serialization::ValueType::Bool, true);
        schema.addField("outputTarget", Utility::Serialization::ValueType::String, false);
        schema.addArrayField("inputTargets", Utility::Serialization::ValueType::String, false);
        schema.addComplexField("parameters", "ParameterMap", false);
    }

    // -------------------------------------------------------------------------
    // RenderStage Implementation
    // -------------------------------------------------------------------------

    RenderStage::RenderStage(const std::string& name)
        : m_name(name)
        , m_enabled(true)
        , m_initialized(false)
        , m_output(nullptr)
    {
        // Initialize inputs array to nullptr
        for (int i = 0; i < MAX_INPUTS; ++i)
        {
            m_inputs[i] = nullptr;
        }

        // Register schemas if not already registered
        auto& registry = Utility::Serialization::SchemaRegistry::getInstance();

        // Register StageConfig schema
        if (!registry.hasSchema(StageConfig::getTypeName()))
        {
            auto configSchema = std::make_shared<Utility::Serialization::Schema>(
                StageConfig::getTypeName(), Utility::Serialization::VersionInfo{1, 0, 0});
            StageConfig::defineSchema(*configSchema);
            registry.registerSchema(configSchema);
        }

        // Register RenderStage schema
        if (!registry.hasSchema("RenderStage"))
        {
            auto stageSchema = std::make_shared<Utility::Serialization::Schema>(
                "RenderStage", getVersion());
            defineSchema(*stageSchema);
            registry.registerSchema(stageSchema);
        }
    }

    RenderStage::~RenderStage()
    {
        // Virtual destructor - cleanup handled by derived classes
    }

    void RenderStage::setOutput(std::shared_ptr<RenderTarget> target)
    {
        m_output = target;
        Core::debug("RenderStage '" + m_name + "': Set output target");
    }

    std::shared_ptr<RenderTarget> RenderStage::getOutput() const
    {
        return m_output;
    }

    void RenderStage::setInput(std::shared_ptr<RenderTarget> target, int index)
    {
        if (index < 0 || index >= MAX_INPUTS)
        {
            Core::error("RenderStage '" + m_name + "': Invalid input index " + std::to_string(index));
            return;
        }

        m_inputs[index] = target;
        Core::debug("RenderStage '" + m_name + "': Set input " + std::to_string(index));
    }

    std::shared_ptr<RenderTarget> RenderStage::getInput(int index) const
    {
        if (index < 0 || index >= MAX_INPUTS)
        {
            Core::error("RenderStage '" + m_name + "': Invalid input index " + std::to_string(index));
            return nullptr;
        }

        return m_inputs[index];
    }

    bool RenderStage::hasInput(int index) const
    {
        if (index < 0 || index >= MAX_INPUTS)
        {
            return false;
        }

        return m_inputs[index] != nullptr;
    }

    void RenderStage::bindOutput()
    {
        if (m_output)
        {
            Core::debug("RenderStage '" + m_name + "': Binding output target");
            m_output->bind();
        }
        else
        {
            // If no output target is set, bind the default framebuffer (screen)
            Core::debug("RenderStage '" + m_name + "': Binding default framebuffer (screen)");
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            
            // Get the current viewport size (assuming screen rendering)
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);
            glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        }
    }

    void RenderStage::unbindOutput()
    {
        if (m_output)
        {
            m_output->unbind();
        }
        else
        {
            // Already bound to default framebuffer, nothing to unbind
        }
    }

    void RenderStage::clearOutput(bool clearColor, bool clearDepth, bool clearStencil)
    {
        if (m_output)
        {
            Core::debug("RenderStage '" + m_name + "': Clearing output target");
            m_output->clear(clearColor, clearDepth, clearStencil);
        }
        else
        {
            // Clear the default framebuffer
            Core::debug("RenderStage '" + m_name + "': Clearing default framebuffer");
            GLbitfield clearFlags = 0;
            if (clearColor) clearFlags |= GL_COLOR_BUFFER_BIT;
            if (clearDepth) clearFlags |= GL_DEPTH_BUFFER_BIT;
            if (clearStencil) clearFlags |= GL_STENCIL_BUFFER_BIT;

            if (clearFlags != 0)
            {
                glClear(clearFlags);
            }
        }
    }

    void RenderStage::clearOutputColor(int index, const glm::vec4& color)
    {
        if (m_output)
        {
            m_output->clearColor(index, color);
        }
        else
        {
            // For default framebuffer, just use standard clear
            if (index == 0)
            {
                glClearColor(color.r, color.g, color.b, color.a);
                glClear(GL_COLOR_BUFFER_BIT);
            }
        }
    }

    bool RenderStage::hasParameter(const std::string& name) const
    {
        return m_parameters.find(name) != m_parameters.end();
    }

    Utility::Serialization::SerializationResult RenderStage::serialize(Utility::Serialization::Serializer& serializer) const
    {
        auto result = serializer.beginObject("RenderStage");
        if (!result) return result;

        // Write stage type for factory reconstruction
        result = serializer.writeField("stageType", getTypeName());
        if (!result) return result;

        // Write version info
        auto version = getVersion();
        result = serializer.writeField("version_major", static_cast<uint32_t>(version.major));
        if (!result) return result;

        result = serializer.writeField("version_minor", static_cast<uint32_t>(version.minor));
        if (!result) return result;

        result = serializer.writeField("version_patch", static_cast<uint32_t>(version.patch));
        if (!result) return result;

        // Serialize base configuration
        auto config = getConfig();
        result = serializer.beginObject("config");
        if (!result.success) return result;

        result = config.serialize(serializer);
        if (!result.success) return result;

        result = serializer.endObject();
        if (!result.success) return result;

        // Save stage-specific configuration
        result = serializer.beginObject("stageSpecific");
        if (!result.success) return result;

        result = saveStageSpecificConfig(serializer);
        if (!result.success) return result;

        result = serializer.endObject();
        if (!result.success) return result;

        return serializer.endObject();
    }

    Utility::Serialization::SerializationResult RenderStage::deserialize(Utility::Serialization::Deserializer& deserializer)
    {
        auto result = deserializer.beginObject("RenderStage");
        if (!result) return result;

        // Read stage type (for verification)
        std::string stageType;
        result = deserializer.readField("stageType", stageType);
        if (!result) return result;

        if (stageType != getTypeName())
        {
            Core::warn("Stage type mismatch: expected " + getTypeName() + ", got " + stageType);
        }

        // Read version info
        uint32_t major, minor, patch;
        result = deserializer.readField("version_major", major);
        if (!result) return result;

        result = deserializer.readField("version_minor", minor);
        if (!result) return result;

        result = deserializer.readField("version_patch", patch);
        if (!result) return result;

        Utility::Serialization::VersionInfo fileVersion{major, minor, patch};
        auto currentVersion = getVersion();

        // Check version compatibility
        if (fileVersion.major != currentVersion.major)
        {
            Core::error("Incompatible stage version: " + std::to_string(fileVersion.major) +
                                " vs " + std::to_string(currentVersion.major));
            return Utility::Serialization::SerializationResult("Version mismatch");
        }

        // Deserialize base configuration
        result = deserializer.beginObject("config");
        if (!result) return result;

        StageConfig config;
        result = config.deserialize(deserializer);
        if (!result) return result;

        result = deserializer.endObject();
        if (!result) return result;

        if (!applyConfig(config))
        {
            return Utility::Serialization::SerializationResult("Failed to apply stage configuration");
        }

        // Load stage-specific configuration
        result = deserializer.beginObject("stageSpecific");
        if (!result) return result;

        result = loadStageSpecificConfig(deserializer);
        if (!result) return result;

        result = deserializer.endObject();
        if (!result) return result;

        return deserializer.endObject();
    }

    StageConfig RenderStage::getConfig() const
    {
        StageConfig config;
        config.name = m_name;
        config.type = getTypeName();
        config.enabled = m_enabled;
        config.parameters = m_parameters;

        // Note: Input/output target names would need to be resolved from actual targets
        // This requires access to the pipeline's target registry in a real implementation
        config.inputTargets.clear();
        for (size_t i = 0; i < MAX_INPUTS; i++)
        {
            if (m_inputs[i] != nullptr)
            {
                config.inputTargets.push_back("input_" + std::to_string(i));
            }
        }

        if (m_output)
        {
            config.outputTarget = "output";
        }

        return config;
    }

    bool RenderStage::applyConfig(const StageConfig& config)
    {
        m_name = config.name;
        m_enabled = config.enabled;
        m_parameters = config.parameters;

        // Note: Input/output targets would need to be resolved from names
        // This requires access to the pipeline's target registry in a real implementation

        Core::debug("Applied configuration to stage: " + m_name);
        return true;
    }

    Utility::Serialization::VersionInfo RenderStage::getVersion() const
    {
        return {1, 0, 0}; // Major, Minor, Patch
    }

    void RenderStage::defineSchema(Utility::Serialization::Schema& schema)
    {
        schema.addField("stageType", Utility::Serialization::ValueType::String, true);
        schema.addField("version_major", Utility::Serialization::ValueType::UInt64, true);
        schema.addField("version_minor", Utility::Serialization::ValueType::UInt64, true);
        schema.addField("version_patch", Utility::Serialization::ValueType::UInt64, true);
        schema.addComplexField("config", StageConfig::getTypeName(), true);
        schema.addComplexField("stageSpecific", "Object", false);
    }

    Utility::Serialization::SerializationResult RenderStage::loadStageSpecificConfig(Utility::Serialization::Deserializer& deserializer)
    {
        // Default implementation - derived classes can override for stage-specific data
        (void)deserializer;  // Unused in default impl
        return Utility::Serialization::SerializationResult();
    }

    Utility::Serialization::SerializationResult RenderStage::saveStageSpecificConfig(Utility::Serialization::Serializer& serializer) const
    {
        // Default implementation - derived classes can override for stage-specific data
        (void)serializer;  // Unused in default impl
        return Utility::Serialization::SerializationResult();
    }

} // namespace PixelCraft::Rendering
