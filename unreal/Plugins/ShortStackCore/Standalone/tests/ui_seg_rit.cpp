// rivals_ui: ui-rit's checks (update 3 SPEC §7.7, v1.2's red and black, §13): the rail's 8 plates open every page (LEAGUE
// hidden until G2; no "COMING SOON" anywhere) and the section slab names each; the Board writes Listed and the arrows walk
// it; its OVERLAY column is site::show's projection; its statuses settle in a wave (RitFlap) and FINAL CALL blinks; night
// service; the octagon's seat lists clear of SidePanel, the lower third and each other; the Second Board only after an
// all-in with cards to come, never moving a chip; the BB toggle; the Kast preview plays no sound; red is light, never ink;
// every label on a red fill at 4.5:1; the Watchlist's star never red; one Signal Red twin stripe a panel (the receipt's
// two) and at most 24 twice-printed runs a page; no RiverLine hex on any page; the test pins the sites' suite reads on
// RunItTwice (the balance run, the Club's kickers and tickets, the Cashier's window, the YOU bar's window); the copy
// (RitInventory) linted and screened; SS_UI_BENCH. Compiled into rivals_ui_test, called from rivals_ui_main.cpp (the kit is
// UiSeg.h); the helpers are in ui_test::rit_seg.
#include "UiSeg.h"

#include "CopyLint.h"
#include "Trademarks.h"
#include "ShortStack/Cards.h"
#include "ShortStack/Equity.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Gear.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/SiteShow.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Rng.h"
#include "ShortStack/Tournament.h"
#include "ShortStack/UI/RiverLine.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/SitesDetail.h"
#include "../../Source/ShortStackCore/Private/ShortStack/UI/RiverLineShared.h"
#include "../../Source/ShortStackCore/Private/ShortStack/UI/RitArt.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace ui_test
{
namespace rit_seg
{
using Page = ss::ui::RiverLine::Page;
using App = ss::ui::RiverLine::App;
using ss::net::SiteId;
using ss::ui::DrawCmd;
using ss::ui::DrawList;
constexpr double RitDay = 1440.0;
/** Wed Nov 4, 2026, 7:30 PM (the sites' suite's evening): RunItTwice's lobby has been open since Night One. */
constexpr double RitEvening = 30.0 * RitDay + 19.5 * 60.0;

int RitChecks = 0;
void RitExpect(bool Condition, const char* What)
{
	++RitChecks;
	Expect(Condition, What);
}

bool RitIsCue(ss::SoundId Id)
{
	return Id == ss::SoundId::RitBlip || Id == ss::SoundId::RitFlap || Id == ss::SoundId::RitBell || Id == ss::SoundId::RitStamp || Id == ss::SoundId::RitRank;
}

struct RitHooks : QuietHooks
{
	std::vector<ss::SoundId> Sounds;
	void Sound(ss::SoundId Id, double /*Volume*/) override { Sounds.push_back(Id); }
	bool GoOut(const std::string&, ss::Chips) override { return true; }
	int Count(ss::SoundId Id) const { return static_cast<int>(std::count(Sounds.begin(), Sounds.end(), Id)); }
	int Cues() const { return static_cast<int>(std::count_if(Sounds.begin(), Sounds.end(), RitIsCue)); }
};

/** A session and its laptop, the pointer driven by hand (the sites' suite's kit, this file's own copy). */
struct RitLaptop
{
	RitHooks H;
	ss::Session S;
	ss::ui::RiverLine RL;
	TableMeasurer M;
	double Now = 1.0;
	explicit RitLaptop(const std::string& Seed) : S(H, Seed), RL(S)
	{
		S.CurrentScreen = ss::Screen::Lobby;
		Now = Step(S, Now, 0.2);
	}
	DrawList Frame(float X = -1.0f, float Y = -1.0f, bool Down = false, bool Pressed = false, bool Released = false)
	{
		RL.UI.Ptr.Active = X >= 0.0f;
		RL.UI.Ptr.X = X;
		RL.UI.Ptr.Y = Y;
		RL.UI.Ptr.Down = Down;
		RL.UI.Ptr.Pressed = Pressed;
		RL.UI.Ptr.Released = Released;
		S.Now = Now; // the cues' 50 ms limiter runs on the session's clock, as the host's Update keeps it
		DrawList L;
		ss::ui::Canvas C(L, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, 1.0f);
		RL.Draw(C, Now);
		RL.UI.Ptr.EndFrame();
		return L;
	}
	void Click(float X, float Y)
	{
		Frame(X, Y, true, true, false);
		Now += 0.05;
		Frame(X, Y, false, false, true);
		Now += 0.05;
	}
	DrawList Settled(float X = -1.0f, float Y = -1.0f)
	{
		Now += 1.5;
		Frame(X, Y);
		Now += 0.05;
		return Frame(X, Y);
	}
	void At(double World)
	{
		if (World > S.WorldMinutes())
		{
			S.LobbyMinutes += World - S.WorldMinutes();
		}
		Now = Step(S, Now, 0.2);
		S.Life.Hunger = 20.0;
		S.Life.Thirst = 20.0;
		S.Life.Energy = 80.0;
	}
	ss::site::Account* Join(ss::Chips Cents)
	{
		ss::site::Join(S.Life.Sites, SiteId::RunItTwice, S.WorldMinutes());
		ss::site::Account* A = ss::site::AccountOf(S.Life.Sites, SiteId::RunItTwice);
		if (A)
		{
			A->Cents = Cents;
		}
		S.Life.Update2LinesRead |= ss::life::U2Sites;
		return A;
	}
	void Open(Page P)
	{
		RL.OpenApp(App::RiverLine, Now);
		RL.SiteShown = SiteId::RunItTwice;
		RL.OpenPage(P, Now);
	}
};

uint32_t RitRgb(const ss::ui::Color& C)
{
	const auto Byte = [](float V) { return static_cast<uint32_t>(std::min(255.0f, std::max(0.0f, V * 255.0f + 0.5f))); };
	return Byte(C.R) << 16 | Byte(C.G) << 8 | Byte(C.B);
}

double RitContrast(const ss::ui::Color& A, const ss::ui::Color& B)
{
	const auto Lin = [](float V) { return V <= 0.04045f ? V / 12.92 : std::pow((V + 0.055) / 1.055, 2.4); };
	const auto Lum = [&Lin](const ss::ui::Color& C) { return 0.2126 * Lin(C.R) + 0.7152 * Lin(C.G) + 0.0722 * Lin(C.B); };
	const double La = Lum(A);
	const double Lb = Lum(B);
	return (std::max(La, Lb) + 0.05) / (std::min(La, Lb) + 0.05);
}

bool RitReddish(const ss::ui::Color& C)
{
	return C.R > 0.4f && C.G < 0.25f && C.B < 0.25f;
}

/** The client's area above the taskbar. */
const ss::ui::Rect RitClientArea{0.0f, 0.0f, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height - 38.0f};

/**
 * Content a page check leaves to its owner: the players' own avatars (Avatars.cpp's palette shares RiverLine's gold
 * f2c14e as one player's colour: a person's colour, not the site's chrome). A leak whose whole box sits inside one of these
 * rects isn't counted; a page that draws avatars sets them around its check (empty otherwise).
 */
std::vector<ss::ui::Rect> RitContent;

/** RiverLine's 16 hexes drawn inside RunItTwice's client that aren't RunItTwice's own (specks under 12 x 12 ignored). */
int RitLeaks(const DrawList& L, std::set<uint32_t>& Seen)
{
	std::set<uint32_t> Home;
	for (const uint32_t H : ss::net::Def(SiteId::RiverLine).Palette)
	{
		Home.insert(H);
	}
	std::set<uint32_t> Own;
	for (const uint32_t H : ss::net::Def(SiteId::RunItTwice).Palette)
	{
		Own.insert(H);
	}
	const ss::ui::Rect& A = RitClientArea;
	const auto Inside = [&A](float X, float Y) { return X >= A.X && X <= A.X + A.W && Y >= A.Y && Y <= A.Y + A.H; };
	int Count = 0;
	for (const DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == DrawCmd::Kind::Text)
		{
			const uint32_t Rgb = RitRgb(Cmd.Text.Col);
			if (Cmd.Text.Col.A > 0.02f && Inside(Cmd.Text.X, Cmd.Text.BaselineY) && Home.count(Rgb) && !Own.count(Rgb))
			{
				++Count;
				Seen.insert(Rgb);
			}
			continue;
		}
		if (Cmd.Type != DrawCmd::Kind::Triangles)
		{
			continue;
		}
		int Here = 0;
		float X0 = 1.0e9f, Y0 = 1.0e9f, X1 = -1.0e9f, Y1 = -1.0e9f;
		std::set<uint32_t> Mine;
		for (uint32_t K = 0; K < Cmd.VertexCount; ++K)
		{
			const ss::ui::Vertex& V = L.Vertices[Cmd.VertexStart + K];
			const uint32_t Rgb = RitRgb(V.Col);
			if (V.Col.A > 0.02f && Inside(V.X, V.Y) && Home.count(Rgb) && !Own.count(Rgb))
			{
				++Here;
				Mine.insert(Rgb);
				X0 = std::min(X0, V.X);
				Y0 = std::min(Y0, V.Y);
				X1 = std::max(X1, V.X);
				Y1 = std::max(Y1, V.Y);
			}
		}
		const bool Content = std::any_of(RitContent.begin(), RitContent.end(),
			[&](const ss::ui::Rect& R) { return X0 >= R.X && Y0 >= R.Y && X1 <= R.X + R.W && Y1 <= R.Y + R.H; });
		if (Here > 0 && (X1 - X0 > 12.0f || Y1 - Y0 > 12.0f) && !Content)
		{
			Count += Here;
			Seen.insert(Mine.begin(), Mine.end());
		}
	}
	return Count;
}

void RitNoLeak(const DrawList& L, const char* What)
{
	std::set<uint32_t> Seen;
	const int Count = RitLeaks(L, Seen);
	if (Count > 0)
	{
		std::printf("  leak (%s):", What);
		for (const uint32_t H : Seen)
		{
			std::printf(" %06x", H);
		}
		std::printf(" in %d places\n", Count);
	}
	RitExpect(Count == 0, (std::string("rit: no RiverLine colour on RunItTwice's page: ") + What).c_str());
}

/** The opaque ground under a point when command Upto drew (the last opaque triangle there before it), and whether one was. */
bool RitGround(const DrawList& L, size_t Upto, float Px, float Py, ss::ui::Color& Out)
{
	bool Found = false;
	for (size_t C = 0; C < Upto; ++C)
	{
		const DrawCmd& Cmd = L.Cmds[C];
		if (Cmd.Type != DrawCmd::Kind::Triangles)
		{
			continue;
		}
		for (uint32_t T = 0; T + 2 < Cmd.IndexCount; T += 3)
		{
			const ss::ui::Vertex& A = L.Vertices[Cmd.VertexStart + L.Indices[Cmd.IndexStart + T]];
			const ss::ui::Vertex& B = L.Vertices[Cmd.VertexStart + L.Indices[Cmd.IndexStart + T + 1]];
			const ss::ui::Vertex& V = L.Vertices[Cmd.VertexStart + L.Indices[Cmd.IndexStart + T + 2]];
			if (A.Col.A < 0.85f || B.Col.A < 0.85f || V.Col.A < 0.85f)
			{
				continue;
			}
			const float D = (B.Y - V.Y) * (A.X - V.X) + (V.X - B.X) * (A.Y - V.Y);
			if (std::fabs(D) < 1e-6f)
			{
				continue;
			}
			const float Wa = ((B.Y - V.Y) * (Px - V.X) + (V.X - B.X) * (Py - V.Y)) / D;
			const float Wb = ((V.Y - A.Y) * (Px - V.X) + (A.X - V.X) * (Py - V.Y)) / D;
			const float Wc = 1.0f - Wa - Wb;
			if (Wa < 0.0f || Wb < 0.0f || Wc < 0.0f)
			{
				continue;
			}
			Out = {A.Col.R * Wa + B.Col.R * Wb + V.Col.R * Wc, A.Col.G * Wa + B.Col.G * Wb + V.Col.G * Wc, A.Col.B * Wa + B.Col.B * Wb + V.Col.B * Wc, 1.0f};
			Found = true;
		}
	}
	return Found;
}

/**
 * Red is light, never ink (§3.3, §7.7): every text run in Signal Red, Deep Red or LED Red is a twice print's under-copy (the
 * same string next, in Ink, Hot White or Paper Ink, at (-o, -o)), the stamp's word on paper, or the logo's "Twice"; and every
 * other label on a red fill reads at 4.5:1 or more.
 */
void RitRedIsLight(const DrawList& L, const TableMeasurer& M, const char* What)
{
	const std::set<uint32_t> Reds = {ss::ui::rlrit_detail::RitRed, ss::ui::rlrit_detail::RitDeep, ss::ui::rlrit_detail::RitLed};
	const std::set<uint32_t> Readable = {ss::ui::rlrit_detail::RitInk, ss::ui::rlrit_detail::RitWhite, ss::ui::rlrit_detail::RitPaperInk};
	const std::set<std::string> Stamps = {"CASHED", "FINAL TABLE", "WINNER", "TAKEN", "PAID"};
	int Ink = 0;
	int Weak = 0;
	for (size_t I = 0; I < L.Cmds.size(); ++I)
	{
		const DrawCmd& Cmd = L.Cmds[I];
		if (Cmd.Type != DrawCmd::Kind::Text || Cmd.Text.Col.A < 0.05f || Cmd.Text.BaselineY > RitClientArea.H)
		{
			continue;
		}
		const uint32_t Rgb = RitRgb(Cmd.Text.Col);
		const float W = M.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx);
		const float Px = Cmd.Text.X + W / 2.0f;
		const float Py = Cmd.Text.BaselineY - Cmd.Text.SizePx * 0.35f;
		if (Reds.count(Rgb))
		{
			bool Ok = Cmd.Text.Text == "Twice";
			for (size_t J = I + 1; J < L.Cmds.size() && !Ok; ++J)
			{
				const DrawCmd& Next = L.Cmds[J];
				if (Next.Type != DrawCmd::Kind::Text)
				{
					continue;
				}
				const float O = Cmd.Text.X - Next.Text.X;
				Ok = Next.Text.Text == Cmd.Text.Text && Readable.count(RitRgb(Next.Text.Col)) && O >= 1.9f && O <= 6.1f && std::fabs(Cmd.Text.BaselineY - Next.Text.BaselineY - O) < 0.05f;
				break;
			}
			ss::ui::Color G;
			if (!Ok && Stamps.count(Cmd.Text.Text) && Cmd.Text.Col.A > 0.99f && RitGround(L, I, Px, Py, G))
			{
				Ok = RitRgb(G) == ss::ui::rlrit_detail::RitPaper;
			}
			if (!Ok)
			{
				++Ink;
				std::printf("  red ink (%s): \"%s\" %06x at %.0f,%.0f\n", What, Cmd.Text.Text.c_str(), Rgb, Cmd.Text.X, Cmd.Text.BaselineY);
			}
			continue;
		}
		ss::ui::Color G;
		if (RitGround(L, I, Px, Py, G) && RitReddish(G))
		{
			const double Ratio = RitContrast(Cmd.Text.Col, G);
			if (Ratio < 4.5)
			{
				++Weak;
				std::printf("  weak label on red (%s): \"%s\" %06x on %06x, %.2f:1\n", What, Cmd.Text.Text.c_str(), Rgb, RitRgb(G), Ratio);
			}
		}
	}
	RitExpect(Ink == 0, (std::string("rit: red is light, never ink: ") + What).c_str());
	RitExpect(Weak == 0, (std::string("rit: every label on a red fill reads at 4.5:1: ") + What).c_str());
}

/** The signatures' tally after a page (§3.3): at most 24 twice prints, one Signal Red twin stripe a panel (the receipt two). */
void RitTallied(const char* What, int Stripes = 1)
{
	const ss::ui::rlrit_detail::RitTally& T = ss::ui::rlrit_detail::RitTallyNow();
	if (T.Twice > ss::ui::rlrit_detail::RitTwiceCap || T.MaxStripes > Stripes)
	{
		std::printf("  tally (%s): %d twice prints, %d stripes in a panel\n", What, T.Twice, T.MaxStripes);
	}
	RitExpect(T.Twice <= ss::ui::rlrit_detail::RitTwiceCap && T.MaxStripes <= Stripes, (std::string("rit: the twice print and the twin stripe within their caps: ") + What).c_str());
}

bool RitDrewSub(const DrawList& L, const std::string& Needle)
{
	for (const DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == DrawCmd::Kind::Text && Cmd.Text.Text.find(Needle) != std::string::npos)
		{
			return true;
		}
	}
	return false;
}

/** A run's box: its left, baseline and width (Found false: none drawn). */
struct RitBox
{
	bool Found = false;
	float X = 0.0f;
	float Base = 0.0f;
	float W = 0.0f;
	ss::ui::Font Face = ss::ui::Font::Regular;
	float Size = 0.0f;
};
RitBox RitRun(const DrawList& L, const std::string& Text, const TableMeasurer& M, float MinY = -1.0e9f, float MaxY = 1.0e9f)
{
	RitBox Out;
	for (const DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == DrawCmd::Kind::Text && Cmd.Text.Text == Text && Cmd.Text.BaselineY >= MinY && Cmd.Text.BaselineY <= MaxY)
		{
			Out.Found = true;
			Out.X = Cmd.Text.X;
			Out.Base = Cmd.Text.BaselineY;
			Out.W = M.Width(Text, Cmd.Text.Face, Cmd.Text.SizePx);
			Out.Face = Cmd.Text.Face;
			Out.Size = Cmd.Text.SizePx;
			return Out;
		}
	}
	return Out;
}

/** Tracked capitals drawn a glyph at a time, joined back up on one baseline (the sites' suite's SitesSpelled). */
bool RitSpelled(const DrawList& L, const std::string& Word)
{
	std::string Cur;
	float Base = -1.0e9f;
	for (const DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != DrawCmd::Kind::Text)
		{
			continue;
		}
		const std::string& T = Cmd.Text.Text;
		const bool Glyph = !T.empty() && (T.size() == 1 || (static_cast<unsigned char>(T[0]) >= 0xC0 && T.size() <= 4));
		if (Glyph && Cmd.Text.BaselineY == Base)
		{
			Cur += T;
			if (Cur.find(Word) != std::string::npos)
			{
				return true;
			}
			continue;
		}
		Cur = Glyph ? T : std::string();
		Base = Glyph ? Cmd.Text.BaselineY : -1.0e9f;
	}
	return false;
}

int RitRuns(const DrawList& L)
{
	int N = 0;
	for (const DrawCmd& Cmd : L.Cmds)
	{
		N += Cmd.Type == DrawCmd::Kind::Text ? 1 : 0;
	}
	return N;
}

bool RitHasHex(const DrawList& L, uint32_t Hex, const ss::ui::Rect& R)
{
	for (const ss::ui::Vertex& V : L.Vertices)
	{
		if (V.Col.A > 0.5f && RitRgb(V.Col) == Hex && V.X >= R.X && V.X <= R.X + R.W && V.Y >= R.Y && V.Y <= R.Y + R.H)
		{
			return true;
		}
	}
	return false;
}

/** Every check a page gets: no leak, red is light, labels on red, the tally; and no "COMING SOON" ever. */
void RitPageChecks(const DrawList& L, const TableMeasurer& M, const char* What, int Stripes = 1)
{
	RitNoLeak(L, What);
	RitRedIsLight(L, M, What);
	RitTallied(What, Stripes);
	bool Soon = false;
	for (const DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == DrawCmd::Kind::Text)
		{
			const std::string Low = copylint::Lower(Cmd.Text.Text);
			Soon = Soon || Low.find("coming soon") != std::string::npos;
		}
	}
	RitExpect(!Soon, (std::string("rit: no COMING SOON: ") + What).c_str());
}

// ------------------------------------------------------------------ the rail, the slab, every page

void RitRailAndPages()
{
	RitLaptop P("rit-rail");
	P.At(RitEvening);
	P.S.BankrollCents = 150000;
	P.Join(21240);
	P.Open(Page::Lobby);
	DrawList L = P.Settled();
	WriteList("rit_board", L);
	const char* const Plates[8] = {"BOARD", "SUNDAY LIVE", "RANKINGS", "THE DESK", "SERIES", "NEWSDESK", "CLUB", "CASHIER"};
	bool All = true;
	for (const char* Plate : Plates)
	{
		All = All && DrewRun(L, Plate);
	}
	RitExpect(All && !DrewRun(L, "LEAGUE"), "rit: the rail's 8 plates, LEAGUE hidden until G2");
	RitExpect(DrewRun(L, "Balance $212.40"), "rit: the header's balance is one run (the sites' pin)");
	const RitBox Bal = RitRun(L, "Balance $212.40", P.M);
	RitExpect(Bal.Found && Bal.Face == ss::ui::Font::BoldCondensed && std::fabs(Bal.X + Bal.W - 1576.0f) < 0.6f,
		"rit: the balance run is BoldCondensed, measured in its own face to end at 1576");
	RitPageChecks(L, P.M, "the Board");
	// Each plate opens its page, and the section slab names it.
	const std::pair<Page, const char*> Pages[8] = {{Page::Lobby, "THE BOARD"}, {Page::Live, "SUNDAY LIVE"}, {Page::Leaderboards, "RANKINGS"}, {Page::Desk, "THE DESK"},
		{Page::Series, "SERIES"}, {Page::News, "NEWSDESK"}, {Page::Promotions, "THE CLUB"}, {Page::Cashier, "CASHIER"}};
	const char* const Files[8] = {"rit_board", "rit_live", "rit_rankings", "rit_desk", "rit_series", "rit_news", "rit_club", "rit_cashier"};
	for (int I = 7; I >= 0; --I)
	{
		P.Click(100.0f, 84.0f + 40.0f * static_cast<float>(I) + 18.0f);
		const DrawList Lp = P.Settled();
		RitExpect(P.RL.CurrentPage() == Pages[I].first && DrewRun(Lp, Pages[I].second), (std::string("rit: plate ") + std::to_string(I + 1) + " opens " + Pages[I].second).c_str());
		WriteList(Files[I], Lp);
		RitPageChecks(Lp, P.M, Pages[I].second);
	}
	// The League is G2's: asked for at G1 (ss.Laptop.Open), the client shows its Board.
	RitExpect(P.RL.DebugOpen("riverline league runittwice"), "rit: ss.Laptop.Open names the League");
	L = P.Settled();
	RitExpect(P.RL.CurrentPage() == Page::Lobby && DrewRun(L, "THE BOARD"), "rit: the League (G2) shows the Board at G1");
	// The Member File, from the member block.
	P.Click(100.0f, 872.0f);
	L = P.Settled();
	WriteList("rit_member", L);
	RitExpect(P.RL.CurrentPage() == Page::Profile && DrewRun(L, "MEMBER FILE") && RitDrewSub(L, "Joined RunItTwice "), "rit: the Member File, joined RunItTwice on its date");
	RitPageChecks(L, P.M, "the Member File");
	// RiverLine's pages asked for on RunItTwice's client go to the Board.
	P.RL.OpenPage(Page::Career, P.Now);
	P.Settled();
	RitExpect(P.RL.CurrentPage() == Page::Lobby, "rit: Career is RiverLine's");
}

// ------------------------------------------------------------------ the Board

/** A run that is exactly Text inside the Board's grid. */
bool RitRunInRows(const DrawList& L, const std::string& Text)
{
	for (const DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == DrawCmd::Kind::Text && Cmd.Text.Text == Text && Cmd.Text.BaselineY >= 252.0f && Cmd.Text.BaselineY <= 932.0f && Cmd.Text.X < 1112.0f)
		{
			return true;
		}
	}
	return false;
}

void RitBoard()
{
	RitLaptop P("rit-board");
	P.At(RitEvening + 60.0); // 8:30 PM: the evening's grid in late registration
	P.S.BankrollCents = 150000;
	P.Join(21240);
	P.Open(Page::Lobby);
	P.Frame();
	// The settle wave: blank flaps first, then each row's status, one RitFlap a row (rate-limited to one per 50 ms).
	P.Now += 0.1;
	DrawList Early = P.Frame();
	const bool Blank = !DrewRun(Early, "BOARDING") && !DrewRun(Early, "SCHEDULED");
	for (int K = 0; K < 40; ++K)
	{
		P.Now += 1.0 / 30.0;
		P.Frame();
	}
	DrawList L = P.Frame();
	RitExpect(Blank && (DrewRun(L, "BOARDING") || DrewRun(L, "SCHEDULED") || DrewRun(L, "FINAL CALL")), "rit: the statuses settle into place on open");
	RitExpect(P.H.Count(ss::SoundId::RitFlap) >= 1, "rit: the wave clacks (RitFlap)");
	// Listed: the Board's own events, in its order; the arrows walk it.
	const std::vector<std::string> Listed = P.RL.ListedEvents();
	RitExpect(!Listed.empty() && std::all_of(Listed.begin(), Listed.end(), [](const std::string& Id) { return ss::net::SiteOf(Id) == SiteId::RunItTwice; }),
		"rit: the Board writes Listed with RunItTwice's events only");
	if (Listed.size() >= 3)
	{
		P.RL.SelectEvent(Listed[0]);
		P.Frame();
		P.RL.Key("ArrowDown");
		const bool Down = P.RL.SelectedEvent() == Listed[1];
		P.RL.Key("ArrowUp");
		RitExpect(Down && P.RL.SelectedEvent() == Listed[0], "rit: the arrows walk the Board");
	}
	// A click on a row selects it (RitBlip); the selection lights: its fixed ground and the lead line into the Prospectus.
	const int Blips = P.H.Count(ss::SoundId::RitBlip);
	P.Click(600.0f, 252.0f + 34.0f * 1.0f + 17.0f);
	L = P.Settled();
	RitExpect(Listed.size() >= 2 && P.RL.SelectedEvent() == Listed[1] && P.H.Count(ss::SoundId::RitBlip) == Blips + 1, "rit: a row click selects it with a RitBlip");
	RitExpect(RitHasHex(L, ss::ui::rlrit_detail::RitSelRow, {232.0f, 252.0f, 880.0f, 680.0f}) && RitHasHex(L, ss::ui::rlrit_detail::RitRed, {1112.0f, 252.0f, 22.0f, 680.0f}),
		"rit: the selected row lights and its lead line meets the Prospectus");
	// The OVERLAY column is site::show's projection (§9.6), "PROJ. ±" while registration is open.
	const ss::net::Network& Net = ss::net::Site(SiteId::RunItTwice);
	int Shown = 0;
	int Same = 0;
	for (size_t I = 0; I < P.RL.ListedEvents().size() && I < 20; ++I) // the rows on screen
	{
		const std::string& Id = P.RL.ListedEvents()[I];
		ss::net::EventInstance E;
		if (!Net.FindInstance(Id, E))
		{
			continue;
		}
		const ss::site::show::OverlayView V = ss::site::show::OverlayOf(Net, E, P.S.WorldMinutes());
		bool Actual = false;
		const ss::Chips Proj = ss::site::show::ProjectedOverlay(Net, E, V.Registered, P.S.WorldMinutes(), Actual);
		if (V.Shown && V.Cents > 0)
		{
			++Shown;
			Same += Proj == V.Cents && RitRunInRows(L, "-" + ss::Money(V.Cents)) ? 1 : 0;
		}
	}
	std::printf("  rit board: %zu listed, %d projected short\n", P.RL.ListedEvents().size(), Shown);
	RitExpect(Same == Shown, "rit: every OVERLAY figure is site::show::ProjectedOverlay's");
	WriteList("rit_board_selected", L);
	RitPageChecks(L, P.M, "the Board, a row selected");
	// Budgets (§10: the Board <= 22k vertices and 345 runs at rest).
	std::printf("  rit board at rest: %zu vertices, %d text runs, %d twice prints\n", L.Vertices.size(), RitRuns(L), ss::ui::rlrit_detail::RitTallyNow().Twice);
	RitExpect(L.Vertices.size() <= 22000 && RitRuns(L) <= 345, "rit: the Board within its budget (22k vertices, 345 runs at rest)");
	// The tape and the KPI tile count the same seated players (both: the events past late registration).
	const auto Figure = [&L](const std::string& Tail, bool Prefix) {
		for (const DrawCmd& Cmd : L.Cmds)
		{
			const std::string& T = Cmd.Text.Text;
			const size_t At = Cmd.Type == DrawCmd::Kind::Text ? T.find(Tail) : std::string::npos;
			if (At != std::string::npos && At > 0)
			{
				const size_t From = Prefix ? T.rfind(' ', At - 1) : std::string::npos;
				return T.substr(From == std::string::npos ? 0 : From + 1, At - (From == std::string::npos ? 0 : From + 1));
			}
		}
		return std::string();
	};
	const std::string OnTape = Figure(" PLAYERS SEATED", true);
	const std::string OnTile = Figure(" players seated", false);
	std::printf("  rit: seated on the tape \"%s\", on the KPI tile \"%s\"\n", OnTape.c_str(), OnTile.c_str());
	RitExpect(!OnTile.empty() && (OnTape.empty() || OnTape == OnTile), "rit: the tape and the RUNNING tile agree on the players seated");
}

void RitFinalCallAndNight()
{
	// FINAL CALL blinks at 1 Hz (Signal Red, then its dim phase); find a row in its last ten minutes of late registration.
	RitLaptop P("rit-final-call");
	P.At(RitEvening);
	P.S.BankrollCents = 150000;
	P.Join(21240);
	P.Open(Page::Lobby);
	bool Found = false;
	for (int Minute = 0; Minute < 180 && !Found; Minute += 2)
	{
		P.At(RitEvening + static_cast<double>(Minute));
		const DrawList L = P.Settled();
		Found = DrewRun(L, "FINAL CALL");
	}
	if (!Found)
	{
		std::printf("  rit: no FINAL CALL in three hours of this evening; the blink check is skipped\n");
	}
	else
	{
		P.Now = std::floor(P.Now) + 2.2;
		const DrawList On = P.Frame();
		P.Now += 0.5;
		const DrawList Off = P.Frame();
		const ss::ui::Rect Status{972.0f, 252.0f, 136.0f, 680.0f};
		RitExpect(RitHasHex(On, ss::ui::rlrit_detail::RitRed, Status) && RitHasHex(Off, ss::ui::rlrit_detail::RitFinalDim, Status), "rit: FINAL CALL blinks between Signal Red and its dim phase");
		WriteList("rit_board_final_call", On);
		RitPageChecks(On, P.M, "the Board at FINAL CALL");
	}
	// Night service: after 1 AM the Panel2 rows take a red cast.
	RitLaptop N("rit-night");
	N.At(RitEvening + 6.5 * 60.0 + 30.0); // Thu 2:30 AM
	N.S.BankrollCents = 150000;
	N.Join(21240);
	N.Open(Page::Lobby);
	const DrawList Ln = N.Settled();
	WriteList("rit_board_night", Ln);
	const DrawList Ld = P.Settled();
	RitExpect(RitHasHex(Ln, ss::ui::rlrit_detail::RitNightRow, {232.0f, 252.0f, 880.0f, 680.0f}) && !RitHasHex(Ld, ss::ui::rlrit_detail::RitNightRow, {232.0f, 252.0f, 880.0f, 680.0f}),
		"rit: night service's red cast after 1 AM, not at night's start");
	RitPageChecks(Ln, N.M, "the Board after 1 AM");
}

// ------------------------------------------------------------------ the Club, the Cashier, Rankings: the sites' pins hold

void RitPins()
{
	{
		RitLaptop P("rit-club");
		P.At(RitEvening);
		P.S.BankrollCents = 50000;
		P.Join(0);
		P.S.Deposit(SiteId::RunItTwice, 5000, "card");
		P.Open(Page::Promotions);
		DrawList L = P.Settled();
		WriteList("rit_club_tickets", L);
		RitExpect(RitSpelled(L, "SECOND RUN CLUB") && RitSpelled(L, "WELCOME TICKETS") && DrewRun(L, "TICKET"), "rit: the Club keeps its kickers and the TICKET runs (the sites' pins)");
		// The card turns: 0.3 s, the text never drawn squashed (none while it turns).
		P.Click(492.0f, 280.0f);
		P.Now = P.Now - 0.05 + 0.1;
		const DrawList Mid = P.Frame();
		RitExpect(!RitDrewSub(Mid, "SIGNATURE") && !RitDrewSub(Mid, "SECOND RUN CLUB"), "rit: the Club card turns with no text squashed");
		L = P.Settled();
		RitExpect(DrewRun(L, "SIGNATURE"), "rit: turned, the card shows its back");
		RitPageChecks(L, P.M, "the Club, its card turned");
		RitLaptop Q("rit-club-missed");
		Q.At(RitEvening);
		Q.S.BankrollCents = 50000;
		Q.Join(0);
		Q.S.Deposit(SiteId::RunItTwice, 2000, "card");
		Q.Open(Page::Promotions);
		const DrawList Lq = Q.Settled();
		RitExpect(RitDrewSub(Lq, "under $50") && !DrewRun(Lq, "None held right now."), "rit: a first deposit under $50 says the tickets passed it by");
	}
	{
		RitLaptop P("rit-cashier");
		P.At(RitEvening);
		P.S.BankrollCents = 50000;
		P.Join(0);
		P.Open(Page::Cashier);
		DrawList L = P.Settled();
		const RitBox Fifty = RitRun(L, "$50", P.M, 380.0f, 440.0f);
		RitExpect(Fifty.Found, "rit: the Cashier's deposit steps sit in the sites' window (y 380-440)");
		if (Fifty.Found)
		{
			P.Click(Fifty.X + Fifty.W / 2.0f, Fifty.Base - 6.0f);
		}
		L = P.Settled();
		RitBox Go = RitRun(L, "Deposit $50", P.M);
		Go = Go.Found ? Go : RitRun(L, "DEPOSIT $50", P.M); // the look's caps
		if (Go.Found)
		{
			P.Click(Go.X + Go.W / 2.0f, Go.Base - 6.0f);
		}
		const ss::site::Account* A = ss::site::AccountOf(P.S.Life.Sites, SiteId::RunItTwice);
		RitExpect(Go.Found && A && A->Cents == 5000, "rit: the Cashier deposits through the session (the money call stays core's)");
		L = P.Settled();
		WriteList("rit_cashier_after", L);
		RitPageChecks(L, P.M, "the Cashier");
	}
	{
		// The Desk from History (G1, R7): its KPIs are site::show::Desk's, its line DeskNet's.
		RitLaptop P("rit-desk");
		P.At(RitEvening);
		ss::site::Account* A = P.Join(21240);
		const ss::net::Network& Net = ss::net::Site(SiteId::RunItTwice);
		int K = 0;
		double Last = -1.0e9;
		for (const ss::net::EventInstance& E : Net.Window(P.S.WorldMinutes() - 20.0 * RitDay, P.S.WorldMinutes() - 300.0))
		{
			const ss::net::EventTemplate& T = Net.TemplateOf(E);
			if (K >= 14 || T.BuyInCents <= 0 || T.BuyInCents > 5500 || !T.Series.empty() || E.Start - Last < 600.0)
			{
				continue;
			}
			Last = E.Start;
			ss::HistoryEntry H;
			H.Name = T.Name;
			H.EventId = E.Id;
			H.BuyInCents = T.BuyInCents;
			H.Entrants = 120;
			H.Place = K % 4 == 0 ? 3 : 40 + K;
			H.Prize = K % 4 == 0 ? T.BuyInCents * 6 : 0;
			P.S.History.insert(P.S.History.begin(), H);
			++K;
		}
		P.Open(Page::Desk);
		const DrawList L = P.Settled();
		WriteList("rit_desk_history", L);
		const ss::site::show::DeskKpis Kp = ss::site::show::Desk(P.S.History, *A, 60, P.S.WorldMinutes());
		RitExpect(K > 0 && Kp.Events == K && DrewRun(L, ss::Grouped(Kp.Events)), "rit: the Desk's EVENTS is site::show::Desk's, from History");
		RitPageChecks(L, P.M, "the Desk with History");
	}
	{
		RitLaptop P("rit-rankings");
		P.At(RitEvening);
		if (ss::site::Account* A = P.Join(0))
		{
			A->Hero.Year = ss::world::YearOf(ss::net::DayOf(P.S.WorldMinutes()));
			A->Hero.Points = 777.0;
			A->Hero.Events = 3;
		}
		P.Open(Page::Leaderboards);
		P.Settled();
		const DrawList L = P.Settled();
		WriteList("rit_rankings_you", L);
		RitExpect(RitRun(L, "777 pts", P.M, 915.0f, 930.0f).Found && RitRun(L, "777 pts", P.M, 0.0f, 914.0f).Found && RitDrewSub(L, "lobby opened on"),
			"rit: the YOU bar keeps the sites' window, the standings their own run, the board its words");
		RitPageChecks(L, P.M, "Rankings with the player on the board");
	}
}

// ------------------------------------------------------------------ the octagon: seat lists, the BB toggle, the Second Board

bool RitOverlap(const ss::ui::Rect& A, const ss::ui::Rect& B)
{
	return A.X < B.X + B.W && B.X < A.X + A.W && A.Y < B.Y + B.H && B.Y < A.Y + A.H;
}

void RitSeatLists()
{
	const ss::ui::rlsites_detail::SiteLook& L = ss::ui::rlsites_detail::SitesLook(SiteId::RunItTwice);
	if (!L.SeatList)
	{
		std::printf("  rit: RunItTwice's look has no seat lists yet (ui-tablestyle); the seat checks are skipped\n");
		return;
	}
	const ss::ui::Rect Third = L.LowerThird.W > 0.0f ? L.LowerThird : ss::ui::Rect{250.0f, 598.0f, 680.0f, 36.0f};
	const auto Clear = [&](const std::vector<ss::ui::Vec2>& Seats) {
		std::vector<ss::ui::Rect> Boxes;
		for (size_t I = 0; I < Seats.size(); ++I)
		{
			Boxes.push_back(ss::ui::rlsites_detail::SitesSeatBox(L, Seats[I], I == 0));
		}
		bool Ok = true;
		for (size_t I = 0; I < Boxes.size(); ++I)
		{
			Ok = Ok && Boxes[I].X + Boxes[I].W <= 1188.0f && Boxes[I].X >= 64.0f && !RitOverlap(Boxes[I], Third);
			for (size_t J = I + 1; J < Boxes.size(); ++J)
			{
				Ok = Ok && !RitOverlap(Boxes[I], Boxes[J]);
			}
		}
		return Ok;
	};
	RitExpect(Clear(std::vector<ss::ui::Vec2>(L.Seat8.begin(), L.Seat8.end())), "rit: 8 data cards clear of SidePanel, the collapsed rail, the lower third and each other");
	RitExpect(Clear(std::vector<ss::ui::Vec2>(L.Seat6.begin(), L.Seat6.end())), "rit: 6 data cards clear of SidePanel, the collapsed rail, the lower third and each other");
}

/** An open RunItTwice event the tables can run now, or false. */
bool RitOpenEvent(ss::Session& S, ss::net::EventInstance& Out, ss::Chips MaxBuyIn)
{
	const ss::net::Network& Net = ss::net::Site(SiteId::RunItTwice);
	const double World = S.WorldMinutes();
	for (const ss::net::EventInstance& E : Net.Window(World - 180.0, World + 60.0))
	{
		const ss::net::LiveState L = Net.Live(E, World);
		const bool Open = L.St == ss::net::Status::LateReg || (L.St == ss::net::Status::Registering && L.StartsIn <= 60.0);
		const ss::LobbyEvent Ev = Net.Listing(E, nullptr, S.Unlocks());
		if (Open && Ev.Joinable && Ev.BuyInCents > 0 && Ev.BuyInCents <= MaxBuyIn && Ev.Spec.Entrants <= 3000)
		{
			Out = E;
			return true;
		}
	}
	return false;
}

/** A finished heads-up hand: the first to act shoves and the other calls (Shove), or folds; a full deck, so the stub is real. */
std::unique_ptr<ss::Hand> RitHand(bool Shove, const std::string& Seed)
{
	ss::HandConfig Cfg;
	Cfg.Players = {{0, ss::HeroId, 4000}, {1, "rit_villain", 4000}};
	Cfg.ButtonSeat = 0;
	Cfg.SmallBlind = 50;
	Cfg.BigBlind = 100;
	ss::Rng R(Seed);
	std::unique_ptr<ss::Hand> H = std::make_unique<ss::Hand>(Cfg, R);
	for (int Guard = 0; Guard < 20 && !H->bComplete; ++Guard)
	{
		const bool First = H->History.empty() || Guard == 0;
		if (!Shove)
		{
			H->Act(ss::PlayerAction::Fold());
		}
		else if (First)
		{
			H->Act(ss::PlayerAction::RaiseTo(4000.0));
		}
		else
		{
			H->Act(ss::PlayerAction::Call());
		}
	}
	return H;
}

void RitTable()
{
	RitSeatLists();
	RitLaptop P("rit-table");
	P.At(RitEvening);
	P.S.BankrollCents = 200000;
	P.Join(50000);
	ss::net::EventInstance E;
	if (!RitOpenEvent(P.S, E, 3300))
	{
		std::printf("  rit: no RunItTwice event open at this hour; the table checks are skipped\n");
		return;
	}
	const ss::LobbyEvent Ev = ss::net::Site(SiteId::RunItTwice).Listing(E, nullptr, P.S.Unlocks());
	const std::string Why = P.S.RegisterEvent(Ev);
	RitExpect(Why.empty() && P.S.T != nullptr, "rit: a RunItTwice table opens");
	if (!P.S.T)
	{
		return;
	}
	P.S.CurrentPace = ss::Pace::Full;
	P.Now = Step(P.S, P.Now, 3.0);
	DrawList L = P.Settled();
	WriteList("rit_table", L);
	RitExpect(RitDrewSub(L, " LEFT \xC2\xB7 "), "rit: the header carries the table's own line");
	RitPageChecks(L, P.M, "the octagon");
	std::printf("  rit table: %zu vertices, %d text runs\n", L.Vertices.size(), RitRuns(L));
	RitExpect(L.Vertices.size() <= 19000, "rit: the octagon within its budget (19k vertices)");
	if (std::getenv("SS_UI_BENCH"))
	{
		double Best = 1.0e9;
		for (int Round = 0; Round < 5; ++Round)
		{
			const auto T0 = std::chrono::steady_clock::now();
			for (int K = 0; K < 40; ++K)
			{
				P.Frame();
			}
			Best = std::min(Best, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count() / 40.0);
		}
		std::printf("UI BENCH runittwice table %6.3f ms/draw, %zu vertices, %d runs\n", Best, L.Vertices.size(), RitRuns(L));
	}
	// A card over the table: the tape and the header draw under its backdrop (from y 64), and a one-frame click on the
	// backdrop over the header closes the card and does nothing else.
	{
		P.RL.ShowPlayer(ss::ui::RiverLine::HeroCard, P.Now);
		const DrawList Lc = P.Settled();
		size_t Header = 0;
		size_t Scrim = 0;
		for (size_t I = 0; I < Lc.Cmds.size(); ++I)
		{
			const DrawCmd& Cmd = Lc.Cmds[I];
			if (Cmd.Type == DrawCmd::Kind::Text && (Cmd.Text.Text == "BETS IN BIG BLINDS" || Cmd.Text.Text == "BETS IN CHIPS"))
			{
				Header = I;
			}
			if (Cmd.Type == DrawCmd::Kind::Triangles && Scrim == 0)
			{
				for (uint32_t K = 0; K < Cmd.VertexCount && Scrim == 0; ++K)
				{
					const ss::ui::Vertex& V = Lc.Vertices[Cmd.VertexStart + K];
					Scrim = V.X <= 0.5f && std::fabs(V.Y - 64.0f) < 0.5f && RitRgb(V.Col) == 0x040810 ? I : 0;
				}
			}
		}
		RitExpect(P.RL.CardOpen() && Header > 0 && Scrim > Header, "rit: at the table, a card's backdrop draws over the tape and the header");
		const int Focus = P.S.FocusedTable();
		P.Click(300.0f, 72.0f);
		P.Settled();
		RitExpect(!P.RL.CardOpen() && P.S.CurrentScreen == ss::Screen::Table && P.S.FocusedTable() == Focus, "rit: a click on the card's backdrop over the header only closes the card");
	}
	// The BB toggle (bets and stacks in big blinds by default; ui-tablestyle's state and pill): the Exchange's own figures follow.
	const ss::ui::rlsites_detail::SiteLook& Look = ss::ui::rlsites_detail::SitesLook(SiteId::RunItTwice);
	const auto RailStack = [](const DrawList& D) {
		for (const DrawCmd& Cmd : D.Cmds)
		{
			if (Cmd.Type == DrawCmd::Kind::Text && Cmd.Text.X < 216.0f && Cmd.Text.X > 100.0f && Cmd.Text.BaselineY > 525.0f && Cmd.Text.BaselineY < 545.0f)
			{
				return Cmd.Text.Text;
			}
		}
		return std::string();
	};
	if (Look.BetsInBb)
	{
		const bool Before = DrewRun(L, "BETS IN BIG BLINDS");
		ss::ui::rlsites_detail::SitesToggleBets(P.RL);
		const DrawList Lc = P.Frame();
		P.S.ShowLobby();
		P.Open(Page::Lobby);
		const DrawList Lb = P.Settled();
		const std::string InChips = RailStack(Lb);
		ss::ui::rlsites_detail::SitesToggleBets(P.RL);
		const std::string InBb = RailStack(P.Frame());
		std::printf("  rit: the rail's stack in chips \"%s\", in big blinds \"%s\"\n", InChips.c_str(), InBb.c_str());
		RitExpect(Before && DrewRun(Lc, "BETS IN CHIPS") && !InChips.empty() && InChips.find(" BB") == std::string::npos && InBb.find(" BB") != std::string::npos,
			"rit: the BB toggle: the header and the rail's stacks follow it, both ways");
		P.S.ShowTables();
	}
	else
	{
		std::printf("  rit: RunItTwice's look has no BB default yet (ui-tablestyle); the toggle check is skipped\n");
	}
	// The Second Board: only after an all-in with cards to come; never a chip moved.
	std::vector<ss::Chips> Stacks;
	for (const ss::SeatVis& V : P.S.Seats)
	{
		Stacks.push_back(V.Stack);
	}
	const int Remaining = P.S.T->Remaining;
	const auto Inject = [&](bool Shove, int Hand) {
		P.S.CurHand = RitHand(Shove, "rit-second-" + std::to_string(Hand));
		P.S.Board = P.S.CurHand->Board;
		P.S.BoardShownAt.assign(P.S.Board.size(), 0.0);
		P.S.HandsPlayed = 9000 + Hand;
		return P.Frame();
	};
	const DrawList Fold = Inject(false, 1);
	RitExpect(!DrewRun(Fold, "IF IT RAN TWICE"), "rit: no Second Board after a hand with no all-in");
	// Mid run-out: an all-in's replay runs on a hand the engine has already completed; with the flop up (its last card shown)
	// nothing of the second run may show yet (it would give the river away).
	P.S.CurHand = RitHand(true, "rit-second-3");
	P.S.Board.assign(P.S.CurHand->Board.begin(), P.S.CurHand->Board.begin() + std::min<size_t>(3, P.S.CurHand->Board.size()));
	P.S.BoardShownAt.assign(P.S.Board.size(), 0.0);
	P.S.HandsPlayed = 9003;
	const DrawList Mid = P.Frame();
	RitExpect(P.S.CurHand->bComplete && !DrewRun(Mid, "IF IT RAN TWICE"), "rit: no Second Board while an all-in's run-out is still dealing (the flop up)");
	const DrawList Shove = Inject(true, 2);
	ss::site::show::SecondBoard Out;
	const bool Runs = ss::site::show::SecondRun(*P.S.CurHand, 0, "rit-check", Out) && Out.HeroIn;
	RitExpect(P.S.CurHand->bComplete && P.S.Board.size() == 5, "rit: the injected all-in ran to a five-card board");
	RitExpect(DrewRun(Shove, "IF IT RAN TWICE") == Runs, "rit: the Second Board shows exactly when site::show::SecondRun gives one");
	if (!Runs)
	{
		std::printf("  rit: site::show::SecondRun gave no board for the all-in (core-sites-look's body)\n");
	}
	bool Same = P.S.T->Remaining == Remaining && P.S.Seats.size() == Stacks.size();
	for (size_t I = 0; I < P.S.Seats.size() && Same; ++I)
	{
		Same = P.S.Seats[I].Stack == Stacks[I];
	}
	RitExpect(Same, "rit: the Second Board never moves a chip");
	WriteList("rit_table_second", Shove);
	RitPageChecks(Shove, P.M, "the octagon with the Second Board");
	// Four seconds on, it has gone.
	P.Now += 4.5;
	const DrawList Later = P.Frame();
	RitExpect(!DrewRun(Later, "IF IT RAN TWICE"), "rit: the Second Board fades after 4 s");
	// Knockouts (§3.5, g1-audit L1): a seat that ends a hand with no chips and is gone when the next is dealt leaves as its
	// card, desaturated, sliding out from the felt's centre and fading (a 0.1 s beat, then 0.6 s), its OUT tag with it.
	int Gone = -1;
	for (size_t I = 0; I < P.S.Seats.size() && Gone < 0; ++I)
	{
		Gone = P.S.Seats[I].Present && !P.S.Seats[I].IsHero ? static_cast<int>(I) : -1;
	}
	RitExpect(Gone >= 0, "rit: the octagon has someone to knock out");
	if (Gone >= 0)
	{
		ss::SeatVis& V = P.S.Seats[static_cast<size_t>(Gone)];
		V.Stack = 0;
		P.Frame();
		V.Present = false;
		const DrawList K0 = P.Frame();
		P.Now += 0.4;
		const DrawList K1 = P.Frame();
		P.Now += 0.5;
		const DrawList K2 = P.Frame();
		const RitBox T0 = RitRun(K0, "OUT", P.M);
		const RitBox T1 = RitRun(K1, "OUT", P.M);
		const auto Far = [&Look](const RitBox& B) { return std::hypot(B.X - Look.Cx, B.Base - Look.Cy); };
		std::printf("  rit knockout: the OUT tag %.1f from the centre, then %.1f\n", T0.Found ? Far(T0) : -1.0, T1.Found ? Far(T1) : -1.0);
		RitExpect(T0.Found && T1.Found && Far(T1) > Far(T0) + 20.0f && !DrewRun(K2, "OUT"), "rit: a busted seat's card slides out and is gone within 0.7 s");
		WriteList("rit_table_knockout", K0);
		RitPageChecks(K0, P.M, "the octagon, a knockout");
	}
}

// ------------------------------------------------------------------ the Settlement

void RitSettlementChecks()
{
	RitLaptop P("rit-settlement");
	P.At(RitEvening);
	P.S.BankrollCents = 123400;
	P.Join(67640);
	const ss::net::Network& Net = ss::net::Site(SiteId::RunItTwice);
	std::string Id;
	for (const ss::net::EventInstance& E : Net.Window(P.S.WorldMinutes() - 600.0, P.S.WorldMinutes()))
	{
		Id = Net.TemplateOf(E).Name == "The Nightcap" || Id.empty() ? E.Id : Id;
	}
	ss::HistoryEntry H;
	H.Name = "The Nightcap";
	H.Place = 4;
	H.Entrants = 47;
	H.Prize = 67200;
	H.BuyInCents = 10900;
	H.EventId = Id;
	P.S.History.insert(P.S.History.begin(), H);
	P.S.LastResults = ss::Results();
	P.S.LastResults.EventName = H.Name;
	P.S.LastResults.Place = H.Place;
	P.S.LastResults.Entrants = H.Entrants;
	P.S.LastResults.PrizeCents = H.Prize;
	P.S.LastResults.BuyInCents = H.BuyInCents;
	P.S.HasResults = true;
	P.S.LastSiteResult = SiteId::RunItTwice; // ShowResults sets it with HasResults
	P.S.CurrentScreen = ss::Screen::Results;
	P.Frame();
	P.Now += 0.3;
	const DrawList Printing = P.Frame();
	RitExpect(!DrewRun(Printing, "FINAL TABLE") && P.H.Count(ss::SoundId::RitStamp) == 0, "rit: the Settlement prints before it stamps");
	const DrawList L = P.Settled();
	WriteList("rit_settlement", L);
	RitExpect(DrewRun(L, "SETTLEMENT") && DrewRun(L, "FINAL TABLE") && DrewRun(L, "RunItTwice balance now $676.40"), "rit: the Settlement prints, stamps FINAL TABLE, and says the balance");
	RitExpect(P.H.Count(ss::SoundId::RitStamp) == 1, "rit: the stamp thunks once (RitStamp)");
	RitPageChecks(L, P.M, "the Settlement", 2);
	// The rail works from the Settlement: a plate leaves the results (as BACK TO THE BOARD does) and opens its page.
	P.Click(100.0f, 84.0f + 40.0f * 2.0f + 18.0f);
	P.Settled();
	RitExpect(!P.S.HasResults && P.S.CurrentScreen == ss::Screen::Lobby && P.RL.CurrentPage() == Page::Leaderboards, "rit: a plate on the Settlement leaves it for its page");
}

// ------------------------------------------------------------------ the Second Boards record (the lead's PIE, 2026-10-21)
// The Settlement's SECOND BOARDS column showed A♥A♠ v K♠5♣, "100% | 0%", RAN 4♦ 6♣ Q♥ 9♥ 3♥ "LOST", then "you'd have WON
// BOTH". A record has to agree with its hand: both hands, the equity when the cards went on their backs, what the first run
// paid and the verdict, for a win, a loss and a split, and the words for every pair of runs. The truth here is worked from
// the finished hand itself (its seats, its pots, autoplay's all-in equity), never from the table's display state.

/** The player's index in H.Seats (-1: not dealt in). */
int RitMe(const ss::Hand& H)
{
	for (size_t I = 0; I < H.Seats.size(); ++I)
	{
		if (H.Seats[I].Id == ss::HeroId)
		{
			return static_cast<int>(I);
		}
	}
	return -1;
}

/** What the engine paid the player on the real run, from the hand's pots: 2 every pot they could win, 0 none, 1 part. */
int RitPaid(const ss::Hand& H, int Me)
{
	const int Seat = H.Seats[static_cast<size_t>(Me)].Seat;
	ss::Chips Could = 0;
	ss::Chips Mine = 0;
	for (const ss::PotResult& Pr : H.PotResults)
	{
		if (std::find(Pr.Eligible.begin(), Pr.Eligible.end(), Seat) == Pr.Eligible.end())
		{
			continue;
		}
		Could += Pr.Amount;
		for (size_t K = 0; K < Pr.Winners.size() && K < Pr.Shares.size(); ++K)
		{
			Mine += Pr.Winners[K] == Seat ? Pr.Shares[K] : 0;
		}
	}
	return Could > 0 && Mine >= Could ? 2 : Mine <= 0 ? 0 : 1;
}

/** A record's truth, from the finished hand: the hands (the first opponent still in, by seat order), the board, the pay, the equity. */
struct RitTruth
{
	std::array<int, 2> Hero{{-1, -1}};
	std::array<int, 2> Villain{{-1, -1}};
	std::vector<int> Ran;
	int Paid = 0;
	double Equity = -1.0;
};
RitTruth RitTruthOf(const ss::Hand& H, int Me)
{
	RitTruth T;
	const ss::HandSeat& Mine = H.Seats[static_cast<size_t>(Me)];
	T.Hero = {Mine.Hole.size() > 0 ? Mine.Hole[0] : -1, Mine.Hole.size() > 1 ? Mine.Hole[1] : -1};
	for (size_t I = 0; I < H.Seats.size(); ++I)
	{
		const ss::HandSeat& S = H.Seats[I];
		if (static_cast<int>(I) != Me && !S.Folded && S.Hole.size() == 2 && T.Villain[0] < 0)
		{
			T.Villain = {S.Hole[0], S.Hole[1]};
		}
	}
	T.Ran.assign(H.Board.begin(), H.Board.end());
	T.Paid = RitPaid(H, Me);
	T.Equity = ss::autoplay::AllInEquity(H, Me);
	return T;
}

const char* RitRanWord(int Paid)
{
	return Paid >= 2 ? "WON" : Paid == 1 ? "SPLIT" : "LOST";
}

const char* RitVerdictWord(int First, int Second)
{
	return First == 2 && Second == 2 ? "WON BOTH" : First == 0 && Second == 0 ? "LOST BOTH" : "SPLIT IT";
}

std::string RitEquityText(double Eq)
{
	return ss::Fixed(Eq * 100.0, 0) + "% | " + ss::Fixed(100.0 - Eq * 100.0, 0) + "%";
}

std::string RitCards(const std::vector<int>& Cards)
{
	std::string Out;
	for (const int C : Cards)
	{
		Out += C >= 0 ? ss::CardToString(C) : std::string("--");
	}
	return Out;
}

/** The texts drawn in the Settlement's SECOND BOARDS column (x 232-514, under its heading). */
std::vector<std::string> RitColumn(const DrawList& L)
{
	std::vector<std::string> Out;
	for (const DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == DrawCmd::Kind::Text && Cmd.Text.X >= 232.0f && Cmd.Text.X < 514.0f && Cmd.Text.BaselineY > 140.0f && Cmd.Text.BaselineY < 880.0f)
		{
			Out.push_back(Cmd.Text.Text);
		}
	}
	return Out;
}

bool RitColumnHas(const std::vector<std::string>& Col, const std::string& Text)
{
	return std::find(Col.begin(), Col.end(), Text) != Col.end();
}

/** Two records of one deal, field for field (the minute aside: each path stamps its own clock). */
bool RitSameRecord(const ss::ui::rlrit_detail::RitSecond& A, const ss::ui::rlrit_detail::RitSecond& B)
{
	return A.EventId == B.EventId && A.HandId == B.HandId && A.Hero == B.Hero && A.Villain == B.Villain && A.Ran == B.Ran && A.Twice == B.Twice && A.Verdict == B.Verdict &&
		A.First == B.First && A.Equity == B.Equity;
}

/**
 * A heads-up deck for a preflop all-in, the villain dealt first (KsAsKdAd: the villain Ks Kd, the player As Ad): the real
 * board Run1 and, past the hand's DeckPos with a burn before each street (spike S-D's walk), the second run Run2. The burns
 * are the first cards nobody else uses.
 */
std::vector<ss::Card> RitCraftDeck(const std::string& Holes, const std::string& Run1, const std::string& Run2)
{
	const std::vector<ss::Card> Hole = ss::ParseCards(Holes);
	const std::vector<ss::Card> A = ss::ParseCards(Run1);
	const std::vector<ss::Card> B = ss::ParseCards(Run2);
	std::vector<ss::Card> Used = Hole;
	Used.insert(Used.end(), A.begin(), A.end());
	Used.insert(Used.end(), B.begin(), B.end());
	std::vector<ss::Card> Spare;
	for (const ss::Card C : ss::FullDeck())
	{
		if (std::find(Used.begin(), Used.end(), C) == Used.end())
		{
			Spare.push_back(C);
		}
	}
	size_t Next = 0;
	std::vector<ss::Card> Deck = Hole;
	const auto BurnAndDeal = [&](const std::vector<ss::Card>& Run, size_t From, size_t Count) {
		Deck.push_back(Spare[Next++]);
		Deck.insert(Deck.end(), Run.begin() + static_cast<std::ptrdiff_t>(From), Run.begin() + static_cast<std::ptrdiff_t>(From + Count));
	};
	BurnAndDeal(A, 0, 3);
	BurnAndDeal(A, 3, 1);
	BurnAndDeal(A, 4, 1);
	BurnAndDeal(B, 0, 3);
	BurnAndDeal(B, 3, 1);
	BurnAndDeal(B, 4, 1);
	Deck.insert(Deck.end(), Spare.begin() + static_cast<std::ptrdiff_t>(Next), Spare.end());
	return Deck;
}

/**
 * The table as the Session leaves it at an all-in's showdown, before the award (Session.cpp's Reveal, Street and Showdown
 * events): the hands in it face up with their labels (the lower third down), the equities run to the river's 1 / 0 / 0.5
 * (UpdateEquities on five cards), nobody a Winner yet; the seats not in the hand folded. The board's last card is up.
 */
void RitAtShowdown(ss::Session& S, std::unique_ptr<ss::Hand> H)
{
	std::vector<std::vector<ss::Card>> Holes;
	std::vector<int> Live;
	for (ss::SeatVis& V : S.Seats)
	{
		if (!V.Present)
		{
			continue;
		}
		const ss::HandSeat* In = H->SeatByNumber(V.Seat);
		V.Folded = !In || In->Folded;
		V.Hole = In && !In->Folded ? In->Hole : std::vector<ss::Card>();
		V.HandLabel = In && !In->Folded ? std::string("Shown") : std::string();
		V.Winner = false;
		V.HasEquity = false;
		if (In && !In->Folded)
		{
			Holes.push_back(In->Hole);
			Live.push_back(V.Seat);
		}
	}
	ss::Rng R("rit-river-equity");
	const std::vector<double> Eq = ss::EquityKnownHands(Holes, H->Board, R, 100);
	for (size_t I = 0; I < Live.size() && I < Eq.size(); ++I)
	{
		S.Seats[static_cast<size_t>(Live[I])].HasEquity = true;
		S.Seats[static_cast<size_t>(Live[I])].Equity = Eq[I];
	}
	S.Board = H->Board;
	S.BoardShownAt.assign(S.Board.size(), 0.0);
	S.CurHand = std::move(H);
}

/** The award lands (Session.cpp's Award event): each pot's winners marked. */
void RitAwardLands(ss::Session& S)
{
	for (const ss::PotResult& Pr : S.CurHand->PotResults)
	{
		for (const int W : Pr.Winners)
		{
			S.Seats[static_cast<size_t>(W)].Winner = true;
		}
	}
}

/** The Settlement for the event at the front table (its History row on top), drawn settled; the table comes back after. */
DrawList RitSettleHere(RitLaptop& P)
{
	ss::HistoryEntry H;
	H.Name = P.S.T->Spec.Name;
	H.Place = 24;
	H.Entrants = 54;
	H.BuyInCents = 330;
	H.EventId = P.S.T->Spec.Id;
	P.S.History.insert(P.S.History.begin(), H);
	P.S.LastResults = ss::Results();
	P.S.LastResults.EventName = H.Name;
	P.S.LastResults.Place = H.Place;
	P.S.LastResults.Entrants = H.Entrants;
	P.S.LastResults.BuyInCents = H.BuyInCents;
	P.S.HasResults = true;
	P.S.LastSiteResult = SiteId::RunItTwice;
	const ss::Screen Was = P.S.CurrentScreen;
	P.S.CurrentScreen = ss::Screen::Results;
	const DrawList L = P.Settled();
	P.S.CurrentScreen = Was;
	P.S.HasResults = false;
	P.S.History.erase(P.S.History.begin());
	return L;
}

/** A RunItTwice table the player sits (the evening's open event), or false. */
bool RitSit(RitLaptop& P)
{
	P.At(RitEvening);
	P.S.BankrollCents = 200000;
	P.Join(50000);
	ss::net::EventInstance E;
	if (!RitOpenEvent(P.S, E, 3300))
	{
		return false;
	}
	const ss::LobbyEvent Ev = ss::net::Site(SiteId::RunItTwice).Listing(E, nullptr, P.S.Unlocks());
	if (!P.S.RegisterEvent(Ev).empty() || !P.S.T)
	{
		return false;
	}
	P.S.CurrentPace = ss::Pace::Full;
	return true;
}

/** One pair of runs on crafted decks: the player's As Ad against Ks Kd, all in preflop. */
struct RitPair
{
	const char* Name;
	const char* Run1;
	const char* Run2;
	int First;   // what the real run paid the player: 2 won, 1 split, 0 lost
	int Second;  // the second run alone
	int Verdict; // 2 won both, 0 lost both, 1 split it
};

/**
 * (1) and (3): a hand on a crafted deck at a live RunItTwice table, at the moment the table reads it (the showdown, before
 * the award): the record keeps the player's and the villain's hands, the real board, the second run, the all-in's equity
 * (autoplay::AllInEquity, the Luck Ledger's number, never the river's 100 / 0) and the verdict; the award landing after
 * changes nothing; and the Settlement prints RAN's word for what the first run paid and the verdict for the pair.
 */
void RitSecondRecordPairs()
{
	RitLaptop P("rit-second-pairs");
	if (!RitSit(P))
	{
		std::printf("  rit: no RunItTwice event open at this hour; the Second Board record checks are skipped\n");
		return;
	}
	P.Now = Step(P.S, P.Now, 3.0);
	P.Settled();
	// Win boards (the aces hold), loss boards (a third king), chop boards (a straight flush on board plays for both).
	const char* W1 = "2c7h9d3s4c";
	const char* W2 = "2h8cTd3d5s";
	const char* L1 = "Kh7c2d3c9h";
	const char* L2 = "Kc8d4d6cJs";
	const char* C1 = "5h6h7h8h9h";
	const char* C2 = "TcJcQcKcAc";
	const RitPair Pairs[] = {
		{"won / won", W1, W2, 2, 2, 2},
		{"won / lost", W1, L2, 2, 0, 1},
		{"lost / won", L1, W2, 0, 2, 1},
		{"lost / lost", L1, L2, 0, 0, 0},
		{"split / split", C1, C2, 1, 1, 1},
		{"split / won", C1, W2, 1, 2, 1},
		{"won / split", W1, C2, 2, 1, 1},
		{"split / lost", C1, L2, 1, 0, 1},
		{"lost / split", L1, C2, 0, 1, 1},
	};
	int HeroSeat = -1;
	int VillainSeat = -1;
	std::string VillainId;
	for (const ss::SeatVis& V : P.S.Seats)
	{
		HeroSeat = V.Present && V.IsHero ? V.Seat : HeroSeat;
		if (V.Present && !V.IsHero && VillainSeat < 0)
		{
			VillainSeat = V.Seat;
			VillainId = V.Id;
		}
	}
	RitExpect(HeroSeat >= 0 && VillainSeat >= 0, "rit: the table seats the player and someone to play");
	if (HeroSeat < 0 || VillainSeat < 0)
	{
		return;
	}
	ss::ui::rlrit_detail::RitUi& U = ss::ui::rlrit_detail::RitOf(&P.RL);
	int Hand = 0;
	int Good = 0;
	for (const RitPair& K : Pairs)
	{
		ss::HandConfig Cfg;
		Cfg.Players = {{HeroSeat, ss::HeroId, 4000}, {VillainSeat, VillainId, 4000}};
		Cfg.ButtonSeat = HeroSeat;
		Cfg.SmallBlind = 50;
		Cfg.BigBlind = 100;
		Cfg.Deck = RitCraftDeck("KsAsKdAd", K.Run1, K.Run2);
		ss::Rng R(std::string("rit-pair:") + K.Name);
		std::unique_ptr<ss::Hand> H = std::make_unique<ss::Hand>(Cfg, R);
		H->Act(ss::PlayerAction::RaiseTo(4000.0));
		H->Act(ss::PlayerAction::Call());
		const int Me = RitMe(*H);
		const bool Dealt = H->bComplete && Me >= 0 && RitCards(std::vector<int>(H->Board.begin(), H->Board.end())) == K.Run1;
		const RitTruth T = Dealt ? RitTruthOf(*H, Me) : RitTruth();
		const bool Crafted = Dealt && T.Paid == K.First && T.Hero == std::array<int, 2>{{ss::ParseCards("As")[0], ss::ParseCards("Ad")[0]}} &&
			T.Villain == std::array<int, 2>{{ss::ParseCards("Ks")[0], ss::ParseCards("Kd")[0]}} && T.Equity > 0.75 && T.Equity < 0.9;
		RitExpect(Crafted, (std::string("rit: the crafted hand deals as planned: ") + K.Name).c_str());
		if (!Crafted)
		{
			continue;
		}
		// The table reads it at the showdown, before the award.
		U.Seconds.clear();
		P.S.SecondBoards.clear();
		P.S.HandsPlayed = 9100 + Hand++;
		RitAtShowdown(P.S, std::move(H));
		P.Frame();
		const bool Read = U.Seconds.size() == 1;
		const ss::ui::rlrit_detail::RitSecond Rec = Read ? U.Seconds.back() : ss::ui::rlrit_detail::RitSecond();
		std::vector<int> Twice(Rec.Twice.begin(), Rec.Twice.end());
		const bool Hands = Read && Rec.Hero == T.Hero && Rec.Villain == T.Villain && Rec.Ran == T.Ran && RitCards(Twice) == K.Run2;
		const bool Equity = Read && std::fabs(Rec.Equity - T.Equity) < 1e-12;
		const bool Verdict = Read && Rec.Verdict == K.Verdict && Rec.First == K.First;
		// The award lands a beat later: the record was read once and stays as it was.
		RitAwardLands(P.S);
		P.Now += 0.3;
		P.Frame();
		// With the award, the session's post-hand hook (AfterAward's HeroHandDone, the watch path's call): the Settlement's log
		// takes the hand, its record the table's.
		ss::Session::HandFacts Hf;
		Hf.EquityAtAllIn = ss::autoplay::AllInEquity(*P.S.CurHand, RitMe(*P.S.CurHand));
		P.S.HeroHandDone(*P.S.CurHand, Hf);
		const bool Logged = P.S.SecondBoards.size() == 1 && RitSameRecord(ss::ui::rlrit_detail::RitSecondOf(P.S.SecondBoards.back()), Rec);
		RitExpect(Logged, (std::string("rit: the session's log keeps the same record as the table read: ") + K.Name).c_str());
		const bool Once = U.Seconds.size() == 1;
		// The Settlement's column: RAN's word for the first run, the all-in's equity, the verdict for the pair.
		const std::vector<std::string> Col = RitColumn(RitSettleHere(P));
		const std::string Ran = RitRanWord(K.First);
		const std::string Says = RitVerdictWord(K.First, K.Second);
		bool Printed = RitColumnHas(Col, RitEquityText(T.Equity)) && RitColumnHas(Col, Says);
		for (const char* Word : {"WON", "SPLIT", "LOST"})
		{
			Printed = Printed && RitColumnHas(Col, Word) == (Ran == Word);
		}
		std::printf("  rit second board %-13s record: %s v %s, %s, ran %s, twice %s, verdict %d (want %d) | printed: %s ... %s ... %s%s\n", K.Name,
			RitCards({Rec.Hero[0], Rec.Hero[1]}).c_str(), RitCards({Rec.Villain[0], Rec.Villain[1]}).c_str(), Rec.Equity >= 0.0 ? RitEquityText(Rec.Equity).c_str() : "no equity",
			RitCards(Rec.Ran).c_str(), RitCards(Twice).c_str(), Rec.Verdict, K.Verdict, RitColumnHas(Col, "WON") ? "WON" : RitColumnHas(Col, "SPLIT") ? "SPLIT" : RitColumnHas(Col, "LOST") ? "LOST" : "?",
			RitColumnHas(Col, RitEquityText(T.Equity)) ? "equity ok" : "equity WRONG", RitColumnHas(Col, "WON BOTH") ? "WON BOTH" : RitColumnHas(Col, "LOST BOTH") ? "LOST BOTH" : RitColumnHas(Col, "SPLIT IT") ? "SPLIT IT" : "?",
			Printed ? "" : "   <-- disagrees with the hand");
		RitExpect(Hands, (std::string("rit: the Second Board record keeps the hand's own cards: ") + K.Name).c_str());
		RitExpect(Equity, (std::string("rit: the record's equity is the all-in's (AllInEquity), not the river's: ") + K.Name).c_str());
		RitExpect(Verdict && Once, (std::string("rit: the record's RAN and verdict are the pair's, read once: ") + K.Name).c_str());
		RitExpect(Printed, (std::string("rit: the Settlement prints RAN's word, the all-in's equity and the verdict for the pair: ") + K.Name).c_str());
		Good += Hands && Equity && Verdict && Once && Printed ? 1 : 0;
	}
	std::printf("  rit second board pairs: %d of %zu agree with their hands\n", Good, sizeof(Pairs) / sizeof(Pairs[0]));
}

/** The plan (Maniac) in the player's seat: watching at Speed, or skipping Hands deals. */
std::string RitPlan(ss::Session& S, ss::autoplay::Mode M, int Speed, ss::autoplay::Target To, int Hands)
{
	ss::autoplay::TableAuto A;
	A.S = ss::style::Style::Maniac;
	A.M = M;
	A.Speed = Speed;
	A.To = To;
	A.Hands = Hands;
	A.Stops = 0;
	return S.SetAutoplay(0, A);
}

/** A deal's number from a Second Board's hand id ("<event>#<table>#<deal>"). */
int RitDealOf(const std::string& HandId)
{
	const size_t At = HandId.rfind('#');
	return At == std::string::npos ? -1 : std::atoi(HandId.c_str() + At + 1);
}

/**
 * The PIE's path on real play: a RunItTwice event with the plan (Maniac) in the seat, watched at 4x, the table drawn every
 * 0.1 s while it can read. (1) Every Second Board the table records is checked against the hand it was read from (the
 * finished hand still in CurHand that frame). (2) The same night (one seed, one event) skipped tick by tick: each deal the
 * player was dealt, as SkipStep hands it to HeroHandDone (Tournament::LastHeroHand), gives exactly the record the watched
 * table made of that deal, and no record the watched table didn't. Then the watched night skips to its result and the
 * Settlement's column prints each shown record's RAN word, equity and verdict as its hand had them.
 */
void RitSecondRecordNight()
{
	const auto T0 = std::chrono::steady_clock::now();
	RitLaptop P("rit-second-night");
	if (!RitSit(P))
	{
		std::printf("  rit: no RunItTwice event open at this hour; the night's Second Board checks are skipped\n");
		return;
	}
	RitExpect(RitPlan(P.S, ss::autoplay::Mode::Watch, 4, ss::autoplay::Target::Result, 0).empty(), "rit: the plan takes the seat at 4x");
	ss::ui::rlrit_detail::RitUi& U = ss::ui::rlrit_detail::RitOf(&P.RL);
	U.Seconds.clear();
	struct Seen
	{
		std::string HandId;
		RitTruth T;
		int Second = 0;
	};
	std::vector<Seen> Log;
	std::map<int, ss::ui::rlrit_detail::RitSecond> Watched;
	int Records = 0, Agree = 0, Landed = 0, EngineWon = 0, River = 0;
	std::string Last;
	for (int F = 0; F < 40000 && Records < 4 && P.S.T && !P.S.Finishing() && !P.S.HasResults; ++F)
	{
		P.Now = Step(P.S, P.Now, 0.1);
		// Drawn every 0.1 s whenever the table can read a Second Board (a finished hand with five cards up), as the laptop
		// draws it; the hands in between end before the river and are stepped without drawing, to keep the suite quick.
		if (!P.S.CurHand || !P.S.CurHand->bComplete || P.S.Board.size() != 5)
		{
			continue;
		}
		P.Frame();
		if (U.Seconds.empty() || U.Seconds.back().HandId == Last)
		{
			continue;
		}
		const ss::ui::rlrit_detail::RitSecond& B = U.Seconds.back();
		Last = B.HandId;
		const ss::Hand& H = *P.S.CurHand;
		const int Me = RitMe(H);
		if (Me < 0)
		{
			continue;
		}
		++Records;
		Watched[RitDealOf(B.HandId)] = B;
		const RitTruth T = RitTruthOf(H, Me);
		ss::site::show::SecondBoard Sb;
		ss::site::show::SecondRun(H, Me, B.HandId, Sb);
		bool Award = false;
		for (const ss::SeatVis& V : P.S.Seats)
		{
			Award = Award || (V.Present && V.Winner);
		}
		Landed += Award ? 1 : 0;
		EngineWon += T.Paid == 2 ? 1 : 0;
		River += B.Equity == 0.0 || B.Equity == 1.0 || B.Equity == 0.5 ? 1 : 0;
		const bool Ok = B.Hero == T.Hero && B.Villain == T.Villain && B.Ran == T.Ran && std::fabs(B.Equity - T.Equity) < 1e-12 && B.Verdict == Sb.Verdict && B.First == T.Paid;
		Agree += Ok ? 1 : 0;
		Log.push_back({B.HandId, T, Sb.Second});
		std::printf("  rit night %s: %s v %s, all-in equity %s, ran %s paid %s | record: equity %s, ran %s, verdict %d, award landed %s\n", B.HandId.c_str(),
			RitCards({T.Hero[0], T.Hero[1]}).c_str(), RitCards({T.Villain[0], T.Villain[1]}).c_str(), RitEquityText(T.Equity).c_str(), RitCards(T.Ran).c_str(), RitRanWord(T.Paid),
			B.Equity >= 0.0 ? RitEquityText(B.Equity).c_str() : "none", RitRanWord(B.First), B.Verdict, Award ? "yes" : "no");
	}
	std::printf("  rit night: %d records watched at 4x, %d agree with their hands; at the read the award had landed in %d; the engine paid the player in %d; "
		"%d records carried a 0, 50 or 100%% equity\n", Records, Agree, Landed, EngineWon, River);
	RitExpect(Records >= 3, "rit: the night at 4x records Second Boards (the plan goes all in)");
	RitExpect(Agree == Records, "rit: every Second Board the table records agrees with its hand (hands, board, RAN, all-in equity, verdict)");
	// The watched session's own log (HeroHandDone at each award, the watch path's call): once the last deal watched has been
	// awarded, the same records as the table's reads.
	const int Upto = P.S.HandsPlayed;
	const auto LogOf = [](const ss::Session& S, int Last) {
		std::map<int, ss::ui::rlrit_detail::RitSecond> Out;
		for (const ss::site::show::SecondRecord& R : S.SecondBoards)
		{
			if (RitDealOf(R.HandId) <= Last)
			{
				Out[RitDealOf(R.HandId)] = ss::ui::rlrit_detail::RitSecondOf(R);
			}
		}
		return Out;
	};
	const auto Agree2 = [](const std::map<int, ss::ui::rlrit_detail::RitSecond>& A, const std::map<int, ss::ui::rlrit_detail::RitSecond>& B) {
		bool Ok = A.size() == B.size();
		for (const auto& Rec : A)
		{
			const auto It = B.find(Rec.first);
			Ok = Ok && It != B.end() && RitSameRecord(Rec.second, It->second);
		}
		return Ok;
	};
	for (int F = 0; F < 200 && LogOf(P.S, Upto).size() < Watched.size() && P.S.HandsPlayed == Upto; ++F)
	{
		P.Now = Step(P.S, P.Now, 0.1);
	}
	const bool WatchLog = Agree2(LogOf(P.S, Upto), Watched);
	std::printf("  rit night, the watched session's log: %zu records to deal %d, %s the table's reads\n", LogOf(P.S, Upto).size(), Upto, WatchLog ? "the same as" : "NOT the same as");
	RitExpect(WatchLog, "rit: the watch path's HeroHandDone logs the table's Second Boards, record for record");
	// (2) The same night, skipped tick by tick to the last deal watched.
	RitLaptop K("rit-second-night");
	const bool Sat = RitSit(K) && K.S.T && P.S.T && K.S.T->Spec.Id == P.S.T->Spec.Id;
	RitExpect(Sat, "rit: the skipped night sits the same event");
	K.S.SprintBudgetMs = 0.0; // one tick an update: each deal's hand is read before the next replaces it
	// The first deal plays on the watch path; the skip then runs past the last deal watched (a skip that ends deals the next
	// hand inside the update that finished its last, so it ends one deal later and the loop reads every deal to Upto).
	RitExpect(Sat && RitPlan(K.S, ss::autoplay::Mode::Skip, 4, ss::autoplay::Target::Hands, Upto).empty(), "rit: the plan skips the same night");
	std::map<int, ss::ui::rlrit_detail::RitSecond> Skipped;
	int Key = 0;
	int Dealt = 0;
	for (int F = 0; Sat && F < 400000 && K.S.T && !K.S.Finishing() && K.S.HandsPlayed <= Upto; ++F)
	{
		K.Now += 1.0 / 30.0;
		K.S.Update(K.Now);
		const ss::Hand* Got = K.S.T ? K.S.T->LastHeroHand() : nullptr;
		if (!Got || !Got->bComplete || K.S.HandsPlayed == Key || K.S.HandsPlayed > Upto)
		{
			continue;
		}
		Key = K.S.HandsPlayed;
		++Dealt;
		ss::site::show::SecondRecord Rec;
		const std::string Id = K.S.T->Spec.Id + "#" + std::to_string(K.S.T->Hero().TableId) + "#" + std::to_string(Key);
		if (ss::site::show::SecondRecordOf(*Got, RitMe(*Got), Id, Rec))
		{
			Rec.EventId = K.S.T->Spec.Id;
			Skipped[Key] = ss::ui::rlrit_detail::RitSecondOf(Rec);
		}
	}
	const bool SkipHands = Agree2(Skipped, Watched);
	const bool SkipLog = Agree2(LogOf(K.S, Upto), Watched);
	std::printf("  rit night, skipped tick by tick: %d deals read to deal %d; the skip path's hands give %zu records, %s the watched table's %zu; the skipped "
		"session's log holds %zu, %s\n", Dealt, Upto, Skipped.size(), SkipHands ? "the same as" : "NOT the same as", Watched.size(), LogOf(K.S, Upto).size(),
		SkipLog ? "the same" : "NOT the same");
	RitExpect(Sat && !Watched.empty() && SkipHands, "rit: the skip path's hands give the watched table's Second Boards, record for record (hands, board, second run, RAN, equity, verdict)");
	RitExpect(Sat && SkipLog, "rit: the skip path's HeroHandDone logs the same Second Boards as the watched night");
	// Both nights to their results, as the PIE did (the watched one skipped from here): the two logs are the same, all the way.
	const std::string EventId = P.S.T ? P.S.T->Spec.Id : std::string();
	P.S.SkipAhead(0, ss::autoplay::Target::Result);
	K.S.SkipAhead(0, ss::autoplay::Target::Result);
	for (int F = 0; F < 200000 && (!P.S.HasResults || !K.S.HasResults); ++F)
	{
		P.Now = P.S.HasResults ? P.Now : Step(P.S, P.Now, 0.1);
		K.Now = K.S.HasResults ? K.Now : Step(K.S, K.Now, 0.1);
	}
	RitExpect(P.S.HasResults && K.S.HasResults, "rit: the skips run both nights to their results");
	bool Whole = P.S.SecondBoards.size() == K.S.SecondBoards.size() && !P.S.SecondBoards.empty();
	bool Clock = Whole;
	for (size_t I = 0; Whole && I < P.S.SecondBoards.size(); ++I)
	{
		Whole = RitSameRecord(ss::ui::rlrit_detail::RitSecondOf(P.S.SecondBoards[I]), ss::ui::rlrit_detail::RitSecondOf(K.S.SecondBoards[I]));
		Clock = Clock && P.S.SecondBoards[I].At == K.S.SecondBoards[I].At;
	}
	std::printf("  rit night to its result: the watched-then-skipped log %zu records (to deal %d), the skipped log %zu: %s%s\n", P.S.SecondBoards.size(),
		P.S.SecondBoards.empty() ? 0 : RitDealOf(P.S.SecondBoards.back().HandId), K.S.SecondBoards.size(), Whole ? "the same" : "NOT the same",
		Clock ? ", stamped at the same minutes" : ", their minutes differ");
	RitExpect(Whole, "rit: a night watched then skipped and the same night skipped log the same Second Boards");
	if (!P.S.HasResults || !K.S.HasResults)
	{
		return;
	}
	// Each Settlement's column prints the event's newest records (up to 3), RAN, equity and verdict as each record has them.
	const auto Settle = [&EventId](RitLaptop& L, const char* Name) {
		if (L.S.History.empty() || L.S.History.front().EventId != EventId)
		{
			ss::HistoryEntry H;
			H.Name = L.S.LastResults.EventName;
			H.EventId = EventId;
			L.S.History.insert(L.S.History.begin(), H);
		}
		L.S.CurrentScreen = ss::Screen::Results;
		const DrawList Dl = L.Settled();
		WriteList(Name, Dl);
		const std::vector<std::string> Col = RitColumn(Dl);
		int Want = 0;
		int Printed = 0;
		for (auto It = L.S.SecondBoards.rbegin(); It != L.S.SecondBoards.rend() && Want < 3; ++It)
		{
			if (It->EventId != EventId)
			{
				continue;
			}
			++Want;
			const ss::ui::rlrit_detail::RitSecond B = ss::ui::rlrit_detail::RitSecondOf(*It);
			Printed += RitColumnHas(Col, RitRanWord(B.First)) && RitColumnHas(Col, RitEquityText(B.Equity)) && RitColumnHas(Col, RitVerdictWord(It->Board.First, It->Board.Second)) ? 1 : 0;
		}
		const int Blocks = static_cast<int>(std::count(Col.begin(), Col.end(), std::string("you'd have")));
		std::printf("  rit night Settlement (%s): %d boards drawn for %d of the event's newest records, %d printed as recorded\n", Name, Blocks, Want, Printed);
		return Want > 0 && Blocks == Want && Printed == Want;
	};
	RitExpect(Settle(P, "rit_settlement_second_boards"), "rit: after the skip, the Settlement prints the night's Second Boards as recorded (RAN, equity, verdict)");
	RitExpect(Settle(K, "rit_settlement_second_boards_skipped"), "rit: a night skipped to its result prints its Second Boards too");
	std::printf("  rit night: the checks in %.1f s\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
}

/**
 * Up next registers from the Settlement of a table played to its end: the finished table goes and the new one opens alone
 * (one table, its buy-in paid once), as Back to the Board would have left it.
 */
void RitUpNext()
{
	RitLaptop P("rit-upnext");
	P.At(RitEvening);
	P.S.BankrollCents = 200000;
	P.Join(60000);
	ss::net::EventInstance E;
	if (!RitOpenEvent(P.S, E, 1100))
	{
		std::printf("  rit: no cheap RunItTwice event open; the Up next check is skipped\n");
		return;
	}
	const ss::net::Network& Net = ss::net::Site(SiteId::RunItTwice);
	if (!P.S.RegisterEvent(Net.Listing(E, nullptr, P.S.Unlocks())).empty() || !P.S.T)
	{
		return;
	}
	// Shove every decision: the table runs to its end quickly.
	for (int Frames = 0; P.S.CurrentScreen == ss::Screen::Table && Frames < 400000; ++Frames)
	{
		P.Now += 1.0 / 30.0;
		P.S.Update(P.Now);
		if (P.S.TableCount() > 0 && P.S.FocusedTable() >= 0)
		{
			const ss::TableGlance G = P.S.Glance(P.S.FocusedTable());
			if (G.YourTurn && P.Now - G.TurnOpenedAt > 0.3)
			{
				P.S.HeroAct(ss::PlayerAction::RaiseTo(1.0e12));
			}
		}
	}
	if (P.S.CurrentScreen != ss::Screen::Results)
	{
		std::printf("  rit: the shoving table didn't finish; the Up next check is skipped\n");
		return;
	}
	P.RL.OpenApp(App::RiverLine, P.Now);
	DrawList L = P.Settled();
	// The receipt's pool is the one this table played for (its tournament's), whatever the listing shows live.
	const ss::Chips Played = P.S.T ? P.S.T->PrizePoolCents : -1;
	RitExpect(Played > 0 && RitDrewSub(L, ss::Money(Played)), "rit: the Settlement's prize pool is its own table's");
	RitBox Go;
	for (const DrawCmd& Cmd : L.Cmds)
	{
		if (!Go.Found && Cmd.Type == DrawCmd::Kind::Text && Cmd.Text.X > 1086.0f && Cmd.Text.BaselineY > 380.0f && Cmd.Text.Text.rfind("REGISTER", 0) == 0)
		{
			Go = RitRun(L, Cmd.Text.Text, P.M, Cmd.Text.BaselineY - 0.5f, Cmd.Text.BaselineY + 0.5f);
		}
	}
	if (!Go.Found)
	{
		std::printf("  rit: nothing boarding on the Settlement; the Up next check is skipped\n");
		return;
	}
	const ss::Chips Wallet = ss::site::AccountOf(P.S.Life.Sites, SiteId::RunItTwice)->Cents;
	P.Click(Go.X + Go.W / 2.0f, Go.Base - 8.0f); // arms
	P.Settled();
	P.Click(1104.0f + 60.0f, Go.Base - 8.0f); // Confirm (the confirm step's left half)
	P.Settled();
	const ss::Chips Paid = Wallet - ss::site::AccountOf(P.S.Life.Sites, SiteId::RunItTwice)->Cents;
	std::printf("  rit: Up next from the Settlement: %d table(s), screen %d, paid %lld\n", P.S.TableCount(), static_cast<int>(P.S.CurrentScreen), static_cast<long long>(Paid));
	RitExpect(P.S.TableCount() == 1 && P.S.CurrentScreen == ss::Screen::Table && P.S.T && !P.S.T->bFinished && !P.S.HasResults && Paid > 0,
		"rit: Up next leaves the finished table behind and opens the new one alone");
}

// ------------------------------------------------------------------ Sunday Live's opening bell

void RitOpeningBell()
{
	// Sun Nov 8, the Sunday 100K at 19:00: RitBell once and SUNDAY SESSION OPEN across Sunday Live's hero band for 3 s.
	RitLaptop P("rit-bell");
	P.S.BankrollCents = 150000;
	P.At(34.0 * RitDay + 18.0 * 60.0 + 57.0);
	P.Join(21240);
	P.Open(Page::Live);
	DrawList L = P.Settled();
	WriteList("rit_live_sunday", L);
	const int Before = P.H.Count(ss::SoundId::RitBell);
	RitExpect(!DrewRun(L, "SUNDAY SESSION OPEN"), "rit: no opening bell before the major starts");
	P.At(34.0 * RitDay + 19.0 * 60.0 + 1.0);
	L = P.Frame();
	WriteList("rit_live_bell", L);
	RitExpect(P.H.Count(ss::SoundId::RitBell) == Before + 1 && DrewRun(L, "SUNDAY SESSION OPEN"), "rit: the opening bell rings once as a Sunday major starts, the band across the hero band");
	RitPageChecks(L, P.M, "Sunday Live at the bell");
	P.Now += 3.5;
	L = P.Frame();
	RitExpect(!DrewRun(L, "SUNDAY SESSION OPEN") && P.H.Count(ss::SoundId::RitBell) == Before + 1, "rit: the band goes after 3 s and the bell never rings twice");
}

// ------------------------------------------------------------------ the Kast preview: stills, no sound

void RitPreview()
{
	RitLaptop P("rit-preview");
	P.S.BankrollCents = 900000; // the rent paid on the way to the evening
	P.At(RitEvening);
	for (const char* Item : {ss::gear::FirstPcUpgrade().Id.c_str(), "webcam-1080", "mic-usb", "ring-light", "overlay-pack"})
	{
		P.S.Buy(Item);
	}
	P.S.Life.Energy = 90.0;
	const std::string Live = P.S.GoLive();
	if (!Live.empty())
	{
		std::printf("  rit: can't go live (%s); the preview check is skipped\n", Live.c_str());
		return;
	}
	P.Now = Step(P.S, P.Now, 2.0);
	P.Join(21240);
	P.RL.SiteShown = SiteId::RunItTwice;
	P.RL.OpenPage(Page::Lobby, P.Now);
	P.RL.OpenApp(App::Kast, P.Now);
	P.RL.ShowKastPage(ss::ui::RiverLine::KastPage::Studio);
	P.H.Sounds.clear();
	DrawList L;
	for (int K = 0; K < 45; ++K)
	{
		P.Now += 1.0 / 30.0;
		L = P.Frame();
	}
	WriteList("rit_kast_preview", L);
	RitExpect(DrewRun(L, "THE BOARD") && P.H.Cues() == 0, "rit: the Kast preview draws RunItTwice's Board as a still, with no cue");
	// Club Day under the preview (g1-audit L1): the slip printed and stamped as a still, no RitStamp, and the player's own
	// print still to come when they look at the Club themselves.
	ss::site::AddLine(P.S.Life.Sites, SiteId::RunItTwice, P.S.WorldMinutes() - 2.0 * RitDay, 4, 3186, "Second Run Club: Member, October");
	P.RL.OpenPage(Page::Promotions, P.Now);
	P.H.Sounds.clear();
	for (int K = 0; K < 20; ++K)
	{
		P.Now += 1.0 / 30.0;
		L = P.Frame();
	}
	const std::string Head = "RUNITTWICE \xC2\xB7 CLUB DAY";
	const RitBox Still = RitRun(L, Head, P.M);
	RitExpect(Still.Found && DrewRun(L, "PAID") && P.H.Cues() == 0, "rit: the Kast preview draws Club Day printed and stamped, with no cue");
	P.RL.OpenApp(App::RiverLine, P.Now);
	P.Frame();
	P.Now += 0.05;
	const RitBox Own = RitRun(P.Frame(), Head, P.M);
	std::printf("  rit: Club Day's head in the preview at y %.1f; the player's own print starts at y %.1f (printed: 538)\n", Still.Base, Own.Base);
	RitExpect(Own.Found && Own.Base < 538.0f - 50.0f, "rit: the preview leaves the player's own Club Day print to come");
}

// ------------------------------------------------------------------ the art, the copy, the bench

void RitArtChecks()
{
	// The Watchlist's star is never red (§13.5): Chrome outline, Hot White when on.
	for (const bool On : {false, true})
	{
		TableMeasurer M;
		DrawList L;
		ss::ui::Canvas C(L, M, 100.0f, 100.0f, 1.0f);
		ss::ui::rlrit_detail::RitStar(C, 50.0f, 50.0f, 10.0f, On);
		bool Red = false;
		for (const ss::ui::Vertex& V : L.Vertices)
		{
			Red = Red || (V.Col.A > 0.05f && RitReddish(V.Col));
		}
		RitExpect(!L.Vertices.empty() && !Red, On ? "rit: the starred Watchlist star is never red" : "rit: the Watchlist star is never red");
	}
	// The palette (§3.3, §13.3): Ink on Panel 15.30, Muted on Bg 7.54; none of RunItTwice's constants a RiverLine hex.
	namespace rd = ss::ui::rlrit_detail;
	RitExpect(RitContrast(rd::RitC(rd::RitInk), rd::RitC(rd::RitCarbon)) >= 15.2 && RitContrast(rd::RitC(rd::RitMuted), rd::RitC(rd::RitOnyx)) >= 7.5,
		"rit: Ink on Carbon and Muted on Onyx read (15.30 and 7.54)");
	RitExpect(RitContrast(rd::RitC(rd::RitWhite), rd::RitC(rd::RitRed)) >= 4.5 && RitContrast(rd::RitC(rd::RitRed), rd::RitC(rd::RitPaper)) >= 4.5,
		"rit: Hot White on Signal Red and the stamp on paper read at 4.5:1");
	std::set<uint32_t> Home;
	for (const uint32_t H : ss::net::Def(SiteId::RiverLine).Palette)
	{
		Home.insert(H);
	}
	bool Apart = true;
	for (const uint32_t H : {rd::RitOnyx, rd::RitBg2, rd::RitCarbon, rd::RitPanel2, rd::RitHair, rd::RitInk, rd::RitMuted, rd::RitDim, rd::RitRed, rd::RitChrome, rd::RitWhite,
			 rd::RitDown, rd::RitAmber, rd::RitUp, rd::RitCloth, rd::RitClothDark, rd::RitTape, rd::RitGloss, rd::RitTwillA, rd::RitTwillB, rd::RitDeep, rd::RitLed,
			 rd::RitChromeDark, rd::RitChromeLight, rd::RitChromeMid, rd::RitGoldDark, rd::RitGoldLight, rd::RitGoldMid, rd::RitPool, rd::RitSelRow, rd::RitFinalDim,
			 rd::RitNightRow, rd::RitPaper, rd::RitPaperInk, rd::RitPaperMuted})
	{
		Apart = Apart && Home.count(H) == 0;
	}
	RitExpect(Apart, "rit: none of RunItTwice's colours is a RiverLine hex");
	// The twice print's offset: round(Size / 16), 2-6.
	RitExpect(rd::RitTwiceOffset(28.0f) == 2.0f && rd::RitTwiceOffset(40.0f) == 3.0f && rd::RitTwiceOffset(56.0f) == 4.0f && rd::RitTwiceOffset(96.0f) == 6.0f,
		"rit: the twice print's offset");
}

// ------------------------------------------------------------------ G1's fix pass (fix-rit: G1_AUDIT L1, L2, L3, L5, I4)

/** An event's winner as the Board's hover names them: the world's result's first place. */
std::string RitWinnerName(const ss::Session& S, const ss::net::EventInstance& E)
{
	const ss::net::Network& Net = ss::net::Site(SiteId::RunItTwice);
	for (const ss::net::Placing& Pl : Net.Result(E).FinalTable)
	{
		if (Pl.Place == 1)
		{
			return Pl.Player >= 0 && Pl.Player < static_cast<int>(Net.Players().size()) ? Net.Players()[static_cast<size_t>(Pl.Player)].Name
				: Pl.Player == -1                                                        ? S.HeroName
																						 : Pl.Name;
		}
	}
	return std::string();
}

void RitArrivedHover()
{
	// L2 (§3.4 A): with RUNNING on, tonight's arrivals sit under the departures as ARRIVED, the latest first; they're not the
	// schedule's (never in Listed, never selected), and the pointer on one names its winner.
	RitLaptop P("rit-arrived");
	P.At(RitEvening + 4.0 * 60.0); // 11:30 PM: the afternoon's and the early evening's events have landed
	P.S.BankrollCents = 150000;
	P.Join(21240);
	P.Open(Page::Lobby);
	P.Settled();
	P.RL.SetFilter(ss::ui::RiverLine::Filter::Running, P.Now);
	const DrawList L = P.Settled();
	const ss::net::Network& Net = ss::net::Site(SiteId::RunItTwice);
	const double World = P.S.WorldMinutes();
	const double Night = std::floor((World - 6.0 * 60.0) / RitDay) * RitDay + 6.0 * 60.0; // the last 6 AM
	std::vector<ss::net::EventInstance> Landed;
	for (const ss::net::EventInstance& E : Net.Window(Night - 16.0 * 60.0, World))
	{
		if (Net.Live(E, World).St == ss::net::Status::Finished && E.Start + E.Duration >= Night)
		{
			Landed.push_back(E);
		}
	}
	std::stable_sort(Landed.begin(), Landed.end(), [](const ss::net::EventInstance& A, const ss::net::EventInstance& B) { return A.Start + A.Duration > B.Start + B.Duration; });
	bool Clean = true;
	for (const std::string& Id : P.RL.ListedEvents())
	{
		ss::net::EventInstance E;
		Clean = Clean && Net.FindInstance(Id, E) && Net.Live(E, World).St != ss::net::Status::Finished;
	}
	float Top = 1.0e9f;
	for (const DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == DrawCmd::Kind::Text && Cmd.Text.Text == "ARRIVED" && Cmd.Text.BaselineY >= 252.0f && Cmd.Text.BaselineY <= 932.0f)
		{
			Top = std::min(Top, Cmd.Text.BaselineY);
		}
	}
	std::printf("  rit arrivals: %zu landed tonight, %zu departures listed, the first ARRIVED at y %.1f\n", Landed.size(), P.RL.ListedEvents().size(), Top);
	RitExpect(!Landed.empty() && Top < 932.0f && Clean, "rit: with RUNNING on, tonight's arrivals show ARRIVED under the departures, none of them in Listed");
	RitExpect(!DrewRun(L, "WINNER"), "rit: no winner plate without the pointer on an arrival");
	if (Landed.empty() || Top >= 932.0f)
	{
		return;
	}
	const std::string Winner = RitWinnerName(P.S, Landed.front());
	const std::string Selected = P.RL.SelectedEvent();
	const float Hy = Top - 6.0f; // inside the latest arrival's row
	const DrawList Lh = P.Settled(600.0f, Hy);
	std::printf("  rit: the latest arrival %s, won by \"%s\"\n", Landed.front().Id.c_str(), Winner.c_str());
	RitExpect(DrewRun(Lh, "WINNER") && !Winner.empty() && DrewRun(Lh, Winner), "rit: the pointer on an ARRIVED row names its winner (the world's result)");
	WriteList("rit_board_arrived", Lh);
	RitPageChecks(Lh, P.M, "the Board, an arrival's winner");
	// Budgets (§10, review-fix-rit): the Board's at-rest caps hold with tonight's arrivals under the departures and the plate up.
	std::printf("  rit board with arrivals and the plate: %zu vertices, %d text runs\n", Lh.Vertices.size(), RitRuns(Lh));
	RitExpect(Lh.Vertices.size() <= 22000 && RitRuns(Lh) <= 345, "rit: the Board with its arrivals and the winner plate within its budget (22k vertices, 345 runs)");
	P.Click(600.0f, Hy);
	P.Settled();
	RitExpect(P.RL.SelectedEvent() == Selected, "rit: a click on an arrival selects nothing (Listed stays the truth)");
}

void RitRankingsEvents()
{
	// L3 (§3.4 D): the EVENTS column, this season's events on RunItTwice: the world's site record for each player, the
	// account's season for the player (in the YOU bar too).
	RitLaptop P("rit-rankings-events");
	P.At(RitEvening);
	if (ss::site::Account* A = P.Join(0))
	{
		A->Hero.Year = ss::world::YearOf(ss::net::DayOf(P.S.WorldMinutes()));
		A->Hero.Points = 777.0;
		A->Hero.Events = 7;
	}
	P.Open(Page::Leaderboards);
	P.Settled(); // the first frame picks the board (its rows ease in from there)
	const DrawList L = P.Settled();
	const RitBox Mine = RitRun(L, "7", P.M, 905.0f, 940.0f);
	RitExpect(DrewRun(L, "EVENTS") && Mine.Found && std::fabs(Mine.X + Mine.W - 1032.0f) < 0.6f, "rit: RANKINGS has an EVENTS column, and the YOU bar carries the player's 7");
	const ss::net::Network& Net = ss::net::Site(SiteId::RunItTwice);
	const ss::net::HeroStats Hero = ss::ui::rlsites_detail::SitesHero(P.S, SiteId::RunItTwice, Net, P.S.WorldMinutes(), ss::net::Board::Season);
	ss::net::BoardRow Me;
	const std::vector<ss::net::BoardRow> Rows = Net.Leaderboard(ss::net::Board::Season, P.S.WorldMinutes(), Hero, 22, &Me);
	const bool WorldRows = &Net.Players() == &P.S.Living().View();
	int Checked = 0;
	int Right = 0;
	int Counted = 0;
	for (size_t I = 0; I < Rows.size() && I < 20; ++I)
	{
		if (Rows[I].Player < 0)
		{
			continue;
		}
		const ss::world::SiteRecord* R = WorldRows ? P.S.Living().SiteRecordOf(SiteId::RunItTwice, Rows[I].Player) : nullptr;
		const std::string Want = R && R->Events > 0 ? ss::Grouped(R->Events) : std::string("\xE2\x80\x94");
		const float Base = 168.0f + 32.0f + static_cast<float>(I) * 34.0f + 17.0f + 15.0f * 0.36f;
		const RitBox Got = RitRun(L, Want, P.M, Base - 0.5f, Base + 0.5f);
		++Checked;
		Counted += R && R->Events > 0 ? 1 : 0;
		Right += Got.Found && std::fabs(Got.X + Got.W - 1032.0f) < 0.6f ? 1 : 0;
	}
	std::printf("  rit rankings: %d rows checked, %d with a season record here, %d drawn right\n", Checked, Counted, Right);
	RitExpect(Checked > 0 && Counted > 0 && Right == Checked, "rit: each row's EVENTS is that player's season record on RunItTwice");
	WriteList("rit_rankings_events", L);
	RitContent = {{340.0f, 168.0f, 40.0f, 760.0f}}; // the rows' square avatars (24 at x 348, the circle under its clip)
	RitPageChecks(L, P.M, "Rankings with the EVENTS column");
	RitContent.clear();
}

void RitDeskOneRake()
{
	// L5: the Desk's RAKE PAID tile and the Ledger's RAKE PAID are one figure, the rake actually paid (the account's: a
	// ticket's face value is never rake), this month's under the tile; the Ledger's FINAL TABLES counts an 8-max's last 8.
	RitLaptop P("rit-desk-rake");
	P.At(RitEvening);
	ss::site::Account* A = P.Join(21240);
	RitExpect(A != nullptr, "rit: the Desk's account opens");
	if (!A)
	{
		return;
	}
	A->RakeLife = 1234;
	A->MonthKey = ss::site::MonthKeyAt(P.S.WorldMinutes());
	A->RakeMonth = 456;
	const double World = P.S.WorldMinutes();
	const auto Row = [World](const char* Name, int Place, ss::Chips BuyIn, const char* Template, double DaysAgo) {
		ss::HistoryEntry H;
		H.Name = Name;
		H.Place = Place;
		H.Entrants = 90;
		H.Prize = Place <= 8 ? 2500 : 0;
		H.BuyInCents = BuyIn;
		H.EventId = std::string(Template) + "@" + std::to_string(static_cast<long long>(std::llround(World - DaysAgo * RitDay)));
		return H;
	};
	// The Nightcap on a ticket (a $109 face value; it paid no rake), 9th of an 8-max; the Sweat 8th.
	P.S.History.insert(P.S.History.begin(), Row("The Nightcap", 9, 10900, "runittwice.the-nightcap", 2.0));
	P.S.History.insert(P.S.History.begin(), Row("The Sweat", 8, 1100, "runittwice.the-sweat", 1.0));
	P.Open(Page::Desk);
	const DrawList L = P.Settled();
	const RitBox Tile = RitRun(L, "$12.34", P.M, 130.0f, 206.0f);
	const RitBox Ledger = RitRun(L, "$12.34", P.M, 538.0f, 950.0f);
	RitExpect(DrewRun(L, "RAKE PAID") && Tile.Found && Ledger.Found && Ledger.X > 912.0f && RitRun(L, "$4.56 this month", P.M, 130.0f, 206.0f).Found,
		"rit: the Desk's RAKE PAID and the Ledger's are one figure, the rake actually paid, with this month's under the tile");
	const RitBox Fts = RitRun(L, "1", P.M, 690.0f, 694.0f);
	RitExpect(Fts.Found && Fts.X > 912.0f, "rit: the Ledger's FINAL TABLES counts an 8-max's last 8 (the 8th, not the 9th)");
	WriteList("rit_desk_rake", L);
	RitPageChecks(L, P.M, "the Desk, one rake");
}

void RitClubDayPrints()
{
	// L1 (§3.4 G): Club Day's slip feeds out of its slot over 0.6 s, then PAID thunks once (RitStamp); a slip prints once.
	RitLaptop P("rit-club-day");
	P.At(RitEvening);
	P.Join(21240);
	ss::site::AddLine(P.S.Life.Sites, SiteId::RunItTwice, P.S.WorldMinutes() - 2.0 * RitDay, 4, 3186, "Second Run Club: Member, October");
	P.Open(Page::Promotions);
	const std::string Head = "RUNITTWICE \xC2\xB7 CLUB DAY";
	P.Frame(); // the page's first frame (drawn at alpha 0 as it eases in): the slip starts to feed
	P.Now += 0.05;
	const DrawList L0 = P.Frame();
	P.Now += 0.25;
	const DrawList L1 = P.Frame();
	const int Before = P.H.Count(ss::SoundId::RitStamp);
	P.Now += 0.4;
	P.Frame();
	P.Now += 0.3;
	const DrawList L3 = P.Frame();
	const RitBox B0 = RitRun(L0, Head, P.M);
	const RitBox B1 = RitRun(L1, Head, P.M);
	const RitBox B3 = RitRun(L3, Head, P.M);
	std::printf("  rit club day: the slip's head at y %.1f, %.1f, %.1f\n", B0.Base, B1.Base, B3.Base);
	RitExpect(B0.Found && B1.Found && B3.Found && B0.Base < B1.Base && B1.Base < B3.Base && std::fabs(B3.Base - 538.0f) < 0.6f && !DrewRun(L1, "PAID") && DrewRun(L3, "PAID"),
		"rit: Club Day's slip feeds out of its slot, then PAID stamps");
	RitExpect(Before == 0 && P.H.Count(ss::SoundId::RitStamp) == 1, "rit: PAID thunks once (RitStamp) when the slip is out");
	WriteList("rit_club_day", L3);
	RitPageChecks(L3, P.M, "the Club with its Club Day");
	P.Open(Page::Lobby);
	P.Settled();
	P.Open(Page::Promotions);
	P.Frame();
	P.Now += 0.05;
	const DrawList Again = P.Frame();
	const RitBox Ba = RitRun(Again, Head, P.M);
	RitExpect(Ba.Found && std::fabs(Ba.Base - B3.Base) < 0.01f && P.H.Count(ss::SoundId::RitStamp) == 1, "rit: a slip prints once: back on the Club it lies printed, no second thunk");
}

/** The vertices a frame draws that a twin frame without them doesn't (one inserted run of commands): their centroid. */
bool RitExtraCentroid(const DrawList& Without, const DrawList& With, ss::ui::Vec2& Out)
{
	if (With.Cmds.size() <= Without.Cmds.size())
	{
		return false;
	}
	size_t First = 0;
	while (First < Without.Cmds.size() && With.Cmds[First].Type == Without.Cmds[First].Type && With.Cmds[First].VertexCount == Without.Cmds[First].VertexCount)
	{
		++First;
	}
	const size_t Extra = With.Cmds.size() - Without.Cmds.size();
	double Sx = 0.0;
	double Sy = 0.0;
	int N = 0;
	for (size_t C = First; C < First + Extra && C < With.Cmds.size(); ++C)
	{
		const DrawCmd& Cmd = With.Cmds[C];
		if (Cmd.Type != DrawCmd::Kind::Triangles)
		{
			continue;
		}
		for (uint32_t K = 0; K < Cmd.VertexCount; ++K)
		{
			const ss::ui::Vertex& V = With.Vertices[Cmd.VertexStart + K];
			Sx += V.X;
			Sy += V.Y;
			++N;
		}
	}
	if (N == 0)
	{
		return false;
	}
	Out = {static_cast<float>(Sx / N), static_cast<float>(Sy / N)};
	return true;
}

void RitPuckAndSlide()
{
	// L4 (g1-audit; fix-rit_to_fix-tablestyle_1): the chrome puck's word sits inside its ring, measured in its own face: the
	// table's puck (R 15) and a tile's (R 9, "D").
	for (const float R : {15.0f, 9.0f})
	{
		TableMeasurer M;
		DrawList L;
		ss::ui::Canvas C(L, M, 100.0f, 100.0f, 1.0f);
		ss::ui::rlsites_detail::SitesDealerButton(C, 50.0f, 50.0f, R, 2);
		const DrawCmd* Word = nullptr;
		for (const DrawCmd& Cmd : L.Cmds)
		{
			Word = Cmd.Type == DrawCmd::Kind::Text ? &Cmd : Word;
		}
		const float W = Word ? M.Width(Word->Text.Text, Word->Text.Face, Word->Text.SizePx) : 1.0e9f;
		const float Rise = Word ? Word->Text.SizePx * 0.36f : 0.0f;
		const float Room = 2.0f * std::sqrt(std::max(0.0f, (R - 2.0f) * (R - 2.0f) - Rise * Rise));
		std::printf("  rit puck R %.0f: \"%s\" at %.2f, %.1f wide in %.1f\n", R, Word ? Word->Text.Text.c_str() : "", Word ? Word->Text.SizePx : 0.0f, W, Room);
		RitExpect(Word && W <= Room && (R < 12.0f || Word->Text.Text == "DEALER"), R > 12.0f ? "rit: the table's puck reads DEALER inside its ring" : "rit: a tile's puck reads inside its ring");
	}
	// L1 (§3.5 Bets; the same patch): a bet's chips slide straight to the bet in 160 ms, no hop: drawn at 0.12 s, gone at
	// 0.2 s (the session gives the flight 0.3), and three samples on one line.
	RitLaptop P("rit-slide");
	P.At(RitEvening);
	P.S.BankrollCents = 200000;
	P.Join(50000);
	ss::net::EventInstance E;
	if (!RitOpenEvent(P.S, E, 3300))
	{
		std::printf("  rit: no RunItTwice event open at this hour; the slide check is skipped\n");
		return;
	}
	P.S.RegisterEvent(ss::net::Site(SiteId::RunItTwice).Listing(E, nullptr, P.S.Unlocks()));
	if (!P.S.T)
	{
		RitExpect(false, "rit: a RunItTwice table opens for the slide");
		return;
	}
	P.S.CurrentPace = ss::Pace::Full;
	P.Now = Step(P.S, P.Now, 3.0);
	P.Settled();
	int Seat = -1;
	for (size_t I = 0; I < P.S.Seats.size() && Seat < 0; ++I)
	{
		Seat = P.S.Seats[I].Present && !P.S.Seats[I].IsHero ? static_cast<int>(I) : -1;
	}
	if (Seat < 0)
	{
		RitExpect(false, "rit: the octagon has a seat to bet from");
		return;
	}
	const double Start = P.Now + 1.0;
	const auto Sample = [&](double At, ss::ui::Vec2& Where) {
		P.Now = Start + At;
		P.S.Flights.clear();
		const DrawList Without = P.Frame();
		ss::Flight F;
		F.IsChips = true;
		F.From = ss::FlightEnd::AtSeat(Seat);
		F.To = ss::FlightEnd::AtBet(Seat);
		F.Amount = 2000;
		F.Start = Start;
		F.Dur = 0.3;
		P.S.Flights = {F};
		const DrawList With = P.Frame();
		P.S.Flights.clear();
		return RitExtraCentroid(Without, With, Where);
	};
	ss::ui::Vec2 A{}, B{}, C{}, Late{};
	const bool Seen = Sample(0.02, A) && Sample(0.06, B) && Sample(0.12, C);
	const bool Gone = !Sample(0.2, Late);
	const float Cross = (B.X - A.X) * (C.Y - A.Y) - (B.Y - A.Y) * (C.X - A.X);
	const float Span = std::hypot(C.X - A.X, C.Y - A.Y);
	std::printf("  rit slide: (%.1f,%.1f) (%.1f,%.1f) (%.1f,%.1f), off the line by %.2f, gone at 0.2 s: %s\n", A.X, A.Y, B.X, B.Y, C.X, C.Y, Span > 0.0f ? std::fabs(Cross) / Span : -1.0f,
		Gone ? "yes" : "no");
	RitExpect(Seen && Gone && Span > 10.0f && std::fabs(Cross) / Span < 1.0f, "rit: a bet's chips slide straight to the bet in 160 ms (drawn at 0.12 s, gone at 0.2 s)");
}

void RitCopy()
{
	std::vector<std::pair<std::string, std::string>> Lines;
	for (const std::string& T : ss::ui::rlrit_detail::RitInventory())
	{
		Lines.push_back({"rit", T});
	}
	int Problems = copylint::Run("rit-ui", Lines, copylint::MailSubject, {});
	std::vector<std::string> Words = ss::ui::rlrit_detail::RitInventory();
	for (const std::string& T : ss::ui::rlrit_detail::RitLabels())
	{
		Words.push_back(T);
	}
	for (const std::string& T : Words)
	{
		const char* A = trademarks::Find(T, trademarks::kSourceScan);
		const char* B = trademarks::Find(T, trademarks::kCopyScan);
		if (A || B)
		{
			std::printf("COPY (rit) \"%s\" names \"%s\"\n", T.c_str(), A ? A : B);
			++Problems;
		}
		const std::string Low = " " + copylint::Lower(T) + " ";
		for (const char* Banned : {" ai ", " ai.", " bot ", " bots ", "autoplay", "autopilot", "coming soon"})
		{
			if (Low.find(Banned) != std::string::npos)
			{
				std::printf("COPY (rit) \"%s\" says \"%s\"\n", T.c_str(), Banned);
				++Problems;
			}
		}
	}
	RitExpect(Problems == 0, "rit: RunItTwice's copy passes the lint and the trademark screen");
	// I4 (g1-audit): the feed's and the Newsdesk's empty lines say it plainly, without the newsroom's "wire".
	bool Wire = false;
	for (const std::string& T : ss::ui::rlrit_detail::RitInventory())
	{
		Wire = Wire || copylint::Lower(T).find("wire") != std::string::npos;
	}
	RitExpect(!Wire && std::string(ss::ui::rlrit_detail::RitNoFeed).find("feed") != std::string::npos, "rit: no \"wire\" anywhere in RunItTwice's copy");
}

void RitBench()
{
	if (!std::getenv("SS_UI_BENCH"))
	{
		return;
	}
	RitLaptop P("rit-bench");
	P.At(RitEvening);
	P.S.BankrollCents = 150000;
	P.Join(21240);
	for (const Page Pg : {Page::Lobby, Page::Live, Page::Desk, Page::Promotions})
	{
		P.Open(Pg);
		const DrawList L = P.Settled();
		double Best = 1.0e9;
		for (int Round = 0; Round < 5; ++Round)
		{
			const auto T0 = std::chrono::steady_clock::now();
			for (int K = 0; K < 40; ++K)
			{
				P.Frame();
			}
			Best = std::min(Best, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count() / 40.0);
		}
		std::printf("UI BENCH runittwice page %d %6.3f ms/draw, %zu vertices, %d runs\n", static_cast<int>(Pg), Best, L.Vertices.size(), RitRuns(L));
	}
	// The Board under RUNNING at 11:30 PM (review-fix-rit): the departures and tonight's arrivals (RitArrivals' window, every
	// frame), the state G1's fix pass added.
	P.At(RitEvening + 4.0 * 60.0);
	P.Open(Page::Lobby);
	P.Settled();
	P.RL.SetFilter(ss::ui::RiverLine::Filter::Running, P.Now);
	const DrawList La = P.Settled();
	double BestA = 1.0e9;
	for (int Round = 0; Round < 5; ++Round)
	{
		const auto T0 = std::chrono::steady_clock::now();
		for (int K = 0; K < 40; ++K)
		{
			P.Frame();
		}
		BestA = std::min(BestA, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count() / 40.0);
	}
	std::printf("UI BENCH runittwice board running+arrivals %6.3f ms/draw, %zu vertices, %d runs\n", BestA, La.Vertices.size(), RitRuns(La));
}
} // namespace rit_seg

void SegRit()
{
	using namespace rit_seg;
	RitArtChecks();
	RitRailAndPages();
	RitBoard();
	RitFinalCallAndNight();
	RitPins();
	RitTable();
	RitSettlementChecks();
	RitSecondRecordPairs();
	RitSecondRecordNight();
	RitUpNext();
	RitOpeningBell();
	RitPreview();
	RitArrivedHover();
	RitRankingsEvents();
	RitDeskOneRake();
	RitClubDayPrints();
	RitPuckAndSlide();
	RitCopy();
	RitBench();
	std::printf("  ui-rit: %d checks\n", RitChecks);
}
} // namespace ui_test
