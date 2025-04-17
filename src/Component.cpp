#include "Component.h"
#include "Entity.h"
#include "Scene.h"

Component::Component() :
    m_entity(nullptr),
    m_active(true),
    m_started(false)
{
}

Component::~Component()
{
}

void Component::initialize()
{
    // Base implementation does nothing
    // Override in derived classes
}

void Component::start()
{
    // Base implementation does nothing
    // Override in derived classes
}

void Component::update(float deltaTime)
{
    // Base implementation does nothing
    // Override in derived classes
}

void Component::render()
{
    // Base implementation does nothing
    // Override in derived classes for visual components
}

void Component::onDestroy()
{
    // Base implementation does nothing
    // Override in derived classes for cleanup
}

Entity* Component::getEntity() const
{
    return m_entity;
}

bool Component::isActive() const
{
    // Component is active only if it and its entity are both active
    return m_active && m_entity && m_entity->isActive();
}

void Component::setActive(bool active)
{
    m_active = active;
}

const char* Component::getTypeName() const
{
    return "Component";
}

Scene* Component::getScene() const
{
    return m_entity ? m_entity->getScene() : nullptr;
}

void Component::onTransformChanged()
{
    // Base implementation does nothing
    // Override in derived classes to respond to transform changes
}