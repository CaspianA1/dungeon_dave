#ifndef FAILURE_H
#define FAILURE_H

#include <stdio.h> // For `fprintf` and `stderr`
#include <stdlib.h> // For `exit`
#include <stdbool.h> // For `false`

typedef enum {
	LoadSDL,
	LoadOpenGL,

	InvalidTextureID,
	InitializeMaterial,

	OpenFile,
	CreateSurface,
	CreateTexture,
	CreateFramebuffer,

	CreateShader,
	ParseIncludeDirectiveInShader,
	InitializeShaderUniform,

	InitializeGPUMemoryMapping,

	UseLevelHeightmap
} FailureType;

#define FAIL(failure_type, format, ...) do {\
	fprintf(stderr, "Failed with error type '%s'. Reason: '"\
		format "'.\n", #failure_type, __VA_ARGS__);\
	exit(failure_type + 1);\
} while (false)

#endif
