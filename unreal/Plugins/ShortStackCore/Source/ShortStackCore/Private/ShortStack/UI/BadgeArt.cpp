#include "ShortStack/UI/BadgeArt.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/UI/SponsorArt.h"
#include "ShortStack/UI/Ui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>

// The badges' art (next_segment_spec §5.5). core-badges.
//
// Every glyph is drawn in a Size box centred on (Cx, Cy) from a unit square (-0.5..0.5). What a size can carry decides
// the detail: under 16 the silhouette and its one mark; from 16 the count medal's number, a Main Event's crown or halo;
// from 20 the patch's stitching; from 24 a Player of the Year's numerals and the finer cuts. The atlas draws the
// 16-unit art large, so a cell sampled down at a seat plate reads like the vector glyph does.

namespace ss
{
namespace ui
{
namespace badgeart
{
namespace badgeart_detail
{
constexpr float Pi2 = 6.28318530718f;

float Fl(int V)
{
	return static_cast<float>(V);
}

Color Fade(const Color& C, float A)
{
	return Color{C.R, C.G, C.B, C.A * A};
}

Color Shade(uint32_t Rgb, float By)
{
	// Toward black (By < 0) or white (By > 0).
	const Color C = Hex(Rgb);
	return By < 0.0f ? Mix(C, Hex(0x000000), -By) : Mix(C, Hex(0xffffff), By);
}

/** Light enough that a mark on it wants dark ink. */
bool Light(uint32_t Rgb)
{
	const float R = static_cast<float>((Rgb >> 16) & 0xffu) / 255.0f;
	const float G = static_cast<float>((Rgb >> 8) & 0xffu) / 255.0f;
	const float B = static_cast<float>(Rgb & 0xffu) / 255.0f;
	return 0.299f * R + 0.587f * G + 0.114f * B > 0.6f;
}

/** A glyph's pen: unit coordinates (-0.5..0.5) around the centre, scaled to its box. */
struct Pen
{
	Canvas& C;
	float Cx;
	float Cy;
	float S;
	Vec2 P(float X, float Y) const { return {Cx + X * S, Cy + Y * S}; }
	float L(float V) const { return V * S; }
};

std::vector<Vec2> Star(const Pen& P, float X, float Y, float Outer, float Inner, int Points, float Turn = 0.0f)
{
	std::vector<Vec2> Out;
	for (int K = 0; K < Points * 2; ++K)
	{
		const float A = -Pi2 * 0.25f + Turn + Pi2 * Fl(K) / Fl(Points * 2);
		const float R = K % 2 == 0 ? Outer : Inner;
		Out.push_back(P.P(X + R * std::cos(A), Y + R * std::sin(A)));
	}
	return Out;
}

/** A number on a small disc at the glyph's lower right: a count of two or more ("5+" from the atlas's last cell). */
void CountMedal(const Pen& P, const std::string& Text, float Detail)
{
	const Vec2 M = P.P(0.31f, 0.31f);
	const float R = P.L(Detail >= 16.0f ? 0.19f : 0.16f);
	P.C.FillCircle(M.X, M.Y, R + P.L(0.035f), Paint(Hex(0x05080e)));
	P.C.FillCircle(M.X, M.Y, R, Paint::Linear({M.X, M.Y - R}, {M.X, M.Y + R}, Hex(0xfff3c9), Hex(0xe0b04a)));
	// The number at every size: at 12 it's a speck of ink, but two and five still read differently from a seat away.
	const float Size = R * (Text.size() > 1 ? 1.2f : 1.5f);
	P.C.Text(Text, M.X, M.Y + Size * 0.36f, Ts(Size, 900, Hex(0x1a1206), Align::Center));
}

std::string CountText(int Count, int Cap)
{
	return Count > Cap ? std::to_string(Cap) + "+" : std::to_string(Count);
}

// ---- the seven silhouettes

void Medallion(const Pen& P, uint32_t Rim, uint32_t Field, float Detail)
{
	const Vec2 O = P.P(0.0f, 0.0f);
	P.C.FillCircle(O.X, O.Y + P.L(0.03f), P.L(0.49f), Paint(Hex(0x000000, 0.35f)));
	P.C.FillCircle(O.X, O.Y, P.L(0.48f), Paint::Linear(P.P(0.0f, -0.48f), P.P(0.0f, 0.48f), Shade(Rim, 0.25f), Shade(Rim, -0.3f)));
	P.C.FillCircle(O.X, O.Y, P.L(0.39f), Paint::Radial(P.P(-0.1f, -0.14f), 0.0f, O, P.L(0.39f), Shade(Field, 0.12f), -1.0f, Color(), Hex(Field)));
	if (Detail >= 24.0f)
	{
		P.C.StrokeEllipse(O.X, O.Y, P.L(0.345f), P.L(0.345f), Fade(Hex(Rim), 0.45f), P.L(0.012f));
	}
}

void Laurel(const Pen& P, uint32_t Col, float Detail)
{
	// Two branches up the sides from a knot at the foot, leaves in pairs along each, open at the top.
	const Color Leaf = Hex(Col);
	const float R = 0.25f;
	const float Oy = 0.03f;
	const int Pairs = Detail >= 24.0f ? 4 : Detail >= 16.0f ? 3 : 2;
	const float Lx = Detail >= 16.0f ? 0.08f : 0.1f;
	const float Ly = Detail >= 16.0f ? 0.033f : 0.05f;
	for (int Side = -1; Side <= 1; Side += 2)
	{
		const float S = static_cast<float>(Side);
		const float From = Pi2 * 0.25f - S * Pi2 * 0.04f; // by the foot
		const float To = Pi2 * 0.25f - S * Pi2 * 0.36f;   // up the side, short of the top
		P.C.StrokeArc(P.Cx, P.Cy + P.L(Oy), P.L(R), std::min(From, To), std::max(From, To), Leaf, P.L(Detail >= 16.0f ? 0.03f : 0.05f), true);
		const float Travel = To > From ? Pi2 * 0.25f : -Pi2 * 0.25f;
		for (int K = 0; K < Pairs; ++K)
		{
			const float A = From + (To - From) * (Fl(K) + 0.55f) / Fl(Pairs);
			for (int Out = -1; Out <= 1; Out += 2)
			{
				const float D = 0.05f * static_cast<float>(Out);
				const Vec2 At = {P.Cx + P.L(R + D) * std::cos(A), P.Cy + P.L(Oy) + P.L(R + D) * std::sin(A)};
				P.C.Save();
				P.C.Translate(At.X, At.Y);
				P.C.Rotate(A + Travel - 0.6f * static_cast<float>(Out) * (Travel > 0.0f ? 1.0f : -1.0f));
				P.C.FillEllipse(0.0f, 0.0f, P.L(Lx), P.L(Ly), Paint(Leaf));
				P.C.Restore();
			}
		}
	}
	// The knot at the foot.
	const Vec2 Foot = P.P(0.0f, Oy + R);
	P.C.FillCircle(Foot.X, Foot.Y, P.L(0.05f), Paint(Leaf));
}

void Hall(const Pen& P, float Detail)
{
	Medallion(P, tok::Gold, tok::Onyx, Detail);
	Laurel(P, tok::Gold, Detail);
}

void PlayerOfYear(const Pen& P, const badge::Badge& B, float Detail, bool Live)
{
	const uint32_t Rim = Live ? tok::LiveGold : tok::Teal;
	Medallion(P, Rim, tok::Midnight, Detail);
	const bool Numerals = Detail >= 24.0f && B.Year > 0;
	const float Sy = Numerals ? -0.07f : 0.0f;
	P.C.FillPolygon(Star(P, 0.0f, Sy, Numerals ? 0.2f : 0.25f, Numerals ? 0.085f : 0.105f, 5), Paint(Hex(Rim)));
	if (Live)
	{
		// The live title's mark: a ring of eight studs around the star (the online one has none).
		for (int K = 0; K < 8; ++K)
		{
			const float A = Pi2 * Fl(K) / 8.0f + Pi2 / 16.0f;
			const Vec2 D = P.P(0.315f * std::cos(A), 0.315f * std::sin(A));
			P.C.FillCircle(D.X, D.Y, P.L(Detail >= 16.0f ? 0.03f : 0.04f), Paint(Hex(Rim)));
		}
	}
	if (Numerals)
	{
		char Buf[8];
		std::snprintf(Buf, sizeof(Buf), "'%02d", ((B.Year % 100) + 100) % 100);
		P.C.Text(Buf, P.Cx, P.Cy + P.L(0.27f), Ts(P.L(0.17f), 900, Hex(Rim), Align::Center));
	}
}

void Bracelet(const Pen& P, const badge::Badge& B, float Detail)
{
	// The band seen from the front and above, the plaque on it in the enamel, a spade on the plaque.
	const uint32_t Plate = B.Color != 0 ? B.Color : 0x2a2a35u;
	const Vec2 O = P.P(0.0f, B.Main ? 0.08f : 0.03f);
	P.C.StrokeEllipse(O.X, O.Y, P.L(0.42f), P.L(0.22f), Hex(0x000000, 0.4f), P.L(0.13f));
	P.C.StrokeEllipse(O.X, O.Y, P.L(0.42f), P.L(0.22f), Hex(tok::Gold), P.L(0.085f));
	if (Detail >= 16.0f)
	{
		// The links: a darker tick every so often round the band.
		for (int K = 0; K < 10; ++K)
		{
			const float A = Pi2 * Fl(K) / 10.0f;
			const Vec2 L0 = {O.X + P.L(0.38f) * std::cos(A), O.Y + P.L(0.18f) * std::sin(A)};
			const Vec2 L1 = {O.X + P.L(0.46f) * std::cos(A), O.Y + P.L(0.26f) * std::sin(A)};
			P.C.StrokePolyline({L0, L1}, false, Shade(tok::Gold, -0.35f), P.L(0.02f));
		}
	}
	const Rect Pl{O.X - P.L(0.2f), O.Y + P.L(0.06f), P.L(0.4f), P.L(0.26f)};
	P.C.FillRoundRect({Pl.X - P.L(0.035f), Pl.Y - P.L(0.035f), Pl.W + P.L(0.07f), Pl.H + P.L(0.07f)}, P.L(0.07f), Paint(Hex(tok::Gold)));
	P.C.FillRoundRect(Pl, P.L(0.045f), Paint::Linear({Pl.X, Pl.Y}, {Pl.X, Pl.Y + Pl.H}, Shade(Plate, 0.2f), Shade(Plate, -0.35f)));
	// The spade.
	const float Sx = Pl.X + Pl.W * 0.5f;
	const float Sy = Pl.Y + Pl.H * 0.46f;
	const float U = P.L(Detail >= 16.0f ? 0.07f : 0.08f);
	P.C.FillPolygon({{Sx, Sy - U}, {Sx + U * 0.95f, Sy + U * 0.2f}, {Sx + U * 0.4f, Sy + U * 0.6f}, {Sx, Sy + U * 0.25f}, {Sx - U * 0.4f, Sy + U * 0.6f}, {Sx - U * 0.95f, Sy + U * 0.2f}},
		Paint(Hex(0xf4f7ff)));
	if (Detail >= 16.0f)
	{
		P.C.FillPolygon({{Sx, Sy + U * 0.3f}, {Sx + U * 0.32f, Sy + U * 1.0f}, {Sx - U * 0.32f, Sy + U * 1.0f}}, Paint(Hex(0xf4f7ff)));
	}
	if (B.Main)
	{
		// A Main Event's: the crown over the band (three points under 16, five from there).
		const float Cy = O.Y - P.L(0.3f);
		const float W2 = P.L(0.2f);
		P.C.FillPolygon({{O.X - W2, Cy + P.L(0.1f)}, {O.X - W2, Cy - P.L(0.06f)}, {O.X - W2 * 0.5f, Cy + P.L(0.02f)}, {O.X, Cy - P.L(0.12f)}, {O.X + W2 * 0.5f, Cy + P.L(0.02f)},
							{O.X + W2, Cy - P.L(0.06f)}, {O.X + W2, Cy + P.L(0.1f)}},
			Paint::Linear({O.X, Cy - P.L(0.12f)}, {O.X, Cy + P.L(0.1f)}, Hex(0xffe9a6), Hex(0xd29a22)));
	}
}

void Ring(const Pen& P, const badge::Badge& B, float Detail)
{
	// The band, the setting and the stone on top; a Main Event's carries a bigger stone with a double halo.
	const uint32_t Stone = B.Color != 0 ? B.Color : 0x2f6fefu;
	const Vec2 O = P.P(0.0f, 0.12f);
	P.C.StrokeEllipse(O.X, O.Y, P.L(0.27f), P.L(0.27f), Hex(0x000000, 0.4f), P.L(0.13f));
	P.C.StrokeEllipse(O.X, O.Y, P.L(0.27f), P.L(0.27f), Hex(tok::Gold), P.L(0.085f));
	const Vec2 St = P.P(0.0f, -0.2f);
	const bool Halo = B.Main;
	const float R = P.L(Halo ? 0.13f : 0.15f);
	if (Halo)
	{
		// The double halo from 16; under it one ring of white round the stone.
		P.C.FillCircle(St.X, St.Y, R + P.L(0.12f), Paint(Hex(tok::Gold)));
		P.C.StrokeEllipse(St.X, St.Y, R + P.L(0.085f), R + P.L(0.085f), Hex(0xf4f7ff), P.L(Detail >= 16.0f ? 0.035f : 0.05f));
		if (Detail >= 16.0f)
		{
			P.C.StrokeEllipse(St.X, St.Y, R + P.L(0.035f), R + P.L(0.035f), Hex(0xf4f7ff), P.L(0.03f));
		}
	}
	else
	{
		P.C.FillCircle(St.X, St.Y, R + P.L(0.05f), Paint(Hex(tok::Gold)));
	}
	P.C.FillCircle(St.X, St.Y, R, Paint::Radial({St.X - R * 0.3f, St.Y - R * 0.35f}, 0.0f, St, R, Shade(Stone, 0.55f), 0.45f, Hex(Stone), Shade(Stone, -0.45f)));
	if (Detail >= 16.0f)
	{
		P.C.FillCircle(St.X - R * 0.35f, St.Y - R * 0.35f, R * 0.25f, Paint(Hex(0xffffff, 0.85f)));
	}
}

std::vector<Vec2> PatchShape(const Pen& P, float Inset)
{
	// The five-sided patch: a flat top, straight shoulders, a point at the foot.
	const float K = 1.0f - Inset;
	return {P.P(-0.4f * K, -0.43f * K - Inset * 0.04f), P.P(0.4f * K, -0.43f * K - Inset * 0.04f), P.P(0.4f * K, 0.1f * K), P.P(0.0f, 0.47f * K), P.P(-0.4f * K, 0.1f * K)};
}

void Stitching(const Pen& P, const Color& Thread)
{
	// A dashed line just inside the merrowed edge.
	const std::vector<Vec2> Path = PatchShape(P, 0.16f);
	const float Dash = P.L(0.07f);
	for (size_t K = 0; K < Path.size(); ++K)
	{
		const Vec2 A = Path[K];
		const Vec2 B = Path[(K + 1) % Path.size()];
		const float Dx = B.X - A.X;
		const float Dy = B.Y - A.Y;
		const float Len = std::sqrt(Dx * Dx + Dy * Dy);
		const int N = std::max(1, static_cast<int>(Len / (Dash * 2.0f)));
		for (int D = 0; D < N; ++D)
		{
			const float T0 = (Fl(D) * 2.0f + 0.5f) * Dash / Len;
			const float T1 = std::min(1.0f, T0 + Dash / Len);
			P.C.StrokePolyline({{A.X + Dx * T0, A.Y + Dy * T0}, {A.X + Dx * T1, A.Y + Dy * T1}}, false, Thread, P.L(0.025f));
		}
	}
}

void Patch(const Pen& P, uint32_t Ground, uint32_t Edge, float Detail)
{
	P.C.FillPolygon(PatchShape(P, -0.03f), Paint(Hex(0x000000, 0.35f)));
	P.C.FillPolygon(PatchShape(P, 0.0f), Paint(Hex(Edge)));
	P.C.FillPolygon(PatchShape(P, 0.09f), Paint::Linear(P.P(0.0f, -0.42f), P.P(0.0f, 0.45f), Shade(Ground, 0.12f), Shade(Ground, -0.18f)));
	if (Detail >= 20.0f)
	{
		Stitching(P, Fade(Hex(Edge), 0.9f));
	}
}

void TeamRiverLine(const Pen& P, float Detail)
{
	Patch(P, tok::Gold, 0x8a6414, Detail);
	// RiverLine's wave: two swells across the gold.
	for (int Line = 0; Line < 2; ++Line)
	{
		std::vector<Vec2> Wave;
		const float Y0 = Line == 0 ? -0.1f : 0.07f;
		for (int K = 0; K <= 12; ++K)
		{
			const float X = -0.24f + 0.48f * Fl(K) / 12.0f;
			Wave.push_back(P.P(X, Y0 + 0.055f * std::sin((X + 0.24f) / 0.48f * Pi2)));
		}
		P.C.StrokePolyline(Wave, false, Hex(tok::Ink), P.L(Detail >= 16.0f ? 0.06f : 0.08f), true);
	}
}

void Sponsored(const Pen& P, const badge::Badge& B, float Detail)
{
	const sponsor::Brand* Br = sponsor::FindBrand(B.Brand);
	const uint32_t Ground = Br ? Br->Color : (B.Color != 0 ? B.Color : tok::Ice);
	const uint32_t Edge = Br ? Br->Ink : (Light(Ground) ? 0x16181du : 0xf4f1eau);
	Patch(P, Ground, Edge, Detail);
	if (Br)
	{
		// The brand's own mark at small size, on a disc of the patch's thread so it reads on its own ground.
		const Vec2 M = P.P(0.0f, -0.03f);
		P.C.FillCircle(M.X, M.Y, P.L(0.25f), Paint(Fade(Hex(Edge), 0.22f)));
		sponsorart::Mark(P.C, B.Brand, {M.X - P.L(0.22f), M.Y - P.L(0.22f), P.L(0.44f), P.L(0.44f)});
	}
	else
	{
		const std::string Initial = B.Brand.empty() ? std::string("S") : B.Brand.substr(0, 1);
		P.C.Text(Initial, P.Cx, P.Cy + P.L(0.1f), Ts(P.L(0.34f), 900, Hex(Edge), Align::Center));
	}
}

void CircuitCard(const Pen& P, const badge::Badge& B, float Detail)
{
	// A 1.586:1 card with its chip. Card: silver. Gold: gold, and a stripe. Black: black, a gold edge and a star.
	const int Tier = std::clamp(B.Tier, 1, 3);
	const float W = P.L(0.94f);
	const float H = W / 1.586f;
	const Rect R{P.Cx - W * 0.5f, P.Cy - H * 0.5f, W, H};
	const float Rad = P.L(0.08f);
	P.C.FillRoundRect({R.X, R.Y + P.L(0.03f), R.W, R.H}, Rad, Paint(Hex(0x000000, 0.35f)));
	const uint32_t Body = Tier == 3 ? tok::Onyx : Tier == 2 ? tok::Gold : tok::CardSilver;
	P.C.FillRoundRect(R, Rad, Paint::Linear({R.X, R.Y}, {R.X + R.W, R.Y + R.H}, Shade(Body, Tier == 3 ? 0.12f : 0.25f), Shade(Body, Tier == 3 ? -0.2f : -0.28f)));
	if (Tier == 2)
	{
		P.C.FillRect({R.X, R.Y + R.H * 0.62f, R.W, R.H * 0.16f}, Paint(Shade(tok::Gold, -0.45f)));
	}
	if (Tier == 3)
	{
		P.C.StrokeRoundRect({R.X + P.L(0.02f), R.Y + P.L(0.02f), R.W - P.L(0.04f), R.H - P.L(0.04f)}, Rad * 0.8f, Hex(tok::Gold), P.L(Detail >= 16.0f ? 0.035f : 0.05f));
		P.C.FillPolygon(Star(P, 0.27f, -0.02f, 0.11f, 0.045f, 5), Paint(Hex(tok::Gold)));
	}
	// The chip: a gold square with its contacts.
	const Rect Chip{R.X + R.W * 0.13f, R.Y + R.H * 0.24f, R.W * 0.22f, R.H * 0.32f};
	P.C.FillRoundRect(Chip, P.L(0.025f), Paint::Linear({Chip.X, Chip.Y}, {Chip.X + Chip.W, Chip.Y + Chip.H}, Hex(0xf7dc8e), Hex(0xb98a2a)));
	if (Detail >= 16.0f)
	{
		P.C.StrokePolyline({{Chip.X, Chip.Y + Chip.H * 0.5f}, {Chip.X + Chip.W, Chip.Y + Chip.H * 0.5f}}, false, Hex(0x7a5a16, 0.8f), P.L(0.012f));
		P.C.StrokePolyline({{Chip.X + Chip.W * 0.5f, Chip.Y}, {Chip.X + Chip.W * 0.5f, Chip.Y + Chip.H}}, false, Hex(0x7a5a16, 0.8f), P.L(0.012f));
	}
}

void VerifiedPro(const Pen& P, float Detail)
{
	// The hexagon, and RiverLine Stats' three bars.
	std::vector<Vec2> Hexagon;
	std::vector<Vec2> Shadow;
	for (int K = 0; K < 6; ++K)
	{
		const float A = -Pi2 * 0.25f + Pi2 * Fl(K) / 6.0f;
		Hexagon.push_back(P.P(0.48f * std::cos(A), 0.48f * std::sin(A)));
		Shadow.push_back(P.P(0.49f * std::cos(A), 0.03f + 0.49f * std::sin(A)));
	}
	P.C.FillPolygon(Shadow, Paint(Hex(0x000000, 0.35f)));
	P.C.FillPolygon(Hexagon, Paint::Linear(P.P(0.0f, -0.48f), P.P(0.0f, 0.48f), Shade(tok::Teal, 0.2f), Shade(tok::Teal, -0.3f)));
	if (Detail >= 24.0f)
	{
		std::vector<Vec2> Inner;
		for (int K = 0; K < 6; ++K)
		{
			const float A = -Pi2 * 0.25f + Pi2 * Fl(K) / 6.0f;
			Inner.push_back(P.P(0.4f * std::cos(A), 0.4f * std::sin(A)));
		}
		P.C.StrokePolyline(Inner, true, Hex(tok::Ink, 0.35f), P.L(0.015f));
	}
	const float Bw = 0.1f;
	const float Base = 0.2f;
	const float Heights[3] = {0.17f, 0.27f, 0.38f};
	for (int K = 0; K < 3; ++K)
	{
		const float X = -0.17f + 0.14f * Fl(K);
		const Vec2 Top = P.P(X, Base - Heights[K]);
		P.C.FillRoundRect({Top.X, Top.Y, P.L(Bw), P.L(Heights[K])}, P.L(0.02f), Paint(Hex(tok::Ink, K == 2 ? 1.0f : 0.7f)));
	}
}

void KastPartner(const Pen& P, float Detail)
{
	const Rect R{P.Cx - P.L(0.43f), P.Cy - P.L(0.43f), P.L(0.86f), P.L(0.86f)};
	P.C.FillRoundRect({R.X, R.Y + P.L(0.03f), R.W, R.H}, P.L(0.2f), Paint(Hex(0x000000, 0.35f)));
	P.C.FillRoundRect(R, P.L(0.2f), Paint::Linear({R.X, R.Y}, {R.X, R.Y + R.H}, Shade(tok::Violet, 0.15f), Shade(tok::Violet, -0.2f)));
	P.C.StrokePolyline({P.P(-0.21f, 0.0f), P.P(-0.06f, 0.15f), P.P(0.22f, -0.15f)}, false, Hex(0xffffff), P.L(Detail >= 16.0f ? 0.1f : 0.13f), true);
}

/** The Embercrest's three peaks, standing on y = 0.24. */
std::vector<Vec2> Peaks(const Pen& P, float Lift = 0.0f)
{
	return {P.P(-0.44f, 0.24f + Lift), P.P(-0.22f, -0.05f + Lift), P.P(-0.12f, 0.05f + Lift), P.P(0.0f, -0.3f + Lift), P.P(0.11f, -0.01f + Lift), P.P(0.21f, -0.11f + Lift),
		P.P(0.44f, 0.24f + Lift)};
}

void Ember(const Pen& P, float X, float Y, float R)
{
	const Vec2 E = P.P(X, Y);
	P.C.FillCircle(E.X, E.Y, P.L(R), Paint::Radial({E.X, E.Y + P.L(R * 0.25f)}, 0.0f, E, P.L(R), Hex(tok::Flame), 0.5f, Hex(tok::EmberHot), Hex(0x9e2f0c)));
}

void Wall(const Pen& P, const badge::Badge& B, float Detail)
{
	// The wall's mark: the flame rising behind the crest, and how many times.
	const Vec2 O = P.P(0.0f, 0.0f);
	P.C.FillCircle(O.X, O.Y, P.L(0.48f), Paint(Hex(tok::Navy)));
	P.C.StrokeEllipse(O.X, O.Y, P.L(0.46f), P.L(0.46f), Hex(tok::EmberHot), P.L(Detail >= 16.0f ? 0.04f : 0.06f));
	// The flame: a teardrop (a round foot, curved sides up to a point), ember outside, flame inside.
	auto Drop = [&](float Scale, float Foot, const Color& Col) {
		const float R = 0.19f * Scale;
		std::vector<Vec2> F;
		for (int K = 0; K <= 10; ++K)
		{
			const float A = Pi2 * 0.5f * Fl(K) / 10.0f; // the foot: right, down, round to the left
			F.push_back(P.P(R * std::cos(A), Foot + R * std::sin(A)));
		}
		// Up the left side to the tip and down the right, each a quadratic curve that leans in.
		const float TipY = Foot - R * 2.3f;
		for (int Side = -1; Side <= 1; Side += 2)
		{
			for (int K = 1; K <= 6; ++K)
			{
				const float T = Side < 0 ? Fl(K) / 6.0f : 1.0f - Fl(K) / 6.0f;
				const float X0 = static_cast<float>(Side) * R;
				const float Cx = static_cast<float>(Side) * R * 0.95f;
				const float Cy = Foot - R * 1.1f;
				const float U = 1.0f - T;
				const float X = U * U * X0 + 2.0f * U * T * Cx;
				const float Y = U * U * Foot + 2.0f * U * T * Cy + T * T * TipY;
				if (Side < 0 || K < 6)
				{
					F.push_back(P.P(X, Y));
				}
			}
		}
		P.C.FillPolygon(F, Paint(Col));
	};
	Drop(1.0f, 0.04f, Hex(tok::EmberHot));
	Drop(0.55f, 0.1f, Hex(tok::Flame));
	P.C.FillPolygon({P.P(-0.36f, 0.3f), P.P(-0.17f, 0.08f), P.P(-0.08f, 0.16f), P.P(0.02f, 0.0f), P.P(0.13f, 0.14f), P.P(0.2f, 0.06f), P.P(0.36f, 0.3f)}, Paint(Hex(tok::Ash)));
	if (B.Count >= 2)
	{
		CountMedal(P, CountText(B.Count, 9), Detail);
	}
}

void Club(const Pen& P, int Tier, float Detail)
{
	// Regular: the crest in an ash outline. Ember: copper, and the ember's dot. Crest: gold on navy, the full ember.
	if (Tier >= 3)
	{
		const Vec2 O = P.P(0.0f, 0.0f);
		P.C.FillCircle(O.X, O.Y, P.L(0.48f), Paint(Hex(tok::Navy)));
		P.C.StrokeEllipse(O.X, O.Y, P.L(0.46f), P.L(0.46f), Hex(tok::LiveGold), P.L(Detail >= 16.0f ? 0.03f : 0.05f));
		Ember(P, 0.0f, -0.08f, 0.24f);
		P.C.FillPolygon(Peaks(P, 0.02f), Paint(Hex(tok::LiveGold)));
		if (Detail >= 20.0f)
		{
			P.C.FillPolygon({P.P(0.0f, -0.28f), P.P(0.11f, 0.01f), P.P(0.05f, 0.26f), P.P(0.0f, 0.26f)}, Paint(Shade(tok::LiveGold, -0.3f)));
		}
		return;
	}
	if (Tier == 2)
	{
		const Vec2 D = P.P(0.0f, -0.36f);
		P.C.FillCircle(D.X, D.Y, P.L(0.1f), Paint(Hex(tok::EmberHot)));
		P.C.FillPolygon(Peaks(P), Paint::Linear(P.P(0.0f, -0.3f), P.P(0.0f, 0.24f), Shade(tok::Copper, 0.15f), Shade(tok::Copper, -0.25f)));
		P.C.FillRect({P.Cx - P.L(0.44f), P.Cy + P.L(0.28f), P.L(0.88f), P.L(0.06f)}, Paint(Hex(tok::Copper)));
		return;
	}
	P.C.StrokePolyline(Peaks(P), true, Hex(tok::Ash), P.L(Detail >= 16.0f ? 0.05f : 0.07f));
	P.C.FillRect({P.Cx - P.L(0.44f), P.Cy + P.L(0.29f), P.L(0.88f), P.L(0.05f)}, Paint(Hex(tok::Ash)));
}

/** One glyph, with the detail Detail allows, centred on (Cx, Cy). */
void Art(Canvas& C, const badge::Badge& B, float Cx, float Cy, float Size, float Detail)
{
	const Pen P{C, Cx, Cy, Size};
	switch (B.K)
	{
	case badge::Kind::HallOfFame: Hall(P, Detail); break;
	case badge::Kind::Bracelet:
		Bracelet(P, B, Detail);
		if (B.Count >= 2)
		{
			CountMedal(P, CountText(B.Count, 9), Detail);
		}
		break;
	case badge::Kind::Ring:
		Ring(P, B, Detail);
		if (B.Count >= 2)
		{
			CountMedal(P, CountText(B.Count, 9), Detail);
		}
		break;
	case badge::Kind::PlayerOfYear: PlayerOfYear(P, B, Detail, false); break;
	case badge::Kind::LivePlayerOfYear: PlayerOfYear(P, B, Detail, true); break;
	case badge::Kind::TeamRiverLine: TeamRiverLine(P, Detail); break;
	case badge::Kind::Sponsored: Sponsored(P, B, Detail); break;
	case badge::Kind::CircuitCard: CircuitCard(P, B, Detail); break;
	case badge::Kind::VerifiedPro: VerifiedPro(P, Detail); break;
	case badge::Kind::KastPartner: KastPartner(P, Detail); break;
	case badge::Kind::OnTheWall: Wall(P, B, Detail); break;
	case badge::Kind::Club: Club(P, B.Tier, Detail); break;
	case badge::Kind::Count: break;
	}
}

uint64_t KeyOf(const badge::Badge& B, float Size, float Detail)
{
	uint64_t H = 1469598103934665603ull;
	auto Mx = [&H](uint64_t V) { H = (H ^ V) * 1099511628211ull; };
	Mx(0xb4d6e000ull);
	Mx(static_cast<uint64_t>(B.K));
	Mx(static_cast<uint64_t>(static_cast<int64_t>(B.Tier)));
	Mx(static_cast<uint64_t>(static_cast<int64_t>(B.Count)));
	Mx(static_cast<uint64_t>(static_cast<int64_t>(Detail >= 24.0f ? B.Year : 0)));
	Mx(B.Main ? 1u : 0u);
	Mx(B.Color);
	for (const char Ch : B.Brand)
	{
		Mx(static_cast<unsigned char>(Ch));
	}
	Mx(static_cast<uint64_t>(std::llround(static_cast<double>(Size) * 16.0)));
	Mx(static_cast<uint64_t>(std::llround(static_cast<double>(Detail) * 16.0)));
	return H;
}

/** Letter-spaced small caps (the room's and the club card's labels). Returns the width. */
float Tracked(Canvas& C, const std::string& S, float X, float Y, float Size, int Weight, const Color& Col, float Track, Align A = Align::Left)
{
	std::vector<std::string> Glyphs;
	for (size_t I = 0; I < S.size();)
	{
		const unsigned char Lead = static_cast<unsigned char>(S[I]);
		const size_t Len = Lead >= 0xF0 ? 4 : Lead >= 0xE0 ? 3 : Lead >= 0xC0 ? 2 : 1;
		Glyphs.push_back(S.substr(I, Len));
		I += Len;
	}
	float Wd = 0.0f;
	for (const std::string& G : Glyphs)
	{
		Wd += C.Measure(G, Size, Weight);
	}
	Wd += Track * Fl(static_cast<int>(Glyphs.empty() ? 0 : Glyphs.size() - 1));
	float Px = X - (A == Align::Center ? Wd * 0.5f : A == Align::Right ? Wd : 0.0f);
	for (const std::string& G : Glyphs)
	{
		Px += C.Text(G, Px, Y, Ts(Size, Weight, Col)) + Track;
	}
	return Wd;
}

const char* TierWord(int Tier)
{
	return Tier >= 3 ? "CREST" : Tier == 2 ? "EMBER" : Tier == 1 ? "REGULAR" : "NEW MEMBER";
}
} // namespace badgeart_detail

using namespace badgeart_detail;

void Draw(Canvas& C, const badge::Badge& B, float Cx, float Cy, float Size, double Time, bool Glint)
{
	if (B.K == badge::Kind::Count || Size <= 0.0f)
	{
		return;
	}
	// Paint runs inside Cached (only the first time), so it can hold B by reference: no copy, no allocation a frame.
	C.Cached(KeyOf(B, Size, Size), Cx, Cy, [&B, Size](Canvas& Cc) { Art(Cc, B, 0.0f, 0.0f, Size, Size); });
	if (Glint)
	{
		// The one glint the first time a badge is seen (P2): a spark at the corner that blooms and goes.
		const float T = static_cast<float>(std::fmod(Time, 2.4) / 2.4);
		const float A = T < 0.5f ? std::sin(T * 2.0f * 3.14159f) : 0.0f;
		if (A > 0.01f)
		{
			const Pen P{C, Cx, Cy, Size};
			C.FillPolygon(Star(P, 0.3f, -0.32f, 0.16f * A, 0.03f, 4), Paint(Hex(0xffffff, 0.9f * A)));
		}
	}
}

float Rail(Canvas& C, const badge::Set& S, uint8_t Context, float X, float Cy, float Size, int Max, bool Labels)
{
	const std::vector<badge::Badge> All = badge::InOrder(S, Context, badge::FrameNone);
	const int Shown = std::min(static_cast<int>(All.size()), std::max(0, Max));
	float Px = X;
	float End = X;
	for (int K = 0; K < Shown; ++K)
	{
		Draw(C, All[static_cast<size_t>(K)], Px + Size * 0.5f, Cy, Size, 0.0);
		Px += Size;
		if (Labels && K < 3)
		{
			Px += Size * 0.3f;
			Px += C.Text(badge::Label(All[static_cast<size_t>(K)]), Px, Cy + Size * 0.19f, Ts(Size * 0.5f, 800, Hex(0xe6edf7, 0.92f)));
		}
		End = Px;
		Px += Size * (Labels ? 0.6f : 0.3f);
	}
	// C7: past Max, a count of the rest (the card's TROPHIES & BADGES tab holds them all).
	const int More = static_cast<int>(All.size()) - Shown;
	if (More > 0)
	{
		End = Px + C.Text("+" + std::to_string(More), Px, Cy + Size * 0.19f, Ts(Size * 0.55f, 800, Hex(0x7b8aa3)));
	}
	return End - X;
}

const std::vector<badge::Badge>& AtlasBadges()
{
	static const std::vector<badge::Badge> All = [] {
		std::vector<badge::Badge> V;
		auto Add = [&V](badge::Kind K, int Tier, int Count, bool Main, uint32_t Color, const std::string& Brand) {
			badge::Badge B;
			B.K = K;
			B.Tier = Tier;
			B.Count = Count;
			B.Main = Main;
			B.Color = Color;
			B.Brand = Brand;
			V.push_back(B);
		};
		Add(badge::Kind::HallOfFame, 0, 1, false, tok::Gold, "");
		Add(badge::Kind::PlayerOfYear, 1, 1, false, tok::Teal, "");
		Add(badge::Kind::LivePlayerOfYear, 1, 1, false, tok::LiveGold, "");
		for (int Ring = 0; Ring < 2; ++Ring)
		{
			for (int Main = 0; Main < 2; ++Main)
			{
				for (int N = 1; N <= 5; ++N)
				{
					Add(Ring ? badge::Kind::Ring : badge::Kind::Bracelet, 0, N, Main == 1, Ring ? 0x2f6fefu : 0x2a2a35u, "");
				}
			}
		}
		Add(badge::Kind::TeamRiverLine, 0, 0, false, tok::Gold, "riverline");
		for (const sponsor::Brand& Br : sponsor::Brands())
		{
			if (Br.Id != "riverline")
			{
				Add(badge::Kind::Sponsored, 0, 0, false, Br.Color, Br.Id);
			}
		}
		Add(badge::Kind::Sponsored, 0, 0, false, tok::Ice, "");
		for (int Tier = 1; Tier <= 3; ++Tier)
		{
			Add(badge::Kind::CircuitCard, Tier, 0, false, Tier == 3 ? tok::Onyx : Tier == 2 ? tok::Gold : tok::CardSilver, "");
		}
		Add(badge::Kind::VerifiedPro, 0, 0, false, tok::Teal, "");
		Add(badge::Kind::KastPartner, 0, 0, false, tok::Violet, "");
		for (int N = 1; N <= 9; ++N)
		{
			Add(badge::Kind::OnTheWall, 0, N, false, tok::EmberHot, "");
		}
		for (int Tier = 1; Tier <= 3; ++Tier)
		{
			Add(badge::Kind::Club, Tier, 0, false, Tier == 3 ? tok::LiveGold : Tier == 2 ? tok::Copper : tok::Ash, "");
		}
		return V;
	}();
	return All;
}

int AtlasCell(const badge::Badge& B)
{
	const std::vector<badge::Badge>& All = AtlasBadges();
	int Generic = -1;
	for (size_t K = 0; K < All.size(); ++K)
	{
		const badge::Badge& A = All[K];
		if (A.K != B.K)
		{
			continue;
		}
		switch (B.K)
		{
		case badge::Kind::Bracelet:
		case badge::Kind::Ring:
			if (A.Main == B.Main && A.Count == std::clamp(B.Count, 1, 5))
			{
				return static_cast<int>(K);
			}
			break;
		case badge::Kind::OnTheWall:
			if (A.Count == std::clamp(B.Count, 1, 9))
			{
				return static_cast<int>(K);
			}
			break;
		case badge::Kind::CircuitCard:
		case badge::Kind::Club:
			if (A.Tier == std::clamp(B.Tier, 1, 3))
			{
				return static_cast<int>(K);
			}
			break;
		case badge::Kind::Sponsored:
			if (A.Brand == B.Brand)
			{
				return static_cast<int>(K);
			}
			if (A.Brand.empty())
			{
				Generic = static_cast<int>(K);
			}
			break;
		case badge::Kind::Count: return -1;
		default: return static_cast<int>(K);
		}
	}
	return Generic;
}

int Atlas(Canvas& C, float Cell)
{
	const std::vector<badge::Badge>& All = AtlasBadges();
	for (size_t K = 0; K < All.size(); ++K)
	{
		const float X = (Fl(static_cast<int>(K % 8)) + 0.5f) * Cell;
		const float Y = (Fl(static_cast<int>(K / 8)) + 0.5f) * Cell;
		badge::Badge B = All[K];
		// The last count cell of each kind is "5+" or "9+": it stands for everything past it.
		const bool Last = (B.K == badge::Kind::OnTheWall && B.Count == 9) || ((B.K == badge::Kind::Bracelet || B.K == badge::Kind::Ring) && B.Count == 5);
		if (Last)
		{
			B.Count = B.K == badge::Kind::OnTheWall ? 10 : 6;
		}
		const float Size = Cell * 0.86f;
		const float Detail = 16.0f;
		const std::string Text = B.K == badge::Kind::OnTheWall ? CountText(B.Count, 9) : CountText(B.Count, 5);
		if (Last || B.Count >= 2)
		{
			// Draw it without its own medal, then the atlas's ("5+" past the fifth).
			badge::Badge NoMedal = B;
			NoMedal.Count = 1;
			Art(C, NoMedal, X, Y, Size, Detail);
			CountMedal(Pen{C, X, Y, Size}, Text, Detail);
		}
		else
		{
			Art(C, B, X, Y, Size, Detail);
		}
	}
	return static_cast<int>(All.size());
}

// ------------------------------------------------------------------ the Players Club card

void ClubCard(Canvas& C, const Rect& R, const ClubCardInfo& Info, double Now)
{
	// Drawn on a 500 x 315 card and scaled to fit R; under 60% the fine print goes and the tier word grows.
	const float Sc = std::min(R.W / 500.0f, R.H / 315.0f);
	if (Sc <= 0.0f)
	{
		return;
	}
	const float Ox = R.X + (R.W - 500.0f * Sc) * 0.5f;
	const float Oy = R.Y + (R.H - 315.0f * Sc) * 0.5f;
	const bool Fine = Sc >= 0.6f;
	const int Tier = std::clamp(Info.Tier, 0, 3);
	const Color Accent = Hex(Tier >= 3 ? tok::LiveGold : Tier == 2 ? tok::Copper : Tier == 1 ? tok::Ash : tok::LiveGold);
	const Color Ash = Hex(tok::Ash);
	const Color Ice = Hex(tok::Ice);
	C.Save();
	C.Translate(Ox, Oy);
	C.Scale(Sc, Sc);
	const Rect Card{0.0f, 0.0f, 500.0f, 315.0f};
	// The stock: midnight navy, warmer as the tier climbs; the ember in its corner; a gold edge on a Crest.
	C.GlowRoundRect({0.0f, 8.0f, 500.0f, 315.0f}, 22.0f, Hex(0x000000, 0.55f), 30.0f);
	const uint32_t Top = Tier >= 3 ? 0x1f2a48u : Tier == 2 ? 0x2a1d1cu : 0x1d2a47u;
	C.FillRoundRect(Card, 22.0f, Paint::Linear({0.0f, 0.0f}, {500.0f, 315.0f}, Hex(Top), Hex(tok::Midnight)));
	C.PushClip({10.0f, 10.0f, 480.0f, 295.0f});
	C.FillCircle(46.0f, 48.0f, 230.0f, Paint::Radial({46.0f, 48.0f}, 0.0f, {46.0f, 48.0f}, 230.0f, Fade(Hex(tok::Copper), Tier >= 2 ? 0.24f : 0.14f), 0.45f, Fade(Hex(tok::Copper), 0.04f), Fade(Hex(tok::Copper), 0.0f)));
	// The tier's crest, large and faint, low on the right.
	{
		const float A0 = C.GetAlpha();
		C.SetAlpha(A0 * 0.09f);
		badge::Badge Big;
		Big.K = badge::Kind::Club;
		Big.Tier = std::max(1, Tier);
		Art(C, Big, 410.0f, 210.0f, 230.0f, 64.0f);
		C.SetAlpha(A0);
	}
	// The foil: a slow sweep, and a bright one as a fresh card leaves the printer.
	const float Sweep = static_cast<float>(std::fmod(Now * 0.25, 1.6)) - 0.3f;
	const float Bx = Sweep * 500.0f;
	C.FillPolygon({{Bx, 0.0f}, {Bx + 120.0f, 0.0f}, {Bx - 60.0f, 315.0f}, {Bx - 180.0f, 315.0f}},
		Paint::Linear({Bx - 180.0f, 0.0f}, {Bx + 120.0f, 0.0f}, Fade(Hex(tok::Flame), 0.0f), Fade(Hex(tok::Flame), Info.New ? 0.12f : 0.05f)));
	C.PopClip();
	C.StrokeRoundRect({0.75f, 0.75f, 498.5f, 313.5f}, 22.0f, Fade(Tier >= 3 ? Hex(tok::LiveGold) : Accent, Tier >= 3 ? 0.85f : 0.4f), Tier >= 3 ? 2.5f : 1.5f);
	// Header: the crest, the room, the club, the tier.
	badge::Badge Mark;
	Mark.K = badge::Kind::Club;
	Mark.Tier = std::max(1, Tier);
	Art(C, Mark, 48.0f, 48.0f, 50.0f, 50.0f);
	Tracked(C, "EMBERCREST", 86.0f, 46.0f, 20.0f, 900, Ash, 5.0f);
	if (Fine)
	{
		Tracked(C, "CASINO \xC2\xB7 PLAYERS CLUB", 86.0f, 64.0f, 11.0f, 700, Fade(Hex(tok::LiveGold), 0.9f), 3.0f);
	}
	{
		const std::string Word = TierWord(Tier);
		const float Size = Fine ? 12.0f : 22.0f;
		float Tw = 0.0f;
		for (const char Ch : Word)
		{
			Tw += C.Measure(std::string(1, Ch), Size, 800);
		}
		Tw += (Fine ? 2.4f : 3.0f) * Fl(static_cast<int>(Word.size()) - 1);
		const float Pw = Tw + (Fine ? 28.0f : 34.0f);
		const float Ph = Fine ? 28.0f : 44.0f;
		const Rect Pill{472.0f - Pw, Fine ? 28.0f : 24.0f, Pw, Ph};
		C.FillRoundRect(Pill, Ph * 0.5f, Paint(Fade(Accent, 0.18f)));
		C.StrokeRoundRect(Pill, Ph * 0.5f, Fade(Accent, 0.6f), 1.2f);
		Tracked(C, Word, Pill.X + Pw * 0.5f, Pill.Y + Ph * 0.5f + Size * 0.36f, Size, 800, Accent, Fine ? 2.4f : 3.0f, Align::Center);
	}
	C.FillRect({28.0f, 82.0f, 444.0f, 1.0f}, Paint::Linear({28.0f, 0.0f}, {472.0f, 0.0f}, Fade(Hex(tok::Copper), 0.6f), Fade(Hex(tok::LiveGold), 0.0f)));
	// The member: their name, their screen name and country.
	const std::string Name = Info.Name.empty() ? Info.ScreenName : Info.Name;
	C.Text(Name, 28.0f, Fine ? 128.0f : 150.0f, Ts(Fine ? 30.0f : 44.0f, 800, Ash, Align::Left, Baseline::Alphabetic, false, 444.0f));
	if (!Fine)
	{
		// The small card keeps one fact, big enough to read: the nights in the last 90 days.
		Tracked(C, std::to_string(Info.Visits) + " IN 90 DAYS", 28.0f, 214.0f, 24.0f, 800, Ice, 3.0f);
	}
	if (Fine)
	{
		std::string Sub = Info.ScreenName.empty() ? std::string() : "@" + Info.ScreenName;
		if (!Info.Country.empty())
		{
			Sub += (Sub.empty() ? "" : " \xC2\xB7 ") + Info.Country;
		}
		C.Text(Sub, 28.0f, 156.0f, Ts(15.0f, 600, Ice, Align::Left, Baseline::Alphabetic, false, 300.0f));
		// The strip under it: the way to the next card.
		const int Need = Tier >= 3 ? badge::ClubCrest : Tier == 2 ? badge::ClubCrest : Tier == 1 ? badge::ClubEmber : badge::ClubRegular;
		const char* Next = Tier >= 2 ? "Crest" : Tier == 1 ? "Ember" : "Regular";
		const std::string Strip = Tier >= 3 ? std::to_string(Info.Visits) + " in 90 days \xC2\xB7 the top card"
											: std::to_string(std::min(Info.Visits, Need)) + " of " + std::to_string(Need) + " to " + Next;
		const float Fill = Tier >= 3 ? 1.0f : std::clamp(static_cast<float>(Info.Visits) / static_cast<float>(Need), 0.0f, 1.0f);
		C.FillRoundRect({28.0f, 178.0f, 250.0f, 8.0f}, 4.0f, Paint(Fade(Ash, 0.12f)));
		if (Fill > 0.0f)
		{
			C.FillRoundRect({28.0f, 178.0f, std::max(8.0f, 250.0f * Fill), 8.0f}, 4.0f, Paint::Linear({28.0f, 0.0f}, {278.0f, 0.0f}, Hex(tok::Copper), Accent));
		}
		Tracked(C, Strip, 28.0f, 206.0f, 12.0f, 800, Ice, 1.6f);
		if (Info.Wins > 0)
		{
			badge::Badge Flame;
			Flame.K = badge::Kind::OnTheWall;
			Flame.Count = Info.Wins;
			Art(C, Flame, 312.0f, 186.0f, 34.0f, 34.0f);
			Tracked(C, Info.Wins >= 2 ? "ON THE WALL \xC3\x97" + std::to_string(Info.Wins) : std::string("ON THE WALL"), 336.0f, 191.0f, 12.0f, 800, Hex(tok::EmberHot), 1.8f);
		}
		// The footer: since when, the member number, the barcode.
		const int Since = Info.SinceDay > 0 ? Info.SinceDay : net::NightOneDay;
		int Y = 0;
		int M = 0;
		int D = 0;
		world::CivilDate(Since, Y, M, D);
		static const char* const Months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
		Tracked(C, std::string("MEMBER SINCE ") + Months[std::clamp(M, 1, 12) - 1] + " " + std::to_string(Y), 28.0f, 262.0f, 10.0f, 800, Ice, 2.6f);
		uint32_t H = 2166136261u;
		for (const char Ch : Name + Info.ScreenName)
		{
			H = (H ^ static_cast<unsigned char>(Ch)) * 16777619u;
		}
		char Num[32];
		std::snprintf(Num, sizeof(Num), "No. %04u %04u %04u", (H >> 20) % 10000u, (H >> 8) % 10000u, H % 10000u);
		C.Text(Num, 28.0f, 287.0f, Ts(15.0f, 700, Fade(Ash, 0.8f), Align::Left, Baseline::Alphabetic, true));
		float Bx2 = 300.0f;
		for (int K = 0; K < 40 && Bx2 < 472.0f; ++K)
		{
			const float Bw = 1.0f + Fl(static_cast<int>((H >> (K % 29)) & 3u));
			C.FillRect({Bx2, 254.0f, Bw, 34.0f}, Paint(Fade(Ash, 0.75f)));
			Bx2 += Bw + 2.0f;
		}
	}
	C.Restore();
}

ClubCardInfo ClubCardOf(const SaveData& D, double World)
{
	ClubCardInfo I;
	const int Day = net::DayOf(World);
	I.Name = D.Person.FullName();
	I.ScreenName = D.HeroName;
	I.Country = D.Person.Country;
	I.Tier = badge::HeroClubTier(D.Life, Day);
	I.Visits = badge::VisitsIn(badge::HeroRoomDays(D.Life), Day);
	I.Wins = D.Life.LiveWins;
	I.SinceDay = net::NightOneDay;
	// New: no card at this tier or above has left the printer. A drop on the 1st keeps the better card's print, so the
	// desk greets a member then; it never re-prints a lower card with an upgrade line.
	I.New = I.Tier > 0;
	for (int Tier = I.Tier; I.New && Tier <= 3; ++Tier)
	{
		I.New = std::find(D.TextsSeen.begin(), D.TextsSeen.end(), badge::DeskKey(Tier)) == D.TextsSeen.end();
	}
	return I;
}

// ------------------------------------------------------------------ the room's screens

void RoomLeaders(Canvas& C, const Rect& R, const std::vector<life::RoomLeader>& Rows, const std::string& Month, const std::string& Hero, double Now)
{
	(void)Now;
	const Color Ash = Hex(tok::Ash);
	const Color Ice = Hex(tok::Ice);
	const Color Hot = Hex(tok::EmberHot);
	const Color Gold = Hex(tok::LiveGold);
	// The header: whose month it is, and the points column.
	const std::string Head = (Month.empty() ? std::string("THIS MONTH") : Month) + "'S LEADERS";
	Tracked(C, Head, R.X + 10.0f, R.Y + 22.0f, 17.0f, 900, Gold, 3.0f);
	Tracked(C, "POINTS", R.X + R.W - 10.0f, R.Y + 22.0f, 12.0f, 800, Ice, 2.4f, Align::Right);
	const float Top = R.Y + 34.0f;
	const float RowH = std::min(44.0f, (R.H - 34.0f) / 10.0f);
	const float Glyph = std::min(32.0f, RowH * 0.76f);
	const float Text = std::min(22.0f, RowH * 0.52f);
	for (size_t K = 0; K < Rows.size() && K < 10; ++K)
	{
		const life::RoomLeader& L = Rows[K];
		const float Y = Top + Fl(static_cast<int>(K)) * RowH;
		const float Mid = Y + RowH * 0.5f;
		const bool Mine = !Hero.empty() && L.Name == Hero;
		if (Mine)
		{
			C.FillRoundRect({R.X, Y + 2.0f, R.W, RowH - 4.0f}, 8.0f, Paint(Fade(Hot, 0.16f)));
			C.FillRect({R.X, Y + 6.0f, 4.0f, RowH - 12.0f}, Paint(Hot));
		}
		else if (K % 2 == 0)
		{
			C.FillRect({R.X, Y + 2.0f, R.W, RowH - 4.0f}, Paint(Fade(Ash, 0.04f)));
		}
		const Color Ink = Mine ? Hot : Ash;
		C.Text(std::to_string(L.Rank > 0 ? L.Rank : static_cast<int>(K) + 1), R.X + 52.0f, Mid + Text * 0.36f, Ts(Text, 800, Mine ? Hot : (K < 3 ? Gold : Ice), Align::Right, Baseline::Alphabetic, true));
		// One glyph a line (C8): the room's top badge, from the snapshot.
		float Nx = R.X + 72.0f;
		const badge::Set Held = badge::Unpack(L.Badge);
		if (const badge::Badge* B = badge::Top(Held, badge::Room, badge::FrameNone))
		{
			Draw(C, *B, Nx + Glyph * 0.5f, Mid, Glyph, 0.0);
		}
		Nx += Glyph + 14.0f;
		const float PointsW = 140.0f;
		C.Text(L.Name, Nx, Mid + Text * 0.36f, Ts(Text, 700, Ink, Align::Left, Baseline::Alphabetic, false, R.X + R.W - PointsW - Nx));
		C.Text(Grouped(static_cast<int64_t>(std::llround(L.Points))), R.X + R.W - 10.0f, Mid + Text * 0.36f, Ts(Text, 800, Ink, Align::Right, Baseline::Alphabetic, true));
	}
}
} // namespace badgeart
} // namespace ui
} // namespace ss
