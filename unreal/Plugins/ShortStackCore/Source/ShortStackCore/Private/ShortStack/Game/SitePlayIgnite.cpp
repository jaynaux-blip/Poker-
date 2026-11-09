// Update 3 (SPEC §2.6, §7.3): Ignite's loops, core-siteplay's file. Sparks and the Prize Counter, the Flame, Dares, the
// Spark Wheel, Crackers, the Wristband, Ignite Levels, looks, Headliners, Beef, the Ouch Bonus, the Sticker Book and theme
// nights. Every random draw is a keyed hash of the world's seed (§1.9); every cash-valued grant goes through the Governor
// (SitePlay.cpp); only paid entries move a loop (W4: OnPaidEntry is never called for a ticket or a freeroll, and a result
// or a hand counts only when its entry's paid marker is there).
//
// A few counters ride inside the book's frozen fields (Life.h froze the structs; each is a whole int in its save line):
//   Dares::WeekClears   bits 0-3 this week's clears, bits 4-7 the run of completed Dare Weeks before this week;
//   Stickers::ScalpCount bits 0-3 this week's Headliner scalps, bit 4 when one fell on Bounty Night (the cap's +1);
//   Spark::OuchCount    bits 0-7 today's Ouch Bonuses, then this week's Flame row (Dares::WeekKey's week, Monday first):
//                       bits 8-14 lit, 16-22 rest days, 24-30 a Spare Match burnt;
//   Dares::Claimed      bits 0-2 the slots claimed, bit 3 the clear's Cracker, bits 4-6 a claimed slot whose Sparks wait on
//                       the Guest List's "Sparks owed" line (G2 fix M4; cleared when that line clears: IgniteOwedCleared);
//   Spark::WheelRing    bits 0-3 the ring spun, bit 4 its Sparks waiting on "Sparks owed" the same way.
// A claim's link to the Guest List: a Sparks claim that queued is on the site's one "Sparks owed" line (by construction, so the
// bit is the link); a ticket that queued is its own hold, found by its key ("1:t:<ticket>:<the claim's key>").
#include "ShortStack/Game/SitePlay.h"

#include "SitePlayPriv.h"
#include "SitesDetail.h"
#include "ShortStack/Common.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Fuse.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Game/WorldLeague.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace site
{
namespace play
{
namespace siteplay_detail
{
namespace
{
// ------------------------------------------------------------------ the catalogs

/** How a Dare is met (DareDef::Kind). */
enum IgniteDareKind : int
{
	IgDkFuseEntry = 1,
	IgDkFlipEntry,
	IgDkEveningEntry,
	IgDkBountyEntry,
	IgDkGtdEntry,
	IgDkEarlyEntry,
	IgDkShowdownWin,
	IgDkFlipWin,
	IgDkCash,
	IgDkKnockout,
	IgDkFuseWin,
	IgDkTopHalf50,
	IgDkFuse3,
	IgDkUnderdog,
	IgDkSevens,
	IgDkSecondBreak,
	IgDkFinalTable,
	IgDkPodium,
	IgDkKnockout3,
	IgDkFuseWin3x,
	IgDkSparkCash,
	IgDkPots3,
	IgDkTop10_100,
	IgDkClean20,
};

/** A Dare's own target (most are 1). */
int IgniteDareTarget(int Kind)
{
	return Kind == IgDkFuse3 || Kind == IgDkKnockout3 || Kind == IgDkPots3 ? 3 : Kind == IgDkClean20 ? 20 : 1;
}

/** Rewards in Sparks by tier (Easy, Medium, Hard) and band (A, B, C), F3. */
const int64_t IgniteDareSparks[3][3] = {{60, 100, 200}, {120, 200, 400}, {200, 400, 800}};
const int64_t IgniteOuchSparks[3] = {100, 300, 600};
constexpr int64_t IgnitePaybackSparks = 500;
constexpr int64_t IgniteScalpSparks = 2000;
constexpr int64_t IgniteDuplicateLookSparks = 500;
constexpr int IgniteScalpCap = 3;
constexpr size_t IgniteShelf = 12;
constexpr int IgniteOuchCap = 3;
constexpr double IgniteStackCap = 1.8;
constexpr size_t IgniteBeefShown = 5; // RIVALS' rows (the view's Beefs)
constexpr size_t IgniteBeefKeep = 24; // names the 30-day window counts (G2 fix M1; about 40 B a beef line)
constexpr unsigned IgniteDareOwed = 16u; // Dares::Claimed's "Sparks owed" bit for slot 0 (slot K: << K)
constexpr int IgniteWheelOwed = 16;      // Spark::WheelRing's "Sparks owed" bit
constexpr int IgniteWheelRingMask = 15;

/** A ticket a loop can hand out: where it's held, what it says, its face. */
struct IgniteTicketDef
{
	int Code;
	const char* Id; // "" for a FUSE ticket: fuse::TicketId(FuseTier)
	int FuseTier;
	const char* Label;
	Chips Face;
};
const IgniteTicketDef IgniteTickets[] = {
	{-1, "", 0, "FUSE $1.10 ticket", 110},
	{-2, "ignite.flip-go", -1, "Flip & Run $1.10 ticket", 110},
	{-3, "ignite.night-glow", -1, "Night Glow $2.20 ticket", 220},
	{-4, "ignite.mystery-pinata", -1, "Mystery Pinata $7.70 ticket", 770},
	{-5, "ignite.spark-jackpot", -1, "The Spark $5.50 ticket", 550},
	{-6, "ignite.big-wave", -1, "The Big Wave $11 ticket", 1100},
};

const IgniteTicketDef* IgniteTicket(int Code)
{
	for (const IgniteTicketDef& T : IgniteTickets)
	{
		if (T.Code == Code)
		{
			return &T;
		}
	}
	return nullptr;
}

std::string IgniteTicketId(const IgniteTicketDef& T)
{
	return T.FuseTier >= 0 ? fuse::TicketId(T.FuseTier) : std::string(T.Id);
}

/** The wheel's band-A tables (F4); negative prizes are ticket codes. */
struct IgniteWedge
{
	int Prize;
	double P;
};
const IgniteWedge IgniteGlowA[] = {{20, 0.55}, {40, 0.30}, {80, 0.12}, {200, 0.029}, {-1, 0.001}};
const IgniteWedge IgniteShineA[] = {{40, 0.50}, {80, 0.30}, {150, 0.15}, {400, 0.047}, {-1, 0.003}};
const IgniteWedge IgniteDazzleA[] = {{80, 0.50}, {150, 0.30}, {300, 0.14}, {800, 0.05}, {-2, 0.008}, {-3, 0.002}};

std::vector<std::pair<int, double>> IgniteBuildWheel(int Ring, int8_t Band)
{
	const IgniteWedge* T = Ring == 0 ? IgniteGlowA : Ring == 1 ? IgniteShineA : IgniteDazzleA;
	const size_t N = Ring == 2 ? 6 : 5;
	const int Scale = Band == 2 ? 7 : Band == 1 ? 2 : 1;
	std::vector<std::pair<int, double>> Out;
	for (size_t I = 0; I < N; ++I)
	{
		int Prize = T[I].Prize;
		double P = T[I].P;
		const bool Second = Ring == 2 && T[I].Prize == -3;
		if (Prize > 0)
		{
			Prize *= Scale;
		}
		else if (Band == 1)
		{
			// Each ticket swapped for one at twice its face: Night Glow $2.20; DAZZLE's second, The Spark $5.50 at 0.16%.
			Prize = Second ? -5 : -3;
			P = Second ? 0.0016 : P;
		}
		else if (Band == 2)
		{
			// x7: Mystery Pinata $7.70; DAZZLE's second, The Big Wave $11 at 0.28%.
			Prize = Second ? -6 : -4;
			P = Second ? 0.0028 : P;
		}
		Out.emplace_back(Prize, P);
	}
	double Sum = 0;
	for (const auto& W : Out)
	{
		Sum += W.second;
	}
	Out[0].second += 1.0 - Sum; // the first row absorbs the difference, so the table sums to 1
	return Out;
}

/** The Prize Counter (F1): 7,000 Sparks a $1.10 ticket, a Spare Match 3,000, the P0 looks. Tab: 0 tickets, 1 looks, 2 extras. */
struct IgnitePrizeExtra
{
	int8_t Tab;
};
const std::vector<PrizeDef>& IgnitePrizes()
{
	static const std::vector<PrizeDef> P = {
		{0, "Flip & Run $1.10 ticket", 7000, "ignite.flip-go", -1, -1},
		{1, "FUSE $1.10 ticket", 7000, fuse::TicketId(0), -1, -1},
		{2, "Night Glow $2.20 ticket", 14000, "ignite.night-glow", -1, -1},
		{3, "Plum Turbo $3.30 ticket", 21000, "ignite.plum-turbo", -1, -1},
		{4, "FUSE $3.30 ticket", 21000, fuse::TicketId(1), -1, -1},
		{5, "The Spark $5.50 ticket", 35000, "ignite.spark-jackpot", -1, -1},
		{6, "FUSE $5.50 ticket", 35000, fuse::TicketId(2), -1, -1},
		{7, "The Big Wave $11 ticket", 70000, "ignite.big-wave", -1, -1},
		{8, "Sunday Blaze $11 ticket", 70000, "ignite.sunday-blaze", -1, -1},
		{9, "Sky High $22 ticket", 140000, "ignite.sky-high", -1, -1},
		{10, "Spare Match", 3000, "", -1, -1},
		{11, "Neon Grid card back", 20000, "", 0, 1},
		{12, "Midnight Boardwalk felt", 40000, "", 1, 1},
		{13, "Arcade Tokens chip set", 25000, "", 2, 1},
		{14, "Bulb Ring frame", 10000, "", 3, 1},
		{15, "Ticket Stub frame", 25000, "", 3, 2},
		{16, "Fireworks win effect", 30000, "", 4, 1},
	};
	return P;
}

/** The Wristband's monthly themes (calendar month 1-12) and their two colours (F6). */
struct IgniteThemeDef
{
	const char* Name;
	uint32_t A, B;
};
const IgniteThemeDef IgniteThemes[12] = {
	{"Frostfire", 0x7fd6ff, 0xff7a5c}, {"Frost Fever", 0xbdf0ff, 0xff4f95}, {"Neon", 0xc6ff52, 0x9466ff}, {"Bloom", 0xff9fc9, 0x83e38f},
	{"Heatwave", 0xffa344, 0xff4b4b}, {"Solstice", 0xffd25c, 0x5cc4ff}, {"Fireflies", 0xdcff6a, 0x2f6b4f}, {"Dog Days", 0xffb36b, 0xe8513a},
	{"Afterparty", 0xc97bff, 0x3ee8d6}, {"Bonfire", 0xff8a3d, 0x7a2e1f}, {"Kindling", 0xffb04a, 0x8f4a2a}, {"Glow", 0xfff1a8, 0xd94dff},
};

/** The looks each slot has (the Locker): the P0 base looks first, then the Wristband months' themed ones. */
struct IgniteLookDef
{
	const char* Name;
	const char* How;
	int Bit;
};
const IgniteLookDef IgniteBaseLooks[5][4] = {
	{{"Sunset", "Yours from the start", 0}, {"Neon Grid", "Prize Counter 20,000", 1}, {nullptr, nullptr, -1}, {nullptr, nullptr, -1}},
	{{"Plum Velvet", "Yours from the start", 2}, {"Midnight Boardwalk", "Prize Counter 40,000", 3}, {nullptr, nullptr, -1}, {nullptr, nullptr, -1}},
	{{"Candy", "Yours from the start", 4}, {"Arcade Tokens", "Prize Counter 25,000", 5}, {nullptr, nullptr, -1}, {nullptr, nullptr, -1}},
	{{"No frame", "Yours from the start", 6}, {"Bulb Ring", "Prize Counter 10,000", 7}, {"Ticket Stub", "Prize Counter 25,000", 8},
		{"Blue Flame", "Keep a 30-day flame", 9}},
	{{"Confetti", "Yours from the start", 10}, {"Fireworks", "Prize Counter 30,000", 11}, {nullptr, nullptr, -1}, {nullptr, nullptr, -1}},
};
const int IgniteBaseCount[5] = {2, 2, 2, 4, 2};
/** The themed looks: the month's card back (T12), felt (T22), frame (T27) and win effect (T29); none for chip sets. */
const int IgniteThemedFirstBit[5] = {12, 24, -1, 36, 48};
const int8_t IgniteThemedBase[5] = {0, 0, 0, 1, 0}; // the base design a themed look restyles (the frame: the Bulb Ring)
const char* const IgniteSlotWord[5] = {"card back", "felt", "chip set", "frame", "win effect"};
const int IgniteThemedTier[5] = {12, 22, -1, 27, 29};
/** The looks a Cracker can hold (its "look piece"): the base looks a Prize Counter also sells. */
const std::pair<int, int> IgniteCrackerLooks[] = {{0, 1}, {1, 1}, {2, 1}, {3, 1}, {3, 2}, {4, 1}};

/**
 * A Cracker's table (F5): its Sparks (exactly one row, by chance), a ticket on top under TicketP, a look piece on top under
 * LookP (1: always), and where it drops. CrackerContents draws from it and CrackerOdds shows it (G2 fix M2, W5), so the Odds
 * drawer can't drift from the draw. Paper's "10% also a sticker" isn't modelled: the Sticker Book has only its firsts.
 */
struct IgniteCrackerDef
{
	std::pair<int64_t, double> Sparks[4];
	size_t Rows;
	double TicketP;
	int TicketCode; // IgniteTickets' code
	const char* TicketWords; // what the open's prize line adds for it
	double LookP;
	const char* From;
};
const IgniteCrackerDef IgniteCrackerTable[3] = {
	{{{100, 0.50}, {150, 0.30}, {250, 0.15}, {600, 0.05}}, 4, 0.0, 0, "", 0.0, "Clear all three Dares, any level-up, Wristband T5"},
	{{{400, 0.50}, {600, 0.30}, {1000, 0.15}, {3000, 0.05}}, 4, 0.02, -1, ", a FUSE $1.10 ticket", 0.30,
		"A Friday or Sunday clear, every fifth level, Wristband T15 and T25, your monthly Ignite anniversary"},
	{{{2000, 0.60}, {3000, 0.25}, {5000, 0.15}, {0, 0.0}}, 3, 0.04, -5, ", The Spark $5.50 ticket", 1.0, "Dare Week, every tenth level, Wristband T30"},
};

bool IgniteOwns(const Looks& L, int Bit)
{
	if (Bit < 0 || Bit >= 64)
	{
		return false;
	}
	if (Bit == 0 || Bit == 2 || Bit == 4 || Bit == 6 || Bit == 10)
	{
		return true; // the defaults
	}
	return ((L.Owned[static_cast<size_t>(Bit / 32)] >> (Bit % 32)) & 1u) != 0;
}

void IgniteGive(Looks& L, int Bit)
{
	if (Bit >= 0 && Bit < 64)
	{
		L.Owned[static_cast<size_t>(Bit / 32)] |= 1u << (Bit % 32);
	}
}

int IgniteLookCountOf(int Slot)
{
	return Slot < 0 || Slot > 4 ? 0 : IgniteBaseCount[Slot] + (IgniteThemedFirstBit[Slot] >= 0 ? 12 : 0);
}

/** A look's bit (-1: no such look). */
int IgniteLookBit(int Slot, int Look)
{
	if (Slot < 0 || Slot > 4 || Look < 0 || Look >= IgniteLookCountOf(Slot))
	{
		return -1;
	}
	if (Look < IgniteBaseCount[Slot])
	{
		return IgniteBaseLooks[Slot][Look].Bit;
	}
	return IgniteThemedFirstBit[Slot] + (Look - IgniteBaseCount[Slot]);
}

/** The Sticker Book (F8): 24 firsts, then 6 secrets (no hint on the page). */
struct IgniteStickerDef
{
	const char* Name;
	const char* How;
};
const IgniteStickerDef IgniteStickers[30] = {
	{"First Light", "Play your first paid entry"},
	{"In the Money", "Cash for the first time"},
	{"Final Nine", "Make your first final table"},
	{"Champion", "Win an Ignite event"},
	{"Week of Fire", "Keep a 7-day flame"},
	{"Two Weeks Lit", "Keep a 14-day flame"},
	{"Ouch", "Collect an Ouch Bonus"},
	{"Payback", "Knock out your Beef"},
	{"Scalp", "Knock out a Headliner"},
	{"Times Ten", "Play a FUSE at x10 or more"}, // C2's trademark pass: not "Big Ten" (a college athletic conference's mark)
	{"Twenty-Five", "Play a FUSE at x25 or more"},
	{"Fuse Winner", "Win a FUSE"},
	{"Local", "Reach Level 10"},
	{"Hotshot", "Reach Level 20"},
	{"Wristband", "Reach Wristband tier 10"},
	{"Top of the Band", "Reach Wristband tier 30"},
	{"Dare Week", "Clear all three Dares five days in one week"},
	{"Clean Sweep", "Clear all three Dares in a day"},
	{"First Spin", "Spin the Spark Wheel"},
	{"Dazzled", "Spin the DAZZLE ring"},
	{"Pop", "Open a Cracker"},
	{"Gold Cracker", "Open a Gold Cracker"},
	{"Shopper", "Buy something at the Prize Counter"},
	{"Big Night", "Cash for $100 or more"},
	{"Seven Deuce", ""},
	{"Four Forty-Four", ""},
	{"Double Trouble", ""},
	{"Blue Flame", ""},
	{"So Close", ""},
	{"Hat Trick", ""}, // C2's trademark pass: not "Dare Devil" (a comic character's mark)
};
enum IgniteStickerBit : int
{
	IgStFirstEntry = 0,
	IgStFirstCash,
	IgStFirstFt,
	IgStFirstWin,
	IgStFlame7,
	IgStFlame14,
	IgStOuch,
	IgStPayback,
	IgStScalp,
	IgStFuse10,
	IgStFuse25,
	IgStFuseWin,
	IgStLevel10,
	IgStLevel20,
	IgStBand10,
	IgStBand30,
	IgStDareWeek,
	IgStClear,
	IgStSpin,
	IgStDazzle,
	IgStCracker,
	IgStGold,
	IgStShop,
	IgStCash100,
	IgStSevenDeuce,
	IgStFourFourFour,
	IgStDouble,
	IgStBlue,
	IgStSoClose,
	IgStDareDevil,
};

void IgniteSticker(Ignite& I, int Bit)
{
	I.St.Bits |= 1ull << Bit;
}

const char* const IgniteTitles[6] = {"Rookie", "Local", "Hotshot", "Star", "Legend", "Icon"};
const char* const IgniteTierNames[3] = {"Easy", "Medium", "Hard"};
const char* const IgniteCrackerNames[3] = {"Paper Cracker", "Neon Cracker", "Gold Cracker"};
const char* const IgniteRingNames[3] = {"GLOW", "SHINE", "DAZZLE"};

/** Theme nights (F10), Monday first. */
struct IgniteNightDef
{
	const char* Name;
	const char* Twist;
};
const IgniteNightDef IgniteNights[7] = {
	{"FLIP MANIA", "Flip & Run pays 1.5 times the Sparks"},
	{"BOUNTY NIGHT", "PKO and Mystery events pay 1.5 times the Sparks"},
	{"FUSE NIGHT", "FUSE pays 1.5 times the Sparks"},
	{"TURBO THURSDAY", "Turbos and hypers pay 1.5 times the Sparks"},
	{"FIREWORKS FRIDAY", "Clearing your Dares drops a Neon Cracker"},
	{"AFTER DARK", "Events from 10 PM to 6 AM pay 1.5 times the Sparks"},
	{"SUNDAY BLAZE", "The Sunday Blaze pays 1.5 times the Sparks"},
};

// ------------------------------------------------------------------ the copy (sentences pass the lint; labels don't stop)
const char* const IgniteSentences[] = {
	"Play one paid entry today to light GLOW.",
	"You've had today's spin. The wheel is back at 6 AM.",
	"The Guest List is full, so your spin waits until a line clears.",
	"The Guest List is full, so this waits in place until a line clears.",
	"That Dare isn't on today's card.",
	"You've claimed that one.",
	"Not done yet. Finish it first.",
	"That tier isn't reached yet.",
	"That tier is claimed.",
	"Nothing to claim on that tier.",
	"That Cracker isn't on your shelf.",
	"There's no Cracker on your shelf.",
	"The Prize Counter doesn't have that.",
	"Not enough Sparks yet.",
	"You already own that look.",
	"You hold two Spare Matches already.",
	"That look isn't yours yet.",
	"There's no such look.",
	"Your flame needs one entry today.",
	"Your flame's at {Days} days.",
	"On ice: your flame waits.",
	"Spare Match lit: your flame survived {Day}.",
	"Spin's ready.",
	"That's 3 of 3: pull your Cracker.",
	"Ouch. That was a {Pct} favourite. Have some Sparks.",
	"Payback. That's {Name} sorted.",
	"Scalp. {Name} is a Headliner.",
	"1 more entry lights SHINE.",
	"2 more light DAZZLE.",
	"1 more lights DAZZLE.",
	"{Sparks} Sparks to Level {Level}.",
	"Your Ignite history counts from today.",
	"Under your Team RiverLine deal, every table here is a strike.",
	"Last night's Spark Wheel: {Sparks} Sparks.",
	"A Cracker opened itself: the shelf holds 12.",
	"Your Ignite anniversary: a Neon Cracker is on the shelf.",
	"It stays on Ignite: it never pays rent.",
};
enum IgniteSentence : int
{
	IgSayLightGlow = 0,
	IgSaySpun,
	IgSaySpinWaits,
	IgSayWaits,
	IgSayNoDare,
	IgSayClaimed,
	IgSayNotDone,
	IgSayTierNotYet,
	IgSayTierClaimed,
	IgSayTierNothing,
	IgSayNoBox,
	IgSayShelfEmpty,
	IgSayNoPrize,
	IgSayNoSparks,
	IgSayOwned,
	IgSayMatches,
	IgSayNotYours,
	IgSayNoLook,
	IgSayNudge,
	IgSayFlameDays,
	IgSayIce,
	IgSayMatchLit,
	IgSaySpinReady,
	IgSayPull,
	IgSayOuch,
	IgSayPayback,
	IgSayScalp,
	IgSayShine,
	IgSayTwoDazzle,
	IgSayOneDazzle,
	IgSayToLevel,
};

const char* IgniteSay(int Line)
{
	return IgniteSentences[Line];
}

/** The Dares' catalog (F3): 8 Easy, 8 Medium, 8 Hard; Needs is the paid entries today a Dare's tier asks for. */
const std::vector<DareDef>& IgniteDares()
{
	static const std::vector<DareDef> D = {
		{101, 0, "Light a Fuse", 1, IgDkFuseEntry},
		{102, 0, "Play a Flip & Run", 1, IgDkFlipEntry},
		{103, 0, "Play an event that starts after 6 PM", 1, IgDkEveningEntry},
		{104, 0, "Play a bounty or mystery event", 1, IgDkBountyEntry},
		{105, 0, "Play a guaranteed event", 1, IgDkGtdEntry},
		{106, 0, "Early bird: play before noon", 1, IgDkEarlyEntry},
		{107, 0, "Win a pot at showdown", 1, IgDkShowdownWin},
		{108, 0, "Win a flip: all in preflop, won", 1, IgDkFlipWin},
		{201, 1, "Cash once", 2, IgDkCash},
		{202, 1, "Knock out a player", 2, IgDkKnockout},
		{203, 1, "Win a Fuse", 2, IgDkFuseWin},
		{204, 1, "Finish in the top half of a field of 50+", 2, IgDkTopHalf50},
		{205, 1, "Light 3 Fuses", 2, IgDkFuse3},
		{206, 1, "Underdog: win an all-in at under 45%", 2, IgDkUnderdog},
		{207, 1, "Lucky sevens: win a pot holding a 7", 2, IgDkSevens},
		{208, 1, "Survive to the second break", 2, IgDkSecondBreak},
		{301, 2, "Final table", 3, IgDkFinalTable},
		{302, 2, "Podium: top 3", 3, IgDkPodium},
		{303, 2, "Knock out 3 players today", 3, IgDkKnockout3},
		{304, 2, "Win a Fuse at x3 or more", 3, IgDkFuseWin3x},
		{305, 2, "Cash in The Spark", 3, IgDkSparkCash},
		{306, 2, "Win 3 pots in a row", 3, IgDkPots3},
		{307, 2, "Top 10% of a field of 100+", 3, IgDkTop10_100},
		{308, 2, "Clean hands: 20 graded decisions at 90% or better", 3, IgDkClean20},
	};
	return D;
}

/**
 * The Dares' key (§1.9): the Ignite account's own, its join minute, in place of the world's seed, because the hooks the
 * Session calls at a paid entry, a result and a hand carry no seed (Session.h is frozen) and every one of them has to
 * know today's three. Reproducible across reloads, watch and skip, and the same three for the whole day.
 */
std::string IgniteDareKey(const life::State& L)
{
	const Account* A = AccountOf(L.Sites, net::SiteId::Ignite);
	return "acct" + std::to_string(A ? static_cast<long long>(std::llround(A->JoinedAt)) : 0LL);
}

/** Today's three Dares: one of each tier, drawn from the day and the band (§1.9), past the formats the player hasn't
 *  unlocked (F3: `Joinable`'s locks; a bounty or mystery event opens at the first cash). A locked draw takes the tier's
 *  next Dare, so only a day that drew it changes (review: "Play a bounty or mystery event" was dealt before the unlock). */
std::array<const DareDef*, 3> IgniteDaresOn(const std::string& Key, int Day, int8_t Band, bool BountyOpen)
{
	std::array<const DareDef*, 3> Out{};
	const std::vector<DareDef>& All = IgniteDares();
	for (int T = 0; T < 3; ++T)
	{
		std::vector<const DareDef*> Pool;
		for (const DareDef& D : All)
		{
			if (D.Tier == T)
			{
				Pool.push_back(&D);
			}
		}
		const uint32_t H = Fnv1a(Key + ":ignite:dares:" + std::to_string(Day) + ":" + std::to_string(static_cast<int>(Band)) + ":" + std::to_string(T));
		size_t Pick = Pool.empty() ? 0 : H % Pool.size();
		for (size_t Step = 0; Step < Pool.size() && !BountyOpen && Pool[Pick]->Kind == IgDkBountyEntry; ++Step)
		{
			Pick = (Pick + 1) % Pool.size();
		}
		Out[static_cast<size_t>(T)] = Pool.empty() ? nullptr : Pool[Pick];
	}
	return Out;
}

bool IgniteBountyOpen(const life::State& L)
{
	return L.Unlocks.count("bounty") > 0;
}

std::array<const DareDef*, 3> IgniteToday(const life::State& L, const Ignite& I)
{
	return IgniteDaresOn(IgniteDareKey(L), I.D.DayKey, I.D.Band, IgniteBountyOpen(L));
}

int64_t IgniteTierCost(int T)
{
	return 700LL * std::min(T, 10) + 2800LL * std::max(0, std::min(T, 20) - 10) + 7000LL * std::max(0, T - 20);
}

/** The Wristband tier a month's base Sparks reach. */
int IgniteTierOf(int64_t BaseMonth)
{
	int T = 0;
	while (T < 30 && BaseMonth >= IgniteTierCost(T + 1))
	{
		++T;
	}
	return T;
}

/** What a tier gives (F6): 0 nothing, 1 ticket, 2 cracker, 3 Spare Match, 4 look; T30 the ticket, a Gold Cracker, the title. */
int8_t IgniteTierKind(int T)
{
	return T == 10 || T == 20 || T == 30 ? 1 : T == 5 || T == 15 || T == 25 ? 2 : T == 8 || T == 18 ? 3 : T == 12 || T == 22 || T == 27 || T == 29 ? 4 : 0;
}

int IgniteRing(int Entries)
{
	return Entries >= 4 ? 3 : Entries >= 2 ? 2 : Entries >= 1 ? 1 : 0;
}

double IgniteFlameMult(int Streak)
{
	return Streak <= 0 ? 1.0 : 1.0 + 0.05 * static_cast<double>(std::min(Streak - 1, 4));
}

int8_t IgniteFlameLook(int Streak)
{
	return static_cast<int8_t>(Streak <= 0 ? 0 : Streak <= 2 ? 1 : Streak <= 6 ? 2 : Streak <= 13 ? 3 : Streak <= 29 ? 4 : 5);
}

const net::EventTemplate* IgniteTemplateOf(const std::string& TemplateOrEvent)
{
	std::string T = TemplateOrEvent.substr(0, TemplateOrEvent.find('@'));
	if (T.compare(0, 7, "ignite.") != 0)
	{
		T = "ignite." + T;
	}
	return net::Site(net::SiteId::Ignite).FindTemplate(T);
}

/** A template's daily, its site's prefix off: a launch-week one-off ("ignite.spark-jackpot-launch3") reads as its daily. */
std::string IgniteDailyOf(const std::string& TemplateId)
{
	const std::string Base = net::netsites_detail::NetSitesLaunchBase(TemplateId);
	return SitePlayTemplate(Base.empty() ? TemplateId : Base);
}

bool IgniteOnIce(const sponsor::Book* Deals)
{
	return Deals && sponsor::WouldStrike(*Deals, static_cast<int>(net::SiteId::Ignite), false, false);
}

int8_t IgniteBandOfRec(const FuseRec& R)
{
	Chips Sum = 0;
	int N = 0;
	for (const Chips C : R.RecentBuyIns)
	{
		if (C > 0)
		{
			Sum += C;
			++N;
		}
	}
	const Chips Avg = N > 0 ? Sum / N : 110;
	return static_cast<int8_t>(Avg <= 220 ? 0 : Avg <= 550 ? 1 : 2);
}

// The week's Flame row inside Spark::OuchCount (the file's header comment).
int IgniteOuchToday(const Spark& S, int Day)
{
	return S.OuchDay == Day ? (S.OuchCount & 0xff) : 0;
}

void IgniteRowMark(Spark& S, int Day, int Kind)
{
	// Kind 0 lit, 1 rest, 2 a Spare Match burnt.
	const int Bit = 8 + 8 * Kind + net::Weekday(Day);
	S.OuchCount |= 1 << Bit;
}

/** Moves the book's week to Week: the Dare Week's run, the Flame row cleared. */
void IgniteWeekTo(Ignite& I, int Week)
{
	Dares& D = I.D;
	if (D.WeekKey == Week)
	{
		return;
	}
	const int Clears = D.WeekClears & 15;
	const int Run = (D.WeekClears >> 4) & 15;
	const int NextRun = D.WeekKey >= 0 && Week == D.WeekKey + 1 && Clears >= 5 ? std::min(15, Run + 1) : 0;
	D.WeekKey = Week;
	D.WeekClears = NextRun << 4;
	I.S.OuchCount &= 0xff;
}

/** The Flame's day with no paid entry (F2): two rest days a week free, then a Spare Match, then the streak goes. */
void IgniteMiss(Ignite& I, int Day)
{
	Flame& F = I.F;
	if (F.Streak <= 0 || Day <= F.LastDay)
	{
		return;
	}
	if (F.IceSince >= 0 && Day >= F.IceSince)
	{
		return; // on ice: misses don't count
	}
	const int Week = PlayWeek(Day);
	if (F.RestWeek != Week)
	{
		F.RestWeek = Week;
		F.RestUsed = 0;
	}
	if (F.RestUsed < 2)
	{
		++F.RestUsed;
		IgniteRowMark(I.S, Day, 1);
	}
	else if (F.Matches > 0)
	{
		--F.Matches;
		IgniteRowMark(I.S, Day, 2);
	}
	else
	{
		F.Streak = 0;
		F.RunDays = 0;
	}
}

std::string IgniteThemedName(int Slot, int Month)
{
	return std::string(IgniteThemes[Month].Name) + " " + IgniteSlotWord[Slot];
}

/** The calendar month (1-12) of a play month key. */
int IgniteMonthOfKey(int MonthKey)
{
	return ((MonthKey % 12) + 12) % 12 + 1;
}

/** The Wristband tier's reward line. */
std::string IgniteTierReward(int T, int MonthKey)
{
	const int M = IgniteMonthOfKey(MonthKey) - 1;
	switch (T)
	{
	case 5: return IgniteCrackerNames[0];
	case 8:
	case 18: return "Spare Match";
	case 10: return "Flip & Run $1.10 ticket";
	case 12: return IgniteThemedName(0, M);
	case 15:
	case 25: return IgniteCrackerNames[1];
	case 20: return "Night Glow $2.20 ticket";
	case 22: return IgniteThemedName(1, M);
	case 27: return IgniteThemedName(3, M);
	case 29: return IgniteThemedName(4, M);
	case 30: return "The Spark $5.50 ticket, a Gold Cracker and the month's title";
	default: return std::string();
	}
}

/** The governed value of a Sparks grant, and its payload. */
SitePlayPayload IgniteSparksPay(int64_t Sparks)
{
	SitePlayPayload P;
	P.Sparks = Sparks;
	return P;
}

SitePlayPayload IgniteTicketPay(const std::string& Id, const std::string& Label)
{
	SitePlayPayload P;
	P.TicketId = Id;
	P.Label = Label;
	return P;
}

bool IgniteGrantSparks(Book& B, life::State& L, Kind K, const std::string& What, int64_t Sparks, const std::string& Key, int Day, double World, bool Reset)
{
	return SitePlayGrant(B, L, net::SiteId::Ignite, K, What, SparksValue(Sparks), IgniteSparksPay(Sparks), Key, Day, World, Reset);
}

bool IgniteGrantTicket(Book& B, life::State& L, Kind K, const std::string& Id, const std::string& Label, Chips Face, const std::string& Key, int Day, double World,
	bool Reset)
{
	return SitePlayGrant(B, L, net::SiteId::Ignite, K, Label, Face * ValuePerCent, IgniteTicketPay(Id, Label), Key, Day, World, Reset);
}

int64_t IgniteLevelNeedsOf(int Level)
{
	if (Level <= 1)
	{
		return 0;
	}
	// 100 x (n - 1)^2.4 = 100 x m^2 x (m^2)^(1/5): the fifth root by Newton's method (only + - * /, so every platform reads
	// the same thresholds; DetPow's few parts in a million put L15 one Spark short of the table's 56,325).
	const double M2 = static_cast<double>(Level - 1) * static_cast<double>(Level - 1);
	double X = 1.0 + (M2 - 1.0) / 5.0;
	for (int K = 0; K < 200; ++K)
	{
		const double X4 = X * X * X * X;
		const double Next = X - (X4 * X - M2) / (5.0 * X4);
		if (Next == X)
		{
			break;
		}
		X = Next;
	}
	return static_cast<int64_t>(std::floor(100.0 * M2 * X + 1e-7));
}

/** Does this entry move this Dare (an entry-time Dare)? */
bool IgniteEntryMoves(int Kind, bool Fuse, const net::EventTemplate* T, double Start, double World)
{
	const double StartOfDay = std::fmod(std::fmod(Start, 1440.0) + 1440.0, 1440.0);
	const double NowOfDay = std::fmod(std::fmod(World, 1440.0) + 1440.0, 1440.0);
	switch (Kind)
	{
	case IgDkFuseEntry:
	case IgDkFuse3: return Fuse;
	case IgDkFlipEntry: return !Fuse && T && T->Fmt == net::Format::Flip;
	case IgDkEveningEntry: return StartOfDay >= 18.0 * 60.0;
	case IgDkBountyEntry: return !Fuse && T && (T->Fmt == net::Format::Bounty || T->Fmt == net::Format::Mystery);
	case IgDkGtdEntry: return !Fuse && T && T->GtdCents > 0;
	case IgDkEarlyEntry: return NowOfDay >= 6.0 * 60.0 && NowOfDay < 12.0 * 60.0;
	default: return false;
	}
}

void IgniteProg(Dares& D, int Slot, int Add, int Target)
{
	const int Was = D.Prog[static_cast<size_t>(Slot)];
	D.Prog[static_cast<size_t>(Slot)] = static_cast<int16_t>(std::min(Target, Was + Add));
}

/** A Box onto the shelf (F5): ids "<source>:<day>:<n>"; the 13th opens itself into Sparks at the next step. */
void IgniteShelve(Ignite& I, int8_t Tier, const std::string& Id, int Day)
{
	for (const Box& X : I.Boxes)
	{
		if (X.Id == Id)
		{
			return; // keyed: one drop per id
		}
	}
	Box X;
	X.Id = Id;
	X.Tier = Tier;
	X.EarnedAt = Day;
	I.Boxes.push_back(X);
}

/** Opens one Box: its contents are drawn from its key (CrackerContents), paid through the Governor; a look that's owned turns
 *  into Sparks. False (nothing taken) when it holds a ticket and the claim lines are full. */
bool IgniteOpenOne(Book& B, life::State& L, uint64_t WorldSeed, size_t Index, double World, OpenView& Out)
{
	Ignite& I = B.Ig;
	const Box X = I.Boxes[Index];
	Out = CrackerContents(WorldSeed, X);
	if (!Out.TicketId.empty() && !SitePlayHasItemRoom(B))
	{
		return false;
	}
	I.Boxes.erase(I.Boxes.begin() + static_cast<std::ptrdiff_t>(Index));
	const int Day = PlayDay(World);
	int64_t Sparks = Out.Sparks;
	if (Out.LookSlot >= 0)
	{
		const int Bit = IgniteLookBit(Out.LookSlot, Out.Look);
		if (IgniteOwns(I.L, Bit))
		{
			Sparks += IgniteDuplicateLookSparks;
			Out.Prize += " (owned: +500 Sparks)";
		}
		else
		{
			IgniteGive(I.L, Bit);
		}
	}
	bool Held = false;
	if (Sparks > 0)
	{
		Held = !IgniteGrantSparks(B, L, Kind::CrackerSparks, IgniteCrackerNames[X.Tier], Sparks, "crk:" + X.Id, Day, World, false);
	}
	if (!Out.TicketId.empty())
	{
		const Chips Face = Out.TicketId == fuse::TicketId(0) ? 110 : 550;
		Held = !IgniteGrantTicket(B, L, Kind::Ticket, Out.TicketId, Out.Prize, Face, "crk:" + X.Id, Day, World, false) || Held;
	}
	Out.Held = Held;
	IgniteSticker(I, IgStCracker);
	if (X.Tier == 2)
	{
		IgniteSticker(I, IgStGold);
	}
	return true;
}

/** Credits a reached Wristband tier (a claim, or the 1st's credit with Reset). */
void IgnitePayTier(Book& B, life::State& L, int T, int MonthKey, double World, bool Reset)
{
	Ignite& I = B.Ig;
	const int Day = PlayDay(World);
	const std::string Key = "band:" + std::to_string(MonthKey) + ":" + std::to_string(T);
	const int M = IgniteMonthOfKey(MonthKey) - 1;
	switch (T)
	{
	case 5:
	case 15:
	case 25: IgniteShelve(I, static_cast<int8_t>(T == 5 ? 0 : 1), Key, Day); break;
	case 8:
	case 18: I.F.Matches = std::min(2, I.F.Matches + 1); break;
	case 10: IgniteGrantTicket(B, L, Kind::Ticket, "ignite.flip-go", "Flip & Run $1.10 ticket", 110, Key, Day, World, Reset); break;
	case 20: IgniteGrantTicket(B, L, Kind::Ticket, "ignite.night-glow", "Night Glow $2.20 ticket", 220, Key, Day, World, Reset); break;
	case 12: IgniteGive(I.L, IgniteThemedFirstBit[0] + M); break;
	case 22: IgniteGive(I.L, IgniteThemedFirstBit[1] + M); break;
	case 27: IgniteGive(I.L, IgniteThemedFirstBit[3] + M); break;
	case 29: IgniteGive(I.L, IgniteThemedFirstBit[4] + M); break;
	case 30:
		IgniteGrantTicket(B, L, Kind::Ticket, "ignite.spark-jackpot", "The Spark $5.50 ticket", 550, Key, Day, World, Reset);
		IgniteShelve(I, 2, Key, Day);
		I.St.GuestUntil = std::max(I.St.GuestUntil, Day + 7);
		break;
	default: break;
	}
	I.BandClaimed |= 1u << (T - 1);
	if (T >= 10)
	{
		IgniteSticker(I, IgStBand10);
	}
	if (T >= 30)
	{
		IgniteSticker(I, IgStBand30);
	}
}

/** Clearing all three of a day's Dares (F3): the clear's Cracker (Neon on Sundays and Fireworks Fridays) and Dare Week. */
void IgniteCleared(Ignite& I, int Day)
{
	Dares& D = I.D;
	if ((D.Claimed & 8) != 0)
	{
		return;
	}
	D.Claimed = static_cast<uint8_t>(D.Claimed | 8);
	const int Wd = net::Weekday(Day);
	IgniteShelve(I, static_cast<int8_t>(Wd == 4 || Wd == 6 ? 1 : 0), "dare:" + std::to_string(Day), Day);
	IgniteSticker(I, IgStClear);
	const int Clears = std::min(15, (D.WeekClears & 15) + 1);
	D.WeekClears = (D.WeekClears & ~15) | Clears;
	if (Clears == 5)
	{
		IgniteShelve(I, 2, "dareweek:" + std::to_string(D.WeekKey), Day);
		IgniteSticker(I, IgStDareWeek);
		if (((D.WeekClears >> 4) & 15) + 1 >= 3)
		{
			IgniteSticker(I, IgStDareDevil);
		}
	}
}

/** The day's Dares that are done: progress at its target and the day's paid entries at its tier's count. */
bool IgniteDareDone(const Ignite& I, int Slot, const DareDef& D)
{
	return I.D.Prog[static_cast<size_t>(Slot)] >= IgniteDareTarget(D.Kind) && I.S.DayEntries >= D.Needs;
}

/** Completed but unclaimed Dares credit at 6 AM (F3), through the Governor ("Sparks owed" when it can't pay yet). */
void IgniteCreditDares(Book& B, life::State& L, double World)
{
	Ignite& I = B.Ig;
	if (I.D.DayKey < 0 || I.S.DayKey != I.D.DayKey)
	{
		return;
	}
	const int Day = I.D.DayKey;
	const std::array<const DareDef*, 3> Today = IgniteToday(L, I);
	bool All = true;
	for (int K = 0; K < 3; ++K)
	{
		const DareDef* D = Today[static_cast<size_t>(K)];
		const bool Done = D && IgniteDareDone(I, K, *D);
		All = All && Done;
		if (Done && (I.D.Claimed & (1u << K)) == 0)
		{
			const int64_t Sparks = IgniteDareSparks[D->Tier][std::clamp<int>(I.D.Band, 0, 2)];
			IgniteGrantSparks(B, L, Kind::DareSparks, D->Label, Sparks, "dare:" + std::to_string(Day) + ":" + std::to_string(K), PlayDay(World), World, true);
			I.D.Claimed = static_cast<uint8_t>(I.D.Claimed | (1u << K));
		}
	}
	if (All)
	{
		IgniteCleared(I, Day);
	}
}

/** The Ouch, Payback and Scalp moments for the table's layer (UI state). */
void IgniteMoment(const Book& B, int8_t Kind, const std::string& Name, const std::string& HandId, int64_t Sparks, double Equity, double World, bool Held)
{
	SitePlayScratch& S = SitePlayScratchOf(B);
	MomentView M;
	M.Kind = Kind;
	M.Name = Name;
	M.HandId = HandId;
	M.Sparks = Sparks;
	M.Equity = Equity;
	M.World = World;
	M.Held = Held;
	S.Moments.push_back(M);
	if (S.Moments.size() > 6)
	{
		S.Moments.erase(S.Moments.begin());
	}
}

/** The Beef (F8): the most busts on the player in the window, at least 2 (ties: the most recent). */
const Beef* IgniteBeefOf(const Ignite& I)
{
	const Beef* Best = nullptr;
	for (const Beef& Bf : I.Beefs)
	{
		if (Bf.TheyKo >= 2 && (!Best || Bf.TheyKo > Best->TheyKo || (Bf.TheyKo == Best->TheyKo && Bf.LastDay > Best->LastDay)))
		{
			Best = &Bf;
		}
	}
	return Best;
}

/** Whether the ticket a claim granted under ClaimKey waits on the Guest List: its own hold, "1:t:<ticket>:<ClaimKey>" (M4). */
bool IgniteTicketOwed(const Book& B, const std::string& ClaimKey)
{
	for (const Hold& H : B.Holds)
	{
		const size_t Colon = H.Key.compare(0, 4, "1:t:") == 0 ? H.Key.find(':', 4) : std::string::npos;
		if (Colon != std::string::npos && H.Key.compare(Colon + 1, std::string::npos, ClaimKey) == 0)
		{
			return true;
		}
	}
	return false;
}

/** Which of two names on the Beef list matters less (the first to make way): the lower count (busts both ways), then the
 *  stalest, then the name (so every machine evicts the same one). */
bool IgniteBeefLess(const Beef& A, const Beef& C)
{
	const int Ca = A.TheyKo + A.YouKo, Cc = C.TheyKo + C.YouKo;
	return Ca != Cc ? Ca < Cc : A.LastDay != C.LastDay ? A.LastDay < C.LastDay : A.Name < C.Name;
}

/**
 * The name's row on the Beef list, or null when it doesn't get one (G2 fix M1). The list counts every bust in the 30-day
 * window, past the five names RIVALS shows (IgniteBeefShown), up to IgniteBeefKeep; when it's full the lowest count makes way,
 * then the stalest, so a name with two busts outlasts the singles (review C2: the old five-name list evicted the stalest
 * whatever its count, so a second bust had to land before five others busted you, and a Beef rarely formed or stood).
 * A screen name that isn't a world member can't sit with you again, so it takes a row only while the list is short of the
 * five shown (Member: the world knows the name; the hand hook's BustedByNpc).
 */
Beef* IgniteBeefFor(Ignite& I, const std::string& Name, bool Member, int Day)
{
	for (Beef& Bf : I.Beefs)
	{
		if (Bf.Name == Name)
		{
			return &Bf;
		}
	}
	if (!Member && I.Beefs.size() >= IgniteBeefShown)
	{
		return nullptr;
	}
	if (I.Beefs.size() >= IgniteBeefKeep)
	{
		I.Beefs.erase(std::min_element(I.Beefs.begin(), I.Beefs.end(), IgniteBeefLess));
	}
	Beef Bf;
	Bf.Name = Name;
	Bf.LastDay = Day;
	I.Beefs.push_back(Bf);
	return &I.Beefs.back();
}
} // namespace

// ------------------------------------------------------------------ the shared entry points (SitePlayPriv.h)

int8_t IgniteBand(const Book& B)
{
	return IgniteBandOfRec(B.Ig.Fuse);
}

void IgniteDrop(Book& B, life::State& L, uint64_t WorldSeed, int8_t Tier, const std::string& Id, int Day, double World)
{
	IgniteShelve(B.Ig, Tier, Id, Day);
	// The shelf holds 12: a 13th opens itself into Sparks (and whatever else it holds) at once.
	while (B.Ig.Boxes.size() > IgniteShelf)
	{
		OpenView Ignored;
		if (!IgniteOpenOne(B, L, WorldSeed, B.Ig.Boxes.size() - 1, World, Ignored))
		{
			break;
		}
	}
}

void IgniteOwedCleared(Book& B)
{
	// Everything a Sparks claim queued sat on this one line, so every owed bit is paid now (M4).
	B.Ig.D.Claimed = static_cast<uint8_t>(B.Ig.D.Claimed & ~(IgniteDareOwed * 7u));
	B.Ig.S.WheelRing = static_cast<int8_t>(B.Ig.S.WheelRing & ~IgniteWheelOwed);
}

void IgniteRoll(Book& B, life::State& L, const sponsor::Book* Deals, uint64_t WorldSeed, int Day, double World)
{
	Ignite& I = B.Ig;
	const bool Ice = IgniteOnIce(Deals);
	if (I.S.DayKey < 0)
	{
		// The book's first day: "your Ignite history counts from today".
		I.S.DayKey = Day;
		I.S.DayEntries = 0;
		I.S.DayRakeCents = 0;
		IgniteWeekTo(I, PlayWeek(Day));
		I.D.DayKey = Day;
		I.D.Band = IgniteBandOfRec(I.Fuse);
		I.D.Prog = {};
		I.D.Claimed = 0;
		I.S.MonthKey = PlayMonth(Day);
		I.BandMonth = I.S.MonthKey;
		I.F.IceSince = Ice ? Day : -1;
		return;
	}
	// The clause (§1.5): on ice from the first day it finds running (a skip over the signing ices the whole gap: nothing nags).
	if (Ice && I.F.IceSince < 0)
	{
		I.F.IceSince = I.S.DayKey;
	}
	if (Day > I.S.DayKey)
	{
		// The days that ended: a Dare credit at 6 AM, a miss for each day with no paid entry (on ice, none counts).
		IgniteCreditDares(B, L, World);
		const int From = I.S.DayKey;
		const int Through = std::min(Day - 1, From + 400);
		for (int C = From; C <= Through; ++C)
		{
			IgniteWeekTo(I, PlayWeek(C));
			const bool Played = C == From && I.S.DayEntries > 0;
			if (!Played)
			{
				IgniteMiss(I, C);
			}
		}
		if (Day - 1 > Through)
		{
			I.F.Streak = I.F.IceSince >= 0 ? I.F.Streak : 0; // a year away: the flame went out long ago
		}
		IgniteWeekTo(I, PlayWeek(Day));
		I.S.DayKey = Day;
		I.S.DayEntries = 0;
		I.S.DayRakeCents = 0;
		I.D.DayKey = Day;
		I.D.Band = IgniteBandOfRec(I.Fuse);
		I.D.Prog = {};
		I.D.Claimed = 0;
		// The Wristband's 1st: unclaimed tiers credit by themselves, the month's band starts over.
		const int Month = PlayMonth(Day);
		if (I.BandMonth >= 0 && I.BandMonth != Month)
		{
			const int Reached = IgniteTierOf(I.S.BaseMonth);
			for (int T = 1; T <= Reached; ++T)
			{
				if ((I.BandClaimed & (1u << (T - 1))) == 0 && IgniteTierKind(T) != 0)
				{
					IgnitePayTier(B, L, T, I.BandMonth, World, true);
				}
			}
			I.BandClaimed = 0;
			I.S.BaseMonth = 0;
		}
		I.BandMonth = Month;
		I.S.MonthKey = Month;
		// The Beef window: 30 days.
		I.Beefs.erase(std::remove_if(I.Beefs.begin(), I.Beefs.end(), [Day](const Beef& Bf) { return Bf.LastDay < Day - 30; }), I.Beefs.end());
		// The monthly anniversary of the Ignite account: a Neon Cracker waits on the shelf.
		if (const Account* A = AccountOf(L.Sites, net::SiteId::Ignite))
		{
			int Y = 0, M = 0, Dm = 0, Jy = 0, Jm = 0, Jd = 0;
			world::CivilDate(Day, Y, M, Dm);
			world::CivilDate(PlayDay(A->JoinedAt), Jy, Jm, Jd);
			const std::string Key = "play:anniv:" + std::to_string(Month);
			if (Dm == Jd && (Y != Jy || M != Jm) && !SitePlayDone(L.Sites, Key))
			{
				SitePlayMarkDone(L, Key, World);
				IgniteShelve(I, 1, "anniv:" + std::to_string(Month), Day);
			}
		}
	}
	// The deal ended: the flame's off the ice (the gap's misses were iced above).
	if (!Ice && I.F.IceSince >= 0)
	{
		I.F.IceSince = -1;
	}
	if (WorldSeed != 0)
	{
		while (I.Boxes.size() > IgniteShelf)
		{
			OpenView Ignored;
			if (!IgniteOpenOne(B, L, WorldSeed, I.Boxes.size() - 1, World, Ignored))
			{
				break;
			}
		}
	}
}

void IgnitePaidEntry(Book& B, life::State& L, Chips Fee, Chips BuyIn, bool Fuse, const std::string& EventId, double World)
{
	Ignite& I = B.Ig;
	const int Day = PlayDay(World);
	IgniteRoll(B, L, &L.Sponsors, 0, Day, World);
	// The band's last 20 paid entries (FUSE's are LightFuse's, at entry: fuse::NoteEntry), after the day's roll: the day's
	// band (and its Dares) is the one its 6 AM read, the card the page showed (review: the book's first entry rolled its first
	// day with its own buy-in and dealt another card).
	if (!Fuse)
	{
		for (size_t K = I.Fuse.RecentBuyIns.size() - 1; K > 0; --K)
		{
			I.Fuse.RecentBuyIns[K] = I.Fuse.RecentBuyIns[K - 1];
		}
		I.Fuse.RecentBuyIns[0] = BuyIn;
	}
	SitePlayScratch& Scratch = SitePlayScratchOf(B);
	EntryView Ev;
	Ev.EventId = EventId;
	Ev.LevelFrom = LevelOf(I.S.BaseLife);
	Ev.TierFrom = IgniteTierOf(I.S.BaseMonth);
	const int RingBefore = IgniteRing(I.S.DayEntries);
	++I.S.DayEntries;
	I.S.DayRakeCents += std::max<Chips>(0, Fee);
	// The Flame: today is a flame day.
	if (I.F.LastDay != Day)
	{
		I.F.Streak += 1;
		I.F.LastDay = Day;
		I.F.RunDays += 1;
		if (I.F.RunDays % 5 == 0)
		{
			I.F.Matches = std::min(2, I.F.Matches + 1);
		}
		I.F.Best = std::max(I.F.Best, I.F.Streak);
		IgniteRowMark(I.S, Day, 0);
		if (I.F.Streak >= 7)
		{
			IgniteSticker(I, IgStFlame7);
		}
		if (I.F.Streak >= 14)
		{
			IgniteSticker(I, IgStFlame14);
		}
		if (I.F.Streak >= 30)
		{
			IgniteSticker(I, IgStBlue);
			IgniteGive(I.L, 9); // the permanent blue-flame frame
		}
	}
	// Base Sparks (F1): 10 a cent of rake x the Flame x the theme night, the stack never past x1.8. Never held by the 50%
	// rule; past the week's band cap they're banked for Monday (their Room reserved at once) and still count for progress.
	const double Start = SitePlayStartOf(EventId) >= 0 ? SitePlayStartOf(EventId) : World;
	const double FlameX = IgniteFlameMult(I.F.Streak);
	const double ThemeX = ThemeMult(EventId, Start);
	const double Stack = std::min(IgniteStackCap, FlameX * ThemeX);
	const int64_t Base = std::llround(10.0 * static_cast<double>(std::max<Chips>(0, Fee)) * Stack);
	const int64_t Value = SparksValue(Base);
	SitePlayRoll(B, net::SiteId::Ignite, Day);
	GovRing& G = B.Gov[1];
	const int64_t Cap = SitePlayWeekCap(L, net::SiteId::Ignite, World);
	if (Cap == 0 || static_cast<int64_t>(G.WeekFace) + Value <= Cap)
	{
		SitePlayBook(B, net::SiteId::Ignite, Day, Value, true, false);
		I.S.Balance += Base;
	}
	else
	{
		I.S.Deferred += Base;
		G.Banked = static_cast<int32_t>(std::min<int64_t>(0x7fffffff, static_cast<int64_t>(G.Banked) + Value));
		Ev.Banked = true;
	}
	I.S.BaseLife += Base;
	I.S.BaseMonth += Base;
	Ev.Base = std::llround(10.0 * static_cast<double>(std::max<Chips>(0, Fee)));
	Ev.Sparks = Base;
	Ev.Flame = FlameX;
	Ev.Theme = ThemeX;
	// The Level: each level-up drops a Cracker (Neon every 5th, Gold every 10th).
	while (LevelOf(I.S.BaseLife) > I.LevelClaimed)
	{
		I.LevelClaimed += 1;
		const int Lv = I.LevelClaimed;
		IgniteShelve(I, static_cast<int8_t>(Lv % 10 == 0 ? 2 : Lv % 5 == 0 ? 1 : 0), "lvl:" + std::to_string(Lv), Day);
		if (Lv >= 10)
		{
			IgniteSticker(I, IgStLevel10);
		}
		if (Lv >= 20)
		{
			IgniteSticker(I, IgStLevel20);
		}
	}
	// The entry's Dares (the ones an entry meets: a Fuse, a Flip & Run, an evening start, a bounty, a guarantee, before noon).
	const net::EventTemplate* T = Fuse ? nullptr : IgniteTemplateOf(EventId);
	const std::array<const DareDef*, 3> Today = IgniteToday(L, I);
	for (int K = 0; K < 3; ++K)
	{
		const DareDef* D = Today[static_cast<size_t>(K)];
		const bool WasDone = D && IgniteDareDone(I, K, *D);
		if (D && IgniteEntryMoves(D->Kind, Fuse, T, Start, World))
		{
			IgniteProg(I.D, K, 1, IgniteDareTarget(D->Kind));
		}
		// The entry also counts toward every Dare's paid-entries floor: a result or a hand earlier today may complete now.
		Ev.DareTicks[static_cast<size_t>(K)] = static_cast<int8_t>(D && !WasDone && IgniteDareDone(I, K, *D) ? 1 : 0);
	}
	IgniteSticker(I, IgStFirstEntry);
	const double NowOfDay = std::fmod(std::fmod(World, 1440.0) + 1440.0, 1440.0);
	if (NowOfDay >= 284.0 && NowOfDay < 285.0)
	{
		IgniteSticker(I, IgStFourFourFour);
	}
	Ev.LevelTo = LevelOf(I.S.BaseLife);
	Ev.TierTo = IgniteTierOf(I.S.BaseMonth);
	const int RingAfter = IgniteRing(I.S.DayEntries);
	Ev.RingLit = static_cast<int8_t>(RingAfter > RingBefore ? RingAfter : 0);
	Scratch.Last = Ev;
	Scratch.For = &B;
}

void IgniteResult(Book& B, life::State& L, const SettleFacts& F, bool Paid, double World)
{
	Ignite& I = B.Ig;
	if (!Paid)
	{
		return;
	}
	const int Day = PlayDay(World);
	IgniteRoll(B, L, &L.Sponsors, 0, Day, World);
	// The template as SettleFacts names it (core-sites-g2): a launch-week one-off reads as the daily it stands in for, so The
	// Spark's "Cash in The Spark" and its SPECIAL GUEST work in Ignite's launch week too; a fact with no template reads the id.
	// (review-core-sites-g2's patch #1.)
	const std::string Tmpl = SitePlayTemplate(F.Template.empty() ? F.EventId : F.Template);
	const bool Cash = F.Prize > 0;
	const bool Ft = !F.Fuse && F.Place >= 1 && (F.Final || (F.Place <= 9 && F.Entrants >= 10));
	const double Start = SitePlayStartOf(F.EventId);
	const std::array<const DareDef*, 3> Today = IgniteToday(L, I);
	for (int K = 0; K < 3; ++K)
	{
		const DareDef* D = Today[static_cast<size_t>(K)];
		if (!D)
		{
			continue;
		}
		bool Met = false;
		switch (D->Kind)
		{
		case IgDkCash: Met = Cash; break;
		case IgDkFuseWin: Met = F.Fuse && F.Place == 1; break;
		case IgDkTopHalf50: Met = !F.Fuse && F.Entrants >= 50 && F.Place >= 1 && F.Place * 2 <= F.Entrants; break;
		case IgDkSecondBreak: Met = !F.Fuse && Start >= 0.0 && World - Start >= 115.0; break;
		case IgDkFinalTable: Met = Ft; break;
		case IgDkPodium: Met = !F.Fuse && F.Entrants >= 9 && F.Place >= 1 && F.Place <= 3; break;
		case IgDkFuseWin3x: Met = F.Fuse && F.Place == 1 && F.FuseM >= 3; break;
		case IgDkSparkCash: Met = Tmpl == "spark-jackpot" && Cash; break;
		case IgDkTop10_100: Met = !F.Fuse && F.Entrants >= 100 && F.Place >= 1 && F.Place * 10 <= F.Entrants; break;
		default: break;
		}
		if (Met)
		{
			IgniteProg(I.D, K, 1, IgniteDareTarget(D->Kind));
		}
	}
	// The Sticker Book's firsts; SPECIAL GUEST for a top 3 in The Spark or the Sunday Blaze; the crowd learns the handle.
	if (Cash)
	{
		IgniteSticker(I, IgStFirstCash);
	}
	if (Ft)
	{
		IgniteSticker(I, IgStFirstFt);
	}
	if (F.Place == 1 && !F.Fuse)
	{
		IgniteSticker(I, IgStFirstWin);
	}
	if (F.Fuse)
	{
		if (F.FuseM >= 10)
		{
			IgniteSticker(I, IgStFuse10);
		}
		if (F.FuseM >= 25)
		{
			IgniteSticker(I, IgStFuse25);
		}
		if (F.Place == 1)
		{
			IgniteSticker(I, IgStFuseWin);
		}
		if (F.FuseM >= 25 && F.Place == 2)
		{
			IgniteSticker(I, IgStSoClose);
		}
	}
	if (F.Prize >= 10000)
	{
		I.St.Cashed100 = true;
		IgniteSticker(I, IgStCash100);
	}
	if ((Tmpl == "spark-jackpot" || Tmpl == "sunday-blaze") && F.Place >= 1 && F.Place <= 3)
	{
		I.St.GuestUntil = std::max(I.St.GuestUntil, Day + 7);
	}
}

void IgniteHand(Book& B, life::State* L, const HandNote& N, double World)
{
	Ignite& I = B.Ig;
	const int Day = PlayDay(World);
	if (L)
	{
		IgniteRoll(B, *L, &L->Sponsors, 0, Day, World);
	}
	if (I.D.DayKey != Day)
	{
		return; // the day hasn't rolled yet (the next step does it): no Dare is today's
	}
	// The hand's Dares.
	if (L)
	{
		const std::array<const DareDef*, 3> Today = IgniteToday(*L, I);
		for (int K = 0; K < 3; ++K)
		{
			const DareDef* D = Today[static_cast<size_t>(K)];
			if (!D)
			{
				continue;
			}
			const int Target = IgniteDareTarget(D->Kind);
			const bool Won = N.Won;
			switch (D->Kind)
			{
			case IgDkShowdownWin: IgniteProg(I.D, K, N.Showdown && Won ? 1 : 0, Target); break;
			case IgDkFlipWin: IgniteProg(I.D, K, N.AllIn && N.Preflop && Won ? 1 : 0, Target); break;
			case IgDkKnockout: IgniteProg(I.D, K, N.Knocked.empty() ? 0 : 1, Target); break;
			case IgDkUnderdog: IgniteProg(I.D, K, N.AllIn && Won && N.Equity >= 0.0 && N.Equity < 0.45 ? 1 : 0, Target); break;
			case IgDkSevens: IgniteProg(I.D, K, Won && N.HoleCards.find('7') != std::string::npos ? 1 : 0, Target); break;
			case IgDkKnockout3: IgniteProg(I.D, K, static_cast<int>(N.Knocked.size()), Target); break;
			case IgDkPots3:
				if (I.D.Prog[static_cast<size_t>(K)] < Target)
				{
					I.D.Prog[static_cast<size_t>(K)] = static_cast<int16_t>(Won ? I.D.Prog[static_cast<size_t>(K)] + 1 : 0);
				}
				break;
			case IgDkClean20: IgniteProg(I.D, K, N.Own ? N.GoodGrades : 0, Target); break;
			default: break;
			}
		}
	}
	// The secret: a flip won with seven-deuce.
	if (N.AllIn && N.Preflop && N.Won && N.HoleCards.size() >= 4 && N.HoleCards.find('7') != std::string::npos && N.HoleCards.find('2') != std::string::npos)
	{
		IgniteSticker(I, IgStSevenDeuce);
	}
	if (!L)
	{
		return;
	}
	// The grants a hand pays (they need the life): the Ouch Bonus, Payback, a Headliner's scalp.
	life::State& Lf = *L;
	const int8_t Band = static_cast<int8_t>(std::clamp<int>(I.D.Band, 0, 2));
	if (N.AllIn && N.Lost && N.Equity >= 0.80)
	{
		if (I.S.OuchDay != Day)
		{
			I.S.OuchDay = Day;
			I.S.OuchCount &= ~0xff;
		}
		if ((I.S.OuchCount & 0xff) < IgniteOuchCap)
		{
			I.S.OuchCount += 1;
			const int64_t Sparks = IgniteOuchSparks[Band];
			const bool Paid = IgniteGrantSparks(B, Lf, Kind::OuchSparks, "Ouch Bonus", Sparks, "ouch:" + N.HandId, Day, World, false);
			IgniteSticker(I, IgStOuch);
			IgniteMoment(B, 2, std::string(), N.HandId, Sparks, N.Equity, World, !Paid);
		}
	}
	// Beef (F8): who busted the player, and whom the player busted among the names already on the list.
	const Beef* Was = IgniteBeefOf(I);
	const std::string BeefName = Was ? Was->Name : std::string();
	if (!N.BustedBy.empty())
	{
		if (Beef* Bf = IgniteBeefFor(I, N.BustedBy, N.BustedByNpc >= 0, Day))
		{
			Bf->TheyKo += 1;
			Bf->LastDay = Day;
		}
	}
	bool PaybackToday = false;
	bool ScalpToday = false;
	for (const std::string& Name : N.Knocked)
	{
		for (Beef& Bf : I.Beefs)
		{
			if (Bf.Name == Name)
			{
				Bf.YouKo += 1;
				Bf.LastDay = Day;
			}
		}
		const std::string PayKey = "play:payback:" + std::to_string(Day);
		if (!BeefName.empty() && Name == BeefName && !SitePlayDone(Lf.Sites, PayKey))
		{
			SitePlayMarkDone(Lf, PayKey, World);
			const bool Paid = IgniteGrantSparks(B, Lf, Kind::PaybackSparks, "Payback", IgnitePaybackSparks, "payback:" + std::to_string(Day), Day, World, false);
			IgniteSticker(I, IgStPayback);
			IgniteMoment(B, 0, Name, N.HandId, IgnitePaybackSparks, 0.0, World, !Paid);
			PaybackToday = true;
		}
	}
	// Headliner bounties: 2,000 Sparks a scalp, 3 a week (a scalp on Bounty Night, Tuesday, raises that week's cap by 1).
	const int Week = PlayWeek(Day);
	if (I.St.ScalpWeek != Week)
	{
		I.St.ScalpWeek = Week;
		I.St.ScalpCount = 0;
	}
	for (const std::string& Name : N.Headliners)
	{
		const bool Tuesday = net::Weekday(Day) == 1;
		if (Tuesday)
		{
			I.St.ScalpCount |= 16;
		}
		const int Cap = IgniteScalpCap + ((I.St.ScalpCount & 16) != 0 ? 1 : 0);
		if ((I.St.ScalpCount & 15) >= Cap)
		{
			continue;
		}
		I.St.ScalpCount += 1;
		I.St.ScalpLife += 1;
		const bool Paid = IgniteGrantSparks(B, Lf, Kind::ScalpSparks, "Headliner scalp", IgniteScalpSparks, "scalp:" + N.HandId + ":" + Name, Day, World, false);
		IgniteSticker(I, IgStScalp);
		IgniteMoment(B, 1, Name, N.HandId, IgniteScalpSparks, 0.0, World, !Paid);
		ScalpToday = true;
	}
	// The secret: a Payback and a scalp the same night (either order within the play day).
	const std::string NightKey = "play:night:" + std::to_string(Day);
	if (PaybackToday || ScalpToday)
	{
		const bool HadPayback = SitePlayDone(Lf.Sites, NightKey + ":p");
		const bool HadScalp = SitePlayDone(Lf.Sites, NightKey + ":s");
		if (PaybackToday && !HadPayback)
		{
			SitePlayMarkDone(Lf, NightKey + ":p", World);
		}
		if (ScalpToday && !HadScalp)
		{
			SitePlayMarkDone(Lf, NightKey + ":s", World);
		}
		if ((PaybackToday || HadPayback) && (ScalpToday || HadScalp))
		{
			IgniteSticker(I, IgStDouble);
		}
	}
}

std::string IgniteSpin(Book& B, life::State& L, uint64_t WorldSeed, double World)
{
	Ignite& I = B.Ig;
	const int Day = PlayDay(World);
	if (!SitePlayIgniteStarted(B))
	{
		return IgniteSay(IgSayLightGlow);
	}
	IgniteRoll(B, L, &L.Sponsors, WorldSeed, Day, World);
	const int Ring = IgniteRing(I.S.DayEntries);
	if (Ring == 0)
	{
		return IgniteSay(IgSayLightGlow);
	}
	if (I.S.WheelDay == Day)
	{
		return IgniteSay(IgSaySpun);
	}
	const int8_t Band = static_cast<int8_t>(std::clamp<int>(I.D.Band, 0, 2));
	const std::vector<std::pair<int, double>>& Table = WheelTable(Ring - 1, Band);
	// The draw is made here, at the press, from the day's key; the claim commits (the Session autosaves) and the wheel
	// lands where it drew. A second spin in a day is refused above. (SitePlayDraw: the wedges' chances are its Odds, W5.)
	double U = SitePlayDraw(WorldSeed, "ignite:wheel:" + std::to_string(Day));
	size_t Wedge = Table.size() - 1;
	for (size_t K = 0; K < Table.size(); ++K)
	{
		U -= Table[K].second;
		if (U < 0.0)
		{
			Wedge = K;
			break;
		}
	}
	const int Prize = Table[Wedge].first;
	if (Prize < 0 && !SitePlayHasItemRoom(B))
	{
		return IgniteSay(IgSaySpinWaits);
	}
	SitePlayTouch(L);
	I.S.WheelDay = Day;
	I.S.WheelRing = static_cast<int8_t>(Ring);
	const std::string Key = "wheel:" + std::to_string(Day);
	if (Prize > 0)
	{
		if (!IgniteGrantSparks(B, L, Kind::WheelSparks, std::string(IgniteRingNames[Ring - 1]) + " spin", Prize, Key, Day, World, false))
		{
			I.S.WheelRing = static_cast<int8_t>(I.S.WheelRing | IgniteWheelOwed); // waits on "Sparks owed" (IgniteView::SpunHeld, M4)
		}
	}
	else if (const IgniteTicketDef* T = IgniteTicket(Prize))
	{
		IgniteGrantTicket(B, L, Kind::Ticket, IgniteTicketId(*T), T->Label, T->Face, Key, Day, World, false);
	}
	IgniteSticker(I, IgStSpin);
	if (Ring == 3)
	{
		IgniteSticker(I, IgStDazzle);
	}
	return std::string();
}

std::string IgniteOpen(Book& B, life::State& L, uint64_t WorldSeed, const std::string& BoxId, double World)
{
	Ignite& I = B.Ig;
	if (I.Boxes.empty())
	{
		return IgniteSay(IgSayShelfEmpty);
	}
	IgniteRoll(B, L, &L.Sponsors, WorldSeed, PlayDay(World), World);
	SitePlayScratch& Scratch = SitePlayScratchOf(B);
	Scratch.Opened.clear();
	SitePlayTouch(L);
	if (BoxId == "*")
	{
		// Every one, in shelf order; one with a ticket waits in place while the Guest List is full.
		size_t K = 0;
		while (K < I.Boxes.size())
		{
			OpenView Ov;
			if (IgniteOpenOne(B, L, WorldSeed, K, World, Ov))
			{
				Scratch.Opened.push_back(Ov);
			}
			else
			{
				++K;
			}
		}
		return Scratch.Opened.empty() ? IgniteSay(IgSayWaits) : std::string();
	}
	for (size_t K = 0; K < I.Boxes.size(); ++K)
	{
		if (I.Boxes[K].Id == BoxId)
		{
			OpenView Ov;
			if (!IgniteOpenOne(B, L, WorldSeed, K, World, Ov))
			{
				return IgniteSay(IgSayWaits);
			}
			Scratch.Opened.push_back(Ov);
			return std::string();
		}
	}
	return IgniteSay(IgSayNoBox);
}

std::string IgniteClaimDare(Book& B, life::State& L, int Slot, double World)
{
	Ignite& I = B.Ig;
	const int Day = PlayDay(World);
	if (!SitePlayIgniteStarted(B) || Slot < 0 || Slot > 2)
	{
		return IgniteSay(IgSayNoDare);
	}
	IgniteRoll(B, L, &L.Sponsors, 0, Day, World);
	const std::array<const DareDef*, 3> Today = IgniteToday(L, I);
	const DareDef* D = Today[static_cast<size_t>(Slot)];
	if (!D)
	{
		return IgniteSay(IgSayNoDare);
	}
	if ((I.D.Claimed & (1u << Slot)) != 0)
	{
		return IgniteSay(IgSayClaimed);
	}
	if (!IgniteDareDone(I, Slot, *D))
	{
		return IgniteSay(IgSayNotDone);
	}
	SitePlayTouch(L);
	const int64_t Sparks = IgniteDareSparks[D->Tier][std::clamp<int>(I.D.Band, 0, 2)];
	const bool Paid = IgniteGrantSparks(B, L, Kind::DareSparks, D->Label, Sparks, "dare:" + std::to_string(Day) + ":" + std::to_string(Slot), Day, World, false);
	// Claimed either way; the owed bit says its Sparks wait on "Sparks owed" (DareView::Held) until that line clears (M4).
	I.D.Claimed = static_cast<uint8_t>(I.D.Claimed | (1u << Slot) | (Paid ? 0u : IgniteDareOwed << Slot));
	if ((I.D.Claimed & 7) == 7)
	{
		IgniteCleared(I, Day);
	}
	return std::string();
}

std::string IgniteClaimTier(Book& B, life::State& L, int Tier, double World)
{
	Ignite& I = B.Ig;
	const int Day = PlayDay(World);
	if (!SitePlayIgniteStarted(B) || Tier < 1 || Tier > 30)
	{
		return IgniteSay(IgSayTierNotYet);
	}
	IgniteRoll(B, L, &L.Sponsors, 0, Day, World);
	if (IgniteTierOf(I.S.BaseMonth) < Tier)
	{
		return IgniteSay(IgSayTierNotYet);
	}
	if ((I.BandClaimed & (1u << (Tier - 1))) != 0)
	{
		return IgniteSay(IgSayTierClaimed);
	}
	const int8_t K = IgniteTierKind(Tier);
	if (K == 0)
	{
		return IgniteSay(IgSayTierNothing);
	}
	if (K == 1 && !SitePlayHasItemRoom(B))
	{
		return IgniteSay(IgSayWaits);
	}
	SitePlayTouch(L);
	IgnitePayTier(B, L, Tier, I.BandMonth, World, false);
	return std::string();
}

std::string IgniteBuy(Book& B, life::State& L, int Item, double World)
{
	Ignite& I = B.Ig;
	const std::vector<PrizeDef>& All = IgnitePrizes();
	if (Item < 0 || Item >= static_cast<int>(All.size()))
	{
		return IgniteSay(IgSayNoPrize);
	}
	const PrizeDef& P = All[static_cast<size_t>(Item)];
	if (I.S.Balance < P.Sparks)
	{
		return IgniteSay(IgSayNoSparks);
	}
	if (P.LookSlot >= 0 && IgniteOwns(I.L, IgniteLookBit(P.LookSlot, P.Look)))
	{
		return IgniteSay(IgSayOwned);
	}
	if (P.TicketId.empty() && P.LookSlot < 0 && I.F.Matches >= 2)
	{
		return IgniteSay(IgSayMatches);
	}
	// Spending: the Sparks were counted when they were credited; never again (§1.4).
	SitePlayTouch(L);
	I.S.Balance -= P.Sparks;
	I.S.SpentLife += P.Sparks;
	if (!P.TicketId.empty())
	{
		++L.Tickets[P.TicketId];
		SitePlayDailyLine(L, net::SiteId::Ignite, World, "Prize Counter: ", P.Label);
	}
	else if (P.LookSlot >= 0)
	{
		IgniteGive(I.L, IgniteLookBit(P.LookSlot, P.Look));
	}
	else
	{
		I.F.Matches = std::min(2, I.F.Matches + 1);
	}
	IgniteSticker(I, IgStShop);
	return std::string();
}

std::string IgniteEquip(Book& B, int Slot, int Look)
{
	Ignite& I = B.Ig;
	const int Bit = IgniteLookBit(Slot, Look);
	if (Bit < 0)
	{
		return IgniteSay(IgSayNoLook);
	}
	if (!IgniteOwns(I.L, Bit))
	{
		return IgniteSay(IgSayNotYours);
	}
	int8_t* Field = Slot == 0 ? &I.L.Back : Slot == 1 ? &I.L.Felt : Slot == 2 ? &I.L.Chips : Slot == 3 ? &I.L.Frame : &I.L.Fx;
	*Field = static_cast<int8_t>(Look);
	return std::string();
}

std::string IgniteDebug(Book& B, life::State& L, const std::string& Verb, int64_t N, double World)
{
	Ignite& I = B.Ig;
	const int Day = PlayDay(World);
	SitePlayTouch(L);
	IgniteRoll(B, L, &L.Sponsors, 0, Day, World);
	if (Verb == "sparks")
	{
		const bool Paid = IgniteGrantSparks(B, L, Kind::DareSparks, "Sparks (dev)", std::max<int64_t>(1, N), "dev:" + std::to_string(I.S.SpentLife + I.S.Balance), Day, World, false);
		return Paid ? "granted " + SitePlayCount(N) + " Sparks" : "held on the Guest List: " + SitePlayCount(N) + " Sparks";
	}
	if (Verb == "flame")
	{
		I.F.Streak = static_cast<int>(std::max<int64_t>(0, N));
		I.F.Best = std::max(I.F.Best, I.F.Streak);
		I.F.LastDay = I.F.Streak > 0 ? Day : -1;
		I.F.RunDays = I.F.Streak;
		return "flame set to " + std::to_string(I.F.Streak) + " days";
	}
	if (Verb == "level")
	{
		const int Lv = static_cast<int>(std::clamp<int64_t>(N, 1, 99));
		I.S.BaseLife = std::max(I.S.BaseLife, LevelNeeds(Lv));
		I.LevelClaimed = std::max(I.LevelClaimed, Lv);
		return "level set to " + std::to_string(LevelOf(I.S.BaseLife));
	}
	return "unknown verb";
}

void IgniteFillView(const Book& B, const life::State& L, const sponsor::Book* Deals, const world::World* W, uint64_t WorldSeed, double World, play::IgniteView& Out)
{
	const Ignite& I = B.Ig;
	const int Day = PlayDay(World);
	const bool Started = SitePlayIgniteStarted(B);
	Out.On = true;
	Out.Started = Started;
	Out.FuseOn = true;
	Out.OnIce = IgniteOnIce(Deals) || (Started && I.F.IceSince >= 0 && Day >= I.F.IceSince);
	// The book as of today: a day that hasn't rolled yet reads as a fresh one (no write: the view is const, S10).
	const bool Today = Started && I.S.DayKey == Day;
	const int Entries = Today ? I.S.DayEntries : 0;
	Out.Band = static_cast<int8_t>(Today ? std::clamp<int>(I.D.Band, 0, 2) : IgniteBandOfRec(I.Fuse));
	Out.Sparks = I.S.Balance;
	Out.MondayBank = I.S.Deferred;
	// The Flame.
	Out.Streak = I.F.Streak;
	Out.Best = I.F.Best;
	Out.Matches = I.F.Matches;
	Out.LitToday = I.F.LastDay == Day;
	Out.Mult = IgniteFlameMult(Out.LitToday ? I.F.Streak : I.F.Streak + 1);
	Out.Look = IgniteFlameLook(I.F.Streak);
	const double NowOfDay = std::fmod(std::fmod(World, 1440.0) + 1440.0, 1440.0);
	Out.Nudge = Started && !Out.LitToday && !Out.OnIce && (NowOfDay >= 18.0 * 60.0 || NowOfDay < 6.0 * 60.0);
	Out.RestLeft = I.F.RestWeek == PlayWeek(Day) ? std::max(0, 2 - static_cast<int>(I.F.RestUsed)) : 2;
	const int Wd = net::Weekday(Day);
	const bool RowWeek = I.D.WeekKey == PlayWeek(Day);
	for (int K = 0; K < 7; ++K)
	{
		int8_t Cell = 0;
		const int Lit = RowWeek ? (I.S.OuchCount >> (8 + K)) & 1 : 0;
		const int Rest = RowWeek ? (I.S.OuchCount >> (16 + K)) & 1 : 0;
		const int Burn = RowWeek ? (I.S.OuchCount >> (24 + K)) & 1 : 0;
		if (K == Wd)
		{
			Cell = Out.LitToday ? 1 : Out.OnIce ? 5 : 4;
		}
		else if (K < Wd)
		{
			Cell = Lit ? 1 : Rest ? 2 : Burn ? 6 : Out.OnIce ? 5 : 3;
		}
		Out.Week[static_cast<size_t>(K)] = Cell;
	}
	// Dares.
	const std::array<const DareDef*, 3> Card = IgniteDaresOn(IgniteDareKey(L), Day, Out.Band, IgniteBountyOpen(L));
	for (int K = 0; K < 3; ++K)
	{
		const DareDef* D = Card[static_cast<size_t>(K)];
		DareView& Dv = Out.Dares[static_cast<size_t>(K)];
		if (!D)
		{
			continue;
		}
		Dv.Id = D->Id;
		Dv.Tier = D->Tier;
		Dv.Label = D->Label;
		Dv.Needs = IgniteDareTarget(D->Kind);
		Dv.Entries = D->Needs;
		Dv.Prog = Today ? I.D.Prog[static_cast<size_t>(K)] : 0;
		Dv.Sparks = IgniteDareSparks[D->Tier][Out.Band];
		Dv.Claimed = Today && (I.D.Claimed & (1u << K)) != 0;
		Dv.Held = Dv.Claimed && (I.D.Claimed & (IgniteDareOwed << K)) != 0; // its Sparks wait on "Sparks owed" (M4)
		Dv.Done = Today && I.D.Prog[static_cast<size_t>(K)] >= Dv.Needs && Entries >= D->Needs;
	}
	Out.DaresReset = PlayDayStart(Day + 1);
	Out.WeekClears = RowWeek ? (I.D.WeekClears & 15) : 0;
	Out.ClearCracker = static_cast<int8_t>(Wd == 4 || Wd == 6 ? 1 : 0);
	Out.ClearPaid = Today && (I.D.Claimed & 8) != 0;
	// The Spark Wheel.
	Out.EntriesToday = Entries;
	Out.RingsLit = static_cast<int8_t>(IgniteRing(Entries));
	Out.Spun = I.S.WheelDay == Day;
	Out.SpinReady = Out.RingsLit > 0 && !Out.Spun;
	Out.MoreForNext = Out.RingsLit == 0 ? 1 : Out.RingsLit == 1 ? 1 : Out.RingsLit == 2 ? 4 - Entries : 0;
	Out.NearGoal = Out.RingsLit == 0 ? std::string(IgniteSay(IgSayLightGlow)) : Out.RingsLit == 1 ? std::string(IgniteSay(IgSayShine))
		: Out.RingsLit == 2 ? std::string(IgniteSay(Out.MoreForNext >= 2 ? IgSayTwoDazzle : IgSayOneDazzle)) : std::string();
	for (int R = 0; R < 3; ++R)
	{
		for (const auto& Wg : WheelTable(R, Out.Band))
		{
			WedgeView Wv;
			Wv.P = Wg.second;
			if (Wg.first > 0)
			{
				Wv.Sparks = Wg.first;
				Wv.Label = SitePlayCount(Wg.first) + " Sparks";
			}
			else if (const IgniteTicketDef* T = IgniteTicket(Wg.first))
			{
				Wv.TicketId = IgniteTicketId(*T);
				Wv.FaceCents = T->Face;
				Wv.Label = T->Label;
			}
			Out.Rings[static_cast<size_t>(R)].push_back(Wv);
		}
	}
	const int SpunRing = I.S.WheelRing & IgniteWheelRingMask;
	if (Out.Spun && SpunRing >= 1 && SpunRing <= 3)
	{
		Out.SpunRing = static_cast<int8_t>(SpunRing);
		const std::vector<std::pair<int, double>>& Table = WheelTable(SpunRing - 1, Out.Band);
		double U = SitePlayDraw(WorldSeed, "ignite:wheel:" + std::to_string(Day));
		Out.SpunWedge = static_cast<int>(Table.size()) - 1;
		for (size_t K = 0; K < Table.size(); ++K)
		{
			U -= Table[K].second;
			if (U < 0.0)
			{
				Out.SpunWedge = static_cast<int>(K);
				break;
			}
		}
		// Held (M4): its Sparks on "Sparks owed" (the owed bit), or its ticket's own hold (the spin's key).
		Out.SpunHeld = (I.S.WheelRing & IgniteWheelOwed) != 0 || IgniteTicketOwed(B, "wheel:" + std::to_string(Day));
	}
	// Crackers, their Odds (W5, M2: the draw's own tables, built once), the last open (UI state).
	Out.Crackers = I.Boxes;
	static const std::array<CrackerOddsView, 3> IgniteOdds = {CrackerOdds(0), CrackerOdds(1), CrackerOdds(2)};
	Out.CrackerOdds = IgniteOdds;
	const SitePlayScratch& Scratch = SitePlayScratchOf(B);
	Out.Opened = Scratch.Opened;
	Out.Moments = Scratch.Moments;
	Out.Last = Scratch.Last;
	// The Wristband and the Level.
	const int Month = PlayMonth(Day);
	const bool ThisMonth = I.BandMonth == Month;
	const int64_t BandSparks = ThisMonth ? I.S.BaseMonth : 0;
	Out.BandSparks = BandSparks;
	Out.Tier = IgniteTierOf(BandSparks);
	Out.Theme = IgniteThemes[IgniteMonthOfKey(Month) - 1].Name;
	Out.DaysLeft = std::max(0, world::MonthStart(Day + 32 - (Day - world::MonthStart(Day))) - Day);
	for (int T = 1; T <= 30; ++T)
	{
		TierView& Tv = Out.Tiers[static_cast<size_t>(T - 1)];
		Tv.Tier = T;
		Tv.Needs = IgniteTierCost(T);
		Tv.Kind = IgniteTierKind(T);
		Tv.Reward = IgniteTierReward(T, Month);
		if (T == 30)
		{
			Tv.Kind = 1;
		}
		Tv.Reached = BandSparks >= Tv.Needs;
		Tv.Claimed = ThisMonth && (I.BandClaimed & (1u << (T - 1))) != 0;
		// Held (M4): a ticket tier's ticket waits on its own Guest List hold (IgnitePayTier's key); the rest are never held.
		Tv.Held = Tv.Claimed && Tv.Kind == 1 && IgniteTicketOwed(B, "band:" + std::to_string(Month) + ":" + std::to_string(T));
	}
	Out.Level = LevelOf(I.S.BaseLife);
	Out.Title = IgniteTitles[std::min(5, Out.Level / 10)];
	Out.LevelSparks = I.S.BaseLife;
	Out.LevelFrom = LevelNeeds(Out.Level);
	Out.LevelTo = LevelNeeds(Out.Level + 1);
	// The Prize Counter, the Locker, the Sticker Book.
	const std::vector<PrizeDef>& Prizes = IgnitePrizes();
	for (size_t K = 0; K < Prizes.size(); ++K)
	{
		const PrizeDef& P = Prizes[K];
		PrizeView Pv;
		Pv.Item = static_cast<int>(K);
		Pv.Label = P.Label;
		Pv.Sparks = P.Sparks;
		Pv.TicketId = P.TicketId;
		Pv.FaceCents = P.Sparks * 110 / 7000;
		Pv.LookSlot = P.LookSlot;
		Pv.Look = P.Look;
		Pv.Tab = static_cast<int8_t>(!P.TicketId.empty() ? 0 : P.LookSlot >= 0 ? 1 : 2);
		Pv.Owned = P.LookSlot >= 0 ? IgniteOwns(I.L, IgniteLookBit(P.LookSlot, P.Look)) : (P.TicketId.empty() && I.F.Matches >= 2);
		Pv.Afford = I.S.Balance >= P.Sparks;
		Out.Prizes.push_back(Pv);
	}
	Out.Worn = I.L;
	for (int S = 0; S < 5; ++S)
	{
		for (int K = 0; K < IgniteLookCountOf(S); ++K)
		{
			LookView Lv = LookOf(S, K);
			Lv.Owned = IgniteOwns(I.L, IgniteLookBit(S, K));
			const int8_t Worn = S == 0 ? I.L.Back : S == 1 ? I.L.Felt : S == 2 ? I.L.Chips : S == 3 ? I.L.Frame : I.L.Fx;
			Lv.Worn = Worn == K;
			Out.Locker.push_back(Lv);
		}
	}
	for (int K = 0; K < 30; ++K)
	{
		StickerView Sv;
		Sv.Id = K;
		Sv.Got = ((I.St.Bits >> K) & 1ull) != 0;
		Sv.Secret = K >= 24;
		Sv.Name = !Sv.Secret || Sv.Got ? IgniteStickers[K].Name : "Secret";
		Sv.How = Sv.Secret ? std::string() : IgniteStickers[K].How;
		Out.Book.push_back(Sv);
	}
	// FUSE.
	Out.Fuse = I.Fuse;
	for (int T = 0; T < 3; ++T)
	{
		Out.FuseTicketIds[static_cast<size_t>(T)] = fuse::TicketId(T);
	}
	// People.
	for (const int Id : Headliners(W))
	{
		if (const world::Npc* N = W ? W->Get(Id) : nullptr)
		{
			Out.Headliners.push_back(N->Name);
		}
	}
	// RIVALS shows five of the window's names: the Beef first, then the most busts both ways, the most recent (G2 fix M1: the
	// list itself counts up to IgniteBeefKeep names, in the order they came).
	const Beef* Current = IgniteBeefOf(I);
	std::vector<const Beef*> Ranked;
	for (const Beef& Bf : I.Beefs)
	{
		Ranked.push_back(&Bf);
	}
	std::sort(Ranked.begin(), Ranked.end(), [Current](const Beef* A, const Beef* C) { return A != C && (A == Current || (C != Current && IgniteBeefLess(*C, *A))); });
	for (size_t K = 0; K < Ranked.size() && K < IgniteBeefShown; ++K)
	{
		Out.Beefs.push_back(*Ranked[K]);
	}
	if (Current)
	{
		Out.BeefName = Current->Name;
	}
	Out.ScalpLife = I.St.ScalpLife;
	Out.ScalpWeek = I.St.ScalpWeek == PlayWeek(Day) ? (I.St.ScalpCount & 15) : 0;
	Out.ScalpCap = IgniteScalpCap + (I.St.ScalpWeek == PlayWeek(Day) && (I.St.ScalpCount & 16) != 0 ? 1 : 0);
	Out.GuestUntil = I.St.GuestUntil;
	Out.Guest = I.St.GuestUntil >= Day;
	Out.CrowdKnows = I.St.Cashed100;
	Out.OuchToday = IgniteOuchToday(I.S, Day);
	// The night.
	Out.ThemeNight = static_cast<int8_t>(Wd);
	Out.ThemeName = IgniteNights[Wd].Name;
	Out.ThemeTwist = IgniteNights[Wd].Twist;
}

void IgniteWrite(const Book& B, std::string& Out)
{
	const Ignite& I = B.Ig;
	if (!SitePlayIgniteStarted(B))
	{
		return;
	}
	const Spark& S = I.S;
	Out += "site\tspark\tignite\t" + std::to_string(S.Balance) + "\t" + std::to_string(S.BaseLife) + "\t" + std::to_string(S.BaseMonth) + "\t" + std::to_string(S.MonthKey) + "\t" +
		std::to_string(S.SpentLife) + "\t" + std::to_string(S.WeekKey) + "\t" + std::to_string(S.WeekFaceCents) + "\t" + std::to_string(S.Deferred) + "\t" +
		std::to_string(S.DayKey) + "\t" + std::to_string(S.DayRakeCents) + "\t" + std::to_string(S.DayEntries) + "\t" + std::to_string(S.WheelDay) + "\t" +
		std::to_string(static_cast<int>(S.WheelRing)) + "\t" + std::to_string(S.OuchDay) + "\t" + std::to_string(S.OuchCount) + "\n";
	const Flame& F = I.F;
	Out += "site\tflame\tignite\t" + std::to_string(F.Streak) + "\t" + std::to_string(F.Best) + "\t" + std::to_string(F.LastDay) + "\t" + std::to_string(F.Matches) + "\t" +
		std::to_string(F.RestWeek) + "\t" + std::to_string(F.IceSince) + "\t" + std::to_string(static_cast<int>(F.RestUsed)) + "\t" + std::to_string(F.RunDays) + "\n";
	const Dares& D = I.D;
	Out += "site\tdare\tignite\t" + std::to_string(D.DayKey) + "\t" + std::to_string(static_cast<int>(D.Band)) + "\t" + std::to_string(D.Prog[0]) + "\t" +
		std::to_string(D.Prog[1]) + "\t" + std::to_string(D.Prog[2]) + "\t" + std::to_string(static_cast<int>(D.Claimed)) + "\t" + std::to_string(D.WeekKey) + "\t" +
		std::to_string(D.WeekClears) + "\n";
	for (const Box& X : I.Boxes)
	{
		Out += "site\tbox\tignite\t" + SitePlayEscape(X.Id) + "\t" + std::to_string(static_cast<int>(X.Tier)) + "\t" + std::to_string(X.EarnedAt) + "\n";
	}
	Out += "site\tband\tignite\t" + std::to_string(I.BandMonth) + "\t" + std::to_string(I.BandClaimed) + "\n";
	Out += "site\tlevel\tignite\t" + std::to_string(I.LevelClaimed) + "\n";
	char Hex[24];
	std::snprintf(Hex, sizeof(Hex), "%08x", I.L.Owned[0]);
	std::string Owned0 = Hex;
	std::snprintf(Hex, sizeof(Hex), "%08x", I.L.Owned[1]);
	Out += "site\tlook\tignite\t" + Owned0 + "\t" + std::string(Hex) + "\t" + std::to_string(static_cast<int>(I.L.Back)) + "\t" + std::to_string(static_cast<int>(I.L.Felt)) + "\t" +
		std::to_string(static_cast<int>(I.L.Chips)) + "\t" + std::to_string(static_cast<int>(I.L.Frame)) + "\t" + std::to_string(static_cast<int>(I.L.Fx)) + "\n";
	fuse::WriteLine(I.Fuse, Out);
	for (const Beef& Bf : I.Beefs)
	{
		Out += "site\tbeef\tignite\t" + SitePlayEscape(Bf.Name) + "\t" + std::to_string(Bf.TheyKo) + "\t" + std::to_string(Bf.YouKo) + "\t" + std::to_string(Bf.LastDay) + "\n";
	}
	const Stickers& St = I.St;
	Out += "site\tstick\tignite\t" + std::to_string(St.Bits) + "\t" + std::to_string(St.ScalpWeek) + "\t" + std::to_string(St.ScalpCount) + "\t" + std::to_string(St.ScalpLife) +
		"\t" + std::to_string(St.GuestUntil) + "\t" + (St.Cashed100 ? "1" : "0") + "\n";
}

bool IgniteRead(const std::vector<std::string>& P, Book& B)
{
	Ignite& I = B.Ig;
	const std::string& Tag = P[1];
	if (Tag == "spark" && P.size() >= 18)
	{
		Spark& S = I.S;
		S.Balance = SitePlayInt(P[3]);
		S.BaseLife = SitePlayInt(P[4]);
		S.BaseMonth = SitePlayInt(P[5]);
		S.MonthKey = static_cast<int>(SitePlayInt(P[6]));
		S.SpentLife = SitePlayInt(P[7]);
		S.WeekKey = static_cast<int>(SitePlayInt(P[8]));
		S.WeekFaceCents = SitePlayInt(P[9]);
		S.Deferred = SitePlayInt(P[10]);
		S.DayKey = static_cast<int>(SitePlayInt(P[11]));
		S.DayRakeCents = SitePlayInt(P[12]);
		S.DayEntries = static_cast<int>(SitePlayInt(P[13]));
		S.WheelDay = static_cast<int>(SitePlayInt(P[14]));
		S.WheelRing = static_cast<int8_t>(SitePlayInt(P[15]));
		S.OuchDay = static_cast<int>(SitePlayInt(P[16]));
		S.OuchCount = static_cast<int>(SitePlayInt(P[17]));
		return true;
	}
	if (Tag == "flame" && P.size() >= 11)
	{
		Flame& F = I.F;
		F.Streak = static_cast<int>(SitePlayInt(P[3]));
		F.Best = static_cast<int>(SitePlayInt(P[4]));
		F.LastDay = static_cast<int>(SitePlayInt(P[5]));
		F.Matches = static_cast<int>(SitePlayInt(P[6]));
		F.RestWeek = static_cast<int>(SitePlayInt(P[7]));
		F.IceSince = static_cast<int>(SitePlayInt(P[8]));
		F.RestUsed = static_cast<int8_t>(SitePlayInt(P[9]));
		F.RunDays = static_cast<int>(SitePlayInt(P[10]));
		return true;
	}
	if (Tag == "dare" && P.size() >= 11)
	{
		Dares& D = I.D;
		D.DayKey = static_cast<int>(SitePlayInt(P[3]));
		D.Band = static_cast<int8_t>(SitePlayInt(P[4]));
		D.Prog[0] = static_cast<int16_t>(SitePlayInt(P[5]));
		D.Prog[1] = static_cast<int16_t>(SitePlayInt(P[6]));
		D.Prog[2] = static_cast<int16_t>(SitePlayInt(P[7]));
		D.Claimed = static_cast<uint8_t>(SitePlayInt(P[8]));
		D.WeekKey = static_cast<int>(SitePlayInt(P[9]));
		D.WeekClears = static_cast<int>(SitePlayInt(P[10]));
		return true;
	}
	if (Tag == "box" && P.size() >= 6)
	{
		Box X;
		X.Id = SitePlayUnescape(P[3]);
		X.Tier = static_cast<int8_t>(std::clamp<int64_t>(SitePlayInt(P[4]), 0, 2));
		X.EarnedAt = static_cast<int>(SitePlayInt(P[5]));
		I.Boxes.push_back(X);
		return true;
	}
	if (Tag == "band" && P.size() >= 5)
	{
		I.BandMonth = static_cast<int>(SitePlayInt(P[3]));
		I.BandClaimed = static_cast<uint32_t>(SitePlayInt(P[4]));
		return true;
	}
	if (Tag == "level" && P.size() >= 4)
	{
		I.LevelClaimed = static_cast<int>(SitePlayInt(P[3]));
		return true;
	}
	if (Tag == "look" && P.size() >= 10)
	{
		I.L.Owned[0] = static_cast<uint32_t>(std::strtoul(P[3].c_str(), nullptr, 16));
		I.L.Owned[1] = static_cast<uint32_t>(std::strtoul(P[4].c_str(), nullptr, 16));
		I.L.Back = static_cast<int8_t>(SitePlayInt(P[5]));
		I.L.Felt = static_cast<int8_t>(SitePlayInt(P[6]));
		I.L.Chips = static_cast<int8_t>(SitePlayInt(P[7]));
		I.L.Frame = static_cast<int8_t>(SitePlayInt(P[8]));
		I.L.Fx = static_cast<int8_t>(SitePlayInt(P[9]));
		return true;
	}
	if (Tag == "beef" && P.size() >= 7)
	{
		Beef Bf;
		Bf.Name = SitePlayUnescape(P[3]);
		Bf.TheyKo = static_cast<int>(SitePlayInt(P[4]));
		Bf.YouKo = static_cast<int>(SitePlayInt(P[5]));
		Bf.LastDay = static_cast<int>(SitePlayInt(P[6]));
		if (I.Beefs.size() < IgniteBeefKeep) // the window's bound (S7): a save never carries more than the list keeps
		{
			I.Beefs.push_back(Bf);
		}
		return true;
	}
	if (Tag == "stick" && P.size() >= 9)
	{
		Stickers& St = I.St;
		St.Bits = static_cast<uint64_t>(std::strtoull(P[3].c_str(), nullptr, 10));
		St.ScalpWeek = static_cast<int>(SitePlayInt(P[4]));
		St.ScalpCount = static_cast<int>(SitePlayInt(P[5]));
		St.ScalpLife = static_cast<int>(SitePlayInt(P[6]));
		St.GuestUntil = static_cast<int>(SitePlayInt(P[7]));
		St.Cashed100 = P[8] == "1";
		return true;
	}
	return false;
}

void IgniteCopy(std::vector<std::string>& Sentences, std::vector<std::string>& Labels)
{
	for (const char* S : IgniteSentences)
	{
		Sentences.push_back(S);
	}
	for (const char* S : {"GLOW", "SHINE", "DAZZLE", "Sparks", "Spare Match", "Monday bonus", "SPECIAL GUEST", "BEEF", "PAYBACK", "SCALP", "Headliner", "Ouch Bonus",
			 "Payback", "Headliner scalp", "GLOW spin", "SHINE spin", "DAZZLE spin", "Prize Counter: ", "Rewards: "})
	{
		Labels.push_back(S);
	}
	for (const char* S : IgniteTierNames)
	{
		Labels.push_back(S);
	}
	for (const char* S : IgniteCrackerNames)
	{
		Labels.push_back(S);
	}
	for (const IgniteCrackerDef& C : IgniteCrackerTable)
	{
		Labels.push_back(C.From);
	}
	for (const char* S : IgniteTitles)
	{
		Labels.push_back(S);
	}
	for (const DareDef& D : IgniteDares())
	{
		Labels.push_back(D.Label);
	}
	for (const PrizeDef& P : IgnitePrizes())
	{
		Labels.push_back(P.Label);
	}
	for (const IgniteTicketDef& T : IgniteTickets)
	{
		Labels.push_back(T.Label);
	}
	for (const IgniteNightDef& N : IgniteNights)
	{
		Labels.push_back(N.Name);
		Labels.push_back(N.Twist);
	}
	for (const IgniteStickerDef& S : IgniteStickers)
	{
		Labels.push_back(S.Name);
		if (S.How[0] != '\0')
		{
			Labels.push_back(S.How);
		}
	}
	for (int S = 0; S < 5; ++S)
	{
		for (int K = 0; K < IgniteLookCountOf(S); ++K)
		{
			const LookView Lv = LookOf(S, K);
			Labels.push_back(Lv.Name);
			Labels.push_back(Lv.How);
		}
	}
	for (int T = 1; T <= 30; ++T)
	{
		const std::string R = IgniteTierReward(T, 0);
		if (!R.empty())
		{
			Labels.push_back(R);
		}
	}
	for (const IgniteThemeDef& T : IgniteThemes)
	{
		Labels.push_back(T.Name);
	}
}

std::string IgniteDescribe(const Book& B, double World)
{
	const Ignite& I = B.Ig;
	const int Day = PlayDay(World);
	std::string S = "IGNITE " + std::string(SitePlayIgniteStarted(B) ? "" : "(no book yet) ") + "Sparks " + SitePlayCount(I.S.Balance) + " (base life " + SitePlayCount(I.S.BaseLife) +
		", spent " + SitePlayCount(I.S.SpentLife) + ", Monday bonus " + SitePlayCount(I.S.Deferred) + ")\n";
	S += "  Flame " + std::to_string(I.F.Streak) + " days (best " + std::to_string(I.F.Best) + ", last " + std::to_string(I.F.LastDay) + ", matches " + std::to_string(I.F.Matches) +
		", rest used " + std::to_string(static_cast<int>(I.F.RestUsed)) + (I.F.IceSince >= 0 ? ", ON ICE" : "") + ")\n";
	S += "  Day " + std::to_string(I.S.DayKey) + " (today " + std::to_string(Day) + "): " + std::to_string(I.S.DayEntries) + " paid entries, band " +
		std::string(1, static_cast<char>('A' + std::clamp<int>(I.D.Band, 0, 2))) + ", Dares prog " + std::to_string(I.D.Prog[0]) + "/" + std::to_string(I.D.Prog[1]) + "/" +
		std::to_string(I.D.Prog[2]) + " claimed bits " + std::to_string(static_cast<int>(I.D.Claimed)) + ", week clears " + std::to_string(I.D.WeekClears & 15) + "\n";
	S += "  Wheel: rings lit " + std::to_string(IgniteRing(I.S.DayKey == Day ? I.S.DayEntries : 0)) + ", last spin day " + std::to_string(I.S.WheelDay) + " ring " +
		std::to_string(I.S.WheelRing & IgniteWheelRingMask) + ((I.S.WheelRing & IgniteWheelOwed) != 0 ? " (Sparks owed)" : "") + "\n";
	S += "  Crackers " + std::to_string(I.Boxes.size()) + ":";
	for (const Box& X : I.Boxes)
	{
		S += " " + X.Id + "(" + IgniteCrackerNames[std::clamp<int>(X.Tier, 0, 2)] + ")";
	}
	S += "\n  Wristband month " + std::to_string(I.BandMonth) + " tier " + std::to_string(IgniteTierOf(I.S.BaseMonth)) + " (" + SitePlayCount(I.S.BaseMonth) + " base), claimed " +
		std::to_string(I.BandClaimed) + "; Level " + std::to_string(LevelOf(I.S.BaseLife)) + " (claimed " + std::to_string(I.LevelClaimed) + ")\n";
	S += "  Beef:";
	for (const Beef& Bf : I.Beefs)
	{
		S += " " + Bf.Name + " " + std::to_string(Bf.TheyKo) + "-" + std::to_string(Bf.YouKo);
	}
	S += "\n  Stickers " + std::to_string(I.St.Bits) + ", scalps " + std::to_string(I.St.ScalpLife) + ", guest until " + std::to_string(I.St.GuestUntil) + "\n";
	S += fuse::Describe(I.Fuse);
	return S;
}
} // namespace siteplay_detail

// ------------------------------------------------------------------ the public catalogs and helpers (SitePlay.h)

const std::vector<DareDef>& DareCatalog()
{
	return siteplay_detail::IgniteDares();
}

const std::vector<std::pair<int, double>>& WheelTable(int Ring, int8_t Band)
{
	using namespace siteplay_detail;
	static const std::array<std::array<std::vector<std::pair<int, double>>, 3>, 3> IgniteWheels = [] {
		std::array<std::array<std::vector<std::pair<int, double>>, 3>, 3> W;
		for (int R = 0; R < 3; ++R)
		{
			for (int Bd = 0; Bd < 3; ++Bd)
			{
				W[static_cast<size_t>(R)][static_cast<size_t>(Bd)] = IgniteBuildWheel(R, static_cast<int8_t>(Bd));
			}
		}
		return W;
	}();
	return IgniteWheels[static_cast<size_t>(std::clamp(Ring, 0, 2))][static_cast<size_t>(std::clamp<int>(Band, 0, 2))];
}

const std::vector<PrizeDef>& PrizeCounter()
{
	return siteplay_detail::IgnitePrizes();
}

int64_t LevelNeeds(int Level)
{
	return siteplay_detail::IgniteLevelNeedsOf(Level);
}

int LevelOf(int64_t BaseLife)
{
	int Lv = 1;
	while (Lv < 99 && BaseLife >= LevelNeeds(Lv + 1))
	{
		++Lv;
	}
	return Lv;
}

int IgniteLevel(const Book& B)
{
	return LevelOf(B.Ig.S.BaseLife);
}

bool FlameNudge(const Book& B, const sponsor::Book& Deals, double World)
{
	using namespace siteplay_detail;
	const double OfDay = std::fmod(std::fmod(World, 1440.0) + 1440.0, 1440.0);
	// From 6 PM to the 6 AM close (the play day's evening and night).
	return (OfDay >= 18.0 * 60.0 || OfDay < 6.0 * 60.0) && SitePlayIgniteStarted(B) && B.Ig.F.LastDay != PlayDay(World) && !IgniteOnIce(&Deals);
}

int8_t BandOf(const Book& B, const std::vector<HistoryEntry>& History)
{
	using namespace siteplay_detail;
	for (const Chips C : B.Ig.Fuse.RecentBuyIns)
	{
		if (C > 0)
		{
			return IgniteBandOfRec(B.Ig.Fuse);
		}
	}
	// No paid entry in the book yet: the last 20 Ignite events in History.
	Chips Sum = 0;
	int N = 0;
	for (const HistoryEntry& E : History)
	{
		if (N < 20 && E.BuyInCents > 0 && net::SiteOf(E.EventId) == net::SiteId::Ignite)
		{
			Sum += E.BuyInCents;
			++N;
		}
	}
	const Chips Avg = N > 0 ? Sum / N : 110;
	return static_cast<int8_t>(Avg <= 220 ? 0 : Avg <= 550 ? 1 : 2);
}

double ThemeMult(const std::string& TemplateId, double Start)
{
	using namespace siteplay_detail;
	const int Wd = net::Weekday(PlayDay(Start));
	const bool Fuse = fuse::IsFuse(TemplateId);
	const net::EventTemplate* T = Fuse ? nullptr : IgniteTemplateOf(TemplateId);
	const double OfDay = std::fmod(std::fmod(Start, 1440.0) + 1440.0, 1440.0);
	bool Themed = false;
	switch (Wd)
	{
	case 0: Themed = T && T->Fmt == net::Format::Flip; break;
	case 1: Themed = T && (T->Fmt == net::Format::Bounty || T->Fmt == net::Format::Mystery); break;
	case 2: Themed = Fuse; break;
	case 3: Themed = Fuse || (T && (T->Speed == "Turbo" || T->Speed == "Hyper")); break;
	case 5: Themed = OfDay >= 22.0 * 60.0 || OfDay < 6.0 * 60.0; break;
	case 6: Themed = T && IgniteDailyOf(T->Id) == "sunday-blaze"; break;
	default: break;
	}
	return Themed ? 1.5 : 1.0;
}

bool DareMayCount(int DareId, const std::string& TemplateId, double Start)
{
	using namespace siteplay_detail;
	for (const DareDef& D : IgniteDares())
	{
		if (D.Id != DareId)
		{
			continue;
		}
		const bool Fuse = fuse::IsFuse(TemplateId);
		const net::EventTemplate* T = Fuse ? nullptr : IgniteTemplateOf(TemplateId);
		switch (D.Kind)
		{
		case IgDkFuseEntry:
		case IgDkFuse3:
		case IgDkFuseWin:
		case IgDkFuseWin3x: return Fuse;
		case IgDkFlipEntry: return T && T->Fmt == net::Format::Flip;
		case IgDkEveningEntry: return std::fmod(std::fmod(Start, 1440.0) + 1440.0, 1440.0) >= 18.0 * 60.0;
		case IgDkBountyEntry: return T && (T->Fmt == net::Format::Bounty || T->Fmt == net::Format::Mystery);
		case IgDkGtdEntry: return T && T->GtdCents > 0;
		case IgDkTopHalf50:
		case IgDkFinalTable:
		case IgDkPodium:
		case IgDkSecondBreak: return !Fuse;
		case IgDkTop10_100: return !Fuse && T && T->Field >= 100;
		case IgDkSparkCash: return T && IgniteDailyOf(T->Id) == "spark-jackpot";
		default: return true; // the hand Dares, a cash, a knockout, the early bird: any paid entry
		}
	}
	return false;
}

OpenView CrackerContents(uint64_t WorldSeed, const Box& B)
{
	using namespace siteplay_detail;
	OpenView O;
	O.BoxId = B.Id;
	O.Tier = static_cast<int8_t>(std::clamp<int>(B.Tier, 0, 2));
	const std::string Key = "ignite:crk:" + B.Id;
	const double U = SitePlayDraw(WorldSeed, Key);
	const double UTicket = SitePlayDraw(WorldSeed, Key + ":ticket");
	const double ULook = SitePlayDraw(WorldSeed, Key + ":look");
	const double UWhich = SitePlayDraw(WorldSeed, Key + ":which");
	// The one table (CrackerOdds shows it): the Sparks by U, a ticket under its chance, a look piece under its chance.
	const IgniteCrackerDef& T = IgniteCrackerTable[static_cast<size_t>(O.Tier)];
	double X = U;
	O.Sparks = T.Sparks[T.Rows - 1].first;
	for (size_t K = 0; K < T.Rows; ++K)
	{
		X -= T.Sparks[K].second;
		if (X < 0.0)
		{
			O.Sparks = T.Sparks[K].first;
			break;
		}
	}
	O.Prize = "+" + SitePlayCount(O.Sparks) + " Sparks";
	if (T.TicketP > 0.0 && UTicket < T.TicketP)
	{
		const IgniteTicketDef* Tk = IgniteTicket(T.TicketCode);
		O.TicketId = IgniteTicketId(*Tk);
		O.Prize += T.TicketWords;
	}
	if (T.LookP > 0.0 && (T.LookP >= 1.0 || ULook < T.LookP))
	{
		const size_t Which = std::min<size_t>(5, static_cast<size_t>(UWhich * 6.0));
		O.LookSlot = IgniteCrackerLooks[Which].first;
		O.Look = IgniteCrackerLooks[Which].second;
		O.Prize += ", " + LookOf(O.LookSlot, O.Look).Name;
	}
	return O;
}

CrackerOddsView CrackerOdds(int Tier)
{
	using namespace siteplay_detail;
	CrackerOddsView V;
	V.Tier = static_cast<int8_t>(std::clamp(Tier, 0, 2));
	const IgniteCrackerDef& T = IgniteCrackerTable[static_cast<size_t>(V.Tier)];
	V.Name = IgniteCrackerNames[V.Tier];
	V.From = T.From;
	V.TicketP = T.TicketP;
	V.LookP = T.LookP;
	V.DuplicateSparks = IgniteDuplicateLookSparks;
	for (size_t K = 0; K < T.Rows; ++K)
	{
		CrackerOddsRow R;
		R.Part = 0;
		R.Sparks = T.Sparks[K].first;
		R.Label = SitePlayCount(R.Sparks) + " Sparks";
		R.P = T.Sparks[K].second;
		V.EvSparks += static_cast<double>(R.Sparks) * R.P;
		V.Rows.push_back(R);
	}
	if (T.TicketP > 0.0)
	{
		const IgniteTicketDef* Tk = IgniteTicket(T.TicketCode);
		CrackerOddsRow R;
		R.Part = 1;
		R.TicketId = IgniteTicketId(*Tk);
		R.FaceCents = Tk->Face;
		R.Label = Tk->Label;
		R.P = T.TicketP;
		// A ticket at its Prize Counter price (7,000 Sparks a $1.10): the spec's EV (Neon 820, Gold 4,100).
		V.EvSparks += static_cast<double>(Tk->Face) * 7000.0 / 110.0 * T.TicketP;
		V.Rows.push_back(R);
	}
	if (T.LookP > 0.0)
	{
		for (const std::pair<int, int>& Lk : IgniteCrackerLooks)
		{
			CrackerOddsRow R;
			R.Part = 2;
			R.LookSlot = Lk.first;
			R.Look = Lk.second;
			R.Label = LookOf(Lk.first, Lk.second).Name + " " + IgniteSlotWord[Lk.first];
			R.P = std::min(1.0, T.LookP) / static_cast<double>(std::size(IgniteCrackerLooks));
			V.Rows.push_back(R);
		}
	}
	return V;
}

LookView LookOf(int Slot, int Look)
{
	using namespace siteplay_detail;
	LookView V;
	V.Slot = static_cast<int8_t>(Slot);
	V.Look = static_cast<int8_t>(Look);
	if (Slot < 0 || Slot > 4 || Look < 0 || Look >= IgniteLookCountOf(Slot))
	{
		return V;
	}
	if (Look < IgniteBaseCount[Slot])
	{
		V.Name = IgniteBaseLooks[Slot][Look].Name;
		V.How = IgniteBaseLooks[Slot][Look].How;
		V.Base = static_cast<int8_t>(Look);
		return V;
	}
	const int M = Look - IgniteBaseCount[Slot];
	V.Name = IgniteThemedName(Slot, M);
	V.How = "Wristband T" + std::to_string(IgniteThemedTier[Slot]) + " in " + MonthName("2026-" + std::string(M + 1 < 10 ? "0" : "") + std::to_string(M + 1));
	V.ColorA = IgniteThemes[M].A;
	V.ColorB = IgniteThemes[M].B;
	V.Base = IgniteThemedBase[Slot];
	return V;
}

int LookCount(int Slot)
{
	return siteplay_detail::IgniteLookCountOf(Slot);
}

std::vector<int> Headliners(const world::World* W)
{
	// The world's rule (WorldLeague.h, core-world's; its patch #1): the six Ignite members with the most stream followers,
	// then by their Ignite points before this month (which stay put all month, so the month's six don't shift with a night's
	// results), then this month's, then by id; a fresh world bills on points alone ("Season 1: the first headliners").
	std::vector<int> Out;
	if (!W)
	{
		return Out;
	}
	for (const world::Headliner& H : world::Headliners(*W, W->Clock()))
	{
		Out.push_back(H.Npc);
	}
	return Out;
}
} // namespace play
} // namespace site
} // namespace ss
