// -------------------------------------------------------------------------
// Shader.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <type_traits>

#include "Core/Resource.h"
#include "Core/Logger.h"
#include "glad/glad.h"
#include "glm/glm.hpp"

namespace PixelCraft::Rendering
{

    /**
     * @brief Shader types supported by the engine
     */
    enum class ShaderType
    {
        Vertex,
        Fragment,
        Geometry,
        Compute
    };

    /**
     * @brief Information about shader attribute (input)
     */
    struct ShaderAttribute
    {
        std::string name;
        GLint location;
        GLint size;
        GLenum type;
    };

    /**
     * @brief Encapsulates OpenGL shader program functionality
     *
     * The Shader class handles loading, compiling, and linking of
     * shader programs with support for hot-reloading and efficient
     * uniform management.
     */
    class Shader : public Core::Resource
    {
    public:
        /**
         * @brief Constructor
         * @param name Unique name for this shader resource
         */
        Shader(const std::string& name);

        /**
         * @brief Destructor
         */
        virtual ~Shader();

        /**
         * @brief Load the shader from associated source files
         * @return True if loading was successful
         */
        bool load() override;

        /**
         * @brief Unload the shader and release GPU resources
         */
        void unload() override;

        /**
         * @brief Handle hot-reloading of shader sources
         * @return True if reload was successful
         */
        bool onReload() override;

        /**
         * @brief Compile and link all shader stages
         * @return True if compilation was successful
         */
        bool compile();

        /**
         * @brief Bind the shader program for rendering
         */
        void bind();

        /**
         * @brief Unbind the shader program
         */
        void unbind();

        /**
         * @brief Check if the shader program is valid
         * @return True if the program is valid and ready for use
         */
        bool isValid() const;

        /**
         * @brief Get the file paths for all shader stages
         * @return Map of shader types to file paths
         */
        const std::unordered_map<ShaderType, std::string>& getFilePaths() const
        {
            return m_filePaths;
        }

        /**
         * @brief Set the source file for a shader stage
         * @param type The type of shader stage
         * @param filepath Path to the shader source file
         */
        void setSourceFile(ShaderType type, const std::string& filepath);

        /**
         * @brief Set the source code for a shader stage directly
         * @param type The type of shader stage
         * @param source The shader source code
         */
        void setSource(ShaderType type, const std::string& source);

        /**
         * @brief Get the attributes (inputs) of the shader
         * @return Vector of shader attributes
         */
        const std::vector<ShaderAttribute>& getAttributes() const;

        /**
         * @brief Dispatch a compute shader
         * @param numGroupsX Number of work groups in X dimension
         * @param numGroupsY Number of work groups in Y dimension
         * @param numGroupsZ Number of work groups in Z dimension
         */
        void dispatchCompute(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ);

        /**
         * @brief Set a float uniform value
         * @param name The uniform name
         * @param value The value to set
         */
        void setFloat(const std::string& name, float value);

        /**
         * @brief Set an integer uniform value
         * @param name The uniform name
         * @param value The value to set
         */
        void setInt(const std::string& name, int value);

        /**
         * @brief Set a vec2 uniform value
         * @param name The uniform name
         * @param value The value to set
         */
        void setVec2(const std::string& name, const glm::vec2& value);

        /**
         * @brief Set a vec3 uniform value
         * @param name The uniform name
         * @param value The value to set
         */
        void setVec3(const std::string& name, const glm::vec3& value);

        /**
         * @brief Set a vec4 uniform value
         * @param name The uniform name
         * @param value The value to set
         */
        void setVec4(const std::string& name, const glm::vec4& value);

        /**
         * @brief Set a mat3 uniform value
         * @param name The uniform name
         * @param value The value to set
         */
        void setMat3(const std::string& name, const glm::mat3& value);

        /**
         * @brief Set a mat4 uniform value
         * @param name The uniform name
         * @param value The value to set
         */
        void setMat4(const std::string& name, const glm::mat4& value);

        /**
         * @brief Set a generic uniform value
         * @tparam T The uniform type
         * @param name The uniform name
         * @param value The value to set
         */
        template<typename T>
        void setUniform(const std::string& name, const T& value)
        {
            if constexpr (std::is_same_v<T, float>)
            {
                setFloat(name, value);
            }
            else if constexpr (std::is_same_v<T, int>)
            {
                setInt(name, value);
            }
            else if constexpr (std::is_same_v<T, glm::vec2>)
            {
                setVec2(name, value);
            }
            else if constexpr (std::is_same_v<T, glm::vec3>)
            {
                setVec3(name, value);
            }
            else if constexpr (std::is_same_v<T, glm::vec4>)
            {
                setVec4(name, value);
            }
            else if constexpr (std::is_same_v<T, glm::mat3>)
            {
                setMat3(name, value);
            }
            else if constexpr (std::is_same_v<T, glm::mat4>)
            {
                setMat4(name, value);
            }
            else
            {
                static_assert(false, "Unsupported uniform type");
            }
        }

    private:
        /**
         * @brief Get the OpenGL shader type constant from ShaderType
         * @param type The shader type
         * @return The OpenGL shader type enum value
         */
        GLenum getGLShaderType(ShaderType type) const;

        /**
         * @brief Get the location of a uniform
         * @param name The uniform name
         * @return The uniform location or -1 if not found
         */
        GLint getUniformLocation(const std::string& name);

        /**
         * @brief Compile an individual shader stage
         * @param type The shader type
         * @param shaderID Output parameter for the shader ID
         * @return True if compilation succeeded
         */
        bool compileShader(ShaderType type, GLuint& shaderID);

        /**
         * @brief Extract attribute information from the program
         */
        void extractAttributes();

        /**
         * @brief Load shader source from file
         * @param filepath Path to the shader file
         * @return The shader source code
         */
        std::string loadShaderSource(const std::string& filepath);

        /**
         * @brief Log shader compilation errors
         * @param shader Shader ID to check for errors
         */
        void logShaderError(GLuint shader);

        /**
         * @brief Log program linking errors
         * @param program Program ID to check for errors
         */
        void logProgramError(GLuint program);

    private:
        GLuint m_programID;
        std::unordered_map<std::string, GLint> m_uniformLocations;
        std::unordered_map<ShaderType, std::string> m_sources;
        std::unordered_map<ShaderType, std::string> m_filePaths;
        std::vector<ShaderAttribute> m_attributes;
        bool m_isValid;
    };

} // namespace PixelCraft::Rendering