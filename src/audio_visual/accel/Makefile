CC = clang

OPTIMIZE = -Ofast
DEBUG = -fsanitize=address
BUILD_TYPE = $(DEBUG)

DEMO = 17
TEST_RANGE = 1..21

OUT = accel_demo

WARNINGS = -Wall -Wextra -Wdouble-promotion -Wpedantic -Wformat
CFLAGS = -std=c99 -march=native $(WARNINGS)

NON_GL_LDFLAGS = -lm -lSDL2
LDFLAGS = -framework OpenGL -lglew $(NON_GL_LDFLAGS)

all:
	make build_demo demo_id=$(DEMO) binary_name=$(OUT)

build_demo: # Takes named arguments `demo_id` and `binary_name`
	$(CC) $(CFLAGS) $(BUILD_TYPE) $(LDFLAGS) -D DEMO_$(demo_id) -o bin/$(binary_name) src/demos/demo_$(demo_id).c
	./bin/$(binary_name)

test_demos: # Tests demos in the test range, stopping if one fails to build
	for i in {$(TEST_RANGE)}; do \
		make build_demo demo_id=$$i binary_name=$(OUT)_$$i & \
	done

%: # This builds any app from 'tools/', which may be `editor` or `lightmapper`
	$(CC) $(CFLAGS) $(BUILD_TYPE) $(NON_GL_LDFLAGS) -o bin/$@ src/tools/$@.c
	./bin/$@

clean:
	rm -r bin/*
