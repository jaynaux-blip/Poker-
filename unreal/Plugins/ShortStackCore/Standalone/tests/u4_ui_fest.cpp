// u4_ui: the festival half's hub and moments (update 4 SPEC v1.7 §2.15.2; F-hubs' file): the hub in three frames, Main Event Day,
// the moments, the strip and popover, the pin wall. Called from u4_ui_main.cpp (the kit).
// Phase A (update 4 SPEC v1.7 §6 PA-48): registered, empty.
// G1 FH-1a (F-hubs' stream A; PLAN_G1 §2.1, §3.2; PLAN_FHUBS §4 and its Corrections): RiverLine's hub on RCOP 2026 (F-net's FN-1 row,
// routed by stream B's FH-1c), its first two pages and FestHonestLines' A half. Every page is emitted at scale 1.0 (the kit's Emit,
// under SS_U4_UI_OUT) and measured at 1.0 and 1.5 (vertices, text runs, NetSpaced lines, commands; Q18); SS_UI_BENCH=1 adds the warm
// draw (100 draws after one cold). §2.8.10's hub row is printed only here: the art is F-look's stubs until FH-7 asserts it. Helpers
// sit in an anonymous namespace and carry "Hubs" (the hosts' file, u4_ui_fest_hosts.cpp, and F-look's share the binary).
// G1 FH-1b (PLAN_G1 §2.1, §3.2): the strip, the day's rows and the popover: the page fest_popover (RCOP's cell on Night One), and
// FestStrikeGate, FestOneFrameClicks' popover half and FestCopyUi's A half (with FestBlurbsFilled' U2 half, PLAN_G1 §1 item 9). The
// popover's budget row (<= 3k, <= 20 runs) is printed, asserted at FH-7 with the hub's.
// G1 FH-2 (PLAN_G1 §2.1, §3.2; PLAN_FHUBS §4 FH-2 and its Corrections): the real calendar in all three frames, after F-net's FN-3 and
// FN-2b. Two careers walked forward in day order (the plan's A and B): A opens Ignite and draws FEIGN 2026's hub (fest_hub_ig_live);
// B never opens Ignite and draws Night One's strip (fest_strip_night_one), RCOP 2026's first day (fest_hub_rl_live) and its ladder
// (fest_hub_rl_locked), GRIT 2027's second night (fest_hub_rit_twice) and FEIGN II's popover on day 395 (fest_popover_pass, its BY
// INVITATION form: every site's Pass is -1 until F-world's FW-7; FH-7 emits the counting form). Checks: FestStripTen, FestLockedMain,
// FestCopyUi extended (U30, U33, U2's NIGHT form, U15's GRIT line and its drawn figures).
// G1 FH-4A (PLAN_G1 §2.1, §3.2; PLAN_FHUBS §4 FH-4 and its Corrections): THE RACE, CHAMPIONS and YOUR <FESTIVAL>, inside career B
// (no new career: the pages' time budget). On RCOP 2026's first day the YOUR panel invites before the hero plays, then the hero's first
// RCOP event (a cash told through World::HeroEntered and HeroFinished, the public path, item 15) fills it: fest_hub_your_ladder; a
// plain series result adds nothing (FestYourStrip, the hub's half). Days 14 and 15 walk THE RACE's movement (none before the hub
// has seen a day turn); day 19 keeps THE RACE first with the pin (FestLockedMain); day 35 shows RCOP 2026's four title lines and its
// Mini and Micro winners, and day 400 RCOP 2026 and 2027 on one roll: fest_hub_champ_roll (FestRollTitles). FestCopyUi extended (U5,
// U7, U8, U10, U12, U27, U28's seat line, U34, U37 and the roll's figures: listed, no digit typed, U37's and U10's forms).
// G1 FH-5b (PLAN_G1 §2.1, §3.2; PLAN_FHUBS §4 FH-5 and its Corrections; after F-world's FW-4): Main Event Day, the reel, the champion
// card, WHO'S IN and the Afterglow. Career B draws RCOP 2026's Main day (fest_hub_rl_mainday, day 34 at 15:30), the reel between the
// Main's finish and the crown (fest_hub_rl_reel), the Afterglow (fest_hub_rl_afterglow, day 35), WRIT 2027's opening bell
// (fest_hub_rit_bell, day 118 at 18:00) and REIGN 2027's Main day (fest_hub_rit_champ_mainday, day 306), and checks U4 on RCOP 2027's
// Main day (399); career A goes on to FEIGN 2026's Main day (fest_hub_ig_mainday, day 65) and PENNANT 2027 (fest_hub_ig_champ_live, day
// 185). Checks: FestPreviewStill's flap, bell and reel half (the laptop drawn as the stream preview through rlfest_detail::HubPreviewing;
// cues counted by HubsHooks), FestReignCopy's REIGN hub half, the broadcast against the network and World::FieldStoriesOf on each host,
// the Mini and Micro rows pinned on PENNANT's and REIGN's days, FestCopyUi's FH-5b listing. U33's finished form (FH-2) is read in WRIT's
// Afterglow: The Summons ends past midnight on the Main's day, where the broadcast holds the hub.
// G1 FH-6a (PLAN_G1 §2.1, §3.2, item 16; PLAN_FHUBS §4 FH-6 and its Corrections; after F-world's FW-5 and FW-6): the moments, the pin
// wall and the stings. Career A plays FEIGN 2026's dev moments on its Main day (fest_moment_main, fest_moment_event, fest_moment_belt;
// ss.Fest.Moment through Session::DebugFest, R-7) and PENNANT 2027's title (fest_moment_title); career B plays REIGN 2027's title (no
// sponsor mark) and RCOP 2027's Main (U36's "2nd edition"), checks the pin wall before and after the hero's first festival event and the
// hub's sting once a world day; a third career plants one world\tfest\tmark line a grade (§2.13's format, the belt's too) and plays
// them through the keyed path across a reload (fest_moment_mark, FINALIST). Every moment frame is drawn as RiverLine::Draw draws it
// with the lead's FestMomentHold line (patch fhubs_to_lead_4): HubsHeld calls the hold first. Checks: FestMarkMoment, FestTitleMoment,
// FestPinsReveal, FestOneFrameClicks' moments half and FestPreviewStill's moments, sting-once and registration-sting half.
// G1 FH-7 (PLAN_G1 §2.1, §3.2, items 11, 16, 33, 34; PLAN_FHUBS §4 FH-7 and its Corrections; after every stream-B package, F-look's
// FL-2/3/4/7, F-world's FW-7 and the lead's H2): FestHubBudgets. Every page this file emits and every hub state it draws without a page
// is measured at both scales: a hub (each host's Buildup, Live, MainDay, Closing and Afterglow: the states no page shows are drawn on the
// careers' way, Ignite's in career A, RunItTwice's Buildup, Closing and Afterglow in career B) within <= 35k vertices, 600 runs and 4
// NetSpaced lines; under SS_PERF=1 (u4_ui_perf, RUN_SERIAL; §8.1's timing rule) its warm draw, the median of five means of 20 draws,
// within 1.5 ms (2.0 on a broadcast frame: the flap row's waves and the reel), a moment's within 2.0 and the mark card's within 1.5; and
// Q15's rows: the popover <= 3k vertices and 20 runs over the page, the lobby with festival crests (RCOP 2026 Live and its Afterglow)
// inside Night One's measured lobby + 2k (29.9k + 2k at 1.0), Ignite's floor chip (chip 10's face; the shelf's two runs) <= 1k, the
// Career page with the hero's festival pieces inside its cost before them + 4k (the moments, the mark card and the pin wall are
// asserted where they play). Plant: the strip's crests drawn at full detail. Pages: fest_career_pins (career B's Career page at the end),
// fest_card_defending (RCOP 2026's Main champion's card while RCOP 2027 is Live, day 395) and fest_popover_pass in its counting form
// (career B's two rents planted on its way to day 395: FEIGN II's Pass counts, U30). FestMarkMoment's FINALIST case on F-world's
// update4_festival.ssave (day 60, RCOP 2026's FINALIST already seen: nothing plays; the key taken out of a copy: the card plays once,
// keyed, and a reload plays nothing). The hub's Details on RunItTwice holds the Main through rlrit_detail::RitHoldSelection (stream B's
// FestYourStrip N-3 half, now through the hub's fsmain); X1 reads "Player of the Series" (the belt's honour, U20/U21) as a title, not a
// count of people.
// G1 polish (key g1p-hub, after G1's PIE, durable/update4/g1/PIE_G1_NOTES.md): FestLockFits (every lock line fits its status cell,
// measured and drawn on three hosts' pages), FestBannerStats (every edition's banner stats whole on its host), FestMainCardFits (every
// Main's guarantee whole, Details clear of it), FestWaveClear (RCOP's waves behind the header text at every phase), FestRollPillClear
// (no pill reaches CHAMPIONS' prize), the champion card's close line on the card, a finished satellite's winner line, RunItTwice's
// codes on the hub's rows (no "#"), and FestLockedMain in 2027. Every check at both UI scales; each one's plant in its comment.
#include "UiSeg.h"

#include "CopyLint.h"
#include "Trademarks.h"
#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/UI/FestivalArt.h"
#include "ShortStack/UI/RiverLine.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/WorldFestDetail.h"
#include "../../Source/ShortStackCore/Private/ShortStack/UI/RiverLineShared.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace ui
{
namespace rlfest_detail
{
// UI/RiverLineFestival.cpp (FH-2): the Festival Pass's form on a rival cell without an account, pure (FestStripTen calls it).
int HubPassForm(const net::InviteGate& Gate, bool Open, int RentsPaid, const fest::Festival* PassFest, const fest::Festival& F, int Events, int& N, int& Need);
// FH-4A: U10's line, U37's sentence, a roll edition's height, and the movement marks THE RACE drew in a laptop's last hub frame.
std::string HubYourCounts(const fest::YourLineView& V, bool Race);
std::string HubGoodOn(const std::vector<int>& Days, int Today);
float HubRollPitch(bool Championship);
int HubRaceMoves(const RiverLine& RL);
void HubPreviewing(RiverLine& RL, bool On);
// FH-4A fix round 1: U35's two pills for a step row (its rung, the rung its seat is; 0 the Main).
std::pair<std::string, std::string> HubStepPills(int K, int Next);
// FH-6a: the host's sting, the moments' pointer hold (the lead's line in RiverLine::Draw, emulated here) and the pin wall drawn alone.
SoundId HubStingOf(const festart::FestivalLook& L, net::SiteId Site);
void FestMomentHold(RiverLine& RL);
void HubPinsDraw(RiverLine& RL, Canvas& Cv, double Now);
// FH-7: the edition the hub drew in its live frame at Now ("" none) and whether that frame was the broadcast (FestHubBudgets).
std::string HubShownKey(const RiverLine& RL, double Now, bool& Broadcast);
// g1p-hub: a lock line set out for a status cell (its size; the line or lines), the banner's stat columns, the Main card's Details and
// guarantee room, and the boxes RCOP's waves passed behind in the last banner.
float HubLockLines(const Canvas& C, const std::string& Lock, float W, std::vector<std::string>& Lines);
void HubStatLayout(const Canvas& C, net::SiteId Site, const std::pair<std::string, std::string> (&Stats)[4], float (&X)[4], float (&MaxW)[4]);
void HubMainBoxes(net::SiteId Site, bool Broadcast, Rect& Details, float& GtdRoom);
std::vector<Rect> HubWaveClear(const RiverLine& RL);
} // namespace rlfest_detail
namespace festart
{
// UI/FestivalArt.cpp (g1p-hub): RCOP's wave lines for a minute's progress and a drop, broken around Clear (canvas units).
std::vector<std::vector<Vec2>> FestWaveLines(const FestivalLook& L, const Rect& R, float Day, float Drop, const std::vector<Rect>& Clear, std::vector<int>& Kinds);
} // namespace festart
namespace rlignite_detail
{
// UI/RiverLineIgnite.cpp (stream B, FH-3b): chip 10's face on Ignite's floor (FestHubBudgets' floor-chip row).
void IgniteFestChipFace(Canvas& At, const net::SeriesInfo& Sr, const Rect& R, bool On, bool Hot, uint32_t Fill, uint32_t Coral, uint32_t RimHi);
} // namespace rlignite_detail
namespace rlnet_detail
{
// UI/RiverLineNet.cpp (stream B, FH-6b): the edition a person defends and the card's honorific line (fest_card_defending).
const fest::Festival* NetFestDefends(const world::World& W, int Npc, double WorldMin);
std::string NetFestHonorific(const world::World& W, int Npc, double WorldMin);
} // namespace rlnet_detail
} // namespace ui
} // namespace ss

namespace
{
using ss::ui::DrawList;
using ss::ui::RiverLine;

/** FH-5b: a laptop's hooks with its cues counted (the opening bell's sting, FestPreviewStill). FH-6a: Keep holds the last save (the
 *  marks' reload). */
struct HubsHooks : ui_test::QuietHooks
{
	std::vector<ss::SoundId> Sounds;
	bool Keep = false;
	ss::SaveData Last;
	void Sound(ss::SoundId Id, double /*Volume*/) override { Sounds.push_back(Id); }
	void Save(const ss::SaveData& D) override
	{
		if (Keep)
		{
			Last = D;
		}
	}
};

/** A session and its laptop on RiverLine's SERIES page, the pointer away (FH-6a: From, a save to load). */
struct HubsLaptop
{
	HubsHooks H;
	ss::Session S;
	RiverLine RL;
	double Now = 1.0;
	explicit HubsLaptop(const std::string& Seed, const ss::SaveData* From = nullptr) : S(H, Seed, From), RL(S)
	{
		S.CurrentScreen = ss::Screen::Lobby;
		RL.UI.Ptr.Active = false;
		Now = ui_test::Step(S, Now, 0.2);
	}
	DrawList Frame(float Scale)
	{
		ui_test::TableMeasurer M;
		DrawList L;
		ss::ui::Canvas C(L, M, RiverLine::Width, RiverLine::Height, Scale);
		RL.Draw(C, Now);
		RL.UI.Ptr.EndFrame();
		return L;
	}
	/** The clock forward to World (the lobby clock; the world catches up), the player fed and rested. */
	void At(double World)
	{
		if (World > S.WorldMinutes())
		{
			S.LobbyMinutes += World - S.WorldMinutes();
		}
		Now = ui_test::Step(S, Now, 0.2);
		S.Life.Hunger = 20.0;
		S.Life.Thirst = 20.0;
		S.Life.Energy = 80.0;
	}
	/** RiverLine's SERIES page with no series picked: the host's current or next festival (§2.8.1). */
	void OpenHub()
	{
		RL.SiteShown = ss::net::SiteId::RiverLine;
		RL.ShowSeries(std::string(), Now);
		RL.OpenPage(RiverLine::Page::Series, Now);
		Now = ui_test::Step(S, Now, 1.5);
	}
	/** FH-1b: the pointer at (X, Y) for one frame, then a settled frame; One: pressed and released inside that one frame (a quick tap at
	 *  30 Hz), else a press frame and a release frame. Returns the settled frame. */
	DrawList Click(float X, float Y, bool One = false)
	{
		RL.UI.Ptr.Active = true;
		RL.UI.Ptr.X = X;
		RL.UI.Ptr.Y = Y;
		RL.UI.Ptr.Pressed = true;
		RL.UI.Ptr.Down = !One;
		RL.UI.Ptr.Released = One;
		Frame(1.0f);
		if (!One)
		{
			RL.UI.Ptr.Down = false;
			RL.UI.Ptr.Released = true;
			Frame(1.0f);
		}
		Away();
		return Frame(1.0f);
	}
	/** The pointer off the screen, still. */
	void Away()
	{
		RL.UI.Ptr.Active = false;
		RL.UI.Ptr.X = -1.0f;
		RL.UI.Ptr.Y = -1.0f;
		RL.UI.Ptr.Down = false;
		RL.UI.Ptr.Pressed = false;
		RL.UI.Ptr.Released = false;
	}
};

/** How many text runs are exactly Text. */
int HubsRuns(const DrawList& L, const std::string& Text)
{
	int N = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		N += Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text ? 1 : 0;
	}
	return N;
}

/** The first run exactly Text: its box's left and its baseline ({-1, -1} when none). */
std::pair<float, float> HubsRunAt(const DrawList& L, const std::string& Text)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text)
		{
			return {Cmd.Text.X, Cmd.Text.BaselineY};
		}
	}
	return {-1.0f, -1.0f};
}

/** FH-6a: how many times Id played from the laptop's cue From on. */
int HubsCues(const HubsHooks& H, ss::SoundId Id, size_t From)
{
	int N = 0;
	for (size_t I = From; I < H.Sounds.size(); ++I)
	{
		N += H.Sounds[I] == Id ? 1 : 0;
	}
	return N;
}

/** A page's cost (§2.8.10's columns): vertices, text runs, NetSpaced lines (a baseline holding three or more one-glyph runs in a
 *  row, as NetSpaced draws them), commands. */
struct HubsCost
{
	size_t Vertices = 0, Runs = 0, Cmds = 0;
	int Spaced = 0;
};

bool HubsGlyph(const std::string& T)
{
	return !T.empty() && (T.size() == 1 || (static_cast<unsigned char>(T[0]) >= 0xC0 && T.size() <= 4));
}

HubsCost HubsCostOf(const DrawList& L)
{
	HubsCost K;
	K.Vertices = L.Vertices.size();
	K.Cmds = L.Cmds.size();
	float Base = -1.0e9f;
	int Run = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text)
		{
			continue;
		}
		++K.Runs;
		if (HubsGlyph(Cmd.Text.Text) && Cmd.Text.BaselineY == Base)
		{
			if (++Run == 3)
			{
				++K.Spaced;
			}
			continue;
		}
		Run = HubsGlyph(Cmd.Text.Text) ? 1 : 0;
		Base = Run ? Cmd.Text.BaselineY : -1.0e9f;
	}
	return K;
}

/** FH-7, FestHubBudgets: what the careers gather as they walk (the hosts' states drawn, emitted or not; the lobby's and the Career
 *  page's costs), read by FestHubBudgetsSummary once every career has run. */
struct HubsBudgetBook
{
	std::map<std::pair<int, int>, std::string> States; // {host, phase} -> the first page or drawn state that showed it
	int Measured = 0, Timed = 0;
	double WorstMs = 0.0; // the largest warm draw over its bound (SS_PERF)
	std::string WorstAt;
	size_t LobbyBase[2] = {0, 0}; // Night One's lobby at 1.0 and 1.5 (the measured cost: 29.9k at 1.0)
	std::vector<std::pair<std::string, std::pair<size_t, size_t>>> Lobbies; // the lobby with festival crests at 1.0 and 1.5
	size_t CareerBefore[2] = {0, 0}, CareerAfter[2] = {0, 0};
};

HubsBudgetBook& HubsBook()
{
	static HubsBudgetBook Book;
	return Book;
}

/** What a measured frame is (§2.8.10's rows): a hub page or a drawn hub state, a page with the popover over the hub, a moment's or the
 *  mark card's page over the page beneath, another page (the Career page, a card). */
enum class HubsKind
{
	Hub,
	Popover,
	Moment,
	Mark,
	Other
};

HubsKind HubsKindOf(const std::string& Name)
{
	if (Name.rfind("fest_hub_", 0) == 0 || Name == "fest_strip_night_one")
	{
		return HubsKind::Hub;
	}
	if (Name.rfind("fest_popover", 0) == 0)
	{
		return HubsKind::Popover;
	}
	if (Name == "fest_moment_mark")
	{
		return HubsKind::Mark;
	}
	return Name.rfind("fest_moment_", 0) == 0 ? HubsKind::Moment : HubsKind::Other;
}

/** SS_PERF=1 (u4_ui_perf): the warm draw at scale 1.0, the median of five means of 20 draws after a cold one (§8.1's timing rule). */
double HubsWarmMs(HubsLaptop& P)
{
	P.Frame(1.0f);
	std::array<double, 5> Means{};
	for (double& Ms : Means)
	{
		const auto T0 = std::chrono::steady_clock::now();
		for (int I = 0; I < 20; ++I)
		{
			P.Frame(1.0f);
		}
		Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count() / 20.0;
	}
	std::sort(Means.begin(), Means.end());
	return Means[2];
}

/** One frame measured at both scales (FestHubBudgets): emitted as Name first (Emit), else drawn only (a hub state no page shows). A hub
 *  frame holds §2.8.10's hub row at each scale and records its host and phase (read back from the hub's own live frame); under SS_PERF=1
 *  the warm draw holds its row (the hub 1.5 ms, 2.0 on a broadcast frame; a moment 2.0; the mark card 1.5), under SS_UI_BENCH=1 it
 *  prints (100 draws after one cold). Returns the 1.0 list. */
DrawList HubsMeasure(const std::string& Name, HubsLaptop& P, bool Emit)
{
	if (Emit)
	{
		ui_test::Emit(Name, P.RL, P.Now);
	}
	HubsBudgetBook& Book = HubsBook();
	++Book.Measured;
	const HubsKind Kind = Emit ? HubsKindOf(Name) : HubsKind::Hub;
	DrawList One;
	bool Broadcast = false;
	std::string Key;
	for (const float Scale : {1.0f, 1.5f})
	{
		const DrawList L = P.Frame(Scale);
		const HubsCost K = HubsCostOf(L);
		if (Kind == HubsKind::Hub)
		{
			std::printf("  %-24s x%.1f: %6zu vertices (hub <= 35000), %3zu runs (<= 600), %d NetSpaced (<= 4), %4zu commands%s\n", Name.c_str(), Scale, K.Vertices, K.Runs, K.Spaced,
				K.Cmds, Emit ? "" : " (drawn, no page)");
		}
		else
		{
			std::printf("  %-24s x%.1f: %6zu vertices, %3zu runs, %d NetSpaced, %4zu commands (%s)\n", Name.c_str(), Scale, K.Vertices, K.Runs, K.Spaced, K.Cmds,
				Kind == HubsKind::Popover ? "the popover's row over the page is asserted beside it" : Kind == HubsKind::Other ? "no hub row" : "the card's row over the page beneath");
		}
		if (Scale == 1.0f)
		{
			One = L;
			Key = ss::ui::rlfest_detail::HubShownKey(P.RL, P.Now, Broadcast);
		}
		if (Kind == HubsKind::Hub)
		{
			char Buf[256];
			std::snprintf(Buf, sizeof(Buf), "FestHubBudgets: %s at x%.1f within the hub's row (%zu vertices <= 35000, %zu runs <= 600, %d NetSpaced <= 4)", Name.c_str(),
				static_cast<double>(Scale), K.Vertices, K.Runs, K.Spaced);
			ui_test::Expect(K.Vertices <= 35000 && K.Runs <= 600 && K.Spaced <= 4, Buf);
		}
	}
	const ss::fest::Festival* Shown = Key.empty() ? nullptr : ss::fest::FestivalOf(Key);
	if (Kind == HubsKind::Hub)
	{
		ui_test::Expect(Shown != nullptr, ("FestHubBudgets: " + Name + " is a hub frame (the hub drew it)").c_str());
	}
	if (Shown)
	{
		const int Ph = static_cast<int>(ss::fest::PhaseOf(*Shown, P.S.WorldMinutes(), &P.S.Living()));
		Book.States.emplace(std::make_pair(static_cast<int>(Shown->Site), Ph), Name);
	}
	const double Bound = Kind == HubsKind::Hub ? (Broadcast ? 2.0 : 1.5) : Kind == HubsKind::Moment ? 2.0 : Kind == HubsKind::Mark ? 1.5 : 0.0;
	if (std::getenv("SS_PERF") && Bound > 0.0)
	{
		const double Ms = HubsWarmMs(P);
		++Book.Timed;
		if (Ms / Bound > Book.WorstMs)
		{
			Book.WorstMs = Ms / Bound;
			Book.WorstAt = Name;
		}
		std::printf("  %-24s warm draw %.3f ms (SS_PERF: the median of five, <= %.1f ms)\n", Name.c_str(), Ms, Bound);
		char Buf[200];
		std::snprintf(Buf, sizeof(Buf), "FestHubBudgets: %s's warm draw %.3f ms within %.1f ms (SS_PERF, the median of five)", Name.c_str(), Ms, Bound);
		ui_test::Expect(Ms <= Bound, Buf);
	}
	else if (std::getenv("SS_UI_BENCH"))
	{
		P.Frame(1.0f);
		const auto T0 = std::chrono::steady_clock::now();
		for (int I = 0; I < 100; ++I)
		{
			P.Frame(1.0f);
		}
		const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count() / 100.0;
		std::printf("  %-24s warm draw %.3f ms (printed; SS_PERF=1 asserts%s)\n", Name.c_str(), Ms, Bound > 0.0 ? "" : ": no ms row");
	}
	return One;
}

/** Emits Name (scale 1.0) and measures it at both scales (HubsMeasure). Returns the 1.0 list. */
DrawList HubsPage(const std::string& Name, HubsLaptop& P)
{
	return HubsMeasure(Name, P, true);
}

/** FH-7: a hub state no page shows, drawn and measured (no page). */
void HubsDrawn(const std::string& Tag, HubsLaptop& P)
{
	HubsMeasure(Tag, P, false);
}

/** FH-7: RiverLine's lobby settled (a second after it opens), its vertices at 1.0 and 1.5 (the lobby rows' row of §2.8.10). */
std::pair<size_t, size_t> HubsLobbyCost(HubsLaptop& P)
{
	P.RL.SiteShown = ss::net::SiteId::RiverLine;
	P.RL.OpenPage(RiverLine::Page::Lobby, P.Now);
	P.Now = ui_test::Step(P.S, P.Now, 1.0);
	P.Away();
	const size_t One = P.Frame(1.0f).Vertices.size();
	return {One, P.Frame(1.5f).Vertices.size()};
}

/** FH-7: RiverLine's Career page, its vertices at 1.0 and 1.5 into Out (the Career row: the page inside its cost before the hero's
 *  festival pieces + 4k). */
void HubsCareerCost(HubsLaptop& P, size_t (&Out)[2])
{
	P.RL.SiteShown = ss::net::SiteId::RiverLine;
	P.RL.OpenPage(RiverLine::Page::Career, P.Now);
	P.Away();
	P.Frame(1.0f);
	P.Now = ui_test::Step(P.S, P.Now, 1.0);
	Out[0] = P.Frame(1.0f).Vertices.size();
	Out[1] = P.Frame(1.5f).Vertices.size();
}

/** FestHonestLines (§2.15.2; F-hubs' A half, FH-1a: U9's empty roll): Night One's hub opens on CHAMPIONS (RCOP 2026 is Announced) and
 *  shows the two honest lines while the roll is empty, "No champion yet. The first name goes up on {Date}." with the Main's date
 *  (Nov 8) and "The record books opened Oct 6, 2026."; neither hub page holds the old typed champion's prize ("4,108,220", "4.1M").
 *  Stream B's half (FH-3a, FH-6b) adds the slide, the claim forms, the goals and the true defenders. Plant: CHAMPIONS' empty state
 *  printing the old claim "Last year's champion won $4,108,220." in U9's place. */
void FestHonestLinesHubs(const DrawList& NightOne, const DrawList& Buildup)
{
	const ss::fest::Festival* Rcop = ss::fest::FestivalOf("rcop");
	ui_test::Expect(Rcop != nullptr && Rcop->MainDay == 34, "FestHonestLines: RCOP 2026 is a festival with its Main on day 34 (F-net's FN-1 row)");
	const std::string FirstName = "No champion yet. The first name goes up on " + (Rcop ? ss::net::DateLabel(Rcop->MainDay) : std::string("?")) + ".";
	ui_test::Expect(FirstName == "No champion yet. The first name goes up on Nov 8.", ("FestHonestLines: U9's date is the Main's, Nov 8 (" + FirstName + ")").c_str());
	ui_test::Expect(ui_test::DrewRun(NightOne, "CHAMPIONS") && ui_test::DrewRun(NightOne, "THE RACE"), "FestHonestLines: Night One's SERIES page is RCOP 2026's hub, its tabs drawn");
	ui_test::Expect(ui_test::DrewRun(NightOne, FirstName), ("FestHonestLines: Night One's hub shows \"" + FirstName + "\"").c_str());
	ui_test::Expect(ui_test::DrewRun(NightOne, "The record books opened Oct 6, 2026."), "FestHonestLines: Night One's hub shows \"The record books opened Oct 6, 2026.\"");
	ui_test::Expect(ui_test::DrewRun(Buildup, FirstName), "FestHonestLines: the Buildup hub (day 6) still opens on CHAMPIONS' honest lines");
	for (const DrawList* L : {&NightOne, &Buildup})
	{
		for (const char* Claim : {"4,108,220", "4.1M"})
		{
			ui_test::Expect(!ui_test::Drew(*L, Claim), (std::string("FestHonestLines: no hub page holds the typed past prize ") + Claim).c_str());
		}
	}
}

void FestWaveClearHubs(HubsLaptop& P, const ss::fest::Festival& Rcop); // g1p-hub, below

/** FH-1a's pages: fest_hub_rl_announced (Night One: RCOP 2026 Announced, the countdown over OPENS IN) and fest_hub_rl_buildup (day 6,
 *  RCOP's Buildup: FirstDay 13 - 7). */
void FestHubPagesRiverLine()
{
	HubsLaptop P("u4-fest-hub");
	const ss::fest::Festival* Rcop = ss::fest::FestivalOf("rcop");
	P.OpenHub();
	const int Day0 = ss::net::DayOf(P.S.WorldMinutes());
	ui_test::Expect(Rcop && ss::fest::PhaseOf(*Rcop, P.S.WorldMinutes(), nullptr) == ss::fest::Phase::Announced, "fest_hub_rl_announced: RCOP 2026 is Announced on Night One");
	const DrawList NightOne = HubsPage("fest_hub_rl_announced", P);
	ui_test::Expect(ui_test::DrewRun(NightOne, "OPENS IN"), "fest_hub_rl_announced: the countdown stands over OPENS IN");
	std::printf("  Night One is world day %d; RCOP 2026 opens on day %d\n", Day0, Rcop ? Rcop->FirstDay : -1);
	P.At(static_cast<double>(Rcop ? Rcop->FirstDay - ss::fest::kBuildupDays : 6) * ss::net::MinutesPerDay + 10.0 * 60.0);
	P.OpenHub();
	ui_test::Expect(Rcop && ss::fest::PhaseOf(*Rcop, P.S.WorldMinutes(), nullptr) == ss::fest::Phase::Buildup, "fest_hub_rl_buildup: RCOP 2026 is in its Buildup on day 6");
	const DrawList Buildup = HubsPage("fest_hub_rl_buildup", P);
	FestHonestLinesHubs(NightOne, Buildup);
	// FestCopyUi's U2 half (FestBlurbsFilled, PLAN_G1 §1 item 9): the banner's state figure is the edition's own, filled from the clock.
	if (Rcop)
	{
		const long long M = static_cast<long long>(std::floor(static_cast<double>(Rcop->FirstDay) * ss::net::MinutesPerDay - P.S.WorldMinutes()));
		char Want[32];
		std::snprintf(Want, sizeof(Want), "%lldD %02lldH %02lldM", M / 1440, (M / 60) % 24, M % 60);
		ui_test::Expect(ui_test::DrewRun(Buildup, Want), (std::string("FestCopyUi: the Buildup hub's countdown is RCOP 2026's own, ") + Want).c_str());
		const std::string Dates = ss::net::DateLabel(Rcop->FirstDay) + " \xE2\x80\x93 " + ss::net::DateLabel(Rcop->LastDay);
		ui_test::Expect(ui_test::DrewRun(Buildup, Dates), ("FestCopyUi: the banner's DATES are the edition's, " + Dates).c_str());
	}
	// FH-1b: THE FESTIVAL YEAR on Night One: RCOP 2026 (Announced, the next to open) lit and underlined in its calendar slot (fsy8); the
	// brands F-net has built so far each in theirs, the rest empty until their rows land.
	ui_test::Expect(ui_test::DrewRun(NightOne, "RCOP") && ui_test::DrewRun(NightOne, "Oct 18"), "THE FESTIVAL YEAR: RCOP 2026's cell, its Short and its opening date");
	ui_test::Expect(ui_test::TextAt(NightOne, "RCOP") > 84.0f && ui_test::TextAt(NightOne, "RCOP") < 124.0f, "THE FESTIVAL YEAR: RCOP's cell sits in the strip's rect");
	// g1p-hub: RCOP's waves behind the header, through a day of the Buildup (this laptop's pages are all drawn above).
	if (Rcop)
	{
		const auto G0 = std::chrono::steady_clock::now();
		FestWaveClearHubs(P, *Rcop);
		std::printf("  g1p-hub: FestWaveClear %.2f s\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - G0).count());
	}
}

/** The settings the clause half needs: a running deal that plays only on Ignite, so a RiverLine registration strikes (update 2 §5.7,
 *  the sites' SitesClause precedent); "" when the sponsors' core offers none to sign (the half is then skipped, said so). */
std::string HubsClauseOnIgnite(ss::Session& S)
{
	S.DebugSponsor("offer riverline");
	for (ss::sponsor::Contract& K : S.Life.Sponsors.Contracts)
	{
		if (K.Brand == "riverline")
		{
			K.T.Only = "ignite";
		}
	}
	if (!S.Life.Sponsors.Contracts.empty())
	{
		S.SignDeal(S.Life.Sponsors.Contracts.back().Id);
	}
	return ss::ui::rlsites_detail::SitesClauseWarning(S, ss::net::SiteId::RiverLine, false, false);
}

/** FestStrikeGate (§2.15.2; FH-1b): the hub's register buttons are the client's RegisterBlock, so a row registers only through
 *  SitesTwoPress's second press: off the clause the first press arms "Confirm" (Session::ConfirmRegister) and registers nothing, the
 *  second registers; under a "plays only on" clause the first press reads the strike, the second ("Take the strike") passes
 *  StrikeConfirm{true} to core's gate (W3). One confirm step at a time, the pressed row's. On RCOP 2026's first day, 30 minutes before
 *  the first joinable event, so its row is open (RegisterBlock opens an hour ahead). Plant: a hub row calling Session::RegisterEvent
 *  directly. Also FestCopyUi's live figure: the banner reads "DAY 1 OF 22". */
void FestStrikeGateHubs()
{
	const ss::fest::Festival* Rcop = ss::fest::FestivalOf("rcop");
	if (!Rcop)
	{
		ui_test::Expect(false, "FestStrikeGate: RCOP 2026 is a festival");
		return;
	}
	const ss::net::Network& Net = ss::net::Shared();
	for (const bool Clause : {false, true})
	{
		HubsLaptop P(Clause ? "u4-fest-strike-clause" : "u4-fest-strike");
		ss::Session& S = P.S;
		S.BankrollCents = 50000000;
		ss::net::EventInstance E;
		bool Found = false;
		for (const ss::net::EventInstance& I : Net.Window(static_cast<double>(Rcop->FirstDay) * ss::net::MinutesPerDay, static_cast<double>(Rcop->FirstDay + 1) * ss::net::MinutesPerDay))
		{
			const ss::net::EventTemplate& T = Net.TemplateOf(I);
			if (!Found && T.Series == "rcop" && T.BuyInCents <= 100000 && Net.Joinable(I, nullptr, S.Unlocks()))
			{
				E = I;
				Found = true;
			}
		}
		ui_test::Expect(Found, "FestStrikeGate: RCOP 2026's first day has a joinable event under $1,000");
		if (!Found)
		{
			return;
		}
		P.At(E.Start - 30.0);
		std::string Warn;
		if (Clause)
		{
			Warn = HubsClauseOnIgnite(S);
			if (Warn.empty())
			{
				std::printf("  FestStrikeGate: no deal to sign that plays only on Ignite; the clause half is skipped\n");
				return;
			}
		}
		P.OpenHub();
		P.Away();
		DrawList L = P.Frame(1.0f);
		const std::vector<std::string> Rows = P.RL.ListedEvents();
		const auto At = std::find(Rows.begin(), Rows.end(), E.Id);
		const int Index = At == Rows.end() ? -1 : static_cast<int>(At - Rows.begin());
		ui_test::Expect(Index >= 0 && Index < 7, ("FestStrikeGate: " + E.Id + " is one of the day's rows in view (written to Listed)").c_str());
		if (Index < 0 || Index >= 7)
		{
			return;
		}
		if (!Clause)
		{
			ui_test::Expect(ui_test::DrewRun(L, "DAY 1 OF 22"), "FestCopyUi: RCOP 2026's first day reads DAY 1 OF 22 (U2, the edition's days)");
		}
		const float RowY = 408.0f + 72.0f * static_cast<float>(Index);
		const float Bx = 24.0f + 1000.0f - 372.0f; // the row's button {R.X + R.W - 372, R.Y + 10, 360, 46}
		const std::string First = Clause ? Warn : "Register \xC2\xB7 " + ss::net::BuyIn(Net.TemplateOf(E).BuyInCents);
		std::pair<float, float> Btn = HubsRunAt(L, First);
		if (Clause && Btn.second < 0.0f)
		{
			Btn = HubsRunAt(L, ss::ui::rlsites_detail::SitesStrikeShort); // the warning shortened to fit the row's button (SitesFitButton)
		}
		ui_test::Expect(Btn.second > RowY && Btn.second < RowY + 66.0f, ("FestStrikeGate: the row's button is RegisterBlock's (\"" + First + "\")").c_str());
		const ss::SoundId Sting = ss::ui::rlfest_detail::HubStingOf(ss::ui::festart::LookOf(Rcop->Which), ss::net::SiteId::RiverLine);
		P.Now = ui_test::Step(P.S, P.Now, 0.2); // past the open's sting (the session plays an id once in 50 ms)
		const size_t Cues = P.H.Sounds.size();
		L = P.Click(Bx + 180.0f, RowY + 33.0f);
		ui_test::Expect(!S.IsPlaying(E.Id), Clause ? "FestStrikeGate: under the clause the first press registers nothing" : "FestStrikeGate: the first press registers nothing");
		ui_test::Expect(HubsCues(P.H, Sting, Cues) == 0, "FestPreviewStill (the registration sting): the first press stings nothing");
		ui_test::Expect(S.ConfirmRegister && P.RL.SelectedEvent() == E.Id, "FestStrikeGate: the first press arms the confirm step, for the row pressed");
		const std::string Ok = Clause ? "Take the strike" : "Confirm";
		ui_test::Expect(HubsRuns(L, Ok) == 1 && ui_test::DrewRun(L, "Cancel"), ("FestStrikeGate: one confirm step (\"" + Ok + "\", Cancel), the pressed row's alone").c_str());
		if (Clause)
		{
			ui_test::Expect(ui_test::Drew(L, ss::ui::rlsites_detail::SitesStrikeConfirm), "FestStrikeGate: under the clause the confirm step says it's a strike");
		}
		P.Click(Bx + 60.0f, RowY + 33.0f); // the confirm step's left half ("regok")
		ui_test::Expect(S.IsPlaying(E.Id), Clause ? "FestStrikeGate: \"Take the strike\" registers through core's gate" : "FestStrikeGate: the second press registers");
		P.Frame(1.0f);
		ui_test::Expect(HubsCues(P.H, Sting, Cues) == 1, ("FestPreviewStill (the registration sting): the confirmed festival registration stings once, after core took it (" +
			std::to_string(HubsCues(P.H, Sting, Cues)) + ")").c_str());
	}
}

/** FH-1b's page fest_popover (RCOP's cell on Night One) and FestOneFrameClicks' popover half (§2.15.2): the click that opens the
 *  popover fires nothing beneath it and doesn't close it; a click on the popover reaches nothing under it (the Main card's Details,
 *  beneath its foot, stays shut); a click off it closes it and is swallowed (the hub stays on CHAMPIONS); OPEN THE HUB shows the cell's
 *  edition and closes it. The popover row of §2.8.10 (<= 3k vertices, <= 20 runs over the page) is asserted (FH-7, FestHubBudgets).
 *  Plant: the popover drawn without SitesBlockUnder. */
void FestPopoverHubs()
{
	HubsLaptop P("u4-fest-popover");
	P.OpenHub();
	P.Away();
	const DrawList Before = P.Frame(1.0f);
	ui_test::Expect(!ui_test::DrewRun(Before, "OPEN THE HUB"), "fest_popover: no popover before a click");
	const float CellX = 260.0f + 8.0f * 128.0f + 50.0f; // RCOP's cell, fsy8
	const DrawList Opened = P.Click(CellX, 104.0f, true);
	ui_test::Expect(ui_test::DrewRun(Opened, "OPEN THE HUB") && ui_test::DrewRun(Opened, "RCOP 2026"), "fest_popover: a one-frame click on RCOP's cell opens its popover");
	ui_test::Expect(P.RL.CurrentPage() == RiverLine::Page::Series, "FestOneFrameClicks: the click that opens the popover fires nothing else");
	ui_test::Expect(ui_test::DrewRun(Opened, "Opens Oct 18.") && ui_test::DrewRun(Opened, "Your account is open.") && ui_test::DrewRun(Opened, "RiverLine \xC2\xB7 Oct 18 \xE2\x80\x93 Nov 8"),
		"fest_popover: the phase line, the account line and the host and window (U16)");
	const DrawList Page = HubsPage("fest_popover", P);
	const HubsCost With = HubsCostOf(Page);
	const HubsCost Without = HubsCostOf(Before);
	const long long PopV = static_cast<long long>(With.Vertices) - static_cast<long long>(Without.Vertices);
	const long long PopR = static_cast<long long>(With.Runs) - static_cast<long long>(Without.Runs);
	std::printf("  fest_popover: the popover %lld vertices (<= 3000), %lld runs (<= 20) over the page\n", PopV, PopR);
	ui_test::Expect(PopV <= 3000 && PopR <= 20, ("FestHubBudgets: the popover within 3k vertices and 20 runs over the page (\xC2\xA7" "2.8.10; " + std::to_string(PopV) + ", " +
		std::to_string(PopR) + ")").c_str());
	const std::string Picked = P.RL.SelectedEvent();
	const DrawList OnIt = P.Click(1500.0f, 290.0f); // on the popover's foot, over the Main card's Details
	ui_test::Expect(P.RL.CurrentPage() == RiverLine::Page::Series && P.RL.SelectedEvent() == Picked, "FestOneFrameClicks: a click on the popover reaches nothing beneath it (Details stays shut)");
	ui_test::Expect(ui_test::DrewRun(OnIt, "OPEN THE HUB"), "FestOneFrameClicks: a click on the popover keeps it open");
	const DrawList Off = P.Click(1100.0f, 366.0f); // off it, over THE RACE's tab
	ui_test::Expect(!ui_test::DrewRun(Off, "OPEN THE HUB"), "FestOneFrameClicks: a click off the popover closes it");
	ui_test::Expect(ui_test::DrewRun(Off, "No champion yet. The first name goes up on Nov 8.") && !ui_test::DrewRun(Off, "No points yet."),
		"FestOneFrameClicks: the click that closes it is swallowed (the hub stays on CHAMPIONS)");
	P.Click(CellX, 104.0f, true);
	const std::pair<float, float> Go = HubsRunAt(P.Frame(1.0f), "OPEN THE HUB");
	const DrawList Shown = P.Click(Go.first + 20.0f, Go.second - 4.0f);
	ui_test::Expect(Go.first > 0.0f && !ui_test::DrewRun(Shown, "OPEN THE HUB") && P.RL.CurrentPage() == RiverLine::Page::Series && ui_test::DrewRun(Shown, "RCOP 2026"),
		"fest_popover: OPEN THE HUB shows RCOP 2026's hub and closes the popover");
}

/** The UTF-8 code points of S. */
std::vector<uint32_t> HubsCodePoints(const std::string& S)
{
	std::vector<uint32_t> Out;
	for (size_t I = 0; I < S.size();)
	{
		const unsigned char B = static_cast<unsigned char>(S[I]);
		const size_t Len = B >= 0xF0 ? 4 : B >= 0xE0 ? 3 : B >= 0xC0 ? 2 : 1;
		uint32_t Cp = Len == 1 ? B : Len == 2 ? (B & 0x1Fu) : Len == 3 ? (B & 0x0Fu) : (B & 0x07u);
		for (size_t K = 1; K < Len && I + K < S.size(); ++K)
		{
			Cp = (Cp << 6) | (static_cast<unsigned char>(S[I + K]) & 0x3Fu);
		}
		Out.push_back(Cp);
		I += Len;
	}
	return Out;
}

/** The words of a lower-case line (letters, digits and ''' joined). */
std::vector<std::string> HubsWords(const std::string& Low)
{
	std::vector<std::string> Out(1);
	for (const char Ch : Low)
	{
		if (std::isalnum(static_cast<unsigned char>(Ch)) || Ch == '\'')
		{
			Out.back() += Ch;
		}
		else if (!Out.back().empty())
		{
			Out.emplace_back();
		}
	}
	if (Out.back().empty())
	{
		Out.pop_back();
	}
	return Out;
}

/** What's wrong with one FestUiInventory entry ("" when nothing), its kind and text, filled from Vars (§2.11): the kind's stop rule
 *  and cap (a sentence: CopyLint at 200; a label: no closing stop, at most 84), U1, both trademark tiers, the design words, the
 *  avoided words, X1 (no count of people as "players"), FestChampWord and the glyph rule, all after the fill. */
std::string HubsCopyWhy(const std::string& Kind, const std::string& Text, const std::map<std::string, std::string>& Vars)
{
	const std::string Line = copylint::Fill(Text, Vars);
	if (Kind == "sentence")
	{
		const std::string Why = copylint::Check(Text, copylint::Pa, Vars);
		if (!Why.empty())
		{
			return Why;
		}
	}
	else if (Kind == "label")
	{
		if (Line.empty() || Line.find('{') != std::string::npos || Line.find('}') != std::string::npos)
		{
			return "a variable with no sample: " + Line;
		}
		if (copylint::Characters(Line) > copylint::TableTalk)
		{
			return "a label longer than 84 characters";
		}
		if (Line.back() == '.' || Line.back() == '!' || Line.back() == '?' || Line.find("  ") != std::string::npos || Line.front() == ' ' || Line.back() == ' ')
		{
			return "a label with a closing stop or a stray space";
		}
	}
	else
	{
		return "no kind (label or sentence)";
	}
	const std::string Low = trademarks::Lower(Line);
	for (const char* W : {"ai", "bot", "bots", "autoplay", "autopilot"})
	{
		if (trademarks::HasWord(Low, W))
		{
			return std::string("U1: \"") + W + "\"";
		}
	}
	if (const char* Hit = trademarks::Find(Line, trademarks::kSourceScan))
	{
		return std::string("the source tier: ") + Hit;
	}
	if (const char* Hit = trademarks::Find(Line, trademarks::kCopyScan))
	{
		return std::string("the copy tier: ") + Hit;
	}
	if (const char* Hit = trademarks::Find(Line, trademarks::kDesignWords))
	{
		return std::string("a design word: ") + Hit;
	}
	static const char* const HubsAvoided[] = {"spark", "flame", "blaze", "wildfire", "ember", "crest", "heat", "cold", "stamina", "momentum", "energy", "board",
		"watchlist", "headliners", "beef", "hall", "class", "boom", "millions", "jackpot", "run good", "supernova", "premier"};
	for (const char* W : HubsAvoided)
	{
		if (trademarks::HasWord(Low, W))
		{
			return std::string("an avoided word: ") + W;
		}
	}
	const std::vector<std::string> Words = HubsWords(Low);
	for (size_t I = 0; I < Words.size(); ++I)
	{
		const std::string& W = Words[I];
		if (W.size() >= 4 && W.compare(W.size() - 4, 4, "coop") == 0)
		{
			return "a *COOP acronym: " + W;
		}
		// FH-7: "{Festival} {Year} Player of the Series" (U20/U21, stream B's trophy case) names the belt's honour, not a count of people.
		const bool HubsHonour = W == "player" && I + 3 < Words.size() && Words[I + 1] == "of" && Words[I + 2] == "the" && Words[I + 3] == "series";
		if ((W == "players" || W == "player") && !HubsHonour && I > 0 && std::isdigit(static_cast<unsigned char>(Words[I - 1].back())))
		{
			return "X1: a count of people as players";
		}
		if (W == "ignite" || W == "ignite's")
		{
			for (size_t J = I > 2 ? I - 2 : 0; J < Words.size() && J <= I + 2; ++J)
			{
				if (Words[J].rfind("championship", 0) == 0)
				{
					return "FestChampWord: Ignite beside \"championship\"";
				}
			}
		}
	}
	for (const uint32_t Cp : HubsCodePoints(Line))
	{
		const bool Ok = (Cp >= 0x20 && Cp < 0x7F) || Cp == 0xB7 || Cp == 0x2014 || Cp == 0x2013 || Cp == 0x2026 || Cp == 0xE9 || Cp == 0x2019;
		if (!Ok)
		{
			char Buf[48];
			std::snprintf(Buf, sizeof(Buf), "a glyph outside the set: U+%04X", static_cast<unsigned>(Cp));
			return Buf;
		}
	}
	return std::string();
}

/** FestCopyUi (§2.15.2, §2.11; F-hubs' A half, FH-1b): every fest::FestUiInventory entry (this file's and stream B's, through
 *  HostUiStrings) carries its kind and passes HubsCopyWhy filled from a sample edition; a second fill with "The Championship Online
 *  2027" (after "the " or "next ", through fest::NameAfterArticle) never prints "the The", and a next-year date prints its year once
 *  fest::DateInSentence has F-net's body (H0's stub prints today's form: FH-2 merges FN-2a's helpers); U2's formats type no digit
 *  (FestBlurbsFilled' U2 half; the drawn figures are checked on the pages above). Plant: "bot" in a line. */
void FestCopyUiHubs()
{
	const std::vector<std::string> Inv = ss::fest::FestUiInventory();
	std::map<std::string, std::string> Vars = copylint::Samples();
	const std::pair<const char*, const char*> Extra[] = {{"D", "12"}, {"H", "07"}, {"M", "41"}, {"N", "4"}, {"Days", "22"}, {"FirstDate", "Oct 18"},
		{"LastDate", "Nov 8"}, {"Weekday", "SUN"}, {"Date", "Nov 8"}, {"Time", "3:00 PM"}, {"Gtd", "$78,203,400"}, {"BuyIn", "$5,250"}, {"Stack", "100,000"},
		{"Added", "$2,500"}, {"FESTIVAL", "RCOP"}, {"Festival", "RCOP 2026"}, {"Year", "2026"}, {"Site", "RiverLine"}, {"SITE", "IGNITE"}, {"Event", "$5,250 Main Event"},
		{"Overlay", "$412,600"}, {"Amount", "$1,280,000"}, {"Entries", "2,345"}, {"Countdown", "2H 10M"}, {"Clock", "0:41:12"},
		{"Nights", "12"}, {"Show1", "7 PM"}, {"Show2", "10 PM"}, {"Need", "20"}, {"Rank", "164th"}, {"Scorers", "2,214"}, {"Points", "290"},
		{"Events", "9"}, {"Cashes", "3"}, {"Best", "31st"}, {"Field", "1,964"}, {"MARK", "FINALIST"}, {"Dates", "Apr 7 and Apr 9"}, {"Prize", "$795.32"},
		{"Place", "31st"}, {"K", "2"}, {"Left", "1,212"}, {"Trophy", "The RCOP Cup"}, {"Journey", "On RiverLine since 2019. First title on record."},
		{"Satellite", "The Summons"}, {"S", "09"}, {"Name", "slychip"}, {"Tier", "Mini"}, {"Ordinal", "2nd"}, {"Grade", "GOLD \xC2\xB7 won an event"}};
	for (const auto& Kv : Extra)
	{
		Vars[Kv.first] = Kv.second;
	}
	const int Today = 1;
	const int NextYear = Today + 400;
	const bool DatesReal = ss::fest::DateInSentence(NextYear, Today) != ss::net::DateLabel(NextYear);
	int Bad = 0;
	int Labels = 0;
	int Sentences = 0;
	for (const std::string& Entry : Inv)
	{
		const size_t Tab = Entry.find('\t');
		const std::string Kind = Tab == std::string::npos ? std::string() : Entry.substr(0, Tab);
		const std::string Text = Tab == std::string::npos ? Entry : Entry.substr(Tab + 1);
		Labels += Kind == "label" ? 1 : 0;
		Sentences += Kind == "sentence" ? 1 : 0;
		std::string Why = HubsCopyWhy(Kind, Text, Vars);
		if (Why.empty())
		{
			// The second fill: The Championship Online 2027 (after "the " or "next ", as the hub prints it) and a next-year date.
			std::map<std::string, std::string> Second = Vars;
			Second["Date"] = ss::fest::DateInSentence(NextYear, Today);
			std::string Filled;
			for (size_t I = 0; I < Text.size();)
			{
				if (Text.compare(I, 10, "{Festival}") == 0)
				{
					const std::string Before = trademarks::Lower(Filled);
					const bool AfterArticle = (Before.size() >= 4 && Before.compare(Before.size() - 4, 4, "the ") == 0) || (Before.size() >= 5 && Before.compare(Before.size() - 5, 5, "next ") == 0);
					Filled += AfterArticle ? ss::fest::NameAfterArticle("The Championship Online 2027") : std::string("The Championship Online 2027");
					I += 10;
				}
				else
				{
					Filled += Text[I++];
				}
			}
			Filled = copylint::Fill(Filled, Second);
			if (trademarks::Lower(Filled).find("the the ") != std::string::npos)
			{
				Why = "\"the The\" in the second fill: " + Filled;
			}
			else if (DatesReal && Text.find("{Date}") != std::string::npos && Filled.find(std::to_string(ss::world::YearOf(NextYear))) == std::string::npos)
			{
				Why = "a next-year date without its year: " + Filled;
			}
		}
		if (!Why.empty())
		{
			++Bad;
			std::printf("COPY (FestUiInventory) \"%s\": %s\n", Entry.c_str(), Why.c_str());
		}
	}
	ui_test::Expect(Bad == 0, ("FestCopyUi: every FestUiInventory line passes the lint, both tiers, U1, the design and avoided words, X1, FestChampWord and the glyph rule (" +
		std::to_string(Labels) + " labels, " + std::to_string(Sentences) + " sentences, " + std::to_string(Bad) + " failing)").c_str());
	if (!DatesReal)
	{
		std::printf("  FestCopyUi: fest::DateInSentence is H0's stub (F-net's body lands in FN-2a, merged in FH-2): the next-year date check waits\n");
	}
	// U2's formats and U15's GRIT line (FestBlurbsFilled' U2/U15 half; FH-2 adds the NIGHT forms): in the inventory, no digit typed.
	for (const char* U2 : {"OPENS IN", "{D}D {H}H {M}M", "PROGRESS", "DAY {N} OF {Days}", "MAIN EVENT TODAY", "COMPLETE", "NIGHT {N} OF {Nights}", "NIGHT {N}",
			 "{FESTIVAL} \xC2\xB7 NIGHT {N} OF {Nights} \xC2\xB7 SHOWS AT {Show1} AND {Show2}"})
	{
		const bool Listed = std::find(Inv.begin(), Inv.end(), std::string("label\t") + U2) != Inv.end();
		std::string Bare; // the format without its placeholders' names ({Show1} is a variable, not a typed digit)
		for (const char* Ch = U2; *Ch; ++Ch)
		{
			if (*Ch == '{')
			{
				while (*Ch && *Ch != '}')
				{
					++Ch;
				}
				if (!*Ch)
				{
					break;
				}
				continue;
			}
			Bare += *Ch;
		}
		const bool Typed = Bare.find_first_of("0123456789") != std::string::npos;
		ui_test::Expect(Listed && !Typed, (std::string("FestCopyUi: U2's \"") + U2 + "\" is in FestUiInventory with no digit typed").c_str());
	}
	// FH-2: U30 and U33 are in the inventory with their kinds (so the lint above ran over them).
	for (const char* Line : {"label\tPASS {N}/{Need}", "sentence\tFestival Pass: {N} of {Need} online events.", "sentence\tOpens to you after your second rent.",
			 "label\t{N} SEATS AT THE POOL SO FAR", "label\t{N} SEATS WON"})
	{
		ui_test::Expect(std::find(Inv.begin(), Inv.end(), std::string(Line)) != Inv.end(), (std::string("FestCopyUi: FestUiInventory lists ") + Line).c_str());
	}
	// FH-4A: U5, U7, U10, U12, U27, U28's seat line, U37 and the roll's figures are listed with their kinds, every figure a placeholder
	// (U8's rungs print their own numbers: "PLAY 1", "TOP 25%").
	for (const char* Line : {"label\tEVERYONE", "label\tPEOPLE YOU KNOW", "label\tYou \xC2\xB7 {Rank} of {Scorers} in the race \xC2\xB7 {Points} pts",
			 "label\t{Events} events \xC2\xB7 {Cashes} cashes \xC2\xB7 best {Best} of {Field} \xC2\xB7 race {Rank} of {Scorers}", "label\tGOLD \xC2\xB7 won an event",
			 "label\tSILVER \xC2\xB7 cashed", "label\tOUTLINE \xC2\xB7 played", "label\t{N} pts to {MARK} \xC2\xB7 right now", "label\tA seat in the next Main Event",
			 "sentence\tGood on {Dates}.", "label\t{Prize} \xC2\xB7 1st of {Entries}", "label\t{Points} pts", "label\t{Place} of {Entries}"})
	{
		std::string Bare;
		for (const char* Ch = std::strchr(Line, '\t') + 1; *Ch; ++Ch)
		{
			if (*Ch == '{')
			{
				while (*Ch && *Ch != '}')
				{
					++Ch;
				}
				if (!*Ch)
				{
					break;
				}
				continue;
			}
			Bare += *Ch;
		}
		const bool Listed = std::find(Inv.begin(), Inv.end(), std::string(Line)) != Inv.end();
		const bool Typed = Bare.find_first_of("0123456789") != std::string::npos && std::string(Line).find("1st of") == std::string::npos;
		ui_test::Expect(Listed && !Typed, (std::string("FestCopyUi: FestUiInventory lists \"") + Line + "\" with no digit typed").c_str());
	}
	for (const char* Rung : {"PLAY 1", "CASH 1", "TOP 25%", "TOP 10%", "FINALIST", "PODIUM", "THE BELT", "MINI MAIN EVENT", "MICRO MAIN EVENT"})
	{
		ui_test::Expect(std::find(Inv.begin(), Inv.end(), std::string("label\t") + Rung) != Inv.end(), (std::string("FestCopyUi: FestUiInventory lists U8/U34's ") + Rung).c_str());
	}
	// U37 (SPEC v1.7 §2.16 step 8a: PENNANT 2027's Step 2 ticket on day 183, its runs on days 184 and 186): one date, two, three.
	namespace rf = ss::ui::rlfest_detail;
	const std::string Two = rf::HubGoodOn({184, 186}, 183);
	ui_test::Expect(Two == "Good on Apr 7 and Apr 9.", ("FestCopyUi: U37 reads \"Good on Apr 7 and Apr 9.\" for PENNANT 2027's Step 2 runs (" + Two + ")").c_str());
	ui_test::Expect(rf::HubGoodOn({186}, 185) == "Good on Apr 9." && rf::HubGoodOn({}, 185).empty() && rf::HubGoodOn({183, 184, 186}, 180) == "Good on Apr 6, Apr 7 and Apr 9.",
		"FestCopyUi: U37 with one day, none and three");
	// U10, singular at one ("1 event · 1 cash"), the race clause only while the hero scores.
	ss::fest::YourLineView V;
	V.Shown = true;
	V.Events = 1;
	V.Cashes = 1;
	V.Best = 3;
	V.Field = 1964;
	const std::string One = rf::HubYourCounts(V, true);
	V.Events = 9;
	V.Cashes = 0;
	V.Best = 31;
	V.Rank = 164;
	V.Scorers = 2214;
	const std::string Many = rf::HubYourCounts(V, true);
	ui_test::Expect(One == "1 event \xC2\xB7 1 cash \xC2\xB7 best 3rd of 1,964" && Many == "9 events \xC2\xB7 0 cashes \xC2\xB7 best 31st of 1,964 \xC2\xB7 race 164th of 2,214" &&
			rf::HubYourCounts(V, false) == "9 events \xC2\xB7 0 cashes \xC2\xB7 best 31st of 1,964",
		("FestCopyUi: U10's forms (" + One + " | " + Many + ")").c_str());
	// FH-4A fix round 1, U35 (§2.8.2): the five step-pill forms listed with their kinds, no digit typed but the pill numbers ({K} is
	// the next rung's, a placeholder); HubStepPills gives each rung its own pair, its seat form the inventory's filled.
	for (const char* Line : {"label\tSTEP 1", "label\tSTEP 2", "label\tSTEP 3", "label\tFOR A STEP {K} TICKET", "label\tFOR A MAIN EVENT SEAT"})
	{
		std::string Bare;
		for (const char* Ch = std::strchr(Line, '\t') + 1; *Ch; ++Ch)
		{
			if (*Ch == '{')
			{
				while (*Ch && *Ch != '}')
				{
					++Ch;
				}
				if (!*Ch)
				{
					break;
				}
				continue;
			}
			Bare += *Ch;
		}
		const bool PillNo = Bare.size() == 6 && Bare.compare(0, 5, "STEP ") == 0;
		const bool Listed = std::find(Inv.begin(), Inv.end(), std::string(Line)) != Inv.end();
		const bool Typed = !PillNo && Bare.find_first_of("0123456789") != std::string::npos;
		ui_test::Expect(Listed && !Typed, (std::string("FestCopyUi: FestUiInventory lists U35's \"") + Line + "\" with no digit typed but the pill number").c_str());
	}
	bool PillsOk = true;
	std::string PillsSeen;
	for (const std::pair<int, int>& Rung : {std::pair<int, int>{1, 2}, std::pair<int, int>{2, 3}, std::pair<int, int>{3, 0}})
	{
		const std::pair<std::string, std::string> Pp = rf::HubStepPills(Rung.first, Rung.second);
		const std::string Seat = Rung.second > 0 ? copylint::Fill("FOR A STEP {K} TICKET", {{"K", std::to_string(Rung.second)}}) : std::string("FOR A MAIN EVENT SEAT");
		PillsOk = PillsOk && Pp.first == "STEP " + std::to_string(Rung.first) && Pp.second == Seat &&
			std::find(Inv.begin(), Inv.end(), "label\t" + Pp.first) != Inv.end();
		PillsSeen += (PillsSeen.empty() ? "" : " | ") + Pp.first + " / " + Pp.second;
	}
	ui_test::Expect(PillsOk, ("FestCopyUi: U35's pills for Step 1, 2 and 3 (" + PillsSeen + ")").c_str());
	// FH-5b: U4, U13's broadcast forms, U14's sentences, U38's labels and the champion card's journey listed with their kinds (so the lint
	// above ran over them), every figure a placeholder but the singular's "1".
	for (const char* Line : {"label\tDEFENDING CHAMPION \xC2\xB7 {Name} \xC2\xB7 registered", "label\tMAIN EVENT DAY", "label\tREGISTRATIONS", "label\tTHE GUARANTEE",
			 "label\tPROJ SHORT \xC2\xB7 {Amount}", "label\tPROJ OVER \xC2\xB7 {Amount}", "label\tOVERLAY \xC2\xB7 {Amount}", "label\tWHO'S IN", "label\tLIVE \xC2\xB7 {Left} LEFT OF {Entries}",
			 "label\tFINAL TABLE", "label\tCHAMPION", "label\tREGISTERING", "label\tLATE REG", "label\tRUNNING", "label\t{H}:{M}:{S}", "sentence\tThe Main Event is under way.",
			 "sentence\t{Name} won {Trophy} and {Prize}. {Journey}", "sentence\t{N} regulars won their seat by satellite.", "sentence\t1 regular won their seat by satellite.",
			 "sentence\tOn {Site} since {Year}.", "sentence\tOn the scene since {Year}.", "sentence\tFirst title on record.", "sentence\t{N} titles on record.", "label\tYOU",
			 "label\tMet {N}x", "label\tDefending champion", "label\tWon a seat in {Satellite}", "label\tTeam pro", "label\tStreams on Kast", "label\t{Festival} champion",
			 "label\tPlayer of the Series, {Festival}"})
	{
		std::string Bare;
		for (const char* Ch = std::strchr(Line, '\t') + 1; *Ch; ++Ch)
		{
			if (*Ch == '{')
			{
				while (*Ch && *Ch != '}')
				{
					++Ch;
				}
				if (!*Ch)
				{
					break;
				}
				continue;
			}
			Bare += *Ch;
		}
		const bool Singular = Bare.compare(0, 2, "1 ") == 0;
		const bool Listed = std::find(Inv.begin(), Inv.end(), std::string(Line)) != Inv.end();
		const bool Typed = Bare.find_first_of("0123456789") != std::string::npos && !Singular;
		ui_test::Expect(Listed && !Typed, (std::string("FestCopyUi: FestUiInventory lists FH-5b's \"") + Line + "\" with no digit typed").c_str());
	}
}
// ------------------------------------------------------------------ FH-2: the real calendar in three frames

/** The text runs inside {X0, Y0}-{X1, Y1} (box left and baseline). */
int HubsRunsIn(const DrawList& L, float X0, float Y0, float X1, float Y1)
{
	int N = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		N += Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.X >= X0 && Cmd.Text.X < X1 && Cmd.Text.BaselineY >= Y0 && Cmd.Text.BaselineY < Y1 ? 1 : 0;
	}
	return N;
}

/** The run exactly Text with its baseline in [Y0, Y1): its box's left and baseline ({-1, -1} when none). */
std::pair<float, float> HubsRunBetween(const DrawList& L, const std::string& Text, float Y0, float Y1)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text && Cmd.Text.BaselineY >= Y0 && Cmd.Text.BaselineY < Y1)
		{
			return {Cmd.Text.X, Cmd.Text.BaselineY};
		}
	}
	return {-1.0f, -1.0f};
}

/** An instance by its id (an empty one when the site has none). */
ss::net::EventInstance HubsInstance(const ss::net::Network& Net, const std::string& Id)
{
	ss::net::EventInstance E;
	if (!Net.FindInstance(Id, E))
	{
		E = ss::net::EventInstance();
	}
	return E;
}

/** The brands of THE FESTIVAL YEAR in calendar order (§2.8.2's fsy0..9) and the Short each edition's series prints. */
const ss::fest::Brand HubsYearBrands[10] = {ss::fest::Brand::Writ, ss::fest::Brand::RingRush, ss::fest::Brand::Pennant, ss::fest::Brand::Grit, ss::fest::Brand::Tco,
	ss::fest::Brand::Reign, ss::fest::Brand::Slam, ss::fest::Brand::Push, ss::fest::Brand::Rcop, ss::fest::Brand::Feign};

std::string HubsShortOf(const ss::fest::Festival& F)
{
	const ss::net::SeriesInfo* Sr = ss::net::Site(F.Site).FindSeries(F.SeriesId);
	std::string Out = Sr ? Sr->Short : std::string();
	for (char& Ch : Out)
	{
		Ch = static_cast<char>(std::toupper(static_cast<unsigned char>(Ch)));
	}
	return Out;
}

/** A frame's strip (§2.8.2): the first cell's x, the pitch, the text column and the strip's top. */
struct HubsStrip
{
	const char* Host;
	float X0, Pitch, Text, Top;
};
constexpr HubsStrip HubsStripRiverLine{"RiverLine", 260.0f, 128.0f, 92.0f, 84.0f};
constexpr HubsStrip HubsStripRunItTwice{"RunItTwice", 368.0f, 116.0f, 80.0f, 96.0f};

/** FestStripTen's page half (§2.15.2, v1.6; FH-2) on one frame's hub: every brand of the ten that has an edition in reach draws its
 *  Short in its calendar slot (the run's box at X0 + Pitch * i + 36, i the brand's place in WRIT ... FEIGN), every cell (Pitch - 6
 *  wide) inside the content rect's right edge (1576 on all three hosts), a rival cell without an account drawing BY INVITATION in its
 *  text column. Returns the cells found. */
int FestStripTenPage(const DrawList& L, const HubsStrip& Sp, double World)
{
	const int Today = ss::net::DayOf(World);
	int Found = 0;
	for (int I = 0; I < 10; ++I)
	{
		// The brand's on-stage or next edition (PLAN_FHUBS Q24): the first edition of the brand whose window ends on or after today.
		const ss::fest::Festival* Ed = nullptr;
		for (const ss::fest::Festival* F : ss::fest::Festivals(Today - 14, Today + 400))
		{
			if (!Ed && F->Which == HubsYearBrands[I] && F->LastDay + 9 >= Today)
			{
				Ed = F;
			}
		}
		if (!Ed)
		{
			continue;
		}
		const std::string Short = HubsShortOf(*Ed);
		const std::pair<float, float> At = HubsRunBetween(L, Short, Sp.Top, Sp.Top + 40.0f);
		const float Want = Sp.X0 + Sp.Pitch * static_cast<float>(I) + 36.0f;
		ui_test::Expect(At.first >= 0.0f && std::fabs(At.first - Want) < 0.5f,
			(std::string("FestStripTen: ") + Sp.Host + "'s cell fsy" + std::to_string(I) + " holds " + Short + " (" + Ed->Name + ") at x " + std::to_string(static_cast<int>(Want))).c_str());
		const float Right = Sp.X0 + Sp.Pitch * static_cast<float>(I) + Sp.Pitch - 6.0f;
		ui_test::Expect(Right <= 1576.0f, (std::string("FestStripTen: ") + Sp.Host + "'s cell fsy" + std::to_string(I) + " ends inside the content rect").c_str());
		Found += At.first >= 0.0f ? 1 : 0;
	}
	return Found;
}

/** FestStripTen (§2.15.2, v1.6; FH-2): the ten cells fit both frames (128 from x 260 on RiverLine and Ignite, 116 from x 368 on
 *  RunItTwice: FestStripTenPage on each frame's page); every Short any edition prints fits its cell's text column (92 units at 13/800,
 *  80 on RunItTwice); "BY INVITATION" fits 80 at 9.5/800; the ids fsy0..9 are in calendar order (each cell's click opens its own
 *  brand's popover: HubsStripClicks). Also the Pass's three forms (§2.10 items 1 and 6, HubPassForm) on constructed gates. Plant: a
 *  ten-letter Short (it overflows the 80-unit column). */
void FestStripTenMeasures()
{
	ui_test::TableMeasurer M;
	DrawList Scratch;
	ss::ui::Canvas C(Scratch, M, RiverLine::Width, RiverLine::Height, 1.0f);
	int Shorts = 0;
	float Widest = 0.0f;
	std::string WidestShort;
	for (const ss::fest::Festival& F : ss::fest::All())
	{
		const std::string Short = HubsShortOf(F);
		const float W = C.Measure(Short, 13.0f, 800);
		++Shorts;
		if (W > Widest)
		{
			Widest = W;
			WidestShort = Short;
		}
		if (W > 80.0f)
		{
			ui_test::Expect(false, ("FestStripTen: " + F.Name + "'s Short " + Short + " measures " + std::to_string(W) + " at 13/800, past RunItTwice's 80-unit column").c_str());
		}
	}
	ui_test::Expect(Shorts > 0, "FestStripTen: F-net's calendar lists editions");
	std::printf("  FestStripTen: %d editions' Shorts measured, the widest %s at %.1f (columns 92 and 80)\n", Shorts, WidestShort.c_str(), Widest);
	const float Invite = C.Measure("BY INVITATION", 9.5f, 800);
	ui_test::Expect(Invite <= 80.0f, ("FestStripTen: BY INVITATION fits the 80-unit column at 9.5/800 (" + std::to_string(Invite) + ")").c_str());
	const float Pass = C.Measure("PASS 20/20", 9.5f, 800);
	ui_test::Expect(Pass <= 80.0f, ("FestStripTen: PASS 20/20 fits the 80-unit column at 9.5/800 (" + std::to_string(Pass) + ")").c_str());
	// The Pass's forms (GateMet's inputs, in its order): no Pass, a closed site, before the second rent, outside the festival weeks,
	// the first edition (two rents alone), and counting (N held at Need).
	const ss::fest::Festival* Feign2 = nullptr;
	const ss::fest::Festival* Feign1 = nullptr;
	for (const ss::fest::Festival& F : ss::fest::All())
	{
		Feign1 = !Feign1 && F.Which == ss::fest::Brand::Feign && F.Edition == 1 ? &F : Feign1;
		Feign2 = !Feign2 && F.Which == ss::fest::Brand::Feign && F.Edition == 2 ? &F : Feign2;
	}
	ui_test::Expect(Feign1 && Feign2, "FestStripTen: FEIGN's first and second editions are on the calendar");
	if (Feign1 && Feign2)
	{
		namespace rf = ss::ui::rlfest_detail;
		ss::net::InviteGate G;
		G.MinRentsPaid = 2;
		int N = 0;
		int Need = 0;
		ui_test::Expect(rf::HubPassForm(G, true, 2, Feign2, *Feign2, 30, N, Need) == 0, "FestStripTen: no Pass on the site (FestivalEvents -1) reads BY INVITATION");
		G.FestivalEvents = ss::fest::kPassEvents;
		ui_test::Expect(rf::HubPassForm(G, false, 2, Feign2, *Feign2, 30, N, Need) == 0, "FestStripTen: a site not open yet reads BY INVITATION");
		ui_test::Expect(rf::HubPassForm(G, true, 1, Feign2, *Feign2, 30, N, Need) == 1, "FestStripTen: before the second rent the popover says so (U30)");
		ui_test::Expect(rf::HubPassForm(G, true, 2, nullptr, *Feign2, 30, N, Need) == 0, "FestStripTen: outside the festival weeks the path is closed (BY INVITATION)");
		ui_test::Expect(rf::HubPassForm(G, true, 2, Feign1, *Feign1, 3, N, Need) == 0 && Need == 0, "FestStripTen: a first edition needs the two rents alone (no count shown)");
		ui_test::Expect(rf::HubPassForm(G, true, 2, Feign2, *Feign2, 7, N, Need) == 2 && N == 7 && Need == ss::fest::kPassEvents, "FestStripTen: the second edition counts, PASS 7/20");
		ui_test::Expect(rf::HubPassForm(G, true, 2, Feign2, *Feign2, 31, N, Need) == 2 && N == Need, "FestStripTen: the count holds at its Need once met");
	}
}

/** FestStripTen's ids (FH-2): a one-frame click on each drawn cell opens the popover of that cell's brand (its edition's name drawn as
 *  the popover's title), then a click on the scrim closes it. */
void HubsStripClicks(HubsLaptop& P, const HubsStrip& Sp)
{
	const int Today = ss::net::DayOf(P.S.WorldMinutes());
	for (int I = 0; I < 10; ++I)
	{
		const ss::fest::Festival* Ed = nullptr;
		for (const ss::fest::Festival* F : ss::fest::Festivals(Today - 14, Today + 400))
		{
			Ed = !Ed && F->Which == HubsYearBrands[I] && F->LastDay + 9 >= Today ? F : Ed;
		}
		if (!Ed)
		{
			continue;
		}
		const DrawList Open = P.Click(Sp.X0 + Sp.Pitch * static_cast<float>(I) + 50.0f, Sp.Top + 20.0f, true);
		const std::pair<float, float> Title = HubsRunBetween(Open, Ed->Name, Sp.Top + 40.0f, Sp.Top + 220.0f);
		ui_test::Expect(Title.first >= 0.0f, (std::string("FestStripTen: fsy") + std::to_string(I) + " on " + Sp.Host + " opens " + Ed->Name + "'s popover (calendar order)").c_str());
		P.Click(800.0f, 700.0f); // off it: the scrim closes it
	}
}

/** The Main's lock line on the hub's edition (Network::Joinable's own words), "" when it is joinable. */
std::string HubsMainLock(const ss::Session& S, const ss::fest::Festival& F)
{
	const ss::net::Network& Net = ss::net::Site(F.Site);
	ss::net::EventInstance Main;
	std::string Lock;
	if (!Net.Next(F.MainId, static_cast<double>(F.FirstDay) * ss::net::MinutesPerDay, Main) || Net.Joinable(Main, &Lock, S.Unlocks()))
	{
		return std::string();
	}
	return Lock;
}

/** FestLockedMain (§2.15.2, §2.2.3; FH-2), the ladder half on RCOP 2026's day 19 (a Saturday: step1-4 and mega-rcop all run): the day's
 *  rows scrolled to their end with the hub's wheel, each ladder row keeps its button (RegisterBlock's, in the row's button rect) and
 *  shows the Main's lock line and U31 beside it. Plant: the lock line dropped for mega-rcop. */
void FestLockedMainRows(HubsLaptop& P, const ss::fest::Festival& Rcop, const DrawList& L)
{
	const std::string Lock = HubsMainLock(P.S, Rcop);
	const std::string U31 = "A seat won here waits in your wallet until big fields open.";
	const std::vector<std::string> Rows = P.RL.ListedEvents();
	const ss::net::Network& Net = ss::net::Site(Rcop.Site);
	int Ladder = 0;
	for (const char* Id : {"step1", "step2", "step3", "step4", "mega-rcop"})
	{
		int Index = -1;
		for (size_t I = 0; I < Rows.size(); ++I)
		{
			Index = Index < 0 && Net.TemplateOf(HubsInstance(Net, Rows[I])).Id == Id ? static_cast<int>(I) : Index;
		}
		ui_test::Expect(Index >= 0, (std::string("FestLockedMain: ") + Id + " is one of day 19's rows").c_str());
		const int First = std::max(0, static_cast<int>(Rows.size()) - 7);
		if (Index < First)
		{
			continue;
		}
		const float RowY = 408.0f + 72.0f * static_cast<float>(Index - First);
		const bool HasLock = HubsRunBetween(L, Lock, RowY, RowY + 66.0f).first >= 0.0f;
		const bool HasU31 = HubsRunBetween(L, U31, RowY, RowY + 66.0f).first >= 0.0f;
		const bool HasButton = HubsRunsIn(L, 24.0f + 1000.0f - 372.0f, RowY, 24.0f + 1000.0f, RowY + 66.0f) > 0;
		ui_test::Expect(HasLock && HasU31 && HasButton, (std::string("FestLockedMain: ") + Id + "'s row keeps its button with the Main's lock line and U31 beside it").c_str());
		++Ladder;
	}
	ui_test::Expect(Ladder == 5, "FestLockedMain: the five ladder rows are in view once the rows are scrolled to their end");
}

/** FestLockedMain's wallet half: a won seat into the locked Main (a rcop-main ticket in the hero's wallet) survives a save and a
 *  reload (§2.2.3: the seat waits until big fields open). */
void FestLockedMainWallet()
{
	struct HubsSaveHooks : ui_test::QuietHooks
	{
		std::string Last;
		void Save(const ss::SaveData& Data) override { Last = Data.Serialize(); }
	};
	const std::string Ticket = ss::net::Shared().TicketOf("rcop-main");
	HubsSaveHooks H;
	std::string Saved;
	{
		ss::Session S(H, "u4-fest-locked-wallet");
		S.Life.Tickets[Ticket] += 1;
		S.Save();
		Saved = H.Last;
	}
	ss::SaveData Data;
	const bool Parsed = !Saved.empty() && ss::SaveData::Parse(Saved, Data);
	ui_test::Expect(Parsed, "FestLockedMain: the session with a rcop-main ticket saves and parses");
	if (Parsed)
	{
		HubsSaveHooks H2;
		ss::Session Back(H2, "u4-fest-locked-wallet", &Data);
		ui_test::Expect(Back.Life.TicketsFor(Ticket) == 1, ("FestLockedMain: the rcop-main ticket (" + Ticket + ") is in the wallet after the reload").c_str());
	}
}

/** The clock forward to Day at Hour (world time), the hub's laptop fed and rested. */
void HubsTo(HubsLaptop& P, int Day, double Hour)
{
	P.At(static_cast<double>(Day) * ss::net::MinutesPerDay + Hour * 60.0);
}

/** A host's SERIES page with no series picked: its current or next festival (§2.8.1), the pointer away. */
void HubsOpenHost(HubsLaptop& P, ss::net::SiteId Site)
{
	P.RL.SiteShown = Site;
	P.RL.ShowSeries(std::string(), P.Now);
	P.RL.OpenPage(RiverLine::Page::Series, P.Now);
	P.Now = ui_test::Step(P.S, P.Now, 1.5);
	P.Away();
}

/** A rival account through its letter (the invitation, then Accept). */
bool HubsAccept(HubsLaptop& P, ss::net::SiteId Site)
{
	const std::string Key = ss::net::KeyOf(Site);
	P.S.DebugSite("invite " + Key);
	const std::string Why = P.S.AnswerInvite(Site, true);
	const bool Ok = ss::ui::rlsites_detail::SitesAccount(P.S, Site) != nullptr;
	if (!Ok)
	{
		std::printf("  %s: no account (%s)\n", Key.c_str(), Why.c_str());
	}
	return Ok;
}

/** FH-4A: the right column's tab I on RiverLine's hub (fstab0..2 in {1044, 348, 532, 36}), clicked; the settled frame. */
DrawList HubsTab(HubsLaptop& P, int I)
{
	const float W = (532.0f - 12.0f) / 3.0f;
	return P.Click(1044.0f + static_cast<float>(I) * (W + 6.0f) + W / 2.0f, 366.0f);
}

/** FH-4A: the first run exactly Text whose box starts at X0 or right of it: its baseline (-1 when none). */
float HubsBaseRight(const DrawList& L, const std::string& Text, float X0)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text && Cmd.Text.X >= X0)
		{
			return Cmd.Text.BaselineY;
		}
	}
	return -1.0f;
}

/** FH-4A: a running event of edition F (a numbered event, headliner or Main; Plain: a RiverLine event of no festival instead) that
 *  started today, still in play: its id ("" for none). */
std::string HubsRunning(HubsLaptop& P, const ss::fest::Festival* F, bool Plain)
{
	const ss::net::Network& Net = ss::net::Site(ss::net::SiteId::RiverLine);
	const double World = P.S.WorldMinutes();
	for (const ss::net::EventInstance& E : Net.Window(static_cast<double>(ss::net::DayOf(World)) * ss::net::MinutesPerDay, World))
	{
		const ss::net::EventTemplate& T = Net.TemplateOf(E);
		const ss::net::Status St = Net.Live(E, World).St;
		const bool InPlay = St == ss::net::Status::LateReg || St == ss::net::Status::Running || St == ss::net::Status::FinalTable;
		const bool Ours = Plain ? ss::fest::FestivalOf(T.Series) == nullptr : (F && T.Series == F->SeriesId && T.Fmt != ss::net::Format::Satellite);
		if (InPlay && Ours && T.BuyInCents > 0)
		{
			return E.Id;
		}
	}
	return std::string();
}

/** FH-4A: the hero plays Id to Place for Prize through the world's public calls (World::HeroEntered, HeroFinished; PLAN_FHUBS
 *  Corrections item 15), the world drained first as the session drains it before telling a finish. */
void HubsHeroFinish(HubsLaptop& P, const std::string& Id, int Place, ss::Chips Prize)
{
	P.S.DrainWorld();
	ss::world::World& W = P.S.Living();
	W.HeroEntered(Id, std::string());
	W.HeroFinished(Id, Place, Prize, ss::world::World::TableReport());
	P.Now = ui_test::Step(P.S, P.Now, 0.2);
}

/** U10's line as FestYourStrip expects it from fest::YourLine's numbers (written here, apart from the hub's HubYourCounts). */
std::string HubsYourWant(const ss::fest::YourLineView& V)
{
	std::string Want = (V.Events == 1 ? std::string("1 event") : ss::Grouped(V.Events) + " events") + " \xC2\xB7 " + (V.Cashes == 1 ? std::string("1 cash") : ss::Grouped(V.Cashes) + " cashes");
	if (V.Best > 0 && V.Field > 0)
	{
		Want += " \xC2\xB7 best " + ss::Ordinal(V.Best) + " of " + ss::Grouped(V.Field);
	}
	if (V.Rank > 0)
	{
		Want += " \xC2\xB7 race " + ss::Ordinal(V.Rank) + " of " + ss::Grouped(V.Scorers);
	}
	return Want;
}

/** FestYourStrip (§2.8.2, §2.15.2; F-hubs' hub half, FH-4A) and fest_hub_your_ladder, on RCOP 2026's first day: before the hero plays,
 *  YOUR RCOP invites (U11) and prints no U10 line; after one cash, the panel prints fest::YourLine's numbers in U10's singular ("1 event
 *  · 1 cash · best ... · race ..."), the seven rungs with PLAY 1 and CASH 1 reached and U27's "{N} pts to {MARK} · right now" for the
 *  next one from fest::MarkCutoff and the hero's points, the YOU row (U7) on THE RACE and the result row; a plain series result adds
 *  nothing. Plant: the U10 line drawn before the hero plays. */
void FestYourStripHub(HubsLaptop& P, const ss::fest::Festival& Rcop)
{
	const DrawList Before = HubsTab(P, 2);
	ui_test::Expect(ui_test::DrewRun(Before, "Play any RCOP 2026 event for your pin.") && !ui_test::Drew(Before, "0 events") && !ui_test::Drew(Before, "1 event"),
		"FestYourStrip: before the hero plays an RCOP 2026 event, YOUR RCOP invites (U11) and prints no U10 line");
	const std::string Id = HubsRunning(P, &Rcop, false);
	ui_test::Expect(!Id.empty(), "FestYourStrip: an RCOP 2026 event is in play on its first day");
	if (Id.empty())
	{
		return;
	}
	const ss::net::Network& Net = ss::net::Site(ss::net::SiteId::RiverLine);
	const ss::net::EventInstance E = HubsInstance(Net, Id);
	HubsHeroFinish(P, Id, 3, std::max<ss::Chips>(100, Net.TemplateOf(E).BuyInCents * 6));
	// The page late on the first day, once the day's first events have finished and the race holds the regulars' points too.
	HubsTo(P, Rcop.FirstDay, 23.5);
	P.OpenHub();
	HubsTab(P, 2);
	const ss::world::World& W = P.S.Living();
	const ss::fest::YourLineView V = ss::fest::YourLine(Rcop, &W);
	const std::pair<int, int> Me = ss::world::FestStanding(W, -1);
	ui_test::Expect(V.Shown && V.Events == 1 && V.Cashes == 1 && Me.first > 0, ("FestYourStrip: the hero's cash in " + Id + " gives RCOP 2026's pin and a place in the race").c_str());
	const DrawList L = HubsPage("fest_hub_your_ladder", P);
	const std::string Want = HubsYourWant(V);
	ui_test::Expect(ui_test::DrewRun(L, Want), ("FestYourStrip: YOUR RCOP prints fest::YourLine's numbers in U10's singular (\"" + Want + "\")").c_str());
	ui_test::Expect(Want == ss::ui::rlfest_detail::HubYourCounts(V, true), "FestYourStrip: the hub's U10 line is the expected one");
	for (const char* Rung : {"PLAY 1", "CASH 1", "TOP 25%", "TOP 10%", "FINALIST", "PODIUM", "THE BELT"})
	{
		ui_test::Expect(HubsBaseRight(L, Rung, 1044.0f) > 392.0f, (std::string("YOUR RCOP: the ladder's rung ") + Rung + " is drawn").c_str());
	}
	// U27: the next mark the hero doesn't hold right now, its cut-off from the live race.
	const std::vector<std::pair<int, ss::world::FestScore>> Race = ss::world::FestTop(W, Me.first);
	const double Mine = Race.empty() ? 0.0 : Race.back().second.Points;
	const ss::world::FestMark Marks[5] = {ss::world::FestMark::Top25, ss::world::FestMark::Top10, ss::world::FestMark::Finalist, ss::world::FestMark::Podium, ss::world::FestMark::Title};
	const char* const Names[5] = {"TOP 25%", "TOP 10%", "FINALIST", "PODIUM", "THE BELT"};
	std::string Line;
	const ss::world::FestMark Held = ss::world::MarkFor(Me.first, Me.second);
	for (int K = 0; K < 5; ++K)
	{
		if (Held == ss::world::FestMark::None || Held < Marks[K])
		{
			const long long N = std::llround(ss::fest::MarkCutoff(W, Marks[K]) - Mine) + 1;
			Line = N > 0 ? ss::Grouped(N) + " pts to " + Names[K] + " \xC2\xB7 right now" : std::string();
			break;
		}
	}
	std::printf("  fest_hub_your_ladder: the hero %s of %d in the race with %.0f pts; U27: %s\n", ss::Ordinal(Me.first).c_str(), Me.second, Mine, Line.empty() ? "(none)" : Line.c_str());
	ui_test::Expect(Line.empty() || ui_test::DrewRun(L, Line), ("YOUR RCOP: U27 reads \"" + Line + "\" (fest::MarkCutoff - the hero's points + 1)").c_str());
	const std::string TName = ss::ui::rlnet_detail::NetShortName(Net.TemplateOf(E).Name);
	ui_test::Expect(ui_test::Drew(L, TName) && ui_test::Drew(L, "3rd"), ("YOUR RCOP: the result row names " + TName + " and the place").c_str());
	// THE RACE: the YOU row (U7).
	const DrawList RaceTab = HubsTab(P, 0);
	const std::string You = "You \xC2\xB7 " + ss::Ordinal(Me.first) + " of " + ss::Grouped(Me.second) + " in the race \xC2\xB7 " + ss::Grouped(static_cast<int64_t>(std::llround(Mine))) + " pts";
	ui_test::Expect(ui_test::DrewRun(RaceTab, You) && HubsBaseRight(RaceTab, You, 1044.0f) > 856.0f, ("THE RACE: the YOU row at the body's foot reads \"" + You + "\" (U7)").c_str());
	ui_test::Expect(ss::ui::rlfest_detail::HubRaceMoves(P.RL) == 0, "THE RACE: no movement before the hub has seen a day turn (never a guess)");
	// PEOPLE YOU KNOW (fsrace1): world::FestKnown's people in race order with their rank in the whole race, else "No points yet.".
	const std::pair<float, float> Known = HubsRunAt(RaceTab, "PEOPLE YOU KNOW");
	const DrawList KnownTab = P.Click(Known.first + 20.0f, Known.second - 4.0f);
	const std::vector<std::pair<int, int>> People = ss::world::FestKnown(W);
	const ss::world::Npc* First = People.empty() ? nullptr : W.Get(People.front().first);
	ui_test::Expect(Known.first > 1044.0f && (First ? ui_test::Drew(KnownTab, First->Name) && ui_test::DrewRun(KnownTab, ss::Grouped(People.front().second))
												 : ui_test::DrewRun(KnownTab, "No points yet.")),
		("THE RACE: PEOPLE YOU KNOW lists world::FestKnown in race order (" + std::to_string(People.size()) + " people)").c_str());
	// None on a plain series event: a plain RiverLine result leaves the panel's numbers as they were.
	const std::string Plain = HubsRunning(P, nullptr, true);
	if (!Plain.empty())
	{
		HubsHeroFinish(P, Plain, 2, 500);
		const ss::fest::YourLineView After = ss::fest::YourLine(Rcop, &W);
		const DrawList Again = HubsTab(P, 2);
		ui_test::Expect(After.Events == 1 && ui_test::Drew(Again, "1 event \xC2\xB7 1 cash"), ("FestYourStrip: a plain series result (" + Plain + ") adds nothing to YOUR RCOP").c_str());
	}
	else
	{
		std::printf("  FestYourStrip: no plain RiverLine event in play on day %d (the plain case skipped)\n", ss::net::DayOf(P.S.WorldMinutes()));
	}
}

/** FestRollTitles (§2.6.2, §2.8.2; FH-4A), day 35: RCOP 2026's roll edition shows MAIN EVENT, MINI MAIN EVENT, MICRO MAIN EVENT and
 *  PLAYER OF THE SERIES in that order inside its 120 units, with the rcop-mini and rcop-micro winners (world::FestRoll's Honors: the
 *  names and "{Prize} · 1st of {Entries}" in NetMoney). Plant: a championship's edition at 88. */
void FestRollTitlesDay35(HubsLaptop& P, const ss::fest::Festival& Rcop)
{
	P.RL.SiteShown = ss::net::SiteId::RiverLine;
	P.RL.ShowSeries("rcop", P.Now);
	P.RL.OpenPage(RiverLine::Page::Series, P.Now);
	P.Now = ui_test::Step(P.S, P.Now, 1.5);
	const DrawList L = HubsTab(P, 1);
	const ss::world::World& W = P.S.Living();
	const ss::world::FestRollRow* Row = nullptr;
	const std::vector<ss::world::FestRollRow> Roll = ss::world::FestRoll(W, ss::fest::Brand::Rcop);
	for (const ss::world::FestRollRow& R : Roll)
	{
		Row = R.Key == Rcop.SeriesId ? &R : Row;
	}
	ui_test::Expect(Row && Row->Mini && Row->Micro, "FestRollTitles: RCOP 2026's roll row holds its Mini and Micro winners on day 35");
	if (!Row || !Row->Mini || !Row->Micro)
	{
		return;
	}
	const float Title = HubsBaseRight(L, "RCOP 2026", 1044.0f);
	const float Main = HubsBaseRight(L, "MAIN EVENT", 1044.0f);
	const float Mini = HubsBaseRight(L, "MINI MAIN EVENT", 1044.0f);
	const float Micro = HubsBaseRight(L, "MICRO MAIN EVENT", 1044.0f);
	const float Pos = Row->Crown ? HubsBaseRight(L, "PLAYER OF THE SERIES", 1044.0f) : Micro + 24.0f;
	ui_test::Expect(Title > 392.0f && Title < Main && Main < Mini && Mini < Micro && Micro < Pos && Pos <= Title - 22.0f + ss::ui::rlfest_detail::HubRollPitch(true),
		("FestRollTitles: RCOP 2026's four lines in order inside 120 units (title " + std::to_string(Title) + ", last line " + std::to_string(Pos) + ")").c_str());
	for (const ss::world::Honor* Hn : {Row->Mini, Row->Micro})
	{
		const std::string Fig = ss::ui::rlnet_detail::NetMoney(Hn->Prize) + " \xC2\xB7 1st of " + ss::Grouped(Hn->Entries);
		ui_test::Expect(ui_test::Drew(L, Hn->Name) && ui_test::DrewRun(L, Fig), ("FestRollTitles: " + Hn->EventId + "'s winner " + Hn->Name + " and \"" + Fig + "\" on the roll").c_str());
	}
}

/** FestRollTitles' pitch and fest_hub_champ_roll, day 400: RCOP 2027's hub (CHAMPIONS) holds RCOP 2027 and RCOP 2026, newest first; the
 *  drawn pitch between their titles is 120 and four of it fit 558 [554] and three Ignite's 478. */
void FestRollTitlesDay400(HubsLaptop& P)
{
	P.RL.SiteShown = ss::net::SiteId::RiverLine;
	P.RL.ShowSeries("rcop27", P.Now);
	P.RL.OpenPage(RiverLine::Page::Series, P.Now);
	P.Now = ui_test::Step(P.S, P.Now, 1.5);
	HubsTab(P, 1);
	const DrawList L = HubsPage("fest_hub_champ_roll", P);
	const float New = HubsBaseRight(L, "RCOP 2027", 1044.0f);
	const float Old = HubsBaseRight(L, "RCOP 2026", 1044.0f);
	const float Pitch = Old - New;
	ui_test::Expect(New > 392.0f && Old > New, "fest_hub_champ_roll: RCOP 2027 above RCOP 2026 on one roll (newest first)");
	ui_test::Expect(Pitch == 120.0f && 4.0f * Pitch <= 554.0f && 3.0f * Pitch <= 478.0f,
		("FestRollTitles: a championship edition's drawn pitch is 120, four fit 558 [554] and three Ignite's 478 (" + std::to_string(Pitch) + ")").c_str());
}

/** Career A (FH-2; PLAN_FHUBS FH-2's two careers): FEIGN 2026 on Ignite, day 56, an Ignite account accepted: fest_hub_ig_live. The
 *  hub in Ignite's frame (L7): the strip and banner in RiverLine's rects, the rows cut at y 870 (no row text below it; six whole and a
 *  seventh under the fade), no overlay ticker, THE RACE preselected while Live. */
bool HubsMainOf(const ss::fest::Festival& F, ss::net::EventInstance& Out);
int FestMainDayCommon(const char* Page, const DrawList& L, HubsLaptop& P, const ss::fest::Festival& F, const ss::net::EventInstance& Main, float WordX, float BandY, int Max);
void FestTitleRowsPinned(HubsLaptop& P, const ss::fest::Festival& F, ss::net::SiteId Host);
// FH-6a (defined below SegFest's other checks).
struct HubsMoment;
void FestMomentsFeign(HubsLaptop& P, const ss::fest::Festival& Feign);
void FestTitleMomentHubs(HubsLaptop& P, const ss::fest::Festival& F, const char* Brand, const char* Page);
void FestPinsRevealAfter(HubsLaptop& P, const ss::fest::Festival& Rcop);
void FestMainEditionTwo(HubsLaptop& P);
int HubsSponsorScan(const DrawList& L, const char* What);

/** FH-7, FestHubBudgets: F's hub on its host in phase Want, drawn on a career's way (no page): Buildup three days before its first day
 *  at noon; Closing a minute after its last day ends (the crown waits for the last finish); the Afterglow from its last day + 1 at
 *  06:00, six hours a step until the crown has come. The hub opens on F itself (ShowSeries), as the host's own SERIES page would. */
void HubsBudgetState(HubsLaptop& P, const ss::fest::Festival& F, ss::fest::Phase Want, const std::string& Tag)
{
	const double Day = ss::net::MinutesPerDay;
	const double At = Want == ss::fest::Phase::Buildup ? static_cast<double>(F.FirstDay - 3) * Day + 12.0 * 60.0
		: Want == ss::fest::Phase::Closing             ? static_cast<double>(F.LastDay + 1) * Day + 1.0
													   : static_cast<double>(F.LastDay + 1) * Day + 6.0 * 60.0;
	P.At(std::max(At, P.S.WorldMinutes()));
	for (int K = 0; Want == ss::fest::Phase::Afterglow && K < 12 && ss::fest::PhaseOf(F, P.S.WorldMinutes(), &P.S.Living()) == ss::fest::Phase::Closing; ++K)
	{
		P.At(P.S.WorldMinutes() + 6.0 * 60.0);
	}
	P.RL.SiteShown = F.Site;
	P.RL.ShowSeries(F.SeriesId, P.Now);
	P.RL.OpenPage(RiverLine::Page::Series, P.Now);
	P.Now = ui_test::Step(P.S, P.Now, 1.5);
	P.Away();
	const ss::fest::Phase Got = ss::fest::PhaseOf(F, P.S.WorldMinutes(), &P.S.Living());
	ui_test::Expect(Got == Want, ("FestHubBudgets: " + Tag + " is drawn in its phase (" + std::to_string(static_cast<int>(Got)) + ", want " +
		std::to_string(static_cast<int>(Want)) + ") on day " + std::to_string(ss::net::DayOf(P.S.WorldMinutes()))).c_str());
	if (const std::optional<ss::FestMoment> M = P.S.PeekFestMoment())
	{
		std::printf("  %s: a moment is up over the hub (kind %d, %s)\n", Tag.c_str(), M->Kind, M->Key.c_str());
	}
	HubsDrawn(Tag, P);
}

// ------------------------------------------------------------------ G1 polish (key g1p-hub): the hub's small fixes after G1's PIE

/** g1p-hub: a text run's box in canvas units (the canvas keeps its 1600 x 1000 units at every pixel scale; the scale changes how finely
 *  it tessellates and the size the host measures at): the measured width, the ascent above the baseline and a quarter of its size below. */
struct HubsBox
{
	float X0 = 0.0f, Y0 = 0.0f, X1 = 0.0f, Y1 = 0.0f;
};

HubsBox HubsRunBox(const ss::ui::DrawCmd& Cmd)
{
	ui_test::TableMeasurer M;
	const float W = M.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx);
	return {Cmd.Text.X, Cmd.Text.Y, Cmd.Text.X + W, Cmd.Text.BaselineY + 0.25f * Cmd.Text.SizePx};
}

/** g1p-hub: Network::Joinable's lock lines as the three sites write them for a career with nothing unlocked (Unlocks 0), read off the
 *  first templates of each site with a run after day 13 (the schedules' own events: bounties, satellites, six-max, Omaha, Flip & Run,
 *  big fields). */
std::vector<std::string> HubsLockLines()
{
	static std::vector<std::string> Read;
	if (!Read.empty())
	{
		return Read;
	}
	std::set<std::string> Seen;
	for (const ss::net::SiteId Site : {ss::net::SiteId::RiverLine, ss::net::SiteId::Ignite, ss::net::SiteId::RunItTwice})
	{
		const ss::net::Network& Net = ss::net::Site(Site);
		int Tried = 0;
		for (const ss::net::EventTemplate& T : Net.Templates())
		{
			ss::net::EventInstance E;
			if (Tried >= 300 || !Net.Next(T.Id, 13.0 * ss::net::MinutesPerDay, E))
			{
				continue;
			}
			++Tried;
			std::string Lock;
			if (!Net.Joinable(E, &Lock, 0) && !Lock.empty())
			{
				Seen.insert(Lock);
			}
		}
	}
	Read.assign(Seen.begin(), Seen.end());
	return Read;
}

/** FestLockFits (g1p-hub item 1; PIE s03_hub_race, s08_grit_hub: "Cash in any tournament to unlock bounty even…"): every lock line set
 *  out by rlfest_detail::HubLockLines for a row's status cell (360 wide; a GRIT row's 300) at both UI scales fits the cell less its
 *  margins (10 a side) whole, every word kept, at 9 or more; the 360 cell holds each on one line (11.5 or more, the row's look). Plant:
 *  the set-out returning RegisterBlock's one line at 14.4 (the old cell) fails on the bounty, big-field and Omaha lines. */
void FestLockFits(const std::vector<std::string>& Locks)
{
	namespace rf = ss::ui::rlfest_detail;
	bool Bounty = false, Sat = false, Six = false;
	for (const std::string& Lk : Locks)
	{
		Bounty = Bounty || Lk.find("bounty") != std::string::npos;
		Sat = Sat || Lk.find("satellites") != std::string::npos;
		Six = Six || Lk.find("six-max") != std::string::npos;
	}
	ui_test::Expect(Bounty && Sat && Six, ("FestLockFits: the sites' lock lines are read off their templates (" + std::to_string(Locks.size()) + " lines, the bounty, satellite and six-max ones among them)").c_str());
	int Bad = 0;
	std::string Worst;
	for (const float Scale : {1.0f, 1.5f})
	{
		ui_test::TableMeasurer M;
		DrawList L;
		ss::ui::Canvas C(L, M, RiverLine::Width, RiverLine::Height, Scale);
		for (const std::string& Lock : Locks)
		{
			for (const float W : {360.0f, 300.0f})
			{
				std::vector<std::string> Lines;
				const float Size = rf::HubLockLines(C, Lock, W, Lines);
				bool Fits = Size >= 9.0f && !Lines.empty() && Lines.size() <= 2 && (W < 360.0f || (Lines.size() == 1 && Size >= 11.5f));
				std::string Joined;
				float Widest = 0.0f;
				for (const std::string& Ln : Lines)
				{
					Widest = std::max(Widest, C.Measure(Ln, Size, 600, true));
					Joined += (Joined.empty() ? "" : " ") + Ln;
				}
				Fits = Fits && Widest <= W - 20.0f && Joined == Lock;
				if (!Fits)
				{
					++Bad;
					char Buf[200];
					std::snprintf(Buf, sizeof(Buf), " \"%s\" in %.0f at x%.1f: %zu line(s) at %.2f, %.1f wide;", Lock.c_str(), static_cast<double>(W), static_cast<double>(Scale), Lines.size(),
						static_cast<double>(Size), static_cast<double>(Widest));
					Worst += Buf;
				}
			}
		}
	}
	ui_test::Expect(Bad == 0, ("FestLockFits: every lock line fits a status cell (360 on one line, a GRIT row's 300) at both scales" + (Worst.empty() ? std::string() : ":" + Worst)).c_str());
}

/** FestLockFits' drawn half (g1p-hub item 1): on a hub frame at Scale, every run that is a lock line or a part of one standing centred in
 *  a row's status cell (Rows: the host's rows rect; a single row's cell is 360 wide, a GRIT row's 300, both ending 12 inside the rows'
 *  right edge) lies inside that cell less 4 a side. Returns the runs seen (Bad counts the ones that spill). */
int HubsLockCells(const DrawList& L, float Scale, const ss::ui::Rect& Rows, const std::vector<std::string>& Locks, int& Bad, std::string& Where)
{
	int Seen = 0;
	const float Right = Rows.X + Rows.W - 12.0f;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || Cmd.Text.Text.size() < 8)
		{
			continue;
		}
		bool IsLock = false;
		for (const std::string& Lk : Locks)
		{
			IsLock = IsLock || Lk.find(Cmd.Text.Text) != std::string::npos;
		}
		const HubsBox B = HubsRunBox(Cmd);
		const float Mid = (B.X0 + B.X1) / 2.0f;
		const float Cell = std::fabs(Mid - (Right - 180.0f)) < 2.0f ? 360.0f : std::fabs(Mid - (Right - 150.0f)) < 2.0f ? 300.0f : 0.0f;
		if (!IsLock || Cell <= 0.0f || B.Y0 < Rows.Y || B.Y1 > Rows.Y + Rows.H + 20.0f)
		{
			continue;
		}
		++Seen;
		if (B.X0 < Right - Cell + 4.0f || B.X1 > Right - 4.0f)
		{
			++Bad;
			char Buf[200];
			std::snprintf(Buf, sizeof(Buf), " \"%s\" %.1f-%.1f in a %.0f cell at x%.1f;", Cmd.Text.Text.c_str(), static_cast<double>(B.X0), static_cast<double>(B.X1), static_cast<double>(Cell),
				static_cast<double>(Scale));
			Where += Buf;
		}
	}
	return Seen;
}

/** FestLockFits on a hub page P just drew (One, its 1.0 list) and the same frame at 1.5. Want: the page must show a lock line. */
void FestLockFitsPage(HubsLaptop& P, const DrawList& One, const std::string& Page, const ss::ui::Rect& Rows, const std::vector<std::string>& Locks, bool Want)
{
	int Bad = 0;
	std::string Where;
	const int Seen = HubsLockCells(One, 1.0f, Rows, Locks, Bad, Where) + HubsLockCells(P.Frame(1.5f), 1.5f, Rows, Locks, Bad, Where);
	std::printf("  FestLockFits: %s shows %d lock-line run(s) in its status cells at the two scales\n", Page.c_str(), Seen);
	ui_test::Expect((!Want || Seen > 0) && Bad == 0, ("FestLockFits: " + Page + "'s lock lines stand inside their status cells at both scales" + (Where.empty() ? std::string() : ":" + Where)).c_str());
}

/** FestBannerStats (g1p-hub item 2; PIE s08_writ_hub, s08_grit_hub: "Jan 22 – Jan …", "May 12 – Ma…"): for every edition of the 2026-2028
 *  calendars on its host, at both UI scales, rlfest_detail::HubStatLayout sets the banner's four stats so every value (24/900) fits its
 *  room whole (the dates beside the progress field, the progress field before the Main card) and no column's text reaches the next
 *  one's, for each state the fourth can show: the countdown, every DAY or NIGHT {N} OF {M}, MAIN EVENT TODAY and COMPLETE. Plant: the
 *  columns kept StatGap apart (the old layout) fails on RunItTwice's dates. */
void FestBannerStats()
{
	namespace rf = ss::ui::rlfest_detail;
	namespace rn = ss::ui::rlnet_detail;
	int Editions = 0, Sets = 0, Bad = 0;
	std::string Worst;
	for (const float Scale : {1.0f, 1.5f})
	{
		ui_test::TableMeasurer M;
		DrawList L;
		ss::ui::Canvas C(L, M, RiverLine::Width, RiverLine::Height, Scale);
		for (const ss::fest::Festival& F : ss::fest::All())
		{
			const ss::net::SeriesInfo* Sr = F.Year >= 2026 && F.Year <= 2028 ? ss::net::Site(F.Site).FindSeries(F.SeriesId) : nullptr;
			if (!Sr)
			{
				continue;
			}
			Editions += Scale == 1.0f ? 1 : 0;
			const int Days = F.LastDay - F.FirstDay + 1;
			std::vector<std::pair<std::string, std::string>> States = {{"99D 23H 59M", "OPENS IN"}, {"MAIN EVENT TODAY", "PROGRESS"}, {"COMPLETE", "PROGRESS"}};
			for (int N = 1; N <= Days; ++N)
			{
				States.push_back({std::string(F.Which == ss::fest::Brand::Grit ? "NIGHT " : "DAY ") + std::to_string(N) + " OF " + std::to_string(Days), "PROGRESS"});
			}
			const std::string Dates = ss::net::DateLabel(F.FirstDay) + " \xE2\x80\x93 " + ss::net::DateLabel(F.LastDay);
			for (const std::pair<std::string, std::string>& St : States)
			{
				const std::pair<std::string, std::string> Stats[4] = {{rn::NetMoney(Sr->GtdCents), "GUARANTEED"}, {std::to_string(Sr->Events), "EVENTS"}, {Dates, "DATES"}, St};
				float X[4];
				float Room[4];
				rf::HubStatLayout(C, F.Site, Stats, X, Room);
				++Sets;
				for (int I = 0; I < 4; ++I)
				{
					const float Vw = C.Measure(Stats[I].first, 24.0f, 900);
					const float Prev = I == 0 ? 0.0f : std::max(C.Measure(Stats[I - 1].first, 24.0f, 900), C.Measure(Stats[I - 1].second, 11.0f, 800));
					if (Vw > Room[I] || (I > 0 && X[I] < X[I - 1] + Prev + 12.0f))
					{
						++Bad;
						if (Worst.size() < 400)
						{
							char Buf[200];
							std::snprintf(Buf, sizeof(Buf), " %s \"%s\" %.1f in %.1f at x%.1f;", F.Name.c_str(), Stats[I].first.c_str(), static_cast<double>(Vw), static_cast<double>(Room[I]),
								static_cast<double>(Scale));
							Worst += Buf;
						}
					}
				}
			}
		}
	}
	std::printf("  FestBannerStats: %d editions of 2026-2028, %d stat sets at the two scales\n", Editions, Sets);
	ui_test::Expect(Editions >= 20 && Bad == 0, ("FestBannerStats: every edition's banner stats stand whole on its host at both scales, the dates beside the progress field" +
												 (Worst.empty() ? std::string() : ":" + Worst)).c_str());
}

/** A NetSpaced line's width (the glyphs' advances and the tracking between them), as the Main card's MAIN EVENT line draws it. */
float HubsSpacedWidth(const ss::ui::Canvas& C, const std::string& Str, float Size, int Weight, float Track)
{
	float Total = 0.0f;
	int N = 0;
	for (size_t I = 0; I < Str.size();)
	{
		const unsigned char B = static_cast<unsigned char>(Str[I]);
		const size_t Len = B >= 0xF0 ? 4 : B >= 0xE0 ? 3 : B >= 0xC0 ? 2 : 1;
		Total += C.Measure(Str.substr(I, Len), Size, Weight) + Track;
		I += Len;
		++N;
	}
	return N > 0 ? Total - Track : 0.0f;
}

/** FestMainCardFits (g1p-hub item 3; PIE s04_mainday_hub: "$25,000,000 GUARANT…"): for every edition's Main of the 2026-2028 calendars
 *  with a guarantee, on its host, at both UI scales, "{Gtd} GUARANTEED" (22/900) fits the room rlfest_detail::HubMainBoxes gives it on
 *  Main Event Day and on every other day, and on Main Event Day the MAIN EVENT line (10.5/800, tracked 1.5) ends 8 before Details, which
 *  stands at its right. Plant: Details beside the guarantee (the old {M.X + M.W - 104, M.Y + 36}, the guarantee's room M.W - 132). */
void FestMainCardFits()
{
	namespace rf = ss::ui::rlfest_detail;
	namespace rn = ss::ui::rlnet_detail;
	int Mains = 0, Bad = 0;
	std::string Worst;
	for (const float Scale : {1.0f, 1.5f})
	{
		ui_test::TableMeasurer M;
		DrawList L;
		ss::ui::Canvas C(L, M, RiverLine::Width, RiverLine::Height, Scale);
		for (const ss::fest::Festival& F : ss::fest::All())
		{
			const ss::net::Network& Net = ss::net::Site(F.Site);
			ss::net::EventInstance Main;
			if (F.Year < 2026 || F.Year > 2028 || F.MainId.empty() || !Net.Next(F.MainId, static_cast<double>(F.FirstDay) * ss::net::MinutesPerDay, Main) || Net.TemplateOf(Main).GtdCents <= 0)
			{
				continue;
			}
			Mains += Scale == 1.0f ? 1 : 0;
			const ss::ui::Rect Card = F.Site == ss::net::SiteId::RunItTwice ? ss::ui::Rect{1170.0f, 160.0f, 390.0f, 164.0f} : ss::ui::Rect{1166.0f, 152.0f, 386.0f, 164.0f};
			const std::string Gtd = rn::NetMoney(Net.TemplateOf(Main).GtdCents) + " GUARANTEED";
			const int Day = ss::net::DayOf(Main.Start);
			const std::string Head = "MAIN EVENT \xC2\xB7 " + rn::NetUpper(ss::net::WeekdayName(Day)) + " " + rn::NetUpper(ss::net::DateLabel(Day)) + " \xC2\xB7 " + ss::net::TimeLabel(Main.Start);
			for (const bool Broadcast : {false, true})
			{
				ss::ui::Rect Details;
				float Room = 0.0f;
				rf::HubMainBoxes(F.Site, Broadcast, Details, Room);
				const float Gw = C.Measure(Gtd, 22.0f, 900);
				const float Hw = HubsSpacedWidth(C, Head, 10.5f, 800, 1.5f);
				const bool Fits = Gw <= Room && Card.X + 18.0f + Gw <= Card.X + Card.W - 18.0f &&
					(!Broadcast || (Card.X + 18.0f + Hw + 8.0f <= Details.X && Details.Y + Details.H <= Card.Y + 58.0f - 22.0f * 0.8f));
				if (!Fits)
				{
					++Bad;
					char Buf[200];
					std::snprintf(Buf, sizeof(Buf), " %s%s: \"%s\" %.1f in %.1f, the MAIN EVENT line %.1f, Details at %.1f (x%.1f);", F.Name.c_str(), Broadcast ? " (Main Event Day)" : "", Gtd.c_str(),
						static_cast<double>(Gw), static_cast<double>(Room), static_cast<double>(Hw), static_cast<double>(Details.X), static_cast<double>(Scale));
					Worst += Buf;
				}
			}
		}
	}
	std::printf("  FestMainCardFits: %d Mains with a guarantee in 2026-2028\n", Mains);
	ui_test::Expect(Mains >= 20 && Bad == 0, ("FestMainCardFits: every Main's guarantee fits its card whole, Details clear of it and of the MAIN EVENT line, at both scales" +
											  (Worst.empty() ? std::string() : ":" + Worst)).c_str());
}

/** FestMainCardFits' drawn half: the Main Event Day page One (and the frame at 1.5) draws Gtd whole and Details' label clear of it. */
void FestMainCardPage(HubsLaptop& P, const DrawList& One, const std::string& Gtd)
{
	int Whole = 0;
	bool Clear = true;
	const DrawList Half = P.Frame(1.5f);
	for (int K = 0; K < 2; ++K)
	{
		const DrawList& L = K == 0 ? One : Half;
		HubsBox G;
		HubsBox D;
		bool HasG = false, HasD = false;
		for (const ss::ui::DrawCmd& Cmd : L.Cmds)
		{
			if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Gtd)
			{
				G = HubsRunBox(Cmd);
				HasG = true;
			}
			if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == "Details" && Cmd.Text.X > 1166.0f)
			{
				D = HubsRunBox(Cmd);
				HasD = true;
			}
		}
		Whole += HasG ? 1 : 0;
		Clear = Clear && HasD && (!HasG || D.X0 > G.X1 + 4.0f || D.Y1 < G.Y0 || D.Y0 > G.Y1);
	}
	ui_test::Expect(Whole == 2 && Clear, ("FestMainCardFits: fest_hub_rl_mainday draws \"" + Gtd + "\" whole and Details clear of it, at both scales").c_str());
}

/** The silver wave line's core vertices: Crest Silver at its 0.5 (FestBannerWave's K = 0 line). */
bool HubsSilver(const ss::ui::Color& C, uint32_t Silver)
{
	const ss::ui::Color W = ss::ui::Hex(Silver, 0.5f);
	return std::fabs(C.R - W.R) < 0.01f && std::fabs(C.G - W.G) < 0.01f && std::fabs(C.B - W.B) < 0.01f && C.A > 0.3f && C.A < 0.6f;
}

/** FestWaveClear (g1p-hub item 5; PIE s03_hub_race, s09b_rcop2027_hub: the line through "The RiverLine Championship of Online Poker…"):
 *  RCOP's waves pass behind the hub's header text at every phase of their motion. Drawn: the RCOP hub at 48 times through a world day
 *  (every half hour of the waves' climb) and six frames through the rise after an open, each at both UI scales: no core vertex of the
 *  silver line lies in the box of any header text run (the banner's runs right of the crest: the word-mark's absent, it is a run of its
 *  own face only where no look draws it). Geometry: with the boxes the hub handed the banner (rlfest_detail::HubWaveClear), every minute
 *  of the day and eleven drops through the rise, festart::FestWaveLines leaves every wave line's pieces out of every box, the waves still
 *  drawn (pieces at every phase), the boxes covering every drawn header run, and without the boxes the lines would cross them. Plant:
 *  the hub drawing festart::FestBanner (no boxes) fails the drawn half. */
void FestWaveClearHubs(HubsLaptop& P, const ss::fest::Festival& Rcop)
{
	namespace rf = ss::ui::rlfest_detail;
	const ss::ui::festart::FestivalLook& Look = ss::ui::festart::LookOf(Rcop.Which);
	const ss::ui::Rect Banner{24.0f, 132.0f, 1552.0f, 204.0f};
	int Frames = 0, Runs = 0, Hits = 0, Uncovered = 0;
	std::string Where;
	auto Check = [&](const std::string& Tag) {
		for (const float Scale : {1.0f, 1.5f})
		{
			const DrawList L = P.Frame(Scale);
			++Frames;
			const std::vector<ss::ui::Rect> Clear = rf::HubWaveClear(P.RL);
			std::vector<HubsBox> Boxes;
			for (const ss::ui::DrawCmd& Cmd : L.Cmds)
			{
				const float Base = Cmd.Text.BaselineY;
				if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.X >= 205.0f && Base > Banner.Y && Base < Banner.Y + Banner.H && !Cmd.Text.Text.empty() && Cmd.Text.Text != " ")
				{
					const HubsBox B = HubsRunBox(Cmd);
					Boxes.push_back(B);
					bool Covered = false;
					for (const ss::ui::Rect& R : Clear)
					{
						Covered = Covered || (B.X0 >= R.X - 0.5f && B.X1 <= R.X + R.W + 0.5f && B.Y0 >= R.Y - 0.5f && B.Y1 <= R.Y + R.H + 0.5f);
					}
					Uncovered += Covered ? 0 : 1;
					if (!Covered && Where.size() < 300)
					{
						Where += " uncovered \"" + Cmd.Text.Text + "\" (" + Tag + ");";
					}
				}
			}
			Runs += static_cast<int>(Boxes.size());
			for (const ss::ui::Vertex& V : L.Vertices)
			{
				if (!HubsSilver(V.Col, Look.Accent2))
				{
					continue;
				}
				const float X = V.X;
				const float Y = V.Y;
				for (const HubsBox& B : Boxes)
				{
					if (X > B.X0 && X < B.X1 && Y > B.Y0 && Y < B.Y1)
					{
						++Hits;
						if (Where.size() < 300)
						{
							char Buf[120];
							std::snprintf(Buf, sizeof(Buf), " a wave vertex at (%.0f, %.0f) %s x%.1f;", static_cast<double>(X), static_cast<double>(Y), Tag.c_str(), static_cast<double>(Scale));
							Where += Buf;
						}
					}
				}
			}
		}
	};
	const double Start = P.S.WorldMinutes();
	for (int K = 0; K < 48; ++K)
	{
		P.At(Start + 30.0 * static_cast<double>(K));
		Check("minute " + std::to_string(static_cast<int>(std::fmod(Start + 30.0 * K, ss::net::MinutesPerDay))));
	}
	// The rise: a new open, then frames 0.3 s apart from its first (the waves drawn live, Drop units lower).
	P.RL.ShowSeries(std::string(), P.Now);
	P.RL.OpenPage(RiverLine::Page::Series, P.Now);
	for (int K = 0; K < 6; ++K)
	{
		Check("the rise at " + std::to_string(K * 3) + "/10 s");
		P.Now += 0.3;
	}
	// The geometry, every minute and through the rise, with the boxes of the last frame.
	const std::vector<ss::ui::Rect> Clear = rf::HubWaveClear(P.RL);
	int Inside = 0, Empty = 0, Crossing = 0;
	std::vector<int> Kinds;
	for (int Minute = 0; Minute <= 1440; Minute += 1)
	{
		for (int D = 0; D <= 10; ++D)
		{
			if (D > 0 && Minute % 60 != 0)
			{
				continue;
			}
			const float Day = static_cast<float>(Minute) / 1440.0f;
			const float Drop = Banner.H * 0.6f * static_cast<float>(D) / 10.0f;
			const std::vector<std::vector<ss::ui::Vec2>> Lines = ss::ui::festart::FestWaveLines(Look, Banner, Day, Drop, Clear, Kinds);
			Empty += Lines.empty() ? 1 : 0;
			for (const std::vector<ss::ui::Vec2>& Ln : Lines)
			{
				for (size_t I = 0; I + 1 < Ln.size(); ++I)
				{
					const ss::ui::Vec2 Mid{(Ln[I].X + Ln[I + 1].X) * 0.5f + Banner.X, (Ln[I].Y + Ln[I + 1].Y) * 0.5f + Banner.Y};
					for (const ss::ui::Rect& R : Clear)
					{
						Inside += Mid.X > R.X + 0.5f && Mid.X < R.X + R.W - 0.5f && Mid.Y > R.Y + 0.5f && Mid.Y < R.Y + R.H - 0.5f ? 1 : 0;
					}
				}
			}
			for (const std::vector<ss::ui::Vec2>& Ln : ss::ui::festart::FestWaveLines(Look, Banner, Day, Drop, {}, Kinds))
			{
				for (const ss::ui::Vec2& Pt : Ln)
				{
					for (const ss::ui::Rect& R : Clear)
					{
						Crossing += Pt.X + Banner.X > R.X && Pt.X + Banner.X < R.X + R.W && Pt.Y + Banner.Y > R.Y && Pt.Y + Banner.Y < R.Y + R.H ? 1 : 0;
					}
				}
			}
		}
	}
	std::printf("  FestWaveClear: %d frames, %d header runs, %zu boxes; the geometry: %d pieces' segments inside a box, %d empty phases, %d crossings without the boxes\n", Frames, Runs,
		Clear.size(), Inside, Empty, Crossing);
	ui_test::Expect(Frames == 108 && Runs > 0 && Hits == 0 && Uncovered == 0, ("FestWaveClear: RCOP's silver wave passes behind every header run of the hub at 48 times of day and through the rise, at both scales" +
																			  (Where.empty() ? std::string() : ":" + Where)).c_str());
	ui_test::Expect(!Clear.empty() && Inside == 0 && Empty == 0 && Crossing > 0, "FestWaveClear: at every minute of the climb and every drop of the rise the wave lines stay out of the header's boxes, and only the boxes keep them out");
}

/** FestRollPillClear (g1p-hub item 4; PIE s09b_rcop2027_hub: DEFENDING CHAMPION over the prize): CHAMPIONS on RCOP 2027's day 395, where
 *  RCOP 2026's Main champion defends, at both UI scales: on every roll line, every run right of the name column ends 6 (a pill's padding)
 *  before the line's figure begins, and the champion's name is drawn. Plant: the name's MaxW without HubNameTail's room. */
void FestRollPillClear(HubsLaptop& P, const std::string& Champion)
{
	P.OpenHub();
	const DrawList One = HubsTab(P, 1);
	const DrawList Half = P.Frame(1.5f);
	int Lines = 0, Bad = 0;
	std::string Where;
	for (int K = 0; K < 2; ++K)
	{
		const DrawList& L = K == 0 ? One : Half;
		std::map<int, std::vector<HubsBox>> ByLine;
		std::map<int, std::vector<std::string>> Texts;
		for (const ss::ui::DrawCmd& Cmd : L.Cmds)
		{
			const float X = Cmd.Text.X;
			const float Base = Cmd.Text.BaselineY;
			if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && X >= 1044.0f + 22.0f + 138.0f && Base > 392.0f && Base < 950.0f)
			{
				ByLine[static_cast<int>(std::lround(Base))].push_back(HubsRunBox(Cmd));
				Texts[static_cast<int>(std::lround(Base))].push_back(Cmd.Text.Text);
			}
		}
		for (const auto& Line : ByLine)
		{
			if (Line.second.size() < 2)
			{
				continue;
			}
			++Lines;
			size_t Fig = 0;
			for (size_t I = 1; I < Line.second.size(); ++I)
			{
				Fig = Line.second[I].X1 > Line.second[Fig].X1 ? I : Fig;
			}
			for (size_t I = 0; I < Line.second.size(); ++I)
			{
				if (I != Fig && Line.second[I].X1 + 6.0f > Line.second[Fig].X0)
				{
					++Bad;
					Where += " \"" + Texts[Line.first][I] + "\" reaches \"" + Texts[Line.first][Fig] + "\" (x" + (K == 0 ? "1.0" : "1.5") + ");";
				}
			}
		}
	}
	ui_test::Expect(Lines >= 4 && Bad == 0 && ui_test::Drew(One, Champion), ("FestRollPillClear: on RCOP 2027's CHAMPIONS no name, glyph or pill reaches a line's prize at either scale (" +
																			 std::to_string(Lines) + " lines" + (Where.empty() ? std::string(")") : "):" + Where)).c_str());
}

/** FestLockedMain in every year (g1p-hub item 14; §2.2.3, §2.16 step 3; PIE s09b: RCOP 2027's Step 1 row had no lock line): on RCOP
 *  2027's day 395, the rows scrolled to their end, every RCOP ladder row in view (step1-4, mega-rcop) shows RCOP 2027's Main lock line
 *  and U31 beside its button. Plant: the seat followed by template (FindTemplate: "rcop-main" names RCOP 2026's Main, past by then). */
void FestLockedMainRows2027(HubsLaptop& P, const ss::fest::Festival& Rcop27)
{
	const std::string Lock = HubsMainLock(P.S, Rcop27);
	const std::string U31 = "A seat won here waits in your wallet until big fields open.";
	P.OpenHub();
	for (int K = 0; K < 12; ++K)
	{
		P.RL.UI.Ptr.Active = true;
		P.RL.UI.Ptr.X = 300.0f;
		P.RL.UI.Ptr.Y = 600.0f;
		P.RL.UI.Ptr.Wheel = 1.0f;
		P.Frame(1.0f);
	}
	P.Away();
	const DrawList L = P.Frame(1.0f);
	const std::vector<std::string> Rows = P.RL.ListedEvents();
	const ss::net::Network& Net = ss::net::Site(Rcop27.Site);
	const int First = std::max(0, static_cast<int>(Rows.size()) - 7);
	int Ladder = 0, Kept = 0;
	for (int I = First; I < static_cast<int>(Rows.size()); ++I)
	{
		const std::string Id = Net.TemplateOf(HubsInstance(Net, Rows[static_cast<size_t>(I)])).Id;
		if (Id != "step1" && Id != "step2" && Id != "step3" && Id != "step4" && Id != "mega-rcop")
		{
			continue;
		}
		++Ladder;
		const float RowY = 408.0f + 72.0f * static_cast<float>(I - First);
		Kept += HubsRunBetween(L, Lock, RowY, RowY + 66.0f).first >= 0.0f && HubsRunBetween(L, U31, RowY, RowY + 66.0f).first >= 0.0f ? 1 : 0;
	}
	ui_test::Expect(!Lock.empty() && Ladder > 0 && Kept == Ladder,
		("FestLockedMain (every year): RCOP 2027's ladder rows keep its Main's lock line and U31 on day 395 (" + std::to_string(Kept) + " of " + std::to_string(Ladder) + " rows, \"" + Lock + "\")").c_str());
}

/** g1p-hub item 13 (§2.16 step 8, N-2; PIE s08_writ_hub: "#5" pills): the runs in the rows rect Rows of L: Hash, the "#{n}" pills;
 *  Coded, the names that start with "{Short} {nn}: " (RunItTwice's code with no "#"). */
void HubsRowCodes(const DrawList& L, const ss::ui::Rect& Rows, const std::string& Short, int& Hash, int& Coded)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		const std::string& T = Cmd.Text.Text;
		const float X = Cmd.Text.X;
		const float Base = Cmd.Text.BaselineY;
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || X < Rows.X || X > Rows.X + Rows.W || Base < Rows.Y || Base > Rows.Y + Rows.H)
		{
			continue;
		}
		Hash += T.size() >= 2 && T[0] == '#' && std::isdigit(static_cast<unsigned char>(T[1])) ? 1 : 0;
		Coded += T.size() > Short.size() + 5 && T.compare(0, Short.size() + 1, Short + " ") == 0 && std::isdigit(static_cast<unsigned char>(T[Short.size() + 1])) &&
				std::isdigit(static_cast<unsigned char>(T[Short.size() + 2])) && T.compare(Short.size() + 3, 2, ": ") == 0
			? 1
			: 0;
	}
}

void FestCareerIgnite()
{
	HubsLaptop P("u4-fest-career-a");
	const ss::fest::Festival* Feign = ss::fest::FestivalOf("ignite.feign26");
	ui_test::Expect(Feign != nullptr && Feign->FirstDay == 56, "fest_hub_ig_live: FEIGN 2026 opens on day 56 (F-net's FN-2a row)");
	if (!Feign)
	{
		return;
	}
	HubsTo(P, 40, 12.0);
	ui_test::Expect(HubsAccept(P, ss::net::SiteId::Ignite), "fest_hub_ig_live: Ignite's invitation accepted");
	// FH-7, FestHubBudgets: FEIGN 2026's Buildup on Ignite (day 53), drawn.
	HubsBudgetState(P, *Feign, ss::fest::Phase::Buildup, "ig_buildup (FEIGN 2026)");
	HubsTo(P, 56, 18.0);
	HubsOpenHost(P, ss::net::SiteId::Ignite);
	const DrawList L = HubsPage("fest_hub_ig_live", P);
	{
		// g1p-hub: FestLockFits on Ignite's rows; Ignite keeps its "#n" pills (item 13 is RunItTwice's alone).
		FestLockFitsPage(P, L, "fest_hub_ig_live", {24.0f, 408.0f, 1000.0f, 462.0f}, HubsLockLines(), false);
		int Hash = 0, Coded = 0;
		HubsRowCodes(L, {24.0f, 408.0f, 1000.0f, 462.0f}, "FEIGN", Hash, Coded);
		ui_test::Expect(Hash > 0, ("g1p-hub: Ignite's FEIGN rows keep their \"#n\" pills (" + std::to_string(Hash) + ")").c_str());
	}
	ui_test::Expect(ui_test::DrewRun(L, "FEIGN") && ui_test::DrewRun(L, "DAY 1 OF 10"), "fest_hub_ig_live: Ignite's SERIES page is FEIGN 2026's hub on its first day (DAY 1 OF 10)");
	ui_test::Expect(ui_test::DrewRun(L, "PLAYER OF THE SERIES"), "fest_hub_ig_live: THE RACE is preselected while Live");
	ui_test::Expect(HubsRunsIn(L, 24.0f, 870.0f, 1024.0f, 884.0f) == 0, "fest_hub_ig_live: no row text below y 870 (the rows end above Ignite's dock)");
	ui_test::Expect(HubsRunsIn(L, 1044.0f, 870.0f, 1576.0f, 884.0f) == 0, "fest_hub_ig_live: the right column's body ends at y 870");
	FestStripTenPage(L, HubsStripRiverLine, P.S.WorldMinutes());
	const std::vector<std::string> Rows = P.RL.ListedEvents();
	std::printf("  fest_hub_ig_live: %zu rows on FEIGN 2026's first day (six whole, the seventh under the fade)\n", Rows.size());
	// FH-5b: FEIGN 2026's Main Event Day (day 65, an hour into its Main): the broadcast in Ignite's frame, WHO'S IN to y 870 (R-33).
	ss::net::EventInstance FeignMain;
	ui_test::Expect(HubsMainOf(*Feign, FeignMain) && ss::net::DayOf(FeignMain.Start) == 65, "fest_hub_ig_mainday: FEIGN 2026's Main runs on day 65");
	P.At(FeignMain.Start + 60.0);
	HubsOpenHost(P, ss::net::SiteId::Ignite);
	ui_test::Expect(ss::fest::PhaseOf(*Feign, P.S.WorldMinutes(), &P.S.Living()) == ss::fest::Phase::MainDay, "fest_hub_ig_mainday: FEIGN 2026 is on its Main day");
	const DrawList Md = HubsPage("fest_hub_ig_mainday", P);
	FestMainDayCommon("fest_hub_ig_mainday", Md, P, *Feign, FeignMain, 210.0f, 132.0f, 8);
	ui_test::Expect(HubsRunsIn(Md, 24.0f, 870.0f, 1024.0f, 884.0f) == 0, "fest_hub_ig_mainday: nothing in the left column below y 870 (WHO'S IN ends above Ignite's dock)");
	// FH-7, FestHubBudgets: FEIGN 2026's Closing (day 66, before the crown) and its Afterglow on Ignite, drawn.
	HubsBudgetState(P, *Feign, ss::fest::Phase::Closing, "ig_closing (FEIGN 2026)");
	HubsBudgetState(P, *Feign, ss::fest::Phase::Afterglow, "ig_afterglow (FEIGN 2026)");
	// PENNANT 2027 (v1.6): its live hub on day 185, then its Micro and Mini rows pinned on their days.
	const ss::fest::Festival* Pennant = ss::fest::FestivalOf("ignite.pennant27");
	ui_test::Expect(Pennant != nullptr, "fest_hub_ig_champ_live: PENNANT 2027 is F-net's row");
	if (!Pennant)
	{
		return;
	}
	HubsTo(P, 185, 12.0);
	HubsOpenHost(P, ss::net::SiteId::Ignite);
	ui_test::Expect(ss::fest::PhaseOf(*Pennant, P.S.WorldMinutes(), &P.S.Living()) == ss::fest::Phase::Live, "fest_hub_ig_champ_live: PENNANT 2027 is Live on day 185");
	const DrawList Pl = HubsPage("fest_hub_ig_champ_live", P);
	ui_test::Expect(ui_test::DrewRun(Pl, "PLAYER OF THE SERIES") &&
			ui_test::DrewRun(Pl, "DAY " + std::to_string(185 - Pennant->FirstDay + 1) + " OF " + std::to_string(Pennant->LastDay - Pennant->FirstDay + 1)),
		"fest_hub_ig_champ_live: PENNANT's hub on Ignite, THE RACE first while Live");
	FestTitleRowsPinned(P, *Pennant, ss::net::SiteId::Ignite);
	// FH-6a, after every page above (the moments' frames and steps move the laptop's clock): PENNANT 2027's Mini title moment
	// (fest_moment_title, FestTitleMoment), then FEIGN 2026's three dev moments over PENNANT's hub (fest_moment_main, _event, _belt).
	FestTitleMomentHubs(P, *Pennant, "pennant", "fest_moment_title");
	FestMomentsFeign(P, *Feign);
}

/** FestCopyUi's drawn U35 half (§2.8.2; FH-4A fix round 1): REIGN 2027's step rows on RunItTwice's hub (rows {232, 412, 860, 504}) on
 *  day 302 (Step 2's first day) and day 303 (Step 1's second, Step 3's first), at noon. Each step row of the edition draws STEP {K} on
 *  its first line and its seat pill on its second, ending 4 short of the entries' column (x 582), and no SATELLITE; K and the seat are read
 *  here from the row's own template (H22's "Step {K} · REIGN Main" and its seats text, "Step {K} tickets (...)" or "REIGN Main Event
 *  seats"), not from the hub's ladder. Adds each form drawn to Seen. Plants: FOR A MAIN EVENT SEAT on Step 1; the STEP {K} pill
 *  dropped; the seat pill kept at 9.5 in RunItTwice's narrower column. */
int FestStepPillsReignDay(HubsLaptop& P, const ss::fest::Festival& Reign, int Day, std::vector<std::string>& Seen)
{
	HubsTo(P, Day, 12.0);
	HubsOpenHost(P, ss::net::SiteId::RunItTwice);
	const DrawList L = P.Frame(1.0f);
	const ss::net::Network& Rit = ss::net::Site(ss::net::SiteId::RunItTwice);
	ui_test::TableMeasurer M;
	DrawList Scratch;
	ss::ui::Canvas C(Scratch, M, RiverLine::Width, RiverLine::Height, 1.0f);
	const std::vector<std::string> Rows = P.RL.ListedEvents();
	int Steps = 0;
	for (size_t I = 0; I < Rows.size() && I < 7; ++I)
	{
		const ss::net::EventTemplate& T = Rit.TemplateOf(HubsInstance(Rit, Rows[I]));
		const bool Own = std::find(Reign.Satellites.begin(), Reign.Satellites.end(), T.Id) != Reign.Satellites.end();
		if (!Own || T.Fmt != ss::net::Format::Satellite || T.Name.size() < 6 || T.Name.compare(0, 5, "Step ") != 0)
		{
			continue;
		}
		const std::string Rung = "STEP " + T.Name.substr(5, 1);
		const std::string SeatPill = T.Seats.compare(0, 5, "Step ") == 0 ? "FOR A STEP " + T.Seats.substr(5, 1) + " TICKET" : std::string("FOR A MAIN EVENT SEAT");
		const float RowY = 412.0f + 72.0f * static_cast<float>(I);
		const bool HasRung = HubsRunBetween(L, Rung, RowY, RowY + 32.0f).first >= 0.0f;
		const ss::ui::TextItem* SeatRun = nullptr;
		bool Sat = false;
		for (const ss::ui::DrawCmd& Cmd : L.Cmds)
		{
			if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || Cmd.Text.BaselineY < RowY || Cmd.Text.BaselineY >= RowY + 66.0f)
			{
				continue;
			}
			SeatRun = Cmd.Text.Text == SeatPill && Cmd.Text.BaselineY >= RowY + 32.0f ? &Cmd.Text : SeatRun;
			Sat = Sat || Cmd.Text.Text == "SATELLITE";
		}
		const float SeatRight = SeatRun ? SeatRun->X + C.Measure(SeatPill, SeatRun->SizePx, SeatRun->Face) + SeatRun->SizePx * 0.65f : -1.0f;
		ui_test::Expect(HasRung && SeatRun && SeatRight <= 578.0f && !Sat,
			("FestCopyUi: U35 on day " + std::to_string(Day) + ": " + T.Name + "'s row draws " + Rung + " and " + SeatPill + " (its pill ends at x " +
				std::to_string(static_cast<int>(SeatRight)) + ", by 578: 4 short of the entries at 582), no SATELLITE")
				.c_str());
		if (HasRung && SeatRun)
		{
			Seen.push_back(Rung + " / " + SeatPill);
			std::printf("  U35 on day %d, row %zu (%s): %s / %s, the seat pill at %.1f ends at x %.1f (by 578, the entries at 582)\n", Day, I, Rows[I].c_str(), Rung.c_str(),
				SeatPill.c_str(), static_cast<double>(SeatRun->SizePx), static_cast<double>(SeatRight));
		}
		++Steps;
	}
	return Steps;
}

void FestStepPillsReign(HubsLaptop& P)
{
	const ss::fest::Festival* Reign = ss::fest::FestivalOf("runittwice.reign27");
	ui_test::Expect(Reign && Reign->FirstDay == 294, "FestCopyUi: REIGN 2027 opens on day 294 (F-net's FN-3 row)");
	if (!Reign)
	{
		return;
	}
	std::vector<std::string> Seen;
	const int On302 = FestStepPillsReignDay(P, *Reign, 302, Seen);
	const int On303 = FestStepPillsReignDay(P, *Reign, 303, Seen);
	std::string List;
	for (const std::string& S : Seen)
	{
		List += (List.empty() ? "" : " | ") + S;
	}
	const auto Has = [&Seen](const char* Form) { return std::find(Seen.begin(), Seen.end(), std::string(Form)) != Seen.end(); };
	ui_test::Expect(On302 == 1 && On303 == 2 && Has("STEP 2 / FOR A STEP 3 TICKET") && Has("STEP 1 / FOR A STEP 2 TICKET") && Has("STEP 3 / FOR A MAIN EVENT SEAT"),
		("FestCopyUi: U35's three rungs drawn on REIGN 2027's days 302 and 303 (" + std::to_string(On302) + " + " + std::to_string(On303) + " step rows: " + List + ")").c_str());
}

/** Career B (FH-2): RiverLine's and RunItTwice's hubs walked forward in day order, never opening Ignite: fest_strip_night_one (Night
 *  One, the pointer resting on FEIGN's cell), fest_hub_rl_live (RCOP 2026's first day), fest_hub_rl_locked (day 19, the rows
 *  scrolled to the ladder), The Summons' seats on WRIT 2027 (U33, no page), fest_hub_rit_twice (GRIT 2027, day 220: two runs a
 *  night), REIGN 2027's step rows (U35, days 302 and 303, no page; FH-4A fix round 1), Summer Slam 2027's locked Main (no page), and fest_popover_pass (FEIGN II's cell on day 395, BY INVITATION until F-world's
 *  FW-7 sets the Pass; FH-7 emits the counting form). */
// ------------------------------------------------------------------ FH-5b: Main Event Day, the reel, the Afterglow, WHO'S IN

/** FH-5b: an edition's Main instance (Network::Next from its first day, as the hub reads it). */
bool HubsMainOf(const ss::fest::Festival& F, ss::net::EventInstance& Out)
{
	return ss::net::Site(F.Site).Next(F.MainId, static_cast<double>(F.FirstDay) * ss::net::MinutesPerDay, Out);
}

/** FH-5b: the hero band's flap row as drawn: the one-glyph runs whose baseline lies in [Y0, Y1] and whose box starts at X0 or right of
 *  it, inside twelve cells, in x order. */
std::string HubsFlapRow(const DrawList& L, float X0, float Y0, float Y1)
{
	std::vector<std::pair<float, std::string>> Cells;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.size() == 1 && Cmd.Text.X >= X0 - 4.0f && Cmd.Text.X < X0 + 48.0f * 12.0f && Cmd.Text.BaselineY >= Y0 &&
			Cmd.Text.BaselineY <= Y1)
		{
			Cells.push_back({Cmd.Text.X, Cmd.Text.Text});
		}
	}
	std::sort(Cells.begin(), Cells.end());
	std::string Out;
	for (const auto& C : Cells)
	{
		Out += C.second;
	}
	return Out;
}

/** FH-5b: what the flap should settle on, read here from the network (§2.8.4): the countdown to the start in world seconds while the Main
 *  hasn't started, then its state's word; spaces dropped (a space cell draws no glyph). */
std::string HubsFlapWant(const ss::net::Network& Net, const ss::net::EventInstance& Main, double World)
{
	const ss::net::LiveState L = Net.Live(Main, World);
	std::string W;
	if ((L.St == ss::net::Status::Announced || L.St == ss::net::Status::Registering) && World < Main.Start)
	{
		const long long S = std::max(0LL, static_cast<long long>(std::ceil((Main.Start - World) * 60.0 - 1.0e-6)));
		char Buf[16];
		std::snprintf(Buf, sizeof(Buf), "%02lld:%02lld:%02lld", S / 3600, (S / 60) % 60, S % 60);
		W = Buf;
	}
	else
	{
		W = L.St == ss::net::Status::Announced || L.St == ss::net::Status::Registering ? "REGISTERING"
			: L.St == ss::net::Status::LateReg                                          ? "LATE REG"
			: L.St == ss::net::Status::Running                                          ? "RUNNING"
			: L.St == ss::net::Status::FinalTable                                       ? "FINAL TABLE"
																						: "COMPLETE";
	}
	W.erase(std::remove(W.begin(), W.end(), ' '), W.end());
	return W;
}

/** FH-5b: the runs holding Name whose box starts inside [X0, X1) x [Y0, Y1] (baseline). */
int HubsNamedIn(const DrawList& L, const std::string& Name, float X0, float Y0, float X1, float Y1)
{
	int N = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		N += Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.find(Name) != std::string::npos && Cmd.Text.X >= X0 && Cmd.Text.X < X1 && Cmd.Text.BaselineY >= Y0 &&
				Cmd.Text.BaselineY <= Y1
			? 1
			: 0;
	}
	return N;
}

/** FH-5b: the broadcast's common half on a host's Main Event Day page L (§2.8.4): the label, the flap's word against the network's,
 *  "LIVE · {Left} LEFT OF {Entries}" while the Main runs, the guarantee meter's form against site::show::OverlayOf, and WHO'S IN: the
 *  first Max people of World::FieldStoriesOf drawn by name and none past them (R-33's eight on Ignite), the Qualifiers line when it is 1
 *  or more. Returns how many people the field holds. Plants: the LIVE line's figures swapped; WHO'S IN cut to four people. */
int FestMainDayCommon(const char* Page, const DrawList& L, HubsLaptop& P, const ss::fest::Festival& F, const ss::net::EventInstance& Main, float WordX, float BandY, int Max)
{
	const ss::net::Network& Net = ss::net::Site(F.Site);
	const double World = P.S.WorldMinutes();
	const std::string Pg = Page;
	ui_test::Expect(ui_test::DrewRun(L, "MAIN EVENT DAY"), (Pg + ": the hero band reads MAIN EVENT DAY").c_str());
	const std::string Want = HubsFlapWant(Net, Main, World);
	const std::string Flap = HubsFlapRow(L, WordX, BandY + 84.0f, BandY + 148.0f);
	ui_test::Expect(Flap == Want, (Pg + ": the flap reads the Main's state (" + Flap + ", the network's " + Want + ")").c_str());
	const ss::net::LiveState Ls = Net.Live(Main, World);
	if (Ls.St == ss::net::Status::LateReg || Ls.St == ss::net::Status::Running || Ls.St == ss::net::Status::FinalTable)
	{
		const std::string Line = "LIVE \xC2\xB7 " + ss::Grouped(Ls.Left) + " LEFT OF " + ss::Grouped(Ls.Entries);
		ui_test::Expect(ui_test::DrewRun(L, Line), (Pg + ": the hero band reads \"" + Line + "\" (Network::Live)").c_str());
	}
	const ss::net::EventTemplate& T = Net.TemplateOf(Main);
	if (T.GtdCents > 0)
	{
		const ss::site::show::OverlayView V = ss::site::show::OverlayOf(Net, Main, World);
		const double Each = static_cast<double>(T.BuyInCents) * Net.Definition().Rake.PrizeShare(T);
		const double Over = V.Entries * Each - static_cast<double>(T.GtdCents);
		const std::string Form = V.Shown && V.Actual && V.Cents > 0 ? "OVERLAY \xC2\xB7 " + ss::ui::rlnet_detail::NetMoney(V.Cents)
			: V.Shown && !V.Actual && V.Cents > 0                    ? "PROJ SHORT \xC2\xB7 " + ss::ui::rlnet_detail::NetMoney(V.Cents)
			: V.Shown && !V.Actual && Over > 0.0                     ? "PROJ OVER \xC2\xB7 " + ss::ui::rlnet_detail::NetMoney(static_cast<ss::Chips>(std::llround(Over)))
																	 : std::string();
		ui_test::Expect(ui_test::DrewRun(L, "THE GUARANTEE") && (Form.empty() || ui_test::DrewRun(L, Form)),
			(Pg + ": the guarantee meter (\"" + Form + "\", site::show::OverlayOf)").c_str());
	}
	const ss::world::World& W = P.S.Living();
	const ss::world::FieldStories& Fs = W.FieldStoriesOf(Main.Id);
	const int Shown = std::min<int>(Max, static_cast<int>(Fs.People.size()));
	int Named = 0;
	for (int I = 0; I < Shown; ++I)
	{
		const ss::world::Npc* N = W.Get(Fs.People[static_cast<size_t>(I)].Npc);
		Named += N && ui_test::Drew(L, N->Name) ? 1 : 0;
	}
	ui_test::Expect(Named == Shown, (Pg + ": WHO'S IN names the field's first " + std::to_string(Shown) + " people in FieldStoriesOf's order (" + std::to_string(Named) + " drawn)").c_str());
	if (static_cast<int>(Fs.People.size()) > Max)
	{
		const ss::world::Npc* Past = W.Get(Fs.People[static_cast<size_t>(Max)].Npc);
		ui_test::Expect(!Past || HubsNamedIn(L, Past->Name, 0.0f, 600.0f, 1040.0f, 960.0f) == 0, (Pg + ": WHO'S IN stops at " + std::to_string(Max) + " people").c_str());
	}
	const int Q = Fs.Groups[static_cast<size_t>(ss::world::StoryGroup::Qualifiers)];
	if (Q > 0)
	{
		const std::string QLine = Q == 1 ? std::string("1 regular won their seat by satellite.") : ss::Grouped(Q) + " regulars won their seat by satellite.";
		ui_test::Expect(ui_test::DrewRun(L, QLine), (Pg + ": WHO'S IN's Qualifiers line \"" + QLine + "\"").c_str());
	}
	std::printf("  %s: %s, flap %s, WHO'S IN %zu people (%d shown), %d qualifiers\n", Page, ss::net::StatusName(Ls.St), Flap.c_str(), Fs.People.size(), Shown, Q);
	return static_cast<int>(Fs.People.size());
}

/** FH-5b: the reel's places drawn in REGISTRATIONS' rect R (their ordinals), top to bottom. */
std::vector<std::string> HubsReelPlaces(const DrawList& L, float X0, float Y0, float X1, float Y1)
{
	std::vector<std::pair<float, std::string>> Rows;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		const std::string& T = Cmd.Text.Text;
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.X >= X0 && Cmd.Text.X < X0 + 64.0f && Cmd.Text.X < X1 && Cmd.Text.BaselineY > Y0 + 36.0f && Cmd.Text.BaselineY < Y1 &&
			T.size() >= 3 && T.size() <= 4 && std::isdigit(static_cast<unsigned char>(T[0])) && std::isalpha(static_cast<unsigned char>(T.back())))
		{
			Rows.push_back({Cmd.Text.BaselineY, T});
		}
	}
	std::sort(Rows.begin(), Rows.end());
	std::vector<std::string> Out;
	for (const auto& R : Rows)
	{
		Out.push_back(R.second);
	}
	return Out;
}

/** FestPreviewStill's flap half (§2.15.2; FH-5b): a fresh open drawn in the stream preview shows every flap cell settled on the Main's
 *  state, while the live frame at the same instant waves (the cells turn from the open), and a second later settles; no cue plays.
 *  Plant: the flap animating in the preview. */
void FestPreviewStillFlap(HubsLaptop& P, const ss::fest::Festival& F, const ss::net::EventInstance& Main, float WordX, float BandY)
{
	const ss::net::Network& Net = ss::net::Site(F.Site);
	P.RL.ShowSeries(std::string(), P.Now);
	P.RL.OpenPage(RiverLine::Page::Series, P.Now);
	const size_t Cues = P.H.Sounds.size();
	ss::ui::rlfest_detail::HubPreviewing(P.RL, true);
	const DrawList Still = P.Frame(1.0f);
	ss::ui::rlfest_detail::HubPreviewing(P.RL, false);
	const std::string Want = HubsFlapWant(Net, Main, P.S.WorldMinutes());
	const DrawList Wave = P.Frame(1.0f);
	P.Now = ui_test::Step(P.S, P.Now, 1.0);
	const DrawList Settled = P.Frame(1.0f);
	const std::string S0 = HubsFlapRow(Still, WordX, BandY + 84.0f, BandY + 148.0f);
	const std::string S1 = HubsFlapRow(Wave, WordX, BandY + 84.0f, BandY + 148.0f);
	const std::string S2 = HubsFlapRow(Settled, WordX, BandY + 84.0f, BandY + 148.0f);
	ui_test::Expect(S0 == Want, ("FestPreviewStill: the preview's flap is its settled still (" + S0 + ", want " + Want + ")").c_str());
	ui_test::Expect(S1 != Want,("FestPreviewStill: the live frame at the open waves (" + S1 + ")").c_str());
	ui_test::Expect(S2 == HubsFlapWant(Net, Main, P.S.WorldMinutes()), ("FestPreviewStill: a second later the live flap has settled (" + S2 + ")").c_str());
	ui_test::Expect(P.H.Sounds.size() == Cues, "FestPreviewStill: no cue plays on the Main day's frames away from the bell");
}

/** RCOP 2026's Main Event Day (day 34, about 15:30; RiverLine): fest_hub_rl_mainday, the broadcast's checks and FestPreviewStill's flap
 *  half; the reel and the champion card between the Main's finish and the crown (fest_hub_rl_reel, with FestPreviewStill's reel half);
 *  the Afterglow is drawn on day 35 by the caller. Plants: the reel revealed 1st first; the reel animating in the preview. */
void FestMainDayRcop(HubsLaptop& P, const ss::fest::Festival& Rcop)
{
	const ss::net::Network& Net = ss::net::Site(ss::net::SiteId::RiverLine);
	ss::net::EventInstance Main;
	ui_test::Expect(HubsMainOf(Rcop, Main) && ss::net::DayOf(Main.Start) == Rcop.MainDay, "fest_hub_rl_mainday: RCOP 2026's Main runs on its Main day (34)");
	HubsTo(P, Rcop.MainDay, 15.5);
	P.OpenHub();
	ui_test::Expect(ss::fest::PhaseOf(Rcop, P.S.WorldMinutes(), &P.S.Living()) == ss::fest::Phase::MainDay, "fest_hub_rl_mainday: RCOP 2026 is on its Main day");
	const DrawList L = HubsPage("fest_hub_rl_mainday", P);
	FestMainCardPage(P, L, ss::ui::rlnet_detail::NetMoney(ss::net::Site(Rcop.Site).FindTemplate(Rcop.MainId)->GtdCents) + " GUARANTEED");
	FestMainDayCommon("fest_hub_rl_mainday", L, P, Rcop, Main, 210.0f, 132.0f, 12);
	const std::string Lock = HubsMainLock(P.S, Rcop);
	ui_test::Expect(ui_test::DrewRun(L, "REGISTRATIONS") && (Lock.empty() || ui_test::DrewRun(L, Lock)),
		("fest_hub_rl_mainday: REGISTRATIONS and the Main card's lock line (\"" + Lock + "\", until G3)").c_str());
	ui_test::Expect(HubsRunsIn(L, 24.0f, 348.0f, 1024.0f, 400.0f) <= 2, "fest_hub_rl_mainday: no day strip under REGISTRATIONS' title (the broadcast replaces the schedule)");
	FestPreviewStillFlap(P, Rcop, Main, 210.0f, 132.0f);
	// The reel: between the Main's finish and the crown (Closing, the result in), else at the first minute the result exists.
	const double End = Main.Start + Main.Duration;
	P.At(End + 1.0);
	bool Result = false;
	for (int K = 0; K < 30 && !Result; ++K)
	{
		Result = Net.Live(Main, P.S.WorldMinutes()).St == ss::net::Status::Finished && !Net.Result(Main).FinalTable.empty();
		if (!Result)
		{
			P.At(P.S.WorldMinutes() + 2.0);
		}
	}
	const ss::fest::Phase Ph = ss::fest::PhaseOf(Rcop, P.S.WorldMinutes(), &P.S.Living());
	std::printf("  the reel: the Main ended %s %s, the result in at %s %s, phase %d (Closing %d)\n", ss::net::DateLabel(ss::net::DayOf(End)).c_str(), ss::net::TimeLabel(End).c_str(),
		ss::net::DateLabel(ss::net::DayOf(P.S.WorldMinutes())).c_str(), ss::net::TimeLabel(P.S.WorldMinutes()).c_str(), static_cast<int>(Ph), static_cast<int>(ss::fest::Phase::Closing));
	ui_test::Expect(Result && Ph == ss::fest::Phase::Closing, "fest_hub_rl_reel: the Main's result is in before the crown (Closing)");
	const std::vector<ss::net::Placing> Table = Net.Result(Main).FinalTable;
	P.RL.ShowSeries(std::string(), P.Now);
	P.RL.OpenPage(RiverLine::Page::Series, P.Now);
	ss::ui::rlfest_detail::HubPreviewing(P.RL, true);
	const DrawList Still = P.Frame(1.0f);
	ss::ui::rlfest_detail::HubPreviewing(P.RL, false);
	const DrawList First = P.Frame(1.0f);
	P.Now = ui_test::Step(P.S, P.Now, 0.65);
	const DrawList Second = P.Frame(1.0f);
	P.Now = ui_test::Step(P.S, P.Now, 6.0);
	const std::vector<std::string> S0 = HubsReelPlaces(Still, 24.0f, 348.0f, 1024.0f, 598.0f);
	const std::vector<std::string> S1 = HubsReelPlaces(First, 24.0f, 348.0f, 1024.0f, 598.0f);
	const std::vector<std::string> S2 = HubsReelPlaces(Second, 24.0f, 348.0f, 1024.0f, 598.0f);
	const std::string Last = Table.empty() ? std::string("?") : ss::Ordinal(Table.back().Place);
	ui_test::Expect(S0.size() == Table.size(), ("FestPreviewStill: the preview draws the reel whole (" + std::to_string(S0.size()) + " of " + std::to_string(Table.size()) + " rows)").c_str());
	ui_test::Expect(S1.size() == 1 && S1.front() == Last && S2.size() == 2, ("fest_hub_rl_reel: the reel reveals " + Last + " first, one row every 0.6 s (" + std::to_string(S1.size()) + ", then " +
		std::to_string(S2.size()) + " rows)").c_str());
	const DrawList Reel = HubsPage("fest_hub_rl_reel", P);
	const std::vector<std::string> S3 = HubsReelPlaces(Reel, 24.0f, 348.0f, 1024.0f, 598.0f);
	ui_test::Expect(S3.size() == Table.size() && !S3.empty() && S3.front() == Last && S3.back() == "1st", "fest_hub_rl_reel: the settled reel runs from the last place to 1st");
	if (!Table.empty())
	{
		const ss::net::Placing& Ch = Table.front();
		const std::string Name = Ch.Player == -1 ? std::string("You") : Ch.Player == -2 ? Ch.Name : ss::net::Shared().Players()[static_cast<size_t>(Ch.Player)].Name;
		const std::string Line = Name + " won " + ss::fest::TrophyName(Rcop.Which) + " and " + ss::ui::rlnet_detail::NetMoney(Ch.Prize) + ".";
		ui_test::Expect(ui_test::DrewRun(Reel, "CHAMPION") && ui_test::DrewRun(Reel, Line) && ui_test::DrewRun(Reel, ss::ui::rlnet_detail::NetMoney(Ch.Prize)),
			("fest_hub_rl_reel: the champion card replaces the Main card (\"" + Line + "\")").c_str());
	}
}

/** The opening bell (§2.8.4; FH-5b) on WRIT 2027's Main (RunItTwice, day 118, 18:00): fest_hub_rit_bell and FestPreviewStill's bell
 *  half. The hub drawn within three world minutes of the start rings the look's sting once and shows "The Main Event is under way." on
 *  its band; drawn in the preview first, it rings nothing and shows no band, and the live frame after still rings (once). Plant: the
 *  bell ringing in the preview. */
void FestBellWrit(HubsLaptop& P, const ss::fest::Festival& Writ)
{
	ss::net::EventInstance Main;
	ui_test::Expect(HubsMainOf(Writ, Main) && ss::net::DayOf(Main.Start) == 118, "fest_hub_rit_bell: WRIT 2027's Main runs on day 118");
	P.At(Main.Start + 0.5);
	P.RL.SiteShown = ss::net::SiteId::RunItTwice;
	P.RL.ShowSeries(std::string(), P.Now);
	P.RL.OpenPage(RiverLine::Page::Series, P.Now);
	P.Now = ui_test::Step(P.S, P.Now, 1.0); // the flap's wave from the open settles (eight cells: 0.6 s), still inside the bell's three world minutes
	P.Away();
	const std::string Band = "The Main Event is under way.";
	const size_t Cues = P.H.Sounds.size();
	ss::ui::rlfest_detail::HubPreviewing(P.RL, true);
	const DrawList Still = P.Frame(1.0f);
	ss::ui::rlfest_detail::HubPreviewing(P.RL, false);
	ui_test::Expect(P.H.Sounds.size() == Cues && !ui_test::DrewRun(Still, Band), "FestPreviewStill: the preview rings no bell and shows no band");
	const DrawList Rung = P.Frame(1.0f);
	const ss::SoundId Sting = ss::ui::rlfest_detail::HubStingOf(ss::ui::festart::LookOf(Writ.Which), Writ.Site); // FH-6a: shared with the open's
	ui_test::Expect(P.H.Sounds.size() == Cues + 1 && P.H.Sounds.back() == Sting && ui_test::DrewRun(Rung, Band),
		"fest_hub_rit_bell: the Main's start rings the look's sting once and shows the band");
	P.Frame(1.0f);
	ui_test::Expect(P.H.Sounds.size() == Cues + 1, "fest_hub_rit_bell: the bell rings once a Main");
	const DrawList L = HubsPage("fest_hub_rit_bell", P);
	ui_test::Expect(ui_test::DrewRun(L, Band), "fest_hub_rit_bell: the band holds for 3 s");
	FestMainDayCommon("fest_hub_rit_bell", L, P, Writ, Main, 418.0f, 144.0f, 12);
	P.Now = ui_test::Step(P.S, P.Now, 3.5);
	ui_test::Expect(!ui_test::DrewRun(P.Frame(1.0f), Band) && P.H.Sounds.size() == Cues + 1, "fest_hub_rit_bell: the band is gone after 3 s, no second sting");
}

/** FestReignCopy's REIGN hub half (§2.15.1, PLAN_FHUBS Corrections item 2; FH-5b): REIGN 2027's Main Event Day (RunItTwice, day 306)
 *  draws no sponsor brand (every brand but a site's own, by name and short name). Plant: a sponsor brand drawn on the REIGN hub. */
void FestReignHub(HubsLaptop& P, const ss::fest::Festival& Reign)
{
	ss::net::EventInstance Main;
	ui_test::Expect(HubsMainOf(Reign, Main) && ss::net::DayOf(Main.Start) == 306, "fest_hub_rit_champ_mainday: REIGN 2027's Main runs on day 306");
	HubsTo(P, 306, 20.0);
	HubsOpenHost(P, ss::net::SiteId::RunItTwice);
	ui_test::Expect(ss::fest::PhaseOf(Reign, P.S.WorldMinutes(), &P.S.Living()) == ss::fest::Phase::MainDay, "fest_hub_rit_champ_mainday: REIGN 2027 is on its Main day");
	const DrawList L = HubsPage("fest_hub_rit_champ_mainday", P);
	FestMainDayCommon("fest_hub_rit_champ_mainday", L, P, Reign, Main, 418.0f, 144.0f, 12);
	const int Brands = HubsSponsorScan(L, "the REIGN hub");
	std::printf("FestReignCopy (the REIGN hub): %d sponsor brands looked for on fest_hub_rit_champ_mainday\n", Brands);
}

/** FestReignCopy's scan (FH-5b; FH-6a shares it for REIGN's title moment): no text run in L holds a sponsor brand (every brand but a
 *  site's own, by name and short name, three letters or more). Returns how many brands it looked for. */
int HubsSponsorScan(const DrawList& L, const char* What)
{
	int Brands = 0;
	for (const ss::sponsor::Brand& B : ss::sponsor::Brands())
	{
		if (B.Site != -1)
		{
			continue;
		}
		++Brands;
		for (const std::string& Word : {B.Name, B.Short})
		{
			if (Word.size() < 3)
			{
				continue;
			}
			const std::string Low = trademarks::Lower(Word);
			for (const ss::ui::DrawCmd& Cmd : L.Cmds)
			{
				if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && trademarks::Lower(Cmd.Text.Text).find(Low) != std::string::npos)
				{
					ui_test::Expect(false, ("FestReignCopy (" + std::string(What) + "): no sponsor brand drawn (" + Word + " in " + Cmd.Text.Text + ")").c_str());
				}
			}
		}
	}
	return Brands;
}

/** FH-5b (§2.8.4): on a championship's Micro and Mini days the schedule stays, the title row pinned at the top of the day's rows. */
void FestTitleRowsPinned(HubsLaptop& P, const ss::fest::Festival& F, ss::net::SiteId Host)
{
	const ss::net::Network& Net = ss::net::Site(F.Site);
	std::vector<std::pair<double, size_t>> Titles; // in day order (the Micro's day comes before the Mini's)
	std::map<size_t, ss::net::EventInstance> Of;
	for (size_t K = 1; K < F.TitleIds.size(); ++K)
	{
		ss::net::EventInstance T;
		if (Net.Next(F.TitleIds[K], static_cast<double>(F.FirstDay) * ss::net::MinutesPerDay, T) && ss::net::DayOf(T.Start) >= ss::net::DayOf(P.S.WorldMinutes()))
		{
			Titles.push_back({T.Start, K});
			Of[K] = T;
		}
	}
	std::sort(Titles.begin(), Titles.end());
	ui_test::Expect(!Titles.empty(), (F.Name + ": its Mini and Micro Main Events lie ahead").c_str());
	for (const std::pair<double, size_t>& It : Titles)
	{
		const size_t K = It.second;
		const ss::net::EventInstance& T = Of[K];
		HubsTo(P, ss::net::DayOf(T.Start), 9.0);
		HubsOpenHost(P, Host);
		P.Frame(1.0f);
		const std::vector<std::string> Rows = P.RL.ListedEvents();
		ui_test::Expect(!Rows.empty() && Rows.front() == T.Id && ss::fest::PhaseOf(F, P.S.WorldMinutes(), &P.S.Living()) != ss::fest::Phase::MainDay,
			(F.Name + ": the " + F.TitleIds[K] + " row is pinned at the top of its day's rows (" + std::to_string(Rows.size()) + " rows; first " + (Rows.empty() ? std::string("none") : Rows.front()) +
				")").c_str());
	}
}

void FestCareerRiverLine()
{
	HubsLaptop P("u4-fest-career-b");
	const ss::fest::Festival* Rcop = ss::fest::FestivalOf("rcop");
	const ss::fest::Festival* Writ = ss::fest::FestivalOf("runittwice.writ27");
	const ss::fest::Festival* Grit = ss::fest::FestivalOf("runittwice.grit27");
	const ss::fest::Festival* Slam = ss::fest::FestivalOf("slam27");
	ui_test::Expect(Rcop && Writ && Grit && Slam, "FH-2: RCOP 2026, WRIT 2027, GRIT 2027 and Summer Slam 2027 are F-net's rows");
	if (!Rcop || !Writ || !Grit || !Slam)
	{
		return;
	}
	// Night One: the ten-cell strip.
	P.OpenHub();
	P.RL.UI.Ptr.Active = true;
	P.RL.UI.Ptr.X = 260.0f + 9.0f * 128.0f + 50.0f; // resting on FEIGN's cell (fsy9: BY INVITATION)
	P.RL.UI.Ptr.Y = 104.0f;
	const DrawList NightOne = HubsPage("fest_strip_night_one", P);
	P.Away();
	const int Cells = FestStripTenPage(NightOne, HubsStripRiverLine, P.S.WorldMinutes());
	ui_test::Expect(Cells == 10, ("FestStripTen: Night One's strip holds all ten brands (" + std::to_string(Cells) + ")").c_str());
	ui_test::Expect(HubsRunsIn(NightOne, 260.0f + 9.0f * 128.0f, 120.0f, 260.0f + 10.0f * 128.0f, 124.0f) == 1 && ui_test::DrewRun(NightOne, "BY INVITATION"),
		"FestStripTen: FEIGN's cell without an Ignite account reads BY INVITATION (no Pass before F-world's FW-7)");
	HubsStripClicks(P, HubsStripRiverLine);
	// FH-7, FestHubBudgets: the Career page before the hero's festival pieces (Night One: no pin, no festival trophy).
	HubsCareerCost(P, HubsBook().CareerBefore);
	// RCOP 2026's first day: the Main card's lock line, THE RACE first.
	HubsTo(P, Rcop->FirstDay, 12.0);
	const ss::SoundId RlSting = ss::ui::rlfest_detail::HubStingOf(ss::ui::festart::LookOf(Rcop->Which), ss::net::SiteId::RiverLine);
	const size_t Stung = P.H.Sounds.size();
	P.OpenHub();
	const DrawList Live = HubsPage("fest_hub_rl_live", P);
	const auto G0 = std::chrono::steady_clock::now();
	const std::vector<std::string> Locks = HubsLockLines();
	FestLockFits(Locks);
	std::printf("  g1p-hub: FestLockFits (the lines read and set out) %.2f s\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - G0).count());
	FestLockFitsPage(P, Live, "fest_hub_rl_live", {24.0f, 408.0f, 1000.0f, 504.0f}, Locks, true);
	const std::string Lock = HubsMainLock(P.S, *Rcop);
	ui_test::Expect(!Lock.empty() && ui_test::DrewRun(Live, Lock), ("FestLockedMain: RCOP 2026's Main card shows the lock line (\"" + Lock + "\")").c_str());
	ui_test::Expect(ui_test::DrewRun(Live, "PLAYER OF THE SERIES") && !ui_test::Drew(Live, "No champion yet."), "FestLockedMain: the hub opens on THE RACE while Live, never past it");
	ui_test::Expect(ui_test::DrewRun(Live, "DAY 1 OF 22"), "fest_hub_rl_live: RCOP 2026's first day reads DAY 1 OF 22");
	// FH-6a, FestPreviewStill (the hub's sting, §2.7.6): the day's first open stung once; the series shown again in the same instant and
	// the preview add none; FestYourStripHub's reopen at 23:30 (the session's clock moved, so a second sting would be heard) adds none, and
	// days 14 and 15 one each: three in all. Plant: a sting on every hub open (the 23:30 reopen makes four).
	const int FirstOpen = HubsCues(P.H, RlSting, Stung);
	P.RL.ShowSeries(std::string(), P.Now + 0.001);
	P.Frame(1.0f);
	ss::ui::rlfest_detail::HubPreviewing(P.RL, true);
	P.Frame(1.0f);
	ss::ui::rlfest_detail::HubPreviewing(P.RL, false);
	ui_test::Expect(FirstOpen == 1 && HubsCues(P.H, RlSting, Stung) == 1,
		("FestPreviewStill (the hub's sting): RCOP 2026's first open of day 13 stings once, the series shown again and the preview never (" +
			std::to_string(FirstOpen) + ", " + std::to_string(HubsCues(P.H, RlSting, Stung)) + ")").c_str());
	// FH-4A: the YOUR panel before and after the hero's first RCOP event (fest_hub_your_ladder), then THE RACE's movement on days 14-15.
	FestYourStripHub(P, *Rcop);
	// FH-6a: the pin wall after the hero's first festival event (FestPinsReveal's after half).
	FestPinsRevealAfter(P, *Rcop);
	for (const int Day : {14, 15})
	{
		HubsTo(P, Day, 12.0);
		P.OpenHub();
		P.Frame(1.0f);
		std::printf("  THE RACE on day %d: %d movement marks (the hub saw the day before)\n", Day, ss::ui::rlfest_detail::HubRaceMoves(P.RL));
	}
	ui_test::Expect(HubsCues(P.H, RlSting, Stung) == 3, ("FestPreviewStill (the hub's sting): day 13's 23:30 reopen stings nothing, the first open of days 14 and 15 once each (" +
		std::to_string(HubsCues(P.H, RlSting, Stung)) + " in all from day 13)").c_str());
	// Day 19 (a Saturday): the ladder in view.
	HubsTo(P, 19, 12.0);
	P.OpenHub();
	for (int K = 0; K < 12; ++K)
	{
		P.RL.UI.Ptr.Active = true;
		P.RL.UI.Ptr.X = 300.0f;
		P.RL.UI.Ptr.Y = 600.0f;
		P.RL.UI.Ptr.Wheel = 1.0f;
		P.Frame(1.0f);
	}
	P.Away();
	const DrawList Locked = HubsPage("fest_hub_rl_locked", P);
	FestLockedMainRows(P, *Rcop, Locked);
	ui_test::Expect(ui_test::DrewRun(Locked, Lock), "FestLockedMain: the Main card keeps its lock line on day 19");
	ui_test::Expect(ss::fest::YourLine(*Rcop, &P.S.Living()).Shown && ui_test::DrewRun(Locked, "PLAYER OF THE SERIES"),
		"FestLockedMain: with the hero's RCOP pin the hub still opens on THE RACE while the Main is locked");
	// FH-7, FestHubBudgets: the lobby with RCOP 2026's crests on its rows (Live, day 19).
	HubsBook().Lobbies.emplace_back("RCOP 2026 Live (day 19)", HubsLobbyCost(P));
	// FH-5b: RCOP 2026's Main Event Day, the reel before the crown, then the Afterglow on day 35.
	FestMainDayRcop(P, *Rcop);
	HubsTo(P, 35, 12.0);
	// FH-6a: RCOP 2026's crown left the hero a mark (the one cash of FestYourStripHub); its moment would cover every page below (and take
	// their clicks), so the hero bows it at once through the session (Session::DoneFestMoment, as TAKE A BOW does), moving no clock.
	// The moments' own pages and checks are FestMarkMomentHubs'.
	for (int K = 0; K < 8 && P.S.PeekFestMoment(); ++K)
	{
		const std::optional<ss::FestMoment> Mm = P.S.PeekFestMoment();
		std::printf("  day 35: the hero bows the moment RCOP 2026's crown left (kind %d, %s, mark %d, %d of %d)\n", Mm->Kind, Mm->Key.c_str(), Mm->Mark, Mm->Rank, Mm->Scorers);
		P.S.DoneFestMoment();
	}
	ui_test::Expect(!P.S.PeekFestMoment(), "FestMarkMoment: the crown's moments are bowed (keyed) before the Afterglow's pages");
	P.OpenHub();
	ui_test::Expect(ss::fest::PhaseOf(*Rcop, P.S.WorldMinutes(), &P.S.Living()) == ss::fest::Phase::Afterglow, "fest_hub_rl_afterglow: RCOP 2026 is in its Afterglow on day 35");
	const DrawList Glow = HubsPage("fest_hub_rl_afterglow", P);
	ui_test::Expect(ui_test::DrewRun(Glow, "COMPLETE") && ui_test::DrewRun(Glow, "CHAMPION") && !ui_test::DrewRun(Glow, "MAIN EVENT DAY") && !ui_test::Drew(Glow, "No champion yet."),
		"fest_hub_rl_afterglow: the schedule again, the banner's COMPLETE, the champion card in the Main card's place, the roll filled");
	// FH-4A, FestRollTitles: RCOP 2026's title lines on day 35.
	FestRollTitlesDay35(P, *Rcop);
	// FH-7, FestHubBudgets: the lobby in RCOP 2026's Afterglow (slide 0's U15 form and the wire's champion item, FH-5c), day 35.
	HubsBook().Lobbies.emplace_back("RCOP 2026 Afterglow (day 35)", HubsLobbyCost(P));
	// WRIT 2027: a RunItTwice account, then The Summons' seats (U33) on its day.
	ss::net::EventInstance SummonsDone; // FH-5b: The Summons, read once finished in WRIT's Afterglow (below the bell)
	ss::Chips SummonsSeat = 0;
	HubsTo(P, Writ->FirstDay, 12.0);
	ui_test::Expect(HubsAccept(P, ss::net::SiteId::RunItTwice), "FH-2: RunItTwice's invitation accepted");
	{
		const ss::net::Network& Rit = ss::net::Site(ss::net::SiteId::RunItTwice);
		ss::net::EventInstance Sum;
		const bool Has = !Writ->Satellites.empty() && Rit.Next(Writ->Satellites.front(), static_cast<double>(Writ->FirstDay) * ss::net::MinutesPerDay, Sum);
		ui_test::Expect(Has, "U33: The Summons runs inside WRIT 2027");
		if (Has)
		{
			P.At(Sum.Start - 20.0);
			HubsOpenHost(P, ss::net::SiteId::RunItTwice);
			P.RL.UI.Ptr.Active = false;
			const ss::net::LiveState Ls = Rit.Live(Sum, P.S.WorldMinutes());
			const ss::net::EventTemplate& T = Rit.TemplateOf(Sum);
			const long long Seats = std::max<long long>(1, Ls.Pool / std::max<ss::Chips>(1, T.SeatValueCents));
			const DrawList W = P.Frame(1.0f);
			const std::string Want = Seats == 1 ? std::string("1 SEAT AT THE POOL SO FAR") : std::to_string(Seats) + " SEATS AT THE POOL SO FAR";
			ui_test::Expect(Ls.Pool > 0 && ui_test::DrewRun(W, Want), ("U33: The Summons' row reads \"" + Want + "\" (max(1, Pool / SeatValue) while it registers)").c_str());
			SummonsDone = Sum;
			SummonsSeat = T.SeatValueCents;
		}
	}
	// FH-5b: WRIT 2027's Main at 18:00 on day 118: the opening bell.
	FestBellWrit(P, *Writ);
	// FH-7, FestHubBudgets: WRIT 2027's Closing on RunItTwice (a minute into its last day + 1, before the crown), drawn.
	HubsBudgetState(P, *Writ, ss::fest::Phase::Closing, "rit_closing (WRIT 2027)");
	// U33's finished form (FH-2's check): The Summons ends past midnight on the Main's day, where the broadcast holds the hub (no day
	// strip, FH-5b), so its row is read in the Afterglow, the schedule back: the day strip's cell for its day brings it (fsd<day>).
	if (!SummonsDone.Id.empty())
	{
		const ss::net::Network& Rit = ss::net::Site(ss::net::SiteId::RunItTwice);
		HubsTo(P, Writ->LastDay + 1, 12.0);
		HubsOpenHost(P, ss::net::SiteId::RunItTwice);
		HubsDrawn("rit_afterglow (WRIT 2027)", P); // FH-7, FestHubBudgets: RunItTwice's Afterglow, drawn
		const int SumDay = ss::net::DayOf(SummonsDone.Start);
		const float Cw = (860.0f - 4.0f * static_cast<float>(Writ->LastDay - Writ->FirstDay)) / static_cast<float>(Writ->LastDay - Writ->FirstDay + 1);
		P.Click(232.0f + static_cast<float>(SumDay - Writ->FirstDay) * (Cw + 4.0f) + Cw / 2.0f, 378.0f);
		const DrawList Done = P.Frame(1.0f);
		const ss::net::EventInstance Fin = HubsInstance(Rit, SummonsDone.Id);
		const long long Won = std::max<long long>(1, Fin.Pool / std::max<ss::Chips>(1, SummonsSeat));
		const std::string WonText = Won == 1 ? std::string("1 SEAT WON") : std::to_string(Won) + " SEATS WON";
		ui_test::Expect(ss::fest::PhaseOf(*Writ, P.S.WorldMinutes(), &P.S.Living()) == ss::fest::Phase::Afterglow && ui_test::DrewRun(Done, WonText),
			("U33: once finished The Summons' row reads \"" + WonText + "\" (read in WRIT's Afterglow)").c_str());
		// g1p-hub item 9 (PIE s08a_pennant_crown_champions: a satellite's winner with "$0.00"): The Summons' winner won a seat, and no
		// row prints a $0.00 prize. Item 13 (§2.16 step 8, N-2; PIE s08_writ_hub): WRIT's rows read THE BOARD's codes, "WRIT {nn}: ...",
		// with no "#n" pill. Both scales. Plants: Money(W.Prize) for a satellite's winner; the "#n" pill and NetShortName on RunItTwice.
		const DrawList DoneHalf = P.Frame(1.5f);
		const ss::ui::Rect RitRows{232.0f, 412.0f, 860.0f, 504.0f};
		int Hash = 0, Coded = 0, Zero = 0, Seat = 0;
		for (int K = 0; K < 2; ++K)
		{
			const DrawList& Ld = K == 0 ? Done : DoneHalf;
			HubsRowCodes(Ld, RitRows, "WRIT", Hash, Coded);
			for (const ss::ui::DrawCmd& Cmd : Ld.Cmds)
			{
				const bool InRows = Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.X >= RitRows.X && Cmd.Text.BaselineY >= RitRows.Y &&
					Cmd.Text.BaselineY <= RitRows.Y + RitRows.H;
				Zero += InRows && Cmd.Text.Text == "$0.00" ? 1 : 0;
				Seat += InRows && Cmd.Text.Text == "Won a seat" ? 1 : 0;
			}
		}
		ui_test::Expect(Seat == 2 && Zero == 0, ("g1p-hub: The Summons' winner reads \"Won a seat\", no row a $0.00 prize, at both scales (" + std::to_string(Seat) + ", " + std::to_string(Zero) + ")").c_str());
		ui_test::Expect(Hash == 0 && Coded >= 2, ("g1p-hub: WRIT's rows print THE BOARD's codes (\"WRIT {nn}: ...\") and no \"#n\" pill, at both scales (" + std::to_string(Coded) + " coded, " +
			std::to_string(Hash) + " pills)").c_str());
	}
	// FH-7, FestHubBudgets: GRIT 2027's Buildup on RunItTwice (day 216), drawn.
	HubsBudgetState(P, *Grit, ss::fest::Phase::Buildup, "rit_buildup (GRIT 2027)");
	// GRIT 2027's second night: two runs a night.
	HubsTo(P, Grit->FirstDay + 1, 18.0);
	HubsOpenHost(P, ss::net::SiteId::RunItTwice);
	const DrawList Twice = HubsPage("fest_hub_rit_twice", P);
	{
		// g1p-hub item 2 (PIE s08_grit_hub: "May 12 – Ma…"): GRIT's dates whole beside NIGHT 2 OF 12 at both scales; item 13: no "#n"
		// pill on its rows (a show's codes stand in its runs' column); item 1: its lock lines inside the 300 cells. Plant (item 2): the
		// stats kept StatGap apart.
		const std::string Dates = ss::net::DateLabel(Grit->FirstDay) + " \xE2\x80\x93 " + ss::net::DateLabel(Grit->LastDay);
		const DrawList Half = P.Frame(1.5f);
		ui_test::Expect(ui_test::DrewRun(Twice, Dates) && ui_test::DrewRun(Half, Dates), ("FestBannerStats: GRIT 2027's banner draws its dates whole, " + Dates + ", at both scales").c_str());
		int Hash = 0, Coded = 0;
		HubsRowCodes(Twice, {232.0f, 412.0f, 860.0f, 504.0f}, "GRIT", Hash, Coded);
		HubsRowCodes(Half, {232.0f, 412.0f, 860.0f, 504.0f}, "GRIT", Hash, Coded);
		ui_test::Expect(Hash == 0, ("g1p-hub: GRIT's rows carry no \"#n\" pill at either scale (" + std::to_string(Hash) + ")").c_str());
		FestLockFitsPage(P, Twice, "fest_hub_rit_twice", {232.0f, 412.0f, 860.0f, 504.0f}, HubsLockLines(), false);
	}
	FestStripTenPage(Twice, HubsStripRunItTwice, P.S.WorldMinutes());
	ui_test::Expect(ui_test::DrewRun(Twice, "NIGHT 2 OF 12"), "fest_hub_rit_twice: GRIT's banner counts nights (U2: NIGHT 2 OF 12)");
	ui_test::Expect(ui_test::DrewRun(Twice, "NIGHT 1") && ui_test::DrewRun(Twice, "NIGHT 12"), "fest_hub_rit_twice: GRIT's day strip reads nights");
	ui_test::Expect(ui_test::DrewRun(Twice, "7 PM") && ui_test::DrewRun(Twice, "10 PM"), "fest_hub_rit_twice: a show's row carries its two buttons, 7 PM and 10 PM");
	const std::vector<std::string> Listed = P.RL.ListedEvents();
	const ss::net::Network& Rit = ss::net::Site(ss::net::SiteId::RunItTwice);
	std::string EarlyId;
	std::string LateId;
	for (size_t I = 0; I + 1 < Listed.size() && EarlyId.empty(); ++I)
	{
		const ss::net::EventInstance A = HubsInstance(Rit, Listed[I]);
		const ss::net::EventInstance B = HubsInstance(Rit, Listed[I + 1]);
		if (A.Template == B.Template && Rit.TemplateOf(A).Series == Grit->SeriesId)
		{
			EarlyId = A.Id;
			LateId = B.Id;
		}
	}
	ui_test::Expect(!EarlyId.empty(), "fest_hub_rit_twice: the night's show lists its two runs");
	if (!EarlyId.empty())
	{
		const std::string CodeE = Rit.NameOf(HubsInstance(Rit, EarlyId)).substr(0, Rit.NameOf(HubsInstance(Rit, EarlyId)).find(": "));
		const std::string CodeL = Rit.NameOf(HubsInstance(Rit, LateId)).substr(0, Rit.NameOf(HubsInstance(Rit, LateId)).find(": "));
		ui_test::Expect(CodeE.size() > 2 && CodeE.compare(CodeE.size() - 2, 2, "-E") == 0 && CodeL.compare(CodeL.size() - 2, 2, "-L") == 0 && ui_test::DrewRun(Twice, CodeE) &&
				ui_test::DrewRun(Twice, CodeL),
			("fest_hub_rit_twice: the buttons stand beside the runs' codes (" + CodeE + ", " + CodeL + ")").c_str());
		const std::pair<float, float> Ten = HubsRunAt(Twice, "10 PM");
		P.Click(Ten.first + 8.0f, Ten.second - 4.0f);
		ui_test::Expect(P.RL.SelectedEvent() == LateId, "fest_hub_rit_twice: 10 PM picks the late run (its register button the late run's)");
		ui_test::Expect(!P.S.IsPlaying(LateId) && !P.S.ConfirmRegister, "fest_hub_rit_twice: picking a run registers nothing");
		// FestCopyUi's U15 half (FestBlurbsFilled): the ticker's GRIT line is the edition's own night, nights and show times.
		const ss::net::EventInstance E = HubsInstance(Rit, EarlyId);
		const ss::net::EventInstance Lt = HubsInstance(Rit, LateId);
		auto ShowOf = [](double Start) {
			const long long M = static_cast<long long>(std::llround(Start)) % 1440;
			const long long H = M / 60;
			return std::to_string(H % 12 == 0 ? 12 : H % 12) + (H < 12 ? " AM" : " PM");
		};
		const std::string Line = "GRIT \xC2\xB7 NIGHT 2 OF " + std::to_string(Grit->LastDay - Grit->FirstDay + 1) + " \xC2\xB7 SHOWS AT " + ShowOf(E.Start) + " AND " + ShowOf(Lt.Start);
		const DrawList Again = P.Frame(1.0f);
		ui_test::Expect(ui_test::Drew(Again, Line), ("FestCopyUi: GRIT's ticker line is the edition's own (\"" + Line + "\", U15)").c_str());
	}
	// FH-4A fix round 1: REIGN 2027's step rows (U35) on days 302 and 303.
	FestStepPillsReign(P);
	// FH-5b: REIGN 2027's Micro and Mini rows pinned on their days, then its Main Event Day (day 306) with no sponsor mark.
	if (const ss::fest::Festival* Reign = ss::fest::FestivalOf("runittwice.reign27"))
	{
		FestTitleRowsPinned(P, *Reign, ss::net::SiteId::RunItTwice);
		FestReignHub(P, *Reign);
	}
	else
	{
		ui_test::Expect(false, "fest_hub_rit_champ_mainday: REIGN 2027 is F-net's row");
	}
	// Summer Slam 2027 (its Main locked until G3, §2.2.3): the Main card's lock line.
	HubsTo(P, Slam->FirstDay + 1, 12.0);
	P.OpenHub();
	const DrawList SlamL = P.Frame(1.0f);
	const std::string SlamLock = HubsMainLock(P.S, *Slam);
	ui_test::Expect(!SlamLock.empty() && ui_test::DrewRun(SlamL, SlamLock) && ui_test::DrewRun(SlamL, "YOUR " + HubsShortOf(*Slam)),
		("FestLockedMain: Summer Slam 2027's Main card shows the lock line (\"" + SlamLock + "\")").c_str());
	// Day 395: FEIGN II's cell (Announced from day 391), never an Ignite account in this career. FH-7 (Q25, PLAN_G1 items 33 and 34):
	// with F-world's FW-7 the Pass counts (FestivalEvents = 20). Career B pays no rent on its walk (the test never pays it), so its two
	// rents are planted here, after Summer Slam's page and before FEIGN II's window opens on day 391: the cell reads "PASS {N}/{Need}"
	// and the popover U30's counting sentence, N the hero's paid online events (max(OnlineEvents, Paid100)), Need fest::PassNeeds.
	P.S.Life.RentsPaid = std::max(P.S.Life.RentsPaid, 2);
	HubsTo(P, 395, 12.0);
	P.OpenHub();
	const DrawList Before = P.Frame(1.0f);
	const ss::fest::Festival* Feign2 = ss::fest::FestivalOf("ignite.feign27");
	ui_test::Expect(Feign2 && Feign2->Edition == 2 && ss::fest::PhaseOf(*Feign2, P.S.WorldMinutes(), nullptr) == ss::fest::Phase::Announced,
		"fest_popover_pass: FEIGN 2027 (edition II) is Announced on day 395");
	const ss::net::InviteGate& IgGate = ss::net::Def(ss::net::SiteId::Ignite).Gate;
	const auto Rec = P.S.Record();
	const int PassNeed = Feign2 ? ss::fest::PassNeeds(*Feign2, IgGate) : 0;
	const int PassN = std::max(0, std::min(PassNeed, std::max(Rec.OnlineEvents, Rec.Paid100)));
	const std::string PassCell = "PASS " + std::to_string(PassN) + "/" + std::to_string(PassNeed);
	ui_test::Expect(IgGate.FestivalEvents == 20 && PassNeed == 20 && PassN < PassNeed && ss::fest::PassFestival(ss::net::SiteId::Ignite, P.S.WorldMinutes()) == Feign2,
		("fest_popover_pass: FEIGN II's Pass counts on day 395 (" + std::to_string(PassN) + " of " + std::to_string(PassNeed) + " paid online events; FestivalEvents " +
			std::to_string(IgGate.FestivalEvents) + ")").c_str());
	ui_test::Expect(HubsRunsIn(Before, 260.0f + 9.0f * 128.0f, 120.0f, 260.0f + 10.0f * 128.0f, 124.0f) == 1 && ui_test::DrewRun(Before, PassCell),
		("fest_popover_pass: FEIGN II's cell carries its third line, " + PassCell + " (U30)").c_str());
	P.Click(260.0f + 9.0f * 128.0f + 50.0f, 104.0f, true);
	const DrawList Pop = HubsPage("fest_popover_pass", P);
	const std::string Counting = "Festival Pass: " + std::to_string(PassN) + " of " + std::to_string(PassNeed) + " online events.";
	ui_test::Expect(ui_test::DrewRun(Pop, "FEIGN 2027") && ui_test::DrewRun(Pop, Counting) && !ui_test::DrewRun(Pop, "By invitation.") && !ui_test::DrewRun(Pop, "OPEN THE HUB"),
		("fest_popover_pass: FEIGN II's popover reads \"" + Counting + "\" with no button (U30's counting form)").c_str());
	{
		const long long PopV = static_cast<long long>(Pop.Vertices.size()) - static_cast<long long>(Before.Vertices.size());
		const long long PopR = static_cast<long long>(HubsCostOf(Pop).Runs) - static_cast<long long>(HubsCostOf(Before).Runs);
		std::printf("  fest_popover_pass: the popover %lld vertices (<= 3000), %lld runs (<= 20) over the page\n", PopV, PopR);
		ui_test::Expect(PopV <= 3000 && PopR <= 20, ("FestHubBudgets: fest_popover_pass's popover within 3k vertices and 20 runs over the page (" + std::to_string(PopV) + ", " +
			std::to_string(PopR) + ")").c_str());
	}
	// FH-7: fest_card_defending (§2.6.3, §2.6.4; PLAN_FHUBS Q16): RCOP 2026's Main champion's card on day 395, RCOP 2027 Live: the true
	// defender's DEFENDING CHAMPION pill and the honorific line (stream B's NetFestDefends and NetFestHonorific, FH-6b).
	{
		const ss::world::World& W = P.S.Living();
		const ss::fest::Festival* Rcop27 = ss::fest::FestivalOf("rcop27");
		int Champ = -1;
		for (const ss::world::FestRollRow& Row : ss::world::FestRoll(W, ss::fest::Brand::Rcop))
		{
			Champ = Row.Key == Rcop->SeriesId && Row.Main ? Row.Main->Npc : Champ;
		}
		const double Now = P.S.WorldMinutes();
		const ss::fest::Festival* Defends = Champ >= 0 ? ss::ui::rlnet_detail::NetFestDefends(W, Champ, Now) : nullptr;
		const std::string Honorific = Champ >= 0 ? ss::ui::rlnet_detail::NetFestHonorific(W, Champ, Now) : std::string();
		ui_test::Expect(Champ >= 0 && Rcop27 && ss::fest::PhaseOf(*Rcop27, Now, &W) == ss::fest::Phase::Live && Defends == Rcop27 && !Honorific.empty(),
			("fest_card_defending: RCOP 2026's Main champion defends the title at RCOP 2027, Live on day 395 (\"" + Honorific + "\")").c_str());
		P.Click(1100.0f, 366.0f); // the popover closed off it (swallowed)
		P.Away();
		P.RL.ShowPlayer(Champ, P.Now);
		P.Now = ui_test::Step(P.S, P.Now, 0.5);
		P.Away();
		const DrawList Card = HubsPage("fest_card_defending", P);
		const ss::world::Npc* N = W.Get(Champ);
		ui_test::Expect(P.RL.CardOpen() && N && ui_test::Drew(Card, N->Name) && ui_test::DrewRun(Card, "DEFENDING CHAMPION") && ui_test::DrewRun(Card, Honorific),
			("fest_card_defending: " + (N ? N->Name : std::string("?")) + "'s card carries DEFENDING CHAMPION and \"" + Honorific + "\"").c_str());
		P.RL.ShowPlayer(-1, P.Now);
		P.Frame(1.0f);
		// g1p-hub items 4 and 14 (after the card's page; the next open, day 399's, starts afresh).
		const auto G1 = std::chrono::steady_clock::now();
		FestRollPillClear(P, N ? N->Name : std::string("?"));
		if (Rcop27)
		{
			FestLockedMainRows2027(P, *Rcop27);
		}
		std::printf("  g1p-hub: FestRollPillClear and FestLockedMain (2027) %.2f s\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - G1).count());
	}
	// FH-5b, U4 (§2.6.3): RCOP 2027's Main Event Day (day 399, 14:30, its field registering): the Main card names RCOP 2026's champion while FieldStoriesOf lists
	// them in the Main's field (and only then). Plant: the U4 line dropped.
	if (const ss::fest::Festival* Rcop27 = ss::fest::FestivalOf("rcop27"))
	{
		ss::net::EventInstance M27;
		ui_test::Expect(HubsMainOf(*Rcop27, M27) && ss::net::DayOf(M27.Start) == 399, "U4: RCOP 2027's Main runs on day 399");
		HubsTo(P, 399, 14.5);
		P.OpenHub();
		const DrawList D27 = P.Frame(1.0f);
		const ss::world::World& W = P.S.Living();
		std::string Want;
		ui_test::Expect(!W.FieldStoriesOf(M27.Id).People.empty(), "U4: RCOP 2027's Main field holds stories at 14:30 (the check below reads a real field)");
		for (const ss::world::StoryPerson& Sp : W.FieldStoriesOf(M27.Id).People)
		{
			const ss::world::Npc* N = Sp.Why == ss::world::StoryWhy::DefendingChampion && Want.empty() ? W.Get(Sp.Npc) : nullptr;
			Want = N ? "DEFENDING CHAMPION \xC2\xB7 " + N->Name + " \xC2\xB7 registered" : Want;
		}
		ui_test::Expect(Want.empty() ? !ui_test::Drew(D27, "DEFENDING CHAMPION") : ui_test::DrewRun(D27, Want),
			("U4: RCOP 2027's Main card shows the defending champion exactly when FieldStoriesOf lists them (\"" + Want + "\")").c_str());
		std::printf("  U4 on day 399: %s\n", Want.empty() ? "RCOP 2026's champion is not in RCOP 2027's field" : Want.c_str());
	}
	// FH-4A: day 400, RCOP 2026 and 2027 on one roll.
	HubsTo(P, 400, 12.0);
	FestRollTitlesDay400(P);
	// FH-6a, after every page above (the moments' frames and steps move the laptop's clock): REIGN 2027's Mini title moment draws no
	// sponsor mark (FestReignCopy's title-moment half, FestTitleMoment), and RCOP 2027's Main moment names this world's second edition (U36).
	if (const ss::fest::Festival* Reign = ss::fest::FestivalOf("runittwice.reign27"))
	{
		FestTitleMomentHubs(P, *Reign, "reign", nullptr);
	}
	FestMainEditionTwo(P);
	// FH-7: fest_career_pins (§2.8.7, §2.15.2: ten slots): RiverLine's Career page at the end of career B, the pin wall shown, and the
	// Career row of FestHubBudgets (the page with the hero's festival pieces inside its Night One cost + 4k).
	HubsCareerCost(P, HubsBook().CareerAfter);
	const DrawList Pins = HubsPage("fest_career_pins", P);
	std::string Wall;
	for (const ss::ui::DrawCmd& Cmd : Pins.Cmds)
	{
		Wall = Wall.empty() && Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.rfind("FESTIVAL PINS \xC2\xB7 ", 0) == 0 ? Cmd.Text.Text : Wall;
	}
	ui_test::Expect(!Wall.empty(), ("fest_career_pins: the Career page shows the pin wall (" + Wall + ")").c_str());
}

// ---- FH-6a: the moments (§2.8.5), the pin wall (§2.8.7) and the stings (§2.7.6).

/** A frame as RiverLine::Draw draws it with the lead's FestMomentHold line before the app (patch fhubs_to_lead_4): the hold, the draw. */
DrawList HubsHeld(HubsLaptop& P)
{
	ss::ui::rlfest_detail::FestMomentHold(P.RL);
	return P.Frame(1.0f);
}

/** A one-frame click at (X, Y) (pressed and released inside one frame) on a held frame, then (Settle) a settled held frame. */
DrawList HubsHeldClick(HubsLaptop& P, float X, float Y, bool Settle = true)
{
	P.RL.UI.Ptr.Active = true;
	P.RL.UI.Ptr.X = X;
	P.RL.UI.Ptr.Y = Y;
	P.RL.UI.Ptr.Pressed = true;
	P.RL.UI.Ptr.Down = false;
	P.RL.UI.Ptr.Released = true;
	const DrawList L = HubsHeld(P);
	P.Away();
	return Settle ? HubsHeld(P) : L;
}

std::string HubsMomentKey(const std::optional<ss::FestMoment>& M)
{
	return M ? std::to_string(M->Kind) + ":" + M->Key : std::string();
}

/** What one moment showed through its life (HubsPlayMoment). */
struct HubsMoment
{
	DrawList Settled;                 // two seconds after its first live frame
	DrawList Settled15;               // g1p-hub: the same frame at 1.5
	ss::FestMoment M;                 // the moment as the session handed it
	long long Vertices = 0, Runs = 0; // the card's cost over the page beneath
	int Champion = 0, Applause = 0, Bubble = 0;
	bool Still = false;  // the preview's still before its first live frame: settled, no cue, nothing granted
	bool Waited = false; // a click at 0.5 s closed nothing
	bool Closed = false; // TAKE A BOW (or a click anywhere) took it away
	std::string Next;    // the moment after ("Kind:Key", "" none)
};

/** One moment through its life on P (§2.8.5): the preview's still first; the first live frame (FirstClick: a one-frame click there,
 *  FestOneFrameClicks); a click at 0.5 s; at 2 s the page (Page through HubsPage, else a frame); then TAKE A BOW (Bow) or a click anywhere
 *  on the scrim, every frame held. Under: the page beneath, drawn just before the moment existed. */
HubsMoment HubsPlayMoment(HubsLaptop& P, const DrawList& Under, const char* Page, bool Bow, std::pair<float, float> FirstClick = {-1.0f, -1.0f})
{
	HubsMoment Out;
	const std::optional<ss::FestMoment> M = P.S.PeekFestMoment();
	if (!M)
	{
		ui_test::Expect(false, (std::string(Page ? Page : "a moment") + ": the session holds a moment to play").c_str());
		return Out;
	}
	Out.M = *M;
	const bool Mark = M->Kind == 4;
	const size_t Cues = P.H.Sounds.size();
	P.Away();
	ss::ui::rlfest_detail::HubPreviewing(P.RL, true);
	const DrawList Still = P.Frame(1.0f);
	ss::ui::rlfest_detail::HubPreviewing(P.RL, false);
	const std::string Figure = M->Kind == 2 ? ss::Grouped(M->Prize) + " pts" : ss::ui::rlnet_detail::NetMoney(M->Prize);
	Out.Still = P.H.Sounds.size() == Cues && HubsMomentKey(P.S.PeekFestMoment()) == HubsMomentKey(M) &&
		(Mark ? ui_test::DrewRun(Still, "Click anywhere to continue.") : ui_test::DrewRun(Still, "TAKE A BOW") && ui_test::DrewRun(Still, Figure));
	// The cues are counted around the moment's frames only (a step may bring a text's own Bubble); a tenth of a second first, so the
	// session's 50 ms rule never folds the moment's cue into one an earlier frame played.
	auto Count = [&Out, &P](size_t From) {
		Out.Champion += HubsCues(P.H, ss::SoundId::FestChampion, From);
		Out.Applause += HubsCues(P.H, ss::SoundId::CrowdApplause, From);
		Out.Bubble += HubsCues(P.H, ss::SoundId::Bubble, From);
	};
	P.Now = ui_test::Step(P.S, P.Now, 0.1);
	size_t From = P.H.Sounds.size();
	if (FirstClick.first >= 0.0f)
	{
		HubsHeldClick(P, FirstClick.first, FirstClick.second);
	}
	else
	{
		HubsHeld(P);
	}
	Count(From);
	P.Now = ui_test::Step(P.S, P.Now, 0.5);
	From = P.H.Sounds.size();
	HubsHeldClick(P, 300.0f, 300.0f);
	Count(From);
	Out.Waited = HubsMomentKey(P.S.PeekFestMoment()) == HubsMomentKey(M);
	P.Now = ui_test::Step(P.S, P.Now, 1.5);
	From = P.H.Sounds.size();
	Out.Settled = Page ? HubsPage(Page, P) : P.Frame(1.0f);
	Out.Settled15 = P.Frame(1.5f);
	Count(From);
	const HubsCost K = HubsCostOf(Out.Settled);
	const HubsCost U = HubsCostOf(Under);
	Out.Vertices = static_cast<long long>(K.Vertices) - static_cast<long long>(U.Vertices);
	Out.Runs = static_cast<long long>(K.Runs) - static_cast<long long>(U.Runs);
	// The close, with no frame after it: the next moment's first live frame is the next HubsPlayMoment's.
	if (Bow && !Mark)
	{
		HubsHeldClick(P, 800.0f, 703.0f, false); // TAKE A BOW {700, 680, 200, 46}
	}
	else
	{
		HubsHeldClick(P, 300.0f, 300.0f, false);
	}
	const std::optional<ss::FestMoment> After = P.S.PeekFestMoment();
	Out.Next = HubsMomentKey(After);
	Out.Closed = Out.Next != HubsMomentKey(M) || (After && After->Dev != M->Dev);
	std::printf("  %-22s kind %d %s: card %lld vertices, %lld runs over the page (moments <= 10000/60, the mark card <= 4000/30); cues %d/%d/%d\n",
		Page ? Page : "(moment)", M->Kind, M->Key.c_str(), Out.Vertices, Out.Runs, Out.Champion, Out.Applause, Out.Bubble);
	return Out;
}

/** FH-6a: FEIGN 2026's three dev moments on its Main day (§2.16 step 7; R-7), career A over Ignite's broadcast: the Main's, another
 *  event's and the belt's, each once with one cue (FestChampion with CrowdApplause), DEV beside its numbers, a still while previewing,
 *  within the moments' budget; TAKE A BOW keeps no key. FestOneFrameClicks: a click at 0.5 s closes nothing. */
void FestMomentsFeign(HubsLaptop& P, const ss::fest::Festival& Feign)
{
	const char* const Kinds[3] = {"main", "event", "belt"};
	const char* const Pages[3] = {"fest_moment_main", "fest_moment_event", "fest_moment_belt"};
	const ss::world::World& W = P.S.Living();
	// Kind 3's line: the edition's first numbered event (never a satellite), as ss.Fest.Moment reads its prize.
	std::string EventName;
	const ss::net::Network& Net = ss::net::ForEvent(Feign.MainId);
	for (const ss::net::EventInstance& E : Net.Window(static_cast<double>(Feign.FirstDay) * ss::net::MinutesPerDay, static_cast<double>(Feign.LastDay + 1) * ss::net::MinutesPerDay))
	{
		const ss::net::EventTemplate& T = Net.TemplateOf(E);
		if (EventName.empty() && T.Series == Feign.SeriesId && std::find(Feign.Satellites.begin(), Feign.Satellites.end(), T.Id) == Feign.Satellites.end())
		{
			EventName = Net.NameOf(E);
		}
	}
	for (int I = 0; I < 3; ++I)
	{
		P.Away();
		const DrawList Under = P.Frame(1.0f);
		const std::string Ack = P.S.DebugFest(std::string("moment feign ") + Kinds[I]);
		ui_test::Expect(Ack.find("plays next") != std::string::npos, (std::string(Pages[I]) + ": ss.Fest.Moment queues FEIGN 2026's dev moment (" + Ack + ")").c_str());
		const HubsMoment Mo = HubsPlayMoment(P, Under, Pages[I], true);
		const std::string Title = I == 2 ? "PLAYER OF THE SERIES" : "CHAMPION";
		const std::string Line = I == 0 ? Feign.Name + " Main Event" : I == 1 ? EventName : Feign.Name;
		ui_test::Expect(Mo.M.Dev && Mo.M.Kind == (I == 0 ? 1 : I == 1 ? 3 : 2) && ui_test::DrewRun(Mo.Settled, Title) && ui_test::DrewRun(Mo.Settled, Line) &&
				ui_test::DrewRun(Mo.Settled, "DEV"),
			(std::string(Pages[I]) + ": " + Title + ", U18's line (\"" + Line + "\") and DEV beside the numbers").c_str());
		if (I == 0)
		{
			const bool Uncrowned = ss::world::FestRaceKey(W) == Feign.SeriesId && ss::world::FestCrownedAt(W, Feign.SeriesId) < 0.0;
			ui_test::Expect(ui_test::DrewRun(Mo.Settled, "The trophy goes on the shelf at home.") &&
					ui_test::DrewRun(Mo.Settled, "The belt is decided at the crown, later tonight.") == Uncrowned,
				"fest_moment_main: U29's shelf line beneath the Main's card, and the belt line exactly while FEIGN 2026's race is uncrowned");
		}
		else
		{
			ui_test::Expect(!ui_test::Drew(Mo.Settled, "shelf at home") && !ui_test::Drew(Mo.Settled, "decided at the crown"), (std::string(Pages[I]) + ": U29's lines only beneath a Main's card").c_str());
		}
		ui_test::Expect(Mo.Champion == 1 && Mo.Applause == 1 && Mo.Bubble == 0, (std::string("FestPreviewStill (one sting a moment): ") + Pages[I] + " plays FestChampion and CrowdApplause once").c_str());
		ui_test::Expect(Mo.Still, (std::string("FestPreviewStill (moments): ") + Pages[I] + "'s still is settled, silent, and leaves the moment pending").c_str());
		ui_test::Expect(Mo.Waited, (std::string("FestOneFrameClicks (moments): a click before 1.6 s closes nothing (") + Pages[I] + ")").c_str());
		ui_test::Expect(Mo.Closed && Mo.Next.empty(), (std::string(Pages[I]) + ": TAKE A BOW closes the moment").c_str());
		// g1p-hub item 8 (PIE s07_moment_feign_b, s07_moment_belt: the line over Ignite's FUSE at y 912): U19 stands on the champion card
		// {500, 170, 600, 580}, under TAKE A BOW (to y 726), at both scales. Plant: the old y 912.
		int OnCard = 0;
		for (int K = 0; K < 2; ++K)
		{
			const DrawList& Lc = K == 0 ? Mo.Settled : Mo.Settled15;
			for (const ss::ui::DrawCmd& Cmd : Lc.Cmds)
			{
				if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == "Click anywhere to continue.")
				{
					const HubsBox B = HubsRunBox(Cmd);
					OnCard += B.X0 >= 516.0f && B.X1 <= 1084.0f && B.Y0 >= 727.0f && B.Y1 <= 749.0f ? 1 : 0;
				}
			}
		}
		ui_test::Expect(OnCard == 2, (std::string(Pages[I]) + ": \"Click anywhere to continue.\" stands on the card under TAKE A BOW, clear of Ignite's FUSE and dock, at both scales").c_str());
		ui_test::Expect(Mo.Vertices <= 10000 && Mo.Runs <= 60, (std::string(Pages[I]) + ": the moment within 10k vertices and 60 runs over the page (\xC2\xA7" "2.8.10)").c_str());
	}
	ui_test::Expect(P.S.Life.Sites.Done.count("festbelt:" + Feign.SeriesId) == 0, "R-7: the DEV belt keeps no festbelt: key");
}

/** FestTitleMoment (§2.15.2, v1.6): a championship's Mini title (Kind 5, ss.Fest.Moment <brand> title) plays once with U18's tier line,
 *  one cue, a still while previewing, <= 10k vertices over the page; Page emits it. Plant: the title moment playing twice for one title. */
void FestTitleMomentHubs(HubsLaptop& P, const ss::fest::Festival& F, const char* Brand, const char* Page)
{
	P.Away();
	const DrawList Under = P.Frame(1.0f);
	const std::string Ack = P.S.DebugFest(std::string("moment ") + Brand + " title");
	ui_test::Expect(Ack.find("plays next") != std::string::npos, ("FestTitleMoment: ss.Fest.Moment queues " + F.Name + "'s title (" + Ack + ")").c_str());
	const HubsMoment Mo = HubsPlayMoment(P, Under, Page, true);
	const std::string Line = F.Name + " \xC2\xB7 Mini Main Event";
	ui_test::Expect(Mo.M.Kind == 5 && Mo.M.Mark == 1 && ui_test::DrewRun(Mo.Settled, "CHAMPION") && ui_test::DrewRun(Mo.Settled, Line),
		("FestTitleMoment: " + F.Name + "'s Mini title shows CHAMPION and \"" + Line + "\" (U18)").c_str());
	const size_t Cues = P.H.Sounds.size();
	P.Now = ui_test::Step(P.S, P.Now, 0.5);
	const DrawList After = HubsHeld(P);
	P.Now = ui_test::Step(P.S, P.Now, 2.0);
	const DrawList Later = HubsHeld(P);
	ui_test::Expect(Mo.Closed && !P.S.PeekFestMoment() && !ui_test::DrewRun(After, Line) && !ui_test::DrewRun(Later, "TAKE A BOW") && Mo.Champion == 1 &&
			HubsCues(P.H, ss::SoundId::FestChampion, Cues) == 0,
		("FestTitleMoment: " + F.Name + "'s title plays once (one cue, gone after TAKE A BOW)").c_str());
	ui_test::Expect(Mo.Still, ("FestTitleMoment: " + F.Name + "'s title is a still while previewing").c_str());
	ui_test::Expect(Mo.Vertices <= 10000 && Mo.Runs <= 60, ("FestTitleMoment: " + F.Name + "'s title within 10k vertices and 60 runs over the page").c_str());
	if (F.Which == ss::fest::Brand::Reign)
	{
		std::printf("FestReignCopy (REIGN's title moment): %d sponsor brands looked for\n", HubsSponsorScan(Mo.Settled, "REIGN's title moment"));
	}
}

/** U36 (§2.8.5, v1.6): RCOP 2027's Main moment (a dev one, day 400) adds "2nd edition": RCOP 2026 was this world's first. */
void FestMainEditionTwo(HubsLaptop& P)
{
	const ss::fest::Festival* Rcop27 = ss::fest::FestivalOf("rcop27");
	if (!Rcop27)
	{
		ui_test::Expect(false, "U36: RCOP 2027 is F-net's row");
		return;
	}
	P.Away();
	const DrawList Under = P.Frame(1.0f);
	P.S.DebugFest("moment rcop main");
	const HubsMoment Mo = HubsPlayMoment(P, Under, nullptr, true);
	const std::string Line = Rcop27->Name + " Main Event \xC2\xB7 2nd edition";
	ui_test::Expect(Mo.M.Kind == 1 && Mo.M.Key == Rcop27->SeriesId && ui_test::DrewRun(Mo.Settled, Line) && Mo.Closed,
		("U36: RCOP 2027's Main moment reads \"" + Line + "\"").c_str());
}

/** FestPinsReveal's after half (§2.8.7), career B on RCOP 2026's first day after the hero's first RCOP event: the wall shows "FESTIVAL
 *  PINS · 2026", within 4k vertices and 30 runs, and hovering RCOP's slot prints its facts. */
void FestPinsRevealAfter(HubsLaptop& P, const ss::fest::Festival& Rcop)
{
	auto Wall = [&P]() {
		ui_test::TableMeasurer Ms;
		DrawList L;
		ss::ui::Canvas C(L, Ms, RiverLine::Width, RiverLine::Height, 1.0f);
		ss::ui::rlfest_detail::HubPinsDraw(P.RL, C, P.Now);
		return L;
	};
	P.Away();
	const DrawList L = Wall();
	const HubsCost K = HubsCostOf(L);
	ui_test::Expect(ui_test::DrewRun(L, "FESTIVAL PINS \xC2\xB7 2026"), "FestPinsReveal: after the hero's first festival event the wall shows FESTIVAL PINS \xC2\xB7 2026");
	ui_test::Expect(K.Vertices <= 4000 && K.Runs <= 30, "FestPinsReveal: the pin wall within 4k vertices and 30 runs (\xC2\xA7" "2.8.10)");
	std::printf("  fest pin wall (career B, day %d): %zu vertices, %zu runs (<= 4000/30)\n", ss::net::DayOf(P.S.WorldMinutes()), K.Vertices, K.Runs);
	P.RL.UI.Ptr.Active = true;
	P.RL.UI.Ptr.X = 1092.0f + 12.0f + 47.0f * 8.0f + 20.0f; // RCOP's slot (calendar order: WRIT ... RCOP, FEIGN)
	P.RL.UI.Ptr.Y = 656.0f + 44.0f + 20.0f;
	const DrawList Hover = Wall();
	P.Away();
	ui_test::Expect(ui_test::Drew(Hover, Rcop.Name + " \xC2\xB7 "), ("FestPinsReveal: hovering RCOP's slot prints its facts (" + Rcop.Name + " \xC2\xB7 ...)").c_str());
}

/** FestMarkMoment (§2.15.2; PLAN_G1 item 16) on a fresh career: one world\tfest\tmark line a grade (§2.13's format: key, mark, rank,
 *  scorers, points), the belt's too, played through the keyed path: the belt first, then TOP 25%, TOP 10%, FINALIST and PODIUM, each once
 *  (festbelt:/festmark: keys), across a reload; a reload before the belt's dismiss plays it again. Each mark card: U8's word, the edition,
 *  U28's rank line, the ticket line for FINALIST and PODIUM, the Bubble cue alone, a still while previewing, <= 4k vertices and 30 runs.
 *  FINALIST is the page fest_moment_mark. Plant: the done key unwritten. Also FestPinsReveal's before half (nothing in the band on the
 *  day-0 Career page; the wall hidden) and FestOneFrameClicks' moments half (a one-frame click on the moment's first frame fires nothing
 *  beneath; plant: the moment drawn without blocking). */
void FestMarkMomentHubs()
{
	namespace world = ss::world;
	HubsLaptop A("u4-fest-marks");
	A.H.Keep = true;
	A.Away();
	// FestPinsReveal, before: the day-0 Career page draws nothing in the wall's band, and the wall itself draws nothing.
	A.RL.OpenPage(RiverLine::Page::Career, A.Now);
	const DrawList Career = A.Frame(1.0f);
	int InBand = HubsRunsIn(Career, 1092.0f, 656.0f, 1576.0f, 760.0f);
	for (const ss::ui::Vertex& V : Career.Vertices)
	{
		InBand += V.X > 1092.5f && V.X < 1575.5f && V.Y > 656.5f && V.Y < 759.5f ? 1 : 0;
	}
	ui_test::Expect(InBand == 0, ("FestPinsReveal: nothing draws in the pin wall's band on the day-0 Career page (" + std::to_string(InBand) + " runs and vertices)").c_str());
	{
		ui_test::TableMeasurer Ms;
		DrawList L;
		ss::ui::Canvas C(L, Ms, RiverLine::Width, RiverLine::Height, 1.0f);
		ss::ui::rlfest_detail::HubPinsDraw(A.RL, C, A.Now);
		ui_test::Expect(L.Cmds.empty() && L.Vertices.empty(), "FestPinsReveal: the pin wall is hidden before the first festival event");
	}
	A.RL.OpenPage(RiverLine::Page::Lobby, A.Now);
	A.Frame(1.0f);
	A.Now = ui_test::Step(A.S, A.Now, 1.0); // the lobby settled (its first frame after an open is not the page beneath a moment)
	const DrawList Under = A.Frame(1.0f);
	// FH-7, FestHubBudgets: Night One's lobby, the lobby row's measured cost (29.9k at 1.0), at both scales.
	HubsBook().LobbyBase[0] = Under.Vertices.size();
	HubsBook().LobbyBase[1] = A.Frame(1.5f).Vertices.size();
	ui_test::Expect(!A.S.PeekFestMoment(), "FestMarkMoment: a fresh career holds no moment");
	const std::pair<float, float> Nav = HubsRunAt(Under, "Career");
	const std::string Selected = A.RL.SelectedEvent();
	struct Plant
	{
		const char* Key;
		world::FestMark Mark;
		int Rank, Scorers, Points;
		const char* Word;
		bool Ticket;
	};
	const Plant Plants[5] = {{"rcop", world::FestMark::Title, 1, 608, 1700, "PLAYER OF THE SERIES", false},
		{"ignite.feign26", world::FestMark::Top25, 110, 500, 640, "TOP 25%", false}, {"runittwice.writ27", world::FestMark::Top10, 40, 500, 1210, "TOP 10%", false},
		{"runittwice.grit27", world::FestMark::Finalist, 9, 500, 2380, "FINALIST", true}, {"slam27", world::FestMark::Podium, 2, 500, 3900, "PODIUM", true}};
	for (const Plant& Pl : Plants)
	{
		A.S.Living().Read({"world", "fest", "mark", Pl.Key, std::to_string(static_cast<int>(Pl.Mark)), std::to_string(Pl.Rank), std::to_string(Pl.Scorers), std::to_string(Pl.Points)});
	}
	ui_test::Expect(world::HeroMarks(A.S.Living()).size() == 5, "FestMarkMoment: the five mark lines read into the world (World's HeroFest)");
	// The belt first; its first frame takes a one-frame click on the nav's Career (FestOneFrameClicks).
	ui_test::Expect(HubsMomentKey(A.S.PeekFestMoment()) == "2:rcop", "FestMarkMoment: the belt's moment comes before the marks'");
	A.S.Save();
	{
		ss::SaveData Back;
		const bool Parsed = ss::SaveData::Parse(A.H.Last.Serialize(), Back);
		HubsLaptop R0("u4-fest-marks", &Back);
		HubsHeld(R0);
		R0.Now = ui_test::Step(R0.S, R0.Now, 1.0); // the title is staged from 0.4 s
		const DrawList L0 = HubsHeld(R0);
		std::printf("  the reload before the belt's dismiss: moment %s, screen %d, skip %d, outcome %d\n", HubsMomentKey(R0.S.PeekFestMoment()).c_str(),
			static_cast<int>(R0.S.CurrentScreen), R0.S.TimeSkip.Active ? 1 : 0, R0.S.HasOutcome ? 1 : 0);
		ui_test::Expect(Parsed && ui_test::DrewRun(L0, "PLAYER OF THE SERIES") && HubsMomentKey(R0.S.PeekFestMoment()) == "2:rcop",
			"FestMarkMoment: a reload before the belt's dismiss plays the belt again (it wasn't seen)");
	}
	const HubsMoment Belt = HubsPlayMoment(A, Under, nullptr, true, {Nav.first + 10.0f, Nav.second - 6.0f});
	ui_test::Expect(Nav.second > 0.0f && A.RL.CurrentPage() == RiverLine::Page::Lobby && A.RL.SelectedEvent() == Selected,
		"FestOneFrameClicks (moments): a one-frame click on the moment's first frame, and every click while it is up, fires nothing beneath");
	ui_test::Expect(ui_test::DrewRun(Belt.Settled, "PLAYER OF THE SERIES") && ui_test::DrewRun(Belt.Settled, "1,700 pts") && !ui_test::DrewRun(Belt.Settled, "DEV") &&
			Belt.Champion == 1 && Belt.Applause == 1,
		"FestMarkMoment: RCOP 2026's belt plays first, its points, no DEV, one cue");
	ui_test::Expect(Belt.Closed && A.S.Life.Sites.Done.count("festbelt:rcop") == 1 && Belt.Next == "4:ignite.feign26", "FestMarkMoment: TAKE A BOW keys the belt (festbelt:rcop) and TOP 25% follows");
	// The marks: two on this laptop, then a reload, then two more.
	auto PlayMark = [&Under](HubsLaptop& P, const Plant& Pl, const char* Page, const std::string& Next) {
		const ss::fest::Festival* F = ss::fest::FestivalOf(Pl.Key);
		const HubsMoment Mo = HubsPlayMoment(P, Under, Page, false);
		const std::string Rank = ss::Ordinal(Pl.Rank) + " of " + ss::Grouped(Pl.Scorers) + " in the race";
		const std::string What = std::string("FestMarkMoment (") + Pl.Word + "): ";
		ui_test::Expect(Mo.M.Kind == 4 && F && ui_test::DrewRun(Mo.Settled, Pl.Word) && ui_test::DrewRun(Mo.Settled, F->Name) && ui_test::DrewRun(Mo.Settled, Rank) &&
				ui_test::DrewRun(Mo.Settled, "A seat in the next Main Event") == Pl.Ticket && !ui_test::DrewRun(Mo.Settled, "TAKE A BOW"),
			(What + "the card shows the mark, the edition, \"" + Rank + "\" and the ticket line only when one was earned").c_str());
		ui_test::Expect(Mo.Bubble == 1 && Mo.Champion == 0 && Mo.Applause == 0, (What + "the Bubble cue once, no sting").c_str());
		ui_test::Expect(Mo.Still, (What + "a still while previewing").c_str());
		ui_test::Expect(Mo.Waited, (What + "a click before 1.6 s closes nothing").c_str());
		ui_test::Expect(Mo.Vertices <= 4000 && Mo.Runs <= 30, (What + "the mark card within 4k vertices and 30 runs over the page").c_str());
		ui_test::Expect(Mo.Closed && Mo.Next == Next && P.S.Life.Sites.Done.count(std::string("festmark:") + Pl.Key) == 1,
			(What + "a click closes it once, keyed festmark:" + Pl.Key + ", and the next follows (" + (Next.empty() ? std::string("none") : Next) + ")").c_str());
	};
	PlayMark(A, Plants[1], nullptr, "4:runittwice.writ27");
	PlayMark(A, Plants[2], nullptr, "4:runittwice.grit27");
	ss::SaveData Back;
	const bool Parsed = ss::SaveData::Parse(A.H.Last.Serialize(), Back);
	HubsLaptop B("u4-fest-marks", &Back);
	B.H.Keep = true;
	ui_test::Expect(Parsed && HubsMomentKey(B.S.PeekFestMoment()) == "4:runittwice.grit27",
		"FestMarkMoment: after a reload the belt, TOP 25% and TOP 10% stay played (their keys saved); FINALIST is next");
	PlayMark(B, Plants[3], "fest_moment_mark", "4:slam27");
	PlayMark(B, Plants[4], nullptr, "");
	const std::set<std::string>& Done = B.S.Life.Sites.Done;
	ui_test::Expect(Done.count("festbelt:rcop") && Done.count("festmark:ignite.feign26") && Done.count("festmark:runittwice.writ27") &&
			Done.count("festmark:runittwice.grit27") && Done.count("festmark:slam27"),
		"FestMarkMoment: every grade's done key is kept");
}

/** FestMarkMoment's FINALIST case on F-world's fixture (PLAN_G1 item 16; FW-7's Tests/fixtures/update4_festival.ssave: day 60, the hero
 *  RCOP 2026's FINALIST, its moment seen, festmark:rcop): loaded as it is, nothing plays (the key kept); with the key taken out of a
 *  copy, RCOP 2026's FINALIST card plays once (U8's word, the edition, U28's rank line, the crown ticket's line, the Bubble cue alone),
 *  keyed festmark:rcop on its close, and a save and reload plays nothing again. */
void FestMarkMomentFixture()
{
	const std::string Path = std::string(SS_TESTS_DIR) + "/fixtures/update4_festival.ssave";
	std::string Saved;
	if (std::FILE* F = std::fopen(Path.c_str(), "rb"))
	{
		char Buf[65536];
		size_t N = 0;
		while ((N = std::fread(Buf, 1, sizeof(Buf), F)) > 0)
		{
			Saved.append(Buf, N);
		}
		std::fclose(F);
	}
	ss::SaveData D;
	const bool Parsed = !Saved.empty() && ss::SaveData::Parse(Saved, D);
	ui_test::Expect(Parsed && D.Life.Sites.Done.count("festmark:rcop") == 1, "FestMarkMoment (fixture): update4_festival.ssave reads, RCOP 2026's mark seen (festmark:rcop)");
	if (!Parsed)
	{
		return;
	}
	// The page beneath: the fixture's lobby as it is (the key kept, so no moment draws over it), settled.
	DrawList Under;
	{
		HubsLaptop Seen("u4-fest-fixture", &D);
		const std::vector<ss::world::HeroFestMark>& Marks = ss::world::HeroMarks(Seen.S.Living());
		ui_test::Expect(Marks.size() == 1 && Marks[0].Key == "rcop" && Marks[0].Mark == ss::world::FestMark::Finalist && HubsMomentKey(Seen.S.PeekFestMoment()).empty(),
			("FestMarkMoment (fixture): the hero is RCOP 2026's FINALIST and, the key kept, no moment plays (" + HubsMomentKey(Seen.S.PeekFestMoment()) + ")").c_str());
		Seen.Away();
		Seen.RL.SiteShown = ss::net::SiteId::RiverLine;
		Seen.RL.OpenPage(RiverLine::Page::Lobby, Seen.Now);
		Seen.Frame(1.0f);
		Seen.Now = ui_test::Step(Seen.S, Seen.Now, 1.0);
		Under = Seen.Frame(1.0f);
	}
	ss::SaveData Unseen = D;
	Unseen.Life.Sites.Done.erase("festmark:rcop");
	HubsLaptop P("u4-fest-fixture", &Unseen);
	P.H.Keep = true;
	ui_test::Expect(HubsMomentKey(P.S.PeekFestMoment()) == "4:rcop", ("FestMarkMoment (fixture): with festmark:rcop taken out, RCOP 2026's mark is up (" +
		HubsMomentKey(P.S.PeekFestMoment()) + ")").c_str());
	// The lobby opened and a second on, no frame drawn: the moment's first live frame is HubsPlayMoment's.
	P.Away();
	P.RL.SiteShown = ss::net::SiteId::RiverLine;
	P.RL.OpenPage(RiverLine::Page::Lobby, P.Now);
	P.Now = ui_test::Step(P.S, P.Now, 1.0);
	const ss::fest::Festival* Rcop = ss::fest::FestivalOf("rcop");
	const std::vector<ss::world::HeroFestMark> Marks = ss::world::HeroMarks(P.S.Living());
	const HubsMoment Mo = HubsPlayMoment(P, Under, nullptr, false);
	const std::string Rank = Marks.empty() ? std::string("?") : ss::Ordinal(Marks[0].Rank) + " of " + ss::Grouped(Marks[0].Scorers) + " in the race";
	ui_test::Expect(Mo.M.Kind == 4 && Rcop && ui_test::DrewRun(Mo.Settled, "FINALIST") && ui_test::DrewRun(Mo.Settled, Rcop->Name) && ui_test::DrewRun(Mo.Settled, Rank) &&
			ui_test::DrewRun(Mo.Settled, "A seat in the next Main Event") && Mo.Bubble == 1 && Mo.Champion == 0 && Mo.Applause == 0 && Mo.Still,
		("FestMarkMoment (fixture): RCOP 2026's FINALIST card, \"" + Rank + "\", the crown ticket's line, the Bubble cue once, a still while previewing").c_str());
	ui_test::Expect(Mo.Vertices <= 4000 && Mo.Runs <= 30, ("FestMarkMoment (fixture): the mark card within 4k vertices and 30 runs over the lobby (" + std::to_string(Mo.Vertices) + ", " +
		std::to_string(Mo.Runs) + ")").c_str());
	ui_test::Expect(Mo.Closed && Mo.Next.empty() && P.S.Life.Sites.Done.count("festmark:rcop") == 1, "FestMarkMoment (fixture): a click closes it once, keyed festmark:rcop");
	P.S.Save();
	ss::SaveData Back;
	const bool Again = ss::SaveData::Parse(P.H.Last.Serialize(), Back);
	HubsLaptop R("u4-fest-fixture", &Back);
	ui_test::Expect(Again && HubsMomentKey(R.S.PeekFestMoment()).empty(), "FestMarkMoment (fixture): after a save and reload FINALIST stays played");
	std::printf("  FestMarkMoment (fixture): day %d, RCOP 2026 FINALIST, %s; card %lld vertices, %lld runs over the lobby\n", ss::net::DayOf(P.S.WorldMinutes()), Rank.c_str(),
		Mo.Vertices, Mo.Runs);
}

/** FestHubBudgets' summary (§2.8.10, §8.5; Q15), once every career has run: every host's hub drawn in each of the five states, emitted
 *  or not; the lobby with festival crests inside Night One's measured lobby + 2k at each scale (and 29.9k + 2k at 1.0); Ignite's floor
 *  chip (chip 10's face, on and off, at both scales, with the shelf's label and sub-line: two runs, no vertices) <= 1k; the Career page
 *  with the hero's festival pieces inside its cost before them + 4k. */
void FestHubBudgetsSummary()
{
	const HubsBudgetBook& Book = HubsBook();
	const ss::net::SiteId Hosts[3] = {ss::net::SiteId::RiverLine, ss::net::SiteId::Ignite, ss::net::SiteId::RunItTwice};
	const ss::fest::Phase States[5] = {ss::fest::Phase::Buildup, ss::fest::Phase::Live, ss::fest::Phase::MainDay, ss::fest::Phase::Closing, ss::fest::Phase::Afterglow};
	const char* StateNames[5] = {"Buildup", "Live", "MainDay", "Closing", "Afterglow"};
	int Covered = 0;
	for (const ss::net::SiteId H : Hosts)
	{
		std::string Line;
		for (int I = 0; I < 5; ++I)
		{
			const auto It = Book.States.find({static_cast<int>(H), static_cast<int>(States[I])});
			const bool Has = It != Book.States.end();
			Covered += Has ? 1 : 0;
			Line += std::string(" ") + StateNames[I] + "=" + (Has ? It->second : std::string("-"));
			ui_test::Expect(Has, ("FestHubBudgets: " + ss::ui::rlsites_detail::SitesShortName(H) + "'s hub is drawn in " + StateNames[I] + " (a page or a drawn state)").c_str());
		}
		std::printf("  FestHubBudgets %-10s%s\n", ss::ui::rlsites_detail::SitesShortName(H).c_str(), Line.c_str());
	}
	std::printf("  FestHubBudgets: %d frames measured, %d of 15 host states covered, %d warm draws timed (worst %.2f of its bound, %s)\n", Book.Measured, Covered, Book.Timed,
		Book.WorstMs, Book.WorstAt.empty() ? "none: SS_PERF unset" : Book.WorstAt.c_str());
	// The lobby rows with festival crests.
	ui_test::Expect(Book.LobbyBase[0] > 0 && !Book.Lobbies.empty(), "FestHubBudgets: Night One's lobby and the festival lobbies were measured");
	for (const auto& L : Book.Lobbies)
	{
		std::printf("  FestHubBudgets lobby %-30s %zu / %zu vertices (Night One's %zu / %zu + 2000; 31900 at 1.0)\n", L.first.c_str(), L.second.first, L.second.second, Book.LobbyBase[0],
			Book.LobbyBase[1]);
		ui_test::Expect(L.second.first <= Book.LobbyBase[0] + 2000 && L.second.second <= Book.LobbyBase[1] + 2000 && L.second.first <= 31900,
			("FestHubBudgets: the lobby with festival crests (" + L.first + ") inside its measured cost + 2k (" + std::to_string(L.second.first) + " / " +
				std::to_string(L.second.second) + ")").c_str());
	}
	// Ignite's floor chip: chip 10's face (on, the busiest, and off) at both scales; the shelf's label and sub-line add two runs.
	const ss::fest::Festival* Feign = ss::fest::FestivalOf("ignite.feign26");
	const ss::net::SeriesInfo* Sr = Feign ? ss::net::Site(ss::net::SiteId::Ignite).FindSeries(Feign->SeriesId) : nullptr;
	size_t Chip = 0;
	for (int Si = 0; Si < 4 && Sr; ++Si)
	{
		ui_test::TableMeasurer M;
		DrawList L;
		ss::ui::Canvas C(L, M, RiverLine::Width, RiverLine::Height, Si % 2 == 0 ? 1.0f : 1.5f);
		const std::string Short = Sr->Short.empty() ? std::string("FEIGN") : Sr->Short;
		ss::ui::rlignite_detail::IgniteFestChipFace(C, *Sr, {120.0f, 440.0f, C.Measure(Short, 13.5f, 700) + 56.0f, 32.0f}, Si < 2, false, Sr->Color, 0xff6b57u, 0x5a4060u);
		Chip = std::max(Chip, L.Vertices.size());
	}
	std::printf("  FestHubBudgets: Ignite's floor chip and label, %zu vertices at most (<= 1000)\n", Chip);
	ui_test::Expect(Sr && Chip > 0 && Chip <= 1000, ("FestHubBudgets: Ignite's floor chip and label within 1k vertices (" + std::to_string(Chip) + ")").c_str());
	// The Career page with the hero's festival pieces.
	std::printf("  FestHubBudgets: the Career page %zu / %zu vertices before the hero's festival pieces, %zu / %zu with them (<= +4000)\n", Book.CareerBefore[0], Book.CareerBefore[1],
		Book.CareerAfter[0], Book.CareerAfter[1]);
	ui_test::Expect(Book.CareerBefore[0] > 0 && Book.CareerAfter[0] > 0 && Book.CareerAfter[0] <= Book.CareerBefore[0] + 4000 && Book.CareerAfter[1] <= Book.CareerBefore[1] + 4000,
		"FestHubBudgets: the Career page with the hero's festival pieces inside its cost before them + 4k");
}
} // namespace

namespace u4_ui_test
{
void SegFest()
{
	const auto T0 = std::chrono::steady_clock::now();
	FestHubPagesRiverLine();
	FestPopoverHubs();
	FestStrikeGateHubs();
	FestCopyUiHubs();
	// FH-2: the real calendar in three frames.
	FestStripTenMeasures();
	FestLockedMainWallet();
	// g1p-hub: the banner's stats and the Main card on every edition of 2026-2028 (the drawn halves sit in the careers).
	{
		const auto G0 = std::chrono::steady_clock::now();
		FestBannerStats();
		FestMainCardFits();
		std::printf("  g1p-hub: FestBannerStats and FestMainCardFits %.2f s\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - G0).count());
	}
	FestCareerIgnite();
	FestCareerRiverLine();
	// FH-6a: the marks through the keyed path, a third career.
	FestMarkMomentHubs();
	// FH-7: FestMarkMoment's FINALIST case on F-world's fixture, then FestHubBudgets' summary over everything measured above.
	FestMarkMomentFixture();
	FestHubBudgetsSummary();
	std::printf("u4_ui SegFest (F-hubs A): %.1fs\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
}
} // namespace u4_ui_test
