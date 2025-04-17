#pragma once

#include <string>

// Forward declarations
class Entity;
class Scene;

/**
 * @class Component
 * @brief Base class for all components in the entity-component system
 * 
 * Components provide behavior and properties to entities. They are
 * initialized, updated, and destroyed based on the entity lifecycle.
 */
class Component
{
public:
    /**
     * @brief Default constructor
     */
    Component() : m_entity(nullptr), m_active(true), m_started(false)
    {
    }

    /**
     * @brief Virtual destructor
     */
    virtual ~Component()
    {
    }

    /**
     * @brief Initialize component
     * Called after component is added to entity
     */
    virtual void initialize()
    {
    }

    /**
     * @brief Start component
     * Called on first frame after initialization
     */
    virtual void start()
    {
    }

    /**
     * @brief Update component
     * Called each frame
     * @param deltaTime Time since last frame
     */
    virtual void update(float deltaTime)
    {
    }

    /**
     * @brief Render component
     * Called during rendering phase if visual
     * @param context Render context information
     */
    virtual void render()
    {
    }

    /**
     * @brief Called before component is destroyed
     * Used for cleanup and resource release
     */
    virtual void onDestroy()
    {
    }

    /**
     * @brief Get owning entity
     * @return Pointer to owning entity
     */
    Entity* getEntity() const
    {
        return m_entity;
    }

    /**
     * @brief Check if component is active
     * @return True if component is active, false otherwise
     */
    bool isActive() const
    {
        return m_active;
    }

    /**
     * @brief Set component active state
     * @param active New active state
     */
    void setActive(bool active)
    {
        m_active = active;
    }

    /**
     * @brief Get component from owning entity
     * @tparam T Component type to retrieve
     * @return Pointer to component or nullptr if not found
     */
    template<typename T>
    T* getComponent()
    {
        return m_entity ? m_entity->getComponent<T>() : nullptr;
    }

    /**
     * @brief Get component from another entity
     * @tparam T Component type to retrieve
     * @param entity Entity to get component from
     * @return Pointer to component or nullptr if not found
     */
    template<typename T>
    T* getComponent(Entity* entity)
    {
        return entity ? entity->getComponent<T>() : nullptr;
    }

    /**
     * @brief Get component type name
     * @return String representation of component type
     */
    virtual const char* getTypeName() const
    {
        return "Component";
    }

    /**
     * @brief Get the owning scene
     * @return Pointer to the scene
     */
    Scene* getScene() const;

protected:
    /**
     * @brief Notify component that entity transform has changed
     * Override to respond to transform changes
     */
    virtual void onTransformChanged()
    {
    }

private:
    Entity* m_entity;
    bool m_active;
    bool m_started;

    // Make Entity a friend class to allow direct access to m_entity
    friend class Entity;
};