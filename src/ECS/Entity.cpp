// -------------------------------------------------------------------------
// Entity.cpp
// -------------------------------------------------------------------------
#include "ECS/Entity.h"
#include "ECS/Registry.h"
#include "Core/Logger.h"

namespace Log = PixelCraft::Core;

namespace PixelCraft::ECS
{

    // Static constants
    constexpr EntityID NULL_ENTITY_ID = 0;

    Entity::Entity()
        : m_id(NULL_ENTITY_ID), m_registry()
    {
    }

    Entity::Entity(EntityID id, Registry* registry)
        : m_id(id), m_registry()
    {
        if (registry)
        {
            // Create a temporary shared_ptr to set the weak_ptr
            // This doesn't increase the reference count since we don't retain the shared_ptr
            std::shared_ptr<Registry> sharedRegistry(registry, [](Registry*) {});
            m_registry = sharedRegistry;
        }
    }

    Entity::Entity(EntityID id, std::weak_ptr<Registry> registry)
        : m_id(id), m_registry(registry)
    {
    }

    EntityID Entity::getID() const
    {
        return m_id;
    }

    bool Entity::isValid() const
    {
        if (isNull())
        {
            return false;
        }

        if (auto registry = m_registry.lock())
        {
            return registry->entityExists(m_id);
        }

        return false;
    }

    void Entity::destroy()
    {
        if (!isValid())
        {
            Log::warn("Attempting to destroy invalid entity");
            return;
        }

        if (auto registry = m_registry.lock())
        {
            registry->destroyEntity(m_id);
        }
    }

    void Entity::setEnabled(bool enabled)
    {
        if (!isValid())
        {
            Log::warn("Attempting to set enabled state on invalid entity");
            return;
        }

        if (auto registry = m_registry.lock())
        {
            registry->setEntityEnabled(m_id, enabled);
        }
    }

    bool Entity::isEnabled() const
    {
        if (!isValid())
        {
            return false;
        }

        if (auto registry = m_registry.lock())
        {
            return registry->isEntityEnabled(m_id);
        }

        return false;
    }

    void Entity::setName(const std::string& name)
    {
        if (!isValid())
        {
            Log::warn("Attempting to set name on invalid entity");
            return;
        }

        if (auto registry = m_registry.lock())
        {
            registry->setEntityName(m_id, name);
        }
    }

    const std::string& Entity::getName() const
    {
        static const std::string emptyString;

        if (!isValid())
        {
            return emptyString;
        }

        if (auto registry = m_registry.lock())
        {
            return registry->getEntityName(m_id);
        }

        return emptyString;
    }

    void Entity::setTag(const std::string& tag)
    {
        if (!isValid())
        {
            Log::warn("Attempting to set tag on invalid entity");
            return;
        }

        if (auto registry = m_registry.lock())
        {
            registry->setEntityTag(m_id, tag);
        }
    }

    const std::string& Entity::getTag() const
    {
        static const std::string emptyString;

        if (!isValid())
        {
            return emptyString;
        }

        if (auto registry = m_registry.lock())
        {
            return registry->getEntityTag(m_id);
        }

        return emptyString;
    }

    Registry* Entity::getRegistry() const
    {
        return m_registry.lock().get();
    }

    bool Entity::operator==(const Entity& other) const
    {
        return m_id == other.m_id && m_registry.lock() == other.m_registry.lock();
    }

    bool Entity::operator!=(const Entity& other) const
    {
        return !(*this == other);
    }

    bool Entity::operator<(const Entity& other) const
    {
        // Primary sort by registry address, secondary by ID
        auto thisReg = m_registry.lock().get();
        auto otherReg = other.m_registry.lock().get();

        if (thisReg != otherReg)
        {
            return thisReg < otherReg;
        }

        return m_id < other.m_id;
    }

    Entity::operator EntityID() const
    {
        return m_id;
    }

    bool Entity::isNull() const
    {
        return m_id == NULL_ENTITY_ID || m_registry.expired();
    }

    Entity Entity::Null()
    {
        return Entity();
    }

} // namespace PixelCraft::ECS