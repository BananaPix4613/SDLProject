#pragma once

#include <glad/glad.h>
#include <glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

// Foward declarations
class ShaderCache;

/**
 * @class Shader
 * @brief Advanced shader program class for GPU shader management
 * 
 * Supports vertex, fragment, geometry, compute, tessellation control, and
 * tessellation evaluation shaders with uniform caching and hot-reloading.
 */
class Shader
{
public:
    /**
     * @brief Default constructor
     */
    Shader();

    /**
     * @brief Constructor for graphics pipeline shaders (vertex and fragment)
     * @param vertexPath Path to vertex shader file
     * @param fragmentPath Path to fragment shader file
     */
    Shader(const char* vertexPath, const char* fragmentPath);

    /**
     * @brief Constructor for graphics pipeline with geometry shader
     * @param vertexPath Path to vertex shader file
     * @param fragmentPath Path to fragment shader file
     * @param geometryPath Path to geometry shader file
     */
    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath);

    /**
     * @brief Constructor for graphics pipeline with tessellation shaders
     * @param vertexPath Path to vertex shader file
     * @param fragmentPath Path to fragment shader file
     * @param tessControlPath Path to tessellation control shader file
     * @param tessEvalPath Path to tessellation evaluation shader file
     */
    Shader(const char* vertexPath, const char* fragmentPath,
           const char* tessControlPath, const char* tessEvalPath);

    /**
     * @brief Complete constructor for all shader types
     * @param vertexPath Path to vertex shader file
     * @param fragmentPath Path to fragment shader file
     * @param geometryPath Path to geometry shader file
     * @param tessControlPath Path to tessellation control shader file
     * @param tessEvalPath Path to tessellation evaluation shader file
     */
    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath,
           const char* tessControlPath, const char* tessEvalPath);

    /**
     * @brief Constructor for compute shaders
     * @param computePath Path to compute shader file
     */
    Shader(const char* computePath, std::nullptr_t, std::nullptr_t,
           std::nullptr_t, const char* fakeParam);

    /**
     * @brief Destructor
     */
    ~Shader();

    /**
     * @brief Use/activate the shader
     */
    void use() const;

    /**
     * @brief Get OpenGL program ID
     * @return Shader program ID
     */
    unsigned int getID() const
    {
        return m_ID;
    }

    /**
     * @brief Check if shader is valid (compiled successfully)
     * @return True if shader is valid
     */
    bool isValid() const
    {
        return m_isValid;
    }

    /**
     * @brief Reload all shader sources and recompile
     * @return True if reload was successful
     */
    bool reload();

    /**
     * @brief Get shader type name
     * @return Type name (e.g., "Graphics", "Compute")
     */
    const std::string& getTypeName() const
    {
        return m_typeName;
    }

    /**
     * @brief Set shader program
     */
    void setProgram(GLuint program);

    // Uniform setters with optimized caching
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setUint(const std::string& name, unsigned int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec2(const std::string& name, float x, float y) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec3(const std::string& name, float x, float y, float z) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setVec4(const std::string& name, float x, float y, float z, float w) const;
    void setMat2(const std::string& name, const glm::mat2& mat) const;
    void setMat3(const std::string& name, const glm::mat3& mat) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;

    // Compute shader dispatching
    void dispatchCompute(unsigned int numGroupsX, unsigned int numGroupsY, unsigned int numGroupsZ) const;

    // Shader storage buffer binding
    void bindShaderStorageBuffer(unsigned int binding, unsigned int buffer) const;

    // Uniform buffer binding
    void bindUniformBuffer(const std::string& blockName, unsigned int bindingPoint) const;

    // Shader cache friend class
    friend class ShaderCache;
    friend class LineBatchRenderer;

private:
    // OpenGL objects
    unsigned int m_ID;
    bool m_isCompute;
    bool m_isValid;
    std::string m_typeName;

    // Shader file paths
    std::string m_vertexPath;
    std::string m_fragmentPath;
    std::string m_geometryPath;
    std::string m_tessControlPath;
    std::string m_tessEvalPath;
    std::string m_computePath;

    // Shader source code cache
    std::string m_vertexCode;
    std::string m_fragmentCode;
    std::string m_geometryCode;
    std::string m_tessControlCode;
    std::string m_tessEvalCode;
    std::string m_computeCode;

    // Uniform location cache
    mutable std::unordered_map<std::string, int> m_uniformLocationCache;

    // Helper methods
    bool createGraphicsShader();
    bool createComputeShader();
    void cleanup();
    bool compileShader(unsigned int& shader, GLenum type, const std::string& source, const std::string& typeName);
    void checkCompileErrors(unsigned int shader, const std::string& type);
    bool loadShaderFile(const std::string& filePath, std::string& code);
    int getUniformLocation(const std::string& name) const;
};

/**
 * @class ShaderCache
 * @brief Singleton class for managing and caching shader programs
 */
class ShaderCache
{
public:
    /**
     * @brief Get or create shader
     * @param vertexPath Path to vertex shader file
     * @param fragmentPath Path to fragment shader file
     * @return Shader pointer
     */
    static Shader* getShader(const char* vertexPath, const char* fragmentPath);

    /**
     * @brief Get or create shader with geometry shader
     * @param vertexPath Path to vertex shader file
     * @param fragmentPath Path to fragment shader file
     * @param geometryPath Path to geometry shader file
     * @return Shader pointer
     */
    static Shader* getShader(const char* vertexPath, const char* fragmentPath, const char* geometryPath);

    /**
     * @brief Get or create compute shader
     * @param computePath Path to compute shader file
     * @return Shader pointer
     */
    static Shader* getComputeShader(const char* computePath);

    /**
     * @brief Clear all cached shaders
     */
    static void clearCache();

    /**
     * @brief Reload all cached shaders
     * @return Number of successfully reloaded shaders
     */
    static int reloadAll();

private:
    static std::unordered_map<std::string, std::shared_ptr<Shader>> s_shaderCache;
    static std::string generateKey(const char* vertexPath, const char* fragmentPath,
                                   const char* geometryPath = nullptr,
                                   const char* tessControlPath = nullptr,
                                   const char* tessEvalPath = nullptr,
                                   const char* computePath = nullptr);
};