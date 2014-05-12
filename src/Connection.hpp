#pragma once

class Peer;

class Connection {
public:
	// member variables
	Peer* to; ///<  The peer we're connected to
	bool interested = false; ///< bit indicating this peer is interested in chunks from other peer
	bool choked = true;  ///< bit indicating this peer is choking out the connection

	// functions
	Connection(Peer* connectedTo);

	/// Equality. A connection is considered equal if it's pointing to the same peer
	bool operator==(const Connection& o) const { return to == o.to; }
};
