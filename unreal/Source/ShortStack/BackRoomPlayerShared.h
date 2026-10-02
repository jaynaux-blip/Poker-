#pragma once

#include "CoreMinimal.h"

#include "BackRoomStage.h"

// Shared by ABackRoomPlayer's source files (BackRoomPlayer.cpp, BackRoomPlayerHands.cpp, BackRoomPlayerTells.cpp).
namespace BackRoomPlayerDetail
{
// The chair: the actor sits on its front edge; the pelvis rests SeatDepth behind it.
inline constexpr double SeatDepth = 20.0;
inline constexpr float SeatHeight = 46.0f;
// The rail's outer edge, ahead of the chair's front edge (ABackRoomStage seats players this far out),
// and in the body's space (+Y toward the table): the rail is 13 cm wide, the felt beyond it.
inline constexpr double RailGap = ABackRoomStage::RailGap;
inline constexpr double Rail = RailGap + SeatDepth;

/** Critically damped approach of X toward Target at Rate (per second). */
inline float Ease(float X, float Target, float Rate, float Dt)
{
	return FMath::Lerp(X, Target, 1.0f - FMath::Exp(-Rate * Dt));
}

inline FVector Ease(const FVector& X, const FVector& Target, float Rate, float Dt)
{
	return FMath::Lerp(X, Target, static_cast<double>(1.0f - FMath::Exp(-Rate * Dt)));
}

/** 0..1 with zero velocity and acceleration at both ends. */
inline float Smoother(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	return T * T * T * (T * (T * 6.0f - 15.0f) + 10.0f);
}

/** Up and back down over T in 0..1: rising over the first Attack share, falling over the last Release share. */
inline float Envelope(float T, float Attack = 0.2f, float Release = 0.3f)
{
	if (T <= 0.0f || T >= 1.0f)
	{
		return 0.0f;
	}
	if (T < Attack)
	{
		return Smoother(T / Attack);
	}
	if (T > 1.0f - Release)
	{
		return Smoother((1.0f - T) / Release);
	}
	return 1.0f;
}
} // namespace BackRoomPlayerDetail
