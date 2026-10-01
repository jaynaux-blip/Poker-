#include "ShortStack/Game/Live.h"

#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Network.h"

namespace ss
{
namespace live
{
const std::vector<CastMember>& RiversideCast()
{
	// The feature table opens with the first five; the rest come over as seats open.
	static const std::vector<CastMember> Cast = {
		{"npc:Sal", "Sal", Archetype::Nit},
		{"npc:Mrs. Park", "Mrs. Park", Archetype::Nit},
		{"npc:Rick", "Rick", Archetype::Lag},
		{"npc:Mei", "Mei", Archetype::Reg},
		{"npc:Dre", "Dre", Archetype::Station},
		{"npc:Big Lou", "Big Lou", Archetype::Station},
		{"npc:Twitch", "Twitch", Archetype::Maniac},
		{"npc:gh0stfold", RivalName, Archetype::Crusher},
	};
	return Cast;
}

const std::vector<Level>& DeepstackLevels()
{
	static const std::vector<Level> Levels = [] {
		const Chips Blinds[][2] = {{100, 200}, {200, 300}, {200, 400}, {300, 500}, {300, 600}, {400, 800}, {500, 1000}, {600, 1200}, {800, 1600},
			{1000, 2000}, {1200, 2400}, {1500, 3000}, {2000, 4000}, {2500, 5000}, {3000, 6000}, {4000, 8000}, {5000, 10000}, {6000, 12000},
			{8000, 16000}, {10000, 20000}, {15000, 30000}, {20000, 40000}, {25000, 50000}, {30000, 60000}, {40000, 80000}, {50000, 100000}};
		std::vector<Level> Out;
		for (size_t I = 0; I < sizeof(Blinds) / sizeof(Blinds[0]); ++I)
		{
			Level L;
			L.Sb = Blinds[I][0];
			L.Bb = Blinds[I][1];
			L.Ante = I >= 2 ? Blinds[I][1] : 0;
			Out.push_back(L);
		}
		return Out;
	}();
	return Levels;
}

TournamentSpec RiversideSpec(int Day)
{
	TournamentSpec S;
	S.Id = "riverside@" + std::to_string(Day);
	S.Name = "Riverside Sunday $150";
	S.BuyInCents = RiversideBuyInCents;
	S.FeeCents = RiversideFeeCents;
	// A local weekly: forty-something to sixty runners, more as word gets around.
	S.Entrants = 42 + ((Day * 37 + 11) % 19);
	S.StartingStack = 20000;
	// Live pace: about two and a half minutes a hand, eight hands a level.
	S.LevelMinutes = 20.0;
	S.SecondsPerHand = 150.0;
	S.Population = "low";
	S.Speed = "Deepstack";
	S.StartClock = 19.0 * 60.0;
	S.TableSize = RiversideTableSize;
	S.Levels = DeepstackLevels();
	return S;
}

std::vector<ReservedPlayer> RiversideReserved()
{
	std::vector<ReservedPlayer> Out;
	for (const CastMember& C : RiversideCast())
	{
		ReservedPlayer P;
		P.Name = C.Name;
		P.Type = C.Type;
		Out.push_back(P);
	}
	return Out;
}

std::unique_ptr<Tournament> MakeRiverside(int Day, const std::string& HeroName, const std::string& Seed)
{
	std::unique_ptr<Tournament> T(new Tournament(RiversideSpec(Day), HeroName, Seed, RiversideReserved()));
	for (const CastMember& C : RiversideCast())
	{
		T->FeatureIds.push_back(C.Id);
	}
	return T;
}
} // namespace live
} // namespace ss
