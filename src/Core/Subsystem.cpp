// -------------------------------------------------------------------------
// Subsystem.cpp
// -------------------------------------------------------------------------
#include "Core/Subsystem.h"
#include "Core/Logger.h"

namespace PixelCraft::Core
{

    Subsystem::Subsystem()
        : m_initialized(false)
        , m_active(true)
    {
    }

    Subsystem::~Subsystem()
    {
        if (m_initialized)
        {
            warn("Subsystem destroyed while still initialized: " + m_name);
        }
    }

    bool Subsystem::isInitialized() const
    {
        return m_initialized;
    }

    bool Subsystem::isActive() const
    {
        return m_active;
    }

    void Subsystem::setActive(bool active)
    {
        if (m_active != active)
        {
            m_active = active;
            debug("Subsystem " + getName() + (m_active ? " activated" : " deactivated"));
        }
    }

    std::vector<std::string> Subsystem::getDependencies() const
    {
        // Default implementation returns an empty dependencies list
        return {};
    }

} // namespace PixelCraft::Core