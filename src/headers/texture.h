#ifndef TEXTURE_H
#define TEXTURE_H

#include "utils.h"
#include "buffer_defs.h"
#include "animation.h"

// These macros aren't enums because they're configurable values, whereas the enums are not

#define SDL_PIXEL_FORMAT SDL_PIXELFORMAT_BGRA32

#define OPENGL_INPUT_PIXEL_FORMAT GL_BGRA

#define OPENGL_NORMAL_MAP_INTERNAL_PIXEL_FORMAT GL_RGBA
#define OPENGL_COLOR_CHANNEL_TYPE GL_UNSIGNED_BYTE

#ifdef USE_GAMMA_CORRECTION
#define OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT GL_SRGB_ALPHA
#else
#define OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT GL_RGBA
#endif

//////////

#define OPENGL_WEAPON_MAG_FILTER TexLinear
#define OPENGL_WEAPON_MIN_FILTER TexLinearMipmapped

#define OPENGL_SCENE_MAG_FILTER TexLinear
#define OPENGL_SCENE_MIN_FILTER TexTrilinear

/* Mip level should not change per skybox, so no trilinear needed.
The skybox mag filter is the scene mag filter. */
#define OPENGL_SKYBOX_MIN_FILTER TexLinearMipmapped

#define OPENGL_SHADOW_MAP_MAG_FILTER TexLinear
#define OPENGL_SHADOW_MAP_MIN_FILTER TexTrilinear

/* There's five bits to store a texture id in a face mesh's face info byte,
And the biggest number possible with five bits is 15, so that gives you
16 different possible texture ids. Also, this is just for wall textures. */
#define MAX_NUM_SECTOR_SUBTEXTURES 16

// TODO: make this to an enum
#define SECTOR_FACE_TEXTURE_UNIT 0
#define SECTOR_NORMAL_MAP_TEXTURE_UNIT 1
#define SHADOW_MAP_TEXTURE_UNIT 2
#define BILLBOARD_TEXTURE_UNIT 3
#define SKYBOX_TEXTURE_UNIT 4
#define WEAPON_TEXTURE_UNIT 5
#define TITLE_SCREEN_LOGO_TEXTURE_UNIT 6

// Excluded: init_still_subtextures_in_texture_set, init_animated_subtextures_in_texture_set

#define set_current_texture glBindTexture

#define deinit_texture(t) glDeleteTextures(1, &(t))
#define deinit_textures(length, ts) glDeleteTextures((length), (ts))
#define deinit_surface SDL_FreeSurface

#define WITH_SURFACE_PIXEL_ACCESS(surface, ...) do {\
	const bool must_lock = SDL_MUSTLOCK((surface));\
	if (must_lock) SDL_LockSurface((surface));\
	__VA_ARGS__\
	if (must_lock) SDL_UnlockSurface((surface));\
} while (0)

//////////

typedef enum {
	TexPlain = GL_TEXTURE_2D,
	TexSet = GL_TEXTURE_2D_ARRAY,
	TexSkybox = GL_TEXTURE_CUBE_MAP
} TextureType;

typedef enum {
	TexRepeating = GL_REPEAT,
	TexNonRepeating = GL_CLAMP_TO_EDGE
} TextureWrapMode;

typedef enum {
	TexNearest = GL_NEAREST,
	TexLinear = GL_LINEAR,
	TexLinearMipmapped = GL_LINEAR_MIPMAP_NEAREST,
	TexTrilinear = GL_LINEAR_MIPMAP_LINEAR
} TextureFilterMode;

typedef Uint8 sdl_pixel_component_t;
typedef Uint32 sdl_pixel_t;

//////////

SDL_Surface* init_blank_surface(const GLsizei width, const GLsizei height, const SDL_PixelFormatEnum pixel_format_name);
SDL_Surface* init_surface(const GLchar* const path);

void set_sampler_texture_unit_for_shader(const GLchar* const sampler_name, const GLuint shader, const byte texture_unit);
void set_current_texture_unit(const byte texture_unit);

void use_texture(const GLuint texture, const GLuint shader,
	const GLchar* const sampler_name, const TextureType type, const byte texture_unit);

GLuint preinit_texture(const TextureType type, const TextureWrapMode wrap_mode,
	const TextureFilterMode mag_filter, const TextureFilterMode min_filter);

void write_surface_to_texture(SDL_Surface* const surface,
	const TextureType type, const GLint internal_format);

GLuint init_plain_texture(const GLchar* const path, const TextureType type,
	const TextureWrapMode wrap_mode, const TextureFilterMode mag_filter,
	const TextureFilterMode min_filter, const GLint internal_format);

GLuint init_texture_set(const TextureWrapMode wrap_mode, const TextureFilterMode mag_filter,
	const TextureFilterMode min_filter, const GLsizei num_still_subtextures,
	const GLsizei num_animation_layouts, const GLsizei rescale_w, const GLsizei rescale_h,
	const GLchar* const* const still_subtexture_paths, const AnimationLayout* const animation_layouts);

#endif
