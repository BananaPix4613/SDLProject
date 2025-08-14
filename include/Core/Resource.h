// -------------------------------------------------------------------------
// Resource.h
// -------------------------------------------------------------------------
#pragma once

#include <string>
#include <atomic>
#include <filesystem>

// In namespace PixelCraft
namespace PixelCraft::Core
{

    /**
     * @brief Base class for all engine resources with reference coutings
     */
    class Resource
    {
    public:
        /**
         * @brief Constructor
         * @param path The file path for this resource
         */
        Resource(const std::string& path);

        /**
         * @brief Virtual destructor
         */
        virtual ~Resource();

        /**
         * @brief Copy constructor (deleted - resources should not be copied)
         */
        Resource(const Resource&) = delete;

        /**
         * @brief Copy assignment (deleted - resources should not be copied)
         */
        Resource& operator=(const Resource&) = delete;

        /**
         * @brief Move constructor
         */
        Resource(Resource&& other) noexcept;

        /**
         * @brief Move assignment operator
         */
        Resource& operator=(Resource&& other) noexcept;

        /**
         * @brief Load the resource from its path
         * @return True if loading was successful
         */
        virtual bool load() = 0;

        /**
         * @brief Unload the resource and free memory
         */
        virtual void unload() = 0;

        /**
         * @brief Check if the resource is currently loaded
         * @return True if the resource is loaded
         */
        bool isLoaded() const
        {
            return m_loaded;
        }

        /**
         * @brief Get the file path for this resource
         * @return The resource file path
         */
        const std::string& getPath() const
        {
            return m_path;
        }

        /**
         * @brief Set the file path for this resource
         */
        void setPath(const std::string& path)
        {
            m_path = path;
        }

        /**
         * @brief Get the name of this resource
         * @return The resource name
         */
        const std::string& getName() const
        {
            return m_name;
        }

        /**
         * @brief Get the current reference count
         * @return Number of references to this resource
         */
        int getRefCount() const
        {
            return m_refCount.load();
        }

        /**
         * @brief Called when hot-reloading this resource
         * @return True if reloading was successful
         */
        virtual bool onReload();

        /**
         * @brief Increment the reference count
         * @return The new reference count
         */
        int addRef()
        {
            return m_refCount++;
        }

        /**
         * @brief Decrement the reference count
         * @return The new reference count
         */
        int releaseRef()
        {
            return m_refCount--;
        }

    protected:
        std::string m_path;    ///< File path of the resource
        std::string m_name;    ///< Name of the resource (derived from path)
        bool m_loaded;         ///< Whether the resource is currently loaded
        std::atomic<int> m_refCount; ///< Reference count for memory management
    };

    /**
     * @brief Concrete implementation of Resource for testing purposes
     */
    class TestResource : public Resource
    {
    public:
        /**
         * @brief Constructor
         * @param path The file path for this resource
         */
        TestResource(const std::string& path) : Resource(path) {}

        /**
         * @brief Load the resource from its path
         * @return True if loading was successful
         */
        bool load() override
        {
            m_loaded = true;
            return true;
        }

        /**
         * @brief Unload the resource and free memory
         */
        void unload() override
        {
            m_loaded = false;
        }
    };

} // namespace PixelCraft::Core