// -------------------------------------------------------------------------
// StringUtils.cpp
// -------------------------------------------------------------------------
#include "Utility/StringUtils.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <functional>
#include <filesystem>
#include <openssl/md5.h>
#include <openssl/sha.h>

namespace PixelCraft::Utility
{

    // -------------------------------------------------------------------------
    // Trimming Functions
    // -------------------------------------------------------------------------

    std::string StringUtils::trim(const std::string& str)
    {
        return trimRight(trimLeft(str));
    }

    std::string StringUtils::trimLeft(const std::string& str)
    {
        if (str.empty())
        {
            return str;
        }

        auto firstNonSpace = std::find_if(str.begin(), str.end(), [](unsigned char ch) {
            return !std::isspace(ch);
                                          });

        return std::string(firstNonSpace, str.end());
    }

    std::string StringUtils::trimRight(const std::string& str)
    {
        if (str.empty())
        {
            return str;
        }

        auto lastNonSpace = std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
                                         }).base();

        return std::string(str.begin(), lastNonSpace);
    }

    // -------------------------------------------------------------------------
    // Case Conversion Functions
    // -------------------------------------------------------------------------

    std::string StringUtils::toUpper(const std::string& str)
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
            return std::toupper(c);
                       });
        return result;
    }

    std::string StringUtils::toLower(const std::string& str)
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
            return std::tolower(c);
                       });
        return result;
    }

    std::string StringUtils::toTitleCase(const std::string& str)
    {
        if (str.empty())
        {
            return str;
        }

        std::string result = str;
        bool capitalizeNext = true;

        for (auto& c : result)
        {
            if (std::isspace(c) || c == '-' || c == '_')
            {
                capitalizeNext = true;
            }
            else if (capitalizeNext)
            {
                c = std::toupper(c);
                capitalizeNext = false;
            }
            else
            {
                c = std::tolower(c);
            }
        }

        return result;
    }

    std::string StringUtils::toCamelCase(const std::string& str)
    {
        if (str.empty())
        {
            return str;
        }

        // First convert to title case
        std::string result = toTitleCase(str);

        // Then make the first character lowercase
        if (!result.empty())
        {
            result[0] = std::tolower(result[0]);
        }

        // Remove spaces, hyphens, and underscores
        result.erase(std::remove_if(result.begin(), result.end(),
                                    [](unsigned char c) { return std::isspace(c) || c == '-' || c == '_'; }),
                     result.end());

        return result;
    }

    std::string StringUtils::toPascalCase(const std::string& str)
    {
        if (str.empty())
        {
            return str;
        }

        // Convert to title case
        std::string result = toTitleCase(str);

        // Remove spaces, hyphens, and underscores
        result.erase(std::remove_if(result.begin(), result.end(),
                                    [](unsigned char c) { return std::isspace(c) || c == '-' || c == '_'; }),
                     result.end());

        return result;
    }

    std::string StringUtils::toSnakeCase(const std::string& str)
    {
        if (str.empty())
        {
            return str;
        }

        std::string result;
        result.reserve(str.size() * 2); // Allocate more space to avoid reallocations

        for (size_t i = 0; i < str.size(); ++i)
        {
            char c = str[i];

            // Skip spaces, hyphens, and underscores
            if (std::isspace(c) || c == '-' || c == '_')
            {
                if (!result.empty() && result.back() != '_')
                {
                    result.push_back('_');
                }
                continue;
            }

            // If uppercase and not the first character, add underscore before it
            if (std::isupper(c) && i > 0 && result.back() != '_')
            {
                result.push_back('_');
            }

            result.push_back(std::tolower(c));
        }

        return result;
    }

    std::string StringUtils::toKebabCase(const std::string& str)
    {
        // Convert to snake case first
        std::string result = toSnakeCase(str);

        // Replace underscores with hyphens
        std::replace(result.begin(), result.end(), '_', '-');

        return result;
    }

    // -------------------------------------------------------------------------
    // Splitting and Joining Functions
    // -------------------------------------------------------------------------

    std::vector<std::string> StringUtils::split(const std::string& str, const std::string& delimiter)
    {
        if (str.empty())
        {
            return {};
        }

        if (delimiter.empty())
        {
            return {str};
        }

        std::vector<std::string> tokens;
        size_t start = 0;
        size_t end = str.find(delimiter);

        while (end != std::string::npos)
        {
            tokens.push_back(str.substr(start, end - start));
            start = end + delimiter.length();
            end = str.find(delimiter, start);
        }

        tokens.push_back(str.substr(start));
        return tokens;
    }

    std::vector<std::string> StringUtils::splitByWhitespace(const std::string& str)
    {
        if (str.empty())
        {
            return {};
        }

        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;

        while (ss >> token)
        {
            tokens.push_back(token);
        }

        return tokens;
    }

    std::string StringUtils::join(const std::vector<std::string>& strings, const std::string& delimiter)
    {
        if (strings.empty())
        {
            return "";
        }

        std::ostringstream result;

        for (size_t i = 0; i < strings.size() - 1; ++i)
        {
            result << strings[i] << delimiter;
        }

        result << strings.back();
        return result.str();
    }

    // -------------------------------------------------------------------------
    // Find and Replace Functions
    // -------------------------------------------------------------------------

    std::string StringUtils::replace(const std::string& str, const std::string& from, const std::string& to)
    {
        if (str.empty() || from.empty())
        {
            return str;
        }

        std::string result = str;
        size_t pos = result.find(from);

        if (pos != std::string::npos)
        {
            result.replace(pos, from.length(), to);
        }

        return result;
    }

    std::string StringUtils::replaceAll(const std::string& str, const std::string& from, const std::string& to)
    {
        if (str.empty() || from.empty())
        {
            return str;
        }

        std::string result = str;
        size_t pos = 0;

        while ((pos = result.find(from, pos)) != std::string::npos)
        {
            result.replace(pos, from.length(), to);
            pos += to.length(); // Skip the replacement to avoid infinite loop if 'to' contains 'from'
        }

        return result;
    }

    // -------------------------------------------------------------------------
    // String Checking Functions
    // -------------------------------------------------------------------------

    bool StringUtils::startsWith(const std::string& str, const std::string& prefix)
    {
        if (str.length() < prefix.length())
        {
            return false;
        }

        return str.compare(0, prefix.length(), prefix) == 0;
    }

    bool StringUtils::endsWith(const std::string& str, const std::string& suffix)
    {
        if (str.length() < suffix.length())
        {
            return false;
        }

        return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    }

    bool StringUtils::contains(const std::string& str, const std::string& substring)
    {
        return str.find(substring) != std::string::npos;
    }

    bool StringUtils::isAlpha(const std::string& str)
    {
        if (str.empty())
        {
            return false;
        }

        return std::all_of(str.begin(), str.end(), [](unsigned char c) {
            return std::isalpha(c);
                           });
    }

    bool StringUtils::isNumeric(const std::string& str)
    {
        if (str.empty())
        {
            return false;
        }

        return std::all_of(str.begin(), str.end(), [](unsigned char c) {
            return std::isdigit(c);
                           });
    }

    bool StringUtils::isAlphaNumeric(const std::string& str)
    {
        if (str.empty())
        {
            return false;
        }

        return std::all_of(str.begin(), str.end(), [](unsigned char c) {
            return std::isalnum(c);
                           });
    }

    // -------------------------------------------------------------------------
    // Padding Functions
    // -------------------------------------------------------------------------

    std::string StringUtils::padLeft(const std::string& str, size_t totalWidth, char padChar)
    {
        if (str.length() >= totalWidth)
        {
            return str;
        }

        return std::string(totalWidth - str.length(), padChar) + str;
    }

    std::string StringUtils::padRight(const std::string& str, size_t totalWidth, char padChar)
    {
        if (str.length() >= totalWidth)
        {
            return str;
        }

        return str + std::string(totalWidth - str.length(), padChar);
    }

    std::string StringUtils::center(const std::string& str, size_t totalWidth, char padChar)
    {
        if (str.length() >= totalWidth)
        {
            return str;
        }

        size_t padSize = totalWidth - str.length();
        size_t padLeft = padSize / 2;
        size_t padRight = padSize - padLeft;

        return std::string(padLeft, padChar) + str + std::string(padRight, padChar);
    }

    // -------------------------------------------------------------------------
    // Parsing Functions
    // -------------------------------------------------------------------------

    int StringUtils::parseInt(const std::string& str, int defaultValue)
    {
        if (str.empty())
        {
            return defaultValue;
        }

        try
        {
            return std::stoi(str);
        }
        catch (const std::exception&)
        {
            return defaultValue;
        }
    }

    float StringUtils::parseFloat(const std::string& str, float defaultValue)
    {
        if (str.empty())
        {
            return defaultValue;
        }

        try
        {
            return std::stof(str);
        }
        catch (const std::exception&)
        {
            return defaultValue;
        }
    }

    bool StringUtils::parseBool(const std::string& str, bool defaultValue)
    {
        if (str.empty())
        {
            return defaultValue;
        }

        std::string lowerStr = toLower(str);

        if (lowerStr == "true" || lowerStr == "yes" || lowerStr == "1" || lowerStr == "on")
        {
            return true;
        }
        else if (lowerStr == "false" || lowerStr == "no" || lowerStr == "0" || lowerStr == "off")
        {
            return false;
        }

        return defaultValue;
    }

    // -------------------------------------------------------------------------
    // Conversion Functions
    // -------------------------------------------------------------------------

    std::string StringUtils::toString(int value)
    {
        return std::to_string(value);
    }

    std::string StringUtils::toString(float value, int precision)
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(precision) << value;
        return ss.str();
    }

    std::string StringUtils::toString(double value, int precision)
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(precision) << value;
        return ss.str();
    }

    std::string StringUtils::toString(bool value)
    {
        return value ? "true" : "false";
    }

    std::string StringUtils::toString(const glm::vec2& value, int precision)
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(precision);
        ss << "(" << value.x << ", " << value.y << ")";
        return ss.str();
    }

    std::string StringUtils::toString(const glm::vec3& value, int precision)
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(precision);
        ss << "(" << value.x << ", " << value.y << ", " << value.z << ")";
        return ss.str();
    }

    std::string StringUtils::toString(const glm::vec4& value, int precision)
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(precision);
        ss << "(" << value.x << ", " << value.y << ", " << value.z << ", " << value.w << ")";
        return ss.str();
    }

    // -------------------------------------------------------------------------
    // Encoding/Decoding Functions
    // -------------------------------------------------------------------------

    std::string StringUtils::urlEncode(const std::string& str)
    {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;

        for (char c : str)
        {
            // Keep alphanumeric and other accepted characters intact
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            {
                escaped << c;
            }
            else if (c == ' ')
            {
                escaped << '+';
            }
            else
            {
                // Any other characters are percent-encoded
                escaped << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
            }
        }

        return escaped.str();
    }

    std::string StringUtils::urlDecode(const std::string& str)
    {
        std::ostringstream decoded;
        for (size_t i = 0; i < str.length(); ++i)
        {
            if (str[i] == '%')
            {
                if (i + 2 < str.length())
                {
                    int value;
                    std::istringstream iss(str.substr(i + 1, 2));
                    iss >> std::hex >> value;
                    decoded << static_cast<char>(value);
                    i += 2;
                }
            }
            else if (str[i] == '+')
            {
                decoded << ' ';
            }
            else
            {
                decoded << str[i];
            }
        }

        return decoded.str();
    }

    static const std::string base64Chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    static inline bool isBase64(unsigned char c)
    {
        return (std::isalnum(c) || (c == '+') || (c == '/'));
    }

    std::string StringUtils::base64Encode(const std::vector<uint8_t>& data)
    {
        std::string encoded;
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];

        for (uint8_t byte : data)
        {
            char_array_3[i++] = byte;
            if (i == 3)
            {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;

                for (i = 0; i < 4; i++)
                    encoded += base64Chars[char_array_4[i]];
                i = 0;
            }
        }

        if (i)
        {
            for (j = i; j < 3; j++)
                char_array_3[j] = '\0';

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (j = 0; j < i + 1; j++)
                encoded += base64Chars[char_array_4[j]];

            while (i++ < 3)
                encoded += '=';
        }

        return encoded;
    }

    std::vector<uint8_t> StringUtils::base64Decode(const std::string& str)
    {
        size_t in_len = str.size();
        int i = 0;
        int j = 0;
        int in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::vector<uint8_t> decoded;

        while (in_len-- && (str[in_] != '=') && isBase64(str[in_]))
        {
            char_array_4[i++] = str[in_]; in_++;
            if (i == 4)
            {
                for (i = 0; i < 4; i++)
                    char_array_4[i] = static_cast<unsigned char>(base64Chars.find(char_array_4[i]));

                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

                for (i = 0; i < 3; i++)
                    decoded.push_back(char_array_3[i]);
                i = 0;
            }
        }

        if (i)
        {
            for (j = i; j < 4; j++)
                char_array_4[j] = 0;

            for (j = 0; j < 4; j++)
                char_array_4[j] = static_cast<unsigned char>(base64Chars.find(char_array_4[j]));

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (j = 0; j < i - 1; j++)
                decoded.push_back(char_array_3[j]);
        }

        return decoded;
    }

    // -------------------------------------------------------------------------
    // Hash Functions
    // -------------------------------------------------------------------------

    size_t StringUtils::hash(const std::string& str)
    {
        return std::hash<std::string>{}(str);
    }

    std::string StringUtils::md5(const std::string& str)
    {
        unsigned char digest[MD5_DIGEST_LENGTH];
        MD5(reinterpret_cast<const unsigned char*>(str.c_str()), str.length(), digest);

        std::ostringstream ss;
        for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
        {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
        }

        return ss.str();
    }

    std::string StringUtils::sha1(const std::string& str)
    {
        unsigned char digest[SHA_DIGEST_LENGTH];
        SHA1(reinterpret_cast<const unsigned char*>(str.c_str()), str.length(), digest);

        std::ostringstream ss;
        for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
        {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
        }

        return ss.str();
    }

    // -------------------------------------------------------------------------
    // Path Utilities
    // -------------------------------------------------------------------------

    std::string StringUtils::getPathExtension(const std::string& path)
    {
        std::filesystem::path fsPath(path);
        std::string extension = fsPath.extension().string();

        // Remove the leading dot from the extension
        if (!extension.empty() && extension[0] == '.')
        {
            extension.erase(0, 1);
        }

        return extension;
    }

    std::string StringUtils::getFileName(const std::string& path)
    {
        std::filesystem::path fsPath(path);
        return fsPath.filename().string();
    }

    std::string StringUtils::getFileNameWithoutExtension(const std::string& path)
    {
        std::filesystem::path fsPath(path);
        return fsPath.stem().string();
    }

    std::string StringUtils::getDirectoryPath(const std::string& path)
    {
        std::filesystem::path fsPath(path);
        return fsPath.parent_path().string();
    }

    std::string StringUtils::normalizePath(const std::string& path)
    {
        std::filesystem::path fsPath(path);
        fsPath = fsPath.lexically_normal();
        return fsPath.string();
    }

    std::string StringUtils::combinePaths(const std::string& path1, const std::string& path2)
    {
        std::filesystem::path fsPath1(path1);
        std::filesystem::path fsPath2(path2);
        std::filesystem::path combined = fsPath1 / fsPath2;
        return combined.string();
    }

} // namespace PixelCraft::Utility