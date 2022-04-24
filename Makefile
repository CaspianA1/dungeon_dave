CC = clang

OPTIMIZE = -Ofast
DEBUG = -fsanitize=address,undefined
BUILD_TYPE = $(DEBUG)

OUT = dungeon_dave

CONVERSION_WARNINGS = -Wfloat-conversion -Wdouble-promotion -Wsign-conversion -Wimplicit-int-conversion -Wshorten-64-to-32 -Wincompatible-pointer-types
WARNINGS = -Wall -Wextra -Wpedantic -Wformat $(CONVERSION_WARNINGS)
CFLAGS = -c -std=c99 $(WARNINGS)

NON_GL_LDFLAGS = $$(pkg-config --cflags --libs sdl2) -lm

ifeq ($(shell uname), Darwin) # If on MacOS
	GL_LDFLAGS = -framework OpenGL
else
	GL_LDFLAGS = -lGL
endif

########## Rules

.PHONY: all clean

SRC_DIR = src
BIN_DIR = bin

OBJS := $(patsubst $(SRC_DIR)/%.c, %.o, $(wildcard $(SRC_DIR)/*.c))

all: $(OBJS) glad
	$(CC) $(BUILD_TYPE) $(NON_GL_LDFLAGS) $(GL_LDFLAGS) $(BIN_DIR)/*.o -o $(BIN_DIR)/$(OUT)
	cd $(BIN_DIR) && ./$(OUT)

$(OBJS):
	$(CC) $(CFLAGS) $(BUILD_TYPE) -o $(BIN_DIR)/$*.o $(SRC_DIR)/$*.c

glad:
	$(CC) $(CFLAGS) $(BUILD_TYPE) -o $(BIN_DIR)/glad.o include/glad/glad.c

##########

editor:
	$(CC) $(CFLAGS) $(BUILD_TYPE) $(NON_GL_LDFLAGS) -o $(BIN_DIR)/editor $(SRC_DIR)/editor/editor.c
	cd $(BIN_DIR) && ./editor

clean:
	rm -r $(BIN_DIR)/*.o