#pragma once

#include <random>
#include <vector>
#include <unordered_map>

#include "Pool.hpp"
#include "Peer.hpp"

/// The whole shebang. Holds our list of connected and disconnected peers.
class Simulator {
public:

	/// A map that maps dest -> a list of pairs in the form of (source, chunk indices)
	typedef std::unordered_map<Peer*, std::vector<std::pair<Peer*, std::vector<size_t>>>> OfferMap;

	Simulator(size_t numClients);

	void tick();

private:

	void connectPeers();

	/**
	 * Gets a map of all offers from one connected peer to another
	 * \returns A map mapping dest -> a list of pairs in the form of (source, chunk indices)
	 */
	OfferMap makeOffers();
	
	void acceptOffers(OfferMap& offers);

	void bumpSimCount();

	std::vector<Peer*> getRandomPeers(size_t num,
	                                  const std::vector<Peer*>& ignore = std::vector<Peer*>());

	Pool<Peer> connected; ///< The clients who are currently connected
	Pool<Peer> disconnected; ///< The clients who are currently disconnected

	// C++11 random number magic. See
	// http://en.cppreference.com/w/cpp/numeric/random

	std::mt19937 rng; ///< Random number generator used to get our probabilities
	std::bernoulli_distribution shouldConnect; ///< Should the peer connect?
	std::bernoulli_distribution shouldDisconnect; ///< Should the peer disconnect?
};
