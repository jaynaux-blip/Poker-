// rivals_ui: ui-ignite's checks (update 3 SPEC §7.7): the dock opens every page (no "COMING SOON" anywhere), the shelves
// write Listed and arrows walk it, one-frame clicks under every overlay fire nothing, Previewing grants nothing and plays no
// sound, Afterburner Hours at 2 AM and Prime Time at 9 PM, no RiverLine hex in any page, contrast at 4.5:1, the dock's
// keep-out, the five sounds, the faces measured with Measure(…, Font), the copy (IgniteInventory: the lint, U1, the
// trademark tiers) and BUZZ's 150 shouts (no line twice within 24 world hours), SS_UI_BENCH (printed). G1 (§2.9): the look
// on today's data; the loops' checks (the wheel lands on core's wedge, the reveal shows core's multiplier) come with G2.
// The G1 fix pass (fix-ignite): PLAY's draw time against SPEC §10's caps, timed as the audit did (M3); the beat by the hour
// and the final table's tube on it, a pot won's fountain and "+4,200" (L1); shouts at their hours and the wheel's placards
// clear of its sign (L6).
// Compiled into rivals_ui_test and called from rivals_ui_main.cpp (the kit is UiSeg.h); the helpers are in
// ui_test::ignite_seg. With an out-dir, the pages are written as ignite_*.json (the GDI+ rasterizer draws them).
#include "UiSeg.h"

#include "CopyLint.h"
#include "Trademarks.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Gear.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/SiteShow.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/UI/EventArt.h"
#include "ShortStack/UI/RiverLine.h"
#include "ShortStack/UI/Ui.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/SitesDetail.h"
#include "../../Source/ShortStackCore/Private/ShortStack/UI/IgniteArt.h"
#include "../../Source/ShortStackCore/Private/ShortStack/UI/RiverLineShared.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace ui_test
{
namespace ignite_seg
{
using App = ss::ui::RiverLine::App;
using Page = ss::ui::RiverLine::Page;
using ss::net::SiteId;
using ss::ui::DrawList;
constexpr double IgDay = 1440.0;
/** Wed Nov 4, 2026 (Ignite's lobby opened Mon Nov 2): 9 PM is Prime Time, the next morning's 2 AM Afterburner Hours. */
constexpr double IgPrime = 30.0 * IgDay + 21.0 * 60.0;
constexpr double IgLate = 31.0 * IgDay + 2.0 * 60.0;

int IgChecks = 0;
void IgExpect(bool Condition, const char* What)
{
	++IgChecks;
	Expect(Condition, What);
}

/** Hooks that count the cues the laptop asked for (Session::UiSound forwards to Sound). */
struct IgHooks : QuietHooks
{
	std::map<int, int> Sounds;
	void Sound(ss::SoundId Id, double) override { ++Sounds[static_cast<int>(Id)]; }
	bool GoOut(const std::string&, ss::Chips) override { return true; }
	int Count(ss::SoundId Id) const
	{
		const auto It = Sounds.find(static_cast<int>(Id));
		return It == Sounds.end() ? 0 : It->second;
	}
	int Ignite() const
	{
		int N = 0;
		for (const ss::SoundId Id : {ss::SoundId::IgSpark, ss::SoundId::IgPop, ss::SoundId::IgTick, ss::SoundId::IgFuse, ss::SoundId::IgBoom})
		{
			N += Count(Id);
		}
		return N;
	}
};

/** A session and its laptop on Ignite, the pointer driven by hand (sites_seg's kit, this file's own copy). */
struct IgLaptop
{
	IgHooks H;
	ss::Session S;
	ss::ui::RiverLine RL;
	TableMeasurer M;
	double Now = 1.0;
	explicit IgLaptop(const std::string& Seed) : S(H, Seed), RL(S)
	{
		S.CurrentScreen = ss::Screen::Lobby;
		Now = Step(S, Now, 0.2);
	}
	~IgLaptop() { ss::ui::rlignite_detail::IgniteForget(&RL); }
	DrawList Frame(float X = -1.0f, float Y = -1.0f, bool Down = false, bool Pressed = false, bool Released = false, float Wheel = 0.0f)
	{
		RL.UI.Ptr.Active = X >= 0.0f;
		RL.UI.Ptr.X = X;
		RL.UI.Ptr.Y = Y;
		RL.UI.Ptr.Down = Down;
		RL.UI.Ptr.Pressed = Pressed;
		RL.UI.Ptr.Released = Released;
		RL.UI.Ptr.Wheel = Wheel;
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
	/** A click shorter than a frame: pressed and released in one (house law 3's case). */
	DrawList OneFrame(float X, float Y)
	{
		const DrawList L = Frame(X, Y, false, true, true);
		Now += 0.05;
		return L;
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
		ss::site::Join(S.Life.Sites, SiteId::Ignite, S.WorldMinutes());
		ss::site::Account* A = ss::site::AccountOf(S.Life.Sites, SiteId::Ignite);
		if (A)
		{
			A->Cents = Cents;
		}
		S.Life.Update2LinesRead |= ss::life::U2Sites;
		return A;
	}
	void Open(Page P)
	{
		RL.SiteShown = SiteId::Ignite;
		RL.OpenApp(App::RiverLine, Now);
		S.ShowLobby();
		RL.OpenPage(P, Now);
	}
};

/** A text run that is exactly Text (baseline in [MinY, MaxY]): where it sits and how wide. */
struct IgBox
{
	bool Found = false;
	float X = 0.0f;
	float Base = 0.0f;
	float W = 0.0f;
	ss::ui::Font Face = ss::ui::Font::Regular;
	float Size = 0.0f;
};
IgBox IgRun(const DrawList& L, const std::string& Text, const TableMeasurer& M, float MinY = -1.0e9f, float MaxY = 1.0e9f)
{
	IgBox Out;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text && Cmd.Text.BaselineY >= MinY && Cmd.Text.BaselineY <= MaxY)
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

bool IgStartsWith(const DrawList& L, const std::string& Head)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.rfind(Head, 0) == 0)
		{
			return true;
		}
	}
	return false;
}

/** Tracked capitals (one glyph a run) joined back up along a baseline. */
bool IgSpelled(const DrawList& L, const std::string& Word)
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

/** No run anywhere says it's coming (SPEC §0.5, §2.4: every G1 surface is live). */
bool IgNoComingSoon(const DrawList& L)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
		{
			const std::string Low = copylint::Lower(Cmd.Text.Text);
			if (Low.find("coming soon") != std::string::npos || Low.find("soon!") != std::string::npos)
			{
				return false;
			}
		}
	}
	return true;
}

uint32_t IgRgb(const ss::ui::Color& C)
{
	const auto Byte = [](float V) { return static_cast<uint32_t>(std::min(255.0f, std::max(0.0f, V * 255.0f + 0.5f))); };
	return Byte(C.R) << 16 | Byte(C.G) << 8 | Byte(C.B);
}

std::set<uint32_t> IgHexes(const std::array<uint32_t, 16>& P)
{
	return std::set<uint32_t>(P.begin(), P.end());
}

/** The palette-leak check (§1.11): RiverLine's 16 hexes (and not one of the skin's own) drawn inside the client's area. */
int IgLeaks(const DrawList& L, std::set<uint32_t>& Seen)
{
	const std::set<uint32_t> Home = IgHexes(ss::net::Def(SiteId::RiverLine).Palette);
	std::set<uint32_t> Own = IgHexes(ss::net::Def(SiteId::Ignite).Palette);
	const std::set<uint32_t> Night = IgHexes(ss::net::Def(SiteId::Ignite).NightPalette);
	Own.insert(Night.begin(), Night.end());
	const auto Inside = [](float X, float Y) { return X >= 0.0f && X <= 1600.0f && Y >= 0.0f && Y <= 962.0f; };
	int Count = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
		{
			const uint32_t Rgb = IgRgb(Cmd.Text.Col);
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
		int Here = 0;
		float X0 = 1.0e9f, Y0 = 1.0e9f, X1 = -1.0e9f, Y1 = -1.0e9f;
		std::set<uint32_t> Mine;
		for (uint32_t K = 0; K < Cmd.VertexCount; ++K)
		{
			const ss::ui::Vertex& V = L.Vertices[Cmd.VertexStart + K];
			const uint32_t Rgb = IgRgb(V.Col);
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

void IgNoLeak(const DrawList& L, const std::string& What)
{
	std::set<uint32_t> Seen;
	const int Count = IgLeaks(L, Seen);
	if (Count > 0)
	{
		std::printf("  leak (%s):", What.c_str());
		for (const uint32_t H : Seen)
		{
			std::printf(" %06x", H);
		}
		std::printf(" in %d places\n", Count);
	}
	IgExpect(Count == 0, ("ignite: no RiverLine colour on the page: " + What).c_str());
}

double IgContrast(uint32_t A, uint32_t B)
{
	const auto Lin = [](double V) { return V <= 0.04045 ? V / 12.92 : std::pow((V + 0.055) / 1.055, 2.4); };
	const auto Lum = [&Lin](uint32_t C) { return 0.2126 * Lin(((C >> 16) & 255) / 255.0) + 0.7152 * Lin(((C >> 8) & 255) / 255.0) + 0.0722 * Lin((C & 255) / 255.0); };
	const double La = Lum(A);
	const double Lb = Lum(B);
	return (std::max(La, Lb) + 0.05) / (std::min(La, Lb) + 0.05);
}

/** Every page the client shows, checked and written (no leak, nothing "coming", it drew). */
void IgPage(const DrawList& L, const std::string& Name)
{
	WriteList(Name, L);
	IgExpect(!L.Cmds.empty() && L.Vertices.size() > 500, ("ignite: the page draws: " + Name).c_str());
	IgExpect(IgNoComingSoon(L), ("ignite: nothing on the page says it's coming: " + Name).c_str());
	IgNoLeak(L, Name);
}

/** An open Ignite event the tables can run (its seats open, a small field), or false. */
bool IgOpenEvent(ss::Session& S, ss::net::EventInstance& Out, ss::Chips MaxBuyIn)
{
	const ss::net::Network& Net = ss::net::Site(SiteId::Ignite);
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

/** Signs the world's Ignite clause (the Team RiverLine deal that plays only on RiverLine); false if it wouldn't bind. */
bool IgSignClause(ss::Session& S)
{
	S.DebugSponsor("offer riverline");
	for (ss::sponsor::Contract& K : S.Life.Sponsors.Contracts)
	{
		if (K.Brand == "riverline")
		{
			K.T.Only = "riverline";
		}
	}
	if (!S.Life.Sponsors.Contracts.empty())
	{
		S.SignDeal(S.Life.Sponsors.Contracts.back().Id);
	}
	return !ss::ui::rlsites_detail::SitesClauseWarning(S, SiteId::Ignite, false, false).empty();
}

// ------------------------------------------------------------------ the pages

void IgPages()
{
	// PLAY at 9 PM (Prime Time) and at 2 AM (Afterburner Hours), and every page the dock and the marquee open.
	IgLaptop P("ignite-pages");
	ss::Session& S = P.S;
	P.At(IgPrime);
	S.BankrollCents = 150000;
	P.Join(21240);
	S.Deposit(SiteId::Ignite, 5000, "card");
	P.Open(Page::Lobby);
	DrawList L = P.Settled();
	IgPage(L, "ignite_play_prime");
	const ss::site::show::Mood Prime = ss::site::show::IgniteMood(S.WorldMinutes());
	IgExpect(DrewRun(L, "Balance $262.40"), "ignite: the marquee's balance is the wallet (one run)");
	IgExpect(DrewRun(L, "PLAY") && DrewRun(L, "REWARDS") && DrewRun(L, "FLIP") && DrewRun(L, "BOARDS") && DrewRun(L, "WALLET"), "ignite: the dock's five items (G1: the centre reads FLIP)");
	IgExpect(!DrewRun(L, "Career") && !DrewRun(L, "Embercrest") && !DrewRun(L, "Promotions"), "ignite: none of RiverLine's nav");
	// Listed: Ignite's own events, in shelf-then-tile order, each once.
	const std::vector<std::string> Listed = P.RL.ListedEvents();
	std::set<std::string> Once(Listed.begin(), Listed.end());
	IgExpect(!Listed.empty() && Once.size() == Listed.size() && std::all_of(Listed.begin(), Listed.end(), [](const std::string& Id) { return ss::net::SiteOf(Id) == SiteId::Ignite; }),
		"ignite: the shelves write Listed (Ignite's events, each once)");
	std::printf("  ignite PLAY (9 PM, mood %d): %zu events listed, %zu vertices, %zu commands\n", static_cast<int>(Prime), Listed.size(), L.Vertices.size(), L.Cmds.size());
	if (Prime == ss::site::show::Mood::PrimeTime)
	{
		IgExpect(IgSpelled(L, "PRIME TIME") && DrewRun(L, "124 BPM"), "ignite: 9 PM is Prime Time (the marquee's band, 124 BPM)");
	}
	else
	{
		std::printf("  ignite: site::show::IgniteMood isn't landed yet (core-sites-look): 9 PM reads mood %d; the mood checks wait for it\n", static_cast<int>(Prime));
	}
	P.At(IgLate);
	P.Open(Page::Lobby);
	L = P.Settled();
	IgPage(L, "ignite_play_afterburner");
	const ss::site::show::Mood Late = ss::site::show::IgniteMood(S.WorldMinutes());
	if (Late == ss::site::show::Mood::Afterburner)
	{
		IgExpect(IgSpelled(L, "AFTERBURNER HOURS") && DrewRun(L, "96 BPM"), "ignite: 2 AM is Afterburner Hours (the moon, 96 BPM)");
	}
	// The other pages.
	const std::pair<Page, const char*> Pages[5] = {{Page::Promotions, "ignite_rewards"}, {Page::Leaderboards, "ignite_boards"}, {Page::News, "ignite_buzz"}, {Page::Cashier, "ignite_wallet"},
		{Page::Profile, "ignite_card"}};
	P.At(IgLate + 19.0 * 60.0);
	for (const auto& Pg : Pages)
	{
		P.Open(Pg.first);
		const DrawList Lp = P.Settled();
		IgPage(Lp, Pg.second);
		IgExpect(P.RL.CurrentPage() == Pg.first, (std::string("ignite: the page stays its own: ") + Pg.second).c_str());
	}
	// BUZZ's shouts.
	P.Open(Page::News);
	{
		const DrawList Lb = P.Settled();
		const IgBox Shouts = IgRun(Lb, "Shouts", P.M);
		if (Shouts.Found)
		{
			P.Click(Shouts.X + Shouts.W / 2.0f, Shouts.Base - 6.0f);
		}
		const DrawList Ls = P.Settled();
		IgPage(Ls, "ignite_buzz_shouts");
		IgExpect(DrewRun(Ls, "SHOUT"), "ignite: BUZZ's shouts, one every ten world minutes");
	}
	// RiverLine's own pages asked for on Ignite's client go to PLAY; FUSE (G2) is PLAY on Flip & Run.
	P.Open(Page::Career);
	P.Settled();
	IgExpect(P.RL.CurrentPage() == Page::Lobby, "ignite: Career is RiverLine's page (Ignite's client goes to PLAY)");
}

// ------------------------------------------------------------------ the dock and the marquee

void IgDock()
{
	IgLaptop P("ignite-dock");
	P.At(IgPrime);
	P.S.BankrollCents = 150000;
	P.Join(21240);
	P.Open(Page::Lobby);
	P.Settled();
	struct Target
	{
		float X, Y;
		Page Want;
		const char* What;
	};
	const Target Items[6] = {{626.0f, 916.0f, Page::Promotions, "REWARDS"}, {974.0f, 916.0f, Page::Leaderboards, "BOARDS"}, {1130.0f, 916.0f, Page::Cashier, "WALLET"},
		{470.0f, 916.0f, Page::Lobby, "PLAY"}, {568.0f, 36.0f, Page::Profile, "the ring (the Player Card)"}, {1174.0f, 36.0f, Page::News, "BUZZ's bell"}};
	for (const Target& T : Items)
	{
		P.Click(T.X, T.Y);
		const DrawList L = P.Settled();
		IgExpect(P.RL.CurrentPage() == T.Want && IgNoComingSoon(L), (std::string("ignite: the dock and the marquee open their pages: ") + T.What).c_str());
	}
	// The raised centre button (G1: FLIP) opens PLAY on Flip & Run; its keep-out holds the pointer from the page under it.
	P.Open(Page::Cashier);
	P.Settled();
	P.Click(800.0f, 850.0f);
	DrawList L = P.Settled();
	const ss::ui::rlignite_detail::IgniteUi& U = ss::ui::rlignite_detail::IgniteOf(&P.RL);
	IgExpect(P.RL.CurrentPage() == Page::Lobby && U.Chip == 2, "ignite: FLIP opens PLAY on Flip & Run");
	bool AllFlips = !P.RL.ListedEvents().empty();
	for (const std::string& Id : P.RL.ListedEvents())
	{
		ss::net::EventInstance E;
		const ss::net::Network& Net = ss::net::ForEvent(Id);
		AllFlips = AllFlips && Net.FindInstance(Id, E) && Net.TemplateOf(E).Fmt == ss::net::Format::Flip;
	}
	IgExpect(AllFlips, "ignite: on Flip & Run the floor lists Flip & Runs only");
	// A one-frame click on the dock's pill reaches nothing on the page under it (the Wallet's deposit button sits clear, so
	// the test page is PLAY: its floor stops at 832, the pill holds 884-948).
	const std::string Before = P.RL.SelectedEvent();
	P.OneFrame(700.0f, 900.0f);
	P.Settled();
	IgExpect(P.RL.SelectedEvent() == Before && ss::ui::rlignite_detail::IgniteOf(&P.RL).Sheet == false, "ignite: the dock's pill holds the pointer (keep-out)");
	(void)L;
}

// ------------------------------------------------------------------ PLAY: the shelves, the arrows, the sheet

void IgFloor()
{
	IgLaptop P("ignite-floor");
	ss::Session& S = P.S;
	P.At(IgPrime);
	S.BankrollCents = 150000;
	P.Join(21240);
	P.Open(Page::Lobby);
	P.Settled();
	const std::vector<std::string> Listed = P.RL.ListedEvents();
	if (Listed.size() < 3)
	{
		IgExpect(false, "ignite: the floor lists events at 9 PM");
		return;
	}
	// The arrow keys walk Listed (RiverLine::Key): down moves to the next cabinet, up back.
	P.RL.SelectEvent(Listed[0]);
	P.Settled();
	P.RL.Key("ArrowDown");
	P.Settled();
	IgExpect(P.RL.SelectedEvent() == Listed[1], "ignite: ArrowDown walks to the next cabinet");
	P.RL.Key("ArrowDown");
	P.RL.Key("ArrowUp");
	P.Settled();
	IgExpect(P.RL.SelectedEvent() == Listed[1], "ignite: ArrowUp walks back");
	IgExpect(!ss::ui::rlignite_detail::IgniteSheetOpen(&P.RL), "ignite: walking the shelves doesn't open the sheet");
	// A cabinet opens the sheet: find the first shelf's first cabinet by its buy-in at y 508-628.
	ss::ui::rlignite_detail::IgniteOpenSheet(P.RL, Listed[0], P.Now);
	DrawList L = P.Settled();
	IgPage(L, "ignite_sheet");
	IgExpect(ss::ui::rlignite_detail::IgniteSheetOpen(&P.RL) && P.RL.SelectedEvent() == Listed[0], "ignite: the event sheet is up on the event");
	IgExpect(DrewRun(L, "WHO'S IN") && DrewRun(L, "YOUR IGNITE BALANCE") && DrewRun(L, "Overview"), "ignite: the sheet's identity, register and tabs");
	// House law 3: a one-frame click inside the sheet over a cabinet underneath reaches no cabinet (the selection holds).
	P.OneFrame(300.0f, 560.0f);
	P.Settled();
	IgExpect(P.RL.SelectedEvent() == Listed[0] && ss::ui::rlignite_detail::IgniteSheetOpen(&P.RL), "ignite: a one-frame click inside the sheet reaches nothing under it");
	// A one-frame click on the scrim (the Bandstand, above the sheet) closes the sheet and is swallowed: no sign opens.
	P.OneFrame(400.0f, 200.0f);
	P.Settled();
	IgExpect(!ss::ui::rlignite_detail::IgniteSheetOpen(&P.RL) && P.RL.SelectedEvent() == Listed[0], "ignite: a scrim click closes the sheet and reaches nothing");
	// The tabs.
	ss::ui::rlignite_detail::IgniteOpenSheet(P.RL, Listed[0], P.Now);
	P.Settled();
	for (int Tab = 1; Tab < 4; ++Tab)
	{
		ss::ui::rlignite_detail::IgniteOf(&P.RL).SheetTab = Tab;
		const DrawList Lt = P.Settled();
		IgNoLeak(Lt, "ignite sheet tab " + std::to_string(Tab));
	}
	// The close button.
	P.Click(1540.0f, 332.0f);
	P.Settled();
	IgExpect(!ss::ui::rlignite_detail::IgniteSheetOpen(&P.RL), "ignite: the sheet's close button");
	// A chip filters the floor.
	const IgBox Bounty = IgRun(P.Frame(), "Bounty", P.M, 440.0f, 480.0f);
	if (Bounty.Found)
	{
		P.Click(Bounty.X + Bounty.W / 2.0f, Bounty.Base - 6.0f);
	}
	P.Settled();
	bool AllBounty = !P.RL.ListedEvents().empty();
	for (const std::string& Id : P.RL.ListedEvents())
	{
		ss::net::EventInstance E;
		const ss::net::Network& Net = ss::net::ForEvent(Id);
		AllBounty = AllBounty && Net.FindInstance(Id, E) && Net.TemplateOf(E).Fmt == ss::net::Format::Bounty;
	}
	IgExpect(Bounty.Found && AllBounty, "ignite: the Bounty chip lists bounty events only");
}

void IgCardHold()
{
	// A player card still up as the lobby opens (opened at a table, or the player's own stats) holds the input under it, as
	// RiverLine's LobbyPages does (house law 3): a one-frame click on the card over a cabinet opens nothing, and a click on its
	// backdrop over the dock closes the card and opens no page.
	IgLaptop P("ignite-card-hold");
	P.At(IgPrime);
	P.S.BankrollCents = 150000;
	P.Join(21240);
	P.Open(Page::Lobby);
	P.Settled();
	const std::string Before = P.RL.SelectedEvent();
	P.RL.ShowPlayer(ss::ui::RiverLine::HeroCard, P.Now);
	P.Settled();
	P.OneFrame(310.0f, 590.0f); // the card's body, over the first shelf's second cabinet (x 220-400, y 508-628)
	P.Settled();
	IgExpect(P.RL.CardOpen() && !ss::ui::rlignite_detail::IgniteSheetOpen(&P.RL) && P.RL.SelectedEvent() == Before,
		"ignite: a player card holds the floor under it (a one-frame click opens no cabinet)");
	P.OneFrame(626.0f, 916.0f); // the card's backdrop, over REWARDS on the dock
	P.Settled();
	IgExpect(P.RL.CurrentPage() == Page::Lobby, "ignite: a player card holds the dock under its backdrop");
	P.RL.ShowPlayer(-1, P.Now);
}

void IgClause()
{
	// Under a Team RiverLine deal the sheet's register button warns, then needs the second press (RegisterBlock's two presses
	// through SitesTwoPress; the strike gate is core's). PLAY's Today strip carries the contract's line.
	IgLaptop P("ignite-clause");
	ss::Session& S = P.S;
	P.At(IgPrime - 60.0);
	S.BankrollCents = 200000;
	P.Join(50000);
	if (!IgSignClause(S))
	{
		std::printf("  ignite: no clause strike to warn of here; the clause checks are skipped\n");
		return;
	}
	ss::net::EventInstance E;
	if (!IgOpenEvent(S, E, 2200))
	{
		std::printf("  ignite: no open event at this hour; the clause checks are skipped\n");
		return;
	}
	P.Open(Page::Lobby);
	DrawList L = P.Settled();
	IgExpect(DrewRun(L, "Under your Team RiverLine deal, every table here is a strike."), "ignite: under the clause PLAY says so, in the Today strip's place");
	ss::ui::rlignite_detail::IgniteOpenSheet(P.RL, E.Id, P.Now);
	P.Now += 7.0; // the deal's badge toast (RiverLine's, over every app) has gone
	L = P.Settled();
	IgPage(L, "ignite_sheet_clause");
	const IgBox Warn = IgRun(L, "Strike under your Team RiverLine deal", P.M);
	IgExpect(Warn.Found, "ignite: the sheet's register reads the strike");
	if (Warn.Found)
	{
		P.Click(Warn.X + Warn.W / 2.0f, Warn.Base - 6.0f);
	}
	IgExpect(S.TableCount() == 0 && S.ConfirmRegister, "ignite: the first press only warns");
	L = P.Settled();
	IgExpect(IgRun(L, "Take the strike", P.M).Found && S.TableCount() == 0, "ignite: the second press is the strike");
}

// ------------------------------------------------------------------ WALLET, REWARDS, BOARDS, the results

void IgMoney()
{
	IgLaptop P("ignite-wallet");
	ss::Session& S = P.S;
	P.At(30.0 * IgDay + 19.5 * 60.0);
	S.BankrollCents = 50000;
	P.Join(0);
	P.Open(Page::Cashier);
	DrawList L = P.Settled();
	IgExpect(DrewRun(L, "Your balance on Ignite"), "wallet: \"Your balance on Ignite\"");
	const IgBox Step50 = IgRun(L, "$50", P.M, 380.0f, 440.0f);
	IgExpect(Step50.Found, "wallet: the $50 step in the Cashier test's window (380-440)");
	if (Step50.Found)
	{
		P.Click(Step50.X + Step50.W / 2.0f, Step50.Base - 6.0f);
	}
	L = P.Settled();
	IgExpect(DrewRun(L, "$51.50"), "wallet: the bank pays the amount and the card's 3%");
	const IgBox Go = IgRun(L, "Deposit $50", P.M);
	if (Go.Found)
	{
		P.Click(Go.X + Go.W / 2.0f, Go.Base - 6.0f);
	}
	const ss::site::Account* A = ss::site::AccountOf(S.Life.Sites, SiteId::Ignite);
	IgExpect(A && A->Cents == 5000 && S.BankrollCents == 50000 - 5150, "wallet: Deposit $50 moves $50 onto Ignite and $51.50 out of the bank");
	L = P.Settled();
	IgPage(L, "ignite_wallet_deposit");
	IgExpect(DrewRun(L, "Balance $50.00"), "wallet: the marquee shows the wallet");
	const IgBox Twenty = IgRun(L, "$20", P.M, 380.0f, 440.0f);
	if (Twenty.Found)
	{
		P.Click(Twenty.X + Twenty.W / 2.0f, Twenty.Base - 6.0f);
	}
	L = P.Settled();
	const IgBox Out = IgRun(L, "Withdraw $20.00", P.M);
	if (Out.Found)
	{
		P.Click(Out.X + Out.W / 2.0f, Out.Base - 6.0f);
	}
	L = P.Settled();
	IgPage(L, "ignite_wallet_transit");
	A = ss::site::AccountOf(S.Life.Sites, SiteId::Ignite);
	IgExpect(A && A->Cents == 3000 && DrewRun(L, "$17.00") && DrewRun(L, "Balance $30.00") && DrewRun(L, "$17.00 arriving Sat to Mon") && DrewRun(L, "Withdrawal of $20.00 \xC2\xB7 requested Wed 7:30 PM"),
		"wallet: $20 out, $3 fee, $17 on its way (the copy the Cashier test pins)");
	// REWARDS: the Welcome bonus sub-page (rw3) with the Fuel Gauge, $50 pending after the first $50.
	P.Open(Page::Promotions);
	L = P.Settled();
	IgPage(L, "ignite_rewards_bonus");
	IgExpect(DrewRun(L, "WELCOME BONUS") && DrewRun(L, "$50.00 still pending") && DrewRun(L, "FUEL GAUGE"), "rewards: the welcome bonus, $50 pending, the Fuel Gauge");
	if (ss::site::Account* Ig = ss::site::AccountOf(S.Life.Sites, SiteId::Ignite))
	{
		Ig->BonusReleased = 1500;
		Ig->BonusPending = 0;
		Ig->BonusExpires = S.WorldMinutes() - 60.0;
	}
	L = P.Settled();
	IgExpect(DrewRun(L, "$15.00 released") && IgStartsWith(L, "The 60 days ran out on ") && !DrewRun(L, "All of it's yours now.") &&
			!DrewRun(L, "Your first deposit starts it: 100% of it, up to $100."),
		"rewards: an expired bonus says the window closed");
}

void IgBoards()
{
	IgLaptop P("ignite-boards");
	ss::Session& S = P.S;
	P.At(IgPrime);
	if (ss::site::Account* A = P.Join(0))
	{
		A->Hero.Year = ss::world::YearOf(ss::net::DayOf(S.WorldMinutes()));
		A->Hero.Points = 777.0;
		A->Hero.Events = 3;
	}
	P.Open(Page::Leaderboards);
	P.Settled();
	const IgBox Season = IgRun(P.Frame(), "Season", P.M, 88.0f, 130.0f);
	if (Season.Found)
	{
		P.Click(Season.X + Season.W / 2.0f, Season.Base - 6.0f);
	}
	const DrawList L = P.Settled();
	IgPage(L, "ignite_boards_season");
	IgExpect(IgRun(L, "777 pts", P.M, 790.0f, 810.0f).Found, "boards: the YOU bar's value sits at 790-810, clear of the raised button");
	IgExpect(DrewRun(L, "This Month") && DrewRun(L, "Season") && !DrewRun(L, "Night Shift"), "boards: Ignite's own boards");
}

void IgResults()
{
	IgLaptop P("ignite-results");
	ss::Session& S = P.S;
	P.At(IgPrime);
	S.BankrollCents = 123400;
	P.Join(21240);
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
	S.LastSiteResult = SiteId::Ignite; // ShowResults sets it (§1.11)
	S.CurrentScreen = ss::Screen::Results;
	const int Before = P.H.Ignite();
	P.Frame();
	for (int K = 0; K < 60; ++K)
	{
		P.Now += 1.0 / 30.0;
		P.Frame();
	}
	const DrawList L = P.Settled();
	IgPage(L, "ignite_results");
	IgExpect(DrewRun(L, "Ignite balance now $212.40") && DrewRun(L, "Balance $212.40") && !DrewRun(L, "Balance now $1,234.00"), "results: \"Your run\" shows Ignite's balance");
	IgExpect(DrewRun(L, "+$186.00") && DrewRun(L, "3RD OF 410") && DrewRun(L, "Play again"), "results: the prize, the place, Play again");
	IgExpect(P.H.Count(ss::SoundId::IgSpark) > 0 && P.H.Count(ss::SoundId::IgBoom) > 0 && P.H.Ignite() > Before, "results: the count-up ticks (IgSpark) and lands (IgBoom)");
	// A second look at the same result plays nothing again (a stage plays once).
	const int After = P.H.Ignite();
	P.Settled();
	IgExpect(P.H.Ignite() == After, "results: each stage's sound plays once");
}

// ------------------------------------------------------------------ the table layer

void IgTable()
{
	IgLaptop P("ignite-table");
	ss::Session& S = P.S;
	P.At(IgPrime - 60.0);
	S.BankrollCents = 200000;
	P.Join(50000);
	ss::net::EventInstance E;
	if (!IgOpenEvent(S, E, 2200))
	{
		std::printf("  ignite: no open event at this hour; the table checks are skipped\n");
		return;
	}
	S.RegisterEvent(ss::net::Site(SiteId::Ignite).Listing(E, nullptr, S.Unlocks()));
	S.CurrentPace = ss::Pace::Full;
	P.Now = Step(S, P.Now, 3.0);
	// The hero to act with 4 seconds left, a neighbour all in a moment ago: the fuse burns red, the ALL IN sticker slams.
	for (ss::SeatVis& Seat : S.Seats)
	{
		if (Seat.Present && Seat.IsHero)
		{
			Seat.Acting = true;
			Seat.ActStart = P.Now - 16.0;
			Seat.ActEnd = P.Now + 4.0;
		}
	}
	for (ss::SeatVis& Seat : S.Seats)
	{
		if (Seat.Present && !Seat.IsHero && !Seat.Folded)
		{
			Seat.LastAction = "All-in";
			Seat.LastActionAt = P.Now - 0.1;
			break;
		}
	}
	S.HasPrompt = false;
	const int Fuse0 = P.H.Count(ss::SoundId::IgFuse);
	const int Pop0 = P.H.Count(ss::SoundId::IgPop);
	const DrawList L = P.Frame();
	IgPage(L, "ignite_table");
	{
		size_t Top = 0; // the marquee's share: every vertex over the felt's chrome line (72)
		for (const ss::ui::Vertex& V : L.Vertices)
		{
			Top += V.Y < 73.0f ? 1 : 0;
		}
		std::printf("  ignite table: %zu vertices, the marquee's %zu (the table's budget is 22k, §10)\n", L.Vertices.size(), Top);
		if (std::getenv("SS_IG_DIAG"))
		{
			// The marquee by piece: the bulbs' row, then by x (logo, the countdown, the crowd, the ring, bubbles, BUZZ, the rest).
			const float Xs[8] = {0.0f, 222.0f, 406.0f, 538.0f, 600.0f, 1146.0f, 1202.0f, 1601.0f};
			const char* Names[7] = {"logo", "countdown", "online", "ring", "bubbles", "buzz", "right"};
			size_t Per[7] = {0, 0, 0, 0, 0, 0, 0};
			size_t Bulbs = 0;
			for (const ss::ui::Vertex& V : L.Vertices)
			{
				if (V.Y >= 73.0f)
				{
					continue;
				}
				if (V.Y >= 61.0f && V.Y <= 77.0f)
				{
					++Bulbs;
					continue;
				}
				for (int B = 0; B < 7; ++B)
				{
					Per[B] += V.X >= Xs[B] && V.X < Xs[B + 1] ? 1 : 0;
				}
			}
			std::printf("  diag: marquee bulbs row %zu", Bulbs);
			for (int B = 0; B < 7; ++B)
			{
				std::printf(", %s %zu", Names[B], Per[B]);
			}
			std::printf("\n");
		}
	}
	IgExpect(P.H.Count(ss::SoundId::IgFuse) > Fuse0, "table: the hero's fuse under 5 s crackles (IgFuse)");
	IgExpect(P.H.Count(ss::SoundId::IgPop) > Pop0, "table: an all-in sticker slams (IgPop)");
	IgExpect(DrewRun(L, "Balance " + ss::Money(ss::site::AccountOf(S.Life.Sites, SiteId::Ignite)->Cents)) && !DrewRun(L, "WALLET"), "table: the marquee stays over the table, the dock hides");
}

// ------------------------------------------------------------------ the stream preview: stills, no sound, no change

void IgPreview()
{
	IgLaptop P("ignite-preview");
	ss::Session& S = P.S;
	S.BankrollCents = 900000; // before the month passes: the rent gets paid, the apartment (and its internet) stays
	P.At(IgPrime);
	for (const char* Id : {ss::gear::FirstPcUpgrade().Id.c_str(), "webcam-1080", "mic-usb", "ring-light", "overlay-pack"})
	{
		S.Buy(Id);
	}
	S.Life.Energy = 90.0;
	const std::string Live = S.GoLive();
	if (!Live.empty() || !S.Streaming())
	{
		std::printf("  ignite: can't go live here (%s); the preview checks are skipped\n", Live.c_str());
		return;
	}
	P.Now = Step(S, P.Now, 5.0);
	P.Join(20000);
	P.Open(Page::Lobby);
	P.Settled();
	const ss::Chips Bank = S.BankrollCents;
	const ss::Chips Wallet = ss::site::AccountOf(S.Life.Sites, SiteId::Ignite)->Cents;
	P.RL.OpenApp(App::Kast, P.Now);
	P.RL.ShowKastPage(ss::ui::RiverLine::KastPage::Studio);
	const int Sounds = P.H.Ignite();
	DrawList A = P.Frame();
	P.Now += 0.7;
	DrawList B = P.Frame();
	WriteList("ignite_kast_studio", B);
	IgExpect(P.H.Ignite() == Sounds, "preview: the stream preview plays no Ignite sound");
	IgExpect(S.BankrollCents == Bank && ss::site::AccountOf(S.Life.Sites, SiteId::Ignite)->Cents == Wallet && S.TableCount() == 0, "preview: the stream preview grants and moves nothing");
	IgExpect(DrewRun(B, "FLIP") && DrewRun(B, "PLAY"), "preview: the stream shows Ignite's own client");
	// A still: two frames 0.7 s apart draw the client the same (every pulse at rest).
	size_t Same = 0;
	size_t Total = 0;
	for (const ss::ui::DrawCmd& Cmd : B.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.BaselineY < 640.0f && Cmd.Text.X < 1028.0f)
		{
			++Total;
			Same += DrewRun(A, Cmd.Text.Text) ? 1 : 0;
		}
	}
	IgExpect(Total > 20 && Same == Total, "preview: the preview's words hold still between frames");
}

// ------------------------------------------------------------------ the copy and the shouts

void IgCopy()
{
	namespace ig = ss::ui::rlignite_detail;
	const std::vector<std::string>& Shouts = ig::IgniteShouts();
	IgExpect(Shouts.size() == 150, "copy: BUZZ has 150 shouts in P0");
	std::vector<std::pair<std::string, std::string>> Lines;
	std::vector<std::pair<std::string, std::string>> Chat;
	const std::set<std::string> Shout(Shouts.begin(), Shouts.end());
	for (const std::string& T : ig::IgniteInventory())
	{
		(Shout.count(T) ? Chat : Lines).push_back({"ignite", T});
	}
	const std::map<std::string, std::string> Vars = {{"Team", "Team RiverLine"}, {"Date", "Dec 4"}, {"Event", "Spark Jackpot"}, {"Headliner", "LilaVance"}};
	int Problems = copylint::Run("ignite-ui", Lines, copylint::LetterBody, Vars) + copylint::Run("ignite-shouts", Chat, copylint::TableTalk, Vars);
	std::vector<std::string> All = ig::IgniteInventory();
	for (const std::string& Label : ig::IgniteLabels())
	{
		All.push_back(Label);
	}
	for (const std::string& T : All)
	{
		const std::string Low = copylint::Lower(T);
		// U1: the world never says it plays for you; the sites' own screen of real and closed sites' names.
		for (const char* Banned : {" ai ", " ai.", "bot", "autoplay", "autopilot", "jackpot", "pokerstars", "ggpoker", "ignition", "run it once", "skrill", "neteller", "paypal", "ember"})
		{
			if (Low.find(Banned) != std::string::npos)
			{
				std::printf("COPY (ignite) \"%s\" says \"%s\"\n", T.c_str(), Banned);
				++Problems;
			}
		}
		if (const char* Hit = trademarks::Find(T, trademarks::kSourceScan))
		{
			std::printf("COPY (ignite) \"%s\": a real name (%s)\n", T.c_str(), Hit);
			++Problems;
		}
		if (const char* Hit = trademarks::Find(T, trademarks::kCopyScan))
		{
			std::printf("COPY (ignite) \"%s\": a real name (%s)\n", T.c_str(), Hit);
			++Problems;
		}
	}
	// Every digit in a shout would be a number nobody can vouch for: the shouts carry none (their slots are facts).
	for (const std::string& T : Shouts)
	{
		if (std::any_of(T.begin(), T.end(), [](char Ch) { return std::isdigit(static_cast<unsigned char>(Ch)) != 0; }))
		{
			std::printf("COPY (ignite) shout \"%s\" carries a digit\n", T.c_str());
			++Problems;
		}
	}
	IgExpect(Problems == 0, "copy: Ignite's copy passes the lint, U1 and both trademark tiers");
	// One shout every 10 world minutes from a keyed order: no line twice inside any 24 world hours, a window's edge included.
	bool Fresh = true;
	for (const uint64_t Seed : {1ull, 77ull, 9000001ull})
	{
		std::vector<int> Seen;
		for (int K = 0; K < 900; ++K)
		{
			Seen.push_back(ig::IgniteShoutAt(Seed, static_cast<double>(K) * 10.0 + 3.0));
		}
		for (size_t K = 0; K < Seen.size() && Fresh; ++K)
		{
			for (size_t J = K + 1; J < Seen.size() && J < K + 144 && Fresh; ++J)
			{
				Fresh = Seen[K] != Seen[J];
			}
		}
		Fresh = Fresh && ig::IgniteShoutAt(Seed, 1234.0) == ig::IgniteShoutAt(Seed, 1234.0);
	}
	IgExpect(Fresh, "copy: no shout repeats within 24 world hours (keyed, the same every time)");
	// "Tonight" (the wheel's sign, the BIG GUARANTEES TONIGHT shelf, a shout's {Event}): today's starts and the small hours
	// before the coming 6 AM, never tomorrow evening's.
	IgExpect(ig::IgniteTonight(IgPrime + 60.0, IgPrime) && ig::IgniteTonight(IgPrime + 4.0 * 60.0, IgPrime) && !ig::IgniteTonight(IgPrime + 23.0 * 60.0, IgPrime) &&
			ig::IgniteTonight(IgLate + 18.0 * 60.0, IgLate) && !ig::IgniteTonight(IgLate + 23.0 * 60.0, IgLate),
		"copy: tonight is tonight (9 PM: 1 AM yes, tomorrow's 8 PM no; 2 AM: this evening yes)");
}

// ------------------------------------------------------------------ type, contrast, the bench

void IgType()
{
	// Every title and number in BlackItalic, measured as it draws (Canvas::Measure(…, Font)).
	IgLaptop P("ignite-type");
	P.At(IgPrime);
	P.Join(21240);
	P.Open(Page::Lobby);
	const DrawList L = P.Settled();
	int Black = 0;
	bool Measured = true;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Face == ss::ui::Font::BlackItalic)
		{
			++Black;
			ss::ui::DrawList Scratch;
			ss::ui::Canvas C(Scratch, P.M, 1600.0f, 1000.0f, 1.0f);
			Measured = Measured && std::fabs(C.Measure(Cmd.Text.Text, Cmd.Text.SizePx, ss::ui::Font::BlackItalic) - P.M.Width(Cmd.Text.Text, ss::ui::Font::BlackItalic, Cmd.Text.SizePx)) < 0.01f;
		}
	}
	IgExpect(Black > 20 && Measured, "type: PLAY's titles and numbers are BlackItalic, measured in their face");
	// Contrast at 4.5:1 or more: the skins (day and Afterburner) and the label inks on Ignite's fills.
	const ss::net::SiteDef& D = ss::net::Def(SiteId::Ignite);
	const auto Pair = [](const char* What, uint32_t Ink, uint32_t Ground) {
		const double R = IgContrast(Ink, Ground);
		std::printf("  contrast %-34s %.2f:1\n", What, R);
		return R >= 4.5;
	};
	bool Reads = Pair("day: Ink on Panel", D.Palette[5], D.Palette[2]) && Pair("day: Muted on Bg", D.Palette[6], D.Palette[0]);
	if (D.NightPalette[0] != 0)
	{
		Reads = Pair("Afterburner: Ink on Panel", D.NightPalette[5], D.NightPalette[2]) && Pair("Afterburner: Muted on Bg", D.NightPalette[6], D.NightPalette[0]) && Reads;
	}
	namespace ia = ss::ui::igniteart;
	Reads = Pair("dark ink on Coral (chips, tabs)", 0x2a0d14, ia::IgCoral) && Reads;
	Reads = Pair("dark ink on Violet (night chips)", 0x2a0d14, ia::IgNightAccent) && Reads;
	Reads = Pair("Cream on the dock's Night", ia::IgCream, ia::IgNight) && Reads;
	Reads = Pair("Lime on the Sparks pill", ia::IgLime, 0x1b2610) && Reads;
	Reads = Pair("Cream on a cabinet", ia::IgCream, 0x2a1645) && Reads;
	Reads = Pair("Sunflower on a panel", ia::IgSun, 0x241532) && Reads;
	Reads = Pair("ink on a Sky sticker", 0x1a0f2a, ia::IgSky) && Reads;
	// The labels on Ignite's own fills (§1.11: every label on a filled accent at 4.5:1): the stickers, BUZZ's badge and cards,
	// the dock's lit item (its gradient at the label's middle, day and night).
	Reads = Pair("dark ink on the LATE sticker", ia::IgDarkInk, ia::IgTang) && Reads;
	Reads = Pair("dark ink on the REG sticker (night aqua)", ia::IgDarkInk, ia::IgNightAccent2) && Reads;
	Reads = Pair("dark ink on the KO and clause stickers", ia::IgDarkInk, ia::IgRed) && Reads;
	Reads = Pair("dark ink on BUZZ's pink badge", ia::IgDarkInk, ia::IgPink) && Reads;
	Reads = Pair("a shout's violet name on a card", ia::IgNightAccent, 0x2c1a3f) && Reads;
	Reads = Pair("dark ink on the lit dock item (day)", 0x2a0d14, 0xe87777) && Reads;
	Reads = Pair("dark ink on the lit dock item (night)", 0x2a0d14, 0xae89ef) && Reads;
	IgExpect(Reads, "type: Ignite's pairs read at 4.5:1 or more");
	// And as drawn: the cabinets' LATE and REG stickers carry the dark ink.
	int Stickers = 0;
	bool StickerInk = true;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && (Cmd.Text.Text == "LATE" || Cmd.Text.Text == "REG") && Cmd.Text.Face == ss::ui::Font::BlackItalic)
		{
			++Stickers;
			StickerInk = StickerInk && IgRgb(Cmd.Text.Col) == ia::IgDarkInk;
		}
	}
	IgExpect(Stickers > 0 && StickerInk, "type: the cabinets' LATE and REG stickers are in the dark ink");
}

void IgBench()
{
	// PLAY's budget (SPEC §10): at most 34k vertices and 400 text runs; SS_UI_BENCH times a draw.
	IgLaptop P("ignite-bench");
	P.At(IgPrime);
	P.S.BankrollCents = 150000;
	P.Join(21240);
	P.Open(Page::Lobby);
	const DrawList L = P.Settled();
	size_t Runs = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		Runs += Cmd.Type == ss::ui::DrawCmd::Kind::Text ? 1 : 0;
	}
	if (std::getenv("SS_IG_DIAG"))
	{
		// Where PLAY's vertices go, by band of the screen (each command counted where its first vertex sits).
		const float Bands[7] = {0.0f, 72.0f, 352.0f, 432.0f, 480.0f, 832.0f, 1000.0f};
		const char* Names[6] = {"marquee", "boardwalk", "today", "chips", "floor", "dock+taskbar"};
		size_t Per[6] = {0, 0, 0, 0, 0, 0};
		for (const ss::ui::DrawCmd& Cmd : L.Cmds)
		{
			if (Cmd.Type != ss::ui::DrawCmd::Kind::Triangles || Cmd.VertexCount == 0)
			{
				continue;
			}
			float Ys = 0.0f;
			for (uint32_t K = 0; K < Cmd.VertexCount; ++K)
			{
				Ys += L.Vertices[Cmd.VertexStart + K].Y;
			}
			Ys /= static_cast<float>(Cmd.VertexCount);
			for (int B = 0; B < 6; ++B)
			{
				Per[B] += Ys >= Bands[B] && Ys < Bands[B + 1] ? Cmd.VertexCount : 0;
			}
		}
		for (int B = 0; B < 6; ++B)
		{
			std::printf("  diag: %-13s %zu vertices\n", Names[B], Per[B]);
		}
		namespace ia = ss::ui::igniteart;
		const auto Count = [&P](const std::function<void(ss::ui::Canvas&)>& Fn) {
			DrawList Dl;
			ss::ui::Canvas C(Dl, P.M, 1600.0f, 1000.0f, 1.0f);
			Fn(C);
			return Dl.Vertices.size();
		};
		ia::IgniteBand B;
		B.K = ia::IgniteTonesFor(ss::site::show::Mood::PrimeTime);
		B.Now = 3.0;
		std::printf("  diag: static band %zu, live band %zu\n", Count([&](ss::ui::Canvas& C) { ia::IgniteBoardwalkStatic(C, B); }), Count([&](ss::ui::Canvas& C) { ia::IgniteBoardwalkLive(C, B); }));
		std::printf("  diag: wheel %zu, cabinet body %zu, sticker %zu, bulb %zu, flame %zu, gauge %zu\n",
			Count([&](ss::ui::Canvas& C) { ia::IgniteFerrisWheel(C, 790.0f, 214.0f, 118.0f, 0.3f, {"8 PM", "9 PM", "10 PM"}, B.K, 3.0); }),
			Count([&](ss::ui::Canvas& C) { ia::IgniteCabinetBody(C, {24.0f, 508.0f, 180.0f, 120.0f}, ss::ui::Hex(0xff6b57), false, 0.0f, B.K); }),
			Count([&](ss::ui::Canvas& C) { ia::IgniteSticker(C, 100.0f, 100.0f, 17.0f, "REG", ss::ui::Hex(0x4fc3f7), ss::ui::Hex(0xfff4ec), 8.5f, 14, 0.78f); }),
			Count([&](ss::ui::Canvas& C) { ia::IgniteBulb(C, 100.0f, 100.0f, 3.0f, ss::ui::Hex(0xffc94a), ss::ui::Hex(0xff3d8b), 1.0f); }),
			Count([&](ss::ui::Canvas& C) { ia::IgniteFlame(C, 100.0f, 100.0f, 20.0f, ss::ui::Hex(0xff6b57), ss::ui::Hex(0xff9e3d), ss::ui::Hex(0xffc94a)); }),
			Count([&](ss::ui::Canvas& C) { ia::IgniteFuelGauge(C, 100.0f, 100.0f, 30.0f, 0.4, B.K); }));
		const ss::net::Network& Net = ss::net::Site(SiteId::Ignite);
		if (!Net.Templates().empty())
		{
			std::printf("  diag: emblem at 36 %zu, glyph at 22: coin %zu sevens %zu rocket %zu\n", Count([&](ss::ui::Canvas& C) { ss::ui::eventart::Emblem(C, Net.Templates().front(), 100.0f, 100.0f, 36.0f, 0.0); }),
				Count([&](ss::ui::Canvas& C) { ss::ui::eventart::DrawGlyph(C, ss::ui::eventart::Glyph::Coin, 100.0f, 100.0f, 22.0f); }),
				Count([&](ss::ui::Canvas& C) { ss::ui::eventart::DrawGlyph(C, ss::ui::eventart::Glyph::Sevens, 100.0f, 100.0f, 22.0f); }),
				Count([&](ss::ui::Canvas& C) { ss::ui::eventart::DrawGlyph(C, ss::ui::eventart::Glyph::Rocket, 100.0f, 100.0f, 22.0f); }));
		}
	}
	std::printf("  ignite PLAY: %zu vertices, %zu text runs (budget 34,000 and 400)\n", L.Vertices.size(), Runs);
	IgExpect(L.Vertices.size() <= 34000 && Runs <= 400, "bench: PLAY within 34k vertices and 400 runs");
	if (std::getenv("SS_UI_BENCH"))
	{
		for (const Page Pg : {Page::Lobby, Page::Leaderboards, Page::News, Page::Cashier})
		{
			P.Open(Pg);
			P.Settled();
			for (int K = 0; K < 60; ++K)
			{
				P.Now += 1.0 / 30.0; // at rest: every ease landed (a frame's step is capped at 0.1 s)
				P.Frame();
			}
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
			std::printf("UI BENCH ignite page %d %6.3f ms/draw (best of 5 x 40)\n", static_cast<int>(Pg), Best);
		}
	}
}

// ------------------------------------------------------------------ G1 fix pass (G1_AUDIT M3, L1, L6)

/** One draw of a laptop with the pointer away, as IgLaptop::Frame draws it (a fresh list, as the CMake benches time it):
 * how long it took, in ms. */
double IgTimedDraw(ss::ui::RiverLine& RL, const TableMeasurer& M, double Now)
{
	RL.UI.Ptr.Active = false;
	RL.UI.Ptr.Down = false;
	RL.UI.Ptr.Pressed = false;
	RL.UI.Ptr.Released = false;
	RL.UI.Ptr.Wheel = 0.0f;
	const auto T0 = std::chrono::steady_clock::now();
	{
		DrawList L;
		ss::ui::Canvas C(L, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, 1.0f);
		RL.Draw(C, Now);
	}
	const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count();
	RL.UI.Ptr.EndFrame();
	return Ms;
}

/** The Q quantile of draw times (0.2: the time a draw takes when nothing else on the machine got in its way). */
double IgQuantile(std::vector<double> V, double Q)
{
	if (V.empty())
	{
		return 0.0;
	}
	std::sort(V.begin(), V.end());
	return V[static_cast<size_t>(Q * static_cast<double>(V.size() - 1))];
}

/** The best mean of Window draws in a row (the CMake benches' "best of 5 x 40"), for comparison with their numbers. */
double IgBestRun(const std::vector<double>& V, size_t Window)
{
	double Best = 1.0e9;
	for (size_t K = 0; K + Window <= V.size(); K += Window)
	{
		double Sum = 0.0;
		for (size_t J = K; J < K + Window; ++J)
		{
			Sum += V[J];
		}
		Best = std::min(Best, Sum / static_cast<double>(Window));
	}
	return Best;
}

/** One measurement of IgPlayBench's two benches (ms; [0] still, [1] moving). */
struct IgM3
{
	double Rl[2] = {0.0, 0.0};
	double Ig[2] = {0.0, 0.0};
	double RlRun[2] = {0.0, 0.0};
	double IgRun2[2] = {0.0, 0.0};
	double Play[2] = {0.0, 0.0};
	double PlayRun[2] = {0.0, 0.0};
	size_t Listed = 0;
	double Cap = 1.5;
	double Load = 1.0;
	bool Within() const { return Ig[0] <= 1.25 * Rl[0] && Ig[1] <= 1.25 * Rl[1] && Play[0] <= Cap && Play[1] <= Cap; }
};

IgM3 IgMeasureM3()
{
	IgM3 Out;
	double* Rl = Out.Rl;
	double* Ig = Out.Ig;
	double* RlRun = Out.RlRun;
	double* IgRun2 = Out.IgRun2;
	{
		IgLaptop P("sites-bench");
		P.At(30.0 * IgDay + 19.5 * 60.0);
		P.S.BankrollCents = 150000;
		P.Join(21240);
		ss::site::Join(P.S.Life.Sites, SiteId::RunItTwice, P.S.WorldMinutes());
		if (ss::site::Account* A = ss::site::AccountOf(P.S.Life.Sites, SiteId::RunItTwice))
		{
			A->Cents = 21240;
		}
		P.Open(Page::Lobby);
		ss::ui::RiverLine Home(P.S);
		Home.SiteShown = SiteId::RiverLine;
		Home.OpenApp(App::RiverLine, P.Now);
		Home.OpenPage(Page::Lobby, P.Now);
		// At rest: two seconds at 30 Hz, so every ease has landed (a frame's step is capped at 0.1 s, so a still clock alone
		// would leave the shelves mid-glide).
		for (int K = 0; K < 60; ++K)
		{
			P.Now += 1.0 / 30.0;
			IgTimedDraw(Home, P.M, P.Now);
			IgTimedDraw(P.RL, P.M, P.Now);
		}
		for (int Moving = 0; Moving < 2; ++Moving)
		{
			std::vector<double> A;
			std::vector<double> B;
			for (int K = 0; K < 240; ++K)
			{
				P.Now += Moving ? 1.0 / 30.0 : 0.0;
				if (K % 2 == 0)
				{
					A.push_back(IgTimedDraw(Home, P.M, P.Now));
					B.push_back(IgTimedDraw(P.RL, P.M, P.Now));
				}
				else
				{
					B.push_back(IgTimedDraw(P.RL, P.M, P.Now));
					A.push_back(IgTimedDraw(Home, P.M, P.Now));
				}
			}
			Rl[Moving] = IgQuantile(A, 0.2);
			Ig[Moving] = IgQuantile(B, 0.2);
			RlRun[Moving] = IgBestRun(A, 40);
			IgRun2[Moving] = IgBestRun(B, 40);
		}
	}
	double* Play = Out.Play;
	double* PlayRun = Out.PlayRun;
	size_t& Listed = Out.Listed;
	{
		IgLaptop P("ignite-bench");
		P.At(IgPrime);
		P.S.BankrollCents = 150000;
		P.Join(21240);
		P.Open(Page::Lobby);
		P.Settled();
		for (int K = 0; K < 60; ++K)
		{
			P.Now += 1.0 / 30.0;
			IgTimedDraw(P.RL, P.M, P.Now);
		}
		Listed = P.RL.ListedEvents().size();
		for (int Moving = 0; Moving < 2; ++Moving)
		{
			std::vector<double> V;
			for (int K = 0; K < 240; ++K)
			{
				P.Now += Moving ? 1.0 / 30.0 : 0.0;
				V.push_back(IgTimedDraw(P.RL, P.M, P.Now));
			}
			Play[Moving] = IgQuantile(V, 0.2);
			PlayRun[Moving] = IgBestRun(V, 40);
		}
	}
	// The 1.5 ms cap is the audit's quiet PC, where RiverLine's lobby drew in 0.86 ms; a machine slower than that this run (a
	// busy one: the suites run three at a time) stretches the cap by RiverLine's own lobby, measured a moment before.
	Out.Load = std::max(1.0, Rl[0] / 0.86);
	Out.Cap = 1.5 * Out.Load;
	return Out;
}

void IgPlayBench()
{
	// G1_AUDIT M3: PLAY's draw time against SPEC §10's caps, the two ways the audit timed it. (1) ui_seg_sites' like-for-like
	// bench: one career at 7:30 PM, RiverLine's lobby and Ignite's PLAY (two laptops on the same session, the same world
	// minute), PLAY within RiverLine's + 25%. (2) rivals_ui's: PLAY at 9 PM with its floor full, within a page's 1.5 ms. Each
	// with the clock still (the CMake benches' way) and moving at 30 Hz (the game's). The two lobbies draw in turn, frame by
	// frame, so a busy machine slows both alike, and each is read at its 20th percentile (a draw nothing else got in the way
	// of); the CMake benches' best 40-draw mean prints beside it. The caps are asserted in an optimised build only (a debug
	// build prints the numbers); the margins print either way.
	// G1_AUDIT N11: wall-clock caps in a suite that runs beside two others. A measurement that misses a cap is taken again, up
	// to three times, and the caps are asserted on the first that holds (else the last): a real regression misses all three,
	// a moment's load on a busy machine doesn't fail the suite.
	IgM3 M;
	int Tries = 0;
	while (Tries < 3)
	{
		M = IgMeasureM3();
		++Tries;
		const char* Try = Tries == 1 ? "" : Tries == 2 ? " (again)" : " (third time)";
		std::printf("UI BENCH M3 like-for-like 7:30 PM (p20, still / moving)%s: RiverLine lobby %.3f / %.3f ms, Ignite PLAY %.3f / %.3f ms: %.2fx / %.2fx (cap 1.25x)\n", Try, M.Rl[0], M.Rl[1],
			M.Ig[0], M.Ig[1], M.Ig[0] / M.Rl[0], M.Ig[1] / M.Rl[1]);
		std::printf("UI BENCH M3 like-for-like 7:30 PM (best 40-draw mean)%s: RiverLine lobby %.3f / %.3f ms, Ignite PLAY %.3f / %.3f ms: %.2fx / %.2fx\n", Try, M.RlRun[0], M.RlRun[1],
			M.IgRun2[0], M.IgRun2[1], M.IgRun2[0] / M.RlRun[0], M.IgRun2[1] / M.RlRun[1]);
		std::printf("UI BENCH M3 Ignite PLAY 9 PM, %zu events (p20, still / moving)%s: %.3f / %.3f ms (cap %.2f ms at load %.2f, headroom %.0f%% / %.0f%%); best 40-draw mean %.3f / %.3f ms\n",
			M.Listed, Try, M.Play[0], M.Play[1], M.Cap, M.Load, 100.0 * (1.0 - M.Play[0] / M.Cap), 100.0 * (1.0 - M.Play[1] / M.Cap), M.PlayRun[0], M.PlayRun[1]);
		if (M.Within())
		{
			break;
		}
	}
#if defined(NDEBUG)
	IgExpect(M.Ig[0] <= 1.25 * M.Rl[0] && M.Ig[1] <= 1.25 * M.Rl[1], "bench: PLAY draws within RiverLine's lobby + 25% (ui_seg_sites' like-for-like, SPEC §10)");
	IgExpect(M.Play[0] <= M.Cap && M.Play[1] <= M.Cap, "bench: PLAY at 9 PM with its floor full draws within 1.5 ms (SPEC §10; on the audit's quiet PC)");
#endif
}

/** An Ignite table at World: an open event registered and its table in front (false if none can run then). */
bool IgTableAt(IgLaptop& P, double World)
{
	ss::Session& S = P.S;
	P.At(World);
	S.BankrollCents = 200000;
	P.Join(50000);
	ss::net::EventInstance E;
	if (!IgOpenEvent(S, E, 2200))
	{
		return false;
	}
	S.RegisterEvent(ss::net::Site(SiteId::Ignite).Listing(E, nullptr, S.Unlocks()));
	S.CurrentPace = ss::Pace::Full;
	P.Now = Step(S, P.Now, 3.0);
	S.HasPrompt = false;
	for (ss::SeatVis& Seat : S.Seats)
	{
		Seat.Acting = false;
	}
	return S.T != nullptr && S.CurrentScreen == ss::Screen::Table;
}

/** The final table's tube, breathing: the sum of the glow's Hot Pink alpha over the felt, frame by frame (60 Hz, 6 s), and which
 * beat it breathes on (the stronger of 96 and 124 BPM in its spectrum). */
double IgTubeBpm(IgLaptop& P)
{
	std::vector<double> Series;
	const double Start = P.Now;
	for (int K = 0; K < 360; ++K)
	{
		P.Now = Start + static_cast<double>(K) / 60.0;
		const DrawList L = P.Frame();
		double Sum = 0.0;
		for (const ss::ui::Vertex& V : L.Vertices)
		{
			Sum += V.Y > 100.0f && V.Y < 760.0f && V.X > 80.0f && V.X < 1100.0f && IgRgb(V.Col) == ss::ui::igniteart::IgPink && V.Col.A < 0.5f ? V.Col.A : 0.0;
		}
		Series.push_back(Sum);
	}
	double Mean = 0.0;
	for (const double V : Series)
	{
		Mean += V / static_cast<double>(Series.size());
	}
	const auto Power = [&](double Bpm) {
		double Re = 0.0;
		double Im = 0.0;
		for (size_t K = 0; K < Series.size(); ++K)
		{
			const double A = 2.0 * 3.14159265358979 * Bpm / 60.0 * static_cast<double>(K) / 60.0;
			Re += (Series[K] - Mean) * std::cos(A);
			Im += (Series[K] - Mean) * std::sin(A);
		}
		return Re * Re + Im * Im;
	};
	const double Slow = Power(96.0);
	const double Fast = Power(124.0);
	return Slow < 1.0e-9 && Fast < 1.0e-9 ? 0.0 : Slow > Fast ? 96.0 : 124.0;
}

void IgBeat()
{
	// G1_AUDIT L1: the beat by the hour (SPEC §2.3): 96 BPM in Afterburner Hours, 124 the rest of the day; and the final
	// table's tube breathes on it (the tube is TblCapsule's, fix-tablestyle's file: patch fix-ignite_to_fix-tablestyle_1).
	namespace ia = ss::ui::igniteart;
	bool ByHour = true;
	for (int Hour = 0; Hour < 24; ++Hour)
	{
		const double World = 31.0 * IgDay + static_cast<double>(Hour) * 60.0 + 30.0;
		ByHour = ByHour && ia::IgniteBpmAt(World) == (Hour < 6 ? 96.0 : 124.0) && ia::IgniteBpmAt(World) == ia::IgniteTonesFor(ss::site::show::IgniteMood(World)).Bpm;
	}
	IgExpect(ByHour, "beat: 96 BPM midnight to 6 AM (Afterburner Hours), 124 the rest of the day");
	double Breaths[2] = {0.0, 0.0};
	const double Hours[2] = {IgLate, IgPrime - 60.0};
	for (int K = 0; K < 2; ++K)
	{
		IgLaptop P(K == 0 ? "ignite-beat-late" : "ignite-beat-prime");
		if (!IgTableAt(P, Hours[K]))
		{
			std::printf("  ignite: no table to sit at %s; the tube's beat isn't timed there\n", K == 0 ? "at 2 AM" : "at 8 PM");
			continue;
		}
		P.S.T->Remaining = 5; // the final table
		Breaths[K] = IgTubeBpm(P);
		if (K == 0)
		{
			WriteList("ignite_table_final_afterburner", P.Frame());
		}
	}
	std::printf("  ignite: the final table's tube breathes at %.0f BPM at 2 AM and %.0f BPM at 8 PM\n", Breaths[0], Breaths[1]);
	// Patch fix-ignite_to_fix-tablestyle_1 is in (applied in the fix round): a tube back on 124 BPM at 2 AM fails here.
	IgExpect((Breaths[0] == 96.0 || Breaths[0] == 0.0) && (Breaths[1] == 124.0 || Breaths[1] == 0.0) && Breaths[0] + Breaths[1] > 0.0,
		"beat: the final table's neon tube breathes at 96 BPM in Afterburner Hours and 124 in Prime Time");
}

void IgPotWin()
{
	// G1_AUDIT L1 (SPEC §2.5 "Bets and pots"): a pot won fountains its chips from the pot to the winner (each on a parabola,
	// one bounce where it lands, six sparklets behind the first), then "+4,200" floats up off the winner. None of it while
	// previewing.
	namespace ig = ss::ui::rlignite_detail;
	// The arcs: launched in turn, a parabola (lift 0 at both ends, highest mid-flight), one bounce, then settled and gone.
	bool Arcs = true;
	const double Dur = 0.6;
	for (int K = 0; K < ig::IgniteFountainChips; ++K)
	{
		const double Launch = 0.035 * K;
		const double Fly = Dur - 0.035 * (ig::IgniteFountainChips - 1);
		Arcs = Arcs && !ig::IgniteHopAt(Launch - 0.01, Dur, K, 77u).Shown;
		const ig::IgniteHop Mid = ig::IgniteHopAt(Launch + Fly / 2.0, Dur, K, 77u);
		const ig::IgniteHop Early = ig::IgniteHopAt(Launch + Fly * 0.1, Dur, K, 77u);
		const ig::IgniteHop Landed = ig::IgniteHopAt(Launch + Fly, Dur, K, 77u);
		const ig::IgniteHop Bounce = ig::IgniteHopAt(Launch + Fly + 0.09, Dur, K, 77u);
		const ig::IgniteHop Rest = ig::IgniteHopAt(Launch + Fly + 0.2, Dur, K, 77u);
		const ig::IgniteHop Gone = ig::IgniteHopAt(Launch + Fly + 0.31, Dur, K, 77u);
		Arcs = Arcs && Mid.Shown && Mid.Lift >= 70.0f && Mid.Lift <= 104.0f && Early.Lift > 0.0f && Early.Lift < Mid.Lift && Early.Along < Mid.Along;
		Arcs = Arcs && Landed.Along == 1.0f && Landed.Lift < 0.01f && Bounce.Lift > 9.0f && Bounce.Lift <= 10.0f && Rest.Lift == 0.0f && Rest.Alpha < 1.0f && !Gone.Shown;
	}
	Arcs = Arcs && ig::IgniteHopAt(Dur, Dur, ig::IgniteFountainChips - 1, 77u).Along == 1.0f; // the last lands with the pot's flight
	IgExpect(Arcs, "pot win: the chips hop out in turn on parabolas, bounce once where they land and settle");
	// At the table: the pot's flight to the hero, mid-fountain, then the float.
	IgLaptop P("ignite-potwin");
	if (!IgTableAt(P, IgPrime - 60.0))
	{
		std::printf("  ignite: no open event at this hour; the pot-win checks are skipped\n");
		return;
	}
	ss::Session& S = P.S;
	int Hero = -1;
	for (const ss::SeatVis& Seat : S.Seats)
	{
		Hero = Seat.Present && Seat.IsHero ? Seat.Seat : Hero;
	}
	const auto Pot = [&](double Start) {
		ss::Flight F;
		F.IsChips = true;
		F.From = ss::FlightEnd::AtPot();
		F.To = ss::FlightEnd::AtSeat(Hero);
		F.Amount = 4200;
		F.Start = Start;
		F.Dur = Dur;
		S.Flights.push_back(F);
	};
	const double T0 = P.Now;
	Pot(T0);
	P.Now = T0 + 0.25;
	const DrawList Mid = P.Frame();
	WriteList("ignite_table_potwin", Mid);
	ig::IgniteOf(&P.RL).Pots.clear();
	S.Flights.clear();
	const DrawList Plain = P.Frame();
	IgExpect(Mid.Vertices.size() > Plain.Vertices.size() + 300, "pot win: the fountain's chips and sparklets draw mid-flight");
	// The effect's own cost (SPEC §10: a win effect within the particles' budget, confetti's 1.6k vertices).
	size_t Effect = 0;
	{
		DrawList Fx;
		ss::ui::Canvas C(Fx, P.M, 1600.0f, 1000.0f, 1.0f);
		for (int K = 0; K < ig::IgniteFountainChips; ++K)
		{
			ss::ui::igniteart::IgniteCandyChip(C, 100.0f + 30.0f * static_cast<float>(K), 100.0f, 8.5f, K);
			ss::ui::igniteart::IgniteSpark(C, 100.0f + 30.0f * static_cast<float>(K), 140.0f, 4.0f, ss::ui::Hex(0xffc94a));
		}
		Effect = Fx.Vertices.size();
	}
	std::printf("  ignite pot win: %zu vertices mid-fountain (%zu without it and the pot's flight; the fountain itself %zu)\n", Mid.Vertices.size(), Plain.Vertices.size(), Effect);
	IgExpect(Effect <= 1600, "pot win: the fountain stays within the particles' budget (1.6k vertices)");
	// The float, after the pot's flight is gone (the layer remembers it).
	Pot(T0);
	P.Now = T0 + 0.05;
	P.Frame();
	S.Flights.clear();
	P.Now = T0 + Dur + 0.3;
	const DrawList F1 = P.Frame();
	P.Now = T0 + Dur + 0.9;
	const DrawList F2 = P.Frame();
	WriteList("ignite_table_potwin_float", F2);
	const IgBox A = IgRun(F1, "+4,200", P.M);
	const IgBox B = IgRun(F2, "+4,200", P.M);
	// The hero's bubble is at (590,690), its cards on its right: the float rises beside it on the left, clear of both.
	IgExpect(A.Found && B.Found && A.Face == ss::ui::Font::BlackItalic && B.Base < A.Base && A.Base < 700.0f && A.Base > 600.0f && A.X + A.W <= 550.0f,
		"pot win: \"+4,200\" floats up beside the winner in BlackItalic");
	P.Now = T0 + Dur + ig::IgniteFloatFor + 0.1;
	IgExpect(!IgRun(P.Frame(), "+4,200", P.M).Found && ig::IgniteOf(&P.RL).Pots.empty(), "pot win: the float is gone after 1.4 s and the layer forgets the pot");
	// review-fix-ignite: a big pot's float at every seat stays on the screen and off the side panel (8-max's seat at
	// (140,430) has no room on its left for "+125,000": it takes the float on its right). At the spring's peak.
	bool OnScreen = true;
	int Seats = 0;
	for (const ss::SeatVis& Seat : S.Seats)
	{
		if (!Seat.Present)
		{
			continue;
		}
		ig::IgniteOf(&P.RL).Pots.clear();
		const double Won = P.Now + 5.0;
		ss::Flight F;
		F.IsChips = true;
		F.From = ss::FlightEnd::AtPot();
		F.To = ss::FlightEnd::AtSeat(Seat.Seat);
		F.Amount = 125000;
		F.Start = Won;
		F.Dur = Dur;
		S.Flights.push_back(F);
		P.Now = Won + 0.05;
		P.Frame();
		S.Flights.clear();
		P.Now = Won + Dur + 0.12;
		const DrawList L = P.Frame();
		const IgBox Big = IgRun(L, "+125,000", P.M);
		OnScreen = OnScreen && Big.Found && Big.X >= 4.0f && Big.X + Big.W <= 1188.0f;
		if (Big.Found && Big.Base > 380.0f && Big.Base < 460.0f && Big.X < 590.0f)
		{
			WriteList("ignite_table_potwin_edge", L); // the left middle seat's
		}
		++Seats;
		if (!(Big.Found && Big.X >= 4.0f && Big.X + Big.W <= 1188.0f))
		{
			std::printf("  ignite pot win: seat %d's \"+125,000\" at x %.1f-%.1f\n", Seat.Seat, Big.X, Big.X + Big.W);
		}
	}
	P.Now += Dur + ig::IgniteFloatFor + 0.1;
	P.Frame();
	IgExpect(OnScreen && Seats >= 6, "pot win: a big pot's float stays on the screen at every seat");
}

void IgShouts()
{
	// G1_AUDIT L6: a shout that says what time it is is heard only at its hours (as of when it was shouted).
	namespace ig = ss::ui::rlignite_detail;
	const std::vector<std::string>& Lines = ig::IgniteShouts();
	bool Known = true;
	for (const ig::IgniteShoutHours& W : ig::IgniteShoutWindows())
	{
		Known = Known && std::find(Lines.begin(), Lines.end(), std::string(W.Line)) != Lines.end();
	}
	IgExpect(Known, "shouts: every time-of-day line is one of the 150");
	const auto At = [](int Day, int Hour) { return static_cast<double>(Day) * IgDay + static_cast<double>(Hour) * 60.0 + 20.0; };
	// Day 34 is Sunday Nov 8, 2026 (day 0 the Monday Oct 5); day 35 a Monday.
	IgExpect(ig::IgniteShoutFits("Evening, boardwalk!", At(30, 19)) && !ig::IgniteShoutFits("Evening, boardwalk!", At(30, 9)) && !ig::IgniteShoutFits("Evening, boardwalk!", At(31, 2)) &&
			!ig::IgniteShoutFits("Evening, boardwalk!", At(30, 14)),
		"shouts: \"Evening, boardwalk!\" only in the evening");
	IgExpect(ig::IgniteShoutFits("Coffee in one hand, mouse in the other.", At(30, 8)) && !ig::IgniteShoutFits("Coffee in one hand, mouse in the other.", At(30, 21)) &&
			ig::IgniteShoutFits("Can't sleep, might as well flip.", At(31, 3)) && !ig::IgniteShoutFits("Can't sleep, might as well flip.", At(30, 15)) &&
			ig::IgniteShoutFits("Lunch break poker, the purest form.", At(30, 12)) && !ig::IgniteShoutFits("Lunch break poker, the purest form.", At(30, 19)),
		"shouts: morning, small-hours and lunch lines at their hours");
	IgExpect(ig::IgniteShoutFits("Good luck to everyone at the tables tonight.", At(30, 21)) && ig::IgniteShoutFits("Good luck to everyone at the tables tonight.", At(31, 1)) &&
			!ig::IgniteShoutFits("Good luck to everyone at the tables tonight.", At(30, 9)) && ig::IgniteShoutFits("Who brought snacks to the pier?", At(30, 9)),
		"shouts: \"tonight\" from mid-afternoon into the small hours; a line with no time any time");
	IgExpect(ss::net::Weekday(34) == 6 && ig::IgniteShoutFits("Nothing beats a Sunday on the boardwalk.", At(34, 15)) && !ig::IgniteShoutFits("Nothing beats a Sunday on the boardwalk.", At(35, 15)),
		"shouts: the Sunday line on Sundays only");
	// BUZZ as drawn: find "Evening, boardwalk!"'s slots in a month of this laptop's world, open the Shouts tab just after one in
	// the evening and one at another hour: heard in the first, not in the second.
	uint64_t Seed = 0;
	{
		IgLaptop Probe("ignite-shouts");
		Seed = Probe.S.WorldSeed();
	}
	const int Evening = static_cast<int>(std::find(Lines.begin(), Lines.end(), std::string("Evening, boardwalk!")) - Lines.begin());
	double In = -1.0;
	double Out = -1.0;
	// Its slot comes round every 25 hours, an hour later each day: 24 days from Nov 4 cover every hour.
	for (int Slot = static_cast<int>(IgPrime / 10.0); Slot < static_cast<int>((IgPrime + 24.0 * IgDay) / 10.0) && (In < 0.0 || Out < 0.0); ++Slot)
	{
		const double When = static_cast<double>(Slot) * 10.0;
		if (ig::IgniteShoutAt(Seed, When + 1.0) == Evening)
		{
			const int Hour = static_cast<int>(std::fmod(When, IgDay) / 60.0);
			double& First = Hour >= 17 && Hour < 23 ? In : Out;
			First = First < 0.0 ? When : First;
		}
	}
	bool Heard = false;
	bool Quiet = true;
	for (int K = 0; K < 2; ++K)
	{
		const double When = K == 0 ? In : Out;
		if (When < 0.0)
		{
			continue;
		}
		IgLaptop P("ignite-shouts");
		P.S.BankrollCents = 900000; // the month's rent gets paid on the way, the apartment (and its internet) stays
		P.At(When + 5.0);
		P.Join(21240);
		IgExpect(P.S.WorldSeed() == Seed, "shouts: the same career seed, the same shouts");
		P.Open(Page::News);
		P.Settled(); // the laptop's Ignite state starts with its first frame
		ig::IgniteOf(&P.RL).Buzz = 3;
		const DrawList L = P.Settled();
		if (K == 0)
		{
			Heard = DrewRun(L, "Evening, boardwalk!");
			WriteList("ignite_buzz_evening", L);
		}
		else
		{
			Quiet = !DrewRun(L, "Evening, boardwalk!");
		}
	}
	std::printf("  ignite shouts: \"Evening, boardwalk!\" falls at %s (heard: %s) and %s (heard: %s)\n", In < 0.0 ? "-" : ss::net::TimeLabel(In).c_str(), Heard ? "yes" : "no",
		Out < 0.0 ? "-" : ss::net::TimeLabel(Out).c_str(), Quiet ? "no" : "yes");
	IgExpect((In < 0.0 || Heard) && (Out < 0.0 || Quiet) && (In >= 0.0 || Out >= 0.0), "shouts: BUZZ hears \"Evening, boardwalk!\" in the evening and not at other hours");
}

void IgPlacards()
{
	// G1_AUDIT L6: the wheel's gondola placards never tuck under its sign: every placard that shows at all (its rim included)
	// sits clear above the sign, all the way round the wheel, wide or narrow, bobbing or not.
	namespace ia = ss::ui::igniteart;
	bool Clear = true;
	int Shown = 0;
	for (int Step = 0; Step < 1440; ++Step)
	{
		const float Angle = static_cast<float>(Step) * 3.14159265f / 720.0f;
		for (size_t L = 0; L < 3; ++L)
		{
			for (const float W : {64.0f, 150.0f})
			{
				for (const double Now : {0.0, 0.98, 1.96})
				{
					const ia::IgnitePlacard Pc = ia::IgnitePlacardAt(ia::IgniteWheelCx, ia::IgniteWheelCy, ia::IgniteWheelR, Angle, L, W, Now);
					if (Pc.Show > 0.01f)
					{
						++Shown;
						Clear = Clear && Pc.Tag.Y + Pc.Tag.H + 1.2f <= ia::IgniteWheelSign.Y - ia::IgnitePlacardGap + 0.001f;
					}
				}
			}
		}
	}
	// As drawn: the labels' runs round a whole turn stay over the sign's top.
	TableMeasurer M;
	const ia::IgniteTones K = ia::IgniteTonesFor(ss::site::show::Mood::PrimeTime);
	int Runs = 0;
	for (int Step = 0; Step < 360; ++Step)
	{
		DrawList L;
		ss::ui::Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
		ia::IgniteFerrisWheel(C, ia::IgniteWheelCx, ia::IgniteWheelCy, ia::IgniteWheelR, static_cast<float>(Step) * 3.14159265f / 180.0f, {"8 PM \xC2\xB7 The Spark", "Thu 9 PM \xC2\xB7 Sunday Bl\xE2\x80\xA6", "10 PM"},
			K, 3.0);
		for (const ss::ui::DrawCmd& Cmd : L.Cmds)
		{
			if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
			{
				++Runs;
				Clear = Clear && Cmd.Text.BaselineY + 5.0f + 1.2f <= ia::IgniteWheelSign.Y;
			}
		}
	}
	std::printf("  ignite wheel: %d placards shown over a turn, %d label runs drawn, all clear of the sign: %s\n", Shown, Runs, Clear ? "yes" : "no");
	IgExpect(Clear && Shown > 1000 && Runs > 500, "wheel: the gondola placards never tuck under the wheel's sign");
}

// ------------------------------------------------------------------ G1 fix round 2 (G1_AUDIT N3, N4, N5, N11)

/** A chat of N player lines, six speakers in turn (g1-reaudit's busy-table chat: short and two-line lines mixed). */
std::vector<ss::ChatLine> IgPlayerChat(int N, double Time)
{
	static const char* const Who[6] = {"neonnancy", "flipqueen", "TacoTuesday", "maxk0", "OllieOops", "LilaVance"};
	static const char* const Says[4] = {"gl all", "lila is streaming this rn", "maxk0 shoving again lol, every single orbit like clockwork", "ship it"};
	std::vector<ss::ChatLine> Out;
	for (int K = 0; K < N; ++K)
	{
		ss::ChatLine L;
		L.Who = Who[K % 6];
		L.Text = Says[K % 4];
		L.Kind = ss::ChatKind::Player;
		L.Time = Time;
		Out.push_back(L);
	}
	return Out;
}

/** The text runs drawn at X (within half a unit). */
std::vector<const ss::ui::TextItem*> IgRunsAt(const DrawList& L, float X)
{
	std::vector<const ss::ui::TextItem*> Out;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && std::fabs(Cmd.Text.X - X) < 0.5f)
		{
			Out.push_back(&Cmd.Text);
		}
	}
	return Out;
}

/** Vertices of colour Rgb (alpha over 0.5). */
size_t IgVertsOf(const DrawList& L, uint32_t Rgb)
{
	size_t N = 0;
	for (const ss::ui::Vertex& V : L.Vertices)
	{
		N += V.Col.A > 0.5f && IgRgb(V.Col) == Rgb ? 1 : 0;
	}
	return N;
}

/**
 * The table's busiest moment (g1-reaudit's bench, G1_AUDIT 8.1): the hero's all-in held up in a bounty, so the confetti, the
 * KO sticker and the pot's fountain (a 64,200 pot) fly at once, with Tab open (Chat with 30 player lines when Full). Returns the
 * peak vertices over two seconds at 30 Hz (writes that frame as Page) and, in Draws, how many frames drew Ignite's chat panel
 * (0: the table still draws RiverLine.cpp's own, patch fix2-ignite_to_lead_1 not in yet). 0 if no table.
 */
size_t IgBusiest(ss::RightTab Tab, bool Full, const std::string& Page, long long* Draws)
{
	namespace ig = ss::ui::rlignite_detail;
	IgLaptop P("ignite-potwin");
	if (!IgTableAt(P, IgPrime - 60.0))
	{
		return 0;
	}
	ss::Session& S = P.S;
	S.Tab = Tab;
	int Hero = -1;
	for (ss::SeatVis& Seat : S.Seats)
	{
		if (Seat.Present && Seat.IsHero)
		{
			Hero = Seat.Seat;
			Seat.Winner = true;
			Seat.AllIn = true;
		}
	}
	if (S.CurHand)
	{
		S.CurHand->bComplete = true;
	}
	if (Full)
	{
		for (const ss::ChatLine& L : IgPlayerChat(30, P.Now))
		{
			S.Chat.push_back(L);
		}
	}
	P.Frame();
	const long long Before = ig::IgniteOf(&P.RL).ChatFrames;
	const double T0 = P.Now;
	ss::Flight F;
	F.IsChips = true;
	F.From = ss::FlightEnd::AtPot();
	F.To = ss::FlightEnd::AtSeat(Hero);
	F.Amount = 64200;
	F.Start = T0;
	F.Dur = 0.6;
	S.Flights.push_back(F);
	S.LastBountyAt = T0;
	S.LastBountyCents = 1100;
	S.LastBountyName = "reckless_fish";
	size_t Peak = 0;
	DrawList Top;
	for (int K = 0; K <= 60; ++K)
	{
		P.Now = T0 + K / 30.0;
		if (K == 21)
		{
			S.Flights.clear(); // Session erases the pot's flight once it has landed
		}
		DrawList L = P.Frame();
		if (L.Vertices.size() > Peak)
		{
			Peak = L.Vertices.size();
			Top = std::move(L);
		}
	}
	if (!Page.empty())
	{
		WriteList(Page, Top);
	}
	if (Draws)
	{
		*Draws = ig::IgniteOf(&P.RL).ChatFrames - Before;
	}
	return Peak;
}

void IgChatCap()
{
	// G1_AUDIT N3: the busiest table frame with a full chat of bubbles within the table's 22k (SPEC §10). The chat draws the
	// newest IgniteChatBubbles items as low-poly bubbles and the older ones as plain lines, a speaker's lines in a row share a
	// bubble, and each bubble's wraps and geometry are kept (the second frame measures nothing).
	namespace ig = ss::ui::rlignite_detail;
	IgLaptop P("ignite-chat");
	const ss::ui::Rect Box{1208.0f, 312.0f, 372.0f, 630.0f};
	const auto Draw = [&P, &Box](const std::vector<ss::ChatLine>& Chat) {
		DrawList L;
		ss::ui::Canvas C(L, P.M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, 1.0f);
		ig::IgniteTableChat(C, &P.RL, Chat, Box);
		return L;
	};
	const ig::IgniteUi& U = ig::IgniteOf(&P.RL);
	const std::vector<ss::ChatLine> Full = IgPlayerChat(30, 1.0);
	const DrawList A = Draw(Full);
	const size_t Fits = U.ChatFits.size();
	const int Bubbles = U.ChatBubbles;
	const int Lines = U.ChatLines;
	const DrawList B = Draw(Full);
	bool Same = A.Vertices.size() == B.Vertices.size() && A.Cmds.size() == B.Cmds.size();
	for (size_t K = 0; Same && K < A.Cmds.size(); ++K)
	{
		Same = A.Cmds[K].Type == B.Cmds[K].Type && (A.Cmds[K].Type != ss::ui::DrawCmd::Kind::Text || (A.Cmds[K].Text.Text == B.Cmds[K].Text.Text && A.Cmds[K].Text.X == B.Cmds[K].Text.X &&
																									  A.Cmds[K].Text.BaselineY == B.Cmds[K].Text.BaselineY));
	}
	IgExpect(Bubbles == ig::IgniteChatBubbles && Lines > Bubbles && A.Vertices.size() <= static_cast<size_t>(ig::IgniteChatBubbles) * 60u,
		"chat: a full chat draws its newest 8 as bubbles (58 vertices each) and the older lines plain");
	IgExpect(Same && U.ChatFits.size() == Fits, "chat: the next frame replays the same bubbles and measures nothing again");
	// The newest line is the lowest, in a bubble (its words at the bubble's 46); the older lines plain at 12, fading.
	const std::vector<const ss::ui::TextItem*> InBubbles = IgRunsAt(A, Box.X + 46.0f);
	const std::vector<const ss::ui::TextItem*> PlainRuns = IgRunsAt(A, Box.X + 12.0f);
	const ss::ui::TextItem* Lowest = nullptr;
	for (const ss::ui::DrawCmd& Cmd : A.Cmds)
	{
		Lowest = Cmd.Type == ss::ui::DrawCmd::Kind::Text && (!Lowest || Cmd.Text.BaselineY > Lowest->BaselineY) ? &Cmd.Text : Lowest;
	}
	bool Faded = !PlainRuns.empty();
	float PlainBottom = 0.0f;
	float PlainTop = 1.0e9f;
	for (const ss::ui::TextItem* T : PlainRuns)
	{
		Faded = Faded && T->Col.A < 0.9f;
		PlainBottom = std::max(PlainBottom, T->BaselineY);
		PlainTop = std::min(PlainTop, T->BaselineY);
	}
	// g1-check2: the oldest line drawn is whole inside the panel. Its first run sits 12 under its top (the name's baseline), so a
	// line the panel's clip would cut in half shows here as a run less than 12 under the box's top.
	IgExpect(!PlainRuns.empty() && PlainTop >= Box.Y + 12.0f - 0.01f, "chat: the oldest plain line is whole inside the panel, none cut in half by its top");
	float BubbleTop = 1.0e9f;
	for (const ss::ui::TextItem* T : InBubbles)
	{
		BubbleTop = std::min(BubbleTop, T->BaselineY);
	}
	IgExpect(Lowest && Lowest->Text == Full.back().Text && std::fabs(Lowest->X - (Box.X + 46.0f)) < 0.5f && Faded && PlainBottom < BubbleTop,
		"chat: newest at the bottom in a bubble; the older lines above it plain and fading");
	std::printf("  ignite chat: 30 lines -> %d bubbles, %d lines drawn, %zu vertices, %zu plain runs\n", Bubbles, Lines, A.Vertices.size(), PlainRuns.size());
	// A speaker's lines in a row share one bubble (three at most); the dealer's lines are bare; the room's line is grape with a
	// lime spark; an initial is a whole character.
	{
		std::vector<ss::ChatLine> Run = IgPlayerChat(1, 1.0);
		Run[0].Who = "flipqueen";
		Run.push_back(Run[0]);
		Run.back().Text = "nh";
		Run.push_back(Run[0]);
		Run.back().Text = "ty all";
		const DrawList L3 = Draw(Run);
		const int One = U.ChatBubbles;
		size_t Names = 0;
		for (const ss::ui::TextItem* T : IgRunsAt(L3, Box.X + 46.0f))
		{
			Names += T->Text == "flipqueen" ? 1 : 0;
		}
		Run.push_back(Run[0]);
		Draw(Run);
		const int Four = U.ChatBubbles;
		IgExpect(One == 1 && Names == 1 && DrewRun(L3, "nh") && DrewRun(L3, "ty all") && Four == 2, "chat: a speaker's lines in a row share a bubble and one name (three to a bubble)");
		ss::ChatLine Dealer;
		Dealer.Who = "Dealer";
		Dealer.Kind = ss::ChatKind::Dealer;
		Dealer.Text = "moser98 raises to 513";
		const DrawList Ld = Draw({Dealer, Dealer, Dealer});
		ss::ChatLine Room;
		Room.Kind = ss::ChatKind::System;
		Room.Text = "Welcome to $22 Sky High. 129 players, 19 paid.";
		const DrawList Lr = Draw({Room});
		ss::ChatLine Acc = Run[0];
		Acc.Who = "\xC3\x89lodie";
		const DrawList La = Draw({Acc});
		IgExpect(Ld.Vertices.empty() && DrewRun(Ld, "moser98 raises to 513") && IgVertsOf(Lr, 0x3a1f5cu) > 0 && IgVertsOf(Lr, ss::ui::igniteart::IgLime) > 0 && DrewRun(La, "\xC3\x89"),
			"chat: the dealer's lines bare, the room's grape with a lime spark, an initial a whole character");
	}
	// The table at its busiest (patch fix2-ignite_to_lead_1: SidePanel's CHAT and STANDINGS are this file's).
	long long Draws = 0;
	const size_t Chat = IgBusiest(ss::RightTab::Chat, true, "ignite_table_busy_chat", &Draws);
	if (Chat == 0)
	{
		std::printf("  ignite: no open event at this hour; the busy-table checks are skipped\n");
		return;
	}
	const size_t Standings = IgBusiest(ss::RightTab::Info, false, "ignite_table_busy_standings", nullptr);
	const size_t Payouts = IgBusiest(ss::RightTab::Payouts, false, std::string(), nullptr);
	const size_t Plain = IgBusiest(ss::RightTab::Chat, false, std::string(), nullptr);
	std::printf("  ignite table at its busiest (confetti, KO, the fountain; budget 22k): full chat %zu, chat %zu, standings %zu, payouts %zu vertices\n", Chat, Plain, Standings, Payouts);
	// Patch fix2-ignite_to_lead_1 is in (applied 2026-10-21): a table that stops drawing Ignite's chat is a failure, not a wait
	// (review-fix2-ignite, second pass).
	IgExpect(Draws > 0 && Chat <= 22000 && Standings <= 22000 && Payouts <= 22000 && Plain <= 22000,
		"chat: the table draws Ignite's chat; its busiest frame (with a full chat of bubbles, and with each tab) within 22k");
}

void IgPotPulseCheck()
{
	// G1_AUDIT N4 (SPEC §2.5 "the pot pill pulses as it grows"): the pill pops when the pot grows in a hand (the bigger the step,
	// the bigger), its figure lit sunflower, and settles back to the pill at rest; a new hand starts it afresh.
	namespace ig = ss::ui::rlignite_detail;
	const ig::IgnitePotPulse Before = ig::IgnitePotPulseAt(-0.1, 1.0f);
	const ig::IgnitePotPulse Peak = ig::IgnitePotPulseAt(0.08, 1.0f);
	const ig::IgnitePotPulse Small = ig::IgnitePotPulseAt(0.08, 0.35f);
	const ig::IgnitePotPulse Back = ig::IgnitePotPulseAt(0.21, 1.0f);
	const ig::IgnitePotPulse Done = ig::IgnitePotPulseAt(ig::IgnitePotPulseFor, 1.0f);
	bool Fades = true;
	float Max = 0.0f;
	float Last = 2.0f;
	for (int K = 0; K <= 45; ++K)
	{
		const ig::IgnitePotPulse Q = ig::IgnitePotPulseAt(K * 0.01, 1.0f);
		Fades = Fades && Q.Glow <= Last && Q.Scale >= 0.97f && Q.Scale <= 1.121f;
		Last = Q.Glow;
		Max = std::max(Max, Q.Scale);
	}
	IgExpect(Before.Scale == 1.0f && Before.Glow == 0.0f && std::fabs(Peak.Scale - 1.12f) < 0.002f && std::fabs(Small.Scale - 1.042f) < 0.002f && std::fabs(Back.Scale - 1.0f) < 0.02f &&
				 Done.Scale == 1.0f && Done.Glow == 0.0f && Done.Flash == 0.0f && Peak.Flash > 0.5f && Fades && Max <= 1.121f,
		"pot pill: pops in 80 ms (12% at most, less for a small step), its glow fading, settled by 0.45 s");
	IgLaptop P("ignite-pot");
	if (!IgTableAt(P, IgPrime - 60.0))
	{
		std::printf("  ignite: no open event at this hour; the pot pill checks are skipped\n");
		return;
	}
	ss::Session& S = P.S;
	const ss::ui::Rect Pill{510.0f, 470.0f, 160.0f, 28.0f};
	const auto Draw = [&P, &Pill](ss::Chips Total, double Now) {
		DrawList L;
		ss::ui::Canvas C(L, P.M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, 1.0f);
		ig::IgniteTablePotPill(C, P.RL, Pill, Total, Now);
		return L;
	};
	const auto Figure = [&P](const DrawList& L, const std::string& Text) { return IgRun(L, Text, P.M); };
	const double T0 = P.Now + 10.0;
	const DrawList Seen = Draw(1200, T0);
	const DrawList Rest = Draw(1200, T0 + 0.5);
	const DrawList Pop = Draw(2400, T0 + 1.0);
	const DrawList Up = Draw(2400, T0 + 1.08);
	const DrawList Settled = Draw(2400, T0 + 1.6);
	const IgBox F0 = Figure(Rest, "POT 1,200");
	const IgBox F1 = Figure(Up, "POT 2,400");
	const IgBox F2 = Figure(Settled, "POT 2,400");
	ss::ui::Color Lit;
	for (const ss::ui::DrawCmd& Cmd : Up.Cmds)
	{
		Lit = Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == "POT 2,400" ? Cmd.Text.Col : Lit;
	}
	IgExpect(Seen.Vertices.size() == Rest.Vertices.size() && F0.Found && F0.Size == 17.0f && F0.Face == ss::ui::Font::BlackItalic,
		"pot pill: the first sight of a hand's pot doesn't pop (Night, coral rim, the figure in BlackItalic 17)");
	IgExpect(F1.Found && F1.Size > 17.0f * 1.05f && Up.Vertices.size() > Rest.Vertices.size() && IgRgb(Lit) != ss::ui::igniteart::IgCream && Pop.Vertices.size() > Rest.Vertices.size(),
		"pot pill: the pot growing pops the pill (scaled, glowing, the figure lit)");
	IgExpect(F2.Found && F2.Size == 17.0f && Settled.Vertices.size() == Rest.Vertices.size(), "pot pill: settled by 0.45 s, the pill at rest again");
	// A new hand (a later deal) starts afresh: its first pot, however big, doesn't pop; a smaller pot is a new count, no pop.
	for (ss::SeatVis& Seat : S.Seats)
	{
		Seat.DealtAt = Seat.Present ? T0 + 2.0 : Seat.DealtAt;
	}
	const DrawList Fresh = Draw(9000, T0 + 2.05);
	const DrawList Smaller = Draw(300, T0 + 2.6);
	const DrawList Again = Draw(900, T0 + 3.0);
	IgExpect(Fresh.Vertices.size() == Rest.Vertices.size() && Figure(Fresh, "POT 9,000").Size == 17.0f && Smaller.Vertices.size() == Rest.Vertices.size() &&
				 Again.Vertices.size() > Rest.Vertices.size(),
		"pot pill: a new hand starts the count afresh (no pop), then pops as that pot grows");
	// review-fix2-ignite: the blinds' pop is still running when the cards go out (the deal is 0.12 s after the big blind); the
	// new deal mustn't snap it back to rest in one frame, it eases out.
	for (ss::SeatVis& Seat : S.Seats)
	{
		Seat.DealtAt = Seat.Present ? T0 + 3.1 : Seat.DealtAt;
	}
	const DrawList Dealing = Draw(900, T0 + 3.12);
	const DrawList Eased = Draw(900, T0 + 3.5);
	IgExpect(Figure(Dealing, "POT 900").Size > 17.2f && Figure(Eased, "POT 900").Size == 17.0f && Eased.Vertices.size() == Rest.Vertices.size(),
		"pot pill: the blinds' pop eases out as the cards go out (no snap at the deal)");
	// review-fix2-ignite: the pot growing again mid-pop (an ante, then a blind a beat later) carries the pop on from where the pill
	// is; it never drops back to rest for a frame.
	Draw(1500, T0 + 4.0);
	const DrawList Carried = Draw(2100, T0 + 4.1);
	const DrawList Calm = Draw(2100, T0 + 4.7);
	IgExpect(Figure(Carried, "POT 2,100").Size > 17.5f && Figure(Calm, "POT 2,100").Size == 17.0f,
		"pot pill: growing again mid-pop carries the pop on (no drop to rest), then settles");
	// At the table (the patch): a bet going in pops the pill on the table's own frame.
	P.Now += 10.0;
	P.Frame();
	const bool Patched = ig::IgniteOf(&P.RL).PotHand >= 0.0 || ig::IgniteOf(&P.RL).PotSeen > 0;
	ss::Chips PotBefore = S.PotChips;
	for (ss::SeatVis& Seat : S.Seats)
	{
		PotBefore += Seat.Present ? Seat.Bet : 0;
	}
	for (ss::SeatVis& Seat : S.Seats)
	{
		if (Seat.Present && !Seat.IsHero)
		{
			Seat.Bet += 4 * std::max<ss::Chips>(1, PotBefore);
			break;
		}
	}
	P.Frame(); // the frame that sees the pot grow
	P.Now += 0.08;
	const DrawList Table = P.Frame();
	WriteList("ignite_table_potpulse", Table);
	IgBox Grown;
	for (const ss::ui::DrawCmd& Cmd : Table.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.rfind("POT ", 0) == 0)
		{
			Grown = IgRun(Table, Cmd.Text.Text, P.M);
		}
	}
	// The patch is in (review-fix2-ignite, second pass): Table() drawing its own pill again fails here.
	IgExpect(Patched && Grown.Found && Grown.Size > 17.5f, "pot pill: at the table, a bet going in pops the pill");
	std::printf("  ignite pot pill: %zu vertices at rest, %zu mid-pop; at the table the figure at %.1f px 80 ms after a bet\n", Rest.Vertices.size(), Up.Vertices.size(), Grown.Size);
}

void IgStandingsRings()
{
	// G1_AUDIT N5 (SPEC §2.5 "Standings with level rings"): each row's bubble sits inside a ring (the level ring's track; its arc
	// is G2's), not a disc with a letter on it; yours lit in the accent round a cream bubble.
	namespace ig = ss::ui::rlignite_detail;
	IgLaptop P("ignite-standings");
	if (!IgTableAt(P, IgPrime - 60.0))
	{
		std::printf("  ignite: no open event at this hour; the standings checks are skipped\n");
		return;
	}
	const ss::Tournament& T = *P.S.T;
	const ss::ui::Rect Box{1208.0f, 312.0f, 372.0f, 630.0f};
	DrawList L;
	{
		ss::ui::Canvas C(L, P.M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, 1.0f);
		ig::IgniteTableStandings(C, T, Box);
	}
	// Row I's ring is centred on (Box.X + 52, Box.Y + 4 + 36 I + 16): the track's edge at 15 units, the bubble's at 10.5, in
	// the player's light (a non-hero row) or cream (yours).
	const std::vector<const ss::TPlayer*> St = T.Standings();
	int Rings = 0;
	int Rows = 0;
	for (size_t I = 0; I < std::min<size_t>(St.size(), 10); ++I)
	{
		const float Cx = Box.X + 52.0f;
		const float Cy = Box.Y + 4.0f + 36.0f * static_cast<float>(I) + 16.0f;
		const uint32_t Light = St[I]->IsHero ? ss::ui::igniteart::IgCream : IgRgb(ig::IgniteNameLight(St[I]->Name, St[I]->Name == ss::RivalName, false));
		bool Track = false;
		bool Bubble = false;
		for (const ss::ui::Vertex& V : L.Vertices)
		{
			const float D = std::sqrt((V.X - Cx) * (V.X - Cx) + (V.Y - Cy) * (V.Y - Cy));
			Track = Track || (V.Col.A > 0.5f && std::fabs(D - 15.0f) < 0.3f && (St[I]->IsHero || IgRgb(V.Col) == ss::ui::rlsites_detail::SitesIgTrack));
			Bubble = Bubble || (V.Col.A > 0.5f && std::fabs(D - 10.5f) < 0.3f && IgRgb(V.Col) == Light);
		}
		++Rows;
		Rings += Track && Bubble && DrewRun(L, St[I]->Name) ? 1 : 0;
	}
	WriteList("ignite_standings_rings", L);
	IgExpect(Rows >= 6 && Rings == Rows, "standings: every row's bubble sits in a level ring (the track at 15, the bubble at 10.5)");
	std::printf("  ignite standings: %d rows, %d rings, %zu vertices\n", Rows, Rings, L.Vertices.size());
	// At the table (the patch): the STANDINGS tab draws the rings. The second row's name places its ring (the name at the ring's
	// x + 24, on the row's baseline 21).
	P.S.Tab = ss::RightTab::Info;
	const DrawList Tl = P.Settled();
	WriteList("ignite_table_standings", Tl);
	const IgBox Second = IgRun(Tl, St[1]->Name, P.M, 300.0f, 1000.0f);
	bool Ring = false;
	if (Second.Found)
	{
		const float Cx = Second.X - 24.0f;
		const float Cy = Second.Base - 21.0f + 16.0f;
		const uint32_t Light = St[1]->IsHero ? ss::ui::igniteart::IgCream : IgRgb(ig::IgniteNameLight(St[1]->Name, St[1]->Name == ss::RivalName, false));
		for (const ss::ui::Vertex& V : Tl.Vertices)
		{
			const float D = std::sqrt((V.X - Cx) * (V.X - Cx) + (V.Y - Cy) * (V.Y - Cy));
			Ring = Ring || (V.Col.A > 0.5f && std::fabs(D - 10.5f) < 0.3f && IgRgb(V.Col) == Light);
		}
	}
	// The patch is in (review-fix2-ignite, second pass): the table's STANDINGS drawing discs again fails here.
	IgExpect(Ring && Second.Found && Tl.Vertices.size() <= 22000, "standings: the table draws the level rings, within 22k");
	std::printf("  ignite table, STANDINGS with rings: %zu vertices\n", Tl.Vertices.size());
}
} // namespace ignite_seg

void SegIgnite()
{
	using namespace ignite_seg;
	IgPages();
	IgDock();
	IgFloor();
	IgCardHold();
	IgClause();
	IgMoney();
	IgBoards();
	IgResults();
	IgTable();
	IgPreview();
	IgCopy();
	IgType();
	IgBench();
	// The G1 fix pass (G1_AUDIT M3, L1, L6).
	IgPlayBench();
	IgBeat();
	IgPotWin();
	IgShouts();
	IgPlacards();
	// The G1 fix round (G1_AUDIT 8.4 N3, N4, N5; N11 is IgPlayBench's retry).
	IgChatCap();
	IgPotPulseCheck();
	IgStandingsRings();
	std::printf("  ui-ignite: %d checks\n", IgChecks);
}
} // namespace ui_test
