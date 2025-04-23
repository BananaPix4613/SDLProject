// -------------------------------------------------------------------------
// Material.inl - Template implementations for Material class
// -------------------------------------------------------------------------
#pragma once

namespace PixelCraft::Rendering
{

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

} // namespace PixelCraft::Rendering