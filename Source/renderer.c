#include "renderer.h"
#include <GL/glew.h>

void Renderer_Init()
{
	// Initialize OpenGL settings
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// Set up shaders, buffers, etc.
	// For example, load shaders and create vertex buffers
	// Shader_Load("vertex_shader.glsl", "fragment_shader.glsl");
	// Buffer_Create();
	SDL_Log("Renderer system initialized.");
}

void Renderer_Render()
{
	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// Render game objects
	// For example, render player, enemies, etc.
	// Player_Render();
	// Enemy_Render();
	// Swap buffers
	SDL_GL_SwapWindow(SDL_GetWindowFromID(1));
}

void Renderer_Cleanup()
{
	// Clean up OpenGL resources
	// For example, delete shaders, buffers, etc.
	// Shader_Delete();
	// Buffer_Delete();
	// Cleanup other renderer components as needed
	SDL_Log("Renderer system cleaned up.");
}
