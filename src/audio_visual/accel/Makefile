CC = clang

OPTIMIZE = -Ofast
DEBUG = -fsanitize=address
BUILD_TYPE = $(OPTIMIZE)

DEMO = 17
TEST_RANGE = 1..21

OUT_PREFIX = accel_demo_

WARNINGS = -Wall -Wextra -Wdouble-promotion -Wpedantic -Wformat
CFLAGS = -std=c99 -march=native $(WARNINGS)

NON_GL_LDFLAGS = -lm -lSDL2
LDFLAGS = -framework OpenGL -lglew $(NON_GL_LDFLAGS)

all:
	make build_demo demo_id=$(DEMO)

build_demo: # Takes a named argument `demo_id`
	$(CC) $(CFLAGS) $(BUILD_TYPE) $(LDFLAGS) -D DEMO_$(demo_id) -o bin/$(OUT_PREFIX)$(demo_id) src/demos/demo_$(demo_id).c
	./bin/$(OUT_PREFIX)$(demo_id)

test_demos: # Tests demos in the test range, stopping if one fails to build
	for i in {$(TEST_RANGE)}; do \
		if !(make build_demo demo_id=$$i &) then exit; \
		fi \
	done; \

%: # This builds any app from 'tools/', which may be `editor` or `lightmapper`
	$(CC) $(CFLAGS) $(BUILD_TYPE) $(NON_GL_LDFLAGS) -o bin/$@ src/tools/$@.c
	./bin/$@

clean:
	rm -r bin/*
