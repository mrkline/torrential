#include <cstdio>
#include <tclap/CmdLine.h>

#include "Simulator.hpp"

// Lee Stratman added this comment and Justin Krosschell revised it.
int main(int argc, char** argv)
{
	using namespace TCLAP;

	CmdLine cmd("Torrential - the BitTorrent simulator");

	ValueArg<int> peerArg("p", "peers", "Peers in the simulation", true, 50, "integer");
	ValueArg<int> chunkArg("c", "chunks", "Chunks in the complete torrent", true, 50, "integer");

	cmd.add(peerArg);
	cmd.add(chunkArg);
	cmd.parse(argc, argv);

	if (peerArg.getValue() < 2) {
		fprintf(stderr, "You cannot have fewer than two peers\n");
		return 1;
	}
	if (chunkArg.getValue() < 2) {
		fprintf(stderr, "You cannot have fewer than two chunks\n");
		return 1;
	}

	Simulator sim(peerArg.getValue(), chunkArg.getValue());

	while (!sim.allDone())
		sim.tick();

	printf("Finished in %d ticks (seconds)\n", sim.getTickCount());

	return 0;
}
