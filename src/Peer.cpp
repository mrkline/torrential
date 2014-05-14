// Painstakingly written by Justin Krosschell
// ECE 537 bittorrent simulator

#include "Peer.hpp"

#include <algorithm>
#include <cassert>

#include "Printer.hpp"

using namespace std;

const size_t Peer::topToSend;

Peer::Peer (int IP, int upload, int download, size_t numChunks, bool isSeed) :
	IPAddress(IP),
	uploadRate(upload),
	downloadRate(download),
	chunkList(numChunks),
	interestedList(),
	done(isSeed),
	// These don't need to be in the list, but -WeffC++,
	// which provides warnings based on Effective C++ (a famous book),
	// recommends putting all members in the initializer list.
	consideredOffers(),
	uploadMutex(),
	uploadRemaining()
{
	// If we're the seed, fill our chunkList
	if (isSeed)
		fill(begin(chunkList), end(chunkList), true);
}

Peer::Peer(Peer&& o) :
	IPAddress(o.IPAddress),
	uploadRate(o.uploadRate),
	downloadRate(o.downloadRate),
	chunkList(o.chunkList),
	interestedList(move(o.interestedList)),
	done(o.done),
	consideredOffers(move(o.consideredOffers)),
	uploadMutex(), // You can't copy, move, or otherwise, a mutex
	uploadRemaining(o.uploadRemaining)
{
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

std::vector<std::pair<Peer*, std::vector<size_t>>> Peer::makeOffers()
{
	// Get out of here if we have nobody we are interested in
	if (interestedList.empty() || uploadRate == 0)
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

	const auto recipientCount = min(topToSend, interestedList.size());

	// Set up our return value
	vector<pair<Peer*, vector<size_t>>> ret(recipientCount);

	// Set up our peer pointers quick
	for (size_t i = 0; i < topToSend && i < interestedList.size(); ++i) {
		ret[i].first = interestedList[i].first;
	}

	size_t peerIdx = 0;
	// Offer our entire upload bandwidth to each peer we are sending to
	for (int offered = 0; offered < uploadRate * (int)recipientCount; ++offered) {

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
			peerIdx = (peerIdx + 1) % recipientCount;

			if (gaveSomething)
				break;

		} while (peerIdx != startingPoint);

		// If nobody wanted our stuff
		if (!gaveSomething)
			break;
	}

	// Be sure to reset our number of upload slots remaining for this tick
	uploadRemaining = uploadRate;

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

void Peer::considerOffers(std::vector<std::pair<Peer*, std::vector<size_t>>>& offers)
{
	// Sanity check: We should only be getting offers for things we don't have
#ifndef NDEBUG
	for (auto& offerSet : offers) {
		for (size_t offer : offerSet.second)
			assert(!chunkList[offer]);
	}
#endif

	assert(consideredOffers.empty());

	auto popularity = getChunkPopularity();

	// Coalesce our offers into one big list
	for (auto& offerSet : offers) {
		for (size_t offer : offerSet.second)
			consideredOffers.emplace_back(offerSet.first, offer);
	}

	// We can free up the offers vector. We're all done with it.
	// TODO: Should we do this? Could be shooting ourselves in the foot
	//       if we forget later. Oh well.
	offers.clear();
	offers.shrink_to_fit();

	// Lets's sort all of our offers by how popular they are
	sort(begin(consideredOffers), end(consideredOffers), [&] (const Offer& a, const Offer& b) {
		return popularity[a.chunkIdx].second < popularity[b.chunkIdx].second;
	});
}

void Peer::acceptOffers()
{
	if (consideredOffers.empty())
		return;

	int downloaded = 0;
	for (size_t offerIdx = 0; downloaded < downloadRate && offerIdx < consideredOffers.size(); ++offerIdx) {

		const Offer& accepting = consideredOffers[offerIdx]; // The offer we're accepting

		// See if this peer sending us stuff is in our interested list
		auto it = find_if(begin(interestedList), end(interestedList), [&](const pair<Peer*, int>& peer) {
			return peer.first == accepting.from;
		});

		// If he is, bump the count of things he's sent us.
		// Even if it's a duplicate, they tried.
		if (it != end(interestedList))
			++it->second;

		// If we have this chunk already, don't waste a download slot
		if (chunkList[accepting.chunkIdx])
			continue;

		// See if the peer still has upload slots to use this tick
		unique_lock<mutex> uploadLock(accepting.from->uploadMutex);
		if (accepting.from->uploadRemaining == 0)
			continue;

		--accepting.from->uploadRemaining;
		uploadLock.unlock();

		printTransmit(accepting.from->IPAddress, accepting.chunkIdx, IPAddress);

		chunkList[accepting.chunkIdx] = true;

		// Increment the chunks we downloaded
		++downloaded;
	}

	done = all_of(begin(chunkList), end(chunkList), [](bool b) { return b; });

	if (done)
		printFinished(IPAddress, chunkList.size());

	// We're done with the considered offers
	consideredOffers.clear();
}
