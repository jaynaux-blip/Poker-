// Update 4, the TPI Rankings (SPEC v1.7 §3.11, §3.13): scoring, the weekly re-rank, the monthly rankings, RISING and Rising
// prospect, Debut, the cash rule and the world's mystery money in the money. W-story's file from G2 (S-promote's one sort over every
// tier and the ranks written back to the crowd at G4).
//
// Phase A (§6 PA-19, PA-23, PA-31; the lead): nobody is ranked; every hook is a no-op or returns today's value: Labels today's label,
// MysteryInMoney false (today's every-place envelopes), TpiCash today's Prize > 0 || Seat, VoiceStory 0 and false.
#include "ShortStack/Game/WorldStory.h"

#include "ShortStack/Game/World.h"

#include <string>

namespace ss
{
namespace world
{
namespace story
{
void OnResult(World& /*W*/, Npc& /*N*/, const Pending& /*P*/, int /*Place*/, Chips /*Prize*/, bool /*Seat*/)
{
	// Phase A: no TPI week to fold, no beat.
}

void OnHeroResult(World& /*W*/, const std::string& /*EventId*/, int /*Place*/, Chips /*Prize*/)
{
	// Phase A: the hero is unranked (G2: HeroTpiCash, the hero's meetings).
}

Identity Labels(const World& /*W*/, const Npc& /*N*/, int /*Today*/, Identity Hidden)
{
	// Phase A: today's label (G2: YoungCrusher and Rising prospect from public results, §3.11).
	return Hidden;
}

bool MysteryInMoney(const Pending& /*P*/)
{
	return false;
}

bool TpiCash(const Pending& /*P*/, int /*Place*/, Chips Prize, bool Seat)
{
	// Phase A: today's cash test, exactly (G2: a paid place, Place <= PaidPlaces(Entries, Curve), or a satellite seat).
	return Prize > 0 || Seat;
}

void VoiceStory(const World& /*W*/, int /*Npc*/, int& TpiRank, bool& Rising)
{
	// Phase A: unranked, not rising (LiveVoice's defaults).
	TpiRank = 0;
	Rising = false;
}
} // namespace story

// Update 4 (LEAD_CALLS Q-5): the TPI's World face, declared before the freeze. Phase A: nobody is ranked.
const TpiRec* World::TpiOf(int /*Npc*/) const
{
	return nullptr;
}

void World::HeroTpiCash(const std::string& /*EventId*/, int /*Place*/, Chips /*Prize*/)
{
	// Phase A: the hero is unranked (G2: the hero's cash folded once per event id, inside HeroFinished).
}
} // namespace world
} // namespace ss
