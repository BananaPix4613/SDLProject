// -------------------------------------------------------------------------
// RenderPipelineVerifier.h
// -------------------------------------------------------------------------
#pragma once

#include "Core/Logger.h"

#include <memory>
#include <string>
#include <vector>

namespace PixelCraft::Rendering
{

    class RenderPipeline;
    class RenderSystem;
    
    /**
     * @brief Verification system for render pipeline integrity
     */
    class RenderPipelineVerifier
    {
    public:
        struct VerificationResult
        {
            bool passed = false;
            std::vector<std::string> errors;
            std::vector<std::string> warnings;

            void addError(const std::string& error)
            {
                errors.push_back(error);
                passed = false;
            }

            void addWarning(const std::string& warning)
            {
                warnings.push_back(warning);
            }
        };

        /**
         * @brief Verify complete render pipeline integrity
         */
        static VerificationResult verifyPipeline(std::shared_ptr<RenderPipeline> pipeline);

        /**
         * @brief Verify render system setup
         */
        static VerificationResult verifyRenderSystem(std::shared_ptr<RenderSystem> renderSystem);

        /**
         * @brief Run complete rendering diagnostics
         */
        static void runDiagnostics(std::shared_ptr<RenderSystem> renderSystem);

    private:
        static void verifyStageConnections(std::shared_ptr<RenderPipeline> pipeline, VerificationResult& result);
        static void verifyStageResources(std::shared_ptr<RenderPipeline> pipeline, VerificationResult& result);
        static void verifyOpenGLState(VerificationResult& result);
    };

} // PixelCraft::Rendering