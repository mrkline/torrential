// Painstakingly written by Justin Krosschell
// ECE 537 bittorrent simulator

#pragma once

#include <vector>
#include <cstddef>
#include <random>

class Peer {
public:

	static const int desiredPeerCount = 40;

	// member variables
	int simCounter = 0;  ///< peer's simulation counter
	const int IPAddress;  ///< peer's IP address
	const int uploadRate;  ///< peer's upload rate in chunks/second
	const int downloadRate;  ///< peer's download rate in chunks/second (roughly 10X the upload rate)
	std::vector<bool> chunkList;  ///< boolean array list of chunks that the peer has

	/// Array of peers that this peer can request chunks from and how many chunks they've given us
	std::vector<std::pair<Peer*, int>> interestedList;

	Peer(int IP, int upload, int download, size_t numChunks, bool isSeed);

	Peer(Peer&&) = default; // Add a move constructor

	bool hasEverything() const { return done; }

	/// Called as the peer disconnects to minimize memory footprint when not in use
	void onDisconnect();

	/// Order peers based on who gave us the most, then reset the counts
	void reorderPeers();

	bool hasSomethingFor(const Peer& other) const;

	/**
	 * \brief Optimistically unchoke a random peer
	 *
	 * We'll unchoke by picking a random guy and sticking him in the fifth slot
	 */
	template <typename RNG>
	void randomUnchoke(RNG& gen)
	{
		// If our interested list is tiny enough that we're always
		// offering to everyone, we don't need to do anything.
		// This probably won't happen, but meh. Why not check?
		if (interestedList.size() <= topToSend)
			return;

		static const auto unchokedPosition = topToSend - 1;

		// Pick someone to unchoke
		std::uniform_int_distribution<size_t> unchoker(unchokedPosition, interestedList.size() - 1);
		size_t idxToUnchoke = unchoker(gen);

		// Swap him with our currently unchoked peer
		swap(interestedList[unchokedPosition], interestedList[idxToUnchoke]);
	}

	/**
	 * \brief Returns a list of offers to peers from our interestedList
	 * \returns The offers in the form of pairs.
	 *          The first item in the pair is the peer we are offering to,
	 *          and the second item is a list of indices of the chunks
	 */
	std::vector<std::pair<Peer*, std::vector<size_t>>> makeOffers() const;

	void considerOffers(std::vector<std::pair<const Peer*, std::vector<size_t>>>& offers);

	void acceptOffers();

private:

	struct Offer {
		const Peer* from;
		size_t chunkIdx;

		Offer(const Peer* f, size_t idx) : from(f), chunkIdx(idx) { }
	};

	static const size_t topToSend = 5; // Send to the top 5 peers (4 + 1 optimistically unchoked)

	std::vector<Offer> consideredOffers;

	std::vector<std::pair<size_t, int>> getChunkPopularity() const;

	bool done;

};
