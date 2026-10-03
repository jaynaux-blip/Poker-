#include "FrameBudget.h"

#include "HAL/IConsoleManager.h"
#include "RHI.h"

static TAutoConsoleVariable<int32> CVarFrameBudgetLog(TEXT("ss.FrameBudget.Log"), 0, TEXT("1: log each change of the dynamic render scale."));

void FFrameBudget::Configure(int32 TargetFps, int32 MaxPercent)
{
	Target = FMath::Clamp(TargetFps, 0, 360);
	Max = FMath::Clamp(static_cast<float>(MaxPercent), MinPercent, 200.0f);
	Warmup = 1.0f;
	// A fixed scale, or start at the top and let the first second of frames find the level.
	Apply(Max);
}

void FFrameBudget::Tick(float DeltaSeconds)
{
	if (Target <= 0)
	{
		return;
	}
	const uint32 Cycles = RHIGetGPUFrameCycles(0);
	const float Ms = static_cast<float>(FPlatformTime::ToMilliseconds(Cycles));
	if (Ms <= 0.01f || Ms > 1000.0f)
	{
		return;
	}
	// The GPU's time, smoothed over a few frames (one slow frame shouldn't drop the resolution).
	GpuMs = GpuMs <= 0.0f ? Ms : FMath::Lerp(GpuMs, Ms, 0.12f);
	SinceChange += DeltaSeconds;
	Warmup = FMath::Max(0.0f, Warmup - DeltaSeconds);
	if (Warmup > 0.0f || SinceChange < 0.25f)
	{
		return;
	}
	// The budget leaves a little room for the rest of the frame. Pixels go with the square of the scale.
	const float Budget = 1000.0f / static_cast<float>(Target) * 0.92f;
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
