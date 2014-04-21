#include "PoolTests.hpp"

#include <vector>

#include "Test.hpp"
#include "Pool.hpp"

using namespace std;
using namespace Exceptions;
using namespace Testing;

namespace {

class Payload {
public:

	Payload() : a(0), b(0) { }

	Payload(const Payload&) = default;

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

	assert(aPool.size() == 0);
	
	// Allocate a bunch of objects
	for (size_t i = 0; i < aPool.max_size(); ++i) {
		pointers.emplace_back(aPool.construct(i, 42 + i));
		assert(aPool.size() == i + 1);
	}

	// Check that they were constructed as we expect
	for (size_t i = 0; i < pointers.size(); ++i) {
		Payload& p = *pointers[i];
		assert(p.a == (int)i);
		assert(p.b == 42 + (int)i);
	}

	// Check that we get nullptr back when we're out of space
	assertThrown<std::bad_alloc>([&] { aPool.construct(); });
	assert(aPool.tryConstruct() == nullptr);

	for (size_t i = 0; i < pointers.size(); ++i)
		aPool.release(pointers[i]);
}

void release()
{
	Pool<Payload> aPool(5);
	vector<Payload*> pointers;
	
	// Allocate a bunch of objects
	for (size_t i = 0; i < aPool.max_size(); ++i) {
		pointers.emplace_back(aPool.construct(i, 42 + i));
	}

	// Check that they were constructed as we expect
	for (size_t i = 0; i < pointers.size(); ++i) {
		Payload& p = *pointers[i];
		assert(p.a == (int)i);
		assert(p.b == 42 + (int)i);
	}

	// Release them
	assert(aPool.size() == 5);
	aPool.release(pointers[0]);
	assert(aPool.size() == 4);
	aPool.release(pointers[4]);
	assert(aPool.size() == 3);
	aPool.release(pointers[1]);
	assert(aPool.size() == 2);
	aPool.release(pointers[3]);
	assert(aPool.size() == 1);
	aPool.release(pointers[2]);
	assert(aPool.size() == 0);
}

void allocate()
{
	Pool<Payload> aPool(10);

	// We have a lot of assertions here,
	// but hell, these are tests after all.

	Payload* first = aPool.allocate(3);
	assert(aPool.size() == 3);
	Payload* second = aPool.allocate(5);
	assert(aPool.size() == 8);
	Payload* third = aPool.allocate(2);
	assert(aPool.full());

	aPool.deallocate(first, 3);
	assert(aPool.size() == 7);
	aPool.deallocate(third, 2);
	assert(aPool.size() == 5);

	// Test that best-fit is working
	Payload* another = aPool.allocate(2);
	assert(aPool.size() == 7);
	// It should be put in the slot after second
	assert(another > second);
	assert(another == third);

	// Fit two allocations where we had our first allocation
	first = aPool.allocate(1);
	assert(aPool.size() == 8);
	Payload* secondFirst = aPool.allocate(2); // I suck at names
	assert(aPool.full());

	// We should be out
	assertThrown<std::bad_alloc>([&] { aPool.allocate(1); });

	// Deallocate two of the same size and allocate again.
	// Our algorithm should choose the first one
	aPool.deallocate(another, 2);
	assert(aPool.size() == 8);
	aPool.deallocate(secondFirst, 2);
	assert(aPool.size() == 6);

	another = aPool.allocate(2);
	assert(aPool.size() == 8);

	assert(another == secondFirst);

	// We should have two slots left
	assert(aPool.remaining() == 2);

	aPool.deallocate(first, 1);
	aPool.deallocate(second, 5);
	aPool.deallocate(another, 2);
}

void forSTL()
{
	Pool<Payload> aPool(20);
	vector<Payload, PoolAllocator<Payload>> vec1(10, aPool.getAllocator());
	vector<Payload, PoolAllocator<Payload>> vec2(10, aPool.getAllocator());
	// We should be out of memory in the pool now
	assertThrown<std::bad_alloc>([&] { vector<Payload, PoolAllocator<Payload>> vec3(1, aPool.getAllocator()); });
}

} // end namespace anonymous

void Testing::runPoolTests()
{
	beginUnit("Pool");
	test("Instantiation", &instantiation);
	test("Construction", &construction);
	test("Release", &release);
	test("Allocate", &allocate);
	test("As allocator for STL", &forSTL);
}
