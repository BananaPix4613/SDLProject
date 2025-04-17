#include "Entity.h"
#include "Scene.h"
#include "Component.h"
#include "EntityManager.h"
#include <gtc/matrix_transform.hpp>
#include <gtx/quaternion.hpp>

Entity::Entity(Scene* scene, const std::string& name) :
    m_id(0),
    m_name(name),
    m_tag(""),
    m_active(true),
    m_parent(nullptr),
    m_scene(scene),
    m_position(0.0f, 0.0f, 0.0f),
    m_rotation(1.0f, 0.0f, 0.0f, 0.0f),   // Identity quaternion
    m_scale(1.0f, 1.0f, 1.0f),
    m_transformDirty(true),
    m_worldTransformDirty(true)
{
    // Identity matrices
    m_cachedLocalTransform = glm::mat4(1.0f);
    m_cachedWorldTransform = glm::mat4(1.0f);
}

Entity::~Entity()
{
    // Remove from parent
    detachFromParent();

    // Remove all children (without deleting them - scene handles deletion)
    for (auto child : m_children)
    {
        child->m_parent = nullptr;
    }
    m_children.clear();

    // Delete all components
    for (auto& pair : m_components)
    {
        pair.second->onDestroy();
        delete pair.second;
    }
    m_components.clear();
}

bool Entity::addExistingComponent(Component* component)
{
    if (!component)
    {
        return false;
    }

    // Get component type
    std::type_index typeIndex = std::type_index(typeid(*component));

    // Check if component of this type already exists
    if (m_components.find(typeIndex) != m_components.end())
    {
        // Component of this type already exists
        return false;
    }

    // Add component
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

    return true;
}

bool Entity::hasComponentOfType(const std::type_index& typeIndex) const
{
    return m_components.find(typeIndex) != m_components.end();
}

Entity* Entity::getParent() const
{
    return m_parent;
}

void Entity::setParent(Entity* parent)
{
    // No change if parent is the same
    if (m_parent == parent)
    {
        return;
    }

    // Check for circular reference
    Entity* checkParent = parent;
    while (checkParent)
    {
        if (checkParent == this)
        {
            // Circular reference detected
            return;
        }
        checkParent = checkParent->getParent();
    }

    // Remove from current parent
    detachFromParent();

    // Set new parent
    m_parent = parent;

    // Add to new parent's children
    if (m_parent)
    {
        m_parent->addChild(this);
    }

    // Update transforms
    setTransformDirty();
}

void Entity::addChild(Entity* child)
{
    if (!child) return;

    // Check if already a child
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it == m_children.end())
    {
        // Add child
        m_children.push_back(child);

        // Update child's parent reference (without triggering recursion)
        if (child->m_parent != this)
        {
            // Temporarily disconnect from current parent
            if (child->m_parent)
            {
                Entity* oldParent = child->m_parent;
                child->m_parent = nullptr;
                oldParent->removeChild(child);
            }

            // Set new parent
            child->m_parent = this;
        }

        // Update transforms
        child->setTransformDirty();
    }
}

void Entity::removeChild(Entity* child)
{
    if (!child) return;

    // Find and remove from children vector
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end())
    {
        m_children.erase(it);

        // Update child's parent reference (without triggering recursion)
        if (child->m_parent == this)
        {
            child->m_parent = nullptr;
        }

        // Update transforms
        child->setTransformDirty();
    }
}

const std::string& Entity::getName() const
{
    return m_name;
}

void Entity::setName(const std::string& name)
{
    // Check if name is different
    if (m_name == name)
    {
        return;
    }

    // Store old name for entity manager update
    std::string oldName = m_name;

    // Update name
    m_name = name;

    // Update entity manager name index
    if (m_scene)
    {
        EntityManager* entityManager = m_scene->getEntityManager();
        if (entityManager)
        {
            entityManager->updateNameIndex(this, oldName, m_name);
        }
    }
}

const std::string& Entity::getTag() const
{
    return m_tag;
}

void Entity::setTag(const std::string& tag)
{
    // Check if tag is different
    if (m_tag == tag)
    {
        return;
    }

    // Store old tag for entity manager update
    std::string oldTag = m_tag;

    // Update tag
    m_tag = tag;

    // Update entity manager tag index
    if (m_scene)
    {
        EntityManager* entityManager = m_scene->getEntityManager();
        if (entityManager)
        {
            entityManager->updateTagIndex(this, oldTag, m_tag);
        }
    }
}

bool Entity::isActive() const
{
    return m_active;
}

void Entity::setActive(bool active)
{
    m_active = active;
}

const glm::vec3& Entity::getPosition() const
{
    return m_position;
}

void Entity::setPosition(const glm::vec3& position)
{
    m_position = position;
    setTransformDirty();
}

const glm::quat& Entity::getRotation() const
{
    return m_rotation;
}

void Entity::setRotation(const glm::quat& rotation)
{
    m_rotation = rotation;
    setTransformDirty();
}

void Entity::setRotationEuler(const glm::vec3& angles)
{
    // Convert Euler angles (in degrees) to quaternion
    glm::vec3 radians = glm::radians(angles);
    m_rotation = glm::quat(radians);
    setTransformDirty();
}

const glm::vec3& Entity::getScale() const
{
    return m_scale;
}

void Entity::setScale(const glm::vec3& scale)
{
    m_scale = scale;
    setTransformDirty();
}

glm::vec3 Entity::getWorldPosition() const
{
    // Extract position from world transform matrix
    const glm::mat4& worldMatrix = getWorldTransform();
    return glm::vec3(worldMatrix[3]); // Fourth column contains position
}

glm::quat Entity::getWorldRotation() const
{
    if (m_parent)
    {
        // Combine parent and local rotations
        return m_parent->getWorldRotation() * m_rotation;
    }
    return m_rotation;
}

glm::vec3 Entity::getWorldScale() const
{
    if (m_parent)
    {
        // Component-wise multiplication of scales
        glm::vec3 parentScale = m_parent->getWorldScale();
        return glm::vec3(
            parentScale.x * m_scale.x,
            parentScale.y * m_scale.y,
            parentScale.z * m_scale.z
        );
    }
    return m_scale;
}

glm::mat4 Entity::getLocalTransform() const
{
    updateTransform();
    return m_cachedLocalTransform;
}

glm::mat4 Entity::getWorldTransform() const
{
    updateTransform();
    return m_cachedWorldTransform;
}

void Entity::setWorldPosition(const glm::vec3& position)
{
    if (m_parent)
    {
        // Convert world position to local position
        glm::vec3 localPos = m_parent->worldToLocalPoint(position);
        setPosition(localPos);
    }
    else
    {
        // No parent, so world position is local position
        setPosition(position);
    }
}

void Entity::setWorldRotation(const glm::quat& rotation)
{
    if (m_parent)
    {
        // Calculate local rotation
        glm::quat parentWorldRotInv = glm::inverse(m_parent->getWorldRotation());
        setRotation(parentWorldRotInv * rotation);
    }
    else
    {
        // No parent, so world rotation is local rotation
        setRotation(rotation);
    }
}

void Entity::lookAt(const glm::vec3& target, const glm::vec3& up)
{
    // Calculate direction vector
    glm::vec3 direction = glm::normalize(target - getWorldPosition());

    // Use glm::lookAt to create a view matrix
    glm::mat4 lookAtMatrix = glm::lookAt(getWorldPosition(), target, up);

    // Extract rotation from look-at matrix and invert (view to model)
    glm::quat worldRotation = glm::quat_cast(glm::inverse(lookAtMatrix));

    // Set world rotation
    setWorldRotation(worldRotation);
}

void Entity::update(float deltaTime)
{
    if (!m_active) return;

    // Update all components
    for (auto& pair : m_components)
    {
        Component* component = pair.second;
        if (component->isActive())
        {
            // Call start on first update
            if (!component->m_started)
            {
                component->start();
                component->m_started = true;
            }

            // Update the component
            component->update(deltaTime);
        }
    }

    // Update children
    for (auto child : m_children)
    {
        child->update(deltaTime);
    }
}

Scene* Entity::getScene() const
{
    return m_scene;
}

EntityID Entity::getID() const
{
    return m_id;
}

const std::vector<Entity*>& Entity::getChildren() const
{
    return m_children;
}

Entity* Entity::findChild(const std::string& name, bool recursive) const
{
    // Linear search through children
    for (auto child : m_children)
    {
        if (child->getName() == name)
        {
            return child;
        }

        // Recursive search if specified
        if (recursive)
        {
            Entity* found = child->findChild(name, true);
            if (found)
            {
                return found;
            }
        }
    }

    return nullptr;
}

Entity* Entity::findChildByTag(const std::string& tag, bool recursive) const
{
    // Linear search through children
    for (auto child : m_children)
    {
        if (child->getTag() == tag)
        {
            return child;
        }

        // Recursive search if specified
        if (recursive)
        {
            Entity* found = child->findChildByTag(tag, true);
            if (found)
            {
                return found;
            }
        }
    }

    return nullptr;
}

std::vector<Entity*> Entity::findChildrenByTag(const std::string& tag, bool recursive) const
{
    std::vector<Entity*> result;

    // Linear search through children
    for (auto child : m_children)
    {
        if (child->getTag() == tag)
        {
            result.push_back(child);
        }

        // Recursive search if specified
        if (recursive)
        {
            std::vector<Entity*> childResults = child->findChildrenByTag(tag, true);
            result.insert(result.end(), childResults.begin(), childResults.end());
        }
    }

    return result;
}

const std::unordered_map<std::type_index, Component*>& Entity::getAllComponents() const
{
    return m_components;
}

void Entity::cloneComponentsTo(Entity* targetEntity) const
{
    if (!targetEntity)
    {
        return;
    }

    // This is a simplified implementation since we can't clone arbitrary components
    // without knowing their concrete types. In a real implementation, each component
    // would need to implement a clone method.

    // For the components we know about, we could implement cloning:
    // Example (if we had concrete component types):
    /*
    if (RenderComponent* renderComp = getComponent<RenderComponent>())
    {
        RenderComponent* closedRender = targetEntity->addComponent<RenderComponent>();
        clonedRender->setModel(renderComp->getModel());
        closedRender->setMaterial(renderComp->getMaterial());
    }
    */
}

void Entity::moveLocal(const glm::vec3& direction, float distance)
{
    // Transform direction from local to world space
    glm::vec3 worldDirection = localToWorldDirection(glm::normalize(direction));

    // Move in world space
    m_position += worldDirection * distance;

    // Update transform
    setTransformDirty();
}

void Entity::rotateLocal(const glm::vec3& axis, float angleDegrees)
{
    // Create rotation quaternion
    glm::quat rotation = glm::angleAxis(glm::radians(angleDegrees), glm::normalize(axis));

    // Apply rotation to current rotation
    m_rotation = m_rotation * rotation;

    // Update transform
    setTransformDirty();
}

bool Entity::isChildOf(Entity* parent, bool recursive) const
{
    if (!parent)
    {
        return false;
    }

    // Check direct parent
    if (m_parent == parent)
    {
        return true;
    }

    // Check recursively if needed
    if (recursive && m_parent)
    {
        return m_parent->isChildOf(parent, true);
    }

    return false;
}

glm::vec3 Entity::localToWorldPoint(const glm::vec3& localPoint) const
{
    // Transform point using world transform matrix
    glm::vec4 worldPoint = getWorldTransform() * glm::vec4(localPoint, 1.0f);
    return glm::vec3(worldPoint);
}

glm::vec3 Entity::worldToLocalPoint(const glm::vec3& worldPoint) const
{
    // Transform point using inverse world transform matrix
    glm::mat4 worldToLocal = glm::inverse(getWorldTransform());
    glm::vec4 localPoint = worldToLocal * glm::vec4(worldPoint, 1.0f);
    return glm::vec3(localPoint);
}

glm::vec3 Entity::localToWorldDirection(const glm::vec3& localDirection) const
{
    // Transform direction using world transform matrix (ignoring translation)
    glm::mat3 rotationMatrix = glm::mat3(getWorldTransform());
    return rotationMatrix * localDirection;
}

glm::vec3 Entity::worldToLocalDirection(const glm::vec3& worldDirection) const
{
    // Transform direction using inverse world transform matrix (ignoring translation)
    glm::mat3 worldToLocal = glm::mat3(glm::inverse(getWorldTransform()));
    return worldToLocal * worldDirection;
}

void Entity::setLocalTransform(const glm::mat4& transform)
{
    // Extract position, rotation, and scale from transform matrix
    m_position = glm::vec3(transform[3]);

    // Extract rotation
    glm::mat3 rotationMatrix = glm::mat3(transform);

    // Normalize rotation matrix (remove scale)
    glm::vec3 col0 = glm::normalize(glm::vec3(rotationMatrix[0]));
    glm::vec3 col1 = glm::normalize(glm::vec3(rotationMatrix[1]));
    glm::vec3 col2 = glm::normalize(glm::vec3(rotationMatrix[2]));
    rotationMatrix = glm::mat3(col0, col1, col2);

    m_rotation = glm::quat_cast(rotationMatrix);

    // Extract scale
    m_scale.x = glm::length(glm::vec3(transform[0]));
    m_scale.y = glm::length(glm::vec3(transform[1]));
    m_scale.z = glm::length(glm::vec3(transform[2]));

    // Update transform
    setTransformDirty();
}

void Entity::setWorldTransform(const glm::mat4& transform)
{
    if (m_parent)
    {
        // Convert world transform to local transform
        glm::mat4 parentWorldInv = glm::inverse(m_parent->getWorldTransform());
        glm::mat4 localTransform = parentWorldInv * transform;
        setLocalTransform(localTransform);
    }
    else
    {
        // No parent, so world transform is local transform
        setLocalTransform(transform);
    }
}

glm::vec3 Entity::getForward() const
{
    // Forward is negative Z axis transformed by rotation
    return localToWorldDirection(glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::vec3 Entity::getRight() const
{
    // Right is X axis transformed by rotation
    return localToWorldDirection(glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 Entity::getUp() const
{
    // Up is Y axis transformed by rotation
    return localToWorldDirection(glm::vec3(0.0f, 1.0f, 0.0f));
}

Component* Entity::createComponentByType(const std::type_index& typeIndex)
{
    // This is a placeholder implementation since we can't create components
    // from type_index without a factory system.
    // In a real implementation, the EntityManager would register component factories
    // and we would use those factories here.

    return nullptr;
}

void Entity::updateTransform() const
{
    if (m_transformDirty)
    {
        // Compute local transform matrix
        m_cachedLocalTransform = glm::translate(glm::mat4(1.0f), m_position);
        m_cachedLocalTransform = m_cachedLocalTransform * glm::mat4_cast(m_rotation);
        m_cachedLocalTransform = glm::scale(m_cachedLocalTransform, m_scale);

        m_transformDirty = false;
        m_worldTransformDirty = true;
    }
}

void Entity::updateWorldTransform() const
{
    updateTransform();

    if (m_worldTransformDirty)
    {
        // Compute world transform matrix
        if (m_parent)
        {
            m_cachedWorldTransform = m_parent->getWorldTransform() * m_cachedLocalTransform;
        }
        else
        {
            m_cachedWorldTransform = m_cachedLocalTransform;
        }

        m_worldTransformDirty = false;

        // Notify components that transform has changed
        for (auto& pair : m_components)
        {
            pair.second->onTransformChanged();
        }
    }
}

void Entity::setTransformDirty()
{
    m_transformDirty = true;
    setWorldTransformDirty();
}

void Entity::setWorldTransformDirty()
{
    m_worldTransformDirty = true;

    // Propagate to children
    propagateTransformDirty();
}

void Entity::propagateTransformDirty()
{
    // Mark all children as dirty
    for (auto child : m_children)
    {
        child->setWorldTransformDirty();
    }
}

void Entity::detachFromParent()
{
    if (m_parent)
    {
        Entity* parent = m_parent;
        m_parent = nullptr;
        parent->removeChild(this);
    }
}