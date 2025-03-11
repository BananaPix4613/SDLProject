#include <windows.h>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

int* gFrameBuffer;
int* gTempBuffer;
GLFWwindow* gWindow;
static int gDone;
const int WINDOW_WIDTH = 1920 / 2;
const int WINDOW_HEIGHT = 1080 / 2;

const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;

out vec4 vertexColor;

void main()
{
	gl_Position = vec4(aPos, 1.0);
	vertexColor = vec4(0.5, 0.0, 0.0, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

uniform vec4 ourColor;

void main()
{
	FragColor = ourColor;
}
)";

unsigned int shaderProgram;
unsigned int VAO, VBO, EBO;

using namespace std;

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

	// Load OpenGL functions using GLAD
	/*if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		fprintf(stderr, "Failed to initialize GLAD\n");
		glfwDestroyWindow(gWindow);
		glfwTerminate();
		return false;
	}*/

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
	/*if (GLAD_GL_VERSION_4_3)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageCallback(errorCallback, 0);
	}
	else
	{
		fprintf(stderr, "OpenGL 4.3 or higher is required for debug output\n");
	}*/

	return true;
}

void deinitialize()
{
	glfwDestroyWindow(gWindow);
	glfwTerminate();
}

bool update()
{
	if (glfwWindowShouldClose(gWindow))
	{
		return false;
	}

	return true;
}

void render(unsigned int aTicks)
{
	// Clear the screen with a green color
	glClearColor(0.0f, 0.5f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float timeValue = glfwGetTime();
	float greenValue = (sin(timeValue) / 2.0f) + 0.5f;
	int vertexColorLocation = glGetUniformLocation(shaderProgram, "ourColor");

	// Use the shader program
	glUseProgram(shaderProgram);
	glUniform4f(vertexColorLocation, 0.0f, greenValue, 0.0f, 1.0f);

	// Bind the VA0
	glBindVertexArray(VAO);

	// Render the triangle
	//glDrawArrays(GL_TRIANGLES, 0, 3);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	// Unbind the VAO
	glBindVertexArray(0);

	glfwSwapBuffers(gWindow);
	glfwPollEvents();
}

void loop()
{
	if (!update())
	{
		gDone = 1;
	}
	else
	{
		render(glfwGetTime());
	}
}

void setupTriangle()
{
	// Vertex data
	float vertices[] = {
			0.5f,  0.5f, 0.0f,  // Top right
			0.5f, -0.5f, 0.0f,  // Bottom right
		-0.5f, -0.5f, 0.0f,  // Bottom left
		-0.5f,  0.5f, 0.0f   // Top left
	};
	unsigned int indices[] = {
		0, 1, 3,  // First triangle
		1, 2, 3   // Second triangle
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

	// Bind EBO and upload indices data
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Set vertex attribute pointers
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Unbind VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// Compile vertex shader
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	// Check for vertex shader compile errors
	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		fprintf(stderr, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
	}

	// Compile fragment shader
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	// Check for fragment shader compile errors
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		fprintf(stderr, "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
	}

	// Link shaders into a shader program
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	// Check for linking errors
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		fprintf(stderr, "ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
	}

	// Delete the shaders as they're linked into our program now and no longer necessary
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

int main(int argc, char** argv)
{
	if (!initialize())
	{
		return -1;
	}

	setupTriangle();

	gDone = 0;
	while (!gDone)
	{
		loop();
	}

	deinitialize();

	return 0;
}
