#include <stdio.h>

enum {w = 30, h = 30};

int main(void) {
	printf("{\n");
	for (int i = 0; i < h; i++) {
		printf("{");
		for (int j = 0; j < w - 1; j++) printf("0, ");
		printf("0},\n");
	}
	printf("}\n");
}
