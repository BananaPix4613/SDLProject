// -------------------------------------------------------------------------
// StringUtils.h
// -------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>

namespace PixelCraft::Utility
{

    /**
     * @brief Provides utility functions for string manipulation.
     *
     * StringUtils contains static methods for common string operations such as
     * trimming, case conversion, splitting, formatting, and path manipulation.
     * All methods are designed to be efficient and consistent with modern C++ practices.
     */
    class StringUtils
    {
    public:
        /**
         * @brief Trims whitespace from both ends of a string
         * @param str The string to trim
         * @return The trimmed string
         */
        static std::string trim(const std::string& str);

        /**
         * @brief Trims whitespace from the beginning of a string
         * @param str The string to trim
         * @return The trimmed string
         */
        static std::string trimLeft(const std::string& str);

        /**
         * @brief Trims whitespace from the end of a string
         * @param str The string to trim
         * @return The trimmed string
         */
        static std::string trimRight(const std::string& str);

        /**
         * @brief Converts a string to uppercase
         * @param str The string to convert
         * @return The uppercase version of the string
         */
        static std::string toUpper(const std::string& str);

        /**
         * @brief Converts a string to lowercase
         * @param str The string to convert
         * @return The lowercase version of the string
         */
        static std::string toLower(const std::string& str);

        /**
         * @brief Converts a string to title case (first letter of each word capitalized)
         * @param str The string to convert
         * @return The title case version of the string
         */
        static std::string toTitleCase(const std::string& str);

        /**
         * @brief Converts a string to camelCase (first word lowercase, subsequent words capitalized)
         * @param str The string to convert
         * @return The camelCase version of the string
         */
        static std::string toCamelCase(const std::string& str);

        /**
         * @brief Converts a string to PascalCase (all words capitalized)
         * @param str The string to convert
         * @return The PascalCase version of the string
         */
        static std::string toPascalCase(const std::string& str);

        /**
         * @brief Converts a string to snake_case
         * @param str The string to convert
         * @return The snake_case version of the string
         */
        static std::string toSnakeCase(const std::string& str);

        /**
         * @brief Converts a string to kebab-case
         * @param str The string to convert
         * @return The kebab-case version of the string
         */
        static std::string toKebabCase(const std::string& str);

        /**
         * @brief Splits a string by a delimiter
         * @param str The string to split
         * @param delimiter The delimiter to split by
         * @return A vector of substrings
         */
        static std::vector<std::string> split(const std::string& str, const std::string& delimiter);

        /**
         * @brief Splits a string by whitespace
         * @param str The string to split
         * @return A vector of substrings
         */
        static std::vector<std::string> splitByWhitespace(const std::string& str);

        /**
         * @brief Joins a vector of strings with a delimiter
         * @param strings The strings to join
         * @param delimiter The delimiter to join with
         * @return The joined string
         */
        static std::string join(const std::vector<std::string>& strings, const std::string& delimiter);

        /**
         * @brief Replaces the first occurrence of a substring
         * @param str The string to perform replacement on
         * @param from The substring to replace
         * @param to The replacement substring
         * @return The string with replacement
         */
        static std::string replace(const std::string& str, const std::string& from, const std::string& to);

        /**
         * @brief Replaces all occurrences of a substring
         * @param str The string to perform replacement on
         * @param from The substring to replace
         * @param to The replacement substring
         * @return The string with all replacements
         */
        static std::string replaceAll(const std::string& str, const std::string& from, const std::string& to);

        /**
         * @brief Checks if a string starts with a prefix
         * @param str The string to check
         * @param prefix The prefix to check for
         * @return True if the string starts with the prefix, false otherwise
         */
        static bool startsWith(const std::string& str, const std::string& prefix);

        /**
         * @brief Checks if a string ends with a suffix
         * @param str The string to check
         * @param suffix The suffix to check for
         * @return True if the string ends with the suffix, false otherwise
         */
        static bool endsWith(const std::string& str, const std::string& suffix);

        /**
         * @brief Checks if a string contains a substring
         * @param str The string to check
         * @param substring The substring to check for
         * @return True if the string contains the substring, false otherwise
         */
        static bool contains(const std::string& str, const std::string& substring);

        /**
         * @brief Checks if a string contains only alphabetic characters
         * @param str The string to check
         * @return True if the string is alphabetic, false otherwise
         */
        static bool isAlpha(const std::string& str);

        /**
         * @brief Checks if a string contains only numeric characters
         * @param str The string to check
         * @return True if the string is numeric, false otherwise
         */
        static bool isNumeric(const std::string& str);

        /**
         * @brief Checks if a string contains only alphanumeric characters
         * @param str The string to check
         * @return True if the string is alphanumeric, false otherwise
         */
        static bool isAlphaNumeric(const std::string& str);

        /**
         * @brief Formats a string like sprintf but type-safe
         * @param format The format string
         * @param args The arguments to format
         * @return The formatted string
         */
        template<typename... Args>
        static std::string format(const std::string& format, Args&&... args);

        /**
         * @brief Pads a string on the left
         * @param str The string to pad
         * @param totalWidth The desired width of the resulting string
         * @param padChar The character to pad with
         * @return The padded string
         */
        static std::string padLeft(const std::string& str, size_t totalWidth, char padChar = ' ');

        /**
         * @brief Pads a string on the right
         * @param str The string to pad
         * @param totalWidth The desired width of the resulting string
         * @param padChar The character to pad with
         * @return The padded string
         */
        static std::string padRight(const std::string& str, size_t totalWidth, char padChar = ' ');

        /**
         * @brief Centers a string with padding
         * @param str The string to center
         * @param totalWidth The desired width of the resulting string
         * @param padChar The character to pad with
         * @return The centered string
         */
        static std::string center(const std::string& str, size_t totalWidth, char padChar = ' ');

        /**
         * @brief Parses a string to an integer
         * @param str The string to parse
         * @param defaultValue The default value to return if parsing fails
         * @return The parsed integer or the default value
         */
        static int parseInt(const std::string& str, int defaultValue = 0);

        /**
         * @brief Parses a string to a float
         * @param str The string to parse
         * @param defaultValue The default value to return if parsing fails
         * @return The parsed float or the default value
         */
        static float parseFloat(const std::string& str, float defaultValue = 0.0f);

        /**
         * @brief Parses a string to a boolean
         * @param str The string to parse
         * @param defaultValue The default value to return if parsing fails
         * @return The parsed boolean or the default value
         */
        static bool parseBool(const std::string& str, bool defaultValue = false);

        /**
         * @brief Converts an integer to a string
         * @param value The integer to convert
         * @return The string representation
         */
        static std::string toString(int value);

        /**
         * @brief Converts a float to a string
         * @param value The float to convert
         * @param precision The number of decimal places
         * @return The string representation
         */
        static std::string toString(float value, int precision = 6);

        /**
         * @brief Converts a double to a string
         * @param value The double to convert
         * @param precision The number of decimal places
         * @return The string representation
         */
        static std::string toString(double value, int precision = 10);

        /**
         * @brief Converts a boolean to a string
         * @param value The boolean to convert
         * @return "true" or "false"
         */
        static std::string toString(bool value);

        /**
         * @brief Converts a vec2 to a string
         * @param value The vec2 to convert
         * @param precision The number of decimal places
         * @return The string representation
         */
        static std::string toString(const glm::vec2& value, int precision = 2);

        /**
         * @brief Converts a vec3 to a string
         * @param value The vec3 to convert
         * @param precision The number of decimal places
         * @return The string representation
         */
        static std::string toString(const glm::vec3& value, int precision = 2);

        /**
         * @brief Converts a vec4 to a string
         * @param value The vec4 to convert
         * @param precision The number of decimal places
         * @return The string representation
         */
        static std::string toString(const glm::vec4& value, int precision = 2);

        /**
         * @brief Encodes a string for URL usage
         * @param str The string to encode
         * @return The URL encoded string
         */
        static std::string urlEncode(const std::string& str);

        /**
         * @brief Decodes a URL encoded string
         * @param str The string to decode
         * @return The decoded string
         */
        static std::string urlDecode(const std::string& str);

        /**
         * @brief Encodes binary data to base64
         * @param data The binary data to encode
         * @return The base64 encoded string
         */
        static std::string base64Encode(const std::vector<uint8_t>& data);

        /**
         * @brief Decodes a base64 string to binary data
         * @param str The base64 string to decode
         * @return The decoded binary data
         */
        static std::vector<uint8_t> base64Decode(const std::string& str);

        /**
         * @brief Computes a hash value for a string
         * @param str The string to hash
         * @return The hash value
         */
        static size_t hash(const std::string& str);

        /**
         * @brief Computes the MD5 hash of a string
         * @param str The string to hash
         * @return The MD5 hash as a hexadecimal string
         */
        static std::string md5(const std::string& str);

        /**
         * @brief Computes the SHA1 hash of a string
         * @param str The string to hash
         * @return The SHA1 hash as a hexadecimal string
         */
        static std::string sha1(const std::string& str);

        /**
         * @brief Gets the file extension from a path
         * @param path The file path
         * @return The file extension (without the dot)
         */
        static std::string getPathExtension(const std::string& path);

        /**
         * @brief Gets the file name from a path
         * @param path The file path
         * @return The file name (with extension)
         */
        static std::string getFileName(const std::string& path);

        /**
         * @brief Gets the file name from a path without extension
         * @param path The file path
         * @return The file name without extension
         */
        static std::string getFileNameWithoutExtension(const std::string& path);

        /**
         * @brief Gets the directory path from a file path
         * @param path The file path
         * @return The directory path
         */
        static std::string getDirectoryPath(const std::string& path);

        /**
         * @brief Normalizes a path (removes redundant separators, resolves '..' and '.')
         * @param path The path to normalize
         * @return The normalized path
         */
        static std::string normalizePath(const std::string& path);

        /**
         * @brief Combines two paths
         * @param path1 The first path
         * @param path2 The second path
         * @return The combined path
         */
        static std::string combinePaths(const std::string& path1, const std::string& path2);

    private:
        // Prevent instantiation
        StringUtils() = delete;
        ~StringUtils() = delete;
    };

    // Template implementation for format
    template<typename... Args>
    std::string StringUtils::format(const std::string& format, Args&&... args)
    {
        // Calculate the required buffer size
        int size = std::snprintf(nullptr, 0, format.c_str(), std::forward<Args>(args)...);
        if (size <= 0)
        {
            return "";
        }

        // Allocate buffer of required size (+1 for null terminator)
        std::string buffer(size + 1, '\0');

        // Format the string
        std::snprintf(&buffer[0], size + 1, format.c_str(), std::forward<Args>(args)...);

        // Remove the null terminator from the string
        buffer.resize(size);

        return buffer;
    }

} // namespace PixelCraft::Utility