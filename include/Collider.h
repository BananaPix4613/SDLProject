#pragma once

#include <glm/glm.hpp>
#include <gtc/quaternion.hpp>
#include <vector>
#include <memory>

// Forward declarations
class PhysicsComponent;
class PhysicsSystem;

/**
 * @enum ColliderType
 * @brief Types of collider shapes
 */
enum class ColliderType
{
    BOX,        // Box (cuboid) collider
    SPHERE,     // Sphere collider
    CAPSULE,    // Capsule collider
    CYLINDER,   // Cylinder collider
    CONVEX_HULL,// Convex hull collider
    VOXEL_GRID  // Voxel-based collider for the grid system
};

/**
 * @class Collider
 * @brief Base class for all collision shapes
 * 
 * Colliders define the shape used for collision detection.
 * Each PhysicsComponent can have multiple colliders attached.
 */
class Collider
{
public:
    /**
     * @brief Constructor
     */
    Collider();

    /**
     * @brief Destructor
     */
    virtual ~Collider();

    /**
     * @brief Get the collider type
     * @return Type of this collider
     */
    virtual ColliderType getType() const = 0;

    /**
     * @brief Set collider local position relative to physics body
     * @param localPosition Offset from body center
     */
    void setLocalPosition(const glm::vec3& localPosition);

    /**
     * @brief Get collider local position
     * @return Local position offset
     */
    const glm::vec3& getLocalPosition() const;

    /**
     * @brief Set collider local rotation
     * @param localRotation Local rotation quaternion
     */
    void setLocalRotation(const glm::quat& localRotation);

    /**
     * @brief Get collider local rotation
     * @return Local rotation quaternion
     */
    const glm::quat& getLocalRotation() const;

    /**
     * @brief Set whether this is a trigger collider (no physical response)
     * @param isTrigger Whether collider is a trigger
     */
    void setTrigger(bool isTrigger);

    /**
     * @brief Check if collider is a trigger
     * @return True if collider is a trigger
     */
    bool isTrigger() const;

    /**
     * @brief Set collider material properties
     * @param friction Friction coefficient
     * @param restitution Bounciness coefficient
     */
    void setMaterial(float friction, float restitution);

    /**
     * @brief Get current friction
     * @return Friction coefficient
     */
    float getFriction() const;

    /**
     * @brief get current restitution
     * @return Restitution coefficient
     */
    float getRestitution() const;

    /**
     * @brief Set the physics component this collider belongs to
     * @param component Owner physics component
     */
    void setPhysicsComponent(PhysicsComponent* component);

    /**
     * @brief Get the owner physics component
     * @return Pointer to owner physics component
     */
    PhysicsComponent* getPhysicsComponent() const;

    /**
     * @brief Get world-space transform matrix for this collider
     * @return Matrix combining owner entity transform with local transform
     */
    glm::mat4 getWorldTransform() const;

    /**
     * @brief Get collider's bounding box in local space
     * @param outMin Reference to store min point
     * @param outMax Reference to store max point
     */
    virtual void getLocalBounds(glm::vec3& outMin, glm::vec3& outMax) const = 0;

    /**
     * @brief Get collider's bounding box in world space
     * @param outMin Reference to store min point
     * @param outMax Reference to store max point
     */
    void getWorldBounds(glm::vec3& outMin, glm::vec3& outMax) const;

    /**
     * @brief Check if point is inside collider
     * @param point Point in world space
     * @return True if point is inside
     */
    virtual bool containsPoint(const glm::vec3& point) const = 0;

    /**
     * @brief Get closest point on collider surface surface to given point
     * @param point Point in world space
     * @return Closest point on surface
     */
    virtual glm::vec3 getClosestPoint(const glm::vec3& point) const = 0;

protected:
    glm::vec3 m_localPosition;
    glm::quat m_localRotation;
    bool m_isTrigger;
    float m_friction;
    float m_restitution;
    PhysicsComponent* m_physicsComponent;

    friend class PhysicsSystem;
};

/**
 * @class BoxCollider
 * @brief Box-shaped (cuboid) collider
 */
class BoxCollider : public Collider
{
public:
    /**
     * @brief Constructor
     * @param size Box dimensions (width, height, depth)
     */
    BoxCollider(const glm::vec3& size = glm::vec3(1.0f));

    /**
     * @brief Get the collider type
     * @return ColliderType::BOX
     */
    ColliderType getType() const override;

    /**
     * @brief Set box size
     * @param size Box dimensions (width, height, depth)
     */
    void setSize(const glm::vec3& size);

    /**
     * @brief Get box size
     * @return Box dimensions (width, height, depth)
     */
    const glm::vec3& getSize() const;

    /**
     * @brief Get collider's bounding box in local space
     * @param outMin Reference to store min point
     * @param outMax Reference to store max point
     */
    void getLocalBounds(glm::vec3& outMin, glm::vec3& outMax) const override;

    /**
     * @brief Check if point is inside collider
     * @param point Point in world space
     * @return True if point is inside
     */
    bool containsPoint(const glm::vec3& point) const override;

    /**
     * @brief Get closest point on collider surface to given point
     * @param point Point in world space
     * @return Closest point on surface
     */
    glm::vec3 getClosestPoint(const glm::vec3& point) const override;

private:
    glm::vec3 m_size;
};

/**
 * @class SphereCollider
 * @brief Sphere-shaped collider
 */
class SphereCollider : public Collider
{
public:
    /**
     * @brief Constructor
     * @param radius Sphere radius
     */
    SphereCollider(float radius = 0.5f);

    /**
     * @brief Get the collider type
     * @return ColliderType::SPHERE
     */
    ColliderType getType() const override;

    /**
     * @brief Set sphere radius
     * @param radius Sphere radius
     */
    void setRadius(float radius);

    /**
     * @brief Get sphere radius
     * @return Sphere radius
     */
    float getRadius() const;

    /**
     * @brief get collider's bounding box in local space
     * @param outMin Reference to store min point
     * @param outMax Reference to store max point
     */
    void getLocalBounds(glm::vec3& outMin, glm::vec3& outMax) const override;

    /**
     * @brief Check if point is inside collider
     * @param point Point in world space
     * @return True if point is inside
     */
    bool containsPoint(const glm::vec3& point) const override;

    /**
     * @brief Get closest point on collider surface to given point
     * @param point Point in world space
     * @return Closest point on surface
     */
    glm::vec3 getClosestPoint(const glm::vec3& point) const override;

private:
    float m_radius;
};

/**
 * @class CapsuleCollider
 * @brief Capsule-shaped collider (cylinder with rounded ends)
 */
class CapsuleCollider : public Collider
{
public:
    /**
     * @brief Constructor
     * @param radius Capsule radius
     * @param height Capsule height (including rounded ends)
     */
    CapsuleCollider(float radius = 0.5f, float height = 2.0f);

    /**
     * @brief Get the collider type
     * @return ColliderType::CAPSULE
     */
    ColliderType getType() const override;

    /**
     * @brief Set capsule radius
     * @param radius Capsule radius
     */
    void setRadius(float radius);

    /**
     * @brief Get capsule radius
     * @return Capsule radius
     */
    float getRadius() const;

    /**
     * @brief Set capsule height
     * @param height Capsule height (including rounded ends)
     */
    void setHeight(float height);

    /**
     * @brief Get capsule height
     * @return Capsule height
     */
    float getHeight() const;

    /**
     * @brief Get collider's bounding box in local space
     * @param outMin Reference to store min point
     * @param outMax Reference to store max point
     */
    void getLocalBounds(glm::vec3& outMin, glm::vec3& outMax) const override;

    /**
     * @brief Check if point is inside collider
     * @param point Point in world space
     * @return True if point is inside
     */
    bool containsPoint(const glm::vec3& point) const override;

    /**
     * @brief Get closest point on collider surface to given point
     * @param point Point in world space
     * @return Closest point on surface
     */
    glm::vec3 getClosestPoint(const glm::vec3& point) const override;

private:
    float m_radius;
    float m_height;
};

/**
 * @class VoxelGridCollider
 * @brief Collider that uses the voxel grid for collision
 */
class VoxelGridCollider : public Collider
{
public:
    /**
     * @brief Constructor
     * @param gridRadius Radius of grid cells to include
     */
    VoxelGridCollider(int gridRadius = 1);

    /**
     * @brief Get the collider type
     * @return ColliderType::VOXEL_GRID
     */
    ColliderType getType() const override;

    /**
     * @brief Set grid radius
     * @param gridRadius Radius of grid cells to include
     */
    void setGridRadius(int gridRadius);

    /**
     * @brief Get grid radius
     * @return Grid radius
     */
    int getGridRadius() const;

    /**
     * @brief Get collider's bounding box in local space
     * @param outMin Reference to store min point
     * @param outMax Reference to store max point
     */
    void getLocalBounds(glm::vec3& outMin, glm::vec3& outMax) const override;

    /**
     * @brief Check if point is inside collider
     * @param point Point in world space
     * @return True if point is inside
     */
    bool containsPoint(const glm::vec3& point) const override;

    /**
     * @brief Get closest point on collider surface to given point
     * @param point Point in world space
     * @return Closest point on surface
     */
    glm::vec3 getClosestPoint(const glm::vec3& point) const override;

private:
    int m_gridRadius;
};