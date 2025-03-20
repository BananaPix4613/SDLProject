#include "ImGuiWrapper.h"

ImGuiWrapper::ImGuiWrapper() : window(nullptr), initialized(false)
{

}

ImGuiWrapper::~ImGuiWrapper()
{
    if (initialized)
    {
        shutdown();
    }
}

bool ImGuiWrapper::initialize(GLFWwindow* mainWindow)
{
    if (initialized)
    {
        return true;
    }

    window = mainWindow;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    // Add this line to check if it's being inappropriately set
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable keyboard controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable docking

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    initialized = true;
    return true;
}

void ImGuiWrapper::newFrame()
{
    if (!initialized)
    {
        return;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiWrapper::render()
{
    if (!initialized)
    {
        return;
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiWrapper::shutdown()
{
    if (!initialized)
    {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    initialized = false;
}

bool ImGuiWrapper::beginWindow(const char* name, bool* p_open)
{
    return ImGui::Begin(name, p_open);
}

void ImGuiWrapper::endWindow()
{
    ImGui::End();
}
