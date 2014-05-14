#include <cstdio>

#include "Simulator.hpp"

// Lee Stratman added this comment and Justin Krosschell revised it.
int main()
{
	printf("Hello, ECE 537! Here goes nothing.\n");

	Simulator ohGod(9001, 30);

	while (!ohGod.allDone())
		ohGod.tick();

	return 0;
}
