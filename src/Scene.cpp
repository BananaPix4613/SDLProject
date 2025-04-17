#include "Scene.h"
#include "Entity.h"
#include "Component.h"
#include "EntityManager.h"
#include "Camera.h"
#include <algorithm>
#include <iostream>

Scene::Scene(const std::string& name, EventSystem* eventSystem) :
    m_name(name),
    m_eventSystem(eventSystem),
    m_mainCamera(nullptr),
    m_nextCallbackId(1)
{
    // Create entity manager
    m_entityManager = std::make_unique<EntityManager>(this, eventSystem);

    // Register systems map
    m_systems.clear();

    // Initialize entity manager
    m_entityManager->initialize();
}

Scene::~Scene()
{
    // Notify that scene is being unloaded
    notifySceneUnloaded();

    // Clear all data
    clear();
}

bool Scene::initialize()
{
    // Nothing required here as EntityManager handles initialization
    return true;
}

void Scene::update(float deltaTime)
{
    // Update entity manager (which updates all entities)
    m_entityManager->update(deltaTime);
}

void Scene::render()
{
    // Rendering is handled by rendering system, not directly by Scene
}

Entity* Scene::createEntity(const std::string& name)
{
    // Use entity manager to create entity
    return m_entityManager->createEntity(name);
}

Entity* Scene::findEntityByName(const std::string& name) const
{
    // Use entity manager to find entity by name
    return m_entityManager->getEntityByName(name);
}

std::vector<Entity*> Scene::findEntitiesByTag(const std::string& tag) const
{
    // Use entity manager to find entities by tag
    return m_entityManager->getEntitiesByTag(tag);
}

void Scene::setMainCamera(Entity* cameraEntity)
{
    m_mainCamera = cameraEntity;
}

Entity* Scene::getMainCamera() const
{
    return m_mainCamera;
}

void Scene::clear()
{
    // Use entity manager to destroy all entities
    if (m_entityManager)
    {
        m_entityManager->destroyAllEntities(true);
    }

    // Clear systems
    m_systems.clear();

    // Clear other data
    m_data.clear();
    m_mainCamera = nullptr;
}

bool Scene::loadFromFile(const std::string& filename)
{
    // This would need to be implemented with a proper serialization system
    // For now, return false indicating failure
    return false;
}

bool Scene::saveToFile(const std::string& filename) const
{
    // This would need to be implemented with a proper serialization system
    // For now, return false indicating failure
    return false;
}

void Scene::registerSystem(const std::string& systemName, void* system)
{
    m_systems[systemName] = system;
}

void* Scene::getSystem(const std::string& systemName) const
{
    auto it = m_systems.find(systemName);
    if (it != m_systems.end())
    {
        return it->second;
    }
    return nullptr;
}

int Scene::subscribeToSceneEvents(std::function<void(Scene*)> onLoad,
                                  std::function<void(Scene*)> onUnload)
{
    SceneCallback callback;
    callback.id = m_nextCallbackId++;
    callback.onLoad = onLoad;
    callback.onUnload = onUnload;

    m_callbacks.push_back(callback);

    return callback.id;
}

void Scene::unsubscribeFromSceneEvents(int subscriptionId)
{
    auto it = std::remove_if(
        m_callbacks.begin(),
        m_callbacks.end(),
        [subscriptionId](const SceneCallback& callback) { return callback.id == subscriptionId; }
    );

    m_callbacks.erase(it, m_callbacks.end());
}

EntityManager* Scene::getEntityManager() const
{
    return m_entityManager.get();
}

EventSystem* Scene::getEventSystem() const
{
    return m_eventSystem;
}

void Scene::notifySceneLoaded()
{
    // Call all load callbacks
    for (const auto& callback : m_callbacks)
    {
        if (callback.onLoad)
        {
            callback.onLoad(this);
        }
    }
}

void Scene::notifySceneUnloaded()
{
    // Call all unload callbacks
    for (const auto& callback : m_callbacks)
    {
        if (callback.onUnload)
        {
            callback.onUnload(this);
        }
    }
}