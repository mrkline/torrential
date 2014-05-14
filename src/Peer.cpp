// Painstakingly written by Justin Krosschell
// ECE 537 bittorrent simulator

#include "Peer.hpp"

#include <algorithm>
#include <cassert>

using namespace std;

const size_t Peer::topToSend;

Peer::Peer (int IP, int upload, int download, size_t numChunks) :
	IPAddress(IP),
	uploadRate(upload),
	downloadRate(download),
	chunkList(numChunks),
	// These don't need to be in the list, but -WeffC++,
	// which provides warnings based on Effective C++ (a famous book),
	// recommends it.
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
	for (auto& item : interestedList) {
		// Peers that we can't help get the lowest possible contribution value (negative, even),
		// so they will not appear at the top of our list.
		if (!hasSomethingFor(*item.first))
			item.second = numeric_limits<decltype(item.second)>::min();
	}

	// Sort by most contributions first
	sort(begin(interestedList), end(interestedList), [](const pair<Peer*, int>&a, const pair<Peer*, int>& b) {
		return a.second > b.second;
	});

	// Zero out the counts
	for (auto& p : interestedList)
		p.second = 0;
}

bool Peer::hasSomethingFor(const Peer& other) const
{
	assert(chunkList.size() == other.chunkList.size());
	for (size_t i = 0; i < chunkList.size(); ++i) {
		// If we have a chunk they don't, return true
		if (chunkList[i] && !other.chunkList[i])
			return true;
	}

	return false;
}

std::vector<std::pair<Peer*, std::vector<size_t>>> Peer::makeOffers() const
{
	// Get out of here if we have nobody we are interested in
	if (interestedList.empty())
		return vector<pair<Peer*, vector<size_t>>>();

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
	vector<pair<Peer*, vector<size_t>>> ret(min(topToSend, interestedList.size()));

	// Set up our peer pointers quick
	for (size_t i = 0; i < topToSend && i < interestedList.size(); ++i) {
		ret[i].first = interestedList[i].first;
	}

	size_t peerIdx = 0;
	for (int offered = 0; offered < uploadRate; ++offered) {

		bool gaveSomething = false;

		// Wrap around the peers we're sending to, finding something we can send
		size_t startingPoint = peerIdx;
		do {
			assert(peerIdx < interestedList.size());
			Peer* top = interestedList[peerIdx].first;
			assert(peerIdx < ret.size());
			assert(ret[peerIdx].first == top);
			vector<size_t>& currentOfferings = ret[peerIdx].second;

			if (top->hasEverything())
				goto nextPeer; // Take a hike

			// Find the rarest they want that we have and haven't offered yet
			for (const auto& offering : popularity) {
				// See if they don't have it and it's not already in our offer list
				assert(offering.first < top->chunkList.size());
				if (!top->chunkList[offering.first] &&
				    find(begin(currentOfferings), end(currentOfferings), offering.first) == end(currentOfferings)) {
					// Offer a chunk!
					currentOfferings.emplace_back(offering.first);
					gaveSomething = true;
					break;
				}
			}

nextPeer:
			peerIdx = (peerIdx + 1) % std::min(topToSend, interestedList.size());

			if (gaveSomething)
				break;

		} while (peerIdx != startingPoint);

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

void Peer::acceptOffers(std::vector<std::pair<Peer*, std::vector<size_t>>>& offers)
{
	// Sanity check: We should only be getting offers for things we don't have
	for (auto& offerSet : offers) {
		for (size_t offer : offerSet.second)
			assert(!chunkList[offer]);
	}

	auto popularity = getChunkPopularity();

	// Coalesce our offers into one big list
	struct Offer {
		Peer* from;
		size_t chunkIdx;

		Offer(Peer* f, size_t idx) : from(f), chunkIdx(idx) { }
	};

	vector<Offer> allOffers;

	for (auto& offerSet : offers) {
		for (size_t offer : offerSet.second)
			allOffers.emplace_back(offerSet.first, offer);
	}
	// We can free up the offers vector. We're all done with it.
	// TODO: Should we do this? Could be shooting ourselves in the foot
	//       if we forget later. Oh well.
	offers.clear();
	offers.shrink_to_fit();

	// Lets's sort all of our offers by how popular they are
	sort(begin(allOffers), end(allOffers), [&] (const Offer& a, const Offer& b) {
		return popularity[a.chunkIdx].second < popularity[b.chunkIdx].second;
	});

	for (int downloaded = 0; downloaded < downloadRate && downloaded < (int)allOffers.size(); ++downloaded) {

		const Offer& accepting = allOffers[downloaded]; // The offer we're accepting

		printf("Peer %d accepting chunk %zu from peer %d%s\n", IPAddress, accepting.chunkIdx, accepting.from->IPAddress,
		       chunkList[accepting.chunkIdx] ? " (duplicate)" : "");

		chunkList[accepting.chunkIdx] = true;

		// See if this peer sending us stuff is in our interested list
		auto it = find_if(begin(interestedList), end(interestedList), [&](const pair<Peer*, int>& peer) {
			return peer.first == accepting.from;
		});

		// If he is, bump the count of things he's sent us.
		if (it != end(interestedList))
			++it->second;
	}
}
