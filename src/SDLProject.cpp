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

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void GLAPIENTRY errorCallback(GLenum source, GLenum type, GLuint id,
	GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
const char* getGLErrorString(GLenum error);

bool initializeWindow(const char* title, int width, int height);
bool initialize();
void deinitialize();
bool update();
void processInput();
void render(Shader &shader);
void loop(Shader &shader);
void setupTriangle(Shader &shader);

// Global variables
int* gFrameBuffer;
int* gTempBuffer;
GLFWwindow* gWindow;
static int gDone;
const int WINDOW_WIDTH = 1920 / 2;
const int WINDOW_HEIGHT = 1080 / 2;

unsigned int shaderProgram;
unsigned int VAO, VBO, EBO;
unsigned int texture1, texture2;

using namespace std;

int main(int argc, char** argv)
{
	if (!initialize())
	{
		return -1;
	}

	Shader shader("shaders/VertexShader.glsl", "shaders/FragmentShader.glsl");
	setupTriangle(shader);

	gDone = 0;
	while (!gDone)
	{
		loop(shader);
	}

	deinitialize();

	return 0;
}

bool initializeWindow(const char* title, int width, int height)
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
	gWindow = glfwCreateWindow(width, height, title, NULL, NULL);
	if (!gWindow)
	{
		fprintf(stderr, "Window creation failed\n");
		glfwTerminate();
		return false;
	}

	// Create OpenGL context
	glfwMakeContextCurrent(gWindow);
	glfwSetFramebufferSizeCallback(gWindow, framebuffer_size_callback);

	// Load OpenGL functions using GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
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
		fprintf(stderr, "Failed to set viewport: %s\n", getGLErrorString(error));
		glfwDestroyWindow(gWindow);
		glfwTerminate();
		return false;
	}

	// Enable depth testing
	glEnable(GL_DEPTH_TEST);
	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		fprintf(stderr, "Failed to enable depth test: %s\n", getGLErrorString(error));
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

	// Get OpenGL vendor and renderer info
	const GLubyte* vendor = glGetString(GL_VENDOR);
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

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
		glDebugMessageCallback(errorCallback, 0);
	}
	else
	{
		fprintf(stderr, "OpenGL 4.3 or higher is required for debug output\n");
	}

	return true;
}

void deinitialize()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	glfwDestroyWindow(gWindow);
	glfwTerminate();
}

bool update()
{
	processInput();
	if (glfwWindowShouldClose(gWindow))
	{
		return false;
	}

	return true;
}

void processInput()
{
	if (glfwGetKey(gWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(gWindow, true);
	}
}

void render(Shader &shader)
{
	// Clear the screen with a green color
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Bind texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture2);

	shader.use();

	// Create transformations
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection;
	model = glm::rotate(model, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
	projection = glm::perspective(glm::radians(45.0f), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);

	unsigned int modelLoc = glGetUniformLocation(shader.ID, "model");
	unsigned int viewLoc = glGetUniformLocation(shader.ID, "view");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	shader.setMat4("projection", projection);

	// Render container
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glfwSwapBuffers(gWindow);
	glfwPollEvents();
}

void loop(Shader &shader)
{
	if (!update())
	{
		gDone = 1;
	}
	else
	{
		render(shader);
	}
}

void setupTriangle(Shader &shader)
{
	// Vertex data
	float vertices[] = {
		// positions          // colors           // texture coords
		 0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   // top right
		 0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
		-0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // top left
	};
	unsigned int indices[] = {
		0, 1, 3, // first triangle
		1, 2, 3  // second triangle
	};

	// Create vertex array object and vertex buffer object
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// Bind VAO
	glBindVertexArray(VAO);

	// Bind VBO and upload vertex data
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Bind EBO and upload vertex data
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Texture coord attribute
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	// Load and create texture1
	glGenTextures(1, &texture1);
	glBindTexture(GL_TEXTURE_2D, texture1);
	// Set the texture wrapping/filtering options (on the currently bound texture object)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Load and generate the texture
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load("assets/container.jpg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture1" << std::endl;
	}
	stbi_image_free(data);

	// Load and create texture2
	glGenTextures(1, &texture2);
	glBindTexture(GL_TEXTURE_2D, texture2);
	// Set the texture wrapping/filtering options (on the currently bound texture object)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Load and generate the texture
	data = stbi_load("assets/awesomeface.png", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture2" << std::endl;
	}
	stbi_image_free(data);

	// Tell OpenGL for each sampler to which texture unit it belongs to (only has to be done once)
	shader.use();
	shader.setInt("texture1", 0);
	shader.setInt("texture2", 1);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and
	// height will be significantly larger than specified on retina displays
	glViewport(0, 0, width, height);
}

void GLAPIENTRY errorCallback(GLenum source, GLenum type, GLuint id,
	GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	fprintf(stderr, "GL Error: type = 0x%x, severity = 0x%x, message = %s\n",
		type, severity, message);
}

const char* getGLErrorString(GLenum error)
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
