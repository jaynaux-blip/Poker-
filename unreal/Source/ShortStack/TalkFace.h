#pragma once

#include "CoreMinimal.h"

/**
 * The talk flap: syllables in the jaw and lips while someone speaks, and the nod and tilt that go with them. A copy of the
 * Back Room's (BackRoomPlayerTells.cpp, "Talking: syllables in the jaw and lips"; BackRoomPlayer.cpp, the head's nod and
 * tilt while TalkLeft runs), shared by the street's people through AShortStackCharacter::SetTalking. The Back Room keeps
 * its own copy for now (spec 6.6). looks wrote it in Phase A; people-ue owns it from Phase B.
 *
 * Controls are RigLogic's CTRL_expressions_* names without the prefix ("jawOpen", "mouthFunnelUL"); Both adds the L and
 * R of a paired one ("mouthStretch": mouthStretchL and mouthStretchR).
 */
namespace TalkFace
{
/** How much of the flap shows with Left seconds of talking to go: all of it, fading over the last 0.2 s. */
inline float Weight(float Left)
{
	return FMath::Clamp(Left / 0.2f, 0.0f, 1.0f);
}

/** Loudness (0..1) as a gain on the mouth: 0.6, an ordinary voice, is the Back Room's flap exactly; 1 opens it a third wider. */
inline float Gain(float Loudness)
{
	return 0.55f + 0.75f * FMath::Clamp(Loudness, 0.0f, 1.0f);
}

/** How long a line takes to say, as the Back Room's Say has it: half a second, and 55 ms a character. */
inline float SecondsFor(const FString& Line)
{
	return 0.5f + 0.055f * static_cast<float>(Line.Len());
}

/** The mouth at Time (any running clock, seconds), at weight W (Weight) and Loudness 0..1. */
inline void Mouth(float Time, float W, float Loudness, TFunctionRef<void(const TCHAR*, float)> Add, TFunctionRef<void(const TCHAR*, float)> Both)
{
	if (W <= 0.0f)
	{
		return;
	}
	const float G = Gain(Loudness);
	const float Syl = 0.5f + 0.5f * FMath::Sin(Time * 15.0f + 2.0f * FMath::Sin(Time * 3.3f));
	const float Shape = 0.5f + 0.5f * FMath::Sin(Time * 6.1f + 1.3f);
	Add(TEXT("jawOpen"), W * G * (0.06f + 0.16f * Syl));
	Add(TEXT("mouthFunnelUL"), W * G * 0.2f * Shape * (1.0f - Syl));
	Add(TEXT("mouthFunnelUR"), W * G * 0.2f * Shape * (1.0f - Syl));
	Add(TEXT("mouthFunnelDL"), W * G * 0.2f * Shape * (1.0f - Syl));
	Add(TEXT("mouthFunnelDR"), W * G * 0.2f * Shape * (1.0f - Syl));
	Both(TEXT("mouthStretch"), W * G * 0.12f * (1.0f - Shape) * Syl);
	Both(TEXT("mouthUpperLipRaise"), W * G * 0.1f * Syl);
}

/** The head while talking (degrees, added to whatever it's doing): a nod (pitch, + up) and a tilt (roll). */
inline float Nod(float Time, float W)
{
	return W * 2.0f * FMath::Sin(Time * 5.3f) * FMath::Sin(Time * 1.7f);
}

inline float Tilt(float Time, float W)
{
	return W * 1.5f * FMath::Sin(Time * 2.1f);
}
} // namespace TalkFace
