// -------------------------------------------------------------------------
// Utility/Serialization/SerializationUtility.h
// -------------------------------------------------------------------------
#pragma once

#include "Utility/Serialization/BinarySerializer.h"
#include "Utility/Serialization/BinaryDeserializer.h"
#include "Utility/Serialization/SchemaRegistry.h"
#include "Utility/StringUtils.h"

#include <fstream>
#include <string>
#include <vector>

using StringUtils = PixelCraft::Utility::StringUtils;

namespace PixelCraft::Utility::Serialization
{

    /**
     * @brief Helper functions for serialization
     */
    class SerializationUtility
    {
    public:
        /**
         * @brief Serialize an object to a binary file
         * @param obj Object to serialize
         * @param filename File to save to
         * @return SerializationResult with success/failure status
         */
        template<typename T>
        static SerializationResult serializeToFile(const T& obj, const std::string& filename)
        {
            // Create output file stream
            std::ofstream file(filename, std::ios::binary);
            if (!file)
            {
                return SerializationResult("Failed to open file for writing: " + filename);
            }

            // Create binary serializer
            BinarySerializer serializer(file);

            // Write version info if the object supports it
            auto result = serializer.writeVersion(getVersionInfo<T>());
            if (!result) return result;

            // Serialize the object
            return serializer.writeValue(obj);
        }
        
        /**
         * @brief Deserialize an object from a binary file
         * @param obj Output object
         * @param filename File to load from
         * @return SerializationResult with success/failure status
         */
        template<typename T>
        static SerializationResult deserializeFromFile(T& obj, const std::string& filename)
        {
            // Create input file stream
            std::ifstream file(filename, std::ios::binary);
            if (!file)
            {
                return SerializationResult("Failed to open file for reading: " + filename);
            }

            // Create binary deserializer
            BinaryDeserializer deserializer(file);
            auto version = deserializer.getVersion();

            // Check version compatibility
            if (!isCompatibleVersion<T>(version))
            {
                std::string msgResult = StringUtils::format("Incompatible version: {}.{}.{}", version.major, version.minor, version.patch);
                return SerializationResult(msgResult);
            }

            return deserializer.readValue(obj);
        }

        /**
         * @brief Get version info for a type
         * @return Version info for the type
         */
        template<typename T>
        static VersionInfo getVersionInfo()
        {
            // Try to get schema for this type
            auto schema = SchemaRegistry::getInstance().getSchema(T::getTypeName());
            if (schema)
            {
                return schema->getVersion();
            }

            // Default version if no schema found
            return {1, 0, 0};
        }

        /**
         * @brief Check if a version is compatible with a type
         * @param version Version to check
         * @return True if version is compatible
         */
        template<typename T>
        static bool isCompatibleVersion(const VersionInfo& version)
        {
            // Get expected version for this type
            VersionInfo expected = getVersionInfo<T>();

            // Major version must match, minor can be higher in the file
            return version.major == expected.major &&
                version.minor >= expected.minor;
        }

        /**
         * @brief Register schema for a type if not already registered
         */
        template<typename T>
        static void registerSchemaIfNeeded()
        {
            if (!SchemaRegistry::getInstance().hasSchema(T::getTypeName()))
            {
                auto schema = std::make_shared<Schema>(T::getTypeName(), VersionInfo{1, 0, 0});
                T::defineSchema(*schema);
                SchemaRegistry::getInstance().registerSchema(schema);
            }
        }

        /**
         * @brief Serialize an object to a binary buffer
         * @param obj Object to serialize
         * @return Binary buffer containing serialized data
         */
        template<typename T>
        static std::vector<uint8_t> serializeToBuffer(const T& obj)
        {
            std::ostringstream stream;
            BinarySerializer serializer(stream);
            serializer.writeVersion({1, 0, 0});
            serializer.writeValue(obj);

            std::string str = stream.str();
            return std::vector<uint8_t>(str.begin(), str.end());
        }

        /**
         * @brief Deserialize an object from a binary buffer
         * @param obj Output object
         * @param buffer Binary buffer containing serialized data
         * @param size Buffer size in bytes
         * @return Result of the operation
         */
        template<typename T>
        static SerializationResult deserializeFromBuffer(T& obj, const uint8_t* buffer, size_t size)
        {
            std::istringstream stream(std::string(reinterpret_cast<const char*>(buffer), size));
            BinaryDeserializer deserializer(stream);
            return deserializer.readValue(obj);
        }
    };

} // namespace PixelCraft::Utility::Serialization