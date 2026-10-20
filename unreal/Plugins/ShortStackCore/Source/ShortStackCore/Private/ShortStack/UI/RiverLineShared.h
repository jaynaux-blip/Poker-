// Drawing helpers shared by the RiverLine lobby pages and the laptop apps (private to the UI sources).
#pragma once

#include "ShortStack/Game/Badges.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/UI/Avatars.h"
#include "ShortStack/UI/EventArt.h"
#include "ShortStack/UI/RiverLine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <string>
#include <vector>

namespace ss
{
namespace ui
{
// Update 2 (G3): the skin-aware shades the helpers below use (RiverLineSites.cpp; declared again with the rest further down).
namespace rlsites_detail
{
bool SitesHome();
Color SitesTone(uint32_t Rgb);
Color SitesOnAccent();
Color SitesAccentDeep();
Color SitesOnGold();
Color SitesGoldDeep();
} // namespace rlsites_detail

namespace rlnet_detail
{
const float NetTop = 64.0f;
const float NetW = RiverLine::Width;
const double SlideSeconds = 7.5;

template <typename T>
float Nf(T V)
{
	return static_cast<float>(V);
}

inline float NetEase(double T)
{
	return static_cast<float>(EaseOutCubic(Clamp01(T)));
}

inline Color NetA(const Color& Cl, float A)
{
	return {Cl.R, Cl.G, Cl.B, Cl.A * A};
}

/** Frame-rate independent approach: the share of the remaining distance to cover this frame. */
inline float NetFollow(double Dt, double Rate)
{
	return static_cast<float>(1.0 - std::exp(-Dt * Rate));
}

inline std::string NetPad2(long long V)
{
	return (V < 10 ? "0" : "") + std::to_string(V);
}

/** Countdown with seconds: "1:03:12", "03:12"; days beyond a day ("5d 14h"). */
inline std::string NetHms(double Minutes)
{
	if (Minutes >= 24.0 * 60.0)
	{
		return net::Countdown(Minutes);
	}
	const long long Secs = std::max(0LL, static_cast<long long>(std::floor(Minutes * 60.0)));
	const long long H = Secs / 3600;
	return (H > 0 ? std::to_string(H) + ":" : std::string()) + NetPad2((Secs / 60) % 60) + ":" + NetPad2(Secs % 60);
}

/** "$2,000" (whole dollars from $1,000 up, or when there are no cents), "$41.40" otherwise. */
inline std::string NetMoney(Chips Cents)
{
	return Cents >= 100000 || Cents % 100 == 0 ? "$" + Grouped((Cents + 50) / 100) : Money(Cents);
}

inline Color NetTier(net::Tier T)
{
	switch (T)
	{
	case net::Tier::Freeroll: return pal::Green;
	case net::Tier::Micro: return pal::Accent;
	case net::Tier::Low: return pal::Accent2;
	case net::Tier::Mid: return Hex(0x9b6bff);
	case net::Tier::High: return pal::Gold;
	}
	return pal::Accent;
}

inline Color NetStatus(net::Status St)
{
	switch (St)
	{
	case net::Status::Announced: return rlsites_detail::SitesTone(0x6b7a93);
	case net::Status::Registering: return Hex(0x4f9bff);
	case net::Status::LateReg: return pal::Green;
	case net::Status::Running: return rlsites_detail::SitesTone(0x9aa7bd);
	case net::Status::FinalTable: return pal::Gold;
	case net::Status::Finished: return pal::Dim;
	}
	return pal::Muted;
}

inline Color NetKind(net::NewsKind K)
{
	switch (K)
	{
	case net::NewsKind::BigWin: return pal::Gold;
	case net::NewsKind::Series: return Hex(0xb8ff2e);
	case net::NewsKind::Schedule: return Hex(0x4f9bff);
	case net::NewsKind::Record: return pal::Orange;
	case net::NewsKind::Hero: return pal::Accent;
	case net::NewsKind::People: return Hex(0xc084fc);
	}
	return pal::Accent;
}

/** Text with letter spacing (labels in capitals). Returns the width. */
inline float NetSpaced(Canvas& Cv, const std::string& Str, float X, float Y, float Size, int Weight, const Color& Col, float Track, Align A = Align::Left)
{
	std::vector<std::string> Glyphs;
	for (size_t I = 0; I < Str.size();)
	{
		const unsigned char B = static_cast<unsigned char>(Str[I]);
		const size_t Len = B >= 0xF0 ? 4 : B >= 0xE0 ? 3 : B >= 0xC0 ? 2 : 1;
		Glyphs.push_back(Str.substr(I, Len));
		I += Len;
	}
	float Total = 0.0f;
	for (const std::string& G : Glyphs)
	{
		Total += Cv.Measure(G, Size, Weight) + Track;
	}
	Total -= Glyphs.empty() ? 0.0f : Track;
	float Xx = X - (A == Align::Center ? Total / 2.0f : A == Align::Right ? Total : 0.0f);
	for (const std::string& G : Glyphs)
	{
		Cv.Text(G, Xx, Y, Ts(Size, Weight, Col));
		Xx += Cv.Measure(G, Size, Weight) + Track;
	}
	return Total;
}

/** A small rounded badge with its top-left at (X, Y); returns its width. */
inline float NetPill(Canvas& Cv, const std::string& Label, float X, float Y, const Color& Col, bool Solid = false, float Size = 10.5f)
{
	const float W = Cv.Measure(Label, Size, 700) + Size * 1.3f;
	const float H = Size + 8.0f;
	Cv.FillRoundRect({X, Y, W, H}, H / 2.0f, Solid ? Col : NetA(Col, 0.16f));
	if (!Solid)
	{
		Cv.StrokeRoundRect({X, Y, W, H}, H / 2.0f, NetA(Col, 0.45f), 1.0f);
	}
	Cv.Text(Label, X + W / 2.0f, Y + H / 2.0f + 0.5f, Ts(Size, 700, Solid ? rlsites_detail::SitesTone(0x07121c) : Col, Align::Center, Baseline::Middle));
	return W;
}

inline void NetStar(Canvas& Cv, float Cx, float Cy, float R, const Color& Col)
{
	std::vector<Vec2> P;
	for (int I = 0; I < 10; ++I)
	{
		const float A = -Pi / 2.0f + Nf(I) * Pi / 5.0f;
		const float Rr = I % 2 == 0 ? R : R * 0.42f;
		P.push_back({Cx + Rr * std::cos(A), Cy + Rr * std::sin(A)});
	}
	Cv.FillPolygon(P, Col);
}

inline void NetLockIcon(Canvas& Cv, float X, float Y, float Sz, const Color& Col)
{
	Cv.StrokeArc(X + Sz * 0.5f, Y + Sz * 0.42f, Sz * 0.26f, Pi, 2.0f * Pi, Col, Sz * 0.12f);
	Cv.StrokePolyline({{X + Sz * 0.24f, Y + Sz * 0.42f}, {X + Sz * 0.24f, Y + Sz * 0.5f}}, false, Col, Sz * 0.12f);
	Cv.StrokePolyline({{X + Sz * 0.76f, Y + Sz * 0.42f}, {X + Sz * 0.76f, Y + Sz * 0.5f}}, false, Col, Sz * 0.12f);
	Cv.FillRoundRect({X + Sz * 0.12f, Y + Sz * 0.48f, Sz * 0.76f, Sz * 0.5f}, Sz * 0.1f, Col);
}

inline void NetCheck(Canvas& Cv, float Cx, float Cy, float Sz, const Color& Col)
{
	Cv.StrokePolyline({{Cx - Sz * 0.42f, Cy}, {Cx - Sz * 0.12f, Cy + Sz * 0.3f}, {Cx + Sz * 0.45f, Cy - Sz * 0.32f}}, false, Col, Sz * 0.2f, true);
}

inline void NetChevron(Canvas& Cv, float Cx, float Cy, float Sz, int Dir, const Color& Col)
{
	// Dir: 0 right, 1 left, 2 up, 3 down.
	const float D = Sz * 0.5f;
	std::vector<Vec2> P;
	switch (Dir)
	{
	case 1: P = {{Cx + D * 0.5f, Cy - D}, {Cx - D * 0.5f, Cy}, {Cx + D * 0.5f, Cy + D}}; break;
	case 2: P = {{Cx - D, Cy + D * 0.5f}, {Cx, Cy - D * 0.5f}, {Cx + D, Cy + D * 0.5f}}; break;
	case 3: P = {{Cx - D, Cy - D * 0.5f}, {Cx, Cy + D * 0.5f}, {Cx + D, Cy - D * 0.5f}}; break;
	default: P = {{Cx - D * 0.5f, Cy - D}, {Cx + D * 0.5f, Cy}, {Cx - D * 0.5f, Cy + D}}; break;
	}
	Cv.StrokePolyline(P, false, Col, Sz * 0.16f, true);
}

inline void NetTriangle(Canvas& Cv, float Cx, float Cy, float Sz, bool Up, const Color& Col)
{
	const float H = Sz * 0.5f;
	if (Up)
	{
		Cv.FillPolygon({{Cx - H, Cy + H * 0.6f}, {Cx, Cy - H * 0.7f}, {Cx + H, Cy + H * 0.6f}}, Col);
	}
	else
	{
		Cv.FillPolygon({{Cx - H, Cy - H * 0.6f}, {Cx + H, Cy - H * 0.6f}, {Cx, Cy + H * 0.7f}}, Col);
	}
}

/** Country flags, simplified to their stripes and crosses. */
inline void NetFlag(Canvas& Cv, const std::string& Code, float X, float Y, float W, float H)
{
	auto Hz = [&](std::initializer_list<uint32_t> Cols) {
		const float Bh = H / Nf(Cols.size());
		float Yy = Y;
		for (uint32_t Cl : Cols)
		{
			Cv.FillRect({X, Yy, W, Bh + 0.3f}, Hex(Cl));
			Yy += Bh;
		}
	};
	auto Vt = [&](std::initializer_list<uint32_t> Cols) {
		const float Bw = W / Nf(Cols.size());
		float Xx = X;
		for (uint32_t Cl : Cols)
		{
			Cv.FillRect({Xx, Y, Bw + 0.3f, H}, Hex(Cl));
			Xx += Bw;
		}
	};
	auto Nordic = [&](uint32_t Field, uint32_t Cross, uint32_t Inner) {
		Cv.FillRect({X, Y, W, H}, Hex(Field));
		Cv.FillRect({X + W * 0.28f, Y, W * 0.2f, H}, Hex(Cross));
		Cv.FillRect({X, Y + H * 0.38f, W, H * 0.24f}, Hex(Cross));
		if (Inner != Cross)
		{
			Cv.FillRect({X + W * 0.33f, Y, W * 0.1f, H}, Hex(Inner));
			Cv.FillRect({X, Y + H * 0.44f, W, H * 0.12f}, Hex(Inner));
		}
	};
	if (Code == "DE") Hz({0x111111, 0xdd0000, 0xffce00});
	else if (Code == "NL") Hz({0xae1c28, 0xffffff, 0x21468b});
	else if (Code == "RU") Hz({0xffffff, 0x0039a6, 0xd52b1e});
	else if (Code == "UA") Hz({0x0057b7, 0xffd700});
	else if (Code == "PL") Hz({0xffffff, 0xdc143c});
	else if (Code == "AT") Hz({0xed2939, 0xffffff, 0xed2939});
	else if (Code == "AR") Hz({0x74acdf, 0xffffff, 0x74acdf});
	else if (Code == "IN") Hz({0xff9933, 0xffffff, 0x138808});
	else if (Code == "ES") { Hz({0xaa151b, 0xf1bf00, 0xf1bf00, 0xaa151b}); }
	else if (Code == "FR") Vt({0x0055a4, 0xffffff, 0xef4135});
	else if (Code == "IT") Vt({0x009246, 0xffffff, 0xce2b37});
	else if (Code == "MX") Vt({0x006847, 0xffffff, 0xce1126});
	else if (Code == "IE") Vt({0x169b62, 0xffffff, 0xff883e});
	else if (Code == "BE") Vt({0x111111, 0xfdda24, 0xef3340});
	else if (Code == "CA")
	{
		Vt({0xd52b1e, 0xffffff, 0xffffff, 0xd52b1e});
		NetStar(Cv, X + W * 0.5f, Y + H * 0.5f, H * 0.3f, Hex(0xd52b1e));
	}
	else if (Code == "PT")
	{
		Cv.FillRect({X, Y, W, H}, Hex(0xda291c));
		Cv.FillRect({X, Y, W * 0.4f, H}, Hex(0x046a38));
		Cv.FillCircle(X + W * 0.4f, Y + H * 0.5f, H * 0.2f, Hex(0xffe900));
	}
	else if (Code == "SE") Nordic(0x006aa7, 0xfecc00, 0xfecc00);
	else if (Code == "FI") Nordic(0xffffff, 0x002f6c, 0x002f6c);
	else if (Code == "NO") Nordic(0xba0c2f, 0xffffff, 0x00205b);
	else if (Code == "DK") Nordic(0xc8102e, 0xffffff, 0xffffff);
	else if (Code == "BR")
	{
		Cv.FillRect({X, Y, W, H}, Hex(0x009c3b));
		Cv.FillPolygon({{X + W * 0.5f, Y + H * 0.12f}, {X + W * 0.9f, Y + H * 0.5f}, {X + W * 0.5f, Y + H * 0.88f}, {X + W * 0.1f, Y + H * 0.5f}}, Hex(0xffdf00));
		Cv.FillCircle(X + W * 0.5f, Y + H * 0.5f, H * 0.2f, Hex(0x002776));
	}
	else if (Code == "US")
	{
		Hz({0xb22234, 0xffffff, 0xb22234, 0xffffff, 0xb22234, 0xffffff, 0xb22234});
		Cv.FillRect({X, Y, W * 0.42f, H * 0.54f}, Hex(0x3c3b6e));
	}
	else if (Code == "GB" || Code == "AU")
	{
		Cv.FillRect({X, Y, W, H}, Hex(0x012169));
		const float Lw = H * 0.12f;
		Cv.StrokePolyline({{X, Y}, {X + W, Y + H}}, false, Hex(0xffffff), Lw * 1.6f);
		Cv.StrokePolyline({{X + W, Y}, {X, Y + H}}, false, Hex(0xffffff), Lw * 1.6f);
		Cv.FillRect({X + W * 0.42f, Y, W * 0.16f, H}, Hex(0xffffff));
		Cv.FillRect({X, Y + H * 0.36f, W, H * 0.28f}, Hex(0xffffff));
		Cv.FillRect({X + W * 0.45f, Y, W * 0.1f, H}, Hex(0xc8102e));
		Cv.FillRect({X, Y + H * 0.41f, W, H * 0.18f}, Hex(0xc8102e));
		if (Code == "AU")
		{
			Cv.FillRect({X, Y + H * 0.55f, W, H * 0.45f}, Hex(0x012169));
			Cv.FillRect({X + W * 0.5f, Y, W * 0.5f, H}, Hex(0x012169));
			NetStar(Cv, X + W * 0.25f, Y + H * 0.78f, H * 0.14f, Hex(0xffffff));
			NetStar(Cv, X + W * 0.75f, Y + H * 0.5f, H * 0.12f, Hex(0xffffff));
		}
	}
	else if (Code == "CN")
	{
		Cv.FillRect({X, Y, W, H}, Hex(0xee1c25));
		NetStar(Cv, X + W * 0.2f, Y + H * 0.3f, H * 0.2f, Hex(0xffff00));
	}
	else if (Code == "JP")
	{
		Cv.FillRect({X, Y, W, H}, Hex(0xffffff));
		Cv.FillCircle(X + W * 0.5f, Y + H * 0.5f, H * 0.3f, Hex(0xbc002d));
	}
	else if (Code == "KR")
	{
		Cv.FillRect({X, Y, W, H}, Hex(0xffffff));
		Cv.FillCircle(X + W * 0.5f, Y + H * 0.5f, H * 0.28f, Hex(0x0047a0));
		Cv.FillEllipse(X + W * 0.5f, Y + H * 0.36f, H * 0.28f, H * 0.14f, Hex(0xcd2e3a));
	}
	else if (Code == "CZ")
	{
		Hz({0xffffff, 0xd7141a});
		Cv.FillPolygon({{X, Y}, {X + W * 0.5f, Y + H * 0.5f}, {X, Y + H}}, Hex(0x11457e));
	}
	else
	{
		Cv.FillRect({X, Y, W, H}, pal::Dim);
	}
	Cv.StrokeRoundRect({X, Y, W, H}, 1.5f, Rgba(0, 0, 0, 0.35f), 1.0f);
}

inline uint32_t NetRgb(const Color& Cl)
{
	const auto Byte = [](float V) { return static_cast<uint32_t>(std::min(255.0f, std::max(0.0f, V * 255.0f + 0.5f))); };
	return Byte(Cl.R) << 16 | Byte(Cl.G) << 8 | Byte(Cl.B);
}

/** A champion's frame (their bracelets and rings) for someone the living world knows, or for the player (Hero). */
inline void NetChampion(AvatarSpec& A, const std::string& Name, bool Hero)
{
	const world::World* W = net::Shared().Attached();
	if (!W)
	{
		return;
	}
	if (Hero)
	{
		eventart::Champion(A, W->HeroAwards());
	}
	else if (const world::Npc* N = W->Get(W->Find(Name)))
	{
		eventart::Champion(A, N->Awards);
	}
}

/**
 * A RiverLine profile picture: the icon the name picked, Team RiverLine pros framed in gold, the player in neon.
 * A visible Ring (medals, the rival's purple) replaces the frame; a champion's bracelet or ring frame replaces that.
 * Returns what it drew (its frame says a badge, so the name beside it can leave that one out: NetBadgeFrame).
 */
inline AvatarSpec NetAvatar(Canvas& Cv, float Cx, float Cy, float R, const std::string& Name, const Color& Ring, bool Hero = false)
{
	AvatarSpec A = AvatarFor(Name);
	const int Id = net::Shared().FindPlayer(Name);
	if (Id >= 0 && net::Shared().Players()[static_cast<size_t>(Id)].Pro)
	{
		A.Frame = AvatarFrame::Gold;
	}
	if (Hero)
	{
		A.Frame = AvatarFrame::Neon;
		A.Rim = Ring.A > 0.0f ? NetRgb(Ring) : NetRgb(pal::Accent);
	}
	else if (Ring.A > 0.0f)
	{
		A.Frame = A.Frame == AvatarFrame::Neon ? AvatarFrame::Neon : AvatarFrame::Ring;
		A.Rim = NetRgb(Ring);
	}
	NetChampion(A, Name, Hero);
	DrawAvatar(Cv, Cx, Cy, R, A);
	return A;
}

// ------------------------------------------------------------------ badges (next_segment_spec §5.4 to §5.6)

/** The badge an avatar's frame already says (rule C2): a champion's bracelet or ring frame, Team RiverLine's gold. */
inline uint8_t NetBadgeFrame(const AvatarSpec& A)
{
	return A.Frame == AvatarFrame::Bracelet || A.Frame == AvatarFrame::Gem ? badge::FrameChampion : A.Frame == AvatarFrame::Gold ? badge::FrameGold : badge::FrameNone;
}

/**
 * Someone's badges: the world's people from the shared index (built once per world revision and day), the player's
 * (-1) from the session. Anyone else (a name the world doesn't follow): none.
 */
inline const badge::Set& NetBadgesOf(const Session& S, int Npc)
{
	static const badge::Set None{};
	if (Npc == -1)
	{
		return S.HeroBadges();
	}
	if (Npc < 0 || !S.Living().Get(Npc))
	{
		return None;
	}
	return badge::Shared().Of(S.Living(), Npc, net::DayOf(S.WorldMinutes()));
}

/** Up to Max badges in a context's order (badge::Top, then the next of what's left), the frame's own left out (C2). */
inline std::vector<badge::Badge> NetBadgeRow(const badge::Set& Held, uint8_t Context, uint8_t Frame, int Max)
{
	std::vector<badge::Badge> Out;
	badge::Set Left = Held;
	while (static_cast<int>(Out.size()) < Max)
	{
		const badge::Badge* B = badge::Top(Left, Context, Frame);
		if (!B)
		{
			break;
		}
		const badge::Badge Got = *B;
		Out.push_back(Got);
		Left.Held.erase(std::remove_if(Left.Held.begin(), Left.Held.end(), [&Got](const badge::Badge& X) { return X.K == Got.K; }), Left.Held.end());
	}
	return Out;
}

/** A badge under the pointer this frame: RiverLine::Draw draws its tooltip over everything once the frame is done. */
struct NetTip
{
	bool Up = false;
	badge::Badge B;
	int Npc = -2; // whose (-1: the player; -2: nobody the world follows)
	float X = 0.0f;
	float Y = 0.0f; // the glyph's bottom edge: the tip opens under it
};
/** The tooltip state of one RiverLine (RiverLineCard.cpp keeps it, by RiverLine: its header is Phase A's). */
NetTip& NetTipOf(const RiverLine* RL);
/** The lobby's own small choices that RiverLine.h has no room for (kept beside NetTipOf, by RiverLine). */
struct NetChoices
{
	int WeekTab = 0;                  // the Embercrest page's right panel: 0 the week, 1 the month's leaders
	double WeekTabAt = -100.0;
	std::map<std::string, int> Flip; // an Embercrest event card's middle: 0 the facts, 1 the structure, 2 the payouts
};
NetChoices& NetChoicesOf(const RiverLine* RL);
/**
 * Drops a laptop's tip and choices when it goes (~RiverLine, with the Mail app's, the Backing page's and the badge
 * toasts'): a new laptop made at the same address never inherits an old one's open cards.
 */
void ForgetNet(const RiverLine* RL);
/**
 * Everything someone holds, for a card (§5.4: the network's order, up to Max); the set's own order if the rules give none.
 * Frame: what the card's avatar already says (NetBadgeFrame, rule C2): a champion's frame is the bracelet or the ring, so
 * the rail and the tiles never show a second one (§9.1, "no second bracelet").
 */
std::vector<badge::Badge> NetCardBadges(const badge::Set& Held, int Max, uint8_t Frame = badge::FrameNone);
/**
 * A badge rail (§5.6): glyphs at Size from X, centred on Cy, the first Labelled of them with their labels, in at most
 * MaxW (labels give way first, then glyphs). Hovering one sets the frame's tooltip (Npc: whose). Returns the width drawn;
 * Drawn (when given) gets how many glyphs made it, so a "+N" counts the ones that didn't.
 */
float NetBadgeRail(Canvas& Cv, Ui& UI, const RiverLine* RL, const std::vector<badge::Badge>& Row, int Npc, float X, float Cy, float Size, int Labelled, float MaxW, double Now,
	int* Drawn = nullptr);
/** The public facts a badge's line reads about someone (-1: the player; anyone the world doesn't follow: none). */
badge::Holder NetHolderOf(const Session& S, int Npc);

inline void NetSpark(Canvas& Cv, const Rect& R, const std::array<float, 8>& V, const Color& Col)
{
	float Hi = 1.0f;
	for (float X : V)
	{
		Hi = std::max(Hi, X);
	}
	std::vector<Vec2> Pts;
	for (size_t I = 0; I < V.size(); ++I)
	{
		Pts.push_back({R.X + R.W * Nf(I) / Nf(V.size() - 1), R.Y + R.H - R.H * V[I] / Hi});
	}
	std::vector<Vec2> Area = Pts;
	Area.push_back({R.X + R.W, R.Y + R.H});
	Area.push_back({R.X, R.Y + R.H});
	Cv.FillPolygon(Area, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, NetA(Col, 0.28f), NetA(Col, 0.0f)));
	Cv.StrokePolyline(Pts, false, Col, 1.6f, true);
	Cv.FillCircle(Pts.back().X, Pts.back().Y, 2.4f, Col);
}

inline void NetTrophy(Canvas& Cv, float Cx, float Top, float Sz, const Color& Col, const Color& Col2)
{
	const Paint Metal = Paint::Linear({Cx - Sz * 0.5f, Top}, {Cx + Sz * 0.5f, Top + Sz}, Col, Col2);
	// Handles.
	Cv.StrokeArc(Cx - Sz * 0.36f, Top + Sz * 0.24f, Sz * 0.16f, Pi * 0.5f, Pi * 1.5f, Col2, Sz * 0.05f);
	Cv.StrokeArc(Cx + Sz * 0.36f, Top + Sz * 0.24f, Sz * 0.16f, -Pi * 0.5f, Pi * 0.5f, Col2, Sz * 0.05f);
	// Cup.
	std::vector<Vec2> Cup;
	Cup.push_back({Cx - Sz * 0.38f, Top});
	Cup.push_back({Cx + Sz * 0.38f, Top});
	for (int I = 0; I <= 12; ++I)
	{
		const float A = Nf(I) / 12.0f * Pi;
		Cup.push_back({Cx + Sz * 0.38f * std::cos(A), Top + Sz * 0.12f + Sz * 0.36f * std::sin(A)});
	}
	Cv.FillPolygon(Cup, Metal);
	Cv.FillRect({Cx - Sz * 0.05f, Top + Sz * 0.46f, Sz * 0.1f, Sz * 0.22f}, Metal);
	Cv.FillRoundRect({Cx - Sz * 0.22f, Top + Sz * 0.66f, Sz * 0.44f, Sz * 0.08f}, Sz * 0.02f, Metal);
	Cv.FillRoundRect({Cx - Sz * 0.3f, Top + Sz * 0.74f, Sz * 0.6f, Sz * 0.16f}, Sz * 0.03f, Paint::Linear({0.0f, Top + Sz * 0.74f}, {0.0f, Top + Sz * 0.9f}, rlsites_detail::SitesTone(0x2a2f3a), rlsites_detail::SitesTone(0x12151c)));
	// Shine.
	Cv.FillEllipse(Cx - Sz * 0.16f, Top + Sz * 0.2f, Sz * 0.06f, Sz * 0.16f, Rgba(255, 255, 255, 0.35f));
}

inline void NetCrown(Canvas& Cv, float Cx, float Base, float Sz, const Color& Col)
{
	Cv.FillPolygon({{Cx - Sz * 0.5f, Base}, {Cx - Sz * 0.56f, Base - Sz * 0.62f}, {Cx - Sz * 0.24f, Base - Sz * 0.3f}, {Cx, Base - Sz * 0.78f}, {Cx + Sz * 0.24f, Base - Sz * 0.3f},
		{Cx + Sz * 0.56f, Base - Sz * 0.62f}, {Cx + Sz * 0.5f, Base}}, Col);
	Cv.FillCircle(Cx, Base - Sz * 0.82f, Sz * 0.08f, Col);
	Cv.FillCircle(Cx - Sz * 0.58f, Base - Sz * 0.66f, Sz * 0.07f, Col);
	Cv.FillCircle(Cx + Sz * 0.58f, Base - Sz * 0.66f, Sz * 0.07f, Col);
}

inline std::vector<std::string> NetWrap(Canvas& Cv, const std::string& Text, float MaxW, float Size, int Weight)
{
	std::vector<std::string> Out;
	std::string Line;
	size_t Start = 0;
	while (Start <= Text.size())
	{
		size_t End = Text.find(' ', Start);
		End = End == std::string::npos ? Text.size() : End;
		const std::string Word = Text.substr(Start, End - Start);
		const std::string Test = Line.empty() ? Word : Line + " " + Word;
		if (Cv.Measure(Test, Size, Weight) > MaxW && !Line.empty())
		{
			Out.push_back(Line);
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
		Out.push_back(Line);
	}
	return Out;
}

/** Draws wrapped text, at most MaxLines lines (the last one ellipsized); returns the y after the last line. */
inline float NetParagraph(Canvas& Cv, const std::string& Text, float X, float Y, float MaxW, float Size, int Weight, const Color& Col, float LineH, int MaxLines = 99)
{
	std::vector<std::string> Lines = NetWrap(Cv, Text, MaxW, Size, Weight);
	for (size_t I = 0; I < Lines.size() && static_cast<int>(I) < MaxLines; ++I)
	{
		std::string L = Lines[I];
		if (static_cast<int>(I) == MaxLines - 1 && I + 1 < Lines.size())
		{
			L += " " + Lines[I + 1];
		}
		Cv.Text(L, X, Y, Ts(Size, Weight, Col, Align::Left, Baseline::Alphabetic, false, MaxW));
		Y += LineH;
	}
	return Y;
}

inline std::string NetFormat(const net::EventTemplate& T)
{
	switch (T.Fmt)
	{
	case net::Format::Freezeout: return "Freezeout";
	case net::Format::ReEntry: return "Re-entry";
	case net::Format::Bounty: return "Progressive KO";
	case net::Format::Mystery: return "Mystery Bounty";
	case net::Format::Satellite: return "Satellite";
	case net::Format::Flip: return "Flip & Run";
	}
	return "";
}

inline std::string NetAgo(double Minutes, double At)
{
	if (Minutes < 1.0)
	{
		return "just now";
	}
	if (Minutes < 60.0)
	{
		return std::to_string(static_cast<int>(Minutes)) + "m ago";
	}
	if (Minutes < 24.0 * 60.0)
	{
		return std::to_string(static_cast<int>(Minutes / 60.0)) + "h ago";
	}
	return net::DateLabel(net::DayOf(At));
}

inline std::string NetUpper(const std::string& In)
{
	std::string Out = In;
	for (char& Ch : Out)
	{
		Ch = Ch >= 'a' && Ch <= 'z' ? static_cast<char>(Ch - 'a' + 'A') : Ch;
	}
	return Out;
}

inline std::string NetLower(const std::string& In)
{
	std::string Out = In;
	for (char& Ch : Out)
	{
		Ch = Ch >= 'A' && Ch <= 'Z' ? static_cast<char>(Ch - 'A' + 'a') : Ch;
	}
	return Out;
}

/** The part of a series event's name after "MM #26: ". */
inline std::string NetShortName(const std::string& Name)
{
	const size_t P = Name.find(": ");
	return P == std::string::npos ? Name : Name.substr(P + 2);
}

inline Color NetEdge(const net::Network& Net, const net::EventTemplate& T)
{
	if (!T.Series.empty())
	{
		if (const net::SeriesInfo* Sr = Net.FindSeries(T.Series))
		{
			return Hex(Sr->Color);
		}
	}
	return NetTier(net::TierOf(T.BuyInCents));
}

inline const char* NetBoardName(net::Board B)
{
	switch (B)
	{
	case net::Board::Earnings: return "Money List";
	case net::Board::Season: return "Player of the Year";
	case net::Board::Wins: return "Titles";
	case net::Board::FinalTables: return "Final Tables";
	case net::Board::Series: return "Series Leaderboard";
	case net::Board::NightShift: return "Night Shift";
	case net::Board::Live: return "Live Player of the Year";
	}
	return "";
}

// ------------------------------------------------------------------ the Embercrest

/** The Embercrest's colours (art/blender/brand.py): midnight navy, ash white and ember copper. */
const uint32_t EcMidnight = 0x0a1020;
const uint32_t EcNavy = 0x121c33;
const uint32_t EcSlate = 0x1d2a47;
const uint32_t EcIce = 0x8d9ab4;
const uint32_t EcAsh = 0xebe7df;
const uint32_t EcEmberDeep = 0x9e2f0c;
const uint32_t EcEmber = 0xd9531e;
const uint32_t EcEmberHot = 0xf2832e;
const uint32_t EcGold = 0xf6b04a;
const uint32_t EcFlame = 0xffd78a;

/**
 * The Embercrest's mark (brand.py's mark()): the ember rising behind a faceted three-peak crest, in a Size box centred
 * on (Cx, Cy). Ground is what it sits on: a rim of it parts the crest from the ember.
 */
inline void NetEmbercrest(Canvas& Cv, float Cx, float Cy, float Size, const Color& Ground)
{
	const float U = Size / 100.0f;
	const float Base = Cy + 28.0f * U;
	auto P = [&](float X, float Y) { return Vec2{Cx + X * U, Base - Y * U}; };
	// The ember: its glow, then the disc from deep ember at the rim to flame at the heart, hottest low where the light
	// breaks over the ridge.
	const Vec2 E = P(0.0f, 42.0f);
	Cv.FillCircle(E.X, E.Y, 47.0f * U, Paint::Radial(E, 30.0f * U, E, 47.0f * U, Hex(EcEmberHot, 0.32f), -1.0f, Color(), Hex(EcEmberHot, 0.0f)));
	Cv.FillCircle(E.X, E.Y, 34.0f * U, Paint::Radial({E.X, E.Y + 7.5f * U}, 0.0f, E, 34.0f * U, Hex(EcFlame), 0.5f, Hex(EcEmberHot), Hex(EcEmberDeep)));
	// The crest: two shoulders and the high middle peak.
	Cv.FillPolygon({P(-51.0f, -1.5f), P(-26.0f, 37.5f), P(-15.0f, 25.5f), P(0.0f, 64.0f), P(13.0f, 32.5f), P(25.0f, 43.5f), P(51.0f, -1.5f)}, Ground);
	const Vec2 Lp = P(-26.0f, 34.0f), V1 = P(-15.0f, 22.0f), Top = P(0.0f, 60.0f), V2 = P(13.0f, 29.0f), Rp = P(25.0f, 40.0f), Rb = P(48.0f, 0.0f);
	Cv.FillPolygon({P(-48.0f, 0.0f), Lp, V1, Top, V2, Rp, Rb}, Hex(EcAsh));
	// The faces away from the light, one per peak, and snow on the high peak's lit face.
	Cv.FillPolygon({Lp, V1, P(-13.0f, 0.0f), P(-21.0f, 0.0f)}, Hex(EcIce));
	Cv.FillPolygon({Top, V2, P(15.0f, 0.0f), P(3.0f, 0.0f)}, Hex(EcIce));
	Cv.FillPolygon({Rp, Rb, P(31.0f, 0.0f)}, Hex(EcIce));
	Cv.FillPolygon({Top, P(-8.0f, 43.0f), P(-4.5f, 46.5f), P(-1.5f, 42.0f), P(1.6f, 49.0f)}, Hex(0xffffff));
	// The ground line, parted under the high peak.
	Cv.FillRect({Cx - 48.0f * U, Base + 4.1f * U, 45.0f * U, 2.4f * U}, Hex(EcEmberHot));
	Cv.FillRect({Cx + 3.0f * U, Base + 4.1f * U, 45.0f * U, 2.4f * U}, Hex(EcEmberHot));
}

/** Each of the room's events in its own colour (the stripe down its card, its dots on the week). */
inline Color LiveColor(const live::EventTemplate& T)
{
	const std::string Key = T.Key;
	return Key == "nightly" ? Hex(EcEmberHot) : Key == "sunday" ? Hex(EcGold) : Key == "turbo" ? pal::Red : Key == "bigstack" ? Hex(0xb48cff)
		: Key == "warmup" ? Hex(0x5fd3b0) : Hex(0x7cc4ff);
}

/** "8 hours", "4.5 hours", "an hour". */
inline std::string LiveHours(double Hours)
{
	const double Tenths = std::round(Hours * 10.0) / 10.0;
	if (Tenths == 1.0)
	{
		return "an hour";
	}
	return (Tenths == std::floor(Tenths) ? std::to_string(static_cast<int>(Tenths)) : Fixed(Tenths, 1)) + " hours";
}

/** A break's length and how often it comes: "15 min every 2 hours". */
inline std::string LiveBreaks(const live::EventTemplate& T)
{
	const double Every = static_cast<double>(T.BreakEvery) * T.LevelMinutes / 60.0;
	return std::to_string(static_cast<int>(T.BreakMinutes)) + " min every " + (Every == 1.0 ? std::string("hour") : LiveHours(Every));
}

/** A start on the hour without its minutes: "7 PM", "10:30 PM". */
inline std::string LiveTime(double World)
{
	std::string Label = net::TimeLabel(World);
	const size_t At = Label.find(":00 ");
	return At == std::string::npos ? Label : Label.erase(At, 3);
}

/** A line on an event card that weighs it against the player's money. */
struct LiveWarning
{
	std::string Text;
	int Severity = 0; // 0 a note, 1 something to weigh (it costs the rent), 2 it can't be done (the money isn't there)
};

/**
 * One of the Embercrest's events as its cards show it (the Burner's rows, RiverLine's Embercrest page, the second
 * screen, the wall calendar): what the room publishes about it, where it stands at World, and what entering costs
 * against the bankroll and the rent. It decides nothing: registering is Session::GoToLive's, and Why is the reason it
 * would give.
 */
struct LiveCard
{
	/** Where the night stands. */
	enum Stage : int
	{
		Soon,    // the desk isn't open yet
		Desk,    // the desk is taking entries, cards haven't started
		LateReg, // cards are in the air and the bus still makes late registration
		Closed,  // registration has closed (for the player: the bus wouldn't make it)
		Over,    // played out, or called off
	};
	live::Occurrence O;
	const life::LiveEntry* Entry = nullptr; // the player's, in any state
	bool Registered = false;                // that entry is live: they're in it
	int Phase = Soon;
	std::string When;   // "Today", "Tomorrow", "Thursday"
	std::string Split;  // "$105 + $15 fee"
	int StackBb = 0;    // the starting stack in big blinds at level 1
	double RunMinutes = 0.0; // about how long a deep run takes with the field expected (live::DeepRunMinutes, to the half hour)
	double Ends = 0.0;       // about when it ends (world minutes)
	int Field = 0;      // the field: the entry's, the world's plan once it has one, or who came to one called off
	bool Planned = false; // Field is known (else the card states the room's usual crowd; the world plans a day ahead)
	bool Cancelled = false; // called off short of its minimum (World::Cancel): nobody played it
	std::vector<std::pair<std::string, std::string>> Known; // who the player knows in the world's plan, and how ("Sal", "Rival")
	std::string Status; // "Desk opens 4:00 PM", "Late reg till 8:00 PM", "You're registered", "11th of 97"
	std::string Why;    // why registering wouldn't go through right now ("" when it would, or the player is in)
	std::vector<LiveWarning> Warnings; // the most serious first
	// Who pays (next_segment_spec §1.2): the entry's funding once registered, else the way Session::GoToLive would
	// register it now (Session::FundingOptions' first that can: a backer's stake, a cover, a package, then the player's
	// own money with any pieces sold).
	life::Funding Fund;
	std::string FundLine; // live::FundingLine: "Big Lou is putting you in the Nightly." ("" for the player's own money, nothing sold)
	std::string Cover;    // a sponsor's cover that takes it: "Covered by Team Embercrest · 1 of 2 left" ("" for none)
	Chips Cost = 0;       // what leaves the bankroll to play it: the buy-in (less the pieces' money) and the bus, unless someone else pays
	bool Sellable = false; // pieces of it can go up for sale on the Backing page (the player's own money, not registered yet)
	int Asked = 0;         // pieces listed on it (permille), and filled so far
	int Filled = 0;
};
/**
 * The card for O as the player stands at World. PlannedField is the world's planned field for it (World::PlannedEntries;
 * 0 when it hasn't planned that day yet): RiverLine::CardFor passes it with who the player knows in it.
 */
LiveCard LiveCardOf(const Session& S, const live::Occurrence& O, double World, int PlannedField = 0);
/** The events the bus can still make (live::Reachable), less any the room has called off. */
std::vector<live::Occurrence> LiveAhead(const Session& S, double World, double HoursAhead);
/**
 * Whether pieces of the occurrence can go up for sale now, as the Backing page's sell panel picks its events (the next
 * six the bus can make within six days that the player hasn't registered for and would pay for with their own money):
 * the event card's Sell action opens the page on it (backing_detail::Preselect).
 */
bool LiveForSale(const Session& S, const std::string& OccurrenceId, double World);
/** A deep run in O with Entrants (the player included), to the half hour: its length into the card, and when it ends. */
inline void LiveRun(LiveCard& Card, int Entrants)
{
	Card.RunMinutes = std::round(live::DeepRunMinutes(Card.O, Entrants) / 30.0) * 30.0;
	Card.Ends = Card.O.Start + Card.RunMinutes;
}
/** What it costs to play O: the buy-in and the bus there and back. */
inline Chips LiveCost(const live::Occurrence& O)
{
	return O.Valid() ? O.T->BuyInCents + 2 * live::BusFareCents : 0;
}
/**
 * What leaves the bankroll to play O paid for by F (live::CanRegister's sums): a backer, a package or a ticket pays the
 * buy-in and the bus is the player's; a sponsor's cover may take the bus too; the player's own money is the buy-in less
 * what the pieces sold on it bring in at the desk, and the bus.
 */
inline Chips LiveCostWith(const live::Occurrence& O, const life::Funding& F)
{
	if (!O.Valid())
	{
		return 0;
	}
	const Chips Bus = 2 * live::BusFareCents;
	switch (F.From)
	{
	case life::Paid::Own: return std::max<Chips>(0, O.T->BuyInCents - std::max<Chips>(0, F.Cents)) + Bus;
	case life::Paid::Sponsor: return F.Fares ? 0 : Bus;
	default: return Bus;
	}
}

/** A share in permille as people say it: "35%", "12.5%". */
inline std::string NetPermille(int P)
{
	return std::to_string(P / 10) + (P % 10 == 0 ? std::string() : "." + std::to_string(P % 10)) + "%";
}

/** What leaves the bank when the player confirms a night, paid the way the card says (the bus up is one fare now). */
inline std::string LiveLeavesNow(const LiveCard& Card, Chips Bankroll)
{
	const Chips Fare = live::BusFareCents;
	const Chips BuyIn = Card.O.Valid() ? Card.O.T->BuyInCents : 0;
	const std::string Who = Card.Fund.Who.empty() ? std::string("Your backer") : Card.Fund.Who;
	switch (Card.Fund.From)
	{
	case life::Paid::Own:
	{
		const Chips Desk = std::max<Chips>(0, BuyIn - std::max<Chips>(0, Card.Fund.Cents));
		return "Leaves now: " + NetMoney(Desk) + " at the desk" + (Card.Fund.Cents > 0 ? " (your buyers put in " + NetMoney(Card.Fund.Cents) + ")" : std::string()) + " and " +
			NetMoney(Fare) + " for the bus \xC2\xB7 balance after " + Money(Bankroll - Desk - Fare);
	}
	case life::Paid::Sponsor:
		return Card.Fund.Fares ? "Leaves now: nothing. " + Who + " covers the " + NetMoney(BuyIn) + " and the bus \xC2\xB7 balance stays " + Money(Bankroll)
							   : "Leaves now: " + NetMoney(Fare) + " for the bus. " + Who + " covers the " + NetMoney(BuyIn) + " \xC2\xB7 balance after " + Money(Bankroll - Fare);
	case life::Paid::Package:
		return "Leaves now: " + NetMoney(Fare) + " for the bus. " + Who + "'s roll pays the " + NetMoney(BuyIn) + " \xC2\xB7 balance after " + Money(Bankroll - Fare);
	default:
		return "Leaves now: " + NetMoney(Fare) + " for the bus. " + Who + " pays the " + NetMoney(BuyIn) + " \xC2\xB7 balance after " + Money(Bankroll - Fare);
	}
}

/** The register button as the funding has it: "Register · $120", "Register · covered" (§3.9), and its second line. */
struct LiveRegisterLabel
{
	std::string Text;
	std::string Sub;
};

inline LiveRegisterLabel LiveRegisterLabelOf(const LiveCard& Card, Chips Fare)
{
	const Chips BuyIn = Card.O.Valid() ? Card.O.T->BuyInCents : 0;
	const std::string Bus = "+ the bus, " + NetMoney(2 * Fare);
	switch (Card.Fund.From)
	{
	case life::Paid::Backer: return {"Register \xC2\xB7 backed", (Card.Fund.Who.empty() ? std::string("Your backer") : Card.Fund.Who) + " pays the " + NetMoney(BuyIn)};
	case life::Paid::Sponsor: return {"Register \xC2\xB7 covered", Card.Fund.Fares ? std::string("the buy-in and the bus") : Bus};
	case life::Paid::Package: return {"Register \xC2\xB7 the roll", Bus};
	case life::Paid::Ticket: return {"Register \xC2\xB7 ticket", Bus};
	default: return {"Register \xC2\xB7 " + NetMoney(BuyIn), Card.Fund.Cents > 0 ? Bus + " \xC2\xB7 buyers " + NetMoney(Card.Fund.Cents) : Bus};
	}
}

/** The line a card leads its notes with about who pays: a cover's count (with the bus), else live::FundingLine. */
inline std::string LivePaying(const LiveCard& Card)
{
	if (Card.Fund.From == life::Paid::Sponsor && !Card.Cover.empty())
	{
		return Card.Cover + (Card.Fund.Fares ? " \xC2\xB7 the bus too" : "");
	}
	return Card.FundLine;
}
} // namespace rlnet_detail

/**
 * Update 2 (§2.6.1, §2.9, §2.11): RiverLine's autoplay screens (RiverLineAuto.cpp). The panel's open state lives beside
 * the class, by laptop (RiverLine.h is frozen); the table, the tabs, the tiles, the results and Career read through here.
 */
namespace rlauto_detail
{
// The copy these screens add to core's (autoplay::Text, style::Catalog). Sentences are in AutoUiInventory (the lint).
inline constexpr const char* AutoSubtitle = "Your plan plays your seat in the style you pick. Any hand is yours again with one press of K.";
inline constexpr const char* AutoCloseHint = "K or Backspace closes this.";
inline constexpr const char* AutoSharpOff = "Read when your plan takes the seat.";
inline constexpr const char* AutoFormNote = "Form is your own graded play online. From 80 it sharpens your plan; at 95 it's as sharp as it gets.";
inline constexpr const char* AutoSloppy = "Under 70% your plan gets sloppy: eat, rest, play some hands yourself.";
inline constexpr const char* AutoThinkHint = "Any action takes the wheel.";
inline constexpr const char* AutoYourHand = "This hand is yours. Your plan's back on the next one.";
inline constexpr const char* AutoYourHandKeys = "Your plan's back next hand. K turns autoplay off.";
// Autoplay turned on while the player's own decision was open: that one stays theirs, the plan takes the next.
inline constexpr const char* AutoYourDecision = "This decision's yours. Your plan takes the next one.";
inline constexpr const char* AutoPending = "Skipping from the next hand.";
inline constexpr const char* AutoDefaultSaved = "Saved. New tables start with this plan.";
inline constexpr const char* AutoEveryDecision = "Your plan made every decision, so there's nothing of yours to review.";
inline constexpr const char* AutoXpOwn = "XP comes from your own decisions, never your plan's.";
/** The closed table's toast where "no decisions" was (§2.9): a fragment, like the line it replaces. */
inline constexpr const char* AutoPlayedToast = "played by your plan";

/** Whether the autoplay panel is open on this laptop. */
bool AutoPanelOpen(const RiverLine* RL);
/**
 * Opens the panel over open table Table (an index; the panel closes by itself if that table goes). Opening it marks the
 * career's autoplay\tpref line as kept (Life.h's U2AutoPref: "opening the panel and any autoplay use", §6.2).
 */
void AutoOpenPanel(const RiverLine* RL, Session& S, int Table, double Now);
void AutoClosePanel(const RiverLine* RL);
/** Drops a laptop's autoplay state when it goes (~RiverLine). */
void ForgetAuto(const RiverLine* RL);
/**
 * K, Backspace, and an action key during the plan's think, at the table in front (§2.6.1, §2.6.5): true when taken.
 * The panel open: K or Backspace closes it. Backspace stops a skip. K with autoplay off hands the seat to the plan the
 * panel shows; with it on, Session::TakeOver. F C X R B A while the plan thinks take the wheel and do nothing else.
 */
bool AutoKey(const RiverLine* RL, Session& S, const std::string& Key);
/** What the plan played in a finished event (its played line), when it played any of it; else null. */
const autoplay::Played* AutoPlayedFor(const Session& S, const Results& R);
const autoplay::Played* AutoPlayedFor(const Session& S, const HistoryEntry& H);
/** A table's one line while the plan holds or skips it ("AUTO · GTO 2x · CALLED 3,200", the skip's line); "" when off. */
std::string AutoTileLine(const autoplay::Panel& P);
/**
 * A table tab's line for the plan, whole, at the largest size that fits MaxW (11 or 10): the tile's status ("AUTO · GTO
 * 2x", "SKIP · 31 to the bubble"), else its short form ("AUTO · 8x", "SKIP · Bubble": the skip's target, not its count).
 * Size returns the size to draw it at (weight 600, the text face).
 */
std::string AutoTabLine(Canvas& Cv, const autoplay::Panel& P, const std::string& Status, float MaxW, float& Size);
/** Every sentence the autoplay screens draw beyond core's own copy, for the copy lint. */
std::vector<std::string> AutoUiInventory();
} // namespace rlauto_detail

/**
 * Update 2, G2 (§4.8, §4.9): Rerun's entry points for the rest of the laptop. The app and its side state (the page, what's
 * been seen since the last visit) are ui-video's (RiverLineVideo.cpp, RiverLine::VideoApp); the taskbar's ninth button
 * and its dot, Mail's link and ~RiverLine (ui-laptop's) reach it through these.
 */
namespace rlvideo_detail
{
/** Switches the laptop to Rerun at a page: 0 Studio, 1 Channel (Mail's Rerun letters open 1). */
void VideoOpen(RiverLine& RL, int Page, double Now);
/** The taskbar's marigold dot: three or more candidates new since the last visit, or a bell milestone not yet seen. */
bool VideoDot(const RiverLine* RL, const Session& S);
/** Drops a laptop's Rerun state when it goes (~RiverLine). */
void ForgetVideo(const RiverLine* RL);
/** Every sentence Rerun's pages draw beyond core's own copy, for the copy lint. */
std::vector<std::string> VideoUiInventory();
} // namespace rlvideo_detail

/**
 * Update 2, G3 (§1.8, §5.5, §5.11): the rival sites on the laptop (RiverLineSites.cpp). Which site the client wears and
 * in whose colours (a SkinScope for its SiteDef's palette), the player's money and record there, the poker button's
 * list, the Cashier and Promotions pages, Career's Rewards card, and a site's own shades of RiverLine's literal colours.
 * Side state lives here by laptop, as Mail's does (RiverLine.h is frozen).
 */
namespace rlsites_detail
{
/** Whether a rival site is registered (core-sites' SiteDef in net::Sites()); RiverLine always is. */
bool SitesRegistered(net::SiteId Site);
/** The player's account on a rival (null for RiverLine, or a site not joined). */
const site::Account* SitesAccount(const Session& S, net::SiteId Site);
/** The sites the player plays: RiverLine first, then each joined rival that's registered, in SiteId order. */
std::vector<net::SiteId> SitesPlayed(const Session& S);
/** Money to play with on a site: RiverLine's is the bank (the bankroll), a rival's its wallet (0 with no account). */
Chips SitesBalance(const Session& S, net::SiteId Site);
/** What's on its way back from a site (pending withdrawals, net of their fees): the sum, or one site's (Site < Count). */
Chips SitesInTransit(const Session& S, net::SiteId Site = net::SiteId::Count);
/** A site's palette as a skin (RiverLine's is pal::'s own values). */
Skin SitesSkin(net::SiteId Site);
/** True while pal:: holds RiverLine's values (outside any rival's scope): RiverLine's literal colours stay exact. */
bool SitesHome();
/**
 * One of RiverLine's literal dark-ground or ink colours, in the skin being drawn: exactly Hex(Rgb) at home; inside a
 * rival's scope, the same shade moved with the nearest of RiverLine's 16 tokens to that token's place in the skin, so
 * panels and rows sit on the site's own ground (§1.8: every P0 skin is dark).
 */
Color SitesTone(uint32_t Rgb);
/** Ink on the accent (a lit chip, a primary button): RiverLine's dark teal at home, the skin's accent darkened away. */
Color SitesOnAccent();
/** The accent's deeper end (a lit chip's gradient): RiverLine's at home, the skin's accent darkened a little away. */
Color SitesAccentDeep();
/** Ink on gold, and gold's deeper end, the same way. */
Color SitesOnGold();
Color SitesGoldDeep();
/** The felt's lit middle: RiverLine's literal at home, the skin's felt brightened inside a rival's scope. */
Color SitesFeltLight();
/** Where the poker button's list of sites opens (above the taskbar's first button), for the input under it. */
Rect SitesMenuRect(const Session& S);
/** The site the laptop's poker client wears now: the lobby's site, the table in front's, a result's event's. */
net::SiteId SitesClient(const RiverLine& RL, const Session& S);
/** The lobby's site (RiverLine::SiteShown) while the player holds an account there; else RiverLine. */
net::SiteId SitesShown(const RiverLine& RL, const Session& S);
/** A table's site (its event id's prefix). */
net::SiteId SitesOfTable(const Session& S);
/**
 * The player's numbers on a rival's boards (§5.8): its season from the account, the series and month points from the
 * results there (net::Points, as the network scores them). RiverLine's stay the session's (net::StatsFrom).
 */
net::HeroStats SitesHero(const Session& S, net::SiteId Site, const net::Network& Net, double World, net::Board For);
/** The boards a site shows, in order (Ignite: Month, Season; RunItTwice: Season, Wins, the series while one runs). */
std::vector<net::Board> SitesBoards(net::SiteId Site, bool SeriesLive);
/** A board's name on a site ("This Month", "Season"); RiverLine's are NetBoardName's. */
std::string SitesBoardName(net::SiteId Site, net::Board B);
/** What a rival's board is, for the page's right panel. */
std::string SitesBoardAbout(const Session& S, net::SiteId Site, net::Board B, double World);
/**
 * The warning before a strike (§5.7): "Strike under your Team RiverLine deal" when playing (or streaming, or posting a
 * video) on Site breaks a running deal's "plays only on" clause; "" when nothing would.
 */
std::string SitesClauseWarning(const Session& S, net::SiteId Site, bool Streaming, bool Video);
/** A site's short name in capitals, for a slide's kicker ("TONIGHT ON IGNITE"). */
std::string SitesUpperName(net::SiteId Site);
/** A site's short name ("Ignite", "RunItTwice", "RiverLine"): the lobby, the buttons, the boards. */
std::string SitesShortName(net::SiteId Site);
/** Its full name where there's room ("Ignite Poker"): the poker button's list, the Bank's pots, Mail. */
std::string SitesLongName(net::SiteId Site);
/**
 * A button in the skin being drawn: exactly Ui::Button at home; inside a rival's scope the same layout in the skin's own
 * colours (Ui.cpp's button shades are RiverLine's literals).
 */
bool SitesButton(Ui& UI, Canvas& Cv, const std::string& Id, const Rect& R, const std::string& Label, const ButtonOpts& Opts);
/** The poker button's list of sites is open (the taskbar's caret). */
bool SitesMenuOpen(const RiverLine* RL);
void SitesMenuToggle(const RiverLine* RL, bool Open);
/** The rival lobby's pages after its first four (§5.11: PROMOTIONS, CASHIER), and the Cashier's small choices. */
struct SitesUi
{
	bool MenuOpen = false;
	double MenuAt = -100.0;
	int Deposit = 1;         // the deposit step ($10, $50, $100, $500)
	Chips Withdraw = 0;      // the amount picked to take out (0: not picked yet)
	std::string Note;        // the Cashier's last answer
	bool NoteBad = false;
	double NoteAt = -100.0;
	double PageAt = -100.0;
};
SitesUi& SitesOf(const RiverLine* RL);
/** Drops a laptop's sites state when it goes (~RiverLine). */
void ForgetSites(const RiverLine* RL);
/**
 * The rent card's line (§5.5) when the bank alone can't cover the rent but the bank and the sites could: core-sites'
 * words (site::RentWarning), "" otherwise.
 */
std::string SitesRentLine(const Session& S, double World);
/** One of the pots the Bank lists beside the balance once a site holds money or money is on its way back (§5.11). */
struct SitesPot
{
	std::string Kicker;
	std::string Name;
	Chips Cents = 0;
	std::string Sub;
	uint32_t Col = 0;
	net::SiteId Site = net::SiteId::Count; // a site's wallet wears its mark
};
/** Whose a letter in Mail is (Source "site": the thread names the site by its key, "ignite" or "ignite:welcome"; else the
 * rep or the site's name in From); SiteId::Count for a letter that isn't a site's.
 */
net::SiteId SitesOfMail(const std::string& Source, const std::string& Thread, const std::string& From);
/** A site's latest invitation, if it sent one. */
const site::Invitation* SitesInvite(const Session& S, net::SiteId Site);
/** Whether the player has played on a rival (Kast's channel says "Poker · tournaments" from then on). */
bool SitesPlayedRival(const Session& S);
/** Every sentence the sites' screens draw, for the copy lint. */
std::vector<std::string> SitesUiInventory();
/** The words Mail and the taskbar use for a site's letters and buttons. */
inline constexpr const char* SitesAcceptLabel = "Accept";
inline constexpr const char* SitesDeclineLabel = "Decline";
inline constexpr const char* SitesCashierLink = "Open the Cashier";
inline constexpr const char* SitesPromotionsLink = "See the promotions";
inline constexpr const char* SitesRewardsLink = "Open Career";
inline constexpr const char* SitesInviteOnce = "Decline and they'll write once more, in a month.";
inline constexpr const char* SitesInviteLast = "They wrote again. They won't a third time.";
inline constexpr const char* SitesInviteDeclined = "You passed. They'll write once more, in a month.";
// The sites' sentences the lobby draws (filled with SitesFill; SitesUiInventory hands the templates to the copy lint).
inline constexpr const char* SitesAboutMonth = "This month's race on {Site}: points from every final table since the 1st, the bigger the field and the deeper the run the more. It starts again next month.";
inline constexpr const char* SitesAboutSeason = "The season on {Site}: points from every final table since its lobby opened on {Date}. Nothing from RiverLine counts here.";
/** A rival's season after its first year: the world starts every rival's points season again on January 1. */
inline constexpr const char* SitesAboutSeasonYear = "The season on {Site}: points from every final table since January. Nothing from RiverLine counts here.";
/** Ignite's welcome bonus once its 60 days are up (core-sites drops what was still pending). */
inline constexpr const char* SitesBonusClosed = "The 60 days ran out on {Date}. What wasn't released by then went with them.";
/** A second invitation declined: the site won't write again. */
inline constexpr const char* SitesInviteDone = "You passed twice. They won't write again.";
/** RunItTwice's welcome tickets once a first deposit under $50 has passed them by. */
inline constexpr const char* SitesTicketsMissed = "Your first deposit was under $50, so the welcome tickets passed you by.";
inline constexpr const char* SitesAboutWins = "Titles won on {Site} this season. The board for closers.";
inline constexpr const char* SitesAboutSeries = "Points from every final table at {Series}.";
inline constexpr const char* SitesAboutAny = "Points from every final table on {Site}.";
inline constexpr const char* SitesBonusSlide = "On your first deposit, released $5 for every $20 of rake you pay here.";
inline constexpr const char* SitesClubSlide = "Rakeback on the 1st by the month's rake, from 15% at $50.";
inline constexpr const char* SitesOwnMoney = "{Backer} funds RiverLine events only. This one's your own money.";
inline constexpr const char* SitesStrikeConfirm = "Confirm and it's a strike under the deal.";
inline constexpr const char* SitesKastClient = "A poker client and a stream encoder at once is too much for it: the fans scream and the stream drops out.";
/** Fills a template's {Key}s. */
std::string SitesFill(std::string Text, const std::vector<std::pair<std::string, std::string>>& Vars);
/** Studio's PUBLISH once it has warned of a clause strike (RiverLineVideo.cpp). */
inline constexpr const char* SitesPublishArmed = "Press again and it's a strike under the deal.";
} // namespace rlsites_detail
} // namespace ui
} // namespace ss
