#pragma once

#include "ShortStack/Common.h"

#include <array>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace kast
{
struct Member;
struct StreamLog;

/**
 * The creator update (SPEC_CREATORS §2, §8.1, §10.7): Kast's growth model as pure functions over plain numbers, no state.
 * The keyed draws that make the room a function of the stream's inputs and never of the pace (Unit), the Ping and drop-in
 * presence, Heat's closed form and the night's gain, discovery's new terms, the Sweat (prestige, depth, railbirds, the
 * Broadcast), raids and clips, lurkers, engagement on the world clock, per-visit money, the pending payout and the sponsor
 * read rule, the forecast, the directory rank, and kast\tmo's line with the member, log and goodwill tails over a SaveView.
 * Declared by the lead in Phase A-K; every body is core-kastmath's (Game/KastGrowth.cpp), the copy core-kastmath's too
 * (Game/KastCopy.cpp). The Room (Game/Kast.cpp) and the chat games (Game/KastPlay.cpp) call these; nothing here draws from
 * an Rng or reads a clock.
 */
namespace growth
{
// ------------------------------------------------------------------ keyed draws (§2.12)

/** What a keyed draw is for: one stream of independent uniforms per tag (append before Count only). */
enum class Tag : uint32_t
{
	Ping,
	Arrive,
	Slot,
	Late,
	Pull,
	RailCall,
	Guest,
	GuestStay,
	GuestName,
	GuestFollow,
	Lurk,
	Raid,
	MomentRaid,
	Raider,
	Troll,
	Talker,
	Front,
	FirstFollower,
	Tide,
	Give,
	Sub,
	Gift,
	Answer,
	Clip,
	ClipReach,
	Renew,
	ClipFollower,
	Affinity,
	Nod, // a Nodded guest's follow, 0.10 x Sat (§2.9): Unit(Seed, Stream, Nod, NameKey, minute); §2.12's table has no row for it
	Count,
};
/** splitmix64's finaliser over the mixed integers, (z >> 11) x 2^-53: a uniform in [0, 1). StreamNo is Channel::Streams
 *  after Start's increment (0 for the offline draws); A and B a NameKey, a minute, a slot, an index. No strings. */
SHORTSTACKCORE_API double Unit(uint64_t Seed, uint64_t StreamNo, Tag T, uint64_t A, uint64_t B = 0);
/** Fnv1a64 of a name: computed once a member a stream, then the A of that member's draws. */
SHORTSTACKCORE_API uint64_t NameKey(const std::string& Name);
/** The exponential with this mean at quantile U. */
SHORTSTACKCORE_API double ExpAt(double Mean, double U);
/** The Poisson count with this mean at quantile U (the guests arriving in a world minute). */
SHORTSTACKCORE_API int PoissonAt(double Mean, double U);
/** The standard normal at quantile U (the Tide, a clip's reach). */
SHORTSTACKCORE_API double NormalAt(double U);

// ------------------------------------------------------------------ the Ping and drop-in presence (§2.3)

/** 1.0 watched within 7 days, 0.7 within 21, 0.35 within 60, 0.12 older; 1.0 for superfans (the caller passes mods as L 1). */
SHORTSTACKCORE_API double PingW(double DaysSince, double L);
/** PingW + (1 - PingW) x 0.5 h: a hot channel reaches lapsed followers. */
SHORTSTACKCORE_API double PingWHot(double W, double H);
/** A new follower's first week: 0.15 e^(-days since they followed / 7) (not friends). */
SHORTSTACKCORE_API double Honey(double DaysSinceFollow);
/** 1 - 0.5 e^(-days since they last watched / 2): nobody watches every night. */
SHORTSTACKCORE_API double Appetite(double DaysSince);
/** The attention budget: e^(-max(0, Hw - 8) / 10). */
SHORTSTACKCORE_API double Budget(double Hw);
/** Hours watched lately today: Hw e^(-(Today - HwDay) / 7), closed form. */
SHORTSTACKCORE_API double HwToday(double Hw, int HwDay, int Today);
/** A member's terms for the go-live roll (§2.3.1). */
struct PingTerms
{
	double L = 0, Honey = 0, PingWHot = 1, HeatA = 1, Appetite = 1, Budget = 1, WelcomeBack = 1, Sched = 1, ToD = 1, Recent = 1;
};
/** Pa = min(0.95, (0.03 + 0.75 L^1.15 + Honey) x the rest): computed once a member at Start. */
SHORTSTACKCORE_API double Pa(const PingTerms& T);
/** Dee's and Mei's chance on streams 2 and up (both come to stream 1). */
SHORTSTACKCORE_API double FriendPa(int Streams, double Sched, double ToD);
/** First Faces (§2.3.3): a friend's arrival in world minutes on streams 1-5 (Mei after Dee); U the friend's Arrive draw. */
SHORTSTACKCORE_API double FriendArrive(int Streams, bool Second, double DeeAt, double U);
/** Everyone else's arrival delay: Exp(3) on schedule, Exp(6) off. */
SHORTSTACKCORE_API double ArriveDelay(bool OnSchedule, double U);
/** Sm: a member's mean stay a slot process holds (15 + 130 L; friends 95, 400 on streams 1-5; mods x1.5). */
SHORTSTACKCORE_API double StayMean(double L, bool Friend, int Streams, bool Mod);
/** G: a member's mean gap away ((30 + 90 (1 - L)) / (1 + 4 Pst2); friends 40; mods x0.5; x1.5 on the results screen). */
SHORTSTACKCORE_API double GapMean(double L, bool Friend, bool Mod, bool Results, double DeepPull);
/** max(P, 0.5) x depth^2 of the deepest alive run: people come back for the final table. */
SHORTSTACKCORE_API double Pst2(double P, double RunDepth);
/** One 10-world-minute slot of the process: whether a member who is Here (or away) is here in the next slot, from its draw. */
SHORTSTACKCORE_API bool SlotNext(bool Here, double Sm, double G, double U);
/** The expected presence Tau minutes after arrival, closed form (the forecast's s and phi). */
SHORTSTACKCORE_API double Presence(double Tau, double Sm, double G);

// ------------------------------------------------------------------ the deepest run (§2.6), Run Alerts and Rail Call (§2.3.5-6)

/** The stages of a deep run Kast reacts to (100+ entrants). */
enum class RunStage : int
{
	None,
	Bubble,
	Itm,
	FinalTable,
	Top3,
	Win,
};
/** P = clamp(0.35 log2(Entrants / 100) + 0.25 log2(BuyIn / $5.50) + Series, 0.25, 4): the event's prestige. */
SHORTSTACKCORE_API double Prestige(int Entrants, Chips BuyIn, double Series);
/** clamp(ln(Entrants / Remaining) / ln(Entrants / 9), 0, 1.15); 0 when Entrants <= 9. */
SHORTSTACKCORE_API double Depth(int Entrants, int Remaining);
/** The deep-run machinery's field minimum: Entrants >= 100. */
SHORTSTACKCORE_API bool Armed(int Entrants);
/** From the last two tables of an armed run: Remaining <= 2 x TableSize. */
SHORTSTACKCORE_API bool EventMode(int Entrants, int Remaining, int TableSize);
/** 1 / (1 + max(0, Ft7 - 1) / 3): the "Sweating now" shelf has seen this channel at final tables all week. */
SHORTSTACKCORE_API double SweatNov(int Ft7);
/** The stranger term: 1 + 4 P depth^2 Nov while the hero is alive in an armed run. */
SHORTSTACKCORE_API double Sweat(double P, double RunDepth, double Nov);
/** Railbirds an hour: (30 + 0.2 Norm) P depth^3 Nov while depth > 0.3. */
SHORTSTACKCORE_API double Railbirds(double Norm, double P, double RunDepth, double Nov);
/** Lurkers back at the screen while deep: 1 + 2 depth^2 min(P, 2). */
SHORTSTACKCORE_API double LurkDeep(double P, double RunDepth);
/** Deep follows: 1 + 0.3 min(1, depth) min(P, 2). */
SHORTSTACKCORE_API double DeepFollow(double P, double RunDepth);
/** The site's Broadcast at a final table with P >= 1: 20 min(P, 2.5)^1.5 viewers. */
SHORTSTACKCORE_API int BroadcastSize(double P);
/** Heat's RunValue: Bubble 0.15, ITM 0.3, final table 1.0, top 3 1.3, win 2.0. */
SHORTSTACKCORE_API double RunValue(RunStage S);
/** A Run Alert's pull: Bubble 0.25, ITM 0.20, final table 0.50, top 3 0.40 (0 for the rest). */
SHORTSTACKCORE_API double Pull(RunStage S);
/** An away attendee comes back for a Run Alert: pull (0.3 + 0.7 L). */
SHORTSTACKCORE_API double AlertBack(double Share, double L);
/** A follower not here tonight comes at a final table or top 3: pull max(PingW, 0.35) (0.05 + 0.6 L) min(2, P) / 2. */
SHORTSTACKCORE_API double AlertFollower(double Share, double W, double L, double P);
/** RAIL CALL opens at the first attended minute an armed run is at depth >= 0.4. */
SHORTSTACKCORE_API bool RailCallOpen(int Entrants, double RunDepth);
/** A pressed Rail Call's second roll: 0.5 max(PingW, 0.35) (0.03 + 0.75 L^1.15) Sched ToD Appetite Budget. */
SHORTSTACKCORE_API double RailCallChance(double W, double L, double Sched, double ToD, double AppetiteNow, double BudgetNow);

// ------------------------------------------------------------------ Heat (§2.4)

/** Heat at Now: 8 + (Heat - 8) 0.5^((Now - At) / 5 world days); closed form, so a skip of N days is N one-day steps. */
SHORTSTACKCORE_API double HeatAt(double Heat, double At, double Now);
/** max(20, HeatAt) while the channel streamed in the last 7 world days, else HeatAt ("Cold means dark"). */
SHORTSTACKCORE_API double HeatShown(double Heat, double At, double Now, int LastStreamDay);
/** h = (HeatShown - 8) / 92. */
SHORTSTACKCORE_API double HeatH(double Shown);
/** Strangers: max(0.8, 1 + 1.5 (h - 0.2)). */
SHORTSTACKCORE_API double HeatS(double H);
/** Attendance: max(0.8, 1 + 1.4 (h - 0.2)). */
SHORTSTACKCORE_API double HeatA(double H);
/** The night at Stop (streams of 60+ world minutes): perf, news, the raw gain and the gain after the week's cap and the
 *  soft ceiling. */
struct NightGain
{
	double Perf = 0, News = 0, Raw = 0, Gain = 0;
};
/** Lr = ln((Avg + 1) / (Fc + 1)); Run the night's best RunValue x P; WeekUsed the positive gains of this world week. */
SHORTSTACKCORE_API NightGain Gain(double Lr, double Bias, double Run, double RunNorm, double WeekUsed, double HeatShownNow);
/** The show-up term: +3 the first time a world day's stream reaches 60 world minutes, at most 4 days a world week. */
SHORTSTACKCORE_API double ShowUp(int ShowWeek, int ShowDays, int Today);
/** The meter's tier: 0 Cold (< 20), 1 Warming up, 2 Running hot (35), 3 On a heater (55), 4 Sun run (75). */
SHORTSTACKCORE_API int Tier(double Shown);
/** Front Row at On a heater: 30% of streams, from the Front draw. */
SHORTSTACKCORE_API bool FrontRow(double Shown, double U);

// ------------------------------------------------------------------ discovery, stay and follow (§2.5)

/** (1 + 0.03 (Level - 1)) (1 + 0.04 min(Streak, 8)). */
SHORTSTACKCORE_API double Disc(int Level, int Streak);
/** Fresh on Kast: 1 + 0.8 max(0, 1 - (Streams - 1) / 12). */
SHORTSTACKCORE_API double Lane(int Streams);
/** Rising in Poker, after stream 3: 1 + 1.0 max(0, 1 - Norm / 15). */
SHORTSTACKCORE_API double Rising(int Streams, double Norm);
/** The poker category's browsers are finite: (1 + V / 600)^-0.5. */
SHORTSTACKCORE_API double Pond(double V);
/** (1 + max(0, H7 - 20) / 16)^-0.5, H7 the world hours live in the last 7 days. */
SHORTSTACKCORE_API double Novelty(double Hours7);
/** max(0.25, e^(-(Uptime - 240) / 300)) after 4 world hours, only while depth <= 0.3. */
SHORTSTACKCORE_API double Taper(double Uptime, double RunDepth);
/** exp(0.12 N(0, 1)) from the day's Tide draw, x1.3 inside a series window. */
SHORTSTACKCORE_API double Tide(double U, bool SeriesWindow);
/** Rerun's buzz: 1 + 0.35 min(1, Views48 / (2000 + 40 Norm)). */
SHORTSTACKCORE_API double Buzz(double Views48, double Norm);
/** Heads-down: min(1, 0.45 + 1.83 E). */
SHORTSTACKCORE_API double HD(double E);
/** Follow saturation: (1 + Followers / 3,000)^-0.65. */
SHORTSTACKCORE_API double Sat(int Followers);
/** clamp(1 - 0.03 max(0, H7 - 25), 0.6, 1): stays, follows and Engage. */
SHORTSTACKCORE_API double Stamina(double Hours7);

// ------------------------------------------------------------------ raids, clips, the community (§2.7, §2.8)

/** The network's raids in, an hour: (0.02 + 0.03 min(10, 0.35 Tokens)) min(2, Disc) (1 + h). */
SHORTSTACKCORE_API double RaidRate(int Tokens, double DiscNow, double H);
/** A raid's size: U(0.5, 0.9) x (2 + U2^2.2 x 55). */
SHORTSTACKCORE_API double RaidSize(double U1, double U2);
/** A raid out earns its channel a token: not raided in the last 7 world days and a stream of 60+ world minutes. */
SHORTSTACKCORE_API bool EarnsToken(int Today, int LastRaidDay, double StreamMinutes);
/** A moment raid on an armed run: ITM 5%, final table 15%, win 30%. */
SHORTSTACKCORE_API double MomentRaidChance(RunStage S);
/** A big name's raid at a final table or a win, once a stream: 0.10 + 0.30 h + 0.15 [P >= 2]. */
SHORTSTACKCORE_API double BigNameChance(double H, double P);
/** A named raider's stay in world minutes: 40% Exp(20), 60% Exp(3); x1.3 welcomed. */
SHORTSTACKCORE_API double RaiderStay(double U1, double U2, bool Welcomed);
/** A clip's reach: (30 + 4 V) e^N (1 + h) (1 + HypeHit / 50). */
SHORTSTACKCORE_API double ClipReach(double V, double N, double H, double HypeHit);
/** Affinity's one-time bumps (Bonded 5%, Noticed 3% of the gap). */
SHORTSTACKCORE_API double AffinityBump(double Affinity, double ShareOfGap);
/** The lurker pool's expected presence: 0.6% Untracked Sched (0.4 + 0.6 ToD / 1.25) + 0.3% SumPingWAway, x Deep. */
SHORTSTACKCORE_API double LurkerMean(int Untracked, double Sched, double ToD, double SumPingWAway, double Deep);
/** floor(Mean + U): an honest integer. */
SHORTSTACKCORE_API int Lurkers(double Mean, double U);

// ------------------------------------------------------------------ engagement (§2.9)

/** Engage fades on the world clock: E e^(-DWorld / 6.9). */
SHORTSTACKCORE_API double EngageFade(double E, double DWorld);
/** Thank chat: 0.25 x 0.6^K, K the thanks in the last 10 world minutes. */
SHORTSTACKCORE_API double ThankGain(int K);
/** Questions a world minute: min(0.39, 0.0197 (strangers + 0.4 present + 0.5 raiders)), x1.6 on a teaching title. */
SHORTSTACKCORE_API double QuestionRate(double Strangers, double Present, double Raiders, bool Teaching);

// ------------------------------------------------------------------ money (§2.10)

/** 0.3 + 2.33 min(E, 0.30): tips and cheers. */
SHORTSTACKCORE_API double EngT(double E);
/** 0.6 + 1.33 min(E, 0.30): new subs. */
SHORTSTACKCORE_API double EngS(double E);
/** (1 + AvgPresent / 90)^-0.5 (1 - e^(-days since this member last gave / 21)). */
SHORTSTACKCORE_API double Dil(double AvgPresent, double DaysSinceGave);
/** A visit's chances (>= 20 world minutes watched; subs >= 30 and not a sub). */
SHORTSTACKCORE_API double TipChance(double L, double Hype, double E, double Dilution);
SHORTSTACKCORE_API double CheerChance(double L, double Hype, double E, double Dilution);
SHORTSTACKCORE_API double SubChance(double L, double Hype, double E);
/** A renewal: 0.40 + 0.45 L (a gift 0.2 + 0.5 L). */
SHORTSTACKCORE_API double RenewChance(double L, bool Gift);
/** Per-visit giving as a hazard a present minute: the visit's chance spread over its expected minutes, bunched in the 6
 *  world minutes after a big moment the member is present for. */
SHORTSTACKCORE_API double GiveHazard(double L, double Hype, double E, double Dilution, double Minutes, bool NearBigMoment);
/** A payout (C14): what lands now and what waits. Affiliate revenue (subs, cheers, ads, renewals, gifts) earned while
 *  RentsPaid < 2 never reaches UnpaidCents: the Room books it as Channel::PendingCents as it's earned (Inputs::RentsPaid),
 *  and Offline does the same for what comes in offline (its RentsPaid). So UnpaidCents (tips, and everything once the second rent
 *  is paid) always pays, and PendingCents joins it once RentsPaid >= 2 ("Your first Kast payout"). Session::PayOut's rule
 *  once life::U3Kast is set; sponsor pay is never in either (C13: LedgerSponsors lines, KastAfterStop). */
struct Payout
{
	Chips Pay = 0, Pending = 0;
};
SHORTSTACKCORE_API Payout PayoutOf(Chips Unpaid, Chips Pending, int RentsPaid);
/** C13: a read pays once a deal a world day, after 30 world minutes live with someone watching, within 1.5x the deal's reads
 *  ask a contract period; a deal that asks for no reads (ReadsAsk 0, Kast's old terms) keeps one a world day, uncapped (M1). */
SHORTSTACKCORE_API bool SponsorReadPays(int TodayDay, double ReadAt, double LiveMinutes, int Count, int ReadsPaid, int ReadsAsk);

// ------------------------------------------------------------------ the week's rings (§2.2)

/** H7's ring: world minutes live on each of the last 7 world days (index Day % 7), aged by its newest day. */
SHORTSTACKCORE_API void LiveAdd(std::array<int, 7>& Ring, int& RingDay, int Today, int Minutes);
/** World hours live in the 7 world days to Today. */
SHORTSTACKCORE_API double H7(const std::array<int, 7>& Ring, int RingDay, int Today);
/** The once-a-world-day grants (Channel::GrantMask bits): a restart the same world day can't re-arm them. */
enum Grant : uint32_t
{
	GrantRailCall = 1,
	GrantBroadcast = 2,
	GrantAlertBubble = 4,
	GrantAlertItm = 8,
	GrantAlertFinal = 16,
	GrantAlertTop3 = 32,
	GrantMomentRaid = 64,
	GrantClip1 = 128,
	GrantClip2 = 256,
};
/** Whether Bit is used today (a mask from another day reads empty). */
SHORTSTACKCORE_API bool Granted(int GrantDay, uint32_t GrantMask, int Today, uint32_t Bit);
/** Uses Bit today (a mask from another day starts over). */
SHORTSTACKCORE_API void UseGrant(int& GrantDay, uint32_t& GrantMask, int Today, uint32_t Bit);

// ------------------------------------------------------------------ the forecast (§2.11) and the directory rank (§2.5.4)

struct Band
{
	double Lo = 0, Centre = 0, Hi = 0;
};
/** Fc = sum Pa_i min(1, 0.25 + 0.75 s_i) + (A0 / 60)(4 + 6Q), shown x e^Bias, the band +-0.85 sd widened by e^0.10. Attend
 *  and S run over the members (their Pa, and the slot process's s = Sm / (Sm + G)); A0 the directory rate at the start. */
SHORTSTACKCORE_API Band Forecast(const std::vector<double>& Attend, const std::vector<double>& S, double A0, double Q, double Bias);
/** "#23 of 49 in Poker": 1 + the live channels with more viewers now (the directory's named streamers and the network). */
struct Standing
{
	int Rank = 0, Of = 0;
};
SHORTSTACKCORE_API Standing RankAt(double World, int Viewers);

// ------------------------------------------------------------------ saves (§8.1)

/** What kast\tmo holds, in its field order: the Channel's appended state (§2.2), filled by core-kastroom. */
struct SaveView
{
	int Version = 1;
	double Heat = 26, HeatAt = 0, Norm = 0, Bias = 0, RunNorm = 0;
	int BestAvg = 0;
	double PingAt = -1.0e9;
	int LastStreamDay = -1, ShowWeek = -1, ShowDays = 0, GainWeek = -1;
	double GainUsed = 0;
	int FtBase = -1, FtDays = 0, LiveDay = -1;
	std::array<int, 7> LiveMin{};
	int GrantDay = -1;
	uint32_t GrantMask = 0;
	bool WelcomeRaidDone = false;
	int CotnDay = -1;
	Chips PendingCents = 0;
	double LiveSince = -1, GainSoFar = 0;
	double CarryFollow = 0, CarryWord = 0, CarrySub = 0, CarryTip = 0, CarryCheer = 0, CarryRaid = 0, CarryGift = 0;
	std::vector<std::pair<double, double>> RerunHeat; // (world minute, amount), at most 4
	uint32_t Firsts = 0; // Channel::Firsts (FirstBit bits): the line's last field, after RerunHeat (Phase A-K's addition to §8.1)
};
/** Channel::Firsts: the one-time milestone stings (§5.3 K17) no other saved field remembers. The follower steps ride
 *  Channel::Milestone, the first raid in RaidsIn, Affiliate and Partner their flags. */
enum FirstBit : uint32_t
{
	FirstStrangerTip = 1,
};
// The Session's save block (Session.cpp, the lead's) calls these only once life::U3Kast is set, so an untouched save writes back
// byte for byte. Each tail is its fields, every one with the tab before it ("\t312\t41\t309"), which the Session appends to
// the line as it stands before its "\n"; the readers take the whole split line (P[0] "kast") and read past its old fields.
// Today is the world day (net::DayOf of the world minute).

/** kast\tmo, after kast\tgrowth: one line ending in "\n", only when it holds something (the Session calls it once life::U3Kast
 *  is set). */
SHORTSTACKCORE_API void WriteLines(std::ostream& Out, const SaveView& V);
/** A split kast\tmo line ("kast", "mo", ...), read as "at least N fields"; false when it isn't one. */
SHORTSTACKCORE_API bool ReadLine(const std::vector<std::string>& P, SaveView& V);
/** kast\tmem's tail after P[11]: GaveDay (within 90 days), Hw in tenths (while above 0.05), HwDay; empty when nothing is set.
 *  (Bonded and Noticed ride the flags field, P[10], as bits 4 and 8: the Session writes and reads them.) */
SHORTSTACKCORE_API std::string MemberTail(const Member& M, int Today);
/** Reads the tail back from a split kast\tmem line; false when there's none. */
SHORTSTACKCORE_API bool ReadMemberTail(const std::vector<std::string>& P, Member& M);
/** kast\tlog's tail: AvgOwn, PeakAt, PeakMoment, HeatAfter, written only under the new model (empty on an old row). */
SHORTSTACKCORE_API std::string LogTail(const StreamLog& L);
SHORTSTACKCORE_API bool ReadLogTail(const std::vector<std::string>& P, StreamLog& L);
/** kast\tgoodwill's tail: the world day of the last raid out (the 7-day rule), empty while unset (-1). */
SHORTSTACKCORE_API std::string GoodwillTail(int LastRaidDay);
SHORTSTACKCORE_API int ReadGoodwillTail(const std::vector<std::string>& P);
} // namespace growth

/** Every Kast string the creator update adds (Game/KastCopy.cpp, core-kastmath's), for the copy lint and kCopyScan. */
SHORTSTACKCORE_API std::vector<std::string> Inventory();
} // namespace kast
} // namespace ss
