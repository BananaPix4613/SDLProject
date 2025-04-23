// -------------------------------------------------------------------------
// Profiler.cpp
// -------------------------------------------------------------------------
#include "Utility/Profiler.h"
#include "Core/Logger.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <iostream>

namespace Log = PixelCraft::Core;

namespace PixelCraft::Utility
{

    // Initialize thread-local storage
    thread_local Profiler::ThreadLocalStorage Profiler::tls;

    //-------------------------------------------------------------------------
    // ScopedProfiler Implementation
    //-------------------------------------------------------------------------

    ScopedProfiler::ScopedProfiler(const std::string& name) : m_name(name)
    {
        if (Profiler::getInstance().isProfilingEnabled())
        {
            Profiler::getInstance().beginSample(m_name);
        }
    }

    ScopedProfiler::~ScopedProfiler()
    {
        if (Profiler::getInstance().isProfilingEnabled())
        {
            Profiler::getInstance().endSample();
        }
    }

    //-------------------------------------------------------------------------
    // Profiler Implementation
    //-------------------------------------------------------------------------

    Profiler& Profiler::getInstance()
    {
        static Profiler instance;
        return instance;
    }

    Profiler::Profiler()
        : m_enabled(false)
        , m_displayMode(ProfilerDisplayMode::Disabled)
        , m_maxFrameStats(120)
        , m_frameCount(0)
        , m_frameStartTime(0.0)
        , m_frameTime(0.0)
        , m_cpuTime(0.0)
        , m_gpuTime(0.0)
        , m_outputFrequency(0)
        , m_outputToLog(false)
        , m_outputToConsole(false)
        , m_outputFilename("")
    {
    }

    Profiler::~Profiler()
    {
        if (m_enabled)
        {
            shutdown();
        }
    }

    bool Profiler::initialize()
    {
        std::lock_guard<std::mutex> lock(m_profilerMutex);

        if (m_enabled)
        {
            Log::warn("Profiler already initialized");
            return true;
        }

        Log::info("Initializing Profiler subsystem");

        m_enabled = true;
        m_displayMode = ProfilerDisplayMode::Disabled;
        m_samples.clear();
        m_sampleNames.clear();
        m_frameStats.clear();
        m_frameStats.reserve(m_maxFrameStats);
        m_frameCount = 0;

        // Set main thread name if not already set
        if (tls.threadName.empty())
        {
            tls.threadName = "MainThread";
            m_threadNames[std::this_thread::get_id()] = "MainThread";
        }

        return true;
    }

    void Profiler::shutdown()
    {
        std::lock_guard<std::mutex> lock(m_profilerMutex);

        if (!m_enabled)
        {
            return;
        }

        Log::info("Shutting down Profiler subsystem");

        // Output final stats if requested
        if (m_outputToLog)
        {
            outputToLog();
        }

        if (!m_outputFilename.empty())
        {
            outputToFile(m_outputFilename);
        }

        if (m_outputToConsole)
        {
            outputToConsole();
        }

        // Clean up resources
        m_samples.clear();
        m_sampleNames.clear();
        m_frameStats.clear();
        m_threadNames.clear();
        m_gpuQueries.clear();

        while (!m_activeSamples.empty())
        {
            m_activeSamples.pop();
        }

        m_threadActiveSamples.clear();

        m_enabled = false;
    }

    void Profiler::enableProfiling(bool enable)
    {
        std::lock_guard<std::mutex> lock(m_profilerMutex);

        if (m_enabled == enable)
        {
            return;
        }

        m_enabled = enable;

        if (enable)
        {
            if (m_samples.empty())
            {
                initialize();
            }
        }
        else
        {
            // Clear active samples when disabling
            while (!m_activeSamples.empty())
            {
                m_activeSamples.pop();
            }

            m_threadActiveSamples.clear();
        }

        Log::info(enable ? "Profiling enabled" : "Profiling disabled");
    }

    bool Profiler::isProfilingEnabled() const
    {
        return m_enabled;
    }

    void Profiler::beginSample(const std::string& name)
    {
        if (!m_enabled)
        {
            return;
        }

        double startTime = getCurrentTimeMs();

        std::thread::id threadId = std::this_thread::get_id();

        {
            std::lock_guard<std::mutex> lock(m_profilerMutex);

            // Get or create the profile sample
            ProfileSample& sample = getThreadSample(name);
            sample.startTime = startTime;
            sample.isOpen = true;

            // Set up parent-child relationship
            if (!tls.activeSamples.empty())
            {
                std::string parentName = tls.activeSamples.top();
                ProfileSample& parentSample = m_samples[parentName];

                // Check if this child already exists in the parent's children
                auto it = std::find_if(parentSample.children.begin(), parentSample.children.end(),
                                       [&name](const ProfileSample& child) { return child.name == name; });

                if (it == parentSample.children.end())
                {
                    // Add as a new child
                    parentSample.children.push_back(sample);
                    parentSample.children.back().parent = &parentSample;
                }

                // Set parent for this sample
                sample.parent = &parentSample;
            }
            else
            {
                sample.parent = nullptr;
            }

            // Push to active samples stack
            tls.activeSamples.push(name);
            m_threadActiveSamples[threadId] = tls.activeSamples;
        }
    }

    void Profiler::endSample()
    {
        if (!m_enabled || tls.activeSamples.empty())
        {
            return;
        }

        double endTime = getCurrentTimeMs();
        std::string name = tls.activeSamples.top();

        {
            std::lock_guard<std::mutex> lock(m_profilerMutex);

            // Update the sample
            if (m_samples.find(name) != m_samples.end())
            {
                ProfileSample& sample = m_samples[name];
                sample.endTime = endTime;
                sample.duration = endTime - sample.startTime;
                sample.isOpen = false;

                // Update statistics
                updateSampleStatistics(sample, sample.duration);
            }

            // Pop from active samples stack
            tls.activeSamples.pop();
            m_threadActiveSamples[std::this_thread::get_id()] = tls.activeSamples;
        }
    }

    void Profiler::beginFrame()
    {
        if (!m_enabled)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(m_profilerMutex);

        m_frameStartTime = getCurrentTimeMs();
        m_frameCount++;

        // Reset active sample stacks to handle cases where samples were not properly ended
        tls.activeSamples = std::stack<std::string>();
        m_threadActiveSamples[std::this_thread::get_id()] = tls.activeSamples;

        // Clear active GPU queries
        while (!m_activeGPUQueries.empty())
        {
            m_activeGPUQueries.pop();
        }
    }

    void Profiler::endFrame()
    {
        if (!m_enabled)
        {
            return;
        }

        double endTime = getCurrentTimeMs();

        std::lock_guard<std::mutex> lock(m_profilerMutex);

        // Calculate frame time
        m_frameTime = endTime - m_frameStartTime;

        // Store frame stats
        FrameStats frameStats;
        frameStats.frameTime = m_frameTime;
        frameStats.cpuTime = m_cpuTime > 0.0 ? m_cpuTime : m_frameTime;
        frameStats.gpuTime = m_gpuTime;
        frameStats.frameNumber = m_frameCount;
        frameStats.samples = m_samples;

        m_frameStats.push_back(frameStats);

        // Limit stored frame stats
        while (m_frameStats.size() > m_maxFrameStats)
        {
            m_frameStats.erase(m_frameStats.begin());
        }

        // Output stats if requested
        if (m_outputFrequency > 0 && (m_frameCount % m_outputFrequency) == 0)
        {
            if (m_outputToLog)
            {
                outputToLog();
            }

            if (!m_outputFilename.empty())
            {
                outputToFile(m_outputFilename);
            }

            if (m_outputToConsole)
            {
                outputToConsole();
            }
        }
    }

    const ProfileSample* Profiler::getSample(const std::string& name) const
    {
        std::lock_guard<std::mutex> lock(m_profilerMutex);

        auto it = m_samples.find(name);
        if (it != m_samples.end())
        {
            return &(it->second);
        }

        return nullptr;
    }

    const std::vector<std::string>& Profiler::getSampleNames() const
    {
        return m_sampleNames;
    }

    const std::vector<FrameStats>& Profiler::getFrameStats() const
    {
        return m_frameStats;
    }

    void Profiler::resetStats()
    {
        std::lock_guard<std::mutex> lock(m_profilerMutex);

        // Reset all samples
        for (auto& pair : m_samples)
        {
            ProfileSample& sample = pair.second;
            sample.callCount = 0;
            sample.duration = 0.0;
            sample.minTime = std::numeric_limits<double>::max();
            sample.maxTime = 0.0;
            sample.averageTime = 0.0;
        }

        // Clear frame stats
        m_frameStats.clear();
    }

    void Profiler::resetSample(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(m_profilerMutex);

        auto it = m_samples.find(name);
        if (it != m_samples.end())
        {
            ProfileSample& sample = it->second;
            sample.callCount = 0;
            sample.duration = 0.0;
            sample.minTime = std::numeric_limits<double>::max();
            sample.maxTime = 0.0;
            sample.averageTime = 0.0;
        }
    }

    void Profiler::setOutputFrequency(uint32_t frameInterval)
    {
        std::lock_guard<std::mutex> lock(m_profilerMutex);
        m_outputFrequency = frameInterval;
    }

    void Profiler::outputToLog()
    {
        if (!m_enabled)
        {
            return;
        }

        Log::info("Profile results for frame " + std::to_string(m_frameCount));
        
        for (const auto& name : m_sampleNames) {
            const ProfileSample& sample = m_samples[name];
            Log::info(formatProfileSampleLog(sample));
        }
        
        Log::info("Total frame time: " + formatTime(m_frameTime));
    }

    void Profiler::outputToFile(const std::string& filename)
    {
        if (!m_enabled)
        {
            return;
        }

        m_outputFilename = filename;

        std::ofstream file(filename, std::ios::out | std::ios::app);
        if (!file.is_open())
        {
            // Logger::error("Failed to open profiler output file: " + filename);
            return;
        }

        file << "Profile results for frame " << m_frameCount << std::endl;
        file << "----------------------------------------" << std::endl;

        for (const auto& name : m_sampleNames)
        {
            const ProfileSample& sample = m_samples[name];

            file << name << ": "
                << "Avg: " << formatTime(sample.averageTime)
                << ", Min: " << formatTime(sample.minTime)
                << ", Max: " << formatTime(sample.maxTime)
                << ", Calls: " << sample.callCount
                << std::endl;
        }

        file << "Total frame time: " << formatTime(m_frameTime) << std::endl;
        file << "----------------------------------------" << std::endl;
        file.close();
    }

    void Profiler::outputToConsole()
    {
        if (!m_enabled)
        {
            return;
        }

        std::cout << "Profile results for frame " << m_frameCount << std::endl;
        std::cout << "----------------------------------------" << std::endl;

        for (const auto& name : m_sampleNames)
        {
            const ProfileSample& sample = m_samples[name];

            std::cout << name << ": "
                << "Avg: " << formatTime(sample.averageTime)
                << ", Min: " << formatTime(sample.minTime)
                << ", Max: " << formatTime(sample.maxTime)
                << ", Calls: " << sample.callCount
                << std::endl;
        }

        std::cout << "Total frame time: " << formatTime(m_frameTime) << std::endl;
        std::cout << "----------------------------------------" << std::endl;
    }

    void Profiler::setDisplayMode(ProfilerDisplayMode mode)
    {
        std::lock_guard<std::mutex> lock(m_profilerMutex);
        m_displayMode = mode;
    }

    ProfilerDisplayMode Profiler::getDisplayMode() const
    {
        return m_displayMode;
    }

    void Profiler::render()
    {
        if (!m_enabled || m_displayMode == ProfilerDisplayMode::Disabled)
        {
            return;
        }

        renderUI();
    }

    void Profiler::setThreadName(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(m_profilerMutex);
        std::thread::id threadId = std::this_thread::get_id();

        tls.threadName = name;
        m_threadNames[threadId] = name;
    }

    std::string Profiler::getThreadName(std::thread::id threadId) const
    {
        std::lock_guard<std::mutex> lock(m_profilerMutex);

        auto it = m_threadNames.find(threadId);
        if (it != m_threadNames.end())
        {
            return it->second;
        }

        return "";
    }

    void Profiler::beginGPUSample(const std::string& name)
    {
        if (!m_enabled)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(m_profilerMutex);

        // Implementation will depend on graphics API (OpenGL, DirectX, etc.)
        // Here's a placeholder for the interface

        // Get or create the GPU query
        uint32_t queryId = 0;
        auto it = m_gpuQueries.find(name);
        if (it == m_gpuQueries.end())
        {
            // Create query objects
            // glGenQueries(2, &queryId); // Start and end query IDs
            m_gpuQueries[name] = queryId;
        }
        else
        {
            queryId = it->second;
        }

        // Begin query
        // glBeginQuery(GL_TIME_ELAPSED, queryId);

        m_activeGPUQueries.push(name);
    }

    void Profiler::endGPUSample()
    {
        if (!m_enabled || m_activeGPUQueries.empty())
        {
            return;
        }

        std::lock_guard<std::mutex> lock(m_profilerMutex);

        std::string name = m_activeGPUQueries.top();
        m_activeGPUQueries.pop();

        // End query
        // glEndQuery(GL_TIME_ELAPSED);

        // Results will be available later, need to poll
        // This is simplified - real implementation would need to handle async results

        // For now, use CPU time as placeholder
        ProfileSample& sample = getThreadSample(name + "_GPU");
        sample.duration = 0.0; // Will be populated when GPU query results are available
        updateSampleStatistics(sample, sample.duration);
    }

    void Profiler::setPerformanceMarker(const std::string& name)
    {
        if (!m_enabled)
        {
            return;
        }

        // Integration with performance monitoring tools
        // This is platform-specific (PIX, RenderDoc, etc.)

        // Example for PIX on Windows:
        // PIXSetMarker(0, name.c_str());
    }

    void Profiler::beginPerformanceMarker(const std::string& name)
    {
        if (!m_enabled)
        {
            return;
        }

        // Example for PIX on Windows:
        // PIXBeginEvent(0, name.c_str());
    }

    void Profiler::endPerformanceMarker()
    {
        if (!m_enabled)
        {
            return;
        }

        // Example for PIX on Windows:
        // PIXEndEvent();
    }

    double Profiler::getCurrentTimeMs()
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch());
        return duration.count() / 1000000.0; // Convert to milliseconds
    }

    void Profiler::updateSampleStatistics(ProfileSample& sample, double duration)
    {
        // Update min/max times
        sample.minTime = (sample.callCount == 0 || duration < sample.minTime) ? duration : sample.minTime;
        sample.maxTime = (duration > sample.maxTime) ? duration : sample.maxTime;

        // Update average (running average)
        if (sample.callCount == 0)
        {
            sample.averageTime = duration;
        }
        else
        {
            // Incremental average calculation
            sample.averageTime = sample.averageTime + (duration - sample.averageTime) / (sample.callCount + 1);
        }

        // Update call count
        sample.callCount++;
    }

    ProfileSample& Profiler::getThreadSample(const std::string& name)
    {
        std::thread::id threadId = std::this_thread::get_id();

        // Check if the sample already exists
        auto it = m_samples.find(name);
        if (it == m_samples.end())
        {
            // Create a new sample
            ProfileSample sample;
            sample.name = name;
            sample.startTime = 0.0;
            sample.endTime = 0.0;
            sample.duration = 0.0;
            sample.minTime = std::numeric_limits<double>::max();
            sample.maxTime = 0.0;
            sample.averageTime = 0.0;
            sample.callCount = 0;
            sample.parent = nullptr;
            sample.isOpen = false;
            sample.threadId = threadId;

            m_samples[name] = sample;
            m_sampleNames.push_back(name);

            return m_samples[name];
        }

        return it->second;
    }

    void Profiler::renderUI()
    {
        if (m_displayMode == ProfilerDisplayMode::Disabled)
        {
            return;
        }

        // This would typically use a UI library like ImGui
        // Here's a placeholder for the interface

        // Example implementation with ImGui:
        /*
        if (ImGui::Begin("Profiler", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            // Display mode selection
            const char* modes[] = { "Disabled", "Simple", "Detailed", "Hierarchical", "Graph" };
            int currentMode = static_cast<int>(m_displayMode);
            if (ImGui::Combo("Display Mode", &currentMode, modes, IM_ARRAYSIZE(modes))) {
                m_displayMode = static_cast<ProfilerDisplayMode>(currentMode);
            }

            ImGui::Separator();

            // Frame time display
            ImGui::Text("Frame Time: %s", formatTime(m_frameTime).c_str());
            ImGui::Text("FPS: %.1f", 1000.0 / m_frameTime);

            ImGui::Separator();

            switch (m_displayMode) {
                case ProfilerDisplayMode::Simple:
                    renderSimpleView();
                    break;
                case ProfilerDisplayMode::Detailed:
                    renderDetailedView();
                    break;
                case ProfilerDisplayMode::Hierarchical:
                    renderHierarchicalView();
                    break;
                case ProfilerDisplayMode::Graph:
                    renderGraphView();
                    break;
                default:
                    break;
            }
        }
        ImGui::End();
        */
    }

    std::string Profiler::formatTime(double timeMs)
    {
        std::stringstream ss;

        if (timeMs < 1.0)
        {
            // Microseconds
            ss << std::fixed << std::setprecision(2) << (timeMs * 1000.0) << " μs";
        }
        else if (timeMs < 1000.0)
        {
            // Milliseconds
            ss << std::fixed << std::setprecision(2) << timeMs << " ms";
        }
        else
        {
            // Seconds
            ss << std::fixed << std::setprecision(2) << (timeMs / 1000.0) << " s";
        }

        return ss.str();
    }

} // namespace PixelCraft::Utility