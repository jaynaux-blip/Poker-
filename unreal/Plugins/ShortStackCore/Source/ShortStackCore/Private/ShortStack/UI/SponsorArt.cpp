#include "ShortStack/UI/SponsorArt.h"
#include "../StrictFloat.h"
#include "RiverLineShared.h"

#include "ShortStack/Game/Hero.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/UI/Portrait.h"
#include "ShortStack/UI/Ui.h"

#include <algorithm>
#include <cmath>

// The brands' art (next_segment_spec §3.9, §1.7). core-sponsor.
//
// sponsorart::Mark is the one source of a brand's logo: the Kast overlay, Mail's threads and contract card, the patch
// sheets (drawn once per brand into a texture by the host's FCoreCanvasTarget), the ambassador banner and can labels all
// call it, so the logo is identical wherever it shows. Every mark is vector, in the brand's own two colours, readable
// from 12 px (a Mail row) to a 512 x 340 patch. Fictional brands only.

namespace ss
{
namespace ui
{
namespace sponsorart
{
namespace sponsorart_detail
{
using namespace rlnet_detail;

/** The square a mark is drawn in: centred in R, its side the shorter of R's. */
struct Box
{
	float Cx = 0.0f;
	float Cy = 0.0f;
	float S = 0.0f; // side
	Vec2 At(float U, float V) const { return {Cx + (U - 0.5f) * S, Cy + (V - 0.5f) * S}; }
	float L(float U) const { return U * S; }
};

Box BoxIn(const Rect& R)
{
	Box B;
	B.S = std::min(R.W, R.H);
	B.Cx = R.X + R.W * 0.5f;
	B.Cy = R.Y + R.H * 0.5f;
	return B;
}

Color Darker(const Color& C, float T)
{
	return Mix(C, Hex(0x000000), T);
}

Color Lighter(const Color& C, float T)
{
	return Mix(C, Hex(0xffffff), T);
}

void Penny(Canvas& C, const Box& B, const Color& Field, const Color& Copper)
{
	// A copper coin with a raised rim and a four-leaf clover struck in it, on the store's green.
	C.FillRoundRect({B.Cx - B.S * 0.5f, B.Cy - B.S * 0.5f, B.S, B.S}, B.S * 0.22f, Field);
	const Vec2 O = B.At(0.5f, 0.5f);
	C.FillCircle(O.X, O.Y, B.L(0.36f), Paint::Radial({O.X - B.L(0.1f), O.Y - B.L(0.12f)}, 0.0f, O, B.L(0.36f), Lighter(Copper, 0.35f), 0.55f, Copper, Darker(Copper, 0.3f)));
	C.StrokeEllipse(O.X, O.Y, B.L(0.3f), B.L(0.3f), Darker(Copper, 0.35f), B.L(0.035f));
	for (int K = 0; K < 4; ++K)
	{
		const float A = Pi * 0.25f + static_cast<float>(K) * Pi * 0.5f;
		C.FillCircle(O.X + std::cos(A) * B.L(0.075f), O.Y + std::sin(A) * B.L(0.075f), B.L(0.075f), Darker(Copper, 0.45f));
	}
	C.StrokePolyline({{O.X, O.Y}, {O.X + B.L(0.05f), O.Y + B.L(0.19f)}}, false, Darker(Copper, 0.45f), B.L(0.03f), true);
}

void BarberPole(Canvas& C, const Box& B, const Color& Navy, const Color& Red)
{
	// The pole: a white capsule with red and navy stripes running round it, capped top and bottom.
	const Rect Pole{B.Cx - B.L(0.17f), B.Cy - B.L(0.36f), B.L(0.34f), B.L(0.72f)};
	C.FillRoundRect(Pole, B.L(0.17f), Hex(0xf4f1ea));
	C.PushClip(Pole);
	for (int K = -4; K < 6; ++K)
	{
		const float Y = Pole.Y + static_cast<float>(K) * B.L(0.16f);
		const Color Ink = K % 2 == 0 ? Red : Navy;
		C.FillPolygon({{Pole.X - B.L(0.05f), Y + B.L(0.12f)}, {Pole.X + Pole.W + B.L(0.05f), Y}, {Pole.X + Pole.W + B.L(0.05f), Y + B.L(0.06f)}, {Pole.X - B.L(0.05f), Y + B.L(0.18f)}}, Ink);
	}
	C.PopClip();
	C.StrokeRoundRect(Pole, B.L(0.17f), Navy, B.L(0.025f));
	C.FillRoundRect({B.Cx - B.L(0.22f), Pole.Y - B.L(0.06f), B.L(0.44f), B.L(0.07f)}, B.L(0.03f), Navy);
	C.FillRoundRect({B.Cx - B.L(0.22f), Pole.Y + Pole.H - B.L(0.01f), B.L(0.44f), B.L(0.07f)}, B.L(0.03f), Navy);
}

void Dumbbell(Canvas& C, const Box& B, const Color& Ground, const Color& Red)
{
	// Iron on a black plate: a dumbbell, its two plates a side in the gym's red.
	C.FillRoundRect({B.Cx - B.S * 0.5f, B.Cy - B.S * 0.5f, B.S, B.S}, B.S * 0.16f, Ground);
	C.FillRoundRect({B.Cx - B.L(0.3f), B.Cy - B.L(0.035f), B.L(0.6f), B.L(0.07f)}, B.L(0.03f), Hex(0xc9ccd1));
	for (const float Side : {-1.0f, 1.0f})
	{
		C.FillRoundRect({B.Cx + Side * B.L(0.22f) - B.L(0.06f), B.Cy - B.L(0.22f), B.L(0.12f), B.L(0.44f)}, B.L(0.04f), Red);
		C.FillRoundRect({B.Cx + Side * B.L(0.33f) - B.L(0.045f), B.Cy - B.L(0.15f), B.L(0.09f), B.L(0.3f)}, B.L(0.03f), Darker(Red, 0.25f));
	}
}

void Bolt(Canvas& C, const Box& B, const Color& Ground, const Color& Lime)
{
	// A can-black disc and a lime bolt through it.
	const Vec2 O = B.At(0.5f, 0.5f);
	C.FillCircle(O.X, O.Y, B.L(0.47f), Ground);
	C.StrokeEllipse(O.X, O.Y, B.L(0.43f), B.L(0.43f), Lime, B.L(0.04f));
	C.FillPolygon({B.At(0.56f, 0.12f), B.At(0.3f, 0.55f), B.At(0.48f, 0.55f), B.At(0.42f, 0.88f), B.At(0.72f, 0.42f), B.At(0.54f, 0.42f)}, Lime);
}

void Gauge(Canvas& C, const Box& B, const Color& Lime, const Color& Ink)
{
	// A tachometer pinned past the red: Overclock.
	const Vec2 O = B.At(0.5f, 0.58f);
	C.FillCircle(B.Cx, B.Cy, B.L(0.48f), Lime);
	C.StrokeArc(O.X, O.Y, B.L(0.3f), Pi * 0.85f, Pi * 2.15f, Ink, B.L(0.07f), true);
	C.StrokePolyline({O, {O.X + std::cos(Pi * 1.85f) * B.L(0.3f), O.Y + std::sin(Pi * 1.85f) * B.L(0.3f)}}, false, Ink, B.L(0.06f), true);
	C.FillCircle(O.X, O.Y, B.L(0.07f), Ink);
}

void Crest(Canvas& C, const Box& B, const Color& Ground)
{
	C.FillRoundRect({B.Cx - B.S * 0.5f, B.Cy - B.S * 0.5f, B.S, B.S}, B.S * 0.18f, Ground);
	rlnet_detail::NetEmbercrest(C, B.Cx, B.Cy - B.L(0.05f), B.L(0.82f), Ground);
}

void Ring(Canvas& C, const Box& B, const Color& Lime, const Color& Ink)
{
	// A halo of light around a lens.
	C.FillCircle(B.Cx, B.Cy, B.L(0.48f), Ink);
	C.StrokeEllipse(B.Cx, B.Cy, B.L(0.36f), B.L(0.36f), Lime, B.L(0.09f));
	C.FillCircle(B.Cx, B.Cy, B.L(0.16f), Hex(0x1c2733));
	C.FillCircle(B.Cx - B.L(0.05f), B.Cy - B.L(0.05f), B.L(0.05f), Lighter(Lime, 0.4f));
}

void Shield(Canvas& C, const Box& B, const Color& Blue, const Color& Ink)
{
	// A shield with a tunnel's mouth in it: TunnelRat.
	C.FillPolygon({B.At(0.5f, 0.04f), B.At(0.9f, 0.18f), B.At(0.86f, 0.58f), B.At(0.5f, 0.96f), B.At(0.14f, 0.58f), B.At(0.1f, 0.18f)}, Blue);
	const Vec2 O = B.At(0.5f, 0.6f);
	C.FillRect({O.X - B.L(0.2f), O.Y, B.L(0.4f), B.L(0.14f)}, Ink);
	C.FillCircle(O.X, O.Y, B.L(0.2f), Ink);
	C.FillCircle(O.X, O.Y + B.L(0.02f), B.L(0.08f), Lighter(Blue, 0.5f));
}

void Stack(Canvas& C, const Box& B, const Color& Amber, const Color& Ink)
{
	// Three bars stacked off-true, like a hoodie folded on a shelf: Stacked.
	C.FillRoundRect({B.Cx - B.S * 0.5f, B.Cy - B.S * 0.5f, B.S, B.S}, B.S * 0.2f, Ink);
	for (int K = 0; K < 3; ++K)
	{
		const float Y = B.Cy + B.L(0.16f) - static_cast<float>(K) * B.L(0.17f);
		const float X = B.Cx - B.L(0.3f) + static_cast<float>(K % 2) * B.L(0.05f);
		C.FillRoundRect({X, Y - B.L(0.06f), B.L(0.55f), B.L(0.12f)}, B.L(0.05f), K == 2 ? Amber : Darker(Amber, 0.2f + 0.15f * static_cast<float>(K)));
	}
}

void Range(Canvas& C, const Box& B, const Color& Indigo, const Color& Ink)
{
	// A thirteen-by-thirteen corner of a range chart, the playable hands lit: Rangewise.
	C.FillRoundRect({B.Cx - B.S * 0.5f, B.Cy - B.S * 0.5f, B.S, B.S}, B.S * 0.14f, Indigo);
	const float Cell = B.L(0.16f);
	for (int R = 0; R < 4; ++R)
	{
		for (int K = 0; K < 4; ++K)
		{
			const bool Lit = K + R < 5 && !(K == 3 && R == 1);
			C.FillRoundRect({B.Cx - B.L(0.34f) + static_cast<float>(K) * Cell * 1.12f, B.Cy - B.L(0.34f) + static_cast<float>(R) * Cell * 1.12f, Cell, Cell}, Cell * 0.2f,
				Lit ? Ink : NetA(Ink, 0.18f));
		}
	}
}

void Waves(Canvas& C, const Box& B, const Color& Teal, const Color& Ink)
{
	// Two river lines: RiverLine.
	C.FillRoundRect({B.Cx - B.S * 0.5f, B.Cy - B.S * 0.5f, B.S, B.S}, B.S * 0.24f, Ink);
	for (int K = 0; K < 2; ++K)
	{
		std::vector<Vec2> Line;
		for (int I = 0; I <= 16; ++I)
		{
			const float U = static_cast<float>(I) / 16.0f;
			Line.push_back({B.Cx - B.L(0.32f) + U * B.L(0.64f), B.Cy - B.L(0.08f) + static_cast<float>(K) * B.L(0.18f) + std::sin(U * Pi * 2.0f) * B.L(0.07f)});
		}
		C.StrokePolyline(Line, false, K == 0 ? Teal : Lighter(Teal, 0.4f), B.L(0.08f), true);
	}
}

void Chair(Canvas& C, const Box& B, const Color& Orange, const Color& Ink)
{
	// A high-backed chair in profile: Sitwell.
	C.FillCircle(B.Cx, B.Cy, B.L(0.48f), Orange);
	C.FillRoundRect({B.Cx - B.L(0.16f), B.Cy - B.L(0.32f), B.L(0.12f), B.L(0.42f)}, B.L(0.05f), Ink);
	C.FillRoundRect({B.Cx - B.L(0.16f), B.Cy + B.L(0.02f), B.L(0.36f), B.L(0.1f)}, B.L(0.04f), Ink);
	C.FillRect({B.Cx + B.L(0.01f), B.Cy + B.L(0.1f), B.L(0.05f), B.L(0.16f)}, Ink);
	C.FillRoundRect({B.Cx - B.L(0.14f), B.Cy + B.L(0.24f), B.L(0.34f), B.L(0.05f)}, B.L(0.025f), Ink);
}

void TwiceCards(Canvas& C, const Box& B, const Color& Slate, const Color& Gold)
{
	// Two river cards, the second run dealt over the first: RunItTwice.
	C.FillCircle(B.Cx, B.Cy, B.L(0.48f), Slate);
	C.StrokeEllipse(B.Cx, B.Cy, B.L(0.42f), B.L(0.42f), Gold, B.L(0.03f));
	const float W = B.L(0.24f);
	const float H = B.L(0.34f);
	C.FillRoundRect({B.Cx - B.L(0.22f), B.Cy - B.L(0.24f), W, H}, B.L(0.04f), Gold);
	C.FillRoundRect({B.Cx - B.L(0.22f) + B.L(0.03f), B.Cy - B.L(0.24f) + B.L(0.03f), W - B.L(0.06f), H - B.L(0.06f)}, B.L(0.025f), Slate);
	C.FillRoundRect({B.Cx - B.L(0.03f), B.Cy - B.L(0.1f), W, H}, B.L(0.04f), Gold);
	C.FillCircle(B.Cx - B.L(0.03f) + W * 0.5f, B.Cy - B.L(0.1f) + H * 0.5f, B.L(0.05f), Slate);
}

/** WCAG contrast between two 0xRRGGBB colours (1: the same, 21: black on white). */
float Contrast(uint32_t A, uint32_t B)
{
	auto Lum = [](uint32_t Rgb) {
		auto Lin = [](uint32_t Ch) {
			const float V = static_cast<float>(Ch & 0xffu) / 255.0f;
			return V <= 0.04045f ? V / 12.92f : std::pow((V + 0.055f) / 1.055f, 2.4f);
		};
		return 0.2126f * Lin(Rgb >> 16) + 0.7152f * Lin(Rgb >> 8) + 0.0722f * Lin(Rgb);
	};
	const float La = Lum(A);
	const float Lb = Lum(B);
	return (std::max(La, Lb) + 0.05f) / (std::min(La, Lb) + 0.05f);
}

/** A brand's word mark in its own weight ("LUCKY PENNY"), fitted to MaxW. */
float WordMark(Canvas& C, const std::string& Words, float X, float Y, float Size, const Color& Ink, float MaxW, Align A)
{
	float S = Size;
	while (S > 8.0f && C.Measure(Words, S, 900) > MaxW)
	{
		S -= 1.0f;
	}
	return C.Text(Words, X, Y, Ts(S, 900, Ink, A));
}

std::string Upper(const std::string& In)
{
	std::string Out = In;
	for (char& Ch : Out)
	{
		Ch = Ch >= 'a' && Ch <= 'z' ? static_cast<char>(Ch - 'a' + 'A') : Ch;
	}
	return Out;
}
} // namespace sponsorart_detail

void Mark(Canvas& C, const std::string& BrandId, const Rect& R)
{
	using namespace sponsorart_detail;
	const sponsor::Brand* Br = sponsor::FindBrand(BrandId);
	const Box B = BoxIn(R);
	if (!Br || B.S <= 0.0f)
	{
		return;
	}
	const Color Main = Hex(Br->Color);
	const Color Ink = Hex(Br->Ink);
	switch (Br->Mark)
	{
	case 0: Penny(C, B, Main, Ink); break;
	case 1: BarberPole(C, B, Main, Ink); break;
	case 2: Dumbbell(C, B, Main, Ink); break;
	case 3: Bolt(C, B, Main, Ink); break;
	case 4: Gauge(C, B, Main, Ink); break;
	case 5: Crest(C, B, Main); break;
	case 6: Ring(C, B, Main, Ink); break;
	case 7: Shield(C, B, Main, Ink); break;
	case 8: Stack(C, B, Main, Ink); break;
	case 9: Range(C, B, Main, Ink); break;
	case 10: Waves(C, B, Main, Ink); break;
	case 11: Chair(C, B, Main, Ink); break;
	default: TwiceCards(C, B, Main, Ink); break;
	}
}

bool PatchColors(const std::string& BrandId, uint32_t& Ground, uint32_t& Thread)
{
	using namespace sponsorart_detail;
	const sponsor::Brand* Br = sponsor::FindBrand(BrandId);
	if (!Br)
	{
		return false;
	}
	// Six brands' patches are twill in a colour of their own (the Lucky Penny's green, the barber's cream, Team
	// Embercrest's, Stacked's, Team RiverLine's and RunItTwice's brand colour); the rest sit on near-black twill.
	const bool Dark = Br->Mark == 0 || Br->Mark == 1 || Br->Mark == 5 || Br->Mark == 8 || Br->Mark == 10 || Br->Mark == 12;
	Ground = Br->Mark == 0 ? 0x1f5c3au : Br->Mark == 1 ? 0xf4f1eau : Dark ? Br->Color : 0x16181du;
	Thread = Br->Mark == 1 ? Br->Color : Br->Mark == 0 ? Br->Ink : Dark ? Br->Ink : Br->Color;
	// The thread has to read on the twill, as the art's atlas stitches it (art/blender/sponsors.py): where the usual pick
	// is lost on the ground (IronHouse's dark grey, Volt Rush's near-black, both on the dark patch) it's the brand's other
	// colour.
	const uint32_t Other = Thread == Br->Color ? Br->Ink : Br->Color;
	if (Contrast(Thread, Ground) < 2.0f && Contrast(Other, Ground) > Contrast(Thread, Ground))
	{
		Thread = Other;
	}
	return true;
}

void Patch(Canvas& C, const std::string& BrandId)
{
	using namespace sponsorart_detail;
	using namespace rlnet_detail;
	const sponsor::Brand* Br = sponsor::FindBrand(BrandId);
	if (!Br)
	{
		return;
	}
	// A 512 x 340 sheet: the twill in the brand's ground, a merrowed edge in thread, the mark and the word mark.
	const float W = 512.0f;
	const float H = 340.0f;
	uint32_t GroundRgb = 0;
	uint32_t ThreadRgb = 0;
	PatchColors(BrandId, GroundRgb, ThreadRgb);
	const Color Ground = Hex(GroundRgb);
	const Color Thread = Hex(ThreadRgb);
	C.FillRoundRect({0.0f, 0.0f, W, H}, 46.0f, Ground);
	// The twill's diagonal weave.
	C.PushClip({0.0f, 0.0f, W, H});
	for (int K = -20; K < 40; ++K)
	{
		const float X = static_cast<float>(K) * 18.0f;
		C.StrokePolyline({{X, 0.0f}, {X + H, H}}, false, NetA(Br->Mark == 1 ? Hex(0x000000) : Hex(0xffffff), 0.05f), 4.0f);
	}
	C.PopClip();
	// The merrowed edge: a thick satin border, then its stitch line.
	C.StrokeRoundRect({9.0f, 9.0f, W - 18.0f, H - 18.0f}, 40.0f, Thread, 14.0f);
	C.StrokeRoundRect({22.0f, 22.0f, W - 44.0f, H - 44.0f}, 30.0f, NetA(Thread, 0.6f), 2.0f);
	Mark(C, BrandId, {48.0f, 74.0f, 190.0f, 190.0f});
	const Color Words = Br->Mark == 1 ? Hex(Br->Color) : Thread;
	WordMark(C, Upper(Br->Short), 260.0f, 160.0f, 52.0f, Words, 214.0f, Align::Left);
	std::string Under = Br->Cat == sponsor::Category::Local ? std::string("NORTHSIDE") : Br->Id == "embercrest" ? std::string("POKER ROOM") : std::string(sponsor::CategoryWord(Br->Cat));
	std::replace(Under.begin(), Under.end(), '-', ' '); // "ENERGY DRINK", "POKER SITE": a patch's words, not a letter's
	C.Text(Upper(Under), 262.0f, 204.0f, Ts(24.0f, 700, NetA(Words, 0.8f)));
}

void Polaroid(Canvas& C, const hero::Look& L, const std::string& Caption)
{
	// 512 x 512: the white frame with its deep bottom, the photo flashed against the store's wall, Sharpie underneath.
	C.FillRect({0.0f, 0.0f, 512.0f, 512.0f}, Hex(0x2a2520));
	C.FillRoundRect({28.0f, 20.0f, 456.0f, 472.0f}, 6.0f, Hex(0xf6f3ec));
	const Rect Photo{56.0f, 48.0f, 400.0f, 340.0f};
	C.FillRect(Photo, Paint::Radial({Photo.X + 200.0f, Photo.Y + 110.0f}, 0.0f, {Photo.X + 200.0f, Photo.Y + 170.0f}, 300.0f, Hex(0xf2e6c8), 0.45f, Hex(0xb9a27a), Hex(0x5a4a36)));
	C.PushClip(Photo);
	// The lotto board behind them, as a flash finds it.
	C.FillRect({Photo.X + 250.0f, Photo.Y + 20.0f, 130.0f, 90.0f}, Hex(0x1e3a8a, 0.85f));
	for (int K = 0; K < 3; ++K)
	{
		C.FillRect({Photo.X + 262.0f, Photo.Y + 34.0f + static_cast<float>(K) * 24.0f, 106.0f, 12.0f}, Hex(0xfacc15, 0.8f));
	}
	hero::Character Who;
	Who.Appearance = L;
	DrawPortrait(C, Who, Photo.X + 180.0f, Photo.Y + 150.0f, 0.78f, 0.0);
	// The flash's hot spot and the corners falling off.
	C.FillCircle(Photo.X + 200.0f, Photo.Y + 120.0f, 220.0f, Paint::Radial({Photo.X + 200.0f, Photo.Y + 120.0f}, 0.0f, {Photo.X + 200.0f, Photo.Y + 120.0f}, 220.0f, Hex(0xffffff, 0.22f), -1.0f, Color(), Hex(0xffffff, 0.0f)));
	C.PopClip();
	C.StrokeRoundRect(Photo, 2.0f, Hex(0x000000, 0.25f), 2.0f);
	sponsorart_detail::WordMark(C, Caption, 256.0f, 452.0f, 42.0f, Hex(0x1b1b1f), 380.0f, Align::Center);
}

void AmbassadorBanner(Canvas& C, const std::string& Name, const std::string& Record)
{
	using namespace sponsorart_detail;
	using namespace rlnet_detail;
	// The banner by the desk: the crest high on the room's navy, TEAM EMBERCREST, the player's name, their record.
	const float W = C.Width();
	const float H = C.Height();
	C.FillRect({0.0f, 0.0f, W, H}, Paint::Linear({0.0f, 0.0f}, {0.0f, H}, Hex(rlnet_detail::EcNavy), Hex(rlnet_detail::EcMidnight)));
	C.FillRect({0.0f, 0.0f, W, H * 0.012f}, Hex(rlnet_detail::EcEmberHot));
	C.FillRect({0.0f, H - H * 0.012f, W, H * 0.012f}, Hex(rlnet_detail::EcEmberHot));
	const float Unit = std::min(W, H * 0.5f);
	rlnet_detail::NetEmbercrest(C, W * 0.5f, H * 0.22f, Unit * 0.55f, Hex(rlnet_detail::EcNavy));
	NetSpaced(C, "TEAM EMBERCREST", W * 0.5f, H * 0.47f, Unit * 0.075f, 900, Hex(rlnet_detail::EcGold), Unit * 0.012f, Align::Center);
	WordMark(C, Upper(Name), W * 0.5f, H * 0.66f, Unit * 0.16f, Hex(rlnet_detail::EcAsh), W * 0.86f, Align::Center);
	if (!Record.empty())
	{
		C.Text(Record, W * 0.5f, H * 0.77f, Ts(Unit * 0.055f, 600, Hex(rlnet_detail::EcIce), Align::Center, Baseline::Alphabetic, false, W * 0.86f));
	}
	C.Text("THE EMBERCREST CASINO \xC2\xB7 POKER ROOM", W * 0.5f, H * 0.92f, Ts(Unit * 0.04f, 700, NetA(Hex(rlnet_detail::EcIce), 0.7f), Align::Center));
}
} // namespace sponsorart
} // namespace ui
} // namespace ss
