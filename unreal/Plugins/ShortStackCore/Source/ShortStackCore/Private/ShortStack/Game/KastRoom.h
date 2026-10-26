// The creator update (SPEC_CREATORS §1.2, §2.2-§2.10, §10.2, §10.7): the Room's private state, core-kastroom's header (private
// to the core: Kast.cpp includes it, and so may tests/kastgrowth_session.cpp by its relative path, so the Room can change without
// a Kast.h patch). Kast::Stream holds one by pointer and names RoomState its friend: RoomState's own members (the static steps
// taking the Stream, and the RoomView it implements for the chat games, KastPlay.h) use the stream's private parts (Say, Queue,
// Earn, R, the debts).
//
// The rules it keeps (§1.2): the room is whole people (members, guests, named raiders, crowd, lurkers: every term an integer);
// the room advances only on the whole-world-minute grid from StartWorld (minute m runs when the clock reaches StartWorld + m,
// with the inputs of that call), every draw that changes it is a keyed growth::Unit, and display chat (ChatStep) samples the
// people present through the stream's R and changes nothing else. Who is here at a world time t is a pure function of the
// grid's state: members on the slot process (one plan a 10-minute slot: here or away at its start and at most one flip, from
// one keyed draw), guests, raiders and crowd with their arrive and leave times. Times below are uptime (world minutes since
// StartWorld) unless they say otherwise.
#pragma once

#include "KastPlay.h"
#include "ShortStack/Game/Kast.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ss
{
namespace kast
{
struct RoomState : public RoomView
{
	View Shown; // what Stream::GetView hands the UI, filled once a Tick

	// ---- the binding: each Stream call that runs the Room points these at the live stream, so the chat games' RoomView reads it
	Stream* ViewS = nullptr;
	Channel* ViewCh = nullptr;
	const Inputs* ViewIn = nullptr;

	// ---- tonight's keys and clock
	bool RoomLive = false;
	uint64_t SeedK = 0;   // Inputs::Seed at the go-live
	uint64_t StreamK = 0; // Channel::Streams after Start's increment
	/** growth::Unit(SeedK, StreamK, T, A, B), the same integer to the bit, with the first three of its five mixes (the seed, the
	 *  stream, the tag) done once a stream (C-K's M2: the slot process draws once a member a slot). kast_growth's UnitFast checks it
	 *  against growth::Unit. */
	std::array<uint64_t, static_cast<size_t>(growth::Tag::Count)> TagKey{};
	void KeyTags();
	double U(growth::Tag T, uint64_t A, uint64_t B = 0) const;
	uint64_t PoolKey = 0; // the seed (and hero) the stream's name pool was built for
	bool PoolBuilt = false;
	int Done = 0;         // grid minutes run: minute m (uptime [m, m + 1)) runs when the clock reaches m
	double Clock = 0.0;   // the uptime the room was last brought to
	int Day0 = 0;         // the world day and week of the go-live
	int Week0 = 0;
	bool AttendedNow = true; // Inputs::Attended as the room last had it
	int RentsPaid = 2;       // Inputs::RentsPaid as the room last had it (C14's hold on Affiliate revenue)
	// The night's constant terms (Start): the stream's quality and the channel's discovery terms, read on every grid minute.
	double NightQ = 0.0, NightDisc = 1.0, NightLane = 1.0, NightRising = 1.0, NightNovelty = 1.0, NightStamina = 1.0, NightNov = 1.0;
	double SchedF = 1.0;      // the Ping's Sched: 1.3 on schedule, 0.75 no posted schedule, 0.6 posted but off it
	double HeatStart = 26.0;  // HeatShown at the go-live (Summary::HeatBefore)
	int TierStart = 1;
	double NormBefore = 0.0;
	double BiasStart = 0.0;   // Channel::Bias at the go-live (the band is shown at Fc e^Bias)
	bool WelcomeBack = false;
	int WelcomeBackIn = 0;    // members out 5+ days who checked in tonight
	bool WelcomeBackSaid = false;
	double FrontAt = -1.0;    // Front Row's window start, -1 none
	double Nominal = 120.0;   // a usual night's length here (the per-visit giving hazard's normaliser)
	double FcRaw = 0.0;       // Start's forecast centre before e^Bias (Heat's lr reads it)
	growth::Band Fc;          // Start's band as shown
	bool PingSent = false;    // a new ping went out at this go-live (one per 4 world hours)
	int PingReached = 0, PingOf = 0;
	std::array<int, 3> PingParts{};
	int LastNightFollows = 0; // the afterglow: the last stream's new followers when it ended within a world day

	// ---- the community tonight: one visit a member who is coming (or came)
	struct Visit
	{
		// The fields the slot process, presence and the lazy replay's bookkeeping read every slot come first, in two cache lines
		// (C-K's M2: SlotAt walks every visit; the rest of a visit is read when it gives, speaks or leaves).
		int Member = -1;
		uint32_t GiveVer = 0; // lazy giving's stamp for this visit's entries in the giving heaps (below)
		uint64_t Key = 0;
		// The plan for the current slot: here or away at SlotStart, at most one flip (a leave or a return) at Flip.
		double SlotStart = 0.0;
		double Flip = 1.0e18;
		double Watched = 0.0;    // minutes here before SlotStart
		double Pending = 1.0e18; // an arrival that lies beyond this slot (a first arrival, a Run Alert's, Rail Call's)
		// The slot's chance as last read (C-K's M2): 1 - e^(-10 / Sm or G) for the loyalty, presence, results screen, deep pull and
		// mod flag it was read at (a slot with the same five reuses it: the same function of the same arguments, to the bit).
		double SlotL = -1.0;
		bool Here0 = false;
		bool Gone = false;    // timed out: no return tonight
		bool Arrived = false; // came in at least once tonight
		bool Friend = false, Mod = false, Sub = false;
		bool SlotHere = false, SlotResults = false;
		double SlotDeep = -1.0, SlotChance = 0.0;
		double FirstIn = -1.0;
		int TalkSlots = 0;    // slots here as a talker (Member::Messages at Stop)
		// Lazy giving (C-K's M2): GiveH, Gave, Sub and the caches below are exact through grid minute GiveUpto, BigAt as of then is
		// GiveBigAt; its plan snapshots run back from PlanTail (newest) to PlanStop (the newest the replay has passed, not included),
		// and MomentNext is the first big moment it hasn't placed; GiveVer (above) stamps this visit's entries in the giving heaps (a
		// re-anchored bound leaves the older ones stale).
		int GiveUpto = -1;
		int PlanStop = -1, PlanTail = -1;
		std::array<bool, 3> Gave{}; // whether each gift fired (one gift a visit: tips, cheers, a new sub)
		bool SlotMod = false;
		uint8_t Source = 0;   // View::Source
		size_t MomentNext = 0;
		double GiveBigAt = -1.0e9;
		// ---- the rest of the visit
		double Pa = 0.0;  // the go-live chance (the forecast's e_i; the latecomer roll is x0.3)
		bool Engaged = false; // thanked while talking, answered
		bool Noticed = false; // Nodded or answered tonight: loyalty x1.6
		bool OnArrive = false; // the games sent for them (play::OnArrive when they come)
		int PullStage = 0;     // the stage whose Run Alert brought them (their line)
		bool Pulled = false;   // a Run Alert sent for them...
		bool PulledIn = false; // ...and they've been here since (the live strip, counted once)
		double PullWatch = 0.0; // their minutes here before the first alert (the end card counts whoever watched past it)
		bool WelcomeBack = false; // out 5+ days on a welcome-back night
		bool WholeRun = true;  // here at every stage moment of tonight's deep run (the roll call)
		double BigAt = -1.0e9; // the last big moment they were here for (the giving hazard's bunching)
		double Expect = 60.0;  // their expected minutes here tonight (the hazard's normaliser)
		std::array<double, 3> GiveH{}; // tips, cheers, a new sub: the night's integrated hazard (and Gave, above: whether it fired)...
		std::array<double, 3> Need{{-1.0, -1.0, -1.0}}; // ...against the night's keyed thresholds, -ln(1 - u) (-1: not drawn yet)
		// The giving terms as last read (the frame budget, §9.4 #13): growth::TipChance and CheerChance at Dilution 1 and SubChance
		// for the loyalty, hype and Engage they were read at (a minute with the same three reuses them), and Dil's member factor
		// for its days since the member last gave (GivingAt: each is the same function of the same arguments, to the bit).
		double GiveL = -1.0, GiveHype = -1.0, GiveE = -1.0;
		double Tip1 = 0.0, Cheer1 = 0.0, Sub1 = 0.0;
		double DilDays = -1.0, DilMember = 0.0;
	};
	std::vector<Visit> Visits;

	// ---- lazy giving (C-K's M2: per-visit giving was the minute's largest cost, a hazard sum a present member a minute, and with
	// the run's stage hype moving every minute its terms were read again for each of them). A visit's hazard sums are carried only
	// when they could cross their keyed thresholds: each kind has an upper bound, its chance at loyalty L as Ratio(L) x the chance
	// at L 1 (the formulas are separable: GiveSeparable checks them) times the minute's room terms, so the bound's running sum
	// lives once for the room (GiveCG), and a visit's threshold in it sits in a heap. A minute where the bound can reach a visit's
	// threshold replays that visit's minutes since its last exact point, with the same arithmetic as the eager loop (GiveStep) on
	// the plans, big moments and terms each minute had (Plans, BigMoments, GiveLog): every gift lands on the minute, in the order,
	// and with the sums the eager loop gives (kast_growth's EventDriven runs both ways and compares every byte).
	struct PlanView
	{
		double SlotStart = 0.0, Watched = 0.0, Flip = 1.0e18;
		bool Here0 = false;
	};
	static PlanView PlanOf(const Visit& V)
	{
		PlanView P;
		P.SlotStart = V.SlotStart;
		P.Watched = V.Watched;
		P.Flip = V.Flip;
		P.Here0 = V.Here0;
		return P;
	}
	struct PlanSnap
	{
		PlanView View;     // a visit's plan as it stood...
		uint64_t EndSeq = 0; // ...until this event replaced it
		int Prev = -1;     // the visit's snapshot before it (a new one writes nothing older: the slot walk stays in cache)
	};
	std::vector<PlanSnap> Plans;
	std::vector<int> PlanScratch; // a replay's snapshots, oldest first
	bool PlanScratchBusy = false;
	struct BigMoment
	{
		double T = 0.0; // uptime
		uint64_t Seq = 0;
	};
	std::vector<BigMoment> BigMoments;
	/** A grid minute's giving terms: the room's hype, Engage, Dil's room factor, the bunching weights near and far, the world day,
	 *  Affiliate, and the event that ran it. */
	struct GiveMinute
	{
		double Hype = 0.0, E = 0.0, DilRoom = 1.0, WN = 1.0, WF = 1.0;
		int Day = 0;
		bool Aff = false;
		uint64_t Seq = 0;
	};
	std::vector<GiveMinute> GiveLog;           // minute -> its terms
	std::array<std::vector<double>, 3> GiveCG; // minute -> the bound's running sum per kind (tip, cheer, sub)
	struct GiveDue
	{
		double Theta = 0.0; // the running sum at which the visit's bound can reach its threshold
		int Visit = -1;
		uint32_t Ver = 0;
		bool operator>(const GiveDue& O) const { return Theta > O.Theta || (Theta == O.Theta && Visit > O.Visit); }
	};
	std::array<std::vector<GiveDue>, 3> GiveHeap; // min-heaps by Theta
	std::vector<int> GiveDueNow;
	uint64_t GiveSeq = 0; // every minute's giving, plan change and big moment in order
	int GiveDone = -1;    // the last minute whose giving ran
	bool GiveLazy = false;
	/** The event-driven Room (lazy giving, presence by transition, the slot chance's and the lurker sum's caches) for every stream
	 *  started from here; kast_growth's EventDriven turns it off to run the eager walks and compares the two. */
	static bool EventDriven;
	/** Replays that found a gift before the minute that asked for them: never (a bound that held); kast_growth asserts 0. */
	static std::atomic<uint64_t> GiveLate;
	static bool GiveSeparable();
	static double GiveRatio(int Kind, double L);
	double GiveCGAt(int Kind, int Minute) const
	{
		return Minute < 0 || static_cast<size_t>(Minute) >= GiveCG[static_cast<size_t>(Kind)].size() ? 0.0 : GiveCG[static_cast<size_t>(Kind)][static_cast<size_t>(Minute)];
	}
	/** One visit's giving at grid minute T with that minute's terms, its plan and its BigAt (GivingAt's body; the replay runs it too).
	 *  True when a gift fired. */
	static bool GiveStep(Stream& S, Channel& Ch, const Inputs& In, Visit& V, const PlanView& P, double T, const GiveMinute& G, double BigAt);
	/** The lazy machinery: a visit's plan is about to change (keep the old one for its replay); a new visit; a visit brought to
	 *  minute To; its bounds re-anchored; brought to the last minute that gave; a member's loyalty written (synced, then re-anchored). */
	void PlanChanging(Visit& V, bool MarkPres = true);
	void GiveNew(const Channel& Ch, int Vi);
	void GiveReplay(Stream& S, Channel& Ch, const Inputs& In, int Vi, int To, bool FireAtTo);
	void GiveAnchor(const Channel& Ch, int Vi);
	void GiveSync(Stream& S, Channel& Ch, int Vi);
	void LoyaltyWrite(Stream& S, Channel& Ch, Member& M, double NewL);
	/** Tests: every visit's giving made exact (kast_growth reads GiveH), and one visit re-anchored after a test changed it. */
	void GiveSyncAll(Stream& S, Channel& Ch);

	std::vector<int> VisitOf;        // member index -> visit (-1 none)
	std::vector<uint64_t> MemberKey; // member index -> NameKey (once a member a stream)
	std::vector<double> MemberPa;    // member index -> the go-live chance (members added tonight: 0)
	std::unordered_map<std::string, int> MemberAt; // name -> member index
	/** Member index -> the index MemberAt holds for its name: the member itself, unless an old save holds the name twice (then the
	 *  name's last). The display step and the View's flags key people by it, as the name-keyed sets before them did. */
	std::vector<int> MemberCanon;
	int CanonOf(int MemberIx) const
	{
		return MemberIx >= 0 && static_cast<size_t>(MemberIx) < MemberCanon.size() ? MemberCanon[static_cast<size_t>(MemberIx)] : MemberIx;
	}

	// ---- guests: named strangers (at most 96 present), trolls and spam bots; beyond 96 the rest is Crowd
	struct Guest
	{
		int Pool = -1;          // the name's index in the stream's pool (-1: a bot, named in Bot)
		std::string Bot;
		double Arrive = 0.0, Leave = 0.0;
		uint8_t Source = 1;     // View::Source
		uint8_t Kind = 0;       // 0 a stranger, 1 a troll, 2 a spam bot
		int FollowAt = -1;      // the grid minute they follow at (-1 never)
		bool FirstFollower = false;
		bool Gone = false;      // timed out or followed (a member now)
	};
	std::vector<Guest> Guests;
	std::vector<double> PoolUntil; // pool index -> busy until: present, a member now, or timed out
	std::unordered_map<std::string, int> PoolAt; // pool name -> pool index (the names are unique): GuestNamed without string compares
	struct Crowd
	{
		double Arrive = 0.0, Leave = 0.0;
		uint8_t Source = 1;
		int FollowAt = -1;
		bool Gone = false;
	};
	std::vector<Crowd> Crowds;
	int BankedLeft = 0;      // Set the scene: the next arrivals stay x BankedStay
	double BankedStay = 1.0;
	bool FirstFollowerDone = true;
	int FirstFollowerAt = -1;    // stream 1: the grid minute the first guest who stayed 10+ follows at...
	bool FirstFollowed = false;  // ...and whether they did
	int ArrivedTonight = 0;  // guest arrivals tonight (Summary::NewFaces)
	int FollowedTonight = 0; // guests and raiders who followed tonight (the guest pool's names)

	// ---- raids in
	struct Raid
	{
		std::string From;
		uint32_t Color = 0;
		int Size = 0;
		double At = 0.0;
		int AtMinute = 0;      // its grid minute
		bool Broadcast = false;
		bool Welcomed = false;
		double FollowBy = 1.0;
		bool Decided = false;  // the party's follows (2 world minutes on, when the welcome window closes)
		int At15 = -1;         // raiders still here at 15 minutes
		int Followed = 0;
	};
	std::vector<Raid> Raids;
	struct Raider
	{
		std::string Name; // "" for the party beyond the 24 named
		double Arrive = 0.0, Leave = 0.0;
		int RaidIx = 0;
		int Pool = -1;    // the name's index in the stream's pool (kept busy while they're here, -1 unnamed)
		double U = 1.0;   // the follow draw
		bool Gone = false;
	};
	std::vector<Raider> Raiders;
	bool BigNamed = false;      // a big name raided tonight (once a stream)
	double LastRaidAt = -1.0e9; // the newest raid

	// ---- lurkers (one pool, an integer a slot) and the count
	int LurkersNow = 0;
	int Watchers = 0; // the count: whole people here now
	int MembersHere = 0, RegularsHere = 0, GuestsHere = 0, CrowdHere = 0, RaidersHere = 0;
	std::array<int, 11> SourcesNow{};
	std::vector<int> ModsHere; // member indices of the mods here now
	// What the grid minute's Recount saw (C-K's M2): the visits here at HereT in Visits' order, and word of mouth's sum over them
	// (max(0, L - 0.4), in that order, at the loyalties of LoyaltyWrites). RunMinute's giving and arrivals walk these instead of
	// every visit while HereFresh holds (set by its Recount, dropped when the minute ends: nothing in between moves anyone).
	std::vector<int> HereIx;
	double HereT = -1.0, HereBring = 0.0;
	bool HereFresh = false;
	// Presence by transition (C-K's M2; Recount walked every visit once a grid minute): each visit's presence as of PresT, kept
	// current by re-reading only the visits whose plan or loyalty changed (PresDirty) and those whose planned flip time has come
	// (FlipHeap); the members here, the regulars among them, and word of mouth's terms (max(0, L - 0.4), summed over the set bits
	// in visit order: the same sum to the bit, its zero terms skipped) follow. The mods are few: ModVisits in visit order.
	bool PresOn = false;
	double PresT = -1.0e18;
	std::vector<uint8_t> PresHere, PresRegular, PresDirtyFlag;
	std::vector<double> PresBring, PresTalk, PresL; // word of mouth's max(0, L - 0.4), the display chat's 0.2 + L, and L, as last read
	std::vector<uint64_t> PresBits, PresBringBits;
	std::vector<int> PresDirty, ModVisits, PresMember; // PresMember: visit -> its member (read with the bits, no visit touched)
	std::vector<std::pair<double, int>> FlipHeap; // (flip time, visit): a min-heap
	int PresMembers = 0, PresRegulars = 0;
	void PresMark(int Vi);
	void PresSize();
	void PresRead(const Channel& C, int Vi, double T, bool FromPlan);
	void PresReadAt(int Vi, double T, bool FromPlan, double L);
	void PresUpdate(const Channel& C, double T);
	uint64_t LoyaltyWrites = 0, HereLoyalty = 0; // every loyalty a live stream changes counts here (Subscribe, Timeout, Promote)
	// The lurker pool's sum of PingW over the followers not coming tonight (SlotAt), as last walked, and what it read: the visits,
	// the members, the loyalties, and the uptime until which every one of those followers stays in its PingW step (C-K's M2).
	double AwaySum = 0.0;
	int AwayTracked = 0;
	size_t AwayVisits = static_cast<size_t>(-1), AwayMembers = 0;
	uint64_t AwayLoyalty = 0;
	double AwayUntil = -1.0;

	// ---- the deepest run, the night's stages and Heat's ghost
	double RunDepth = 0.0, RunP = 0.0; // the deepest live run (or ss.Kast.Moment's held one)
	int RunEntrants = 0;
	double RunValue = 0.0;       // max over tonight's armed stages of RunValue x P (Heat's Run)
	int StageMoments = 0;        // tonight's armed stage moments of the deep run (the roll call reads WholeRun)
	double GhostNow = 0.0;       // tonight's gain so far (the ghost fill)
	int BestStageKind = -1;      // tonight's best armed stage (a growth::RunStage) and its field
	int BestStageEntrants = 0;
	// ss.Kast.Moment's held run: a deepest run at that stage until the stream ends
	bool Held = false;
	int HeldStage = 0;
	double HeldP = 0.0;
	int HeldEntrants = 0;
	int AlertSent = 0, AlertBack = 0; // tonight's Run Alerts: followers they reached, and who came

	// ---- the night's running sums (minute by minute)
	double VM = 0.0, VMOwn = 0.0, PresentSum = 0.0;
	int LastMinuteCount = 0, LastMinuteRaiders = 0;
	int PeakAt = -1;
	int PeakMoment = -1;
	int LastMomentKind = -1;
	double LastMomentAt = -1.0e9;
	double StageHype = 0.0;             // the run's stage hits on the room clock (the room's hype: RoomHypeBase 14 + this)
	double BigAtRoom = -1.0e9;          // the stream's last big moment (the giving hazard's bunching weight)...
	int GiveMinutes = 0, NearMinutes = 0; // ...and its grid minutes so far, those inside 6 world minutes after one
	double AdUntilUp = -1.0;   // the ad's room effect, a world window
	int LastAdMinute = 0;
	double QuestionDebt = 0.0; // questions a world minute (the grid's; ChatStep puts them up)
	int QuestionsDue = 0;
	std::vector<double> ThankTimes; // Thank presses in the last 10 world minutes
	int RankBest = 0, RankStart = 0, RankNow = 0, RankOf = 0;
	/** The rank's name hashes (C-K's M2): each directory and network channel's NameHash(name + day) for today ([0]) and yesterday
	 *  ([1]), kept for the world day, so the rank once a grid minute builds no strings (growth::RankAt's own result, to the bit). */
	struct RankHashes
	{
		int Today = -1000000000;
		std::array<std::vector<uint32_t>, 2> Dir, Net;
	};
	RankHashes RankDay;
	/** growth::RankAt(World, Viewers), from the day's hashes. */
	growth::Standing RoomRank(double World, int Viewers);
	std::array<int, 11> NightSources{};   // tonight's people by source (each counted once a person)
	std::array<int, 11> FollowSources{};
	std::vector<Summary::SponsorPay> SponsorPaid;
	bool FriendsSubPending = false; // Affiliate landed tonight: Dee and Mei subscribe when they're here
	Chips EarnedSubsAtStop = -1;    // EarnedSubs at the last stream's end (renewals while away), -1 unknown
	Chips RenewSince = 0;
	// The facts behind Heat's why lines (this session): the last armed stage on stream and its world day, the last night against
	// its forecast, the last top-of-feed clip's world day.
	int WhyStage = -1, WhyStageRunners = 0, WhyStageDay = -1, WhyOverPct = 0, WhyOverDay = -1, WhyClipDay = -1;
	// C-K's L5: the other bumps' facts (§2.4: a record peak, a raid of 5+ that was a fifth of the room, a follower milestone, Rerun's
	// push), each with its world day, and the last night under its forecast. WhyEpoch counts every change to these facts.
	int WhyUnderPct = 0;
	int WhyRecordPeak = 0, WhyRecordDay = -1;
	std::string WhyRaidFrom;
	int WhyRaidSize = 0, WhyRaidDay = -1;
	int WhyMilestone = 0, WhyMilestoneDay = -1;
	std::string WhyVideoTitle;
	bool WhyVideoBreakout = false;
	int WhyVideoDay = -1;
	uint64_t WhyEpoch = 0;
	// The View's why lines as last built, and what they read (FillView builds them again only when one moved: C-K's M2).
	bool WhyHeld = false;
	std::array<uint64_t, 10> WhyKey{};
	std::array<std::string, 2> WhyLines;
	/** Heat's top two reasons now, each a fact (§2.4; View::Why while live, Summary::Why at Stop). */
	std::array<std::string, 2> WhyNow(const Channel& Ch, double World) const;

	// ---- display (ChatStep's; nothing here changes the channel)
	struct Line
	{
		std::string Who;
		std::string Text;
		LineKind Kind = LineKind::Chat;
		Chips Cents = 0;
		uint32_t Color = 0;        // 0: the name's own
		int Badges = -1;           // -1: the name's own
		bool Toxic = false, Spam = false;
		double DeleteAfter = -1.0; // a catch: real seconds after it lands (-1 none; -2 missed)
		std::string DeletedBy;
		double Delay = 0.0;        // real seconds after it goes up that it lands (a burst)
		double World = -1.0;       // the uptime minute it goes up at (a troll, a raider arriving later in the minute); -1 now
	};
	std::array<double, 30> SeenRing{}; // R6: the directory's impressions and the arrivals of each of the last 30 grid minutes
	std::array<int, 30> CameRing{};
	Inputs LastIn;                     // the inputs the room last had (the verbs that take none speak with them)
	std::vector<Line> DueLines; // lines the grid set off (a troll's line, raiders' hellos), put up at the end of AdvanceTo once the
	                       // clock reaches their minute, if the speaker is here then
	// The named people here at the last display step (arrivals say hi): members by MemberCanon (a flag a member, not a set of
	// every name: the frame budget), the named guests by name (a guest who just followed is the same person: no second hello).
	std::vector<uint8_t> ShownMembers, ShownMembersNow;
	std::unordered_set<std::string> ShownGuests;
	std::vector<uint8_t> ShownPool; // ...by pool index while the pool is indexed (C-K's M2: no set of names a frame)
	std::vector<int> ShownPoolList;
	struct Spoken
	{
		std::string Who;
		double At = 0.0; // the real second it landed
		int Member = -1; // MemberAt's index for Who once they are a member (members stay members tonight), -1 not yet
	};
	std::vector<Spoken> Spoke;   // who spoke when: "N chatting" (the last 2 real minutes)
	std::vector<int> SpokeStamp; // MemberAt index -> the View fill that last counted them (one count a person)
	std::unordered_set<std::string_view> SpokeOthers; // the fill's non-member speakers (FillView empties it each time)
	std::vector<uint8_t> FlagsBase;      // View::MemberFlags without the present bit, as of FlagsKey (C-K's M2)
	std::array<uint64_t, 7> FlagsKey{};  // loyalty writes, members, visits, mod visits, Nods, names indexed, the mods
	int SpokeFill = 0;
	double CountFrom = 0.0, CountTo = 0.0, CountAtReal = -1.0e9; // ShownCount's 0.3 s count-up
	double EmptyLineAt = -1.0e9; // R6: an empty room's system line, at most one every 3 real minutes
	int EmptyLineIx = 0;
	bool DeeHiSaid = false, MeiHiSaid = false, DeeEmbercrestSaid = false;
	int ModsSaid = 0;
	double LastPurgeReal = -1.0e9;

	// ---- the RoomView (KastPlay.h): the chat games' window on the live room
	double Minute() const override;
	uint64_t Seed() const override;
	uint64_t StreamNo() const override;
	bool Attended() const override;
	double Sched() const override;
	double ToD() const override;
	int Watching() const override;
	bool Present(const std::string& Name) const override;
	std::vector<std::string> PresentNames() const override;
	bool Attending(const std::string& Name) const override;
	bool Speak(const std::string& Who, const std::string& Text, LineKind Kind) override;
	void PostAlert(const Alert& A) override;
	void PostNotice(const Notice& N) override;
	double Engage() const override;
	void AddEngage(double By) override;
	void AddHype(double By) override;
	bool Follow(const std::string& Name) override;
	void MarkNoticed(const std::string& Name) override;
	void ArriveAt(const std::string& Name, double At, View::Source From) override;
	double RaidAt() const override;
	std::vector<std::string> RaidNamesHere() const override;
	void WelcomeRaiders(double StayBy, double FollowBy) override;
	void BankArrivals(int N, double StayBy) override;

	// ---- the Room's steps (Kast.cpp): static members, so they reach the stream's private parts through the friend
	static void Begin(Stream& S, Channel& Ch, const Inputs& In);
	static void Advance(Stream& S, Channel& Ch, const Inputs& In);
	static void RunMinute(Stream& S, Channel& Ch, const Inputs& In, int M);
	static void Display(Stream& S, Channel& Ch, const Inputs& In, double DReal);
	static void FillView(Stream& S, Channel& Ch, const Inputs& In);
	static void Close(Stream& S, Channel& Ch, const Inputs& In, Summary& Out, const std::string& RaidOut);
	static void MomentStep(Stream& S, Channel& Ch, const Inputs& In, ss::kast::Moment M);
	static void Earned(Stream& S, Channel& Ch, Chips Cents, int Source);
	static void AddRaid(Stream& S, Channel& Ch, const Inputs& In, const std::string& From, uint32_t Color, int Size, bool Broadcast, double At, int Kind);
	static int AddMemberKeyed(Stream& S, Channel& Ch, const std::string& Name, bool Follower, double Loyalty, double World);
	static int AddVisit(Stream& S, Channel& Ch, int MemberIx, double Arrive, uint8_t Source);
	static void FollowGuest(Stream& S, Channel& Ch, int GuestIx, double T, bool Announce);
	static void FollowName(Stream& S, Channel& Ch, const std::string& Name, double T, uint8_t Source, bool Here, double Until, bool First);
	static void Flush(Stream& S, const Inputs& In);
	/** A first-time milestone the moment it happens (C-K's M3; §5.3 K17: the first stranger's tip, the first raid in): its alert with
	 *  KastShine, a system line after the event's own, and the end card's mention (Summary::Firsts). */
	static void PostFirst(Stream& S, const std::string& Who, const std::string& Text);
	static void TrollStepAt(Stream& S, Channel& Ch, const Inputs& In, int M);
	static void GivingAt(Stream& S, Channel& Ch, const Inputs& In, int M, double E);
	static void ArrivalsAt(Stream& S, Channel& Ch, const Inputs& In, int M, double E);
	static void RaidsAt(Stream& S, Channel& Ch, const Inputs& In, int M);
	static void SlotAt(Stream& S, Channel& Ch, const Inputs& In, int M);
	static void LateAt(Stream& S, Channel& Ch, int M);
	static int AddGuest(Stream& S, double Arrive, double Leave, uint8_t Source, int FollowAt, double UName, uint8_t Kind);
	// A visit's plan changes only here, in SlotAt and at a visit's creation (each keeps the old plan for lazy giving's replay).
	void PullAt(Visit& V, double T, double Arrive);
	void LeaveNow(Visit& V, double T);
	static std::string Describe(Stream& S, Channel& Ch, const Inputs& In);

	// ---- presence, pure in the room's state
	static bool HereAt(const Visit& V, double T);
	static double WatchedTo(const Visit& V, double T);
	/** A talker tonight (§2.8.1's Talker; Thank's "chatted"): one keyed draw a member a stream, 0.25 + 0.5 L, as synth_kast.py's
	 *  `chatted` (S-K: the per-slot flag made every member who stayed two slots a talker, loyalty x1.3 on everyone). */
	bool TalkerTonight(const Visit& V, double L) const
	{
		return growth::Unit(SeedK, StreamK, growth::Tag::Talker, V.Key, 1u << 20) < 0.25 + 0.5 * L;
	}
	bool GuestHere(const Guest& G, double T) const { return !G.Gone && G.Arrive <= T && T < G.Leave; }
	bool RaiderHere(const Raider& X, double T) const { return !X.Gone && X.Arrive <= T && T < X.Leave; }
	/** Recounts the room at T (the count, the sources, the mods here). */
	void Recount(const Channel& C, double T);
	/** The visit of a member here now by name, or -1; the guest here now by name, or -1; the named raider here now, or -1. */
	int VisitNamed(const Channel& C, const std::string& Name) const;
	int GuestNamed(const std::string& Name) const;
	int RaiderNamed(const std::string& Name) const;
	/** A free name from the stream's pool for an arrival at T (keyed by U), skipping names present, timed out or in the
	 *  community; -1 when none is free. */
	int FreeName(const Stream& S, double U, double T) const;
	/** The binding for a call (the RoomView reads the live stream through it). */
	void Bind(Stream& S, Channel& C, const Inputs& I);

	/** The tests' window on a stream's Room (tests/kastgrowth_session.cpp): the friend reaches Stream's private pointer. */
	static const RoomState* Of(const Stream& S)
	{
		return S.Room.get();
	}
	static RoomState* Of(Stream& S)
	{
		return S.Room.get();
	}
	/** ...and on its chat games' state (read only: the careers press Rail Call when it's offered). */
	static const PlayState* PlayOf(const Stream& S)
	{
		return S.Play.get();
	}
};
} // namespace kast
} // namespace ss
