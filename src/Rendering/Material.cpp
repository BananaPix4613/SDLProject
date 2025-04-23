// -------------------------------------------------------------------------
// Material.cpp
// -------------------------------------------------------------------------
#include "Rendering/Material.h"
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"
#include "Core/Logger.h"
#include "Core/ResourceManager.h"
#include "Core/Application.h"

namespace Log = PixelCraft::Core;

namespace PixelCraft::Rendering
{

    Material::Material(const std::string& name)
        : Core::Resource(name)
        , m_blendMode(BlendMode::Opaque)
        , m_cullMode(CullMode::Back)
        , m_depthTest(true)
        , m_depthWrite(true)
    {
        Log::debug("Creating material: " + name);
    }

    Material::~Material()
    {
        Log::debug("Destroying material: " + getName());
        unload();
    }

    bool Material::load()
    {
        if (isLoaded())
        {
            return true;
        }

        Log::debug("Loading material: " + getName());

        // Load any textures that have been set
        for (auto& [slot, texture] : m_textures)
        {
            if (texture && !texture->isLoaded())
            {
                texture->load();
            }
        }

        for (auto& [name, texture] : m_customTextures)
        {
            if (texture && !texture->isLoaded())
            {
                texture->load();
            }
        }

        return true;
    }

    void Material::unload()
    {
        if (!isLoaded())
        {
            return;
        }

        Log::debug("Unloading material: " + getName());

        // No need to unload textures as they are managed by ResourceManager

        m_shader.reset();
    }

    bool Material::onReload()
    {
        Log::debug("Reloading material: " + getName());

        // Reload textures if needed
        for (auto& [slot, texture] : m_textures)
        {
            if (texture)
            {
                texture->onReload();
            }
        }

        for (auto& [name, texture] : m_customTextures)
        {
            if (texture)
            {
                texture->onReload();
            }
        }

        return true;
    }

    void Material::setShader(std::shared_ptr<Shader> shader)
    {
        m_shader = shader;
    }

    std::shared_ptr<Shader> Material::getShader() const
    {
        return m_shader;
    }

    bool Material::hasParameter(const std::string& name) const
    {
        // Check local parameters first
        if (m_parameters.find(name) != m_parameters.end())
        {
            return true;
        }

        // Check parent if available
        auto parent = m_parent.lock();
        if (parent)
        {
            return parent->hasParameter(name);
        }

        return false;
    }

    void Material::setFloat(const std::string& name, float value)
    {
        ParameterValue param;
        param.value = value;
        m_parameters[name] = param;
    }

    void Material::setInt(const std::string& name, int value)
    {
        ParameterValue param;
        param.value = value;
        m_parameters[name] = param;
    }

    void Material::setVector2(const std::string& name, const glm::vec2& value)
    {
        ParameterValue param;
        param.value = value;
        m_parameters[name] = param;
    }

    void Material::setVector3(const std::string& name, const glm::vec3& value)
    {
        ParameterValue param;
        param.value = value;
        m_parameters[name] = param;
    }

    void Material::setVector4(const std::string& name, const glm::vec4& value)
    {
        ParameterValue param;
        param.value = value;
        m_parameters[name] = param;
    }

    void Material::setMatrix4(const std::string& name, const glm::mat4& value)
    {
        ParameterValue param;
        param.value = value;
        m_parameters[name] = param;
    }

    void Material::setColor(const std::string& name, const glm::vec4& value)
    {
        setVector4(name, value);
    }

    void Material::setTexture(TextureSlot slot, std::shared_ptr<Texture> texture)
    {
        m_textures[slot] = texture;
    }

    std::shared_ptr<Texture> Material::getTexture(TextureSlot slot) const
    {
        std::shared_ptr<Texture> result;
        findTexture(slot, result);
        return result;
    }

    void Material::setTextureByName(const std::string& name, std::shared_ptr<Texture> texture)
    {
        m_customTextures[name] = texture;
    }

    std::shared_ptr<Texture> Material::getTextureByName(const std::string& name) const
    {
        auto it = m_customTextures.find(name);
        if (it != m_customTextures.end())
        {
            return it->second;
        }

        // Check parent if available
        auto parent = m_parent.lock();
        if (parent)
        {
            return parent->getTextureByName(name);
        }

        return nullptr;
    }

    void Material::setBaseColor(const glm::vec3& color)
    {
        setVector3("baseColor", color);
    }

    void Material::setMetallic(float metallic)
    {
        setFloat("metallic", metallic);
    }

    void Material::setRoughness(float roughness)
    {
        setFloat("roughness", roughness);
    }

    void Material::setEmissive(const glm::vec3& emissive)
    {
        setVector3("emissive", emissive);
    }

    glm::vec3 Material::getBaseColor() const
    {
        return getParameter<glm::vec3>("baseColor", glm::vec3(1.0f));
    }

    float Material::getMetallic() const
    {
        return getParameter<float>("metallic", 0.0f);
    }

    float Material::getRoughness() const
    {
        return getParameter<float>("roughness", 0.5f);
    }

    glm::vec3 Material::getEmissive() const
    {
        return getParameter<glm::vec3>("emissive", glm::vec3(0.0f));
    }

    void Material::setBlendMode(BlendMode mode)
    {
        m_blendMode = mode;
    }

    void Material::setCullMode(CullMode mode)
    {
        m_cullMode = mode;
    }

    void Material::setDepthTest(bool enabled)
    {
        m_depthTest = enabled;
    }

    void Material::setDepthWrite(bool enabled)
    {
        m_depthWrite = enabled;
    }

    BlendMode Material::getBlendMode() const
    {
        return m_blendMode;
    }

    CullMode Material::getCullMode() const
    {
        return m_cullMode;
    }

    bool Material::getDepthTest() const
    {
        return m_depthTest;
    }

    bool Material::getDepthWrite() const
    {
        return m_depthWrite;
    }

    void Material::setParent(std::shared_ptr<Material> parent)
    {
        // Prevent circular inheritance
        std::shared_ptr<Material> check = parent;
        while (check)
        {
            if (check.get() == this)
            {
                Log::error("Circular material inheritance detected: " + getName());
                return;
            }
            check = check->getParent();
        }

        m_parent = parent;
    }

    std::shared_ptr<Material> Material::getParent() const
    {
        return m_parent.lock();
    }

    std::shared_ptr<Material> Material::clone() const
    {
        auto newMaterial = std::make_shared<Material>(getName() + "_Clone");

        // Copy all properties
        newMaterial->m_shader = m_shader;
        newMaterial->m_parameters = m_parameters;
        newMaterial->m_textures = m_textures;
        newMaterial->m_customTextures = m_customTextures;
        newMaterial->m_blendMode = m_blendMode;
        newMaterial->m_cullMode = m_cullMode;
        newMaterial->m_depthTest = m_depthTest;
        newMaterial->m_depthWrite = m_depthWrite;

        // Set parent to this material's parent (not to this material)
        newMaterial->m_parent = m_parent;

        return newMaterial;
    }

    void Material::bind()
    {
        if (!m_shader)
        {
            Log::warn("Material::bind: No shader assigned to material: " + getName());
            return;
        }

        m_shader->bind();

        bindRenderState();
        bindParameters();
        bindTextures();
    }

    void Material::unbind()
    {
        if (m_shader)
        {
            m_shader->unbind();
        }
    }

    void Material::bindRenderState()
    {
        // Note: This would typically map to platform-specific graphics API calls
        // For example, in OpenGL these would be glEnable/glDisable calls

        // Set blend mode
        switch (m_blendMode)
        {
            case BlendMode::Opaque:
                // Disable blending
                break;
            case BlendMode::Transparent:
                // Enable alpha blending
                break;
            case BlendMode::Additive:
                // Enable additive blending
                break;
            case BlendMode::Multiply:
                // Enable multiply blending
                break;
            case BlendMode::Screen:
                // Enable screen blending
                break;
        }

        // Set cull mode
        switch (m_cullMode)
        {
            case CullMode::None:
                // Disable face culling
                break;
            case CullMode::Front:
                // Cull front faces
                break;
            case CullMode::Back:
                // Cull back faces
                break;
        }

        // Set depth test
        if (m_depthTest)
        {
            // Enable depth testing
        }
        else
        {
            // Disable depth testing
        }

        // Set depth write
        if (m_depthWrite)
        {
            // Enable depth writing
        }
        else
        {
            // Disable depth writing
        }
    }

    void Material::bindParameters()
    {
        if (!m_shader)
        {
            return;
        }

        // Process all parameters in this material
        for (const auto& [name, paramValue] : m_parameters)
        {
            std::visit([this, &name](const auto& value) {
                using T = std::decay_t<decltype(value)>;

                if constexpr (std::is_same_v<T, int>)
                {
                    m_shader->setUniform(name, value);
                }
                else if constexpr (std::is_same_v<T, float>)
                {
                    m_shader->setUniform(name, value);
                }
                else if constexpr (std::is_same_v<T, glm::vec2>)
                {
                    m_shader->setUniform(name, value);
                }
                else if constexpr (std::is_same_v<T, glm::vec3>)
                {
                    m_shader->setUniform(name, value);
                }
                else if constexpr (std::is_same_v<T, glm::vec4>)
                {
                    m_shader->setUniform(name, value);
                }
                else if constexpr (std::is_same_v<T, glm::mat4>)
                {
                    m_shader->setUniform(name, value);
                }
                       }, paramValue.value);
        }

        // Process parent parameters not overridden by this material
        auto parent = m_parent.lock();
        if (parent)
        {
            for (const auto& [name, paramValue] : parent->m_parameters)
            {
                // Skip if this material already has this parameter
                if (m_parameters.find(name) != m_parameters.end())
                {
                    continue;
                }

                std::visit([this, &name](const auto& value) {
                    using T = std::decay_t<decltype(value)>;

                    if constexpr (std::is_same_v<T, int>)
                    {
                        m_shader->setUniform(name, value);
                    }
                    else if constexpr (std::is_same_v<T, float>)
                    {
                        m_shader->setUniform(name, value);
                    }
                    else if constexpr (std::is_same_v<T, glm::vec2>)
                    {
                        m_shader->setUniform(name, value);
                    }
                    else if constexpr (std::is_same_v<T, glm::vec3>)
                    {
                        m_shader->setUniform(name, value);
                    }
                    else if constexpr (std::is_same_v<T, glm::vec4>)
                    {
                        m_shader->setUniform(name, value);
                    }
                    else if constexpr (std::is_same_v<T, glm::mat4>)
                    {
                        m_shader->setUniform(name, value);
                    }
                           }, paramValue.value);
            }
        }
    }

    void Material::bindTextures()
    {
        if (!m_shader)
        {
            return;
        }

        int textureUnit = 0;

        // Process standard texture slots
        for (const auto& [slot, texture] : m_textures)
        {
            if (!texture)
            {
                continue;
            }

            // Map texture slot to uniform name
            std::string uniformName;
            switch (slot)
            {
                case TextureSlot::Albedo:
                    uniformName = "albedoMap";
                    break;
                case TextureSlot::Normal:
                    uniformName = "normalMap";
                    break;
                case TextureSlot::Metallic:
                    uniformName = "metallicMap";
                    break;
                case TextureSlot::Roughness:
                    uniformName = "roughnessMap";
                    break;
                case TextureSlot::AmbientOcclusion:
                    uniformName = "aoMap";
                    break;
                case TextureSlot::Emissive:
                    uniformName = "emissiveMap";
                    break;
                case TextureSlot::Height:
                    uniformName = "heightMap";
                    break;
                case TextureSlot::User0:
                    uniformName = "userMap0";
                    break;
                case TextureSlot::User1:
                    uniformName = "userMap1";
                    break;
                case TextureSlot::User2:
                    uniformName = "userMap2";
                    break;
                case TextureSlot::User3:
                    uniformName = "userMap3";
                    break;
            }

            texture->bind(textureUnit);
            m_shader->setUniform(uniformName, textureUnit);
            textureUnit++;
        }

        // Process custom textures
        for (const auto& [name, texture] : m_customTextures)
        {
            if (!texture)
            {
                continue;
            }

            texture->bind(textureUnit);
            m_shader->setUniform(name, textureUnit);
            textureUnit++;
        }

        // Process parent textures not overridden by this material
        auto parent = m_parent.lock();
        if (parent)
        {
            // Process standard texture slots from parent
            for (int i = 0; i < static_cast<int>(TextureSlot::User3) + 1; i++)
            {
                TextureSlot slot = static_cast<TextureSlot>(i);

                // Skip if this material already has this texture
                if (m_textures.find(slot) != m_textures.end())
                {
                    continue;
                }

                auto texture = parent->getTexture(slot);
                if (!texture)
                {
                    continue;
                }

                // Map texture slot to uniform name
                std::string uniformName;
                switch (slot)
                {
                    case TextureSlot::Albedo:
                        uniformName = "albedoMap";
                        break;
                    case TextureSlot::Normal:
                        uniformName = "normalMap";
                        break;
                    case TextureSlot::Metallic:
                        uniformName = "metallicMap";
                        break;
                    case TextureSlot::Roughness:
                        uniformName = "roughnessMap";
                        break;
                    case TextureSlot::AmbientOcclusion:
                        uniformName = "aoMap";
                        break;
                    case TextureSlot::Emissive:
                        uniformName = "emissiveMap";
                        break;
                    case TextureSlot::Height:
                        uniformName = "heightMap";
                        break;
                    case TextureSlot::User0:
                        uniformName = "userMap0";
                        break;
                    case TextureSlot::User1:
                        uniformName = "userMap1";
                        break;
                    case TextureSlot::User2:
                        uniformName = "userMap2";
                        break;
                    case TextureSlot::User3:
                        uniformName = "userMap3";
                        break;
                }

                texture->bind(textureUnit);
                m_shader->setUniform(uniformName, textureUnit);
                textureUnit++;
            }
        }
    }

    bool Material::findParameter(const std::string& name, ParameterValue& outValue) const
    {
        // Check local parameters first
        auto it = m_parameters.find(name);
        if (it != m_parameters.end())
        {
            outValue = it->second;
            return true;
        }

        // Check parent if available
        auto parent = m_parent.lock();
        if (parent)
        {
            return parent->findParameter(name, outValue);
        }

        return false;
    }

    bool Material::findTexture(TextureSlot slot, std::shared_ptr<Texture>& outTexture) const
    {
        // Check local textures first
        auto it = m_textures.find(slot);
        if (it != m_textures.end() && it->second)
        {
            outTexture = it->second;
            return true;
        }

        // Check parent if available
        auto parent = m_parent.lock();
        if (parent)
        {
            return parent->findTexture(slot, outTexture);
        }

        return false;
    }

    // Template specializations for getParameter
    template<>
    int Material::getParameter<int>(const std::string& name, const int& defaultValue) const
    {
        ParameterValue value;
        if (findParameter(name, value))
        {
            if (std::holds_alternative<int>(value.value))
            {
                return std::get<int>(value.value);
            }
        }
        return defaultValue;
    }

    template<>
    float Material::getParameter<float>(const std::string& name, const float& defaultValue) const
    {
        ParameterValue value;
        if (findParameter(name, value))
        {
            if (std::holds_alternative<float>(value.value))
            {
                return std::get<float>(value.value);
            }
        }
        return defaultValue;
    }

    template<>
    glm::vec2 Material::getParameter<glm::vec2>(const std::string& name, const glm::vec2& defaultValue) const
    {
        ParameterValue value;
        if (findParameter(name, value))
        {
            if (std::holds_alternative<glm::vec2>(value.value))
            {
                return std::get<glm::vec2>(value.value);
            }
        }
        return defaultValue;
    }

    template<>
    glm::vec3 Material::getParameter<glm::vec3>(const std::string& name, const glm::vec3& defaultValue) const
    {
        ParameterValue value;
        if (findParameter(name, value))
        {
            if (std::holds_alternative<glm::vec3>(value.value))
            {
                return std::get<glm::vec3>(value.value);
            }
        }
        return defaultValue;
    }

    template<>
    glm::vec4 Material::getParameter<glm::vec4>(const std::string& name, const glm::vec4& defaultValue) const
    {
        ParameterValue value;
        if (findParameter(name, value))
        {
            if (std::holds_alternative<glm::vec4>(value.value))
            {
                return std::get<glm::vec4>(value.value);
            }
        }
        return defaultValue;
    }

    template<>
    glm::mat4 Material::getParameter<glm::mat4>(const std::string& name, const glm::mat4& defaultValue) const
    {
        ParameterValue value;
        if (findParameter(name, value))
        {
            if (std::holds_alternative<glm::mat4>(value.value))
            {
                return std::get<glm::mat4>(value.value);
            }
        }
        return defaultValue;
    }

} // namespace PixelCraft::Rendering