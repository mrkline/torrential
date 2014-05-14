#pragma once

#include <cstddef> // for size_t

class Peer;

void printMachineOutput(bool forMachines);

void printTick(int tickNum);

void printConnection(const Peer& p);

void printDisconnection(int id);

void printTransmit(int from, size_t chunk, int to);

void printFinished(int id, size_t totalChunks);
