#include <SDL3/SDL.h>
#include <GL/glew.h>
#include "asset_manager.h"
#include "game.h"
#include "renderer.h"
#include "input.h"

int main(int argc, char* argv[])
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
		return -1;
	}

	SDL_Window* window = SDL_CreateWindow("3D Isometric Game", 800, 600, SDL_WINDOW_OPENGL);
	if (!window)
	{
		SDL_Log("Failed to create window: %s", SDL_GetError());
		SDL_Quit();
		return -1;
	}

	SDL_GLContext glContext = SDL_GL_CreateContext(window);
	if (!glContext)
	{
		SDL_Log("Failed to create OpenGL context: %s", SDL_GetError());
		SDL_DestroyWindow(window);
		SDL_Quit();
		return -1;
	}

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		SDL_Log("Failed to initialize GLEW");
		SDL_GL_DestroyContext(glContext);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return -1;
	}

	// Initialize asset_manager, game, renderer, and input
	AssetManager_Init();
	Game_Init();
	Renderer_Init();
	Input_Init();

	bool running = true;
	while (running)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
			{
				running = false;
			}
			Input_HandleEvent(&event);
		}

		Game_Update();
		Renderer_Render();

		SDL_GL_SwapWindow(window);
	}

	// Cleanup
	AssetManager_Cleanup();
	Renderer_Cleanup();
	Game_Cleanup();
	SDL_GL_DestroyContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}