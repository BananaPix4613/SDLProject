#pragma once

#include "CubeGrid.h"
#include "CubeRenderer.h"
#include "DebugRenderer.h"
#include "FileDialog.h"
#include "Frustum.h"
#include "GridSerializer.h"
#include "Camera.h"
#include "Profiler.h"
#include "RenderSettings.h"
#include "UIManager.h"
#include <Shader.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <string>
#include <limits>
#include <ctime>

// Required for Windows file dialog integration
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

class UIManager; // Forward declaration

class Application
{
private:
    GLFWmonitor* primaryMonitor;
    GLFWwindow* window;
    int width, height;
    bool isFullscreen;

    CubeGrid* grid;
    CubeRenderer* renderer;
    DebugRenderer* debugRenderer;
    RenderSettings renderSettings;
    Camera* camera;
    Shader* shader;
    Shader* shadowShader;
    Shader* instancedShader;
    Shader* instancedShadowShader;
    UIManager* uiManager;
    Frustum viewFrustum;
    Profiler profiler;

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

    bool showUI;          // Toggle for UI visibility

    unsigned int depthMapFBO;
    unsigned int depthMap;
    unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    unsigned int debugLineVAO, debugLineVBO;

    unsigned int sceneFramebuffer;
    unsigned int sceneColorTexture;
    unsigned int sceneDepthTexture;

    float lastFrame;
    float deltaTime;
    int visibleCubeCount;

    // Viewport variables
    int viewportX, viewportY, viewportWidth, viewportHeight;
    bool viewportActive;

    // UI state variables
    glm::vec3 selectedCubeColor;
    bool isEditing;
    int brushSize;
    int selectedCubeX, selectedCubeY, selectedCubeZ;
    int chunkViewDistance = 5;
    float maxViewDistance = 500.0f;
    bool useInstanceCache = true;
    bool perCubeCulling = true;
    int batchSize = 10000;

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

    int getVisibleCubeCount() const { return visibleCubeCount; }
    void setVisibleCubeCount(int visibleCount);

    const Frustum& getViewFrustum() const { return viewFrustum; }
    const glm::vec3 getCameraPosition() const { return camera->getPosition(); }
    unsigned int getInstancedShaderID() const { return instancedShader ? instancedShader->ID : 0; }
    unsigned int getInstancedShadowShaderID() const { return instancedShadowShader ? instancedShadowShader->ID : 0; }

    bool isCubeVisible(int x, int y, int z) const;
    bool isFrustumCullingEnabled() const { return renderSettings.enableFrustumCulling; }

    void setupShadowMap();
    GLFWwindow* getWindow() const { return window; }
    void getWindowSize(int& w, int& h) const { w = width, h = height; }
    void resizeWindow(int newWidth, int newHeight);
    void resizeWindow(float newWidth, float newHeight);
    void toggleFullscreen();
    bool getIsFullscreen() const { return isFullscreen; }

    void setViewportSize(int width, int height);
    void setViewportPos(int x, int y);
    void setCameraAspectRatio(float aspect);
    bool isViewportActive() const { return viewportActive; }

    CubeGrid* getGrid() const { return grid; }
    CubeRenderer* getRenderer() const { return renderer; }
    DebugRenderer* getDebugRenderer() const { return debugRenderer; }
    Camera* getCamera() { return camera; }
    Profiler& getProfiler() { return profiler; }
    RenderSettings& getRenderSettings() { return renderSettings; }

    void setCubeAt(int x, int y, int z, bool active, const glm::vec3& color);
    bool pickCube(int& outX, int& outY, int& outZ);
    void clearGrid(bool resetFloor);

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
    void setupFramebuffer();
    void renderSceneToFrameBuffer();
    void renderUI();
    void renderFrustumDebug();
    void renderDebugView();
    void updateViewFrustum();

    // Debug functions
    std::vector<glm::vec3> getFrustumCorners() const;

    // File handling
    std::string generateAutoSaveFilename() const;
    void performAutoSave();

    // Static callbacks
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

    // Error handling
    static void GLAPIENTRY errorCallback(GLenum source, GLenum type, GLuint id,
                                         GLenum severity, GLsizei length,
                                         const GLchar* message, const void* userParam);
};