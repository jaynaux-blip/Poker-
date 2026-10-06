#include "FrameBudget.h"

#include "HAL/IConsoleManager.h"
#include "RHI.h"
#include "RenderTimer.h"

static TAutoConsoleVariable<int32> CVarFrameBudgetLog(TEXT("ss.FrameBudget.Log"), 0, TEXT("1: log each change of the dynamic render scale."));

void FFrameBudget::Configure(int32 TargetFps, int32 MaxPercent)
{
	Target = FMath::Clamp(TargetFps, 0, 360);
	Max = FMath::Clamp(static_cast<float>(MaxPercent), MinPercent, 200.0f);
	Warmup = 1.0f;
	FloorSeconds = 0.0f;
	// A fixed scale, or start at the top and let the first second of frames find the level.
	Apply(Max);
}

void FFrameBudget::Probe(float DeltaSeconds, float GpuNow)
{
	// The engine's own frame times (as stat unit shows them): the game thread's, the render thread's, the GPU's.
	const float Game = static_cast<float>(FPlatformTime::ToMilliseconds(GGameThreadTime));
	const float Draw = static_cast<float>(FPlatformTime::ToMilliseconds(GRenderThreadTime));
	const float Frame = DeltaSeconds * 1000.0f;
	const float K = FMath::Clamp(DeltaSeconds * 4.0f, 0.02f, 1.0f); // about half a second's worth
	auto Ease = [K](float& Into, float Now) { Into = Into <= 0.0f ? Now : FMath::Lerp(Into, Now, K); };
	if (Frame > 0.0f && Frame < 1000.0f)
	{
		Ease(Smoothed.FrameMs, Frame);
	}
	if (Game > 0.0f && Game < 1000.0f)
	{
		Ease(Smoothed.GameMs, Game);
	}
	if (Draw > 0.0f && Draw < 1000.0f)
	{
		Ease(Smoothed.DrawMs, Draw);
	}
	if (GpuNow > 0.01f && GpuNow < 1000.0f)
	{
		Ease(Smoothed.GpuMs, GpuNow);
	}
}

void FFrameBudget::Tick(float DeltaSeconds)
{
	const uint32 Cycles = RHIGetGPUFrameCycles(0);
	const float Ms = static_cast<float>(FPlatformTime::ToMilliseconds(Cycles));
	Probe(DeltaSeconds, Ms);
	if (Target <= 0)
	{
		FloorSeconds = 0.0f;
		return;
	}
	if (Ms <= 0.01f || Ms > 1000.0f)
	{
		return;
	}
	// The GPU's time, smoothed over a few frames (one slow frame shouldn't drop the resolution).
	GpuMs = GpuMs <= 0.0f ? Ms : FMath::Lerp(GpuMs, Ms, 0.12f);
	SinceChange += DeltaSeconds;
	Warmup = FMath::Max(0.0f, Warmup - DeltaSeconds);
	// The budget leaves a little room for the rest of the frame. Pixels go with the square of the scale.
	const float Budget = 1000.0f / static_cast<float>(Target) * 0.92f;
	// At the floor and still over: the scene is too heavy for any scale (held a moment, so a hitch doesn't count).
	FloorSeconds = Current <= MinPercent + 0.5f && GpuMs > Budget ? FloorSeconds + DeltaSeconds : 0.0f;
	if (Warmup > 0.0f || SinceChange < 0.25f)
	{
		return;
	}
	const float Want = FMath::Clamp(Current * FMath::Sqrt(Budget / GpuMs), MinPercent, Max);
	float Next = Current;
	if (Want < Current - 1.0f)
	{
		Next = FMath::Max(Want, Current - 10.0f); // over budget: down at once
	}
	else if (Want > Current + 3.0f && GpuMs < Budget * 0.88f)
	{
		Next = FMath::Min(Want, Current + 2.0f); // room to spare: up gently, so it doesn't hunt
	}
	if (FMath::Abs(Next - Current) >= 1.0f)
	{
		if (CVarFrameBudgetLog.GetValueOnGameThread() != 0)
		{
			UE_LOG(LogTemp, Display, TEXT("Frame budget: GPU %.1f ms for a %.1f ms budget, render scale %.0f%% -> %.0f%%"), GpuMs, Budget, Current, Next);
		}
		Apply(FMath::RoundToFloat(Next));
		SinceChange = 0.0f;
	}
}

void FFrameBudget::Apply(float NewPercent)
{
	Current = NewPercent;
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage")))
	{
		Var->Set(Current, ECVF_SetByGameSetting);
	}
}

FString FFrameBudget::Describe() const
{
	const FString Scale = Target > 0 ? FString::Printf(TEXT("render %.0f%% (holding %d fps%s)"), Current, Target, IsAtFloor() ? TEXT(", at its floor") : TEXT(""))
									 : FString::Printf(TEXT("render %.0f%% (fixed)"), Current);
	return FString::Printf(TEXT("%s | frame %.1f ms, game %.1f, draw %.1f, gpu %.1f"), *Scale, Smoothed.FrameMs, Smoothed.GameMs, Smoothed.DrawMs, Smoothed.GpuMs);
}
