// -------------------------------------------------------------------------
// Logger.cpp
// -------------------------------------------------------------------------
#include "Core/Logger.h"

#include <iostream>
#include <ctime>
#include <thread>
#include <algorithm>

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
    FileLogChannel::FileLogChannel(const std::string& filename)
    {
        m_fileStream.open(filename, std::ios::out | std::ios::app);
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

    // Logger implementation
    Logger::Logger()
        : m_logLevel(LogLevel::Info)
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
            warn("Logger already initialized");
            return true;
        }

        // Add default console channel
        auto consoleChannel = std::make_shared<ConsoleLogChannel>();
        m_channels.push_back(consoleChannel);

        m_initialized = true;
        info("Logger initialized");
        return true;
    }

    void Logger::shutdown()
    {
        std::lock_guard<std::mutex> lock(m_logMutex);

        if (!m_initialized)
        {
            return;
        }

        info("Logger shutting down");

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
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
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