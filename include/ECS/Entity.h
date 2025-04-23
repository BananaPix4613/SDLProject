// -------------------------------------------------------------------------
// Entity.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <string>
#include <cstdint>

namespace PixelCraft::ECS
{

    using EntityID = uint32_t;
    class Registry;

    /**
     * @brief Lightweight handle to an entity in the ECS architecture
     *
     * The Entity class provides a convenient interface for entity operations
     * and component management through the Registry. It serves as a handle
     * to a unique entity identifier and maintains a weak reference to the
     * Registry to avoid ownership cycles.
     */
    class Entity
    {
    public:
        /**
         * @brief Default constructor creates a null entity
         */
        Entity();

        /**
         * @brief Construct an entity with the given ID and Registry pointer
         * @param id The unique entity identifier
         * @param registry Pointer to the registry that owns this entity
         */
        Entity(EntityID id, Registry* registry);

        /**
         * @brief Construct an entity with the given ID and weak reference to Registry
         * @param id The unique entity identifier
         * @param registry Weak pointer to the registry that owns this entity
         */
        Entity(EntityID id, std::weak_ptr<Registry> registry);

        /**
         * @brief Get the entity's unique identifier
         * @return The entity ID
         */
        EntityID getID() const;

        /**
         * @brief Check if the entity is valid and exists in the registry
         * @return True if the entity exists in the registry, false otherwise
         */
        bool isValid() const;

        /**
         * @brief Destroy the entity, removing it from the registry
         *
         * This method marks the entity for destruction in the registry.
         * The actual destruction might be deferred until the next registry update.
         */
        void destroy();

        /**
         * @brief Enable or disable the entity
         * @param enabled True to enable the entity, false to disable
         *
         * Disabled entities are not processed by systems but remain in the registry.
         */
        void setEnabled(bool enabled);

        /**
         * @brief Check if the entity is enabled
         * @return True if the entity is enabled, false otherwise
         */
        bool isEnabled() const;

        /**
         * @brief Set the entity's name
         * @param name The name to assign to the entity
         */
        void setName(const std::string& name);

        /**
         * @brief Get the entity's name
         * @return The entity's name
         */
        const std::string& getName() const;

        /**
         * @brief Set the entity's tag
         * @param tag The tag to assign to the entity
         */
        void setTag(const std::string& tag);

        /**
         * @brief Get the entity's tag
         * @return The entity's tag
         */
        const std::string& getTag() const;

        /**
         * @brief Add a component to the entity
         * @tparam T The component type to add
         * @tparam Args Argument types for the component constructor
         * @param args Arguments forwarded to the component constructor
         * @return Pointer to the newly created component, or nullptr if failed
         */
        template<typename T, typename... Args>
        T* addComponent(Args&&... args);

        /**
         * @brief Get a component from the entity
         * @tparam T The component type to retrieve
         * @return Pointer to the component, or nullptr if the entity doesn't have it
         */
        template<typename T>
        T* getComponent();

        /**
         * @brief Remove a component from the entity
         * @tparam T The component type to remove
         * @return True if the component was removed, false if it wasn't found
         */
        template<typename T>
        bool removeComponent();

        /**
         * @brief Check if the entity has a specific component
         * @tparam T The component type to check for
         * @return True if the entity has the component, false otherwise
         */
        template<typename T>
        bool hasComponent() const;

        /**
         * @brief Check if the entity has all of the specified components
         * @tparam Components The component types to check for
         * @return True if the entity has all the specified components, false otherwise
         */
        template<typename... Components>
        bool hasComponents() const;

        /**
         * @brief Get the registry that owns this entity
         * @return Pointer to the owning registry, or nullptr if the registry has been destroyed
         */
        Registry* getRegistry() const;

        /**
         * @brief Equality comparison operator
         * @param other The entity to compare with
         * @return True if both entities reference the same entity ID and registry, false otherwise
         */
        bool operator==(const Entity& other) const;

        /**
         * @brief Inequality comparison operator
         * @param other The entity to compare with
         * @return True if the entities reference different entity IDs or registries, false otherwise
         */
        bool operator!=(const Entity& other) const;

        /**
         * @brief Less-than comparison operator for ordering in containers
         * @param other The entity to compare with
         * @return True if this entity's ID is less than the other's within the same registry, false otherwise
         */
        bool operator<(const Entity& other) const;

        /**
         * @brief Conversion operator to EntityID
         * @return The entity's ID
         */
        operator EntityID() const;

        /**
         * @brief Check if the entity is null (invalid ID or no registry)
         * @return True if the entity is null, false otherwise
         */
        bool isNull() const;

        /**
         * @brief Create a null entity (static factory method)
         * @return A null entity instance
         */
        static Entity Null();

    private:
        EntityID m_id;                       ///< The entity's unique identifier
        std::weak_ptr<Registry> m_registry;  ///< Weak reference to the owning registry
    };

} // namespace PixelCraft::ECS

// Template method implementations
#include "Entity.inl"

namespace std
{
    /**
     * @brief Hash function specialization for Entity
     *
     * Enables using Entity as a key in unordered containers.
     */
    template<>
    struct hash<PixelCraft::ECS::Entity>
    {
        /**
         * @brief Calculate hash value for an Entity
         * @param entity The entity to hash
         * @return Hash value based on the entity's ID
         */
        size_t operator()(const PixelCraft::ECS::Entity& entity) const
        {
            return hash<PixelCraft::ECS::EntityID>()(entity.getID());
        }
    };
}