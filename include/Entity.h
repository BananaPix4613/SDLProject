#pragma once

#include <string>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <gtc/quaternion.hpp>

// Forward declaration
class Scene;
class Component;
class EntityManager;
using EntityID = uint64_t;

/**
 * @class Entity
 * @brief Represents a game object in the scene
 * 
 * Entities are containers for components that define behaviors and properties.
 * They support hierarchical relationships with parent-child transformations
 * and provide efficient component management.
 */
class Entity
{
public:
    /**
     * @brief Constructor
     * @param scene Pointer to owning scene
     * @param name Optional name for the entity (defaults to "Entity")
     */
    Entity(Scene* scene, const std::string& name = "Entity");

    /**
     * @brief Destructor
     * Automatically removes entity from scene and destroys all components
     */
    ~Entity();

    /**
     * @brief Add a component of type T with constructor arguments
     * @tparam T Component type to add
     * @tparam Args Constructor argument types
     * @param args Constructor arguments
     * @return Pointer to the newly added component or nullptr if failed
     */
    template<typename T, typename... Args>
    T* addComponent(Args&&... args)
    {
        // Check if component of this type already exists
        if (hasComponent<T>())
        {
            return getComponent<T>();
        }

        // Create component
        T* component = new T();

        // Add in components map
        std::type_index typeIndex = std::type_index(typeid(T));
        m_components[typeIndex] = component;

        // Set entity reference in component
        component->m_entity = this;

        // Initialize component
        component->initialize();

        // Notify entity manager about component addition
        if (m_scene)
        {
            EntityManager* entityManager = m_scene->getEntityManager();
            if (entityManager)
            {
                entityManager->onComponentAdded(this, typeIndex);
            }
        }

        return component;
    }

    /**
     * @brief Add an existing component
     * @param component Pointer to the component to add
     * @return True if the pointer was added successfully
     */
    bool addExistingComponent(Component* component);

    /**
     * @brief Get a component of type T
     * @tparam T Component type to retrieve
     * @return Pointer to component or nullptr if not found
     */
    template<typename T>
    T* getComponent()
    {
        std::type_index typeIndex = std::type_index(typeid(T));
        auto it = m_components.find(typeIndex);

        if (it != m_components.end())
        {
            return static_cast<T*>(it->second);
        }

        return nullptr;
    }

    /**
     * @brief Check if entity has a component of type T
     * @tparam T Component type to check for
     * @return True if component exists, false otherwise
     */
    template<typename T>
    bool hasComponent() const
    {
        std::type_index typeIndex = std::type_index(typeid(T));
        return m_components.find(typeIndex) != m_components.end();
    }

    /**
     * @brief Check if entity has a component of the specified type
     * @param typeIndex Type index of the component
     * @return True if component exists
     */
    bool hasComponentOfType(const std::type_index& typeIndex) const;

    /**
     * @brief Remove a component of type T
     * @tparam T Component type to remove
     * @return True if the component was removed
     */
    template<typename T>
    bool removeComponent()
    {
        std::type_index typeIndex = std::type_index(typeid(T));
        auto it = m_components.find(typeIndex);

        if (it != m_components.end())
        {
            // Notify entity manager about component removal
            if (m_scene)
            {
                EntityManager* entityManager = m_scene->getEntityManager();
                if (entityManager)
                {
                    entityManager->onComponentRemoved(this, typeIndex);
                }
            }

            // Call onDestroy on the component
            Component* component = it->second;
            component->onDestroy();

            // Remove and delete component
            m_components.erase(it);
            delete component;

            return true;
        }

        return false;
    }

    /**
     * @brief Get parent entity
     * @return Pointer to parent entity or nullptr if no parent
     */
    Entity* getParent() const;

    /**
     * @brief Set parent entity
     * @param parent Pointer to new parent entity
     */
    void setParent(Entity* parent);

    /**
     * @brief Add child entity
     * @param child Pointer to child entity to add
     */
    void addChild(Entity* child);

    /**
     * @brief Remove child entity
     * @param child Pointer to child entity to remove
     */
    void removeChild(Entity* child);

    /**
     * @brief Get entity name
     * @return Entity name
     */
    const std::string& getName() const;

    /**
     * @brief Set entity name
     * @param name New entity name
     */
    void setName(const std::string& name);

    /**
     * @brief Get entity tag
     * @return Entity tag
     */
    const std::string& getTag() const;

    /**
     * @brief Set entity tag
     * @param tag New entity tag
     */
    void setTag(const std::string& tag);

    /**
     * @brief Check if entity is active
     * @return True if entity is active, false otherwise
     */
    bool isActive() const;

    /**
     * @brief Set entity active state
     * @param active New active state
     */
    void setActive(bool active);

    /**
     * @brief Get local position
     * @return Local position vector
     */
    const glm::vec3& getPosition() const;

    /**
     * @brief Set local position
     * @param position New local position
     */
    void setPosition(const glm::vec3& position);

    /**
     * @brief Get local rotation
     * @return Local rotation quaternion
     */
    const glm::quat& getRotation() const;

    /**
     * @brief Set local rotation
     * @param rotation New rotation quaternion
     */
    void setRotation(const glm::quat& rotation);

    /**
     * @brief Set local rotation using Euler angles
     * @param angles Euler angles in degrees (pitch, yaw, roll)
     */
    void setRotationEuler(const glm::vec3& angles);

    /**
     * @brief Get local scale
     * @return Local scale vector
     */
    const glm::vec3& getScale() const;

    /**
     * @brief Set local scale
     * @param scale New local scale
     */
    void setScale(const glm::vec3& scale);

    /**
     * @brief Get world position
     * @return World position vector
     */
    glm::vec3 getWorldPosition() const;

    /**
     * @brief Get world rotation
     * @return World rotation quaternion
     */
    glm::quat getWorldRotation() const;

    /**
     * @brief Get world scale
     * @return World scale vector
     */
    glm::vec3 getWorldScale() const;

    /**
     * @brief Get local transform matrix
     * @return Local transform matrix
     */
    glm::mat4 getLocalTransform() const;

    /**
     * @brief Get world transform matrix
     * @return World transform matrix
     */
    glm::mat4 getWorldTransform() const;

    /**
     * @brief Set world position
     * @param position New world position
     */
    void setWorldPosition(const glm::vec3& position);

    /**
     * @brief Set world rotation
     * @param rotation New world rotation
     */
    void setWorldRotation(const glm::quat& rotation);

    /**
     * @brief Look at a target position
     * @param target Position to look at
     * @param up Up vector
     */
    void lookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));

    /**
     * @brief Update this entity and all its components
     * @param deltaTime Time since last frame
     */
    void update(float deltaTime);

    /**
     * @brief Get owning scene
     * @return Pointer to owning scene
     */
    Scene* getScene() const;

    /**
     * @brief Get unique entity ID
     * @return Entity ID
     */
    EntityID getID() const;

    /**
     * @brief Get all child entities
     * @return Vector of child entity pointers
     */
    const std::vector<Entity*>& getChildren() const;

    /**
     * @brief Find child entity by name
     * @param name Name to search for
     * @param recursive Whether to search recursively through children
     * @return Pointer to found entity or nullptr if not found
     */
    Entity* findChild(const std::string& name, bool recursive = false) const;

    /**
     * @brief Find child entity by tag
     * @param tag Tag to search for
     * @param recursive Whether to search recursively through children
     * @return Pointer to first matching entity or nullptr if not found
     */
    Entity* findChildByTag(const std::string& tag, bool recursive = false) const;

    /**
     * @brief Find all child entities with a tag
     * @param tag Tag to search for
     * @param recursive Whether to search recursively through children
     * @return Vector of matching entities
     */
    std::vector<Entity*> findChildrenByTag(const std::string& tag, bool recursive = false) const;

    /**
     * @brief Get all components
     * @return Map of components by type
     */
    const std::unordered_map<std::type_index, Component*>& getAllComponents() const;

    /**
     * @brief Clone all components to another entity
     * @param targetEntity Entity to clone components to
     */
    void cloneComponentsTo(Entity* targetEntity) const;

    /**
     * @brief Move in local space
     * @param direction Direction vector in local space
     * @param distance Distance to move
     */
    void moveLocal(const glm::vec3& direction, float distance);

    /**
     * @brief Rotate around a local axis
     * @param axis Axis to rotate around
     * @param angleDegrees Angle in degrees
     */
    void rotateLocal(const glm::vec3& axis, float angleDegrees);

    /**
     * @brief Check if this entity is a child of another entity
     * @param parent Parent entity to check against
     * @param recursive Whether to check recursively up the hierarchy
     * @return True if this is a child of the parent
     */
    bool isChildOf(Entity* parent, bool recursive = true) const;

    /**
     * @brief Convert a point from local to world space
     * @param localPoint Point in local space
     * @return Point in world space
     */
    glm::vec3 localToWorldPoint(const glm::vec3& localPoint) const;

    /**
     * @brief Convert a point from world to local space
     * @param worldPoint Point in world space
     * @return Point in local space
     */
    glm::vec3 worldToLocalPoint(const glm::vec3& worldPoint) const;

    /**
     * @brief Convert a direction from local to world space
     * @param localDirection Direction in local space
     * @return Direction in world space
     */
    glm::vec3 localToWorldDirection(const glm::vec3& localDirection) const;

    /**
     * @brief Convert a direction from world
     * @param worldDirection Direction in world space
     * @return Direction in local space
     */
    glm::vec3 worldToLocalDirection(const glm::vec3& worldDirection) const;

    /**
     * @brief Set the local transform matrix directly
     * @param transform Transform matrix
     */
    void setLocalTransform(const glm::mat4& transform);

    /**
     * @brief Set the world transform matrix directly
     * @param transform World transform matrix
     */
    void setWorldTransform(const glm::mat4& transform);

    /**
     * @brief Get the forward direction vector in world space
     * @return Forward direction
     */
    glm::vec3 getForward() const;

    /**
     * @brief Get the right direction vector in world space
     * @return Right direction
     */
    glm::vec3 getRight() const;

    /**
     * @brief Get the up direction vector in world space
     * @return Up direction
     */
    glm::vec3 getUp() const;

    /**
     * @brief Create a component by type index (for use with archetypes)
     * @param typeIndex Type index of component to create
     * @return Pointer to created component or nullptr if type is not registered
     */
    Component* createComponentByType(const std::type_index& typeIndex);

private:
    // Entity ID
    EntityID m_id;

    // Basic properties
    std::string m_name;
    std::string m_tag;
    bool m_active;

    // Hierarchy
    Entity* m_parent;
    std::vector<Entity*> m_children;
    Scene* m_scene;

    // Transform
    glm::vec3 m_position;
    glm::quat m_rotation;
    glm::vec3 m_scale;
    mutable glm::mat4 m_cachedLocalTransform;
    mutable glm::mat4 m_cachedWorldTransform;
    mutable bool m_transformDirty;
    mutable bool m_worldTransformDirty;

    // Components
    std::unordered_map<std::type_index, Component*> m_components;

    // Helper methods
    void updateTransform() const;
    void updateWorldTransform() const;
    void setTransformDirty();
    void setWorldTransformDirty();
    void propagateTransformDirty();
    void detachFromParent();

    // Allow EntityManager access to private members
    friend class EntityManager;
};