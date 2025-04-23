// -------------------------------------------------------------------------
// Shader.cpp
// -------------------------------------------------------------------------
#include "Rendering/Shader.h"
#include "Core/Logger.h"
#include "Core/FileSystem.h"
#include "gtc/type_ptr.hpp"

namespace Log = PixelCraft::Core;

namespace PixelCraft::Rendering
{

    Shader::Shader(const std::string& name)
        : Core::Resource(name)
        , m_programID(0)
        , m_isValid(false)
    {
        Log::debug("Creating shader: " + name);
    }

    Shader::~Shader()
    {
        unload();
    }

    bool Shader::load()
    {
        if (isLoaded())
        {
            return true;
        }

        Log::debug("Loading shader: " + getName());

        // Load shader source files
        for (auto& [type, filepath] : m_filePaths)
        {
            std::string source = loadShaderSource(filepath);
            if (source.empty())
            {
                Log::error("Failed to load shader source: " + filepath);
                return false;
            }
            m_sources[type] = source;
        }

        // Compile the shader program
        if (!compile())
        {
            Log::error("Failed to compile shader: " + getName());
            return false;
        }

        m_loaded = true;
        return true;
    }

    void Shader::unload()
    {
        if (!isLoaded())
        {
            return;
        }

        Log::debug("Unloading shader: " + getName());

        if (m_programID != 0)
        {
            glDeleteProgram(m_programID);
            m_programID = 0;
        }

        m_uniformLocations.clear();
        m_attributes.clear();
        m_isValid = false;
        m_loaded = false;
    }

    bool Shader::onReload()
    {
        Log::debug("Reloading shader: " + getName());

        // Save the old program ID in case we need to roll back
        GLuint oldProgramID = m_programID;
        m_programID = 0;

        // Clear uniform locations as they will be invalidated
        m_uniformLocations.clear();
        m_attributes.clear();

        // Reload source files
        bool sourcesValid = true;
        for (auto& [type, filepath] : m_filePaths)
        {
            std::string source = loadShaderSource(filepath);
            if (source.empty())
            {
                Log::error("Failed to reload shader source: " + filepath);
                sourcesValid = false;
                break;
            }
            m_sources[type] = source;
        }

        // If loading failed, restore the old program and return false
        if (!sourcesValid || !compile())
        {
            Log::error("Failed to recompile shader: " + getName() + ", rolling back");

            // Cleanup the new failed program if it was created
            if (m_programID != 0)
            {
                glDeleteProgram(m_programID);
            }

            // Restore the old program
            m_programID = oldProgramID;
            return false;
        }

        // Delete the old program now that we've successfully recompiled
        if (oldProgramID != 0)
        {
            glDeleteProgram(oldProgramID);
        }

        Log::info("Successfully reloaded shader: " + getName());
        return true;
    }

    bool Shader::compile()
    {
        // Create a new program
        GLuint newProgram = glCreateProgram();
        if (newProgram == 0)
        {
            Log::error("Failed to create shader program");
            return false;
        }

        std::vector<GLuint> shaderIDs;
        bool compilationSucceeded = true;

        // Compile each shader stage
        for (auto& [type, source] : m_sources)
        {
            GLuint shaderID = 0;
            if (!compileShader(type, shaderID))
            {
                compilationSucceeded = false;
                break;
            }

            shaderIDs.push_back(shaderID);
            glAttachShader(newProgram, shaderID);
        }

        // Link the program if all shaders compiled successfully
        if (compilationSucceeded)
        {
            glLinkProgram(newProgram);

            GLint isLinked = 0;
            glGetProgramiv(newProgram, GL_LINK_STATUS, &isLinked);

            if (isLinked == GL_FALSE)
            {
                logProgramError(newProgram);
                compilationSucceeded = false;
            }
        }

        // Detach and delete individual shaders
        for (GLuint shaderID : shaderIDs)
        {
            glDetachShader(newProgram, shaderID);
            glDeleteShader(shaderID);
        }

        // If compilation or linking failed, delete the program and return false
        if (!compilationSucceeded)
        {
            glDeleteProgram(newProgram);
            return false;
        }

        // Validate the program
        glValidateProgram(newProgram);
        GLint isValid = 0;
        glGetProgramiv(newProgram, GL_VALIDATE_STATUS, &isValid);

        if (isValid == GL_FALSE)
        {
            logProgramError(newProgram);
            glDeleteProgram(newProgram);
            return false;
        }

        // Update the program ID and extract attributes
        m_programID = newProgram;
        m_isValid = true;
        extractAttributes();

        Log::info("Successfully compiled and linked shader: " + getName());
        return true;
    }

    void Shader::bind()
    {
        if (m_programID != 0)
        {
            glUseProgram(m_programID);
        }
    }

    void Shader::unbind()
    {
        glUseProgram(0);
    }

    bool Shader::isValid() const
    {
        return m_isValid && m_programID != 0;
    }

    void Shader::setSourceFile(ShaderType type, const std::string& filepath)
    {
        m_filePaths[type] = filepath;

        // If already loaded, we'll need to reload the shader
        if (isLoaded())
        {
            std::string source = loadShaderSource(filepath);
            if (!source.empty())
            {
                m_sources[type] = source;
                // We don't recompile immediately - it will happen on next use or explicit reload
            }
        }
    }

    void Shader::setSource(ShaderType type, const std::string& source)
    {
        m_sources[type] = source;
        // Clear the file path since we're using direct source code
        m_filePaths.erase(type);

        // If already loaded, we'll need to mark for recompilation
        if (isLoaded())
        {
            m_loaded = false;
        }
    }

    const std::vector<ShaderAttribute>& Shader::getAttributes() const
    {
        return m_attributes;
    }

    void Shader::dispatchCompute(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ)
    {
        if (!isValid())
        {
            Log::error("Cannot dispatch compute with invalid shader: " + getName());
            return;
        }

        bind();
        glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
    }

    void Shader::setFloat(const std::string& name, float value)
    {
        GLint location = getUniformLocation(name);
        if (location != -1)
        {
            glUniform1f(location, value);
        }
    }

    void Shader::setInt(const std::string& name, int value)
    {
        GLint location = getUniformLocation(name);
        if (location != -1)
        {
            glUniform1i(location, value);
        }
    }

    void Shader::setVec2(const std::string& name, const glm::vec2& value)
    {
        GLint location = getUniformLocation(name);
        if (location != -1)
        {
            glUniform2fv(location, 1, glm::value_ptr(value));
        }
    }

    void Shader::setVec3(const std::string& name, const glm::vec3& value)
    {
        GLint location = getUniformLocation(name);
        if (location != -1)
        {
            glUniform3fv(location, 1, glm::value_ptr(value));
        }
    }

    void Shader::setVec4(const std::string& name, const glm::vec4& value)
    {
        GLint location = getUniformLocation(name);
        if (location != -1)
        {
            glUniform4fv(location, 1, glm::value_ptr(value));
        }
    }

    void Shader::setMat3(const std::string& name, const glm::mat3& value)
    {
        GLint location = getUniformLocation(name);
        if (location != -1)
        {
            glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
        }
    }

    void Shader::setMat4(const std::string& name, const glm::mat4& value)
    {
        GLint location = getUniformLocation(name);
        if (location != -1)
        {
            glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
        }
    }

    GLenum Shader::getGLShaderType(ShaderType type) const
    {
        switch (type)
        {
            case ShaderType::Vertex:   return GL_VERTEX_SHADER;
            case ShaderType::Fragment: return GL_FRAGMENT_SHADER;
            case ShaderType::Geometry: return GL_GEOMETRY_SHADER;
            case ShaderType::Compute:  return GL_COMPUTE_SHADER;
            default:
                Log::error("Unknown shader type");
                return GL_VERTEX_SHADER;
        }
    }

    GLint Shader::getUniformLocation(const std::string& name)
    {
        // Check if we've already cached the location
        auto it = m_uniformLocations.find(name);
        if (it != m_uniformLocations.end())
        {
            return it->second;
        }

        // Look up the location and cache it
        GLint location = glGetUniformLocation(m_programID, name.c_str());
        if (location == -1)
        {
            Log::warn("Uniform not found in shader: " + name);
        }

        m_uniformLocations[name] = location;
        return location;
    }

    bool Shader::compileShader(ShaderType type, GLuint& shaderID)
    {
        auto sourceIt = m_sources.find(type);
        if (sourceIt == m_sources.end())
        {
            // Not an error, this shader stage is just not present
            return true;
        }

        const std::string& source = sourceIt->second;
        if (source.empty())
        {
            Log::error("Empty shader source for " + getName());
            return false;
        }

        // Create the shader object
        GLenum glType = getGLShaderType(type);
        shaderID = glCreateShader(glType);

        // Set the source and compile
        const char* sourcePtr = source.c_str();
        glShaderSource(shaderID, 1, &sourcePtr, nullptr);
        glCompileShader(shaderID);

        // Check for errors
        GLint isCompiled = 0;
        glGetShaderiv(shaderID, GL_COMPILE_STATUS, &isCompiled);

        if (isCompiled == GL_FALSE)
        {
            logShaderError(shaderID);
            glDeleteShader(shaderID);
            shaderID = 0;
            return false;
        }

        return true;
    }

    void Shader::extractAttributes()
    {
        m_attributes.clear();

        if (!isValid())
        {
            return;
        }

        // Get the number of active attributes
        GLint numAttributes = 0;
        glGetProgramiv(m_programID, GL_ACTIVE_ATTRIBUTES, &numAttributes);

        // Get the maximum name length
        GLint maxNameLength = 0;
        glGetProgramiv(m_programID, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxNameLength);

        // Allocate a buffer for the name
        std::vector<char> nameBuffer(maxNameLength);

        // Loop through all active attributes
        for (GLint i = 0; i < numAttributes; ++i)
        {
            GLint size = 0;
            GLenum type = GL_FLOAT;
            GLsizei length = 0;

            glGetActiveAttrib(m_programID, i, maxNameLength, &length, &size, &type, nameBuffer.data());

            ShaderAttribute attribute;
            attribute.name = std::string(nameBuffer.data(), length);
            attribute.size = size;
            attribute.type = type;
            attribute.location = glGetAttribLocation(m_programID, attribute.name.c_str());

            m_attributes.push_back(attribute);
        }
    }

    std::string Shader::loadShaderSource(const std::string& filepath)
    {
        try
        {
            return Core::FileSystem::readFile(filepath);
        }
        catch (const std::exception& e)
        {
            Log::error("Failed to read shader file: " + filepath + " | " + e.what());
            return "";
        }
    }

    void Shader::logShaderError(GLuint shader)
    {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength > 1)
        {
            std::vector<char> log(logLength);
            glGetShaderInfoLog(shader, logLength, nullptr, log.data());
            Log::error("Shader compilation error: " + std::string(log.data()));
        }
        else
        {
            Log::error("Shader compilation failed with no info log");
        }
    }

    void Shader::logProgramError(GLuint program)
    {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength > 1)
        {
            std::vector<char> log(logLength);
            glGetProgramInfoLog(program, logLength, nullptr, log.data());
            Log::error("Shader program error: " + std::string(log.data()));
        }
        else
        {
            Log::error("Shader program linking/validation failed with no info log");
        }
    }

} // namespace PixelCraft::Rendering