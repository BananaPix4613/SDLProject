#include "Application.h"

int main(int argc, char** argv[])
{
    // For high DPI displays
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing

    Application app(1280, 720);

    if (!app.initialize())
    {
        return -1;
    }

    app.run();

    return 0;
}