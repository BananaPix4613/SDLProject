// -------------------------------------------------------------------------
// UUID.h
// -------------------------------------------------------------------------
#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace PixelCraft::ECS
{

    class Serializer;
    class Deserializer;

    /**
     * @brief Universally unique identifier for entity persistence in ECS architecture
     *
     * The UUID class provides a standards-compliant implementation of RFC 4122 UUIDs
     * (Universally Unique Identifiers) for use within the PixelCraft ECS system.
     * UUIDs allow for consistent identification of entities across application sessions,
     * serialization/deserialization, and networked environments.
     *
     * This implementation supports:
     * - Random UUID generation (version 4)
     * - Name-based UUID generation (version 5)
     * - String conversion and parsing
     * - Comparison operations for container usage
     * - Serialization support
     * - Null UUID detection
     */
    class UUID
    {
    public:
        /**
         * @brief Default constructor creates a random UUID (version 4)
         */
        UUID();

        /**
         * @brief Copy constructor
         * @param other UUID to copy
         */
        UUID(const UUID& other);

        /**
         * @brief Move constructor
         * @param other UUID to move from
         */
        UUID(UUID&& other) noexcept;

        /**
         * @brief Construct from raw bytes
         * @param bytes Array of 16 bytes representing the UUID
         */
        explicit UUID(const std::array<uint8_t, 16>& bytes);

        /**
         * @brief Copy assignment operator
         * @param other UUID to copy
         * @return Reference to this UUID
         */
        UUID& operator=(const UUID& other);

        /**
         * @brief Move assignment operator
         * @param other UUID to move from
         * @return Reference to this UUID
         */
        UUID& operator=(UUID&& other) noexcept;

        /**
         * @brief Create a UUID from a string representation
         *
         * @param str String in standard UUID format "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
         * @return UUID parsed from the string, or null UUID if parsing fails
         */
        static UUID fromString(const std::string& str);

        /**
         * @brief Convert UUID to string representation
         *
         * @return String in standard UUID format "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
         */
        std::string toString() const;

        /**
         * @brief Check if this UUID is null (all zeros)
         *
         * @return true if this is a null UUID
         * @return false otherwise
         */
        bool isNull() const;

        /**
         * @brief Equality operator
         *
         * @param other UUID to compare with
         * @return true if UUIDs are equal
         * @return false otherwise
         */
        bool operator==(const UUID& other) const;

        /**
         * @brief Inequality operator
         *
         * @param other UUID to compare with
         * @return true if UUIDs are not equal
         * @return false otherwise
         */
        bool operator!=(const UUID& other) const;

        /**
         * @brief Less than operator for ordered container support
         *
         * @param other UUID to compare with
         * @return true if this UUID is less than the other
         * @return false otherwise
         */
        bool operator<(const UUID& other) const;

        /**
         * @brief Get raw UUID data
         *
         * @return Const reference to the 16-byte array
         */
        const std::array<uint8_t, 16>& getData() const
        {
            return m_data;
        }

        /**
         * @brief Serialize this UUID
         *
         * @param serializer Serializer to write to
         */
        void serialize(Serializer& serializer);

        /**
         * @brief Deserialize this UUID
         *
         * @param deserializer Deserializer to read from
         */
        void deserialize(Deserializer& deserializer);

        /**
         * @brief Create a random UUID (version 4)
         *
         * Generates a new UUID using a cryptographically secure
         * random number generator with proper thread safety.
         *
         * @return Randomly generated UUID
         */
        static UUID createRandom();

        /**
         * @brief Create a deterministic UUID from a name (version 5)
         *
         * Generates a deterministic UUID based on the provided name.
         * The same name will always generate the same UUID.
         *
         * @param name String name to generate UUID from
         * @return Deterministically generated UUID
         */
        static UUID createFromName(const std::string& name);

        /**
         * @brief Create a null UUID (all zeros)
         *
         * @return Null UUID
         */
        static UUID createNull();

    private:
        /** Raw 16-byte storage for the UUID */
        std::array<uint8_t, 16> m_data;

        /**
         * @brief Convert string representation to byte array
         *
         * @param str String in UUID format "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
         * @param bytes Array to store the result in
         */
        static void stringToBytes(const std::string& str, std::array<uint8_t, 16>& bytes);

        /**
         * @brief Convert byte array to string representation
         *
         * @param bytes Byte array to convert
         * @return String in UUID format "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
         */
        static std::string bytesToString(const std::array<uint8_t, 16>& bytes);
    };

} // namespace PixelCraft::ECS

// Hash function for using UUID in unordered containers
namespace std
{
    template<>
    struct hash<PixelCraft::ECS::UUID>
    {
        /**
         * @brief Hash function implementation for UUID
         *
         * @param uuid UUID to hash
         * @return size_t Hash value
         */
        size_t operator()(const PixelCraft::ECS::UUID& uuid) const
        {
            const auto& data = uuid.getData();
            size_t result = 0;

            for (size_t i = 0; i < data.size(); i += sizeof(size_t))
            {
                size_t chunk = 0;
                for (size_t j = 0; j < sizeof(size_t) && i + j < data.size(); ++j)
                {
                    chunk |= static_cast<size_t>(data[i + j]) << (j * 8);
                }
                result ^= chunk;
            }

            return result;
        }
    };
}