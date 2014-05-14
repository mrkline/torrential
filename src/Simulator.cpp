#include "Simulator.hpp"

#include <algorithm>

#include "IteratorUtils.hpp"
#include "Printer.hpp"

using namespace std;

Simulator::Simulator(size_t numClients, size_t numChunks, double joinProbability, double leaveProbability,
                     std::pair<int, int> uploadRange, std::pair<int, int> downloadRange, size_t freeriders) :
	connected(numClients),
	disconnected(numClients),
	rng(random_device()()), // Seed the RNG with entropy from the system via random_device
	shouldConnect(joinProbability), // Connect at a 2% rate. Feel free to play with this
	shouldDisconnect(leaveProbability) // Disconnect at a 80% rate when done. Feel free to play with this.
{
	assert(numClients > 1); // Don't be stupid.

	uniform_int_distribution<int> upload(uploadRange.first, uploadRange.second);
	uniform_int_distribution<int> download(downloadRange.first, downloadRange.second);

	static int uid = 0;

	// Start out with one seeder with all the file chunks
	connected.construct(uid++, upload(rng), download(rng), numChunks, true);

	// Start out with everyone else with nothing
	for (size_t i = 0; i < numClients - 1 - freeriders; ++i)
		disconnected.construct(uid++, upload(rng), download(rng), numChunks, false);
	// Add our freeriders in at the end
	for (size_t i = 0; i < freeriders; ++i)
		disconnected.construct(uid++, 0, download(rng), numChunks, false);
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
	printTick(++tickNumber);
	connectPeers();
	periodicTasks();
	bumpSimCount();
	auto offers = makeOffers();
	considerOffers(offers);
	acceptOffers();
	disconnectPeers();
}

bool Simulator::allDone() const
{
	return all_of(begin(connected), end(connected), [](const Peer& p) { return p.hasEverything(); })
		&& all_of(begin(disconnected), end(disconnected), [](const Peer& p) { return p.hasEverything(); });
}

void Simulator::connectPeers()
{
	// Go through the disconnected peers, connecting some at random
	for (auto it = begin(disconnected); it != end(disconnected);) {
		if (shouldConnect(rng)) {
			printConnection(it->IPAddress);
			// Initialize it
			it->simCounter = 0; // sim counter gets reset
			// Get us some peers
			// We are not interested in ourselves
			auto peerList = getRandomPeers(Peer::desiredPeerCount, {&(*it)});
			assert(it->interestedList.empty()); // This had better be empty
			// Convert our Peer* list to a pair<Peer*, int> list
			transform(begin(peerList), end(peerList), back_inserter(it->interestedList),
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

void Simulator::disconnectPeers()
{
	for (auto it = begin(connected); it != end(connected);) {
		if (shouldDisconnect(rng)) {
			printDisconnection(it->IPAddress);

			it->onDisconnect();

			disconnected.construct(std::move(*it));

			it = connected.destroy(it);
		}
		else {
			++it;
		}
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

		for (Peer& connectedPeer : connected) {
			// Ignore those that have everything
			if (!connectedPeer.hasEverything())
				peerList.emplace_back(&connectedPeer);
		}

		// Shuffle that pointer list
		shuffle(begin(peerList), end(peerList), rng);

		// Only take the first num pointers
		if (peerList.size() > num)
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
	mutex mapLock;
	OfferMap ret;

	parallelForEach(begin(connected), end(connected), [&ret, &mapLock](Peer& p) {
		auto offers = p.makeOffers();
		lock_guard<mutex> guard(mapLock);
		for (auto& offer : offers) {
			ret[offer.first].emplace_back(&p, offer.second);
		}
	});

	return ret;
}

void Simulator::considerOffers(OfferMap& offers)
{
	parallelForEach(begin(connected), end(connected), [&offers](Peer& p) {
		auto it = offers.find(&p);
		if (it == end(offers))
			return;

		// If it's in the list, carry on.
		p.considerOffers(it->second);
	});
}

void Simulator::acceptOffers()
{
	parallelForEach(begin(connected), end(connected), [](Peer& p) {
		p.acceptOffers();
	});
}

void Simulator::bumpSimCount()
{
	parallelForEach(begin(connected), end(connected), [](Peer& p) {
		++p.simCounter;
	});
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
			transform(begin(p.interestedList), end(p.interestedList), back_inserter(alreadyHas),
			          [](const pair<Peer*, int>& pp) {
				return pp.first;
			});

			alreadyHas.emplace_back(&p); // We are not interested in ourselves

			auto newPeers = getRandomPeers(Peer::desiredPeerCount, alreadyHas);

			vector<pair<Peer*, int>> newPairs;
			newPairs.reserve(newPeers.size());

			transform(begin(newPeers), end(newPeers), back_inserter(newPairs), [](Peer* p) {
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

		// Every so often, churn it up.
		// Chuck out peers we can't help and replace them with new random guys
		// This is important, and prevents us from getting "stuck" where everyone in your list
		// already has the chunks you are offering.
		if (p.simCounter % 120 == 0) {

			// Find peers we can't help anymore
			vector<decltype(p.interestedList)::iterator> cannotHelp;
			for (auto it = begin(p.interestedList); it != end(p.interestedList); ++it) {
				if (!p.hasSomethingFor(*it->first))
					cannotHelp.emplace_back(it);
			}

			// If we can help someone, get out.
			if (cannotHelp.size() == 0)
				return;

			// Don't get any peers we already have
			vector<Peer*> alreadyHas;
			transform(begin(p.interestedList), end(p.interestedList), back_inserter(alreadyHas),
			          [](const pair<Peer*, int>& pp) {
				return pp.first;
			});

			alreadyHas.emplace_back(&p);

			// Remove the peers we can't help
			for (auto it = cannotHelp.rbegin(); it != cannotHelp.rend(); ++it)
				p.interestedList.erase(*it);

			assert(Peer::desiredPeerCount > p.interestedList.size());
			auto newPeers = getRandomPeers(Peer::desiredPeerCount - p.interestedList.size(), alreadyHas);

			vector<pair<Peer*, int>> newPairs;
			newPairs.reserve(newPeers.size());

			transform(begin(newPeers), end(newPeers), back_inserter(newPairs), [](Peer* p) {
				return pair<Peer*, int>(p, 0);
			});

			p.interestedList.insert(end(p.interestedList), begin(newPairs), end(newPairs));
		}
	}
}
