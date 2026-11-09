#include "ShortStack/UI/Ui.h"
#include "../StrictFloat.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Game/SitePlay.h"

#include <cmath>
#include <cstring>
#include <vector>

namespace ss
{
namespace ui
{
namespace ui_detail
{

struct ButtonPalette
{
	Color Base;
	Color Hi;
	Color Ink;
};

ButtonPalette PaletteFor(ButtonKind K)
{
	switch (K)
	{
	case ButtonKind::Primary: return {Hex(0x1fb3a5), Hex(0x27d3c3), Hex(0x06201d)};
	case ButtonKind::Danger: return {Hex(0x7a2230), Hex(0x9b2b3c), Hex(0xffe9ec)};
	case ButtonKind::Ghost: return {Rgba(255, 255, 255, 0.02f), Rgba(255, 255, 255, 0.07f), pal::Ink};
	case ButtonKind::Gold: return {Hex(0xc9962b), Hex(0xf2c14e), Hex(0x231704)};
	default: return {Hex(0x1a2940), Hex(0x233756), pal::Ink};
	}
}
} // namespace ui_detail

using namespace ui_detail;

// ------------------------------------------------------------------ ui

void Ui::Begin(Canvas& InCanvas, double InTime)
{
	C = &InCanvas;
	Time = InTime;
	CursorIsPointer = false;
}

void Ui::End()
{
	if (Ptr.Released)
	{
		PressedId.clear();
	}
	Ptr.EndFrame();
	C = nullptr;
}

bool Ui::Hover(const Rect& R) const
{
	return Ptr.Active && Ptr.X >= R.X && Ptr.X <= R.X + R.W && Ptr.Y >= R.Y && Ptr.Y <= R.Y + R.H;
}

Ui::ClickState Ui::Clickable(const std::string& Id, const Rect& R, bool Enabled)
{
	ClickState S;
	S.Hover = Enabled && Hover(R);
	if (S.Hover)
	{
		CursorIsPointer = true;
	}
	if (S.Hover && Ptr.Pressed)
	{
		PressedId = Id;
	}
	S.Down = PressedId == Id && Ptr.Down && S.Hover;
	S.Clicked = Enabled && S.Hover && Ptr.Released && PressedId == Id;
	return S;
}

void Ui::RRect(const Rect& R, float Radius, const Paint& Fill)
{
	C->FillRoundRect(R, Radius, Fill);
}

void Ui::RRect(const Rect& R, float Radius, const Paint& Fill, const Color& Stroke, float LineWidth)
{
	C->FillRoundRect(R, Radius, Fill);
	C->StrokeRoundRect(R, Radius, Stroke, LineWidth);
}

void Ui::RRectStroke(const Rect& R, float Radius, const Color& Stroke, float LineWidth)
{
	C->StrokeRoundRect(R, Radius, Stroke, LineWidth);
}

bool Ui::Button(const std::string& Id, const Rect& R, const std::string& Label, const ButtonOpts& Opts)
{
	const ClickState St = Clickable(Id, R, Opts.Enabled);
	const ButtonPalette P = PaletteFor(Opts.Kind);
	const float Shift = St.Down ? 2.0f : 0.0f;
	const float Prev = C->GetAlpha();
	C->SetAlpha(Opts.Enabled ? Prev : Prev * 0.35f);
	const Rect Body{R.X, R.Y + Shift, R.W, R.H};
	const Paint Grad = Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, St.Hover ? P.Hi : P.Base, P.Base);
	if (Opts.Kind == ButtonKind::Ghost)
	{
		RRect(Body, 10.0f, Grad, Rgba(255, 255, 255, 0.12f));
	}
	else
	{
		RRect(Body, 10.0f, Grad);
	}
	const float Size = Opts.Size;
	const float Cy = R.Y + R.H / 2.0f + Shift;
	if (!Opts.Sub.empty())
	{
		Text(Label, R.X + R.W / 2.0f, Cy - 4.0f, Ts(Size, 700, P.Ink, Align::Center));
		Text(Opts.Sub, R.X + R.W / 2.0f, Cy + Size * 0.85f, Ts(Size * 0.72f, 600, P.Ink, Align::Center, Baseline::Alphabetic, true));
	}
	else
	{
		Text(Label, R.X + R.W / 2.0f, Cy + 1.0f, Ts(Size, 700, P.Ink, Align::Center, Baseline::Middle));
	}
	if (!Opts.Hotkey.empty())
	{
		Text(Opts.Hotkey, R.X + R.W - 10.0f, R.Y + 16.0f, Ts(12.0f, 700, P.Ink, Align::Right, Baseline::Middle, true));
	}
	C->SetAlpha(Prev);
	return St.Clicked;
}

double Ui::Slider(const std::string& Id, const Rect& R, double Value, double Min, double Max, double Step)
{
	const ClickState St = Clickable(Id, {R.X - 10.0f, R.Y - 12.0f, R.W + 20.0f, R.H + 24.0f});
	double V = Value;
	if ((St.Down || (PressedId == Id && Ptr.Down)) && Max > Min)
	{
		double T = (static_cast<double>(Ptr.X) - R.X) / R.W;
		T = T < 0.0 ? 0.0 : T > 1.0 ? 1.0 : T;
		V = Min + T * (Max - Min);
		V = std::round(V / Step) * Step;
	}
	if (St.Hover && Ptr.Wheel != 0.0f)
	{
		V -= (Ptr.Wheel > 0.0f ? 1.0 : -1.0) * Step;
	}
	V = V < Min ? Min : V > Max ? Max : V;
	const float T = Max > Min ? static_cast<float>((V - Min) / (Max - Min)) : 0.0f;
	RRect({R.X, R.Y + R.H / 2.0f - 4.0f, R.W, 8.0f}, 4.0f, Hex(0x0b1422), pal::Line);
	RRect({R.X, R.Y + R.H / 2.0f - 4.0f, R.W * T, 8.0f}, 4.0f, pal::Accent);
	const float Kx = R.X + R.W * T;
	const float Ky = R.Y + R.H / 2.0f;
	C->FillCircle(Kx, Ky, 13.0f, (St.Hover || PressedId == Id) ? Hex(0xffffff) : Hex(0xdce6f5));
	C->StrokeArc(Kx, Ky, 13.0f, 0.0f, 2.0f * Pi, pal::Accent, 3.0f);
	return V;
}

// ------------------------------------------------------------------ card art

Color SuitColor(int Suit)
{
	static const uint32_t Colors[4] = {0x1f8f4e, 0x2a6fdb, 0xd8313f, 0x1b1e24};
	return Hex(Colors[Suit & 3]);
}

void DrawSuit(Canvas& C, int Suit, float X, float Y, float S, const Color& Col)
{
	auto P = [&](float U, float V) { return Vec2{X + U * S, Y + V * S}; };
	const Paint Fill(Col);
	switch (Suit & 3)
	{
	case 0: // clubs
		C.FillCircle(X + 0.5f * S, Y + 0.28f * S, 0.21f * S, Fill);
		C.FillCircle(X + 0.26f * S, Y + 0.58f * S, 0.21f * S, Fill);
		C.FillCircle(X + 0.74f * S, Y + 0.58f * S, 0.21f * S, Fill);
		C.FillCircle(X + 0.5f * S, Y + 0.52f * S, 0.13f * S, Fill);
		C.FillPolygon({P(0.5f, 0.5f), P(0.64f, 1.0f), P(0.36f, 1.0f)}, Fill);
		break;
	case 1: // diamonds
		C.FillPolygon({P(0.5f, 0.0f), P(0.9f, 0.5f), P(0.5f, 1.0f), P(0.1f, 0.5f)}, Fill);
		break;
	case 2: // hearts
		C.FillCircle(X + 0.28f * S, Y + 0.3f * S, 0.25f * S, Fill);
		C.FillCircle(X + 0.72f * S, Y + 0.3f * S, 0.25f * S, Fill);
		C.FillPolygon({P(0.04f, 0.38f), P(0.96f, 0.38f), P(0.5f, 0.97f)}, Fill);
		break;
	default: // spades
		C.FillCircle(X + 0.28f * S, Y + 0.58f * S, 0.23f * S, Fill);
		C.FillCircle(X + 0.72f * S, Y + 0.58f * S, 0.23f * S, Fill);
		C.FillPolygon({P(0.06f, 0.53f), P(0.5f, 0.0f), P(0.94f, 0.53f)}, Fill);
		C.FillPolygon({P(0.5f, 0.58f), P(0.66f, 1.0f), P(0.34f, 1.0f)}, Fill);
		break;
	}
}

namespace ui_detail
{
/**
 * G2 fix (M5; §10: Ignite's table within 22k vertices at its busiest): a soft glow round a round rect with its core's corners
 * at two segments (the canvas's GlowPolygon), for the rival decks' shadows and highlights. The fringe (1.3 blurs wide) hides
 * the core's facets: 25 vertices where GlowRoundRect's curve takes 41-65. A core with no corner left after the inset (the
 * radius inside a third of the blur) is GlowRoundRect's own 9-vertex one. Review fix (review-g2f-tablestyle): shadows only
 * (a 6 blur); under a wide one (the winner's 18) the fringe fanned over 45-degree steps shows a ray along each diagonal.
 */
void TblLowGlow(Canvas& C, const Rect& R, float Radius, const Color& Col, float Blur)
{
	if (Radius <= Blur * 0.3f + 0.01f)
	{
		C.GlowRoundRect(R, Radius, Col, Blur);
		return;
	}
	C.GlowPolygon(Canvas::RoundRectPath(R, Radius, 2), Col, Blur);
}

/** A face: Low draws its shadow through TblLowGlow (a rival deck's, G2 fix M5); RiverLine's is today's. */
void CardFace(Canvas& C, int Card, float X, float Y, float W, float H, bool Low = false)
{
	if (Low)
	{
		TblLowGlow(C, {X, Y + 2.0f, W, H}, W * 0.1f, Rgba(0, 0, 0, 0.45f), 6.0f);
	}
	else
	{
		C.GlowRoundRect({X, Y + 2.0f, W, H}, W * 0.1f, Rgba(0, 0, 0, 0.45f), 6.0f);
	}
	C.FillRoundRect({X, Y, W, H}, W * 0.1f, Paint::Linear({0.0f, Y}, {0.0f, Y + H}, Hex(0xffffff), Hex(0xe9edf3)));
	C.StrokeRoundRect({X, Y, W, H}, W * 0.1f, Rgba(0, 0, 0, 0.18f), 1.0f);
	const int Suit = SuitOf(Card);
	const Color Col = SuitColor(Suit);
	const char R = RankChars[RankOf(Card)];
	// Glyph text only when the card is not being squashed by the flip animation.
	if (C.Transform().IsUniform())
	{
		const std::string Rank = R == 'T' ? std::string("10") : std::string(1, R);
		C.Text(Rank, X + W * 0.09f, Y + H * 0.05f, Ts(std::round(H * 0.36f), 800, Col, Align::Left, Baseline::Top));
	}
	const float Small = std::round(H * 0.3f);
	DrawSuit(C, Suit, X + W * 0.1f + Small * 0.04f, Y + H * 0.42f + Small * 0.14f, Small * 0.7f, Col);
	const float Big = std::round(H * 0.5f);
	const float Prev = C.GetAlpha();
	C.SetAlpha(Prev * 0.9f);
	DrawSuit(C, Suit, X + W * 0.95f - Big * 0.74f, Y + H * 0.98f - Big * 0.92f, Big * 0.7f, Col);
	C.SetAlpha(Prev);
}

// Update 3 (§2.5, §3.5, G1): the rival looks' backs and chips, in their own constants (never pal::, none a RiverLine hex).
const uint32_t TblCoral = 0xff6b57, TblPink = 0xff3d8b, TblViolet = 0x8b5cff, TblSky = 0x4fc3f7, TblLime = 0xc8ff4a, TblSun = 0xffc94a, TblCream = 0xfff4ec, TblGrape = 0x3a1f5c,
			   TblPlum = 0x1a1220;
const uint32_t TblOnyx = 0x0c0c0f, TblGloss = 0x202026, TblSignal = 0xd2141a, TblChrome = 0xb6bbc5, TblWhite = 0xffffff, TblCarbon = 0x16161a, TblStatusGold = 0xe6c66a, TblDeep = 0x7a0a12,
			   TblLacquerTop = 0xe3262c;
// G2 (ui-tablestyle): Ignite's Neon Grid back on Afterburner's night (its Panel and Bg, §2.3).
const uint32_t TblNightTop = 0x1a1230, TblNightLow = 0x120c1f;

/** Sutherland-Hodgman: a convex polygon clipped to a rect (the backs' bands and rays stay inside their panel). */
std::vector<Vec2> TblClip(const std::vector<Vec2>& Poly, const Rect& Box)
{
	std::vector<Vec2> Kept = Poly;
	for (int Side = 0; Side < 4 && !Kept.empty(); ++Side)
	{
		const auto IsIn = [&](const Vec2& Pt) {
			return Side == 0 ? Pt.X >= Box.X : Side == 1 ? Pt.X <= Box.X + Box.W : Side == 2 ? Pt.Y >= Box.Y : Pt.Y <= Box.Y + Box.H;
		};
		const auto Meet = [&](const Vec2& Pa, const Vec2& Pb) {
			float Tt = 0.0f;
			if (Side == 0) Tt = (Box.X - Pa.X) / (Pb.X - Pa.X);
			else if (Side == 1) Tt = (Box.X + Box.W - Pa.X) / (Pb.X - Pa.X);
			else if (Side == 2) Tt = (Box.Y - Pa.Y) / (Pb.Y - Pa.Y);
			else Tt = (Box.Y + Box.H - Pa.Y) / (Pb.Y - Pa.Y);
			return Vec2{Pa.X + (Pb.X - Pa.X) * Tt, Pa.Y + (Pb.Y - Pa.Y) * Tt};
		};
		const std::vector<Vec2> From = Kept;
		Kept.clear();
		for (size_t I = 0; I < From.size(); ++I)
		{
			const Vec2& Pa = From[I];
			const Vec2& Pb = From[(I + 1) % From.size()];
			if (IsIn(Pb))
			{
				if (!IsIn(Pa))
				{
					Kept.push_back(Meet(Pa, Pb));
				}
				Kept.push_back(Pb);
			}
			else if (IsIn(Pa))
			{
				Kept.push_back(Meet(Pa, Pb));
			}
		}
	}
	return Kept;
}

/** Ignite's flame (the 24-point mark of siteart::Logo, simplified): its base at (Cx, Base), S tall. */
void TblFlame(Canvas& C, float Cx, float Base, float S, const Color& Outer, const Color& Inner)
{
	const auto P = [&](float U, float V) { return Vec2{Cx + U * S * 0.7f, Base - V * S}; };
	C.FillPolygon({P(0.0f, 0.0f), P(-0.62f, 0.32f), P(-0.5f, 0.62f), P(-0.3f, 0.78f), P(-0.26f, 0.58f), P(-0.06f, 1.0f), P(0.26f, 0.7f), P(0.36f, 0.86f), P(0.62f, 0.4f), P(0.48f, 0.1f)}, Outer);
	C.FillPolygon({P(0.0f, 0.04f), P(-0.3f, 0.24f), P(-0.16f, 0.52f), P(0.02f, 0.62f), P(0.12f, 0.44f), P(0.3f, 0.3f), P(0.22f, 0.08f)}, Inner);
}

/**
 * Ignite's looks as the table wears them (G2, §2.4 L's Locker; core-siteplay's site::play::LookOf): a look's design (its base:
 * a card back's 0 Sunset, 1 Neon Grid; a chip set's 0 candy, 1 Arcade Tokens) and its two Wristband colours (0: its own).
 */
struct TblWorn
{
	int Base = 0;
	uint32_t A = 0, B = 0;
};

TblWorn TblWornLook(int Slot, int Look)
{
	TblWorn W;
	if (Look >= 0 && Look < site::play::LookCount(Slot))
	{
		const site::play::LookView V = site::play::LookOf(Slot, Look);
		W.Base = V.Base;
		W.A = V.ColorA;
		W.B = V.ColorB;
	}
	return W;
}

/**
 * The rivals' backs (Back 1 Ignite's Sunset, 2 RunItTwice's Twin Stripe, 3 its lacquer variant, 4 Ignite's Neon Grid; G2:
 * SitesIgBackLooks + n, Ignite's card back look n from the Locker, its design and its Wristband colours), drawn around (X, Y).
 */
void TblBack(Canvas& C, float X, float Y, float W, float H, int Back)
{
	const float Rad = W * 0.1f;
	const bool Lod = W <= 58.0f; // small: ground, stripes or bands, border
	TblWorn Worn;
	if (Back >= 64)
	{
		Worn = TblWornLook(0, Back - 64);
		Back = Worn.Base == 1 ? 4 : 1;
	}
	if (Back == 4)
	{
		// Neon Grid (Ignite's second back, G2; core-siteplay's name): the cream border round a night panel, a neon sun on the
		// horizon cut by three night bars, and a hot pink grid running to it. A month's colours (not in the G2 catalog) would
		// light the grid in the first and the sun in the second.
		C.FillRoundRect({X, Y, W, H}, Rad, Hex(TblCream));
		const Rect In{X + W * 0.07f, Y + W * 0.07f, W - W * 0.14f, H - W * 0.14f};
		C.FillRoundRect(In, W * 0.06f, Paint::Linear({In.X, In.Y}, {In.X + In.W, In.Y + In.H}, Hex(TblNightTop), Hex(TblNightLow)));
		const float Hz = In.Y + In.H * 0.46f;
		const float Bottom = In.Y + In.H;
		const float Cx = In.X + In.W / 2.0f;
		const Color Grid = Worn.A ? Hex(Worn.A) : Hex(TblPink);
		const float Lw = std::max(1.0f, W * 0.012f);
		if (!Lod)
		{
			// The sun: a half disc on the horizon, sunflower to coral, three night bars across its lower half.
			const float Rs = In.W * 0.3f;
			std::vector<Vec2> Sun;
			for (int K = 0; K <= 16; ++K)
			{
				const float A = Pi + Pi * static_cast<float>(K) / 16.0f;
				Sun.push_back({Cx + Rs * std::cos(A), Hz + Rs * std::sin(A)});
			}
			C.FillPolygon(Sun, Paint::Linear({Cx, Hz - Rs}, {Cx, Hz}, Worn.B ? Hex(Worn.B) : Hex(TblSun), Worn.B ? Hex(Worn.B) : Hex(TblCoral)));
			for (int K = 0; K < 3; ++K)
			{
				const float By = Hz - Rs * (0.12f + 0.16f * static_cast<float>(K));
				C.FillRect({Cx - Rs, By, 2.0f * Rs, Rs * (0.07f - 0.015f * static_cast<float>(K))}, Hex(TblNightLow));
			}
			// The grid: rows closing toward the horizon, and lines from its middle (each cut at the panel's sides).
			for (int K = 1; K <= 5; ++K)
			{
				const float T = static_cast<float>(K) / 5.0f;
				const float Ry = Hz + (Bottom - Hz) * T * T;
				C.StrokePolyline({{In.X, Ry}, {In.X + In.W, Ry}}, false, Color{Grid.R, Grid.G, Grid.B, 0.75f}, Lw);
			}
			for (int K = -3; K <= 3; ++K)
			{
				const float Dx = In.W * 0.34f * static_cast<float>(K);
				const float T = std::fabs(Dx) > In.W / 2.0f ? In.W / 2.0f / std::fabs(Dx) : 1.0f;
				C.StrokePolyline({{Cx, Hz}, {Cx + Dx * T, Hz + (Bottom - Hz) * T}}, false, Color{Grid.R, Grid.G, Grid.B, 0.75f}, Lw);
			}
		}
		else
		{
			for (int K = 2; K <= 4; K += 2)
			{
				const float T = static_cast<float>(K) / 5.0f;
				C.FillRect({In.X, Hz + (Bottom - Hz) * T * T, In.W, Lw}, Color{Grid.R, Grid.G, Grid.B, 0.75f});
			}
		}
		C.FillRect({In.X, Hz - Lw * 0.75f, In.W, Lw * 1.5f}, Grid);
		return;
	}
	if (Back == 1)
	{
		// Sunset (§2.5): a cream border around a plum panel, sunburst rays at 0.18, a coral and a violet diagonal band, the flame.
		// A Wristband month's card back (G2: Sunset restyled) paints the band pair in its two colours.
		C.FillRoundRect({X, Y, W, H}, Rad, Hex(TblCream));
		const Rect In{X + W * 0.07f, Y + W * 0.07f, W - W * 0.14f, H - W * 0.14f};
		C.FillRoundRect(In, W * 0.06f, Paint::Linear({In.X, In.Y}, {In.X + In.W, In.Y + In.H}, Hex(TblGrape), Hex(TblPlum)));
		const Vec2 Mid{In.X + In.W / 2.0f, In.Y + In.H / 2.0f};
		if (!Lod)
		{
			const float Reach = In.W + In.H;
			for (int K = 0; K < 12; ++K)
			{
				const float A = static_cast<float>(K) * Pi / 6.0f;
				const std::vector<Vec2> Ray = TblClip({Mid, {Mid.X + Reach * std::cos(A - 0.09f), Mid.Y + Reach * std::sin(A - 0.09f)}, {Mid.X + Reach * std::cos(A + 0.09f), Mid.Y + Reach * std::sin(A + 0.09f)}}, In);
				if (Ray.size() >= 3)
				{
					C.FillPolygon(Ray, Rgba(255, 244, 236, 0.18f));
				}
			}
		}
		// The band pair, bottom-left to top-right.
		const float Bw = In.W * 0.22f;
		const float Lean = In.H * 0.55f;
		const std::vector<Vec2> First = TblClip({{Mid.X - Bw * 1.1f - Lean, In.Y + In.H}, {Mid.X - Bw * 0.1f - Lean, In.Y + In.H}, {Mid.X - Bw * 0.1f + Lean, In.Y}, {Mid.X - Bw * 1.1f + Lean, In.Y}}, In);
		const std::vector<Vec2> Second = TblClip({{Mid.X + Bw * 0.1f - Lean, In.Y + In.H}, {Mid.X + Bw * 0.7f - Lean, In.Y + In.H}, {Mid.X + Bw * 0.7f + Lean, In.Y}, {Mid.X + Bw * 0.1f + Lean, In.Y}}, In);
		// Sunset's first band runs coral to pink; a month's is flat in its first colour, the second band in its second.
		const Color A0 = Worn.A ? Hex(Worn.A) : Hex(TblCoral);
		const Color A1 = Worn.A ? A0 : Hex(TblPink);
		const Color B0 = Worn.B ? Hex(Worn.B) : Hex(TblViolet);
		if (First.size() >= 3)
		{
			C.FillPolygon(First, Paint::Linear({In.X, In.Y + In.H}, {In.X + In.W, In.Y}, A0, A1));
		}
		if (Second.size() >= 3)
		{
			C.FillPolygon(Second, B0);
		}
		if (!Lod)
		{
			C.FillCircle(Mid.X, Mid.Y, W * 0.2f, Hex(TblPlum));
			TblFlame(C, Mid.X, Mid.Y + W * 0.13f, W * 0.26f, Hex(TblCream), Hex(TblCoral));
		}
		return;
	}
	// The Twin Stripe (§3.5): a gloss-black ground (Back 3: red lacquer), two vertical stripes off-centre (16% and 6% of the
	// width, a 4% gap, from 30% across), the turnstone mark in white, a 1-unit chrome border inset 4%.
	const bool Lacquer = Back == 3;
	if (Lacquer)
	{
		C.FillRoundRect({X, Y, W, H}, Rad, Paint::Linear({X, Y}, {X, Y + H}, Hex(TblLacquerTop), Hex(TblDeep)));
	}
	else
	{
		C.FillRoundRect({X, Y, W, H}, Rad, Paint::Linear({X, Y}, {X + W, Y + H}, Hex(TblGloss), Hex(TblOnyx)));
	}
	const Color Stripe = Lacquer ? Hex(TblOnyx) : Hex(TblSignal);
	C.FillRect({X + W * 0.30f, Y + 0.5f, W * 0.16f, H - 1.0f}, Stripe);
	C.FillRect({X + W * 0.50f, Y + 0.5f, W * 0.06f, H - 1.0f}, Stripe);
	if (!Lod)
	{
		const float S = W * 0.32f;
		const float Mx = X + W * 0.5f - S * 0.36f;
		const float My = Y + H * 0.5f - S * 0.36f;
		C.FillRoundRect({Mx, My + S * 0.16f, S * 0.40f, S * 0.56f}, S * 0.04f, Hex(TblWhite));
		C.FillRoundRect({Mx + S * 0.30f, My, S * 0.40f, S * 0.56f}, S * 0.04f, Hex(TblWhite));
		C.StrokeRoundRect({Mx + S * 0.30f, My, S * 0.40f, S * 0.56f}, S * 0.04f, Hex(TblOnyx), std::max(1.0f, S * 0.035f));
	}
	C.StrokeRoundRect({X + W * 0.04f, Y + W * 0.04f, W - W * 0.08f, H - W * 0.08f}, Rad * 0.6f, Rgba(182, 187, 197, 0.8f), 1.0f);
}

/** A rival back's art at (X, Y) without its shadow, cached by back and size (its own constants: no skin in the key). */
void TblBackCached(Canvas& C, float X, float Y, float W, float H, int Back)
{
	uint32_t Wb = 0;
	uint32_t Hb = 0;
	std::memcpy(&Wb, &W, sizeof(Wb));
	std::memcpy(&Hb, &H, sizeof(Hb));
	const uint64_t Key = (0x7b1ac5e3d9f20000ull ^ (static_cast<uint64_t>(Back) << 8)) * 0x100000001b3ull ^ (static_cast<uint64_t>(Wb) << 32 | Hb);
	C.Cached(Key, X, Y, [W, H, Back](Canvas& Cv) { TblBack(Cv, 0.0f, 0.0f, W, H, Back); });
}

/**
 * G2 fix (M5): a rival back with no shadow, turned Rotate about its centre as DrawCard turns one: a crowded felt's small backs
 * (RiverLine.cpp's TblHoleCards, with four or more stickers up), 25 vertices fewer each.
 */
void TblBackBare(Canvas& C, float X, float Y, float W, float H, float Rotate, int Back)
{
	C.Save();
	C.Translate(X + W / 2.0f, Y + H / 2.0f);
	if (Rotate != 0.0f)
	{
		C.Rotate(Rotate);
	}
	TblBackCached(C, -W / 2.0f, -H / 2.0f, W, H, Back);
	C.Restore();
}

void CardBack(Canvas& C, float X, float Y, float W, float H, int Back)
{
	// Update 3 (§2.5, §3.5): a site look's back (Back > 0: Ignite's Sunset, RunItTwice's Twin Stripe and its lacquer variant),
	// cached by back and size; 0 is today's.
	if (Back > 0)
	{
		TblLowGlow(C, {X, Y + 2.0f, W, H}, W * 0.1f, Rgba(0, 0, 0, 0.45f), 6.0f); // G2 fix M5: the rival decks' shadow
		TblBackCached(C, X, Y, W, H, Back);
		return;
	}
	C.GlowRoundRect({X, Y + 2.0f, W, H}, W * 0.1f, Rgba(0, 0, 0, 0.45f), 6.0f);
	C.FillRoundRect({X, Y, W, H}, W * 0.1f, Hex(0xf4f6fa));
	const Rect In{X + W * 0.07f, Y + W * 0.07f, W - W * 0.14f, H - W * 0.14f};
	// Update 2 (§1.8): a rival site's deck wears that site's colours (its accents, darkened); RiverLine's teal back is
	// exactly as it was whenever pal:: holds RiverLine's accent (outside any rival's SkinScope).
	const Color HomeAccent = Hex(0x27d3c3);
	const bool Home = pal::Accent.R == HomeAccent.R && pal::Accent.G == HomeAccent.G && pal::Accent.B == HomeAccent.B;
	const Color BackTop = Home ? Hex(0x0e7c72) : Mix(pal::Accent, Hex(0x000000), 0.5f);
	const Color BackBottom = Home ? Hex(0x0b3f5c) : Mix(pal::Accent2, Hex(0x000000), 0.62f);
	C.FillRoundRect(In, W * 0.06f, Paint::Linear({X, Y}, {X + W, Y + H}, BackTop, BackBottom));
	// Diagonal hatching, clipped to the inner panel analytically.
	for (float I = -H; I < W + H; I += 9.0f)
	{
		// Line from (X + I, Y) to (X + I + H, Y + H), clipped to In.
		float T0 = 0.0f;
		float T1 = 1.0f;
		const float Dx = H;
		const float Dy = H;
		const float Px = X + I;
		const float Py = Y;
		auto ClipT = [&](float P, float Q) {
			if (P == 0.0f)
			{
				return Q >= 0.0f;
			}
			const float T = Q / P;
			if (P < 0.0f)
			{
				T0 = T > T0 ? T : T0;
			}
			else
			{
				T1 = T < T1 ? T : T1;
			}
			return T0 <= T1;
		};
		if (ClipT(-Dx, Px - In.X) && ClipT(Dx, In.X + In.W - Px) && ClipT(-Dy, Py - In.Y) && ClipT(Dy, In.Y + In.H - Py) && T1 > T0)
		{
			C.StrokePolyline({{Px + Dx * T0, Py + Dy * T0}, {Px + Dx * T1, Py + Dy * T1}}, false, Rgba(255, 255, 255, 0.12f), 2.0f);
		}
	}
	// River wave mark.
	std::vector<Vec2> Wave;
	const float Cy = Y + H / 2.0f;
	for (float Px = W * 0.25f; Px <= W * 0.75f; Px += 1.0f)
	{
		Wave.push_back({X + Px, Cy + std::sin(((Px - W * 0.25f) / (W * 0.5f)) * Pi * 2.0f) * H * 0.06f});
	}
	C.StrokePolyline(Wave, false, Rgba(255, 255, 255, 0.75f), W * 0.05f > 2.0f ? W * 0.05f : 2.0f);
}
} // namespace ui_detail

void DrawCard(Canvas& C, int Card, float X, float Y, float W, float H, const CardOpts& Opts)
{
	const bool ShowFace = Card >= 0 && Opts.Flip >= 0.5f;
	float Sx = std::fabs(std::cos(Opts.Flip * Pi));
	Sx = Sx < 0.02f ? 0.02f : Sx;
	C.Save();
	C.SetAlpha(C.GetAlpha() * Opts.Alpha);
	C.Translate(X + W / 2.0f, Y + H / 2.0f);
	if (Opts.Rotate != 0.0f)
	{
		C.Rotate(Opts.Rotate);
	}
	C.Scale(Sx, 1.0f);
	// G2 fix (M5): a rival deck (Back > 0, the rival tables' every card) draws its shadow through TblLowGlow; RiverLine's
	// cards (Back 0) are today's. Review fix (review-g2f-tablestyle): the winner's glow stays at the canvas's curve on every
	// deck (an 18 blur over two segments a corner lit a ray along each corner's diagonal; RiverLine.cpp's TblRunGlow).
	const bool Rival = Opts.Back > 0;
	if (Opts.Highlight)
	{
		// The winner's glow in the skin's gold (fix2-auto: RiverLine's own f2c14e at home, the same floats as Rgba(242, 193, 78);
		// a rival's Gold token at its table, never RiverLine's gold there, §9.6).
		C.GlowRoundRect({-W / 2.0f - 4.0f, -H / 2.0f - 4.0f, W + 8.0f, H + 8.0f}, W * 0.12f, Color{pal::Gold.R, pal::Gold.G, pal::Gold.B, 0.9f}, 18.0f);
	}
	if (ShowFace)
	{
		CardFace(C, Card, -W / 2.0f, -H / 2.0f, W, H, Rival);
	}
	else
	{
		CardBack(C, -W / 2.0f, -H / 2.0f, W, H, Opts.Back);
	}
	if (Opts.Dim)
	{
		C.FillRoundRect({-W / 2.0f, -H / 2.0f, W, H}, W * 0.1f, Rgba(8, 12, 20, 0.55f));
	}
	C.Restore();
}

namespace ui_detail
{
void ChipColors(Chips Value, Color& Face, Color& Edge)
{
	if (Value >= 100000) { Face = Hex(0xe8e8ee); Edge = Hex(0x8c3cd6); }
	else if (Value >= 25000) { Face = Hex(0xf2c14e); Edge = Hex(0x6b4a10); }
	else if (Value >= 5000) { Face = Hex(0xff6f91); Edge = Hex(0x7a1030); }
	else if (Value >= 1000) { Face = Hex(0xf5f5f5); Edge = Hex(0x222222); }
	else if (Value >= 500) { Face = Hex(0x8c52ff); Edge = Hex(0x2a0f66); }
	else if (Value >= 100) { Face = Hex(0x1d1f26); Edge = Hex(0xe6e6e6); }
	else if (Value >= 25) { Face = Hex(0x2fbf71); Edge = Hex(0x0c3d22); }
	else { Face = Hex(0xe84a5f); Edge = Hex(0xffffff); }
}

/** A stack's columns, as today's (up to three denominations, up to eight chips a column): (denomination's index, chips). */
std::vector<std::pair<int, int>> TblStackColumns(Chips Amount)
{
	static const Chips TblDenoms[8] = {100000, 25000, 5000, 1000, 500, 100, 25, 5};
	Chips Rem = Amount;
	std::vector<std::pair<int, int>> Cols;
	for (int K = 0; K < 8; ++K)
	{
		const Chips N = Rem / TblDenoms[K];
		if (N > 0)
		{
			Cols.push_back({K, static_cast<int>(N < 8 ? N : 8)});
			Rem -= N * TblDenoms[K];
		}
		if (Cols.size() >= 3)
		{
			break;
		}
	}
	if (Cols.empty())
	{
		Cols.push_back({7, 1});
	}
	return Cols;
}

/** A point on a chip's rim: the ellipse (Rx, Ry) round (Cx, Cy) at angle A (0 right, Pi/2 the front, Pi left). */
Vec2 TblRim(float Cx, float Cy, float Rx, float Ry, float A)
{
	return {Cx + Rx * std::cos(A), Cy + Ry * std::sin(A)};
}

/** A rim's front half (angle 0 to Pi) in Segs segments; Close adds nothing (the caller closes the shape). */
void TblFront(std::vector<Vec2>& Out, float Cx, float Cy, float Rx, float Ry, int Segs)
{
	for (int K = 0; K <= Segs; ++K)
	{
		Out.push_back(TblRim(Cx, Cy, Rx, Ry, Pi * static_cast<float>(K) / static_cast<float>(Segs)));
	}
}

/** A whole rim in N points. */
std::vector<Vec2> TblRing(float Cx, float Cy, float Rx, float Ry, int N)
{
	std::vector<Vec2> Out;
	for (int K = 0; K < N; ++K)
	{
		Out.push_back(TblRim(Cx, Cy, Rx, Ry, 2.0f * Pi * static_cast<float>(K) / static_cast<float>(N)));
	}
	return Out;
}

/**
 * One column of a rival's stack, merged (G1_AUDIT N12), its bottom chip centred on the origin: a shadow, the side as one shape
 * in the chip's ground shaded, a seam at every chip's edge, the top chip's face and its marks. Detail 0 is the full look; 1
 * (a busy felt) fewer points, the front stripes or marks only and no inner ring; 2 (the busiest) the side, the face and its
 * front marks, no shadow or seams. Vertices (TblColumnVertices): candy 180 + 20 a chip past the first at detail 0, 101 + 16 at
 * 1, 63 at 2; lacquer 163 + 20, 79 + 16, 52; tokens 120 + 20, 79 + 16, 64 (a chip-by-chip column was ~224 a chip), so nine
 * stacks of three columns fit a felt's 1,800 at detail 2 in any set. Look (G2) is an Ignite chip set look + 1 from the
 * Locker (0: none): a Wristband month's two colours would restyle candy (the stripes in the first, the inner ring in the
 * second), the denominations' own grounds kept (core-siteplay's catalog has no monthly chip set at G2).
 */
void TblColumn(Canvas& C, int Set, int Di, int N, float Scale, int Detail, int Look)
{
	static const uint32_t Candy[8][2] = {{TblGrape, TblSun}, {TblCream, TblCoral}, {TblPink, TblCream}, {TblSun, 0x5a3a00}, {TblLime, 0x2a3a10}, {TblViolet, TblCream}, {TblSky, TblCream},
		{TblCoral, TblCream}};
	static const uint32_t Lacquer[5][2] = {{TblCarbon, TblWhite}, {TblChrome, TblOnyx}, {TblSignal, TblWhite}, {TblWhite, TblOnyx}, {TblStatusGold, TblOnyx}};
	// Set 3 (Ignite's Arcade Tokens, G2's second chip set): struck metal tokens, flat, a stamped rim and a star in the middle,
	// one metal a denomination (100k violet anodised, 25k black and gold, 5k rose gold, 1k silver, 500 nickel, 100 brass, 25
	// verdigris, 5 copper).
	static const uint32_t Tokens[8][2] = {{0x8a63c9, 0xe9e2f7}, {0x2b2b30, 0xe0b24a}, {0xe7a093, 0x8a4f43}, {0xdfe3e8, 0x7b828c}, {0x9fb4d6, 0x4c5d78}, {0xd1a646, 0x7a5a18},
		{0x6fae9a, 0x2f5a4c}, {0xc56b45, 0x6e3220}};
	const bool Token = Set == 3;
	const TblWorn Worn = Look > 0 ? TblWornLook(2, Look - 1) : TblWorn{};
	const uint32_t* Pair = Set == 1 ? Candy[Di] : Token ? Tokens[Di] : Lacquer[(7 - Di) % 5];
	const Color Ground = Hex(Pair[0]);
	const Color Mark = Set == 1 && Worn.A ? Hex(Worn.A) : Hex(Pair[1]);
	const Color Ring = Set == 1 && Worn.B ? Hex(Worn.B) : Mark;
	const Color Black = Hex(0x000000);
	const float Rr = 13.0f * Scale;
	const float Ry = Rr * 0.55f;
	const float Step = 3.2f * Scale;
	const float Lean = Set == 1 ? Scale : 0.0f;
	const float Tx = Lean * static_cast<float>(N - 1);
	const float Ty = -Step * static_cast<float>(N - 1);
	const int Arc = Detail == 0 ? 6 : 4;
	std::vector<Vec2> P;
	if (Detail < 2)
	{
		// The shadow: the bottom rim's front half, two units down (the side covers its top).
		TblFront(P, 0.0f, 2.0f * Scale, Rr, Ry, Arc);
		C.FillPolygon(P, Rgba(0, 0, 0, 0.35f));
	}
	if (N > 1)
	{
		// The side: the bottom rim's front half up to the top chip's rim ends, shaded.
		P.clear();
		TblFront(P, 0.0f, 0.0f, Rr, Ry, Arc);
		P.push_back(TblRim(Tx, Ty, Rr, Ry, Pi));
		P.push_back(TblRim(Tx, Ty, Rr, Ry, 0.0f));
		C.FillPolygon(P, Mix(Ground, Black, Token ? 0.32f : 0.22f));
		if (Set == 1 && Detail < 2)
		{
			// Candy's edge stripes down the side (a sixth of the rim apart, 20 degrees wide), cut into chips by the seams.
			const Color Side = Mix(Mark, Black, 0.22f);
			for (int K = 0; K < 3; ++K)
			{
				const float Mid = (15.0f + 60.0f * static_cast<float>(K)) * Pi / 180.0f;
				const float H = 10.0f * Pi / 180.0f;
				C.FillPolygon({TblRim(0.0f, 0.0f, Rr, Ry, Mid - H), TblRim(0.0f, 0.0f, Rr, Ry, Mid + H), TblRim(Tx, Ty, Rr, Ry, Mid + H), TblRim(Tx, Ty, Rr, Ry, Mid - H)}, Side);
			}
		}
		if (Detail < 2)
		{
			// A seam at every chip's edge (its shadow on the chip below).
			const Color Seam = Rgba(0, 0, 0, Token ? 0.4f : 0.32f);
			for (int I = 1; I < N; ++I)
			{
				P.clear();
				TblFront(P, Lean * static_cast<float>(I), -Step * static_cast<float>(I) + Scale, Rr, Ry, Detail == 0 ? 4 : 3);
				C.StrokePolyline(P, false, Seam, 1.4f * Scale);
			}
		}
	}
	// The top chip's face.
	C.FillPolygon(TblRing(Tx, Ty, Rr, Ry, Detail == 0 ? 12 : Token && Detail == 2 ? 8 : 10), Ground);
	if (Set == 1)
	{
		// Six stripes round the edge (the front three on a busy felt), from the rim to 0.7 of the way in; the inner ring.
		for (int K = 0; K < 6; ++K)
		{
			const float Mid = (15.0f + 60.0f * static_cast<float>(K)) * Pi / 180.0f;
			if (Detail > 0 && Mid > Pi)
			{
				continue;
			}
			const float H = 10.0f * Pi / 180.0f;
			C.FillPolygon({TblRim(Tx, Ty, Rr, Ry, Mid - H), TblRim(Tx, Ty, Rr, Ry, Mid + H), TblRim(Tx, Ty, 0.7f * Rr, 0.7f * Ry, Mid + H), TblRim(Tx, Ty, 0.7f * Rr, 0.7f * Ry, Mid - H)}, Mark);
		}
		if (Detail == 0)
		{
			C.StrokePolyline(TblRing(Tx, Ty, Rr * 0.62f, Rr * 0.34f, 10), true, Color{Ring.R, Ring.G, Ring.B, 0.7f}, 1.0f);
		}
		return;
	}
	if (Token)
	{
		// The stamped rim and, at full detail, the star struck in the middle.
		C.StrokePolyline(TblRing(Tx, Ty, Rr * 0.8f, Ry * 0.8f, Detail == 0 ? 10 : 8), true, Color{Mark.R, Mark.G, Mark.B, 0.8f}, 1.2f * Scale);
		if (Detail == 0)
		{
			std::vector<Vec2> Star;
			for (int K = 0; K < 10; ++K)
			{
				const float A = -Pi / 2.0f + Pi * static_cast<float>(K) / 5.0f;
				const float Rs = (K % 2 == 0 ? 0.42f : 0.18f) * Rr;
				Star.push_back({Tx + Rs * std::cos(A), Ty + Rs * 0.55f * std::sin(A)});
			}
			C.FillPolygon(Star, Color{Mark.R, Mark.G, Mark.B, 0.85f});
		}
		return;
	}
	// Lacquer: eight fine edge marks in the contrasting colour (four on a busy felt, the front two on the busiest), the inner
	// ring at full detail.
	for (int M = 0; M < (Detail == 2 ? 4 : 8); M += Detail == 0 ? 1 : 2)
	{
		const float A = (static_cast<float>(M) * 45.0f + 22.5f) * Pi / 180.0f;
		C.StrokePolyline({TblRim(Tx, Ty, Rr * 0.8f, Rr * 0.44f, A), TblRim(Tx, Ty, Rr * 0.98f, Rr * 0.54f, A)}, false, Mark, std::max(1.2f, 2.2f * Scale));
	}
	if (Detail == 0)
	{
		C.StrokePolyline(TblRing(Tx, Ty, Rr * 0.62f, Rr * 0.34f, 10), true, Color{Mark.R, Mark.G, Mark.B, 0.45f}, 0.8f);
	}
}

/**
 * The rivals' stacks (§2.5, §3.5), up to three columns of up to eight as today's. Candy (Set 1): 5 coral, 25 sky, 100 violet,
 * 500 lime, 1k sunflower, 5k pink, 25k cream with coral stripes, 100k grape with sunflower; six edge stripes; a stack leans one
 * unit a chip. Lacquer (Set 2): Carbon, Chrome, Signal Red, Hot White, Status Gold in turn up the denominations, flat, the top
 * chip with eight fine edge marks in the contrasting colour. Arcade Tokens (Set 3, Ignite's second chip set, G2): struck metal
 * tokens with a stamped rim and a star. G1_AUDIT N12: each column is one merged shape (TblColumn), replayed from the canvas's
 * cache by set, denomination, height, size, detail and look; Detail (0-2) is the felt's (RiverLine.cpp's TblChipDetail), Look
 * an Ignite chip set look + 1 (G2; 0 none).
 */
void TblChipStack(Canvas& C, Chips Amount, float X, float Y, float Scale, float Alpha, int Set, int Detail, int Look)
{
	const std::vector<std::pair<int, int>> Cols = TblStackColumns(Amount);
	C.Save();
	C.SetAlpha(C.GetAlpha() * Alpha);
	const float Rr = 13.0f * Scale;
	uint32_t Sb = 0;
	std::memcpy(&Sb, &Scale, sizeof(Sb));
	for (size_t Si = 0; Si < Cols.size(); ++Si)
	{
		const float Sx = X + (static_cast<float>(Si) - static_cast<float>(Cols.size() - 1) / 2.0f) * Rr * 2.1f;
		const int Di = Cols[Si].first;
		const int N = Cols[Si].second;
		const uint64_t Key = (0x3c41f7d2b98e0000ull ^ static_cast<uint64_t>(Look << 16 | Set << 12 | Di << 8 | N << 4 | Detail)) * 0x100000001b3ull ^ (static_cast<uint64_t>(Sb) << 24);
		C.Cached(Key, Sx, Y, [Set, Di, N, Scale, Detail, Look](Canvas& Cv) { TblColumn(Cv, Set, Di, N, Scale, Detail, Look); });
	}
	C.Restore();
}

/** TblColumn's vertices (a polygon of n points is 2n + 1, a line of n points 4n): any denomination, any size. */
int TblColumnVertices(int Set, int N, int Detail)
{
	const int Side = N > 1 ? 1 : 0;
	if (Detail >= 2)
	{
		return Side * 15 + (Set == 1 ? 21 + 27 : Set == 3 ? 17 + 32 : 21 + 16);
	}
	const bool Full = Detail == 0;
	const int Shadow = Full ? 15 : 11;
	const int Body = (Full ? 19 : 15) + (Set == 1 ? 27 : 0) + (N - 1) * (Full ? 20 : 16);
	const int Face = Full ? 25 : 21;
	const int Marks = Set == 1 ? (Full ? 54 + 40 : 27) : Set == 3 ? (Full ? 40 + 21 : 32) : (Full ? 64 + 40 : 32);
	return Shadow + Side * Body + Face + Marks;
}
} // namespace ui_detail

namespace rlsites_detail
{
// Declared in RiverLineShared.h (the shared table's felt budget, G1_AUDIT N12).
int SitesChipVertices(Chips Amount, int Set, int Detail);

int SitesChipVertices(Chips Amount, int Set, int Detail)
{
	if (Amount <= 0)
	{
		return 0;
	}
	int V = 0;
	for (const std::pair<int, int>& Col : ui_detail::TblStackColumns(Amount))
	{
		V += ui_detail::TblColumnVertices(Set & 15, Col.second, Detail);
	}
	return V;
}
} // namespace rlsites_detail

void DrawChipStack(Canvas& C, Chips Amount, float X, float Y, float Scale, float Alpha, int Set)
{
	if (Amount <= 0)
	{
		return;
	}
	// Update 3 (§2.5, §3.5): a site look's chips (Set 1: Ignite's candy, 2: RunItTwice's lacquer, 3: Ignite's Neon); 0 is today's.
	// G1_AUDIT N12: bits 4-5 carry the felt's detail; G2: bits 8-15 an Ignite chip set look + 1 (RiverLineShared.h,
	// SitesChipVertices and SitesIgChipSet).
	if (Set > 0)
	{
		TblChipStack(C, Amount, X, Y, Scale, Alpha, Set & 15, (Set >> 4) & 3, (Set >> 8) & 0xff);
		return;
	}
	static const Chips Denoms[8] = {100000, 25000, 5000, 1000, 500, 100, 25, 5};
	Chips Rem = Amount;
	std::vector<std::pair<Chips, int>> Stacks;
	for (const Chips D : Denoms)
	{
		const Chips N = Rem / D;
		if (N > 0)
		{
			Stacks.push_back({D, static_cast<int>(N < 8 ? N : 8)});
			Rem -= N * D;
		}
		if (Stacks.size() >= 3)
		{
			break;
		}
	}
	if (Stacks.empty())
	{
		Stacks.push_back({5, 1});
	}
	C.Save();
	C.SetAlpha(C.GetAlpha() * Alpha);
	const float R = 13.0f * Scale;
	for (size_t Si = 0; Si < Stacks.size(); ++Si)
	{
		const float Sx = X + (static_cast<float>(Si) - static_cast<float>(Stacks.size() - 1) / 2.0f) * R * 2.1f;
		Color Face;
		Color Edge;
		ChipColors(Stacks[Si].first, Face, Edge);
		for (int I = 0; I < Stacks[Si].second; ++I)
		{
			const float Sy = Y - static_cast<float>(I) * 3.2f * Scale;
			C.FillEllipse(Sx, Sy + 2.0f * Scale, R, R * 0.55f, Rgba(0, 0, 0, 0.35f));
			C.FillEllipse(Sx, Sy, R, R * 0.55f, Face);
			C.StrokeEllipse(Sx, Sy, R, R * 0.55f, Edge, 2.2f * Scale, 4.0f * Scale);
		}
	}
	C.Restore();
}
} // namespace ui
} // namespace ss
