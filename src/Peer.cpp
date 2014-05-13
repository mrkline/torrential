// Painstakingly written by Justin Krosschell
// ECE 537 bittorrent simulator

#include "Peer.hpp"

#include <algorithm>
#include <cassert>

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

bool Peer::hasEverything() const
{
	// In the future, we could cache this value, but for now let's focus on correctness
	return all_of(begin(chunkList), end(chunkList), [](bool b) { return b; });
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


std::vector<std::pair<Peer*, std::vector<size_t>>> Peer::makeOffers() const
{
	static const size_t topToSend = 5; // Send to the top 5

	// Find the rarest chunks among our entire interested list
	auto popularity = getChunkPopularity();

	// Now that we have our counts, remove any that we don't have to offer
	std::vector<size_t> toRemove; // indices of elements to remove from popularity
	for (size_t i = 0 ; i < chunkList.size(); ++i) {
		if (!chunkList[i])
			toRemove.emplace_back(i);
	}

	// Remove in reverse order (so that the indices don't change as we go)
	for (auto it = toRemove.rbegin(); it != toRemove.rend(); ++it) {
		// Erase takes an iterator, and we can get that from an index, i.e. *it
		// by adding that index to begin()
		popularity.erase(popularity.begin() + *it);
	}

	// Popularity is now a list of chunks we have with their indices and how many peers have them.
	// Let's sort by last-to-most popular chunks.
	sort(begin(popularity), end(popularity), [](const pair<size_t, int>& a, const pair<size_t, int>& b) {
		return a.second < b.second;
	});

	// Set up our return value
	vector<pair<Peer*, vector<size_t>>> ret(min((size_t)4, interestedList.size()));

	// Set up our peer pointers quick
	for (size_t i = 0; i < topToSend && i < interestedList.size(); ++i) {
		ret[i].first = interestedList[i].first;
	}

	for (int offered = 0; offered < uploadRate; ++offered) {

		bool gaveSomething = false;

		// For each of our top four peers
		for (size_t i = 0; i < topToSend && i < interestedList.size(); ++i) {
			Peer* top = interestedList[i].first;
			assert(ret[i].first == top);
			vector<size_t>& currentOfferings = ret[i].second;

			if (top->hasEverything())
				continue; // Take a hike

			// Find the rarest they want that we have and haven't offered yet
			for (const auto& offering : popularity) {
				// See if they don't have it and it's not already in our offer list
				if (!top->chunkList[offering.first] &&
				    find(begin(currentOfferings), end(currentOfferings), offering.first) == end(currentOfferings)) {
					// Offer a chunk!
					currentOfferings.emplace_back(offering.first);
					gaveSomething = true;
					break;
				}
			}
		}

		// If nobody wanted our stuff
		if (!gaveSomething)
			break;
	}

	return ret;
}

std::vector<std::pair<size_t, int>> Peer::getChunkPopularity() const
{
	// Keeps track of how many of our connected peers (in interestList) have any given chunk
	// The first item in the pair is the chunk index (because we'll reorder this later)
	// The second item in the pair is how many peers have that chunk.
	vector<pair<size_t, int>> popularity(chunkList.size());

	for (size_t i = 0; i < chunkList.size(); ++i)
		popularity[i].first = i;

	// For each peer we're interested in sharing with
	for (auto p : interestedList) {
		const Peer* pp = p.first;
		assert(pp->chunkList.size() == chunkList.size());

		// Add their chunks to our count
		for (size_t i = 0; i < chunkList.size(); ++i) {
			if (pp->chunkList[i])
				++popularity[i].second;
		}
	}

	return popularity;
}
