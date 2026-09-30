#pragma once

#include "ShortStack/UI/Canvas.h"

namespace ss
{
namespace ui
{
/** RiverLine palette (web/src/client/canvasui.ts). */
namespace pal
{
inline const Color Bg = Hex(0x0a111c);
inline const Color Bg2 = Hex(0x0e1726);
inline const Color Panel = Hex(0x111c2e);
inline const Color Panel2 = Hex(0x16233a);
inline const Color Line = Hex(0x223352);
inline const Color Ink = Hex(0xe6edf7);
inline const Color Muted = Hex(0x7b8aa3);
inline const Color Dim = Hex(0x4d5b73);
inline const Color Accent = Hex(0x27d3c3);
inline const Color Accent2 = Hex(0x3b82f6);
inline const Color Gold = Hex(0xf2c14e);
inline const Color Red = Hex(0xef4d5a);
inline const Color Orange = Hex(0xf28a3a);
inline const Color Green = Hex(0x3ecf6e);
inline const Color Felt = Hex(0x0f6a4e);
inline const Color FeltDark = Hex(0x083a2b);
} // namespace pal

/** Pointer state fed by the host (a ray from the camera through the mouse onto the laptop screen). */
struct Pointer
{
	float X = -1.0f;
	float Y = -1.0f;
	bool Down = false;
	bool Pressed = false;  // went down since the last frame
	bool Released = false; // went up since the last frame
	float Wheel = 0.0f;
	bool Active = false; // over the screen

	void EndFrame()
	{
		Pressed = false;
		Released = false;
		Wheel = 0.0f;
	}
};

inline TextStyle Ts(float Size, int Weight = 500, const Color& Col = pal::Ink, Align A = Align::Left, Baseline B = Baseline::Alphabetic, bool Mono = false, float MaxWidth = 0.0f)
{
	TextStyle S;
	S.Size = Size;
	S.Weight = Weight;
	S.Col = Col;
	S.HAlign = A;
	S.VAlign = B;
	S.Mono = Mono;
	S.MaxWidth = MaxWidth;
	return S;
}

enum class ButtonKind : int
{
	Primary,
	Secondary,
	Danger,
	Ghost,
	Gold,
};

struct ButtonOpts
{
	ButtonKind Kind = ButtonKind::Secondary;
	bool Enabled = true;
	float Size = 22.0f;
	std::string Sub;
	std::string Hotkey;
};

/**
 * Minimal immediate-mode UI for the in-world laptop screen: widgets draw
 * into the canvas and test the pointer as they go. Port of the UI class in
 * web/src/client/canvasui.ts. Logical space is 1600 x 1000.
 */
class Ui
{
public:
	Pointer Ptr;
	bool CursorIsPointer = false;
	double Time = 0.0;

	void Begin(Canvas& InCanvas, double InTime);
	void End();
	Canvas& Cv() { return *C; }

	bool Hover(const Rect& R) const;
	struct ClickState
	{
		bool Hover = false;
		bool Down = false;
		bool Clicked = false;
	};
	/** Clicked means press and release inside the same widget. */
	ClickState Clickable(const std::string& Id, const Rect& R, bool Enabled = true);

	void RRect(const Rect& R, float Radius, const Paint& Fill);
	void RRect(const Rect& R, float Radius, const Paint& Fill, const Color& Stroke, float LineWidth = 1.0f);
	void RRectStroke(const Rect& R, float Radius, const Color& Stroke, float LineWidth = 1.0f);
	float Text(const std::string& S, float X, float Y, const TextStyle& Style) { return C->Text(S, X, Y, Style); }
	float Measure(const std::string& S, float Size, int Weight = 500, bool Mono = false) const { return C->Measure(S, Size, Weight, Mono); }
	bool Button(const std::string& Id, const Rect& R, const std::string& Label, const ButtonOpts& Opts = ButtonOpts());
	/** Horizontal slider; returns the new value. */
	double Slider(const std::string& Id, const Rect& R, double Value, double Min, double Max, double Step = 1.0);

private:
	Canvas* C = nullptr;
	std::string PressedId;
};

// ------------------------------------------------------------------ card art

struct CardOpts
{
	float Flip = 1.0f; // 0..1 animates a turn-over
	float Alpha = 1.0f;
	float Rotate = 0.0f;
	bool Dim = false;
	bool Highlight = false;
};

/** Four-color deck (clubs green, diamonds blue, hearts red, spades black). */
Color SuitColor(int Suit);
/** A suit symbol drawn from shapes, filling the square at (X, Y) of side Size. */
void DrawSuit(Canvas& C, int Suit, float X, float Y, float Size, const Color& Col);
/** Card at top-left (X, Y); Card < 0 draws the back. */
void DrawCard(Canvas& C, int Card, float X, float Y, float W, float H, const CardOpts& Opts = CardOpts());
/** A small stack of chips representing Amount, centered at (X, Y). */
void DrawChipStack(Canvas& C, Chips Amount, float X, float Y, float Scale = 1.0f, float Alpha = 1.0f);
} // namespace ui
} // namespace ss
