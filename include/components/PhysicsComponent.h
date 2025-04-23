#pragma once

#include "Component.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <functional>

// Foward declarations
class PhysicsSystem;
class Collider;
struct CollisionInfo;

/**
 * @enum PhysicsBodyType
 * @brief Defines the type of physics body behavior
 */
enum class PhysicsBodyType
{
    STATIC,     // Immovable body, doesn't respond to forces (platforms, walls)
    DYNAMIC,    // Movable body affected by forces and gravity (players, objects)
    KINEMATIC,  // Movable body controlled by code, not physics (moving platforms)
    TRIGGER     // Non-solid body that detects overlaps but doesn't collide (pickup areas)
};

/**
 * @class PhysicsComponent
 * @brief Allows an entity to participate in physics simulation
 * 
 * PhysicsComponent provides physics properties like mass, velocity,
 * and collision behavior. It integrates with the PhysicsSystem for
 * simulation and collision detection.
 */
class PhysicsComponent : public Component
{
public:
    /**
     * @brief Constructor
     */
    PhysicsComponent();

    /**
     * @brief Destructor
     */
    ~PhysicsComponent() override;

    /**
     * @brief Initialize component
     * Called when component is first added to an entity
     */
    void initialize() override;

    /**
     * @brief Start component
     * Called on the first frame after initialization
     */
    void start() override;

    /**
     * @brief Update component
     * Called each frame
     * @param deltaTime Time since last frame
     */
    void update(float deltaTime) override;

    /**
     * @brief Called before component is destroyed
     * Used for cleanup and resource release
     */
    void onDestroy() override;

    /**
     * @brief Set the type of physics body
     * @param type Body type to set
     */
    void setBodyType(PhysicsBodyType type);

    /**
     * @brief Get the current body type
     * @return Current physics body type
     */
    PhysicsBodyType getBodyType() const;

    /**
     * @brief Set the mass of the body in kg
     * @param mass Mass value (0 for infinite mass/static)
     */
    void setMass(float mass);

    /**
     * @brief Get the current mass
     * @return Current mass in kg
     */
    float getMass() const;

    /**
     * @brief Set whether the body uses gravity
     * @param useGravity Whether to apply gravity to his body
     */
    void setUseGravity(bool useGravity);

    /**
     * @brief Check if body uses gravity
     * @return True if body is affected by gravity
     */
    bool getUseGravity() const;

    /**
     * @brief Set linear damping (drag) factor
     * @param damping Damping factor (0 = no damping)
     */
    void setLinearDamping(float damping);

    /**
     * @brief Get the current linear damping
     * @return Current damping factor
     */
    float getLinearDamping() const;

    /**
     * @brief Set the physics material properties
     * @param friction Friction coefficient (0-1)
     * @param restitution Bounciness (0-1)
     */
    void setMaterial(float friction, float restitution);

    /**
     * @brief Get the current friction value
     * @return Friction coefficient
     */
    float getFriction() const;

    /**
     * @brief Get the current restitution value
     * @return Restitution coefficient
     */
    float getRestitution() const;

    /**
     * @brief Set whether body can rotate
     * @param canRotate Whether body can rotate
     */
    void setCanRotate(bool canRotate);

    /**
     * @brief Check if body can rotate
     * @return True if body can rotate
     */
    bool canRotate() const;

    /**
     * @brief Apply a force to the center of mass
     * @param force Force vector to apply
     */
    void applyForce(const glm::vec3& force);

    /**
     * @brief Apply a force at a specific point
     * @param force Force vector to apply
     * @param point Point at which to apply force (world coordinates)
     */
    void applyForceAtPoint(const glm::vec3& force, const glm::vec3& point);

    /**
     * @brief Apply an impulse to the center of mass
     * @param impulse Impulse vector to apply
     */
    void applyImpulse(const glm::vec3& impulse);

    /**
     * @brief Apply a torque to the body
     * @param torque Torque vector to apply
     */
    void applyTorque(const glm::vec3& torque);

    /**
     * @brief Set the linear velocity directly
     * @param velocity New velocity vector
     */
    void setLinearVelocity(const glm::vec3& velocity);

    /**
     * @brief Get the current linear velocity
     * @return Current velocity vector
     */
    glm::vec3 getLinearVelocity() const;

    /**
     * @brief Set the angular velocity directly
     * @param angularVelocity New angular velocity vector
     */
    void setAngularVelocity(const glm::vec3& angularVelocity);

    /**
     * @brief Get the current angular velocity
     * @return Current angular velocity vector
     */
    glm::vec3 getAngularVelocity() const;

    /**
     * @brief Add a collider to this physics body
     * @param collider Collider to add
     */
    void addCollider(Collider* collider);

    /**
     * @brief Remove a collider from this physics body
     * @param collider Collider to remove
     * @return True if collider was found and removed
     */
    bool removeCollider(Collider* collider);

    /**
     * @brief Get all colliders attached to this body
     * @return vector of pointers to colliders
     */
    const std::vector<Collider*>& getColliders() const;

    /**
     * @brief Set collision filtering layer
     * @param layer Collision layer (0-31)
     */
    void setCollisionLayer(int layer);

    /**
     * @brief Get the collision layer
     * @return Current collision layer
     */
    int getCollisionLayer() const;

    /**
     * @brief Set collision mask (which layers to collide with)
     * @param mask Bitmask of collision layers
     */
    void setCollisionMask(uint32_t mask);

    /**
     * @brief Get the collision mask
     * @return Current collision mask
     */
    uint32_t getCollisionMask() const;

    /**
     * @brief Check if this body collides with a specific layer
     * @param layer Layer to check
     * @return True if body collides with the layer
     */
    bool collidesWithLayer(int layer) const;

    /**
     * @brief Lock movement along specific axes
     * @param lockX Whether to lock X-axis movement
     * @param lockY Whether to lock Y-axis movement
     * @param lockZ Whether to lock Z-axis movement
     */
    void setAxisLock(bool lockX, bool lockY, bool lockZ);

    /**
     * @brief Check if an axis is locked
     * @param axis Axis index (0=X, 1=Y, 2=Z)
     * @return True if axis is locked
     */
    bool isAxisLocked(int axis) const;

    /**
     * @brief Set a callback for collision events
     * @param callback Function to call when collision occurs
     */
    void setCollisionCallback(std::function<void(const CollisionInfo&)> callback);

    /**
     * @brief Set a callback for trigger events
     * @param enterCallback Function to call when trigger is entered
     * @param exitCallback Function to call when trigger is exited
     */
    void setTriggerCallbacks(
        std::function<void(PhysicsComponent*)> enterCallback,
        std::function<void(PhysicsComponent*)> exitCallback);

    /**
     * @brief Wake up the physics body if it was sleeping
     */
    void wakeUp();

    /**
     * @brief Check if the body is currently sleeping
     * @return True if body is sleeping
     */
    bool isSleeping() const;

    /**
     * @brief Set sleeping threshold parameters
     * @param linearThreshold Linear velocity below which body may sleep
     * @param angularThreshold Angular velocity below which body may sleep
     * @param sleepTime Time below threshold before sleeping
     */
    void setSleepThresholds(float linearThreshold, float angularThreshold, float sleepTime);

    /**
     * @brief Set whether the physics body can sleep
     * @param canSleep Whether the body can enter sleep state
     */
    void setCanSleep(bool canSleep);

    /**
     * @brief Get component type name
     * @return String representation of component type
     */
    const char* getTypeName() const override;

    /**
     * @brief Check if the body is currently grounded
     * @return True if the body is touching ground
     */
    bool isGrounded() const;

    /**
     * @brief Set continuous collision detection mode
     * @param enableCCD Whether to use continuous collision detection
     */
    void setContinuousCollisionDetection(bool enableCCD);

    /**
     * @brief Check if continuous collision detection is enabled
     * @return True if CCD is enabled
     */
    bool hasContinuousCollisionDetection() const;

    /**
     * @brief Handle a collision with another physics component
     * @param other The other physics component
     * @param collisionInfo Collision data
     * @return True if collision was handled normally
     */
    bool handleCollision(PhysicsComponent* other, const CollisionInfo& collisionInfo);

    /**
     * @brief Set the center of mass offset from the entity position
     * @param centerOfMass Offset vector from entity position
     */
    void setCenterOfMass(const glm::vec3& centerOfMass);

    /**
     * @brief Get the center of mass offset
     * @return Center of mass offset vector
     */
    glm::vec3 getCenterOfMass() const;

    /**
     * @brief Move and rotate the component instantly by bypassing physics
     * @param position New position
     * @param rotation New rotation
     */
    void moveAndRotate(const glm::vec3& position, const glm::quat& rotation);

    // Platformer-specific physics methods

    /**
     * @brief Set whether this body should use platformer-specific physics
     * @param isPlatformerPhysics Whether to use platformer physics
     */
    void setPlatformerPhysics(bool isPlatformerPhysics);

    /**
     * @brief Check if using platformer physics
     * @return True if using platformer physics
     */
    bool isPlatformerPhysics() const;

    /**
     * @brief Set platformer jump parameters
     * @param jumpHeight Maximum jump height in units
     * @param jumpTime Time to reach peak height in seconds
     */
    void setJumpParameters(float jumpHeight, float jumpTime);

    /**
     * @brief Get current jump parameters
     * @param outJumpHeight Reference to store jump height
     * @param outJumpTime Reference to store jump time
     */
    void getJumpParameters(float& outJumpHeight, float& outJumpTime) const;

    /**
     * @brief Set variable jump height
     * @param minJumpHeight Minimum jump height when button released early
     */
    void setVariableJumpHeight(float minJumpHeight);

    /**
     * @brief Execute a jump
     * @return True if jump was initiated
     */
    bool jump();

    /**
     * @brief Call when jump button is released (for variable height jumps)
     */
    void jumpReleased();

    /**
     * @brief Set coyote time (allows jump shortly after leaving platform)
     * @param coyoteTime Time in seconds
     */
    void setCoyoteTime(float coyoteTime);

    /**
     * @brief Set jump buffer time (registers early jump inputs)
     * @param bufferTime Time in seconds
     */
    void setJumpBufferTime(float bufferTime);

    /**
     * @brief Set wall slide and jump parameters
     * @param wallSlideGravityScale Gravity scale when sliding on wall
     * @param wallJumpForce Force of wall jump
     * @param wallJumpAngle Angle of wall jump in degrees
     */
    void setWallJumpParameters(float wallSlideGravityScale, float wallJumpForce, float wallJumpAngle);

    /**
     * @brief Check if body is sliding on a wall
     * @return True if wall sliding
     */
    bool isWallSliding() const;

    /**
     * @brief Attempt to perform a wall jump
     * @return True if wall jump was executed
     */
    bool wallJump();

    /**
     * @brief Check if entity can grab a ledge
     * @return True if ledge can be grabbed
     */
    bool canGrabLedge() const;

    /**
     * @brief Try to grab a ledge
     * @return True if ledge was grabbed
     */
    bool tryGrabLedge();

    /**
     * @brief Release a grabbed ledge
     */
    void releaseLedge();

    /**
     * @brief Is entity currently grabbing a ledge
     * @return True if grabbing ledge
     */
    bool isGrabbingLedge() const;

    /**
     * @brief Climb up from a grabbed ledge
     */
    void climbLedge();

    /**
     * @brief Set if entity can dash
     * @param canDash Whether entity can dash
     * @param dashForce Force of dash
     * @param dashCooldown Cooldown time between dashes
     */
    void setDashParameters(bool canDash, float dashForce, float dashCooldown);

    /**
     * @brief Execute a dash in the specified direction
     * @param direction Direction to dash
     * @return True if dash was executed
     */
    bool dash(const glm::vec3& direction);

    /**
     * @brief Check if entity is currently dashing
     * @return True if dashing
     */
    bool isDashing() const;

    /**
     * @brief Set the horizontal movement parameters
     * @param maxSpeed Maximum horizontal speed
     * @param acceleration Acceleration rate
     * @param deceleration Deceleration rate
     */
    void setMovementParameters(float maxSpeed, float acceleration, float deceleration);

    /**
     * @brief Apply horizontal movement input
     * @param direction Normalized direction vector
     * @param inputStrength Input strength (0-1)
     */
    void move(const glm::vec3& direction, float inputStrength = 1.0f);

private:
    // Physics properties
    PhysicsBodyType m_bodyType;
    float m_mass;
    bool m_useGravity;
    float m_linearDamping;
    float m_friction;
    float m_restitution;
    bool m_canRotate;
    bool m_isSleeping;
    bool m_canSleep;
    float m_linearSleepThreshold;
    float m_angularSleepThreshold;
    float m_sleepTimeThreshold;
    float m_sleepTime;
    bool m_useCCD;
    glm::vec3 m_centerOfMass;

    // Collision filtering
    int m_collisionLayer;
    uint32_t m_collisionMask;

    // Movement constraints
    bool m_lockAxisX;
    bool m_lockAxisY;
    bool m_lockAxisZ;

    // Dynamic state
    glm::vec3 m_linearVelocity;
    glm::vec3 m_angularVelocity;
    glm::vec3 m_totalForce;
    glm::vec3 m_totalTorque;
    bool m_isGrounded;
    float m_groundedCheckDistance;

    // Colliders
    std::vector<Collider*> m_colliders;

    // Callbacks
    std::function<void(const CollisionInfo&)> m_collisionCallback;
    std::function<void(PhysicsComponent*)> m_triggerEnterCallback;
    std::function<void(PhysicsComponent*)> m_triggerExitCallback;

    // Platformer physics
    bool m_isPlatformerPhysics;
    glm::vec3 m_currentMoveDirection;
    float m_jumpHeight;
    float m_jumpTime;
    float m_jumpBufferTime;
    float m_jumpBufferTimer;
    float m_jumpVelocity;
    float m_minJumpHeight;
    float m_coyoteTime;
    float m_coyoteTimer;
    bool m_isJumping;
    bool m_isWallSliding;
    float m_wallSlideGravityScale;
    float m_wallJumpForce;
    float m_wallJumpAngle;
    bool m_isGrabbingLedge;
    bool m_canDash;
    float m_dashForce;
    float m_dashCooldown;
    float m_dashTimer;
    bool m_isDashing;
    float m_maxSpeed;
    float m_acceleration;
    float m_deceleration;

    // Private helper methods
    void updatePlatformerPhysics(float deltaTime);
    void applyPlatformerMovement(float deltaTime);
    void checkGrounded();
    void checkWallSliding();
    void checkLedgeGrab();
    void calculateJumpParameters();
    void updateTimers(float deltaTime);
    void wakeUpConnectedBodies();
    void refreshColliders();
    
    // Friend class to allow direct access
    friend class PhysicsSystem;
};