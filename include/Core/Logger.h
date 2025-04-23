// -------------------------------------------------------------------------
// Logger.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace PixelCraft::Core
{

    /**
     * @brief Log severity levels
     */
    enum class LogLevel
    {
        Debug,   // Detailed information for debugging
        Info,    // General information about system operation
        Warning, // Potential issues that aren't critical errors
        Error,   // Errors that affect functionality but don't crash
        Fatal,   // Critical errors that prevent operation
    };

    /**
     * @brief Abstract base class for log output channels
     */
    class LogChannel
    {
    public:
        virtual ~LogChannel() = default;

        /**
         * @brief Write a log message to the channel
         * @param level The severity level of the message
         * @param message The formatted message to write
         */
        virtual void write(LogLevel level, const std::string& message) = 0;

        /**
         * @brief Flush any buffered messages
         */
        virtual void flush() = 0;
    };

    /**
     * Channel for logging to console output
     */
    class ConsoleLogChannel : public LogChannel
    {
    public:
        ConsoleLogChannel();
        ~ConsoleLogChannel() override;

        void write(LogLevel level, const std::string& message) override;
        void flush() override;
    };

    /**
     * @brief Channel for logging to a file
     */
    class FileLogChannel : public LogChannel
    {
    public:
        /**
         * @brief Constructor
         * @param filename Path to the log file
         */
        FileLogChannel(const std::string& filename);
        ~FileLogChannel() override;

        void write(LogLevel level, const std::string& message) override;
        void flush() override;

    private:
        std::ofstream m_fileStream;
    };

    /**
     * @brief Thread-safe logging system with multiple output channels
     */
    class Logger
    {
    public:
        /**
         * @brief Get the singleton logger instance
         * @return Reference to the logger instance
         */
        static Logger& getInstance();

        /**
         * @brief Initialize the logging system
         * @return True if initialization succeeded
         */
        bool initialize();

        /**
         * @brief Shut down the logging system and flush all channels
         */
        void shutdown();

        /**
         * @brief Set the minimum severity level for logging
         * @param level The minimum level to log
         */
        void setLogLevel(LogLevel level);

        /**
         * @brief Get the current minimum severity level
         * @return The current log level
         */
        LogLevel getLogLevel() const;

        /**
         * @brief Add an output channel
         * @param channel Shared pointer to a log channel
         */
        void addChannel(std::shared_ptr<LogChannel> channel);

        /**
         * @brief Remove an output channel
         * @param channel Shared pointer to the channel to remove
         */
        void removeChannel(std::shared_ptr<LogChannel> channel);

        /**
         * @brief Set a category for subsequent log messages
         * @param category The category name
         */
        void setCategory(const std::string& category);

        /**
         * @brief Clear the current category
         */
        void clearCategory();

        /**
         * @brief Log a message with specific severity
         * @param level The severity level
         * @param message The message to log
         */
        void log(LogLevel level, const std::string& message);

        /**
         * @brief Log a debug message
         * @param message The message to log
         */
        void debug(const std::string& message);

        /**
         * @brief Log an info message
         * @param message The message to log
         */
        void info(const std::string& message);

        /**
         * @brief Log a warning message
         * @param message The message to log
         */
        void warn(const std::string& message);

        /**
         * @brief Log an error message
         * @param message The message to log
         */
        void error(const std::string& message);

        /**
         * @brief Log a fatal error message
         * @param message The message to log
         */
        void fatal(const std::string& message);

    private:
        Logger();
        ~Logger();

        // Prevent copying
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        std::vector<std::shared_ptr<LogChannel>> m_channels;
        LogLevel m_logLevel;
        std::mutex m_logMutex;
        std::string m_category;
        bool m_initialized;

        /**
         * @brief Format a log message with timestamp and context
         * @param level The severity level
         * @param message The raw message
         * @return Formatted message ready for output
         */
        std::string formatMessage(LogLevel level, const std::string& message);

        /**
         * @brief Convert a log level to its string representation
         * @param level The log level
         * @return String representation of the log level
         */
        std::string logLevelToString(LogLevel level);
    };

    // Global convenience functions
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
    void fatal(const std::string& message);

} // namespace PixelCraft::Core