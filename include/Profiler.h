#pragma once

#include "ImGuiWrapper.h"
#include <chrono>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <deque>
#include <GLFW/glfw3.h>

// Simple profiler class
class Profiler
{
private:
    // Structure to store a single profile measurement
    struct ProfileMeasurement {
        double duration;
        double frameTime; // When the measurement was taken
    };

    // Structure to store profile statistics for a named section
    struct ProfileStats {
        std::string name;
        std::deque<ProfileMeasurement> history; // Recent measurements
        double averageDuration;
        double minDuration;
        double maxDuration;
        size_t callCount;

        ProfileStats() : averageDuration(0.0), minDuration(999999.0),
            maxDuration(0.0), callCount(0) {
        }
    };

    // Active profiling points (currently being timed)
    struct ActivePoint {
        std::string name;
        double startTime = 0.0;
    };

    std::vector<ActivePoint> activePoints;
    std::map<std::string, ProfileStats> statsMap;

    bool enabled;
    bool paused;

    // Configuration
    const size_t MAX_HISTORY_PER_PROFILE = 100; // Maximum measurements to keep per profile
    const size_t MAX_DISPLAYED_PROFILES = 20;  // Maximum rows to show in the table

    double frameStartTime;
    double lastFrameTime;
    double currentFPS;

public:
    Profiler()
        : enabled(true), paused(false), frameStartTime(0.0), lastFrameTime(0.0), currentFPS(0.0)
    {
        
    }

    void setEnabled(bool enable) { enabled = enable; }
    void setPaused(bool pause) { paused = pause; }

    void beginFrame() {
        if (!enabled) return;

        // Store the actual tiem when the frame starts
        frameStartTime = glfwGetTime();

        // Clear active points at the start of each frame
        activePoints.clear();
    }

    void endFrame() {
        if (!enabled) return;

        // Calculate frame time
        double currentTime = glfwGetTime();
        lastFrameTime = currentTime - frameStartTime;

        // Don't allow zero frame time to avoid division by zero
        if (lastFrameTime < 0.0001) lastFrameTime = 0.0001;

        // Calculate FPS directly from frame time
        currentFPS = 1.0 / lastFrameTime;

        // Add to basic measurements
        if (!paused) {
            addMeasurement("Total Frame Time", lastFrameTime);
            addMeasurement("FPS", currentFPS);
        }
    }

    void startProfile(const std::string& name) {
        if (!enabled || paused) return;

        ActivePoint point;
        point.name = name;
        point.startTime = glfwGetTime();
        activePoints.push_back(point);
    }

    void endProfile(const std::string& name) {
        if (!enabled || paused) return;

        double endTime = glfwGetTime();

        // Find the matching start point
        for (auto it = activePoints.begin(); it != activePoints.end(); ++it) {
            if (it->name == name) {
                double duration = endTime - it->startTime;
                addMeasurement(name, duration);

                // Remove the active point
                activePoints.erase(it);
                return;
            }
        }
    }

    void clearProfiles() {
        statsMap.clear();
        activePoints.clear();
    }

    void addMeasurement(const std::string& name, double duration) {
        // Create stats entry if it doesn't exist
        if (statsMap.find(name) == statsMap.end()) {
            statsMap[name] = ProfileStats();
            statsMap[name].name = name;
        }

        ProfileStats& stats = statsMap[name];

        // Add the new measurement
        ProfileMeasurement measurement;
        measurement.duration = duration;
        measurement.frameTime = glfwGetTime();

        stats.history.push_back(measurement);

        // Limit history size
        while (stats.history.size() > MAX_HISTORY_PER_PROFILE) {
            stats.history.pop_front();
        }

        // Update statistics
        stats.callCount++;

        // Update min/max (convert to milliseconds for display)
        double durationMs = duration * 1000.0;

        if (stats.history.size() == 1 || durationMs < stats.minDuration)
        {
            stats.minDuration = durationMs;
        }

        if (stats.history.size() == 1 || durationMs > stats.maxDuration)
        {
            stats.maxDuration = durationMs;
        }

        // Recalculate average (in milliseconds for display)
        double sum = 0.0;
        for (const auto& m : stats.history) {
            sum += m.duration * 1000.0; // Convert to ms
        }
        stats.averageDuration = sum / stats.history.size();
    }

    double getLastTime(const std::string& name) const
    {
        auto it = statsMap.find(name);
        if (it != statsMap.end() && !it->second.history.empty())
        {
            return it->second.history.back().duration;
        }
        return 0.0; // Return 0 if no data is available
    }

    void drawImGuiWindow() {
        if (!ImGui::Begin("Profiler")) {
            ImGui::End();
            return;
        }

        // Controls
        ImGui::Checkbox("Enable Profiling", &enabled);
        ImGui::SameLine();
        ImGui::Checkbox("Pause", &paused);
        ImGui::SameLine();

        if (ImGui::Button("Clear")) {
            clearProfiles();
        }

        // Display current FPS
        ImGui::Text("FPS: %.1f (%.3f ms/frame)", currentFPS, lastFrameTime * 1000.0);

        if (statsMap.empty()) {
            ImGui::Text("No profile data collected.");
            ImGui::End();
            return;
        }

        // Convert map to vector for sorting
        std::vector<ProfileStats*> sortedStats;
        for (auto& pair : statsMap) {
            sortedStats.push_back(&pair.second);
        }

        // Sort by average duration (descending)
        std::sort(sortedStats.begin(), sortedStats.end(),
            [](const ProfileStats* a, const ProfileStats* b) {
                return a->averageDuration > b->averageDuration;
            });

        // Limit display size
        size_t displayCount = min(sortedStats.size(), MAX_DISPLAYED_PROFILES);

        // Table display
        ImGui::Columns(4, "profileTable");
        ImGui::Separator();
        ImGui::Text("Name"); ImGui::NextColumn();
        ImGui::Text("Avg (ms)"); ImGui::NextColumn();
        ImGui::Text("Min/Max (ms)"); ImGui::NextColumn();
        ImGui::Text("% of Frame"); ImGui::NextColumn();
        ImGui::Separator();

        // Find the total frame time stat for percentage calculation
        double totalFrameTimeAvg = 0.0;
        auto it = statsMap.find("Total Frame Time");
        if (it != statsMap.end())
        {
            totalFrameTimeAvg = it->second.averageDuration;
        }

        if (totalFrameTimeAvg < 0.001) totalFrameTimeAvg = 0.001; // Avoid division by zero

        for (size_t i = 0; i < displayCount; i++) {
            const ProfileStats* stat = sortedStats[i];

            ImGui::Text("%s", stat->name.c_str()); ImGui::NextColumn();
            ImGui::Text("%.3f", stat->averageDuration * 1000.0); ImGui::NextColumn();
            ImGui::Text("%.2f / %.2f", stat->minDuration * 1000.0, stat->maxDuration * 1000.0); ImGui::NextColumn();

            // Calculate percentage of total frame time
            double percentage = 100.0 * stat->averageDuration / totalFrameTimeAvg;

            // Cap percentage to 100% for display purposes
            if (percentage > 100.0 && stat->name != "Total Frame Time")
            {
                percentage = 100.0;
            }

            ImGui::Text("%.1f%%", percentage); ImGui::NextColumn();
        }

        ImGui::Columns(1);
        ImGui::Separator();

        // If we've truncated the list, show how many more there are
        if (sortedStats.size() > MAX_DISPLAYED_PROFILES) {
            ImGui::Text("... and %zu more profiles (not shown)",
                sortedStats.size() - MAX_DISPLAYED_PROFILES);
        }

        ImGui::End();
    }
};