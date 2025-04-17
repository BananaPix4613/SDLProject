#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <any>

// Forward declarations
class Entity;
class EntityManager;
class EventSystem;
class Camera;

/**
 * @class Scene
 * @brief Represents a game scene containing entities and systems
 * 
 * The Scene class manages entities and provides a container for game state.
 * It coordinates with the EntityManager for entity lifecycle operations.
 */
class Scene
{
public:
    /**
     * @brief Constructor
     * @param name Scene name
     * @param eventSystem Event system to use
     */
    Scene(const std::string& name, EventSystem* eventSystem);

    /**
     * @brief Destructor
     */
    ~Scene();

    /**
     * @brief Initialize the scene
     */
    bool initialize();

    /**
     * @brief Update the scene
     * @param deltaTime Time since last frame
     */
    void update(float deltaTime);

    /**
     * @brief Render the scene
     */
    void render();

    /**
     * @brief Get scene name
     * @return Scene name
     */
    const std::string& getName() const
    {
        return m_name;
    }

    /**
     * @brief Set scene name
     * @param name New scene name
     */
    void setName(const std::string& name)
    {
        m_name = name;
    }

    /**
     * @brief Get the entity manager
     * @return Pointer to entity manager
     */
    EntityManager* getEntityManager() const
    {
        return m_entityManager.get();
    }

    /**
     * @brief Get the event system
     * @return Pointer to event system
     */
    EventSystem* getEventSystem() const
    {
        return m_eventSystem;
    }

    /**
     * @brief Create a new entity
     * @param name Optional name for the entity
     * @return Pointer to created entity
     */
    Entity* createEntity(const std::string& name = "Entity");

    /**
     * @brief Find entity by name
     * @param name Name to search for
     * @return Pointer to found entity or nullptr if not found
     */
    Entity* findEntityByName(const std::string& name) const;

    /**
     * @brief Find entities by tag
     * @param tag Tag to search for
     * @return Vector of matching entities
     */
    std::vector<Entity*> findEntitiesByTag(const std::string& tag) const;

    /**
     * @brief Set the main camera
     * @param camera Camera entity
     */
    void setMainCamera(Entity* cameraEntity);

    /**
     * @brief Get the main camera
     * @return Pointer to main camera entity
     */
    Entity* getMainCamera() const
    {
        return m_mainCamera;
    }

    /**
     * @brief Clear all entities
     */
    void clear();

    /**
     * @brief Load scene from file
     * @param filename Path to scene file
     * @return True if load succeeded
     */
    bool loadFromFile(const std::string& filename);

    /**
     * @brief Save scene to file
     * @param filename Path to save scene
     * @return True if save succeeded
     */
    bool saveToFile(const std::string& filename) const;

    /**
     * @brief Register a system with the scene
     * @param systemName Name of the system
     * @param system Pointer to system instance
     */
    void registerSystem(const std::string& systemName, void* system);

    /**
     * @brief Get a registered system
     * @param systemName Name of the system
     * @return Pointer to system or nullptr if not found
     */
    void* getSystem(const std::string& systemName) const;

    /**
     * @brief Get a system by type
     * @tparam T System type
     * @return Pointer to system or nullptr if not found
     */
    template<typename T>
    T* getSystem() const
    {
        std::string typeName = typeid(T).name();
        return static_cast<T*>(getSystem(typeName));
    }

    /**
     * @brief Register a scene-wide variable
     * @param name Variable name
     * @param data Variable data
     */
    template<typename T>
    void setData(const std::string& name, const T& data)
    {
        m_data[name] = std::make_any<T>(data);
    }

    /**
     * @brief Get a scene-wide variable
     * @param name Variable name
     * @return Variable value or default T() if not found
     */
    template<typename T>
    T getData(const std::string& name) const
    {
        auto it = m_data.find(name);
        if (it != m_data.end())
        {
            try
            {
                return std::any_cast<T>(it->second);
            }
            catch (const std::bad_any_cast&)
            {
                return T();
            }
        }
        return T();
    }

    /**
     * @brief Subscribe to scene load/unload events
     * @param onLoad Function to call when scene is loaded
     * @param onUnload Function to call when scene is unloaded
     * @return Subscription ID
     */
    int subscribeToSceneEvents(std::function<void(Scene*)> onLoad,
                               std::function<void(Scene*)> onUnload);

    /**
     * @brief Unsubscribe from scene events
     * @param subscriptionId Subsription ID from subscribeToSceneEvents
     */
    void unsubscribeFromSceneEvents(int subscriptionId);

private:
    // Scene properties
    std::string m_name;

    // Systems
    std::unique_ptr<EntityManager> m_entityManager;
    EventSystem* m_eventSystem;
    std::unordered_map<std::string, void*> m_systems;

    // Main camera
    Entity* m_mainCamera;

    // Scene data
    std::unordered_map<std::string, std::any> m_data;

    // Scene event handling
    struct SceneCallback
    {
        int id;
        std::function<void(Scene*)> onLoad;
        std::function<void(Scene*)> onUnload;
    };

    std::vector<SceneCallback> m_callbacks;
    int m_nextCallbackId;

    // Helper methods
    void notifySceneLoaded();
    void notifySceneUnloaded();
};