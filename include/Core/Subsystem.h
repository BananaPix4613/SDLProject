// -------------------------------------------------------------------------
// Subsystem.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace PixelCraft::Core
{

    /**
     * @brief Base class for all engine subsystems with standardized lifecycle.
     * 
     * Provides a common interface for initialization, updating, rendering,
     * and shutdown that all engine subsystems must implement.
     */
    class Subsystem
    {
    public:
        /**
         * @brief Constructor
         */
        Subsystem();

        /**
         * @brief Virtual destructor for proper cleanup of derived classes
         */
        virtual ~Subsystem();

        /**
         * @brief Initialize the subsystem
         * @return True if initialization was successful, false otherwise
         */
        virtual bool initialize() = 0;

        /**
         * @brief Update the subsystem logic
         * @param deltaTime Time elapsed since the last update in seconds
         */
        virtual void update(float deltaTime) = 0;

        /**
         * @brief Render any visual elements of the subsystem
         */
        virtual void render() = 0;

        /**
         * @brief Shutdown the subsystem and release resources
         */
        virtual void shutdown() = 0;

        /**
         * @brief Check if the subsystem has been initialized
         * @return True if initialized, false otherwise
         */
        bool isInitialized() const;

        /**
         * @brief Check if the subsystem is currently active
         * @return True if active, false otherwise
         */
        bool isActive() const;

        /**
         * @brief Set the active state of the subsystem
         * @param active The new active state
         */
        void setActive(bool active);

        /**
         * @brief Get the name of the subsystem
         * @return The subsystem name as a string
         */
        virtual std::string getName() const = 0;

        /**
         * @brief Get the dependencies of this subsystem
         * @return Vector of the subsystem names that this subsystem depends on
         */
        virtual std::vector<std::string> getDependencies() const;

    protected:
        /** @brief Flag indicating whether the subsystem has been initialized */
        bool m_initialized;

        /** @brief Flag indicating whether the subsystem is active */
        bool m_active;
    };

} // namespace PixelCraft::Core