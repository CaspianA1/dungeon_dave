#ifndef FAILURE_H
#define FAILURE_H

#include <stdio.h> // For `fprintf` and `stderr`
#include <stdlib.h> // For `exit`
#include <stdbool.h> // For `false`

typedef enum {
	LoadSDL,
	LoadOpenGL,
	LoadOpenAL,

	InvalidTextureID,
	InitializeMaterial,

	OpenFile,
	CreateSurface,
	CreateTexture,
	CreateFramebuffer,
	CreateAudioBuffer,
	CreateAudioSource,
	UseAudioSource,

	CreateShader,
	ParseIncludeDirectiveInShader,
	InitializeShaderUniform,

	InitializeGPUMemoryMapping,

	UseLevelHeightmap
} FailureType;

#define FAIL(failure_type, format, ...) do {\
	fprintf(stderr, "Failed with error type '%s' in source file '%s' on line %d. Reason: '"\
		format "'.\n", #failure_type, __FILE__, __LINE__, __VA_ARGS__);\
	exit(failure_type + 1);\
} while (false)

#endif
