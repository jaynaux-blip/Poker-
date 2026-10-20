// riverline_ui: ui-laptop's additions (next_segment_spec §7.6, §8.3), compiled into ui_test and called from main() in
// ui_main.cpp (the shared kit is UiSeg.h):
// - the taskbar with 8 apps (9 from update 2's G2), Mail opened from it (core-sponsor's mail_* goldens run once it is);
// - bank_segment: 9 In and 8 Out rows at the 26-unit pitch, the ledger's kinds 10 to 12 by name, the staked-roll and
//   store-credit cards that never count toward Net;
// - the Backing tab (hidden until stake::Anything), the nav with four table tabs and Backing (nav_four_tables_backing);
// - RiverLine's presets from core's sizing; the results screen's split;
// - the Embercrest card's funding (embercrest_card_backed, embercrest_card_covered), its Sell action, the STRUCTURE and
//   PAYOUTS flip, the LEADERS page;
// - Sam on the Burner (burner_sam_offer, burner_sam_debt);
// - badges: the board rows' glyph and hover card, the player card's rail and TROPHIES & BADGES, Career's YOUR BADGES
//   (badges_board_live, badges_board_honors, badges_board_hover, badges_card_rail, badges_card_tab,
//   badges_career_progress), HONORS and CIRCUIT CARDS;
// - the second screen's COMING UP and the wall calendar's marks;
// - the copy lint over this track's sentences;
// - update 2, G2 (§4.8, §4.9, §1.2, §5.11): Rerun the ninth app after Mail, the news wire's lane at 1,600 wide with the
//   status at its widest (taskbar_rerun) and "Your turn at ..." whole in it, Rerun's marigold dot (taskbar_rerun_dot; a
//   new laptop where the last one was starts its own count), the Bank's "Streaming & video" row
//   for an old save and with a Rerun payout (bank_video_old_save, bank_video), Mail's button to the Channel page
//   (mail_rerun, mail_rerun_opened), "Rerun payout" on the second screen and the wall calendar (second_screen_rerun,
//   wall_calendar_rerun).
// Checks that need the badges' rules (core-badges) say so and skip while those are stubs.
#include "UiSeg.h"

#include "CopyLint.h"
#include "ShortStack/Game/Badges.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Sizing.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/Stake.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/UI/FrontEnd.h"
#include "ShortStack/UI/RiverLine.h"
#include "ShortStack/UI/SecondScreen.h"
#include "ShortStack/UI/VideoArt.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <new>
#include <set>
#include <sstream>

namespace ui_test
{
namespace laptop_seg
{
using App = ss::ui::RiverLine::App;
using Page = ss::ui::RiverLine::Page;
using ss::ui::DrawList;
constexpr double Day = 1440.0;

struct GoHooks : QuietHooks
{
	bool GoOut(const std::string&, ss::Chips) override { return true; }
};

/** A session and its laptop, the pointer driven by hand. */
struct Laptop
{
	GoHooks H;
	ss::Session S;
	ss::ui::RiverLine RL;
	TableMeasurer M;
	double Now = 1.0;
	explicit Laptop(const std::string& Seed) : S(H, Seed), RL(S)
	{
		S.CurrentScreen = ss::Screen::Lobby;
		Now = Step(S, Now, 0.2);
	}
	/** A career loaded from a save (a fixture). */
	Laptop(const std::string& Seed, const ss::SaveData& Loaded) : S(H, Seed, &Loaded), RL(S)
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
	/** The page settled (its fade-ins done), the pointer at (X, Y) or away. */
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
	float Measure(const std::string& Text, float Size, int Weight) const
	{
		DrawList L;
		ss::ui::Canvas C(L, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, 1.0f);
		return C.Measure(Text, Size, Weight);
	}
};

/** A text run that is exactly Text with its baseline in [MinY, MaxY]: its left, baseline and width. */
struct Box
{
	bool Found = false;
	float X = 0.0f;
	float Base = 0.0f;
	float W = 0.0f;
	float Size = 0.0f;
};
Box RunOf(const DrawList& L, const std::string& Text, const TableMeasurer& M, float MinY = -1.0e9f, float MaxY = 1.0e9f)
{
	Box Out;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text && Cmd.Text.BaselineY >= MinY && Cmd.Text.BaselineY <= MaxY)
		{
			Out.Found = true;
			Out.X = Cmd.Text.X;
			Out.Base = Cmd.Text.BaselineY;
			Out.Size = Cmd.Text.SizePx;
			Out.W = M.Width(Text, Cmd.Text.Face, Cmd.Text.SizePx);
			return Out;
		}
	}
	return Out;
}

/** Words drawn a letter at a time (NetSpaced's tracked capitals), joined back up: one string per line of them. */
std::vector<std::string> Spaced(const DrawList& L)
{
	std::vector<std::string> Out;
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
			continue;
		}
		if (!Cur.empty())
		{
			Out.push_back(Cur);
		}
		Cur = Glyph ? T : std::string();
		Base = Glyph ? Cmd.Text.BaselineY : -1.0e9f;
	}
	if (!Cur.empty())
	{
		Out.push_back(Cur);
	}
	return Out;
}

/** Tracked capitals that spell Word (on one line). */
bool Spelled(const DrawList& L, const std::string& Word)
{
	for (const std::string& S : Spaced(L))
	{
		if (S.find(Word) != std::string::npos)
		{
			return true;
		}
	}
	return false;
}

/** Whether the badges' rules are in (core-badges): a label for a badge any build can name. */
bool BadgesIn()
{
	ss::badge::Badge B;
	B.K = ss::badge::Kind::VerifiedPro;
	return !ss::badge::Label(B).empty();
}

/** Up to Count online events open to join now, small and cheap, one of each name (Bounty: progressive knockouts only). */
std::vector<ss::LobbyEvent> OpenNow(ss::Session& S, size_t Count, bool Bounty = false)
{
	const ss::net::Network& Net = ss::net::Shared();
	const double World = S.WorldMinutes();
	std::vector<ss::LobbyEvent> Out;
	std::set<std::string> Names;
	for (const ss::net::EventInstance& E : Net.Window(World - 180.0, World + 60.0))
	{
		const ss::LobbyEvent L = Net.Listing(E, nullptr, S.Unlocks());
		if (L.Joinable && L.BuyInCents <= 1100 && L.Spec.Entrants <= 3000 && (!Bounty || (L.Spec.BountyCents > 0 && !L.Spec.MysteryBounty)) && Out.size() < Count &&
			Names.insert(L.Spec.Name).second)
		{
			Out.push_back(L);
		}
	}
	return Out;
}

/** Whether two colours match (to a byte), alpha included. */
bool SameColor(const ss::ui::Color& A, const ss::ui::Color& B)
{
	return std::fabs(A.R - B.R) < 0.004f && std::fabs(A.G - B.G) < 0.004f && std::fabs(A.B - B.B) < 0.004f && std::fabs(A.A - B.A) < 0.004f;
}

/** The vertices filled with Col (the triangles' own, so text is left out), as points. */
std::vector<std::pair<float, float>> PointsIn(const DrawList& L, const ss::ui::Color& Col)
{
	std::vector<std::pair<float, float>> Out;
	for (const ss::ui::Vertex& V : L.Vertices)
	{
		if (SameColor(V.Col, Col))
		{
			Out.push_back({V.X, V.Y});
		}
	}
	return Out;
}

/** Whether something was filled with Col within Radius of (X, Y). */
bool FilledNear(const DrawList& L, const ss::ui::Color& Col, float X, float Y, float Radius)
{
	for (const std::pair<float, float>& P : PointsIn(L, Col))
	{
		if ((P.first - X) * (P.first - X) + (P.second - Y) * (P.second - Y) <= Radius * Radius)
		{
			return true;
		}
	}
	return false;
}

// ------------------------------------------------------------------ the taskbar and Mail

/** The dot over an app's taskbar button (Btn.X + 29, Btn.Y + 4): the button starts 38 units left of its name. */
bool TaskbarDot(const DrawList& L, const char* App, const ss::ui::Color& Col, const TableMeasurer& M)
{
	const Box B = RunOf(L, App, M, 960.0f, 1000.0f);
	return B.Found && FilledNear(L, Col, B.X - 38.0f + 29.0f, ss::ui::RiverLine::Height - 38.0f + 4.0f + 4.0f, 5.0f);
}

void Taskbar()
{
	Laptop P("laptop-taskbar");
	const DrawList L = P.Settled();
	// Mail's unread dot and Kast's gold one (a brand waiting on an answer): neither before there's a reason.
	const ss::ui::Color MailDot = ss::ui::Hex(0x6fa8ff);
	Expect(!TaskbarDot(L, "Mail", MailDot, P.M) && !TaskbarDot(L, "Kast", ss::ui::pal::Gold, P.M), "taskbar: no dots on Mail or Kast with nothing waiting");
	{
		Laptop Q("laptop-taskbar-dots");
		ss::Session& S = Q.S;
		S.Life.Inbox.push_back(ss::life::Letter{S.WorldMinutes(), "note", "stake", "Lantern Row Staking", "A note", "Read me.", 25, false});
		Expect(TaskbarDot(Q.Settled(), "Mail", MailDot, Q.M), "taskbar: an unread letter puts the blue dot on Mail");
		S.Life.Inbox.back().Read = true;
		Expect(!TaskbarDot(Q.Settled(), "Mail", MailDot, Q.M), "taskbar: read, the dot goes");
		S.DebugSponsor("offer luckypenny");
		bool Waiting = false;
		for (const ss::sponsor::Contract& K : S.Life.Sponsors.Contracts)
		{
			Waiting = Waiting || ((K.St == ss::sponsor::State::Offered || K.St == ss::sponsor::State::Countered) && K.ExpiresAt > S.WorldMinutes());
		}
		Expect(Waiting && TaskbarDot(Q.Settled(), "Kast", ss::ui::pal::Gold, Q.M), "taskbar: a sponsor's open offer puts the gold dot on Kast");
	}
	// Update 2 (§4.8, §5.11): Rerun is the ninth app, after Mail. The old check here was eight apps, with Sleep's end under
	// x 1,100; Rerun's button (about 93 units) moves Sleep past that, so the room left is checked where it matters, the news
	// wire's lane between Sleep and the status (TaskbarLane, below: at least 120 at 1,600 wide in the widest case).
	const char* Apps[10] = {"RiverLine", "ShiftLink", "Burner", "Bank", "GearDrop", "Kast", "Penny Drop", "Mail", "Rerun", "Sleep"};
	float Prev = -1.0f;
	bool Ordered = true;
	bool All = true;
	for (const char* A : Apps)
	{
		const Box B = RunOf(L, A, P.M, 960.0f, 1000.0f);
		All = All && B.Found;
		Ordered = Ordered && B.X > Prev;
		Prev = B.X + B.W;
	}
	Expect(All && Ordered, "laptop: the taskbar has its nine apps and Sleep, in order (Rerun after Mail)");
	// Mail opens from the taskbar.
	const Box Mail = RunOf(L, "Mail", P.M, 960.0f, 1000.0f);
	P.Click(Mail.X + Mail.W / 2.0f, Mail.Base - 4.0f);
	Expect(P.RL.CurrentApp() == App::Mail, "laptop: the taskbar opens Mail");
	Expect(Drew(P.Settled(), "Inbox"), "laptop: Mail draws its inbox");
}

// ------------------------------------------------------------------ the Bank

void Bank()
{
	Laptop P("laptop-bank");
	ss::Session& S = P.S;
	S.BankrollCents = 412300;
	S.DebugStake("grant lanternrow");
	for (ss::stake::Deal& D : S.Life.Stakes.Deals)
	{
		if (D.K == ss::stake::Kind::Package && D.State == ss::stake::DealOpen)
		{
			D.Funded = D.RollNow + 68000;
		}
	}
	S.Life.Sponsors.StoreCreditCents = 2500;
	// The ledger: every new kind, with amounts a person can add up.
	S.Life.Ledger.clear();
	const double W = S.WorldMinutes();
	S.Life.Record(W - 300.0, "Big Lou: stake, Embercrest Nightly", 12000, ss::life::LedgerBacking);
	S.Life.Record(W - 299.0, "Embercrest Nightly $120: buy-in", -10500, 0);
	S.Life.Record(W - 298.0, "Embercrest Nightly $120: entry fee", -1500, 0);
	S.Life.Record(W - 200.0, "Team Embercrest: covered the Nightly", 12000, ss::life::LedgerSponsors);
	S.Life.Record(W - 150.0, "Team Embercrest: their 30% of the profit", -3000, ss::life::LedgerSponsors);
	S.Life.Record(W - 100.0, "Night Shift: 3rd place", 23600, 4);
	S.Life.Record(W - 90.0, "Big Lou: his share, Embercrest Nightly", -17800, ss::life::LedgerBacking);
	S.Life.Record(W - 60.0, "FoldEquityFred: the review", -2500, ss::life::LedgerCoaching);
	P.RL.OpenApp(App::Bank, P.Now);
	const DrawList L = P.Settled();
	WriteList("bank_segment", L);
	// Update 2 §1.2: the sixth row is "Streaming & video" (Kast and Rerun's payouts, kinds 6 and 13); still nine (BankVideo).
	const char* In[9] = {"Tournament cashes", "The Embercrest", "Leaderboard prizes", "Shifts", "Cash work", "Streaming & video", "Sponsors", "Backers & buyers", "Sales & refunds"};
	// Update 2 §1.2 (G3): the first Out row is "Poker & sites" (buy-ins, and a rival site's card and cashier fees, kind 14).
	const char* Out[8] = {"Poker & sites", "Rent, bills & fares", "Food & deliveries", "Gear", "Fines & Marcus", "Looks, fitness & food out", "Backers' shares & swaps", "Coaching"};
	bool Rows = true;
	for (const char* R : In)
	{
		Rows = Rows && DrewRun(L, R);
	}
	for (const char* R : Out)
	{
		Rows = Rows && DrewRun(L, R);
	}
	Expect(Rows, "bank: nine rows in, eight out");
	const Box Last = RunOf(L, "Sales & refunds", P.M);
	const Box Lout = RunOf(L, "Coaching", P.M, 600.0f, 940.0f);
	Expect(Last.Found && Last.Base + 11.0f <= 570.0f + 360.0f && Lout.Found && Lout.Base < Last.Base, "bank: the ninth row fits inside the card at the 26-unit pitch");
	// Netted: the cover less the cut; the stake in, Lou's share out; coaching out.
	Expect(DrewRun(L, "$90.00") && DrewRun(L, "$120.00") && DrewRun(L, "$178.00") && DrewRun(L, "$25.00"), "bank: the sponsor row is netted, backing in and out, coaching");
	// Net counts the ledger: 90 + 120 + 236 in, 120 + 178 + 25 out.
	Expect(Drew(L, "Net +$123.00"), "bank: Net is the ledger's, the pots aside");
	Expect(Drew(L, "\xC2\xB7 Backing") && Drew(L, "\xC2\xB7 Sponsors") && Drew(L, "\xC2\xB7 Coaching"), "bank: the history names kinds 10 to 12 (none reads Prizes)");
	// The pots beside the balance, never in it.
	Expect(Spelled(L, "STAKED ROLL") && DrewRun(L, "Lantern Row Staking") && Drew(L, "\xE2\x88\x92$680 makeup"), "bank: the staked roll's card, makeup with its sign");
	Expect(Spelled(L, "STORE CREDIT") && Drew(L, "$25.00") && Drew(L, "Lucky Penny"), "bank: the store credit's card");
	Expect(Drew(L, "$4,123.00"), "bank: the balance is the bankroll alone");
}

// ------------------------------------------------------------------ Rerun around its app (update 2, G2)

/** A fixture's text (Tests/fixtures), "" when it can't be read. */
std::string Fixture(const std::string& Name)
{
	std::ifstream In(std::string(SS_TESTS_DIR) + "/fixtures/" + Name, std::ios::binary);
	std::stringstream Text;
	Text << In.rdbuf();
	return Text.str();
}

/**
 * The news wire's lane in a taskbar frame (RiverLineApps.cpp, Taskbar): from the ticker's LIVE pill (drawn 18 in from the
 * lane's left, and only while the lane is over 120 wide) to the battery, which sits 40 left of the energy figure with the
 * lane stopping 18 short of it. -1 when the frame has no ticker or no energy figure.
 */
float LaneWidth(const DrawList& L, float& Left)
{
	float Live = -1.0f;
	float Energy = -1.0f;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || Cmd.Text.BaselineY < 962.0f || Cmd.Text.BaselineY > 1000.0f)
		{
			continue;
		}
		const std::string& T = Cmd.Text.Text;
		Live = T == "LIVE" ? Cmd.Text.X : Live;
		Energy = !T.empty() && T.back() == '%' ? Cmd.Text.X : Energy;
	}
	if (Live < 0.0f || Energy < 0.0f)
	{
		return -1.0f;
	}
	Left = Live - 18.0f;
	return Energy - 40.0f - 18.0f - Left;
}

void TaskbarLane()
{
	// Nine apps and Sleep at 1,600 wide (§5.11: the lane needs 120; G3's caret takes about 20 more): measured in a frame with
	// the status at its widest that a frame can be set to (heat at 100, energy at 100%), less what the widest weekday and
	// date, and the widest clock, would take over the ones drawn.
	Laptop P("laptop-taskbar-lane");
	ss::Session& S = P.S;
	S.Life.Heat = 100.0;
	S.Life.Energy = 100.0;
	const DrawList L = P.Settled();
	WriteList("taskbar_rerun", L);
	float Left = 0.0f;
	const float Lane = LaneWidth(L, Left);
	const ss::ui::Font DayFace = ss::ui::Canvas::FaceFor(600, false);
	const ss::ui::Font Clock = ss::ui::Canvas::FaceFor(700, true);
	const auto DayText = [](int D) { return std::string(ss::net::WeekdayName(D)) + ", " + ss::net::DateLabel(D); };
	const double World = S.WorldMinutes();
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
	const float Worst = Lane - (DayMax - P.M.Width(DayText(ss::net::DayOf(World)), DayFace, 12.0f)) - (ClockMax - P.M.Width(ss::net::TimeLabel(World), Clock, 14.0f));
	std::printf("  taskbar: nine apps and Sleep end at x %.0f; the news wire's lane is %.0f wide here, %.0f at its narrowest (needs 120)\n", Left, Lane, Worst);
	Expect(Lane > 0.0f && Worst >= 120.0f, "taskbar: nine apps and Sleep leave the news wire its 120 at 1,600 wide, the status at its widest");
	Expect(Worst >= 140.0f, "taskbar: and the 20 more G3's poker-button caret will take (§5.11)");
}

void TaskbarTurn()
{
	// A table waiting on the player while another app is up: the lane says whose turn it is, whole. With the ninth app it has
	// about 90 less, so the clock's half ("the clock is running") goes when it won't fit beside the name.
	Laptop P("laptop-taskbar-turn");
	ss::Session& S = P.S;
	S.BankrollCents = 5000;
	const std::vector<ss::LobbyEvent> One = OpenNow(S, 1);
	if (One.empty())
	{
		std::printf("  laptop: no event open right now; the lane's turn check is skipped\n");
		return;
	}
	S.RegisterEvent(One.front());
	S.CurrentPace = ss::Pace::Full;
	for (int Guard = 0; Guard < 30 * 900 && S.TablesWaiting() < 1; ++Guard)
	{
		P.Now = Step(S, P.Now, 1.0 / 30.0);
	}
	S.Life.Heat = 100.0; // the status at its widest
	P.RL.OpenApp(App::Bank, P.Now);
	const DrawList L = P.Settled();
	std::string Said;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.BaselineY > 962.0f && Cmd.Text.Text.rfind("Your turn at ", 0) == 0)
		{
			Said = Cmd.Text.Text;
		}
	}
	std::printf("  taskbar: a table waiting, the lane says \"%s\"\n", Said.c_str());
	Expect(S.TablesWaiting() == 1 && !Said.empty() && Said.find("\xE2\x80\xA6") == std::string::npos, "taskbar: whose turn it is reads whole beside nine apps");
}

void RerunTaskbar()
{
	Laptop P("laptop-rerun-dot");
	ss::Session& S = P.S;
	const ss::ui::Color Marigold = ss::ui::Hex(ss::ui::videoart::Marigold);
	Expect(!TaskbarDot(P.Settled(), "Rerun", Marigold, P.M), "rerun dot: none with nothing new");
	S.DebugVideo("fake badbeat online");
	S.DebugVideo("fake bluff embercrest");
	Expect(S.Life.Video.Open.size() == 2 && !TaskbarDot(P.Settled(), "Rerun", Marigold, P.M), "rerun dot: two new highlights aren't enough");
	S.DebugVideo("fake bigpot skip");
	const DrawList Three = P.Settled();
	WriteList("taskbar_rerun_dot", Three);
	Expect(S.Life.Video.Open.size() == 3 && TaskbarDot(Three, "Rerun", Marigold, P.M), "rerun dot: three new highlights put the marigold dot on Rerun");
	// A visit sees them: the button opens Rerun, its underline in marigold, and the dot's gone after.
	const Box Btn = RunOf(Three, "Rerun", P.M, 960.0f, 1000.0f);
	P.Click(Btn.X + Btn.W / 2.0f, Btn.Base - 4.0f);
	Expect(P.RL.CurrentApp() == App::Video, "rerun: the taskbar opens Rerun");
	const DrawList Open = P.Settled();
	Expect(FilledNear(Open, Marigold, Btn.X - 38.0f + 9.0f, ss::ui::RiverLine::Height - 2.0f, 3.0f), "rerun: its button's underline is marigold while it's open");
	P.RL.OpenApp(App::RiverLine, P.Now);
	Expect(!TaskbarDot(P.Settled(), "Rerun", Marigold, P.M), "rerun dot: gone after a visit");
	S.DebugVideo("fake cooler dees");
	S.DebugVideo("fake herocall online");
	Expect(!TaskbarDot(P.Settled(), "Rerun", Marigold, P.M), "rerun dot: two more since the visit still aren't enough");
	// A milestone on the bell (Channel::Bell, the last 20) brings it back on its own.
	S.Life.Video.Bell.push_back({S.WorldMinutes(), "100 subscribers."});
	Expect(TaskbarDot(P.Settled(), "Rerun", Marigold, P.M), "rerun dot: a milestone on the bell puts it back");
	// With Rerun in front the dot waits (the bell in its header says it there).
	P.RL.OpenApp(App::Video, P.Now);
	Expect(!TaskbarDot(P.Settled(), "Rerun", Marigold, P.M), "rerun dot: not on its button while Rerun is open");
}

void RerunForget()
{
	// ~RiverLine drops Rerun's side state (rlvideo_detail::ForgetVideo): a second career's laptop made where the first one's
	// was takes its own first look, its highlights from before the last twelve hours seen, so no dot.
	alignas(Laptop) static unsigned char Room[sizeof(Laptop)];
	Laptop* First = new (Room) Laptop("laptop-rerun-forget-a");
	(void)First->Settled(); // the first laptop's first look
	First->~Laptop();
	Laptop* P = new (Room) Laptop("laptop-rerun-forget-b");
	ss::Session& S = P->S;
	S.DebugVideo("fake badbeat online");
	S.DebugVideo("fake bluff embercrest");
	S.DebugVideo("fake bigpot skip");
	P->At(S.WorldMinutes() + 13.0 * 60.0);
	const bool Dot = TaskbarDot(P->Settled(), "Rerun", ss::ui::Hex(ss::ui::videoart::Marigold), P->M);
	const size_t Open = S.Life.Video.Open.size();
	P->~Laptop();
	Expect(Open == 3 && !Dot, "rerun dot: a new laptop where the last one was starts its own count (three from yesterday aren't new)");
}

void RerunEdit()
{
	// A video's edit (§4.5) is a time skip like a shift; its result card wears Rerun's marigold and name.
	Laptop P("laptop-rerun-edit");
	ss::Session& S = P.S;
	S.DebugVideo("fake badbeat online");
	ss::video::Choice Ch;
	Ch.Candidates = {S.Life.Video.Open.empty() ? std::string() : S.Life.Video.Open.front().Id};
	Ch.F = ss::video::Format::Breakdown;
	const std::string Why = S.Publish(Ch);
	for (int Guard = 0; Guard < 200 && (S.TimeSkip.Active || !S.HasOutcome); ++Guard)
	{
		P.Now = Step(S, P.Now, 0.1);
	}
	const DrawList L = P.Settled();
	WriteList("rerun_edit_outcome", L);
	// The kicker (tracked capitals at the card's top, y 316) in marigold.
	bool Marigold = false;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		Marigold = Marigold || (Cmd.Type == ss::ui::DrawCmd::Kind::Text && std::fabs(Cmd.Text.BaselineY - 316.0f) < 1.0f && SameColor(Cmd.Text.Col, ss::ui::Hex(ss::ui::videoart::Marigold)));
	}
	Expect(Why.empty() && S.HasOutcome && S.LastOutcome.ActivityId.rfind("video:", 0) == 0 && Spelled(L, "RERUN") && Drew(L, "Up on Rerun") && Marigold,
		("rerun: the edit's result card says RERUN in marigold" + (Why.empty() ? std::string() : " (" + Why + ")")).c_str());
}

/** The amount on one of the Bank's MONEY IN rows: the run on the label's baseline that ends at the column's right (x 452). */
std::string InRowAmount(const DrawList& L, const std::string& Label, const TableMeasurer& M)
{
	const Box Lb = RunOf(L, Label, M, 570.0f, 930.0f);
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Lb.Found && Cmd.Type == ss::ui::DrawCmd::Kind::Text && std::fabs(Cmd.Text.BaselineY - Lb.Base) < 0.01f && Cmd.Text.X > Lb.X + Lb.W &&
			std::fabs(Cmd.Text.X + M.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx) - 452.0f) < 0.5f)
		{
			return Cmd.Text.Text;
		}
	}
	return std::string();
}

void BankVideo()
{
	// An old save (a streamer with a Kast payout, nothing from Rerun): the row is renamed and its total is the same, the
	// kind-6 lines it always summed.
	{
		ss::SaveData D;
		const bool Parsed = ss::SaveData::Parse(Fixture("pre_update2_sponsored_streamer.ssave"), D);
		Laptop P("laptop-bank-old", D);
		ss::Chips Kast = 0;
		bool Rerun = false;
		for (const ss::life::LedgerEntry& E : P.S.Life.Ledger)
		{
			Kast += E.Kind == 6 && E.Amount > 0 ? E.Amount : 0;
			Rerun = Rerun || E.Kind == ss::life::LedgerVideo;
		}
		P.RL.OpenApp(App::Bank, P.Now);
		const DrawList L = P.Settled();
		WriteList("bank_video_old_save", L);
		const std::string Shown = InRowAmount(L, "Streaming & video", P.M);
		std::printf("  bank: an old save's streaming row reads %s (its Kast lines: %s)\n", Shown.c_str(), ss::Money(Kast).c_str());
		Expect(Parsed && Kast > 0 && !Rerun && Shown == ss::Money(Kast) && !DrewRun(L, "Streaming"), "bank: an old save's row reads \"Streaming & video\", its total the same");
	}
	// A Rerun payout beside a Kast payout: one row holds both (still nine rows), Net counts it, the history names it.
	{
		Laptop P("laptop-bank-video");
		ss::Session& S = P.S;
		S.Life.Ledger.clear();
		const double W = S.WorldMinutes();
		S.Life.Record(W - 120.0, "Kast payout", 2380, 6);
		S.Life.Record(W - 60.0, "Rerun payout: April", 14620, ss::life::LedgerVideo);
		P.RL.OpenApp(App::Bank, P.Now);
		const DrawList L = P.Settled();
		WriteList("bank_video", L);
		Expect(InRowAmount(L, "Streaming & video", P.M) == "$170.00", "bank: a Rerun payout sums into \"Streaming & video\" with Kast's (kinds 6 and 13)");
		Expect(Drew(L, "Net +$170.00"), "bank: the payout counts in Net");
		Expect(DrewRun(L, "Rerun payout: April") && Drew(L, "\xC2\xB7 Rerun") && !Drew(L, "\xC2\xB7 Prizes"), "bank: the history names Rerun's line (kind 13 isn't Prizes)");
		int Rows = 0;
		for (const char* R : {"Tournament cashes", "The Embercrest", "Leaderboard prizes", "Shifts", "Cash work", "Streaming & video", "Sponsors", "Backers & buyers", "Sales & refunds"})
		{
			Rows += DrewRun(L, R) ? 1 : 0;
		}
		Expect(Rows == 9, "bank: still nine rows in");
	}
}

void MailRerun()
{
	// Rerun's letters (Source "video"): its mark, "Partner Program" under the name, and a button to the Channel page.
	{
		Laptop P("laptop-mail-rerun");
		ss::Session& S = P.S;
		const double W = S.WorldMinutes();
		S.Life.Inbox.push_back(ss::life::Letter{W - 60.0, "rerun:partner", "video", "Rerun", "Your channel's under review.",
			"Your channel just cleared both bars for the Rerun Partner Program. Someone here is looking it over now.", ss::video::LetterReview, false});
		S.Life.Inbox.push_back(ss::life::Letter{W, "rerun:partner", "video", "Rerun", "Welcome to the Rerun Partner Program.",
			"You're in. From today ads run on your videos.", ss::video::LetterWelcome, false});
		P.RL.OpenApp(App::Mail, P.Now);
		const DrawList L = P.Settled();
		WriteList("mail_rerun", L);
		Expect(DrewRun(L, "Open your channel") && DrewRun(L, "Partner Program") && !DrewRun(L, "Open the Backing page"), "mail: Rerun's letters get a button to the channel");
		const Box B = RunOf(L, "Open your channel", P.M);
		P.Click(B.X + B.W / 2.0f, B.Base - 5.0f);
		Expect(P.RL.CurrentApp() == App::Video, "mail: the button opens Rerun");
		const DrawList V = P.Settled();
		WriteList("mail_rerun_opened", V);
		Expect(Drew(V, "No videos yet.") && !Drew(V, "Play some hands. The good ones show up here."), "mail: at the Channel page (an empty channel's \"No videos yet.\")");
	}
	// A statement has it too.
	{
		Laptop P("laptop-mail-rerun-pay");
		P.S.Life.Inbox.push_back(ss::life::Letter{P.S.WorldMinutes(), "rerun:pay", "video", "Rerun", "Your Rerun payout for April.",
			"We've sent $146.20 to your bank account.", ss::video::LetterStatement, false});
		P.RL.OpenApp(App::Mail, P.Now);
		Expect(DrewRun(P.Settled(), "Open your channel"), "mail: a Rerun statement links to the channel too");
	}
	// A staking outfit's letter keeps its own link, and no Rerun button.
	{
		Laptop P("laptop-mail-stake");
		P.S.Life.Inbox.push_back(ss::life::Letter{P.S.WorldMinutes(), "note", "stake", "Lantern Row Staking", "A note", "Read me.", 25, false});
		P.RL.OpenApp(App::Mail, P.Now);
		const DrawList L = P.Settled();
		Expect(DrewRun(L, "Open the Backing page") && !DrewRun(L, "Open your channel"), "mail: a staking letter still links to Backing, not Rerun");
	}
}

/** The baseline of the first run exactly Text in L, or -1. */
float RunBase(const DrawList& L, const std::string& Text)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text == Text)
		{
			return Cmd.Text.BaselineY;
		}
	}
	return -1.0f;
}

void RerunDates()
{
	// "Rerun payout Apr 21" (§1.6, §4.9): Session::Marks merges video::Marks. The second screen's COMING UP gives a mark a
	// week or more out its date (a weekday would read as this week's), and the wall calendar writes RERUN on the 21st.
	Laptop P("laptop-rerun-dates");
	ss::Session& S = P.S;
	int Tenth = -1;
	for (int D = ss::net::DayOf(S.WorldMinutes()); D < 400 && Tenth < 0; ++D)
	{
		int Y = 0, M = 0, Dm = 0;
		ss::world::CivilDate(D, Y, M, Dm);
		Tenth = Dm == 10 ? D : -1;
	}
	P.At(static_cast<double>(Tenth) * Day + 14.0 * 60.0);
	S.Life.Video.PartnerAt = S.WorldMinutes() - Day; // a Partner with $100 in the balance: the 21st pays, so it's marked
	S.Life.Video.BalanceCents = 12000;
	const auto Coming = [&P, &S]() {
		DrawList Tile;
		ss::ui::Canvas C(Tile, P.M, ss::ui::secondscreen::Width, ss::ui::secondscreen::Height, 1.0f);
		ss::ui::secondscreen::Draw(C, S, 0, P.Now);
		return Tile;
	};
	const int Pay = Tenth + 11;
	const DrawList Far = Coming();
	WriteList("second_screen_rerun", Far);
	const float Label = RunBase(Far, "Rerun payout");
	Expect(DrewRun(Far, "COMING UP") && Label > 0.0f && RunBase(Far, ss::net::DateLabel(Pay)) == Label,
		("second screen: \"" + ss::net::DateLabel(Pay) + "  Rerun payout\", eleven days out").c_str());
	DrawList Wall;
	{
		ss::ui::Canvas C(Wall, P.M, ss::ui::wallcalendar::Width, ss::ui::wallcalendar::Height, 1.4f);
		ss::ui::wallcalendar::Draw(C, S);
	}
	WriteList("wall_calendar_rerun", Wall);
	Expect(DrewRun(Wall, "RERUN"), "wall calendar: the 21st's payout written on the month");
	// Four days out it's a weekday again.
	P.At(static_cast<double>(Pay - 4) * Day + 14.0 * 60.0);
	S.Life.Video.PartnerAt = S.WorldMinutes() - Day;
	const DrawList Near = Coming();
	const float Label2 = RunBase(Near, "Rerun payout");
	Expect(Label2 > 0.0f && RunBase(Near, ss::net::WeekdayName(Pay, true)) == Label2, "second screen: four days out, the payout's weekday");
}

/** The labels this track adds that aren't sentences (the copy lint wants an ending): no placeholder, no real brand, no "AI". */
void RerunLabels()
{
	const char* Labels[] = {"Rerun", "Streaming & video", "Poker sites", "Open your channel", "Partner Program", "RERUN"};
	const char* Real[] = {"youtube", "twitch", "kick", "tiktok", "rumble", "vimeo", "dailymotion", "pokerstars", "ggpoker", "skrill", "paypal", "venmo"};
	bool Clean = true;
	for (const char* Lb : Labels)
	{
		const std::string Low = copylint::Lower(Lb);
		Clean = Clean && !Low.empty() && Low.find("  ") == std::string::npos && Low.find("todo") == std::string::npos && Low.find("placeholder") == std::string::npos;
		for (const char* R : Real)
		{
			Clean = Clean && Low.find(R) == std::string::npos;
		}
		for (const char* Word : {"ai", "bot", "autoplay"})
		{
			const std::string W(Word);
			for (size_t At = Low.find(W); At != std::string::npos; At = Low.find(W, At + 1))
			{
				const bool Start = At == 0 || !std::isalpha(static_cast<unsigned char>(Low[At - 1]));
				const bool End = At + W.size() >= Low.size() || !std::isalpha(static_cast<unsigned char>(Low[At + W.size()]));
				Clean = Clean && !(Start && End);
			}
		}
	}
	Expect(Clean, "rerun: the taskbar, Bank, Mail and calendar labels pass the screen (no placeholder, no real platform, no \"AI\")");
}

// ------------------------------------------------------------------ the Backing tab and the nav

void Backing()
{
	Laptop P("laptop-backing");
	ss::Session& S = P.S;
	Expect(!ss::stake::Anything(S.Life.Stakes) && !DrewRun(P.Settled(), "Backing"), "backing tab: hidden while there's nothing to show");
	S.DebugStake("offer lou");
	const DrawList L = P.Settled();
	const Box Tab = RunOf(L, "Backing", P.M, 20.0f, 60.0f);
	Expect(Tab.Found, "backing tab: shown once an offer stands");
	P.Click(Tab.X + Tab.W / 2.0f, Tab.Base - 5.0f);
	const DrawList Pg = P.Settled();
	Expect(P.RL.CurrentPage() == Page::Backing && Spelled(Pg, "OFFERS") && Drew(Pg, "Big Lou"), "backing tab: opens the page with Lou's offer");

	// Four tables and the Backing tab in one bar.
	S.BankrollCents = 6000 + 14900 + 28900 + 20000;
	Expect(S.Buy("monitor-24").empty() && S.Buy("monitor-27").empty(), "nav: two monitors for four tables");
	const std::vector<ss::LobbyEvent> Picks = OpenNow(S, 4);
	if (Picks.size() < 4)
	{
		std::printf("  laptop: fewer than four tables open right now; the nav width check is skipped\n");
		return;
	}
	for (const ss::LobbyEvent& E : Picks)
	{
		S.RegisterEvent(E);
		P.Now = Step(S, P.Now, 1.0);
	}
	S.ShowLobby();
	const DrawList Four = P.Settled();
	WriteList("nav_four_tables_backing", Four);
	const Box Nav = RunOf(Four, "Backing", P.M, 20.0f, 60.0f);
	// Each tab's name (cut short with an ellipsis when it must be): after Backing, and the last clear of the balance.
	int Found = 0;
	float Left = 1.0e9f;
	float Last = 0.0f;
	float Balance = 0.0f;
	for (const ss::ui::DrawCmd& Cmd : Four.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || Cmd.Text.BaselineY > 60.0f)
		{
			continue;
		}
		const std::string& T = Cmd.Text.Text;
		Balance = T.rfind("Balance ", 0) == 0 ? Cmd.Text.X : Balance;
		for (int I = 0; I < S.TableCount(); ++I)
		{
			const std::string Name = S.Glance(I).Name;
			const size_t Colon = Name.find(": ");
			const std::string Short = Colon == std::string::npos ? Name : Name.substr(Colon + 2);
			if (Cmd.Text.BaselineY < 35.0f && T.size() >= 4 && Short.compare(0, 4, T, 0, 4) == 0)
			{
				++Found;
				Left = std::min(Left, Cmd.Text.X);
				Last = std::max(Last, Cmd.Text.X);
				break;
			}
		}
	}
	Expect(Nav.Found && DrewRun(Four, "Boards") && S.TableCount() == 4 && Found >= 4 && Left > Nav.X + Nav.W && Balance > Last + 80.0f,
		"nav: four table tabs and Backing share the bar (the pages tighten up)");
}

// ------------------------------------------------------------------ sizing

void Sizing()
{
	Laptop P("laptop-sizing");
	ss::Session& S = P.S;
	S.BankrollCents = 5000;
	const std::vector<ss::LobbyEvent> One = OpenNow(S, 1);
	if (One.empty())
	{
		std::printf("  laptop: no event open right now; the presets check is skipped\n");
		return;
	}
	S.RegisterEvent(One.front());
	S.CurrentPace = ss::Pace::Full;
	for (int Guard = 0; Guard < 30 * 900 && !(S.HasPrompt && S.Prompt.CanRaise && S.CurHand); ++Guard)
	{
		P.Now = Step(S, P.Now, 1.0 / 30.0);
	}
	if (!(S.HasPrompt && S.Prompt.CanRaise && S.CurHand))
	{
		Expect(false, "sizing: a decision with a raise comes");
		return;
	}
	ss::sizing::Spot Spot;
	for (const ss::HandSeat& Hs : S.CurHand->Seats)
	{
		if (Hs.Id == ss::HeroId)
		{
			Spot = ss::sizing::SpotOf(*S.CurHand, Hs.Seat);
		}
	}
	Spot.Unit = 1;
	const std::vector<ss::sizing::Preset> Want = ss::sizing::Presets(Spot);
	const DrawList L = P.Frame();
	WriteList("table_presets_pins", L);
	bool All = !Want.empty();
	for (const ss::sizing::Preset& Pr : Want)
	{
		All = All && DrewRun(L, Pr.Label);
	}
	Expect(All, "sizing: RiverLine's presets are core's for the spot");
	// The first preset sets the size.
	const float Pitch = Want.size() > 5 ? 62.0f : 72.0f;
	P.Click(640.0f + (Pitch - 6.0f) / 2.0f, 786.0f + 17.0f);
	const ss::Chips Expected = std::max(S.Prompt.MinRaise, std::min(S.Prompt.MaxRaise, Want.front().To));
	Expect(!S.HasPrompt || S.Prompt.RaiseTo == Expected, "sizing: a preset sets the raise");
	// The arrows step the way the card room's do (sizing::Step), inside the legal range.
	if (S.HasPrompt && S.Prompt.CanRaise)
	{
		const ss::Chips From = S.Prompt.RaiseTo;
		const ss::Chips Up = std::max(S.Prompt.MinRaise, std::min(S.Prompt.MaxRaise, ss::sizing::Step(Spot, From, 1)));
		P.RL.Key("ArrowUp");
		Expect(S.Prompt.RaiseTo == Up, "sizing: ArrowUp is sizing::Step's next size");
		P.RL.Key("ArrowDown");
		const ss::Chips Down = std::max(S.Prompt.MinRaise, std::min(S.Prompt.MaxRaise, ss::sizing::Step(Spot, Up, -1)));
		Expect(S.Prompt.RaiseTo == Down, "sizing: ArrowDown steps back");
	}
}

/** A progressive knockout's table: the seats' badge pins beside the bounty tags (badges_table_seat). */
void SeatPins()
{
	Laptop P("laptop-pko");
	ss::Session& S = P.S;
	S.Life.Unlocks.insert("bounty");
	S.BankrollCents = 2000;
	// A Black Card on the player, so the hero's seat carries a pin beside the bounty tag.
	(void)S.DebugBadge("grant Card 3");
	ss::net::EventInstance Pko;
	if (!ss::net::Shared().FindInstance("hh-110@1500", Pko))
	{
		Expect(false, "seat pins: the 1 AM PKO is scheduled");
		return;
	}
	S.RegisterEvent(ss::net::Shared().Listing(Pko, nullptr, S.Unlocks()));
	S.CurrentPace = ss::Pace::Full;
	for (int Guard = 0; Guard < 200000 && !(S.HasPrompt && S.CurHand); ++Guard)
	{
		P.Now = Step(S, P.Now, 1.0 / 30.0);
	}
	const DrawList L = P.Frame();
	WriteList("badges_table_seat", L);
	int Tags = 0;
	int Pinned = 0;
	bool HeroPin = false;
	for (const ss::SeatVis& V : S.Seats)
	{
		const auto Known = S.FieldNpc.find(V.Id);
		HeroPin = HeroPin || (V.IsHero && V.Top.K == ss::badge::Kind::CircuitCard);
		if (V.Present && Known != S.FieldNpc.end() && !ss::badge::Shared().Of(S.Living(), Known->second, ss::net::DayOf(S.WorldMinutes())).Held.empty())
		{
			++Pinned;
		}
	}
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		Tags += Cmd.Type == ss::ui::DrawCmd::Kind::Text && !Cmd.Text.Text.empty() && Cmd.Text.Text[0] == '$' && Cmd.Text.SizePx == 12.0f ? 1 : 0;
	}
	std::printf("  laptop: the PKO table: %d bounty tags, %d seats with badges the pin may show\n", Tags, Pinned);
	Expect(S.HasPrompt && Tags >= 2 && HeroPin, "seat pins: a PKO table with its bounty tags and the player's Black Card on the seat");
	// The pin as drawn: what the Black Card adds to the same frame (its dark disc), clear of every bounty tag.
	const ss::ui::Color Disc = ss::ui::Hex(0x0b1220);
	std::vector<std::pair<float, float>> With = PointsIn(L, Disc);
	(void)S.DebugBadge("clear");
	std::vector<std::pair<float, float>> Without = PointsIn(P.Frame(), Disc);
	std::sort(With.begin(), With.end());
	std::sort(Without.begin(), Without.end());
	std::vector<std::pair<float, float>> Pin;
	std::set_difference(With.begin(), With.end(), Without.begin(), Without.end(), std::back_inserter(Pin));
	bool Clear = !Pin.empty();
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || Cmd.Text.Text.empty() || Cmd.Text.Text[0] != '$' || Cmd.Text.SizePx != 12.0f)
		{
			continue;
		}
		const float W = P.M.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx);
		for (const std::pair<float, float>& Pt : Pin)
		{
			Clear = Clear && !(Pt.first >= Cmd.Text.X - 6.0f && Pt.first <= Cmd.Text.X + W + 6.0f && Pt.second >= Cmd.Text.BaselineY - 13.0f && Pt.second <= Cmd.Text.BaselineY + 5.0f);
		}
	}
	Expect(Clear, "seat pins: the player's pin is drawn (13 on a dark disc), clear of every bounty tag");
}

/** The lobby's event panel: an online event the player pays for, its Sell action opening the sell panel on it. */
void OnlineSell()
{
	Laptop P("laptop-online-sell");
	ss::Session& S = P.S;
	S.BankrollCents = 50000;
	const ss::net::Network& Net = ss::net::Shared();
	const double World = S.WorldMinutes();
	std::string Pick;
	for (const ss::net::EventInstance& E : Net.Window(World, World + 180.0))
	{
		const ss::LobbyEvent L = Net.Listing(E, nullptr, S.Unlocks());
		const ss::net::LiveState St = Net.Live(E, World);
		if (Pick.empty() && L.Joinable && L.BuyInCents > 0 && L.BuyInCents <= 1100 &&
			(St.St == ss::net::Status::Announced || St.St == ss::net::Status::Registering || St.St == ss::net::Status::LateReg))
		{
			Pick = E.Id;
		}
	}
	if (Pick.empty())
	{
		std::printf("  laptop: no online event open to sell right now; the online Sell action check is skipped\n");
		return;
	}
	P.RL.SelectEvent(Pick);
	const DrawList L = P.Settled();
	WriteList("lobby_sell_action", L);
	const Box Sell = RunOf(L, "Sell action  \xE2\x80\xBA", P.M);
	Expect(Sell.Found, "online: the event panel offers pieces of an event the player pays for");
	if (Sell.Found)
	{
		P.Click(Sell.X + Sell.W / 2.0f, Sell.Base - 4.0f);
		const DrawList Pg = P.Settled();
		Expect(P.RL.CurrentPage() == Page::Backing && Drew(Pg, "RiverLine \xC2\xB7 "), "online: Sell action opens the sell panel on the RiverLine event");
	}
}

// ------------------------------------------------------------------ the Embercrest card's funding

/** A laptop on Wednesday at 5 PM: the Nightly's desk open, $1,300 in the bank, the Embercrest page on today. */
void ToWednesday(Laptop& P)
{
	P.At(2.0 * Day + 17.0 * 60.0);
	P.S.BankrollCents = 130000;
	P.RL.OpenPage(Page::Embercrest, P.Now);
	P.RL.ShowLiveDay(2, std::string(), P.Now);
}

void Funding()
{
	const float CardY = 282.0f + 214.0f + 12.0f; // the day's second card: the Nightly
	{
		// The player's own money: the Sell action opens the Backing page on the event.
		Laptop P("laptop-sell");
		ToWednesday(P);
		const DrawList L = P.Settled();
		Expect(!Drew(L, "is putting you in") && DrewRun(L, "Register \xC2\xB7 $120"), "embercrest card: own money, the price on the button");
		const Box Sell = RunOf(L, "Sell action  \xE2\x80\xBA", P.M, CardY + 190.0f, CardY + 214.0f);
		Expect(Sell.Found, "embercrest card: the Sell action on a night it's the player's to sell");
		if (Sell.Found)
		{
			P.Click(Sell.X + Sell.W / 2.0f, Sell.Base - 4.0f);
			const DrawList Pg = P.Settled();
			Expect(P.RL.CurrentPage() == Page::Backing && Drew(Pg, "Nightly \xC2\xB7 Today"), "embercrest card: Sell action opens the sell panel on the Nightly");
		}
	}
	{
		// Lou's stake: his line, the button, the confirm's money.
		Laptop P("laptop-backed");
		ToWednesday(P);
		P.S.DebugStake("offer lou");
		const DrawList L = P.Settled();
		WriteList("embercrest_card_backed", L);
		Expect(Drew(L, "Big Lou is putting you in the Nightly.") && DrewRun(L, "Register \xC2\xB7 backed") && Drew(L, "Big Lou pays the $120"),
			"embercrest card: Lou's stake says so, on the strip and the button");
		P.Click(24.0f + 1052.0f - 26.0f - 115.0f, CardY + 138.0f + 21.0f);
		Expect(P.RL.LiveConfirming() == "embercrest-nightly@2", "embercrest card: Register asks to confirm");
		const DrawList Cf = P.Frame();
		Expect(Drew(Cf, "Leaves now: $2.90 for the bus. Big Lou pays the $120 \xC2\xB7 balance after $1,297.10"), "embercrest card: confirming, only the bus leaves the bank");
	}
	{
		// Team Embercrest's cover: the count left, the bus too.
		Laptop P("laptop-covered");
		ToWednesday(P);
		P.S.DebugSponsor("standing 2");
		const std::string Signed = P.S.DebugSponsor("sign embercrest");
		const DrawList L = P.Settled();
		WriteList("embercrest_card_covered", L);
		Expect(Drew(L, "Covered by Team Embercrest \xC2\xB7 2 of 2 left \xC2\xB7 the bus too") && DrewRun(L, "Register \xC2\xB7 covered"),
			("embercrest card: a cover says how many are left (" + Signed + ")").c_str());
		P.RL.OpenApp(App::Burner, P.Now);
		P.RL.ShowContact(2);
		Expect(DrewRun(P.Settled(), "Register \xC2\xB7 covered"), "burner: Dee's rows register on the cover too");
		// A second brand whose cover doesn't take the Nightly (listed with why) changes nothing: the cover still pays, and
		// a covered night is nobody's to sell (the sell panel's own rule, Picks).
		const std::string Penny = P.S.DebugSponsor("sign luckypenny");
		P.RL.OpenApp(App::RiverLine, P.Now);
		P.RL.OpenPage(Page::Embercrest, P.Now);
		P.RL.ShowLiveDay(2, std::string(), P.Now);
		const DrawList Two = P.Settled();
		Expect(Drew(Two, "Covered by Team Embercrest") && DrewRun(Two, "Register \xC2\xB7 covered") && !DrewRun(Two, "Sell action  \xE2\x80\xBA"),
			("embercrest card: covered by one brand of two, and not for sale (" + Penny + ")").c_str());
	}
}

// ------------------------------------------------------------------ the flip, and the room's leaders

void Flip()
{
	Laptop P("laptop-flip");
	ToWednesday(P);
	DrawList L = P.Settled();
	// The first card's (the Noon Deepstack's) three small tabs, on the price's second line left of the split.
	const Box Split = RunOf(L, "$70 + $10 fee", P.M, 282.0f + 50.0f, 282.0f + 80.0f);
	Expect(Split.Found, "flip: the Noon Deepstack's card leads the day");
	const char* Faces[3] = {"FACTS", "STRUCTURE", "PAYOUTS"};
	float Fx = Split.X - 18.0f;
	float Cx[3] = {0.0f, 0.0f, 0.0f};
	for (int K = 2; K >= 0; --K)
	{
		const float Fw = P.Measure(Faces[K], 9.0f, 800) + 1.0f * static_cast<float>(std::string(Faces[K]).size()) + 16.0f;
		Fx -= Fw;
		Cx[K] = Fx + Fw / 2.0f;
		Fx -= 6.0f;
	}
	const float Fy = 282.0f + 55.0f + 9.0f;
	Expect(Spelled(L, "STACK") && Spelled(L, "DEEP RUN"), "flip: the facts first");
	P.Click(Cx[1], Fy);
	L = P.Settled();
	Expect(DrewRun(L, "100/200") && Drew(L, "no ante") && Spelled(L, "LEVEL 1"), "flip: STRUCTURE shows the levels");
	{
		// The levels clear the register button's top (the card's y + 138), which sits under the last two columns.
		bool Above = true;
		for (const ss::ui::DrawCmd& Cmd : L.Cmds)
		{
			const std::string& T = Cmd.Text.Text;
			if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.BaselineY > 282.0f && Cmd.Text.BaselineY < 282.0f + 214.0f && (T.rfind("ante ", 0) == 0 || T == "no ante"))
			{
				Above = Above && Cmd.Text.BaselineY + 3.0f < 282.0f + 138.0f;
			}
		}
		Expect(Above, "flip: STRUCTURE's rows stay above the register button");
	}
	WriteList("embercrest_card_structure", L);
	P.Click(Cx[2], Fy);
	L = P.Settled();
	Expect(Spelled(L, "MIN CASH") && Spelled(L, "1ST") && Drew(L, "runners: a ") && Drew(L, "prize pool, the buy-in less the fee."), "flip: PAYOUTS shows the ladder for the field");
	WriteList("embercrest_card_payouts", L);
	P.Click(Cx[0], Fy);
	Expect(Spelled(P.Settled(), "DEEP RUN"), "flip: back to the facts");

	// LEADERS on the right: the club card and the month's board.
	const float Tw0 = P.Measure("THE WEEK", 12.0f, 800) + 1.6f * 8.0f + 11.0f;
	const float T1 = 1092.0f + 24.0f + Tw0 + 22.0f;
	P.Click(T1 + 30.0f, 84.0f + 31.0f);
	L = P.Settled();
	WriteList("badges_embercrest_page", L);
	Expect(Spelled(L, "PLAYERS CLUB") && Spelled(L, "LEADERS") && Drew(L, "in 90 days") && (Drew(L, "more for Regular") || Drew(L, "more for Ember") || Drew(L, "The top card")),
		"leaders: the club card's standing and the month's board");

	// Choices last a visit: back on the page, the week again, every card on its facts.
	P.Click(Cx[1], Fy);
	P.RL.OpenPage(Page::Lobby, P.Now);
	P.Settled();
	P.RL.OpenPage(Page::Embercrest, P.Now);
	L = P.Settled();
	Expect(!Spelled(L, "PLAYERS CLUB") && Spelled(L, "DEEP RUN") && !Spelled(L, "LEVEL 1"), "embercrest page: reopened, the week and the facts (the flips don't follow the laptop around)");
}

/** Dee's rows on the Burner: the regulars down for the Nightly by name, each with the room's glyph, inside the row. */
void BurnerRegulars()
{
	Laptop P("laptop-burner-regulars");
	ToWednesday(P);
	ss::Session& S = P.S;
	ss::world::World& W = S.Living();
	const ss::live::Occurrence O = ss::live::FindOccurrence("embercrest-nightly@2");
	std::vector<std::string> Names;
	for (const int Npc : W.Registered(O.Id))
	{
		const ss::world::Npc* N = W.Get(Npc);
		if (Names.size() < 3 && N && N->Name != S.HeroName)
		{
			// Met at the room twice, on two nights: they know the player.
			W.Remember(Npc, ss::world::MemoryKind::Met, "the Embercrest", 0, S.WorldMinutes() - 3.0 * Day);
			W.Remember(Npc, ss::world::MemoryKind::Met, "the Embercrest", 0, S.WorldMinutes());
			Names.push_back(N->Name);
		}
	}
	if (Names.empty())
	{
		std::printf("  laptop: nobody planned for the Nightly yet; the Burner regulars check is skipped\n");
		return;
	}
	P.At(S.WorldMinutes() + 1.0); // the rows' plan is looked up again each minute
	P.RL.OpenApp(App::Burner, P.Now);
	P.RL.ShowContact(2);
	const DrawList L = P.Settled();
	WriteList("app_burner_regulars", L);
	const Box You = RunOf(L, "  \xC2\xB7  you know ", P.M);
	bool Named = false;
	bool Inside = You.Found;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || !You.Found || Cmd.Text.BaselineY != You.Base)
		{
			continue;
		}
		Named = Named || Cmd.Text.Text.find(Names.front()) != std::string::npos;
		Inside = Inside && Cmd.Text.X + P.M.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx) <= 1600.0f - 40.0f - 20.0f - 4.0f + 1.0f;
	}
	Expect(You.Found && Named && Inside, "burner: Dee's row names the regulars down for the Nightly, inside the row");
}

// ------------------------------------------------------------------ Sam on the Burner

void Sam()
{
	Laptop P("laptop-sam");
	ss::Session& S = P.S;
	P.At(1.0 * Day + 3.0 * 60.0 + 12.0);
	S.DebugStake("offer sam");
	P.RL.OpenApp(App::Burner, P.Now);
	P.RL.ShowContact(1);
	DrawList L = P.Settled();
	WriteList("burner_sam_offer", L);
	Expect(Drew(L, "His package") && DrewRun(L, "Take it") && DrewRun(L, "Quit in makeup and it's a debt: 25% of every cash and shift until it's paid.") && Drew(L, "u keep 35"),
		"burner: Sam's offer, its strings spelled out, his lines in the thread");
	P.Click(380.0f + 600.0f + 26.0f + 125.0f, 962.0f - 260.0f + 24.0f + 214.0f - 62.0f + 23.0f);
	const ss::stake::Deal* Deal = nullptr;
	for (const ss::stake::Deal& D : S.Life.Stakes.Deals)
	{
		Deal = D.By == ss::stake::Outfit::Sam ? &D : Deal;
	}
	Expect(Deal != nullptr, "burner: Take it takes his package");
	L = P.Settled();
	Expect(Drew(L, "Roll $1,500") && Drew(L, "Quit now and you owe him nothing."), "burner: his package while it runs");
	// Quit in makeup: the debt on the preview line and the card.
	for (ss::stake::Deal& D : S.Life.Stakes.Deals)
	{
		if (D.By == ss::stake::Outfit::Sam)
		{
			D.State = ss::stake::DealWalked;
			D.Debt = 114000;
		}
	}
	L = P.Settled();
	WriteList("burner_sam_debt", L);
	Expect(DrewRun(L, "debt's $1,140. 25% til its paid") && Drew(L, "You owe Sam") && DrewRun(L, "$1,140.00"), "burner: the debt Sam's quit left, on his preview line and the card");
}

// ------------------------------------------------------------------ badges

void Badges()
{
	Laptop P("laptop-badges");
	ss::Session& S = P.S;
	P.RL.OpenPage(Page::Career, P.Now);
	DrawList L = P.Settled();
	WriteList("badges_career_progress", L);
	Expect(Spelled(L, "YOUR BADGES") && Spelled(L, "SPONSORS") && DrewRun(L, ss::sponsor::StandingWords(S.SponsorTier())), "career: YOUR BADGES and the sponsors' word for the player");
	Expect(Spelled(L, "LEADERBOARD STANDINGS") && Spelled(L, "RECENT TOURNAMENTS"), "career: the standings and results moved down for the badges");
	// The progress cards say all of it: no line under an ellipsis (Verified Pro's "30 of 30 days clear" is the part a
	// ShiftLink shift resets, §9.1), and every line inside its card.
	{
		const std::vector<ss::badge::Progress> Next = S.BadgeProgress(3);
		bool Whole = true;
		bool Inside = true;
		std::string Said[3];
		for (const ss::ui::DrawCmd& Cmd : L.Cmds)
		{
			if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || Cmd.Text.BaselineY < 424.0f || Cmd.Text.BaselineY > 508.0f || Cmd.Text.X > 24.0f + 3.0f * 312.0f)
			{
				continue;
			}
			Whole = Whole && Cmd.Text.Text.find("\xE2\x80\xA6") == std::string::npos;
			const int Card = std::min(2, static_cast<int>((Cmd.Text.X - 24.0f) / 312.0f));
			const float Right = 24.0f + static_cast<float>(Card) * 312.0f + 300.0f;
			Inside = Inside && Cmd.Text.X + P.M.Width(Cmd.Text.Text, Cmd.Text.Face, Cmd.Text.SizePx) <= Right - 8.0f && Cmd.Text.BaselineY <= 424.0f + 84.0f - 6.0f;
			if (Cmd.Text.Text.size() > 1)
			{
				Said[Card] += (Said[Card].empty() ? "" : " ") + Cmd.Text.Text;
			}
		}
		// Each card's detail, its lines joined back up, word for word.
		bool Told = true;
		for (size_t K = 0; K < Next.size() && K < 3; ++K)
		{
			Told = Told && (Next[K].Detail.empty() || Said[K].find(Next[K].Detail) != std::string::npos);
		}
		Expect(!Next.empty() && Whole && Inside && Told, "career: the progress cards' lines fit whole (the days clear of the day job included)");
	}

	// Leaderboards: a glyph by a name and its hover card (the rules are core-badges').
	P.RL.ShowBoard(ss::net::Board::Earnings, P.Now);
	P.RL.OpenPage(Page::Leaderboards, P.Now);
	L = P.Settled();
	WriteList("badges_board_live", L);
	if (!BadgesIn())
	{
		std::printf("  laptop: the badges' rules aren't in yet (core-badges): glyph, hover and rail checks skipped\n");
	}
	else
	{
		// The first row below the podium whose name has a glyph: hover it.
		const ss::net::Network& Net = ss::net::Shared();
		bool Hovered = false;
		for (int Who = 0; Who < static_cast<int>(Net.Players().size()) && !Hovered; ++Who)
		{
			const ss::badge::Set& Held = ss::badge::Shared().Of(S.Living(), Who, ss::net::DayOf(S.WorldMinutes()));
			if (Held.Held.empty())
			{
				continue;
			}
			const Box Name = RunOf(L, Net.Players()[static_cast<size_t>(Who)].Name, P.M, 452.0f, 860.0f);
			if (!Name.Found || Name.Size < 14.5f)
			{
				continue;
			}
			const float Gx = Name.X + std::min(Name.W, 360.0f - 15.0f * 1.2f - 8.0f) + 6.0f + 8.0f;
			const DrawList H = P.Settled(Gx, Name.Base - 15.0f * 0.36f);
			for (const ss::badge::Badge& B : Held.Held)
			{
				Hovered = Hovered || (!ss::badge::Title(B).empty() && Drew(H, ss::badge::Title(B)));
			}
			if (Hovered)
			{
				WriteList("badges_board_hover", H);
			}
		}
		Expect(Hovered, "boards: a badge by a name, and its hover card");
	}

	// HONORS: the page's own mode.
	const Box Hon = RunOf(L, "Honors", P.M, 90.0f, 120.0f);
	Expect(Hon.Found, "boards: the HONORS pill");
	P.Click(Hon.X + Hon.W / 2.0f, Hon.Base - 4.0f);
	L = P.Settled();
	WriteList("badges_board_honors", L);
	Expect(DrewRun(L, "Hall of Fame") && DrewRun(L, "Players of the Year") && Spelled(L, "YOURS"), "honors: the Hall and the Players of the Year");
	P.RL.ShowBoard(ss::net::Board::Live, P.Now);
	L = P.Settled();
	Expect(!DrewRun(L, "Players of the Year") && Spelled(L, "CIRCUIT CARDS") && DrewRun(L, "Black Card") && DrewRun(L, "Top 10") && Spelled(L, "REIGNING"),
		"boards: the Live board's CIRCUIT CARDS and REIGNING");
	WriteList("badges_board_circuit", L);

	// A player card: the rail and TROPHIES & BADGES.
	int Pick = -1;
	for (int Who = 0; Who < static_cast<int>(S.Living().People().size()) && Pick < 0; ++Who)
	{
		Pick = ss::badge::Shared().Of(S.Living(), Who, ss::net::DayOf(S.WorldMinutes())).Held.size() >= 2 ? Who : Pick;
	}
	Pick = Pick < 0 ? 0 : Pick;
	P.RL.ShowPlayer(Pick, P.Now);
	L = P.Settled();
	WriteList("badges_card_rail", L);
	Expect(Spelled(L, "TROPHIES & BADGES"), "card: the TROPHIES & BADGES tab");
	if (BadgesIn())
	{
		const ss::badge::Set& Held = ss::badge::Shared().Of(S.Living(), Pick, ss::net::DayOf(S.WorldMinutes()));
		bool Labelled = Held.Held.empty();
		for (const ss::badge::Badge& B : Held.Held)
		{
			Labelled = Labelled || Spelled(L, ss::badge::Label(B));
		}
		Expect(Labelled, "card: the rail labels its badges");
	}
	P.RL.ShowCardTab(2);
	L = P.Settled();
	WriteList("badges_card_tab", L);
	Expect(Spelled(L, "BRACELETS AND RINGS"), "card: TROPHIES & BADGES opens on the trophies");
	if (BadgesIn() && !ss::badge::Shared().Of(S.Living(), Pick, ss::net::DayOf(S.WorldMinutes())).Held.empty())
	{
		Expect(Spelled(L, "BADGES"), "card: a tile for each badge under the trophies");
	}
	P.RL.ShowPlayer(-1, P.Now);
}

// ------------------------------------------------------------------ the second screen and the wall calendar

void CircuitPerWorld()
{
	// CIRCUIT CARDS works out its holders and cut lines once per world state (§10, the Leaderboards' draw time). A second
	// career's laptop made where the first one's was, its world at the same revisions and day, shows its own numbers.
	alignas(Laptop) static unsigned char Room[sizeof(Laptop)];
	std::string Want[2];
	bool Shows[2] = {false, false};
	int Rev[2] = {0, 0};
	int HeroRev[2] = {0, 0};
	int Today[2] = {0, 0};
	uint32_t Seed[2] = {0, 0};
	for (int K = 0; K < 2; ++K)
	{
		Laptop* P = new (Room) Laptop(K == 0 ? "laptop-circuit-a" : "laptop-circuit-b");
		P->At(P->S.WorldMinutes() + 150.0 * Day); // into the season: circuit points, cut lines
		const ss::world::World& W = P->S.Living();
		P->RL.ShowBoard(ss::net::Board::Live, P->Now);
		P->RL.OpenPage(Page::Leaderboards, P->Now);
		const DrawList L = P->Settled();
		Rev[K] = W.Revision();
		HeroRev[K] = W.HeroRevision();
		Today[K] = ss::net::DayOf(P->S.WorldMinutes());
		Seed[K] = W.Seed();
		const double Cut = W.CardCutLine(1);
		int Holders = W.HeroCardBest() >= 1 ? 1 : 0;
		for (const ss::world::Npc& N : W.People())
		{
			Holders += N.CardBest >= 1 ? 1 : 0;
		}
		Want[K] = (Cut > 0.0 ? "cut " + ss::Grouped(static_cast<int64_t>(std::llround(Cut))) : std::string("cut \xE2\x80\x94")) + " / " + std::to_string(Holders);
		Shows[K] = Drew(L, Cut > 0.0 ? "cut " + ss::Grouped(static_cast<int64_t>(std::llround(Cut))) : std::string("cut \xE2\x80\x94")) &&
			Drew(L, std::to_string(Holders) + (Holders == 1 ? " holds it" : " hold it"));
		P->~Laptop();
	}
	std::printf("  circuit cards: worlds %08x/%08x at rev %d/%d, hero rev %d/%d, day %d/%d: %s and %s\n", Seed[0], Seed[1], Rev[0], Rev[1], HeroRev[0], HeroRev[1], Today[0],
		Today[1], Want[0].c_str(), Want[1].c_str());
	Expect(Shows[0] && Shows[1], "boards: CIRCUIT CARDS shows each world's own cut line and holders, a second career's laptop where the first's was");
}

void RingWinnerCard()
{
	// §9.1: a ring winner's card (ss.World.Award): the champion's frame on the face, the rest of what they hold on the rail,
	// and no second ring there (C2: the frame is the badge). The player's own card, then a champion's from the world.
	Laptop P("laptop-ring-card");
	ss::Session& S = P.S;
	S.Living().GrantAward(-1, true, false);
	const ss::badge::Badge* Ring = S.HeroBadges().Find(ss::badge::Kind::Ring);
	Expect(Ring != nullptr, "ring card: the award is a ring the player holds");
	if (!Ring)
	{
		return;
	}
	const std::string RingLabel = ss::badge::Label(*Ring);
	P.RL.ShowPlayer(ss::ui::RiverLine::HeroCard, P.Now);
	const DrawList Mine = P.Settled();
	WriteList("badges_card_ring_hero", Mine);
	Expect(!RingLabel.empty() && Drew(Mine, "Your tournaments on RiverLine since Night One") && !Spelled(Mine, RingLabel),
		("ring card: the player's card shows the ring in its frame, not again on the rail (" + RingLabel + ")").c_str());
	P.RL.ShowPlayer(-1, P.Now);
	// Someone the world has crowned: their bracelets and rings are their frame, not rail glyphs.
	const ss::world::World& W = S.Living();
	int Champ = -1;
	for (const ss::world::Npc& N : W.People())
	{
		if (Champ < 0 && !N.Awards.empty() && N.Rings > 0 && N.Bracelets == 0)
		{
			Champ = N.Id;
		}
	}
	if (Champ < 0)
	{
		std::printf("  ring card: nobody in this world has only rings: their card skipped\n");
		return;
	}
	const ss::badge::Set& Held = ss::badge::Shared().Of(W, Champ, ss::net::DayOf(S.WorldMinutes()));
	const ss::badge::Badge* Theirs = Held.Find(ss::badge::Kind::Ring);
	P.RL.ShowPlayer(Champ, P.Now);
	const DrawList Card = P.Settled();
	WriteList("badges_card_ring_npc", Card);
	Expect(Theirs != nullptr && Drew(Card, W.Get(Champ)->Name) && !Spelled(Card, ss::badge::Label(*Theirs)), "ring card: a ring winner's player card has no second ring on its rail");
}

void Dates()
{
	Laptop P("laptop-dates");
	ss::Session& S = P.S;
	P.At(2.0 * Day + 14.0 * 60.0);
	S.BankrollCents = 130000;
	S.DebugStake("offer lou");
	for (const ss::stake::Offer& O : std::vector<ss::stake::Offer>(S.Life.Stakes.Offers))
	{
		S.AcceptStake(O.Id);
	}
	DrawList Tile;
	{
		ss::ui::Canvas C(Tile, P.M, ss::ui::secondscreen::Width, ss::ui::secondscreen::Height, 1.0f);
		ss::ui::secondscreen::Draw(C, S, 0, P.Now);
	}
	WriteList("second_screen_marks", Tile);
	Expect(DrewRun(Tile, "COMING UP") && Drew(Tile, "Big Lou's putting you in: Nightly"), "second screen: what's coming up beside what came in");
	DrawList Wall;
	{
		ss::ui::Canvas C(Wall, P.M, ss::ui::wallcalendar::Width, ss::ui::wallcalendar::Height, 1.4f);
		ss::ui::wallcalendar::Draw(C, S);
	}
	WriteList("wall_calendar_marks", Wall);
	Expect(DrewRun(Wall, "BACKED"), "wall calendar: the stake's night written on the month");
	const std::string Key = ss::ui::wallcalendar::Key(S);
	S.Life.Stakes.Deals.clear();
	Expect(ss::ui::wallcalendar::Key(S) != Key, "wall calendar: its key moves with the marks");
}

// ------------------------------------------------------------------ the results screen's split

void Results()
{
	Laptop P("laptop-results");
	ss::Session& S = P.S;
	ss::HistoryEntry H;
	H.Name = "$11 Midnight Grind";
	H.Place = 4;
	H.Entrants = 410;
	H.Prize = 41000;
	H.EventId = "grind@1500";
	S.History.insert(S.History.begin(), H);
	S.LastResults = ss::Results();
	S.LastResults.EventName = H.Name;
	S.LastResults.Place = 4;
	S.LastResults.Entrants = 410;
	S.LastResults.PrizeCents = 41000;
	S.LastResults.BuyInCents = 1100;
	S.HasResults = true;
	S.CurrentScreen = ss::Screen::Results;
	ss::stake::Piece Pc;
	Pc.EntryId = H.EventId;
	Pc.Permille = 300;
	Pc.Share = 12300;
	Pc.Settled = true;
	S.Life.Stakes.Pieces.push_back(Pc);
	DrawList L = P.Settled();
	WriteList("results_split", L);
	Expect(Drew(L, "Prize $410.00 \xC2\xB7 buyers' share \xE2\x88\x92$123.00 \xC2\xB7 yours $287.00"), "results: the buyers' share and the player's");
	S.Life.Stakes.Pieces.clear();
	ss::stake::Deal D;
	D.Id = "lanternrow:test";
	D.K = ss::stake::Kind::Package;
	D.By = ss::stake::Outfit::LanternRow;
	D.Backer = "Lantern Row Staking";
	S.Life.Stakes.Deals.push_back(D);
	ss::stake::Fund F;
	F.EventId = H.EventId;
	F.DealId = D.Id;
	F.Settled = true;
	S.Life.Stakes.Funds.push_back(F);
	L = P.Settled();
	Expect(Drew(L, "went into Lantern Row Staking's roll. Your cut comes at the chop."), "results: a package's prize goes into the roll");
}

// ------------------------------------------------------------------ the CONTINUE card

/** The main menu's CONTINUE card (§5.6): the club crest and the network's one badge at 24, with their words, in the card. */
void ContinueCard()
{
	Laptop P("laptop-continue");
	(void)P.S.DebugBadge("grant club 2");
	(void)P.S.DebugBadge("grant Card 3");
	// Two results, the newest first as the session keeps them: LAST EVENT is the newest.
	for (const char* Name : {"$1.10 Night Owl Turbo", "$3.30 Daily Grind"})
	{
		ss::HistoryEntry H;
		H.Name = Name;
		H.Place = 12;
		H.Entrants = 180;
		P.S.History.insert(P.S.History.begin(), H);
	}
	Expect(ss::ui::DescribeSession(P.S, true).LastResult == "$3.30 Daily Grind \xC2\xB7 12th", "continue card: LAST EVENT is the newest result");
	ss::ui::FrontEndHooks H;
	ss::ui::FrontEnd Fe(H);
	Fe.Info = ss::ui::DescribeSession(P.S, true);
	double Now = 100.0;
	Fe.Open(ss::ui::FrontEnd::Page::Attract, Now);
	Now += 3.0;
	Fe.Key("Any", Now);
	DrawList L;
	for (int K = 0; K < 45; ++K)
	{
		L = DrawList();
		ss::ui::Canvas C(L, P.M, 1920.0f, ss::ui::FrontEnd::Height, 1.0f);
		Now += 1.0 / 30.0;
		Fe.Draw(C, Now);
	}
	WriteList("menu_main_earned", L);
	ss::badge::Badge Club;
	Club.K = ss::badge::Kind::Club;
	Club.Tier = 2;
	const std::string Top = Fe.Info.TopBadge.K == ss::badge::Kind::Count ? std::string() : ss::badge::Label(Fe.Info.TopBadge);
	const Box Crest = RunOf(L, ss::badge::Label(Club), P.M);
	const Box Mark = Top.empty() ? Box() : RunOf(L, Top, P.M, Crest.Base - 1.0f, Crest.Base + 1.0f);
	// On one line, the network's badge right of the crest's word, with the crest's glyph between them.
	Expect(Fe.Info.ClubTier == 2 && !Top.empty() && Spelled(L, "EARNED") && Crest.Found && Mark.Found && Mark.X > Crest.X + Crest.W + 24.0f,
		"continue card: EARNED, the club's tier and the network's badge, each with its glyph");
	// Right-aligned with the card's other values (the bankroll's), so it never leaves the card.
	const Box Bank = RunOf(L, ss::Money(P.S.BankrollCents), P.M);
	Expect(Bank.Found && Mark.Found && std::fabs((Mark.X + Mark.W) - (Bank.X + Bank.W)) < 1.0f, "continue card: the marks end where the card's values do");
}

// ------------------------------------------------------------------ a fresh start (update 2 §3.6)

/** Every text run from x X0 on, in draw order, joined with spaces (a wrapped paragraph read back whole). */
std::string JoinedFrom(const DrawList& L, float X0, float X1 = 1.0e9f)
{
	std::string Out;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.X >= X0 && Cmd.Text.X <= X1)
		{
			Out += (Out.empty() ? "" : " ") + Cmd.Text.Text;
		}
	}
	return Out;
}

void FreshStart()
{
	// Night One, 2:07 AM, a fresh start: every board but Night Shift says nobody's scored, Earnings that nobody's cashed;
	// the about lines count from Oct 6; the Hall says why nobody's eligible; the player's stats say why there's no rank.
	ss::hero::Character Who;
	Laptop P("laptop-fresh");
	ss::Session& S = P.S;
	S.NewCareer(Who, ss::world::Start::Blank);
	S.CurrentScreen = ss::Screen::Lobby;
	P.Now = Step(S, P.Now, 0.2);
	Expect(S.WorldStart() == ss::world::Start::Blank, "fresh: a Blank world");
	struct BoardCase
	{
		ss::net::Board B;
		const char* Empty;
	};
	const BoardCase Cases[4] = {{ss::net::Board::NightShift, "No scores yet tonight."},
		{ss::net::Board::Season, "Nobody's scored yet."},
		{ss::net::Board::Earnings, "Nobody's cashed on RiverLine yet."},
		{ss::net::Board::Wins, "Nobody's scored yet."}};
	P.RL.OpenPage(Page::Leaderboards, P.Now);
	for (const BoardCase& C : Cases)
	{
		P.RL.ShowBoard(C.B, P.Now);
		const DrawList L = P.Settled();
		const std::vector<ss::net::BoardRow> Rows = ss::net::Shared().Leaderboard(C.B, S.WorldMinutes(), ss::net::HeroStats(), 60, nullptr);
		Expect(!Rows.empty() || DrewRun(L, C.Empty), (std::string("fresh: an empty board says so (board ") + std::to_string(static_cast<int>(C.B)) + ")").c_str());
		Expect(C.B == ss::net::Board::NightShift || Rows.empty(), (std::string("fresh: nobody's on the board at 2:07 AM (board ") + std::to_string(static_cast<int>(C.B)) + ")").c_str());
		if (C.B == ss::net::Board::Season)
		{
			WriteList("fresh_boards_season", L);
			Expect(JoinedFrom(L, 1092.0f).find("Points from every final table since Oct 6, the night the record books opened.") != std::string::npos,
				"fresh: the Season board counts from Oct 6, the night the record books opened");
			Expect(DrewRun(L, "Crowned on New Year's Day. Nobody holds it yet."), "fresh: REIGNING: nobody holds it yet");
		}
		if (C.B == ss::net::Board::Earnings)
		{
			Expect(JoinedFrom(L, 1092.0f).find("Tournament winnings on RiverLine since Oct 6.") != std::string::npos, "fresh: Earnings counts from Oct 6");
		}
	}
	// The Hall: nobody's eligible, and why.
	{
		const DrawList L0 = P.Settled();
		const Box Honors = RunOf(L0, "Honors", P.M);
		P.Click(Honors.X + Honors.W / 2.0f, Honors.Base - 5.0f);
		const DrawList L = P.Settled();
		Expect(JoinedFrom(L, 24.0f, 560.0f).find("Nobody's eligible yet. The Hall wants ten years on the scene.") != std::string::npos, "fresh: the Hall wants ten years on the scene");
		WriteList("fresh_honors", L);
	}
	// The player's stats: a big enough sample, too few regulars on record to rank it against (RoiRank -1).
	S.Living().GrantHeroResults(260);
	P.RL.OpenPage(Page::Career, P.Now);
	P.RL.ShowPlayer(ss::ui::RiverLine::HeroCard, P.Now);
	{
		const DrawList L = P.Settled();
		Expect(S.Living().RoiRank(S.Living().HeroStats().Roi()) < 0.0, "fresh: too few regulars to rank anyone yet");
		// The ROI tile (the second of six, from x 552): the whole sentence, over its two lines, nothing cut.
		Expect(JoinedFrom(L, 552.0f, 700.0f).find("Not enough regulars on record yet to rank you.") != std::string::npos && !Drew(L, "regulars\xE2\x80\xA6"),
			"fresh: the stats page says why there's no rank, whole");
		WriteList("fresh_stats_unranked", L);
	}
	P.RL.ShowPlayer(-1, P.Now);
	// An established world keeps its own words.
	Laptop E("laptop-established");
	E.RL.OpenPage(Page::Leaderboards, E.Now);
	E.RL.ShowBoard(ss::net::Board::Season, E.Now);
	const DrawList Le = E.Settled();
	Expect(JoinedFrom(Le, 1092.0f).find("Points from every final table since January.") != std::string::npos, "established: the Season board counts from January");
}

// ------------------------------------------------------------------ the copy

void Copy()
{
	// Every sentence this track added, as drawn (the copy lint, §7.6).
	const std::vector<std::pair<std::string, std::string>> Lines = {
		{"bank", "Theirs: it never pays the rent."},
		{"bank", "At the counter and on Penny Drop. Never the rent."},
		{"burner", "Quit in makeup and it's a debt: 25% of every cash and shift until it's paid."},
		{"burner", "Quit now and it's a debt: 25% of every cash and every shift until it's paid."},
		{"burner", "Quit now and you owe him nothing."},
		{"burner", "He takes 25% of every cash and every shift until it's paid. You walked out in makeup, and that was the deal."},
		{"embercrest", "With 88 runners: a $9,240 prize pool, the buy-in less the fee."},
		{"embercrest", "Nobody's on the board yet this month. Points come from cashes in the room's own events."},
		{"embercrest", "The room's own board: points from the Embercrest's events this month, the same race the side screens show between levels. No prize, just the room's respect."},
		{"embercrest", "The top card in the room."},
		{"embercrest", "10 more for Ember."},
		{"career", "None yet. The room, the site and the circuit notice before you do."},
		{"boards", "Live Player of the Year 2027: points from every cash at a regional festival or bigger, from the River City Classic to the Grand Circuit and the Championship in Las Vegas. The Embercrest has its own monthly board."},
		{"boards", "Crowned on New Year's Day. Nobody holds it yet."},
		{"boards", "No circuit points yet. They come from cashes at a regional festival or bigger."},
		{"boards", "512 circuit pts to your Card. Standings post on Mondays from week 13."},
		{"honors", "Nobody's in yet. Each July 15 the eligible are weighed: forty or older, ten years on the scene, high stakes played, and a record of bracelets, rings, majors and Player of the Year titles that holds up. Two at most a year."},
		{"honors", "The first are crowned on New Year's Day: the most points on RiverLine all year, and live, from the regional festivals up to the Championship."},
		{"honors", "The titles that stay with a name. The Hall of Fame takes its class each July 15; the Players of the Year are crowned when the year turns, one online and one live. They show on every card and every board."},
		{"honors", "None yet. The Season board is this year's race."},
		{"honors", "Nobody's eligible yet."},
		{"honors", "In the Hall of Fame, class of 2036."},
		{"honors", "2 Player of the Year titles."},
		{"boards", "You hold the Gold Card this season."},
		{"boards", "#41 in circuit points: inside the Card's cut. River City is today."},
		{"boards", "512 circuit pts to your Card. River City is in 9 days."},
		{"boards", "River City is tomorrow."},
		{"embercrest", "Nobody played it: every entry was refunded."},
		{"results", "Prize $410.00 went into Lantern Row Staking's roll. Your cut comes at the chop."},
		{"embercrest", "You're $25.80 short: your end of the buy-in and the bus are $125.80."},
		{"embercrest", "You're $3.20 short: the bus is $5.80."},
		// Update 2 (§3.6): a fresh start's empty boards and their about lines, the Hall, the stats page.
		{"boards", "Nobody's scored yet."},
		{"boards", "Nobody's cashed on RiverLine yet."},
		{"boards", "Player of the Year 2026. Points from every final table since Oct 6, the night the record books opened. The top three win RCOP 2027 Platinum Passes worth $25,000."},
		{"boards", "Tournament winnings on RiverLine since Oct 6. Every cash counts, every buy-in forgotten."},
		{"boards", "Tournament winnings on RiverLine since Oct 6, 2026. Every cash counts, every buy-in forgotten."},
		{"boards", "Points from every final table at Micro Madness, from Oct 6: the series opened before the record books did. The top three win RCOP Main Event packages."},
		{"honors", "Nobody's eligible yet. The Hall wants ten years on the scene."},
		{"stats", "Not enough regulars on record yet to rank you."},
		{"stats", "Not enough regulars on record yet to rank them."},
	};
	Failures += copylint::Run("ui-laptop", Lines, copylint::LetterBody);
}
} // namespace laptop_seg

void SegLaptop()
{
	laptop_seg::Taskbar();
	laptop_seg::Bank();
	// Update 2, G2: Rerun around its app.
	laptop_seg::TaskbarLane();
	laptop_seg::TaskbarTurn();
	laptop_seg::RerunTaskbar();
	laptop_seg::RerunForget();
	laptop_seg::RerunEdit();
	laptop_seg::BankVideo();
	laptop_seg::MailRerun();
	laptop_seg::RerunDates();
	laptop_seg::RerunLabels();
	laptop_seg::Backing();
	laptop_seg::Sizing();
	laptop_seg::SeatPins();
	laptop_seg::OnlineSell();
	laptop_seg::Funding();
	laptop_seg::Flip();
	laptop_seg::BurnerRegulars();
	laptop_seg::Sam();
	laptop_seg::Badges();
	laptop_seg::CircuitPerWorld();
	laptop_seg::RingWinnerCard();
	laptop_seg::Dates();
	laptop_seg::Results();
	laptop_seg::ContinueCard();
	laptop_seg::FreshStart();
	laptop_seg::Copy();
}
} // namespace ui_test
