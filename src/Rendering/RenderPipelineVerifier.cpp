// -------------------------------------------------------------------------
// RenderPipelineVerifier.cpp
// -------------------------------------------------------------------------

#include "Rendering/RenderPipelineVerifier.h"
#include "Rendering/RenderPipeline.h"
#include "Rendering/RenderSystem.h"
#include "Rendering/Stages/DepthPrepassStage.h"
#include "Rendering/Stages/ClusterUpdateStage.h"
#include "Rendering/Stages/GeometryStage.h"
#include "Rendering/Stages/PostProcessStage.h"
#include "Rendering/Stages/ShadowStage.h"
#include "Rendering/Stages/SkyboxStage.h"
#include "Rendering/Stages/TransparencyStage.h"
#include "Rendering/Stages/UIStage.h"
#include "Rendering/Stages/VelocityStage.h"

#include <glad/glad.h>

namespace PixelCraft::Rendering
{
    
    RenderPipelineVerifier::VerificationResult RenderPipelineVerifier::verifyPipeline(std::shared_ptr<RenderPipeline> pipeline)
    {
        VerificationResult result;
        result.passed = true;

        if (!pipeline)
        {
            result.addError("Pipeline is null");
            return result;
        }

        // Check if pipeline is initialized
        if (!pipeline->validate())
        {
            result.addError("Pipeline is not initialized");
        }

        // Check stages
        auto stages = pipeline->getStages();
        if (stages.empty())
        {
            result.addError("Pipeline has no stages");
        }

        // Verify each stage
        for (const auto& stage : stages)
        {
            if (!stage)
            {
                result.addError("Null stage found in pipeline");
                continue;
            }

            if (!stage->isInitialized())
            {
                result.addError("Stage '" + stage->getName() + "' is not initialized");
            }

            if (!stage->isEnabled())
            {
                result.addWarning("Stage '" + stage->getName() + "' is disabled");
            }

            // Check stage-specific requirements
            std::string stageType = stage->getTypeName();

            if (stageType == "GeometryStage")
            {
                auto geometryStage = std::dynamic_pointer_cast<Stages::GeometryStage>(stage);
                if (geometryStage && !geometryStage->getForwardPlusShaderManager())
                {
                    result.addError("GeometryStage has no forward plus shader manager");
                }
            }
            else if (stageType == "ClusteredUpdateStage")
            {
                auto clusterUpdateStage = std::dynamic_pointer_cast<Stages::ClusterUpdateStage>(stage);
                if (clusterUpdateStage)
                {
                    glm::vec3 dimensions = clusterUpdateStage->getClusterDimensions();
                    if (dimensions.x <= 0 || dimensions.y <= 0 || dimensions.z <= 0)
                    {
                        result.addError("ClusterUpdateStage has invalid cluster dimensions");
                    }
                }
            }
            else if (stageType == "PostProcessStage")
            {
                auto postProcessStage = std::dynamic_pointer_cast<Stages::PostProcessStage>(stage);
                if (postProcessStage)
                {
                    // Check if any effects are enabled
                    bool anyEffectEnabled = false;
                    for (int i = 0; i < 8; ++i)
                    {
                        if (postProcessStage->isEffectEnabled(static_cast<Stages::PostProcessStage::EffectType>(i)))
                        {
                            anyEffectEnabled = true;
                            break;
                        }
                    }
                    if (!anyEffectEnabled)
                    {
                        result.addWarning("PostProcessStage has no effects enabled");
                    }
                }
            }
        }

        // Verify stage connections
        verifyStageConnections(pipeline, result);

        // Verify stage resources
        verifyStageResources(pipeline, result);

        return result;
    }

    RenderPipelineVerifier::VerificationResult RenderPipelineVerifier::verifyRenderSystem(std::shared_ptr<RenderSystem> renderSystem)
    {
        VerificationResult result;
        result.passed = true;

        if (!renderSystem)
        {
            result.addError("RenderSystem is null");
            return result;
        }

        // Check if render system is initialized
        if (!renderSystem->isInitialized())
        {
            result.addError("RenderSystem is not initialized");
        }

        // Check forward+ pipeline
        auto pipeline = renderSystem->getRenderPipeline();
        if (!pipeline)
        {
            result.addError("Active forward+ pipeline not found");
        }
        else
        {
            // Verify the active pipeline
            auto pipelineResult = verifyPipeline(pipeline);
            result.errors.insert(result.errors.end(),
                                    pipelineResult.errors.begin(),
                                    pipelineResult.errors.end());
            result.warnings.insert(result.warnings.end(),
                                    pipelineResult.warnings.begin(),
                                    pipelineResult.warnings.end());
            if (!pipelineResult.passed)
            {
                result.passed = false;
            }
        }

        // Check main camera
        auto camera = renderSystem->getMainCamera();
        if (!camera)
        {
            result.addError("No main camera set");
        }

        // Check render context
        const auto& context = renderSystem->getRenderContext();
        auto viewport = context.getViewport();
        if (viewport.z == 0 || viewport.w == 0)
        {
            result.addError("Invalid viewport dimensions: " +
                            std::to_string(viewport.z) + "x" + std::to_string(viewport.w));
        }

        // Check OpenGL state
        verifyOpenGLState(result);

        return result;
    }

    void RenderPipelineVerifier::runDiagnostics(std::shared_ptr<RenderSystem> renderSystem)
    {
        Core::info("=== RENDER PIPELINE DIAGNOSTICS ===");

        // Verify render system
        auto result = verifyRenderSystem(renderSystem);

        if (result.passed)
        {
            Core::info("Render system verification PASSED");
        }
        else
        {
            Core::error("Render system verification FAILED");
        }

        // Log errors
        if (!result.errors.empty())
        {
            Core::error("ERRORS:");
            for (const auto& error : result.errors)
            {
                Core::error("  - " + error);
            }
        }

        // Log warnings
        if (!result.warnings.empty())
        {
            Core::warn("WARNINGS:");
            for (const auto& warning : result.warnings)
            {
                Core::warn("  - " + warning);
            }
        }

        // Additional diagnostics
        Core::info("=== RENDER QUEUES ===");
        Core::info("Opaque queue size: " + std::to_string(renderSystem->getOpaqueQueue().size()));
        Core::info("Transparent queue size: " + std::to_string(renderSystem->getTransparentQueue().size()));
        Core::info("UI queue size: " + std::to_string(renderSystem->getUIQueue().size()));

        Core::info("=== RENDER STATS ===");
        Core::info("Draw calls: " + std::to_string(renderSystem->getDrawCallCount()));
        Core::info("Triangles: " + std::to_string(renderSystem->getTriangleCount()));
        Core::info("Visible objects: " + std::to_string(renderSystem->getVisibleObjectCount()));

        // Check OpenGL errors
        GLenum glError = glGetError();
        if (glError != GL_NO_ERROR)
        {
            std::stringstream ss;
            ss << "OpenGL error detected: 0x" << std::hex << glError;
            Core::error(ss.str());
        }

        Core::info("=== END DIAGNOSTICS ===");
    }

    void RenderPipelineVerifier::verifyStageConnections(std::shared_ptr<RenderPipeline> pipeline, VerificationResult& result)
    {
        auto stages = pipeline->getStages();
        if (stages.empty())
        {
            result.addError("No stages to verify connections for");
            return;
        }

        Core::info("Verifying stage connections for pipeline with " + std::to_string(stages.size()) + " stages");

        // Verify the optimal forward+ pipeline flow we implemented:
        // ClusterUpdate → None
        // DepthPrepass → DepthBuffer (depth only)
        // Shadow → None
        // DepthBuffer → Geometry → SceneColor
        // SceneColor → Transparency → SceneColorFinal
        // SceneColorFinal → PostProcess → SceneColorFinal

        for (size_t i = 0; i < stages.size(); ++i)
        {
            auto stage = stages[i];
            std::string stageType = stage->getTypeName();

            // Verify stage-specific connection requirements
            if (stageType == "ClusterUpdateStage")
            {
                // ClusterUpdate has no targets, only updates GPU buffers
            }
            else if (stageType == "DepthPrepassStage")
            {
                // DepthPrepass should write to DepthBuffer and have no input
                auto output = stage->getOutput();
                if (!output)
                {
                    result.addError("DepthPrepassStage has no output render target");
                }
                else
                {
                    // Verify it's writing to DepthBuffer
                    auto depthBuffer = pipeline->getIntermediateTarget("DepthBuffer");
                    if (output != depthBuffer)
                    {
                        result.addWarning("DepthPrepassStage is not writing to 'DepthBuffer' intermediate target");
                    }
                }

                // DepthPrepass should not have input (it's the first stage)
                if (stage->hasInput(0))
                {
                    result.addWarning("DepthPrepassStage has input - depth prepass should be first stage with no input");
                }
            }
            else if (stageType == "ShadowStage")
            {
                // Shadow stage manages its own targets
            }
            else if (stageType == "GeometryStage")
            {
                // Geometry should read from DepthBuffer and write to SceneColor
                auto output = stage->getOutput();

                if (!output)
                {
                    result.addError("GeometryStage has no output render target");
                }
                else
                {
                    auto sceneColor = pipeline->getIntermediateTarget("SceneColor");
                    if (output != sceneColor)
                    {
                        result.addWarning("GeometryStage is not writing to 'SceneColor' intermediate target");
                    }
                }
            }
            else if (stageType == "TransparencyStage")
            {
                // Transparency should read SceneColor and write to SceneColorFinal
                auto output = stage->getOutput();
                auto input = stage->getInput(0);

                if (!output)
                {
                    result.addError("TransparencyStage has no output render target");
                }
                else
                {
                    auto sceneColorFinal = pipeline->getIntermediateTarget("SceneColorFinal");
                    if (output != sceneColorFinal)
                    {
                        result.addWarning("TransparencyStage is not writing to 'SceneColorFinal' intermediate target");
                    }
                }

                if (!input)
                {
                    result.addError("TransparencyStage has no input - should read opaque scene from SceneColor");
                }
                else
                {
                    auto sceneColor = pipeline->getIntermediateTarget("SceneColor");
                    if (input != sceneColor)
                    {
                        result.addWarning("TransparencyStage is not reading from 'SceneColor' intermediate target");
                    }
                }
            }
            // else if (stageType == "SkyboxStage")
            // {
            //     // Skybox should read and write to SceneColorFinal (additive)
            //     auto output = stage->getOutput();
            //     auto input = stage->getInput(0);

            //     auto sceneColorFinal = pipeline->getIntermediateTarget("SceneColorFinal");
            //     auto sceneColor = pipeline->getIntermediateTarget("SceneColor");

            //     if (!output)
            //     {
            //         result.addError("SkyboxStage has no output render target");
            //     }
            //     else if (output != sceneColorFinal && output != sceneColor)
            //     {
            //         result.addWarning("SkyboxStage is not writing to 'SceneColorFinal' or 'SceneColor' intermediate target");
            //     }

            //     if (!input)
            //     {
            //         result.addError("SkyboxStage has no input - should read existing scene for additive rendering");
            //     }
            //     else if (input != output)
            //     {
            //         result.addWarning("SkyboxStage input and output don't match - should read and write to same target for additive rendering");
            //     }
            // }
            else if (stageType == "PostProcessStage")
            {
                // PostProcess should read from SceneColorFinal and write to FinalColor (or PostProcessTemp if new stages added afterward later)
                auto output = stage->getOutput();
                auto input = stage->getInput(0);

                if (!output)
                {
                    result.addError("PostProcessStage has no output render target");
                }
                else
                {
                    auto finalColor = pipeline->getIntermediateTarget("FinalColor");
                    if (output != finalColor)
                    {
                        result.addWarning("PostProcessStage is not writing to 'FinalColor' intermediate target");
                    }
                }

                if (!input)
                {
                    result.addError("PostProcessStage has no input - should read final scene data");
                }
                else
                {
                    auto sceneColorFinal = pipeline->getIntermediateTarget("SceneColorFinal");
                    if (input != sceneColorFinal)
                    {
                        result.addWarning("PostProcessStage is not reading from 'SceneColorFinal' intermediate target");
                    }
                }
            }
            // else if (stageType == "UIStage")
            // {
            //     // UI should read from PostProcessTemp and write to FinalColor or screen (null)
            //     auto output = stage->getOutput();
            //     auto input = stage->getInput(0);

            //     // UI can write to screen (null output) or FinalColor target
            //     if (output)
            //     {
            //         auto finalColor = pipeline->getIntermediateTarget("FinalColor");
            //         if (output != finalColor)
            //         {
            //             result.addWarning("UIStage is not writing to 'FinalColor' intermediate target or screen");
            //         }
            //     }

            //     if (!input)
            //     {
            //         result.addError("UIStage has no input - should read processed scene data");
            //     }
            //     else
            //     {
            //         auto postProcessTemp = pipeline->getIntermediateTarget("PostProcessTemp");
            //         auto sceneColorFinal = pipeline->getIntermediateTarget("SceneColorFinal");
            //         auto sceneColor = pipeline->getIntermediateTarget("SceneColor");
            //         if (input != postProcessTemp && input != sceneColorFinal && input != sceneColor)
            //         {
            //             result.addWarning("UIStage is not reading from processed scene data");
            //         }
            //     }
            // }
            // else if (stageType == "VelocityStage" || stageType == "ShadowStage")
            // {
            //     // These are standalone stages that don't participate in main pipeline flow
            //     // No specific connection requirements
            //     Core::debug("Verified standalone stage: " + stageType);
            // }
            else
            {
                result.addWarning("Unknown stage type '" + stageType + "' - cannot verify connections");
            }
        }

        // Additional validation: Check for proper linear flow
        bool hasDepthPrepass = false;
        bool hasClusterUpdateStage = false;
        bool hasGeometry = false;

        for (const auto& stage : stages)
        {
            std::string stageType = stage->getTypeName();
            if (stageType == "DepthPrepassStage") hasDepthPrepass = true;
            else if (stageType == "ClusterUpdateStage") hasClusterUpdateStage = true;
            else if (stageType == "GeometryStage") hasGeometry = true;
        }

        // Core pipeline requirements
        if (!hasGeometry)
        {
            result.addError("Pipeline missing GeometryStage - required for rendering");
        }

        if (hasDepthPrepass && !hasGeometry)
        {
            result.addWarning("Pipeline has DepthPrepass but no Geometry stage");
        }

        if (hasGeometry && !hasClusterUpdateStage)
        {
            result.addWarning("Pipeline has Geometry but no Cluster Update stage - objects may appear unlit");
        }

        Core::info("Stage connections verification completed");
    }

    void RenderPipelineVerifier::verifyStageResources(std::shared_ptr<RenderPipeline> pipeline, VerificationResult& result)
    {
        // Check intermediate render targets match our fixed pipeline design
        auto depthBuffer = pipeline->getIntermediateTarget("DepthBuffer");
        if (!depthBuffer)
        {
            result.addError("Missing 'DepthBuffer' intermediate render target");
        }

        auto sceneColor = pipeline->getIntermediateTarget("SceneColor");
        if (!sceneColor)
        {
            result.addError("Missing 'SceneColor' intermediate render target");
        }

        auto sceneColorFinal = pipeline->getIntermediateTarget("SceneColorFinal");
        if (!sceneColorFinal)
        {
            result.addWarning("Missing 'SceneColorFinal' intermediate render target");
        }

        auto postProcessTemp = pipeline->getIntermediateTarget("PostProcessTemp");
        if (!postProcessTemp)
        {
            result.addWarning("Missing 'PostProcessTemp' intermediate render target");
        }

        auto finalColor = pipeline->getIntermediateTarget("FinalColor");
        if (!finalColor)
        {
            result.addWarning("Missing 'FinalColor' intermediate render target");
        }

        // Validate render target properties
        if (depthBuffer && !depthBuffer->isInitialized())
        {
            result.addError("DepthBuffer render target is not initialized");
        }

        if (sceneColor && !sceneColor->isInitialized())
        {
            result.addError("SceneColor render target is not initialized");
        }
    }

    void RenderPipelineVerifier::verifyOpenGLState(VerificationResult& result)
    {
        // Check if we have a valid OpenGL context
        GLint currentFramebuffer;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFramebuffer);
        
        // Check viewport
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        if (viewport[2] == 0 || viewport[3] == 0)
        {
            result.addError("OpenGL viewport has zero dimensions");
        }

        // Check for OpenGL errors
        GLenum error;
        while ((error = glGetError()) != GL_NO_ERROR)
        {
            std::stringstream ss;
            ss << "OpenGL error: 0x" << std::hex << error;
            result.addError(ss.str());
        }
    }

} // namespace PixelCraft::Rendering
