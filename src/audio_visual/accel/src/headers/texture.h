#ifndef TEXTURE_H
#define TEXTURE_H

#include "utils.h"

// These macros aren't enums because they're configurable values, whereas the enums are not

#define SDL_PIXEL_FORMAT SDL_PIXELFORMAT_BGRA32

#define OPENGL_INPUT_PIXEL_FORMAT GL_BGRA
#define OPENGL_GRAYSCALE_INTERNAL_PIXEL_FORMAT GL_RED
#define OPENGL_COLOR_CHANNEL_TYPE GL_UNSIGNED_BYTE

#ifdef USE_GAMMA_CORRECTION
#define OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT GL_SRGB_ALPHA
#else
#define OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT GL_RGBA
#endif

//////////

#define OPENGL_TEX_MAG_FILTER GL_LINEAR
#define OPENGL_TEX_MIN_FILTER GL_LINEAR_MIPMAP_LINEAR
// Mip level should not change per skybox, so no trilinear needed
#define OPENGL_SKYBOX_TEX_MIN_FILTER GL_LINEAR_MIPMAP_NEAREST
#define ENABLE_ANISOTROPIC_FILTERING

/* There's five bits to store a texture id in a face mesh's face info byte,
And the biggest number possible with five bits is 31, so that gives you
32 different possible texture ids. Also, this is just for wall textures. */
#define MAX_NUM_SECTOR_SUBTEXTURES 32

#define SECTOR_TEXTURE_UNIT 0
#define BILLBOARD_TEXTURE_UNIT 1
#define SKYBOX_TEXTURE_UNIT 2
#define WEAPON_TEXTURE_UNIT 3
#define SHADOW_MAP_TEXTURE_UNIT 4

typedef enum {
	TexPlain = GL_TEXTURE_2D,
	TexSet = GL_TEXTURE_2D_ARRAY,
	TexSkybox = GL_TEXTURE_CUBE_MAP
} TextureType;

typedef enum {
	TexRepeating = GL_REPEAT,
	TexNonRepeating = GL_CLAMP_TO_EDGE
} TextureWrapMode;

// Excluded: init_still_subtextures_in_texture_set, init_animated_subtextures_in_texture_set

#define deinit_texture(t) glDeleteTextures(1, &(t))

#define deinit_textures(length, ts) do {\
	glDeleteTextures((length), (ts));\
	free((ts));\
} while (0)

#define deinit_surface SDL_FreeSurface

SDL_Surface* init_blank_surface(const GLsizei width, const GLsizei height);
SDL_Surface* init_surface(const GLchar* const path);

void use_texture(const GLuint texture, const GLuint shader_program,
	const GLchar* const sampler_name, const TextureType type, const byte texture_unit);

GLuint preinit_texture(const TextureType type, const TextureWrapMode wrap_mode);

void write_surface_to_texture(SDL_Surface* const surface,
	const TextureType type, const GLint internal_format);

GLuint init_plain_texture(const GLchar* const path, const TextureType type,
	const TextureWrapMode wrap_mode, const GLint internal_format);

GLuint init_texture_set(const TextureWrapMode wrap_mode, const GLsizei num_still_subtextures,
	const GLsizei num_animation_sets, const GLsizei rescale_w, const GLsizei rescale_h, ...);

#endif
