// Painstakingly written by Justin Krosschell
// ECE 537 bittorrent simulator

#pragma once

#include <vector>

#include "Connection.hpp"

class Peer {
public:
	// member variables
	int simCounter = 0;  //peer's simulation counter
	int IPAddress;  //peer's IP address
	int uploadRate;  //peer's upload rate (should be a multiple of 256kB/s)
	int downloadRate;  //peer's download rate (roughly 10X the upload rate)
	std::vector<bool> chunkList;  //boolean array list of chunks that the peer has
	std::vector<Connection> connectionList;  //connection array list of peers that this peer can request chunks from

	Peer(int IP, int upload, int download);
};