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
 * by the street HUD (two boxes). The night log (the table card's NIGHT page) is the same lines kept.
 *
 * The rules: at most MaxShown boxes, the newest at the bottom; over the cap the oldest room line goes first and the
 * floor's never does. At most two lines a box (about 42 characters each at 100%). A caption holds 1.5 s plus a second
 * for every 15 characters, between 2.6 and 8 s, and never less than its speech. People get a teal name tag, a line said
 * to the player a warm one, the floor an amber FLOOR tag with a speaker and a rule, sounds are bracketed and dimmed, and
 * in Everything mode a speaker off to the side gets a mark (left, right, behind) before the name.
 */
struct Caption
{
	std::string Speaker; // "Rick", "Mrs. Park"; empty for nobody in particular
	std::string Line;    // "All in. Eighteen thousand four hundred." / "[Applause.]"
	int Style = 0;       // 0 a person, 1 the PA (the floor), 2 a sound, 3 said to the player
	int Side = 0;        // where the speaker is: 0 in view, -1 off to the left, 1 off to the right, 2 behind
	double At = 0.0;     // when it started (seconds, the HUD's clock)
	float Hold = 0.0f;   // how long it stays up; 0: CaptionHold(Line, 0)
	int Priority = 0;    // over the cap, the lowest (then the oldest) goes first; the floor is never dropped
	std::string Clock;   // the room's clock when it was said ("9:42 PM"), for the night log
	bool Room = false;   // another table or the rail: Everything mode only (Dialogue is the player's table, the floor and
	                     // anyone talking to them)
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
/** Whether S shows Line at all (its mode), whatever its age. */
SHORTSTACKCORE_API bool CaptionShown(const Caption& Line, const CaptionStyle& S);
SHORTSTACKCORE_API void DrawCaptions(Canvas& C, const std::vector<Caption>& Lines, const CaptionStyle& S, float BottomY, float MaxWidth, double Now);
/**
 * The night log in R: a title ("THE NIGHT · EMBERCREST NIGHTLY") and the newest lines that fit, oldest at the top, each
 * with the room's clock in mono, the speaker's tag and the line (two rows at most).
 */
SHORTSTACKCORE_API void DrawNightLog(Canvas& C, const std::vector<Caption>& Lines, const std::string& Title, const Rect& R, double Now);
} // namespace ui
} // namespace ss
