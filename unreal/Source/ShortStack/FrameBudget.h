// Dynamic resolution that works everywhere the game runs (Unreal's own only runs in a packaged game, not in the editor).
//
// Each frame the GPU's real time is read back; the render scale (r.ScreenPercentage, temporal super resolution
// rebuilding the full output from it) is moved to hold a frame-time budget: down quickly when a scene is heavier than
// the budget, back up slowly when there's room, never above the player's Resolution Scale (which can go past 100% to
// supersample when the GPU has power to spare) and never below a floor.
//
// It's also the frame's probe (design spec 3.8): what the frame, the game thread, the render thread and the GPU cost,
// smoothed, whether or not the scale moves, and whether the scale sits at its floor (the people's budget cuts the
// pedestrians first then, spec 6.7). The street's tests read it (ss.Street.Describe, ss.Street.Budget).
#pragma once

#include "CoreMinimal.h"

/** What the frame costs now (milliseconds, smoothed over about half a second). */
struct FFrameCost
{
	float FrameMs = 0.0f;
	float GameMs = 0.0f;
	float DrawMs = 0.0f;
	float GpuMs = 0.0f;
};

class FFrameBudget
{
public:
	/** TargetFps 0: a fixed render scale (MaxPercent). Otherwise the scale moves between MinPercent and MaxPercent. */
	void Configure(int32 TargetFps, int32 MaxPercent);
	/** Every frame, from the game mode. */
	void Tick(float DeltaSeconds);
	/** The render scale now (percent). */
	float Percent() const { return Current; }
	/** The frame rate it holds (0: a fixed scale). */
	int32 TargetFps() const { return Target; }
	/** The frame's cost now: the whole frame, the game thread, the render thread and the GPU. */
	const FFrameCost& Cost() const { return Smoothed; }
	/** The scale has sat at its floor for a couple of seconds with the GPU still over its budget: the scene itself is too heavy. */
	bool IsAtFloor() const { return FloorSeconds >= FloorHold; }
	/** One line: "render 67% (holding 60 fps) | frame 16.4 ms, game 6.1, draw 5.2, gpu 14.8". */
	FString Describe() const;

	static constexpr float MinPercent = 60.0f;

private:
	void Apply(float NewPercent);
	/** Reads this frame's times (each frame, whatever the scale does). */
	void Probe(float DeltaSeconds, float GpuNow);

	int32 Target = 0;
	float Max = 100.0f;
	float Current = 100.0f;
	float GpuMs = 0.0f;
	float SinceChange = 0.0f;
	float Warmup = 1.0f;
	FFrameCost Smoothed;
	float FloorSeconds = 0.0f;
	static constexpr float FloorHold = 2.0f;
};
