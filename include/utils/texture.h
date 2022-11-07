#ifndef TEXTURE_H
#define TEXTURE_H

#include "buffer_defs.h"
#include "utils.h"
#include "animation.h"

// TODO: put these macros in the `constants` struct

#define SDL_PIXEL_FORMAT SDL_PIXELFORMAT_BGRA32
#define OPENGL_INPUT_PIXEL_FORMAT GL_BGRA

#define OPENGL_NORMAL_MAP_INTERNAL_PIXEL_FORMAT GL_RGBA
#define OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT GL_SRGB8_ALPHA8

#define OPENGL_SHADOW_MAP_COLOR_CHANNEL_TYPE GL_FLOAT
#define OPENGL_COLOR_CHANNEL_TYPE GL_UNSIGNED_BYTE

//////////

#define OPENGL_SCENE_MAG_FILTER TexLinear
#define OPENGL_SCENE_MIN_FILTER TexTrilinear

/* There's five bits to store a texture id in a face mesh's face info byte,
And the biggest number possible with five bits is 31, so that gives you
32 different possible texture ids. Also, this is just for wall textures. */
#define MAX_NUM_SECTOR_SUBTEXTURES ((byte) 32u)

// Each enum value is a texture unit id.
typedef enum {
	TU_Temporary = 0, // This can be reused for various purposes

	TU_Skybox,

	TU_AmbientOcclusionMap,
	TU_CascadedShadowMapPlain,
	TU_CascadedShadowMapDepthComparison,

	TU_SectorFaceDiffuse, TU_SectorFaceNormalMap,
	TU_BillboardDiffuse, TU_BillboardNormalMap,
	TU_WeaponSpriteDiffuse, TU_WeaponSpriteNormalMap,

	TU_TitleScreenStillDiffuse,
	TU_TitleScreenScrollingDiffuse,
	TU_TitleScreenScrollingNormalMap
} TextureUnit;

// Excluded: premultiply_surface_alpha, init_still_subtextures_in_texture_set, init_animated_subtextures_in_texture_set

#define WITH_SURFACE_PIXEL_ACCESS(surface, ...) do {\
	const bool must_lock = SDL_MUSTLOCK((surface));\
	if (must_lock) SDL_LockSurface((surface));\
	__VA_ARGS__\
	if (must_lock) SDL_UnlockSurface((surface));\
} while (false)

//////////

typedef enum {
	TexPlain = GL_TEXTURE_2D,
	TexSkybox = GL_TEXTURE_CUBE_MAP,
	TexSet = GL_TEXTURE_2D_ARRAY,
	TexVolumetric = GL_TEXTURE_3D
} TextureType;

typedef enum {
	TexRepeating = GL_REPEAT,
	TexNonRepeating = GL_CLAMP_TO_EDGE,
	TexBordered = GL_CLAMP_TO_BORDER
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

SDL_Surface* init_blank_surface(const GLsizei width, const GLsizei height);
SDL_Surface* init_surface(const GLchar* const path);

void* read_surface_pixel(const SDL_Surface* const surface, const GLint x, const GLint y);

void use_texture_in_shader(const GLuint texture,
	const GLuint shader, const GLchar* const sampler_name,
	const TextureType type, const TextureUnit texture_unit);

GLuint preinit_texture(const TextureType type, const TextureWrapMode wrap_mode,
	const TextureFilterMode mag_filter, const TextureFilterMode min_filter,
	const bool force_disable_aniso_filtering);

/* Size formats:
Plain 2D textures: {width, height}
Skyboxes: {width/height, face index}
Texture sets/volumetric textures: {width, height, depth} */
void init_texture_data(const TextureType type, const GLsizei* const size,
	const GLenum input_format, const GLint internal_format, const GLenum color_channel_type,
	const void* const pixels);

GLuint init_texture_set(const bool premultiply_alpha,
	const TextureWrapMode wrap_mode, const TextureFilterMode mag_filter,
	const TextureFilterMode min_filter, const GLsizei num_still_subtextures,
	const GLsizei num_animation_layouts, const GLsizei rescale_w, const GLsizei rescale_h,
	const GLchar* const* const still_subtexture_paths, const AnimationLayout* const animation_layouts);

GLuint init_plain_texture(const GLchar* const path,
	const TextureWrapMode wrap_mode, const TextureFilterMode mag_filter,
	const TextureFilterMode min_filter, const GLint internal_format);

#endif
