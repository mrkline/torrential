import std.algorithm;
import std.array;
import std.stdio;
import std.process;
import std.exception;
import std.conv;

int main(string args[])
{
	args = args[1 .. $];

	if (args.length < 2) {
		stderr.writeln("Usage: tstats <num runs> <torrential args>");
		return 1;
	}

	immutable int runs = args[0].to!int;

	args = args[1 .. $];

	// Light up torrential
	for (int i; i < runs; ++i) {
		writeln("\nRun ", i, ":");
		doRun(args);
	}

	return 0;
}

void doRun(string args[])
{
	args ~= "-m";

	writeln("Flags: ", args.join(" "));

	auto sim = pipeProcess("./torrential" ~ args, Redirect.stdout);

	struct Peer {
		int lastConnect;
		int up;
		int down;
		int totalTicks;
		int downloadTicks;
		int uploaded;
		bool disconnected;

		this(int conn, int u, int d)
		{
			lastConnect = conn;
			up = u;
			down = d;
		}
	}

	Peer[int] peers;

	int lastTick;
	int totalChunks;

	foreach (line; sim.stdout.byLine) {
		string[] tokens = line.idup.split();
		enforce(tokens[0].length == 1, "Unknown, multi-char line code");

		switch(tokens[0]) {
			case "t":
				enforce(tokens.length == 2, "Invalid tick line");
				lastTick = tokens[1].to!int;
				break;

			case "c":
				enforce(tokens.length == 4, "Invalid connect line");
				int id = tokens[1].to!int;
				Peer* p = id in peers;
				if (p != null) {
					p.disconnected = false;
				}
				else  {
					
					peers[id] = Peer(lastTick, tokens[2].to!int, tokens[3].to!int);
				}
				break;

			case "d":
				enforce(tokens.length == 2, "Invalid disconnect line");
				int id = tokens[1].to!int;
				Peer* p = id in peers;
				p.totalTicks += lastTick - p.lastConnect + 1;
				p.disconnected = 1;
				break;

			case "x":
				enforce(tokens.length == 4, "Invalid transmit line");
				peers[tokens[1].to!int].uploaded += 1;
				break;

			case "f":
				enforce(tokens.length == 3, "Invalid finish time");
				Peer* p = tokens[1].to!int in peers;
				p.downloadTicks = lastTick - p.lastConnect + 1 + p.totalTicks;
				totalChunks = tokens[2].to!int;
				break;

			default:
				stderr.writeln("Unexptected line type found (", tokens[0], ")");
		}
	}

	writeln("Total ticks: ", lastTick);

	int totalUpload = 0;
	int minDown = int.max;
	foreach (ref p; peers) {
		totalUpload += p.up;
		minDown = min(minDown, p.down);
	}

	Peer* seeder = 0 in peers;
	writeln("Client-server optimum: ", max(cast(double)(peers.length * totalChunks) / cast(double)seeder.up,
	                                       cast(double)totalChunks / cast(double)minDown));
	writeln("Peer-Peer optimum ", max(cast(double)totalChunks / cast(double)seeder.up,
	                                 cast(double)totalChunks / cast(double)minDown,
	                                 cast(double)(peers.length * totalChunks) / cast(double)(totalUpload)));

	writeln();

	// Finish out everyone's times and write everything out
	writeln("Peer, Upload, Download, Download Ticks, Total Ticks, Chunks Sent");
	foreach (id; peers.keys.sort) {
		Peer* p = id in peers;
		if (!p.disconnected)
			p.totalTicks += lastTick - p.lastConnect + 1;

		writeln(id, ", ", p.up, ", ", p.down, ", ", p.downloadTicks, ", ", p.totalTicks, ", ", p.uploaded);
	}
}
