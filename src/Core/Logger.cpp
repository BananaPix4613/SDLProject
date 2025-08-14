// -------------------------------------------------------------------------
// Logger.cpp
// -------------------------------------------------------------------------
#include "Core/Logger.h"

#include <iostream>
#include <ctime>
#include <thread>
#include <algorithm>
#include <filesystem>
#include <vector>

// In namespace PixelCraft
namespace PixelCraft::Core
{
    
    // ConsoleLogChannel implementation
    ConsoleLogChannel::ConsoleLogChannel()
    {
    }

    ConsoleLogChannel::~ConsoleLogChannel()
    {
        flush();
    }

    void ConsoleLogChannel::write(LogLevel level, const std::string& message)
    {
        // Set console color based on log level
        switch (level)
        {
            case LogLevel::Debug:
                std::cout << "\033[37m"; // White
                break;
            case LogLevel::Info:
                std::cout << "\033[32m"; // Green
                break;
            case LogLevel::Warning:
                std::cout << "\033[33m"; // Yellow
                break;
            case LogLevel::Error:
                std::cout << "\033[31m"; // Red
                break;
            case LogLevel::Fatal:
                std::cout << "\033[35m"; // Magenta
                break;
        }

        std::cout << message << "\033[0m" << std::endl;
    }

    void ConsoleLogChannel::flush()
    {
        std::cout.flush();
    }

    // FileLogChannel implementation
    FileLogChannel::FileLogChannel(const std::string& filename, bool append)
    {
        auto openMode = append ? (std::ios::out | std::ios::app) : std::ios::out;
        m_fileStream.open(filename, openMode);
        if (!m_fileStream.is_open())
        {
            std::cerr << "Failed to open log file: " << filename << std::endl;
        }
    }

    FileLogChannel::~FileLogChannel()
    {
        flush();
        if (m_fileStream.is_open())
        {
            m_fileStream.close();
        }
    }

    void FileLogChannel::write(LogLevel level, const std::string& message)
    {
        (void)level; // Cannot be used here
        if (m_fileStream.is_open())
        {
            m_fileStream << message << std::endl;
        }
    }

    void FileLogChannel::flush()
    {
        if (m_fileStream.is_open())
        {
            m_fileStream.flush();
        }
    }

    // RotatingFileLogChannel implementation
    RotatingFileLogChannel::RotatingFileLogChannel(const LogRotationConfig& config)
        : m_config(config)
    {
        initializeLogDirectory();
        createNewLogFile();
        cleanupOldLogFiles();
    }

    RotatingFileLogChannel::~RotatingFileLogChannel()
    {
        flush();
        if (m_fileStream.is_open())
        {
            m_fileStream.close();
        }
        if (m_latestFileStream.is_open())
        {
            m_latestFileStream.close();
        }
    }

    void RotatingFileLogChannel::write(LogLevel level, const std::string& message)
    {
        (void)level; // Cannot be used here
        if (m_fileStream.is_open())
        {
            m_fileStream << message << std::endl;
        }

        if (m_latestFileStream.is_open())
        {
            m_latestFileStream << message << std::endl;
        }
    }

    void RotatingFileLogChannel::flush()
    {
        if (m_fileStream.is_open())
        {
            m_fileStream.flush();
        }

        if (m_latestFileStream.is_open())
        {
            m_latestFileStream.flush();
        }
    }

    std::string RotatingFileLogChannel::getCurrentLogPath() const
    {
        return m_currentLogFile;
    }

    std::string RotatingFileLogChannel::getLatestLogPath() const
    {
        return m_fullLogDirectory + "\\" + m_config.latestLogFilename;
    }

    void RotatingFileLogChannel::initializeLogDirectory()
    {
        try
        {
            // Create logs directory if it doesn't exist
            std::filesystem::path logDir(m_config.logDirectory);
            m_fullLogDirectory = std::filesystem::absolute(logDir).string();

            if (!std::filesystem::exists(logDir))
            {
                std::filesystem::create_directories(logDir);
                std::cout << "Created log directory: " << m_fullLogDirectory << std::endl;
            }
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            std::cerr << "Failed to create log directory: " << e.what() << std::endl;
            // Fall back to current directory
            m_fullLogDirectory = ".";
        }
    }

    void RotatingFileLogChannel::createNewLogFile()
    {
        // Close existing file if open
        if (m_fileStream.is_open())
        {
            m_fileStream.close();
        }
        if (m_latestFileStream.is_open())
        {
            m_latestFileStream.close();
        }

        // Generate timestamped filename
        std::string filename = generateTimestampedFilename();
        m_currentLogFile = m_fullLogDirectory + "\\" + filename;

        // Open new log file
        m_fileStream.open(m_currentLogFile, std::ios::out);
        if (!m_fileStream.is_open())
        {
            std::cerr << "Failed to create new log file: " << m_currentLogFile << std::endl;
        }
        else
        {
            std::cout << "Created new log file: " << m_currentLogFile << std::endl;
        }

        // Open latest log file (overwrite existing)
        if (m_config.enableLatestSymlink)
        {
            std::string latestLogPath = getLatestLogPath();
            m_latestFileStream.open(latestLogPath, std::ios::out);
            if (!m_latestFileStream.is_open())
            {
                std::cerr << "Failed to create latest log file: " << latestLogPath << std::endl;
            }
            else
            {
                std::cout << "Created latest log file: " << latestLogPath << std::endl;
            }
        }
    }

    void RotatingFileLogChannel::cleanupOldLogFiles()
    {
        try
        {
            auto existingLogs = getExistingLogFiles();

            // Sort by modification time (newest first)
            std::sort(existingLogs.begin(), existingLogs.end(),
                [](const std::string& a, const std::string& b)
                {
                    try
                    {
                        auto timeA = std::filesystem::last_write_time(a);
                        auto timeB = std::filesystem::last_write_time(b);
                        return timeA > timeB;
                    }
                    catch (...)
                    {
                        return false;
                    }
                });
            
            // Remove old log files if we exceed the maximum count
            if (static_cast<int>(existingLogs.size()) > m_config.maxLogFiles)
            {
                for (size_t i = m_config.maxLogFiles; i < existingLogs.size(); ++i)
                {
                    try
                    {
                        std::filesystem::remove(existingLogs[i]);
                        std::cout << "Removed old log file: " << existingLogs[i] << std::endl;
                    }
                    catch (const std::filesystem::filesystem_error& e)
                    {
                        std::cerr << "Failed to remove old log file " << existingLogs[i] << ": " << e.what() << std::endl;
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error during log cleanup: " << e.what() << std::endl;
        }
    }

    std::string RotatingFileLogChannel::generateTimestampedFilename() const
    {
        if (!m_config.enableTimestamps)
        {
            return m_config.logPrefix + ".txt";
        }

        // Get current time
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        // Format: log_YYYY-MM-DD_HH-MM-SS.txt
        std::stringstream ss;
        std::tm timeInfo;
        localtime_s(&timeInfo, &time);

        ss << m_config.logPrefix << "_";
        ss << std::put_time(&timeInfo, "%Y-%m-%d_%H-%M-%S");
        ss << ".txt";

        return ss.str();
    }

    std::vector<std::string> RotatingFileLogChannel::getExistingLogFiles() const
    {
        std::vector<std::string> logFiles;

        try
        {
            std::filesystem::path logDir(m_fullLogDirectory);

            if (!std::filesystem::exists(logDir))
            {
                return logFiles;
            }

            for (const auto& entry : std::filesystem::directory_iterator(logDir))
            {
                if (entry.is_regular_file())
                {
                    std::string filename = entry.path().filename().string();

                    // Check if this is a log file (starts with prefix and ends with .txt)
                    if (filename.find(m_config.logPrefix) == 0 &&
                        filename.size() > 4 &&
                        filename.substr(filename.size() - 4) == ".txt" &&
                        filename != m_config.latestLogFilename)
                    {
                        logFiles.push_back(entry.path().string());
                    }
                }
            }
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            std::cerr << "Error scanning log directory: " << e.what() << std::endl;
        }

        return logFiles;
    }

    // Logger implementation
    Logger::Logger()
        : m_logLevel(LogLevel::Debug)
        , m_initialized(false)
    {
    }

    Logger::~Logger()
    {
        if (m_initialized)
        {
            shutdown();
        }
    }

    Logger& Logger::getInstance()
    {
        static Logger instance;
        return instance;
    }

    bool Logger::initialize()
    {
        std::lock_guard<std::mutex> lock(m_logMutex);

        if (m_initialized)
        {
            // Use internal method to avoid deadlock
            logInternal(LogLevel::Warning, "Logger already initialized");
            return true;
        }

        // Add default console channel
        auto consoleChannel = std::make_shared<ConsoleLogChannel>();
        m_channels.push_back(consoleChannel);

        m_initialized = true;
        // Use internal method to avoid deadlock
        logInternal(LogLevel::Info, "Logger initialized");
        return true;
    }

    bool Logger::initializeWithRotation(const LogRotationConfig& rotationConfig)
    {
        std::lock_guard<std::mutex> lock(m_logMutex);

        if (m_initialized)
        {
            // Use internal method to avoid deadlock
            logInternal(LogLevel::Warning, "Logger already initialized");
            return true;
        }

        // Add default console channel
        auto consoleChannel = std::make_shared<ConsoleLogChannel>();
        m_channels.push_back(consoleChannel);

        // Add rotating file channel
        auto rotatingFileChannel = std::make_shared<RotatingFileLogChannel>(rotationConfig);
        m_channels.push_back(rotatingFileChannel);

        m_initialized = true;
        // use internal method to avoid deadlock
        logInternal(LogLevel::Info, "Logger initialized with log rotation");
        return true;
    }

    void Logger::shutdown()
    {
        std::lock_guard<std::mutex> lock(m_logMutex);

        if (!m_initialized)
        {
            return;
        }

        // Use internal method to avoid deadlock
        logInternal(LogLevel::Info, "Logger shutting down");

        // Flush and clear all channels
        for (auto& channel : m_channels)
        {
            channel->flush();
        }
        m_channels.clear();

        m_initialized = false;
    }

    void Logger::setLogLevel(LogLevel level)
    {
        std::lock_guard<std::mutex> lock(m_logMutex);
        m_logLevel = level;
    }

    LogLevel Logger::getLogLevel() const
    {
        return m_logLevel;
    }

    void Logger::addChannel(std::shared_ptr<LogChannel> channel)
    {
        if (!channel)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(m_logMutex);
        m_channels.push_back(channel);
    }

    void Logger::removeChannel(std::shared_ptr<LogChannel> channel)
    {
        if (!channel)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(m_logMutex);
        m_channels.erase(
            std::remove(m_channels.begin(), m_channels.end(), channel),
            m_channels.end()
        );
    }

    std::shared_ptr<RotatingFileLogChannel> Logger::addRotatingFileChannel(const LogRotationConfig& config)
    {
        auto rotatingChannel = std::make_shared<RotatingFileLogChannel>(config);
        addChannel(rotatingChannel);
        return rotatingChannel;
    }

    void Logger::setCategory(const std::string& category)
    {
        std::lock_guard<std::mutex> lock(m_logMutex);
        m_category = category;
    }

    void Logger::clearCategory()
    {
        std::lock_guard<std::mutex> lock(m_logMutex);
        m_category.clear();
    }

    void Logger::log(LogLevel level, const std::string& message)
    {
        // Early return if message level is below current log level
        if (level < m_logLevel)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(m_logMutex);
        logInternal(level, message);
    }

    void Logger::logInternal(LogLevel level, const std::string& message)
    {
        // This method assumes the mutex is already locked by the caller
        if (!m_initialized)
        {
            // Fall back to std::cerr if logger is not initialized
            std::cerr << formatMessage(level, message) << std::endl;
            return;
        }

        std::string formattedMessage = formatMessage(level, message);

        // Write to all channels
        for (auto& channel : m_channels)
        {
            channel->write(level, formattedMessage);
        }
    }

    void Logger::debug(const std::string& message)
    {
        log(LogLevel::Debug, message);
    }

    void Logger::info(const std::string& message)
    {
        log(LogLevel::Info, message);
    }

    void Logger::warn(const std::string& message)
    {
        log(LogLevel::Warning, message);
    }

    void Logger::error(const std::string& message)
    {
        log(LogLevel::Error, message);
    }

    void Logger::fatal(const std::string& message)
    {
        log(LogLevel::Fatal, message);
    }

    std::string Logger::formatMessage(LogLevel level, const std::string& message)
    {
        // Get current time
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        // Format timestamp
        std::stringstream ss;
        std::tm timeInfo;
        localtime_s(&timeInfo, &time);
        ss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

        // Get thread ID
        std::stringstream threadId;
        threadId << std::this_thread::get_id();

        // Build the formatted message
        std::stringstream result;
        result << "[" << ss.str() << "] ";
        result << "[" << logLevelToString(level) << "] ";
        result << "[Thread:" << threadId.str() << "] ";

        if (!m_category.empty())
        {
            result << "[" << m_category << "] ";
        }

        result << message;

        return result.str();
    }

    std::string Logger::logLevelToString(LogLevel level)
    {
        switch (level)
        {
            case LogLevel::Debug:   return "DEBUG";
            case LogLevel::Info:    return "INFO";
            case LogLevel::Warning: return "WARNING";
            case LogLevel::Error:   return "ERROR";
            case LogLevel::Fatal:   return "FATAL";
            default:                return "UNKNOWN";
        }
    }

    // Global convenience functions
    void debug(const std::string& message)
    {
        Logger::getInstance().debug(message);
    }

    void info(const std::string& message)
    {
        Logger::getInstance().info(message);
    }

    void warn(const std::string& message)
    {
        Logger::getInstance().warn(message);
    }

    void error(const std::string& message)
    {
        Logger::getInstance().error(message);
    }

    void fatal(const std::string& message)
    {
        Logger::getInstance().fatal(message);
    }

} // namespace PixelCraft::Core