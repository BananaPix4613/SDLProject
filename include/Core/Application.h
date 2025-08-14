// -------------------------------------------------------------------------
// Application.h
// -------------------------------------------------------------------------
#pragma once

#include "Core/ConfigManager.h"
#include "Core/Logger.h"
#include "Core/Subsystem.h"

#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace PixelCraft::Rendering
{
    class RenderSystem;
    namespace Cameras
    {
        class OrthographicCamera;
    }
}

namespace PixelCraft::Voxel
{
    class Grid;
    class VoxelRenderer;
}

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
        static Application *getInstance();

        /**
         * @brief Deleted copy constructor to enforce singleton
         */
        Application(const Application &) = delete;

        /**
         * @brief Deleted assignment operator to enforce singleton
         */
        Application &operator=(const Application &) = delete;

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
        template <typename T> std::shared_ptr<T> getSubsystem()
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
        template <typename T> bool registerSubsystem(std::shared_ptr<T> subsystem)
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
        bool initializeSubsystem(const std::type_index &typeIndex, std::set<std::type_index> &initializedSystems);

        /**
         * @brief Setup the isometric rendering pipeline for pixel-perfect isometric rendering
         * @return True if setup was successful
         */
        bool setupIsometricRenderingPipeline();

        /**
         * @brief Setup the camera for isometric rendering
         * @return True if setup was successful
         */
        bool setupCamera();

        /**
         * @brief Setup the test voxel world
         * @return True if the voxel world was set up successfully
         */
        bool setupTestVoxelWorld();

        /**
         * @brief Create basic materials for rendering
         */
        void createBasicMaterials();

        /**
         * @brief Create basic shaders for rendering
         * @return True if shaders were created successfully
         */
        bool createBasicShaders();

        /**
         * @brief Debug utility for subsystem initialization order
         */
        void debugInitializationOrder();

        /**
         * @brief Debug utility for rendering pipeline
         */
        void debugRenderingPipeline();

        /**
         * @brief Debug utility for voxel renderer
         */
        void debugVoxelRenderer();

        // Member variables
        bool m_isRunning = false;
        float m_deltaTime = 0.0f;
        std::unordered_map<std::type_index, std::shared_ptr<Subsystem>> m_subsystems;
        std::vector<std::type_index> m_initializationOrder; // Sorted order of subsystems based on dependencies

        // Singleton management
        static Application *s_instance;
        static std::mutex m_instanceMutex;

        // World variables
        std::shared_ptr<Voxel::Grid> m_voxelGrid;
        std::shared_ptr<Rendering::Cameras::OrthographicCamera> m_camera;
    };

} // namespace PixelCraft::Core
