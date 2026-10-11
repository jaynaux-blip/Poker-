// What BackRoomGameMode.cpp (the table: the pawn, the night, the pause menu) and BackRoomHud.cpp (the table's HUD) share.
#pragma once

#include "CoreMinimal.h"

namespace BackRoomGameDetail
{
inline float Ease(float X, float Target, float Rate, float Dt)
{
	return FMath::Lerp(X, Target, 1.0f - FMath::Exp(-Rate * Dt));
}

inline float Smooth(float A, float B, float X)
{
	return FMath::SmoothStep(A, B, X);
}

/** "$1,234" */
inline FString Dollars(int64 Amount)
{
	return TEXT("$") + FText::AsNumber(FMath::Abs(Amount)).ToString();
}
} // namespace BackRoomGameDetail
