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
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Rng.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
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
	if (LongRun)
	{
		ss_::Season(1234u);
		ss_::FreshTeams(98765u);
	}
	std::printf("sites: %d checks failed (%.1f s)\n", ss_::Failures, ss_::Seconds(T0));
	return ss_::Failures;
}
} // namespace world_test
