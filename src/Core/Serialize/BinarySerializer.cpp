// -------------------------------------------------------------------------
// BinarySerializer.cpp
// -------------------------------------------------------------------------
#include "Core/Serialize/BinarySerializer.h"
#include "Core/Serialize/DataNode.h"
#include "Core/Logger.h"
#include "Utility/FileSystem.h"

#include <filesystem>

// For compression
#include <zlib.h>

// For Base64 encoding/decoding
#include <algorithm>
#include <string>

namespace PixelCraft::Core::Serialize
{

    // Binary format magic header to identify valid files
    constexpr uint32_t BINARY_FORMAT_MAGIC = 0x5043424E; // "PCBN" (PixelCraft Binary)
    constexpr uint32_t CURRENT_VERSION = 1;

    // Binary type identifiers
    enum class BinaryNodeType : uint8_t
    {
        Null = 0,
        Bool = 1,
        Int32 = 2,
        Float = 3,
        String = 4,
        Array = 5,
        Object = 6
    };

    BinarySerializer::BinarySerializer()
        : Serializer(SerializationFormat::Binary)
        , m_readPosition(0)
        , m_compression(false)
        , m_version(CURRENT_VERSION)
    {
    }

    BinarySerializer::~BinarySerializer()
    {
        shutdown();
    }

    bool BinarySerializer::initialize()
    {
        m_buffer.clear();
        m_readPosition = 0;
        m_errorMessage.clear();
        return true;
    }

    void BinarySerializer::shutdown()
    {
        m_buffer.clear();
        m_readPosition = 0;
    }

    bool BinarySerializer::serialize(const DataNode& node)
    {
        try
        {
            // Reset buffer and error state
            m_buffer.clear();
            m_errorMessage.clear();

            // Write magic header and version
            write(BINARY_FORMAT_MAGIC);
            write(m_version);

            // Serialize the node
            serializeNode(node);

            // Apply compression if enabled
            if (m_compression)
            {
                compressBuffer();
            }

            return true;
        }
        catch (const std::exception& e)
        {
            m_errorMessage = "Failed to serialize node: " + std::string(e.what());
            error(m_errorMessage);
            return false;
        }
    }

    bool BinarySerializer::deserialize(DataNode& node)
    {
        try
        {
            if (m_buffer.empty())
            {
                m_errorMessage = "No data to deserialize";
                error(m_errorMessage);
                return false;
            }

            // Reset read position and error state
            m_readPosition = 0;
            m_errorMessage.clear();

            // Read and validate magic header
            uint32_t magic = read<uint32_t>();
            if (magic != BINARY_FORMAT_MAGIC)
            {
                m_errorMessage = "Invalid binary format: magic header mismatch";
                error(m_errorMessage);
                return false;
            }

            // Read version
            uint32_t version = read<uint32_t>();
            if (version > m_version)
            {
                m_errorMessage = "Unsupported binary format version: " + std::to_string(version);
                error(m_errorMessage);
                return false;
            }

            // Deserialize the node
            deserializeNode(node);

            return !hasError();
        }
        catch (const std::exception& e)
        {
            m_errorMessage = "Failed to deserialize data: " + std::string(e.what());
            error(m_errorMessage);
            return false;
        }
    }

    bool BinarySerializer::writeToFile(const std::string& filePath)
    {
        try
        {
            // Create parent directories if they don't exist
            std::filesystem::path path(filePath);
            if (!std::filesystem::exists(path.parent_path()))
            {
                std::filesystem::create_directories(path.parent_path());
            }

            // Open file for binary writing
            std::ofstream file(filePath, std::ios::binary);
            if (!file.is_open())
            {
                m_errorMessage = "Failed to open file for writing: " + filePath;
                error(m_errorMessage);
                return false;
            }

            // Write buffer data to file
            file.write(reinterpret_cast<const char*>(m_buffer.data()), m_buffer.size());

            if (file.fail())
            {
                m_errorMessage = "Failed to write data to file: " + filePath;
                error(m_errorMessage);
                return false;
            }

            file.close();
            return true;
        }
        catch (const std::exception& e)
        {
            m_errorMessage = "Failed to write to file: " + std::string(e.what());
            error(m_errorMessage);
            return false;
        }
    }

    bool BinarySerializer::readFromFile(const std::string& filePath)
    {
        try
        {
            // Check if file exists
            if (!std::filesystem::exists(filePath))
            {
                m_errorMessage = "File does not exist: " + filePath;
                error(m_errorMessage);
                return false;
            }

            // Get file size
            std::filesystem::path path(filePath);
            size_t fileSize = std::filesystem::file_size(path);

            // Open file for binary reading
            std::ifstream file(filePath, std::ios::binary);
            if (!file.is_open())
            {
                m_errorMessage = "Failed to open file for reading: " + filePath;
                error(m_errorMessage);
                return false;
            }

            // Resize buffer to file size
            m_buffer.resize(fileSize);

            // Read file data into buffer
            file.read(reinterpret_cast<char*>(m_buffer.data()), fileSize);

            if (file.fail())
            {
                m_errorMessage = "Failed to read data from file: " + filePath;
                error(m_errorMessage);
                return false;
            }

            // Decompress if compression is enabled
            if (m_compression)
            {
                if (!decompressBuffer())
                {
                    return false;
                }
            }

            file.close();
            return true;
        }
        catch (const std::exception& e)
        {
            m_errorMessage = "Failed to read from file: " + std::string(e.what());
            error(m_errorMessage);
            return false;
        }
    }

    bool BinarySerializer::writeToStream(std::ostream& stream)
    {
        try
        {
            // Write buffer data to stream
            stream.write(reinterpret_cast<const char*>(m_buffer.data()), m_buffer.size());

            if (stream.fail())
            {
                m_errorMessage = "Failed to write data to stream";
                error(m_errorMessage);
                return false;
            }

            return true;
        }
        catch (const std::exception& e)
        {
            m_errorMessage = "Failed to write to stream: " + std::string(e.what());
            error(m_errorMessage);
            return false;
        }
    }

    bool BinarySerializer::readFromStream(std::istream& stream)
    {
        try
        {
            // Get current position
            std::streampos startPos = stream.tellg();

            // Seek to end to determine size
            stream.seekg(0, std::ios::end);
            std::streampos endPos = stream.tellg();

            // Calculate size and seek back to start
            size_t size = static_cast<size_t>(endPos - startPos);
            stream.seekg(startPos);

            // Resize buffer to stream size
            m_buffer.resize(size);

            // Read stream data into buffer
            stream.read(reinterpret_cast<char*>(m_buffer.data()), size);

            if (stream.fail())
            {
                m_errorMessage = "Failed to read data from stream";
                error(m_errorMessage);
                return false;
            }

            // Decompress if compression is enabled
            if (m_compression)
            {
                if (!decompressBuffer())
                {
                    return false;
                }
            }

            return true;
        }
        catch (const std::exception& e)
        {
            m_errorMessage = "Failed to read from stream: " + std::string(e.what());
            error(m_errorMessage);
            return false;
        }
    }

    std::string BinarySerializer::toString()
    {
        // Convert binary data to Base64 string
        static const char base64_chars[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string result;
        size_t len = m_buffer.size();

        // Reserve appropriate space in output string
        result.reserve(((len / 3) + (len % 3 > 0)) * 4);

        // Process 3 bytes at a time
        for (size_t i = 0; i < len; i += 3)
        {
            uint32_t chunk = static_cast<uint32_t>(m_buffer[i]) << 16;

            if (i + 1 < len)
            {
                chunk |= static_cast<uint32_t>(m_buffer[i + 1]) << 8;
            }

            if (i + 2 < len)
            {
                chunk |= static_cast<uint32_t>(m_buffer[i + 2]);
            }

            // Output 4 characters for the chunk
            result += base64_chars[(chunk & 0x00FC0000) >> 18];
            result += base64_chars[(chunk & 0x0003F000) >> 12];

            if (i + 1 < len)
            {
                result += base64_chars[(chunk & 0x00000FC0) >> 6];
            }
            else
            {
                result += '=';
            }

            if (i + 2 < len)
            {
                result += base64_chars[(chunk & 0x0000003F)];
            }
            else
            {
                result += '=';
            }
        }

        return result;
    }

    bool BinarySerializer::fromString(const std::string& data)
    {
        // Convert Base64 string to binary data
        static uint8_t base64_index[256] = {0};
        static bool base64_initialized = false;

        if (!base64_initialized)
        {
            for (int i = 0; i < 64; i++)
            {
                if (i < 26) base64_index['A' + i] = i;
                else if (i < 52) base64_index['a' + (i - 26)] = i;
                else if (i < 62) base64_index['0' + (i - 52)] = i;
                else if (i == 62) base64_index['+'] = i;
                else if (i == 63) base64_index['/'] = i;
            }
            base64_initialized = true;
        }

        // Remove whitespace from input
        std::string cleanData;
        cleanData.reserve(data.size());
        for (char c : data)
        {
            if (!std::isspace(c))
            {
                cleanData += c;
            }
        }

        // Check for valid Base64 string
        if (cleanData.length() % 4 != 0)
        {
            m_errorMessage = "Invalid Base64 data: length not a multiple of 4";
            error(m_errorMessage);
            return false;
        }

        // Estimate output size
        size_t padding = 0;
        if (!cleanData.empty())
        {
            if (cleanData[cleanData.length() - 1] == '=') padding++;
            if (cleanData[cleanData.length() - 2] == '=') padding++;
        }

        size_t outputLen = ((cleanData.length() / 4) * 3) - padding;
        m_buffer.resize(outputLen);

        // Process 4 characters at a time
        size_t j = 0;
        for (size_t i = 0; i < cleanData.length(); i += 4)
        {
            // Get values of Base64 characters
            uint32_t a = (cleanData[i] == '=') ? 0 : base64_index[static_cast<uint8_t>(cleanData[i])];
            uint32_t b = (cleanData[i + 1] == '=') ? 0 : base64_index[static_cast<uint8_t>(cleanData[i + 1])];
            uint32_t c = (cleanData[i + 2] == '=') ? 0 : base64_index[static_cast<uint8_t>(cleanData[i + 2])];
            uint32_t d = (cleanData[i + 3] == '=') ? 0 : base64_index[static_cast<uint8_t>(cleanData[i + 3])];

            // Combine into 24-bit chunk
            uint32_t triple = (a << 18) | (b << 12) | (c << 6) | d;

            // Extract bytes
            if (j < outputLen) m_buffer[j++] = (triple >> 16) & 0xFF;
            if (j < outputLen) m_buffer[j++] = (triple >> 8) & 0xFF;
            if (j < outputLen) m_buffer[j++] = triple & 0xFF;
        }

        // Decompress if compression is enabled
        if (m_compression)
        {
            if (!decompressBuffer())
            {
                return false;
            }
        }

        return true;
    }

    bool BinarySerializer::hasError() const
    {
        return !m_errorMessage.empty();
    }

    std::string BinarySerializer::getErrorMessage() const
    {
        return m_errorMessage;
    }

    void BinarySerializer::setCompression(bool enabled)
    {
        m_compression = enabled;
    }

    bool BinarySerializer::isCompressionEnabled() const
    {
        return m_compression;
    }

    void BinarySerializer::setVersion(uint32_t version)
    {
        m_version = version;
    }

    uint32_t BinarySerializer::getVersion() const
    {
        return m_version;
    }

    void BinarySerializer::serializeNode(const DataNode& node)
    {
        BinaryNodeType type;

        switch (node.getType())
        {
            case DataNode::Type::Null:
                type = BinaryNodeType::Null;
                write(type);
                break;

            case DataNode::Type::Bool:
                type = BinaryNodeType::Bool;
                write(type);
                write(node.getValue<bool>());
                break;

            case DataNode::Type::Int:
                type = BinaryNodeType::Int32;
                write(type);
                write(node.getValue<int>());
                break;

            case DataNode::Type::Float:
                type = BinaryNodeType::Float;
                write(type);
                write(node.getValue<float>());
                break;

            case DataNode::Type::String:
                type = BinaryNodeType::String;
                write(type);
                writeString(node.getValue<std::string>());
                break;

            case DataNode::Type::Array:
            {
                type = BinaryNodeType::Array;
                write(type);

                // Write array size
                const auto& elements = node.getArrayElements();
                uint32_t arraySize = static_cast<uint32_t>(elements.size());
                write(arraySize);

                // Write array elements
                for (const auto& childNode : elements)
                {
                    serializeNode(childNode);
                }
                break;
            }

            case DataNode::Type::Object:
            {
                type = BinaryNodeType::Object;
                write(type);

                // Write object size
                const auto& elements = node.getObjectElements();
                uint32_t objectSize = static_cast<uint32_t>(elements.size());
                write(objectSize);

                // Write object key-value pairs
                for (const auto& [key, childNode] : elements)
                {
                    writeString(key);
                    serializeNode(childNode);
                }
                break;
            }

            default:
                // Unsupported type, write as null
                type = BinaryNodeType::Null;
                write(type);
                warn("Unsupported DataNode type encountered during binary serialization");
                break;
        }
    }

    void BinarySerializer::deserializeNode(DataNode& node)
    {
        BinaryNodeType type = read<BinaryNodeType>();

        switch (type)
        {
            case BinaryNodeType::Null:
                node.setType(DataNode::Type::Null);
                break;

            case BinaryNodeType::Bool:
                node.setType(DataNode::Type::Bool);
                node.setValue(read<bool>());
                break;

            case BinaryNodeType::Int32:
                node.setType(DataNode::Type::Int);
                node.setValue(read<int32_t>());
                break;

            case BinaryNodeType::Float:
                node.setType(DataNode::Type::Float);
                node.setValue(read<float>());
                break;

            case BinaryNodeType::String:
                node.setType(DataNode::Type::String);
                node.setValue(readString());
                break;

            case BinaryNodeType::Array:
                node.setType(DataNode::Type::Array);

                // Read array size
                uint32_t arraySize = read<uint32_t>();

                // Read array elements
                for (uint32_t i = 0; i < arraySize; i++)
                {
                    DataNode childNode;
                    deserializeNode(childNode);
                    node.addChild(std::move(childNode));
                }
                break;

            case BinaryNodeType::Object:
                node.setType(DataNode::Type::Object);

                // Read object size
                uint32_t objectSize = read<uint32_t>();

                // Read object key-value pairs
                for (uint32_t i = 0; i < objectSize; i++)
                {
                    std::string key = readString();
                    DataNode childNode;
                    deserializeNode(childNode);
                    node.addChild(key, std::move(childNode));
                }
                break;

            default:
                m_errorMessage = "Invalid binary node type encountered";
                error(m_errorMessage);
                node.setType(DataNode::Type::Null);
                break;
        }
    }

    void BinarySerializer::writeString(const std::string& str)
    {
        // Write string length
        uint32_t length = static_cast<uint32_t>(str.length());
        write(length);

        // Write string data
        if (length > 0)
        {
            size_t currentSize = m_buffer.size();
            m_buffer.resize(currentSize + length);
            std::memcpy(m_buffer.data() + currentSize, str.data(), length);
        }
    }

    std::string BinarySerializer::readString()
    {
        // Read string length
        uint32_t length = read<uint32_t>();

        // Read string data
        std::string str;
        if (length > 0)
        {
            if (m_readPosition + length <= m_buffer.size())
            {
                str.resize(length);
                std::memcpy(&str[0], m_buffer.data() + m_readPosition, length);
                m_readPosition += length;
            }
            else
            {
                m_errorMessage = "Buffer read position out of bounds";
                error(m_errorMessage);
            }
        }

        return str;
    }

    void BinarySerializer::compressBuffer()
    {
        // Save original uncompressed buffer
        std::vector<uint8_t> originalBuffer = m_buffer;

        // Get maximum compressed buffer size
        uLong compressedSize = compressBound(static_cast<uLong>(originalBuffer.size()));

        // Prepare compressed buffer
        m_buffer.resize(compressedSize + sizeof(uint32_t));

        // Store original size at the beginning
        uint32_t originalSize = static_cast<uint32_t>(originalBuffer.size());
        std::memcpy(m_buffer.data(), &originalSize, sizeof(uint32_t));

        // Compress data
        int result = compress(
            m_buffer.data() + sizeof(uint32_t),
            &compressedSize,
            originalBuffer.data(),
            static_cast<uLong>(originalBuffer.size())
        );

        if (result != Z_OK)
        {
            // Compression failed, revert to uncompressed data
            m_buffer = originalBuffer;
            warn("Compression failed, using uncompressed data");
        }
        else
        {
            // Resize buffer to actual compressed size plus the size header
            m_buffer.resize(compressedSize + sizeof(uint32_t));
        }
    }

    bool BinarySerializer::decompressBuffer()
    {
        // Check if buffer is large enough to contain size header
        if (m_buffer.size() < sizeof(uint32_t))
        {
            m_errorMessage = "Invalid compressed data: buffer too small";
            error(m_errorMessage);
            return false;
        }

        // Read original size
        uint32_t originalSize;
        std::memcpy(&originalSize, m_buffer.data(), sizeof(uint32_t));

        // Create buffer for decompressed data
        std::vector<uint8_t> decompressedBuffer(originalSize);

        // Decompress data
        uLong destLen = static_cast<uLong>(originalSize);
        int result = uncompress(
            decompressedBuffer.data(),
            &destLen,
            m_buffer.data() + sizeof(uint32_t),
            static_cast<uLong>(m_buffer.size() - sizeof(uint32_t))
        );

        if (result != Z_OK)
        {
            m_errorMessage = "Decompression failed";
            error(m_errorMessage);
            return false;
        }

        // Replace buffer with decompressed data
        m_buffer = std::move(decompressedBuffer);
        return true;
    }

} // namespace PixelCraft::Core::Serialize