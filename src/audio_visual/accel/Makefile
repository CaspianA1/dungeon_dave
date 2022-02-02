CC = clang

OUT = accel_demo

#####

OPTIMIZE = -Ofast
DEBUG = -fsanitize=address,undefined
BUILD_TYPE = $(DEBUG)

DEMO = 17
TEST_RANGE = 1..21

WARNINGS = -Wall -Wextra -Wdouble-promotion -Wpedantic -Wformat
CFLAGS = -std=c99 -march=native $(WARNINGS)

NON_GL_LDFLAGS = $$(pkg-config --cflags --libs sdl2) -lm
GL_LDFLAGS = -lGLEW

ifeq (uname, Darwin)
GL_LDFLAGS += -framework OpenGL
else
GL_LDFLAGS += -lGL
endif

all:
	make build_demo demo_id=$(DEMO) binary_name=$(OUT)

build_demo: # Takes named arguments `demo_id` and `binary_name`
	$(CC) $(CFLAGS) $(BUILD_TYPE) $(NON_GL_LDFLAGS) $(GL_LDFLAGS) -D DEMO_$(demo_id) -o bin/$(binary_name) src/demos/demo_$(demo_id).c
	cd bin && ./$(binary_name)

test_demos: # Tests demos in the test range, stopping if one fails to build
	for i in {$(TEST_RANGE)}; do \
		make build_demo demo_id=$$i binary_name=$(OUT)_$$i & \
	done

%: # This builds any app from 'tools/', which may be `editor` or `lightmapper`
	$(CC) $(CFLAGS) $(BUILD_TYPE) $(NON_GL_LDFLAGS) -o bin/$@ src/tools/$@.c
	cd bin && ./$@

clean:
	rm -r bin/*
