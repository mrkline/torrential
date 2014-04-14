#include "PoolTests.hpp"

#include <vector>

#include "Test.hpp"
#include "Pool.hpp"

using namespace std;

namespace {

class Payload {
public:

	Payload() : a(0), b(0) { }

	Payload(int a, int b) : a(a), b(b) { }

	int a, b;
};

void instantiation()
{
	Pool<Payload> aPool(100);
}

void construction()
{
	Pool<Payload> aPool(5);
	vector<Payload*> pointers;
	
	// Allocate a bunch of objects
	for (size_t i = 0; i < aPool.getSize(); ++i) {
		pointers.emplace_back(aPool.construct(i, 42 + i));
	}

	// Check that they were constructed as we expect
	for (size_t i = 0; i < pointers.size(); ++i) {
		Payload& p = *pointers[i];
		assert(p.a == (int)i);
		assert(p.b == 42 + (int)i);
	}

	// Check that we get nullptr back when we're out of space
	assert(aPool.construct() == nullptr);
}

void release()
{
	Pool<Payload> aPool(5);
	vector<Payload*> pointers;
	
	// Allocate a bunch of objects
	for (size_t i = 0; i < aPool.getSize(); ++i) {
		pointers.emplace_back(aPool.construct(i, 42 + i));
	}

	// Check that they were constructed as we expect
	for (size_t i = 0; i < pointers.size(); ++i) {
		Payload& p = *pointers[i];
		assert(p.a == (int)i);
		assert(p.b == 42 + (int)i);
	}

	// Check that we get nullptr back when we're out of space
	aPool.release(pointers[0]);
}

} // end namespace anonymous

void Testing::runPoolTests()
{
	beginUnit("Pool");
	test("Instantiation", &instantiation);
	test("Construction", &construction);
}
