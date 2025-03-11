#include <windows.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <GLAD/glad.h>
#include <GLFW/glfw3.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

int* gFrameBuffer;
int* gTempBuffer;
SDL_Window* gSDLWindow;
SDL_Renderer* gSDLRenderer;
SDL_Texture* gSDLTexture;
SDL_GLContext gl_context;
static int gDone;
const int WINDOW_WIDTH = 1920 / 2;
const int WINDOW_HEIGHT = 1080 / 2;

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
	// Initialize SDL
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
	{
		SDL_Log("SDL initialization failed: %s", SDL_GetError());
		return false;
	}

	// Set OpenGL attributes before window creation
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) < 0 ||
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3) < 0 ||
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE) < 0 ||
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) < 0 ||
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24) < 0)
	{
		SDL_Log("Failed to set OpenGL attributes: %s", SDL_GetError());
		SDL_Quit();
		return false;
	}

	// Create window with SDL_WINDOW_RESIZABLE flag
	gSDLWindow = SDL_CreateWindow(
		title,                         // Window title
		width,                         // Width
		height,                        // Height
		SDL_WINDOW_OPENGL |            // OpenGL context
		SDL_WINDOW_RESIZABLE |         // Resizable window
		SDL_WINDOW_HIGH_PIXEL_DENSITY  // High DPI support
	);

	if (!gSDLWindow) {
		SDL_Log("Window creation failed: %s", SDL_GetError());
		SDL_Quit();
		return false;
	}

	// Create OpenGL context
	gl_context = SDL_GL_CreateContext(gSDLWindow);
	if (!gl_context) {
		SDL_Log("OpenGL context creation failed: %s", SDL_GetError());
		SDL_DestroyWindow(gSDLWindow);
		SDL_Quit();
		return false;
	}

	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
	{
		SDL_Log("Failed to initialize GLAD");
		SDL_GL_DestroyContext(gl_context);
		SDL_DestroyWindow(gSDLWindow);
		SDL_Quit();
		return false;
	}

	// Enable VSync (optional)
	if (SDL_GL_SetSwapInterval(1) < 0)
	{
		SDL_Log("Failed to set VSync: %s", SDL_GetError());
		// Continue without VSync
	}

	// Setup OpenGL viewport
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		SDL_Log("Failed to set viewport: %s", getGLErrorString(error));
		SDL_GL_DestroyContext(gl_context);
		SDL_DestroyWindow(gSDLWindow);
		SDL_Quit();
		return false;
	}

	// Enable depth testing
    glEnable(GL_DEPTH_TEST);
	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		SDL_Log("Failed to enable depth test: %s", getGLErrorString(error));
		SDL_GL_DestroyContext(gl_context);
		SDL_DestroyWindow(gSDLWindow);
		SDL_Quit();
		return false;
	}

	return true;
}

bool initialize()
{
	// Set before context creation
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

	if (!initializeWindow("SDL3 window", WINDOW_WIDTH, WINDOW_HEIGHT))
	{
		return false;
	}

	// Set after context creation
	int major, minor;
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
	printf("OpenGL %d.%d context created\n", major, minor);

	// Get OpenGL vendor and renderer info
	const GLubyte* vendor = glGetString(GL_VENDOR);
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

	if (!vendor || !renderer || !version || !glslVersion)
	{
		SDL_Log("Failed to get OpenGL info");
		SDL_GL_DestroyContext(gl_context);
		SDL_DestroyWindow(gSDLWindow);
		SDL_Quit();
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
		SDL_Log("OpenGL 4.3 or higher is required for debug output");
	}

	return true;
}

void deinitialize()
{
	SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(gSDLWindow);
}

bool update()
{
	SDL_Event e;
	if (SDL_PollEvent(&e))
	{
		if (e.type == SDL_EVENT_QUIT)
		{
			return false;
		}
		if (e.type == SDL_EVENT_KEY_UP && e.key.key == SDLK_ESCAPE)
		{
			return false;
		}
	}

	return true;
}

void render(Uint64 aTicks)
{
	// Clear the screen with a green color
	//for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++)
	//	gFrameBuffer[i] = 0xff005f00;
}

void loop()
{
	if (!update())
	{
		gDone = 1;
	}
	else
	{
		render(SDL_GetTicks());
	}
}

int main(int argc, char** argv)
{
	if (!initialize())
	{
		return -1;
	}
	gDone = 0;
	while (!gDone)
	{
		loop();
	}
	deinitialize();
	SDL_Quit();

	return 0;
}
