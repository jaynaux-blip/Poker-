// The player card: click a name anywhere on RiverLine and see who they are. Only what the poker world can see:
// results, titles, reputations, the people they run with, and what they think of you. Never a bankroll.
#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "RiverLineShared.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/UI/EventArt.h"

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
	CardTab = 0;
	CardTabAt = Now;
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
		Nx += NetPill(*C, "KAST \xC2\xB7 " + Grouped(P.Followers), Nx, R.Y + 40.0f, Hex(0xa855f7), true, 10.0f) + 8.0f;
	}
	if (P.New)
	{
		NetPill(*C, "NEW FACE", Nx, R.Y + 40.0f, pal::Accent, true, 10.0f);
	}
	// Their bracelets and rings on a shelf, the latest first (the TROPHIES tab has them all).
	if (!P.Awards.empty())
	{
		const size_t Shown = std::min<size_t>(P.Awards.size(), 4);
		float Sx = R.X + R.W - 30.0f;
		const float Sy = R.Y + 112.0f;
		if (P.Awards.size() > Shown)
		{
			Sx -= UI.Text("+" + std::to_string(P.Awards.size() - Shown), Sx, Sy + 6.0f, Ts(15.0f, 800, pal::Gold, Align::Right, Baseline::Alphabetic, true)) + 10.0f;
		}
		const float Left = Sx - Nf(Shown) * 52.0f;
		C->FillRoundRect({Left - 8.0f, Sy + 20.0f, Sx - Left + 12.0f, 5.0f}, 2.5f, Paint::Linear({Left, 0.0f}, {Sx, 0.0f}, Hex(0x3a2a0c), Hex(0x6b4e16)));
		for (size_t K = 0; K < Shown; ++K)
		{
			const world::Award& A = P.Awards[P.Awards.size() - 1 - K];
			const float X = Sx - 26.0f - Nf(K) * 52.0f;
			eventart::Trophy(*C, A, X, Sy, 50.0f, Now + 0.7 * static_cast<double>(K));
		}
		NetSpaced(*C, P.Bracelets > 0 && P.Rings > 0 ? "BRACELETS & RINGS" : P.Bracelets > 0 ? (P.Bracelets == 1 ? "BRACELET" : "BRACELETS") : (P.Rings == 1 ? "RING" : "RINGS"),
			Sx, Sy - 30.0f, 9.5f, 800, Hex(0xf28a3a), 1.6f, Align::Right);
	}
	const std::string Since = P.Arrived >= 0 ? "on RiverLine since " + net::DateLabel(P.Arrived) + ", " + std::to_string(world::YearOf(P.Arrived))
											 : "on the scene since " + std::to_string(P.Since);
	UI.Text(P.Known + "  \xC2\xB7  " + std::to_string(P.Age) + "  \xC2\xB7  " + Since, R.X + 142.0f, R.Y + 98.0f,
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

	// Middle and right: the overview (recent results, what they think of you, their story, who they run with), or the
	// journey (how they got here and every step since).
	const float Mx = R.X + 392.0f;
	const float Mw = R.X + R.W - 26.0f - Mx;
	{
		const std::string Tabs[3] = {"OVERVIEW", "JOURNEY", P.Awards.empty() ? std::string("TROPHIES") : "TROPHIES \xC2\xB7 " + std::to_string(P.Awards.size())};
		float Tx = Mx + Mw;
		for (int K = 2; K >= 0; --K)
		{
			const float Tw = UI.Measure(Tabs[K], 11.0f, 800) + 26.0f;
			Tx -= Tw;
			const Rect Tr{Tx, R.Y + 272.0f, Tw, 26.0f};
			const Ui::ClickState St = UI.Clickable("cardtab" + std::to_string(K), Tr);
			if (St.Clicked && CardTab != K)
			{
				CardTab = K;
				CardTabAt = Now;
			}
			const bool On = CardTab == K;
			UI.RRect(Tr, 13.0f, On ? NetA(pal::Accent, 0.18f) : St.Hover ? Hex(0x172a42) : Rgba(255, 255, 255, 0.02f), On ? pal::Accent : CardRule);
			NetSpaced(*C, Tabs[K], Tr.X + Tw / 2.0f, Tr.Y + 17.0f, 11.0f, 800, On ? CardInk : pal::Muted, 1.2f, Align::Center);
			Tx -= 8.0f;
		}
	}
	if (CardTab == 1 || CardTab == 2)
	{
		if (CardTab == 1)
		{
			CardJourney(P, Mx, R.Y + 292.0f, Mw, R.Y + R.H - 20.0f - (R.Y + 292.0f), Now);
		}
		else
		{
			CardTrophies(P, Mx, R.Y + 292.0f, Mw, R.Y + R.H - 20.0f - (R.Y + 292.0f), Now);
		}
		C->SetAlpha(A0);
		if (Outside.Clicked && !Inside.Hover)
		{
			CardShown = -1;
		}
		return;
	}
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
void RiverLine::CardTrophies(const world::Profile& P, float X, float Y, float W, float H, double Now)
{
	const float In = NetEase((Now - CardTabAt) / 0.4);
	NetSpaced(*C, "BRACELETS AND RINGS", X, Y, 11.0f, 800, pal::Muted, 1.6f);
	if (P.Awards.empty())
	{
		float Ty = NetParagraph(*C, "No bracelets or rings. Yet.", X, Y + 30.0f, W, 17.0f, 600, CardInk, 23.0f, 1);
		NetParagraph(*C,
			"Bracelets are won at The Championship (Las Vegas, every summer) and The Championship Online in June. Rings are won on the Grand Circuit, a stop every six "
			"to eight weeks, and at Ring Rush in March.",
			X, Ty + 22.0f, W, 13.5f, 500, pal::Muted, 20.0f, 4);
		return;
	}
	// The newest first: big cards for a few, a tighter grid for a collection.
	const bool Big = P.Awards.size() <= 6;
	const int Cols = Big ? 3 : 4;
	const float Gap = 12.0f;
	const float Cw = (W - Gap * Nf(Cols - 1)) / Nf(Cols);
	const float Ch = Big ? 228.0f : 150.0f;
	const int Rows = std::max(1, static_cast<int>((H - 20.0f + Gap) / (Ch + Gap)));
	const size_t Fit = static_cast<size_t>(Cols * Rows);
	const size_t Shown = std::min(P.Awards.size(), Fit);
	for (size_t K = 0; K < Shown; ++K)
	{
		const world::Award& A = P.Awards[P.Awards.size() - 1 - K];
		const float Ri = NetEase((Now - CardTabAt - 0.05 * static_cast<double>(K)) / 0.35);
		const float A0 = C->GetAlpha();
		C->SetAlpha(A0 * Ri);
		const Rect Cd{X + Nf(static_cast<int>(K) % Cols) * (Cw + Gap), Y + 18.0f + Nf(static_cast<int>(K) / Cols) * (Ch + Gap) + (1.0f - Ri) * 10.0f, Cw, Ch};
		const Color Tint = Hex(eventart::TrophyStone(A));
		C->FillRoundRect(Cd, 14.0f, Paint::Linear({0.0f, Cd.Y}, {0.0f, Cd.Y + Cd.H}, Mix(Hex(0x0f1b2e), Tint, 0.16f), Hex(0x0c1626)));
		C->StrokeRoundRect(Cd, 14.0f, A.Main ? NetA(pal::Gold, 0.7f) : CardRule, A.Main ? 1.5f : 1.0f);
		const float Ix = Cd.X + Cd.W / 2.0f;
		const float Iy = Cd.Y + (Big ? 64.0f : 46.0f);
		C->FillEllipse(Ix, Iy, Big ? 70.0f : 48.0f, Big ? 56.0f : 38.0f, Paint::Radial({Ix, Iy}, 0.0f, {Ix, Iy}, Big ? 70.0f : 48.0f, NetA(Tint, 0.28f), 0.5f, NetA(Tint, 0.08f), NetA(Tint, 0.0f)));
		eventart::Trophy(*C, A, Ix, Iy, (Big ? 108.0f : 76.0f) * (0.9f + 0.1f * In), Now + 0.6 * static_cast<double>(K));
		const float Ty = Cd.Y + (Big ? 132.0f : 98.0f);
		NetSpaced(*C, A.Main ? (A.Ring ? "MAIN EVENT RING" : "MAIN EVENT BRACELET") : A.Ring ? "RING" : "BRACELET", Ix, Ty, Big ? 9.5f : 8.5f, 900, A.Main ? pal::Gold : Hex(0xf28a3a), 1.5f, Align::Center);
		// The event's name, on two lines when it needs them.
		const std::string Event = eventart::TrophyEvent(A).empty() ? std::string(A.Ring ? "A ring" : "A bracelet") : eventart::TrophyEvent(A);
		const float Ts0 = Big ? 15.0f : 12.5f;
		const float Room = Cd.W - 20.0f;
		std::string First = Event;
		std::string Second;
		if (Big && UI.Measure(Event, Ts0, 800) > Room)
		{
			First.clear();
			size_t At = 0;
			while (At < Event.size())
			{
				const size_t Sp = Event.find(' ', At);
				const std::string Word = Event.substr(At, Sp == std::string::npos ? std::string::npos : Sp - At);
				const std::string Try = First.empty() ? Word : First + " " + Word;
				if (!First.empty() && UI.Measure(Try, Ts0, 800) > Room)
				{
					Second = Event.substr(At);
					break;
				}
				First = Try;
				At = Sp == std::string::npos ? Event.size() : Sp + 1;
			}
		}
		const float Off = Second.empty() ? 0.0f : 17.0f;
		UI.Text(First, Ix, Ty + (Big ? 21.0f : 18.0f), Ts(Ts0, 800, CardInk, Align::Center, Baseline::Alphabetic, false, Room));
		if (!Second.empty())
		{
			UI.Text(Second, Ix, Ty + 38.0f, Ts(Ts0, 800, CardInk, Align::Center, Baseline::Alphabetic, false, Room));
		}
		UI.Text(eventart::TrophySeries(A), Ix, Ty + (Big ? 40.0f : 34.0f) + Off, Ts(Big ? 12.0f : 10.5f, 600, CardSoft, Align::Center, Baseline::Alphabetic, false, Room));
		if (Big)
		{
			std::string When = A.Day > 0 ? net::DateLabel(A.Day) + ", " + std::to_string(world::YearOf(A.Day)) : std::string();
			if (A.Entries > 0)
			{
				When += (When.empty() ? "" : "  \xC2\xB7  ") + Grouped(A.Entries) + " entries";
			}
			UI.Text(When, Ix, Ty + 59.0f + Off, Ts(11.5f, 600, pal::Muted, Align::Center, Baseline::Alphabetic, false, Room));
			if (A.Prize > 0)
			{
				UI.Text(NetMoney(A.Prize), Ix, Ty + 78.0f + Off, Ts(13.0f, 800, pal::Gold, Align::Center, Baseline::Alphabetic, true));
			}
		}
		C->SetAlpha(A0);
	}
	if (P.Awards.size() > Shown)
	{
		UI.Text("and " + std::to_string(P.Awards.size() - Shown) + " more, won before these", X, Y + H - 2.0f, Ts(12.5f, 600, pal::Muted));
	}
}

void RiverLine::CardJourney(const world::Profile& P, float X, float Y, float W, float H, double Now)
{
	const float In = NetEase((Now - CardTabAt) / 0.4);
	const int Today = net::DayOf(S.WorldMinutes());
	NetSpaced(*C, "HOW THEY GOT HERE", X, Y, 11.0f, 800, pal::Muted, 1.6f);
	float Ty = NetParagraph(*C, P.Came, X, Y + 26.0f, W, 17.0f, 600, CardInk, 23.0f, 2);
	if (P.Arrived >= 0)
	{
		const int Ago = Today - P.Arrived;
		const std::string When = Ago <= 0 ? "today" : Ago == 1 ? "yesterday" : Ago < 60 ? std::to_string(Ago) + " days ago" : Ago < 730 ? std::to_string(Ago / 30) + " months ago"
																																: std::to_string(Ago / 365) + " years ago";
		UI.Text("Joined RiverLine " + net::DateLabel(P.Arrived) + ", " + std::to_string(world::YearOf(P.Arrived)) + "  \xC2\xB7  " + When, X, Ty + 18.0f,
			Ts(13.0f, 600, P.New ? pal::Accent : pal::Muted));
		Ty += 22.0f;
	}
	Ty += 24.0f;
	C->FillRect({X, Ty - 10.0f, W, 1.0f}, CardRule);
	NetSpaced(*C, "THE JOURNEY", X, Ty + 10.0f, 11.0f, 800, pal::Muted, 1.6f);
	Ty += 22.0f;
	// The timeline: the first steps always, then the latest (as many as fit).
	const float Row = 33.0f;
	const int Fit = std::max(1, static_cast<int>((Y + H - Ty) / Row));
	std::vector<const world::Profile::Moment*> Shown;
	const int Total = static_cast<int>(P.Journey.size());
	if (Total <= Fit)
	{
		for (const world::Profile::Moment& M : P.Journey)
		{
			Shown.push_back(&M);
		}
	}
	else
	{
		const int Head = std::min(4, Fit / 2);
		for (int K = 0; K < Head; ++K)
		{
			Shown.push_back(&P.Journey[static_cast<size_t>(K)]);
		}
		Shown.push_back(nullptr); // the gap
		for (int K = Total - (Fit - Head - 1); K < Total; ++K)
		{
			Shown.push_back(&P.Journey[static_cast<size_t>(K)]);
		}
	}
	const float Lx = X + 118.0f;
	if (!Shown.empty())
	{
		C->FillRect({Lx - 1.0f, Ty + 8.0f, 2.0f, Nf(Shown.size() - 1) * Row * In}, Hex(0x2b4a6e));
	}
	for (size_t K = 0; K < Shown.size(); ++K)
	{
		const float Ry = Ty + Nf(K) * Row;
		const float Ri = NetEase((Now - CardTabAt - 0.04 * static_cast<double>(K)) / 0.35);
		const float A0 = C->GetAlpha();
		C->SetAlpha(A0 * Ri);
		const world::Profile::Moment* M = Shown[K];
		if (!M)
		{
			for (int Dot = 0; Dot < 3; ++Dot)
			{
				C->FillCircle(Lx, Ry + 2.0f + Nf(Dot) * 6.0f, 1.8f, pal::Dim);
			}
			C->SetAlpha(A0);
			continue;
		}
		Color Col = Hex(0x5f7fa6);
		switch (M->Kind)
		{
		case world::StepKind::Joined: Col = pal::Accent; break;
		case world::StepKind::FirstEvent:
		case world::StepKind::FirstCash:
		case world::StepKind::FirstFinalTable:
		case world::StepKind::FirstLive: Col = Hex(0x27d3c3); break;
		case world::StepKind::FirstWin:
		case world::StepKind::FirstSeries:
		case world::StepKind::Major:
		case world::StepKind::BigScore:
		case world::StepKind::PlayerOfYear: Col = pal::Gold; break;
		case world::StepKind::Bracelet:
		case world::StepKind::Ring: Col = Hex(0xf28a3a); break;
		case world::StepKind::MovedUp:
		case world::StepKind::TurnedPro:
		case world::StepKind::Sponsored:
		case world::StepKind::StartedStreaming: Col = Hex(0x3b82f6); break;
		case world::StepKind::WentBroke:
		case world::StepKind::MovedDown: Col = pal::Red; break;
		case world::StepKind::Break:
		case world::StepKind::Retired: Col = pal::Dim; break;
		default: break;
		}
		UI.Text(net::DateLabel(M->Day), X + 100.0f, Ry + 13.0f, Ts(12.0f, 700, pal::Muted, Align::Right, Baseline::Alphabetic, true));
		UI.Text(std::to_string(world::YearOf(M->Day)), X + 100.0f, Ry + 27.0f, Ts(10.0f, 600, pal::Dim, Align::Right, Baseline::Alphabetic, true));
		C->FillCircle(Lx, Ry + 8.0f, 6.0f, Hex(0x0c1626));
		C->FillCircle(Lx, Ry + 8.0f, 4.5f, Col);
		UI.Text(M->Text, Lx + 18.0f, Ry + 13.0f, Ts(13.5f, 600, M->Kind == world::StepKind::Joined ? CardInk : CardSoft, Align::Left, Baseline::Alphabetic, false, W - 140.0f));
		C->SetAlpha(A0);
	}
}
} // namespace ui
} // namespace ss
