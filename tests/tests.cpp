#include <cstdio>

#include "Test.hpp"
#include "PoolTests.hpp"
#include "PeerTests.hpp"

int main()
{
	using namespace Testing;

	printf("Running unit tests...\n");
	runPoolTests();
	runPeerTests();
	return 0;
}
