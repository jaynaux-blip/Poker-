// Dynamic resolution that works everywhere the game runs (Unreal's own only runs in a packaged game, not in the editor).
//
// Each frame the GPU's real time is read back; the render scale (r.ScreenPercentage, temporal super resolution
// rebuilding the full output from it) is moved to hold a frame-time budget: down quickly when a scene is heavier than
// the budget, back up slowly when there's room, never above the player's Resolution Scale (which can go past 100% to
// supersample when the GPU has power to spare) and never below a floor.
#pragma once

#include "CoreMinimal.h"

class FFrameBudget
{
public:
	/** TargetFps 0: a fixed render scale (MaxPercent). Otherwise the scale moves between MinPercent and MaxPercent. */
	void Configure(int32 TargetFps, int32 MaxPercent);
	/** Every frame, from the game mode. */
	void Tick(float DeltaSeconds);
	/** The render scale now (percent). */
	float Percent() const { return Current; }

	static constexpr float MinPercent = 60.0f;

private:
	void Apply(float NewPercent);

	int32 Target = 0;
	float Max = 100.0f;
	float Current = 100.0f;
	float GpuMs = 0.0f;
	float SinceChange = 0.0f;
	float Warmup = 1.0f;
};
