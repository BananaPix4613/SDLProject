#pragma once

#include "CubeGrid.h"
#include "CubeRenderer.h"
#include "DebugRenderer.h"
#include "IsometricCamera.h"
#include "ImGuiWrapper.h"
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

    bool showDebugView;
    int currentDebugView; // 0 = normal, 1 = depth map, 2 = shadow map, etc.
    bool showUI;          // Toggle for UI visibility

    unsigned int depthMapFBO;
    unsigned int depthMap;
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

    float lastFrame;
    float deltaTime;

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

private:
    bool initializeWindow();
    void setupShadowMap();
    void processInput();
    void update();
    void render();
    void renderUI();
    void renderSceneDepth(Shader &depthShader);

    // Editing functions
    void setCubeAt(int x, int y, int z, bool active, const glm::vec3& color);
    bool pickCube(int& outX, int& outY, int& outZ);

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