#pragma once

#include <random>

#include "Pool.hpp"
#include "Peer.hpp"

/// The whole shebang. Holds our list of connected and disconnected peers.
class Simulator {
public:

	Simulator(size_t numClients);

	void tick();

	void connectPeers();

private:

	// TODO: Take a list of connections we already have to filter those out
	std::vector<Connection> getRandomPeerConnections(size_t num,
	                                                 const std::vector<Connection>& ignore = std::vector<Connection>());

	Pool<Peer> connected; ///< The clients who are currently connected
	Pool<Peer> disconnected; ///< The clients who are currently disconnected

	// C++11 random number magic. See
	// http://en.cppreference.com/w/cpp/numeric/random

	std::mt19937 rng; ///< Random number generator used to get our probabilities
	std::bernoulli_distribution shouldConnect; ///< Should the peer connect?
	std::bernoulli_distribution shouldDisconnect; ///< Should the peer disconnect?
};
