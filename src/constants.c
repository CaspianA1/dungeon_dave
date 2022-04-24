#ifndef CONSTANTS_C
#define CONSTANTS_C

#include "headers/constants.h"

// The return type is GLfloat, in order to be flexible for both integers and floats
GLfloat get_runtime_constant(const RuntimeConstantName runtime_constant_name) {
	static GLfloat aniso_filtering_level;
	static bool first_call = true;

	if (first_call) {
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso_filtering_level);
		first_call = false;
	}

	switch (runtime_constant_name) {
		case RefreshRate: {
			SDL_DisplayMode display_mode;
			SDL_GetCurrentDisplayMode(0, &display_mode);
			return display_mode.refresh_rate;
		}
		case AnisotropicFilteringLevel:
			return aniso_filtering_level;
	}
}

#endif
