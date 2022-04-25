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

########## Rules

.PHONY: all clean

SRC_DIR = src
HEADER_DIR = $(SRC_DIR)/headers
OBJ_DIR = obj
BIN_DIR = bin
GLAD_DIR = include/glad

OBJS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(SRC_DIR)/*.c))

all: $(OBJS) $(OBJ_DIR)/glad.o

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADER_DIR)/%.h
	$(CC) -c $(CFLAGS) $(BUILD_TYPE) -o $@ $(SRC_DIR)/$*.c

$(OBJ_DIR)/glad.o: $(GLAD_DIR)/*
	$(CC) -c $(CFLAGS) $(BUILD_TYPE) -o $@ $(GLAD_DIR)/glad.c

##########

editor:
	$(CC) $(CFLAGS) $(BUILD_TYPE) $(NON_GL_LDFLAGS) -o $(BIN_DIR)/editor $(SRC_DIR)/editor/editor.c
	cd $(BIN_DIR) && ./editor

clean:
	rm $(OBJ_DIR)/*.o $(BIN_DIR)/$(OUT)
