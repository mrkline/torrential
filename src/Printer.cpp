#include "Printer.hpp"

#include <cstdio>

namespace {

bool machineOutput = false;

}

void printMachineOutput(bool forMachines)
{
	machineOutput = forMachines;
}

void printTick(int tickNum)
{
	if (!machineOutput)
		return;

	printf("t %d\n", tickNum);
}

void printConnection(int id)
{
	if (machineOutput)
		printf("c %d\n", id);
	else
		printf("Peer %d connecting\n", id);
}

void printDisconnection(int id)
{
	if (machineOutput)
		printf("d %d\n", id);
	else
		printf("Peer %d disconnecting\n", id);
}

void printTransmit(int from, size_t chunk, int to)
{
	if (machineOutput)
		printf("x %d %zu %d\n", from, chunk, to);
	else
		printf("Peer %d sending chunk %zu from to %d\n", from, chunk, to);
}

void printFinished(int id, size_t totalChunks)
{
	if (machineOutput)
		printf("f %d %zu\n", id, totalChunks);
	else
		printf("Peer %d finished (%zu total chunks)\n", id, totalChunks);
}
