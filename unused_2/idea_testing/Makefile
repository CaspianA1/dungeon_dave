OUT = small_shade
CFLAGS = clang -Wall -Wextra


all: build
	./$(OUT)

build:
	$(CFLAGS) -o $(OUT) $(OUT).c

asm:
	$(CFLAGS) -S -masm=intel -o $(OUT).asm $(OUT).c