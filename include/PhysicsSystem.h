#pragma once

#include "glm/glm.hpp"
#include <gtc/quaternion.hpp>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include "Component.h"
#include "Entity.h"

// Forward declaration
class PhysicsComponent;
class Collider;
class BoxCollider;
class SphereCollider;
class CapsuleCollider;
class VoxelGridCollider;
class Scene;
class CubeGrid;
template <typename T> class Grid;

/**
 * @struct Ray
 * @brief Represents a ray for raycasting
 */
struct Ray
{
    glm::vec3 origin;    // Starting point
    glm::vec3 direction; // Normalized direction
    float maxDistance;   // Maximum distance to check

    Ray() : origin(0.0f), direction(0.0f, 0.0f, 1.0f), maxDistance(FLT_MAX)
    {
    }
    Ray(const glm::vec3& o, const glm::vec3& dir, float maxDist = FLT_MAX) :
        origin(o), direction(glm::normalize(dir)), maxDistance(maxDist)
    {
    }
};

/**
 * @struct RaycastHit
 * @brief Contains information about a raycast hit
 */
struct RaycastHit
{
    PhysicsComponent* hitComponent; // Component that was hit
    Collider* hitCollider;          // Collider that was hit
    glm::vec3 point;                // Hit point in world space
    glm::vec3 normal;               // Surface normal at hit point
    float distance;                 // Distance from ray origin to hit point
    bool hasHit;                    // Whether anything was hit

    RaycastHit() :
        hitComponent(nullptr), hitCollider(nullptr), point(0.0f),
        normal(0.0f, 1.0f, 0.0f), distance(0.0f), hasHit(false)
    {
    }
};

/**
 * @struct CollisionInfo
 * @brief Contains information about a collision between two objects
 */
struct CollisionInfo
{
    PhysicsComponent* componentA;   // First component
    PhysicsComponent* componentB;   // Second component
    Collider* colliderA;            // First collider
    Collider* colliderB;            // Second collider
    glm::vec3 contactPoint;         // Point of contact in world space
    glm::vec3 normal;               // Contact normal pointing from A to B
    float penetrationDepth;         // Penetration depth
    bool isTrigger;                 // Whether this is a trigger collision

    CollisionInfo() :
        componentA(nullptr), componentB(nullptr), colliderA(nullptr),
        colliderB(nullptr), contactPoint(0.0f), normal(0.0f, 1.0f, 0.0f),
        penetrationDepth(0.0f), isTrigger(false)
    {
    }
};

/**
 * @class PhysicsSystem
 * @brief Central system for physics simulation and collision detection
 * 
 * PhysicsSystem manages all physics components, handles collision detection,
 * and simulates physics behavior. It integrates with the voxel grid
 * system for efficient spatial queries and collision detection.
 */
class PhysicsSystem
{
public:
    /**
     * @brief Constructor
     */
    PhysicsSystem();

    /**
     * @brief Destructor
     */
    ~PhysicsSystem();

    /**
     * @brief Initialize the physics system
     * @param scene Pointer to the scene to manage physics for
     * @return True if initialization succeeded
     */
    bool initialize(Scene* scene);

    /**
     * @brief Update physics simulation
     * @param deltaTime Time since last frame
     */
    void update(float deltaTime);

    /**
     * @brief Set the scene reference
     * @param scene Pointer to the scene
     */
    void setScene(Scene* scene);

    /**
     * @brief Set the voxel grid reference
     * @param grid Pointer to the cube grid
     */
    void setCubeGrid(CubeGrid* grid);

    /**
     * @brief Set gravity vector
     * @param gravity Gravity vector (default is down)
     */
    void setGravity(const glm::vec3& gravity);

    /**
     * @brief Get current gravity vector
     * @return Current gravity vector
     */
    const glm::vec3& getGravity() const;

    /**
     * @brief Set physics time step
     * @param timeStep Fixed time step for physics (seconds)
     */
    void setTimeStep(float timeStep);

    /**
     * @brief Get physics time step
     * @return Current physics time step
     */
    float getTimeStep() const;

    /**
     * @brief Set number of physics iterations per step
     * @param iterations Number of iterations
     */
    void setIterations(int iterations);

    /**
     * @brief Get number of physics iterations
     * @return Current number of iterations
     */
    int getIterations() const;

    /**
     * @brief Register a physics component
     * @param component Component to register
     */
    void registerComponent(PhysicsComponent* component);

    /**
     * @brief Unregister a physics component
     * @param component Component to unregister
     */
    void unregisterComponent(PhysicsComponent* component);

    /**
     * @brief Register a collider
     * @param collider Collider to register
     */
    void registerCollider(Collider* collider);

    /**
     * @brief Unregister a collider
     * @param collider Collider to unregister
     */
    void unregisterCollider(Collider* collider);

    /**
     * @brief Create a box collider
     * @param size Box dimensions
     * @return Pointer to created collider
     */
    BoxCollider* createBoxCollider(const glm::vec3& size = glm::vec3(1.0f));

    /**
     * @brief Create a sphere collider
     * @param radius Sphere radius
     * @return Pointer to created collider
     */
    SphereCollider* createSphereCollider(float radius = 0.5f);

    /**
     * @brief Create a capsule collider
     * @param radius Capsule radius
     * @param height Capsule height
     * @return Pointer to created collider
     */
    CapsuleCollider* createCapsuleCollider(float radius = 0.5f, float height = 2.0f);

    /**
     * @brief Create a voxel grid collider
     * @param gridRadius Radius of grid cells to include
     * @return Pointer to created collider
     */
    VoxelGridCollider* createVoxelGridCollider(int gridRadius = 1);

    /**
     * @brief Delete a collider
     * @param collider Collider to delete
     */
    void deleteCollider(Collider* collider);

    /**
     * @brief Cast a ray and find the first hit
     * @param ray Ray to cast
     * @param hitInfo reference to store hit information
     * @param layerMask Collision layer mask to filter against
     * @return True if the ray hit something
     */
    bool raycast(const Ray& ray, RaycastHit& hitInfo, uint32_t layerMask = 0xFFFFFFFF);

    /**
     * @brief Cast a ray and find all hits
     * @param ray Ray to cast
     * @param hitInfos Vector to store hit information
     * @param layerMask Collision layer mask to filter against
     * @return Number of hits found
     */
    int raycastAll(const Ray& ray, std::vector<RaycastHit>& hitInfos, uint32_t layerMask = 0xFFFFFFFF);

    /**
     * @brief Check for overlaps with a sphere
     * @param center Sphere center
     * @param radius Sphere radius
     * @param results Vector to store overlapping components
     * @param layerMask Collision layer mask to filter against
     * @return Number of overlapping components
     */
    int overlapSphere(const glm::vec3& center, float radius,
                      std::vector<PhysicsComponent*>& results,
                      uint32_t layerMask = 0xFFFFFFFF);

    /**
     * @brief Check for overlaps with a box
     * @param center Box center
     * @param halfExtents Half box dimensions
     * @param results Vector to store overlapping components
     * @param layerMask Collision layer mask to filter against
     * @return Number of overlapping components
     */
    int overlapBox(const glm::vec3& center, const glm::vec3& halfExtents,
                   std::vector<PhysicsComponent*>& results,
                   uint32_t layerMask = 0xFFFFFFFF);

    /**
     * @brief Check for overlaps with a capsule
     * @param point1 First capsule endpoint
     * @param point2 Second capsule endpoint
     * @param radius Capsule radius
     * @param results Vector to store overlapping components
     * @param layerMask Collision layer mask to filter against
     * @return Number of overlapping components
     */
    int overlapCapsule(const glm::vec3& point1, const glm::vec3& point2, float radius,
                       std::vector<PhysicsComponent*>& results,
                       uint32_t layerMask = 0xFFFFFFFF);

    /**
     * @brief Set whether to use continuous collision detection
     * @param useCCD Whether to use CCD
     */
    void setUseContinuousCollisionDetection(bool useCCD);

    /**
     * @brief Check if using continuous collision detection
     * @return True if using CCD
     */
    bool isUsingContinuousCollisionDetection() const;

    /**
     * @brief Set global collision callback
     * @param callback Function to call for all collisions
     */
    void setGlobalCollisionCallback(std::function<void(const CollisionInfo&)> callback);

    /**
     * @brief Set global trigger callback
     * @param enterCallback Function to call when triggers are entered
     * @param exitCallback Function to call when triggers are exited
     */
    void setGlobalTriggerCallbacks(
        std::function<void(PhysicsComponent*, PhysicsComponent*)> enterCallback,
        std::function<void(PhysicsComponent*, PhysicsComponent*)> exitCallback);

    /**
     * @brief Get whether collision is enabled between two layers
     * @param layer1 First collision layer
     * @param layer2 Second collision layer
     * @return True if collision is enabled
     */
    bool getCollisionEnabled(int layer1, int layer2) const;

    /**
     * @brief Set whether collision is enabled between two layers
     * @param layer1 First collision layer
     * @param layer2 Second collision layer
     * @param enabled Whether collision is enabled
     */
    void setCollisionEnabled(int layer1, int layer2, bool enabled);

    /**
     * @brief Set name for a collision layer
     * @param layer Layer index (0-31)
     * @param name Name for the layer
     */
    void setLayerName(int layer, const std::string& name);

    /**
     * @brief Get name for a collision layer
     * @param layer Layer index (0-31)
     * @return Name of the layer
     */
    const std::string& getLayerName(int layer) const;

    /**
     * @brief Get layer index by name
     * @param name Layer name
     * @return Layer index or -1 if not found
     */
    int getLayerByName(const std::string& name) const;

    /**
     * @brief Get debug draw state
     * @return True if debug drawing is enabled
     */
    bool isDebugDrawEnabled() const;

    /**
     * @brief Set debug draw state
     * @param enabled Whether to enable debug drawing
     */
    void setDebugDrawEnabled(bool enabled);

    /**
     * @brief Draw debug visualization for physics objects
     */
    void debugDraw();

    /**
     * @brief Pause or resume physics simulation
     * @param paused Whether physics should be paused
     */
    void setPaused(bool paused);

    /**
     * @brief Check if physics is paused
     * @return True if physics is paused
     */
    bool isPaused() const;

    /**
     * @brief Set physics simulation speed multiplier
     * @param timeScale Scale factor for physics time (1.0 = normal)
     */
    void setTimeScale(float timeScale);

    /**
     * @brief Get physics simulation speed multiplier
     * @return Current time scale factor
     */
    float getTimeScale() const;

    /**
     * @brief Create a character controller component
     * @param entity Entity to attach controller to
     * @param height Controller height
     * @param radius Controller radius
     * @return Pointer to created physics component
     */
    PhysicsComponent* createCharacterController(Entity* entity, float height, float radius);

    /**
     * @brief Set up platformer physics for a component
     * @param component Physics component to configure
     * @param jumpHeight Maximum jump height
     * @param jumpTime Time to reach peak height
     * @param maxSpeed Maximum movement speed
     */
    void setupPlatformerPhysics(PhysicsComponent* component,
                                float jumpHeight = 2.0f,
                                float jumpTime = 0.5f,
                                float maxSpeed = 5.0f);

    /**
     * @brief Get collision statistics for debugging
     * @param outTotalTests Reference to store total collision tests
     * @param outActiveCollisions Reference to store active collisions
     */
    void getCollisionStats(int& outTotalTests, int& outActiveCollisions) const;

private:
    // Core simulation properties
    Scene* m_scene;
    CubeGrid* m_cubeGrid;
    glm::vec3 m_gravity;
    float m_fixedTimeStep;
    float m_accumulatedTime;
    int m_iterations;
    bool m_paused;
    float m_timeScale;
    bool m_useCCD;
    bool m_debugDrawEnabled;

    // Entity tracking
    std::vector<PhysicsComponent*> m_components;
    std::vector<Collider*> m_colliders;
    std::unordered_map<Entity*, PhysicsComponent*> m_entityComponentMap;

    // Spatial partitioning
    float m_worldCellSize;
    std::vector<std::vector<std::vector<std::vector<PhysicsComponent*>>>> m_worldGrid;
    std::unordered_map<PhysicsComponent*, glm::ivec3> m_componentGridPos;

    // Collision settings
    bool m_collisionMatrix[32][32];
    std::string m_layerNames[32];

    // Collision tracking
    struct CollisionPair
    {
        PhysicsComponent* a;
        PhysicsComponent* b;
        bool inContact;
        bool isTrigger;

        bool operator==(const CollisionPair& other) const
        {
            return (a == other.a && b == other.b) || (a == other.b && b == other.a);
        }
    };
    std::vector<CollisionPair> m_collisionPairs;
    std::vector<CollisionInfo> m_currentCollisions;
    int m_totalCollisionTests;
    int m_activeCollisions;

    // Callbacks
    std::function<void(const CollisionInfo&)> m_globalCollisionCallback;
    std::function<void(PhysicsComponent*, PhysicsComponent*)> m_globalTriggerEnterCallback;
    std::function<void(PhysicsComponent*, PhysicsComponent*)> m_globalTriggerExitCallback;

    // Memory management
    std::vector<std::unique_ptr<Collider>> m_ownedColliders;

    // Grid query helpers
    std::vector<glm::ivec3> getGridCellsForAABB(const glm::vec3& min, const glm::vec3& max) const;
    glm::ivec3 worldToGridPos(const glm::vec3& worldPos) const;

    // Fixed update method for physics simulation
    void fixedUpdate(float fixedDeltaTime);

    // Physics update steps
    void updateTransforms();
    void integrateVelocities(float deltaTime);
    void detectCollisions();
    void resolveCollisions();
    void integratePositions(float deltaTime);
    void updateTriggers();
    void updateGridPositions();

    // Collision detection helpers
    bool checkCollision(PhysicsComponent* a, PhysicsComponent* b, CollisionInfo& info);
    bool checkCollision(Collider* a, Collider* b, CollisionInfo& info);
    bool sphereVsSphere(const SphereCollider* a, const SphereCollider* b, CollisionInfo& info);
    bool sphereVsBox(const SphereCollider* a, const BoxCollider* b, CollisionInfo& info);
    bool boxVsBox(const BoxCollider* a, const BoxCollider* b, CollisionInfo& info);
    bool sphereVsCapsule(const SphereCollider* a, const CapsuleCollider* b, CollisionInfo& info);
    bool boxVsCapsule(const BoxCollider* a, const CapsuleCollider* b, CollisionInfo& info);
    bool capsuleVsCapsule(const CapsuleCollider* a, const CapsuleCollider* b, CollisionInfo& info);
    bool sphereVsVoxelGrid(const SphereCollider* a, const VoxelGridCollider* b, CollisionInfo& info);
    bool boxVsVoxelGrid(const BoxCollider* a, const VoxelGridCollider* b, CollisionInfo& info);
    bool capsuleVsVoxelGrid(const CapsuleCollider* a, const VoxelGridCollider* b, CollisionInfo& info);
    bool checkVoxelCollision(const glm::vec3& worldPos, glm::vec3& normal);

    // Voxel collision helpers
    void checkVoxelGridCollisions(PhysicsComponent* component);
    void resolveVoxelCollisions(PhysicsComponent* component, const CollisionInfo& collision);

    // Collision resolution
    void resolveContactConstraint(CollisionInfo& info, float deltaTime);
    void resolvePositionalConstraint(CollisionInfo& info);
    void applyImpulse(PhysicsComponent* a, PhysicsComponent* b, const glm::vec3& point,
                      const glm::vec3& normal, float depth);

    // Raycast helpers
    bool raycastVoxels(const Ray& ray, RaycastHit& hitInfo);

    // Spatial partitioning helpers
    void updateComponentGridPosition(PhysicsComponent* component);
    void removeComponentFromGrid(PhysicsComponent* component);

    // Collision pair management
    void updateCollisionPair(PhysicsComponent* a, PhysicsComponent* b, bool isTrigger);
    void removeComponentFromCollisionPairs(PhysicsComponent* component);

    // Platformer physics helpers
    void updatePlatformerPhysics(PhysicsComponent* component, float deltaTime);
};

/**
 * @class CharacterController
 * @brief Specialized physics handler for character movement in platformers
 * 
 * The CharacterController provides high-level methods for controlling
 * character movement in a platformer game with appropriate physics behavior.
 */
class CharacterController
{
public:
    /**
     * @brief Constructor
     * @param physicsComponent Component to control
     */
    CharacterController(PhysicsComponent* physicsComponent);

    /**
     * @brief Destructor
     */
    ~CharacterController();

    /**
     * @brief Set the physics component to control
     * @param component Component to control
     */
    void setPhysicsComponent(PhysicsComponent* component);

    /**
     * @brief Get the physics component
     * @return Component being controlled
     */
    PhysicsComponent* getPhysicsComponent() const;

    /**
     * @brief Apply movement input
     * @param direction Direction in world space
     * @param strength Input strength (0-1)
     */
    void move(const glm::vec3& direction, float strength = 1.0f);

    /**
     * @brief Attempt to jump
     * @return True if jump was executed
     */
    bool jump();

    /**
     * @brief Signal that jump button was released
     */
    void jumpReleased();

    /**
     * @brief Attempt to dash
     * @param direction Direction to dash
     * @return True if dash was executed
     */
    bool dash(const glm::vec3& direction);

    /**
     * @brief Set movement speed parameters
     * @param maxSpeed Maximum movement speed
     * @param acceleration Acceleration rate
     * @param deceleration Deceleration rate
     */
    void setMovementParameters(float maxSpeed, float acceleration, float deceleration);

    /**
     * @brief Set jump parameters
     * @param jumpHeight Maximum jump height
     * @param jumpTime Time to reach peak height
     * @param variableHeight Whether to use variable height jumps
     */
    void setJumpParameters(float jumpHeight, float jumpTime, bool variableHeight = true);

    /**
     * @brief Set dash parameters
     * @param dashForce Force of dash
     * @param dashDuration Duration of the dash
     * @param dashCooldown Time between dashes
     */
    void setDashParameters(float dashForce, float dashDuration, float dashCooldown);

    /**
     * @brief Set wall jump parameters
     * @param enabled Whether wall jumping is enabled
     * @param slideGravityScale Gravity scale when wall sliding
     * @param jumpForce Force of wall jump
     */
    void setWallJumpParameters(bool enabled, float slideGravityScale, float jumpForce);

    /**
     * @brief Set ledge grab parameters
     * @param enabled Whether ledge grabbing is enabled
     */
    void setLedgeGrabParameters(bool enabled);

    /**
     * @brief Check if controller is grounded
     * @return True if grounded
     */
    bool isGrounded() const;

    /**
     * @brief Check if controller is wall sliding
     * @return True if sliding on a wall
     */
    bool isWallSliding() const;

    /**
     * @brief Check if controller is dashing
     * @return True if currently dashing
     */
    bool isDashing() const;

    /**
     * @brief Check if controller is grabbing a ledge
     * @return True if grabbing a ledge
     */
    bool isGrabbingLedge() const;

    /**
     * @brief Try to climb up from a ledge
     */
    void climbLedge();

    /**
     * @brief Get the current velocity
     * @return Current velocity vector
     */
    glm::vec3 getVelocity() const;

    /**
     * @brief Set whether to use air control
     * @param useAirControl Whether to allow movement control in air
     * @param airControlFactor Air control strength (0-1)
     */
    void setAirControl(bool useAirControl, float airControlFactor = 0.5f);

    /**
     * @brief Update the controller
     * @param deltaTime Time since last frame
     */
    void update(float deltaTime);

private:
    PhysicsComponent* m_physicsComponent;

    // Movement parameters
    float m_maxSpeed;
    float m_acceleration;
    float m_deceleration;
    bool m_useAirControl;
    float m_airControlFactor;

    // Jump parameters
    float m_jumpHeight;
    float m_jumpTime;
    bool m_variableJumpHeight;
    float m_coyoteTime;
    float m_jumpBufferTime;

    // Dash parameters
    float m_dashForce;
    float m_dashDuration;
    float m_dashCooldown;
    float m_dashTimer;
    float m_dashCooldownTimer;
    bool m_dashing;

    // Wall jump parameters
    bool m_wallJumpEnabled;
    float m_wallSlideGravityScale;
    float m_wallJumpForce;

    // Ledge grab parameters
    bool m_ledgeGrabEnabled;

    // State tracking
    bool m_wasGrounded;
    glm::vec3 m_currentMoveDirection;
    float m_currentMoveStrength;

    // Helpers
    void checkLedgeGrab();
    void updateDashState(float deltaTime);
};