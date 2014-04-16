// Painstakingly written by Justin Krosschell
// ECE 537 bittorrent simulator

#include "peer.hpp"

// Connection sets the IP address of the connection object
Connection::Connection (int IP) :
	IPAddress(IP)
{
	// don't do anyting, but this is cool
}

Peer::Peer (int IP, int upload, int download) :
	IPAddress(IP),
	uploadRate(upload),
	downloadRate(download)
{
	// don't do anything, but it is cool
}

// Algorithm
// for each node
