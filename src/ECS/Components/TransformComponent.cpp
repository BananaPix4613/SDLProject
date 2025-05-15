// -------------------------------------------------------------------------
// TransformComponent.cpp
// -------------------------------------------------------------------------
#include "ECS/Components/TransformComponent.h"
#include "ECS/Entity.h"
#include "ECS/Registry.h"
#include "ECS/World.h"
#include "Core/Logger.h"
#include "Core/Application.h"
#include "ECS/FlatBufferSerializer.h"

namespace Log = PixelCraft::Core;

namespace PixelCraft::ECS
{

    TransformComponent::TransformComponent()
        : m_parent(INVALID_ENTITY_ID), m_worldTransformDirty(true)
    {
        // Initialize the transform with identity values
        m_localTransform.setPosition(glm::vec3(0.0f));
        m_localTransform.setRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        m_localTransform.setScale(glm::vec3(1.0f));
    }

    void TransformComponent::initialize()
    {
        Component::initialize();
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
            Log::warn("TransformComponent: Cannot set entity as its own parent");
            return;
        }

        // Remove from previous parent if exists
        if (m_parent != INVALID_ENTITY_ID)
        {
            auto world = Core::Application::getInstance().getSubsystem<World>();
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
            auto world = Core::Application::getInstance().getSubsystem<World>();
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
            Log::warn("TransformComponent: Cannot add entity as its own child");
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
        auto world = Core::Application::getInstance().getSubsystem<World>();
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
            auto world = Core::Application::getInstance().getSubsystem<World>();
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
            auto world = Core::Application::getInstance().getSubsystem<World>();
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
        auto world = Core::Application::getInstance().getSubsystem<World>();
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

    void TransformComponent::serialize(Serializer& serializer)
    {
        // Call base class serialization first
        Component::serialize(serializer);

        // Serialize transform data
        serializer.beginObject("transform");

        // Serialize position
        glm::vec3 position = m_localTransform.getPosition();
        serializer.write<float>(position.x);
        serializer.write<float>(position.y);
        serializer.write<float>(position.z);

        // Serialize rotation
        glm::quat rotation = m_localTransform.getRotation();
        serializer.write<float>(rotation.w);
        serializer.write<float>(rotation.x);
        serializer.write<float>(rotation.y);
        serializer.write<float>(rotation.z);

        // Serialize scale
        glm::vec3 scale = m_localTransform.getScale();
        serializer.write<float>(scale.x);
        serializer.write<float>(scale.y);
        serializer.write<float>(scale.z);

        serializer.endObject();

        // Serialize parent-child relationships
        serializer.write<EntityID>(m_parent);

        // Serialize children
        size_t childCount = m_children.size();
        serializer.beginArray("children", childCount);
        for (EntityID child : m_children)
        {
            serializer.writeEntityRef(child);
        }
        serializer.endArray();
    }

    void TransformComponent::deserialize(Deserializer& deserializer)
    {
        // Call base class deserialization first
        Component::deserialize(deserializer);

        // Deserialize transform data
        if (deserializer.beginObject("transform"))
        {
            // Deserialize position
            float x, y, z;
            deserializer.read<float>(x);
            deserializer.read<float>(y);
            deserializer.read<float>(z);
            m_localTransform.setPosition(glm::vec3(x, y, z));

            // Deserialize rotation
            float w, rx, ry, rz;
            deserializer.read<float>(w);
            deserializer.read<float>(rx);
            deserializer.read<float>(ry);
            deserializer.read<float>(rz);
            m_localTransform.setRotation(glm::quat(w, rx, ry, rz));

            // Deserialize scale
            float sx, sy, sz;
            deserializer.read<float>(sx);
            deserializer.read<float>(sy);
            deserializer.read<float>(sz);
            m_localTransform.setScale(glm::vec3(sx, sy, sz));

            deserializer.endObject();
        }

        // Deserialize parent
        deserializer.read<EntityID>(m_parent);

        // Deserialize children
        m_children.clear();
        size_t childCount;
        if (deserializer.beginArray("children", childCount))
        {
            for (size_t i = 0; i < childCount; ++i)
            {
                EntityID childID = deserializer.readEntityRef();
                m_children.push_back(childID);
            }
            deserializer.endArray();
        }

        m_worldTransformDirty = true;
    }

    std::unique_ptr<FlatBuffers::Schema> TransformComponent::createSchema()
    {
        // This implementation would create the FlatBuffers schema
        // for efficient serialization of the TransformComponent
        class TransformSchema : public FlatBuffers::Schema
        {
        public:
            std::string getDefinition() const override
            {
                return R"(
namespace PixelCraft.ECS;

struct Vec3 {
  x:float;
  y:float;
  z:float;
}

struct Quat {
  x:float;
  y:float;
  z:float;
  w:float;
}

table TransformComponentData {
  local_position:Vec3;
  local_rotation:Quat;
  local_scale:Vec3;
  parent:uint32 = 0;
  children:[uint32];
  world_transform_dirty:bool = true;
}

root_type TransformComponentData;
            )";
            }

            std::string getIdentifier() const override
            {
                return "TRFM"; // PixelCraft Transform FlatBuffer
            }

            uint32_t getVersion() const override
            {
                return 1; // Initial schema version
            }
        };

        return std::make_unique<TransformSchema>();
    }

    namespace  
    {  
       // Register the TransformComponent with the component registry
       static bool registered = ComponentRegistry::registerComponentType<TransformComponent>("TransformComponent");  
    }

} // namespace PixelCraft::ECS