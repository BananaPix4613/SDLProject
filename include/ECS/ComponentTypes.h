// -------------------------------------------------------------------------
// ECSTypes.h
// -------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <bitset>

namespace PixelCraft::ECS
{

    // Basic type definitions
    using EntityID = uint32_t;                  ///< Entity identifier type
    using ComponentTypeID = uint32_t;           ///< Component type identifier
    constexpr size_t MAX_COMPONENT_TYPES = 64;  ///< Maximum number of component types in the system
    constexpr EntityID INVALID_ENTITY_ID = 0;   ///< Invalid entity ID constant
    const ComponentTypeID INVALID_COMPONENT_TYPE = static_cast<ComponentTypeID>(-1);

    using ComponentMask = std::bitset<MAX_COMPONENT_TYPES>;  ///< Component mask to identify which components a system requires

    /**
     * @brief Enumeration for byte order handling in serialization
     */
    enum class ByteOrder
    {
        LittleEndian, //!< Little endian byte order (x86, ARM)
        BigEndian,    //!< Big endian byte order (PowerPC, SPARC)
        Native        //!< Use the native byte order of the current platform
    };

} // namespace PixelCraft::ECS