#pragma once

#include <cstddef> // for size_t

void printMachineOutput(bool forMachines);

void printTick(int tickNum);

void printConnection(int id);

void printDisconnection(int id);

void printTransmit(int from, size_t chunk, int to);

void printFinished(int id, size_t totalChunks);
