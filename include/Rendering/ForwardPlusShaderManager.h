// -------------------------------------------------------------------------
// ForwardPlusShaderManager.h
// -------------------------------------------------------------------------
#pragma once

#include "Core/Subsystem.h"
#include "Rendering/Shader.h"
#include "Rendering/Light.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

namespace PixelCraft::Rendering
{
    class ShaderManager;
    class ClusteredLightCuller;
    class RenderContext;
    struct ClusterData;
    struct LightData;

    /**
     * @brief Shader variant type for forward+ rendering
     */
    enum class ForwardPlusShaderType
    {
        Opaque,      ///< Opaque geometry with forward+ lighting
        Transparent, ///< Transparent geometry with forward+ lighting
        Standard     ///< Standard forward+ shader without variant specifics
    };

    /**
     * @brief Configuration for forward+ shader preprocessing
     */
    struct ForwardPlusShaderConfig
    {
        glm::ivec3 clusterDimensions = glm::ivec3(32, 18, 24); ///< Cluster grid dimensions
        int maxLightsPerCluster = 128;                         ///< Maximum lights per cluster
        bool enableAdaptiveDensity = true;                     ///< Enable adaptive density optimization
        bool enableVerticalParallax = true;                    ///< Enable vertical parallax for isometric
        bool enableDebugVisualization = false;                 ///< Enable cluster debug visualization
    };

    /**
     * @brief Specialized shader manager for forward+ rendering with cluster data integration
     * 
     * The ForwardPlusShaderManager handles the complexities of forward+ shaders, including
     * automatic binding of cluster buffers, light buffers, and cluster dimension uniforms.
     * This simplifies shader usage in render stages by encapsulating forward+ specific logic.
     */
    class ForwardPlusShaderManager : public Core::Subsystem
    {
    public:
        /**
         * @brief Constructor
         */
        ForwardPlusShaderManager();

        /**
         * @brief Destructor
         */
        virtual ~ForwardPlusShaderManager();

        /**
         * @brief Initialize the forward+ shader manager
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Update the shader manager (handles hot-reloading and cluster updates)
         * @param deltaTime Time since last update
         */
        void update(float deltaTime) override;

        /**
         * @brief Render debug visualization if enabled
         */
        void render() override;

        /**
         * @brief Shut down the shader manager and release resources
         */
        void shutdown() override;

        /**
         * @brief Get the name of the subsystem
         * @return The subsystem name
         */
        std::string getName() const override
        {
            return "ForwardPlusShaderManager";
        }

        /**
         * @brief Get dependencies required by this subsystem
         * @return List of dependency subsystem names
         */
        std::vector<std::string> getDependencies() const override;

        /**
         * @brief Set the base shader manager for delegation
         * @param shaderManager Base shader manager instance
         */
        void setShaderManager(std::shared_ptr<ShaderManager> shaderManager);

        /**
         * @brief Get the base shader manager
         * @return Base shader manager instance
         */
        std::shared_ptr<ShaderManager> getShaderManager() const;

        /**
         * @brief Set the clustered light culler for cluster data integration
         * @param culler Clustered light culler instance
         */
        void setClusteredLightCuller(std::shared_ptr<ClusteredLightCuller> culler);

        /**
         * @brief Get the clustered light culler
         * @return Clustered light culler instance
         */
        std::shared_ptr<ClusteredLightCuller> getClusteredLightCuller() const;

        /**
         * @brief Configure forward+ shader preprocessing
         * @param config Forward+ shader configuration
         */
        void configureShaders(const ForwardPlusShaderConfig& config);

        /**
         * @brief Get the current forward+ shader configuration
         * @return Current configuration
         */
        const ForwardPlusShaderConfig& getConfig() const;

        /**
         * @brief Create a forward+ shader with automatic cluster integration
         * @param name The shader name
         * @param vertexPath Path to vertex shader
         * @param fragmentPath Path to fragment shader
         * @param shaderType Type of forward+ shader variant
         * @return Shared pointer to the created shader
         */
        std::shared_ptr<Shader> createForwardPlusShader(
            const std::string& name,
            const std::string& vertexPath,
            const std::string& fragmentPath,
            ForwardPlusShaderType shaderType = ForwardPlusShaderType::Standard
        );

        /**
         * @brief Create an opaque forward+ shader
         * @param name The shader name
         * @param vertexPath Path to vertex shader
         * @param fragmentPath Path to fragment shader
         * @return Shared pointer to the created shader
         */
        std::shared_ptr<Shader> createOpaqueShader(
            const std::string& name,
            const std::string& vertexPath,
            const std::string& fragmentPath
        );

        /**
         * @brief Create a transparent forward+ shader
         * @param name The shader name
         * @param vertexPath Path to vertex shader
         * @param fragmentPath Path to fragment shader
         * @return Shared pointer to the created shader
         */
        std::shared_ptr<Shader> createTransparentShader(
            const std::string& name,
            const std::string& vertexPath,
            const std::string& fragmentPath
        );

        /**
         * @brief Get an existing forward+ shader
         * @param name The shader name
         * @return Shared pointer to the shader or nullptr if not found
         */
        std::shared_ptr<Shader> getForwardPlusShader(const std::string& name);

        /**
         * @brief Bind cluster data to a forward+ shader
         * @param shader Shader to bind cluster data to
         * @param context Current render context for viewport information
         */
        void bindClusterData(std::shared_ptr<Shader> shader, const std::shared_ptr<RenderContext> context);

        /**
         * @brief Update cluster uniforms for a forward+ shader
         * @param shader Shader to update uniforms for
         * @param context Current render context
         */
        void updateClusterUniforms(std::shared_ptr<Shader> shader, const std::shared_ptr<RenderContext> context);

        /**
         * @brief Update visible lights data in GPU buffer
         * @param lightDataArray Array of light data to upload
         * @return True if update was successful
         */
        bool updateVisibleLights(const std::vector<LightData>& lightDataArray);

        /**
         * @brief Update all forward+ GPU buffers with current data
         * @param lightDataArray Array of light data
         * @param clusterData Optional cluster data (will fetch from culler if not provided)
         * @return True if update was successful
         */
        bool updateForwardPlusBuffers(const std::vector<LightData>& lightDataArray, const ClusterData* clusterData = nullptr);

        /**
         * @brief Bind all forward+ buffers to their designated binding points
         */
        void bindAllForwardPlusBuffers();

        /**
         * @brief Check if forward+ resources are available and ready
         * @return True if forward+ rendering is ready
         */
        bool isForwardPlusReady() const;

        /**
         * @brief Get the cluster data buffer for manual binding
         * @return OpenGL buffer ID for cluster data
         */
        unsigned int getClusterDataBuffer() const;

        /**
         * @brief Get the light grid buffer for manual binding
         * @return OpenGL buffer ID for light grid
         */
        unsigned int getLightGridBuffer() const;

        /**
         * @brief Get the light indices buffer for manual binding
         * @return OpenGL buffer ID for light indices
         */
        unsigned int getLightIndicesBuffer() const;

        /**
         * @brief Get the visible lights buffer for manual binding
         * @return OpenGL buffer ID for visible lights
         */
        unsigned int getVisibleLightsBuffer() const;

        /**
         * @brief Reload all forward+ shaders
         */
        void reloadAllForwardPlusShaders();

    private:
        /**
         * @brief Preprocess shader source with forward+ defines
         * @param source Original shader source
         * @param shaderType Type of forward+ shader variant
         * @return Preprocessed shader source with forward+ defines
         */
        std::string preprocessShaderSource(const std::string& source, ForwardPlusShaderType shaderType);

        /**
         * @brief Generate forward+ preprocessor defines
         * @param shaderType Type of forward+ shader variant
         * @return String containing all forward+ defines
         */
        std::string generateForwardPlusDefines(ForwardPlusShaderType shaderType);

        /**
         * @brief Create or update cluster data buffers
         */
        void createClusterBuffers();

        /**
         * @brief Update cluster data buffers with latest data
         */
        void updateClusterBuffers();

        /**
         * @brief Load shader source from file with preprocessing
         * @param filepath Path to the shader file
         * @param shaderType Type of forward+ shader variant
         * @return Preprocessed shader source
         */
        std::string loadAndPreprocessShader(const std::string& filepath, ForwardPlusShaderType shaderType);

        /**
         * @brief Check if cluster buffers need updating
         * @return True if buffers are dirty
         */
        bool areClusterBuffersDirty() const;

        /**
         * @brief Mark cluster buffers as dirty for next update
         */
        void markClusterBuffersDirty();

        /**
         * @brief Update only the visible lights buffer
         * @param lightDataArray Array of light data to upload
         */
        void updateVisibleLightsBuffer(const std::vector<LightData>& lightDataArray);

        /**
         * @brief Update cluster data from the cluster culler
         */
        void updateClusterDataFromCuller();

    private:
        /** Base shader manager for delegation */
        std::shared_ptr<ShaderManager> m_shaderManager;

        /** Clustered light culler for cluster data */
        std::shared_ptr<ClusteredLightCuller> m_clusteredLightCuller;

        /** Forward+ shader configuration */
        ForwardPlusShaderConfig m_config;

        /** Cache of forward+ shaders */
        std::unordered_map<std::string, std::shared_ptr<Shader>> m_forwardPlusShaders;

        /** OpenGL buffer objects for cluster data */
        unsigned int m_clusterDataBuffer = 0;
        unsigned int m_lightGridBuffer = 0;
        unsigned int m_lightIndicesBuffer = 0;
        unsigned int m_visibleLightsBuffer = 0;

        /** Buffer binding points for forward+ data */
        static constexpr int CLUSTER_DATA_BINDING = 0;
        static constexpr int LIGHT_GRID_BINDING = 1;
        static constexpr int LIGHT_INDICES_BINDING = 2;
        static constexpr int VISIBLE_LIGHTS_BINDING = 3;

        /** State tracking */
        bool m_initialized = false;
        bool m_clusterBuffersDirty = true;
        bool m_configDirty = true;
        bool m_visibleLightsBufferDirty = true;

        /** Performance tracking */
        float m_lastBufferUpdateTime = 0.0f;
        uint32_t m_frameCounter = 0;
    };

} // namespace PixelCraft::Rendering
