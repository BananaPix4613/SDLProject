// -------------------------------------------------------------------------
// ForwardPlusShaderManager.cpp
// -------------------------------------------------------------------------
#include "Rendering/ForwardPlusShaderManager.h"
#include "Rendering/ShaderManager.h"
#include "Rendering/ClusteredLightCuller.h"
#include "Rendering/RenderContext.h"
#include "Rendering/Light.h"
#include "Core/Application.h"
#include "Core/Logger.h"
#include "Utility/FileSystem.h"

#include <sstream>
#include <fstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace PixelCraft::Rendering
{

    ForwardPlusShaderManager::ForwardPlusShaderManager()
        : m_initialized(false)
        , m_clusterBuffersDirty(true)
        , m_configDirty(true)
        , m_visibleLightsBufferDirty(true)
        , m_lastBufferUpdateTime(0.0f)
        , m_frameCounter(0)
    {
    }

    ForwardPlusShaderManager::~ForwardPlusShaderManager()
    {
        if (m_initialized)
        {
            shutdown();
        }
    }

    bool ForwardPlusShaderManager::initialize()
    {
        if (m_initialized)
        {
            Core::warn("ForwardPlusShaderManager already initialized");
            return true;
        }

        m_shaderManager = Core::Application::getInstance()->getSubsystem<Rendering::ShaderManager>();
        if (!m_shaderManager)
        {
            Core::error("ForwardPlusShaderManager: Could not find shader manager");
            return false;
        }

        Core::info("Initializing ForwardPlusShaderManager");

        // Create OpenGL buffers for cluster data
        createClusterBuffers();

        if (m_clusterDataBuffer == 0 || m_lightGridBuffer == 0 ||
            m_lightIndicesBuffer == 0 || m_visibleLightsBuffer == 0)
        {
            Core::error("ForwardPlusShaderManager: Failed to create cluster data buffers");
            return false;
        }

        m_initialized = true;
        Core::info("ForwardPlusShaderManager initialized successfully");
        return true;
    }

    void ForwardPlusShaderManager::update(float deltaTime)
    {
        if (!m_initialized)
        {
            return;
        }

        m_frameCounter++;

        // Update cluster buffers if needed
        if (areClusterBuffersDirty())
        {
            updateClusterBuffers();
        }

        // Update visible lights buffer if needed
        if (m_visibleLightsBufferDirty)
        {
            // Note: Visible lights are updated by LightManager calling updateVisibleLights()
            // This just ensures the flag is reset when buffers are updated
            m_visibleLightsBufferDirty = false;
        }

        // Delegate hot-reloading to base shader manager
        m_shaderManager->update(deltaTime);
    }

    void ForwardPlusShaderManager::render()
    {
        // Render debug visualization if enabled
        if (m_config.enableDebugVisualization)
        {
            // Debug visualization would be implemented here
            // For now, just delegate to base shader manager
            m_shaderManager->render();
        }
    }

    void ForwardPlusShaderManager::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        Core::info("Shutting down ForwardPlusShaderManager");

        // Clean up OpenGL buffers
        if (m_clusterDataBuffer != 0)
        {
            glDeleteBuffers(1, &m_clusterDataBuffer);
            m_clusterDataBuffer = 0;
        }
        if (m_lightGridBuffer != 0)
        {
            glDeleteBuffers(1, &m_lightGridBuffer);
            m_lightGridBuffer = 0;
        }
        if (m_lightIndicesBuffer != 0)
        {
            glDeleteBuffers(1, &m_lightIndicesBuffer);
            m_lightIndicesBuffer = 0;
        }
        if (m_visibleLightsBuffer != 0)
        {
            glDeleteBuffers(1, &m_visibleLightsBuffer);
            m_visibleLightsBuffer = 0;
        }

        // Clear shader caches
        m_forwardPlusShaders.clear();

        // Reset dependencies
        m_shaderManager.reset();
        m_clusteredLightCuller.reset();

        m_initialized = false;
    }

    std::vector<std::string> ForwardPlusShaderManager::getDependencies() const
    {
        return {"MemoryManager", "ShaderManager"};
    }

    void ForwardPlusShaderManager::setShaderManager(std::shared_ptr<ShaderManager> shaderManager)
    {
        m_shaderManager = shaderManager;
    }

    std::shared_ptr<ShaderManager> ForwardPlusShaderManager::getShaderManager() const
    {
        return m_shaderManager;
    }

    void ForwardPlusShaderManager::setClusteredLightCuller(std::shared_ptr<ClusteredLightCuller> culler)
    {
        m_clusteredLightCuller = culler;
        markClusterBuffersDirty();
    }

    std::shared_ptr<ClusteredLightCuller> ForwardPlusShaderManager::getClusteredLightCuller() const
    {
        return m_clusteredLightCuller;
    }

    void ForwardPlusShaderManager::configureShaders(const ForwardPlusShaderConfig& config)
    {
        if (m_config.clusterDimensions != config.clusterDimensions ||
            m_config.maxLightsPerCluster != config.maxLightsPerCluster ||
            m_config.enableAdaptiveDensity != config.enableAdaptiveDensity ||
            m_config.enableVerticalParallax != config.enableVerticalParallax ||
            m_config.enableDebugVisualization != config.enableDebugVisualization)
        {
            m_config = config;
            m_configDirty = true;
            markClusterBuffersDirty();

            Core::info("ForwardPlusShaderManager: Configuration updated - cluster dimensions: " +
                       std::to_string(config.clusterDimensions.x) + "x" +
                       std::to_string(config.clusterDimensions.y) + "x" +
                       std::to_string(config.clusterDimensions.z));
        }
    }

    const ForwardPlusShaderConfig& ForwardPlusShaderManager::getConfig() const
    {
        return m_config;
    }

    std::shared_ptr<Shader> ForwardPlusShaderManager::createForwardPlusShader(
        const std::string& name,
        const std::string& vertexPath,
        const std::string& fragmentPath,
        ForwardPlusShaderType shaderType)
    {
        if (!m_shaderManager)
        {
            Core::error("ForwardPlusShaderManager: Cannot create shader without base ShaderManager");
            return nullptr;
        }

        // Check if shader already exists
        auto it = m_forwardPlusShaders.find(name);
        if (it != m_forwardPlusShaders.end())
        {
            return it->second;
        }

        // Load and preprocess shader sources
        std::string vertexSource = loadAndPreprocessShader(vertexPath, shaderType);
        std::string fragmentSource = loadAndPreprocessShader(fragmentPath, shaderType);

        if (vertexSource.empty() || fragmentSource.empty())
        {
            Core::error("ForwardPlusShaderManager: Failed to load shader sources for: " + name);
            return nullptr;
        }

        // Create shader through base manager with preprocessed sources
        auto shader = m_shaderManager->createShaderFromSource(name, vertexSource, fragmentSource);
        if (!shader)
        {
            Core::error("ForwardPlusShaderManager: Failed to create forward+ shader: " + name);
            return nullptr;
        }

        // Cache the forward+ shader
        m_forwardPlusShaders[name] = shader;

        Core::info("ForwardPlusShaderManager: Created forward+ shader: " + name);
        return shader;
    }

    std::shared_ptr<Shader> ForwardPlusShaderManager::createOpaqueShader(
        const std::string& name,
        const std::string& vertexPath,
        const std::string& fragmentPath)
    {
        return createForwardPlusShader(name, vertexPath, fragmentPath, ForwardPlusShaderType::Opaque);
    }

    std::shared_ptr<Shader> ForwardPlusShaderManager::createTransparentShader(
        const std::string& name,
        const std::string& vertexPath,
        const std::string& fragmentPath)
    {
        return createForwardPlusShader(name, vertexPath, fragmentPath, ForwardPlusShaderType::Transparent);
    }

    std::shared_ptr<Shader> ForwardPlusShaderManager::getForwardPlusShader(const std::string& name)
    {
        auto it = m_forwardPlusShaders.find(name);
        if (it != m_forwardPlusShaders.end())
        {
            return it->second;
        }

        // Also check base shader manager
        auto shader = m_shaderManager->getShader(name);
        if (shader)
        {
            return shader;
        }

        return nullptr;
    }

    void ForwardPlusShaderManager::bindClusterData(std::shared_ptr<Shader> shader, const std::shared_ptr<RenderContext> context)
    {
        if (!shader || !context || !isForwardPlusReady())
        {
            return;
        }

        // Bind all forward+ buffers
        bindAllForwardPlusBuffers();

        // Update cluster uniforms
        updateClusterUniforms(shader, context);
    }

    void ForwardPlusShaderManager::updateClusterUniforms(std::shared_ptr<Shader> shader, const std::shared_ptr<RenderContext> context)
    {
        if (!shader || !context || !m_clusteredLightCuller)
        {
            return;
        }

        const auto& clusterData = m_clusteredLightCuller->getClusterData();

        // Set cluster-related uniforms
        if (shader->hasUniform("clusterDimensions"))
        {
            shader->setIVec3("clusterDimensions", clusterData.dimensions);
        }
        if (shader->hasUniform("clusterMinBounds"))
        {
            shader->setVec3("clusterMinBounds", clusterData.minBounds);
        }
        if (shader->hasUniform("clusterMaxBounds"))
        {
            shader->setVec3("clusterMaxBounds", clusterData.maxBounds);
        }
        if (shader->hasUniform("clusterSize"))
        {
            shader->setVec3("clusterSize", clusterData.clusterSize);
        }

        // Set viewport information for cluster calculation
        const auto& viewport = context->getViewport();
        if (shader->hasUniform("viewportSize"))
        {
            shader->setVec2("viewportSize", glm::vec2(viewport.z, viewport.w));
        }

        // Set frame counter for temporal effects
        if (shader->hasUniform("frameCounter"))
        {
            shader->setInt("frameCounter", static_cast<int>(m_frameCounter));
        }
    }

    bool ForwardPlusShaderManager::updateVisibleLights(const std::vector<LightData>& lightDataArray)
    {
        if (!m_initialized)
        {
            Core::warn("ForwardPlusShaderManager not initialized - cannot update visible lights");
            return false;
        }

        updateVisibleLightsBuffer(lightDataArray);
        m_visibleLightsBufferDirty = false;
        return true;
    }

    bool ForwardPlusShaderManager::updateForwardPlusBuffers(const std::vector<LightData>& lightDataArray, const ClusterData* clusterData)
    {
        if (!m_initialized)
        {
            Core::warn("ForwardPlusShaderManager not initialized - cannot update forward+ buffers");
            return false;
        }

        // Update visible lights buffer
        updateVisibleLightsBuffer(lightDataArray);

        // Update cluster data if provided, otherwise update from culler
        if (clusterData)
        {
            // Update cluster buffers with provided data
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_clusterDataBuffer);
            glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ClusterData), clusterData, GL_DYNAMIC_DRAW);

            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_lightGridBuffer);
            glBufferData(GL_SHADER_STORAGE_BUFFER,
                         clusterData->lightGrid.size() * sizeof(uint32_t),
                         clusterData->lightGrid.data(), GL_DYNAMIC_DRAW);

            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_lightIndicesBuffer);
            glBufferData(GL_SHADER_STORAGE_BUFFER,
                         clusterData->lightIndices.size() * sizeof(uint32_t),
                         clusterData->lightIndices.data(), GL_DYNAMIC_DRAW);

            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        }
        else
        {
            // Update cluster data from culler
            updateClusterDataFromCuller();
        }

        m_visibleLightsBufferDirty = false;
        m_clusterBuffersDirty = false;

        // Check for OpenGL errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
        {
            Core::error("ForwardPlusShaderManager: OpenGL error during buffer update: " + std::to_string(error));
            return false;
        }

        Core::info("ForwardPlusShaderManager: Updated all forward+ buffers with " + 
                  std::to_string(lightDataArray.size()) + " lights");
        return true;
    }

    void ForwardPlusShaderManager::bindAllForwardPlusBuffers()
    {
        if (!m_initialized)
        {
            return;
        }

        // Bind cluster data buffers as shader storage buffers
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, CLUSTER_DATA_BINDING, m_clusterDataBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, LIGHT_GRID_BINDING, m_lightGridBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, LIGHT_INDICES_BINDING, m_lightIndicesBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, VISIBLE_LIGHTS_BINDING, m_visibleLightsBuffer);
    }

    bool ForwardPlusShaderManager::isForwardPlusReady() const
    {
        return m_initialized &&
               m_clusteredLightCuller &&
               m_clusteredLightCuller->isInitialized() &&
               m_clusterDataBuffer != 0 &&
               m_lightGridBuffer != 0 &&
               m_lightIndicesBuffer != 0 &&
               m_visibleLightsBuffer != 0;
    }

    unsigned int ForwardPlusShaderManager::getClusterDataBuffer() const
    {
        return m_clusterDataBuffer;
    }

    unsigned int ForwardPlusShaderManager::getLightGridBuffer() const
    {
        return m_lightGridBuffer;
    }

    unsigned int ForwardPlusShaderManager::getLightIndicesBuffer() const
    {
        return m_lightIndicesBuffer;
    }

    unsigned int ForwardPlusShaderManager::getVisibleLightsBuffer() const
    {
        return m_visibleLightsBuffer;
    }

    void ForwardPlusShaderManager::reloadAllForwardPlusShaders()
    {
        Core::info("ForwardPlusShaderManager: Reloading all forward+ shaders");

        for (auto& [name, shader] : m_forwardPlusShaders)
        {
            if (shader && shader->isLoaded())
            {
                if (!shader->onReload())
                {
                    Core::error("ForwardPlusShaderManager: Failed to reload forward+ shader: " + name);
                }
            }
        }

        // Also delegate to base shader manager
        m_shaderManager->reloadAllShaders();
    }

    std::string ForwardPlusShaderManager::preprocessShaderSource(const std::string& source, ForwardPlusShaderType shaderType)
    {
        std::stringstream result;
        std::istringstream sourceStream(source);
        std::string line;
        bool foundVersion = false;

        // First, find and extract the #version line
        while (std::getline(sourceStream, line))
        {
            // Trim whitespace for comparison
            std::string trimmedLine = line;
            trimmedLine.erase(0, trimmedLine.find_first_not_of(" \t"));

            if (trimmedLine.substr(0, 8) == "#version")
            {
                // Found #version line - add it first
                result << line << "\n";
                foundVersion = true;
                break;
            }
            else if (!trimmedLine.empty() && trimmedLine[0] != '/' && trimmedLine.substr(0, 2) != "//")
            {
                // Non-comment, non-empty line before #version - this is an error
                Core::error("ForwardPlusShaderManager: Found non-comment content before #version directive");
                break;
            }
            else
            {
                // Comment or empty line before #version - include it
                result << line << "\n";
            }
        }

        if (!foundVersion)
        {
            // No #version found - add a default one
            result << "#version 450 core\n";
            Core::warn("ForwardPlusShaderManager: No #version directive found, adding default #version 450 core");

            // Reset stream to beginning since we didn't find a version
            sourceStream.clear();
            sourceStream.seekg(0);
        }

        // Add forward+ defines after #version
        result << "\n";
        result << generateForwardPlusDefines(shaderType);
        result << "\n";

        // Add the rest of the source (everything after #version line)
        if (foundVersion)
        {
            while (std::getline(sourceStream, line))
            {
                result << line << "\n";
            }
        }
        else
        {
            // No version was found, add entire source
            result << source;
        }

        std::string finalSource = result.str();
    
        // Debug: Log the preprocessed shader for troubleshooting
        //Core::debug("ForwardPlusShaderManager: Preprocessed shader source:\n" + finalSource);
        
        return finalSource;
    }

    std::string ForwardPlusShaderManager::generateForwardPlusDefines(ForwardPlusShaderType shaderType)
    {
        std::stringstream defines;

        // Core forward+ defines
        defines << "#define FORWARD_PLUS_ENABLED\n";
        defines << "#define CLUSTER_COUNT_X " << m_config.clusterDimensions.x << "\n";
        defines << "#define CLUSTER_COUNT_Y " << m_config.clusterDimensions.y << "\n";
        defines << "#define CLUSTER_COUNT_Z " << m_config.clusterDimensions.z << "\n";
        defines << "#define MAX_LIGHTS_PER_CLUSTER " << m_config.maxLightsPerCluster << "\n";

        // Buffer binding points - use the constants to ensure consistency
        defines << "#define CLUSTER_DATA_BINDING " << CLUSTER_DATA_BINDING << "\n";
        defines << "#define LIGHT_GRID_BINDING " << LIGHT_GRID_BINDING << "\n";
        defines << "#define LIGHT_INDICES_BINDING " << LIGHT_INDICES_BINDING << "\n";
        defines << "#define VISIBLE_LIGHTS_BINDING " << VISIBLE_LIGHTS_BINDING << "\n";

        // Feature-specific defines
        if (m_config.enableAdaptiveDensity)
        {
            defines << "#define ENABLE_ADAPTIVE_DENSITY\n";
        }
        if (m_config.enableVerticalParallax)
        {
            defines << "#define ENABLE_VERTICAL_PARALLAX\n";
        }
        if (m_config.enableDebugVisualization)
        {
            defines << "#define ENABLE_DEBUG_VISUALIZATION\n";
        }

        // Shader type specific defines
        switch (shaderType)
        {
        case ForwardPlusShaderType::Opaque:
            defines << "#define FORWARD_PLUS_OPAQUE\n";
            break;
        case ForwardPlusShaderType::Transparent:
            defines << "#define FORWARD_PLUS_TRANSPARENT\n";
            defines << "#define ENABLE_ALPHA_BLENDING\n";
            break;
        case ForwardPlusShaderType::Standard:
            defines << "#define FORWARD_PLUS_STANDARD\n";
            break;
        }

        std::string result = defines.str();
        //Core::debug("ForwardPlusShaderManager: Generated defines:\n" + result);
        
        return result;
    }

    void ForwardPlusShaderManager::createClusterBuffers()
    {
        // Create cluster data buffer
        glGenBuffers(1, &m_clusterDataBuffer);
        glGenBuffers(1, &m_lightGridBuffer);
        glGenBuffers(1, &m_lightIndicesBuffer);
        glGenBuffers(1, &m_visibleLightsBuffer);

        //Core::debug("ForwardPlusShaderManager: Created cluster data buffers");
    }

    void ForwardPlusShaderManager::updateClusterBuffers()
    {
        updateClusterDataFromCuller();
        m_clusterBuffersDirty = false;
        m_lastBufferUpdateTime = static_cast<float>(glfwGetTime()) * 1000.0f; // Convert to milliseconds
    }

    std::string ForwardPlusShaderManager::loadAndPreprocessShader(const std::string& filepath, ForwardPlusShaderType shaderType)
    {
        try
        {
            // Load original shader source
            std::ifstream file(filepath);
            if (!file.is_open())
            {
                Core::error("ForwardPlusShaderManager: Cannot open shader file: " + filepath);
                return "";
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string source = buffer.str();

            // Preprocess with forward+ defines
            return preprocessShaderSource(source, shaderType);
        }
        catch (const std::exception& e)
        {
            Core::error("ForwardPlusShaderManager: Exception loading shader file: " + filepath + " | " + e.what());
            return "";
        }
    }

    bool ForwardPlusShaderManager::areClusterBuffersDirty() const
    {
        return m_clusterBuffersDirty;
    }

    void ForwardPlusShaderManager::markClusterBuffersDirty()
    {
        m_clusterBuffersDirty = true;
    }

    void ForwardPlusShaderManager::updateVisibleLightsBuffer(const std::vector<LightData>& lightDataArray)
    {
        if (m_visibleLightsBuffer == 0)
        {
            Core::error("ForwardPlusShaderManager: Visible lights buffer not initialized");
            return;
        }

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_visibleLightsBuffer);
        
        if (!lightDataArray.empty())
        {
            glBufferData(GL_SHADER_STORAGE_BUFFER, 
                         lightDataArray.size() * sizeof(LightData), 
                         lightDataArray.data(), 
                         GL_DYNAMIC_DRAW);
        }
        else
        {
            // Clear the buffer if no lights are visible
            glBufferData(GL_SHADER_STORAGE_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
        }

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Check for OpenGL errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
        {
            Core::error("ForwardPlusShaderManager: Failed to update visible lights buffer: " + std::to_string(error));
        }
        else
        {
            //Core::debug("ForwardPlusShaderManager: Updated visible lights buffer with " + 
            //           std::to_string(lightDataArray.size()) + " lights");
        }
    }

    void ForwardPlusShaderManager::updateClusterDataFromCuller()
    {
        if (!m_clusteredLightCuller || !m_clusteredLightCuller->isInitialized())
        {
            return;
        }

        const auto& clusterData = m_clusteredLightCuller->getClusterData();

        // Update cluster data buffer
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_clusterDataBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ClusterData), &clusterData, GL_DYNAMIC_DRAW);

        // Update light grid buffer
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_lightGridBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     clusterData.lightGrid.size() * sizeof(uint32_t),
                     clusterData.lightGrid.data(), GL_DYNAMIC_DRAW);

        // Update light indices buffer
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_lightIndicesBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     clusterData.lightIndices.size() * sizeof(uint32_t),
                     clusterData.lightIndices.data(), GL_DYNAMIC_DRAW);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        //Core::debug("ForwardPlusShaderManager: Updated cluster data buffers from culler");
    }

} // namespace PixelCraft::Rendering
