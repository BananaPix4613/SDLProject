#include <windows.h>
#include <string>
#include <iostream>
#include <vector>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <filesystem>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include "shader.h"
#include "camera.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Function declarations
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void GLAPIENTRY error_callback(GLenum source, GLenum type, GLuint id,
                               GLenum severity, GLsizei length,
                               const GLchar *message, const void *userParam);
void mouse_callback(GLFWwindow *window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput();
unsigned int loadTexture(const char* path);
const char *getGLErrorString(GLenum error);

bool initializeWindow(const char *title, int width, int height);
bool initialize();
void deinitialize();
bool update();
void render(Shader& lightingShader, Shader& lightSourceShader);
void loop(Shader& lightingShader, Shader& lightSourceShader);
void setupTriangle(Shader &lightingShader, Shader &lightSourceShader);

// Global variables
int *gFrameBuffer;
int *gTempBuffer;
GLFWwindow *gWindow;
static int gDone;
constexpr int WINDOW_WIDTH = 1920 / 2;
constexpr int WINDOW_HEIGHT = 1080 / 2;

unsigned int shaderProgram;
unsigned int VBO, cubeVAO, lightVAO;
unsigned int texture1, texture2;

unsigned int diffuseMap;
unsigned int specularMap;

// Camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = WINDOW_WIDTH / 2;
float lastY = WINDOW_HEIGHT / 2;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

// Lighting
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

float vertices[] = {
    // positions          // normals           // texture coords
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
    0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,

    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

    -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
    0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
    0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,

    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f
};

using namespace std;

int main(int argc, char **argv)
{
    if (!initialize())
    {
        return -1;
    }

    //Shader shader("shaders/VertexShader.glsl", "shaders/FragmentShader.glsl");
    Shader lightingShader("shaders/BasicLightingVertexShader.glsl", "shaders/BasicLightingFragmentShader.glsl");
    Shader lightSourceShader("shaders/LightCubeVertexShader.glsl", "shaders/LightCubeFragmentShader.glsl");
    setupTriangle(lightingShader, lightSourceShader);

    gDone = 0;
    while (!gDone)
    {
        loop(lightingShader, lightSourceShader);
    }

    deinitialize();

    return 0;
}

bool initializeWindow(const char *title, int width, int height)
{
    // Initialize GLFW
    if (!glfwInit())
    {
        fprintf(stderr, "GLFW initialization failed\n");
        return false;
    }

    // Set GLFW version and profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    gWindow = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!gWindow)
    {
        fprintf(stderr, "Window creation failed\n");
        glfwTerminate();
        return false;
    }

    // Create OpenGL context
    glfwMakeContextCurrent(gWindow);
    glfwSetFramebufferSizeCallback(gWindow, framebuffer_size_callback);
    glfwSetCursorPosCallback(gWindow, mouse_callback);
    glfwSetScrollCallback(gWindow, scroll_callback);

    // Load OpenGL functions using GLAD
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        fprintf(stderr, "Failed to initialize GLAD\n");
        glfwDestroyWindow(gWindow);
        glfwTerminate();
        return false;
    }

    // Enable VSync (optional)
    glfwSwapInterval(1);

    // Setup OpenGL viewport
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        fprintf(stderr, "Failed to set viewport: %s\n",
                getGLErrorString(error));
        glfwDestroyWindow(gWindow);
        glfwTerminate();
        return false;
    }

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    error = glGetError();
    if (error != GL_NO_ERROR)
    {
        fprintf(stderr, "Failed to enable depth test: %s\n",
                getGLErrorString(error));
        glfwDestroyWindow(gWindow);
        glfwTerminate();
        return false;
    }

    return true;
}

bool initialize()
{
    if (!initializeWindow("GLFW window", WINDOW_WIDTH, WINDOW_HEIGHT))
    {
        return false;
    }


    // Set input mode
    glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Get OpenGL vendor and renderer info
    const GLubyte *vendor = glGetString(GL_VENDOR);
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *version = glGetString(GL_VERSION);
    const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    if (!vendor || !renderer || !version || !glslVersion)
    {
        fprintf(stderr, "Failed to get OpenGL info\n");
        glfwDestroyWindow(gWindow);
        glfwTerminate();
        return false;
    }

    printf("OpenGL Vendor: %s\n", vendor);
    printf("OpenGL Renderer: %s\n", renderer);
    printf("OpenGL Version: %s\n", version);
    printf("GLSL Version: %s\n", glslVersion);

    // Enable debug output
    if (GLAD_GL_VERSION_4_3)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(error_callback, nullptr);
    }
    else
    {
        fprintf(
            stderr, "OpenGL 4.3 or higher is required for debug output\n");
    }

    return true;
}

void deinitialize()
{
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &lightVAO);
    glDeleteBuffers(1, &VBO);
    //glDeleteBuffers(1, &EBO);

    glfwDestroyWindow(gWindow);
    glfwTerminate();
}

bool update()
{
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    processInput();
    if (glfwWindowShouldClose(gWindow))
    {
        return false;
    }

    return true;
}

void render(Shader& lightingShader, Shader& lightSourceShader)
{
    // Clear the screen with a green color
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Be sure to activate shader when setting uniforms/drawing objects
    lightingShader.use();
    lightingShader.setVec3("light.position", lightPos);
    lightingShader.setVec3("viewPos", camera.Position);

    // Light properties
    lightingShader.setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
    lightingShader.setVec3("light.diffuse", 0.5f, 0.5f, 0.5f);
    lightingShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);

    // Material properties
    lightingShader.setFloat("material.shininess", 64.0f);

    // Camera/View transformations
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                            static_cast<float>(WINDOW_WIDTH)
                                            / static_cast<float>(
                                                WINDOW_HEIGHT), 0.1f,
                                            100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    lightingShader.setMat4("projection", projection);
    lightingShader.setMat4("view", view);

    // World transformation
    glm::mat4 model = glm::mat4(1.0f);
    lightingShader.setMat4("model", model);

    // Bind diffuse map
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseMap);

    // Bind specular map
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, specularMap);

    // Render the cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // Also draw the lamp object
    lightSourceShader.use();
    lightSourceShader.setMat4("projection", projection);
    lightSourceShader.setMat4("view", view);
    model = glm::mat4(1.0f);
    model = glm::translate(model, lightPos);
    model = glm::scale(model, glm::vec3(0.2f)); // A smaller cube
    lightSourceShader.setMat4("model", model);

    glBindVertexArray(lightVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glfwSwapBuffers(gWindow);
    glfwPollEvents();
}

void loop(Shader& lightingShader, Shader& lightSourceShader)
{
    if (!update())
    {
        gDone = 1;
    }
    else
    {
        render(lightingShader, lightSourceShader);
    }
}

void setupTriangle(Shader& lightingShader, Shader& lightSourceShader)
{
    // Configure VAO (and VBO)
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &VBO);
    //glGenBuffers(1, &EBO);

    // Bind VBO and upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
                 GL_STATIC_DRAW);

    // Bind VAO
    glBindVertexArray(cubeVAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Configure the light VAO (VBO stays the same)
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Set the vertex attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    diffuseMap = loadTexture("assets/container2.png");
    specularMap = loadTexture("assets/container2_specular.png");

    lightingShader.use();
    lightingShader.setInt("material.diffuse", 0);
    lightingShader.setInt("material.specular", 1);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays
    glViewport(0, 0, width, height);
}

void GLAPIENTRY error_callback(GLenum source, GLenum type, GLuint id,
                               GLenum severity, GLsizei length,
                               const GLchar *message, const void *userParam)
{
    fprintf(stderr, "GL Error: type = 0x%x, severity = 0x%x, message = %s\n",
            type, severity, message);
}

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY -
        ypos; // reversed since y-coordinates range from bottom to top
    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void processInput()
{
    if (glfwGetKey(gWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(gWindow, true);

    if (glfwGetKey(gWindow, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(gWindow, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(gWindow, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(gWindow, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

unsigned int loadTexture(const char *path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

const char *getGLErrorString(GLenum error)
{
    switch (error)
    {
    case GL_NO_ERROR:
        return "No error";
    case GL_INVALID_ENUM:
        return "Invalid enum";
    case GL_INVALID_VALUE:
        return "Invalid value";
    case GL_STACK_OVERFLOW:
        return "Stack overflow";
    case GL_STACK_UNDERFLOW:
        return "Stack underflow";
    case GL_OUT_OF_MEMORY:
        return "Out of memory";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "Invalid framebuffer operation";
    default:
        return "Unknown error";
    }
}
