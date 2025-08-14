// -------------------------------------------------------------------------
// ConfigManager.cpp
// -------------------------------------------------------------------------
#include "Core/ConfigManager.h"
#include "Core/Logger.h"
#include "Core/EventSystem.h"
#include "Core/Application.h"

#include <fstream>
#include <filesystem>
#include <algorithm>
#include <sstream>
#include <ctime>
#include <chrono>

// JSON parsing library
#include "nlohmann/json.hpp"
// YAML parsing library
#include "yaml-cpp/yaml.h"

namespace fs = std::filesystem;

// In namespace PixelCraft
namespace PixelCraft::Core
{

    ConfigManager::ConfigManager() : m_fileWatcher(std::make_unique<FileWatcher>()), m_shuttingDown(false)
    {
        // Constructor - nothing to do here
    }

    ConfigManager::~ConfigManager()
    {
        shutdown();
    }

    bool ConfigManager::initialize()
    {
        info("Initializing ConfigManager subsystem");

        // Initialize file watcher
        m_fileWatcher = std::make_unique<FileWatcher>();
        m_shuttingDown = false;

        // Load default engine configuration
        std::string engineConfigPath = "config/engine.json";
        if (fs::exists(engineConfigPath))
        {
            if (!loadConfig(engineConfigPath, "engine"))
            {
                warn("Failed to load engine configuration file: " + engineConfigPath);
            }

            // Automatically watch the engine config
            watch(engineConfigPath, true);
        }
        else
        {
            warn("Engine configuration file not found: " + engineConfigPath);
        }

        return true;
    }

    void ConfigManager::update(float deltaTime)
    {
        // Early exit if shutting down
        if (m_shuttingDown.load())
        {
            return;
        }

        // Accumulate time since last check
        m_lastCheckTime += deltaTime;

        // Only check for changes at the specified interval
        if (m_lastCheckTime >= m_checkInterval)
        {
            checkWatchedFiles();
            m_lastCheckTime = 0.0f;
        }
    }

    void ConfigManager::render()
    {
        // Nothing to render for the config manager
    }

    void ConfigManager::shutdown()
    {
        // Signal shutdown to prevent new operations
        m_shuttingDown = true;

        info("Shutting down ConfigManager subsystem");

        // Stop watching all files first
        m_watchedFiles.clear();
        m_fileWatcher.reset();

        // Clear all configuration values and callbacks under lock
        std::lock_guard<std::mutex> lock(m_configMutex);
        m_configValues.clear();
        m_configFiles.clear();
        m_fileSections.clear();
        m_changeCallbacks.clear();
    }

    std::vector<std::string> ConfigManager::getDependencies() const
    {
        return {"MemoryManager", "EventSystem"};
    }

    bool ConfigManager::loadConfig(const std::string& filename, const std::string& section)
    {
        if (m_shuttingDown.load())
        {
            return false;
        }

        if (!fs::exists(filename))
        {
            error("Configuration file not found: " + filename);
            return false;
        }

        try
        {
            std::string format = getFileFormat(filename);

            // Track which file this section comes from
            {
                std::lock_guard<std::mutex> lock(m_configMutex);

                if (!section.empty())
                {
                    m_configFiles[section] = filename;
                    m_fileSections[filename].push_back(section);
                }
                else
                {
                    // Use filename as default section
                    std::string defaultSection = fs::path(filename).stem().string();
                    m_configFiles[defaultSection] = filename;
                    m_fileSections[filename].push_back(defaultSection);
                }
            }

            // Parse the config file without holding the lock
            parseConfigFile(filename, section);
            return true;
        }
        catch (const std::exception& e)
        {
            error("Error loading configuration file: " + filename + " - " + e.what());
            return false;
        }
    }

    bool ConfigManager::saveConfig(const std::string& filename, const std::string& section)
    {
        if (m_shuttingDown.load())
        {
            return false;
        }

        try
        {
            std::string format = getFileFormat(filename);

            // Create directory if it doesn't exist
            fs::path filePath = filename;
            if (!fs::exists(filePath.parent_path()))
            {
                fs::create_directories(filePath.parent_path());
            }

            if (format == "json")
            {
                nlohmann::json jsonConfig;

                // Get configuration values for this section
                std::lock_guard<std::mutex> lock(m_configMutex);

                for (const auto& [key, value] : m_configValues)
                {
                    if (section.empty() || key.find(section + ".") == 0)
                    {
                        std::string jsonKey = key;

                        // Remove section prefix if present
                        if (!section.empty() && key.find(section + ".") == 0)
                        {
                            jsonKey = key.substr(section.length() + 1);
                        }

                        // Convert variant to JSON
                        std::visit([&jsonKey, &jsonConfig](const auto& v) {
                            using T = std::decay_t<decltype(v)>;

                            if constexpr (std::is_same_v<T, int> ||
                                          std::is_same_v<T, float> ||
                                          std::is_same_v<T, bool> ||
                                          std::is_same_v<T, std::string>)
                            {
                                jsonConfig[jsonKey] = v;
                            }
                            else if constexpr (std::is_same_v<T, glm::vec2>)
                            {
                                jsonConfig[jsonKey] = {{"x", v.x}, {"y", v.y}, {"type", "vec2"}};
                            }
                            else if constexpr (std::is_same_v<T, glm::vec3>)
                            {
                                jsonConfig[jsonKey] = {{"x", v.x}, {"y", v.y}, {"z", v.z}, {"type", "vec3"}};
                            }
                            else if constexpr (std::is_same_v<T, glm::vec4>)
                            {
                                jsonConfig[jsonKey] = {{"x", v.x}, {"y", v.y}, {"z", v.z}, {"w", v.w}, {"type", "vec4"}};
                            }
                                   }, value);
                    }
                }

                // Write JSON to file
                std::ofstream file(filename);
                if (!file.is_open())
                {
                    error("Failed to open configuration file for writing: " + filename);
                    return false;
                }

                file << jsonConfig.dump(4); // Pretty print with 4-space indent
                file.close();

                info("Configuration saved to: " + filename);
                return true;
            }
            else if (format == "yaml")
            {
                YAML::Node yamlConfig;

                // Get configuration values for this section
                std::lock_guard<std::mutex> lock(m_configMutex);

                for (const auto& [key, value] : m_configValues)
                {
                    if (section.empty() || key.find(section + ".") == 0)
                    {
                        std::string yamlKey = key;

                        // Remove section prefix if present
                        if (!section.empty() && key.find(section + ".") == 0)
                        {
                            yamlKey = key.substr(section.length() + 1);
                        }

                        // Convert variant to YAML
                        std::visit([&yamlKey, &yamlConfig](const auto& v) {
                            using T = std::decay_t<decltype(v)>;

                            if constexpr (std::is_same_v<T, int>)
                            {
                                yamlConfig[yamlKey] = v;
                            }
                            else if constexpr (std::is_same_v<T, float>)
                            {
                                yamlConfig[yamlKey] = v;
                            }
                            else if constexpr (std::is_same_v<T, bool>)
                            {
                                yamlConfig[yamlKey] = v;
                            }
                            else if constexpr (std::is_same_v<T, std::string>)
                            {
                                yamlConfig[yamlKey] = v;
                            }
                            else if constexpr (std::is_same_v<T, glm::vec2>)
                            {
                                YAML::Node vecNode;
                                vecNode["x"] = v.x;
                                vecNode["y"] = v.y;
                                vecNode["type"] = "vec2";
                                yamlConfig[yamlKey] = vecNode;
                            }
                            else if constexpr (std::is_same_v<T, glm::vec3>)
                            {
                                YAML::Node vecNode;
                                vecNode["x"] = v.x;
                                vecNode["y"] = v.y;
                                vecNode["z"] = v.z;
                                vecNode["type"] = "vec3";
                                yamlConfig[yamlKey] = vecNode;
                            }
                            else if constexpr (std::is_same_v<T, glm::vec4>)
                            {
                                YAML::Node vecNode;
                                vecNode["x"] = v.x;
                                vecNode["y"] = v.y;
                                vecNode["z"] = v.z;
                                vecNode["w"] = v.w;
                                vecNode["type"] = "vec4";
                                yamlConfig[yamlKey] = vecNode;
                            }
                                   }, value);
                    }
                }

                // Write YAML to file
                std::ofstream file(filename);
                if (!file.is_open())
                {
                    error("Failed to open configuration file for writing: " + filename);
                    return false;
                }

                file << YAML::Dump(yamlConfig);
                file.close();

                info("Configuration saved to: " + filename);
                return true;
            }
            else
            {
                error("Unsupported configuration file format: " + format);
                return false;
            }
        }
        catch (const std::exception& e)
        {
            error("Error saving configuration file: " + filename + " - " + e.what());
            return false;
        }
    }

    int ConfigManager::getInt(const std::string& key, int defaultValue) const
    {
        return get<int>(key, defaultValue);
    }

    float ConfigManager::getFloat(const std::string& key, float defaultValue) const
    {
        return get<float>(key, defaultValue);
    }

    bool ConfigManager::getBool(const std::string& key, bool defaultValue) const
    {
        return get<bool>(key, defaultValue);
    }

    std::string ConfigManager::getString(const std::string& key, const std::string& defaultValue) const
    {
        return get<std::string>(key, defaultValue);
    }

    glm::vec2 ConfigManager::getVec2(const std::string& key, const glm::vec2& defaultValue) const
    {
        return get<glm::vec2>(key, defaultValue);
    }

    glm::vec3 ConfigManager::getVec3(const std::string& key, const glm::vec3& defaultValue) const
    {
        return get<glm::vec3>(key, defaultValue);
    }

    glm::vec4 ConfigManager::getVec4(const std::string& key, const glm::vec4& defaultValue) const
    {
        return get<glm::vec4>(key, defaultValue);
    }

    bool ConfigManager::hasKey(const std::string& key) const
    {
        if (m_shuttingDown.load())
        {
            return false;
        }

        std::lock_guard<std::mutex> lock(m_configMutex);
        return m_configValues.find(key) != m_configValues.end();
    }

    void ConfigManager::watch(const std::string& filename, bool enable)
    {
        if (m_shuttingDown.load())
        {
            return;
        }

        if (!fs::exists(filename))
        {
            warn("Cannot watch non-existant file: " + filename);
            return;
        }

        if (enable)
        {
            m_watchedFiles.insert(filename);
            if (m_fileWatcher)
            {
                m_fileWatcher->addWatch(filename);
                debug("Started watching configuration file: " + filename);
            }
        }
        else
        {
            m_watchedFiles.erase(filename);
            if (m_fileWatcher)
            {
                m_fileWatcher->removeWatch(filename);
                debug("Stopped watching configuration file: " + filename);
            }
        }
    }

    uint64_t ConfigManager::registerChangeCallback(const ConfigChangeCallback& callback)
    {
        if (!callback || m_shuttingDown.load())
        {
            return 0;
        }

        uint64_t id = m_nextCallbackId++;
        std::lock_guard<std::mutex> lock(m_configMutex);
        m_changeCallbacks[id] = callback;
        return id;
    }

    void ConfigManager::unregisterChangeCallback(uint64_t callbackId)
    {
        if (callbackId == 0)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(m_configMutex);
        m_changeCallbacks.erase(callbackId);
    }

    void ConfigManager::checkWatchedFiles()
    {
        if (m_shuttingDown.load() || !m_fileWatcher)
        {
            return;
        }

        auto changedFiles = m_fileWatcher->checkChanges();
        for (const auto& filename : changedFiles)
        {
            // Check again in case we're shutting down
            if (m_shuttingDown.load())
            {
                break;
            }

            debug("Detected change in configuration file: " + filename);

            // Get all sections associated with this file
            std::vector<std::string> sections;
            {
                std::lock_guard<std::mutex> lock(m_configMutex);
                auto it = m_fileSections.find(filename);
                if (it != m_fileSections.end())
                {
                    sections = it->second;
                }
            }

            // Reload all sections from this file
            for (const auto& section : sections)
            {
                if (m_shuttingDown.load())
                {
                    break;
                }

                parseConfigFile(filename, section);

                // Notify about changes to all configs in this section
                std::lock_guard<std::mutex> lock(m_configMutex);
                for (const auto& [key, _] : m_configValues)
                {
                    if (key.find(section + ".") == 0)
                    {
                        // Check once more before notifying
                        if (!m_shuttingDown.load())
                        {
                            notifyConfigChange(key, section);
                        }
                    }
                }
            }
        }
    }

    void ConfigManager::parseConfigFile(const std::string& filename, const std::string& section)
    {
        if (m_shuttingDown.load())
        {
            return;
        }

        std::string format = getFileFormat(filename);
        std::string effectiveSection = section.empty() ? fs::path(filename).stem().string() : section;
        std::string prefix = effectiveSection.empty() ? "" : effectiveSection + ".";

        try
        {
            if (format == "json")
            {
                // Parse JSON file
                std::ifstream file(filename);
                if (!file.is_open())
                {
                    error("Failed to open configuration file: " + filename);
                    return;
                }

                nlohmann::json jsonConfig;
                file >> jsonConfig;
                file.close();

                // Process all key-value pairs
                std::function<void(const nlohmann::json&, const std::string&)> processJson;
                processJson = [this, &prefix, &processJson](const nlohmann::json& j, const std::string& path) {
                    if (m_shuttingDown.load())
                    {
                        return;
                    }

                    if (j.is_object())
                    {
                        for (auto it = j.begin(); it != j.end(); it++)
                        {
                            if (m_shuttingDown.load())
                            {
                                break;
                            }
                            std::string newPath = path.empty() ? it.key() : path + "." + it.key();
                            processJson(it.value(), newPath);
                        }
                    }
                    else
                    {
                        std::lock_guard<std::mutex> lock(m_configMutex);

                        if (j.is_null())
                        {
                            // Skip null values
                        }
                        else if (j.is_number_integer())
                        {
                            m_configValues[prefix + path] = j.get<int>();
                        }
                        else if (j.is_number_float())
                        {
                            m_configValues[prefix + path] = j.get<float>();
                        }
                        else if (j.is_boolean())
                        {
                            m_configValues[prefix + path] = j.get<bool>();
                        }
                        else if (j.is_string())
                        {
                            m_configValues[prefix + path] = j.get<std::string>();
                        }
                        else if (j.is_object() && j.contains("type"))
                        {
                            std::string type = j["type"].get<std::string>();
                            if (type == "vec2" && j.contains("x") && j.contains("y"))
                            {
                                m_configValues[prefix + path] = glm::vec2(
                                    j["x"].get<float>(),
                                    j["y"].get<float>()
                                );
                            }
                            else if (type == "vec3" && j.contains("x") && j.contains("y") && j.contains("z"))
                            {
                                m_configValues[prefix + path] = glm::vec3(
                                    j["x"].get<float>(),
                                    j["y"].get<float>(),
                                    j["z"].get<float>()
                                );
                            }
                            else if (type == "vec4" && j.contains("x") && j.contains("y") && j.contains("z") && j.contains("w"))
                            {
                                m_configValues[prefix + path] = glm::vec4(
                                    j["x"].get<float>(),
                                    j["y"].get<float>(),
                                    j["z"].get<float>(),
                                    j["w"].get<float>()
                                );
                            }
                        }
                    }
                    };

                processJson(jsonConfig, "");
            }
            else if (format == "yaml")
            {
                // Parse YAML file
                YAML::Node yamlConfig = YAML::LoadFile(filename);

                // Process all key-value pairs
                std::function<void(const YAML::Node&, const std::string&)> processYaml;
                processYaml = [this, &prefix, &processYaml](const YAML::Node& node, const std::string& path) {
                    if (m_shuttingDown.load())
                    {
                        return;
                    }

                    if (node.IsMap())
                    {
                        for (auto it = node.begin(); it != node.end(); it++)
                        {
                            if (m_shuttingDown.load())
                            {
                                break;
                            }
                            std::string key = it->first.as<std::string>();
                            std::string newPath = path.empty() ? key : path + "." + key;
                            processYaml(it->second, newPath);
                        }
                    }
                    else
                    {
                        std::lock_guard<std::mutex> lock(m_configMutex);

                        if (node.IsNull())
                        {
                            // Skip null values
                        }
                        else if (node.IsScalar())
                        {
                            // Try to determine the type
                            try
                            {
                                // First try as integer
                                m_configValues[prefix + path] = node.as<int>();
                            }
                            catch (const YAML::Exception&)
                            {
                                try
                                {
                                    // Then try as float
                                    m_configValues[prefix + path] = node.as<float>();
                                }
                                catch (const YAML::Exception&)
                                {
                                    try
                                    {
                                        // Then try as boolean
                                        m_configValues[prefix + path] = node.as<bool>();
                                    }
                                    catch (const YAML::Exception&)
                                    {
                                        // Default to string
                                        m_configValues[prefix + path] = node.as<std::string>();
                                    }
                                }
                            }
                        }
                        else if (node.IsMap() && node["type"])
                        {
                            std::string type = node["type"].as<std::string>();
                            if (type == "vec2" && node["x"] && node["y"])
                            {
                                m_configValues[prefix + path] = glm::vec2(
                                    node["x"].as<float>(),
                                    node["y"].as<float>()
                                );
                            }
                            else if (type == "vec3" && node["x"] && node["y"] && node["z"])
                            {
                                m_configValues[prefix + path] = glm::vec3(
                                    node["x"].as<float>(),
                                    node["y"].as<float>(),
                                    node["z"].as<float>()
                                );
                            }
                            else if (type == "vec4" && node["x"] && node["y"] && node["z"] && node["w"])
                            {
                                m_configValues[prefix + path] = glm::vec4(
                                    node["x"].as<float>(),
                                    node["y"].as<float>(),
                                    node["z"].as<float>(),
                                    node["w"].as<float>()
                                );
                            }
                        }
                    }
                    };

                processYaml(yamlConfig, "");
            }
            else
            {
                error("Unsupported configuration file format: " + format);
            }
        }
        catch (const std::exception& e)
        {
            error("Error parsing configuration file: " + filename + " - " + e.what());
        }
    }

    void ConfigManager::notifyConfigChange(const std::string& key, const std::string& section)
    {
        // Don't notify during shutdown
        if (m_shuttingDown.load())
        {
            return;
        }

        // Create event for the change
        ConfigChangeEvent event(key, section);

        // Notify local callbacks
        std::unordered_map<uint64_t, ConfigChangeCallback> callbacks;
        {
            std::lock_guard<std::mutex> lock(m_configMutex);
            callbacks = m_changeCallbacks;
        }

        for (const auto& [id, callback] : callbacks)
        {
            if (m_shuttingDown.load())
            {
                break;
            }
            try
            {
                callback(event);
            }
            catch (const std::exception& e)
            {
                warn("Exception in config change callback: " + std::string(e.what()));
            }
        }

        // Dispatch through the event system if available and not shutting down
        if (!m_shuttingDown.load())
        {
            try
            {
                auto app = Application::getInstance();
                if (app)
                {
                    auto eventSystem = app->getSubsystem<EventSystem>();
                    if (eventSystem)
                    {
                        eventSystem->dispatch(event);
                    }
                }
            }
            catch (const std::exception& e)
            {
                // Silently ignore exceptions during shutdown
                debug("Exception dispatching config event during shutdown: " + std::string(e.what()));
            }
        }
    }

    std::string ConfigManager::getFileFormat(const std::string& filename) const
    {
        fs::path path(filename);
        std::string extension = path.extension().string();

        if (extension == ".json")
        {
            return "json";
        }
        else if (extension == ".yaml" || extension == ".yml")
        {
            return "yaml";
        }
        else
        {
            warn("Unknown configuration file format: " + extension + ", assuming JSON");
            return "json";
        }
    }

    std::variant<int, float, bool, std::string, glm::vec2, glm::vec3, glm::vec4>
        ConfigManager::stringToVariant(const std::string& value, const std::string& type) const
    {
        if (type == "int")
        {
            return std::stoi(value);
        }
        else if (type == "float")
        {
            return std::stof(value);
        }
        else if (type == "bool")
        {
            return (value == "true" || value == "1" || value == "yes");
        }
        else if (type == "vec2")
        {
            std::istringstream iss(value);
            float x, y;
            iss >> x >> y;
            return glm::vec2(x, y);
        }
        else if (type == "vec3")
        {
            std::istringstream iss(value);
            float x, y, z;
            iss >> x >> y >> z;
            return glm::vec3(x, y, z);
        }
        else if (type == "vec4")
        {
            std::istringstream iss(value);
            float x, y, z, w;
            iss >> x >> y >> z >> w;
            return glm::vec4(x, y, z, w);
        }
        else
        {
            // Default to string
            return value;
        }
    }

    // Template specializations
    template<>
    int ConfigManager::get<int>(const std::string& key, const int& defaultValue) const
    {
        if (m_shuttingDown.load())
        {
            return defaultValue;
        }

        std::lock_guard<std::mutex> lock(m_configMutex);

        auto it = m_configValues.find(key);
        if (it == m_configValues.end())
        {
            return defaultValue;
        }

        try
        {
            return std::get<int>(it->second);
        }
        catch (const std::bad_variant_access&)
        {
            try
            {
                // Try converting from float
                return static_cast<int>(std::get<float>(it->second));
            }
            catch (const std::bad_variant_access&)
            {
                return defaultValue;
            }
        }
    }

    template<>
    float ConfigManager::get<float>(const std::string& key, const float& defaultValue) const
    {
        if (m_shuttingDown.load())
        {
            return defaultValue;
        }

        std::lock_guard<std::mutex> lock(m_configMutex);

        auto it = m_configValues.find(key);
        if (it == m_configValues.end())
        {
            return defaultValue;
        }

        try
        {
            return std::get<float>(it->second);
        }
        catch (const std::bad_variant_access&)
        {
            try
            {
                // Try converting from int
                return static_cast<float>(std::get<int>(it->second));
            }
            catch (const std::bad_variant_access&)
            {
                return defaultValue;
            }
        }
    }

    template<>
    bool ConfigManager::get<bool>(const std::string& key, const bool& defaultValue) const
    {
        if (m_shuttingDown.load())
        {
            return defaultValue;
        }

        std::lock_guard<std::mutex> lock(m_configMutex);

        auto it = m_configValues.find(key);
        if (it == m_configValues.end())
        {
            return defaultValue;
        }

        try
        {
            return std::get<bool>(it->second);
        }
        catch (const std::bad_variant_access&)
        {
            return defaultValue;
        }
    }

    template<>
    std::string ConfigManager::get<std::string>(const std::string& key, const std::string & defaultValue) const
    {
        if (m_shuttingDown.load())
        {
            return defaultValue;
        }

        std::lock_guard<std::mutex> lock(m_configMutex);

        auto it = m_configValues.find(key);
        if (it == m_configValues.end())
        {
            return defaultValue;
        }

        try
        {
            return std::get<std::string>(it->second);
        }
        catch (const std::bad_variant_access&)
        {
            return defaultValue;
        }
    }

    template<>
    glm::vec4 ConfigManager::get<glm::vec4>(const std::string& key, const glm::vec4& defaultValue) const
    {
        std::lock_guard<std::mutex> lock(m_configMutex);

        auto it = m_configValues.find(key);
        if (it == m_configValues.end())
        {
            return defaultValue;
        }

        try
        {
            return std::get<glm::vec4>(it->second);
        }
        catch (const std::bad_variant_access&)
        {
            return defaultValue;
        }
    }

    template<>
    glm::vec2 ConfigManager::get<glm::vec2>(const std::string& key, const glm::vec2& defaultValue) const
    {
        if (m_shuttingDown.load())
        {
            return defaultValue;
        }

        std::lock_guard<std::mutex> lock(m_configMutex);

        auto it = m_configValues.find(key);
        if (it == m_configValues.end())
        {
            return defaultValue;
        }

        try
        {
            return std::get<glm::vec2>(it->second);
        }
        catch (const std::bad_variant_access&)
        {
            return defaultValue;
        }
    }

    template<>
    glm::vec3 ConfigManager::get<glm::vec3>(const std::string& key, const glm::vec3& defaultValue) const
    {
        if (m_shuttingDown.load())
        {
            return defaultValue;
        }

        std::lock_guard<std::mutex> lock(m_configMutex);

        auto it = m_configValues.find(key);
        if (it == m_configValues.end())
        {
            return defaultValue;
        }

        try
        {
            return std::get<glm::vec3>(it->second);
        }
        catch (const std::bad_variant_access&)
        {
            return defaultValue;
        }
    }

    // -------------------------------------------------------------------------
    // FileWatcher Implementation
    // -------------------------------------------------------------------------

    FileWatcher::FileWatcher()
    {
        // Nothing to initialize
    }

    FileWatcher::~FileWatcher()
    {
        // Nothing to clean up
    }

    void FileWatcher::addWatch(const std::string& filename)
    {
        if (!fs::exists(filename))
        {
            warn("Cannot watch non-existant file: " + filename);
            return;
        }

        // Store initial modification time
        std::time_t modTime = fs::last_write_time(filename).time_since_epoch().count();
        m_fileTimestamps[filename] = modTime;
    }

    void FileWatcher::removeWatch(const std::string& filename)
    {
        m_fileTimestamps.erase(filename);
    }

    std::vector<std::string> FileWatcher::checkChanges()
    {
        std::vector<std::string> changedFiles;

        for (auto it = m_fileTimestamps.begin(); it != m_fileTimestamps.end(); it++)
        {
            const std::string& filename = it->first;

            if (!fs::exists(filename))
            {
                warn("Watched file no longer exists: " + filename);
                continue;
            }

            std::time_t currentModTime = fs::last_write_time(filename).time_since_epoch().count();
            std::time_t storedModTime = it->second;

            if (currentModTime > storedModTime)
            {
                changedFiles.push_back(filename);
                it->second = currentModTime; // Update stored timestamp
            }
        }

        return changedFiles;
    }

} // namespace PixelCraft::Core
