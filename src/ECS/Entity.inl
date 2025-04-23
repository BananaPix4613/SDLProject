// -------------------------------------------------------------------------
// Entity.inl - Template implementations for Entity class
// -------------------------------------------------------------------------
#pragma once

#include "ECS/Registry.h"
#include "Core/Logger.h"

namespace Log = PixelCraft::Core;

namespace PixelCraft::ECS
{

    template<typename T, typename... Args>
    T* Entity::addComponent(Args&&... args)
    {
        if (!isValid())
        {
            Log::warn("Attempting to add component to invalid entity");
            return nullptr;
        }

        if (auto registry = m_registry.lock())
        {
            return registry->addComponent<T>(m_id, std::forward<Args>(args)...);
        }

        return nullptr;
    }

    template<typename T>
    T* Entity::getComponent()
    {
        if (!isValid())
        {
            return nullptr;
        }

        if (auto registry = m_registry.lock())
        {
            return registry->getComponent<T>(m_id);
        }

        return nullptr;
    }

    template<typename T>
    bool Entity::removeComponent()
    {
        if (!isValid())
        {
            Log::warn("Attempting to remove component from invalid entity");
            return false;
        }

        if (auto registry = m_registry.lock())
        {
            return registry->removeComponent<T>(m_id);
        }

        return false;
    }

    template<typename T>
    bool Entity::hasComponent() const
    {
        if (!isValid())
        {
            return false;
        }

        if (auto registry = m_registry.lock())
        {
            return registry->hasComponent<T>(m_id);
        }

        return false;
    }

    template<typename... Components>
    bool Entity::hasComponents() const
    {
        if (!isValid())
        {
            return false;
        }

        if (auto registry = m_registry.lock())
        {
            return registry->hasComponents<Components...>(m_id);
        }

        return false;
    }

} // namespace PixelCraft::ECS