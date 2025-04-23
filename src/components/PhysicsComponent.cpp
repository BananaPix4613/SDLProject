#include "PhysicsComponent.h"
#include "PhysicsSystem.h"
#include "Collider.h"
#include "Entity.h"
#include "Scene.h"
#include <algorithm>
#include <glm/glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/quaternion.hpp>

// Constructor
PhysicsComponent::PhysicsComponent() :
    m_bodyType(PhysicsBodyType::DYNAMIC),
    m_mass(1.0f),
    m_useGravity(true),
    m_linearDamping(0.01f),
    m_friction(0.5f),
    m_restitution(0.0f),
    m_canRotate(true),
    m_isSleeping(false),
    m_canSleep(true),
    m_linearSleepThreshold(0.01f),
    m_angularSleepThreshold(0.01f),
    m_sleepTimeThreshold(0.5f),
    m_sleepTime(0.0f),
    m_useCCD(false),
    m_centerOfMass(0.0f),
    m_collisionLayer(0),
    m_collisionMask(0xFFFFFFFF),
    m_lockAxisX(false),
    m_lockAxisY(false),
    m_lockAxisZ(false),
    m_linearVelocity(0.0f),
    m_angularVelocity(0.0f),
    m_totalForce(0.0f),
    m_totalTorque(0.0f),
    m_isGrounded(false),
    m_groundedCheckDistance(0.1f),
    m_isPlatformerPhysics(false),
    m_jumpHeight(2.0f),
    m_jumpTime(0.5f),
    m_jumpVelocity(0.5f)
    m_minJumpHeight(0.5f),
    m_coyoteTime(0.1f),
    m_coyoteTimer(0.0f),
    m_jumpBufferTime(0.1f),
    m_jumpBufferTimer(0.0f),
    m_isJumping(false),
    m_isWallSliding(false),
    m_wallSlideGravityScale(0.3f),
    m_wallJumpForce(5.0f),
    m_wallJumpAngle(45.0f),
    m_isGrabbingLedge(false),
    m_canDash(false),
    m_dashForce(10.0f),
    m_dashCooldown(1.0f),
    m_dashTimer(0.0f),
    m_isDashing(false),
    m_maxSpeed(5.0f),
    m_acceleration(20.0f),
    m_deceleration(10.0f)
{
    calculateJumpParameters();
}

// Destructor
PhysicsComponent::~PhysicsComponent()
{
    // Clean up colliders
    for (auto collider : m_colliders)
    {
        collider->setPhysicsComponent(nullptr);
    }
    m_colliders.clear();
}

// Lifecycle methods
void PhysicsComponent::initialize()
{
    // Register with physics system
    Scene* scene = getEntity()->getScene();
    if (scene)
    {
        PhysicsSystem* physicsSystem = scene->getSystem<PhysicsSystem>();
        if (physicsSystem)
        {
            physicsSystem->registerComponent(this);
        }
    }
}

void PhysicsComponent::start()
{
    // Initial check for grounded state
    checkGrounded();
}

void PhysicsComponent::update(float deltaTime)
{
    // Most physics is handled by PhysicsSystem
    // This is for component-specific updates

    if (m_isPlatformerPhysics)
    {
        updatePlatformerPhysics(deltaTime);
    }

    // Update timers
    updateTimers(deltaTime);
}

void PhysicsComponent::onDestroy()
{
    // Unregister from physics system
    Scene* scene = getEntity()->getScene();
    if (scene)
    {
        PhysicsSystem* physicsSystem = scene->getSystem<PhysicsSystem>();
        if (physicsSystem)
        {
            physicsSystem->unregisterComponent(this);
        }
    }

    // Clear colliders
    for (auto collider : m_colliders)
    {
        // Don't delete colliders here, the physics system owns them
        collider->setPhysicsComponent(nullptr);
    }
    m_colliders.clear();
}

// Property setters/getters
void PhysicsComponent::setBodyType(PhysicsBodyType type)
{
    if (m_bodyType == type)
        return;

    m_bodyType = type;

    // Update mass for static objects
    if (type == PhysicsBodyType::STATIC)
    {
        m_mass = 0.0f;
        m_linearVelocity = glm::vec3(0.0f);
        m_angularVelocity = glm::vec3(0.0f);
        m_totalForce = glm::vec3(0.0f);
        m_totalTorque = glm::vec3(0.0f);
    }
    else if (m_mass == 0.0f)
    {
        // If changing from static, give a default mass
        m_mass = 1.0f;
    }

    wakeUp();
}

PhysicsBodyType PhysicsComponent::getBodyType() const
{
    return m_bodyType;
}

void PhysicsComponent::setMass(float mass)
{
    // Ensure mass is not negative
    if (mass < 0.0f)
        mass = 0.0f;

    m_mass = mass;

    // If mass is zero, body becomes static
    if (mass == 0.0f && m_bodyType != PhysicsBodyType::STATIC && m_bodyType != PhysicsBodyType::TRIGGER)
    {
        m_bodyType = PhysicsBodyType::STATIC;
    }

    wakeUp();
}

float PhysicsComponent::getMass() const
{
    return m_mass;
}

void PhysicsComponent::setUseGravity(bool useGravity)
{
    if (m_useGravity == useGravity)
        return;

    m_useGravity = useGravity;
    wakeUp();
}

bool PhysicsComponent::getUseGravity() const
{
    return m_useGravity;
}

void PhysicsComponent::setLinearDamping(float damping)
{
    // Ensure damping is not negative
    if (damping < 0.0f)
        damping = 0.0f;

    m_linearDamping = damping;
}

float PhysicsComponent::getLinearDamping() const
{
    return m_linearDamping;
}

void PhysicsComponent::setMaterial(float friction, float restitution)
{
    // Clamp values to valid ranges
    m_friction = glm::clamp(friction, 0.0f, 1.0f);
    m_restitution = glm::clamp(restitution, 0.0f, 1.0f);

    // Update all colliders with the same material
    for (auto collider : m_colliders)
    {
        collider->setMaterial(m_friction, m_restitution);
    }
}

float PhysicsComponent::getFriction() const
{
    return m_friction;
}

float PhysicsComponent::getRestitution() const
{
    return m_restitution;
}

void PhysicsComponent::setCanRotate(bool canRotate)
{
    m_canRotate = canRotate;

    // If rotation is disabled, clear angular velocity
    if (!canRotate)
    {
        m_angularVelocity = glm::vec3(0.0f);
        m_totalTorque = glm::vec3(0.0f);
    }
}

bool PhysicsComponent::canRotate() const
{
    return m_canRotate;
}

// Force application methods
void PhysicsComponent::applyForce(const glm::vec3& force)
{
    // Only apply forces to dynamic bodies
    if (m_bodyType != PhysicsBodyType::DYNAMIC)
        return;

    m_totalForce += force;
    wakeUp();
}

void PhysicsComponent::applyForceAtPoint(const glm::vec3& force, const glm::vec3& point)
{
    // Only apply forces to dynamic bodies
    if (m_bodyType != PhysicsBodyType::DYNAMIC)
        return;

    // Get world position including center of mass offset
    glm::vec3 worldPos = getEntity()->getWorldPosition() + m_centerOfMass;

    // Apply linear force
    m_totalForce += force;

    // Apply torque if rotation is enabled
    if (m_canRotate)
    {
        // Calculate torque as cross product of offset and force
        glm::vec3 offset = point - worldPos;
        m_totalTorque += glm::cross(offset, force);
    }

    wakeUp();
}

void PhysicsComponent::applyImpulse(const glm::vec3& impulse)
{
    // Only apply impulses to dynamic bodies
    if (m_bodyType != PhysicsBodyType::DYNAMIC || m_mass == 0.0f)
        return;

    // F = ma, impulse = mv, thus v = impulse/m
    m_linearVelocity += impulse / m_mass;
    wakeUp();
}

void PhysicsComponent::applyTorque(const glm::vec3& torque)
{
    // Only apply torque to dynamic bodies with rotation enabled
    if (m_bodyType != PhysicsBodyType::DYNAMIC || !m_canRotate)
        return;

    m_totalTorque += torque;
    wakeUp();
}

void PhysicsComponent::setLinearVelocity(const glm::vec3& velocity)
{
    // Static bodies can't have velocity
    if (m_bodyType == PhysicsBodyType::STATIC)
        return;

    glm::vec3 newVelocity = velocity;

    // Apply axis locks
    if (m_lockAxisX) newVelocity.x = 0.0f;
    if (m_lockAxisY) newVelocity.y = 0.0f;
    if (m_lockAxisZ) newVelocity.z = 0.0f;

    m_linearVelocity = newVelocity;
    wakeUp();
}

glm::vec3 PhysicsComponent::getLinearVelocity() const
{
    return m_linearVelocity;
}

void PhysicsComponent::setAngularVelocity(const glm::vec3& angularVelocity)
{
    // Static bodies or bodies without rotation can't have angular velocity
    if (m_bodyType == PhysicsBodyType::STATIC || !m_canRotate)
        return;

    m_angularVelocity = angularVelocity;
    wakeUp();
}

glm::vec3 PhysicsComponent::getAngularVelocity() const
{
    return m_angularVelocity;
}

// Collider management
void PhysicsComponent::addCollider(Collider* collider)
{
    if (!collider)
        return;

    // Check if collider already exists
    auto it = std::find(m_colliders.begin(), m_colliders.end(), collider);
    if (it != m_colliders.end())
        return;

    // Set this component as the owner
    collider->setPhysicsComponent(this);

    // Add to collider list
    m_colliders.push_back(collider);

    // Set material properties from component
    collider->setMaterial(m_friction, m_restitution);

    wakeUp();
}

bool PhysicsComponent::removeCollider(Collider* collider)
{
    if (!collider)
        return false;

    // Find and remove collider
    auto it = std::find(m_colliders.begin(), m_colliders.end(), collider);
    if (it != m_colliders.end())
    {
        // Unlink from this component
        (*it)->setPhysicsComponent(nullptr);

        // Remove from list
        m_colliders.erase(it);
        wakeUp();
        return true;
    }

    return false;
}

const std::vector<Collider*>& PhysicsComponent::getColliders() const
{
    return m_colliders;
}

// Collision filtering
void PhysicsComponent::setCollisionLayer(int layer)
{
    // Ensure layer is in valid range (0-31)
    layer = glm::clamp(layer, 0, 31);
    m_collisionLayer = layer;
}

int PhysicsComponent::getCollisionLayer() const
{
    return m_collisionLayer;
}

void PhysicsComponent::setCollisionMask(uint32_t mask)
{
    m_collisionMask = mask;
}

uint32_t PhysicsComponent::getCollisionMask() const
{
    return m_collisionMask;
}

bool PhysicsComponent::collidesWithLayer(int layer) const
{
    // Ensure layer is in valid range
    if (layer < 0 || layer > 31)
        return false;

    // Check if the bit for this layer is set in the mask
    return (m_collisionMask & (1 << layer)) != 0;
}

// Movement constraints
void PhysicsComponent::setAxisLock(bool lockX, bool lockY, bool lockZ)
{
    m_lockAxisX = lockX;
    m_lockAxisY = lockY;
    m_lockAxisZ = lockZ;

    // Apply locks to current velocity
    if (lockX) m_linearVelocity.x = 0.0f;
    if (lockY) m_linearVelocity.y = 0.0f;
    if (lockZ) m_linearVelocity.z = 0.0f;
}

bool PhysicsComponent::isAxisLocked(int axis) const
{
    switch (axis)
    {
        case 0: return m_lockAxisX;
        case 1: return m_lockAxisY;
        case 2: return m_lockAxisZ;
        default: return false;
    }
}

// Collision callbacks
void PhysicsComponent::setCollisionCallback(std::function<void(const CollisionInfo&)> callback)
{
    m_collisionCallback = callback;
}

void PhysicsComponent::setTriggerCallbacks(
    std::function<void(PhysicsComponent*)> enterCallback,
    std::function<void(PhysicsComponent*)> exitCallback)
{
    m_triggerEnterCallback = enterCallback;
    m_triggerExitCallback = exitCallback;
}

// Sleep management
void PhysicsComponent::wakeUp()
{
    if (!m_isSleeping)
        return;

    m_isSleeping = false;
    m_sleepTime = 0.0f;

    // Wake up connected bodies
    wakeUpConnectedBodies();
}

bool PhysicsComponent::isSleeping() const
{
    return m_isSleeping;
}

void PhysicsComponent::setSleepThresholds(float linearThreshold, float angularThreshold, float sleepTime)
{
    m_linearSleepThreshold = linearThreshold;
    m_angularSleepThreshold = angularThreshold;
    m_sleepTimeThreshold = sleepTime;
}

void PhysicsComponent::setCanSleep(bool canSleep)
{
    m_canSleep = canSleep;

    // Wake up if sleep is disabled
    if (!canSleep && m_isSleeping)
    {
        wakeUp();
    }
}

// Component type information
const char* PhysicsComponent::getTypeName() const
{
    return "PhysicsComponent";
}

// Grounding checks
bool PhysicsComponent::isGrounded() const
{
    return m_isGrounded;
}

// Continuous collision detection
void PhysicsComponent::setContinuousCollisionDetection(bool enableCCD)
{
    m_useCCD = enableCCD;
}

bool PhysicsComponent::hasContinuousCollisionDetection() const
{
    return m_useCCD;
}

// Collision handling
bool PhysicsComponent::handleCollision(PhysicsComponent* other, const CollisionInfo& collisionInfo)
{
    // Call collision callback if registered
    if (m_collisionCallback)
    {
        m_collisionCallback(collisionInfo);
    }

    // Default collision response is to accept the collision
    return true;
}

// Center of mass
void PhysicsComponent::setCenterOfMass(const glm::vec3& centerOfMass)
{
    m_centerOfMass = centerOfMass;
}

glm::vec3 PhysicsComponent::getCenterOfMass() const
{
    return m_centerOfMass;
}

// Direct movement
void PhysicsComponent::moveAndRotate(const glm::vec3& position, const glm::quat& rotation)
{
    // Update entity position and rotation directly
    Entity* entity = getEntity();
    if (entity)
    {
        entity->setPosition(position);
        entity->setRotation(rotation);
    }

    // Wake up for physics processing
    wakeUp();
}

// Platformer physics methods
void PhysicsComponent::setPlatformerPhysics(bool isPlatformerPhysics)
{
    m_isPlatformerPhysics = isPlatformerPhysics;

    if (isPlatformerPhysics)
    {
        // Set default platformer physics parameters
        calculateJumpParameters();
    }
}

bool PhysicsComponent::isPlatformerPhysics() const
{
    return m_isPlatformerPhysics;
}

void PhysicsComponent::setJumpParameters(float jumpHeight, float jumpTime)
{
    m_jumpHeight = jumpHeight;
    m_jumpTime = jumpTime;
    calculateJumpParameters();
}

void PhysicsComponent::getJumpParameters(float& outJumpHeight, float& outJumpTime) const
{
    outJumpHeight = m_jumpHeight;
    outJumpTime = m_jumpTime;
}

void PhysicsComponent::setVariableJumpHeight(float minJumpHeight)
{
    m_minJumpHeight = minJumpHeight;
}

bool PhysicsComponent::jump()
{
    // Can only jump if platformer physics is enabled
    if (!m_isPlatformerPhysics)
        return false;

    // Check if can jump (grounded or within coyote time, or wall sliding)
    if ((m_isGrounded || m_coyoteTimer > 0.0f || m_isWallSliding) && !m_isGrabbingLedge)
    {
        // Set vertical velocity to jump velocity
        m_linearVelocity.y = m_jumpVelocity;

        // Reset coyote timer
        m_coyoteTimer = 0.0f;

        // Reset jump buffer
        m_jumpBufferTimer = 0.0f;

        // Set jumping state
        m_isJumping = true;

        // No longer grounded
        m_isGrounded = false;

        wakeUp();
        return true;
    }
    else if (!m_isGrounded && !m_isJumping)
    {
        // Buffer the jump if in the air and not already jumping
        m_jumpBufferTimer = m_jumpBufferTime;
    }

    return false;
}

void PhysicsComponent::jumpReleased()
{
    // For variable height jumps, limit upward velocity when button is released
    if (m_isJumping && m_linearVelocity.y > 0.0f)
    {
        // Calculate minimum jump velocity based on minimum height
        Scene* scene = getEntity()->getScene();
        if (scene)
        {
            PhysicsSystem* physicsSystem = scene->getSystem<PhysicsSystem>();
            if (physicsSystem)
            {
                float gravity = physicsSystem->getGravity().y;
                float minJumpVelocity = sqrtf(2.0f * fabsf(gravity) * m_minJumpHeight);

                // Clamp upward velocity to minimum jump velocity
                if (m_linearVelocity.y > minJumpVelocity)
                {
                    m_linearVelocity.y = minJumpVelocity;
                }
            }
        }
    }
}

void PhysicsComponent::setCoyoteTime(float coyoteTime)
{
    m_coyoteTime = coyoteTime;
}

void PhysicsComponent::setJumpBufferTime(float bufferTime)
{
    m_jumpBufferTime = bufferTime;
}

void PhysicsComponent::setWallJumpParameters(float wallSlideGravityScale, float wallJumpForce, float wallJumpAngle)
{
    m_wallSlideGravityScale = wallSlideGravityScale;
    m_wallJumpForce = wallJumpForce;
    m_wallJumpAngle = wallJumpAngle;
}

bool PhysicsComponent::isWallSliding() const
{
    return m_isWallSliding;
}

bool PhysicsComponent::wallJump()
{
    // Can only wall jump if platformer physics is enabled and wall sliding
    if (!m_isPlatformerPhysics || !m_isWallSliding)
        return false;

    // Calculate wall jump direction (away from wall at an angle)
    Entity* entity = getEntity();
    if (!entity)
        return false;

    // Assuming the entity forward direction is facing the wall
    glm::vec3 forward = entity->getForward();
    forward.y = 0.0f; // Keep it horizontal

    if (glm::length(forward) < 0.01f)
    {
        // Fallback if no clear forward direction
        forward = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    else
    {
        forward = glm::normalize(forward);
    }

    // Calculate wall jump direction (opposite of forward, at an angle)
    float angle = glm::radians(m_wallJumpAngle);
    glm::vec3 jumpDir = -forward; // Away from wall

    // Set velocity for wall jump
    m_linearVelocity = jumpDir * m_wallJumpForce;
    m_linearVelocity.y = m_jumpVelocity; // Use regular jump velocity for vertical component

    // No longer wall sliding or grounded
    m_isWallSliding = false;
    m_isGrounded = false;

    // Set jumping state
    m_isJumping = true;

    wakeUp();
    return true;
}

bool PhysicsComponent::canGrabLedge() const
{
    // Logic to check if there's a ledge nearby to grab
    // This would typically use raycasts to detect ledges

    // For demonstration, we'll return a simple check
    return !m_isGrounded && !m_isGrabbingLedge;
}

bool PhysicsComponent::tryGrabLedge()
{
    // Can only grab ledges if platformer physics is enabled
    if (!m_isPlatformerPhysics)
        return false;

    if (canGrabLedge())
    {
        m_isGrabbingLedge = true;

        // Stop all movement when grabbing ledge
        m_linearVelocity = glm::vec3(0.0f);
        m_angularVelocity = glm::vec3(0.0f);

        // No longer jumping or wall sliding
        m_isJumping = false;
        m_isWallSliding = false;

        wakeUp();
        return true;
    }

    return false;
}

void PhysicsComponent::releaseLedge()
{
    if (m_isGrabbingLedge)
    {
        m_isGrabbingLedge = false;
        wakeUp();
    }
}

bool PhysicsComponent::isGrabbingLedge() const
{
    return m_isGrabbingLedge;
}

void PhysicsComponent::climbLedge()
{
    if (m_isGrabbingLedge)
    {
        // Move character up and forward to climb the ledge
        Entity* entity = getEntity();
        if (entity)
        {
            glm::vec3 position = entity->getPosition();
            glm::vec3 forward = entity->getForward();
            forward.y = 0.0f;

            if (glm::length(forward) > 0.01f)
            {
                forward = glm::normalize(forward);

                // Move up and forward
                position.y += 1.0f; // Move up (adjust based on character height)
                position += forward * 0.5f; // Move forward slightly

                entity->setPosition(position);

                // No longer grabbing ledge
                m_isGrabbingLedge = false;

                // Set as grounded
                m_isGrounded = true;

                wakeUp();
            }
        }
    }
}

void PhysicsComponent::setDashParameters(bool canDash, float dashForce, float dashCooldown)
{
    m_canDash = canDash;
    m_dashForce = dashForce;
    m_dashCooldown = dashCooldown;
}

bool PhysicsComponent::dash(const glm::vec3& direction)
{
    // Can only dash if platformer physics is enabled and dash is available
    if (!m_isPlatformerPhysics || !m_canDash || m_dashTimer > 0.0f)
        return false;

    // Normalize direction
    glm::vec3 dashDir = direction;
    float length = glm::length(dashDir);

    if (length < 0.01f)
    {
        // If no direction specified, use entity forward direction
        Entity* entity = getEntity();
        if (entity)
        {
            dashDir = entity->getForward();
            dashDir.y = 0.0f; // Keep dash horizontal

            length = glm::length(dashDir);
            if (length < 0.01f)
            {
                // Fallback if no clear direction
                dashDir = glm::vec3(1.0f, 0.0f, 0.0f);
            }
            else
            {
                dashDir /= length;
            }
        }
        else
        {
            return false;
        }
    }
    else
    {
        dashDir /= length;
    }

    // Set dash velocity
    m_linearVelocity = dashDir * m_dashForce;

    // Start dash timer
    m_dashTimer = 0.2f; // Default dash duration
    m_isDashing = true;

    wakeUp();
    return true;
}

bool PhysicsComponent::isDashing() const
{
    return m_isDashing;
}

void PhysicsComponent::setMovementParameters(float maxSpeed, float acceleration, float deceleration)
{
    m_maxSpeed = maxSpeed;
    m_acceleration = acceleration;
    m_deceleration = deceleration;
}

void PhysicsComponent::move(const glm::vec3& direction, float inputStrength)
{
    // Can only use movement controller if platformer physics is enabled
    if (!m_isPlatformerPhysics)
        return;

    // Clamp input strength to valid range
    inputStrength = glm::clamp(inputStrength, 0.0f, 1.0f);

    // Normalize direction if not zero
    glm::vec3 moveDir = direction;
    float length = glm::length(moveDir);

    if (length > 0.01f)
    {
        moveDir /= length;

        // In an isometric game, we might want to handle direction differently
        // This is a simplified implementation

        // For platformers, usually keep y component 0 (horizontal movement only)
        moveDir.y = 0.0f;

        // Re-normalize if needed
        length = glm::length(moveDir);
        if (length > 0.01f)
        {
            moveDir /= length;
        }
        else
        {
            moveDir = glm::vec3(0.0f);
            inputStrength = 0.0f;
        }
    }
    else
    {
        moveDir = glm::vec3(0.0f);
        inputStrength = 0.0f;
    }

    // Store movement intent for physics update
    m_currentMoveDirection = moveDir;

    // Apply horizontal force in next physics update
    wakeUp();
}

// Private helper methods
void PhysicsComponent::updatePlatformerPhysics(float deltaTime)
{
    // Only apply to dynamic bodies with platformer physics enabled
    if (m_bodyType != PhysicsBodyType::DYNAMIC || !m_isPlatformerPhysics)
        return;

    // Update grounded state
    bool wasGrounded = m_isGrounded;
    checkGrounded();

    // Start coyote time when falling off platform
    if (wasGrounded && !m_isGrounded)
    {
        m_coyoteTimer = m_coyoteTime;
    }

    // Update wall sliding state
    checkWallSliding();

    // Check for ledge grab
    checkLedgeGrab();

    // Apply horizontal movement
    applyPlatformerMovement(deltaTime);

    // Process buffered jump
    if (m_jumpBufferTimer > 0.0f && m_isGrounded)
    {
        jump();
    }

    // Reset jumping state if grounded
    if (m_isGrounded && m_linearVelocity.y <= 0.0f)
    {
        m_isJumping = false;
    }

    // Update dash state
    if (m_isDashing)
    {
        if (m_dashTimer <= 0.0f)
        {
            m_isDashing = false;

            // Reset velocity after dash
            m_linearVelocity *= 0.5f;
        }
    }
}

void PhysicsComponent::applyPlatformerMovement(float deltaTime)
{
    // Skip if grabbing ledge or dashing
    if (m_isGrabbingLedge || m_isDashing)
        return;

    // Get current horizontal velocity
    glm::vec3 hVelocity = m_linearVelocity;
    hVelocity.y = 0.0f;

    // Target velocity based on input
    glm::vec3 targetVelocity = m_currentMoveDirection * m_maxSpeed;

    // Determine acceleration rate based on grounded state
    float accelRate = m_isGrounded ? m_acceleration : (m_acceleration * 0.5f);
    float decelRate = m_isGrounded ? m_deceleration : (m_deceleration * 0.5f);

    // If moving toward target velocity
    if (glm::length(m_currentMoveDirection) > 0.01f)
    {
        // Apply acceleration toward target velocity
        glm::vec3 velocityChange = targetVelocity - hVelocity;
        float changeAmount = accelRate * deltaTime;

        // Limit change amount to avoid overshooting
        float changeNeeded = glm::length(velocityChange);
        if (changeAmount > changeNeeded)
        {
            changeAmount = changeNeeded;
        }

        if (changeNeeded > 0.01f)
        {
            velocityChange = glm::normalize(velocityChange) * changeAmount;
            hVelocity += velocityChange;
        }
    }
    else if (glm::length(hVelocity) > 0.01f)
    {
        // No input, apply deceleration
        float currentSpeed = glm::length(hVelocity);
        float reduction = decelRate * deltaTime;

        if (reduction > currentSpeed)
        {
            hVelocity = glm::vec3(0.0f);
        }
        else
        {
            hVelocity -= glm::normalize(hVelocity) * reduction;
        }
    }

    // Apply horizontal velocity
    m_linearVelocity.x = hVelocity.x;
    m_linearVelocity.z = hVelocity.z;
}

void PhysicsComponent::checkGrounded()
{
    // Use a simple ray cast to check if grounded
    // In the actual implementation, this would use proper collision detection

    // For demonstration purposes, we'll set a simple check
    // This should be replaced with proper ground detection using raycasts
    bool groundCheck = false;

    if (m_colliders.size() > 0)
    {
        Entity* entity = getEntity();
        if (entity)
        {
            glm::vec3 position = entity->getWorldPosition();

            // Simple ground check - raycasting would be used in real implementation
            Ray ray;
            ray.origin = position;
            ray.direction = glm::vec3(0.0f, -1.0f, 0.0f);
            ray.maxDistance = m_groundedCheckDistance;

            Scene* scene = entity->getScene();
            if (scene)
            {
                PhysicsSystem* physicsSystem = scene->getSystem<PhysicsSystem>();
                if (physicsSystem)
                {
                    RaycastHit hitInfo;
                    groundCheck = physicsSystem->raycast(ray, hitInfo) && hitInfo.hitComponent != this;
                }
            }
        }
    }

    m_isGrounded = groundCheck;
}

void PhysicsComponent::checkWallSliding()
{
    // Skip if grounded or not moving
    if (m_isGrounded || glm::length(m_linearVelocity) < 0.01f)
    {
        m_isWallSliding = false;
        return;
    }

    // For demonstration purposes, we'll use a simple check
    // This should be replaced with proper wall detection using raycasts
    bool wallCheck = false;

    if (m_colliders.size() > 0)
    {
        Entity* entity = getEntity();
        if (entity)
        {
            glm::vec3 position = entity->getWorldPosition();
            glm::vec3 forward = entity->getForward();
            forward.y = 0.0f;

            if (glm::length(forward) > 0.01f)
            {
                forward = glm::normalize(forward);

                // Simple wall check - raycasting would be used in real implementation
                Ray ray;
                ray.origin = position;
                ray.direction = forward;
                ray.maxDistance = 0.5f; // Check a short distance in front

                Scene* scene = entity->getScene();
                if (scene)
                {
                    PhysicsSystem* physicsSystem = scene->getSystem<PhysicsSystem>();
                    if (physicsSystem)
                    {
                        RaycastHit hitInfo;
                        wallCheck = physicsSystem->raycast(ray, hitInfo) && hitInfo.hitComponent != this;
                    }
                }
            }
        }
    }

    m_isWallSliding = wallCheck && m_linearVelocity.y < 0.0f;
}

void PhysicsComponent::checkLedgeGrab()
{
    // Skip if already grabbing ledge or grounded
    if (m_isGrabbingLedge || m_isGrounded)
        return;

    // For demonstration purposes, we'll use a simple check
    // This would be replaced with proper ledge detection using raycasts

    // In a real implementation, you would cast rays at different heights
    // to detect a ledge (solid at top, empty below)
}

void PhysicsComponent::calculateJumpParameters()
{
    // Calculate jump velocity from height and time
    // Jump velocity formula: v = sqrt(2 * g * h)
    // where g is gravity and h is jump height

    Scene* scene = getEntity() ? getEntity()->getScene() : nullptr;
    if (scene)
    {
        PhysicsSystem* physicsSystem = scene->getSystem<PhysicsSystem>();
        if (physicsSystem)
        {
            float gravity = physicsSystem->getGravity().y;
            m_jumpVelocity = sqrtf(2.0f * fabsf(gravity) * m_jumpHeight);
        }
        else
        {
            // Default gravity if physics system is not available
            m_jumpVelocity = sqrtf(2.0f * 9.81f * m_jumpHeight);
        }
    }
    else
    {
        // Default gravity if scene is not available
        m_jumpVelocity = sqrtf(2.0f * 9.81f * m_jumpHeight);
    }
}

void PhysicsComponent::updateTimers(float deltaTime)
{
    // Update coyote time
    if (m_coyoteTimer > 0.0f)
    {
        m_coyoteTimer -= deltaTime;
        if (m_coyoteTimer < 0.0f)
        {
            m_coyoteTimer = 0.0f;
        }
    }

    // Update jump buffer timer
    if (m_jumpBufferTimer > 0.0f)
    {
        m_jumpBufferTimer -= deltaTime;
        if (m_jumpBufferTimer < 0.0f)
        {
            m_jumpBufferTimer = 0.0f;
        }
    }

    // Update dash timer
    if (m_dashTimer > 0.0f)
    {
        m_dashTimer -= deltaTime;
        if (m_dashTimer < 0.0f)
        {
            m_dashTimer = 0.0f;
        }
    }

    // Sleep timer for auto-sleep
    if (m_canSleep && !m_isSleeping)
    {
        bool canSleep = true;

        // Check linear velocity
        if (glm::length(m_linearVelocity) > m_linearSleepThreshold)
        {
            canSleep = false;
        }

        // Check angular velocity if rotation is enabled
        if (m_canRotate && glm::length(m_angularVelocity) > m_angularSleepThreshold)
        {
            canSleep = false;
        }

        // Update sleep timer
        if (canSleep)
        {
            m_sleepTime += deltaTime;
            if (m_sleepTime >= m_sleepTimeThreshold)
            {
                m_isSleeping = true;
            }
        }
        else
        {
            m_sleepTime = 0.0f;
        }
    }
}

void PhysicsComponent::wakeUpConnectedBodies()
{
    // Wake up connected bodies
    // In a real implementation, we would have a list of bodies in contact with this one
    // For demonstration, we'll just update any colliders in contact
    // This would be handled by the physics system in practice
}

void PhysicsComponent::refreshColliders()
{
    // Update any cached collider information
    // This is called when the entity transform changes

    // For each collider, update its transform
    for (auto collider : m_colliders)
    {
        // In a real implementation, we would update cached bounds, etc.
    }
}