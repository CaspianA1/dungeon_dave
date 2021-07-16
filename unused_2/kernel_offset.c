#include <stdio.h>

#define DEBUG(var, format) printf(#var " = %" #format "\n", var)

int main(void) {
	for (int y = -2; y <= 2; y++) {
		DEBUG(y + 2, d);
		for (int x = -2; x <= 2; x++) {
			// DEBUG(x + 2, d);
		}
	}
}
