// Painstakingly written by Justin Krosschell
// ECE 537 bittorrent simulator

#pragma once

#include <vector>
#include <cstddef>

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

	Peer(int IP, int upload, int download);

	Peer(Peer&&) = default; // Add a move constructor

	bool hasEverything() const;

	/// Called as the peer disconnects to minimize memory footprint when not in use
	void onDisconnect();

	/// Order peers based on who gave us the most, then reset the counts
	void reorderPeers();

	/**
	 * \brief Returns a list of offers to peers from our interestedList
	 * \returns The offers in the form of pairs.
	 *          The first item in the pair is the peer we are offering to,
	 *          and the second item is a list of indices of the chunks
	 */
	std::vector<std::pair<Peer*, std::vector<size_t>>> makeOffers() const;
};
