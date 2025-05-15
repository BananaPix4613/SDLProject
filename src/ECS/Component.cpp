// -------------------------------------------------------------------------
// Component.cpp
// -------------------------------------------------------------------------
#include "ECS/Component.h"
#include "Core/Logger.h"

namespace Log = PixelCraft::Core;

namespace PixelCraft::ECS
{

    // Component implementation
    Component::Component()
        : m_owner(0), m_version(1), m_enabled(true)
    {
        // Default constructor - initializes with no owner and version 1
    }

    void Component::initialize()
    {
        // Base implementation does nothing
        // Derived classes should override for specific initialization
    }

    void Component::setOwner(EntityID owner)
    {
        m_owner = owner;
    }

    void Component::setEnabled(bool enabled)
    {
        m_enabled = enabled;
    }

    bool Component::isEnabled() const
    {
        return m_enabled;
    }

} // namespace PixelCraft::ECS