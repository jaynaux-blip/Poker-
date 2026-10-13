// pro_badges: core-world's fresh distribution (update 2 SPEC §3.8): a Blank variant of the distribution check. Day one has
// no badge a record earns (Team RiverLine 0, Verified 0, no club card, nobody on the wall, no circuit card, no bracelet or
// ring); Team RiverLine signs its first class on results from Nov 1 and has 1 to 10 by Dec 1, every one with an online
// final table or a top-100 Season rank; Verified stays empty until the first monthly review a 1,000-event record can reach
// (Dec 1) and every one it finds holds the record. Phase A, day 1 (the lead): the entry point; core-world fills it, its
// helpers in badges_test::fresh_seg.
#include "BadgesSeg.h"

#include "ShortStack/Game/Badges.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/World.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>

namespace badges_test
{
namespace fresh_seg
{
namespace world = ss::world;
namespace net = ss::net;
namespace badge = ss::badge;

void Check(bool Condition, const std::string& What)
{
	if (!Condition)
	{
		++Failures;
		std::printf("FAILED: %s\n", What.c_str());
	}
}

struct Census
{
	int Team = 0;
	int Verified = 0;
	int Club = 0;
	int Card = 0;
	int Wall = 0;
	int Titled = 0; // a bracelet or a ring
	bool Honest = true; // every Verified badge on a record that earns it
};

Census Count(const world::World& W, int Day)
{
	Census C;
	badge::Index Ix;
	for (const world::Npc& N : W.People())
	{
		const badge::Set& S = Ix.Of(W, N.Id, Day);
		C.Team += S.Find(badge::Kind::TeamRiverLine) ? 1 : 0;
		const bool Verified = S.Find(badge::Kind::VerifiedPro) != nullptr;
		C.Verified += Verified ? 1 : 0;
		C.Honest = C.Honest && (!Verified || N.Stats.PaidEvents() >= badge::VerifiedEvents); // (the ROI was the review's; events only grow)
		C.Club += S.Find(badge::Kind::Club) ? 1 : 0;
		C.Card += S.Find(badge::Kind::CircuitCard) ? 1 : 0;
		C.Wall += S.Find(badge::Kind::OnTheWall) ? 1 : 0;
		C.Titled += S.Find(badge::Kind::Bracelet) || S.Find(badge::Kind::Ring) ? 1 : 0;
	}
	return C;
}
} // namespace fresh_seg

void SegFresh(bool Quick)
{
	namespace fs = fresh_seg;
	namespace world = ss::world;
	namespace net = ss::net;
	const double NightOne = 1440.0 + 127.0;
	const int Nov1 = world::DayOn(2026, 11, 1);
	const int Dec1 = world::DayOn(2026, 12, 1);
	for (const uint32_t Seed : {7u, 1234u, 98765u})
	{
		if (Quick && Seed != 7u)
		{
			continue;
		}
		const auto Start = std::chrono::steady_clock::now();
		const std::string Tag = "fresh distribution, seed " + std::to_string(Seed);
		world::World W;
		W.Create(Seed, NightOne, world::Start::Blank);
		const fs::Census First = fs::Count(W, net::DayOf(W.Clock()));
		fs::Check(First.Team == 0 && First.Verified == 0 && First.Club == 0 && First.Card == 0 && First.Wall == 0 && First.Titled == 0,
			Tag + ": day one has no badge a record earns (Team " + std::to_string(First.Team) + ", Verified " + std::to_string(First.Verified) + ", club " +
				std::to_string(First.Club) + ", card " + std::to_string(First.Card) + ", wall " + std::to_string(First.Wall) + ", titles " + std::to_string(First.Titled) + ")");
		bool EarlyTeam = false;
		bool EarlyVerified = false;
		bool Honest = true;
		int TeamOnDec1 = -1;
		int FirstVerified = -1;
		int Club = 0;
		while (net::DayOf(W.Clock()) <= Dec1)
		{
			W.Simulate(1);
			const int Day = net::DayOf(W.Clock());
			if (Day < Nov1 || Day == Dec1 || Day % 7 == 0)
			{
				const fs::Census C = fs::Count(W, Day);
				EarlyTeam = EarlyTeam || (Day < Nov1 && C.Team > 0);
				EarlyVerified = EarlyVerified || (Day < Dec1 && C.Verified > 0);
				Honest = Honest && C.Honest;
				FirstVerified = FirstVerified < 0 && C.Verified > 0 ? Day - net::DayOf(W.StartedAt()) : FirstVerified;
				TeamOnDec1 = Day == Dec1 ? C.Team : TeamOnDec1;
				Club = C.Club;
			}
		}
		// Every signing on results (the headline's numbers: online final tables, the Season rank at the signing).
		bool OnResults = true;
		int Signed = 0;
		for (const world::WorldEvent& E : W.Events())
		{
			if (E.Kind == world::EventKind::Sponsored && E.What == "Team RiverLine")
			{
				++Signed;
				// (Value: the Season rank at the signing plus 1,000 a final table on record; Amount stays 0, the news reads it as
				// money.)
				const world::Npc* N = W.Get(E.Npc);
				const int Fts = E.Value / 1000;
				const int Rank = E.Value % 1000;
				OnResults = OnResults && N && net::DayOf(E.At) >= Nov1 && E.Amount == 0 && (Fts >= 1 || (Rank >= 1 && Rank <= 100)) &&
					N->Totals[static_cast<size_t>(world::Venue::Online)].FinalTables >= Fts;
			}
		}
		fs::Check(!EarlyTeam, Tag + ": Team RiverLine signs nobody before Nov 1");
		fs::Check(TeamOnDec1 >= 1 && TeamOnDec1 <= 10 && Signed >= TeamOnDec1 && OnResults,
			Tag + ": Team RiverLine has 1 to 10 by Dec 1 (" + std::to_string(TeamOnDec1) + "), every one signed on results");
		fs::Check(!EarlyVerified && Honest, Tag + ": nobody is Verified before the Dec 1 review, and every Verified badge rests on 1,000 paid events on the books");
		std::printf("  %s (%.1f s): Team %d on Dec 1, first Verified on day %d, %d club cards on Dec 1\n", Tag.c_str(),
			std::chrono::duration<double>(std::chrono::steady_clock::now() - Start).count(), TeamOnDec1, FirstVerified, Club);
	}
}
} // namespace badges_test
