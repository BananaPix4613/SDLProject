#include "Application.h"
#include "UIManager.h"
#include <iostream>

Application::Application(int windowWidth, int windowHeight)
    : width(windowWidth), height(windowHeight),
      window(nullptr), grid(nullptr), renderer(nullptr), camera(nullptr),
      shader(nullptr), shadowShader(nullptr),
      instancedShader(nullptr), instancedShadowShader(nullptr),
      lastFrame(0.0f), deltaTime(0.0f), showUI(true),
      selectedCubeColor(0.5f, 0.5f, 1.0f), isEditing(false), brushSize(1),
      selectedCubeX(-1), selectedCubeY(-1), selectedCubeZ(-1),
      visibleCubeCount(0),enableAutoSave(false), autoSaveInterval(5),
      autoSaveFolder(""), lastAutoSaveTime(0.0)
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
    delete uiManager;

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

    // Set up DPI scaling for high-resolution displays
    float xscale, yscale;
    glfwGetWindowContentScale(window, &xscale, &yscale);
    float dpiScale = xscale > yscale ? xscale : yscale;

    // Initialize framebuffer
    setupFramebuffer();

    // Create grid with chunk-based system
    grid = new CubeGrid(20, 1.0f);

    // In Application::initialize() after the CubeGrid is created
    std::cout << "Initial active cubes: " << grid->getTotalActiveCubeCount() << std::endl;
    std::cout << "Initial grid bounds: ("
        << grid->getMinBounds().x << "," << grid->getMinBounds().y << "," << grid->getMinBounds().z
        << ") to ("
        << grid->getMaxBounds().x << "," << grid->getMaxBounds().y << "," << grid->getMaxBounds().z
        << ")" << std::endl;


    // Create camera
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    camera = new Camera(aspect);

    float distance = 50.0f; // Distance from origin
    float angle = 45.0f * (3.14159f / 180.0f); // 45 degrees in radians

    // Calculate isometric camera position at a fixed height
    glm::vec3 cameraPos(
        distance * cos(angle),
        distance * 0.5f,        // Fixed height 
        distance * sin(angle)
    );

    // Set camera to look at the origin
    camera->setPosition(cameraPos);
    camera->setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    camera->setZoom(1.0f);

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

    // Initialize UI Manager
    uiManager = new UIManager(this);
    uiManager->setDpiScale(dpiScale);
    if (!uiManager->initialize(window))
    {
        std::cerr << "Failed to initialize UI Manager" << std::endl;
        return false;
    }

    // Generate buffers for debug lines (frustum visualization)
    glGenVertexArrays(1, &debugLineVAO);
    glGenBuffers(1, &debugLineVBO);

    glBindVertexArray(debugLineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, debugLineVBO);

    // Position attribute (3 floats, stride is 6 floats, no offset)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute (3 floats, stride is 6 floats, offset 3 floats)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

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

    // Initialize viewport properties
    viewportWidth = width;
    viewportHeight = height;
    viewportX = 0;
    viewportY = 0;
    viewportActive = true;

    // Store monitor info for fullscreen toggle
    this->primaryMonitor = primaryMonitor;
    this->isFullscreen = false;

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
        uiManager->beginFrame();

        // 2. Render 3D scene to framebuffer
        renderSceneToFrameBuffer();

        // 3. Render main framebuffer to screen
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 4. Render UI with the scene texture in the viewport
        renderUI();

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
}

void Application::setupFramebuffer()
{
    // Create framebuffer
    glGenFramebuffers(1, &sceneFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFramebuffer);

    // Create color attachment texture
    glGenTextures(1, &sceneColorTexture);
    glBindTexture(GL_TEXTURE_2D, sceneColorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneColorTexture, 0);

    // Create depth texture
    glGenTextures(1, &sceneDepthTexture);
    glBindTexture(GL_TEXTURE_2D, sceneDepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sceneDepthTexture, 0);

    // Check framebuffer status
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "ERROR: Framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

bool errorYet = false;
void Application::renderSceneToFrameBuffer()
{
    // Bind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFramebuffer);
    glViewport(0, 0, viewportWidth, viewportHeight);

    //// Set the viewport to match the panel exactly
    //glViewport(viewportX, viewportY, viewportWidth, viewportHeight);

    // Enable depth testing and face culling for 3D rendering
    //glEnable(GL_DEPTH_TEST);
    // In renderSceneToFrameBuffer(), before rendering:
    //glDisable(GL_DEPTH_TEST);  // Temporarily disable depth testing
    // OR
    //glDepthFunc(GL_ALWAYS);    // Always pass depth test
    //glDisable(GL_CULL_FACE);
    //glDepthFunc(GL_LESS); // Use the most common depth function

    // In renderSceneToFrameBuffer():
    //glCullFace(GL_BACK);  // Default is GL_BACK
    //glFrontFace(GL_CCW);  // Try both GL_CCW and GL_CW

    // Clear with sky blue color
    glClearColor(0.678f, 0.847f, 0.902f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Check that camera, grid, and renderer are valid
    if (!camera || !grid || !renderer)
    {
        std::cerr << "Scene components not initialized!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    

    // Temporarily disable frustum culling
    bool originalCullingState = renderSettings.enableFrustumCulling;
    renderSettings.enableFrustumCulling = false;

    //renderCubesDirectly();

    // Update view frustum
    updateViewFrustum();

    // Shadow mapping
    if (renderSettings.enableShadows)
    {
        // Set up the light space matrices for shadows
        glm::mat4 lightProjection, lightView, lightSpaceMatrix;
        float near_plane = 1.0f, far_plane = 100.0f;
        float orthoSize = 15.0f;

        lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, near_plane, far_plane);
        glm::vec3 lightPos = glm::vec3(15.0f, 20.0f, 15.0f);
        glm::vec3 lightTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        lightView = glm::lookAt(lightPos, lightTarget, glm::vec3(0.0f, 1.0f, 0.0f));
        lightSpaceMatrix = lightProjection * lightView;

        // Choose shadow shader
        Shader* activeShadowShader = renderSettings.useInstancing ? instancedShadowShader : shadowShader;

        // Render shadow map
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(2.0f, 4.0f);

        activeShadowShader->use();
        activeShadowShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

        // Render scene from light's perspective
        renderer->render(*activeShadowShader);

        glDisable(GL_POLYGON_OFFSET_FILL);

        // Restore framebuffer and viewport
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
    }

    // Main rendering pass
    Shader* activeShader = renderSettings.useInstancing ? instancedShader : shader;
    activeShader->use();

    // Set matrices
    activeShader->setMat4("view", camera->getViewMatrix());
    activeShader->setMat4("projection", camera->getProjectionMatrix());

    // Set lighting parameters
    glm::vec3 lightPos = glm::vec3(15.0f, 20.0f, 15.0f);
    activeShader->setVec3("lightPos", lightPos);
    activeShader->setVec3("lightColor", glm::vec3(15.0f, 15.0f, 15.0f));
    activeShader->setVec3("viewPos", camera->getPosition());

    // Apply render settings
    activeShader->setFloat("ambientStrength", renderSettings.ambientStrength);
    activeShader->setFloat("specularStrength", renderSettings.specularStrength);
    activeShader->setFloat("shininess", renderSettings.shininess);

    // Shadow settings
    if (renderSettings.enableShadows)
    {
        glm::mat4 lightProjection, lightView, lightSpaceMatrix;
        float near_plane = 1.0f, far_plane = 100.0f;
        float orthoSize = 15.0f;

        lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, near_plane, far_plane);
        glm::vec3 lightPos = glm::vec3(15.0f, 20.0f, 15.0f);
        glm::vec3 lightTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        lightView = glm::lookAt(lightPos, lightTarget, glm::vec3(0.0f, 1.0f, 0.0f));
        lightSpaceMatrix = lightProjection * lightView;

        activeShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
        activeShader->setBool("usePCF", renderSettings.usePCF);
        activeShader->setFloat("shadowBias", renderSettings.shadowBias);

        // Bind shadow map texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        activeShader->setInt("shadowMap", 0);
    }
    else
    {
        // Ensure nothing is in shadow
        activeShader->setFloat("shadowBias", 1.0f);
    }
    
    // Render the cubes
    renderer->render(*activeShader);

    // Debug visualizations
    if (renderSettings.showFrustumWireframe)
    {
        renderFrustumDebug();
    }

    if (debugRenderer)
    {
        if (renderSettings.showChunkBoundaries)
        {
            debugRenderer->renderChunkBoundaries(
                camera->getViewMatrix(),
                camera->getProjectionMatrix(),
                grid);
        }

        if (renderSettings.showGridLines)
        {
            debugRenderer->renderGridLines(
                camera->getViewMatrix(),
                camera->getProjectionMatrix(),
                grid->getMinBounds(),
                grid->getMaxBounds(),
                grid->getSpacing());
        }
    }

    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Restore original culling state
    renderSettings.enableFrustumCulling = originalCullingState;

    /*GLenum err = glGetError();
    if (err != GL_NO_ERROR && !errorYet)
    {
        std::cout << "5OpenGL error during rendering: 0x" << std::hex << err << std::dec << std::endl;
    }
    errorYet = true;*/
}

void Application::renderUI()
{
    // The viewport panel in UIManager would display the scene texture
    uiManager->setSceneTexture(sceneColorTexture);
    uiManager->render();
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
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lines.size()));
    glLineWidth(1.0f); // Reset line width

    glBindVertexArray(0);
}

void Application::renderDebugView()
{
    // Reset viewport to full screen for main rendering
    glViewport(0, 0, width, height);

    float posX = static_cast<float>(width - SHADOW_WIDTH / 4);
    float posY = static_cast<float>(height - SHADOW_HEIGHT / 4);

    switch (renderSettings.currentDebugView)
    {
        case 0: // Depth map in corner of screen
            debugRenderer->renderDebugTexture(depthMap, posX, posY,
                                              static_cast<float>(SHADOW_WIDTH / 4),
                                              static_cast<float>(SHADOW_HEIGHT / 4), true);
            break;

        case 1: // Full screen depth map
            debugRenderer->renderDebugTexture(depthMap,
                                              0, 0,
                                              static_cast<float>(width),
                                              static_cast<float>(height),
                                              true);
            break;

        case 2: // Full screen depth map with linear depth output
            // Special visualization with depth linearization
            // This would need a custom shader to properly visualize depth
            debugRenderer->renderDebugTexture(depthMap,
                                              0, 0,
                                              static_cast<float>(width),
                                              static_cast<float>(height),
                                              true);
            // Add text overlay explaining the view
            // (Would need additional text rendering functionality)
            break;
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
    std::vector<glm::vec4> ndc = {ntl, ntr, nbl, nbr, ftl, ftr, fbl, fbr};

    for (int i = 0; i < 8; i++)
    {
        // Apply inverse view-projection to get world space corners
        glm::vec4 worldPos = invViewProj * ndc[i];
        worldPos /= worldPos.w; // Perspective divide
        corners[i] = glm::vec3(worldPos);
    }

    return corners;
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
    cullingStats.visibleCubes = visibleCount;

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

    // Get the cube's world position
    const Cube& cube = grid->getCube(x, y, z);

    // Use the frustum test to determine visibility
    return viewFrustum.isCubeVisible(cube.position, grid->getSpacing());
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
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
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
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Error: Shadow map framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

        // Resize framebuffer textures
        glBindTexture(GL_TEXTURE_2D, sceneColorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

        glBindTexture(GL_TEXTURE_2D, sceneDepthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

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
