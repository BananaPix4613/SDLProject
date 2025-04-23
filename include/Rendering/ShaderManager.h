// -------------------------------------------------------------------------
// ShaderManager.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "Rendering/Shader.h"
#include "Core/Subsystem.h"

namespace PixelCraft::Rendering
{

    /**
     * @brief Manages shader resources, provides caching and utility methods
     */
    class ShaderManager : public Core::Subsystem
    {
    public:
        /**
         * @brief Constructor
         */
        ShaderManager();

        /**
         * @brief Destructor
         */
        virtual ~ShaderManager();

        /**
         * @brief Initialize the shader manager
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Update the shader manager (checks for hot-reloading)
         * @param deltaTime Time since last update
         */
        void update(float deltaTime) override;

        /**
         * @brief Render debug info (if enabled)
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
            return "ShaderManager";
        }

        /**
         * @brief Get dependencies required by this subsystem
         * @return List of dependency subsystem names
         */
        std::vector<std::string> getDependencies() const override;

        /**
         * @brief Create or get a shader
         * @param name The shader name
         * @return Shared pointer to the shader
         */
        std::shared_ptr<Shader> createShader(const std::string& name);

        /**
         * @brief Get an existing shader
         * @param name The shader name
         * @return Shared pointer to the shader or nullptr if not found
         */
        std::shared_ptr<Shader> getShader(const std::string& name);

        /**
         * @brief Create a standard shader with vertex and fragment stages
         * @param name The shader name
         * @param vertexPath Path to vertex shader
         * @param fragmentPath Path to fragment shader
         * @return Shared pointer to the created shader
         */
        std::shared_ptr<Shader> createStandardShader(
            const std::string& name,
            const std::string& vertexPath,
            const std::string& fragmentPath
        );

        /**
         * @brief Create a compute shader
         * @param name The shader name
         * @param computePath Path to compute shader
         * @return Shared pointer to the created shader
         */
        std::shared_ptr<Shader> createComputeShader(
            const std::string& name,
            const std::string& computePath
        );

        /**
         * @brief Check if shader hot-reloading is enabled
         * @return True if hot-reloading is enabled
         */
        bool isHotReloadingEnabled() const
        {
            return m_hotReloadingEnabled;
        }

        /**
         * @brief Enable or disable shader hot-reloading
         * @param enabled Whether hot-reloading should be enabled
         */
        void setHotReloadingEnabled(bool enabled)
        {
            m_hotReloadingEnabled = enabled;
        }

        /**
         * @brief Reload all shaders
         */
        void reloadAllShaders();

    private:
        /**
         * @brief Check for modified shader files
         */
        void checkForModifiedShaders();

    private:
        std::unordered_map<std::string, std::shared_ptr<Shader>> m_shaders;
        std::unordered_map<std::string, std::filesystem::file_time_type> m_fileTimestamps;
        bool m_initialized;
        bool m_hotReloadingEnabled;
        float m_hotReloadCheckTimer;
        const float m_hotReloadCheckInterval = 1.0f; // Check every second
    };

} // namespace PixelCraft::Rendering