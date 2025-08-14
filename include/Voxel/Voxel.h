// -------------------------------------------------------------------------
// Voxel.h
// -------------------------------------------------------------------------
#pragma once

#include "Utility/Serialization/SerializationUtility.h"

#include <cstdint>

namespace PixelCraft::Voxel
{

    /**
     * @brief Basic voxel data structure
     * 
     * Compact representation of a single voxel in the world, containing
     * type information and additional data alues
     */
    struct Voxel
    {
        uint16_t type;     ///< Voxel type (0 = empty/air)
        uint16_t data;     ///< Additional data (material ID, rotation, etc.)

        /**
         * @brief Default constructor (creates empty voxel)
         */
        Voxel() : type(0), data(0)
        {
        }

        /**
         * @brief Constructor with type and data
         * @param _type Voxel type
         * @param _data Additional data
         */
        Voxel(uint16_t _type, uint16_t _data) : type(_type), data(_data)
        {
        }

        /**
         * @brief Check if voxel is empty (air)
         * @return True if voxel is empty
         */
        bool isEmpty() const
        {
            return type == 0;
        }

        /**
         * @brief Equality comparison operator
         * @param other Voxel to compare with
         * @return True if voxels are identical
         */
        bool operator==(const Voxel& other) const
        {
            return type == other.type && data == other.data;
        }

        /**
         * @brief Inequality comparison operator
         * @param other Voxel to compare with
         * @return True if voxels are different
         */
        bool operator!=(const Voxel& other) const
        {
            return !(*this == other);
        }

        /**
         * @brief Serialize the voxel to the given serializer
         * @param serializer The serializer to use
         * @return SerializationResult with success/failure status
         */
        Utility::Serialization::SerializationResult serialize(Utility::Serialization::Serializer& serializer) const
        {
            auto result = serializer.beginObject("Voxel");
            if (!result) return result;

            result = serializer.writeField("type", type);
            if (!result) return result;

            result = serializer.writeField("data", data);
            if (!result) return result;

            return serializer.endObject();
        }

        /**
         * @brief Deserialize the voxel from the given deserializer
         * @param deserializer The deserializer to use
         * @return SerializationResult with success/failure status
         */
        Utility::Serialization::SerializationResult deserialize(Utility::Serialization::Deserializer& deserializer)
        {
            auto result = deserializer.beginObject("Voxel");
            if (!result) return result;

            result = deserializer.readField("type", type);
            if (!result) return result;

            result = deserializer.readField("data", data);
            if (!result) return result;

            return deserializer.endObject();
        }

        /**
         * @brief Define the schema for Voxel serialization
         * @param schema The schema to define
         */
        static void defineSchema(Utility::Serialization::Schema& schema)
        {
            schema.addField("type", Utility::Serialization::ValueType::UInt16, true);
            schema.addField("data", Utility::Serialization::ValueType::UInt16, true);
        }

        /**
         * @brief Get the type name for serialization
         * @return The type name string
         */
        static std::string getTypeName()
        {
            return "Voxel";
        }
    };

} // namespace PixelCraft::Voxel