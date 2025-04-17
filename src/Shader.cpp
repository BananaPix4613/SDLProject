#include "Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

// Initialize static shader cache
std::unordered_map<std::string, std::shared_ptr<Shader>> ShaderCache::s_shaderCache;

// Default constructor
Shader::Shader()
    : m_ID(0), m_isCompute(false), m_isValid(false), m_typeName("Unititialized")
{
}

// Constructor for graphics pipeline (vertex and fragment)
Shader::Shader(const char* vertexPath, const char* fragmentPath)
    : m_ID(0), m_isCompute(false), m_isValid(false), m_typeName("Graphics")
{
    if (vertexPath) m_vertexPath = vertexPath;
    if (fragmentPath) m_fragmentPath = fragmentPath;

    createGraphicsShader();
}

// Constructor with geometry shader
Shader::Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath)
    : m_ID(0), m_isCompute(false), m_isValid(false), m_typeName("Graphics+Geometry")
{
    if (vertexPath) m_vertexPath = vertexPath;
    if (fragmentPath) m_fragmentPath = fragmentPath;
    if (geometryPath) m_geometryPath = geometryPath;

    createGraphicsShader();
}

// Constructor with tessellation shaders
Shader::Shader(const char* vertexPath, const char* fragmentPath,
               const char* tessControlPath, const char* tessEvalPath)
    : m_ID(0), m_isCompute(false), m_isValid(false), m_typeName("Graphics+Tessellation")
{
    if (vertexPath) m_vertexPath = vertexPath;
    if (fragmentPath) m_fragmentPath = fragmentPath;
    if (tessControlPath) m_tessControlPath = tessControlPath;
    if (tessEvalPath) m_tessEvalPath = tessEvalPath;

    createGraphicsShader();
}

// Complete constructor
Shader::Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath,
               const char* tessControlPath, const char* tessEvalPath)
    : m_ID(0), m_isCompute(false), m_isValid(false), m_typeName("Graphics+Complete")
{
    if (vertexPath) m_vertexPath = vertexPath;
    if (fragmentPath) m_fragmentPath = fragmentPath;
    if (geometryPath) m_geometryPath = geometryPath;
    if (tessControlPath) m_tessControlPath = tessControlPath;
    if (tessEvalPath) m_tessEvalPath = tessEvalPath;

    createGraphicsShader();
}

// Constructor for compute shaders
Shader::Shader(const char* computePath, std::nullptr_t, std::nullptr_t,
               std::nullptr_t, const char* fakeParam)
    : m_ID(0), m_isCompute(true), m_isValid(false), m_typeName("Compute")
{
    if (computePath) m_computePath = computePath;

    createComputeShader();
}

// Destructor
Shader::~Shader()
{
    cleanup();
}

// Use the shader program
void Shader::use() const
{
    if (m_isValid)
    {
        glUseProgram(m_ID);
    }
}

// Reload all shader sources and recompile
bool Shader::reload()
{
    cleanup();

    if (m_isCompute)
    {
        return createComputeShader();
    }
    else
    {
        return createGraphicsShader();
    }
}

// Create and compile graphics shader
bool Shader::createGraphicsShader()
{
    // Clear cache and create new program
    m_uniformLocationCache.clear();
    m_ID = glCreateProgram();

    // Load shader files
    if (!m_vertexPath.empty() && !loadShaderFile(m_vertexPath, m_vertexCode))
    {
        std::cerr << "Failed to load vertex shader: " << m_vertexPath << std::endl;
        cleanup();
        return false;
    }

    if (!m_fragmentPath.empty() && !loadShaderFile(m_fragmentPath, m_fragmentCode))
    {
        std::cerr << "Failed to load fragment shader: " << m_fragmentPath << std::endl;
        cleanup();
        return false;
    }

    if (!m_geometryPath.empty() && !loadShaderFile(m_geometryPath, m_geometryCode))
    {
        std::cerr << "Failed to load geometry shader: " << m_geometryPath << std::endl;
        cleanup();
        return false;
    }

    if (!m_tessControlPath.empty() && !loadShaderFile(m_tessControlPath, m_tessControlCode))
    {
        std::cerr << "Failed to load tessellation control shader: " << m_tessControlPath << std::endl;
        cleanup();
        return false;
    }

    if (!m_tessEvalPath.empty() && !loadShaderFile(m_tessEvalPath, m_tessEvalCode))
    {
        std::cerr << "Failed to load tessellation evaluation shader: " << m_tessEvalPath << std::endl;
        cleanup();
        return false;
    }

    // Create shader objects
    unsigned int vertexShader = 0, fragmentShader = 0, geometryShader = 0;
    unsigned int tessControlShader = 0, tessEvalShader = 0;

    // Compile vertex shader
    if (!m_vertexCode.empty())
    {
        if (!compileShader(vertexShader, GL_VERTEX_SHADER, m_vertexCode, "VERTEX"))
        {
            cleanup();
            return false;
        }
        glAttachShader(m_ID, vertexShader);
    }

    // Compile fragment shader
    if (!m_fragmentCode.empty())
    {
        if (!compileShader(fragmentShader, GL_FRAGMENT_SHADER, m_fragmentCode, "FRAGMENT"))
        {
            cleanup();
            return false;
        }
        glAttachShader(m_ID, fragmentShader);
    }

    // Compile geometry shader if provided
    if (!m_geometryCode.empty())
    {
        if (!compileShader(geometryShader, GL_GEOMETRY_SHADER, m_geometryCode, "GEOMETRY"))
        {
            cleanup();
            return false;
        }
        glAttachShader(m_ID, geometryShader);
    }

    // Compile tessellation control shader if provided
    if (!m_tessControlCode.empty())
    {
        if (!compileShader(tessControlShader, GL_TESS_CONTROL_SHADER, m_tessControlCode, "TESS_CONTROL"))
        {
            cleanup();
            return false;
        }
        glAttachShader(m_ID, tessControlShader);
    }

    // Compile tessellation evaluation shader if provided
    if (!m_tessEvalCode.empty())
    {
        if (!compileShader(tessEvalShader, GL_TESS_EVALUATION_SHADER, m_tessEvalCode, "TESS_EVAL"))
        {
            cleanup();
            return false;
        }
        glAttachShader(m_ID, tessEvalShader);
    }

    // Link program
    glLinkProgram(m_ID);
    checkCompileErrors(m_ID, "PROGRAM");

    // Delete shader objects after linking
    if (vertexShader) glDeleteShader(vertexShader);
    if (fragmentShader) glDeleteShader(fragmentShader);
    if (geometryShader) glDeleteShader(geometryShader);
    if (tessControlShader) glDeleteShader(tessControlShader);
    if (tessEvalShader) glDeleteShader(tessEvalShader);

    // Check if program linking was successful
    int success;
    glGetProgramiv(m_ID, GL_LINK_STATUS, &success);
    m_isValid = (success != 0);

    return m_isValid;
}

// Create and compile compute shader
bool Shader::createComputeShader()
{
    // Clear cache and create new program
    m_uniformLocationCache.clear();
    m_ID = glCreateProgram();

    // Load compute shader file
    if (!loadShaderFile(m_computePath, m_computeCode))
    {
        std::cerr << "Failed to load compute shader: " << m_computePath << std::endl;
        cleanup();
        return false;
    }

    // Create and compile compute shader
    unsigned int computeShader = 0;
    if (!compileShader(computeShader, GL_COMPUTE_SHADER, m_computeCode, "COMPUTE"))
    {
        cleanup();
        return false;
    }

    // Attach and link
    glAttachShader(m_ID, computeShader);
    glLinkProgram(m_ID);
    checkCompileErrors(m_ID, "PROGRAM");

    // Delete shader object after linking
    glDeleteShader(computeShader);

    // Check if program linking was successful
    int success;
    glGetProgramiv(m_ID, GL_LINK_STATUS, &success);
    m_isValid = (success != 0);

    return m_isValid;
}

// Clean up resources
void Shader::cleanup()
{
    if (m_ID != 0)
    {
        glDeleteProgram(m_ID);
        m_ID = 0;
    }
    m_isValid;
}

// Compile a single shader
bool Shader::compileShader(unsigned int& shader, GLenum type, const std::string& source, const std::string& typeName)
{
    shader = glCreateShader(type);
    const char* sourcePtr = source.c_str();
    glShaderSource(shader, 1, &sourcePtr, nullptr);
    glCompileShader(shader);

    // Check for compilation errors
    checkCompileErrors(shader, typeName);

    // Return success status
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    return success != 0;
}

// Check for shader compilation or program linking errors
void Shader::checkCompileErrors(unsigned int shader, const std::string& type)
{
    int success;
    char infoLog[1024];

    if (type != "PROGRAM")
    {
        // Check shader compilation status
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << std::endl
                << infoLog << std::endl << " -- --------------------------------------------------- -- " << std::endl;
        }
    }
    else
    {
        // Check program linking status
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << std::endl
                << infoLog << std::endl << " -- --------------------------------------------------- -- " << std::endl;
        }
    }
}

// Load shader source from file
bool Shader::loadShaderFile(const std::string& filePath, std::string& code)
{
    if (filePath.empty()) return false;

    std::ifstream shaderFile;
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try
    {
        shaderFile.open(filePath);
        std::stringstream shaderStream;
        shaderStream << shaderFile.rdbuf();
        shaderFile.close();
        code = shaderStream.str();
        return true;
    }
    catch (std::ifstream::failure& e)
    {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << filePath << std::endl;
        std::cerr << "Exception: " << e.what() << std::endl;
        return false;
    }
}

// Get uniform location with caching
int Shader::getUniformLocation(const std::string& name) const
{
    // Check if the location is already cached
    auto it = m_uniformLocationCache.find(name);
    if (it != m_uniformLocationCache.end())
    {
        return it->second;
    }

    // Look up the location and cache it
    int location = glGetUniformLocation(m_ID, name.c_str());
    m_uniformLocationCache[name] = location;

    // Warn if uniform not found (could be optimized out by the GLSL compiler
    if (location == -1)
    {
        // Uncomment for debugging, but this is quite verbose
        // std::cerr << "Warning: Uniform '" << name << "' not found in shader program " << m_ID << std::endl;
    }

    return location;
}

void Shader::setProgram(GLuint program)
{
    // Clean up any existing program
    if (m_ID != 0)
    {
        glDeleteProgram(m_ID);
    }

    // Set the program ID
    m_ID = program;
    m_isValid = (program != 0);

    // Update type name
    m_typeName = "Graphics";

    // Clear the uniform cache
    m_uniformLocationCache.clear();
}

// Set a boolean uniform
void Shader::setBool(const std::string& name, bool value) const
{
    glUniform1i(getUniformLocation(name), static_cast<int>(value));
}

// Set an integer uniform
void Shader::setInt(const std::string& name, int value) const
{
    glUniform1i(getUniformLocation(name), value);
}

// Set an unsigned integer uniform
void Shader::setUint(const std::string& name, unsigned int value) const
{
    glUniform1ui(getUniformLocation(name), value);
}

// Set a float uniform
void Shader::setFloat(const std::string& name, float value) const
{
    glUniform1f(getUniformLocation(name), value);
}

// Set a vec2 uniform
void Shader::setVec2(const std::string& name, const glm::vec2& value) const
{
    glUniform2fv(getUniformLocation(name), 1, &value[0]);
}

// Set a vec2 uniform with component values
void Shader::setVec2(const std::string& name, float x, float y) const
{
    glUniform2f(getUniformLocation(name), x, y);
}

// Set a vec3 uniform
void Shader::setVec3(const std::string& name, const glm::vec3& value) const
{
    glUniform3fv(getUniformLocation(name), 1, &value[0]);
}

// Set a vec3 uniform with component values
void Shader::setVec3(const std::string& name, float x, float y, float z) const
{
    glUniform3f(getUniformLocation(name), x, y, z);
}

// Set a vec4 uniform
void Shader::setVec4(const std::string& name, const glm::vec4& value) const
{
    glUniform4fv(getUniformLocation(name), 1, &value[0]);
}

// Set a vec4 uniform with component values
void Shader::setVec4(const std::string& name, float x, float y, float z, float w) const
{
    glUniform4f(getUniformLocation(name), x, y, z, w);
}

// Set a mat2 uniform
void Shader::setMat2(const std::string& name, const glm::mat2& mat) const
{
    glUniformMatrix2fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}

// Set a mat3 uniform
void Shader::setMat3(const std::string& name, const glm::mat3& mat) const
{
    glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}

// Set a mat4 uniform
void Shader::setMat4(const std::string& name, const glm::mat4& mat) const
{
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}

// Dispatch compute shader
void Shader::dispatchCompute(unsigned int numGroupsX, unsigned int numGroupsY, unsigned int numGroupsZ) const
{
    if (!m_isValid || !m_isCompute)
    {
        std::cerr << "Cannot dispatch non-compute shader or invalid shader" << std::endl;
        return;
    }

    use();
    glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
}

// Bind shader storage buffer
void Shader::bindShaderStorageBuffer(unsigned int binding, unsigned int buffer) const
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, buffer);
}

// Bind uniform buffer
void Shader::bindUniformBuffer(const std::string& blockName, unsigned int bindingPoint) const
{
    if (!m_isValid) return;

    unsigned int blockIndex = glGetUniformBlockIndex(m_ID, blockName.c_str());
    if (blockIndex != GL_INVALID_INDEX)
    {
        glUniformBlockBinding(m_ID, blockIndex, bindingPoint);
    }
    else
    {
        std::cerr << "Uniform block: '" << blockName << "' not found in shader" << std::endl;
    }
}

//
// ShaderCache Implementation
//

// Generate unique key for shader combination
std::string ShaderCache::generateKey(const char* vertexPath, const char* fragmentPath,
                                     const char* geometryPath, const char* tessControlPath,
                                     const char* tessEvalPath, const char* computePath)
{
    std::string key;

    if (computePath)
    {
        key = "compute:" + std::string(computePath);
    }
    else
    {
        key = "graphics:";
        if (vertexPath) key += std::string(vertexPath);
        key += "|";
        if (fragmentPath) key += std::string(fragmentPath);
        key += "|";
        if (geometryPath) key += std::string(geometryPath);
        key += "|";
        if (tessControlPath) key += std::string(tessControlPath);
        key += "|";
        if (tessEvalPath) key += std::string(tessEvalPath);
    }

    return key;
}

// Get or create shader
Shader* ShaderCache::getShader(const char* vertexPath, const char* fragmentPath)
{
    std::string key = generateKey(vertexPath, fragmentPath);

    auto it = s_shaderCache.find(key);
    if (it != s_shaderCache.end())
    {
        return it->second.get();
    }

    // Create new shader and cache it
    auto shader = std::make_shared<Shader>(vertexPath, fragmentPath);
    s_shaderCache[key] = shader;
    return shader.get();
}

// Get or create shader with geometry shader
Shader* ShaderCache::getShader(const char* vertexPath, const char* fragmentPath, const char* geometryPath)
{
    std::string key = generateKey(vertexPath, fragmentPath, geometryPath);

    auto it = s_shaderCache.find(key);
    if (it != s_shaderCache.end())
    {
        return it->second.get();
    }

    // Create new shader and cache it
    auto shader = std::make_shared<Shader>(vertexPath, fragmentPath, geometryPath);
    s_shaderCache[key] = shader;
    return shader.get();
}

// Get or create compute shader
Shader* ShaderCache::getComputeShader(const char* computePath)
{
    std::string key = generateKey(nullptr, nullptr, nullptr, nullptr, nullptr, computePath);

    auto it = s_shaderCache.find(key);
    if (it != s_shaderCache.end())
    {
        return it->second.get();
    }

    // Create new compute shader and cache it
    auto shader = std::make_shared<Shader>(computePath, nullptr, nullptr, nullptr, nullptr);
    s_shaderCache[key] = shader;
    return shader.get();
}

// Clear shader cache
void ShaderCache::clearCache()
{
    s_shaderCache.clear();
}

// Reload all shaders in cache
int ShaderCache::reloadAll()
{
    int successCount = 0;

    for (auto& pair : s_shaderCache)
    {
        if (pair.second->reload())
        {
            successCount++;
        }
    }

    return successCount;
}