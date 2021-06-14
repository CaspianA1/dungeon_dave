CC = clang
OUT = dda
CFLAGS = -Wall -Wformat -Wpedantic
LDFLAGS = -o bin/$(OUT) src/$(OUT).c

all: build run

run:
	./bin/$(OUT) || (reset; echo "An error occurred.")

build:
	$(CC) $(CFLAGS) -O3 $(LDFLAGS)

asm:
	$(CC) $(CFLAGS) -S -masm=intel -o bin/$(OUT).asm src/$(OUT).c

debug:
	$(CC) $(CFLAGS) -ggdb3 $(LDFLAGS)
	lldb bin/$(OUT)

clean:
	rm -r bin/*
