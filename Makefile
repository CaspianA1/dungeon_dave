CC = clang

OPTIMIZE = -Ofast
DEBUG = -fsanitize=address,undefined
BUILD_TYPE = $(DEBUG)

OUT = dungeon_dave

CONVERSION_WARNINGS = -Wfloat-conversion -Wdouble-promotion -Wsign-conversion -Wimplicit-int-conversion -Wshorten-64-to-32 -Wincompatible-pointer-types
WARNINGS = -Wall -Wextra -Wpedantic -Wformat $(CONVERSION_WARNINGS)
CFLAGS = -std=c99 $(WARNINGS)

NON_GL_LDFLAGS = $$(pkg-config --cflags --libs sdl2) -lm

ifeq ($(shell uname), Darwin) # If on MacOS
	GL_LDFLAGS = -framework OpenGL
else
	GL_LDFLAGS = -lGL
endif

all: build
	cd bin && ./$(OUT)

build:
	$(CC) $(CFLAGS) $(BUILD_TYPE) $(NON_GL_LDFLAGS) $(GL_LDFLAGS) -o bin/$(OUT) src/main.c

%:
	$(CC) $(CFLAGS) $(BUILD_TYPE) $(NON_GL_LDFLAGS) -o bin/$@ src/editor/$@.c
	cd bin && ./$@

clean:
	rm -r bin/*
