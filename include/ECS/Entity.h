// -------------------------------------------------------------------------
// Entity.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>

#include "ECS/Types.h"
#include "ECS/UUID.h"

namespace PixelCraft::ECS
{

    // Forward declarations
    class Registry;
    class Serializer;
    class Deserializer;

    /**
     * @brief Lightweight entity handle with component and serialization operations
     */
    class Entity
    {
    public:
        /**
         * @brief Default constructor creates a null entity
         */
        Entity();

        /**
         * @brief Constructor with ID and registry reference
         * @param id The entity ID
         * @param registry Weak reference to the registry
         */
        Entity(EntityID id, std::weak_ptr<Registry> registry);

        /**
         * @brief Check if the entity is valid
         * @return True if the entity is valid and exists in the registry
         */
        bool isValid() const;

        /**
         * @brief Destroy the entity and all its components
         */
        void destroy();

        /**
         * @brief Add a component to the entity
         * @tparam T Component type
         * @tparam Args Constructor argument types
         * @param args Constructor arguments
         * @return Reference to the created component
         */
        template<typename T, typename... Args>
        T& addComponent(Args&&... args);

        /**
         * @brief Remove a component from the entity
         * @tparam T Component type to remove
         * @return True if component was removed
         */
        template<typename T>
        bool removeComponent();

        /**
         * @brief Get a component from the entity
         * @tparam T Component type to retrieve
         * @return Reference to the component
         */
        template<typename T>
        T& getComponent();

        /**
         * @brief Get a const component from the entity
         * @tparam T Component type to retrieve
         * @return Const reference to the component
         */
        template<typename T>
        const T& getComponent() const;

        /**
         * @brief Check if the entity has a specific component
         * @tparam T Component type to check for
         * @return True if the entity has the component
         */
        template<typename T>
        bool hasComponent() const;

        /**
         * @brief Get the component mask for this entity
         * @return The component mask
         */
        const ComponentMask& getComponentMask() const;
        
        /**
         * @brief Serialize the entity to the given serializer
         * @param serializer The serializer to use
         */
        void serialize(Serializer& serializer);

        /**
         * @brief Deserialize the entity from the given deserializer
         * @param deserializer The deserializer to use
         */
        void deserialize(Deserializer& deserializer);

        /**
         * @brief Get the entity's UUID
         * @return The UUID of the entity
         */
        UUID getUUID() const;

        /**
         * @brief Set the entity's UUID
         * @param uuid The UUID to set
         */
        void setUUID(const UUID& uuid);

        /**
         * @brief Enable or disable UUID generation for this entity
         * @param needsUUID True to enable UUID generation
         */
        void setNeedsUUID(bool needsUUID);

        /**
         * @brief Check if the entity has UUID generation enabled
         * @return True if UUID generation is enabled
         */
        bool needsUUID() const;

        /**
         * @brief Set the entity's name
         * @param name The name to set
         */
        void setName(const std::string& name);

        /**
         * @brief Get the entity's name
         * @return The name of the entity
         */
        std::string getName() const;

        /**
         * @brief Add a tag to the entity
         * @param tag The tag to add
         */
        void addTag(const std::string& tag);

        /**
         * @brief Remove a tag from the entity
         * @param tag The tag to remove
         */
        void removeTag(const std::string& tag);

        /**
         * @brief Check if the entity has a tag
         * @param tag The tag to check for
         * @return True if the entity has the tag
         */
        bool hasTag(const std::string& tag) const;

        /**
         * @brief Set the entity's parent
         * @param parent The parent entity
         */
        void setParent(Entity parent);

        /**
         * @brief Get the entity's parent
         * @return The parent entity
         */
        Entity getParent() const;

        /**
         * @brief Get the entity's children
         * @return Vector of child entities
         */
        std::vector<Entity> getChildren() const;

        /**
         * @brief Set the entity's active state
         * @param active True to set active, false to set inactive
         */
        void setActive(bool active);

        /**
         * @brief Check if the entity is active
         * @return True if the entity is active
         */
        bool isActive() const;

        /**
         * @brief Get the registry the entity belongs to
         * @return Shared pointer to the registry
         */
        std::shared_ptr<Registry> getRegistry() const;

        /**
         * @brief Get the entity's ID
         * @return The ID of the entity
         */
        EntityID getID() const
        {
            return m_id;
        }

        /**
         * @brief Equality comparison
         * @param other Entity to compare with
         * @return True if entities are equal
         */
        bool operator==(const Entity& other) const
        {
            return m_id == other.m_id && m_registry.lock() == other.m_registry.lock();
        }

        /**
         * @brief Inequality comparison
         * @param other Entity to compare with
         * @return True if entities are not equal
         */
        bool operator!=(const Entity& other) const
        {
            return !(*this == other);
        }

        /**
         * @brief Check if the entity is null
         * @return True if this is a null entity
         */
        bool isNull() const;

        /**
         * @brief Create a null entity
         * @return A null entity
         */
        static Entity Null();

    private:
        EntityID m_id;                      ///< The entity's ID
        std::weak_ptr<Registry> m_registry; ///< Weak reference to the registry
    };

    // Template implementations
    template<typename T, typename... Args>
    T& Entity::addComponent(Args&&... args)
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            throw std::runtime_error("Entity::addComponent: Registry is invalid");
        }

        return registry->addComponent<T>(m_id, std::forward<Args>(args)...);
    }

    template<typename T>
    bool Entity::removeComponent()
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            return false;
        }

        return registry->removeComponent<T>(m_id);
    }

    template<typename T>
    T& Entity::getComponent()
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            throw std::runtime_error("Entity::getComponent: Registry is invalid");
        }

        return registry->getComponent<T>(m_id);
    }

    template<typename T>
    const T& Entity::getComponent() const
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            throw std::runtime_error("Entity::getComponent: Registry is invalid");
        }

        return registry->getComponent<T>(m_id);
    }

    template<typename T>
    bool Entity::hasComponent() const
    {
        auto registry = m_registry.lock();
        if (!registry)
        {
            return false;
        }

        return registry->hasComponent<T>(m_id);
    }

} // namespace PixelCraft::ECS

// Hash function for using Entity in unordered containers
namespace std
{
    template<>
    struct hash<PixelCraft::ECS::Entity>
    {
        size_t operator()(const PixelCraft::ECS::Entity& entity) const
        {
            return hash<PixelCraft::ECS::EntityID>()(entity.getID());
        }
    };
}