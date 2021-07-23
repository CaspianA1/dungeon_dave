CC = clang
# export LIBRARY_PATH="$LIBRARY_PATH:/usr/local/lib"

MAIN = dungeon_dave
# OUT = Dungeon\ Dave.app
OUT = $(MAIN).app

OPTIMIZE = -Ofast
DEBUG = -ggdb3
DEBUG_2_FLAGS = -fsanitize=address

ifeq ($(CC),gcc-10)
	DEBUG_2_FLAGS += -fsanitize=leak # -fsanitize=memory
endif

CFLAGS = -std=c99 -march=native -Wall -Wextra -Wdouble-promotion -Wpedantic -Wformat
LIBS = -lSDL2 -lSDL2_ttf -lSDL2_mixer -lm
LDFLAGS = $(LIBS) -o bin/$(OUT) src/main/$(MAIN).c

all: build run

run:
	./bin/$(OUT)

build:
	$(CC) $(CFLAGS) $(OPTIMIZE) $(LDFLAGS)

build_debug:
	$(CC) $(CFLAGS) $(DEBUG) $(LDFLAGS)

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
	$(CC) $(CFLAGS) $(DEBUG) $(DEBUG_2_FLAGS) $(LDFLAGS)
	make run

clean:
	rm -r bin/*