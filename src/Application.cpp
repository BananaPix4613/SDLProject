#include "Application.h"
#include <iostream>

Application::Application(int windowWidth, int windowHeight)
    : width(windowWidth), height(windowHeight),
      window(nullptr), grid(nullptr), renderer(nullptr), camera(nullptr),
      shader(nullptr), showDebugView(false), currentDebugView(0),
      lastFrame(0.0f), deltaTime(0.0f), showUI(true),
      selectedCubeColor(0.5f, 0.5f, 1.0f), isEditing(false), brushSize(1),
      selectedCubeX(-1), selectedCubeY(-1), selectedCubeZ(-1),
      imGui(nullptr) // Initialize ImGui pointer
{
    
}

Application::~Application()
{
    delete grid;
    delete renderer;
    delete camera;
    delete shader;
    delete debugRenderer;
    delete imGui;

    glfwDestroyWindow(window);
    glfwTerminate();
}

bool Application::initialize()
{
    if (!initializeWindow())
    {
        return false;
    }

    // Create grid
    grid = new CubeGrid(20, 1.0f);

    // Create camera
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    camera = new IsometricCamera(aspect);

    // Create renderer
    renderer = new CubeRenderer(grid);
    renderer->initialize();

    // Load shader
    shader = new Shader("shaders/VertexShader.glsl", "shaders/FragmentShader.glsl");
    shadowShader = new Shader("shaders/ShadowMappingVertexShader.glsl", "shaders/ShadowMappingFragmentShader.glsl");

    // Setup shadow mapping
    setupShadowMap();

    // Initialize debug renderer
    debugRenderer = new DebugRenderer();

    // Initialize ImGui
    imGui = new ImGuiWrapper();
    imGui->initialize(window);

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
    // Create a framebuffer object for the depth map
    glGenFramebuffers(1, &depthMapFBO);

    // Create a 2D texture for the depth map
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    // Improved texture filtering for better quality shadows
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Important: Fix shadow acne with proper clamping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Use comparison mode for better shadow mapping quality
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

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

        // Camera panning
        glm::vec3 panDirection(0.0f);
        float panSpeed = 5.0f * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            panDirection += glm::vec3(0.0f, 0.0f, -1.0f);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            panDirection += glm::vec3(0.0f, 0.0f, 1.0f);
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            panDirection += glm::vec3(-1.0f, 0.0f, 0.0f);
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            panDirection += glm::vec3(1.0f, 0.0f, 0.0f);
        }

        if (glm::length(panDirection) > 0.0f)
        {
            // Transform direction according to camera orientation
            glm::mat4 viewMatrix = camera->getViewMatrix();
            glm::mat3 rotationMatrix = glm::mat3(viewMatrix);
            glm::vec3 transformedDirection = rotationMatrix * glm::normalize(panDirection);

            // Remove any vertical component for pure horizontal panning
            transformedDirection.y = 0.0f;
            transformedDirection = glm::normalize(transformedDirection);

            camera->pan(transformedDirection * panSpeed);
        }
    }

    // Toggle debug view
    static bool tabReleased = true;
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS && tabReleased)
    {
        showDebugView = !showDebugView;
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
    grid->update(deltaTime);
}

void Application::render()
{
    // 1. First render to depth map (shadow pass)
    glm::mat4 lightProjection, lightView, lightSpaceMatrix;
    float near_plane = 1.0f, far_plane = 40.0f;

    // Better orthographic projection dimensions for the light
    float orthoSize = 15.0f; // Adjust based on scene size
    lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, near_plane, far_plane);

    // Light view matrix - adjust light position for better coverage
    // Position the light to cast shadows at an angle for more interesting results
    glm::vec3 lightPos = glm::vec3(15.0f, 20.0f, 15.0f);
    glm::vec3 lightTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    lightView = glm::lookAt(lightPos, lightTarget, glm::vec3(0.0f, 1.0f, 0.0f));

    // Light space transformation matrix
    lightSpaceMatrix = lightProjection * lightView;

    // Use the shadow mapping shader
    shadowShader->use();
    shadowShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

    // Configure viewport to shadow map dimensions
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

    // When rendering to depth map, use polygon offset to reduce shadow acne
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);

    // Cull front faces for shadow mapping to reduce peter-panning
    //GLint cullFaceMode;
    //glGetIntegerv(GL_CULL_FACE_MODE, &cullFaceMode);
    //glCullFace(GL_FRONT);

    // Bind shadow map framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Render scene from light's perspective
    renderSceneDepth(*shadowShader);

    // Reset state
    //glCullFace(cullFaceMode);
    glDisable(GL_POLYGON_OFFSET_FILL);

    // Reset framebuffer and viewport
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 2. Render scene normally with shadow mapping
    shader->use();

    // Set view and projection matrices
    shader->setMat4("view", camera->getViewMatrix());
    shader->setMat4("projection", camera->getProjectionMatrix());

    // Set light position and view position
    shader->setVec3("lightPos", lightPos);
    shader->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    shader->setVec3("viewPos", camera->getPosition());

    // Add additional lighting parameters for better control
    shader->setFloat("ambientStrength", 0.2f);    // Ambient light intensity
    shader->setFloat("specularStrength", 0.5f);   // Specular highlights intensity
    shader->setFloat("shininess", 32.0f);         // Specular shininess

    // Set light space matrix and bind shadow map
    shader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    shader->setInt("shadowMap", 0);

    // Additional shadows parameters
    shader->setFloat("shadowBias", 0.005f);       // Shadow bias to reduce acne
    shader->setBool("usePCF", true);              // Enable PCF filtering for soft shadows

    // Render the grid
    renderer->render(*shader);

    // 3. Debug visualization if enabled
    if (showDebugView)
    {
        // Reset viewport to full screen for main rendering
        glViewport(0, 0, width, height);

        switch (currentDebugView)
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
}

void Application::renderUI()
{
    // Main control panel
    imGui->beginWindow("Control Panel");

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
            // Implement grid clearing functionality
            // This would reset all cubes except the floor
            for (int x = 0; x < grid->getSize(); x++)
            {
                for (int y = 0; y < grid->getSize(); y++)
                {
                    for (int z = 0; z < grid->getSize(); z++)
                    {
                        if (y > 0) // Keep the floor
                        {
                            setCubeAt(x, y, z, false, glm::vec3(0.0f));
                        }
                    }
                }
            }
        }
    }

    // Debug options
    if (ImGui::CollapsingHeader("Debug Options"))
    {
        ImGui::Checkbox("Show Debug View", &showDebugView);

        const char* debugViews[] = { "Shadow Map Corner", "Full Shadow Map", "Linearized Depth" };
        ImGui::Combo("Debug View Mode", &currentDebugView, debugViews, 3);
    }

    // Performance stats
    if (ImGui::CollapsingHeader("Performance"))
    {
        ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
        ImGui::Text("Frame Time: %.3f ms", deltaTime * 1000.0f);
    }

    imGui->endWindow();
}

void Application::renderSceneDepth(Shader& depthShader)
{
    // Similar to render, but only pass model matrices for depth rendering
    renderer->renderDepthOnly(depthShader);
}

void Application::setCubeAt(int x, int y, int z, bool active, const glm::vec3& color)
{
    if (x >= 0 && x < grid->getSize() &&
        y >= 0 && y < grid->getSize() &&
        z >= 0 && z < grid->getSize())
    {
        Cube cube = grid->getCube(x, y, z);
        cube.active = active;
        if (active)
        {
            cube.color = color;
        }
        grid->setCube(x, y, z, cube);
    }
}

bool Application::pickCube(int& outX, int& outY, int& outZ)
{
    // Get mouse position
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    // Convert mouse position to normalized device coordinates
    float ndcX = (2.0f * mouseX) / width - 1.0f;
    float ndcY = 1.0f - (2.0f * mouseY) / height;

    // For orthographic camera, ray is parallel to view direction
    glm::vec4 rayStart = glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 rayEnd = glm::vec4(ndcX, ndcY, 1.0f, 1.0f);

    // Convert to world space
    glm::mat4 invViewProj = glm::inverse(camera->getProjectionMatrix() * camera->getViewMatrix());
    glm::vec4 worldRayStart = invViewProj * rayStart;
    glm::vec4 worldRayEnd = invViewProj * rayEnd;

    // Perform perspective division
    worldRayStart /= worldRayStart.w;
    worldRayEnd /= worldRayEnd.w;

    // Calculate ray direction
    glm::vec3 rayOrigin = glm::vec3(worldRayStart);
    glm::vec3 rayDir = glm::normalize(glm::vec3(worldRayEnd) - glm::vec3(worldRayStart));

    // Rest of your ray intersection code...
    float gridSize = grid->getSize();
    float spacing = grid->getSpacing();

    // Grid bounds
    glm::vec3 gridMin = glm::vec3(-gridSize * spacing / 2.0f);
    glm::vec3 gridMax = glm::vec3(gridSize * spacing / 2.0f);

    // Debug output
    std::cout << "Ray origin: " << rayOrigin.x << ", " << rayOrigin.y << ", " << rayOrigin.z << std::endl;
    std::cout << "Ray direction: " << rayDir.x << ", " << rayDir.y << ", " << rayDir.z << std::endl;
    std::cout << "Grid min: " << gridMin.x << ", " << gridMin.y << ", " << gridMin.z << std::endl;
    std::cout << "Grid max: " << gridMax.x << ", " << gridMax.y << ", " << gridMax.z << std::endl;

    // Check if ray intersects grid bounds
    float tMin = 0.0f;
    float tMax = 1000.0f;

    // X slab
    if (rayDir.x != 0.0f)
    {
        float t1 = (gridMin.x - rayOrigin.x) / rayDir.x;
        float t2 = (gridMax.x - rayOrigin.x) / rayDir.x;
        tMin = glm::max(tMin, glm::min(t1, t2));
        tMax = glm::min(tMax, glm::max(t1, t2));
    }

    // Y slab
    if (rayDir.y != 0.0f)
    {
        float t1 = (gridMin.y - rayOrigin.y) / rayDir.y;
        float t2 = (gridMax.y - rayOrigin.y) / rayDir.y;
        tMin = glm::max(tMin, glm::min(t1, t2));
        tMax = glm::min(tMax, glm::max(t1, t2));
    }

    // Z slab
    if (rayDir.z != 0.0f)
    {
        float t1 = (gridMin.z - rayOrigin.z) / rayDir.z;
        float t2 = (gridMax.z - rayOrigin.z) / rayDir.z;
        tMin = glm::max(tMin, glm::min(t1, t2));
        tMax = glm::min(tMax, glm::max(t1, t2));
    }

    std::cout << "tMin: " << tMin << ", tMax: " << tMax << std::endl;

    // If tMax < tMin, no intersection
    if (tMax < tMin)
    {
        std::cout << "No intersection with grid bounds" << std::endl;
        return false;
    }

    // Start traversal at first intersection with grid bounds
    glm::vec3 pos = rayOrigin + rayDir * tMin;

    // Convert to grid coordinates
    int gridX = static_cast<int>((pos.x - gridMin.x) / spacing);
    int gridY = static_cast<int>((pos.y - gridMin.y) / spacing);
    int gridZ = static_cast<int>((pos.z - gridMin.z) / spacing);

    // Direction to increment grid coordinates
    int stepX = rayDir.x > 0 ? 1 : -1;
    int stepY = rayDir.y > 0 ? 1 : -1;
    int stepZ = rayDir.z > 0 ? 1 : -1;

    // Distance to next cell boundary
    float nextX = (stepX > 0) ? (gridX + 1) * spacing + gridMin.x : gridX * spacing + gridMin.x;
    float nextY = (stepY > 0) ? (gridY + 1) * spacing + gridMin.y : gridY * spacing + gridMin.y;
    float nextZ = (stepZ > 0) ? (gridZ + 1) * spacing + gridMin.z : gridZ * spacing + gridMin.z;

    // Distance along ray to next cell boundary
    float tMaxX = (rayDir.x != 0) ? (nextX - rayOrigin.x) / rayDir.x : tMax;
    float tMaxY = (rayDir.y != 0) ? (nextY - rayOrigin.y) / rayDir.y : tMax;
    float tMaxZ = (rayDir.z != 0) ? (nextZ - rayOrigin.z) / rayDir.z : tMax;

    // Delta t to cross one cell
    float tDeltaX = (rayDir.x != 0) ? spacing / std::abs(rayDir.x) : tMax;
    float tDeltaY = (rayDir.y != 0) ? spacing / std::abs(rayDir.y) : tMax;
    float tDeltaZ = (rayDir.z != 0) ? spacing / std::abs(rayDir.z) : tMax;

    // Previous cell for placing new cube
    int prevX = gridX;
    int prevY = gridY;
    int prevZ = gridZ;

    // Traverse grid
    while (gridX >= 0 && gridX < gridSize && gridY >= 0 && gridY < gridSize && gridZ >= 0 && gridZ < gridSize)
    {
        // Check if current cell has a cube
        if (grid->isCubeActive(gridX, gridY, gridZ))
        {
            // Found a cube! Return its position
            outX = gridX;
            outY = gridY;
            outZ = gridZ;
            return true;
        }

        // Store previous position for placing cubes on empty space
        prevX = gridX;
        prevY = gridY;
        prevZ = gridZ;

        // Advance to next cell
        if (tMaxX < tMaxY && tMaxX < tMaxZ)
        {
            // X axis traversal
            tMaxX += tDeltaX;
            gridX += stepX;
        }
        else if (tMaxY < tMaxZ)
        {
            // Y axis traversal
            tMaxY += tDeltaY;
            gridY += stepY;
        }
        else
        {
            // Z axis traversal
            tMaxZ += tDeltaZ;
            gridZ += stepZ;
        }

        // Check if we've gone too far
        if (tMaxX > tMax && tMaxY > tMax && tMaxZ > tMax)
        {
            break;
        }
    }

    // If we didn't hit a cube, use the last empty cell we traversed
    // This lets users place cubes on empty space
    outX = prevX;
    outY = prevY;
    outZ = prevZ;

    return true;
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
