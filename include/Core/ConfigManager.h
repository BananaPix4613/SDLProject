// -------------------------------------------------------------------------
// ConfigManager.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <variant>
#include <vector>
#include <mutex>
#include <atomic>

#include "Core/Subsystem.h"
//#include "Events/Event.h"
#include "glm/glm.hpp"

namespace PixelCraft::Core
{

    // Forward delcarations
    class ConfigChangeEvent;
    class FileWatcher;

    /**
     * @brief Engine and game configuration with hot-reloading support
     */
    class ConfigManager : public Subsystem
    {
    public:
        /**
         * @brief Get the singleton instance
         * @return Reference to the ConfigManager instance
         */
        static ConfigManager& getInstance();

        /**
         * @brief Initialize the configuration system
         * @return True if initialization was successful
         */
        bool initialize() override;

        /**
         * @brief Update the configuration system to check for file system
         * @param deltaTime Time since last update
         */
        void update(float deltaTime) override;

        /**
         * @brief Shut down the configuration system
         */
        void shutdown() override;

        /**
         * @brief Get the name of the subsystem
         * @return The subsystem name
         */
        std::string getName() const override
        {
            return "ConfigManager";
        }

        /**
         * @brief Get the subsystem dependencies
         * @return Vector of dependency subsystem names
         */
        std::vector<std::string> getDependencies() const override;

        /**
         * @brief Load configuration from file
         * @param filename Path to the configuration file
         * @param section Optional section name for grouping
         * @return True if loading was successful
         */
        bool loadConfig(const std::string& filename, const std::string& section = "");

        /**
         * @brief Save configuration to a file
         * @param filename Path to the configuration file
         * @param section Optional section name for grouping
         * @return True if saving was successful
         */
        bool saveConfig(const std::string& filename, const std::string& section = "");

        /**
         * @brief Get a configuration value with type safety
         * @param key Configuration key
         * @param defaultValue Default value if key doesn't exist
         * @return Configuration value or default if not found
         */
        template<typename T>
        T get(const std::string& key, const T& defaultValue = T()) const;

        /**
         * @brief Get an integer configuration value
         * @param key Configuration key
         * @param defaultValue Default value if key doesn't exist
         * @return Integer value or default if not found
         */
        int getInt(const std::string& key, int defaultValue = 0) const;

        /**
         * @brief Get a float configuration value
         * @param key Configuration key
         * @param defaultValue Default value if key doesn't exist
         * @return Float value or default if not found
         */
        float getFloat(const std::string& key, float defaultValue = 0.0f) const;

        /**
         * @brief Get a boolean configuration value
         * @param key Configuration key
         * @param defaultValue Default value if key doesn't exist
         * @return Boolean value or default if not found
         */
        bool getBool(const std::string& key, bool defaultValue = false) const;

        /**
         * @brief Get a string configuration value
         * @param key Configuration key
         * @param defaultValue Default value if key doesn't exist
         * @return String value or default if not found
         */
        std::string getString(const std::string& key, const std::string& defaultValue = "") const;

        /**
         * @brief Get a vec2 configuration value
         * @param key Configuration key
         * @param defaultValue Default value if key doesn't exist
         * @return Vec2 value or default if not found
         */
        glm::vec2 getVec2(const std::string& key, const glm::vec2& defaultValue = glm::vec2(0.0f)) const;

        /**
         * @brief Get a vec3 configuration value
         * @param key Configuration key
         * @param defaultValue Default value if key doesn't exist
         * @return Vec3 value or default if not found
         */
        glm::vec3 getVec3(const std::string& key, const glm::vec3& defaultValue = glm::vec3(0.0f)) const;

        /**
         * @brief Get a vec4 configuration value
         * @param key Configuration key
         * @param defaultValue Default value if key doesn't exist
         * @return Vec4 value or default if not found
         */
        glm::vec4 getVec4(const std::string& key, const glm::vec4& defaultValue = glm::vec4(0.0f)) const;

        /**
         * @brief Set a configuration value
         * @param key Configuration key
         * @param value Value to set
         */
        template<typename T>
        void set(const std::string& key, const T& value);

        /**
         * @brief Check if a configuration key exists
         * @param key Configuration key
         * @return True if the key exists
         */
        bool hasKey(const std::string& key) const;

        /**
         * @brief Watch a configuration file for changes
         * @param filename Path to the configuration file
         * @param enable Whether to enable disable watching
         */
        void watch(const std::string& filename, bool enable = true);

        /**
         * @brief Callback type for configuration changes
         */
        using ConfigChangeCallback = std::function<void(const ConfigChangeEvent&)>;

        /**
         * @brief Register a callback for configuration changes
         * @param callback Function to call when configuration changes
         * @return Callback ID for unregistering
         */
        uint64_t registerChangeCallback(const ConfigChangeCallback& callback);

        /**
         * @brief Unregister a change callback
         * @param callbackId ID from registerChangeCallback
         */
        void unregisterChangeCallback(uint64_t callbackId);

    private:
        /**
         * @brief Private constructor for singleton
         */
        ConfigManager();

        /**
         * @brief Private destructor for singleton
         */
        //~ConfigManager();

        /**
         * @brief Check watched files for changes
         */
        void checkWatchedFiles();

        /**
         * @brief Parse a configuration file
         * @param filename Path to the configuration file
         * @param section Optional section name for grouping
         */
        void parseConfigFile(const std::string& filename, const std::string& section);

        /**
         * @brief Notify callbacks about configuration changes
         * @param key Changed configuration key
         * @param section Section containing the key
         */
        void notifyConfigChange(const std::string& key, const std::string& section);

        /**
         * @brief Determine file format from extension
         * @param filename Path to the configuration file
         * @return String indicating the format ("json", "yaml", etc.)
         */
        std::string getFileFormat(const std::string& filename) const;

        /**
         * @brief Convert string to appropriate variant type
         * @param value String representation of value
         * @param type Type hint string ("int", "float", etc.)
         * @return Variant containing the converted value
         */
        std::variant<int, float, bool, std::string, glm::vec2, glm::vec3, glm::vec4>
            stringToVariant(const std::string& value, const std::string& type) const;

        // Config storage - maps keys to values of various types
        std::unordered_map<std::string, std::variant<int, float, bool, std::string, glm::vec2, glm::vec3, glm::vec4>> m_configValues;

        // Maps sections to filename
        std::unordered_map<std::string, std::string> m_configFiles;

        // Maps filenames to section
        std::unordered_map<std::string, std::vector<std::string>> m_fileSections;

        // Set of files being watched for changes
        std::unordered_set<std::string> m_watchedFiles;

        // File watcher implementation
        std::unique_ptr<FileWatcher> m_fileWatcher;

        // Callback management
        std::unordered_map<uint64_t, ConfigChangeCallback> m_changeCallbacks;
        std::atomic<uint64_t> m_nextCallbackId{1};

        // Threading protection
        mutable std::mutex m_configMutex;

        // Last check time for file changes
        float m_lastCheckTime{0.0f};

        // Check interval for file changes in seconds
        float m_checkInterval{1.0f};

        // Singleton instance
        static std::unique_ptr<ConfigManager> s_instance;
        static std::once_flag s_onceFlag;
    };

    /**
     * @brief Event for configuration changes
     */
    //class ConfigChangeEvent : public Events::Event
    //{
    //public:
    //    /**
    //     * @brief Constructor
    //     * @param key Changed configuration key
    //     * @param section Section containing the key
    //     */
    //    ConfigChangeEvent(const std::string& key, const std::string& section)
    //        : m_key(key), m_section(section)
    //    {
    //    }

    //    /**
    //     * @brief Get the changed key
    //     * @return Configuration key
    //     */
    //    const std::string& getKey() const
    //    {
    //        return m_key;
    //    }

    //    /**
    //     * @brief Get the section
    //     * @return Section name
    //     */
    //    const std::string& getSection() const
    //    {
    //        return m_section;
    //    }

    //    /**
    //     * @brief Get the event type name
    //     * @return Type name string
    //     */
    //    std::string getTypeName() const override
    //    {
    //        return "ConfigChangeEvent";
    //    }

    //    /**
    //     * @brief Get the event type index
    //     * @return Type index for event system
    //     */
    //    static std::type_index getTypeIndex()
    //    {
    //        return std::type_index(typeid(ConfigChangeEvent));
    //    }

    //private:
    //    std::string m_key;
    //    std::string m_section;
    //};

    /**
     * @brief Helper for file change monitoring
     */
    class FileWatcher
    {
    public:
        /**
         * @brief Constructor
         */
        FileWatcher();

        /**
         * @brief Destructor
         */
        ~FileWatcher();

        /**
         * @brief Add a file to watch
         * @param filename Path to the file
         */
        void addWatch(const std::string& filename);

        /**
         * @brief Remove a file from watching
         * @param filename Path to the file
         */
        void removeWatch(const std::string& filename);

        /**
         * @brief Check if any watched files have changed
         * @return Vector of changed filenames
         */
        std::vector<std::string> checkChanges();

    private:
        // Maps filenames to last modification times
        std::unordered_map<std::string, std::time_t> m_fileTimestamps;
    };

    // Template implementation for get
    template<typename T>
    T ConfigManager::get(const std::string& key, const T& defaultValue) const
    {
        std::lock_guard<std::mutex> lock(m_configMutex);

        auto it = m_configValues.find(key);
        if (it == m_configValues.end())
        {
            return defaultValue;
        }

        try
        {
            return std::get<T>(it->second);
        }
        catch (const std::bad_variant_access&)
        {
            return defaultValue;
        }
    }

    // Template implementation for set
    template<typename T>
    void ConfigManager::set(const std::string& key, const T& value)
    {
        std::string section;
        {
            std::lock_guard<std::mutex> lock(m_configMutex);
            m_configValues[key] = value;

            // Find which section this key belongs to
            for (const auto& [sec, file] : m_configFiles)
            {
                if (!sec.empty())
                {
                    // If the key starts with the section name and a dot
                    if (key.find(sec + ".") == 0)
                    {
                        section = sec;
                        break;
                    }
                }
            }
        }

        // Notify about the change
        notifyConfigChange(key, section);
    }

    // Template specializations for common types
    template<>
    int ConfigManager::get<int>(const std::string& key, const int& defaultValue) const;

    template<>
    float ConfigManager::get<float>(const std::string& key, const float& defaultValue) const;

    template<>
    bool ConfigManager::get<bool>(const std::string& key, const bool& defaultValue) const;

    template<>
    std::string ConfigManager::get<std::string>(const std::string& key, const std::string& defaultValue) const;

    template<>
    glm::vec2 ConfigManager::get<glm::vec2>(const std::string& key, const glm::vec2& defaultValue) const;

    template<>
    glm::vec3 ConfigManager::get<glm::vec3>(const std::string& key, const glm::vec3& defaultValue) const;

    template<>
    glm::vec4 ConfigManager::get<glm::vec4>(const std::string& key, const glm::vec4& defaultValue) const;

} // namespace PixelCraft::Core