// The Lucky Penny #212's counter: the shelves, the clerk, the receipt and the bag.
#include "ShortStack/UI/StoreCounter.h"
#include "../StrictFloat.h"
#include "FrontEndShared.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace ss
{
namespace ui
{
namespace counter_detail
{
const Color Paper = Hex(0xf3efe6);
const Color PaperInk = Hex(0x2b2723);
const Color Copper = Hex(0xd08a4e);
const Color Mint = Hex(0x3ecf8e);
const float Margin = 96.0f;
const float ReceiptW = 440.0f;

float Cf(int V)
{
	return static_cast<float>(V);
}

Color Fade(Color C, float A)
{
	C.A *= A;
	return C;
}

Color Shade(const Color& C, float K)
{
	return K >= 0.0f ? Mix(C, Hex(0xffffff), K) : Mix(C, Hex(0x000000), -K);
}

/** A horizontal cylinder shading (light on the left, a highlight, dark on the right). */
Paint Cylinder(float X0, float X1, const Color& Base)
{
	return Paint::Linear({X0, 0.0f}, {X1, 0.0f}, Shade(Base, 0.1f), Shade(Base, -0.35f));
}

std::string Money2(Chips Cents)
{
	char B[32];
	std::snprintf(B, sizeof(B), "$%lld.%02lld", static_cast<long long>(Cents / 100), static_cast<long long>(Cents % 100));
	return B;
}

/** The first letters of a brand for a label too small for the name. */
std::string Initials(const std::string& Name)
{
	std::string Out;
	bool Start = true;
	for (const char Ch : Name)
	{
		if (Start && Ch != ' ')
		{
			Out += Ch;
		}
		Start = Ch == ' ';
	}
	return Out;
}

/** Hunger (a fork), thirst (a drop) and energy (a bolt). */
void NeedIcon(Canvas& C, int Which, float X, float Y, float S, const Color& Col)
{
	if (Which == 0)
	{
		C.StrokePolyline({{X - S * 0.18f, Y - S * 0.42f}, {X - S * 0.18f, Y - S * 0.05f}}, false, Col, S * 0.1f, true);
		C.StrokePolyline({{X, Y - S * 0.42f}, {X, Y + S * 0.42f}}, false, Col, S * 0.12f, true);
		C.StrokePolyline({{X + S * 0.18f, Y - S * 0.42f}, {X + S * 0.18f, Y - S * 0.05f}}, false, Col, S * 0.1f, true);
		C.StrokePolyline({{X - S * 0.18f, Y - S * 0.05f}, {X, Y + S * 0.05f}, {X + S * 0.18f, Y - S * 0.05f}}, false, Col, S * 0.1f, true);
	}
	else if (Which == 1)
	{
		C.FillPolygon({{X, Y - S * 0.46f}, {X + S * 0.28f, Y + S * 0.06f}, {X, Y + S * 0.18f}, {X - S * 0.28f, Y + S * 0.06f}}, Paint(Col));
		C.FillCircle(X, Y + S * 0.14f, S * 0.29f, Paint(Col));
	}
	else
	{
		C.FillPolygon({{X + S * 0.1f, Y - S * 0.48f}, {X - S * 0.26f, Y + S * 0.06f}, {X - S * 0.02f, Y + S * 0.06f}, {X - S * 0.1f, Y + S * 0.48f}, {X + S * 0.26f, Y - S * 0.06f}, {X + S * 0.02f, Y - S * 0.06f}},
			Paint(Col));
	}
}

/** A coin with a clover: the Lucky Penny's sign. */
void PennyLogo(Canvas& C, float X, float Y, float R, float A)
{
	C.FillCircle(X, Y, R * 1.9f, Paint::Radial({X, Y}, R * 0.9f, {X, Y}, R * 1.9f, Fade(Copper, 0.3f * A), 0.4f, Fade(Copper, 0.1f * A), Fade(Copper, 0.0f)));
	C.FillCircle(X, Y, R, Paint::Linear({X - R, Y - R}, {X + R, Y + R}, Fade(Hex(0xf6c58f), A), Fade(Hex(0x8a4a1c), A)));
	C.StrokeEllipse(X, Y, R * 0.84f, R * 0.84f, Fade(Hex(0x6b3412), 0.6f * A), R * 0.06f);
	const Color Leaf = Fade(Hex(0x5a2a0c), 0.9f * A);
	for (int I = 0; I < 4; ++I)
	{
		const float Ang = Cf(I) * 1.5707963f + 0.785398f;
		C.FillCircle(X + std::cos(Ang) * R * 0.22f, Y - R * 0.06f + std::sin(Ang) * R * 0.22f, R * 0.2f, Paint(Leaf));
	}
	C.StrokePolyline({{X, Y + R * 0.05f}, {X + R * 0.08f, Y + R * 0.46f}}, false, Leaf, R * 0.08f, true);
}
} // namespace counter_detail

using namespace frontend_detail;
using namespace counter_detail;

// ------------------------------------------------------------------ product art

void DrawProduct(Canvas& C, const store::Item& I, float X, float Y, float Size)
{
	const Color Body = Hex(I.Color);
	const Color Label = Hex(I.Accent);
	const float A = C.GetAlpha();
	C.Save();
	C.Translate(X, Y);
	C.Scale(Size / 200.0f, Size / 200.0f);
	// A soft shadow on the shelf.
	C.FillEllipse(0.0f, 96.0f, 60.0f, 8.0f, Paint::Radial({0.0f, 96.0f}, 0.0f, {0.0f, 96.0f}, 60.0f, Rgba(0, 0, 0, 0.45f * A), 0.5f, Rgba(0, 0, 0, 0.2f * A), Rgba(0, 0, 0, 0.0f)));
	switch (I.Look)
	{
	case store::Art::Can:
	{
		const Rect R{-38.0f, -78.0f, 76.0f, 172.0f};
		C.FillRoundRect(R, 12.0f, Cylinder(-38.0f, 38.0f, Body));
		C.FillRect({-38.0f, -30.0f, 76.0f, 70.0f}, Cylinder(-38.0f, 38.0f, Mix(Body, Label, 0.12f)));
		C.FillRect({-38.0f, 40.0f, 76.0f, 6.0f}, Paint(Label));
		C.FillRect({-38.0f, -36.0f, 76.0f, 6.0f}, Paint(Label));
		C.FillEllipse(0.0f, -80.0f, 36.0f, 8.0f, Paint::Linear({-36.0f, 0.0f}, {36.0f, 0.0f}, Hex(0xe9edf2), Hex(0x8b929c)));
		C.StrokeEllipse(0.0f, -80.0f, 30.0f, 6.0f, Hex(0x6d747e), 1.5f);
		C.FillRoundRect({-10.0f, -84.0f, 20.0f, 6.0f}, 3.0f, Paint(Hex(0xb9c0c9)));
		C.FillRect({-24.0f, -70.0f, 7.0f, 154.0f}, Paint(Rgba(255, 255, 255, 0.22f)));
		C.Text(Initials(I.Name), 2.0f, 16.0f, Ts(30.0f, 900, Label, Align::Center));
		break;
	}
	case store::Art::Bottle:
	{
		const std::vector<Vec2> Shape = {{-16.0f, -96.0f}, {16.0f, -96.0f}, {16.0f, -70.0f}, {36.0f, -40.0f}, {38.0f, 86.0f}, {32.0f, 96.0f}, {-32.0f, 96.0f}, {-38.0f, 86.0f}, {-36.0f, -40.0f},
			{-16.0f, -70.0f}};
		const bool Clear = I.Id == "cascade";
		C.FillPolygon(Shape, Paint::Linear({-38.0f, 0.0f}, {38.0f, 0.0f}, Fade(Shade(Body, 0.25f), Clear ? 0.75f : 1.0f), Fade(Shade(Body, -0.3f), Clear ? 0.75f : 1.0f)));
		C.FillRoundRect({-18.0f, -112.0f, 36.0f, 20.0f}, 4.0f, Paint::Linear({-18.0f, 0.0f}, {18.0f, 0.0f}, Shade(Label, 0.2f), Shade(Label, -0.3f)));
		C.FillRect({-37.0f, -8.0f, 74.0f, 62.0f}, Cylinder(-37.0f, 37.0f, Clear ? Hex(0xffffff) : Shade(Body, 0.55f)));
		C.FillRect({-37.0f, 46.0f, 74.0f, 6.0f}, Paint(Label));
		C.Text(Initials(I.Name), 0.0f, 32.0f, Ts(28.0f, 900, Label, Align::Center));
		C.FillRect({-26.0f, -36.0f, 6.0f, 120.0f}, Paint(Rgba(255, 255, 255, 0.28f)));
		break;
	}
	case store::Art::Cup:
	{
		const std::vector<Vec2> Cup = {{-40.0f, -62.0f}, {40.0f, -62.0f}, {30.0f, 96.0f}, {-30.0f, 96.0f}};
		C.FillPolygon(Cup, Paint::Linear({-40.0f, 0.0f}, {40.0f, 0.0f}, Shade(Body, 0.05f), Shade(Body, -0.25f)));
		C.FillPolygon({{-37.0f, -10.0f}, {37.0f, -10.0f}, {33.5f, 46.0f}, {-33.5f, 46.0f}}, Paint::Linear({-37.0f, 0.0f}, {37.0f, 0.0f}, Shade(Label, 0.1f), Shade(Label, -0.3f)));
		C.FillRoundRect({-46.0f, -78.0f, 92.0f, 18.0f}, 6.0f, Paint::Linear({-46.0f, 0.0f}, {46.0f, 0.0f}, Hex(0xf0ece4), Hex(0x9a958c)));
		C.FillEllipse(0.0f, -80.0f, 34.0f, 7.0f, Paint(Hex(0xd9d4ca)));
		C.FillRoundRect({10.0f, -88.0f, 14.0f, 5.0f}, 2.0f, Paint(Hex(0x8a857c)));
		if (I.Id == "drip-coffee")
		{
			for (int K = -1; K <= 1; ++K)
			{
				C.StrokePolyline({{Cf(K) * 14.0f, -96.0f}, {Cf(K) * 14.0f - 6.0f, -114.0f}, {Cf(K) * 14.0f + 4.0f, -130.0f}}, false, Rgba(255, 255, 255, 0.35f), 3.0f, true);
			}
		}
		C.Text(Initials(I.Name), 0.0f, 26.0f, Ts(24.0f, 900, Hex(0xffffff), Align::Center));
		break;
	}
	case store::Art::Bag:
	{
		std::vector<Vec2> Bag;
		for (int K = 0; K <= 8; ++K)
		{
			Bag.push_back({-56.0f + Cf(K) * 14.0f, -92.0f + (K % 2 == 0 ? 0.0f : 6.0f)});
		}
		Bag.push_back({60.0f, -70.0f});
		Bag.push_back({62.0f, 70.0f});
		for (int K = 8; K >= 0; --K)
		{
			Bag.push_back({-56.0f + Cf(K) * 14.0f, 92.0f - (K % 2 == 0 ? 0.0f : 6.0f)});
		}
		Bag.push_back({-62.0f, 70.0f});
		Bag.push_back({-60.0f, -70.0f});
		C.FillPolygon(Bag, Paint::Linear({-62.0f, 0.0f}, {62.0f, 0.0f}, Shade(Body, 0.15f), Shade(Body, -0.35f)));
		C.FillCircle(0.0f, 10.0f, 38.0f, Paint(Label));
		C.FillEllipse(-6.0f, 22.0f, 24.0f, 12.0f, Paint(Hex(0xe8c36a)));
		C.FillEllipse(12.0f, 16.0f, 18.0f, 10.0f, Paint(Hex(0xd9ad4e)));
		C.Text(I.Name, 0.0f, -38.0f, Ts(22.0f, 900, Label, Align::Center));
		C.FillRect({-50.0f, -80.0f, 8.0f, 150.0f}, Paint(Rgba(255, 255, 255, 0.16f)));
		break;
	}
	case store::Art::Bar:
	{
		C.Save();
		C.Rotate(-0.2f);
		std::vector<Vec2> Wrap = {{-86.0f, -30.0f}};
		for (int K = 0; K <= 6; ++K)
		{
			Wrap.push_back({-86.0f + (K % 2 == 0 ? 0.0f : -6.0f), -30.0f + Cf(K) * 10.0f});
		}
		Wrap = {{-80.0f, -30.0f}, {80.0f, -30.0f}, {88.0f, -22.0f}, {80.0f, -14.0f}, {88.0f, -6.0f}, {80.0f, 2.0f}, {88.0f, 10.0f}, {80.0f, 18.0f}, {88.0f, 26.0f}, {80.0f, 30.0f}, {-80.0f, 30.0f},
			{-88.0f, 26.0f}, {-80.0f, 18.0f}, {-88.0f, 10.0f}, {-80.0f, 2.0f}, {-88.0f, -6.0f}, {-80.0f, -14.0f}, {-88.0f, -22.0f}};
		C.FillPolygon(Wrap, Paint::Linear({0.0f, -30.0f}, {0.0f, 30.0f}, Shade(Body, 0.2f), Shade(Body, -0.3f)));
		C.FillPolygon({{-30.0f, -30.0f}, {10.0f, -30.0f}, {-10.0f, 30.0f}, {-50.0f, 30.0f}}, Paint(Label));
		C.Text(I.Name, 30.0f, 8.0f, Ts(18.0f, 900, Hex(0xffffff), Align::Center));
		C.Restore();
		break;
	}
	case store::Art::Pouch:
	{
		const std::vector<Vec2> Pouch = {{-52.0f, -86.0f}, {52.0f, -86.0f}, {56.0f, 70.0f}, {46.0f, 92.0f}, {-46.0f, 92.0f}, {-56.0f, 70.0f}};
		C.FillPolygon(Pouch, Paint::Linear({-56.0f, 0.0f}, {56.0f, 0.0f}, Shade(Body, 0.12f), Shade(Body, -0.3f)));
		C.FillRect({-52.0f, -86.0f, 104.0f, 12.0f}, Paint(Shade(Body, -0.2f)));
		C.FillRoundRect({-34.0f, 4.0f, 68.0f, 62.0f}, 30.0f, Paint(Hex(0xe8d7b4)));
		for (int K = 0; K < 9; ++K)
		{
			C.FillCircle(-22.0f + Cf(K % 3) * 22.0f, 20.0f + Cf(K / 3) * 15.0f, 5.5f, Paint(K % 2 ? Hex(0x7a4a22) : Hex(0xb07a3f)));
		}
		C.FillRect({-52.0f, -54.0f, 104.0f, 40.0f}, Paint(Label));
		C.Text(I.Name, 0.0f, -27.0f, Ts(18.0f, 900, Hex(0xf5f0e1), Align::Center));
		break;
	}
	case store::Art::HotDog:
	{
		C.FillRoundRect({-92.0f, -18.0f, 184.0f, 36.0f}, 18.0f, Paint::Linear({0.0f, -18.0f}, {0.0f, 18.0f}, Shade(Body, 0.15f), Shade(Body, -0.3f)));
		C.FillEllipse(0.0f, 22.0f, 82.0f, 26.0f, Paint::Linear({0.0f, 0.0f}, {0.0f, 48.0f}, Hex(0xf0b867), Hex(0xb57a32)));
		C.FillEllipse(0.0f, -8.0f, 80.0f, 16.0f, Paint(Hex(0xe8a85a)));
		C.FillRoundRect({-86.0f, -22.0f, 172.0f, 26.0f}, 13.0f, Paint::Linear({0.0f, -22.0f}, {0.0f, 4.0f}, Shade(Body, 0.1f), Shade(Body, -0.25f)));
		std::vector<Vec2> Mustard;
		for (int K = 0; K <= 14; ++K)
		{
			Mustard.push_back({-70.0f + Cf(K) * 10.0f, -12.0f + (K % 2 == 0 ? -5.0f : 4.0f)});
		}
		C.StrokePolyline(Mustard, false, Label, 4.0f, true);
		break;
	}
	case store::Art::Burrito:
	{
		C.Save();
		C.Rotate(-0.35f);
		C.FillRoundRect({-90.0f, -34.0f, 170.0f, 68.0f}, 32.0f, Paint::Linear({0.0f, -34.0f}, {0.0f, 34.0f}, Hex(0xe3e7ec), Hex(0x8e959f)));
		for (int K = 0; K < 6; ++K)
		{
			C.StrokePolyline({{-70.0f + Cf(K) * 24.0f, -30.0f}, {-60.0f + Cf(K) * 24.0f, 30.0f}}, false, Rgba(255, 255, 255, 0.35f), 2.0f);
		}
		C.FillEllipse(80.0f, 0.0f, 22.0f, 33.0f, Paint(Body));
		C.FillEllipse(84.0f, 0.0f, 13.0f, 22.0f, Paint(Label));
		C.Restore();
		break;
	}
	case store::Art::Noodles:
	{
		const std::vector<Vec2> Cup = {{-52.0f, -58.0f}, {52.0f, -58.0f}, {40.0f, 92.0f}, {-40.0f, 92.0f}};
		C.FillPolygon(Cup, Paint::Linear({-52.0f, 0.0f}, {52.0f, 0.0f}, Hex(0xffffff), Hex(0xbdbab4)));
		C.FillPolygon({{-49.0f, -20.0f}, {49.0f, -20.0f}, {45.5f, 30.0f}, {-45.5f, 30.0f}}, Paint(Label));
		C.Text("OODLE", 0.0f, 13.0f, Ts(24.0f, 900, Hex(0xffffff), Align::Center));
		C.FillEllipse(0.0f, -60.0f, 54.0f, 10.0f, Paint::Linear({-54.0f, 0.0f}, {54.0f, 0.0f}, Hex(0xf3f1ec), Hex(0xa9a59e)));
		C.FillPolygon({{36.0f, -64.0f}, {64.0f, -76.0f}, {66.0f, -66.0f}, {44.0f, -58.0f}}, Paint(Hex(0xd8d4cc)));
		break;
	}
	default: // Sandwich
	{
		C.FillRoundRect({-80.0f, -70.0f, 160.0f, 150.0f}, 10.0f, Paint(Rgba(220, 235, 245, 0.18f)));
		C.StrokeRoundRect({-80.0f, -70.0f, 160.0f, 150.0f}, 10.0f, Rgba(220, 235, 245, 0.5f), 2.0f);
		C.FillPolygon({{-64.0f, 64.0f}, {64.0f, 64.0f}, {-64.0f, -56.0f}}, Paint::Linear({-64.0f, -56.0f}, {64.0f, 64.0f}, Hex(0xf2dfb0), Hex(0xc9a46a)));
		C.FillPolygon({{-52.0f, 52.0f}, {40.0f, 52.0f}, {-52.0f, -34.0f}}, Paint(Body));
		C.StrokePolyline({{-52.0f, 50.0f}, {38.0f, 50.0f}}, false, Label, 5.0f, true);
		C.StrokePolyline({{-64.0f, -56.0f}, {64.0f, 64.0f}}, false, Hex(0xa8794a), 4.0f, true);
		C.FillRect({-44.0f, -64.0f, 70.0f, 18.0f}, Paint(Hex(0xffffff)));
		C.Text("TODAY", -9.0f, -51.0f, Ts(12.0f, 900, Hex(0x2b2723), Align::Center));
		break;
	}
	}
	C.Restore();
}

// ------------------------------------------------------------------ vitals

void DrawVitals(Canvas& C, const life::State& L, float X, float Y, float Width, double Now, const store::Basket* Preview, float Alpha)
{
	// What the basket would do if it were all eaten and drunk now (store::ReliefOf: the same relief eating applies).
	double Hunger = L.Hunger;
	double Thirst = L.Thirst;
	double Energy = L.Energy;
	if (Preview)
	{
		for (const std::pair<std::string, int>& Ln : Preview->Lines)
		{
			if (const store::Item* I = store::Find(Ln.first))
			{
				const store::Relief Rl = store::ReliefOf(*I, L.Perks.MealBoost);
				Hunger -= Rl.Hunger * Ln.second;
				Thirst -= Rl.Thirst * Ln.second;
				Energy += Rl.Energy * Ln.second;
			}
		}
	}
	struct Row
	{
		const char* Label;
		double Now;
		double After;
		const char* Word;
		uint32_t Col;
	};
	const Row Rows[3] = {{"HUNGER", 100.0 - L.Hunger, 100.0 - std::clamp(Hunger, 0.0, 100.0), life::HungerWord(L.Hunger), 0xff9f43},
		{"THIRST", 100.0 - L.Thirst, 100.0 - std::clamp(Thirst, 0.0, 100.0), life::ThirstWord(L.Thirst), 0x4fb8ff},
		{"ENERGY", L.Energy, std::clamp(Energy, 0.0, 100.0), L.Energy < 25.0 ? "Exhausted" : L.Energy < 50.0 ? "Tired" : "Awake", 0xf2c14e}};
	const float BarX = X + 132.0f;
	const float BarW = Width - 132.0f - 96.0f;
	for (int I = 0; I < 3; ++I)
	{
		const Row& R = Rows[I];
		const float Ry = Y + Cf(I) * 34.0f;
		const bool Low = R.Now < 25.0;
		const float Pulse = Low ? 0.6f + 0.4f * static_cast<float>(std::sin(Now * 5.0)) : 1.0f;
		const Color Col = Fade(Low ? Hex(0xff5a5f) : Hex(R.Col), Alpha);
		NeedIcon(C, I, X + 10.0f, Ry + 9.0f, 18.0f, Fade(Col, Pulse));
		TrackedText(C, R.Label, X + 30.0f, Ry + 15.0f, 11.0f, 800, Fade(MenuMuted, Alpha), 2.4f);
		C.FillRoundRect({BarX, Ry + 5.0f, BarW, 8.0f}, 4.0f, Paint(Fade(MenuInk, 0.1f * Alpha)));
		const float Wn = BarW * static_cast<float>(std::clamp(R.Now, 0.0, 100.0) / 100.0);
		const float Wa = BarW * static_cast<float>(std::clamp(R.After, 0.0, 100.0) / 100.0);
		if (Wa > Wn + 0.5f)
		{
			// The gain from the basket, ghosted and breathing.
			const float G = 0.35f + 0.2f * static_cast<float>(std::sin(Now * 4.0));
			C.FillRoundRect({BarX, Ry + 5.0f, Wa, 8.0f}, 4.0f, Paint(Fade(Hex(R.Col), G * Alpha)));
		}
		C.FillRoundRect({BarX, Ry + 5.0f, std::max(8.0f, Wn), 8.0f}, 4.0f, Paint(Col));
		if (Wa < Wn - 0.5f)
		{
			// What it would cost (salty chips make the thirst worse): the stretch of the bar it would take, in red, breathing.
			const float G = 0.5f + 0.25f * static_cast<float>(std::sin(Now * 4.0));
			C.FillRoundRect({BarX + Wa, Ry + 5.0f, Wn - Wa, 8.0f}, 4.0f, Paint(Fade(Hex(0xff5a5f), G * Alpha)));
		}
		// Hunger or thirst past 70 wears the energy down by the hour: the energy row says how fast instead of a word.
		const double Drain = I == 2 ? life::NeedsDrain(L) : 0.0;
		const bool Draining = Drain >= 0.1;
		const std::string Word = Draining ? "\xE2\x88\x92" + Fixed(Drain, 1) + "/hr" : std::string(R.Word);
		C.Text(Word, X + Width, Ry + 15.0f, Ts(13.0f, 700, Fade(Low || Draining ? Hex(0xff8a8d) : MenuInk, Alpha * (Draining ? 0.7f + 0.25f * Pulse : 0.9f)), Align::Right));
	}
}

// ------------------------------------------------------------------ the counter

std::vector<const store::Item*> StoreCounter::OnShelf() const
{
	std::vector<const store::Item*> Out;
	for (const store::Item& I : store::Catalog())
	{
		if (static_cast<int>(I.Where) == ShelfAt)
		{
			Out.push_back(&I);
		}
	}
	return Out;
}

void StoreCounter::Open(double Now, int Shelf)
{
	Shown = true;
	ShelfAt = std::clamp(Shelf, 0, store::ShelfCount - 1);
	Sel = 0;
	Left = false;
	OpenedAt = Now;
	SelAt = Now;
	Basket = store::Basket();
	PaidAt = -10.0;
	DeclinedAt = -10.0;
	LineAt.clear();
	BagCells.clear();
	BagFirst = 0;
	Note = S.ClerkSays(Basket);
	NoteAt = Now;
	NoteBad = false;
}

void StoreCounter::Close(double Now)
{
	Shown = false;
	OpenedAt = Now;
}

void StoreCounter::Say(const std::string& Words, double Now, bool Bad)
{
	Note = Words;
	NoteAt = Now;
	NoteBad = Bad;
}

void StoreCounter::Pay(double Now)
{
	if (Basket.Empty())
	{
		Say("You gotta pick something up first.", Now, false);
		return;
	}
	const store::Basket Was = Basket;
	const std::string Why = S.Checkout(Basket);
	if (!Why.empty())
	{
		DeclinedAt = Now;
		Say("Card says no. Want to put something back?", Now, true);
		return;
	}
	LastPaid = Was;
	Basket = store::Basket();
	PaidAt = Now;
	// The reader's approval code: the moment and the amount, scrambled (the same sale prints the same code).
	PaidAuth = std::floor(S.WorldMinutes() * 60.0) * 31.0 + static_cast<double>(Was.Total());
	Say(Was.Count() >= 4 ? "Have a good one. Don't eat it all at once." : "There you go. Stay dry out there.", Now, false);
}

void StoreCounter::Pick(const std::string& Id, double Now)
{
	const int Had = Basket.Count();
	Basket.Add(Id);
	if (Basket.Count() == Had)
	{
		// Basket::Add stops a line at nine.
		Say("Nine's the limit on those, man. Ray's rule, not mine.", Now, false);
		return;
	}
	LineAt[Id] = Now;
	PaidAt = -10.0;
	DeclinedAt = -10.0;
	Say(S.ClerkSays(Basket), Now, false);
}

void StoreCounter::PutBack(const std::string& Id, double Now)
{
	const int Had = Basket.Count();
	Basket.Remove(Id);
	if (Basket.Count() == Had)
	{
		return;
	}
	LineAt[Id] = Now;
	DeclinedAt = -10.0;
}

void StoreCounter::Key(const std::string& Name, double Now)
{
	if (!Shown)
	{
		return;
	}
	// Just walked up: the button that opened the counter (E, or the pad's A) may still be held down, and its repeats
	// would drop something in the basket or turn the shelf (a held stick would move the selection). Leaving always works.
	if (Now - OpenedAt < OpenGrace && Name != "Escape" && Name != "P")
	{
		return;
	}
	const std::vector<const store::Item*> Items = OnShelf();
	const int N = static_cast<int>(Items.size());
	if (Name == "Escape" || Name == "P")
	{
		Left = true;
		Close(Now);
	}
	else if ((Name == "Left" || Name == "Right") && N > 0)
	{
		Sel = std::clamp(Sel + (Name == "Left" ? -1 : 1), 0, N - 1);
		SelAt = Now;
	}
	else if (Name == "Up" || Name == "Down")
	{
		const int Next = Sel + (Name == "Up" ? -3 : 3);
		Sel = Next >= 0 && Next < N ? Next : Sel;
		SelAt = Now;
	}
	else if (Name == "Q" || Name == "E" || Name == "TabPrev" || Name == "TabNext")
	{
		ShelfAt = (ShelfAt + (Name == "Q" || Name == "TabPrev" ? store::ShelfCount - 1 : 1)) % store::ShelfCount;
		Sel = 0;
		SelAt = Now;
	}
	else if ((Name == "Enter" || Name == "Space") && Sel < N)
	{
		Pick(Items[static_cast<size_t>(Sel)]->Id, Now);
	}
	else if ((Name == "Backspace" || Name == "Delete" || Name == "PutBack") && Sel < N)
	{
		PutBack(Items[static_cast<size_t>(Sel)]->Id, Now);
	}
	else if (Name == "Tab" || Name == "Reset")
	{
		Pay(Now);
	}
}

void StoreCounter::Draw(Canvas& C, double Now)
{
	if (!Shown)
	{
		Ptr.EndFrame();
		return;
	}
	if (Ptr.Pressed)
	{
		PressX = Ptr.X;
		PressY = Ptr.Y;
	}
	auto Released = [&](const Rect& R) { return Ptr.Released && Inside(R, Ptr.X, Ptr.Y) && Inside(R, PressX, PressY); };
	const float W = C.Width();
	const float H = Height;
	const float In = Ease((Now - OpenedAt) / 0.4);
	const float Al = C.GetAlpha();
	C.SetAlpha(Al * In);
	// The store behind stays visible on the right, under the receipt.
	C.FillRect({0.0f, 0.0f, W, H}, Paint::Linear({0.0f, 0.0f}, {W, 0.0f}, Rgba(4, 6, 10, 0.9f), Rgba(4, 6, 10, 0.55f)));
	C.FillRect({0.0f, H - 240.0f, W, 240.0f}, Paint::Linear({0.0f, H - 240.0f}, {0.0f, H}, Rgba(4, 6, 10, 0.0f), Rgba(4, 6, 10, 0.8f)));

	// The sign (a size down on narrow screens, where the clerk needs the room).
	const float GridW = W - Margin * 2.0f - ReceiptW - 48.0f;
	const bool Narrow = GridW < 1000.0f;
	PennyLogo(C, Margin + 34.0f, 112.0f, 34.0f, 1.0f);
	const float SignW = TrackedText(C, "LUCKY PENNY", Margin + 86.0f, 116.0f, Narrow ? 30.0f : 38.0f, 900, MenuInk, Narrow ? 4.0f : 5.0f);
	const float SubW = TrackedText(C, "#212 \xC2\xB7 FIFTH & MARKET \xC2\xB7 OPEN 24 HOURS", Margin + 88.0f, 142.0f, Narrow ? 11.0f : 12.0f, 700, Copper, Narrow ? 2.2f : 3.0f);

	// The clerk: the rest of the row past the sign (never over it), as tall as what he says.
	const float BubX = std::max(Margin + GridW * 0.48f, Margin + 88.0f + std::max(SignW, SubW) + 32.0f);
	const float BubW = std::max(240.0f, Margin + GridW - BubX);
	const std::string Quote = "\xE2\x80\x9C" + Note + "\xE2\x80\x9D";
	float Fs = 17.0f;
	float Lh = 24.0f;
	std::vector<std::string> Said = WrapText(C, Quote, BubW - 44.0f, Fs, 400);
	if (Said.size() > 2)
	{
		Fs = 15.0f;
		Lh = 21.0f;
		Said = WrapText(C, Quote, BubW - 44.0f, Fs, 400);
	}
	if (Said.size() > 3)
	{
		Said.resize(3);
		Said[2] += "\xE2\x80\xA6";
	}
	const Rect Bubble{BubX, 70.0f, BubW, std::max(96.0f, 46.0f + Cf(static_cast<int>(Said.size())) * Lh)};
	const float Na = Ease((Now - NoteAt) / 0.3);
	C.FillRoundRect(Bubble, 14.0f, Paint(Rgba(255, 255, 255, 0.06f * Na)));
	C.FillPolygon({{Bubble.X + 30.0f, Bubble.Y + Bubble.H}, {Bubble.X + 54.0f, Bubble.Y + Bubble.H}, {Bubble.X + 26.0f, Bubble.Y + Bubble.H + 16.0f}}, Paint(Rgba(255, 255, 255, 0.06f * Na)));
	TrackedText(C, "BENNY \xC2\xB7 NIGHT CLERK", Bubble.X + 22.0f, Bubble.Y + 28.0f, 11.0f, 800, Fade(NoteBad ? MenuWarn : MenuTeal, Na), 2.6f);
	float Ny = Bubble.Y + 34.0f;
	for (const std::string& Ln : Said)
	{
		Ny += Lh;
		C.Text(Ln, Bubble.X + 22.0f, Ny, Ts(Fs, 400, Fade(MenuInk, 0.92f * Na)));
	}

	// Shelf tabs.
	float Tx = Margin;
	const float TabY = 226.0f;
	Tx += Glyph(C, Gamepad ? "LB" : "Q", Tx, TabY + 3.0f, 1.0f) + 24.0f;
	for (int I = 0; I < store::ShelfCount; ++I)
	{
		const char* Name = store::ShelfName(static_cast<store::Shelf>(I));
		const bool On = I == ShelfAt;
		const float Lw = TrackedWidth(C, Name, 16.0f, 800, 4.0f);
		TrackedText(C, Name, Tx, TabY, 16.0f, 800, On ? MenuInk : MenuMuted, 4.0f);
		if (On)
		{
			C.FillRect({Tx, TabY + 12.0f, Lw, 3.0f}, Paint(Copper));
		}
		if (Released({Tx - 10.0f, TabY - 26.0f, Lw + 20.0f, 40.0f}))
		{
			ShelfAt = I;
			Sel = 0;
			SelAt = Now;
		}
		Tx += Lw + 36.0f;
	}
	Glyph(C, Gamepad ? "RB" : "E", Tx - 12.0f, TabY + 3.0f, 1.0f);

	// The shelf. Three across; on a narrow screen (4:3, 5:4, 3:2) each card stacks the words under the product instead.
	const std::vector<const store::Item*> Items = OnShelf();
	const float Cw = (GridW - 40.0f) / 3.0f;
	const float Ch = 236.0f;
	const bool Stacked = Cw < 320.0f;
	for (size_t K = 0; K < Items.size(); ++K)
	{
		const store::Item& I = *Items[K];
		const int Idx = static_cast<int>(K);
		const float Ci = Ease((Now - OpenedAt - 0.04 * static_cast<double>(K)) / 0.35);
		const bool Focus = Idx == Sel;
		// The chosen card lifts off the shelf a little.
		const float Lift = Focus ? 5.0f * Ease((Now - SelAt) / 0.18) : 0.0f;
		const Rect R{Margin + Cf(Idx % 3) * (Cw + 20.0f), 268.0f + Cf(Idx / 3) * (Ch + 20.0f) + (1.0f - Ci) * 14.0f - Lift, Cw, Ch};
		const bool Over = Inside(R, Ptr.X, Ptr.Y);
		int InBasket = 0;
		for (const std::pair<std::string, int>& Ln : Basket.Lines)
		{
			InBasket = Ln.first == I.Id ? Ln.second : InBasket;
		}
		if (Focus)
		{
			C.GlowRoundRect(R, 6.0f, Fade(Copper, 0.25f), 22.0f);
		}
		C.FillRoundRect(R, 6.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Focus ? Rgba(30, 33, 44, 0.96f) : Rgba(24, 28, 38, 0.95f), Rgba(12, 14, 20, 0.95f)));
		C.StrokeRoundRect({R.X + 0.75f, R.Y + 0.75f, R.W - 1.5f, R.H - 1.5f}, 6.0f, Focus ? Copper : Fade(MenuInk, Over ? 0.22f : 0.08f), Focus ? 2.0f : 1.0f);
		float Tx0 = R.X + 184.0f;
		float Tw = R.W - 184.0f - 18.0f;
		float Ey = R.Y + 146.0f;
		Rect Tag{R.X + R.W - 112.0f, R.Y + R.H - 52.0f, 94.0f, 36.0f};
		// What it does (store::ReliefOf: what eating it applies, the line cook's boost included), as pills. Measured first:
		// on a card too narrow for them in a row (16:10) they take two rows and the blurb gives up a line, so none goes missing.
		const store::Relief Rl = store::ReliefOf(I, S.Life.Perks.MealBoost);
		const std::pair<int, double> Effects[3] = {{0, Rl.Hunger}, {1, Rl.Thirst}, {2, Rl.Energy}};
		auto PillText = [](double V) { return (V > 0.0 ? "+" : "\xE2\x88\x92") + std::to_string(static_cast<int>(std::lround(std::fabs(V)))); };
		float PillsW = -6.0f;
		for (const std::pair<int, double>& E : Effects)
		{
			PillsW += std::fabs(E.second) < 0.5 ? 0.0f : C.Measure(PillText(E.second), 13.0f, 800) + 40.0f;
		}
		const bool TwoRows = !Stacked && Tx0 + PillsW > R.X + R.W - 12.0f;
		if (Stacked)
		{
			// The product on top, then the name, the kind and what it does; the tag in the corner.
			C.FillRect({R.X + R.W * 0.5f - 60.0f, R.Y + 122.0f, 120.0f, 3.0f}, Paint(Rgba(255, 255, 255, 0.08f)));
			DrawProduct(C, I, R.X + R.W * 0.5f, R.Y + 70.0f, 104.0f);
			Tx0 = R.X + 14.0f;
			Tw = R.W - 28.0f;
			C.Text(I.Name, Tx0, R.Y + 150.0f, Ts(18.0f, 900, MenuInk, Align::Left, Baseline::Alphabetic, false, Tw));
			C.Text(I.Kind, Tx0, R.Y + 168.0f, Ts(12.0f, 600, MenuMuted, Align::Left, Baseline::Alphabetic, false, Tw));
			Ey = R.Y + 176.0f;
			Tag = {R.X + R.W - 86.0f, R.Y + R.H - 34.0f, 74.0f, 26.0f};
		}
		else
		{
			// The shelf edge under the product.
			C.FillRect({R.X + 18.0f, R.Y + R.H - 46.0f, 150.0f, 4.0f}, Paint(Rgba(255, 255, 255, 0.08f)));
			DrawProduct(C, I, R.X + 93.0f, R.Y + R.H * 0.5f - 18.0f, 150.0f);
			C.Text(I.Name, Tx0, R.Y + 46.0f, Ts(22.0f, 900, MenuInk, Align::Left, Baseline::Alphabetic, false, Tw));
			C.Text(I.Kind, Tx0, R.Y + 68.0f, Ts(13.0f, 600, MenuMuted, Align::Left, Baseline::Alphabetic, false, Tw));
			// The blurb: three lines, or two over a second row of pills; cut short, its last line ends in an ellipsis.
			const std::vector<std::string> Blurb = WrapText(C, I.Blurb, Tw, 13.0f, 400);
			const size_t Keep = TwoRows ? 2u : 3u;
			for (size_t Bi = 0; Bi < Blurb.size() && Bi < Keep; ++Bi)
			{
				const std::string Ln = Bi + 1 == Keep && Bi + 1 < Blurb.size() ? Blurb[Bi] + " " + Blurb[Bi + 1] : Blurb[Bi];
				C.Text(Ln, Tx0, R.Y + 92.0f + Cf(static_cast<int>(Bi)) * 18.0f, Ts(13.0f, 400, Fade(MenuInk, 0.6f), Align::Left, Baseline::Alphabetic, false, Tw));
			}
			Ey = TwoRows ? R.Y + 118.0f : Ey;
		}
		float Ex = Tx0;
		bool Wrapped = false;
		for (const std::pair<int, double>& E : Effects)
		{
			if (std::fabs(E.second) < 0.5)
			{
				continue;
			}
			const bool Good = E.second > 0.0;
			const uint32_t Hue = E.first == 0 ? 0xff9f43u : E.first == 1 ? 0x4fb8ffu : 0xf2c14eu;
			const std::string Txt = PillText(E.second);
			const float Pw = C.Measure(Txt, 13.0f, 800) + 34.0f;
			if (Ex + Pw > R.X + R.W - 12.0f)
			{
				// One wrap, onto the second row (still clear of the price tag), and only when it helps.
				if (!TwoRows || Wrapped || Ex <= Tx0 + 0.5f)
				{
					break;
				}
				Wrapped = true;
				Ex = Tx0;
				Ey += 28.0f;
			}
			C.FillRoundRect({Ex, Ey, Pw, 24.0f}, 12.0f, Paint(Fade(Good ? Hex(Hue) : MenuWarn, 0.14f)));
			NeedIcon(C, E.first, Ex + 13.0f, Ey + 12.0f, 13.0f, Good ? Hex(Hue) : MenuWarn);
			C.Text(Txt, Ex + 24.0f, Ey + 17.0f, Ts(13.0f, 800, Good ? Hex(Hue) : MenuWarn));
			Ex += Pw + 6.0f;
		}
		// The shelf tag.
		C.FillRect(Tag, Paint(Hex(0xffd23f)));
		C.FillRect({Tag.X, Tag.Y, 6.0f, Tag.H}, Paint(Hex(0xe23b4e)));
		C.Text(Money2(I.PriceCents), Tag.X + Tag.W * 0.5f + 3.0f, Tag.Y + Tag.H * 0.5f + (Stacked ? 6.0f : 8.0f), Ts(Stacked ? 16.0f : 20.0f, 900, Hex(0x1a1408), Align::Center));
		// In the basket: a minus to put one back with the mouse, and how many. Bottom left, on the shelf strip under the
		// product: up top a long name (16:10) or the product art (5:4) would run under them.
		const float Qy = Stacked ? Tag.Y : R.Y + R.H - 38.0f;
		const Rect Minus{R.X + (Stacked ? 14.0f : 18.0f), Qy, 26.0f, 26.0f};
		if (InBasket > 0)
		{
			const bool MinusOver = Inside(Minus, Ptr.X, Ptr.Y);
			C.FillCircle(Minus.X + 13.0f, Minus.Y + 13.0f, 13.0f, Paint(MinusOver ? Fade(MenuInk, 0.28f) : Fade(MenuInk, 0.12f)));
			C.StrokePolyline({{Minus.X + 8.0f, Minus.Y + 13.0f}, {Minus.X + 18.0f, Minus.Y + 13.0f}}, false, MenuInk, 2.2f, true);
			C.FillRoundRect({Minus.X + 32.0f, Qy, 52.0f, 26.0f}, 13.0f, Paint(Copper));
			C.Text("\xC3\x97" + std::to_string(InBasket), Minus.X + 58.0f, Qy + 18.0f, Ts(14.0f, 900, Hex(0x1a0e04), Align::Center));
		}
		if ((InBasket > 0 && Released(Minus)) || Released(R))
		{
			SelAt = Sel != Idx ? Now : SelAt;
			Sel = Idx;
			if (InBasket > 0 && Released(Minus))
			{
				PutBack(I.Id, Now);
			}
			else
			{
				Pick(I.Id, Now);
			}
		}
	}

	// Vitals, and what's in the bag (the vitals give up some width on a narrow screen so the bag keeps a few chips).
	const float Vy = 806.0f;
	const float VitW = std::clamp(GridW * 0.42f, 360.0f, 520.0f);
	TrackedText(C, "HOW YOU'RE DOING", Margin, Vy - 18.0f, 11.0f, 800, MenuMuted, 2.6f);
	DrawVitals(C, S.Life, Margin, Vy, VitW, Now, &Basket);
	const float Bx = Margin + VitW + 60.0f;
	DrawBag(C, Bx, Vy, Margin + GridW - Bx, Now);

	// The receipt, and paying.
	DrawReceipt(C, W - Margin - ReceiptW, Now);

	// Hints.
	float Hx = Margin;
	const float Hy = H - 62.0f;
	const std::pair<const char*, const char*> Hints[5] = {{Gamepad ? "DPAD" : "UP", "CHOOSE"}, {Gamepad ? "A" : "ENTER", "ADD"}, {Gamepad ? "X" : "BACKSPACE", "PUT BACK"}, {Gamepad ? "Y" : "TAB", "PAY"},
		{Gamepad ? "B" : "ESC", "WALK AWAY"}};
	for (const std::pair<const char*, const char*>& Hp : Hints)
	{
		Hx += Glyph(C, Hp.first, Hx, Hy, 1.0f) + 10.0f;
		Hx += TrackedText(C, Hp.second, Hx, Hy - 2.0f, 13.0f, 700, MenuMuted, 2.6f) + 32.0f;
	}
	C.SetAlpha(Al);
	Ptr.EndFrame();
}

void StoreCounter::DrawBag(Canvas& C, float X, float Y, float W, double Now)
{
	const life::State& L = S.Life;
	auto Held = [&L](const std::string& Id) {
		const auto It = L.Pantry.find(Id);
		return It == L.Pantry.end() ? 0 : It->second;
	};
	auto Released = [this](const Rect& R) { return Ptr.Released && Inside(R, Ptr.X, Ptr.Y) && Inside(R, PressX, PressY); };
	// Two rows of chips, as many across as fit.
	const float ChipW = 104.0f;
	const float ChipH = 88.0f;
	const float Gap = 10.0f;
	const int PerRow = std::max(1, static_cast<int>((W + Gap) / (ChipW + Gap)));
	const int Slots = PerRow * 2;
	const Rect Area{X - 8.0f, Y - 12.0f, Cf(PerRow) * (ChipW + Gap) - Gap + 16.0f, ChipH * 2.0f + Gap + 16.0f};
	const bool Over = Inside(Area, Ptr.X, Ptr.Y);
	// The chips, shelf by shelf. While the pointer is over the bag they stay put: one eaten to the last keeps its
	// place (a ghost) so the next click lands on what it was aimed at; the bag closes up once the pointer leaves.
	if (!Over)
	{
		BagCells.clear();
	}
	for (const store::Item& I : store::Catalog())
	{
		if (Held(I.Id) > 0 && std::find(BagCells.begin(), BagCells.end(), I.Id) == BagCells.end())
		{
			BagCells.push_back(I.Id);
		}
	}
	int Things = 0;
	for (const auto& H : L.Pantry)
	{
		Things += std::max(0, H.second);
	}
	const float Lw = TrackedText(C, "IN YOUR BAG", X, Y - 18.0f, 11.0f, 800, MenuMuted, 2.6f);
	if (Things > 0)
	{
		TrackedText(C, std::to_string(Things), X + Lw + 12.0f, Y - 18.0f, 11.0f, 800, Copper, 2.6f);
	}
	if (BagCells.empty())
	{
		C.Text("Nothing yet.", X, Y + 22.0f, Ts(15.0f, 400, MenuDim));
		BagFirst = 0;
		return;
	}
	// More kinds than chips: the last chip turns the page (and comes back around to the first).
	const int N = static_cast<int>(BagCells.size());
	const bool Pages = N > Slots;
	const int Per = Pages ? Slots - 1 : Slots;
	BagFirst = Pages && BagFirst > 0 && BagFirst < N ? BagFirst - BagFirst % Per : 0;
	// The wheel over the bag turns the pages too, as Penny Drop's does (positive: down, the next page).
	if (Pages && Over && Ptr.Wheel != 0.0f)
	{
		const int LastPage = (N - 1) / Per * Per;
		BagFirst = Ptr.Wheel > 0.0f ? std::min(LastPage, BagFirst + Per) : std::max(0, BagFirst - Per);
	}
	const int End = std::min(N, BagFirst + Per);
	auto ChipAt = [&](int Slot) { return Rect{X + Cf(Slot % PerRow) * (ChipW + Gap), Y - 4.0f + Cf(Slot / PerRow) * (ChipH + Gap), ChipW, ChipH}; };
	std::string Eat;
	std::string Grab;
	for (int K = BagFirst; K < End; ++K)
	{
		const std::string& Id = BagCells[static_cast<size_t>(K)];
		const store::Item* I = store::Find(Id);
		if (!I)
		{
			continue;
		}
		const Rect Chip = ChipAt(K - BagFirst);
		const bool Hover = Inside(Chip, Ptr.X, Ptr.Y);
		const int Have = Held(Id);
		if (Have > 0)
		{
			C.FillRoundRect(Chip, 8.0f, Paint(Fade(MenuInk, Hover ? 0.11f : 0.05f)));
			if (Hover)
			{
				C.StrokeRoundRect(Chip, 8.0f, Fade(Copper, 0.7f), 1.5f);
			}
			DrawProduct(C, *I, Chip.X + 28.0f, Chip.Y + 40.0f, 56.0f);
			C.Text("\xC3\x97" + std::to_string(Have), Chip.X + Chip.W - 10.0f, Chip.Y + 24.0f, Ts(15.0f, 900, MenuInk, Align::Right));
			TrackedText(C, I->Drink() ? "DRINK" : "EAT", Chip.X + Chip.W - 10.0f, Chip.Y + Chip.H - 12.0f, 11.0f, 800, Hover ? Copper : MenuMuted, 2.4f, Align::Right);
			if (Released(Chip))
			{
				Eat = Id;
			}
		}
		else
		{
			// Eaten to the last: a ghost of it, and a click grabs another off the shelf.
			C.StrokeRoundRect({Chip.X + 0.5f, Chip.Y + 0.5f, Chip.W - 1.0f, Chip.H - 1.0f}, 8.0f, Fade(MenuInk, Hover ? 0.3f : 0.12f), 1.0f);
			const float A0 = C.GetAlpha();
			C.SetAlpha(A0 * 0.3f);
			DrawProduct(C, *I, Chip.X + 28.0f, Chip.Y + 40.0f, 56.0f);
			C.SetAlpha(A0);
			TrackedText(C, "ALL GONE", Chip.X + Chip.W - 10.0f, Chip.Y + 24.0f, 10.0f, 800, MenuDim, 1.6f, Align::Right);
			TrackedText(C, "GRAB ONE", Chip.X + Chip.W - 10.0f, Chip.Y + Chip.H - 12.0f, 11.0f, 800, Hover ? Copper : MenuDim, 2.0f, Align::Right);
			if (Released(Chip))
			{
				Grab = Id;
			}
		}
	}
	if (Pages)
	{
		const Rect More = ChipAt(End - BagFirst);
		const int Rest = N - End;
		const bool Hover = Inside(More, Ptr.X, Ptr.Y);
		C.FillRoundRect(More, 8.0f, Paint(Fade(Copper, Hover ? 0.24f : 0.12f)));
		C.StrokeRoundRect({More.X + 0.5f, More.Y + 0.5f, More.W - 1.0f, More.H - 1.0f}, 8.0f, Fade(Copper, Hover ? 0.85f : 0.45f), 1.0f);
		C.Text(Rest > 0 ? "+" + std::to_string(Rest) : std::string("BACK"), More.X + More.W * 0.5f, More.Y + 48.0f, Ts(Rest > 0 ? 28.0f : 18.0f, 900, MenuInk, Align::Center));
		TrackedText(C, Rest > 0 ? "MORE" : "TO THE TOP", More.X + More.W * 0.5f, More.Y + More.H - 12.0f, 10.0f, 800, Hover ? Copper : MenuMuted, 2.0f, Align::Center);
		if (Released(More))
		{
			BagFirst = Rest > 0 ? End : 0;
		}
	}
	if (!Eat.empty())
	{
		const store::Item* I = store::Find(Eat);
		if (I && S.Consume(Eat).empty())
		{
			Say(I->Drink() ? "Recycling's out front." : "Not in the store, man. Okay, fine. Napkins are by the door.", Now, false);
		}
	}
	if (const store::Item* I = Grab.empty() ? nullptr : store::Find(Grab))
	{
		// Its shelf comes up with it chosen, and one goes in the basket.
		ShelfAt = static_cast<int>(I->Where);
		const std::vector<const store::Item*> Items = OnShelf();
		for (size_t K = 0; K < Items.size(); ++K)
		{
			Sel = Items[K] == I ? static_cast<int>(K) : Sel;
		}
		SelAt = Now;
		Pick(Grab, Now);
	}
}

void StoreCounter::DrawReceipt(Canvas& C, float Rx0, double Now)
{
	auto Released = [this](const Rect& R) { return Ptr.Released && Inside(R, Ptr.X, Ptr.Y) && Inside(R, PressX, PressY); };
	// A declined card shakes the paper: a quick wobble that dies away.
	const double Dd = Now - DeclinedAt;
	const float Shake = DeclinedAt > 0.0 && Dd >= 0.0 && Dd < 0.4 ? static_cast<float>(std::sin(Dd * 70.0) * 9.0 * (1.0 - Dd / 0.4)) : 0.0f;
	const float Rx = Rx0 + Shake;
	float Ry = 86.0f;
	struct Row
	{
		std::string Id;
		std::string Left;
		std::string Right;
	};
	std::vector<Row> Lines;
	const bool ShowPaid = PaidAt > 0.0 && Basket.Empty();
	const store::Basket& Shown2 = ShowPaid ? LastPaid : Basket;
	for (const std::pair<std::string, int>& Ln : Shown2.Lines)
	{
		const store::Item* I = store::Find(Ln.first);
		std::string Up = I ? I->Name : Ln.first;
		std::transform(Up.begin(), Up.end(), Up.begin(), [](char Cc) { return static_cast<char>(Cc >= 'a' && Cc <= 'z' ? Cc - 32 : Cc); });
		Lines.push_back({Ln.first, std::to_string(Ln.second) + " " + Up, Money2((I ? I->PriceCents : 0) * Ln.second)});
	}
	const float RowH = 26.0f;
	// The header, the lines, the sums, the card, the balance.
	const float PaperH = 326.0f + Cf(std::max(1, static_cast<int>(Lines.size()))) * RowH;
	std::vector<Vec2> PaperShape = {{Rx, Ry}, {Rx + ReceiptW, Ry}};
	for (int K = 0; K <= 22; ++K)
	{
		PaperShape.push_back({Rx + ReceiptW - Cf(K) * ReceiptW / 22.0f, Ry + PaperH + (K % 2 == 0 ? 0.0f : 9.0f)});
	}
	C.GlowRoundRect({Rx, Ry + 8.0f, ReceiptW, PaperH}, 4.0f, Rgba(0, 0, 0, 0.5f), 26.0f);
	C.FillPolygon(PaperShape, Paint::Linear({0.0f, Ry}, {0.0f, Ry + PaperH}, Paper, Shade(Paper, -0.05f)));
	auto Mono = [&](const std::string& Txt, float Xx, float Yy, Align Al2, float Size = 16.0f, int Weight = 500, float A = 1.0f) {
		C.Text(Txt, Xx, Yy, Ts(Size, Weight, Fade(PaperInk, A), Al2, Baseline::Alphabetic, true));
	};
	Ry += 46.0f;
	Mono("LUCKY PENNY #212", Rx + ReceiptW * 0.5f, Ry, Align::Center, 20.0f, 700);
	Ry += 24.0f;
	Mono("1840 FIFTH ST \xC2\xB7 OPEN 24 HRS", Rx + ReceiptW * 0.5f, Ry, Align::Center, 13.0f);
	Ry += 20.0f;
	const double World = S.WorldMinutes();
	Mono(std::string(net::WeekdayName(net::DayOf(World))) + " " + net::DateLabel(net::DayOf(World)) + "  " + net::TimeLabel(World), Rx + ReceiptW * 0.5f, Ry, Align::Center, 13.0f);
	Ry += 22.0f;
	C.StrokePolyline({{Rx + 24.0f, Ry}, {Rx + ReceiptW - 24.0f, Ry}}, false, Fade(PaperInk, 0.35f), 1.0f);
	Ry += 6.0f;
	if (Lines.empty())
	{
		Ry += RowH;
		Mono("Pick something up.", Rx + ReceiptW * 0.5f, Ry, Align::Center, 15.0f);
	}
	std::string Back;
	for (const Row& Ln : Lines)
	{
		Ry += RowH;
		// A line that just changed prints out again: it drops in from the line above.
		const auto At = LineAt.find(Ln.Id);
		const float P = ShowPaid || At == LineAt.end() ? 1.0f : Ease((Now - At->second) / 0.16);
		const float Y = Ry - (1.0f - P) * 10.0f;
		// The mouse's way to put one back: a minus beside the paper on the line under the pointer.
		const Rect Band{Rx - 40.0f, Ry - RowH + 6.0f, ReceiptW + 40.0f, RowH};
		if (!ShowPaid && Inside(Band, Ptr.X, Ptr.Y))
		{
			C.FillRect({Rx + 16.0f, Band.Y, ReceiptW - 32.0f, RowH}, Paint(Fade(PaperInk, 0.06f)));
			const Rect Minus{Rx - 33.0f, Band.Y + 2.0f, 22.0f, 22.0f};
			const bool MinusOver = Inside(Minus, Ptr.X, Ptr.Y);
			C.FillCircle(Minus.X + 11.0f, Minus.Y + 11.0f, 11.0f, Paint(MinusOver ? Fade(MenuWarn, 0.9f) : Fade(MenuInk, 0.22f)));
			C.StrokePolyline({{Minus.X + 6.5f, Minus.Y + 11.0f}, {Minus.X + 15.5f, Minus.Y + 11.0f}}, false, MenuInk, 2.0f, true);
			if (Released(Minus))
			{
				Back = Ln.Id;
			}
		}
		Mono(Ln.Left, Rx + 28.0f, Y, Align::Left, 16.0f, 500, P);
		Mono(Ln.Right, Rx + ReceiptW - 28.0f, Y, Align::Right, 16.0f, 500, P);
	}
	Ry += 18.0f;
	C.StrokePolyline({{Rx + 24.0f, Ry}, {Rx + ReceiptW - 24.0f, Ry}}, false, Fade(PaperInk, 0.35f), 1.0f);
	const Chips Sub = Shown2.Subtotal();
	Ry += 26.0f;
	Mono("SUBTOTAL", Rx + 28.0f, Ry, Align::Left);
	Mono(Money2(Sub), Rx + ReceiptW - 28.0f, Ry, Align::Right);
	Ry += 24.0f;
	Mono("TAX 7.25%", Rx + 28.0f, Ry, Align::Left);
	Mono(Money2(store::Tax(Sub)), Rx + ReceiptW - 28.0f, Ry, Align::Right);
	Ry += 32.0f;
	Mono("TOTAL", Rx + 28.0f, Ry, Align::Left, 20.0f, 700);
	Mono(Money2(Shown2.Total()), Rx + ReceiptW - 28.0f, Ry, Align::Right, 20.0f, 700);
	// The card it goes on (the Bank's checking account), and the reader's approval once it's paid.
	Ry += 26.0f;
	Mono("DEBIT \xC2\xB7\xC2\xB7\xC2\xB7\xC2\xB7 4471", Rx + 28.0f, Ry, Align::Left, 13.0f);
	if (ShowPaid)
	{
		char Auth[24];
		std::snprintf(Auth, sizeof(Auth), "AUTH %06llu", static_cast<unsigned long long>(PaidAuth) * 2654435761ull % 1000000ull);
		Mono(Auth, Rx + ReceiptW - 28.0f, Ry, Align::Right, 13.0f);
	}
	Ry += 28.0f;
	Mono("CARD BALANCE " + Money2(S.BankrollCents), Rx + ReceiptW * 0.5f, Ry, Align::Center, 13.0f);
	// Stamps.
	const bool Paid = PaidAt > 0.0 && Now - PaidAt < 4.0 && Basket.Empty();
	const bool Declined = DeclinedAt > 0.0 && Now - DeclinedAt < 4.0;
	if (Paid || Declined)
	{
		const float Sa = Ease((Now - (Paid ? PaidAt : DeclinedAt)) / 0.15);
		const Color StampInk = Paid ? Hex(0x1f9d55) : Hex(0xd7263d);
		C.Save();
		C.Translate(Rx + ReceiptW * 0.5f, 86.0f + PaperH * 0.55f);
		C.Rotate(-0.22f);
		C.Scale(1.6f - 0.6f * Sa, 1.6f - 0.6f * Sa);
		const std::string Word = Paid ? "PAID" : "DECLINED";
		const float Sw = TrackedWidth(C, Word, 40.0f, 900, 6.0f) + 40.0f;
		C.StrokeRoundRect({-Sw * 0.5f, -36.0f, Sw, 64.0f}, 8.0f, Fade(StampInk, 0.85f * Sa), 4.0f);
		TrackedText(C, Word, 0.0f, 12.0f, 40.0f, 900, Fade(StampInk, 0.85f * Sa), 6.0f, Align::Center);
		C.Restore();
	}
	// Pay (the button stays put while the paper shakes).
	const Rect PayR{Rx0, 86.0f + PaperH + 36.0f, ReceiptW, 66.0f};
	const bool CanPay = !Basket.Empty();
	const bool Short = Basket.Total() > S.BankrollCents;
	const bool PayOver = Inside(PayR, Ptr.X, Ptr.Y);
	C.FillRect(PayR, Paint(CanPay ? (Short ? Fade(MenuWarn, 0.3f) : Mix(Mint, Hex(0xffffff), PayOver ? 0.15f : 0.0f)) : Fade(MenuInk, 0.06f)));
	if (CanPay && !Short)
	{
		C.GlowRoundRect(PayR, 1.0f, Fade(Mint, 0.3f), 18.0f);
	}
	TrackedText(C, CanPay ? (Short ? "CARD WON'T COVER IT" : "PAY " + Money2(Basket.Total())) : "PAY", PayR.X + (PayR.W - 70.0f) * 0.5f, PayR.Y + 41.0f, 20.0f, 900,
		CanPay && !Short ? Hex(0x05140c) : MenuMuted, 3.0f, Align::Center);
	if (Released(PayR))
	{
		Pay(Now);
	}
	Glyph(C, Gamepad ? "Y" : "TAB", PayR.X + PayR.W - 64.0f, PayR.Y + 43.0f, CanPay ? 0.9f : 0.4f);
	if (!Back.empty())
	{
		PutBack(Back, Now);
	}
}

// ------------------------------------------------------------------ the walking HUD

void DrawStreetHud(Canvas& C, const StreetHudInfo& Info, double Now)
{
	const float W = C.Width();
	const float H = StoreCounter::Height;
	// Where and when, top left, over a soft shade so it reads on a bright window.
	C.FillRect({0.0f, 0.0f, 760.0f, 260.0f}, Paint::Radial({0.0f, 0.0f}, 0.0f, {0.0f, 0.0f}, 700.0f, Rgba(0, 0, 0, 0.55f), 0.5f, Rgba(0, 0, 0, 0.25f), Rgba(0, 0, 0, 0.0f)));
	C.FillRect({58.0f, 52.0f, 34.0f, 3.0f}, Paint(MenuNeon));
	TrackedText(C, Info.Place, 104.0f, 60.0f, 15.0f, 800, MenuInk, 5.0f);
	const float Cw = TrackedText(C, Info.Clock, 58.0f, 104.0f, 36.0f, 900, MenuInk, 2.0f);
	C.Text(Money(Info.BankrollCents), 58.0f + Cw + 22.0f, 102.0f, Ts(20.0f, 800, MenuGold));
	if (Info.Life)
	{
		DrawVitals(C, *Info.Life, 58.0f, 128.0f, 380.0f, Now, nullptr, 0.95f);
		// An order on its way (Penny Drop): how long until the soonest one is at the door, so the player knows to head home.
		if (Info.World >= 0.0 && !Info.Life->Deliveries.empty())
		{
			double Soonest = Info.Life->Deliveries.front().ArriveAt;
			for (const life::State::Delivery& Dv : Info.Life->Deliveries)
			{
				Soonest = std::min(Soonest, Dv.ArriveAt);
			}
			const int Mins = static_cast<int>(std::ceil(std::max(0.0, Soonest - Info.World)));
			const std::string Label = "PENNY DROP \xC2\xB7 " + (Mins <= 1 ? std::string("ANY MINUTE") : std::to_string(Mins) + " MIN");
			const float Lw = TrackedWidth(C, Label, 11.0f, 800, 2.4f);
			const Rect Chip{58.0f, 238.0f, Lw + 40.0f, 26.0f};
			C.FillRoundRect(Chip, 13.0f, Paint(Rgba(215, 38, 61, 0.88f)));
			const float Beat = 0.55f + 0.45f * static_cast<float>(std::sin(Now * 3.0));
			C.FillCircle(Chip.X + 15.0f, Chip.Y + 13.0f, 4.0f, Paint(Rgba(255, 255, 255, Beat)));
			TrackedText(C, Label, Chip.X + 28.0f, Chip.Y + 17.5f, 11.0f, 800, Hex(0xffffff), 2.4f);
		}
	}
	// The camera, top right.
	const std::string Mode = Info.FirstPerson ? "FIRST PERSON" : "THIRD PERSON";
	const float Mw = TrackedWidth(C, Mode, 12.0f, 800, 3.0f);
	const float Gw = Glyph(C, Info.Gamepad ? "Y" : "V", W - 58.0f - Mw - 46.0f, 66.0f, 0.85f);
	(void)Gw;
	TrackedText(C, Mode, W - 58.0f, 62.0f, 12.0f, 800, Fade(MenuInk, 0.8f), 3.0f, Align::Right);
	// Texts, top right under the camera chip: newest first, each for eight seconds.
	float Ty = 104.0f;
	for (auto It = Info.Toasts.rbegin(); It != Info.Toasts.rend(); ++It)
	{
		const double Age = Now - It->At;
		if (Age < 0.0 || Age > 8.0)
		{
			continue;
		}
		const float A = std::min(Ease(Age / 0.35), Ease((8.0 - Age) / 0.6));
		const float Tw = 420.0f;
		const Rect R{W - 58.0f - Tw + (1.0f - A) * 40.0f, Ty, Tw, 0.0f};
		const std::vector<std::string> Lines = WrapText(C, It->Body, Tw - 40.0f, 16.0f, 400);
		const float Th = 52.0f + Cf(static_cast<int>(std::min<size_t>(Lines.size(), 3))) * 22.0f;
		C.FillRoundRect({R.X, R.Y, Tw, Th}, 12.0f, Paint(Rgba(10, 13, 22, 0.86f * A)));
		C.FillRoundRect({R.X, R.Y, 4.0f, Th}, 2.0f, Paint(Fade(MenuTeal, A)));
		TrackedText(C, It->From, R.X + 20.0f, R.Y + 28.0f, 12.0f, 800, Fade(MenuTeal, A), 2.4f);
		float Ly = R.Y + 30.0f;
		for (size_t I = 0; I < Lines.size() && I < 3; ++I)
		{
			Ly += 22.0f;
			C.Text(Lines[I], R.X + 20.0f, Ly, Ts(16.0f, 400, Fade(MenuInk, 0.92f * A)));
		}
		Ty += Th + 10.0f;
	}
	// The prompt, bottom center.
	if (!Info.Prompt.empty())
	{
		const float Pw = TrackedWidth(C, Info.Prompt, 15.0f, 800, 2.6f) + 92.0f;
		const float Px = W * 0.5f - Pw * 0.5f;
		const float Py = H - 210.0f;
		C.FillRoundRect({Px, Py, Pw, 52.0f}, 26.0f, Paint(Rgba(8, 10, 16, 0.78f)));
		C.StrokeRoundRect({Px + 0.5f, Py + 0.5f, Pw - 1.0f, 51.0f}, 26.0f, Fade(MenuInk, 0.18f), 1.0f);
		Glyph(C, Info.Gamepad ? "A" : Info.PromptKey, Px + 14.0f, Py + 38.0f, 1.0f);
		TrackedText(C, Info.Prompt, Px + 62.0f, Py + 32.0f, 15.0f, 800, MenuInk, 2.6f);
	}
	// The controls, for the first while.
	const double Since = Now - Info.HintsAt;
	if (Since >= 0.0 && Since < 14.0)
	{
		const float A = std::min(Ease(Since / 0.6), Ease((14.0 - Since) / 1.0));
		float Hx = 58.0f;
		const float Hy = H - 62.0f;
		const std::pair<const char*, const char*> Hints[6] = {{Info.Gamepad ? "LS" : "WASD", "MOVE"}, {Info.Gamepad ? "LB" : "SHIFT", "RUN"}, {Info.Gamepad ? "Y" : "V", "1ST / 3RD PERSON"},
			{Info.Gamepad ? "A" : "E", "INTERACT"}, {Info.Gamepad ? "X" : "F", "EAT OR DRINK"}, {Info.Gamepad ? "B" : "ESC", "PAUSE"}};
		for (const std::pair<const char*, const char*>& Hp : Hints)
		{
			Hx += Glyph(C, Hp.first, Hx, Hy, A) + 10.0f;
			Hx += TrackedText(C, Hp.second, Hx, Hy - 2.0f, 13.0f, 700, Fade(MenuMuted, A), 2.6f) + 30.0f;
		}
	}
	if (Info.Fade > 0.0f)
	{
		C.FillRect({0.0f, 0.0f, W, H}, Paint(Rgba(0, 0, 0, std::min(1.0f, Info.Fade))));
	}
}

} // namespace ui
} // namespace ss
