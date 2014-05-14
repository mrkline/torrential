#include <cstdio>
#include <tclap/CmdLine.h>

#include "Simulator.hpp"

using namespace std;

namespace {

void howAboutNo(const char* c)
{
	fprintf(stderr, "%s\n", c);
	exit(1);
}

}

// TLCAP black magic: http://tclap.sourceforge.net/manual.html
namespace TCLAP {
	template<class T, class U>
	struct ArgTraits<std::pair<T, U>> {
		typedef ValueLike ValueCategory;
	};
}

// Used by TCLAP to parse in our min/max pairs.
// TODO: Doesn't check for errors or anything. Might wanna add that later
void operator>>(istringstream& iss, pair<int, int>& p)
{
	char comma;
	iss >> p.first >> comma >> p.second;
}

// Lee Stratman added this comment and Justin Krosschell revised it.
int main(int argc, char** argv)
{
	using namespace TCLAP;

	CmdLine cmd("Torrential - the BitTorrent simulator");

	ValueArg<int> peerArg("p", "peers", "Peers in the simulation", true, 50, "number of peers");
	ValueArg<int> chunkArg("c", "chunks", "Chunks in the complete torrent", true, 50, "number of chunks");
	ValueArg<double> joinProbArg("j", "join-prob", "The probability a peer will join in a given tick",
	                             false, 0.2, "join probability");
	ValueArg<double> leaveProbArg("l", "leave-prob", "The probability that a peer will leave in a given tick",
	                              false, 0.01, "leave probability");
	ValueArg<pair<int,int>> uploadArg("u", "upload-range", "The range (in chunks) of uplaod rates for each peer",
	                                  false, pair<int, int>(10, 10), "min,max");
	ValueArg<pair<int, int>> downloadArg("d", "download-range", "The range (in chunks) of download rates for each peer",
	                                     false, pair<int, int>(100, 100), "min,max");
	ValueArg<int> freeriderArg("f", "freeriders", "The number of free riders", false, 0, "number of free riders");

	cmd.add(peerArg);
	cmd.add(chunkArg);
	cmd.add(joinProbArg);
	cmd.add(leaveProbArg);
	cmd.add(uploadArg);
	cmd.add(downloadArg);
	cmd.add(freeriderArg);
	cmd.parse(argc, argv);

	const auto peers = peerArg.getValue();
	const auto chunks = chunkArg.getValue();
	const auto joinProb = joinProbArg.getValue();
	const auto leaveProb = leaveProbArg.getValue();
	const auto upload = uploadArg.getValue();
	const auto download = downloadArg.getValue();
	const auto frees = freeriderArg.getValue();

	if (peers < 2)
		howAboutNo("You cannot have fewer than two peers.");

	if (chunks < 2)
		howAboutNo("You cannot have fewer than two chunks.");

	if (joinProb <= 0.0)
		howAboutNo("Peers must join at some positive rate.");

	if (leaveProb < 0.0)
		howAboutNo("Peers cannot leave at a negative rate.");

	if (joinProb < leaveProb) {
		howAboutNo("Peers cannot leave more ofthen than they will join;"
		           "the torrent will likely never finish.\n");
	}

	if (upload.first > upload.second)
		howAboutNo("Upload min cannot be greater than the upload max");

	if (download.first > download.second)
		howAboutNo("Download min cannot be greater than the download max");

	if (frees < 0)
		howAboutNo("You cannot have a negative number of free riders.");

	if (peers - frees < 1)
		howAboutNo("At least one peer cannot be a free rider");

	Simulator sim(peers, chunks, joinProb, leaveProb, upload, download, frees);

	while (!sim.allDone())
		sim.tick();

	printf("Finished in %d ticks (seconds)\n", sim.getTickCount());

	return 0;
}
