#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_timer.h>

#include "emulator.h"

int pollEvent();
void createWindow();
void closeWindow();
void* drawScreen(void* p);

#endif
