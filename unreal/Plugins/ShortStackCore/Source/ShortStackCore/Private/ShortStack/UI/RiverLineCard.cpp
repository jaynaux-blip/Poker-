// The player card: click a name anywhere on RiverLine and see who they are. Only what the poker world can see:
// results, titles, reputations, the people they run with, and what they think of you. Never a bankroll.
#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "RiverLineShared.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <cmath>

namespace ss
{
namespace ui
{
using namespace rlnet_detail;

namespace rlcard_detail
{
const Color CardInk = Hex(0xe6edf7);
const Color CardSoft = Hex(0xc3cedf);
const Color CardRule = Hex(0x223352);

Color RepColor(int K)
{
	static const uint32_t Cols[world::RepCount] = {0x27d3c3, 0xf2c14e, 0xef4d5a, 0xa855f7, 0x3b82f6, 0xe6edf7};
	return Hex(Cols[std::max(0, std::min(world::RepCount - 1, K))]);
}
} // namespace rlcard_detail

using namespace rlcard_detail;

void RiverLine::ShowPlayer(int Index, double Now)
{
	CardShown = Index;
	CardAt = Now;
}

void RiverLine::PlayerCard(double Now)
{
	const world::World& W = S.Living();
	const world::Npc* N = W.Get(CardShown);
	if (!N)
	{
		CardShown = -1;
		return;
	}
	const world::Profile P = W.ProfileOf(CardShown);
	const float In = NetEase((Now - CardAt) / 0.3);
	// Everything else waits: a click outside the card closes it.
	C->FillRect({0.0f, NetTop, NetW, RiverLine::Height - NetTop}, Rgba(4, 8, 16, 0.76f * In));
	const Ui::ClickState Outside = UI.Clickable("cardbackdrop", {0.0f, NetTop, NetW, RiverLine::Height - NetTop});
	const Rect R{NetW / 2.0f - 530.0f, 96.0f + (1.0f - In) * 22.0f, 1060.0f, 806.0f};
	const Ui::ClickState Inside = UI.Clickable("cardbody", R);
	const float A0 = C->GetAlpha();
	C->SetAlpha(A0 * In);
	C->GlowRoundRect({R.X, R.Y + 8.0f, R.W, R.H}, 22.0f, Rgba(0, 0, 0, 0.5f), 36.0f);
	UI.RRect(R, 22.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Hex(0x13223a), Hex(0x0c1626)), CardRule);
	const Color Ring = P.Rival ? Hex(0xb36bff) : P.Pro ? pal::Gold : P.Bond.empty() ? CardRule : pal::Accent;
	C->FillRoundRect({R.X, R.Y, R.W, 6.0f}, 3.0f, Ring);

	// Who: the face, the name, what the world calls them.
	NetAvatar(*C, R.X + 76.0f, R.Y + 84.0f, 46.0f, P.Name, Ring);
	NetFlag(*C, P.Country, R.X + 140.0f, R.Y + 44.0f, 30.0f, 20.0f);
	float Nx = R.X + 180.0f;
	Nx += UI.Text(P.Name, Nx, R.Y + 64.0f, Ts(34.0f, 900, P.Rival ? Hex(0xd5b8ff) : CardInk, Align::Left, Baseline::Alphabetic, false, 520.0f)) + 14.0f;
	if (P.Rival)
	{
		Nx += NetPill(*C, "RIVAL", Nx, R.Y + 40.0f, Hex(0xb36bff), true, 10.0f) + 8.0f;
	}
	if (P.Pro)
	{
		Nx += NetPill(*C, "TEAM RIVERLINE", Nx, R.Y + 40.0f, pal::Gold, false, 10.0f) + 8.0f;
	}
	else if (!P.Sponsor.empty())
	{
		Nx += NetPill(*C, NetUpper(P.Sponsor), Nx, R.Y + 40.0f, pal::Gold, false, 10.0f) + 8.0f;
	}
	if (P.Streams)
	{
		NetPill(*C, "KAST \xC2\xB7 " + Grouped(P.Followers), Nx, R.Y + 40.0f, Hex(0xa855f7), true, 10.0f);
	}
	UI.Text(P.Known + "  \xC2\xB7  " + std::to_string(P.Age) + "  \xC2\xB7  on the scene since " + std::to_string(P.Since), R.X + 142.0f, R.Y + 98.0f,
		Ts(16.0f, 600, CardSoft, Align::Left, Baseline::Alphabetic, false, 640.0f));
	UI.Text(P.Stakes + "  \xC2\xB7  " + P.Live + "  \xC2\xB7  " + P.Style, R.X + 142.0f, R.Y + 124.0f, Ts(14.0f, 500, pal::Muted, Align::Left, Baseline::Alphabetic, false, 700.0f));
	// Status and the close button.
	{
		const Color Sc = P.Status == "Active" ? pal::Green : P.Status == "Retired" ? pal::Muted : pal::Orange;
		NetPill(*C, NetUpper(P.Status), R.X + R.W - 170.0f, R.Y + 34.0f, Sc, false, 10.0f);
		const Rect X{R.X + R.W - 56.0f, R.Y + 22.0f, 36.0f, 36.0f};
		const Ui::ClickState Cx = UI.Clickable("cardclose", X);
		C->FillCircle(X.X + 18.0f, X.Y + 18.0f, 18.0f, Cx.Hover ? Hex(0x223352) : Hex(0x162540));
		C->StrokePolyline({{X.X + 12.0f, X.Y + 12.0f}, {X.X + 24.0f, X.Y + 24.0f}}, false, CardSoft, 2.0f, true);
		C->StrokePolyline({{X.X + 24.0f, X.Y + 12.0f}, {X.X + 12.0f, X.Y + 24.0f}}, false, CardSoft, 2.0f, true);
		if (Cx.Clicked)
		{
			CardShown = -1;
		}
	}

	// The record: six numbers.
	{
		const world::Ledger& On = P.Totals[0];
		const world::Ledger& Lv = P.Totals[1];
		struct Tile
		{
			std::string Value;
			const char* Label;
			Color Col;
		};
		const Tile Tiles[6] = {
			{net::MoneyShort(On.Won), "ONLINE WINNINGS", pal::Accent},
			{net::MoneyShort(Lv.Won), "LIVE WINNINGS", pal::Gold},
			{Grouped(On.Wins + Lv.Wins), "TITLES", CardInk},
			{Grouped(On.FinalTables + Lv.FinalTables), "FINAL TABLES", CardInk},
			{net::MoneyShort(P.Best), "BIGGEST SCORE", pal::Gold},
			{std::to_string(P.Bracelets) + " \xC2\xB7 " + std::to_string(P.Rings), "BRACELETS \xC2\xB7 RINGS", Hex(0xf28a3a)},
		};
		const float Tw = (R.W - 48.0f - 5.0f * 10.0f) / 6.0f;
		for (int K = 0; K < 6; ++K)
		{
			const Rect T{R.X + 24.0f + Nf(K) * (Tw + 10.0f), R.Y + 152.0f, Tw, 78.0f};
			C->FillRoundRect(T, 12.0f, Hex(0x0f1b2e));
			C->StrokeRoundRect(T, 12.0f, CardRule, 1.0f);
			UI.Text(Tiles[K].Value, T.X + 14.0f, T.Y + 40.0f, Ts(24.0f, 800, Tiles[K].Col, Align::Left, Baseline::Alphabetic, true, Tw - 20.0f));
			NetSpaced(*C, Tiles[K].Label, T.X + 14.0f, T.Y + 62.0f, 9.5f, 800, pal::Muted, 1.2f);
		}
		if (!P.BestEvent.empty())
		{
			UI.Text("Biggest score: " + P.BestEvent + " (" + net::DateLabel(P.BestDay) + ", " + std::to_string(world::YearOf(P.BestDay)) + ")", R.X + 26.0f, R.Y + 254.0f,
				Ts(13.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, R.W - 52.0f));
		}
	}

	// Left: reputations, form, the years.
	const float Lx = R.X + 26.0f;
	const float Lw = 330.0f;
	float Y = R.Y + 292.0f;
	NetSpaced(*C, "REPUTATION", Lx, Y, 11.0f, 800, pal::Muted, 1.6f);
	Y += 14.0f;
	for (int K = 0; K < world::RepCount; ++K)
	{
		const int V = P.Reps[static_cast<size_t>(K)];
		const float By = Y + Nf(K) * 30.0f;
		UI.Text(world::RepName(static_cast<world::Rep>(K)), Lx, By + 17.0f, Ts(13.0f, 600, CardSoft));
		const Rect Bar{Lx + 108.0f, By + 8.0f, Lw - 150.0f, 9.0f};
		C->FillRoundRect(Bar, 4.5f, Hex(0x1a2a44));
		if (V > 0)
		{
			C->FillRoundRect({Bar.X, Bar.Y, std::max(9.0f, Bar.W * Nf(V) / 100.0f), Bar.H}, 4.5f, RepColor(K));
		}
		UI.Text(std::to_string(V), Lx + Lw, By + 17.0f, Ts(13.0f, 800, V > 0 ? CardInk : pal::Dim, Align::Right, Baseline::Alphabetic, true));
	}
	Y += 6.0f * 30.0f + 22.0f;
	// Form: board points the last eight weeks.
	NetSpaced(*C, "FORM (8 WEEKS)", Lx, Y, 11.0f, 800, pal::Muted, 1.6f);
	{
		float Top = 1.0f;
		for (float F : P.Form)
		{
			Top = std::max(Top, F);
		}
		const float Bw = (Lw - 7.0f * 6.0f) / 8.0f;
		for (size_t K = 0; K < P.Form.size(); ++K)
		{
			const float H = std::max(3.0f, 46.0f * P.Form[K] / Top);
			C->FillRoundRect({Lx + Nf(K) * (Bw + 6.0f), Y + 62.0f - H, Bw, H}, 3.0f, K + 1 == P.Form.size() ? pal::Accent : Hex(0x2b4a6e));
		}
	}
	Y += 86.0f;
	NetSpaced(*C, "BY YEAR", Lx, Y, 11.0f, 800, pal::Muted, 1.6f);
	{
		const size_t First = P.Years.size() > 6 ? P.Years.size() - 6 : 0;
		Chips Top = 1;
		for (size_t K = First; K < P.Years.size(); ++K)
		{
			Top = std::max(Top, P.Years[K].Online + P.Years[K].Live);
		}
		for (size_t K = First; K < P.Years.size(); ++K)
		{
			const world::Year& Yr = P.Years[K];
			const float Ry = Y + 14.0f + Nf(K - First) * 24.0f;
			UI.Text(std::to_string(Yr.Number), Lx, Ry + 14.0f, Ts(12.0f, 700, pal::Muted, Align::Left, Baseline::Alphabetic, true));
			const float Full = Lw - 150.0f;
			const float Wd = std::max(2.0f, Full * Nf(static_cast<double>(Yr.Online + Yr.Live) / static_cast<double>(Top)));
			C->FillRoundRect({Lx + 46.0f, Ry + 5.0f, Wd, 10.0f}, 3.0f, Yr.Live > Yr.Online ? pal::Gold : pal::Accent);
			UI.Text(net::MoneyShort(Yr.Online + Yr.Live) + (Yr.Wins > 0 ? "  \xC2\xB7  " + std::to_string(Yr.Wins) + (Yr.Wins == 1 ? " win" : " wins") : ""), Lx + 54.0f + Wd, Ry + 14.0f,
				Ts(12.0f, 600, CardSoft, Align::Left, Baseline::Alphabetic, false, Lx + Lw - (Lx + 54.0f + Wd) + 40.0f));
		}
	}

	// Middle and right: recent results, what they think of you, their story, who they run with.
	const float Mx = R.X + 392.0f;
	const float Mw = R.X + R.W - 26.0f - Mx;
	float My = R.Y + 292.0f;
	NetSpaced(*C, "RECENT RESULTS", Mx, My, 11.0f, 800, pal::Muted, 1.6f);
	My += 10.0f;
	if (P.Recent.empty())
	{
		UI.Text("Nothing worth writing home about lately.", Mx, My + 22.0f, Ts(14.0f, 500, pal::Dim));
		My += 34.0f;
	}
	for (size_t K = 0; K < P.Recent.size() && K < 6; ++K)
	{
		const world::Finish& F = P.Recent[K];
		const float Ry = My + Nf(K) * 30.0f;
		UI.Text(net::DateLabel(F.Day), Mx, Ry + 20.0f, Ts(12.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, true));
		const std::string Place = Ordinal(F.Place) + " / " + Grouped(F.Entries);
		UI.Text(Place, Mx + 62.0f, Ry + 20.0f, Ts(13.0f, 800, F.Place == 1 ? pal::Gold : F.Place <= 9 ? CardInk : CardSoft, Align::Left, Baseline::Alphabetic, true));
		UI.Text(F.Event, Mx + 170.0f, Ry + 20.0f, Ts(13.0f, 600, F.Major ? CardInk : CardSoft, Align::Left, Baseline::Alphabetic, false, Mw - 280.0f));
		UI.Text(F.Seat > 0 ? "Seat \xC2\xB7 " + NetMoney(F.Seat) : NetMoney(F.Prize), Mx + Mw, Ry + 20.0f,
			Ts(13.0f, 800, F.Prize > 0 || F.Seat > 0 ? pal::Gold : pal::Dim, Align::Right, Baseline::Alphabetic, true));
		if (F.Where == 1)
		{
			NetPill(*C, "LIVE", Mx + Mw - 120.0f, Ry + 6.0f, pal::Gold, false, 8.5f);
		}
	}
	My += Nf(std::min<size_t>(P.Recent.size(), 6)) * 30.0f + 26.0f;
	C->FillRect({Mx, My - 8.0f, Mw, 1.0f}, CardRule);
	// What they think of you, and why.
	const float Half = (Mw - 24.0f) / 2.0f;
	float Ay = My + 12.0f;
	NetSpaced(*C, "ABOUT YOU", Mx, Ay, 11.0f, 800, pal::Muted, 1.6f);
	Ay += 10.0f;
	if (P.Bond.empty() && P.Memories.empty())
	{
		Ay = NetParagraph(*C, "You haven't crossed paths yet.", Mx, Ay + 18.0f, Half, 14.0f, 500, pal::Dim, 20.0f, 2);
	}
	else
	{
		if (!P.Bond.empty())
		{
			NetPill(*C, NetUpper(P.Bond), Mx, Ay + 4.0f, P.Bond == "Rival" || P.Bond == "Holds a grudge" ? pal::Red : pal::Accent, true, 10.0f);
			Ay += 30.0f;
		}
		for (size_t K = 0; K < P.Memories.size() && K < 4; ++K)
		{
			Ay = NetParagraph(*C, P.Memories[K], Mx, Ay + 16.0f, Half, 13.0f, 500, CardSoft, 18.0f, 2) - 2.0f;
		}
	}
	// Their story, and the people they're known to run with.
	const float Rx = Mx + Half + 24.0f;
	float By = My + 12.0f;
	NetSpaced(*C, "STORY", Rx, By, 11.0f, 800, pal::Muted, 1.6f);
	By += 8.0f;
	if (P.Story.empty())
	{
		By = NetParagraph(*C, "No headlines. Yet.", Rx, By + 18.0f, Half, 14.0f, 500, pal::Dim, 20.0f, 1);
	}
	for (size_t K = 0; K < P.Story.size() && K < 4; ++K)
	{
		By = NetParagraph(*C, P.Story[K], Rx, By + 16.0f, Half, 13.0f, 500, CardSoft, 18.0f, 2) - 2.0f;
	}
	if (!P.Knows.empty())
	{
		By += 18.0f;
		NetSpaced(*C, "RUNS WITH", Rx, By, 11.0f, 800, pal::Muted, 1.6f);
		for (size_t K = 0; K < P.Knows.size() && K < 3; ++K)
		{
			UI.Text(P.Knows[K].first + ": " + P.Knows[K].second, Rx, By + 22.0f + Nf(K) * 20.0f, Ts(13.0f, 600, CardSoft, Align::Left, Baseline::Alphabetic, false, Half));
		}
	}
	(void)Ay;
	C->SetAlpha(A0);
	if (Outside.Clicked && !Inside.Hover)
	{
		CardShown = -1;
	}
}
} // namespace ui
} // namespace ss
