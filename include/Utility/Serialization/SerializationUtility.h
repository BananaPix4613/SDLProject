// -------------------------------------------------------------------------
// Utility/Serialization/SerializationUtility.h
// -------------------------------------------------------------------------
#pragma once

#include "Utility/Serialization/Serializer.h"
#include "Utility/Serialization/Deserializer.h"
#include "Utility/Serialization/BinarySerializer.h"
#include "Utility/Serialization/BinaryDeserializer.h"

#include <fstream>
#include <string>
#include <vector>

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
         * @return Result of the operation
         */
        template<typename T>
        static SerializationResult serializeToFile(const T& obj, const std::string& filename)
        {
            std::ofstream file(filename, std::ios::binary);
            if (!file)
            {
                return SerializationResult("Failed to open file for writing: " + filename);
            }

            BinarySerializer serializer(file);
            auto result = serializer.writeVersion({1, 0, 0});
            if (!result) return result;

            return serializer.writeValue(obj);
        }

        /**
         * @brief Deserialize an object from a binary file
         * @param obj Output object
         * @param filename File to load from
         * @return Result of the operation
         */
        template<typename T>
        static SerializationResult deserializeFromFile(T& obj, const std::string& filename)
        {
            std::ifstream file(filename, std::ios::binary);
            if (!file)
            {
                return SerializationResult("Failed to open file for reading: " + filename);
            }

            BinaryDeserializer deserializer(file);
            auto version = deserializer.getVersion();

            // Version compatibility check could be done here

            return deserializer.readValue(obj);
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