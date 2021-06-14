CC = clang

MAIN = dungeon_dave
# OUT = Dungeon\ Dave.app
OUT = $(MAIN).app

OPTIMIZE = -Ofast
DEBUG = -ggdb3

CFLAGS = -march=native -Wall -Wdouble-promotion -Wformat -Wpedantic -Wextra
LIBS = -lSDL2 -lSDL2_ttf -lSDL2_mixer -lm -lpthread
LDFLAGS = $(LIBS) -o bin/$(OUT) src/main/$(MAIN).c

all: build_optimized run

run:
	./bin/$(OUT)

build_optimized:
	$(CC) $(CFLAGS) $(OPTIMIZE) $(LDFLAGS)

build_debug:
	$(CC) $(CFLAGS) $(DEBUG) $(LDFLAGS)

asm:
	$(CC) $(CFLAGS) -S -masm=intel $(OPTIMIZE) -o bin/$(MAIN).asm src/main/$(MAIN).c

debug: build_debug
	lldb -d bin/$(OUT)

profile:
	$(CC) $(CFLAGS) $(DEBUG) --coverage $(LDFLAGS)
	make run
	gcov -f -a $(OUT) > /dev/null
	rm *.gcda *.gcno
	mkdir -p bin/profiling
	mv *.gcov bin/profiling
	@echo Profiling data is in any .gcov file in bin/profiling.

find_leaks:
	$(CC) $(CFLAGS) $(DEBUG) -fsanitize=address $(LDFLAGS)
	make run

clean:
	rm -r bin/*