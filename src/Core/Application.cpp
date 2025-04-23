// -------------------------------------------------------------------------
// Application.cpp
// -------------------------------------------------------------------------
#include "Core/Application.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <functional>

namespace PixelCraft::Core
{

    // Initialize static members
    std::mutex Application::m_instanceMutex;

    Application& Application::getInstance()
    {
        std::lock_guard<std::mutex> lock(m_instanceMutex);
        static Application instance;
        return instance;
    }

    Application::Application()
        : m_isRunning(false), m_deltaTime(0.0f), m_initialized(false)
    {
        // Create the configuration manager
        m_config = std::make_shared<ConfigManager>();
    }

    bool Application::initialize()
    {
        if (m_initialized)
        {
            warn("Application already initialized");
            return true;
        }

        info("Initializing PixelCraft Engine");

        // Initialize config manager first
        if (!m_config->initialize())
        {
            error("Failed to initialize ConfigManager");
            return false;
        }

        // Register the config manager as a subsystem
        registerSubsystem<ConfigManager>(m_config);

        // Sort subsystems based on dependencies
        if (!sortSubsystemDependencies())
        {
            error("Failed to resolve subsystem dependencies");
            return false;
        }

        // Initialize all subsystems in dependency order
        std::set<std::type_index> initializedSystems;
        for (const auto& typeIndex : m_subsystemInitOrder)
        {
            if (!initializeSubsystem(typeIndex, initializedSystems))
            {
                error("Failed to initialize subsystems: " + m_subsystems[typeIndex]->getName());
                return false;
            }
        }

        m_initialized = true;
        info("PixelCraft Engine initialized successfully");
        return true;
    }

    bool Application::sortSubsystemDependencies()
    {
        // Create a temporary vector for the new order
        std::vector<std::type_index> newOrder;
        std::set<std::type_index> visited;
        std::set<std::type_index> inProgress;

        // Topological sort using DFS
        std::function<bool(const std::type_index&)> visitNode = [&](const std::type_index& typeIndex) {
            if (visited.find(typeIndex) != visited.end())
            {
                return true;
            }

            if (inProgress.find(typeIndex) != inProgress.end())
            {
                error("Circular dependency detected in subsystems");
                return false;
            }

            inProgress.insert(typeIndex);

            auto subsystem = m_subsystems[typeIndex];
            auto dependencies = subsystem->getDependencies();

            for (const auto& depName : dependencies)
            {
                // Find the subsystem with this name
                auto it = std::find_if(m_subsystems.begin(), m_subsystems.end(),
                                       [&depName](const auto& pair) {
                                           return pair.second->getName() == depName;
                                       });

                if (it == m_subsystems.end())
                {
                    warn("Dependency not found: " + depName + " for subsystem " + subsystem->getName());
                    continue;
                }

                if (!visitNode(it->first))
                {
                    return false;
                }
            }

            inProgress.erase(typeIndex);
            visited.insert(typeIndex);
            newOrder.push_back(typeIndex);
            return true;
            };

        // Visit all nodes
        for (const auto& typeIndex : m_subsystemInitOrder)
        {
            if (!visitNode(typeIndex))
            {
                return false;
            }
        }

        // Update the initialization order
        m_subsystemInitOrder = newOrder;
        return true;
    }

    bool Application::initializeSubsystem(const std::type_index& typeIndex, std::set<std::type_index>& initializedSystems)
    {
        // Check if already initialized
        if (initializedSystems.find(typeIndex) != initializedSystems.end())
        {
            return true;
        }
        
        auto subsystem = m_subsystems[typeIndex];
        info("Initializing subsystem: " + subsystem->getName());

        // Legitimate dependencies first
        auto dependencies = subsystem->getDependencies();
        for (const auto& depName : dependencies)
        {
            // Find the subsystem with this name
            auto it = std::find_if(m_subsystems.begin(), m_subsystems.end(),
                                   [&depName](const auto& pair) {
                                       return pair.second->getName() == depName;
                                   });

            if (it == m_subsystems.end())
            {
                warn("Dependency not found: " + depName + " for subsystem " + subsystem->getName());
                continue;
            }

            if (!initializeSubsystem(it->first, initializedSystems))
            {
                return false;
            }
        }

        // Initialize the subsystem
        if (!subsystem->initialize())
        {
            error("Failed to initialize subsystem: " + subsystem->getName());
            return false;
        }

        initializedSystems.insert(typeIndex);
        return true;
    }

    void Application::run()
    {
        if (!m_initialized)
        {
            error("Cannot run application before initialization");
            return;
        }

        info("Starting main loop");
        m_isRunning = true;

        // Get configuration values for the main loop
        bool useFixedTimestep = m_config->get<bool>("engine.useFixedTimestep", false);
        float fixedTimestep = m_config->get<float>("engine.fixedTimestep", 1.0f / 60.0f);
        float maxFrameTime = m_config->get<float>("engine.maxFrameTime", 0.25f);

        using clock = std::chrono::high_resolution_clock;
        auto lastFrameTime = clock::now();
        float accumulatedTime = 0.0f;

        while (m_isRunning)
        {
            auto currentFrameTime = clock::now();
            float frameTime = std::chrono::duration<float>(currentFrameTime - lastFrameTime).count();
            lastFrameTime = currentFrameTime;

            // Cap extremely long frames (e.g., after debugging pause)
            frameTime = std::min(frameTime, maxFrameTime);

            if (useFixedTimestep)
            {
                // Fixed timestep update
                accumulatedTime += frameTime;
                while (accumulatedTime >= fixedTimestep)
                {
                    m_deltaTime = fixedTimestep;
                    update(m_deltaTime);
                    accumulatedTime -= fixedTimestep;
                }
            }
            else
            {
                // Variable timestep update
                m_deltaTime = frameTime;
                update(m_deltaTime);
            }

            // Always render once per frame
            render();

            // Optional frame rate limiter
            bool limitFrameRate = m_config->get<bool>("engine.limitFrameRate", false);
            if (limitFrameRate)
            {
                float targetFPS = m_config->get<float>("engine.targetFPS", 60.0f);
                float targetFrameTime = 1.0f / targetFPS;
                float sleepTime = targetFrameTime - frameTime;

                if (sleepTime > 0.0f)
                {
                    std::this_thread::sleep_for(std::chrono::duration<float>(sleepTime));
                }
            }
        }

        info("Main loop ended");
    }

    void Application::update(float deltaTime)
    {
        // Update all subsystems in dependency order
        for (const auto& typeIndex : m_subsystemInitOrder)
        {
            auto subsystem = m_subsystems[typeIndex];
            subsystem->update(deltaTime);
        }
    }

    void Application::render()
    {
        // Call render on all subsystems in dependency order
        for (const auto& typeIndex : m_subsystemInitOrder)
        {
            auto subsystem = m_subsystems[typeIndex];
            subsystem->render();
        }
    }

    void Application::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        info("Shutting down PixelCraft Engine");

        // Shutdown subsystems in reverse dependency order
        for (auto it = m_subsystemInitOrder.rbegin(); it != m_subsystemInitOrder.rend(); it++)
        {
            auto subsystem = m_subsystems[*it];
            info("Shutting down subsystem: " + subsystem->getName());
            subsystem->shutdown();
        }

        m_subsystems.clear();
        m_subsystemInitOrder.clear();
        m_initialized = false;
    }

    void Application::quit()
    {
        m_isRunning = false;
    }

    float Application::getDeltaTime() const
    {
        return m_deltaTime;
    }

} // namespace PixelCraft::Core