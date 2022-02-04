/*
- This demo focuses on shadow volumes for sectors

Decision to make (one to choose):
- One shadow volume per sector?
- One per face? If so, per overall faces or per faces split by other sectors?
*/

#include "../utils.c"
#include "../sector.c"


void foo(const Sector* const sector) {
	const byte
		*const origin = sector -> origin,
		*const size = sector -> size;
}

#ifdef DEMO_23
int main(void) {

}
#endif
