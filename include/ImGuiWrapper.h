#pragma once

#include <glad/glad.h>
#include <glfw/glfw3.h>

#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

class ImGuiWrapper
{
private:
    GLFWwindow* window;
    bool initialized;

public:
    ImGuiWrapper();
    ~ImGuiWrapper();

    bool initialize(GLFWwindow* mainWindow);
    void newFrame();
    void render();
    void shutdown();

    // Helper methods for common widgets
    bool beginWindow(const char* name, bool* p_open = nullptr);
    void endWindow();

    // Can add more helper methods for common ImGui widgets as needed
};