// The Lucky Penny #212's counter: the shelves, the clerk, the receipt and the bag.
#include "ShortStack/UI/StoreCounter.h"
#include "../StrictFloat.h"
#include "FrontEndShared.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

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
	// What the basket would do if it were all eaten and drunk now.
	double Hunger = L.Hunger;
	double Thirst = L.Thirst;
	double Energy = L.Energy;
	if (Preview)
	{
		for (const std::pair<std::string, int>& Ln : Preview->Lines)
		{
			if (const store::Item* I = store::Find(Ln.first))
			{
				const double Boost = I->Food() ? L.Perks.MealBoost : 1.0;
				Hunger -= I->Hunger * Boost * Ln.second;
				Thirst -= I->Thirst * Ln.second;
				Energy += I->Energy * Boost * Ln.second;
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
		C.Text(R.Word, X + Width, Ry + 15.0f, Ts(13.0f, 700, Fade(Low ? Hex(0xff8a8d) : MenuInk, Alpha * 0.9f), Align::Right));
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
	Say(Was.Count() >= 4 ? "Have a good one. Don't eat it all at once." : "There you go. Stay dry out there.", Now, false);
}

void StoreCounter::Key(const std::string& Name, double Now)
{
	if (!Shown)
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
	else if (Name == "Left" || Name == "Right")
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
		Basket.Add(Items[static_cast<size_t>(Sel)]->Id);
		PaidAt = -10.0;
		DeclinedAt = -10.0;
		Say(S.ClerkSays(Basket), Now, false);
	}
	else if (Name == "Backspace" && Sel < N)
	{
		Basket.Remove(Items[static_cast<size_t>(Sel)]->Id);
		DeclinedAt = -10.0;
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

	// The sign.
	PennyLogo(C, Margin + 34.0f, 112.0f, 34.0f, 1.0f);
	TrackedText(C, "LUCKY PENNY", Margin + 86.0f, 116.0f, 38.0f, 900, MenuInk, 5.0f);
	TrackedText(C, "#212 \xC2\xB7 FIFTH & MARKET \xC2\xB7 OPEN 24 HOURS", Margin + 88.0f, 142.0f, 12.0f, 700, Copper, 3.0f);

	// The clerk.
	const float GridW = W - Margin * 2.0f - ReceiptW - 48.0f;
	const Rect Bubble{Margin + GridW * 0.48f, 70.0f, GridW * 0.52f, 96.0f};
	const float Na = Ease((Now - NoteAt) / 0.3);
	C.FillRoundRect(Bubble, 14.0f, Paint(Rgba(255, 255, 255, 0.06f * Na)));
	C.FillPolygon({{Bubble.X + 30.0f, Bubble.Y + Bubble.H}, {Bubble.X + 54.0f, Bubble.Y + Bubble.H}, {Bubble.X + 26.0f, Bubble.Y + Bubble.H + 16.0f}}, Paint(Rgba(255, 255, 255, 0.06f * Na)));
	TrackedText(C, "BENNY \xC2\xB7 NIGHT CLERK", Bubble.X + 22.0f, Bubble.Y + 28.0f, 11.0f, 800, Fade(NoteBad ? MenuWarn : MenuTeal, Na), 2.6f);
	float Ny = Bubble.Y + 34.0f;
	for (const std::string& Ln : WrapText(C, "\xE2\x80\x9C" + Note + "\xE2\x80\x9D", Bubble.W - 44.0f, 17.0f, 400))
	{
		Ny += 24.0f;
		C.Text(Ln, Bubble.X + 22.0f, Ny, Ts(17.0f, 400, Fade(MenuInk, 0.92f * Na)));
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

	// The shelf.
	const std::vector<const store::Item*> Items = OnShelf();
	const float Cw = (GridW - 40.0f) / 3.0f;
	const float Ch = 236.0f;
	for (size_t K = 0; K < Items.size(); ++K)
	{
		const store::Item& I = *Items[K];
		const int Idx = static_cast<int>(K);
		const float Ci = Ease((Now - OpenedAt - 0.04 * static_cast<double>(K)) / 0.35);
		const Rect R{Margin + Cf(Idx % 3) * (Cw + 20.0f), 268.0f + Cf(Idx / 3) * (Ch + 20.0f) + (1.0f - Ci) * 14.0f, Cw, Ch};
		const bool Focus = Idx == Sel;
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
		C.FillRoundRect(R, 6.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Rgba(24, 28, 38, 0.95f), Rgba(12, 14, 20, 0.95f)));
		C.StrokeRoundRect({R.X + 0.75f, R.Y + 0.75f, R.W - 1.5f, R.H - 1.5f}, 6.0f, Focus ? Copper : Fade(MenuInk, Over ? 0.22f : 0.08f), Focus ? 2.0f : 1.0f);
		// The shelf edge under the product.
		C.FillRect({R.X + 18.0f, R.Y + R.H - 46.0f, 150.0f, 4.0f}, Paint(Rgba(255, 255, 255, 0.08f)));
		DrawProduct(C, I, R.X + 93.0f, R.Y + R.H * 0.5f - 18.0f, 150.0f);
		const float Tx0 = R.X + 184.0f;
		const float Tw = R.W - 184.0f - 18.0f;
		C.Text(I.Name, Tx0, R.Y + 46.0f, Ts(22.0f, 900, MenuInk, Align::Left, Baseline::Alphabetic, false, Tw));
		C.Text(I.Kind, Tx0, R.Y + 68.0f, Ts(13.0f, 600, MenuMuted, Align::Left, Baseline::Alphabetic, false, Tw));
		float By = R.Y + 74.0f;
		for (const std::string& Ln : WrapText(C, I.Blurb, Tw, 13.0f, 400))
		{
			By += 18.0f;
			if (By > R.Y + 132.0f)
			{
				break;
			}
			C.Text(Ln, Tx0, By, Ts(13.0f, 400, Fade(MenuInk, 0.6f)));
		}
		// What it does.
		float Ex = Tx0;
		const double Boost = I.Food() ? S.Life.Perks.MealBoost : 1.0;
		const std::pair<int, double> Effects[3] = {{0, I.Hunger * Boost}, {1, I.Thirst}, {2, I.Energy * Boost}};
		for (const std::pair<int, double>& E : Effects)
		{
			if (std::fabs(E.second) < 0.5)
			{
				continue;
			}
			const bool Good = E.second > 0.0;
			const uint32_t Hue = E.first == 0 ? 0xff9f43u : E.first == 1 ? 0x4fb8ffu : 0xf2c14eu;
			const std::string Txt = (Good ? "+" : "\xE2\x88\x92") + std::to_string(static_cast<int>(std::lround(std::fabs(E.second))));
			const float Pw = C.Measure(Txt, 13.0f, 800) + 34.0f;
			if (Ex + Pw > R.X + R.W - 12.0f)
			{
				break;
			}
			C.FillRoundRect({Ex, R.Y + 146.0f, Pw, 24.0f}, 12.0f, Paint(Fade(Good ? Hex(Hue) : MenuWarn, 0.14f)));
			NeedIcon(C, E.first, Ex + 13.0f, R.Y + 158.0f, 13.0f, Good ? Hex(Hue) : MenuWarn);
			C.Text(Txt, Ex + 24.0f, R.Y + 163.0f, Ts(13.0f, 800, Good ? Hex(Hue) : MenuWarn));
			Ex += Pw + 6.0f;
		}
		// The shelf tag.
		const Rect Tag{R.X + R.W - 112.0f, R.Y + R.H - 52.0f, 94.0f, 36.0f};
		C.FillRect(Tag, Paint(Hex(0xffd23f)));
		C.FillRect({Tag.X, Tag.Y, 6.0f, Tag.H}, Paint(Hex(0xe23b4e)));
		C.Text(Money2(I.PriceCents), Tag.X + Tag.W * 0.5f + 3.0f, Tag.Y + 26.0f, Ts(20.0f, 900, Hex(0x1a1408), Align::Center));
		if (InBasket > 0)
		{
			C.FillRoundRect({R.X + R.W - 66.0f, R.Y + 14.0f, 52.0f, 26.0f}, 13.0f, Paint(Copper));
			C.Text("\xC3\x97" + std::to_string(InBasket), R.X + R.W - 40.0f, R.Y + 32.0f, Ts(14.0f, 900, Hex(0x1a0e04), Align::Center));
		}
		if (Released(R))
		{
			Sel = Idx;
			Basket.Add(I.Id);
			PaidAt = -10.0;
			DeclinedAt = -10.0;
			Say(S.ClerkSays(Basket), Now, false);
		}
	}

	// Vitals, and what's in the bag.
	const float Vy = 806.0f;
	TrackedText(C, "HOW YOU'RE DOING", Margin, Vy - 18.0f, 11.0f, 800, MenuMuted, 2.6f);
	DrawVitals(C, S.Life, Margin, Vy, 520.0f, Now, &Basket);
	const float Bx = Margin + 580.0f;
	TrackedText(C, "IN YOUR BAG", Bx, Vy - 18.0f, 11.0f, 800, MenuMuted, 2.6f);
	if (S.Life.Pantry.empty())
	{
		C.Text("Nothing yet.", Bx, Vy + 22.0f, Ts(15.0f, 400, MenuDim));
	}
	float Px = Bx;
	for (const auto& Held : S.Life.Pantry)
	{
		const store::Item* I = store::Find(Held.first);
		if (!I || Px > Margin + GridW - 120.0f)
		{
			continue;
		}
		const Rect Chip{Px, Vy - 4.0f, 118.0f, 100.0f};
		const bool Over = Inside(Chip, Ptr.X, Ptr.Y);
		C.FillRoundRect(Chip, 8.0f, Paint(Fade(MenuInk, Over ? 0.1f : 0.05f)));
		DrawProduct(C, *I, Chip.X + 30.0f, Chip.Y + 44.0f, 62.0f);
		C.Text("\xC3\x97" + std::to_string(Held.second), Chip.X + Chip.W - 12.0f, Chip.Y + 26.0f, Ts(15.0f, 900, MenuInk, Align::Right));
		TrackedText(C, I->Food() ? "EAT" : "DRINK", Chip.X + Chip.W - 12.0f, Chip.Y + 86.0f, 11.0f, 800, Over ? Copper : MenuMuted, 2.4f, Align::Right);
		if (Released(Chip))
		{
			const std::string Id = I->Id;
			S.Consume(Id);
			Say(I->Food() ? "Not in the store, man. Okay, fine. Napkins are by the door." : "Recycling's out front.", Now, false);
			break; // the bag changed under the loop
		}
		Px += Chip.W + 10.0f;
	}

	// The receipt.
	const float Rx = W - Margin - ReceiptW;
	float Ry = 86.0f;
	std::vector<std::pair<std::string, std::string>> Lines;
	const store::Basket& Shown2 = PaidAt > 0.0 && Basket.Empty() ? LastPaid : Basket;
	for (const std::pair<std::string, int>& Ln : Shown2.Lines)
	{
		const store::Item* I = store::Find(Ln.first);
		std::string Up = I ? I->Name : Ln.first;
		std::transform(Up.begin(), Up.end(), Up.begin(), [](char Cc) { return static_cast<char>(Cc >= 'a' && Cc <= 'z' ? Cc - 32 : Cc); });
		Lines.push_back({std::to_string(Ln.second) + " " + Up, Money2((I ? I->PriceCents : 0) * Ln.second)});
	}
	const float RowH = 26.0f;
	const float PaperH = 300.0f + Cf(std::max(1, static_cast<int>(Lines.size()))) * RowH;
	std::vector<Vec2> PaperShape = {{Rx, Ry}, {Rx + ReceiptW, Ry}};
	for (int K = 0; K <= 22; ++K)
	{
		PaperShape.push_back({Rx + ReceiptW - Cf(K) * ReceiptW / 22.0f, Ry + PaperH + (K % 2 == 0 ? 0.0f : 9.0f)});
	}
	C.GlowRoundRect({Rx, Ry + 8.0f, ReceiptW, PaperH}, 4.0f, Rgba(0, 0, 0, 0.5f), 26.0f);
	C.FillPolygon(PaperShape, Paint::Linear({0.0f, Ry}, {0.0f, Ry + PaperH}, Paper, Shade(Paper, -0.05f)));
	auto Mono = [&](const std::string& Txt, float Xx, float Yy, Align Al2, float Size = 16.0f, int Weight = 500) {
		C.Text(Txt, Xx, Yy, Ts(Size, Weight, PaperInk, Al2, Baseline::Alphabetic, true));
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
	for (const std::pair<std::string, std::string>& Ln : Lines)
	{
		Ry += RowH;
		Mono(Ln.first, Rx + 28.0f, Ry, Align::Left);
		Mono(Ln.second, Rx + ReceiptW - 28.0f, Ry, Align::Right);
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
	Ry += 30.0f;
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
	// Pay.
	const Rect PayR{Rx, 86.0f + PaperH + 36.0f, ReceiptW, 66.0f};
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
