// Update 3 (SPEC §3.6, §7.3): RunItTwice's loops, core-siteplay's file. The League with LeagueScore (the rule the world
// scores its members by), Callouts, H2H, Team Scalps, the Watchlist and the desk bell's keys, the Luck Ledger, credentials
// and the Briefing. RunItTwice pays in status first and in cash it already pays (Second Run, SiteMoney.cpp); its League and
// Scalp tickets go through the Governor (SitePlay.cpp), and the Second Run is booked to the month that earned it.
#include "ShortStack/Game/SitePlay.h"

#include "SitePlayPriv.h"
#include "SitesDetail.h"
#include "ShortStack/Common.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/SiteShow.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Game/WorldLeague.h"

#include <algorithm>
#include <array>
#include <map>
#include <cmath>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace site
{
namespace play
{
namespace siteplay_detail
{
namespace
{
const char* const RitDivNames[5] = {"Open", "Iron", "Steel", "Gold", "High Bridge"};
const Chips RitFloors[5] = {0, 550, 1100, 2200, 5500};

/** Each division's own Matchdays (R2): the fixture on its weekday (0 Monday) at x1.5. */
struct RitFixture
{
	int Div;
	const char* Template;
	int Weekday;
	const char* Name;
};
const RitFixture RitFixtures[] = {
	{0, "supper-stack", 1, "Supper Stack"},
	{0, "double-shot", 4, "Double Shot"},
	{1, "supper-stack", 1, "Supper Stack"},
	{1, "double-shot", 4, "Double Shot"},
	{2, "the-sweat", 1, "The Sweat"},
	{2, "twin-turbo-pko", 4, "Twin Turbo PKO"},
	{3, "encore-pko", 1, "Encore PKO"},
	{3, "run-it-back", 3, "Run It Back"},
	{3, "sunday-grind", 6, "Sunday Grind"},
	{4, "the-fifty-five", 1, "The Fifty-Five"},
	{4, "the-nightcap", 4, "The Nightcap"},
	{4, "sunday-100k", 6, "Sunday 100K"},
};

/** A ticket RunItTwice hands out: where it's held, what it says, its face. */
struct RitTicketDef
{
	const char* Id;
	const char* Label;
	Chips Face;
};
const RitTicketDef RitSupperStack = {"runittwice.supper-stack", "Supper Stack $5.50 ticket", 550};
const RitTicketDef RitSweat = {"runittwice.the-sweat", "The Sweat $11 ticket", 1100};
const RitTicketDef RitGrind = {"runittwice.sunday-grind", "Sunday Grind $22 ticket", 2200};
const RitTicketDef RitFiftyFive = {"runittwice.the-fifty-five", "The Fifty-Five $55 ticket", 5500};
const RitTicketDef RitHundredK = {"runittwice.sunday-100k", "Sunday 100K $109 ticket", 10900};
const RitTicketDef RitDeepstack = {"runittwice.afternoon-deepstack", "Afternoon Deepstack $27.50 ticket", 2750};

/** A promotion's ticket, by the division it leaves (High Bridge: its top 3's Sunday 100K). */
const RitTicketDef& RitPromotionTicket(int FromDiv)
{
	switch (FromDiv)
	{
	case 0: return RitSupperStack;
	case 1: return RitSweat;
	case 2: return RitGrind;
	case 3: return RitFiftyFive;
	default: return RitHundredK;
	}
}

/** A Team scalp's ticket, by the event's buy-in (R4). */
const RitTicketDef& RitScalpTicket(Chips BuyIn)
{
	return BuyIn >= 30000 ? RitDeepstack : BuyIn >= 10900 ? RitSweat : RitSupperStack;
}

/** The FULL SET's ticket, by division (R4). */
const RitTicketDef& RitFullSetTicket(int Div)
{
	return Div >= 4 ? RitHundredK : Div == 3 ? RitGrind : RitSweat;
}

/** Credentials (R7), by bit. */
const char* const RitCreds[11] = {"First cash", "First final table", "First win", "Sunday 100K final table", "RunItTwice Open title", "Promoted to Gold",
	"High Bridge reached", "Top of the group", "FULL SET", "100 RunItTwice events", "1,000 RunItTwice events"};

const char* const RitSentences[] = {
	"You're not in a League group this month.",
	"That member isn't in your group.",
	"You've called someone out this Matchweek already.",
	"You're on leave this month: no Callouts in or out.",
	"Your Watchlist holds 12. Take one off first.",
	"That event is on your Watchlist already.",
	"That event isn't on your Watchlist.",
	"There's no such event on RunItTwice.",
	"We don't do boxes. We do standings.",
	"No spins. No boxes. Rake back, in cash, on the 1st.",
	"Under your Team RiverLine deal, every table here is a strike.",
	"On leave: your division is kept.",
	"Cash 3 times in Matchweek 1 and start 30 points up.",
	"You are running {Pct} below expectation.",
	"You are running {Pct} above expectation.",
	"You are running right at expectation.",
	"You'd have won {Won} of {Of} second runs.",
	"Ingrid Vale here. You finished {Place} of {Of} in {Div} in {Month}, and that takes you up to {NewDiv}. Your {Ticket} is on the account.",
	"You're the bounty now: every table here knows who you are.",
};
enum RitSentence : int
{
	RitSayNoGroup = 0,
	RitSayNotMember,
	RitSayCalled,
	RitSayLeave,
	RitSayStarsFull,
	RitSayStarred,
	RitSayNotStarred,
	RitSayNoEvent,
	RitSayStandings,
	RitSayNoBoxes,
	RitSayClause,
	RitSayOnLeave,
	RitSayBounce,
	RitSayBelow,
	RitSayAbove,
	RitSayEven,
	RitSaySecond,
	RitSayLetter,
	RitSayBounty,
};
const char* const RitLabels[] = {
	"THE LEAGUE", "MATCHDAY", "DEADLINE DAY", "LEAGUE NIGHT", "PROMOTION ZONE", "BOUNCE BACK", "PLACEMENT", "ON LEAVE", "INCOMING CALLOUT", "CALLOUT",
	"TAKEN", "TEAM", "FULL SET", "BRIEFING", "LUCK LEDGER", "Second Run Club", "League ticket", "Team Scalp", "The League: you're up to {NewDiv}",
	"Rewards: ",
	// The Briefing's parts (Session::SitePlayView joins them with " · ").
	"yesterday {Events} events, {Net}", "yesterday 1 event, {Net}", "League {From} to {To}", "{Pts} pts to promotion", "MATCHDAY TONIGHT: {Fixture}",
	"INCOMING CALLOUT: {Name}",
};

const char* RitSay(int Line)
{
	return RitSentences[Line];
}

bool RitOnLeaveNow(const sponsor::Book* Deals)
{
	return Deals && sponsor::WouldStrike(*Deals, static_cast<int>(net::SiteId::RunItTwice), false, false);
}

const net::EventTemplate* RitTemplateOf(const std::string& TemplateOrEvent)
{
	std::string T = TemplateOrEvent.substr(0, TemplateOrEvent.find('@'));
	if (T.compare(0, 11, "runittwice.") != 0)
	{
		T = "runittwice." + T;
	}
	return net::Site(net::SiteId::RunItTwice).FindTemplate(T);
}

/** The Matchweek key: weeks of the month from its 1st (1-7, 8-14, ...). */
int RitMatchweekKey(int Day)
{
	int Y = 0, M = 0, Dm = 0;
	world::CivilDate(Day, Y, M, Dm);
	return PlayMonth(Day) * 8 + (Dm - 1) / 7;
}

/** The last play day of Day's month (Deadline Day). */
int RitLastDayOf(int Day)
{
	return world::MonthStart(world::MonthStart(Day) + 32) - 1;
}

// The seen marks (G2 fix L6): one dated done key of each kind, its key the closed month or the Matchweek.
const char* const RitSeenNight = "play:seen:night:";
const char* const RitSeenCall = "play:seen:call:";

int RitSeenOf(const site::Book& Sites, const std::string& Prefix)
{
	const auto It = Sites.Done.lower_bound(Prefix);
	return It != Sites.Done.end() && It->compare(0, Prefix.size(), Prefix) == 0 ? std::atoi(It->c_str() + Prefix.size()) : -1;
}

bool RitSee(life::State& L, const std::string& Prefix, int Key, double World)
{
	if (Key < 0 || !SitePlayRitStarted(L.SitePlay) || RitSeenOf(L.Sites, Prefix) == Key)
	{
		return false;
	}
	for (auto It = L.Sites.Done.lower_bound(Prefix); It != L.Sites.Done.end() && It->compare(0, Prefix.size(), Prefix) == 0;)
	{
		It = L.Sites.Done.erase(It);
	}
	SitePlayMarkDone(L, Prefix + std::to_string(Key), World);
	return true;
}

/** The world's group book for a month (null: none recorded for it). */
const world::LeagueBook* RitBookOf(const world::World* W, int Month)
{
	const world::LeagueBook* Lb = W ? W->League(net::SiteId::RunItTwice) : nullptr;
	return Lb && Lb->MonthKey == Month ? Lb : nullptr;
}

double RitMemberPoints(const world::LeagueMember& M)
{
	double Sum = 0.0;
	for (const auto& P : M.Best)
	{
		Sum += static_cast<double>(P.second) / 10.0;
	}
	return Sum;
}

/** A member's points as of the end of a day of the month (the overnight move): their best days on or before it. */
double RitMemberPointsBy(const world::LeagueMember& M, int DayOfMonth)
{
	double Sum = 0.0;
	for (const auto& P : M.Best)
	{
		Sum += P.first > 0 && P.first <= DayOfMonth ? static_cast<double>(P.second) / 10.0 : 0.0;
	}
	return Sum;
}

const world::LeagueMember* RitMember(const world::LeagueBook* Lb, int Npc)
{
	if (!Lb)
	{
		return nullptr;
	}
	for (const world::LeagueMember& M : Lb->Members)
	{
		if (M.Npc == Npc)
		{
			return &M;
		}
	}
	return nullptr;
}

/** The player's points from days before Day (the overnight move). */
double RitHeroPointsBefore(const League& Lg, int Day)
{
	std::vector<float> Pts;
	double Bonus = 0.0;
	for (const auto& D : Lg.Days)
	{
		if (D.first < 0)
		{
			Bonus += static_cast<double>(D.second);
		}
		else if (D.first < Day)
		{
			Pts.push_back(D.second);
		}
	}
	std::sort(Pts.begin(), Pts.end(), [](float A, float B) { return A > B; });
	double Sum = Bonus;
	for (size_t K = 0; K < Pts.size() && K < 8; ++K)
	{
		Sum += static_cast<double>(Pts[K]);
	}
	return Sum;
}

bool RitTierFits(int Div, int Tier, int Widen)
{
	// Division -> the online stakes tiers (net::Tier: 1 micro, 2 low, 3 mid, 4 high) its members play.
	static const int Lo[5] = {1, 1, 2, 2, 3};
	static const int Hi[5] = {1, 2, 2, 3, 4};
	return Tier >= Lo[Div] - Widen && Tier <= Hi[Div] + Widen;
}

/**
 * The month's group (R2): a keyed shuffle over RunItTwice members whose stakes fit the division, preferring people the
 * player has sat with, last month's group-mates and members whose monthly volume is within 0.5-2x the player's. Fewer
 * than 19 eligible widens the band a tier at a time. The group's pace is the first-3 rule unless the draw couldn't find
 * enough volume-matched members: then it's the members' median entries per playing day (1-3).
 */
void RitDraw(Book& B, const life::State& L, const world::World* W, uint64_t WorldSeed, int Month, int Day)
{
	League& Lg = B.Rt.L;
	const int Div = std::clamp<int>(Lg.Div, 0, 4);
	std::array<int, 19> Prev = Lg.Npcs;
	const bool HadLast = Lg.Month == Month - 1;
	Lg.Npcs.fill(-1);
	Lg.Days.clear();
	Lg.TodayKey = -1;
	Lg.TodayCount = 0;
	Lg.Pace = 3;
	Lg.Leave = false; // the new month's own (RitStep marks it while a clause covers the month)
	Lg.Month = Month;
	Lg.Group = static_cast<int8_t>(1 + Fnv1a(std::to_string(WorldSeed) + ":rit:group:" + std::to_string(Month) + ":" + std::to_string(Div)) % 12u);
	if (!W)
	{
		return;
	}
	const Account* A = AccountOf(L.Sites, net::SiteId::RunItTwice);
	const double Months = A ? std::max(1.0, (PlayDayStart(Day) - A->JoinedAt) / (30.0 * 1440.0)) : 1.0;
	const double Mine = std::max(10.0, A ? static_cast<double>(A->LifetimeEvents) / Months : 10.0);
	const uint8_t Bit = static_cast<uint8_t>(1u << static_cast<int>(net::SiteId::RunItTwice));
	struct Pick
	{
		double Score;
		int Id;
		bool Matched;
		double Vol;
	};
	std::vector<Pick> Picks;
	// The members' monthly volumes, the world's own (core-world's WorldLeague.h MonthlyVolume, measured in S-6'): this season's
	// entries per 30 days, with the entries the world has planned for them in the season's first week, so January 1's draw
	// reads a real rate (the season's counts start again at midnight). One pass for everyone. (core-world's patch #1.)
	std::map<int, double> Volumes;
	for (const world::SiteMember& Sm : world::SiteMembers(*W, net::SiteId::RunItTwice, PlayDayStart(Day)))
	{
		Volumes[Sm.Npc] = Sm.Volume;
	}
	for (int Widen = 0; Widen <= 3 && Picks.size() < 19; ++Widen)
	{
		Picks.clear();
		for (const world::Npc& N : W->People())
		{
			if ((N.Sites & Bit) == 0 || !N.Playing() || N.Faded || !RitTierFits(Div, N.Tier, Widen))
			{
				continue;
			}
			const auto Known = Volumes.find(N.Id);
			const double Vol = Known == Volumes.end() ? 0.0 : Known->second;
			const bool Matched = Vol >= 0.5 * Mine && Vol <= 2.0 * Mine;
			double Score = SitePlayUnit(WorldSeed, "rit:league:" + std::to_string(Month) + ":" + std::to_string(Div) + ":" + std::to_string(N.Id));
			Score -= W->BondWith(N.Id) ? 0.5 : 0.0;
			Score -= HadLast && std::find(Prev.begin(), Prev.end(), N.Id) != Prev.end() ? 0.4 : 0.0;
			Score -= Matched ? 0.6 : 0.0;
			Picks.push_back({Score, N.Id, Matched, Vol});
		}
	}
	std::sort(Picks.begin(), Picks.end(), [](const Pick& X, const Pick& Y) { return X.Score != Y.Score ? X.Score < Y.Score : X.Id < Y.Id; });
	int Matched = 0;
	std::vector<double> Vols;
	for (size_t K = 0; K < Picks.size() && K < 19; ++K)
	{
		Lg.Npcs[K] = Picks[K].Id;
		Matched += Picks[K].Matched ? 1 : 0;
		Vols.push_back(Picks[K].Vol);
	}
	if (!Vols.empty() && Matched < 10)
	{
		// The group's pace: the members' median entries per playing day (1-3), for everyone in it.
		std::vector<double> Per;
		for (const double V : Vols)
		{
			Per.push_back(V >= 30.0 ? V / 30.0 : 1.0);
		}
		std::sort(Per.begin(), Per.end());
		Lg.Pace = static_cast<int8_t>(std::clamp<long long>(std::llround(Per[Per.size() / 2]), 1, 3));
	}
}

/** The roster for the world (World::SetLeagueRoster): the drawn members, the division packed with the pace. */
void RitRosterOut(const League& Lg, StepOut& Out)
{
	Out.Roster = true;
	Out.MonthKey = Lg.Month;
	Out.Div = LeaguePack(Lg.Div, Lg.Pace);
	Out.Npcs.clear();
	for (const int N : Lg.Npcs)
	{
		if (N >= 0)
		{
			Out.Npcs.push_back(N);
		}
	}
}

/** The player's place in the group now (1 + the members ahead; ties go the player's way) and the group's size. */
std::pair<int, int> RitRank(const League& Lg, const world::World* W)
{
	const double Mine = RitHeroPoints(Lg);
	const world::LeagueBook* Lb = RitBookOf(W, Lg.Month);
	int Ahead = 0;
	int Of = 1;
	for (const int N : Lg.Npcs)
	{
		if (N < 0)
		{
			continue;
		}
		++Of;
		const world::LeagueMember* M = RitMember(Lb, N);
		Ahead += M && RitMemberPoints(*M) > Mine ? 1 : 0;
	}
	return {1 + Ahead, Of};
}

H2H& RitHeadFor(Rit& R, const std::string& Name)
{
	for (H2H& H : R.H)
	{
		if (H.Name == Name)
		{
			return H;
		}
	}
	if (R.H.size() >= 8)
	{
		R.H.erase(R.H.begin()); // the oldest name makes way (8 kept)
	}
	H2H H;
	H.Name = Name;
	R.H.push_back(H);
	return R.H.back();
}

void RitCred(Rit& R, int Bit)
{
	R.Creds |= 1u << Bit;
}

bool RitGrantTicket(Book& B, life::State& L, Kind K, const RitTicketDef& T, const std::string& Key, int Day, double World, bool Reset)
{
	SitePlayPayload P;
	P.TicketId = T.Id;
	P.Label = T.Label;
	return SitePlayGrant(B, L, net::SiteId::RunItTwice, K, T.Label, T.Face * ValuePerCent, P, Key, Day, World, Reset);
}

/** Closes a League month (R2) on the 1st: the table's verdict, the ticket, the history, a letter for Gold or High Bridge. */
void RitClose(Book& B, life::State& L, const world::World* W, const sponsor::Book* Deals, double World)
{
	Rit& R = B.Rt;
	League& Lg = R.L;
	const std::pair<int, int> Place = RitRank(Lg, W);
	const int Rank = Place.first;
	const int Of = Place.second;
	const int Up = Of < 14 ? 2 : 3;
	const int Down = Of < 14 ? 2 : 4;
	const int Div = std::clamp<int>(Lg.Div, 0, 4);
	bool Played = false;
	for (const auto& D : Lg.Days)
	{
		Played = Played || D.first >= 0;
	}
	// On leave (W7): a month with no RunItTwice entry that a clause covered at any step of it, not only at the close (review:
	// a deal that ended on the 20th relegated a player who had kept it all month).
	Lg.Leave = !Played && (Lg.Leave || RitOnLeaveNow(Deals));
	int8_t Move = 0;
	const int Day = PlayDay(World);
	if (!Lg.Leave && Played && Rank <= Up)
	{
		// Promotion (High Bridge can't go higher: its top 3 win a Sunday 100K ticket instead).
		RitGrantTicket(B, L, Kind::LeagueTicket, RitPromotionTicket(Div), "league:" + std::to_string(Lg.Month), Day, World, true);
		if (Div < 4)
		{
			Move = 1;
			Lg.Div = static_cast<int8_t>(Div + 1);
		}
	}
	else if (!Lg.Leave && !Lg.Placement && Div > 0 && Rank > Of - Down && Of > Up + Down)
	{
		Move = -1;
		Lg.Div = static_cast<int8_t>(Div - 1);
	}
	if (Rank == 1 && Played)
	{
		RitCred(R, 7);
	}
	if (Lg.Div >= 3)
	{
		RitCred(R, 5);
	}
	if (Lg.Div >= 4)
	{
		RitCred(R, 6);
	}
	LeagueHist H;
	H.Month = Lg.Month;
	H.Div = static_cast<int8_t>(Div);
	H.Rank = Rank;
	H.Of = Of;
	H.Move = Move;
	R.Hist.push_back(H);
	if (R.Hist.size() > 12)
	{
		R.Hist.erase(R.Hist.begin());
	}
	Lg.Bounce = Move < 0;
	if (Move > 0 && Lg.Div >= 3 && !WroteThisWeek(L, net::SiteId::RunItTwice, World))
	{
		const std::string MonthWord = MonthName(MonthKeyAt(PlayDayStart(Day - 1)));
		const std::vector<std::pair<std::string, std::string>> Vars = {{"Place", Ordinal(Rank)}, {"Of", std::to_string(Of)}, {"Div", RitDivNames[Div]},
			{"Month", MonthWord}, {"NewDiv", RitDivNames[Lg.Div]}, {"Ticket", RitPromotionTicket(Div).Label}};
		PostLetter(L, net::SiteId::RunItTwice, World, LetterRoutine, FillCopy(RitLabels[18], Vars), FillCopy(RitSay(RitSayLetter), Vars));
	}
	R.C = Callout();
}

/**
 * A Callout's end (R3): the week's League points each, a win or a loss on the head-to-head record by CalloutEdge, the card's own
 * rule. A week that ends all square (0-0 when neither played) is neither: no row is touched (G2 PIE P6: a tie was booked as a
 * loss while the card said LEADING). The save keeps W and L only, so a tie simply isn't counted.
 */
void RitSettleCallout(Rit& R, const world::World* W)
{
	Callout& C = R.C;
	if (C.Npc < 0 || C.Week < 0)
	{
		return;
	}
	const world::Npc* N = W ? W->Get(C.Npc) : nullptr;
	const world::LeagueMember* M = RitMember(RitBookOf(W, C.Week / 8), C.Npc);
	const double Them = (M ? RitMemberPoints(*M) : 0.0) - std::max(0.0f, C.NpcPts0);
	const double Me = RitHeroPoints(R.L) - static_cast<double>(C.HeroPts0);
	const int CallEdge = CalloutEdge(Me, Them);
	if (N && CallEdge != 0)
	{
		H2H& H = RitHeadFor(R, N->Name);
		H.W += CallEdge > 0 ? 1 : 0;
		H.L += CallEdge < 0 ? 1 : 0;
	}
	C = Callout();
}
} // namespace

// ------------------------------------------------------------------ the shared entry points (SitePlayPriv.h)

double RitHeroPoints(const League& Lg)
{
	return RitHeroPointsBefore(Lg, 1 << 30);
}

void RitPaidEntry(Book& B, life::State& L, Chips Fee, Chips BuyIn, const std::string& EventId, double World)
{
	(void)Fee;
	(void)BuyIn;
	(void)EventId;
	(void)L;
	(void)World;
	// The first paid RunItTwice entry after G2 makes a Placement month and asks for the group: the next step draws it (it
	// has the world), and the Session places the division by Abi100 first.
	League& Lg = B.Rt.L;
	if (Lg.Month == -1)
	{
		Lg.Month = -2;
		Lg.Placement = true;
	}
}

void RitResult(Book& B, life::State& L, const SettleFacts& F, bool Paid, double World)
{
	Rit& R = B.Rt;
	League& Lg = R.L;
	const int Day = PlayDay(World);
	const std::string Tmpl = SitePlayTemplate(F.EventId);
	const net::EventTemplate* T = RitTemplateOf(F.EventId);
	const bool Cash = F.Prize > 0;
	const bool Ft = F.Place >= 1 && (F.Final || site::show::FinalTableSeats(F.EventId) >= F.Place) && F.Entrants > 3;
	// Credentials are status, whatever paid for the entry.
	if (Cash)
	{
		RitCred(R, 0);
	}
	if (Ft)
	{
		RitCred(R, 1);
	}
	if (F.Place == 1)
	{
		RitCred(R, 2);
	}
	if (Ft && Tmpl == "sunday-100k")
	{
		RitCred(R, 3);
	}
	if (F.Place == 1 && T && T->Series.compare(0, 4, "open") == 0)
	{
		RitCred(R, 4);
	}
	if (const Account* A = AccountOf(L.Sites, net::SiteId::RunItTwice))
	{
		if (A->LifetimeEvents >= 100)
		{
			RitCred(R, 9);
		}
		if (A->LifetimeEvents >= 1000)
		{
			RitCred(R, 10);
		}
	}
	if (!Paid || Lg.Month != PlayMonth(Day))
	{
		return; // only paid entries score; a month not drawn yet scores nothing
	}
	// What this result scored, for the Desk's Results column and the Settlement (UI state: the sitting's, not saved).
	SitePlayScratch& Scratch = SitePlayScratchOf(B);
	if (Scratch.Scored.size() >= 60)
	{
		Scratch.Scored.erase(Scratch.Scored.begin());
	}
	Scratch.Scored.emplace_back(F.EventId, 0.0);
	// The League day (R2): the first k paid entries of the day at or above the division's floor, their best counts.
	const int Div = std::clamp<int>(Lg.Div, 0, 4);
	if (F.BuyIn < RitFloors[Div])
	{
		return;
	}
	if (Lg.TodayKey != Day)
	{
		Lg.TodayKey = Day;
		Lg.TodayCount = 0;
	}
	if (Lg.TodayCount >= std::clamp<int>(Lg.Pace, 1, 3))
	{
		return;
	}
	Lg.TodayCount = static_cast<int8_t>(Lg.TodayCount + 1);
	const double Raw = F.Points > 0.0 ? F.Points : net::Points(F.Place, F.Entrants, F.BuyIn);
	const bool Series = F.Series || (T && !T->Series.empty());
	const double Pts = LeagueScore(Div, Tmpl, F.BuyIn, Day, Raw, Series, Day == RitLastDayOf(Day));
	Scratch.Scored.back().second = Pts;
	if (Pts <= 0.0)
	{
		// A blank day still counts as a day played (the form's "blank").
		bool Seen = false;
		for (const auto& D : Lg.Days)
		{
			Seen = Seen || D.first == Day;
		}
		if (!Seen && Lg.Days.size() < 31)
		{
			Lg.Days.emplace_back(Day, 0.0f);
		}
		return;
	}
	bool Found = false;
	for (auto& D : Lg.Days)
	{
		if (D.first == Day)
		{
			D.second = std::max(D.second, static_cast<float>(Pts));
			Found = true;
		}
	}
	if (!Found && Lg.Days.size() < 31)
	{
		Lg.Days.emplace_back(Day, static_cast<float>(Pts));
	}
	// Bounce Back: a relegated month that cashes 3 days in Matchweek 1 starts 30 points up.
	int Dm = 0, Y = 0, M = 0;
	world::CivilDate(Day, Y, M, Dm);
	if (Lg.Bounce && Dm <= 7)
	{
		int Cashed = 0;
		bool Bonus = false;
		for (const auto& D : Lg.Days)
		{
			Bonus = Bonus || D.first < 0;
			Cashed += D.first >= 0 && D.second > 0.0f ? 1 : 0;
		}
		if (!Bonus && Cashed >= 3 && Lg.Days.size() < 32)
		{
			Lg.Days.emplace_back(-1, 30.0f);
		}
	}
}

void RitHand(Book& B, life::State* L, const HandNote& N, double World)
{
	Rit& R = B.Rt;
	// The Luck Ledger (R7): every all-in of the player's own in a paid entry, its equity against the result.
	if (N.AllIn && N.Equity >= 0.0)
	{
		const size_t Bucket = static_cast<size_t>(std::clamp(static_cast<int>(N.Equity * 10.0), 0, 9));
		R.Lk.AllIns += 1;
		R.Lk.ExpWinsX1000 += std::llround(N.Equity * 1000.0);
		R.Lk.Wins += N.Won ? 1 : 0;
		R.Lk.Buckets[Bucket] += 1;
		R.Lk.Buckets[10 + Bucket] += N.Won ? 1 : 0;
	}
	if (N.SecondVerdict >= 0)
	{
		R.Lk.SecondOf += 1;
		R.Lk.SecondWon += N.SecondVerdict == 2 ? 1 : 0;
	}
	if (!L)
	{
		return;
	}
	life::State& Lf = *L;
	const int Day = PlayDay(World);
	// Head to head with the group: busts each way.
	for (size_t K = 0; K < N.KnockedNpcs.size() && K < N.Knocked.size(); ++K)
	{
		if (std::find(R.L.Npcs.begin(), R.L.Npcs.end(), N.KnockedNpcs[K]) != R.L.Npcs.end() || N.KnockedNpcs[K] == R.C.Npc)
		{
			RitHeadFor(R, N.Knocked[K]).YouKo += 1;
		}
	}
	if (N.BustedByNpc >= 0 && (std::find(R.L.Npcs.begin(), R.L.Npcs.end(), N.BustedByNpc) != R.L.Npcs.end() || N.BustedByNpc == R.C.Npc))
	{
		RitHeadFor(R, N.BustedBy).TheyKo += 1;
	}
	// Team Scalps (R4): the year's six; once signed with the team, the player is the bounty and scalps are off.
	if (N.TeamPros.empty() || sponsor::Active(Lf.Sponsors, "turnstone", World))
	{
		return;
	}
	Scalps& Sc = R.Sc;
	const int Year = world::YearOf(Day);
	if (Sc.Year != Year)
	{
		Sc.Year = Year;
		Sc.Taken = 0;
		Sc.Set.fill(-1);
		for (size_t K = 0; K < N.TeamSix.size() && K < 6; ++K)
		{
			Sc.Set[K] = N.TeamSix[K];
		}
	}
	const int Week = PlayWeek(Day);
	if (Sc.WeekKey != Week)
	{
		Sc.WeekKey = Week;
		Sc.WeekCount = 0;
	}
	for (const int Pro : N.TeamPros)
	{
		auto Slot = std::find(Sc.Set.begin(), Sc.Set.end(), Pro);
		if (Slot == Sc.Set.end())
		{
			// A newly signed pro fills a departed pro's slot.
			for (auto It = Sc.Set.begin(); It != Sc.Set.end(); ++It)
			{
				if (*It < 0 || std::find(N.TeamSix.begin(), N.TeamSix.end(), *It) == N.TeamSix.end())
				{
					*It = Pro;
					Sc.Taken = static_cast<uint8_t>(Sc.Taken & ~(1u << (It - Sc.Set.begin())));
					Slot = It;
					break;
				}
			}
		}
		if (Slot == Sc.Set.end())
		{
			continue;
		}
		const uint8_t Bit = static_cast<uint8_t>(1u << (Slot - Sc.Set.begin()));
		if ((Sc.Taken & Bit) != 0)
		{
			continue;
		}
		SitePlayTouch(Lf);
		Sc.Taken = static_cast<uint8_t>(Sc.Taken | Bit);
		Sc.Lifetime += 1;
		if (Sc.WeekCount < 2)
		{
			Sc.WeekCount += 1;
			RitGrantTicket(B, Lf, Kind::ScalpTicket, RitScalpTicket(N.BuyIn), "scalp:" + std::to_string(Year) + ":" + std::to_string(Pro), Day, World, true);
		}
		if ((Sc.Taken & 0x3F) == 0x3F && Sc.SetPaidYear != Year)
		{
			Sc.SetPaidYear = Year;
			RitGrantTicket(B, Lf, Kind::ScalpTicket, RitFullSetTicket(R.L.Div), "fullset:" + std::to_string(Year), Day, World, true);
			RitCred(R, 8);
		}
	}
}

void RitStep(Book& B, life::State& L, const world::World* W, const sponsor::Book* Deals, uint64_t WorldSeed, double From, double To, bool End, StepOut& Out)
{
	(void)From;
	Rit& R = B.Rt;
	League& Lg = R.L;
	const int Day = PlayDay(To);
	const int Month = PlayMonth(Day);
	// The month (R2): the first draw a paid entry asked for, or the 1st: close the month that ended, then draw the new one.
	// (Forward only: an instant before the book's month, an overlapping window's, never draws an older month again.)
	if (Lg.Month == -2)
	{
		RitDraw(B, L, W, WorldSeed, Month, Day);
		Lg.Placement = true;
		RitRosterOut(Lg, Out);
	}
	else if (Lg.Month >= 0 && Lg.Month < Month)
	{
		// The last Matchweek's Callout settles on the month's own scores (the world still holds that month's group).
		if (R.C.Npc >= 0)
		{
			RitSettleCallout(R, W);
		}
		RitClose(B, L, W, Deals, To);
		Lg.Placement = false;
		RitDraw(B, L, W, WorldSeed, Month, Day);
		RitRosterOut(Lg, Out);
	}
	const bool Leave = RitOnLeaveNow(Deals);
	if (Lg.Month == Month)
	{
		bool Played = false;
		for (const auto& D : Lg.Days)
		{
			Played = Played || D.first >= 0;
		}
		// On leave for the month while a clause covers it and nothing's been played (RitClose reads it on the 1st).
		Lg.Leave = Lg.Leave || (Leave && !Played);
		RitCallouts(B, W, WorldSeed, Day, Leave);
	}
	// The desk bell (R1): a starred event projected $300 or more short within 15 minutes of late registration's close rings
	// once, keyed by its instance id; keys are pruned after 2 days. The Watchlist is its own (no League month needed: review),
	// read at the step's end only, so a skip's 6 AM instants never ring a bell for an event long gone.
	if (End)
	{
		RitBell(R, To);
	}
}

void RitCallouts(Book& B, const world::World* W, uint64_t WorldSeed, int Day, bool Leave)
{
	Rit& R = B.Rt;
	League& Lg = R.L;
	// The Callout's baseline (a call made without the world at hand), its end at the Matchweek's close, and the NPCs' own.
	// (Forward only, by the Matchweek's key: an earlier instant settles nothing and draws nothing.)
	const world::LeagueBook* Lb = RitBookOf(W, Lg.Month);
	if (R.C.Npc >= 0 && R.C.NpcPts0 < 0.0f)
	{
		const world::LeagueMember* M = RitMember(Lb, R.C.Npc);
		R.C.NpcPts0 = static_cast<float>(M ? RitMemberPoints(*M) : 0.0);
	}
	const int WeekKey = RitMatchweekKey(Day);
	if (R.C.Npc >= 0 && R.C.Week < WeekKey)
	{
		RitSettleCallout(R, W);
	}
	if (R.C.Npc < 0 && R.C.Week < WeekKey && !Leave && W)
	{
		const double Mine = RitHeroPoints(Lg);
		for (const int Npc : Lg.Npcs)
		{
			const world::Npc* N = W->Get(Npc);
			const world::LeagueMember* M = RitMember(Lb, Npc);
			if (!N || N->TraitOf(world::Trait::Aggro) < 0.6f)
			{
				continue;
			}
			const double Theirs = M ? RitMemberPoints(*M) : 0.0;
			if (std::fabs(Theirs - Mine) > 0.15 * std::max({Mine, Theirs, 10.0}))
			{
				continue;
			}
			if (SitePlayUnit(WorldSeed, "rit:callout:" + std::to_string(WeekKey) + ":" + std::to_string(Npc)) < 0.30)
			{
				R.C.Week = WeekKey;
				R.C.Npc = Npc;
				R.C.NpcPts0 = static_cast<float>(Theirs);
				R.C.HeroPts0 = static_cast<float>(Mine);
				R.C.ByThem = true;
				break;
			}
		}
		if (R.C.Npc < 0)
		{
			R.C.Week = WeekKey; // nobody called this Matchweek: the draw is made once a week (the player can still call)
			R.C.Npc = -1;
			R.C.ByThem = false;
		}
	}
}

void RitBell(Rit& R, double To)
{
	net::Network& Net = net::Site(net::SiteId::RunItTwice);
	for (const std::string& Star : R.Stars)
	{
		net::EventInstance E;
		if (!Net.Next("runittwice." + Star, To - 240.0, E))
		{
			continue;
		}
		const double Closes = E.Start + E.LateReg;
		if (To < Closes - 15.0 || To >= Closes)
		{
			continue;
		}
		bool Rung = false;
		for (const std::string& K : R.BellKeys)
		{
			Rung = Rung || K.compare(0, E.Id.size() + 1, E.Id + "@") == 0;
		}
		if (Rung)
		{
			continue;
		}
		const site::show::OverlayView Ov = site::show::OverlayOf(Net, E, To);
		if (!Ov.Actual && Ov.Cents >= 30000)
		{
			R.BellKeys.insert(E.Id + "@" + std::to_string(static_cast<long long>(std::llround(To))));
		}
	}
	for (auto It = R.BellKeys.begin(); It != R.BellKeys.end();)
	{
		const size_t At = It->rfind('@');
		const double When = At == std::string::npos ? 0.0 : std::atof(It->c_str() + At + 1);
		It = When + 2.0 * 1440.0 < To ? R.BellKeys.erase(It) : std::next(It);
	}
}

void RitBookSecondRunPay(Book& B, life::State& L, Chips Pay, int Earned, double MonthRake, double World)
{
	// The Second Run Club (§1.3, §1.4): SiteMoney.cpp pays the closed month's rakeback on the 1st (pinned, never held). The
	// Governor counts it in the slots of the month that earned it, pro rata by that month's daily rake (slots already out of
	// the ring drop with their rake), keyed by the month so it books once, whichever door brings it (core-sites-g2's
	// Grant(Kind::SecondRun) at the close, or the step's read of the wallet's line).
	const std::string Key = "play:sr:" + std::to_string(Earned);
	if (Pay <= 0 || MonthRake <= 0.0 || SitePlayDone(L.Sites, Key))
	{
		return;
	}
	SitePlayMarkDone(L, Key, World);
	SitePlayTouch(L);
	SitePlayRoll(B, net::SiteId::RunItTwice, std::max(PlayDay(World), B.Gov[2].Day0));
	GovRing& G = B.Gov[2];
	for (int D = G.Day0 - 29; D <= G.Day0; ++D)
	{
		if (PlayMonth(D) != Earned)
		{
			continue;
		}
		const size_t Slot = static_cast<size_t>(((D % 30) + 30) % 30);
		const double Share = static_cast<double>(G.Rake[Slot]) / static_cast<double>(ValuePerCent) / MonthRake;
		const int64_t Value = std::llround(std::min(1.0, Share) * static_cast<double>(Pay) * static_cast<double>(ValuePerCent));
		G.Promo[Slot] = static_cast<int32_t>(std::min<int64_t>(0x7fffffff, static_cast<int64_t>(G.Promo[Slot]) + Value));
	}
}

void RitBookSecondRun(Book& B, life::State& L, double From, double To)
{
	// The wallet's Second Run lines (the door for a tree where the close doesn't call Grant(Kind::SecondRun)).
	(void)From;
	const Account* A = AccountOf(L.Sites, net::SiteId::RunItTwice);
	if (!A)
	{
		return;
	}
	for (const Line& Ln : A->Lines)
	{
		if (Ln.Kind != 4 || Ln.Cents <= 0 || Ln.Label.compare(0, 17, "Second Run Club: ") != 0 || Ln.At > To)
		{
			continue;
		}
		const int Earned = PlayMonth(PlayDay(Ln.At) - 1);
		// The month's rake: the rate's tier is the account's VIP after the close (SecondRunTiers' per mille).
		const std::vector<TierInfo>& Tiers = SecondRunTiers();
		const int Permille = Tiers[static_cast<size_t>(std::clamp(A->Vip, 0, static_cast<int>(Tiers.size()) - 1))].Permille;
		const double MonthRake = Permille > 0 ? static_cast<double>(Ln.Cents) * 1000.0 / static_cast<double>(Permille) : 0.0;
		RitBookSecondRunPay(B, L, Ln.Cents, Earned, MonthRake, Ln.At);
	}
}

void RitFillView(const Book& B, const life::State& L, const sponsor::Book* Deals, const world::World* W, uint64_t WorldSeed, double World, play::RitView& Out)
{
	(void)WorldSeed;
	const Rit& R = B.Rt;
	const League& Lg = R.L;
	const int Day = PlayDay(World);
	const int Month = PlayMonth(Day);
	Out.On = true;
	Out.Started = SitePlayRitStarted(B);
	// On leave as RitClose decides it (Lg.Leave = !Played && (Lg.Leave || the clause)): the clause now, or this month already
	// marked on leave by RitStep (a deal that ended mid-month keeps the month's leave; review-ui-rit: the page told such a
	// member he could drop). The UI adds the "nothing played this month" half from Days. A Callout's gate is core's own
	// (RitCallOutBlocked: the clause now and nothing played), so a member who played under the deal can call.
	Out.OnLeave = RitOnLeaveNow(Deals) || (Lg.Month == Month && Lg.Leave);
	const bool NoCall = !RitCallOutBlocked(B, Deals, World).empty();
	Out.Bounty = sponsor::Active(L.Sponsors, "turnstone", World);
	Out.DrawPending = Lg.Month == -2;
	Out.League = Lg.Month == Month;
	Out.Month = Lg.Month;
	Out.Div = static_cast<int8_t>(std::clamp<int>(Lg.Div, 0, 4));
	Out.DivName = RitDivNames[Out.Div];
	Out.Group = Lg.Group;
	Out.Placement = Lg.Placement;
	Out.Bounce = Lg.Bounce;
	Out.Pace = Lg.Pace;
	Out.Hist = R.Hist;
	Out.NightSeen = RitSeenOf(L.Sites, RitSeenNight);
	Out.IncomingSeen = RitSeenOf(L.Sites, RitSeenCall);
	int Y = 0, M = 0, Dm = 0;
	world::CivilDate(Day, Y, M, Dm);
	const int Last = RitLastDayOf(Day);
	const int MonthDays = Last - world::MonthStart(Day) + 1;
	Out.Matchweek = (Dm - 1) / 7 + 1;
	Out.Matchweeks = (MonthDays + 6) / 7;
	Out.DeadlineAt = PlayDayStart(Last + 1);
	Out.DeadlineDay = Day == Last;
	for (const RitFixture& F : RitFixtures)
	{
		if (F.Div == Out.Div)
		{
			Out.Matchdays.emplace_back(F.Name, F.Weekday);
			if (F.Weekday == net::Weekday(Day))
			{
				Out.MatchdayTonight = F.Name;
			}
		}
	}
	if (Out.League)
	{
		const world::LeagueBook* Lb = RitBookOf(W, Lg.Month);
		std::vector<LeagueRowView> Standings;
		LeagueRowView Me;
		Me.Hero = true;
		Me.Points = RitHeroPoints(Lg);
		std::vector<double> MyBest;
		for (const auto& D : Lg.Days)
		{
			Me.Days += D.first >= 0 ? 1 : 0;
			Out.BounceBonus += D.first < 0 ? static_cast<double>(D.second) : 0.0;
			if (D.first >= 0)
			{
				MyBest.push_back(static_cast<double>(D.second));
				Out.HeroDays.emplace_back(D.first, static_cast<double>(D.second));
			}
		}
		std::sort(MyBest.begin(), MyBest.end(), [](double A, double C) { return A > C; });
		for (size_t K = 0; K < 8 && K < MyBest.size(); ++K)
		{
			Me.Best[K] = MyBest[K];
		}
		Out.Points = Me.Points;
		Out.Days = Me.Days;
		const double MeBefore = RitHeroPointsBefore(Lg, Day);
		std::vector<std::pair<double, double>> Scores; // now, before today
		Standings.push_back(Me);
		Scores.emplace_back(Me.Points, MeBefore);
		for (const int Npc : Lg.Npcs)
		{
			if (Npc < 0)
			{
				continue;
			}
			LeagueRowView Rv;
			Rv.Npc = Npc;
			if (const world::Npc* N = W ? W->Get(Npc) : nullptr)
			{
				Rv.Name = N->Name;
				Rv.Country = N->Country;
				Rv.Team = N->Sponsor == net::Def(net::SiteId::RunItTwice).TeamName;
			}
			const world::LeagueMember* Mb = RitMember(Lb, Npc);
			Rv.Points = Mb ? RitMemberPoints(*Mb) : 0.0;
			Rv.Days = Mb ? Mb->Days : 0;
			if (Mb)
			{
				std::vector<double> Theirs;
				for (const auto& P : Mb->Best)
				{
					if (P.second > 0)
					{
						Theirs.push_back(static_cast<double>(P.second) / 10.0);
					}
				}
				std::sort(Theirs.begin(), Theirs.end(), [](double A, double C) { return A > C; });
				for (size_t K = 0; K < 8 && K < Theirs.size(); ++K)
				{
					Rv.Best[K] = Theirs[K];
				}
			}
			for (int K = 0; K < 5; ++K)
			{
				Rv.Form[static_cast<size_t>(K)] = static_cast<int8_t>(Mb ? LeagueFormAt(Mb->Form, K) : 0);
			}
			Rv.Path[2] = Out.Div;
			Rv.Callout = R.C.Npc == Npc;
			Rv.CanCall = !NoCall && R.C.Npc < 0;
			Standings.push_back(Rv);
			Scores.emplace_back(Rv.Points, Mb ? RitMemberPointsBy(*Mb, Dm - 1) : 0.0);
		}
		// The hero's path: the last two closed months, then this one.
		for (size_t K = 0; K < 2 && K < R.Hist.size(); ++K)
		{
			Standings[0].Path[1 - K] = R.Hist[R.Hist.size() - 1 - K].Div;
		}
		Standings[0].Path[2] = Out.Div;
		// The hero's form: the last 5 days with a result, newest first.
		std::vector<std::pair<int, float>> Recent(Lg.Days.begin(), Lg.Days.end());
		std::sort(Recent.begin(), Recent.end(), [](const auto& A, const auto& C) { return A.first > C.first; });
		size_t F = 0;
		for (const auto& D : Recent)
		{
			if (D.first >= 0 && F < 5)
			{
				Standings[0].Form[F++] = static_cast<int8_t>(D.second > 0.0f ? 2 : 1);
			}
		}
		// Rank now and as of last night (ties: the player first, then by id).
		auto RankOf = [&](size_t I, bool Before) {
			int Ahead = 0;
			for (size_t J = 0; J < Standings.size(); ++J)
			{
				const double Mine = Before ? Scores[I].second : Scores[I].first;
				const double Theirs = Before ? Scores[J].second : Scores[J].first;
				if (J != I && (Theirs > Mine || (Theirs == Mine && (Standings[J].Hero || (!Standings[I].Hero && Standings[J].Npc < Standings[I].Npc)))))
				{
					++Ahead;
				}
			}
			return Ahead + 1;
		};
		const int Of = static_cast<int>(Standings.size());
		Out.Up = Of < 14 ? 2 : 3;
		Out.Down = Of < 14 ? 2 : 4;
		for (size_t I = 0; I < Standings.size(); ++I)
		{
			Standings[I].Rank = RankOf(I, false);
			Standings[I].Move = RankOf(I, true) - Standings[I].Rank;
			Standings[I].Zone = static_cast<int8_t>(Standings[I].Rank <= Out.Up ? 1 : Standings[I].Rank > Of - Out.Down && Out.Div > 0 && !Lg.Placement ? -1 : 0);
		}
		std::sort(Standings.begin(), Standings.end(), [](const LeagueRowView& A, const LeagueRowView& C) { return A.Rank < C.Rank; });
		for (const LeagueRowView& Rv : Standings)
		{
			if (Rv.Hero)
			{
				Out.Rank = Rv.Rank;
			}
		}
		Out.Of = Of;
		// What promotion needs (the points of the last promoted row, and a tenth more), what safety needs.
		std::vector<double> Others;
		for (const LeagueRowView& Rv : Standings)
		{
			if (!Rv.Hero)
			{
				Others.push_back(Rv.Points);
			}
		}
		std::sort(Others.begin(), Others.end(), [](double A, double C) { return A > C; });
		const size_t UpIdx = static_cast<size_t>(Out.Up - 1);
		Out.ToPromotion = Out.Rank <= Out.Up || UpIdx >= Others.size() ? 0.0 : std::max(0.0, Others[UpIdx] + 0.1 - Out.Points);
		const size_t SafeIdx = static_cast<size_t>(std::max(0, Of - Out.Down - 1));
		Out.ToSafety = Out.Rank <= Of - Out.Down || SafeIdx >= Others.size() ? 0.0 : std::max(0.0, Others[SafeIdx] + 0.1 - Out.Points);
		Out.Table = Standings;
	}
	// The Callout.
	Out.CalloutOn = R.C.Npc >= 0;
	Out.CalloutNpc = R.C.Npc;
	Out.CalloutByThem = R.C.ByThem;
	if (const world::Npc* N = W && R.C.Npc >= 0 ? W->Get(R.C.Npc) : nullptr)
	{
		Out.CalloutName = N->Name;
		const world::LeagueMember* Mb = RitMember(RitBookOf(W, Lg.Month), R.C.Npc);
		// At the League's tenth (CalloutEdge's unit): the baselines are floats, so a week with no points came out a hair under
		// zero and the card printed "-0.0" beside ALL SQUARE (review of G2 PIE P6).
		Out.CalloutThem = static_cast<double>(std::llround(((Mb ? RitMemberPoints(*Mb) : 0.0) - std::max(0.0f, R.C.NpcPts0)) * 10.0)) / 10.0;
		Out.CalloutHero = static_cast<double>(std::llround((RitHeroPoints(Lg) - static_cast<double>(R.C.HeroPts0)) * 10.0)) / 10.0;
	}
	Out.CanCallOut = Out.League && !NoCall && R.C.Npc < 0;
	Out.Heads = R.H;
	// The Watchlist: the next instance of each starred template.
	Out.Stars = R.Stars;
	net::Network& Net = net::Site(net::SiteId::RunItTwice);
	for (const std::string& Star : R.Stars)
	{
		net::EventInstance E;
		if (!Net.Next("runittwice." + Star, World - 15.0, E))
		{
			continue;
		}
		WatchView Wv;
		Wv.TemplateId = Star;
		Wv.EventId = E.Id;
		Wv.Start = E.Start;
		if (const net::EventTemplate* T = RitTemplateOf(Star))
		{
			Wv.Name = T->Name;
			Wv.BuyIn = T->BuyInCents;
		}
		for (const std::string& K : R.BellKeys)
		{
			if (K.compare(0, E.Id.size() + 1, E.Id + "@") == 0)
			{
				Wv.Bell = true;
				Wv.BellAt = std::atof(K.c_str() + E.Id.size() + 1);
			}
		}
		Out.Watch.push_back(Wv);
	}
	std::sort(Out.Watch.begin(), Out.Watch.end(), [](const WatchView& A, const WatchView& C) { return A.Start < C.Start; });
	// Team Scalps.
	const std::vector<int> Team = TeamPros(W);
	const bool ThisYear = R.Sc.Year == world::YearOf(Day);
	for (size_t K = 0; K < 6; ++K)
	{
		ScalpView& Sv = Out.Scalps[K];
		Sv.Npc = ThisYear ? R.Sc.Set[K] : (K < Team.size() ? Team[K] : -1);
		Sv.Taken = ThisYear && (R.Sc.Taken & (1u << K)) != 0;
		if (const world::Npc* N = W && Sv.Npc >= 0 ? W->Get(Sv.Npc) : nullptr)
		{
			Sv.Name = N->Name;
		}
		Out.Taken += Sv.Taken ? 1 : 0;
	}
	Out.FullSet = ThisYear && (R.Sc.Taken & 0x3F) == 0x3F;
	Out.WeekCount = R.Sc.WeekKey == PlayWeek(Day) ? R.Sc.WeekCount : 0;
	Out.Lifetime = R.Sc.Lifetime;
	// The Luck Ledger.
	LuckView& Lk = Out.Luck;
	Lk.AllIns = R.Lk.AllIns;
	Lk.Won = R.Lk.Wins;
	Lk.Expected = static_cast<double>(R.Lk.ExpWinsX1000) / 1000.0;
	Lk.GapPct = Lk.Expected > 0.0 ? (static_cast<double>(Lk.Won) - Lk.Expected) / Lk.Expected * 100.0 : 0.0;
	for (size_t K = 0; K < 10; ++K)
	{
		Lk.Of[K] = R.Lk.Buckets[K];
		Lk.Wins[K] = R.Lk.Buckets[10 + K];
		Lk.Exp[K] = static_cast<double>(R.Lk.Buckets[K]) * (static_cast<double>(K) + 0.5) / 10.0;
	}
	Lk.SecondWon = R.Lk.SecondWon;
	Lk.SecondOf = R.Lk.SecondOf;
	const std::string Pct = Fixed(std::fabs(Lk.GapPct), 1) + "%";
	Lk.Line = Lk.AllIns == 0 ? std::string() : std::fabs(Lk.GapPct) < 0.05 ? std::string(RitSay(RitSayEven))
		: FillCopy(RitSay(Lk.GapPct < 0.0 ? RitSayBelow : RitSayAbove), {{"Pct", Pct}});
	// Credentials.
	for (int K = 0; K < 11; ++K)
	{
		CredView Cv;
		Cv.Id = K;
		Cv.Name = RitCreds[K];
		Cv.Got = (R.Creds & (1u << K)) != 0;
		Out.Creds.push_back(Cv);
	}
	// The Desk's and the Settlement's League points by result (the sitting's), RANKINGS' PYRAMID, R4's ladder, R2's rewards.
	Out.Scored = SitePlayScratchOf(B).Scored;
	if (W)
	{
		const uint8_t Bit = static_cast<uint8_t>(1u << static_cast<int>(net::SiteId::RunItTwice));
		for (const world::Npc& N : W->People())
		{
			if ((N.Sites & Bit) == 0 || !N.Playing() || N.Faded)
			{
				continue;
			}
			for (int D = 0; D < 5; ++D)
			{
				Out.Members[static_cast<size_t>(D)] += RitTierFits(D, N.Tier, 0) ? 1 : 0;
			}
		}
	}
	Out.ScalpLadder = {{0, RitSupperStack.Label}, {10900, RitSweat.Label}, {30000, RitDeepstack.Label}};
	for (int D = 1; D < 5; ++D)
	{
		Out.Rewards[static_cast<size_t>(D)] = RitPromotionTicket(D - 1).Label;
	}
	// The Statement: Second Run as the wallet paid it, and each promotion's ticket, dated by the month that earned it.
	if (const Account* A = AccountOf(L.Sites, net::SiteId::RunItTwice))
	{
		for (const Line& Ln : A->Lines)
		{
			if (Ln.Kind == 4 && Ln.Cents > 0 && Ln.Label.compare(0, 17, "Second Run Club: ") == 0)
			{
				StatementRow Paid;
				Paid.Label = "Second Run Club";
				Paid.Month = Ln.Label.substr(17);
				Paid.Cents = Ln.Cents;
				Out.Statement.push_back(Paid);
			}
		}
	}
	for (const LeagueHist& H : R.Hist)
	{
		// (High Bridge's ticket goes to its promotion places: the top 2 in a group under 14, as RitClose pays it.)
		if (H.Move > 0 || (H.Div >= 4 && H.Rank <= (H.Of < 14 ? 2 : 3)))
		{
			StatementRow Promoted;
			Promoted.Label = "League ticket";
			Promoted.Month = MonthName(MonthKeyAt(PlayDayStart(world::DayOn(H.Month / 12, H.Month % 12 + 1, 1))));
			Promoted.Cents = RitPromotionTicket(H.Div).Face;
			Out.Statement.push_back(Promoted);
		}
	}
}

void RitWrite(const Book& B, std::string& Out)
{
	const Rit& R = B.Rt;
	if (!SitePlayRitStarted(B))
	{
		return;
	}
	if (!R.Stars.empty() || !R.BellKeys.empty())
	{
		std::string Stars;
		for (const std::string& S : R.Stars)
		{
			Stars += (Stars.empty() ? "" : ",") + SitePlayEscape(S);
		}
		std::string Bells;
		for (const std::string& K : R.BellKeys)
		{
			Bells += (Bells.empty() ? "" : ",") + SitePlayEscape(K);
		}
		Out += "site\tstar\trunittwice\t" + Stars + "\t" + Bells + "\n";
	}
	const League& Lg = R.L;
	if (Lg.Month != -1)
	{
		Out += "site\tleague\trunittwice\t" + std::to_string(Lg.Month) + "\t" + std::to_string(static_cast<int>(Lg.Div)) + "\t" + std::to_string(static_cast<int>(Lg.Group)) + "\t" +
			(Lg.Placement ? "1" : "0") + "\t" + (Lg.Leave ? "1" : "0") + "\t" + (Lg.Bounce ? "1" : "0");
		for (const int N : Lg.Npcs)
		{
			Out += "\t" + std::to_string(N);
		}
		std::string Days;
		for (const auto& D : Lg.Days)
		{
			Days += (Days.empty() ? "" : ",") + std::to_string(D.first) + ":" + SitePlayNumText(static_cast<double>(D.second));
		}
		Out += "\t" + Days + "\t" + std::to_string(Lg.TodayKey) + "\t" + std::to_string(static_cast<int>(Lg.TodayCount)) + "\t" + std::to_string(static_cast<int>(Lg.Pace)) + "\n";
	}
	for (const LeagueHist& H : R.Hist)
	{
		Out += "site\tlhist\trunittwice\t" + std::to_string(H.Month) + "\t" + std::to_string(static_cast<int>(H.Div)) + "\t" + std::to_string(H.Rank) + "\t" + std::to_string(H.Of) +
			"\t" + std::to_string(static_cast<int>(H.Move)) + "\n";
	}
	if (R.C.Week >= 0 || R.C.Npc >= 0)
	{
		Out += "site\tcallout\trunittwice\t" + std::to_string(R.C.Week) + "\t" + std::to_string(R.C.Npc) + "\t" + SitePlayNumText(static_cast<double>(R.C.NpcPts0)) + "\t" +
			SitePlayNumText(static_cast<double>(R.C.HeroPts0)) + "\t" + (R.C.ByThem ? "1" : "0") + "\n";
	}
	for (const H2H& H : R.H)
	{
		Out += "site\th2h\trunittwice\t" + SitePlayEscape(H.Name) + "\t" + std::to_string(H.W) + "\t" + std::to_string(H.L) + "\t" + std::to_string(H.TheyKo) + "\t" +
			std::to_string(H.YouKo) + "\n";
	}
	const Scalps& Sc = R.Sc;
	if (Sc.Year != 0 || Sc.Lifetime != 0)
	{
		Out += "site\tscalp\trunittwice\t" + std::to_string(Sc.Year);
		for (const int N : Sc.Set)
		{
			Out += "\t" + std::to_string(N);
		}
		Out += "\t" + std::to_string(static_cast<int>(Sc.Taken)) + "\t" + std::to_string(Sc.WeekKey) + "\t" + std::to_string(Sc.WeekCount) + "\t" + std::to_string(Sc.SetPaidYear) +
			"\t" + std::to_string(Sc.Lifetime) + "\n";
	}
	const Luck& Lk = R.Lk;
	if (Lk.AllIns != 0 || Lk.SecondOf != 0)
	{
		Out += "site\tluck\trunittwice\t" + std::to_string(Lk.AllIns) + "\t" + std::to_string(Lk.ExpWinsX1000) + "\t" + std::to_string(Lk.Wins);
		for (const int C : Lk.Buckets)
		{
			Out += "\t" + std::to_string(C);
		}
		Out += "\t" + std::to_string(Lk.SecondWon) + "\t" + std::to_string(Lk.SecondOf) + "\n";
	}
	if (R.Creds != 0)
	{
		Out += "site\tcred\trunittwice\t" + std::to_string(R.Creds) + "\n";
	}
}

bool RitRead(const std::vector<std::string>& P, Book& B)
{
	Rit& R = B.Rt;
	const std::string& Tag = P[1];
	if (Tag == "star" && P.size() >= 5)
	{
		for (const std::string& S : SitePlaySplit(P[3], ','))
		{
			R.Stars.push_back(SitePlayUnescape(S));
		}
		for (const std::string& K : SitePlaySplit(P[4], ','))
		{
			R.BellKeys.insert(SitePlayUnescape(K));
		}
		return true;
	}
	if (Tag == "league" && P.size() >= 3 + 6 + 19 + 4)
	{
		League& Lg = R.L;
		Lg.Month = static_cast<int>(SitePlayInt(P[3]));
		Lg.Div = static_cast<int8_t>(SitePlayInt(P[4]));
		Lg.Group = static_cast<int8_t>(SitePlayInt(P[5]));
		Lg.Placement = P[6] == "1";
		Lg.Leave = P[7] == "1";
		Lg.Bounce = P[8] == "1";
		for (size_t K = 0; K < 19; ++K)
		{
			Lg.Npcs[K] = static_cast<int>(SitePlayInt(P[9 + K]));
		}
		Lg.Days.clear();
		for (const std::string& D : SitePlaySplit(P[28], ','))
		{
			const size_t Colon = D.find(':');
			if (Colon != std::string::npos)
			{
				Lg.Days.emplace_back(static_cast<int>(SitePlayInt(D.substr(0, Colon))), static_cast<float>(SitePlayNum(D.substr(Colon + 1))));
			}
		}
		Lg.TodayKey = static_cast<int>(SitePlayInt(P[29]));
		Lg.TodayCount = static_cast<int8_t>(SitePlayInt(P[30]));
		Lg.Pace = static_cast<int8_t>(SitePlayInt(P[31]));
		return true;
	}
	if (Tag == "lhist" && P.size() >= 8)
	{
		LeagueHist H;
		H.Month = static_cast<int>(SitePlayInt(P[3]));
		H.Div = static_cast<int8_t>(SitePlayInt(P[4]));
		H.Rank = static_cast<int>(SitePlayInt(P[5]));
		H.Of = static_cast<int>(SitePlayInt(P[6]));
		H.Move = static_cast<int8_t>(SitePlayInt(P[7]));
		R.Hist.push_back(H);
		return true;
	}
	if (Tag == "callout" && P.size() >= 8)
	{
		R.C.Week = static_cast<int>(SitePlayInt(P[3]));
		R.C.Npc = static_cast<int>(SitePlayInt(P[4]));
		R.C.NpcPts0 = static_cast<float>(SitePlayNum(P[5]));
		R.C.HeroPts0 = static_cast<float>(SitePlayNum(P[6]));
		R.C.ByThem = P[7] == "1";
		return true;
	}
	if (Tag == "h2h" && P.size() >= 8)
	{
		H2H H;
		H.Name = SitePlayUnescape(P[3]);
		H.W = static_cast<int>(SitePlayInt(P[4]));
		H.L = static_cast<int>(SitePlayInt(P[5]));
		H.TheyKo = static_cast<int>(SitePlayInt(P[6]));
		H.YouKo = static_cast<int>(SitePlayInt(P[7]));
		R.H.push_back(H);
		return true;
	}
	if (Tag == "scalp" && P.size() >= 3 + 1 + 6 + 5)
	{
		Scalps& Sc = R.Sc;
		Sc.Year = static_cast<int>(SitePlayInt(P[3]));
		for (size_t K = 0; K < 6; ++K)
		{
			Sc.Set[K] = static_cast<int>(SitePlayInt(P[4 + K]));
		}
		Sc.Taken = static_cast<uint8_t>(SitePlayInt(P[10]));
		Sc.WeekKey = static_cast<int>(SitePlayInt(P[11]));
		Sc.WeekCount = static_cast<int>(SitePlayInt(P[12]));
		Sc.SetPaidYear = static_cast<int>(SitePlayInt(P[13]));
		Sc.Lifetime = static_cast<int>(SitePlayInt(P[14]));
		return true;
	}
	if (Tag == "luck" && P.size() >= 3 + 3 + 20 + 2)
	{
		Luck& Lk = R.Lk;
		Lk.AllIns = static_cast<int>(SitePlayInt(P[3]));
		Lk.ExpWinsX1000 = SitePlayInt(P[4]);
		Lk.Wins = static_cast<int>(SitePlayInt(P[5]));
		for (size_t K = 0; K < 20; ++K)
		{
			Lk.Buckets[K] = static_cast<int>(SitePlayInt(P[6 + K]));
		}
		Lk.SecondWon = static_cast<int>(SitePlayInt(P[26]));
		Lk.SecondOf = static_cast<int>(SitePlayInt(P[27]));
		return true;
	}
	if (Tag == "cred" && P.size() >= 4)
	{
		R.Creds = static_cast<uint32_t>(SitePlayInt(P[3]));
		return true;
	}
	return false;
}

void RitCopy(std::vector<std::string>& Sentences, std::vector<std::string>& Labels)
{
	for (const char* S : RitSentences)
	{
		Sentences.push_back(S);
	}
	for (const char* S : RitLabels)
	{
		Labels.push_back(S);
	}
	for (const char* S : RitDivNames)
	{
		Labels.push_back(S);
	}
	for (const char* S : RitCreds)
	{
		Labels.push_back(S);
	}
	for (const RitFixture& F : RitFixtures)
	{
		Labels.push_back(F.Name);
	}
	for (const RitTicketDef* T : {&RitSupperStack, &RitSweat, &RitGrind, &RitFiftyFive, &RitHundredK, &RitDeepstack})
	{
		Labels.push_back(T->Label);
	}
}

std::string RitDescribe(const Book& B, const world::World* W, double World)
{
	const Rit& R = B.Rt;
	const League& Lg = R.L;
	std::string S = "RUNITTWICE " + std::string(SitePlayRitStarted(B) ? "" : "(no book yet) ") + "League month " + std::to_string(Lg.Month) + " " + RitDivNames[std::clamp<int>(Lg.Div, 0, 4)] +
		" group " + std::to_string(static_cast<int>(Lg.Group)) + (Lg.Placement ? " PLACEMENT" : "") + (Lg.Leave ? " ON LEAVE" : "") + (Lg.Bounce ? " BOUNCE" : "") + ", pace " +
		std::to_string(static_cast<int>(Lg.Pace)) + ", " + Fixed(RitHeroPoints(Lg), 1) + " pts over " + std::to_string(Lg.Days.size()) + " days\n";
	const world::LeagueBook* Lb = RitBookOf(W, Lg.Month);
	if (Lg.Month >= 0)
	{
		const std::pair<int, int> Place = RitRank(Lg, W);
		S += "  rank " + std::to_string(Place.first) + " of " + std::to_string(Place.second) + (Lb ? " (the world's recorded scores)" : " (no world line yet)") + "\n";
		for (const int N : Lg.Npcs)
		{
			if (N < 0)
			{
				continue;
			}
			const world::Npc* P = W ? W->Get(N) : nullptr;
			const world::LeagueMember* M = RitMember(Lb, N);
			S += "    " + (P ? P->Name : std::to_string(N)) + " " + Fixed(M ? RitMemberPoints(*M) : 0.0, 1) + " (" + std::to_string(M ? M->Days : 0) + " days)\n";
		}
	}
	S += "  callout week " + std::to_string(R.C.Week) + " npc " + std::to_string(R.C.Npc) + (R.C.ByThem ? " (by them)" : "") + "; h2h " + std::to_string(R.H.size()) + "; history " +
		std::to_string(R.Hist.size()) + "\n";
	S += "  scalps " + std::to_string(R.Sc.Year) + ": taken bits " + std::to_string(static_cast<int>(R.Sc.Taken)) + ", lifetime " + std::to_string(R.Sc.Lifetime) + "; luck " +
		std::to_string(R.Lk.AllIns) + " all-ins, " + Fixed(static_cast<double>(R.Lk.ExpWinsX1000) / 1000.0, 2) + " expected, " + std::to_string(R.Lk.Wins) + " won; creds " +
		std::to_string(R.Creds) + "\n";
	S += "  stars " + std::to_string(R.Stars.size()) + ", bells " + std::to_string(R.BellKeys.size()) + " (today " + std::to_string(PlayDay(World)) + ")\n";
	return S;
}

std::string RitCallOutBlocked(const Book& B, const sponsor::Book* Deals, double World)
{
	// On leave (§1.5, R2): a clause strikes RunItTwice and no RunItTwice day has been played this month (a member who played
	// under the deal took the strike and is in the month's contest, Callouts included).
	const League& Lg = B.Rt.L;
	bool Played = false;
	for (const auto& D : Lg.Days)
	{
		Played = Played || D.first >= 0;
	}
	Played = Played && Lg.Month == PlayMonth(PlayDay(World));
	return RitOnLeaveNow(Deals) && !Played ? std::string(RitSay(RitSayLeave)) : std::string();
}

std::string RitCallOut(Book& B, int Npc, double World)
{
	Rit& R = B.Rt;
	const int Day = PlayDay(World);
	if (R.L.Month != PlayMonth(Day))
	{
		return RitSay(RitSayNoGroup);
	}
	if (std::find(R.L.Npcs.begin(), R.L.Npcs.end(), Npc) == R.L.Npcs.end() || Npc < 0)
	{
		return RitSay(RitSayNotMember);
	}
	const int Week = RitMatchweekKey(Day);
	if (R.C.Npc >= 0)
	{
		return RitSay(RitSayCalled); // one a Matchweek: a Callout runs to the week's end
	}
	R.C.Week = Week;
	R.C.Npc = Npc;
	R.C.NpcPts0 = -1.0f; // the step (or the Session) reads the member's points now
	R.C.HeroPts0 = static_cast<float>(RitHeroPoints(R.L));
	R.C.ByThem = false;
	return std::string();
}

std::string RitStar(Book& B, const std::string& TemplateId, bool On)
{
	Rit& R = B.Rt;
	const std::string T = SitePlayTemplate(TemplateId);
	if (!RitTemplateOf(T))
	{
		return RitSay(RitSayNoEvent);
	}
	const auto It = std::find(R.Stars.begin(), R.Stars.end(), T);
	if (On)
	{
		if (It != R.Stars.end())
		{
			return RitSay(RitSayStarred);
		}
		if (R.Stars.size() >= 12)
		{
			return RitSay(RitSayStarsFull);
		}
		R.Stars.push_back(T);
		return std::string();
	}
	if (It == R.Stars.end())
	{
		return RitSay(RitSayNotStarred);
	}
	R.Stars.erase(It);
	return std::string();
}

std::string RitDebug(Book& B, life::State& L, const world::World* W, const sponsor::Book* Deals, uint64_t WorldSeed, const std::string& Verb, int Npc, double World,
	StepOut& Out)
{
	Rit& R = B.Rt;
	const int Day = PlayDay(World);
	SitePlayTouch(L);
	if (Verb == "league-draw")
	{
		const bool Was = R.L.Month == PlayMonth(Day);
		RitDraw(B, L, W, WorldSeed, PlayMonth(Day), Day);
		R.L.Placement = !Was || R.L.Placement;
		RitRosterOut(R.L, Out);
		return "drew " + std::string(RitDivNames[std::clamp<int>(R.L.Div, 0, 4)]) + " group " + std::to_string(static_cast<int>(R.L.Group)) + " (" + std::to_string(Out.Npcs.size()) +
			" members, pace " + std::to_string(static_cast<int>(R.L.Pace)) + ")";
	}
	if (Verb == "league-close")
	{
		if (R.L.Month < 0)
		{
			return RitSay(RitSayNoGroup);
		}
		RitClose(B, L, W, Deals, World);
		const LeagueHist& H = R.Hist.back();
		R.L.Placement = false;
		RitDraw(B, L, W, WorldSeed, PlayMonth(Day), Day);
		RitRosterOut(R.L, Out);
		return "closed: " + std::to_string(H.Rank) + " of " + std::to_string(H.Of) + ", move " + std::to_string(static_cast<int>(H.Move)) + "; now " + RitDivNames[std::clamp<int>(R.L.Div, 0, 4)];
	}
	if (Verb == "scalp")
	{
		HandNote N;
		N.Paid = true;
		N.TeamSix = TeamPros(W);
		N.TeamPros = {Npc};
		N.BuyIn = 1100;
		RitHand(B, &L, N, World);
		return "scalp noted for " + std::to_string(Npc) + " (taken bits " + std::to_string(static_cast<int>(R.Sc.Taken)) + ")";
	}
	return "unknown verb";
}
} // namespace siteplay_detail

// ------------------------------------------------------------------ the public helpers (SitePlay.h)

bool SeeLeagueNight(life::State& L, int Month, double World)
{
	return siteplay_detail::RitSee(L, siteplay_detail::RitSeenNight, Month, World);
}

bool SeeIncomingCallout(life::State& L, int Week, double World)
{
	return siteplay_detail::RitSee(L, siteplay_detail::RitSeenCall, Week, World);
}

const char* DivisionName(int Div)
{
	return siteplay_detail::RitDivNames[std::clamp(Div, 0, 4)];
}

Chips DivisionFloor(int Div)
{
	return siteplay_detail::RitFloors[std::clamp(Div, 0, 4)];
}

double MatchdayMult(int Div, const std::string& TemplateId, int Day)
{
	using namespace siteplay_detail;
	const std::string T = SitePlayTemplate(TemplateId);
	const int Wd = net::Weekday(Day);
	for (const RitFixture& F : RitFixtures)
	{
		if (F.Div == Div && F.Weekday == Wd && T == F.Template)
		{
			return 1.5;
		}
	}
	return 1.0;
}

double LeagueScore(int Div, const std::string& TemplateId, Chips BuyIn, int WorldDay, double RawPoints, bool Series, bool Deadline)
{
	using namespace siteplay_detail;
	// One rule for the player and the world's members (R2): 0 below the division's floor; the division's Matchdays x1.5; a
	// RunItTwice series x1.25; Deadline Day's featured events x1.25. WorldDay is the play day (PlayDay of the result's minute).
	// Rounded to a tenth, the world's unit (points x 10 as integers), so both sides keep the same number.
	const int D = std::clamp(Div, 0, 4);
	if (BuyIn < RitFloors[D] || RawPoints <= 0.0)
	{
		return 0.0;
	}
	double Mult = MatchdayMult(D, TemplateId, WorldDay);
	Mult *= Series ? 1.25 : 1.0;
	const net::EventTemplate* T = RitTemplateOf(SitePlayTemplate(TemplateId));
	Mult *= Deadline && T && T->Featured ? 1.25 : 1.0;
	return std::round(RawPoints * Mult * 10.0) / 10.0;
}

int CalloutEdge(double Hero, double Them)
{
	// Both sides' points are tenths (LeagueScore's unit, the world's points x 10) carried in floats: compared as whole tenths.
	const long long HeroTenths = std::llround(Hero * 10.0);
	const long long ThemTenths = std::llround(Them * 10.0);
	return HeroTenths > ThemTenths ? 1 : HeroTenths < ThemTenths ? -1 : 0;
}

std::vector<int> TeamPros(const world::World* W)
{
	std::vector<int> Out;
	if (!W)
	{
		return Out;
	}
	const std::string& Team = net::Def(net::SiteId::RunItTwice).TeamName;
	for (const world::Npc& N : W->People())
	{
		if (!Team.empty() && N.Sponsor == Team && N.Playing())
		{
			Out.push_back(N.Id);
		}
	}
	return Out;
}

int PlacementDivision(Chips Abi100)
{
	return Abi100 < 550 ? 0 : Abi100 < 1100 ? 1 : Abi100 < 2200 ? 2 : Abi100 < 5500 ? 3 : 4;
}
} // namespace play
} // namespace site
} // namespace ss
