// Kast: the streaming site. The studio (the stream as viewers see it, the controls, the numbers, chat), the channel
// (growth, money, sponsors, clips) and the Poker directory. While live, RiverLine shows a LIVE pill, the alerts and a
// Stream tab with chat at the table.
#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "RiverLineShared.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/UI/Avatars.h"
#include "ShortStack/UI/StreamArt.h"

#include <algorithm>
#include <cmath>

namespace ss
{
namespace ui
{
using namespace rlnet_detail;

namespace rlkast_detail
{
const float KastH = 962.0f;
const Color KBg = Hex(0x0e0a16);
const Color KPanel = Hex(0x17121f);
const Color KPanel2 = Hex(0x201832);
const Color KLine = Hex(0x2d2442);
const Color KInk = Hex(0xf1ecff);
const Color KMuted = Hex(0x9d93b8);
const Color KDim = Hex(0x625a7c);
const Color KViolet = Hex(kast::Violet);
const Color KLime = Hex(kast::Lime);
const Color KRed = Hex(0xff3b5c);
const Color KGold = Hex(0xf5c542);

std::string KastCount(double V)
{
	const long long N = static_cast<long long>(std::round(V));
	if (N >= 100000)
	{
		return std::to_string(N / 1000) + "K";
	}
	if (N >= 10000)
	{
		return Fixed(static_cast<double>(N) / 1000.0, 1) + "K";
	}
	return Grouped(N);
}

std::string KastClock(double Minutes)
{
	const long long Secs = std::max(0LL, static_cast<long long>(std::floor(Minutes * 60.0)));
	return std::to_string(Secs / 3600) + ":" + NetPad2((Secs / 60) % 60) + ":" + NetPad2(Secs % 60);
}

/** Kast's logo: a violet tile with a broadcast wedge. */
void KastLogo(Canvas& Cv, float X, float Y, float Sz)
{
	Cv.FillRoundRect({X, Y, Sz, Sz}, Sz * 0.26f, Paint::Linear({X, Y}, {X + Sz, Y + Sz}, Hex(0xb07cff), Hex(0x6d28d9)));
	Cv.FillPolygon({{X + Sz * 0.32f, Y + Sz * 0.26f}, {X + Sz * 0.74f, Y + Sz * 0.5f}, {X + Sz * 0.32f, Y + Sz * 0.74f}}, KLime);
	Cv.StrokeArc(X + Sz * 0.32f, Y + Sz * 0.5f, Sz * 0.5f, -0.6f, 0.6f, NetA(Hex(0xffffff), 0.5f), Sz * 0.06f, true);
}

Color AlertColor(kast::AlertKind K)
{
	switch (K)
	{
	case kast::AlertKind::Follow: return KViolet;
	case kast::AlertKind::Sub:
	case kast::AlertKind::Gift: return KLime;
	case kast::AlertKind::Tip: return KGold;
	case kast::AlertKind::Cheer: return Hex(0xf472b6);
	case kast::AlertKind::Raid: return KRed;
	case kast::AlertKind::Clip: return Hex(0x38bdf8);
	default: return KLime;
	}
}

std::string AlertTitle(const kast::Alert& A)
{
	switch (A.Kind)
	{
	case kast::AlertKind::Follow: return A.Count > 1 ? std::to_string(A.Count) + " NEW FOLLOWERS" : "NEW FOLLOWER";
	case kast::AlertKind::Sub: return "NEW SUBSCRIBER";
	case kast::AlertKind::Gift: return std::to_string(A.Count) + " GIFTED SUB" + (A.Count == 1 ? "" : "S");
	case kast::AlertKind::Tip: return Money(A.Cents) + " TIP";
	case kast::AlertKind::Cheer: return "CHEER " + Grouped(A.Count);
	case kast::AlertKind::Raid: return "RAID!";
	case kast::AlertKind::Milestone: return "MILESTONE";
	case kast::AlertKind::Sponsor: return "SPONSOR";
	case kast::AlertKind::Clip: return "YOUR CLIP IS TAKING OFF";
	case kast::AlertKind::Affiliate: return "KAST AFFILIATE!";
	case kast::AlertKind::Partner: return "KAST PARTNER!";
	}
	return "";
}

std::string AlertLine(const kast::Alert& A)
{
	switch (A.Kind)
	{
	case kast::AlertKind::Follow: return A.Count > 1 ? "welcome in!" : A.Who;
	case kast::AlertKind::Sub: return A.Who + (A.Text.empty() ? "" : ": " + A.Text);
	case kast::AlertKind::Gift: return A.Who + " gifted to the community";
	case kast::AlertKind::Tip:
	case kast::AlertKind::Cheer: return A.Who + ": " + A.Text;
	case kast::AlertKind::Raid: return A.Who + " is raiding with " + Grouped(A.Count) + " viewers";
	case kast::AlertKind::Clip: return A.Text;
	default: return A.Text;
	}
}

/** A little spark line of the viewer graph. */
void KastSpark(Canvas& Cv, const Rect& R, const std::vector<float>& G, const Color& Col)
{
	if (G.size() < 2)
	{
		return;
	}
	const size_t N = std::min<size_t>(G.size(), 90);
	float Top = 1.0f;
	for (size_t I = G.size() - N; I < G.size(); ++I)
	{
		Top = std::max(Top, G[I]);
	}
	std::vector<Vec2> P;
	for (size_t I = 0; I < N; ++I)
	{
		const float V = G[G.size() - N + I];
		P.push_back({R.X + R.W * Nf(I) / Nf(N - 1), R.Y + R.H - R.H * V / Top});
	}
	std::vector<Vec2> Fill = P;
	Fill.push_back({R.X + R.W, R.Y + R.H});
	Fill.push_back({R.X, R.Y + R.H});
	Cv.FillPolygon(Fill, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, NetA(Col, 0.35f), NetA(Col, 0.0f)));
	Cv.StrokePolyline(P, false, Col, 2.0f, true);
}

/** Badges in front of a chatter's name; returns the width used. */
float KastBadges(Canvas& Cv, int Badges, float X, float Cy, float Sz)
{
	float W = 0.0f;
	auto Badge = [&](const Color& Col, int Kind) {
		const Rect R{X + W, Cy - Sz * 0.5f, Sz, Sz};
		Cv.FillRoundRect(R, Sz * 0.22f, Col);
		const float Mx = R.X + Sz * 0.5f;
		const float My = R.Y + Sz * 0.5f;
		switch (Kind)
		{
		case 0: NetStar(Cv, Mx, My, Sz * 0.36f, Hex(0x1b1036)); break;                                                                   // sub
		case 1: Cv.FillRect({Mx - Sz * 0.08f, My - Sz * 0.3f, Sz * 0.16f, Sz * 0.5f}, Hex(0xffffff)); Cv.FillRect({Mx - Sz * 0.28f, My - Sz * 0.3f, Sz * 0.56f, Sz * 0.16f}, Hex(0xffffff)); break; // mod: a sword-ish hammer
		case 2: Cv.FillPolygon({{Mx, My - Sz * 0.32f}, {Mx + Sz * 0.3f, My}, {Mx, My + Sz * 0.32f}, {Mx - Sz * 0.3f, My}}, Hex(0xffffff)); break;         // vip
		case 3: Cv.FillRoundRect({Mx - Sz * 0.28f, My - Sz * 0.2f, Sz * 0.56f, Sz * 0.42f}, Sz * 0.1f, Hex(0xffffff)); break;                              // bot
		case 4: Cv.FillPolygon({{Mx - Sz * 0.22f, My - Sz * 0.28f}, {Mx + Sz * 0.3f, My}, {Mx - Sz * 0.22f, My + Sz * 0.28f}}, Hex(0xffffff)); break;      // broadcaster
		case 5: NetCheck(Cv, Mx, My, Sz * 0.7f, Hex(0xffffff)); break;                                                                                   // verified
		default: Cv.FillCircle(Mx, My, Sz * 0.25f, Hex(0xffffff)); break;                                                                                  // gifter
		}
		W += Sz + 3.0f;
	};
	if (Badges & kast::BadgeStreamer)
	{
		Badge(KRed, 4);
	}
	if (Badges & kast::BadgeVerified)
	{
		Badge(KViolet, 5);
	}
	if (Badges & kast::BadgeMod)
	{
		Badge(Hex(0x16a34a), 1);
	}
	if (Badges & kast::BadgeVip)
	{
		Badge(Hex(0xe11d84), 2);
	}
	if (Badges & kast::BadgeSub)
	{
		Badge(KLime, 0);
	}
	if (Badges & kast::BadgeGifter)
	{
		Badge(Hex(0xf59e0b), 6);
	}
	return W;
}

/** Splits on spaces (keeping the spaces with the words). */
std::vector<std::string> KastWords(const std::string& Text)
{
	std::vector<std::string> W;
	std::string Cur;
	for (const char Ch : Text)
	{
		Cur.push_back(Ch);
		if (Ch == ' ')
		{
			W.push_back(Cur);
			Cur.clear();
		}
	}
	if (!Cur.empty())
	{
		W.push_back(Cur);
	}
	return W;
}

std::string KastTrim(const std::string& W)
{
	return !W.empty() && W.back() == ' ' ? W.substr(0, W.size() - 1) : W;
}
} // namespace rlkast_detail

using namespace rlkast_detail;

// ------------------------------------------------------------------ text with emotes

float RiverLine::EmoteText(const std::string& Text, float X, float Y, float Size, int Weight, const Color& Col, float MaxW, bool Draw)
{
	const std::string Prefix = kast::EmotePrefix(S.HeroName);
	float Cx = X;
	for (const std::string& W : KastWords(Text))
	{
		const std::string Bare = KastTrim(W);
		const int E = kast::EmoteOf(Bare, Prefix);
		const float Ww = E >= 0 ? Size * 1.35f + (W.size() > Bare.size() ? UI.Measure(" ", Size, Weight) : 0.0f) : UI.Measure(W, Size, Weight);
		if (MaxW > 0.0f && Cx + Ww - X > MaxW)
		{
			break;
		}
		if (Draw)
		{
			if (E >= 0)
			{
				streamart::Emote(*C, E, Cx, Y - Size * 1.05f, Size * 1.3f);
			}
			else
			{
				UI.Text(W, Cx, Y, Ts(Size, Weight, Col));
			}
		}
		Cx += Ww;
	}
	return Cx - X;
}

void RiverLine::StreamChatLine(const kast::ChatMsg& M, float X, float Y, float W, float Size, bool Interactive, double Now, float& LineHeight, bool Measure)
{
	const float LineH = Size * 1.5f;
	const std::string Prefix = kast::EmotePrefix(S.HeroName);
	// System lines: one muted paragraph.
	if (M.Kind == kast::LineKind::System || M.Kind == kast::LineKind::Raid)
	{
		const std::vector<std::string> Lines = NetWrap(*C, M.Text, W - 16.0f, Size - 1.0f, 600);
		LineHeight = Nf(Lines.size()) * LineH + 6.0f;
		if (!Measure)
		{
			const Color Col = M.Kind == kast::LineKind::Raid ? KRed : NetA(Hex(M.NameColor), 0.95f);
			if (M.Kind == kast::LineKind::Raid)
			{
				C->FillRoundRect({X, Y, W, LineHeight}, 6.0f, NetA(KRed, 0.12f));
			}
			for (size_t I = 0; I < Lines.size(); ++I)
			{
				UI.Text(Lines[I], X + 8.0f, Y + LineH * (Nf(I) + 0.75f), Ts(Size - 1.0f, 600, M.Kind == kast::LineKind::Raid ? Col : KMuted));
			}
		}
		return;
	}
	const bool Boxed = M.Kind == kast::LineKind::Tip || M.Kind == kast::LineKind::Sub || M.Kind == kast::LineKind::Cheer || M.Kind == kast::LineKind::Question;
	const float Pad = Boxed ? 8.0f : 4.0f;
	const float Inner = W - Pad * 2.0f - (Interactive ? 0.0f : 0.0f);
	// Lay out: badges, name, then the words.
	struct Piece
	{
		std::string Word;
		int Emote = -1;
		float X = 0.0f;
		int Row = 0;
		float W = 0.0f;
	};
	std::vector<Piece> Pieces;
	int BadgeCount = 0;
	for (int Bit = M.Badges; Bit != 0; Bit &= Bit - 1)
	{
		++BadgeCount;
	}
	const float BadgeW = Nf(BadgeCount) * (Size + 3.0f);
	const std::string Name = M.Who + (M.Kind == kast::LineKind::Streamer || M.Kind == kast::LineKind::Chat || M.Kind == kast::LineKind::Question || M.Kind == kast::LineKind::Tip ? ":" : "");
	const float NameW = UI.Measure(Name + " ", Size, 800);
	float Cx = BadgeW + NameW;
	int Row = 0;
	const std::string Body = M.Deleted ? std::string("<message deleted>") : M.Text;
	for (const std::string& Wd : KastWords(Body))
	{
		Piece P;
		P.Word = Wd;
		P.Emote = M.Deleted ? -1 : kast::EmoteOf(KastTrim(Wd), Prefix);
		P.W = P.Emote >= 0 ? Size * 1.35f + (Wd.size() > KastTrim(Wd).size() ? UI.Measure(" ", Size, 500) : 0.0f) : UI.Measure(Wd, Size, 500);
		if (Cx + P.W > Inner && Cx > 0.0f)
		{
			++Row;
			Cx = 0.0f;
		}
		P.X = Cx;
		P.Row = Row;
		Cx += P.W;
		Pieces.push_back(P);
	}
	LineHeight = Nf(Row + 1) * LineH + Pad * 2.0f - 2.0f;
	if (Boxed)
	{
		LineHeight += M.Kind == kast::LineKind::Question && Interactive && !M.Answered ? 0.0f : 0.0f;
	}
	if (Measure)
	{
		return;
	}
	const Rect Box{X, Y, W, LineHeight};
	Color BoxCol{0.0f, 0.0f, 0.0f, 0.0f};
	switch (M.Kind)
	{
	case kast::LineKind::Tip: BoxCol = NetA(KGold, 0.14f); break;
	case kast::LineKind::Sub: BoxCol = NetA(KLime, 0.12f); break;
	case kast::LineKind::Cheer: BoxCol = NetA(Hex(0xf472b6), 0.13f); break;
	case kast::LineKind::Question: BoxCol = M.Answered ? NetA(KViolet, 0.08f) : NetA(KViolet, 0.18f); break;
	case kast::LineKind::Streamer: BoxCol = NetA(KLime, 0.06f); break;
	default: break;
	}
	const bool Hover = Interactive && UI.Hover(Box);
	if (BoxCol.A > 0.0f)
	{
		C->FillRoundRect(Box, 6.0f, BoxCol);
		if (M.Kind != kast::LineKind::Streamer)
		{
			C->FillRoundRect({Box.X, Box.Y, 3.0f, Box.H}, 1.5f, NetA(M.Kind == kast::LineKind::Tip ? KGold : M.Kind == kast::LineKind::Sub ? KLime : M.Kind == kast::LineKind::Cheer ? Hex(0xf472b6) : KViolet, 0.9f));
		}
	}
	else if (Hover)
	{
		C->FillRoundRect(Box, 6.0f, Rgba(255, 255, 255, 0.04f));
	}
	const float Base = Y + Pad + Size * 1.05f;
	float Bx = X + Pad;
	if (M.Badges)
	{
		Bx += KastBadges(*C, M.Badges, Bx, Base - Size * 0.36f, Size);
	}
	const Color NameCol = M.Kind == kast::LineKind::Streamer ? KLime : Hex(M.NameColor);
	UI.Text(Name, Bx, Base, Ts(Size, 800, M.Deleted ? KDim : NameCol));
	const float BodyX = X + Pad;
	const Color TextCol = M.Deleted ? KDim : M.Kind == kast::LineKind::Tip ? Hex(0xfde68a) : KInk;
	for (const Piece& P : Pieces)
	{
		const float Px = BodyX + P.X + (P.Row == 0 ? 0.0f : 0.0f);
		const float Py = Base + Nf(P.Row) * LineH;
		if (P.Emote >= 0)
		{
			streamart::Emote(*C, P.Emote, Px, Py - Size * 1.05f, Size * 1.3f);
		}
		else
		{
			UI.Text(P.Word, Px, Py, Ts(Size, M.Deleted ? 400 : 500, TextCol));
		}
	}
	(void)BadgeW;
	// Moderation and answers, on hover.
	if (Interactive && S.Streaming() && !M.Deleted)
	{
		if (M.Kind == kast::LineKind::Question && !M.Answered)
		{
			const Rect Btn{Box.X + Box.W - 74.0f, Box.Y + Box.H - 28.0f, 66.0f, 22.0f};
			const Ui::ClickState St = UI.Clickable("kastans" + std::to_string(M.Id), Btn);
			C->FillRoundRect(Btn, 11.0f, St.Hover ? KLime : KViolet);
			UI.Text("Answer", Btn.X + Btn.W / 2.0f, Btn.Y + 15.5f, Ts(12.0f, 800, St.Hover ? Hex(0x1b1036) : Hex(0xffffff), Align::Center));
			if (St.Clicked)
			{
				S.StreamAnswer(M.Id);
			}
		}
		else if (Hover && M.Kind == kast::LineKind::Chat && !M.Who.empty())
		{
			const Rect Btn{Box.X + Box.W - 76.0f, Box.Y + 3.0f, 70.0f, 22.0f};
			const Ui::ClickState St = UI.Clickable("kastto" + std::to_string(M.Id), Btn);
			C->FillRoundRect(Btn, 11.0f, St.Hover ? KRed : Hex(0x3a2f55));
			UI.Text("Timeout", Btn.X + Btn.W / 2.0f, Btn.Y + 15.5f, Ts(12.0f, 800, Hex(0xffffff), Align::Center));
			if (St.Clicked)
			{
				S.StreamTimeout(M.Id);
			}
		}
	}
	(void)Now;
}

void RiverLine::StreamChat(const Rect& R, double Now, bool Interactive)
{
	const kast::Stream& St = S.Stream;
	C->FillRoundRect(R, 14.0f, KPanel);
	C->StrokeRoundRect(R, 14.0f, KLine, 1.0f);
	// Tabs.
	const char* Names[3] = {"Chat", "Mods", "Activity"};
	const std::string Counts[3] = {St.Live ? KastCount(St.Chatters()) : std::string(), std::to_string(St.ModsOnline()) + "/" + std::to_string(S.Channel.Mods.size()), St.Live ? std::to_string(St.Feed.size()) : std::string()};
	float Tx = R.X + 16.0f;
	for (int K = 0; K < 3; ++K)
	{
		const float W = UI.Measure(Names[K], 14.0f, 800) + (Counts[K].empty() ? 0.0f : UI.Measure(Counts[K], 11.0f, 700) + 8.0f) + 20.0f;
		const Rect T{Tx, R.Y + 10.0f, W, 32.0f};
		const Ui::ClickState Cs = UI.Clickable(std::string("kastchattab") + Names[K], T, Interactive);
		if (Cs.Clicked)
		{
			ChatTab = K;
		}
		const bool On = ChatTab == K;
		if (On)
		{
			C->FillRoundRect(T, 9.0f, KPanel2);
		}
		UI.Text(Names[K], T.X + 10.0f, T.Y + 21.0f, Ts(14.0f, 800, On ? KInk : KMuted));
		if (!Counts[K].empty())
		{
			UI.Text(Counts[K], T.X + T.W - 10.0f, T.Y + 21.0f, Ts(11.0f, 700, On ? KLime : KDim, Align::Right));
		}
		Tx += W + 4.0f;
	}
	const Rect Body{R.X + 8.0f, R.Y + 52.0f, R.W - 16.0f, R.H - 52.0f - (ChatTab == 0 ? 56.0f : 8.0f)};
	C->FillRect({R.X, R.Y + 48.0f, R.W, 1.0f}, KLine);
	if (ChatTab == 0)
	{
		C->PushClip(Body);
		if (St.Chat.empty())
		{
			UI.Text(St.Live ? "Chat is quiet. Say something." : "Chat opens when you go live.", Body.X + Body.W / 2.0f, Body.Y + Body.H / 2.0f, Ts(14.0f, 600, KDim, Align::Center));
		}
		// Newest at the bottom, laid out upward.
		float Y = Body.Y + Body.H - 4.0f;
		const float Size = Body.W < 380.0f ? 13.0f : 14.0f;
		for (size_t I = St.Chat.size(); I-- > 0 && Y > Body.Y;)
		{
			const kast::ChatMsg& M = St.Chat[I];
			if (M.At > Now)
			{
				continue;
			}
			float H = 0.0f;
			StreamChatLine(M, Body.X, 0.0f, Body.W, Size, Interactive, Now, H, true);
			Y -= H + 2.0f;
			const float Fresh = NetEase((Now - M.At) / 0.25);
			const float A0 = C->GetAlpha();
			C->SetAlpha(A0 * (0.4f + 0.6f * Fresh));
			StreamChatLine(M, Body.X, Y, Body.W, Size, Interactive, Now, H, false);
			C->SetAlpha(A0);
		}
		C->PopClip();
		// The message bar.
		const Rect Bar{R.X + 12.0f, R.Y + R.H - 48.0f, R.W - 24.0f, 38.0f};
		C->FillRoundRect(Bar, 10.0f, KPanel2);
		C->StrokeRoundRect(Bar, 10.0f, KLine, 1.0f);
		UI.Text(St.Live ? "Chatting as " + S.HeroName : "Offline", Bar.X + 14.0f, Bar.Y + 24.0f, Ts(13.0f, 600, KDim, Align::Left, Baseline::Alphabetic, false, Bar.W - 140.0f));
		const Rect Thx{Bar.X + Bar.W - 112.0f, Bar.Y + 5.0f, 106.0f, 28.0f};
		const bool Can = St.Live && Now - St.ThankAt >= 20.0;
		const Ui::ClickState Cs = UI.Clickable("kastthank", Thx, Interactive && Can);
		C->FillRoundRect(Thx, 8.0f, Can ? (Cs.Hover ? KLime : KViolet) : NetA(KViolet, 0.25f));
		streamart::Emote(*C, static_cast<int>(kast::Emote::Love), Thx.X + 8.0f, Thx.Y + 5.0f, 18.0f);
		UI.Text("Talk to chat", Thx.X + 30.0f, Thx.Y + 19.0f, Ts(12.0f, 800, Can && Cs.Hover ? Hex(0x1b1036) : Hex(0xffffff)));
		if (Cs.Clicked)
		{
			S.StreamThank();
		}
		return;
	}
	if (ChatTab == 1)
	{
		float Y = Body.Y + 26.0f;
		UI.Text("Moderators " + std::to_string(S.Channel.Mods.size()) + "/" + std::to_string(kast::Stream::MaxMods), Body.X + 8.0f, Y, Ts(15.0f, 800, KInk));
		Y += 14.0f;
		if (S.Channel.Mods.empty())
		{
			Y = NetParagraph(*C, "No mods yet. Regulars who behave show up below: make a few of them mods and they'll clean up trolls and spam while you play.", Body.X + 8.0f, Y + 16.0f, Body.W - 16.0f, 13.0f, 500, KMuted, 18.0f, 4) + 4.0f;
		}
		for (const kast::Moderator& M : S.Channel.Mods)
		{
			const bool On = std::find(St.ModsHere().begin(), St.ModsHere().end(), M.Name) != St.ModsHere().end();
			const Rect Row{Body.X, Y + 6.0f, Body.W, 44.0f};
			C->FillRoundRect(Row, 8.0f, KPanel2);
			C->FillCircle(Row.X + 18.0f, Row.Y + 22.0f, 5.0f, On && St.Live ? Hex(0x22c55e) : KDim);
			KastBadges(*C, kast::BadgeMod, Row.X + 32.0f, Row.Y + 22.0f, 14.0f);
			UI.Text(M.Name, Row.X + 52.0f, Row.Y + 20.0f, Ts(14.0f, 800, Hex(kast::Violet)));
			UI.Text(std::to_string(M.Actions) + " timeouts \xC2\xB7 " + (On && St.Live ? "watching now" : "offline"), Row.X + 52.0f, Row.Y + 36.0f, Ts(11.5f, 600, KMuted));
			const Rect Btn{Row.X + Row.W - 78.0f, Row.Y + 11.0f, 70.0f, 22.0f};
			const Ui::ClickState Cs = UI.Clickable("kastdemote" + M.Name, Btn, Interactive);
			C->FillRoundRect(Btn, 11.0f, Cs.Hover ? KRed : Hex(0x3a2f55));
			UI.Text("Remove", Btn.X + Btn.W / 2.0f, Btn.Y + 15.5f, Ts(11.5f, 800, Hex(0xffffff), Align::Center));
			if (Cs.Clicked)
			{
				S.StreamDemote(M.Name);
				break;
			}
			Y += 50.0f;
		}
		Y += 24.0f;
		UI.Text("Regulars", Body.X + 8.0f, Y, Ts(15.0f, 800, KInk));
		Y += 6.0f;
		const std::vector<std::string> Cand = St.ModCandidates(S.Channel, World);
		if (Cand.empty())
		{
			UI.Text("Nobody's chatted enough yet.", Body.X + 8.0f, Y + 22.0f, Ts(13.0f, 500, KMuted));
			Y += 30.0f;
		}
		for (const std::string& Name : Cand)
		{
			const Rect Row{Body.X, Y + 6.0f, Body.W, 36.0f};
			const auto It = S.Channel.Regulars.find(Name);
			UI.Text(Name, Row.X + 10.0f, Row.Y + 23.0f, Ts(14.0f, 800, Hex(0xd8ccff), Align::Left, Baseline::Alphabetic, false, Row.W - 200.0f));
			UI.Text(std::to_string(It != S.Channel.Regulars.end() ? It->second : 0) + " messages" + (S.Channel.IsSub(Name, World) ? " \xC2\xB7 sub" : ""), Row.X + Row.W - 96.0f, Row.Y + 23.0f, Ts(11.5f, 600, KMuted, Align::Right));
			const Rect Btn{Row.X + Row.W - 86.0f, Row.Y + 7.0f, 80.0f, 22.0f};
			const bool Full = static_cast<int>(S.Channel.Mods.size()) >= kast::Stream::MaxMods;
			const Ui::ClickState Cs = UI.Clickable("kastmod" + Name, Btn, Interactive && !Full);
			C->FillRoundRect(Btn, 11.0f, Full ? Hex(0x2b2340) : Cs.Hover ? KLime : Hex(0x16a34a));
			UI.Text("Make mod", Btn.X + Btn.W / 2.0f, Btn.Y + 15.5f, Ts(11.5f, 800, Cs.Hover && !Full ? Hex(0x1b1036) : Hex(0xffffff), Align::Center));
			if (Cs.Clicked)
			{
				S.StreamPromote(Name);
				break;
			}
			Y += 40.0f;
		}
		Y += 26.0f;
		const gear::Effects& Fx = S.GearFx();
		const Rect Bot{Body.X, Y, Body.W, 64.0f};
		C->FillRoundRect(Bot, 10.0f, NetA(Hex(0x7c3aed), 0.14f));
		streamart::Product(*C, gear::Art::ModBot, {Bot.X + 6.0f, Bot.Y + 6.0f, 52.0f, 52.0f}, 0x7c3aed, Now);
		UI.Text(Fx.ModBot > 0.0 ? "WardenBot is on" : "No chat filter", Bot.X + 66.0f, Bot.Y + 26.0f, Ts(14.0f, 800, KInk));
		UI.Text(Fx.ModBot > 0.0 ? "Catches " + std::to_string(static_cast<int>(Fx.ModBot * 100.0)) + "% of trolls and spam instantly" : "GearDrop sells one for $7.99/mo", Bot.X + 66.0f, Bot.Y + 46.0f, Ts(12.0f, 600, KMuted));
		Y += 84.0f;
		if (St.Live)
		{
			UI.Text("Tonight: " + std::to_string(St.Caught) + " caught \xC2\xB7 " + std::to_string(St.Missed) + " slipped through", Body.X + 8.0f, Y, Ts(13.0f, 700, St.Missed > St.Caught ? KRed : KMuted));
		}
		return;
	}
	// Activity.
	float Y = Body.Y + 8.0f;
	if (St.Feed.empty())
	{
		UI.Text(St.Live ? "Follows, subs, tips and raids show up here." : "Go live to see activity.", Body.X + Body.W / 2.0f, Body.Y + 40.0f, Ts(13.0f, 600, KDim, Align::Center));
	}
	for (const kast::Alert& A : St.Feed)
	{
		if (Y > Body.Y + Body.H - 50.0f)
		{
			break;
		}
		const Color Col = AlertColor(A.Kind);
		const Rect Row{Body.X, Y, Body.W, 46.0f};
		C->FillRoundRect(Row, 8.0f, KPanel2);
		C->FillRoundRect({Row.X, Row.Y, 4.0f, Row.H}, 2.0f, Col);
		UI.Text(AlertTitle(A), Row.X + 14.0f, Row.Y + 19.0f, Ts(11.5f, 900, Col));
		UI.Text(AlertLine(A), Row.X + 14.0f, Row.Y + 37.0f, Ts(13.0f, 600, KInk, Align::Left, Baseline::Alphabetic, false, Row.W - 90.0f));
		UI.Text(NetHms(std::max(0.0, (Now - A.At) / 60.0)) + " ago", Row.X + Row.W - 10.0f, Row.Y + 19.0f, Ts(11.0f, 600, KDim, Align::Right));
		Y += 52.0f;
	}
}

// ------------------------------------------------------------------ alerts and the overlay

void RiverLine::StreamAlert(float Cx, float Y, float Scale, double Now)
{
	const kast::Stream& St = S.Stream;
	if (St.Alerts.empty() || St.Alerts.front().At < 0.0)
	{
		return;
	}
	const kast::Alert& A = St.Alerts.front();
	const double Age = Now - A.At;
	if (Age < 0.0 || Age > 4.8)
	{
		return;
	}
	const float In = NetEase(Age / 0.35);
	const float Out = 1.0f - NetEase((Age - 4.2) / 0.4);
	const float Pop = 1.0f + 0.08f * Nf(std::max(0.0, 1.0 - Age / 0.3));
	const Color Col = AlertColor(A.Kind);
	const std::string Title = AlertTitle(A);
	const std::string Body = AlertLine(A);
	const float W = std::max(UI.Measure(Title, 22.0f, 900), UI.Measure(Body, 16.0f, 700)) + 120.0f;
	const float H = 84.0f;
	C->Save();
	C->Translate(Cx, Y + (1.0f - In) * -30.0f * Scale);
	C->Scale(Scale * Pop, Scale * Pop);
	const float A0 = C->GetAlpha();
	C->SetAlpha(A0 * In * Out);
	const Rect R{-W / 2.0f, 0.0f, std::min(W, 760.0f), H};
	const Rect Rr{-R.W / 2.0f, 0.0f, R.W, H};
	C->GlowRoundRect(Rr, 18.0f, NetA(Col, 0.55f), 22.0f);
	C->FillRoundRect(Rr, 18.0f, Paint::Linear({Rr.X, 0.0f}, {Rr.X + Rr.W, 0.0f}, Hex(0x1b1036), Hex(0x2a1652)));
	C->StrokeRoundRect(Rr, 18.0f, Col, 2.5f);
	// The icon: the alert's emote.
	int E = static_cast<int>(kast::Emote::Hype);
	switch (A.Kind)
	{
	case kast::AlertKind::Follow: E = static_cast<int>(kast::Emote::Love); break;
	case kast::AlertKind::Sub:
	case kast::AlertKind::Gift: E = static_cast<int>(kast::Emote::Ship); break;
	case kast::AlertKind::Tip:
	case kast::AlertKind::Cheer: E = static_cast<int>(kast::Emote::Chip); break;
	case kast::AlertKind::Raid: E = static_cast<int>(kast::Emote::Pog); break;
	case kast::AlertKind::Milestone: E = static_cast<int>(kast::Emote::Clap); break;
	default: break;
	}
	const float Bob = Nf(std::sin(Age * 6.0)) * 3.0f;
	streamart::Emote(*C, E, Rr.X + 16.0f, 12.0f + Bob, 60.0f);
	UI.Text(Title, Rr.X + 90.0f, 36.0f, Ts(22.0f, 900, Col, Align::Left, Baseline::Alphabetic, false, Rr.W - 104.0f));
	EmoteText(Body, Rr.X + 90.0f, 62.0f, 16.0f, 700, KInk, Rr.W - 104.0f, true);
	// Confetti for the big ones.
	if (A.Kind == kast::AlertKind::Raid || A.Kind == kast::AlertKind::Milestone || A.Kind == kast::AlertKind::Gift || A.Kind == kast::AlertKind::Partner || A.Kind == kast::AlertKind::Affiliate)
	{
		for (int K = 0; K < 24; ++K)
		{
			const float Px = Rr.X + Rr.W * Nf((K * 37) % 100) / 100.0f;
			const float Py = -20.0f + Nf(std::fmod(Age * (60.0 + K * 7.0) + K * 13.0, 140.0));
			C->FillRect({Px, Py, 6.0f, 3.0f}, K % 3 == 0 ? KLime : K % 3 == 1 ? KViolet : KGold);
		}
	}
	C->SetAlpha(A0);
	C->Restore();
}

void RiverLine::StreamOverlay(const Rect& R, float Scale, double Now)
{
	const kast::Stream& St = S.Stream;
	const gear::Effects& Fx = S.GearFx();
	// The facecam.
	streamart::Cam Look;
	Look.Gear = Fx;
	Look.Headphones = S.Owns("headphones");
	Look.Face = St.Face;
	Look.FaceAge = Now - St.FaceAt;
	Look.Talking = Now - St.ThankAt < 3.0;
	Look.Time = Now;
	Look.Live = St.Live;
	const float CamW = (Fx.GreenScreen ? 330.0f : 300.0f) * Scale;
	const float CamH = CamW * 9.0f / 16.0f * (Fx.GreenScreen ? 1.15f : 1.0f);
	const Rect Cam{R.X + R.W - CamW - 14.0f * Scale, R.Y + R.H - CamH - 14.0f * Scale, CamW, CamH};
	if (!Fx.GreenScreen)
	{
		C->GlowRoundRect(Cam, 8.0f * Scale, Rgba(0, 0, 0, 0.5f), 14.0f * Scale);
	}
	streamart::Facecam(*C, Cam, Look);
	if (Fx.Overlay && !Fx.GreenScreen)
	{
		C->StrokeRoundRect(Cam, 6.0f * Scale, KViolet, 3.0f * Scale);
		C->FillRoundRect({Cam.X, Cam.Y - 24.0f * Scale, 140.0f * Scale, 22.0f * Scale}, 6.0f * Scale, KViolet);
		UI.Text(S.HeroName, Cam.X + 10.0f * Scale, Cam.Y - 8.0f * Scale, Ts(13.0f * Scale, 900, Hex(0xffffff), Align::Left, Baseline::Alphabetic, false, 124.0f * Scale));
	}
	// The overlay pack: a rent goal and the latest supporters along the bottom.
	if (Fx.Overlay)
	{
		const Rect Goal{R.X + 14.0f * Scale, R.Y + R.H - 46.0f * Scale, 360.0f * Scale, 32.0f * Scale};
		C->FillRoundRect(Goal, 10.0f * Scale, Rgba(15, 10, 25, 0.82f));
		const Chips Due = S.Life.RentStage == life::Rent::Paid ? 0 : S.Life.RentDueCents;
		const double Have = Due > 0 ? std::min(1.0, static_cast<double>(S.BankrollCents + S.Channel.UnpaidCents) / static_cast<double>(Due)) : 1.0;
		C->FillRoundRect({Goal.X + 4.0f * Scale, Goal.Y + 4.0f * Scale, std::max(8.0f * Scale, (Goal.W - 8.0f * Scale) * Nf(Have)), Goal.H - 8.0f * Scale}, 7.0f * Scale,
			Paint::Linear({Goal.X, 0.0f}, {Goal.X + Goal.W, 0.0f}, KViolet, KLime));
		UI.Text(Due > 0 ? "RENT GOAL  " + NetMoney(S.BankrollCents + S.Channel.UnpaidCents) + " / " + NetMoney(Due) : std::string("RENT PAID THIS MONTH"), Goal.X + 14.0f * Scale, Goal.Y + 21.0f * Scale,
			Ts(13.0f * Scale, 900, Hex(0xffffff)));
		std::string Latest;
		for (const kast::Alert& A : St.Feed)
		{
			if (A.Kind == kast::AlertKind::Follow && A.Count == 1 && !A.Who.empty())
			{
				Latest = "Latest follower: " + A.Who;
				break;
			}
		}
		if (!Latest.empty())
		{
			const float Lw = UI.Measure(Latest, 13.0f * Scale, 700) + 24.0f * Scale;
			C->FillRoundRect({Goal.X + Goal.W + 8.0f * Scale, Goal.Y, Lw, Goal.H}, 10.0f * Scale, Rgba(15, 10, 25, 0.82f));
			UI.Text(Latest, Goal.X + Goal.W + 20.0f * Scale, Goal.Y + 21.0f * Scale, Ts(13.0f * Scale, 700, KInk));
		}
	}
	// Sponsors in the corner.
	float Sx = R.X + R.W - 14.0f * Scale;
	for (const kast::Deal& D : S.Channel.Deals)
	{
		const kast::Sponsor* Sp = D.Until > World ? kast::FindSponsor(D.Id) : nullptr;
		if (!Sp)
		{
			continue;
		}
		const float W = UI.Measure(Sp->Brand, 13.0f * Scale, 900) + 24.0f * Scale;
		Sx -= W;
		C->FillRoundRect({Sx, R.Y + 46.0f * Scale, W, 26.0f * Scale}, 8.0f * Scale, NetA(Hex(Sp->Color), 0.9f));
		UI.Text(Sp->Brand, Sx + W / 2.0f, R.Y + 64.0f * Scale, Ts(13.0f * Scale, 900, Hex(0x0b0716), Align::Center));
		Sx -= 8.0f * Scale;
	}
	// Ad break.
	if (St.AdRunning(Now))
	{
		C->FillRect(R, Rgba(10, 6, 20, 0.72f));
		UI.Text("AD BREAK", R.X + R.W / 2.0f, R.Y + R.H / 2.0f - 6.0f * Scale, Ts(40.0f * Scale, 900, KInk, Align::Center));
		UI.Text("back in " + std::to_string(static_cast<int>(std::ceil(St.AdUntil - Now))) + "s \xC2\xB7 thanks for supporting the channel", R.X + R.W / 2.0f, R.Y + R.H / 2.0f + 30.0f * Scale,
			Ts(16.0f * Scale, 600, KMuted, Align::Center));
	}
	StreamAlert(R.X + R.W / 2.0f, R.Y + 20.0f * Scale, Scale, Now);
}

void RiverLine::StreamPreview(const Rect& R, double Now)
{
	C->FillRect(R, Hex(0x05030a));
	C->PushClip(R);
	// The capture: RiverLine as it is right now, scaled into the frame (the very top and bottom cropped).
	const float Scale = R.W / NetW;
	const float Crop = std::max(0.0f, (1000.0f * Scale - R.H) / Scale);
	const Pointer Real = UI.Ptr;
	const bool Cursor = UI.CursorIsPointer;
	UI.Ptr.Active = false;
	UI.Ptr.X = -1000.0f;
	UI.Ptr.Y = -1000.0f;
	UI.Ptr.Pressed = false;
	UI.Ptr.Released = false;
	UI.Ptr.Wheel = 0.0f;
	Previewing = true;
	C->Save();
	C->Translate(R.X, R.Y - std::min(Crop, 64.0f) * Scale);
	C->Scale(Scale, Scale);
	C->FillRect({0.0f, 0.0f, NetW, 1000.0f}, Paint::Linear({0.0f, 0.0f}, {0.0f, 1000.0f}, pal::Bg2, pal::Bg));
	TopBar(Now);
	switch (S.CurrentScreen)
	{
	case Screen::Boot: Boot(Now); break;
	case Screen::Lobby: LobbyPages(Now); break;
	case Screen::Table:
		if (S.Tiled && S.TableCount() >= 2)
		{
			Tiles(Now);
		}
		else
		{
			Table(Now);
		}
		break;
	case Screen::Results: ResultsScreen(Now); break;
	}
	C->Restore();
	Previewing = false;
	UI.Ptr = Real;
	UI.CursorIsPointer = Cursor;
	// Dropped frames: the image stutters when the machine can't keep up.
	const gear::Effects& Fx = S.GearFx();
	if (S.TableCount() > Fx.StreamTables && std::fmod(Now, 1.7) < 0.12)
	{
		C->FillRect(R, Rgba(0, 0, 0, 0.35f));
	}
	if (Fx.Resolution < 720)
	{
		// A soft, low-resolution picture.
		for (float Y = R.Y; Y < R.Y + R.H; Y += 4.0f)
		{
			C->FillRect({R.X, Y, R.W, 1.5f}, Rgba(0, 0, 0, 0.12f));
		}
	}
	StreamOverlay(R, R.W / NetW * 1.6f, Now);
	C->PopClip();
	// Frame and status.
	C->StrokeRoundRect(R, 4.0f, S.Streaming() ? NetA(KRed, 0.7f) : KLine, 2.0f);
	const std::string Tag = S.Streaming() ? "LIVE" : "PREVIEW";
	C->FillRoundRect({R.X + 12.0f, R.Y + 12.0f, UI.Measure(Tag, 12.0f, 900) + 30.0f, 24.0f}, 6.0f, S.Streaming() ? KRed : Hex(0x3a2f55));
	if (S.Streaming())
	{
		C->FillCircle(R.X + 24.0f, R.Y + 24.0f, 4.0f, NetA(Hex(0xffffff), 0.6f + 0.4f * Nf(std::sin(Now * 4.0))));
	}
	UI.Text(Tag, R.X + (S.Streaming() ? 32.0f : 24.0f), R.Y + 28.5f, Ts(12.0f, 900, Hex(0xffffff)));
	UI.Text(gear::ResolutionLabel(Fx.Resolution) + (S.TableCount() > Fx.StreamTables ? "  \xC2\xB7  DROPPING FRAMES" : ""), R.X + R.W - 12.0f, R.Y + R.H - 12.0f,
		Ts(11.5f, 800, S.TableCount() > Fx.StreamTables ? KRed : NetA(KInk, 0.6f), Align::Right, Baseline::Alphabetic, true));
}

// ------------------------------------------------------------------ the app

void RiverLine::KastHeader(double Now)
{
	C->FillRect({0.0f, 0.0f, NetW, 60.0f}, Hex(0x130d1f));
	C->FillRect({0.0f, 59.0f, NetW, 1.0f}, KLine);
	KastLogo(*C, 22.0f, 12.0f, 36.0f);
	UI.Text("Kast", 68.0f, 40.0f, Ts(26.0f, 900, KInk));
	const std::pair<KastPage, const char*> Tabs[3] = {{KastPage::Studio, "Studio"}, {KastPage::Channel, "Channel"}, {KastPage::Browse, "Browse"}};
	float X = 160.0f;
	for (const auto& T : Tabs)
	{
		const float W = UI.Measure(T.second, 16.0f, 800) + 28.0f;
		const Rect R{X, 10.0f, W, 40.0f};
		const Ui::ClickState St = UI.Clickable(std::string("kasttab") + T.second, R);
		if (St.Clicked)
		{
			KastShown = T.first;
			KastAt = Now;
			ChannelScrollGoal = ChannelScroll = 0.0f;
		}
		const bool On = KastShown == T.first;
		UI.Text(T.second, R.X + 14.0f, R.Y + 26.0f, Ts(16.0f, 800, On ? KInk : St.Hover ? Hex(0xd6ccf5) : KMuted));
		if (On)
		{
			C->FillRoundRect({R.X + 10.0f, 56.0f, W - 20.0f, 3.0f}, 1.5f, KLime);
		}
		X += W + 6.0f;
	}
	// Right: the channel at a glance.
	float Rx = NetW - 24.0f;
	const std::string Bal = "Balance " + Money(S.Channel.UnpaidCents);
	Rx -= UI.Text(Bal, Rx, 37.0f, Ts(15.0f, 800, S.Channel.UnpaidCents > 0 ? KLime : KMuted, Align::Right, Baseline::Alphabetic, true)) + 22.0f;
	const std::string Fol = KastCount(S.Channel.Followers) + " followers";
	Rx -= UI.Text(Fol, Rx, 37.0f, Ts(15.0f, 700, KInk, Align::Right)) + 22.0f;
	if (S.Streaming())
	{
		const std::string L = "LIVE  " + KastClock(S.Stream.Uptime(World));
		const float W = UI.Measure(L, 14.0f, 900, true) + 34.0f;
		C->FillRoundRect({Rx - W, 16.0f, W, 28.0f}, 8.0f, KRed);
		C->FillCircle(Rx - W + 14.0f, 30.0f, 4.0f, NetA(Hex(0xffffff), 0.6f + 0.4f * Nf(std::sin(Now * 4.0))));
		UI.Text(L, Rx - W + 24.0f, 35.0f, Ts(14.0f, 900, Hex(0xffffff), Align::Left, Baseline::Alphabetic, true));
	}
	else
	{
		const float W = UI.Measure("OFFLINE", 13.0f, 900) + 24.0f;
		C->FillRoundRect({Rx - W, 17.0f, W, 26.0f}, 8.0f, Hex(0x2b2340));
		UI.Text("OFFLINE", Rx - W / 2.0f, 34.5f, Ts(13.0f, 900, KMuted, Align::Center));
	}
}

void RiverLine::KastApp(double Now)
{
	C->FillRect({0.0f, 0.0f, NetW, KastH}, KBg);
	KastHeader(Now);
	switch (KastShown)
	{
	case KastPage::Studio: KastStudio(Now); break;
	case KastPage::Channel: KastChannel(Now); break;
	case KastPage::Browse: KastBrowse(Now); break;
	}
	if (S.StreamCard)
	{
		StreamSummary(Now);
	}
}

void RiverLine::KastStudio(double Now)
{
	const kast::Stream& St = S.Stream;
	const kast::Channel& Ch = S.Channel;
	const bool Live = St.Live;
	const Rect Prev{20.0f, 74.0f, 1008.0f, 567.0f};
	StreamPreview(Prev, Now);
	const bool CanStream = S.GearFx().CanStream();
	if (!CanStream)
	{
		// Locked: the laptop can't run the client and an encoder at once. The way in is the next PC upgrade.
		const gear::Item& Up = gear::FirstPcUpgrade();
		C->FillRect(Prev, Rgba(10, 6, 20, 0.86f));
		C->StrokeRoundRect(Prev, 4.0f, KLine, 2.0f);
		const float Cx = Prev.X + Prev.W / 2.0f;
		C->FillCircle(Cx, Prev.Y + 120.0f, 46.0f, NetA(KViolet, 0.18f));
		NetLockIcon(*C, Cx - 26.0f, Prev.Y + 90.0f, 52.0f, KInk);
		UI.Text("Your laptop can't stream", Cx, Prev.Y + 222.0f, Ts(32.0f, 900, KInk, Align::Center));
		UI.Text("RiverLine and a stream encoder at once is too much for it: the fans scream and the stream drops out.", Cx, Prev.Y + 256.0f, Ts(15.0f, 500, KMuted, Align::Center));
		UI.Text("Upgrade the PC on GearDrop and Kast opens up: chat, followers, subs, sponsors.", Cx, Prev.Y + 280.0f, Ts(15.0f, 500, KMuted, Align::Center));
		const Rect Card{Cx - 260.0f, Prev.Y + 312.0f, 520.0f, 110.0f};
		C->FillRoundRect(Card, 16.0f, KPanel2);
		C->StrokeRoundRect(Card, 16.0f, NetA(Hex(Up.Color), 0.6f), 1.5f);
		C->FillRoundRect({Card.X + 14.0f, Card.Y + 14.0f, 82.0f, 82.0f}, 12.0f, Mix(Hex(Up.Color), Hex(0xffffff), 0.75f));
		streamart::Product(*C, Up.Pic, {Card.X + 16.0f, Card.Y + 16.0f, 78.0f, 78.0f}, Up.Color, Now);
		NetSpaced(*C, "THE NEXT PC UPGRADE", Card.X + 114.0f, Card.Y + 32.0f, 10.0f, 900, KLime, 1.4f);
		UI.Text(Up.Name, Card.X + 114.0f, Card.Y + 60.0f, Ts(20.0f, 900, KInk));
		UI.Text(Up.Effect, Card.X + 114.0f, Card.Y + 84.0f, Ts(13.5f, 700, Hex(Up.Color)));
		UI.Text(Money(Up.PriceCents), Card.X + Card.W - 20.0f, Card.Y + 60.0f, Ts(22.0f, 900, KInk, Align::Right, Baseline::Alphabetic, true));
		if (AppButton("kastupgrade", {Cx - 160.0f, Prev.Y + 446.0f, 320.0f, 54.0f}, "Upgrade on GearDrop", KLime, Hex(0x1b1036), true))
		{
			OpenApp(App::GearDrop, Now);
			ShowStoreCategory(static_cast<int>(gear::Category::Rig));
			ShowOrder(Up.Id);
		}
	}

	// Controls.
	const float Cy = 654.0f;
	const Rect Go{20.0f, Cy, 220.0f, 64.0f};
	if (Live)
	{
		if (AppButton("kastend", Go, "End stream", KRed, Hex(0xffffff), true, KastClock(St.Uptime(World)) + " live"))
		{
			S.EndStream();
		}
	}
	else
	{
		const kast::Inputs In = S.StreamInputs();
		const int Est = static_cast<int>(std::round(St.Target(Ch, In) * 0.6 + 1.0));
		if (AppButton("kastlive", Go, "Go live", KLime, Hex(0x1b1036), CanStream && !S.TimeSkip.Active, CanStream ? "~" + std::to_string(Est) + " viewers to start" : std::string("needs a PC upgrade")))
		{
			S.GoLive();
		}
	}
	// The title.
	{
		const Rect T{252.0f, Cy, 470.0f, 64.0f};
		C->FillRoundRect(T, 12.0f, KPanel);
		C->StrokeRoundRect(T, 12.0f, KLine, 1.0f);
		const kast::TitleSpec& Spec = kast::Titles()[static_cast<size_t>(std::max(0, std::min(static_cast<int>(kast::Titles().size()) - 1, Ch.Title)))];
		NetSpaced(*C, "STREAM TITLE", T.X + 16.0f, T.Y + 22.0f, 10.0f, 900, KDim, 1.4f);
		NetPill(*C, NetUpper(Spec.Tag), T.X + 112.0f, T.Y + 9.0f, KViolet, false, 9.5f);
		UI.Text(Spec.Text, T.X + 16.0f, T.Y + 48.0f, Ts(15.0f, 800, KInk, Align::Left, Baseline::Alphabetic, false, T.W - 100.0f));
		for (int Dir = -1; Dir <= 1; Dir += 2)
		{
			const Rect A{T.X + T.W - (Dir < 0 ? 76.0f : 40.0f), T.Y + 16.0f, 32.0f, 32.0f};
			const Ui::ClickState Cs = UI.Clickable(Dir < 0 ? "kasttitleprev" : "kasttitlenext", A);
			C->FillRoundRect(A, 8.0f, Cs.Hover ? KPanel2 : Hex(0x1a1426));
			NetChevron(*C, A.X + 16.0f, A.Y + 16.0f, 10.0f, Dir < 0 ? 2 : 0, KInk);
			if (Cs.Clicked)
			{
				S.StreamTitle(Ch.Title + Dir);
			}
		}
	}
	// Ads, sponsor reads.
	{
		float X = 736.0f;
		NetSpaced(*C, "AD BREAK", X, Cy + 14.0f, 10.0f, 900, KDim, 1.4f);
		const double Cool = St.AdCooldown(World);
		std::string State = !Ch.Affiliate ? "Affiliates only" : St.AdRunning(Now) ? "running, " + std::to_string(static_cast<int>(std::ceil(St.AdUntil - Now))) + "s" : Cool > 0.0 ? "ready in " + NetHms(Cool) : St.AdsDue(World) ? "due now" : "ready";
		UI.Text(State, X + 80.0f, Cy + 14.0f, Ts(11.5f, 700, St.AdsDue(World) ? KGold : KMuted));
		const int Lengths[3] = {60, 90, 180};
		for (int K = 0; K < 3; ++K)
		{
			const Rect B{X + Nf(K) * 98.0f, Cy + 24.0f, 92.0f, 40.0f};
			const bool Can = Live && Ch.Affiliate && !St.AdRunning(Now) && Cool <= 0.0;
			const Ui::ClickState Cs = UI.Clickable("kastad" + std::to_string(Lengths[K]), B, Can);
			C->FillRoundRect(B, 10.0f, Can ? (Cs.Hover ? KPanel2 : KPanel) : Hex(0x15101d));
			C->StrokeRoundRect(B, 10.0f, Can ? KLine : Hex(0x1d1729), 1.0f);
			UI.Text(K == 2 ? "3 min" : std::to_string(Lengths[K]) + "s", B.X + B.W / 2.0f, B.Y + 18.0f, Ts(14.0f, 800, Can ? KInk : KDim, Align::Center));
			const double Est = St.Viewers * static_cast<double>(Lengths[K]) / 30.0 * (Ch.Partner ? 0.45 : 0.32);
			UI.Text("~" + Money(static_cast<Chips>(std::round(Est))), B.X + B.W / 2.0f, B.Y + 33.0f, Ts(11.0f, 700, Can ? KLime : KDim, Align::Center, Baseline::Alphabetic, true));
			if (Cs.Clicked)
			{
				S.StreamAd(Lengths[K]);
			}
		}
	}
	// Sponsor reads and the prediction, in a strip.
	{
		float X = 20.0f;
		const float Y = Cy + 76.0f;
		bool Any = false;
		for (const kast::Deal& D : Ch.Deals)
		{
			const kast::Sponsor* Sp = D.Until > World ? kast::FindSponsor(D.Id) : nullptr;
			if (!Sp)
			{
				continue;
			}
			Any = true;
			const bool Can = Live && D.ReadAt < St.StartWorld;
			const std::string Label = Can ? "Read for " + Sp->Brand + "  +" + Money(Sp->ReadCents) : Sp->Brand + (Live ? ": read done" : ": read when live");
			const float W = UI.Measure(Label, 13.0f, 800) + 30.0f;
			const Rect B{X, Y, W, 32.0f};
			const Ui::ClickState Cs = UI.Clickable("kastread" + D.Id, B, Can);
			C->FillRoundRect(B, 9.0f, Can ? (Cs.Hover ? Hex(Sp->Color) : NetA(Hex(Sp->Color), 0.22f)) : Hex(0x15101d));
			UI.Text(Label, B.X + 15.0f, B.Y + 21.0f, Ts(13.0f, 800, Can ? (Cs.Hover ? Hex(0x0b0716) : Hex(Sp->Color)) : KDim));
			if (Cs.Clicked)
			{
				S.StreamRead(D.Id);
			}
			X += W + 8.0f;
		}
		if (!Any)
		{
			UI.Text(Ch.Offers.empty() ? "No sponsors yet: they find you at " + Grouped(kast::Sponsors().front().Followers) + " followers." : "A sponsor offer is waiting on the Channel page.", X, Y + 21.0f,
				Ts(13.0f, 600, Ch.Offers.empty() ? KDim : KGold));
		}
		if (St.Pred.Active)
		{
			const int Total = std::max(1, St.Pred.Yes + St.Pred.No);
			const std::string P = St.Pred.Resolved ? std::string("Prediction: ") + (St.Pred.Outcome ? "YES" : "NO") + " won" : "Prediction: " + std::to_string(St.Pred.Yes * 100 / Total) + "% believe";
			UI.Text(P, 1028.0f, Y + 21.0f, Ts(13.0f, 700, KViolet, Align::Right));
		}
	}

	// Numbers.
	const float Ty = 772.0f;
	const float Tw = (1008.0f - 5.0f * 10.0f) / 6.0f;
	auto Tile = [&](int K, const std::string& Label, const std::string& Big, const std::string& Sub, const Color& Col) -> Rect {
		const Rect R{20.0f + Nf(K) * (Tw + 10.0f), Ty, Tw, 174.0f};
		C->FillRoundRect(R, 14.0f, KPanel);
		C->StrokeRoundRect(R, 14.0f, KLine, 1.0f);
		NetSpaced(*C, Label, R.X + 16.0f, R.Y + 26.0f, 10.0f, 900, KDim, 1.4f);
		UI.Text(Big, R.X + 16.0f, R.Y + 68.0f, Ts(30.0f, 900, Col, Align::Left, Baseline::Alphabetic, true, R.W - 24.0f));
		UI.Text(Sub, R.X + 16.0f, R.Y + 92.0f, Ts(12.0f, 600, KMuted, Align::Left, Baseline::Alphabetic, false, R.W - 24.0f));
		return R;
	};
	{
		const Rect R = Tile(0, "VIEWERS", Live ? KastCount(St.Viewers) : std::string("\xE2\x80\x94"), Live ? "peak " + KastCount(St.Peak) : "offline", Live ? KRed : KDim);
		KastSpark(*C, {R.X + 12.0f, R.Y + 106.0f, R.W - 24.0f, 54.0f}, St.Graph, KRed);
	}
	{
		const Rect R = Tile(1, "FOLLOWERS", KastCount(Ch.Followers), Live ? "+" + Grouped(St.Tonight.Follows) + " tonight" : "next: " + Grouped(Ch.Milestone > 0 ? Ch.Milestone : 10), KInk);
		const int Next = Ch.Followers < kast::AffiliateFollowers ? kast::AffiliateFollowers : Ch.Followers < kast::PartnerFollowers ? kast::PartnerFollowers : (Ch.Followers / 10000 + 1) * 10000;
		const float P = Nf(std::min(1.0, static_cast<double>(Ch.Followers) / static_cast<double>(Next)));
		C->FillRoundRect({R.X + 16.0f, R.Y + 128.0f, R.W - 32.0f, 8.0f}, 4.0f, KPanel2);
		C->FillRoundRect({R.X + 16.0f, R.Y + 128.0f, std::max(8.0f, (R.W - 32.0f) * P), 8.0f}, 4.0f, KViolet);
		UI.Text(Grouped(Next) + (Next == kast::AffiliateFollowers ? " for affiliate" : Next == kast::PartnerFollowers ? " for partner" : ""), R.X + 16.0f, R.Y + 156.0f, Ts(11.5f, 600, KDim));
	}
	{
		const Rect R = Tile(2, "SUBSCRIBERS", Ch.Affiliate ? Grouped(Ch.ActiveSubs(World)) : std::string("\xE2\x80\x94"), Ch.Affiliate ? (Live ? "+" + std::to_string(St.Tonight.Subs + St.Tonight.Gifted) + " tonight" : (Ch.Partner ? "70% share" : "50% share")) : "affiliate unlocks subs", Ch.Affiliate ? KLime : KDim);
		UI.Text(Ch.GiftedSubs > 0 ? Grouped(Ch.GiftedSubs) + " gifted, all time" : "", R.X + 16.0f, R.Y + 156.0f, Ts(11.5f, 600, KDim));
	}
	{
		const kast::Summary& T = St.Tonight;
		const Rect R = Tile(3, "TONIGHT", Money(Live ? T.Total() : 0), "balance " + Money(Ch.UnpaidCents), KGold);
		const std::pair<const char*, Chips> Parts[4] = {{"tips", T.TipCents}, {"subs", T.SubCents}, {"bits", T.BitCents}, {"sponsor", T.SponsorCents + T.AdCents}};
		for (int K = 0; K < 4; ++K)
		{
			const float Py = R.Y + 118.0f + Nf(K) * 13.5f;
			UI.Text(Parts[K].first, R.X + 16.0f, Py, Ts(11.0f, 600, KDim));
			UI.Text(NetMoney(Live ? Parts[K].second : 0), R.X + R.W - 16.0f, Py, Ts(11.0f, 800, KInk, Align::Right, Baseline::Alphabetic, true));
		}
	}
	{
		const Rect R = Tile(4, "HYPE", Live ? std::to_string(static_cast<int>(St.Hype)) : std::string("\xE2\x80\x94"), Live ? (St.Hype > 60 ? "chat is losing it" : St.Hype > 30 ? "chat is awake" : "quiet grind") : "big hands make hype", Hex(0xf97316));
		const float H = Nf(St.Hype / 100.0);
		C->FillRoundRect({R.X + 16.0f, R.Y + 128.0f, R.W - 32.0f, 10.0f}, 5.0f, KPanel2);
		C->FillRoundRect({R.X + 16.0f, R.Y + 128.0f, std::max(10.0f, (R.W - 32.0f) * H), 10.0f}, 5.0f, Paint::Linear({R.X, 0.0f}, {R.X + R.W, 0.0f}, Hex(0xfacc15), Hex(0xef4444)));
		streamart::Emote(*C, static_cast<int>(kast::Emote::Hype), R.X + R.W - 46.0f, R.Y + 40.0f, 30.0f + (Live ? 8.0f * H * Nf(0.5 + 0.5 * std::sin(Now * 8.0)) : 0.0f));
	}
	{
		const Rect R = Tile(5, "CHAT HEALTH", Live ? std::to_string(static_cast<int>(St.Health * 100.0)) + "%" : std::string("\xE2\x80\x94"), std::to_string(St.ModsOnline()) + (St.ModsOnline() == 1 ? " mod" : " mods") + " watching", St.Health > 0.7 ? Hex(0x22c55e) : St.Health > 0.4 ? KGold : KRed);
		UI.Text(Live ? std::to_string(St.Caught) + " caught \xC2\xB7 " + std::to_string(St.Missed) + " missed" : std::string("trolls show up as you grow"), R.X + 16.0f, R.Y + 132.0f, Ts(11.5f, 600, KDim));
	}

	// Chat.
	StreamChat({1044.0f, 74.0f, 536.0f, 872.0f}, Now, true);
}

void RiverLine::KastChannel(double Now)
{
	const kast::Channel& Ch = S.Channel;
	const float In = NetEase((Now - KastAt) / 0.5);
	// The banner.
	const Rect Ban{20.0f, 74.0f, NetW - 40.0f, 170.0f};
	C->FillRoundRect(Ban, 18.0f, Paint::Linear({Ban.X, Ban.Y}, {Ban.X + Ban.W, Ban.Y + Ban.H}, Hex(0x3b1d7a), Hex(0x14091f)));
	C->PushClip(Ban);
	for (int K = 0; K < 5; ++K)
	{
		C->FillCircle(Ban.X + Ban.W - 120.0f - Nf(K) * 180.0f, Ban.Y + 40.0f + Nf(K % 2) * 90.0f, 70.0f + Nf(K) * 12.0f, NetA(K % 2 ? KLime : KViolet, 0.06f));
	}
	C->PopClip();
	AvatarSpec Av = AvatarFor(S.HeroName);
	Av.Frame = S.TeamRiverLine() ? AvatarFrame::Gold : AvatarFrame::Neon;
	Av.Rim = kast::Lime;
	DrawAvatar(*C, Ban.X + 90.0f, Ban.Y + Ban.H / 2.0f, 56.0f, Av);
	UI.Text(S.HeroName, Ban.X + 170.0f, Ban.Y + 72.0f, Ts(34.0f, 900, KInk));
	float Bx = Ban.X + 182.0f + UI.Measure(S.HeroName, 34.0f, 900);
	if (Ch.Partner)
	{
		C->FillCircle(Bx + 12.0f, Ban.Y + 60.0f, 12.0f, KViolet);
		NetCheck(*C, Bx + 12.0f, Ban.Y + 60.0f, 13.0f, Hex(0xffffff));
		Bx += 32.0f;
	}
	Bx += 6.0f;
	if (Ch.Partner || Ch.Affiliate)
	{
		Bx += NetPill(*C, Ch.Partner ? "PARTNER" : "AFFILIATE", Bx, Ban.Y + 50.0f, Ch.Partner ? KViolet : KLime, true, 11.0f) + 8.0f;
	}
	if (S.TeamRiverLine())
	{
		NetPill(*C, "TEAM RIVERLINE", Bx, Ban.Y + 50.0f, pal::Accent, true, 11.0f);
	}
	UI.Text(Grouped(Ch.Followers) + " followers \xC2\xB7 " + Grouped(Ch.ActiveSubs(World)) + " subs \xC2\xB7 " + std::to_string(Ch.Streams) + " streams \xC2\xB7 " + NetHms(Ch.MinutesLive).substr(0, 6) + " live",
		Ban.X + 170.0f, Ban.Y + 104.0f, Ts(15.0f, 600, Hex(0xcfc4ee)));
	UI.Text("Poker \xC2\xB7 RiverLine tournaments \xC2\xB7 peak " + Grouped(Ch.Peak) + " viewers", Ban.X + 170.0f, Ban.Y + 130.0f, Ts(13.0f, 600, KMuted));
	// Money.
	const float Mx = Ban.X + Ban.W - 30.0f;
	NetSpaced(*C, "CHANNEL BALANCE", Mx - 260.0f, Ban.Y + 42.0f, 10.0f, 900, Hex(0xcfc4ee), 1.4f);
	UI.Text(Money(Ch.UnpaidCents), Mx - 260.0f, Ban.Y + 88.0f, Ts(36.0f, 900, KLime, Align::Left, Baseline::Alphabetic, true));
	UI.Text("earned all time " + Money(Ch.Earned()), Mx - 260.0f, Ban.Y + 110.0f, Ts(12.5f, 600, KMuted));
	if (AppButton("kastcashout", {Mx - 260.0f, Ban.Y + 120.0f, 200.0f, 38.0f}, "Cash out to bank", KLime, Hex(0x1b1036), Ch.UnpaidCents > 0))
	{
		S.CashOut();
	}

	// Left column: the road to partner, the money, past streams.
	const float Lx = 20.0f;
	const float Lw = 760.0f;
	float Y = 262.0f;
	{
		const Rect R{Lx, Y, Lw, 196.0f};
		C->FillRoundRect(R, 16.0f, KPanel);
		UI.Text("Growing the channel", R.X + 20.0f, R.Y + 34.0f, Ts(18.0f, 900, KInk));
		struct Goal
		{
			std::string Label;
			double Have;
			double Need;
			std::string Text;
		};
		const double Hours = Ch.MinutesLive / 60.0;
		const std::vector<Goal> Aff = {{"Followers", static_cast<double>(Ch.Followers), kast::AffiliateFollowers, Grouped(std::min(Ch.Followers, kast::AffiliateFollowers)) + " / " + std::to_string(kast::AffiliateFollowers)},
			{"Hours live", Hours, kast::AffiliateMinutes / 60.0, Fixed(std::min(Hours, 8.0), 1) + " / 8"},
			{"Streams", static_cast<double>(Ch.Streams), kast::AffiliateStreams, std::to_string(std::min(Ch.Streams, kast::AffiliateStreams)) + " / " + std::to_string(kast::AffiliateStreams)}};
		const std::vector<Goal> Par = {{"Followers", static_cast<double>(Ch.Followers), kast::PartnerFollowers, Grouped(std::min(Ch.Followers, kast::PartnerFollowers)) + " / " + Grouped(kast::PartnerFollowers)},
			{"Average viewers", Ch.AvgViewers(), kast::PartnerAvgViewers, std::to_string(static_cast<int>(std::min(Ch.AvgViewers(), 75.0))) + " / 75"}};
		for (int Col = 0; Col < 2; ++Col)
		{
			const float Cx = R.X + 20.0f + Nf(Col) * (Lw / 2.0f);
			const bool Done = Col == 0 ? Ch.Affiliate : Ch.Partner;
			UI.Text(Col == 0 ? "Affiliate" : "Partner", Cx, R.Y + 70.0f, Ts(15.0f, 800, Done ? KLime : KInk));
			UI.Text(Done ? "unlocked" : Col == 0 ? "subs, cheers, ads" : "70% subs, verified", Cx + (Col == 0 ? 80.0f : 70.0f), R.Y + 70.0f, Ts(12.0f, 600, Done ? KLime : KMuted));
			const std::vector<Goal>& G = Col == 0 ? Aff : Par;
			for (size_t I = 0; I < G.size(); ++I)
			{
				const float Gy = R.Y + 96.0f + Nf(I) * 32.0f;
				const float P = Nf(std::min(1.0, G[I].Have / G[I].Need)) * In;
				UI.Text(G[I].Label, Cx, Gy + 4.0f, Ts(12.5f, 600, KMuted));
				UI.Text(G[I].Text, Cx + Lw / 2.0f - 40.0f, Gy + 4.0f, Ts(12.5f, 800, P >= 1.0f ? KLime : KInk, Align::Right, Baseline::Alphabetic, true));
				C->FillRoundRect({Cx, Gy + 11.0f, Lw / 2.0f - 40.0f, 6.0f}, 3.0f, KPanel2);
				C->FillRoundRect({Cx, Gy + 11.0f, std::max(6.0f, (Lw / 2.0f - 40.0f) * P), 6.0f}, 3.0f, P >= 1.0f ? KLime : KViolet);
			}
		}
		Y += 196.0f + 14.0f;
	}
	{
		const Rect R{Lx, Y, Lw, 216.0f};
		C->FillRoundRect(R, 16.0f, KPanel);
		UI.Text("Where the money comes from", R.X + 20.0f, R.Y + 34.0f, Ts(18.0f, 900, KInk));
		UI.Text("paid " + Money(Ch.PaidCents) + " to the bank", R.X + R.W - 20.0f, R.Y + 34.0f, Ts(13.0f, 600, KMuted, Align::Right));
		const std::pair<const char*, std::pair<Chips, Color>> Bars[5] = {{"Subscriptions", {Ch.EarnedSubs, KLime}}, {"Tips", {Ch.EarnedTips, KGold}}, {"Cheers", {Ch.EarnedBits, Hex(0xf472b6)}},
			{"Ads", {Ch.EarnedAds, Hex(0x38bdf8)}}, {"Sponsors", {Ch.EarnedSponsors, KViolet}}};
		Chips Top = 1;
		for (const auto& B : Bars)
		{
			Top = std::max(Top, B.second.first);
		}
		for (int K = 0; K < 5; ++K)
		{
			const float By = R.Y + 64.0f + Nf(K) * 30.0f;
			UI.Text(Bars[K].first, R.X + 20.0f, By + 12.0f, Ts(13.0f, 600, KMuted));
			const float W = (R.W - 300.0f) * Nf(static_cast<double>(Bars[K].second.first) / static_cast<double>(Top)) * In;
			C->FillRoundRect({R.X + 140.0f, By + 2.0f, R.W - 300.0f, 12.0f}, 6.0f, KPanel2);
			if (W > 1.0f)
			{
				C->FillRoundRect({R.X + 140.0f, By + 2.0f, std::max(12.0f, W), 12.0f}, 6.0f, Bars[K].second.second);
			}
			UI.Text(Money(Bars[K].second.first), R.X + R.W - 20.0f, By + 13.0f, Ts(13.5f, 800, KInk, Align::Right, Baseline::Alphabetic, true));
		}
		Y += 216.0f + 14.0f;
	}
	{
		const Rect R{Lx, Y, Lw, KastH - Y - 16.0f};
		C->FillRoundRect(R, 16.0f, KPanel);
		UI.Text("Past streams", R.X + 20.0f, R.Y + 34.0f, Ts(18.0f, 900, KInk));
		const char* Heads[6] = {"WHEN", "LENGTH", "AVG", "PEAK", "FOLLOWS", "EARNED"};
		const float Cols[6] = {20.0f, 250.0f, 350.0f, 430.0f, 510.0f, 740.0f};
		for (int K = 0; K < 6; ++K)
		{
			NetSpaced(*C, Heads[K], R.X + Cols[K] - (K == 5 ? UI.Measure(Heads[K], 10.0f, 900) + 7.0f : 0.0f), R.Y + 62.0f, 10.0f, 900, KDim, 1.2f);
		}
		if (Ch.Log.empty())
		{
			UI.Text("Nothing yet. Every stream lands here with its numbers.", R.X + 20.0f, R.Y + 96.0f, Ts(14.0f, 500, KMuted));
		}
		for (size_t I = 0; I < Ch.Log.size(); ++I)
		{
			const float Ry = R.Y + 90.0f + Nf(I) * 30.0f;
			if (Ry > R.Y + R.H - 14.0f)
			{
				break;
			}
			const kast::StreamLog& L = Ch.Log[I];
			UI.Text(std::string(net::WeekdayName(net::DayOf(L.Start), true)) + " " + net::TimeLabel(L.Start), R.X + Cols[0], Ry, Ts(13.5f, 700, KInk));
			UI.Text(NetHms(L.Minutes / 60.0).substr(0, 5), R.X + Cols[1], Ry, Ts(13.5f, 600, KMuted, Align::Left, Baseline::Alphabetic, true));
			UI.Text(KastCount(L.Avg), R.X + Cols[2], Ry, Ts(13.5f, 700, KInk, Align::Left, Baseline::Alphabetic, true));
			UI.Text(KastCount(L.Peak), R.X + Cols[3], Ry, Ts(13.5f, 700, KInk, Align::Left, Baseline::Alphabetic, true));
			UI.Text("+" + Grouped(L.Follows), R.X + Cols[4], Ry, Ts(13.5f, 700, KViolet, Align::Left, Baseline::Alphabetic, true));
			UI.Text(Money(L.Cents), R.X + Cols[5], Ry, Ts(13.5f, 800, KLime, Align::Right, Baseline::Alphabetic, true));
		}
	}

	// Right column: sponsors, clips.
	const float Rx = 800.0f;
	const float Rw = NetW - 20.0f - Rx;
	float Ry = 262.0f;
	{
		const Rect R{Rx, Ry, Rw, 336.0f};
		C->FillRoundRect(R, 16.0f, KPanel);
		UI.Text("Sponsors", R.X + 20.0f, R.Y + 34.0f, Ts(18.0f, 900, KInk));
		UI.Text(std::to_string(kast::MaxDeals) + " slots on the overlay", R.X + R.W - 20.0f, R.Y + 34.0f, Ts(13.0f, 600, KMuted, Align::Right));
		float Sy = R.Y + 54.0f;
		bool Any = false;
		for (const kast::Deal& D : Ch.Deals)
		{
			const kast::Sponsor* Sp = D.Until > World ? kast::FindSponsor(D.Id) : nullptr;
			if (!Sp || Sy > R.Y + R.H - 60.0f)
			{
				continue;
			}
			Any = true;
			const Rect Row{R.X + 14.0f, Sy, R.W - 28.0f, 60.0f};
			C->FillRoundRect(Row, 12.0f, KPanel2);
			C->FillRoundRect({Row.X, Row.Y, 6.0f, Row.H}, 3.0f, Hex(Sp->Color));
			UI.Text(Sp->Brand, Row.X + 20.0f, Row.Y + 25.0f, Ts(15.0f, 900, Hex(Sp->Color)));
			UI.Text(Sp->Product, Row.X + 20.0f, Row.Y + 45.0f, Ts(12.0f, 600, KMuted));
			UI.Text(Money(D.EarnedCents) + " earned", Row.X + Row.W - 16.0f, Row.Y + 25.0f, Ts(14.0f, 800, KLime, Align::Right, Baseline::Alphabetic, true));
			UI.Text(std::to_string(static_cast<int>(std::ceil((D.Until - World) / net::MinutesPerDay))) + " days left", Row.X + Row.W - 16.0f, Row.Y + 45.0f, Ts(12.0f, 600, KMuted, Align::Right));
			Sy += 68.0f;
		}
		for (const std::string& Id : Ch.Offers)
		{
			const kast::Sponsor* Sp = kast::FindSponsor(Id);
			if (!Sp || Sy > R.Y + R.H - 80.0f)
			{
				continue;
			}
			Any = true;
			const Rect Row{R.X + 14.0f, Sy, R.W - 28.0f, 118.0f};
			C->FillRoundRect(Row, 12.0f, NetA(Hex(Sp->Color), 0.1f));
			C->StrokeRoundRect(Row, 12.0f, NetA(Hex(Sp->Color), 0.5f), 1.5f);
			NetPill(*C, "OFFER", Row.X + 14.0f, Row.Y + 12.0f, KGold, true, 10.0f);
			UI.Text(Sp->Brand, Row.X + 76.0f, Row.Y + 27.0f, Ts(16.0f, 900, Hex(Sp->Color)));
			UI.Text(Money(Sp->PerHourCents) + "/hour live \xC2\xB7 " + Money(Sp->ReadCents) + " a read", Row.X + Row.W - 14.0f, Row.Y + 27.0f, Ts(13.0f, 800, KInk, Align::Right));
			NetParagraph(*C, Sp->Pitch, Row.X + 14.0f, Row.Y + 52.0f, Row.W - 28.0f, 12.5f, 500, KMuted, 17.0f, 2);
			if (AppButton("kastaccept" + Id, {Row.X + 14.0f, Row.Y + 80.0f, 140.0f, 30.0f}, "Accept", KLime, Hex(0x1b1036), true))
			{
				const std::string Why = S.AcceptDeal(Id);
				if (!Why.empty())
				{
					Toast = Why;
					ToastAt = Now;
				}
				break;
			}
			const Rect No{Row.X + 164.0f, Row.Y + 80.0f, 100.0f, 30.0f};
			const Ui::ClickState Cs = UI.Clickable("kastdecline" + Id, No);
			C->FillRoundRect(No, 10.0f, Cs.Hover ? KPanel2 : Hex(0x1a1426));
			UI.Text("Decline", No.X + No.W / 2.0f, No.Y + 20.0f, Ts(13.0f, 700, KMuted, Align::Center));
			if (Cs.Clicked)
			{
				S.DeclineDeal(Id);
				break;
			}
			Sy += 128.0f;
		}
		if (!Any)
		{
			Sy = NetParagraph(*C, "Brands watch the follower count. The first offers come at " + Grouped(kast::Sponsors().front().Followers) + " followers; the big ones want a Partner.", R.X + 20.0f, Sy + 20.0f, R.W - 40.0f, 14.0f, 500, KMuted, 20.0f, 3);
			for (const kast::Sponsor& Sp : kast::Sponsors())
			{
				if (Sy > R.Y + R.H - 24.0f)
				{
					break;
				}
				UI.Text(Sp.Brand, R.X + 20.0f, Sy + 24.0f, Ts(13.5f, 800, NetA(Hex(Sp.Color), 0.85f)));
				UI.Text(Grouped(Sp.Followers) + " followers" + (Sp.NeedsPartner ? " + partner" : "") + " \xC2\xB7 " + Money(Sp.PerHourCents) + "/h", R.X + R.W - 20.0f, Sy + 24.0f, Ts(12.5f, 600, KMuted, Align::Right));
				Sy += 26.0f;
			}
		}
		Ry += 336.0f + 14.0f;
	}
	{
		const Rect R{Rx, Ry, Rw, KastH - Ry - 16.0f};
		C->FillRoundRect(R, 16.0f, KPanel);
		UI.Text("Top clips", R.X + 20.0f, R.Y + 34.0f, Ts(18.0f, 900, KInk));
		if (Ch.Clips.empty())
		{
			UI.Text("Chat clips the big hands. Win an all-in on stream.", R.X + 20.0f, R.Y + 70.0f, Ts(14.0f, 500, KMuted));
		}
		std::vector<const kast::Clip*> Sorted;
		for (const kast::Clip& Cl : Ch.Clips)
		{
			Sorted.push_back(&Cl);
		}
		std::stable_sort(Sorted.begin(), Sorted.end(), [](const kast::Clip* A, const kast::Clip* B) { return A->Views > B->Views; });
		for (size_t I = 0; I < Sorted.size(); ++I)
		{
			const float Cy = R.Y + 54.0f + Nf(I) * 62.0f;
			if (Cy + 56.0f > R.Y + R.H)
			{
				break;
			}
			const kast::Clip& Cl = *Sorted[I];
			const Rect Th{R.X + 16.0f, Cy, 96.0f, 54.0f};
			C->FillRoundRect(Th, 6.0f, Hex(0x0a111c));
			C->FillEllipse(Th.X + Th.W / 2.0f, Th.Y + Th.H * 0.55f, Th.W * 0.36f, Th.H * 0.28f, Hex(0x0f6a4e));
			C->FillPolygon({{Th.X + 40.0f, Th.Y + 17.0f}, {Th.X + 58.0f, Th.Y + 27.0f}, {Th.X + 40.0f, Th.Y + 37.0f}}, Hex(0xffffff));
			UI.Text(Cl.Title, Th.X + Th.W + 14.0f, Cy + 22.0f, Ts(14.0f, 800, KInk, Align::Left, Baseline::Alphabetic, false, R.W - Th.W - 60.0f));
			UI.Text(KastCount(Cl.Views) + " views \xC2\xB7 clipped by " + Cl.By, Th.X + Th.W + 14.0f, Cy + 42.0f, Ts(12.0f, 600, Cl.Views > 10000 ? KLime : KMuted, Align::Left, Baseline::Alphabetic, false, R.W - Th.W - 60.0f));
		}
	}
	(void)ChannelScroll;
}

void RiverLine::KastBrowse(double Now)
{
	struct Card
	{
		std::string Name;
		std::string Title;
		std::string Tag;
		int Viewers = 0;
		uint32_t Color = 0;
		bool You = false;
		bool Rival = false;
	};
	std::vector<Card> Live;
	std::vector<const kast::Streamer*> Off;
	for (const kast::Streamer& St : kast::Directory())
	{
		const int V = kast::ViewersNow(St, World);
		if (V > 0)
		{
			Live.push_back({St.Name, St.Title, St.Tag, V, St.Color, false, St.Rival});
		}
		else
		{
			Off.push_back(&St);
		}
	}
	if (S.Streaming())
	{
		const kast::TitleSpec& T = kast::Titles()[static_cast<size_t>(std::max(0, std::min(static_cast<int>(kast::Titles().size()) - 1, S.Channel.Title)))];
		Live.push_back({S.HeroName, T.Text, "That's you.", static_cast<int>(std::round(S.Stream.Viewers)), kast::Lime, true, false});
	}
	std::stable_sort(Live.begin(), Live.end(), [](const Card& A, const Card& B) { return A.Viewers > B.Viewers; });
	long long Total = 0;
	int Rank = 0;
	for (size_t I = 0; I < Live.size(); ++I)
	{
		Total += Live[I].Viewers;
		Rank = Live[I].You ? static_cast<int>(I) + 1 : Rank;
	}
	UI.Text("Poker", 24.0f, 112.0f, Ts(34.0f, 900, KInk));
	UI.Text(std::to_string(Live.size()) + " channels live \xC2\xB7 " + Grouped(Total) + " viewers \xC2\xB7 sorted by viewers", 24.0f, 140.0f, Ts(14.0f, 600, KMuted));
	if (Rank > 0)
	{
		const std::string R = "You're #" + std::to_string(Rank) + " in Poker right now";
		const float W = UI.Measure(R, 15.0f, 900) + 32.0f;
		C->FillRoundRect({NetW - 24.0f - W, 92.0f, W, 36.0f}, 10.0f, KLime);
		UI.Text(R, NetW - 24.0f - W / 2.0f, 116.0f, Ts(15.0f, 900, Hex(0x1b1036), Align::Center));
	}
	else
	{
		UI.Text(S.Streaming() ? "" : "Go live and your channel shows up here, ranked by viewers.", NetW - 24.0f, 116.0f, Ts(14.0f, 600, KMuted, Align::Right));
	}
	const float CardW = 362.0f;
	const float CardH = 268.0f;
	for (size_t I = 0; I < Live.size() && I < 8; ++I)
	{
		const Card& Cd = Live[I];
		const float Rise = (1.0f - NetEase((Now - KastAt - 0.04 * static_cast<double>(I)) / 0.4)) * 20.0f;
		const Rect R{24.0f + Nf(I % 4) * (CardW + 16.0f), 160.0f + Nf(I / 4) * (CardH + 18.0f) + Rise, CardW, CardH};
		const Rect Th{R.X, R.Y, R.W, 204.0f};
		C->FillRoundRect(Th, 10.0f, Paint::Linear({Th.X, Th.Y}, {Th.X, Th.Y + Th.H}, Hex(0x0e1726), Hex(0x0a111c)));
		C->FillEllipse(Th.X + Th.W * 0.42f, Th.Y + Th.H * 0.54f, Th.W * 0.3f, Th.H * 0.26f, Paint::Radial({Th.X + Th.W * 0.42f, Th.Y + Th.H * 0.5f}, 0.0f, {Th.X + Th.W * 0.42f, Th.Y + Th.H * 0.5f}, Th.W * 0.32f, Hex(0x15885f), 0.6f, Hex(0x0f6a4e), Hex(0x083a2b)));
		for (int K = 0; K < 3; ++K)
		{
			C->FillRoundRect({Th.X + Th.W * (0.34f + 0.06f * Nf(K)), Th.Y + Th.H * 0.47f, Th.W * 0.045f, Th.H * 0.13f}, 2.0f, Hex(0xf5f5f0));
		}
		if (Cd.You)
		{
			streamart::Cam Look;
			Look.Gear = S.GearFx();
			Look.Headphones = S.Owns("headphones");
			Look.Face = S.Stream.Face;
			Look.FaceAge = Now - S.Stream.FaceAt;
			Look.Time = Now;
			streamart::Facecam(*C, {Th.X + Th.W - 140.0f, Th.Y + Th.H - 84.0f, 132.0f, 76.0f}, Look);
		}
		else
		{
			const Rect Cam{Th.X + Th.W - 140.0f, Th.Y + Th.H - 84.0f, 132.0f, 76.0f};
			C->FillRoundRect(Cam, 4.0f, Mix(Hex(Cd.Color), Hex(0x000000), 0.6f));
			C->PushClip(Cam);
			C->FillEllipse(Cam.X + Cam.W / 2.0f, Cam.Y + Cam.H + 8.0f, 36.0f, 30.0f, Mix(Hex(Cd.Color), Hex(0x000000), 0.15f));
			C->FillCircle(Cam.X + Cam.W / 2.0f, Cam.Y + Cam.H * 0.44f, 15.0f, Hex(0xd9a37c));
			C->FillEllipse(Cam.X + Cam.W / 2.0f, Cam.Y + Cam.H * 0.44f - 10.0f, 15.0f, 8.0f, Hex(0x2b1d14));
			C->PopClip();
		}
		NetPill(*C, Cd.Rival ? "LIVE \xC2\xB7 24/7" : "LIVE", Th.X + 10.0f, Th.Y + 10.0f, KRed, true, 10.5f);
		const std::string V = KastCount(Cd.Viewers) + " viewers";
		const float Vw = UI.Measure(V, 12.0f, 800) + 16.0f;
		C->FillRoundRect({Th.X + 10.0f, Th.Y + Th.H - 32.0f, Vw, 22.0f}, 5.0f, Rgba(0, 0, 0, 0.7f));
		UI.Text(V, Th.X + 18.0f, Th.Y + Th.H - 16.5f, Ts(12.0f, 800, Hex(0xffffff)));
		if (Cd.You)
		{
			C->StrokeRoundRect(Th, 10.0f, KLime, 3.0f);
		}
		AvatarSpec Av = AvatarFor(Cd.Name);
		if (Cd.Rival)
		{
			Av.Icon = AvatarIcon::Ghost;
		}
		DrawAvatar(*C, R.X + 20.0f, R.Y + 230.0f, 18.0f, Av);
		UI.Text(Cd.Title, R.X + 48.0f, R.Y + 226.0f, Ts(14.5f, 800, KInk, Align::Left, Baseline::Alphabetic, false, R.W - 52.0f));
		UI.Text(Cd.Name, R.X + 48.0f, R.Y + 246.0f, Ts(13.0f, 700, Cd.You ? KLime : KMuted));
		UI.Text(Cd.Tag, R.X + 48.0f, R.Y + 264.0f, Ts(11.5f, 500, KDim, Align::Left, Baseline::Alphabetic, false, R.W - 52.0f));
	}
	// Offline, at the bottom.
	float Ox = 24.0f;
	UI.Text("Offline", 24.0f, 760.0f + (Live.size() > 4 ? 0.0f : -270.0f), Ts(18.0f, 900, KInk));
	const float Oy = 778.0f + (Live.size() > 4 ? 0.0f : -270.0f);
	for (const kast::Streamer* St : Off)
	{
		if (Ox > NetW - 260.0f)
		{
			break;
		}
		const Rect R{Ox, Oy, 240.0f, 70.0f};
		C->FillRoundRect(R, 12.0f, KPanel);
		AvatarSpec Av = AvatarFor(St->Name);
		DrawAvatar(*C, R.X + 30.0f, R.Y + 35.0f, 18.0f, Av);
		UI.Text(St->Name, R.X + 58.0f, R.Y + 30.0f, Ts(14.0f, 800, KInk, Align::Left, Baseline::Alphabetic, false, R.W - 66.0f));
		const int Opens = St->Opens;
		UI.Text("live at " + net::TimeLabel(static_cast<double>(Opens)) + " \xC2\xB7 " + KastCount(St->Followers), R.X + 58.0f, R.Y + 50.0f, Ts(12.0f, 600, KMuted));
		Ox += 252.0f;
	}
}

void RiverLine::StreamSummary(double Now)
{
	const kast::Summary& L = S.Stream.Last;
	C->FillRect({0.0f, 0.0f, NetW, KastH}, Rgba(5, 2, 10, 0.7f));
	const Rect R{NetW / 2.0f - 360.0f, 150.0f, 720.0f, 600.0f};
	const float In = NetEase((Now - S.Stream.FaceAt) / 0.4);
	(void)In;
	C->GlowRoundRect(R, 22.0f, NetA(KViolet, 0.4f), 30.0f);
	C->FillRoundRect(R, 22.0f, Paint::Linear({R.X, R.Y}, {R.X, R.Y + R.H}, Hex(0x221640), KPanel));
	C->StrokeRoundRect(R, 22.0f, KLine, 1.0f);
	NetSpaced(*C, "STREAM ENDED", R.X + 40.0f, R.Y + 56.0f, 12.0f, 900, KViolet, 2.0f);
	UI.Text(KastClock(L.Minutes) + " live", R.X + 40.0f, R.Y + 104.0f, Ts(40.0f, 900, KInk, Align::Left, Baseline::Alphabetic, true));
	struct Num
	{
		const char* Label;
		std::string Value;
		Color Col;
	};
	const Num Nums[4] = {{"AVERAGE VIEWERS", KastCount(L.Avg), KInk}, {"PEAK", KastCount(L.Peak), KRed}, {"NEW FOLLOWERS", "+" + Grouped(L.Follows), KViolet}, {"NEW SUBS", "+" + Grouped(L.Subs + L.Gifted), KLime}};
	for (int K = 0; K < 4; ++K)
	{
		const float X = R.X + 40.0f + Nf(K) * 162.0f;
		NetSpaced(*C, Nums[K].Label, X, R.Y + 150.0f, 9.5f, 900, KDim, 1.2f);
		UI.Text(Nums[K].Value, X, R.Y + 188.0f, Ts(28.0f, 900, Nums[K].Col, Align::Left, Baseline::Alphabetic, true));
	}
	C->FillRect({R.X + 40.0f, R.Y + 214.0f, R.W - 80.0f, 1.0f}, KLine);
	const std::pair<const char*, std::pair<Chips, Color>> Bars[5] = {{"Tips", {L.TipCents, KGold}}, {"Subscriptions", {L.SubCents, KLime}}, {"Cheers", {L.BitCents, Hex(0xf472b6)}}, {"Ads", {L.AdCents, Hex(0x38bdf8)}}, {"Sponsors", {L.SponsorCents, KViolet}}};
	Chips Top = 1;
	for (const auto& B : Bars)
	{
		Top = std::max(Top, B.second.first);
	}
	for (int K = 0; K < 5; ++K)
	{
		const float By = R.Y + 240.0f + Nf(K) * 34.0f;
		UI.Text(Bars[K].first, R.X + 40.0f, By + 13.0f, Ts(14.0f, 600, KMuted));
		const float W = (R.W - 340.0f) * Nf(static_cast<double>(Bars[K].second.first) / static_cast<double>(Top));
		C->FillRoundRect({R.X + 170.0f, By + 2.0f, R.W - 340.0f, 14.0f}, 7.0f, KPanel2);
		if (W > 1.0f)
		{
			C->FillRoundRect({R.X + 170.0f, By + 2.0f, std::max(14.0f, W), 14.0f}, 7.0f, Bars[K].second.second);
		}
		UI.Text(Money(Bars[K].second.first), R.X + R.W - 40.0f, By + 14.0f, Ts(15.0f, 800, KInk, Align::Right, Baseline::Alphabetic, true));
	}
	UI.Text("Sent to your bank", R.X + 40.0f, R.Y + 450.0f, Ts(15.0f, 700, KMuted));
	UI.Text(Money(L.Total()), R.X + R.W - 40.0f, R.Y + 452.0f, Ts(30.0f, 900, KLime, Align::Right, Baseline::Alphabetic, true));
	if (!L.BestClip.empty())
	{
		UI.Text("Clip of the night: \"" + L.BestClip + "\"", R.X + 40.0f, R.Y + 490.0f, Ts(13.5f, 600, KViolet, Align::Left, Baseline::Alphabetic, false, R.W - 80.0f));
	}
	if (L.Trolls > 0)
	{
		UI.Text(std::to_string(L.Trolls) + " trolls showed up" + (S.Channel.Mods.empty() ? ": a couple of mods would help." : "."), R.X + 40.0f, R.Y + 514.0f, Ts(12.5f, 600, KDim));
	}
	if (AppButton("kastdone", {R.X + R.W - 200.0f, R.Y + R.H - 70.0f, 160.0f, 46.0f}, "Done", KViolet, Hex(0xffffff), true))
	{
		S.StreamCard = false;
	}
}

// ------------------------------------------------------------------ RiverLine while live

void RiverLine::LivePill(float X, float Y, double Now)
{
	const std::string L = KastCount(S.Stream.Viewers);
	const float W = UI.Measure(L, 13.0f, 900, true) + 80.0f;
	const Rect R{X - W, Y, W, 30.0f};
	const Ui::ClickState St = UI.Clickable("kastpill", R, !Previewing);
	C->FillRoundRect(R, 15.0f, St.Hover ? Hex(0x6d28d9) : Hex(0x2a1652));
	C->StrokeRoundRect(R, 15.0f, KViolet, 1.5f);
	C->FillCircle(R.X + 15.0f, R.Y + 15.0f, 4.5f, NetA(KRed, 0.6f + 0.4f * Nf(std::sin(Now * 4.0))));
	UI.Text("KAST", R.X + 24.0f, R.Y + 19.5f, Ts(11.0f, 900, Hex(0xd8ccff)));
	UI.Text(L, R.X + W - 12.0f, R.Y + 20.0f, Ts(13.0f, 900, Hex(0xffffff), Align::Right, Baseline::Alphabetic, true));
	if (St.Clicked)
	{
		OpenApp(App::Kast, Now);
	}
}

void RiverLine::StreamSide(const Rect& R, double Now)
{
	const kast::Stream& St = S.Stream;
	// A strip of numbers, then chat.
	const Rect Top{R.X, R.Y, R.W, 58.0f};
	C->FillRoundRect(Top, 12.0f, KPanel);
	C->FillCircle(Top.X + 16.0f, Top.Y + 20.0f, 4.5f, NetA(KRed, 0.6f + 0.4f * Nf(std::sin(Now * 4.0))));
	UI.Text(KastCount(St.Viewers), Top.X + 28.0f, Top.Y + 26.0f, Ts(18.0f, 900, KInk, Align::Left, Baseline::Alphabetic, true));
	UI.Text("viewers", Top.X + 34.0f + UI.Measure(KastCount(St.Viewers), 18.0f, 900, true), Top.Y + 26.0f, Ts(12.0f, 600, KMuted));
	UI.Text(KastClock(St.Uptime(World)), Top.X + Top.W - 14.0f, Top.Y + 26.0f, Ts(13.0f, 700, KMuted, Align::Right, Baseline::Alphabetic, true));
	UI.Text("+" + Grouped(St.Tonight.Follows) + " follows \xC2\xB7 " + Money(St.Tonight.Total()) + " tonight", Top.X + 16.0f, Top.Y + 46.0f, Ts(12.0f, 700, KLime));
	const float H = Nf(St.Hype / 100.0);
	C->FillRoundRect({Top.X + Top.W - 110.0f, Top.Y + 38.0f, 96.0f, 6.0f}, 3.0f, KPanel2);
	C->FillRoundRect({Top.X + Top.W - 110.0f, Top.Y + 38.0f, std::max(6.0f, 96.0f * H), 6.0f}, 3.0f, Paint::Linear({Top.X + Top.W - 110.0f, 0.0f}, {Top.X + Top.W - 14.0f, 0.0f}, Hex(0xfacc15), Hex(0xef4444)));
	StreamChat({R.X, R.Y + 66.0f, R.W, R.H - 66.0f}, Now, true);
}
} // namespace ui
} // namespace ss
