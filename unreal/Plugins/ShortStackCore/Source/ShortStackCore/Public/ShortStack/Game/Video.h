#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/Recall.h"

#include <array>
#include <cstdint>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
struct SaveData;
namespace life
{
struct State;
struct Mark;
}
namespace kast
{
struct Channel;
}
namespace room
{
struct HandNote;
}

/**
 * Rerun (update 2 §4): the fictional video site where the player posts highlights cut from their own hands and streams.
 * Candidates exist only for hands that happened and carry only what the player saw (U7, N3); each video rolls its numbers
 * once at upload and then earns views in closed form over world minutes (a burst, then a long tail); ad money starts at
 * the partner gate (1,000 subscribers and 4,000 watch hours in 365 days) and pays monthly into the bank (ledger kind 13).
 * Saved as video\t... lines once Update2LinesRead's U2Video bit is set. Declared by the lead in Phase A; bodies by
 * core-video (Game/Video.cpp, VideoCopy.cpp, SessionVideo.cpp). Dormant until G2.
 */
namespace video
{
/** Saved: append-only. */
enum class Kind : int
{
	BadBeat,
	Suckout,
	Cooler,
	Bluff,
	BluffCaught,
	HeroCall,
	HeroFold,
	BigPot,
	Knockout,
	Bubble,
	DeepRun,
	FinalTable,
	Win,
	Rival,
	Story,
	StreamMoment,
	Count,
};
/** Saved: append-only. */
enum class Format : int
{
	Short,
	Breakdown,
	Vlog,
	Recap,
};
/** Saved: append-only. */
enum class TitleStyle : int
{
	Story,
	Hype,
	Teaching,
	Question,
	Plain,
};
/** The thumbnail's frame. Saved: append-only. */
enum class Frame : int
{
	Reaction,
	Cards,
	Number,
	Versus,
	Trophy,
};
/** How a video's tail behaves. Saved: append-only. */
enum class Class : int
{
	News,
	Strategy,
	Short,
};
/** What the stream had going when a candidate came from it (Kast's moment log). */
struct StreamBit
{
	int Viewers = 0;
	double HypeAfter = 0;
	int Face = 0;
	std::vector<std::string> Chat;
	double ClipAt = -1; // the Kast clip it links to, by its time (clips have no id)
};
/** A highlight that could be a video: a real hand or night, as the player saw it. */
struct Candidate
{
	std::string Id;
	double At = 0;
	int Source = 0, Site = 0; // Source: 0 online, 1 the Embercrest, 2 Dee's (storytime), 3 a skip
	Kind K = Kind::BigPot;
	recall::Record Rec;
	double EquityAtAllIn = -1;
	std::string Opponent;
	int OpponentNpc = -1;
	bool Rival = false;
	std::string Event;
	Chips BuyIn = 0;
	int Field = 0, PlaceSoFar = 0;
	bool Bubble = false, Final = false, Series = false;
	bool Auto = false;
	int Style = 0;
	bool Storytime = false;
	StreamBit Stream;
	double Drama = 0, Expires = 0;
};
/** What the player picked in Studio. */
struct Choice
{
	std::vector<std::string> Candidates;
	Format F = Format::Breakdown;
	TitleStyle T = TitleStyle::Story;
	Frame Fr = Frame::Reaction;
};
struct Thumb
{
	Frame Fr = Frame::Reaction;
	int Mood = 0;
	std::string Cards; // one field, "AsKd7h"
	std::string BigText;
	uint32_t Accent = 0;
	std::string Opponent;
	int Site = 0;
};
/** A published video: its numbers are rolled once at upload (P, a, the two time constants, E, conv, retention, rpm). */
struct Video
{
	std::string Id, Title;
	double At = 0;
	Kind K = Kind::BigPot;
	Format F = Format::Breakdown;
	TitleStyle T = TitleStyle::Story;
	Frame Fr = Frame::Reaction;
	Class C = Class::News;
	Thumb Th;
	double P = 0, A = 0, Tau1 = 0, Tau2 = 0, E = 0, Conv = 0, Retention = 0, LengthMin = 0, Rpm = 0;
	std::string Sponsor;
	bool Auto = false;
	int Site = 0;
};
/** The folded tails of videos past the newest 60, per time constant; weights: views, subscribers, ad RPM (rebased each step). */
struct Tail
{
	double Total[3] = {0, 0, 0};
	double Decay[2][3] = {{0, 0, 0}, {0, 0, 0}};
};
/** An evergreen video's linear term, under 365 days old, after it left the newest 60. */
struct Stub
{
	std::string Id;
	double At = 0, E = 0, Conv = 0, Rpm = 0;
};
/** The player's channel (life::State::Video). */
struct Channel
{
	double Subs = 0;
	double PartnerAt = -1, ReviewAt = -1;
	Chips BalanceCents = 0, PaidCents = 0;
	std::vector<Candidate> Open; // at most 30
	std::vector<Video> Videos;   // at most 60
	std::array<Tail, 3> Tails{};
	std::vector<Stub> Stubs;
	std::array<float, 366> HoursByDay{};
	int HoursDay0 = 0; // until Partner
	std::set<std::string> Paid;
	double SteppedTo = 0;
	int Uploads14 = 0, Uploads7 = 0;
	std::vector<std::pair<double, std::string>> Bell; // the in-app bell, the last 20: when (world minutes), the line (video\tbell)
	double GrantedHours = 0; // dev only
};
/** What the Channel page draws. */
struct ChannelView
{
	double Subs = 0, WatchHours365 = 0, Views28 = 0;
	bool Partner = false;
	Chips Balance = 0;
	std::array<double, 28> ViewsByDay{}, SubsByDay{}, CentsByDay{}; // the Channel page's chart
};
/** A live hand's context (the card room has no Session). */
struct LiveContext
{
	int Source = 1;
	std::string EntryId, Event;
	Chips BuyIn = 0;
	int Field = 0, Remaining = 0;
	bool Bubble = false, Final = false;
	bool Auto = false;
	int Style = 0;
	double World = 0;
};
/** What the session reads at upload. */
struct UploadFacts
{
	double GearQuality = 0;
	int Sharp = 0;
	int StreamViewers = 0;
	int KastFollowers = 0;
	double Energy = 0;
};
SHORTSTACKCORE_API Kind Classify(const recall::Record& Rec, double EquityAtAllIn, bool Bubble, bool Showdown, double AvgStackBb, bool& Is);
SHORTSTACKCORE_API void NoteHand(Channel& C, const Candidate& Cand);
SHORTSTACKCORE_API void NoteLiveHand(life::State& L, const recall::Record& Rec, const room::HandNote* Note, const LiveContext& Ctx);
SHORTSTACKCORE_API void NoteStory(Channel& C, const std::string& Key, double World);
SHORTSTACKCORE_API std::string Title(const Choice& Ch, const std::vector<Candidate>& Cands, uint32_t Seed);
/** Publishes (the edit's time is the session's TimeSkip): the new video's id, or "" with nothing changed. */
SHORTSTACKCORE_API std::string Upload(Channel& C, const Choice& Ch, double World, const std::string& Seed, const UploadFacts& F);
SHORTSTACKCORE_API double Views(const Video& V, double World);
/** Views, subscribers, the gate, the review and the monthly payout from From to To (idempotent over any span). */
SHORTSTACKCORE_API void Step(Channel& C, life::State& L, Chips& Bankroll, kast::Channel& K, double From, double To);
SHORTSTACKCORE_API ChannelView View(const Channel& C, double World);
SHORTSTACKCORE_API std::vector<life::Mark> Marks(const Channel& C, double From, double To);
/** Dev (ss.Video.Grant): subscribers, watch hours and a synthetic video that gains Views over the next 30 days. */
SHORTSTACKCORE_API void Grant(Channel& C, double Subs, double Hours, double Views, double World);
SHORTSTACKCORE_API std::vector<std::string> Inventory();
SHORTSTACKCORE_API void WriteLines(const life::State& L, std::string& Out);
SHORTSTACKCORE_API bool ReadLine(const std::vector<std::string>& P, life::State& L);
SHORTSTACKCORE_API void AfterParse(SaveData& D);

// ------------------------------------------------------------------ core-video's additions (G2; append-only, §6.1)
// What Studio, the Channel page, the card room's host and Kast read beyond the day-1 contract. No struct above changed.
//
// Conventions the bodies keep (for ui-video, ue-auto and core-econ):
//   - A candidate's Rec names the player "You" (the live recall's own convention, BackRoomTable.cpp), so the online
//     records match the live ones. Storytime records (Dee's) name nobody else: "Seat 1", "Seat 2" by first appearance.
//   - Candidate::Source: 0 online, 1 the Embercrest, 2 Dee's (storytime), 3 an online hand the plan played while
//     skipping, 4 a life story (NoteStory). Rival: gh0stfold, or a name the room knows (Rep Overall >= 35).
//   - DeepRun, FinalTable and Win candidates are the event, not a hand: Rec.Pot is the prize in cents, Rec.HeroNet the
//     prize less the buy-in, Rec.Mine and Rec.Lines empty.
//   - Thumb::Mood: a kast::Mood (0..5) for the facecam; 10 + mood for the portrait (live, where nobody films: §4.4).
//     Thumb::Cards: the player's cards, then '/', the opponent's if they were shown, then '/', the board ("AsAd/7c7h/Kd8s2c").

/** Letters Rerun mails (life::Letter::Kind, Source "video"). */
enum LetterKind : int
{
	LetterReview = 1,    // "Your channel's under review."
	LetterWelcome = 2,   // the Partner Program welcome
	LetterStatement = 3, // the monthly statement (paid, or rolled over under $100)
};
/** Marks (life::Mark::Kind) Rerun adds to the wall calendar and the second screen. */
enum MarkKind : int
{
	MarkPayout = 40, // "Rerun payout Apr 21"
	MarkReview = 41, // the review's answer
};
/** Studio's PUBLISH: "" when the choice can go up, else why not (a line for the button's tooltip). */
SHORTSTACKCORE_API std::string CanUpload(const Choice& Ch, const std::vector<Candidate>& Open);
/** The thumbnail the choice will get (Studio's preview; Upload stores the same). */
SHORTSTACKCORE_API Thumb ThumbFor(const Choice& Ch, const std::vector<Candidate>& Open);
/** The candidates a choice names, in the choice's order (the lead first for a Recap). */
SHORTSTACKCORE_API std::vector<Candidate> Picked(const Choice& Ch, const std::vector<Candidate>& Open);
/** The edit: world minutes and energy (§4.5's table). */
SHORTSTACKCORE_API double EditMinutes(Format F);
SHORTSTACKCORE_API double EditEnergy(Format F);
/** The kind's topic weight (§4.4), and its label in Studio ("BAD BEAT"). */
SHORTSTACKCORE_API double KindWeight(Kind K);
SHORTSTACKCORE_API const char* KindLabel(Kind K);
SHORTSTACKCORE_API const char* FormatName(Format F);
SHORTSTACKCORE_API const char* TitleStyleName(TitleStyle T);
SHORTSTACKCORE_API const char* FrameName(Frame F);
/** Studio's right-hand column: a line on what each choice does, never a promised number. */
SHORTSTACKCORE_API std::vector<std::string> Advice(const Choice& Ch, const std::vector<Candidate>& Open);
/** The channel at World in closed form (the saved numbers are as of the last day stepped). */
SHORTSTACKCORE_API double SubsAt(const Channel& C, double World);
SHORTSTACKCORE_API double TotalViews(const Channel& C, double World);
/** Views in the 48 hours to World: kast::Inputs::VideoBuzz (§4.7). */
SHORTSTACKCORE_API double Buzz(const Channel& C, double World);
/** The card room's settle (FAutoHost::OnSettle): a DeepRun, FinalTable or Win candidate for the night, when it was one. */
SHORTSTACKCORE_API void NoteLiveRecap(life::State& L, const LiveContext& Ctx, int Place, Chips PrizeCents);
} // namespace video
} // namespace ss
