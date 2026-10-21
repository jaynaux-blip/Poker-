#pragma once

#include "ShortStack/Common.h"

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace life
{
struct LiveVoice;
}

/**
 * What the card room's people say (next_segment_spec §4.5 to §4.8): the field's table talk by personality, the
 * dealers' calls, the floor's PA copy, and the Back Room's eight word for word. Every line claims only what the engine
 * or the save knows that tick (N4): a speaker's results come from their own Story, nobody talks about anyone's money
 * but their own (N3), and an empty Text means there's nothing they'd honestly say. Templates fill {Vars}. Every
 * inventory passes the copy lint (tests/CopyLint.h).
 *
 * How a line is picked: a moment (and a Topic, Vars["Topic"], inside it) names a pool of templates. A template is
 * eligible only when every {Var} it uses is in Vars, so a line never says what the caller couldn't fill (no bubble's
 * name, no tabled cards: no line that needs them). Of the eligible ones, those not yet told tonight (C.Told holds
 * "#<n>", the template's number) come first, and Salt picks among them. Gates read the speaker's own LiveVoice:
 * a heater or a downswing only from their Mood, online talk only from an online grinder, makeup only when they're
 * staked, results only from their Story (Vars["Record"] names which one), a needle only from someone aggressive or a
 * rival. Topic in the Said is what the line spends (a story is told once a night).
 *
 * Bodies: core-room (TableTalk.cpp).
 */
namespace talk
{
enum class Moment : int
{
	SitDown,
	Small,
	Ask,
	Answer,
	Needle,
	NiceHand,
	Suckout,
	BadBeat,
	Cooler,
	ShowedBluff,
	DoubleUp,
	Goodbye,
	Tank,
	Bubble,
	InTheMoney,
	FinalTable,
	Break,
	Noticed,
	Win,
	Lose,
	Rail,
};
/** "SitDown", "Small", ... (ss.Live.Cue reads them back with MomentFrom). */
SHORTSTACKCORE_API const char* MomentName(Moment M);
SHORTSTACKCORE_API bool MomentFrom(const std::string& Name, Moment& Out);
struct Context
{
	const life::LiveVoice* Self = nullptr;
	const life::LiveVoice* To = nullptr;
	/**
	 * The fills, and a few switches the pools read:
	 *   Topic   the pool inside the moment: Small and Answer "trip", "food", "room", "night", "week", "bubble", "heater",
	 *           "down", "online", "makeup", "regular", "friend", "rival"; Ask "average", "left", "break"; Needle "" or
	 *           "rival"; NiceHand "" or "called.made" / "called.air"; Goodbye "" or "paid" or "felted"; Break "" or
	 *           "beat"; Noticed the brand id (its category's pool when it has none of its own).
	 *   Record  Small "week" / "bubble": which of the speaker's Story results (0 the newest); Day the night's day.
	 *   Venue   "embercrest" (the Back Room's lines that belong to Dee's game stay there); Dealer "Dee" when she deals.
	 */
	std::map<std::string, std::string> Vars;
	uint32_t Salt = 0;
	const std::set<std::string>* Told = nullptr;
};
/** Empty Text: nothing they'd say. Key: the template ("field.small.trip.open.2"; the VoiceBank's key); Topic: what it
 *  spends ("story:Mei:20:Embercrest Nightly", "Mei:small.trip"); Told: the template's own "#<n>". */
struct Said
{
	std::string Text;
	std::string Key;
	std::string Topic;
	std::string Told;
	/** The pick was the last of its pool's untold templates (for this context): Remember deals the pool fresh. */
	bool Exhausts = false;
};
SHORTSTACKCORE_API Said Line(Moment M, const Context& C);
/**
 * Keeps what a line said in Told: its Topic (a story, a personal topic, a brand noticed: once a night) and its template.
 * When the line was its pool's last untold one, the pool is dealt fresh, all but the line just said, so a long night
 * varies its reactions without ever saying one twice running.
 */
SHORTSTACKCORE_API void Remember(std::set<std::string>& Told, const Said& S);
/**
 * A dealer's call (Vars as the template needs): "allin", "allin.seat", "allin.named", "allin.count" ({Spoken}:
 * chipset::Spoken), "turnup", "table.allin", "table.called", "table.seatopen", "seatopen", "cage", "blinds", "break",
 * "h4h", "average", "left", "left.itm", "break.next", "break.levels", "turn", "welcome" and "move" ({Table}, {Seat}:
 * digits, as the seat card says), "back" (the player sits back down). Dee has her own "turn" and "move" when
 * Vars["Dealer"] is "Dee". {Seat} in "allin.seat" and "turn" is said as a word ("Seat four, it's on you.").
 */
SHORTSTACKCORE_API Said Dealer(const std::string& Call, const Context& C);
/**
 * The floor on the PA (§4.7): "desk.open", "ten.minutes", "start", "start.late", "start.closed", "level", "level.ante",
 * "last.call", "reg.closed", "break", "break.next", "break.next.ante", "colorup", "five.minutes", "break.over", "away",
 * "away.bubble", "onbubble", "h4h", "h4h.lone", "h4h.call", "h4h.done", "bubble", "bubble.seat", "bust", "bust.paid",
 * "table.breaking", "moved", "two.tables", "final", "final.honor.title" / ".own" / ".article", "intro.open", "intro.seat",
 * "intro.seat.title" / ".own" / ".article" / ".nameless", "headsup", "champion", "champion.dee.hero", "champion.seat",
 * "cancelled", "courtesy".
 */
SHORTSTACKCORE_API Said Floor(const std::string& Call, const std::map<std::string, std::string>& Vars, uint32_t Salt);
/** The Back Room's eight, word for word: Small (their chat), Win, Lose, Noticed (when you stare), Goodbye (leaving a
 *  tournament). Salt picks the line; empty for anyone else or a moment they have no lines for. */
SHORTSTACKCORE_API Said Cast(const std::string& Name, Moment M, uint32_t Salt);
/** Every one of a cast member's lines for a moment, in their order (Dee's game's FLines reads them through this, so its
 *  timers and draws pick exactly what they picked before). The name matches in any case, as FLines' did. Anyone else's
 *  Goodbye is empty: the old host's own fallback was "Good luck, everyone." */
SHORTSTACKCORE_API std::vector<std::string> CastLines(const std::string& Name, Moment M);
/** Sal, Big Lou, Twitch, Mei, Mrs. Park, Rick, Dre, gh0stfold. */
SHORTSTACKCORE_API bool IsCast(const std::string& Name);
/** Their chatter (CastLooks' persona), 0 for anyone else. */
SHORTSTACKCORE_API float CastChatter(const std::string& Name);
/** "aces", "seven-four", "ace-king suited". */
SHORTSTACKCORE_API std::string SpokenHand(const std::vector<Card>& Hole);
/**
 * What a caption calls someone across a live table: the world's people by their names ("Boots", "Maria G.", the cast),
 * the player by their first name (Id "hero": Name is passed as the first name), and anyone known only by an online
 * handle by the first name in it ("mike_1987" is "Mike"), otherwise "Seat 3" (Seat is 0-based). Handles are never said.
 */
SHORTSTACKCORE_API std::string CaptionName(const std::string& Id, const std::string& Name, int Seat);
/** The first name in an online handle ("mike_1987" "Mike", "DmitriVolkov" "Dmitri", "olgapetrov" "Olga"); "" for none. */
SHORTSTACKCORE_API std::string FirstNameIn(const std::string& Handle);
/** One of the room's own (a RoomLocal or the cast): their name is their name. */
SHORTSTACKCORE_API bool KnownByName(const std::string& Name);
/** Every template and variant, by speaker (the copy lint reads it): (key, template). */
SHORTSTACKCORE_API std::vector<std::pair<std::string, std::string>> Inventory();

// ------------------------------------------------------------------ the words (shared with the director)

/** Every {Key} Vars knows, filled. */
SHORTSTACKCORE_API std::string Fill(const std::string& Template, const std::map<std::string, std::string>& Vars);
/** 74 "seventy-four", 160 "one hundred sixty", 1036000 "one million thirty-six thousand". */
SHORTSTACKCORE_API std::string NumberWords(long long N);
/** 9 "ninth", 21 "twenty-first". */
SHORTSTACKCORE_API std::string OrdinalWord(int N);
/** 3 "3rd", 11 "11th". */
SHORTSTACKCORE_API std::string Ordinal(int N);
/** 1036000 "1,036,000". */
SHORTSTACKCORE_API std::string Count(long long N);
/** 16000 cents "$160", 128050 "$1,280.50" (whole dollars without cents). */
SHORTSTACKCORE_API std::string Dollars(Chips Cents);
/** The first letter upper case. */
SHORTSTACKCORE_API std::string Capitalized(std::string S);
/** One result a speaker can claim (LiveVoice::Story: their own world records, newest first). */
struct Result
{
	int Day = 0;
	std::string Event;
	int Place = 0;
	int Entries = 0;
	Chips Prize = 0;
	bool Online = false;
};
SHORTSTACKCORE_API std::vector<Result> StoryOf(const std::string& Story);
SHORTSTACKCORE_API std::string StoryText(const std::vector<Result>& Results);
/** How an event's name is said at the table: "Noon Deepstack", "Nightly", "Big Stack" (the price dropped). */
SHORTSTACKCORE_API std::string SpokenEvent(const std::string& EventName);
/** When a day was, said on Day: "today", "yesterday", "Thursday", "last week". */
SHORTSTACKCORE_API std::string DayWord(int Then, int Day);
} // namespace talk
} // namespace ss
