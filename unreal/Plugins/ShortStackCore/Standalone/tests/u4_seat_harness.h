// Update 4 test helper (SPEC v1.7 §3.17, §3.24 fidelity_exact, §6.5 S-fidelity; §6 PA-49): the story-surfaces probe's seat harness
// (durable/update4/probes/story-surfaces/probe_main.cpp, 688 events over a year at 12 a sampled day), as a header the world_u4 suite
// and the S-fidelity spike include. It replays Session::NameField's matching (G/Session.cpp:5906-6053 at 8db6d21) on a real
// Tournament draw and counts how often a named world person's seat archetype is their own (live::PlayStyle), at the hero's table and
// in the field. Read-only on the world (it registers nobody and seats nobody in the session).
//
// Phase A (the lead): declared and compiled (world_u4_main.cpp includes it); nothing calls it yet. G2's W-table runs it in
// fidelity_exact (target >= 95% exact, today 52.8-53.3% at the hero's table) and the lead in S-fidelity (100 restores).
#pragma once

#include "ShortStack/AI/Profiles.h"
#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Lobby.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Rng.h"
#include "ShortStack/Tournament.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace u4seat
{
/** The probe's tallies: every named seat, and the hero's table on its own. */
struct SeatStats
{
	int Events = 0, Named = 0, Exact = 0, SameClass = 0, Cross = 0, StrongIntoWeak = 0, WeakIntoStrong = 0;
	int HeroNamed = 0, HeroExact = 0, HeroCross = 0, HeroStrongIntoWeak = 0;
	int HeroNew = 0, HeroProspect = 0, HeroPotential = 0, HeroElite = 0;
	int Fallback = 0; // events nobody registered for (the weak-regular fallback)
	std::map<int, int> WantByType, GotByType;
	std::map<int, int> MetDistinct;
	double ExactShare() const { return Named > 0 ? static_cast<double>(Exact) / static_cast<double>(Named) : 0.0; }
	double HeroExactShare() const { return HeroNamed > 0 ? static_cast<double>(HeroExact) / static_cast<double>(HeroNamed) : 0.0; }
};

/** A strong archetype (the probe's classes): Reg, Crusher, Tag, Lag. */
inline bool SeatStrong(int A)
{
	return A == static_cast<int>(ss::Archetype::Reg) || A == static_cast<int>(ss::Archetype::Crusher) || A == static_cast<int>(ss::Archetype::Tag) ||
		A == static_cast<int>(ss::Archetype::Lag);
}

/** One event: Session::NameField's matching, RiverLine only, on a real Tournament (the probe's Seat, verbatim but for names). */
inline void SeatEvent(ss::world::World& W, const ss::net::EventInstance& E, const ss::TournamentSpec& Spec, SeatStats& S, int Day)
{
	const std::string Seed = E.Id + ":probe:0";
	std::vector<ss::ReservedPlayer> Reserved{{ss::RivalName, ss::Archetype::Crusher}};
	ss::Tournament T(Spec, "Hero", Seed, Reserved);
	ss::Rng Nr(T.Spec.Id + ":probe:field");
	std::vector<int> Who = W.Registered(T.Spec.Id);
	const int Rival = W.Find(ss::RivalName);
	Who.erase(std::remove(Who.begin(), Who.end(), Rival), Who.end());
	if (Who.empty())
	{
		++S.Fallback;
		const std::string& Pop = T.Spec.Population;
		const int Tier = Pop == "low" ? 2 : Pop == "high" ? 3 : 1;
		std::vector<int> Pool;
		for (const ss::world::Npc& N : W.People())
		{
			if (N.Playing() && N.Tier == Tier && !N.Rival && N.Overall() < 0.5f && (N.Sites & 1))
			{
				Pool.push_back(N.Id);
			}
		}
		Nr.Shuffle(Pool);
		const int Count = std::min(static_cast<int>(Pool.size()), std::min(40, std::max(4, T.Spec.Entrants * 3 / 100)));
		for (int K = 0; K < Count; ++K)
		{
			Who.push_back(Pool[static_cast<size_t>(K)]);
		}
	}
	Nr.Shuffle(Who);
	{
		int Fresh = 0;
		for (size_t K = 0; K < Who.size() && Fresh < 2; ++K)
		{
			const ss::world::Npc* N = W.Get(Who[K]);
			if (N && N->Came != ss::world::Arrival::None && Day - N->Arrived <= 60 && !W.BondWith(N->Id))
			{
				std::swap(Who[static_cast<size_t>(Fresh)], Who[K]);
				++Fresh;
			}
		}
	}
	std::map<int, std::vector<size_t>> SeatsBy, HeroTable;
	const int HeroTableId = T.Hero().TableId;
	for (size_t I = 0; I < T.Players.size(); ++I)
	{
		const ss::TPlayer& P = T.Players[I];
		if (!P.IsHero && P.Name != ss::RivalName)
		{
			(P.TableId == HeroTableId ? HeroTable : SeatsBy)[static_cast<int>(P.Prof.Type)].push_back(I);
		}
	}
	for (auto& It : SeatsBy)
	{
		Nr.Shuffle(It.second);
	}
	for (auto& It : HeroTable)
	{
		Nr.Shuffle(It.second);
	}
	int AtHeroTable = std::min(5, static_cast<int>(Who.size()) / 3 + 1);
	auto TakeFrom = [&](std::map<int, std::vector<size_t>>& From, int Wanted) -> size_t {
		auto Exact = From.find(Wanted);
		if (Exact != From.end() && !Exact->second.empty())
		{
			const size_t X = Exact->second.back();
			Exact->second.pop_back();
			return X;
		}
		for (int Pass = 0; Pass < 2; ++Pass)
		{
			for (auto& It : From)
			{
				if (!It.second.empty() && (Pass == 1 || SeatStrong(It.first) == SeatStrong(Wanted)))
				{
					const size_t X = It.second.back();
					It.second.pop_back();
					return X;
				}
			}
		}
		return static_cast<size_t>(-1);
	};
	auto Take = [&](int Wanted) -> size_t {
		if (AtHeroTable > 0)
		{
			--AtHeroTable;
			const size_t X = TakeFrom(HeroTable, Wanted);
			if (X != static_cast<size_t>(-1))
			{
				return X;
			}
		}
		return TakeFrom(SeatsBy, Wanted);
	};
	++S.Events;
	for (int Id : Who)
	{
		const ss::world::Npc* N = W.Get(Id);
		if (!N)
		{
			continue;
		}
		const int Want = static_cast<int>(ss::live::PlayStyle(*N));
		const size_t X = Take(Want);
		if (X == static_cast<size_t>(-1))
		{
			break;
		}
		const ss::TPlayer& P = T.Players[X];
		const int Got = static_cast<int>(P.Prof.Type);
		const bool AtHero = P.TableId == HeroTableId;
		++S.Named;
		S.WantByType[Want]++;
		S.GotByType[Got]++;
		if (Got == Want)
		{
			++S.Exact;
		}
		else if (SeatStrong(Got) == SeatStrong(Want))
		{
			++S.SameClass;
		}
		else
		{
			++S.Cross;
			(SeatStrong(Want) ? S.StrongIntoWeak : S.WeakIntoStrong)++;
		}
		if (AtHero)
		{
			++S.HeroNamed;
			S.MetDistinct[Id]++;
			if (Got == Want)
			{
				++S.HeroExact;
			}
			else if (SeatStrong(Got) != SeatStrong(Want))
			{
				++S.HeroCross;
				S.HeroStrongIntoWeak += SeatStrong(Want) ? 1 : 0;
			}
			S.HeroNew += N->Came != ss::world::Arrival::None && Day - N->Arrived <= 60 ? 1 : 0;
			S.HeroProspect += N->Is == ss::world::Identity::RisingProspect ? 1 : 0;
			S.HeroPotential += N->Potential > 0.75f ? 1 : 0;
			S.HeroElite += N->Overall() >= 0.72f ? 1 : 0;
		}
	}
}

/**
 * The probe's year: from Night One, one sampled day a week, PerDay events of the day (18-4,000 entries, joinable) seated through
 * SeatEvent. The probe's defaults (365 days, 12 a day) give its 688 events. W must be created and attached (net::AttachAll) by the
 * caller; the world is advanced to the end of each day it walks.
 */
inline void SeatYear(ss::world::World& W, int Days, int PerDay, SeatStats& S)
{
	const int FirstDay = ss::net::NightOneDay;
	for (int Day = FirstDay; Day < FirstDay + Days; ++Day)
	{
		const double Midnight = static_cast<double>(Day) * 1440.0;
		W.AdvanceTo(std::max(Midnight, W.Clock()));
		W.EnsurePlanned(Day);
		if ((Day - FirstDay) % 7 == 0 && PerDay > 0)
		{
			const std::vector<ss::net::EventInstance> Today = ss::net::Shared().Window(std::max(Midnight, W.Clock()), Midnight + 1440.0);
			std::vector<ss::net::EventInstance> Pick;
			for (const ss::net::EventInstance& E : Today)
			{
				if (E.Start >= std::max(Midnight, W.Clock()) && E.Entries >= 18 && E.Entries <= 4000 && ss::net::Shared().Joinable(E, nullptr, 0xffff))
				{
					Pick.push_back(E);
				}
			}
			const size_t Step = std::max<size_t>(1, Pick.size() / static_cast<size_t>(PerDay));
			for (size_t I = 0; I < Pick.size(); I += Step)
			{
				const ss::LobbyEvent L = ss::net::Shared().Listing(Pick[I], nullptr, 0xffff);
				SeatEvent(W, Pick[I], L.Spec, S, Day);
			}
		}
		W.AdvanceTo(Midnight + 1439.0);
	}
}
} // namespace u4seat
