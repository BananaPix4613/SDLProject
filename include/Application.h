#pragma once

#include "CubeGrid.h"
#include "CubeRenderer.h"
#include "DebugRenderer.h"
#include "IsometricCamera.h"
#include "ImGuiWrapper.h"
#include "Frustum.h"
#include <Shader.h>
#include <GLFW/glfw3.h>

class Application
{
private:
    GLFWwindow* window;
    int width, height;

    CubeGrid* grid;
    CubeRenderer* renderer;
    DebugRenderer* debugRenderer;
    IsometricCamera* camera;
    Shader* shader;
    Shader* shadowShader;
    ImGuiWrapper* imGui;
    Frustum viewFrustum;

    bool showDebugView;
    int currentDebugView; // 0 = normal, 1 = depth map, 2 = shadow map, etc.
    bool showUI;          // Toggle for UI visibility
    bool enableFrustumCulling; // Toggle for frustum culling

    unsigned int depthMapFBO;
    unsigned int depthMap;
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    unsigned int debugLineVAO, debugLineVBO;
    bool showFrustumDebug;

    float lastFrame;
    float deltaTime;
    int visibleCubeCount;

    // UI state variables
    glm::vec3 selectedCubeColor;
    bool isEditing;
    int brushSize;
    int selectedCubeX, selectedCubeY, selectedCubeZ;

public:
    Application(int windowWidth, int windowHeight);
    ~Application();

    bool initialize();
    void run();

    void setVisibleCubeCount(int visibleCount);

    // Make these methods public so CubeRenderer can access them
    bool isCubeVisible(int x, int y, int z) const;
    const Frustum& getViewFrustum() const { return viewFrustum; }
    bool isFrustumCullingEnabled() const { return enableFrustumCulling; }

private:
    bool initializeWindow();
    void setupShadowMap();
    void processInput();
    void update();
    void render();
    void renderUI();
    void renderSceneDepth(Shader &depthShader);
    void renderFrustumDebug();
    void updateViewFrustum();

    // Debug functions
    std::vector<glm::vec3> getFrustumCorners() const;

    // Window size control
    void resizeWindow(int newWidth, int newHeight);
    void resizeWindow(float newWidth, float newHeight);

    // Editing functions
    void setCubeAt(int x, int y, int z, bool active, const glm::vec3& color);
    bool pickCube(int& outX, int& outY, int& outZ);
    void clearGrid(bool resetFloor);

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