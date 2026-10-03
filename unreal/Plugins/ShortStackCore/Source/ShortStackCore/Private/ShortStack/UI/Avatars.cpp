// RiverLine profile pictures: vector icons on colored discs, picked by screen name.
#include "ShortStack/UI/Avatars.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Chat.h"
#include "ShortStack/UI/Ui.h"

#include <cctype>
#include <cmath>

namespace ss
{
namespace ui
{
namespace avatars_detail
{
struct Swatch
{
	uint32_t Bg;
	uint32_t Bg2;
	uint32_t Ink;
};

// Disc gradients that sit well together on the dark client: bright on top, deep below.
const Swatch Swatches[18] = {
	{0x2dd4bf, 0x0e7490, 0xffffff}, {0xf2c14e, 0xc2410c, 0x1f1305}, {0xf87171, 0x9f1239, 0xffffff}, {0xa78bfa, 0x4c1d95, 0xffffff}, {0x4ade80, 0x166534, 0xffffff},
	{0xf472b6, 0x9d174d, 0xffffff}, {0x60a5fa, 0x1e3a8a, 0xffffff}, {0xfb923c, 0x9a3412, 0xffffff}, {0xbef264, 0x3f6212, 0x1a2e05}, {0x22d3ee, 0x164e63, 0xffffff},
	{0xe879f9, 0x701a75, 0xffffff}, {0xfde047, 0xa16207, 0x1f1305}, {0x94a3b8, 0x1e293b, 0xffffff}, {0x475569, 0x0b1120, 0xffffff}, {0xfda4af, 0xbe123c, 0xffffff},
	{0x5eead4, 0x115e59, 0x042f2e}, {0xc4b5fd, 0x5b21b6, 0xffffff}, {0xfcd34d, 0xb45309, 0x1f1305}};

/** A name read the way people read it: "C0ldSh4rk" is cold + shark, "lazy_owl" lazy + owl, "ICMhero" icm + hero. */
struct NameWords
{
	std::string Flat;               // lowercase, digits inside words read as letters
	std::vector<std::string> Words; // split on case changes, separators and digits
};

NameWords ReadName(const std::string& Name)
{
	std::string Plain = Name;
	for (size_t I = 0; I < Plain.size(); ++I)
	{
		const auto IsAlpha = [&](size_t K) { return K < Plain.size() && std::isalpha(static_cast<unsigned char>(Name[K])); };
		const char Ch = Name[I];
		const bool Lone = !(I > 0 && std::isdigit(static_cast<unsigned char>(Name[I - 1]))) && !(I + 1 < Name.size() && std::isdigit(static_cast<unsigned char>(Name[I + 1])));
		if (Lone && ((I > 0 && IsAlpha(I - 1)) || IsAlpha(I + 1)))
		{
			Plain[I] = Ch == '0' ? 'o' : Ch == '3' ? 'e' : Ch == '1' ? 'i' : Ch == '4' ? 'a' : Ch == '5' ? 's' : Ch;
		}
	}
	NameWords Out;
	std::string Word;
	const auto Flush = [&]() {
		if (!Word.empty())
		{
			Out.Words.push_back(Word);
			Word.clear();
		}
	};
	for (size_t I = 0; I < Plain.size(); ++I)
	{
		const unsigned char Ch = static_cast<unsigned char>(Plain[I]);
		if (!std::isalpha(Ch))
		{
			Flush();
			continue;
		}
		const bool AfterLower = I > 0 && std::islower(static_cast<unsigned char>(Plain[I - 1]));
		const bool EndsCaps = I > 0 && std::isupper(static_cast<unsigned char>(Plain[I - 1])) && I + 1 < Plain.size() && std::islower(static_cast<unsigned char>(Plain[I + 1]));
		if (std::isupper(Ch) && (AfterLower || EndsCaps)) // "PSchmidt" is P + Schmidt
		{
			Flush();
		}
		Word.push_back(static_cast<char>(std::tolower(Ch)));
	}
	Flush();
	for (const char Ch : Plain)
	{
		Out.Flat.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(Ch))));
	}
	return Out;
}

/** Long keywords match anywhere ("Kartenhai" has no hai, but "LoSqualo" has squalo); short ones only as whole words, so "Volkov" is no wolf. */
bool Says(const NameWords& N, std::initializer_list<const char*> Anywhere, std::initializer_list<const char*> Whole = {})
{
	for (const char* W : Anywhere)
	{
		if (N.Flat.find(W) != std::string::npos)
		{
			return true;
		}
	}
	for (const char* W : Whole)
	{
		const std::string K = W;
		for (const std::string& Word : N.Words)
		{
			if (Word == K || Word == K + "s")
			{
				return true;
			}
		}
	}
	return false;
}

std::string InitialsOf(const NameWords& N)
{
	std::string Out;
	for (const std::string& Word : N.Words)
	{
		Out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(Word[0]))));
		if (Out.size() == 2)
		{
			break;
		}
	}
	return Out.empty() ? std::string("?") : Out;
}

void StarShape(Canvas& C, float Cx, float Cy, float R, const Color& Col)
{
	std::vector<Vec2> P;
	for (int I = 0; I < 10; ++I)
	{
		const float A = -Pi / 2.0f + static_cast<float>(I) * Pi / 5.0f;
		const float Rr = I % 2 == 0 ? R : R * 0.42f;
		P.push_back({Cx + Rr * std::cos(A), Cy + Rr * std::sin(A)});
	}
	C.FillPolygon(P, Col);
}

/** The icon, in units of S around (Cx, Cy): the drawing fits in about -0.9..0.9. */
void DrawIcon(Canvas& C, const AvatarSpec& A, float Cx, float Cy, float S, float R)
{
	const Color Ink = Hex(A.Ink);
	const Color Deep = Hex(A.Bg2);
	const Color Dark = Hex(0x111318);
	const Color White = Hex(0xffffff);
	auto P = [&](float X, float Y) { return Vec2{Cx + X * S, Cy + Y * S}; };
	auto Disc = [&](float X, float Y, float Rr, const Paint& Pt) { C.FillCircle(Cx + X * S, Cy + Y * S, Rr * S, Pt); };
	auto Oval = [&](float X, float Y, float Rx, float Ry, const Paint& Pt) { C.FillEllipse(Cx + X * S, Cy + Y * S, Rx * S, Ry * S, Pt); };
	auto Line = [&](std::initializer_list<Vec2> Pts, const Color& Col, float W) { C.StrokePolyline(std::vector<Vec2>(Pts), false, Col, W * S, true); };
	auto Poly = [&](std::initializer_list<Vec2> Pts, const Paint& Pt) { C.FillPolygon(std::vector<Vec2>(Pts), Pt); };
	switch (A.Icon)
	{
	case AvatarIcon::Initials:
		C.Text(A.Initials, Cx, Cy + R * 0.03f, Ts(R * (A.Initials.size() > 1 ? 0.74f : 0.9f), 800, Ink, Align::Center, Baseline::Middle));
		break;
	case AvatarIcon::Shark:
	{
		// A dorsal fin: a bowed leading edge up to the tip, a hollow trailing edge back down to the water.
		std::vector<Vec2> Fin;
		for (int I = 0; I <= 12; ++I)
		{
			const float T = static_cast<float>(I) / 12.0f;
			const float U = 1.0f - T;
			Fin.push_back(P(U * U * -0.62f + 2.0f * U * T * -0.24f + T * T * 0.3f, U * U * 0.34f + 2.0f * U * T * -0.52f + T * T * -0.84f));
		}
		for (int I = 1; I <= 12; ++I)
		{
			const float T = static_cast<float>(I) / 12.0f;
			const float U = 1.0f - T;
			Fin.push_back(P(U * U * 0.3f + 2.0f * U * T * 0.16f + T * T * 0.52f, U * U * -0.84f + 2.0f * U * T * 0.0f + T * T * 0.34f));
		}
		C.FillPolygon(Fin, Ink);
		for (int K = 0; K < 2; ++K)
		{
			std::vector<Vec2> Wave;
			for (int I = 0; I <= 16; ++I)
			{
				const float X = -0.86f + 1.72f * static_cast<float>(I) / 16.0f;
				Wave.push_back(P(X, 0.4f + 0.24f * static_cast<float>(K) + std::sin(X * 7.0f + static_cast<float>(K)) * 0.06f));
			}
			C.StrokePolyline(Wave, false, Color{Ink.R, Ink.G, Ink.B, K == 0 ? 1.0f : 0.55f}, 0.11f * S, true);
		}
		break;
	}
	case AvatarIcon::Fish:
		Poly({P(0.3f, 0.0f), P(0.9f, -0.45f), P(0.76f, 0.0f), P(0.9f, 0.45f)}, Ink);
		Poly({P(-0.3f, -0.3f), P(0.02f, -0.62f), P(0.14f, -0.28f)}, Ink);
		Oval(-0.12f, 0.0f, 0.6f, 0.38f, Ink);
		Disc(-0.45f, -0.08f, 0.09f, Deep);
		Line({P(-0.68f, 0.12f), P(-0.56f, 0.1f)}, Deep, 0.06f);
		break;
	case AvatarIcon::Whale:
		Poly({P(0.5f, 0.12f), P(0.94f, -0.42f), P(0.78f, 0.02f), P(0.98f, 0.12f), P(0.62f, 0.3f)}, Ink);
		Oval(-0.1f, 0.18f, 0.72f, 0.44f, Ink);
		Oval(-0.14f, 0.42f, 0.5f, 0.15f, Mix(Ink, Deep, 0.2f));
		Disc(-0.48f, 0.08f, 0.07f, Deep);
		Line({P(-0.16f, -0.3f), P(-0.26f, -0.52f), P(-0.42f, -0.68f)}, Ink, 0.09f);
		Line({P(-0.16f, -0.3f), P(-0.06f, -0.52f), P(0.08f, -0.68f)}, Ink, 0.09f);
		break;
	case AvatarIcon::Owl:
		Poly({P(-0.62f, -0.3f), P(-0.58f, -0.9f), P(-0.25f, -0.5f)}, Ink);
		Poly({P(0.62f, -0.3f), P(0.58f, -0.9f), P(0.25f, -0.5f)}, Ink);
		Oval(0.0f, 0.12f, 0.7f, 0.74f, Ink);
		for (int K = -1; K <= 1; K += 2)
		{
			Disc(0.3f * static_cast<float>(K), -0.02f, 0.29f, Deep);
			Disc(0.3f * static_cast<float>(K), -0.02f, 0.15f, Hex(0xfde047));
			Disc(0.3f * static_cast<float>(K), -0.02f, 0.07f, Dark);
		}
		Poly({P(-0.1f, 0.2f), P(0.1f, 0.2f), P(0.0f, 0.4f)}, Hex(0xf59e0b));
		break;
	case AvatarIcon::Cat:
		Poly({P(-0.64f, -0.06f), P(-0.54f, -0.84f), P(-0.12f, -0.42f)}, Ink);
		Poly({P(0.64f, -0.06f), P(0.54f, -0.84f), P(0.12f, -0.42f)}, Ink);
		Poly({P(-0.52f, -0.24f), P(-0.48f, -0.62f), P(-0.26f, -0.42f)}, Hex(0xf9a8d4));
		Poly({P(0.52f, -0.24f), P(0.48f, -0.62f), P(0.26f, -0.42f)}, Hex(0xf9a8d4));
		Oval(0.0f, 0.14f, 0.66f, 0.55f, Ink);
		Oval(-0.24f, 0.06f, 0.07f, 0.13f, Deep);
		Oval(0.24f, 0.06f, 0.07f, 0.13f, Deep);
		Poly({P(-0.08f, 0.26f), P(0.08f, 0.26f), P(0.0f, 0.35f)}, Hex(0xf472b6));
		for (int K = -1; K <= 1; K += 2)
		{
			const float F = static_cast<float>(K);
			Line({P(0.16f * F, 0.34f), P(0.76f * F, 0.26f)}, Deep, 0.035f);
			Line({P(0.16f * F, 0.38f), P(0.74f * F, 0.46f)}, Deep, 0.035f);
		}
		break;
	case AvatarIcon::Fox:
	{
		const Color FoxFur = Hex(0xf97316);
		Poly({P(-0.74f, -0.28f), P(-0.6f, -0.92f), P(-0.18f, -0.48f)}, FoxFur);
		Poly({P(0.74f, -0.28f), P(0.6f, -0.92f), P(0.18f, -0.48f)}, FoxFur);
		Poly({P(-0.6f, -0.42f), P(-0.56f, -0.76f), P(-0.36f, -0.52f)}, Dark);
		Poly({P(0.6f, -0.42f), P(0.56f, -0.76f), P(0.36f, -0.52f)}, Dark);
		Poly({P(-0.82f, -0.34f), P(-0.4f, -0.5f), P(0.4f, -0.5f), P(0.82f, -0.34f), P(0.5f, 0.2f), P(0.0f, 0.78f), P(-0.5f, 0.2f)}, FoxFur);
		Poly({P(-0.82f, -0.34f), P(-0.08f, 0.12f), P(0.0f, 0.78f), P(-0.5f, 0.2f)}, White);
		Poly({P(0.82f, -0.34f), P(0.08f, 0.12f), P(0.0f, 0.78f), P(0.5f, 0.2f)}, White);
		Oval(-0.26f, -0.12f, 0.06f, 0.1f, Dark);
		Oval(0.26f, -0.12f, 0.06f, 0.1f, Dark);
		Disc(0.0f, 0.7f, 0.1f, Dark);
		break;
	}
	case AvatarIcon::Bear:
	{
		const Color Fur = Hex(0x9a5b2e);
		const Color Tan = Hex(0xe8b98a);
		Disc(-0.48f, -0.5f, 0.24f, Fur);
		Disc(0.48f, -0.5f, 0.24f, Fur);
		Disc(-0.48f, -0.5f, 0.12f, Tan);
		Disc(0.48f, -0.5f, 0.12f, Tan);
		Disc(0.0f, 0.06f, 0.64f, Fur);
		Oval(0.0f, 0.28f, 0.3f, 0.22f, Tan);
		Oval(0.0f, 0.18f, 0.1f, 0.07f, Dark);
		Disc(-0.25f, -0.08f, 0.07f, Dark);
		Disc(0.25f, -0.08f, 0.07f, Dark);
		break;
	}
	case AvatarIcon::Panda:
		Disc(-0.5f, -0.5f, 0.22f, Dark);
		Disc(0.5f, -0.5f, 0.22f, Dark);
		Disc(0.0f, 0.06f, 0.66f, White);
		Oval(-0.27f, 0.02f, 0.16f, 0.21f, Dark);
		Oval(0.27f, 0.02f, 0.16f, 0.21f, Dark);
		Disc(-0.25f, -0.02f, 0.06f, White);
		Disc(0.25f, -0.02f, 0.06f, White);
		Oval(0.0f, 0.3f, 0.1f, 0.07f, Dark);
		Line({P(-0.1f, 0.44f), P(0.0f, 0.38f), P(0.1f, 0.44f)}, Dark, 0.05f);
		break;
	case AvatarIcon::Skull:
		Disc(0.0f, -0.12f, 0.62f, Ink);
		C.FillRoundRect({Cx - 0.36f * S, Cy + 0.2f * S, 0.72f * S, 0.44f * S}, 0.12f * S, Ink);
		Disc(-0.25f, -0.08f, 0.17f, Deep);
		Disc(0.25f, -0.08f, 0.17f, Deep);
		Poly({P(0.0f, 0.12f), P(-0.08f, 0.28f), P(0.08f, 0.28f)}, Deep);
		for (int K = -1; K <= 1; ++K)
		{
			Line({P(0.15f * static_cast<float>(K), 0.44f), P(0.15f * static_cast<float>(K), 0.62f)}, Deep, 0.045f);
		}
		break;
	case AvatarIcon::Crown:
	{
		const Paint CrownGold = Paint::Linear(P(0.0f, -0.6f), P(0.0f, 0.6f), Hex(0xfff1a8), Hex(0xf59e0b));
		Poly({P(-0.76f, 0.44f), P(-0.84f, -0.38f), P(-0.4f, -0.02f), P(0.0f, -0.62f), P(0.4f, -0.02f), P(0.84f, -0.38f), P(0.76f, 0.44f)}, CrownGold);
		C.FillRoundRect({Cx - 0.78f * S, Cy + 0.38f * S, 1.56f * S, 0.2f * S}, 0.06f * S, Hex(0xd97706));
		Disc(0.0f, -0.68f, 0.09f, Hex(0xfff1a8));
		Disc(-0.86f, -0.44f, 0.08f, Hex(0xfff1a8));
		Disc(0.86f, -0.44f, 0.08f, Hex(0xfff1a8));
		Disc(0.0f, 0.18f, 0.1f, Hex(0xef4444));
		Disc(-0.42f, 0.24f, 0.07f, Hex(0x3b82f6));
		Disc(0.42f, 0.24f, 0.07f, Hex(0x22c55e));
		break;
	}
	case AvatarIcon::Gem:
		Poly({P(-0.76f, -0.2f), P(-0.42f, -0.6f), P(0.42f, -0.6f), P(0.76f, -0.2f), P(0.0f, 0.78f)}, Paint::Linear(P(0.0f, -0.6f), P(0.0f, 0.78f), Ink, Mix(Ink, Deep, 0.35f)));
		Line({P(-0.76f, -0.2f), P(0.76f, -0.2f)}, Color{Deep.R, Deep.G, Deep.B, 0.5f}, 0.04f);
		Line({P(-0.42f, -0.6f), P(-0.2f, -0.2f), P(0.0f, 0.78f), P(0.2f, -0.2f), P(0.42f, -0.6f)}, Color{Deep.R, Deep.G, Deep.B, 0.5f}, 0.04f);
		Line({P(-0.2f, -0.2f), P(0.0f, -0.6f), P(0.2f, -0.2f)}, Color{Deep.R, Deep.G, Deep.B, 0.5f}, 0.04f);
		break;
	case AvatarIcon::Spade:
	case AvatarIcon::Heart:
	case AvatarIcon::Club:
	case AvatarIcon::Diamond:
	{
		const int Suit = A.Icon == AvatarIcon::Club ? 0 : A.Icon == AvatarIcon::Diamond ? 1 : A.Icon == AvatarIcon::Heart ? 2 : 3;
		DrawSuit(C, Suit, Cx - 0.72f * S, Cy - 0.72f * S, 1.44f * S, Ink);
		break;
	}
	case AvatarIcon::Dice:
		C.FillRoundRect({Cx - 0.62f * S, Cy - 0.62f * S, 1.24f * S, 1.24f * S}, 0.26f * S, Ink);
		Disc(-0.3f, -0.3f, 0.12f, Deep);
		Disc(0.3f, -0.3f, 0.12f, Deep);
		Disc(0.0f, 0.0f, 0.12f, Deep);
		Disc(-0.3f, 0.3f, 0.12f, Deep);
		Disc(0.3f, 0.3f, 0.12f, Deep);
		break;
	case AvatarIcon::Clover:
		Line({P(0.05f, 0.25f), P(0.18f, 0.55f), P(0.4f, 0.82f)}, Ink, 0.12f);
		Disc(0.0f, -0.34f, 0.3f, Ink);
		Disc(0.34f, 0.0f, 0.3f, Ink);
		Disc(0.0f, 0.34f, 0.3f, Ink);
		Disc(-0.34f, 0.0f, 0.3f, Ink);
		Disc(0.0f, 0.0f, 0.12f, Mix(Ink, Deep, 0.25f));
		break;
	case AvatarIcon::Rocket:
	{
		// Painted its own colors so it reads on any disc.
		const Color Hull = Hex(0xf1f5f9);
		Poly({P(-0.17f, 0.54f), P(0.0f, 0.98f), P(0.17f, 0.54f)}, Hex(0xfb923c));
		Poly({P(-0.1f, 0.54f), P(0.0f, 0.78f), P(0.1f, 0.54f)}, Hex(0xfde047));
		Poly({P(-0.28f, 0.14f), P(-0.6f, 0.6f), P(-0.22f, 0.52f)}, Hex(0xef4444));
		Poly({P(0.28f, 0.14f), P(0.6f, 0.6f), P(0.22f, 0.52f)}, Hex(0xef4444));
		Oval(0.0f, -0.04f, 0.3f, 0.68f, Paint::Linear(P(-0.3f, 0.0f), P(0.3f, 0.0f), Hull, Hex(0xb6c2d1)));
		Poly({P(-0.2f, -0.56f), P(0.0f, -0.74f), P(0.2f, -0.56f)}, Hex(0xef4444));
		Disc(0.0f, -0.16f, 0.15f, Hex(0x1e3a8a));
		Disc(-0.04f, -0.2f, 0.06f, Hex(0x93c5fd));
		C.StrokeEllipse(Cx, Cy - 0.16f * S, 0.15f * S, 0.15f * S, Hex(0x64748b), 0.05f * S);
		break;
	}
	case AvatarIcon::Robot:
		Line({P(0.0f, -0.48f), P(0.0f, -0.76f)}, Ink, 0.07f);
		Disc(0.0f, -0.8f, 0.1f, Hex(0xef4444));
		C.FillRoundRect({Cx - 0.78f * S, Cy - 0.18f * S, 0.2f * S, 0.4f * S}, 0.06f * S, Ink);
		C.FillRoundRect({Cx + 0.58f * S, Cy - 0.18f * S, 0.2f * S, 0.4f * S}, 0.06f * S, Ink);
		C.FillRoundRect({Cx - 0.62f * S, Cy - 0.5f * S, 1.24f * S, 1.0f * S}, 0.2f * S, Ink);
		Disc(-0.26f, -0.1f, 0.15f, Deep);
		Disc(0.26f, -0.1f, 0.15f, Deep);
		Disc(-0.26f, -0.1f, 0.06f, Hex(0x67e8f9));
		Disc(0.26f, -0.1f, 0.06f, Hex(0x67e8f9));
		C.FillRoundRect({Cx - 0.3f * S, Cy + 0.2f * S, 0.6f * S, 0.15f * S}, 0.05f * S, Deep);
		for (int K = -1; K <= 1; ++K)
		{
			Line({P(0.1f * static_cast<float>(K), 0.21f), P(0.1f * static_cast<float>(K), 0.34f)}, Ink, 0.03f);
		}
		break;
	case AvatarIcon::Alien:
	{
		const Color AlienSkin = Hex(0x86efac);
		Oval(0.0f, -0.1f, 0.58f, 0.62f, AlienSkin);
		Poly({P(-0.5f, 0.05f), P(0.0f, 0.78f), P(0.5f, 0.05f)}, AlienSkin);
		Poly({P(-0.48f, -0.12f), P(-0.1f, 0.02f), P(-0.16f, 0.22f), P(-0.42f, 0.1f)}, Dark);
		Poly({P(0.48f, -0.12f), P(0.1f, 0.02f), P(0.16f, 0.22f), P(0.42f, 0.1f)}, Dark);
		Line({P(-0.1f, 0.48f), P(0.1f, 0.48f)}, Hex(0x166534), 0.04f);
		break;
	}
	case AvatarIcon::Shades:
		Disc(0.0f, 0.0f, 0.74f, Hex(0xfacc15));
		C.FillRoundRect({Cx - 0.62f * S, Cy - 0.24f * S, 0.54f * S, 0.32f * S}, 0.1f * S, Dark);
		C.FillRoundRect({Cx + 0.08f * S, Cy - 0.24f * S, 0.54f * S, 0.32f * S}, 0.1f * S, Dark);
		Line({P(-0.1f, -0.16f), P(0.1f, -0.16f)}, Dark, 0.06f);
		Line({P(-0.5f, -0.18f), P(-0.36f, -0.18f)}, Color{1.0f, 1.0f, 1.0f, 0.5f}, 0.04f);
		C.StrokeArc(Cx, Cy + 0.08f * S, 0.36f * S, 0.25f * Pi, 0.75f * Pi, Hex(0x713f12), 0.09f * S, true);
		break;
	case AvatarIcon::Flame:
	{
		// Teardrops with the tip licking to one side.
		auto Drop = [&](float Wd, float Ht, float Dy, const Color& Col) {
			std::vector<Vec2> Pts;
			for (int I = 0; I < 28; ++I)
			{
				const float T = static_cast<float>(I) / 28.0f * 2.0f * Pi;
				const float Y = -std::cos(T);
				const float Lean = Y < 0.0f ? Y * Y * 0.22f : 0.0f;
				Pts.push_back(P(Wd * std::sin(T) * std::sin(T * 0.5f) + Lean, Dy + Y * Ht));
			}
			C.FillPolygon(Pts, Col);
		};
		Drop(0.74f, 0.82f, 0.02f, Hex(0xff6a1a));
		Drop(0.42f, 0.46f, 0.36f, Hex(0xffd166));
		break;
	}
	case AvatarIcon::Bolt:
		Poly({P(0.22f, -0.9f), P(-0.46f, 0.1f), P(-0.04f, 0.1f), P(-0.24f, 0.9f), P(0.46f, -0.14f), P(0.04f, -0.14f)}, Ink);
		break;
	case AvatarIcon::Chip:
		Disc(0.0f, 0.0f, 0.8f, Ink);
		for (int K = 0; K < 6; ++K)
		{
			const float A0 = static_cast<float>(K) * Pi / 3.0f;
			C.StrokeArc(Cx, Cy, 0.66f * S, A0 - 0.17f, A0 + 0.17f, Deep, 0.24f * S);
		}
		Disc(0.0f, 0.0f, 0.46f, Deep);
		C.StrokeEllipse(Cx, Cy, 0.38f * S, 0.38f * S, Ink, 0.05f * S, 0.0f);
		StarShape(C, Cx, Cy, 0.2f * S, Ink);
		break;
	case AvatarIcon::Cowboy:
		Oval(0.0f, 0.32f, 0.9f, 0.2f, Ink);
		Poly({P(-0.46f, 0.32f), P(-0.4f, -0.36f), P(-0.15f, -0.46f), P(0.0f, -0.3f), P(0.15f, -0.46f), P(0.4f, -0.36f), P(0.46f, 0.32f)}, Ink);
		C.FillRect({Cx - 0.44f * S, Cy + 0.08f * S, 0.88f * S, 0.12f * S}, Deep);
		Oval(0.0f, 0.3f, 0.5f, 0.05f, Color{Deep.R, Deep.G, Deep.B, 0.35f});
		break;
	case AvatarIcon::Moon:
	{
		// A crescent: the outer circle's arc outside a second, offset circle, then that circle's arc back.
		const float D = 0.62f;
		const float Ri = 0.86f;
		const float Xi = (1.0f - Ri * Ri + D * D) / (2.0f * D);
		const float Yi = std::sqrt(std::max(0.0f, 1.0f - Xi * Xi));
		const float Phi = std::atan2(Yi, Xi);
		const float Psi = std::atan2(Yi, Xi - D);
		const float Turn = -0.55f;
		std::vector<Vec2> Pts;
		auto Add = [&](float X, float Y) {
			const float Rx = X * std::cos(Turn) - Y * std::sin(Turn);
			const float Ry = X * std::sin(Turn) + Y * std::cos(Turn);
			Pts.push_back(P(-0.08f + Rx * 0.72f, 0.04f + Ry * 0.72f));
		};
		for (int I = 0; I <= 24; ++I)
		{
			const float T = Phi + (2.0f * Pi - 2.0f * Phi) * static_cast<float>(I) / 24.0f;
			Add(std::cos(T), std::sin(T));
		}
		for (int I = 1; I < 24; ++I) // the arc ends are the outer arc's, already in
		{
			const float T = (2.0f * Pi - Psi) - (2.0f * Pi - 2.0f * Psi) * static_cast<float>(I) / 24.0f;
			Add(D + Ri * std::cos(T), Ri * std::sin(T));
		}
		C.FillPolygon(Pts, Ink);
		StarShape(C, Cx + 0.42f * S, Cy - 0.36f * S, 0.17f * S, Ink);
		StarShape(C, Cx + 0.58f * S, Cy + 0.3f * S, 0.1f * S, Ink);
		break;
	}
	case AvatarIcon::Wolf:
	{
		const Color Fur = Hex(0x9aa5b4);
		const Color Pale = Hex(0xe5e9f0);
		Poly({P(-0.62f, -0.2f), P(-0.52f, -0.92f), P(-0.18f, -0.5f), P(0.18f, -0.5f), P(0.52f, -0.92f), P(0.62f, -0.2f), P(0.52f, 0.18f), P(0.2f, 0.5f), P(0.0f, 0.8f),
				 P(-0.2f, 0.5f), P(-0.52f, 0.18f)},
			Fur);
		Poly({P(-0.46f, -0.32f), P(-0.44f, -0.7f), P(-0.26f, -0.48f)}, Hex(0x4b5563));
		Poly({P(0.46f, -0.32f), P(0.44f, -0.7f), P(0.26f, -0.48f)}, Hex(0x4b5563));
		Poly({P(-0.3f, 0.02f), P(0.0f, -0.12f), P(0.3f, 0.02f), P(0.16f, 0.5f), P(0.0f, 0.78f), P(-0.16f, 0.5f)}, Pale);
		Poly({P(-0.36f, -0.14f), P(-0.12f, -0.1f), P(-0.2f, -0.02f)}, Hex(0xfacc15));
		Poly({P(0.36f, -0.14f), P(0.12f, -0.1f), P(0.2f, -0.02f)}, Hex(0xfacc15));
		Disc(0.0f, 0.66f, 0.1f, Dark);
		break;
	}
	case AvatarIcon::Tiger:
	{
		const Color TigerFur = Hex(0xf59e0b);
		Disc(-0.5f, -0.52f, 0.2f, TigerFur);
		Disc(0.5f, -0.52f, 0.2f, TigerFur);
		Disc(-0.5f, -0.52f, 0.1f, Dark);
		Disc(0.5f, -0.52f, 0.1f, Dark);
		Oval(0.0f, 0.04f, 0.7f, 0.64f, TigerFur);
		Poly({P(-0.12f, -0.6f), P(0.12f, -0.6f), P(0.0f, -0.3f)}, Dark);
		Poly({P(-0.32f, -0.52f), P(-0.18f, -0.56f), P(-0.24f, -0.34f)}, Dark);
		Poly({P(0.32f, -0.52f), P(0.18f, -0.56f), P(0.24f, -0.34f)}, Dark);
		for (int K = -1; K <= 1; K += 2)
		{
			const float F = static_cast<float>(K);
			Poly({P(0.7f * F, -0.06f), P(0.7f * F, 0.06f), P(0.42f * F, 0.02f)}, Dark);
			Poly({P(0.66f * F, 0.2f), P(0.62f * F, 0.32f), P(0.4f * F, 0.22f)}, Dark);
		}
		Disc(-0.13f, 0.32f, 0.17f, White);
		Disc(0.13f, 0.32f, 0.17f, White);
		Oval(-0.25f, -0.08f, 0.07f, 0.09f, Dark);
		Oval(0.25f, -0.08f, 0.07f, 0.09f, Dark);
		Poly({P(-0.1f, 0.16f), P(0.1f, 0.16f), P(0.0f, 0.27f)}, Hex(0xf472b6));
		break;
	}
	case AvatarIcon::AceCard:
	{
		const Rect Card{Cx - 0.5f * S, Cy - 0.7f * S, 1.0f * S, 1.4f * S};
		C.FillRoundRect({Card.X + 0.05f * S, Card.Y + 0.07f * S, Card.W, Card.H}, 0.12f * S, Color{0.0f, 0.0f, 0.0f, 0.3f});
		C.FillRoundRect(Card, 0.12f * S, White);
		C.Text("A", Card.X + 0.12f * S, Card.Y + 0.38f * S, Ts(0.36f * S, 900, Dark));
		DrawSuit(C, 3, Cx - 0.3f * S, Cy - 0.18f * S, 0.62f * S, Dark);
		break;
	}
	case AvatarIcon::Coffee:
		for (int K = 0; K < 2; ++K)
		{
			const float X = -0.24f + 0.26f * static_cast<float>(K);
			Line({P(X, -0.32f), P(X + 0.08f, -0.5f), P(X - 0.02f, -0.68f), P(X + 0.06f, -0.86f)}, Color{Ink.R, Ink.G, Ink.B, 0.7f}, 0.06f);
		}
		C.StrokeArc(Cx + 0.34f * S, Cy + 0.18f * S, 0.2f * S, -Pi * 0.5f, Pi * 0.5f, Ink, 0.12f * S);
		C.FillRoundRect({Cx - 0.56f * S, Cy - 0.18f * S, 0.9f * S, 0.82f * S}, 0.16f * S, Ink);
		Oval(-0.11f, -0.14f, 0.38f, 0.07f, Hex(0x5b3416));
		break;
	case AvatarIcon::Headphones:
		C.StrokeArc(Cx, Cy + 0.08f * S, 0.64f * S, Pi, 2.0f * Pi, Ink, 0.15f * S, true);
		C.FillRoundRect({Cx - 0.8f * S, Cy - 0.02f * S, 0.3f * S, 0.6f * S}, 0.12f * S, Ink);
		C.FillRoundRect({Cx + 0.5f * S, Cy - 0.02f * S, 0.3f * S, 0.6f * S}, 0.12f * S, Ink);
		C.FillRoundRect({Cx - 0.7f * S, Cy + 0.08f * S, 0.1f * S, 0.4f * S}, 0.04f * S, Hex(0xf472b6));
		C.FillRoundRect({Cx + 0.6f * S, Cy + 0.08f * S, 0.1f * S, 0.4f * S}, 0.04f * S, Hex(0xf472b6));
		break;
	case AvatarIcon::Pizza:
		Poly({P(-0.64f, -0.5f), P(0.64f, -0.5f), P(0.0f, 0.8f)}, Hex(0xfbbf24));
		Line({P(-0.66f, -0.54f), P(0.0f, -0.62f), P(0.66f, -0.54f)}, Hex(0xb45309), 0.2f);
		Disc(-0.2f, -0.22f, 0.12f, Hex(0xdc2626));
		Disc(0.22f, -0.18f, 0.12f, Hex(0xdc2626));
		Disc(0.0f, 0.2f, 0.11f, Hex(0xdc2626));
		Disc(0.08f, -0.36f, 0.04f, Hex(0x15803d));
		break;
	case AvatarIcon::EightBall:
		Disc(0.0f, 0.0f, 0.8f, Paint::Radial(P(-0.3f, -0.35f), 0.05f * S, P(0.0f, 0.0f), 0.8f * S, Hex(0x4b5563), 0.6f, Hex(0x111318), Hex(0x05060a)));
		Disc(-0.08f, -0.1f, 0.36f, White);
		C.Text("8", Cx - 0.08f * S, Cy - 0.08f * S, Ts(0.5f * S, 900, Dark, Align::Center, Baseline::Middle));
		break;
	case AvatarIcon::Ghost:
	default:
	{
		const Color Body = Rgba(255, 255, 255, 0.94f);
		Disc(0.0f, -0.16f, 0.56f, Body);
		C.FillRect({Cx - 0.56f * S, Cy - 0.16f * S, 1.12f * S, 0.56f * S}, Body);
		for (int I = 0; I < 3; ++I)
		{
			const float X0 = -0.56f + static_cast<float>(I) * 0.3733f;
			Poly({P(X0, 0.38f), P(X0 + 0.3733f, 0.38f), P(X0 + 0.1867f, 0.66f)}, Body);
		}
		Oval(-0.2f, -0.2f, 0.1f, 0.15f, Hex(0x2a0d14));
		Oval(0.2f, -0.2f, 0.1f, 0.15f, Hex(0x2a0d14));
		break;
	}
	}
}
// ---- champions' frames

const Color GoldHi = Hex(0xfff4c6);
const Color GoldMid = Hex(0xf2c14e);
const Color GoldLo = Hex(0x7a4e0e);

Color Shade(const Color& A, const Color& B, float T)
{
	return {A.R + (B.R - A.R) * T, A.G + (B.G - A.G) * T, A.B + (B.B - A.B) * T, A.A + (B.A - A.A) * T};
}

/** A cut stone seen from the side (table, crown, pavilion), K across, its girdle at (X, Y). */
void CutStone(Canvas& C, float X, float Y, float K, const Color& Col)
{
	const Color Hi = Shade(Col, Hex(0xffffff), 0.55f);
	const Color Lo = Shade(Col, Hex(0x000000), 0.35f);
	auto Q = [&](float U, float V) { return Vec2{X + U * K, Y + V * K}; };
	C.FillPolygon({Q(-0.5f, 0.0f), Q(-0.24f, -0.3f), Q(-0.11f, 0.0f)}, Shade(Col, Hex(0xffffff), 0.3f));
	C.FillPolygon({Q(-0.24f, -0.3f), Q(0.24f, -0.3f), Q(0.11f, 0.0f), Q(-0.11f, 0.0f)}, Hi);
	C.FillPolygon({Q(0.24f, -0.3f), Q(0.5f, 0.0f), Q(0.11f, 0.0f)}, Col);
	C.FillPolygon({Q(-0.5f, 0.0f), Q(-0.11f, 0.0f), Q(0.0f, 0.55f)}, Col);
	C.FillPolygon({Q(-0.11f, 0.0f), Q(0.11f, 0.0f), Q(0.0f, 0.55f)}, Shade(Col, Hex(0xffffff), 0.15f));
	C.FillPolygon({Q(0.11f, 0.0f), Q(0.5f, 0.0f), Q(0.0f, 0.55f)}, Lo);
	if (K >= 8.0f)
	{
		C.StrokePolyline({Q(-0.5f, 0.0f), Q(-0.24f, -0.3f), Q(0.24f, -0.3f), Q(0.5f, 0.0f), Q(0.0f, 0.55f)}, true, Rgba(255, 255, 255, 0.5f), std::max(0.6f, K * 0.03f), true);
	}
	StarShape(C, X - 0.16f * K, Y - 0.2f * K, std::max(1.2f, 0.13f * K), Rgba(255, 255, 255, 0.95f));
}

/** A ring winner's stone, set on top of the frame: prongs, a basket, the stone. */
void SetStone(Canvas& C, float Cx, float Top, float K, const Color& Stone)
{
	C.FillPolygon({{Cx - K * 0.2f, Top + K * 0.32f}, {Cx + K * 0.2f, Top + K * 0.32f}, {Cx + K * 0.34f, Top + K * 0.02f}, {Cx - K * 0.34f, Top + K * 0.02f}},
		Paint::Linear({Cx - K * 0.34f, Top}, {Cx + K * 0.34f, Top}, GoldHi, GoldLo));
	CutStone(C, Cx, Top - K * 0.02f, K, Stone);
	C.FillCircle(Cx - K * 0.44f, Top - K * 0.04f, std::max(0.8f, K * 0.05f), GoldHi);
	C.FillCircle(Cx + K * 0.44f, Top - K * 0.04f, std::max(0.8f, K * 0.05f), GoldHi);
}

/** How many they've won, in a small medal at the lower right. */
void CountMedal(Canvas& C, float Cx, float Cy, float R, int Count)
{
	if (Count < 2 || R < 14.0f)
	{
		return;
	}
	const float Mx = Cx + R * 0.76f;
	const float My = Cy + R * 0.7f;
	const float Mr = std::max(7.0f, R * 0.3f);
	C.FillCircle(Mx, My + Mr * 0.08f, Mr * 1.08f, Rgba(0, 0, 0, 0.45f));
	C.FillCircle(Mx, My, Mr, Paint::Linear({Mx - Mr, My - Mr}, {Mx + Mr, My + Mr}, GoldHi, GoldLo));
	C.FillCircle(Mx, My, Mr * 0.8f, Paint::Linear({Mx, My - Mr}, {Mx, My + Mr}, Hex(0x2a1b04), Hex(0x120b02)));
	C.Text(std::to_string(Count), Mx, My + Mr * 0.04f, Ts(Mr * (Count >= 10 ? 0.9f : 1.1f), 900, GoldHi, Align::Center, Baseline::Middle));
}

/** A bracelet winner: polished gold links all the way round, a plaque in their bracelet's enamel at the bottom. */
void BraceletFrame(Canvas& C, float Cx, float Cy, float R, const AvatarSpec& A)
{
	const float T = std::max(3.0f, R * 0.2f);
	const float R0 = R - T * 0.2f;
	const float R1 = R + T * 0.8f;
	C.StrokeEllipse(Cx, Cy, (R0 + R1) * 0.5f, (R0 + R1) * 0.5f, Hex(0x2e1c03), R1 - R0 + 1.5f);
	const int N = std::max(12, std::min(30, static_cast<int>(R * 0.85f)));
	const float Gap = std::min(0.05f, 0.6f / R);
	for (int K = 0; K < N; ++K)
	{
		const float A0 = 2.0f * Pi * static_cast<float>(K) / static_cast<float>(N) + Gap;
		const float A1 = 2.0f * Pi * static_cast<float>(K + 1) / static_cast<float>(N) - Gap;
		const float Am = (A0 + A1) * 0.5f;
		// The light comes from the upper left.
		const float Lit = 0.5f + 0.5f * std::cos(Am + 2.3f);
		const Color Outer = K % 2 == 0 ? Shade(GoldMid, GoldHi, Lit) : Shade(GoldLo, GoldMid, Lit);
		const Color Inner = K % 2 == 0 ? Shade(GoldLo, GoldMid, Lit * 0.8f) : Shade(Hex(0x4a2e06), GoldLo, Lit);
		const float Cs0 = std::cos(A0);
		const float Sn0 = std::sin(A0);
		const float Cs1 = std::cos(A1);
		const float Sn1 = std::sin(A1);
		C.FillPolygon({{Cx + R0 * Cs0, Cy + R0 * Sn0}, {Cx + R1 * Cs0, Cy + R1 * Sn0}, {Cx + R1 * Cs1, Cy + R1 * Sn1}, {Cx + R0 * Cs1, Cy + R0 * Sn1}},
			Paint::Linear({Cx + R1 * std::cos(Am), Cy + R1 * std::sin(Am)}, {Cx + R0 * std::cos(Am), Cy + R0 * std::sin(Am)}, Outer, Inner));
	}
	C.StrokeArc(Cx, Cy, R1 - T * 0.22f, Pi * 1.02f, Pi * 1.48f, Rgba(255, 255, 255, 0.55f), std::max(1.0f, T * 0.14f), true);
	// The plaque.
	const float Pw = std::max(9.0f, R * 0.8f);
	const float Ph = std::max(6.0f, R * 0.44f);
	const Rect Pl{Cx - Pw * 0.5f, Cy + R1 - Ph * 0.62f, Pw, Ph};
	C.FillRoundRect({Pl.X - 1.0f, Pl.Y + 1.5f, Pl.W + 2.0f, Pl.H}, Ph * 0.3f, Rgba(0, 0, 0, 0.45f));
	C.FillRoundRect(Pl, Ph * 0.28f, Paint::Linear({Pl.X, Pl.Y}, {Pl.X + Pl.W, Pl.Y + Pl.H}, GoldHi, GoldLo));
	const float In = std::max(1.2f, Ph * 0.16f);
	const Rect En{Pl.X + In, Pl.Y + In, Pl.W - 2.0f * In, Pl.H - 2.0f * In};
	const Color Plate = Hex(A.Plate);
	C.FillRoundRect(En, std::max(1.0f, Ph * 0.18f), Paint::Linear({0.0f, En.Y}, {0.0f, En.Y + En.H}, Shade(Plate, Hex(0xffffff), 0.12f), Shade(Plate, Hex(0x000000), 0.55f)));
	if (Ph >= 8.0f)
	{
		StarShape(C, Cx, En.Y + En.H * 0.52f, En.H * 0.42f, Hex(0xf4f7ff));
	}
	else
	{
		C.FillCircle(Cx, En.Y + En.H * 0.5f, std::max(0.8f, En.H * 0.3f), Hex(0xf4f7ff));
	}
	if (A.Rings > 0)
	{
		SetStone(C, Cx, Cy - R1 - std::max(2.0f, R * 0.08f), std::max(7.0f, R * 0.46f), Hex(A.Stone));
	}
	CountMedal(C, Cx, Cy, R, A.Bracelets);
}

/** A ring winner: the picture set in a polished band, their ring's stone on top. */
void GemFrame(Canvas& C, float Cx, float Cy, float R, const AvatarSpec& A)
{
	const float T = std::max(2.5f, R * 0.17f);
	const float Rm = R + T * 0.3f;
	C.StrokeEllipse(Cx, Cy, Rm, Rm, Hex(0x2e1c03), T + 1.5f);
	C.StrokeEllipse(Cx, Cy, Rm, Rm, GoldLo, T);
	C.StrokeEllipse(Cx, Cy, Rm, Rm, GoldMid, T * 0.62f);
	C.StrokeArc(Cx, Cy, Rm + T * 0.12f, Pi * 0.98f, Pi * 1.55f, GoldHi, std::max(1.0f, T * 0.28f), true);
	C.StrokeArc(Cx, Cy, Rm + T * 0.12f, Pi * 0.1f, Pi * 0.38f, Rgba(255, 244, 198, 0.6f), std::max(0.8f, T * 0.2f), true);
	const float K = std::max(7.0f, R * 0.5f);
	if (R >= 16.0f)
	{
		// A halo of small diamonds round the stone.
		for (int I = 0; I < 9; ++I)
		{
			const float Ang = Pi + Pi * (static_cast<float>(I) + 0.5f) / 9.0f;
			C.FillCircle(Cx + std::cos(Ang) * K * 0.62f, Cy - Rm - K * 0.12f + std::sin(Ang) * K * 0.2f, std::max(0.9f, K * 0.06f), Hex(0xeaf7ff));
		}
	}
	SetStone(C, Cx, Cy - Rm - T * 0.2f, K, Hex(A.Stone));
	CountMedal(C, Cx, Cy, R, A.Rings);
}
} // namespace avatars_detail

using namespace avatars_detail;

const char* AvatarIconName(AvatarIcon I)
{
	static const char* const Names[] = {"Initials", "Shark", "Fish", "Whale", "Owl", "Cat", "Fox", "Bear", "Panda", "Skull", "Crown", "Gem", "Spade", "Heart", "Club", "Diamond", "Dice",
		"Clover", "Rocket", "Robot", "Alien", "Shades", "Flame", "Bolt", "Chip", "Cowboy", "Moon", "Ace", "Coffee", "Headphones", "Pizza", "Eight ball", "Wolf", "Tiger", "Ghost"};
	const int K = static_cast<int>(I);
	return K >= 0 && K < static_cast<int>(AvatarIcon::Count) ? Names[K] : "";
}

AvatarSpec AvatarFor(const std::string& Name)
{
	AvatarSpec A;
	const NameWords N = ReadName(Name);
	A.Initials = InitialsOf(N);
	if (Name == RivalName)
	{
		A.Icon = AvatarIcon::Ghost;
		A.Bg = 0x5b1022;
		A.Bg2 = 0x1a0509;
		A.Frame = AvatarFrame::Neon;
		A.Rim = 0xb36bff;
		return A;
	}
	const uint32_t H = Fnv1a(N.Flat);
	const Swatch& Sw = Swatches[(H >> 7) % 18u];
	A.Bg = Sw.Bg;
	A.Bg2 = Sw.Bg2;
	A.Ink = Sw.Ink;
	// A name that says what it is gets that picture, in any of the network's languages.
	using I = AvatarIcon;
	if (Says(N, {"shark", "tiburon", "tubarao", "akula", "rekin", "requin", "squalo", "hajen", "kaashaai", "kartenhai", "shayu", "zralok"})) A.Icon = I::Shark;
	else if (Says(N, {"whale"})) A.Icon = I::Whale;
	else if (Says(N, {"fish", "pescado", "peixe"})) A.Icon = I::Fish;
	else if (Says(N, {"nachtfalke", "insomniac", "nightcrawler"}, {"owl"})) A.Icon = I::Owl;
	else if (Says(N, {"neko", "kitty"}, {"cat"})) A.Icon = I::Cat;
	else if (Says(N, {"renard", "kettu", "kitsune"}, {"fox"})) A.Icon = I::Fox;
	else if (Says(N, {"panda", "xiongmao"})) A.Icon = I::Panda;
	else if (Says(N, {"medved", "karhu", "isbjorn", "medvidek"}, {"bear", "bjorn"})) A.Icon = I::Bear;
	else if (Says(N, {"vargen", "lupo"}, {"wolf", "volk", "vovk", "wilk", "vlk", "susi", "ulv"})) A.Icon = I::Wolf;
	else if (Says(N, {"tiger", "laohu", "horangi", "jaguar"}, {"tigr", "sher"})) A.Icon = I::Tiger;
	else if (Says(N, {"queen", "royal", "koenig", "kongen", "baron", "sultan", "countess", "shogun", "hetman", "kaiser"}, {"king", "rei", "duke"})) A.Icon = I::Crown;
	else if (Says(N, {"jewel"}, {"gem"})) A.Icon = I::Gem;
	else if (Says(N, {"rocket"})) A.Icon = I::Rocket;
	else if (Says(N, {"coffee", "cuppa", "energy", "fika"}, {"chai"})) A.Icon = I::Coffee;
	else if (Says(N, {"fire", "flame", "heater", "picante", "masala", "flambeur", "vesuvio"}, {"hot"})) A.Icon = I::Flame;
	else if (Says(N, {"luck", "fortune", "fortuna", "sorte", "suerte", "udacha", "glueck", "lycka", "stesti", "szczescie", "clover", "shamrock", "lachance"}))
		A.Icon = H % 2u ? I::Clover : I::Dice;
	else if (Says(N, {"solver", "machine", "icm", "stats", "hud", "plusev", "gto", "robot"}, {"bot"})) A.Icon = I::Robot;
	else if (Says(N, {"moon", "lunar", "norrsken", "nordlys", "revontuli", "sleep", "midnight", "siesta"})) A.Icon = I::Moon;
	else if (Says(N, {"cowboy", "rodeo", "gaucho", "charro"}, {"tex"})) A.Icon = I::Cowboy;
	else if (Says(N, {"skull", "calavera", "dead", "banshee"}, {"rip"})) A.Icon = I::Skull;
	else if (Says(N, {"pizza", "taco", "ramen", "sushi", "gnocchi", "tapas", "pierogi", "poutine", "asado", "kimchi", "bibimbap", "strudel", "stroopwafel", "croissant",
				 "frites", "pastel", "bacalhau", "borshch", "knedlik", "salsa", "timbit", "praline", "gaufre"}))
		A.Icon = I::Pizza;
	else if (Says(N, {"ttv", "stream", "music", "tango", "fado"})) A.Icon = I::Headphones;
	else if (Says(N, {"bolt", "electric", "turbo", "hyper", "snap", "bystro", "hammer"})) A.Icon = I::Bolt;
	else if (Says(N, {"ficha", "jeton", "stack"}, {"chip"})) A.Icon = I::Chip;
	else if (Says(N, {"smooth", "chill", "pokerface", "gezellig", "hygge"}, {"cool"})) A.Icon = I::Shades;
	else if (Says(N, {"alien", "cosmic", "kaiju"})) A.Icon = I::Alien;
	else if (Says(N, {"asso"}, {"ace"})) A.Icon = I::AceCard;
	else if (Says(N, {"spade"})) A.Icon = I::Spade;
	else if (Says(N, {"heart"})) A.Icon = I::Heart;
	else if (Says(N, {"club"})) A.Icon = I::Club;
	else if (Says(N, {"diamond"})) A.Icon = I::Diamond;
	else if (Says(N, {"88", "eight"}, {"pool"})) A.Icon = I::EightBall;
	else
	{
		// Everyone else: plain initials now and then, otherwise one of the set.
		const uint32_t Pick = (H >> 13) % 100u;
		A.Icon = Pick < 20 ? I::Initials : static_cast<I>(1u + (H >> 3) % (static_cast<uint32_t>(I::Ghost) - 1u));
	}
	const uint32_t Frame = (H >> 21) % 100u;
	A.Frame = Frame < 12 ? AvatarFrame::Chip : Frame < 22 ? AvatarFrame::Ring : AvatarFrame::None;
	A.Rim = Frame < 22 ? Sw.Bg : 0xffffff;
	return A;
}

namespace
{
void DrawAvatarNow(Canvas& C, float Cx, float Cy, float R, const AvatarSpec& A);

uint64_t AvatarKey(const AvatarSpec& A, float R)
{
	uint64_t H = 0xcbf29ce484222325ull;
	const auto Mix = [&](const void* Data, size_t Size) {
		const unsigned char* B = static_cast<const unsigned char*>(Data);
		for (size_t K = 0; K < Size; ++K)
		{
			H = (H ^ B[K]) * 0x100000001b3ull;
		}
	};
	const uint32_t Ints[11] = {static_cast<uint32_t>(A.Icon), A.Bg, A.Bg2, A.Ink, static_cast<uint32_t>(A.Frame), A.Rim, static_cast<uint32_t>(A.Bracelets), static_cast<uint32_t>(A.Rings), A.Stone, A.Plate,
		A.Halo ? 1u : 0u};
	Mix("avatar1", 7);
	Mix(Ints, sizeof(Ints));
	Mix(&R, sizeof(R));
	Mix(A.Initials.data(), A.Initials.size());
	return H;
}
} // namespace

// Avatars don't move: each is drawn once and copied in after (Canvas::Cached).
void DrawAvatar(Canvas& C, float Cx, float Cy, float R, const AvatarSpec& A)
{
	C.Cached(AvatarKey(A, R), Cx, Cy, [&](Canvas& At) { DrawAvatarNow(At, 0.0f, 0.0f, R, A); });
}

namespace
{
void DrawAvatarNow(Canvas& C, float Cx, float Cy, float R, const AvatarSpec& A)
{
	const Color Bg = Hex(A.Bg);
	const Color Bg2 = Hex(A.Bg2);
	const Color Rim = Hex(A.Rim);
	if (A.Frame == AvatarFrame::Neon || A.Halo)
	{
		C.FillCircle(Cx, Cy, R * 1.45f, Paint::Radial({Cx, Cy}, R * 0.9f, {Cx, Cy}, R * 1.45f, Color{Rim.R, Rim.G, Rim.B, 0.45f}, -1.0f, Color(), Color{Rim.R, Rim.G, Rim.B, 0.0f}));
	}
	C.FillCircle(Cx, Cy, R, Paint::Linear({Cx - R * 0.6f, Cy - R}, {Cx + R * 0.6f, Cy + R}, Bg, Bg2));
	// A soft highlight across the top, like a glossy sticker.
	C.FillEllipse(Cx, Cy - R * 0.45f, R * 0.7f, R * 0.38f, Paint::Linear({0.0f, Cy - R * 0.85f}, {0.0f, Cy - R * 0.05f}, Rgba(255, 255, 255, 0.16f), Rgba(255, 255, 255, 0.0f)));
	DrawIcon(C, A, Cx, Cy, R * 0.6f, R);
	switch (A.Frame)
	{
	case AvatarFrame::Ring:
		C.StrokeEllipse(Cx, Cy, R + R * 0.06f, R + R * 0.06f, Rim, std::max(1.5f, R * 0.12f));
		break;
	case AvatarFrame::Chip:
		C.StrokeEllipse(Cx, Cy, R + R * 0.07f, R + R * 0.07f, Bg2, std::max(2.0f, R * 0.16f));
		for (int K = 0; K < 8; ++K)
		{
			const float A0 = static_cast<float>(K) * Pi / 4.0f;
			C.StrokeArc(Cx, Cy, R + R * 0.07f, A0 - 0.16f, A0 + 0.16f, Hex(0xffffff), std::max(2.0f, R * 0.16f));
		}
		break;
	case AvatarFrame::Gold:
	{
		C.StrokeEllipse(Cx, Cy, R + R * 0.08f, R + R * 0.08f, Hex(0xf2c14e), std::max(2.0f, R * 0.16f));
		C.StrokeEllipse(Cx, Cy, R + R * 0.17f, R + R * 0.17f, Rgba(255, 241, 168, 0.55f), std::max(1.0f, R * 0.04f));
		StarShape(C, Cx, Cy + R * 1.02f, std::max(4.0f, R * 0.26f), Hex(0xfff1a8));
		break;
	}
	case AvatarFrame::Neon:
		C.StrokeEllipse(Cx, Cy, R + R * 0.06f, R + R * 0.06f, Rim, std::max(1.5f, R * 0.12f));
		break;
	case AvatarFrame::Bracelet:
		BraceletFrame(C, Cx, Cy, R, A);
		break;
	case AvatarFrame::Gem:
		GemFrame(C, Cx, Cy, R, A);
		break;
	default:
		C.StrokeEllipse(Cx, Cy, R, R, Rgba(0, 0, 0, 0.35f), 1.0f);
		break;
	}
}
} // namespace
} // namespace ui
} // namespace ss
