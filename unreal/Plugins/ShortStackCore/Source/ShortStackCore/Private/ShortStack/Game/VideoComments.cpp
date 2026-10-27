// Update 3 (SPEC §6.4, §7.3): Rerun 2.0's comments, core-videocontent's file: 20 archetypes, procedural and honest (every
// number a fact), keyed from the video's Fate. Nothing is saved: comment i of a video is the same comment on every read.
//
// Comment i comes from Fnv1a(Fate:comment:<i>): an archetype weighted by the video's kind, style, honesty (its promise over
// its payoff), ad load, retention and series; a template whose slots its facts fill (the title's numbers, the thumbnail's
// cards, the arithmetic of them); a commenter (a procedural handle with no digits, gh0stfold's own spelling, or the named
// player who was in the hand, online and never in storytime); likes on a power law in i (at most 3% of the video's).
// Never "AI", a bot or autoplay; no real brand or creator; no name in storytime; no unshown card; no number that isn't a
// fact; no link. At most one Results-oriented, Nit or gh0stfold comment in any ten in a row (they sit only at i % 10 == 9).
#include "ShortStack/Game/VideoContent.h"

#include "VideoContentPriv.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Common.h"
#include "ShortStack/Evaluator.h"
#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Video.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace ss
{
namespace video
{
namespace vcontent
{
namespace
{
/** The archetypes, in SPEC §6.4's order (Comment::Archetype). */
enum VmArch : uint8_t
{
	VmSympathizer = 0,
	VmMath,
	VmResults, // picky
	VmCoach,
	VmTimestamp,
	VmEarly,
	VmJoker,
	VmWasInHand,
	VmKastRegular,
	VmQuestion,
	VmHandReader,
	VmNit, // picky
	VmSuperfan,
	VmGhost, // picky
	VmLocal,
	VmStorySafe,
	VmClickbait,
	VmAds,
	VmSeries,
	VmSpam,
	VmCount
};

bool VmPicky(int A)
{
	return A == VmResults || A == VmNit || A == VmGhost;
}

/** Each archetype's templates ({Slot}s from the video's facts; a template is skipped when a slot is missing). */
const std::vector<std::vector<const char*>>& VmTemplates()
{
	static const std::vector<std::vector<const char*>> All = {
		// Sympathizer: beats, coolers
		{"{Eq}% and {Killer} still finds a way. Brutal.", "that {KillStreet} card is just cruel", "nothing you could do there. nothing.", "I felt that one from here",
			"{MineWords} never hold when it matters, I swear", "watching this hurt more than my own beats", "keep your head up, you played it right",
			"that's poker. still hurts to watch.", "the worst part is you did everything right", "{Bb} big blinds gone on one card. rough.",
			"I'd have closed the laptop after that", "some nights the deck just isn't on your side", "{Killer}. of course it was.",
			"sorry man, that one's going to sting for a week", "you got it in good. that's all anyone can do"},
		// The math one: beats, suckouts, lessons
		{"{Eq}% means you lose this {Lose} times in 100. it's not a bad beat, it's Tuesday", "people forget {Lose}% happens a lot",
			"you were {Eq}% to win. that's a great spot to get it in, every time", "ran the numbers: {Lose} in 100. it was always coming",
			"the money went in good. that's all you control", "{Eq}% isn't 100%. never will be", "over a thousand of these you print money. this was one of the {Lose}%",
			"at {Eq}% you want this spot every single night", "the math says do it again tomorrow", "variance doesn't care how good the spot was",
			"{Bb} big blinds at {Eq}% is a great trade, even when it loses", "if you never lose at {Eq}% you're not getting it in enough",
			"it's a {Lose}% shot. those land more often than it feels", "good spot, bad result. the only kind of beat there is", "nobody talks about the {Eq}% you won"},
		// Results-oriented (picky)
		{"just don't get it in lol", "should've folded", "why would you call there", "this is why I don't play {MineWords}", "skill issue",
			"easy fold for me", "you had to know they had it", "results say fold", "I'd have just folded and gone to bed", "never get it in early, that's my rule",
			"fold and live to fight another day", "you play too many hands", "I would never", "that's what you get for getting it in", "just play tighter"},
		// The coach: lessons, bluffs
		{"the {Street} sizing is what got you paid, not the cards", "nice line. the turn check sets the whole hand up", "I'd bet smaller on the {Street}, but the idea is right",
			"the decision before the all in is the one to study", "you gave them room to bluff. that's the whole hand", "good spot to slow down",
			"your preflop sizing gave away a lot there", "the real mistake was earlier in the hand", "this is a great hand to review with a solver",
			"the {Street} is where most players get this wrong", "position did half the work in this hand", "think about what they can have on the {Street}",
			"nice discipline. most players can't make that play", "you kept their bluffs in. that's the skill", "I'd study the flop decision more than the all in"},
		// Timestamp: high retention
		{"{Time} the moment you knew", "{Time} that face", "{Time} is the whole video", "{Time} best part", "{Time} I gasped",
			"{Time} rewatched this so many times", "{Time} the silence", "{Time} you can see it hit you", "{Time} here we go", "{Time} the turn of the night",
			"{Time} perfect reaction", "{Time} that's when it got real", "{Time} chills", "{Time} oh no", "{Time} the best decision in the video"},
		// Early: every video
		{"here before this hits {Milestone} views", "early. this one's going places", "first time I've been early to one of these",
			"early gang", "caught this one right after it went up", "here before everyone else finds this channel", "early and it's a good one",
			"notification squad", "I always click these the second they go up", "early again, love these", "the channel's about to blow up, I can feel it",
			"here before {Milestone}", "early and not disappointed", "just got the notification, let's go", "early! this is going to do numbers"},
		// The joker: beats
		{"the dealer owes you {Bb} big blinds and an apology", "the deck said no", "the {KillStreet} card is a menace", "somebody check that deck",
			"{Killer} has a personal problem with you", "this is why I play chess now", "the poker gods are watching this channel",
			"that {KillStreet} card should be in jail", "{MineWords} are just pretty cards to look at", "the deck owes you a drink", "I'd frame that {KillStreet} card and throw darts at it",
			"the dealer is in on it", "new rule: no more {KillStreet}s", "the deck heard you were feeling confident", "my condolences to your stack"},
		// Was in the hand: signed by the named player (online, never storytime)
		// ("#W " the player won the pot, so its loser speaks; "#L " the player lost it, so its winner does.)
		{"nice hand. I'd do it again.", "#W I knew you had it. still had to look", "#L gg. you'll get me next time", "#W that one stung on my end too",
			"funny seeing this hand from the other side", "#W I had to call there. no regrets", "good game, see you at the tables",
			"I still think about this hand", "#W well played. I hate it, but well played", "#W you got me fair and square", "#W I wanted that pot so badly",
			"rematch tomorrow night?", "#W this video is painful for me to watch", "I can confirm it was as tense as it looks", "respect. tough hand for both of us",
			"#L gg. I got lucky and I know it", "#L sorry. I'd be posting this one too"},
		// Kast regular: stream moments
		{"caught this live, chat lost its mind", "was there for this on Kast. the reaction was better live", "the stream for this one was unreal",
			"still can't believe I saw this live", "chat went silent for a full minute", "I was in chat for this one", "best moment on the stream all week",
			"watching it again here is just as good", "live it felt even worse", "the clip doesn't do the live reaction justice",
			"I remember everyone spamming in chat", "Kast regular here, this was a classic", "you should stream more nights like this",
			"saw this live and still watched the whole video", "chat called it before you did"},
		// Question asker: Question titles, lessons
		{"what's your stop-loss on a night like this?", "how do you stay calm after one like that?", "do you play the same tournaments every night?",
			"what's your bankroll rule for moving up?", "would you fold {MineWords} there next time?", "how many tables do you usually play?",
			"what made you think they were strong?", "do you review every hand like this after a session?", "how long have you been playing tournaments?",
			"what would you do differently on the {Street}?", "do you use a tracker or just memory?", "how do you pick which tournaments to play?",
			"is there a hand like this you'd play differently now?", "what's the hardest spot for you in tournaments?", "when do you decide to call it a night?"},
		// Hand reader: bluffs, hero calls, quizzes
		{"I'm calling there every time. that line screams bluff", "the {Street} bet doesn't make sense for a strong hand",
			"they never have it when they bet like that on the {Street}", "that sizing looks like a bluff to me", "{MineWords} is a fine call at that price",
			"the turn check is the giveaway", "they'd raise the flop with a real hand", "I put them on a missed draw from the flop",
			"tough spot, but the story doesn't add up for them", "you have the right blockers to call there", "that's a classic overbet bluff",
			"easy call once you count their value hands", "they're polarised there, and you beat half of it", "great read. the timing gave it away",
			"I'd fold there, but I respect the call"},
		// The nit (picky)
		{"too loose. that's a fold from early position", "I'd never put {Bb} big blinds in with that", "fold preflop and you never have this problem",
			"way too much action for a tournament this size", "that's a fold for me every time", "too many chips with too little hand",
			"playing {MineWords} like that will cost you", "tight is right in these fields", "you don't need to play every pot", "I'd have folded on the flop",
			"wait for a better spot", "that's a lot of risk for a small edge", "nitting up would fix this", "patience wins tournaments", "fold and wait"},
		// Superfan: every video
		{"these are the best poker videos on here", "every upload makes my week", "the editing keeps getting better", "been here since the early ones, love this channel",
			"this channel deserves way more views", "the best part of my week is a new upload", "I watch these while I play my own sessions",
			"you make poker look fun even when it hurts", "honest videos like this are rare", "the storytelling on this channel is so good",
			"I've learned more here than anywhere else", "never miss an upload", "these just keep getting better", "love how real these are",
			"you're my favourite poker creator"},
		// gh0stfold and his sock puppet (picky)
		{"lucky.", "same result next time.", "you got there. once.", "enjoy it while it lasts.", "I remember this hand differently.", "see you at the tables.",
			"one pot doesn't make a rivalry.", "cute.", "I let you have that one.", "watch the next one.", "noted.", "I'll be at your table tonight.",
			"keep posting. I keep watching.", "you're welcome for the content.", "the real story is the hands you didn't post."},
		// The local: live videos (only if you went)
		{"saw you at the Embercrest", "the Embercrest card room looks exactly like this", "I was at the table next to you that night",
			"the bus ride home after one like that is the worst", "the Embercrest on a busy night is something else", "I play there too. the regulars are tough",
			"that room gets so loud when an all in happens", "next time say hi if you see me there", "the Embercrest coffee is the real MVP",
			"love seeing live poker on here", "the dealers there are great", "I recognise that table", "that card room has the best atmosphere",
			"live tournaments hit different", "the Embercrest crowd would have loved that hand"},
		// Storytime-safe: storytime
		{"the regular across from you sounds like a nightmare", "every back-room game has that one player", "I'd play in that game just for the stories",
			"no names is the right call", "the back room always has the best hands", "home games are a different sport", "that game sounds wild",
			"the stories from that room are the best videos", "I'd love to sit in that game for one night", "every private game has a character like that",
			"keep the back-room stories coming", "I can picture the whole table", "nobody plays like the back-room regulars", "that's why I love home games",
			"the best poker stories never have names in them"},
		// Clickbait callout: the title promised more than the video
		{"the title promised more than this", "the thumbnail made it look a lot bigger", "clickbait, but I watched the whole thing", "that title was doing a lot of work",
			"I came for the title and stayed anyway", "not quite what the title said", "the thumbnail is a bit dramatic", "the title oversold this one",
			"good video, the title is a stretch", "the thumbnail had me expecting something else", "honest titles next time please", "a bit of a bait title, but fine",
			"I'd have clicked anyway, the title didn't need to lie", "the hand was good, the title was better", "this wasn't the hand the title told me about"},
		// Ad complaint: heavy ads
		{"so many ad breaks in this one", "the ads are getting out of hand", "an ad every couple of minutes, come on", "great video, way too many ads",
			"the ad breaks keep killing the tension", "too many ads for a video this short", "I get it, but this many ads hurts", "ad, hand, ad, hand",
			"the ads ruined the river for me", "please fewer ads next time", "the content is great, the ad load isn't", "had to sit through a lot of ads for this",
			"the ad right before the river, really?", "love the channel, not the ad breaks", "every time it got good, an ad"},
		// Series tracker: episodes
		{"episode {Ep} let's go", "episode {Ep} and still going", "this series is the best thing on the channel", "can't wait for episode {EpNext}",
			"binged every episode today", "{Money} and climbing", "episode {Ep}! the series keeps getting better", "been following since episode one",
			"I check every day for the next episode", "this challenge is so fun to follow", "don't stop this series", "episode {EpNext} when?",
			"the series is why I subscribed", "rooting for you to finish this", "best series on the channel"},
		// Spam (low weight; never a link)
		{"I teach poker for free, check my channel", "who else is grinding tonight", "anyone want to study together?", "sub to my channel for more hands",
			"I post my hands too, come say hi", "poker study group, anyone?", "check out my channel for daily sessions", "looking for people to rail tonight",
			"who's playing the Sunday majors this week?", "new poker channel here, any tips?"},
	};
	return All;
}

/** The archetype weights for a video (SPEC §6.4's "more likely on"), x10 fixed point. */
struct VmFacts
{
	std::map<std::string, std::string> V;
	bool Storytime = false, Live = false, Lesson = false, Beat = false, Suck = false, Bluffy = false, Stream = false, Rival = false, Hand = false;
	bool Won = false, Lost = false; // the player's hand, as its cards (or its kind) say: nobody commiserates over a pot you won
	bool Heavy = false, Series = false, QuestionT = false, HighRet = false;
	int Over = 0;
};

/** The first number before a word in a text ("64 big blinds" -> "64"), "" when none. */
std::string VmNumberBefore(const std::string& Low, const std::string& Word)
{
	const size_t At = Low.find(Word);
	if (At == std::string::npos || At < 2)
	{
		return std::string();
	}
	size_t End = At;
	while (End > 0 && Low[End - 1] == ' ')
	{
		--End;
	}
	size_t Start = End;
	while (Start > 0 && (std::isdigit(static_cast<unsigned char>(Low[Start - 1])) || Low[Start - 1] == ','))
	{
		--Start;
	}
	return Start < End ? Low.substr(Start, End - Start) : std::string();
}

std::string VmCardName(Card C)
{
	static const char* const Suits[4] = {"clubs", "diamonds", "hearts", "spades"};
	const std::string S = CardToString(C);
	const char Suit = S.size() >= 2 ? S[1] : 'c';
	const int I = Suit == 'd' ? 1 : Suit == 'h' ? 2 : Suit == 's' ? 3 : 0;
	return "the " + video_copy::RankWord(RankOf(C)) + " of " + Suits[I];
}

std::string VmGrouped(long long N)
{
	std::string S = std::to_string(N < 0 ? -N : N);
	for (int I = static_cast<int>(S.size()) - 3; I > 0; I -= 3)
	{
		S.insert(static_cast<size_t>(I), ",");
	}
	return S;
}

VmFacts VmFactsOf(const Video& V, double World)
{
	VmFacts F;
	std::string Low = V.Title;
	for (char& Ch : Low)
	{
		Ch = Ch >= 'A' && Ch <= 'Z' ? static_cast<char>(Ch - 'A' + 'a') : Ch;
	}
	// The title's numbers (the player saw them on the tile).
	const size_t Pct = Low.find('%');
	if (Pct != std::string::npos)
	{
		size_t S = Pct;
		while (S > 0 && std::isdigit(static_cast<unsigned char>(Low[S - 1])))
		{
			--S;
		}
		if (S < Pct)
		{
			F.V["Eq"] = Low.substr(S, Pct - S);
		}
	}
	std::string Big = V.Th.BigText;
	if (!F.V.count("Eq") && !Big.empty() && Big.back() == '%')
	{
		F.V["Eq"] = Big.substr(0, Big.size() - 1);
	}
	std::string Bb = VmNumberBefore(Low, "big blind");
	if (Bb.empty() && Big.size() > 3 && Big.compare(Big.size() - 3, 3, " BB") == 0)
	{
		Bb = Big.substr(0, Big.size() - 3);
	}
	if (!Bb.empty())
	{
		F.V["Bb"] = Bb;
	}
	const std::string Ep = [&] {
		const size_t At = Low.find("episode ");
		if (At == std::string::npos)
		{
			return std::string();
		}
		size_t E = At + 8;
		while (E < Low.size() && std::isdigit(static_cast<unsigned char>(Low[E])))
		{
			++E;
		}
		return Low.substr(At + 8, E - At - 8);
	}();
	if (!Ep.empty())
	{
		F.V["Ep"] = Ep;
		F.V["EpNext"] = std::to_string(std::atoi(Ep.c_str()) + 1);
	}
	const size_t Dollar = V.Title.find('$');
	if (Dollar != std::string::npos)
	{
		size_t E = Dollar + 1;
		while (E < V.Title.size() && (std::isdigit(static_cast<unsigned char>(V.Title[E])) || V.Title[E] == ',' ||
										 (V.Title[E] == '.' && E + 1 < V.Title.size() && std::isdigit(static_cast<unsigned char>(V.Title[E + 1])))))
		{
			++E;
		}
		if (E > Dollar + 1)
		{
			F.V["Money"] = V.Title.substr(Dollar, E - Dollar);
		}
	}
	const int Eq = F.V.count("Eq") ? std::atoi(F.V.at("Eq").c_str()) : -1;
	if (Eq > 50 && Eq < 100)
	{
		F.V["Lose"] = std::to_string(100 - Eq);
	}
	// Won or lost: a showdown's cards say it; else the kind does (a fold is neither).
	F.Won = V.K == Kind::Suckout || V.K == Kind::Bluff || V.K == Kind::HeroCall || V.K == Kind::BigPot || V.K == Kind::BiggestPot || V.K == Kind::Knockout ||
		V.K == Kind::DoubleKo || V.K == Kind::Monster || V.K == Kind::Envelope || V.K == Kind::Win;
	F.Lost = V.K == Kind::BadBeat || V.K == Kind::Cooler || V.K == Kind::BluffCaught || V.K == Kind::LastHand;
	// The thumbnail's cards: the player's (never theirs: only turned-up cards reach a Thumb, and comments name none) and, on
	// a hand the player lost, the board card that turned it (none when they were behind from the flop: a cooler's).
	const std::string& Cards = V.Th.Cards;
	const size_t A1 = Cards.find('/'), A2 = A1 == std::string::npos ? std::string::npos : Cards.find('/', A1 + 1);
	if (A1 != std::string::npos && A1 >= 4)
	{
		const std::vector<Card> Mine = ParseCards(Cards.substr(0, A1));
		if (Mine.size() >= 2)
		{
			F.V["MineWords"] = video_copy::HandWords(Mine);
			F.Hand = true;
		}
		if (A2 != std::string::npos)
		{
			const std::vector<Card> Theirs = ParseCards(Cards.substr(A1 + 1, A2 - A1 - 1));
			const std::vector<Card> Board = ParseCards(Cards.substr(A2 + 1));
			if (Board.size() >= 4)
			{
				F.V["Street"] = Board.size() >= 5 ? "river" : "turn";
			}
			if (Mine.size() >= 2 && Theirs.size() >= 2 && Board.size() >= 5)
			{
				const int Me = EvaluateHand(Mine, Board), Them = EvaluateHand(Theirs, Board);
				F.Won = Me > Them;
				F.Lost = Me < Them;
				for (int N = 4; N >= 3 && F.Lost; --N)
				{
					// The last street the player was ahead on: the next card is the one that did it.
					const std::vector<Card> Part(Board.begin(), Board.begin() + N);
					if (EvaluateHand(Mine, Part) > EvaluateHand(Theirs, Part))
					{
						F.V["Killer"] = VmCardName(Board[static_cast<size_t>(N)]);
						F.V["KillStreet"] = N == 4 ? "river" : "turn";
						break;
					}
				}
			}
		}
	}
	// Live (the Embercrest) and storytime (Dee's: a portrait face and nobody named).
	F.Live = Low.find("embercrest") != std::string::npos || (V.Th.Mood >= 10 && !V.Th.Opponent.empty());
	F.Storytime = V.Th.Mood >= 10 && V.Th.Opponent.empty() && !F.Live;
	F.Rival = V.K == Kind::Rival || V.Th.Opponent == RivalName;
	if (!F.Storytime && !F.Rival && !V.Th.Opponent.empty() && !F.Live)
	{
		F.V["Opp"] = V.Th.Opponent;
	}
	F.Beat = V.K == Kind::BadBeat || V.K == Kind::Cooler || V.K == Kind::SetOverSet || V.K == Kind::LastHand || V.K == Kind::Bubble || V.K == Kind::RunnerRunner;
	F.Suck = V.K == Kind::Suckout;
	F.Bluffy = V.K == Kind::Bluff || V.K == Kind::BluffCaught || V.K == Kind::HeroCall || V.K == Kind::HeroFold || V.K == Kind::LadderFold;
	F.Stream = V.K == Kind::StreamMoment || V.X.KastShare > 0.0;
	F.Lesson = V.T == TitleStyle::Teaching || V.X.Idea / 100 == 6;
	F.QuestionT = V.T == TitleStyle::Question;
	F.Heavy = V.X.Ads == AdLoad::Heavy;
	F.Series = V.X.Series >= 0 || F.V.count("Ep");
	F.HighRet = V.Retention > 0.45;
	F.Over = std::max(0, static_cast<int>(V.X.Promise) - static_cast<int>(V.X.Payoff));
	// The next round number of views (the "here before" line's), and timestamps within the video's length.
	const double Now = std::max(0.0, video::Views(V, World));
	static const long long Steps[] = {100, 250, 500, 1000, 2500, 5000, 10000, 25000, 50000, 100000, 250000, 500000, 1000000};
	for (const long long S : Steps)
	{
		if (Now < static_cast<double>(S))
		{
			F.V["Milestone"] = VmGrouped(S);
			break;
		}
	}
	return F;
}

/** A procedural handle: two poker words, no digits, no names. */
std::string VmHandle(uint32_t H)
{
	static const char* const First[] = {"river", "flop", "turn", "chip", "stack", "fold", "bluff", "pocket", "felt", "suited", "limp", "shove", "short", "deep",
		"tilt", "rail", "ante", "blind", "muck", "button", "cutoff", "kicker", "nuts", "gutshot", "rivered", "overbet", "minraise", "donk", "slowroll", "sunday"};
	static const char* const Second[] = {"rat", "goblin", "wizard", "enjoyer", "hound", "owl", "panda", "fox", "otter", "moth", "crow", "lord", "bandit", "monk",
		"sprout", "gremlin", "raccoon", "pigeon", "badger", "heron", "ferret", "walrus", "toad", "lynx", "mole", "beetle", "gecko", "falcon", "yak", "bison"};
	const uint32_t A = H % 30u, B = (H / 30u) % 30u, Style = (H / 900u) % 4u;
	std::string X = First[A], Y = Second[B];
	switch (Style)
	{
	case 0: return X + Y;
	case 1: return X + "_" + Y;
	case 2:
		X[0] = static_cast<char>(X[0] - 'a' + 'A');
		Y[0] = static_cast<char>(Y[0] - 'a' + 'A');
		return X + Y;
	default: return "the" + X + Y;
	}
}

/** A comment's timestamp "M:SS" inside the video (none when its length isn't known). */
std::string VmTime(const Video& V, uint32_t H)
{
	const int Seconds = static_cast<int>(std::floor(std::max(0.0, V.LengthMin) * 60.0));
	if (Seconds < 30)
	{
		return std::string();
	}
	const int At = 15 + static_cast<int>(H % static_cast<uint32_t>(Seconds - 15));
	char Buf[16];
	std::snprintf(Buf, sizeof(Buf), "%d:%02d", At / 60, At % 60);
	return Buf;
}

/** A template's condition ("#W " the player won the pot, "#L " lost it) off its text; false when the video doesn't meet it. */
bool VmCondition(std::string& T, bool Won, bool Lost)
{
	if (T.size() > 3 && T[0] == '#' && T[2] == ' ')
	{
		const char C = T[1];
		T = T.substr(3);
		return C == 'W' ? Won : C == 'L' ? Lost : false;
	}
	return true;
}

std::string VmFill(const std::string& T, const std::map<std::string, std::string>& V)
{
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
		const auto It = Close == std::string::npos ? V.end() : V.find(T.substr(Open + 1, Close - Open - 1));
		if (It == V.end() || It->second.empty())
		{
			return std::string();
		}
		Out.append(T, I, Open - I);
		FitArticle(Out, It->second); // "an 18% shot"
		Out += It->second;
		I = Close + 1;
	}
	return Out;
}

/** The archetype weights (x10) for a video. */
std::array<int, VmCount> VmWeights(const VmFacts& F)
{
	std::array<int, VmCount> W{};
	// The hand's people talk about the hand: sympathy, the joker and the results-oriented only over a pot the player lost; the
	// chance, the coach and the hand reader only on a hand or a lesson (nobody analyses a setup tour's river).
	W[VmSympathizer] = F.Lost ? (F.Beat ? 60 : 20) : 0;
	W[VmMath] = F.V.count("Eq") ? (F.Beat || F.Suck ? 40 : F.Lesson ? 30 : 10) : F.Lesson ? 12 : F.Hand ? 4 : 0;
	W[VmResults] = F.Lost ? 30 : 0;
	W[VmCoach] = F.Lesson ? 40 : F.Bluffy ? 30 : F.Hand ? 10 : 0;
	W[VmTimestamp] = F.HighRet ? 22 : 6;
	W[VmEarly] = 15;
	W[VmJoker] = F.Lost ? (F.Beat ? 30 : 8) : 0;
	W[VmWasInHand] = F.V.count("Opp") ? 20 : 0;
	W[VmKastRegular] = F.Stream ? 60 : 0;
	W[VmQuestion] = F.QuestionT || F.Lesson ? 40 : 10;
	W[VmHandReader] = F.Bluffy ? 50 : F.Hand ? 10 : 0;
	W[VmNit] = F.Bluffy ? 30 : F.Hand ? 8 : 0;
	W[VmSuperfan] = 20;
	W[VmGhost] = F.Rival && !F.Storytime ? 60 : 0;
	W[VmLocal] = F.Live && !F.Storytime ? 40 : 0;
	W[VmStorySafe] = F.Storytime ? 60 : 0;
	W[VmClickbait] = F.Over * 15;
	W[VmAds] = F.Heavy ? 40 : 0;
	W[VmSeries] = F.Series ? 50 : 0;
	W[VmSpam] = 3;
	if (F.Storytime)
	{
		W[VmWasInHand] = 0;
		W[VmGhost] = 0;
		W[VmLocal] = 0;
	}
	return W;
}

int VmPick(const std::array<int, VmCount>& W, uint32_t H, bool AllowPicky)
{
	int Total = 0;
	for (int A = 0; A < VmCount; ++A)
	{
		Total += AllowPicky || !VmPicky(A) ? W[static_cast<size_t>(A)] : 0;
	}
	if (Total <= 0)
	{
		return VmSuperfan;
	}
	int R = static_cast<int>(H % static_cast<uint32_t>(Total));
	for (int A = 0; A < VmCount; ++A)
	{
		const int X = AllowPicky || !VmPicky(A) ? W[static_cast<size_t>(A)] : 0;
		if (R < X)
		{
			return A;
		}
		R -= X;
	}
	return VmSuperfan;
}

uint32_t VmMix(uint32_t H, uint32_t Salt)
{
	H ^= Salt * 0x9e3779b9u;
	H ^= H >> 16;
	H *= 0x85ebca6bu;
	H ^= H >> 13;
	H *= 0xc2b2ae35u;
	H ^= H >> 16;
	return H;
}
} // namespace

std::vector<std::string> CommentFactsOf(const Video& V, double World)
{
	const VmFacts F = VmFactsOf(V, World);
	std::vector<std::string> Out;
	for (const auto& Kv : F.V)
	{
		Out.push_back(Kv.second);
	}
	Out.push_back(V.Title);
	// Timestamps: any minute and second inside the video.
	const int Seconds = static_cast<int>(std::floor(std::max(0.0, V.LengthMin) * 60.0));
	for (int M = 0; M * 60 < Seconds; ++M)
	{
		Out.push_back(std::to_string(M));
	}
	for (int S = 0; S < 60 && Seconds >= 30; ++S)
	{
		char Buf[8];
		std::snprintf(Buf, sizeof(Buf), "%02d", S);
		Out.push_back(Buf);
	}
	Out.push_back("100"); // "times in 100"
	return Out;
}

std::vector<std::string> CommentInventory()
{
	std::vector<std::string> Out;
	for (const std::vector<const char*>& Set : VmTemplates())
	{
		for (const char* T : Set)
		{
			std::string Text = T;
			(void)VmCondition(Text, true, true);
			Out.push_back(Text);
		}
	}
	return Out;
}

/** A published video as a lead (Repackage's options): what it keeps of its moment (the comments' facts, read the same way). */
Candidate RepackLead(const Video& V)
{
	const VmFacts F = VmFactsOf(V, V.At);
	Candidate L;
	L.Id = "repack:" + V.Id;
	L.K = V.K;
	L.Idea = V.X.Idea;
	L.At = V.At;
	L.Site = V.Site;
	L.Source = F.Live ? 1 : F.Storytime ? 2 : 0;
	L.Storytime = F.Storytime;
	L.Opponent = F.Storytime ? std::string() : V.Th.Opponent;
	L.Drama = 0.6;
	const std::string& Cards = V.Th.Cards;
	const size_t A1 = Cards.find('/'), A2 = A1 == std::string::npos ? std::string::npos : Cards.find('/', A1 + 1);
	if (A1 != std::string::npos && A2 != std::string::npos)
	{
		L.Rec.Mine = ParseCards(Cards.substr(0, A1));
		const std::vector<Card> Theirs = ParseCards(Cards.substr(A1 + 1, A2 - A1 - 1));
		if (Theirs.size() >= 2)
		{
			L.Rec.Shown.push_back({L.Opponent.empty() ? std::string("Seat 1") : L.Opponent, Theirs});
		}
		L.Rec.Board = ParseCards(Cards.substr(A2 + 1));
	}
	// Won or lost, as its kind says (the record's net isn't kept).
	const bool Lost = V.K == Kind::BadBeat || V.K == Kind::Cooler || V.K == Kind::BluffCaught || V.K == Kind::LastHand || V.K == Kind::Bubble;
	L.Rec.HeroNet = L.Rec.Mine.size() >= 2 ? (Lost ? -1 : 1) : 0;
	L.EquityAtAllIn = F.V.count("Eq") ? std::atof(F.V.at("Eq").c_str()) / 100.0 : -1.0;
	std::string Facts;
	for (const char* K : {"Bb", "Ep"})
	{
		if (F.V.count(K))
		{
			Facts += std::string(K) + "=" + FactEsc(F.V.at(K)) + ";";
		}
	}
	L.IdeaFacts = Facts;
	return L;
}
} // namespace vcontent

std::vector<TitleOption> RepackTitles(const Video& V, const Channel& C, int Shuffle, int Count)
{
	const Candidate L = vcontent::RepackLead(V);
	Choice Ch;
	Ch.Candidates = {L.Id};
	Ch.F = V.F;
	Ch.T = V.T;
	// C3 gate: never the title the video already has (its own template, or the same words): a repackage to it would spend one
	// of the two changes and skip a check-in for nothing. (A video past the last 20 uploads isn't covered by the picker's skip.)
	std::vector<TitleOption> Out = TitleOptions(Ch, {L}, C, Shuffle, Count + 2);
	Out.erase(std::remove_if(Out.begin(), Out.end(), [&V](const TitleOption& O) { return O.Key == V.X.TitleTpl || O.Text == V.Title; }), Out.end());
	Out.resize(std::min(Out.size(), static_cast<size_t>(std::max(0, Count))));
	return Out;
}

std::vector<ThumbSpec> RepackThumbs(const Video& V, const Channel& C, int Shuffle, int Count)
{
	const Candidate L = vcontent::RepackLead(V);
	Choice Ch;
	Ch.Candidates = {L.Id};
	Ch.F = V.F;
	Ch.T = V.T;
	// C3 gate: never the thumbnail it shows now (the same recipe and caption), for the same reason.
	const ThumbSpec& Now = V.X.Spec[static_cast<size_t>(std::max<int>(0, V.X.DuelWin))];
	std::vector<ThumbSpec> Out = ThumbOptions(Ch, {L}, C, Shuffle, Count + 1);
	Out.erase(std::remove_if(Out.begin(), Out.end(),
				  [&Now](const ThumbSpec& S) { return S.Arch == Now.Arch && S.Pal == Now.Pal && S.Variant == Now.Variant && S.Caption == Now.Caption; }),
		Out.end());
	Out.resize(std::min(Out.size(), static_cast<size_t>(std::max(0, Count))));
	return Out;
}

std::vector<Comment> CommentsOf(const Video& V, double World, int From, int Count)
{
	using namespace vcontent;
	std::vector<Comment> Out;
	if (!V.X.On || Count <= 0)
	{
		return Out;
	}
	VmFacts F = VmFactsOf(V, World);
	if (!F.V.count("Lose"))
	{
		F.V.erase("Eq"); // (a suckout's 30% is no "great spot to get it in": the chance lines are the favourite's)
	}
	const std::array<int, VmCount> W = VmWeights(F);
	const auto& Sets = VmTemplates();
	// The video's likes (about 4.5% of its views): the top comment holds at most 3% of them.
	const double Likes = std::max(0.0, video::Views(V, World)) * 0.045;
	const std::string Key = std::to_string(V.X.Fate) + ":comment:";
	for (int I = std::max(0, From); I < From + Count; ++I)
	{
		const uint32_t H = Fnv1a(Key + std::to_string(I));
		// The picky ones sit only at i % 10 == 9: never two in any ten in a row. The player who was in the hand speaks once,
		// early (at a slot of the video's own, 1-9), not in every page of comments.
		const int OppSlot = 1 + static_cast<int>(V.X.Fate % 9u);
		std::array<int, VmCount> Wi = W;
		Wi[VmWasInHand] = 0;
		int A = I == OppSlot && W[VmWasInHand] > 0 ? static_cast<int>(VmWasInHand) : VmPick(Wi, H, I % 10 == 9);
		std::map<std::string, std::string> Slots = F.V;
		const std::string Time = VmTime(V, VmMix(H, 7));
		if (!Time.empty())
		{
			Slots["Time"] = Time;
		}
		std::string Text;
		for (int Try = 0; Try < 3 && Text.empty(); ++Try)
		{
			const std::vector<const char*>& Set = Sets[static_cast<size_t>(A)];
			const size_t Start = VmMix(H, 11u + static_cast<uint32_t>(Try)) % Set.size();
			for (size_t J = 0; J < Set.size() && Text.empty(); ++J)
			{
				std::string T = Set[(Start + J) % Set.size()];
				Text = VmCondition(T, F.Won, F.Lost) ? VmFill(T, Slots) : std::string();
			}
			if (Text.empty())
			{
				A = Try == 0 ? VmSuperfan : VmEarly; // every archetype's set has slot-free lines; this is the last word
			}
		}
		Comment C;
		C.Text = Text;
		C.Archetype = static_cast<uint8_t>(A);
		C.Author = A == VmWasInHand && F.V.count("Opp") ? F.V.at("Opp") : A == VmGhost ? ((H >> 8) % 3 == 0 ? std::string("totally_not_gh0stfold") : std::string(RivalName))
																						 : VmHandle(VmMix(H, 3));
		const double Rank = std::pow(static_cast<double>(I + 1), -1.1);
		C.Likes = static_cast<int>(std::floor(Likes * 0.03 * Rank * (0.6 + 0.4 * static_cast<double>(VmMix(H, 5) % 1000u) / 1000.0)));
		C.At = std::min(World, V.At + 30.0 * std::pow(static_cast<double>(I + 1), 1.25));
		Out.push_back(C);
	}
	return Out;
}
} // namespace video
} // namespace ss
