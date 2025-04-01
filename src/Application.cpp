#include "Application.h"
#include "ApplicationRenderIntegration.h"
#include "UIManager.h"
#include <iostream>

Application::Application(int windowWidth, int windowHeight)
    : width(windowWidth), height(windowHeight), isFullscreen(false),
      window(nullptr), grid(nullptr), camera(nullptr),
      renderIntegration(nullptr), uiManager(nullptr),
      lastFrame(0.0f), deltaTime(0.0f), visibleCubeCount(0),
      showUI(true), isEditing(false), brushSize(1),
      selectedCubeX(-1), selectedCubeY(-1), selectedCubeZ(-1),
      selectedCubeColor(0.5f, 0.5f, 1.0f),
      enableAutoSave(false), autoSaveInterval(5),
      autoSaveFolder(""), lastAutoSaveTime(0.0)
{
    
}

Application::~Application()
{
    delete renderIntegration;
    renderIntegration = nullptr;

    delete grid;
    grid = nullptr;

    delete camera;
    camera = nullptr;

    delete uiManager;
    uiManager = nullptr;

    glfwDestroyWindow(window);
    glfwTerminate();
}

bool Application::initialize()
{
    if (!initializeWindow())
    {
        return false;
    }

    // Create and initialize core components
    grid = new CubeGrid(20, 1.0f);

    // Create camera
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    camera = new IsometricCamera(aspect);

    // Set camera position
    float distance = 50.0f;
    float angle = 45.0f * (3.14159f / 180.0f);
    glm::vec3 cameraPos(
        distance * cos(angle),
        distance * 0.5f,
        distance * sin(angle)
    );
    camera->setPosition(cameraPos);
    camera->setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    camera->setZoom(1.0f);

    // Initialize UI Manager
    uiManager = new UIManager(this);
    if (!uiManager->initialize(window))
    {
        std::cerr << "Failed to initialize UI Manager" << std::endl;
        return false;
    }

    // Create render integration AFTER grid and camera are initialized
    renderIntegration = new ApplicationRenderIntegration(this);
    if (!renderIntegration->initialize())
    {
        std::cerr << "Failed to initialize render system" << std::endl;
        return false;
    }

    // Set up DPI scaling for high-resolution displays
    float xscale, yscale;
    glfwGetWindowContentScale(window, &xscale, &yscale);
    float dpiScale = xscale > yscale ? xscale : yscale;

    // Initialize UI Manager with DPI scale
    uiManager->setDpiScale(dpiScale);

    // Initialize window viewport
    viewportWidth = width;
    viewportHeight = height;
    viewportX = 0;
    viewportY = 0;
    viewportActive = true;

    // Store monitor info for fullscreen toggle
    primaryMonitor = glfwGetPrimaryMonitor();
    isFullscreen = false;

    // Print initial grid stats
    std::cout << "Initial active cubes: " << grid->getTotalActiveCubeCount() << std::endl;
    std::cout << "Initial grid bounds: ("
        << grid->getMinBounds().x << "," << grid->getMinBounds().y << "," << grid->getMinBounds().z
        << ") to ("
        << grid->getMaxBounds().x << "," << grid->getMaxBounds().y << "," << grid->getMaxBounds().z
        << ")" << std::endl;

    // Set up initial render settings
    updateRenderSettings();

    // Add mouse button callback for cube picking
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    return true;
}

void Application::run()
{
    while (!glfwWindowShouldClose(window))
    {
        // Calculate delta timing
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process input and update
        glfwPollEvents();
        processInput();
        update();

        // 1. Start UI frame
        //uiManager->beginFrame();

        // 2. Update render integration (updates the scene)
        renderIntegration->update();

        // 3. Perform rendering
        renderIntegration->render();

        // 4. Finish UI rendering
        //uiManager->render();

        // Swap buffers
        glfwSwapBuffers(window);
    }
}

void Application::update()
{
    // Main grid update
    grid->update(deltaTime);

    // Update UI Manager notifications
    uiManager->updateNotifications(deltaTime);

    // Make sure camera is correctly configured
    if (camera && viewportActive)
    {
        float aspect = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
        camera->setAspectRatio(aspect);
    }

    // Auto-save check
    if (enableAutoSave && !autoSaveFolder.empty())
    {
        double currentTime = glfwGetTime();
        double autoSaveIntervalSeconds = autoSaveInterval * 60.0f;

        if (currentTime - lastAutoSaveTime >= autoSaveIntervalSeconds)
        {
            performAutoSave();
        }
    }

    // Check if it's time to update loaded chunks (don't do this every frame)
    static float chunkUpdateTimer = 0.0f;
    chunkUpdateTimer += deltaTime;

    // Update loaded chunks based on camera position every frame
    if (chunkUpdateTimer >= 1.0f)
    {
        profiler.startProfile("ChunkUpdate");

        // Get current view distance from settings
        static int viewDistance = 8; // Default, can be changed in UI

        // Convert camera position to grid coordinates
        glm::vec3 camWorldPos = camera->getPosition();
        glm::ivec3 camGridPos = grid->worldToGridCoordinates(camWorldPos);

        // Update which chunks are loaded
        grid->updateLoadedChunks(camGridPos, viewDistance);

        // Reset timer
        chunkUpdateTimer = 0.0f;

        profiler.endProfile("ChunkUpdate");
    }

    // Update view frustum for culling
    updateViewFrustum();
}

void Application::processInput()
{
    // Check if ImGui wants to capture input
    ImGuiIO& io = ImGui::GetIO();

    // Check if we're hovering over the viewport specifically
    bool hoveringViewport = false;
    ImGuiWindow* viewportWindow = ImGui::FindWindowByName("Viewport");
    if (viewportWindow && !viewportWindow->Collapsed)
    {
        // Get mouse position
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // Check if mouse is inside viewport content area
        ImVec2 min = viewportWindow->InnerRect.Min;
        ImVec2 max = viewportWindow->InnerRect.Max;

        if (mouseX >= min.x && mouseX <= max.x &&
            mouseY >= min.y && mouseY <= max.y)
        {
            hoveringViewport = true;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    // Only process game inputs if ImGui isn't capturing
    if (hoveringViewport)
    {
        // Camera movement
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        {
            camera->rotate(deltaTime * 1.0f); // Rotate left
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        {
            camera->rotate(-deltaTime * 1.0f); // Rotate right
        }

        // Camera panning - improved to be view-relative
        float panSpeed = 5.0f * deltaTime;
        glm::vec3 forward(0.0f), right(0.0f);

        // Extract forward and right vectors from view matrix
        glm::mat4 viewMatrix = camera->getViewMatrix();

        // Forward is negative z-axis of view matrix (third row with sign flipped)
        forward = -glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);

        // Right is positive x-axis of view matrix (first row)
        right = glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);

        // Normalize vectors to ensure consistent movement speed
        forward = glm::normalize(forward);
        right = glm::normalize(right);

        // Force movement to be only on the xz-plane (horizontal)
        forward.y = 0.0f;
        right.y = 0.0f;

        // Renormalize after zeroing y component
        if (glm::length(forward) > 0.001f) forward = glm::normalize(forward);
        if (glm::length(right) > 0.001f) right = glm::normalize(right);

        glm::vec3 panDirection(0.0f);

        // Apply movement based on WASD keys
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            panDirection += forward;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            panDirection -= forward;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            panDirection -= right;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            panDirection += right;
        }

        if (glm::length(panDirection) > 0.001f)
        {
            panDirection = glm::normalize(panDirection) * panSpeed;
            camera->pan(panDirection);
        }

        // Window size hotkeys (optional)
        // Hold Ctrl and press + to increase and - to decrease window size
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
        {
            static bool plusReleased = true;
            static bool minusReleased = true;

            // Hold Ctrl and press + to increase window size
            if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS && plusReleased)
            {
                resizeWindow(width * 1.25f, height * 1.25f);
                plusReleased = false;
            }
            else if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_RELEASE)
            {
                plusReleased = true;
            }

            // Hold Ctrl and press - to decrease window size
            if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS && minusReleased)
            {
                resizeWindow(width * 0.75f, height * 0.75f);
                minusReleased = false;
            }
            else if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_RELEASE)
            {
                minusReleased = true;
            }
        }
    }

    // Toggle debug view
    static bool tabReleased = true;
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS && tabReleased)
    {
        renderSettings.showDebugView = !renderSettings.showDebugView;
        updateRenderSettings();
        tabReleased = false;
    }
    else if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_RELEASE)
    {
        tabReleased = true;
    }

    // Toggle UI with F1
    static bool f1Released = true;
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS && f1Released)
    {
        showUI = !showUI;
        uiManager->setShowUI(showUI);
        f1Released = false;
    }
    else if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_RELEASE)
    {
        f1Released = true;
    }

    // Toggle fullscreen with F11
    static bool f11Released = true;
    if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS && f11Released)
    {
        toggleFullscreen();
        f11Released = false;
    }
    else if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_RELEASE)
    {
        f11Released = true;
    }
}

void Application::setCameraAspectRatio(float aspect)
{
    if (camera)
    {
        camera->setAspectRatio(aspect);
    }
}

void Application::setCubeAt(int x, int y, int z, bool active, const glm::vec3& color)
{
    glm::vec3 position = grid->calculatePosition(x, y, z);
    Cube cube(position, color);
    cube.active = active;
    grid->setCube(x, y, z, cube);

    // Notify render integration of grid change
    if (renderIntegration && renderIntegration->getVoxelObject())
    {
        // Mark the affected chunk as dirty for re-rendering
        glm::ivec3 chunkPos(
            std::floor(float(x) / GridChunk::CHUNK_SIZE),
            std::floor(float(y) / GridChunk::CHUNK_SIZE),
            std::floor(float(z) / GridChunk::CHUNK_SIZE)
        );
        renderIntegration->getVoxelObject()->markChunkDirty(chunkPos);
    }
}

void Application::updateRenderSettings()
{
    if (renderIntegration)
    {
        renderIntegration->updateRenderSettings();
    }
}

void Application::setupShadowMap()
{
    // This is now handled by the render integration
    if (renderIntegration)
    {
        updateRenderSettings();
    }
}

bool Application::initializeWindow()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "GLFW initialization failed" << std::endl;
        return false;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Get primary monitor resolution to calculate medium-sized window
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

    // Set window size to approximately 70% of screen size
    width = static_cast<int>(mode->width * 0.7f);
    height = static_cast<int>(mode->height * 0.7f);

    // Create windowed mode window
    window = glfwCreateWindow(width, height, "Isometric Grid", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Window creation failed" << std::endl;
        glfwTerminate();
        return false;
    }

    // Center the window on screen
    int monitorX, monitorY;
    glfwGetMonitorPos(primaryMonitor, &monitorX, &monitorY);
    glfwSetWindowPos(window,
                     monitorX + (mode->width - width) / 2,
                     monitorY + (mode->height - height) / 2);

    // Make context current
    glfwMakeContextCurrent(window);

    // Set callbacks
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    // Set user pointer to this instance for callbacks
    glfwSetWindowUserPointer(window, this);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    // Configure OpenGL
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return true;
}

void Application::updateViewFrustum()
{
    if (!camera) return;

    // Get the view-projection matrix from the camera
    glm::mat4 viewProj = camera->getProjectionMatrix() * camera->getViewMatrix();

    // Update the frustum planes
    viewFrustum.extractFromMatrix(viewProj);
}

std::string Application::generateAutoSaveFilename() const
{
    if (autoSaveFolder.empty())
    {
        return ""; // No folder selected
    }

    // Create a timestamp for the filename
    time_t now = time(nullptr);
    tm localTime;
    localtime_s(&localTime, &now);

    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &localTime);

    // Create the full path
    std::string filename = autoSaveFolder + "\\autosave_" + timestamp + ".bin";
    return filename;
}

void Application::performAutoSave()
{
    if (!enableAutoSave || autoSaveFolder.empty())
    {
        return;
    }

    std::string filename = generateAutoSaveFilename();
    if (filename.empty())
    {
        return;
    }

    if (GridSerializer::saveGridToBinary(grid, filename))
    {
        uiManager->addNotification("Auto-saved world");
        lastAutoSaveTime = glfwGetTime();
    }
    else
    {
        uiManager->addNotification("Auto-save failed", true);
    }
}

void Application::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Update viewport
    glViewport(0, 0, width, height);

    // Update camera aspect ratio
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app && app->camera)
    {
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        app->camera->setAspectRatio(aspect);

        // Update internal width and height values
        app->width = width;
        app->height = height;

        // Reset viewport to full window size when resizing
        app->viewportWidth = width;
        app->viewportHeight = height;
        app->viewportX = 0;
        app->viewportY = 0;

        // Update the render integration with new size
        if (app->renderIntegration)
        {
            app->renderIntegration->resizeViewport(width, height);
        }
    }
}

void Application::mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Handle mouse movement if needed
}

void Application::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Handle scroll for zoom
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app && app->camera)
    {
        float zoom = yoffset > 0 ? 1.1f : 0.9f;
        app->camera->setZoom(app->camera->getZoom() * zoom);
    }
}

void Application::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

    // Only process clicks if we're in editing mode and ImGui isn't capturing mouse
    ImGuiIO& io = ImGui::GetIO();
    if (button >= 0 && button < 5)
    {
        io.MouseDown[button] = (action == GLFW_PRESS);
    }

    if (!io.WantCaptureMouse && app->isEditing)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        {
            int x, y, z;
            if (app->pickCube(x, y, z))
            {
                // Store selected cube
                app->selectedCubeX = x;
                app->selectedCubeY = y;
                app->selectedCubeZ = z;

                // Update UI manager
                app->uiManager->setSelectedCubeCoords(x, y, z);

                // Add cube based on brush size
                for (int bx = -app->brushSize / 2; bx <= app->brushSize / 2; bx++)
                {
                    for (int by = -app->brushSize / 2; by <= app->brushSize / 2; by++)
                    {
                        for (int bz = -app->brushSize / 2; bz <= app->brushSize / 2; bz++)
                        {
                            app->setCubeAt(x + bx, y + by, z + bz, true, app->selectedCubeColor);
                        }
                    }
                }
            }
        }
        else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        {
            int x, y, z;
            if (app->pickCube(x, y, z))
            {
                // Store selected cube
                app->selectedCubeX = x;
                app->selectedCubeY = y;
                app->selectedCubeZ = z;

                // Update UI manager
                app->uiManager->setSelectedCubeCoords(x, y, z);

                // Remove cube based on brush size
                for (int bx = -app->brushSize / 2; bx <= app->brushSize / 2; bx++)
                {
                    for (int by = -app->brushSize / 2; by <= app->brushSize / 2; by++)
                    {
                        for (int bz = -app->brushSize / 2; bz <= app->brushSize / 2; bz++)
                        {
                            if (y + by > 0) // Don't remove floor
                            {
                                app->setCubeAt(x + bx, y + by, z + bz, false, glm::vec3(0.0f));
                            }
                        }
                    }
                }
            }
        }
    }
}

void Application::errorCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                GLsizei length, const GLchar* message, const void* userParam)
{
    std::cerr << "GL Error: type = 0x" << std::hex << type
        << ", severity = 0x" << std::hex << severity
        << ", message = " << message << std::endl;
}

// //////////////////////////
// ALL OTHER PUBLIC ACCESSORS
// //////////////////////////

void Application::setVisibleCubeCount(int visibleCount)
{
    // Then set to the new count
    visibleCubeCount = visibleCount;

    // Count total active cubes
    int totalActive = grid->getTotalActiveCubeCount();

    cullingStats.totalActiveCubes = totalActive;
    cullingStats.visibleCubes = getVisibleCubeCount();

    if (totalActive >= visibleCount)
    {
        cullingStats.culledCubes = totalActive - visibleCount;
    }
    else
    {
        cullingStats.culledCubes = 0;
    }

    // Calculate culling percentage
    if (totalActive > 0)
    {
        cullingStats.cullingPercentage = 100.0f * (float)cullingStats.culledCubes / (float)totalActive;
    }
    else
    {
        cullingStats.cullingPercentage = 0.0f;
    }

    // Update history for graphs
    float currentTime = static_cast<float>(glfwGetTime());
    if (currentTime - cullingStats.lastUpdateTime >= 0.1f)
    {
        cullingStats.lastUpdateTime = currentTime;

        // Shift values in the history arrays
        for (size_t i = 0; i < cullingStats.fpsHistory.size() - 1; i++)
        {
            cullingStats.fpsHistory[i] = cullingStats.fpsHistory[i + 1];
            cullingStats.cullingHistory[i] = cullingStats.cullingHistory[i + 1];
        }

        // Add new values - cap FPS to something reasonable for the graph
        float currentFps = 1.0f / deltaTime;
        if (currentFps > 1000.0f) currentFps = 1000.0f;

        cullingStats.fpsHistory[cullingStats.fpsHistory.size() - 1] = currentFps;
        cullingStats.cullingHistory[cullingStats.cullingHistory.size() - 1] = cullingStats.cullingPercentage;
    }
}

bool Application::isCubeVisible(int x, int y, int z) const
{
    if (!renderSettings.enableFrustumCulling)
    {
        return true; // Always visible when culling is disabled
    }

    // Let the VoxelObject handle visiblity if it exists
    if (renderIntegration && renderIntegration->getVoxelObject())
    {
        // Get the chunk position
        glm::ivec3 chunkPos(
            std::floor(float(x) / GridChunk::CHUNK_SIZE),
            std::floor(float(y) / GridChunk::CHUNK_SIZE),
            std::floor(float(z) / GridChunk::CHUNK_SIZE)
        );

        // Use the frustum test to determine visibility
        return renderIntegration->getVoxelObject()->isChunkVisible(chunkPos, viewFrustum, camera->getPosition());
    }

    // Fallback to simple position test if no VoxelObject
    const Cube& cube = grid->getCube(x, y, z);
    return viewFrustum.isCubeVisible(cube.position, grid->getSpacing());
}

void Application::resizeWindow(int newWidth, int newHeight)
{
    // Make sure we don't set ridiculous values
    if (newWidth < 320) newWidth = 320;
    if (newHeight < 240) newHeight = 240;

    // Update internal width and height
    width = newWidth;
    height = newHeight;

    // Resize the GLFW window
    glfwSetWindowSize(window, width, height);

    // Center the window on the screen (optional)
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (monitor)
    {
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        int monitorX, monitorY;
        glfwGetMonitorPos(monitor, &monitorX, &monitorY);
        int windowX = monitorX + (mode->width - width) / 2;
        int windowY = monitorY + (mode->height - height) / 2;
        glfwSetWindowPos(window, windowX, windowY);
    }

    // Update viewport and camera aspect ratio
    glViewport(0, 0, width, height);

    // Update camera aspect ratio
    if (camera)
    {
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        camera->setAspectRatio(aspect);
    }
}

void Application::resizeWindow(float newWidth, float newHeight)
{
    resizeWindow(static_cast<int>(newWidth), static_cast<int>(newHeight));
}

void Application::toggleFullscreen()
{
    if (isFullscreen)
    {
        // Switch to windowed mode
        const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

        // Calculate medium-sized window
        int newWidth = static_cast<int>(mode->width * 0.7f);
        int newHeight = static_cast<int>(mode->height * 0.7f);

        // Center on screen
        int monitorX, monitorY;
        glfwGetMonitorPos(primaryMonitor, &monitorX, &monitorY);
        int xpos = monitorX + (mode->width - newWidth) / 2;
        int ypos = monitorY + (mode->height - newHeight) / 2;

        // Set window mode
        glfwSetWindowMonitor(window, nullptr, xpos, ypos, newWidth, newHeight, GLFW_DONT_CARE);

        // Update our size tracking
        width = newWidth;
        height = newHeight;
    }
    else
    {
        // Switch to fullscreen mode
        const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
        glfwSetWindowMonitor(window, primaryMonitor, 0, 0, mode->width, mode->height, mode->refreshRate);

        // Update our size tracking
        width = mode->width;
        height = mode->height;
    }

    // Update the fullscreen flag
    isFullscreen = !isFullscreen;

    // Update camera aspect ratio
    if (camera)
    {
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        camera->setAspectRatio(aspect);
    }

    // Update viewport
    glViewport(0, 0, width, height);
}

void Application::setViewportSize(int width, int height)
{
    if (width > 0 && height > 0 && (viewportWidth != width || viewportHeight != height))
    {
        viewportWidth = width;
        viewportHeight = height;
        viewportActive = true;

        // Update render integration viewport
        renderIntegration->resizeViewport(width, height);

        // Update camera aspect ratio
        if (camera)
        {
            float aspect = static_cast<float>(width) / static_cast<float>(height);
            camera->setAspectRatio(aspect);
        }
    }
}

void Application::setViewportPos(int x, int y)
{
    viewportX = x;
    viewportY = y;
}

bool Application::pickCube(int& outX, int& outY, int& outZ)
{
    // Get mouse position
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    // Convert mouse position to normalized device coordinates
    float ndcX = (2.0f * static_cast<float>(mouseX)) / width - 1.0f;
    float ndcY = 1.0f - (2.0f * static_cast<float>(mouseY)) / height;

    // For orthographic projection, we need to create a ray that's parallel to view direction
    // Get the camera's view and projection matricies
    glm::mat4 viewMatrix = camera->getViewMatrix();
    glm::mat4 projMatrix = camera->getProjectionMatrix();
    glm::mat4 invViewProj = glm::inverse(projMatrix * viewMatrix);

    // Create two points in NDC space at different depths
    glm::vec4 nearPoint = glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 farPoint = glm::vec4(ndcX, ndcY, 1.0f, 1.0f);

    // Transform to world space
    glm::vec4 worldNearPoint = invViewProj * nearPoint;
    glm::vec4 worldFarPoint = invViewProj * farPoint;

    // Perform perspective division if w is not 1.0
    if (abs(worldNearPoint.w) > 0.0001f) worldNearPoint /= worldNearPoint.w;
    if (abs(worldFarPoint.w) > 0.0001f) worldFarPoint /= worldFarPoint.w;

    // Calculate ray origin and direction
    glm::vec3 rayOrigin = glm::vec3(worldNearPoint);
    glm::vec3 rayDir = glm::normalize(glm::vec3(worldFarPoint) - rayOrigin);

    // Debug output
    std::cout << "Ray origin: " << rayOrigin.x << ", " << rayOrigin.y << ", " << rayOrigin.z << std::endl;
    std::cout << "Ray direction: " << rayDir.x << ", " << rayDir.y << ", " << rayDir.z << std::endl;

    // Grid parameters
    float spacing = grid->getSpacing();

    // Get current grid bounds
    glm::ivec3 minBounds = grid->getMinBounds();
    glm::ivec3 maxBounds = grid->getMaxBounds();

    // Grid bounds (assuming grid is centered at origin)
    glm::vec3 gridMin = grid->calculatePosition(minBounds.x - 10, minBounds.y - 10, minBounds.z - 10);
    glm::vec3 gridMax = grid->calculatePosition(maxBounds.x + 10, maxBounds.y + 10, maxBounds.z + 10);

    // Check ray interaction with grid bounds using slab method
    float tMin = -INFINITY;
    float tMax = INFINITY;

    // For each axis (X, Y, Z)
    for (int i = 0; i < 3; i++)
    {
        if (abs(rayDir[i]) < 0.0001f)
        {
            // Ray is parallel to the slab. Check if ray origin is within the slab
            if (rayOrigin[i] < gridMin[i] || rayOrigin[i] > gridMax[i])
            {
                // Ray doesn't intersect grid bounds
                return false;
            }
        }
        else
        {
            // Calculate intersection distances
            float t1 = (gridMin[i] - rayOrigin[i]) / rayDir[i];
            float t2 = (gridMax[i] - rayOrigin[i]) / rayDir[i];

            // Ensure t1 <= t2
            if (t1 > t2) std::swap(t1, t2);

            // Update tMin and tMax
            tMin = max(tMin, t1);
            tMax = min(tMax, t2);

            // Early termination
            if (tMin > tMax) return false;
        }
    }

    // If we get here, ray intersects grid bounds
    // Start traversal at intersection point with grid bounds
    glm::vec3 intersectionPoint = rayOrigin + tMin * rayDir;

    // Voxel traversal setup
    // Convert world coordinates to grid indices
    glm::ivec3 gridPos = grid->worldToGridCoordinates(intersectionPoint);
    int voxelX = gridPos.x;
    int voxelY = gridPos.y;
    int voxelZ = gridPos.z;

    // Direction to increment grid coordinates
    int stepX = (rayDir.x > 0) ? 1 : ((rayDir.x < 0) ? -1 : 0);
    int stepY = (rayDir.y > 0) ? 1 : ((rayDir.y < 0) ? -1 : 0);
    int stepZ = (rayDir.z > 0) ? 1 : ((rayDir.z < 0) ? -1 : 0);

    // Distance to next voxel boundary for each axis
    float nextVoxelBoundaryX = (stepX > 0) ? (voxelX + 1) * spacing : voxelX * spacing;
    float nextVoxelBoundaryY = (stepY > 0) ? (voxelY + 1) * spacing : voxelY * spacing;
    float nextVoxelBoundaryZ = (stepZ > 0) ? (voxelZ + 1) * spacing : voxelZ * spacing;

    // tMax is how far we need to travel in ray direction to reach the next voxel boundary
    float tMaxX = (rayDir.x != 0) ? (nextVoxelBoundaryX - rayOrigin.x) / rayDir.x : INFINITY;
    float tMaxY = (rayDir.y != 0) ? (nextVoxelBoundaryY - rayOrigin.y) / rayDir.y : INFINITY;
    float tMaxZ = (rayDir.z != 0) ? (nextVoxelBoundaryZ - rayOrigin.z) / rayDir.z : INFINITY;

    // tDelta is how far we need to travel in ray direction to cross one voxel
    float tDeltaX = (rayDir.x != 0) ? spacing / std::abs(rayDir.x) : INFINITY;
    float tDeltaY = (rayDir.y != 0) ? spacing / std::abs(rayDir.y) : INFINITY;
    float tDeltaZ = (rayDir.z != 0) ? spacing / std::abs(rayDir.z) : INFINITY;

    // Previous cell for placing new cube
    int prevX = voxelX;
    int prevY = voxelY;
    int prevZ = voxelZ;

    // Traverse the grid
    int maxSteps = 1000; // Limit iterations for infinite grid
    for (int i = 0; i < maxSteps; i++)
    {
        // Check if current voxel has a cube
        if (grid->isCubeActive(voxelX, voxelY, voxelZ))
        {
            // Cube found
            outX = voxelX;
            outY = voxelY;
            outZ = voxelZ;
            return true;
        }

        // Store previous position for placing cubes on empty space
        prevX = voxelX;
        prevY = voxelY;
        prevZ = voxelZ;

        // Move to next voxel
        if (tMaxX < tMaxY && tMaxX < tMaxZ)
        {
            // X axis traversal
            tMaxX += tDeltaX;
            voxelX += stepX;
        }
        else if (tMaxY < tMaxZ)
        {
            // Y axis traversal
            tMaxY += tDeltaY;
            voxelY += stepY;
        }
        else
        {
            // Z axis traversal
            tMaxZ += tDeltaZ;
            voxelZ += stepZ;
        }

        // Check if we've gone too far
        float distance = glm::length(rayOrigin - (rayOrigin + tMin * rayDir + glm::vec3(voxelX, voxelY, voxelZ) * spacing));
        if (distance > 1000.0f) // Limit picking distance
        {
            break;
        }
    }

    // If we get here, no cube was found, so we use the last empty position
    outX = prevX;
    outY = prevY;
    outZ = prevZ;

    return false; // No active cube was hit, but we provide an empty position
}

void Application::clearGrid(bool resetFloor)
{
    // Get current bounds to know what to clear
    glm::ivec3 minBounds = grid->getMinBounds();
    glm::ivec3 maxBounds = grid->getMaxBounds();

    // We'll only clear within the current active bounds plus a margin
    for (int x = minBounds.x - 5; x <= maxBounds.x; x++)
    {
        for (int y = minBounds.y; y <= maxBounds.y; y++)
        {
            for (int z = minBounds.z; z <= maxBounds.z; z++)
            {
                // Skip positions that don't have active cubes (for efficiency)
                if (!grid->isCubeActive(x, y, z)) continue;

                if (y > 0) // Above floor
                {
                    // Clear all cubes above the floor
                    setCubeAt(x, y, z, false, glm::vec3(0.0f));
                }
                else if (resetFloor && y == 0) // Floor level and resetFloor is true
                {
                    // Reset floor to default color but keep it active
                    setCubeAt(x, y, z, true, glm::vec3(0.9f, 0.9f, 0.9f));
                }
                // Note: We don't touch the floor cubes if resetFloor is false
            }
        }
    }

    // Notify user
    uiManager->addNotification(resetFloor ? "World reset" : "Grid cleared");
}
