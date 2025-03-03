#include "input.h"

void Input_Init()
{
	// Initialize input handling here if needed
	// For example, you can set up key mappings or initialize input devices
	SDL_Log("Input system initialized.");
}

void Input_HandleEvent(SDL_Event* event)
{
	if (event->type == SDL_EVENT_KEY_DOWN)
	{
		SDL_Log("Key pressed: %s", SDL_GetKeyName(event->key.key));
	}
	else if (event->type == SDL_EVENT_KEY_UP)
	{
		SDL_Log("Key released: %s", SDL_GetKeyName(event->key.key));
	}
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
	{
		SDL_Log("Mouse button pressed: %d", event->button.button);
	}
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP)
	{
		SDL_Log("Mouse button released: %d", event->button.button);
	}
	else if (event->type == SDL_EVENT_MOUSE_MOTION)
	{
		SDL_Log("Mouse moved to: (%d, %d)", (int)event->motion.x, (int)event->motion.y);
	}
}

void Input_Cleanup()
{
	// Clean up input handling here if needed
	SDL_Log("Input system cleaned up.");
}
