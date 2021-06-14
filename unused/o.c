#include <stdio.h>

int main() {
	int foo[4] = {5, 6, 7, 8};

	for (int i = 3; i > 0; i--)
		foo[i] = foo[i - 1];

	for (int i = 0; i < 4; i++)
		printf("%d\n", foo[i]);
}
