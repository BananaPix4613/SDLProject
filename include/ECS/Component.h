// -------------------------------------------------------------------------
// Component.h
// -------------------------------------------------------------------------
#pragma once

#include "Utility/Serialization/SerializationUtility.h"
#include "ECS/ComponentTypes.h"

#include <memory>
#include <string>
#include <typeindex>
#include <functional>
#include <typeindex>

namespace PixelCraft::ECS
{

    namespace FlatBuffers
    {
        /**
         * @brief Schema definition for FlatBuffers serialization
         */
        class Schema
        {
        public:
            /**
             * @brief Get the schema definition as a string
             * @return String representation of the FlatBuffers schema
             */
            virtual std::string getDefinition() const = 0;

            /**
             * @brief Get the schema unique identifier
             * @return The unique identifier for this schema
             */
            virtual std::string getIdentifier() const = 0;

            /**
             * @brief Get the schema version
             * @return The schema version number
             */
            virtual uint32_t getVersion() const = 0;
        };
    }
    
    /**
     * @brief Base class for all components in the ECS architecture
     *
     * The Component class defines the interface that all components must implement.
     * It provides serialization capabilities, lifecycle methods, type identification,
     * and entity ownership tracking.
     */
    class Component
    {
    public:
        /**
         * @brief Default constructor
         */
        Component();

        /**
         * @brief Virtual destructor for proper inheritance
         */
        virtual ~Component() = default;

        /**
         * @brief Initialize the component
         *
         * Called after the component is created and attached to an entity.
         * Override in derived classes to implement component-specific initialization.
         */
        virtual void initialize();

        /**
         * @brief Create a copy of this component
         * @return A unique pointer to a new component instance
         */
        virtual std::unique_ptr<Component> clone() = 0;

        /**
         * @brief Get the component type ID
         * @return The unique type identifier for this component
         */
        ComponentTypeID getTypeID() const
        {
            return m_typeId;
        }

        /**
         * @brief Get the component type name
         * @return The string representation of the component type
         */
        std::string getComponentTypeName() const
        {
            return m_typeName;
        }

        /**
         * @brief Set the entity that owns this component
         * @param owner The entity ID of the owner
         */
        void setOwner(EntityID owner);

        /**
         * @brief Get the entity that owns this component
         * @return The entity ID of the owner
         */
        EntityID getOwner() const
        {
            return m_owner;
        }

        /**
         * @brief Get the component version
         * @return The version number for backward compatibility
         */
        uint32_t getVersion() const
        {
            return m_version;
        }

        /**
         * @brief Set the component version
         * @param version The version number to set
         */
        void setVersion(uint32_t version)
        {
            m_version = version;
        }

        /**
         * @brief Enable or disable the component
         * @param enabled True to enable, false to disable
         */
        void setEnabled(bool enabled);

        /**
         * @brief Check if the component is enabled
         * @return True if the component is enabled
         */
        bool isEnabled() const;

        // Virtual serialization methods
        virtual Utility::Serialization::SerializationResult serialize(Utility::Serialization::Serializer& serializer) const
        {
            auto result = serializer.beginObject("Component");
            if (!result) return result;

            result = serializer.writeField("owner", m_owner);
            if (!result) return result;

            result = serializer.writeField("version", m_version);
            if (!result) return result;

            return serializer.endObject();
        }

        virtual Utility::Serialization::SerializationResult deserialize(Utility::Serialization::Deserializer& deserializer)
        {
            auto result = deserializer.beginObject("Component");
            if (!result) return result;

            result = deserializer.readField("owner", m_owner);
            if (!result) return result;

            result = deserializer.readField("version", m_version);
            if (!result) return result;

            return deserializer.endObject();
        }

        static void defineSchema(Utility::Serialization::Schema& schema)
        {
            schema.addField("owner", Utility::Serialization::ValueType::EntityRef);
            schema.addField("version", Utility::Serialization::ValueType::UInt32);
        }

        static std::string getTypeName()
        {
            return "Component";
        }

    protected:
        EntityID m_owner;       ///< ID of the entity that owns this component
        uint32_t m_version;     ///< The component version for backward compatibility
        ComponentTypeID m_typeId; ///< The component type ID
        std::string m_typeName; ///< The component type name
        bool m_enabled;         ///< Flag indicating if this component is enabled
    };

} // namespace PixelCraft::ECS