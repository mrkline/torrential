#pragma once

class Connection {
public:
	// member variables
	int IPAddress;  //other peer's IP address
	bool interested = false; //bit indicating this peer is interested in chunks from other peer
	bool choked = true;  //bit indicating this peer is choking out the connection

	// functions
	Connection(int IP);
};
