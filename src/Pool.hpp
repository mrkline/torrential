#pragma once

#include <cstdlib>
#include <new>
#include <utility>

#include "Exceptions.hpp"

namespace Exceptions {

/// Thrown if the user tries to construct an item in a full pool
class PoolFullException final : public InvalidOperationException {
public:
	PoolFullException(const std::string& exceptionMessage,
	                  const char* file, int line) :
		InvalidOperationException(exceptionMessage, file, line, "pool full")
	{ }

	PoolFullException(const PoolFullException&) = default;

	PoolFullException& operator=(PoolFullException&) = delete;
};

} // end namespace Exceptions

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
			currentSlot.next = &buff[i + 1];
		}
		// The last next pointer should point to null
		buff[poolSize - 1].next = nullptr;
		firstFree = &buff[0];
	}

	~Pool()
	{
		free(buff);
	}

	size_t getSize() const { return size; }

	/// Allocates and returns a single node from the pool, or throws PoolFullException otherwise
	template <typename... Args>
	T* construct(Args&&... args)
	{
		T* ret = tryConstruct(std::forward<Args>(args)...);
		if (ret == nullptr)
			THROW(Exceptions::PoolFullException, "The pool is full.");

		return ret;
	}

	/// Allocates and returns a single node from the pool, or null if the pool is full
	template <typename... Args>
	T* tryConstruct(Args&&... args)
	{
		if (firstFree == nullptr)
			return nullptr;

		// Go to our first free slot
		Slot* toUse = firstFree;
		Slot* nextFree = firstFree->next;

		// Acctually instantiate the object in the chunk of memory we have for it
		::new (&toUse->data) T(std::forward<Args>(args)...);

		// Update the first free pointer now that this slot is in use
		firstFree = nextFree;

		return &toUse->data;
	}

	void release(T* toRelease)
	{
		// TODO: Validation: Make sure this is a valid pointer


		toRelease->~T(); // Call its destructor

		Slot* releaseSlot = reinterpret_cast<Slot*>(toRelease);

		if (releaseSlot < firstFree) {
			releaseSlot->next = firstFree;
			firstFree = releaseSlot;
		}
		else {
			// Thanks, Linus (http://stackoverflow.com/q/12914917/713961)
			Slot** next = &firstFree->next;
			while (*next != nullptr && *next < releaseSlot) {
				next = &(*next)->next;
			}
			releaseSlot->next = *next;
			(*next)->next = releaseSlot;
		}
	}

	// No copy or assign
	Pool(const Pool&) = delete;
	const Pool& operator=(const Pool&) = delete;


private:

	 /// A slot in our pool.
	 /// We use this union so that we can hold a pointer to the next free slot
	 ///when the slot is not in use
	union Slot {
		T data; ///< The allocated data in the slot, or alternatively...
		Slot* next; ///< ...A pointer to the next free slot
	};

	Slot* buff;
	Slot* firstFree;
	size_t size;
};
