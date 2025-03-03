#ifndef INPUT_H
#define INPUT_H

#include <SDL3/SDL.h>

void Input_Init();
void Input_HandleEvent(SDL_Event* event);
void Input_Cleanup();

#endif // INPUT_H