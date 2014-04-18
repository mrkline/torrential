#pragma once

#include <exception>
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
		numSlots(poolSize)
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
		// If everything isn't free, we're in undefined territory.
		// Give up!
		if (firstFree != buff || size() != 0) {
			fprintf(stderr, "A pool was destroyed before its elements were freed.\n");
			std::terminate();
		}

		free(buff);
	}

	size_t size() const { return numSlots - getFreeCount(); }

	size_t max_size() const { return numSlots; }

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
		Slot* releaseSlot = reinterpret_cast<Slot*>(toRelease);

		// Validation: Make sure this is a valid pointer
		if (!isValidPointer(releaseSlot))
			throw std::invalid_argument("The provided pointer is not valid");

		toRelease->~T(); // Call its destructor

		// This will be our first free node
		if (firstFree == nullptr) {
			releaseSlot->next = nullptr;
			firstFree = releaseSlot;
		}
		// The slot comes before our previously first free slot
		else if (releaseSlot < firstFree) {
			releaseSlot->next = firstFree;
			firstFree = releaseSlot;
		}
		// Walk the free list, inserting this node in the correct place
		else {
			// Thanks, Linus (http://stackoverflow.com/q/12914917/713961)
			Slot** curr = &firstFree;
			while ((*curr)->next != nullptr && (*curr)->next < releaseSlot) {
				curr = &(*curr)->next;
			}
			if ((*curr)->next == releaseSlot) {
				// Duplicate release
				return;
			}
			releaseSlot->next = (*curr)->next;
			(*curr)->next = releaseSlot;
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

	/**
	 * \brief Gets the number of contiguous slots start
	 * \param s The first free slot in a possible group of contiguous slots
	 * \returns The number of contiguous slots, starting at the given slot,
	 *          and the next free pointer after this current slot
	 *
	 * This function assumes that the pointer it is passed is free.
	 */
	std::pair<int, Slot*> getContiguousCount(Slot* s) const
	{
		assert(isValidPointer(s));
		int contig = 1;
		while (s->next == s + 1) {
			++contig;
			++s;
		}

		return std::pair<int, Slot*>(contig, s->next);
	}

	/// Gets the number of free slots
	int getFreeCount() const
	{
		// Just walk the free list until we hit null
		int ret = 0;
		for (Slot* curr = firstFree; curr != nullptr; curr = curr->next)
			++ret;

		return ret;
	}

	/// \brief Checks if a pointer is within the range of the buffer and is aligned.
	/// \warning This does not check if the pointer is free or used. That would take too much time.
	bool isValidPointer(Slot* s) const
	{
		if (s < buff || s >= buff + numSlots)
			return false; // The pointer is not inside our buffer

		const uintptr_t distance = (char*)s - (char*)buff;

		if (distance % sizeof(Slot) != 0)
			return false; // The pointer is not aligned

		return true;
	}

	Slot* buff;
	Slot* firstFree;
	size_t numSlots;
};
