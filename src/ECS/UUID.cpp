// -------------------------------------------------------------------------
// UUID.cpp
// -------------------------------------------------------------------------
#include "ECS/UUID.h"
#include "Utility/Serialization/Serializer.h"
#include "Utility/Serialization/Deserializer.h"
#include "Core/Logger.h"

#include <random>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <functional>
#include <mutex>
#include <chrono>

// SHA-1 implementation for name-based UUIDs
#include <openssl/evp.h>

namespace
{
    // Thread-local random number generator for UUID generation
    thread_local std::mt19937_64 randomEngine;
    thread_local bool randomEngineInitialized = false;

    // Mutex for singleton initialization
    std::mutex initMutex;

    // Namespace for name-based UUIDs (using a standard namespace from RFC 4122)
    // This is the namespace for DNS names
    const std::array<uint8_t, 16> DNS_NAMESPACE = {
        0x6b, 0xa7, 0xb8, 0x10, 0x9d, 0xad, 0x11, 0xd1,
        0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8
    };

    // Ensure random engine is properly initialized for this thread
    void ensureRandomEngineInitialized()
    {
        if (!randomEngineInitialized)
        {
            std::lock_guard<std::mutex> lock(initMutex);

            // Seed with high-quality entropy
            std::random_device rd;
            std::array<uint64_t, 4> seedData;
            for (auto& elem : seedData)
            {
                elem = rd();
            }

            // Also use the current time for added entropy
            auto now = std::chrono::high_resolution_clock::now();
            auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
                now.time_since_epoch()).count();

            // Initialize with good entropy from both sources
            std::seed_seq seeds{rd(), rd(), rd(), rd(),
                static_cast<uint32_t>(nanos),
                static_cast<uint32_t>(nanos >> 32)};
            randomEngine.seed(seeds);
            randomEngineInitialized = true;
        }
    }
}

namespace PixelCraft::ECS
{

    UUID::UUID()
    {
        *this = createRandom();
    }

    UUID::UUID(const UUID& other) : m_data(other.m_data)
    {
    }

    UUID::UUID(UUID&& other) noexcept : m_data(std::move(other.m_data))
    {
    }

    UUID::UUID(const std::array<uint8_t, 16>& bytes) : m_data(bytes)
    {
    }

    UUID& UUID::operator=(const UUID& other)
    {
        if (this != &other)
        {
            m_data = other.m_data;
        }
        return *this;
    }

    UUID& UUID::operator=(UUID&& other) noexcept
    {
        if (this != &other)
        {
            m_data = std::move(other.m_data);
        }
        return *this;
    }

    UUID UUID::fromString(const std::string& str)
    {
        std::array<uint8_t, 16> bytes;
        stringToBytes(str, bytes);
        return UUID(bytes);
    }

    std::string UUID::toString() const
    {
        return bytesToString(m_data);
    }

    bool UUID::isNull() const
    {
        // Check if all bytes are zero
        return std::all_of(m_data.begin(), m_data.end(), [](uint8_t b) { return b == 0; });
    }

    bool UUID::operator==(const UUID& other) const
    {
        return m_data == other.m_data;
    }

    bool UUID::operator!=(const UUID& other) const
    {
        return !(*this == other);
    }

    bool UUID::operator<(const UUID& other) const
    {
        // Lexicographical comparison of the byte arrays
        return std::lexicographical_compare(
            m_data.begin(), m_data.end(),
            other.m_data.begin(), other.m_data.end()
        );
    }

    Utility::Serialization::SerializationResult UUID::serialize(Utility::Serialization::Serializer& serializer) const
    {
        auto result = serializer.beginObject("UUID");
        if (!result) return result;

        result = serializer.writeField("data", bytesToString(m_data));
        if (!result) return result;

        return serializer.endObject();
    }

    Utility::Serialization::SerializationResult UUID::deserialize(Utility::Serialization::Deserializer& deserializer)
    {
        auto result = deserializer.beginObject("UUID");
        if (!result) return result;

        std::string dataString;
        result = deserializer.readField("data", dataString);
        if (!result) return result;

        stringToBytes(dataString, m_data);

        return deserializer.endObject();
    }

    void UUID::defineSchema(Utility::Serialization::Schema& schema)
    {
        schema.addField("data", Utility::Serialization::ValueType::String);
    }

    std::string UUID::getTypeName()
    {
        return "UUID";
    }

    UUID UUID::createRandom()
    {
        std::array<uint8_t, 16> bytes;

        // Ensure the random engine is initialized
        ensureRandomEngineInitialized();

        // Fill with random bytes
        std::uniform_int_distribution<uint16_t> dist(0, 255);
        for (size_t i = 0; i < bytes.size(); ++i)
        {
            bytes[i] = static_cast<uint8_t>(dist(randomEngine));
        }

        // Set version to 4 (random)
        bytes[6] = (bytes[6] & 0x0F) | 0x40;

        // Set variant to RFC 4122
        bytes[8] = (bytes[8] & 0x3F) | 0x80;

        return UUID(bytes);
    }

    UUID UUID::createFromName(const std::string& name)
    {
        // Version 5 UUIDs are created by hashing a namespace UUID and a name
        std::array<uint8_t, 16> result;

        // Create a SHA-1 hash of the namespace concatenated with the name using EVP interface
        std::array<uint8_t, 20> hash;
        EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
        if (mdctx == nullptr) {
            Core::error("Failed to create EVP_MD_CTX for SHA-1.");
            std::fill(result.begin(), result.end(), 0);
            return UUID(result);
        }
        if (EVP_DigestInit_ex(mdctx, EVP_sha1(), nullptr) != 1 ||
            EVP_DigestUpdate(mdctx, DNS_NAMESPACE.data(), DNS_NAMESPACE.size()) != 1 ||
            EVP_DigestUpdate(mdctx, reinterpret_cast<const uint8_t*>(name.data()), name.size()) != 1 ||
            EVP_DigestFinal_ex(mdctx, hash.data(), nullptr) != 1) {
            Core::error("SHA-1 digest failed in UUID::createFromName.");
            EVP_MD_CTX_free(mdctx);
            std::fill(result.begin(), result.end(), 0);
            return UUID(result);
        }
        EVP_MD_CTX_free(mdctx);

        // Use the first 16 bytes of the hash for the UUID
        std::copy(hash.begin(), hash.begin() + 16, result.begin());

        // Set version to 5 (name-based SHA-1)
        result[6] = (result[6] & 0x0F) | 0x50;

        // Set variant to RFC 4122
        result[8] = (result[8] & 0x3F) | 0x80;

        return UUID(result);
    }

    UUID UUID::createNull()
    {
        std::array<uint8_t, 16> bytes{};
        // Fill with zeros
        std::fill(bytes.begin(), bytes.end(), 0);
        return UUID(bytes);
    }

    void UUID::stringToBytes(const std::string& str, std::array<uint8_t, 16>& bytes)
    {
        // Clear the bytes array first
        std::fill(bytes.begin(), bytes.end(), 0);

        // Validate string format: 8-4-4-4-12 (36 chars total including hyphens)
        if (str.length() != 36 ||
            str[8] != '-' || str[13] != '-' ||
            str[18] != '-' || str[23] != '-')
        {
            Core::error("Invalid UUID format: " + str + ". Expected xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx");
            return;
        }

        // Parse the string
        try
        {
            // Remove hyphens to get a continuous hex string
            std::string hexStr = str.substr(0, 8) +
                str.substr(9, 4) +
                str.substr(14, 4) +
                str.substr(19, 4) +
                str.substr(24, 12);

            // Convert each byte (2 hex chars)
            for (size_t i = 0; i < 16; ++i)
            {
                std::string byteStr = hexStr.substr(i * 2, 2);
                bytes[i] = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
            }
        }
        catch (const std::exception& e)
        {
            Core::error("Failed to parse UUID: " + str + " - " + e.what());
            // Reset to null UUID on failure
            std::fill(bytes.begin(), bytes.end(), 0);
        }
    }

    std::string UUID::bytesToString(const std::array<uint8_t, 16>& bytes)
    {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');

        // Format: 8-4-4-4-12
        // First 4 bytes
        for (size_t i = 0; i < 4; ++i)
        {
            ss << std::setw(2) << static_cast<int>(bytes[i]);
        }
        ss << '-';

        // Next 2 bytes
        for (size_t i = 4; i < 6; ++i)
        {
            ss << std::setw(2) << static_cast<int>(bytes[i]);
        }
        ss << '-';

        // Next 2 bytes
        for (size_t i = 6; i < 8; ++i)
        {
            ss << std::setw(2) << static_cast<int>(bytes[i]);
        }
        ss << '-';

        // Next 2 bytes
        for (size_t i = 8; i < 10; ++i)
        {
            ss << std::setw(2) << static_cast<int>(bytes[i]);
        }
        ss << '-';

        // Last 6 bytes
        for (size_t i = 10; i < 16; ++i)
        {
            ss << std::setw(2) << static_cast<int>(bytes[i]);
        }

        return ss.str();
    }

} // namespace PixelCraft::ECS