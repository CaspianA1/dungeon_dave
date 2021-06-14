#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#define inlinable extern inline
#define FAIL(a)

#define current_level.map_height foo

typedef struct {
	int x, y;
} VectorI;

typedef struct {
	double x, y;
} VectorF;

#include "bfs.c"

int main() {

}
