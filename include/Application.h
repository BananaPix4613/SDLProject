#pragma once

#include "CubeGrid.h"
#include "Camera.h"
#include "Frustum.h"
#include "GridSerializer.h"
#include "Profiler.h"
#include "RenderSettings.h"
#include <Shader.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <string>
#include <limits>
#include <ctime>

// Required for Windows file dialog integration
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

// Forward declarations
class ApplicationRenderIntegration;
class UIManager;
class Frustum;

class Application
{
private:
    GLFWmonitor* primaryMonitor;
    GLFWwindow* window;
    int width, height;
    bool isFullscreen;

    // Core systems
    CubeGrid* grid;
    IsometricCamera* camera;
    ApplicationRenderIntegration* renderIntegration;
    UIManager* uiManager;
    RenderSettings renderSettings;
    Profiler profiler;

    // Frame timing
    float lastFrame;
    float deltaTime;

    // Rendering statistics
    int visibleCubeCount;

    // Frustum for culling
    Frustum viewFrustum;

    struct CullingStats {
        int totalActiveCubes;
        int visibleCubes;
        int culledCubes;
        float cullingPercentage;
        float lastUpdateTime;
        std::vector<float> fpsHistory;
        std::vector<float> cullingHistory;

        CullingStats() : totalActiveCubes(0), visibleCubes(0), culledCubes(0),
            cullingPercentage(0.0f), lastUpdateTime(0.0f)
        {
            fpsHistory.resize(100, 0.0f);
            cullingHistory.resize(100, 0.0f);
        }
    };

    CullingStats cullingStats;

    // UI state
    bool showUI;

    // Viewport variables
    int viewportX, viewportY, viewportWidth, viewportHeight;
    bool viewportActive;

    // UI state variables
    glm::vec3 selectedCubeColor;
    bool isEditing;
    int brushSize;
    int selectedCubeX, selectedCubeY, selectedCubeZ;
    int chunkViewDistance = 5;

    // Auto-save features
    bool enableAutoSave;
    int autoSaveInterval; // In minutes
    std::string autoSaveFolder;
    double lastAutoSaveTime;

public:
    Application(int windowWidth, int windowHeight);
    ~Application();

    bool initialize();
    void run();

    // Statistics and visiblity
    int getVisibleCubeCount() const { return visibleCubeCount; }
    void setVisibleCubeCount(int visibleCount);
    bool isCubeVisible(int x, int y, int z) const;

    // Window management
    void getWindowSize(int& w, int& h) const { w = width, h = height; }
    void resizeWindow(int newWidth, int newHeight);
    void resizeWindow(float newWidth, float newHeight);
    void toggleFullscreen();
    bool getIsFullscreen() const { return isFullscreen; }
    GLFWwindow* getWindow() const { return window; }

    // Viewport management
    void setViewportSize(int width, int height);
    void setViewportPos(int x, int y);
    bool isViewportActive() const { return viewportActive; }
    void setCameraAspectRatio(float aspect);

    // Settings management
    void updateRenderSettings();
    RenderSettings& getRenderSettings() { return renderSettings; }
    void setupShadowMap();

    // Access to core components
    CubeGrid* getGrid() const { return grid; }
    IsometricCamera* getCamera() const { return camera; }
    Profiler& getProfiler() { return profiler; }
    UIManager* getUIManager() const { return uiManager; }
    ApplicationRenderIntegration* getRenderIntegration() const { return renderIntegration; }

    // Cube editing
    void setCubeAt(int x, int y, int z, bool active, const glm::vec3& color);
    bool pickCube(int& outX, int& outY, int& outZ);
    void clearGrid(bool resetFloor);

    // Editing state
    void setEditingMode(bool editing) { isEditing = editing; }
    bool getEditingMode() const { return isEditing; }
    void setBrushSize(int size) { brushSize = size; }
    int getBrushSize() const { return brushSize; }
    void setSelectedCubeColor(const glm::vec3& color) { selectedCubeColor = color; }
    const glm::vec3& getSelectedCubeColor() const { return selectedCubeColor; }
    void setSelectedCubeCoords(int x, int y, int z)
    {
        selectedCubeX = x;
        selectedCubeY = y;
        selectedCubeZ = z;
    }

    // Auto-save settings
    void setAutoSaveSettings(bool enable, int intervalMinutes, const std::string& folder)
    {
        enableAutoSave = enable;
        autoSaveInterval = intervalMinutes;
        autoSaveFolder = folder;
        lastAutoSaveTime = glfwGetTime(); // Reset timer
    }

private:
    bool initializeWindow();
    void processInput();
    void update();
    void updateViewFrustum();

    // File handling
    std::string generateAutoSaveFilename() const;
    void performAutoSave();

    // Static callbacks
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void GLAPIENTRY errorCallback(GLenum source, GLenum type, GLuint id,
                                         GLenum severity, GLsizei length,
                                         const GLchar* message, const void* userParam);
};