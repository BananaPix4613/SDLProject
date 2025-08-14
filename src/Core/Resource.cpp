// -------------------------------------------------------------------------
// Resource.cpp
// -------------------------------------------------------------------------
#include "Core/Resource.h"
#include "Core/Logger.h"

// In namespace PixelCraft
namespace PixelCraft::Core
{

    Resource::Resource(const std::string& path)
        : m_path(path), m_loaded(false), m_refCount(0)
    {
        // Extract name from path (filename without extension)
        std::filesystem::path filePath(path);
        m_name = filePath.stem().string();
    }

    Resource::~Resource()
    {
        // Cannot call pure virtual unload() from base class destructor
        // Derived classes must handle their own cleanup in their destructors
        // This is because virtual function tables are destroyed before base destructors run

        if (m_loaded)
        {
            // Log a warning if resource wasn't properly cleaned up
            Core::warn("Resource '" + m_name + "' was not properly unloaded before destruction");
        }
    }

    Resource::Resource(Resource&& other) noexcept
        : m_path(std::move(other.m_path))
        , m_name(std::move(other.m_name))
        , m_loaded(other.m_loaded)
        , m_refCount(other.m_refCount.load())
    {
        // Reset other object
        other.m_loaded = false;
        other.m_refCount = 0;
    }

    Resource& Resource::operator=(Resource&& other) noexcept
    {
        if (this != &other)
        {
            // Move data
            m_path = std::move(other.m_path);
            m_name = std::move(other.m_name);
            m_loaded = other.m_loaded;
            m_refCount = other.m_refCount.load();

            // Reset other object
            other.m_loaded = false;
            other.m_refCount = 0;
        }
        return *this;
    }

    bool Resource::onReload()
    {
        // Default implementation just unloads and reloads
        if (m_loaded)
        {
            unload();
        }
        return load();
    }

} // namespace PixelCraft::Core