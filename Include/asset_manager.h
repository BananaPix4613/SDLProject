#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include <SDL3/SDL.h>
#include <gl/glew.h>

typedef struct
{
	SDL_Texture* texture;
	GLuint model;
	// Add other asset types as needed
} Asset;

void AssetManager_Init();
void AssetManager_Cleanup();
Asset* AssetManager_LoadTexture(const char* filePath);
Asset* AssetManager_LoadModel(const char* filePath);
// Add other asset loading functions as needed

#endif // ASSET_MANAGER_H