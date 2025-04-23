// -------------------------------------------------------------------------
// Profiler.h
// -------------------------------------------------------------------------
#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <stack>
#include <mutex>
#include <thread>
#include <chrono>

namespace PixelCraft::Utility
{

    /**
     * @brief Performance profiling data for a single measurement
     */
    struct ProfileSample
    {
        std::string name;              ///< Name of the profile sample
        double startTime;              ///< Start time in milliseconds
        double endTime;                ///< End time in milliseconds
        double duration;               ///< Duration in milliseconds (endTime - startTime)
        double minTime;                ///< Minimum duration recorded for this sample
        double maxTime;                ///< Maximum duration recorded for this sample
        double averageTime;            ///< Average duration for this sample
        uint64_t callCount;            ///< Number of times this sample has been called
        std::vector<ProfileSample> children; ///< Child samples
        ProfileSample* parent;         ///< Parent sample (if any)
        bool isOpen;                   ///< Whether this sample is currently being measured
        std::thread::id threadId;      ///< ID of the thread this sample was recorded on
    };

    /**
     * @brief Performance statistics for a single frame
     */
    struct FrameStats
    {
        double frameTime;              ///< Total frame time in milliseconds
        double cpuTime;                ///< CPU time in milliseconds
        double gpuTime;                ///< GPU time in milliseconds (if available)
        uint64_t frameNumber;          ///< Frame number
        std::unordered_map<std::string, ProfileSample> samples; ///< Samples recorded in this frame
    };

    /**
     * @brief Display modes for the profiler
     */
    enum class ProfilerDisplayMode
    {
        Disabled,                      ///< No display
        Simple,                        ///< Simple flat list of timings
        Detailed,                      ///< Detailed view with statistics
        Hierarchical,                  ///< Tree view showing parent-child relationships
        Graph                          ///< Graph view showing timing history
    };

    /**
     * @brief RAII wrapper for automatic profiling of a scope
     */
    class ScopedProfiler
    {
    public:
        /**
         * @brief Constructor starts timing the named scope
         * @param name Name of the scope to profile
         */
        ScopedProfiler(const std::string& name);

        /**
         * @brief Destructor ends timing the scope
         */
        ~ScopedProfiler();

    private:
        std::string m_name;            ///< Name of the profile sample
        double m_startTime;            ///< Start time of the sample
    };

    /**
     * @brief Performance profiling system
     *
     * The Profiler provides performance measurement and tracking throughout
     * the engine. It supports hierarchical profiling with nested measurements,
     * frame-based and cumulative statistics, and visualization capabilities.
     */
    class Profiler
    {
    public:
        /**
         * @brief Get the singleton instance
         * @return Reference to the global profiler instance
         */
        static Profiler& getInstance();

        /**
         * @brief Initialize the profiler
         * @return True if initialization was successful
         */
        bool initialize();

        /**
         * @brief Shut down the profiler and release resources
         */
        void shutdown();

        /**
         * @brief Enable or disable profiling
         * @param enable True to enable profiling, false to disable
         */
        void enableProfiling(bool enable);

        /**
         * @brief Check if profiling is enabled
         * @return True if profiling is enabled
         */
        bool isProfilingEnabled() const;

        /**
         * @brief Begin a profiling sample
         * @param name Name of the sample
         */
        void beginSample(const std::string& name);

        /**
         * @brief End the current profiling sample
         */
        void endSample();

        /**
         * @brief Begin a new frame
         */
        void beginFrame();

        /**
         * @brief End the current frame
         */
        void endFrame();

        /**
         * @brief Get a specific profile sample
         * @param name Name of the sample
         * @return Pointer to the sample, or nullptr if not found
         */
        const ProfileSample* getSample(const std::string& name) const;

        /**
         * @brief Get names of all samples
         * @return Vector of sample names
         */
        const std::vector<std::string>& getSampleNames() const;

        /**
         * @brief Get frame statistics
         * @return Vector of frame stats
         */
        const std::vector<FrameStats>& getFrameStats() const;

        /**
         * @brief Reset all profiling statistics
         */
        void resetStats();

        /**
         * @brief Reset statistics for a specific sample
         * @param name Name of the sample to reset
         */
        void resetSample(const std::string& name);

        /**
         * @brief Set how frequently profile data is output
         * @param frameInterval Interval in frames (0 to disable)
         */
        void setOutputFrequency(uint32_t frameInterval);

        /**
         * @brief Output profiling data to the log
         */
        void outputToLog();

        /**
         * @brief Output profiling data to a file
         * @param filename Name of the file to output to
         */
        void outputToFile(const std::string& filename);

        /**
         * @brief Output profiling data to the console
         */
        void outputToConsole();

        /**
         * @brief Set the display mode
         * @param mode Display mode to use
         */
        void setDisplayMode(ProfilerDisplayMode mode);

        /**
         * @brief Get the current display mode
         * @return Current display mode
         */
        ProfilerDisplayMode getDisplayMode() const;

        /**
         * @brief Render profiling visualization
         */
        void render();

        /**
         * @brief Set the name of the current thread
         * @param name Thread name
         */
        void setThreadName(const std::string& name);

        /**
         * @brief Get the name of a thread
         * @param threadId Thread ID
         * @return Thread name, or empty string if not found
         */
        std::string getThreadName(std::thread::id threadId) const;

        /**
         * @brief Begin a GPU timing sample
         * @param name Name of the sample
         */
        void beginGPUSample(const std::string& name);

        /**
         * @brief End the current GPU timing sample
         */
        void endGPUSample();

        /**
         * @brief Set a performance marker for external tools
         * @param name Name of the marker
         */
        void setPerformanceMarker(const std::string& name);

        /**
         * @brief Begin a performance marker section for external tools
         * @param name Name of the marker
         */
        void beginPerformanceMarker(const std::string& name);

        /**
         * @brief End the current performance marker section
         */
        void endPerformanceMarker();

    private:
        /**
         * @brief Constructor is private (singleton)
         */
        Profiler();

        /**
         * @brief Destructor is private (singleton)
         */
        ~Profiler();

        // Prevent copying
        Profiler(const Profiler&) = delete;
        Profiler& operator=(const Profiler&) = delete;

        /**
         * @brief Get the current time in milliseconds
         * @return Current time in milliseconds
         */
        double getCurrentTimeMs();

        /**
         * @brief Update statistics for a sample
         * @param sample Sample to update
         * @param duration Duration of the sample
         */
        void updateSampleStatistics(ProfileSample& sample, double duration);

        /**
         * @brief Get or create a sample for the current thread
         * @param name Name of the sample
         * @return Reference to the sample
         */
        ProfileSample& getThreadSample(const std::string& name);

        /**
         * @brief Render the profiler UI
         */
        void renderUI();

        /**
         * @brief Format a time value for display
         * @param timeMs Time in milliseconds
         * @return Formatted time string
         */
        std::string formatTime(double timeMs);

        // Main profiling data
        bool m_enabled;                ///< Whether profiling is enabled
        ProfilerDisplayMode m_displayMode; ///< Current display mode
        std::unordered_map<std::string, ProfileSample> m_samples; ///< All samples
        std::vector<std::string> m_sampleNames; ///< Names of all samples
        std::vector<FrameStats> m_frameStats; ///< Statistics for recent frames
        uint32_t m_maxFrameStats;      ///< Maximum number of frame stats to keep

        // Current frame data
        uint64_t m_frameCount;         ///< Current frame count
        double m_frameStartTime;       ///< Start time of the current frame
        double m_frameTime;            ///< Duration of the current frame
        double m_cpuTime;              ///< CPU time for the current frame
        double m_gpuTime;              ///< GPU time for the current frame

        // Active samples tracking
        std::stack<std::string> m_activeSamples; ///< Stack of active samples
        std::unordered_map<std::thread::id, std::stack<std::string>> m_threadActiveSamples; ///< Active samples per thread
        std::unordered_map<std::thread::id, std::string> m_threadNames; ///< Thread names

        // GPU timing objects
        std::unordered_map<std::string, uint32_t> m_gpuQueries; ///< GPU timing queries
        std::stack<std::string> m_activeGPUQueries; ///< Stack of active GPU queries

        // Output control
        uint32_t m_outputFrequency;    ///< Output frequency in frames
        bool m_outputToLog;            ///< Whether to output to log
        bool m_outputToConsole;        ///< Whether to output to console
        std::string m_outputFilename;  ///< Filename for file output

        // Threading support
        mutable std::mutex m_profilerMutex;    ///< Mutex for thread safety

        /**
         * @brief Thread-local storage for profiling
         */
        struct ThreadLocalStorage
        {
            std::stack<std::string> activeSamples; ///< Active samples for this thread
            std::string threadName;    ///< Name of this thread
        };

        /**
         * @brief Thread-local storage instance
         */
        static thread_local ThreadLocalStorage tls;
    };

    // Convenient macro for scoped profiling
#ifdef ENABLE_PROFILING
#define PROFILE_SCOPE(name) PixelCraft::Utility::ScopedProfiler scopedProfiler##__LINE__(name)
#define PROFILE_FUNCTION() PixelCraft::Utility::ScopedProfiler scopedProfiler##__LINE__(__FUNCTION__)
#define PROFILE_BEGIN(name) PixelCraft::Utility::Profiler::getInstance().beginSample(name)
#define PROFILE_END() PixelCraft::Utility::Profiler::getInstance().endSample()
#else
#define PROFILE_SCOPE(name)
#define PROFILE_FUNCTION()
#define PROFILE_BEGIN(name)
#define PROFILE_END()
#endif

} // namespace PixelCraft::Utility