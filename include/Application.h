#pragma once

#include "CubeGrid.h"
#include "CubeRenderer.h"
#include "DebugRenderer.h"
#include "FileDialog.h"
#include "Frustum.h"
#include "GridSerializer.h"
#include "ImGuiWrapper.h"
#include "IsometricCamera.h"
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

class Application
{
private:
    GLFWwindow* window;
    int width, height;

    CubeGrid* grid;
    CubeRenderer* renderer;
    DebugRenderer* debugRenderer;
    RenderSettings renderSettings;
    IsometricCamera* camera;
    Shader* shader;
    Shader* shadowShader;
    Shader* instancedShader;
    Shader* instancedShadowShader;
    ImGuiWrapper* imGui;
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

    struct Notification
    {
        std::string message;
        float timeRemaining;
        bool isError;

        Notification(const std::string& msg, bool error, float duration = 3.0f)
            : message(msg), timeRemaining(duration), isError(error) {}
    };

    std::vector<Notification> notifications;

    bool showUI;          // Toggle for UI visibility

    unsigned int depthMapFBO;
    unsigned int depthMap;
    unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    unsigned int debugLineVAO, debugLineVBO;

    float lastFrame;
    float deltaTime;
    int visibleCubeCount;

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

    void setVisibleCubeCount(int visibleCount);
    int getVisibleCubeCount() const { return visibleCubeCount; }

    const Frustum& getViewFrustum() const { return viewFrustum; }
    const glm::vec3 getCameraPosition() const { return camera->getPosition(); }
    unsigned int getInstancedShaderID() const { return instancedShader ? instancedShader->ID : 0; }
    unsigned int getInstancedShadowShaderID() const { return instancedShadowShader ? instancedShadowShader->ID : 0; }

    bool isCubeVisible(int x, int y, int z) const;
    bool isFrustumCullingEnabled() const { return renderSettings.enableFrustumCulling; }

private:
    bool initializeWindow();
    void setupShadowMap();
    void processInput();
    void update();
    void render();
    void renderUI();
    void renderSettingsUI();
    void renderFileOperationsUI();
    void renderSceneDepth(Shader &depthShader);
    void renderFrustumDebug();
    void renderDebugView();
    void renderNotifications();
    void updateViewFrustum();

    // Debug functions
    std::vector<glm::vec3> getFrustumCorners() const;

    // Notifications
    void showNotification(const std::string& message, bool isError = false);

    // Window size control
    void resizeWindow(int newWidth, int newHeight);
    void resizeWindow(float newWidth, float newHeight);

    // Editing functions
    void setCubeAt(int x, int y, int z, bool active, const glm::vec3& color);
    bool pickCube(int& outX, int& outY, int& outZ);
    void clearGrid(bool resetFloor);

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