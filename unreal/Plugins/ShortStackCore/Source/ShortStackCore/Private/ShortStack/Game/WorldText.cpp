// The living world in words, for the debug tools: one person inside out, and the world's health at a glance.
#include "ShortStack/Game/World.h"

#include "ShortStack/Game/Badges.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Sites.h"
#include "WorldSim.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <sstream>

namespace ss
{
namespace world
{
namespace worldtext_detail
{
std::string Pct(double V)
{
	return Fixed(V * 100.0, 1) + "%";
}

const char* VenueName(int V)
{
	return V == 0 ? "Online" : V == 1 ? "Live" : "Underground";
}

const char* LiveName(LiveLevel L)
{
	switch (L)
	{
	case LiveLevel::None: return "none";
	case LiveLevel::Local: return "local";
	case LiveLevel::Regional: return "regional";
	case LiveLevel::Circuit: return "circuit";
	case LiveLevel::Championship: return "championship";
	case LiveLevel::HighRoller: return "high roller";
	}
	return "";
}

const char* OriginName(Origin O)
{
	switch (O)
	{
	case Origin::Founding: return "network regular";
	case Origin::Cast: return "Embercrest / Back Room";
	case Origin::Directory: return "Kast streamer";
	case Origin::Rookie: return "newcomer";
	case Origin::Discovered: return "discovered";
	case Origin::Local: return "Embercrest regular";
	}
	return "";
}

std::string Short(Chips C)
{
	return net::MoneyShort(C);
}

std::string StyleOf(const Npc& N)
{
	const float Aggro = N.TraitOf(Trait::Aggro);
	const float Patience = N.TraitOf(Trait::Patience);
	if (Aggro > 0.8f && N.TraitOf(Trait::Risk) > 0.75f)
	{
		return "Wild: raises everything";
	}
	if (Aggro > 0.62f)
	{
		return Patience > 0.55f ? "Tight and aggressive" : "Loose and aggressive";
	}
	if (Aggro < 0.3f)
	{
		return Patience > 0.6f ? "Tight and patient" : "Calls a lot";
	}
	return Patience > 0.6f ? "Solid and patient" : "Balanced";
}

std::string LiveText(const Npc& N)
{
	switch (N.Live)
	{
	case LiveLevel::None: return "Online only";
	case LiveLevel::Local: return "Plays the local weeklies";
	case LiveLevel::Regional: return "Plays regional festivals";
	case LiveLevel::Circuit: return "Travels the Grand Circuit";
	case LiveLevel::Championship: return "Plays the Championship";
	case LiveLevel::HighRoller: return "Plays the high rollers";
	}
	return "";
}

double Percentile(std::vector<double> V, double P)
{
	if (V.empty())
	{
		return 0.0;
	}
	std::sort(V.begin(), V.end());
	const size_t K = static_cast<size_t>(sim::Clamp(P, 0.0, 1.0) * static_cast<double>(V.size() - 1));
	return V[K];
}
const char* const Months[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

/** Small numbers as words ("six years"), the rest as digits. */
std::string Count(int N)
{
	static const char* const Words[13] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve"};
	return N >= 0 && N <= 12 ? Words[N] : std::to_string(N);
}

/** "in the $1.10 Night Owl Turbo", "in FROST #12: $55 Iron Gambit PKO". */
std::string In(const std::string& Event)
{
	return Event.find('#') != std::string::npos ? "in " + Event : "in the " + Event;
}

std::string Prize(Chips C)
{
	return C >= 1000000 ? net::MoneyShort(C) : Money(C);
}

/** A site's everyday name (update 2 §5.3): "Ignite" for Ignite Poker; RunItTwice and RiverLine as they are. */
std::string TextSiteShort(net::SiteId S)
{
	std::string Name = net::Def(S).Name;
	const std::string Tail = " Poker";
	if (Name.size() > Tail.size() && Name.compare(Name.size() - Tail.size(), Tail.size(), Tail) == 0)
	{
		Name.resize(Name.size() - Tail.size());
	}
	return Name;
}

/** "Ignite's", "RunItTwice's". */
std::string TextSitePossessive(net::SiteId S)
{
	return TextSiteShort(S) + "'s";
}

/** Whole dollars, as the news writes a guarantee or an overlay: "$3,100". */
std::string TextWholeDollars(Chips C)
{
	return "$" + Grouped(static_cast<int64_t>(std::llround(static_cast<double>(C) / 100.0)));
}

/** How they arrived, in a sentence (the same every time for the same person). */
std::string ArrivalText(const World& W, const Npc& N)
{
	const uint32_t H = Fnv1a(N.Name);
	const int Years = std::max(1, (N.Arrived - N.Joined) / 365);
	int Y = 0;
	int M = 1;
	int D = 1;
	CivilDate(N.Arrived, Y, M, D);
	const Npc* With = W.Get(N.CameWith);
	switch (N.Came)
	{
	case Arrival::FirstTimer:
	{
		static const char* const Lines[4] = {"Downloaded RiverLine on a slow night and never uninstalled it.", "Signed up for the freerolls. Stayed for everything else.",
			"A friend's referral link, a ten-dollar deposit, and here they are.", "Watched one hand over a roommate's shoulder. Opened an account that night."};
		if (H % 4 == 0 && !N.Path.empty() && N.Path.front().Kind == StepKind::Joined && N.Path.front().What == net::Def(net::SiteId::Ignite).Name)
		{
			// One of Ignite's own arrivals (update 2 §5.9): they found the game there, not on RiverLine. (Their journey's first
			// step says so for good; where they play now can change: a team's "plays only on" leaves one site's bit alone.)
			return "Downloaded " + net::Def(net::SiteId::Ignite).Name + " on a slow night and never uninstalled it.";
		}
		return Lines[H % 4];
	}
	case Arrival::CameOfAge: return "Turned " + std::to_string(std::max(18, Y - N.Born)) + " in " + Months[M - 1] + " and registered the same night.";
	case Arrival::SiteClosed:
	{
		static const char* const Sites[10] = {"NorthPot", "BluffHarbor", "AceLine", "ChipCity", "Felt Republic", "PocketRocket", "RiverBend Poker", "Saltwater Poker",
			"CardRoom 88", "Tiltproof"};
		return std::string("Came over when ") + Sites[H % 10] + " closed its doors, after " + Count(Years) + (Years == 1 ? " year" : " years") + " grinding there.";
	}
	case Arrival::LiveCrossover:
	{
		static const char* const Cities[3][4] = {{"Austin", "Toronto", "Atlantic City", "Reno"}, {"London", "Berlin", "Lyon", "Dublin"}, {"Sydney", "Seoul", "Manila", "Auckland"}};
		const int R = std::max(0, std::min(2, static_cast<int>(N.Home)));
		return std::string("A regular in the ") + Cities[R][H % 4] + " card rooms for " + Count(Years) + (Years == 1 ? " year" : " years") + ", now trying the online grind.";
	}
	case Arrival::Comeback: return "First played online in " + std::to_string(YearOf(N.Joined)) + ". Back after " + Count(Years) + " years away.";
	case Arrival::HomeGame:
		return With ? "Talked into it by " + With->Name + " and the rest of their home game." : "Talked into it by the regulars at a home game.";
	case Arrival::Watched:
		if (With && With->Streams && With->Followers >= 2000)
		{
			return "Signed up after a night of watching " + With->Name + " stream on Kast.";
		}
		return With ? "Signed up after watching " + With->Name + " at a televised final table." : "Signed up after a night of watching poker on TV.";
	case Arrival::Streamer: return "Started a Kast channel on day one: learning poker live, one stream at a time.";
	case Arrival::None:
	case Arrival::Count: break;
	}
	return "";
}

/** A step of the journey, in a line. */
std::string StepText(const Step& S)
{
	const std::string Place = S.Place > 0 ? Ordinal(S.Place) : std::string();
	const std::string Of = S.Of > 0 ? " of " + Grouped(S.Of) : std::string();
	const std::string Paid = S.Amount > 0 ? " (" + Prize(S.Amount) + ")" : std::string();
	switch (S.Kind)
	{
	case StepKind::Joined: return S.What.empty() ? std::string("Joined RiverLine.") : "Joined " + S.What + "."; // (Ignite's own arrivals name it)
	case StepKind::FirstEvent: return "First tournament: " + Place + Of + " " + In(S.What) + Paid + ".";
	case StepKind::FirstCash:
		// Out of the money but a head or two collected: a bounty, not a cash.
		return S.Of > 0 && S.Place > std::max(1, S.Of / 5) ? "First money: a " + Prize(S.Amount) + " bounty " + In(S.What) + "."
														  : "First cash: " + Prize(S.Amount) + " for " + Place + " " + In(S.What) + ".";
	case StepKind::FirstFinalTable: return "First final table: " + Place + Of + " " + In(S.What) + Paid + ".";
	case StepKind::FirstWin: return "First title: won " + (S.What.find('#') != std::string::npos ? S.What : "the " + S.What) + Paid + ".";
	case StepKind::FirstLive: return "First live event: " + Place + Of + " " + In(S.What) + Paid + ".";
	case StepKind::FirstSeries: return "First series title: " + S.What + Paid + ".";
	case StepKind::Bracelet: return "Won a Championship bracelet: " + S.What + Paid + ".";
	case StepKind::Ring: return "Won a Grand Circuit ring: " + S.What + Paid + ".";
	case StepKind::Major: return "Won " + (S.What.find('#') != std::string::npos ? S.What : "the " + S.What) + Paid + ".";
	case StepKind::BigScore: return "Career-best score: " + Prize(S.Amount) + " for " + Place + " " + In(S.What) + ".";
	case StepKind::MovedUp: return "Moved up to " + S.What + " stakes.";
	case StepKind::MovedDown: return "Moved back down to " + S.What + " stakes.";
	case StepKind::TurnedPro: return "Quit the day job to play full time.";
	case StepKind::Sponsored: return "Signed with " + S.What + ".";
	case StepKind::StartedStreaming: return "Went live on Kast for the first time.";
	case StepKind::WentBroke: return "Went broke.";
	case StepKind::Break: return "Stepped away from the tables.";
	case StepKind::Returned: return S.What == "broke" ? "Rebuilt a bankroll and came back." : S.What == "retired" ? "Came out of retirement." : "Back from a break.";
	case StepKind::Retired: return "Retired.";
	case StepKind::PlayerOfYear: return "Named " + S.What + " for " + std::to_string(YearOf(S.Day - 1)) + ".";
	case StepKind::HallOfFame: return "Inducted into the Hall of Fame, class of " + std::to_string(S.Place > 0 ? S.Place : YearOf(S.Day)) + ".";
	case StepKind::Count: break;
	}
	return "";
}
} // namespace worldtext_detail

using namespace worldtext_detail;

Profile World::ProfileOf(int Id) const
{
	Profile Pr;
	const Npc* P = Get(Id);
	if (!P)
	{
		return Pr;
	}
	const Npc& N = *P;
	const int Day = sim::DayAt(Now);
	Pr.Id = N.Id;
	Pr.Name = N.Name;
	Pr.Country = N.Country;
	Pr.Age = N.Age(Day);
	Pr.Known = IdentityName(N.Is);
	Pr.Status = N.St == Status::Broke ? "Away from the tables" : StatusName(N.St);
	Pr.Stakes = std::string(net::TierName(static_cast<net::Tier>(sim::TierIndex(N.Tier)))) + " stakes online";
	Pr.Live = LiveText(N);
	Pr.Style = StyleOf(N);
	Pr.Since = YearOf(N.Joined);
	for (int K = 0; K < RepCount; ++K)
	{
		Pr.Reps[static_cast<size_t>(K)] = static_cast<int>(std::lround(static_cast<double>(N.Reps[static_cast<size_t>(K)])));
	}
	Pr.Totals = N.Totals;
	for (Ledger& L : Pr.Totals)
	{
		L.Spent = 0;
	}
	Pr.Bracelets = N.Bracelets;
	Pr.Rings = N.Rings;
	Pr.Awards = N.Awards;
	Pr.Stats = N.Stats;
	Pr.Titles = N.Titles;
	Pr.Majors = N.Majors;
	Pr.BestEvent = N.BestEvent;
	Pr.Best = std::max({N.Totals[0].Best, N.Totals[1].Best, N.Totals[2].Best});
	Pr.BestDay = N.BestDay;
	Pr.Recent = N.Recent;
	Pr.Years = N.Years;
	for (Year& Y : Pr.Years)
	{
		Y.Net = 0;
	}
	Pr.Form = N.Weekly;
	Pr.ThisSeason = N.ThisSeason;
	Pr.Streams = N.Streams;
	Pr.Followers = N.Followers;
	Pr.Sponsor = N.Sponsor;
	Pr.Pro = N.Pro;
	Pr.Rival = N.Rival;
	for (const Tie& T : N.Ties)
	{
		// Who backs whom stays between them.
		if (T.Kind != TieKind::Backer && T.Kind != TieKind::Backed && T.Strength >= 0.2f)
		{
			if (const Npc* O = Get(T.Other))
			{
				Pr.Knows.push_back({TieName(T.Kind), O->Name});
			}
		}
	}
	if (const Bond* B = BondWith(N.Id))
	{
		Pr.Bond = B->Label();
		for (auto It = B->Memories.rbegin(); It != B->Memories.rend(); ++It)
		{
			Pr.Memories.push_back(net::DateLabel(It->Day) + ", " + std::to_string(YearOf(It->Day)) + ": " + MemoryText(It->Kind) + (It->Where.empty() ? "" : " (" + It->Where + ")"));
		}
	}
	for (auto It = Log.rbegin(); It != Log.rend() && Pr.Story.size() < 8; ++It)
	{
		std::string Title;
		std::string Body;
		std::string Tag;
		if ((It->Npc == N.Id || It->Other == N.Id) && Headline(*It, Title, Body, Tag))
		{
			Pr.Story.push_back(net::DateLabel(sim::DayAt(It->At)) + ", " + std::to_string(YearOf(sim::DayAt(It->At))) + ": " + Title);
		}
	}
	// The journey: how they got here, then the steps since.
	if (N.Came != Arrival::None)
	{
		Pr.Came = ArrivalText(*this, N);
		Pr.Arrived = N.Arrived;
		Pr.New = Day - N.Arrived <= 30;
	}
	else
	{
		Profile::Moment Start;
		Start.Day = N.Joined;
		// A fresh start's books opened on its first night (update 2 §3.3 F4): nobody's card claims years it has no record of.
		const bool Fresh = Mode == world::Start::Blank;
		switch (N.From)
		{
		case Origin::Cast: Pr.Came = "One of the faces at the Embercrest and Dee's Tuesday game."; break;
		case Origin::Local:
			Pr.Came = Fresh ? std::string("One of the Embercrest card room's regulars since the first night.") : "A regular in the Embercrest's card room since " + std::to_string(Pr.Since) + ".";
			break;
		case Origin::Directory:
			Pr.Came = Fresh ? std::string("A name on Kast long before RiverLine's record books opened.") : "Streaming poker on Kast since " + std::to_string(Pr.Since) + ".";
			break;
		case Origin::Discovered: Pr.Came = "Nobody was following them until " + (N.BestEvent.empty() ? std::string("one big night") : "the " + N.BestEvent) + "."; break;
		default:
			Pr.Came = Fresh ? std::string("One of RiverLine's regulars since the first night.") : "On the scene since " + std::to_string(Pr.Since) + ": one of RiverLine's regulars.";
			break;
		}
		Start.Text = Fresh ? std::string("On the books since the first night.") : "Playing since " + std::to_string(Pr.Since) + ".";
		Pr.Journey.push_back(Start);
	}
	for (const Step& S : N.Path)
	{
		Profile::Moment Mo;
		Mo.Day = S.Day;
		Mo.Kind = S.Kind;
		Mo.Text = StepText(S);
		Pr.Journey.push_back(std::move(Mo));
	}
	return Pr;
}

bool World::Headline(const WorldEvent& E, std::string& Title, std::string& Body, std::string& Tag) const
{
	// A fresh start's books (update 2 §3.3 F9): every line names a result that happened in this world, and nothing older.
	const bool Fresh = Mode == Start::Blank;
	const std::string Opened = net::DateLabel(Sim::BooksOpened(*this));
	if (E.Kind == EventKind::FirstWinner || E.Kind == EventKind::BoardReport)
	{
		// Someone the world follows, or the player; a stranger's first stays in the books without a headline.
		const Npc* P = Get(E.Npc);
		const std::string Name = P ? P->Name : E.Npc == -1 ? HeroName : std::string();
		if (Name.empty())
		{
			return false;
		}
		if (E.Kind == EventKind::FirstWinner)
		{
			Title = "The first " + E.What + " title in the record books goes to " + Name;
			Body = Grouped(E.Value) + " entries \xC2\xB7 " + Money(E.Amount) + " to the winner. The books opened on " + Opened + ".";
			Tag = "FIRST WINNER";
			return true;
		}
		// The board report: What is "day,rank,climb" (the report's day from the books' first, the riser's rank now and the
		// places climbed; never in Amount, which the news reads as money).
		const size_t Comma = E.What.find(',');
		const size_t Comma2 = Comma == std::string::npos ? std::string::npos : E.What.find(',', Comma + 1);
		const int Since = std::atoi(E.What.c_str());
		const int Rank = Comma == std::string::npos ? 0 : std::atoi(E.What.c_str() + Comma + 1);
		const int Climb = Comma2 == std::string::npos ? 0 : std::atoi(E.What.c_str() + Comma2 + 1);
		const std::string When = Since >= 1 && Since < 7 ? "Night " + Count(Since) : "Week " + Count(std::max(1, (Since + 6) / 7));
		Title = When + " on the books: " + Name + " tops the Season board with " + Grouped(E.Value) + (E.Value == 1 ? " point" : " points");
		const Npc* Riser = Get(E.Other);
		Body = Riser && Climb > 0 && Rank > 0
			? Riser->Name + " climbs " + Grouped(Climb) + (Climb == 1 ? " place" : " places") + " to No. " + Grouped(Rank) + "."
			: "The record books opened on " + Opened + ". Every final table since counts toward Player of the Year.";
		Tag = "BOARD REPORT";
		return true;
	}
	// The rival sites' own news (update 2 §5.9, §5.10): their events carry the site in their flags, so only that site's News
	// shows them (RiverLine's never names a rival).
	const net::SiteId Site = FlagSiteOf(E.Flags);
	if (E.Kind == EventKind::SiteLaunched)
	{
		const net::SiteId Launched = net::SiteOf(E.What + ".");
		const net::SiteDef& Info = net::Def(Launched);
		if (Launched == net::SiteId::RiverLine || Info.Id != Launched)
		{
			return false;
		}
		const bool OnTime = sim::DayAt(E.At) == Info.LaunchDay;
		if (Launched == net::SiteId::RunItTwice)
		{
			Title = Info.Name + " opens its new lobby";
			Body = (Info.Tagline.empty() ? std::string() : Info.Tagline + " ") + "Its points season starts today.";
		}
		else
		{
			Title = Info.Name + " opens its lobby";
			// (Ignite's launch week runs bigger guarantees on its own dates; a world that met it later just sees the doors open.)
			Body = (Info.Tagline.empty() ? std::string() : Info.Tagline + " ") + (OnTime ? "Launch week runs bigger guarantees." : "The lobby's open and the tables are filling.");
		}
		Tag = "LAUNCH";
		return true;
	}
	if (E.Kind == EventKind::SiteOverlay)
	{
		if (Site == net::SiteId::RiverLine || E.Amount <= 0)
		{
			return false;
		}
		Title = TextSitePossessive(Site) + " " + E.What + " posts a " + TextWholeDollars(E.Amount) + " overlay";
		Body = Grouped(E.Value) + " entries didn't cover the guarantee. The site paid the other " + TextWholeDollars(E.Amount) + ".";
		Tag = "OVERLAY";
		return true;
	}
	const Npc* N = Get(E.Npc);
	if (!N)
	{
		return false;
	}
	const Npc* O = Get(E.Other);
	const std::string& Who = N->Name;
	const bool Known = N->RepOf(Rep::Overall) >= 15.0f || N->Anchored || N->Pro || N->Streams;
	const bool Live = (E.Flags & FlagLive) != 0;
	const int Day = sim::DayAt(E.At);
	switch (E.Kind)
	{
	case EventKind::Won:
	case EventKind::Champion:
	{
		const bool Qualifier = (E.Flags & FlagQualifier) != 0;
		Title = Who + " wins " + E.What;
		Body = Grouped(E.Value) + " entries \xC2\xB7 " + Money(E.Amount) + " to the winner.";
		if (Qualifier)
		{
			Body = "From a satellite seat to the title: " + Body;
		}
		if (O)
		{
			Body += " Runner-up: " + O->Name + ".";
		}
		if ((E.Flags & FlagFirst) != 0)
		{
			Body += Fresh ? " Their first title in the record books." : " A first title.";
		}
		Tag = E.Kind == EventKind::Champion ? "CHAMPION" : Live ? "LIVE" : "BIG WIN";
		return true;
	}
	case EventKind::FinalTable:
		Title = Who + " makes the final table of " + E.What;
		Body = Ordinal(E.Value) + " place, " + Money(E.Amount) + ".";
		Tag = Live ? "LIVE" : "FINAL TABLE";
		return true;
	case EventKind::Discovered:
		Title = "Unknown " + Who + " wins " + E.What;
		Body = std::string((E.Flags & FlagQualifier) != 0 ? "A satellite qualifier nobody had heard of" : "A name nobody had heard of") + " takes " + Money(E.Amount) + " from " + Grouped(E.Value) +
			" entries.";
		Tag = "NEW NAME";
		return true;
	case EventKind::Breakout:
		Title = Who + "'s breakout";
		Body = Money(E.Amount) + " for " + Ordinal(E.Value) + " in " + E.What + (Fresh ? ": the biggest score in their books so far." : ": the score of a career so far.");
		Tag = "BREAKOUT";
		return true;
	case EventKind::Qualified:
		if (!Known)
		{
			return false;
		}
		Title = Who + " wins a seat to the " + E.What;
		Body = "A " + Money(E.Amount) + " seat from a satellite.";
		Tag = "QUALIFIED";
		return true;
	case EventKind::MovedUp:
		if (!Known && E.Value < 4)
		{
			return false;
		}
		Title = Who + " moves up to " + E.What + " stakes";
		Body = "A new level, new regulars, bigger swings.";
		Tag = "MOVES";
		return true;
	case EventKind::MovedDown:
		if (!Known)
		{
			return false;
		}
		Title = Who + " steps down to " + E.What + " stakes";
		Body = "Rebuilding after a rough stretch.";
		Tag = "MOVES";
		return true;
	case EventKind::Retired:
		Title = Who + " retires";
		Body = Fresh ? (E.Amount > 0 ? Who + " steps away from the game with " + Short(E.Amount) + " in winnings since the record books opened." : Who + " steps away from the game.")
					 : "After " + std::to_string(std::max(1, (Day - N->Joined) / 365)) + " years and " + Short(E.Amount) + " in winnings, " + Who + " steps away from the game.";
		Tag = "RETIRED";
		return true;
	case EventKind::Returned:
		Title = Who + " is back";
		Body = E.What == "retired" ? "Out of retirement after " + std::to_string(std::max(1, E.Value / 30)) + " months away."
			: E.What == "broke" ? "Back at the tables after time away."
			: E.Value >= 14 ? "Back at the tables after " + std::to_string(E.Value / 7) + " weeks away." : "Back at the tables.";
		Tag = "COMEBACK";
		return Known || E.What == "retired";
	case EventKind::Break:
		if (!Known)
		{
			return false;
		}
		Title = Who + " takes a break";
		Body = "Stepping away for a while.";
		Tag = "AWAY";
		return true;
	case EventKind::Sponsored:
		Title = Who + " joins " + E.What;
		Body = E.What == "Team RiverLine" ? "The site's newest sponsored pro." : "A sponsorship deal for the Kast channel.";
		if (Fresh && E.What == "Team RiverLine")
		{
			// A fresh world's team signs on results (update 2 §3.3 F3), and its first class is news of its own.
			// (Value packs the Season rank and the final tables at the signing: Sim::SigningFinalTables.)
			const int Fts = E.Value / Sim::SigningFinalTables;
			const int Rank = E.Value % Sim::SigningFinalTables;
			std::string Why;
			if (Fts > 0)
			{
				Why = Fts == 1 ? std::string("a final table on record") : Grouped(Fts) + " final tables on record";
			}
			if (Rank >= 1 && Rank <= 100)
			{
				Why += (Why.empty() ? "" : " and ") + std::string("No. ") + Grouped(Rank) + " on the Season board";
			}
			Body = std::string((E.Flags & FlagFirst) != 0 ? "One of the site's first team pros" : "The site's newest sponsored pro") + (Why.empty() ? "." : ", signed on results: " + Why + ".");
		}
		Tag = "SPONSOR";
		return true;
	case EventKind::SignedSiteTeam:
		// A rival's team (update 2 §5.9): "Ana Ribeiro joins Team RunItTwice".
		Title = Who + " joins " + E.What;
		Body = TextSitePossessive(Site) + " newest sponsored pro." + (Fresh ? " Signed on results." : "");
		Tag = "SPONSOR";
		return Site != net::SiteId::RiverLine;
	case EventKind::JoinedSite:
	{
		// A regular the news knows opens an account on a rival.
		const net::SiteId Account = net::SiteOf(E.What + ".");
		if (!Known || Account == net::SiteId::RiverLine)
		{
			return false;
		}
		Title = Who + " opens an account on " + net::Def(Account).Name;
		Body = Account == net::SiteId::RunItTwice ? "Another regular takes on the tougher fields." : "Another familiar name in the lobby.";
		Tag = "MOVES";
		return true;
	}
	case EventKind::Milestone:
		if (E.What == "career-won")
		{
			Title = Fresh ? Who + " passes " + Short(E.Amount) + " in winnings on the books" : Who + " passes " + Short(E.Amount) + " in career winnings";
			Body = Fresh ? "Since the record books opened on " + Opened + ". A milestone few reach." : "A milestone few reach.";
		}
		else if (E.What == "first-title")
		{
			Title = Fresh ? Who + " wins a first major title in the new record books" : Who + " wins a first major title";
			Body = Money(E.Amount) + ".";
		}
		else if (E.What == "turned-pro")
		{
			if (!Known)
			{
				return false;
			}
			Title = Who + " goes pro";
			Body = "The day job is over.";
		}
		else
		{
			return false;
		}
		Tag = "MILESTONE";
		return true;
	case EventKind::StartedStreaming:
		Title = Who + " starts streaming on Kast";
		Body = "Another regular goes live.";
		Tag = "KAST";
		return Known;
	case EventKind::StreamMilestone:
		Title = Who + " reaches " + Grouped(E.Value) + " followers on Kast";
		Body = "The channel keeps growing.";
		Tag = "KAST";
		return true;
	case EventKind::Debut:
		Title = "A new name: " + Who;
		Body = "Fresh on the scene and already turning heads.";
		Tag = "NEW FACE";
		return true;
	case EventKind::Rivalry:
		if (!O)
		{
			return false;
		}
		Title = Who + " vs " + O->Name;
		Body = "They keep meeting heads-up. Neither is letting it go.";
		Tag = "RIVALRY";
		return true;
	case EventKind::PlayerOfYear:
		Title = Who + " is " + E.What + " " + std::to_string(YearOf(Day));
		// (A fresh world's first season began with its books: "2026 · from Oct 6", F6.)
		Body = Fresh && YearOf(Day) == YearOf(Sim::BooksOpened(*this)) ? Grouped(E.Value) + " points since the record books opened on " + Opened + "." : Grouped(E.Value) + " points.";
		Tag = "PLAYER OF THE YEAR";
		return true;
	case EventKind::HallOfFame:
	{
		// "Class of 2031: two bracelets, three rings, a Player of the Year title and $4.2M in winnings."
		std::vector<std::string> Held;
		if (N->Bracelets > 0)
		{
			Held.push_back(N->Bracelets == 1 ? std::string("a bracelet") : Count(N->Bracelets) + " bracelets");
		}
		if (N->Rings > 0)
		{
			Held.push_back(N->Rings == 1 ? std::string("a ring") : Count(N->Rings) + " rings");
		}
		if (const int Poy = PlayerOfYearTitles(N->Id))
		{
			Held.push_back(Poy == 1 ? std::string("a Player of the Year title") : Count(Poy) + " Player of the Year titles");
		}
		Held.push_back(Short(E.Amount) + " in winnings");
		std::string List;
		for (size_t K = 0; K < Held.size(); ++K)
		{
			List += (K == 0 ? "" : K + 1 == Held.size() ? " and " : ", ") + Held[K];
		}
		Title = Who + " enters the Hall of Fame";
		Body = "Class of " + std::to_string(E.Value) + ": " + List + ".";
		Tag = "HALL OF FAME";
		return true;
	}
	default: return false; // going broke, taking a stake: nobody's business
	}
}

std::string World::Describe(int Id) const
{
	const Npc* P = Get(Id);
	if (!P)
	{
		return "no such person\n";
	}
	const Npc& N = *P;
	const int Day = sim::DayAt(Now);
	std::ostringstream O;
	O << "#" << N.Id << " " << N.Name << " (" << N.Country << ", " << N.Age(Day) << ")  " << IdentityName(N.Is) << " - started as " << IdentityName(N.Began) << ", " << OriginName(N.From)
	  << "\n";
	O << "  status " << StatusName(N.St);
	if (N.St == Status::Break)
	{
		O << " until " << net::DateLabel(N.Until);
	}
	O << "  mood " << MomentumName(N.Mood) << " (since " << net::DateLabel(N.MoodSince) << ")  confidence " << Fixed(static_cast<double>(N.Confidence), 2) << "  fatigue "
	  << Fixed(static_cast<double>(N.Fatigue), 2) << "  tilt " << Fixed(static_cast<double>(N.Tilt), 2) << "\n";
	O << "  bankroll " << Money(N.Bankroll) << " (peak " << Money(N.PeakRoll) << ")  " << (N.Professional ? "professional, living " + Money(N.Living) + "/wk" : "job " + Money(N.Income) + "/wk");
	if (N.Professional && N.Income > 0)
	{
		O << ", income " << Money(N.Income) << "/wk";
	}
	if (N.Backer != -1)
	{
		// -2 is the stable: Lantern Row Staking, FoldEquityFred's (next_segment_spec §2.5).
		O << "  backed by " << (N.Backer >= 0 ? Roster[static_cast<size_t>(N.Backer)].Name : std::string("Lantern Row Staking")) << " (makeup " << Money(N.Makeup) << ")";
	}
	O << "\n  stakes " << net::TierName(static_cast<net::Tier>(sim::TierIndex(N.Tier))) << " (comfortable at " << Money(Sim::Comfort(N)) << ")  live " << LiveName(N.Live)
	  << "  plays " << PlanName(N.Schedule) << ", " << Pct(static_cast<double>(N.OnlineShare)) << " online";
	if (N.TripUntil >= Day)
	{
		O << "  trip: " << N.Trip << " " << net::DateLabel(N.TripFrom) << "-" << net::DateLabel(N.TripUntil);
	}
	O << "\n  skill " << Fixed(static_cast<double>(N.Overall()), 3) << " (peak " << Fixed(static_cast<double>(N.Peak), 2) << ", potential " << Fixed(static_cast<double>(N.Potential), 2)
	  << "):";
	for (int K = 0; K < SkillCount; ++K)
	{
		O << (K % 5 == 0 ? "\n    " : "  ") << SkillName(static_cast<Skill>(K)) << " " << Fixed(static_cast<double>(N.Skills[static_cast<size_t>(K)]), 2);
	}
	static const char* const Traits[TraitCount] = {"discipline", "ambition", "risk", "patience", "sociability", "grit", "aggression"};
	O << "\n  traits:";
	for (int K = 0; K < TraitCount; ++K)
	{
		O << " " << Traits[K] << " " << Fixed(static_cast<double>(N.Traits[static_cast<size_t>(K)]), 2);
	}
	O << "\n  reputation:";
	for (int K = 0; K < RepCount; ++K)
	{
		O << " " << RepName(static_cast<Rep>(K)) << " " << static_cast<int>(std::lround(static_cast<double>(N.Reps[static_cast<size_t>(K)])));
	}
	if (N.Streams || N.Followers > 0)
	{
		O << "\n  streaming " << (N.Streams ? "on" : "stopped") << ", " << Grouped(N.Followers) << " followers";
	}
	if (!N.Sponsor.empty())
	{
		O << "  sponsor " << N.Sponsor;
	}
	O << "\n  career " << Money(N.CareerWon()) << "  bracelets " << N.Bracelets << "  rings " << N.Rings << "  series titles " << N.Titles << "  majors " << N.Majors << "\n";
	// The sites they play, and their record on each rival (update 2 §5.9).
	O << "  sites:";
	for (int S = 0; S < static_cast<int>(net::SiteId::Count); ++S)
	{
		if (((N.Sites >> S) & 1) != 0)
		{
			O << " " << net::KeyOf(static_cast<net::SiteId>(S));
			if (const SiteRecord* R = SiteRecordOf(static_cast<net::SiteId>(S), N.Id))
			{
				O << " (" << R->Events << " events, " << Fixed(R->Points, 0) << " pts, " << Money(R->Won) << " this season, " << Money(R->Lifetime) << " all time)";
			}
		}
	}
	O << "\n";
	{
		const Tracker& T = N.Stats;
		char Buf[256];
		std::snprintf(Buf, sizeof(Buf), "  stats: %d tournaments, ROI %+.1f%%, ITM %.1f%%, ABI %s, net %s, peak %s, downswing %s (#%d-#%d), drought %d, avg finish top %.0f%%, graph %zu x %d\n",
			T.Events, T.Roi() * 100.0, T.Itm() * 100.0, Money(T.AverageBuyIn()).c_str(), Money(T.Net).c_str(), Money(T.Peak).c_str(), Money(T.Downswing).c_str(), T.DownFrom, T.DownTo,
			T.LongestDry, T.AverageFinish() * 100.0, T.Curve.size(), T.Stride);
		O << Buf;
	}
	for (const Award& A : N.Awards)
	{
		O << "    " << (A.Ring ? "ring" : "bracelet") << (A.Main ? " (Main Event)" : "") << ": " << (A.Event.empty() ? std::string("(from an older save)") : A.Event)
		  << (A.Series.empty() ? std::string() : "  [" + A.Series + "]") << "  day " << A.Day << "\n";
	}
	for (int V = 0; V < VenueCount; ++V)
	{
		const Ledger& L = N.Totals[static_cast<size_t>(V)];
		if (L.Events == 0)
		{
			continue;
		}
		O << "    " << VenueName(V) << ": " << Grouped(L.Events) << " events, won " << Money(L.Won) << ", spent " << Money(L.Spent) << ", " << L.Cashes << " cashes, " << L.FinalTables
		  << " final tables, " << L.Wins << " wins, best " << Money(L.Best) << "\n";
	}
	if (!N.BestEvent.empty())
	{
		O << "  best: " << N.BestEvent << " (" << net::DateLabel(N.BestDay) << ")\n";
	}
	O << "  season " << N.ThisSeason.Year << ": " << Fixed(N.ThisSeason.Points, 0) << " pts, " << Money(N.ThisSeason.Won) << ", " << N.ThisSeason.Wins << " wins, live "
	  << Fixed(N.ThisSeason.LivePoints, 0) << " pts\n";
	{
		static const char* const Cards[4] = {"none", "card", "gold", "black"};
		const RoomRecord& R = N.Room;
		O << "  badges: circuit " << Fixed(N.CircuitPoints, 0) << " pts (rank " << CircuitRank(N.Id) << "), card " << Cards[std::max(0, std::min(3, N.CardBest))] << ", last season "
		  << Cards[std::max(0, std::min(3, N.CardLast))] << ", verified " << (N.Verified ? "yes" : "no");
		if (const int Class = HallClass(N.Id))
		{
			O << ", Hall of Fame " << Class;
		}
		O << "\n  embercrest: " << R.Events << " events, " << R.Cashes << " cashes, " << R.FinalTables << " final tables, " << R.Wins << " on the wall, " << R.Days.size()
		  << " days kept, month " << Fixed(R.MonthPoints, 0) << " pts (last " << Fixed(R.LastMonthPoints, 0) << ")\n";
	}
	for (const Year& Y : N.Years)
	{
		O << "    " << Y.Number << ": online " << Money(Y.Online) << ", live " << Money(Y.Live) << ", net " << Money(Y.Net) << ", " << Y.Wins << " wins in " << Y.Events << "\n";
	}
	if (!N.Recent.empty())
	{
		O << "  recent:\n";
		for (const world::Finish& F : N.Recent)
		{
			O << "    " << net::DateLabel(F.Day) << " " << Ordinal(F.Place) << " of " << Grouped(F.Entries) << " " << F.Event << " " << Money(F.Prize) << "\n";
		}
	}
	if (!N.Ties.empty())
	{
		O << "  knows:";
		for (const Tie& T : N.Ties)
		{
			const Npc* Other = Get(T.Other);
			O << " " << TieName(T.Kind) << " " << (Other ? Other->Name : "?") << " (" << Fixed(static_cast<double>(T.Strength), 2) << ")";
		}
		O << "\n";
	}
	if (!N.Tickets.empty())
	{
		O << "  tickets:";
		for (const auto& T : N.Tickets)
		{
			O << " " << T.first << " x" << T.second;
		}
		O << "\n";
	}
	if (const Bond* B = BondWith(N.Id))
	{
		O << "  about you: " << (B->Label().empty() ? "a stranger" : B->Label()) << " (familiar " << Fixed(static_cast<double>(B->Familiarity), 2) << ", respect "
		  << Fixed(static_cast<double>(B->Respect), 2) << ", trust " << Fixed(static_cast<double>(B->Trust), 2) << ", rivalry " << Fixed(static_cast<double>(B->Rivalry), 2) << ", resentment "
		  << Fixed(static_cast<double>(B->Resentment), 2) << ")\n";
		for (const Memory& M : B->Memories)
		{
			O << "    " << net::DateLabel(M.Day) << " " << MemoryText(M.Kind) << " (" << M.Where << ")\n";
		}
	}
	// What they're registered for.
	int Shown = 0;
	for (const Pending& Q : Queue)
	{
		for (const Entry& E : Q.Who)
		{
			if (E.Npc == N.Id && Shown < 6)
			{
				O << (Shown == 0 ? "  playing: " : ", ") << Q.Name << " " << net::TimeLabel(Q.Start);
				++Shown;
			}
		}
	}
	if (Shown > 0)
	{
		O << "\n";
	}
	int Story = 0;
	for (auto It = Log.rbegin(); It != Log.rend() && Story < 8; ++It)
	{
		if (It->Npc == N.Id || It->Other == N.Id)
		{
			O << (Story == 0 ? "  history:\n" : "") << "    " << net::DateLabel(sim::DayAt(It->At)) << " " << YearOf(sim::DayAt(It->At)) << " " << EventKindName(It->Kind) << " " << It->What
			  << (It->Amount ? " " + Money(It->Amount) : "") << "\n";
			++Story;
		}
	}
	// How they got here, and the steps since.
	const Profile Pr = ProfileOf(N.Id);
	O << "  arrival: " << (N.Came == Arrival::None ? std::string("before the story began") : std::string(ArrivalName(N.Came)) + ", " + net::DateLabel(N.Arrived) + " " + std::to_string(YearOf(N.Arrived)))
	  << " - " << Pr.Came << "\n";
	for (const Profile::Moment& M : Pr.Journey)
	{
		O << "    " << net::DateLabel(M.Day) << " " << YearOf(M.Day) << "  " << M.Text << "\n";
	}
	return O.str();
}

std::string World::Report() const
{
	const int Day = sim::DayAt(Now);
	std::ostringstream O;
	O << "World " << WorldSeed << " on " << net::DateLabel(Day) << " " << YearOf(Day) << " (day " << Day << ", " << Fixed((Now - Origin) / 1440.0 / 365.0, 2) << " years in)\n";
	int ByStatus[4] = {0, 0, 0, 0};
	int ByTier[5] = {0, 0, 0, 0, 0};
	int ByMood[static_cast<int>(Momentum::Count)] = {};
	int ByIdentity[static_cast<int>(Identity::Count)] = {};
	int ByOrigin[5] = {0, 0, 0, 0, 0};
	int ByLive[6] = {0, 0, 0, 0, 0, 0};
	int Pros = 0;
	int Streamers = 0;
	int Backed = 0;
	int Elite = 0;
	std::vector<double> Rolls[5];
	std::vector<double> Skills;
	std::vector<double> Ages;
	Chips Total = 0;
	for (const Npc& N : Roster)
	{
		++ByStatus[static_cast<int>(N.St)];
		++ByOrigin[static_cast<int>(N.From)];
		if (!N.Playing())
		{
			continue;
		}
		const int T = sim::TierIndex(N.Tier);
		++ByTier[T];
		++ByMood[static_cast<int>(N.Mood)];
		++ByIdentity[static_cast<int>(N.Is)];
		++ByLive[static_cast<int>(N.Live)];
		Pros += N.Professional ? 1 : 0;
		Streamers += N.Streams ? 1 : 0;
		Backed += N.Backer != -1 ? 1 : 0;
		Elite += N.Overall() >= 0.75f ? 1 : 0;
		Rolls[T].push_back(sim::Dollars(N.Bankroll));
		Skills.push_back(static_cast<double>(N.Overall()));
		Ages.push_back(static_cast<double>(N.Age(Day)));
		Total += N.Bankroll;
	}
	O << "  people " << Roster.size() << ": active " << ByStatus[0] << ", break " << ByStatus[1] << ", broke " << ByStatus[2] << ", retired " << ByStatus[3] << "  (target " << Target << ")\n";
	O << "  origins: founding " << ByOrigin[0] << ", cast " << ByOrigin[1] << ", streamers " << ByOrigin[2] << ", newcomers " << ByOrigin[3] << ", discovered " << ByOrigin[4] << "\n";
	{
		std::array<int, static_cast<int>(Arrival::Count)> Came{};
		int Fresh = 0;
		for (const Npc& N : Roster)
		{
			++Came[static_cast<size_t>(N.Came)];
			Fresh += N.Came != Arrival::None && Day - N.Arrived <= 30 ? 1 : 0;
		}
		O << "  newcomers: " << Fresh << " in the last month;";
		for (int K = 1; K < static_cast<int>(Arrival::Count); ++K)
		{
			O << " " << ArrivalName(static_cast<Arrival>(K)) << " " << Came[static_cast<size_t>(K)] << (K + 1 < static_cast<int>(Arrival::Count) ? "," : "");
		}
		O << "\n";
	}
	O << "  active by stakes: micro " << ByTier[1] << ", low " << ByTier[2] << ", mid " << ByTier[3] << ", high " << ByTier[4] << "   live: none " << ByLive[0] << ", local " << ByLive[1]
	  << ", regional " << ByLive[2] << ", circuit " << ByLive[3] << ", championship " << ByLive[4] << ", high roller " << ByLive[5] << "\n";
	O << "  professionals " << Pros << ", streamers " << Streamers << ", backed " << Backed << ", skill >= .75: " << Elite << "\n";
	O << "  skill: median " << Fixed(Percentile(Skills, 0.5), 3) << ", 10% " << Fixed(Percentile(Skills, 0.1), 3) << ", 90% " << Fixed(Percentile(Skills, 0.9), 3) << ", max "
	  << Fixed(Percentile(Skills, 1.0), 3) << "   age median " << Fixed(Percentile(Ages, 0.5), 0) << "\n";
	O << "  bankrolls (median / 90%):";
	static const char* const Tiers[5] = {"", "micro", "low", "mid", "high"};
	for (int T = 1; T < 5; ++T)
	{
		O << "  " << Tiers[T] << " " << Money(sim::Cents(Percentile(Rolls[T], 0.5))) << " / " << Money(sim::Cents(Percentile(Rolls[T], 0.9)));
	}
	O << "\n  money in active bankrolls " << Money(Total) << "\n";
	O << "  field strength:";
	for (int T = 1; T < 5; ++T)
	{
		O << " " << Tiers[T] << " " << Fixed(Field[static_cast<size_t>(T)], 3);
	}
	O << "\n  moods:";
	for (int K = 0; K < static_cast<int>(Momentum::Count); ++K)
	{
		O << " " << MomentumName(static_cast<Momentum>(K)) << " " << ByMood[K];
	}
	O << "\n  identities:";
	for (int K = 0; K < static_cast<int>(Identity::Count); ++K)
	{
		if (ByIdentity[K] > 0)
		{
			O << " " << IdentityName(static_cast<Identity>(K)) << " " << ByIdentity[K] << ";";
		}
	}
	O << "\n  reputation leaders:";
	for (int Id : Leaders(Rep::Overall, 5))
	{
		const Npc& N = Roster[static_cast<size_t>(Id)];
		O << " " << N.Name << " " << static_cast<int>(std::lround(static_cast<double>(N.RepOf(Rep::Overall))));
	}
	O << "\n  events logged " << Log.size() << ", honors " << Titles.size() << ", results kept " << Results.size() << ", queued " << Queue.size() << "\n";
	{
		// The badges' world data (next_segment_spec §5.3).
		int Verified = 0;
		int Cards[4] = {0, 0, 0, 0};
		int Last[4] = {0, 0, 0, 0};
		int Club[4] = {0, 0, 0, 0};
		int Wall = 0;
		int Bracelets = 0;
		int Rings = 0;
		int Branded = 0;
		int Team = 0;
		for (const Npc& N : Roster)
		{
			if (N.Faded)
			{
				continue;
			}
			Verified += N.Verified ? 1 : 0;
			++Cards[std::max(0, std::min(3, N.CardBest))];
			++Last[std::max(0, std::min(3, N.CardLast))];
			const int Visits = N.Room.VisitsIn(Day);
			++Club[Visits >= badge::ClubCrest ? 3 : Visits >= badge::ClubEmber ? 2 : Visits >= badge::ClubRegular ? 1 : 0];
			Wall += N.Room.Wins > 0 ? 1 : 0;
			Bracelets += N.Bracelets > 0 ? 1 : 0;
			Rings += N.Rings > 0 ? 1 : 0;
			Team += N.Pro ? 1 : 0;
			Branded += !N.Pro && !N.Sponsor.empty() ? 1 : 0;
		}
		int Hall = 0;
		for (const Honor& H : Titles)
		{
			Hall += H.Title == HallOfFameTitle ? 1 : 0;
		}
		O << "  badges: verified " << Verified << ", cards " << Cards[1] << "/" << Cards[2] << "/" << Cards[3] << " (last season " << Last[1] << "/" << Last[2] << "/" << Last[3]
		  << "), club " << Club[1] << "/" << Club[2] << "/" << Club[3] << ", on the wall " << Wall << ", bracelet holders " << Bracelets << ", ring holders " << Rings
		  << ", Team RiverLine " << Team << ", brand-sponsored " << Branded << ", Hall of Fame " << Hall << "\n";
	}
	if (!SiteLaunches.empty())
	{
		// The rival sites (update 2 §5.9): who plays where, each one's records and its team.
		O << "  sites:";
		for (const auto& It : SiteLaunches)
		{
			const net::SiteId S = It.first;
			int Members = 0;
			int Only = 0;
			int Team = 0;
			const std::string TeamName = net::Def(S).Id == S ? net::Def(S).TeamName : std::string();
			for (const Npc& N : Roster)
			{
				if (!N.Playing() || ((N.Sites >> static_cast<int>(S)) & 1) == 0)
				{
					continue;
				}
				++Members;
				Only += (N.Sites & 1) == 0 ? 1 : 0;
				Team += !TeamName.empty() && N.Sponsor == TeamName ? 1 : 0;
			}
			const auto Records = SiteRecords.find(S);
			O << " " << net::KeyOf(S) << " opens " << net::DateLabel(sim::DayAt(It.second)) << " " << YearOf(sim::DayAt(It.second)) << ": " << Members << " playing (" << Only
			  << " there only), " << (Records == SiteRecords.end() ? 0 : Records->second.size()) << " with a record, team " << Team << ";";
		}
		O << "\n";
	}
	return O.str();
}
} // namespace world
} // namespace ss
