// Painstakingly written by Justin Krosschell
// ECE 537 bittorrent simulator

#include <cstdio>
#include <vector>

class Connection {
public:
	// member variables
	int IPAddress;  //other peer's IP address
	bool interested = false; //bit indicating this peer is interested in chunks from other peer
	bool choked = true;  //bit indicating this peer is choking out the connection
	
	// functions
	Connection(int IP);
};


class Peer {
public:
	// member variables
	int simCounter;  //peer's simulation counter
	int IPAddress;  //peer's IP address
	int uploadRate;  //peer's upload rate (should be a multiple of 256kB/s)
	int downloadRate;  //peer's download rate (roughly 10X the upload rate)
	std::vector<bool> chunkList;  //boolean array list of chunks that the peer has
	std::vector<Connection> connectionList;  //connection array list of peers that this peer can request chunks from
};
