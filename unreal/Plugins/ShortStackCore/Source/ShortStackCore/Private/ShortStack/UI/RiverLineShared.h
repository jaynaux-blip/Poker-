// Drawing helpers shared by the RiverLine lobby pages and the laptop apps (private to the UI sources).
#pragma once

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/UI/RiverLine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace ss
{
namespace ui
{
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
	case net::Tier::Freeroll: return Hex(0x3ecf6e);
	case net::Tier::Micro: return Hex(0x27d3c3);
	case net::Tier::Low: return Hex(0x3b82f6);
	case net::Tier::Mid: return Hex(0x9b6bff);
	case net::Tier::High: return Hex(0xf2c14e);
	}
	return pal::Accent;
}

inline Color NetStatus(net::Status St)
{
	switch (St)
	{
	case net::Status::Announced: return Hex(0x6b7a93);
	case net::Status::Registering: return Hex(0x4f9bff);
	case net::Status::LateReg: return Hex(0x3ecf6e);
	case net::Status::Running: return Hex(0x9aa7bd);
	case net::Status::FinalTable: return Hex(0xf2c14e);
	case net::Status::Finished: return Hex(0x4d5b73);
	}
	return pal::Muted;
}

inline Color NetKind(net::NewsKind K)
{
	switch (K)
	{
	case net::NewsKind::BigWin: return Hex(0xf2c14e);
	case net::NewsKind::Series: return Hex(0xb8ff2e);
	case net::NewsKind::Schedule: return Hex(0x4f9bff);
	case net::NewsKind::Record: return Hex(0xf28a3a);
	case net::NewsKind::Hero: return Hex(0x27d3c3);
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
	Cv.Text(Label, X + W / 2.0f, Y + H / 2.0f + 0.5f, Ts(Size, 700, Solid ? Hex(0x07121c) : Col, Align::Center, Baseline::Middle));
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

inline int NetHue(const std::string& Name)
{
	return static_cast<int>(Fnv1a(Name) % 360u);
}

inline std::string NetInitials(const std::string& Name)
{
	std::string Out;
	for (const char Ch : Name)
	{
		if ((Ch >= 'a' && Ch <= 'z') || (Ch >= 'A' && Ch <= 'Z') || (Ch >= '0' && Ch <= '9'))
		{
			Out.push_back(Ch >= 'a' && Ch <= 'z' ? static_cast<char>(Ch - 'a' + 'A') : Ch);
		}
		if (Out.size() == 2)
		{
			break;
		}
	}
	return Out.empty() ? std::string("?") : Out;
}

inline void NetAvatar(Canvas& Cv, float Cx, float Cy, float R, const std::string& Name, int Hue, const Color& Ring)
{
	const float H = Nf(Hue);
	Cv.FillCircle(Cx, Cy, R, Paint::Linear({Cx - R, Cy - R}, {Cx + R, Cy + R}, Hsl(H, 0.62f, 0.52f), Hsl(H + 30.0f, 0.55f, 0.26f)));
	Cv.Text(NetInitials(Name), Cx, Cy + R * 0.02f, Ts(R * 0.78f, 800, Hex(0xffffff), Align::Center, Baseline::Middle));
	if (Ring.A > 0.0f)
	{
		Cv.StrokeEllipse(Cx, Cy, R + 1.5f, R + 1.5f, Ring, 2.5f);
	}
}

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
	Cv.FillRoundRect({Cx - Sz * 0.3f, Top + Sz * 0.74f, Sz * 0.6f, Sz * 0.16f}, Sz * 0.03f, Paint::Linear({0.0f, Top + Sz * 0.74f}, {0.0f, Top + Sz * 0.9f}, Hex(0x2a2f3a), Hex(0x12151c)));
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
	case net::Format::Flip: return "Flip & Go";
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
	}
	return "";
}
} // namespace rlnet_detail
} // namespace ui
} // namespace ss
