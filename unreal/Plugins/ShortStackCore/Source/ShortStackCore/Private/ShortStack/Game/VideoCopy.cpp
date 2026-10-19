#include "ShortStack/Game/Video.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Evaluator.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>

// Rerun's words (update 2 §4.5, §4.9): the five title styles' templates for every kind and for the Vlog and Recap
// formats, the thumbnails' big text, Studio's lines, Rerun's letters and the bell. core-video.
//
// A title is filled only from the candidates' facts (U7): the player's cards, a card the opponent turned up, the pot, the
// equity at the all-in, the event, the buy-in, the place, the prize. A template whose facts the moment doesn't have is
// never picked. Nothing says who decided (U1); storytime names nobody and never says whose game it was (§4.4).
//
// Inventory() lists every line for the tests' copy lint, each as "<kind>\t<text>": "title" (a template, not a sentence:
// no closing stop), "line" (a sentence), "subject", "body" (letters, filled with sample values).

namespace ss
{
namespace video
{
namespace video_detail
{
int GroupOf(Kind K); // Video.cpp
bool EventKind(Kind K);
double PotBbOf(const Candidate& C);
int MadeLevel(const std::vector<Card>& Mine, const std::vector<Card>& Board);
} // namespace video_detail

namespace video_copy
{
std::string StorytimeEvent();

// ---------------------------------------------------------------- templates

/** A template's style, for what (a Kind, or VlogKey/RecapKey), and any condition (a leading "#W " and the like). */
struct Template
{
	int For;
	TitleStyle T;
	const char* Text;
};
constexpr int VlogKey = 100;
constexpr int RecapKey = 101;
constexpr int StoryKey = 102; // a story's own lines are keyed by its story key (StoryLines)

// Conditions: "#W " the player won the hand, "#L " lost it, "#B " on the bubble, "#X " the event was won, "#N " it wasn't,
// "#0 " the player had no pair of their own on the final board (a bluff with bottom pair didn't have "nothing").
const std::vector<Template>& Templates()
{
	using K = Kind;
	using S = TitleStyle;
	static const std::vector<Template> All = {
		// ---- bad beats
		{int(K::BadBeat), S::Story, "{Eq}% to win, and then {Killer}"},
		{int(K::BadBeat), S::Story, "the {KillStreet} that ruined my night"},
		{int(K::BadBeat), S::Story, "{MineWords} all in for {Bb} big blinds. you know how this ends"},
		{int(K::BadBeat), S::Story, "I did everything right in the {Event} and it didn't matter"},
		{int(K::BadBeat), S::Story, "{MineWords} against {TheirsWords}, and the deck had other plans"},
		{int(K::BadBeat), S::Story, "the hand that wrecked my {Event}"},
		{int(K::BadBeat), S::Story, "this one still stings"},
		{int(K::BadBeat), S::Hype, "THE CRUELEST {KillStreet} I'VE EVER SEEN"},
		{int(K::BadBeat), S::Hype, "{Eq}% AND I STILL LOST"},
		{int(K::BadBeat), S::Hype, "{MineWords} CRACKED FOR {Bb} BIG BLINDS"},
		{int(K::BadBeat), S::Hype, "I CAN'T BELIEVE THIS {KillStreet}"},
		{int(K::BadBeat), S::Hype, "THE DECK HATES ME"},
		{int(K::BadBeat), S::Teaching, "Bad beats happen: why getting {MineWords} in was still right"},
		{int(K::BadBeat), S::Teaching, "Why I'd get {MineWords} in again ({Bb} big blinds, {Eq}% to win)"},
		{int(K::BadBeat), S::Teaching, "Getting it in good: {Eq}% is all you can ask for"},
		{int(K::BadBeat), S::Teaching, "How to handle a {Bb} big blind bad beat without tilting"},
		{int(K::BadBeat), S::Question, "{Eq}% to win. What would you have done next hand?"},
		{int(K::BadBeat), S::Question, "Would you get {MineWords} in here? {Stakes}, {Bb} big blinds"},
		{int(K::BadBeat), S::Question, "Is this the worst {KillStreet} in poker?"},
		{int(K::BadBeat), S::Question, "How do you bounce back from this?"},
		{int(K::BadBeat), S::Plain, "{Stakes}, hand {HandNo}: {Mine} vs {Theirs} all in"},
		{int(K::BadBeat), S::Plain, "{Stakes}: {Mine} vs {Theirs}, {Bb} big blind pot"},
		{int(K::BadBeat), S::Plain, "{Stakes}, hand {HandNo}: {Mine} all in, {Eq}% to win"},
		{int(K::BadBeat), S::Plain, "{Stakes}: {Mine} all in for {Bb} big blinds"},
		// ---- suckouts
		{int(K::Suckout), S::Story, "I had no business winning this pot"},
		{int(K::Suckout), S::Story, "{Eq}% to win and somehow still here"},
		{int(K::Suckout), S::Story, "{Saver}. sorry, not sorry"},
		{int(K::Suckout), S::Story, "the {Event} gave me one back tonight"},
		{int(K::Suckout), S::Story, "{MineWords} needed help and got it"},
		{int(K::Suckout), S::Hype, "{Eq}% AND I WON IT ANYWAY"},
		{int(K::Suckout), S::Hype, "THE LUCKIEST {SaveStreet} OF MY LIFE"},
		{int(K::Suckout), S::Hype, "{MineWords} GOT THERE"},
		{int(K::Suckout), S::Hype, "NEVER GIVE UP: {Saver}"},
		{int(K::Suckout), S::Teaching, "What {Eq}% really means: {MineWords} all in"},
		{int(K::Suckout), S::Teaching, "Running good isn't a strategy: {MineWords} at {Eq}%"},
		{int(K::Suckout), S::Teaching, "Pot odds or a prayer? {MineWords} all in for {Bb} big blinds"},
		{int(K::Suckout), S::Question, "Lucky, or was it right? {MineWords} all in at {Eq}%"},
		{int(K::Suckout), S::Question, "Would you get it in with {MineWords} here? {Stakes}"},
		{int(K::Suckout), S::Question, "Do I deserve this pot?"},
		{int(K::Suckout), S::Plain, "{Stakes}, hand {HandNo}: {Mine} vs {Theirs}, {Eq}% and won"},
		{int(K::Suckout), S::Plain, "{Stakes}: {Mine} wins a {Bb} big blind pot from behind"},
		// ---- coolers
		{int(K::Cooler), S::Story, "{MineWords} into {TheirsWords}. nothing I could do"},
		{int(K::Cooler), S::Story, "there was no folding this one"},
		{int(K::Cooler), S::Story, "the cooler that sent my {Event} sideways"},
		{int(K::Cooler), S::Story, "#L a {Bb} big blind pot I was always going to lose"},
		{int(K::Cooler), S::Story, "#W somebody had to lose this one. not me"},
		{int(K::Cooler), S::Hype, "THE BIGGEST COOLER I'VE EVER PLAYED"},
		{int(K::Cooler), S::Hype, "#L {MineWords} RAN INTO {TheirsWords}"},
		{int(K::Cooler), S::Hype, "#W {MineWords} OVER {TheirsWords} FOR {Bb} BIG BLINDS"},
		{int(K::Cooler), S::Hype, "NOBODY FOLDS HERE"},
		{int(K::Cooler), S::Teaching, "Coolers: why this {Bb} big blind pot was never a mistake"},
		{int(K::Cooler), S::Teaching, "When to stop worrying about coolers ({MineWords} vs {TheirsWords})"},
		{int(K::Cooler), S::Teaching, "Two strong hands, one pot: playing {MineWords} for stacks"},
		{int(K::Cooler), S::Question, "Can anyone fold here? {MineWords} vs {TheirsWords}"},
		{int(K::Cooler), S::Question, "Is there any way out of this hand?"},
		{int(K::Cooler), S::Question, "Would you have found a fold? {Stakes}, {Bb} big blinds"},
		{int(K::Cooler), S::Plain, "{Stakes}, hand {HandNo}: {Mine} vs {Theirs}, {Bb} big blinds"},
		{int(K::Cooler), S::Plain, "{Stakes}: cooler, {Mine} vs {Theirs}"},
		// ---- bluffs (the player's own cards: nobody at the table saw them, and the video is where they're shown)
		{int(K::Bluff), S::Story, "#0 I had nothing. they folded anyway"},
		{int(K::Bluff), S::Story, "the {Street} bluff I'm still proud of"},
		{int(K::Bluff), S::Story, "{MineWords} and a lot of nerve"},
		{int(K::Bluff), S::Story, "nobody saw my cards, so here they are"},
		{int(K::Bluff), S::Story, "the bluff that kept me in the {Event}"},
		{int(K::Bluff), S::Hype, "I BLUFFED WITH {MineWords}"},
		{int(K::Bluff), S::Hype, "#0 {Bb} BIG BLINDS WITH NOTHING"},
		{int(K::Bluff), S::Hype, "THEY'LL NEVER KNOW (UNTIL NOW)"},
		{int(K::Bluff), S::Hype, "THE {Street} BLUFF OF THE NIGHT"},
		{int(K::Bluff), S::Teaching, "How to bluff the {Street} with {MineWords} (and when not to)"},
		{int(K::Bluff), S::Teaching, "Picking a bluff: {MineWords} on the {Street}, {Bb} big blind pot"},
		{int(K::Bluff), S::Teaching, "Why I picked {MineWords} to bluff with here"},
		{int(K::Bluff), S::Teaching, "Bluffing the {Street}: the story your bets have to tell"},
		{int(K::Bluff), S::Question, "Would you have called? I had {MineWords}"},
		{int(K::Bluff), S::Question, "Call or fold? {Stakes}, {Bb} big blinds, {Street} bet"},
		{int(K::Bluff), S::Question, "What do you put me on here?"},
		{int(K::Bluff), S::Plain, "{Stakes}, hand {HandNo}: {Mine} bluff on the {Street}"},
		{int(K::Bluff), S::Plain, "{Stakes}: {Mine} takes {Bb} big blinds without a showdown"},
		// ---- bluffs caught
		{int(K::BluffCaught), S::Story, "my {Street} bluff did not go to plan"},
		{int(K::BluffCaught), S::Story, "{MineWords}, a big bet, and a very fast call"},
		{int(K::BluffCaught), S::Story, "they had it. I didn't"},
		{int(K::BluffCaught), S::Story, "caught red-handed in the {Event}"},
		{int(K::BluffCaught), S::Hype, "CAUGHT BLUFFING FOR {Bb} BIG BLINDS"},
		{int(K::BluffCaught), S::Hype, "THE WORST BLUFF I'VE EVER RUN"},
		{int(K::BluffCaught), S::Hype, "{MineWords}?! WHAT WAS I DOING"},
		{int(K::BluffCaught), S::Teaching, "Where my {Street} bluff went wrong ({Bb} big blinds)"},
		{int(K::BluffCaught), S::Teaching, "Bad bluffs: what {MineWords} on the {Street} taught me"},
		{int(K::BluffCaught), S::Teaching, "Picking the wrong spot to bluff, and how to see it next time"},
		{int(K::BluffCaught), S::Question, "Was this bluff ever working? {Stakes}, {Bb} big blinds"},
		{int(K::BluffCaught), S::Question, "Good bluff, bad timing?"},
		{int(K::BluffCaught), S::Plain, "{Stakes}, hand {HandNo}: {Mine} bluff called on the {Street}"},
		{int(K::BluffCaught), S::Plain, "{Stakes}: {Mine} bluffs and gets called, {Bb} big blinds"},
		// ---- hero calls
		{int(K::HeroCall), S::Story, "I called with {MineWords} and I'd do it again"},
		{int(K::HeroCall), S::Story, "something about that river bet felt wrong"},
		{int(K::HeroCall), S::Story, "the call that saved my {Event}"},
		{int(K::HeroCall), S::Story, "they were bluffing. I knew it"},
		{int(K::HeroCall), S::Hype, "I CALLED WITH {MineWords}"},
		{int(K::HeroCall), S::Hype, "HERO CALL FOR {Bb} BIG BLINDS"},
		{int(K::HeroCall), S::Hype, "THE SICKEST CALL I'VE EVER MADE"},
		{int(K::HeroCall), S::Hype, "I KNEW IT. I KNEW IT"},
		{int(K::HeroCall), S::Teaching, "How I knew to call the river with {MineWords}"},
		{int(K::HeroCall), S::Teaching, "Bluff catching: calling {Bb} big blinds with {MineWords}"},
		{int(K::HeroCall), S::Teaching, "Reading a river bet: calling with {MineWords}"},
		{int(K::HeroCall), S::Question, "Would you call the river with {MineWords}?"},
		{int(K::HeroCall), S::Question, "Call or fold? {Stakes}, {Bb} big blinds, river"},
		{int(K::HeroCall), S::Question, "Genius or lucky? I called with {MineWords}"},
		{int(K::HeroCall), S::Plain, "{Stakes}, hand {HandNo}: {Mine} calls the river and wins"},
		{int(K::HeroCall), S::Plain, "{Stakes}: river call with {Mine}, {Bb} big blinds"},
		// ---- hero folds (the villain's cards only when they were shown, which online folds rarely are)
		{int(K::HeroFold), S::Story, "I folded {MineWords} and I'm still thinking about it"},
		{int(K::HeroFold), S::Story, "letting go of {MineWords} {Spot}"},
		{int(K::HeroFold), S::Story, "the hardest fold of my {Event}"},
		{int(K::HeroFold), S::Story, "sometimes the best hand is the one you throw away"},
		{int(K::HeroFold), S::Hype, "I FOLDED {MineWords}"},
		{int(K::HeroFold), S::Hype, "FOLDING {MineWords} {Spot}?!"},
		{int(K::HeroFold), S::Hype, "THE HARDEST FOLD OF MY LIFE"},
		{int(K::HeroFold), S::Teaching, "#B Why I folded {MineWords} on the bubble (ICM, {Bb} big blind pot)"},
		{int(K::HeroFold), S::Teaching, "Why I folded {MineWords} {Spot}"},
		{int(K::HeroFold), S::Teaching, "Not good enough this time: folding {MineWords} {Spot}"},
		{int(K::HeroFold), S::Teaching, "Big laydowns: folding {MineWords} with {Bb} big blinds in the middle"},
		{int(K::HeroFold), S::Question, "Would you fold {MineWords} here?"},
		{int(K::HeroFold), S::Question, "Fold or call? {Stakes}, {MineWords}, {Bb} big blinds"},
		{int(K::HeroFold), S::Question, "Was folding {MineWords} {Spot} a mistake?"},
		{int(K::HeroFold), S::Plain, "{Stakes}, hand {HandNo}: {Mine} folded {Spot}"},
		{int(K::HeroFold), S::Plain, "{Stakes}: folding {Mine} {Spot}"},
		// ---- big pots
		{int(K::BigPot), S::Story, "the {Bb} big blind pot that turned my {Event} around"},
		{int(K::BigPot), S::Story, "{MineWords} and the biggest pot of my night"},
		{int(K::BigPot), S::Story, "everything went in, and it came back"},
		{int(K::BigPot), S::Story, "a pot like this doesn't come around often"},
		{int(K::BigPot), S::Hype, "{Bb} BIG BLIND POT!!"},
		{int(K::BigPot), S::Hype, "THE BIGGEST POT OF MY {Event}"},
		{int(K::BigPot), S::Hype, "{MineWords}, SHIP IT"},
		{int(K::BigPot), S::Hype, "MASSIVE POT WITH {MineWords}"},
		{int(K::BigPot), S::Teaching, "Playing a {Bb} big blind pot with {MineWords}, street by street"},
		{int(K::BigPot), S::Teaching, "How {MineWords} won a {Bb} big blind pot"},
		{int(K::BigPot), S::Teaching, "Building a big pot with {MineWords}: what went right"},
		{int(K::BigPot), S::Question, "Would you play {MineWords} the same way? {Bb} big blinds"},
		{int(K::BigPot), S::Question, "How would you play this {Bb} big blind pot?"},
		{int(K::BigPot), S::Question, "Too risky, or exactly right? {MineWords} in the {Event}"},
		{int(K::BigPot), S::Plain, "{Stakes}, hand {HandNo}: {Mine} wins {Bb} big blinds"},
		{int(K::BigPot), S::Plain, "{Stakes}: {Bb} big blind pot with {Mine}"},
		// ---- knockouts
		{int(K::Knockout), S::Story, "sending someone home from the {Event}"},
		{int(K::Knockout), S::Story, "{MineWords} for a knockout"},
		{int(K::Knockout), S::Story, "one fewer at the table, and it was my doing"},
		{int(K::Knockout), S::Hype, "KNOCKOUT!"},
		{int(K::Knockout), S::Hype, "SENT PACKING BY {MineWords}"},
		{int(K::Knockout), S::Hype, "ANOTHER ONE GONE"},
		{int(K::Knockout), S::Teaching, "Putting a short stack to the test with {MineWords}"},
		{int(K::Knockout), S::Teaching, "When to call a shove: {MineWords} for a knockout"},
		{int(K::Knockout), S::Question, "Would you call the shove with {MineWords}?"},
		{int(K::Knockout), S::Question, "Easy call or hero call? {MineWords} for a knockout"},
		{int(K::Knockout), S::Plain, "{Stakes}, hand {HandNo}: {Mine} knocks out a player"},
		{int(K::Knockout), S::Plain, "{Stakes}: knockout with {Mine}"},
		// ---- the bubble
		{int(K::Bubble), S::Story, "all in on the bubble of the {Event}"},
		{int(K::Bubble), S::Story, "#W surviving the bubble with {MineWords}"},
		{int(K::Bubble), S::Story, "#L busting on the bubble. it hurts as much as you'd think"},
		{int(K::Bubble), S::Story, "the longest minute of my {Event}"},
		{int(K::Bubble), S::Hype, "ALL IN ON THE BUBBLE!"},
		{int(K::Bubble), S::Hype, "#W {MineWords} SAVED MY TOURNAMENT"},
		{int(K::Bubble), S::Hype, "#L THE WORST PLACE TO BUST"},
		{int(K::Bubble), S::Teaching, "Shoving the bubble with {MineWords}: the ICM math"},
		{int(K::Bubble), S::Teaching, "Bubble play: shoving {MineWords} with the money in sight"},
		{int(K::Bubble), S::Question, "Shove {MineWords} on the bubble?"},
		{int(K::Bubble), S::Question, "Would you risk it here? Bubble of the {Event}"},
		{int(K::Bubble), S::Plain, "{Stakes}, hand {HandNo}: {Mine} all in on the bubble"},
		{int(K::Bubble), S::Plain, "{Stakes}: bubble all in with {Mine}"},
		// ---- deep runs
		{int(K::DeepRun), S::Story, "{PlaceWord} of {Field} in the {Event}"},
		{int(K::DeepRun), S::Story, "a deep run in the {Event}, and what it paid"},
		{int(K::DeepRun), S::Story, "{Prize} from a {BuyIn} buy-in"},
		{int(K::DeepRun), S::Story, "the night the {Event} finally went my way"},
		{int(K::DeepRun), S::Hype, "{Prize} FROM {BuyIn}!"},
		{int(K::DeepRun), S::Hype, "{PlaceWord} OF {Field}!"},
		{int(K::DeepRun), S::Hype, "THE RUN OF THE NIGHT"},
		{int(K::DeepRun), S::Teaching, "How I went deep in the {Event} ({PlaceWord} of {Field})"},
		{int(K::DeepRun), S::Teaching, "Deep run lessons from the {Event}"},
		{int(K::DeepRun), S::Question, "Could I have gone further? {PlaceWord} of {Field} in the {Event}"},
		{int(K::DeepRun), S::Question, "What would you have done differently? {Prize} in the {Event}"},
		{int(K::DeepRun), S::Plain, "{Event}: {Place} of {Field} for {Prize}"},
		{int(K::DeepRun), S::Plain, "{Stakes}: {Place} place, {Prize}"},
		// ---- final tables
		{int(K::FinalTable), S::Story, "making the final table of the {Event}"},
		{int(K::FinalTable), S::Story, "{PlaceWord} at the {Event} final table"},
		{int(K::FinalTable), S::Story, "a seat at the last table, and what I did with it"},
		{int(K::FinalTable), S::Hype, "FINAL TABLE!"},
		{int(K::FinalTable), S::Hype, "{PlaceWord} AT THE FINAL TABLE OF THE {Event}"},
		{int(K::FinalTable), S::Teaching, "Final table strategy from my {Event} run"},
		{int(K::FinalTable), S::Teaching, "What the {Event} final table taught me ({PlaceWord} of {Field})"},
		{int(K::FinalTable), S::Question, "Where did my final table go wrong? {PlaceWord} in the {Event}"},
		{int(K::FinalTable), S::Question, "Would you have played this final table differently?"},
		{int(K::FinalTable), S::Plain, "{Event} final table: {Place} of {Field}, {Prize}"},
		{int(K::FinalTable), S::Plain, "{Stakes}: final table, {Place} place"},
		// ---- wins
		{int(K::Win), S::Story, "I won the {Event}"},
		{int(K::Win), S::Story, "{Field} players, and I outlasted all of them"},
		{int(K::Win), S::Story, "the night I won the {Event}"},
		{int(K::Win), S::Story, "first place, {Prize}, and no sleep"},
		{int(K::Win), S::Hype, "I WON THE {Event}!!"},
		{int(K::Win), S::Hype, "CHAMPION! {Prize}!"},
		{int(K::Win), S::Hype, "FROM {BuyIn} TO {Prize}"},
		{int(K::Win), S::Teaching, "How I won the {Event}: the key hands"},
		{int(K::Win), S::Teaching, "Closing out a tournament: heads-up lessons from the {Event}"},
		{int(K::Win), S::Question, "Can you spot the hand that won me the {Event}?"},
		{int(K::Win), S::Question, "What would you have done heads-up? {Event} for {Prize}"},
		{int(K::Win), S::Plain, "{Event}: 1st of {Field} for {Prize}"},
		{int(K::Win), S::Plain, "{Stakes}: winner, {Prize}"},
		// ---- the rival
		{int(K::Rival), S::Story, "{Opp}. again"},
		{int(K::Rival), S::Story, "me against {Opp} for {Bb} big blinds"},
		{int(K::Rival), S::Story, "{Opp} always finds my table"},
		{int(K::Rival), S::Story, "#W one back from {Opp}"},
		{int(K::Rival), S::Story, "#L {Opp} gets the better of me this time"},
		{int(K::Rival), S::Hype, "ME VS {Opp}!"},
		{int(K::Rival), S::Hype, "#W I GOT {Opp}!"},
		{int(K::Rival), S::Hype, "#L {Opp} DID IT AGAIN"},
		{int(K::Rival), S::Teaching, "How I play against {Opp}"},
		{int(K::Rival), S::Teaching, "Adjusting to an aggressive regular: {Opp}, {Bb} big blinds"},
		{int(K::Rival), S::Question, "How would you play against {Opp}?"},
		{int(K::Rival), S::Question, "Who wins this one, me or {Opp}?"},
		{int(K::Rival), S::Plain, "{Stakes}, hand {HandNo}: vs {Opp}, {Bb} big blinds"},
		{int(K::Rival), S::Plain, "{Stakes}: {Mine} against {Opp}"},
		// ---- stream moments
		{int(K::StreamMoment), S::Story, "the best minute of the stream"},
		{int(K::StreamMoment), S::Hype, "CHAT WENT CRAZY"},
		{int(K::StreamMoment), S::Teaching, "What streaming a session taught me about tilt"},
		{int(K::StreamMoment), S::Question, "Were you there for this?"},
		{int(K::StreamMoment), S::Plain, "Stream highlight: {Event}"},
		// ---- a night (Vlog)
		{VlogKey, S::Story, "{PlaceWord} at the {Event} (and {Ender})"},
		{VlogKey, S::Story, "{PlaceWord} of {Field}: my night at the {Event}"},
		{VlogKey, S::Story, "a {Weekday} night at the {Event}"},
		{VlogKey, S::Story, "{Count} hands from the {Event}"},
		{VlogKey, S::Story, "one night, {Count} hands"},
		{VlogKey, S::Hype, "{PlaceWord} OF {Field} AT THE {Event}!"},
		{VlogKey, S::Hype, "THE CRAZIEST NIGHT OF POKER"},
		{VlogKey, S::Hype, "{Count} HANDS YOU HAVE TO SEE"},
		{VlogKey, S::Teaching, "{Count} spots from the {Event}, explained"},
		{VlogKey, S::Teaching, "A night at the {Event}: every big decision"},
		{VlogKey, S::Question, "How would you have played my night at the {Event}?"},
		{VlogKey, S::Question, "Which of these {Count} hands would you play differently?"},
		{VlogKey, S::Plain, "{Event} vlog: {Count} hands"},
		{VlogKey, S::Plain, "{Event}, {Weekday}: {Place} of {Field}"},
		// ---- a run (Recap)
		{RecapKey, S::Story, "{PlaceWord} at the {Event}: the whole run"},
		{RecapKey, S::Story, "how my {Event} run went"},
		{RecapKey, S::Story, "the {Event}, from the first hand to {PlaceWord}"},
		{RecapKey, S::Story, "#X how I won the {Event}"},
		{RecapKey, S::Hype, "{PlaceWord} IN THE {Event}! THE FULL RUN"},
		{RecapKey, S::Hype, "{Prize} RUN!"},
		{RecapKey, S::Teaching, "{Event} recap: every key hand, explained"},
		{RecapKey, S::Teaching, "Playing for {Prize}: my {Event} run, hand by hand"},
		{RecapKey, S::Question, "#N Where did my {Event} run go wrong?"},
		{RecapKey, S::Question, "What would you change? My {Event} run, {PlaceWord} of {Field}"},
		{RecapKey, S::Plain, "{Event} recap: {Place} of {Field}, {Prize}"},
		{RecapKey, S::Plain, "{Stakes} recap: {Count} hands, {Place} place"},
	};
	return All;
}

/** A life story's titles, one per style (§4.4: rent paid by a score, back after an eviction, a first title). */
struct StoryLine
{
	const char* Key;
	const char* Lines[5];
};
const std::vector<StoryLine>& StoryLines()
{
	static const std::vector<StoryLine> All = {
		{"rent-paid", {"I paid rent with poker money", "RENT PAID. WITH POKER.", "From broke to rent paid: the bankroll plan that worked", "Can you pay rent playing small tournaments?",
						  "Rent paid: a month of small stakes poker"}},
		{"seat-rcop-main", {"I won a seat to the main event", "I'M GOING TO THE MAIN EVENT!", "How a small satellite got me into the main event",
							   "Would you play the main event or sell the seat?", "Satellite win: a seat to the main event"}},
		{"bracelet-first", {"I won a bracelet", "BRACELET!!", "What it took to win my first bracelet", "Is an online bracelet a real bracelet?", "Championship bracelet win"}},
		{"ring-first", {"my first Grand Circuit ring", "I WON A RING!", "Winning a Grand Circuit ring: the hands that mattered", "Ring or bracelet: which would you rather win?",
						   "Grand Circuit ring win"}},
		{"mm-title", {"my name on the Micro Madness leaderboard", "SERIES CHAMPION!", "How I won a series title on a small bankroll", "Does a series title change anything?",
						 "Micro Madness title win"}},
		{"series-title", {"my name on a series leaderboard", "SERIES CHAMPION!", "How I won a series title on a small bankroll", "Does a series title change anything?",
							 "Series title win"}},
		{"back", {"I got evicted. I'm back", "BACK IN THE APARTMENT!", "Rebuilding after losing everything: what I changed", "How do you come back from an eviction?",
					 "Back home after the eviction"}},
	};
	return All;
}

/** A story's big words on its thumbnail. */
std::string StoryBig(const std::string& Key)
{
	if (Key == "rent-paid")
	{
		return "RENT PAID";
	}
	if (Key == "seat-rcop-main")
	{
		return "MAIN EVENT";
	}
	if (Key == "bracelet-first")
	{
		return "BRACELET";
	}
	if (Key == "ring-first")
	{
		return "RING";
	}
	if (Key == "mm-title" || Key == "series-title")
	{
		return "CHAMPION";
	}
	return Key.rfind("back", 0) == 0 ? "BACK HOME" : "STORY";
}

// ---------------------------------------------------------------- facts

std::string Lower(std::string S)
{
	for (char& Ch : S)
	{
		Ch = Ch >= 'A' && Ch <= 'Z' ? static_cast<char>(Ch - 'A' + 'a') : Ch;
	}
	return S;
}

std::string Upper(std::string S)
{
	for (char& Ch : S)
	{
		Ch = Ch >= 'a' && Ch <= 'z' ? static_cast<char>(Ch - 'a' + 'A') : Ch;
	}
	return S;
}

std::string RankWord(int Rank)
{
	return Lower(RankNames[std::clamp(Rank, 0, 12)]);
}

std::string WithArticle(const std::string& Word)
{
	return (!Word.empty() && (Word[0] == 'a' || Word[0] == 'e') ? "an " : "a ") + Word;
}

/** "aces", "ace-king", "seven-deuce". */
std::string HandWords(const std::vector<Card>& H)
{
	if (H.size() < 2)
	{
		return std::string();
	}
	const int A = std::max(RankOf(H[0]), RankOf(H[1]));
	const int B = std::min(RankOf(H[0]), RankOf(H[1]));
	return A == B ? Lower(RankPlurals[A]) : RankWord(A) + "-" + RankWord(B);
}

/** "AK", "QQ", "72". */
std::string HandShort(const std::vector<Card>& H)
{
	if (H.size() < 2)
	{
		return std::string();
	}
	const int A = std::max(RankOf(H[0]), RankOf(H[1]));
	const int B = std::min(RankOf(H[0]), RankOf(H[1]));
	return std::string(1, RankChars[A]) + RankChars[B];
}

std::string StreetWord(Street S)
{
	switch (S)
	{
	case Street::Flop: return "flop";
	case Street::Turn: return "turn";
	case Street::River:
	case Street::Showdown: return "river";
	case Street::Preflop: break;
	}
	return "preflop";
}

std::string PlaceWords(int Place)
{
	static const char* const Words[] = {"", "first", "second", "third", "fourth", "fifth", "sixth", "seventh", "eighth", "ninth", "tenth"};
	return Place >= 1 && Place <= 10 ? Words[Place] : Ordinal(Place);
}

std::string Dollars(Chips Cents)
{
	return Cents % 100 == 0 ? "$" + Grouped(Cents / 100) : Money(Cents);
}

/** The card that turned the hand (the player's view of a shown-down all-in): the first street from which the leader never changed. */
bool Turning(const std::vector<Card>& Mine, const std::vector<Card>& Theirs, const std::vector<Card>& Board, bool HeroWon, int& OnStreet, Card& Which)
{
	if (Mine.size() < 2 || Theirs.size() < 2 || Board.size() < 5)
	{
		return false;
	}
	for (int N = 5; N >= 3; --N)
	{
		const std::vector<Card> Part(Board.begin(), Board.begin() + N);
		const int Me = EvaluateHand(Mine, Part), Them = EvaluateHand(Theirs, Part);
		const bool AheadNow = HeroWon ? Me > Them : Them > Me;
		if (!AheadNow)
		{
			if (N == 5)
			{
				return false;
			}
			OnStreet = N + 1; // the card after this one
			Which = Board[static_cast<size_t>(N)];
			return true;
		}
	}
	return false; // ahead on every street with a board: no card turned it
}

struct Facts
{
	std::map<std::string, std::string> V;
	bool Won = false, Lost = false, Bubble = false, EventWon = false, Nothing = false;
};

Facts FactsFor(const Choice& Ch, const std::vector<Candidate>& P, const Candidate& L)
{
	Facts F;
	const recall::Record& R = L.Rec;
	const bool Event = video_detail::EventKind(L.K);
	F.Won = R.HeroNet > 0;
	F.Lost = R.HeroNet < 0;
	F.Bubble = L.Bubble;
	F.Nothing = !Event && video_detail::MadeLevel(R.Mine, R.Board) == 0;
	auto Set = [&F](const char* Key, const std::string& Value) {
		if (!Value.empty())
		{
			F.V[Key] = Value;
		}
	};
	// (A story's Event is its story key: the story's own lines name it.) Storytime's game has no name: {Event} always
	// follows "the" or "my" ("the hand that wrecked my back-room game"), and {Stakes} stands alone ("a back-room game").
	// A name that carries its own price ("Embercrest Nightly $120", as the card room's schedule writes it) loses it here:
	// {Stakes} puts the buy-in in front, and it's never said twice.
	std::string Named = L.Event;
	const std::string Price = " " + Dollars(L.BuyIn);
	if (L.BuyIn > 0 && Named.size() > Price.size() && Named.compare(Named.size() - Price.size(), Price.size(), Price) == 0)
	{
		Named.resize(Named.size() - Price.size());
	}
	const std::string EventName = L.K == Kind::Story ? std::string() : L.Storytime ? std::string("back-room game") : Named;
	Set("Event", EventName);
	if (!L.Storytime && L.BuyIn > 0)
	{
		Set("BuyIn", Dollars(L.BuyIn));
		Set("Stakes", EventName.empty() || EventName[0] == '$' ? EventName : Dollars(L.BuyIn) + " " + EventName);
	}
	else
	{
		Set("Stakes", L.Storytime && L.K != Kind::Story ? StorytimeEvent() : EventName);
	}
	if (!Event)
	{
		const double PotBb = video_detail::PotBbOf(L);
		if (PotBb >= 1.0)
		{
			Set("Bb", Grouped(static_cast<int64_t>(std::llround(PotBb))));
		}
		if (L.EquityAtAllIn >= 0.0)
		{
			Set("Eq", std::to_string(static_cast<int>(std::lround(L.EquityAtAllIn * 100.0))));
		}
		Set("Mine", HandShort(R.Mine));
		Set("MineWords", HandWords(R.Mine));
		if (R.Number > 0)
		{
			Set("HandNo", std::to_string(R.Number));
		}
		// The opponent's cards: only a hand that was turned up (N3).
		const std::vector<Card>* Theirs = nullptr;
		for (const auto& S : R.Shown)
		{
			if (S.second.size() >= 2 && (L.Opponent.empty() || S.first == L.Opponent || !Theirs))
			{
				Theirs = &S.second;
			}
		}
		if (Theirs)
		{
			Set("Theirs", HandShort(*Theirs));
			Set("TheirsWords", HandWords(*Theirs));
			int On = 0;
			Card Which = 0;
			if (Turning(R.Mine, *Theirs, R.Board, F.Won, On, Which))
			{
				const std::string Street = On == 5 ? "river" : On == 4 ? "turn" : "flop";
				if (L.K == Kind::BadBeat || F.Lost)
				{
					Set("Killer", WithArticle(RankWord(RankOf(Which))) + " on the " + Street);
					Set("KillStreet", Street);
				}
				else
				{
					Set("Saver", WithArticle(RankWord(RankOf(Which))) + " on the " + Street);
					Set("SaveStreet", Street);
				}
			}
		}
		// The street of the player's last say (a bluff's bet, a fold).
		Street Last = Street::Preflop;
		bool FacedShove = false;
		for (const recall::Line& X : R.Lines)
		{
			if (X.Who == "You" && X.What != "checks")
			{
				Last = X.On;
			}
			FacedShove = FacedShove || (X.Who != "You" && X.What == "all in" && X.On == Last);
		}
		if (Last != Street::Preflop)
		{
			Set("Street", StreetWord(Last)); // "the river bluff": a street after the flop only
		}
		Set("Spot", L.Bubble ? "on the bubble" : FacedShove ? "to a shove" : Last == Street::Preflop ? "preflop" : "on the " + StreetWord(Last));
		if (L.Rival && !L.Storytime && !L.Opponent.empty())
		{
			Set("Opp", L.Opponent);
		}
	}
	// The event's result, when one of the picks is it.
	for (const Candidate& X : P)
	{
		if (video_detail::EventKind(X.K) && X.PlaceSoFar > 0)
		{
			Set("Place", Ordinal(X.PlaceSoFar));
			Set("PlaceWord", PlaceWords(X.PlaceSoFar));
			if (X.Rec.Pot > 0)
			{
				Set("Prize", Dollars(X.Rec.Pot));
			}
			F.EventWon = F.EventWon || X.K == Kind::Win;
		}
		if (X.Field > 1 && (video_detail::EventKind(X.K) || !F.V.count("Field")))
		{
			Set("Field", Grouped(X.Field));
		}
		if (!video_detail::EventKind(X.K) && X.Rec.HeroNet < 0 && (X.K == Kind::BadBeat || X.K == Kind::Cooler || X.K == Kind::Bubble) && Ch.F != Format::Breakdown &&
			Ch.F != Format::Short)
		{
			const std::vector<Card>* Theirs = X.Rec.Shown.empty() ? nullptr : &X.Rec.Shown.front().second;
			int On = 0;
			Card Which = 0;
			if (Theirs && Turning(X.Rec.Mine, *Theirs, X.Rec.Board, false, On, Which))
			{
				Set("Ender", std::string("the ") + (On == 5 ? "river" : On == 4 ? "turn" : "flop") + " that ended it");
			}
		}
	}
	// "{Count} hands": the hands, not the run's result that leads a Recap (or sits in a Vlog).
	const auto Hands = std::count_if(P.begin(), P.end(), [](const Candidate& X) { return !video_detail::EventKind(X.K) && X.K != Kind::Story; });
	if (Hands > 1)
	{
		Set("Count", std::to_string(Hands));
	}
	Set("Weekday", net::WeekdayName(net::DayOf(L.At - 10.0 * 60.0), true));
	return F;
}

/** The template filled from the facts, or "" when it needs a fact the moment doesn't have. */
std::string Fill(const char* Text, const Facts& F)
{
	std::string T(Text);
	if (T.size() > 3 && T[0] == '#' && T[2] == ' ')
	{
		const char C = T[1];
		if ((C == 'W' && !F.Won) || (C == 'L' && !F.Lost) || (C == 'B' && !F.Bubble) || (C == 'X' && !F.EventWon) || (C == 'N' && F.EventWon) ||
			(C == '0' && !F.Nothing))
		{
			return std::string();
		}
		T = T.substr(3);
	}
	std::string Out;
	size_t I = 0;
	while (I < T.size())
	{
		const size_t Open = T.find('{', I);
		if (Open == std::string::npos)
		{
			Out.append(T, I, std::string::npos);
			break;
		}
		const size_t Close = T.find('}', Open);
		const auto It = Close == std::string::npos ? F.V.end() : F.V.find(T.substr(Open + 1, Close - Open - 1));
		if (It == F.V.end())
		{
			return std::string();
		}
		Out.append(T, I, Open - I);
		Out += It->second;
		I = Close + 1;
	}
	return Out;
}

std::string Styled(std::string Title, TitleStyle T)
{
	if (T == TitleStyle::Hype)
	{
		return Upper(Title);
	}
	if (T == TitleStyle::Story)
	{
		return Title; // a story's titles are lower case, as people write them
	}
	// A capital to start, and after a question or a full stop ("Easy call or hero call? Aces for a knockout").
	for (size_t I = 0; I < Title.size(); ++I)
	{
		const bool Starts = I == 0 || (I >= 2 && Title[I - 1] == ' ' && (Title[I - 2] == '?' || Title[I - 2] == '.'));
		if (Starts && Title[I] >= 'a' && Title[I] <= 'z')
		{
			Title[I] = static_cast<char>(Title[I] - 'a' + 'A');
		}
	}
	return Title;
}

const char* KindPhrase(Kind K)
{
	switch (K)
	{
	case Kind::BadBeat: return "a bad beat";
	case Kind::Suckout: return "a suckout";
	case Kind::Cooler: return "a cooler";
	case Kind::Bluff: return "a bluff";
	case Kind::BluffCaught: return "a bluff gone wrong";
	case Kind::HeroCall: return "a hero call";
	case Kind::HeroFold: return "a big fold";
	case Kind::BigPot: return "a big pot";
	case Kind::Knockout: return "a knockout";
	case Kind::Bubble: return "a bubble hand";
	case Kind::DeepRun: return "a deep run";
	case Kind::FinalTable: return "a final table";
	case Kind::Win: return "a win";
	case Kind::Rival: return "a rivalry";
	case Kind::Story: return "a story";
	case Kind::StreamMoment:
	case Kind::Count: break;
	}
	return "a stream moment";
}

// ---------------------------------------------------------------- the other lines

std::string ReviewSubject()
{
	return "Your channel's under review.";
}

std::string ReviewBody(double Subs, double Hours)
{
	return "Your channel just cleared both bars for the Rerun Partner Program: " + Grouped(static_cast<int64_t>(std::floor(Subs))) + " subscribers and " +
		Grouped(static_cast<int64_t>(std::floor(Hours))) +
		" watch hours in the last twelve months. Someone here is looking it over now, which usually takes about a week. Nothing to do but keep posting.";
}

std::string WelcomeSubject()
{
	return "Welcome to the Rerun Partner Program.";
}

std::string WelcomeBody()
{
	return "You're in. From today ads run on your videos, and you keep your share of what they earn. It adds up through the month and pays out on the 21st once your "
		   "balance reaches $100; anything less rolls over. Views from before today don't count, so keep the uploads coming.";
}

std::string StatementSubject(const std::string& Month, bool Paid)
{
	return Paid ? "Your Rerun payout for " + Month + "." : "Your Rerun statement for " + Month + ".";
}

std::string StatementBody(const std::string& Month, Chips PaidCents, Chips BalanceCents, bool Paid)
{
	(void)Month;
	if (Paid)
	{
		return "We've sent " + Money(PaidCents) + " to your bank account. It should already be there. Thanks for making videos people want to watch.";
	}
	return "Your balance is " + Money(BalanceCents) + ", under the $100 minimum, so it rolls into next month's. Nothing's lost; it just waits.";
}

std::string BellViews(const std::string& Title, long long Views)
{
	return Grouped(Views) + " views on \xE2\x80\x9C" + Title + "\xE2\x80\x9D.";
}

std::string BellSubs(long long Subs)
{
	return Grouped(Subs) + " subscribers. People are sticking around.";
}

std::string BellReview()
{
	return "Your channel's under review for the Partner Program.";
}

std::string BellPartner()
{
	return "You're a Rerun Partner. Ads are on.";
}

std::string MonthName(int Month)
{
	static const char* const Names[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
	return Names[std::clamp(Month, 1, 12) - 1];
}

std::string PayoutLabel(const std::string& Month)
{
	return "Rerun payout: " + Month;
}

std::string MarkPayoutLabel()
{
	return "Rerun payout";
}

std::string MarkReviewLabel()
{
	return "Rerun review";
}

std::string GrantTitle()
{
	return "Granted views (dev)";
}

std::string StorytimeEvent()
{
	return "a back-room game";
}

std::string WhyNot(int Reason)
{
	switch (Reason)
	{
	case 1: return "That moment's gone stale. Pick another.";
	case 2: return "You picked the same moment twice.";
	case 3: return "A Short is one moment.";
	case 4: return "A Breakdown is one hand.";
	case 5: return "A Vlog takes three to eight moments from one night.";
	case 6: return "Those moments aren't all from the same night.";
	case 7: return "A Recap needs a deep run, a final table or a win, plus two to six hands from that event.";
	case 8: return "Those hands aren't all from the same event.";
	case 9: return "A story stands on its own. Post it as a Short or a Breakdown.";
	default: break;
	}
	return "Pick something to post first.";
}

// The session's lines (SessionVideo.cpp declares these; PublishWhy and Deleted are public, in Video.h).
std::string EditLabel(Format F)
{
	switch (F)
	{
	case Format::Short: return "Cutting a Short";
	case Format::Vlog: return "Editing the night into a Vlog";
	case Format::Recap: return "Putting the Recap together";
	case Format::Breakdown: break;
	}
	return "Editing a Breakdown";
}

std::string UploadedTitle()
{
	return "Up on Rerun";
}

std::string UploadedBody(const std::string& Title)
{
	return "\xE2\x80\x9C" + Title + "\xE2\x80\x9D is live. The views come in over the next few days.";
}

std::string EditFailedTitle()
{
	return "The edit fell apart";
}

std::string DeeStorytime()
{
	return "Saw the video. No names. Good.";
}

std::string NoSuchVideo()
{
	return "That video's already gone.";
}
} // namespace video_copy

// ================================================================== the public words

std::string Title(const Choice& Ch, const std::vector<Candidate>& Cands, uint32_t Seed)
{
	using namespace video_copy;
	std::vector<Candidate> P = Picked(Ch, Cands);
	if (P.empty())
	{
		return std::string();
	}
	const Candidate* Lead = &P.front();
	for (const Candidate& X : P)
	{
		if (Ch.F != Format::Recap && (video_detail::EventKind(X.K) ? 10.0 : 0.0) + X.Drama > (video_detail::EventKind(Lead->K) ? 10.0 : 0.0) + Lead->Drama)
		{
			Lead = &X;
		}
	}
	if (Seed == 0)
	{
		// The upload's own seed: the same choice always gets the same title (Studio's preview is the video's title).
		std::string Key = std::to_string(static_cast<int>(Ch.F)) + "|" + std::to_string(static_cast<int>(Ch.T));
		for (const std::string& Id : Ch.Candidates)
		{
			Key += "|" + Id;
		}
		Seed = Fnv1a(Key);
	}
	const Facts F = FactsFor(Ch, P, *Lead);
	const bool Multi = Ch.F == Format::Vlog || Ch.F == Format::Recap;
	std::vector<std::string> Options;
	if (Lead->K == Kind::Story && !Multi)
	{
		for (const StoryLine& S : StoryLines())
		{
			if (Lead->Event == S.Key || Lead->Event.rfind(std::string(S.Key) + ":", 0) == 0) // "back:2": the second eviction's
			{
				Options.push_back(S.Lines[static_cast<int>(Ch.T)]);
			}
		}
	}
	const int For = Ch.F == Format::Vlog ? VlogKey : Ch.F == Format::Recap ? RecapKey : static_cast<int>(Lead->K);
	for (const Template& T : Templates())
	{
		if (T.For == For && T.T == Ch.T)
		{
			const std::string Filled = Fill(T.Text, F);
			if (!Filled.empty())
			{
				Options.push_back(Filled);
			}
		}
	}
	if (Options.empty())
	{
		// Every style has a template that needs only the event (or nothing); this is the last word if a fact is missing.
		const std::string Event = F.V.count("Stakes") ? F.V.at("Stakes") : std::string("the tables");
		Options.push_back(std::string(KindPhrase(Lead->K)) + " from " + Event);
	}
	return Styled(Options[Seed % Options.size()], Ch.T);
}

Thumb ThumbFor(const Choice& Ch, const std::vector<Candidate>& Open)
{
	using namespace video_copy;
	Thumb T;
	T.Fr = Ch.Fr;
	const std::vector<Candidate> P = Picked(Ch, Open);
	if (P.empty())
	{
		return T;
	}
	const Candidate* Lead = &P.front();
	for (const Candidate& X : P)
	{
		if (Ch.F != Format::Recap && (video_detail::EventKind(X.K) ? 10.0 : 0.0) + X.Drama > (video_detail::EventKind(Lead->K) ? 10.0 : 0.0) + Lead->Drama)
		{
			Lead = &X;
		}
	}
	const Candidate& L = *Lead;
	const Facts F = FactsFor(Ch, P, L);
	// The face: the stream's at the moment, else what the hand would do to anyone (kast::Mood).
	int Mood = 0;
	switch (L.K)
	{
	case Kind::BadBeat:
	case Kind::Cooler:
	case Kind::BluffCaught: Mood = 3; break; // Shocked
	case Kind::Suckout: Mood = 5; break;     // Laugh
	case Kind::Bluff:
	case Kind::HeroCall: Mood = 1; break; // Happy
	case Kind::BigPot:
	case Kind::Knockout:
	case Kind::DeepRun:
	case Kind::FinalTable:
	case Kind::Win: Mood = 2; break; // Hyped
	case Kind::Rival: Mood = 4; break; // Tilted
	default: Mood = 0; break;
	}
	if (L.Stream.Viewers > 0)
	{
		Mood = std::clamp(L.Stream.Face, 0, 5);
	}
	const bool Portrait = L.Source == 1 || L.Source == 2;
	T.Mood = Portrait ? 10 + Mood : Mood;
	// The cards: the player's, the opponent's only if turned up, then the board.
	auto Cards = [](const std::vector<Card>& C) {
		std::string S;
		for (const Card X : C)
		{
			S += CardToString(X);
		}
		return S;
	};
	if (!video_detail::EventKind(L.K) && L.Rec.Mine.size() >= 2)
	{
		std::string Theirs;
		for (const auto& S : L.Rec.Shown)
		{
			if (S.second.size() >= 2 && (S.first == L.Opponent || Theirs.empty()))
			{
				Theirs = Cards(S.second);
			}
		}
		T.Cards = Cards(L.Rec.Mine) + "/" + Theirs + "/" + Cards(L.Rec.Board);
	}
	// The big words.
	const auto Has = [&F](const char* K) { return F.V.count(K) > 0; };
	bool AllIn = false;
	for (const recall::Line& X : L.Rec.Lines)
	{
		AllIn = AllIn || (X.Who == "You" && X.What == "all in");
	}
	switch (Ch.Fr)
	{
	case Frame::Reaction:
		T.BigText = L.K == Kind::BadBeat && Has("Eq") ? F.V.at("Eq") + "%" : L.K == Kind::Story ? StoryBig(L.Event) : Upper(KindLabel(L.K));
		break;
	case Frame::Cards: T.BigText = AllIn ? "ALL IN" : Upper(KindLabel(L.K)); break;
	case Frame::Number:
		T.BigText = Has("Prize") && video_detail::EventKind(L.K) ? F.V.at("Prize") : Has("Bb") ? F.V.at("Bb") + " BB" : Upper(KindLabel(L.K));
		break;
	case Frame::Versus: T.BigText = "VS"; break;
	case Frame::Trophy:
		T.BigText = L.K == Kind::Win ? "WINNER" : Has("Place") && Has("Field") ? Upper(F.V.at("Place") + " of " + F.V.at("Field")) : Upper(KindLabel(L.K));
		break;
	}
	static const uint32_t Accents[6] = {0xFF7A59, 0x7CC6FE, 0xF2A33A, 0xFFD166, 0xFF9F1C, 0xFFF4E0};
	T.Accent = Accents[std::clamp(video_detail::GroupOf(L.K), 0, 5)];
	T.Opponent = L.Storytime ? std::string() : L.Opponent;
	T.Site = L.Site;
	return T;
}

const char* KindLabel(Kind K)
{
	switch (K)
	{
	case Kind::BadBeat: return "BAD BEAT";
	case Kind::Suckout: return "SUCKOUT";
	case Kind::Cooler: return "COOLER";
	case Kind::Bluff: return "BLUFF";
	case Kind::BluffCaught: return "BLUFF CAUGHT";
	case Kind::HeroCall: return "HERO CALL";
	case Kind::HeroFold: return "HERO FOLD";
	case Kind::BigPot: return "BIG POT";
	case Kind::Knockout: return "KNOCKOUT";
	case Kind::Bubble: return "BUBBLE";
	case Kind::DeepRun: return "DEEP RUN";
	case Kind::FinalTable: return "FINAL TABLE";
	case Kind::Win: return "WIN";
	case Kind::Rival: return "RIVAL";
	case Kind::Story: return "STORY";
	case Kind::StreamMoment: return "STREAM";
	case Kind::Count: break;
	}
	return "MOMENT";
}

const char* FormatName(Format F)
{
	switch (F)
	{
	case Format::Short: return "Short";
	case Format::Vlog: return "Vlog";
	case Format::Recap: return "Recap";
	case Format::Breakdown: break;
	}
	return "Breakdown";
}

const char* TitleStyleName(TitleStyle T)
{
	switch (T)
	{
	case TitleStyle::Hype: return "Hype";
	case TitleStyle::Teaching: return "Teaching";
	case TitleStyle::Question: return "Question";
	case TitleStyle::Plain: return "Plain";
	case TitleStyle::Story: break;
	}
	return "Story";
}

const char* FrameName(Frame F)
{
	switch (F)
	{
	case Frame::Cards: return "Cards";
	case Frame::Number: return "Number";
	case Frame::Versus: return "Versus";
	case Frame::Trophy: return "Trophy";
	case Frame::Reaction: break;
	}
	return "Reaction";
}

std::string PublishWhy(int Reason)
{
	switch (Reason)
	{
	case 1: return "You're busy.";
	case 2: return "Close your tables first. Editing needs your whole attention.";
	case 3: return "End the stream first.";
	case 4: return "Too tired to edit. Get some sleep first.";
	default: break;
	}
	return "Rerun can't open that right now.";
}

std::string Deleted()
{
	return "Taken down. The subscribers it brought in stay.";
}

std::vector<std::string> Advice(const Choice& Ch, const std::vector<Candidate>& Open)
{
	return Advice(Ch, Open, false);
}

std::vector<std::string> Advice(const Choice& Ch, const std::vector<Candidate>& Open, bool AdsOn)
{
	std::vector<std::string> Out;
	const std::vector<Candidate> P = Picked(Ch, Open);
	switch (Ch.F)
	{
	case Format::Short: Out.push_back("Shorts get seen fast and forgotten fast, and they don't count toward watch hours."); break;
	case Format::Breakdown: Out.push_back("A Breakdown walks through one hand, and it counts toward watch hours."); break;
	case Format::Vlog:
		// Ads are no reason to pick a format until there are any (the gate, then the review).
		Out.push_back(AdsOn ? "A Vlog tells the whole night. Longer videos carry more ads." : "A Vlog tells the whole night, and it counts toward watch hours.");
		break;
	case Format::Recap: Out.push_back("A Recap follows one run from the first hand to the last."); break;
	}
	switch (Ch.T)
	{
	case TitleStyle::Story: Out.push_back("Story titles make people care, and the ones who care stay."); break;
	case TitleStyle::Hype: Out.push_back("Hype titles get the click. On a small pot, people leave early."); break;
	case TitleStyle::Teaching: Out.push_back("Teaching titles keep people watching. On a decision, people keep finding it for months."); break;
	case TitleStyle::Question: Out.push_back("A question pulls people in to argue about it."); break;
	case TitleStyle::Plain: Out.push_back("Plain titles tell people exactly what they're getting."); break;
	}
	if (!P.empty())
	{
		const Kind K = P.front().K;
		const int G = video_detail::GroupOf(K);
		static const double Fit[5][6] = {
			{1.3, 1.1, 1.1, 1.1, 1.0, 1.2}, {1.2, 1.3, 1.0, 0.8, 0.9, 0.7}, {0.9, 0.8, 1.3, 1.2, 0.9, 1.0}, {1.0, 1.0, 1.1, 0.9, 1.4, 0.8}, {0.7, 0.7, 0.8, 1.4, 0.9, 1.0},
		};
		const double Here = Fit[std::clamp(static_cast<int>(Ch.Fr), 0, 4)][std::clamp(G, 0, 5)];
		const std::string Frame = FrameName(Ch.Fr);
		Out.push_back(Here >= 1.2 ? "The " + Frame + " frame suits " + video_copy::KindPhrase(K) + "."
					  : Here <= 0.8 ? "The " + Frame + " frame undersells " + video_copy::KindPhrase(K) + "."
									: "The " + Frame + " frame works here.");
		switch (K)
		{
		case Kind::BadBeat: Out.push_back("Bad beats travel."); break;
		case Kind::Bluff: Out.push_back("Everybody wants to see the cards nobody saw."); break;
		case Kind::Win: Out.push_back("A win is the one people share."); break;
		case Kind::Rival: Out.push_back("People love a grudge."); break;
		default: break;
		}
		if (P.front().Storytime)
		{
			Out.push_back("Storytime: nobody from the game is named, and no face but yours.");
		}
	}
	return Out;
}

std::vector<std::string> Inventory()
{
	using namespace video_copy;
	std::vector<std::string> Out;
	for (const Template& T : Templates())
	{
		std::string Text = T.Text;
		if (Text.size() > 3 && Text[0] == '#')
		{
			Text = Text.substr(3);
		}
		Out.push_back(std::string("title\t") + Text);
	}
	for (const StoryLine& S : StoryLines())
	{
		for (const char* L : S.Lines)
		{
			Out.push_back(std::string("title\t") + L);
		}
	}
	for (int K = 0; K < static_cast<int>(Kind::Count); ++K)
	{
		Out.push_back(std::string("title\t") + KindPhrase(static_cast<Kind>(K)) + " from {Stakes}");
	}
	Out.push_back("subject\t" + ReviewSubject());
	Out.push_back("body\t" + ReviewBody(1012.0, 4210.0));
	Out.push_back("subject\t" + WelcomeSubject());
	Out.push_back("body\t" + WelcomeBody());
	for (const bool Paid : {true, false})
	{
		Out.push_back("subject\t" + StatementSubject("April", Paid));
		Out.push_back("body\t" + StatementBody("April", 14620, 5810, Paid));
	}
	Out.push_back("line\t" + BellViews("the hand that wrecked my Night Owl Turbo", 1000));
	Out.push_back("line\t" + BellSubs(100));
	Out.push_back("line\t" + BellReview());
	Out.push_back("line\t" + BellPartner());
	for (int R = 0; R <= 9; ++R)
	{
		Out.push_back("line\t" + WhyNot(R));
	}
	for (int R = 0; R <= 4; ++R)
	{
		Out.push_back("line\t" + PublishWhy(R));
	}
	Out.push_back("line\t" + UploadedBody("the hand that wrecked my Night Owl Turbo"));
	Out.push_back("line\t" + DeeStorytime());
	Out.push_back("line\t" + Deleted());
	Out.push_back("line\t" + NoSuchVideo());
	Out.push_back("label\t" + UploadedTitle());
	Out.push_back("label\t" + EditFailedTitle());
	Out.push_back("label\t" + PayoutLabel("April"));
	Out.push_back("label\t" + MarkPayoutLabel());
	Out.push_back("label\t" + MarkReviewLabel());
	for (int F = 0; F < 4; ++F)
	{
		Out.push_back("label\t" + EditLabel(static_cast<Format>(F)));
		Out.push_back(std::string("label\t") + FormatName(static_cast<Format>(F)));
	}
	for (int T = 0; T < 5; ++T)
	{
		Out.push_back(std::string("label\t") + TitleStyleName(static_cast<TitleStyle>(T)));
		Out.push_back(std::string("label\t") + FrameName(static_cast<Frame>(T)));
	}
	for (int K = 0; K < static_cast<int>(Kind::Count); ++K)
	{
		Out.push_back(std::string("label\t") + KindLabel(static_cast<Kind>(K)));
	}
	// Studio's lines, every format, style and frame over a bad beat and a storytime bluff.
	Candidate Beat;
	Beat.Id = "rl:sample:1";
	Beat.K = Kind::BadBeat;
	Candidate Dees = Beat;
	Dees.Id = "dee:9:4";
	Dees.K = Kind::Bluff;
	Dees.Storytime = true;
	for (const Candidate* C : {&Beat, &Dees})
	{
		for (int F = 0; F < 2; ++F)
		{
			for (int T = 0; T < 5; ++T)
			{
				for (int Fr = 0; Fr < 5; ++Fr)
				{
					Choice Ch;
					Ch.Candidates = {C->Id};
					Ch.F = static_cast<Format>(F);
					Ch.T = static_cast<TitleStyle>(T);
					Ch.Fr = static_cast<Frame>(Fr);
					for (const std::string& L : Advice(Ch, {*C}))
					{
						if (std::find(Out.begin(), Out.end(), "line\t" + L) == Out.end())
						{
							Out.push_back("line\t" + L);
						}
					}
				}
			}
		}
	}
	for (int F = 2; F < 4; ++F)
	{
		for (const bool AdsOn : {false, true})
		{
			Choice Ch;
			Ch.F = static_cast<Format>(F);
			for (const std::string& L : Advice(Ch, {}, AdsOn))
			{
				if (std::find(Out.begin(), Out.end(), "line\t" + L) == Out.end())
				{
					Out.push_back("line\t" + L);
				}
			}
		}
	}
	return Out;
}
} // namespace video
} // namespace ss
