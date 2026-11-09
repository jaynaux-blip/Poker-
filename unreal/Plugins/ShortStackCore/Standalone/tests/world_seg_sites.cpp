// living_world: core-world's sites checks (update 2 SPEC §6.7, G3): memberships deterministic per seed and drawing nothing
// from the existing forks; a v3 world migrating to v4 with RiverLine's boards unchanged on day 0; NPCs playing rival events
// and hunting overlays; teams per site; the world text within +10% after 30 days; EventKindName covering Count;
// RanksYesterday never indexed past Board::Live. Also the traps review_ready.md gave core-world (a rival's series or micro
// event never touches RiverLine's Series board or Night Shift; every rival world event carries its site), the boards by
// subtraction, Player of the Year on RiverLine's points, the launch days, the save, and the copy.
#include "WorldSeg.h"

#include "CopyLint.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/SitePlay.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Game/WorldLeague.h"
#include "ShortStack/Rng.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace world_test
{
// world_main.cpp's: "world_test long", and the suite's invariants (their failures counted).
extern bool LongRun;
int CheckInvariants(const ss::world::World& W, const std::string& When);

namespace sites_seg
{
namespace world = ss::world;
namespace net = ss::net;

int Failures = 0;

void Check(bool Ok, const std::string& What)
{
	if (!Ok)
	{
		++Failures;
		std::printf("FAIL: %s\n", What.c_str());
	}
}

const double NightOne = 1440.0 + 127.0;
const double Day = 1440.0;

double Seconds(std::chrono::steady_clock::time_point Since)
{
	return std::chrono::duration<double>(std::chrono::steady_clock::now() - Since).count();
}

std::vector<std::string> Tabs(const std::string& Line)
{
	std::vector<std::string> Out(1);
	for (const char C : Line)
	{
		if (C == '\t')
		{
			Out.emplace_back();
		}
		else if (C != '\r')
		{
			Out.back() += C;
		}
	}
	return Out;
}

void Load(world::World& W, const std::string& Text)
{
	std::istringstream In(Text);
	std::string Line;
	while (std::getline(In, Line))
	{
		W.Read(Tabs(Line));
	}
	W.Finish();
}

std::string Text(const world::World& W)
{
	std::string Out;
	W.Write(Out);
	return Out;
}

std::vector<std::string> Lines(const std::string& T)
{
	std::vector<std::string> Out;
	std::istringstream In(T);
	std::string Line;
	while (std::getline(In, Line))
	{
		Out.push_back(Line);
	}
	return Out;
}

int Bit(net::SiteId S)
{
	return 1 << static_cast<int>(S);
}

int Today(const world::World& W)
{
	return static_cast<int>(std::floor(W.Clock() / Day));
}

bool Registered(net::SiteId S)
{
	return net::Def(S).Id == S;
}

/** A site's team pro ("Team RunItTwice"). */
bool OnTeam(const world::Npc& N, net::SiteId S)
{
	const net::SiteDef& D = net::Def(S);
	return D.Id == S && !D.TeamName.empty() && N.Sponsor == D.TeamName;
}

/**
 * §5.9's membership rule, worked out here from the fork it names ("world/<seed>/sites/<name>") with the numbers the spec
 * gives, so the world's own can't drift from it: RunItTwice 22% of the online regulars (OnlineShare >= 0.5) at Low and up and
 * 8% of Micro ones; Halcyon 30% of High; Ignite from its launch, 30% of Micro and 10% of Low (OnlineShare >= 0.4) each on a
 * join day in its first 30, and 40% of the online arrivals since (a fifth of them on Ignite alone).
 */
int Rule(const world::World& W, const world::Npc& N, int On)
{
	ss::Rng R("world/" + std::to_string(W.Seed()) + "/sites/" + N.Name);
	const double UTwice = R.Next();
	const double UHalcyon = R.Next();
	const double UIgnite = R.Next();
	const double UOnly = R.Next();
	int Bits = Bit(net::SiteId::RiverLine);
	const double Twice = N.Tier >= 2 ? 0.22 : N.Tier == 1 ? 0.08 : 0.0;
	if (N.OnlineShare >= 0.5f && UTwice < Twice)
	{
		Bits |= Bit(net::SiteId::RunItTwice);
	}
	if (N.OnlineShare >= 0.5f && N.Tier >= 4 && UHalcyon < 0.30)
	{
		Bits |= Bit(net::SiteId::Halcyon);
	}
	const double Launched = W.SiteLaunched(net::SiteId::Ignite);
	const int Launch = Launched >= 0.0 ? static_cast<int>(std::floor(Launched / Day)) : -1;
	if (Launch >= 0 && On >= Launch && N.OnlineShare >= 0.4f)
	{
		if (N.Came != world::Arrival::None && N.Arrived >= Launch)
		{
			if (UIgnite < 0.40)
			{
				Bits = UOnly < 0.20 ? Bit(net::SiteId::Ignite) : Bits | Bit(net::SiteId::Ignite);
			}
		}
		else
		{
			const double Share = N.Tier == 1 ? 0.30 : N.Tier == 2 ? 0.10 : 0.0;
			if (Share > 0.0 && UIgnite < Share && On >= Launch + static_cast<int>(UIgnite / Share * 30.0))
			{
				Bits |= Bit(net::SiteId::Ignite);
			}
		}
	}
	return Bits;
}

/** Every rival's template names that RiverLine's schedule doesn't have (what its News must never mention). */
std::vector<std::string> RivalNames()
{
	std::set<std::string> Home;
	for (const net::EventTemplate& T : net::Shared().Templates())
	{
		Home.insert(T.Name);
	}
	std::set<std::string> Out;
	for (const net::SiteId S : {net::SiteId::Ignite, net::SiteId::RunItTwice})
	{
		for (const net::EventTemplate& T : net::Site(S).Templates())
		{
			if (T.Name.size() >= 8 && Home.count(T.Name) == 0)
			{
				Out.insert(T.Name);
			}
		}
	}
	return std::vector<std::string>(Out.begin(), Out.end());
}

// ------------------------------------------------------------------ memberships (§5.9)

void Memberships(const world::World& W, const world::World& Again, const world::World& Other, const world::World& Fresh)
{
	const int On = Today(W);
	int Wrong = 0;
	std::string First;
	int Pros = 0;
	int ProsHome = 0;
	for (const world::Npc& N : W.People())
	{
		if (N.Faded)
		{
			continue;
		}
		int Want = Rule(W, N, On);
		if (N.Pro)
		{
			++Pros;
			ProsHome += N.Sites == Bit(net::SiteId::RiverLine) ? 1 : 0;
			continue;
		}
		if (OnTeam(N, net::SiteId::RunItTwice))
		{
			Want = Bit(net::SiteId::RunItTwice);
		}
		if (N.Sites != Want)
		{
			First = First.empty() ? N.Name + " " + std::to_string(N.Sites) + " (rule " + std::to_string(Want) + ")" : First;
			++Wrong;
		}
	}
	Check(Wrong == 0, "sites: every regular's accounts on Night One are §5.9's rule from their own fork (" + std::to_string(Wrong) + " off, " + First + ")");
	Check(Pros >= 1 && ProsHome == Pros, "sites: Team RiverLine plays only on RiverLine (" + std::to_string(ProsHome) + " of " + std::to_string(Pros) + ")");
	// The same seed, the same accounts; another seed, others.
	bool Same = W.People().size() == Again.People().size();
	for (size_t K = 0; Same && K < W.People().size(); ++K)
	{
		Same = W.People()[K].Sites == Again.People()[K].Sites && W.People()[K].Name == Again.People()[K].Name;
	}
	Check(Same, "sites: the same seed gives everyone the same accounts");
	int Differ = 0;
	for (const world::Npc& N : Other.People())
	{
		const world::Npc* M = W.Get(W.Find(N.Name));
		Differ += M && M->Sites != N.Sites ? 1 : 0;
	}
	Check(Differ > 20, "sites: another seed gives others (" + std::to_string(Differ) + " regulars differ)");
	// A fresh start seeds them the same way (a membership isn't a record); only the established world's team differs.
	int FreshWrong = 0;
	for (const world::Npc& N : Fresh.People())
	{
		const world::Npc* M = W.Get(N.Id);
		if (M && !M->Pro && !N.Pro && !OnTeam(*M, net::SiteId::RunItTwice) && M->Sites != N.Sites)
		{
			++FreshWrong;
		}
	}
	Check(FreshWrong == 0, "sites: a fresh start seeds the same accounts (" + std::to_string(FreshWrong) + " differ)");
	// The shares, against the rule's.
	int Low = 0;
	int LowIn = 0;
	int Micro = 0;
	int MicroIn = 0;
	int High = 0;
	int HighIn = 0;
	for (const world::Npc& N : W.People())
	{
		if (N.From != world::Origin::Founding || N.Pro || OnTeam(N, net::SiteId::RunItTwice) || N.OnlineShare < 0.5f)
		{
			continue;
		}
		const bool Twice = (N.Sites & Bit(net::SiteId::RunItTwice)) != 0;
		(N.Tier >= 2 ? Low : Micro) += N.Tier >= 1 ? 1 : 0;
		(N.Tier >= 2 ? LowIn : MicroIn) += N.Tier >= 1 && Twice ? 1 : 0;
		High += N.Tier >= 4 ? 1 : 0;
		HighIn += N.Tier >= 4 && (N.Sites & Bit(net::SiteId::Halcyon)) != 0 ? 1 : 0;
	}
	const double LowShare = Low > 0 ? static_cast<double>(LowIn) / Low : 0.0;
	const double MicroShare = Micro > 0 ? static_cast<double>(MicroIn) / Micro : 0.0;
	const double HighShare = High > 0 ? static_cast<double>(HighIn) / High : 0.0;
	std::printf("  sites: RunItTwice on Night One: %d of %d Low and up (%.1f%%), %d of %d Micro (%.1f%%); Halcyon (dormant) %d of %d High\n", LowIn, Low, LowShare * 100.0, MicroIn,
		Micro, MicroShare * 100.0, HighIn, High);
	Check(LowShare >= 0.17 && LowShare <= 0.27 && MicroShare >= 0.04 && MicroShare <= 0.12, "sites: RunItTwice opens with about 22% of the regulars at Low and up, 8% of Micro");
	Check(High < 20 || (HighShare >= 0.18 && HighShare <= 0.42), "sites: Halcyon's 30% of High is seeded now (dormant until P1)");
}

// ------------------------------------------------------------------ launches and teams on the first night

void FirstNight(const world::World& W, const world::World& Fresh)
{
	Check(W.SiteLaunched(net::SiteId::RiverLine) == 0.0 && W.SiteLaunched(net::SiteId::RunItTwice) == 1.0 * Day && W.SiteLaunched(net::SiteId::Ignite) == 28.0 * Day &&
			  W.SiteLaunched(net::SiteId::Halcyon) < 0.0,
		"sites: RunItTwice opened on Night One (Oct 6), Ignite opens Nov 2 (day 28), Halcyon isn't in the world (P1)");
	int Launches = 0;
	bool Flagged = true;
	for (const world::WorldEvent& E : W.Events())
	{
		if (E.Kind == world::EventKind::SiteLaunched)
		{
			++Launches;
			Flagged = Flagged && world::FlagSiteOf(E.Flags) == net::SiteId::RunItTwice && E.What == "runittwice";
		}
	}
	Check(Launches == 1 && Flagged, "sites: the first night's news has RunItTwice's new lobby (on its own page), and nothing of Ignite yet");
	// Team RunItTwice: an established world starts with its six, signed already (no news), playing only on RunItTwice.
	const int Size = net::Def(net::SiteId::RunItTwice).TeamSize > 0 ? net::Def(net::SiteId::RunItTwice).TeamSize : 6;
	int Team = 0;
	bool Shape = true;
	for (const world::Npc& N : W.People())
	{
		if (OnTeam(N, net::SiteId::RunItTwice))
		{
			++Team;
			Shape = Shape && !N.Pro && N.Professional && N.Sites == Bit(net::SiteId::RunItTwice) && N.RepOf(world::Rep::Overall) >= 35.0f;
		}
	}
	int FreshTeam = 0;
	for (const world::Npc& N : Fresh.People())
	{
		FreshTeam += OnTeam(N, net::SiteId::RunItTwice) || N.Pro ? 1 : 0;
	}
	bool Quiet = true;
	for (const world::WorldEvent& E : W.Events())
	{
		Quiet = Quiet && E.Kind != world::EventKind::SignedSiteTeam;
	}
	Check(Team == Size && Shape && Quiet, "sites: an established world's Team RunItTwice has its " + std::to_string(Size) + " on the first night, known professionals playing only there (" +
											  std::to_string(Team) + ")");
	Check(FreshTeam == 0, "sites: a fresh world's site teams start empty");
}

// ------------------------------------------------------------------ the world on several sites, a season's worth

struct Watch
{
	int RivalEntries = 0;
	std::map<net::SiteId, int> BySite;
	int OverlayEvents = 0;
	int OverlayEntries = 0;
	int OverlaySkilled = 0;
	int GtdEvents = 0;
	int GtdEntries = 0;
	int GtdSkilled = 0;
	bool SeriesClean = true;
	std::string SeriesSeen;
	bool RivalRace = false;
	bool NightClean = true;
	bool SeriesPointsClean = true;
	bool IgniteEarly = false;
	int IgniteOnly = 0;
	// RunItTwice's members (on RiverLine too, no team) before Ignite opens: their entries on each site (§5.9's traffic weight).
	int TwiceHome = 0;
	int TwiceAway = 0;
};

/** RunItTwice's members' entries on Day, on RiverLine and on RunItTwice (Oct 8 to Nov 1: two sites, no rival series yet). */
void TrafficShare(const world::World& W, int D, Watch& Out)
{
	if (D < 3 || D > 26)
	{
		return;
	}
	for (const net::SiteId S : {net::SiteId::RiverLine, net::SiteId::RunItTwice})
	{
		for (const net::EventInstance& E : net::Site(S).Window(D * Day, (D + 1) * Day))
		{
			for (const int Id : W.RegisteredNow(E.Id))
			{
				const world::Npc* N = W.Get(Id);
				if (N && (N->Sites & Bit(net::SiteId::RiverLine)) != 0 && (N->Sites & Bit(net::SiteId::RunItTwice)) != 0 && !OnTeam(*N, net::SiteId::RunItTwice))
				{
					(S == net::SiteId::RiverLine ? Out.TwiceHome : Out.TwiceAway) += 1;
				}
			}
		}
	}
}

/** Tomorrow's registrations on every rival (the world has planned them by now): who plays, which events overlay. */
void Registrations(world::World& W, int D, Watch& Out)
{
	for (const net::SiteId S : {net::SiteId::Ignite, net::SiteId::RunItTwice})
	{
		const net::Network& Net = net::Site(S);
		for (const net::EventInstance& E : Net.Window(D * Day, (D + 1) * Day))
		{
			const net::EventTemplate& T = Net.TemplateOf(E);
			const std::vector<int> Who = W.RegisteredNow(E.Id);
			Out.RivalEntries += static_cast<int>(Who.size());
			Out.BySite[S] += static_cast<int>(Who.size());
			Out.IgniteEarly = Out.IgniteEarly || (S == net::SiteId::Ignite && D < 28 && !Who.empty());
			if (T.GtdCents <= 0 || T.BuyInCents <= 0)
			{
				continue;
			}
			const ss::Chips Contribution = static_cast<ss::Chips>(std::llround(static_cast<double>(T.BuyInCents) * Net.Definition().Rake.PrizeShare(T)));
			const bool Overlay = T.GtdCents > Contribution * E.Entries;
			int Skilled = 0;
			for (const int Id : Who)
			{
				const world::Npc* N = W.Get(Id);
				Skilled += N && N->Overall() >= 0.70f ? 1 : 0;
			}
			(Overlay ? Out.OverlayEvents : Out.GtdEvents) += 1;
			(Overlay ? Out.OverlayEntries : Out.GtdEntries) += static_cast<int>(Who.size());
			(Overlay ? Out.OverlaySkilled : Out.GtdSkilled) += Skilled;
		}
	}
}

/** The traps (review_ready.md): RiverLine's Series board and Night Shift only ever hold RiverLine's events. Text: read the
 *  head's running series and the rivals' races too (a whole world written: only on the days that matter). */
void Traps(const world::World& W, const std::set<int>& TwiceOnly, bool Read, Watch& Out)
{
	if (Read)
	{
		const std::vector<std::string> All = Lines(Text(W));
		const std::vector<std::string> Head = Tabs(All.front());
		const std::string Key = Head.size() > 18 ? Head[18] : std::string();
		Out.SeriesClean = Out.SeriesClean && net::SiteOf(Key) == net::SiteId::RiverLine;
		for (const std::string& L : All)
		{
			if (L.rfind("world\tsiteseries\t2\t", 0) == 0)
			{
				const std::vector<std::string> F = Tabs(L);
				Out.RivalRace = Out.RivalRace || (F.size() > 3 && net::SiteOf(F[3]) == net::SiteId::RunItTwice);
			}
		}
		if (!Key.empty())
		{
			Out.SeriesSeen = Key;
		}
	}
	for (const world::Npc& N : W.People())
	{
		if ((N.Sites & Bit(net::SiteId::RiverLine)) == 0 || TwiceOnly.count(N.Id) > 0)
		{
			// Someone who plays no RiverLine event: nothing on its Night Shift, ever.
			Out.NightClean = Out.NightClean && W.BoardValue(net::SiteId::RiverLine, net::Board::NightShift, N.Id, W.Clock()) == 0.0;
		}
	}
}

void Subtraction(const world::World& W, const std::string& When)
{
	const double At = W.Clock();
	int Wrong = 0;
	std::string First;
	int Played = 0;
	for (const world::Npc& N : W.People())
	{
		double Points = 0.0;
		double Won = 0.0;
		double Wins = 0.0;
		double Fts = 0.0;
		bool Any = false;
		for (const net::SiteId S : {net::SiteId::Ignite, net::SiteId::RunItTwice})
		{
			if (const world::SiteRecord* R = W.SiteRecordOf(S, N.Id))
			{
				Any = true;
				Points += R->Points;
				Won += static_cast<double>(R->Lifetime);
				Wins += R->Wins;
				Fts += R->FinalTables;
				const bool Same = W.BoardValue(S, net::Board::Season, N.Id, At) == R->Points && W.BoardValue(S, net::Board::Wins, N.Id, At) == R->Wins &&
					W.BoardValue(S, net::Board::Earnings, N.Id, At) == static_cast<double>(R->Lifetime);
				if (!Same && First.empty())
				{
					First = N.Name + " on " + net::KeyOf(S);
				}
				Wrong += Same ? 0 : 1;
			}
		}
		Played += Any ? 1 : 0;
		const auto Near = [](double A, double B) { return std::fabs(A - B) <= 1e-6 * std::max(1.0, std::fabs(B)); };
		const bool Sums = Near(W.BoardValue(net::SiteId::RiverLine, net::Board::Earnings, N.Id, At) + Won, W.BoardValue(net::Board::Earnings, N.Id, At)) &&
			Near(W.BoardValue(net::SiteId::RiverLine, net::Board::Season, N.Id, At) + Points, W.BoardValue(net::Board::Season, N.Id, At)) &&
			W.BoardValue(net::SiteId::RiverLine, net::Board::Wins, N.Id, At) + Wins == W.BoardValue(net::Board::Wins, N.Id, At) &&
			W.BoardValue(net::SiteId::RiverLine, net::Board::FinalTables, N.Id, At) + Fts == W.BoardValue(net::Board::FinalTables, N.Id, At);
		const bool Rest = W.BoardValue(net::SiteId::RiverLine, net::Board::Live, N.Id, At) == W.BoardValue(net::Board::Live, N.Id, At) &&
			W.BoardValue(net::SiteId::RiverLine, net::Board::Series, N.Id, At) == W.BoardValue(net::Board::Series, N.Id, At);
		if (!(Sums && Rest) && First.empty())
		{
			First = N.Name + " (RiverLine + rivals != the whole record)";
		}
		Wrong += Sums && Rest ? 0 : 1;
	}
	Check(Wrong == 0 && Played > 50, "sites, " + When + ": RiverLine's boards are the world's record less the rivals', a rival's its own (" + std::to_string(Played) + " played a rival" +
										 (First.empty() ? std::string() : "; " + First) + ")");
}

void Leaderboards(world::World& W, const std::string& When)
{
	net::AttachAll(&W);
	net::HeroStats Hero;
	Hero.Name = "Tester";
	bool Ok = true;
	std::string Why;
	for (const net::SiteId S : {net::SiteId::RiverLine, net::SiteId::Ignite, net::SiteId::RunItTwice})
	{
		const net::Network& Net = net::Site(S);
		for (const net::Board B : {net::Board::Earnings, net::Board::Season, net::Board::Wins, net::Board::FinalTables, net::Board::Series, net::Board::NightShift, net::Board::Live,
				 net::Board::Today, net::Board::Month, net::Board::Ladder, net::Board::HighRollers})
		{
			const std::vector<net::BoardRow> Rows = Net.Leaderboard(B, W.Clock(), Hero, 40, nullptr);
			double Last = 1e300;
			for (const net::BoardRow& R : Rows)
			{
				const bool Good = R.Player >= 0 && R.Value > 0.0 && R.Value <= Last && R.Value == W.BoardValue(S, B, R.Player, W.Clock());
				if (!Good && Why.empty())
				{
					Why = std::string(net::KeyOf(S)) + " board " + std::to_string(static_cast<int>(B));
				}
				Ok = Ok && Good;
				Last = R.Value;
			}
			if (S != net::SiteId::RiverLine && (B == net::Board::NightShift || B == net::Board::Live || B >= net::Board::Ladder))
			{
				Ok = Ok && Rows.empty(); // a rival has no Night Shift or live board; the Ladder and the high rollers are P1's
			}
			if (S != net::SiteId::RiverLine && B == net::Board::Season)
			{
				// Its rows' arrows: where they stood a day ago, from the site's own ranks (never RiverLine's).
				int Ranked = 0;
				for (const net::BoardRow& R : Rows)
				{
					const int Before = W.RankBefore(S, B, R.Player);
					Ranked += Before > 0 ? 1 : 0;
					Ok = Ok && R.Move == (Before > 0 ? Before - R.Rank : R.Move);
				}
				if (Ranked == 0 && Why.empty())
				{
					Why = std::string(net::KeyOf(S)) + "'s Season board has no rank a day ago";
				}
				Ok = Ok && Ranked > 0;
			}
		}
	}
	Check(Ok, "sites, " + When + ": every site's boards list its own values, best first, nothing at zero (" + Why + ")");
	net::AttachAll(nullptr); // (the networks never outlive the world they show: a rival's launch is read from it)
}

void News(world::World& W, const std::vector<std::string>& Names, const std::string& When)
{
	net::AttachAll(&W);
	net::HeroStats Hero;
	Hero.Name = "Tester";
	std::string Leak;
	for (const net::NewsItem& It : net::Shared().News(W.Clock(), Hero, 400))
	{
		for (const std::string& N : Names)
		{
			if (It.Title.find(N) != std::string::npos || It.Body.find(N) != std::string::npos)
			{
				Leak = It.Title;
			}
		}
	}
	Check(Leak.empty(), "sites, " + When + ": RiverLine's News never shows a rival's event (" + Leak + ")");
	// Every world event a rival made carries its site.
	int Untagged = 0;
	int Tagged = 0;
	for (const world::WorldEvent& E : W.Events())
	{
		const bool Rivals = E.Kind == world::EventKind::SiteLaunched || E.Kind == world::EventKind::SiteOverlay || E.Kind == world::EventKind::JoinedSite ||
			E.Kind == world::EventKind::SignedSiteTeam;
		bool Named = false;
		for (const std::string& N : Names)
		{
			Named = Named || E.What.find(N) != std::string::npos;
		}
		const bool Site = world::FlagSiteOf(E.Flags) != net::SiteId::RiverLine;
		Untagged += (Rivals || Named) && !Site ? 1 : 0;
		Tagged += Site ? 1 : 0;
	}
	Check(Untagged == 0 && Tagged > 0, "sites, " + When + ": every rival world event carries its site in its flags (" + std::to_string(Tagged) + " tagged, " +
										   std::to_string(Untagged) + " not)");
	net::AttachAll(nullptr);
}

/**
 * The field a rival's event is played against (Pending.Strength, §5.9): the world's field at the tier plus the site's softness
 * (Ignite's softer, RunItTwice's tougher), read from the queue's saved lines for plain events (no series, satellite, big field
 * or high buy-in adjustment).
 */
void Softness(const std::string& Saved)
{
	std::vector<double> Field;
	int Plain = 0;
	int Wrong = 0;
	std::map<net::SiteId, int> Seen;
	for (const std::string& L : Lines(Saved))
	{
		const std::vector<std::string> F = Tabs(L);
		if (F.size() > 6 && F[1] == "field")
		{
			for (size_t K = 2; K < 7; ++K)
			{
				Field.push_back(std::strtod(F[K].c_str(), nullptr));
			}
		}
		if (F.size() < 27 || F[1] != "q" || F[4] != "1" || Field.size() < 5)
		{
			continue;
		}
		const int Entries = std::atoi(F[9].c_str());
		const long long BuyIn = std::atoll(F[11].c_str());
		const int Tier = std::atoi(F[12].c_str());
		const int Kinds = std::atoi(F[26].c_str());
		if (!F[18].empty() || Entries > 3000 || BuyIn >= 100000 || (Kinds & 2) != 0 || Tier < 0 || Tier > 4)
		{
			continue;
		}
		const net::SiteId S = net::SiteOf(F[2]);
		const double Want = Field[static_cast<size_t>(Tier)] + (S == net::SiteId::RiverLine ? 0.0 : net::Def(S).Softness[static_cast<size_t>(Tier)]);
		Wrong += std::fabs(std::strtod(F[24].c_str(), nullptr) - Want) > 1e-9 ? 1 : 0;
		++Plain;
		++Seen[S];
	}
	Check(Plain > 0 && Wrong == 0 && Seen[net::SiteId::Ignite] > 0 && Seen[net::SiteId::RunItTwice] > 0,
		"sites: a rival's events are played against the tier's field plus its softness (" + std::to_string(Wrong) + " of " + std::to_string(Plain) + " off)");
}

/** Bytes of the world text that are the rival sites' (their lines, the npc field, their events in the queue, results and log). */
void TextBudget(const world::World& W)
{
	size_t Rival = 0;
	size_t All = 0;
	for (const std::string& L : Lines(Text(W)))
	{
		All += L.size() + 1;
		const std::vector<std::string> F = Tabs(L);
		if (F.size() < 3)
		{
			continue;
		}
		const std::string& Kind = F[1];
		if (Kind == "site" || Kind == "siteseries" || Kind == "sitelaunch")
		{
			Rival += L.size() + 1;
		}
		else if (Kind == "npc" && F.back() != "1")
		{
			Rival += F.back().size() + 1;
		}
		else if ((Kind == "q" || Kind == "res") && net::SiteOf(F[2]) != net::SiteId::RiverLine)
		{
			Rival += L.size() + 1;
		}
		else if (Kind == "log" && F.size() > 9 && world::FlagSiteOf(std::atoi(F[9].c_str())) != net::SiteId::RiverLine)
		{
			Rival += L.size() + 1;
		}
	}
	const double Share = All > Rival ? static_cast<double>(Rival) / static_cast<double>(All - Rival) : 1.0;
	std::printf("  sites: the world text after 30 days is %zu bytes, %zu of them the rivals' (+%.1f%% on the rest)\n", All, Rival, Share * 100.0);
	Check(Share <= 0.10, "sites: the rival sites add at most 10% to the world text after 30 days");
}

void Season(uint32_t Seed)
{
	{
		// The budgets (§9: create at most 0.15 s, the first 30 days at most 0.75 s): wall clock on a shared machine, three
		// suites at once, so printed and held to three times them.
		const auto T0 = std::chrono::steady_clock::now();
		world::World Timed;
		Timed.Create(Seed + 1, NightOne);
		const double Create = Seconds(T0);
		const auto T1 = std::chrono::steady_clock::now();
		Timed.Simulate(30);
		const double Month = Seconds(T1);
		std::printf("  sites: created in %.3f s, the first 30 days in %.2f s (budgets 0.15 s, 0.75 s)\n", Create, Month);
		Check(Create <= 0.45 && Month <= 2.25, "sites: a world with two rivals creates and plays its first month inside three times §9's budgets");
	}
	world::World W;
	W.Create(Seed, NightOne);
	const std::vector<std::string> Names = RivalNames();
	// Who plays only on RunItTwice from the first night (its team), and their RiverLine numbers then.
	std::set<int> TwiceOnly;
	std::map<int, double> Before;
	std::set<int> Pros;
	for (const world::Npc& N : W.People())
	{
		if (OnTeam(N, net::SiteId::RunItTwice))
		{
			TwiceOnly.insert(N.Id);
			Before[N.Id] = W.BoardValue(net::SiteId::RiverLine, net::Board::Series, N.Id, W.Clock());
		}
		if (N.Pro)
		{
			Pros.insert(N.Id);
		}
	}
	Watch Seen;
	std::string Saved30;
	const int NewYear = world::DayOn(2027, 1, 1);
	// A rival grinder who wins everything they play on RunItTwice and Ignite in the year's last fortnight: the most points
	// anywhere, and none on RiverLine. Player of the Year is RiverLine's: never theirs.
	int Ringer = -1;
	for (const world::Npc& N : W.People())
	{
		if (Ringer < 0 && N.Playing() && !N.Pro && N.Sponsor.empty() && N.Tier == 2 && (N.Sites & Bit(net::SiteId::RunItTwice)) != 0)
		{
			Ringer = N.Id;
		}
	}
	for (int D = Today(W); D + 1 < NewYear - 1; ++D)
	{
		// The night's first hours (the Night Shift's) and the day's end, each day. The head's running series is read weekly,
		// and every day around the RunItTwice Open (Nov 13-29, days 39-55) and the RCOP before it.
		W.AdvanceTo(D * Day + 1440.0 + 90.0);
		Traps(W, TwiceOnly, (D >= 30 && D <= 58) || D % 7 == 0, Seen);
		W.AdvanceTo((D + 1) * Day + 1380.0);
		Registrations(W, D + 2, Seen);
		TrafficShare(W, D + 2, Seen);
		if (Ringer >= 0 && D + 2 >= NewYear - 14 && D + 2 < NewYear)
		{
			for (const net::SiteId S : {net::SiteId::Ignite, net::SiteId::RunItTwice})
			{
				for (const net::EventInstance& E : net::Site(S).Window((D + 2) * Day, (D + 3) * Day))
				{
					if (!W.RegisteredNow(E.Id).empty() && E.Start + E.Duration < NewYear * Day)
					{
						W.Join(E.Id, Ringer);
						W.KnownPlaces(E.Id, {{Ringer, 1}});
					}
				}
			}
		}
		if (D + 1 == 27)
		{
			// Nov 1, the day before Ignite opens: its regulars' first accounts, and not one result there.
			bool None = true;
			for (const world::Npc& N : W.People())
			{
				None = None && W.SiteRecordOf(net::SiteId::Ignite, N.Id) == nullptr;
			}
			Check(None && !Seen.IgniteEarly, "sites: nobody has played Ignite before it opens (no history before its launch)");
		}
		if (D + 1 == 31)
		{
			// Nov 5: a few days into Ignite. Its launch was that morning's news (on its own page).
			int Launch = 0;
			for (const world::WorldEvent& E : W.Events())
			{
				Launch += E.Kind == world::EventKind::SiteLaunched && world::FlagSiteOf(E.Flags) == net::SiteId::Ignite && static_cast<int>(E.At / Day) == 28 ? 1 : 0;
			}
			int Ignite = 0;
			int Joined = 0;
			int Micro = 0;
			for (const world::Npc& N : W.People())
			{
				Ignite += W.SiteRecordOf(net::SiteId::Ignite, N.Id) != nullptr ? 1 : 0;
				const bool Regular = N.Came == world::Arrival::None && N.Playing() && N.Tier == 1 && N.OnlineShare >= 0.4f && !N.Pro;
				Micro += Regular ? 1 : 0;
				Joined += Regular && (N.Sites & Bit(net::SiteId::Ignite)) != 0 ? 1 : 0;
			}
			Check(Launch == 1 && Ignite >= 5, "sites: Ignite opened Nov 2 with its own news, and its regulars play it (" + std::to_string(Ignite) + " with a record by Nov 5)");
			Check(Micro > 0 && Joined > 0 && Joined < Micro / 2, "sites: Ignite's Micro regulars are joining over its first month (" + std::to_string(Joined) + " of " + std::to_string(Micro) + ")");
		}
		if (D + 1 == 57)
		{
			// Dec 1 (a Tuesday: no stakes move between the 1st's accounts and tonight): RunItTwice keeps its share of the regulars
			// at its stakes, so everyone who has risen to them since and drew in its share has an account (§5.9, asked each 1st).
			int Missing = 0;
			int Rose = 0;
			for (const world::Npc& N : W.People())
			{
				// (Someone the world discovered today, after the 1st's accounts, is asked at the next 1st.)
				if (N.Faded || !N.Playing() || N.Pro || !N.Sponsor.empty() || (N.Sites & Bit(net::SiteId::RiverLine)) == 0 || N.From == world::Origin::Discovered ||
					(Rule(W, N, D + 1) & Bit(net::SiteId::RunItTwice)) == 0)
				{
					continue;
				}
				Missing += (N.Sites & Bit(net::SiteId::RunItTwice)) == 0 ? 1 : 0;
				Rose += N.TierSince > 1 ? 1 : 0; // (their stakes moved since the first night)
			}
			Check(Missing == 0 && Rose > 0, "sites: on the 1st, the regulars who rose to RunItTwice's stakes open their accounts there (" + std::to_string(Missing) + " missing, " +
												std::to_string(Rose) + " whose stakes moved)");
		}
		if (D + 1 == 56)
		{
			// Nov 30, the day after the RunItTwice Open: its main event's final table and titles (the majors' news) are in the log.
			Subtraction(W, "day 56");
			News(W, Names, "day 56");
		}
		if (D + 1 == 35)
		{
			Saved30 = Text(W);
			Subtraction(W, "day 35");
			Leaderboards(W, "day 35");
			News(W, Names, "day 35");
			TextBudget(W);
			Failures += CheckInvariants(W, "the world on three sites, day 35");
			// Saved and loaded, a world on three sites reads back as it was and plays on exactly as one never saved.
			world::World Copy;
			Load(Copy, Saved30);
			Check(Text(Copy) == Saved30, "sites: a world on three sites writes, reads and writes again the same text");
			// A loaded world works out its boards' ranks a day ago from what it read (Finish): on a rival's boards, 1..n for the
			// people with something on them, most first, ties by id, and 0 for everyone else.
			bool Ranked = true;
			int Rows = 0;
			for (const net::SiteId S : {net::SiteId::Ignite, net::SiteId::RunItTwice})
			{
				for (const net::Board B : {net::Board::Earnings, net::Board::Season, net::Board::Wins, net::Board::FinalTables, net::Board::Series, net::Board::Month})
				{
					std::vector<std::pair<double, int>> On;
					for (const world::Npc& N : Copy.People())
					{
						const double V = Copy.BoardValue(S, B, N.Id, Copy.Clock());
						if (V > 0.0)
						{
							On.push_back({-V, N.Id});
						}
						else
						{
							Ranked = Ranked && Copy.RankBefore(S, B, N.Id) == 0;
						}
					}
					std::sort(On.begin(), On.end());
					for (size_t K = 0; K < On.size(); ++K)
					{
						Ranked = Ranked && Copy.RankBefore(S, B, On[K].second) == static_cast<int>(K) + 1;
					}
					Rows += static_cast<int>(On.size());
				}
			}
			Check(Ranked && Rows > 0, "sites: a rival's boards rank their people a day ago 1..n, most first, and nobody else");
			world::World Ahead;
			Load(Ahead, Saved30);
			Ahead.Simulate(3);
			world::World Unsaved;
			Load(Unsaved, Saved30);
			Unsaved.Simulate(3);
			Check(Text(Ahead) == Text(Unsaved), "sites: a loaded world on three sites plays on the same");
			// Ignite's first month: about 30% of its Micro regulars and 10% of Low ones.
			int Micro = 0;
			int MicroIn = 0;
			int Low = 0;
			int LowIn = 0;
			for (const world::Npc& N : W.People())
			{
				if (N.Came != world::Arrival::None || !N.Playing() || N.OnlineShare < 0.4f || N.Pro || OnTeam(N, net::SiteId::RunItTwice))
				{
					continue;
				}
				const bool In = (N.Sites & Bit(net::SiteId::Ignite)) != 0;
				(N.Tier == 1 ? Micro : Low) += N.Tier == 1 || N.Tier == 2 ? 1 : 0;
				(N.Tier == 1 ? MicroIn : LowIn) += (N.Tier == 1 || N.Tier == 2) && In ? 1 : 0;
			}
			std::printf("  sites: Ignite a week in: %d of %d Micro regulars, %d of %d Low\n", MicroIn, Micro, LowIn, Low);
		}
		if (D + 1 == 38)
		{
			// Thursday Nov 12: everything in the queue was planned this week (the fields move on Mondays).
			Softness(Text(W));
		}
		if (D + 1 == 58)
		{
			// Dec 3: Ignite's first month is over (30 days from Nov 2): its share of the regulars.
			int Micro = 0;
			int MicroIn = 0;
			int Low = 0;
			int LowIn = 0;
			int Arrived = 0;
			int ArrivedIn = 0;
			int Only = 0;
			for (const world::Npc& N : W.People())
			{
				if (N.Pro || OnTeam(N, net::SiteId::RunItTwice) || N.Faded)
				{
					continue;
				}
				const bool In = (N.Sites & Bit(net::SiteId::Ignite)) != 0;
				if (N.Came != world::Arrival::None && N.Arrived >= 28 && N.OnlineShare >= 0.4f)
				{
					++Arrived;
					ArrivedIn += In ? 1 : 0;
					Only += N.Sites == Bit(net::SiteId::Ignite) ? 1 : 0;
				}
				else if (N.Came == world::Arrival::None && N.OnlineShare >= 0.4f && (N.Tier == 1 || N.Tier == 2))
				{
					(N.Tier == 1 ? Micro : Low) += 1;
					(N.Tier == 1 ? MicroIn : LowIn) += In ? 1 : 0;
				}
			}
			std::printf("  sites: Ignite after its first month: %d of %d Micro regulars, %d of %d Low; %d of %d arrivals since its launch (%d on Ignite alone)\n", MicroIn, Micro, LowIn,
				Low, ArrivedIn, Arrived, Only);
			const double MicroShare = Micro > 0 ? static_cast<double>(MicroIn) / Micro : 0.0;
			const double LowShare = Low > 0 ? static_cast<double>(LowIn) / Low : 0.0;
			Check(MicroShare >= 0.2 && MicroShare <= 0.4 && LowShare >= 0.04 && LowShare <= 0.18, "sites: Ignite's first month brings about 30% of the Micro regulars and 10% of Low ones");
			Check(Arrived >= 10 && ArrivedIn > 0 && ArrivedIn < Arrived, "sites: some of the arrivals since Ignite's launch come in through it");
		}
	}
	// How they arrived reads from their journey, never from where they play now: an Ignite arrival's card says Ignite from its
	// first step on, and nobody else's does (a team pro plays one site alone, and that's no arrival story).
	const std::string IgniteName = net::Def(net::SiteId::Ignite).Name;
	int IgniteCards = 0;
	int WrongCards = 0;
	std::string WrongCard;
	for (const world::Npc& N : W.People())
	{
		Seen.IgniteOnly += N.Playing() && N.Sites == Bit(net::SiteId::Ignite) ? 1 : 0;
		if (N.Came == world::Arrival::None)
		{
			continue;
		}
		const world::Profile Pr = W.ProfileOf(N.Id);
		const bool Theirs = !N.Path.empty() && N.Path.front().Kind == world::StepKind::Joined && N.Path.front().What == IgniteName;
		const bool Says = Pr.Came.find("Downloaded " + IgniteName + " ") == 0;
		const bool Step = !Pr.Journey.empty() && Pr.Journey.front().Text == "Joined " + IgniteName + ".";
		IgniteCards += Theirs ? 1 : 0;
		if ((Says && !Theirs) || Theirs != Step)
		{
			++WrongCards;
			WrongCard = N.Name + ": " + Pr.Came;
		}
	}
	Check(WrongCards == 0 && (Seed != 7u || IgniteCards > 0),
		"sites: only Ignite's own arrivals' cards say they came through Ignite, from their first step on (" + std::to_string(IgniteCards) + " of them; " + WrongCard + ")");
	{
		// One of RiverLine's own arrivals made to play on RunItTwice alone (as its team's pros do), through the save: their card
		// still says they downloaded RiverLine.
		int Pick = -1;
		for (const world::Npc& N : W.People())
		{
			if (Pick < 0 && N.Came == world::Arrival::FirstTimer && W.ProfileOf(N.Id).Came.find("Downloaded RiverLine ") == 0)
			{
				Pick = N.Id;
			}
		}
		std::string Moved;
		const std::string Head = "world\tnpc\t" + std::to_string(Pick) + "\t";
		for (const std::string& L : Lines(Text(W)))
		{
			std::string Out = L;
			if (Pick >= 0 && L.rfind(Head, 0) == 0)
			{
				// (The sites are the npc line's last field from version 4, and before it whenever they aren't RiverLine's alone.)
				const bool Has = world::Version >= 4 || W.Get(Pick)->Sites != Bit(net::SiteId::RiverLine);
				Out = (Has ? L.substr(0, L.rfind('\t')) : L) + "\t" + std::to_string(Bit(net::SiteId::RunItTwice));
			}
			Moved += Out + "\n";
		}
		world::World Again;
		Load(Again, Moved);
		const world::Npc* N = Pick >= 0 ? Again.Get(Pick) : nullptr;
		Check(N && N->Sites == Bit(net::SiteId::RunItTwice) && Again.ProfileOf(Pick).Came.find("Downloaded RiverLine ") == 0,
			"sites: where someone plays now never rewrites how they arrived (a RiverLine arrival on RunItTwice alone)");
	}
	Check(Seen.SeriesClean && Seen.RivalRace, "sites: a RunItTwice series never becomes RiverLine's running series (RiverLine's last: " + Seen.SeriesSeen + "), and runs its own");
	Check(Seen.NightClean, "sites: nobody who plays no RiverLine event ever has Night Shift points (rival micro events stay off it)");
	bool TeamSeries = true;
	bool TeamStayed = true;
	for (const int Id : TwiceOnly)
	{
		const world::Npc* N = W.Get(Id);
		TeamStayed = TeamStayed && N && OnTeam(*N, net::SiteId::RunItTwice);
		TeamSeries = TeamSeries && W.BoardValue(net::SiteId::RiverLine, net::Board::Series, Id, W.Clock()) <= Before[Id];
	}
	Check(TeamSeries && !TwiceOnly.empty(), "sites: Team RunItTwice (only on RunItTwice) never adds a point to RiverLine's series board");
	bool ProsHome = true;
	for (const int Id : Pros)
	{
		const world::Npc* N = W.Get(Id);
		ProsHome = ProsHome && (!N || !N->Pro || (W.SiteRecordOf(net::SiteId::Ignite, Id) == nullptr && W.SiteRecordOf(net::SiteId::RunItTwice, Id) == nullptr));
	}
	Check(ProsHome, "sites: Team RiverLine plays nothing on a rival");
	// NPCs play rival events and hunt overlays (§5.8, §5.9).
	const double OverSkill = Seen.OverlayEntries > 0 ? static_cast<double>(Seen.OverlaySkilled) / Seen.OverlayEntries : 0.0;
	const double GtdSkill = Seen.GtdEntries > 0 ? static_cast<double>(Seen.GtdSkilled) / Seen.GtdEntries : 0.0;
	std::printf("  sites: rival registrations %d (Ignite %d, RunItTwice %d); GTD events %d with %d entries (skilled %.1f%%), overlays %d with %d (skilled %.1f%%); Ignite alone: %d\n",
		Seen.RivalEntries, Seen.BySite[net::SiteId::Ignite], Seen.BySite[net::SiteId::RunItTwice], Seen.GtdEvents, Seen.GtdEntries, GtdSkill * 100.0, Seen.OverlayEvents,
		Seen.OverlayEntries, OverSkill * 100.0, Seen.IgniteOnly);
	Check(Seen.BySite[net::SiteId::Ignite] > 0 && Seen.BySite[net::SiteId::RunItTwice] > 0, "sites: the world's people play both rivals' events");
	const double Away = Seen.TwiceHome + Seen.TwiceAway > 0 ? static_cast<double>(Seen.TwiceAway) / static_cast<double>(Seen.TwiceHome + Seen.TwiceAway) : 0.0;
	std::printf("  sites: RunItTwice's members, Oct 8 to Nov 1: %d entries on RiverLine, %d on RunItTwice (%.1f%%)\n", Seen.TwiceHome, Seen.TwiceAway, Away * 100.0);
	// §5.9: a rival's options weigh as much as its traffic (RunItTwice 0.55), so its members still play mostly on RiverLine:
	// about 23% of their entries there (32% with no traffic weight, 15% with it twice; seed 7, about 21,000 entries).
	Check(Away >= 0.17 && Away <= 0.28, "sites: RunItTwice's members put about its traffic's share of their entries there (" + ss::Fixed(Away * 100.0, 1) + "%)");
	Check(Seen.OverlayEvents > 0 && Seen.OverlayEntries > 0, "sites: the people the world follows play the rival events that overlay");
	// The skilled hunt them (×1.2 on an overlay, §5.9): a bigger share of the skilled in the overlays than in the guarantees
	// that fill. (The weight is small against everything else that decides who plays what: held on the default seed, printed
	// on the long run's.)
	Check(Seed != 7u || OverSkill > GtdSkill, "sites: the skilled make up more of an overlay's field than of a guarantee that fills");
	// Player of the Year 2026: RiverLine's points only.
	const double Eve = NewYear * Day - 1e-6;
	W.AdvanceTo(Eve);
	std::map<int, double> Home;
	std::map<int, ss::Chips> Rival;
	int Leader = -1;
	double LeaderAll = 0.0;
	for (const world::Npc& N : W.People())
	{
		Home[N.Id] = W.BoardValue(net::SiteId::RiverLine, net::Board::Season, N.Id, Eve);
		if (W.BoardValue(net::Board::Season, N.Id, Eve) > LeaderAll)
		{
			LeaderAll = W.BoardValue(net::Board::Season, N.Id, Eve);
			Leader = N.Id;
		}
		if (const world::SiteRecord* R = W.SiteRecordOf(net::SiteId::RunItTwice, N.Id))
		{
			Rival[N.Id] = R->Lifetime;
		}
	}
	W.AdvanceTo(NewYear * Day + 0.5);
	bool Restarted = !Rival.empty();
	for (const auto& It : Rival)
	{
		const world::SiteRecord* R = W.SiteRecordOf(net::SiteId::RunItTwice, It.first);
		Restarted = Restarted && (It.second == 0 ? R == nullptr || R->Events <= 2 : R && R->Lifetime >= It.second && R->Events <= 2 && R->Points <= 500.0);
	}
	Check(Restarted, "sites: a new year restarts the rivals' seasons and keeps their lifetime money");
	int Crowned = -3;
	double Points = 0.0;
	for (const world::Honor& H : W.Honors())
	{
		if (H.Year == 2026 && H.Title == "Player of the Year")
		{
			Crowned = H.Npc;
			Points = static_cast<double>(H.Entries);
		}
	}
	double Best = 0.0;
	for (const auto& It : Home)
	{
		Best = std::max(Best, It.second);
	}
	std::printf("  sites: Player of the Year 2026: %s on %.0f RiverLine points (the most points anywhere: %s, %.0f)\n", Crowned >= 0 ? W.People()[static_cast<size_t>(Crowned)].Name.c_str() : "?",
		Points, Leader >= 0 ? W.People()[static_cast<size_t>(Leader)].Name.c_str() : "?", LeaderAll);
	Check(Crowned >= 0 && Home[Crowned] >= Best - 1e-9 && std::fabs(Points - std::floor(Home[Crowned])) <= 1.0 && Leader == Ringer && Crowned != Ringer,
		"sites: Player of the Year 2026 is RiverLine's points leader, on RiverLine's points, never the rival grinder with the most points anywhere");
}

// ------------------------------------------------------------------ the teams in a fresh world

void FreshTeams(uint32_t Seed)
{
	world::World W;
	W.Create(Seed, NightOne, world::Start::Blank);
	std::set<int> Signed;
	bool Earned = true;
	std::string Months;
	int Before = -1;
	for (const int First : {world::DayOn(2026, 11, 1), world::DayOn(2026, 12, 1)})
	{
		W.AdvanceTo(First * Day - 1e-6); // the last instant before the 1st
		if (Before < 0)
		{
			Before = 0;
			for (const world::Npc& N : W.People())
			{
				Before += OnTeam(N, net::SiteId::RunItTwice) ? 1 : 0;
			}
		}
		// The site's Season board as the 1st begins (what the signing reads).
		std::vector<std::pair<double, int>> Board;
		for (const world::Npc& N : W.People())
		{
			const double V = W.BoardValue(net::SiteId::RunItTwice, net::Board::Season, N.Id, First * Day);
			if (V > 0.0)
			{
				Board.push_back({-V, N.Id});
			}
		}
		std::sort(Board.begin(), Board.end());
		std::map<int, int> Rank;
		for (size_t K = 0; K < Board.size(); ++K)
		{
			Rank[Board[K].second] = static_cast<int>(K) + 1;
		}
		int Members = 0;
		int Known = 0;
		int Results = 0;
		for (const world::Npc& N : W.People())
		{
			if (N.Pro || !N.Sponsor.empty() || !N.Playing() || !N.Professional || (N.Sites & Bit(net::SiteId::RunItTwice)) == 0)
			{
				continue;
			}
			++Members;
			// (An established world's bar, a reputation of 35, which nobody here has reached yet: why a fresh one reads results.)
			Known += N.RepOf(world::Rep::Overall) >= 35.0f ? 1 : 0;
			const world::SiteRecord* R = W.SiteRecordOf(net::SiteId::RunItTwice, N.Id);
			Results += (R && R->FinalTables > 0) || (Rank.count(N.Id) > 0 && Rank[N.Id] <= 100) ? 1 : 0;
		}
		std::printf("  sites: before %s, a fresh world's RunItTwice professionals with no deal: %d (%d with a reputation of 35), %d with results there\n", net::DateLabel(First).c_str(),
			Members, Known, Results);
		W.AdvanceTo(First * Day + 600.0);
		int New = 0;
		for (const world::Npc& N : W.People())
		{
			if (!OnTeam(N, net::SiteId::RunItTwice) || Signed.count(N.Id) > 0)
			{
				continue;
			}
			Signed.insert(N.Id);
			++New;
			const world::SiteRecord* R = W.SiteRecordOf(net::SiteId::RunItTwice, N.Id);
			Earned = Earned && ((R && R->FinalTables > 0) || (Rank.count(N.Id) > 0 && Rank[N.Id] <= 100)) && N.Sites == Bit(net::SiteId::RunItTwice) && N.Professional;
		}
		Months += " " + net::DateLabel(First) + ": " + std::to_string(New);
	}
	int News = 0;
	for (const world::WorldEvent& E : W.Events())
	{
		if (E.Kind == world::EventKind::SignedSiteTeam)
		{
			std::string Title;
			std::string Body;
			std::string Tag;
			News += world::FlagSiteOf(E.Flags) == net::SiteId::RunItTwice && W.Headline(E, Title, Body, Tag) && Title.find("joins Team RunItTwice") != std::string::npos &&
					Body.find("Signed on results.") != std::string::npos
				? 1
				: 0;
		}
	}
	std::printf("  sites: a fresh world's Team RunItTwice: %d before Nov 1; signed%s\n", Before, Months.c_str());
	Check(Before == 0 && !Signed.empty() && Earned && News == static_cast<int>(Signed.size()),
		"sites: a fresh world's Team RunItTwice starts empty and signs only on results there, from Nov 1, each one news on RunItTwice's page");
	// Team RiverLine signs its first class on the same 1sts: from the day they sign, they play only on RiverLine (§1.9 #11).
	int Home = 0;
	int Away = 0;
	for (const world::Npc& N : W.People())
	{
		Home += N.Pro ? 1 : 0;
		Away += N.Pro && N.Sites != Bit(net::SiteId::RiverLine) ? 1 : 0;
	}
	Check(Home > 0 && Away == 0, "sites: a fresh world's Team RiverLine signings play only on RiverLine (" + std::to_string(Away) + " of " + std::to_string(Home) + " don't)");
	// A fresh world's record books (§3.3 F9) are RiverLine's and the live rooms': a rival's first winners are no "first in the
	// books" (its own launch is its story), so no first-winner item names a rival's event or series.
	std::set<std::string> Rival;
	for (const std::string& Name : RivalNames())
	{
		Rival.insert(Name);
	}
	for (const net::SiteId S : {net::SiteId::Ignite, net::SiteId::RunItTwice})
	{
		for (const net::SeriesInfo& Sr : net::Site(S).Series())
		{
			Rival.insert(Sr.Name);
		}
	}
	int Firsts = 0;
	std::string Named;
	for (const world::WorldEvent& E : W.Events())
	{
		Firsts += E.Kind == world::EventKind::FirstWinner ? 1 : 0;
		Named = E.Kind == world::EventKind::FirstWinner && Rival.count(E.What) > 0 ? E.What : Named;
	}
	Check(Firsts > 0 && Named.empty(), "sites: a fresh world's first winners are RiverLine's and the rooms', never a rival's (" + Named + ")");
}

/**
 * The player's rival (gh0stfold, RiverLine's night-shift crusher whom the session seats at the player's every RiverLine table)
 * may hold a RunItTwice account, but its team never signs him: "plays only on RunItTwice" would take him off RiverLine's boards
 * while he still sat at the player's tables. Fresh worlds where he plays RunItTwice and its team signs on results there.
 */
void RivalStaysHome()
{
	int Members = 0;
	int Signed = 0;
	int Clause = 0;
	int Broken = 0;
	for (const uint32_t Seed : {12u, 14u})
	{
		world::World W;
		W.Create(Seed, NightOne, world::Start::Blank);
		W.AdvanceTo(world::DayOn(2026, 12, 1) * Day + 600.0);
		const world::Npc* R = W.Get(W.Find("gh0stfold"));
		Members += R && R->Rival && (R->Sites & Bit(net::SiteId::RunItTwice)) != 0 ? 1 : 0;
		Signed += R && (OnTeam(*R, net::SiteId::RunItTwice) || (R->Sites & Bit(net::SiteId::RiverLine)) == 0) ? 1 : 0;
		// (And the first classes both teams signed here play only on their own site.)
		for (const world::Npc& N : W.People())
		{
			Clause += (N.Pro ? 1 : 0) + (OnTeam(N, net::SiteId::RunItTwice) ? 1 : 0);
			Broken += (N.Pro && N.Sites != Bit(net::SiteId::RiverLine)) || (OnTeam(N, net::SiteId::RunItTwice) && N.Sites != Bit(net::SiteId::RunItTwice)) ? 1 : 0;
		}
	}
	Check(Members == 2 && Signed == 0, "sites: the player's rival can play RunItTwice but its team never signs him off RiverLine (" + std::to_string(Signed) + " of " +
										   std::to_string(Members) + " signed)");
	Check(Clause > 0 && Broken == 0, "sites: every site team's signing plays only on its site (" + std::to_string(Broken) + " of " + std::to_string(Clause) + " don't)");
}

// ------------------------------------------------------------------ the player's own results on a rival

void Hero()
{
	world::World W;
	W.Create(31u, NightOne);
	W.AdvanceTo(30.0 * Day + 600.0);
	W.HeroName = "Tester";
	net::EventInstance Rival;
	net::EventInstance Home;
	bool HaveRival = false;
	bool HaveHome = false;
	for (const net::EventInstance& E : net::Site(net::SiteId::Ignite).Window(W.Clock() + 60.0, W.Clock() + Day))
	{
		if (!HaveRival && net::Site(net::SiteId::Ignite).TemplateOf(E).BuyInCents > 0)
		{
			Rival = E;
			HaveRival = true;
		}
	}
	for (const net::EventInstance& E : net::Shared().Window(W.Clock() + 60.0, W.Clock() + Day))
	{
		if (!HaveHome && net::Shared().TemplateOf(E).BuyInCents > 0)
		{
			Home = E;
			HaveHome = true;
		}
	}
	if (!HaveRival || !HaveHome)
	{
		Check(false, "sites: an Ignite and a RiverLine event to play");
		return;
	}
	const double Points = W.HeroSeasonPoints();
	const int Events = W.HeroStats().Events;
	W.HeroEntered(Rival.Id, "");
	W.HeroFinished(Rival.Id, 1, 50000, world::World::TableReport());
	Check(W.HeroSeasonPoints() == Points && W.HeroStats().Events == Events + 1, "sites: the player's Ignite win is on their stats page, never in RiverLine's Player of the Year points");
	W.HeroEntered(Home.Id, "");
	W.HeroFinished(Home.Id, 1, 50000, world::World::TableReport());
	Check(W.HeroSeasonPoints() > Points && W.HeroStats().Events == Events + 2, "sites: a RiverLine win still counts toward Player of the Year");
}

// ------------------------------------------------------------------ ss.Site.Launch (§6.6, §8.3 step 2)

void LaunchCommand()
{
	ss::SessionHooks Hooks;
	ss::hero::Character Who;
	ss::Session S(Hooks, "sites-launch-now");
	S.NewCareer(Who, world::Start::Blank);
	const double Now = S.Living().Clock();
	const bool Ahead = S.Living().SiteLaunched(net::SiteId::Ignite) > Now;
	// SessionSites.cpp forwards ss.Site.Launch to the world's debug command ("sitelaunch <key>").
	const std::string Line = S.DebugSite("launch ignite");
	const world::World& W = S.Living();
	const double Opened = W.SiteLaunched(net::SiteId::Ignite);
	int News = 0;
	int TwiceNews = 0;
	bool Early = false;
	for (const world::WorldEvent& E : W.Events())
	{
		const net::SiteId On = world::FlagSiteOf(E.Flags);
		News += E.Kind == world::EventKind::SiteLaunched && On == net::SiteId::Ignite && E.At == Opened ? 1 : 0;
		TwiceNews += E.Kind == world::EventKind::SiteLaunched && On == net::SiteId::RunItTwice ? 1 : 0;
		Early = Early || (On == net::SiteId::Ignite && E.At < Opened);
	}
	bool Lobby = false;
	bool Before = false;
	for (const net::EventInstance& E : net::Site(net::SiteId::Ignite).Window(Opened - Day, Opened + Day))
	{
		Lobby = Lobby || E.Start >= Opened;
		Before = Before || E.Start < Opened;
	}
	Check(Ahead && Opened == Now && News == 1 && TwiceNews == 1 && !Early && Line.find(net::Def(net::SiteId::Ignite).Name + " opens now.") == 0,
		"sites: ss.Site.Launch opens Ignite in this world this minute, with its own news and nothing before (" + Line.substr(0, Line.find('.') + 1) + ")");
	Check(Lobby && !Before, "sites: a launch forced today lists Ignite's events from that minute on, none before");
	const std::string Again = S.DebugSite("launch ignite");
	const std::string Open = S.DebugSite("launch runittwice");
	Check(Again.find("already open") != std::string::npos && Open.find("already open") != std::string::npos && W.SiteLaunched(net::SiteId::Ignite) == Opened &&
			  W.SiteLaunched(net::SiteId::RunItTwice) == Day,
		"sites: ss.Site.Launch leaves a site that's open as it is (" + Again + " " + Open + ")");
	// The days already planned keep their strangers; from the first day the world plans after it, its people play Ignite.
	S.Living().AdvanceTo(std::floor(Now / Day) * Day + 4.0 * Day + 600.0);
	int Played = 0;
	for (const world::Npc& N : W.People())
	{
		Played += W.SiteRecordOf(net::SiteId::Ignite, N.Id) != nullptr ? 1 : 0;
	}
	Check(Played > 0, "sites: after a forced launch the world's people play Ignite (" + std::to_string(Played) + " with a record by day 5)");
	// An established world knows its regulars: the ones who open an account in Ignite's first days make its news, never before
	// the minute it opened.
	ss::Session E(Hooks, "sites-launch-established");
	E.NewCareer(Who);
	E.DebugSite("launch ignite");
	const double OpenedE = E.Living().SiteLaunched(net::SiteId::Ignite);
	int Joins = 0;
	bool EarlyE = false;
	for (const world::WorldEvent& Ev : E.Living().Events())
	{
		const bool Ign = world::FlagSiteOf(Ev.Flags) == net::SiteId::Ignite;
		Joins += Ign && Ev.Kind == world::EventKind::JoinedSite ? 1 : 0;
		EarlyE = EarlyE || (Ign && Ev.At < OpenedE);
	}
	std::printf("  sites: ss.Site.Launch in an established world: %d regulars the news knows joined Ignite at once\n", Joins);
	Check(OpenedE == E.Living().Clock() && Joins > 0 && !EarlyE, "sites: a forced launch's first joiners make its news from the minute it opened, never before");
}

// ------------------------------------------------------------------ an older world brought up to version 4 (§6.5)

std::string Fixture(const std::string& Name)
{
	// tests/ -> Standalone/ -> the plugin's Tests/fixtures/ (world_test has no SS_TESTS_DIR of its own; world_main.cpp's way).
	std::string Here = __FILE__;
	for (char& C : Here)
	{
		C = C == '\\' ? '/' : C;
	}
	const size_t Cut = Here.rfind("/Standalone/");
	const std::string Path = (Cut == std::string::npos ? std::string("..") : Here.substr(0, Cut)) + "/Tests/fixtures/" + Name;
	std::string Out;
	if (FILE* F = std::fopen(Path.c_str(), "rb"))
	{
		char Buf[65536];
		size_t N = 0;
		while ((N = std::fread(Buf, 1, sizeof(Buf), F)) > 0)
		{
			Out.append(Buf, N);
		}
		std::fclose(F);
	}
	Out.erase(std::remove(Out.begin(), Out.end(), '\r'), Out.end());
	return Out;
}

/** A world text with its head's version set (the rest as it is). */
std::string WithVersion(const std::string& Text, int Version)
{
	std::string Out;
	for (const std::string& L : Lines(Text))
	{
		std::string Line = L;
		if (Line.rfind("world\thead\t", 0) == 0)
		{
			const size_t From = std::string("world\thead\t").size();
			Line = "world\thead\t" + std::to_string(Version) + Line.substr(Line.find('\t', From));
		}
		Out += Line + "\n";
	}
	return Out;
}

void Migration()
{
	const std::string Old = Fixture("world_v2.txt");
	if (Old.empty())
	{
		Check(false, "sites: world_v2.txt to migrate");
		return;
	}
	world::World W;
	Load(W, Old);
	const std::string Out = Text(W);
	bool Lines4 = false;
	for (const std::string& L : Lines(Out))
	{
		Lines4 = Lines4 || L.rfind("world\tsite", 0) == 0;
	}
	int Version = world::Version; // (not a constant expression: both branches compile at either version)
	if (Version < 4)
	{
		// Before the version moves to 4 (the lead's bump, with this migration, in one commit): an older world reads and writes
		// back as it was, RiverLine only (MigrationAll holds it byte for byte).
		bool Home = true;
		for (const world::Npc& N : W.People())
		{
			Home = Home && N.Sites == Bit(net::SiteId::RiverLine);
		}
		Check(!Lines4 && Home && W.SiteLaunched(net::SiteId::RunItTwice) < 0.0, "sites: before version 4, an older world stays RiverLine's alone and writes no site line");
		std::printf("  sites: world version %d: the version 4 migration checks run once the head moves to 4\n", world::Version);
		return;
	}
	// Day 0 after the migration: RiverLine's boards exactly as they were, every npc line as it was with the sites appended.
	const double At = W.Clock();
	bool Same = true;
	for (const world::Npc& N : W.People())
	{
		for (const net::Board B : {net::Board::Earnings, net::Board::Season, net::Board::Wins, net::Board::FinalTables, net::Board::Series, net::Board::NightShift, net::Board::Live})
		{
			Same = Same && W.BoardValue(net::SiteId::RiverLine, B, N.Id, At) == W.BoardValue(B, N.Id, At) &&
				W.RankBefore(net::SiteId::RiverLine, B, N.Id) == W.RankBefore(B, N.Id);
		}
	}
	std::map<std::string, std::string> Was;
	for (const std::string& L : Lines(Old))
	{
		if (L.rfind("world\tnpc\t", 0) == 0)
		{
			Was[L.substr(0, L.find('\t', 10))] = L;
		}
	}
	int Kept = 0;
	int Lost = 0;
	for (const std::string& L : Lines(Out))
	{
		if (L.rfind("world\tnpc\t", 0) == 0)
		{
			const auto It = Was.find(L.substr(0, L.find('\t', 10)));
			const bool Ok = It != Was.end() && L.rfind(It->second + "\t", 0) == 0 && L.find('\t', It->second.size() + 1) == std::string::npos;
			(Ok ? Kept : Lost) += 1;
		}
	}
	int Wrong = 0;
	int Twice = 0;
	for (const world::Npc& N : W.People())
	{
		if (N.Faded)
		{
			continue;
		}
		const int Want = N.Pro ? Bit(net::SiteId::RiverLine) : Rule(W, N, Today(W));
		Wrong += N.Sites == Want ? 0 : 1;
		Twice += (N.Sites & Bit(net::SiteId::RunItTwice)) != 0 ? 1 : 0;
	}
	int Team = 0;
	for (const world::Npc& N : W.People())
	{
		Team += OnTeam(N, net::SiteId::RunItTwice) ? 1 : 0;
	}
	world::World Again;
	Load(Again, Out);
	Check(Same, "sites: an older world migrates to version 4 with RiverLine's boards (and their arrows) exactly as they were");
	Check(Lost == 0 && Kept > 1000, "sites: every npc line after the migration is the old one with its sites appended (" + std::to_string(Lost) + " changed otherwise)");
	Check(Wrong == 0 && Twice > 50, "sites: the migration gives everyone the accounts the rule has as of the load day (" + std::to_string(Twice) + " on RunItTwice)");
	Check(Team == 0 && W.SiteLaunched(net::SiteId::RunItTwice) == 1.0 * Day && W.SiteLaunched(net::SiteId::Ignite) == 28.0 * Day,
		"sites: a migrated world before Nov 2: RunItTwice open since Night One, Ignite on its day, Team RunItTwice signs at the next 1st");
	Check(Text(Again) == Out && Again.Began() == world::Start::Established, "sites: a migrated world reads back as it was written, migrated once");
	// A save past Nov 2 when the sites reach it: Ignite opens on the first day the world hasn't planned yet, with no history.
	world::World Before;
	Load(Before, WithVersion(Old, world::Version)); // (read as this version: no migration, no site; RiverLine alone)
	Before.AdvanceTo(40.0 * Day + 600.0);
	world::World Late;
	Load(Late, WithVersion(Text(Before), 3));
	const int LoadDay = Today(Late);
	const double Ignite = Late.SiteLaunched(net::SiteId::Ignite);
	const int Opens = Ignite >= 0.0 ? static_cast<int>(Ignite / Day) : -1;
	Late.AdvanceTo((Opens + 1) * Day + 600.0);
	int News = 0;
	bool Early = false;
	for (const world::WorldEvent& E : Late.Events())
	{
		News += E.Kind == world::EventKind::SiteLaunched && world::FlagSiteOf(E.Flags) == net::SiteId::Ignite && static_cast<int>(E.At / Day) == Opens ? 1 : 0;
		Early = Early || (world::FlagSiteOf(E.Flags) == net::SiteId::Ignite && E.At < Opens * Day);
	}
	std::printf("  sites: a world loaded on day %d past Nov 2: Ignite opens day %d\n", LoadDay, Opens);
	Check(Opens == LoadDay + 1 && News == 1 && !Early, "sites: a save past Nov 2 sees Ignite open the first day after loading, its launch that morning's news, no history before");
}

// ------------------------------------------------------------------ the boards' guard, the event kinds, the copy

void Guards(const world::World& W)
{
	bool Zero = true;
	for (const world::Npc& N : W.People())
	{
		for (const net::Board B : {net::Board::Today, net::Board::Month, net::Board::Ladder, net::Board::HighRollers})
		{
			Zero = Zero && W.RankBefore(B, N.Id) == 0 && W.RankBefore(net::SiteId::RiverLine, B, N.Id) == 0 && W.BoardValue(B, N.Id, W.Clock()) == 0.0 &&
				W.BoardValue(net::SiteId::RiverLine, B, N.Id, W.Clock()) == 0.0;
		}
	}
	Check(Zero, "sites: RanksYesterday is never read past Board::Live (a rival's boards have no RiverLine rank or value)");
	bool Named = std::string(world::EventKindName(world::EventKind::Count)).empty();
	std::set<std::string> Seen;
	for (int K = 0; K < static_cast<int>(world::EventKind::Count); ++K)
	{
		const std::string Name = world::EventKindName(static_cast<world::EventKind>(K));
		Named = Named && !Name.empty() && Seen.insert(Name).second;
	}
	Check(Named, "sites: EventKindName names every kind through Count, each once");
}

bool Machine(const std::string& Line)
{
	std::string Low;
	for (const char C : Line)
	{
		Low += C >= 'A' && C <= 'Z' ? static_cast<char>(C - 'A' + 'a') : C;
	}
	for (const char* Word : {" ai ", " ai.", " bot", "autoplay"})
	{
		if ((" " + Low).find(Word) != std::string::npos)
		{
			return true;
		}
	}
	return false;
}

void Copy(world::World& W)
{
	const int Mei = W.Find("Mei");
	const int Rick = W.Find("Rick");
	auto Event = [&](world::EventKind K, int Npc, const std::string& What, ss::Chips Amount, int Value, net::SiteId S, double At) {
		world::WorldEvent E;
		E.Kind = K;
		E.Npc = Npc;
		E.What = What;
		E.Amount = Amount;
		E.Value = Value;
		E.Flags = static_cast<int>(S) << world::FlagSiteShift;
		E.At = At;
		return E;
	};
	const std::string Ignite = net::Def(net::SiteId::Ignite).Name;
	const std::string Twice = net::Def(net::SiteId::RunItTwice).Name;
	const std::vector<std::pair<world::WorldEvent, std::string>> Samples = {
		{Event(world::EventKind::SiteLaunched, -1, "ignite", 0, 0, net::SiteId::Ignite, 28.0 * Day), Ignite + " opens its lobby"},
		{Event(world::EventKind::SiteLaunched, -1, "ignite", 0, 0, net::SiteId::Ignite, 45.0 * Day), Ignite + " opens its lobby"},
		{Event(world::EventKind::SiteLaunched, -1, "runittwice", 0, 0, net::SiteId::RunItTwice, NightOne), Twice + " opens its new lobby"},
		{Event(world::EventKind::SiteOverlay, -1, "$11 Sunday Blaze", 310012, 312, net::SiteId::Ignite, 33.0 * Day), "Ignite's $11 Sunday Blaze posts a $3,100 overlay"},
		{Event(world::EventKind::JoinedSite, Mei, "ignite", 0, 0, net::SiteId::Ignite, 29.0 * Day), "Mei opens an account on " + Ignite},
		{Event(world::EventKind::JoinedSite, Rick, "runittwice", 0, 0, net::SiteId::RunItTwice, 57.0 * Day), "Rick opens an account on " + Twice},
		{Event(world::EventKind::SignedSiteTeam, Rick, "Team RunItTwice", 0, 0, net::SiteId::RunItTwice, 57.0 * Day), "Rick joins Team RunItTwice"},
	};
	std::vector<std::pair<std::string, std::string>> Bodies;
	bool Read = true;
	bool Human = true;
	for (const auto& S : Samples)
	{
		std::string Title;
		std::string Body;
		std::string Tag;
		const bool Ok = W.Headline(S.first, Title, Body, Tag) && Title == S.second && !Tag.empty();
		if (!Ok)
		{
			std::printf("  sites headline: \"%s\" / \"%s\" (wanted \"%s\")\n", Title.c_str(), Body.c_str(), S.second.c_str());
		}
		Read = Read && Ok;
		Human = Human && !Machine(Title) && !Machine(Body);
		Bodies.push_back({Title, Body});
	}
	// (Mei and Rick are the Back Room's: the news knows them.)
	Check(Read && Mei >= 0 && Rick >= 0, "sites: the rivals' headlines read as written");
	Check(Human, "sites: no rival headline says the game played for anyone (U1)");
	Check(copylint::Run("world-sites", Bodies, copylint::Pa) == 0, "sites: the rivals' headlines pass the copy lint");
	// No real site and none of the ten that closed in the world's own story (WorldText.cpp's arrivals).
	bool Clean = true;
	for (const auto& B : Bodies)
	{
		for (const char* Name : {"PokerStars", "GGPoker", "partypoker", "888poker", "ACR", "Ignition", "BetOnline", "WPT Global", "CoinPoker", "Natural8", "Winamax", "Unibet",
				 "Run It Once", "NorthPot", "BluffHarbor", "AceLine", "ChipCity", "Felt Republic", "PocketRocket", "RiverBend Poker", "Saltwater Poker", "CardRoom 88", "Tiltproof"})
		{
			Clean = Clean && B.first.find(Name) == std::string::npos && B.second.find(Name) == std::string::npos;
		}
	}
	Check(Clean, "sites: the rivals' headlines name no real site and no closed one");
	// A stranger's account is nobody's news; the launch week's guarantees only on the launch's own dates.
	std::string Title;
	std::string Body;
	std::string Tag;
	const int Nobody = [&]() {
		for (const world::Npc& N : W.People())
		{
			if (N.RepOf(world::Rep::Overall) < 15.0f && !N.Anchored && !N.Pro && !N.Streams)
			{
				return N.Id;
			}
		}
		return -1;
	}();
	Check(Nobody >= 0 && !W.Headline(Event(world::EventKind::JoinedSite, Nobody, "ignite", 0, 0, net::SiteId::Ignite, 29.0 * Day), Title, Body, Tag),
		"sites: a regular nobody follows opening an account isn't news");
	W.Headline(Samples[0].first, Title, Body, Tag);
	const bool Week = Body.find("Launch week") != std::string::npos;
	W.Headline(Samples[1].first, Title, Body, Tag);
	Check(Week && Body.find("Launch week") == std::string::npos, "sites: only a launch on its own date promises its launch week (" + Body + ")");
}

// ------------------------------------------------------------------ Update 3 G2: the League's world side (core-world)
// SPEC §3.6 R2 and §7.7's living_world row: the group's world\tleague line written and read back exactly, within 2.5 KB; a
// member's paid RunItTwice result recorded as a daily best through site::play::LeagueScore, a non-member's not; the
// division's floor, the first 3 entries a day (or the group's pace), the best 8, the days and the form; a result after the
// 1st's 6 AM turn left out of last month's book; nothing else in the world moving with a group; a world without the line
// loading as today; a world built from the text alone (a card room's) recording the group too; the 30 days' budget with a
// group (S-6'); and what core-siteplay reads from the world (monthly volumes, the pace, Team RunItTwice's six, Ignite's
// Headliners). The points are whatever LeagueScore says, so these hold before core-siteplay's body lands (the stub scores
// nothing, and says so) and after.

/** RunItTwice's members at a tier (playing), the busiest this season first, then by id. */
std::vector<int> LeaguePeople(const world::World& W, int Tier)
{
	std::vector<std::pair<int, int>> Ranked;
	for (const world::Npc& N : W.People())
	{
		if (N.Playing() && (N.Sites & Bit(net::SiteId::RunItTwice)) != 0 && N.Tier == Tier)
		{
			const world::SiteRecord* R = W.SiteRecordOf(net::SiteId::RunItTwice, N.Id);
			Ranked.push_back({-(R ? R->Events : 0), N.Id});
		}
	}
	std::sort(Ranked.begin(), Ranked.end());
	std::vector<int> Out;
	for (const std::pair<int, int>& It : Ranked)
	{
		Out.push_back(It.second);
	}
	return Out;
}

/** The world text's League lines (Keep) or everything else, a line each. */
std::string LeagueLines(const std::string& T, bool Keep)
{
	std::string Out;
	for (const std::string& L : Lines(T))
	{
		if ((L.compare(0, 13, "world\tleague\t") == 0) == Keep)
		{
			Out += L + "\n";
		}
	}
	return Out;
}

/** Who the world has registered for RunItTwice's events starting between two minutes (the days it has planned). */
std::set<int> LeagueBusy(const world::World& W, double From, double To)
{
	std::set<int> Out;
	for (const net::EventInstance& E : net::Site(net::SiteId::RunItTwice).Window(From, To))
	{
		for (const int Id : W.RegisteredNow(E.Id))
		{
			Out.insert(Id);
		}
	}
	return Out;
}

/** The first instance of a RunItTwice template starting in [From, To). */
bool LeagueEvent(const std::string& Template, double From, double To, net::EventInstance& Out)
{
	const net::Network& Net = net::Site(net::SiteId::RunItTwice);
	for (const net::EventInstance& E : Net.Window(From, To))
	{
		if (Net.TemplateOf(E).Id == Template && E.Start >= From)
		{
			Out = E;
			return true;
		}
	}
	return false;
}

/** A day's slot in a member's best 8 (0: none). */
int LeagueHeld(const world::LeagueMember& M, int Date)
{
	for (const std::pair<int8_t, int16_t>& Slot : M.Best)
	{
		if (Slot.second > 0 && Slot.first == Date)
		{
			return Slot.second;
		}
	}
	return 0;
}

/** §7.4's bounds on a book: at most 19, each once; days and form in range; the best 8 sorted, the empty slots last. */
bool LeagueSane(const world::LeagueBook& B, std::string& Why)
{
	std::set<int> Seen;
	if (B.Members.size() > 19)
	{
		Why = "more than 19";
		return false;
	}
	for (const world::LeagueMember& M : B.Members)
	{
		if (M.Npc < 0 || !Seen.insert(M.Npc).second || M.Days < 0 || M.Days > 31 || M.TodayCount < 0 || M.TodayCount > 3 || M.Form > 0x3ff)
		{
			Why = "member " + std::to_string(M.Npc) + "'s counts";
			return false;
		}
		bool Empty = false;
		for (size_t K = 0; K < M.Best.size(); ++K)
		{
			const std::pair<int8_t, int16_t>& S = M.Best[K];
			const bool Held = S.second > 0;
			if ((Empty && Held) || (Held && (S.first < 1 || S.first > 31)) || (!Held && (S.first != 0 || S.second != 0)) ||
				(K > 0 && Held && (M.Best[K - 1].second < S.second || (M.Best[K - 1].second == S.second && M.Best[K - 1].first > S.first))))
			{
				Why = "member " + std::to_string(M.Npc) + "'s best days";
				return false;
			}
			Empty = Empty || !Held;
		}
	}
	return true;
}

/** The rules with no world: the 6 AM day and month, the packed division and pace, a member's points and form. */
void LeagueRules()
{
	const int Dec1 = world::DayOn(2026, 12, 1);
	Check(world::LeagueDay(Dec1 * Day + 359.9) == Dec1 - 1 && world::LeagueDay(Dec1 * Day + 360.0) == Dec1 && world::LeagueDay(Dec1 * Day + 1439.0) == Dec1,
		"league: the League's day turns at 6 AM");
	Check(world::LeagueMonth(Dec1 * Day + 359.0) == world::MonthKeyOf(Dec1 - 1) && world::LeagueMonth(Dec1 * Day + 360.0) == world::MonthKeyOf(Dec1),
		"league: the League's month turns on the 1st at 6 AM");
	bool Packs = world::LeagueDivision(world::LeaguePack(9, 7)) == 4 && world::LeagueFirst(world::LeaguePack(9, 7)) == 3 && world::LeaguePack(-2, -1) == 0;
	for (int D = 0; D <= 4; ++D)
	{
		for (int P = 0; P <= 3; ++P)
		{
			const int Packed = world::LeaguePack(D, P);
			Packs = Packs && Packed >= 0 && Packed <= 127 && world::LeagueDivision(Packed) == D && world::LeagueFirst(Packed) == (P > 0 ? P : 3);
		}
	}
	Check(Packs, "league: the division and the group's pace pack into SetLeagueRoster's Div and come back out (pace 0: the first 3)");
	world::LeagueMember M;
	M.Npc = 3;
	M.Days = 3;
	M.Form = static_cast<uint16_t>((3 << 4) | (2 << 2) | 1);
	M.Best[0] = {static_cast<int8_t>(5), static_cast<int16_t>(1234)};
	M.Best[1] = {static_cast<int8_t>(3), static_cast<int16_t>(66)};
	const std::vector<int> Form = world::LeagueForm(M);
	Check(std::fabs(world::LeaguePoints(M) - 130.0) < 1e-9 && Form == std::vector<int>({1, 2, 3}),
		"league: a member's points are their best days' tenths summed, their form newest first");
}

/**
 * A group of 19 Steel members in December, recorded beside the same world with no group: one member wins five events on
 * Dec 8 (the first under Steel's $11 floor) and The Sweat on Deadline Day, someone outside the group is second; then the
 * line, the month's turn, the roster's own rules, a card room's world from the text, and January's group at a pace of 1.
 * Base is a fresh world (Seed) that this plays to Dec 7 at noon and leaves there for LeagueQueries; DecemberText is it at
 * Dec 1, 6 AM (LeagueBudget's start).
 */
void LeagueScoring(world::World& Base, uint32_t Seed, std::string& DecemberText)
{
	namespace play = ss::site::play;
	const net::SiteId Rit = net::SiteId::RunItTwice;
	const int Dec1 = world::DayOn(2026, 12, 1);
	const int Dec8 = Dec1 + 7;
	const int Dec31 = Dec1 + 30;
	const int Jan1 = world::DayOn(2027, 1, 1);
	const bool Live = play::LeagueScore(2, "runittwice.the-sweat", 1100, Dec8, 10.0, false, false) > 0.0;
	std::printf("  league: site::play::LeagueScore %s\n", Live ? "scores (core-siteplay's body)" : "is the Phase A stub and scores nothing: the checks below hold with no points");
	Base.Create(Seed, NightOne);
	Base.AdvanceTo(27.0 * Day + 720.0); // Nov 1: Ignite opens tomorrow
	Check(world::Headliners(Base, Base.Clock()).empty(), "league: Ignite has no Headliners before it opens");
	Base.AdvanceTo(Dec1 * Day + 360.0);
	DecemberText = Text(Base);
	Base.AdvanceTo((Dec8 - 1) * Day + 720.0); // Dec 7 at noon: Dec 8's registrations are planned
	const std::string Saved = Text(Base);
	world::World Plain;
	Load(Plain, Saved);
	world::World Grouped;
	Load(Grouped, Saved);
	Check(LeagueLines(Saved, true).empty() && !Grouped.League(Rit) && Text(Grouped) == Saved,
		"league: a world nobody drew a group in has no League line or book, and loads and writes back as it was");
	// The group: Steel's busiest, with two people the world hasn't put in any RunItTwice event on Dec 7 or 8 (whose results there
	// are the ones forced below): the member, and the outsider just past the 19.
	const std::set<int> Busy = LeagueBusy(Grouped, (Dec8 - 1) * Day, (Dec8 + 1) * Day + 360.0);
	const std::vector<int> Steel = LeaguePeople(Grouped, 2);
	int Member = -1;
	int Outsider = -1;
	std::vector<int> Roster;
	for (const int Id : Steel)
	{
		if (Member < 0 && Busy.count(Id) == 0)
		{
			Member = Id;
			Roster.push_back(Id);
		}
	}
	for (const int Id : Steel)
	{
		if (Roster.size() < 19 && Id != Member)
		{
			Roster.push_back(Id);
		}
	}
	for (const int Id : Steel)
	{
		if (Outsider < 0 && Busy.count(Id) == 0 && std::find(Roster.begin(), Roster.end(), Id) == Roster.end())
		{
			Outsider = Id;
		}
	}
	if (Member < 0 || Outsider < 0 || Roster.size() < 19)
	{
		Check(false, "league: 21 Steel members on RunItTwice in December, two of them free on Dec 8 (" + std::to_string(Steel.size()) + ")");
		return;
	}
	const int Month = world::LeagueMonth(Grouped.Clock());
	const int HeroRev = Grouped.HeroRevision();
	Grouped.SetLeagueRoster(Rit, Month, world::LeaguePack(2, 0), Roster);
	Check(Grouped.HeroRevision() != HeroRev, "league: handing the world a group moves its HeroRevision (the session's next save writes the world)");
	const world::LeagueBook* Book = Grouped.League(Rit);
	Check(Book && Book->Members.size() == 19 && Book->MonthKey == Month && world::LeagueDivision(Book->Div) == 2 && world::LeagueFirst(Book->Div) == 3,
		"league: SetLeagueRoster keeps the month's group of 19 and League reads it back");
	// Dec 8: the member wins five of RunItTwice's evening events and the outsider is second in The Sweat (both worlds).
	struct Forced
	{
		std::string Template;
		net::EventInstance E;
		double Raw = 0.0;
	};
	std::vector<Forced> Force;
	for (const char* T : {"runittwice.supper-stack", "runittwice.the-sweat", "runittwice.run-it-back", "runittwice.encore-pko", "runittwice.the-fifty-five"})
	{
		Forced F;
		F.Template = T;
		if (LeagueEvent(T, Dec8 * Day + 360.0, (Dec8 + 1) * Day, F.E) && !Grouped.RegisteredNow(F.E.Id).empty())
		{
			Force.push_back(F);
		}
	}
	Check(Force.size() == 5, "league: Dec 8's five evening events are planned (" + std::to_string(Force.size()) + ")");
	for (world::World* Each : {&Plain, &Grouped})
	{
		for (const Forced& F : Force)
		{
			std::vector<std::pair<int, int>> Places = {{Member, 1}};
			if (F.Template == "runittwice.the-sweat")
			{
				Places.push_back({Outsider, 2});
			}
			Each->KnownPlaces(F.E.Id, Places);
		}
	}
	std::stable_sort(Force.begin(), Force.end(), [](const Forced& A, const Forced& B) { return A.E.Start + A.E.Duration < B.E.Start + B.E.Duration; });
	Plain.AdvanceTo(Dec8 * Day + 360.0);
	Grouped.AdvanceTo(Dec8 * Day + 360.0);
	const world::LeagueMember* Was = world::LeagueMemberOf(*Grouped.League(Rit), Member);
	const world::LeagueMember Before = Was ? *Was : world::LeagueMember();
	bool Alone = true;
	for (Forced& F : Force)
	{
		// Each result in turn, the member's raw points read off their RunItTwice record across the event's end.
		const double End = F.E.Start + F.E.Duration;
		Plain.AdvanceTo(End - 0.5);
		Grouped.AdvanceTo(End - 0.5);
		const world::SiteRecord* R0 = Grouped.SiteRecordOf(Rit, Member);
		const double Points0 = R0 ? R0->Points : 0.0;
		const int Events0 = R0 ? R0->Events : 0;
		Plain.AdvanceTo(End + 0.5);
		Grouped.AdvanceTo(End + 0.5);
		const world::SiteRecord* R1 = Grouped.SiteRecordOf(Rit, Member);
		F.Raw = (R1 ? R1->Points : 0.0) - Points0;
		Alone = Alone && R1 && R1->Events == Events0 + 1 && F.Raw > 0.0;
	}
	Check(Alone, "league: the member's five wins are their only RunItTwice results on Dec 8, each with points");
	// What the rule says: entries at or above the floor, the first 3 of them as they ended, the day's best of those.
	int Counted = 0;
	double DayBest = 0.0;
	for (const Forced& F : Force)
	{
		const net::EventTemplate& T = net::Site(Rit).TemplateOf(F.E);
		const int Played = world::LeagueDay(F.E.Start + F.E.Duration); // the play day the result came in
		if (Counted >= 3 || play::LeagueScore(2, F.Template, T.BuyInCents, Played, 1.0, false, false) <= 0.0)
		{
			continue;
		}
		++Counted;
		DayBest = std::max(DayBest, play::LeagueScore(2, F.Template, T.BuyInCents, Played, F.Raw, !T.Series.empty(), false));
	}
	const int Tenths = static_cast<int>(std::min<long long>(32767, std::llround(DayBest * 10.0)));
	const world::LeagueMember* After = world::LeagueMemberOf(*Grouped.League(Rit), Member);
	Check(After && After->TodayKey == (Counted > 0 ? Dec8 : Before.TodayKey) && After->TodayCount == (Counted > 0 ? Counted : Before.TodayCount),
		"league: a member's entries count from the first 3 a day at or above the division's floor (" + std::to_string(Counted) + " of 5)");
	Check(After && After->Days == Before.Days + (Counted > 0 ? 1 : 0) && (Counted == 0 || (After->Form & 3) == 3),
		"league: a League day is counted once, with its form (a win: a final table)");
	Check(After && LeagueHeld(*After, 8) == (Tenths > 0 ? Tenths : 0), "league: the day's best is the best of those entries through LeagueScore, in tenths (" + std::to_string(Tenths) + ")");
	Check(!world::LeagueMemberOf(*Grouped.League(Rit), Outsider) && Grouped.League(Rit)->Members.size() == 19, "league: someone outside the group isn't recorded");
	Check(!Live || (Counted == 3 && Tenths > 0), "league: Steel's floor leaves out the $5.50 entry, and the 4th entry at it is past the first 3");
	// A world built from the text alone (a card room's, with no session), from Dec 30 at noon: it reads the line back exactly
	// and goes on recording the group as this one does.
	Plain.AdvanceTo((Dec31 - 1) * Day + 720.0);
	Grouped.AdvanceTo((Dec31 - 1) * Day + 720.0);
	const std::string Dec30 = Text(Grouped);
	world::World Room;
	Load(Room, Dec30);
	Check(Text(Room) == Dec30, "league: the League line reads back exactly");
	// Deadline Day: a member the world has nothing planned for on the 31st wins The Sweat (featured events count x1.25 then, by
	// the rule), in all three worlds.
	net::EventInstance Last;
	const bool Found = LeagueEvent("runittwice.the-sweat", Dec31 * Day + 360.0, (Dec31 + 1) * Day, Last);
	const std::set<int> BusyLast = LeagueBusy(Grouped, Dec31 * Day, (Dec31 + 1) * Day + 360.0);
	int Closer = -1;
	for (const int Id : Roster)
	{
		Closer = Closer < 0 && BusyLast.count(Id) == 0 ? Id : Closer;
	}
	const bool Free = Found && Closer >= 0 && !Grouped.RegisteredNow(Last.Id).empty();
	const std::vector<world::World*> Three = {&Plain, &Grouped, &Room};
	for (world::World* Each : Three)
	{
		if (Free)
		{
			Each->KnownPlaces(Last.Id, {{Closer, 1}});
		}
		Each->AdvanceTo(Last.Start + Last.Duration - 0.5);
	}
	const world::SiteRecord* L0 = Grouped.SiteRecordOf(Rit, Closer);
	const double LastPoints0 = L0 ? L0->Points : 0.0;
	for (world::World* Each : Three)
	{
		Each->AdvanceTo(Last.Start + Last.Duration + 0.5);
	}
	const world::SiteRecord* L1 = Grouped.SiteRecordOf(Rit, Closer);
	const double LastRaw = (L1 ? L1->Points : 0.0) - LastPoints0;
	const net::EventTemplate& Sweat = net::Site(Rit).TemplateOf(Last);
	const double Deadline = play::LeagueScore(2, Sweat.Id, Sweat.BuyInCents, world::LeagueDay(Last.Start + Last.Duration), LastRaw, !Sweat.Series.empty(), true);
	const int DeadlineTenths = static_cast<int>(std::min<long long>(32767, std::llround(Deadline * 10.0)));
	const world::LeagueMember* OnDeadline = world::LeagueMemberOf(*Grouped.League(Rit), Closer);
	const int HeldLast = OnDeadline ? LeagueHeld(*OnDeadline, 31) : -1;
	Check(Free && LastRaw > 0.0 && (HeldLast == DeadlineTenths || (HeldLast == 0 && OnDeadline && OnDeadline->Best.back().second >= DeadlineTenths)),
		"league: Deadline Day's result is scored with the Deadline flag (" + std::to_string(DeadlineTenths) + " tenths)");
	// Nothing else moved: the two worlds differ by the League line alone, all month; the card room's world kept up.
	for (world::World* Each : Three)
	{
		Each->AdvanceTo(Dec31 * Day + 1380.0);
	}
	const std::string Month31 = Text(Grouped);
	Check(LeagueLines(Text(Plain), false) == LeagueLines(Month31, false), "league: recording a group moves nothing else in the world (its text less the League line is the same)");
	const std::string Line = LeagueLines(Month31, true);
	Check(LeagueLines(Text(Room), true) == Line && (!Live || Line != LeagueLines(Dec30, true)), "league: a world built from the text (a card room's) records the group too");
	std::printf("  league: the group's line at the month's end is %zu bytes (budget 2,500), the member's month %.1f points\n", Line.empty() ? 0 : Line.size() - 1,
		world::LeagueMemberOf(*Grouped.League(Rit), Member) ? world::LeaguePoints(*world::LeagueMemberOf(*Grouped.League(Rit), Member)) : 0.0);
	Check(!Line.empty() && Line.size() - 1 <= 2500, "league: the month's world\\tleague line is at most 2.5 KB");
	// The longest line there can be: 19 members with 5-digit ids, every day of the month played, full form, today's 3, and all 8
	// best days at the cap. It reads back exactly and still fits.
	std::string Worst = "world\tleague\t2\t" + std::to_string(Month) + "\t" + std::to_string(world::LeaguePack(4, 3));
	std::string Members;
	for (int K = 0; K < 19; ++K)
	{
		Worst += "\t" + std::to_string(10000 + K);
		Members += (K ? "," : "") + std::to_string(10000 + K) + ":31:1023:9999:3";
		for (int B = 0; B < 8; ++B)
		{
			Members += ":" + std::to_string(31 - B) + "/32767";
		}
	}
	Worst += "\t" + Members + "\n";
	world::World Longest;
	Load(Longest, LeagueLines(Dec30, false) + Worst);
	std::printf("  league: the longest possible League line is %zu bytes\n", Worst.size() - 1);
	Check(LeagueLines(Text(Longest), true) == Worst && Worst.size() - 1 <= 2500, "league: the longest possible League line reads back exactly and is at most 2.5 KB");
	std::string Why;
	Check(LeagueSane(*Grouped.League(Rit), Why), "league: the book keeps §7.4's bounds (" + Why + ")");
	int Recorded = 0;
	for (const world::LeagueMember& M : Grouped.League(Rit)->Members)
	{
		Recorded += M.Days > 0 ? 1 : 0;
	}
	Check(!Live || Recorded >= 10, "league: most of the group has League days by the month's end (" + std::to_string(Recorded) + " of 19)");
	// The 1st's turn at 6 AM: nothing ended after it lands in December's book.
	Room.AdvanceTo(Jan1 * Day + 359.0);
	const std::string AtTurn = LeagueLines(Text(Room), true);
	Room.AdvanceTo(Jan1 * Day + 1380.0);
	Check(LeagueLines(Text(Room), true) == AtTurn, "league: a result after the 1st's 6 AM turn stays out of last month's book");
	// Handing over the same month's group again keeps what's recorded; a new month's starts everyone at nothing; the guards.
	const world::LeagueBook December = *Room.League(Rit);
	std::vector<int> Reversed(Roster.rbegin(), Roster.rend());
	Room.SetLeagueRoster(Rit, December.MonthKey, December.Div, Reversed);
	bool Kept = Room.League(Rit)->Members.size() == 19;
	for (const world::LeagueMember& M : December.Members)
	{
		const world::LeagueMember* Now = world::LeagueMemberOf(*Room.League(Rit), M.Npc);
		Kept = Kept && Now && Now->Days == M.Days && Now->Form == M.Form && Now->Best == M.Best && Now->TodayKey == M.TodayKey;
	}
	Check(Kept, "league: the same month's group handed over again keeps what the world recorded");
	std::vector<int> Messy = Roster;
	Messy.insert(Messy.begin(), {Roster[0], -1, 1000000, Outsider});
	Messy.push_back(Outsider);
	Room.SetLeagueRoster(Rit, December.MonthKey + 1, world::LeaguePack(2, 1), Messy);
	const world::LeagueBook* January = Room.League(Rit);
	bool Fresh = January && January->MonthKey == December.MonthKey + 1 && January->Members.size() == 19 && world::LeagueFirst(January->Div) == 1;
	std::set<int> Ids;
	for (const world::LeagueMember& M : January ? January->Members : std::vector<world::LeagueMember>())
	{
		Fresh = Fresh && Ids.insert(M.Npc).second && Room.Get(M.Npc) && M.Days == 0 && M.Best[0].second == 0 && M.TodayKey == -1;
	}
	Check(Fresh, "league: a new month's group starts everyone at nothing: at most 19, each once, only people the world has");
	// January at a pace of 1: the member's second entry of a day doesn't count.
	const int Jan6 = Jan1 + 5;
	Room.AdvanceTo((Jan6 - 1) * Day + 720.0);
	std::vector<net::EventInstance> Two(2);
	const bool Pair = LeagueEvent("runittwice.the-sweat", Jan6 * Day + 360.0, (Jan6 + 1) * Day, Two[0]) && LeagueEvent("runittwice.encore-pko", Jan6 * Day + 360.0, (Jan6 + 1) * Day, Two[1]) &&
		LeagueBusy(Room, (Jan6 - 1) * Day, (Jan6 + 1) * Day + 360.0).count(Roster[0]) == 0 && !Room.RegisteredNow(Two[0].Id).empty() && !Room.RegisteredNow(Two[1].Id).empty();
	if (Pair)
	{
		Room.KnownPlaces(Two[0].Id, {{Roster[0], 1}});
		Room.KnownPlaces(Two[1].Id, {{Roster[0], 1}});
	}
	const world::LeagueMember* Unpaced = world::LeagueMemberOf(*Room.League(Rit), Roster[0]);
	const int DaysBefore = Unpaced ? Unpaced->Days : -1;
	Room.AdvanceTo((Jan6 + 1) * Day + 300.0);
	const world::LeagueMember* Paced = world::LeagueMemberOf(*Room.League(Rit), Roster[0]);
	Check(Pair && Paced && Paced->TodayCount == (Live ? 1 : 0) && Paced->Days == DaysBefore + (Live ? 1 : 0) && (!Live || Paced->TodayKey == Jan6),
		"league: at the group's pace of 1 only a day's first entry counts");
	Room.SetLeagueRoster(Rit, December.MonthKey + 1, world::LeaguePack(2, 0), {});
	Room.SetLeagueRoster(net::SiteId::RiverLine, December.MonthKey + 1, 0, Roster);
	Check(!Room.League(Rit) && !Room.League(net::SiteId::RiverLine) && LeagueLines(Text(Room), true).empty(), "league: an empty group clears the book (no line); RiverLine has none");
}

/**
 * S-6' (budget): the world's 30 days from Dec 1, side by side, with a group of the 19 busiest Steel members recorded and
 * without one. The two worlds take turns a day at a time (which goes first alternates), so a shared machine's load falls on
 * both alike; Reps fresh pairs. The day's pair is timed back to back, so the median of the days' ratios shrugs off a spike
 * the totals can't (under ctest -j and other builds the totals swing several percent either way). Measured with more pairs
 * in S-6p.md.
 */
void LeagueBudget(const std::string& DecemberText, int Reps)
{
	double Plain = 0.0;
	double Grouped = 0.0;
	std::vector<double> Ratios;
	size_t Line = 0;
	for (int K = 0; K < Reps; ++K)
	{
		world::World A;
		Load(A, DecemberText);
		world::World B;
		Load(B, DecemberText);
		std::vector<int> Busiest = LeaguePeople(B, 2);
		Busiest.resize(std::min<size_t>(19, Busiest.size()));
		B.SetLeagueRoster(net::SiteId::RunItTwice, world::LeagueMonth(B.Clock()), world::LeaguePack(2, 0), Busiest);
		const double Start = A.Clock();
		for (int D = 1; D <= 30; ++D)
		{
			const double To = Start + static_cast<double>(D) * Day - 1.0;
			double Took[2] = {0.0, 0.0};
			for (int Turn = 0; Turn < 2; ++Turn)
			{
				const bool Group = (Turn == 0) == ((D + K) % 2 == 0);
				const auto T0 = std::chrono::steady_clock::now();
				(Group ? B : A).AdvanceTo(To);
				Took[Group ? 1 : 0] = Seconds(T0);
			}
			Plain += Took[0];
			Grouped += Took[1];
			if (Took[0] > 0.0)
			{
				Ratios.push_back(Took[1] / Took[0]);
			}
		}
		const std::string L = LeagueLines(Text(B), true);
		Line = L.empty() ? 0 : L.size() - 1;
	}
	std::sort(Ratios.begin(), Ratios.end());
	const double Median = Ratios.empty() ? 1.0 : Ratios[Ratios.size() / 2];
	std::printf("  league: 30 days from Dec 1 (%d pairs, a day each in turn): %.3f s a month with no group, %.3f s with the 19 busiest Steel members (%+.2f%%; the "
				"median day %+.2f%%); their line %zu B\n",
		Reps, Plain / Reps, Grouped / Reps, (Grouped / Plain - 1.0) * 100.0, (Median - 1.0) * 100.0, Line);
	Check(Median <= 1.05, "league: recording a group costs the world's 30 days next to nothing (S-6': +1%; the median day held to +5% here, wall clock)");
}

/**
 * What core-siteplay reads: Ignite's Headliners, Team RunItTwice's six, the members' monthly volumes and the group's pace.
 * W is LeagueScoring's world at Dec 7 at noon (Seed's), played on to Dec 20 and January 1 here.
 */
void LeagueQueries(world::World& W, uint32_t Seed)
{
	const net::SiteId Rit = net::SiteId::RunItTwice;
	const int Dec3 = world::DayOn(2026, 12, 3);
	const std::vector<world::Headliner> Early = world::Headliners(W, W.Clock(), 100000);
	W.AdvanceTo((Dec3 + 17) * Day + 720.0);
	const std::vector<world::Headliner> All = world::Headliners(W, W.Clock(), 100000);
	const std::vector<world::Headliner> Six = world::Headliners(W, W.Clock());
	bool Ordered = Six.size() == std::min<size_t>(6, All.size()) && !Six.empty();
	for (size_t K = 0; K < All.size(); ++K)
	{
		const world::Npc* N = W.Get(All[K].Npc);
		Ordered = Ordered && N && N->Playing() && (N->Sites & Bit(net::SiteId::Ignite)) != 0 && N->Followers == All[K].Followers;
		Ordered = Ordered && (All[K].Followers > 0 || All[K].PointsBefore > 0.0 || All[K].WonBefore > 0 || All[K].PointsMonth > 0.0);
		const world::SiteRecord* Rk = W.SiteRecordOf(net::SiteId::Ignite, All[K].Npc);
		Ordered = Ordered && All[K].WonBefore == (Rk ? std::max<ss::Chips>(0, Rk->Lifetime - Rk->Won) : 0);
		if (K > 0)
		{
			const world::Headliner& P = All[K - 1];
			const world::Headliner& H = All[K];
			const bool Rest = P.WonBefore > H.WonBefore ||
				(P.WonBefore == H.WonBefore && (P.PointsMonth > H.PointsMonth || (P.PointsMonth == H.PointsMonth && P.Npc < H.Npc)));
			Ordered = Ordered && (P.Followers > H.Followers || (P.Followers == H.Followers && (P.PointsBefore > H.PointsBefore || (P.PointsBefore == H.PointsBefore && Rest))));
		}
		Ordered = Ordered && (K >= Six.size() || Six[K].Npc == All[K].Npc);
	}
	Check(Ordered, "league: Ignite's Headliners are its playing members by followers, then points before the month, Ignite money before the season, this month's points, then id (six)");
	std::map<int, double> EarlyBefore;
	for (const world::Headliner& H : Early)
	{
		EarlyBefore[H.Npc] = H.PointsBefore;
	}
	int Same = 0;
	int Moved = 0;
	for (const world::Headliner& H : All)
	{
		const auto It = EarlyBefore.find(H.Npc);
		if (It != EarlyBefore.end())
		{
			(It->second == H.PointsBefore ? Same : Moved) += 1;
		}
	}
	Check(Same > 0 && Moved == 0, "league: a Headliner's points before the month stay put all month (" + std::to_string(Moved) + " moved of " + std::to_string(Same + Moved) + ")");
	std::string Names;
	for (const world::Headliner& H : Six)
	{
		Names += (Names.empty() ? "" : ", ") + W.Get(H.Npc)->Name + " (" + std::to_string(H.Followers) + ")";
	}
	std::printf("  league: Ignite's Headliners on Dec 20: %s\n", Names.c_str());
	// Team RunItTwice's six: the people the world signed (Npc::Sponsor), by id; nobody else's team.
	const std::vector<int> Team = world::SiteTeamNow(W, Rit);
	int Signed = 0;
	for (const world::Npc& N : W.People())
	{
		Signed += OnTeam(N, Rit) ? 1 : 0;
	}
	bool Pros = static_cast<int>(Team.size()) == Signed && Team.size() == 6 && std::is_sorted(Team.begin(), Team.end());
	for (const int Id : Team)
	{
		Pros = Pros && OnTeam(*W.Get(Id), Rit);
	}
	Check(Pros && world::SiteTeamNow(W, net::SiteId::Ignite).empty() && world::SiteTeamNow(W, net::SiteId::RiverLine).empty(),
		"league: Team RunItTwice's current six are the world's signed pros (" + std::to_string(Team.size()) + ")");
	// The monthly volumes: this season's entries per 30 days since the season began for them (at least a week).
	const double At = W.Clock();
	const std::vector<world::SiteMember> Members = world::SiteMembers(W, Rit, At);
	int Playing = 0;
	for (const world::Npc& N : W.People())
	{
		Playing += N.Playing() && !N.Faded && (N.Sites & Bit(Rit)) != 0 ? 1 : 0;
	}
	bool Volumes = static_cast<int>(Members.size()) == Playing && Playing > 100;
	std::vector<double> December;
	for (const world::SiteMember& M : Members)
	{
		const world::Npc* N = W.Get(M.Npc);
		const world::SiteRecord* R = W.SiteRecordOf(Rit, M.Npc);
		const double From = std::max({static_cast<double>(world::YearStart(world::YearOf(Today(W)))) * Day, W.SiteLaunched(Rit), static_cast<double>(N->Joined) * Day});
		const double Season = (At - From) / Day;
		const double Want = static_cast<double>(R ? R->Events : 0) * 30.0 / Season;
		Volumes = Volumes && M.Volume >= 0.0 && (Season < 7.0 || std::fabs(M.Volume - Want) < 1e-9) && M.Events == (R ? R->Events : 0) && M.Tier == N->Tier &&
			std::fabs(world::MonthlyVolume(W, Rit, M.Npc, At) - M.Volume) < 1e-9;
		December.push_back(M.Volume);
	}
	int Outside = -1;
	for (const world::Npc& N : W.People())
	{
		Outside = Outside < 0 && (N.Sites & Bit(Rit)) == 0 ? N.Id : Outside;
	}
	Check(Volumes && Outside >= 0 && world::MonthlyVolume(W, Rit, Outside, At) == -1.0 && world::MonthlyVolume(W, net::SiteId::RiverLine, Members[0].Npc, At) == -1.0,
		"league: a member's monthly volume is their season's entries per 30 days (-1 for a non-member, and on RiverLine)");
	std::vector<int> Group;
	for (size_t K = 0; K < Members.size() && Group.size() < 19; ++K)
	{
		Group.push_back(Members[K].Npc);
	}
	const int Pace = world::LeaguePaceOf(W, Group, At);
	Check(Pace >= 1 && Pace <= 3 && world::LeaguePaceOf(W, {}, At) == 3, "league: the group's pace is 1-3 (" + std::to_string(Pace) + ")");
	// January 1 at 6 AM, the new season's first morning (every count starts again): the volumes still read the members' real
	// pace, from what the world has planned for them, about as December's did.
	W.AdvanceTo(world::DayOn(2027, 1, 1) * Day + 360.0);
	std::vector<double> January;
	int Some = 0;
	for (const world::SiteMember& M : world::SiteMembers(W, Rit, W.Clock()))
	{
		January.push_back(M.Volume);
		Some += M.Volume > 0.0 ? 1 : 0;
	}
	std::sort(December.begin(), December.end());
	std::sort(January.begin(), January.end());
	const double DecMedian = December.empty() ? 0.0 : December[December.size() / 2];
	const double JanMedian = January.empty() ? 0.0 : January[January.size() / 2];
	std::printf("  league: RunItTwice's members' monthly volume, median %.1f on Dec 20 and %.1f on Jan 1 at 6 AM (%d of %zu above 0)\n", DecMedian, JanMedian, Some, January.size());
	Check(!January.empty() && Some * 2 >= static_cast<int>(January.size()) && JanMedian >= 0.5 * DecMedian && JanMedian <= 2.0 * DecMedian,
		"league: on January 1 the members' monthly volumes still read their pace (the season's counts start again)");
	// A fresh world bills on points alone (Season 1: the first headliners).
	world::World Blank;
	Blank.Create(Seed, NightOne, world::Start::Blank);
	Blank.AdvanceTo(world::DayOn(2026, 11, 16) * Day + 720.0); // two weeks into Ignite's first season
	const std::vector<world::Headliner> First = world::Headliners(Blank, Blank.Clock(), 100000);
	bool ByPoints = !First.empty();
	for (size_t K = 0; K < First.size(); ++K)
	{
		ByPoints = ByPoints && (First[K].PointsBefore > 0.0 || First[K].PointsMonth > 0.0);
		ByPoints = ByPoints && (K == 0 || First[K - 1].PointsBefore > First[K].PointsBefore ||
								   (First[K - 1].PointsBefore == First[K].PointsBefore && First[K - 1].PointsMonth >= First[K].PointsMonth));
	}
	Check(ByPoints, "league: a fresh world's Headliners are billed on Ignite points alone");
}

/**
 * S-6' (durable/update3/S-6p.md; world_test long): RunItTwice's members' monthly volumes, Nov to Mar (the true counts, the
 * season record's difference across each 6 AM month turn; December's ends at 23:30 on the 31st before January's new season,
 * January's starts at it), how often a player's 0.5-2x band holds 19 at each division's tiers (else the group-pace rule),
 * the pace then, and MonthlyVolume's estimate on the 1st against the month just played.
 */
void S6Prime(uint32_t Seed)
{
	const net::SiteId Rit = net::SiteId::RunItTwice;
	world::World W;
	W.Create(Seed, NightOne);
	auto Snap = [&]() {
		std::map<int, int> Out;
		for (const world::Npc& N : W.People())
		{
			if (const world::SiteRecord* R = W.SiteRecordOf(Rit, N.Id))
			{
				Out[N.Id] = R->Events;
			}
		}
		return Out;
	};
	auto Members = [&]() {
		std::vector<int> Out;
		for (const world::Npc& N : W.People())
		{
			if (N.Playing() && (N.Sites & Bit(Rit)) != 0)
			{
				Out.push_back(N.Id);
			}
		}
		return Out;
	};
	struct Span
	{
		const char* Name;
		int From, To;          // days
		double FromMin, ToMin; // minutes
		bool Reset;
	};
	const int Nov1 = world::DayOn(2026, 11, 1);
	const int Dec1 = world::DayOn(2026, 12, 1);
	const int Jan1 = world::DayOn(2027, 1, 1);
	const int Feb1 = world::DayOn(2027, 2, 1);
	const int Mar1 = world::DayOn(2027, 3, 1);
	const int Apr1 = world::DayOn(2027, 4, 1);
	const Span Months[] = {{"Nov", Nov1, Dec1, Nov1 * Day + 360.0, Dec1 * Day + 360.0, false}, {"Dec", Dec1, Jan1, Dec1 * Day + 360.0, (Jan1 - 1) * Day + 1410.0, false},
		{"Jan", Jan1, Feb1, Jan1 * Day + 360.0, Feb1 * Day + 360.0, true}, {"Feb", Feb1, Mar1, Feb1 * Day + 360.0, Mar1 * Day + 360.0, false},
		{"Mar", Mar1, Apr1, Mar1 * Day + 360.0, Apr1 * Day + 360.0, false}};
	struct Division
	{
		const char* Name;
		int Lo, Hi;
	};
	const Division Divs[] = {{"Open", 1, 1}, {"Iron", 1, 2}, {"Steel", 2, 2}, {"Gold", 2, 3}, {"High Bridge", 3, 4}};
	const int Heroes[] = {15, 30, 60, 120};
	int Pace = 0;
	int Matched = 0;
	int Ten = 0; // cells with at least 10 in the band (core-siteplay's RitDraw falls to the pace under 10 matched)
	int Tables = 0;
	std::map<int, int> Last; // npc -> last month's true volume
	std::printf("  S-6': RunItTwice's members' monthly volumes (entries in the month, the true counts; tiers 1 micro .. 4 high)\n");
	for (const Span& M : Months)
	{
		W.AdvanceTo(M.FromMin);
		// On the 1st (6 AM), the draw's view: MonthlyVolume against the month just played, and the bands.
		const std::vector<int> Now = Members();
		if (!Last.empty())
		{
			std::vector<double> Err;
			int Inside = 0;
			int Judged = 0;
			int Agree = 0;
			int Pairs = 0;
			for (const int Id : Now)
			{
				const auto It = Last.find(Id);
				const double Est = world::MonthlyVolume(W, Rit, Id, W.Clock());
				if (It == Last.end() || Est < 0.0)
				{
					continue;
				}
				if (It->second >= 4)
				{
					Err.push_back(std::fabs(Est / It->second - 1.0));
					Inside += Est >= 0.5 * It->second && Est <= 2.0 * It->second ? 1 : 0;
					++Judged;
				}
				for (const int H : {30, 60})
				{
					Agree += (It->second >= 0.5 * H && It->second <= 2.0 * H) == (Est >= 0.5 * H && Est <= 2.0 * H) ? 1 : 0;
					++Pairs;
				}
			}
			std::sort(Err.begin(), Err.end());
			std::printf("  S-6': %s 1st, MonthlyVolume vs last month (>= 4 entries, %d people): median |error| %.0f%%, p90 %.0f%%, inside 0.5-2x %.0f%%; the 30 and 60 bands agree %.0f%%\n", M.Name,
				Judged, Err.empty() ? 0.0 : 100.0 * Err[Err.size() / 2], Err.empty() ? 0.0 : 100.0 * Err[Err.size() * 9 / 10], Judged ? 100.0 * Inside / Judged : 0.0,
				Pairs ? 100.0 * Agree / Pairs : 0.0);
			for (const Division& D : Divs)
			{
				std::string Row;
				for (const int H : Heroes)
				{
					std::vector<std::pair<double, int>> Near;
					int Band = 0;
					int Eligible = 0;
					for (const int Id : Now)
					{
						const int T = W.Get(Id)->Tier;
						if (T < D.Lo || T > D.Hi)
						{
							continue;
						}
						++Eligible;
						const auto It = Last.find(Id);
						const double V = It == Last.end() ? 0.0 : It->second;
						Band += V >= 0.5 * H && V <= 2.0 * H ? 1 : 0;
						Near.push_back({V > 0.0 ? std::fabs(std::log(V / H)) : 99.0, Id});
					}
					std::sort(Near.begin(), Near.end());
					std::vector<double> PerDay;
					for (size_t K = 0; K < Near.size() && K < 19; ++K)
					{
						const auto It = Last.find(Near[K].second);
						const double V = It == Last.end() ? 0.0 : It->second;
						PerDay.push_back(V >= 30.0 ? V / 30.0 : 1.0);
					}
					std::sort(PerDay.begin(), PerDay.end());
					const int GroupPace = PerDay.empty() ? 1 : std::max(1, std::min(3, static_cast<int>(std::lround(PerDay[PerDay.size() / 2]))));
					const bool Enough = Band >= 19;
					++Tables;
					Matched += Enough ? 1 : 0;
					Ten += Band >= 10 ? 1 : 0;
					Pace += Enough ? 0 : 1;
					Row += "  h" + std::to_string(H) + ": " + std::to_string(Band) + "/" + std::to_string(Eligible) + (Enough ? " ok" : " pace " + std::to_string(GroupPace));
				}
				std::printf("  S-6':   %-11s%s\n", D.Name, Row.c_str());
			}
		}
		const std::map<int, int> S0 = M.Reset ? std::map<int, int>() : Snap();
		W.AdvanceTo(M.ToMin);
		const std::map<int, int> S1 = Snap();
		std::vector<int> All;
		std::map<int, std::vector<int>> ByTier;
		int Zero = 0;
		Last.clear();
		for (const int Id : Members())
		{
			const auto A = S1.find(Id);
			const auto B = S0.find(Id);
			const int V = (A == S1.end() ? 0 : A->second) - (B == S0.end() ? 0 : B->second);
			Last[Id] = V;
			All.push_back(V);
			ByTier[W.Get(Id)->Tier].push_back(V);
			Zero += V == 0 ? 1 : 0;
		}
		std::sort(All.begin(), All.end());
		auto Pct = [](const std::vector<int>& V, double Q) { return V.empty() ? 0 : V[std::min(V.size() - 1, static_cast<size_t>(Q * static_cast<double>(V.size())))]; };
		std::string Tiers;
		for (auto& It : ByTier)
		{
			std::sort(It.second.begin(), It.second.end());
			Tiers += "  t" + std::to_string(It.first) + " " + std::to_string(It.second.size()) + " p50 " + std::to_string(Pct(It.second, 0.5)) + " p90 " + std::to_string(Pct(It.second, 0.9));
		}
		std::printf("  S-6': %s (%.1f days): %zu members, %d with none; p10 %d, p25 %d, p50 %d, p75 %d, p90 %d, max %d;%s\n", M.Name, (M.ToMin - M.FromMin) / Day, All.size(), Zero,
			Pct(All, 0.1), Pct(All, 0.25), Pct(All, 0.5), Pct(All, 0.75), Pct(All, 0.9), All.empty() ? 0 : All.back(), Tiers.c_str());
	}
	std::printf("  S-6': on the 1sts above, %d of %d (division, volume) cells hold 19 in the 0.5-2x band (%d fall to the group-pace rule); %d hold at least 10\n", Matched, Tables,
		Pace, Ten);
}

// ------------------------------------------------------------------ G2 review (review-core-world): the League's oracle

std::vector<std::string> SplitOn(const std::string& S, char By)
{
	std::vector<std::string> Out(1);
	for (const char C : S)
	{
		if (C == By)
		{
			Out.emplace_back();
		}
		else
		{
			Out.back() += C;
		}
	}
	return Out;
}

/** A RunItTwice event in the world's queue, from its q line (World::Write's): what LeagueOracle needs of it. */
struct LeagueQueued
{
	std::string Id;
	int Template = -1;
	double End = 0.0;
	int Entries = 0;
	long long BuyIn = 0;
	int TableSize = 8;
	int FinalSize = 9;
	std::string Series;
	std::vector<std::pair<int, bool>> Who; // npc, on a ticket (a seat won in a satellite)
};

/** The queued RunItTwice events ending in [From, To), in queue order (the order the world resolves equal ends in). */
std::vector<LeagueQueued> LeagueQueue(const std::string& T, double From, double To)
{
	std::vector<LeagueQueued> Out;
	for (const std::string& L : Lines(T))
	{
		if (L.compare(0, 8, "world\tq\t") != 0)
		{
			continue;
		}
		const std::vector<std::string> F = Tabs(L);
		if (F.size() < 30 || F[4] != "1" || F[2].compare(0, 11, "runittwice.") != 0)
		{
			continue;
		}
		LeagueQueued Q;
		Q.Id = F[2];
		Q.Template = std::atoi(F[5].c_str());
		Q.End = std::atof(F[8].c_str());
		Q.Entries = std::atoi(F[9].c_str());
		Q.BuyIn = std::atoll(F[11].c_str());
		Q.TableSize = std::atoi(F[14].c_str());
		Q.Series = F[18];
		Q.FinalSize = std::atoi(F[28].c_str());
		if (Q.End < From || Q.End >= To)
		{
			continue;
		}
		for (const std::string& R : SplitOn(F[29], '|'))
		{
			const std::vector<std::string> V = SplitOn(R, '~');
			if (V.size() >= 2 && !V[0].empty())
			{
				Q.Who.push_back({std::atoi(V[0].c_str()), V[1] == "1"});
			}
		}
		Out.push_back(Q);
	}
	return Out;
}

/** What the rule says a member's month holds, worked out from the world's records. */
struct LeagueTruth
{
	int TodayKey = -1;
	int TodayCount = 0;
	int Days = 0;
	int Form = 0;
	std::map<int, int> Best; // day of the month -> tenths
	bool Unsure = false;     // two of their entries ended together, or a result no queue line showed: nothing to split
	bool FormUnsure = false; // a place the records can't put on either side of the table's size
};

/**
 * Whether a result was a final table as the League's form reads it (the event's last table: its TableSize; more than 3
 * entries), from the record's change across its end: 1 yes, 0 no, -1 the record can't tell. A win is one; with points, the
 * places net::Points gives those points for; with none, the places past the paid ones inside (or past) the record's own
 * final table (P.FinalSize).
 */
int LeagueFt(double Raw, bool Won, bool RecordFt, int Field, long long BuyIn, int TableSize, int FinalSize)
{
	if (Field <= 3)
	{
		return 0;
	}
	if (Won)
	{
		return 1;
	}
	const int Paid = std::max(1, static_cast<int>(std::lround(static_cast<double>(Field) * 0.15)));
	bool In = false;
	bool Out = false;
	if (Raw > 0.0)
	{
		for (int P = 2; P <= Paid; ++P)
		{
			if (std::fabs(net::Points(P, Field, BuyIn) - Raw) < 1e-9)
			{
				(P <= TableSize ? In : Out) = true;
			}
		}
	}
	else
	{
		const int From = RecordFt ? Paid + 1 : std::max(Paid + 1, FinalSize + 1);
		const int To = RecordFt ? std::min(FinalSize, Field) : Field;
		for (int P = From; P <= To && !(In && Out); ++P)
		{
			(P <= TableSize ? In : Out) = true;
		}
	}
	return In && !Out ? 1 : Out && !In ? 0 : -1;
}

/**
 * The League's world side against the rule worked out a second way, from the world's own records rather than the code that
 * keeps the book: all 19 members, every day of a month and the 1st after it. Each member's raw points and prize come from
 * their RunItTwice record across each event's end; the buy-in, template, series, table size and tickets from the event's
 * queue line; then the rule (§3.6 R2): the 6 AM day the result came in, the division's floor through LeagueScore, the
 * first 3 a day (or the pace), the day's best, the best 8, the days, the form, Deadline Day, the month. Two forced entries
 * run the rare paths: a member's ticket entry (in the record, but no League entry: W4) and a member 9th at an 8-max table
 * that pays more than 8 (a cash on the form, not a final table). The month played in one jump, and saved and loaded every
 * few days at odd minutes, ends with the same line (§1.9: a skip is the sum of its steps).
 */
void LeagueOracle(const std::string& DecemberText, int Div, int Pace)
{
	namespace play = ss::site::play;
	const net::SiteId Rit = net::SiteId::RunItTwice;
	std::string Base;
	int TicketNpc = -1;
	int NinthNpc = -1;
	std::string TicketEvent;
	std::string NinthEvent;
	std::vector<int> Roster;
	double Start = 0.0;
	{
		// The forced entries, on Dec 1 and 2: a $11-or-more event each, the first two members.
		world::World W;
		Load(W, DecemberText);
		Start = W.Clock();
		Roster = LeaguePeople(W, Div == 0 ? 1 : 2);
		Roster.resize(std::min<size_t>(19, Roster.size()));
		for (const LeagueQueued& E : LeagueQueue(Text(W), Start, Start + 2.0 * Day))
		{
			const int Field = std::max(E.Entries, static_cast<int>(E.Who.size()) + 1);
			if (E.BuyIn < 1100 || !E.Series.empty() || Roster.size() < 2)
			{
				continue;
			}
			if (TicketNpc < 0)
			{
				TicketNpc = Roster[0];
				TicketEvent = E.Id;
				W.KnownPlaces(E.Id, {{TicketNpc, 1}});
			}
			else if (NinthNpc < 0 && E.TableSize == 8 && std::lround(Field * 0.15) >= 12)
			{
				NinthNpc = Roster[1];
				NinthEvent = E.Id;
				W.KnownPlaces(E.Id, {{NinthNpc, 9}});
			}
		}
		// The ticket: the forced entry's record in its event's q line says the seat was won in a satellite.
		Base = Text(W);
		const size_t At = TicketNpc >= 0 ? Base.find("world\tq\t" + TicketEvent + "\t") : std::string::npos;
		const size_t Eol = At == std::string::npos ? At : Base.find('\n', At);
		bool Marked = false;
		if (Eol != std::string::npos)
		{
			const std::string Line = Base.substr(At, Eol - At);
			const size_t Tab = Line.rfind('\t');
			std::string Joined;
			const std::vector<std::string> Recs = SplitOn(Line.substr(Tab + 1), '|');
			for (size_t R = 0; R < Recs.size(); ++R)
			{
				std::vector<std::string> V = SplitOn(Recs[R], '~');
				if (V.size() >= 2 && std::atoi(V[0].c_str()) == TicketNpc)
				{
					V[1] = "1";
					Marked = true;
				}
				Joined += R ? "|" : "";
				for (size_t K = 0; K < V.size(); ++K)
				{
					Joined += (K ? "~" : "") + V[K];
				}
			}
			Base = Base.substr(0, At) + Line.substr(0, Tab + 1) + Joined + Base.substr(Eol);
		}
		Check(Marked && NinthNpc >= 0, "league oracle: the forced ticket entry and 9th place are set up (" + std::to_string(TicketNpc) + ", " + std::to_string(NinthNpc) + ")");
	}
	const int Month = world::LeagueMonth(Start);
	const int Packed = world::LeaguePack(Div, Pace);
	const int First = world::LeagueFirst(Packed);
	world::World W;
	Load(W, Base);
	W.SetLeagueRoster(Rit, Month, Packed, Roster);
	world::World Jump;
	Load(Jump, Base);
	Jump.SetLeagueRoster(Rit, Month, Packed, Roster);
	std::string Hopped = Text(Jump);
	const std::set<int> Group(Roster.begin(), Roster.end());
	std::map<int, LeagueTruth> Truth;
	std::map<int, world::SiteRecord> Last;
	auto Snap = [&](std::map<int, world::SiteRecord>& Out) {
		for (const int Id : Roster)
		{
			const world::SiteRecord* R = W.SiteRecordOf(Rit, Id);
			Out[Id] = R ? *R : world::SiteRecord();
		}
	};
	Snap(Last);
	int Counted = 0;
	int Tickets = 0;
	int Under = 0;
	int Later = 0;
	int Past = 0;
	int Ninth = -1;
	int TicketSeen = 0;
	bool Agree = true;
	std::string Why;
	for (int D = 0; D < 32; ++D) // December, then the 1st's day (January's)
	{
		const double From = Start + static_cast<double>(D) * Day;
		const double To = From + Day;
		std::vector<LeagueQueued> Q = LeagueQueue(Text(W), From, To);
		std::stable_sort(Q.begin(), Q.end(), [](const LeagueQueued& A, const LeagueQueued& B) { return A.End < B.End; });
		for (size_t K = 0; K < Q.size();)
		{
			// The events ending together (within a second), across their end.
			size_t J = K;
			std::map<int, int> Count;
			bool Any = false;
			for (; J < Q.size() && Q[J].End < Q[K].End + 0.02; ++J)
			{
				for (const std::pair<int, bool>& E : Q[J].Who)
				{
					const bool In = Group.count(E.first) != 0;
					Count[E.first] += In ? 1 : 0;
					Any = Any || In;
				}
			}
			if (!Any)
			{
				K = J;
				continue;
			}
			W.AdvanceTo(Q[K].End - 0.01);
			std::map<int, world::SiteRecord> Before;
			Snap(Before);
			for (const int Id : Roster)
			{
				// (a result no queue line showed; January 1's new season zeroes the counts, which is no result)
				Truth[Id].Unsure = Truth[Id].Unsure || Before[Id].Events > Last[Id].Events;
			}
			W.AdvanceTo(Q[J - 1].End + 0.01);
			std::map<int, world::SiteRecord> After;
			Snap(After);
			for (size_t I = K; I < J; ++I)
			{
				const LeagueQueued& E = Q[I];
				const int Field = std::max(E.Entries, static_cast<int>(E.Who.size()) + 1);
				const std::vector<net::EventTemplate>& Temps = net::ForEvent(E.Id).Templates();
				const std::string TemplateId = E.Template >= 0 && static_cast<size_t>(E.Template) < Temps.size() ? Temps[static_cast<size_t>(E.Template)].Id : std::string();
				for (const std::pair<int, bool>& Who : E.Who)
				{
					if (Group.count(Who.first) == 0)
					{
						continue;
					}
					LeagueTruth& T = Truth[Who.first];
					const world::SiteRecord& A = After[Who.first];
					const world::SiteRecord& B = Before[Who.first];
					if (Count[Who.first] != 1 || A.Events != B.Events + 1)
					{
						T.Unsure = true;
						continue;
					}
					const double Raw = A.Points - B.Points;
					const long long Prize = A.Won - B.Won;
					const int Ft = LeagueFt(Raw, A.Wins > B.Wins, A.FinalTables > B.FinalTables, Field, E.BuyIn, E.TableSize, E.FinalSize);
					Ninth = Who.first == NinthNpc && E.Id == NinthEvent ? Ft : Ninth;
					if (Who.second)
					{
						++Tickets;
						TicketSeen += Who.first == TicketNpc && E.Id == TicketEvent ? 1 : 0;
						continue;
					}
					if (E.BuyIn <= 0)
					{
						continue;
					}
					const int Played = world::LeagueDay(E.End);
					if (world::MonthKeyOf(Played) != Month)
					{
						++Past;
						continue;
					}
					if (play::LeagueScore(Div, TemplateId, E.BuyIn, Played, 1.0, false, false) <= 0.0)
					{
						++Under;
						continue;
					}
					if (T.TodayKey != Played)
					{
						T.TodayKey = Played;
						T.TodayCount = 0;
					}
					if (T.TodayCount >= First)
					{
						++Later;
						continue;
					}
					++Counted;
					const bool FirstToday = T.TodayCount == 0;
					++T.TodayCount;
					T.FormUnsure = T.FormUnsure || Ft < 0;
					const int Outcome = Ft > 0 ? 3 : Prize > 0 ? 2 : 1;
					if (FirstToday)
					{
						T.Days = std::min(31, T.Days + 1);
						T.Form = ((T.Form << 2) | Outcome) & 0x3ff;
					}
					else if ((T.Form & 3) < Outcome)
					{
						T.Form = (T.Form & ~3) | Outcome;
					}
					const bool Deadline = world::MonthKeyOf(Played + 1) != Month;
					const double Score = play::LeagueScore(Div, TemplateId, E.BuyIn, Played, Raw, !E.Series.empty(), Deadline);
					const int Tenths = static_cast<int>(std::min<long long>(32767, std::max<long long>(0, std::llround(Score * 10.0))));
					if (Tenths > 0)
					{
						int Yy = 0;
						int Mm = 0;
						int Dd = 0;
						world::CivilDate(Played, Yy, Mm, Dd);
						int& Held = T.Best[Dd];
						Held = std::max(Held, Tenths);
					}
				}
			}
			Last = After;
			K = J;
		}
		W.AdvanceTo(To);
		Snap(Last);
		// The book against the rule, member by member, at each day's end.
		const world::LeagueBook* Book = W.League(Rit);
		for (const int Id : Roster)
		{
			const LeagueTruth& T = Truth[Id];
			const world::LeagueMember* M = Book ? world::LeagueMemberOf(*Book, Id) : nullptr;
			if (!M || T.Unsure)
			{
				Agree = Agree && M;
				continue;
			}
			std::vector<int> Want;
			for (const auto& It : T.Best)
			{
				Want.push_back(It.second);
			}
			std::sort(Want.rbegin(), Want.rend());
			Want.resize(std::min<size_t>(8, Want.size()));
			std::vector<int> Got;
			bool Days = true;
			for (const std::pair<int8_t, int16_t>& S : M->Best)
			{
				if (S.second > 0)
				{
					Got.push_back(S.second);
					// (each held day scored what it holds; a tie may keep either day)
					const auto It = T.Best.find(S.first);
					Days = Days && It != T.Best.end() && It->second == S.second;
				}
			}
			const bool Same = Got == Want && Days && M->Days == T.Days && M->TodayKey == T.TodayKey && M->TodayCount == T.TodayCount && (T.FormUnsure || M->Form == T.Form);
			if (!Same && Agree)
			{
				Why = "member " + std::to_string(Id) + " after day " + std::to_string(D + 1) + ": days " + std::to_string(M->Days) + "/" + std::to_string(T.Days) + ", today " +
					std::to_string(M->TodayKey) + ":" + std::to_string(M->TodayCount) + "/" + std::to_string(T.TodayKey) + ":" + std::to_string(T.TodayCount) + ", form " +
					std::to_string(M->Form) + "/" + std::to_string(T.Form) + ", best " + std::to_string(Got.size()) + "/" + std::to_string(Want.size());
			}
			Agree = Agree && Same;
		}
		if (D % 4 == 1)
		{
			world::World H;
			Load(H, Hopped);
			H.AdvanceTo(From + 457.0 + static_cast<double>(D));
			Hopped = Text(H);
		}
	}
	int Unsure = 0;
	int FormUnsure = 0;
	int Full = 0;
	for (const auto& It : Truth)
	{
		Unsure += It.second.Unsure ? 1 : 0;
		FormUnsure += It.second.FormUnsure ? 1 : 0;
		Full += It.second.Best.size() > 8 ? 1 : 0;
	}
	std::printf("  league oracle (%s, pace %d): %d entries counted, %d past the day's first %d, %d under the floor, %d on a ticket, %d after the 1st's turn; %d of %zu "
				"members with more than 8 days, %d the records can't split, %d with a form they can't pin\n",
		Div == 0 ? "Open" : "Steel", Pace, Counted, Later, First, Under, Tickets, Past, Full, Roster.size(), Unsure, FormUnsure);
	Check(Agree, "league oracle: every member's book is the rule worked out from the world's own records, at every day's end (" + Why + ")");
	Check(Unsure <= 2 && Counted > 100 && Past > 0 && (Div != 0 || Full > 0) && (Div == 0 || Under > 0) && (Pace != 1 || Later > 0),
		"league oracle: the month ran the rule's paths (counted, under the floor, past the day's first, past 8 days, the 1st's turn)");
	Check(TicketSeen == 1, "league oracle: a member's ticket entry is in their record but is no League entry (W4)");
	Check(Ninth == 0, "league oracle: 9th at an 8-max table that pays more than 8 is a cash on the form, not a final table (" + std::to_string(Ninth) + ")");
	Jump.AdvanceTo(W.Clock());
	{
		world::World H;
		Load(H, Hopped);
		H.AdvanceTo(W.Clock());
		Hopped = Text(H);
	}
	const std::string Line = LeagueLines(Text(W), true);
	Check(!Line.empty() && LeagueLines(Text(Jump), true) == Line && LeagueLines(Hopped, true) == Line,
		"league oracle: the month in one jump, and saved and loaded every few days at odd minutes, ends with the same League line");
}

/**
 * Ignite's Headliners hold for the month (F8: "each month the six ..."): through January, where the season's points start
 * again at nothing, the six change only when someone's audience moves or they stop playing, never with a night's results.
 * W is LeagueQueries' world at January 1, 6 AM.
 */
void LeagueHeadlinersHold(world::World& W)
{
	const int Jan1 = world::DayOn(2027, 1, 1);
	std::vector<world::Headliner> Was;
	std::map<int, int> FollowersWas;
	std::set<int> PlayingWas;
	int Changed = 0;
	int Unexplained = 0;
	bool Stable = true;
	std::map<int, std::pair<double, ss::Chips>> Keys;
	for (int D = 0; D < 31; ++D)
	{
		W.AdvanceTo(static_cast<double>(Jan1 + D) * Day + 720.0);
		const std::vector<world::Headliner> All = world::Headliners(W, W.Clock(), 100000);
		std::vector<world::Headliner> Six(All.begin(), All.begin() + static_cast<std::ptrdiff_t>(std::min<size_t>(6, All.size())));
		std::map<int, int> Followers;
		std::set<int> Playing;
		for (const world::Headliner& H : All)
		{
			Followers[H.Npc] = H.Followers;
			Playing.insert(H.Npc);
			// The month's keys stay put: the points before the month and the money before the season.
			const auto It = Keys.find(H.Npc);
			Stable = Stable && (It == Keys.end() || (It->second.first == H.PointsBefore && It->second.second == H.WonBefore));
			Keys[H.Npc] = {H.PointsBefore, H.WonBefore};
		}
		bool Same = Six.size() == Was.size();
		for (size_t K = 0; Same && K < Six.size(); ++K)
		{
			Same = Six[K].Npc == Was[K].Npc;
		}
		if (D > 0 && !Same)
		{
			++Changed;
			// Explained: someone in either six has a different audience than yesterday, or wasn't billed one of the days.
			bool Moved = false;
			for (const std::vector<world::Headliner>* List : {&Six, &Was})
			{
				for (const world::Headliner& H : *List)
				{
					const auto F = FollowersWas.find(H.Npc);
					Moved = Moved || PlayingWas.count(H.Npc) == 0 || Playing.count(H.Npc) == 0 || F == FollowersWas.end() || F->second != Followers[H.Npc];
				}
			}
			Unexplained += Moved ? 0 : 1;
		}
		Was = Six;
		FollowersWas = Followers;
		PlayingWas = Playing;
	}
	std::printf("  league: Ignite's Headliners through January changed on %d of 30 days (%d not by an audience or a break)\n", Changed, Unexplained);
	Check(Stable && Unexplained == 0, "league: January's Headliners hold for the month: their points before it and money before the season stay put, and the six move only with an audience or a break");
}

/** A session's saves, kept (the last one). */
struct LeagueSaveHooks : ss::SessionHooks
{
	ss::SaveData Last;
	void Save(const ss::SaveData& D) override
	{
		Last = D;
	}
};

/** The save's League line's month (-1: none). */
int LeagueLineMonth(const std::string& Text)
{
	const size_t At = Text.find("world\tleague\t");
	const std::vector<std::string> F = At == std::string::npos ? std::vector<std::string>() : Tabs(Text.substr(At, Text.find('\n', At) - At));
	return F.size() > 3 ? std::atoi(F[3].c_str()) : -1;
}

/**
 * The 1st's 6 AM step closes the month and hands the world the new group; the player saves within the hour and loads.
 * The session keeps its last world text for up to an hour while nothing of the player's changes in the world (HeroRevision),
 * so SetLeagueRoster must count as such a change: else that save holds last month's book, and the loaded world never
 * records the new month's group (every member at 0 points all month).
 */
void LeagueSaveAfterDraw()
{
	LeagueSaveHooks H;
	ss::Session S(H, "league-save-after-draw");
	double Now = 0.0;
	auto Jump = [&](double ToWorld) {
		if (ToWorld > S.WorldMinutes())
		{
			S.LobbyMinutes += ToWorld - S.WorldMinutes();
		}
		Now += 1.0 / 30.0;
		S.Update(Now);
	};
	const int Dec1 = world::DayOn(2026, 12, 1);
	Jump(static_cast<double>(world::DayOn(2026, 11, 20)) * Day + 18.0 * 60.0);
	ss::site::Join(S.Life.Sites, net::SiteId::RunItTwice, S.WorldMinutes());
	// The book as the first paid RunItTwice entry leaves it (a Placement month, its group drawn at the next step).
	S.Life.SitePlay.Rt.L.Month = -2;
	S.Life.SitePlay.Rt.L.Placement = true;
	S.Life.Update3LinesRead |= ss::life::U3SitePlay;
	Jump(S.WorldMinutes() + 1.0);
	const world::LeagueBook* November = S.Living().League(net::SiteId::RunItTwice);
	const int NovemberKey = November ? November->MonthKey : -1;
	// Dec 1 at 5:30 AM a save writes the world; at 6:05 the step has closed November and drawn December; a save at once.
	Jump(static_cast<double>(Dec1) * Day + 330.0);
	S.Save();
	const int Before = LeagueLineMonth(H.Last.Serialize());
	Jump(static_cast<double>(Dec1) * Day + 365.0);
	const world::LeagueBook* December = S.Living().League(net::SiteId::RunItTwice);
	S.Save();
	const std::string Text = H.Last.Serialize();
	const int After = LeagueLineMonth(Text);
	ss::SaveData Back;
	int Loaded = -1;
	if (ss::SaveData::Parse(Text, Back))
	{
		LeagueSaveHooks H2;
		ss::Session R(H2, "league-save-after-draw", &Back);
		const world::LeagueBook* Again = R.Living().League(net::SiteId::RunItTwice);
		Loaded = Again && Again->MonthKey == R.Life.SitePlay.Rt.L.Month ? Again->MonthKey : -1;
	}
	std::printf("  league: a save at 6:05 on Dec 1: the play book's month %d, the save's League line %d (the 5:30 save's %d), loaded %d\n", S.Life.SitePlay.Rt.L.Month, After, Before, Loaded);
	Check(NovemberKey == world::MonthKeyOf(Dec1 - 1) && Before == NovemberKey && December && December->MonthKey == S.Life.SitePlay.Rt.L.Month && After == December->MonthKey &&
			  Loaded == December->MonthKey,
		"league: the group the 1st's step hands the world is in the next save's world text, and a load records it (SetLeagueRoster moves HeroRevision)");
}
} // namespace sites_seg

int SegSites()
{
	namespace ss_ = sites_seg;
	const auto T0 = std::chrono::steady_clock::now();
	ss_::Failures = 0;
	std::fflush(stdout);
	std::setvbuf(stdout, nullptr, _IONBF, 0); // (what a crash cuts short still reaches the log)
	if (!ss_::Registered(ss::net::SiteId::Ignite) || !ss_::Registered(ss::net::SiteId::RunItTwice))
	{
		// (Until core-sites registers the rivals, the world has only RiverLine: nothing here applies.)
		std::printf("sites: Ignite and RunItTwice aren't registered; %d checks failed\n", ss_::Failures);
		ss_::Check(false, "sites: Ignite and RunItTwice registered (Sites.cpp)");
		return ss_::Failures;
	}
	ss::world::World W;
	W.Create(7u, ss_::NightOne);
	ss::world::World Again;
	Again.Create(7u, ss_::NightOne);
	ss::world::World Other;
	Other.Create(8u, ss_::NightOne);
	ss::world::World Fresh;
	Fresh.Create(7u, ss_::NightOne, ss::world::Start::Blank);
	ss_::Memberships(W, Again, Other, Fresh);
	ss_::FirstNight(W, Fresh);
	ss_::Guards(W);
	ss_::Copy(W);
	ss_::Season(7u);
	ss_::FreshTeams(7u);
	ss_::RivalStaysHome();
	ss_::Hero();
	ss_::LaunchCommand();
	ss_::Migration();
	// Update 3 G2 (core-world): the League's world side and what core-siteplay reads from the world.
	{
		const auto TL = std::chrono::steady_clock::now();
		std::string December;
		ss::world::World Base;
		ss_::LeagueRules();
		ss_::LeagueScoring(Base, 7u, December);
		ss_::LeagueQueries(Base, 7u);
		ss_::LeagueHeadlinersHold(Base);
		ss_::LeagueSaveAfterDraw();
		// The G2 review's oracle: the book against the rule worked out from the world's records (Open at the first 3, whose
		// micro fields run an event past the 6 AM turn; the long run adds Steel, with its floor, at the first 3 and a pace of 1).
		ss_::LeagueOracle(December, 0, 0);
		if (LongRun)
		{
			ss_::LeagueOracle(December, 2, 0);
			ss_::LeagueOracle(December, 2, 1);
		}
		const char* Pairs = std::getenv("SS_S6P_PAIRS"); // S-6p.md's measurement: more pairs
		ss_::LeagueBudget(December, Pairs ? std::max(1, std::atoi(Pairs)) : LongRun ? 5 : 1);
		if (LongRun || std::getenv("SS_S6P"))
		{
			ss_::S6Prime(7u);
		}
		std::printf("  league: %.1f s\n", ss_::Seconds(TL));
	}
	if (LongRun)
	{
		ss_::Season(1234u);
		ss_::FreshTeams(98765u);
	}
	std::printf("sites: %d checks failed (%.1f s)\n", ss_::Failures, ss_::Seconds(T0));
	return ss_::Failures;
}
} // namespace world_test
