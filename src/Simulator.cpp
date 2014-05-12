#include "Simulator.hpp"

#include <algorithm>

using namespace std;

Simulator::Simulator(size_t numClients) :
	connected(numClients),
	disconnected(numClients),
	rng(random_device()()), // Seed the RNG with entropy from the system via random_device
	shouldConnect(0.02), // Connect at a 2% rate. Feel free to play with this
	shouldDisconnect(0.01) // Disconnect at a 1% rate. Feel free to play with this.
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
	disconnectPeers();
	bumpSimCount();
}

void Simulator::connectPeers()
{
	// Go through the disconnected peers, connecting some at random
	for (auto it = begin(disconnected); it != end(disconnected);) {
		if (shouldConnect(rng)) {
			// Initialize it
			it->simCounter = 0; // sim counter gets reset
			it->interestedList = getRandomPeers(Peer::desiredPeerCount); // Get us some peers

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
	vector<Peer*> removing;

	// Go through the connected peers, disconnecting some at random
	for (auto it = begin(connected); it != end(connected);) {
		if (shouldDisconnect(rng)) {

			// Go ahead and kill its interested list since we don't need it anymore
			// and it will get a new one if/when we reconnect
			it->interestedList.clear();
			it->interestedList.shrink_to_fit();

			// Why the hell are we dereferncing, then taking address of?
			// Well, you see, it isn't a pointer, it's an iterator.
			// Iterators act like pointers in that they return a reference
			// when dereferenced with *, but they aren't actually pointers,
			// they're their own classes.
			// So, we get a refernce to a Peer with * and then get its address with &.
			removing.emplace_back(&(*it));

			// Move the peer to the disconnected list
			connected.construct(std::move(*it));

			it = connected.destroy(it);
		}
		else {
			++it;
		}
	}

	// Now that we've removed peers from the connected list,
	// we have to remove their pointers from everyone's interest list
	// because these pointers are no longer valid and are now pointing
	// at unused memory inside the pool.

	// For each peer in our connected list
	for (Peer& cp : connected) {

		// Keep track of the iterators to items we're going to remove
		std::vector<decltype(cp.interestedList)::iterator> toRemove;

		// Search through its interested list for the peers we removed
		for (auto it = begin(cp.interestedList);
		     it != end(cp.interestedList);
			 it = find_first_of(it, end(cp.interestedList), begin(removing), end(removing))) {

			toRemove.emplace_back(it);
			++it; // on to the next one
		}

		// Remove them in reverse order (think about how vectors work)
		for (auto it = toRemove.rbegin(); it != toRemove.rend(); ++it) {
			// It's an iterator to an iterator. God help us all.
			cp.interestedList.erase(*it);
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

void Simulator::bumpSimCount()
{
	for (Peer& p : connected)
		++p.simCounter;
}
