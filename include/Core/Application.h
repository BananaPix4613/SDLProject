// -------------------------------------------------------------------------
// Application.h
// -------------------------------------------------------------------------
#pragma core

#include <memory>
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <string>
#include <mutex>
#include <set>

#include "Core/Subsystem.h"
#include "Core/ConfigManager.h"
#include "Core/Logger.h"

namespace PixelCraft::Core
{

    /**
     * @brief Central engine controller with modular subsystem management
     */
    class Application
    {
    public:
        /**
         * @brief Get the singleton instance of the Application
         * @return Reference to the Application instance
         */
        static Application* getInstance();

        /**
         * @brief Deleted copy constructor to enforce singleton
         */
        Application(const Application&) = delete;

        /**
         * @brief Deleted assignment operator to enforce singleton
         */
        Application& operator=(const Application&) = delete;

        /**
         * @brief Initialize the application and still unregistered subsystems
         * @return True if initialization was successful
         */
        bool initialize();

        /**
         * @brief Run the main application loop
         */
        void run();

        /**
         * @brief Update all subsystems for a single frame
         * @param deltaTime Time since last update
         */
        void update(float deltaTime);

        /**
         * @brief Render a single frame
         */
        void render();

        /**
         * @brief Shut down the application and all subsystems
         */
        void shutdown();

        /**
         * @brief Request the application to quit
         */
        void quit();

        /**
         * @brief Get the time elapsed since the last frame
         * @return Delta time in seconds
         */
        float getDeltaTime() const;

        /**
         * @brief Get a subsystem by type
         * @tparam T Type of subsystem to retrieve
         * @return Shared pointer to the requested subsystem
         */
        template<typename T>
        std::shared_ptr<T> getSubsystem()
        {
            static_assert(std::is_base_of<Subsystem, T>::value, "T must derive from Subsystem");

            auto typeIndex = std::type_index(typeid(T));
            auto it = m_subsystems.find(typeIndex);
            if (it == m_subsystems.end())
            {
                return nullptr;
            }

            return std::dynamic_pointer_cast<T>(it->second);
        }

        /**
         * @brief Register a subsystem with the application
         * @tparam T Type of subsystem to register
         * @param subsystem Shared pointer to the subsystem
         * @return True if registration was successful
         */
        template<typename T>
        bool registerSubsystem(std::shared_ptr<T> subsystem)
        {
            static_assert(std::is_base_of<Subsystem, T>::value, "T must derive from Subsystem");

            if (!subsystem)
            {
                error("Attempted to register null subsystem");
                return false;
            }

            auto typeIndex = std::type_index(typeid(T));
            if (m_subsystems.find(typeIndex) != m_subsystems.end())
            {
                warn("Subsystem of type " + std::string(typeid(T).name()) + " already registered");
                return false;
            }

            m_subsystems[typeIndex] = subsystem;
            m_subsystemInitOrder.push_back(typeIndex);
            info("Registered subsystem: " + subsystem->getName());
            return true;
        }

    private:
        /**
         * @brief Private constructor to enforce singleton
         */
        Application();

        /**
         * @brief Initialize all serialization types
         * @return True if initialization was successful
         */
        bool initializeSerializationSystem();

        /**
         * @brief Sort subsystems based on their dependencies
         * @return True if dependencies could be resolved without cycles
         */
        bool sortSubsystemDependencies();

        /**
         * @brief Initialize a specific subsystem and its dependencies
         * @param typeIndex Type index of the subsystem to initialize
         * @param initializedSystems Set of already initialized subsystems
         * @return True if initialization was successful
         */
        bool initializeSubsystem(const std::type_index& typeIndex, std::set<std::type_index>& initializedSystems);

        // Singleton instance
        static Application* s_instance;

        // Member variables
        std::unordered_map<std::type_index, std::shared_ptr<Subsystem>> m_subsystems;
        std::vector<std::type_index> m_subsystemInitOrder;
        bool m_isRunning = false;
        float m_deltaTime = 0.0f;
        bool m_initialized = false;
        std::shared_ptr<ConfigManager> m_config;
        static std::mutex m_instanceMutex;
    };

} // namespace PixelCraft::Core