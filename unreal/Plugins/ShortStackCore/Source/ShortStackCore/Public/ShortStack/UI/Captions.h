#pragma once

#include "ShortStack/UI/Canvas.h"

#include <string>
#include <vector>

namespace ss
{
namespace ui
{
/**
 * Captions (next_segment_spec §4.9): what the room says, on screen, with real settings (off, dialogue, everything;
 * size, backdrop, speaker names). Drawn in the table HUD's caption zone (bottom centre, the bottom edge at BottomY) and
 * by the street HUD. The night log (the table card's NIGHT page) is the same lines kept.
 *
 * Bodies: core-tableui (Captions.cpp). Phase A, day 1: stubs that compile and draw nothing.
 */
struct Caption
{
	std::string Speaker;
	std::string Line;
	int Style = 0; // 0 a person, 1 the PA, 2 a sound, 3 said to the player
	int Side = 0;
	double At = 0.0;
	float Hold = 0.0f;
	int Priority = 0;
	std::string Clock;
};
struct CaptionStyle
{
	int Mode = 1; // GameSettings::Captions: 0 off, 1 dialogue, 2 everything
	int Size = 100;
	int Backdrop = 72;
	bool Names = true;
	int MaxShown = 3;
};
/** How long a line stays up (seconds), from its length and the speech that carries it. */
SHORTSTACKCORE_API float CaptionHold(const std::string& Line, float SpeechSeconds);
SHORTSTACKCORE_API void DrawCaptions(Canvas& C, const std::vector<Caption>& Lines, const CaptionStyle& S, float BottomY, float MaxWidth, double Now);
} // namespace ui
} // namespace ss
