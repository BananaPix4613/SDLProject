#include "asset_manager.h"
#include <stdlib.h>
#include <string.h>

static Asset* assets = NULL;
static size_t assetCount = 0;

void AssetManager_Init()
{
	// Initialize asset manager
	assetCount = 0;
	assets = NULL;
	SDL_Log("Asset manager system initialized.");
}

void AssetManager_Cleanup()
{
	// Cleanup assets
	for (size_t i = 0; i < assetCount; ++i)
	{
		if (assets[i].texture)
		{
			SDL_DestroyTexture(assets[i].texture);
		}
		if (assets[i].model)
		{
			glDeleteBuffers(1, &assets[i].model);
		}
	}
	free(assets);
	SDL_Log("Asset manager system cleaned up.");
}

Asset* AssetManager_LoadTexture(const char* filePath)
{
	// Load texture and add to assets
	//SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, SDL_LoadBMP(filePath));
	//if (!texture)
	//{
	//	SDL_Log("Failed to load texture: %s", SDL_GetError());
	//	return NULL;
	//}

	//assets = realloc(assets, (assetCount + 1) * sizeof(Asset));
	//assets[assetCount].texture = texture;
	//assets[assetCount].model = 0; // Not a model
	//return &assets[assetCount++];
	return NULL; // Placeholder for now
}

Asset* AssetManager_LoadModel(const char* filePath)
{
	// Load model and add to assets
	//GLuint model = LoadModelFromFile(filePath); // Implement this function
	//if (!model)
	//{
	//	SDL_Log("Failed to load model");
	//	return NULL;
	//}

	//Asset* temp = realloc(assets, (assetCount + 1) * sizeof(Asset));
	//if (!temp)
	//{
	//	SDL_Log("Failed to allocate memory for assets");
	//	return NULL;
	//}
	//assets = temp;

	//assets[assetCount].texture = 0; // Not a texture
	//assets[assetCount].model = model; 
	//return &assets[assetCount++];
	return NULL; // Placeholder for now
}
