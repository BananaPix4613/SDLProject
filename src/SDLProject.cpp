#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

int* gFrameBuffer;
int* gTempBuffer;
SDL_Window* gSDLWindow;
SDL_Renderer* gSDLRenderer;
SDL_Texture* gSDLTexture;
static int gDone;
const int WINDOW_WIDTH = 1920 / 2;
const int WINDOW_HEIGHT = 1080 / 2;

bool initializeWindow(const char* title, int width, int height)
{
	// Initialize SDL
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
	{
		SDL_Log("SDL initialization failed: %s", SDL_GetError());
		return false;
	}

	// Set OpenGL attributes before window creation
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

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
	SDL_GLContext gl_context = SDL_GL_CreateContext(gSDLWindow);
	if (!gl_context) {
		SDL_Log("OpenGL context creation failed: %s", SDL_GetError());
		SDL_DestroyWindow(gSDLWindow);
		SDL_Quit();
		return false;
	}

	return true;
}

bool initialize()
{
	initializeWindow("SDL3 window", WINDOW_WIDTH, WINDOW_HEIGHT);

	return true;
}

void deinitialize()
{
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
	initialize();
	gDone = 0;
	while (!gDone)
	{
		loop();
	}
	deinitialize();
	SDL_Quit();

	return 0;
}
