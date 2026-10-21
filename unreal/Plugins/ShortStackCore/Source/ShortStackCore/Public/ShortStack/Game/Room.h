#pragma once

#include "ShortStack/Audio/Voice.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/TableTalk.h"
#include "ShortStack/Tournament.h"

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
/** The tick just finished, table by table (Tournament::Rounds; Tournament.h gets it with Phase A's seams). */
struct TableRound;
namespace world
{
class World;
}

/**
 * The room's one voice (next_segment_spec §1.5, §4): the director is the only thing that speaks in the card room.
 * Other features hand it data (the rail's backers, the field's patches and badges, the clock); none of them calls the
 * table's Announce or the floor directly. It reads the engine (events, the rounds at every table, the hero's hands, the
 * clock) and hands the host cues: lines, dealer calls, the PA, the crowd's vocals and phone buzzes, with the mood that
 * sets the mix. Every choice is a pure function of saved state, world minutes and a seed built from ids (N5); caps and
 * rate limits count in engine ticks, never frames or real seconds, so the cue log is a pure function of the
 * tournament, the seed and the player's inputs. Write/Read keep what's been said across a checkpoint (liveroom).
 *
 * Feeding it, once per tick, in this order (the order is part of the cue log):
 *   1. Events(T, E) with StartTick's events (Balance: tables breaking, moves, the final table), then Take() and play
 *      them as the hand starts (the floor calls the final table before its first hand, not after it);
 *   2. optionally, mid-hand: AllInCalled(T, 0, HeroIn) when the action closes on an all-in and a call at the player's
 *      table, before the board runs out (the rail leans in while the cards turn), then Take();
 *   3. once the tick is finished: Hand(T, Note) when the hero's table (or the feature table, Note.TableId) played a hand
 *      the host saw, then Rounds(T, T.Rounds()), then Events(T, E) with FinishTick's (or EndFinish's) events, then Take();
 *   4. Clock(T, World, BreakLeft) every frame (the desk, last call, the break's clock), and Take() after it.
 * Take() hands over the cues in the order to play them (Delay is seconds after Take). How often the host takes them
 * changes only the delays, never what's said. Engine time is the tick at a nominal 45 s a hand, whatever the table's
 * pace: the same night at Live, Brisk or Fast says the same lines in the same hands. The clock's thresholds during play
 * (last call, registration closing) are read from the tournament's own clock at each tick, not the frame's minutes,
 * for the same reason.
 *
 * Bodies: core-room (Room.cpp).
 */
namespace room
{
enum class Mood : int
{
	Arrival,
	Early,
	Running,
	Break,
	MoneyClose,
	HandForHand,
	Burst,
	LastTables,
	FinalTable,
	HeadsUp,
	Champion,
	After,
};
SHORTSTACKCORE_API const char* MoodName(Mood M);
/** The engine's mood (§4.3) for T: Burst and Champion are the director's (they follow an event), never this. */
SHORTSTACKCORE_API Mood MoodOf(const Tournament& T, bool OnBreak, bool Dealt);
/**
 * Levels in dB for audio::CardRoom::SetMix (0 dB is the bed at today's send, about -30 dBFS). Talk already carries the
 * crowd, 10 log10(OpenShare), and the Lombard lift (talkers raise 0.5 dB for every dB the room rises, relative to a full
 * room Running): don't apply Crowd as a gain again. Crowd is the share of the room's tables running (0..1), for how many
 * walla talkers to voice. Rail -60 is off; Calls -60 is no calls.
 */
struct Mix
{
	float Bed = 0.0f;
	float Talk = 0.0f;
	float Chips = 0.0f;
	float Calls = 0.0f;
	float Rail = -60.0f;
	float Crowd = 1.0f;
};
SHORTSTACKCORE_API Mix MixFor(Mood M, float OpenShare);
enum class Kind : int
{
	Line,  // a person: a seat at a table, the rail, the bar
	Call,  // a dealer
	Pa,    // the floor on the PA (the chime first when Chime)
	Vocal, // the crowd's non-word sounds: Noise, Voices of them (Sound is its caption, "[The rail groans.]")
	Phone, // a text to the player (the toast lane, held during the player's decision)
};
enum class From : int
{
	HeroTable,
	Table,
	FeatureTable,
	Rail,
	Bar,
	Desk,
	Pa,
};
/**
 * One thing to hear. Who is the tournament player's id at a table ("npc:Mei", "p12": the host finds the body by it) or a
 * backer's name at the rail; Speaker is what the caption calls them (talk::CaptionName; "Dealer", "Floor", "Dee").
 * TableId is the engine's table (0 for none); Seat the seat at it (0-based, -1 none). Key is the template (the VoiceBank's
 * key with Speaker); Topic what the line claims or spends ("story:...", "noticed:luckypenny"), for the honesty checks.
 * Calls with Effort 0.85 or more are shouted and carry across the room; quieter ones are heard within about 12 m.
 * A line to the player with a Key starting "field.needle" is a needle (the host's Tilt +0.04, §11 Q18).
 */
struct Cue
{
	Kind What = Kind::Line;
	From Where = From::HeroTable;
	int TableId = 0;
	int Seat = -1;
	std::string Who;
	std::string Speaker;
	std::string Key;
	std::string Text;
	std::string Sound;
	audio::Vocal Noise = audio::Vocal::None;
	int Voices = 1;
	double Delay = 0.0;
	int Priority = 0;
	bool ToHero = false;
	bool Chime = false;
	float Effort = 0.6f;
	std::string Topic;
	int Tick = 0; // the engine tick it belongs to
	/**
	 * A long announcement comes as consecutive Pa cues, a sentence or two each, so every caption fits its box in two
	 * rows at any caption size: Part 0 carries the chime, 1, 2... follow it. The VoiceBank's key is Key with Part.
	 */
	int Part = 0;
};
/**
 * What the host saw of a hand at the hero's table (or, with TableId, the feature table it's watching). Names are the
 * tournament's names or ids (either works; the player is "hero"). InHand: still in at the end; Shown: hands tabled.
 */
struct HandNote
{
	int Hand = 0;
	std::vector<std::string> InHand;
	std::vector<std::string> Folded;
	std::string Aggressor;
	bool HeroFoldedToRaise = false;
	bool HeroShowedBluff = false;
	double HeroThink = 0.0;
	bool AllIn = false;
	bool Called = false;
	std::vector<std::pair<std::string, std::vector<Card>>> Shown;
	std::vector<Card> Board;
	std::vector<Card> BoardAtAllIn;
	std::vector<std::string> Winners;
	std::vector<std::string> Busted;
	Chips Pot = 0;
	Chips HeroStack = 0;
	/** The engine table it was played at (0: the hero's). */
	int TableId = 0;
};
struct RailBacker
{
	std::string Name;
	int Permille = 0;
};
/** How long a fresh world's room talks about its new books: their first thirty days (update 2 §3.3 F9). */
constexpr int FreshBooksDays = 30;
/**
 * A fresh world's books as the room can know them (update 2 §3.3 F9, U5): what the regulars' talk about the room's firsts
 * may claim. BooksOf reads them from the save's world; the host sets Director::Fresh before the night's first hand. Blank
 * false (an established world, no world, a host that never set them): the room says none of it.
 */
struct Books
{
	bool Blank = false;
	double Opened = 0.0; // when the books opened (World::StartedAt), world minutes
	double Known = 0.0;  // the world's clock as read (World::Clock): of whatever finished after it, nothing is claimed
	/** The room's events called off in the books (never played: no first is spent on them). */
	std::vector<std::string> Cancelled;
	/** Each of the room's event families' first decided event in the books, oldest first. */
	struct First
	{
		std::string Id;     // the occurrence ("embercrest-nightly@0")
		std::string Winner; // as the world has them, "" when it can't show who
		bool Hero = false;  // the player won it
	};
	std::vector<First> Firsts;
	/** Wins up the hill in the books: the player's, and who the world follows with the most (strangers' it doesn't keep). */
	int HeroWins = 0;
	std::string Most;
	int MostWins = 0;
};
class Director
{
public:
	/** Seed: live::SeedFor(E, hero) + ":room". Voices: E.Voices, or VoicesFor(E). */
	SHORTSTACKCORE_API Director(const life::LiveEntry& E, const live::Occurrence& O, const std::string& Seed, const std::string& HeroFirstName);
	/** Nothing said twice: State is the entry's liveroom (Write); empty or unreadable, what was said is worked out from T
	 *  and World (§7.5 step 6: levels, money close, hand for hand, the bubble, two tables, the final table, heads-up, last
	 *  call and registration). */
	SHORTSTACKCORE_API void Resume(const Tournament& T, const std::string& State, double World);
	/** stake::RailFor, from the host. */
	SHORTSTACKCORE_API void SetRail(const std::vector<RailBacker>& Backers);
	SHORTSTACKCORE_API void Events(const Tournament& T, const std::vector<TEvent>& E, double World);
	SHORTSTACKCORE_API void Rounds(const Tournament& T, const std::vector<TableRound>& R);
	SHORTSTACKCORE_API void Hand(const Tournament& T, const HandNote& H);
	/**
	 * Mid-hand (optional): the action has closed with an all-in and a call at the player's table (TableId 0) or the
	 * feature table, and the board is about to run out. Where someone's watching (the final table, or near the money)
	 * the rail leans in ("ooh"), and with the player in it at the final table, their backer at the rail (or someone out
	 * tonight who knows them) calls their name: "Hold, Jesse! Hold!". Hand then doesn't say those again. Without this
	 * call the rail's ooh comes with the hand's end, and a name only when the player won it.
	 */
	SHORTSTACKCORE_API void AllInCalled(const Tournament& T, int TableId, bool HeroIn);
	/** BreakLeft: real seconds left on the host's break (0 when there's none). */
	SHORTSTACKCORE_API void Clock(const Tournament& T, double World, double BreakLeft);
	SHORTSTACKCORE_API std::vector<Cue> Take();
	Mood Now() const { return Current; }
	float TalkScale = 1.0f;
	/** The name the floor calls for the player (the registration's: Person.FullName()); the first name if never set. */
	std::string HeroFullName;
	/** The host's break in real seconds (BackRoomLive.cpp's 24): the five-minute call comes at its share. */
	double BreakSeconds = 24.0;
	/** The player is up from their seat (on foot): a move is called on the PA; seated, their dealer tells them. */
	bool HeroUp = false;
	/** A fresh world's books (BooksOf, from the host before the first hand): the regulars' talk about the room's firsts. */
	Books Fresh;
	SHORTSTACKCORE_API std::string Write() const;
	SHORTSTACKCORE_API bool Read(const std::string& Text);
	/** Tables whose hands the director ran equity for, in order (RoomEquityScope: only the hero's and the feature table). */
	const std::vector<int>& EquityTables() const { return EquityLog; }
	/** The Sunday or the Saturday Big Stack: seat-by-seat final-table introductions (§11 Q20). */
	SHORTSTACKCORE_API bool Flagship() const;
	/**
	 * ss.Live.Cue <moment> [seat]: someone at the player's table (Seat, 0-based; -1 the first who has something) says that
	 * moment now, past the rates and the caps but never the honesty gates (no tabled cards, no bad beat). Rail asks the
	 * backers at the rail. Nothing the night keeps changes; the cue comes out of Take. False when nobody would say it.
	 */
	SHORTSTACKCORE_API bool Prompt(const Tournament& T, talk::Moment M, int Seat);

private:
	struct Seen
	{
		std::string Id, Name, Caption;
		int Table = 0, Seat = -1;
		bool Hero = false;
	};
	// Fixed at construction.
	std::string EntryId, SeedText, HeroFirst, EventShort, EventName, TemplateKey;
	uint32_t SeedHash = 0;
	live::Occurrence Occ;
	std::map<std::string, life::LiveVoice> Voices;
	std::map<std::string, std::string> FaceLabel, FaceLine, BadgePack;
	std::string HeroBadges; // the player's own row of E.Badges (the one name not in the roster): badge::Pack
	std::vector<std::pair<std::string, std::string>> HeroPatches, FieldPatches;
	std::set<std::string> RosterNames;
	std::vector<RailBacker> Rail;
	// The night so far (Write/Read).
	Mood Current = Mood::Arrival;
	uint32_t SaidBits = 0;
	int LevelSaid = 0, BreakSaid = 0, FiveSaid = 0, OverSaid = 0, BreakLevel = 0, BreakTick = -1;
	bool OnBreak = false, BreakCounting = false, Greet = true;
	int GreetTable = -1;
	int BurstTick = -1, ChampionTick = -1, LastTick = 0, Engine = 0, PaFree = 0, H4HHands = 0, RailRead = 0, HandsSeen = 0;
	int AskTick = -100; // the last hand the player's table asked the dealer something
	double ChampionUntil = -1.0;
	double WorldNow = 0.0;
	int HeroOutTick = -1;
	std::vector<int> TableLines;
	std::map<std::string, int> SpokeAt, SmallAt, NeedleAt, Acc;
	std::set<std::string> Told;
	std::vector<std::pair<std::string, std::string>> Beats; // name -> "Mine|Theirs|Street", retold at the break
	// Rebuilt from the tournament, never saved.
	std::map<std::string, Chips> Stacks;
	int StacksTable = -1;
	std::vector<Cue> Out;
	std::vector<int> EquityLog;
	double PaCursor = 0.0, TalkCursor = 0.0;
	int TableClock = 0;
	// Within one call: who at the rail has spoken (nobody says two things at once). Within one tick: the all-in the rail
	// already leaned in on (AllInCalled). Within one Take: when the hand's own reactions are done (the floor waits).
	std::set<std::string> RailSpoke;
	int LeanTick = -1, LeanTable = -1;
	double ReactionEnd = 0.0;

	// Room.cpp's helpers.
	uint32_t Hash(const std::string& What) const;
	double Unit(const std::string& What) const;
	bool Has(int Bit) const { return (SaidBits & (1u << Bit)) != 0; }
	void Mark(int Bit) { SaidBits |= 1u << Bit; }
	void Sync(const Tournament& T);
	void UpdateMood(const Tournament& T, double World);
	void Snapshot(const Tournament& T);
	double TickWorld(const Tournament& T) const;
	Seen Find(const Tournament& T, const std::string& IdOrName) const;
	const life::LiveVoice& VoiceFor(const Tournament& T, const Seen& P) const;
	mutable std::map<std::string, life::LiveVoice> Strangers; // voices made up for the anonymous field, as they sit down
	std::vector<Seen> AtTable(const Tournament& T, int TableId) const;
	int HeroTable(const Tournament& T) const;
	/** The player's table, still theirs on the tick that put them out (lines and calls there are "at your table"). */
	int SeatedTable(const Tournament& T) const;
	bool Sunday() const;
	std::string Spoken(const Seen& P) const;
	std::map<std::string, std::string> RoomVars(const Tournament& T) const;
	void Pa(const Tournament& T, const std::string& Name, std::map<std::string, std::string> Vars, int Priority, bool Chime, const std::string& Speaker = "Floor",
		bool Routine = true);
	/** Floor calls said as one announcement (PaTogether): each call's name and its vars, in the order they're said. */
	using FloorCalls = std::vector<std::pair<std::string, std::map<std::string, std::string>>>;
	/**
	 * One announcement of the calls in turn (the floor's milestones near the money): one chime, one "Players," (the calls
	 * after the first without theirs), and a key naming every call's template ("pa.away.0+pa.h4h.0"). One call: Pa's.
	 */
	void PaTogether(const Tournament& T, const FloorCalls& Calls, int Priority);
	/** An announcement's parts on the PA, in caption-sized parts (Pa's and PaTogether's). */
	void Announce(const talk::Said& S, int Priority, bool Chime, const std::string& Speaker);
	void Call(const Tournament& T, const std::string& Name, std::map<std::string, std::string> Vars, int TableId, float Effort, const std::string& Speaker = "Dealer",
		bool AtHero = false);
	/** Who calls the player's name from the rail: a backer there, or someone out tonight who knows them (false: nobody). */
	bool Shouter(const Tournament& T, std::string& Who, std::string& WhoId) const;
	void Shout(const Tournament& T, const std::string& Topic, int TableId, double Delay);
	bool Say(const Tournament& T, const Seen& P, talk::Moment M, std::map<std::string, std::string> Vars, bool Small, bool ToHero, const life::LiveVoice* To = nullptr,
		bool Reaction = false);
	void Vocal(audio::Vocal V, From Where, int TableId, int Count, const std::string& Sound, double Delay);
	void RailSay(const Tournament& T, int Moment, const std::string& Text = std::string(), const std::string& Key = std::string());
	/**
	 * The floor's calls near the money, each once, added to Calls when due: how far from the money (three away, or two when
	 * the field jumps there; a jump to one is the away call's on-the-bubble form), and on the bubble (one away). Milestones
	 * says them; at hand for hand they open its call.
	 */
	void Away(const Tournament& T, FloorCalls& Calls);
	void Milestones(const Tournament& T, double World);
	void Showdown(const Tournament& T, const HandNote& H, const std::vector<Seen>& Table, int TableId);
	void Chatter(const Tournament& T, const HandNote& H, const std::vector<Seen>& Table);
	void Greetings(const Tournament& T);
	void FinalTableCalls(const Tournament& T);
	void BreakOver(const Tournament& T);
	void Bust(const Tournament& T, const TEvent& E);
};
/**
 * Who says a cue, as one voice (Turns): the same key for every line one person says, wherever it's heard from. A seat, a
 * backer at the rail or someone out tonight by Who (their tournament id, a backer's name), else by the name the caption
 * gives them ("Cashier"); a dealer by their table ("dealer:3"); Dee as herself, dealing or on the PA. Empty for what no
 * one voice says: the floor's PA (one announcement at a time already, the host's PaFreeAt), the crowd's vocals, a text
 * to the player, narration.
 */
SHORTSTACKCORE_API std::string VoiceKey(const Cue& C);
/** How long a cue's words take to say by the director's own reckoning (15 characters a second, the PA's 14, and a
 *  breath), after the chime when it has one. */
SHORTSTACKCORE_API double LineSeconds(const Cue& C);
/**
 * One voice, one line at a time, across Takes. Within a Take the director spaces what one voice says (a line, then its
 * length and 0.3 s and more), but each Take's Delay counts from its own Take: a hand's start and its end, a lean
 * mid-hand, and RoomFeed::Simulated's start and finish (taken at one moment) can put one person's two lines on top of
 * each other at any pace, a backer's double-up line under her read of the stack. The host places every cue it takes, in
 * the order it takes them, on its cue clock (Place: a pure function of the cues and the clock they were taken at, so a
 * night taken at the same moments is placed the same), and keeps a second Turns on real time for what it heard (Hold and
 * FreeAt: a line lasts as long as its speech, or its clip). Only when a line starts changes, never what's said.
 */
class Turns
{
public:
	/** After a voice's line, the least before its next (the director's own within a Take). */
	static constexpr double Gap = 0.3;
	/** How long past its time a line waits for its own voice before the moment's gone (the host's overdue cue's). */
	static constexpr double Late = 8.0;
	/**
	 * When C, due at At, starts (Start): At, or once its voice has finished the line placed before it; C's line then holds
	 * the voice (LineSeconds, then Gap). False when its voice is busy until Late seconds and more past At and C isn't the
	 * PA or a milestone (Priority under 2): the moment's gone by then, the voice isn't held, and the host leaves the line
	 * to the night log. A cue with no voice starts at At.
	 */
	SHORTSTACKCORE_API bool Place(const Cue& C, double At, double& Start);
	/** The voice is speaking until At at least (the host's real length of a line it played). */
	SHORTSTACKCORE_API void Hold(const std::string& Voice, double At);
	/** When the voice is next free (0 when it has said nothing). */
	SHORTSTACKCORE_API double FreeAt(const std::string& Voice) const;

private:
	std::map<std::string, double> Until;
};
/**
 * An announcement in caption-sized parts: whole sentences, as many as fit in 96 characters (a caption box's two rows at
 * 150%, at 16:9 and at 2550x1220). The director's Pa cues come this way already; the host uses it for a floor line it
 * says itself (talk::Floor("cancelled", ...)), one caption a part.
 */
SHORTSTACKCORE_API std::vector<std::string> Announcement(const std::string& Text);
/** Voices for an entry registered before they were kept: from its roster, the cast and the faces' labels; no story. */
SHORTSTACKCORE_API std::vector<life::LiveVoice> VoicesFor(const life::LiveEntry& E);
/** For live::DescribeField: how one of the world's people sounds and what they can honestly say (their own results,
 *  their mood and who they are, their ties, and their bond with the player), fixed at registration. */
SHORTSTACKCORE_API life::LiveVoice VoiceOf(const world::World& W, int Npc, const std::string& Hero);
/** A voice for someone the save knows nothing about (the anonymous field): how they play and a hash of their name. */
SHORTSTACKCORE_API life::LiveVoice VoiceForStranger(const std::string& Name, int Style);
/**
 * A world's books for the room (update 2 §3.3 F9): Blank only for a fresh start. Each family's first decided event (a
 * result, the player's own settled entry, the world's FirstWinner news, or over by the world's clock and not called off,
 * its winner then unknown), the room's wins, and the events called off. Entries: the player's (life::State::LiveEntries).
 */
SHORTSTACKCORE_API Books BooksOf(const world::World& W, const std::vector<life::LiveEntry>& Entries);
/** BooksOf from a save's world text (SaveData::WorldText), as the card room's host has it: a fresh start in its books' first
 *  FreshBooksDays at Now is read (only the lines the books need); any other world costs its head line. */
SHORTSTACKCORE_API Books BooksOfText(const std::string& WorldText, const std::vector<life::LiveEntry>& Entries, double Now);
} // namespace room
} // namespace ss
