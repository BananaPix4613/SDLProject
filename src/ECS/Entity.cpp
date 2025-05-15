// -------------------------------------------------------------------------
// Entity.cpp
// -------------------------------------------------------------------------
#include "ECS/Entity.h"
#include "ECS/Registry.h"
#include "Core/Logger.h"

namespace Log = PixelCraft::Core;

namespace PixelCraft::ECS
{

    Entity::Entity() : m_id(INVALID_ENTITY_ID)
    {
        // Default constructor creates a null entity
    }

    Entity::Entity(EntityID id, std::weak_ptr<Registry> registry)
        : m_id(id), m_registry(registry)
    {
        // Constructor with ID and registry reference
    }

    bool Entity::isValid() const
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            return false;
        }

        return registry->isValid(m_id);
    }

    void Entity::destroy()
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            return;
        }

        registry->destroyEntity(m_id);
    }

    const ComponentMask& Entity::getComponentMask() const
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            static ComponentMask emptyMask;
            return emptyMask;
        }

        return registry->getEntityMask(m_id);
    }

    void Entity::serialize(Serializer& serializer)
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            Log::error("Entity::serialize: Registry is invalid");
            return;
        }

        registry->serialize(m_id, serializer);
    }

    void Entity::deserialize(Deserializer& deserializer)
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            Log::error("Entity::deserialize: Registry is invalid");
            return;
        }

        registry->deserialize(m_id, deserializer);
    }

    UUID Entity::getUUID() const
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            return UUID::createNull();
        }

        return registry->getEntityUUID(m_id);
    }

    void Entity::setUUID(const UUID& uuid)
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            return;
        }

        registry->setEntityUUID(m_id, uuid);
    }

    void Entity::setNeedsUUID(bool needsUUID)
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            return;
        }

        registry->setEntityNeedsUUID(m_id, needsUUID);
    }

    bool Entity::needsUUID() const
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            return false;
        }

        return registry->entityNeedsUUID(m_id);
    }

    void Entity::setName(const std::string& name)
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            return;
        }

        registry->setEntityName(m_id, name);
    }

    std::string Entity::getName() const
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            return "";
        }

        return registry->getEntityName(m_id);
    }

    void Entity::addTag(const std::string& tag)
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            return;
        }

        registry->addTag(m_id, tag);
    }

    void Entity::removeTag(const std::string& tag)
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            return;
        }

        registry->removeTag(m_id, tag);
    }

    bool Entity::hasTag(const std::string& tag) const
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            return false;
        }

        return registry->hasTag(m_id, tag);
    }

    void Entity::setParent(Entity parent)
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            return;
        }

        registry->setEntityParent(m_id, parent.getID());
    }

    Entity Entity::getParent() const
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            return Entity::Null();
        }

        EntityID parentID = registry->getEntityParent(m_id);
        if (parentID == INVALID_ENTITY_ID)
        {
            return Entity::Null();
        }

        return Entity(parentID, m_registry);
    }

    std::vector<Entity> Entity::getChildren() const
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            return {};
        }

        std::vector<EntityID> childIDs = registry->getEntityChildren(m_id);
        std::vector<Entity> children;
        children.reserve(childIDs.size());

        for (EntityID childID : childIDs)
        {
            children.emplace_back(childID, m_registry);
        }

        return children;
    }

    void Entity::setActive(bool active)
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            return;
        }

        registry->setEntityActive(m_id, active);
    }

    bool Entity::isActive() const
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            return false;
        }

        return registry->isEntityActive(m_id);
    }

    std::shared_ptr<Registry> Entity::getRegistry() const
    {
        return m_registry.lock();
    }

    bool Entity::isNull() const
    {
        return m_id == INVALID_ENTITY_ID || !m_registry.lock();
    }

    Entity Entity::Null()
    {
        return Entity();
    }

} // namespace PixelCraft::ECS