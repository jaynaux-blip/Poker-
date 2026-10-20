#pragma once

#include "ShortStack/UI/Canvas.h"

#include <array>
#include <cstdint>
#include <cstring>

namespace ss
{
namespace ui
{
/**
 * RiverLine palette (web/src/client/canvasui.ts). Mutable since update 2's A3 (§1.8): a SkinScope swaps all 16 tokens for
 * a rival site's while its client, a table tile or a stream preview of it draws, and puts RiverLine's back after. Outside
 * a scope these are always RiverLine's values. Game thread only, and core only: an inline variable is per module, so a
 * scope opened in the game module would not reach the core's draws (no game file uses pal:: today).
 */
namespace pal
{
inline Color Bg = Hex(0x0a111c);
inline Color Bg2 = Hex(0x0e1726);
inline Color Panel = Hex(0x111c2e);
inline Color Panel2 = Hex(0x16233a);
inline Color Line = Hex(0x223352);
inline Color Ink = Hex(0xe6edf7);
inline Color Muted = Hex(0x7b8aa3);
inline Color Dim = Hex(0x4d5b73);
inline Color Accent = Hex(0x27d3c3);
inline Color Accent2 = Hex(0x3b82f6);
inline Color Gold = Hex(0xf2c14e);
inline Color Red = Hex(0xef4d5a);
inline Color Orange = Hex(0xf28a3a);
inline Color Green = Hex(0x3ecf6e);
inline Color Felt = Hex(0x0f6a4e);
inline Color FeltDark = Hex(0x083a2b);
} // namespace pal

/** A site's palette, token for token with pal:: (update 2 §1.8). Every P0 skin is dark (the lobby's literal colours). */
struct Skin
{
	Color Bg, Bg2, Panel, Panel2, Line, Ink, Muted, Dim, Accent, Accent2, Gold, Red, Orange, Green, Felt, FeltDark;

	/** From 16 hex values in pal:: order: a net::SiteDef's Palette. */
	static Skin FromHex(const std::array<uint32_t, 16>& Hexes)
	{
		Skin S;
		Color* Tokens[16] = {&S.Bg, &S.Bg2, &S.Panel, &S.Panel2, &S.Line, &S.Ink, &S.Muted, &S.Dim, &S.Accent, &S.Accent2, &S.Gold, &S.Red, &S.Orange, &S.Green, &S.Felt, &S.FeltDark};
		for (size_t I = 0; I < 16; ++I)
		{
			*Tokens[I] = Hex(Hexes[I]);
		}
		return S;
	}
	/** The 16 tokens as they are now (RiverLine's outside any scope). */
	static Skin Current()
	{
		return {pal::Bg, pal::Bg2, pal::Panel, pal::Panel2, pal::Line, pal::Ink, pal::Muted, pal::Dim, pal::Accent, pal::Accent2, pal::Gold, pal::Red, pal::Orange, pal::Green, pal::Felt,
			pal::FeltDark};
	}
	/**
	 * A hash of the 16 tokens (review of A3). Canvas::Cached replays a fragment as it was first painted, so art cached from a
	 * paint that reads pal:: mixes Skin::Current().Key() into its key, or a page drawn in two skins replays one skin's
	 * colours in the other's.
	 */
	uint64_t Key() const
	{
		const Color* Tokens[16] = {&Bg, &Bg2, &Panel, &Panel2, &Line, &Ink, &Muted, &Dim, &Accent, &Accent2, &Gold, &Red, &Orange, &Green, &Felt, &FeltDark};
		uint64_t H = 0xcbf29ce484222325ull;
		for (const Color* C : Tokens)
		{
			for (const float F : {C->R, C->G, C->B, C->A})
			{
				uint32_t Bits = 0;
				std::memcpy(&Bits, &F, sizeof(Bits));
				H = (H ^ Bits) * 0x100000001b3ull;
			}
		}
		return H;
	}
	/** Writes all 16 into pal::. */
	void Apply() const
	{
		pal::Bg = Bg;
		pal::Bg2 = Bg2;
		pal::Panel = Panel;
		pal::Panel2 = Panel2;
		pal::Line = Line;
		pal::Ink = Ink;
		pal::Muted = Muted;
		pal::Dim = Dim;
		pal::Accent = Accent;
		pal::Accent2 = Accent2;
		pal::Gold = Gold;
		pal::Red = Red;
		pal::Orange = Orange;
		pal::Green = Green;
		pal::Felt = Felt;
		pal::FeltDark = FeltDark;
	}
};

/**
 * Draws in a site's colours (update 2 §1.8): swaps all 16 pal:: tokens for S's and restores what was there when the scope
 * ends, so scopes nest (a rival tile inside RiverLine's client, a preview inside a tile). The scope sites: a rival
 * client's Draw, each table tile, the Kast stream preview. Game thread only.
 */
struct SkinScope
{
	explicit SkinScope(const Skin& S) : Saved(Skin::Current()) { S.Apply(); }
	~SkinScope() { Saved.Apply(); }
	SkinScope(const SkinScope&) = delete;
	SkinScope& operator=(const SkinScope&) = delete;
	Skin Saved;
};

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
