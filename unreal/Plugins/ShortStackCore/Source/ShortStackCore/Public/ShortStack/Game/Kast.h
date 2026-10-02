#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/Gear.h"
#include "ShortStack/Rng.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace ss
{
/**
 * Kast: the streaming site on the laptop. The player goes live with whatever the desk has (GearDrop sells the rest),
 * plays RiverLine on stream and grows a channel: viewers drift toward what the show deserves (followers who come back,
 * strangers browsing the Poker directory, the hype of an all-in), some of them follow, a few subscribe, cheer, tip or
 * raid. Chat reacts to every hand; trolls show up as the channel grows and moderators (people the player promotes, or a
 * paid bot) clean up after them. Affiliate turns on subs, bits and ads; Partner raises the split; sponsors pay by the
 * hour. Earnings land in the channel's balance and go to the bank when the stream ends.
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
	bool NeedsPartner = false;
	Chips PerHourCents = 0;  // live hours while the deal runs
	Chips ReadCents = 0;     // a sponsor read (once a stream)
	int Days = 30;
};
SHORTSTACKCORE_API const std::vector<Sponsor>& Sponsors();
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
};

struct Deal
{
	std::string Id;
	double Since = 0.0;
	double Until = 0.0;
	double ReadAt = -1e9; // last read (world minutes)
	Chips EarnedCents = 0;
};

/** Thresholds. */
constexpr int AffiliateFollowers = 50;
constexpr double AffiliateMinutes = 8.0 * 60.0;
constexpr int AffiliateStreams = 3;
constexpr int PartnerFollowers = 2500;
constexpr int PartnerAvgViewers = 75;
constexpr Chips SubPriceCents = 499;
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
	std::vector<StreamLog> Log;   // newest first, at most 30
	std::map<std::string, int> Regulars; // chatter -> messages (the top 60 are kept)
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

	SHORTSTACKCORE_API int ActiveSubs(double World) const;
	SHORTSTACKCORE_API double AvgViewers() const; // over the last ten streams
	Chips Earned() const { return EarnedSubs + EarnedBits + EarnedTips + EarnedAds + EarnedSponsors; }
	SHORTSTACKCORE_API bool IsMod(const std::string& Name) const;
	SHORTSTACKCORE_API bool IsSub(const std::string& Name, double World) const;
	SHORTSTACKCORE_API const Deal* ActiveDeal(const std::string& Id, double World) const;
};

/** What the channel earns and loses while the player isn't live (sub renewals, clip views, deals running out). */
SHORTSTACKCORE_API void Offline(Channel& Ch, double World, Rng& R);
/** The level the channel is at, for the checklist: 0 none, 1 affiliate, 2 partner. */
SHORTSTACKCORE_API int Tier(const Channel& Ch);

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

class Stream
{
public:
	SHORTSTACKCORE_API explicit Stream(const std::string& Seed);

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
	SHORTSTACKCORE_API Summary Stop(Channel& Ch, const Inputs& In);
	SHORTSTACKCORE_API void Tick(Channel& Ch, const Inputs& In, double DWorld, double DReal);
	SHORTSTACKCORE_API void OnMoment(Channel& Ch, const Inputs& In, Moment M, const std::string& Detail = std::string(), double Size = 1.0);
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
	/** What the stream would settle at right now (viewers). */
	SHORTSTACKCORE_API double Target(const Channel& Ch, const Inputs& In) const;
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

private:
	/** Viewers the show would settle at, and how many of them are new to the channel (Fresh). */
	double Audience(const Channel& Ch, const Inputs& In, double& Fresh) const;
	void Say(const Inputs& In, const std::string& Who, const std::string& Text, LineKind Kind, int Badges, uint32_t Color, Chips Cents = 0);
	std::string Chatter(Channel& Ch, const Inputs& In, int& Badges, uint32_t& Color);
	std::string LineFor(Channel& Ch, const Inputs& In, LineKind& Kind);
	void Queue(const Alert& A);
	void Follow(Channel& Ch, const Inputs& In, int Count);
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
	std::vector<std::string> TopRegulars;
	double TopAt = -100.0;
	double LastReal = 0.0;
	int NextId = 1;
	std::map<std::string, double> TimedOut; // name -> until (real)
};
} // namespace kast
} // namespace ss
