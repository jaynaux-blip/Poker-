// The player card: click a name anywhere on RiverLine and see who they are. Only what the poker world can see:
// results, titles, reputations, the people they run with, and what they think of you. Never a bankroll.
#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "RiverLineShared.h"

#include "ShortStack/Game/Badges.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/UI/BadgeArt.h"
#include "ShortStack/UI/EventArt.h"

#include <algorithm>
#include <cmath>
#include <map>

namespace ss
{
namespace ui
{
using namespace rlnet_detail;

namespace rlcard_detail
{
// The card's inks, read each draw: a card opened on a rival's board wears its skin (update 2 §1.8).
Color CardInk()
{
	return pal::Ink;
}
Color CardSoft()
{
	return rlsites_detail::SitesTone(0xc3cedf);
}
Color CardRule()
{
	return pal::Line;
}

/** Where someone arrived (update 2 §5.9): RiverLine, or the site their journey's first step names (Ignite's own arrivals). */
std::string CardJoinedSite(const world::Npc* N)
{
	return N && !N->Path.empty() && N->Path.front().Kind == world::StepKind::Joined && !N->Path.front().What.empty() ? N->Path.front().What : std::string("RiverLine");
}

Color RepColor(int K)
{
	static const uint32_t Cols[world::RepCount] = {0x27d3c3, 0xf2c14e, 0xef4d5a, 0xa855f7, 0x3b82f6, 0xe6edf7};
	return Hex(Cols[std::max(0, std::min(world::RepCount - 1, K))]);
}
} // namespace rlcard_detail

// The badges' shared pieces (RiverLineShared.h): the card's rails here, the hero's stats card and Career's too.
namespace rlnet_detail
{
std::vector<badge::Badge> NetCardBadges(const badge::Set& Held, int Max, uint8_t Frame)
{
	std::vector<badge::Badge> Row = NetBadgeRow(Held, badge::Card, Frame, Max);
	// The card's order covers every kind, so an empty row under a frame means the frame says it all: nothing to add back.
	for (size_t K = 0; Row.empty() && Frame == badge::FrameNone && K < Held.Held.size() && static_cast<int>(K) < Max; ++K)
	{
		if (Held.Held[K].K != badge::Kind::Count)
		{
			Row.push_back(Held.Held[K]);
		}
	}
	return Row;
}

float NetBadgeRail(Canvas& Cv, Ui& UI, const RiverLine* RL, const std::vector<badge::Badge>& Row, int Npc, float X, float Cy, float Size, int Labelled, float MaxW, double Now,
	int* Drawn)
{
	int Count = 0;
	const float LabelSize = Size * 0.5f;
	auto WidthWith = [&](int Labels) {
		float W = 0.0f;
		for (size_t K = 0; K < Row.size(); ++K)
		{
			W += Size + (K + 1 < Row.size() ? 8.0f : 0.0f);
			const std::string L = static_cast<int>(K) < Labels ? badge::Label(Row[K]) : std::string();
			W += L.empty() ? 0.0f : Cv.Measure(L, LabelSize, 800) + 1.2f * Nf(L.size()) + 6.0f;
		}
		return W;
	};
	int Labels = Labelled;
	while (Labels > 0 && WidthWith(Labels) > MaxW)
	{
		--Labels;
	}
	float Xx = X;
	for (size_t K = 0; K < Row.size(); ++K)
	{
		if (Xx + Size > X + MaxW)
		{
			break;
		}
		badgeart::Draw(Cv, Row[K], Xx + Size / 2.0f, Cy, Size, Now);
		++Count;
		if (UI.Hover({Xx - 2.0f, Cy - Size / 2.0f - 2.0f, Size + 4.0f, Size + 4.0f}))
		{
			NetTip& Tip = NetTipOf(RL);
			Tip.Up = true;
			Tip.B = Row[K];
			Tip.Npc = Npc;
			Tip.X = Xx + Size / 2.0f;
			Tip.Y = Cy + Size / 2.0f;
		}
		Xx += Size;
		const std::string L = static_cast<int>(K) < Labels ? badge::Label(Row[K]) : std::string();
		if (!L.empty())
		{
			Xx += 6.0f + NetSpaced(Cv, L, Xx + 6.0f, Cy + LabelSize * 0.36f, LabelSize, 800, rlcard_detail::CardSoft(), 1.2f);
		}
		Xx += 8.0f;
	}
	if (Drawn)
	{
		*Drawn = Count;
	}
	return Count == 0 ? 0.0f : Xx - 8.0f - X;
}

badge::Holder NetHolderOf(const Session& S, int Npc)
{
	if (Npc == -1)
	{
		return badge::OfHero(S);
	}
	return Npc >= 0 && S.Living().Get(Npc) ? badge::Of(S.Living(), Npc, net::DayOf(S.WorldMinutes())) : badge::Holder();
}
} // namespace rlnet_detail

namespace rlcard_detail
{
/** The trophies' share of the TROPHIES & BADGES tab when there are badges under them: one row of the tighter grid. */
const float CardTrophyRowH = 196.0f;

/**
 * TROPHIES & BADGES (§5.6): under the trophies, a tile for every badge held: the glyph at 64, its title, how it was
 * earned, and the year when it has one. Two columns, as many rows as fit; the rest counted.
 */
void CardBadgeTiles(Canvas& Cv, Ui& UI, const Session& S, int Npc, const badge::Set& Held, uint8_t Frame, float X, float Y, float W, float H, bool HasAwards, double TabAt,
	double Now)
{
	// Not what the avatar's frame already says (the trophies above show the bracelets and rings).
	const std::vector<badge::Badge> All = NetCardBadges(Held, 32, Frame);
	if (All.empty())
	{
		return;
	}
	const float Top = HasAwards ? Y + CardTrophyRowH + 20.0f : Y + 66.0f;
	NetSpaced(Cv, "BADGES", X, Top, 11.0f, 800, pal::Muted, 1.6f);
	const badge::Holder Who = NetHolderOf(S, Npc);
	const float Gap = 10.0f;
	const float Tw = (W - Gap) / 2.0f;
	const float Th = 92.0f;
	const int Rows = std::max(1, static_cast<int>((Y + H - (Top + 14.0f) + Gap) / (Th + Gap)));
	const size_t Fit = static_cast<size_t>(Rows * 2);
	const size_t Shown = All.size() > Fit ? Fit - 1 : All.size();
	for (size_t K = 0; K < Shown; ++K)
	{
		const badge::Badge& B = All[K];
		const float Ri = NetEase((Now - TabAt - 0.05 * static_cast<double>(K)) / 0.35);
		const float A0 = Cv.GetAlpha();
		Cv.SetAlpha(A0 * Ri);
		const Rect T{X + Nf(K % 2) * (Tw + Gap), Top + 14.0f + Nf(K / 2) * (Th + Gap) + (1.0f - Ri) * 8.0f, Tw, Th};
		Cv.FillRoundRect(T, 12.0f, rlsites_detail::SitesTone(0x0f1b2e));
		Cv.StrokeRoundRect(T, 12.0f, CardRule(), 1.0f);
		badgeart::Draw(Cv, B, T.X + 44.0f, T.Y + T.H / 2.0f, 64.0f, Now);
		const std::string Title = badge::Title(B);
		const float Yw = B.Year > 0 ? UI.Text(std::to_string(B.Year), T.X + T.W - 14.0f, T.Y + 26.0f, Ts(12.0f, 700, pal::Muted, Align::Right, Baseline::Alphabetic, true)) + 10.0f : 0.0f;
		UI.Text(Title.empty() ? badge::Label(B) : Title, T.X + 86.0f, T.Y + 26.0f, Ts(15.0f, 800, CardInk(), Align::Left, Baseline::Alphabetic, false, T.W - 100.0f - Yw));
		NetParagraph(Cv, badge::HowEarned(B, Who), T.X + 86.0f, T.Y + 46.0f, T.W - 100.0f, 12.5f, 500, CardSoft(), 16.0f, 3);
		Cv.SetAlpha(A0);
	}
	if (All.size() > Shown)
	{
		const Rect T{X + Nf(Shown % 2) * (Tw + Gap), Top + 14.0f + Nf(Shown / 2) * (Th + Gap), Tw, Th};
		Cv.StrokeRoundRect(T, 12.0f, CardRule(), 1.0f);
		UI.Text("+" + std::to_string(All.size() - Shown) + " more", T.X + T.W / 2.0f, T.Y + T.H / 2.0f + 1.0f, Ts(15.0f, 800, CardSoft(), Align::Center, Baseline::Middle));
	}
}
} // namespace rlcard_detail

namespace rlnet_detail
{
std::map<const RiverLine*, NetTip>& TipMap()
{
	static std::map<const RiverLine*, NetTip> ByLaptop;
	return ByLaptop;
}

std::map<const RiverLine*, NetChoices>& ChoiceMap()
{
	static std::map<const RiverLine*, NetChoices> ByLaptop;
	return ByLaptop;
}

NetTip& NetTipOf(const RiverLine* RL)
{
	return TipMap()[RL];
}

NetChoices& NetChoicesOf(const RiverLine* RL)
{
	return ChoiceMap()[RL];
}

void ForgetNet(const RiverLine* RL)
{
	TipMap().erase(RL);
	ChoiceMap().erase(RL);
}
} // namespace rlnet_detail

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
	if (CardShown == HeroCard)
	{
		HeroStatsCard(Now);
		return;
	}
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
	UI.RRect(R, 22.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, rlsites_detail::SitesTone(0x13223a), rlsites_detail::SitesTone(0x0c1626)), CardRule());
	const Color Ring = P.Rival ? Hex(0xb36bff) : P.Pro ? pal::Gold : P.Bond.empty() ? CardRule() : pal::Accent;
	C->FillRoundRect({R.X, R.Y, R.W, 6.0f}, 3.0f, Ring);

	// Who: the face, the name, what the world calls them.
	const AvatarSpec Pic = NetAvatar(*C, R.X + 76.0f, R.Y + 84.0f, 46.0f, P.Name, Ring);
	// A champion's frame is their bracelet or ring (C2): the rail and the tiles leave it out (§9.1, no second bracelet).
	const uint8_t Frame = NetBadgeFrame(Pic);
	NetFlag(*C, P.Country, R.X + 140.0f, R.Y + 44.0f, 30.0f, 20.0f);
	float Nx = R.X + 180.0f;
	Nx += UI.Text(P.Name, Nx, R.Y + 64.0f, Ts(34.0f, 900, P.Rival ? Hex(0xd5b8ff) : CardInk(), Align::Left, Baseline::Alphabetic, false, 520.0f)) + 14.0f;
	// RIVAL, then the badge rail (§5.6): 22 units, the top three labelled, "+N" past six (it opens TROPHIES & BADGES).
	// The status pill and the close button keep the right end.
	if (P.Rival)
	{
		Nx += NetPill(*C, "RIVAL", Nx, R.Y + 40.0f, Hex(0xb36bff), true, 10.0f) + 8.0f;
	}
	const badge::Set& Held = NetBadgesOf(S, CardShown);
	const std::vector<badge::Badge> Shows = NetCardBadges(Held, 32, Frame);
	const std::vector<badge::Badge> Rail(Shows.begin(), Shows.begin() + static_cast<std::ptrdiff_t>(std::min<size_t>(Shows.size(), 6)));
	const float RailEnd = R.X + R.W - 186.0f;
	int OnRail = 0;
	const float RailW = NetBadgeRail(*C, UI, this, Rail, CardShown, Nx, R.Y + 50.0f, 22.0f, 3, RailEnd - Nx - 40.0f, Now, &OnRail);
	Nx += RailW > 0.0f ? RailW + 10.0f : 0.0f;
	// "+N": everything held that the rail didn't show (past six, or squeezed out by a long name); the frame shows its own.
	const size_t Total = Shows.size();
	if (OnRail > 0 && Total > static_cast<size_t>(OnRail))
	{
		const std::string More = "+" + std::to_string(Total - static_cast<size_t>(OnRail));
		const float Mw = UI.Measure(More, 13.0f, 800) + 14.0f;
		const Ui::ClickState Mc = UI.Clickable("cardmorebadges", {Nx, R.Y + 38.0f, Mw, 24.0f});
		UI.RRect({Nx, R.Y + 38.0f, Mw, 24.0f}, 12.0f, Mc.Hover ? pal::Line : rlsites_detail::SitesTone(0x162540), CardRule());
		UI.Text(More, Nx + Mw / 2.0f, R.Y + 51.0f, Ts(13.0f, 800, CardSoft(), Align::Center, Baseline::Middle));
		if (Mc.Clicked && CardTab != 2)
		{
			CardTab = 2;
			CardTabAt = Now;
		}
		Nx += Mw + 8.0f;
	}
	if (Rail.empty())
	{
		// No badges to show (none held, or the rules not in yet): what the world says about them, as before.
		if (P.Pro)
		{
			Nx += NetPill(*C, "TEAM RIVERLINE", Nx, R.Y + 40.0f, pal::Gold, false, 10.0f) + 8.0f;
		}
		else if (!P.Sponsor.empty())
		{
			Nx += NetPill(*C, NetUpper(P.Sponsor), Nx, R.Y + 40.0f, pal::Gold, false, 10.0f) + 8.0f;
		}
		if (P.New)
		{
			NetPill(*C, "NEW FACE", Nx, R.Y + 40.0f, pal::Accent, true, 10.0f);
		}
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
			Sx, Sy - 30.0f, 9.5f, 800, pal::Orange, 1.6f, Align::Right);
	}
	const std::string Since = P.Arrived >= 0 ? "on " + CardJoinedSite(N) + " since " + net::DateLabel(P.Arrived) + ", " + std::to_string(world::YearOf(P.Arrived))
											 : "on the scene since " + std::to_string(P.Since);
	UI.Text(P.Known + "  \xC2\xB7  " + std::to_string(P.Age) + "  \xC2\xB7  " + Since, R.X + 142.0f, R.Y + 98.0f,
		Ts(16.0f, 600, CardSoft(), Align::Left, Baseline::Alphabetic, false, 640.0f));
	UI.Text(P.Stakes + "  \xC2\xB7  " + P.Live + "  \xC2\xB7  " + P.Style + (P.Streams ? "  \xC2\xB7  Kast, " + Grouped(P.Followers) + " followers" : std::string()), R.X + 142.0f, R.Y + 124.0f,
		Ts(14.0f, 500, pal::Muted, Align::Left, Baseline::Alphabetic, false, 700.0f));
	// Status and the close button.
	{
		const Color Sc = P.Status == "Active" ? pal::Green : P.Status == "Retired" ? pal::Muted : pal::Orange;
		NetPill(*C, NetUpper(P.Status), R.X + R.W - 170.0f, R.Y + 34.0f, Sc, false, 10.0f);
		const Rect X{R.X + R.W - 56.0f, R.Y + 22.0f, 36.0f, 36.0f};
		const Ui::ClickState Cx = UI.Clickable("cardclose", X);
		C->FillCircle(X.X + 18.0f, X.Y + 18.0f, 18.0f, Cx.Hover ? pal::Line : rlsites_detail::SitesTone(0x162540));
		C->StrokePolyline({{X.X + 12.0f, X.Y + 12.0f}, {X.X + 24.0f, X.Y + 24.0f}}, false, CardSoft(), 2.0f, true);
		C->StrokePolyline({{X.X + 24.0f, X.Y + 12.0f}, {X.X + 12.0f, X.Y + 24.0f}}, false, CardSoft(), 2.0f, true);
		if (Cx.Clicked)
		{
			CardShown = -1;
		}
	}

	const bool StatsTab = CardTab == 3;
	if (StatsTab)
	{
		// RiverLine Stats: the numbers up top, the dashboard below.
		StatsKpis(P.Stats, {R.X + 24.0f, R.Y + 150.0f, R.W - 48.0f, 104.0f}, Now);
		StatsBrand(R.X + 26.0f, R.Y + 290.0f);
	}
	else
	{
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
				{Grouped(On.Wins + Lv.Wins), "TITLES", CardInk()},
				{Grouped(On.FinalTables + Lv.FinalTables), "FINAL TABLES", CardInk()},
				{net::MoneyShort(P.Best), "BIGGEST SCORE", pal::Gold},
				{std::to_string(P.Bracelets) + " \xC2\xB7 " + std::to_string(P.Rings), "BRACELETS \xC2\xB7 RINGS", pal::Orange},
			};
			const float Tw = (R.W - 48.0f - 5.0f * 10.0f) / 6.0f;
			for (int K = 0; K < 6; ++K)
			{
				const Rect T{R.X + 24.0f + Nf(K) * (Tw + 10.0f), R.Y + 152.0f, Tw, 78.0f};
				C->FillRoundRect(T, 12.0f, rlsites_detail::SitesTone(0x0f1b2e));
				C->StrokeRoundRect(T, 12.0f, CardRule(), 1.0f);
				UI.Text(Tiles[K].Value, T.X + 14.0f, T.Y + 40.0f, Ts(24.0f, 800, Tiles[K].Col, Align::Left, Baseline::Alphabetic, true, Tw - 20.0f));
				NetSpaced(*C, Tiles[K].Label, T.X + 14.0f, T.Y + 62.0f, 9.5f, 800, pal::Muted, 1.2f);
			}
			if (!P.BestEvent.empty())
			{
				UI.Text("Biggest score: " + P.BestEvent + " (" + net::DateLabel(P.BestDay) + ", " + std::to_string(world::YearOf(P.BestDay)) + ")", R.X + 26.0f, R.Y + 254.0f,
					Ts(13.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, R.W - 52.0f));
			}
		}

	}

	if (!StatsTab)
	{
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
			UI.Text(world::RepName(static_cast<world::Rep>(K)), Lx, By + 17.0f, Ts(13.0f, 600, CardSoft()));
			const Rect Bar{Lx + 108.0f, By + 8.0f, Lw - 150.0f, 9.0f};
			C->FillRoundRect(Bar, 4.5f, rlsites_detail::SitesTone(0x1a2a44));
			if (V > 0)
			{
				C->FillRoundRect({Bar.X, Bar.Y, std::max(9.0f, Bar.W * Nf(V) / 100.0f), Bar.H}, 4.5f, RepColor(K));
			}
			UI.Text(std::to_string(V), Lx + Lw, By + 17.0f, Ts(13.0f, 800, V > 0 ? CardInk() : pal::Dim, Align::Right, Baseline::Alphabetic, true));
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
				C->FillRoundRect({Lx + Nf(K) * (Bw + 6.0f), Y + 62.0f - H, Bw, H}, 3.0f, K + 1 == P.Form.size() ? pal::Accent : rlsites_detail::SitesTone(0x2b4a6e));
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
					Ts(12.0f, 600, CardSoft(), Align::Left, Baseline::Alphabetic, false, Lx + Lw - (Lx + 54.0f + Wd) + 40.0f));
			}
		}

	}

	// Middle and right: the overview (recent results, what they think of you, their story, who they run with), or the
	// journey (how they got here and every step since).
	const float Mx = R.X + 392.0f;
	const float Mw = R.X + R.W - 26.0f - Mx;
	{
		const size_t Earned = P.Awards.size() + Held.Held.size();
		const std::string Tabs[4] = {"OVERVIEW", "JOURNEY", Earned == 0 ? std::string("TROPHIES & BADGES") : "TROPHIES & BADGES \xC2\xB7 " + std::to_string(Earned), "STATS"};
		float Tx = Mx + Mw;
		for (int K = 3; K >= 0; --K)
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
			UI.RRect(Tr, 13.0f, On ? NetA(pal::Accent, 0.18f) : St.Hover ? rlsites_detail::SitesTone(0x172a42) : Rgba(255, 255, 255, 0.02f), On ? pal::Accent : CardRule());
			NetSpaced(*C, Tabs[K], Tr.X + Tw / 2.0f, Tr.Y + 17.0f, 11.0f, 800, On ? CardInk() : pal::Muted, 1.2f, Align::Center);
			Tx -= 8.0f;
		}
	}
	if (CardTab == 1 || CardTab == 2 || CardTab == 3)
	{
		if (CardTab == 3)
		{
			StatsBoard(P.Stats, P.Awards, {R.X + 24.0f, R.Y + 306.0f, R.W - 48.0f, R.H - 306.0f - 22.0f}, Now);
		}
		else if (CardTab == 1)
		{
			CardJourney(P, Mx, R.Y + 292.0f, Mw, R.Y + R.H - 20.0f - (R.Y + 292.0f), Now);
		}
		else
		{
			CardTrophies(P, Mx, R.Y + 292.0f, Mw, R.Y + R.H - 20.0f - (R.Y + 292.0f), Now);
			CardBadgeTiles(*C, UI, S, CardShown, Held, Frame, Mx, R.Y + 292.0f, Mw, R.Y + R.H - 20.0f - (R.Y + 292.0f), !P.Awards.empty(), CardTabAt, Now);
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
		UI.Text(Place, Mx + 62.0f, Ry + 20.0f, Ts(13.0f, 800, F.Place == 1 ? pal::Gold : F.Place <= 9 ? CardInk() : CardSoft(), Align::Left, Baseline::Alphabetic, true));
		UI.Text(F.Event, Mx + 170.0f, Ry + 20.0f, Ts(13.0f, 600, F.Major ? CardInk() : CardSoft(), Align::Left, Baseline::Alphabetic, false, Mw - 280.0f));
		UI.Text(F.Seat > 0 ? "Seat \xC2\xB7 " + NetMoney(F.Seat) : NetMoney(F.Prize), Mx + Mw, Ry + 20.0f,
			Ts(13.0f, 800, F.Prize > 0 || F.Seat > 0 ? pal::Gold : pal::Dim, Align::Right, Baseline::Alphabetic, true));
		if (F.Where == 1)
		{
			NetPill(*C, "LIVE", Mx + Mw - 120.0f, Ry + 6.0f, pal::Gold, false, 8.5f);
		}
	}
	My += Nf(std::min<size_t>(P.Recent.size(), 6)) * 30.0f + 26.0f;
	C->FillRect({Mx, My - 8.0f, Mw, 1.0f}, CardRule());
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
			Ay = NetParagraph(*C, P.Memories[K], Mx, Ay + 16.0f, Half, 13.0f, 500, CardSoft(), 18.0f, 2) - 2.0f;
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
		By = NetParagraph(*C, P.Story[K], Rx, By + 16.0f, Half, 13.0f, 500, CardSoft(), 18.0f, 2) - 2.0f;
	}
	if (!P.Knows.empty())
	{
		By += 18.0f;
		NetSpaced(*C, "RUNS WITH", Rx, By, 11.0f, 800, pal::Muted, 1.6f);
		for (size_t K = 0; K < P.Knows.size() && K < 3; ++K)
		{
			UI.Text(P.Knows[K].first + ": " + P.Knows[K].second, Rx, By + 22.0f + Nf(K) * 20.0f, Ts(13.0f, 600, CardSoft(), Align::Left, Baseline::Alphabetic, false, Half));
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
	// With badges to show under them (CardBadgeTiles), the trophies take one row of the tighter grid.
	const bool Badges = !NetBadgesOf(S, CardShown).Held.empty();
	if (Badges)
	{
		H = std::min(H, CardTrophyRowH);
	}
	NetSpaced(*C, "BRACELETS AND RINGS", X, Y, 11.0f, 800, pal::Muted, 1.6f);
	if (P.Awards.empty())
	{
		float Ty = NetParagraph(*C, "No bracelets or rings. Yet.", X, Y + 30.0f, W, 17.0f, 600, CardInk(), 23.0f, 1);
		if (!Badges)
		{
			NetParagraph(*C,
				"Bracelets are won at The Championship (Las Vegas, every summer) and The Championship Online in June. Rings are won on the Grand Circuit, a stop every six "
				"to eight weeks, and at Ring Rush in March.",
				X, Ty + 22.0f, W, 13.5f, 500, pal::Muted, 20.0f, 4);
		}
		return;
	}
	// The newest first: big cards for a few, a tighter grid for a collection (or above the badges).
	const bool Big = P.Awards.size() <= 6 && !Badges;
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
		C->FillRoundRect(Cd, 14.0f, Paint::Linear({0.0f, Cd.Y}, {0.0f, Cd.Y + Cd.H}, Mix(rlsites_detail::SitesTone(0x0f1b2e), Tint, 0.16f), rlsites_detail::SitesTone(0x0c1626)));
		C->StrokeRoundRect(Cd, 14.0f, A.Main ? NetA(pal::Gold, 0.7f) : CardRule(), A.Main ? 1.5f : 1.0f);
		const float Ix = Cd.X + Cd.W / 2.0f;
		const float Iy = Cd.Y + (Big ? 64.0f : 46.0f);
		C->FillEllipse(Ix, Iy, Big ? 70.0f : 48.0f, Big ? 56.0f : 38.0f, Paint::Radial({Ix, Iy}, 0.0f, {Ix, Iy}, Big ? 70.0f : 48.0f, NetA(Tint, 0.28f), 0.5f, NetA(Tint, 0.08f), NetA(Tint, 0.0f)));
		eventart::Trophy(*C, A, Ix, Iy, (Big ? 108.0f : 76.0f) * (0.9f + 0.1f * In), Now + 0.6 * static_cast<double>(K));
		const float Ty = Cd.Y + (Big ? 132.0f : 98.0f);
		NetSpaced(*C, A.Main ? (A.Ring ? "MAIN EVENT RING" : "MAIN EVENT BRACELET") : A.Ring ? "RING" : "BRACELET", Ix, Ty, Big ? 9.5f : 8.5f, 900, A.Main ? pal::Gold : pal::Orange, 1.5f, Align::Center);
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
		UI.Text(First, Ix, Ty + (Big ? 21.0f : 18.0f), Ts(Ts0, 800, CardInk(), Align::Center, Baseline::Alphabetic, false, Room));
		if (!Second.empty())
		{
			UI.Text(Second, Ix, Ty + 38.0f, Ts(Ts0, 800, CardInk(), Align::Center, Baseline::Alphabetic, false, Room));
		}
		UI.Text(eventart::TrophySeries(A), Ix, Ty + (Big ? 40.0f : 34.0f) + Off, Ts(Big ? 12.0f : 10.5f, 600, CardSoft(), Align::Center, Baseline::Alphabetic, false, Room));
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
	float Ty = NetParagraph(*C, P.Came, X, Y + 26.0f, W, 17.0f, 600, CardInk(), 23.0f, 2);
	if (P.Arrived >= 0)
	{
		const int Ago = Today - P.Arrived;
		const std::string When = Ago <= 0 ? "today" : Ago == 1 ? "yesterday" : Ago < 60 ? std::to_string(Ago) + " days ago" : Ago < 730 ? std::to_string(Ago / 30) + " months ago"
																																: std::to_string(Ago / 365) + " years ago";
		UI.Text("Joined " + CardJoinedSite(S.Living().Get(CardShown)) + " " + net::DateLabel(P.Arrived) + ", " + std::to_string(world::YearOf(P.Arrived)) + "  \xC2\xB7  " + When, X, Ty + 18.0f,
			Ts(13.0f, 600, P.New ? pal::Accent : pal::Muted));
		Ty += 22.0f;
	}
	Ty += 24.0f;
	C->FillRect({X, Ty - 10.0f, W, 1.0f}, CardRule());
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
		C->FillRect({Lx - 1.0f, Ty + 8.0f, 2.0f, Nf(Shown.size() - 1) * Row * In}, rlsites_detail::SitesTone(0x2b4a6e));
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
		case world::StepKind::FirstLive: Col = pal::Accent; break;
		case world::StepKind::FirstWin:
		case world::StepKind::FirstSeries:
		case world::StepKind::Major:
		case world::StepKind::BigScore:
		case world::StepKind::PlayerOfYear:
		case world::StepKind::HallOfFame: Col = pal::Gold; break;
		case world::StepKind::Bracelet:
		case world::StepKind::Ring: Col = pal::Orange; break;
		case world::StepKind::MovedUp:
		case world::StepKind::TurnedPro:
		case world::StepKind::Sponsored:
		case world::StepKind::StartedStreaming: Col = pal::Accent2; break;
		case world::StepKind::WentBroke:
		case world::StepKind::MovedDown: Col = pal::Red; break;
		case world::StepKind::Break:
		case world::StepKind::Retired: Col = pal::Dim; break;
		default: break;
		}
		UI.Text(net::DateLabel(M->Day), X + 100.0f, Ry + 13.0f, Ts(12.0f, 700, pal::Muted, Align::Right, Baseline::Alphabetic, true));
		UI.Text(std::to_string(world::YearOf(M->Day)), X + 100.0f, Ry + 27.0f, Ts(10.0f, 600, pal::Dim, Align::Right, Baseline::Alphabetic, true));
		C->FillCircle(Lx, Ry + 8.0f, 6.0f, rlsites_detail::SitesTone(0x0c1626));
		C->FillCircle(Lx, Ry + 8.0f, 4.5f, Col);
		UI.Text(M->Text, Lx + 18.0f, Ry + 13.0f, Ts(13.5f, 600, M->Kind == world::StepKind::Joined ? CardInk() : CardSoft(), Align::Left, Baseline::Alphabetic, false, W - 140.0f));
		C->SetAlpha(A0);
	}
}

// ------------------------------------------------------------------ the next segment's badges (next_segment_spec §5.6)
// The Leaderboards page's HONORS mode, and a badge's hover card (drawn last in RiverLine::Draw, over everything). The
// rules and the art are core-badges' (Badges.h, BadgeArt.h); the pages are ui-laptop's.

void RiverLine::HonorsPanel(const Rect& R, double Now)
{
	// HONORS (§5.6): the Leaderboards page's own mode. Left, the Hall of Fame by class; middle, the Players of the Year by
	// season, online and live; right, what the honors are. Everything here is public and kept for good.
	const world::World& W = S.Living();
	const double Since = Now - BoardAt;
	std::vector<const world::Honor*> Hall;
	std::map<int, std::pair<const world::Honor*, const world::Honor*>> Poy; // year -> online, live
	for (const world::Honor& Hn : W.Honors())
	{
		if (Hn.Title == world::HallOfFameTitle)
		{
			Hall.push_back(&Hn);
		}
		else if (Hn.Title == "Player of the Year")
		{
			Poy[Hn.Year].first = &Hn;
		}
		else if (Hn.Title == "Live Player of the Year")
		{
			Poy[Hn.Year].second = &Hn;
		}
	}
	std::stable_sort(Hall.begin(), Hall.end(), [](const world::Honor* A, const world::Honor* B) { return A->Year > B->Year; });
	const float Cw = (R.W - 16.0f) / 2.0f;
	auto Name = [&](const world::Honor& Hn, float X, float Y, float MaxW) {
		const bool Hero = Hn.Npc == -1;
		const std::string& Nm = Hero ? S.HeroName : Hn.Name;
		const AvatarSpec Pic = NetAvatar(*C, X + 15.0f, Y - 5.0f, 14.0f, Nm, Color{0.0f, 0.0f, 0.0f, 0.0f}, Hero);
		if (Hero || W.Get(Hn.Npc))
		{
			PlayerName(Hn.Npc, X + 38.0f, Y, 15.0f, MaxW - 38.0f, true, NetBadgeFrame(Pic));
		}
		else
		{
			UI.Text(Nm, X + 38.0f, Y, Ts(15.0f, 700, CardSoft(), Align::Left, Baseline::Alphabetic, false, MaxW - 38.0f));
		}
	};

	// The Hall of Fame.
	{
		const Rect P{R.X, R.Y, Cw, R.H};
		Panel(P);
		badge::Badge Medal;
		Medal.K = badge::Kind::HallOfFame;
		badgeart::Draw(*C, Medal, P.X + 40.0f, P.Y + 40.0f, 32.0f, Now);
		UI.Text("Hall of Fame", P.X + 66.0f, P.Y + 48.0f, Ts(24.0f, 900, CardInk()));
		float Y = P.Y + 92.0f;
		if (Hall.empty())
		{
			Y = NetParagraph(*C,
					"Nobody's in yet. Each July 15 the eligible are weighed: forty or older, ten years on the scene, high stakes played, and a record of bracelets, rings, majors "
					"and Player of the Year titles that holds up. Two at most a year.",
					P.X + 26.0f, Y, P.W - 52.0f, 14.0f, 500, pal::Muted, 21.0f, 6) + 24.0f;
			// The ballot as it stands: who's eligible next July 15, the best records first, against the bar.
			const int Today = net::DayOf(S.WorldMinutes());
			int Year = world::YearOf(Today);
			Year += Today > world::DayOn(Year, 7, 15) ? 1 : 0;
			// Weighed once a day and world revision (§10: the roster's walk never runs per frame).
			static const world::World* BallotOf = nullptr;
			static int BallotDay = -1;
			static int BallotRev = -1;
			static uint32_t BallotSeed = 0;
			static std::vector<std::pair<int, double>> Ballot;
			if (BallotOf != &W || BallotDay != Today || BallotRev != W.Revision() || BallotSeed != W.Seed())
			{
				BallotOf = &W;
				BallotDay = Today;
				BallotRev = W.Revision();
				BallotSeed = W.Seed();
				Ballot.clear();
				for (const world::Npc& N : W.People())
				{
					if (W.HallEligible(N.Id, Year))
					{
						Ballot.push_back({N.Id, W.HallScore(N.Id, Year)});
					}
				}
				std::stable_sort(Ballot.begin(), Ballot.end(), [](const std::pair<int, double>& A, const std::pair<int, double>& B) { return A.second > B.second; });
			}
			NetSpaced(*C, "ON THE BALLOT \xC2\xB7 JULY 15, " + std::to_string(Year), P.X + 26.0f, Y, 10.5f, 800, pal::Gold, 1.6f);
			Y += 32.0f;
			if (Ballot.empty())
			{
				// A fresh start (update 2 §3.6, F4): everyone's ten years on the scene began on the first night.
				NetParagraph(*C, S.WorldStart() == world::Start::Blank ? "Nobody's eligible yet. The Hall wants ten years on the scene." : "Nobody's eligible yet.", P.X + 26.0f, Y,
					P.W - 52.0f, 14.0f, 500, pal::Dim, 21.0f, 3);
			}
			for (size_t K = 0; K < Ballot.size() && K < 8 && Y < P.Y + P.H - 40.0f; ++K)
			{
				const world::Npc* N = W.Get(Ballot[K].first);
				world::Honor Row;
				Row.Npc = N ? N->Id : -2;
				Row.Name = N ? N->Name : std::string();
				Name(Row, P.X + 22.0f, Y, P.W - 190.0f);
				const bool Over = Ballot[K].second >= badge::HallBar;
				UI.Text(Fixed(Ballot[K].second, 1) + (Over ? " \xC2\xB7 over the bar" : " of " + Fixed(badge::HallBar, 0)), P.X + P.W - 26.0f, Y,
					Ts(13.0f, 700, Over ? pal::Gold : pal::Muted, Align::Right, Baseline::Alphabetic, true));
				Y += 40.0f;
			}
		}
		int Shown = 0;
		for (size_t K = 0; K < Hall.size() && Y < P.Y + P.H - 40.0f; ++K)
		{
			const world::Honor& Hn = *Hall[K];
			if (K == 0 || Hall[K - 1]->Year != Hn.Year)
			{
				NetSpaced(*C, "CLASS OF " + std::to_string(Hn.Year), P.X + 26.0f, Y, 10.5f, 800, pal::Gold, 1.6f);
				Y += 30.0f;
			}
			const float Ri = NetEase((Since - 0.05 * static_cast<double>(Shown++)) / 0.4);
			const float A0 = C->GetAlpha();
			C->SetAlpha(A0 * Ri);
			Name(Hn, P.X + 22.0f, Y, P.W - 150.0f);
			UI.Text(Hn.Prize > 0 ? net::MoneyShort(Hn.Prize) + " career" : std::string(), P.X + P.W - 26.0f, Y, Ts(13.0f, 700, pal::Muted, Align::Right, Baseline::Alphabetic, true));
			C->SetAlpha(A0);
			Y += 40.0f;
		}
	}

	// The Players of the Year.
	{
		const Rect P{R.X + Cw + 16.0f, R.Y, Cw, R.H};
		Panel(P);
		badge::Badge Medal;
		Medal.K = badge::Kind::PlayerOfYear;
		badgeart::Draw(*C, Medal, P.X + 40.0f, P.Y + 40.0f, 32.0f, Now);
		UI.Text("Players of the Year", P.X + 66.0f, P.Y + 48.0f, Ts(24.0f, 900, CardInk()));
		float Y = P.Y + 92.0f;
		if (Poy.empty())
		{
			Y = NetParagraph(*C,
					"The first are crowned on New Year's Day: the most points on RiverLine all year, and live, from the regional festivals up to the Championship.",
					P.X + 26.0f, Y, P.W - 52.0f, 14.0f, 500, pal::Muted, 21.0f, 5) + 24.0f;
			// The race as it stands: the Season board's top three, and the Live board's.
			const net::Network& Net = net::Shared();
			const std::pair<net::Board, const char*> Races[2] = {{net::Board::Season, "THIS YEAR ONLINE"}, {net::Board::Live, "THIS YEAR LIVE"}};
			for (const auto& Race : Races)
			{
				NetSpaced(*C, Race.second, P.X + 26.0f, Y, 10.5f, 800, pal::Gold, 1.6f);
				Y += 32.0f;
				const std::vector<net::BoardRow> Top = Net.Leaderboard(Race.first, World, You, 3, nullptr);
				if (Top.empty())
				{
					UI.Text("Nobody's scored yet.", P.X + 26.0f, Y, Ts(13.5f, 500, pal::Dim));
					Y += 34.0f;
				}
				for (const net::BoardRow& Rw : Top)
				{
					world::Honor Row;
					Row.Npc = Rw.Player;
					Row.Name = Rw.Player < 0 ? S.HeroName : Net.Players()[static_cast<size_t>(Rw.Player)].Name;
					UI.Text(std::to_string(Rw.Rank), P.X + 34.0f, Y, Ts(14.0f, 900, Rw.Rank == 1 ? pal::Gold : pal::Muted, Align::Right, Baseline::Alphabetic, true));
					Name(Row, P.X + 42.0f, Y, P.W - 190.0f);
					UI.Text(Grouped(static_cast<int64_t>(std::llround(Rw.Value))) + " pts", P.X + P.W - 26.0f, Y, Ts(13.0f, 700, pal::Muted, Align::Right, Baseline::Alphabetic, true));
					Y += 38.0f;
				}
				Y += 14.0f;
			}
		}
		int Shown = 0;
		for (auto It = Poy.rbegin(); It != Poy.rend() && Y < P.Y + P.H - 90.0f; ++It)
		{
			const float Ri = NetEase((Since - 0.06 * static_cast<double>(Shown++)) / 0.4);
			const float A0 = C->GetAlpha();
			C->SetAlpha(A0 * Ri);
			// A fresh start's first season ran from Night One (update 2 §3.6, F6): "2026 · FROM OCT 6".
			const bool Short = S.WorldStart() == world::Start::Blank && It->first == world::YearOf(net::NightOneDay);
			NetSpaced(*C, std::to_string(It->first) + (Short ? " \xC2\xB7 FROM " + NetUpper(net::DateLabel(net::NightOneDay)) : std::string()), P.X + 26.0f, Y, 10.5f, 800, pal::Gold, 1.6f);
			Y += 30.0f;
			const std::pair<const world::Honor*, const char*> Lines[2] = {{It->second.first, "ONLINE"}, {It->second.second, "LIVE"}};
			for (const auto& Ln : Lines)
			{
				NetSpaced(*C, Ln.second, P.X + 26.0f, Y, 9.0f, 800, pal::Dim, 1.2f);
				if (Ln.first)
				{
					Name(*Ln.first, P.X + 80.0f, Y, P.W - 200.0f);
					UI.Text(Grouped(Ln.first->Entries) + " pts", P.X + P.W - 26.0f, Y, Ts(13.0f, 700, pal::Muted, Align::Right, Baseline::Alphabetic, true));
				}
				else
				{
					UI.Text("not awarded", P.X + 118.0f, Y, Ts(13.0f, 500, pal::Dim));
				}
				Y += 34.0f;
			}
			C->SetAlpha(A0);
			Y += 6.0f;
		}
	}

	// About the honors (the board's right panel, as the boards have it).
	{
		const Rect Rt{R.X + R.W + 16.0f, 84.0f, 484.0f, 866.0f};
		Panel(Rt);
		UI.Text("Honors", Rt.X + 26.0f, Rt.Y + 54.0f, Ts(28.0f, 900, CardInk()));
		float Y = NetParagraph(*C,
			"The titles that stay with a name. The Hall of Fame takes its class each July 15; the Players of the Year are crowned when the year turns, one online and one "
			"live. They show on every card and every board.",
			Rt.X + 26.0f, Rt.Y + 90.0f, Rt.W - 52.0f, 15.0f, 500, CardSoft(), 22.0f, 6) + 18.0f;
		C->FillRect({Rt.X + 26.0f, Y, Rt.W - 52.0f, 1.0f}, pal::Line);
		Y += 36.0f;
		// The player's own: theirs if they hold any, else the honest distance.
		NetSpaced(*C, "YOURS", Rt.X + 26.0f, Y, 11.0f, 800, pal::Muted, 1.6f);
		const badge::Set& Mine = S.HeroBadges();
		const badge::Badge* MyHall = Mine.Find(badge::Kind::HallOfFame);
		const badge::Badge* MyPoy = Mine.Find(badge::Kind::PlayerOfYear);
		const badge::Badge* MyLive = Mine.Find(badge::Kind::LivePlayerOfYear);
		int Titles = 0;
		for (const auto& Yr : Poy)
		{
			Titles += (Yr.second.first && Yr.second.first->Npc == -1 ? 1 : 0) + (Yr.second.second && Yr.second.second->Npc == -1 ? 1 : 0);
		}
		const std::string Own = MyHall ? "In the Hall of Fame, class of " + std::to_string(MyHall->Year) + "."
			: Titles > 0 || MyPoy || MyLive ? std::to_string(std::max(Titles, 1)) + (Titles > 1 ? " Player of the Year titles." : " Player of the Year title.")
											: std::string("None yet. The Season board is this year's race.");
		NetParagraph(*C, Own, Rt.X + 26.0f, Y + 28.0f, Rt.W - 52.0f, 15.0f, 600, MyHall || Titles > 0 ? pal::Gold : CardSoft(), 21.0f, 3);
	}
}

void RiverLine::BadgeTip(const badge::Badge& B, float X, float Y, double Now)
{
	// The hover card (§5.6): the glyph at 32, the title, how it was earned, from the holder's public facts. Under the
	// glyph, kept on screen (above it near the taskbar).
	const std::string Title = badge::Title(B);
	if (Title.empty())
	{
		return;
	}
	const std::string How = badge::HowEarned(B, NetHolderOf(S, NetTipOf(this).Npc));
	const float W = 320.0f;
	const float TextW = W - 70.0f - 14.0f;
	const std::vector<std::string> Lines = NetWrap(*C, How, TextW, 12.5f, 500);
	const size_t Shown = std::min<size_t>(Lines.size(), 4);
	const float H = std::max(88.0f, 46.0f + 17.0f * Nf(Shown) + 10.0f);
	Rect R{std::min(std::max(8.0f, X - 30.0f), NetW - W - 8.0f), Y + 8.0f, W, H};
	if (R.Y + R.H > RiverLine::Height - 46.0f)
	{
		R.Y = Y - 30.0f - H;
	}
	C->GlowRoundRect({R.X, R.Y + 6.0f, R.W, R.H}, 12.0f, Rgba(0, 0, 0, 0.55f), 18.0f);
	UI.RRect(R, 12.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, rlsites_detail::SitesTone(0x172a44), rlsites_detail::SitesTone(0x0e1a2c)), rlsites_detail::SitesTone(0x2a3f60));
	badgeart::Draw(*C, B, R.X + 36.0f, R.Y + 44.0f, 32.0f, Now);
	UI.Text(Title, R.X + 70.0f, R.Y + 30.0f, Ts(14.5f, 800, CardInk(), Align::Left, Baseline::Alphabetic, false, TextW));
	float Ly = R.Y + 50.0f;
	for (size_t K = 0; K < Shown; ++K)
	{
		std::string L = Lines[K];
		if (K + 1 == Shown && Lines.size() > Shown)
		{
			L += " " + Lines[K + 1];
		}
		UI.Text(L, R.X + 70.0f, Ly, Ts(12.5f, 500, CardSoft(), Align::Left, Baseline::Alphabetic, false, TextW));
		Ly += 17.0f;
	}
}
} // namespace ui
} // namespace ss
