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
}

void Simulator::connectPeers()
{
	// Go through the disconnected peers, connecting some at random
	for (auto it = begin(disconnected); it != end(disconnected);) {
		if (shouldConnect(rng)) {
			// Initialize it
			it->simCounter = 0; // sim counter gets reset
			it->connectionList = getRandomPeerConnections(Peer::desiredPeerCount); // Get us some peers

			// Move the peer to the connected list
			connected.construct(std::move(*it));

			// Then remove it from the disconnected list
			it = disconnected.destroy(it);
		}
		else
			++it;
	}
}

std::vector<Connection> Simulator::getRandomPeerConnections(size_t num,
                                                            const std::vector<Connection>& ignore)
{
	vector<Connection> ret; // The one we're going to return
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

		// Make connections from those
		for (Peer* pp : peerList)
			ret.emplace_back(pp);
	}

	// Remove peers we already have from the results
	// TODO: This is n^2, but do smarter sorting or something later
	for (const Connection& conn : ignore) {
		auto it = find(begin(ret), end(ret), conn);
		if (it != end(ret))
			ret.erase(it);
	}

	return ret;
}
