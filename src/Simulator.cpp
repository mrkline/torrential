#include "Simulator.hpp"

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
 *    - Check the sim counter. If counter % == 0, re-eval top 4 peers
 *      - Reorder list of connected peers
 *      - Empty history structure (who we've gotten data from)
 *    - If counter % == 0, optimally unchoke a peer
 *    - If number in chunklist < 20, get more peers randomly
 *
 * 4. Generate a list of offers for each peer,
 *    based on the upload rates of those offering and how many they are offering to.
 *
 * 5. Have each peer accept as many of its offers as possible,
 *    based on its download rate. Update the chunk lists accordingly.
 */
void Simulator::tick()
{
}
