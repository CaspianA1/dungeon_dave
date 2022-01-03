CC = clang

OPTIMIZE = -Ofast
DEBUG = -fsanitize=address
BUILD_TYPE = $(OPTIMIZE)

DEMO = 17

OUT = accel_demo

WARNINGS = -Wall -Wextra -Wdouble-promotion -Wpedantic -Wformat
CFLAGS = -std=c99 -march=native $(WARNINGS)

NON_GL_LDFLAGS = -lm -lSDL2
LDFLAGS = -framework OpenGL -lglew $(NON_GL_LDFLAGS)

all:
	make build_any demo_id=$(DEMO)
	cd bin && ./accel_demo

editor:
	$(CC) $(CFLAGS) $(OPTIMIZE) $(NON_GL_LDFLAGS) -lSDL2_ttf -o bin/editor src/editor/editor.c
	./bin/editor

# takes a named argument demo_id
build_any:
	$(CC) $(CFLAGS) $(BUILD_TYPE) $(LDFLAGS) -D DEMO_$(demo_id) -o bin/$(OUT) src/demos/demo_$(demo_id).c

# tests demos from 1 to DEMO, stopping if one fails to build
test_all:
	for i in {$(DEMO)..1}; do \
		if !(make build_any demo_id=$$i) then break; \
		else ./bin/accel_demo & \
		fi \
	done

clean:
	rm -r bin/*
