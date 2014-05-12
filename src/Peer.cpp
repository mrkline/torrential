// Painstakingly written by Justin Krosschell
// ECE 537 bittorrent simulator

#include "Peer.hpp"

#include <algorithm>

using namespace std;

Peer::Peer (int IP, int upload, int download) :
	IPAddress(IP),
	uploadRate(upload),
	downloadRate(download),
	// These don't need to be in the list, but -WeffC++,
	// which provides warnings based on Effective C++ (a famous book),
	// recommends it.
	chunkList(),
	interestedList()
{
	// don't do anything, but it is cool
}

void Peer::onDisconnect()
{
	// Go ahead and kill its interested list since we don't need it anymore
	// and it will get a new one if/when we reconnect
	interestedList.clear();
	interestedList.shrink_to_fit();
}

void Peer::reorderPeers()
{
	// Sort by most contributions first
	sort(begin(interestedList), end(interestedList), [](const pair<Peer*, int>&a, const pair<Peer*, int>& b) {
		return a.second > b.second;
	});

	// Zero out the counts
	for (auto& p : interestedList)
		p.second = 0;
}
