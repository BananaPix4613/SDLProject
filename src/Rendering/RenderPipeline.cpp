// -------------------------------------------------------------------------
// RenderPipeline.cpp
// -------------------------------------------------------------------------
#include "Rendering/RenderPipeline.h"
#include "Core/Application.h"
#include "Core/Window.h"
#include "Core/Logger.h"
#include "Utility/Serialization/SerializationUtility.h"
#include "Utility/FileSystem.h"

// Stages
#include "Rendering/Stages/ClusterUpdateStage.h"
#include "Rendering/Stages/DepthPrepassStage.h"
#include "Rendering/Stages/ShadowStage.h"
#include "Rendering/Stages/GeometryStage.h"
#include "Rendering/Stages/TransparencyStage.h"
#include "Rendering/Stages/PostProcessStage.h"
#include "Rendering/Stages/SkyboxStage.h"
#include "Rendering/Stages/VelocityStage.h"
#include "Rendering/Stages/UIStage.h"

// Additional includes for pipeline configuration
#include "Rendering/ShaderManager.h"
#include "Rendering/ForwardPlusShaderManager.h"
#include "Rendering/RenderSystem.h"
#include "ECS/World.h"

#include <fstream>

// In namespace PixelCraft
namespace PixelCraft::Rendering
{

    // -------------------------------------------------------------------------
    // PipelineConfig Implementation
    // -------------------------------------------------------------------------

    Utility::Serialization::SerializationResult PipelineConfig::serialize(Utility::Serialization::Serializer& serializer) const
    {
        auto result = serializer.beginObject("PipelineConfig");
        if (!result) return result;

        result = serializer.beginArray("stageTypes", stageTypes.size());
        if (!result) return result;

        for (const auto& type : stageTypes)
        {
            result = serializer.writeField("stageType", type);
            if (!result) return result;
        }

        result = serializer.endArray();
        if (!result) return result;

        result = serializer.beginArray("connections", connections.size());
        if (!result) return result;

        for (const auto& [input, output] : connections)
        {
            result = serializer.writeField("connectionInput", input);
            if (!result) return result;

            result = serializer.writeField("connectionOutput", output);
            if (!result) return result;
        }

        result = serializer.endArray();
        if (!result) return result;

        result = serializer.beginArray("globalParameters", globalParameters.size());
        if (!result) return result;

        for (const auto& [paramName, paramValue] : globalParameters)
        {
            result = serializer.writeField("parameterName", paramName);
            if (!result) return result;

            result = paramValue.serialize(serializer);
            if (!result) return result;
        }

        result = serializer.endArray();
        if (!result) return result;

        return serializer.endObject();
    }

    Utility::Serialization::SerializationResult PipelineConfig::deserialize(Utility::Serialization::Deserializer& deserializer)
    {
        auto result = deserializer.beginObject("PipelineConfig");
        if (!result) return result;

        size_t arraySize;
        result = deserializer.beginArray("stageTypes", arraySize);
        if (!result) return result;

        stageTypes.clear();
        for (size_t i = 0; i < arraySize; i++)
        {
            std::string stageType;
            result = deserializer.readField("stageType", stageType);
            if (!result) return result;

            stageTypes.push_back(stageType);
        }

        result = deserializer.endArray();
        if (!result) return result;

        result = deserializer.beginArray("connections", arraySize);
        if (!result) return result;

        connections.clear();
        for (size_t i = 0; i < arraySize; i++)
        {
            std::string input, output;
            result = deserializer.readField("connectionInput", input);
            if (!result) break;

            result = deserializer.readField("connectionOutput", output);
            if (!result) return result;

            connections[input] = output;
        }

        result = deserializer.endArray();
        if (!result) return result;

        result = deserializer.beginArray("globalParameters", arraySize);
        if (!result) return result;

        globalParameters.clear();
        for (size_t i = 0; i < arraySize; i++)
        {
            std::string paramName;
            result = deserializer.readField("parameterName", paramName);
            if (!result) break;

            Utility::Serialization::ParameterValue paramValue;
            result = paramValue.deserialize(deserializer);
            if (!result) return result;

            globalParameters[paramName] = paramValue;
        }

        result = deserializer.endArray();
        if (!result) return result;

        return deserializer.endObject();
    }

    void PipelineConfig::defineSchema(Utility::Serialization::Schema& schema)
    {
        schema.addArrayField("stageTypes", Utility::Serialization::ValueType::String);
        schema.addArrayField("connections", Utility::Serialization::ValueType::String);
        schema.addArrayField("globalParameters", Utility::Serialization::ValueType::String);
    }

    // -------------------------------------------------------------------------
    // RenderPipeline Implementation
    // -------------------------------------------------------------------------

    RenderPipeline::RenderPipeline()
        : m_frameNumber(0)
    {
        auto& registry = Utility::Serialization::SchemaRegistry::getInstance();
        if (!registry.hasSchema(getTypeName()))
        {
            auto schema = std::make_shared<Utility::Serialization::Schema>(getTypeName(), getVersion());
            defineSchema(*schema);
            registry.registerSchema(schema);
        }

        // Set debugger instance
        m_debugger = std::make_shared<RenderPipelineDebugger>();
    }

    RenderPipeline::~RenderPipeline()
    {
        shutdown();
    }

    bool RenderPipeline::initialize()
    {
        if (m_initialized)
        {
            return true;
        }

        // Initialize debugger
        if (!m_debugger->initialize())
        {
            Core::warn("Failed to initialize pipeline debugger");
        }

        // Get window to determine dimensions
        auto window = Core::Application::getInstance()->getSubsystem<Core::Window>();
        if (!window)
        {
            Core::error("Window not found");
            return false;
        }

        int windowWidth, windowHeight;
        window->getSize(windowWidth, windowHeight);

        // Create shared depth texture for forward+ early-Z optimization
        // OpenGL 4.3+ supports depth textures, which this engine requires
        auto sharedDepth = createSharedDepthTexture("ForwardPlusDepth", windowWidth, windowHeight,
                                                    TextureFormat::Depth24);
        
        if (sharedDepth)
        {
            Core::info("RenderPipeline: Created shared depth texture for forward+ early-Z optimization");
            Core::info("  - Texture ID: " + std::to_string(sharedDepth->getID()));
            Core::info("  - Dimensions: " + std::to_string(sharedDepth->getWidth()) + "x" + std::to_string(sharedDepth->getHeight()));
            Core::info("  - Format: Depth24 (GL_DEPTH_COMPONENT24)");
        }
        else
        {
            Core::error("RenderPipeline: Failed to create shared depth texture - forward+ optimization disabled");
        }

        // Create depth-only buffer for Z-prepass using shared depth if available
        IntermediateTargetConfig depthConfig;
        depthConfig.width = windowWidth;
        depthConfig.height = windowHeight;
        depthConfig.createColor = false;                // Depth-only target
        depthConfig.createDepth = true;
        depthConfig.sharedDepthTexture = sharedDepth;   // Use shared depth if available
        
        Core::debug("Creating DepthBuffer intermediate target with shared depth: " + 
                   (sharedDepth ? "YES (ID: " + std::to_string(sharedDepth->getID()) + ")" : "NO"));
        
        auto depthBufferTarget = createIntermediateTarget("DepthBuffer", depthConfig);
        if (!depthBufferTarget)
        {
            Core::error("CRITICAL: Failed to create DepthBuffer intermediate target!");
        }
        else
        {
            Core::info("DepthBuffer intermediate target created successfully (ID: " + 
                      std::to_string(depthBufferTarget->getFramebufferID()) + ")");
        }

        // Create main scene color using the SAME shared depth if available
        IntermediateTargetConfig sceneConfig;
        sceneConfig.width = windowWidth;
        sceneConfig.height = windowHeight;
        sceneConfig.colorFormat = TextureFormat::RGBA16F;
        sceneConfig.createColor = true;
        sceneConfig.createDepth = true;
        sceneConfig.sharedDepthTexture = sharedDepth;   // Share the same depth if available
        createIntermediateTarget("SceneColor", sceneConfig);
        
        // Other targets created with simpler method
        createIntermediateTarget("SceneColorFinal", windowWidth, windowHeight,
                                TextureFormat::RGBA8, false, false);

        createIntermediateTarget("PostProcessTemp", windowWidth, windowHeight, 
                            TextureFormat::RGBA16F, false, false);
        
        createIntermediateTarget("FinalColor", windowWidth, windowHeight, 
                            TextureFormat::RGBA8, false, false);

        Core::info("Created intermediate targets for forward+ pipeline with shared depth optimization");

        // Create default forward+ stages in optimal order
        createDefaultStages();

        // Initialize all stages
        for (auto& stage : m_stages)
        {
            if (!stage->initialize())
            {
                Core::error("Failed to initialize stage: " + stage->getName());
                return false;
            }
        }

        // Set up fixed stage connections optimized for forward+ rendering
        setupStageConnections();

        if (!validate())
        {
            Core::error("Pipeline validation failed");
            return false;
        }

        m_initialized = true;
        Core::info("Initialized forward+ render pipeline");
        return true;
    }

    void RenderPipeline::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        if (m_debugger)
        {
            m_debugger->shutdown();
        }

        for (auto& stage : m_stages)
        {
            stage->shutdown();
        }

        m_intermediateTargets.clear();
        m_sharedTextures.clear();
        m_output.reset();
        m_initialized = false;
        Core::info("Shutdown render pipeline");
    }

    void RenderPipeline::update(float deltaTime)
    {
        // Nothing to update currently
        (void)deltaTime;
    }

    void RenderPipeline::render(const std::shared_ptr<RenderContext> context)
    {
        if (!m_initialized)
        {
            Core::error("Cannot render - pipeline not initialized");
            return;
        }

        // Increment frame counter for debugging
        m_frameNumber++;

        // Execute stages with debugging - each stage binds its own output target
        for (auto& stage : m_stages)
        {
            if (stage && stage->isEnabled() && stage->isInitialized())
            {
                Core::debug("Executing stage: " + stage->getName());

                // Execute the stage
                stage->execute(context);

                // Capture debug output if debugging is enabled
                if (m_debugger && m_debugger->isEnabled())
                {
                    m_debugger->captureStage(stage, m_frameNumber, context->getNearPlane(), context->getFarPlane());
                }
            }
        }

        // After all stages complete, handle final output
        bool renderingToScreen = (m_output == nullptr);

        if (renderingToScreen)
        {
            // Find the final render target from the last enabled stage
            std::shared_ptr<RenderTarget> finalTarget = nullptr;

            // Work backwards to find the last stage with output
            for (auto it = m_stages.rbegin(); it != m_stages.rend(); it++)
            {
                auto& stage = *it;
                if (stage && stage->isEnabled() && stage->isInitialized() && stage->getOutput())
                {
                    finalTarget = stage->getOutput();
                    break;
                }
            }

            // Debug the final output before blitting to screen
            if (m_debugger && m_debugger->isEnabled() && finalTarget)
            {
                m_debugger->captureRenderTarget(finalTarget, "FinalOutput", m_frameNumber,
                                                context->getNearPlane(), context->getFarPlane());
            }

            // If we found a final target, blit it to screen
            if (finalTarget)
            {
                // Bind screen as draw target, final render target as read target
                glBindFramebuffer(GL_READ_FRAMEBUFFER, finalTarget->getFramebufferID());
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // Default framebuffer (screen)

                // Get viewport dimensions
                int viewportWidth = context->getViewport().z;
                int viewportHeight = context->getViewport().w;

                // Blit the final result to screen
                glBlitFramebuffer(
                    0, 0, finalTarget->getWidth(), finalTarget->getHeight(),
                    0, 0, viewportWidth, viewportHeight,
                    GL_COLOR_BUFFER_BIT,
                    GL_NEAREST
                );

                // Unbind framebuffers
                glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

                Core::debug("RenderPipeline: Blitted final result to screen from " +
                            std::to_string(finalTarget->getWidth()) + "x" +
                            std::to_string(finalTarget->getHeight()) + " to " +
                            std::to_string(viewportWidth) + "x" +
                            std::to_string(viewportHeight));
            }
            else
            {
                Core::warn("No final render target found - nothing to display");

                // Debug: Check what stages are actually enabled and have outputs
                Core::warn("Stage status check:");
                for (const auto& stage : m_stages)
                {
                    if (stage)
                    {
                        Core::warn("  " + stage->getName() + ": enabled=" +
                                   (stage->isEnabled() ? "true" : "false") +
                                   ", initialized=" + (stage->isInitialized() ? "true" : "false") +
                                   ", hasOutput=" + (stage->getOutput() ? "true" : "false"));
                    }
                }
            }
        }
        else
        {
            // Custom output target specified - copy to it
            std::shared_ptr<RenderTarget> lastStageOutput = nullptr;
            for (auto it = m_stages.rbegin(); it != m_stages.rend(); it++)
            {
                auto& stage = *it;
                if (stage && stage->isEnabled() && stage->isInitialized() && stage->getOutput())
                {
                    lastStageOutput = stage->getOutput();
                    break;
                }
            }

            if (lastStageOutput && m_output)
            {
                // Blit from last stage to custom output
                lastStageOutput->blit(m_output, true, true, false);
            }
        }
    }

    void RenderPipeline::addStage(std::shared_ptr<RenderStage> stage)
    {
        if (!stage)
        {
            Core::error("Attempted to add null stage to pipeline");
            return;
        }

        m_stages.push_back(stage);
        Core::debug("Added stage of type '" + stage->getTypeName() + "' to pipeline");
    }

    void RenderPipeline::removeStage(const std::string& stageType)
    {
        auto it = std::find_if(m_stages.begin(), m_stages.end(),
                               [&stageType](const auto& stage) { return stage->getTypeName() == stageType; });

        if (it != m_stages.end())
        {
            m_stages.erase(it);
            Core::debug("Removed stage of type '" + stageType + "' from pipeline");
        }
    }

    void RenderPipeline::clearStages()
    {
        m_stages.clear();
        Core::debug("Cleared all stages from pipeline");
    }

    void RenderPipeline::setOutput(std::shared_ptr<RenderTarget> target)
    {
        m_output = target;
    }

    std::shared_ptr<RenderTarget> RenderPipeline::createIntermediateTarget(
        const std::string& name,
        const IntermediateTargetConfig& config)
    {
        auto target = std::make_shared<RenderTarget>(config.width, config.height, config.multisampled);
        if (!target->initialize())
        {
            Core::error("Failed to initialize intermediate target: " + name);
            return nullptr;
        }
        
        // Handle color attachment
        if (config.createColor)
        {
            if (config.sharedColorTexture)
            {
                // Use shared color texture
                glBindFramebuffer(GL_FRAMEBUFFER, target->getFramebufferID());
                glFramebufferTexture2D(GL_FRAMEBUFFER,
                                       GL_COLOR_ATTACHMENT0 + config.colorAttachmentIndex,
                                       config.multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D,
                                       config.sharedColorTexture->getID(), 0);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                Core::info("Target '" + name + "': Using shared color texture");
            }
            else
            {
                // Create new color attachment
                target->addColorAttachment(config.colorFormat, config.colorAttachmentIndex);
            }
        }

        // Handle depth attachment
        if (config.createDepth)
        {
            if (config.sharedDepthTexture)
            {
                // Use shared depth texture - this is the key feature for forward+ optimization
                glBindFramebuffer(GL_FRAMEBUFFER, target->getFramebufferID());
                
                // Debug: Check framebuffer status before attaching depth
                GLenum fbStatusBefore = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                Core::debug("  - Framebuffer status BEFORE depth attachment: 0x" + 
                           std::to_string(fbStatusBefore) + (fbStatusBefore == GL_FRAMEBUFFER_COMPLETE ? " (COMPLETE)" : " (INCOMPLETE)"));
                
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                       GL_TEXTURE_2D, config.sharedDepthTexture->getID(), 0);
                
                // Debug: Check for OpenGL errors after attachment
                GLenum glError = glGetError();
                if (glError != GL_NO_ERROR)
                {
                    Core::error("  - OpenGL error during depth attachment: 0x" + std::to_string(glError));
                }
                
                // Debug: Check framebuffer status after attaching depth
                GLenum fbStatusAfter = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                Core::debug("  - Framebuffer status AFTER depth attachment: 0x" + 
                           std::to_string(fbStatusAfter) + (fbStatusAfter == GL_FRAMEBUFFER_COMPLETE ? " (COMPLETE)" : " (INCOMPLETE)"));
                
                // Update RenderTarget's depth texture reference so getDepthTexture() works
                target->setSharedDepthTexture(config.sharedDepthTexture);

                // Special handling for depth-only targets - configure BEFORE unbinding
                if (!config.createColor && config.createDepth)
                {
                    Core::debug("Configuring depth-only framebuffer (no color attachments)");
                    // For depth-only framebuffers, we need to explicitly disable color operations
                    // This tells OpenGL we're not writing to any color buffer
                    glDrawBuffer(GL_NONE);
                    glReadBuffer(GL_NONE);
                    
                    // Check if this fixes the framebuffer completeness
                    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                    Core::debug("Depth-only framebuffer status: 0x" + std::to_string(status) + 
                               (status == GL_FRAMEBUFFER_COMPLETE ? " (COMPLETE)" : " (INCOMPLETE)"));
                }
                
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                Core::info("Target '" + name + "': Using shared depth texture for early-Z optimization");
                Core::info("  - Shared depth texture ID: " + std::to_string(config.sharedDepthTexture->getID()));
            }
            else
            {
                // Create new depth attachment using depth texture (OpenGL 4.3+ supports this)
                target->setDepthAttachment(config.depthFormat);
                
                // Special handling for depth-only targets - configure AFTER depth attachment
                if (!config.createColor && config.createDepth)
                {
                    Core::debug("Configuring depth-only framebuffer (no color attachments) for new depth attachment");
                    glBindFramebuffer(GL_FRAMEBUFFER, target->getFramebufferID());
                    
                    // For depth-only framebuffers, we need to explicitly disable color operations
                    // This tells OpenGL we're not writing to any color buffer
                    glDrawBuffer(GL_NONE);
                    glReadBuffer(GL_NONE);
                    
                    // Check if this fixes the framebuffer completeness
                    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                    Core::debug("Depth-only framebuffer status: 0x" + std::to_string(status) + 
                               (status == GL_FRAMEBUFFER_COMPLETE ? " (COMPLETE)" : " (INCOMPLETE)"));
                    
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                }
            }
        }

        if (!target->validate())
        {
            Core::error("Failed to validate intermediate target: " + name);
            return nullptr;
        }

        m_intermediateTargets[name] = target;
        Core::info("Created intermediate target: " + name + " (" + 
                   std::to_string(config.width) + "x" + std::to_string(config.height) + ")");
        return target;
    }

    std::shared_ptr<RenderTarget> RenderPipeline::createIntermediateTarget(
        const std::string& name, int width, int height,
        TextureFormat colorFormat, bool createDepth, bool multisampled)
    {
        IntermediateTargetConfig config;
        config.width = width;
        config.height = height;
        config.colorFormat = colorFormat;
        config.createColor = (colorFormat != TextureFormat::Depth16 && 
                            colorFormat != TextureFormat::Depth24 && 
                            colorFormat != TextureFormat::Depth32F);
        config.createDepth = createDepth || !config.createColor; // Depth-only targets always need depth
        config.multisampled = multisampled;
        
        return createIntermediateTarget(name, config);
    }

    std::shared_ptr<RenderTarget> RenderPipeline::getIntermediateTarget(const std::string& name)
    {
        auto it = m_intermediateTargets.find(name);
        return (it != m_intermediateTargets.end()) ? it->second : nullptr;
    }

    /**
     * @brief Create a shared depth texture for use between multiple targets
     * @param name Name for the shared texture
     * @param width Width in pixels
     * @param height Height in pixels
     * @param format Depth texture format
     * @return The created shared depth texture
     */
    std::shared_ptr<Texture> RenderPipeline::createSharedDepthTexture(
        const std::string& name, int width, int height,
        TextureFormat format)
    {
        auto sharedDepth = Core::makeShared<Texture>(name);

        sharedDepth->initialize(width, height, format);

        // Store reference to shared texture for cleanup
        m_sharedTextures[name] = sharedDepth;

        Core::info("Created shared depth texture: " + name + " (" +
                   std::to_string(width) + "x" + std::to_string(height) + ")");
        return sharedDepth;
    }

    std::shared_ptr<Texture> RenderPipeline::getSharedTexture(const std::string& name)
    {
        auto it = m_sharedTextures.find(name);
        return (it != m_sharedTextures.end()) ? it->second : nullptr;
    }

    void RenderPipeline::resize(int width, int height)
    {
        // Resize shared textures first
        for (auto& [name, texture] : m_sharedTextures)
        {
            texture->resize(width, height);
        }

        // Then resize render targets (they'll use the resized shared textures)
        for (auto& [name, target] : m_intermediateTargets)
        {
            target->resize(width, height);
        }
    }

    std::shared_ptr<RenderPipelineDebugger> RenderPipeline::getDebugger() const
    {
        return m_debugger;
    }

    void RenderPipeline::setDebuggingEnabled(bool enabled)
    {
        if (m_debugger)
        {
            m_debugger->setEnabled(enabled);

            if (enabled)
            {
                Core::info("RenderPipeline: Debugging enabled");
            }
            else
            {
                Core::info("RenderPipeline: Debugging disabled");
            }
        }
    }

    void RenderPipeline::enableStageDebugging(const std::string& stageName, bool captureColor, bool captureDepth)
    {
        if (m_debugger)
        {
            m_debugger->enableStage(stageName, captureColor, captureDepth);
            Core::info("RenderPipeline: Enabled debugging for stage: " + stageName);
        }
    }

    void RenderPipeline::disableStageDebugging(const std::string& stageName)
    {
        if (m_debugger)
        {
            m_debugger->disableStage(stageName);
            Core::info("RenderPipeline: Disabled debugging for stage: " + stageName);
        }
    }

    bool RenderPipeline::validate()
    {
        if (!validateStageDependencies())
        {
            return false;
        }

        for (const auto& stage : m_stages)
        {
            if (!stage)
            {
                Core::error("Found null stage in pipeline");
                return false;
            }
        }

        return true;
    }

    Utility::Serialization::SerializationResult RenderPipeline::serialize(Utility::Serialization::Serializer& serializer) const
    {
        auto result = serializer.beginObject("RenderPipeline");
        if (!result) return result;

        auto version = getVersion();
        result = serializer.writeField("version_major", static_cast<uint32_t>(version.major));
        if (!result) return result;

        result = serializer.writeField("version_minor", static_cast<uint32_t>(version.minor));
        if (!result) return result;

        result = serializer.writeField("version_patch", static_cast<uint32_t>(version.patch));
        if (!result) return result;

        PipelineConfig config;

        for (const auto& stage : m_stages)
        {
            config.stageTypes.push_back(stage->getTypeName());
        }

        for (const auto& [name, value] : m_globalParameters)
        {
            config.globalParameters[name] = value;
        }

        result = config.serialize(serializer);
        if (!result) return result;

        result = serializer.beginArray("stages", m_stages.size());
        if (!result) return result;

        for (const auto& stage : m_stages)
        {
            result = stage->serialize(serializer);
            if (!result) return result;
        }

        result = serializer.endArray();
        if (!result) return result;

        return serializer.endObject();
    }

    Utility::Serialization::SerializationResult RenderPipeline::deserialize(Utility::Serialization::Deserializer& deserializer)
    {
        auto result = deserializer.beginObject("RenderPipeline");
        if (!result) return result;

        uint32_t major, minor, patch;
        result = deserializer.readField("version_major", major);
        if (!result) return result;

        result = deserializer.readField("version_minor", minor);
        if (!result) return result;

        result = deserializer.readField("version_patch", patch);
        if (!result) return result;

        Utility::Serialization::VersionInfo fileVersion{major, minor, patch};
        auto currentVersion = getVersion();

        if (fileVersion.major != currentVersion.major)
        {
            Core::error("Incompatible pipeline version: " + std::to_string(fileVersion.major) +
                       " vs " + std::to_string(currentVersion.major));
            return Utility::Serialization::SerializationResult("Version mismatch");
        }

        PipelineConfig config;
        result = config.deserialize(deserializer);
        if (!result) return result;

        m_globalParameters = config.globalParameters;

        clearStages();

        size_t stageCount;
        result = deserializer.beginArray("stages", stageCount);
        if (!result) return result;

        for (size_t i = 0; i < stageCount; ++i)
        {
            auto stage = createStageFromType(config.stageTypes[i], config.globalParameters);
            if (stage)
            {
                result = stage->deserialize(deserializer);
                if (!result) return result;
                addStage(stage);
            }
        }

        result = deserializer.endArray();
        if (!result) return result;

        return deserializer.endObject();
    }

    bool RenderPipeline::saveToFile(const std::string& filepath, Utility::Serialization::SerializationFormat format)
    {
        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open())
        {
            Core::error("Failed to open file for writing: " + filepath);
            return false;
        }

        using SerializationFormat = Utility::Serialization::SerializationFormat;
        
        switch (format)
        {
            case SerializationFormat::Binary:
            {
                Utility::Serialization::BinarySerializer serializer(file);
                auto result = serialize(serializer);
                if (!result.success)
                {
                    Core::error("Failed to serialize pipeline: " + result.error);
                    return false;
                }
                break;
            }
            default:
                Core::error("Unsupported serialization format");
                return false;
        }

        Core::info("Saved pipeline to: " + filepath);
        return true;
    }

    bool RenderPipeline::loadFromFile(const std::string& filepath)
    {
        if (!Utility::FileSystem::fileExists(filepath))
        {
            Core::error("Pipeline file not found: " + filepath);
            return false;
        }

        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open())
        {
            Core::error("Failed to open file for reading: " + filepath);
            return false;
        }

        Utility::Serialization::BinaryDeserializer deserializer(file);
        auto result = deserialize(deserializer);
        if (!result.success)
        {
            Core::error("Failed to deserialize pipeline: " + result.error);
            return false;
        }

        Core::info("Loaded pipeline from: " + filepath);
        return true;
    }

    std::shared_ptr<RenderPipeline> RenderPipeline::createFromFile(const std::string& filepath)
    {
        auto pipeline = std::make_shared<RenderPipeline>();

        if (!pipeline->loadFromFile(filepath))
        {
            return nullptr;
        }

        return pipeline;
    }

    Utility::Serialization::VersionInfo RenderPipeline::getVersion() const
    {
        return {1, 0, 0};
    }

    void RenderPipeline::defineSchema(Utility::Serialization::Schema& schema)
    {
        schema.addField("version_major", Utility::Serialization::ValueType::UInt32, true);
        schema.addField("version_minor", Utility::Serialization::ValueType::UInt32, true);
        schema.addField("version_patch", Utility::Serialization::ValueType::UInt32, true);
        schema.addComplexField("config", PipelineConfig::getTypeName(), true);
        schema.addComplexArrayField("stages", "RenderStage", true);
    }

    void RenderPipeline::createDefaultStages()
    {
        // Get required subsystems from the application
        auto app = Core::Application::getInstance();
        if (!app)
        {
            Core::error("Cannot get Application instance for pipeline configuration");
            return;
        }

        auto shaderManager = app->getSubsystem<ShaderManager>();
        auto forwardPlusShaderManager = app->getSubsystem<ForwardPlusShaderManager>();
        auto renderSystem = app->getSubsystem<RenderSystem>();
        auto world = app->getSubsystem<ECS::World>();

        if (!shaderManager || !forwardPlusShaderManager || !renderSystem)
        {
            Core::error("Required shader managers not available for pipeline configuration");
            return;
        }
        
        std::shared_ptr<ECS::Registry> registry = nullptr;
        if (world)
        {
            registry = world->getRegistry();
        }

        // Get light manager and clustered light culler from render system
        auto& lightManager = renderSystem->getLightManager();
        auto lightManagerPtr = std::shared_ptr<LightManager>(&lightManager, [](LightManager*) {});

        auto& clusteredLightCuller = renderSystem->getClusteredLightCuller();
        auto clusteredLightCullerPtr = std::shared_ptr<ClusteredLightCuller>(&clusteredLightCuller, [](ClusteredLightCuller*) {});

        // Get all required forward+ shaders
        auto voxelShader = forwardPlusShaderManager->getForwardPlusShader("voxel_basic");
        auto transparencyShader = forwardPlusShaderManager->getForwardPlusShader("transparency");

        // Get all required standard shaders
        auto depthShader = shaderManager->getShader("voxel_depth");
        auto skyboxShader = shaderManager->getShader("skybox");
        auto shadowShader = shaderManager->getShader("shadow_mapping");
        auto velocityShader = shaderManager->getShader("velocity");
        auto uiShader = shaderManager->getShader("ui");

        // Get post-processing shaders
        auto toneMappingShader = shaderManager->getShader("postprocess_tonemap");
        auto paletteMappingShader = shaderManager->getShader("postprocess_palette");
        auto ditheringShader = shaderManager->getShader("postprocess_dither");
        auto bloomShader = shaderManager->getShader("postprocess_bloom");
        auto fxaaShader = shaderManager->getShader("postprocess_fxaa");
        auto pixelPerfectShader = shaderManager->getShader("postprocess_pixelperfect");

        // OPTIMAL FORWARD+ STAGE ORDER:
        // 1. ClusterUpdate -> 2. DepthPrepass -> 3. Shadow -> 4. Geometry -> 5. Transparency -> 6. PostProcess

        // 1. Add cluster update stage FIRST (required for forward+ lighting in geometry pass)
        if (lightManagerPtr && clusteredLightCullerPtr)
        {
            auto clusterUpdateStage = std::make_shared<Stages::ClusterUpdateStage>();
            clusterUpdateStage->setClusterDimensions(clusteredLightCullerPtr->getClusterData().dimensions);
            clusterUpdateStage->setForwardPlusShaderManager(forwardPlusShaderManager);
            clusterUpdateStage->setLightManager(lightManagerPtr);
            
            addStage(clusterUpdateStage);
            Core::info("Added ClusterUpdateStage to forward+ pipeline (FIRST for optimal ordering)");
        }
        else
        {
            Core::warn("LightManager or ClusteredLightCuller not available for cluster update stage");
        }

        // 2. Add depth prepass stage
        if (depthShader)
        {
            auto depthStage = std::make_shared<Stages::DepthPrepassStage>();
            depthStage->setDepthShader(depthShader);
            depthStage->setRenderSystem(renderSystem);
            depthStage->setRegistry(registry);
            depthStage->setOpaqueOnly(true);  // Only depth test opaque voxels
            depthStage->setDepthBias(0.0f);   // No bias needed for voxels
            
            addStage(depthStage);
            Core::info("Added DepthPrepassStage to forward+ pipeline");
        }
        else
        {
            Core::warn("Depth shader not available, skipping depth prepass stage");
        }

        // 3. Add shadow mapping stage (before geometry)
        if (shadowShader && lightManagerPtr)
        {
            auto shadowStage = std::make_shared<Stages::ShadowStage>();
            shadowStage->setShadowShader(shadowShader);
            shadowStage->setLightManager(lightManagerPtr);
            shadowStage->setRegistry(registry);
            shadowStage->setShadowMapResolution(1024);
            shadowStage->setShadowBias(0.005f);
            shadowStage->setCascadeCount(3);
            shadowStage->setSoftShadowsEnabled(false); // Keep it simple for voxels
            
            addStage(shadowStage);
            Core::info("Added ShadowStage to forward+ pipeline");
        }
        else
        {
            Core::warn("Shadow shader or LightManager not available for shadow stage");
        }

        // 4. Configure geometry stage for forward+ voxel rendering with lighting
        if (voxelShader)
        {
            auto geometryStage = std::make_shared<Stages::GeometryStage>();
            geometryStage->setForwardPlusShaderManager(forwardPlusShaderManager);
            geometryStage->setRegistry(registry);
            geometryStage->setRenderOpaque(true);                 // Render opaque voxels
            geometryStage->setRenderTransparent(false);           // Not transparent pass
            geometryStage->setDepthTest(true);                    // Enable depth testing
            geometryStage->setDepthWrite(true);                   // Write depth values
            geometryStage->setSortObjects(true);                  // Sort for efficiency
            geometryStage->setUseInstancing(true);                // Enable instancing for voxels
            
            addStage(geometryStage);
            Core::info("Added configured GeometryStage with forward+ voxel rendering to pipeline");
        }
        else
        {
            Core::error("Voxel shader not available, cannot configure geometry stage");
        }

        // 5. Add transparency stage for transparent objects
        if (transparencyShader)
        {
            auto transparencyStage = std::make_shared<Stages::TransparencyStage>();
            transparencyStage->setTransparencyShader(transparencyShader);
            transparencyStage->setForwardPlusShaderManager(forwardPlusShaderManager);
            transparencyStage->setRegistry(registry);
            transparencyStage->setTransparencyTechnique(Stages::TransparencyStage::TransparencyTechnique::AlphaBlending);
            transparencyStage->setAlphaToCoverageEnabled(false);
            transparencyStage->setAlphaDiscardThreshold(0.1f);
            transparencyStage->setSoftParticlesEnabled(false);
            
            addStage(transparencyStage);
            Core::info("Added TransparencyStage to forward+ pipeline");
        }
        else
        {
            Core::warn("Transparency shader not available for transparency stage");
        }

        // 6. Add post-processing stage
        if (toneMappingShader)
        {
            auto postProcessStage = std::make_shared<Stages::PostProcessStage>();
            postProcessStage->setToneMappingShader(toneMappingShader);
            postProcessStage->enableEffect(Stages::PostProcessStage::EffectType::ToneMapping);
            postProcessStage->setExposure(1.0f);
            
            // Configure additional post-processing effects
            if (paletteMappingShader) postProcessStage->setPaletteMappingShader(paletteMappingShader);
            if (ditheringShader) 
            {
                postProcessStage->setDitheringShader(ditheringShader);
                postProcessStage->setDitheringStrength(0.3f);
            }
            if (bloomShader) 
            {
                postProcessStage->setBloomShader(bloomShader);
                postProcessStage->setBloomThreshold(1.0f);
                postProcessStage->setBloomIntensity(0.8f);
            }
            if (fxaaShader) postProcessStage->setFXAAShader(fxaaShader);
            if (pixelPerfectShader) 
            {
                postProcessStage->setPixelPerfectShader(pixelPerfectShader);
                postProcessStage->setPixelScale(2);
            }
            
            addStage(postProcessStage);
            Core::info("Added PostProcessStage to forward+ pipeline");
        }
        else
        {
            Core::warn("Tone mapping shader not available for post-processing stage");
        }

        Core::info("Forward+ rendering pipeline configured with " + std::to_string(getStages().size()) + " stages in optimal order");
    }

    void RenderPipeline::setupStageConnections()
    {
        if (m_stages.empty())
        {
            Core::warn("No stages to connect in pipeline");
            return;
        }

        Core::info("Setting up forward+ stage connections");

        // Get intermediate targets
        auto depthBuffer = getIntermediateTarget("DepthBuffer");
        auto sceneColor = getIntermediateTarget("SceneColor");
        auto sceneColorFinal = getIntermediateTarget("SceneColorFinal");
        auto postProcessTemp = getIntermediateTarget("PostProcessTemp");
        auto finalColor = getIntermediateTarget("FinalColor");
        
        Core::debug("Stage connections - Retrieved intermediate targets:");
        Core::debug("  - DepthBuffer: " + std::string(depthBuffer ? "FOUND" : "MISSING"));
        Core::debug("  - SceneColor: " + std::string(sceneColor ? "FOUND" : "MISSING"));
        Core::debug("  - SceneColorFinal: " + std::string(sceneColorFinal ? "FOUND" : "MISSING"));
        Core::debug("  - PostProcessTemp: " + std::string(postProcessTemp ? "FOUND" : "MISSING"));
        Core::debug("  - FinalColor: " + std::string(finalColor ? "FOUND" : "MISSING"));

        for (auto& stage : m_stages)
        {
            std::string stageType = stage->getName();

            if (stageType == "ClusterUpdateStage")
            {
                // No render targets needed - updates GPU buffers
                Core::info("ClusterUpdateStage configured (no render targets)");
            }
            else if (stageType == "DepthPrepassStage")
            {
                stage->setOutput(depthBuffer);
                Core::info("DepthPrepassStage output -> DepthBuffer");
            }
            else if (stageType == "ShadowStage")
            {
                // Shadow stage manages its own targets
                Core::info("ShadowStage configured (manages own targets)");
            }
            else if (stageType == "GeometryStage")
            {
                stage->setInput(depthBuffer, 0);    // Read depth buffer
                stage->setOutput(sceneColor);       // Write color + depth
                Core::info("GeometryStage: DepthBuffer -> SceneColor");
            }
            else if (stageType == "TransparencyStage")
            {
                stage->setInput(sceneColor, 0);     // Read opaque scene
                stage->setOutput(sceneColorFinal);  // Write to different buffer
                Core::info("TransparencyStage: SceneColor -> SceneColorFinal");
            }
            else if (stageType == "PostProcessStage")
            {
                stage->setInput(sceneColorFinal, 0);  // Read final scene
                stage->setOutput(finalColor);    // Write to final color since final stage
                Core::info("PostProcessStage: SceneColorFinal -> FinalColor");
            }
        }

        Core::info("Completed fixed forward+ stage connections for pipeline");
    }

    bool RenderPipeline::validateStageDependencies()
    {
        // Fixed forward+ pipeline has predetermined dependencies - always valid
        return true;
    }

    std::shared_ptr<RenderStage> RenderPipeline::createStageFromType(const std::string& typeName,
        const std::unordered_map<std::string, Utility::Serialization::ParameterValue>& config)
    {
        (void)config;  // Currently unused

        // Create stage instances for forward+ pipeline
        if (typeName == "ClusterUpdateStage")
        {
            return std::make_shared<Stages::ClusterUpdateStage>();
        }

        if (typeName == "GeometryStage")
        {
            return std::make_shared<Stages::GeometryStage>();
        }

        if (typeName == "DepthPrepassStage")
        {
            return std::make_shared<Stages::DepthPrepassStage>();
        }

        if (typeName == "PostProcessStage")
        {
            return std::make_shared<Stages::PostProcessStage>();
        }
        
        if (typeName == "ShadowStage")
        {
            return std::make_shared<Stages::ShadowStage>();
        }

        if (typeName == "TransparencyStage")
        {
            return std::make_shared<Stages::TransparencyStage>();
        }
        
        if (typeName == "SkyboxStage")
        {
            return std::make_shared<Stages::SkyboxStage>();
        }
        
        if (typeName == "VelocityStage")
        {
            return std::make_shared<Stages::VelocityStage>();
        }
        
        if (typeName == "UIStage")
        {
            return std::make_shared<Stages::UIStage>();
        }

        Core::warn("Unknown stage type: " + typeName);
        return nullptr;
    }

} // namespace PixelCraft::Rendering