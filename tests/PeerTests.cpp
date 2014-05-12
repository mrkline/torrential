#include "PeerTests.hpp"

#include "Test.hpp"
#include "Peer.hpp"

namespace {

void everythingTest()
{
	Peer p(1, 2, 3);
	p.chunkList = { true, false, true };
	assert(!p.hasEverything());
	p.chunkList[1] = true;
	assert(p.hasEverything());
}

void simpleOffers()
{
	// Offer one chunk
	{
		Peer p1(1, 1, 1);
		Peer p2(2, 1, 1);

		p1.chunkList = { true };
		p2.chunkList = { false };

		p1.interestedList = { {&p2, 0} };

		auto offers = p1.makeOffers();
		assert(offers.size() == 1);
		assert(offers[0].first == &p2); // Offering to p2
		assert(offers[0].second.size() == 1); // Should only offer one chunk
		assert(offers[0].second[0] == 0); // Should be offering chunk 0
	}
	// Offer no chunks because we don't have any
	{
		Peer p1(1, 1, 1);
		Peer p2(2, 1, 1);

		p1.chunkList = { false };
		p2.chunkList = { false };

		p1.interestedList = { {&p2, 0} };

		auto offers = p1.makeOffers();
		// We don't care if we make an offer or not, but if we do,
		// it had better be a zero-sized offer
		if (!offers.empty()) {
			assert(offers[0].first == &p2); // Offering to p2
			assert(offers[0].second.size() == 0); // Should offer nothing
		}
	}
	// Offer no chunks because everyone has them
	{
		Peer p1(1, 1, 1);
		Peer p2(2, 1, 1);

		p1.chunkList = { true };
		p2.chunkList = { true };

		p1.interestedList = { {&p2, 0} };

		auto offers = p1.makeOffers();
		// We don't care if we make an offer or not, but if we do,
		// it had better be a zero-sized offer
		if (!offers.empty()) {
			assert(offers[0].first == &p2); // Offering to p2
			assert(offers[0].second.size() == 0); // Should offer nothing
		}
	}
	// Make sure we're offering the right chunk
	{
		Peer p1(1, 1, 1);
		Peer p2(2, 1, 1);

		p1.chunkList = { false, false, true };
		p2.chunkList = { false, false, false };

		p1.interestedList = { {&p2, 0} };

		auto offers = p1.makeOffers();
		assert(offers.size() == 1);
		assert(offers[0].first == &p2); // Offering to p2
		assert(offers[0].second.size() == 1); // Should only offer one chunk
		assert(offers[0].second[0] == 2); // Should be offering chunk 2
	}
	// Make sure we're offering multiple chunks
	{
		Peer p1(1, 2, 1);
		Peer p2(2, 1, 1);

		p1.chunkList = { true, false, true };
		p2.chunkList = { false, false, false };

		p1.interestedList = { {&p2, 0} };

		auto offers = p1.makeOffers();
		assert(offers.size() == 1);
		assert(offers[0].first == &p2);
		assert(offers[0].second.size() == 2);
		assert(offers[0].second[0] == 0);
		assert(offers[0].second[1] == 2);
	}
	// Make sure we're not offering multiple if we don't have the bandwidth
	{
		Peer p1(1, 1, 1);
		Peer p2(2, 1, 1);

		p1.chunkList = { true, false, true };
		p2.chunkList = { false, false, false };

		p1.interestedList = { {&p2, 0} };

		auto offers = p1.makeOffers();
		assert(offers.size() == 1);
		assert(offers[0].first == &p2);
		assert(offers[0].second.size() == 1);
		assert(offers[0].second[0] == 0);
	}
}

} // end anonymous namespace

void Testing::runPeerTests()
{
	beginUnit("Peer");
	test("Has everything", &everythingTest);
	test("Simple offers", &simpleOffers);
}
