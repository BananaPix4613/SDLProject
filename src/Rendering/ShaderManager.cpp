// -------------------------------------------------------------------------
// ShaderManager.cpp
// -------------------------------------------------------------------------
#include "Rendering/ShaderManager.h"
#include "Core/Logger.h"
#include "Core/Application.h"
#include "Core/FileSystem.h"
#include <filesystem>
#include <exception>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Rendering
{

    ShaderManager::ShaderManager()
        : m_initialized(false)
        , m_hotReloadingEnabled(true)
        , m_hotReloadCheckTimer(0.0f)
    {
    }

    ShaderManager::~ShaderManager()
    {
        if (m_initialized)
        {
            shutdown();
        }
    }

    bool ShaderManager::initialize()
    {
        if (m_initialized)
        {
            Log::warn("ShaderManager already initialized");
            return true;
        }

        Log::info("Initializing ShaderManager");

        m_initialized = true;
        return true;
    }

    void ShaderManager::update(float deltaTime)
    {
        if (!m_initialized)
        {
            return;
        }

        // Check for hot-reloading if enabled
        if (m_hotReloadingEnabled)
        {
            m_hotReloadCheckTimer += deltaTime;

            if (m_hotReloadCheckTimer >= m_hotReloadCheckInterval)
            {
                checkForModifiedShaders();
                m_hotReloadCheckTimer = 0.0f;
            }
        }
    }

    void ShaderManager::render()
    {
        // Nothing to render for shader manager
    }

    void ShaderManager::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        Log::info("Shutting down ShaderManager");

        // Clear all cached shaders
        m_shaders.clear();

        m_initialized = false;
    }

    std::vector<std::string> ShaderManager::getDependencies() const
    {
        // ShaderManager only depends on core systems
        return {"ResourceManager"};
    }

    std::shared_ptr<Shader> ShaderManager::createShader(const std::string& name)
    {
        // Check if shader already exists
        auto it = m_shaders.find(name);
        if (it != m_shaders.end())
        {
            return it->second;
        }

        // Create a new shader
        auto shader = std::make_shared<Shader>(name);
        m_shaders[name] = shader;

        return shader;
    }

    std::shared_ptr<Shader> ShaderManager::getShader(const std::string& name)
    {
        auto it = m_shaders.find(name);
        if (it != m_shaders.end())
        {
            return it->second;
        }

        return nullptr;
    }

    std::shared_ptr<Shader> ShaderManager::createStandardShader(
        const std::string& name,
        const std::string& vertexPath,
        const std::string& fragmentPath)
    {
        auto shader = createShader(name);

        shader->setSourceFile(ShaderType::Vertex, vertexPath);
        shader->setSourceFile(ShaderType::Fragment, fragmentPath);

        if (!shader->load())
        {
            Log::error("Failed to load standard shader: " + name);
            m_shaders.erase(name);
            return nullptr;
        }

        return shader;
    }

    std::shared_ptr<Shader> ShaderManager::createComputeShader(
        const std::string& name,
        const std::string& computePath)
    {
        auto shader = createShader(name);

        shader->setSourceFile(ShaderType::Compute, computePath);

        if (!shader->load())
        {
            Log::error("Failed to load compute shader: " + name);
            m_shaders.erase(name);
            return nullptr;
        }

        return shader;
    }

    void ShaderManager::reloadAllShaders()
    {
        Log::info("Reloading all shaders");

        for (auto& [name, shader] : m_shaders)
        {
            if (shader->isLoaded())
            {
                if (!shader->onReload())
                {
                    Log::error("Failed to reload shader: " + name);
                }
            }
        }
    }

    void ShaderManager::checkForModifiedShaders()
    {
        for (auto& [name, shader] : m_shaders)
        {
            if (!shader->isLoaded())
            {
                continue;
            }

            bool needsReload = false;

            // Check each shader file for modifications
            for (const auto& [type, filepath] : shader->getFilePaths())
            {
                try
                {
                    auto lastModified = Core::FileSystem::getLastModifiedTime(filepath);

                    // If we haven't seen this file before, store its timestamp
                    if (m_fileTimestamps.find(filepath) == m_fileTimestamps.end())
                    {
                        m_fileTimestamps[filepath] = lastModified;
                        continue;
                    }

                    // Check if file has been modified since we last loaded it
                    auto cachedTime = m_fileTimestamps[filepath];
                    if (lastModified > cachedTime)
                    {
                        Log::debug("Shader file modified: " + filepath);
                        needsReload = true;
                        m_fileTimestamps[filepath] = lastModified;
                    }
                }
                catch (const std::exception& e)
                {
                    Log::warn("Failed to check shader file modification time: " + filepath + " | " + e.what());
                }
            }

            // Reload the shader if any of its source files were modified
            if (needsReload)
            {
                Log::info("Hot-reloading shader: " + name);
                if (!shader->onReload())
                {
                    Log::error("Failed to hot-reload shader: " + name);
                }
            }
        }
    }

} // namespace PixelCraft::Rendering