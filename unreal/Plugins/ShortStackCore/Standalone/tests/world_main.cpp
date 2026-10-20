// The living world on its own: invariants, save and load, and long runs.
//
//   world_test                     the checks (ctest)
//   world_test years N [seed]      plays N years and prints the world's health every year
//   world_test npc NAME [days]     plays some days and describes someone
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/World.h"

#include "CopyLint.h"
#include "WorldSeg.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <sstream>
#include <string>

using namespace ss;

namespace
{
int Failures = 0;

void Check(bool Ok, const std::string& What)
{
	if (!Ok)
	{
		++Failures;
		std::printf("FAIL: %s\n", What.c_str());
	}
}

std::vector<std::string> Tabs(const std::string& Line)
{
	std::vector<std::string> Out;
	std::string Cur;
	for (char C : Line)
	{
		if (C == '\t')
		{
			Out.push_back(Cur);
			Cur.clear();
		}
		else
		{
			Cur += C;
		}
	}
	Out.push_back(Cur);
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

double Seconds(std::chrono::steady_clock::time_point Since)
{
	return std::chrono::duration<double>(std::chrono::steady_clock::now() - Since).count();
}

const double NightOne = 1440.0 + 127.0;

/** Everyone's numbers make sense. */
void Invariants(const world::World& W, const std::string& When)
{
	int Active = 0;
	std::set<std::string> Names;
	for (const world::Npc& N : W.People())
	{
		Active += N.Playing() ? 1 : 0;
		Check(N.Bankroll >= 0, When + ": " + N.Name + " has a negative bankroll");
		Check(!N.Name.empty() && Names.insert(N.Name).second, When + ": duplicate or empty name " + N.Name);
		for (float S : N.Skills)
		{
			Check(S > 0.0f && S < 1.0f && std::isfinite(S), When + ": " + N.Name + " skill out of range");
		}
		for (const world::Ledger& L : N.Totals)
		{
			Check(L.Wins <= L.FinalTables + 1 && L.Cashes >= 0 && L.Won >= 0 && L.Events >= 0, When + ": " + N.Name + " ledger is inconsistent");
		}
		Check(N.Tier >= 1 && N.Tier <= 4, When + ": " + N.Name + " tier");
		Check(N.Ties.size() <= 7, When + ": " + N.Name + " too many ties");
		Check(N.Recent.size() <= 10, When + ": " + N.Name + " too many recent results");
		int Rings = 0;
		for (const world::Award& A : N.Awards)
		{
			Rings += A.Ring ? 1 : 0;
		}
		Check(Rings == N.Rings && static_cast<int>(N.Awards.size()) - Rings == N.Bracelets, When + ": " + N.Name + "'s trophy case holds every bracelet and ring");
		if (!N.Faded)
		{
			// The stats page: every tournament (online and live), every buy-in, every prize (and seats at their value).
			const world::Tracker& T = N.Stats;
			const world::Ledger& On = N.Totals[0];
			const world::Ledger& Lv = N.Totals[1];
			Check(T.Events == On.Events + Lv.Events && T.BuyIns == On.Spent + Lv.Spent && T.Prizes >= On.Won + Lv.Won, When + ": " + N.Name + "'s stats page adds up");
			int Lines = 0;
			for (const world::TrackLine& L : T.ByStake)
			{
				Lines += L.Events;
			}
			Check(Lines == T.Events && T.Cashes <= T.Events && T.Wins <= T.FinalTables && static_cast<int>(T.Curve.size()) < world::Tracker::CurveMax && T.Net == T.Prizes - T.BuyIns,
				When + ": " + N.Name + "'s stats page is consistent");
			bool Rising = T.Spend.size() == T.Curve.size();
			for (size_t K = 1; Rising && K < T.Spend.size(); ++K)
			{
				Rising = T.Spend[K] >= T.Spend[K - 1];
			}
			Check(Rising && (T.Spend.empty() || T.Spend.back() <= T.BuyIns + 100), When + ": " + N.Name + "'s ABI graph adds up");
		}
	}
	Check(Active > 1000, When + ": the world emptied out (" + std::to_string(Active) + " active)");
	Check(W.View().size() == W.People().size(), When + ": the network view is out of step");
	// The badges' world data (next_segment_spec §5.3).
	for (const world::Npc& N : W.People())
	{
		const world::RoomRecord& R = N.Room;
		bool Ordered = true;
		for (size_t K = 1; K < R.Days.size(); ++K)
		{
			Ordered = Ordered && R.Days[K] > R.Days[K - 1];
		}
		Ordered = Ordered && (R.Days.empty() || R.Days.back() - R.Days.front() < world::RoomDaysKept);
		Check(Ordered && R.Wins <= R.Events && R.Cashes <= R.Events && R.FinalTables <= R.Events && static_cast<int>(R.Days.size()) <= R.Events,
			When + ": " + N.Name + "'s room record is consistent (a visit a day, kept " + std::to_string(world::RoomDaysKept) + " days)");
		Check(N.CardBest >= world::CardNone && N.CardBest <= world::CardBlack && N.CardLast >= world::CardNone && N.CardLast <= world::CardBlack, When + ": " + N.Name + "'s cards");
		// Live Player of the Year and the circuit count the same events (Regional and up): the same points all season.
		Check(N.ThisSeason.LivePoints == N.CircuitPoints, When + ": " + N.Name + "'s live points are the circuit's");
		Check(N.Pro == (N.Sponsor == "Team RiverLine") && N.Sponsor != "RiverLine", When + ": " + N.Name + " wears the site's patch only as one of its ten");
	}
	Check(W.HeroSeasonLivePoints() == W.HeroCircuitPoints(), When + ": the player's live points are the circuit's");
}

/** RiverLine's online calendar: a series in every season of every year, each name used once. */
void Calendar()
{
	const net::Network& Net = net::Shared();
	std::vector<const net::SeriesInfo*> All;
	std::set<std::string> SeriesNames;
	std::map<int, int> PerYear;
	for (const net::SeriesInfo& Sr : Net.Series())
	{
		All.push_back(&Sr);
		Check(SeriesNames.insert(Sr.Name).second, "a series name is used once: " + Sr.Name);
		++PerYear[world::YearOf(Sr.FirstDay)];
		const net::EventTemplate* Main = Net.FindTemplate(Sr.MainEvent);
		Check(Main && Main->Series == Sr.Id, Sr.Name + " has its Main Event");
		int Events = 0;
		int Awards = 0;
		for (const net::EventTemplate& T : Net.Templates())
		{
			if (T.Series == Sr.Id)
			{
				++Events;
				Awards += T.Bracelet || T.Ring ? 1 : 0;
				Check(T.OnlyDay >= Sr.FirstDay && T.OnlyDay <= Sr.LastDay, T.Name + " is inside its series");
			}
		}
		// (2026's three were written by hand: Summer Slam's events are history, only its Main Event is kept.)
		Check(Events == Sr.Events || Sr.Id == "mm" || Sr.Id == "rcop" || Sr.Id == "slam", Sr.Name + " lists its events");
		Check(Awards == Sr.Bracelets + Sr.Rings, Sr.Name + " counts its rings and bracelets");
		if (Sr.Short == "TCO")
		{
			Check(Sr.Bracelets >= 30, Sr.Name + " has its bracelet events");
		}
		if (Sr.Short == "RING")
		{
			Check(Sr.Rings >= 20, Sr.Name + " has its ring events");
		}
	}
	for (int Y = 2027; Y <= 2040; ++Y)
	{
		Check(PerYear[Y] == 9, "nine series in " + std::to_string(Y));
	}
	Check(PerYear[2026] >= 4, "December 2026 has a series of its own");
	std::sort(All.begin(), All.end(), [](const net::SeriesInfo* A, const net::SeriesInfo* B) { return A->FirstDay < B->FirstDay; });
	for (size_t K = 1; K < All.size(); ++K)
	{
		Check(All[K]->FirstDay > All[K - 1]->LastDay, All[K]->Name + " doesn't overlap " + All[K - 1]->Name);
	}
	// Every event in every series has a name of its own; every template its own id.
	std::set<std::string> Names;
	std::set<std::string> Ids;
	int SeriesEvents = 0;
	for (const net::EventTemplate& T : Net.Templates())
	{
		Check(Ids.insert(T.Id).second, "a template id is used once: " + T.Id);
		if (!T.Series.empty())
		{
			++SeriesEvents;
			Check(Names.insert(T.Name).second, "an event name is used once: " + T.Name);
		}
	}
	// Seats into the RCOP Main carry over to every year's Main Event.
	Check(Net.TicketOf("rcop27-main") == "rcop-main", "every RCOP Main takes an RCOP Main seat");
	net::EventInstance Next;
	Check(Net.NextFor("rcop-main", static_cast<double>(world::YearStart(2027)) * net::MinutesPerDay, Next) && Net.TemplateOf(Next).Id == "rcop27-main",
		"a seat won in 2027 is for RCOP 2027");
	Check(net::DateLabel(world::DayOn(2028, 2, 29)) == "Feb 29" && net::DateLabel(world::DayOn(2028, 3, 1)) == "Mar 1", "leap days are on the calendar");
	std::printf("calendar: %zu series, %d series events, %zu templates\n", All.size(), SeriesEvents, Net.Templates().size());
}

/** The Embercrest's card room over four weeks: the schedule, its crowds, and the cast by habit and money. */
void EmbercrestWeeks(const world::World& Base)
{
	world::World W = Base;
	const int First = net::DayOf(W.Clock()) + 1;
	std::map<int, int> PerWeekday;
	std::set<std::string> Ids;
	std::vector<std::set<std::string>> SundayFaces;
	std::set<std::string> CastSeen;
	int Doubles = 0;
	int Tracked = 0;
	// The room's regulars: who they are, how often they come, how many a night.
	const std::vector<int> Regulars = W.EmbercrestRegulars();
	const std::set<int> RegularSet(Regulars.begin(), Regulars.end());
	std::map<int, int> RegularNights;
	int NightlyRegulars = 0;
	int Nightlies = 0;
	for (int D = First; D < First + 28; ++D)
	{
		W.EnsurePlanned(D);
		const std::vector<live::Occurrence> Day = live::Occurrences(D);
		PerWeekday[((D % 7) + 7) % 7] += static_cast<int>(Day.size());
		std::map<int, int> Seen;
		for (const live::Occurrence& O : Day)
		{
			Check(Ids.insert(O.Id).second, "an occurrence id is used once: " + O.Id);
			const int Entries = W.PlannedEntries(O.Id);
			Check(Entries >= 60 && Entries <= 120, O.Id + " plans 60 to 120 runners (" + std::to_string(Entries) + ")");
			std::set<std::string> Faces;
			for (int Id : W.Registered(O.Id))
			{
				++Seen[Id];
				++Tracked;
				if (RegularSet.count(Id) > 0)
				{
					++RegularNights[Id];
					NightlyRegulars += std::string(O.T->Key) == "nightly" ? 1 : 0;
				}
				const world::Npc* N = W.Get(Id);
				Check(N && N->Playing(), "the Embercrest registered someone who isn't playing");
				for (const live::CastMember& C : live::EmbercrestCast())
				{
					if (N && N->Name == C.Name)
					{
						Faces.insert(N->Name);
						CastSeen.insert(N->Name);
					}
				}
			}
			if (std::string(O.T->Key) == "sunday")
			{
				SundayFaces.push_back(Faces);
			}
			Nightlies += std::string(O.T->Key) == "nightly" ? 1 : 0;
		}
		for (const auto& It : Seen)
		{
			Doubles += It.second > 1 ? 1 : 0;
		}
		W.Simulate(1);
	}
	Check(PerWeekday[0] == 8 && PerWeekday[1] == 8 && PerWeekday[2] == 8 && PerWeekday[3] == 8 && PerWeekday[4] == 12 && PerWeekday[5] == 12 && PerWeekday[6] == 8,
		"two events Monday to Thursday and Sunday, three Friday and Saturday");
	Check(Doubles == 0, "nobody plays two of the Embercrest's events in a day");
	bool Varies = false;
	for (size_t K = 1; K < SundayFaces.size(); ++K)
	{
		Varies = Varies || SundayFaces[K] != SundayFaces[0];
	}
	Check(SundayFaces.size() == 4 && Varies, "the Sunday's faces change from week to week");
	Check(CastSeen.size() >= 6, "most of the cast play the Embercrest in a month (" + std::to_string(CastSeen.size()) + ")");
	std::printf("embercrest: 4 weeks, %zu events, %d registrations the world follows, %zu of the cast seen\n", Ids.size(), Tracked, CastSeen.size());
	// The regulars: a room's worth, the same people a month later, and back week after week.
	int Recurring = 0;
	for (const auto& It : RegularNights)
	{
		Recurring += It.second >= 3 ? 1 : 0;
	}
	const std::vector<int> Later = W.EmbercrestRegulars();
	int Stayed = 0;
	for (int Id : Later)
	{
		Stayed += RegularSet.count(Id) > 0 ? 1 : 0;
	}
	const double PerNightly = Nightlies > 0 ? static_cast<double>(NightlyRegulars) / Nightlies : 0.0;
	Check(static_cast<int>(Regulars.size()) == world::World::EmbercrestRegularCount, "the Embercrest has its regulars (" + std::to_string(Regulars.size()) + ")");
	Check(Stayed >= static_cast<int>(Regulars.size()) * 9 / 10, "the regulars are mostly the same people a month later (" + std::to_string(Stayed) + ")");
	Check(PerNightly >= 5.0, "a Nightly has its regulars (" + std::to_string(PerNightly) + " a night)");
	Check(Recurring >= static_cast<int>(Regulars.size()) / 2, "most regulars are back three or more times a month (" + std::to_string(Recurring) + ")");
	std::printf("embercrest: %zu regulars, %.1f at a Nightly, %d back three or more times in four weeks, %d still regulars a month on\n", Regulars.size(), PerNightly, Recurring, Stayed);
}

/** The minimum field (R6): a thin night is called off when its cards should go in the air; nobody plays or pays for it, and it never has a winner. */
void MinimumField(const world::World& Base)
{
	world::World W = Base;
	const int Tomorrow = net::DayOf(W.Clock()) + 1;
	W.EnsurePlanned(Tomorrow);
	const std::vector<live::Occurrence> Day = live::Occurrences(Tomorrow);
	const live::Occurrence Full = Day.front();
	const live::Occurrence Thin = Day.back();
	W.SetPlannedEntries(Thin.Id, 12);
	const std::vector<int> Coming = W.Registered(Thin.Id);
	Check(W.PlannedEntries(Thin.Id) == 12 && Coming.size() <= 12, "a thin night: twelve coming");
	W.AdvanceTo(Thin.Start - 1.0);
	Check(W.CancellationOf(Thin.Id) == nullptr && W.PlannedEntries(Thin.Id) == 12, "nothing is called off before cards");
	std::map<int, world::Ledger> Before;
	for (int Id : Coming)
	{
		Before[Id] = W.Get(Id)->Totals[static_cast<size_t>(world::Venue::Live)];
	}
	W.AdvanceTo(Thin.Start + 1.0);
	const world::Cancellation* C = W.CancellationOf(Thin.Id);
	Check(C && C->Field == 12 && C->At == Thin.Start, "at cards-up a thin night is called off");
	Check(W.Registered(Thin.Id).empty() && W.PlannedEntries(Thin.Id) == 0, "nobody plays a cancelled event");
	W.AdvanceTo(Thin.Start + Thin.T->Hours * 60.0 + 120.0);
	Check(W.ResultOf(Thin.Id) == nullptr && W.ResultOf(Full.Id) != nullptr, "the cancelled night has no winner; the day's full one does");
	W.Cancel(Full.Id, Full.Start, 5);
	Check(W.CancellationOf(Full.Id) == nullptr && W.ResultOf(Full.Id) != nullptr, "an event already decided stays decided");
	// Nobody paid for it or finished in it (nobody plays two live events in a day).
	bool Untouched = true;
	for (const auto& It : Before)
	{
		const world::Npc* N = W.Get(It.first);
		const world::Ledger& Now = N->Totals[static_cast<size_t>(world::Venue::Live)];
		Untouched = Untouched && Now.Events == It.second.Events && Now.Spent == It.second.Spent;
		for (const world::Finish& F : N->Recent)
		{
			Untouched = Untouched && !(F.Event == Thin.T->Name && F.Day == Thin.Day);
		}
	}
	Check(!Before.empty() && Untouched, "nobody paid for the cancelled night or finished in it");
	// The world remembers it through a save.
	std::string Saved;
	W.Write(Saved);
	world::World Copy;
	Load(Copy, Saved);
	std::string Again;
	Copy.Write(Again);
	Check(Again == Saved && Copy.CancellationOf(Thin.Id) && Copy.CancellationOf(Thin.Id)->Field == 12, "a cancellation is saved");
	// A week on, the same event runs as usual.
	live::Occurrence Later;
	for (const live::Occurrence& O : live::Occurrences(Tomorrow + 7))
	{
		Later = std::string(O.T->Key) == Thin.T->Key ? O : Later;
	}
	W.AdvanceTo(Later.Start + 30.0);
	Check(Later.Valid() && W.CancellationOf(Later.Id) == nullptr && W.PlannedEntries(Later.Id) >= 60, "a week on, the same event runs with its usual crowd");
	std::printf("minimum field: %s called off with 12 at %s; nobody paid, no winner\n", Thin.Id.c_str(), net::TimeLabel(Thin.Start).c_str());
}

/** The player's own nights: never called off over their head, where the people around them finished, who's coming back. */
void PlayerNights(const world::World& Base)
{
	world::World W = Base;
	const int Tomorrow = net::DayOf(W.Clock()) + 1;
	W.EnsurePlanned(Tomorrow);
	const std::vector<live::Occurrence> Day = live::Occurrences(Tomorrow);
	const live::Occurrence Noon = Day.front();
	const live::Occurrence Night = Day.back();
	// Someone registered knows it's their next night at the room (as far as the world has planned).
	const std::vector<int> Coming = W.Registered(Night.Id);
	Check(!Coming.empty() && W.NextEmbercrest(Coming.front(), Night.Start - 60.0) == Night.Id && W.NextEmbercrest(Coming.front(), Night.Start + 1.0) != Night.Id,
		"who's coming back to the room, and when");
	// A thin night the player is in: their room calls it, and the world takes its word.
	W.HeroEntered(Night.Id, Night.T->Name);
	W.SetPlannedEntries(Night.Id, 10);
	// The noon game: the player goes out 40th, after one of the world's people (50th) and before another.
	W.HeroEntered(Noon.Id, Noon.T->Name);
	const std::vector<int> AtNoon = W.Registered(Noon.Id);
	Check(AtNoon.size() >= 2, "the noon game has the world's people in it");
	if (AtNoon.size() < 2)
	{
		return;
	}
	W.AdvanceTo(Noon.Start + 180.0);
	world::World::TableReport Rp;
	Rp.Places.push_back({AtNoon[0], 50});
	Rp.StillIn.push_back(AtNoon[1]);
	Rp.Met = {AtNoon[0], AtNoon[1]};
	W.HeroFinished(Noon.Id, 40, 0, Rp);
	W.AdvanceTo(Night.Start + 30.0);
	Check(W.CancellationOf(Night.Id) == nullptr, "a night the player is in is never called off over their head");
	W.Cancel(Night.Id, Night.Start, 11);
	Check(W.CancellationOf(Night.Id) && W.Registered(Night.Id).empty(), "the player's room called it off: the world agrees");
	W.AdvanceTo(Night.Start + 2.5 * 1440.0);
	Check(W.ResultOf(Night.Id) == nullptr, "and it never has a winner");
	int Place = 0;
	Chips Prize = -1;
	int Better = 0;
	Chips BetterPrize = -1;
	int Nobody = 0;
	Chips NobodyPrize = 0;
	const bool Out = W.FinishIn(Noon.Id, AtNoon[0], Place, Prize);
	const bool Ahead = W.FinishIn(Noon.Id, AtNoon[1], Better, BetterPrize);
	Check(Out && Place == 50 && Prize == 0 && Ahead && Better >= 1 && Better < 40 && !W.FinishIn(Noon.Id, -5, Nobody, NobodyPrize),
		"where the people at the player's tables finished: as the room saw it, and ahead of the player for those still in");
	std::string Saved;
	W.Write(Saved);
	world::World Copy;
	Load(Copy, Saved);
	int Kept = 0;
	Chips KeptPrize = -1;
	Check(Copy.FinishIn(Noon.Id, AtNoon[1], Kept, KeptPrize) && Kept == Better && KeptPrize == BetterPrize, "the places are saved with the result");
	std::printf("player nights: out 40th at %s; %s went out 50th, %s finished %s\n", Noon.Id.c_str(), W.Get(AtNoon[0])->Name.c_str(), W.Get(AtNoon[1])->Name.c_str(), Ordinal(Better).c_str());
}

/**
 * Dee's game is on the world's calendar on her three nights (Tuesday, Thursday, Saturday, doors at nine, the last hand at
 * five), as Life runs it and her regulars keep it: the world registers them for all three, and nobody plays it otherwise.
 */
void DeesNights(const world::World& Base)
{
	world::World W = Base;
	const int First = net::DayOf(W.Clock()) + 1;
	int Nights = 0;
	int Seats = 0;
	for (int D = First; D < First + 28; ++D)
	{
		W.EnsurePlanned(D);
		const int Wd = net::Weekday(D);
		const bool Hers = Wd == 1 || Wd == 3 || Wd == 5;
		int Listed = 0;
		Chips BuyIn = 0;
		for (const world::LiveEvent& E : world::LiveCalendar(D))
		{
			if (E.Id.rfind("dees@", 0) == 0)
			{
				++Listed;
				BuyIn = E.Stakes * 100; // a hundred big blinds, as the world buys into a back-room game
				Check(E.Id == "dees@" + std::to_string(D) && E.Kind == world::LiveKind::Underground && E.Start == D * 1440.0 + 21.0 * 60.0 && E.Duration == 8.0 * 60.0,
					"Dee's game: " + E.Id + " opens at nine and deals till five");
			}
		}
		Check(Listed == (Hers ? 1 : 0), "Dee's game is on " + std::string(net::WeekdayName(D, true)) + "s only if it's one of her nights");
		const std::vector<int> Who = W.Registered("dees@" + std::to_string(D));
		if (!Hers)
		{
			Check(Who.empty(), "nobody is registered for a Dee's game that isn't on");
			W.Simulate(1);
			continue;
		}
		++Nights;
		int Regulars = 0;
		for (const char* Name : {"Sal", "Mei", "Big Lou", "Twitch"})
		{
			const int Id = W.Find(Name);
			const world::Npc* N = W.Get(Id);
			const bool In = std::find(Who.begin(), Who.end(), Id) != Who.end();
			Regulars += In ? 1 : 0;
			const bool Away = N && N->TripUntil >= D && D >= N->TripFrom;
			// Short: the buy-in is more than they have, and nobody stakes them (Twitch, broke, is at Ruby's instead).
			const bool Short = N && N->Backer == -1 && N->Bankroll < BuyIn && N->Income * 8 < BuyIn;
			Check(In || !N || !N->Playing() || Away || Short, std::string(Name) + " is at Dee's on " + net::WeekdayName(D, true) + " (day " + std::to_string(D) + ")");
			if (In)
			{
				// Her game comes first: not at the ridge, nor at another room's game, that day.
				for (const live::Occurrence& O : live::Occurrences(D))
				{
					const std::vector<int> There = W.Registered(O.Id);
					Check(std::find(There.begin(), There.end(), Id) == There.end(), std::string(Name) + " isn't at the ridge on one of Dee's nights");
				}
				for (const world::LiveEvent& E : world::LiveCalendar(D))
				{
					const std::vector<int> There = E.Kind == world::LiveKind::Local && !live::IsEmbercrest(E.Id) ? W.Registered(E.Id) : std::vector<int>();
					Check(std::find(There.begin(), There.end(), Id) == There.end(), std::string(Name) + " isn't at " + E.Name + " on one of Dee's nights");
				}
			}
		}
		Seats += Regulars;
		W.Simulate(1);
	}
	Check(Nights == 12, "four weeks of Dee's nights: Tuesday, Thursday and Saturday");
	// Who sat down at Dee's stays known after the night is played (Entered): the block's schedule doesn't lose them at five.
	{
		world::World After = Base;
		int Day = net::DayOf(After.Clock()) + 1;
		while (net::Weekday(Day) != 1)
		{
			++Day;
		}
		After.AdvanceTo(Day * 1440.0 + 22.0 * 60.0);
		const std::string Id = "dees@" + std::to_string(Day);
		const std::vector<int> During = After.Entered(Id);
		Check(!During.empty() && During == After.RegisteredNow(Id), "Entered: Dee's table, while it's dealt, is who registered");
		After.AdvanceTo((Day + 1) * 1440.0 + 6.0 * 60.0);
		Check(After.RegisteredNow(Id).empty() && After.Entered(Id) == During, "Entered: who played it, once it's played");
		std::string Saved;
		After.Write(Saved);
		world::World Loaded;
		Load(Loaded, Saved);
		Check(Loaded.Entered(Id).empty(), "Entered: a loaded world knows only what's ahead");
		After.AdvanceTo((Day + 3) * 1440.0 + 6.0 * 60.0);
		Check(After.Entered(Id).empty(), "Entered: forgotten two days on");
	}
	Check(Seats >= 2 * Nights, "Dee's regulars fill her seats (" + std::to_string(Seats) + " over " + std::to_string(Nights) + " nights)");
	std::printf("dee's game: %d nights in 4 weeks, %d of her regulars' seats filled\n", Nights, Seats);
}

/** Talk on the block (MemoryKind::Street): once a day, one memory for a run of it, saved, in words and in the greetings. */
void StreetMemory(world::World& W)
{
	const int Sal = W.Find("Sal");
	const int Mei = W.Find("Mei");
	const double Now = W.Clock();
	const world::Bond* B0 = W.BondWith(Mei);
	const size_t Had = B0 ? B0->Memories.size() : 0;
	W.Remember(Mei, world::MemoryKind::Street, "Ruby's", 0, Now);
	W.Remember(Mei, world::MemoryKind::Street, "Ruby's", 0, Now + 10.0);
	const world::Bond* B = W.BondWith(Mei);
	Check(B && B->Memories.size() == Had + 1 && B->Memories.back().Kind == world::MemoryKind::Street, "street: once a day");
	W.Remember(Mei, world::MemoryKind::Street, "the 14's stop", 0, Now + 1440.0);
	W.Remember(Mei, world::MemoryKind::Street, "Ruby's", 0, Now + 2880.0);
	B = W.BondWith(Mei);
	const size_t Run = B->Memories.size();
	W.Remember(Mei, world::MemoryKind::Street, "Fifth St.", 0, Now + 4320.0);
	B = W.BondWith(Mei);
	Check(B->Memories.size() == Run && B->Memories.back().Where == "Fifth St." && B->Memories.back().Day == net::DayOf(Now + 4320.0), "street: a run of meetings is one memory, kept current");
	Check(B->Label() == "Knows you" || B->Familiarity > 0.0f, "street: they know the player");
	W.Remember(Sal, world::MemoryKind::Street, "the Wash & Fold", 0, Now);
	Check(std::string(world::MemoryText(world::MemoryKind::Street)) == "Talked on the block", "street: MemoryText covers it");
	std::set<std::string> Texts;
	for (int K = 0; K < static_cast<int>(world::MemoryKind::Count); ++K)
	{
		const std::string T = world::MemoryText(static_cast<world::MemoryKind>(K));
		Check(!T.empty() && Texts.insert(T).second, "MemoryText has its own words for every kind (" + std::to_string(K) + ")");
	}
	bool Spoken = false;
	bool Chat = false;
	for (uint32_t Salt = 0; Salt < 3; ++Salt)
	{
		Spoken = Spoken || W.SpokenGreeting(Mei, Salt).find("Fifth") != std::string::npos;
		Chat = Chat || W.Greeting(Mei, Salt).find("fifth") != std::string::npos;
	}
	Check(Spoken && Chat, "street: the greetings learn it (\"" + W.SpokenGreeting(Mei, 0) + "\")");
	const world::Profile P = W.ProfileOf(Mei);
	bool Card = false;
	for (const std::string& M : P.Memories)
	{
		Card = Card || M.find("Talked on the block (Fifth St.)") != std::string::npos;
	}
	Check(Card, "street: the player card says where");
	// Saved and loaded as it was.
	std::string Saved;
	W.Write(Saved);
	world::World Copy;
	Load(Copy, Saved);
	const world::Bond* C = Copy.BondWith(Mei);
	Check(C && C->Memories.size() == B->Memories.size() && C->Memories.back().Kind == world::MemoryKind::Street && C->Memories.back().Where == "Fifth St.", "street: MemoryKind::Street round-trips");
	std::string Again;
	Copy.Write(Again);
	Check(Again == Saved, "street: a loaded world writes the same save");
	// The twenty lent on Fifth: trust, and more when it comes back.
	const int Twitch = W.Find("Twitch");
	const float Before = W.BondWith(Twitch) ? W.BondWith(Twitch)->Trust : 0.0f;
	W.Lent(Twitch, false, Now);
	const float Lent = W.BondWith(Twitch)->Trust;
	W.Lent(Twitch, true, Now + 1440.0);
	Check(Lent > Before && W.BondWith(Twitch)->Trust > Lent, "street: lending moves Trust");
	// Run into someone once, it's an accident; again the same day, they hold it against you a little.
	const float Calm = W.BondWith(Sal) ? W.BondWith(Sal)->Resentment : 0.0f;
	const bool First = W.Bumped(Sal, Now + 100.0);
	const bool Second = W.Bumped(Sal, Now + 103.0);
	Check(!First && Second && W.BondWith(Sal)->Resentment > Calm && W.BondWith(Sal)->Resentment <= Calm + 0.0201f, "street: a second bump the same day, Resentment +0.02");
	// The capsules touch for a few frames: contacts within seconds of each other are the one collision.
	const float Held = W.BondWith(Sal)->Resentment;
	const bool Touch = W.Bumped(Sal, Now + 103.0 + 2.0 / 60.0);
	const bool Touch2 = W.Bumped(Sal, Now + 103.0 + 4.0 / 60.0);
	Check(!Touch && !Touch2 && W.BondWith(Sal)->Resentment == Held, "street: one collision is one bump, however many frames it touches");
	Check(W.Bumped(Sal, Now + 103.0 + 0.5) && W.BondWith(Sal)->Resentment > Held, "street: running into him again half a minute on counts");
	Check(!W.Bumped(Sal, Now + 100.0 + 2.0 * 1440.0), "street: a bump days later is an accident again");
}

std::string FixturePath(const char* Name)
{
	// tests/ -> Standalone/ -> the plugin's Tests/fixtures/ (world_test has no SS_TESTS_DIR of its own).
	std::string Here = __FILE__;
	for (char& C : Here)
	{
		C = C == '\\' ? '/' : C;
	}
	const size_t Cut = Here.rfind("/Standalone/");
	return (Cut == std::string::npos ? std::string("..") : Here.substr(0, Cut)) + "/Tests/fixtures/" + Name;
}

std::string ReadText(const std::string& Path)
{
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
	// (A checkout that converted the line endings still reads as written.)
	Out.erase(std::remove(Out.begin(), Out.end(), '\r'), Out.end());
	return Out;
}

double MeanVisits(const world::World& W, int Day)
{
	const std::vector<int> Regulars = W.EmbercrestRegulars();
	int Sum = 0;
	for (int Id : Regulars)
	{
		Sum += W.Get(Id)->Room.VisitsIn(Day);
	}
	return Regulars.empty() ? 0.0 : static_cast<double>(Sum) / static_cast<double>(Regulars.size());
}

/** The calendar's badge helpers: months, season weeks, the circuit's events, the Verified rule at its edges, freerolls. */
void BadgeRules()
{
	Check(world::SeasonWeek(world::DayOn(2027, 1, 1)) == 1 && world::SeasonWeek(world::DayOn(2027, 1, 7)) == 1 && world::SeasonWeek(world::DayOn(2027, 1, 8)) == 2,
		"season weeks start on January 1");
	Check(world::SeasonWeek(world::DayOn(2027, 3, 26)) == 13 && world::SeasonWeek(world::DayOn(2027, 3, 25)) == 12, "week 13 starts on March 26 (the standings' first)");
	Check(world::MonthStart(world::DayOn(2027, 2, 28)) == world::DayOn(2027, 2, 1) && world::MonthStart(world::DayOn(2027, 3, 1)) == world::DayOn(2027, 3, 1), "a month's first day");
	Check(world::MonthKeyOf(world::DayOn(2027, 1, 31)) == 2027 * 12 && world::MonthKeyOf(world::DayOn(2027, 2, 1)) == 2027 * 12 + 1, "month keys");
	Check(!world::CircuitEvent(world::LiveKind::Local, false) && !world::CircuitEvent(world::LiveKind::Underground, false) && world::CircuitEvent(world::LiveKind::Regional, false) &&
			  world::CircuitEvent(world::LiveKind::Summit, false) && !world::CircuitEvent(world::LiveKind::Regional, true),
		"circuit points: Regional and up, never a satellite, never the room's weeklies");
	// A room record's visits, and the month's start the tier can't drop below.
	{
		world::RoomRecord R;
		const int First = world::DayOn(2027, 3, 1);
		for (int D = First - 95; D < First + 10; D += 5)
		{
			R.Days.push_back(D);
		}
		Check(R.VisitsIn(First - 1) == 18 && R.VisitsAtMonthStart(First + 9) == 18 && R.VisitsIn(First + 9) == 18, "visits in 90 days, and at the month's start");
		Check(R.VisitsIn(First - 1, 1) == 0 && R.VisitsIn(First + 5, 1) == 1, "a window counts the days in it");
	}
	// Verified Pro's record (§5.3): 1,000 paid tournaments, +15%, played in 30 days, a professional. Freerolls count for nothing.
	{
		auto Page = [](int Events, int Freerolls, double Roi) {
			world::Tracker T;
			const Chips BuyIn = 1100;
			const Chips Profit = static_cast<Chips>(std::llround(Roi * static_cast<double>(BuyIn) * Events));
			for (int K = 0; K < Events; ++K)
			{
				T.Add(BuyIn, K == 0 ? BuyIn * Events + Profit : 0, K == 0 ? 1 : 50, 100, 9, world::TrackStake::Low, world::TrackFormat::Regular);
			}
			for (int K = 0; K < Freerolls; ++K)
			{
				T.Add(0, 50000, 1, 300, 9, world::TrackStake::Micro, world::TrackFormat::Regular);
			}
			return T;
		};
		const world::Tracker Ok = Page(1000, 0, 0.15);
		Check(world::VerifiedRecord(Ok, true, 30) && !world::VerifiedRecord(Ok, true, 31) && !world::VerifiedRecord(Ok, false, 0), "Verified: 1,000 at +15%, played in 30 days, a professional");
		Check(!world::VerifiedRecord(Page(999, 0, 0.15), true, 0) && !world::VerifiedRecord(Page(1000, 0, 0.149), true, 0), "Verified: not at 999, not at +14.9%");
		const world::Tracker Free = Page(999, 40, 0.10);
		Check(Free.Events == 1039 && Free.PaidEvents() == 999 && Free.Freerolls == 40 && Free.PaidRoi() < 0.1001 && Free.Roi() > Free.PaidRoi() && !world::VerifiedRecord(Free, true, 0),
			"freerolls make neither the count nor the ROI");
		Check(Free.PaidAbi() == 1100 && Free.PaidProfit() == Free.PaidPrizes() - Free.BuyIns, "the paid numbers: ABI and profit without the freerolls");
	}
}

/** The badges' world data on day one: the titles before the story began, the room's months, the first review. */
void DayOne(const world::World& W)
{
	int Founding = 0;
	int Holders = 0;
	int Bracelets = 0;
	int Rings = 0;
	bool Before = true;
	bool Travel = true;
	bool Booked = true;
	for (const world::Npc& N : W.People())
	{
		Founding += N.From == world::Origin::Founding ? 1 : 0;
		if (N.Awards.empty())
		{
			continue;
		}
		++Holders;
		Bracelets += N.Bracelets > 0 ? 1 : 0;
		Rings += N.Rings > 0 ? 1 : 0;
		Chips Sum = 0;
		Chips Top = 0;
		for (const world::Award& A : N.Awards)
		{
			Before = Before && world::YearOf(A.Day) < 2026 && !A.Online && !A.Series.empty() && A.Event.rfind(A.Series + ": ", 0) == 0 && A.Prize > 0 && A.Entries > 0;
			Before = Before && (A.Ring ? A.Series.rfind("Grand Circuit ", 0) == 0 : A.Series.rfind("The Championship ", 0) == 0);
			Sum += A.Prize;
			Top = std::max(Top, A.Prize);
		}
		Travel = Travel && N.From == world::Origin::Founding && N.Live >= world::LiveLevel::Regional;
		// The trophy and the career agree: the live record holds every title's money and its win.
		const world::Ledger& Lv = N.Totals[static_cast<size_t>(world::Venue::Live)];
		Booked = Booked && Lv.Won >= Sum && Lv.Best >= Top && Lv.Wins >= static_cast<int>(N.Awards.size()) && Lv.Cashes >= Lv.Wins && Lv.Events >= Lv.Cashes &&
			N.Bracelets + N.Rings == static_cast<int>(N.Awards.size()) && N.CareerWon() >= Sum;
	}
	Check(Before && Travel, "the titles before the story began: live, before 2026, won by regulars who travel");
	Check(Booked, "the titles before the story began are in their live records: the wins, the money, the best cash");
	Check(Holders * 45 >= Founding && Holders * 20 <= Founding, "about one regular in thirty holds a bracelet or a ring on the first night (" + std::to_string(Holders) + ")");
	// The room's last months, from habits: about 21 visits in 90 days for a regular; nobody on the wall without a win.
	const int Today = net::DayOf(W.Clock());
	const double Mean = MeanVisits(W, Today);
	int Walled = 0;
	for (const world::Npc& N : W.People())
	{
		Walled += N.Room.Wins;
	}
	Check(Mean >= 15.0 && Mean <= 28.0 && Walled == 0, "the room's regulars have their months (" + Fixed(Mean, 1) + " visits in 90 days), and nobody is on the wall yet");
	// The first review: Verified is the record's (and a believable share of the professionals).
	int Verified = 0;
	bool Honest = true;
	for (const world::Npc& N : W.People())
	{
		Verified += N.Verified ? 1 : 0;
		Honest = Honest && N.Verified == world::VerifiedRecord(N.Stats, N.Professional, Today - N.LastDay);
	}
	Check(Honest && Verified >= 30 && Verified <= 220, "the first Verified review (" + std::to_string(Verified) + ")");
	// Team RiverLine is paid what its terms pay (the flat $1,500 until the catalog lands).
	const Chips Weekly = sponsor::NpcWeekly("riverline") > 0 ? sponsor::NpcWeekly("riverline") : 150000;
	int Pros = 0;
	bool Paid = true;
	for (const world::Npc& N : W.People())
	{
		Pros += N.Pro ? 1 : 0;
		Paid = Paid && (!N.Pro || N.Income == Weekly);
	}
	Check(Pros == 10 && Paid, "Team RiverLine's ten are paid NpcWeekly(riverline) a week");
	std::printf("day one: %d of %d regulars hold titles (%d bracelets, %d rings), %.1f room visits in 90 days, %d Verified\n", Holders, Founding, Bracelets, Rings, Mean, Verified);
}

/** The player's money with the world's people (HeroMoney): bankrolls, memories, regard, privacy, the save. */
void HeroMoney(world::World& W)
{
	const int Lou = W.Find("Big Lou");
	const int Park = W.Find("Mrs. Park");
	const double Now = W.Clock();
	const Chips Roll = W.Get(Lou)->Bankroll;
	const size_t Logged = W.Events().size();
	W.HeroMoney(Lou, -12000, world::MemoryKind::Staked, "Embercrest Nightly $120", Now);
	Check(W.Get(Lou)->Bankroll == Roll - 12000 && W.BondWith(Lou) && W.BondWith(Lou)->Memories.back().Kind == world::MemoryKind::Staked && W.BondWith(Lou)->Memories.back().Amount == 12000,
		"a stake comes out of Lou's roll, and he remembers it");
	const float Trust = W.BondWith(Lou)->Trust;
	W.HeroMoney(Lou, 17800, world::MemoryKind::PaidUp, "Embercrest Nightly $120", Now + 600.0, true);
	Check(W.Get(Lou)->Bankroll == Roll + 5800 && W.BondWith(Lou)->Trust > Trust + 0.059f && W.BondWith(Lou)->Respect >= 0.02f, "paid early: his share back, Trust +0.06, Respect +0.02");
	// A welch: the regard falls apart, the label says so, and nothing said out loud mentions money (N3).
	for (int K = 0; K < 3; ++K)
	{
		W.Remember(Park, world::MemoryKind::Met, "the Embercrest", 0, Now - 3000.0 + K);
	}
	W.HeroMoney(Park, -1580, world::MemoryKind::BoughtPiece, "Embercrest Sunday $150", Now);
	W.HeroMoney(Park, 0, world::MemoryKind::PaidLate, "Embercrest Sunday $150", Now + 4320.0);
	W.HeroMoney(Park, 0, world::MemoryKind::Welched, "Embercrest Sunday $150", Now + 14400.0);
	const world::Bond* B = W.BondWith(Park);
	Check(B && B->Label() == "Holds a grudge" && B->Resentment >= 0.4f && B->Trust == 0.0f, "a welch: Mrs. Park holds a grudge");
	bool Quiet = true;
	for (uint32_t Salt = 0; Salt < 3; ++Salt)
	{
		for (const std::string& Line : {W.SpokenGreeting(Park, Salt), W.Greeting(Park, Salt), W.SpokenGreeting(Lou, Salt), W.Greeting(Lou, Salt)})
		{
			Quiet = Quiet && !Line.empty() && Line.find('$') == std::string::npos && Line.find("owe") == std::string::npos && Line.find("money") == std::string::npos &&
				Line.find("stake") == std::string::npos && Line.find("piece") == std::string::npos;
		}
	}
	Check(Quiet, "greetings after money change their tone, never mention it");
	// Every line a money memory puts in someone's mouth at the table passes the copy lint as table talk.
	{
		world::World Scratch = W;
		const int Dre = Scratch.Find("Dre");
		std::vector<std::pair<std::string, std::string>> Lines;
		double At = Now + 40000.0;
		for (world::MemoryKind M : {world::MemoryKind::Staked, world::MemoryKind::BoughtPiece, world::MemoryKind::Swapped, world::MemoryKind::PaidUp, world::MemoryKind::PaidLate,
				 world::MemoryKind::Welched, world::MemoryKind::WalkedOut})
		{
			Scratch.HeroMoney(Dre, 0, M, "Embercrest Nightly $120", At);
			At += 1440.0;
			Check(Scratch.BondWith(Dre) && Scratch.BondWith(Dre)->Memories.back().Kind == M && std::strlen(world::MemoryText(M)) > 0, std::string("a money memory is kept and named: ") + world::MemoryText(M));
			for (uint32_t Salt = 0; Salt < 3; ++Salt)
			{
				Lines.push_back({world::MemoryText(M), Scratch.SpokenGreeting(Dre, Salt)});
			}
		}
		Check(copylint::Run("world", Lines, copylint::TableTalk) == 0, "the money memories' greetings pass the copy lint");
	}
	// A late debt is one memory and one knock to their trust (§2.9): the mark at the due time, then the money settles it.
	// And money their way to the player under "paid up" is a swap settling, never the player paying them.
	{
		world::World Scratch = W;
		const int Rick = Scratch.Find("Rick");
		for (int K = 0; K < 4; ++K)
		{
			Scratch.HeroMoney(Rick, 100, world::MemoryKind::PaidUp, "Embercrest Nightly $120", Now + 100.0 * K);
		}
		const float Trusted = Scratch.BondWith(Rick)->Trust;
		const size_t Had = Scratch.BondWith(Rick)->Memories.size();
		const Chips RickRoll = Scratch.Get(Rick)->Bankroll;
		Scratch.HeroMoney(Rick, 0, world::MemoryKind::PaidLate, "Embercrest Sunday $150", Now + 4320.0);
		Scratch.HeroMoney(Rick, 4200, world::MemoryKind::PaidLate, "Embercrest Sunday $150", Now + 6000.0);
		const world::Bond* Late = Scratch.BondWith(Rick);
		Check(Trusted >= 0.15f && Late->Memories.size() == Had + 1 && Late->Memories.back().Kind == world::MemoryKind::PaidLate && Late->Memories.back().Amount == 4200 &&
				  std::fabs((Trusted - Late->Trust) - 0.10f) < 1e-4f && Scratch.Get(Rick)->Bankroll == RickRoll + 4200,
			"a late debt: one memory, Trust -0.10 once, the money when it's paid");
		Scratch.HeroMoney(Rick, -900, world::MemoryKind::PaidUp, "Embercrest Big Stack", Now + 9000.0);
		Check(Scratch.BondWith(Rick)->Memories.back().Kind == world::MemoryKind::Swapped && Scratch.Get(Rick)->Bankroll == RickRoll + 3300,
			"money from them under \"paid up\" is remembered as the swap settling");
	}
	// Nobody's roll goes below zero, whatever the player is owed.
	W.HeroMoney(Park, -(1LL << 40), world::MemoryKind::Swapped, "Embercrest Big Stack", Now + 20000.0);
	Check(W.Get(Park)->Bankroll == 0, "a roll never goes below zero");
	// Never public: no headline, nothing on the card but the player's own memories of them.
	const world::Profile P = W.ProfileOf(Park);
	bool Told = false;
	for (const std::string& M : P.Memories)
	{
		Told = Told || M.find("You never paid them") != std::string::npos;
	}
	Check(Told && W.Events().size() == Logged, "the player's card of her remembers it (theirs to know), and nothing of it makes the news");
	Invariants(W, "after the player's money");
	// Every money memory round-trips.
	for (world::MemoryKind K : {world::MemoryKind::Swapped, world::MemoryKind::WalkedOut})
	{
		W.HeroMoney(Lou, 100, K, "Embercrest Nightly $120", Now + 30000.0 + static_cast<double>(K));
	}
	std::string Saved;
	W.Write(Saved);
	world::World Copy;
	Load(Copy, Saved);
	bool Same = true;
	for (int Id : {Lou, Park})
	{
		const world::Bond* A = W.BondWith(Id);
		const world::Bond* C = Copy.BondWith(Id);
		Same = Same && A && C && A->Memories.size() == C->Memories.size() && A->Trust == C->Trust && A->Resentment == C->Resentment;
		for (size_t K = 0; Same && K < A->Memories.size(); ++K)
		{
			Same = A->Memories[K].Kind == C->Memories[K].Kind && A->Memories[K].Amount == C->Memories[K].Amount;
		}
	}
	std::string Again;
	Copy.Write(Again);
	Check(Same && Again == Saved, "money memories round-trip");
	std::printf("player's money: Lou %s (%s), Mrs. Park %s\n", W.BondWith(Lou)->Label().c_str(), W.SpokenGreeting(Lou, 0).c_str(), W.BondWith(Park)->Label().c_str());
}

/** Backers (§2.5): people who respect or trust the player, with the roll for it and room for another horse. */
void WorldBackers(world::World& W)
{
	Check(W.Backers(5).empty() || W.HeroStats().PaidEvents() > 0, "nobody backs a player with no paid results");
	if (W.HeroStats().PaidEvents() == 0)
	{
		W.GrantHeroResults(60);
	}
	const int Hana = W.Find("HighRollerHana");
	for (int K = 0; K < 7; ++K)
	{
		W.Remember(Hana, world::MemoryKind::FinalTable, "the Embercrest", 0, W.Clock() + K);
	}
	const std::vector<int> Who = W.Backers(5);
	const bool Respected = W.BondWith(Hana) && W.BondWith(Hana)->Label() == "Respects you";
	Check(Respected && std::find(Who.begin(), Who.end(), Hana) != Who.end() && Who.size() <= 5, "a high roller who respects the player would back them");
	bool Fit = true;
	for (int Id : Who)
	{
		const world::Npc* N = W.Get(Id);
		const std::string L = W.BondWith(Id) ? W.BondWith(Id)->Label() : std::string();
		Fit = Fit && N && N->Playing() && (L == "Respects you" || L == "Trusts you") && N->Bankroll >= W.HeroStats().PaidAbi() * 400;
	}
	const int Park = W.Find("Mrs. Park");
	Check(Fit && std::find(Who.begin(), Who.end(), Park) == Who.end(), "backers: the regard, the roll; a welch's victim isn't one");
}

/** The player's live results: the room's record and leaders from the Embercrest, the circuit (and Live POY) from Regional up. */
void HeroRoomAndCircuit(world::World& W)
{
	const int Today = net::DayOf(W.Clock()) + 1;
	W.EnsurePlanned(Today);
	const live::Occurrence Night = live::Occurrences(Today).back();
	const double LiveBefore = W.HeroSeasonLivePoints();
	const int RoomBefore = W.HeroRoom().Events;
	W.HeroEntered(Night.Id, Night.T->Name);
	W.AdvanceTo(Night.Start + 120.0);
	W.HeroFinished(Night.Id, 1, 300000, world::World::TableReport());
	W.HeroFinished(Night.Id, 1, 300000, world::World::TableReport()); // told twice: still one result
	const world::RoomRecord& R = W.HeroRoom();
	Check(R.Events == RoomBefore + 1 && R.Wins >= 1 && R.LastWin == Night.Day && !R.Days.empty() && R.Days.back() == Night.Day && R.MonthPoints > 0.0,
		"an Embercrest win goes on the player's room record, once");
	Check(W.HeroSeasonLivePoints() == LiveBefore && W.HeroCircuitPoints() == LiveBefore, "the room's weeklies score no circuit or Live Player of the Year points");
	const std::vector<std::pair<int, double>> Board = W.RoomLeaders("embercrest", world::MonthKeyOf(Night.Day), 1000);
	bool Listed = false;
	bool Sorted = true;
	for (size_t K = 0; K < Board.size(); ++K)
	{
		Listed = Listed || Board[K].first == -1;
		Sorted = Sorted && Board[K].second > 0.0 && (K == 0 || Board[K - 1].second >= Board[K].second);
	}
	const std::vector<std::pair<int, double>> Top = W.RoomLeaders("embercrest", world::MonthKeyOf(Night.Day), 10);
	Check(Listed && Sorted && Top.size() == std::min<size_t>(10, Board.size()) && std::equal(Top.begin(), Top.end(), Board.begin()) &&
			  W.RoomLeaders("embercrest", world::MonthKeyOf(Night.Day) + 1, 10).empty(),
		"the month's room leaders: the top ten of the board, the player on it");
	// A regional Main Event: the circuit's points and the live race.
	int Day = Today;
	world::LiveEvent Main;
	for (; Day < Today + 60 && Main.Id.empty(); ++Day)
	{
		for (const world::LiveEvent& E : world::LiveCalendar(Day))
		{
			Main = Main.Id.empty() && E.Kind == world::LiveKind::Regional && E.Main ? E : Main;
		}
	}
	Check(!Main.Id.empty(), "a regional Main Event within two months");
	W.AdvanceTo(Main.Start - 600.0);
	W.HeroEntered(Main.Id, Main.Name);
	W.AdvanceTo(Main.Start + 600.0);
	W.HeroFinished(Main.Id, 3, 2000000, world::World::TableReport());
	Check(W.HeroCircuitPoints() > LiveBefore && W.HeroSeasonLivePoints() == W.HeroCircuitPoints() && W.CircuitRank(-1) >= 1, "a regional final table scores the circuit and the live race");
	std::printf("player's live: Embercrest month %.0f pts, circuit %.0f pts (rank %d)\n", R.MonthPoints, W.HeroCircuitPoints(), W.CircuitRank(-1));
}

/** Who plays where earns what: room records only from the Embercrest, circuit points only from Regional events up. */
void WhereItCounts(const world::World& Base)
{
	world::World W = Base;
	std::map<int, int> Room;
	std::map<int, double> Circuit;
	for (const world::Npc& N : W.People())
	{
		Room[N.Id] = N.Room.Events;
		Circuit[N.Id] = N.CircuitPoints;
	}
	std::set<int> AtRoom;
	std::set<int> AtCircuit;
	// Who's registered for each day's live events, read early that day (before any of them is played): yesterday's and
	// today's now, then each day's as the world reaches it.
	auto Capture = [&](int D) {
		W.EnsurePlanned(D);
		for (const world::LiveEvent& E : world::LiveCalendar(D))
		{
			const bool Counts = world::CircuitEvent(E.Kind, !E.Ticket.empty());
			for (int Id : W.Registered(E.Id))
			{
				if (live::IsEmbercrest(E.Id))
				{
					AtRoom.insert(Id);
				}
				if (Counts)
				{
					AtCircuit.insert(Id);
				}
			}
		}
	};
	const int Today = net::DayOf(W.Clock());
	Capture(Today - 1);
	Capture(Today);
	for (int K = 1; K <= 14; ++K)
	{
		W.Simulate(1);
		Capture(Today + K);
	}
	bool RoomOk = true;
	bool CircuitOk = true;
	int RoomGrew = 0;
	for (const world::Npc& N : W.People())
	{
		const int Grew = N.Room.Events - (Room.count(N.Id) ? Room[N.Id] : 0);
		RoomGrew += Grew > 0 ? 1 : 0;
		const bool RoomFine = Grew == 0 || AtRoom.count(N.Id) > 0;
		const bool CircuitFine = N.CircuitPoints == (Circuit.count(N.Id) ? Circuit[N.Id] : 0.0) || AtCircuit.count(N.Id) > 0;
		if (!RoomFine || !CircuitFine)
		{
			std::printf("  %s: room +%d (registered %d), circuit %.0f -> %.0f (registered %d)\n", N.Name.c_str(), Grew, static_cast<int>(AtRoom.count(N.Id)),
				Circuit.count(N.Id) ? Circuit[N.Id] : 0.0, N.CircuitPoints, static_cast<int>(AtCircuit.count(N.Id)));
		}
		RoomOk = RoomOk && RoomFine;
		CircuitOk = CircuitOk && CircuitFine;
	}
	Check(RoomOk && RoomGrew > 30, "room records grow only from the Embercrest's events (" + std::to_string(RoomGrew) + " people in two weeks)");
	Check(CircuitOk, "circuit points come only from Regional events and up");
}

/** A season and a half: the standings, the rollover, Live Player of the Year, the reviews, the Hall of Fame's first July. */
void Seasons()
{
	world::World W;
	W.Create(7u, NightOne);
	// Into the money at the River City Classic's Main in October: the player on the circuit's board.
	world::LiveEvent Main;
	for (int Day = net::DayOf(W.Clock()); Day < net::DayOf(W.Clock()) + 40 && Main.Id.empty(); ++Day)
	{
		for (const world::LiveEvent& E : world::LiveCalendar(Day))
		{
			Main = Main.Id.empty() && E.Kind == world::LiveKind::Regional && E.Main && E.Where == world::Region::Americas ? E : Main;
		}
	}
	W.AdvanceTo(Main.Start - 300.0);
	W.HeroEntered(Main.Id, Main.Name);
	W.AdvanceTo(Main.Start + 300.0);
	W.HeroFinished(Main.Id, 2, 3500000, world::World::TableReport());
	const double Mine = W.HeroCircuitPoints();
	// The standings on Mondays: the ranks now are the ranks they posted (nothing has finished since midnight).
	int Monday = net::DayOf(W.Clock()) + 1;
	while (net::Weekday(Monday) != 0)
	{
		++Monday;
	}
	auto Posted = [&](const char* When) {
		std::vector<std::pair<double, int>> Board;
		for (const world::Npc& N : W.People())
		{
			if (!N.Faded && N.CircuitPoints > 0.0)
			{
				Board.push_back({N.CircuitPoints, N.Id});
			}
		}
		if (W.HeroCircuitPoints() > 0.0)
		{
			Board.push_back({W.HeroCircuitPoints(), -1});
		}
		std::sort(Board.begin(), Board.end(), [](const std::pair<double, int>& A, const std::pair<double, int>& B) { return A.first > B.first || (A.first == B.first && A.second < B.second); });
		bool Ok = true;
		for (size_t K = 0; K < Board.size(); ++K)
		{
			const int Want = K < 10 ? world::CardBlack : K < 50 ? world::CardGold : K < 200 ? world::CardRegular : world::CardNone;
			const int Has = Board[K].second < 0 ? W.HeroCardBest() : W.Get(Board[K].second)->CardBest;
			const int Rank = Board[K].second < 0 ? W.CircuitRank(-1) : W.CircuitRank(Board[K].second);
			Ok = Ok && Has >= Want && Rank == static_cast<int>(K) + 1;
		}
		Check(Ok && !Board.empty(), std::string(When) + ": the circuit's standings: Black for the top 10, Gold to 50, the Card to 200, each rank as posted");
		const double Cut = W.CardCutLine(world::CardBlack);
		Check(Board.size() < 10 ? Cut == 0.0 : Cut == Board[9].first, std::string(When) + ": the Black Card's cut line is the 10th best");
		return Board.size();
	};
	W.AdvanceTo(static_cast<double>(Monday) * 1440.0);
	Posted("October");
	Check(W.HeroCardBest() >= world::CardRegular && Mine > 0.0, "the player's first card");
	// The 1st's review: Verified is exactly the record's that morning.
	const int Feb = world::DayOn(2027, 2, 1);
	W.AdvanceTo(static_cast<double>(world::DayOn(2027, 1, 1)) * 1440.0);
	// New Year: last season's card is good through this one; the race starts again; Live POY from Regional points only.
	{
		bool Rolled = W.HeroCardLast() >= world::CardRegular && W.HeroCardBest() == world::CardNone && W.HeroCircuitPoints() == 0.0 && W.HeroSeasonLivePoints() == 0.0;
		int Carded = 0;
		for (const world::Npc& N : W.People())
		{
			Rolled = Rolled && N.CircuitPoints == 0.0 && N.ThisSeason.LivePoints == 0.0 && (N.CardBest == world::CardNone || !N.Awards.empty());
			Carded += N.CardLast > world::CardNone ? 1 : 0;
		}
		Check(Rolled && Carded > 0, "New Year: cards roll over to last season's, the points to zero (" + std::to_string(Carded) + " with a 2026 card)");
		bool Crowned = false;
		for (const world::Honor& H : W.Honors())
		{
			Crowned = Crowned || (H.Year == 2026 && H.Title == "Live Player of the Year");
		}
		Check(Crowned, "a Live Player of the Year for 2026, from Regional events and up");
	}
	W.AdvanceTo(static_cast<double>(Feb) * 1440.0);
	{
		bool Honest = true;
		int Verified = 0;
		for (const world::Npc& N : W.People())
		{
			Honest = Honest && N.Verified == (!N.Faded && world::VerifiedRecord(N.Stats, N.Professional, Feb - N.LastDay)) && (!N.Verified || N.ReviewMonth == world::MonthKeyOf(Feb));
			Verified += N.Verified ? 1 : 0;
		}
		Check(Honest && Verified >= 30 && Verified <= 220, "the 1st's review: Verified Pro is the record's (" + std::to_string(Verified) + ")");
	}
	// Week 13's Monday: the first standings of 2027.
	Monday = world::DayOn(2027, 3, 26);
	while (net::Weekday(Monday) != 0)
	{
		++Monday;
	}
	W.AdvanceTo(static_cast<double>(Monday - 7) * 1440.0 + 60.0);
	int Early = 0;
	for (const world::Npc& N : W.People())
	{
		const bool Titled = std::any_of(N.Awards.begin(), N.Awards.end(), [](const world::Award& A) { return world::YearOf(A.Day) == 2027; });
		Early += N.CardBest > world::CardNone && !Titled ? 1 : 0;
	}
	Check(Early == 0, "no card from the standings before week 13 (only a title this season grants one)");
	W.AdvanceTo(static_cast<double>(Monday) * 1440.0);
	const size_t Holders = Posted("week 13");
	// The room's own months by now (nothing seeded left in the window): the habits' history matches how they play.
	const double Played = MeanVisits(W, Monday);
	Check(Played >= 15.0 && Played <= 34.0, "the room's regulars, played out: " + Fixed(Played, 1) + " visits in 90 days");
	// July 15: the Hall of Fame's class (at most two, each eligible and over the bar), and the news.
	W.AdvanceTo(static_cast<double>(world::DayOn(2027, 7, 16)) * 1440.0);
	int Inducted = 0;
	bool Fair = true;
	for (const world::Honor& H : W.Honors())
	{
		if (H.Title == world::HallOfFameTitle)
		{
			++Inducted;
			const world::Npc* N = W.Get(H.Npc);
			Fair = Fair && H.Year == 2027 && N && N->Age(world::DayOn(2027, 7, 15)) >= 40 && 2027 - world::YearOf(N->Joined) >= 10 && H.Entries >= 12 && W.HallClass(H.Npc) == 2027 &&
				!W.HallEligible(H.Npc, 2028);
			bool Told = false;
			for (const world::WorldEvent& E : W.Events())
			{
				std::string Title;
				std::string Body;
				std::string Tag;
				Told = Told || (E.Kind == world::EventKind::HallOfFame && E.Npc == H.Npc && W.Headline(E, Title, Body, Tag) && Title.find("Hall of Fame") != std::string::npos &&
								   Tag == "HALL OF FAME" && Body.rfind("Class of 2027: ", 0) == 0);
			}
			Fair = Fair && Told;
		}
	}
	Check(Inducted <= 2 && Fair, "the Hall of Fame's first July: at most two, each eligible and over the bar, in the news (" + std::to_string(Inducted) + ")");
	// The player's own case: never while the world doesn't know their age (HeroBorn), the real rule once it does (§11 Q25).
	{
		const bool Unknown = W.HallEligible(-1, 2040);
		W.HeroBorn = 2000;
		const bool Forty = W.HallEligible(-1, 2040);
		const bool TooSoon = W.HallEligible(-1, 2035);
		W.HeroBorn = 2001;
		const bool ThirtyNine = W.HallEligible(-1, 2040);
		W.HeroBorn = 0;
		Check(!Unknown && Forty && !TooSoon && !ThirtyNine, "the player's Hall of Fame: forty and ten years on the scene, once the world knows their age");
	}
	Invariants(W, "a season and a half");
	std::printf("seasons: %zu on the circuit's board at week 13, %.1f room visits in 90 days; %d into the Hall of Fame in 2027\n%s", Holders, Played, Inducted, W.Report().c_str());
}

/** A version 1 world (Tests/fixtures/world_v1.txt): migrated once, as version 2 from then on. */
void Migration()
{
	const std::string Text = ReadText(FixturePath("world_v1.txt"));
	Check(Text.rfind("world\thead\t1\t", 0) == 0, "the version 1 world is there (" + FixturePath("world_v1.txt") + ")");
	if (Text.empty())
	{
		return;
	}
	world::World W;
	Load(W, Text);
	std::string Saved;
	W.Write(Saved);
	// (Pinned to world::Version: version 2 then, 3 from update 2's G1, each bump with its migration, update 2 §6.5.)
	Check(W.Ready() && Saved.rfind("world\thead\t" + std::to_string(world::Version) + "\t", 0) == 0 && Saved != Text && W.Began() == world::Start::Established,
		"a version 1 world loads and writes as version " + std::to_string(world::Version) + ", an established world");
	world::World Again;
	Load(Again, Saved);
	std::string Twice;
	Again.Write(Twice);
	Check(Twice == Saved, "the migrated world round-trips: migrated once, never twice");
	const int Today = net::DayOf(W.Clock());
	const double Mean = MeanVisits(W, Today);
	int Walled = 0;
	int Seeded = 0;
	bool Clean = true;
	bool Raced = W.HeroSeasonLivePoints() == 0.0;
	for (const world::Npc& N : W.People())
	{
		Walled += N.Room.Wins;
		Seeded += N.Room.Events > 0 ? 1 : 0;
		Clean = Clean && (N.Pro || N.Sponsor != "RiverLine");
		Raced = Raced && N.ThisSeason.LivePoints == 0.0;
	}
	Check(Mean >= 15.0 && Mean <= 28.0 && Walled == 0 && Seeded >= 64, "migration: the room's regulars get their months from habits (" + Fixed(Mean, 1) + " visits), nobody on the wall");
	Check(Clean, "migration: the site's patch comes off anyone who isn't one of its ten");
	Check(Raced, "migration: the live race starts again under the Regional-and-up rule");
	int Holders = 0;
	for (const world::Npc& N : W.People())
	{
		Holders += N.Awards.empty() ? 0 : 1;
	}
	Check(Holders >= 30, "migration: the titles before the story began (" + std::to_string(Holders) + " holders)");
	// Team RiverLine's older pros give back what signing added over the terms (§3.7: $1,500 a week then, NpcWeekly now), so
	// one who leaves later takes off exactly what's left; nobody else's income moves.
	{
		world::World Raw; // the version 1 lines as read, before Finish migrates them
		std::istringstream In(Text);
		std::string Line;
		while (std::getline(In, Line))
		{
			Raw.Read(Tabs(Line));
		}
		const Chips Weekly = sponsor::NpcWeekly("riverline") > 0 ? sponsor::NpcWeekly("riverline") : 150000;
		const Chips Less = 150000 - Weekly;
		bool Paid = true;
		int Pros = 0;
		for (size_t K = 0; K < Raw.People().size() && K < W.People().size(); ++K)
		{
			const world::Npc& Was = Raw.People()[K];
			const world::Npc& Now = W.People()[K];
			Pros += Was.Pro ? 1 : 0;
			Paid = Paid && Now.Income == (Was.Pro && Less > 0 ? std::max<Chips>(0, Was.Income - Less) : Was.Income);
		}
		Check(Paid && Pros == 10, "migration: Team RiverLine's pros are paid the terms' weekly, everyone else as they were");
	}
	Invariants(W, "a migrated world");
	W.Simulate(3);
	Invariants(W, "a migrated world, three days on");
	std::printf("migration: v1 day %d -> v2, %d room records, %.1f visits in 90 days, %d title holders\n", Today, Seeded, Mean, Holders);
}

int Checks()
{
	Calendar();
	BadgeRules();
	const auto T0 = std::chrono::steady_clock::now();
	world::World W;
	W.Create(1234u, NightOne);
	std::printf("created %zu people in %.2fs\n", W.People().size(), Seconds(T0));
	Invariants(W, "day one");
	DayOne(W);
	// The cast and the rival are in it.
	for (const char* Name : {"Sal", "Mrs. Park", "Rick", "Mei", "Dre", "Big Lou", "Twitch", "gh0stfold", "VikingVolta", "MissFinch"})
	{
		Check(W.Find(Name) >= 0, std::string("missing ") + Name);
	}
	Check(W.Find("gh0stfold") == net::Shared().RivalIndex(), "the rival keeps the network's index");
	// Tonight's events have registrations, and the people in them can afford them.
	int Registered = 0;
	for (const net::EventInstance& E : net::Shared().Window(NightOne, NightOne + 600.0))
	{
		for (int Id : W.Registered(E.Id))
		{
			++Registered;
			const world::Npc* N = W.Get(Id);
			Check(N && N->Playing(), "registered someone who isn't playing");
		}
	}
	Check(Registered > 200, "tonight's events have the world's regulars in them (" + std::to_string(Registered) + ")");
	// The Embercrest's Sunday has its cast.
	{
		const int Sunday = 6; // Sunday, October 11
		W.EnsurePlanned(Sunday);
		bool Planned = false;
		for (int Id : W.Registered("embercrest@" + std::to_string(Sunday)))
		{
			Planned = Planned || W.Get(Id)->Name == "Sal";
		}
		Check(!Planned, "a Sunday five days out isn't planned yet");
	}

	// A month: results come back, the boards fill, the story starts.
	const auto T1 = std::chrono::steady_clock::now();
	W.Simulate(30);
	const double Month = Seconds(T1);
	std::printf("30 days in %.2fs\n", Month);
	Invariants(W, "day 30");
	WhereItCounts(W);
	EmbercrestWeeks(W);
	DeesNights(W);
	MinimumField(W);
	PlayerNights(W);
	int Finished = 0;
	int Anonymous = 0;
	int Ours = 0;
	for (const net::EventInstance& E : net::Shared().Window(W.Clock() - 2880.0, W.Clock() - 1440.0))
	{
		if (const net::EventResult* R = W.ResultOf(E.Id))
		{
			++Finished;
			for (const net::Placing& P : R->FinalTable)
			{
				Anonymous += P.Player == -2 ? 1 : 0;
				Ours += P.Player >= 0 ? 1 : 0;
				Check(P.Player != -2 || !P.Name.empty(), "an anonymous finisher has a name");
				Check(P.Player < 0 || W.Get(P.Player) != nullptr, "a finisher is someone");
			}
		}
	}
	Check(Finished > 50, "yesterday's events have results (" + std::to_string(Finished) + ")");
	Check(Ours > 0 && Anonymous > 0, "final tables mix the world's regulars and everyone else");
	std::printf("yesterday: %d results, %d regulars and %d others at final tables\n", Finished, Ours, Anonymous);
	Check(!W.Events().empty(), "things happened");
	Check(W.BoardValue(net::Board::Season, W.Find("gh0stfold"), W.Clock()) > 0.0, "the rival has season points");
	// Newcomers: new names keep arriving, each with a story and a journey that starts the day they joined.
	{
		int Newcomers = 0;
		std::set<int> Kinds;
		for (const world::Npc& N : W.People())
		{
			if (N.Came == world::Arrival::None)
			{
				continue;
			}
			++Newcomers;
			Kinds.insert(static_cast<int>(N.Came));
			Check(!N.Path.empty() && N.Path.front().Kind == world::StepKind::Joined && N.Path.front().Day == N.Arrived, N.Name + "'s journey starts the day they joined");
			const world::Profile P = W.ProfileOf(N.Id);
			Check(!P.Came.empty() && !P.Journey.empty() && P.Arrived == N.Arrived, N.Name + "'s card tells how they got here");
		}
		Check(Newcomers >= 10 && Kinds.size() >= 4, "newcomers arrive, in different ways (" + std::to_string(Newcomers) + ")");
		const world::Profile Rival = W.ProfileOf(W.Find("gh0stfold"));
		Check(!Rival.Came.empty() && !Rival.Journey.empty() && Rival.Arrived < 0, "an old hand's card starts before the story did");
	}

	// Save and load: a world that's saved carries on exactly as one that isn't (the trophy cases too).
	W.GrantAward(-1, false, true);
	W.GrantAward(W.Find("Mei"), true, false);
	Check(W.HeroAwards().size() == 1 && W.HeroAwards().front().Main && !W.HeroAwards().front().Ring && !W.HeroAwards().front().Series.empty(), "the player's bracelet is in their trophy case");
	Check(!W.Get(W.Find("Mei"))->Awards.empty() && W.Get(W.Find("Mei"))->Awards.back().Ring, "a ring for Mei");
	std::string Saved;
	W.Write(Saved);
	std::printf("save: %zu KB\n", Saved.size() / 1024);
	Check(Saved.size() < 4u * 1024u * 1024u, "the save stays reasonable");
	world::World Copy;
	Load(Copy, Saved);
	std::string Again;
	Copy.Write(Again);
	Check(Again == Saved, "a loaded world writes the same save");
	Check(Copy.HeroAwards().size() == 1 && Copy.HeroAwards().front().Series == W.HeroAwards().front().Series, "the player's trophy case loads");
	{
		// A save from before the trophy cases (counts only): the titles in history fill them in.
		std::string Old;
		size_t At = 0;
		while (At < Saved.size())
		{
			const size_t End = Saved.find('\n', At);
			const std::string Ln = Saved.substr(At, End == std::string::npos ? std::string::npos : End - At + 1);
			if (Ln.rfind("world\tawards\t", 0) != 0 && Ln.rfind("world\theroawards\t", 0) != 0)
			{
				Old += Ln;
			}
			At = End == std::string::npos ? Saved.size() : End + 1;
		}
		world::World Before;
		Load(Before, Old);
		Invariants(Before, "an older save");
	}
	{
		// A save from before the Embercrest had regulars of its own: they join on load, once, as the same people.
		world::World Fresh;
		Fresh.Create(4321u, NightOne);
		std::string Text;
		Fresh.Write(Text);
		std::string Old;
		int Dropped = 0;
		size_t At = 0;
		while (At < Text.size())
		{
			const size_t End = Text.find('\n', At);
			const std::string Ln = Text.substr(At, End == std::string::npos ? std::string::npos : End - At + 1);
			bool Local = false;
			for (const live::RoomLocal& L : live::RoomLocals())
			{
				Local = Local || (Ln.rfind("world\tnpc\t", 0) == 0 && Ln.find("\t" + std::string(L.Name) + "\t") != std::string::npos);
			}
			Dropped += Local ? 1 : 0;
			Old += Local ? std::string() : Ln;
			At = End == std::string::npos ? Text.size() : End + 1;
		}
		world::World Migrated;
		Load(Migrated, Old);
		int Locals = 0;
		bool SameIds = true;
		for (const live::RoomLocal& L : live::RoomLocals())
		{
			const int Id = Migrated.Find(L.Name);
			const world::Npc* N = Migrated.Get(Id);
			Locals += N && N->From == world::Origin::Local ? 1 : 0;
			SameIds = SameIds && Id == Fresh.Find(L.Name);
		}
		Check(Dropped == static_cast<int>(live::RoomLocals().size()), "the older save had none of the room's regulars");
		Check(Locals == static_cast<int>(live::RoomLocals().size()) && SameIds, "an older save gets the room's regulars, the same people");
		std::string Joined;
		Migrated.Write(Joined);
		world::World Twice;
		Load(Twice, Joined);
		std::string Third;
		Twice.Write(Third);
		Check(Third == Joined && Twice.People().size() == Migrated.People().size(), "the regulars join once");
		Invariants(Migrated, "an older save, with the regulars");
	}
	W.Simulate(20);
	Copy.Simulate(20);
	std::string A;
	std::string B;
	W.Write(A);
	Copy.Write(B);
	Check(A == B, "a loaded world plays on exactly as the original");
	Invariants(Copy, "after load");

	// Different saves, different worlds.
	world::World Other;
	Other.Create(98765u, NightOne);
	Other.Simulate(50);
	std::string C;
	Other.Write(C);
	Check(C != A, "another save has another history");

	// The player: memories and greetings.
	{
		const int Sal = W.Find("Sal");
		W.BackRoomNight(W.Clock(), {"Sal", "Big Lou", "Twitch", "Mei"}, 4200);
		const world::Bond* Bd = W.BondWith(Sal);
		Check(Bd && !Bd->Memories.empty(), "Sal remembers a night at Dee's");
		Check(!W.Greeting(Sal, 1).empty(), "Sal has something to say");
	}
	StreetMemory(W);
	HeroMoney(W);
	WorldBackers(W);
	HeroRoomAndCircuit(W);
	// The news has a name for every kind of world event, the Hall of Fame's included.
	{
		std::set<std::string> Names;
		for (int K = 0; K < static_cast<int>(world::EventKind::Count); ++K)
		{
			const std::string Name = world::EventKindName(static_cast<world::EventKind>(K));
			Check(!Name.empty() && Names.insert(Name).second, "EventKindName names every kind (" + std::to_string(K) + ")");
		}
		Check(std::string(world::EventKindName(world::EventKind::Count)).empty() && std::string(world::EventKindName(world::EventKind::HallOfFame)) == "NPCHallOfFame",
			"EventKindName covers Count");
		world::WorldEvent E;
		E.Kind = world::EventKind::HallOfFame;
		E.Npc = W.Find("Mei");
		E.Amount = 420000000;
		E.Value = 2031;
		std::string Title;
		std::string Body;
		std::string Tag;
		Check(W.Headline(E, Title, Body, Tag) && Title == "Mei enters the Hall of Fame" && Body.rfind("Class of 2031: ", 0) == 0 && Body.back() == '.' && Tag == "HALL OF FAME",
			"a Hall of Fame class makes a headline (" + Body + ")");
		Check(copylint::Run("world", {{"Hall of Fame", Body}}, copylint::Pa) == 0, "the Hall of Fame's headline passes the copy lint");
	}
	// The world's sponsored: brands from the catalog (or Kast's, before it lands), never the site's or a card room's. (A poker
	// site's own team, Team RiverLine's or Team RunItTwice's, is the site's: update 2 §5.9.)
	{
		bool Fit = true;
		for (const world::Npc& N : W.People())
		{
			if (N.Pro || N.Sponsor.empty() || N.Sponsor == ss::net::Def(ss::net::SiteId::RunItTwice).TeamName)
			{
				continue;
			}
			const sponsor::Brand* Brand = sponsor::FindBrandByName(N.Sponsor);
			bool Kast = false;
			for (const kast::Sponsor& S : kast::Sponsors())
			{
				Kast = Kast || (S.Brand == N.Sponsor && S.Id != "riverline");
			}
			Fit = Fit && (Brand ? Brand->Cat != sponsor::Category::Site && Brand->Cat != sponsor::Category::CardRoom && Brand->Cat != sponsor::Category::Local : Kast);
		}
		Check(Fit, "the world's sponsored wear the catalog's brands, never a poker site's, a card room's or the block's");
	}
	std::printf("%s", W.Report().c_str());
	std::printf("%s", W.Describe(W.Find("gh0stfold")).c_str());
	Seasons();
	Migration();
	return Failures;
}

int Years(int Count, uint32_t Seed)
{
	world::World W;
	W.Create(Seed, NightOne);
	std::printf("%s\n", W.Report().c_str());
	for (int Y = 1; Y <= Count; ++Y)
	{
		const auto T = std::chrono::steady_clock::now();
		W.Simulate(365);
		const double S = Seconds(T);
		std::string Save;
		W.Write(Save);
		std::printf("---- year %d: %.1fs simulated, save %zu KB\n%s", Y, S, Save.size() / 1024, W.Report().c_str());
		const std::array<double, 6>& F = W.Flows();
		std::printf("  money: prizes $%.0fK, buy-ins $%.0fK (net $%.0fK), deposits and pay $%.0fK, living $%.0fK, cash-outs $%.0fK, travel $%.0fK\n", F[0] / 1e3, F[1] / 1e3,
			(F[0] - F[1]) / 1e3, F[2] / 1e3, F[3] / 1e3, F[4] / 1e3, F[5] / 1e3);
		for (const auto& K : W.FlowsByKind())
		{
			std::printf("    %-22s %7.0f entries  prizes $%9.0fK  buy-ins $%9.0fK  roi %+.0f%%\n", K.first.c_str(), K.second[2], K.second[0] / 1e3, K.second[1] / 1e3,
				K.second[1] > 0 ? (K.second[0] / K.second[1] - 1.0) * 100.0 : 0.0);
		}
		W.ResetFlows();
		Invariants(W, "year " + std::to_string(Y));
		// Who's on top, what happened this year.
		std::map<int, int> Kinds;
		for (const world::WorldEvent& E : W.Events())
		{
			if (E.At >= W.Clock() - 365.0 * 1440.0)
			{
				++Kinds[static_cast<int>(E.Kind)];
			}
		}
		std::printf("  this year's events:");
		for (const auto& K : Kinds)
		{
			std::printf(" %s %d", world::EventKindName(static_cast<world::EventKind>(K.first)), K.second);
		}
		std::printf("\n  honors this year:\n");
		for (const world::Honor& H : W.Honors())
		{
			if (H.At >= W.Clock() - 365.0 * 1440.0 && (H.Title.find("Main Event") != std::string::npos || H.Title.find("Year") != std::string::npos || H.Title.find("Summit") != std::string::npos))
			{
				const world::Npc* N = H.Npc >= 0 ? W.Get(H.Npc) : nullptr;
				std::printf("    %d %s: %s (%s, skill %.2f) $%lld\n", H.Year, H.Title.c_str(), H.Name.c_str(), N ? world::IdentityName(N->Is) : H.Npc == -1 ? "you" : "unknown", N ? static_cast<double>(N->Overall()) : 0.0,
					static_cast<long long>(H.Prize / 100));
			}
		}
	}
	// Every online bracelet that has been played for is in the history books too.
	for (const net::SeriesInfo& Sr : net::Shared().Series())
	{
		if (Sr.Bracelets == 0 || static_cast<double>(Sr.LastDay + 3) * net::MinutesPerDay > W.Clock() || static_cast<double>(Sr.FirstDay) * net::MinutesPerDay < W.StartedAt())
		{
			continue;
		}
		int Won = 0;
		for (const world::Honor& H : W.Honors())
		{
			const net::EventTemplate* T = net::Shared().FindTemplate(H.EventId.substr(0, H.EventId.find('@')));
			Won += T && T->Series == Sr.Id && T->Bracelet ? 1 : 0;
		}
		Check(Won == Sr.Bracelets, Sr.Name + ": every bracelet has a winner (" + std::to_string(Won) + " of " + std::to_string(Sr.Bracelets) + ")");
	}
	// Every big one that has finished has a champion in the history books.
	for (int Day = static_cast<int>(W.StartedAt() / 1440.0); Day < static_cast<int>(W.Clock() / 1440.0); ++Day)
	{
		for (const world::LiveEvent& E : world::LiveCalendar(Day))
		{
			if ((E.Kind == world::LiveKind::Summit || E.Kind == world::LiveKind::ChampionshipMain) && E.Start + E.Duration + 2.0 * 1440.0 < W.Clock())
			{
				bool Crowned = false;
				for (const world::Honor& H : W.Honors())
				{
					Crowned = Crowned || H.EventId == E.Id;
				}
				Check(Crowned, E.Name + " has a champion");
			}
		}
	}
	// The Hall of Fame (§5.3): classes of at most two a year, every member eligible on their July 15 and over the bar.
	{
		std::map<int, int> Classes;
		for (const world::Honor& H : W.Honors())
		{
			if (H.Title != world::HallOfFameTitle)
			{
				continue;
			}
			++Classes[H.Year];
			const world::Npc* N = W.Get(H.Npc);
			Check(N && N->Age(world::DayOn(H.Year, 7, 15)) >= 40 && H.Year - world::YearOf(N->Joined) >= 10 && H.Entries >= 12, "the Hall of Fame's " + std::to_string(H.Year) + " class: " + H.Name);
			std::printf("  Hall of Fame %d: %s (score %d, %s)\n", H.Year, H.Name.c_str(), H.Entries, N ? world::IdentityName(N->Is) : "?");
		}
		for (const auto& It : Classes)
		{
			Check(It.second <= 2, "at most two into the Hall of Fame in " + std::to_string(It.first));
		}
	}
	for (const char* Name : {"gh0stfold", "Mei", "Big Lou", "VikingVolta"})
	{
		std::printf("%s", W.Describe(W.Find(Name)).c_str());
	}
	return Failures;
}
} // namespace

namespace world_test
{
// For the per-track files (WorldSeg.h's kit): "world_test long", and the suite's invariants with their failures counted.
bool LongRun = false;
int CheckInvariants(const world::World& W, const std::string& When)
{
	const int Before = Failures;
	Invariants(W, When);
	return Failures - Before;
}
} // namespace world_test

int main(int Argc, char** Argv)
{
	// "world_test long": the long runs (update 2 SPEC §6.7: FreshEmerges' 365 days over three seeds).
	world_test::LongRun = Argc >= 2 && std::strcmp(Argv[1], "long") == 0;
	if (Argc >= 3 && std::strcmp(Argv[1], "years") == 0)
	{
		return Years(std::atoi(Argv[2]), Argc >= 4 ? static_cast<uint32_t>(std::strtoul(Argv[3], nullptr, 10)) : 7u) > 0 ? 1 : 0;
	}
	if (Argc >= 3 && std::strcmp(Argv[1], "npc") == 0)
	{
		world::World W;
		W.Create(7u, NightOne);
		W.Simulate(Argc >= 4 ? std::atoi(Argv[3]) : 30);
		std::printf("%s", W.Describe(W.Find(Argv[2])).c_str());
		return 0;
	}
	// The suite's own checks, then update 2's per-track files (update 2 SPEC §6.7; tests/world_seg_<track>.cpp).
	// (In order: the per-track files count their own failures, the suite's invariants included, after Checks has returned.)
	const int Own = Checks();
	const int Fresh = world_test::SegFresh();
	const int Sites = world_test::SegSites();
	const int F = Own + Fresh + Sites;
	std::printf(F == 0 ? "world: all checks passed\n" : "world: %d checks failed\n", F);
	return F == 0 ? 0 : 1;
}
