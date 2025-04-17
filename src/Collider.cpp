#include "Collider.h"
#include "components/PhysicsComponent.h"
#include "Entity.h"
#include "Scene.h"
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/quaternion.hpp>
#include <algorithm>

// Base Collider Implementation

Collider::Collider() :
    m_localPosition(0.0f),
    m_localRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
    m_isTrigger(false),
    m_friction(0.5f),
    m_restitution(0.0f),
    m_physicsComponent(nullptr)
{
}

Collider::~Collider()
{
    if (m_physicsComponent)
    {
        m_physicsComponent->removeCollider(this);
    }
}

void Collider::setLocalPosition(const glm::vec3& localPosition)
{
    m_localPosition = localPosition;
}

const glm::vec3& Collider::getLocalPosition() const
{
    return m_localPosition;
}

void Collider::setLocalRotation(const glm::quat& localRotation)
{
    m_localRotation = localRotation;
}

const glm::quat& Collider::getLocalRotation() const
{
    return m_localRotation;
}

void Collider::setTrigger(bool isTrigger)
{
    m_isTrigger = isTrigger;
}

bool Collider::isTrigger() const
{
    return m_isTrigger;
}

void Collider::setMaterial(float friction, float restitution)
{
    // Clamp values to valid ranges
    m_friction = glm::clamp(friction, 0.0f, 1.0f);
    m_restitution = glm::clamp(restitution, 0.0f, 1.0f);
}

float Collider::getFriction() const
{
    return m_friction;
}

float Collider::getRestitution() const
{
    return m_restitution;
}

void Collider::setPhysicsComponent(PhysicsComponent* component)
{
    m_physicsComponent = component;
}

PhysicsComponent* Collider::getPhysicsComponent() const
{
    return m_physicsComponent;
}

glm::mat4 Collider::getWorldTransform() const
{
    // Get entity transform if available
    glm::mat4 entityTransform(1.0f);
    if (m_physicsComponent && m_physicsComponent->getEntity())
    {
        entityTransform = m_physicsComponent->getEntity()->getWorldTransform();
    }

    // Create local transform matrix
    glm::mat4 localTransform = glm::translate(glm::mat4(1.0f), m_localPosition);
    localTransform = localTransform * glm::mat4_cast(m_localRotation);

    // Combine transforms
    return entityTransform * localTransform;
}

void Collider::getWorldBounds(glm::vec3& outMin, glm::vec3& outMax) const
{
    // Get local bounds
    glm::vec3 localMin, localMax;
    getLocalBounds(localMin, localMax);

    // Get world transform
    glm::mat4 worldTransform = getWorldTransform();

    // Transform each corner of the bounding box
    glm::vec3 corners[8];
    corners[0] = glm::vec3(localMin.x, localMin.y, localMin.z);
    corners[1] = glm::vec3(localMax.x, localMin.y, localMin.z);
    corners[2] = glm::vec3(localMin.x, localMax.y, localMin.z);
    corners[3] = glm::vec3(localMax.x, localMax.y, localMin.z);
    corners[4] = glm::vec3(localMin.x, localMin.y, localMax.z);
    corners[5] = glm::vec3(localMax.x, localMin.y, localMax.z);
    corners[6] = glm::vec3(localMin.x, localMax.y, localMax.z);
    corners[7] = glm::vec3(localMax.x, localMax.y, localMax.z);

    // Transform corners to world space
    for (int i = 0; i < 8; i++)
    {
        glm::vec4 transformedCorner = worldTransform * glm::vec4(corners[i], 1.0f);
        corners[i] = glm::vec3(transformedCorner) / transformedCorner.w;
    }

    // Find min and max of transformed corners
    outMin = corners[0];
    outMax = corners[0];

    for (int i = 1; i < 8; i++)
    {
        outMin = glm::min(outMin, corners[i]);
        outMax = glm::max(outMax, corners[i]);
    }
}

// BoxCollider Implementation

BoxCollider::BoxCollider(const glm::vec3& size) :
    m_size(size)
{
}

ColliderType BoxCollider::getType() const
{
    return ColliderType::BOX;
}

void BoxCollider::setSize(const glm::vec3& size)
{
    m_size = size;
}

const glm::vec3& BoxCollider::getSize() const
{
    return m_size;
}

void BoxCollider::getLocalBounds(glm::vec3& outMin, glm::vec3& outMax) const
{
    // Half size
    glm::vec3 halfSize = m_size * 0.5f;

    // Local bounds are centered around local position
    outMin = m_localPosition - halfSize;
    outMax = m_localPosition + halfSize;
}

bool BoxCollider::containsPoint(const glm::vec3& point) const
{
    // Transform point to local space
    glm::mat4 worldToLocal = glm::inverse(getWorldTransform());
    glm::vec4 localPoint = worldToLocal * glm::vec4(point, 1.0f);

    // Half size
    glm::vec3 halfSize = m_size * 0.5f;

    // Check if point is within box
    return (fabs(localPoint.x) <= halfSize.x &&
            fabs(localPoint.y) <= halfSize.y &&
            fabs(localPoint.z) <= halfSize.z);
}

glm::vec3 BoxCollider::getClosestPoint(const glm::vec3& point) const
{
    // Transform point to local space
    glm::mat4 worldToLocal = glm::inverse(getWorldTransform());
    glm::vec4 localPointVec4 = worldToLocal * glm::vec4(point, 1.0f);
    glm::vec3 localPoint(localPointVec4.x, localPointVec4.y, localPointVec4.z);

    // Half size
    glm::vec3 halfSize = m_size * 0.5f;

    // Clamp point to box
    glm::vec3 closest;
    closest.x = glm::clamp(localPoint.x, -halfSize.x, halfSize.x);
    closest.y = glm::clamp(localPoint.y, -halfSize.y, halfSize.y);
    closest.z = glm::clamp(localPoint.z, -halfSize.z, halfSize.z);

    // Transform back to world space
    glm::mat4 localToWorld = getWorldTransform();
    glm::vec4 worldPointVec4 = localToWorld * glm::vec4(closest, 1.0f);

    return glm::vec3(worldPointVec4.x, worldPointVec4.y, worldPointVec4.z) / worldPointVec4.w;
}

// SphereCollider Implementation

SphereCollider::SphereCollider(float radius) :
    m_radius(radius)
{
}

ColliderType SphereCollider::getType() const
{
    return ColliderType::SPHERE;
}

void SphereCollider::setRadius(float radius)
{
    m_radius = radius;
}

float SphereCollider::getRadius() const
{
    return m_radius;
}

void SphereCollider::getLocalBounds(glm::vec3& outMin, glm::vec3& outMax) const
{
    outMin = m_localPosition - glm::vec3(m_radius);
    outMax = m_localPosition + glm::vec3(m_radius);
}

bool SphereCollider::containsPoint(const glm::vec3& point) const
{
    // Get sphere center in world space
    glm::mat4 worldTransform = getWorldTransform();
    glm::vec4 centerVec4 = worldTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec3 center = glm::vec3(centerVec4) / centerVec4.w;

    // Check if point is within radius
    return glm::distance(center, point) <= m_radius;
}

glm::vec3 SphereCollider::getClosestPoint(const glm::vec3& point) const
{
    // Get sphere center in world space
    glm::mat4 worldTransform = getWorldTransform();
    glm::vec4 centerVec4 = worldTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec3 center = glm::vec3(centerVec4) / centerVec4.w;

    // Direction from center to point
    glm::vec3 direction = point - center;
    float distance = glm::length(direction);

    // If point is inside sphere, it's already the closest point
    if (distance <= m_radius)
    {
        return point;
    }

    // Otherwise, closest point is on the surface
    return center + direction * (m_radius / distance);
}

// CapsuleCollider Implementation

CapsuleCollider::CapsuleCollider(float radius, float height) :
    m_radius(radius),
    m_height(height)
{
}

ColliderType CapsuleCollider::getType() const
{
    return ColliderType::CAPSULE;
}

void CapsuleCollider::setRadius(float radius)
{
    m_radius = radius;
}

float CapsuleCollider::getRadius() const
{
    return m_radius;
}

void CapsuleCollider::setHeight(float height)
{
    m_height = height;
}

float CapsuleCollider::getHeight() const
{
    return m_height;
}

void CapsuleCollider::getLocalBounds(glm::vec3& outMin, glm::vec3& outMax) const
{
    // Assume capsule is aligned along the y-axis
    outMin = m_localPosition + glm::vec3(-m_radius, -m_height * 0.5f, -m_radius);
    outMax = m_localPosition + glm::vec3(m_radius, m_height * 0.5f, m_radius);
}

bool CapsuleCollider::containsPoint(const glm::vec3& point) const
{
    // Transform point to local space
    glm::mat4 worldToLocal = glm::inverse(getWorldTransform());
    glm::vec4 localPointVec4 = worldToLocal * glm::vec4(point, 1.0f);
    glm::vec3 localPoint(localPointVec4.x, localPointVec4.y, localPointVec4.z);

    // Capsule extends along y-axis from -halfHeight to +halfHeight
    float halfHeight = m_height * 0.5f - m_radius;

    // Clamp y to capsule segment
    float y = glm::clamp(localPoint.y, -halfHeight, halfHeight);

    // Distance from point to capsule segment
    glm::vec3 centerPoint(0.0f, y, 0.0f);
    float distance = glm::distance(localPoint, centerPoint);

    // Check if distance is less than radius
    return distance <= m_radius;
}

glm::vec3 CapsuleCollider::getClosestPoint(const glm::vec3& point) const
{
    // Transform point to local space
    glm::mat4 worldToLocal = glm::inverse(getWorldTransform());
    glm::vec4 localPointVec4 = worldToLocal * glm::vec4(point, 1.0f);
    glm::vec3 localPoint(localPointVec4.x, localPointVec4.y, localPointVec4.z);

    // Capsule extends along y-axis from -halfHeight to +halfHeight
    float halfHeight = m_height * 0.5f - m_radius;

    // Clamp y to capsule segment
    float y = glm::clamp(localPoint.y, -halfHeight, halfHeight);

    // Find closest point on capsule segment
    glm::vec3 centerPoint(0.0f, y, 0.0f);
    glm::vec3 direction = localPoint - centerPoint;
    float distance = glm::length(direction);

    // Get closest point in local space
    glm::vec3 localClosest;
    if (distance <= 0.0001f)
    {
        // Point is on the center line, project outward in any direction
        localClosest = centerPoint + glm::vec3(m_radius, 0.0f, 0.0f);
    }
    else
    {
        localClosest = centerPoint + direction * (m_radius / distance);
    }

    // Transform back to world space
    glm::mat4 localToWorld = getWorldTransform();
    glm::vec4 worldClosestVec4 = localToWorld * glm::vec4(localClosest, 1.0f);

    return glm::vec3(worldClosestVec4) / worldClosestVec4.w;
}

// VoxelGridCollider Implementation

VoxelGridCollider::VoxelGridCollider(int gridRadius) :
    m_gridRadius(gridRadius)
{
}

ColliderType VoxelGridCollider::getType() const
{
    return ColliderType::VOXEL_GRID;
}

void VoxelGridCollider::setGridRadius(int gridRadius)
{
    m_gridRadius = gridRadius;
}

int VoxelGridCollider::getGridRadius() const
{
    return m_gridRadius;
}

void VoxelGridCollider::getLocalBounds(glm::vec3& outMin, glm::vec3& outMax) const
{
    // Create a bounding box based on grid radius
    float radius = static_cast<float>(m_gridRadius);
    outMin = m_localPosition - glm::vec3(radius);
    outMax = m_localPosition + glm::vec3(radius);
}

bool VoxelGridCollider::containsPoint(const glm::vec3& point) const
{
    // For voxel grid, we need to check with the physics system
    if (!m_physicsComponent)
        return false;

    Entity* entity = m_physicsComponent->getEntity();
    if (!entity)
        return false;

    Scene* scene = entity->getScene();
    if (!scene)
        return false;

    PhysicsSystem* physicsSystem = scene->getSystem<PhysicsSystem>();
    if (!physicsSystem)
        return false;

    // Delegate to physics system which has access to the voxel grid
    // This is simplified for the implementation
    // In a real implementation, we would check the voxel at the point position

    // For now, just check if point is within radius
    glm::mat4 worldTransform = getWorldTransform();
    glm::vec4 centerVec4 = worldTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec3 center = glm::vec3(centerVec4) / centerVec4.w;

    return glm::distance(center, point) <= static_cast<float>(m_gridRadius);
}

glm::vec3 VoxelGridCollider::getClosestPoint(const glm::vec3& point) const
{
    // For a voxel grid, finding the closest point is complex
    // In a real implementation, we would find the closest solid voxel

    // For now, just clamp to the bounding box
    glm::vec3 min, max;
    getWorldBounds(min, max);

    return glm::clamp(point, min, max);
}