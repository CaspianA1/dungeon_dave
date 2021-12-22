#ifndef TEXTURE_H
#define TEXTURE_H

#include "utils.h"

#define SDL_PIXEL_FORMAT SDL_PIXELFORMAT_BGRA32
#define OPENGL_INPUT_PIXEL_FORMAT GL_BGRA
#define OPENGL_INTERNAL_PIXEL_FORMAT GL_RGBA
#define OPENGL_COLOR_CHANNEL_TYPE GL_UNSIGNED_BYTE

#define OPENGL_TEX_MAG_FILTER GL_LINEAR
#define OPENGL_TEX_MIN_FILTER GL_LINEAR_MIPMAP_LINEAR
// Mip level should not change per skybox, so no trilinear needed
#define OPENGL_SKYBOX_TEX_MIN_FILTER GL_LINEAR_MIPMAP_NEAREST
#define ENABLE_ANISOTROPIC_FILTERING

#define deinit_texture(t) glDeleteTextures(1, &t)

#define deinit_textures(length, ts) do {\
	glDeleteTextures(length, ts);\
	free(ts);\
} while (0)

/* There's five bits to store a texture id in a face mesh's face info byte,
And the biggest number possible with five bits is 31, so that gives you
32 different possible texture ids. */
#define MAX_NUM_TEXTURES 32

typedef enum {
	TexPlain = GL_TEXTURE_2D,
	TexBillboard = GL_TEXTURE_2D,
	TexAnimation = GL_TEXTURE_2D_ARRAY,
	TexSet = GL_TEXTURE_2D_ARRAY,
	TexSkybox = GL_TEXTURE_CUBE_MAP,
} TextureType;

typedef enum {
	TexRepeating = GL_REPEAT,
	TexNonRepeating = GL_CLAMP_TO_EDGE
} TextureWrapMode;

typedef struct { // An animation FPS would be global
	byte curr_frame;
	const byte total_frames;
	const GLuint texture;
	float last_frame_time;
} AnimationInstance;

// Excluded: init_blank_surface

SDL_Surface* init_surface(const char* const path);
void deinit_surface(SDL_Surface* const surface);

GLuint preinit_texture(const TextureType texture_type, const TextureWrapMode wrap_mode);
void use_texture(const GLuint texture, const GLuint shader_program, const TextureType texture_type);

void write_surface_to_texture(const SDL_Surface* const surface, const GLenum opengl_texture_type);
GLuint* init_plain_textures(const GLsizei num_textures, ...);

GLuint init_animation(const char* const path, const GLsizei frames_across,
	const GLsizei frames_down, const GLsizei total_frames);

GLuint init_texture_set(const TextureWrapMode wrap_mode,
	const GLsizei subtex_width, const GLsizei subtex_height, const GLsizei num_textures, ...);

#endif
