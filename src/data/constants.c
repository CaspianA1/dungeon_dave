#include "data/constants.h"
#include "utils/utils.h"

// The return type is GLfloat, in order to be flexible for both integers and floats
GLfloat get_runtime_constant(const RuntimeConstantName runtime_constant_name) {
	static GLint refresh_rate;
	static GLfloat aniso_filtering_level;

	ON_FIRST_CALL(
		SDL_DisplayMode display_mode;
		SDL_GetCurrentDisplayMode(0, &display_mode);

		refresh_rate = (display_mode.refresh_rate == 0)
			? constants.window.default_fps : display_mode.refresh_rate;

		GLfloat max_aniso_filtering_level;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso_filtering_level);

		// If the filtering level defined in `constants` exceeds the max supported one, the max one will be chosen
		aniso_filtering_level = fminf(max_aniso_filtering_level, constants.lighting.aniso_filtering_level);
	);

	switch (runtime_constant_name) {
		case RefreshRate: return refresh_rate;
		case AnisotropicFilteringLevel: return aniso_filtering_level;
	}
}
