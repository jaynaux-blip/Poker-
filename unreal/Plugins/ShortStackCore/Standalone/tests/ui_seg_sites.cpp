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
#include "ShortStack/UI/EventArt.h"
#include "ShortStack/UI/RiverLine.h"
#include "ShortStack/UI/SecondScreen.h"
#include "ShortStack/UI/SiteArt.h"
#include "ShortStack/UI/Ui.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/SitesDetail.h"
#include "../../Source/ShortStackCore/Private/ShortStack/UI/RiverLineShared.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
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
		SitesExpect(DrewRun(L, "Promotions") && DrewRun(L, "Cashier") && !DrewRun(L, "Career") && !DrewRun(L, "Embercrest"), ("sites: " + Short + "'s nav has its own six pages").c_str());
		SitesExpect(DrewRun(L, "Balance $212.40"), ("sites: " + Short + "'s top bar shows its wallet").c_str());
		SitesExpect(!P.RL.ListedEvents().empty() && std::all_of(P.RL.ListedEvents().begin(), P.RL.ListedEvents().end(), [Site](const std::string& Id) { return ss::net::SiteOf(Id) == Site; }),
			("sites: " + Short + "'s lobby lists its own events only").c_str());
		SitesExpect(SitesSpelled(L, "TONIGHT ON " + ss::ui::rlsites_detail::SitesUpperName(Site)) || SitesSpelled(L, "SUNDAY MAJOR") || SitesSpelled(L, "THIS WEEK'S BIGGEST") ||
				SitesSpelled(L, Site == SiteId::Ignite ? "WELCOME BONUS" : "SECOND RUN CLUB"),
			("sites: " + Short + "'s featured slides are its own").c_str());
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
	SitesExpect(DrewRun(L, "Your balance on Ignite") && DrewRun(L, "Deposit") && DrewRun(L, "Withdraw"), "cashier: the balance, a deposit and a withdrawal");
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
	WriteList("site_promotions", L);
	SitesExpect(SitesSpelled(L, "WELCOME BONUS") && DrewRun(L, "$50.00 still pending"), "promotions: Ignite's bonus bar, $50 pending");
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
	SitesExpect(SitesSpelled(L, "SECOND RUN CLUB") && SitesSpelled(L, "WELCOME TICKETS"), "promotions: RunItTwice's club and its welcome tickets");
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
	S.CurrentScreen = ss::Screen::Results;
	DrawList L = P.Settled();
	WriteList("site_results_ignite", L);
	SitesExpect(DrewRun(L, "Ignite balance now $212.40") && !DrewRun(L, "Balance now $1,234.00") && DrewRun(L, "Balance $212.40"), "results: a rival's result shows that site's balance");
	SitesNoLeak(L, SiteId::Ignite, "Ignite's results");
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
	int Problems = copylint::Run("sites-ui", Lines, copylint::LetterBody, {{"Site", "RunItTwice"}, {"Date", "Oct 6"}, {"Series", "RunItTwice Open 2026"}, {"Backer", "Lantern Row Staking"}});
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
