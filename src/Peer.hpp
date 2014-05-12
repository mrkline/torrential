// Painstakingly written by Justin Krosschell
// ECE 537 bittorrent simulator

#pragma once

#include <vector>

class Peer {
public:

	static const int desiredPeerCount = 40;

	// member variables
	int simCounter = 0;  ///< peer's simulation counter
	const int IPAddress;  ///< peer's IP address
	const int uploadRate;  ///< peer's upload rate (should be a multiple of 256kB/s)
	const int downloadRate;  ///< peer's download rate (roughly 10X the upload rate)
	std::vector<bool> chunkList;  ///< boolean array list of chunks that the peer has

	/// Array of peers that this peer can request chunks from and how many chunks they've given us
	std::vector<std::pair<Peer*, int>> interestedList;

	Peer(int IP, int upload, int download);

	Peer(Peer&&) = default; // Add a move constructor

	/// Called as the peer disconnects to minimize memory footprint when not in use
	void onDisconnect();

	/// Order peers based on who gave us the most, then reset the counts
	void reorderPeers();
};
