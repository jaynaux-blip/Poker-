// Rerun 2.0's content library, its private seam (core-videocontent's): what VideoCopy.cpp (the title engine, the letters,
// the packaging read), VideoTitles.cpp (the templates), VideoThumbs.cpp (the recipes), VideoIdeas.cpp (the ideas),
// VideoComments.cpp, VideoWorld.cpp and the Session files (SessionVideo.cpp, SessionVideo3.cpp) share beyond the public
// headers. Every name lives in ss::video::vcontent; nobody pulls it in with a using-directive (the unity rules).
//
// The facts a title, a caption or a comment may say (U7: only what happened, as the player saw it) are one map of slot ->
// text (Facts below), filled from the candidates (VideoCopy.cpp's FactsFor), an idea's facts (Candidate::IdeaFacts: the
// display keys are the slots' own names, capitalised; core-videoalgo's lower-case keys ltd, ad, trend, few, w, cool, fmt
// and cls ride beside them) and the channel (a series' episode).
#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/VideoContent.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace ss
{
namespace video
{
namespace vcontent
{
// ---------------------------------------------------------------- the facts (VideoCopy.cpp)
/** What a title may say about the moment, slot -> text, and the template conditions it meets. */
struct Facts
{
	std::map<std::string, std::string> V;
	bool Won = false, Lost = false, Bubble = false, EventWon = false, Nothing = false;
	bool First = false, RentShort = false, Series = false, Partner = false, Trend = false;
	bool Storytime = false, Live = false;
	bool AllIn = false; // the player's chips went in with cards to come (the hand's equity at the all in is known): "#A "
};
/** The facts of a choice (its lead, the picks, the idea's facts, the series' episode on C). */
Facts FactsOf(const Choice& Ch, const std::vector<Candidate>& Picks, const Candidate& Lead, const Channel* C);
/** The lead of a choice's picks (a Recap's event, else the most dramatic moment). */
const Candidate& LeadOf(const Choice& Ch, const std::vector<Candidate>& Picks);
/** The template filled from the facts, or "" when it needs a fact the moment doesn't have or a condition it doesn't meet. */
std::string FillU3(const std::string& Text, const Facts& F);
/** Update 2's styling: Loud in capitals, Story as written, the rest in sentence case. */
std::string StyledU3(const std::string& Title, TitleStyle T);
/** "a b" -> "a+b", '%' and the rest escaped: a display fact's value in Candidate::IdeaFacts (no ';', '=' or whitespace). */
std::string FactEsc(const std::string& Value);
std::string FactUnesc(const std::string& Value);
/** Every "Key=value" display fact of an idea's facts (upper-case first letter), unescaped. */
std::map<std::string, std::string> DisplayFacts(const std::string& IdeaFacts);
/** "$5.50", "$1,190", "$1.2M" (Update 2's Dollars, and millions shortened). */
std::string Cash(Chips Cents);
std::string UpperU3(std::string S);
std::string LowerU3(std::string S);
/** The kind's Studio phrase ("a bad beat"), Update 3's kinds too. */
const char* KindPhraseU3(Kind K);

// ---------------------------------------------------------------- the title templates (VideoTitles.cpp: tables only)
/** What a template is for: a Kind (0 .. Kind::Count - 1), or one of these families. */
constexpr int ForVlog = 100;     // a night (Update 2's VlogKey)
constexpr int ForRecap = 101;    // a run (Update 2's RecapKey)
constexpr int ForCut = 102;      // a Cut's own titles (<= 50 characters filled)
constexpr int ForSeries = 103;   // a series' episode ({Series}, {Ep})
constexpr int ForFallback = 104; // the last word for a moment whose cell is empty ({KindPhrase} and the event)
constexpr int ForStory = 200;    // + the story key's index (StoryKeyOf)
constexpr int ForIdea = 1000;    // + the idea's id (group x 100 + n: E1 is 501)
/**
 * One template: what it's for, its style, its flags and its text. Flags: 'c' a curiosity gap, 's' a search phrase,
 * 'p' one more promise (an intensity word the rule doesn't catch), 'q' one less. Conditions lead the text as Update 2's do
 * ("#W " the hand won, "#L " lost, "#B " the bubble, "#X " the event won, "#N " not won, "#0 " no pair of the player's own),
 * and Update 3's: "#F " a first time, "#R " rent short, "#S " a series, "#P " a Partner, "#T " a trend running, "#D " not
 * storytime (a name can appear), "#V " live, "#A " an all in with cards to come (a Cut that says "all in" or "one card": a
 * fold never gets it). Keys come from the order: <for>.<style>.<n>, n counting the cell's templates in
 * table order after Update 2's own (which keep their bucket order): append, never insert above a cell's last row.
 */
struct Row
{
	int For;
	TitleStyle T;
	const char* Flags;
	const char* Text;
};
const std::vector<Row>& TitleRows();
/** The story keys (Update 2's StoryLines' order, then Update 3's life keys). */
const std::vector<std::string>& StoryKeys();
int StoryKeyOf(const std::string& Event);
/** Each style's mean trait factor, Story .. Plain (rerun_content --write pins them; TraitsOf renormalizes by them). */
extern const double TitleStyleMean[5];

/** A template as the picker sees it. */
struct Tpl
{
	std::string Key, Text;
	int For = 0;
	TitleStyle T = TitleStyle::Story;
	uint8_t Promise = 0;
	uint16_t Flags = 0; // TitleOption::Traits' tagged bits (1 a curiosity gap, 128 a search phrase)
	bool U2 = false;    // one of Update 2's own strings
};
/** Every template, Update 2's first (built once). */
const std::vector<Tpl>& Templates3();
/** The cells a choice draws from, in order (the lead's own first). */
std::vector<int> CellsFor(const Choice& Ch, const Candidate& Lead);
/** A filled template's traits: the tagged bits and the measured ones (a number, first person, a question, the length, a name). */
uint16_t TraitsOf(const Tpl& T, const std::string& Filled);
/** The family's slug in keys ("badbeat", "vlog", "idea.e1"). */
std::string ForSlug(int For);
const char* StyleSlug(TitleStyle T);

// ---------------------------------------------------------------- the ideas (VideoIdeas.cpp)
struct IdeaDef
{
	int Id = 0;
	const char* Code = "";  // "E1"
	const char* Name = "";
	double Topic = 1.0;
	uint8_t Formats = 0;    // bits by Format
	uint8_t Styles = 0;     // bits by TitleStyle: the 2-3 its titles allow
	bool Strategy = false;  // S: a year of search tail
	int Ad = -1;            // IdeaAdCategory's row
	bool Limited = false, Adult = false, Flag = false; // LTD, 18+, the clause's flag (a rival site on screen)
	int Cool = 7;           // the cooldown in days (0 none)
	int Arch[3] = {-1, -1, -1}; // its best archetypes
	Kind HandKind = Kind::Idea; // group A: the hand kind it reads from MOMENTS
};
const std::vector<IdeaDef>& IdeaTable();
const IdeaDef* IdeaById(int Id);
/** An idea's candidate on a channel: false (and why, the card's hint) when it isn't ready; group A's are their moments. */
bool MakeIdeaCandidateOn(const IdeaCtx& X, const Channel& C, int IdeaId, double World, Candidate& Out, std::string& Why);
/** What core-videoalgo reads of an idea (w, cool, ad, ltd, fmt, cls): a group A moment is tagged with it. */
std::string MomentIdeaFacts(int IdeaId);
/** A group A idea's moment in MOMENTS (its id; "" when none). */
std::string IdeaMoment(const IdeaCtx& X, const Channel& C, int IdeaId, double World);
/** A group P series: declared once (its challenge and its playlist "s<id>"), and each episode counted after it goes up. */
int DeclareSeries(Channel& C, const IdeaCtx& X, int IdeaId, double World);
void NoteEpisode(Channel& C, const IdeaCtx& X, int SeriesId, const std::string& VideoId);
/** The library's own fixture moments (one of each kind, the hand facts every title needs), for ContactSheet and the suite. */
std::vector<Candidate> FixtureMoments();
/** Research's search phrases as written (the table behind Searches; this week's trend rows say the trend's own label). */
std::vector<std::string> SearchInventory();
/** The kind of a Cut made from a published video (Make a Cut, §4.6.9): a hand video's own; an idea video's hand as its idea
 *  and its cards show it (a fold idea's hand is a fold, a busted first level the last hand, quads a monster); else Kind::Idea,
 *  one hand told by a Cut's own titles (no pot, no all in promised). Never a pot the video didn't show. */
Kind CutKindOf(const Video& Long);

// ---------------------------------------------------------------- the recipes (VideoThumbs.cpp)
/** The facts block for the compositor (thumb::FactSep "key=value" ...). */
std::string FactBlock(const std::vector<std::pair<const char*, std::string>>& KV);
/** One recipe for an archetype on a lead (deterministic from H). */
ThumbSpec RecipeFor(int Arch, const Candidate& Lead, const std::vector<Candidate>& Picks, const Facts& F, uint32_t H, int LastArch, int LastPal);
/** The archetypes a lead allows (its "never" rules), and the best fit's. */
std::vector<int> AllowedArchs(const Candidate& Lead, const Facts& F);
/** The compositor's facts (thumb::FactSep "key=value" ...) for an archetype's recipe on a lead: RecipeFor's, and a Studio
 *  edit's (the palette brings the site's colours' fact, the subject the other face's). */
std::string VtFactBlock(int Arch, const Candidate& Lead, const Facts& F, int Pal, int Subj);
/** ThumbWithin on a lead and its facts: every field inside the archetype's values, a fillable caption row, the facts as dealt. */
bool ThumbWithinOn(const ThumbSpec& S, const Candidate& Lead, const Facts& F);

// ---------------------------------------------------------------- the copy (VideoCopy.cpp)
/** The itemized statement (§4.5.7): watch-page ads with their RPM and CPM, the Cuts feed, the payout and the balance. */
std::string StatementU3(const std::string& Month, Chips WatchCents, double Rpm, double Cpm, Chips CutsCents, Chips PaidCents, Chips BalanceCents, bool Paid);
std::vector<std::string> CopyInventoryU3();
/** Publish's refusal of a recipe edited outside its archetype's values (ThumbWithin). */
std::string ThumbNotALook();
/** Whether Next is read with "an": "an $11 buy-in", "an 8% shot", "an 18 big blind pot", "an Embercrest night"; "a $1.2M
 *  score", "a Midnight Turbo vlog" (a number by its spoken first word: eight, eleven, eighteen, eighty, eight hundred). */
bool TakesAn(const std::string& Next);
/** Out ends with a lone article ("a ", "an ", "A ", "AN ") that Next is about to follow: make it the one Next is read with
 *  (Caps: the text is written in capitals, so "A" becomes "AN", not "An"). Only before a number, a "$" or a word. Rerun
 *  2.0's own fills call it where a slot's value lands (FillU3, the comments' fill); Update 2's fill never does. */
void FitArticle(std::string& Out, const std::string& Next, bool Caps = false);

// ---------------------------------------------------------------- the comments and the world's creators
/** The numbers a video's comments may say (VideoComments.cpp): its title's, its thumbnail's, the arithmetic of them, the
 *  next round number of views and the timestamps within its length; the suite checks every digit against them. */
std::vector<std::string> CommentFactsOf(const Video& V, double World);
/** A published video as a lead for Repackage's options (VideoComments.cpp): its kind, cards, opponent, old title's numbers. */
Candidate RepackLead(const Video& V);
/** The comments' inventory (every template, unfilled) for the copy checks. */
std::vector<std::string> CommentInventory();
/** The world creators' own weekly shows' titles (VideoWorld.cpp), for the copy checks. */
std::vector<std::string> WorldShowInventory();
} // namespace vcontent

namespace video_copy
{
// Update 2's words for cards (VideoCopy.cpp), shared with the comments.
std::string HandWords(const std::vector<Card>& H); // "aces", "ace-king"
std::string RankWord(int Rank);                     // "nine"
} // namespace video_copy
} // namespace video
} // namespace ss
