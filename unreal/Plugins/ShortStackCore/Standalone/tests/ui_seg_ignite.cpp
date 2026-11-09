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
#include "ShortStack/Game/Fuse.h"
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

/** What the raised centre button reads: FUSE once core's View lights FUSE (G2), else FLIP (G1's state). */
const char* IgCentre(IgLaptop& P)
{
	const ss::ui::rlignite_detail::IgniteLoops& L = ss::ui::rlignite_detail::IgniteLoopsOf(P.RL, P.Now);
	return L.Live && L.FuseOn ? "FUSE" : "FLIP";
}

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
	IgExpect(DrewRun(L, "PLAY") && DrewRun(L, "REWARDS") && DrewRun(L, IgCentre(P)) && DrewRun(L, "BOARDS") && DrewRun(L, "WALLET"), "ignite: the dock's five items (the centre: FUSE once FUSE lights, else FLIP)");
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
	// The raised centre button: FUSE once FUSE lights (G2: it opens the Fuse Booth), else FLIP (G1, and if S-J had failed:
	// PLAY on Flip & Run); its keep-out holds the pointer from the page under it.
	P.Open(Page::Cashier);
	P.Settled();
	const bool Fuse = std::string(IgCentre(P)) == "FUSE";
	P.Click(800.0f, 850.0f);
	DrawList L = P.Settled();
	const ss::ui::rlignite_detail::IgniteUi& U = ss::ui::rlignite_detail::IgniteOf(&P.RL);
	if (Fuse)
	{
		IgExpect(P.RL.CurrentPage() == Page::Fuse && DrewRun(L, "THE FUSE WALL"), "ignite: FUSE opens the Fuse Booth (G2)");
		P.Open(Page::Lobby);
		P.Settled();
		ss::ui::rlignite_detail::IgniteOf(&P.RL).Chip = 2;
		P.Settled();
	}
	else
	{
		IgExpect(P.RL.CurrentPage() == Page::Lobby && U.Chip == 2, "ignite: FLIP opens PLAY on Flip & Run");
	}
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
	IgExpect(DrewRun(B, IgCentre(P)) && DrewRun(B, "PLAY"), "preview: the stream shows Ignite's own client");
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
	// The samples a slot is filled with (G2 adds the loops' figures: every one a fact where it's drawn).
	const std::map<std::string, std::string> Vars = {{"Team", "Team RiverLine"}, {"Date", "Dec 4"}, {"Event", "The Spark"}, {"Headliner", "LilaVance"}, {"Pct", "87"},
		{"Promo", "$4.20"}, {"Rake", "$9.60"}, {"Prize", "$22.00"}, {"Place", "3rd"}, {"An", "an"}, {"N", "500"}};
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

// ------------------------------------------------------------------ G2: the loops' pages and overlays (SPEC §2.4, §2.6, house law 3)

namespace ig2
{
using namespace ignite_seg;
namespace ig = ss::ui::rlignite_detail;

/** A stand-in for core's View (until core-siteplay's lands, and for states a fresh career can't reach in a test): a measured
 * hero's week, its figures from SPEC §2.6 (band A's wheel tables, the Wristband's tier costs, the Level curve). */
ig::IgniteLoops Fake(bool Fuse)
{
	ig::IgniteLoops L;
	L.Live = true;
	L.Started = true;
	L.FuseOn = Fuse;
	L.Sparks = 12480;
	L.Streak = 6;
	L.Best = 12;
	L.Matches = 1;
	L.Mult = 1.20;
	L.LitToday = true;
	L.Week = {1, 1, 2, 1, 4, 0, 0};
	const char* const Labels[3] = {"Play a Flip & Run", "Cash once", "Final table"};
	const int64_t Rewards[3] = {60, 120, 200};
	for (int I = 0; I < 3; ++I)
	{
		ig::IgniteDare& D = L.Dares[static_cast<size_t>(I)];
		D.Id = I * 8 + 1;
		D.Tier = I;
		D.Label = Labels[I];
		D.Needs = 1;
		D.Prog = I == 0 ? 1 : 0;
		D.Done = I == 0;
		D.Sparks = Rewards[I];
	}
	L.DaresReset = 31.0 * IgDay + 6.0 * 60.0;
	L.WeekClears = 2;
	L.EntriesToday = 2;
	L.RingsLit = 2;
	L.SpinReady = true;
	L.MoreForNext = 2;
	const auto Ring = [](std::initializer_list<std::pair<int64_t, double>> Rows, const char* Ticket, ss::Chips Face, double P) {
		std::vector<ig::IgniteWedge> W;
		for (const auto& R : Rows)
		{
			ig::IgniteWedge Wd;
			Wd.Sparks = R.first;
			Wd.P = R.second;
			Wd.Label = std::to_string(R.first) + " Sparks";
			W.push_back(Wd);
		}
		ig::IgniteWedge T;
		T.Label = Ticket;
		T.TicketId = "ignite.flip-go";
		T.FaceCents = Face;
		T.P = P;
		W.push_back(T);
		return W;
	};
	L.Rings[0] = Ring({{20, 0.55}, {40, 0.30}, {80, 0.12}, {200, 0.029}}, "FUSE $1.10 ticket", 110, 0.001);
	L.Rings[1] = Ring({{40, 0.50}, {80, 0.30}, {150, 0.15}, {400, 0.047}}, "FUSE $1.10 ticket", 110, 0.003);
	L.Rings[2] = Ring({{80, 0.50}, {150, 0.30}, {300, 0.14}, {800, 0.05}}, "Flip & Run $1.10 ticket", 110, 0.008);
	for (int I = 0; I < 3; ++I)
	{
		ss::site::play::Box B;
		B.Id = "dares:" + std::to_string(30 + I) + ":0";
		B.Tier = static_cast<int8_t>(I == 2 ? 1 : 0);
		L.Crackers.push_back(B);
	}
	L.Tier = 7;
	L.BandSparks = 5240;
	L.Theme = "Kindling";
	L.DaysLeft = 26;
	int64_t Need = 0;
	for (int T = 1; T <= 30; ++T)
	{
		Need += T <= 10 ? 700 : T <= 20 ? 2800 : 7000;
		ig::IgniteTier& Tr = L.Tiers[static_cast<size_t>(T - 1)];
		Tr.Tier = T;
		Tr.Needs = Need;
		Tr.Reached = Need <= L.BandSparks;
		Tr.Claimed = Tr.Reached && T != 5;
		Tr.Kind = T == 10 || T == 20 || T == 30 ? 1 : T == 5 || T == 15 || T == 25 ? 2 : T == 8 || T == 18 ? 3 : T == 12 || T == 22 || T == 27 || T == 29 ? 4 : 0;
		Tr.Reward = T == 10 ? "Flip & Run $1.10 ticket" : T == 5 ? "Paper Cracker" : T == 8 ? "Spare Match" : T == 12 ? "Kindling card back" : T == 30 ? "The Spark $5.50 ticket" : "";
	}
	L.Level = 7;
	L.Title = "Rookie";
	L.LevelFrom = static_cast<int64_t>(100.0 * std::pow(6.0, 2.4));
	L.LevelTo = static_cast<int64_t>(100.0 * std::pow(7.0, 2.4));
	L.LevelSparks = L.LevelFrom + (L.LevelTo - L.LevelFrom) * 6 / 10;
	const std::pair<const char*, int64_t> Tickets[6] = {{"Flip & Run $1.10 ticket", 7000}, {"FUSE $1.10 ticket", 7000}, {"Night Glow $2.20 ticket", 14000},
		{"Plum Turbo $3.30 ticket", 21000}, {"The Spark $5.50 ticket", 35000}, {"Sunday Blaze $11 ticket", 70000}};
	const ss::Chips Faces[6] = {110, 110, 220, 330, 550, 1100};
	for (int I = 0; I < 6; ++I)
	{
		ig::IgnitePrize Pz;
		Pz.Item = I;
		Pz.Label = Tickets[I].first;
		Pz.Sparks = Tickets[I].second;
		Pz.TicketId = "ignite.t" + std::to_string(I);
		Pz.FaceCents = Faces[I];
		L.Prizes.push_back(Pz);
	}
	const char* const LookNames[4] = {"Ember Glow back", "Plum Velvet felt", "Candy chips", "Neon frame"};
	(void)LookNames;
	const std::pair<const char*, int> Looks[4] = {{"Sunburst back", 0}, {"Midnight felt", 1}, {"Jelly chips", 2}, {"Spark frame", 3}};
	for (int I = 0; I < 4; ++I)
	{
		ig::IgnitePrize Pz;
		Pz.Item = 6 + I;
		Pz.Label = Looks[I].first;
		Pz.Sparks = 10000 + 5000 * I;
		Pz.LookSlot = Looks[I].second;
		Pz.Look = 1;
		Pz.Tab = 1;
		Pz.Owned = I == 0;
		L.Prizes.push_back(Pz);
	}
	{
		ig::IgnitePrize Pz;
		Pz.Item = 10;
		Pz.Label = "Spare Match";
		Pz.Sparks = 3000;
		Pz.Tab = 2;
		L.Prizes.push_back(Pz);
	}
	const char* const Slots[5][2] = {{"Sunset", "Sunburst"}, {"Plum Velvet", "Midnight"}, {"Candy", "Jelly"}, {"Bubble", "Spark"}, {"Confetti", "Fireworks"}};
	for (int S = 0; S < 5; ++S)
	{
		for (int K = 0; K < 2; ++K)
		{
			ig::IgniteLook Lk;
			Lk.Slot = S;
			Lk.Look = K;
			Lk.Name = Slots[S][K];
			Lk.Owned = K == 0 || S == 0;
			Lk.Worn = K == 0;
			Lk.How = K == 1 ? (S == 1 ? "Wristband T22" : "Prize Counter 25,000") : "";
			L.Locker.push_back(Lk);
		}
	}
	for (int I = 0; I < 30; ++I)
	{
		ig::IgniteStickerRow St;
		St.Id = I;
		St.Name = I < 24 ? "First " + std::to_string(I + 1) : "";
		St.Name = I == 0 ? "First cash" : I == 1 ? "Final table" : I == 2 ? "7-day flame" : St.Name;
		St.How = I < 24 ? "Do it once" : "";
		St.Got = I < 5;
		St.Secret = I >= 24;
		L.Book.push_back(St);
	}
	L.Fuse.N = {212, 14, 3};
	L.Fuse.Played = 229;
	L.Fuse.Won = 88;
	L.Fuse.NetCents = 1640;
	L.Fuse.BestM = 25;
	L.Fuse.Last10 = {2, 3, 2, 5, 2, 25, 3, 2, 10, 2};
	ss::site::play::WallRow W1;
	W1.M = 25;
	W1.Place = 1;
	W1.Prize = 2200;
	W1.Day = 29;
	ss::site::play::WallRow W2;
	W2.M = 10;
	W2.Place = 2;
	W2.Prize = 0;
	W2.Day = 27;
	L.Fuse.Wall = {W1, W2};
	ss::site::play::HitRow H;
	H.M = 25;
	H.Winner = "LilaVance";
	H.Prize = 2200;
	H.Day = 28;
	L.Fuse.Hits = {H};
	L.FuseTicketIds = {"ignite.fuse0", "ignite.fuse1", "ignite.fuse2"};
	L.Headliners = {"LilaVance", "KatOnTheRiver"};
	ss::site::play::Beef B;
	B.Name = "maxk0";
	B.TheyKo = 3;
	B.YouKo = 1;
	L.Beefs = {B};
	L.BeefName = "maxk0";
	L.ScalpLife = 2;
	L.ThemeNight = 2;
	L.ThemeName = "FUSE NIGHT";
	L.ThemeTwist = "FUSE pays 1.5 times the base Sparks";
	L.Rake30 = 960;
	L.Promo30 = 420;
	L.Share = 0.4375;
	L.Ceiling = 0.75;
	L.Welcome = true;
	L.WeekFace = 420;
	L.WeekCap = 1200;
	ig::IgniteHold Hd;
	Hd.What = "Night Glow ticket";
	Hd.Cents = 220;
	Hd.Covered = 100;
	Hd.RakeLeft = 240;
	L.Holds = {Hd};
	L.Months = {{"November", 420}};
	return L;
}

int Runs(const DrawList& L)
{
	int N = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		N += Cmd.Type == ss::ui::DrawCmd::Kind::Text ? 1 : 0;
	}
	return N;
}

/** A G2 page: checked as every page is (it draws, nothing "coming", no leak), and within SPEC §10's caps. */
void Page2(const DrawList& L, const std::string& Name, size_t MaxVerts = 35000, int MaxRuns = 600)
{
	IgPage(L, Name);
	std::printf("  %s: %zu vertices, %d runs\n", Name.c_str(), L.Vertices.size(), Runs(L));
	IgExpect(L.Vertices.size() <= MaxVerts && Runs(L) <= MaxRuns, ("ignite g2: within SPEC §10's caps: " + Name).c_str());
}

/** Whether some run Text (baseline in [MinY, MaxY]) is centred within Tol of X (centred text, or left text whose middle is). */
bool CentredAt(const DrawList& L, const std::string& Text, const TableMeasurer& M, float X, float Tol, float MinY, float MaxY)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text && Cmd.Text.BaselineY >= MinY && Cmd.Text.BaselineY <= MaxY)
		{
			const float W = M.Width(Text, Cmd.Text.Face, Cmd.Text.SizePx);
			if (std::fabs(Cmd.Text.X - X) < Tol || std::fabs(Cmd.Text.X + W / 2.0f - X) < Tol)
			{
				return true;
			}
		}
	}
	return false;
}

/** Clicks the run Text (its middle) if it's drawn; false when it isn't. */
bool ClickRun(IgLaptop& P, const DrawList& L, const std::string& Text)
{
	const IgBox B = IgRun(L, Text, P.M);
	if (!B.Found)
	{
		return false;
	}
	P.Click(B.X + B.W / 2.0f, B.Base - 6.0f);
	return true;
}

void Pages()
{
	IgLaptop P("ignite-g2-pages");
	ss::Session& S = P.S;
	P.At(IgPrime);
	S.BankrollCents = 150000;
	P.Join(21240);
	const ig::IgniteLoops F = Fake(true);
	ig::IgniteFakeLoops(&P.RL, &F);
	P.Open(Page::Lobby);
	DrawList L = P.Settled();
	Page2(L, "ignite_g2_play_prime", 34000, 400);
	IgExpect(DrewRun(L, "12,480") && DrewRun(L, "FUSE") && !DrewRun(L, "FLIP"), "ignite g2: the marquee's Sparks; the raised button reads FUSE once FUSE lights");
	IgExpect(DrewRun(L, "SHINE LIT \xC2\xB7 2 MORE FOR DAZZLE") && DrewRun(L, "6 DAYS") && DrewRun(L, "Play a Flip & Run") && DrewRun(L, "Final table"),
		"ignite g2: the wheel's sign, the Flame and three Dare chips on PLAY");
	IgExpect(DrewRun(L, "+120") && DrewRun(L, "LIGHT A FUSE") && DrewRun(L, "WRISTBAND") && DrewRun(L, "SPIN READY"), "ignite g2: cabinets earn Sparks (+120 at $1.10), the Fuse Booth, the dock's strips");
	IgExpect(DrewRun(L, "2 more light DAZZLE"), "ignite g2: the near-goal line says 1 more or 2 more");
	// The Sparks pill: "+610" floats up as Sparks land.
	{
		ig::IgniteLoops G = F;
		G.Sparks += 610;
		ig::IgniteFakeLoops(&P.RL, &G);
		P.Now += 0.05;
		P.Frame();
		P.Now += 0.3;
		const DrawList Lf = P.Frame();
		IgExpect(DrewRun(Lf, "+610") && DrewRun(Lf, "13,090"), "ignite g2: \"+610\" floats up when Sparks land");
		ig::IgniteFakeLoops(&P.RL, &F);
	}
	// The sheet: what an entry earns, WHO'S IN's tags.
	{
		const std::vector<std::string> Listed = P.RL.ListedEvents();
		if (!Listed.empty())
		{
			ig::IgniteOpenSheet(P.RL, Listed.front(), P.Now);
			const DrawList Ls = P.Settled();
			Page2(Ls, "ignite_g2_sheet");
			IgExpect(IgStartsWith(Ls, "EARNS "), "ignite g2: the sheet says what an entry earns in Sparks");
			ig::IgniteOf(&P.RL).Sheet = false;
		}
	}
	// REWARDS: Today, the Wristband, the Prize Counter.
	P.Open(Page::Promotions);
	L = P.Settled();
	Page2(L, "ignite_g2_rewards_today");
	IgExpect(DrewRun(L, "DARES") && DrewRun(L, "SPARK WHEEL") && DrewRun(L, "CRACKERS") && DrewRun(L, "THEME NIGHTS") && DrewRun(L, "Claim"), "ignite g2: REWARDS' Today (Dares, the wheel, crackers, the week's themes)");
	IgExpect(ClickRun(P, L, "Wristband"), "ignite g2: REWARDS' sub-pills");
	L = P.Settled();
	Page2(L, "ignite_g2_rewards_band");
	IgExpect(DrewRun(L, "WRISTBAND \xC2\xB7 KINDLING") && DrewRun(L, "CLAIM"), "ignite g2: the Wristband's 30 tiers, a reached one waiting on CLAIM");
	ClickRun(P, L, "Prize Counter");
	L = P.Settled();
	Page2(L, "ignite_g2_rewards_prizes");
	IgExpect(DrewRun(L, "Flip & Run $1.10 ticket") && DrewRun(L, "7,000") && DrewRun(L, "Buy"), "ignite g2: the Prize Counter's tickets and prices");
	// FUSE's booth.
	P.Open(Page::Fuse);
	L = P.Settled();
	Page2(L, "ignite_g2_fuse");
	IgExpect(P.RL.CurrentPage() == Page::Fuse && DrewRun(L, "THE FUSE WALL") && DrewRun(L, "Light it \xC2\xB7 $1.10") && DrewRun(L, "LAST 10"), "ignite g2: FUSE's page (cabinets, LIGHT IT, the last ten, the Wall)");
	// BOARDS' Fuse Wall, BUZZ's Big hits.
	P.Open(Page::Leaderboards);
	L = P.Settled();
	IgExpect(ClickRun(P, L, "Fuse Wall"), "ignite g2: BOARDS has the Fuse Wall tab once FUSE lights");
	L = P.Settled();
	Page2(L, "ignite_g2_boards_wall");
	P.Open(Page::News);
	L = P.Settled();
	Page2(L, "ignite_g2_buzz");
	IgExpect(ClickRun(P, L, "Big hits"), "ignite g2: BUZZ's Big hits filter appears with the first hit");
	L = P.Settled();
	// The title's multiplier is the vector cross (the glyph rule, G2_AUDIT L9): "LilaVance hit ", the cross, "25 at your table".
	IgExpect(DrewRun(L, "LilaVance hit ") && DrewRun(L, "25 at your table") && !DrewRun(L, "LilaVance hit x25 at your table") && DrewRun(L, "BIG HIT"),
		"ignite g2: BUZZ's Big hits (the true results of FUSEs you sat in), the multiplier crossed");
	// The Player Card: the Locker, the Sticker Book, Rivals.
	P.Open(Page::Profile);
	L = P.Settled();
	IgExpect(DrewRun(L, "LEVEL 7 \xC2\xB7 ROOKIE") && DrewRun(L, "LOCKER") && DrewRun(L, "STICKER BOOK") && DrewRun(L, "RIVALS"), "ignite g2: the Player Card's level and tabs");
	const char* const Tabs[3] = {"LOCKER", "STICKER BOOK", "RIVALS"};
	const char* const Names[3] = {"ignite_g2_card_locker", "ignite_g2_card_stickers", "ignite_g2_card_rivals"};
	for (int I = 0; I < 3; ++I)
	{
		ClickRun(P, L, Tabs[I]);
		L = P.Settled();
		Page2(L, Names[I]);
	}
	IgExpect(DrewRun(L, "maxk0") && DrewRun(L, "BEEF"), "ignite g2: Rivals shows your Beef");
	// Afterburner Hours with the loops on.
	P.At(IgLate);
	P.Open(Page::Lobby);
	L = P.Settled();
	Page2(L, "ignite_g2_play_afterburner", 34000, 400);
	// Under the clause: the Today strip is the deal's line, the Flame wears frost.
	{
		ig::IgniteLoops G = F;
		G.OnIce = true;
		ig::IgniteFakeLoops(&P.RL, &G);
		IgSignClause(S);
		ig::IgniteOf(&P.RL).Rewards = 0;
		P.Open(Page::Promotions);
		P.Settled();
		P.Now += 12.0; // the deal's badge toast (RiverLine's own, over every app) has gone
		L = P.Settled();
		Page2(L, "ignite_g2_rewards_clause");
		IgExpect(DrewRun(L, ig::IgniteOnIce) && DrewRun(L, ig::IgniteDaresGrey), "ignite g2: under the clause the Flame waits on ice and the Dares grey");
	}
	// The stand-in dropped, the pages read core's own View again (G1's states while it isn't on).
	ig::IgniteFakeLoops(&P.RL, nullptr);
	P.Open(Page::Lobby);
	L = P.Settled();
	const ig::IgniteLoops Core = ig::IgniteLoopsFrom(S, S.WorldMinutes());
	IgExpect(!DrewRun(L, "12,480") && DrewRun(L, Core.Live && Core.FuseOn ? "FUSE" : "FLIP") && (!Core.Live || DrewRun(L, ss::Grouped(Core.Sparks))),
		"ignite g2: without the stand-in the pages read core's View (G1's states while it's off)");
}

/** House law 3 for each G2 overlay: a one-frame click (pressed and released in one 30 Hz frame) reaches nothing under it. */
void Overlays()
{
	IgLaptop P("ignite-g2-overlays");
	ss::Session& S = P.S;
	P.At(IgPrime);
	S.BankrollCents = 150000;
	P.Join(21240);
	ig::IgniteLoops F = Fake(true);
	F.SpunRing = 2;
	F.SpunWedge = 3;
	ig::IgniteFakeLoops(&P.RL, &F);
	ig::IgniteUi& U = ig::IgniteOf(&P.RL);
	// The dock's BOARDS sits outside every overlay's own rect: a one-frame click there is a scrim click.
	const float Bx = 974.0f;
	const float By = 916.0f;
	P.Open(Page::Promotions);
	P.Settled();
	// The wheel's spin lands on core's wedge (SpunWedge 3 of SHINE: 400 Sparks) under the flapper.
	ig::IgniteStartSpin(U, F, P.Now);
	P.Frame();
	P.OneFrame(800.0f, 300.0f); // a click inside while it turns skips to the landing
	IgExpect(U.Spin && P.Now - U.SpinAt >= 3.2, "ignite g2: a click on the spinning wheel skips to its landing");
	DrawList L = P.Frame();
	IgExpect(CentredAt(L, "400", P.M, 800.0f, 4.0f, 100.0f, 320.0f), "ignite g2: the wheel lands on core's wedge (its label under the flapper)");
	IgExpect(DrewRun(L, "+400 SPARKS"), "ignite g2: the spin's prize card says what core drew");
	WriteList("ignite_g2_spin", L);
	P.OneFrame(Bx, By);
	P.Settled();
	IgExpect(!U.Spin && P.RL.CurrentPage() == Page::Promotions, "ignite g2: the spin's scrim closes it and the click reaches nothing (BOARDS stays shut)");
	// A cracker before its pull: the scrim closes it; PULL goes to core (which refuses until core-siteplay lands: its reason shows).
	U.Crack = true;
	U.CrackId = F.Crackers.front().Id;
	U.CrackAt = -100.0;
	L = P.Frame();
	WriteList("ignite_g2_cracker", L);
	P.OneFrame(Bx, By);
	P.Settled();
	IgExpect(!U.Crack && P.RL.CurrentPage() == Page::Promotions, "ignite g2: a cracker's scrim closes it, the click reaches nothing");
	U.Crack = true;
	U.CrackId = F.Crackers.front().Id;
	L = P.Frame();
	const std::string Why = S.OpenCracker(U.CrackId);
	ClickRun(P, L, "Pull");
	if (!Why.empty())
	{
		IgExpect(U.CrackAt < -50.0 && U.Note == Why, "ignite g2: PULL asks core; a refusal shows its reason and nothing animates");
	}
	U.Crack = false;
	// The Prize Counter: BUY opens the confirm card; a one-frame click on the card spends nothing and reaches nothing under it.
	U.Rewards = 2;
	L = P.Settled();
	IgExpect(ClickRun(P, L, "Buy") && U.BuyItem >= 0, "ignite g2: BUY opens the confirm card (it spends nothing)");
	const int64_t Sparks = ig::IgniteLoopsOf(P.RL, P.Now).Sparks;
	P.OneFrame(700.0f, 420.0f);
	P.Settled();
	IgExpect(U.BuyItem >= 0 && ig::IgniteLoopsOf(P.RL, P.Now).Sparks == Sparks, "ignite g2: a one-frame click on the confirm card spends nothing");
	L = P.Frame();
	WriteList("ignite_g2_buy", L);
	P.OneFrame(Bx, By);
	P.Settled();
	IgExpect(U.BuyItem < 0 && P.RL.CurrentPage() == Page::Promotions, "ignite g2: the confirm card's scrim cancels it, the click reaches nothing");
	// The drawers: the wheel's odds and the Fine print.
	U.Rewards = 0;
	L = P.Settled();
	IgExpect(ClickRun(P, L, "FINE PRINT") && U.Drawer == 3, "ignite g2: FINE PRINT opens its drawer");
	L = P.Settled();
	Page2(L, "ignite_g2_fineprint");
	IgExpect(IgStartsWith(L, "Ignite gave back $4.20 of your $9.60 rake") && (IgStartsWith(L, "last 30 days (44%).") || IgStartsWith(L, "30 days (44%).")) && DrewRun(L, "Night Glow ticket"),
		"ignite g2: the Fine print shows the ring's own figure and the Guest List");
	P.OneFrame(Bx, By);
	P.Settled();
	IgExpect(U.Drawer == 0 && P.RL.CurrentPage() == Page::Promotions, "ignite g2: a drawer's scrim closes it, the click reaches nothing");
	L = P.Settled();
	ClickRun(P, L, "ODDS");
	L = P.Settled();
	IgExpect(U.Drawer == 1 && DrewRun(L, "THE WHEEL'S ODDS") && DrewRun(L, "55.0%"), "ignite g2: the wheel's odds, one click away");
	WriteList("ignite_g2_wheelodds", L);
	P.OneFrame(Bx, By);
	P.Settled();
	IgExpect(U.Drawer == 0 && P.RL.CurrentPage() == Page::Promotions, "ignite g2: the odds drawer's scrim swallows the click");
	// SPIN goes to core: a refusal shows its reason and starts no spin.
	L = P.Settled();
	const std::string SpinWhy = S.SpinWheel();
	ClickRun(P, L, "Spin \xC2\xB7 SHINE");
	if (!SpinWhy.empty())
	{
		IgExpect(!U.Spin && U.Note == SpinWhy, "ignite g2: SPIN asks core; a refusal starts no spin");
	}
	// The level-up banner holds its rect: a one-frame click over the wheel under it reaches nothing.
	P.Open(Page::Lobby);
	P.Settled();
	ig::IgniteLoops G = F;
	G.Level = 8;
	G.Title = "Rookie";
	ig::IgniteFakeLoops(&P.RL, &G);
	P.Frame();
	IgExpect(ig::IgniteBannerUp(U, P.Now) && U.LevelShown == 8, "ignite g2: a level-up drops the banner");
	L = P.Frame();
	WriteList("ignite_g2_levelup", L);
	IgExpect(DrewRun(L, "LEVEL 8"), "ignite g2: the banner says the level");
	P.OneFrame(790.0f, 200.0f);
	P.Frame();
	IgExpect(P.RL.CurrentPage() == Page::Lobby, "ignite g2: the banner holds the pointer over its rect (the wheel under it stays shut)");
	P.Now += 4.0;
	P.Frame();
	IgExpect(!ig::IgniteBannerUp(U, P.Now), "ignite g2: the banner dismisses itself at 3.5 s");
	// The FUSE reveal is modal over the whole client: a one-frame click is its own (a skip), never the page's.
	ss::net::EventInstance E;
	if (IgOpenEvent(S, E, 2200))
	{
		S.RegisterEvent(ss::net::Site(SiteId::Ignite).Listing(E, nullptr, S.Unlocks()));
		S.ShowLobby();
		P.Open(Page::Lobby);
		P.Settled();
		ig::IgniteStartReveal(U, S, 0, P.Now);
		P.Frame();
		P.Now += 0.7;
		L = P.Frame();
		WriteList("ignite_g2_reveal", L);
		const std::string Sel = P.RL.SelectedEvent();
		P.OneFrame(Bx, By);
		P.Frame();
		IgExpect(P.RL.CurrentPage() == Page::Lobby && P.RL.SelectedEvent() == Sel, "ignite g2: the reveal swallows a one-frame click (nothing under it fires)");
		IgExpect(ig::IgniteRevealTimesFor(2).BoomEnd == 1.5 && ig::IgniteRevealTimesFor(10).BoomEnd == 3.4 && ig::IgniteRevealTimesFor(25).BoomEnd == 6.3,
			"ignite g2: the reveal's length by multiplier (1.5 s, 3.4 s, 6.3 s)");
		P.Now += 8.0;
		P.Frame();
		P.Frame();
		IgExpect(U.RevealEvent.empty(), "ignite g2: the reveal ends itself and seats you");
	}
	else
	{
		std::printf("  ignite g2: no open event at this hour; the reveal's checks are skipped\n");
	}
}

/** The table's people (F8, F9): a Headliner's spotlight and star, your Beef's pin, PAYBACK and the Ouch bubble slammed once. */
void TablePeople()
{
	IgLaptop P("ignite-g2-table");
	ss::Session& S = P.S;
	P.At(IgPrime - 60.0);
	S.BankrollCents = 200000;
	P.Join(50000);
	ss::net::EventInstance E;
	if (!IgOpenEvent(S, E, 2200))
	{
		std::printf("  ignite g2: no open event at this hour; the table's people checks are skipped\n");
		return;
	}
	S.RegisterEvent(ss::net::Site(SiteId::Ignite).Listing(E, nullptr, S.Unlocks()));
	S.CurrentPace = ss::Pace::Full;
	P.Now = Step(S, P.Now, 3.0);
	std::vector<std::string> Names;
	for (const ss::SeatVis& Seat : S.Seats)
	{
		if (Seat.Present && !Seat.IsHero)
		{
			Names.push_back(Seat.Name);
		}
	}
	if (Names.size() < 2)
	{
		std::printf("  ignite g2: fewer than two others seated; the table's people checks are skipped\n");
		return;
	}
	ig::IgniteLoops F = Fake(true);
	F.Headliners = {Names[0]};
	F.BeefName = Names[1];
	F.Beefs.front().Name = Names[1];
	ig::IgniteMoment Pay;
	Pay.Kind = 0;
	Pay.Name = Names[1];
	Pay.HandId = "test-hand-1";
	Pay.Sparks = 500;
	Pay.World = S.WorldMinutes();
	ig::IgniteMoment Ouch;
	Ouch.Kind = 2;
	Ouch.HandId = "test-hand-2";
	Ouch.Sparks = 300;
	Ouch.Equity = 0.87;
	Ouch.World = S.WorldMinutes();
	F.Moments = {Pay, Ouch};
	ig::IgniteFakeLoops(&P.RL, &F);
	const int Pops = P.H.Count(ss::SoundId::IgPop);
	P.Frame();
	P.Now += 0.4;
	const DrawList L = P.Frame();
	WriteList("ignite_g2_table_people", L);
	IgExpect(DrewRun(L, "BEEF") && DrewRun(L, "PAYBACK") && DrewRun(L, "Ouch. That was an 87% favourite. Have some Sparks."), "ignite g2: the table's BEEF pin, PAYBACK and the Ouch bubble");
	IgExpect(P.H.Count(ss::SoundId::IgPop) >= Pops + 1, "ignite g2: a slam pops (IgPop), once a moment");
	std::printf("  ignite g2 table with its people (a spotlight, pins, PAYBACK, Ouch): %zu vertices\n", L.Vertices.size());
	IgExpect(L.Vertices.size() <= 22000, "ignite g2: the table with its people within 22k vertices");
	P.Now += 3.0;
	const DrawList L2 = P.Frame();
	IgExpect(!DrewRun(L2, "PAYBACK"), "ignite g2: a slam is gone after its moment");
}

/** W5 for the reveal's reel: it lands on the multiplier core drew, and nothing bigger passes in its slow last stretch. */
void Reel()
{
	bool Lands = true;
	bool Honest = true;
	for (int Tier = 0; Tier < 3; ++Tier)
	{
		for (const ss::fuse::Row& Row : ss::fuse::Table())
		{
			const int M = std::max(2, ss::fuse::MOf(Row, Tier));
			const int Final = ig::IgniteReelFinal(M, Tier);
			Lands = Lands && ig::IgniteReelAt(M, Tier, Final) == M;
			for (int I = Final - 6; I < Final; ++I)
			{
				Honest = Honest && ig::IgniteReelAt(M, Tier, I) <= M;
			}
		}
	}
	IgExpect(Lands, "ignite g2: the reel lands on core's multiplier at every tier");
	IgExpect(Honest, "ignite g2: the reel never slows past a prize you didn't win (W5: its last six symbols are smaller)");
}

/** The stream preview with the loops on: a still, no sound, nothing claimed. */
void Preview()
{
	IgLaptop P("ignite-g2-preview");
	ss::Session& S = P.S;
	P.At(IgPrime);
	S.BankrollCents = 150000;
	P.Join(21240);
	const ig::IgniteLoops F = Fake(true);
	ig::IgniteFakeLoops(&P.RL, &F);
	P.Open(Page::Promotions);
	P.Settled();
	ig::IgniteUi& U = ig::IgniteOf(&P.RL);
	const int Sounds = P.H.Ignite();
	ig::Access::SetPreviewing(P.RL, true);
	const DrawList A = P.Frame();
	P.Now += 0.7;
	const DrawList B = P.Frame();
	ig::Access::SetPreviewing(P.RL, false);
	IgExpect(P.H.Ignite() == Sounds && !U.Spin && !U.Crack && U.Drawer == 0, "ignite g2: previewing plays nothing and opens no overlay");
	// A still: the client's words (the laptop's taskbar ticker under it scrolls on its own) are the same 0.7 s apart.
	size_t Same = 0;
	size_t Total = 0;
	for (const ss::ui::DrawCmd& Cmd : B.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.BaselineY < 960.0f)
		{
			++Total;
			Same += DrewRun(A, Cmd.Text.Text) ? 1 : 0;
		}
	}
	IgExpect(Total > 40 && Same == Total, "ignite g2: the preview's REWARDS holds still between frames");
}

/** The pages on core's own View (no stand-in): a paid entry lights the loops, SPIN and PULL go to core and the overlays
 * land where core drew, LIGHT IT's reveal shows core's multiplier and the 3-max table's marquee its exact pool. */
void Live()
{
	IgLaptop P("ignite-g2-live");
	ss::Session& S = P.S;
	P.At(IgPrime - 60.0);
	S.BankrollCents = 200000;
	P.Join(50000);
	P.Open(Page::Lobby);
	P.Settled();
	const ig::IgniteLoops L0 = ig::IgniteLoopsFrom(S, S.WorldMinutes());
	if (!L0.Live)
	{
		std::printf("  ignite g2: core's View isn't on in this build; the live checks are skipped\n");
		return;
	}
	ig::IgniteUi& U = ig::IgniteOf(&P.RL);
	// A paid $1.10 entry.
	ss::net::EventInstance E;
	bool Paid = false;
	const ss::net::Network& Net = ss::net::Site(SiteId::Ignite);
	for (const ss::net::EventInstance& Ei : Net.Window(S.WorldMinutes() - 120.0, S.WorldMinutes() + 60.0))
	{
		const ss::net::LiveState Ls = Net.Live(Ei, S.WorldMinutes());
		const ss::LobbyEvent Ev = Net.Listing(Ei, nullptr, S.Unlocks());
		if (!Paid && (Ls.St == ss::net::Status::LateReg || Ls.St == ss::net::Status::Registering) && Ev.Joinable && Ev.BuyInCents == 110)
		{
			Paid = S.RegisterEvent(Ev).empty();
			E = Ei;
		}
	}
	if (!Paid)
	{
		std::printf("  ignite g2: no $1.10 entry open at this hour; the live checks are skipped\n");
		return;
	}
	S.ShowLobby();
	P.Open(Page::Lobby);
	P.Frame();
	P.Now += 0.3;
	DrawList L = P.Frame();
	const ig::IgniteLoops L1 = ig::IgniteLoopsFrom(S, S.WorldMinutes());
	std::printf("  ignite g2 live: a paid $1.10 entry: %lld Sparks, %d rings lit, level %d, %zu crackers, spin %s\n", static_cast<long long>(L1.Sparks), L1.RingsLit, L1.Level,
		L1.Crackers.size(), L1.SpinReady ? "ready" : "not ready");
	IgExpect(L1.Sparks > L0.Sparks && DrewRun(L, ss::Grouped(L1.Sparks)), "ignite g2 live: a paid entry's Sparks land in the marquee's pill (core's figure)");
	IgExpect(DrewRun(L, "+" + ss::Grouped(L1.Sparks - L0.Sparks)), "ignite g2 live: \"+N\" floats up as they land");
	WriteList("ignite_g2_live_play", L);
	// SPIN goes to core; the wheel lands on core's wedge.
	U.Rewards = 0;
	P.Open(Page::Promotions);
	L = P.Settled();
	if (L1.SpinReady && ClickRun(P, L, std::string("Spin \xC2\xB7 ") + ss::ui::igniteart::IgniteRingName(L1.RingsLit)))
	{
		const ig::IgniteLoops L2 = ig::IgniteLoopsFrom(S, S.WorldMinutes());
		IgExpect(U.Spin && L2.Spun && L2.SpunWedge >= 0, "ignite g2 live: SPIN asks core, which draws; the spin starts on its draw");
		P.Now += 3.5;
		L = P.Frame();
		WriteList("ignite_g2_live_spin", L);
		if (L2.SpunRing >= 1 && L2.SpunWedge >= 0 && L2.SpunWedge < static_cast<int>(L2.Rings[static_cast<size_t>(L2.SpunRing - 1)].size()))
		{
			const ig::IgniteWedge& W = L2.Rings[static_cast<size_t>(L2.SpunRing - 1)][static_cast<size_t>(L2.SpunWedge)];
			const std::string Lab = W.TicketId.empty() ? ss::Grouped(W.Sparks) : std::string("TICKET");
			IgExpect(CentredAt(L, Lab, P.M, 800.0f, 4.0f, 100.0f, 320.0f), "ignite g2 live: the wheel stops on core's wedge, under the flapper");
		}
		P.OneFrame(974.0f, 916.0f);
		P.Settled();
		L = P.Settled();
		const std::string Again = S.SpinWheel();
		IgExpect(!U.Spin && !Again.empty() && DrewRun(L, "Spun today"), "ignite g2 live: a second spin in a day is refused in core (the button says so)");
	}
	// A cracker (the first entry is a level-up, F6): PULL asks core; the prize card says what core opened.
	const ig::IgniteLoops L3 = ig::IgniteLoopsFrom(S, S.WorldMinutes());
	if (!L3.Crackers.empty())
	{
		L = P.Settled();
		ClickRun(P, L, "OPEN");
		L = P.Settled();
		IgExpect(U.Crack && ClickRun(P, L, "Pull"), "ignite g2 live: OPEN puts the cracker up, PULL is there");
		const ig::IgniteLoops L4 = ig::IgniteLoopsFrom(S, S.WorldMinutes());
		IgExpect(L4.Crackers.size() + 1 == L3.Crackers.size() && !U.Cracked.empty(), "ignite g2 live: PULL opens it in core (the shelf is one shorter)");
		P.Now += 1.0;
		L = P.Frame();
		WriteList("ignite_g2_live_cracker", L);
		IgExpect(!U.Cracked.empty() && DrewRun(L, U.Cracked.front().Prize), "ignite g2 live: the prize card says what core opened");
		U.Crack = false;
	}
	// LIGHT IT: the reveal plays core's multiplier, then the 3-max table's marquee reads its exact pool.
	if (L3.FuseOn)
	{
		P.Open(Page::Fuse);
		L = P.Settled();
		const int Tables = S.TableCount();
		IgExpect(ClickRun(P, L, "Light it \xC2\xB7 $1.10"), "ignite g2 live: LIGHT IT on the $1.10 cabinet");
		if (S.TableCount() > Tables && !U.RevealEvent.empty())
		{
			const ss::fuse::Reveal Rv = S.FuseReveal(U.RevealTable);
			IgExpect(ss::fuse::IsFuse(U.RevealEvent) && Rv.M >= 2, "ignite g2 live: the reveal is core's FUSE (its table, its multiplier)");
			P.Now += ig::IgniteRevealTimesFor(Rv.M).ReelEnd + 0.5;
			L = P.Frame();
			WriteList("ignite_g2_live_reveal", L);
			IgExpect(DrewRun(L, ss::Grouped(Rv.M)), "ignite g2 live: the reel lands on core's multiplier");
			P.Now += 3.0;
			P.Frame();
			L = P.Frame();
			IgExpect(U.RevealEvent.empty() && S.CurrentScreen == ss::Screen::Table, "ignite g2 live: the table opens itself after the boom");
			L = P.Frame();
			WriteList("ignite_g2_live_fusetable", L);
			IgExpect(DrewRun(L, "PRIZE") && DrewRun(L, ss::Money(static_cast<ss::Chips>(Rv.M) * 110)), "ignite g2 live: the FUSE table's marquee reads exactly M x the buy-in");
			IgPage(L, "ignite_g2_live_fusetable");
			std::printf("  ignite g2 live: the FUSE table (marquee, fuse clock): %zu vertices, %d runs (the table's budget is 22k)\n", L.Vertices.size(), Runs(L));
			IgExpect(L.Vertices.size() <= 22000, "ignite g2 live: the FUSE table within the table's 22k vertices");
			// A level later the fuse is out: BLINDS UP! slams once on the new level.
			if (S.T)
			{
				U.BlindsLevel = S.T->LevelIndex - 1;
				U.BlindsTable = S.T->Spec.Id;
				P.Frame();
				P.Now += 0.3;
				L = P.Frame();
				IgExpect(IgStartsWith(L, "BLINDS UP!"), "ignite g2 live: BLINDS UP! slams on a new level");
			}
		}
		else
		{
			std::printf("  ignite g2 live: LIGHT IT said \"%s\"\n", U.Note.c_str());
		}
	}
}

/** How many text runs are exactly Text. */
int CountRuns(const DrawList& L, const std::string& Text)
{
	int N = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		N += Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text ? 1 : 0;
	}
	return N;
}

/**
 * The G2 review's checks (each fails on the hand-in): a Wristband tier with nothing on it never wears CLAIM (core refuses that
 * claim and never marks it claimed, so the Today preview mustn't stick at T1); a Dare met but short of today's paid entries
 * says so; a ticket entry's sheet promises no Sparks; "Your run" shows a rewards ticker only for the entry it closed; the
 * Spare Match at its cap isn't "in your Locker".
 */
void Review()
{
	// The Wristband on core's own book: base Sparks to T8 (T5's cracker and T8's Spare Match wait; T1-T4, T6, T7 hold nothing).
	{
		IgLaptop P("ignite-g2-review-band");
		ss::Session& S = P.S;
		P.At(IgPrime - 60.0);
		S.BankrollCents = 200000;
		P.Join(50000);
		ss::net::EventInstance E;
		if (ig::IgniteLoopsFrom(S, S.WorldMinutes()).Live && IgOpenEvent(S, E, 2200))
		{
			ss::site::play::OnPaidEntry(S.Life.SitePlay, S.Life, SiteId::Ignite, 590, 5500, 0, E.Id, S.WorldMinutes());
			const ig::IgniteLoops Lv = ig::IgniteLoopsFrom(S, S.WorldMinutes());
			// Counted here from core's own fields (not through the pages' helper): reached, unclaimed, with something on it.
			int Waiting = 0;
			int Reached = 0;
			for (const ig::IgniteTier& T : Lv.Tiers)
			{
				Waiting += T.Reached && !T.Claimed && T.Kind != 0 ? 1 : 0;
				Reached += T.Reached ? 1 : 0;
			}
			std::printf("  ignite g2 review: core's book at T%d (%lld base Sparks this month), %d tiers reached, %d wait on a claim\n", Lv.Tier, static_cast<long long>(Lv.BandSparks), Reached,
				Waiting);
			IgExpect(Lv.Tier >= 5 && Waiting > 0 && Waiting < Reached && !Lv.Tiers[0].Claimed && !S.ClaimTier(1).empty(),
				"ignite g2 review: core refuses a claim on a tier with nothing and never marks it claimed");
			P.Open(Page::Promotions);
			P.Settled(); // the laptop's first frame starts its side state afresh: the sub-page is picked after it
			ig::IgniteOf(&P.RL).Rewards = 1;
			DrawList L = P.Settled();
			IgExpect(DrewRun(L, "WRISTBAND \xC2\xB7 KINDLING") || IgStartsWith(L, "WRISTBAND \xC2\xB7 "), "ignite g2 review: REWARDS' Wristband sub-page is up");
			IgExpect(CountRuns(L, "CLAIM") == Waiting, "ignite g2 review: REWARDS' Wristband: CLAIM only on tiers with something on them");
			WriteList("ignite_g2_live_band", L);
			ig::IgniteOf(&P.RL).Rewards = 0;
			L = P.Settled();
			IgExpect(CountRuns(L, "CLAIM") == Waiting, "ignite g2 review: REWARDS' Today: the Wristband preview starts at the first tier with a claim (not stuck at T1)");
			WriteList("ignite_g2_live_rewards", L);
			// Core's near-goal words end a sentence; the wheel's line puts ", or spin now" before the full stop.
			bool Stop = false;
			bool Spin = false;
			for (const ss::ui::DrawCmd& Cmd : L.Cmds)
			{
				if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
				{
					Stop = Stop || Cmd.Text.Text.find("., or spin now") != std::string::npos;
					Spin = Spin || Cmd.Text.Text.find(", or spin now") != std::string::npos;
				}
			}
			IgExpect(Spin && !Stop, "ignite g2 review: the wheel's line reads \"1 more entry lights SHINE, or spin now\" (no stop mid-sentence)");
		}
		else
		{
			std::printf("  ignite g2 review: core's View isn't on or no event is open; the Wristband's live checks are skipped\n");
		}
	}
	IgLaptop P("ignite-g2-review");
	ss::Session& S = P.S;
	P.At(IgPrime - 60.0);
	S.BankrollCents = 200000;
	P.Join(50000);
	ig::IgniteUi& U = ig::IgniteOf(&P.RL);
	// A Dare whose own target is met waits on today's paid entries (core's floor: 2 for a Medium): it says how many more.
	{
		ig::IgniteLoops F = Fake(true);
		F.EntriesToday = 1;
		F.RingsLit = 1;
		F.Dares[1].Prog = 1;
		F.Dares[1].Needs = 1;
		F.Dares[1].Entries = 2;
		F.Dares[1].Done = false;
		ig::IgniteFakeLoops(&P.RL, &F);
		P.Open(Page::Lobby);
		DrawList L = P.Settled();
		IgExpect(DrewRun(L, "MEDIUM DARE \xC2\xB7 1 MORE ENTRY"), "ignite g2 review: the Today strip's Dare chip says a met Dare needs 1 more paid entry");
		U.Rewards = 0;
		P.Open(Page::Promotions);
		L = P.Settled();
		IgExpect(DrewRun(L, "1 MORE ENTRY") && CountRuns(L, "Claim") == 1, "ignite g2 review: REWARDS' Dare card says 1 more entry where its CLAIM would be (no Claim)");
		WriteList("ignite_g2_dare_short", L);
	}
	// The Spare Match at its cap of two.
	{
		ig::IgniteLoops F = Fake(true);
		F.Prizes.back().Owned = true;
		ig::IgniteFakeLoops(&P.RL, &F);
		U.Rewards = 2;
		U.PrizeTab = 2;
		P.Open(Page::Promotions);
		const DrawList L = P.Settled();
		IgExpect(DrewRun(L, "2 held \xC2\xB7 the most") && !DrewRun(L, "In your Locker"), "ignite g2 review: the Spare Match at two held isn't \"in your Locker\"");
		U.PrizeTab = 0;
	}
	// "Your run": the ticker belongs to the entry the result closed, never a later ticket entry of the same event.
	{
		const ss::net::Network& Net = ss::net::Site(SiteId::Ignite);
		ss::net::EventInstance E;
		if (IgOpenEvent(S, E, 2200))
		{
			const std::string Name = Net.TemplateOf(E).Name;
			ig::IgniteLoops F = Fake(true);
			F.Last.EventId = E.Id;
			F.Last.Base = 120;
			F.Last.Sparks = 120;
			F.Last.Flame = 1.0;
			ig::IgniteFakeLoops(&P.RL, &F);
			ss::HistoryEntry H;
			H.Name = Name;
			H.Place = 40;
			H.Entrants = 200;
			H.BuyInCents = 110;
			H.EventId = E.Id;
			S.History.insert(S.History.begin(), H);
			S.LastResults = ss::Results();
			S.LastResults.EventName = Name;
			S.LastResults.Place = 40;
			S.LastResults.Entrants = 200;
			S.LastResults.BuyInCents = 110;
			S.HasResults = true;
			S.LastSiteResult = SiteId::Ignite;
			S.CurrentScreen = ss::Screen::Results;
			P.RL.SiteShown = SiteId::Ignite;
			DrawList L = P.Settled();
			const bool Paid = IgStartsWith(L, "(120");
			// The same event's next start, played on a ticket: History's newest line is that one.
			S.History.front().EventId = E.Id + "0";
			S.ResultsAt += 1.0;
			L = P.Settled();
			const bool Ticket = IgStartsWith(L, "(120");
			// A ticket FUSE at the stake after the paid one (the ordinal moved on) and the paid one itself.
			F.Last.EventId = "ignite.fuse0@44400#5";
			F.Fuse.N = {6, 0, 0};
			ig::IgniteFakeLoops(&P.RL, &F);
			S.LastResults.EventName = ss::fuse::NameOf(0);
			S.ResultsAt += 1.0;
			L = P.Settled();
			const bool FuseTicket = IgStartsWith(L, "(120");
			F.Fuse.N = {5, 0, 0};
			ig::IgniteFakeLoops(&P.RL, &F);
			S.ResultsAt += 1.0;
			L = P.Settled();
			const bool FusePaid = IgStartsWith(L, "(120");
			std::printf("  ignite g2 review: the rewards ticker: paid %d, a ticket entry %d, a ticket FUSE %d, the paid FUSE %d\n", Paid, Ticket, FuseTicket, FusePaid);
			IgExpect(Paid && !Ticket && FusePaid && !FuseTicket, "ignite g2 review: \"Your run\" shows the Sparks only of the entry it closed (a ticket entry earns nothing, W4)");
			S.HasResults = false;
			S.History.erase(S.History.begin());
			S.CurrentScreen = ss::Screen::Lobby;
		}
		else
		{
			std::printf("  ignite g2 review: no open event at this hour; the ticker's checks are skipped\n");
		}
	}
	// The sheet of an event a held ticket will play: no Sparks promised.
	{
		ig::IgniteFakeLoops(&P.RL, nullptr);
		ss::net::EventInstance E;
		if (ig::IgniteLoopsFrom(S, S.WorldMinutes()).Live && IgOpenEvent(S, E, 2200))
		{
			const ss::net::Network& Net = ss::net::Site(SiteId::Ignite);
			P.Open(Page::Lobby);
			P.Settled();
			ig::IgniteOpenSheet(P.RL, E.Id, P.Now);
			DrawList L = P.Settled();
			const bool Earns = IgStartsWith(L, "EARNS ");
			const size_t At = E.Id.find('@');
			S.Life.Tickets[Net.TicketOf(E.Id.substr(0, At))] = 1;
			L = P.Settled();
			IgExpect(Earns && !IgStartsWith(L, "EARNS ") && DrewRun(L, "TICKET ENTRY \xC2\xB7 NO SPARKS"), "ignite g2 review: a ticket entry's sheet promises no Sparks (W4)");
			WriteList("ignite_g2_sheet_ticket", L);
			S.Life.Tickets.erase(Net.TicketOf(E.Id.substr(0, At)));
			U.Sheet = false;
		}
		else
		{
			std::printf("  ignite g2 review: core's View isn't on or no event is open; the ticket sheet's check is skipped\n");
		}
	}
	// LIGHT IT at $5.50 (forced to x5): the 3-max table's marquee reads exactly 5 x $5.50, the table's own stake.
	{
		IgLaptop Pf("ignite-g2-review-fuse55");
		ss::Session& Sf = Pf.S;
		Pf.At(IgPrime - 60.0);
		Sf.BankrollCents = 200000;
		Pf.Join(50000);
		if (ig::IgniteLoopsFrom(Sf, Sf.WorldMinutes()).FuseOn)
		{
			ss::fuse::DebugForce(Sf, 2, 5);
			Pf.Open(Page::Fuse);
			DrawList L = Pf.Settled();
			ig::IgniteUi& Uf = ig::IgniteOf(&Pf.RL);
			const bool Lit = ClickRun(Pf, L, "Light it \xC2\xB7 $5.50");
			const ss::fuse::Reveal Rv = Uf.RevealTable >= 0 ? Sf.FuseReveal(Uf.RevealTable) : ss::fuse::Reveal();
			Pf.Now += 6.0;
			Pf.Frame();
			Pf.Frame();
			L = Pf.Frame();
			std::printf("  ignite g2 review: LIGHT IT at $5.50: x%d at tier %d, screen %d\n", Rv.M, Rv.Tier, static_cast<int>(Sf.CurrentScreen));
			IgExpect(Lit && Rv.M == 5 && Rv.Tier == 2 && Sf.CurrentScreen == ss::Screen::Table && DrewRun(L, ss::Money(5 * 550)) && !DrewRun(L, ss::Money(5 * 110)),
				"ignite g2 review: the FUSE table's marquee reads exactly M x its own buy-in ($27.50 at x5, $5.50)");
		}
		else
		{
			std::printf("  ignite g2 review: FUSE isn't lit in this build; the $5.50 marquee check is skipped\n");
		}
	}
	// Under the clause the adapter carries core's on-ice (derived from the deal while the book is empty, S10): frost, ON ICE.
	{
		IgLaptop Pc("ignite-g2-review-ice");
		ss::Session& Sc = Pc.S;
		Pc.At(IgPrime - 60.0);
		Sc.BankrollCents = 200000;
		Pc.Join(50000);
		if (ig::IgniteLoopsFrom(Sc, Sc.WorldMinutes()).Live && IgSignClause(Sc))
		{
			const ig::IgniteLoops Li = ig::IgniteLoopsFrom(Sc, Sc.WorldMinutes());
			ig::IgniteOf(&Pc.RL).Rewards = 0;
			Pc.Open(Page::Promotions);
			Pc.Settled();
			Pc.Now += 12.0; // the deal's badge toast has gone
			const DrawList L = Pc.Settled();
			IgExpect(Li.OnIce && DrewRun(L, ig::IgniteOnIce) && DrewRun(L, "ON ICE"), "ignite g2 review: under the clause the live pages show core's on-ice (the Flame waits, the dock says so)");
		}
		else
		{
			std::printf("  ignite g2 review: the clause wouldn't bind or the View isn't on; the live on-ice check is skipped\n");
		}
	}
}

/** review-core-fuse: a FUSE never enters History, so "Your run"'s Play again is the next FUSE at the same stake (§2.4 G):
 *  enabled, it names the stake and opens FUSE's page (where LIGHT IT is the strike gate); it lights nothing itself. */
void FuseAgain()
{
	IgLaptop P("ignite-g2-review-fuse-again");
	ss::Session& S = P.S;
	P.At(IgPrime);
	S.BankrollCents = 123400;
	P.Join(21240);
	S.LastResults = ss::Results();
	S.LastResults.EventName = ss::fuse::NameOf(1);
	S.LastResults.Place = 2;
	S.LastResults.Entrants = 3;
	S.LastResults.PrizeCents = 0;
	S.LastResults.BuyInCents = 330;
	S.HasResults = true;
	S.LastSiteResult = SiteId::Ignite;
	S.CurrentScreen = ss::Screen::Results;
	// The lobby's site is left at RiverLine (tiled, the last tab the player opened was RiverLine's): Play again still lands on
	// Ignite's FUSE page, never RiverLine's lobby (SitesPageHome sends FUSE home on any other site).
	const size_t Tables = static_cast<size_t>(S.TableCount());
	const ss::Chips Wallet = ss::site::AccountOf(S.Life.Sites, SiteId::Ignite)->Cents;
	DrawList L = P.Settled();
	const bool Named = DrewRun(L, "Play again") && DrewRun(L, "FUSE $3.30") && !DrewRun(L, "Nothing like it on the floor right now");
	const bool Clicked = ClickRun(P, L, "Play again");
	L = P.Settled();
	IgExpect(Named && Clicked && P.RL.CurrentPage() == Page::Fuse && S.CurrentScreen != ss::Screen::Results && static_cast<size_t>(S.TableCount()) == Tables &&
			ss::site::AccountOf(S.Life.Sites, SiteId::Ignite)->Cents == Wallet,
		"ignite g2 review: after a FUSE, Play again names its stake and opens FUSE's page (nothing lit, nothing paid)");
}

void Bench()
{
	// PLAY with the loops on, timed like IgPlayBench (printed; SS_UI_BENCH=1 for the timing).
	if (!std::getenv("SS_UI_BENCH"))
	{
		return;
	}
	IgLaptop P("ignite-g2-bench");
	P.At(IgPrime);
	P.S.BankrollCents = 150000;
	P.Join(21240);
	const ig::IgniteLoops F = Fake(true);
	ig::IgniteFakeLoops(&P.RL, &F);
	for (const Page Pg : {Page::Lobby, Page::Promotions, Page::Fuse})
	{
		P.Open(Pg);
		P.Settled();
		std::vector<double> Ms;
		for (int I = 0; I < 120; ++I)
		{
			P.Now += 1.0 / 30.0;
			const auto T0 = std::chrono::steady_clock::now();
			P.Frame();
			Ms.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count());
		}
		std::sort(Ms.begin(), Ms.end());
		std::printf("  SS_UI_BENCH ignite g2 page %d: median %.3f ms, p90 %.3f ms\n", static_cast<int>(Pg), Ms[Ms.size() / 2], Ms[Ms.size() * 9 / 10]);
	}
}

/** Whether every BlackItalic run among Labels drawn inside a wheel's rim (centre Cx, Cy, radius R) keeps off its rings: its
 *  glyph box wholly in the band between SHINE (0.7 R, stroked 2 either side) and GLOW's rim (R + 6). Found counts them. */
bool OffRings(const DrawList& L, const TableMeasurer& M, float Cx, float Cy, float R, const std::set<std::string>& Labels, int& Found)
{
	bool Ok = true;
	Found = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || Cmd.Text.Face != ss::ui::Font::BlackItalic || Labels.count(Cmd.Text.Text) == 0)
		{
			continue;
		}
		const ss::ui::TextItem& T = Cmd.Text;
		const float X0 = T.X;
		const float X1 = T.X + M.Width(T.Text, T.Face, T.SizePx);
		const float Y0 = T.BaselineY - 0.72f * T.SizePx;
		const float Y1 = T.BaselineY;
		if (std::hypot((X0 + X1) / 2.0f - Cx, (Y0 + Y1) / 2.0f - Cy) > R + 6.0f)
		{
			continue;
		}
		++Found;
		const float Near = std::hypot(std::min(std::max(Cx, X0), X1) - Cx, std::min(std::max(Cy, Y0), Y1) - Cy);
		const float Far = std::max(std::hypot(std::max(std::fabs(X0 - Cx), std::fabs(X1 - Cx)), std::max(std::fabs(Y0 - Cy), std::fabs(Y1 - Cy))), 0.0f);
		if (Near < R * 0.7f + 2.0f || Far > R + 4.0f)
		{
			std::printf("  ignite fix: \"%s\" at %.1f-%.1f from the wheel's centre (band %.1f-%.1f)\n", T.Text.c_str(), Near, Far, R * 0.7f + 2.0f, R + 4.0f);
			Ok = false;
		}
	}
	return Ok;
}

/** A paid $1.10 Ignite entry open at this minute (a live career's first), registered; false when none is open. */
bool PayOneTen(ss::Session& S)
{
	const ss::net::Network& Net = ss::net::Site(SiteId::Ignite);
	for (const ss::net::EventInstance& Ei : Net.Window(S.WorldMinutes() - 120.0, S.WorldMinutes() + 60.0))
	{
		const ss::net::LiveState Ls = Net.Live(Ei, S.WorldMinutes());
		const ss::LobbyEvent Ev = Net.Listing(Ei, nullptr, S.Unlocks());
		if ((Ls.St == ss::net::Status::LateReg || Ls.St == ss::net::Status::Registering) && Ev.Joinable && Ev.BuyInCents == 110 && S.RegisterEvent(Ev).empty())
		{
			return true;
		}
	}
	return false;
}

/**
 * The G2 fix round (G2_AUDIT M2, M5, L9): the Crackers' odds one click away (core's own tables), a split pot's lighter
 * fountains, the Flame's week before the book, the wheel's prizes off its rings, the reveal's "1 in 250" and its ODDS pill,
 * and the vector cross wherever copy says "x25".
 */
void FixRound()
{
	// L9: the glyph rule. Copy writes "x25"; a drawn label draws the vector cross in its place.
	IgExpect(ig::IgniteHasTimes("x25") && ig::IgniteHasTimes("80 / 12 / 8 from x25") && ig::IgniteHasTimes("(610 x 1.20 FLAME)") && ig::IgniteHasTimes("FUSE x25: 1st") &&
			!ig::IgniteHasTimes("max5") && !ig::IgniteHasTimes("6x the Sparks") && !ig::IgniteHasTimes("Flip & Run") && !ig::IgniteHasTimes("box 5") && !ig::IgniteHasTimes("x-ray"),
		"ignite fix: a multiplier in copy is an 'x' that starts a word before a digit");
	// L9: the reveal's line under BIG FUSE, from the multiplier's own row of core's table at its stake: a whole one-in where the
	// chance is one to within 0.5%, else the percentage (every digit a fact; review-g2f-ignite).
	{
		const auto PctAt = [](int M, int Tier) {
			for (const ss::fuse::Row& Row : ss::fuse::Table())
			{
				if (ss::fuse::MOf(Row, Tier) == M)
				{
					return ss::Fixed(ss::fuse::POf(Row, Tier) * 100.0, 1) + "% at " + ss::Money(ss::fuse::BuyInOf(Tier));
				}
			}
			return std::string("?");
		};
		IgExpect(ig::IgniteRevealOdds(25, 0) == "1 in 250 at $1.10" && ig::IgniteRevealOdds(10, 1) == "1 in 50 at $3.30" && ig::IgniteRevealOdds(100, 0) == "1 in 2,000 at $1.10" &&
				ig::IgniteRevealOdds(100, 1) == "1 in 1,667 at $3.30" && ig::IgniteRevealOdds(1000, 0) == "1 in 20,000 at $1.10" && ig::IgniteRevealOdds(250, 2) == "1 in 20,000 at $5.50" &&
				ig::IgniteRevealOdds(5, 0) == "6.5% at $1.10" && ig::IgniteRevealOdds(2, 0) == PctAt(2, 0) && ig::IgniteRevealOdds(3, 2) == PctAt(3, 2) && ig::IgniteRevealOdds(1000, 1).empty(),
			"ignite fix: the reveal's odds line reads each multiplier's own chance at its stake (1 in 250, 6.5%, none off the table)");
	}
	IgLaptop P("ignite-fix");
	{
		DrawList A;
		DrawList B;
		DrawList Plain;
		ss::ui::Canvas Ca(A, P.M, 1600.0f, 1000.0f, 1.0f);
		ss::ui::Canvas Cb(B, P.M, 1600.0f, 1000.0f, 1.0f);
		ss::ui::Canvas Cp(Plain, P.M, 1600.0f, 1000.0f, 1.0f);
		const ss::ui::TextStyle St = ss::ui::igniteart::IgniteTx(11.5f, 700, ss::ui::Hex(0xfff4ec), ss::ui::Align::Center);
		const float Wa = ig::IgniteLineTimes(Ca, "80 / 12 / 8 from x25", 300.0f, 100.0f, St);
		ig::IgniteLineTimes(Cb, "Three players, one fuse", 300.0f, 100.0f, St);
		Cp.Text("Three players, one fuse", 300.0f, 100.0f, St);
		const float Want = P.M.Width("80 / 12 / 8 from ", ss::ui::Font::Bold, 11.5f) + 11.5f * 0.54f + P.M.Width("25", ss::ui::Font::Bold, 11.5f);
		IgExpect(DrewRun(A, "80 / 12 / 8 from ") && DrewRun(A, "25") && !DrewRun(A, "80 / 12 / 8 from x25") && A.Vertices.size() > 0 && std::fabs(Wa - Want) < 0.01f,
			"ignite fix: a drawn multiplier is the vector cross between its runs (centred as one)");
		IgExpect(B.ToJson() == Plain.ToJson(), "ignite fix: a line with no multiplier draws exactly as it did");
	}
	ss::Session& S = P.S;
	P.At(IgPrime);
	S.BankrollCents = 150000;
	P.Join(21240);
	ig::IgniteLoops F = Fake(true);
	ig::IgniteFakeLoops(&P.RL, &F);
	ig::IgniteUi& U = ig::IgniteOf(&P.RL);
	const float Bx = 974.0f; // the dock's BOARDS: outside every overlay's own rect
	const float By = 916.0f;
	// PLAY's Today strip: the Flame's multiplier is the cross and "1.20" in BlackItalic, never "x1.20".
	P.Open(Page::Lobby);
	DrawList L = P.Settled();
	const IgBox Mult = IgRun(L, "1.20", P.M, 380.0f, 400.0f);
	IgExpect(Mult.Found && Mult.Face == ss::ui::Font::BlackItalic && !DrewRun(L, "x1.20"), "ignite fix: the Today strip's multiplier carries the vector cross");
	// FUSE's booth: "80 / 12 / 8 from" and the how-it-works line cross their multipliers.
	P.Open(Page::Fuse);
	L = P.Settled();
	const auto Lettered = [](const DrawList& Lx) {
		int N = 0;
		for (const ss::ui::DrawCmd& Cmd : Lx.Cmds)
		{
			if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && ig::IgniteHasTimes(Cmd.Text.Text))
			{
				std::printf("  ignite fix: a lettered multiplier is drawn: \"%s\"\n", Cmd.Text.Text.c_str());
				++N;
			}
		}
		return N;
	};
	IgExpect(DrewRun(L, "80 / 12 / 8 from ") && !DrewRun(L, "80 / 12 / 8 from x25") && Lettered(L) == 0, "ignite fix: FUSE's booth crosses every multiplier (the cabinets, how it works)");
	P.Open(Page::News);
	L = P.Settled();
	ClickRun(P, L, "Big hits");
	L = P.Settled();
	IgExpect(DrewRun(L, "25 at your table") && Lettered(L) == 0, "ignite fix: BUZZ's Big hits cross their multipliers");
	// REWARDS: the wheel's prizes ride the band between SHINE and GLOW's rim, off the lit rings, at every band's widths.
	U.Rewards = 0;
	P.Open(Page::Promotions);
	L = P.Settled();
	int Found = 0;
	bool Off = OffRings(L, P.M, 1356.0f, 336.0f, 128.0f, {"40", "80", "150", "400", "TICKET"}, Found);
	IgExpect(Off && Found == 5 && Lettered(L) == 0, "ignite fix: REWARDS' wheel labels sit between its rings (SHINE's five, none on a ring)");
	WriteList("ignite_fix_rewards_wheel", L);
	ig::IgniteLoops G = F;
	G.RingsLit = 3;
	G.Rings[2].clear();
	for (const int64_t Sp : {560, 1050, 2100, 5600})
	{
		ig::IgniteWedge W;
		W.Sparks = Sp;
		W.P = 0.2;
		W.Label = ss::Grouped(Sp) + " Sparks";
		G.Rings[2].push_back(W);
	}
	for (const char* Tk : {"Mystery Pinata $7.70 ticket", "The Big Wave $11 ticket"})
	{
		ig::IgniteWedge W;
		W.Label = Tk;
		W.TicketId = "ignite.fix-ticket";
		W.FaceCents = 770;
		W.P = 0.004;
		G.Rings[2].push_back(W);
	}
	G.SpunRing = 3;
	G.SpunWedge = 3;
	ig::IgniteFakeLoops(&P.RL, &G);
	L = P.Settled();
	const std::set<std::string> Wide = {"560", "1,050", "2,100", "5,600", "TICKET"};
	Off = OffRings(L, P.M, 1356.0f, 336.0f, 128.0f, Wide, Found);
	IgExpect(Off && Found == 6, "ignite fix: band C's DAZZLE labels (5,600 across the wheel) still clear both rings");
	WriteList("ignite_fix_rewards_wheel_c", L);
	ig::IgniteStartSpin(U, G, P.Now);
	bool SpinOff = true;
	int SpinFound = 0;
	for (const double At : {0.4, 1.1, 2.0, 3.6})
	{
		P.Now = U.SpinAt + At;
		L = P.Frame();
		SpinOff = SpinOff && OffRings(L, P.M, 800.0f, 480.0f, 300.0f, Wide, Found);
		SpinFound += Found;
	}
	IgExpect(SpinOff && SpinFound >= 18, "ignite fix: the spin's labels ride between its rings while it turns and where it lands");
	U.Spin = false;
	ig::IgniteFakeLoops(&P.RL, &F);
	// M2 (W5): the Crackers' odds, one click away on the shelf: core's own tables (site::play::CrackerOdds), the ones an open
	// draws from.
	L = P.Settled();
	const IgBox Odds = IgRun(L, "ODDS", P.M, 470.0f, 520.0f);
	if (Odds.Found)
	{
		P.Click(Odds.X + Odds.W / 2.0f, Odds.Base - 6.0f);
	}
	L = P.Settled();
	IgExpect(Odds.Found && U.Drawer == 4 && DrewRun(L, "CRACKERS' ODDS"), "ignite fix: the cracker shelf's ODDS opens the Crackers' odds");
	Page2(L, "ignite_fix_crackerodds");
	bool Rows = true;
	for (int T = 0; T < 3; ++T)
	{
		const ss::site::play::CrackerOddsView V = ss::site::play::CrackerOdds(T);
		double Sum = 0.0;
		for (const ss::site::play::CrackerOddsRow& Rw : V.Rows)
		{
			const std::string Pct = ss::Fixed(Rw.P * 100.0, Rw.P > 0.0 && Rw.P < 0.01 ? 2 : 1) + "%";
			if (Rw.Part == 0)
			{
				Sum += Rw.P;
				Rows = Rows && DrewRun(L, Rw.Label) && DrewRun(L, Pct);
			}
			else if (Rw.Part == 1)
			{
				Rows = Rows && DrewRun(L, "+ " + Rw.Label) && DrewRun(L, Pct);
			}
			else
			{
				Rows = Rows && DrewRun(L, "+ a look piece (one of 6, " + Pct + " each)");
			}
		}
		Rows = Rows && DrewRun(L, V.Name) && std::fabs(Sum - 1.0) < 1e-9;
		if (!Rows)
		{
			std::printf("  ignite fix: the %s's rows aren't all drawn as core has them\n", V.Name.c_str());
		}
	}
	IgExpect(Rows, "ignite fix: every Cracker's chances are core's own rows (the Sparks summing to 100%, the ticket and the looks on top)");
	IgExpect(IgStartsWith(L, "A look you already own comes as " + ss::Grouped(ss::site::play::CrackerOdds(1).DuplicateSparks)), "ignite fix: the drawer says what a look already owned becomes");
	// House law 3: a one-frame click inside the drawer (over the wheel's SPIN under it) fires nothing; its scrim closes it.
	P.OneFrame(1356.0f, 578.0f);
	P.Settled();
	IgExpect(U.Drawer == 4 && !U.Spin, "ignite fix: a one-frame click on the Crackers' odds reaches nothing under it");
	P.OneFrame(Bx, By);
	P.Settled();
	IgExpect(U.Drawer == 0 && P.RL.CurrentPage() == Page::Promotions, "ignite fix: the Crackers' odds' scrim closes it, the click reaches nothing");
	// M5: a split pot's fountains fly lighter past one (6, then 5 each for two, 4 each for three or more), one trail.
	IgExpect(ig::IgniteFountainFor(0) == 6 && ig::IgniteFountainFor(1) == 6 && ig::IgniteFountainFor(2) == 5 && ig::IgniteFountainFor(3) == 4 && ig::IgniteFountainFor(8) == 4,
		"ignite fix: the fountain's chips by how many fly at once");
	ig::IgniteFakeLoops(&P.RL, nullptr);
	{
		IgLaptop Pt("ignite-fix-split");
		if (IgTableAt(Pt, IgPrime - 60.0))
		{
			ss::Session& St = Pt.S;
			std::vector<int> Seats;
			for (const ss::SeatVis& Seat : St.Seats)
			{
				if (Seat.Present && Seats.size() < 3)
				{
					Seats.push_back(Seat.Seat);
				}
			}
			const auto Fly = [&](size_t N) {
				ig::IgniteOf(&Pt.RL).Pots.clear();
				St.Flights.clear();
				const DrawList Plain = Pt.Frame();
				const double T0 = Pt.Now;
				for (size_t K = 0; K < N; ++K)
				{
					ss::Flight Fl;
					Fl.IsChips = true;
					Fl.From = ss::FlightEnd::AtPot();
					Fl.To = ss::FlightEnd::AtSeat(Seats[K]);
					Fl.Amount = 1400;
					Fl.Start = T0;
					Fl.Dur = 0.6;
					St.Flights.push_back(Fl);
				}
				Pt.Now = T0 + 0.25;
				const DrawList Mid = Pt.Frame();
				if (N == 3)
				{
					WriteList("ignite_fix_split3", Mid);
				}
				ig::IgniteOf(&Pt.RL).Pots.clear();
				St.Flights.clear();
				Pt.Now += 3.0;
				return Mid.Vertices.size() > Plain.Vertices.size() ? Mid.Vertices.size() - Plain.Vertices.size() : 0;
			};
			if (Seats.size() == 3)
			{
				const size_t One = Fly(1);
				const size_t Three = Fly(3);
				std::printf("  ignite fix: the fountain mid-flight: one pot %zu vertices, a 3-way split %zu (was 3 x one)\n", One, Three);
				IgExpect(One > 300 && Three < 3 * One && Three <= 12 * 79 + 6 * 17 + 60, "ignite fix: a 3-way split's fountains cost 12 chips and one trail, not 18 and three");
				// Side pots 0.3 s apart (a fast pace: the hero out of the hand, a watched plan): the side pot launching under a
				// fountain already in the air flies lighter and without a trail; the one in the air keeps all six chips and its
				// trail, so nothing blinks out mid-flight (a fountain's weight is fixed at its launch; review-g2f-ignite).
				ig::IgniteOf(&Pt.RL).Pots.clear();
				St.Flights.clear();
				const double T0 = Pt.Now;
				const auto Pot = [&](int Seat, double At) {
					ss::Flight Fl;
					Fl.IsChips = true;
					Fl.From = ss::FlightEnd::AtPot();
					Fl.To = ss::FlightEnd::AtSeat(Seat);
					Fl.Amount = 1400;
					Fl.Start = At;
					Fl.Dur = 0.6;
					St.Flights.push_back(Fl);
				};
				Pot(Seats[0], T0);
				Pt.Now = T0 + 0.3;
				const size_t Alone = Pt.Frame().Vertices.size();
				Pot(Seats[1], T0 + 0.3);
				const size_t Both = Pt.Frame().Vertices.size();
				std::printf("  ignite fix: a side pot launching under a flying fountain adds %zu vertices (its first chip; one chip is %zu)\n", Both > Alone ? Both - Alone : 0,
					(One - 6 * 17) / 6);
				IgExpect(Both > Alone && Both - Alone <= One / 6, "ignite fix: a fountain in the air keeps its chips and trail when a side pot launches under it");
				ig::IgniteOf(&Pt.RL).Pots.clear();
				St.Flights.clear();
				Pt.Now += 3.0;
			}
		}
	}
	// L9: the Flame's week before the book. A career whose first paid entry is this Wednesday: Monday and Tuesday were no
	// flame days to miss (core's view marks them missed; the pages read them as plain days).
	{
		IgLaptop Pw("ignite-fix-week");
		ss::Session& Sw = Pw.S;
		Pw.At(IgPrime - 60.0);
		Sw.BankrollCents = 200000;
		Pw.Join(50000);
		// M2, live (on the same career, before its first entry): the adapter carries core's Crackers' odds whole.
		{
			const ig::IgniteLoops Ll = ig::IgniteLoopsFrom(Sw, Sw.WorldMinutes());
			bool Same = Ll.Live;
			for (int T = 0; T < 3 && Same; ++T)
			{
				const ss::site::play::CrackerOddsView V = ss::site::play::CrackerOdds(T);
				const ig::IgniteCrackerOdds& O = Ll.CrackerOdds[static_cast<size_t>(T)];
				Same = O.Rows.size() == V.Rows.size() && O.Name == V.Name && O.LookP == V.LookP && O.TicketP == V.TicketP;
				for (size_t K = 0; K < V.Rows.size() && Same; ++K)
				{
					Same = O.Rows[K].P == V.Rows[K].P && O.Rows[K].Label == V.Rows[K].Label && O.Rows[K].Part == V.Rows[K].Part;
				}
			}
			IgExpect(Same, "ignite fix live: the pages' Crackers' odds are core's, row for row");
		}
		if (ig::IgniteLoopsFrom(Sw, Sw.WorldMinutes()).Live && PayOneTen(Sw))
		{
			const ig::IgniteLoops Lw = ig::IgniteLoopsFrom(Sw, Sw.WorldMinutes());
			const ss::site::play::View V = Sw.SitePlayView(SiteId::Ignite);
			const int Wd = ss::net::Weekday(ss::site::play::PlayDay(Sw.WorldMinutes()));
			bool Plain = Wd >= 2;
			for (int D = 0; D < Wd && D < 7; ++D)
			{
				Plain = Plain && Lw.Week[static_cast<size_t>(D)] == 0;
			}
			std::printf("  ignite fix: the week (weekday %d): core %d%d%d%d%d%d%d, the pages %d%d%d%d%d%d%d\n", Wd, V.Ig.Week[0], V.Ig.Week[1], V.Ig.Week[2], V.Ig.Week[3], V.Ig.Week[4],
				V.Ig.Week[5], V.Ig.Week[6], Lw.Week[0], Lw.Week[1], Lw.Week[2], Lw.Week[3], Lw.Week[4], Lw.Week[5], Lw.Week[6]);
			IgExpect(Plain && Lw.Week[static_cast<size_t>(Wd)] == 1, "ignite fix: the Flame's week marks no day before the first paid entry as missed (today lit)");
			ig::IgniteOf(&Pw.RL).Rewards = 0;
			Pw.Open(Page::Promotions);
			WriteList("ignite_fix_week", Pw.Settled());
			// Only the days before the first paid entry read plain: four days on (Sunday), a day core marks missed after it stays
			// missed, every other day as core has it (review-g2f-ignite).
			Pw.At(Sw.WorldMinutes() + 4.0 * 1440.0);
			const bool Paid = PayOneTen(Sw);
			const ig::IgniteLoops Ls = ig::IgniteLoopsFrom(Sw, Sw.WorldMinutes());
			const ss::site::play::View Vs = Sw.SitePlayView(SiteId::Ignite);
			const int Ws = ss::net::Weekday(ss::site::play::PlayDay(Sw.WorldMinutes()));
			bool Kept = Paid && Ws == Wd + 4;
			int Missed = 0;
			for (int D = 0; D < Ws && D < 7; ++D)
			{
				Kept = Kept && Ls.Week[static_cast<size_t>(D)] == (D < Wd ? 0 : Vs.Ig.Week[static_cast<size_t>(D)]);
				Missed += D >= Wd && Vs.Ig.Week[static_cast<size_t>(D)] == 3 ? 1 : 0;
			}
			std::printf("  ignite fix: the week four days on (weekday %d, paid %d): core %d%d%d%d%d%d%d, the pages %d%d%d%d%d%d%d\n", Ws, Paid ? 1 : 0, Vs.Ig.Week[0], Vs.Ig.Week[1],
				Vs.Ig.Week[2], Vs.Ig.Week[3], Vs.Ig.Week[4], Vs.Ig.Week[5], Vs.Ig.Week[6], Ls.Week[0], Ls.Week[1], Ls.Week[2], Ls.Week[3], Ls.Week[4], Ls.Week[5], Ls.Week[6]);
			IgExpect(Kept && Missed >= 1, "ignite fix: a day missed after the first paid entry stays missed; only the days before it read plain");
		}
		else
		{
			std::printf("  ignite fix: no View or no $1.10 entry open; the week's check is skipped\n");
		}
	}
	// L9: the reveal's odds line and ODDS pill (the mockup's): a x25 at $1.10 says "1 in 250 at $1.10"; ODDS opens FUSE's odds
	// over the reveal, which waits while it's up; a click outside closes it (swallowed) and the table opens itself after.
	{
		IgLaptop Pr("ignite-fix-reveal");
		ss::Session& Sr = Pr.S;
		Pr.At(IgPrime - 60.0);
		Sr.BankrollCents = 200000;
		Pr.Join(50000);
		if (ig::IgniteLoopsFrom(Sr, Sr.WorldMinutes()).FuseOn)
		{
			ss::fuse::DebugForce(Sr, 0, 25);
			Pr.Open(Page::Fuse);
			L = Pr.Settled();
			ig::IgniteUi& Ur = ig::IgniteOf(&Pr.RL);
			const bool Lit = ClickRun(Pr, L, "Light it \xC2\xB7 $1.10");
			const ss::fuse::Reveal Rv = Ur.RevealTable >= 0 ? Sr.FuseReveal(Ur.RevealTable) : ss::fuse::Reveal();
			Pr.Now = Ur.RevealAt + ig::IgniteRevealTimesFor(25).BoomEnd;
			L = Pr.Frame();
			WriteList("ignite_fix_reveal25", L);
			const IgBox Pill = IgRun(L, "ODDS", Pr.M, 520.0f, 545.0f);
			IgExpect(Lit && Rv.M == 25 && DrewRun(L, "BIG FUSE") && DrewRun(L, "1 in 250 at $1.10") && DrewRun(L, "pays 80 / 12 / 8") && DrewRun(L, "$22.00 \xC2\xB7 $3.30 \xC2\xB7 $2.20") &&
					Pill.Found && std::fabs(Pill.X + Pill.W / 2.0f - 1106.0f) < 1.0f && Lettered(L) == 0,
				"ignite fix: the x25 reveal reads as the mockup (BIG FUSE, 1 in 250 at $1.10, 80 / 12 / 8, the places, ODDS)");
			if (Pill.Found)
			{
				Pr.Click(Pill.X + Pill.W / 2.0f, Pill.Base - 6.0f);
			}
			L = Pr.Frame();
			IgExpect(Ur.Drawer == 2 && !Ur.RevealEvent.empty() && DrewRun(L, "FUSE ODDS") && DrewRun(L, "1 in 250 at $1.10"), "ignite fix: the reveal's ODDS opens FUSE's odds over it");
			// The table sits under the reveal from LIGHT IT on (the session's screen is already its); the reveal is what waits.
			const int Tb = Ur.RevealTable;
			Pr.Now += 4.0;
			L = Pr.Frame();
			WriteList("ignite_fix_reveal_odds", L);
			IgExpect(!Ur.RevealEvent.empty() && Ur.Drawer == 2 && DrewRun(L, "FUSE ODDS"), "ignite fix: the reveal waits while its odds are up (it doesn't seat you by itself)");
			// M3's laptop half (g2f-fuse's patch): the reveal drawn under the odds isn't seen, so core holds the first deal. The
			// session's own clock is run past the reveal's run and its second (core's clock alone would call it seen there); the
			// laptop drawing it every frame (RevealShown) is what keeps it unseen (review-g2f-ignite).
			Pr.Now = Step(Sr, Pr.Now, 0.2);
			Pr.Frame();
			const ss::fuse::Reveal Held = Tb >= 0 ? Sr.FuseReveal(Tb) : ss::fuse::Reveal();
			IgExpect(Tb >= 0 && Sr.Now >= Held.PaidAt + ss::fuse::RevealSeconds(25) + 1.0 && !Held.Seen && Ur.Drawer == 2,
				"ignite fix: core counts the reveal unseen while the laptop draws it under its odds, past its run and a second");
			Pr.OneFrame(800.0f, 836.0f); // TAKE YOUR SEAT, under the drawer's scrim
			Pr.Frame();
			IgExpect(Ur.Drawer == 0 && !Ur.RevealEvent.empty(), "ignite fix: a click outside the odds closes them, swallowed (TAKE YOUR SEAT doesn't fire)");
			Pr.Now += 1.2;
			Pr.Frame();
			Pr.Frame();
			IgExpect(Ur.RevealEvent.empty() && Sr.CurrentScreen == ss::Screen::Table, "ignite fix: a second after the odds close the reveal seats you");
			IgExpect(Tb >= 0 && Sr.FuseReveal(Tb).Seen, "ignite fix: the table opening itself takes the seat (core's TakeSeat: the first hand deals on the beat)");
		}
		else
		{
			std::printf("  ignite fix: FUSE isn't lit in this build; the reveal's checks are skipped\n");
		}
	}
}

// ------------------------------------------------------------------ the PIE G2 fixes (g2p-sheet: P1 the sheet's close, P4 the clause's fit)

/**
 * Whether every text run centred inside R (its baseline within R's height) lies inside R; Found counts them. A line wider than
 * its button (PIE G2 P4's "Strike under your Team RiverLine deal" on a FUSE pill) is centred on it, so it's caught.
 */
bool RunsInside(const DrawList& L, const TableMeasurer& M, const ss::ui::Rect& R, int& Found)
{
	bool Inside = true;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || Cmd.Text.BaselineY < R.Y || Cmd.Text.BaselineY > R.Y + R.H)
		{
			continue;
		}
		const float W = M.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx);
		const float Mid = Cmd.Text.X + W / 2.0f;
		if (Mid < R.X || Mid > R.X + R.W)
		{
			continue;
		}
		++Found;
		Inside = Inside && Cmd.Text.X >= R.X - 0.01f && Cmd.Text.X + W <= R.X + R.W + 0.01f;
	}
	return Inside;
}

/** The size a run that is exactly Text was drawn at (0 if it wasn't). */
float RunSize(const DrawList& L, const std::string& Text)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text)
		{
			return Cmd.Text.SizePx;
		}
	}
	return 0.0f;
}

/** Whether some run ends in an ellipsis. */
bool Ellipsized(const DrawList& L)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		const std::string& T = Cmd.Text.Text;
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && T.size() >= 3 && T.compare(T.size() - 3, 3, "\xE2\x80\xA6") == 0)
		{
			return true;
		}
	}
	return false;
}

/**
 * PIE G2 P4: SitesTwoPress under the clause keeps every line inside its button, in each look: what fits draws exactly as
 * before (the long warning at its size), a deal's name too long for the button gives way to the short warning, then both
 * lines shrink to a floor, then end in an ellipsis; the confirm step's "Take the strike" and "Cancel" fit their halves and
 * its line stays over the button's width.
 */
void TwoPressFit()
{
	namespace sd = ss::ui::rlsites_detail;
	TableMeasurer M;
	const std::string Long = "Strike under your Team RiverLine deal";
	const auto Draw = [&M, &Long](SiteId Site, const ss::ui::Rect& R, float Size, bool Armed) {
		DrawList L;
		ss::ui::Ui U;
		ss::ui::Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
		U.Begin(C, 1.0);
		bool Flag = Armed;
		sd::TwoPressOpts O;
		O.First.Kind = ss::ui::ButtonKind::Primary;
		O.First.Size = Size;
		O.Strike = Long;
		O.Armed = &Flag;
		sd::SitesTwoPress(C, U, sd::SitesLook(Site), "fitcheck", R, "Light it \xC2\xB7 $1.10", true, O);
		U.End();
		return L;
	};
	int N = 0;
	// RiverLine's 400-wide button (ui_seg_seams' rig): it fits, so it's exactly as it was.
	const ss::ui::Rect Wide{400.0f, 500.0f, 400.0f, 70.0f};
	const DrawList A = Draw(SiteId::RiverLine, Wide, 22.0f, false);
	IgExpect(RunSize(A, Long) == 22.0f && RunSize(A, "Light it \xC2\xB7 $1.10 anyway") > 0.0f && RunsInside(A, M, Wide, N), "g2p: a strike line that fits keeps the deal's name at its size");
	// A FUSE pill (Ignite's look, 308 wide at 22): the short warning, at its size, inside the pill.
	const ss::ui::Rect Pill{48.0f, 500.0f, 308.0f, 72.0f};
	const DrawList B = Draw(SiteId::Ignite, Pill, 22.0f, false);
	N = 0;
	IgExpect(RunSize(B, ss::ui::rlsites_detail::SitesStrikeShort) == 22.0f && RunSize(B, Long) == 0.0f && RunsInside(B, M, Pill, N) && N == 2,
		"g2p: on a FUSE pill the strike line gives way to \"Strike under your deal\", inside the pill");
	// RunItTwice's slab in caps, 240 wide.
	const ss::ui::Rect Slab{100.0f, 300.0f, 240.0f, 56.0f};
	const DrawList Cs = Draw(SiteId::RunItTwice, Slab, 20.0f, false);
	N = 0;
	IgExpect(RunSize(Cs, ss::ui::rlnet_detail::NetUpper(ss::ui::rlsites_detail::SitesStrikeShort)) == 20.0f && RunsInside(Cs, M, Slab, N) && N == 2,
		"g2p: RunItTwice's narrow slab: the short warning in caps, inside");
	// Narrower: the short warning shrinks (to no less than 12), then both lines end in an ellipsis; nothing leaves.
	const ss::ui::Rect Small{100.0f, 300.0f, 150.0f, 44.0f};
	const DrawList D = Draw(SiteId::Ignite, Small, 15.0f, false);
	N = 0;
	const float Ds = RunSize(D, ss::ui::rlsites_detail::SitesStrikeShort);
	IgExpect(Ds >= 12.0f && Ds < 15.0f && RunsInside(D, M, Small, N) && N == 2 && !Ellipsized(D), "g2p: a small pill shrinks the short warning to fit (no ellipsis needed)");
	const ss::ui::Rect Tiny{100.0f, 300.0f, 100.0f, 44.0f};
	const DrawList E = Draw(SiteId::Ignite, Tiny, 15.0f, false);
	N = 0;
	IgExpect(Ellipsized(E) && RunsInside(E, M, Tiny, N) && N == 2, "g2p: past the floor both lines end in an ellipsis, inside");
	// The confirm step on a FUSE pill: "Take the strike" fits its half (smaller), "Cancel" its own; the line over the pill.
	const DrawList F = Draw(SiteId::Ignite, Pill, 22.0f, true);
	const float Hw = (Pill.W - 12.0f) / 2.0f;
	int Nl = 0;
	int Nr = 0;
	const IgBox Note = IgRun(F, ss::ui::rlsites_detail::SitesStrikeConfirm, M);
	const float Take = RunSize(F, "Take the strike");
	IgExpect(Take >= 16.5f && Take < 22.0f && RunSize(F, "Cancel") == 22.0f && RunsInside(F, M, {Pill.X, Pill.Y, Hw, Pill.H}, Nl) && Nl == 1 &&
			RunsInside(F, M, {Pill.X + Hw + 12.0f, Pill.Y, Hw, Pill.H}, Nr) && Nr == 1 && Note.Found && Note.X >= Pill.X && Note.X + Note.W <= Pill.X + Pill.W + 0.01f,
		"g2p: the confirm step fits: \"Take the strike\" smaller in its half, \"Cancel\" as it was, the line no wider than the pill");
	std::printf("  g2p two-press fit: FUSE pill %.1f (short), small pill %.1f, Take the strike %.1f\n", RunSize(B, ss::ui::rlsites_detail::SitesStrikeShort), Ds, Take);
}

/**
 * PIE G2 P4 on the page: FUSE under Team RiverLine with a ticket held (the PIE shot g2_13 had none): the three LIGHT IT pills
 * and the ticket pill under the first keep every line inside; the first pill armed, its halves too; Cancel disarms it.
 */
void ClauseFuse()
{
	IgLaptop P("ignite-clause-fuse");
	ss::Session& S = P.S;
	P.At(IgPrime - 60.0);
	S.BankrollCents = 200000;
	P.Join(50000);
	if (!IgSignClause(S))
	{
		std::printf("  g2p: no clause strike to warn of here; the FUSE fit checks are skipped\n");
		return;
	}
	const ig::IgniteLoops F = Fake(true);
	ig::IgniteFakeLoops(&P.RL, &F);
	S.Life.Tickets[F.FuseTicketIds[0]] = 2;
	P.Open(Page::Fuse);
	P.Settled();
	P.Now += 7.0; // the deal's badge toast (RiverLine's, over every app) has gone
	DrawList L = P.Settled();
	IgPage(L, "ignite_g2p_fuse_clause");
	bool Inside = true;
	int Lines = 0;
	for (int I = 0; I < 3; ++I)
	{
		Inside = RunsInside(L, P.M, {48.0f + static_cast<float>(I) * 372.0f, 500.0f, 308.0f, 72.0f}, Lines) && Inside;
	}
	Inside = RunsInside(L, P.M, {48.0f, 582.0f, 308.0f, 44.0f}, Lines) && Inside;
	IgExpect(Inside && Lines == 8 && CountRuns(L, ss::ui::rlsites_detail::SitesStrikeShort) == 3 && DrewRun(L, "Light it \xC2\xB7 $1.10 anyway") &&
			DrewRun(L, "Use a ticket \xC2\xB7 2 held anyway"),
		"g2p: under the clause FUSE's pills read the short warning and every line stays inside its pill (P4)");
	P.Click(202.0f, 530.0f); // the $1.10 pill: the first press arms
	L = P.Settled();
	int Nl = 0;
	int Nr = 0;
	const bool Halves = RunsInside(L, P.M, {48.0f, 500.0f, 148.0f, 72.0f}, Nl) && RunsInside(L, P.M, {208.0f, 500.0f, 148.0f, 72.0f}, Nr);
	IgExpect(Halves && Nl == 1 && Nr == 1 && DrewRun(L, "Take the strike") && S.TableCount() == 0, "g2p: armed, \"Take the strike\" and \"Cancel\" fit their halves (no table yet)");
	P.Click(282.0f, 536.0f); // Cancel
	L = P.Settled();
	IgExpect(CountRuns(L, ss::ui::rlsites_detail::SitesStrikeShort) == 3 && S.TableCount() == 0, "g2p: Cancel disarms the pill");
	ig::IgniteFakeLoops(&P.RL, nullptr);
}

/**
 * The fit's other halves (review-g2p-sheet: three mutants of SitesFitButton no check caught): off the clause, a label that
 * fits over a line that doesn't shrinks together with it before anything is cut; the confirm step's line over a narrower
 * button shrinks first (to 11), then ends in an ellipsis, never wider than the button; on a button too narrow for "Cancel",
 * both halves keep their words inside.
 */
void TwoPressFitEdges()
{
	namespace sd = ss::ui::rlsites_detail;
	TableMeasurer M;
	DrawList Scratch;
	const ss::ui::Canvas Cm(Scratch, M, 1600.0f, 1000.0f, 1.0f);
	const std::string Reg = "Register \xC2\xB7 $5.50";
	const auto Draw = [&M, &Reg](SiteId Site, const ss::ui::Rect& R, float Size, bool Clause, bool Armed, const std::string& Sub) {
		DrawList L;
		ss::ui::Ui U;
		ss::ui::Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
		U.Begin(C, 1.0);
		bool Flag = Armed;
		sd::TwoPressOpts O;
		O.First.Kind = ss::ui::ButtonKind::Primary;
		O.First.Size = Size;
		O.First.Sub = Sub;
		O.Strike = "Strike under your Team RiverLine deal";
		O.Always = true;
		O.Armed = &Flag;
		sd::SitesTwoPress(C, U, sd::SitesLook(Site), "fitedge", R, Reg, Clause, O);
		U.End();
		return L;
	};
	// The confirm step's line (its baseline 10 over the button): inside the button's width, its size, whether it was cut.
	const auto Over = [&M](const DrawList& L, const ss::ui::Rect& R, float& Size, bool& Cuts) {
		for (const ss::ui::DrawCmd& Cmd : L.Cmds)
		{
			if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && std::fabs(Cmd.Text.BaselineY - (R.Y - 10.0f)) < 0.01f)
			{
				const std::string& T = Cmd.Text.Text;
				Size = Cmd.Text.SizePx;
				Cuts = T.size() >= 3 && T.compare(T.size() - 3, 3, "\xE2\x80\xA6") == 0;
				return Cmd.Text.X >= R.X - 0.01f && Cmd.Text.X + M.Width(T, Cmd.Text.Face, Cmd.Text.SizePx) <= R.X + R.W + 0.01f;
			}
		}
		Size = 0.0f;
		Cuts = false;
		return false;
	};
	// Off the clause on RunItTwice's slab (caps): "REGISTER · $5.50" fits; RegisterBlock's "Late registration · 12:34 left" doesn't.
	const sd::SiteLook& RitLook = sd::SitesLook(SiteId::RunItTwice);
	const std::string Late = "Late registration \xC2\xB7 12:34 left";
	const std::string RegCaps = ss::ui::rlnet_detail::NetUpper(Reg);
	const float RegAt20 =
		RitLook.TitleFace >= 0 ? Cm.Measure(RegCaps, 20.0f, static_cast<ss::ui::Font>(RitLook.TitleFace)) : Cm.Measure(RegCaps, 20.0f, 700, false);
	const float LateAt20 = Cm.Measure(Late, 20.0f * 0.72f, 600, true);
	const float Inner = std::max(RegAt20 + 1.0f, LateAt20 * 0.85f);
	const float Ends = RitLook.Button == sd::ButtonShape::Pill ? 56.0f * 0.2f : std::max(5.0f, RitLook.Radius * 0.5f);
	const ss::ui::Rect Slab{100.0f, 300.0f, Inner + 2.0f * Ends, 56.0f};
	const DrawList A = Draw(SiteId::RunItTwice, Slab, 20.0f, false, false, Late);
	int N = 0;
	const float LateSize = RunSize(A, Late);
	IgExpect(Inner < LateAt20 && LateSize > 0.0f && LateSize < 20.0f * 0.72f - 0.01f && std::fabs(RunSize(A, RegCaps) * 0.72f - LateSize) < 0.01f && RunsInside(A, M, Slab, N) && N == 2 &&
			!Ellipsized(A),
		"review: a label that fits over a line that doesn't: the two shrink together, nothing cut, both inside");
	// Under the clause, armed, over a button narrower than the line at 14: smaller first (to 12.5 here), whole, no wider.
	const std::string Confirm = sd::SitesStrikeConfirm;
	const ss::ui::Rect Mid{100.0f, 300.0f, Cm.Measure(Confirm, 12.5f, 600, false) + 1.0f, 72.0f};
	float MidSize = 0.0f;
	bool MidCut = true;
	const bool MidIn = Over(Draw(SiteId::Ignite, Mid, 22.0f, true, true, std::string()), Mid, MidSize, MidCut);
	IgExpect(Cm.Measure(Confirm, 13.0f, 600, false) > Mid.W && MidIn && MidSize == 12.5f && !MidCut, "review: the confirm step's line shrinks to sit over a narrower button (12.5 here), whole");
	// Narrower than the line at its floor and than "Cancel" at 15: the line at 11 ends in an ellipsis over the button, and
	// "Take the strike" and "Cancel" stay inside their halves.
	const ss::ui::Rect Thin{100.0f, 300.0f, 100.0f, 44.0f};
	const float Hw = (Thin.W - 12.0f) / 2.0f;
	const sd::SiteLook& IgLook = sd::SitesLook(SiteId::Ignite);
	const float CancelAt15 =
		IgLook.TitleFace >= 0 ? Cm.Measure("Cancel", 15.0f, static_cast<ss::ui::Font>(IgLook.TitleFace)) : Cm.Measure("Cancel", 15.0f, 700, false);
	const DrawList Dt = Draw(SiteId::Ignite, Thin, 15.0f, true, true, std::string());
	float ThinSize = 0.0f;
	bool ThinCut = false;
	const bool ThinIn = Over(Dt, Thin, ThinSize, ThinCut);
	int Nl = 0;
	int Nr = 0;
	IgExpect(CancelAt15 > Hw && ThinIn && ThinSize == 11.0f && ThinCut && RunsInside(Dt, M, {Thin.X, Thin.Y, Hw, Thin.H}, Nl) && Nl == 1 &&
			RunsInside(Dt, M, {Thin.X + Hw + 12.0f, Thin.Y, Hw, Thin.H}, Nr) && Nr == 1,
		"review: on a thin button the confirm line ends in an ellipsis at 11 over it, and both halves keep their words inside");
	std::printf("  review two-press edges: the line under at %.2f with its label, the confirm line %.1f then %.1f (cut), \"Cancel\" at 15 is %.1f in a %.1f half\n", LateSize, MidSize,
		ThinSize, CancelAt15, Hw);
}

/**
 * PIE G2 P1: the sheet opened as PIE opened it (a cabinet clicked through the pointer, which stays on the screen), then its
 * close button clicked as a mouse does, after hovering it: a click inside one frame, a press and a release a frame apart, and
 * a press held for ten frames. Each closes the sheet, and nothing opens it again (no confirm step is left armed).
 */
void SheetClose()
{
	IgLaptop P("ignite-sheet-close");
	P.At(IgPrime);
	P.S.BankrollCents = 150000;
	P.Join(21240);
	P.Open(Page::Lobby);
	P.Settled(800.0f, 600.0f);
	const double Tick = 1.0 / 30.0;
	const auto At = [&P, Tick](float X, float Y, bool Down, bool Pressed, bool Released) {
		P.Frame(X, Y, Down, Pressed, Released);
		P.Now += Tick;
	};
	int Opened = 0;
	int Closed = 0;
	for (int Kind = 0; Kind < 3; ++Kind)
	{
		At(110.0f, 590.0f, true, true, false); // the first shelf's first cabinet {24, 508, 180, 120}
		At(110.0f, 590.0f, false, false, true);
		for (int I = 0; I < 3; ++I)
		{
			At(110.0f, 590.0f, false, false, false);
		}
		Opened += ig::IgniteSheetOpen(&P.RL) ? 1 : 0;
		for (int I = 0; I < 3; ++I)
		{
			At(1540.0f, 332.0f, false, false, false); // hovering the close button
		}
		if (Kind == 0)
		{
			At(1540.0f, 332.0f, false, true, true);
		}
		else
		{
			At(1540.0f, 332.0f, true, true, false);
			for (int I = 0; I < (Kind == 2 ? 10 : 0); ++I) // held a third of a second (PIE's 0.35 s press)
			{
				At(1540.0f, 332.0f, true, false, false);
			}
			At(1540.0f, 332.0f, false, false, true);
		}
		for (int I = 0; I < 3; ++I)
		{
			At(1540.0f, 332.0f, false, false, false);
		}
		Closed += !ig::IgniteSheetOpen(&P.RL) && !P.S.ConfirmRegister ? 1 : 0;
		P.Settled(800.0f, 200.0f);
	}
	std::printf("  g2p sheet close: opened by a cabinet %d of 3, closed by the x %d of 3\n", Opened, Closed);
	IgExpect(Opened == 3 && Closed == 3, "g2p: a sheet opened from a cabinet closes on its x however the click comes (one frame, two, held) and stays closed (P1)");
}
} // namespace ig2

void SegIgnite()
{
	using namespace ignite_seg;
	const auto Seg0 = std::chrono::steady_clock::now();
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
	// G2: the loops' pages and overlays (on a stand-in View until core-siteplay's lands).
	ig2::Pages();
	ig2::Overlays();
	ig2::Preview();
	ig2::Reel();
	ig2::TablePeople();
	ig2::Live();
	ig2::Review();
	ig2::FuseAgain();
	ig2::Bench();
	// The G2 fix round (G2_AUDIT M2, M5, L9), timed apart (rivals_ui's 60 s budget).
	const auto Fix0 = std::chrono::steady_clock::now();
	ig2::FixRound();
	const auto Fix1 = std::chrono::steady_clock::now();
	// The PIE G2 fixes (g2p-sheet): P1 the sheet's close button as PIE clicked it, P4 the clause's lines inside their buttons.
	ig2::SheetClose();
	ig2::TwoPressFit();
	ig2::ClauseFuse();
	ig2::TwoPressFitEdges(); // review-g2p-sheet: the fit's mutants no check caught
	const auto Fix2 = std::chrono::steady_clock::now();
	std::printf("  ui-ignite: %d checks in %.1f s (the G2 fix round's %.1f s, the PIE fixes' %.1f s)\n", IgChecks, std::chrono::duration<double>(Fix2 - Seg0).count(),
		std::chrono::duration<double>(Fix1 - Fix0).count(), std::chrono::duration<double>(Fix2 - Fix1).count());
}
} // namespace ui_test
