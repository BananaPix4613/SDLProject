#include "PhysicsSystem.h"
#include "components/PhysicsComponent.h"
#include "Collider.h"
#include "Entity.h"
#include "Scene.h"
#include "CubeGrid.h"
#include <glm/glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/quaternion.hpp>
#include <algorithm>
#include <memory>
#include <unordered_set>

PhysicsSystem::PhysicsSystem() :
    m_scene(nullptr),
    m_cubeGrid(nullptr),
    m_gravity(0.0f, -9.81f, 0.0f),
    m_fixedTimeStep(1.0f / 60.0f),
    m_accumulatedTime(0.0f),
    m_iterations(4),
    m_paused(false),
    m_timeScale(1.0f),
    m_useCCD(false),
    m_debugDrawEnabled(false),
    m_worldCellSize(10.0f),
    m_totalCollisionTests(0),
    m_activeCollisions(0)
{
    // Initialize collision matrix to allow all collisions by default
    for (int i = 0; i < 32; i++)
    {
        for (int j = 0; j < 32; j++)
        {
            m_collisionMatrix[i][j] = true;
        }

        // Set default layer name
        m_layerNames[i] = "Layer " + std::to_string(i);
    }
}

PhysicsSystem::~PhysicsSystem()
{
    // Clean up owned colliders
    m_ownedColliders.clear();
}

bool PhysicsSystem::initialize(Scene* scene)
{
    m_scene = scene;

    // Create default spatial partitioning grid
    // In a real implementation, this would be sized based on world bounds
    m_worldGrid.resize(10);
    for (auto& x : m_worldGrid)
    {
        x.resize(10);
        for (auto& y : x)
        {
            y.resize(10);
        }
    }

    return true;
}

void PhysicsSystem::update(float deltaTime)
{
    if (m_paused)
        return;

    // Scale time
    float scaledDelta = deltaTime * m_timeScale;

    // Accumulate time for fixed timestep
    m_accumulatedTime += scaledDelta;

    // Run fixed timestep updates
    while (m_accumulatedTime >= m_fixedTimeStep)
    {
        // Perform physics step
        fixedUpdate(m_fixedTimeStep);

        // Reduce accumulated time
        m_accumulatedTime -= m_fixedTimeStep;
    }

    // Update debug visualization
    if (m_debugDrawEnabled)
    {
        debugDraw();
    }
}

void PhysicsSystem::fixedUpdate(float fixedDeltaTime)
{
    // Reset collision statistics
    m_totalCollisionTests = 0;
    m_activeCollisions = 0;

    // 1. Update entity transforms from physics components
    updateTransforms();

    // 2. Integrate velocities (apply forces)
    integrateVelocities(fixedDeltaTime);

    // 3. Detect collisions
    detectCollisions();

    // 4. Resolve collisions
    for (int i = 0; i < m_iterations; i++)
    {
        resolveCollisions();
    }

    // 5. Integrate positions
    integratePositions(fixedDeltaTime);

    // 6. Update triggers
    updateTriggers();

    // 7. Update spatial partitioning
    updateGridPositions();

    // Clear forces for next update
    for (auto component : m_components)
    {
        if (component->getBodyType() == PhysicsBodyType::DYNAMIC)
        {
            component->m_totalForce = glm::vec3(0.0f);
            component->m_totalTorque = glm::vec3(0.0f);
        }
    }
}

void PhysicsSystem::setScene(Scene* scene)
{
    m_scene = scene;
}

void PhysicsSystem::setCubeGrid(CubeGrid* grid)
{
    m_cubeGrid = grid;
}

void PhysicsSystem::setGravity(const glm::vec3& gravity)
{
    m_gravity = gravity;
}

const glm::vec3& PhysicsSystem::getGravity() const
{
    return m_gravity;
}

void PhysicsSystem::setTimeStep(float timeStep)
{
    m_fixedTimeStep = timeStep;
}

float PhysicsSystem::getTimeStep() const
{
    return m_fixedTimeStep;
}

void PhysicsSystem::setIterations(int iterations)
{
    m_iterations = glm::max(1, iterations);
}

int PhysicsSystem::getIterations() const
{
    return m_iterations;
}

void PhysicsSystem::registerComponent(PhysicsComponent* component)
{
    if (!component)
        return;

    // Check if component is already registered
    auto it = std::find(m_components.begin(), m_components.end(), component);
    if (it != m_components.end())
        return;

    // Add to components list
    m_components.push_back(component);

    // Add to entity map
    Entity* entity = component->getEntity();
    if (entity)
    {
        m_entityComponentMap[entity] = component;
    }

    // Register colliders
    for (auto collider : component->getColliders())
    {
        registerCollider(collider);
    }

    // Add to spatial grid
    updateComponentGridPosition(component);
}

void PhysicsSystem::unregisterComponent(PhysicsComponent* component)
{
    if (!component)
        return;

    // Remove from components list
    auto it = std::find(m_components.begin(), m_components.end(), component);
    if (it != m_components.end())
    {
        m_components.erase(it);
    }

    // Remove from entity map
    Entity* entity = component->getEntity();
    if (entity)
    {
        m_entityComponentMap.erase(entity);
    }

    // Unregister colliders
    for (auto collider : component->getColliders())
    {
        unregisterCollider(collider);
    }

    // Remove from spatial grid
    removeComponentFromGrid(component);

    // Remove component from collision pairs
    removeComponentFromCollisionPairs(component);
}

void PhysicsSystem::registerCollider(Collider* collider)
{
    if (!collider)
        return;

    // Check if collider is already registered
    auto it = std::find(m_colliders.begin(), m_colliders.end(), collider);
    if (it != m_colliders.end())
        return;

    // Add to colliders list
    m_colliders.push_back(collider);
}

void PhysicsSystem::unregisterCollider(Collider* collider)
{
    if (!collider)
        return;

    // Remove from colliders list
    auto it = std::find(m_colliders.begin(), m_colliders.end(), collider);
    if (it != m_colliders.end())
    {
        m_colliders.erase(it);
    }
}

BoxCollider* PhysicsSystem::createBoxCollider(const glm::vec3& size)
{
    auto collider = new BoxCollider(size);
    m_ownedColliders.push_back(std::unique_ptr<Collider>(collider));
    return collider;
}

SphereCollider* PhysicsSystem::createSphereCollider(float radius)
{
    auto collider = new SphereCollider(radius);
    m_ownedColliders.push_back(std::unique_ptr<Collider>(collider));
    return collider;
}

CapsuleCollider* PhysicsSystem::createCapsuleCollider(float radius, float height)
{
    auto collider = new CapsuleCollider(radius, height);
    m_ownedColliders.push_back(std::unique_ptr<Collider>(collider));
    return collider;
}

VoxelGridCollider* PhysicsSystem::createVoxelGridCollider(int gridRadius)
{
    auto collider = new VoxelGridCollider(gridRadius);
    m_ownedColliders.push_back(std::unique_ptr<Collider>(collider));
    return collider;
}

void PhysicsSystem::deleteCollider(Collider* collider)
{
    if (!collider)
        return;

    // Unregister collider
    unregisterCollider(collider);

    // Remove from owner component if any
    PhysicsComponent* component = collider->getPhysicsComponent();
    if (component)
    {
        component->removeCollider(collider);
    }

    // Remove from owned colliders if present
    for (auto it = m_ownedColliders.begin(); it != m_ownedColliders.end(); it++)
    {
        if (it->get() == collider)
        {
            m_ownedColliders.erase(it);
            return;
        }
    }
}

bool PhysicsSystem::raycast(const Ray& ray, RaycastHit& hitInfo, uint32_t layerMask)
{
    // Reset hit info
    hitInfo = RaycastHit();

    // First check against voxel grid if available
    bool hitVoxels = false;
    RaycastHit voxelHit;

    if (m_cubeGrid)
    {
        hitVoxels = raycastVoxels(ray, voxelHit);
    }

    // Check against physics objects
    bool hitObjects = false;
    RaycastHit objectHit;
    objectHit.distance = FLT_MAX;

    // For each physics component with appropriate layer mask
    for (auto component : m_components)
    {
        // Skip if component's layer is not in mask
        if (!(layerMask & (1 << component->getCollisionLayer())))
            continue;

        // For each collider in the component
        for (auto collider : component->getColliders())
        {
            // Simple sphere test for demonstration
            // In a real implementation, we would have proper ray-collider tests
            if (collider->getType() == ColliderType::SPHERE)
            {
                SphereCollider* sphere = static_cast<SphereCollider*>(collider);

                // Get sphere center in world space
                glm::mat4 worldTransform = collider->getWorldTransform();
                glm::vec4 centerVec4 = worldTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                glm::vec3 center = glm::vec3(centerVec4) / centerVec4.w;

                // Check ray-sphere intersection
                glm::vec3 l = center - ray.origin;
                float tca = glm::dot(l, ray.direction);

                // If sphere is behind ray origin and ray is not pointing toward it
                if (tca < 0.0f)
                    continue;

                float d2 = glm::dot(l, l) - tca * tca;
                float radius2 = sphere->getRadius() * sphere->getRadius();

                // Ray misses the sphere
                if (d2 > radius2)
                    continue;

                // Ray hits the sphere
                float thc = sqrtf(radius2 - d2);
                float t0 = tca - thc;
                float t1 = tca + thc;

                // Ensure t0 is the closest hit point
                if (t0 > t1)
                    std::swap(t0, t1);

                // If closest hit is behind ray origin, use the other hit point
                if (t0 < 0.0f)
                {
                    t0 = t1;

                    // Both hit points are behind ray origin
                    if (t0 < 0.0f)
                        continue;
                }

                // Check distance against ray max distance
                if (t0 > ray.maxDistance)
                    continue;

                // Check if this is the closest hit so far
                if (t0 < objectHit.distance)
                {
                    hitObjects = true;
                    objectHit.distance = t0;
                    objectHit.point = ray.origin + ray.direction * t0;
                    objectHit.normal = glm::normalize(objectHit.point - center);
                    objectHit.hitCollider = collider;
                    objectHit.hitComponent = component;
                    objectHit.hasHit = true;
                }
            }
            else if (collider->getType() == ColliderType::BOX)
            {
                // Ray-box intersection would be implemented here
                // For brevity, we're skipping the implementation details
            }
            // Other collider types would be handled similarly
        }
    }

    // Determine which hit to return (closest one)
    if (hitVoxels && hitObjects)
    {
        if (voxelHit.distance < objectHit.distance)
        {
            hitInfo = voxelHit;
        }
        else
        {
            hitInfo = objectHit;
        }
        return true;
    }
    else if (hitVoxels)
    {
        hitInfo = voxelHit;
        return true;
    }
    else if (hitObjects)
    {
        hitInfo = objectHit;
        return true;
    }

    return false;
}

int PhysicsSystem::raycastAll(const Ray& ray, std::vector<RaycastHit>& hitInfos, uint32_t layerMask)
{
    // Clear results vector
    hitInfos.clear();

    // Check against voxel grid if available
    if (m_cubeGrid)
    {
        RaycastHit voxelHit;
        if (raycastVoxels(ray, voxelHit))
        {
            hitInfos.push_back(voxelHit);
        }
    }

    // For each physics component with appropriate layer mask
    for (auto component : m_components)
    {
        // Skip if component's layer is not in mask
        if (!(layerMask & (1 << component->getCollisionMask())))
            continue;

        // For each collider in the component
        for (auto collider : component->getColliders())
        {
            // Simple sphere test for demonstration
            // In a real implementation, we would have proper ray-collider tests
            if (collider->getType() == ColliderType::SPHERE)
            {
                SphereCollider* sphere = static_cast<SphereCollider*>(collider);

                // Get sphere center in world space
                glm::mat4 worldTransform = collider->getWorldTransform();
                glm::vec4 centerVec4 = worldTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                glm::vec3 center = glm::vec3(centerVec4) / centerVec4.w;

                // Check ray-sphere intersection
                glm::vec3 l = center - ray.origin;
                float tca = glm::dot(l, ray.direction);

                // If sphere is behind ray origin and ray is not pointing toward it
                if (tca < 0.0f)
                    continue;

                float d2 = glm::dot(l, l) - tca * tca;
                float radius2 = sphere->getRadius() * sphere->getRadius();

                // Ray misses the sphere
                if (d2 > radius2)
                    continue;

                // Ray hits the sphere
                float thc = sqrtf(radius2 - d2);
                float t0 = tca - thc;
                float t1 = tca + thc;

                // Ensure t0 is the closest hit point
                if (t0 > t1)
                    std::swap(t0, t1);

                // If closest hit is behind ray origin, use the other hit point
                if (t0 < 0.0f)
                {
                    t0 = t1;

                    // Both hit points are behind ray origin
                    if (t0 < 0.0f)
                        continue;
                }

                // Check distance against ray max distance
                if (t0 > ray.maxDistance)
                    continue;

                // Add hit to results
                RaycastHit hit;
                hit.distance = t0;
                hit.point = ray.origin + ray.direction * t0;
                hit.normal = glm::normalize(hit.point - center);
                hit.hitCollider = collider;
                hit.hitComponent = component;
                hit.hasHit = true;

                hitInfos.push_back(hit);
            }
            else if (collider->getType() == ColliderType::BOX)
            {
                // Ray-box intersection would be implemented here
                // for brevity, we're skipping the implementation details
            }
            // Other collider types would be handled similarly
        }
    }

    // Stort hits by distance
    std::sort(hitInfos.begin(), hitInfos.end(),
              [](const RaycastHit& a, const RaycastHit& b) { return a.distance < b.distance; });

    return hitInfos.size();
}

int PhysicsSystem::overlapSphere(const glm::vec3& center, float radius,
                                 std::vector<PhysicsComponent*>& results,
                                 uint32_t layerMask)
{
    // Clear results vector
    results.clear();

    // Use spatial partitioning to find potential overlaps
    std::vector<PhysicsComponent*> potentialOverlaps;

    // Get grid cells that the sphere might overlap
    glm::vec3 min = center - glm::vec3(radius);
    glm::vec3 max = center + glm::vec3(radius);
    std::vector<glm::ivec3> cells = getGridCellsForAABB(min, max);

    // Gather components from potential cells
    for (const auto& cell : cells)
    {
        // Skip invalid cells
        if (cell.x < 0 || cell.y < 0 || cell.z < 0 ||
            cell.x >= m_worldGrid.size() ||
            cell.y >= m_worldGrid[0].size() ||
            cell.z >= m_worldGrid[0][0].size())
            continue;

        // Add components from this cell
        for (auto component : m_worldGrid[cell.x][cell.y][cell.z])
        {
            // Skip if already in potential overlaps
            if (std::find(potentialOverlaps.begin(), potentialOverlaps.end(), component) != potentialOverlaps.end())
                continue;

            // Skip if component's layer is not in mask
            if (!(layerMask & (1 << component->getCollisionLayer())))
                continue;

            potentialOverlaps.push_back(component);
        }
    }

    // Check each potential overlap
    for (auto component : potentialOverlaps)
    {
        bool overlaps = false;

        // Check each collider in the component
        for (auto collider : component->getColliders())
        {
            // Sphere-sphere test for simplicity
            if (collider->getType() == ColliderType::SPHERE)
            {
                SphereCollider* sphere = static_cast<SphereCollider*>(collider);

                // Get sphere center in world space
                glm::mat4 worldTransform = collider->getWorldTransform();
                glm::vec4 centerVec4 = worldTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                glm::vec3 sphereCenter = glm::vec3(centerVec4) / centerVec4.w;

                // Check sphere-sphere overlap
                float distSquared = glm::distance2(center, sphereCenter);
                float radiusSum = radius + sphere->getRadius();

                if (distSquared <= radiusSum * radiusSum)
                {
                    overlaps = true;
                    break;
                }
            }
            else if (collider->getType() == ColliderType::BOX)
            {
                // Sphere-box overlap test would be implemented here
                // For brevity, we're using a simplified test

                // Get box world bounds
                glm::vec3 boxMin, boxMax;
                collider->getWorldBounds(boxMin, boxMax);

                // Find closest point on box to sphere center
                glm::vec3 closest;
                closest.x = glm::max(boxMin.x, glm::min(center.x, boxMax.x));
                closest.y = glm::max(boxMin.y, glm::min(center.y, boxMax.y));
                closest.z = glm::max(boxMin.z, glm::min(center.z, boxMax.z));

                // Check if closest point is within radius
                float distSquared = glm::distance2(center, closest);
                if (distSquared <= radius * radius)
                {
                    overlaps = true;
                    break;
                }
            }
            // Other collider types would be handled similarly
        }

        // Add to results if any collider overlaps
        if (overlaps)
        {
            results.push_back(component);
        }
    }

    return results.size();
}

int PhysicsSystem::overlapBox(const glm::vec3& center, const glm::vec3& halfExtents,
                              std::vector<PhysicsComponent*>& results,
                              uint32_t layerMask)
{
    // Clear results vector
    results.clear();

    // Calculate box bounds
    glm::vec3 min = center - halfExtents;
    glm::vec3 max = center + halfExtents;

    // Use spatial partitioning to find potential overlaps
    std::vector<PhysicsComponent*> potentialOverlaps;

    // Get grid cells that the box might overlap
    std::vector<glm::ivec3> cells = getGridCellsForAABB(min, max);

    // Gather components from potential cells
    for (const auto& cell : cells)
    {
        // Skip invalid cells
        if (cell.x < 0 || cell.y < 0 || cell.z < 0 ||
            cell.x >= m_worldGrid.size() ||
            cell.y >= m_worldGrid[0].size() ||
            cell.z >= m_worldGrid[0][0].size())
            continue;

        // Add components from this cell
        for (auto component : m_worldGrid[cell.x][cell.y][cell.z])
        {
            // Skip if already in potential overlaps
            if (std::find(potentialOverlaps.begin(), potentialOverlaps.end(), component) != potentialOverlaps.end())
                continue;

            // Skip if component's layer is not in mask
            if (!(layerMask & (1 << component->getCollisionLayer())))
                continue;

            potentialOverlaps.push_back(component);
        }
    }

    // Check each potential overlap
    for (auto component : potentialOverlaps)
    {
        bool overlaps = false;

        // Check each collider in the component
        for (auto collider : component->getColliders())
        {
            // Box-box test for BoxCollider
            if (collider->getType() == ColliderType::BOX)
            {
                // Get box world bounds
                glm::vec3 boxMin, boxMax;
                collider->getWorldBounds(boxMin, boxMax);

                // Check AABB overlap
                if (boxMin.x <= max.x && boxMax.x >= min.x &&
                    boxMin.y <= max.y && boxMax.y >= min.y &&
                    boxMin.z <= max.z && boxMax.z >= min.z)
                {
                    overlaps = true;
                    break;
                }
            }
            else if (collider->getType() == ColliderType::SPHERE)
            {
                // Box-sphere overlap test (reusing the sphere-box test from above)
                SphereCollider* sphere = static_cast<SphereCollider*>(collider);

                // Get sphere center in world space
                glm::mat4 worldTransform = collider->getWorldTransform();
                glm::vec4 centerVec4 = worldTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                glm::vec3 sphereCenter = glm::vec3(centerVec4) / centerVec4.w;

                // Find closest point on box to sphere center
                glm::vec3 closest;
                closest.x = glm::max(min.x, glm::min(sphereCenter.x, max.x));
                closest.y = glm::max(min.y, glm::min(sphereCenter.y, max.y));
                closest.z = glm::max(min.z, glm::min(sphereCenter.z, max.z));

                // Check if closest point is within radius
                float distSquared = glm::distance2(sphereCenter, closest);
                if (distSquared <= sphere->getRadius() * sphere->getRadius())
                {
                    overlaps = true;
                    break;
                }
            }
            // Other collider types would be handled similarly
        }

        // Add to results if any collider overlaps
        if (overlaps)
        {
            results.push_back(component);
        }
    }

    return results.size();
}

int PhysicsSystem::overlapCapsule(const glm::vec3& point1, const glm::vec3& point2, float radius,
                                  std::vector<PhysicsComponent*>& results,
                                  uint32_t layerMask)
{
    // Clear results vector
    results.clear();

    // Calculate capsule bounds (as an AABB)
    glm::vec3 min = glm::min(point1, point2) - glm::vec3(radius);
    glm::vec3 max = glm::max(point1, point2) + glm::vec3(radius);

    // Use spatial partitioning to find potential overlaps
    std::vector<PhysicsComponent*> potentialOverlaps;

    // Get grid cells that the capsule might overlap
    std::vector<glm::ivec3> cells = getGridCellsForAABB(min, max);

    // Gather components from potential cells
    for (const auto& cell : cells)
    {
        // Skip invalid cells
        if (cell.x < 0 || cell.y < 0 || cell.z < 0 ||
            cell.x >= m_worldGrid.size() ||
            cell.y >= m_worldGrid[0].size() ||
            cell.z >= m_worldGrid[0][0].size())
            continue;

        // Add components from this cell
        for (auto component : m_worldGrid[cell.x][cell.y][cell.z])
        {
            // Skip if already in potential overlaps
            if (std::find(potentialOverlaps.begin(), potentialOverlaps.end(), component) != potentialOverlaps.end())
                continue;

            // Skip if component's layer is not in mask
            if (!(layerMask & (1 << component->getCollisionLayer())))
                continue;

            potentialOverlaps.push_back(component);
        }
    }

    // Check each potential overlap
    for (auto component : potentialOverlaps)
    {
        bool overlaps = false;

        // Check each collider in the component
        for (auto collider : component->getColliders())
        {
            // For demonstration, we'll use simplified capsule overlap checks
            // In a real implementation, proper capsule-collider tests would be used

            // For spheres, we'll check capsule-sphere overlap
            if (collider->getType() == ColliderType::SPHERE)
            {
                SphereCollider* sphere = static_cast<SphereCollider*>(collider);

                // Get sphere center in world space
                glm::mat4 worldTransform = collider->getWorldTransform();
                glm::vec4 centerVec4 = worldTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                glm::vec3 sphereCenter = glm::vec3(centerVec4) / centerVec4.w;

                // Calculate closest point on capsule line segment to sphere center
                glm::vec3 capsuleDir = point2 - point1;
                float capsuleLength = glm::length(capsuleDir);

                if (capsuleLength < 0.0001f)
                {
                    // Capsule is degenerate (a sphere)
                    float distSquared = glm::distance2(sphereCenter, point1);
                    float radiusSum = radius + sphere->getRadius();

                    if (distSquared <= radiusSum * radiusSum)
                    {
                        overlaps = true;
                        break;
                    }
                }
                else
                {
                    // Normalize capsule direction
                    capsuleDir /= capsuleLength;

                    // Project sphere center onto capsule axis
                    glm::vec3 toSphere = sphereCenter - point1;
                    float projection = glm::dot(toSphere, capsuleDir);

                    // Clamp projection to capsule segment
                    projection = glm::clamp(projection, 0.0f, capsuleLength);

                    // Get closest point on capsule axis to sphere center
                    glm::vec3 closestPoint = point1 + capsuleDir * projection;

                    // Check distance
                    float distSquared = glm::distance2(sphereCenter, closestPoint);
                    float radiusSum = radius + sphere->getRadius();

                    if (distSquared <= radiusSum * radiusSum)
                    {
                        overlaps = true;
                        break;
                    }
                }
            }
            else if (collider->getType() == ColliderType::BOX)
            {
                // For box colliders, we'll use a conservative check
                // A proper capsule-box test would be more complex

                // Get box world bounds
                glm::vec3 boxMin, boxMax;
                collider->getWorldBounds(boxMin, boxMax);

                // Check if capsule endpoints might overlap box (expanded by radius)
                if ((point1.x >= boxMin.x - radius && point1.x <= boxMax.x + radius &&
                     point1.y >= boxMin.y - radius && point1.y <= boxMax.y + radius &&
                     point1.z >= boxMin.z - radius && point1.z <= boxMax.z + radius) ||
                    (point2.x >= boxMin.x - radius && point2.x <= boxMax.x + radius &&
                     point2.y >= boxMin.y - radius && point2.y <= boxMax.y + radius &&
                     point2.z >= boxMin.z - radius && point2.z <= boxMax.z + radius))
                {
                    // This is just a conservative check
                    // A proper capsule-box test would be more precise
                    overlaps = true;
                    break;
                }
            }
            // Other collider types would be handled similarly
        }

        // Add to results if any collider overlaps
        if (overlaps)
        {
            results.push_back(component);
        }
    }

    return results.size();
}

void PhysicsSystem::setUseContinuousCollisionDetection(bool useCCD)
{
    m_useCCD = useCCD;
}

bool PhysicsSystem::isUsingContinuousCollisionDetection() const
{
    return m_useCCD;
}

void PhysicsSystem::setGlobalCollisionCallback(std::function<void(const CollisionInfo&)> callback)
{
    m_globalCollisionCallback = callback;
}

void PhysicsSystem::setGlobalTriggerCallbacks(
    std::function<void(PhysicsComponent*, PhysicsComponent*)> enterCallback,
    std::function<void(PhysicsComponent*, PhysicsComponent*)> exitCallback)
{
    m_globalTriggerEnterCallback = enterCallback;
    m_globalTriggerExitCallback = exitCallback;
}

bool PhysicsSystem::getCollisionEnabled(int layer1, int layer2) const
{
    // Ensure layers are in valid range
    if (layer1 < 0 || layer1 > 31 || layer2 < 0 || layer2 > 31)
        return false;

    return m_collisionMatrix[layer1][layer2];
}

void PhysicsSystem::setCollisionEnabled(int layer1, int layer2, bool enabled)
{
    // Ensure layers are in valid range
    if (layer1 < 0 || layer1 > 31 || layer2 < 0 || layer2 > 31)
        return;

    // Set collision between these layers
    m_collisionMatrix[layer1][layer2] = enabled;

    // Ensure symmetry (if A collides with B, B collides with A)
    m_collisionMatrix[layer2][layer1] = enabled;
}

void PhysicsSystem::setLayerName(int layer, const std::string& name)
{
    // Ensure layer is in valid range
    if (layer < 0 || layer > 31)
        return;

    m_layerNames[layer] = name;
}

const std::string& PhysicsSystem::getLayerName(int layer) const
{
    // Ensure layer is in valid range
    if (layer < 0 || layer > 31)
    {
        static std::string emptyString;
        return emptyString;
    }

    return m_layerNames[layer];
}

int PhysicsSystem::getLayerByName(const std::string& name) const
{
    // Find layer with matching name
    for (int i = 0; i < 32; i++)
    {
        if (m_layerNames[i] == name)
            return i;
    }

    // Not found
    return -1;
}

bool PhysicsSystem::isDebugDrawEnabled() const
{
    return m_debugDrawEnabled;
}

void PhysicsSystem::setDebugDrawEnabled(bool enabled)
{
    m_debugDrawEnabled = enabled;
}

void PhysicsSystem::debugDraw()
{
    // This would be implemented using a debug drawing system
    // For now, we'll leave it as a stub
}

void PhysicsSystem::setPaused(bool paused)
{
    m_paused = paused;
}

bool PhysicsSystem::isPaused() const
{
    return m_paused;
}

void PhysicsSystem::setTimeScale(float timeScale)
{
    m_timeScale = timeScale;
}

float PhysicsSystem::getTimeScale() const
{
    return m_timeScale;
}

PhysicsComponent* PhysicsSystem::createCharacterController(Entity* entity, float height, float radius)
{
    if (!entity)
        return nullptr;

    // Create physics component
    PhysicsComponent* component = entity->addComponent<PhysicsComponent>();
    if (!component)
        return nullptr;

    // Set up as kinematic by default
    component->setBodyType(PhysicsBodyType::DYNAMIC);
    component->setUseGravity(true);

    // Create capsule collider
    CapsuleCollider* capsule = createCapsuleCollider(radius, height);
    component->addCollider(capsule);

    // Set up for platformer physics
    component->setPlatformerPhysics(true);
    component->setJumpParameters(2.0f, 0.5f);
    component->setMovementParameters(5.0f, 20.0f, 10.0f);

    // Lock rotation
    component->setCanRotate(false);

    return component;
}

void PhysicsSystem::setupPlatformerPhysics(PhysicsComponent* component, float jumpHeight, float jumpTime, float maxSpeed)
{
    if (!component)
        return;

    // Enable platformer physics
    component->setPlatformerPhysics(true);

    // Set jump parameters
    component->setJumpParameters(jumpHeight, jumpTime);

    // Set variable jump height for better control
    component->setVariableJumpHeight(jumpHeight * 0.5f);

    // Set coyote time for better jump timing
    component->setCoyoteTime(0.1f);

    // Set jump buffer for better responsiveness
    component->setJumpBufferTime(0.1f);

    // Set movement parameters
    float acceleration = maxSpeed * 4.0f; // Quick acceleration
    float deceleration = maxSpeed * 2.0f; // Responsive deceleration
    component->setMovementParameters(maxSpeed, acceleration, deceleration);

    // Set wall jump parameters
    component->setWallJumpParameters(0.3f, maxSpeed, 45.0f);

    // Lock rotation for better control
    component->setCanRotate(false);
}

void PhysicsSystem::getCollisionStats(int& outTotalTests, int& outActiveCollisions) const
{
    outTotalTests = m_totalCollisionTests;
    outActiveCollisions = m_activeCollisions;
}

// Private helper methods

std::vector<glm::ivec3> PhysicsSystem::getGridCellsForAABB(const glm::vec3& min, const glm::vec3& max) const
{
    std::vector<glm::ivec3> cells;

    // Convert min/max to cell indices
    glm::ivec3 minCell = worldToGridPos(min);
    glm::ivec3 maxCell = worldToGridPos(max);

    // Iterate over all cells that the AABB might touch
    for (int x = minCell.x; x <= maxCell.x; x++)
    {
        for (int y = minCell.y; y <= maxCell.y; y++)
        {
            for (int z = minCell.z; z <= maxCell.z; z++)
            {
                cells.push_back(glm::ivec3(x, y, z));
            }
        }
    }

    return cells;
}

glm::ivec3 PhysicsSystem::worldToGridPos(const glm::vec3& worldPos) const
{
    return glm::ivec3(
        static_cast<int>(worldPos.x / m_worldCellSize),
        static_cast<int>(worldPos.y / m_worldCellSize),
        static_cast<int>(worldPos.z / m_worldCellSize)
    );
}

void PhysicsSystem::updateTransforms()
{
    // Update entity transforms from physics component state
    for (auto component : m_components)
    {
        Entity* entity = component->getEntity();
        if (!entity)
            continue;

        // Only update for kinematic and dynamic bodies
        if (component->getBodyType() == PhysicsBodyType::STATIC)
            continue;

        // Get current position/rotation
        glm::vec3 position = entity->getPosition();
        glm::quat rotation = entity->getRotation();

        // Only update if necessary
        bool positionChanged = false;
        bool rotationChanged = false;

        // Update entity position from physics position
        if (component->getBodyType() == PhysicsBodyType::KINEMATIC)
        {
            // For kinematic bodies, we might want custom movement logic
            // This would be game-specific
        }
        else if (component->getBodyType() == PhysicsBodyType::DYNAMIC)
        {
            // For dynamic bodies, entity follows physics
            // This is handled in integratePositions()
        }

        // Update entity transform if needed
        if (positionChanged)
        {
            entity->setPosition(position);
        }

        if (rotationChanged && component->canRotate())
        {
            entity->setRotation(rotation);
        }
    }
}

void PhysicsSystem::integrateVelocities(float deltaTime)
{
    // Apply forces and update velocities
    for (auto component : m_components)
    {
        // Skip if not dynamic or sleeping
        if (component->getBodyType() != PhysicsBodyType::DYNAMIC || component->isSleeping())
            continue;

        // Apply gravity
        if (component->getUseGravity())
        {
            if (component->isPlatformerPhysics() && component->isWallSliding())
            {
                // Reduced gravity when wall sliding
                component->applyForce(m_gravity * component->getMass() * component->m_wallSlideGravityScale);
            }
            else if (!component->isGrabbingLedge())
            {
                // Full gravity
                component->applyForce(m_gravity * component->getMass());
            }
        }

        // Skip velocity integration if mass is zero (static)
        if (component->getMass() <= 0.0f)
            continue;

        // Apply forces to update velocity
        glm::vec3 acceleration = component->m_totalForce / component->getMass();
        component->m_linearVelocity += acceleration * deltaTime;

        // Apply damping
        float damping = powf(1.0f - component->getLinearDamping(), deltaTime);
        component->m_linearVelocity *= damping;

        // Apply angular forces if rotation is enabled
        if (component->canRotate())
        {
            // Note: This is simplified, proper angular velocity would use
            // moment of inertia and rigid body dynamics
            component->m_angularVelocity += component->m_totalTorque * deltaTime;

            // Apply angular damping
            component->m_angularVelocity *= damping;
        }

        // Apply axis locks
        if (component->m_lockAxisX) component->m_linearVelocity.x = 0.0f;
        if (component->m_lockAxisY) component->m_linearVelocity.y = 0.0f;
        if (component->m_lockAxisZ) component->m_linearVelocity.z = 0.0f;
    }
}

void PhysicsSystem::detectCollisions()
{
    // Clear current collision list
    m_currentCollisions.clear();

    // Use special partitioning to find potential collision pairs
    // For simplicity, we'll check all pairs for now

    // For each component
    for (size_t i = 0; i < m_components.size(); i++)
    {
        auto compA = m_components[i];

        // Skip if not active or no colliders
        if (!compA->getEntity() || !compA->getEntity()->isActive() || compA->getColliders().empty())
            continue;

        // Check against all other components
        for (size_t j = i + 1; j < m_components.size(); j++)
        {
            auto compB = m_components[j];

            // Skip if not active or no colliders
            if (!compB->getEntity() || !compB->getEntity()->isActive() || compB->getColliders().empty())
                continue;

            // Check if layers collide
            if (!getCollisionEnabled(compA->getCollisionLayer(), compB->getCollisionLayer()))
                continue;

            // Check if masks allow collision
            if (!(compA->getCollisionMask() & (1 << compB->getCollisionMask())) ||
                !(compB->getCollisionMask() & (1 << compA->getCollisionMask())))
                continue;

            // Increment collision test counter
            m_totalCollisionTests++;

            // Broad phase: bounding box test for quick rejection
            // Note: In a real system, this would be handled by spatial
            // partitioning for better performance
            glm::vec3 aMin, aMax, bMin, bMax;
            bool aHasBounds = false, bHasBounds = false;

            // Find bounds for all colliders on compA
            for (auto colliderA : compA->getColliders())
            {
                glm::vec3 colliderMin, colliderMax;
                colliderA->getWorldBounds(colliderMin, colliderMax);

                if (!aHasBounds)
                {
                    aMin = colliderMin;
                    aMax = colliderMax;
                    aHasBounds = true;
                }
                else
                {
                    aMin = glm::min(aMin, colliderMin);
                    aMax = glm::max(aMax, colliderMax);
                }
            }

            // Find bounds for all colliders on compB
            for (auto colliderB : compB->getColliders())
            {
                glm::vec3 colliderMin, colliderMax;
                colliderB->getWorldBounds(colliderMin, colliderMax);

                if (!aHasBounds)
                {
                    bMin = colliderMin;
                    bMax = colliderMax;
                    bHasBounds = true;
                }
                else
                {
                    bMin = glm::min(bMin, colliderMin);
                    bMax = glm::max(bMax, colliderMax);
                }
            }

            // Check if AABBs overlap
            if (aHasBounds && bHasBounds)
            {
                if (aMax.x < bMin.x || aMin.x > bMax.x ||
                    aMax.y < bMin.y || aMin.y > bMax.y ||
                    aMax.z < bMin.z || aMin.z > bMax.z)
                {
                    // No collision, AABBs don't overlap
                    continue;
                }
            }

            // Narrow phase: check collisions between individual colliders
            bool collisionDetected = false;
            CollisionInfo collisionInfo;

            for (auto colliderA : compA->getColliders())
            {
                for (auto colliderB : compB->getColliders())
                {
                    // Check if either is a trigger
                    bool isTrigger = colliderA->isTrigger() || colliderB->isTrigger();

                    // Check collision between colliders
                    if (checkCollision(colliderA, colliderB, collisionInfo))
                    {
                        // Set collision components
                        collisionInfo.componentA = compA;
                        collisionInfo.componentB = compB;
                        collisionInfo.colliderA = colliderA;
                        collisionInfo.colliderB = colliderB;
                        collisionInfo.isTrigger = isTrigger;

                        // Add to collision list for resolution
                        m_currentCollisions.push_back(collisionInfo);
                        collisionDetected = true;

                        // Update collision pair tracking
                        updateCollisionPair(compA, compB, isTrigger);

                        // Increment active collision counter
                        m_activeCollisions++;

                        // Notify via global callback if set
                        if (m_globalCollisionCallback)
                        {
                            m_globalCollisionCallback(collisionInfo);
                        }

                        // For triggers, call trigger callbacks
                        if (isTrigger)
                        {
                            // This will be handled in updateTriggers() to ensure
                            // proper enter/exit event ordering
                        }
                        else
                        {
                            // For non-triggers, notify components of collision
                            compA->handleCollision(compB, collisionInfo);

                            // Swap A and B in collisionInfo
                            std::swap(collisionInfo.componentA, collisionInfo.componentB);
                            std::swap(collisionInfo.colliderA, collisionInfo.colliderB);
                            collisionInfo.normal = -collisionInfo.normal; // Flip normal

                            compB->handleCollision(compA, collisionInfo);
                        }
                    }
                }
            }
        }
    }

    // Check for collisions with voxel grid
    for (auto component : m_components)
    {
        // Skip if not active or no colliders
        if (!component->getEntity() || !component->getEntity()->isActive() || component->getColliders().empty())
            continue;

        // Skip for static and trigger components (they don't collide with voxels)
        if (component->getBodyType() == PhysicsBodyType::STATIC ||
            component->getColliders()[0]->isTrigger())
            continue;

        // Check for collisions with voxel grid
        checkVoxelGridCollisions(component);
    }
}

void PhysicsSystem::checkVoxelGridCollisions(PhysicsComponent* component)
{
    // Skip if no voxel grid
    if (!m_cubeGrid)
        return;

    // For each collider
    for (auto collider : component->getColliders())
    {
        // Get collider bounds
        glm::vec3 min, max;
        collider->getWorldBounds(min, max);

        // Convert to grid coordinates
        glm::ivec3 minGrid = m_cubeGrid->worldToGridCoordinates(min);
        glm::ivec3 maxGrid = m_cubeGrid->worldToGridCoordinates(max);

        // Expand by 1 to handle edge cases
        minGrid -= glm::ivec3(1);
        maxGrid += glm::ivec3(1);

        // Iterate through all potential grid cells
        for (int x = minGrid.x; x <= maxGrid.x; x++)
        {
            for (int y = minGrid.y; y <= maxGrid.y; y++)
            {
                for (int z = minGrid.z; z <= maxGrid.z; z++)
                {
                    // Check if cube is active
                    if (m_cubeGrid->isCubeActive(x, y, z))
                    {
                        // For simplicity, treat each voxel as a box collider
                        glm::vec3 voxelPos = m_cubeGrid->gridToWorldPosition(x, y, z);
                        float voxelSize = m_cubeGrid->getSpacing();

                        // Simplified collision for demonstration
                        // In a real system, proper shape-voxel collision would be used

                        if (collider->getType() == ColliderType::SPHERE)
                        {
                            SphereCollider* sphere = static_cast<SphereCollider*>(collider);

                            // Get sphere center
                            glm::mat4 worldTransform = collider->getWorldTransform();
                            glm::vec4 centerVec4 = worldTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                            glm::vec3 center = glm::vec3(centerVec4) / centerVec4.w;

                            // Check sphere-box collision
                            glm::vec3 voxelMin = voxelPos - glm::vec3(voxelSize / 2.0f);
                            glm::vec3 voxelMax = voxelPos + glm::vec3(voxelSize / 2.0f);

                            // Find closest point on voxel to sphere center
                            glm::vec3 closest;
                            closest.x = glm::max(voxelMin.x, glm::min(center.x, voxelMax.x));
                            closest.y = glm::max(voxelMin.y, glm::min(center.y, voxelMax.y));
                            closest.z = glm::max(voxelMin.z, glm::min(center.z, voxelMax.z));

                            // Check distance
                            float distSquared = glm::distance2(center, closest);
                            if (distSquared <= sphere->getRadius() * sphere->getRadius())
                            {
                                // Collision detected, create collision info
                                CollisionInfo info;
                                info.componentA = component;
                                info.componentB = nullptr; // No component for voxel
                                info.colliderA = collider;
                                info.colliderB = nullptr; // No collider for voxel
                                info.isTrigger = false;

                                // Calculate contact point and normal
                                float dist = sqrtf(distSquared);
                                if (dist < 0.00001f)
                                {
                                    // Sphere center is exactly on the voxel surface
                                    // Use any direction as normal
                                    info.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                                }
                                else
                                {
                                    info.normal = (center - closest) / dist;
                                }

                                info.contactPoint = closest;
                                info.penetrationDepth = sphere->getRadius() - dist;

                                // Add to collision list
                                m_currentCollisions.push_back(info);

                                // Increment active collision counter
                                m_activeCollisions++;

                                // Notify component
                                component->handleCollision(nullptr, info);
                            }
                        }
                        else if (collider->getType() == ColliderType::BOX)
                        {
                            // Box-voxel collision would be implemented here
                            // For brevity, we're skipping the implementation details
                        }
                        // Other collider types would be handled similarly
                    }
                }
            }
        }
    }
}

void PhysicsSystem::resolveCollisions()
{
    // For each collision
    for (auto& collision : m_currentCollisions)
    {
        // Skip triggers - they don't have physical collision response
        if (collision.isTrigger)
            continue;

        // Get components
        PhysicsComponent* compA = collision.componentA;
        PhysicsComponent* compB = collision.componentB;

        // Handle voxel grid collision (compB is null)
        if (!compB)
        {
            resolveVoxelCollisions(compA, collision);
            continue;
        }

        // Skip if either component is static
        if (compA->getBodyType() == PhysicsBodyType::STATIC &&
            compB->getBodyType() == PhysicsBodyType::STATIC)
            continue;

        // Resolve collision
        resolveContactConstraint(collision, m_fixedTimeStep);
    }
}

void PhysicsSystem::resolveVoxelCollisions(PhysicsComponent* component, const CollisionInfo& collision)
{
    // Skip if the component is static or kinematic
    if (component->getBodyType() != PhysicsBodyType::DYNAMIC)
        return;

    // Apply positional correction to keep object out of the voxel
    Entity* entity = component->getEntity();
    if (!entity)
        return;

    // Position correction - move out of collision
    float positionCorrection = 0.2f; // Correction factor
    glm::vec3 correction = collision.normal * collision.penetrationDepth * positionCorrection;
    entity->setPosition(entity->getPosition() + correction);

    // Velocity correction
    float restitution = component->getRestitution();
    glm::vec3 velocity = component->getLinearVelocity();

    // Calculate new velocity based on reflection and friction
    float normalVelocity = glm::dot(velocity, collision.normal);

    // Only apply if moving toward the voxel
    if (normalVelocity < 0.0f)
    {
        // Reflection component
        glm::vec3 newVelocity = velocity - (1.0f + restitution) * normalVelocity * collision.normal;

        // Friction component
        glm::vec3 tangent = newVelocity - glm::dot(newVelocity, collision.normal) * collision.normal;
        float tangentLength = glm::length(tangent);

        if (tangentLength > 0.0001f)
        {
            tangent = tangent / tangentLength;
            float frictionImpulse = -glm::dot(newVelocity, tangent) * component->getFriction();

            // Apply friction
            newVelocity += tangent * frictionImpulse;
        }

        // Update velocity
        component->setLinearVelocity(newVelocity);

        // Update grounded state for platformer physics
        if (component->isPlatformerPhysics() && collision.normal.y > 0.7f)
        {
            component->m_isGrounded = true;
        }
    }
}

void PhysicsSystem::resolveContactConstraint(CollisionInfo& info, float deltaTime)
{
    // Get components
    PhysicsComponent* compA = info.componentA;
    PhysicsComponent* compB = info.componentB;

    // Skip if both are static
    if ((compA->getBodyType() == PhysicsBodyType::STATIC || compA->getMass() <= 0.0f) &&
        (compB->getBodyType() == PhysicsBodyType::STATIC || compB->getMass() <= 0.0f))
        return;

    // For platformer physics, special handling for certain collisions
    bool isPlatformerA = compA->isPlatformerPhysics();
    bool isPlatformerB = compB->isPlatformerPhysics();

    // Update grounded state for platformer physics
    if (isPlatformerA && info.normal.y > 0.7f)
    {
        compA->m_isGrounded = true;
    }
    if (isPlatformerB && info.normal.y < -0.7f)
    {
        compB->m_isGrounded = true;
    }

    // Position correction
    resolvePositionalConstraint(info);

    // Prepare data for impulse calculation
    glm::vec3 rA = info.contactPoint - compA->getEntity()->getWorldPosition();
    glm::vec3 rB = info.contactPoint - compB->getEntity()->getWorldPosition();

    glm::vec3 vA = compA->getLinearVelocity();
    glm::vec3 vB = compB->getLinearVelocity();

    glm::vec3 wA = compA->getAngularVelocity();
    glm::vec3 wB = compB->getAngularVelocity();

    // Calculate relative velocity
    glm::vec3 relativeVelocity = (vB + glm::cross(wB, rB)) - (vA + glm::cross(wA, rA));

    // Calculate normal component of relative velocity
    float normalVelocity = glm::dot(relativeVelocity, info.normal);

    // If objects are moving apart, skip impulse calculation
    if (normalVelocity > 0.0f)
        return;

    // Calculate restitution (bounciness)
    float e = glm::min(compA->getRestitution(), compB->getRestitution());

    // For low velocities, don't apply restitution to prevent jitter
    if (fabsf(normalVelocity) < 0.2f)
        e = 0.0f;

    // Calculate impulse scalar
    float j = -(1.0f + e) * normalVelocity;
    float invMassA = (compA->getBodyType() == PhysicsBodyType::DYNAMIC) ? 1.0f / compA->getMass() : 0.0f;
    float invMassB = (compB->getBodyType() == PhysicsBodyType::DYNAMIC) ? 1.0f / compB->getMass() : 0.0f;
    j /= invMassA + invMassB;

    // Apply impulse
    glm::vec3 impulse = j * info.normal;

    if (compA->getBodyType() == PhysicsBodyType::DYNAMIC)
    {
        compA->setLinearVelocity(vA - impulse * invMassA);
        if (compA->canRotate())
        {
            // Angular impulse would be computed here
            // This is a simplified version
            compA->setAngularVelocity(wA - glm::cross(rA, impulse * invMassA));
        }
    }

    if (compB->getBodyType() == PhysicsBodyType::DYNAMIC)
    {
        compB->setLinearVelocity(vB + impulse * invMassB);
        if (compB->canRotate())
        {
            // Angular impulse would be computed here
            // This is a simplified version
            compB->setAngularVelocity(wB + glm::cross(rB, impulse * invMassB));
        }
    }

    // Friction impulse
    relativeVelocity = (vB + glm::cross(wB, rB)) - (vA + glm::cross(wA, rA));
    glm::vec3 tangent = relativeVelocity - (glm::dot(relativeVelocity, info.normal) * info.normal);
    float tangentLength = glm::length(tangent);

    if (tangentLength > 0.00001f)
    {
        tangent = tangent / tangentLength;

        // Calculate friction impulse scalar
        float jt = -glm::dot(relativeVelocity, tangent);
        jt /= invMassA + invMassB;

        // Calculate friction coefficient
        float mu = sqrtf(compA->getFriction() * compB->getFriction());

        // Clamp friction impulse
        glm::vec3 frictionImpulse;
        if (fabsf(jt) < j * mu)
        {
            frictionImpulse = jt * tangent;
        }
        else
        {
            frictionImpulse = -j * mu * tangent;
        }

        // Apply friction impulse
        if (compA->getBodyType() == PhysicsBodyType::DYNAMIC)
        {
            compA->setLinearVelocity(compA->getLinearVelocity() - frictionImpulse * invMassA);
        }

        if (compB->getBodyType() == PhysicsBodyType::DYNAMIC)
        {
            compB->setLinearVelocity(compB->getLinearVelocity() - frictionImpulse * invMassB);
        }
    }
}

void PhysicsSystem::resolvePositionalConstraint(CollisionInfo& info)
{
    // Get components
    PhysicsComponent* compA = info.componentA;
    PhysicsComponent* compB = info.componentB;

    // Skip if both are static
    if ((compA->getBodyType() == PhysicsBodyType::STATIC || compA->getMass() <= 0.0f) &&
        (compB->getBodyType() == PhysicsBodyType::STATIC || compB->getMass() <= 0.0f))
        return;

    // Positional correction factor
    float percent = 0.2f; // Typically 0.2 to 0.8
    float slop = 0.01f;   // Small penetration allowed

    // Calculate correction magnitude
    float penetration = glm::max(info.penetrationDepth - slop, 0.0f) * percent;

    // Calculate mass factors
    float invMassA = (compA->getBodyType() == PhysicsBodyType::DYNAMIC) ? 1.0f / compA->getMass() : 0.0f;
    float invMassB = (compB->getBodyType() == PhysicsBodyType::DYNAMIC) ? 1.0f / compB->getMass() : 0.0f;
    float totalInvMass = invMassA + invMassB;

    if (totalInvMass < 0.00001f)
        return;

    // Calculate correction vectors
    glm::vec3 correction = (penetration / totalInvMass) * info.normal;

    // Apply correction to entity positions
    if (compA->getBodyType() == PhysicsBodyType::DYNAMIC)
    {
        Entity* entityA = compA->getEntity();
        if (entityA)
        {
            entityA->setPosition(entityA->getPosition() - correction * invMassA);
        }
    }

    if (compB->getBodyType() == PhysicsBodyType::DYNAMIC)
    {
        Entity* entityB = compB->getEntity();
        if (entityB)
        {
            entityB->setPosition(entityB->getPosition() + correction * invMassB);
        }
    }
}

void PhysicsSystem::integratePositions(float deltaTime)
{
    // Update positions based on velocities
    for (auto component : m_components)
    {
        // Skip if not dynamic or sleeping
        if (component->getBodyType() != PhysicsBodyType::DYNAMIC || component->isSleeping())
            continue;

        // Get entity
        Entity* entity = component->getEntity();
        if (!entity)
            continue;

        // Update position
        entity->setPosition(entity->getPosition() + component->getLinearVelocity() * deltaTime);

        // Update rotation if enabled
        if (component->canRotate())
        {
            // Convert angular velocity to quaternion change
            glm::vec3 angularVelocity = component->getAngularVelocity();
            float angularSpeed = glm::length(angularVelocity);

            if (angularSpeed > 0.0001f)
            {
                glm::vec3 rotationAxis = angularVelocity / angularSpeed;
                float rotationAngle = angularSpeed * deltaTime;

                glm::quat rotationDelta = glm::angleAxis(rotationAngle, rotationAxis);
                entity->setRotation(rotationDelta * entity->getRotation());
            }
        }
    }
}

void PhysicsSystem::updateTriggers()
{
    // Update trigger enter/exit events
    for (auto& pair : m_collisionPairs)
    {
        if (pair.isTrigger)
        {
            // If this pair wasn't in contact before, trigger enter event
            if (!pair.inContact)
            {
                pair.inContact = true;

                // Call component callbacks
                if (pair.a && pair.a->m_triggerEnterCallback)
                {
                    pair.a->m_triggerEnterCallback(pair.b);
                }

                if (pair.b && pair.b->m_triggerEnterCallback)
                {
                    pair.b->m_triggerEnterCallback(pair.a);
                }

                // Call global callback
                if (m_globalTriggerEnterCallback)
                {
                    m_globalTriggerEnterCallback(pair.a, pair.b);
                }
            }
        }
    }

    // Check for trigger exits
    for (auto it = m_collisionPairs.begin(); it != m_collisionPairs.end(); )
    {
        if (it->isTrigger && it->inContact)
        {
            // Check if the pair is still colliding
            bool stillColliding = false;

            for (const auto& collision : m_currentCollisions)
            {
                if ((collision.componentA == it->a && collision.componentA == it->b) ||
                    (collision.componentA == it->b && collision.componentB == it->a))
                {
                    stillColliding = true;
                    break;
                }
            }

            // If no longer colliding, trigger exit event
            if (!stillColliding)
            {
                // Call component callbacks
                if (it->a && it->a->m_triggerExitCallback)
                {
                    it->a->m_triggerExitCallback(it->b);
                }

                if (it->b && it->b->m_triggerExitCallback)
                {
                    it->b->m_triggerExitCallback(it->a);
                }

                // Call global callback
                if (m_globalTriggerExitCallback)
                {
                    m_globalTriggerExitCallback(it->a, it->b);
                }

                // Remove from active pairs
                it = m_collisionPairs.erase(it);
                continue;
            }
        }

        it++;
    }
}

void PhysicsSystem::updateGridPositions()
{
    // Update spatial partitioning grid
    for (auto component : m_components)
    {
        updateComponentGridPosition(component);
    }
}

void PhysicsSystem::updateComponentGridPosition(PhysicsComponent* component)
{
    if (!component || !component->getEntity())
        return;

    // Remove from old grid position
    removeComponentFromGrid(component);

    // Get entity position
    glm::vec3 position = component->getEntity()->getWorldPosition();

    // Convert to grid position
    glm::ivec3 gridPos = worldToGridPos(position);

    // Ensure valid grid position
    if (gridPos.x >= 0 && gridPos.y >= 0 && gridPos.z >= 0 &&
        gridPos.x < m_worldGrid.size() &&
        gridPos.y < m_worldGrid[0].size() &&
        gridPos.z < m_worldGrid[0][0].size())
    {
        // Add to grid
        m_worldGrid[gridPos.x][gridPos.y][gridPos.z].push_back(component);

        // Update component grid position
        m_componentGridPos[component] = gridPos;
    }
}

void PhysicsSystem::removeComponentFromGrid(PhysicsComponent* component)
{
    // Check if component has a grid position
    auto it = m_componentGridPos.find(component);
    if (it != m_componentGridPos.end())
    {
        glm::ivec3 gridPos = it->second;

        // Ensure valid grid position
        if (gridPos.x >= 0 && gridPos.y >= 0 && gridPos.z >= 0 &&
            gridPos.x < m_worldGrid.size() &&
            gridPos.y < m_worldGrid[0].size() &&
            gridPos.z < m_worldGrid[0][0].size())
        {
            // Remove from grid
            auto& cell = m_worldGrid[gridPos.x][gridPos.y][gridPos.z];
            auto compIt = std::find(cell.begin(), cell.end(), component);
            if (compIt != cell.end())
            {
                cell.erase(compIt);
            }
        }

        // Remove from map
        m_componentGridPos.erase(it);
    }
}

void PhysicsSystem::updateCollisionPair(PhysicsComponent* a, PhysicsComponent* b, bool isTrigger)
{
    // Check if pair already exists
    for (auto& pair : m_collisionPairs)
    {
        if ((pair.a == a && pair.b == b) || (pair.a == b && pair.b == a))
        {
            // Update existing pair
            pair.inContact = true;
            return;
        }
    }

    // Add new pair
    CollisionPair newPair;
    newPair.a = a;
    newPair.b = b;
    newPair.inContact = true;
    newPair.isTrigger = isTrigger;

    m_collisionPairs.push_back(newPair);
}

void PhysicsSystem::removeComponentFromCollisionPairs(PhysicsComponent* component)
{
    // Remove any collision pairs involving this component
    m_collisionPairs.erase(
        std::remove_if(m_collisionPairs.begin(), m_collisionPairs.end(),
                       [component](const CollisionPair& pair) {
                           return pair.a == component || pair.b == component;
                       }),
        m_collisionPairs.end());
}

bool PhysicsSystem::raycastVoxels(const Ray& ray, RaycastHit& hitInfo)
{
    // Skip if no voxel grid
    if (!m_cubeGrid)
        return false;

    // Implementation of ray-voxel grid intersection
    // This is a simplified version using the Digital Differential Analyser (DDA) algorithm

    // Convert ray to grid coordinates
    glm::vec3 gridOrigin = ray.origin;
    glm::vec3 gridDirection = ray.direction;

    // Initialize voxel coordinates
    glm::ivec3 mapPos = m_cubeGrid->worldToGridCoordinates(gridOrigin);

    // Calculate ray delta and step direction
    glm::vec3 deltaDist;
    deltaDist.x = (gridDirection.x == 0.0f) ? FLT_MAX : fabs(1.0f / gridDirection.x);
    deltaDist.y = (gridDirection.y == 0.0f) ? FLT_MAX : fabs(1.0f / gridDirection.y);
    deltaDist.z = (gridDirection.z == 0.0f) ? FLT_MAX : fabs(1.0f / gridDirection.z);

    glm::ivec3 step;
    glm::vec3 sideDist;

    // Calculate step direction and initial sideDist
    if (gridDirection.x < 0)
    {
        step.x = -1;
        sideDist.x = (gridOrigin.x - mapPos.x) * deltaDist.x;
    }
    else
    {
        step.x = 1;
        sideDist.x = (mapPos.x + 1.0f - gridOrigin.x) * deltaDist.x;
    }

    if (gridDirection.y < 0)
    {
        step.y = -1;
        sideDist.y = (gridOrigin.y - mapPos.y) * deltaDist.y;
    }
    else
    {
        step.y = 1;
        sideDist.y = (mapPos.y + 1.0f - gridOrigin.y) * deltaDist.y;
    }

    if (gridDirection.z < 0)
    {
        step.z = -1;
        sideDist.z = (gridOrigin.z - mapPos.z) * deltaDist.z;
    }
    else
    {
        step.z = 1;
        sideDist.z = (mapPos.z + 1.0f - gridOrigin.z) * deltaDist.z;
    }

    // DDA algorithm
    bool hit = false;
    int side;
    float distance = 0.0f;

    while (!hit && distance < ray.maxDistance)
    {
        // Jump to next voxel
        if (sideDist.x < sideDist.y && sideDist.x < sideDist.z)
        {
            sideDist.x += deltaDist.x;
            mapPos.x += step.x;
            side = 0;
        }
        else if (sideDist.y < sideDist.z)
        {
            sideDist.y += deltaDist.y;
            mapPos.y += step.y;
            side = 1;
        }
        else
        {
            sideDist.z += deltaDist.z;
            mapPos.z += step.z;
            side = 2;
        }

        // Check if the current voxel is active
        if (m_cubeGrid->isCubeActive(mapPos.x, mapPos.y, mapPos.z))
        {
            hit = true;
        }
    }

    // If we hit a voxel, calculate hit info
    if (hit)
    {
        // Calculate distance
        if (side == 0)
        {
            distance = (mapPos.x - gridOrigin.x + (1 - step.x) / 2) / gridDirection.x;
        }
        else if (side == 1)
        {
            distance = (mapPos.y - gridOrigin.y + (1 - step.y) / 2) / gridDirection.y;
        }
        else
        {
            distance = (mapPos.z - gridOrigin.z + (1 - step.z) / 2) / gridDirection.z;
        }

        // Calculate hit point
        glm::vec3 hitPoint = gridOrigin + gridDirection * distance;

        // Calculate normal
        glm::vec3 normal(0.0f);
        if (side == 0) normal.x = -step.x;
        if (side == 1) normal.y = -step.y;
        if (side == 2) normal.z = -step.z;

        // Fill hit info
        hitInfo.hasHit = true;
        hitInfo.distance = distance;
        hitInfo.point = hitPoint;
        hitInfo.normal = normal;
        hitInfo.hitComponent = nullptr; // No component for voxel
        hitInfo.hitCollider = nullptr;  // No collider for voxel

        return true;
    }

    return false;
}

bool PhysicsSystem::checkCollision(PhysicsComponent* a, PhysicsComponent* b, CollisionInfo& info)
{
    // Check collision between any collider in component A and any in component B
    for (auto colliderA : a->getColliders())
    {
        for (auto colliderB : b->getColliders())
        {
            if (checkCollision(colliderA, colliderB, info))
            {
                info.componentA = a;
                info.componentB = b;
                return true;
            }
        }
    }

    return false;
}

bool PhysicsSystem::checkCollision(Collider* a, Collider* b, CollisionInfo& info)
{
    // Dispatch to appropriate collision detection function based on collider types
    if (a->getType() == ColliderType::SPHERE && b->getType() == ColliderType::SPHERE)
    {
        return sphereVsSphere(static_cast<SphereCollider*>(a), static_cast<SphereCollider*>(b), info);
    }
    else if (a->getType() == ColliderType::SPHERE && b->getType() == ColliderType::BOX)
    {
        return sphereVsBox(static_cast<SphereCollider*>(a), static_cast<BoxCollider*>(b), info);
    }
    else if (a->getType() == ColliderType::BOX && b->getType() == ColliderType::SPHERE)
    {
        // Swap A and B
        bool result = sphereVsBox(static_cast<SphereCollider*>(b), static_cast<BoxCollider*>(a), info);
        if (result)
        {
            // Flip normal direction
            info.normal = -info.normal;
        }
        return result;
    }
    else if (a->getType() == ColliderType::BOX && b->getType() == ColliderType::BOX)
    {
        return boxVsBox(static_cast<BoxCollider*>(a), static_cast<BoxCollider*>(b), info);
    }
    // Add other collider type combinations as needed

    return false;
}

bool PhysicsSystem::sphereVsSphere(const SphereCollider* a, const SphereCollider* b, CollisionInfo& info)
{
    // Get sphere centers in world space
    glm::mat4 transformA = a->getWorldTransform();
    glm::mat4 transformB = b->getWorldTransform();

    glm::vec4 centerA_vec4 = transformA * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 centerB_vec4 = transformB * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    glm::vec3 centerA = glm::vec3(centerA_vec4) / centerA_vec4.w;
    glm::vec3 centerB = glm::vec3(centerB_vec4) / centerB_vec4.w;

    // Get sphere radii
    float radiusA = a->getRadius();
    float radiusB = b->getRadius();

    // Calculate distance between centers
    glm::vec3 direction = centerB - centerA;
    float distance = glm::length(direction);

    // Check if spheres are colliding
    float radiusSum = radiusA + radiusB;
    if (distance >= radiusSum)
    {
        return false; // No collision
    }

    // Normalize direction
    glm::vec3 normal;
    if (distance > 0.0001f)
    {
        normal = direction / distance;
    }
    else
    {
        // Centers are too close, use any direction
        normal = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    // Fill collision info
    info.contactPoint = centerA + normal * radiusA;
    info.normal = normal;
    info.penetrationDepth = radiusSum - distance;
    info.colliderA = const_cast<SphereCollider*>(a);
    info.colliderB = const_cast<SphereCollider*>(b);

    return true;
}

bool PhysicsSystem::sphereVsBox(const SphereCollider* sphere, const BoxCollider* box, CollisionInfo& info)
{
    // Get transforms
    glm::mat4 sphereTransform = sphere->getWorldTransform();
    glm::mat4 boxTransform = box->getWorldTransform();

    // Get sphere center in world space
    glm::vec4 center_vec4 = sphereTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec3 center = glm::vec3(center_vec4) / center_vec4.w;

    // Get sphere center in box space
    glm::mat4 boxInvTransform = glm::inverse(boxTransform);
    glm::vec4 localCenter_vec4 = boxInvTransform * glm::vec4(center, 1.0f);
    glm::vec3 localCenter = glm::vec3(localCenter_vec4) / localCenter_vec4.w;

    // Get box half size
    glm::vec3 halfSize = box->getSize() * 0.5f;

    // Find closest point on box to sphere center in box space
    glm::vec3 closestPoint;
    closestPoint.x = glm::clamp(localCenter.x, -halfSize.x, halfSize.x);
    closestPoint.y = glm::clamp(localCenter.y, -halfSize.y, halfSize.y);
    closestPoint.z = glm::clamp(localCenter.z, -halfSize.z, halfSize.z);

    // Check if closest point is inside sphere
    float distSquared = glm::distance2(localCenter, closestPoint);
    float radiusSquared = sphere->getRadius() * sphere->getRadius();

    if (distSquared > radiusSquared)
    {
        return false; // No collision
    }

    // Transform closest point back to world space
    glm::vec4 worldClosest_vec4 = boxTransform * glm::vec4(closestPoint, 1.0f);
    glm::vec3 worldClosest = glm::vec3(worldClosest_vec4) / worldClosest_vec4.w;

    // Calculate normal and penetration
    glm::vec3 normal = center - worldClosest;
    float distance = glm::length(normal);

    if (distance > 0.0001f)
    {
        normal /= distance;
    }
    else
    {
        // Inside the box, use direction to closest face
        glm::vec3 distToFace;
        distToFace.x = halfSize.x - fabs(localCenter.x);
        distToFace.y = halfSize.y - fabs(localCenter.y);
        distToFace.z = halfSize.z - fabs(localCenter.z);

        // find closest face
        if (distToFace.x <= distToFace.y && distToFace.x <= distToFace.z)
        {
            normal = glm::vec3(localCenter.x > 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f);
        }
        else if (distToFace.y <= distToFace.z)
        {
            normal = glm::vec3(0.0f, localCenter.y > 0.0f ? 1.0f : -1.0f, 0.0f);
        }
        else
        {
            normal = glm::vec3(0.0f, 0.0f, localCenter.z > 0.0f ? 1.0f : -1.0f);
        }

        // Transform normal to world space
        glm::vec4 worldNormal_vec4 = boxTransform * glm::vec4(normal, 0.0f);
        normal = glm::normalize(glm::vec3(worldNormal_vec4));
    }

    // Fill collision info
    info.contactPoint = worldClosest;
    info.normal = normal;
    info.penetrationDepth = sphere->getRadius() - distance;
    info.colliderA = const_cast<SphereCollider*>(sphere);
    info.colliderB = const_cast<BoxCollider*>(box);

    return true;
}

bool PhysicsSystem::boxVsBox(const BoxCollider* a, const BoxCollider* b, CollisionInfo& info)
{
    // This is a simplified box vs box test
    // A complete implementation would use the Separating Axis Theorem (SAT)
    // and handle rotated boxes properly

    // Get world bounds
    glm::vec3 minA, maxA, minB, maxB;
    a->getWorldBounds(minA, maxA);
    b->getWorldBounds(minB, maxB);

    // Check for overlap
    if (maxA.x < minB.x || minA.x > maxB.x ||
        maxA.y < minB.y || minA.y > maxB.y ||
        maxA.z < minB.z || minA.z > maxB.z)
    {
        return false; // No overlap
    }

    // Calculate overlap on each axis
    glm::vec3 overlap;
    overlap.x = glm::min(maxA.x, maxB.x) - glm::max(minA.x, minB.x);
    overlap.y = glm::min(maxA.y, maxB.y) - glm::max(minA.y, minB.y);
    overlap.z = glm::min(maxA.z, maxB.z) - glm::max(minA.z, minB.z);

    // Find minimum overlap axis
    glm::vec3 normal(0.0f);
    float penetration = FLT_MAX;

    if (overlap.x < overlap.y && overlap.x < overlap.z)
    {
        penetration = overlap.x;
        // Determine normal direction based on centers
        glm::vec3 centerA = (minA + maxA) * 0.5f;
        glm::vec3 centerB = (minB + maxB) * 0.5f;

        if (centerA.x < centerB.x)
        {
            normal = glm::vec3(-1.0f, 0.0f, 0.0f);
        }
        else
        {
            normal = glm::vec3(1.0f, 0.0f, 0.0f);
        }
    }
    else if (overlap.y < overlap.z)
    {
        penetration = overlap.y;
        // Determine normal direction based on centers
        glm::vec3 centerA = (minA + maxA) * 0.5f;
        glm::vec3 centerB = (minB + maxB) * 0.5f;

        if (centerA.y < centerB.y)
        {
            normal = glm::vec3(0.0f, -1.0f, 0.0f);
        }
        else
        {
            normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }
    else
    {
        penetration = overlap.z;
        // Determine normal direction based on centers
        glm::vec3 centerA = (minA + maxA) * 0.5f;
        glm::vec3 centerB = (minB + maxB) * 0.5f;

        if (centerA.z < centerB.z)
        {
            normal = glm::vec3(0.0f, 0.0f, -1.0f);
        }
        else
        {
            normal = glm::vec3(0.0f, 0.0f, 1.0f);
        }
    }

    // Calculate contact point (approximate)
    glm::vec3 centerA = (minA + maxA) * 0.5f;
    glm::vec3 centerB = (minB + maxB) * 0.5f;
    glm::vec3 contactPoint = (centerA + centerB) * 0.5f;

    // Fill collision info
    info.contactPoint = contactPoint;
    info.normal = normal;
    info.penetrationDepth = penetration;
    info.colliderA = const_cast<BoxCollider*>(a);
    info.colliderB = const_cast<BoxCollider*>(b);

    return true;
}