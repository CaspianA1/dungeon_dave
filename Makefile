########## Defining various compiler details, like optimization, debug mode, and warnings

CC = clang

OPTIMIZE = -Ofast
DEBUG = -fsanitize=address,undefined
BUILD_TYPE = $(DEBUG)

CONVERSION_WARNINGS = -Wfloat-conversion -Wdouble-promotion -Wsign-conversion\
	-Wimplicit-int-conversion -Wshorten-64-to-32 -Wincompatible-pointer-types

WARNINGS = -Wall -Wextra -Wpedantic -Wformat $(CONVERSION_WARNINGS)

CFLAGS := `pkg-config --cflags sdl2` -std=c99 $(WARNINGS)
NON_GL_LDFLAGS := -ldl -lm -lcjson `pkg-config --libs sdl2`

##########

ifeq ($(shell uname), Darwin) # If on MacOS
	GL_LDFLAGS = -framework OpenGL
else
	GL_LDFLAGS = -lGL
endif

########## Rules

.PHONY: all clean

OUT = dungeon_dave

SRC_DIR = src
HEADER_DIR = $(SRC_DIR)/headers
OBJ_DIR = obj
BIN_DIR = bin
GLAD_DIR = include/glad

OBJS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(SRC_DIR)/*.c))

########## Rules for the main project

all: $(BIN_DIR)/$(OUT)
	cd bin && ./$(OUT) && cd ..

$(BIN_DIR)/$(OUT): $(OBJS) $(OBJ_DIR)/glad.o
	$(CC) $(BUILD_TYPE) $(NON_GL_LDFLAGS) $(GL_LDFLAGS) -o $@ $^

$(OBJ_DIR)/glad.o: $(GLAD_DIR)/*
	$(CC) -c $(BUILD_TYPE) $(CFLAGS) -o $@ $(GLAD_DIR)/glad.c

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADER_DIR)/%.h
	$(CC) -c $(BUILD_TYPE) $(CFLAGS) -o $@ $(SRC_DIR)/$*.c

########## The rule for the editor + the clean rule

editor:
	$(CC) $(BUILD_TYPE) $(CFLAGS) $(NON_GL_LDFLAGS) -o $(BIN_DIR)/editor $(SRC_DIR)/editor/editor.c
	cd $(BIN_DIR) && ./editor

clean:
	rm $(OBJ_DIR)/*.o $(BIN_DIR)/*
