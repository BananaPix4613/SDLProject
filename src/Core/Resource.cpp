// -------------------------------------------------------------------------
// Resource.cpp
// -------------------------------------------------------------------------
#include "Core/Resource.h"
#include "Core/Logger.h"

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
        // Ensure resource is unloaded when destroyed
        if (m_loaded)
        {
            unload();
        }
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