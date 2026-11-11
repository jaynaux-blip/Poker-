// Update 4, Growth (SPEC v1.7 §3.8-§3.12): phases, causes, caps, dwell, the potential factor, the drift guard, per-tier softness and
// turning pro on results (H/WorldGrowth.h). W-growth's file from G2 (S-scale adds the crowd's Growth and FieldTiers' crowd weights at
// G4).
//
// Phase A (§6 PA-19, PA-23; the lead): every hook is a no-op or returns today's value, so no draw and no number moves: Rate returns
// today's rate, VolumeScale and BuyInScale 1.0. Growth::Default is the record's own test (nothing writes a record before G2).
#include "ShortStack/Game/WorldGrowth.h"

#include "ShortStack/Game/World.h"

namespace ss
{
namespace world
{
bool Growth::Default() const
{
	return BirthPeak == 0.0f && BirthPot == 0.0f && Ph == GrowthPhase::Steady && Why == Cause::None && Until == 0 && Coach == -1 && CoachUntil == 0 &&
		BusyUntil == 0 && GrindCool == 0 && TiltCool == 0 && GrindRun == 0 && TiltRun == 0 && Band == -1 && BandSince == 0 && Flags == 0 && SeriousAt == 0 &&
		SeriousTop == 0.0f;
}

namespace grow
{
/** World's friend for the Growth module (§6.2): the field tiers and the anchor's references World keeps (W-growth's from G2). */
struct Access
{
	static std::array<double, 5>& Field(World& W) { return W.Field; }
	static const SceneBook& Book(const World& W) { return W.Scene; }
};

void Week(World& /*W*/, Npc& /*N*/, int /*Day*/)
{
	// Phase A: no phase, cause or dwell moves.
}

double Rate(const World& /*W*/, const Npc& /*N*/, int /*Day*/, double TodayRate, double /*Study*/)
{
	// Phase A: today's rate, exactly (G2: the phase's closing, §3.8 R2).
	return TodayRate;
}

void Clamp(World& /*W*/, Npc& /*N*/, int /*Day*/)
{
	// Phase A: no weekly cap (G2: kCap1-3 x PaceX).
}

double VolumeScale(const Npc& /*N*/, int /*Day*/)
{
	return 1.0;
}

double BuyInScale(const Npc& /*N*/, int /*Day*/)
{
	return 1.0;
}

void FieldTiers(World& /*W*/)
{
	// Phase A: today's field strength by tier stands (G2: per-tier softness and the drift guard, §3.12).
}

void OnResult(World& /*W*/, Npc& /*N*/, const Pending& /*P*/, int /*Place*/, Chips /*Prize*/)
{
}

void OnBreakout(World& /*W*/, Npc& /*N*/, int /*Day*/)
{
}

void OnMovedUp(World& /*W*/, Npc& /*N*/, int /*Day*/)
{
}

void OnStaked(World& /*W*/, Npc& /*N*/, int /*Day*/)
{
}

void TurnProOnResults(World& /*W*/, Npc& /*N*/, int /*Day*/)
{
	// Phase A: today's TurnPro alone turns people pro (G2: the results path, §3.10).
}
} // namespace grow
} // namespace world
} // namespace ss
