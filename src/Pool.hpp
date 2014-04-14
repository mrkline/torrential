#pragma once

#include <cstdlib>
#include <new>
#include <utility>

template <typename T>
class Pool {

public:

	Pool(size_t poolSize) :
		buff(nullptr),
		firstFree(nullptr),
		size(poolSize)
	{
		buff = static_cast<Slot*>(malloc(poolSize * sizeof(Slot)));
		if (buff == nullptr)
			throw std::bad_alloc();

		// Initialize all our slots
		for (size_t i = 0; i < poolSize; ++i) {
			Slot& currentSlot = buff[i];
			currentSlot.inUse = false;
			currentSlot.u.next = &buff[i + 1];
		}
		// The last next pointer should point to null
		buff[poolSize - 1].u.next = nullptr;
		firstFree = &buff[0];
	}

	~Pool()
	{
		free(buff);
	}

	size_t getSize() const { return size; }

	/// Allocates and returns a single node from the pool, or null if it cannot
	template <typename... Args>
	T* construct(Args&&... args)
	{
		if (firstFree == nullptr)
			return nullptr;

		// Go to our first free slot
		Slot* toUse = firstFree;
		Slot* nextFree = firstFree->u.next;

		// Mark our slot as in-use
		toUse->inUse = true;
		// Acctually instantiate the object in the chunk of memory we have for it
		::new (&toUse->u.data) T(std::forward<Args>(args)...);

		// Update the first free pointer now that this slot is in use
		firstFree = nextFree;

		return &toUse->u.data;
	}

	// No copy or assign
	Pool(const Pool&) = delete;
	const Pool& operator=(const Pool&) = delete;


private:

	struct Slot {
		bool inUse;
		union {
			T data;
			Slot* next;
		} u;
	};

	Slot* buff;
	Slot* firstFree;
	size_t size;
};
