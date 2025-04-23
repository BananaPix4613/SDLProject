// -------------------------------------------------------------------------
// RenderStage.h
// -------------------------------------------------------------------------
#pragma once

namespace PixelCraft::Rendering
{

    template<typename T>
    void RenderStage::setParameter(const std::string& name, const T& value)
    {
        ParameterValue param;
        param.value = value;
        m_parameters[name] = param;
    }

    template<typename T>
    T RenderStage::getParameter(const std::string& name, const T& defaultValue) const
    {
        auto it = m_parameters.find(name);
        if (it == m_parameters.end())
        {
            return defaultValue;
        }

        try
        {
            return std::get<T>(it->second.value);
        }
        catch (const std::bad_variant_access&)
        {
            Core::Logger::warn("RenderStage: Parameter type mismatch for '" + name + "' in stage '" + m_name + "'");
            return defaultValue;
        }
    }

} // namespace PixelCraft::Rendering