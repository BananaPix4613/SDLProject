// -------------------------------------------------------------------------
// ChunkCoord.h
// -------------------------------------------------------------------------
#pragma once

#include "Utility/Serialization/SerializationUtility.h"
#include "glm/glm.hpp"

#include <functional>
#include <string>

namespace PixelCraft::Voxel
{

    /**
     * @brief 3D chunk coordinate in the voxel grid
     */
    struct ChunkCoord
    {
        int x;  ///< X coordinate
        int y;  ///< Y coordinate
        int z;  ///< Z coordinate

        /**
         * @brief Default constructor initializes to (0,0,0)
         */
        ChunkCoord() : x(0), y(0), z(0)
        {
        }

        /**
         * @brief Constructor with explicit coordinates
         * @param _x X coordinate
         * @param _y Y coordinate
         * @param _z Z coordinate
         */
        ChunkCoord(int _x, int _y, int _z) : x(_x), y(_y), z(_z)
        {
        }

        /**
         * @brief Equality operation
         * @param other Coordinate to compare with
         * @return True if coordinates are equal
         */
        bool operator==(const ChunkCoord& other) const
        {
            return x == other.x && y == other.y && z == other.z;
        }

        /**
         * @brief Inequality operator
         * @param other Coordinate to compare with
         * @return True if coordinates are not equal
         */
        bool operator!=(const ChunkCoord& other) const
        {
            return !(*this == other);
        }

        /**
         * @brief Addition operator
         * @param other Coordinate to add
         * @return New coordinate with added values
         */
        ChunkCoord operator+(const ChunkCoord& other) const
        {
            return ChunkCoord(x + other.x, y + other.y, z + other.z);
        }

        /**
         * @brief Subtraction operator
         * @param other Coordinate to subtract
         * @return New coordinate with subtracted values
         */
        ChunkCoord operator-(const ChunkCoord& other) const
        {
            return ChunkCoord(x - other.x, y - other.y, z - other.z);
        }

        /**
         * @brief Converts world position to chunk coordinate
         * @param worldPos World position
         * @param chunkSize Size of a chunk
         * @return Chunk coordinate containing the world position
         */
        static ChunkCoord fromWorldPosition(const glm::vec3& worldPos, int chunkSize)
        {
            return ChunkCoord(
                static_cast<int>(floor(worldPos.x / chunkSize)),
                static_cast<int>(floor(worldPos.y / chunkSize)),
                static_cast<int>(floor(worldPos.z / chunkSize))
            );
        }

        /**
         * @brief Get the center position of this chunk in world space
         * @param chunkSize Size of a chunk
         * @return Center world position
         */
        glm::vec3 toWorldPosition(int chunkSize) const
        {
            return glm::vec3(
                (x * chunkSize) + (chunkSize / 2.0f),
                (y * chunkSize) + (chunkSize / 2.0f),
                (z * chunkSize) + (chunkSize / 2.0f)
            );
        }

        /**
         * @brief String representation of the coordinate
         * @return Formatted string with coordinates
         */
        std::string toString() const
        {
            return "(" + std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(z) + ")";
        }

        /**
         * @brief Manhattan distance from the origin
         * @return Sum of absolute coordinate values
         */
        int manhattanDistance() const
        {
            return std::abs(x) + std::abs(y) + std::abs(z);
        }

        /**
         * @brief Manhattan distance between two chunk coordinates
         * @param other Other coordinate
         * @return Manhattan distance value
         */
        int manhattanDistance(const ChunkCoord& other) const
        {
            return std::abs(x - other.x) + std::abs(y - other.y) + std::abs(z - other.z);
        }

        // Serialization methods
        static void defineSchema(Utility::Serialization::Schema& schema)
        {
            schema.addField("x", Utility::Serialization::ValueType::Int32);
            schema.addField("y", Utility::Serialization::ValueType::Int32);
            schema.addField("z", Utility::Serialization::ValueType::Int32);
        }

        Utility::Serialization::SerializationResult serialize(Utility::Serialization::Serializer& serializer) const
        {
            auto result = serializer.beginObject("ChunkCoord");
            if (!result) return result;

            result = serializer.writeField("x", x);
            if (!result) return result;

            result = serializer.writeField("y", y);
            if (!result) return result;

            result = serializer.writeField("z", z);
            if (!result) return result;

            return serializer.endObject();
        }

        Utility::Serialization::SerializationResult deserialize(Utility::Serialization::Deserializer& deserializer)
        {
            auto result = deserializer.beginObject("ChunkCoord");
            if (!result) return result;

            result = deserializer.readField("x", x);
            if (!result) return result;

            result = deserializer.readField("y", y);
            if (!result) return result;

            result = deserializer.readField("z", z);
            if (!result) return result;

            return deserializer.endObject();
        }

        static std::string getTypeName()
        {
            return "ChunkCoord";
        }
    };

} // namespace PixelCraft::Voxel

// Hash function for ChunkCoord to use in unordered_map/set
namespace std
{
    template<>
    struct hash<PixelCraft::Voxel::ChunkCoord>
    {
        size_t operator()(const PixelCraft::Voxel::ChunkCoord& coord) const
        {
            // Simple hash combining the three coordinates
            size_t h1 = std::hash<int>()(coord.x);
            size_t h2 = std::hash<int>()(coord.y);
            size_t h3 = std::hash<int>()(coord.z);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}