#ifndef CONSTANTS_C
#define CONSTANTS_C

#include "headers/constants.h"
#include "headers/utils.h"

// The return type is GLfloat, in order to be flexible for both integers and floats
GLfloat get_runtime_constant(const RuntimeConstantName runtime_constant_name) {
	static GLfloat aniso_filtering_level;
	ON_FIRST_CALL(glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso_filtering_level););

	switch (runtime_constant_name) {
		case RefreshRate: {
			SDL_DisplayMode display_mode;
			SDL_GetCurrentDisplayMode(0, &display_mode);

			return (display_mode.refresh_rate == 0)
				? constants.default_fps : display_mode.refresh_rate;
		}
		case AnisotropicFilteringLevel:
			return aniso_filtering_level;
	}
}

#endif