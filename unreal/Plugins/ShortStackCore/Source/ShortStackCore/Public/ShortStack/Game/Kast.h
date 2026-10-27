#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/Gear.h"
#include "ShortStack/Game/KastGrowth.h"
#include "ShortStack/Rng.h"

#include <array>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
/**
 * Kast: the streaming site on the laptop. The player goes live with whatever the desk has (GearDrop sells the rest)
 * and plays RiverLine on stream. Growing a channel is a grind, the way it is for real: the first streams are Dee, a
 * friend and a lurker or two.
 *
 * The audience is built one person at a time. Strangers find the stream in the Poker directory (more of them as the
 * channel levels up, keeps a schedule and holds viewers); the ones who stay long enough, in a chat that feels alive,
 * follow. Every follower is a member of the community with a loyalty that grows only by watching: each stream they
 * catch, each question answered, each night they're greeted. Loyal members come back (much more often when the
 * stream is on its posted schedule), chat, bring friends, subscribe and tip; members who are left waiting drift away.
 * Regulars and superfans are the channel's real size, and the more streams, the more of them there are.
 *
 * The structure: a channel level (hours live and the community raise how often Kast shows the stream to strangers),
 * weekly goals and a streak for consistency, eight community stages from the first stream to a poker personality,
 * Kast's 30-day rules for Affiliate (subs, cheers, ads) and Partner (verified, 70%), sponsors who want real average
 * viewers, and a network of small channels that raid the ones who raid them.
 *
 * Money and growth run on world minutes (a sprinted tournament streams for as long as it lasts on the clock); chat and
 * alerts run on real seconds, so the screen stays readable. Everything draws from the stream's own generator: the
 * poker never sees it.
 */
namespace kast
{
/** Brand colors (the app, the overlay, the alerts). */
constexpr uint32_t Violet = 0x9b5cff;
constexpr uint32_t Lime = 0xc6f432;

// ------------------------------------------------------------------ the directory

/** Another channel in the Poker category (all fictional). */
struct Streamer
{
	std::string Name;
	std::string Title;
	std::string Tag; // a line about them for the directory
	std::string Country;
	int Followers = 0;
	int Viewers = 0;  // a normal night at their peak
	int Opens = 0;    // minute of the day they go live
	int Hours = 0;    // how long they stay on
	int Days = 0x7f;  // weekdays live (bit 0 Monday)
	uint32_t Color = 0x9b5cff;
	bool Rival = false; // gh0stfold: never logs off
};
SHORTSTACKCORE_API const std::vector<Streamer>& Directory();
/** Their viewers now (0 offline). */
SHORTSTACKCORE_API int ViewersNow(const Streamer& S, double World);

// ------------------------------------------------------------------ sponsors and titles

struct Sponsor
{
	std::string Id;
	std::string Brand;
	std::string Product;
	std::string Pitch;   // the offer text
	std::string ReadLine; // what the streamer says on a read
	uint32_t Color = 0xffffff;
	int Followers = 0;       // the channel size that gets their attention
	int AvgViewers = 0;      // and the average viewers (last 30 days) they want to see
	bool NeedsPartner = false;
	Chips PerHourCents = 0;  // live hours while the deal runs
	Chips ReadCents = 0;     // a sponsor read (once a stream)
	int Days = 30;
};
/**
 * The five brands Kast has carried from the start (sponsor::Brands()' legacy ids, in their order: Overclock, TunnelRat,
 * Stacked, RiverLine, Sitwell), as the stream sees them. The world's streamers draw their sponsors from these.
 */
SHORTSTACKCORE_API const std::vector<Sponsor>& Sponsors();
/** Any brand that pays on stream (the sponsor catalog's: Halo too), by id; nullptr for a brand that never reads. */
SHORTSTACKCORE_API const Sponsor* FindSponsor(const std::string& Id);

/** Stream titles: each one shapes the show a little. */
struct TitleSpec
{
	std::string Text;
	std::string Tag;
	double Discover = 1.0; // strangers clicking in
	double Follow = 1.0;   // of those, following
	double Hype = 1.0;     // how hard the big hands hit
	double Calm = 1.0;     // trolls (lower is calmer)
};
SHORTSTACKCORE_API const std::vector<TitleSpec>& Titles();

// ------------------------------------------------------------------ the channel (persists)

struct Subscriber
{
	std::string Name;
	double Since = 0.0;  // world minutes
	double Renews = 0.0; // the next renewal (thirty days on)
	int Months = 1;
	bool Gift = false;
};

/** Someone who has followed or watched: the community is built one of these at a time. */
struct Member
{
	std::string Name;
	double Loyalty = 0.0;  // 0..1: how much this channel is part of their week
	double Affinity = 0.3; // how much they could ever care (most followers are casual; a few become superfans)
	double FirstSeen = 0.0;
	double LastSeen = 0.0;
	int Streams = 0; // streams they've watched
	double WatchMinutes = 0.0;
	int Messages = 0;
	bool Follower = false;
	bool Friend = false; // Dee and Mei: there from the first night
	Chips Given = 0;     // tips, cheers and subs
	// The creator update (SPEC_CREATORS §2.2, §2.8.2, §8.1), appended: kast\tmem's flags gain bit 4 Bonded and bit 8 Noticed,
	// and the rest rides its tail (growth::MemberTail), written only when set. Read nowhere until core-kastroom's Room.
	bool Bonded = false;  // present at a final table or a win on stream (100+ entrants): Affinity +5% of its gap, once
	bool Noticed = false; // answered or Nodded: Affinity +3% of its gap, once
	int GaveDay = -1;     // the world day of the last tip or cheer (per-visit giving's 21-day refractory)
	double Hw = 0.0;      // hours watched lately as of HwDay (the attention budget; decays e^(-days / 7))
	int HwDay = -1;
};
constexpr double RegularLoyalty = 0.35;
constexpr double SuperfanLoyalty = 0.7;
/** Members tracked one by one; beyond that, followers are counted (and lurk). */
constexpr int MaxMembers = 1500;

struct Moderator
{
	std::string Name;
	double Since = 0.0;
	int Actions = 0;
	double Online = 0.6; // chance they're watching any given stream
};

struct Clip
{
	std::string Title;
	std::string By;
	double At = 0.0;
	double Views = 0.0;
	double Reach = 0.0; // the views it will settle at
	int Kind = 0;       // the Moment it caught
	/** Update 2: the Rerun candidate cut from this clip's moment (kast\tclip's 9th field, written only when set). */
	std::string Candidate;
	/** The creator update (C-K's L11, appended): the clip went into the Clip Lane, Kast's feed, when it was made (MakeClip,
	 *  ClipIt): the Channel page's Top clips flag. Saved as kast\tclip's 10th field, "1", only when set (an old line reads false). */
	bool Lane = false;
};

struct StreamLog
{
	double Start = 0.0;
	double Minutes = 0.0;
	int Avg = 0;
	int Peak = 0;
	int Follows = 0;
	int Subs = 0;
	Chips Cents = 0;
	std::string Title;
	int Returning = 0; // members who came back
	bool OnSchedule = false;
	// The creator update (SPEC_CREATORS §2.10.3, §8.1), appended: kast\tlog's tail (growth::LogTail), written only under the new
	// model; an old row reads -1 everywhere.
	int AvgOwn = -1;         // the average without raiders and the Broadcast (Partner's rule); -1: read Avg
	double PeakAt = -1.0;    // the stream minute of the peak
	int PeakMoment = -1;     // the Moment at the peak, or -1
	double HeatAfter = -1.0; // Heat after the night's commit
};

struct Deal
{
	std::string Id; // the brand (sponsor::Brand::Id)
	double Since = 0.0;
	double Until = 0.0;
	double ReadAt = -1e9; // last read (world minutes)
	Chips EarnedCents = 0;
	// The view's terms (sponsor::StreamView, next_segment_spec §3.8): what Kast pays for this deal. A deal is the sponsor
	// book's contract seen from the stream; nothing here is saved (Channel::Deals is read only to migrate an old save).
	Chips PerHourCents = 0;
	Chips ReadCents = 0;
	bool Overlay = true; // holds one of the overlay's two slots (its mark in the corner)
	// The creator update (SPEC_CREATORS C13, §8.1's ReadsPaid): this contract period's paid reads and the deal's reads ask, for
	// the 1.5x cap (growth::SponsorReadPays). The book keeps both (sponsor::Contract's Now.Reads and Terms::Asks[Ask::Reads])
	// and Sponsor.cpp is frozen, so Session::KastInputsExtra (core-kastglue) fills them on Inputs::Deals; read nowhere until the
	// Room. ReadsAsk 0: the deal asks for no reads.
	int ReadsPaid = 0;
	int ReadsAsk = 0;
};

/** Kast's rules, over the last 30 days (Affiliate also needs 50 followers). */
constexpr int AffiliateFollowers = 50;
constexpr double AffiliateMinutes = 500.0;
constexpr int AffiliateDays = 7;
constexpr double AffiliateAvg = 3.0;
/** The creator update's Affiliate bar (SPEC_CREATORS C8, the real platform's current one; the average stays AffiliateAvg):
 *  read nowhere until core-kastroom's Room and ui-kast's checklist take it in place of the three above, which are today's. */
constexpr int AffiliateFollowersC8 = 25;
constexpr double AffiliateMinutesC8 = 240.0;
constexpr int AffiliateDaysC8 = 4;
constexpr double PartnerMinutes = 25.0 * 60.0;
constexpr int PartnerDays = 12;
constexpr double PartnerAvg = 75.0;
constexpr Chips SubPriceCents = 499;
/** Channel levels. */
constexpr int MaxLevel = 50;
/** Sponsor slots on the overlay. */
constexpr int MaxDeals = 2;

struct Channel
{
	int Title = 0;
	int Followers = 0;
	double FollowFrac = 0.0;
	std::vector<Subscriber> Subs;
	std::vector<Moderator> Mods;
	std::vector<Clip> Clips;      // newest first, at most 16
	std::vector<StreamLog> Log;   // newest first, at most 60
	std::vector<Member> Members;  // the community
	// Kast's own deals from before the sponsor book (next_segment_spec §3.8): read from an old save, moved into the book
	// by sponsor::AfterParse, and never written again. The stream works on Inputs::Deals.
	std::vector<Deal> Deals;
	std::set<std::string> Offers;   // sponsor ids waiting for an answer
	std::set<std::string> Declined;
	double MinutesLive = 0.0;
	double ViewerMinutes = 0.0;
	int Peak = 0;
	int Streams = 0;
	bool Affiliate = false;
	bool Partner = false;
	Chips EarnedSubs = 0;
	Chips EarnedBits = 0;
	Chips EarnedTips = 0;
	Chips EarnedAds = 0;
	Chips EarnedSponsors = 0;
	Chips UnpaidCents = 0; // the channel balance (paid out at the end of each stream, or on demand)
	Chips PaidCents = 0;
	int GiftedSubs = 0;
	int RaidsIn = 0;
	int Milestone = 0; // the last follower milestone celebrated
	double LastOffline = 0.0; // Offline() has run up to here
	// Consistency.
	int ScheduleDays = 0;          // posted schedule: weekdays (bit 0 Monday), 0 for none
	int ScheduleStart = 21 * 60;   // minute of the day
	std::vector<int> DaysLive;     // days with a stream (the last 60)
	int ProcessedDay = -1;         // Offline() has handled the days before this one
	// Growth.
	double Xp = 0.0;
	int Streak = 0; // weeks in a row with three streams or more
	int Stage = 0;  // the community stage reached (Stages())
	int Week = -1;  // this week's goals
	int WeekStreams = 0;
	double WeekMinutes = 0.0;
	int WeekAnswers = 0;
	int WeekRegulars = 0;
	int WeekRewarded = 0; // goals already paid out this week (bits)
	std::map<std::string, int> Goodwill; // small channels raided -> times (they raid back)
	int RaidsOut = 0;
	// ---- The creator update (SPEC_CREATORS §2.2, §8.1), appended. Saved as kast\tmo (growth::WriteLines over a SaveView)
	// and kast\tgoodwill's tail once life::U3Kast is set; read nowhere until core-kastroom's Room lands. The defaults are a
	// new channel's (an old one is seeded at its first stream under the new model, §8.3).
	double Heat = 26.0;     // the Heat meter, raw (8..100), as of HeatAt: growth::HeatAt decays it in closed form
	double HeatAt = 0.0;    // world minutes
	double Norm = 0.0;      // the channel's usual average (0.8 Norm + 0.2 Avg a stream of 60+ world minutes)
	double Bias = 0.0;      // how the forecast usually lands here (log)
	double RunNorm = 0.0;   // the usual night at the tables (Heat's news reads against it)
	int BestAvg = 0;        // the record average
	double PingAt = -1.0e9; // the last go-live ping (one per 4 world hours)
	int LastStreamDay = -1; // the world day of the last stream (Welcome back, Heat's floor, Rerun's Heat rule)
	int ShowWeek = -1;      // the show-up term's world week...
	int ShowDays = 0;       // ...and the days it counted (bits, at most 4)
	int GainWeek = -1;      // the world week of the positive night gains...
	double GainUsed = 0.0;  // ...and their sum (at most 35)
	int FtDays = 0;         // the world days with a final table (100+ entrants) on stream in the last 7: bits from FtBase
	int FtBase = -1;
	std::array<int, 7> LiveMin{}; // world minutes live on each of the last 7 world days, short streams included (H7's ring)
	int LiveDay = -1;             // the ring's newest world day
	int GrantDay = -1;            // the once-a-world-day grants used on GrantDay (growth::Grant bits)
	uint32_t GrantMask = 0;
	std::vector<std::pair<double, double>> RerunHeat; // Rerun's Heat bumps not yet applied: (world minute, amount), at most 4
	bool WelcomeRaidDone = false; // the welcome raid came (or the channel was past stream 3 when the new model reached it)
	int CotnDay = -1;             // Clip of the Night's world day
	Chips PendingCents = 0;       // Affiliate revenue held while RentsPaid < 2 (C14)
	double LiveSince = -1.0;      // an unfinished stream's start (-1: none)...
	double GainSoFar = 0.0;       // ...and its ghost gain at the last save (a stream lost to a reload closes at min(this, 0))
	// The per-stream accumulators (Stream's debts today), carried from stream to stream instead of reset at Start.
	double CarryFollow = 0.0;
	double CarryWord = 0.0;
	double CarrySub = 0.0;
	double CarryTip = 0.0;
	double CarryCheer = 0.0;
	double CarryRaid = 0.0;
	double CarryGift = 0.0;
	uint32_t Firsts = 0; // the one-time stings no other field remembers (growth::FirstBit: the first stranger's tip)
	std::map<std::string, int> RaidedDay; // small channels raided -> the world day of the last raid out (the 7-day rule)

	SHORTSTACKCORE_API int ActiveSubs(double World) const;
	SHORTSTACKCORE_API double AvgViewers() const; // over the last ten streams
	/** The last 30 days: minutes live, days streamed, average viewers. */
	SHORTSTACKCORE_API void Window30(double World, double& Minutes, int& LiveDays, double& Avg) const;
	SHORTSTACKCORE_API int Regulars() const;
	SHORTSTACKCORE_API int Superfans() const;
	SHORTSTACKCORE_API int Level() const;
	/** XP for a level (the total). */
	SHORTSTACKCORE_API static double LevelXp(int Level);
	/** How often Kast shows the stream to strangers (1 for a new channel). */
	SHORTSTACKCORE_API double Discoverability() const;
	/** Whether a stream starting at World is on the posted schedule. */
	SHORTSTACKCORE_API bool OnSchedule(double World) const;
	SHORTSTACKCORE_API const Member* FindMember(const std::string& Name) const;
	SHORTSTACKCORE_API Member* FindMember(const std::string& Name);
	Chips Earned() const { return EarnedSubs + EarnedBits + EarnedTips + EarnedAds + EarnedSponsors; }
	SHORTSTACKCORE_API bool IsMod(const std::string& Name) const;
	SHORTSTACKCORE_API bool IsSub(const std::string& Name, double World) const;
	SHORTSTACKCORE_API const Deal* ActiveDeal(const std::string& Id, double World) const;
};

/**
 * What the channel earns and loses while the player isn't live (sub renewals, clip views, deals running out). The creator
 * update (SPEC_CREATORS §8.4, C14): Seed (Session::WorldSeed) keys the renewals and the clip followers, so a skip of N days
 * is N one-day steps and R is never drawn; RentsPaid (life::State) holds the Affiliate money earned offline in PendingCents
 * before the second rent. The session's offline branch passes both, Start passes its Inputs'; Phase A-K: neither is read.
 */
SHORTSTACKCORE_API void Offline(Channel& Ch, double World, Rng& R, uint64_t Seed = 0, int RentsPaid = 2);
/**
 * The creator update (SPEC_CREATORS §8.1): the Channel's appended state as kast\tmo's SaveView, field for field, and back.
 * The Session's save block hands them to growth::WriteLines and growth::ReadLine, and only once life::U3Kast is set, so an
 * untouched save never reaches them. core-kastroom's (Kast.cpp); Phase A-K's bodies copy every field.
 */
SHORTSTACKCORE_API growth::SaveView SaveViewOf(const Channel& Ch);
SHORTSTACKCORE_API void ApplySaveView(Channel& Ch, const growth::SaveView& V);
/** The level the channel is at, for the checklist: 0 none, 1 affiliate, 2 partner. */
SHORTSTACKCORE_API int Tier(const Channel& Ch);

/** The community's stages, in order: what each takes and what it opens up. */
struct StageSpec
{
	std::string Name;
	std::string Blurb;
	std::string Unlock;
	int Streams = 0;
	int Regulars = 0;
	int AvgViewers = 0; // last 30 days
	int Followers = 0;
	int Tier = 0; // Kast tier needed (1 affiliate, 2 partner)
};
SHORTSTACKCORE_API const std::vector<StageSpec>& Stages();
/** One requirement and how far along it is. */
struct Goal
{
	std::string Label;
	double Have = 0.0;
	double Need = 1.0;
	int Xp = 0; // weekly goals: the reward
};
/** What the next stage still needs. */
SHORTSTACKCORE_API std::vector<Goal> StageGoals(const Channel& Ch, double World, int Stage);
/** This week's goals (stream three times, six hours, answer five questions, two new regulars). */
SHORTSTACKCORE_API std::vector<Goal> WeekGoals(const Channel& Ch, double World);

/** Small channels in the Poker category (procedural): the network that raids, and gets raided. */
struct SmallChannel
{
	std::string Name;
	std::string Title;
	int Viewers = 0;
	uint32_t Color = 0;
};
/** The small channels live right now, biggest first. */
SHORTSTACKCORE_API std::vector<SmallChannel> Network(double World);

// ------------------------------------------------------------------ live

enum class Moment : int
{
	Register,  // a new tournament on stream
	AllIn,     // the player's chips go in
	WonAllIn,
	LostAllIn, // and survived (or busted: Bust follows)
	BadBeat,
	BigPot,
	Knockout,
	Bubble,
	InTheMoney,
	FinalTable,
	Win,
	Bust,
	Cashed, // finished in the money
	BestPlay,
	Blunder,
	Rival,
	LevelUp,
	// The creator update (SPEC_CREATORS §2.2, §2.3.5): the hero's table reaches three left (the Session fires it where it fires
	// FinalTable). Inert until core-kastroom's Room: OnMoment ignores it.
	Top3,
	Count,
};
SHORTSTACKCORE_API const char* MomentName(Moment M);

/** Emotes: Kast's own ("kastPog") and, once the channel is affiliate, the channel's ("grindShip"). */
enum class Emote : int
{
	Pog,
	Lul,
	Gg,
	Rip,
	Hype,
	Love,
	Fish,
	Salt,
	Clap,
	Chip,
	Ship, // channel emotes from here
	Tilt,
	Rent,
	Count,
};
/** The emote a chat word stands for, or -1. Prefix: the channel's emote prefix. */
SHORTSTACKCORE_API int EmoteOf(const std::string& Word, const std::string& Prefix);
/** The channel's emote prefix, from the screen name ("grinder_3c" -> "grind"). */
SHORTSTACKCORE_API std::string EmotePrefix(const std::string& Hero);

enum Badge : int
{
	BadgeSub = 1,
	BadgeMod = 2,
	BadgeVip = 4,
	BadgeBot = 8,
	BadgeStreamer = 16,
	BadgeGifter = 32,
	BadgeVerified = 64,
};

enum class LineKind : int
{
	Chat,
	Streamer, // the player
	System,   // Kast's own notices
	Tip,      // a tip with a message (read out)
	Sub,
	Cheer,
	Raid,
	Question, // someone asking the streamer something (answerable)
};

struct ChatMsg
{
	int Id = 0;
	std::string Who;
	std::string Text;
	uint32_t NameColor = 0xffffff;
	int Badges = 0;
	LineKind Kind = LineKind::Chat;
	double At = 0.0; // real seconds
	Chips Cents = 0;
	bool Toxic = false;
	bool Spam = false;
	bool Deleted = false;
	double DeleteAt = -1.0; // a moderator has seen it
	std::string DeletedBy;
	bool Answered = false;
};

enum class AlertKind : int
{
	Follow,
	Sub,
	Gift,
	Tip,
	Cheer,
	Raid,
	Milestone,
	Sponsor,
	Clip,
	Affiliate,
	Partner,
	Stage,
};

struct Alert
{
	AlertKind Kind = AlertKind::Follow;
	std::string Who;
	std::string Text; // the message under it
	int Count = 1;
	Chips Cents = 0;
	double At = -1.0; // real seconds it went up (-1 queued)
};

/** Sounds and texts the session plays out (Kast doesn't know the host). */
struct Notice
{
	enum class Kind : int
	{
		Chime, // a follow
		Sub,
		Tip,
		Raid,
		Milestone,
		Text,
	};
	Kind Type = Kind::Chime;
	std::string From;
	std::string Body;
	/** The creator update (SPEC_CREATORS §5.1, §5.4): a SoundId (Session.h) the session plays through the Kast cue queue in
	 *  place of the kind's own; -1 plays the kind's own (every notice until the Room sets one). */
	int Sound = -1;
};

/** The facecam's face. */
enum class Mood : int
{
	Focus,
	Happy,
	Hyped,
	Shocked,
	Tilted,
	Laugh,
};

/** What the stream knows about the game this tick. */
struct Inputs
{
	double World = 0.0; // world minutes
	double Real = 0.0;  // seconds
	bool AtTable = false;
	bool Results = false;
	int Tables = 0;
	bool Sprinting = false;
	std::string Hero;
	std::string EventName;
	int Remaining = 0;
	int Entrants = 0;
	int Rank = 0;
	bool InMoney = false;
	double StackBb = 0.0;
	double Tilt = 0.0;
	Chips Bankroll = 0;
	Chips RentDue = 0;
	gear::Effects Gear;
	/** The sponsor book's deals Kast works on (Session::SponsorStream: sponsor::StreamView). */
	std::vector<Deal> Deals;
	/** Update 2 (core-econ): Rerun's recent views raising the stream's strangers (§4.7), and the watch speed (§1.5: 4x
	 *  thins the strangers; 8x and a skip count as Sprinting). */
	double VideoBuzz = 0;
	int WatchSpeed = 1;
	// ---- The creator update (SPEC_CREATORS §2.2), appended: filled by Session::KastInputsExtra (core-kastglue), read by the
	// Room (core-kastroom). Until both land nothing fills them and nothing reads them.
	uint64_t Seed = 0; // Session::WorldSeed(): every keyed draw's seed (§2.12)
	// The deepest live run over every table (the largest P x depth among armed runs), with its event's facts.
	int DeepTable = -1;          // its table's id (-1: no run)
	std::string DeepTournament;  // an id for the run (one tournament's stages are granted once)
	std::string DeepEvent;       // its event's name ("$11 Big Stack")
	int DeepSite = 0;            // its site (net::SiteId)
	int DeepEntrants = 0;
	int DeepRemaining = 0;
	int DeepTableSize = 0;
	int DeepPaid = 0;            // places paid
	double Depth = 0;            // growth::Depth of the run
	double Prestige = 0;         // growth::Prestige: P
	double Series = 0;           // 1 a series main or a weekly major, 0.5 any other series event
	Chips BuyInCents = 0;
	int FrontTable = -1;         // the table in front (Hold or Crack: background tables fire AllIn too)
	int ThisTable = -1;          // the table this call is about: a moment's own (a table behind takes its turn), else the front
	                             // one; moments key on (world minute, kind, this table) (§2.12)
	double AllInEquity = -1;     // the hero's equity at the all-in, from the cached SeatVis (never computed here); -1 unknown
	bool Attended = true;        // the pace leaves real seconds (Smart, Full, watch 2x, the lobby, the results, an autoplay stop)
	std::string SeriesWindow;    // the series running now (Network::CurrentSeries), "" none
	bool LiveNightRecent = false; // a live Embercrest night that cashed in the last 48 world hours (Dee's line)
	std::vector<std::pair<double, double>> RerunHeat; // Rerun's Heat bumps with their times (video::algo::KastHeatSince)
	int RentsPaid = 0; // life::State::RentsPaid (C14: Affiliate revenue earned before the second rent waits as PendingCents)
	// ---- C-K's L5 (appended; filled by Session::KastInputsExtra, empty until it does): the Rerun videos the Room's lines name.
	std::string VideoTitle;     // the video behind VideoBuzz (the most views in the last 48 world hours): a guest from Rerun says it
	std::string RerunHeatTitle; // the video behind RerunHeat's newest bump: Heat's why line ("Your video ... got pushed hard")
};

/**
 * One moment of a stream as it happened (update 2 §4.4): what Rerun's candidates keep of the stream around a hand (the
 * viewers, the hype after it, the face, a few chat lines, the clip if someone made one). Kept on the Stream, not saved.
 */
struct MomentNote
{
	Moment Kind = Moment::Register;
	double World = 0, Real = 0;
	int Viewers = 0;
	double HypeAfter = 0;
	Mood Face = Mood::Focus;
	std::vector<std::string> Chat;
	std::string Candidate;
	double ClipAt = -1;
};

/** How a stream went (the card when it ends). */
struct Summary
{
	bool Valid = false;
	double Minutes = 0.0;
	int Avg = 0;
	int Peak = 0;
	int Follows = 0;
	int Subs = 0;
	int Gifted = 0;
	int Raids = 0;
	int Clips = 0;
	int Trolls = 0;
	Chips SubCents = 0;
	Chips BitCents = 0;
	Chips TipCents = 0;
	Chips AdCents = 0;
	Chips SponsorCents = 0;
	Chips Total() const { return SubCents + BitCents + TipCents + AdCents + SponsorCents; }
	std::string BestClip;
	int Returning = 0;   // community members who came back
	int NewFaces = 0;    // strangers who dropped in
	int NewRegulars = 0;
	double Xp = 0.0;
	bool OnSchedule = false;
	std::string RaidedOut; // the channel the stream ended by raiding
	int RaidSize = 0;
	// ---- The creator update (SPEC_CREATORS §2.2, §5.3 K17), appended: the night's facts the peak-first end card reads. Filled
	// by core-kastroom's Room; empty until then.
	double PeakAt = -1.0;    // the stream minute of the peak
	int PeakMoment = -1;     // the Moment at the peak, or -1
	double AvgOwn = 0.0;     // the average without raiders and the Broadcast (Partner's rule; Live Drop's push, NoteStream)
	double HeatBefore = 0.0; // HeatShown at the go-live...
	double GainSoFar = 0.0;  // ...and the night's gain (the ghost fill while live, committed at Stop)
	std::array<int, 11> Sources{};       // tonight's people by where they came from (View::Source's order)
	std::array<int, 11> FollowSources{}; // tonight's new followers by where they came from
	std::vector<std::string> Regulars;   // the roll call: members present for the whole run
	int Chatted = 0;         // the night's distinct chatters ("38 people chatted tonight")
	int RunAlerts = 0;       // followers a Run Alert reached tonight...
	int RunAlertsBack = 0;   // ...and the ones who came
	bool RailCalled = false;
	int RailReach = 0;
	int RailArrived = 0;
	int BroadcastSize = 0;   // the site's Broadcast tonight (0: none)
	int RankStart = 0;       // the directory rank at the go-live and the night's best ("from #31 to #6")
	int RankBest = 0;
	growth::Band Fc;         // Start's forecast ("above your forecast")
	Chips RenewCents = 0;    // renewals that came in while the streamer was away (apart from tonight's tips)
	Chips PendingCents = 0;  // held until the second rent (C14)
	std::string BestCaller;  // Hold or Crack's best caller tonight...
	int BestCalls = 0;       // ...right this many times...
	int BestCallsOf = 0;     // ...of this many
	/** A raid in and how it held: "kat_v raided with 14 · still here at 15 min: 6 · followed: 2". */
	struct RaidHeld
	{
		std::string From;
		int Size = 0;
		int At15 = 0;
		int Followed = 0;
		bool Broadcast = false;
	};
	std::vector<RaidHeld> RaidsHeld;
	int PingReached = 0;            // the go-live ping: "Your ping reached 412 of 640 followers"...
	int PingOf = 0;
	std::array<int, 3> PingParts{}; // ...those it reached by when they last watched: this week, this month, lapsed (the Community page)
	double NormBefore = 0.0;        // Channel::Norm at the go-live (Stop moves it): Live Drop's cap is 1.5x it (video::algo::NoteStream)
	/** C13: a deal's pay tonight, which KastAfterStop books as its own LedgerSponsors lines ("TunnelRat: stream read · $10"),
	 *  never inside the Kast payout. */
	struct SponsorPay
	{
		std::string Id; // the brand
		int Reads = 0;
		Chips ReadCents = 0;
		Chips HourCents = 0;
	};
	std::vector<SponsorPay> SponsorPaid;
	bool ClipItOpen = false;   // Clip of the Night: the end card offers "Clip it" (Stream::ClipIt would take it)
	bool BestClipLane = false; // tonight's best clip is in the Clip Lane ("in the Kast feed"), not unlisted
	// ---- C-K's fixes (M3, L5), appended: filled by the Room at Stop.
	std::vector<std::string> Firsts; // tonight's one-time firsts as they were posted live (the first stranger's tip, the first raid in)
	std::array<std::string, 2> Why;  // Heat's top two why lines after tonight's commit (each a fact; "" none)
};

struct Prediction
{
	bool Active = false;
	bool Resolved = false;
	bool Outcome = false;
	std::string Question;
	int Yes = 0; // channel points on each side
	int No = 0;
	double At = 0.0;         // real seconds
	double ResolvedAt = 0.0; // real seconds
};

/**
 * The creator update (SPEC_CREATORS §1.4, §5.1, §10.7): everything the UI reads of a live stream, filled once a Tick by the
 * Room (core-kastroom) so every surface (the pill, StreamSide, the VIEWERS tile, the directory card, the 24-inch Studio, the
 * tile strip) draws the same integer and no surface scans the community per chat line. Empty until the Room lands.
 */
struct View
{
	/** Where the people in the room came from (Sources, Summary::Sources). */
	enum class Source : int
	{
		Members, // the community (the Ping, latecomers, Run Alerts, Rail Call)
		Directory,
		Fresh,   // Fresh on Kast
		Rising,  // Rising in Poker
		Rail,    // railbirds
		Raid,
		Broadcast,
		Word,    // word of mouth
		Video,   // Rerun
		Clip,
		Lurkers,
		Count,
	};
	int Count = 0;      // whole people watching now
	int ShownCount = 0; // the count the surfaces draw (one count-up clock for all of them)
	int Chatting = 0;   // distinct speakers in the last 2 real minutes who are still present (<= Count)
	std::array<int, static_cast<size_t>(Source::Count)> Sources{};
	double Heat = 0, GhostGain = 0; // HeatShown and tonight's gain so far
	int Tier = 0;                   // growth::Tier
	std::array<std::string, 2> Why; // Heat's top two reasons, each a fact
	bool EventMode = false;         // the last two tables of the deepest run
	int DeepRemaining = 0, DeepEntrants = 0;
	double Prestige = 0;
	std::string DeepEvent;
	/** StreamSide's one action slot (Hold or Crack > Welcome the raid > Rail Call; the others wait as chips). */
	enum class Slot : int
	{
		None,
		HoldCrack,
		WelcomeRaid,
		RailCall,
	};
	Slot Action = Slot::None;
	int Waiting = 0; // the actions waiting as small chips under the strip: bits 1 << Slot (the one in Action excluded)
	int RailReach = 0, RailArrived = 0;
	bool WholeRail = false; // every follower is here already: no RAIL CALL, and StreamSide says "Your whole rail is here"
	int RunAlertSent = 0, RunAlertBack = 0;
	/** Hold or Crack while it's open: the present chatters' calls, the hero's equity and the streamer's own call. */
	struct HoldCrack
	{
		bool Open = false;
		int Holds = 0, Cracks = 0;
		double Equity = 0;
		int Called = 0; // the streamer's call: 0 none yet, 1 HOLDS, 2 CRACKS
	};
	HoldCrack HoC;
	int Rank = 0, RankOf = 0, RankStart = 0; // "#23 of 49 in Poker", cached once a world minute
	growth::Band Fc;
	double Stamina = 1;
	int NodsLeft = 3;
	std::vector<std::string> Lit; // the names a Nod lights on the overlay (a world minute each), guests' included
	int Thanks10 = 0;             // Thank chat's presses in the last 10 world minutes ("(chat heard you)" from two)
	bool SceneOpen = false;       // Set the scene can be pressed now (a true 0 on streams 1-20, attended, once a stream)
	bool HeadsDown = false;       // the pace leaves no real seconds and Engage is under 0.15: "heads-down tonight: chat is quieter"
	int PingReached = 0, PingOf = 0; // the go-live ping's reach: "Your ping reached 412 of 640 followers (64%)"
	std::vector<uint8_t> MemberFlags; // per member, cached each Tick: 1 regular, 2 mod, 4 Nodded, 8 present
	// ---- C-K's fixes (L11), appended: filled each Tick by the Room.
	int WelcomeRaidSecs = 0;          // WELCOME THE RAID's window left in world seconds (120..1; 0 when it isn't open): "1:52"
	int Impressions = 0;              // the directory's impressions over the last 30 world minutes...
	int ImpressionsCame = 0;          // ...and the arrivals they brought (the VIEWERS tile's line)
	int LiveChannels = 0;             // Poker channels live now: the directory's, the network's and yours (RankOf)
	std::vector<std::string> Nodded;  // tonight's Nodded names, guests included (no second Nod for anyone here)
};
static_assert(static_cast<size_t>(View::Source::Count) == 11, "Summary::Sources and FollowSources hold View::Source's 11 kinds");

/** The Room's and the chat games' private state (Private/ShortStack/Game/KastRoom.h, core-kastroom's; KastPlay.h,
 *  core-kastplay's): held by pointer so either can change without a Kast.h patch. */
struct RoomState;
struct PlayState;

class Stream
{
public:
	SHORTSTACKCORE_API explicit Stream(const std::string& Seed);
	// The creator update (Phase A-K): the private state behind Room and Play keeps the special members out of line (Kast.cpp),
	// where KastRoom.h and KastPlay.h are complete. A stream moves (the session starts a fresh one); it doesn't copy.
	SHORTSTACKCORE_API ~Stream();
	SHORTSTACKCORE_API Stream(Stream&& Other);
	SHORTSTACKCORE_API Stream& operator=(Stream&& Other);
	Stream(const Stream&) = delete;
	Stream& operator=(const Stream&) = delete;

	bool Live = false;
	double StartWorld = 0.0;
	double StartReal = 0.0;
	double Viewers = 0.0;
	int Peak = 0;
	double Hype = 0.0;   // 0..100
	double Health = 1.0; // chat health, 0..1
	double Engage = 0.0; // the streamer talking to chat (0..1, fades)
	std::vector<ChatMsg> Chat; // oldest first, at most 140
	std::vector<Alert> Alerts; // the one showing first
	std::vector<Alert> Feed;   // everything tonight, newest first (at most 60)
	std::vector<float> Graph;  // viewers, one sample a world minute
	std::vector<Notice> Notices; // for the session to play out (it clears them)
	Summary Tonight;
	Summary Last; // the stream that just ended (the card)
	Prediction Pred;
	/** The deals as Kast left them after its last call (Tick, SponsorRead): the session hands them back to the sponsor book
	 *  (sponsor::AbsorbStream) and clears them. Not saved. */
	std::vector<Deal> Deals;
	/**
	 * Update 2 (§4.4; core-econ keeps it): tonight's moments as they happened, at most 60, for Rerun's candidates. Not
	 * saved. (Named Moments: Tonight is the summary's.)
	 */
	std::vector<MomentNote> Moments;
	Mood Face = Mood::Focus;
	double FaceAt = 0.0;
	double AdUntil = -1.0; // real seconds
	int AdSeconds = 0;
	double LastAdWorld = 0.0;
	double ThankAt = -100.0;
	double RaidViewers = 0.0;
	std::string LastRaider;
	double LastMomentReal = -100.0;
	Moment LastMoment = Moment::Register;
	double Uptime(double World) const { return Live ? World - StartWorld : 0.0; }
	bool AdRunning(double Real) const { return AdUntil > Real; }
	/** Minutes until the next ad break is allowed (0 now). */
	SHORTSTACKCORE_API double AdCooldown(double World) const;
	bool AdsDue(double World) const { return Live && World - LastAdWorld >= 45.0; }

	SHORTSTACKCORE_API void Start(Channel& Ch, const Inputs& In);
	/** Ends the stream (raiding a small channel with the viewers when Raid names one). */
	SHORTSTACKCORE_API Summary Stop(Channel& Ch, const Inputs& In, const std::string& Raid = std::string());
	SHORTSTACKCORE_API void Tick(Channel& Ch, const Inputs& In, double DWorld, double DReal);
	SHORTSTACKCORE_API void OnMoment(Channel& Ch, const Inputs& In, Moment M, const std::string& Detail = std::string(), double Size = 1.0);
	/**
	 * Chat reacts once to a badge the player earned live (next_segment_spec §5.6; Line is badge::ChatReaction's): one
	 * chatter watching says it, and the room warms a little. Nothing while offline or with nobody watching.
	 */
	SHORTSTACKCORE_API void Celebrate(Channel& Ch, const Inputs& In, const std::string& Line);
	/** An ad break (60, 90 or 180 seconds); false when not affiliate or too soon. */
	SHORTSTACKCORE_API bool RunAd(Channel& Ch, const Inputs& In, int Seconds);
	/** A sponsor read; returns the cents earned (0 when not due). */
	SHORTSTACKCORE_API Chips SponsorRead(Channel& Ch, const Inputs& In, const std::string& Id);
	/** The streamer talks to chat for a bit (engagement up). False while it's too soon. */
	SHORTSTACKCORE_API bool Thank(Channel& Ch, const Inputs& In);
	/** Answers a question in chat. */
	SHORTSTACKCORE_API bool Answer(Channel& Ch, const Inputs& In, int MsgId);
	/** Times out whoever wrote MsgId (the player's own moderation). */
	SHORTSTACKCORE_API bool Timeout(Channel& Ch, const Inputs& In, int MsgId);
	/** Makes a chatter a moderator (at most MaxMods). */
	SHORTSTACKCORE_API bool Promote(Channel& Ch, const Inputs& In, const std::string& Name);
	SHORTSTACKCORE_API bool Demote(Channel& Ch, const std::string& Name);
	/** Chatters worth making a mod: regulars who behave, most active first. */
	SHORTSTACKCORE_API std::vector<std::string> ModCandidates(const Channel& Ch, double World) const;
	/** About how many viewers a stream started now would draw. */
	SHORTSTACKCORE_API static double Expected(const Channel& Ch, const Inputs& In);
	/** Who's watching now: community members, strangers, followers beyond the tracked community, raiders. */
	int Present = 0;
	int RegularsHere = 0;
	double Strangers = 0.0;
	double Lurkers = 0.0;
	/** 0..1: how good the stream looks and sounds with this gear and this many tables. */
	SHORTSTACKCORE_API static double Quality(const Inputs& In);
	/** Mods watching this stream right now. */
	int ModsOnline() const { return static_cast<int>(OnlineMods.size()); }
	const std::vector<std::string>& ModsHere() const { return OnlineMods; }
	/** Unique chatters this stream. */
	int Chatters() const { return static_cast<int>(Seen.size()); }
	/** Trolls caught (by mods, the bot or the streamer) and missed this stream. */
	int Caught = 0;
	int Missed = 0;
	/** Resolves the prediction when the tournament ends. */
	SHORTSTACKCORE_API void Resolve(Channel& Ch, const Inputs& In, bool Yes);

	static constexpr int MaxMods = 6;

	// ---- The creator update (SPEC_CREATORS §1.2, §2.9, §10.7). Declared in Phase A-K: the Room's bodies are core-kastroom's
	// (Kast.cpp), the verbs' core-kastplay's (KastPlay.cpp, through a RoomView Kast.cpp implements). Until they land every one
	// is inert: nothing calls them, they change nothing and the verbs refuse. Start, Stop, Tick, OnMoment and SponsorRead keep
	// their signatures.
	/** Runs the room on whole world minutes from StartWorld up to In.World, and nothing else changes channel state (R3). */
	SHORTSTACKCORE_API void AdvanceTo(Channel& Ch, const Inputs& In);
	/** Display chat over DReal real seconds: speakers sampled from the people present through the stream's own generator.
	 *  Changes no channel state. */
	SHORTSTACKCORE_API void ChatStep(Channel& Ch, const Inputs& In, double DReal);
	/** AdvanceTo(In.World): the session's call before OnMoment, so a moment never fires against the last tick's room (F5c). */
	SHORTSTACKCORE_API void Recount(Channel& Ch, const Inputs& In);
	/** What the UI reads (an empty view until the Room fills it). */
	SHORTSTACKCORE_API const View& GetView() const;
	/** Whole people watching now: View::Count. */
	SHORTSTACKCORE_API int Count() const;
	/** Nod at a chatter: their name lights for a world minute (3 a stream, one a member). */
	SHORTSTACKCORE_API bool Nod(Channel& Ch, const std::string& Name);
	/** RAIL CALL: every follower not here gets a second roll (once a world day, from depth 0.4, attended). */
	SHORTSTACKCORE_API bool RailCall(Channel& Ch, const Inputs& In);
	/** Welcome the raid: within 2 world minutes of it, attended. */
	SHORTSTACKCORE_API bool WelcomeRaid(Channel& Ch);
	/** Set the scene: at a true 0 on streams 1-20, once a stream. */
	SHORTSTACKCORE_API bool SetTheScene(Channel& Ch);
	/** Hold or Crack: the streamer calls it. */
	SHORTSTACKCORE_API bool CallIt(Channel& Ch, bool Holds);
	/** Clip of the Night (§2.7.2): the end card's "Clip it" clips the ended stream's best moment, even at 0 viewers, while
	 *  Summary::ClipItOpen (fewer than 2 in today's Clip Lane, 30+ world minutes, once a world day: CotnDay). */
	SHORTSTACKCORE_API bool ClipIt(Channel& Ch, const Inputs& In);
	/** The forecast for a stream started now (§2.11): "Tonight: likely 8-14", shown x e^Bias (Session::KastForecast). */
	SHORTSTACKCORE_API static growth::Band Forecast(const Channel& Ch, const Inputs& In);
	/** ss.Kast.*'s verbs that need the Room (Session::DebugKast forwards them): "room" (who is here by source, the guests'
	 *  leave times, the next slot), "raid <size> [broadcast]", "moment <stage> [prestige] [entrants]" (a deepest run held at
	 *  that stage until the stream ends), "forecast" (its terms). The answer to print; "" for a verb it doesn't take. */
	SHORTSTACKCORE_API std::string DebugRoom(Channel& Ch, const Inputs& In, const std::string& Args);
	/** Puts V up as the stream's view until the Room's next fill: kast_ui's pages draw states the Room hasn't reached. */
	SHORTSTACKCORE_API void DebugView(const View& V);

private:
	// The creator update (Phase A-K): core-kastroom's helpers (RoomState's members in KastRoom.h, and the RoomView it implements
	// for the chat games) reach the stream's private parts below (Say, Queue, Earn, R, the debts) without a Kast.h patch.
	friend struct RoomState;
	/** A community member watching tonight: when they turn up, when they leave, what happened. */
	struct Visit
	{
		int Member = 0;
		double Arrive = 0.0;
		double Leave = 0.0;
		double Watched = 0.0;
		bool Chatted = false;
		bool Engaged = false; // answered, greeted, thanked
	};
	std::vector<Visit> Visits;
	std::vector<int> Here; // indices into Visits, watching now
	double RollAt = 0.0;
	bool Scheduled = false;
	double StrangerRate(const Channel& Ch, const Inputs& In) const;
	double StrangerStay(const Inputs& In) const;
	void Invite(Channel& Ch, const Inputs& In, double Chance, double MeanDelay);
	int AddMember(Channel& Ch, const Inputs& In, const std::string& Name, bool Follower, double Loyalty);
	void NewFollower(Channel& Ch, const Inputs& In, double Stay);
	int HereVisit(const Channel& Ch, const std::string& Name) const;
	void Say(const Inputs& In, const std::string& Who, const std::string& Text, LineKind Kind, int Badges, uint32_t Color, Chips Cents = 0);
	std::string Chatter(Channel& Ch, const Inputs& In, int& Badges, uint32_t& Color);
	std::string LineFor(Channel& Ch, const Inputs& In, LineKind& Kind);
	void Queue(const Alert& A);
	void Subscribe(Channel& Ch, const Inputs& In, const std::string& Who, bool Gift, int Count);
	void Earn(Channel& Ch, Chips Cents, int Source);
	void TrollStep(Channel& Ch, const Inputs& In, double DReal);
	void MakeClip(Channel& Ch, const Inputs& In, Moment M, const std::string& Detail, double Size);
	void CheckGrowth(Channel& Ch, const Inputs& In);
	void React(Channel& Ch, const Inputs& In, Moment M, int Lines);

	Rng R;
	std::vector<std::string> Pool; // the people who might be watching
	std::set<std::string> Seen;
	std::vector<std::string> OnlineMods;
	double ChatDebt = 0.0;
	double TrollDebt = 0.0;
	double TipDebt = 0.0;
	double CheerDebt = 0.0;
	double SubDebt = 0.0;
	double QuestionDebt = 0.0;
	double GraphDebt = 0.0;
	double ViewerMinutes = 0.0;
	double RaidDebt = 0.0;
	double QualityNagAt = -100.0;
	double SponsorDebt = 0.0;
	double PredDebt = 0.0;
	double GreetDebt = 0.0;
	std::string Prefix;
	double LastReal = 0.0;
	double FollowDebt = 0.0;
	double ArriveDebt = 0.0;
	double WordDebt = 0.0;
	double GiftDebt = 0.0;
	int NextId = 1;
	std::map<std::string, double> TimedOut; // name -> until (real)
	// The creator update (Phase A-K): the Room's state (KastRoom.h, core-kastroom) and the chat games' (KastPlay.h,
	// core-kastplay). Never null on a stream that wasn't moved from.
	std::unique_ptr<RoomState> Room;
	std::unique_ptr<PlayState> Play;
};

/** C-K's L11 (appended): the Poker channels live at World, the directory's and the network's (Browse's "N channels live" off
 *  air; while live, View::LiveChannels adds yours). */
SHORTSTACKCORE_API int ChannelsLive(double World);

/** The C-K PIE fix (F1, appended): StreamLog::PeakMoment and Summary::PeakMoment for a peak a raid from the network brought (the
 *  network's raids, the welcome raid): "Peak 41 on a raid, 9:48 PM". A Moment names the moment that brought the peak (a moment
 *  raid's, a big name's, the Broadcast's final table, a deep run's stage), -1 none. */
constexpr int PeakRaid = -2;
} // namespace kast
} // namespace ss
