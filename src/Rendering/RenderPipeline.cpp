// -------------------------------------------------------------------------
// RenderPipeline.cpp
// -------------------------------------------------------------------------
#include "Rendering/RenderPipeline.h"
#include "Core/Logger.h"
#include "Core/Serialization/JsonSerializer.h"
#include "Utility/DebugDraw.h"
#include <unordered_set>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Rendering
{

    RenderPipeline::RenderPipeline(const std::string& name)
        : m_name(name)
        , m_initialized(false)
    {
    }

    RenderPipeline::~RenderPipeline()
    {
        if (m_initialized)
        {
            shutdown();
        }
    }

    bool RenderPipeline::initialize()
    {
        if (m_initialized)
        {
            Log::warn("RenderPipeline " + m_name + " already initialized");
            return true;
        }

        Log::info("Initializing RenderPipeline " + m_name);

        // Initialize all stages
        for (auto& stage : m_stages)
        {
            if (!stage->initialize())
            {
                Log::error("Failed to initialize stage " + stage->getName() +
                           " in pipeline " + m_name);
                return false;
            }
        }

        // Setup connections between stages
        setupStageConnections();

        // Validate the pipeline configuration
        if (!validate())
        {
            Log::error("Pipeline " + m_name + " validation failed");
            return false;
        }

        m_initialized = true;
        return true;
    }

    void RenderPipeline::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        Log::info("Shutting down RenderPipeline " + m_name);

        // Shutdown in reverse order
        for (auto it = m_stages.rbegin(); it != m_stages.rend(); ++it)
        {
            (*it)->shutdown();
        }

        // Clear all intermediate targets
        m_intermediateTargets.clear();

        m_initialized = false;
    }

    void RenderPipeline::render(const RenderContext& context)
    {
        if (!m_initialized)
        {
            Log::warn("Attempting to render with uninitialized pipeline " + m_name);
            return;
        }

        // Execute each stage in order
        for (auto& stage : m_stages)
        {
            if (stage->isEnabled())
            {
                stage->execute(context);
            }
        }
    }

    void RenderPipeline::addStage(std::shared_ptr<RenderStage> stage)
    {
        if (!stage)
        {
            Log::error("Attempted to add null stage to pipeline " + m_name);
            return;
        }

        // Check for duplicate stage names
        for (auto& existingStage : m_stages)
        {
            if (existingStage->getName() == stage->getName())
            {
                Log::warn("Stage with name " + stage->getName() +
                          " already exists in pipeline " + m_name);
                return;
            }
        }

        m_stages.push_back(stage);

        // If already initialized, initialize the new stage and update connections
        if (m_initialized)
        {
            if (!stage->initialize())
            {
                Log::error("Failed to initialize newly added stage " + stage->getName() +
                           " in pipeline " + m_name);
                m_stages.pop_back();
                return;
            }

            setupStageConnections();
        }
    }

    void RenderPipeline::removeStage(const std::string& stageName)
    {
        auto it = std::find_if(m_stages.begin(), m_stages.end(),
                               [&stageName](const std::shared_ptr<RenderStage>& stage) {
                                   return stage->getName() == stageName;
                               });

        if (it != m_stages.end())
        {
            // If initialized, shut down the stage before removing
            if (m_initialized)
            {
                (*it)->shutdown();
            }

            m_stages.erase(it);

            // Update connections if initialized
            if (m_initialized)
            {
                setupStageConnections();
            }
        }
    }

    void RenderPipeline::clearStages()
    {
        // If initialized, shut down all stages before clearing
        if (m_initialized)
        {
            for (auto& stage : m_stages)
            {
                stage->shutdown();
            }
        }

        m_stages.clear();
    }

    std::shared_ptr<RenderStage> RenderPipeline::getStageByName(const std::string& stageName)
    {
        auto it = std::find_if(m_stages.begin(), m_stages.end(),
                               [&stageName](const std::shared_ptr<RenderStage>& stage) {
                                   return stage->getName() == stageName;
                               });

        if (it != m_stages.end())
        {
            return *it;
        }

        return nullptr;
    }

    const std::vector<std::shared_ptr<RenderStage>>& RenderPipeline::getStages() const
    {
        return m_stages;
    }

    void RenderPipeline::setOutput(std::shared_ptr<RenderTarget> target)
    {
        m_output = target;

        // Update the final stage's output if we have stages
        if (!m_stages.empty())
        {
            m_stages.back()->setOutput(target);
        }
    }

    std::shared_ptr<RenderTarget> RenderPipeline::getOutput() const
    {
        return m_output;
    }

    std::shared_ptr<RenderTarget> RenderPipeline::createIntermediateTarget(
        const std::string& name,
        int width,
        int height,
        TextureFormat colorFormat,
        bool createDepth,
        bool multisampled)
    {

        // Check if a target with this name already exists
        auto it = m_intermediateTargets.find(name);
        if (it != m_intermediateTargets.end())
        {
            Log::warn("Intermediate target " + name + " already exists in pipeline " + m_name);
            return it->second;
        }

        // Create a new render target
        auto target = std::make_shared<RenderTarget>(name, width, height, colorFormat, createDepth, multisampled);
        if (!target->initialize())
        {
            Log::error("Failed to initialize intermediate target " + name + " in pipeline " + m_name);
            return nullptr;
        }

        m_intermediateTargets[name] = target;
        return target;
    }

    std::shared_ptr<RenderTarget> RenderPipeline::getIntermediateTarget(const std::string& name)
    {
        auto it = m_intermediateTargets.find(name);
        if (it != m_intermediateTargets.end())
        {
            return it->second;
        }

        return nullptr;
    }

    bool RenderPipeline::validate()
    {
        // Check if output is set
        if (!m_output)
        {
            Log::error("Pipeline " + m_name + " has no output target");
            return false;
        }

        // Check if we have any stages
        if (m_stages.empty())
        {
            Log::error("Pipeline " + m_name + " has no stages");
            return false;
        }

        // Check if all stages have outputs set
        for (auto& stage : m_stages)
        {
            if (!stage->getOutput())
            {
                Log::error("Stage " + stage->getName() + " in pipeline " + m_name + " has no output target");
                return false;
            }
        }

        // Validate stage dependencies
        if (!validateStageDependencies())
        {
            return false;
        }

        return true;
    }

    void RenderPipeline::saveToFile(const std::string& filepath)
    {
        Core::Serialization::JsonSerializer serializer;

        // Create root node
        auto root = std::make_shared<Core::Serialization::DataNode>("RenderPipeline");

        // Save basic pipeline info
        root->addChild("name")->set(m_name);

        // Save stages
        auto stagesNode = root->addChild("stages");
        for (size_t i = 0; i < m_stages.size(); ++i)
        {
            auto stageNode = stagesNode->addChild("stage");
            auto stage = m_stages[i];

            stageNode->addChild("name")->set(stage->getName());
            stageNode->addChild("type")->set(stage->getTypeName());
            stageNode->addChild("enabled")->set(stage->isEnabled());

            // Let the stage serialize its own parameters
            auto paramsNode = stageNode->addChild("parameters");
            stage->serialize(paramsNode);
        }

        // Save intermediate targets
        auto targetsNode = root->addChild("intermediateTargets");
        for (const auto& [name, target] : m_intermediateTargets)
        {
            auto targetNode = targetsNode->addChild("target");
            targetNode->addChild("name")->set(name);
            targetNode->addChild("width")->set(target->getWidth());
            targetNode->addChild("height")->set(target->getHeight());
            targetNode->addChild("colorFormat")->set(static_cast<int>(target->getColorFormat()));
            targetNode->addChild("hasDepth")->set(target->hasDepth());
            targetNode->addChild("multisampled")->set(target->isMultisampled());
        }

        // Save stage connections
        auto connectionsNode = root->addChild("connections");
        for (size_t i = 0; i < m_stages.size(); ++i)
        {
            auto stage = m_stages[i];
            auto connectionNode = connectionsNode->addChild("connection");

            connectionNode->addChild("stageName")->set(stage->getName());

            // Save inputs
            auto inputsNode = connectionNode->addChild("inputs");
            for (size_t j = 0; j < stage->getInputs().size(); ++j)
            {
                auto input = stage->getInputs()[j];
                if (input)
                {
                    inputsNode->addChild("input")->set(input->getName());
                }
                else
                {
                    inputsNode->addChild("input")->set("null");
                }
            }

            // Save output
            auto output = stage->getOutput();
            if (output)
            {
                connectionNode->addChild("output")->set(output->getName());
            }
            else
            {
                connectionNode->addChild("output")->set("null");
            }
        }

        // Write to file
        if (!serializer.writeToFile(root, filepath))
        {
            Log::error("Failed to save pipeline configuration to " + filepath);
        }
        else
        {
            Log::info("Saved pipeline configuration to " + filepath);
        }
    }

    bool RenderPipeline::loadFromFile(const std::string& filepath)
    {
        Core::Serialization::JsonSerializer serializer;

        // Clear existing pipeline
        shutdown();
        clearStages();
        m_intermediateTargets.clear();

        // Read from file
        auto root = serializer.readFromFile(filepath);
        if (!root)
        {
            Log::error("Failed to load pipeline configuration from " + filepath);
            return false;
        }

        // Load basic pipeline info
        if (root->hasChild("name"))
        {
            m_name = root->getChild("name")->as<std::string>();
        }

        // Load intermediate targets
        if (root->hasChild("intermediateTargets"))
        {
            auto targetsNode = root->getChild("intermediateTargets");
            for (auto& targetNode : targetsNode->getChildren())
            {
                std::string name = targetNode->getChild("name")->as<std::string>();
                int width = targetNode->getChild("width")->as<int>();
                int height = targetNode->getChild("height")->as<int>();
                TextureFormat colorFormat = static_cast<TextureFormat>(targetNode->getChild("colorFormat")->as<int>());
                bool hasDepth = targetNode->getChild("hasDepth")->as<bool>();
                bool multisampled = targetNode->getChild("multisampled")->as<bool>();

                createIntermediateTarget(name, width, height, colorFormat, hasDepth, multisampled);
            }
        }

        // Load stages (this requires a factory system to create the right stage types)
        // This is simplified for the example
        if (root->hasChild("stages"))
        {
            auto stagesNode = root->getChild("stages");
            for (auto& stageNode : stagesNode->getChildren())
            {
                std::string name = stageNode->getChild("name")->as<std::string>();
                std::string type = stageNode->getChild("type")->as<std::string>();
                bool enabled = stageNode->getChild("enabled")->as<bool>();

                // Create stage based on type
                std::shared_ptr<RenderStage> stage;

                // In a real implementation, we'd use a factory to create the right stage type
                // For now, we'll assume a hypothetical StageFactory
                stage = RenderStageFactory::createStage(type, name);

                if (!stage)
                {
                    Log::error("Failed to create stage of type " + type + " with name " + name);
                    continue;
                }

                stage->setEnabled(enabled);

                // Deserialize parameters
                if (stageNode->hasChild("parameters"))
                {
                    stage->deserialize(stageNode->getChild("parameters"));
                }

                addStage(stage);
            }
        }

        // Setup connections
        if (root->hasChild("connections"))
        {
            auto connectionsNode = root->getChild("connections");
            for (auto& connectionNode : connectionsNode->getChildren())
            {
                std::string stageName = connectionNode->getChild("stageName")->as<std::string>();

                auto stage = getStageByName(stageName);
                if (!stage)
                {
                    Log::error("Failed to find stage " + stageName + " for connection setup");
                    continue;
                }

                // Set inputs
                if (connectionNode->hasChild("inputs"))
                {
                    auto inputsNode = connectionNode->getChild("inputs");
                    auto inputs = inputsNode->getChildren();

                    for (size_t i = 0; i < inputs.size(); ++i)
                    {
                        std::string inputName = inputs[i]->as<std::string>();

                        if (inputName != "null")
                        {
                            std::shared_ptr<RenderTarget> target = getIntermediateTarget(inputName);
                            if (!target && m_output && m_output->getName() == inputName)
                            {
                                target = m_output;
                            }

                            if (target)
                            {
                                stage->setInput(target, i);
                            }
                            else
                            {
                                Log::error("Failed to find render target " + inputName + " for stage " + stageName);
                            }
                        }
                    }
                }

                // Set output
                if (connectionNode->hasChild("output"))
                {
                    std::string outputName = connectionNode->getChild("output")->as<std::string>();

                    if (outputName != "null")
                    {
                        std::shared_ptr<RenderTarget> target = getIntermediateTarget(outputName);
                        if (!target && m_output && m_output->getName() == outputName)
                        {
                            target = m_output;
                        }

                        if (target)
                        {
                            stage->setOutput(target);
                        }
                        else
                        {
                            Log::error("Failed to find render target " + outputName + " for stage " + stageName);
                        }
                    }
                }
            }
        }

        // Initialize the pipeline
        return initialize();
    }

    void RenderPipeline::debugDrawPipeline(const glm::vec2& position, float scale)
    {
        if (!m_initialized)
        {
            return;
        }

        Utility::DebugDraw::begin2D();

        const float nodeWidth = 150.0f * scale;
        const float nodeHeight = 40.0f * scale;
        const float nodeSpacing = 20.0f * scale;
        const float connectionOffset = 10.0f * scale;
        const float levelSpacing = 100.0f * scale;

        // Draw nodes for render targets
        std::unordered_map<std::string, glm::vec2> targetPositions;
        float targetY = position.y;

        // Draw output target first
        if (m_output)
        {
            glm::vec2 pos = position + glm::vec2(0.0f, targetY);
            Utility::DebugDraw::drawRect(pos, glm::vec2(nodeWidth, nodeHeight), glm::vec4(0.2f, 0.6f, 0.8f, 1.0f));
            Utility::DebugDraw::drawText(pos + glm::vec2(5.0f, 5.0f), m_output->getName(), glm::vec4(1.0f));
            targetPositions[m_output->getName()] = pos;
            targetY += nodeHeight + nodeSpacing;
        }

        // Draw intermediate targets
        for (const auto& [name, target] : m_intermediateTargets)
        {
            glm::vec2 pos = position + glm::vec2(0.0f, targetY);
            Utility::DebugDraw::drawRect(pos, glm::vec2(nodeWidth, nodeHeight), glm::vec4(0.1f, 0.5f, 0.7f, 1.0f));
            Utility::DebugDraw::drawText(pos + glm::vec2(5.0f, 5.0f), name, glm::vec4(1.0f));
            targetPositions[name] = pos;
            targetY += nodeHeight + nodeSpacing;
        }

        // Draw nodes for render stages
        std::unordered_map<std::string, glm::vec2> stagePositions;
        float stageX = position.x + nodeWidth + levelSpacing;
        float stageY = position.y;

        for (auto& stage : m_stages)
        {
            glm::vec2 pos = glm::vec2(stageX, stageY);
            glm::vec4 color = stage->isEnabled() ? glm::vec4(0.3f, 0.7f, 0.3f, 1.0f) : glm::vec4(0.7f, 0.3f, 0.3f, 1.0f);

            Utility::DebugDraw::drawRect(pos, glm::vec2(nodeWidth, nodeHeight), color);
            Utility::DebugDraw::drawText(pos + glm::vec2(5.0f, 5.0f), stage->getName(), glm::vec4(1.0f));

            stagePositions[stage->getName()] = pos;
            stageY += nodeHeight + nodeSpacing;
        }

        // Draw connections
        for (auto& stage : m_stages)
        {
            // Draw input connections
            for (size_t i = 0; i < stage->getInputs().size(); ++i)
            {
                auto input = stage->getInputs()[i];
                if (input)
                {
                    glm::vec2 stagePos = stagePositions[stage->getName()];
                    glm::vec2 targetPos = targetPositions[input->getName()];

                    glm::vec2 start = targetPos + glm::vec2(nodeWidth, nodeHeight / 2.0f + i * connectionOffset);
                    glm::vec2 end = stagePos + glm::vec2(0.0f, nodeHeight / 2.0f);

                    Utility::DebugDraw::drawLine(start, end, glm::vec4(0.8f, 0.8f, 0.2f, 1.0f));
                    Utility::DebugDraw::drawRect(start - glm::vec2(3.0f), glm::vec2(6.0f), glm::vec4(0.8f, 0.8f, 0.2f, 1.0f));
                    Utility::DebugDraw::drawRect(end - glm::vec2(3.0f), glm::vec2(6.0f), glm::vec4(0.8f, 0.8f, 0.2f, 1.0f));
                }
            }

            // Draw output connection
            auto output = stage->getOutput();
            if (output)
            {
                glm::vec2 stagePos = stagePositions[stage->getName()];
                glm::vec2 targetPos = targetPositions[output->getName()];

                glm::vec2 start = stagePos + glm::vec2(nodeWidth, nodeHeight / 2.0f);
                glm::vec2 end = targetPos + glm::vec2(0.0f, nodeHeight / 2.0f);

                Utility::DebugDraw::drawLine(start, end, glm::vec4(0.2f, 0.8f, 0.8f, 1.0f));
                Utility::DebugDraw::drawRect(start - glm::vec2(3.0f), glm::vec2(6.0f), glm::vec4(0.2f, 0.8f, 0.8f, 1.0f));
                Utility::DebugDraw::drawRect(end - glm::vec2(3.0f), glm::vec2(6.0f), glm::vec4(0.2f, 0.8f, 0.8f, 1.0f));
            }
        }

        Utility::DebugDraw::end2D();
    }

    const std::string& RenderPipeline::getName() const
    {
        return m_name;
    }

    void RenderPipeline::setupStageConnections()
    {
        // If no stages, nothing to do
        if (m_stages.empty())
        {
            return;
        }

        // For simple linear pipeline, connect stages in sequence
        for (size_t i = 0; i < m_stages.size() - 1; ++i)
        {
            // Create an intermediate target if needed
            std::string targetName = m_stages[i]->getName() + "Output";
            std::shared_ptr<RenderTarget> output = getIntermediateTarget(targetName);

            if (!output)
            {
                // Create a new target matching the output dimensions
                int width = 0;
                int height = 0;

                if (m_output)
                {
                    width = m_output->getWidth();
                    height = m_output->getHeight();
                }
                else
                {
                    // Default to some reasonable size
                    width = 1920;
                    height = 1080;
                }

                output = createIntermediateTarget(targetName, width, height);
            }

            // Set output of current stage
            m_stages[i]->setOutput(output);

            // Set input of next stage
            m_stages[i + 1]->setInput(output);
        }

        // Set output of last stage to pipeline output
        if (m_output)
        {
            m_stages.back()->setOutput(m_output);
        }
    }

    void RenderPipeline::resizeIntermediateTargets(int width, int height)
    {
        for (auto& [name, target] : m_intermediateTargets)
        {
            target->resize(width, height);
        }
    }

    bool RenderPipeline::validateStageDependencies()
    {
        // Check for output target existence
        if (!m_output)
        {
            Log::error("Pipeline " + m_name + " has no output target");
            return false;
        }

        // Check for cyclic dependencies - simplified version
        // In a real implementation, we'd do a proper graph traversal

        // For now, just check that all inputs exist before they're used
        std::unordered_set<std::string> availableTargets;

        // Add all intermediate targets
        for (const auto& [name, target] : m_intermediateTargets)
        {
            availableTargets.insert(name);
        }

        // Add output target
        if (m_output)
        {
            availableTargets.insert(m_output->getName());
        }

        // Check each stage
        for (auto& stage : m_stages)
        {
            // Check inputs
            for (auto& input : stage->getInputs())
            {
                if (input && availableTargets.find(input->getName()) == availableTargets.end())
                {
                    Log::error("Stage " + stage->getName() + " in pipeline " + m_name +
                               " uses input target " + input->getName() + " that doesn't exist yet");
                    return false;
                }
            }

            // Add its output to available targets
            auto output = stage->getOutput();
            if (output)
            {
                availableTargets.insert(output->getName());
            }
        }

        return true;
    }

    // ForwardRenderPipeline implementation
    ForwardRenderPipeline::ForwardRenderPipeline()
        : RenderPipeline("ForwardPipeline")
    {
    }

    ForwardRenderPipeline::~ForwardRenderPipeline()
    {
    }

    bool ForwardRenderPipeline::initialize()
    {
        // Create default stages before initializing
        createDefaultStages();

        return RenderPipeline::initialize();
    }

    void ForwardRenderPipeline::createDefaultStages()
    {
        // Clear any existing stages
        clearStages();

        // Create the standard stages for forward rendering

        // 1. Shadow Map Stage
        auto shadowStage = std::make_shared<ShadowMapRenderStage>("ShadowPass");
        addStage(shadowStage);

        // 2. Depth Prepass Stage
        auto depthStage = std::make_shared<DepthPrepassRenderStage>("DepthPrepass");
        addStage(depthStage);

        // 3. Skybox Stage
        auto skyboxStage = std::make_shared<SkyboxRenderStage>("SkyboxPass");
        addStage(skyboxStage);

        // 4. Opaque Geometry Stage
        auto opaqueStage = std::make_shared<OpaqueRenderStage>("OpaquePass");
        addStage(opaqueStage);

        // 5. Transparent Geometry Stage
        auto transparentStage = std::make_shared<TransparentRenderStage>("TransparentPass");
        addStage(transparentStage);

        // 6. Post-Processing Stage
        auto postProcessStage = std::make_shared<PostProcessRenderStage>("PostProcess");
        addStage(postProcessStage);

        // 7. UI Stage
        auto uiStage = std::make_shared<UIRenderStage>("UIPass");
        addStage(uiStage);

        // 8. Debug Stage
        auto debugStage = std::make_shared<DebugRenderStage>("DebugPass");
        addStage(debugStage);
    }

} // namespace PixelCraft::Rendering