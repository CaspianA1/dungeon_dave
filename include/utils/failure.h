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
	UseDict,

	OpenFile,
	CreateSurface,
	CreateTexture,
	CreateFramebuffer,
	CreateAudioBuffer,
	CreateAudioSource,
	CreateFormatString,

	ParseJSON,
	ReadFromJSON,

	MakeBillboard,

	CreateShader,
	ParseIncludeDirectiveInShader,
	InitializeShaderUniform,

	InitializeGPUMemoryMapping,

	UseLevelHeightmap,
	CreateLevel
} FailureType;

void print_failure_message(const char* const failure_type_string,
	const char* const format_string, const char* const filename, const int line_number, ...);

#define FAIL(failure_type, format_string, ...) do {\
	print_failure_message(#failure_type, format_string, __FILE__, __LINE__, __VA_ARGS__);\
	exit((int) failure_type + 1);\
} while (false)

#endif
