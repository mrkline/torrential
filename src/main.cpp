#include <cstdio>
#include <tclap/CmdLine.h>

#include "Simulator.hpp"

// Lee Stratman added this comment and Justin Krosschell revised it.
int main(int argc, char** argv)
{
	using namespace TCLAP;

	CmdLine cmd("Torrential - the BitTorrent simulator");

	ValueArg<int> peerArg("p", "peers", "Peers in the simulation", true, 50, "number of peers");
	ValueArg<int> chunkArg("c", "chunks", "Chunks in the complete torrent", true, 50, "number of chunks");
	ValueArg<double> joinProbArg("j", "join-probab", "The probability a peer will join in a given tick",
	                             false, 0.2, "join probability");
	ValueArg<double> leaveProbArg("l", "leave-prob", "The probability that a peer will leave in a given tick",
	                              false, 0.01, "leave probability");

	cmd.add(peerArg);
	cmd.add(chunkArg);
	cmd.add(joinProbArg);
	cmd.add(leaveProbArg);
	cmd.parse(argc, argv);

	if (peerArg.getValue() < 2) {
		fprintf(stderr, "You cannot have fewer than two peers.\n");
		return 1;
	}
	if (chunkArg.getValue() < 2) {
		fprintf(stderr, "You cannot have fewer than two chunks.\n");
		return 1;
	}

	if (joinProbArg.getValue() <= 0.0) {
		fprintf(stderr, "Peers must join at some positive rate.\n");
		return 1;
	}

	if (leaveProbArg.getValue() < 0.0) {
		fprintf(stderr, "Peers cannot leave at a negative rate.\n");
		return 1;
	}

	Simulator sim(peerArg.getValue(), chunkArg.getValue(), joinProbArg.getValue(), leaveProbArg.getValue());

	while (!sim.allDone())
		sim.tick();

	printf("Finished in %d ticks (seconds)\n", sim.getTickCount());

	return 0;
}
