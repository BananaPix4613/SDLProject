// -------------------------------------------------------------------------
// TransformComponent.cpp
// -------------------------------------------------------------------------
#include "ECS/Components/TransformComponent.h"
#include "ECS/Entity.h"
#include "ECS/Registry.h"
#include "ECS/ComponentRegistry.h"
#include "ECS/World.h"
#include "Core/Logger.h"
#include "Core/Application.h"

namespace PixelCraft::ECS
{

    TransformComponent::TransformComponent()
        : m_parent(INVALID_ENTITY_ID)
        , m_worldTransformDirty(true)
    {
        // Initialize the transform with identity values
        m_localTransform.setPosition(glm::vec3(0.0f));
        m_localTransform.setRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        m_localTransform.setScale(glm::vec3(1.0f));
    }

    void TransformComponent::initialize()
    {
        Component::initialize();
        m_typeId = 1;
        m_typeName = "TransformComponent";
        m_worldTransformDirty = true;
    }

    std::unique_ptr<Component> TransformComponent::clone()
    {
        auto clone = std::make_unique<TransformComponent>();

        // Copy local transform data
        clone->m_localTransform = m_localTransform;
        clone->m_worldTransformDirty = true;

        // Don't copy parent-child relationships as these are specific to entity instances
        return clone;
    }

    void TransformComponent::setParent(EntityID parent)
    {
        // Don't set parent to self
        if (parent == m_owner)
        {
            Core::warn("TransformComponent: Cannot set entity as its own parent");
            return;
        }

        // Remove from previous parent if exists
        if (m_parent != INVALID_ENTITY_ID)
        {
            auto world = Core::Application::getInstance()->getSubsystem<World>();
            if (world)
            {
                Entity parentEntity(m_parent, world->getRegistry());
                if (parentEntity.isValid() && parentEntity.hasComponent<TransformComponent>())
                {
                    auto& parentTransform = parentEntity.getComponent<TransformComponent>();
                    parentTransform.removeChild(m_owner);
                }
            }
        }

        m_parent = parent;
        m_worldTransformDirty = true;

        // Add to new parent's children list if parent exists
        if (m_parent != INVALID_ENTITY_ID)
        {
            auto world = Core::Application::getInstance()->getSubsystem<World>();
            if (world)
            {
                Entity parentEntity(m_parent, world->getRegistry());
                if (parentEntity.isValid() && parentEntity.hasComponent<TransformComponent>())
                {
                    auto& parentTransform = parentEntity.getComponent<TransformComponent>();
                    parentTransform.addChild(m_owner);
                }
            }
        }
    }

    void TransformComponent::addChild(EntityID child)
    {
        // Don't add self as child
        if (child == m_owner)
        {
            Core::warn("TransformComponent: Cannot add entity as its own child");
            return;
        }

        // Check if already a child
        auto it = std::find(m_children.begin(), m_children.end(), child);
        if (it != m_children.end())
        {
            return; // Already a child
        }

        m_children.push_back(child);

        // Set this entity as the parent of the child
        auto world = Core::Application::getInstance()->getSubsystem<World>();
        if (world)
        {
            Entity childEntity(child, world->getRegistry());
            if (childEntity.isValid() && childEntity.hasComponent<TransformComponent>())
            {
                auto& childTransform = childEntity.getComponent<TransformComponent>();
                // Avoid circular call by directly setting the parent field
                childTransform.m_parent = m_owner;
                childTransform.m_worldTransformDirty = true;
            }
        }
    }

    void TransformComponent::removeChild(EntityID child)
    {
        auto it = std::find(m_children.begin(), m_children.end(), child);
        if (it != m_children.end())
        {
            m_children.erase(it);

            // Clear parent reference in child
            auto world = Core::Application::getInstance()->getSubsystem<World>();
            if (world)
            {
                Entity childEntity(child, world->getRegistry());
                if (childEntity.isValid() && childEntity.hasComponent<TransformComponent>())
                {
                    auto& childTransform = childEntity.getComponent<TransformComponent>();
                    // Avoid circular call by directly setting the parent field
                    childTransform.m_parent = INVALID_ENTITY_ID;
                    childTransform.m_worldTransformDirty = true;
                }
            }
        }
    }

    void TransformComponent::clearChildren()
    {
        auto childrenCopy = m_children; // Make a copy since removeChild modifies m_children
        for (auto child : childrenCopy)
        {
            removeChild(child);
        }
        m_children.clear();
    }
    
    const glm::vec3& TransformComponent::getLocalPosition() const
    {
        return m_localTransform.getPosition();
    }

    const glm::quat& TransformComponent::getLocalRotation() const
    {
        return m_localTransform.getRotation();
    }

    const glm::vec3& TransformComponent::getLocalScale() const
    {
        return m_localTransform.getScale();
    }

    Utility::Transform TransformComponent::getWorldTransform()
    {
        if (m_worldTransformDirty)
        {
            updateWorldTransform();
        }

        return m_localTransform;
    }

    void TransformComponent::updateWorldTransform()
    {
        if (!m_worldTransformDirty)
        {
            return; // Already up to date
        }

        if (m_parent != INVALID_ENTITY_ID)
        {
            auto world = Core::Application::getInstance()->getSubsystem<World>();
            if (world)
            {
                Entity parentEntity(m_parent, world->getRegistry());
                if (parentEntity.isValid() && parentEntity.hasComponent<TransformComponent>())
                {
                    auto& parentTransform = parentEntity.getComponent<TransformComponent>();

                    // Get parent's world transform
                    Utility::Transform parentWorldTransform = parentTransform.getWorldTransform();

                    // Store local transform
                    Utility::Transform localCopy = m_localTransform;

                    // Combine parent world transform with local transform
                    m_localTransform = parentWorldTransform.combinedWith(localCopy);
                }
            }
        }

        m_worldTransformDirty = false;
    }

    void TransformComponent::markDirty()
    {
        if (m_worldTransformDirty)
        {
            return; // Already dirty
        }

        m_worldTransformDirty = true;

        // Mark all children as dirty recursively
        auto world = Core::Application::getInstance()->getSubsystem<World>();
        if (world)
        {
            for (EntityID childID : m_children)
            {
                Entity childEntity(childID, world->getRegistry());
                if (childEntity.isValid() && childEntity.hasComponent<TransformComponent>())
                {
                    auto& childTransform = childEntity.getComponent<TransformComponent>();
                    childTransform.markDirty();
                }
            }
        }
    }

    void TransformComponent::setLocalPosition(const glm::vec3& position)
    {
        m_localTransform.setPosition(position);
        markDirty();
    }

    void TransformComponent::setLocalRotation(const glm::quat& rotation)
    {
        m_localTransform.setRotation(rotation);
        markDirty();
    }

    void TransformComponent::setLocalScale(const glm::vec3& scale)
    {
        m_localTransform.setScale(scale);
        markDirty();
    }

    namespace
    {
        // Register the TransformComponent with the component registry
        bool registerTransformComponent()
        {
            ComponentRegistry::registerComponent<TransformComponent>("TransformComponent");
            return true;
        }

        const bool registered = registerTransformComponent();
    }

} // namespace PixelCraft::ECS