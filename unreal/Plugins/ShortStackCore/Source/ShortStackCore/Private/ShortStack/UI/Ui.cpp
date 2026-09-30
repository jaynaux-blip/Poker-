#include "ShortStack/UI/Ui.h"
#include "../StrictFloat.h"

#include "ShortStack/Cards.h"

#include <cmath>

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
void CardFace(Canvas& C, int Card, float X, float Y, float W, float H)
{
	C.GlowRoundRect({X, Y + 2.0f, W, H}, W * 0.1f, Rgba(0, 0, 0, 0.45f), 6.0f);
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

void CardBack(Canvas& C, float X, float Y, float W, float H)
{
	C.GlowRoundRect({X, Y + 2.0f, W, H}, W * 0.1f, Rgba(0, 0, 0, 0.45f), 6.0f);
	C.FillRoundRect({X, Y, W, H}, W * 0.1f, Hex(0xf4f6fa));
	const Rect In{X + W * 0.07f, Y + W * 0.07f, W - W * 0.14f, H - W * 0.14f};
	C.FillRoundRect(In, W * 0.06f, Paint::Linear({X, Y}, {X + W, Y + H}, Hex(0x0e7c72), Hex(0x0b3f5c)));
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
	if (Opts.Highlight)
	{
		C.GlowRoundRect({-W / 2.0f - 4.0f, -H / 2.0f - 4.0f, W + 8.0f, H + 8.0f}, W * 0.12f, Rgba(242, 193, 78, 0.9f), 18.0f);
	}
	if (ShowFace)
	{
		CardFace(C, Card, -W / 2.0f, -H / 2.0f, W, H);
	}
	else
	{
		CardBack(C, -W / 2.0f, -H / 2.0f, W, H);
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
} // namespace ui_detail

void DrawChipStack(Canvas& C, Chips Amount, float X, float Y, float Scale, float Alpha)
{
	if (Amount <= 0)
	{
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
