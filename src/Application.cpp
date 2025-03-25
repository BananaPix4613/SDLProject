#include "Application.h"
#include <iostream>

Application::Application(int windowWidth, int windowHeight)
    : width(windowWidth), height(windowHeight),
      window(nullptr), grid(nullptr), renderer(nullptr), camera(nullptr),
      shader(nullptr), shadowShader(nullptr),
      instancedShader(nullptr), instancedShadowShader(nullptr),
      lastFrame(0.0f), deltaTime(0.0f), showUI(true),
      selectedCubeColor(0.5f, 0.5f, 1.0f), isEditing(false), brushSize(1),
      selectedCubeX(-1), selectedCubeY(-1), selectedCubeZ(-1),
      imGui(nullptr), visibleCubeCount(0)
{
    
}

Application::~Application()
{
    delete grid;
    delete renderer;
    delete camera;
    delete shader;
    delete shadowShader;
    delete instancedShader;
    delete instancedShadowShader;
    delete debugRenderer;
    delete imGui;

    glDeleteVertexArrays(1, &debugLineVAO);
    glDeleteBuffers(1, &debugLineVBO);

    // Clean up shadow resources
    glDeleteFramebuffers(1, &depthMapFBO);
    glDeleteTextures(1, &depthMap);

    glfwDestroyWindow(window);
    glfwTerminate();
}

bool Application::initialize()
{
    if (!initializeWindow())
    {
        return false;
    }

    // Create grid with chunk-based system
    grid = new CubeGrid(20, 1.0f);

    // Create camera
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    camera = new IsometricCamera(aspect);

    // Create renderer with support for chunks
    renderer = new CubeRenderer(grid, this);
    renderer->initialize();

    // Load shader
    shader = new Shader("shaders/VertexShader.glsl", "shaders/FragmentShader.glsl");
    shadowShader = new Shader("shaders/ShadowMappingVertexShader.glsl", "shaders/ShadowMappingFragmentShader.glsl");

    // Load instanced versions
    instancedShader = new Shader("shaders/InstancedVertexShader.glsl", "shaders/InstancedFragmentShader.glsl");
    instancedShadowShader = new Shader("shaders/InstancedShadowVertexShader.glsl", "shaders/ShadowMappingFragmentShader.glsl");

    // Setup shadow mapping
    setupShadowMap();

    // Initialize debug renderer
    debugRenderer = new DebugRenderer();
    debugRenderer->initialize();

    // Initialize ImGui
    imGui = new ImGuiWrapper();
    imGui->initialize(window);

    // Generate buffers for debug lines (frustum visualization)
    glGenVertexArrays(1, &debugLineVAO);
    glGenBuffers(1, &debugLineVBO);

    glBindVertexArray(debugLineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, debugLineVBO);

    // We'll update the buffer data when rendering
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // Add mouse button callback for cube picking
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    return true;
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

    // Create window
    window = glfwCreateWindow(width, height, "Isometric Grid", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Window creation failed" << std::endl;
        glfwTerminate();
        return false;
    }

    // Make context current
    glfwMakeContextCurrent(window);

    // Set callbacks
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    //glfwSetCursorPosCallback(window, mouseCallback);
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

void Application::setupShadowMap()
{
    // Use the shadow map resolution from settings
    SHADOW_WIDTH = renderSettings.shadowMapResolution;
    SHADOW_HEIGHT = renderSettings.shadowMapResolution;

    // Delete existing shadow map resources if they exist
    if (depthMapFBO)
    {
        glDeleteFramebuffers(1, &depthMapFBO);
        glDeleteTextures(1, &depthMap);
    }

    // Create a framebuffer object for the depth map
    glGenFramebuffers(1, &depthMapFBO);

    // Create a 2D texture for the depth map
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    // Configure texture filtering
    if (renderSettings.usePCF)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    // Fix shadow acne with proper clamping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Use comparison mode only when needed
    if (renderSettings.usePCF)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    }
    else
    {
        // CRITICAL: Disable comparison mode for standard shadow mapping
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    }

    // Attach the depth texture to the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Error: Shadow map framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Application::run()
{
    while (!glfwWindowShouldClose(window))
    {
        // Poll events first to collect input
        glfwPollEvents();

        // Calculate delta time
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process input
        processInput();

        // Update logic
        update();

        // Start ImGui frame
        imGui->newFrame();

        // Create UI
        if (showUI)
        {
            renderUI();
        }

        // Render
        render();

        // Render ImGui on top
        imGui->render();

        // Swap buffers
        glfwSwapBuffers(window);
    }
}

void Application::setVisibleCubeCount(int visibleCount)
{
    // Store the visible count
    visibleCubeCount = visibleCount;

    // Count total active cubes
    int totalActive = grid->getTotalActiveCubeCount();

    cullingStats.totalActiveCubes = totalActive;
    cullingStats.visibleCubes = visibleCount;
    cullingStats.culledCubes = totalActive - visibleCount;

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

        // Add new values
        cullingStats.fpsHistory[cullingStats.fpsHistory.size() - 1] = 1.0f / deltaTime;
        cullingStats.cullingHistory[cullingStats.cullingHistory.size() - 1] = cullingStats.cullingPercentage;
    }
}

bool Application::isCubeVisible(int x, int y, int z) const
{
    if (!renderSettings.enableFrustumCulling)
    {
        return true; // Always visible when culling is disabled
    }

    // Get the cube's world position
    const Cube& cube = grid->getCube(x, y, z);

    // Use the frustum test to determine visibility
    return viewFrustum.isCubeVisible(cube.position, grid->getSpacing());
}

void Application::processInput()
{
    // Check if ImGui wants to capture input
    ImGuiIO& io = ImGui::GetIO();
    bool imguiWantsCapture = io.WantCaptureMouse || io.WantCaptureKeyboard;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    // Only process game inputs if ImGui isn't capturing
    if (!imguiWantsCapture)
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
        f1Released = false;
    }
    else if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_RELEASE)
    {
        f1Released = true;
    }
}

void Application::update()
{
    // Main grid update
    grid->update(deltaTime);

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

        // Mark renderer cache for update due to chunk changes
        renderer->markCacheForUpdate();

        profiler.endProfile("ChunkUpdate");
    }
}

void Application::render()
{
    profiler.beginFrame();

    // First update the view frustum for culling
    profiler.startProfile("Update Frustum");
    updateViewFrustum();
    profiler.endProfile("Update Frustum");

    // Pre-shadow calculations
    glm::mat4 lightProjection, lightView, lightSpaceMatrix;
    float near_plane = 1.0f, far_plane = 100.0f;

    // Orthographic projection for the light
    float orthoSize = 15.0f; // Adjust based on scene size
    lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, near_plane, far_plane);

    // Light view matrix
    glm::vec3 lightPos = glm::vec3(15.0f, 20.0f, 15.0f);
    glm::vec3 lightTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    lightView = glm::lookAt(lightPos, lightTarget, glm::vec3(0.0f, 1.0f, 0.0f));

    // Light space transformation matrix
    lightSpaceMatrix = lightProjection * lightView;

    // Skip shadow rendering if disabled
    if (renderSettings.enableShadows)
    {
        // 1. First render to depth map (shadow map)
        profiler.startProfile("Shadow Pass");

        // Choose shadow shader based on instancing setting
        Shader* activeShadowShader = renderSettings.useInstancing ? instancedShadowShader : shadowShader;

        // Use the shadow mapping shader
        activeShadowShader->use();
        activeShadowShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

        // Configure viewport to shadow map dimensions
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

        // When rendering to depth map, use polygon offset to reduce shadow acne
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(2.0f, 4.0f);

        // Bind shadow map framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        // Render scene from light's perspective
        renderSceneDepth(*activeShadowShader);

        // Reset state
        glDisable(GL_POLYGON_OFFSET_FILL);
        glBindBuffer(GL_FRAMEBUFFER, 0);

        profiler.endProfile("Shadow Pass");
    }

    // 2. Render scene normally with shadow mapping
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);

    // Format is R, G, B, A where values range from 0.0 to 1.0
    glClearColor(0.678f, 0.847f, 0.902f, 1.0f); // Light blue sky color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    profiler.startProfile("Main Rendering");

    // Choose shader based on instancing setting
    Shader* activeShader = renderSettings.useInstancing ? instancedShader : shader;
    activeShader->use();

    // Set view and projection matrices
    activeShader->setMat4("view", camera->getViewMatrix());
    activeShader->setMat4("projection", camera->getProjectionMatrix());

    // Set light position and view position
    activeShader->setVec3("lightPos", lightPos);
    activeShader->setVec3("lightColor", glm::vec3(15.0f, 15.0f, 15.0f));
    activeShader->setVec3("viewPos", camera->getPosition());

    // Pass render settings to shader
    activeShader->setFloat("ambientStrength", renderSettings.ambientStrength);
    activeShader->setFloat("specularStrength", renderSettings.specularStrength);
    activeShader->setFloat("shininess", renderSettings.shininess);
    activeShader->setBool("usePCF", renderSettings.usePCF);
    activeShader->setFloat("shadowBias", renderSettings.shadowBias);

    // Only bind shadow map if shadows are enabled
    if (renderSettings.enableShadows)
    {
        // Set light space matrix and bind shadow map
        activeShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        activeShader->setInt("shadowMap", 0);
    }
    else
    {
        // When shadows are disabled, make sure nothing is in shadow
        activeShader->setFloat("shadowBias", 1.0f); // Value that ensures no shadows
    }

    // Render the grid
    renderer->render(*activeShader);
    profiler.endProfile("Main Rendering");

    // Render debug visualizations if enabled
    if (renderSettings.showFrustumWireframe)
    {
        renderFrustumDebug();
    }

    // Render chunk boundaries if enabled
    if (debugRenderer && renderSettings.showChunkBoundaries)
    {
        debugRenderer->renderChunkBoundaries(
            camera->getViewMatrix(),
            camera->getProjectionMatrix(),
            grid);
    }

    // Render grid lines if enabled
    if (debugRenderer && renderSettings.showGridLines)
    {
        debugRenderer->renderGridLines(
            camera->getViewMatrix(),
            camera->getProjectionMatrix(),
            grid->getMinBounds(),
            grid->getMaxBounds(),
            grid->getSpacing());
    }

    // 3. Debug visualization if enabled
    if (renderSettings.showDebugView && renderSettings.enableShadows)
    {
        renderDebugView();
    }

    profiler.endFrame();
}

void Application::renderUI()
{
    // Main control panel
    imGui->beginWindow("Control Panel");

    // Window settings
    if (ImGui::CollapsingHeader("Window Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // Display current window size
        ImGui::Text("Current Size: %d x %d", width, height);

        // Presets for common window sizes
        if (ImGui::Button("720p (1280x720)"))
        {
            resizeWindow(1280, 720);
        }
        ImGui::SameLine();
        if (ImGui::Button("1080p (1920x1080)"))
        {
            resizeWindow(1920, 1080);
        }

        if (ImGui::Button("1440p (2560x1440)"))
        {
            resizeWindow(2560, 1440);
        }
        ImGui::SameLine();
        if (ImGui::Button("4K (3840x2160)"))
        {
            resizeWindow(3840, 2160);
        }

        // Custom window size input
        static int customWidth = width;
        static int customHeight = height;
        ImGui::InputInt("Width", &customWidth, 10, 100);
        ImGui::InputInt("Height", &customHeight, 10, 100);

        if (ImGui::Button("Apply Custom Size"))
        {
            // Make sure we don't set ridiculous values
            if (customWidth >= 320 && customWidth <= 7680 &&
                customHeight >= 240 && customHeight <= 4320)
            {
                resizeWindow(customWidth, customHeight);
            }
        }

        // Scale options for high-DPI displays
        float scaleFactors[] = { 0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 2.5f, 3.0f };
        static int currentScaleIndex = 2; // Default to 1.0

        ImGui::Text("Window Scale");
        ImGui::PushItemWidth(200);
        if (ImGui::Combo("##WindowScale", &currentScaleIndex, "50%\0""75%\0""100%\0""125%\0""150%\0""175%\0""200%\0""250%\0""300%\0"))
        {
            float scale = scaleFactors[currentScaleIndex];
            int newWidth = static_cast<int>(1280 * scale);  // Base size of 1280x720
            int newHeight = static_cast<int>(720 * scale);
            resizeWindow(newWidth, newHeight);
        }
        ImGui::PopItemWidth();

        ImGui::Separator();
        ImGui::Text("Hotkeys: Ctrl+Plus to increase size, Ctrl+Minus to decrease");
    }

    // Camera settings
    if (ImGui::CollapsingHeader("Camera Settings"))
    {
        float zoom = camera->getZoom();
        if (ImGui::SliderFloat("Zoom", &zoom, 0.1f, 5.0f))
        {
            camera->setZoom(zoom);
        }

        // Camera position controls could be added here
    }

    // Editing options
    if (ImGui::CollapsingHeader("Editing Tools"))
    {
        ImGui::Checkbox("Editing Mode", &isEditing);

        ImGui::ColorEdit3("Cube Color", &selectedCubeColor[0]);

        ImGui::SliderInt("Brush Size", &brushSize, 1, 5);

        if (selectedCubeX != -1)
        {
            ImGui::Text("Selected Cube: (%d, %d, %d)", selectedCubeX, selectedCubeY, selectedCubeZ);

            if (ImGui::Button("Delete Cube"))
            {
                setCubeAt(selectedCubeX, selectedCubeY, selectedCubeZ, false, glm::vec3(0.0f));
            }
        }
        else
        {
            ImGui::Text("No cube selected.");
        }

        // Tool buttons
        if (ImGui::Button("Clear Grid"))
        {
            clearGrid(false); // Clear grid but don't reset floor
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset All"))
        {
            clearGrid(true); // Clear grid and reset floor to default color
        }
        ImGui::Separator();
    }

    // Add Graphics Settings section
    renderSettingsUI();

    // Debug options
    if (ImGui::CollapsingHeader("Debug Options"))
    {
        ImGui::Checkbox("Show Frustum Wireframe", &renderSettings.showFrustumWireframe);
        ImGui::Checkbox("Show Debug View", &renderSettings.showDebugView);

        const char* debugViews[] = { "Shadow Map Corner", "Full Shadow Map", "Linearized Depth" };
        ImGui::Combo("Debug View Mode", &renderSettings.currentDebugView, debugViews, 3);
    }

    // Performance stats
    if (ImGui::CollapsingHeader("Performance"))
    {
        ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
        ImGui::Text("Frame Time: %.3f ms", deltaTime * 1000.0f);

        ImGui::Checkbox("Enable Frustum Culling", &renderSettings.enableFrustumCulling);

        ImGui::Text("Culling Statistics:");
        ImGui::Text("  Active Cubes %d", cullingStats.totalActiveCubes);
        ImGui::Text("  Visible Cubes: %d", cullingStats.visibleCubes);
        ImGui::Text("  Culled Cubes %d", cullingStats.culledCubes);
        ImGui::Text("  Culling Rate %.1f%%", cullingStats.cullingPercentage);

        // Create a simple FPS graph
        ImGui::PlotLines("FPS", cullingStats.fpsHistory.data(), cullingStats.fpsHistory.size(),
            0, NULL, 0.0f, 120.0f, ImVec2(0, 80));

        // Create a culling percentage graph
        ImGui::PlotLines("Culling %", cullingStats.cullingHistory.data(), cullingStats.cullingHistory.size(),
            0, NULL, 0.0f, 100.0f, ImVec2(0, 80));

        // Force culling percentage to 0 when disabled
        if (!renderSettings.enableFrustumCulling)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
                "Culling is disabled! Enable for performance boost.");
        }
    }

    imGui->endWindow();

    // File operations window
    imGui->beginWindow("File Operations");

    static char saveFilename[256] = "world.grid";
    ImGui::InputText("Filename", saveFilename, 256);

    if (ImGui::Button("Save Grid"))
    {
        if (GridSerializer::saveGridToFile(grid, saveFilename))
        {
            // Success notification
            showNotification("Grid saved successfully");
        }
        else
        {
            // Error notification
            showNotification("Failed to save grid", true);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Load Grid"))
    {
        if (GridSerializer::loadGridFromFile(grid, saveFilename))
        {
            // Success notification
            showNotification("Grid loaded successfully");
        }
        else
        {
            // Error notification
            showNotification("Failed to load grid", true);
        }
    }

    ImGui::Separator();

    // Option to use binary format
    static bool useBinaryFormat = false;
    ImGui::Checkbox("Use Binary Format", &useBinaryFormat);

    if (ImGui::Button("Export Binary"))
    {
        std::string binFilename = std::string(saveFilename) + ".bin";
        if (GridSerializer::saveGridToBinary(grid, binFilename))
        {
            showNotification("Grid exported as binary");
        }
        else
        {
            showNotification("Failed to export binary grid", true);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Import Binary"))
    {
        std::string binFilename = std::string(saveFilename) + ".bin";
        if (GridSerializer::loadGridFromBinary(grid, binFilename))
        {
            showNotification("Binary grid imported");
        }
        else
        {
            showNotification("Failed to import binary grid", true);
        }
    }

    imGui->endWindow();

    // Grid navigation window
    imGui->beginWindow("Grid Navigation");

    // Show current bounds
    glm::ivec3 minBounds = grid->getMinBounds();
    glm::ivec3 maxBounds = grid->getMaxBounds();

    ImGui::Text("Grid Bounds: (%d, %d, %d) to (%d, %d, %d)",
        minBounds.x, minBounds.y, minBounds.z,
        maxBounds.x, maxBounds.y, maxBounds.z);

    ImGui::Separator();

    // Navigation to specific coordinates
    static int targetX = 0, targetY = 0, targetZ = 0;
    ImGui::InputInt("Target X", &targetX);
    ImGui::InputInt("Target Y", &targetY);
    ImGui::InputInt("Target Z", &targetZ);

    if (ImGui::Button("Go To Target"))
    {
        // Calculate world coordinates
        glm::vec3 worldPos = grid->calculatePosition(targetX, targetY, targetZ);

        // Set camera target to that location
        camera->setTarget(worldPos);

        // Check if we're in edit mode
        if (isEditing)
        {
            // Select cube at that location (or nearby empty spot)
            if (grid->isCubeActive(targetX, targetY, targetZ))
            {
                selectedCubeX = targetX;
                selectedCubeY = targetY;
                selectedCubeZ = targetZ;
            }
        }
    }

    ImGui::Separator();

    // Chunk loading settings
    static int viewDistance = 8;
    ImGui::SliderInt("View Distance (chunks)", &viewDistance, 2, 16);

    if (ImGui::Button("Update Loaded Chunks"))
    {
        // Calculate camera position in grid coordinates
        glm::vec3 camWorldPos = camera->getPosition();
        glm::ivec3 camGridPos = grid->worldToGridCoordinates(camWorldPos);

        // Update which chunks are loaded
        grid->updateLoadedChunks(camGridPos, viewDistance);
    }

    ImGui::Separator();

    // Statistics
    ImGui::Text("Active Chunks: %d", grid->getActiveChunkCount());
    ImGui::Text("Total Cubes: %d", grid->getTotalActiveCubeCount());

    imGui->endWindow();

    // Add notification system UI (auto-hiding messages)
    renderNotifications();

    // Profiler Window
    profiler.drawImGuiWindow();
}

void Application::renderSettingsUI()
{
    if (ImGui::CollapsingHeader("Graphics Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // Quality presets
        const char* presets[] = { "Low", "Medium", "High", "Ultra" };
        static int currentPreset = 1; // Medium by default

        if (ImGui::Combo("Quality Preset", &currentPreset, presets, 4))
        {
            renderSettings.applyPreset(static_cast<RenderSettings::QualityPreset>(currentPreset));
            // Recreate shadow map if resolution changed
            setupShadowMap();
        }

        // Individual settings
        if (ImGui::TreeNode("Shadow Settings"))
        {
            bool shadowsChanged = false;

            shadowsChanged |= ImGui::Checkbox("Enable Shadows", &renderSettings.enableShadows);

            const char* resolutions[] = { "512x512", "1024x1024", "2048x2048", "4096x4096" };
            int currentRes = 0;

            if (renderSettings.shadowMapResolution == 512) currentRes = 0;
            else if (renderSettings.shadowMapResolution == 1024) currentRes = 1;
            else if (renderSettings.shadowMapResolution == 2048) currentRes = 2;
            else if (renderSettings.shadowMapResolution == 4096) currentRes = 3;

            if (ImGui::Combo("Shadow Resolution", &currentRes, resolutions, 4))
            {
                switch (currentRes)
                {
                case 0: renderSettings.shadowMapResolution = 512; break;
                case 1: renderSettings.shadowMapResolution = 1024; break;
                case 2: renderSettings.shadowMapResolution = 2048; break;
                case 3: renderSettings.shadowMapResolution = 4096; break;
                }
                shadowsChanged = true;
            }

            shadowsChanged |= ImGui::Checkbox("Use PCF Filtering", &renderSettings.usePCF);

            // Make the bias slider more precise for debugging
            ImGui::SliderFloat("Shadow Bias", &renderSettings.shadowBias, 0.0001f, 0.01f, "%.5f");

            if (shadowsChanged)
            {
                // Recreate shadow map if settings changed
                setupShadowMap();
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Lighting Settings"))
        {
            ImGui::SliderFloat("Ambient Strength", &renderSettings.ambientStrength, 0.0f, 1.0f);
            ImGui::SliderFloat("Specular Strength", &renderSettings.specularStrength, 0.0f, 1.0f);
            ImGui::SliderFloat("Shininess", &renderSettings.shininess, 1.0f, 128.0f);
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Performance Settings"))
        {
            ImGui::Checkbox("Enable Frustum Culling", &renderSettings.enableFrustumCulling);
            ImGui::Checkbox("Use Instanced Rendering", &renderSettings.useInstancing);

            bool vsyncChanged = ImGui::Checkbox("VSync", &renderSettings.enableVSync);
            if (vsyncChanged)
            {
                glfwSwapInterval(renderSettings.enableVSync ? 1 : 0);
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Advanced Visualization"))
        {
            bool settingsChanged = false;

            // Chunk visualization
            if (ImGui::TreeNode("Chunk System"))
            {
                ImGui::Text("Active Chunks: %d", grid->getActiveChunkCount());
                ImGui::Text("Total Cubes: %d", grid->getTotalActiveCubeCount());

                if (ImGui::Checkbox("Show Chunk Boundaries", &renderSettings.showChunkBoundaries))
                {
                    settingsChanged = true;
                    debugRenderer->setShowChunkBoundaries(renderSettings.showChunkBoundaries);
                }

                if (ImGui::SliderInt("Chunk View Distance", &chunkViewDistance, 1, 16))
                {
                    settingsChanged = true;
                    grid->updateLoadedChunks(
                        grid->worldToGridCoordinates(camera->getPosition()),
                        chunkViewDistance);
                }

                if (ImGui::SliderFloat("Max View Distance", &maxViewDistance, 100.0f, 2000.0f))
                {
                    settingsChanged = true;
                }

                if (ImGui::Button("Update Loaded Chunks"))
                {
                    grid->updateLoadedChunks(
                        grid->worldToGridCoordinates(camera->getPosition()),
                        chunkViewDistance);
                }

                ImGui::TreePop();
            }

            // Rendering optimization
            if (ImGui::TreeNode("Rendering Optimization"))
            {
                if (ImGui::Checkbox("Use Instance Cache", &useInstanceCache))
                {
                    settingsChanged = true;
                }

                if (ImGui::Checkbox("Per-Cube Culling", &perCubeCulling))
                {
                    settingsChanged = true;
                }

                if (ImGui::SliderInt("Batch Size", &batchSize, 1000, 50000))
                {
                    settingsChanged = true;
                }

                if (ImGui::Button("Update Render Cache"))
                {
                    renderer->updateChunkInstanceCache();
                }

                ImGui::TreePop();
            }

            // Apply settings changes
            if (settingsChanged)
            {
                // Get values from UI widgets
                float viewDist = maxViewDistance;
                bool useCache = useInstanceCache;
                bool cubeCulling = perCubeCulling;
                int batch = batchSize;

                // Update renderer settings
                renderer->setRenderSettings(useCache, cubeCulling, viewDist, batch);
                renderer->markCacheForUpdate();
            }

            ImGui::TreePop();
        }

        // LOD Settings
        if (ImGui::TreeNode("Level of Detail"))
        {
            ImGui::Checkbox("Enable LOD", &renderSettings.enableLOD);

            // Only show these controls if LOD is enabled
            if (renderSettings.enableLOD)
            {
                static float lodDistanceMedium = 200.0f;
                static float lodDistanceLow = 500.0f;

                ImGui::SliderFloat("Medium Detail Distance", &lodDistanceMedium, 50.0f, 1000.0f);
                ImGui::SliderFloat("Low Detail Distance", &lodDistanceLow, lodDistanceMedium + 50.0f, 2000.0f);

                ImGui::Text("High Detail: 0-%.0f units", lodDistanceMedium);
                ImGui::Text("Medium Detail: %.0f-%.0f units", lodDistanceMedium, lodDistanceLow);
                ImGui::Text("Low Detail: %.0f+ units", lodDistanceLow);

                // Preview Colors
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "High Detail");
                ImGui::SameLine(150);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Medium Detail");
                ImGui::SameLine(300);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Low Detail");

                if (ImGui::Button("Apply LOD Settings"))
                {
                    // Apply LOD settings to the renderer
                    // Implementation would depend on your LOD system desin
                    if (renderer)
                    {
                        // This would be added to renderer class
                        // renderer-setLodDistances(lodDistanceMedium, lodDistanceLow, enableLOD);
                    }
                }
            }

            ImGui::TreePop();
        }

        // Debug visualization
        if (ImGui::TreeNode("Visualization Debug"))
        {
            if (ImGui::Checkbox("Show Bounding Boxes", &renderSettings.showBoundingBoxes))
            {
                debugRenderer->setShowBoundingBoxes(renderSettings.showBoundingBoxes);
            }

            if (ImGui::Checkbox("Show Grid Lines", &renderSettings.showGridLines))
            {
                debugRenderer->setShowGridLines(renderSettings.showGridLines);
            }

            if (ImGui::Checkbox("Color by Distance", &renderSettings.colorByDistance))
            {
                // This would enable a debug shader that colors cubes based on distance from camera
                // Could be implemented in the renderer class
                // renderer->setDebugColorMode(colorByDistance ? 1 : 0);
            }

            if (ImGui::Checkbox("Show Chunk Stats", &renderSettings.showChunkStats))
            {
                renderSettings.showChunkStats = renderSettings.showChunkStats;
            }

            if (renderSettings.showChunkStats)
            {
                // Display additional statistics about chunks if enabled
                ImGui::Text("Chunk Update Time: %.3f ms", profiler.getLastTime("ChunkUpdate") * 1000.0f);
                ImGui::Text("Chunk Culling Time: %.3f ms", profiler.getLastTime("ChunkCulling") * 1000.0f);
                ImGui::Text("Chunk Mesh Updates: %d", renderer->getChunkUpdatesThisFrame());
            }

            ImGui::TreePop();
        }

        // World settings
        if (ImGui::TreeNode("World Settings"))
        {
            static int worldSize = 20;
            ImGui::SliderInt("Initial World Size", &worldSize, 10, 100);

            static float cubeSpacing = 1.0f;
            if (ImGui::SliderFloat("Cube Spacing", &cubeSpacing, 0.5f, 2.0f))
            {
                // This would update the spacing in the grid if implemented
                // grid->setSpacing(cubeSpacing);
            }

            static int worldHeight = 128;
            ImGui::SliderInt("World Height Limit", &worldHeight, 64, 512);

            if (ImGui::Button("Reset World"))
            {
                ImGui::OpenPopup("Reset World?");
            }

            // Confirmation popup
            if (ImGui::BeginPopupModal("Reset World?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("This will delete the entire world and create a new one.\nThis operation cannot be undone!\n\n");
                ImGui::Separator();

                if (ImGui::Button("OK", ImVec2(120, 0)))
                {
                    // Reset world with new settings
                    delete grid;
                    grid = new CubeGrid(worldSize, cubeSpacing);
                    renderer->markCacheForUpdate();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0)))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Debug Settings"))
        {
            ImGui::Checkbox("Show Debug View", &renderSettings.showDebugView);
            ImGui::Checkbox("Show Frustum Wireframe", &renderSettings.showFrustumWireframe);
            ImGui::Checkbox("Show Performance Overlay", &renderSettings.showPerformanceOverlay);

            const char* debugViews[] = { "Shadow Map Corner", "Full Shadow Map", "Linearized Depth" };
            ImGui::Combo("Debug View Mode", &renderSettings.currentDebugView, debugViews, 3);

            ImGui::TreePop();
        }
    }
}

void Application::renderSceneDepth(Shader& depthShader)
{
    // Similar to render, but only pass model matrices for depth rendering
    renderer->renderDepthOnly(depthShader);
}

void Application::renderFrustumDebug()
{
    if (!renderSettings.showFrustumWireframe) return;

    // Create simple shader for lines if needed
    static Shader* lineShader = nullptr;
    if (!lineShader)
    {
        lineShader = new Shader("shaders/LineVertexShader.glsl", "shaders/LineFragmentShader.glsl");
    }

    // Get frustum corners
    std::vector<glm::vec3> corners = getFrustumCorners();

    // Define line segments for frustum edges
    std::vector<glm::vec3> lines = {
        // Near face
        corners[0], corners[1],
        corners[1], corners[3],
        corners[3], corners[2],
        corners[2], corners[0],

        // Far face
        corners[4], corners[5],
        corners[5], corners[7],
        corners[7], corners[6],
        corners[6], corners[4],

        // Connecting edges
        corners[0], corners[4],
        corners[1], corners[5],
        corners[2], corners[6],
        corners[3], corners[7]
    };

    // Update VBO with line data
    glBindBuffer(GL_ARRAY_BUFFER, debugLineVBO);
    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(glm::vec3), lines.data(), GL_DYNAMIC_DRAW);

    // Draw lines
    lineShader->use();
    lineShader->setMat4("view", camera->getViewMatrix());
    lineShader->setMat4("projection", camera->getProjectionMatrix());
    lineShader->setVec3("color", glm::vec3(1.0f, 0.0f, 0.0f)); // Red frustum wireframe

    glBindVertexArray(debugLineVAO);
    glLineWidth(2.0f); // Thicker lines for better visibility
    glDrawArrays(GL_LINES, 0, lines.size());
    glLineWidth(1.0f); // Reset line width

    glBindVertexArray(0);
}

void Application::renderDebugView()
{
    // Reset viewport to full screen for main rendering
    glViewport(0, 0, width, height);

    switch (renderSettings.currentDebugView)
    {
    case 0: // Depth map in corner of screen
        debugRenderer->renderDebugTexture(depthMap,
            width - SHADOW_WIDTH / 4,
            height - SHADOW_HEIGHT / 4,
            SHADOW_WIDTH / 4,
            SHADOW_HEIGHT / 4,
            true);
        break;

    case 1: // Full screen depth map
        debugRenderer->renderDebugTexture(depthMap,
            0, 0,
            width, height,
            true);
        break;

    case 2: // Full screen depth map with linear depth output
        // Special visualization with depth linearization
        // This would need a custom shader to properly visualize depth
        debugRenderer->renderDebugTexture(depthMap,
            0, 0,
            width, height,
            true);
        // Add text overlay explaining the view
        // (Would need additional text rendering functionality)
        break;
    }
}

void Application::renderNotifications()
{
    if (notifications.empty()) return;

    float PADDING = 10.0f;
    ImVec2 windowPos = ImVec2(width - 300.0f - PADDING, PADDING);
    ImGui::SetNextWindowPos(windowPos);
    ImGui::SetNextWindowSize(ImVec2(300, 0));
    ImGui::SetNextWindowBgAlpha(0.8f);

    if (ImGui::Begin("##Notifications", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar))
    {
        for (auto& notification : notifications)
        {
            if (notification.isError)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
            }

            ImGui::TextWrapped("%s", notification.message.c_str());
            ImGui::PopStyleColor();
        }
    }
    ImGui::End();

    // Update notification timers
    float currentTime = static_cast<float>(glfwGetTime());
    static float lastTime = currentTime;
    float deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    for (auto it = notifications.begin(); it != notifications.end();)
    {
        it->timeRemaining -= deltaTime;
        if (it->timeRemaining <= 0.0f)
        {
            it = notifications.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void Application::updateViewFrustum()
{
    if (!camera) return;

    // Get the view-projection matrix from the camera
    glm::mat4 viewProj = camera->getProjectionMatrix() * camera->getViewMatrix();

    // Update the frustum planes
    viewFrustum.extractFromMatrix(viewProj);
}

std::vector<glm::vec3> Application::getFrustumCorners() const
{
    std::vector<glm::vec3> corners(8);

    // Get the inverse of the view-projection matrix
    glm::mat4 viewProj = camera->getProjectionMatrix() * camera->getViewMatrix();
    glm::mat4 invViewProj = glm::inverse(viewProj);

    // Define corners in NDC space (-1 to 1 cube)
    // Near face (CCW when looking at the near face from inside the frustum)
    glm::vec4 ntl(-1.0f, 1.0f, -1.0f, 1.0f);  // Near top left
    glm::vec4 ntr(1.0f, 1.0f, -1.0f, 1.0f);   // Near top right
    glm::vec4 nbl(-1.0f, -1.0f, -1.0f, 1.0f); // Near bottom left
    glm::vec4 nbr(1.0f, -1.0f, -1.0f, 1.0f);  // Near bottom right

    // Far face (CCW when looking at the far face from inside the frustum)
    glm::vec4 ftl(-1.0f, 1.0f, 1.0f, 1.0f);   // Far top left
    glm::vec4 ftr(1.0f, 1.0f, 1.0f, 1.0f);    // Far top right
    glm::vec4 fbl(-1.0f, -1.0f, 1.0f, 1.0f);  // Far bottom left
    glm::vec4 fbr(1.0f, -1.0f, 1.0f, 1.0f);   // Far bottom right

    // Transform all points to world space
    std::vector<glm::vec4> ndc = { ntl, ntr, nbl, nbr, ftl, ftr, fbl, fbr };

    for (int i = 0; i < 8; i++)
    {
        // Apply inverse view-projection to get world space corners
        glm::vec4 worldPos = invViewProj * ndc[i];
        worldPos /= worldPos.w; // Perspective divide
        corners[i] = glm::vec3(worldPos);
    }

    return corners;
}

void Application::showNotification(const std::string& message, bool isError)
{
    notifications.emplace_back(message, isError);
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

void Application::setCubeAt(int x, int y, int z, bool active, const glm::vec3& color)
{
    glm::vec3 position = grid->calculatePosition(x, y, z);
    Cube cube(position, color);
    cube.active = active;
    grid->setCube(x, y, z, cube);

    // Mark the renderer cache as needing update due to grid changes
    renderer->markCacheForUpdate();
}

bool Application::pickCube(int& outX, int& outY, int& outZ)
{
    // Get mouse position
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    // Convert mouse position to normalized device coordinates
    float ndcX = (2.0f * mouseX) / width - 1.0f;
    float ndcY = 1.0f - (2.0f * mouseY) / height;

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
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);

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

    // Mark renderer cache for update
    renderer->markCacheForUpdate();
}

void Application::framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    // Update viewport
    glViewport(0, 0, width, height);

    // Update camera aspect ratio
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app && app->camera)
    {
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        app->camera->setAspectRatio(aspect);
    }
}

void Application::mouseCallback(GLFWwindow *window, double xpos, double ypos)
{
    // Handle mouse movement if needed
}

void Application::scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
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
    if (button >= 0 && button < 5) {
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
                                GLsizei length, const GLchar *message, const void *userParam)
{
    std::cerr << "GL Error: type = 0x" << std::hex << type
              << ", severity = 0x" << std::hex << severity
              << ", message = " << message << std::endl;
}
