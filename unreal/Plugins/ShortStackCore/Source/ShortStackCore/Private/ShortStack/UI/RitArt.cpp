// Update 3 (SPEC §3.3, §3.4, §13.2; v1.2's red and black): RunItTwice's art, ui-rit's file (with its private RitArt.h): the
// type, the twice print and the twin stripe with the page's tally of them, the one-cut shapes, the slabs and their light,
// the marks (race-number boxes, tags, the star, the bell, the triangles, the ±), the carbon twill and the tape's scanlines
// (cached), the octagonal stamps, the split-flap, the screen state kept beside the class, and the copy. Every colour is a
// palette constant of RitArt.h or a token of the skin: none is a RiverLine hex.
#include "RitArt.h"
#include "../StrictFloat.h"
#include "RiverLineShared.h"

#include "ShortStack/Common.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/SitePlay.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/UI/Avatars.h"

#include <algorithm>
#include <cmath>

namespace ss
{
namespace ui
{
namespace rlrit_detail
{
// ------------------------------------------------------------------ type

TextStyle RitStyle(float Size, const Color& Col, RitFace Face, Align A, Baseline B, float MaxW)
{
	TextStyle St = Ts(Size, Face == RitFace::Body ? 500 : 700, Col, A, B, Face == RitFace::Mono, MaxW);
	if (Face == RitFace::Mono)
	{
		St.Weight = 500;
	}
	if (Face == RitFace::Cond)
	{
		St.Face = static_cast<int8_t>(Font::BoldCondensed);
	}
	return St;
}

float RitText(Canvas& C, const std::string& S, float X, float Y, float Size, const Color& Col, RitFace Face, Align A, Baseline B, float MaxW)
{
	return C.Text(S, X, Y, RitStyle(Size, Col, Face, A, B, MaxW));
}

float RitMeasure(const Canvas& C, const std::string& S, float Size, RitFace Face)
{
	switch (Face)
	{
	case RitFace::Cond: return C.Measure(S, Size, Font::BoldCondensed);
	case RitFace::Mono: return C.Measure(S, Size, Font::Mono);
	case RitFace::Name: return C.Measure(S, Size, Font::Bold);
	case RitFace::Body: break;
	}
	return C.Measure(S, Size, Font::Regular);
}

std::string RitUpper(const std::string& S)
{
	std::string Out = S;
	for (char& Ch : Out)
	{
		Ch = Ch >= 'a' && Ch <= 'z' ? static_cast<char>(Ch - 'a' + 'A') : Ch;
	}
	return Out;
}

float RitLabel(Canvas& C, const std::string& S, float X, float Y, float Size, const Color& Col, Align A)
{
	return RitText(C, RitUpper(S), X, Y, Size, Col, RitFace::Cond, A);
}

float RitSpaced(Canvas& C, const std::string& S, float X, float Y, float Size, const Color& Col, float Track)
{
	// One glyph at a time with Track between (the kickers the sites' test spells back together on one baseline).
	float Xx = X;
	for (size_t I = 0; I < S.size();)
	{
		const unsigned char B = static_cast<unsigned char>(S[I]);
		const size_t Len = B >= 0xF0 ? 4 : B >= 0xE0 ? 3 : B >= 0xC0 ? 2 : 1;
		const std::string G = S.substr(I, Len);
		Xx += RitText(C, G, Xx, Y, Size, Col) + Track;
		I += Len;
	}
	return Xx - X - Track;
}

float RitFit(const Canvas& C, const std::string& S, float Size, float Min, float MaxW)
{
	float Sz = Size;
	while (Sz > Min && RitMeasure(C, S, Sz) > MaxW)
	{
		Sz -= 1.0f;
	}
	return std::max(Sz, Min);
}

float RitParagraph(Canvas& C, const std::string& Text, float X, float Y, float MaxW, float Size, const Color& Col, float LineH, int MaxLines)
{
	std::vector<std::string> Lines;
	std::string Line;
	size_t Start = 0;
	while (Start <= Text.size())
	{
		size_t End = Text.find(' ', Start);
		End = End == std::string::npos ? Text.size() : End;
		const std::string Word = Text.substr(Start, End - Start);
		const std::string Test = Line.empty() ? Word : Line + " " + Word;
		if (RitMeasure(C, Test, Size, RitFace::Body) > MaxW && !Line.empty())
		{
			Lines.push_back(Line);
			Line = Word;
		}
		else
		{
			Line = Test;
		}
		Start = End + 1;
	}
	if (!Line.empty())
	{
		Lines.push_back(Line);
	}
	for (size_t I = 0; I < Lines.size() && static_cast<int>(I) < MaxLines; ++I)
	{
		std::string L = Lines[I];
		if (static_cast<int>(I) == MaxLines - 1 && I + 1 < Lines.size())
		{
			L += " " + Lines[I + 1];
		}
		RitText(C, L, X, Y, Size, Col, RitFace::Body, Align::Left, Baseline::Alphabetic, MaxW);
		Y += LineH;
	}
	return Y;
}

// ------------------------------------------------------------------ the tally (what a page drew of the signatures)

RitTally& RitTallyRef()
{
	static RitTally Tally;
	return Tally;
}

void RitPageBegin()
{
	RitTallyRef() = RitTally();
}

void RitPanelBegin()
{
	RitTally& T = RitTallyRef();
	T.Stripes = 0;
	++T.Panels;
}

const RitTally& RitTallyNow()
{
	return RitTallyRef();
}

void RitTallyStripe(uint32_t Rgb)
{
	if (Rgb != RitRed)
	{
		return; // only Signal Red twin stripes are the brand device (the League's zone lines are rules)
	}
	RitTally& T = RitTallyRef();
	++T.Stripes;
	T.MaxStripes = std::max(T.MaxStripes, T.Stripes);
}

// ------------------------------------------------------------------ the twice print

float RitTwiceOffset(float Size)
{
	return std::min(6.0f, std::max(2.0f, std::floor(Size / 16.0f + 0.5f)));
}

float RitTwice(Canvas& C, const std::string& S, float X, float Y, float Size, const Color& Fill, uint32_t Under, Align A)
{
	RitTally& T = RitTallyRef();
	if (Size >= RitTwiceFrom && T.Twice < RitTwiceCap)
	{
		const float O = RitTwiceOffset(Size);
		RitText(C, S, X + O, Y + O, Size, RitC(Under, Fill.A), RitFace::Cond, A);
		++T.Twice;
	}
	return RitText(C, S, X, Y, Size, Fill, RitFace::Cond, A);
}

// ------------------------------------------------------------------ shapes

std::vector<Vec2> RitCutPath(const Rect& R, float Cut)
{
	const float K = std::min(std::max(Cut, 0.0f), std::min(R.W, R.H));
	if (K <= 0.0f)
	{
		return {{R.X, R.Y}, {R.X + R.W, R.Y}, {R.X + R.W, R.Y + R.H}, {R.X, R.Y + R.H}};
	}
	return {{R.X, R.Y}, {R.X + R.W - K, R.Y}, {R.X + R.W, R.Y + K}, {R.X + R.W, R.Y + R.H}, {R.X, R.Y + R.H}};
}

void RitCutFill(Canvas& C, const Rect& R, float Cut, const Paint& P)
{
	if (Cut <= 0.0f)
	{
		C.FillRect(R, P);
		return;
	}
	C.FillPolygon(RitCutPath(R, Cut), P);
}

void RitCutStroke(Canvas& C, const Rect& R, float Cut, const Color& Col, float W)
{
	const float H = W / 2.0f;
	C.StrokePolyline(RitCutPath({R.X + H, R.Y + H, R.W - W, R.H - W}, std::max(0.0f, Cut - H)), true, Col, W);
}

Paint RitGlossPaint(const Rect& R)
{
	return Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, RitC(RitGloss), RitC(RitOnyx));
}

void RitPanel(Canvas& C, const Rect& R, float Cut, uint32_t Fill, uint32_t Line)
{
	RitPanelBegin();
	RitCutFill(C, R, Cut, RitC(Fill));
	RitCutStroke(C, R, Cut, RitC(Line), 1.0f);
}

void RitLedEdge(Canvas& C, float X, float Y, float W)
{
	C.FillRect({X, Y, W, 1.0f}, RitC(RitLed));
}

void RitGlow(Canvas& C, const Rect& R, uint32_t Rgb, float Alpha, float Blur)
{
	C.GlowRoundRect(R, 2.0f, RitC(Rgb, Alpha), Blur);
}

void RitSweep(Canvas& C, const Rect& R, float At, float Band)
{
	const float Cx = R.X + R.W * At;
	const float Half = R.H * 0.5f;
	C.PushClip(R);
	C.FillPolygon({{Cx - Band / 2.0f + Half, R.Y - 2.0f}, {Cx + Band / 2.0f + Half, R.Y - 2.0f}, {Cx + Band / 2.0f - Half, R.Y + R.H + 2.0f}, {Cx - Band / 2.0f - Half, R.Y + R.H + 2.0f}},
		RitC(RitWhite, 0.10f));
	C.PopClip();
}

void RitSlab(Canvas& C, const Rect& R, float Cut, bool Hover, double Now)
{
	RitGlow(C, {R.X, R.Y + 6.0f, R.W, R.H - 4.0f}, RitRed, 0.35f, 6.0f);
	RitCutFill(C, R, Cut, RitC(RitRed));
	RitCutFill(C, {R.X, R.Y, R.W, R.H / 2.0f}, std::min(Cut, R.H / 2.0f), RitC(RitWhite, 0.05f));
	RitLedEdge(C, R.X, R.Y, R.W - Cut);
	if (Hover)
	{
		// The light sweep crosses in 0.4 s and waits a beat before the next pass.
		const double T = std::fmod(Now, 1.4) / 0.4;
		if (T < 1.0)
		{
			RitSweep(C, R, static_cast<float>(-0.2 + 1.4 * T), 40.0f);
		}
	}
}

// ------------------------------------------------------------------ the twin stripe

void RitTwinH(Canvas& C, float X, float Y, float W, float K, uint32_t Rgb)
{
	RitTallyStripe(Rgb);
	C.FillRect({X, Y, W, 8.0f * K}, RitC(Rgb));
	C.FillRect({X, Y + 12.0f * K, W, 3.0f * K}, RitC(Rgb));
}

void RitTwinV(Canvas& C, float X, float Y, float H, float K, uint32_t Rgb)
{
	RitTallyStripe(Rgb);
	C.FillRect({X, Y, 8.0f * K, H}, RitC(Rgb));
	C.FillRect({X + 12.0f * K, Y, 3.0f * K, H}, RitC(Rgb));
}

void RitTwinSlash(Canvas& C, float X, float Y, float H, float K, uint32_t Rgb)
{
	RitTallyStripe(Rgb);
	const float D = H * 0.3249197f; // tan 18 degrees
	const float Bars[2][2] = {{X, 8.0f * K}, {X + 12.0f * K, 3.0f * K}};
	for (const auto& Bar : Bars)
	{
		C.FillPolygon({{Bar[0], Y}, {Bar[0] + Bar[1], Y}, {Bar[0] + Bar[1] - D, Y + H}, {Bar[0] - D, Y + H}}, RitC(Rgb));
	}
}

void RitTwinTick(Canvas& C, float X, float Y, float H, const Color& Col)
{
	const float D = H * 0.3249197f;
	C.FillPolygon({{X + D, Y}, {X + D + 3.0f, Y}, {X + 3.0f, Y + H}, {X, Y + H}}, Col);
	C.FillPolygon({{X + D + 5.0f, Y}, {X + D + 6.0f, Y}, {X + 6.0f, Y + H}, {X + 5.0f, Y + H}}, Col);
}

// ------------------------------------------------------------------ marks

void RitRaceBox(Canvas& C, float X, float Y, int N, bool Active, float S, float Size)
{
	C.FillRect({X, Y, S, S}, RitC(Active ? RitWhite : RitOnyx));
	C.StrokePolyline({{X + 0.5f, Y + 0.5f}, {X + S - 0.5f, Y + 0.5f}, {X + S - 0.5f, Y + S - 0.5f}, {X + 0.5f, Y + S - 0.5f}}, true, RitC(RitRed), 1.0f);
	RitText(C, std::to_string(N), X + S / 2.0f, Y + S / 2.0f, Size, RitC(Active ? RitOnyx : RitInk), RitFace::Cond, Align::Center, Baseline::Middle);
}

float RitTag(Canvas& C, float X, float Y, const std::string& S, uint32_t Fill, uint32_t Ink, float Size, float PadX, float H, float Cut, bool Outline)
{
	const std::string Text = RitUpper(S);
	const float W = RitMeasure(C, Text, Size) + PadX * 2.0f;
	if (Outline)
	{
		RitCutStroke(C, {X, Y, W, H}, Cut, RitC(Fill), 1.0f);
	}
	else
	{
		RitCutFill(C, {X, Y, W, H}, Cut, RitC(Fill));
	}
	RitText(C, Text, X + W / 2.0f, Y + H / 2.0f, Size, RitC(Ink), RitFace::Cond, Align::Center, Baseline::Middle);
	return W;
}

float RitParaTag(Canvas& C, float X, float Y, const std::string& S, float Size, float H)
{
	const std::string Text = RitUpper(S);
	const float W = RitMeasure(C, Text, Size) + 22.0f;
	const float D = H * 0.3249197f;
	C.FillPolygon({{X + D, Y}, {X + W + D, Y}, {X + W, Y + H}, {X, Y + H}}, RitC(RitRed));
	RitText(C, Text, X + W / 2.0f + D / 2.0f, Y + H / 2.0f, Size, RitC(RitWhite), RitFace::Cond, Align::Center, Baseline::Middle);
	return W + D;
}

void RitStar(Canvas& C, float Cx, float Cy, float R, bool On)
{
	std::vector<Vec2> P;
	for (int I = 0; I < 10; ++I)
	{
		const float A = -Pi / 2.0f + static_cast<float>(I) * Pi / 5.0f;
		const float Rr = I % 2 == 0 ? R : R * 0.42f;
		P.push_back({Cx + Rr * std::cos(A), Cy + Rr * std::sin(A)});
	}
	if (On)
	{
		C.FillPolygon(P, RitC(RitWhite));
	}
	else
	{
		C.StrokePolyline(P, true, RitC(RitChrome), 1.1f);
	}
}

void RitLock(Canvas& C, float X, float Y, const Color& Col)
{
	C.StrokeArc(X + 5.0f, Y + 4.0f, 3.0f, Pi, 2.0f * Pi, Col, 1.4f);
	C.FillRect({X, Y + 4.0f, 10.0f, 8.0f}, Col);
}

void RitBell(Canvas& C, float Cx, float Cy, float S, const Color& Col)
{
	const float K = S / 20.0f;
	C.FillPolygon({{Cx - 6.5f * K, Cy + 5.0f * K}, {Cx - 6.0f * K, Cy - 4.0f * K}, {Cx - 3.0f * K, Cy - 8.0f * K}, {Cx + 3.0f * K, Cy - 8.0f * K}, {Cx + 6.0f * K, Cy - 4.0f * K},
					  {Cx + 6.5f * K, Cy + 5.0f * K}, {Cx + 9.5f * K, Cy + 8.0f * K}, {Cx - 9.5f * K, Cy + 8.0f * K}},
		Col);
	C.FillCircle(Cx, Cy + 10.5f * K, 2.2f * K, Col);
	C.FillCircle(Cx, Cy - 9.5f * K, 1.6f * K, Col);
}

void RitTri(Canvas& C, float Cx, float Cy, float S, bool Up, const Color& Col)
{
	if (Up)
	{
		C.FillPolygon({{Cx, Cy - S * 0.55f}, {Cx + S * 0.6f, Cy + S * 0.45f}, {Cx - S * 0.6f, Cy + S * 0.45f}}, Col);
	}
	else
	{
		C.FillPolygon({{Cx - S * 0.6f, Cy - S * 0.45f}, {Cx + S * 0.6f, Cy - S * 0.45f}, {Cx, Cy + S * 0.55f}}, Col);
	}
}

void RitArrow(Canvas& C, float X, float Y, float W, const Color& Col, float Sw)
{
	C.StrokePolyline({{X, Y}, {X + W, Y}}, false, Col, Sw);
	C.StrokePolyline({{X + W - 5.0f, Y - 5.0f}, {X + W, Y}, {X + W - 5.0f, Y + 5.0f}}, false, Col, Sw);
}

float RitPlusMinus(Canvas& C, float X, float Baseline, float Size, const Color& Col)
{
	const float W = Size * 0.55f;
	const float Lw = std::max(1.2f, Size * 0.085f);
	const float Cx = X + W / 2.0f;
	const float Cy = Baseline - Size * 0.42f;
	const float Arm = Size * 0.2f;
	C.FillRect({Cx - Arm, Cy - Lw / 2.0f, Arm * 2.0f, Lw}, Col);
	C.FillRect({Cx - Lw / 2.0f, Cy - Arm, Lw, Arm * 2.0f}, Col);
	C.FillRect({Cx - Arm, Baseline - Size * 0.08f - Lw / 2.0f, Arm * 2.0f, Lw}, Col);
	return W;
}

/** A convex polygon clipped to a rect (Sutherland-Hodgman): the twill's stripes stay inside their ground. */
std::vector<Vec2> RitClipToRect(const std::vector<Vec2>& In, const Rect& R)
{
	std::vector<Vec2> Poly = In;
	for (int Edge = 0; Edge < 4 && !Poly.empty(); ++Edge)
	{
		const auto Inside = [&R, Edge](const Vec2& P) {
			switch (Edge)
			{
			case 0: return P.X >= R.X;
			case 1: return P.X <= R.X + R.W;
			case 2: return P.Y >= R.Y;
			default: return P.Y <= R.Y + R.H;
			}
		};
		const auto Cross = [&R, Edge](const Vec2& A, const Vec2& B) {
			float T = 0.0f;
			switch (Edge)
			{
			case 0: T = (R.X - A.X) / (B.X - A.X); break;
			case 1: T = (R.X + R.W - A.X) / (B.X - A.X); break;
			case 2: T = (R.Y - A.Y) / (B.Y - A.Y); break;
			default: T = (R.Y + R.H - A.Y) / (B.Y - A.Y); break;
			}
			return Vec2{A.X + (B.X - A.X) * T, A.Y + (B.Y - A.Y) * T};
		};
		std::vector<Vec2> Out;
		for (size_t I = 0; I < Poly.size(); ++I)
		{
			const Vec2& A = Poly[I];
			const Vec2& B = Poly[(I + 1) % Poly.size()];
			const bool Ia = Inside(A);
			const bool Ib = Inside(B);
			if (Ia)
			{
				Out.push_back(A);
			}
			if (Ia != Ib)
			{
				Out.push_back(Cross(A, B));
			}
		}
		Poly = Out;
	}
	return Poly;
}

void RitTwill(Canvas& C, const Rect& R)
{
	const uint64_t Key = 0x52697454776c6cull ^ (static_cast<uint64_t>(R.W) << 24) ^ (static_cast<uint64_t>(R.H) << 4) ^ (Skin::Current().Key() * 31ull);
	const float W = R.W;
	const float H = R.H;
	C.Cached(Key, R.X, R.Y, [W, H](Canvas& Cv) {
		const Rect Ground{0.0f, 0.0f, W, H};
		Cv.FillRect(Ground, RitC(RitTwillA));
		// 14-unit stripes on a 28-unit pitch, at 45 degrees: (W + H) / 28 quads (the rail's 42, at most 90).
		for (float K = -H; K < W; K += 28.0f)
		{
			const std::vector<Vec2> Quad = RitClipToRect({{K, H}, {K + 14.0f, H}, {K + 14.0f + H, 0.0f}, {K + H, 0.0f}}, Ground);
			if (Quad.size() >= 3)
			{
				Cv.FillPolygon(Quad, RitC(RitTwillB));
			}
		}
	});
}

void RitScanlines(Canvas& C, const Rect& R, uint32_t Rgb, float Alpha)
{
	const uint64_t Key = 0x5269745363616eull ^ (static_cast<uint64_t>(R.W) << 20) ^ static_cast<uint64_t>(R.H) ^ (static_cast<uint64_t>(Rgb) << 32) ^
		static_cast<uint64_t>(Alpha * 1000.0f) * 7919ull;
	const float W = R.W;
	const float H = R.H;
	C.Cached(Key, R.X, R.Y, [W, H, Rgb, Alpha](Canvas& Cv) {
		for (float Y = 1.0f; Y < H; Y += 3.0f)
		{
			Cv.FillRect({0.0f, Y, W, 1.0f}, RitC(Rgb, Alpha));
		}
	});
}

std::vector<Vec2> RitOctagon(const Rect& R, float Cut)
{
	const float K = std::min(Cut, std::min(R.W, R.H) / 2.0f);
	return {{R.X + K, R.Y}, {R.X + R.W - K, R.Y}, {R.X + R.W, R.Y + K}, {R.X + R.W, R.Y + R.H - K}, {R.X + R.W - K, R.Y + R.H}, {R.X + K, R.Y + R.H}, {R.X, R.Y + R.H - K}, {R.X, R.Y + K}};
}

void RitStampPaper(Canvas& C, float Cx, float Cy, const std::string& Word, float Size, float Scale, float Spread)
{
	const std::string Text = RitUpper(Word);
	const float W = RitMeasure(C, Text, Size) + Size * 1.5f;
	const float H = Size * 1.95f;
	const float Cut = H * 0.3f;
	C.Save();
	C.Translate(Cx, Cy);
	C.Scale(Scale, Scale);
	if (Spread > 0.0f)
	{
		C.StrokePolyline(RitOctagon({-W / 2.0f - 7.0f, -H / 2.0f - 7.0f, W + 14.0f, H + 14.0f}, Cut + 3.0f), true, RitC(RitRed, 0.25f * Spread), 5.0f);
	}
	C.StrokePolyline(RitOctagon({-W / 2.0f, -H / 2.0f, W, H}, Cut), true, RitC(RitRed, 0.92f), 3.2f);
	C.StrokePolyline(RitOctagon({-W / 2.0f + 6.0f, -H / 2.0f + 6.0f, W - 12.0f, H - 12.0f}, Cut - 2.5f), true, RitC(RitRed, 0.92f), 1.2f);
	// The word at full alpha: Signal Red ink on Paper is 4.83:1 (at 0.92 it would fall to 4.50).
	RitText(C, Text, 0.0f, 0.0f, Size, RitC(RitRed), RitFace::Cond, Align::Center, Baseline::Middle);
	C.Restore();
}

void RitStampDark(Canvas& C, float Cx, float Cy, const std::string& Word, float Size)
{
	const std::string Text = RitUpper(Word);
	const float W = RitMeasure(C, Text, Size) + Size * 1.4f;
	const float H = Size * 1.8f;
	C.FillPolygon(RitOctagon({Cx - W / 2.0f, Cy - H / 2.0f, W, H}, H * 0.3f), RitC(RitRed));
	RitText(C, Text, Cx, Cy, Size, RitC(RitWhite), RitFace::Cond, Align::Center, Baseline::Middle);
}

void RitSquareAvatar(Canvas& C, float X, float Y, float S, const std::string& Name, const Color& Frame)
{
	C.PushClip({X, Y, S, S});
	DrawAvatar(C, X + S / 2.0f, Y + S / 2.0f, S * 0.72f, AvatarFor(Name));
	C.PopClip();
	if (Frame.A > 0.0f)
	{
		C.StrokePolyline({{X - 0.5f, Y - 0.5f}, {X + S + 0.5f, Y - 0.5f}, {X + S + 0.5f, Y + S + 0.5f}, {X - 0.5f, Y + S + 0.5f}}, true, Frame, 1.0f);
	}
}

// ------------------------------------------------------------------ the split-flap

float RitFlap(Canvas& C, float X, float Y, const std::string& Text, float Size, float H, int Settled, RitTile Tile, const Color& Ink, float W)
{
	const float Pad = 3.0f;
	std::vector<float> Adv;
	float Tw = 0.0f;
	for (const char Ch : Text)
	{
		Adv.push_back(RitMeasure(C, std::string(1, Ch), Size) + 1.0f);
		Tw += Adv.back();
	}
	const float Cw = W > 0.0f ? W : Tw + Pad * 2.0f;
	const float Half = H / 2.0f;
	uint32_t Top0 = RitFlapTop, Top1 = RitTwillB, Low0 = 0x070708, Low1 = RitFlapLow;
	switch (Tile)
	{
	case RitTile::Red: Top0 = 0xe4252b; Top1 = RitRed; Low0 = 0xb31016; Low1 = 0xc3131a; break;
	case RitTile::RedDim: Top0 = 0x8a1a20; Top1 = RitFinalDim; Low0 = 0x5e1015; Low1 = 0x6a1217; break;
	case RitTile::Chrome: Top0 = RitChromeDark; Top1 = RitChromeLight; Low0 = RitChromeLight; Low1 = RitChromeMid; break;
	case RitTile::Gloss: break;
	}
	C.FillRect({X, Y, Cw, Half}, Paint::Linear({0.0f, Y}, {0.0f, Y + Half}, RitC(Top0), RitC(Top1)));
	C.FillRect({X, Y + Half, Cw, H - Half}, Paint::Linear({0.0f, Y + Half}, {0.0f, Y + H}, RitC(Low0), RitC(Low1)));
	// The tiles: a seam at each character's edge across the run.
	const int Shown = std::min(std::max(Settled, 0), static_cast<int>(Text.size()));
	float Sx = X + Pad;
	for (size_t I = 0; I < Adv.size(); ++I)
	{
		if (static_cast<int>(I) >= Shown && Text[I] != ' ')
		{
			// A flap still turning: a blank panel, lit on its upper half (text never scales).
			C.FillRect({Sx, Y + 1.0f, Adv[I] - 1.0f, Half - 1.0f}, RitC(RitWhite, 0.08f));
		}
		Sx += Adv[I];
		if (I + 1 < Adv.size())
		{
			C.FillRect({Sx - 1.0f, Y, 1.0f, H}, RitC(0x000000, 0.55f));
		}
	}
	if (Shown > 0)
	{
		RitText(C, Text.substr(0, static_cast<size_t>(Shown)), X + Pad + 0.5f, Y + Half, Size, Ink, RitFace::Cond, Align::Left, Baseline::Middle);
	}
	C.FillRect({X, Y + Half - 0.5f, Cw, 1.0f}, RitC(0x000000, 0.45f));
	return Cw;
}

// ------------------------------------------------------------------ the screen state

std::map<const RiverLine*, RitUi>& RitStates()
{
	static std::map<const RiverLine*, RitUi> ByLaptop;
	return ByLaptop;
}

RitUi& RitOf(const RiverLine* RL)
{
	return RitStates()[RL];
}

void ForgetRit(const RiverLine* RL)
{
	RitStates().erase(RL);
}

// ------------------------------------------------------------------ G2: the loops' model and its marks

RitPlay RitPlayOf(const RiverLine* RL, const Session& S)
{
	RitUi& U = RitOf(RL);
	if (U.HasOverride)
	{
		return U.Override;
	}
	const double World = S.WorldMinutes();
	if (U.PlayAt == S.Now && U.PlayWorld == World)
	{
		return U.PlayCache; // read once a frame (§10: SitePlayView <= 0.2 ms)
	}
	U.PlayCache = RitPlayFrom(S, World);
	U.PlayAt = S.Now;
	U.PlayWorld = World;
	return U.PlayCache;
}

RitPlay RitPlayFrom(const Session& S, double World)
{
	RitPlay P;
	P.Member = rlsites_detail::SitesAccount(S, RitSite) != nullptr;
	// The clause's flag the pages already read from the sponsor book (as RegisterBlock's warning does): the Board's banner.
	P.Clause = sponsor::WouldStrike(S.Life.Sponsors, static_cast<int>(RitSite), false, false);
	P.Today = site::play::PlayDay(World);
	// Everything else is core-siteplay's View (Session::SitePlayView: the book, World::League, the Governor's ring), copied
	// across; the pages rank, zone and score nothing themselves.
	const site::play::View V = S.SitePlayView(RitSite);
	const site::play::RitView& R = V.Rt;
	P.On = R.On;
	P.Started = R.Started;
	// On leave is core's close rule (SitePlayRit.cpp's Lg.Leave): the clause strikes RunItTwice AND no day scored or played
	// this month. RitView::OnLeave is the clause alone, so a member who has played here under the deal (and can still be
	// relegated) is not told "no relegation" (review-ui-rit).
	P.Leave = R.OnLeave && R.Days <= 0;
	P.InLeague = R.League && !R.Table.empty();
	P.MonthKey = R.Month;
	P.Div = R.Div;
	P.Group = R.Group;
	P.Placement = R.Placement;
	P.Bounce = R.Bounce;
	P.Rank = R.Rank;
	P.Of = R.Of;
	P.Points = static_cast<float>(R.Points);
	P.ToPromotion = static_cast<float>(R.ToPromotion);
	P.ToSafety = static_cast<float>(R.ToSafety);
	P.Up = R.Up;
	P.Down = R.Down;
	P.Matchweek = R.Matchweek;
	P.Matchweeks = R.Matchweeks;
	P.Closes = R.DeadlineAt;
	P.Deadline = R.DeadlineDay;
	// This week's fixtures at the player's division (core names each and its weekday; the template is the network's).
	const net::Network& Net = net::Site(RitSite);
	const int Wd = net::Weekday(P.Today);
	for (const std::pair<std::string, int>& M : R.Matchdays)
	{
		RitFixture F;
		F.Day = P.Today - Wd + M.second;
		F.Name = M.first;
		for (const net::EventTemplate& T : Net.Templates())
		{
			F.TemplateId = F.TemplateId.empty() && T.Name == M.first ? T.Id : F.TemplateId;
		}
		F.Mult = static_cast<float>(site::play::MatchdayMult(R.Div, F.TemplateId, F.Day));
		F.Mult = F.Mult > 1.0f ? F.Mult : 1.5f;
		P.Fixtures.push_back(F);
	}
	std::sort(P.Fixtures.begin(), P.Fixtures.end(), [](const RitFixture& A, const RitFixture& B) { return A.Day < B.Day; });
	if (R.DeadlineDay)
	{
		P.Fixtures.push_back({P.Today, std::string(), "Featured events", 1.25f, true});
	}
	for (const site::play::LeagueRowView& Row : R.Table)
	{
		RitLeagueRow Q;
		Q.Npc = Row.Hero ? -1 : Row.Npc;
		Q.Name = Row.Name;
		Q.Country = Row.Country;
		Q.Rank = Row.Rank;
		Q.Move = Row.Move;
		Q.Days = Row.Days;
		Q.Points = static_cast<float>(Row.Points);
		// The best-8 meter: core's counting days' points (highest first); while it carries none, the count as filled cells.
		bool Valued = false;
		for (size_t I = 0; I < 8; ++I)
		{
			Q.Best[I] = static_cast<float>(Row.Best[I]);
			Valued = Valued || Row.Best[I] > 0.0;
		}
		for (int I = 0; I < 8 && I < Row.Days && !Valued; ++I)
		{
			Q.Best[static_cast<size_t>(I)] = 1.0f;
		}
		// Core's form is newest first (0 no result, 1 blank, 2 cash, 3 final table); the page's oldest first, one lower.
		for (size_t K = 0; K < 5; ++K)
		{
			Q.Form[4 - K] = static_cast<int8_t>(Row.Form[K] - 1);
		}
		Q.Path = Row.Path;
		Q.Zone = Row.Zone;
		Q.Team = Row.Team;
		Q.Callout = Row.Callout;
		Q.CanCall = Row.CanCall && R.CanCallOut;
		P.Table.push_back(Q);
	}
	for (const site::play::LeagueHist& H : R.Hist)
	{
		P.History.push_back({H.Month, H.Div, H.Rank, H.Of, H.Move});
	}
	std::sort(P.History.begin(), P.History.end(), [](const RitPlayHist& A, const RitPlayHist& B) { return A.Month > B.Month; });
	// The Callout: core's; its week and end are core's Matchweek (SitePlayRit.cpp's RitMatchweekKey: days 1-7, 8-14, 15-21,
	// 22-28 and 29 to the month's end, each closing at 6 AM), not the Monday play week (review-ui-rit: a Monday key rang the
	// incoming alert twice inside one Matchweek, and "Ends in" counted to a Monday the duel doesn't end on).
	P.Callout.On = R.CalloutOn;
	P.Callout.Npc = R.CalloutNpc;
	P.Callout.Name = R.CalloutName;
	P.Callout.ByThem = R.CalloutByThem;
	P.Callout.Mine = static_cast<float>(R.CalloutHero);
	P.Callout.Theirs = static_cast<float>(R.CalloutThem);
	{
		int Cy = 0, Cm = 0, Cd = 1;
		world::CivilDate(P.Today, Cy, Cm, Cd);
		const int Mw = (Cd - 1) / 7;
		const int First = world::MonthStart(P.Today);
		P.Callout.Week = site::play::PlayMonth(P.Today) * 8 + Mw;
		P.Callout.Seen = R.IncomingSeen == P.Callout.Week; // G2 fix L6: the save's own mark
		P.Callout.Ends = site::play::PlayDayStart(std::min(First + 7 * (Mw + 1), world::MonthStart(First + 32)));
	}
	const world::World& W = S.Living();
	if (const world::Npc* N = W.Get(R.CalloutNpc))
	{
		P.Callout.Country = N->Country;
	}
	for (const site::play::H2H& H : R.Heads)
	{
		P.H2H.push_back({H.Name, H.W, H.L, H.TheyKo, H.YouKo});
		P.Callout.W += H.W;
		P.Callout.L += H.L;
	}
	// The Watchlist: the stars, and the bells core rang (still fresh: 15 world minutes, registration open).
	P.Watch = R.Stars;
	for (const site::play::WatchView& Wv : R.Watch)
	{
		net::EventInstance E;
		if (!Wv.Bell || Wv.BellAt < World - 15.0 || !Net.FindInstance(Wv.EventId, E))
		{
			continue;
		}
		const net::LiveState L = Net.Live(E, World);
		const site::show::OverlayView Ov = site::show::OverlayOf(Net, E, World);
		if (L.St == net::Status::LateReg || L.St == net::Status::Registering)
		{
			P.Bells.push_back({Wv.EventId, Wv.Name, Ov.Cents, E.Start + E.LateReg, Wv.BellAt});
		}
	}
	// Team Scalps, "you're the bounty".
	for (const site::play::ScalpView& Sc : R.Scalps)
	{
		if (Sc.Npc >= 0 || !Sc.Name.empty())
		{
			P.Scalps.Six.push_back({Sc.Npc, Sc.Name, Sc.Taken});
		}
	}
	P.Scalps.Taken = R.Taken;
	P.Scalps.Week = R.WeekCount;
	P.Scalps.WeekCap = R.WeekCap;
	P.Scalps.Lifetime = R.Lifetime;
	P.Scalps.FullSet = R.FullSet;
	P.Scalps.Bounty = R.Bounty;
	// The Statement (the Governor's ring) and its dated lines; the Guest List.
	P.Statement.Line = V.Gov.Line;
	P.Statement.Rake30 = V.Gov.Rake30;
	P.Statement.Promo30 = V.Gov.Promo30;
	P.Statement.Share = V.Gov.Share;
	P.Statement.Ceiling = V.Gov.Ceiling;
	P.Statement.Day0 = V.Gov.FirstDay;
	for (size_t I = 0; I < V.Gov.RakeByDay.size() && I < P.Statement.RakeByDay.size(); ++I)
	{
		P.Statement.RakeByDay[I] = static_cast<int32_t>(V.Gov.RakeByDay[I]);
	}
	for (const site::play::HoldView& H : V.Gov.Holds)
	{
		P.Statement.Holds.push_back({H.What, H.Cents, H.Covered, H.RakeLeft, H.Day, H.SparksOwed});
	}
	for (const site::play::StatementRow& Row : R.Statement)
	{
		RitStatementLine Ln;
		Ln.Label = Row.Label;
		Ln.Month = Row.Month;
		Ln.Cents = Row.Cents;
		P.Statement.Lines.push_back(Ln);
	}
	// The Luck Ledger, credentials, the Briefing.
	P.Luck.AllIns = R.Luck.AllIns;
	P.Luck.Won = R.Luck.Won;
	P.Luck.Expected = R.Luck.Expected;
	P.Luck.N = R.Luck.Of;
	P.Luck.Wins = R.Luck.Wins;
	P.Luck.Exp = R.Luck.Exp;
	P.Luck.SecondWon = R.Luck.SecondWon;
	P.Luck.SecondOf = R.Luck.SecondOf;
	P.Luck.Seconds = R.Luck.SecondOf > 0;
	for (const site::play::CredView& Cr : R.Creds)
	{
		RitCred Q;
		Q.Id = Cr.Id;
		Q.Name = Cr.Name;
		Q.Earned = Cr.Got;
		Q.Day = Cr.Day;
		Q.Event = Cr.Event;
		P.Creds.push_back(Q);
	}
	// The projection's inputs, the Desk's and the Settlement's scores, the pyramid, the scalp ladder (patch 2's fields).
	for (const std::pair<int, double>& D : R.HeroDays)
	{
		P.HeroDays.push_back({D.first, static_cast<float>(D.second)});
	}
	P.Counting = R.Counting;
	for (const std::pair<std::string, double>& Sc : R.Scored)
	{
		P.Scored.push_back({Sc.first, static_cast<float>(Sc.second)});
	}
	P.Scalps.Ladder = R.ScalpLadder;
	// Core's line leads with its own "BRIEFING" (the band's tag says it): what follows it is the news; nothing, no band.
	std::string Brief = R.Briefing;
	for (const char* Lead : {"BRIEFING", " \xC2\xB7 "})
	{
		Brief = Brief.compare(0, std::char_traits<char>::length(Lead), Lead) == 0 ? Brief.substr(std::char_traits<char>::length(Lead)) : Brief;
	}
	P.Briefing.Line = Brief;
	P.Briefing.On = !Brief.empty();
	P.Briefing.Day = P.Today;
	// The divisions: their floors (core's rule); members and rewards when core counts them.
	for (int D = 0; D < RitDivisions; ++D)
	{
		RitDivision& Dv = P.Divisions[static_cast<size_t>(D)];
		Dv.Floor = site::play::DivisionFloor(D);
		Dv.Members = R.Members[static_cast<size_t>(D)];
		Dv.Reward = R.Rewards[static_cast<size_t>(D)];
	}
	for (const std::pair<std::string, int>& M : R.Matchdays)
	{
		P.Divisions[static_cast<size_t>(std::min(std::max<int>(R.Div, 0), RitDivisions - 1))].Matchdays.push_back(M.first);
	}
	// League Night: the month the book closed last (its own record of the player's place), the first open after it.
	if (!P.History.empty() && P.History.front().Month == world::MonthKeyOf(net::DayOf(World - 6.0 * 60.0)) - 1)
	{
		const RitPlayHist& H = P.History.front();
		P.Night.On = true;
		P.Night.Month = H.Month;
		P.Night.From = H.Div;
		P.Night.To = static_cast<int8_t>(std::min(std::max(H.Div + H.Move, 0), RitDivisions - 1));
		P.Night.Rank = H.Rank;
		P.Night.Of = H.Of;
		P.Night.Seen = R.NightSeen == H.Month; // G2 fix L6: the save's own mark, so a relaunch doesn't play it again
	}
	return P;
}

void RitPlayOverride(const RiverLine* RL, const RitPlay* P)
{
	RitUi& U = RitOf(RL);
	U.HasOverride = P != nullptr;
	U.Override = P ? *P : RitPlay();
}

const char* RitDivName(int Div)
{
	static const char* const Names[RitDivisions] = {"Open", "Iron", "Steel", "Gold", "High Bridge"};
	return Names[std::min(std::max(Div, 0), RitDivisions - 1)];
}

uint32_t RitDivMetal(int Div)
{
	switch (Div)
	{
	case 1: return RitPanel2;   // Iron: carbon
	case 2: return RitChrome;   // Steel: chrome
	case 3: return RitGoldMid;  // Gold: Status Gold
	case 4: return RitRed;      // High Bridge: red lacquer
	default: return RitChromeDark; // Open: a chrome outline
	}
}

void RitCrest(Canvas& C, float Cx, float Cy, float H, int Div, int Stars, float Split)
{
	const int D = std::min(std::max(Div, 0), RitDivisions - 1);
	const float W = H * 0.82f;
	const float T = Cy - H / 2.0f;
	const float Sh = Cy + H * 0.12f;
	const float B = Cy + H / 2.0f;
	const float Gap = W * 0.34f * std::min(std::max(Split, 0.0f), 1.0f);
	// The two halves along the chevron (drawn apart while League Night re-forges it; together they are the whole shield).
	const std::vector<Vec2> Left = {{Cx - W / 2.0f - Gap, T}, {Cx - Gap, T}, {Cx - Gap, B}, {Cx - W / 2.0f - Gap, Sh}};
	const std::vector<Vec2> Right = {{Cx + Gap, T}, {Cx + W / 2.0f + Gap, T}, {Cx + W / 2.0f + Gap, Sh}, {Cx + Gap, B}};
	const std::vector<Vec2> Whole = {{Cx - W / 2.0f, T}, {Cx + W / 2.0f, T}, {Cx + W / 2.0f, Sh}, {Cx, B}, {Cx - W / 2.0f, Sh}};
	const Paint Fill = D == 2 ? Paint::Linear({Cx - W / 2.0f, T}, {Cx + W / 2.0f, B}, RitC(RitChromeDark), RitC(RitChromeLight))
		: D == 3			  ? Paint::Linear({Cx - W / 2.0f, T}, {Cx + W / 2.0f, B}, RitC(RitGoldDark), RitC(RitGoldLight))
		: D == 4			  ? Paint::Linear({0.0f, T}, {0.0f, B}, RitC(0xe3262c), RitC(RitDeep))
		: D == 1			  ? Paint(RitC(RitPanel2))
							  : Paint(RitC(RitOnyx));
	const uint32_t Mark = D == 2 || D == 3 ? RitOnyx : D == 0 ? RitChrome : RitWhite;
	if (Gap > 0.0f)
	{
		C.FillPolygon(Left, Fill);
		C.FillPolygon(Right, Fill);
		C.StrokePolyline(Left, true, RitC(RitChrome, 0.7f), 1.0f);
		C.StrokePolyline(Right, true, RitC(RitChrome, 0.7f), 1.0f);
	}
	else
	{
		C.FillPolygon(Whole, Fill);
		C.StrokePolyline(Whole, true, RitC(D == 0 ? RitChrome : D == 1 ? RitChromeDark : RitChrome, D == 0 ? 1.0f : 0.6f), D == 0 ? 1.4f : 1.0f);
	}
	if (D == 4)
	{
		// High Bridge: an arch over two piers.
		std::vector<Vec2> Arch;
		const float Ar = W * 0.26f;
		const float Ay = Cy + H * 0.06f;
		for (int I = 0; I <= 10; ++I)
		{
			const float A = Pi + Pi * static_cast<float>(I) / 10.0f;
			Arch.push_back({Cx + Ar * std::cos(A), Ay + Ar * std::sin(A)});
		}
		C.StrokePolyline(Arch, false, RitC(Mark), std::max(1.2f, H * 0.06f));
		C.FillRect({Cx - W * 0.34f, T + H * 0.16f, W * 0.68f, std::max(1.2f, H * 0.06f)}, RitC(Mark));
	}
	else
	{
		// 1-4 bars (Open one, Gold four).
		const int Bars = D + 1;
		const float Bw = W * 0.56f;
		const float Bh = std::max(1.0f, H * 0.065f);
		for (int I = 0; I < Bars; ++I)
		{
			C.FillRect({Cx - Bw / 2.0f, T + H * 0.16f + static_cast<float>(I) * Bh * 2.0f, Bw, Bh}, RitC(Mark, Gap > 0.0f ? 0.4f : 1.0f));
		}
	}
	// The group wins: up to 5 stars over the shield, Hot White.
	const int N = std::min(std::max(Stars, 0), 5);
	for (int I = 0; I < N; ++I)
	{
		const float Sx = Cx + (static_cast<float>(I) - static_cast<float>(N - 1) / 2.0f) * H * 0.2f;
		RitStar(C, Sx, T - H * 0.12f, H * 0.075f, true);
	}
}

void RitCredPlate(Canvas& C, const Rect& R, const std::string& Name, const std::string& Line, bool Earned)
{
	if (Earned)
	{
		RitCutFill(C, R, 6.0f, Paint::Linear({R.X, R.Y}, {R.X + R.W, R.Y + R.H}, RitC(RitChromeMid), RitC(RitChromeLight)));
		C.FillRect({R.X, R.Y + R.H - 1.0f, R.W, 1.0f}, RitC(RitChromeDark));
		// The engraving: Onyx type on the chrome (8.5:1 or more on its lighter ramp).
		RitText(C, RitUpper(Name), R.X + 10.0f, R.Y + 16.0f, 13.0f, RitC(RitOnyx), RitFace::Cond, Align::Left, Baseline::Alphabetic, R.W - 20.0f);
		RitText(C, Line, R.X + 10.0f, R.Y + 31.0f, 11.0f, RitC(RitOnyx), RitFace::Mono, Align::Left, Baseline::Alphabetic, R.W - 20.0f);
		return;
	}
	RitCutStroke(C, R, 6.0f, RitC(RitHair), 1.0f);
	RitText(C, RitUpper(Name), R.X + 10.0f, R.Y + 16.0f, 13.0f, RitC(RitMuted), RitFace::Cond, Align::Left, Baseline::Alphabetic, R.W - 20.0f);
	RitText(C, Line, R.X + 10.0f, R.Y + 31.0f, 11.0f, RitC(RitMuted), RitFace::Mono, Align::Left, Baseline::Alphabetic, R.W - 20.0f);
}

void RitCrosshair(Canvas& C, float Cx, float Cy, float R)
{
	C.StrokeArc(Cx, Cy, R, 0.0f, 2.0f * Pi, RitC(RitRed), 1.6f);
	C.StrokeArc(Cx, Cy, R * 0.45f, 0.0f, 2.0f * Pi, RitC(RitRed), 1.2f);
	C.FillRect({Cx - R - 4.0f, Cy - 0.8f, R * 0.7f, 1.6f}, RitC(RitRed));
	C.FillRect({Cx + R * 0.3f + 4.0f, Cy - 0.8f, R * 0.7f, 1.6f}, RitC(RitRed));
	C.FillRect({Cx - 0.8f, Cy - R - 4.0f, 1.6f, R * 0.7f}, RitC(RitRed));
	C.FillRect({Cx - 0.8f, Cy + R * 0.3f + 4.0f, 1.6f, R * 0.7f}, RitC(RitRed));
}

void RitEye(Canvas& C, float Cx, float Cy, float W, const Color& Col)
{
	std::vector<Vec2> Lid;
	for (int I = 0; I <= 12; ++I)
	{
		const float T = static_cast<float>(I) / 12.0f;
		Lid.push_back({Cx - W / 2.0f + W * T, Cy - W * 0.32f * std::sin(Pi * T)});
	}
	for (int I = 12; I >= 0; --I)
	{
		const float T = static_cast<float>(I) / 12.0f;
		Lid.push_back({Cx - W / 2.0f + W * T, Cy + W * 0.32f * std::sin(Pi * T)});
	}
	C.StrokePolyline(Lid, true, Col, 1.2f);
	C.FillCircle(Cx, Cy, W * 0.16f, Col);
}

std::string RitPlace(int N)
{
	return RitUpper(Ordinal(N));
}

std::string RitPts(double V)
{
	return Fixed(V, 1);
}

const RitLeagueRow* RitHeroRow(const RitPlay& P)
{
	for (const RitLeagueRow& R : P.Table)
	{
		if (R.Npc < 0)
		{
			return &R;
		}
	}
	return nullptr;
}

int RitRankIf(const RitPlay& P, double Score)
{
	const RitLeagueRow* Me = RitHeroRow(P);
	if (!Me || (P.HeroDays.empty() && Me->Days > 0))
	{
		return 0; // not known: core hasn't said which days count (the tab hides the projection rather than guess)
	}
	// The player's best 8 with today's daily best raised to Score (core's numbers; Bounce Back's bonus kept as it is).
	std::vector<float> Days;
	float Counted = 0.0f;
	bool Today = false;
	for (const std::pair<int, float>& D : P.HeroDays)
	{
		Days.push_back(D.second);
		if (D.first == P.Today)
		{
			Days.back() = std::max(D.second, static_cast<float>(Score));
			Today = true;
		}
	}
	if (!Today)
	{
		Days.push_back(static_cast<float>(Score));
	}
	std::vector<float> Was;
	for (const std::pair<int, float>& D : P.HeroDays)
	{
		Was.push_back(D.second);
	}
	std::sort(Days.begin(), Days.end(), [](float A, float B) { return A > B; });
	std::sort(Was.begin(), Was.end(), [](float A, float B) { return A > B; });
	float Then = 0.0f;
	for (size_t I = 0; I < Was.size() && I < 8; ++I)
	{
		Then += Was[I];
	}
	for (size_t I = 0; I < Days.size() && I < 8; ++I)
	{
		Counted += Days[I];
	}
	const float Mine = Counted + std::max(0.0f, Me->Points - Then);
	int Rank = 1;
	for (const RitLeagueRow& R : P.Table)
	{
		Rank += R.Npc >= 0 && R.Points > Mine + 0.0001f ? 1 : 0;
	}
	return Rank;
}

RitSecond RitSecondOf(const site::show::SecondRecord& R)
{
	RitSecond B;
	B.EventId = R.EventId;
	B.HandId = R.HandId;
	B.Hero = R.Hero;
	B.Villain = R.Villain;
	B.Ran.assign(R.Ran.begin(), R.Ran.end());
	B.Twice = R.Board.Cards;
	B.Verdict = R.Board.Verdict;
	B.First = R.Board.First;
	B.Equity = R.Equity;
	B.At = R.At;
	return B;
}

// ------------------------------------------------------------------ the copy

std::vector<std::string> RitInventory()
{
	return {RitNoTables, RitBoardOpen, RitNoSpins, RitRakeBack, RitAttitude, RitBoardFoot, RitBoardEmpty, RitSitePays, RitNoOverlay, RitNoOverlayYet, RitNoGtd, RitSettles, RitNoRecord,
		RitPickRow, RitPaysGrow, RitNoMajor, RitNoChipCounts, RitLiveOpen, RitNoFeed, RitDeskEmpty, RitDeskTape, RitDeskNoTape, RitSeriesNone, RitNewsEmpty,
		RitClubTurn, RitProvisional, RitTopTier, RitNoLetters, RitStaysHere, RitDepositHow, RitWithdrawHow, RitOnlyBank, RitNothingComing, RitNoLines, RitSettled,
		RitRunTwice, RitJustToSee, RitForShow, RitNoSeconds, RitNoUpNext, RitMemberNote, RitNoClub, RitTicketsRule, RitTicketsNone, RitClubDayNone, RitClubPaid,
		RitNoClubMonths, RitSecondHow, RitNoNotables, RitNoWinner,
		// G2.
		RitLeagueNone, RitLeagueNoGroup, RitLeagueRule, RitLeagueOther, RitDivNoCount, RitLeagueLeave, RitLeagueBounce, RitNoFixtures, RitNightSkip,
		RitCalloutNone, RitCalloutNoGroup, RitCalloutLeave, RitCalloutClause, RitCalloutRule, RitCallDrawerWhat, RitCallOnce, RitWatchNone, RitWatchQuiet, RitMineNone,
		RitClauseBanner, RitBounty, RitScalpsRule, RitScalpsNone, RitScalpsLadder, RitStatementNone, RitLuckNone, RitLuckEven, RitLuckHow, RitCredsNone,
		RitHistNone, RitH2HNone, RitSideNoLeague, RitSideNotCounting, RitSideFlip, RitTodaysBest, RitNotToday, RitNoLeagueYet,
		// The sentences G2's pages fill from core's figures (the lint fills the samples).
		"RunItTwice gave back $53.48 of your $212.40 rake in the last 30 days.", "You are running 7.6% below expectation.",
		"You'd have won 9 of 22 second runs.", "Your group plays in Steel.", "Steel's Matchday: this event counts x1.5.",
		"Steel's Matchday tonight is The Sweat (x1.5).", "K.Lindqvist is registered.", RitNoLuckHere, RitRakeNone};
}

std::vector<std::string> RitLabels()
{
	return {"BOARD", "SUNDAY LIVE", "LEAGUE", "RANKINGS", "THE DESK", "SERIES", "NEWSDESK", "CLUB", "CASHIER", "THE BOARD", "THE LEAGUE", "THE CLUB", "SETTLEMENT",
		"MEMBER FILE", "SEATED", "ACTION", "LIVE", "OPEN", "MEMBER", "SEASON", "TONIGHT GTD", "OVERLAY WATCH", "RUNNING", "YOUR RAKE", "FILTER", "SORT", "TIME",
		"EVENT", "BUY-IN", "GTD", "FIELD", "OVERLAY", "WIN PTS", "STATUS", "SCHEDULED", "BOARDING", "FINAL CALL", "DEPARTED", "ARRIVED", "LOCKED", "PROSPECTUS",
		"PROJECTED OVERLAY", "PAYS", "YOUR RECORD", "OVERVIEW", "STRUCTURE", "PAYOUTS", "ENTRANTS", "REGISTRATION CLOSES", "FEATURED", "TEAM", "REGISTERING",
		"LATE REG", "FINAL TABLE", "COMPLETE", "PROJ SHORT", "PROJ OVER", "STORY", "FEED", "LEDGER", "TAPE", "EVENTS", "ROI", "ITM", "FT", "ABI", "NET", "RAKE",
		"LAST 60 DAYS", "ALL", "RESULTS", "PROGRAMME", "MAIN EVENT", "RESULT", "BIG WIN", "SCHEDULE", "RECORD", "PEOPLE", "SECOND RUN CLUB", "WELCOME TICKETS",
		"TICKET", "ACCRUED", "PROVISIONAL", "INNER CIRCLE", "HIGH TABLE", "REGULAR", "DEPOSIT", "WITHDRAW", "STATEMENT", "IN TRANSIT", "RUNITTWICE \xC2\xB7 SETTLEMENT",
		"NET", "IF IT RAN TWICE", "WON BOTH", "SPLIT IT", "LOST BOTH", "CASHED", "WINNER", "UP NEXT", "BACK TO THE BOARD", "ALL IN", "BETS IN", "BB", "CHIPS",
		"DEALER", "MEMBER SINCE", "SIGNATURE", "SUMMARY", "CLUB HISTORY", "BIGGEST CASH", "WINS", "FINAL TABLES", "SUNDAY SESSION OPEN", "STARTS IN",
		"LATE REGISTRATION CLOSES IN", "REGISTRATION OPENS IN", "PLAYERS LEFT", "FINISHED AT", "BETS IN BIG BLINDS", "BETS IN CHIPS", "NOTABLE ENTRIES", "THE RESULT", "CUT A VIDEO",
		"THE TIERS", "CLUB DAY", "FROM RUNITTWICE", "ACCOUNTS", "METHOD", "FEE", "SECOND BOARDS", "RAN", "WON", "SPLIT", "LOST", "MAIN EVENT", "WIN PTS",
		"RUNITTWICE \xC2\xB7 CLUB DAY", "NO TIER YET", "THE RATE", "RAKE PAID", "OUT", "SEASON EVENTS",
		// G2.
		"OPEN", "IRON", "STEEL", "GOLD", "HIGH BRIDGE", "GROUP", "POS", "PLAYER", "PATH", "DAYS", "BEST 8", "POINTS", "FORM", "CALL", "VS", "YOU",
		"YOUR SEASON", "TO GO UP", "TO STAY UP", "THE CALLOUT", "RECORD", "LEADING", "TRAILING", "ALL SQUARE", "CALLED YOU OUT", "YOU CALLED THEM OUT", "THE WATCHLIST",
		"THIS WEEK", "MD", "DEADLINE", "DEADLINE DAY", "MATCHWEEK", "BEST 8 DAYS COUNT", "TOP 3 UP", "BOTTOM 4 DOWN", "NOBODY GOES DOWN", "CLEAR", "PTS SHORT", "CLEARS AT THE NEXT 6 AM", "ON LEAVE",
		"PLACEMENT MONTH", "BOUNCE BACK", "LEAGUE NIGHT", "PROMOTED TO GOLD", "RELEGATED TO IRON", "STAYED IN STEEL", "CALL OUT", "CANCEL",
		"THEIR POINTS", "DAYS WITH A RESULT", "YOURS", "INCOMING CALLOUT", "THE LEAGUE", "BRIEFING", "MATCHDAY TONIGHT", "PTS TO PROMOTION",
		"IN THE PROMOTION PLACES", "TEAM SCALPS", "TAKEN", "OF 6 TAKEN", "A SCALP PAYS", "FULL SET", "YOU'RE THE BOUNTY", "THE STATEMENT",
		"GIVEN BACK", "THE CEILING", "WAITING", "LUCK LEDGER", "ALL-INS", "EXPECTED", "CREDENTIALS", "NOT YET", "LEAGUE HISTORY", "CALLOUTS",
		"HEAD TO HEAD", "THIS EVENT, PROJECTED", "PROMOTION ZONE", "PROMOTED", "AT TABLE", "LIVE", "MINE", "PYRAMID", "MEMBERS", "SCORES FROM",
		"MATCHDAYS", "PROMOTION INTO IT", "LEAGUE POINTS", "LUCK, THIS EVENT", "WON", "GAP", "STATEMENT", "CLUB DAY", "UNDER YOUR TEAM RIVERLINE DEAL",
		"BY THE EVENT'S BUY-IN", "FEATURED EVENTS", "EARNED", "NOT YET", "PTS FOR A WIN", "UNDER THE FLOOR", "THE LEAGUE \xC2\xB7 MEMBERS BY DIVISION",
		"RAKE BY DAY \xC2\xB7 30 DAYS", "NO GROUP YET", "ON LEAVE \xC2\xB7 YOUR DIVISION IS KEPT", "REGISTERED TONIGHT", "ENDS IN", "EXPECTED WINS",
		"ACTUAL WINS", "TEAM SCALP", "TODAY'S BEST", "NONE \xC2\xB7 DIDN'T SCORE", "TOP 3 GO UP ON", "DAYS LEFT", "YOUR BEST 8 DAYS COUNT",
		"IN THE PROMOTION PLACES (TOP 3)", "PTS SHORT OF PROMOTION", "CASH", "FINAL TABLE", "WIN", "SCALP", "TEAM RUNITTWICE", "YOUR CALLOUT",
		"H2H", "BUSTS", "THE STATEMENT \xC2\xB7 LAST 30 DAYS", "THE CEILING", "CLEARS AFTER", "MORE RAKE", "PAID RUNITTWICE EVENTS",
		"YOU'D HAVE WON", "SECOND RUNS", "PLACES", "THIS MATCHWEEK", "IN THE GROUP", "THE CALLOUT", "SEASON", "ON YOUR WATCHLIST",
		// High Bridge (nobody goes up: its top places win a Sunday 100K ticket; review-ui-rit).
		"TOP 3 WIN A SUNDAY 100K TICKET", "TO THE TOP 3", "100K TICKET", "TOP 3 WIN A SUNDAY 100K TICKET ON"};
}
} // namespace rlrit_detail
} // namespace ui
} // namespace ss
