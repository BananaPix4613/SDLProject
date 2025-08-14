// -------------------------------------------------------------------------
// Material.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Core/Resource.h"
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"

namespace PixelCraft::Rendering
{

    /**
     * @brief Defines the blending mode for material rendering
     */
    enum class BlendMode
    {
        Opaque,     ///< No blending (default)
        Transparent, ///< Alpha blending
        Additive,   ///< Additive blending
        Multiply,   ///< Multiply blending
        Screen      ///< Screen blending
    };

    /**
     * @brief Defines face culling mode for material rendering
     */
    enum class CullMode
    {
        None,       ///< No face culling
        Front,      ///< Cull front faces
        Back        ///< Cull back faces (default)
    };

    /**
     * @brief Defines standard texture slots for materials
     */
    enum class TextureSlot
    {
        Albedo,          ///< Base color/diffuse texture
        Normal,          ///< Normal map texture
        Metallic,        ///< Metallic texture
        Roughness,       ///< Roughness texture
        AmbientOcclusion, ///< Ambient occlusion texture
        Emissive,        ///< Emissive texture
        Height,          ///< Height/displacement map
        User0,           ///< Custom user texture slot 0
        User1,           ///< Custom user texture slot 1
        User2,           ///< Custom user texture slot 2
        User3            ///< Custom user texture slot 3
    };

    /**
     * @brief Represents surface appearance properties and shader parameters
     *
     * The Material class manages shader parameters, textures, and render states
     * for rendering 3D objects. It supports PBR (Physically Based Rendering)
     * properties and material inheritance.
     */
    class Material : public Core::Resource
    {
    public:
        /**
         * @brief Default constructor
         */
        Material();

        /**
         * @brief Constructor
         * @param name Name of the material
         */
        Material(const std::string& name);

        /**
         * @brief Destructor
         */
        virtual ~Material();

        // Resource implementation
        /**
         * @brief Load the material
         * @return True if successful
         */
        bool load() override;

        /**
         * @brief Unload the material
         */
        void unload() override;

        /**
         * @brief Handle material reload
         * @return True if reload was successful
         */
        bool onReload() override;

        // Shader management
        /**
         * @brief Set the shader for this material
         * @param shader Shader to use
         */
        void setShader(std::shared_ptr<Shader> shader);

        /**
         * @brief Get the current shader
         * @return The shader used by this material
         */
        std::shared_ptr<Shader> getShader() const;

        // Parameter management
        /**
         * @brief Set a parameter value of any supported type
         * @param name Parameter name
         * @param value Parameter value
         */
        template<typename T>
        void setParameter(const std::string& name, const T& value);

        /**
         * @brief Get a parameter value
         * @param name Parameter name
         * @param defaultValue Default value to return if parameter doesn't exist
         * @return Parameter value or default value
         */
        template<typename T>
        T getParameter(const std::string& name, const T& defaultValue = T()) const;

        /**
         * @brief Check if a parameter exists
         * @param name Parameter name
         * @return True if parameter exists
         */
        bool hasParameter(const std::string& name) const;

        // Specialized parameter setters (for common types)
        /**
         * @brief Set a float parameter
         * @param name Parameter name
         * @param value Float value
         */
        void setFloat(const std::string& name, float value);

        /**
         * @brief Set an integer parameter
         * @param name Parameter name
         * @param value Integer value
         */
        void setInt(const std::string& name, int value);

        /**
         * @brief Set a 2D vector parameter
         * @param name Parameter name
         * @param value Vector2 value
         */
        void setVector2(const std::string& name, const glm::vec2& value);

        /**
         * @brief Set a 3D vector parameter
         * @param name Parameter name
         * @param value Vector3 value
         */
        void setVector3(const std::string& name, const glm::vec3& value);

        /**
         * @brief Set a 4D vector parameter
         * @param name Parameter name
         * @param value Vector4 value
         */
        void setVector4(const std::string& name, const glm::vec4& value);

        /**
         * @brief Set a 4x4 matrix parameter
         * @param name Parameter name
         * @param value Matrix4x4 value
         */
        void setMatrix4(const std::string& name, const glm::mat4& value);

        /**
         * @brief Set a color parameter
         * @param name Parameter name
         * @param value Color as Vector4 (RGBA)
         */
        void setColor(const std::string& name, const glm::vec4& value);

        // Texture management
        /**
         * @brief Set a texture in a standard slot
         * @param slot Texture slot
         * @param texture Texture to set
         */
        void setTexture(TextureSlot slot, std::shared_ptr<Texture> texture);

        /**
         * @brief Get a texture from a standard slot
         * @param slot Texture slot
         * @return Texture in the slot or nullptr
         */
        std::shared_ptr<Texture> getTexture(TextureSlot slot) const;

        /**
         * @brief Set a texture with a custom name
         * @param name Custom texture name
         * @param texture Texture to set
         */
        void setTextureByName(const std::string& name, std::shared_ptr<Texture> texture);

        /**
         * @brief Get a texture by custom name
         * @param name Custom texture name
         * @return Texture with the name or nullptr
         */
        std::shared_ptr<Texture> getTextureByName(const std::string& name) const;

        // PBR properties convenience methods
        /**
         * @brief Set the base color (albedo)
         * @param color Base color as Vector3 (RGB)
         */
        void setBaseColor(const glm::vec3& color);

        /**
         * @brief Set the metallic value
         * @param metallic Metallic value [0-1]
         */
        void setMetallic(float metallic);

        /**
         * @brief Set the roughness value
         * @param roughness Roughness value [0-1]
         */
        void setRoughness(float roughness);

        /**
         * @brief Set the emissive color
         * @param emissive Emissive color as Vector3 (RGB)
         */
        void setEmissive(const glm::vec3& emissive);

        /**
         * @brief Get the base color
         * @return Base color as Vector3 (RGB)
         */
        glm::vec3 getBaseColor() const;

        /**
         * @brief Get the metallic value
         * @return Metallic value [0-1]
         */
        float getMetallic() const;

        /**
         * @brief Get the roughness value
         * @return Roughness value [0-1]
         */
        float getRoughness() const;

        /**
         * @brief Get the emissive color
         * @return Emissive color as Vector3 (RGB)
         */
        glm::vec3 getEmissive() const;

        // Render state
        /**
         * @brief Set the blending mode
         * @param mode Blend mode to use
         */
        void setBlendMode(BlendMode mode);

        /**
         * @brief Set the face culling mode
         * @param mode Culling mode to use
         */
        void setCullMode(CullMode mode);

        /**
         * @brief Enable or disable depth testing
         * @param enabled True to enable depth testing
         */
        void setDepthTest(bool enabled);

        /**
         * @brief Enable or disable depth writing
         * @param enabled True to enable depth writing
         */
        void setDepthWrite(bool enabled);

        /**
         * @brief Get the current blend mode
         * @return Current blend mode
         */
        BlendMode getBlendMode() const;

        /**
         * @brief Get the current cull mode
         * @return Current cull mode
         */
        CullMode getCullMode() const;

        /**
         * @brief Check if depth testing is enabled
         * @return True if depth testing is enabled
         */
        bool getDepthTest() const;

        /**
         * @brief Check if depth writing is enabled
         * @return True if depth writing is enabled
         */
        bool getDepthWrite() const;

        // Material inheritance
        /**
         * @brief Set a parent material for inheritance
         * @param parent Parent material
         */
        void setParent(std::shared_ptr<Material> parent);

        /**
         * @brief Get the parent material
         * @return Parent material or nullptr
         */
        std::shared_ptr<Material> getParent() const;

        /**
         * @brief Check if parents are the same
         * @param other_parent Other parent to compare with
         * @return True if parents are the same
         */
        bool isSameParent(const std::weak_ptr<Material>& other_parent) const;

        /**
         * @brief Create a clone of this material
         * @return New material instance with same properties
         */
        std::shared_ptr<Material> clone() const;

        /**
         * @brief Does equal operator for materials
         * @param other Other material to check against
         * @return True if passed material is equal to this material
         */
        bool operator==(const Material& other) const;

        /**
         * @brief Does not equal operator for materials
         * @param material Other material to check against
         * @return True if passed material is not equal to this material
         */
        bool operator!=(const Material& other) const;

        // Binding
        /**
         * @brief Bind the material for rendering
         */
        void bind();

        /**
         * @brief Unbind the material after rendering
         */
        void unbind();

    private:
        std::shared_ptr<Shader> m_shader;
        std::weak_ptr<Material> m_parent;

        struct ParameterValue
        {
            std::variant<int, float, glm::vec2, glm::vec3, glm::vec4, glm::mat4> value = {};

            bool operator==(const ParameterValue& other) const
            {
                return value == other.value;
            }
            
            bool operator!=(const ParameterValue& other) const
            {
                return value != other.value;
            }
        };

        std::unordered_map<std::string, ParameterValue> m_parameters;
        std::unordered_map<TextureSlot, std::shared_ptr<Texture>> m_textures;
        std::unordered_map<std::string, std::shared_ptr<Texture>> m_customTextures;

        // Render state
        BlendMode m_blendMode;
        CullMode m_cullMode;
        bool m_depthTest;
        bool m_depthWrite;

        /**
         * @brief Apply the current render state
         */
        void bindRenderState();

        /**
         * @brief Bind all textures to their shader uniforms
         */
        void bindTextures();

        /**
         * @brief Bind all parameters to their shader uniforms
         */
        void bindParameters();

        /**
         * @brief Find a parameter value including inheritance
         * @param name Parameter name
         * @param outValue Output parameter value
         * @return True if parameter was found
         */
        bool findParameter(const std::string& name, ParameterValue& outValue) const;

        /**
         * @brief Find a texture including inheritance
         * @param slot Texture slot
         * @param outTexture Output texture
         * @return True if texture was found
         */
        bool findTexture(TextureSlot slot, std::shared_ptr<Texture>& outTexture) const;
    };

    template<>
    inline int Material::getParameter<int>(const std::string& name, const int& defaultValue) const
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
    inline float Material::getParameter<float>(const std::string& name, const float& defaultValue) const
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
    inline glm::vec2 Material::getParameter<glm::vec2>(const std::string& name, const glm::vec2& defaultValue) const
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
    inline glm::vec3 Material::getParameter<glm::vec3>(const std::string& name, const glm::vec3& defaultValue) const
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
    inline glm::vec4 Material::getParameter<glm::vec4>(const std::string& name, const glm::vec4& defaultValue) const
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
    inline glm::mat4 Material::getParameter<glm::mat4>(const std::string& name, const glm::mat4& defaultValue) const
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

    template<typename T>
    T Material::getParameter(const std::string& name, const T& defaultValue) const
    {
        // Default implementation handles most common cases
        // Specialized implementations are in Material.cpp
        ParameterValue value;
        if (findParameter(name, value))
        {
            if (std::holds_alternative<T>(value.value))
            {
                return std::get<T>(value.value);
            }
        }
        return defaultValue;
    }

    template<typename T>
    void Material::setParameter(const std::string& name, const T& value)
    {
        // Use compile-time errors to limit to supported types
        static_assert(
            std::is_same_v<T, int> ||
            std::is_same_v<T, float> ||
            std::is_same_v<T, glm::vec2> ||
            std::is_same_v<T, glm::vec3> ||
            std::is_same_v<T, glm::vec4> ||
            std::is_same_v<T, glm::mat4>,
            "Unsupported parameter type"
            );

        ParameterValue param;
        param.value = value;
        m_parameters[name] = param;
    }

} // namespace PixelCraft::Rendering