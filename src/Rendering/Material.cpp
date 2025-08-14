// -------------------------------------------------------------------------
// Material.cpp
// -------------------------------------------------------------------------
#include "Rendering/Material.h"
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"
#include "Core/Logger.h"
#include "Core/ResourceManager.h"
#include "Core/Application.h"

namespace PixelCraft::Rendering
{

    Material::Material()
        : Core::Resource("UnnamedMaterial")
        , m_blendMode(BlendMode::Opaque)
        , m_cullMode(CullMode::Back)
        , m_depthTest(true)
        , m_depthWrite(true)
    {
        Core::debug("Creating default material");
    }

    Material::Material(const std::string& name)
        : Core::Resource(name)
        , m_blendMode(BlendMode::Opaque)
        , m_cullMode(CullMode::Back)
        , m_depthTest(true)
        , m_depthWrite(true)
    {
        Core::debug("Creating material: " + name);
    }

    Material::~Material()
    {
        Core::debug("Destroying material: " + getName());
        
        // Add exception handling to prevent unhandled exceptions during destruction
        try
        {
            unload();
        }
        catch (const std::exception& e)
        {
            Core::error("Material destructor: Exception during unload: " + std::string(e.what()));
        }
        catch (...)
        {
            Core::error("Material destructor: Unknown exception during unload");
        }
    }

    bool Material::load()
    {
        if (isLoaded())
        {
            return true;
        }

        Core::debug("Loading material: " + getName());

        try
        {
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
        catch (const std::exception& e)
        {
            Core::error("Material::load: Exception loading material '" + getName() + "': " + e.what());
            return false;
        }
        catch (...)
        {
            Core::error("Material::load: Unknown exception loading material '" + getName() + "'");
            return false;
        }
    }

    void Material::unload()
    {
        if (!isLoaded())
        {
            return;
        }

        Core::debug("Unloading material: " + getName());

        try
        {
            // Clear all references safely
            
            // Clear shader reference
            if (m_shader)
            {
                m_shader.reset();
            }

            // Clear texture references - but don't unload them as they might be shared
            // The ResourceManager will handle texture cleanup
            m_textures.clear();
            m_customTextures.clear();

            // Clear parameters
            m_parameters.clear();

            // Clear parent reference
            m_parent.reset();
        }
        catch (const std::exception& e)
        {
            Core::error("Material::unload: Exception during unload: " + std::string(e.what()));
        }
        catch (...)
        {
            Core::error("Material::unload: Unknown exception during unload");
        }
    }

    bool Material::onReload()
    {
        Core::debug("Reloading material: " + getName());

        try
        {
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
        catch (const std::exception& e)
        {
            Core::error("Material::onReload: Exception reloading material '" + getName() + "': " + e.what());
            return false;
        }
        catch (...)
        {
            Core::error("Material::onReload: Unknown exception reloading material '" + getName() + "'");
            return false;
        }
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
                Core::error("Circular material inheritance detected: " + getName());
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

    bool Material::isSameParent(const std::weak_ptr<Material>& other_parent) const
    {
        auto my_parent = m_parent.lock();
        auto their_parent = other_parent.lock();
        return my_parent && their_parent && (my_parent == their_parent);
    }

    std::shared_ptr<Material> Material::clone() const
    {
        auto newMaterial = Core::makeShared<Material>(getName() + "_Clone");

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

    bool Material::operator==(const Material& other) const
    {
        // Check all properties
        if (m_shader != other.m_shader) return false;
        if (m_parameters != other.m_parameters) return false;
        if (m_textures != other.m_textures) return false;
        if (m_customTextures != other.m_customTextures) return false;
        if (m_blendMode != other.m_blendMode) return false;
        if (m_cullMode != other.m_cullMode) return false;
        if (m_depthTest != other.m_depthTest) return false;
        if (m_depthWrite != other.m_depthWrite) return false;

        // Check parent weak_ptr
        if (!isSameParent(other.m_parent)) return false;

        return true;
    }

    bool Material::operator!=(const Material& other) const
    {
        return !(*this == other);
    }

    void Material::bind()
    {
        if (!m_shader)
        {
            Core::warn("Material::bind: No shader assigned to material: " + getName());
            return;
        }

        try
        {
            m_shader->bind();

            bindRenderState();
            bindParameters();
            bindTextures();
        }
        catch (const std::exception& e)
        {
            Core::error("Material::bind: Exception binding material '" + getName() + "': " + e.what());
        }
        catch (...)
        {
            Core::error("Material::bind: Unknown exception binding material '" + getName() + "'");
        }
    }

    void Material::unbind()
    {
        try
        {
            if (m_shader)
            {
                m_shader->unbind();
            }
        }
        catch (const std::exception& e)
        {
            Core::error("Material::unbind: Exception unbinding material '" + getName() + "': " + e.what());
        }
        catch (...)
        {
            Core::error("Material::unbind: Unknown exception unbinding material '" + getName() + "'");
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

        // Helper lambda to safely set uniforms
        auto safeSetUniform = [this](const std::string& name, const auto& value)
        {
            try
            {
                using T = std::decay_t<decltype(value)>;

                if constexpr (std::is_same_v<T, int>)
                {
                    if (m_shader->hasUniform(name))
                    {
                        m_shader->setUniform(name, value);
                    }
                }
                else if constexpr (std::is_same_v<T, float>)
                {
                    if (m_shader->hasUniform(name))
                    {
                        m_shader->setUniform(name, value);
                    }
                }
                else if constexpr (std::is_same_v<T, glm::vec2>)
                {
                    if (m_shader->hasUniform(name))
                    {
                        m_shader->setUniform(name, value);
                    }
                }
                else if constexpr (std::is_same_v<T, glm::vec3>)
                {
                    if (m_shader->hasUniform(name))
                    {
                        m_shader->setUniform(name, value);
                    }
                }
                else if constexpr (std::is_same_v<T, glm::vec4>)
                {
                    if (m_shader->hasUniform(name))
                    {
                        m_shader->setUniform(name, value);
                    }
                }
                else if constexpr (std::is_same_v<T, glm::mat4>)
                {
                    if (m_shader->hasUniform(name))
                    {
                        m_shader->setUniform(name, value);
                    }
                }
            }
            catch (const std::exception& e)
            {
                // Log the exception but don't crash
                Core::error("Material::bindParameters: Exception setting uniform '" + name + "': " + e.what());
            }
            catch (...) {
                // Catch any other exceptions
                Core::error("Material::bindParameters: Unknown exception setting uniform '" + name + "'");
            }
        };

        // Process all parameters in this material
        for (const auto& [name, paramValue] : m_parameters)
        {
            std::visit([&safeSetUniform, &name](const auto& value) {
                safeSetUniform(name, value);
            }, paramValue.value);
        }

        // Process parent parameters not overriden by this material
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

                std::visit([&safeSetUniform, &name](const auto& value) {
                    safeSetUniform(name, value);
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

        try
        {
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
        catch (const std::exception& e)
        {
            Core::error("Material::bindTextures: Exception binding textures: " + std::string(e.what()));
        }
        catch (...)
        {
            Core::error("Material::bindTextures: Unknown exception binding textures");
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

} // namespace PixelCraft::Rendering