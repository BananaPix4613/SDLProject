#include "PhysicsSystem.h"
#include "components/PhysicsComponent.h"
#include "Entity.h"
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

CharacterController::CharacterController(PhysicsComponent* physicsComponent) :
    m_physicsComponent(physicsComponent),
    m_maxSpeed(5.0f),
    m_acceleration(20.0f),
    m_deceleration(10.0f),
    m_useAirControl(true),
    m_airControlFactor(0.5f),
    m_jumpHeight(2.0f),
    m_jumpTime(0.5f),
    m_variableJumpHeight(true),
    m_coyoteTime(0.1f),
    m_jumpBufferTime(0.1f),
    m_dashForce(10.0f),
    m_dashDuration(0.2f),
    m_dashCooldown(1.0f),
    m_dashTimer(0.0f),
    m_dashCooldownTimer(0.0f),
    m_dashing(false),
    m_wallJumpEnabled(true),
    m_wallSlideGravityScale(0.3f),
    m_wallJumpForce(5.0f),
    m_ledgeGrabEnabled(true),
    m_wasGrounded(false),
    m_currentMoveDirection(0.0f),
    m_currentMoveStrength(0.0f)
{
    // Configure physics component if provided
    if (m_physicsComponent)
    {
        m_physicsComponent->setPlatformerPhysics(true);
        m_physicsComponent->setJumpParameters(m_jumpHeight, m_jumpTime);
        m_physicsComponent->setVariableJumpHeight(m_jumpHeight * 0.5f);
        m_physicsComponent->setCoyoteTime(m_coyoteTime);
        m_physicsComponent->setJumpBufferTime(m_jumpBufferTime);
        m_physicsComponent->setMovementParameters(m_maxSpeed, m_acceleration, m_deceleration);
        m_physicsComponent->setWallJumpParameters(m_wallSlideGravityScale, m_wallJumpForce, 45.0f);
        m_physicsComponent->setDashParameters(true, m_dashForce, m_dashCooldown);
    }
}

CharacterController::~CharacterController()
{
    // Nothing to clean up
}

void CharacterController::setPhysicsComponent(PhysicsComponent* component)
{
    m_physicsComponent = component;

    // Configure physics component
    if (m_physicsComponent)
    {
        m_physicsComponent->setPlatformerPhysics(true);
        m_physicsComponent->setJumpParameters(m_jumpHeight, m_jumpTime);
        m_physicsComponent->setVariableJumpHeight(m_jumpHeight * 0.5f);
        m_physicsComponent->setCoyoteTime(m_coyoteTime);
        m_physicsComponent->setJumpBufferTime(m_jumpBufferTime);
        m_physicsComponent->setMovementParameters(m_maxSpeed, m_acceleration, m_deceleration);
        m_physicsComponent->setWallJumpParameters(m_wallSlideGravityScale, m_wallJumpForce, 45.0f);
        m_physicsComponent->setDashParameters(true, m_dashForce, m_dashCooldown);
    }
}

PhysicsComponent* CharacterController::getPhysicsComponent() const
{
    return m_physicsComponent;
}

void CharacterController::move(const glm::vec3& direction, float strength)
{
    if (!m_physicsComponent)
        return;

    // Store movement intent
    m_currentMoveDirection = direction;
    m_currentMoveStrength = strength;

    // Apply movement to physics component
    m_physicsComponent->move(direction, strength);
}

bool CharacterController::jump()
{
    if (!m_physicsComponent)
        return false;

    return m_physicsComponent->jump();
}

void CharacterController::jumpReleased()
{
    if (!m_physicsComponent)
        return;

    m_physicsComponent->jumpReleased();
}

bool CharacterController::dash(const glm::vec3& direction)
{
    if (!m_physicsComponent)
        return false;

    // Check if dash is available
    if (m_dashCooldownTimer > 0.0f)
        return false;

    // Execute dash
    bool dashExecuted = m_physicsComponent->dash(direction);

    if (dashExecuted)
    {
        m_dashing = true;
        m_dashTimer = m_dashDuration;
        // Cooldown will start after dash ends
    }

    return dashExecuted;
}

void CharacterController::setMovementParameters(float maxSpeed, float acceleration, float deceleration)
{
    m_maxSpeed = maxSpeed;
    m_acceleration = acceleration;
    m_deceleration = deceleration;

    if (m_physicsComponent)
    {
        m_physicsComponent->setMovementParameters(maxSpeed, acceleration, deceleration);
    }
}

void CharacterController::setJumpParameters(float jumpHeight, float jumpTime, bool variableHeight)
{
    m_jumpHeight = jumpHeight;
    m_jumpTime = jumpTime;
    m_variableJumpHeight = variableHeight;

    if (m_physicsComponent)
    {
        m_physicsComponent->setJumpParameters(jumpHeight, jumpTime);

        if (variableHeight)
        {
            m_physicsComponent->setVariableJumpHeight(jumpHeight * 0.5f);
        }
    }
}

void CharacterController::setDashParameters(float dashForce, float dashDuration, float dashCooldown)
{
    m_dashForce = dashForce;
    m_dashDuration = dashDuration;
    m_dashCooldown = dashCooldown;

    if (m_physicsComponent)
    {
        m_physicsComponent->setDashParameters(true, dashForce, dashCooldown);
    }
}

void CharacterController::setWallJumpParameters(bool enabled, float slideGravityScale, float jumpForce)
{
    m_wallJumpEnabled = enabled;
    m_wallSlideGravityScale = slideGravityScale;
    m_wallJumpForce = jumpForce;

    if (m_physicsComponent)
    {
        m_physicsComponent->setWallJumpParameters(slideGravityScale, jumpForce, 45.0f);
    }
}

void CharacterController::setLedgeGrabParameters(bool enabled)
{
    m_ledgeGrabEnabled = enabled;
}

bool CharacterController::isGrounded() const
{
    return m_physicsComponent ? m_physicsComponent->isGrounded() : false;
}

bool CharacterController::isWallSliding() const
{
    return m_physicsComponent ? m_physicsComponent->isWallSliding() : false;
}

bool CharacterController::isDashing() const
{
    return m_dashing;
}

bool CharacterController::isGrabbingLedge() const
{
    return m_physicsComponent ? m_physicsComponent->isGrabbingLedge() : false;
}

void CharacterController::climbLedge()
{
    if (m_physicsComponent)
    {
        m_physicsComponent->climbLedge();
    }
}

glm::vec3 CharacterController::getVelocity() const
{
    return m_physicsComponent ? m_physicsComponent->getLinearVelocity() : glm::vec3(0.0f);
}

void CharacterController::setAirControl(bool useAirControl, float airControlFactor)
{
    m_useAirControl = useAirControl;
    m_airControlFactor = airControlFactor;
}

void CharacterController::update(float deltaTime)
{
    if (!m_physicsComponent)
        return;

    // Track grounded state changes
    bool isGrounded = m_physicsComponent->isGrounded();
    bool groundedChanged = isGrounded != m_wasGrounded;

    if (groundedChanged)
    {
        // Just landed
        if (isGrounded)
        {
            // Reset dash cooldown on landing
            m_dashCooldownTimer = 0.0f;
        }
    }

    m_wasGrounded = isGrounded;

    // Update dash state
    updateDashState(deltaTime);

    // Check ledge grab
    if (m_ledgeGrabEnabled)
    {
        checkLedgeGrab();
    }

    // Air control adjustment
    if (!isGrounded && m_useAirControl && !m_dashing)
    {
        // Apply reduced movement control when in air
        if (glm::length(m_currentMoveDirection) > 0.001f)
        {
            m_physicsComponent->move(m_currentMoveDirection, m_currentMoveStrength * m_airControlFactor);
        }
    }
}

void CharacterController::checkLedgeGrab()
{
    if (!m_physicsComponent)
        return;

    // Ledge grab logic would be implemented here
    // This requires raycasting to detect ledges

    // For demonstration purposes:
    if (m_physicsComponent->canGrabLedge())
    {
        m_physicsComponent->tryGrabLedge();
    }
}

void CharacterController::updateDashState(float deltaTime)
{
    // Update dash timer
    if (m_dashTimer > 0.0f)
    {
        m_dashTimer -= deltaTime;

        if (m_dashTimer <= 0.0f)
        {
            // Dash ended
            m_dashTimer = 0.0f;
            m_dashing = false;

            // Start cooldown
            m_dashCooldownTimer = m_dashCooldown;
        }
    }

    // Update dash cooldown
    if (m_dashCooldownTimer > 0.0f)
    {
        m_dashCooldownTimer -= deltaTime;

        if (m_dashCooldownTimer < 0.0f)
        {
            m_dashCooldownTimer = 0.0f;
        }
    }
}