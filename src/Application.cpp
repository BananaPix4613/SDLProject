#include "Application.h"
#include <iostream>

Application::Application(int windowWidth, int windowHeight)
    : width(windowWidth), height(windowHeight),
      window(nullptr), grid(nullptr), renderer(nullptr), camera(nullptr), shader(nullptr),
      lastFrame(0.0f), deltaTime(0.0f)
{
    
}

Application::~Application()
{
    delete grid;
    delete renderer;
    delete camera;
    delete shader;

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
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);

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

    return true;
}

void Application::run()
{
    while (!glfwWindowShouldClose(window))
    {
        // Calculate delta time
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process input
        processInput();

        // Update logic
        update();

        // Render
        render();

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void Application::processInput()
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

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
        camera->pan(glm::normalize(panDirection) * panSpeed);
    }
}

void Application::update()
{
    grid->update(deltaTime);
}

void Application::render()
{
    // Clear the screen
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Update the view and projection matrices
    shader->use();
    shader->setMat4("view", camera->getViewMatrix());
    shader->setMat4("projection", camera->getProjectionMatrix());

    // Set light position and view position
    glm::vec3 lightPos = glm::vec3(10.0f, 10.0f, 10.0f); // Light position
    shader->setVec3("lightPos", lightPos);
    shader->setVec3("viewPos", camera->getPosition()); // Camera position for specular calculations

    // Render the grid
    renderer->render(*shader);
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

void Application::errorCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                GLsizei length, const GLchar *message, const void *userParam)
{
    std::cerr << "GL Error: type = 0x" << std::hex << type
              << ", severity = 0x" << std::hex << severity
              << ", message = " << message << std::endl;
}

//#define GLFW_INCLUDE_NONE
//#include <GLFW/glfw3.h>
//#include <glad/glad.h>
//
//#include <glm.hpp>
//#include <gtc/matrix_transform.hpp>
//#include <gtc/type_ptr.hpp>
//
//#include <shader.h>
//#include <camera.h>
//#include <model.h>
//
//#include <iostream>
//#include <vector>
//
//// Function declarations
//void framebuffer_size_callback(GLFWwindow *window, int width, int height);
//void GLAPIENTRY error_callback(GLenum source, GLenum type, GLuint id,
//                               GLenum severity, GLsizei length,
//                               const GLchar *message, const void *userParam);
//void mouse_callback(GLFWwindow *window, double xposIn, double yposIn);
//void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
//void processInput();
////unsigned int loadTexture(const char* path);
//const char *getGLErrorString(GLenum error);
//
//bool initializeWindow(const char *title, int width, int height);
//bool initialize();
//void initializeGrid();
//void deinitialize();
//bool update();
//void render(Shader &shader);
//void loop(Shader& shader);
//
//// Global variables
//int *gFrameBuffer;
//int *gTempBuffer;
//GLFWwindow *gWindow;
//static int gDone;
//constexpr int WINDOW_WIDTH = 1920 / 2;
//constexpr int WINDOW_HEIGHT = 1080 / 2;
//
//unsigned int shaderProgram;
//unsigned int gridVAO, gridVBO;
//unsigned int cubeVAO, cubeVBO;
//
//unsigned int diffuseMap;
//unsigned int specularMap;
//
//// Camera
//Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
//float lastX = WINDOW_WIDTH / 2;
//float lastY = WINDOW_HEIGHT / 2;
//bool firstMouse = true;
//
//// Timing
//float deltaTime = 0.0f;	// Time between current frame and last frame
//float lastFrame = 0.0f; // Time of last frame
//
//// Lighting
//glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
//
//// Structs
//struct Cube
//{
//    glm::vec3 position;
//    glm::vec3 color; // Optional
//
//    Cube() : position(0.0f), color(1.0f) {}
//    Cube(glm::vec3 pos, glm::vec3 col) : position(pos), color(col) {}
//};
//
//int gridSize = 20;
//float gridSpacing = 1.0f;
//std::vector<std::vector<std::vector<Cube>>> grid;
//
//float cubeVertices[] = {
//    // positions          // normals           // texture coords
//    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
//     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
//     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
//     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
//    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
//    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
//
//    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
//     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
//     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
//     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
//    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
//    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
//
//    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
//    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
//    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
//    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
//    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
//    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
//
//     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
//     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
//     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
//     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
//     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
//     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
//
//    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
//     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
//     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
//     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
//    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
//    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
//
//    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
//     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
//     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
//     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
//    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
//    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
//};
//
////glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//
//using namespace std;
//
//int main(int argc, char **argv)
//{
//    if (!initialize())
//    {
//        return -1;
//    }
//
//    //setupTriangle();
//    // Shaders
//    Shader shader("shaders/VertexShader.glsl", "shaders/FragmentShader.glsl");
//    //Shader lightingShader("shaders/BasicLightingVertexShader.glsl", "shaders/BasicLightingFragmentShader.glsl");
//    //Shader lightSourceShader("shaders/LightCubeVertexShader.glsl", "shaders/LightCubeFragmentShader.glsl");
//
//    // Models
//    //Model backpack("assets/objects/backpack/backpack.obj");
//
//    gDone = 0;
//    while (!gDone)
//    {
//        loop(shader);
//    }
//
//    deinitialize();
//
//    return 0;
//}
//
//bool initializeWindow(const char *title, int width, int height)
//{
//    // Initialize GLFW
//    if (!glfwInit())
//    {
//        fprintf(stderr, "GLFW initialization failed\n");
//        return false;
//    }
//
//    // Set GLFW version and profile
//    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
//    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//
//    // Create window
//    gWindow = glfwCreateWindow(width, height, title, nullptr, nullptr);
//    if (!gWindow)
//    {
//        fprintf(stderr, "Window creation failed\n");
//        glfwTerminate();
//        return false;
//    }
//
//    // Create OpenGL context
//    glfwMakeContextCurrent(gWindow);
//    glfwSetFramebufferSizeCallback(gWindow, framebuffer_size_callback);
//    glfwSetCursorPosCallback(gWindow, mouse_callback);
//    glfwSetScrollCallback(gWindow, scroll_callback);
//
//    // Load OpenGL functions using GLAD
//    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
//    {
//        fprintf(stderr, "Failed to initialize GLAD\n");
//        glfwDestroyWindow(gWindow);
//        glfwTerminate();
//        return false;
//    }
//
//    // Enable VSync (optional)
//    glfwSwapInterval(1);
//
//    // Setup OpenGL viewport
//    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
//    GLenum error = glGetError();
//    if (error != GL_NO_ERROR)
//    {
//        fprintf(stderr, "Failed to set viewport: %s\n",
//                getGLErrorString(error));
//        glfwDestroyWindow(gWindow);
//        glfwTerminate();
//        return false;
//    }
//
//    // Tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
//    stbi_set_flip_vertically_on_load(true);
//
//    // Enable depth testing
//    glEnable(GL_DEPTH_TEST);
//    error = glGetError();
//    if (error != GL_NO_ERROR)
//    {
//        fprintf(stderr, "Failed to enable depth test: %s\n",
//                getGLErrorString(error));
//        glfwDestroyWindow(gWindow);
//        glfwTerminate();
//        return false;
//    }
//
//    glEnable(GL_CULL_FACE);
//    error = glGetError();
//    if (error != GL_NO_ERROR)
//    {
//        fprintf(stderr, "Failed to enable face culling: %s\n",
//                getGLErrorString(error));
//        glfwDestroyWindow(gWindow);
//        glfwTerminate();
//        return false;
//    }
//
//    return true;
//}
//
//bool initialize()
//{
//    if (!initializeWindow("GLFW window", WINDOW_WIDTH, WINDOW_HEIGHT))
//    {
//        return false;
//    }
//
//    // Set input mode
//    glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
//
//    // Get OpenGL vendor and renderer info
//    const GLubyte *vendor = glGetString(GL_VENDOR);
//    const GLubyte *renderer = glGetString(GL_RENDERER);
//    const GLubyte *version = glGetString(GL_VERSION);
//    const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
//
//    if (!vendor || !renderer || !version || !glslVersion)
//    {
//        fprintf(stderr, "Failed to get OpenGL info\n");
//        glfwDestroyWindow(gWindow);
//        glfwTerminate();
//        return false;
//    }
//
//    printf("OpenGL Vendor: %s\n", vendor);
//    printf("OpenGL Renderer: %s\n", renderer);
//    printf("OpenGL Version: %s\n", version);
//    printf("GLSL Version: %s\n", glslVersion);
//
//    // Enable debug output
//    if (GLAD_GL_VERSION_4_3)
//    {
//        glEnable(GL_DEBUG_OUTPUT);
//        glDebugMessageCallback(error_callback, nullptr);
//    }
//    else
//    {
//        fprintf(
//            stderr, "OpenGL 4.3 or higher is required for debug output\n");
//    }
//
//    // World initialization
//    initializeGrid();
//
//    glGenVertexArrays(1, &cubeVAO);
//    glGenBuffers(1, &cubeVBO);
//
//    glBindVertexArray(cubeVAO);
//
//    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
//    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
//
//    // Position attribute
//    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
//    glEnableVertexAttribArray(0);
//    // Normal attribute
//    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
//    glEnableVertexAttribArray(1);
//    // Texture coordinate attribute
//    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
//    glEnableVertexAttribArray(2);
//
//    glBindVertexArray(0);
//
//    return true;
//}
//
//void initializeGrid()
//{
//    grid.resize(gridSize);
//    for (int x = 0; x < gridSize; ++x)
//    {
//        grid[x].resize(gridSize);
//        for (int y = 0; y < gridSize; ++y)
//        {
//            grid[x][y].resize(gridSize);
//            for (int z = 0; z < gridSize; ++z)
//            {
//                glm::vec3 position = glm::vec3(
//                    (x - gridSize / 2.0f) * gridSpacing,
//                    (y - gridSize / 2.0f) * gridSpacing,
//                    (z - gridSize / 2.0f) * gridSpacing
//                );
//
//                // Example condition for creating cubes - adjust as needed
//                if ((x + y + z) % 3 == 0) {
//                    float r = static_cast<float>(x) / gridSize;
//                    float g = static_cast<float>(y) / gridSize;
//                    float b = static_cast<float>(z) / gridSize;
//                    glm::vec3 color = glm::vec3(r, g, b);
//                    grid[x][y][z] = Cube(position, color);
//                }
//            }
//        }
//    }
//
//    glGenVertexArrays(1, &gridVAO);
//    glGenBuffers(1, &gridVBO);
//
//    glBindVertexArray(gridVAO);
//    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
//    glBufferData(GL_ARRAY_BUFFER, grid.size() * sizeof(float), &grid[0], GL_STATIC_DRAW);
//
//    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
//    glEnableVertexAttribArray(0);
//    glBindVertexArray(0);
//}
//
//void deinitialize()
//{
//    glDeleteVertexArrays(1, &gridVAO);
//    glDeleteBuffers(1, &gridVBO);
//
//    glDeleteVertexArrays(1, &cubeVAO);
//    glDeleteBuffers(1, &cubeVBO);
//
//    glfwDestroyWindow(gWindow);
//    glfwTerminate();
//}
//
//bool update()
//{
//    float currentFrame = static_cast<float>(glfwGetTime());
//    deltaTime = currentFrame - lastFrame;
//    lastFrame = currentFrame;
//
//    processInput();
//    if (glfwWindowShouldClose(gWindow))
//    {
//        return false;
//    }
//
//    return true;
//}
//
//void render(Shader &shader)
//{
//    // Clear the screen with a green color
//    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//    shader.use();
//
//    // Set up isometric view
//    float aspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
//    float orthoSize = 20.0f;
//    glm::mat4 projection = glm::ortho(-orthoSize * aspect, orthoSize * aspect, -orthoSize, orthoSize, -100.0f, 100.0f);
//
//    // Classic isometric angles approximately (35.264, 45, 0)
//    glm::mat4 view = glm::lookAt(
//        glm::vec3(1.0f, 1.0f, 1.0f) * 30.0f,  // Position camera equally in all directions
//        glm::vec3(0.0f, 0.0f, 0.0f),          // Look at center
//        glm::vec3(0.0f, 1.0f, 0.0f)           // Up vector
//    );
//
//    shader.setMat4("projection", projection);
//    shader.setMat4("view", view);
//
//    // Render the loaded model
//    //glm::mat4 model = glm::mat4(1.0f);
//    //model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
//    //model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
//    //shader.setMat4("model", model);
//    //backpack.Draw(shader);
//
//    for (int x = 0; x < gridSize; x++)
//    {
//        for (int y = 0; y < gridSize; y++)
//        {
//            for (int z = 0; z < gridSize; z++)
//            {
//                Cube& cube = grid[x][y][z];
//                glm::mat4 model = glm::mat4(1.0f);
//                model = glm::translate(model, cube.position);
//                shader.setMat4("model", model);
//
//                // Set cube color
//                shader.setVec3("color", cube.color);
//
//                // Draw the cube
//                glBindVertexArray(cubeVAO);
//                glDrawArrays(GL_TRIANGLES, 0, 36);
//                glBindVertexArray(0);
//            }
//        }
//    }
//
//    glfwSwapBuffers(gWindow);
//    glfwPollEvents();
//}
//
//void loop(Shader &shader)
//{
//    if (!update())
//    {
//        gDone = 1;
//    }
//    else
//    {
//        render(shader);
//    }
//}
//
//void framebuffer_size_callback(GLFWwindow *window, int width, int height)
//{
//    // make sure the viewport matches the new window dimensions; note that width and
//    // height will be significantly larger than specified on retina displays
//    glViewport(0, 0, width, height);
//}
//
//void GLAPIENTRY error_callback(GLenum source, GLenum type, GLuint id,
//                               GLenum severity, GLsizei length,
//                               const GLchar *message, const void *userParam)
//{
//    fprintf(stderr, "GL Error: type = 0x%x, severity = 0x%x, message = %s\n",
//            type, severity, message);
//}
//
//void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
//{
//    float xpos = static_cast<float>(xposIn);
//    float ypos = static_cast<float>(yposIn);
//
//    if (firstMouse)
//    {
//        lastX = xpos;
//        lastY = ypos;
//        firstMouse = false;
//    }
//
//    float xoffset = xpos - lastX;
//    float yoffset = lastY -
//        ypos; // reversed since y-coordinates range from bottom to top
//    lastX = xpos;
//    lastY = ypos;
//
//    camera.ProcessMouseMovement(xoffset, yoffset);
//}
//
//void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
//{
//    camera.ProcessMouseScroll(static_cast<float>(yoffset));
//}
//
//void processInput()
//{
//    if (glfwGetKey(gWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
//        glfwSetWindowShouldClose(gWindow, true);
//
//    if (glfwGetKey(gWindow, GLFW_KEY_W) == GLFW_PRESS)
//        camera.ProcessKeyboard(FORWARD, deltaTime);
//    if (glfwGetKey(gWindow, GLFW_KEY_S) == GLFW_PRESS)
//        camera.ProcessKeyboard(BACKWARD, deltaTime);
//    if (glfwGetKey(gWindow, GLFW_KEY_A) == GLFW_PRESS)
//        camera.ProcessKeyboard(LEFT, deltaTime);
//    if (glfwGetKey(gWindow, GLFW_KEY_D) == GLFW_PRESS)
//        camera.ProcessKeyboard(RIGHT, deltaTime);
//}
//
////unsigned int loadTexture(const char *path)
////{
////    unsigned int textureID;
////    glGenTextures(1, &textureID);
////
////    int width, height, nrComponents;
////    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
////    if (data)
////    {
////        GLenum format;
////        if (nrComponents == 1)
////            format = GL_RED;
////        else if (nrComponents == 3)
////            format = GL_RGB;
////        else if (nrComponents == 4)
////            format = GL_RGBA;
////
////        glBindTexture(GL_TEXTURE_2D, textureID);
////        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
////        glGenerateMipmap(GL_TEXTURE_2D);
////
////        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
////        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
////        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
////        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
////
////        stbi_image_free(data);
////    }
////    else
////    {
////        std::cout << "Texture failed to load at path: " << path << std::endl;
////        stbi_image_free(data);
////    }
////
////    return textureID;
////}
//
//const char *getGLErrorString(GLenum error)
//{
//    switch (error)
//    {
//    case GL_NO_ERROR:
//        return "No error";
//    case GL_INVALID_ENUM:
//        return "Invalid enum";
//    case GL_INVALID_VALUE:
//        return "Invalid value";
//    case GL_STACK_OVERFLOW:
//        return "Stack overflow";
//    case GL_STACK_UNDERFLOW:
//        return "Stack underflow";
//    case GL_OUT_OF_MEMORY:
//        return "Out of memory";
//    case GL_INVALID_FRAMEBUFFER_OPERATION:
//        return "Invalid framebuffer operation";
//    default:
//        return "Unknown error";
//    }
//}
