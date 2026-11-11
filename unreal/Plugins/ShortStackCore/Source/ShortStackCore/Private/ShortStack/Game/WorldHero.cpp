// The living world and the player: its clock, its events and boards as the screens ask for them, and what
// the people in it remember about the player.
#include "ShortStack/Game/World.h"

#include "ShortStack/Game/Badges.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Sites.h"
#include "WorldSim.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <limits>

namespace ss
{
namespace world
{
namespace worldhero_detail
{
Pending* FindPending(std::vector<Pending>& Queue, const std::string& Id)
{
	for (Pending& P : Queue)
	{
		if (P.Id == Id)
		{
			return &P;
		}
	}
	return nullptr;
}

/** The day an event id starts on ("embercrest@<day>", "dees@<day>", otherwise "<id>@<minute>"). */
int EventDay(const std::string& Id)
{
	const size_t At = Id.rfind('@');
	if (At == std::string::npos)
	{
		return -1;
	}
	const double V = std::atof(Id.c_str() + At + 1);
	const bool ByDay = live::IsEmbercrest(Id) || Id.rfind("dees@", 0) == 0;
	return ByDay ? static_cast<int>(V) : sim::DayAt(V);
}

float Add01(float V, double D)
{
	return sim::Clampf(static_cast<double>(V) + D, 0.0, 1.0);
}

/** Update 4 (SPEC v1.7 §2.6.2, §6 PA-25): a festival title whose template isn't a major (T.Main false: PENNANT's Micro Main). The
 *  template is the id before '@'; fest::IsTitleTemplate is false for every template until G1, so nothing is skipped before then. */
bool WorldHeroMinorTitle(const Honor& H)
{
	const std::string Template = H.EventId.substr(0, H.EventId.find('@'));
	if (!fest::IsTitleTemplate(Template))
	{
		return false;
	}
	const net::EventTemplate* T = net::ForEvent(H.EventId).FindTemplate(Template);
	return T && !T->Main;
}

/** The fewest an event runs with (the Embercrest's stated minimum; 0 where none is stated). */
int MinimumField(const std::string& Id)
{
	if (!live::IsEmbercrest(Id))
	{
		return 0;
	}
	const live::Occurrence O = live::FindOccurrence(Id);
	return O.Valid() ? O.T->MinEntrants : 0;
}

/** Update 4 (SPEC v1.7 §3.7 R4, §6 PA-35): the most roster pages one slice clones before it stops (with a finite budget). */
constexpr int HeroTurnCloneCap = 8;
/** And the people a ranged unit walks (§3.7 R2: ranges of 500, of PlanDay's shuffled order or of ids). */
constexpr int HeroTurnRange = 500;

/** Milliseconds since a time (the held turn's budget). */
double HeroTurnMs(std::chrono::steady_clock::time_point Since)
{
	return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - Since).count();
}

/** The HoldGuard's sink, process-wide (World::SetHoldGuardSink). */
World::HoldGuardSink& HeroHoldSink()
{
	static World::HoldGuardSink Sink = nullptr;
	return Sink;
}
} // namespace worldhero_detail

using namespace worldhero_detail;

// ------------------------------------------------------------------ time

// Update 4 (SPEC v1.7 §3.7, §6 PA-35; track A3): the held turn. AdvanceTo queues the time it's asked for as a target and runs the
// targets in order, each the way its own call always ran it: hour slices to the next hour, the day's end or the target, each opened
// by tomorrow's registrations (PlanDay's units) and a midnight closed by the day's turn (NewDay's units), and Rev moved once at the
// target's end if anything changed. With a finite TurnBudgetMs the run stops after any unit once the budget is spent (or a slice has
// cloned HeroTurnCloneCap roster pages), with the clock at the held instant; the next AdvanceTo, or a DrainHold, picks up exactly
// there. Unlimited (Phase A's default) never stops, so one call runs its target whole, as before. The units are A2's range splits
// (G/WorldSim.h's PlanBegin/PlanPeople/PlanEnd/PlanSort and NewDayOpen ... NewDayClose, with SS_WORLDSIM_TURN_PIECES defined there)
// or, without them, PlanDay and NewDay whole.

void World::AdvanceTo(double To)
{
	if (!Created)
	{
		return;
	}
	// The time the world has been asked to reach: past it, a target of its own (at or before it, nothing new: as before, an
	// earlier time changes nothing).
	const double Reached = HeldTurn.Targets.empty() ? Now : HeldTurn.Targets.back();
	if (To > Reached)
	{
		HeldTurn.Targets.push_back(To);
	}
	if (!HeldTurn.Targets.empty() && !HeldTurn.Running)
	{
		RunTurn(TurnBudgetMs, false);
	}
}

bool World::RunTurn(double BudgetMs, bool Drain)
{
	const std::chrono::steady_clock::time_point SliceStart = std::chrono::steady_clock::now();
	const bool Budgeted = !Drain && BudgetMs < std::numeric_limits<double>::infinity();
	HeldTurn.Running = true;
	HeldTurn.ClonesNow = 0;
	int Units = 0;
	bool Stopped = false;
	// The roster pages this call has cloned (A2's pages count them; without them, NoteTurnClone).
#if defined(SS_WORLDSIM_TURN_PIECES)
	const uint64_t ClonesBefore = PageClones();
	auto Cloned = [&]() { return static_cast<int>(PageClones() - ClonesBefore); };
#else
	auto Cloned = [&]() { return HeldTurn.ClonesNow; };
#endif
	// After each unit (and slice): has this call spent its budget?
	auto Spent = [&]() { return Budgeted && (BudgetMs <= 0.0 || Cloned() >= HeroTurnCloneCap || HeroTurnMs(SliceStart) >= BudgetMs); };
	auto Unit = [&](TurnUnit U, int Walked) {
		++Units;
		++HeldTurn.Stats.Units;
		++HeldTurn.Stats.ByUnit[static_cast<size_t>(U)];
		HeldTurn.Stats.MaxWalked = std::max(HeldTurn.Stats.MaxWalked, Walked);
		return Spent();
	};
	while (!HeldTurn.Targets.empty() && !Stopped)
	{
		const double To = HeldTurn.Targets.front();
		if (!HeldTurn.Started)
		{
			// The revision moves only when something the boards show did (the clock moves every frame; a ranking of the
			// world is kept until the world itself changes).
			HeldTurn.Started = true;
			HeldTurn.PlannedBefore = Planned;
			HeldTurn.Changed = false;
			HeldTurn.Phase = TurnPhase::Begin;
		}
		while (!Stopped)
		{
			if (HeldTurn.Phase == TurnPhase::Begin)
			{
				if (!(Now < To))
				{
					break;
				}
				HeldTurn.Day = sim::DayAt(Now);
				HeldTurn.DayEnd = static_cast<double>(HeldTurn.Day + 1) * 1440.0;
				HeldTurn.NextHour = (std::floor(Now / 60.0) + 1.0) * 60.0;
				HeldTurn.Limit = std::min({To, HeldTurn.DayEnd, HeldTurn.NextHour});
				HeldTurn.Cur = {};
				HeldTurn.Phase = TurnPhase::Plan;
			}
			if (HeldTurn.Phase == TurnPhase::Plan)
			{
				// Tomorrow's registrations are open.
				for (;;)
				{
					int Walked = 0;
					const TurnUnit U = TurnPlanUnit(HeldTurn.Day + 1, Walked);
					if (U == TurnUnit::Count)
					{
						break;
					}
					if (Unit(U, Walked))
					{
						Stopped = true;
						break;
					}
				}
				if (Stopped)
				{
					break;
				}
				HeldTurn.Phase = TurnPhase::Hour;
			}
			if (HeldTurn.Phase == TurnPhase::Hour)
			{
				const double Limit = HeldTurn.Limit;
				++HeldTurn.Stats.HourSlices;
				// A live event short of its stated minimum is called off when its cards should go in the air (R6): nobody plays
				// it or pays for it. One the player is in is their own room's to call (the session tells the world).
				for (size_t K = 0; K < Queue.size();)
				{
					const Pending& P = Queue[K];
					const int Minimum = !P.Online && P.HeroPlace == 0 && P.Start <= Limit ? MinimumField(P.Id) : 0;
					const int Came = std::max(P.Entries, static_cast<int>(P.Who.size()));
					if (Came < Minimum)
					{
						const std::string Id = P.Id;
						Cancel(Id, P.Start, Came);
						HeldTurn.Changed = true;
						continue;
					}
					++K;
				}
				// Everything that finishes by then.
				std::vector<Pending> Later;
				size_t Done = 0;
				for (; Done < Queue.size() && Queue[Done].End <= Limit; ++Done)
				{
					Pending& P = Queue[Done];
					if (P.HeroPlace < 0 && Limit < P.Start + 2.0 * 1440.0)
					{
						// The player is still at the tables: the result waits for them (not forever: a table walked away from
						// is decided without them).
						P.End = Limit + 15.0;
						Later.push_back(std::move(P));
						continue;
					}
					if (P.HeroPlace < 0)
					{
						P.HeroPlace = 0;
						HeroIn.erase(P.Id);
					}
					if (!P.Online)
					{
						// Who played it, a while longer (Entered): the block's people don't change their night once it's decided.
						std::vector<int>& Who = Played[P.Id].second;
						Played[P.Id].first = P.End;
						Who.clear();
						for (const Entry& E : P.Who)
						{
							Who.push_back(E.Npc);
						}
					}
					Sim::Resolve(*this, P);
				}
				Queue.erase(Queue.begin(), Queue.begin() + static_cast<std::ptrdiff_t>(Done));
				HeldTurn.Changed = HeldTurn.Changed || Done > 0;
				for (auto It = Played.begin(); It != Played.end();)
				{
					It = It->second.first < Limit - 2.0 * 1440.0 ? Played.erase(It) : std::next(It);
				}
				if (!Later.empty())
				{
					for (Pending& P : Later)
					{
						Queue.push_back(std::move(P));
					}
					std::stable_sort(Queue.begin(), Queue.end(), [](const Pending& A, const Pending& B) { return A.End < B.End; });
				}
				Now = Limit;
				if (Now >= HeldTurn.NextHour)
				{
					NightPointsHourAgo = NightPoints;
					NightHour = Now;
					HeldTurn.Changed = true;
					// Update 4 (SPEC v1.7 §2.6.1, §3.2 R2; §6 PA-24): the festival crown, and the Scene Book's 09:00 step (no-ops until G1 and G2).
					FestHour(*this, Now);
					scene::Hour(*this, Now);
				}
				HeldTurn.Cur = {};
				HeldTurn.Phase = Now >= HeldTurn.DayEnd ? TurnPhase::Day : TurnPhase::Begin;
				// A slice is a stopping point too (a skip's hours of results, or a load's catch-up, spread over frames like units;
				// §3.7 R2 counts the 09:00 slice like a unit). Not a unit: the structural checks count units.
				if (Now < To && Spent())
				{
					Stopped = true;
					break;
				}
			}
			if (HeldTurn.Phase == TurnPhase::Day)
			{
				// The day's turn, unit by unit.
				for (;;)
				{
					int Walked = 0;
					const TurnUnit U = TurnDayUnit(HeldTurn.Day + 1, Walked);
					if (U == TurnUnit::Count)
					{
						break;
					}
					HeldTurn.Changed = true;
					if (Unit(U, Walked))
					{
						Stopped = true;
						break;
					}
				}
				if (Stopped)
				{
					break;
				}
				HeldTurn.Changed = true;
				HeldTurn.Phase = TurnPhase::Begin;
			}
		}
		if (Stopped)
		{
			break;
		}
		// The target is reached.
		if (HeldTurn.Changed || Planned != HeldTurn.PlannedBefore)
		{
			++Rev;
		}
		HeldTurn.Targets.erase(HeldTurn.Targets.begin());
		HeldTurn.Started = false;
		HeldTurn.Phase = TurnPhase::Begin;
	}
	HeldTurn.Running = false;
#if defined(SS_WORLDSIM_TURN_PIECES)
	HeldTurn.Stats.Clones += Cloned(); // (NoteTurnClone counts them as they happen otherwise)
#endif
	if (Drain)
	{
		HeldTurn.Stats.MaxDrainUnits = std::max(HeldTurn.Stats.MaxDrainUnits, Units);
	}
	else
	{
		++HeldTurn.Stats.Calls;
		HeldTurn.Stats.Holds += Stopped ? 1 : 0;
		HeldTurn.Stats.MaxUnitsPerCall = std::max(HeldTurn.Stats.MaxUnitsPerCall, Units);
		HeldTurn.Stats.MaxClonesPerCall = std::max(HeldTurn.Stats.MaxClonesPerCall, Cloned());
		HeldTurn.Stats.MaxCallMs = std::max(HeldTurn.Stats.MaxCallMs, HeroTurnMs(SliceStart));
	}
	return !Stopped;
}

TurnUnit World::TurnPlanUnit(int Day, int& Walked)
{
#if defined(SS_WORLDSIM_TURN_PIECES)
	// A2's split (G/WorldSim.h): PlanDay(W, Day) is while (PlanBegin) { PlanPeople(all); PlanEnd; } PlanSort. The cursor: [0] the
	// piece (0 open, 1 ranges, 2 insert, 3 done), [1] the range's start, [2] the open plan's people. The first range runs with the
	// open (one unit: PlanDay), each further range of Sim's TurnRange is a unit (PlanDay), and the insertion with the next open or the
	// sort is one (PlanInsert). With nothing to plan only the sort runs (it's called every slice, as before), and that's no unit.
	std::array<uint32_t, 8>& C = HeldTurn.Cur;
	if (C[0] == 3)
	{
		return TurnUnit::Count;
	}
	if (C[0] == 2)
	{
		Sim::PlanEnd(*this);
		if (Sim::PlanBegin(*this, Day))
		{
			C[0] = 1;
			C[1] = 0;
			C[2] = static_cast<uint32_t>(Sim::PlanCount(*this));
		}
		else
		{
			Sim::PlanSort(*this);
			C[0] = 3;
		}
		return TurnUnit::PlanInsert;
	}
	if (C[0] == 0)
	{
		if (!Sim::PlanBegin(*this, Day))
		{
			Sim::PlanSort(*this);
			C[0] = 3;
			return TurnUnit::Count;
		}
		C[0] = 1;
		C[1] = 0;
		C[2] = static_cast<uint32_t>(Sim::PlanCount(*this));
	}
	const int From = static_cast<int>(C[1]);
	const int To = std::min(static_cast<int>(C[2]), From + HeroTurnRange);
	if (To > From)
	{
		Sim::PlanPeople(*this, From, To);
	}
	Walked = To - From;
	C[1] = static_cast<uint32_t>(To);
	C[0] = To >= static_cast<int>(C[2]) ? 2u : 1u;
	return TurnUnit::PlanDay;
#else
	// Without A2's splits: PlanDay whole, one unit when it plans a day (it's called every slice, as before: with nothing to plan it
	// only keeps the queue in order, and isn't a unit).
	if (HeldTurn.Cur[0] != 0)
	{
		return TurnUnit::Count;
	}
	HeldTurn.Cur[0] = 1;
	const bool Plans = Day > Planned;
	Walked = Plans ? static_cast<int>(People().size()) : 0;
	Sim::PlanDay(*this, Day);
	return Plans ? TurnUnit::PlanDay : TurnUnit::Count;
#endif
}

TurnUnit World::TurnDayUnit(int Day, int& Walked)
{
#if defined(SS_WORLDSIM_TURN_PIECES)
	// A2's split (G/WorldSim.h): NewDay(W, Day) is NewDayOpen; the month (when W.Turn.MonthTurned: NewMonthOpen, NewMonthPeople by
	// ranges, NewMonthClose); the week (when NewDayWeek: NewWeekOpen, NewWeekPeople by ranges, NewWeekClose); NewDayCalendar and
	// scene::Day; NewDayPeople by ranges; NewDayClose and Ranks; story::Day. Each piece is a unit, glue joined to its neighbour; each
	// loop's size is the roster's when the loop starts. The cursor: [0] the piece, [1] the range's start, [2] the loop's size.
	std::array<uint32_t, 8>& C = HeldTurn.Cur;
	enum : uint32_t
	{
		PcOpen,
		PcMonthOpen,
		PcMonthPeople,
		PcMonthClose,
		PcWeekOpen,
		PcWeekPeople,
		PcWeekClose,
		PcScene,
		PcDayPeople,
		PcClose,
		PcDirector,
		PcDone,
	};
	auto Ranges = [&](void (*Piece)(World&, int, int, int), uint32_t Next) {
		const int From = static_cast<int>(C[1]);
		const int To = std::min(static_cast<int>(C[2]), From + HeroTurnRange);
		if (To > From)
		{
			Piece(*this, Day, From, To);
		}
		Walked = To - From;
		C[1] = static_cast<uint32_t>(To);
		if (To >= static_cast<int>(C[2]))
		{
			C[0] = Next;
		}
	};
	auto Loop = [&](uint32_t Piece) {
		C[0] = Piece;
		C[1] = 0;
		C[2] = static_cast<uint32_t>(People().size());
	};
	switch (C[0])
	{
	case PcOpen:
		Sim::NewDayOpen(*this, Day);
		C[0] = Turn.MonthTurned ? PcMonthOpen : PcWeekOpen;
		return TurnUnit::NewDayPeople;
	case PcMonthOpen:
		Sim::NewMonthOpen(*this, Day);
		Loop(PcMonthPeople);
		return TurnUnit::NewMonthPeople;
	case PcMonthPeople:
		Ranges(&Sim::NewMonthPeople, PcMonthClose);
		return TurnUnit::NewMonthPeople;
	case PcMonthClose:
		Sim::NewMonthClose(*this, Day);
		C[0] = PcWeekOpen;
		return TurnUnit::NewMonthPeople;
	case PcWeekOpen:
		if (!Sim::NewDayWeek(*this, Day))
		{
			C[0] = PcScene;
			return TurnDayUnit(Day, Walked);
		}
		Sim::NewWeekOpen(*this, Day);
		Loop(PcWeekPeople);
		return TurnUnit::NewWeekPeople;
	case PcWeekPeople:
		Ranges(&Sim::NewWeekPeople, PcWeekClose);
		return TurnUnit::NewWeekPeople;
	case PcWeekClose:
		Sim::NewWeekClose(*this, Day);
		C[0] = PcScene;
		return TurnUnit::KeeperWeek;
	case PcScene:
		Sim::NewDayCalendar(*this, Day);
		scene::Day(*this, Day);
		Loop(PcDayPeople);
		return TurnUnit::SceneDay;
	case PcDayPeople:
		Ranges(&Sim::NewDayPeople, PcClose);
		return TurnUnit::NewDayPeople;
	case PcClose:
		Sim::NewDayClose(*this, Day);
		Sim::Ranks(*this);
		C[0] = PcDirector;
		return TurnUnit::Ranks;
	case PcDirector:
		story::Day(*this, Day);
		C[0] = PcDone;
		return TurnUnit::DirectorDay;
	default: return TurnUnit::Count;
	}
#else
	// Without A2's splits: NewDay whole, one unit (its per-person loops walk everyone).
	if (HeldTurn.Cur[0] != 0)
	{
		return TurnUnit::Count;
	}
	HeldTurn.Cur[0] = 1;
	Walked = static_cast<int>(People().size());
	Sim::NewDay(*this, Day);
	return TurnUnit::NewDayPeople;
#endif
}

bool World::TurnUnitsRanged()
{
#if defined(SS_WORLDSIM_TURN_PIECES)
	return true;
#else
	return false;
#endif
}

double World::ForcedTurnBudgetMs()
{
#if defined(UE_BUILD_DEBUG) || defined(UE_BUILD_DEVELOPMENT) || defined(UE_BUILD_TEST) || defined(UE_BUILD_SHIPPING)
	return std::numeric_limits<double>::infinity(); // (a test switch: the game's budget is the session's, §3.7 R1)
#else
	const char* Text = std::getenv("SS_TURN_BUDGET");
	if (!Text || !*Text)
	{
		return std::numeric_limits<double>::infinity();
	}
	char* End = nullptr;
	const double Ms = std::strtod(Text, &End);
	return End != Text && *End == '\0' && Ms >= 0.0 ? Ms : std::numeric_limits<double>::infinity();
#endif
}

World::HoldGuardSink World::SetHoldGuardSink(HoldGuardSink Sink)
{
	HoldGuardSink& Slot = HeroHoldSink();
	const HoldGuardSink Was = Slot;
	Slot = Sink;
	return Was;
}

bool World::Holding() const
{
	// §3.7, PA-35: a target is still to reach (a turn stopped mid-way at its budget). Never with an unlimited budget.
	return !HeldTurn.Targets.empty();
}

void World::DrainHold()
{
	// §3.7 R3, PA-35: every target asked for so far, to the end, now: the world stands where the calls that asked for them would have
	// left it. (Inside the machine already: nothing to do.)
	if (HeldTurn.Targets.empty() || HeldTurn.Running)
	{
		return;
	}
	++HeldTurn.Stats.Drains;
	RunTurn(std::numeric_limits<double>::infinity(), true);
}

void World::HoldGuard(const char* Method) const
{
	// The HoldGuard (§3.7 R3, rev-code M2): a non-display method called while the world holds a turn, outside the machine, would read
	// or change a half-turned world. The caller drains first (Session's drain-on-touch); this catches the calls a grep misses. Display
	// reads never check (the allowlist: BoardValue, RankBefore, Leaders, RoomLeaders, CircuitRank, CardCutLine, PlayerOfYearTitles,
	// HallClass, HallScore, HallEligible, Holding, TurnCounters and the inline views in World.h). Test builds report to the sink, or
	// abort; an Unreal build only counts.
	if (HeldTurn.Targets.empty() || HeldTurn.Running)
	{
		return;
	}
	++TurnGuardTrips;
#if !(defined(UE_BUILD_DEBUG) || defined(UE_BUILD_DEVELOPMENT) || defined(UE_BUILD_TEST) || defined(UE_BUILD_SHIPPING))
	if (const HoldGuardSink Sink = HeroHoldSink())
	{
		Sink(Method);
		return;
	}
	std::fprintf(stderr, "HoldGuard: World::%s called while the world holds a turn (drain first, SPEC v1.7 §3.7 R3)\n", Method);
	std::fflush(stderr);
	std::abort();
#else
	(void)Method;
#endif
}

void World::Simulate(int DayCount)
{
	if (DayCount > 0)
	{
		// From the time the world has been asked to reach, and to the end (a debug run, and WorldPreview's copy, want all of it).
		const double From = HeldTurn.Targets.empty() ? Now : HeldTurn.Targets.back();
		AdvanceTo(From + static_cast<double>(DayCount) * 1440.0);
		DrainHold();
	}
}

// ------------------------------------------------------------------ events and boards

const net::EventResult* World::ResultOf(const std::string& EventId) const
{
	HoldGuard("ResultOf");
	const auto It = Results.find(EventId);
	return It == Results.end() ? nullptr : &It->second;
}

const Cancellation* World::CancellationOf(const std::string& EventId) const
{
	HoldGuard("CancellationOf");
	const auto It = Cancelled.find(EventId);
	return It == Cancelled.end() ? nullptr : &It->second;
}

void World::Cancel(const std::string& EventId, double At, int Entrants)
{
	HoldGuard("Cancel");
	if (!Created || Results.count(EventId) > 0)
	{
		return; // (an event already decided stays decided: its winner was paid)
	}
	if (Cancelled.count(EventId) == 0)
	{
		Cancellation C;
		C.At = At;
		C.Field = Entrants;
		Cancelled[EventId] = C;
	}
	// Buy-ins come out when an event is decided, so the people registered for it simply never pay; nobody wins it.
	Queue.erase(std::remove_if(Queue.begin(), Queue.end(), [&](const Pending& P) { return P.Id == EventId; }), Queue.end());
	HeroIn.erase(EventId);
	++Rev;
	++HeroRev;
}

bool World::FinishIn(const std::string& EventId, int Npc, int& Place, Chips& Prize) const
{
	HoldGuard("FinishIn");
	const auto It = FieldPlaces.find(EventId);
	if (It == FieldPlaces.end())
	{
		return false;
	}
	for (const Placed& P : It->second)
	{
		if (P.Npc == Npc)
		{
			Place = P.Place;
			Prize = P.Prize;
			return true;
		}
	}
	return false;
}

std::string World::NextEmbercrest(int Npc, double From) const
{
	HoldGuard("NextEmbercrest");
	const int Today = sim::DayAt(From);
	for (int Day = Today; Day <= Today + 1; ++Day)
	{
		for (const live::Occurrence& O : live::Occurrences(Day))
		{
			if (O.Start <= From)
			{
				continue;
			}
			for (const Pending& P : Queue)
			{
				if (P.Id == O.Id && std::any_of(P.Who.begin(), P.Who.end(), [&](const Entry& E) { return E.Npc == Npc; }))
				{
					return O.Id;
				}
			}
		}
	}
	return std::string();
}

void World::EnsurePlanned(int Day)
{
	// Update 4 (SPEC v1.7 §3.7, §6 PA-35): a held turn first finishes (the plan, and the clock it's checked against, are the finished
	// world's).
	DrainHold();
	// Tomorrow at most: registrations further out don't exist yet (and looking doesn't change the world).
	if (Created && Day > Planned && Day <= sim::DayAt(Now) + 1)
	{
		Sim::PlanDay(*this, Day);
	}
}

std::vector<int> World::RegisteredNow(const std::string& EventId) const
{
	HoldGuard("RegisteredNow");
	std::vector<int> Out;
	for (const Pending& P : Queue)
	{
		if (P.Id == EventId)
		{
			for (const Entry& E : P.Who)
			{
				Out.push_back(E.Npc);
			}
			break;
		}
	}
	return Out;
}

std::vector<int> World::Entered(const std::string& EventId) const
{
	HoldGuard("Entered");
	const auto It = Played.find(EventId);
	return It != Played.end() ? It->second.second : RegisteredNow(EventId);
}

std::vector<int> World::Registered(const std::string& EventId)
{
	HoldGuard("Registered");
	std::vector<int> Out;
	const int Day = EventDay(EventId);
	if (Day >= 0)
	{
		EnsurePlanned(Day);
	}
	if (const Pending* P = FindPending(Queue, EventId))
	{
		for (const Entry& E : P->Who)
		{
			Out.push_back(E.Npc);
		}
	}
	return Out;
}

double World::BoardValue(net::Board B, int Npc, double At) const
{
	const world::Npc* N = Get(Npc);
	if (!N)
	{
		return 0.0;
	}
	const bool ThisYear = N->ThisSeason.Year == YearOf(sim::DayAt(At));
	switch (B)
	{
	case net::Board::Earnings: return static_cast<double>(N->Totals[static_cast<size_t>(Venue::Online)].Won);
	case net::Board::Season: return ThisYear ? N->ThisSeason.Points : 0.0;
	case net::Board::Wins: return ThisYear ? static_cast<double>(N->ThisSeason.Wins) : 0.0;
	case net::Board::FinalTables: return ThisYear ? static_cast<double>(N->ThisSeason.FinalTables) : 0.0;
	case net::Board::Live: return ThisYear ? N->ThisSeason.LivePoints : 0.0;
	case net::Board::Series:
	{
		const auto It = SeriesPoints.find(Npc);
		return It == SeriesPoints.end() ? 0.0 : It->second;
	}
	case net::Board::NightShift:
	{
		if (life::NightShiftStart(At) != NightKey)
		{
			return 0.0;
		}
		const auto It = NightPoints.find(Npc);
		return It == NightPoints.end() ? 0.0 : It->second;
	}
	}
	return 0.0;
}

int World::RankBefore(net::Board B, int Npc) const
{
	if (B == net::Board::NightShift)
	{
		const auto Mine = NightPointsHourAgo.find(Npc);
		const double V = Mine == NightPointsHourAgo.end() ? 0.0 : Mine->second;
		// Nothing an hour ago is no rank at all (update 2 §3.6): the row reads NEW.
		if (!(V > 0.0))
		{
			return 0;
		}
		// 1 + how many had more. Update 4 (SPEC v1.7 §6 PA-33): the hour-ago values are ranked once when they move (the hour mark
		// copies them, a new night clears them, a load reads them), not counted again for every row of the board: O(log m) a row.
		if (!NightAgoBuilt || NightAgoHour != NightHour || NightAgoSize != NightPointsHourAgo.size())
		{
			NightAgoSorted.clear();
			NightAgoSorted.reserve(NightPointsHourAgo.size());
			for (const auto& It : NightPointsHourAgo)
			{
				NightAgoSorted.push_back(It.second);
			}
			std::sort(NightAgoSorted.begin(), NightAgoSorted.end(), std::greater<double>());
			NightAgoHour = NightHour;
			NightAgoSize = NightPointsHourAgo.size();
			NightAgoBuilt = true;
		}
		const auto Ahead = std::lower_bound(NightAgoSorted.begin(), NightAgoSorted.end(), V, std::greater<double>());
		return 1 + static_cast<int>(Ahead - NightAgoSorted.begin());
	}
	// 0: nothing on the board a day ago (Sim::Ranks), or someone who wasn't in the world yet. RanksYesterday ends at Live: a
	// rival's board (update 2 §5.8, appended after it) keeps its ranks in the site maps, so it has none here.
	if (static_cast<size_t>(B) >= RanksYesterday.size())
	{
		return 0;
	}
	const std::vector<int>& Rk = RanksYesterday[static_cast<size_t>(B)];
	return Npc >= 0 && static_cast<size_t>(Npc) < Rk.size() ? Rk[static_cast<size_t>(Npc)] : 0;
}

std::vector<int> World::Leaders(Rep R, int Count) const
{
	std::vector<int> Order;
	for (const Npc& N : People())
	{
		if (N.St != Status::Retired)
		{
			Order.push_back(N.Id);
		}
	}
	std::stable_sort(Order.begin(), Order.end(), [&](int A, int B) { return People()[static_cast<size_t>(A)].RepOf(R) > People()[static_cast<size_t>(B)].RepOf(R); });
	if (static_cast<int>(Order.size()) > Count)
	{
		Order.resize(static_cast<size_t>(std::max(0, Count)));
	}
	return Order;
}

// ------------------------------------------------------------------ the player

void World::HeroEntered(const std::string& EventId, const std::string& EventName)
{
	HoldGuard("HeroEntered");
	if (!Created || Cancelled.count(EventId) > 0)
	{
		return;
	}
	Registered(EventId); // plans its day if it hasn't been
	Pending* P = FindPending(Queue, EventId);
	if (!P)
	{
		// Nobody the world follows is in it: the player's result still makes it a world event.
		net::EventInstance E;
		if (net::ForEvent(EventId).FindInstance(EventId, E))
		{
			Queue.push_back(Sim::OnlineEvent(*this, E));
		}
		else
		{
			const int Day = EventDay(EventId);
			for (const LiveEvent& L : LiveCalendar(Day >= 0 ? Day : sim::DayAt(Now)))
			{
				if (L.Id == EventId)
				{
					Queue.push_back(Sim::LiveEventOf(*this, L));
				}
			}
		}
		std::stable_sort(Queue.begin(), Queue.end(), [](const Pending& A, const Pending& B) { return A.End < B.End; });
		P = FindPending(Queue, EventId);
	}
	if (P)
	{
		P->HeroPlace = -1;
		if (!EventName.empty())
		{
			P->Name = EventName;
		}
	}
	HeroIn.insert(EventId);
	++Rev;
	++HeroRev;
}

int World::PlannedEntries(const std::string& EventId)
{
	HoldGuard("PlannedEntries");
	if (Cancelled.count(EventId) > 0)
	{
		return 0; // called off: nobody plays it
	}
	Registered(EventId); // plans its day if it hasn't been
	if (const Pending* P = FindPending(Queue, EventId))
	{
		return P->Entries;
	}
	// Nobody the world follows is in it: the crowd it has anyway.
	net::EventInstance E;
	if (net::ForEvent(EventId).FindInstance(EventId, E))
	{
		return Sim::OnlineEvent(*this, E).Entries;
	}
	const int Day = EventDay(EventId);
	for (const LiveEvent& L : LiveCalendar(Day >= 0 ? Day : sim::DayAt(Now)))
	{
		if (L.Id == EventId)
		{
			return Sim::LiveEventOf(*this, L).Entries;
		}
	}
	return 0;
}

void World::Join(const std::string& EventId, int Npc)
{
	HoldGuard("Join");
	Pending* P = FindPending(Queue, EventId);
	if (!P || !Get(Npc))
	{
		return;
	}
	for (const Entry& E : P->Who)
	{
		if (E.Npc == Npc)
		{
			return;
		}
	}
	Entry E;
	E.Npc = Npc;
	P->Who.push_back(E);
	++Rev;
	++HeroRev;
}

void World::GrantAward(int Npc, bool Ring, bool Main)
{
	HoldGuard("GrantAward");
	if (!Created || (Npc != -1 && !Get(Npc)))
	{
		return;
	}
	const net::Network& Net = net::Shared();
	const int Today = sim::DayAt(Now);
	const net::SeriesInfo* Pick = nullptr;
	for (const net::SeriesInfo& Sr : Net.Series())
	{
		if ((Ring ? Sr.Rings : Sr.Bracelets) == 0)
		{
			continue;
		}
		// The latest to have started; before the first, the first.
		const bool Better = Sr.FirstDay <= Today ? !Pick || Pick->FirstDay > Today || Sr.FirstDay > Pick->FirstDay : !Pick || (Pick->FirstDay > Today && Sr.FirstDay < Pick->FirstDay);
		Pick = Better ? &Sr : Pick;
	}
	if (!Pick)
	{
		return;
	}
	const net::EventTemplate* T = Main ? Net.FindTemplate(Pick->MainEvent) : nullptr;
	for (const net::EventTemplate& E : Net.Templates())
	{
		if (!T && E.Series == Pick->Id && (Ring ? E.Ring : E.Bracelet))
		{
			T = &E;
		}
	}
	Award A;
	A.Day = Today;
	A.Ring = Ring;
	A.Main = T && T->Id == Pick->MainEvent;
	A.Series = Pick->Id;
	A.Event = T ? T->Name : Pick->Name;
	A.Prize = T ? T->GtdCents / 5 : 0;
	A.Entries = T ? T->Field : 0;
	A.Tourney = Npc == -1 ? HeroBook.Events : People()[static_cast<size_t>(Npc)].Stats.Events;
	if (Npc == -1)
	{
		HeroTrophies.push_back(A);
		++HeroRev;
	}
	else
	{
		world::Npc& N = Edit(Npc);
		N.Awards.push_back(A);
		N.Bracelets += Ring ? 0 : 1;
		N.Rings += Ring ? 1 : 0;
		Refresh(Npc);
	}
	++Rev;
}

void World::SetPlannedEntries(const std::string& EventId, int Entries)
{
	HoldGuard("SetPlannedEntries");
	if (!Created)
	{
		return;
	}
	Registered(EventId); // plans its day if it can be
	Pending* P = FindPending(Queue, EventId);
	if (!P)
	{
		return;
	}
	const int Was = std::max(1, P->Entries);
	P->Entries = std::max(1, Entries);
	P->Pool = P->Pool * P->Entries / Was;
	if (static_cast<int>(P->Who.size()) > P->Entries)
	{
		P->Who.resize(static_cast<size_t>(P->Entries));
	}
	++Rev;
	++HeroRev; // (saved with the next save, like anything the player's nights depend on)
}

void World::KnownPlaces(const std::string& EventId, const std::vector<std::pair<int, int>>& Places)
{
	HoldGuard("KnownPlaces");
	Pending* P = Created ? FindPending(Queue, EventId) : nullptr;
	if (!P)
	{
		return;
	}
	for (const std::pair<int, int>& It : Places)
	{
		Entry* Found = nullptr;
		for (Entry& E : P->Who)
		{
			if (E.Npc == It.first)
			{
				Found = &E;
				break;
			}
		}
		if (!Found && Get(It.first))
		{
			Entry E;
			E.Npc = It.first;
			P->Who.push_back(E);
			Found = &P->Who.back();
		}
		if (Found)
		{
			Found->Known = It.second;
		}
	}
}

void World::HeroFinished(const std::string& EventId, int Place, Chips Prize, const TableReport& Report)
{
	HoldGuard("HeroFinished");
	if (!Created)
	{
		return;
	}
	HeroIn.erase(EventId);
	const bool Again = HeroResults.count(EventId) > 0;
	HeroResults[EventId] = {Place, Prize};
	Pending* P = FindPending(Queue, EventId);
	const std::string Where = P ? P->Name : EventId;
	const double At = Now;
	if (P)
	{
		P->HeroPlace = std::max(1, Place);
		P->HeroPrize = Prize;
		auto EntryOf = [&](int Npc) -> Entry* {
			for (Entry& E : P->Who)
			{
				if (E.Npc == Npc)
				{
					return &E;
				}
			}
			// Seated with the player but not on the world's list (a late registration): they played it.
			if (Get(Npc))
			{
				Entry E;
				E.Npc = Npc;
				P->Who.push_back(E);
				return &P->Who.back();
			}
			return nullptr;
		};
		for (const std::pair<int, int>& It : Report.Places)
		{
			if (Entry* E = EntryOf(It.first))
			{
				E->Known = It.second;
			}
		}
		for (int Npc : Report.StillIn)
		{
			if (Entry* E = EntryOf(Npc))
			{
				E->Better = P->HeroPlace;
			}
		}
		const Pending& Ev = *P;
		const bool Online = Ev.Online;
		const double Pts = Ev.BuyIn > 0 ? net::Points(Place, Ev.Entries, Ev.BuyIn) : 0.0;
		if (Online && !Again && net::SiteOf(EventId) == net::SiteId::RiverLine)
		{
			// (Once: a result told twice isn't two results. RiverLine's points only: Player of the Year is RiverLine's title, and
			// a rival's points stay in the player's book on that site, update 2 §5.8. The stats page below is every site's.)
			HeroPoints += Pts;
		}
		else if (!Online && !Again && CircuitEvent(static_cast<LiveKind>(Ev.Kind), (Ev.Kinds & KindSatellite) != 0))
		{
			// Live: Regional events and up (§11 Q3), the same points for the circuit's cards.
			HeroLivePoints += Pts;
			HeroCircuit += Pts;
		}
		if (Online && !Again)
		{
			// Update 4 (SPEC v1.7 §2.5.2 rule 3, §6 PA-24): the hero's finish in the live festival's race, any site's (a no-op until G1).
			FestScoreHero(*this, Ev, Place);
		}
		// The player's stats page (a satellite seat at its value).
		if (!Again)
		{
			const bool Seat = (Ev.Kinds & KindSatellite) != 0 && Place >= 1 && Place <= Ev.Seats;
			HeroBook.Add(Ev.BuyIn, Prize + (Seat ? Ev.SeatValue : 0), Place, Ev.Entries, Ev.FinalSize, Sim::StakeOf(Ev), Sim::FormatOf(Ev));
			if (!Online && live::IsEmbercrest(EventId))
			{
				Sim::NoteRoom(HeroRoomRec, sim::DayAt(Ev.Start), Place, Ev.FinalSize, Prize, Pts);
			}
		}
		// A bracelet or a ring for the player: into the trophy case (once).
		if (Place == 1 && (Ev.Bracelet || Ev.Ring))
		{
			Award A = Sim::AwardOf(Ev, sim::DayAt(At), Prize);
			A.Tourney = HeroBook.Events;
			const bool Had = std::any_of(HeroTrophies.begin(), HeroTrophies.end(), [&](const Award& X) { return X.Day == A.Day && X.Event == A.Event; });
			if (!Had)
			{
				HeroTrophies.push_back(A);
			}
		}
	}
	// Update 4 (§3.5 R1, §3.13, §3.17 R5; §6 PA-24): the wake of a public result, the hero's TPI cash, the hero's notes (no-ops until
	// G2; each keyed or folded once, so a result told twice is still one result).
	scene::OnHeroResult(*this, EventId, Place, Prize);
	story::OnHeroResult(*this, EventId, Place, Prize);
	NoteObserved(Report.Seen);
	// What the people at the player's tables will remember.
	for (int Npc : Report.Met)
	{
		Remember(Npc, MemoryKind::Met, Where, 0, At);
	}
	if (Report.KnockedOutHero >= 0)
	{
		Remember(Report.KnockedOutHero, MemoryKind::KnockedOutHero, Where, Prize, At);
	}
	for (int Npc : Report.HeroKnockedOut)
	{
		Remember(Npc, MemoryKind::HeroKnockedOut, Where, 0, At);
	}
	for (const std::pair<int, Chips>& It : Report.BigPots)
	{
		Remember(It.first, It.second > 0 ? MemoryKind::BigPotWon : MemoryKind::BigPotLost, Where, It.second > 0 ? It.second : -It.second, At);
	}
	const int Final = P ? P->FinalSize : 9;
	if (Place >= 1 && Place <= Final)
	{
		for (const std::pair<int, int>& It : Report.Places)
		{
			if (It.second <= Final)
			{
				Remember(It.first, MemoryKind::FinalTable, Where, 0, At);
			}
		}
		for (int Npc : Report.StillIn)
		{
			Remember(Npc, MemoryKind::FinalTable, Where, 0, At);
		}
	}
	// Heads-up for the title.
	if (Place == 2 && Report.KnockedOutHero >= 0)
	{
		Remember(Report.KnockedOutHero, MemoryKind::HeadsUpLost, Where, Prize, At);
	}
	if (Place == 1)
	{
		for (const std::pair<int, int>& It : Report.Places)
		{
			if (It.second == 2)
			{
				Remember(It.first, MemoryKind::HeadsUpWon, Where, Prize, At);
			}
		}
	}
	++Rev;
	++HeroRev;
}

void World::BackRoomNight(double At, const std::vector<std::string>& Names, Chips HeroNet)
{
	HoldGuard("BackRoomNight");
	for (const std::string& Name : Names)
	{
		const int Id = Find(Name);
		if (Id >= 0)
		{
			Remember(Id, MemoryKind::BackRoom, "Dee's game", HeroNet, At);
		}
	}
	++Rev;
	++HeroRev;
}

void World::EmbercrestDone(double At, const std::vector<std::pair<std::string, int>>& Places, int HeroPlace, int FieldSize)
{
	HoldGuard("EmbercrestDone");
	const int Day = sim::DayAt(At - 6.0 * 60.0); // it starts at 7 PM and can run past midnight
	const std::string Id = "embercrest@" + std::to_string(Day);
	Registered(Id);
	Pending* P = FindPending(Queue, Id);
	if (P)
	{
		P->HeroPlace = std::max(1, HeroPlace);
		P->Entries = std::max(P->Entries, FieldSize);
	}
	for (const std::pair<std::string, int>& It : Places)
	{
		const int Npc = Find(It.first);
		if (Npc < 0)
		{
			continue;
		}
		if (P)
		{
			bool Found = false;
			for (Entry& E : P->Who)
			{
				if (E.Npc == Npc)
				{
					E.Known = It.second;
					Found = true;
				}
			}
			if (!Found)
			{
				Entry E;
				E.Npc = Npc;
				E.Known = It.second;
				P->Who.push_back(E);
			}
		}
		Remember(Npc, MemoryKind::Embercrest, "the Embercrest Sunday", 0, At);
		if (HeroPlace >= 1 && HeroPlace <= 6 && It.second <= 6)
		{
			Remember(Npc, MemoryKind::FinalTable, "the Embercrest Sunday", 0, At);
		}
	}
	if (HeroResults.count(Id) == 0 && HeroPlace >= 1)
	{
		// The room's record (an older save's note: the place, not the prize).
		const live::Occurrence O = live::FindOccurrence(Id);
		const Chips BuyIn = P ? P->BuyIn : O.Valid() ? O.T->BuyInCents : 0;
		const double Points = BuyIn > 0 ? net::Points(HeroPlace, std::max(FieldSize, P ? P->Entries : 0), BuyIn) : 0.0;
		Sim::NoteRoom(HeroRoomRec, Day, HeroPlace, live::EmbercrestTableSize, 0, Points);
	}
	HeroResults[Id] = {HeroPlace, 0};
	scene::OnRoom(*this, Id, HeroPlace, FieldSize); // Update 4 (§3.5 R1, §6 PA-24): a room win's Room moment (no-op until G2)
	++Rev;
	++HeroRev;
}

void Sim::Feel(Bond& B, MemoryKind K, float Emotion)
{
	const double Hot = 1.0 - static_cast<double>(Emotion); // the ones who can't let things go
	switch (K)
	{
	case MemoryKind::Met: B.Familiarity = Add01(B.Familiarity, 0.08); break;
	case MemoryKind::KnockedOutHero:
		B.Familiarity = Add01(B.Familiarity, 0.08);
		B.Rivalry = Add01(B.Rivalry, 0.1);
		break;
	case MemoryKind::HeroKnockedOut:
		B.Familiarity = Add01(B.Familiarity, 0.08);
		B.Rivalry = Add01(B.Rivalry, 0.15);
		B.Respect = Add01(B.Respect, 0.06);
		B.Resentment = Add01(B.Resentment, 0.15 * Hot);
		break;
	case MemoryKind::BigPotWon:
		B.Respect = Add01(B.Respect, 0.05);
		B.Resentment = Add01(B.Resentment, 0.08 * Hot);
		B.Familiarity = Add01(B.Familiarity, 0.04);
		break;
	case MemoryKind::BigPotLost:
		B.Familiarity = Add01(B.Familiarity, 0.04);
		B.Respect = Add01(B.Respect, -0.02);
		break;
	case MemoryKind::FinalTable:
		B.Respect = Add01(B.Respect, 0.08);
		B.Familiarity = Add01(B.Familiarity, 0.1);
		break;
	case MemoryKind::HeadsUpWon:
		B.Rivalry = Add01(B.Rivalry, 0.3);
		B.Respect = Add01(B.Respect, 0.2);
		B.Resentment = Add01(B.Resentment, 0.12 * Hot);
		break;
	case MemoryKind::HeadsUpLost:
		B.Rivalry = Add01(B.Rivalry, 0.15);
		B.Respect = Add01(B.Respect, 0.08);
		break;
	case MemoryKind::BackRoom:
		B.Familiarity = Add01(B.Familiarity, 0.08);
		B.Trust = Add01(B.Trust, 0.05);
		break;
	case MemoryKind::Embercrest: B.Familiarity = Add01(B.Familiarity, 0.06); break;
	case MemoryKind::ShowedBluff: B.Rivalry = Add01(B.Rivalry, 0.05); break;
	case MemoryKind::Street: B.Familiarity = Add01(B.Familiarity, 0.03); break;
	// Money (next_segment_spec §2.9): a stake or a piece is a bet on the player; how the player settles up is what stays.
	case MemoryKind::Staked: B.Familiarity = Add01(B.Familiarity, 0.04); break;
	case MemoryKind::BoughtPiece: B.Familiarity = Add01(B.Familiarity, 0.03); break;
	case MemoryKind::Swapped:
		B.Familiarity = Add01(B.Familiarity, 0.03);
		B.Respect = Add01(B.Respect, 0.02);
		break;
	case MemoryKind::PaidUp: B.Trust = Add01(B.Trust, 0.04); break;
	case MemoryKind::PaidLate: B.Trust = Add01(B.Trust, -0.10); break;
	case MemoryKind::Welched:
		B.Trust = Add01(B.Trust, -0.40);
		B.Resentment = Add01(B.Resentment, 0.40);
		break;
	case MemoryKind::WalkedOut:
		B.Trust = Add01(B.Trust, -0.30);
		B.Resentment = Add01(B.Resentment, 0.25);
		break;
	default: break;
	}
}

void Sim::Fade(World& W)
{
	// A month: grudges cool, faces blur a little; respect lasts.
	for (auto& It : W.HeroBonds)
	{
		Bond& B = It.second;
		B.Familiarity = sim::Clampf(static_cast<double>(B.Familiarity) * 0.98, 0.0, 1.0);
		B.Resentment = sim::Clampf(static_cast<double>(B.Resentment) * 0.95, 0.0, 1.0);
		B.Rivalry = sim::Clampf(static_cast<double>(B.Rivalry) * 0.985, 0.0, 1.0);
	}
}

void World::Remember(int Npc, MemoryKind Kind, const std::string& Where, Chips Amount, double At)
{
	HoldGuard("Remember");
	const world::Npc* N = Get(Npc);
	if (!N)
	{
		return;
	}
	Bond& B = HeroBonds[Npc];
	const int Day = sim::DayAt(At);
	const bool First = B.Memories.empty();
	if (Kind == MemoryKind::Street && !First)
	{
		// Talk on the block counts once a day, and a run of it is one memory, kept current: chatting with Sal at Ruby's
		// every morning doesn't push the night he knocked the player out off the end of what he remembers.
		for (const Memory& Had : B.Memories)
		{
			if (Had.Kind == MemoryKind::Street && Had.Day == Day)
			{
				return;
			}
		}
		if (B.Memories.size() > 1 && B.Memories.back().Kind == MemoryKind::Street)
		{
			if (B.LastDay != Day)
			{
				++B.Encounters;
			}
			B.LastDay = Day;
			Sim::Feel(B, Kind, N->SkillOf(Skill::Emotion));
			B.Memories.back().Day = Day;
			B.Memories.back().Where = Where;
			++HeroRev;
			return;
		}
	}
	if (Kind == MemoryKind::Met && !First)
	{
		// They know the player already: it's another encounter, not a first meeting.
		if (B.LastDay != Day)
		{
			++B.Encounters;
			B.Familiarity = Add01(B.Familiarity, 0.03);
		}
		B.LastDay = Day;
		return;
	}
	if (B.LastDay != Day)
	{
		++B.Encounters;
	}
	B.LastDay = Day;
	Sim::Feel(B, Kind, N->SkillOf(Skill::Emotion));
	Memory M;
	M.Day = Day;
	M.Kind = Kind;
	M.Where = Where;
	M.Amount = Amount;
	B.Memories.push_back(M);
	++HeroRev;
	if (B.Memories.size() > 12)
	{
		B.Memories.erase(B.Memories.begin() + 1); // the first meeting is always kept
	}
}

void World::Lent(int Npc, bool Repaid, double At)
{
	HoldGuard("Lent");
	if (!Get(Npc))
	{
		return;
	}
	Bond& B = HeroBonds[Npc];
	B.Trust = Add01(B.Trust, Repaid ? 0.1 : 0.06);
	B.Familiarity = Add01(B.Familiarity, 0.02);
	B.LastDay = std::max(B.LastDay, sim::DayAt(At));
	++Rev;
	++HeroRev;
}

bool World::Bumped(int Npc, double At)
{
	HoldGuard("Bumped");
	if (!Get(Npc))
	{
		return false;
	}
	const auto Last = Bumps.find(Npc);
	if (Last != Bumps.end() && At >= Last->second && At - Last->second < BumpSameSeconds / 60.0)
	{
		return false; // the same collision, still touching
	}
	const bool Again = Last != Bumps.end() && At - Last->second < 1440.0;
	Bumps[Npc] = At;
	if (!Again)
	{
		return false;
	}
	Bond& B = HeroBonds[Npc];
	B.Resentment = Add01(B.Resentment, 0.02);
	++HeroRev;
	return true;
}

const Bond* World::BondWith(int Npc) const
{
	HoldGuard("BondWith");
	const auto It = HeroBonds.find(Npc);
	return It == HeroBonds.end() ? nullptr : &It->second;
}

std::string Bond::Label() const
{
	if (Rivalry >= 0.5f)
	{
		return "Rival";
	}
	if (Resentment >= 0.4f)
	{
		return "Holds a grudge";
	}
	if (Respect >= 0.5f)
	{
		return "Respects you";
	}
	if (Trust >= 0.4f)
	{
		return "Trusts you";
	}
	if (Familiarity >= 0.5f)
	{
		return "Knows you well";
	}
	if (Familiarity >= 0.12f || Encounters >= 2)
	{
		return "Knows you";
	}
	return "";
}

// ------------------------------------------------------------------ the badges' world data (next_segment_spec §5.3)

std::vector<std::pair<int, double>> World::RoomLeaders(const std::string& Venue, int MonthKey, int Count) const
{
	std::vector<std::pair<int, double>> Out;
	if (!Created || Count <= 0)
	{
		return Out;
	}
	auto Consider = [&](int Id, const RoomRecord& R) {
		const double Points = R.Venue == Venue ? R.PointsIn(MonthKey) : 0.0;
		if (Points > 0.0)
		{
			Out.push_back({Id, Points});
		}
	};
	Consider(-1, HeroRoomRec);
	for (const Npc& N : People())
	{
		if (!N.Faded)
		{
			Consider(N.Id, N.Room);
		}
	}
	std::stable_sort(Out.begin(), Out.end(), [](const std::pair<int, double>& A, const std::pair<int, double>& B) {
		return A.second > B.second || (A.second == B.second && A.first < B.first);
	});
	if (static_cast<int>(Out.size()) > Count)
	{
		Out.resize(static_cast<size_t>(Count));
	}
	return Out;
}

int World::CircuitRank(int Npc) const
{
	const world::Npc* N = Npc >= 0 ? Get(Npc) : nullptr;
	if (Npc >= 0 && (!N || N->Faded))
	{
		return 0;
	}
	const double Mine = Npc < 0 ? HeroCircuit : N->CircuitPoints;
	if (!Created || Mine <= 0.0)
	{
		return 0;
	}
	// Ahead: more points, or as many and an earlier id (the player's -1 is ahead of everyone on a tie).
	int Rank = 1;
	Rank += Npc >= 0 && HeroCircuit >= Mine ? 1 : 0;
	for (const world::Npc& O : People())
	{
		if (O.Id != Npc && !O.Faded && O.CircuitPoints > 0.0 && (O.CircuitPoints > Mine || (O.CircuitPoints == Mine && O.Id < Npc)))
		{
			++Rank;
		}
	}
	return Rank;
}

double World::CardCutLine(int Tier) const
{
	if (!Created || Tier < CardRegular || Tier > CardBlack)
	{
		return 0.0;
	}
	const size_t Cut = static_cast<size_t>(badge::CardTop[Tier - 1]);
	std::vector<double> All;
	All.reserve(People().size() + 1);
	if (HeroCircuit > 0.0)
	{
		All.push_back(HeroCircuit);
	}
	for (const Npc& N : People())
	{
		if (!N.Faded && N.CircuitPoints > 0.0)
		{
			All.push_back(N.CircuitPoints);
		}
	}
	if (All.size() < Cut)
	{
		return 0.0;
	}
	std::nth_element(All.begin(), All.begin() + static_cast<std::ptrdiff_t>(Cut - 1), All.end(), [](double A, double B) { return A > B; });
	return All[Cut - 1];
}

int World::PlayerOfYearTitles(int Npc) const
{
	int Count = 0;
	for (const Honor& H : Titles)
	{
		Count += H.Npc == Npc && (H.Title == "Player of the Year" || H.Title == "Live Player of the Year") ? 1 : 0;
	}
	return Count;
}

int World::HallClass(int Npc) const
{
	for (const Honor& H : Titles)
	{
		if (H.Npc == Npc && H.Title == HallOfFameTitle)
		{
			return H.Year;
		}
	}
	return 0;
}

double World::HallScore(int Npc, int Year) const
{
	(void)Year; // the record as it stands on the day (the year names the class)
	int Bracelets = 0;
	int Rings = 0;
	int Majors = 0;
	double Career = 0.0;
	double Overall = 0.0;
	if (Npc < 0)
	{
		for (const Award& A : HeroTrophies)
		{
			(A.Ring ? Rings : Bracelets) += 1;
		}
		for (const Honor& H : Titles)
		{
			// Update 4 (SPEC v1.7 §2.6.1, §2.6.2; §6 PA-25): a festival's Player of the Series belt never counts as a major, nor (v1.6) a
			// title whose template isn't a major (PENNANT's Micro Main). Neither exists before G1.
			if (IsCrownHonor(H) || WorldHeroMinorTitle(H))
			{
				continue;
			}
			// The player's majors are the titles history keeps for them (Resolve's Npc -1 honors), not the year-end ones.
			Majors += H.Npc == -1 && H.Title != "Player of the Year" && H.Title != "Live Player of the Year" && H.Title != HallOfFameTitle ? 1 : 0;
		}
		Career = sim::Dollars(HeroBook.Prizes);
		// (The world keeps no reputation for the player: that part of the score is the record's alone.)
	}
	else
	{
		const world::Npc* N = Get(Npc);
		if (!N)
		{
			return 0.0;
		}
		Bracelets = N->Bracelets;
		Rings = N->Rings;
		Majors = N->Majors;
		Career = sim::Dollars(N->CareerWon());
		Overall = static_cast<double>(N->RepOf(Rep::Overall));
	}
	return 3.0 * Bracelets + Rings + 3.0 * Majors + 4.0 * PlayerOfYearTitles(Npc) + 2.0 * std::log10(std::max(1.0, Career) / 100000.0) + Overall / 20.0;
}

bool World::HallEligible(int Npc, int Year) const
{
	if (!Created)
	{
		return false;
	}
	const int Day = DayOn(Year, 7, 15);
	if (Npc < 0)
	{
		// The player: the real rule (§11 Q25), once the session has told the world their age.
		const bool HighStakes = HeroBook.ByStake[static_cast<size_t>(TrackStake::High)].Events > 0 || HeroCardTop >= CardRegular || HeroCardPrev >= CardRegular ||
			std::any_of(HeroTrophies.begin(), HeroTrophies.end(), [](const Award& A) { return !A.Online; });
		return HeroBorn > 0 && Year - HeroBorn >= badge::HallAge && Year - YearOf(sim::DayAt(Origin)) >= badge::HallYears && HighStakes && HallClass(-1) == 0;
	}
	const world::Npc* N = Get(Npc);
	if (!N || N->Faded)
	{
		return false;
	}
	const bool HighStakes = N->Live >= LiveLevel::Circuit || N->Tier >= static_cast<int>(net::Tier::High) ||
		N->Stats.ByStake[static_cast<size_t>(TrackStake::High)].Events > 0 || N->RepOf(Rep::HighStakes) >= 20.0f;
	// (The history books last: most of the world is ruled out by age, years or stakes first.)
	return N->Age(Day) >= badge::HallAge && Year - YearOf(N->Joined) >= badge::HallYears && HighStakes && HallClass(Npc) == 0;
}

void World::HeroMoney(int Npc, Chips Cents, MemoryKind Kind, const std::string& Where, double At, bool Early)
{
	HoldGuard("HeroMoney");
	const world::Npc* Seen = Created && Npc >= 0 && Npc < static_cast<int>(People().size()) ? &People()[static_cast<size_t>(Npc)] : nullptr;
	if (!Seen || Seen->Faded)
	{
		return;
	}
	// Their money (private): what they can't cover simply isn't there to lose.
	world::Npc& N = Edit(Npc);
	N.Bankroll = std::max<Chips>(0, N.Bankroll + Cents);
	N.PeakRoll = std::max(N.PeakRoll, N.Bankroll);
	++Rev;
	++HeroRev;
	if (Kind == MemoryKind::PaidLate && Cents != 0)
	{
		// The money for a debt that already went late: the late mark they remember (its own Cents 0, at the due time) is
		// the one memory and the one knock to their trust (§2.9); paying it now just settles it.
		const auto Known = HeroBonds.find(Npc);
		if (Known != HeroBonds.end())
		{
			for (auto It = Known->second.Memories.rbegin(); It != Known->second.Memories.rend(); ++It)
			{
				if (It->Kind == MemoryKind::PaidLate && It->Where == Where && It->Amount == 0)
				{
					It->Amount = Cents >= 0 ? Cents : -Cents;
					return;
				}
			}
		}
	}
	// "Paid up" is the player paying them; money their way to the player (a swap they lost) is the swap settling.
	const MemoryKind Kept = Kind == MemoryKind::PaidUp && Cents < 0 ? MemoryKind::Swapped : Kind;
	Remember(Npc, Kept, Where, Cents >= 0 ? Cents : -Cents, At);
	if (Kept == MemoryKind::PaidUp && Early)
	{
		// Paid within a day of the result: the kind of thing a backer remembers.
		Bond& B = HeroBonds[Npc];
		B.Trust = Add01(B.Trust, 0.02);
		B.Respect = Add01(B.Respect, 0.02);
	}
}

std::vector<int> World::Backers(int Max) const
{
	HoldGuard("Backers");
	std::vector<int> Out;
	const Chips Abi = HeroBook.PaidAbi();
	if (!Created || Max <= 0 || Abi <= 0)
	{
		return Out;
	}
	std::vector<std::pair<double, int>> Pool;
	for (const auto& It : HeroBonds)
	{
		const world::Npc* N = Get(It.first);
		const std::string Label = It.second.Label();
		if (!N || N->Faded || !N->Playing() || N->Backer != -1 || (Label != "Respects you" && Label != "Trusts you") || N->Bankroll < Abi * 400)
		{
			continue;
		}
		const std::ptrdiff_t Horses = std::count_if(N->Ties.begin(), N->Ties.end(), [](const Tie& T) { return T.Kind == TieKind::Backer; });
		if (Horses >= 3)
		{
			continue;
		}
		Pool.push_back({static_cast<double>(It.second.Trust) + static_cast<double>(It.second.Respect), It.first});
	}
	std::stable_sort(Pool.begin(), Pool.end(), [](const std::pair<double, int>& A, const std::pair<double, int>& B) { return A.first > B.first || (A.first == B.first && A.second < B.second); });
	for (size_t K = 0; K < Pool.size() && static_cast<int>(K) < Max; ++K)
	{
		Out.push_back(Pool[K].second);
	}
	return Out;
}

std::vector<int> World::EmbercrestRegulars() const
{
	HoldGuard("EmbercrestRegulars");
	// Update 4 (SPEC v1.7 §3.18 R1, §6 PA-52): the pinned list once G2 sets it when the book opens (scene::PinRegulars). Never set in
	// Phase A (the pin is disabled), so today's rule below answers.
	if (!Regulars.empty())
	{
		return Regulars;
	}
	// Update 4 (SPEC v1.7 §6 PA-33): today's rule is a sort of the roster, so its answer is kept per planned day and revision: the
	// world's people change only inside a turn (PlanDay plans tomorrow under a new Planned, and the turn's end moves Rev) or through a
	// call that moves Rev (HeroMoney, GrantAward), and a load or a migration moves the version or the roster's size. Inside the turn
	// it's always worked out fresh (the roster is mid-change), and not kept.
	const std::array<int64_t, 5> Key{{static_cast<int64_t>(Rev), static_cast<int64_t>(Planned), static_cast<int64_t>(People().size()), static_cast<int64_t>(LoadedVersion),
		static_cast<int64_t>(WorldSeed)}};
	if (!HeldTurn.Running && Key == RegularsKey)
	{
		return RegularsMemo;
	}
	// Not who's playing this week (a break or a broke spell doesn't change who the room's regulars are). The room's
	// own locals, then the town's live players, then its online players who've had the money for a live game (their
	// best bankroll only grows, so they stay).
	std::vector<std::pair<uint64_t, int>> Pool;
	for (const Npc& N : People())
	{
		if (!N.Faded && N.St != Status::Retired && N.From != Origin::Cast && !N.Rival && N.Home == Region::Americas && N.Country == "US" &&
			(N.Live >= LiveLevel::Local || N.PeakRoll >= 100000))
		{
			const uint64_t Tier = N.From == Origin::Local ? 0u : N.Live >= LiveLevel::Local ? 1u : 2u;
			Pool.push_back({(Tier << 32) | live::FaceHash(N.Name), N.Id});
		}
	}
	std::sort(Pool.begin(), Pool.end());
	std::vector<int> Out;
	for (size_t I = 0; I < Pool.size() && I < static_cast<size_t>(EmbercrestRegularCount); ++I)
	{
		Out.push_back(Pool[I].second);
	}
	if (!HeldTurn.Running)
	{
		RegularsMemo = Out;
		RegularsKey = Key;
	}
	return Out;
}

std::string World::SpokenGreeting(int Npc, uint32_t Salt) const
{
	HoldGuard("SpokenGreeting");
	const Bond* B = BondWith(Npc);
	if (!B || B->Memories.empty())
	{
		return "";
	}
	const Memory& Last = B->Memories.back();
	const int Pick = static_cast<int>(Salt % 3u);
	auto One = [&](const char* A, const char* Bb, const char* C) { return std::string(Pick == 0 ? A : Pick == 1 ? Bb : C); };
	// Update 4 (SPEC v1.7 §3.16 R1, §6 PA-24): PASSED YOU within 30 days speaks first (G2; "" while Bond::PassedDay is 0).
	const std::string Passed = story::PassedGreeting(*this, Npc, Salt, true);
	if (!Passed.empty())
	{
		return Passed;
	}
	switch (Last.Kind)
	{
	case MemoryKind::KnockedOutHero: return One("You again. It went better for me last time.", "Rematch, huh?", "Remember that hand? I do.");
	case MemoryKind::HeroKnockedOut:
		return B->Resentment >= 0.3f ? One("Still thinking about that river.", "You owe me one.", "Not you again.")
									 : One("Good luck. You got me last time.", "Careful with this one, everybody.", "Nice hand last time, honestly.");
	case MemoryKind::HeadsUpWon: return One("Heads-up rematch, whenever you want.", "Look who it is. Good luck.", "I still think about that final.");
	case MemoryKind::HeadsUpLost: return One("Back for more?", "Good game last time.", "Good luck. See you heads-up again.");
	case MemoryKind::FinalTable: return One("Final table buddy. Good luck.", "See you at another final.", "This one again. Good luck.");
	case MemoryKind::BackRoom: return One("How's Dee?", "The laundromat crew, out in daylight.", "I hear you play better at Dee's.");
	case MemoryKind::BigPotWon: return One("I want my chips back.", "Not you again.", "That pot still hurts.");
	case MemoryKind::BigPotLost: return One("Thanks for the chips last time.", "Good to see you. Bring chips?", "Evening. Feeling lucky?");
	case MemoryKind::Street:
		// Where they last talked: Ruby's, the Wash & Fold's door, the 14's stop, Fifth St.
		return Last.Where == "Ruby's" ? One("Saw you at Ruby's.", "Hey, it's the neighbor.", "The booth by the window, right? I know you from Ruby's.")
									  : One("Saw you on Fifth.", "Hey, it's the neighbor.", "Fifth Street, right? I know you from the block.");
	// Money between them stays between them (N3): out loud it's only ever warm, cool or cold.
	case MemoryKind::Staked:
	case MemoryKind::BoughtPiece:
	case MemoryKind::Swapped:
	case MemoryKind::PaidUp: return One("There you are. Make it a good one.", "Good to see you. Play well tonight.", "Evening. Go get 'em.");
	case MemoryKind::PaidLate: return One("Evening.", "Oh. You. Good luck.", "Hey. Play well.");
	case MemoryKind::Welched:
	case MemoryKind::WalkedOut: return One("Hm. You.", "Let's just play cards.", "Funny seeing you here.");
	case MemoryKind::Watched: break; // Update 4 (§3.17 R3, §6 PA-24): the ForYou greeting (W-table, G2); no such memory before then
	default: break;
	}
	return B->Encounters >= 2 ? One("Hey again.", "Good to see you.", "Evening.") : One("I know you. Good luck.", "Hey, I remember you.", "Good luck again.");
}

std::string World::Greeting(int Npc, uint32_t Salt) const
{
	HoldGuard("Greeting");
	const Bond* B = BondWith(Npc);
	const world::Npc* N = Get(Npc);
	if (!B || !N || B->Memories.empty())
	{
		return "";
	}
	const Memory& Last = B->Memories.back();
	const int Pick = static_cast<int>(Salt % 3u);
	auto One = [&](const char* A, const char* Bb, const char* C) { return std::string(Pick == 0 ? A : Pick == 1 ? Bb : C); };
	// Update 4 (SPEC v1.7 §3.16 R1, §6 PA-24): PASSED YOU within 30 days speaks first (G2; "" while Bond::PassedDay is 0).
	const std::string Passed = story::PassedGreeting(*this, Npc, Salt, false);
	if (!Passed.empty())
	{
		return Passed;
	}
	switch (Last.Kind)
	{
	case MemoryKind::KnockedOutHero: return One("you again. went better for me last time", "rematch?", "remember that hand? I do");
	case MemoryKind::HeroKnockedOut:
		return B->Resentment >= 0.3f ? One("still thinking about that river", "you owe me one", "not you again") : One("gl, you got me last time", "careful with this one, everyone", "nh last time, honestly");
	case MemoryKind::HeadsUpWon: return One("heads-up rematch whenever you want", "the one who beat me heads-up. gl", "I've watched that final back twice");
	case MemoryKind::HeadsUpLost: return One("gl, see you heads-up again", "back for more?", "gg last time");
	case MemoryKind::FinalTable: return One("ft buddy, gl", "see you at another final", "this one again. gl");
	case MemoryKind::BackRoom: return One("Dee says hi", "the laundromat crew online", "you play better at Dee's");
	case MemoryKind::Embercrest: return One("see you Sunday?", "Embercrest regular in the house", "gl, Sunday was fun");
	case MemoryKind::BigPotWon: return One("I want my chips back", "gl. not you again", "that pot still hurts");
	case MemoryKind::Street: return One("saw you on fifth the other day", "hey neighbor, gl", "fifth st in the building");
	case MemoryKind::Staked:
	case MemoryKind::BoughtPiece:
	case MemoryKind::Swapped:
	case MemoryKind::PaidUp: return One("there you are. gl", "gl tonight, make it count", "good to see you. gl");
	case MemoryKind::PaidLate: return One("oh. you. gl", "hm. gl", "evening");
	case MemoryKind::Welched:
	case MemoryKind::WalkedOut: return One("not you", "just play cards", "hm");
	case MemoryKind::Watched: break; // Update 4 (§3.17 R3, §6 PA-24): the ForYou greeting (W-table, G2); no such memory before then
	default: break;
	}
	return B->Encounters >= 3 ? One("gl", "hey again", "gl all") : "";
}

// ------------------------------------------------------------------ Update 4 (SPEC v1.7; Phase A's stubs)

void World::KnownFinalTable(const std::string& /*EventId*/, const std::vector<KnownSeat>& /*Seats*/)
{
	HoldGuard("KnownFinalTable");
	// §4.10.2, PA-32: no field plays on before G3 (B-session: the seats into Pending::KnownFt, which Resolve then reads).
}

void World::HeroBullet(const std::string& /*EventId*/)
{
	HoldGuard("HeroBullet");
	// §4.13.1 rule 5, PA-32: no re-entry before G3 (B-session: one entry and its share of the pool on the pending event).
}

int World::HeroBullets(const std::string& /*EventId*/) const
{
	HoldGuard("HeroBullets");
	return 0;
}
} // namespace world
} // namespace ss
