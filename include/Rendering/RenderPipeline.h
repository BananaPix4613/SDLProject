// -------------------------------------------------------------------------
// RenderPipeline.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Rendering/RenderContext.h"
#include "Rendering/RenderStage.h"
#include "Rendering/RenderTarget.h"
#include "Rendering/Texture.h"

namespace PixelCraft::Rendering
{

    /**
     * @brief Configurable multi-stage rendering pipeline
     *
     * The RenderPipeline orchestrates multiple render stages into a complete
     * rendering pipeline, managing dependencies, intermediate targets, and execution.
     */
    class RenderPipeline
    {
    public:
        /**
         * @brief Constructor
         * @param name The name of the pipeline
         */
        RenderPipeline(const std::string& name);

        /**
         * @brief Destructor
         */
        virtual ~RenderPipeline();

        /**
         * @brief Initialize the pipeline
         * @return True if initialization was successful
         */
        virtual bool initialize();

        /**
         * @brief Shut down the pipeline and release resources
         */
        void shutdown();

        /**
         * @brief Execute the pipeline with the provided context
         * @param context The rendering context
         */
        void render(const RenderContext& context);

        /**
         * @brief Add a render stage to the pipeline
         * @param stage The render stage to add
         */
        void addStage(std::shared_ptr<RenderStage> stage);

        /**
         * @brief Remove a render stage by name
         * @param stageName The name of the stage to remove
         */
        void removeStage(const std::string& stageName);

        /**
         * @brief Remove all render stages from the pipeline
         */
        void clearStages();

        /**
         * @brief Get a stage by name with type casting
         * @param stageName The name of the stage to retrieve
         * @return The render stage, or nullptr if not found
         */
        template<typename T>
        std::shared_ptr<T> getStage(const std::string& stageName)
        {
            auto stage = getStageByName(stageName);
            if (!stage)
            {
                return nullptr;
            }
            return std::dynamic_pointer_cast<T>(stage);
        }

        /**
         * @brief Get a stage by name
         * @param stageName The name of the stage to retrieve
         * @return The render stage, or nullptr if not found
         */
        std::shared_ptr<RenderStage> getStageByName(const std::string& stageName);

        /**
         * @brief Get all stages in the pipeline
         * @return Vector of render stages
         */
        const std::vector<std::shared_ptr<RenderStage>>& getStages() const;

        /**
         * @brief Set the final output target of the pipeline
         * @param target The render target to output to
         */
        void setOutput(std::shared_ptr<RenderTarget> target);

        /**
         * @brief Get the final output target
         * @return The output render target
         */
        std::shared_ptr<RenderTarget> getOutput() const;

        /**
         * @brief Create an intermediate render target
         * @param name The name of the target
         * @param width Width in pixels
         * @param height Height in pixels
         * @param colorFormat Format for the color attachment
         * @param createDepth Whether to create a depth attachment
         * @param multisampled Whether to enable multisampling
         * @return The created render target
         */
        std::shared_ptr<RenderTarget> createIntermediateTarget(
            const std::string& name,
            int width,
            int height,
            TextureFormat colorFormat = TextureFormat::RGBA8,
            bool createDepth = true,
            bool multisampled = false);

        /**
         * @brief Get an intermediate render target by name
         * @param name The name of the target to retrieve
         * @return The render target, or nullptr if not found
         */
        std::shared_ptr<RenderTarget> getIntermediateTarget(const std::string& name);

        /**
         * @brief Set a parameter value for the entire pipeline
         * @param name The name of the parameter
         * @param value The value to set
         */
        template<typename T>
        void setParameter(const std::string& name, const T& value)
        {
            // Propagate the parameter to all stages
            for (auto& stage : m_stages)
            {
                stage->setParameter(name, value);
            }
        }

        /**
         * @brief Validate the pipeline configuration
         * @return True if valid, false if invalid
         */
        bool validate();

        /**
         * @brief Save the pipeline configuration to a file
         * @param filepath The path to save to
         */
        void saveToFile(const std::string& filepath);

        /**
         * @brief Load the pipeline configuration from a file
         * @param filepath The path to load from
         * @return True if successful, false otherwise
         */
        bool loadFromFile(const std::string& filepath);

        /**
         * @brief Debug visualization of the pipeline
         * @param position Screen position to draw at
         * @param scale Scaling factor for the visualization
         */
        void debugDrawPipeline(const glm::vec2& position, float scale = 1.0f);

        /**
         * @brief Get the name of the pipeline
         * @return The pipeline name
         */
        const std::string& getName() const;

    private:
        /**
         * @brief Set up connections between stages
         */
        void setupStageConnections();

        /**
         * @brief Resize all intermediate targets
         * @param width New width
         * @param height New height
         */
        void resizeIntermediateTargets(int width, int height);

        /**
         * @brief Validate stage dependencies
         * @return True if valid, false if invalid
         */
        bool validateStageDependencies();

        // Member variables
        std::string m_name;
        bool m_initialized;
        std::vector<std::shared_ptr<RenderStage>> m_stages;
        std::shared_ptr<RenderTarget> m_output;
        std::unordered_map<std::string, std::shared_ptr<RenderTarget>> m_intermediateTargets;
    };

    /**
     * @brief Specialized pipeline for forward rendering
     */
    class ForwardRenderPipeline : public RenderPipeline
    {
    public:
        /**
         * @brief Constructor
         */
        ForwardRenderPipeline();

        /**
         * @brief Destructor
         */
        ~ForwardRenderPipeline() override;

        /**
         * @brief Initialize the forward rendering pipeline
         * @return True if initialization was successful
         */
        bool initialize() override;

    private:
        /**
         * @brief Create the default stages for forward rendering
         */
        void createDefaultStages();
    };

} // namespace PixelCraft::Rendering