#pragma once

#include <exception>
#include <cstdlib>
#include <new>
#include <utility>
#include <queue>


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
			buff[i].next = &buff[i + 1];
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

	/// Gets the number of free slots
	size_t remaining() const
	{
		// Just walk the free list until we hit null
		size_t ret = 0;
		for (Slot* curr = firstFree; curr != nullptr; curr = curr->next)
			++ret;

		return ret;
	}

	size_t size() const { return numSlots - remaining(); }

	size_t max_size() const { return numSlots; }

	bool empty() const { return remaining() == numSlots; }

	bool full() const { return remaining() == 0; }

	T* allocate(size_t num)
	{
		struct Block {
			Slot* start;
			size_t size;
			Slot** previous;

			Block(Slot* st, size_t sz, Slot** prev) : start(st), size(sz), previous(prev) { }
		};

		auto calcFit = [num](const Block& b1, const Block& b2) {
			// We shouldn't have blocks that don't fit our needed amount in the queue
			assert(b1.size >= num);
			assert(b2.size >= num);
			const int n = (int)num;
			const int b1n = (int)b1.size;
			const int b2n = (int)b2.size;
			const int a1 = abs(b1n - n);
			const int a2 = abs(b2n - n);
			if (a1 == a2)
				return b1.start > b2.start; // If these are the same fit, prefer the one closer to the start
			else
				return  a1 > a2; // Sort by best fit
			// TODO: Tiebreakers, possibly based on position
		};

		std::priority_queue<Block, std::vector<Block>, decltype(calcFit)> bestFitter(calcFit);

		// Iterate through our free list, finding contiguous chunks
		Slot** curr = &firstFree;
		std::pair<int, Slot**> countInfo = getContiguousCount(*curr);
		for (; *curr != nullptr; curr = countInfo.second, countInfo = getContiguousCount(*curr)) {
			// If a chunk can fit our needed size, throw it in the priority queue
			if (countInfo.first >= num)
				bestFitter.emplace(*curr, countInfo.first, curr);
		}

		if (bestFitter.empty())
			throw std::bad_alloc();

		const auto best = bestFitter.top();

		// Set our previous pointer to the next pointer at the end of this block,
		// effectively skipping this block
		*best.previous = best.start[num - 1].next;

		return reinterpret_cast<T*>(best.start); // Return our best fit block
	}

	void deallocate(T* allocated, size_t n)
	{
		Slot* blockStart = reinterpret_cast<Slot*>(allocated);

		// Validation: Make sure this is a valid pointer
		if (!isValidPointer(blockStart))
			throw std::invalid_argument("The provided pointer is not valid");

		// This will be our first free node
		if (firstFree == nullptr) {
			blockStart[n - 1].next = nullptr;
			firstFree = blockStart;
		}
		// The slot comes before our previously first free slot
		else if (blockStart < firstFree) {
			blockStart[n - 1].next = firstFree;
			firstFree = blockStart;
		}
		// Walk the free list, inserting this node in the correct place
		else {
			Slot** curr = &firstFree;
			while ((*curr)->next != nullptr && (*curr)->next < blockStart) {
				curr = &(*curr)->next;
			}
			if ((*curr)->next == blockStart) {
				throw std::logic_error("Double deallocate detected");
			}

			blockStart[n - 1].next = (*curr)->next;
			(*curr)->next = blockStart;
		}

		// Set up the block's "next" pointers
		for (size_t i = 0; i < n - 1; ++i)
			blockStart[i].next = &blockStart[i + 1];
	}


	/// Allocates and returns a single node from the pool, or throws PoolFullException otherwise
	template <typename... Args>
	T* construct(Args&&... args)
	{
		T* ret = allocate(1);
		::new (ret) T(std::forward<Args>(args)...);

		return ret;
	}

	/// Allocates and returns a single node from the pool, or null if the pool is full
	template <typename... Args>
	T* tryConstruct(Args&&... args)
	{
		try {
			return construct(std::forward<Args>(args)...);
		}
		catch (const std::bad_alloc&) {
			return nullptr;
		}
	}

	void release(T* toRelease)
	{
		deallocate(toRelease, 1);

		toRelease->~T(); // Call its destructor
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
	 *          and the next free pointer after this current slot,
	 *          or 0 and null if null is given as an input
	 *
	 * This function assumes that the pointer it is passed is free.
	 */
	std::pair<int, Slot**> getContiguousCount(Slot* s) const
	{
		if (s == nullptr)
			return std::pair<int, Slot**>(0, nullptr);

		assert(isValidPointer(s));
		int contig = 1;
		while (s->next == s + 1) {
			++contig;
			++s;
		}

		return std::pair<int, Slot**>(contig, &s->next);
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
