// riverline_ui: ui-laptop's sites additions (update 2 SPEC §6.7, §5.11, §1.8): a rival's lobby (site_lobby_ignite,
// site_lobby_runittwice), the Cashier and money in transit (site_cashier, site_cashier_transit), Promotions
// (site_promotions), the poker button's menu (poker_button_menu), the Bank's pots (bank_pots), Career's Rewards card
// (career_rewards), the rent card's line (rent_card_sites), a site's invitation in Mail (mail_site_invite); for every skin
// the contrast check (Ink on Panel, Muted on Bg at 4.5:1 or more) and the palette-leak check (none of RiverLine's 16 hexes
// drawn inside a rival's SkinScope, on every page a rival shows, its table and its tile); the taskbar with nine apps and the
// caret leaves the news wire's lane 120 wide. Compiled into ui_test and called from main() in ui_main.cpp (the kit is
// UiSeg.h); the helpers are in ui_test::sites_seg.
#include "UiSeg.h"

#include "CopyLint.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Structure.h"
#include "ShortStack/UI/EventArt.h"
#include "ShortStack/UI/RiverLine.h"
#include "ShortStack/UI/SecondScreen.h"
#include "ShortStack/UI/SiteArt.h"
#include "ShortStack/UI/Ui.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/SitesDetail.h"
#include "../../Source/ShortStackCore/Private/ShortStack/UI/RiverLineShared.h"
#include "../../Source/ShortStackCore/Private/ShortStack/UI/IgniteArt.h"
#include "../../Source/ShortStackCore/Private/ShortStack/UI/RitArt.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace ui_test
{
namespace sites_seg
{
using App = ss::ui::RiverLine::App;
using Page = ss::ui::RiverLine::Page;
using ss::net::SiteId;
using ss::ui::DrawList;
constexpr double SitesDay = 1440.0;
/** Wed Nov 4, 2026, 7:30 PM: Ignite's lobby has been open two days (it launched Mon Nov 2), RunItTwice's since Night One. */
constexpr double SitesEvening = 30.0 * SitesDay + 19.5 * 60.0;

/** The checks this file makes (printed at its end), each through the suite's Expect. */
int SitesChecks = 0;
void SitesExpect(bool Condition, const char* What)
{
	++SitesChecks;
	Expect(Condition, What);
}

struct SitesHooks : QuietHooks
{
	bool GoOut(const std::string&, ss::Chips) override { return true; }
};

/** A session and its laptop, the pointer driven by hand (laptop_seg's kit, this file's own copy). */
struct SitesLaptop
{
	SitesHooks H;
	ss::Session S;
	ss::ui::RiverLine RL;
	TableMeasurer M;
	double Now = 1.0;
	explicit SitesLaptop(const std::string& Seed) : S(H, Seed), RL(S)
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
	/** The clock to World (forward only), the player fed and rested so nothing else gets in the way. */
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
	/** An account on a rival with Cents in its wallet (core-sites' Join, as Accept makes it). */
	ss::site::Account* Join(SiteId Site, ss::Chips Cents)
	{
		ss::site::Join(S.Life.Sites, Site, S.WorldMinutes());
		ss::site::Account* A = ss::site::AccountOf(S.Life.Sites, Site);
		if (A)
		{
			A->Cents = Cents;
		}
		S.Life.Update2LinesRead |= ss::life::U2Sites;
		return A;
	}
};

/** A text run that is exactly Text with its baseline in [MinY, MaxY]. */
struct SitesBox
{
	bool Found = false;
	float X = 0.0f;
	float Base = 0.0f;
	float W = 0.0f;
};
SitesBox SitesRun(const DrawList& L, const std::string& Text, const TableMeasurer& M, float MinY = -1.0e9f, float MaxY = 1.0e9f)
{
	SitesBox Out;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text && Cmd.Text.BaselineY >= MinY && Cmd.Text.BaselineY <= MaxY)
		{
			Out.Found = true;
			Out.X = Cmd.Text.X;
			Out.Base = Cmd.Text.BaselineY;
			Out.W = M.Width(Text, Cmd.Text.Face, Cmd.Text.SizePx);
			return Out;
		}
	}
	return Out;
}

/** Tracked capitals (NetSpaced draws them a glyph at a time) joined back up, one string per baseline. */
bool SitesSpelled(const DrawList& L, const std::string& Word)
{
	std::string Cur;
	float Base = -1.0e9f;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text)
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

uint32_t SitesRgb(const ss::ui::Color& C)
{
	const auto Byte = [](float V) { return static_cast<uint32_t>(std::min(255.0f, std::max(0.0f, V * 255.0f + 0.5f))); };
	return Byte(C.R) << 16 | Byte(C.G) << 8 | Byte(C.B);
}

/** The 16 tokens of a skin as 0xRRGGBB. */
std::set<uint32_t> SitesHexes(const ss::ui::Skin& K)
{
	const ss::ui::Color* T[16] = {&K.Bg, &K.Bg2, &K.Panel, &K.Panel2, &K.Line, &K.Ink, &K.Muted, &K.Dim, &K.Accent, &K.Accent2, &K.Gold, &K.Red, &K.Orange, &K.Green, &K.Felt, &K.FeltDark};
	std::set<uint32_t> Out;
	for (const ss::ui::Color* C : T)
	{
		Out.insert(SitesRgb(*C));
	}
	return Out;
}

/**
 * The palette-leak check (§1.8, §6.7): colours drawn in Area (vertices that show, text runs) that are one of RiverLine's 16
 * tokens and not one of the skin's own. Returns how many, and fills Seen with the leaked hexes.
 */
int SitesLeaks(const DrawList& L, const ss::ui::Skin& Skin, const ss::ui::Rect& Area, std::set<uint32_t>& Seen)
{
	const std::set<uint32_t> Home = SitesHexes(ss::ui::Skin::FromHex(ss::net::Def(SiteId::RiverLine).Palette));
	const std::set<uint32_t> Own = SitesHexes(Skin);
	const auto Inside = [&Area](float X, float Y) { return X >= Area.X && X <= Area.X + Area.W && Y >= Area.Y && Y <= Area.Y + Area.H; };
	int Count = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
		{
			const uint32_t Rgb = SitesRgb(Cmd.Text.Col);
			if (Cmd.Text.Col.A > 0.02f && Inside(Cmd.Text.X, Cmd.Text.BaselineY) && Home.count(Rgb) && !Own.count(Rgb))
			{
				++Count;
				Seen.insert(Rgb);
			}
			continue;
		}
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Triangles)
		{
			continue;
		}
		// A run's leaked vertices, and how far they spread: a speck under 12 by 12 is the shared art's own ink (a badge's
		// mark, a crown avatar's jewel: the same on every site, like the logos), never a ground, a line or a word.
		int Here = 0;
		float X0 = 1.0e9f, Y0 = 1.0e9f, X1 = -1.0e9f, Y1 = -1.0e9f;
		std::set<uint32_t> Mine;
		for (uint32_t K = 0; K < Cmd.VertexCount; ++K)
		{
			const ss::ui::Vertex& V = L.Vertices[Cmd.VertexStart + K];
			const uint32_t Rgb = SitesRgb(V.Col);
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
		if (Here > 0 && (X1 - X0 > 12.0f || Y1 - Y0 > 12.0f))
		{
			Count += Here;
			Seen.insert(Mine.begin(), Mine.end());
		}
	}
	return Count;
}

/** WCAG contrast between two colours (1: the same, 21: black on white). */
double SitesContrast(const ss::ui::Color& A, const ss::ui::Color& B)
{
	const auto Lin = [](float V) { return V <= 0.04045f ? V / 12.92 : std::pow((V + 0.055) / 1.055, 2.4); };
	const auto Lum = [&Lin](const ss::ui::Color& C) { return 0.2126 * Lin(C.R) + 0.7152 * Lin(C.G) + 0.0722 * Lin(C.B); };
	const double La = Lum(A);
	const double Lb = Lum(B);
	return (std::max(La, Lb) + 0.05) / (std::min(La, Lb) + 0.05);
}

/** The client's area: everything above the taskbar (the taskbar, the toasts and other apps stay RiverLine's). */
const ss::ui::Rect SitesClientArea{0.0f, 0.0f, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height - 38.0f};

/** Checks one page drawn in a rival's skin for leaks, printing what leaked. */
void SitesNoLeak(const DrawList& L, SiteId Site, const char* What, const ss::ui::Rect& Area = SitesClientArea)
{
	std::set<uint32_t> Seen;
	const int Count = SitesLeaks(L, ss::ui::Skin::FromHex(ss::net::Def(Site).Palette), Area, Seen);
	if (Count > 0)
	{
		std::printf("  leak (%s):", What);
		for (const uint32_t H : Seen)
		{
			std::printf(" %06x", H);
		}
		std::printf(" in %d places\n", Count);
	}
	SitesExpect(Count == 0, (std::string("sites: no RiverLine colour inside the skin: ") + What).c_str());
}

/** An open event on a site's lobby the tables can run, or false. */
bool SitesOpenEvent(ss::Session& S, SiteId Site, ss::net::EventInstance& Out, ss::Chips MaxBuyIn)
{
	const ss::net::Network& Net = ss::net::Site(Site);
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

// ------------------------------------------------------------------ the skins

void SitesSkins()
{
	// Contrast (§6.7): Ink on Panel and Muted on Bg at 4.5:1 or more, for every site's palette.
	for (const ss::net::SiteDef& D : ss::net::Sites())
	{
		const ss::ui::Skin K = ss::ui::Skin::FromHex(D.Palette);
		const double InkPanel = SitesContrast(K.Ink, K.Panel);
		const double MutedBg = SitesContrast(K.Muted, K.Bg);
		std::printf("  skin %-10s Ink on Panel %.2f:1, Muted on Bg %.2f:1\n", D.Key.c_str(), InkPanel, MutedBg);
		SitesExpect(InkPanel >= 4.5 && MutedBg >= 4.5, ("sites: " + D.Key + "'s skin reads (Ink on Panel, Muted on Bg at 4.5:1 or more)").c_str());
	}
	SitesExpect(ss::net::Sites().size() >= 3, "sites: Ignite and RunItTwice are registered (core-sites)");
	// SitesTone: RiverLine's literals stay exact at home and move with the skin inside a rival's scope.
	{
		using namespace ss::ui::rlsites_detail;
		const ss::ui::Color Home = SitesTone(0x101a2b);
		SitesExpect(SitesRgb(Home) == 0x101a2b && SitesHome(), "sites: at home a toned literal is exactly itself");
		const ss::ui::SkinScope In(ss::ui::Skin::FromHex(ss::net::Def(SiteId::Ignite).Palette));
		const ss::ui::Color Away = SitesTone(0x101a2b);
		SitesExpect(!SitesHome() && SitesRgb(Away) != 0x101a2b, "sites: inside Ignite's scope a toned literal is Ignite's shade");
	}
	SitesExpect(ss::ui::rlsites_detail::SitesHome(), "sites: the scope put RiverLine's tokens back");
	// Update 3 (§1.11, §2.3): Ignite's Afterburner palette, once core-sites-look sets one, reads as well as its day palette.
	const ss::net::SiteDef& Ig = ss::net::Def(SiteId::Ignite);
	if (std::any_of(Ig.NightPalette.begin(), Ig.NightPalette.end(), [](uint32_t V) { return V != 0; }))
	{
		const ss::ui::Skin K = ss::ui::Skin::FromHex(Ig.NightPalette);
		const double InkPanel = SitesContrast(K.Ink, K.Panel);
		const double MutedBg = SitesContrast(K.Muted, K.Bg);
		std::printf("  skin %-10s Ink on Panel %.2f:1, Muted on Bg %.2f:1\n", "afterburner", InkPanel, MutedBg);
		SitesExpect(InkPanel >= 4.5 && MutedBg >= 4.5, "sites: Ignite's Afterburner skin reads (Ink on Panel, Muted on Bg at 4.5:1 or more)");
	}
}

// ------------------------------------------------------------------ update 3, G1: the looks (ui-tablestyle)

void SitesLooks()
{
	namespace sd = ss::ui::rlsites_detail;
	using ss::ui::Vec2;
	// RiverLine's look is today's literals (the no-op proof; its draw lists are the re-baselined day 0's, checked by the JSON).
	const sd::SiteLook& Home = sd::SitesLook(SiteId::RiverLine);
	SitesExpect(Home.Shape == sd::TableShape::Oval && !Home.SeatList && Home.Cut == 0.0f && Home.Radius == 10.0f && Home.PanelRadius == 14.0f && Home.ButtonH == 66.0f &&
			Home.Button == sd::ButtonShape::Rounded && Home.Cx == 590.0f && Home.Cy == 430.0f && Home.Rx == 420.0f && Home.Ry == 226.0f && Home.ChipSet == 0 && Home.CardBack == 0 &&
			Home.Dealer == 0 && Home.Seat == sd::SeatStyle::Plate && !Home.BetsInBb && !Home.RestoreAlpha && Home.TitleFace < 0 && !Home.LabelCaps && Home.ChromeH == 64.0f,
		"looks: RiverLine's look is today's literals");
	const sd::SiteLook& Ig = sd::SitesLook(SiteId::Ignite);
	const sd::SiteLook& Rit = sd::SitesLook(SiteId::RunItTwice);
	SitesExpect(Ig.Shape == sd::TableShape::Capsule && Ig.Seat == sd::SeatStyle::Bubble && Ig.ChipSet == 1 && Ig.CardBack == 1 && Ig.Dealer == 1 && Ig.Cut == 0.0f && Ig.Button == sd::ButtonShape::Pill &&
			Ig.ChromeH == 72.0f && Ig.RestoreAlpha && Ig.SeatList && Ig.Cx == 590.0f && Ig.Rx == 440.0f && Ig.Ry == 220.0f && Ig.Chamfer == 220.0f,
		"looks: Ignite's arcade (the capsule, bubbles, candy, the Sunset back, the sunflower button; no cut)");
	SitesExpect(Rit.Shape == sd::TableShape::Octagon && Rit.Seat == sd::SeatStyle::DataCard && Rit.ChipSet == 2 && Rit.CardBack == 2 && Rit.Dealer == 2 && Rit.Cut == 10.0f &&
			Rit.Button == sd::ButtonShape::Slab && Rit.BetsInBb && Rit.HudDefault && Rit.Clock24 && Rit.ChromeH == 84.0f && Rit.RestoreAlpha && Rit.Chamfer == 96.0f && Rit.LowerThird.X == 250.0f &&
			Rit.LowerThird.Y == 598.0f && Rit.LowerThird.W == 680.0f && Rit.LowerThird.H == 36.0f,
		"looks: RunItTwice's exchange (the octagon, data cards, lacquer, the Twin Stripe, the puck, BB, the HUD, the cut)");
	SitesExpect(Rit.Seat8[1].X == 280.0f && Rit.Seat8[1].Y == 690.0f && Rit.Seat8[7].X == 900.0f && Rit.Seat8[7].Y == 690.0f && Ig.Seat8[0].Y == 690.0f && Ig.Seat3[0].Y == 720.0f,
		"looks: the seat lists are the spec's (v1.2's bottom seats in the hero's row)");
	// Every list keeps its seats' boxes inside the client, clear of SidePanel (x 1188), of each other, and RunItTwice's of the
	// lower third, at 8, 6 and 3 seats (§2.5, §3.5).
	for (const sd::SiteLook* L : {&Ig, &Rit})
	{
		const char* Who = L == &Ig ? "Ignite" : "RunItTwice";
		for (const int N : {8, 6, 3})
		{
			std::vector<ss::ui::Rect> Boxes;
			for (int K = 0; K < N; ++K)
			{
				const Vec2 Pt = N == 8 ? L->Seat8[static_cast<size_t>(K)] : N == 6 ? L->Seat6[static_cast<size_t>(K)] : L->Seat3[static_cast<size_t>(K)];
				Boxes.push_back(sd::SitesSeatBox(*L, Pt, K == 0));
			}
			bool Clear = true;
			for (size_t A = 0; A < Boxes.size(); ++A)
			{
				const ss::ui::Rect& Ba = Boxes[A];
				Clear = Clear && Ba.X >= 0.0f && Ba.X + Ba.W < 1188.0f && Ba.Y >= L->ChromeH && Ba.Y + Ba.H <= 962.0f;
				const ss::ui::Rect& Lt = L->LowerThird;
				Clear = Clear && (Lt.W <= 0.0f || !(Ba.X < Lt.X + Lt.W && Lt.X < Ba.X + Ba.W && Ba.Y < Lt.Y + Lt.H && Lt.Y < Ba.Y + Ba.H));
				for (size_t B = A + 1; B < Boxes.size(); ++B)
				{
					const ss::ui::Rect& Bb = Boxes[B];
					Clear = Clear && !(Ba.X < Bb.X + Bb.W && Bb.X < Ba.X + Ba.W && Ba.Y < Bb.Y + Bb.H && Bb.Y < Ba.Y + Ba.H);
				}
			}
			SitesExpect(Clear, (std::string("looks: ") + Who + "'s " + std::to_string(N) + " seats clear of SidePanel, the lower third and each other").c_str());
		}
	}
	// Ink on a filled accent (§3.4, §13.2 #15): at home the dark ink whatever the fill; inside Ignite's skin (day and Afterburner)
	// the dark mix wins on every filled kind, so its buttons and pills don't move; on RunItTwice's red, Ink, at 4.5:1 or more.
	const auto Dark = [](const ss::ui::Color& Fill, float K) { return ss::ui::Mix(Fill, ss::ui::Hex(0x000000), K); };
	const auto Same = [](const ss::ui::Color& A, const ss::ui::Color& B) { return A.R == B.R && A.G == B.G && A.B == B.B && A.A == B.A; };
	SitesExpect(sd::SitesHome() && Same(sd::SitesLabelInk(ss::ui::Hex(0x07121c), ss::ui::Hex(0x9b5cff), ss::ui::Hex(0x9b5cff)), ss::ui::Hex(0x07121c)),
		"looks: at home a pill's label is the dark ink whatever the fill (Kast's, the Store's, Rerun's pills never move)");
	std::vector<std::pair<std::string, ss::ui::Skin>> Rivals = {{"ignite", ss::ui::Skin::FromHex(ss::net::Def(SiteId::Ignite).Palette)}};
	if (std::any_of(ss::net::Def(SiteId::Ignite).NightPalette.begin(), ss::net::Def(SiteId::Ignite).NightPalette.end(), [](uint32_t V) { return V != 0; }))
	{
		Rivals.push_back({"afterburner", ss::ui::Skin::FromHex(ss::net::Def(SiteId::Ignite).NightPalette)});
	}
	for (const auto& Rv : Rivals)
	{
		const ss::ui::SkinScope In(Rv.second);
		bool Kept = true;
		for (const auto& Fk : {std::make_pair(ss::ui::pal::Accent, 0.16f), std::make_pair(ss::ui::pal::Gold, 0.17f)})
		{
			const ss::ui::Color Base = Dark(Fk.first, Fk.second);
			Kept = Kept && Same(sd::SitesLabelInk(Dark(Fk.first, 0.86f), Base, Fk.first), Dark(Fk.first, 0.86f));
			Kept = Kept && Same(sd::SitesLabelInk(sd::SitesTone(0x07121c), Fk.first, Fk.first), sd::SitesTone(0x07121c));
		}
		SitesExpect(Kept, ("looks: inside " + Rv.first + "'s skin the filled buttons and pills keep the dark ink").c_str());
	}
	{
		const ss::ui::SkinScope In(ss::ui::Skin::FromHex(ss::net::Def(SiteId::RunItTwice).Palette));
		const ss::ui::Color Red = ss::ui::pal::Accent;
		const ss::ui::Color Base = Dark(Red, 0.16f);
		const ss::ui::Color Ink = sd::SitesLabelInk(Dark(Red, 0.86f), Base, Red);
		const double Reads = std::min(SitesContrast(Ink, Base), SitesContrast(Ink, Red));
		std::printf("  label ink on RunItTwice's primary fill: %06x, %.2f:1 at the lower of its two shades\n", SitesRgb(Ink), Reads);
		SitesExpect(Reads >= 4.5, "looks: a label on RunItTwice's filled accent reads at 4.5:1 or more");
		// The pills: a solid pill on Signal Red takes the ink that reads.
		const ss::ui::Color Pill = sd::SitesLabelInk(sd::SitesTone(0x07121c), Red, Red);
		SitesExpect(SitesContrast(Pill, Red) >= 4.5, "looks: a solid pill's label on Signal Red reads at 4.5:1 or more");
	}
	// The cut (§13.2 #5): five points, the top-right corner cut at 45 degrees.
	{
		const std::vector<Vec2> Cut = sd::SitesCutPath({100.0f, 50.0f, 200.0f, 60.0f}, 10.0f);
		SitesExpect(Cut.size() == 5 && Cut[1].X == 290.0f && Cut[1].Y == 50.0f && Cut[2].X == 300.0f && Cut[2].Y == 60.0f, "looks: the cut corner is a 45-degree cut of the top right");
	}
	// The night: Ignite's palette at 2 AM is its Afterburner one when core-sites-look has set it, at 9 PM its day one; RiverLine
	// and RunItTwice wear theirs at every hour.
	{
		const double Day = 30.0 * SitesDay;
		const auto Hexes = [](const ss::ui::Skin& K) { return SitesHexes(K); };
		const std::set<uint32_t> Night = Hexes(sd::SitesSkin(SiteId::Ignite, Day + 2.0 * 60.0));
		const std::set<uint32_t> Prime = Hexes(sd::SitesSkin(SiteId::Ignite, Day + 21.0 * 60.0));
		const std::set<uint32_t> IgDay = Hexes(ss::ui::Skin::FromHex(ss::net::Def(SiteId::Ignite).Palette));
		const bool HasNight = std::any_of(ss::net::Def(SiteId::Ignite).NightPalette.begin(), ss::net::Def(SiteId::Ignite).NightPalette.end(), [](uint32_t V) { return V != 0; });
		SitesExpect(Prime == IgDay && (HasNight ? Night == Hexes(ss::ui::Skin::FromHex(ss::net::Def(SiteId::Ignite).NightPalette)) : Night == IgDay),
			"looks: Ignite's skin is Afterburner's at 2 AM (when it has one) and the day's at 9 PM");
		SitesExpect(Hexes(sd::SitesSkin(SiteId::RunItTwice, Day + 2.0 * 60.0)) == Hexes(ss::ui::Skin::FromHex(ss::net::Def(SiteId::RunItTwice).Palette)) &&
				Hexes(sd::SitesSkin(SiteId::RiverLine, Day + 2.0 * 60.0)) == Hexes(ss::ui::Skin::FromHex(ss::net::Def(SiteId::RiverLine).Palette)),
			"looks: RunItTwice and RiverLine wear their own palettes at 2 AM");
	}
	// Pages: Profile is both rivals' (the Player Card, the Member File), never RiverLine's.
	SitesExpect(sd::SitesPageHome(SiteId::RunItTwice, Page::Profile) == Page::Profile && sd::SitesPageHome(SiteId::Ignite, Page::Profile) == Page::Profile &&
			sd::SitesPageHome(SiteId::RiverLine, Page::Profile) == Page::Lobby && sd::SitesPageHome(SiteId::RunItTwice, Page::Fuse) == Page::Lobby,
		"looks: Profile is both rivals' page; FUSE stays Ignite's");
	SitesExpect(sd::SitesBbText(3850, 100) == "38.5 BB" && sd::SitesBbText(25000, 200) == "125 BB", "looks: amounts in big blinds read \"38.5 BB\", \"125 BB\"");
	// The table strip: every open table in the session's order with its site (§2.4 bubbles, §3.4 Seated).
	{
		SitesLaptop P("sites-strip");
		P.At(SitesEvening);
		P.S.BankrollCents = 200000;
		P.Join(SiteId::Ignite, 50000);
		ss::net::EventInstance E;
		if (SitesOpenEvent(P.S, SiteId::Ignite, E, 2200))
		{
			P.S.RegisterEvent(ss::net::Site(SiteId::Ignite).Listing(E, nullptr, P.S.Unlocks()));
			const std::vector<sd::StripRow> Rows = sd::SitesTableStrip(P.RL, P.S);
			SitesExpect(Rows.size() == 1 && Rows[0].Site == SiteId::Ignite && Rows[0].Table == 0 && !Rows[0].Event.empty() && Rows[0].StackBb > 0.0, "looks: the table strip lists Ignite's table with its stack in big blinds");
		}
	}
}

// ------------------------------------------------------------------ a rival's lobby and pages

void SitesLobbies()
{
	for (const SiteId Site : {SiteId::Ignite, SiteId::RunItTwice})
	{
		const std::string Key = ss::net::KeyOf(Site);
		SitesLaptop P("sites-lobby-" + Key);
		ss::Session& S = P.S;
		P.At(SitesEvening);
		S.BankrollCents = 150000;
		P.Join(Site, 21240);
		P.RL.SiteShown = Site;
		P.RL.OpenPage(Page::Lobby, P.Now);
		const DrawList L = P.Settled();
		WriteList("site_lobby_" + Key, L);
		const std::string Short = ss::ui::rlsites_detail::SitesShortName(Site);
		if (Site == SiteId::RunItTwice)
		{
			// Update 3 (G1, SPEC §7.7's deliberate rewrite, ui-rit's patch): RunItTwice's rail of plates in caps (BOARD ... CLUB, CASHIER;
			// LEAGUE hidden until G2), and its Board in place of the featured slides (SECOND RUN CLUB's kicker moved to the Club page).
			SitesExpect(DrewRun(L, "BOARD") && DrewRun(L, "CLUB") && DrewRun(L, "CASHIER") && !DrewRun(L, "LEAGUE") && !DrewRun(L, "Career") && !DrewRun(L, "Embercrest"),
				("sites: " + Short + "'s rail has its own pages").c_str());
		}
		else
		{
			// Update 3 (G1, SPEC §7.7's deliberate rewrite): Ignite's dock, PLAY, REWARDS, the raised FLIP (FUSE from G2), BOARDS and
			// WALLET; nothing of RiverLine's.
			SitesExpect(DrewRun(L, "PLAY") && DrewRun(L, "REWARDS") && DrewRun(L, "FLIP") && DrewRun(L, "BOARDS") && DrewRun(L, "WALLET") && !DrewRun(L, "Career") && !DrewRun(L, "Embercrest"),
				("sites: " + Short + "'s dock has its own pages").c_str());
		}
		SitesExpect(DrewRun(L, "Balance $212.40"), ("sites: " + Short + "'s top bar shows its wallet").c_str());
		SitesExpect(!P.RL.ListedEvents().empty() && std::all_of(P.RL.ListedEvents().begin(), P.RL.ListedEvents().end(), [Site](const std::string& Id) { return ss::net::SiteOf(Id) == Site; }),
			("sites: " + Short + "'s lobby lists its own events only").c_str());
		if (Site == SiteId::RunItTwice)
		{
			SitesExpect(DrewRun(L, "THE BOARD"), ("sites: " + Short + " opens on its own Board").c_str());
		}
		else
		{
			// The slides' TONIGHT kicker moved to the Bandstand (§2.4 A).
			bool Tonight = false;
			for (const ss::ui::DrawCmd& Cmd : L.Cmds)
			{
				Tonight = Tonight || (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.rfind("TONIGHT", 0) == 0);
			}
			SitesExpect(Tonight, ("sites: " + Short + "'s Bandstand says what's on TONIGHT").c_str());
		}
		std::printf("  %s's lobby: %zu events listed, %zu vertices\n", Short.c_str(), P.RL.ListedEvents().size(), L.Vertices.size());
		SitesNoLeak(L, Site, (Key + " lobby").c_str());
		// Every page a rival shows, in its skin.
		const std::pair<Page, const char*> Pages[5] = {{Page::Series, "series"}, {Page::Leaderboards, "boards"}, {Page::News, "news"}, {Page::Promotions, "promotions"}, {Page::Cashier, "cashier"}};
		for (const auto& Pg : Pages)
		{
			P.RL.OpenPage(Pg.first, P.Now);
			const DrawList Lp = P.Settled();
			WriteList("site_" + std::string(Pg.second) + "_" + Key, Lp);
			SitesNoLeak(Lp, Site, (Key + " " + Pg.second).c_str());
		}
		// A RiverLine page asked for on a rival's client goes to its lobby.
		P.RL.OpenPage(Page::Career, P.Now);
		P.Settled();
		SitesExpect(P.RL.CurrentPage() == Page::Lobby, ("sites: " + Short + " has no Career page (it's RiverLine's)").c_str());
		// The boards: the site's own, RiverLine's never.
		P.RL.OpenPage(Page::Leaderboards, P.Now);
		const DrawList Lb = P.Settled();
		SitesExpect(!DrewRun(Lb, "Night Shift") && !DrewRun(Lb, "Honors") && (Site != SiteId::Ignite || DrewRun(Lb, "This Month")), ("sites: " + Short + "'s boards are its own").c_str());
		// The other rival, never joined on this career (a laptop outlives a career): its lobby isn't reachable, RiverLine's shows.
		P.RL.SiteShown = Site == SiteId::Ignite ? SiteId::RunItTwice : SiteId::Ignite;
		P.RL.OpenPage(Page::Lobby, P.Now);
		const DrawList Ln = P.Settled();
		SitesExpect(DrewRun(Ln, "Career") && !DrewRun(Ln, "Cashier") && DrewRun(Ln, "Balance $1,500.00"), ("sites: no account, no lobby (" + Short + "'s career)").c_str());
		// Back home: RiverLine's nav again.
		P.RL.SiteShown = SiteId::RiverLine;
		P.RL.OpenPage(Page::Lobby, P.Now);
		const DrawList Lh = P.Settled();
		SitesExpect(DrewRun(Lh, "Career") && !DrewRun(Lh, "Cashier") && DrewRun(Lh, "Balance $1,500.00"), "sites: RiverLine's lobby is RiverLine's again");
	}
}

// ------------------------------------------------------------------ the Cashier

void SitesCashier()
{
	SitesLaptop P("sites-cashier");
	ss::Session& S = P.S;
	P.At(SitesEvening);
	S.BankrollCents = 50000;
	P.Join(SiteId::Ignite, 0);
	P.RL.SiteShown = SiteId::Ignite;
	P.RL.OpenPage(Page::Cashier, P.Now);
	DrawList L = P.Settled();
	// Update 3 (G1): Ignite's WALLET heads its two panels in caps (§2.4 K); the panels, the steps and the money are as before.
	SitesExpect(DrewRun(L, "Your balance on Ignite") && (DrewRun(L, "Deposit") || DrewRun(L, "DEPOSIT")) && (DrewRun(L, "Withdraw") || DrewRun(L, "WITHDRAW")),
		"cashier: the balance, a deposit and a withdrawal");
	// Deposit $50 by card: the bank pays $51.50 (3%), the wallet gets $50.
	const SitesBox Step50 = SitesRun(L, "$50", P.M, 380.0f, 440.0f);
	SitesExpect(Step50.Found, "cashier: the $50 step");
	if (Step50.Found)
	{
		P.Click(Step50.X + Step50.W / 2.0f, Step50.Base - 6.0f);
	}
	L = P.Settled();
	SitesExpect(DrewRun(L, "$51.50"), "cashier: the bank pays the amount and the card's 3%");
	const SitesBox Go = SitesRun(L, "Deposit $50", P.M);
	SitesExpect(Go.Found, "cashier: Deposit $50");
	if (Go.Found)
	{
		P.Click(Go.X + Go.W / 2.0f, Go.Base - 6.0f);
	}
	const ss::site::Account* A = ss::site::AccountOf(S.Life.Sites, SiteId::Ignite);
	SitesExpect(A && A->Cents == 5000 && S.BankrollCents == 50000 - 5150, "cashier: the deposit moved $50 onto Ignite and $51.50 out of the bank");
	L = P.Settled();
	WriteList("site_cashier", L);
	SitesExpect(DrewRun(L, "Balance $50.00"), "cashier: the top bar shows the wallet");
	SitesNoLeak(L, SiteId::Ignite, "cashier after a deposit");
	// A withdrawal of $20: in transit, net of the fee, with its window in words.
	const SitesBox Twenty = SitesRun(L, "$20", P.M, 380.0f, 440.0f);
	if (Twenty.Found)
	{
		P.Click(Twenty.X + Twenty.W / 2.0f, Twenty.Base - 6.0f);
	}
	L = P.Settled();
	const SitesBox Out = SitesRun(L, "Withdraw $20.00", P.M);
	SitesExpect(Out.Found, "cashier: Withdraw $20.00");
	if (Out.Found)
	{
		P.Click(Out.X + Out.W / 2.0f, Out.Base - 6.0f);
	}
	L = P.Settled();
	WriteList("site_cashier_transit", L);
	SitesExpect(ss::ui::rlsites_detail::SitesInTransit(S) > 0, "cashier: the withdrawal is in transit");
	bool Arriving = false;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		Arriving = Arriving || (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.find(" arriving ") != std::string::npos);
	}
	SitesExpect(Arriving, "cashier: money in transit says \"arriving\" (never colour alone)");
	// To the cent (§5.5): $20 left the wallet, $3 is the fee, $17 is on its way; the wallet holds $30.
	A = ss::site::AccountOf(S.Life.Sites, SiteId::Ignite);
	SitesExpect(A && A->Cents == 3000 && ss::ui::rlsites_detail::SitesInTransit(S) == 1700 && DrewRun(L, "$17.00") && DrewRun(L, "Balance $30.00") && DrewRun(L, "$17.00 arriving Sat to Mon") &&
			DrewRun(L, "Withdrawal of $20.00 \xC2\xB7 requested Wed 7:30 PM"),
		"cashier: the wallet down $20, $17 in transit after the $3 fee");
	SitesNoLeak(L, SiteId::Ignite, "cashier in transit");
}

// ------------------------------------------------------------------ Promotions

void SitesPromotions()
{
	SitesLaptop P("sites-promotions");
	ss::Session& S = P.S;
	P.At(SitesEvening);
	S.BankrollCents = 50000;
	P.Join(SiteId::Ignite, 0);
	S.Deposit(SiteId::Ignite, 5000, "card");
	P.RL.SiteShown = SiteId::Ignite;
	P.RL.OpenPage(Page::Promotions, P.Now);
	DrawList L = P.Settled();
	// Update 3 (G1, SPEC §7.7's deliberate rewrite): Ignite's welcome copy lives on REWARDS › Welcome bonus (its sub-pill, rw3).
	const SitesBox Sub = SitesRun(L, "Welcome bonus", P.M, 88.0f, 128.0f);
	if (Sub.Found)
	{
		P.Click(Sub.X + Sub.W / 2.0f, Sub.Base - 6.0f);
		L = P.Settled();
	}
	WriteList("site_promotions", L);
	SitesExpect(Sub.Found && (SitesSpelled(L, "WELCOME BONUS") || DrewRun(L, "WELCOME BONUS")) && DrewRun(L, "$50.00 still pending"), "promotions: Ignite's bonus bar, $50 pending (REWARDS › Welcome bonus)");
	SitesNoLeak(L, SiteId::Ignite, "promotions (Ignite)");
	// The 60 days up: core-sites' SiteStep drops what was still pending, so the page says what was released and that the
	// window closed, never "All of it's yours now." (and never the first-deposit pitch again).
	if (ss::site::Account* Ig = ss::site::AccountOf(S.Life.Sites, SiteId::Ignite))
	{
		Ig->BonusReleased = 1500;
		Ig->BonusPending = 0;
		Ig->BonusExpires = S.WorldMinutes() - 60.0;
	}
	L = P.Settled();
	bool Closed = false;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		Closed = Closed || (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.rfind("The 60 days ran out on ", 0) == 0);
	}
	SitesExpect(DrewRun(L, "$15.00 released") && Closed && !DrewRun(L, "All of it's yours now.") && !DrewRun(L, "Your first deposit starts it: 100% of it, up to $100."),
		"promotions: an expired welcome bonus says the window closed, not that it's all yours");
	P.Join(SiteId::RunItTwice, 0);
	S.Deposit(SiteId::RunItTwice, 5000, "card");
	P.RL.SiteShown = SiteId::RunItTwice;
	P.RL.OpenPage(Page::Promotions, P.Now);
	L = P.Settled();
	WriteList("site_promotions_club", L);
	// The kickers stay on the Club page (§3.4 G), tracked glyphs or one run in the look's face.
	SitesExpect((SitesSpelled(L, "SECOND RUN CLUB") || DrewRun(L, "SECOND RUN CLUB")) && (SitesSpelled(L, "WELCOME TICKETS") || DrewRun(L, "WELCOME TICKETS")),
		"promotions: RunItTwice's club and its welcome tickets");
	bool Ticket = false;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		Ticket = Ticket || (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == "TICKET");
	}
	SitesExpect(Ticket, "promotions: the welcome tickets held");
	SitesNoLeak(L, SiteId::RunItTwice, "promotions (RunItTwice)");
	// Only a first deposit of $50 or more brings the tickets (core-sites): one under it had its chance, and the page says so.
	{
		SitesLaptop Q("sites-promotions-missed");
		Q.At(SitesEvening);
		Q.S.BankrollCents = 50000;
		Q.Join(SiteId::RunItTwice, 0);
		Q.S.Deposit(SiteId::RunItTwice, 2000, "card");
		Q.RL.SiteShown = SiteId::RunItTwice;
		Q.RL.OpenPage(Page::Promotions, Q.Now);
		const DrawList Lq = Q.Settled();
		bool Missed = false;
		for (const ss::ui::DrawCmd& Cmd : Lq.Cmds)
		{
			Missed = Missed || (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.find("under $50") != std::string::npos);
		}
		SitesExpect(Missed && !DrewRun(Lq, "None held right now."), "promotions: a first deposit under $50 says the welcome tickets passed it by");
	}
}

// ------------------------------------------------------------------ the poker button and the taskbar

void SitesPokerButton()
{
	SitesLaptop P("sites-poker-button");
	ss::Session& S = P.S;
	P.At(SitesEvening);
	S.BankrollCents = 123400;
	// No rival yet: the button is RiverLine's alone, no caret.
	DrawList L = P.Settled();
	SitesExpect(SitesRun(L, "RiverLine", P.M, 962.0f, 1000.0f).Found, "poker button: RiverLine's, alone");
	P.Join(SiteId::Ignite, 21240);
	P.Join(SiteId::RunItTwice, 6400);
	L = P.Settled();
	const SitesBox Rl = SitesRun(L, "RiverLine", P.M, 962.0f, 1000.0f);
	SitesExpect(Rl.Found, "poker button: the site in front's name");
	// The caret, at the button's right: 40 + the name + 12, then 20 for the caret.
	const float CaretX = 8.0f + 40.0f + Rl.W + 12.0f + 10.0f;
	P.Click(CaretX, 981.0f);
	L = P.Settled();
	WriteList("poker_button_menu", L);
	SitesExpect(ss::ui::rlsites_detail::SitesMenuOpen(&P.RL), "poker button: the caret opens the list");
	SitesExpect(DrewRun(L, "Ignite Poker") && DrewRun(L, "$212.40") && DrewRun(L, "RunItTwice") && DrewRun(L, "$64.00") && DrewRun(L, "$1,234.00"), "poker button: every site played, with its balance");
	// Picking Ignite: its lobby, in its skin.
	const SitesBox Ig = SitesRun(L, "Ignite Poker", P.M);
	if (Ig.Found)
	{
		P.Click(Ig.X + 4.0f, Ig.Base - 6.0f);
	}
	L = P.Settled();
	SitesExpect(P.RL.SiteShown == SiteId::Ignite && !ss::ui::rlsites_detail::SitesMenuOpen(&P.RL) && P.RL.CurrentPage() == Page::Lobby, "poker button: a site picked opens its lobby");
	SitesExpect(SitesRun(L, "Ignite", P.M, 962.0f, 1000.0f).Found, "poker button: it shows the site in front");
	// The lane (§5.11): nine apps, Sleep and the caret leave the news wire 120 at 1,600 wide, the status at its widest.
	S.Life.Heat = 100.0;
	S.Life.Energy = 100.0;
	P.RL.SiteShown = SiteId::RunItTwice; // the longest name on the button
	L = P.Settled();
	float Live = -1.0f;
	float Energy = -1.0f;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.BaselineY >= 962.0f)
		{
			Live = Cmd.Text.Text == "LIVE" ? Cmd.Text.X : Live;
			Energy = !Cmd.Text.Text.empty() && Cmd.Text.Text.back() == '%' ? Cmd.Text.X : Energy;
		}
	}
	const float Lane = Live >= 0.0f && Energy >= 0.0f ? Energy - 40.0f - 18.0f - (Live - 18.0f) : -1.0f;
	// The widest weekday and date and the widest clock the status can draw, over the ones drawn.
	const ss::ui::Font DayFace = ss::ui::Canvas::FaceFor(600, false);
	const ss::ui::Font Clock = ss::ui::Canvas::FaceFor(700, true);
	const auto DayText = [](int D) { return std::string(ss::net::WeekdayName(D)) + ", " + ss::net::DateLabel(D); };
	float DayMax = 0.0f;
	for (int D = 0; D < 2 * 366; ++D)
	{
		DayMax = std::max(DayMax, P.M.Width(DayText(D), DayFace, 12.0f));
	}
	float ClockMax = 0.0f;
	for (int M = 0; M < 24 * 60; ++M)
	{
		ClockMax = std::max(ClockMax, P.M.Width(ss::net::TimeLabel(static_cast<double>(M)), Clock, 14.0f));
	}
	const double World = S.WorldMinutes();
	const float Worst = Lane - (DayMax - P.M.Width(DayText(ss::net::DayOf(World)), DayFace, 12.0f)) - (ClockMax - P.M.Width(ss::net::TimeLabel(World), Clock, 14.0f));
	std::printf("  taskbar with the caret: the news wire's lane %.0f wide here, %.0f at its narrowest (needs 120)\n", Lane, Worst);
	SitesExpect(Lane > 0.0f && Worst >= 120.0f, "taskbar: nine apps, Sleep and the poker button's caret leave the lane its 120");
}

// ------------------------------------------------------------------ the Bank, Career, the rent card

void SitesBank()
{
	SitesLaptop P("sites-bank");
	ss::Session& S = P.S;
	P.At(SitesEvening);
	// Rent paid up (the career would be on the couch by now otherwise): the shot is the pots.
	S.Life.RentStage = ss::life::Rent::Paid;
	S.Life.RentsPaid = 2;
	S.Life.RentDeadline = S.WorldMinutes() + 20.0 * SitesDay;
	S.Life.RentDueCents = 107500;
	S.BankrollCents = 80000;
	S.Life.Ledger.clear();
	P.Join(SiteId::Ignite, 0);
	S.Deposit(SiteId::Ignite, 10000, "card");
	ss::site::Account* A = ss::site::AccountOf(S.Life.Sites, SiteId::Ignite);
	if (A)
	{
		A->Cents += 11240; // a cash there
	}
	S.Withdraw(SiteId::Ignite, 5000, "bank");
	P.RL.OpenApp(App::Bank, P.Now);
	const DrawList L = P.Settled();
	WriteList("bank_pots", L);
	SitesExpect(SitesSpelled(L, "IGNITE") && SitesSpelled(L, "IN TRANSIT"), "bank: the pots list (Ignite's wallet, money in transit)");
	// The fee on the row itself (its own baseline), not just somewhere on the page (the history lists it too).
	const SitesBox Row = SitesRun(L, "Poker & sites", P.M);
	bool FeeOnRow = false;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		FeeOnRow = FeeOnRow || (Row.Found && Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == "$3.00" && std::fabs(Cmd.Text.BaselineY - Row.Base) < 2.0f);
	}
	SitesExpect(Row.Found && FeeOnRow, "bank: the card fee sits in the \"Poker & sites\" row");
	SitesExpect(DrewRun(L, "Deposit: Ignite") || DrewRun(L, "Ignite card fee"), "bank: the deposit's own lines in the history");
	bool Arriving = false;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		Arriving = Arriving || (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.find("arriving") != std::string::npos);
	}
	SitesExpect(Arriving, "bank: money in transit is said in words (\"arriving\")");
}

void SitesCareer()
{
	SitesLaptop P("sites-career");
	ss::Session& S = P.S;
	P.At(SitesEvening);
	S.Life.Sites.RiverLine.MonthKey = ss::site::MonthKeyAt(S.WorldMinutes());
	S.Life.Sites.RiverLine.Rake = 5240;
	P.RL.OpenPage(Page::Career, P.Now);
	DrawList L = P.Settled();
	WriteList("career_rewards", L);
	SitesExpect(SitesSpelled(L, "RIVERLINE REWARDS") && DrewRun(L, "Silver") && DrewRun(L, "8% back"), "career: the Rewards card, Silver at $52.40 of rake");
	SitesExpect(SitesSpelled(L, "YOUR RIVAL"), "career: the rival's card still there, under it");
	// RiverLine notices (§5.7): the retention letter's tier up shows on the card.
	S.Life.Sites.RiverLine.BoostUntil = S.WorldMinutes() + 60.0 * SitesDay;
	L = P.Settled();
	SitesExpect(DrewRun(L, "Gold") && DrewRun(L, "14% back"), "career: one tier up while the retention offer runs");
	// A boost that ends before the 1st doesn't reach this month's payout (core-sites pays one tier up only when the 1st falls
	// inside it), so the card shows the tier the 1st will pay: Silver, no "+1 TIER".
	S.Life.Sites.RiverLine.BoostUntil = S.WorldMinutes() + 10.0 * SitesDay; // Nov 14: the boost is over by Dec 1
	L = P.Settled();
	bool Pill = false;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		Pill = Pill || (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.rfind("+1 TIER", 0) == 0);
	}
	SitesExpect(DrewRun(L, "Silver") && DrewRun(L, "8% back") && !DrewRun(L, "14% back") && !Pill, "career: a boost that's over before the 1st doesn't show a tier the 1st won't pay");
}

void SitesRent()
{
	SitesLaptop P("sites-rent");
	ss::Session& S = P.S;
	P.At(SitesEvening);
	S.BankrollCents = 120000;
	P.Join(SiteId::Ignite, 0);
	S.Deposit(SiteId::Ignite, 20000, "card");
	S.Withdraw(SiteId::Ignite, 20000, "bank");
	// The rent due in two days, more than the bank holds; the bank and the money on its way would cover it.
	ss::life::State& Lf = S.Life;
	Lf.RentStage = ss::life::Rent::Due;
	Lf.RentDeadline = S.WorldMinutes() + 48.0 * 60.0;
	Lf.RentDueCents = S.BankrollCents + 10000;
	P.RL.OpenApp(App::Bank, P.Now);
	const DrawList L = P.Settled();
	WriteList("rent_card_sites", L);
	bool Line = false;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		Line = Line || (Cmd.Type == ss::ui::DrawCmd::Kind::Text && (Cmd.Text.Text.find("Site money") != std::string::npos || Cmd.Text.Text.find("pay rent.") != std::string::npos));
	}
	const std::string Said = ss::ui::rlsites_detail::SitesRentLine(S, S.WorldMinutes());
	std::printf("  the rent card: \"%s\"\n", Said.c_str());
	SitesExpect(!Said.empty() && Line, "rent card: the bank's short, the sites aren't: \"Site money can't pay rent.\"");
}

void SitesMarks()
{
	// The marks (§1.6): a withdrawal on its way is on the second screen's COMING UP in core-sites' words, and written on the
	// wall calendar in the site's name; RiverLine Rewards' payday on the 1st.
	SitesLaptop P("sites-marks");
	ss::Session& S = P.S;
	P.At(SitesEvening);
	S.BankrollCents = 50000;
	P.Join(SiteId::Ignite, 0);
	S.Deposit(SiteId::Ignite, 5000, "card");
	S.Withdraw(SiteId::Ignite, 2000, "bank");
	DrawList Screen;
	{
		ss::ui::Canvas C(Screen, P.M, ss::ui::secondscreen::Width, ss::ui::secondscreen::Height, 1.0f);
		ss::ui::secondscreen::Draw(C, S, 0, P.Now);
	}
	WriteList("second_screen_sites", Screen);
	bool Coming = false;
	for (const ss::ui::DrawCmd& Cmd : Screen.Cmds)
	{
		Coming = Coming || (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.rfind("Ignite withdrawal", 0) == 0);
	}
	SitesExpect(Coming, "second screen: the Ignite withdrawal on its way is coming up");
	DrawList Wall;
	{
		ss::ui::Canvas C(Wall, P.M, ss::ui::wallcalendar::Width, ss::ui::wallcalendar::Height, 1.4f);
		ss::ui::wallcalendar::Draw(C, S);
	}
	WriteList("wall_calendar_sites", Wall);
	SitesExpect(DrewRun(Wall, "IGNITE"), "wall calendar: the withdrawal's day says IGNITE");
	// RunItTwice's name is the longest word a square gets: written smaller, no wider than the widest of the others, so it
	// clears the date at the square's left (IGNITE keeps its size).
	P.Join(SiteId::RunItTwice, 0);
	S.Deposit(SiteId::RunItTwice, 5000, "card");
	S.Withdraw(SiteId::RunItTwice, 2000, "bank");
	Wall = DrawList();
	{
		ss::ui::Canvas C(Wall, P.M, ss::ui::wallcalendar::Width, ss::ui::wallcalendar::Height, 1.4f);
		ss::ui::wallcalendar::Draw(C, S);
	}
	float LongSize = -1.0f;
	float LongX = 0.0f;
	float LongBase = 0.0f;
	float IgniteSize = -1.0f;
	for (const ss::ui::DrawCmd& Cmd : Wall.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == "RUNITTWICE")
		{
			LongSize = Cmd.Text.SizePx;
			LongX = Cmd.Text.X;
			LongBase = Cmd.Text.BaselineY;
		}
		IgniteSize = Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == "IGNITE" ? Cmd.Text.SizePx : IgniteSize;
	}
	// Its square's date: the number drawn nearest to its left on its line. The word must start clear of it.
	float DateEnd = -1.0f;
	for (const ss::ui::DrawCmd& Cmd : Wall.Cmds)
	{
		const std::string& T = Cmd.Text.Text;
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && !T.empty() && T.size() <= 2 && std::isdigit(static_cast<unsigned char>(T[0])) != 0 &&
			std::fabs(Cmd.Text.BaselineY - LongBase) < 3.0f && Cmd.Text.X < LongX && LongX - Cmd.Text.X < 80.0f)
		{
			DateEnd = std::max(DateEnd, Cmd.Text.X + P.M.Width(T, Cmd.Text.Face, Cmd.Text.SizePx));
		}
	}
	std::printf("  wall calendar: RUNITTWICE at %.1f px from x %.1f, its date ends at %.1f; IGNITE at %.1f px\n", LongSize, LongX, DateEnd, IgniteSize);
	SitesExpect(LongSize > 0.0f && DateEnd > 0.0f && LongX >= DateEnd + 2.0f && IgniteSize == 10.0f, "wall calendar: RUNITTWICE written small enough to clear its date, IGNITE as it was");
	// Two sites' withdrawals on their way: each Cashier counts its own ($17 from Ignite, $15 from RunItTwice after its $5 fee).
	P.RL.SiteShown = SiteId::RunItTwice;
	P.RL.OpenApp(App::RiverLine, P.Now);
	P.RL.OpenPage(Page::Cashier, P.Now);
	const DrawList Lc = P.Settled();
	SitesExpect(ss::ui::rlsites_detail::SitesInTransit(S) == 3200 && DrewRun(Lc, "$15.00") && !DrewRun(Lc, "$32.00") && !DrewRun(Lc, "$17.00"),
		"cashier: in transit is the site's own, not every site's");
}

// ------------------------------------------------------------------ Mail

void SitesMail()
{
	SitesLaptop P("sites-mail");
	ss::Session& S = P.S;
	P.At(SitesEvening);
	S.BankrollCents = 50000;
	ss::site::Invite(S.Life.Sites, S.Life, SiteId::Ignite, S.WorldMinutes(), 0, false);
	P.RL.OpenApp(App::Mail, P.Now);
	DrawList L = P.Settled();
	WriteList("mail_site_invite", L);
	const SitesBox Accept = SitesRun(L, "Accept", P.M);
	SitesExpect(Accept.Found && DrewRun(L, "Decline") && DrewRun(L, "Ignite Poker"), "mail: Ignite's invitation, with Accept and Decline");
	if (Accept.Found)
	{
		P.Click(Accept.X + Accept.W / 2.0f, Accept.Base - 6.0f);
	}
	SitesExpect(ss::site::AccountOf(S.Life.Sites, SiteId::Ignite) != nullptr, "mail: Accept opens the account");
	L = P.Settled();
	const SitesBox Cashier = SitesRun(L, "Open the Cashier", P.M);
	SitesExpect(Cashier.Found, "mail: then the way to the Cashier");
	if (Cashier.Found)
	{
		P.Click(Cashier.X + Cashier.W / 2.0f, Cashier.Base - 6.0f);
	}
	SitesExpect(P.RL.CurrentApp() == App::RiverLine && P.RL.SiteShown == SiteId::Ignite && P.RL.CurrentPage() == Page::Cashier, "mail: Open the Cashier goes to Ignite's Cashier");
	// A first deposit brings RiverLine's "We noticed." (RiverLine Player Relations), with the way to Career.
	S.Deposit(SiteId::Ignite, 5000, "card");
	P.RL.OpenApp(App::Mail, P.Now);
	L = P.Settled();
	SitesExpect(DrewRun(L, "RiverLine Player Relations") && DrewRun(L, "Open Career"), "mail: RiverLine Player Relations' letter, to Career");
	// Declined once, they write again in a month; declined twice, that was the last letter, and Mail says each truly.
	{
		SitesLaptop Q("sites-mail-twice");
		Q.At(SitesEvening);
		ss::Session& Sq = Q.S;
		ss::site::Invite(Sq.Life.Sites, Sq.Life, SiteId::Ignite, Sq.WorldMinutes(), 0, false);
		Sq.AnswerInvite(SiteId::Ignite, false);
		Q.RL.OpenApp(App::Mail, Q.Now);
		DrawList Lq = Q.Settled();
		SitesExpect(DrewRun(Lq, "You passed. They'll write once more, in a month."), "mail: a first invitation declined: they'll write once more");
		Sq.Life.Sites.Done.insert("resent:ignite"); // what SiteStep marks as it sends the second letter
		ss::site::Invite(Sq.Life.Sites, Sq.Life, SiteId::Ignite, Sq.WorldMinutes() + 1.0, 0, true);
		Sq.AnswerInvite(SiteId::Ignite, false);
		Q.RL.OpenApp(App::Mail, Q.Now);
		Lq = Q.Settled();
		SitesExpect(DrewRun(Lq, "You passed twice. They won't write again.") && !DrewRun(Lq, "You passed. They'll write once more, in a month."),
			"mail: a second invitation declined is the last, and Mail says so");
	}
}

// ------------------------------------------------------------------ a rival's table, its tile, the register button

// G1 fix (fix-tablestyle): the rivals' own controls and panels, checked on the tables below (bodies after SitesTables).
void SitesRivalControls(const DrawList& L, SiteId Site, const char* Who);
void SitesRivalGrade(SitesLaptop& P, SiteId Site, const char* Page);
void SitesIgnitePanel(SitesLaptop& P, const DrawList& L);
bool SitesRunIn(const DrawList& L, const std::string& Text, ss::ui::Font Face, const ss::ui::Rect& R);
bool SitesAnyRunIn(const DrawList& L, const std::string& Text, const ss::ui::Rect& R);

void SitesTables()
{
	SitesLaptop P("sites-tables");
	ss::Session& S = P.S;
	P.At(SitesEvening);
	S.BankrollCents = 200000;
	P.Join(SiteId::Ignite, 50000);
	ss::net::EventInstance E;
	if (!SitesOpenEvent(S, SiteId::Ignite, E, 2200))
	{
		std::printf("  sites: no Ignite event open at this hour; the table checks are skipped\n");
		return;
	}
	const ss::LobbyEvent Ev = ss::net::Site(SiteId::Ignite).Listing(E, nullptr, S.Unlocks());
	const ss::Chips Before = S.BankrollCents;
	S.RegisterEvent(Ev);
	S.CurrentPace = ss::Pace::Full;
	const ss::site::Account* A = ss::site::AccountOf(S.Life.Sites, SiteId::Ignite);
	SitesExpect(S.BankrollCents == Before && A && A->Cents == 50000 - Ev.BuyInCents, "tables: an Ignite buy-in comes off Ignite's wallet, the bank untouched");
	P.Now = Step(S, P.Now, 3.0);
	DrawList L = P.Settled();
	WriteList("site_table_ignite", L);
	SitesNoLeak(L, SiteId::Ignite, "Ignite's table");
	// Update 3, G1 (§2.5): the capsule in its neon tube, Plum Velvet, candy chips; within its budget (§10: 22k vertices).
	{
		std::set<uint32_t> Cols;
		for (const ss::ui::Vertex& V : L.Vertices)
		{
			Cols.insert(SitesRgb(V.Col));
		}
		std::printf("  Ignite's table: %zu vertices, %zu commands (budget 22k vertices)\n", L.Vertices.size(), L.Cmds.size());
		SitesExpect(Cols.count(0x4a2142) && Cols.count(0x120a18) && Cols.count(0xff6b57), "tables: Ignite's capsule (Plum Velvet in the Night rail, the coral tube)");
		SitesExpect(L.Vertices.size() <= 22000, "tables: Ignite's table within its vertex budget (22k, §10)");
	}
	// G1 fix (g1-audit M2, M1): its own controls and its own panel (§2.5).
	SitesRivalControls(L, SiteId::Ignite, "Ignite");
	SitesIgnitePanel(P, L);
	SitesRivalGrade(P, SiteId::Ignite, "site_table_ignite_grade");
	// A second table on RiverLine: the tiles wear each its own site's colours, each with its site's mark.
	const ss::net::Network& Home = ss::net::Shared();
	for (const ss::net::EventInstance& X : Home.Window(S.WorldMinutes() - 180.0, S.WorldMinutes() + 60.0))
	{
		const ss::LobbyEvent Lx = Home.Listing(X, nullptr, S.Unlocks());
		const ss::net::LiveState Lv = Home.Live(X, S.WorldMinutes());
		if (Lx.Joinable && Lx.BuyInCents > 0 && Lx.BuyInCents <= 1100 && Lx.Spec.Entrants <= 3000 && (Lv.St == ss::net::Status::LateReg || (Lv.St == ss::net::Status::Registering && Lv.StartsIn <= 60.0)))
		{
			S.RegisterEvent(Lx);
			break;
		}
	}
	if (S.TableCount() < 2)
	{
		std::printf("  sites: no RiverLine event open at this hour; the tile checks are skipped\n");
		return;
	}
	S.Tiled = true;
	S.ShowTables();
	P.Now = Step(S, P.Now, 2.0);
	L = P.Settled();
	WriteList("site_tiles_mixed", L);
	SitesExpect(SitesRun(L, "IGNITE", P.M).Found && SitesRun(L, "RIVERLINE", P.M).Found, "tiles: each carries its site's mark");
	// The Ignite tile is the first (it opened first): no RiverLine colour inside it.
	const float TileW = (ss::ui::RiverLine::Width - 16.0f - 8.0f) / 2.0f;
	SitesNoLeak(L, SiteId::Ignite, "Ignite's tile", {8.0f, 72.0f, TileW, 878.0f});
	// G1 fix (g1-audit M2): a waiting Ignite tile's status line and pace row are Ignite's (BlackItalic caps in candy pills);
	// the RiverLine tile beside it keeps RiverLine's Bold.
	S.WithTable(0, [&]() {
		if (S.HasPrompt)
		{
			S.HeroAct(ss::PlayerAction::Fold());
		}
	});
	const DrawList Lw = P.Settled();
	WriteList("site_tiles_mixed_waiting", Lw);
	const ss::ui::Rect IgTile{8.0f, 72.0f, TileW, 878.0f};
	const ss::ui::Rect RlTile{8.0f + TileW + 8.0f, 72.0f, TileW, 878.0f};
	const bool IgPace = SitesRunIn(Lw, "FULL", ss::ui::Font::BlackItalic, IgTile) && SitesRunIn(Lw, "AUTO", ss::ui::Font::BlackItalic, IgTile);
	const bool IgRiverLine = SitesRunIn(Lw, "Full", ss::ui::Font::Bold, IgTile) || SitesRunIn(Lw, "Smart", ss::ui::Font::Bold, IgTile);
	const bool RlPace = !SitesAnyRunIn(Lw, "AUTO", RlTile) || SitesRunIn(Lw, "Full", ss::ui::Font::Bold, RlTile);
	if (!SitesAnyRunIn(Lw, "AUTO", IgTile))
	{
		std::printf("  sites: the Ignite tile shows no pace row this frame; its check is skipped\n");
	}
	SitesExpect((IgPace || !SitesAnyRunIn(Lw, "AUTO", IgTile)) && !IgRiverLine && RlPace, "tiles: a waiting Ignite tile's pace row is Ignite's, the RiverLine tile's RiverLine's");
	SitesNoLeak(Lw, SiteId::Ignite, "Ignite's tile, waiting", IgTile);
}

// ------------------------------------------------------------------ G1 fix (fix-tablestyle): the rivals' own controls and panels

/** A run that is exactly Text in Face, inside R (its x and its baseline). */
bool SitesRunIn(const DrawList& L, const std::string& Text, ss::ui::Font Face, const ss::ui::Rect& R)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text && Cmd.Text.Face == Face && Cmd.Text.X >= R.X && Cmd.Text.X <= R.X + R.W &&
			Cmd.Text.BaselineY >= R.Y && Cmd.Text.BaselineY <= R.Y + R.H)
		{
			return true;
		}
	}
	return false;
}

/** A run that is exactly Text inside R, in any face. */
bool SitesAnyRunIn(const DrawList& L, const std::string& Text, const ss::ui::Rect& R)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text && Cmd.Text.X >= R.X && Cmd.Text.X <= R.X + R.W && Cmd.Text.BaselineY >= R.Y && Cmd.Text.BaselineY <= R.Y + R.H)
		{
			return true;
		}
	}
	return false;
}

/** How many vertices of colour Rgb (alpha over 0.5) lie inside R. */
int SitesHexIn(const DrawList& L, uint32_t Rgb, const ss::ui::Rect& R)
{
	int N = 0;
	for (const ss::ui::Vertex& V : L.Vertices)
	{
		N += V.Col.A > 0.5f && SitesRgb(V.Col) == Rgb && V.X >= R.X && V.X <= R.X + R.W && V.Y >= R.Y && V.Y <= R.Y + R.H ? 1 : 0;
	}
	return N;
}

/** The left-hand controls' corner of the table (Pace, HUD, Lean back, Composure; RunItTwice's start right of its rail). */
const ss::ui::Rect SitesControlsArea{0.0f, 790.0f, 640.0f, 170.0f};
/** The side panel's column (SidePanel's x 1188 to the client's right edge). */
const ss::ui::Rect SitesPanelArea{1188.0f, 64.0f, 412.0f, 898.0f};

/**
 * g1-audit M2 (§2.5, §3.5, §9.6 "not a reskin: share no chrome"): a rival table's left-hand controls are its own. Every caption
 * and label is in the look's face (Ignite's BlackItalic, RunItTwice's BoldCondensed) and none of RiverLine's runs is there
 * ("Pace", "Composure", "Full", "Smart", "Lean back (Space)" and the HUD's in RiverLine's Bold); the lit pace (FULL) is in the
 * look's light (Ignite's accent pill, RunItTwice's Signal Red slab); the composure meter is the look's (Ignite's Night pill,
 * RunItTwice's 20 cells).
 */
void SitesRivalControls(const DrawList& L, SiteId Site, const char* Who)
{
	namespace sd = ss::ui::rlsites_detail;
	const sd::SiteLook& Look = sd::SitesLook(Site);
	const ss::ui::Font Face = static_cast<ss::ui::Font>(Look.TitleFace);
	const bool Rit = Site == SiteId::RunItTwice;
	bool Own = SitesRunIn(L, "HUD ON", Face, SitesControlsArea) || SitesRunIn(L, "HUD OFF", Face, SitesControlsArea);
	for (const char* W : {"PACE", "COMPOSURE", "FULL", "SMART", "AUTO", "LEAN BACK (SPACE)"})
	{
		Own = Own && SitesRunIn(L, W, Face, SitesControlsArea);
	}
	bool Theirs = false;
	for (const char* W : {"Pace", "Composure", "Full", "Smart", "AUTO", "HUD ON", "HUD OFF", "Lean back (Space)"})
	{
		Theirs = Theirs || SitesRunIn(L, W, ss::ui::Font::Bold, SitesControlsArea) || SitesRunIn(L, W, ss::ui::Font::Black, SitesControlsArea);
	}
	SitesExpect(Own && !Theirs, (std::string("rival controls: ") + Who + "'s Pace, HUD, Lean back and Composure in its own face, none of RiverLine's").c_str());
	const float Lx = Rit ? 56.0f : 0.0f;
	const ss::ui::Rect Full{24.0f + Lx, 874.0f, 92.0f, 36.0f};
	SitesExpect(SitesHexIn(L, Rit ? 0xd2141au : 0xff6b57u, Full) >= 4, (std::string("rival controls: ") + Who + "'s lit pace in its own light (" + (Rit ? "a Signal Red slab" : "the accent pill") + ")").c_str());
	const ss::ui::Rect Meter{24.0f + Lx, 798.0f, 420.0f, 18.0f};
	SitesExpect(Rit ? SitesHexIn(L, 0x48c77au, Meter) >= 80 : SitesHexIn(L, 0x120a18u, Meter) > 0,
		(std::string("rival controls: ") + Who + "'s composure meter is its own (" + (Rit ? "20 square cells" : "a candy pill on Night") + ")").c_str());
}

/**
 * Review fix (g1-audit M2, §9.6 "share no chrome"; §3.3 "red is light, never ink"): a rival table's decision grade is the
 * look's, not RiverLine's pill in Bold. A Good grade: Ignite's pill with "Good" in BlackItalic; RunItTwice's Signal Red slab
 * with "GOOD" in BoldCondensed in Ink (never the dark mix, 2.75:1 on red), and its note under the action area in Chrome, never
 * Signal Red. The badge is the session's own kind, pushed as HeroAct pushes one, and taken off again after the page.
 */
void SitesRivalGrade(SitesLaptop& P, SiteId Site, const char* Page)
{
	namespace sd = ss::ui::rlsites_detail;
	ss::Session& S = P.S;
	const sd::SiteLook& Look = sd::SitesLook(Site);
	const ss::ui::Font Face = static_cast<ss::ui::Font>(Look.TitleFace);
	const bool Rit = Site == SiteId::RunItTwice;
	ss::GradeBadge Bd;
	Bd.G.Result = ss::Grade::Good;
	Bd.G.Note = "Calling keeps the bluffs in.";
	Bd.At = P.Now + 1.15; // Settled draws at Now + 1.55: the badge 0.4 s old, at full alpha
	S.Grades.push_back(Bd.G);
	S.Badges.push_back(Bd);
	const DrawList L = P.Settled();
	WriteList(Page, L);
	const ss::ui::TextItem* Label = nullptr;
	const ss::ui::TextItem* Note = nullptr;
	bool Riverlines = false;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text)
		{
			continue;
		}
		Label = Cmd.Text.Text == (Rit ? "GOOD" : "Good") && Cmd.Text.Face == Face ? &Cmd.Text : Label;
		Note = Cmd.Text.Text == Bd.G.Note ? &Cmd.Text : Note;
		Riverlines = Riverlines || (Cmd.Text.Text == "Good" && Cmd.Text.Face != Face);
	}
	bool Own = Label && !Riverlines;
	if (Own && Rit)
	{
		// The slab behind the label is Signal Red and the label Ink (Frost) on it.
		Own = SitesHexIn(L, 0xd2141au, {Label->X - 40.0f, Label->BaselineY - 26.0f, 110.0f, 36.0f}) >= 4 && SitesRgb(Label->Col) == 0xececefu;
	}
	SitesExpect(Own, (std::string("rival grade: ") + Page + "'s Good grade in the look's own shape and face, readable on its fill").c_str());
	if (Rit && !S.HasPrompt)
	{
		SitesExpect(Note && SitesRgb(Note->Col) == 0xb6bbc5u, "rival grade: RunItTwice's Good note in Chrome, never Signal Red ink");
	}
	else if (Rit)
	{
		std::printf("  sites: the hero is to act on the octagon; the grade note's check is skipped\n");
	}
	for (size_t I = S.Badges.size(); I-- > 0;)
	{
		if (S.Badges[I].At == Bd.At && S.Badges[I].G.Note == Bd.G.Note)
		{
			S.Badges.erase(S.Badges.begin() + static_cast<std::ptrdiff_t>(I));
			break;
		}
	}
	if (!S.Grades.empty() && S.Grades.back().Note == Bd.G.Note)
	{
		S.Grades.pop_back();
	}
}

/**
 * g1-audit M1 (§2.5): Ignite's panel. Its head (the event, "BLINDS UP IN" over the fuse), candy tabs in BlackItalic, chat in
 * bubbles (the welcome in the grape room bubble), Payouts with medals (sunflower, silver, bronze) and Standings with level rings
 * (the ring's track; the hero's in the accent); none of RiverLine's card ("Players left", "Min cash / 1st"); each within 22k.
 */
void SitesIgnitePanel(SitesLaptop& P, const DrawList& L)
{
	using ss::ui::Font;
	ss::Session& S = P.S;
	const ss::ui::Rect Body{1188.0f, 300.0f, 412.0f, 660.0f};
	bool Tabs = true;
	for (const char* W : {"CHAT", "PAYOUTS", "STANDINGS"})
	{
		Tabs = Tabs && SitesRunIn(L, W, Font::BlackItalic, SitesPanelArea);
	}
	bool Clock = false;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		Clock = Clock || (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.rfind("BLINDS UP IN ", 0) == 0 && Cmd.Text.X >= 1188.0f);
	}
	const bool Card = SitesAnyRunIn(L, "Players left", SitesPanelArea) || SitesAnyRunIn(L, "Min cash / 1st", SitesPanelArea) || SitesAnyRunIn(L, "Standings", SitesPanelArea);
	SitesExpect(Tabs && Clock && !Card, "ignite panel: its head with the fuse clock and candy tabs in BlackItalic, not RiverLine's card and tabs");
	SitesExpect(S.Tab == ss::RightTab::Chat && SitesHexIn(L, 0x3a1f5cu, Body) > 0, "ignite panel: chat in bubbles (the room's welcome in its grape bubble)");
	S.Tab = ss::RightTab::Payouts;
	const DrawList Lp = P.Settled();
	WriteList("site_table_ignite_payouts", Lp);
	SitesExpect(SitesHexIn(Lp, 0xffc94au, Body) > 0 && SitesHexIn(Lp, 0xdcd3ecu, Body) > 0 && SitesHexIn(Lp, 0xe0915au, Body) > 0 && Lp.Vertices.size() <= 22000,
		"ignite panel: Payouts with medals for 1st, 2nd and 3rd (sunflower, silver, bronze), within 22k");
	S.Tab = ss::RightTab::Info;
	const DrawList Ls = P.Settled();
	WriteList("site_table_ignite_standings", Ls);
	SitesExpect(SitesHexIn(Ls, 0x3a2240u, Body) > 0 && SitesHexIn(Ls, 0xff6b57u, Body) > 0 && SitesRunIn(Ls, "STANDINGS", Font::BlackItalic, SitesPanelArea) && Ls.Vertices.size() <= 22000,
		"ignite panel: Standings with level rings (the track, the hero's in the accent), within 22k");
	std::printf("  Ignite's panel: chat %zu, payouts %zu, standings %zu vertices (budget 22k)\n", L.Vertices.size(), Lp.Vertices.size(), Ls.Vertices.size());
	S.Tab = ss::RightTab::Chat;
	SitesNoLeak(Lp, SiteId::Ignite, "Ignite's table, Payouts");
	SitesNoLeak(Ls, SiteId::Ignite, "Ignite's table, Standings");
}

/** The value run beside a RunItTwice INFO label (the label in BoldCondensed caps, its value right-aligned on its baseline). */
std::string SitesInfoValue(const DrawList& L, const std::string& Label, ss::ui::Font* Face = nullptr, ss::ui::Color* Col = nullptr)
{
	float Base = -1.0f;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Label && Cmd.Text.Face == ss::ui::Font::BoldCondensed && Cmd.Text.X >= 1188.0f)
		{
			Base = Cmd.Text.BaselineY;
		}
	}
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Base > 0.0f && Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.BaselineY == Base && Cmd.Text.X >= 1300.0f && Cmd.Text.Text != Label)
		{
			if (Face)
			{
				*Face = Cmd.Text.Face;
			}
			if (Col)
			{
				*Col = Cmd.Text.Col;
			}
			return Cmd.Text.Text;
		}
	}
	return std::string();
}

/** Two draw colours are the same token (fix2-icm: CHIP SHARE in Ink, the ICM VALUE in Gold). */
bool SitesSameCol(const ss::ui::Color& A, const ss::ui::Color& B)
{
	return A.R == B.R && A.G == B.G && A.B == B.B && A.A == B.A;
}

/** Icm()'s fallback test, as Structure.cpp makes it: more than two million finishing orders for the places left. */
bool SitesIcmBeyond(size_t Players, size_t Prizes)
{
	double Orders = 1.0;
	for (size_t K = 0; K < std::min(Players, Prizes); ++K)
	{
		Orders *= static_cast<double>(Players - K);
	}
	return Orders > 2000000.0;
}

/** INFO's ICM row label for a tournament as it stands: CHIP SHARE where Icm() falls back (g1-audit N7), else ICM VALUE. */
std::string SitesIcmLabel(const ss::Tournament& T)
{
	const size_t Left = T.AlivePlayers().size();
	return SitesIcmBeyond(Left, std::min(Left, T.Payouts.size())) ? "CHIP SHARE" : "ICM VALUE";
}

/** The engine's ICM for the hero over a tournament's players still in and the prizes they play for (Structure's Icm()). */
double SitesEngineIcm(const ss::Tournament& T)
{
	std::vector<double> Stacks;
	std::vector<double> Prizes;
	size_t Mine = 0;
	for (const ss::TPlayer* Pl : T.AlivePlayers())
	{
		Mine = Pl->IsHero ? Stacks.size() : Mine;
		Stacks.push_back(static_cast<double>(Pl->Stack));
	}
	for (size_t I = 0; I < Stacks.size() && I < T.Payouts.size(); ++I)
	{
		Prizes.push_back(static_cast<double>(T.Payouts[I]));
	}
	const std::vector<double> Eq = ss::Icm(Stacks, Prizes);
	return Mine < Eq.size() ? Eq[Mine] : 0.0;
}

/**
 * g1-audit M1 (§3.5): RunItTwice's panel. Tabs INFO, PAYOUTS, CHAT in BoldCondensed caps with the active one twin-striped;
 * chat in Mono; INFO with the ICM value; none of RiverLine's card or tabs; within 19k.
 */
void SitesRitPanel(SitesLaptop& P, const DrawList& L)
{
	using ss::ui::Font;
	ss::Session& S = P.S;
	bool Tabs = true;
	for (const char* W : {"INFO", "PAYOUTS", "CHAT"})
	{
		Tabs = Tabs && SitesRunIn(L, W, Font::BoldCondensed, SitesPanelArea);
	}
	const bool Card = SitesAnyRunIn(L, "Players left", SitesPanelArea) || SitesAnyRunIn(L, "Standings", SitesPanelArea) || SitesAnyRunIn(L, "Chat", SitesPanelArea);
	// The active tab (CHAT, the session's default) carries the panel's twin stripe: Signal Red under its label.
	const float TabW = (ss::ui::RiverLine::Width - 1188.0f - 16.0f) / 3.0f;
	const ss::ui::Rect ChatTab{1188.0f + 2.0f * TabW, 120.0f, TabW, 18.0f};
	SitesExpect(Tabs && !Card && S.Tab == ss::RightTab::Chat && SitesHexIn(L, 0xd2141au, ChatTab) >= 8, "rit panel: INFO, PAYOUTS, CHAT in BoldCondensed caps, the active tab twin-striped, not RiverLine's card and tabs");
	int Chat = 0;
	int NotMono = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.X >= 1200.0f && Cmd.Text.BaselineY > 150.0f && Cmd.Text.BaselineY < 950.0f)
		{
			++Chat;
			NotMono += Cmd.Text.Face == Font::Mono ? 0 : 1;
		}
	}
	SitesExpect(Chat > 0 && NotMono == 0, "rit panel: the chat in Mono, compact");
	S.Tab = ss::RightTab::Info;
	const int Before = ss::ui::rlsites_detail::SitesIcmComputes();
	const DrawList Li = P.Settled();
	WriteList("site_table_runittwice_info", Li);
	Font Vf = Font::Regular;
	// The live field is far beyond Icm()'s exact range, so the engine's figure is its chip share and the row says so (N7).
	const std::string Label = SitesIcmLabel(*S.T);
	const std::string Icm = SitesInfoValue(Li, Label, &Vf);
	const std::string Want = ss::Money(static_cast<ss::Chips>(ss::JsRound(SitesEngineIcm(*S.T))));
	std::printf("  RunItTwice's INFO: %s %s (the engine's %s), %zu vertices\n", Label.c_str(), Icm.c_str(), Want.c_str(), Li.Vertices.size());
	SitesExpect(!Icm.empty() && Icm == Want && Vf == Font::Mono && SitesRunIn(Li, "CHIP LEADERS", Font::BoldCondensed, SitesPanelArea) && !SitesInfoValue(Li, "NEXT LEVEL").empty() &&
			!SitesInfoValue(Li, "AVERAGE").empty() && Li.Vertices.size() <= 19000,
		"rit panel: INFO's ICM row is the engine's Icm() for the field (Mono; CHIP SHARE where it falls back), with the level, the average and the chip leaders, within 19k");
	SitesExpect(ss::ui::rlsites_detail::SitesIcmComputes() == Before + 1, "rit panel: INFO computes the ICM once for the stacks it shows (cached)");
	SitesNoLeak(Li, SiteId::RunItTwice, "RunItTwice's table, INFO");
	S.Tab = ss::RightTab::Payouts;
	const DrawList Lp = P.Settled();
	SitesExpect(SitesRunIn(Lp, "PLACE", Font::BoldCondensed, SitesPanelArea) && SitesRunIn(Lp, "PRIZE", Font::BoldCondensed, SitesPanelArea) && SitesAnyRunIn(Lp, "1st", SitesPanelArea),
		"rit panel: PAYOUTS in Mono under BoldCondensed heads");
	SitesNoLeak(Lp, SiteId::RunItTwice, "RunItTwice's table, PAYOUTS");
	S.Tab = ss::RightTab::Chat;
}

/** The ICM checks' numbers (xorshift): the same every run. */
uint64_t SitesIcmRnd(uint64_t& Seed)
{
	Seed ^= Seed << 13;
	Seed ^= Seed >> 7;
	Seed ^= Seed << 17;
	return Seed;
}

/** Stacks for the ICM checks. Kind 0: spread; 1: many level; 2: one chip among big stacks; 3: one stack at 0 (Icm() skips it). */
std::vector<double> SitesIcmStacks(int Count, int Kind, uint64_t& Seed)
{
	std::vector<double> Out;
	for (int I = 0; I < Count; ++I)
	{
		const double Spread = static_cast<double>(1 + SitesIcmRnd(Seed) % 100000);
		const double Level = 1000.0 * static_cast<double>(1 + SitesIcmRnd(Seed) % 4);
		Out.push_back(Kind == 1 ? Level : Kind == 2 ? (I == Count / 2 ? 1.0 : Spread * 40.0) : Kind == 3 && I == 1 && Count > 2 ? 0.0 : Spread);
	}
	return Out;
}

/** A falling payout ladder of Count places, in cents. */
std::vector<double> SitesIcmLadder(int Count)
{
	std::vector<double> Out;
	for (int I = 0; I < Count; ++I)
	{
		Out.push_back(std::floor(500000.0 / (1.0 + 0.7 * static_cast<double>(I))));
	}
	return Out;
}

/**
 * g1-audit N2 (fix2-icm): INFO walks the hero's share of Icm() (rlsites_detail::SitesIcmWalk) instead of the whole table. It must
 * be Icm()'s number to the bit: every count of places up to nine left (and more places than players), the hardest exact cases
 * beyond (10 paying 8, 11 paying 7, 13 paying 6, 20 paying 5), every player in the hero's seat, level, tiny and empty stacks;
 * cut into odd slices it stays the same; past Icm()'s exact range it is Icm()'s own chip share, flagged.
 */
void SitesIcmExact()
{
	namespace sd = ss::ui::rlsites_detail;
	uint64_t Seed = 0x9e3779b97f4a7c15ull;
	int Exact = 0;
	int ExactOff = 0;
	int Share = 0;
	int ShareOff = 0;
	int Sliced = 0;
	int SliceOff = 0;
	struct IcmShape
	{
		int Left;
		int Places;
		int Sets;
	};
	std::vector<IcmShape> Shapes;
	for (int N = 1; N <= 9; ++N)
	{
		for (int Pl = 0; Pl <= N + 1; ++Pl)
		{
			Shapes.push_back({N, Pl, N >= 9 && Pl >= 8 ? 1 : N >= 8 && Pl >= 7 ? 2 : 4});
		}
	}
	for (const IcmShape& Big : {IcmShape{10, 8, 1}, IcmShape{11, 7, 1}, IcmShape{13, 6, 1}, IcmShape{20, 5, 1}, IcmShape{10, 9, 2}, IcmShape{12, 7, 2}, IcmShape{40, 10, 1}, IcmShape{180, 27, 1}})
	{
		Shapes.push_back(Big);
	}
	for (const IcmShape& Sh : Shapes)
	{
		const bool Beyond = SitesIcmBeyond(static_cast<size_t>(Sh.Left), static_cast<size_t>(Sh.Places));
		for (int Set = 0; Set < Sh.Sets; ++Set)
		{
			const std::vector<double> Stacks = SitesIcmStacks(Sh.Left, (Set + Sh.Left) % 4, Seed);
			const std::vector<double> Prizes = SitesIcmLadder(Sh.Places);
			const std::vector<double> Eq = ss::Icm(Stacks, Prizes);
			// Every seat at the small tables; three at the big ones (each walk there is a millisecond or two).
			const bool Few = Sh.Left > 9;
			for (int M = 0; M < Sh.Left; M += Few ? std::max(1, Sh.Left / 3) : 1)
			{
				bool Cs = false;
				const double Got = sd::SitesIcmOne(Stacks, Prizes, static_cast<size_t>(M), &Cs);
				const bool Bits = std::memcmp(&Got, &Eq[static_cast<size_t>(M)], sizeof(double)) == 0;
				if (Beyond)
				{
					++Share;
					ShareOff += Bits && Cs ? 0 : 1;
				}
				else
				{
					++Exact;
					ExactOff += Bits && !Cs ? 0 : 1;
				}
				if (!Beyond && M == 0 && Sh.Left >= 5)
				{
					// The same walk in slices of 1 to 5,000 steps.
					sd::SitesIcmWalk W;
					sd::SitesIcmStart(W, Stacks, Prizes, 0);
					while (!sd::SitesIcmRun(W, 1 + static_cast<int64_t>(SitesIcmRnd(Seed) % 5000)))
					{
					}
					++Sliced;
					SliceOff += std::memcmp(&W.Sum, &Got, sizeof(double)) == 0 ? 0 : 1;
				}
			}
		}
	}
	std::printf("  RunItTwice's INFO walk vs Icm(): %d exact values (%d differ), %d sliced (%d differ), %d chip shares (%d differ)\n", Exact, ExactOff, Sliced, SliceOff, Share,
		ShareOff);
	SitesExpect(Exact > 400 && ExactOff == 0, "rit icm: the hero's walk is Icm()'s exact value to the bit (up to 9 left, every places count; 10/8, 11/7, 13/6, 20/5)");
	SitesExpect(Sliced > 20 && SliceOff == 0, "rit icm: walked in slices of any size, the value doesn't move a bit");
	SitesExpect(Share > 0 && ShareOff == 0, "rit icm: past two million finishing orders it is Icm()'s own chip share, flagged as such");
}

/**
 * g1-audit N7 (fix2-icm): players past the 32nd. Icm()'s exact path marks the placed players in a 32-bit mask, so at 40 left paying
 * 3 (59,280 orders, inside its range) player 33 is taken for player 1 and the shares don't add up (the engine patch
 * fix2-icm_to_lead_1 fixes it). The walk marks players one by one: the shares add up to the prizes, and equal stacks get equal
 * shares wherever they sit.
 */
void SitesIcmPast32()
{
	namespace sd = ss::ui::rlsites_detail;
	std::vector<double> Stacks;
	for (int I = 0; I < 40; ++I)
	{
		Stacks.push_back(1000.0 + 500.0 * static_cast<double>(I % 8)); // seat I and seat I + 32 hold the same stack
	}
	const std::vector<double> Prizes = {300000.0, 180000.0, 120000.0};
	const std::vector<double> Eq = ss::Icm(Stacks, Prizes);
	std::vector<double> Got;
	double Sum = 0.0;
	double Engine = 0.0;
	for (size_t M = 0; M < Stacks.size(); ++M)
	{
		Got.push_back(sd::SitesIcmOne(Stacks, Prizes, M));
		Sum += Got.back();
		Engine += Eq[M];
	}
	bool Equal = true;
	for (size_t M = 0; M + 32 < Stacks.size(); ++M)
	{
		Equal = Equal && std::fabs(Got[M] - Got[M + 32]) <= 1e-6 * Got[M] && std::fabs(Got[M] - Got[M + 8]) <= 1e-6 * Got[M];
	}
	std::printf("  40 left paying 3: the walk shares out %.4f of 600000.0000 cents; Icm() today %.4f (N7, its 32-bit mask)\n", Sum, Engine);
	SitesExpect(std::fabs(Sum - 600000.0) < 1e-4 && Equal, "rit icm: past the 32nd player the walk keeps everyone apart (the shares add up; equal stacks, equal shares)");
}

/** The tournament with the hero and Stacks.size() - 1 others still in (the hero's stack first), everyone else out. */
void SitesIcmField(ss::Tournament& T, const std::vector<ss::Chips>& Stacks)
{
	size_t Kept = 1;
	for (ss::TPlayer& Pl : T.Players)
	{
		if (Pl.IsHero)
		{
			Pl.Stack = Stacks[0];
			Pl.Busted = false;
		}
		else if (Kept < Stacks.size())
		{
			Pl.Stack = Stacks[Kept++];
			Pl.Busted = false;
		}
		else
		{
			Pl.Busted = true;
		}
	}
	T.Remaining = static_cast<int>(Stacks.size());
}

/** Icm()'s arguments for the hero, as the INFO tab takes them (the stacks still in; one prize a player left). */
size_t SitesIcmArgs(const ss::Tournament& T, std::vector<double>& Stacks, std::vector<double>& Prizes)
{
	size_t Mine = 0;
	Stacks.clear();
	Prizes.clear();
	for (const ss::TPlayer* Pl : T.AlivePlayers())
	{
		Mine = Pl->IsHero ? Stacks.size() : Mine;
		Stacks.push_back(static_cast<double>(Pl->Stack));
	}
	for (size_t I = 0; I < Stacks.size() && I < T.Payouts.size(); ++I)
	{
		Prizes.push_back(static_cast<double>(T.Payouts[I]));
	}
	return Mine;
}

/**
 * g1-audit N2, N7 (fix2-icm): INFO at 9 to 20 left, through the laptop. For each count left its hardest exact case (the most steps:
 * 9 paying 9, 10 paying 8, 11 paying 7, 12-13 paying 6, 14-20 paying 5) and the next places count up (past Icm()'s range, but
 * 9 left), and 20 left paying 20. The row is the engine's to the cent, ICM VALUE in Gold or CHIP SHARE in Ink; a dash shows only
 * while an event's first walk is under way, and a new hand's walk keeps the last value up; one walk a change; no frame walks
 * more than SitesIcmFrameSteps (plus one place's last players). The slices' and frames' times are printed (the PC's load moves
 * them; the pass marks are the steps).
 */
void SitesIcmFrames(SitesLaptop& P)
{
	namespace sd = ss::ui::rlsites_detail;
	using Clock = std::chrono::steady_clock;
	ss::Tournament& T = *P.S.T;
	const std::vector<ss::Chips> Paid = T.Payouts;
	struct IcmCase
	{
		int Left;
		int Places;
	};
	const IcmCase Cases[] = {{9, 9}, {10, 8}, {10, 9}, {11, 7}, {11, 8}, {12, 6}, {12, 7}, {13, 6}, {13, 7}, {14, 5}, {14, 6}, {15, 5}, {16, 5}, {17, 5}, {18, 5}, {19, 5},
		{20, 5}, {20, 6}, {20, 20}};
	bool Same = true;
	bool Rows = true;
	bool Dash = true;
	bool Bounded = true;
	bool Once = true;
	bool Stale = false;
	bool OnePlayer = false;
	double WorstSlice = 0.0;
	double WorstFrame = 0.0;
	double WorstRest = 0.0;
	int MostSlices = 0;
	std::string Line;
	for (const IcmCase& K : Cases)
	{
		// The field's chips (entrants x the starting stack) shared out unevenly among the players left, the hero mid-pack.
		std::vector<ss::Chips> St;
		const ss::Chips Field = static_cast<ss::Chips>(T.Spec.Entrants) * T.Spec.StartingStack;
		ss::Chips Weights = 0;
		for (int I = 0; I < K.Left; ++I)
		{
			Weights += 2 + (I * 7 + 3) % K.Left;
		}
		for (int I = 0; I < K.Left; ++I)
		{
			St.push_back(Field * (2 + (I * 7 + 3) % K.Left) / Weights);
		}
		SitesIcmField(T, St);
		T.Payouts.clear();
		for (int I = 0; I < K.Places; ++I)
		{
			T.Payouts.push_back(static_cast<ss::Chips>(400000 / (I + 2)));
		}
		std::vector<double> Stacks;
		std::vector<double> Prizes;
		const size_t Mine = SitesIcmArgs(T, Stacks, Prizes);
		const std::vector<double> Eq = ss::Icm(Stacks, Prizes);
		const bool Beyond = SitesIcmBeyond(Stacks.size(), Prizes.size());
		// The cache's walk by hand, slice by slice at its budget: the steps and the time of each.
		sd::SitesIcmWalk W;
		sd::SitesIcmStart(W, Stacks, Prizes, Mine);
		int Slices = 0;
		for (bool Done = W.Done; !Done;)
		{
			int64_t Steps = 0;
			const Clock::time_point T0 = Clock::now();
			Done = sd::SitesIcmRun(W, sd::SitesIcmFrameSteps, &Steps);
			WorstSlice = std::max(WorstSlice, std::chrono::duration<double, std::milli>(Clock::now() - T0).count());
			Bounded = Bounded && Steps <= sd::SitesIcmFrameSteps + 3 * K.Left;
			++Slices;
		}
		MostSlices = std::max(MostSlices, Slices);
		Same = Same && std::memcmp(&W.Sum, &Eq[Mine], sizeof(double)) == 0 && W.ChipShare == Beyond;
		// Through the laptop with the event's memo cleared (INFO's first look): a dash until the walk is done.
		sd::SitesOf(&P.RL).Icm.clear();
		const int Before = sd::SitesIcmComputes();
		const std::string Label = Beyond ? "CHIP SHARE" : "ICM VALUE";
		const std::string Want = ss::Money(static_cast<ss::Chips>(ss::JsRound(Eq[Mine])));
		P.Now += 1.0;
		int Frames = 0;
		std::string Shown;
		ss::ui::Color Col;
		DrawList L;
		while (Frames < 12)
		{
			const Clock::time_point T0 = Clock::now();
			L = P.Frame();
			WorstFrame = std::max(WorstFrame, std::chrono::duration<double, std::milli>(Clock::now() - T0).count());
			++Frames;
			Shown = SitesInfoValue(L, Label, nullptr, &Col);
			if (Frames == 1 && K.Left == 10 && K.Places == 8)
			{
				WriteList("site_table_runittwice_icm_walk", L); // ten left paying eight, INFO's first frame: the walk under way
			}
			if (Shown != "\xE2\x80\x94")
			{
				break;
			}
			P.Now += 1.0 / 30.0;
		}
		if (K.Left == 10 && K.Places == 9)
		{
			WriteList("site_table_runittwice_icm_share", L); // ten left paying nine: past Icm()'s range, CHIP SHARE
			OnePlayer = SitesInfoValue(L, "TO THE MONEY").rfind("1 player \xC2\xB7 min ", 0) == 0;
		}
		Dash = Dash && Frames == std::max(1, Slices);
		ss::ui::Color Ink;
		ss::ui::Color Gold;
		SitesInfoValue(L, "NEXT LEVEL", nullptr, &Ink);
		SitesInfoValue(L, "FIRST", nullptr, &Gold);
		Rows = Rows && Shown == Want && SitesSameCol(Col, Beyond ? Ink : Gold) && SitesInfoValue(L, Beyond ? "ICM VALUE" : "CHIP SHARE").empty();
		Once = Once && sd::SitesIcmComputes() == Before + 1;
		{
			P.Now += 1.0 / 30.0;
			const Clock::time_point T0 = Clock::now();
			P.Frame();
			WorstRest = std::max(WorstRest, std::chrono::duration<double, std::milli>(Clock::now() - T0).count());
		}
		if (K.Left == 10 && K.Places == 8)
		{
			// A hand later (one stack moved): the new walk takes frames, and the last value stands until it is done.
			for (ss::TPlayer& Pl : T.Players)
			{
				if (!Pl.IsHero && !Pl.Busted)
				{
					Pl.Stack += 500;
					break;
				}
			}
			SitesIcmArgs(T, Stacks, Prizes);
			const std::string Next = ss::Money(static_cast<ss::Chips>(ss::JsRound(ss::Icm(Stacks, Prizes)[Mine])));
			P.Now += 1.0;
			const std::string During = SitesInfoValue(P.Frame(), "ICM VALUE");
			std::string Later = During;
			for (int I = 0; I < 12 && Later == During; ++I)
			{
				P.Now += 1.0 / 30.0;
				Later = SitesInfoValue(P.Frame(), "ICM VALUE");
			}
			Stale = During == Want && Next != Want && Later == Next && sd::SitesIcmComputes() == Before + 2;
		}
		Line += " " + std::to_string(K.Left) + "/" + std::to_string(K.Places) + (Beyond ? " share" : " " + std::to_string(Slices) + "f") + ";";
	}
	T.Payouts = Paid;
	std::printf("  RunItTwice's INFO at 9-20 left (left/paid, frames to the value):%s\n", Line.c_str());
	std::printf("  RunItTwice's INFO: worst walk slice %.3f ms (budget %lld steps, at most %d frames); laptop frame %.2f ms walking, %.2f ms at rest\n", WorstSlice,
		static_cast<long long>(sd::SitesIcmFrameSteps), MostSlices, WorstFrame, WorstRest);
	SitesExpect(Same && Bounded && MostSlices <= 4,
		"rit icm: at 9-20 left the walk is Icm()'s value to the bit, no frame walks more than its budget, and the slowest case is done in 4 frames");
	SitesExpect(Rows && Dash && Once, "rit icm: INFO shows the engine's value (ICM VALUE in Gold, CHIP SHARE in Ink past Icm()'s range), a dash only while the first walk runs, one walk a change");
	SitesExpect(Stale, "rit icm: a hand later the last value stands until the new walk is done, then the new one shows");
	SitesExpect(OnePlayer, "rit info: one off the money reads \"1 player\", not \"1 players\"");
}

/** g1-audit N2 (fix2-icm): no walk while a tick's tables are still being played (the stacks half old); one once it has ended. */
void SitesIcmFinishGate(SitesLaptop& P)
{
	namespace sd = ss::ui::rlsites_detail;
	ss::TournamentSpec Spec = P.S.T->Spec;
	Spec.Entrants = 27;
	ss::Tournament X(Spec, "Gate", "fix2-icm-gate");
	sd::SitesOf(&P.RL).Icm.clear();
	const int Before = sd::SitesIcmComputes();
	std::vector<ss::TEvent> Events;
	std::unique_ptr<ss::Hand> H = X.StartTick(Events);
	X.BeginFinish(H.get());
	const sd::SitesIcmRead Mid = sd::SitesIcmCached(P.RL, X, P.Now + 10.0);
	const bool Held = X.FinishPending() && !Mid.Known && sd::SitesIcmComputes() == Before;
	while (!X.FinishSome(4))
	{
	}
	X.EndFinish();
	sd::SitesIcmRead After;
	for (int I = 0; I < 12 && !After.Known; ++I)
	{
		After = sd::SitesIcmCached(P.RL, X, P.Now + 10.1 + static_cast<double>(I) / 30.0);
	}
	const double Want = SitesEngineIcm(X);
	SitesExpect(Held && After.Known && sd::SitesIcmComputes() == Before + 1 && std::memcmp(&After.Cents, &Want, sizeof(double)) == 0,
		"rit icm: no walk while a tick's tables are still being played; one once the hand has ended everywhere, the engine's value");
	sd::SitesOf(&P.RL).Icm.clear();
}

/**
 * review-fix2-icm: INFO left mid-walk (PAYOUTS opened a frame after a hand ended) and opened again a hand later. The walk left
 * behind finishes on its old stacks, and the current stacks' walk starts the frame after (the twice-a-second clock runs from a
 * walk's start), so the current value shows within two walks' frames (10 left paying 8: 7), not after half a second of an old
 * hand's value (22 frames before the fix).
 */
void SitesIcmTabReturn(SitesLaptop& P)
{
	namespace sd = ss::ui::rlsites_detail;
	ss::Session& S = P.S;
	ss::Tournament& T = *S.T;
	const std::vector<ss::Chips> Paid = T.Payouts;
	const auto Hand = [&T](ss::Chips Moved) {
		std::vector<ss::Chips> St;
		for (int I = 0; I < 10; ++I)
		{
			St.push_back(static_cast<ss::Chips>(200000 + 40000 * ((I * 7 + 3) % 10)));
		}
		St[1] += Moved;
		St[2] -= Moved;
		SitesIcmField(T, St);
		return ss::Money(static_cast<ss::Chips>(ss::JsRound(SitesEngineIcm(T))));
	};
	T.Payouts.clear();
	for (int I = 0; I < 8; ++I)
	{
		T.Payouts.push_back(static_cast<ss::Chips>(400000 / (I + 2)));
	}
	S.Tab = ss::RightTab::Info;
	sd::SitesOf(&P.RL).Icm.clear();
	const std::string First = Hand(0);
	P.Now += 1.0;
	std::string Shown;
	for (int I = 0; I < 12 && Shown != First; ++I)
	{
		P.Now += 1.0 / 30.0;
		Shown = SitesInfoValue(P.Frame(), "ICM VALUE");
	}
	const std::string Left = Hand(3000);
	P.Now += 1.0;
	P.Frame(); // the walk for Left starts
	S.Tab = ss::RightTab::Payouts;
	P.Now += 1.0;
	P.Frame();
	const std::string Want = Hand(6000);
	S.Tab = ss::RightTab::Info;
	int Frames = 0;
	bool Got = false;
	while (Frames < 30 && !Got)
	{
		P.Now += 1.0 / 30.0;
		++Frames;
		Got = SitesInfoValue(P.Frame(), "ICM VALUE") == Want;
	}
	std::printf("  RunItTwice's INFO left mid-walk and opened a hand later: the current value in %d frames (10 left paying 8)\n", Frames);
	SitesExpect(Shown == First && First != Left && Left != Want && Want != First && Got && Frames <= 8,
		"rit icm: INFO left mid-walk and opened again a hand later shows the current value within two walks, not an old hand's for half a second");
	T.Payouts = Paid;
	sd::SitesOf(&P.RL).Icm.clear();
}

/**
 * The INFO tab's ICM value for a known stack set (§3.5, §10): six players left (the exact Icm(): 720 finishing orders), then two
 * level stacks (each worth half of 1st and 2nd, whatever the model); computed when the stacks change, never per frame.
 */
void SitesRitIcm()
{
	namespace sd = ss::ui::rlsites_detail;
	SitesLaptop P("sites-rit-icm");
	ss::Session& S = P.S;
	P.At(SitesEvening);
	S.BankrollCents = 200000;
	P.Join(SiteId::RunItTwice, 50000);
	ss::net::EventInstance E;
	if (!SitesOpenEvent(S, SiteId::RunItTwice, E, 2200))
	{
		std::printf("  sites: no RunItTwice event open at this hour; the ICM checks are skipped\n");
		return;
	}
	S.RegisterEvent(ss::net::Site(SiteId::RunItTwice).Listing(E, nullptr, S.Unlocks()));
	S.CurrentPace = ss::Pace::Full;
	P.Now = Step(S, P.Now, 3.0);
	if (!S.T || S.T->Payouts.size() < 6)
	{
		std::printf("  sites: the RunItTwice event pays under 6 places; the ICM checks are skipped\n");
		return;
	}
	S.Tab = ss::RightTab::Info;
	ss::Tournament& T = *S.T;
	// Six left: the hero and five others, the rest out.
	const ss::Chips Known[6] = {31000, 22000, 15000, 12000, 9000, 6000};
	int Kept = 0;
	for (ss::TPlayer& Pl : T.Players)
	{
		if (Pl.IsHero)
		{
			Pl.Stack = Known[0];
			Pl.Busted = false;
		}
		else if (!Pl.Busted && Kept < 5)
		{
			Pl.Stack = Known[1 + Kept++];
		}
		else
		{
			Pl.Busted = true;
		}
	}
	T.Remaining = 6;
	const double Six = SitesEngineIcm(T);
	const int Before = sd::SitesIcmComputes();
	DrawList L = P.Settled();
	WriteList("site_table_runittwice_icm", L);
	const std::string Shown = SitesInfoValue(L, "ICM VALUE");
	std::printf("  RunItTwice's INFO, six left: ICM value %s (the engine's %.2f cents)\n", Shown.c_str(), Six);
	SitesExpect(Kept == 5 && std::fabs(sd::SitesIcmCents(T) - Six) < 1e-9 && Shown == ss::Money(static_cast<ss::Chips>(ss::JsRound(Six))) && Six > 0.0,
		"rit icm: six left, INFO's ICM value is the engine's exact Icm() for the hero's stack");
	const int After = sd::SitesIcmComputes();
	for (int I = 0; I < 6; ++I)
	{
		P.Now += 0.1;
		P.Frame();
	}
	SitesExpect(After == Before + 1 && sd::SitesIcmComputes() == After, "rit icm: computed once for the stacks, never per frame");
	// Two left with level stacks: each is worth half of what 1st and 2nd pay, whatever the model.
	ss::TPlayer* Other = nullptr;
	for (ss::TPlayer& Pl : T.Players)
	{
		if (!Pl.IsHero && !Pl.Busted)
		{
			if (Other)
			{
				Pl.Busted = true;
			}
			else
			{
				Other = &Pl;
			}
		}
	}
	for (ss::TPlayer& Pl : T.Players)
	{
		Pl.Stack = Pl.IsHero || &Pl == Other ? 40000 : Pl.Stack;
	}
	T.Remaining = 2;
	P.Now += 1.0;
	L = P.Frame();
	const double Half = static_cast<double>(T.Payouts[0] + T.Payouts[1]) / 2.0;
	SitesExpect(Other && sd::SitesIcmComputes() == After + 1 && std::fabs(sd::SitesIcmCents(T) - Half) < 0.01 && SitesInfoValue(L, "ICM VALUE") == ss::Money(static_cast<ss::Chips>(ss::JsRound(Half))),
		"rit icm: when the stacks change it computes again: two level stacks are each worth half of 1st and 2nd");
	// What one compute costs at its worst (a nine-handed final table paying nine: Icm()'s exact 9! orders), the hitch the
	// cache keeps to once a change (printed for the lead, not a pass mark: the PC's load moves it).
	{
		const std::vector<double> Nine = {31000.0, 22000.0, 15000.0, 12000.0, 9000.0, 6000.0, 4000.0, 3000.0, 1000.0};
		const std::vector<double> Paid = {30000.0, 18000.0, 12000.0, 9000.0, 7000.0, 5500.0, 4500.0, 3500.0, 2500.0};
		const auto T0 = std::chrono::steady_clock::now();
		const std::vector<double> Eq = ss::Icm(Nine, Paid);
		const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count();
		double Sum = 0.0;
		for (const double V : Eq)
		{
			Sum += V;
		}
		std::printf("  RunItTwice's INFO: Icm() for the whole table at nine left paying nine: %.2f ms (INFO walks the hero's share instead)\n", Ms);
		SitesExpect(std::fabs(Sum - 92000.0) < 0.5, "rit icm: the nine-handed exact ICM shares out the prizes left");
	}
	// g1-audit N2, N7 (fix2-icm): the hero's walk against Icm() bit for bit, past the 32nd player, INFO frame by frame at 9-20
	// left, and no walk while a tick is mid-finish.
	const std::chrono::steady_clock::time_point Checks = std::chrono::steady_clock::now();
	SitesIcmExact();
	SitesIcmPast32();
	SitesIcmFrames(P);
	SitesIcmFinishGate(P);
	SitesIcmTabReturn(P);
	std::printf("  RunItTwice's INFO walk checks: %.0f ms of the suite\n", std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - Checks).count());
}

/**
 * RiverLine's table is RiverLine's (§9.6; the JSON proof is the re-baselined day 0): its own controls and card at their places
 * in its Bold ("Pace" at 24/866, "Composure" at 24/812, "Players left", the Chat, Payouts and Standings tabs) and nothing of a
 * rival's; and its HUD is the session's switch (g1-audit L10: only a look with HudDefault, RunItTwice's, is on whatever the
 * switch says, and its own toggle leaves Session::Hud alone).
 */
void SitesRiverLineTable()
{
	namespace sd = ss::ui::rlsites_detail;
	using ss::ui::Font;
	SitesLaptop P("sites-riverline-table");
	ss::Session& S = P.S;
	P.At(SitesEvening);
	S.BankrollCents = 200000;
	const ss::net::Network& Home = ss::net::Shared();
	for (const ss::net::EventInstance& X : Home.Window(S.WorldMinutes() - 180.0, S.WorldMinutes() + 60.0))
	{
		const ss::LobbyEvent Lx = Home.Listing(X, nullptr, S.Unlocks());
		const ss::net::LiveState Lv = Home.Live(X, S.WorldMinutes());
		if (Lx.Joinable && Lx.BuyInCents > 0 && Lx.BuyInCents <= 1100 && Lx.Spec.Entrants <= 3000 && (Lv.St == ss::net::Status::LateReg || (Lv.St == ss::net::Status::Registering && Lv.StartsIn <= 60.0)))
		{
			S.RegisterEvent(Lx);
			break;
		}
	}
	if (S.TableCount() < 1)
	{
		std::printf("  sites: no RiverLine event open at this hour; RiverLine's table checks are skipped\n");
		return;
	}
	S.CurrentPace = ss::Pace::Full;
	P.Now = Step(S, P.Now, 3.0);
	const DrawList L = P.Settled();
	const SitesBox Pace = SitesRun(L, "Pace", P.M);
	const SitesBox Comp = SitesRun(L, "Composure", P.M);
	const bool Own = Pace.Found && Pace.X == 24.0f && Pace.Base == 866.0f && Comp.Found && Comp.X == 24.0f && Comp.Base == 812.0f && SitesRunIn(L, "Pace", Font::Bold, SitesControlsArea) &&
		SitesRunIn(L, "Lean back (Space)", Font::Bold, SitesControlsArea) && SitesRunIn(L, "Full", Font::Bold, SitesControlsArea) && SitesRunIn(L, "Players left", Font::Regular, SitesPanelArea) &&
		SitesRunIn(L, "Chat", Font::Bold, SitesPanelArea) && SitesRunIn(L, "Payouts", Font::Bold, SitesPanelArea) && SitesRunIn(L, "Standings", Font::Bold, SitesPanelArea);
	bool Rival = false;
	for (const char* W : {"PACE", "COMPOSURE", "FULL", "LEAN BACK (SPACE)", "INFO", "CHAT", "STANDINGS", "ICM VALUE", "CHIP LEADERS", "YOUR PLACE"})
	{
		Rival = Rival || SitesAnyRunIn(L, W, {0.0f, 0.0f, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height});
	}
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		Rival = Rival || (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.rfind("BLINDS UP", 0) == 0);
	}
	SitesExpect(Own && !Rival, "riverline table: its own controls and card at their places in its Bold, nothing of a rival's");
	// L10: the HUD's default, look by look.
	const sd::SiteLook& Rl = sd::SitesLook(SiteId::RiverLine);
	const sd::SiteLook& Ig = sd::SitesLook(SiteId::Ignite);
	const sd::SiteLook& Rit = sd::SitesLook(SiteId::RunItTwice);
	const bool Was = S.Hud;
	S.Hud = false;
	const bool OffEverywhereButRit = !sd::SitesHudOn(P.RL, S, Rl) && !sd::SitesHudOn(P.RL, S, Ig) && sd::SitesHudOn(P.RL, S, Rit);
	sd::SitesToggleHud(P.RL, S, Rit);
	const bool RitOff = !sd::SitesHudOn(P.RL, S, Rit) && !S.Hud;
	sd::SitesToggleHud(P.RL, S, Rit);
	sd::SitesToggleHud(P.RL, S, Rl);
	const bool HomeOn = S.Hud && sd::SitesHudOn(P.RL, S, Rl) && sd::SitesHudOn(P.RL, S, Ig) && sd::SitesHudOn(P.RL, S, Rit);
	S.Hud = Was;
	SitesExpect(Was && OffEverywhereButRit && RitOff && HomeOn && !Rl.HudDefault && !Ig.HudDefault && Rit.HudDefault,
		"hud default: RunItTwice's HUD is on whatever the session's switch says (its own toggle UI state); RiverLine's and Ignite's follow Session::Hud");
}

/** The rank glyphs a face-up card draws ("A", "K", ..., "10", "2"): one text run each. */
bool SitesRankRun(const std::string& T)
{
	static const std::set<std::string> Ranks = {"A", "K", "Q", "J", "10", "9", "8", "7", "6", "5", "4", "3", "2"};
	return Ranks.count(T) > 0;
}

void SitesRitTable()
{
	// Update 3, G1 (§3.5, v1.2): RunItTwice's octagon through its look: the black cloth inside the red-lit rail, data cards with
	// the stacks in big blinds, the Twin Stripe backs, lacquer chips, the chrome puck, the slabs; no RiverLine colour; and while
	// the lower third is up (an all-in with cards to come) no hole card inside its rect.
	SitesLaptop P("sites-rit-table");
	ss::Session& S = P.S;
	P.At(SitesEvening);
	S.BankrollCents = 200000;
	P.Join(SiteId::RunItTwice, 50000);
	ss::net::EventInstance E;
	if (!SitesOpenEvent(S, SiteId::RunItTwice, E, 2200))
	{
		std::printf("  sites: no RunItTwice event open at this hour; the octagon's checks are skipped\n");
		return;
	}
	S.RegisterEvent(ss::net::Site(SiteId::RunItTwice).Listing(E, nullptr, S.Unlocks()));
	S.CurrentPace = ss::Pace::Full;
	P.Now = Step(S, P.Now, 3.0);
	DrawList L = P.Settled();
	WriteList("site_table_runittwice", L);
	SitesNoLeak(L, SiteId::RunItTwice, "RunItTwice's table");
	namespace sd = ss::ui::rlsites_detail;
	const sd::SiteLook& Look = sd::SitesLook(SiteId::RunItTwice);
	// The cloth, the rail's Signal Red light and its LED core, the chrome hairline: all drawn.
	std::set<uint32_t> Cols;
	for (const ss::ui::Vertex& V : L.Vertices)
	{
		Cols.insert(SitesRgb(V.Col));
	}
	SitesExpect(Cols.count(0x09090b) && Cols.count(0xd2141a) && Cols.count(0xff2633), "rit table: the black cloth and the red-lit rail (Signal Red, its LED core)");
	bool Bb = false;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		Bb = Bb || (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == "BB" && Cmd.Text.Face == ss::ui::Font::BoldCondensed);
	}
	SitesExpect(Bb && sd::SitesBetsInBb(P.RL, Look), "rit table: stacks and bets in big blinds by default, in BoldCondensed");
	std::printf("  RunItTwice's table: %zu vertices, %zu commands (budget 19k vertices)\n", L.Vertices.size(), L.Cmds.size());
	SitesExpect(L.Vertices.size() <= 19000, "rit table: within its vertex budget (19k, §10)");
	// G1 fix (g1-audit M2, M1): its own controls and its own panel (§3.5).
	SitesRivalControls(L, SiteId::RunItTwice, "RunItTwice");
	SitesRitPanel(P, L);
	SitesRivalGrade(P, SiteId::RunItTwice, "site_table_runittwice_grade");
	// The toggle: chips, then big blinds again (UI state).
	sd::SitesToggleBets(P.RL);
	SitesExpect(!sd::SitesBetsInBb(P.RL, Look), "rit table: the toggle shows chips");
	sd::SitesToggleBets(P.RL);
	SitesExpect(sd::SitesBetsInBb(P.RL, Look), "rit table: and big blinds again");
	// The lower third: the hero showing equity in a hand still running puts the band up, and no hole card is drawn inside it.
	if (!S.CurHand || S.CurHand->bComplete)
	{
		std::printf("  sites: no hand running on the octagon this frame; the lower third's check is skipped\n");
		return;
	}
	ss::SeatVis* Hero = nullptr;
	for (ss::SeatVis& V : S.Seats)
	{
		Hero = V.Present && V.IsHero ? &V : Hero;
	}
	if (!Hero || Hero->Hole.size() != 2 || Hero->Folded)
	{
		std::printf("  sites: the hero isn't holding cards this frame; the lower third's check is skipped\n");
		return;
	}
	const auto RanksIn = [](const DrawList& Ld, const ss::ui::Rect& R) {
		int N = 0;
		for (const ss::ui::DrawCmd& Cmd : Ld.Cmds)
		{
			const bool In = Cmd.Text.BaselineY >= R.Y && Cmd.Text.Y <= R.Y + R.H && Cmd.Text.X >= R.X && Cmd.Text.X <= R.X + R.W;
			N += Cmd.Type == ss::ui::DrawCmd::Kind::Text && SitesRankRun(Cmd.Text.Text) && In ? 1 : 0;
		}
		return N;
	};
	const ss::ui::Rect Above{Look.Cx - 118.0f, 560.0f, 236.0f, 110.0f}; // over the hero's card, where its hole cards stand
	const int Before = RanksIn(L, Above);
	const bool HadEquity = Hero->HasEquity;
	// Both hands shown, as Session::UpdateEquities sets them at the reveal (the hero's and one other's equity): ui-rit's band
	// draws inside the lower third's rect, and the shared table draws no hole card inside it or over the hero's card.
	ss::SeatVis* Villain = nullptr;
	for (ss::SeatVis& V : S.Seats)
	{
		Villain = !Villain && V.Present && !V.IsHero && !V.Folded ? &V : Villain;
	}
	const bool VillainHad = Villain && Villain->HasEquity;
	Hero->HasEquity = true;
	Hero->Equity = 0.43;
	// The hero's equity alone raises the band for the shared table (ui-rit's band itself waits for a second hand), so this
	// frame shows only what the shared table draws: no hole card inside the band's rect or over the hero's card.
	const DrawList Lb = P.Settled();
	SitesExpect(sd::SitesLowerThirdUp(S) && Before >= 2 && RanksIn(Lb, Look.LowerThird) == 0 && RanksIn(Lb, Above) == 0,
		"rit table: with the lower third up no hole card is drawn inside it (the band shows the all-in hands)");
	if (Villain)
	{
		Villain->HasEquity = true;
		Villain->Equity = 0.57;
	}
	const DrawList Lv = P.Settled();
	WriteList("site_table_runittwice_allin", Lv);
	bool BandDrawn = false;
	for (const ss::ui::DrawCmd& Cmd : Lv.Cmds)
	{
		const ss::ui::Rect& Lt = Look.LowerThird;
		BandDrawn = BandDrawn || (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == "ALL IN" && Cmd.Text.BaselineY >= Lt.Y && Cmd.Text.BaselineY <= Lt.Y + Lt.H &&
									 Cmd.Text.X >= Lt.X && Cmd.Text.X <= Lt.X + Lt.W);
	}
	SitesExpect(!Villain || BandDrawn, "rit table: with both hands' equity the band (its ALL IN tag) draws inside the lower third's rect");
	// The run-out is a replay of a hand the engine has already finished (CurHand->bComplete is true through it): the band stays
	// up for the whole run-out and goes down when the showdown prints a hand (review fix: it used to need an unfinished hand,
	// so no real all-in ever raised it).
	if (S.Board.empty())
	{
		S.CurHand->bComplete = true;
		const bool UpInRunOut = sd::SitesLowerThirdUp(S);
		Hero->HandLabel = "Pair of Fives";
		const bool DownAtShowdown = !sd::SitesLowerThirdUp(S);
		Hero->HandLabel.clear();
		S.CurHand->bComplete = false;
		SitesExpect(UpInRunOut && DownAtShowdown, "rit table: the band is up through the replayed run-out and down once the showdown prints");
	}
	Hero->HasEquity = HadEquity;
	if (Villain)
	{
		Villain->HasEquity = VillainHad;
	}
}

void SitesClause()
{
	// The clause (§5.7): under a deal that plays only on RiverLine, a rival's register button warns and needs a second press.
	SitesLaptop P("sites-clause");
	ss::Session& S = P.S;
	P.At(SitesEvening);
	S.BankrollCents = 200000;
	P.Join(SiteId::Ignite, 50000);
	S.DebugSponsor("offer riverline");
	for (ss::sponsor::Contract& K : S.Life.Sponsors.Contracts)
	{
		if (K.Brand == "riverline")
		{
			K.T.Only = "riverline";
		}
	}
	const std::string Sign = S.Life.Sponsors.Contracts.empty() ? std::string("no contract") : S.SignDeal(S.Life.Sponsors.Contracts.back().Id);
	const std::string Warn = ss::ui::rlsites_detail::SitesClauseWarning(S, SiteId::Ignite, false, false);
	if (Warn.empty())
	{
		std::printf("  sites: no clause strike to warn of (core-deals' WouldStrike: %s); the register check is skipped\n", Sign.empty() ? "signed" : Sign.c_str());
		return;
	}
	SitesExpect(Warn == "Strike under your Team RiverLine deal", "clause: the warning names Team RiverLine's deal");
	ss::net::EventInstance E;
	if (!SitesOpenEvent(S, SiteId::Ignite, E, 2200))
	{
		return;
	}
	P.RL.SiteShown = SiteId::Ignite;
	P.RL.OpenPage(Page::Lobby, P.Now);
	P.RL.SelectEvent(E.Id);
	// Update 3 (G1): Ignite's register button lives on its event sheet (§2.4 B), opened as a cabinet's click opens it, once the
	// page has drawn (a page opening closes any sheet).
	P.Settled();
	ss::ui::rlignite_detail::IgniteOpenSheet(P.RL, E.Id, P.Now);
	DrawList L = P.Settled();
	WriteList("site_register_clause", L);
	const SitesBox B = SitesRun(L, Warn, P.M);
	SitesExpect(B.Found, "clause: the register button reads the strike");
	if (B.Found)
	{
		P.Click(B.X + B.W / 2.0f, B.Base - 6.0f);
	}
	SitesExpect(S.TableCount() == 0 && S.ConfirmRegister, "clause: the first press only warns");
	L = P.Settled();
	const SitesBox Take = SitesRun(L, "Take the strike", P.M);
	SitesExpect(Take.Found, "clause: the second press is the strike");
	// A featured slide's Register opens the confirm step directly (DrawSlide's case 0: SelectEvent, then ConfirmRegister):
	// under the clause that step still says it's a strike, never a plain Confirm.
	const SitesBox Cancel = SitesRun(L, "Cancel", P.M);
	if (Cancel.Found)
	{
		P.Click(Cancel.X + Cancel.W / 2.0f, Cancel.Base - 6.0f);
	}
	SitesExpect(!S.ConfirmRegister, "clause: Cancel steps back");
	P.RL.SelectEvent(E.Id);
	S.ConfirmRegister = true;
	L = P.Settled();
	SitesExpect(SitesRun(L, "Take the strike", P.M).Found && !SitesRun(L, "Confirm", P.M).Found && S.TableCount() == 0,
		"clause: the confirm step a featured slide opens warns of the strike too");
	// Update 3 (G1, SPEC §7.7): RunItTwice's Prospectus registers through the same shared block, so under the clause it warns
	// and waits for the second press too.
	P.Join(SiteId::RunItTwice, 50000);
	ss::net::EventInstance Er;
	if (SitesOpenEvent(S, SiteId::RunItTwice, Er, 2200))
	{
		S.ConfirmRegister = false;
		P.RL.SiteShown = SiteId::RunItTwice;
		P.RL.OpenPage(Page::Lobby, P.Now);
		P.Settled();
		P.RL.SelectEvent(Er.Id);
		const DrawList Lr = P.Settled();
		const std::string WarnR = ss::ui::rlsites_detail::SitesClauseWarning(S, SiteId::RunItTwice, false, false);
		bool Warned = false;
		for (const ss::ui::DrawCmd& Cmd : Lr.Cmds)
		{
			Warned = Warned || (Cmd.Type == ss::ui::DrawCmd::Kind::Text && (Cmd.Text.Text == WarnR || Cmd.Text.Text == ss::ui::rlnet_detail::NetUpper(WarnR)));
		}
		SitesExpect(!WarnR.empty() && Warned && S.TableCount() == 0, "clause: RunItTwice's Prospectus warns of the strike before it registers");
	}
}

// ------------------------------------------------------------------ a rival's result, a rival arrival's card

void SitesResults()
{
	// A rival's result (§5.5): its prize landed on that site's wallet, so the results screen's balance is that wallet's (as
	// the top bar's is), never the bank's; RiverLine's results keep the bank's.
	SitesLaptop P("sites-results");
	ss::Session& S = P.S;
	P.At(SitesEvening);
	S.BankrollCents = 123400;
	P.Join(SiteId::Ignite, 21240);
	ss::HistoryEntry H;
	H.Name = "Spark Jackpot";
	H.Place = 3;
	H.Entrants = 410;
	H.Prize = 18600;
	H.BuyInCents = 550;
	H.EventId = "ignite.spark-jackpot@" + std::to_string(static_cast<long long>(S.WorldMinutes() - 240.0));
	S.History.insert(S.History.begin(), H);
	S.LastResults = ss::Results();
	S.LastResults.EventName = H.Name;
	S.LastResults.Place = H.Place;
	S.LastResults.Entrants = H.Entrants;
	S.LastResults.PrizeCents = H.Prize;
	S.LastResults.BuyInCents = H.BuyInCents;
	S.HasResults = true;
	S.LastSiteResult = SiteId::Ignite; // as ShowResults names the result's site (update 3, §1.11)
	S.CurrentScreen = ss::Screen::Results;
	DrawList L = P.Settled();
	WriteList("site_results_ignite", L);
	SitesExpect(DrewRun(L, "Ignite balance now $212.40") && !DrewRun(L, "Balance now $1,234.00") && DrewRun(L, "Balance $212.40"), "results: a rival's result shows that site's balance");
	SitesNoLeak(L, SiteId::Ignite, "Ignite's results");
	// Update 3 (§1.11): the frame is the result's own site, Session::LastSiteResult, even for a result that never entered
	// History (a FUSE): History's front here is a RiverLine event of another name, and the screen is still Ignite's.
	{
		const std::vector<ss::HistoryEntry> Kept = S.History;
		S.History.front().EventId = "mm-26@1530";
		S.History.front().Name = "Monday Mash";
		const DrawList Lf = P.Settled();
		SitesExpect(DrewRun(Lf, "Ignite balance now $212.40") && !DrewRun(Lf, "Balance now $1,234.00"), "results: the result's site is LastSiteResult's, whatever History's front was");
		S.History = Kept;
	}
	// Career is RiverLine's page (its numbers RiverLine's alone): the result in its recent list names the site it was played on.
	S.HasResults = false;
	S.CurrentScreen = ss::Screen::Lobby;
	P.RL.SiteShown = SiteId::RiverLine;
	P.RL.OpenPage(Page::Career, P.Now);
	const DrawList Lc = P.Settled();
	SitesExpect(DrewRun(Lc, "Spark Jackpot") && DrewRun(Lc, "IGNITE"), "career: a rival's result in the recent list names its site");
	S.HasResults = true;
	S.CurrentScreen = ss::Screen::Results;
	// The same screen for a RiverLine event: the bank, as it always was.
	S.History.front().EventId = "mm-26@1530";
	S.LastSiteResult = SiteId::RiverLine;
	L = P.Settled();
	SitesExpect(DrewRun(L, "Balance now $1,234.00"), "results: RiverLine's result shows the bank");
}

void SitesArrivalCard()
{
	// Someone who arrived through Ignite alone (core-world names the site on their journey's first step): their card says
	// where they joined, never "RiverLine".
	SitesLaptop P("sites-arrival-card");
	ss::Session& S = P.S;
	P.At(SitesEvening);
	ss::world::World& W = S.Living();
	int Who = -1;
	for (int I = 0; I < static_cast<int>(W.People().size()) && Who < 0; ++I)
	{
		const ss::world::Npc* N = W.Get(I);
		Who = N && !N->Path.empty() && N->Path.front().Kind == ss::world::StepKind::Joined && W.ProfileOf(I).Arrived >= 0 ? I : Who;
	}
	if (Who < 0)
	{
		std::printf("  sites: no arrival on record in this world yet; the card check is skipped\n");
		return;
	}
	const_cast<ss::world::Npc*>(W.Get(Who))->Path.front().What = "Ignite";
	P.RL.ShowPlayer(Who, P.Now);
	const DrawList L = P.Settled();
	bool OnIgnite = false;
	bool OnRiverLine = false;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
		{
			OnIgnite = OnIgnite || Cmd.Text.Text.find("on Ignite since ") != std::string::npos;
			OnRiverLine = OnRiverLine || Cmd.Text.Text.find("on RiverLine since ") != std::string::npos;
		}
	}
	SitesExpect(OnIgnite && !OnRiverLine, "card: an Ignite arrival is \"on Ignite since\", not on RiverLine");
	// The JOURNEY tab: "Joined Ignite ..." under HOW THEY GOT HERE.
	P.RL.ShowCardTab(1);
	const DrawList Lj = P.Settled();
	bool Joined = false;
	for (const ss::ui::DrawCmd& Cmd : Lj.Cmds)
	{
		Joined = Joined || (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.rfind("Joined Ignite ", 0) == 0);
	}
	SitesExpect(Joined, "card: the journey says \"Joined Ignite\" for an Ignite arrival");
}

void SitesRivalBoardHero()
{
	// A rival's boards rank the player by what they did there (site::HeroOn: the account's season), never RiverLine's numbers.
	SitesLaptop P("sites-rival-board-hero");
	ss::Session& S = P.S;
	P.At(SitesEvening);
	if (ss::site::Account* A = P.Join(SiteId::RunItTwice, 0))
	{
		A->Hero.Year = ss::world::YearOf(ss::net::DayOf(S.WorldMinutes()));
		A->Hero.Points = 777.0;
		A->Hero.Events = 3;
	}
	P.RL.SiteShown = SiteId::RunItTwice;
	P.RL.OpenPage(Page::Leaderboards, P.Now);
	const DrawList L = P.Settled();
	// The YOU bar under the board (its value at the bar's baseline), and the standings list on the right, both.
	SitesExpect(SitesRun(L, "777 pts", P.M, 915.0f, 930.0f).Found && SitesRun(L, "777 pts", P.M, 0.0f, 914.0f).Found,
		"boards: the player's row on RunItTwice's Season board is their RunItTwice season");
	bool Opened = false;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		Opened = Opened || (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.find("lobby opened on") != std::string::npos);
	}
	// The next year the world starts every rival's season again on January 1 (Sim::SitesNewYear): the board says so.
	P.At(SitesEvening + 70.0 * SitesDay); // mid-January 2027
	P.RL.SiteShown = SiteId::RunItTwice;
	P.RL.OpenPage(Page::Leaderboards, P.Now);
	const DrawList Lj = P.Settled();
	bool January = false;
	bool Since = false;
	for (const ss::ui::DrawCmd& Cmd : Lj.Cmds)
	{
		January = January || (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.find("January") != std::string::npos);
		Since = Since || (Cmd.Text.Text.find("lobby opened on") != std::string::npos && Cmd.Type == ss::ui::DrawCmd::Kind::Text);
	}
	SitesExpect(Opened && January && !Since, "boards: a rival's season counts from its lobby's opening in its first year, from January after");
	// Update 3 (G1, SPEC §7.7's deliberate rewrite): Ignite's YOU bar sits at 790-810, clear of the dock's raised button; its
	// Season board (the second tab) is the account's season.
	{
		SitesLaptop Q("sites-rival-board-hero-ignite");
		Q.At(SitesEvening);
		if (ss::site::Account* A = Q.Join(SiteId::Ignite, 0))
		{
			A->Hero.Year = ss::world::YearOf(ss::net::DayOf(Q.S.WorldMinutes()));
			A->Hero.Points = 777.0;
			A->Hero.Events = 3;
		}
		Q.RL.SiteShown = SiteId::Ignite;
		Q.RL.OpenPage(Page::Leaderboards, Q.Now);
		const SitesBox Season = SitesRun(Q.Settled(), "Season", Q.M, 88.0f, 130.0f);
		if (Season.Found)
		{
			Q.Click(Season.X + Season.W / 2.0f, Season.Base - 6.0f);
		}
		const DrawList Li = Q.Settled();
		SitesExpect(Season.Found && SitesRun(Li, "777 pts", Q.M, 790.0f, 810.0f).Found, "boards: Ignite's YOU bar holds the player's season at 790-810, clear of the raised button");
	}
}

void SitesLifetime()
{
	// Bug 7 (§5.8): RiverLine's numbers on the laptop are site::HeroOn's: its own events only, and once History is full (it
	// holds the last hundred, never the whole record) the lifetime ones from the world's tracker less the rivals'. C3 (the gate
	// audit's should-fix 1): never below the hundred's own, since the tracker only exists since abeb60b and a career older than it
	// (or a save without a world) has a tracker that knows fewer. Here the tracker knows more than the hundred (200 results
	// granted), so Career's winnings are the tracker's; and on a laptop whose tracker knows none of them, the hundred's, never $0.
	const auto Hundred = [](ss::Session& S) {
		for (int K = 0; K < 100; ++K)
		{
			ss::HistoryEntry H;
			H.Name = "Daily Grind";
			H.Place = 40;
			H.Entrants = 500;
			H.Prize = 1 + K;
			H.BuyInCents = 1100;
			H.EventId = "daily-grind@" + std::to_string(1000 + K * 10);
			S.History.push_back(H);
		}
	};
	SitesLaptop P("sites-lifetime");
	ss::Session& S = P.S;
	P.At(SitesEvening);
	Hundred(S);
	S.Living().GrantHeroResults(200);
	const double World = S.WorldMinutes();
	const ss::net::HeroStats Want = ss::site::HeroOn(SiteId::RiverLine, S.HeroName, S.History, S.Life.Sites, &S.Living(), World);
	const ss::net::HeroStats Last = ss::net::StatsFrom(S.HeroName, S.History, World);
	P.RL.OpenPage(Page::Career, P.Now);
	const DrawList L = P.Settled();
	SitesExpect(Want.Earnings > Last.Earnings && DrewRun(L, ss::Money(Want.Earnings)) && !DrewRun(L, ss::Money(Last.Earnings)),
		"career: RiverLine's lifetime numbers are site::HeroOn's (the tracker less the rivals'), not the last hundred's");
	SitesLaptop Q("sites-lifetime-before");
	Q.At(SitesEvening);
	Hundred(Q.S);
	const ss::net::HeroStats Older = ss::site::HeroOn(SiteId::RiverLine, Q.S.HeroName, Q.S.History, Q.S.Life.Sites, &Q.S.Living(), Q.S.WorldMinutes());
	const ss::net::HeroStats Own = ss::net::StatsFrom(Q.S.HeroName, Q.S.History, Q.S.WorldMinutes());
	Q.RL.OpenPage(Page::Career, Q.Now);
	const DrawList Lq = Q.Settled();
	SitesExpect(Q.S.Living().HeroStats().Events == 0 && Own.Earnings > 0 && Older.Earnings == Own.Earnings && DrewRun(Lq, ss::Money(Own.Earnings)),
		"career: a tracker that knows none of the hundred shows the hundred's winnings, never $0 (C3)");
}

void SitesKast()
{
	// Kast's channel line (§5.11): "Poker · RiverLine tournaments" until a rival site has been played, then "Poker · tournaments".
	SitesLaptop P("sites-kast");
	ss::Session& S = P.S;
	P.At(SitesEvening);
	P.RL.OpenApp(App::Kast, P.Now);
	P.RL.ShowKastPage(ss::ui::RiverLine::KastPage::Channel);
	const auto Line = [](const DrawList& L, const std::string& Head) {
		for (const ss::ui::DrawCmd& Cmd : L.Cmds)
		{
			if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.rfind(Head, 0) == 0)
			{
				return true;
			}
		}
		return false;
	};
	DrawList L = P.Settled();
	const bool Home = Line(L, "Poker \xC2\xB7 RiverLine tournaments \xC2\xB7 peak ");
	P.Join(SiteId::Ignite, 0);
	ss::HistoryEntry H;
	H.Name = "Spark Jackpot";
	H.Place = 120;
	H.Entrants = 410;
	H.BuyInCents = 550;
	H.EventId = "ignite.spark-jackpot@" + std::to_string(static_cast<long long>(S.WorldMinutes() - 240.0));
	S.History.insert(S.History.begin(), H);
	L = P.Settled();
	SitesExpect(Home && Line(L, "Poker \xC2\xB7 tournaments \xC2\xB7 peak ") && !Line(L, "Poker \xC2\xB7 RiverLine tournaments"), "kast: the channel's line names RiverLine until a rival was played");
}

void SitesEmblems()
{
	// FamilyOf strips a rival's key (§5.8): a template id "ignite.<id>" keeps the emblem "<id>" has (its family's tile).
	const ss::net::Network& Net = ss::net::Shared();
	int Checked = 0;
	int Same = 0;
	for (const ss::net::EventTemplate& T : Net.Templates())
	{
		if (!T.Series.empty() || Checked >= 60)
		{
			continue;
		}
		ss::net::EventTemplate Away = T;
		Away.Id = "ignite." + T.Id;
		TableMeasurer M;
		DrawList A;
		DrawList B;
		{
			ss::ui::Canvas C(A, M, 200.0f, 200.0f, 1.0f);
			ss::ui::eventart::Emblem(C, T, 100.0f, 100.0f, 64.0f, 0.0);
		}
		{
			ss::ui::Canvas C(B, M, 200.0f, 200.0f, 1.0f);
			ss::ui::eventart::Emblem(C, Away, 100.0f, 100.0f, 64.0f, 0.0);
		}
		bool Equal = A.Vertices.size() == B.Vertices.size();
		for (size_t K = 0; Equal && K < A.Vertices.size(); ++K)
		{
			const ss::ui::Vertex& X = A.Vertices[K];
			const ss::ui::Vertex& Y = B.Vertices[K];
			Equal = X.X == Y.X && X.Y == Y.Y && X.Col.R == Y.Col.R && X.Col.G == Y.Col.G && X.Col.B == Y.Col.B && X.Col.A == Y.Col.A;
		}
		++Checked;
		Same += Equal ? 1 : 0;
		if (!Equal)
		{
			std::printf("  emblem differs: %s and %s\n", T.Id.c_str(), Away.Id.c_str());
		}
	}
	SitesExpect(Checked > 20 && Same == Checked, "emblems: a site's prefixed template id keeps its family's emblem");
}

void SitesBench()
{
	// SS_UI_BENCH=1 only (§6.7, §9: a rival-skinned lobby within 5% of RiverLine's draw time; never a CI assertion).
	if (!std::getenv("SS_UI_BENCH"))
	{
		return;
	}
	SitesLaptop P("sites-bench");
	P.At(SitesEvening);
	P.S.BankrollCents = 150000;
	P.Join(SiteId::Ignite, 21240);
	P.Join(SiteId::RunItTwice, 21240);
	for (const SiteId Site : {SiteId::RiverLine, SiteId::Ignite, SiteId::RunItTwice})
	{
		P.RL.SiteShown = Site;
		P.RL.OpenPage(Page::Lobby, P.Now);
		P.Settled();
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
		std::printf("UI BENCH sites lobby %-10s %6.3f ms/draw (best of 5 x 40)\n", ss::net::KeyOf(Site), Best);
	}
}

// ------------------------------------------------------------------ the copy

void SitesCopy()
{
	std::vector<std::pair<std::string, std::string>> Lines;
	for (const std::string& T : ss::ui::rlsites_detail::SitesUiInventory())
	{
		Lines.push_back({"sites", T});
	}
	// Update 3 (G1, SPEC §7.7): joined by RunItTwice's (ui-rit's RitInventory: every sentence its pages draw) and Ignite's
	// (ui-ignite's IgniteInventory, the shouts included).
	for (const std::string& T : ss::ui::rlrit_detail::RitInventory())
	{
		Lines.push_back({"rit", T});
	}
	std::vector<std::pair<std::string, std::string>> Chat; // BUZZ's shouts are table talk (ui-ignite lints them the same way)
	{
		const std::vector<std::string>& Shouts = ss::ui::rlignite_detail::IgniteShouts();
		const std::set<std::string> Shout(Shouts.begin(), Shouts.end());
		for (const std::string& T : ss::ui::rlignite_detail::IgniteInventory())
		{
			(Shout.count(T) ? Chat : Lines).push_back({"ignite", T});
		}
	}
	const std::map<std::string, std::string> Vars = {{"Site", "RunItTwice"}, {"Date", "Oct 6"}, {"Series", "RunItTwice Open 2026"}, {"Backer", "Lantern Row Staking"}, {"Team", "Team RiverLine"},
		{"Event", "Spark Jackpot"}, {"Headliner", "LilaVance"}};
	int Problems = copylint::Run("sites-ui", Lines, copylint::LetterBody, Vars) + copylint::Run("sites-shouts", Chat, copylint::TableTalk, Vars);
	Lines.insert(Lines.end(), Chat.begin(), Chat.end());
	// In-world copy never says "AI", "bot" or "autoplay" (U1), and no real or dead site's name.
	for (const auto& L : Lines)
	{
		const std::string Low = copylint::Lower(L.second);
		for (const char* Banned : {" ai ", " ai.", "bot", "autoplay", "pokerstars", "ggpoker", "ignition", "run it once", "skrill", "neteller", "paypal"})
		{
			if (Low.find(Banned) != std::string::npos)
			{
				std::printf("COPY (sites-ui) \"%s\" says \"%s\"\n", L.second.c_str(), Banned);
				++Problems;
			}
		}
	}
	SitesExpect(Problems == 0, "sites: the laptop's sites copy passes the lint");
}
} // namespace sites_seg

void SegSites()
{
	using namespace sites_seg;
	SitesSkins();
	SitesLooks();
	SitesLobbies();
	SitesCashier();
	SitesPromotions();
	SitesPokerButton();
	SitesBank();
	SitesCareer();
	SitesRent();
	SitesMarks();
	SitesMail();
	SitesTables();
	SitesRitTable();
	SitesRitIcm();
	SitesRiverLineTable();
	SitesClause();
	SitesResults();
	SitesArrivalCard();
	SitesLifetime();
	SitesRivalBoardHero();
	SitesKast();
	SitesEmblems();
	SitesBench();
	SitesCopy();
	std::printf("  ui-sites: %d checks\n", SitesChecks);
}
} // namespace ui_test
