#ifndef SDL_INCLUDE_H
#define SDL_INCLUDE_H

// If "SDL2/SDL.h" isn't found, try "SDL.h"
#if __has_include("SDL2/SDL.h")
	#include <SDL2/SDL.h>
#else
	#include <SDL.h>
#endif

#endif
