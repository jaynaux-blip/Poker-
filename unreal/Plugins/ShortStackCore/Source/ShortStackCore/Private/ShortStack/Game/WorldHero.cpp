// The living world and the player: its clock, its events and boards as the screens ask for them, and what
// the people in it remember about the player.
#include "ShortStack/Game/World.h"

#include "ShortStack/Game/Life.h"
#include "WorldSim.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

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

/** The day an event id starts on ("riverside@<day>", "dees@<day>", otherwise "<id>@<minute>"). */
int EventDay(const std::string& Id)
{
	const size_t At = Id.rfind('@');
	if (At == std::string::npos)
	{
		return -1;
	}
	const double V = std::atof(Id.c_str() + At + 1);
	const bool ByDay = Id.rfind("riverside@", 0) == 0 || Id.rfind("dees@", 0) == 0;
	return ByDay ? static_cast<int>(V) : sim::DayAt(V);
}

float Add01(float V, double D)
{
	return sim::Clampf(static_cast<double>(V) + D, 0.0, 1.0);
}
} // namespace worldhero_detail

using namespace worldhero_detail;

// ------------------------------------------------------------------ time

void World::AdvanceTo(double To)
{
	if (!Created || To <= Now)
	{
		return;
	}
	while (Now < To)
	{
		const int Day = sim::DayAt(Now);
		const double DayEnd = static_cast<double>(Day + 1) * 1440.0;
		const double NextHour = (std::floor(Now / 60.0) + 1.0) * 60.0;
		const double Limit = std::min({To, DayEnd, NextHour});
		// Tomorrow's registrations are open.
		Sim::PlanDay(*this, Day + 1);
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
			Sim::Resolve(*this, P);
		}
		Queue.erase(Queue.begin(), Queue.begin() + static_cast<std::ptrdiff_t>(Done));
		if (!Later.empty())
		{
			for (Pending& P : Later)
			{
				Queue.push_back(std::move(P));
			}
			std::stable_sort(Queue.begin(), Queue.end(), [](const Pending& A, const Pending& B) { return A.End < B.End; });
		}
		Now = Limit;
		if (Now >= NextHour)
		{
			NightPointsHourAgo = NightPoints;
			NightHour = Now;
		}
		if (Now >= DayEnd)
		{
			Sim::NewDay(*this, Day + 1);
		}
	}
	++Rev;
}

void World::Simulate(int DayCount)
{
	if (DayCount > 0)
	{
		AdvanceTo(Now + static_cast<double>(DayCount) * 1440.0);
	}
}

// ------------------------------------------------------------------ events and boards

const net::EventResult* World::ResultOf(const std::string& EventId) const
{
	const auto It = Results.find(EventId);
	return It == Results.end() ? nullptr : &It->second;
}

void World::EnsurePlanned(int Day)
{
	// Tomorrow at most: registrations further out don't exist yet (and looking doesn't change the world).
	if (Created && Day > Planned && Day <= sim::DayAt(Now) + 1)
	{
		Sim::PlanDay(*this, Day);
	}
}

std::vector<int> World::Registered(const std::string& EventId)
{
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
		int Rank = 1;
		for (const auto& It : NightPointsHourAgo)
		{
			Rank += It.second > V ? 1 : 0;
		}
		return V > 0.0 ? Rank : static_cast<int>(Roster.size());
	}
	const std::vector<int>& Rk = RanksYesterday[static_cast<size_t>(B)];
	return Npc >= 0 && static_cast<size_t>(Npc) < Rk.size() ? Rk[static_cast<size_t>(Npc)] : static_cast<int>(Roster.size());
}

std::vector<int> World::Leaders(Rep R, int Count) const
{
	std::vector<int> Order;
	for (const Npc& N : Roster)
	{
		if (N.St != Status::Retired)
		{
			Order.push_back(N.Id);
		}
	}
	std::stable_sort(Order.begin(), Order.end(), [&](int A, int B) { return Roster[static_cast<size_t>(A)].RepOf(R) > Roster[static_cast<size_t>(B)].RepOf(R); });
	if (static_cast<int>(Order.size()) > Count)
	{
		Order.resize(static_cast<size_t>(std::max(0, Count)));
	}
	return Order;
}

// ------------------------------------------------------------------ the player

void World::HeroEntered(const std::string& EventId, const std::string& EventName)
{
	if (!Created)
	{
		return;
	}
	Registered(EventId); // plans its day if it hasn't been
	Pending* P = FindPending(Queue, EventId);
	if (!P)
	{
		// Nobody the world follows is in it: the player's result still makes it a world event.
		net::EventInstance E;
		if (net::Shared().FindInstance(EventId, E))
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

void World::Join(const std::string& EventId, int Npc)
{
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

void World::HeroFinished(const std::string& EventId, int Place, Chips Prize, const TableReport& Report)
{
	if (!Created)
	{
		return;
	}
	HeroIn.erase(EventId);
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
		(Online ? HeroPoints : HeroLivePoints) += Pts;
	}
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

void World::RiversideDone(double At, const std::vector<std::pair<std::string, int>>& Places, int HeroPlace, int FieldSize)
{
	const int Day = sim::DayAt(At - 6.0 * 60.0); // it starts at 7 PM and can run past midnight
	const std::string Id = "riverside@" + std::to_string(Day);
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
		Remember(Npc, MemoryKind::Riverside, "the Riverside Sunday", 0, At);
		if (HeroPlace >= 1 && HeroPlace <= 6 && It.second <= 6)
		{
			Remember(Npc, MemoryKind::FinalTable, "the Riverside Sunday", 0, At);
		}
	}
	HeroResults[Id] = {HeroPlace, 0};
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
	case MemoryKind::Riverside: B.Familiarity = Add01(B.Familiarity, 0.06); break;
	case MemoryKind::ShowedBluff: B.Rivalry = Add01(B.Rivalry, 0.05); break;
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
	const world::Npc* N = Get(Npc);
	if (!N)
	{
		return;
	}
	Bond& B = HeroBonds[Npc];
	const int Day = sim::DayAt(At);
	const bool First = B.Memories.empty();
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

const Bond* World::BondWith(int Npc) const
{
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

std::string World::Greeting(int Npc, uint32_t Salt) const
{
	const Bond* B = BondWith(Npc);
	const world::Npc* N = Get(Npc);
	if (!B || !N || B->Memories.empty())
	{
		return "";
	}
	const Memory& Last = B->Memories.back();
	const int Pick = static_cast<int>(Salt % 3u);
	auto One = [&](const char* A, const char* Bb, const char* C) { return std::string(Pick == 0 ? A : Pick == 1 ? Bb : C); };
	switch (Last.Kind)
	{
	case MemoryKind::KnockedOutHero: return One("you again. went better for me last time", "rematch?", "remember that hand? I do");
	case MemoryKind::HeroKnockedOut:
		return B->Resentment >= 0.3f ? One("still thinking about that river", "you owe me one", "not you again") : One("gl, you got me last time", "careful with this one, everyone", "nh last time, honestly");
	case MemoryKind::HeadsUpWon: return One("heads-up rematch whenever you want", "the one who beat me heads-up. gl", "I've watched that final back twice");
	case MemoryKind::HeadsUpLost: return One("gl, see you heads-up again", "back for more?", "gg last time");
	case MemoryKind::FinalTable: return One("ft buddy, gl", "see you at another final", "this one again. gl");
	case MemoryKind::BackRoom: return One("Dee says hi", "the laundromat crew online", "you play better at Dee's");
	case MemoryKind::Riverside: return One("see you Sunday?", "Riverside regular in the house", "gl, Sunday was fun");
	case MemoryKind::BigPotWon: return One("I want my chips back", "gl. not you again", "that pot still hurts");
	default: break;
	}
	return B->Encounters >= 3 ? One("gl", "hey again", "gl all") : "";
}
} // namespace world
} // namespace ss
