// -------------------------------------------------------------------------
// RenderPipeline.h
// -------------------------------------------------------------------------
#pragma once

#include "Rendering/RenderPipelineDebugger.h"
#include "Rendering/RenderStage.h"
#include "Rendering/RenderTarget.h"
#include "Rendering/RenderContext.h"
#include "Utility/Serialization/SerializationUtility.h"
#include "Utility/Serialization/ParameterValue.h"

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <variant>

// In namespace PixelCraft
namespace PixelCraft::Rendering
{

    /**
     * @brief Pipeline configuration data for serialization
     */
    struct PipelineConfig
    {
        std::vector<std::string> stageTypes;                       ///< Stage type list
        std::unordered_map<std::string, std::string> connections;  ///< Stage input/output connections
        std::unordered_map<std::string, Utility::Serialization::ParameterValue> globalParameters;  ///< Global pipeline parameters
    
        /**
         * @brief Serialize the pipeline configuration
         * @param serializer The serializer to use
         * @return SerializationResult with success/failure status
         */
        Utility::Serialization::SerializationResult serialize(Utility::Serialization::Serializer& serializer) const;

        /**
         * @brief Deserialize the pipeline configuration
         * @param deserializer The deserializer to use
         * @return SerializationResult with success/failure status
         */
        Utility::Serialization::SerializationResult deserialize(Utility::Serialization::Deserializer& deserializer);

        /**
         * @brief Define the schema for pipeline configuration serialization
         * @param schema The schema to define
         */
        static void defineSchema(Utility::Serialization::Schema& schema);

        /**
         * @brief Get the type name for serialization
         * @return The type name string
         */
        static std::string getTypeName()
        {
            return "PipelineConfig";
        }
    };

    /**
     * @brief Fixed forward+ rendering pipeline optimized for single-pass shading
     *
     * The RenderPipeline implements a fixed, high-performance forward+ rendering
     * pipeline with optimal stage ordering: ClusterUpdate -> DepthPrepass -> Shadow -> 
     * Geometry -> Transparency -> PostProcess. This design eliminates the overhead
     * of flexible configuration in favor of maximum forward+ rendering performance.
     */
    class RenderPipeline
    {
    public:
        /**
         * @brief Configuration for creating intermediate targets with shared textures
         */
        struct IntermediateTargetConfig
        {
            int width = 1024;
            int height = 1024;
            TextureFormat colorFormat = TextureFormat::RGBA8;
            TextureFormat depthFormat = TextureFormat::Depth24;
            bool createColor = false;
            bool createDepth = false;
            bool multisampled = false;
            std::shared_ptr<Texture> sharedDepthTexture = nullptr;   // For sharing depth between targets
            std::shared_ptr<Texture> sharedColorTexture = nullptr;   // For sharing color between targets
            int colorAttachmentIndex = 0;                            // Which color attachment to use
        };

        /**
         * @brief Constructor
         */
        RenderPipeline();

        /**
         * @brief Destructor
         */
        ~RenderPipeline();

        /**
         * @brief Initialize the forward+ pipeline with fixed stage ordering
         * @return True if initialization was successful
         */
        bool initialize();

        /**
         * @brief Shut down the pipeline and release resources
         */
        void shutdown();

        /**
         * @brief Update the pipeline
         * @param deltaTime Time since last update
         */
        void update(float deltaTime);

        /**
         * @brief Execute the pipeline with the provided context
         * @param context The rendering context
         */
        void render(const std::shared_ptr<RenderContext> context);

        /**
         * @brief Add a render stage to the pipeline
         * @param stage The render stage to add
         */
        void addStage(std::shared_ptr<RenderStage> stage);

        /**
         * @brief Remove a render stage by type
         * @param stageType The type of the stage to remove
         */
        void removeStage(const std::string& stageType);

        /**
         * @brief Remove all render stages from the pipeline
         */
        void clearStages();

        /**
         * @brief Get a stage by type casting
         * @param stageType The type of the stage to retrieve
         * @return The render stage, or nullptr if not found
         */
        template<typename T>
        std::shared_ptr<T> getStage(const std::string& stageType)
        {
            for (const auto& stage : m_stages)
            {
                if (stage->getType() == stageType)
                {
                    return std::dynamic_pointer_cast<T>(stage);
                }
            }
            return nullptr;
        }

        /**
         * @brief Get all stages in the pipeline
         * @return Vector of render stages
         */
        const std::vector<std::shared_ptr<RenderStage>>& getStages() const
        {
            return m_stages;
        }

        /**
         * @brief Set the final output target of the pipeline
         * @param target The render target to output to
         */
        void setOutput(std::shared_ptr<RenderTarget> target);

        /**
         * @brief Get the final output target
         * @return The output render target
         */
        std::shared_ptr<RenderTarget> getOutput() const
        {
            return m_output;
        }

        /**
         * @brief Create an intermediate render target with enhanced configuration
         * @param name The name of the target
         * @param config Configuration for the target
         * @return The created render target
         */
        std::shared_ptr<RenderTarget> createIntermediateTarget(
            const std::string& name,
            const IntermediateTargetConfig& config);
        
        /**
         * @brief Create an intermediate render target
         * @param name The name of the target
         * @param width Width of the target
         * @param height Height of the target
         * @param colorFormat Color format of the target
         * @param createDepth Whether the target should create depth
         * @param multisampled Whether the target should be multisampled
         * @return The created render target
         */
        std::shared_ptr<RenderTarget> createIntermediateTarget(
            const std::string& name, int width, int height,
            TextureFormat colorFormat, bool createDepth, bool multisampled);

        /**
         * @brief Get an intermediate render target by name
         * @param name The name of the target to retrieve
         * @return The render target, or nullptr if not found
         */
        std::shared_ptr<RenderTarget> getIntermediateTarget(const std::string& name);

        /**
         * @brief Create a shared depth texture for use between multiple targets
         * @param name Name for the shared texture (for reference tracking)
         * @param width Width in pixels
         * @param height Height in pixels
         * @param format Depth texture format
         * @return The created shared depth texture
         */
        std::shared_ptr<Texture> createSharedDepthTexture(
            const std::string& name, int width, int height,
            TextureFormat format = TextureFormat::Depth24);
        
        /**
         * @brief Get a shared texture by name
         * @param name The name of the shared texture
         * @return The shared texture, or nullptr if not found
         */
        std::shared_ptr<Texture> getSharedTexture(const std::string& name);

        /**
         * @brief Resize all intermediate targets
         * @param width New width
         * @param height New height
         */
        void resize(int width, int height);

        /**
         * @brief Get the pipeline debugger
         * @return Shared pointer to the debugger
         */
        std::shared_ptr<RenderPipelineDebugger> getDebugger() const;

        /**
         * @brief Enable pipeline debugging
         * @param enabled True to enable debugging
         */
        void setDebuggingEnabled(bool enabled);

        /**
         * @brief Configure debugging for a specific stage
         * @param stageName Name of the stage
         * @param captureColor Whether to capture color buffer
         * @param captureDepth Whether to capture depth buffer
         */
        void enableStageDebugging(const std::string& stageName, bool captureColor = true, bool captureDepth = false);

        /**
         * @brief Disable debugging for a specific stage
         * @param stageName Name of the stage
         */
        void disableStageDebugging(const std::string& stageName);

        /**
         * @brief Set a parameter value for the entire pipeline
         * @param name The name of the parameter
         * @param value The value to set
         */
        template<typename T>
        void setParameter(const std::string& name, const T& value)
        {
            m_globalParameters[name] = Utility::Serialization::ParameterValue(value);

            // Propagate the parameter to all stages
            for (auto& stage : m_stages)
            {
                stage->setParameter(name, value);
            }
        }

        /**
         * @brief Get a parameter value from the pipeline
         * @param name The name of the parameter
         * @param defaultValue Default value to return if parameter doesn't exist
         * @return Parameter value or default value
         */
        template<typename T>
        T getParameter(const std::string& name, const T& defaultValue = T()) const
        {
            auto it = m_globalParameters.find(name);
            if (it != m_globalParameters.end())
            {
                return std::get<T>(it->second.getValue());
            }
            return defaultValue;
        }

        /**
         * @brief Validate the pipeline configuration
         * @return True if valid, false if invalid
         */
        bool validate();

        /**
         * @brief Serialize the pipeline configuration
         * @param serializer The serializer to use
         * @return SerializationResult with success/failure status
         */
        Utility::Serialization::SerializationResult serialize(Utility::Serialization::Serializer& serializer) const;

        /**
         * @brief Deserialize the pipeline configuration
         * @param deserializer The deserializer to use
         * @return SerializationResult with success/failure status
         */
        Utility::Serialization::SerializationResult deserialize(Utility::Serialization::Deserializer& deserializer);

        /**
         * @brief Save the pipeline configuration to a file
         * @param filepath The path to save to
         * @param format The serialization format to use
         * @return True if successful, false otherwise
         */
        bool saveToFile(const std::string& filepath,
                        Utility::Serialization::SerializationFormat format = Utility::Serialization::SerializationFormat::Binary);

        /**
         * @brief Load the pipeline configuration from a file
         * @param filepath The path to load from
         * @return True if successful, false otherwise
         */
        bool loadFromFile(const std::string& filepath);

        /**
         * @brief Create a pipeline from a configuration file
         * @param filepath The path to the configuration file
         * @return The created pipeline, or nullptr if loading failed
         */
        static std::shared_ptr<RenderPipeline> createFromFile(const std::string& filepath);

        /**
         * @brief Debug visualization of the pipeline
         * @param position Screen position to draw at
         * @param scale Scaling factor for the visualization
         */
        void debugDrawPipeline(const glm::vec2& position, float scale = 1.0f);

        /**
         * @brief Get the version info for this pipeline
         * @return Version information
         */
        Utility::Serialization::VersionInfo getVersion() const;

        /**
         * @brief Define the schema for pipeline serialization
         * @param schema The schema to define
         */
        static void defineSchema(Utility::Serialization::Schema& schema);

        /**
         * @brief Get the type name for serialization
         * @return The type name string
         */
        static std::string getTypeName()
        {
            return "RenderPipeline";
        }

    private:
        /**
         * @brief Create the default forward+ stages in optimal order
         */
        void createDefaultStages();

        /**
         * @brief Set up fixed connections between stages for forward+ rendering
         */
        void setupStageConnections();

        /**
         * @brief Validate stage dependencies
         * @return True if valid, false if invalid
         */
        bool validateStageDependencies();

        /**
         * @brief Create stage from type name and configuration
         * @param typeName The type of stage to create
         * @param config Stage-specific configuration
         * @return The created stage, or nullptr if creation failed
         */
        std::shared_ptr<RenderStage> createStageFromType(
            const std::string& typeName,
            const std::unordered_map<std::string, Utility::Serialization::ParameterValue>& config);

        // Member variables
        std::vector<std::shared_ptr<RenderStage>> m_stages;                ///< Fixed ordered stages
        std::shared_ptr<RenderTarget> m_output;                            ///< Final output target
        std::unordered_map<std::string, std::shared_ptr<RenderTarget>> m_intermediateTargets;  ///< Named intermediate targets
        std::unordered_map<std::string, std::shared_ptr<Texture>> m_sharedTextures;  ///< Named shared textures
        std::unordered_map<std::string, Utility::Serialization::ParameterValue> m_globalParameters;  ///< Global parameters
        bool m_initialized = false;                                        ///< Initialization flag

        // Debug variables
        std::shared_ptr<RenderPipelineDebugger> m_debugger;                ///< Pipeline debugger
        uint32_t m_frameNumber;                                            ///< Current frame number for debugging
    };

} // namespace PixelCraft::Rendering
