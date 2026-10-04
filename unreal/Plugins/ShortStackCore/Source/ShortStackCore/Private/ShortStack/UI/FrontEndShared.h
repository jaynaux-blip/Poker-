#pragma once
// Shared by the front end's pages (FrontEnd.cpp) and the character creator (FrontEndCreator.cpp).

#include "ShortStack/Game/Format.h"
#include "ShortStack/UI/FrontEnd.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace ss
{
namespace ui
{
namespace frontend_detail
{
inline const Color MenuInk = Hex(0xeef1f6);
inline const Color MenuMuted = Hex(0x8c95a8);
inline const Color MenuDim = Hex(0x4a5366);
inline const Color MenuNeon = Hex(0xff2e88);
inline const Color MenuTeal = Hex(0x27d3c3);
inline const Color MenuGold = Hex(0xf2c14e);
inline const Color MenuWarn = Hex(0xff5a5f);
inline const Color MenuShade = Hex(0x030408);
inline const Color MenuPanel = Hex(0x080a12);
inline const float MenuMargin = 132.0f;

/**
 * The creator's portrait card's width, and its form column's beside it (capped on ultrawide screens). On 4:3 and 5:4
 * the card gives up some width so the form keeps room for its buttons.
 */
inline float CreatorCardWidth(float ViewW)
{
	return std::clamp(ViewW * 0.31f, 400.0f, 640.0f);
}

inline float CreatorFormWidth(float ViewW)
{
	return std::min(1060.0f, ViewW - MenuMargin * 2.0f - CreatorCardWidth(ViewW) - 64.0f);
}

/** A vector icon for each background, drawn in its color (FrontEndCreator.cpp). */
void BackgroundIcon(Canvas& C, hero::Background B, float Cx, float Cy, float S, const Color& Col);

/** Credits, top to bottom. "#" starts a heading, "*" the title, "~" small print, "" a gap. */
inline const char* const CreditLines[] = {
	"*SHORT STACK",
	"~NIGHT ONE",
	"",
	"#CREATED BY",
	"jaynaux",
	"",
	"#POKER ENGINE",
	"ShortStackCore",
	"~No-Limit Hold'em with side pots, multi-table tournaments with ICM,",
	"~nine opponent archetypes and a coach that grades every decision",
	"",
	"#BUILT WITH",
	"Unreal Engine 5",
	"Blender",
	"Claude by Anthropic",
	"",
	"#TYPE",
	"Roboto",
	"~Apache License 2.0",
	"",
	"#SOUND",
	"Synthesized in code",
	"~Every chip, card, drop of rain and roll of thunder",
	"",
	"#THANKS FOR PLAYING",
	"Don't tilt.",
};

inline float Ease(double V)
{
	return static_cast<float>(EaseOutCubic(Clamp01(V)));
}

inline bool Inside(const Rect& R, float X, float Y)
{
	return X >= R.X && X <= R.X + R.W && Y >= R.Y && Y <= R.Y + R.H;
}

inline std::vector<std::string> Utf8Glyphs(const std::string& S)
{
	std::vector<std::string> Out;
	for (size_t I = 0; I < S.size();)
	{
		const unsigned char Lead = static_cast<unsigned char>(S[I]);
		const size_t Len = Lead >= 0xF0 ? 4 : Lead >= 0xE0 ? 3 : Lead >= 0xC0 ? 2 : 1;
		Out.push_back(S.substr(I, Len));
		I += Len;
	}
	return Out;
}

inline float TrackedWidth(Canvas& Cv, const std::string& S, float Size, int Weight, float Track)
{
	const std::vector<std::string> Glyphs = Utf8Glyphs(S);
	float Sum = 0.0f;
	for (const std::string& G : Glyphs)
	{
		Sum += Cv.Measure(G, Size, Weight);
	}
	return Sum + Track * static_cast<float>(Glyphs.empty() ? 0 : Glyphs.size() - 1);
}

/** Letter-spaced text, one run per glyph. Returns the width. */
inline float TrackedText(Canvas& Cv, const std::string& S, float X, float Y, float Size, int Weight, const Color& Col, float Track, Align A = Align::Left)
{
	const float Wd = TrackedWidth(Cv, S, Size, Weight, Track);
	float Px = X - (A == Align::Center ? Wd * 0.5f : A == Align::Right ? Wd : 0.0f);
	for (const std::string& G : Utf8Glyphs(S))
	{
		Px += Cv.Text(G, Px, Y, Ts(Size, Weight, Col)) + Track;
	}
	return Wd;
}

inline std::vector<std::string> WrapText(Canvas& Cv, const std::string& Text, float MaxW, float Size, int Weight)
{
	std::vector<std::string> Lines;
	std::string Line;
	size_t Start = 0;
	while (Start <= Text.size())
	{
		size_t End = Text.find(' ', Start);
		if (End == std::string::npos)
		{
			End = Text.size();
		}
		const std::string Word = Text.substr(Start, End - Start);
		const std::string Trial = Line.empty() ? Word : Line + " " + Word;
		if (!Line.empty() && Cv.Measure(Trial, Size, Weight) > MaxW)
		{
			Lines.push_back(Line);
			Line = Word;
		}
		else
		{
			Line = Trial;
		}
		Start = End + 1;
	}
	if (!Line.empty())
	{
		Lines.push_back(Line);
	}
	return Lines;
}

/** A thin chevron centered at (X, Y) pointing left or right. */
inline void Chevron(Canvas& Cv, float X, float Y, float Size, bool Right, const Color& Col)
{
	const float D = Right ? 1.0f : -1.0f;
	Cv.StrokePolyline({{X - D * Size * 0.4f, Y - Size}, {X + D * Size * 0.5f, Y}, {X - D * Size * 0.4f, Y + Size}}, false, Col, 2.2f, true);
}

/** A small filled triangle arrow centered at (X, Y): 0 up, 1 down, 2 left, 3 right. */
inline void Arrow(Canvas& Cv, float X, float Y, float Size, int Dir, const Color& Col)
{
	std::vector<Vec2> P;
	switch (Dir)
	{
	case 0: P = {{X, Y - Size}, {X + Size, Y + Size * 0.6f}, {X - Size, Y + Size * 0.6f}}; break;
	case 1: P = {{X, Y + Size}, {X + Size, Y - Size * 0.6f}, {X - Size, Y - Size * 0.6f}}; break;
	case 2: P = {{X - Size, Y}, {X + Size * 0.6f, Y - Size}, {X + Size * 0.6f, Y + Size}}; break;
	default: P = {{X + Size, Y}, {X - Size * 0.6f, Y - Size}, {X - Size * 0.6f, Y + Size}}; break;
	}
	Cv.FillPolygon(P, Paint(Col));
}

/** How wide Glyph draws a key or button. */
inline float GlyphWidth(Canvas& Cv, const std::string& Name)
{
	if (Name == "A" || Name == "B" || Name == "X" || Name == "Y" || Name == "DPAD")
	{
		return 28.0f;
	}
	const bool IsArrow = Name == "UP" || Name == "DOWN" || Name == "LEFT" || Name == "RIGHT";
	return std::max(28.0f, (IsArrow ? 0.0f : Cv.Measure(Name, 13.0f, 700)) + 18.0f);
}

/** Keyboard key or gamepad button glyph with its baseline at Y. Returns the width. */
inline float Glyph(Canvas& Cv, const std::string& Name, float X, float Y, float Alpha)
{
	const float Top = Y - 21.0f;
	const float Hh = 28.0f;
	const Color Line = Rgba(238, 241, 246, 0.55f * Alpha);
	const Color Label = Rgba(238, 241, 246, 0.95f * Alpha);
	if (Name == "A" || Name == "B" || Name == "X" || Name == "Y")
	{
		const uint32_t Hue = Name == "A" ? 0x3ecf6eu : Name == "B" ? 0xef4d5au : Name == "X" ? 0x3b82f6u : 0xf2c14eu;
		Cv.FillCircle(X + 14.0f, Top + 14.0f, 14.0f, Paint(Hex(Hue, 0.92f * Alpha)));
		Cv.Text(Name, X + 14.0f, Top + 14.0f, Ts(14.0f, 900, Hex(0x07090d, Alpha), Align::Center, Baseline::Middle));
		return 28.0f;
	}
	if (Name == "DPAD")
	{
		const float Cx = X + 14.0f;
		const float Cy = Top + 14.0f;
		Cv.FillRoundRect({Cx - 4.5f, Cy - 13.0f, 9.0f, 26.0f}, 2.0f, Paint(Line));
		Cv.FillRoundRect({Cx - 13.0f, Cy - 4.5f, 26.0f, 9.0f}, 2.0f, Paint(Line));
		return 28.0f;
	}
	const bool IsArrow = Name == "UP" || Name == "DOWN" || Name == "LEFT" || Name == "RIGHT";
	const float TextW = IsArrow ? 0.0f : Cv.Measure(Name, 13.0f, 700);
	const float Bw = std::max(28.0f, TextW + 18.0f);
	Cv.StrokeRoundRect({X + 0.75f, Top + 0.75f, Bw - 1.5f, Hh - 1.5f}, 5.0f, Line, 1.5f);
	if (IsArrow)
	{
		const int Dir = Name == "UP" ? 0 : Name == "DOWN" ? 1 : Name == "LEFT" ? 2 : 3;
		Arrow(Cv, X + Bw * 0.5f, Top + Hh * 0.5f, 5.0f, Dir, Label);
	}
	else
	{
		Cv.Text(Name, X + Bw * 0.5f, Top + Hh * 0.5f, Ts(13.0f, 700, Label, Align::Center, Baseline::Middle));
	}
	return Bw;
}

/** Neon tube brightness: an ignition flicker when the sign first lights, then an occasional buzz. */
inline float NeonLevel(double Since, bool Ignite)
{
	if (Ignite)
	{
		static const double Steps[][2] = {{0.00, 0.0}, {0.30, 1.0}, {0.36, 0.15}, {0.47, 1.0}, {0.52, 0.3}, {0.66, 1.0}, {0.70, 0.55}, {0.80, 1.0}};
		if (Since < 0.30)
		{
			return 0.0f;
		}
		for (int I = 7; I >= 0; --I)
		{
			if (Since >= Steps[I][0])
			{
				if (I < 7)
				{
					return static_cast<float>(Steps[I][1]);
				}
				break;
			}
		}
	}
	const double Phase = std::fmod(Since + 3.1, 7.3);
	return Phase < 0.09 ? 0.35f : Phase < 0.14 ? 0.8f : 1.0f;
}
} // namespace frontend_detail
} // namespace ui
} // namespace ss
