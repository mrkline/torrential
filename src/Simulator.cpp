#include "Simulator.hpp"

#include <algorithm>

using namespace std;

Simulator::Simulator(size_t numClients) :
	connected(numClients),
	disconnected(numClients),
	rng(random_device()()), // Seed the RNG with entropy from the system via random_device
	shouldConnect(0.02), // Connect at a 2% rate. Feel free to play with this
	shouldDisconnect(0.8) // Disconnect at a 80% rate when done. Feel free to play with this.
{
	// Start out with one seeder with all the file chunks
}

/**
 * \brief Runs one iteration of the simulator
 *
 * The process is as follows:
 *
 * 1. For each disconneted peer, determine if it is coming back.
 *    If it is, add it to the connected list
 *    - Register with tracker
 *    - Add to connected pool
 *    - Reset sim counter to 0
 *
 * 2. For reach connected peer, determine if it will leave
 *    - Remove from tracker list, update each connected peer's list
 *    - Add it to the disconnected pool
 *
 * 3. Work on connected peers
 *    - Check the sim counter. If counter % 10 == 0, re-eval top 4 peers
 *      - Reorder list of connected peers
 *      - Empty history structure (who we've gotten data from)
 *    - If counter % 30 == 0, optimally unchoke a peer
 *    - If number in chunklist < 20, get more peers randomly (up to 40)
 *
 * 4. Generate a list of offers for each peer,
 *    based on the upload rates of those offering and how many they are offering to.
 *
 * 5. Have each peer accept as many of its offers as possible,
 *    based on its download rate. Update the chunk lists accordingly.
 */
void Simulator::tick()
{
	connectPeers();
	periodicTasks();
	bumpSimCount();
	auto offers = makeOffers();
	acceptOffers(offers);
}

void Simulator::connectPeers()
{
	// Go through the disconnected peers, connecting some at random
	for (auto it = begin(disconnected); it != end(disconnected);) {
		if (shouldConnect(rng)) {
			// Initialize it
			it->simCounter = 0; // sim counter gets reset
			// Get us some peers
			auto peerList = getRandomPeers(Peer::desiredPeerCount);
			assert(it->interestedList.empty()); // This had better be empty
			// Convert our Peer* list to a pair<Peer*, int> list
			transform(begin(peerList), end(peerList), begin(it->interestedList),
			          [](Peer* p) {
				return pair<Peer*, int>(p, 0);
			});

			// Move the peer to the connected list
			connected.construct(std::move(*it));

			// Then remove it from the disconnected list
			it = disconnected.destroy(it);
		}
		else
			++it;
	}
}

std::vector<Peer*> Simulator::getRandomPeers(size_t num,
                                             const std::vector<Peer*>& ignore)
{
	vector<Peer*> ret; // The one we're going to return
	ret.reserve(num);

	// If we have fewer than num connected peers, congrats.
	// All of them are in our list
	if (connected.size() <= num) {
		for (Peer& connectedPeer : connected)
			ret.emplace_back(&connectedPeer);
	}
	// No such luck. Let's get a random subset of our peers
	else {
		// Get pointers to all the connected peers
		vector<Peer*> peerList;
		peerList.reserve(connected.size());

		for (Peer& connectedPeer : connected)
			peerList.emplace_back(&connectedPeer);

		// Shuffle that pointer list
		shuffle(begin(peerList), end(peerList), rng);

		// Only take the first num pointers
		peerList.resize(num);

		ret = move(peerList);
	}

	// Remove peers we already have from the results
	// TODO: This is n^2, but do smarter sorting or something later
	for (Peer* conn : ignore) {
		auto it = find(begin(ret), end(ret), conn);
		if (it != end(ret))
			ret.erase(it);
	}

	return ret;
}

Simulator::OfferMap Simulator::makeOffers()
{
	OfferMap ret;

	for (Peer& p : connected) {
		auto offers = p.makeOffers();
		for (auto& offer : offers) {
			ret[offer.first].emplace_back(&p, offer.second);
		}
	}

	return ret;
}

void Simulator::acceptOffers(OfferMap& offers)
{
	for (Peer& p : connected) {
		auto it = offers.find(&p);
		if (it == end(offers))
			continue;

		// If it's in the list, carry on.
		p.acceptOffers(it->second);
	}
}

void Simulator::bumpSimCount()
{
	for (Peer& p : connected)
		++p.simCounter;
}

void Simulator::periodicTasks()
{
	for (Peer& p : connected) {
		// If we have less than 20 peers, get some more
		if (p.interestedList.size() < 20) {
			// First we need to get a list of peers we already have.
			// Time for our best friend, std::transform again!
			// We have to transform interestedLists's <Peer*, int> pairs
			// into just Peer pointers.
			vector<Peer*> alreadyHas;
			transform(begin(p.interestedList), end(p.interestedList), begin(alreadyHas),
			          [](const pair<Peer*, int>& pp) {
				return pp.first;
			});

			auto newPeers = getRandomPeers(Peer::desiredPeerCount, alreadyHas);

			vector<pair<Peer*, int>> newPairs;
			newPairs.reserve(newPeers.size());

			transform(begin(newPeers), end(newPeers), begin(newPairs), [](Peer* p) {
				return pair<Peer*, int>(p, 0);
			});

			p.interestedList.insert(end(p.interestedList), begin(newPairs), end(newPairs));
		}

		// Every 10 ticks, re-evaluate top four
		if (p.simCounter % 10 == 0)
			p.reorderPeers();

		// Every 30 ticks, optimistically unchoke a random peer
		if (p.simCounter % 30 == 0)
			p.randomUnchoke(rng);
	}
}
