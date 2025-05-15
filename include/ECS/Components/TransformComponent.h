// -------------------------------------------------------------------------
// TransformComponent.h
// -------------------------------------------------------------------------
#pragma once

#include "ECS/Component.h"
#include "ECS/ComponentRegistry.h"
#include "Utility/Transform.h"
#include "Utility/Serialization/SerializationUtility.h"

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace PixelCraft::ECS
{

    /**
     * @brief Component for managing spatial transformations
     *
     * TransformComponent handles position, rotation, and scale for entities.
     * It also supports parent-child relationships between entities.
     */
    class TransformComponent : public Component
    {
    public:
        /**
         * @brief Default constructor
         */
        TransformComponent();

        /**
         * @brief Initialize the component
         */
        void initialize() override;

        /**
         * @brief Create a clone of this component
         * @return A unique pointer to a new TransformComponent instance
         */
        std::unique_ptr<Component> clone() override;

        /**
         * @brief Set the parent entity for this transform
         * @param parent The entity ID of the parent
         */
        void setParent(EntityID parentID);

        /**
         * @brief Add a child entity to this transform
         * @param child The entity ID of the child to add
         */
        void addChild(EntityID child);

        /**
         * @brief Remove a child entity from this transform
         * @param child The entity ID of the child to remove
         */
        void removeChild(EntityID child);

        /**
         * @brief Clear all children
         */
        void clearChildren();

        /**
         * @brief Get the parent entity
         * @return The entity ID of the parent
         */
        EntityID getParent() const
        {
            return m_parent;
        }

        /**
         * @brief Get the list of child entities
         * @return Vector of child entity IDs
         */
        const std::vector<EntityID>& getChildren() const
        {
            return m_children;
        }

        /**
         * @brief Get the local position
         * @return Local position vector
         */
        const glm::vec3& getLocalPosition() const;

        /**
         * @brief Get the local rotation
         * @return Local rotation quaternion
         */
        const glm::quat& getLocalRotation() const;

        /**
         * @brief Get the local scale
         * @return Local scale vector
         */
        const glm::vec3& getLocalScale() const;

        /**
         * @brief Get the world transform
         * @return The world transform, combining parent transformations
         */
        Utility::Transform getWorldTransform();

        /**
         * @brief Update the world transform by combining local transform with parent transform
         */
        void updateWorldTransform();

        /**
         * @brief Mark the transform as dirty (needing recalculation)
         * Also marks all child transforms as dirty recursively
         */
        void markDirty();

        /**
         * @brief Set the local position
         * @param position The new local position
         */
        void setLocalPosition(const glm::vec3& position);

        /**
         * @brief Set the local rotation
         * @param rotation The new local rotation
         */
        void setLocalRotation(const glm::quat& rotation);

        /**
         * @brief Set the local scale
         * @param scale The new local scale
         */
        void setLocalScale(const glm::vec3& scale);

        /**
         * @brief Check if world transform dirty
         * @return True if world transform needs update
         */
        bool isWorldTransformDirty() const
        {
            return m_worldTransformDirty;
        }

        // Serialization methods
        Utility::Serialization::SerializationResult serialize(Utility::Serialization::Serializer& serializer) const override
        {
            // First serialize base component data
            auto result = Component::serialize(serializer);
            if (!result) return result;

            // Now serialize transform-specific data
            result = serializer.beginObject("TransformComponent");
            if (!result) return result;

            result = serializer.writeField("localPosition", m_localTransform.getPosition());
            if (!result) return result;

            result = serializer.writeField("localRotation", m_localTransform.getRotation());
            if (!result) return result;

            result = serializer.writeField("localScale", m_localTransform.getScale());
            if (!result) return result;

            result = serializer.writeField("parent", m_parent);
            if (!result) return result;

            // Serialize children as array of entity references
            result = serializer.beginArray("children", m_children.size());
            if (!result) return result;

            for (EntityID childID : m_children)
            {
                result = serializer.writeEntityRef(childID);
                if (!result) return result;
            }

            result = serializer.endArray();
            if (!result) return result;

            return serializer.endObject();
        }

        Utility::Serialization::SerializationResult deserialize(Utility::Serialization::Deserializer& deserializer) override
        {
            // First deserialize base component data
            auto result = Component::deserialize(deserializer);
            if (!result) return result;

            // Now deserialize transform-specific data
            result = deserializer.beginObject("TransformComponent");
            if (!result) return result;

            result = deserializer.readField("localPosition", m_localTransform.getPosition());
            if (!result) return result;

            result = deserializer.readField("localRotation", m_localTransform.getRotation());
            if (!result) return result;

            result = deserializer.readField("localScale", m_localTransform.getScale());
            if (!result) return result;

            result = deserializer.readField("parent", m_parent);
            if (!result) return result;

            // Read children array
            size_t childCount = 0;
            result = deserializer.beginArray("children", childCount);
            if (!result) return result;

            m_children.clear();
            m_children.reserve(childCount);

            for (size_t i = 0; i < childCount; ++i)
            {
                uint64_t childID;
                result = deserializer.readEntityRef(childID);
                if (!result) return result;

                m_children.push_back(childID);
            }

            result = deserializer.endArray();
            if (!result) return result;

            // Mark transform as dirty to recalculate world transform
            m_worldTransformDirty = true;

            return deserializer.endObject();
        }

        static void defineSchema(Utility::Serialization::Schema& schema)
        {
            // Include base component fields
            Component::defineSchema(schema);

            // Add transform-specific fields
            schema.addField("localPosition", Utility::Serialization::ValueType::Object);
            schema.addField("localRotation", Utility::Serialization::ValueType::Object);
            schema.addField("localScale", Utility::Serialization::ValueType::Object);
            schema.addField("parent", Utility::Serialization::ValueType::EntityRef);
            schema.addArrayField("children", Utility::Serialization::ValueType::EntityRef);
        }

        static std::string getTypeName()
        {
            return "TransformComponent";
        }

        // Define component type with ID 1 (first component type)
        DEFINE_COMPONENT_TYPE(TransformComponent, 1)

    private:
        Utility::Transform m_localTransform;     ///< Local space transform
        EntityID m_parent;                       ///< Parent entity ID
        std::vector<EntityID> m_children;        ///< Child entity IDs
        bool m_worldTransformDirty;              ///< Flag indicating world transform needs update
    };

} // namespace PixelCraft::ECS