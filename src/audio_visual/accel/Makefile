DEMO = 9

OPTIMIZE = -Ofast
DEBUG = -fsanitize=address
BUILD_TYPE = $(DEBUG)

OUT = accel_demo

WARNINGS = -Wall -Wextra -Wdouble-promotion -Wpedantic -Wformat
CFLAGS = -std=c99 -march=native $(WARNINGS)
LDFLAGS = -lm -framework OpenGL -lglew -lSDL2

accel: accel_build
	./bin/accel_demo

accel_build:
	$(CC) $(CFLAGS) $(BUILD_TYPE) $(LDFLAGS) -D DEMO_$(DEMO) -o bin/$(OUT) src/demos/demo_$(DEMO).c

accel_test:
	for i in {1..$(ACCEL_DEMO)}; do\
		echo $$i;\
	done