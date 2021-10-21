CC = clang
# export LIBRARY_PATH="$LIBRARY_PATH:/usr/local/lib"

MAIN = dungeon_dave
# OUT = Dungeon\ Dave.app
OUT = $(MAIN).app

OPTIMIZE = -Ofast
DEBUG = -ggdb3
DEBUG_2 = -fsanitize=address

ifeq ($(CC),gcc-10)
	DEBUG_2 += -fsanitize=leak # -fsanitize=memory
endif

WARNINGS = -Wall -Wextra -Wdouble-promotion -Wpedantic -Wformat
CFLAGS = -std=c99 -march=native $(WARNINGS)
SDL_EXTENSION_LIBS = -lSDL2_ttf -lSDL2_mixer
CORE_LIBS = -lm -framework OpenGL -lglew -lSDL2
LDFLAGS = $(CORE_LIBS) $(SDL_EXTENSION_LIBS) -o bin/$(OUT) src/main/$(MAIN).c

all: build run

run:
	./bin/$(OUT)

build:
	$(CC) $(CFLAGS) $(OPTIMIZE) $(LDFLAGS)

build_debug:
	$(CC) $(CFLAGS) $(DEBUG) $(LDFLAGS)

accel: accel_build
	./bin/accel_demo

accel_build:
	$(CC) $(CFLAGS) $(OPTIMIZE) $(CORE_LIBS) -o bin/accel_demo src/audio_visual/accel/demo_2.c

asm:
	$(CC) $(CFLAGS) -S -masm=intel $(OPTIMIZE) -o bin/$(MAIN).asm src/main/$(MAIN).c

debug: build_debug
	lldb -d bin/$(OUT)

profile_gcov:
	$(CC) $(CFLAGS) $(DEBUG) --coverage $(LDFLAGS)
	make run
	gcov -f -a $(OUT) > /dev/null
	rm *.gcda *.gcno
	mkdir -p bin/profiling
	mv *.gcov bin/profiling
	@echo Profiling data is in any .gcov file in bin/profiling.

debug_2: # finds writes to unallocated memory and such
	$(CC) $(CFLAGS) $(DEBUG) $(DEBUG_2) $(LDFLAGS)
	make run

clean:
	rm -r bin/*
