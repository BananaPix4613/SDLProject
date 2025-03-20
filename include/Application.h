#pragma once

#include "CubeGrid.h"
#include "CubeRenderer.h"
#include "IsometricCamera.h"
#include <shader.h>
#include <GLFW/glfw3.h>

class Application
{
private:
    GLFWwindow* window;
    int width, height;

    CubeGrid* grid;
    CubeRenderer* renderer;
    IsometricCamera* camera;
    Shader* shader;

    float lastFrame;
    float deltaTime;

public:
    Application(int windowWidth, int windowHeight);
    ~Application();

    bool initialize();
    void run();

private:
    bool initializeWindow();
    void processInput();
    void update();
    void render();

    // Static callbacks
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    // Error handling
    static void GLAPIENTRY errorCallback(GLenum source, GLenum type, GLuint id,
                                         GLenum severity, GLsizei length,
                                         const GLchar* message, const void* userParam);
};