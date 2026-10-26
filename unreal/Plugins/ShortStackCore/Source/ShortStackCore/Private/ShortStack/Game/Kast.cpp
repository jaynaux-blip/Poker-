#include "ShortStack/Game/Kast.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Handles.h"
#include "ShortStack/Game/KastGrowth.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/SiteData.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sponsor.h"
#include "KastCopy.h"
#include "KastPlay.h"
#include "KastRoom.h"
#include "SponsorCopy.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <string_view>
#include <unordered_set>

namespace ss
{
namespace kast
{
namespace kast_detail
{
constexpr double KastDay = 24.0 * 60.0;
constexpr double AlertSeconds = 4.6;

uint32_t NameHash(const std::string& S)
{
	uint32_t H = 2166136261u;
	for (const char Ch : S)
	{
		H ^= static_cast<uint32_t>(static_cast<unsigned char>(Ch));
		H *= 16777619u;
	}
	return H;
}

uint32_t NameColor(const std::string& Name)
{
	static const uint32_t Colors[] = {0xff6b6b, 0x4dabf7, 0x51cf66, 0xfcc419, 0xcc5de8, 0xff922b, 0x22b8cf, 0xf06595, 0x94d82d, 0x845ef7, 0x20c997, 0xffa94d, 0x74c0fc, 0xe599f7};
	return Colors[NameHash(Name) % (sizeof(Colors) / sizeof(Colors[0]))];
}

/** Audience by the hour: poker streams live at night. */
double TimeOfDay(double World)
{
	const double H = std::fmod(World, KastDay) / 60.0;
	double F = H >= 18.0 ? 1.25 : H < 3.0 ? 1.05 : H < 8.0 ? 0.55 : H < 12.0 ? 0.7 : 0.9;
	const int Weekday = static_cast<int>(std::floor(World / KastDay)) % 7; // 0 Monday
	F *= Weekday >= 4 ? 1.1 : 1.0;
	return F;
}

/** Fewer channels live late at night: easier to get found. */
double Competition(double World)
{
	const double H = std::fmod(World, KastDay) / 60.0;
	return H >= 22.0 || H < 6.0 ? 1.15 : 1.0;
}

// (The pace terms on strangers and follows retired with the creator update, ruling C6: the room never reads pace; heads-down
// reaches it only through Engage. Rerun's buzz is growth::Buzz now, read against the channel's size, §2.5.1.)

/** Tonight's moment log (update 2 §4.4): at most this many notes, and this many chat lines on each. */
constexpr size_t KastMomentNotes = 60;
constexpr size_t KastMomentLines = 3;

std::string Fill(std::string Text, const std::string& Key, const std::string& Value)
{
	for (size_t At = Text.find(Key); At != std::string::npos; At = Text.find(Key, At + Value.size()))
	{
		Text.replace(At, Key.size(), Value);
	}
	return Text;
}

const std::vector<std::string>& Lines(Moment M)
{
	static const std::vector<std::string> Register = {"new tourney lets go", "gl!", "how many runners?", "kastHype", "{event} lets gooo", "gl gl gl", "another one?", "{event}!! kastHype", "deep run tonight"};
	static const std::vector<std::string> AllIn = {"ALL IN kastPog", "here we go", "heart rate check", "call call call", "no way he has it", "i can't watch", "kastPog kastPog kastPog", "dont do it", "pray", "HOLD", "oh no oh no", "sweat time"};
	static const std::vector<std::string> Won = {"SHIP IT kastHype", "LETS GOOO", "kastClap kastClap", "holds!!", "EZ", "{hero} is cooking", "doubled up kastChip", "never in doubt", "{ship} {ship}", "chat we're so back", "HOLDS kastHype"};
	static const std::vector<std::string> Lost = {"unlucky", "noooo", "kastRIP", "every time", "sick", "that river man", "variance", "deep breaths", "it's a marathon", "kastSalt", "brutal"};
	static const std::vector<std::string> Beat = {"RIGGED kastSalt", "you were {pct}%", "THAT RIVER", "kastRIP kastRIP", "the poker gods hate you", "unreal", "sick beat", "nooooo", "rigged site confirmed kastLUL", "{tilt} {tilt}", "close the laptop"};
	static const std::vector<std::string> Big = {"big pot kastChip", "nice pot", "chip leader soon?", "kastHype", "stack it", "monster pot"};
	static const std::vector<std::string> Ko = {"bounty! kastChip", "sent him home", "kastGG", "KO!", "collecting heads", "bye {who}", "another scalp"};
	static const std::vector<std::string> Bubble = {"bubble time kastPog", "fold everything", "ICM pressure", "don't min cash scared", "tighten up", "who's the short stack?", "hand for hand kastPog"};
	static const std::vector<std::string> Itm = {"IN THE MONEY kastHype", "rent money!", "gg bubble", "now play to win", "kastClap", "{rent} {rent}", "we're cashing!!"};
	static const std::vector<std::string> Ft = {"FINAL TABLE kastPog", "clip it", "we're watching history", "FT!! kastHype kastHype", "tell your mom", "raid incoming?", "FINAL TABLE LETS GO", "kastPog FT kastPog"};
	static const std::vector<std::string> Win = {"CHAMPION kastHype", "HE DID IT", "kastPog kastPog kastPog", "WE WON", "screenshot this", "{prize}!!!", "best stream ever", "I WAS HERE", "{ship} {ship} {ship}", "rent PAID"};
	static const std::vector<std::string> Bust = {"gg", "gg wp", "kastRIP", "next one", "unlucky gg", "it happens", "re-reg?", "kastGG", "gg chat"};
	static const std::vector<std::string> Cashed = {"nice cash", "gg, profit", "rent fund +", "kastClap", "well played"};
	static const std::vector<std::string> Best = {"big brain", "great fold", "that's the spot", "solver approved", "kastClap", "textbook", "sick read"};
	static const std::vector<std::string> Blunder = {"what was that kastLUL", "uhhh", "bro", "tilted?", "misclick?", "kastLUL kastLUL", "that's a fold...", "?????", "the coach is crying"};
	static const std::vector<std::string> Rival = {"gh0stfold is here kastPog", "the rival!!", "it's personal now", "get him", "ghost on your table", "not this guy again", "gh0stfold never sleeps"};
	static const std::vector<std::string> Level = {"blinds up", "antes", "clock's ticking", "level up kastChip"};
	switch (M)
	{
	case Moment::Register: return Register;
	case Moment::AllIn: return AllIn;
	case Moment::WonAllIn: return Won;
	case Moment::LostAllIn: return Lost;
	case Moment::BadBeat: return Beat;
	case Moment::BigPot: return Big;
	case Moment::Knockout: return Ko;
	case Moment::Bubble: return Bubble;
	case Moment::InTheMoney: return Itm;
	case Moment::FinalTable: return Ft;
	case Moment::Win: return Win;
	case Moment::Bust: return Bust;
	case Moment::Cashed: return Cashed;
	case Moment::BestPlay: return Best;
	case Moment::Blunder: return Blunder;
	case Moment::Rival: return Rival;
	default: return Level;
	}
}

const std::vector<std::string>& IdleTable()
{
	static const std::vector<std::string> V = {"what are the blinds?", "how many left?", "fold pre", "this table is soft", "kastFish seat 4", "who's the chip leader", "gl gl", "{tables} tables is crazy",
		"rank?", "patience", "card dead again kastRIP", "!rent", "how much is rent", "what stakes is this", "kastChip kastChip", "lurking kastLove", "open shove it", "fold fold fold",
		"chat is he good?", "so many fish", "late reg still open?", "what's the field like", "seat 7 plays every hand", "kastFish kastFish", "stack is healthy", "{bb} bb, we're fine",
		"I learn so much here", "hi from work", "just woke up", "the 3am grind kastHype", "is this the night owl?"};
	return V;
}

const std::vector<std::string>& IdleLobby()
{
	static const std::vector<std::string> V = {"what are we playing next?", "register the Night Owl", "bed time?", "go play a turbo", "lobby simulator kastLUL", "pick a tourney already",
		"multi-table!", "play a bounty", "PKO pls", "chat what should he play", "satellite to the big one?", "!schedule", "just chatting today?", "how's the bankroll"};
	return V;
}

const std::vector<std::string>& Greetings()
{
	static const std::vector<std::string> V = {"hi chat", "hey {hero}", "first time here", "evening grinders", "o7", "just got off work, what did I miss", "kastLove", "hello from {country}",
		"yo", "hiii", "hey hey", "back again", "made it", "good luck tonight", "how's the session going"};
	return V;
}

const std::vector<std::string>& Questions()
{
	static const std::vector<std::string> V = {"how long have you been playing?", "what would you do with AQ under the gun?", "do you play live too?", "what's your biggest cash?",
		"how do you deal with tilt?", "tips for a beginner?", "what's ICM?", "how many tables can you handle?", "is RiverLine soft?", "what's your setup?", "do you have a job too?",
		"how big is your bankroll?", "when do you sleep?", "favorite starting hand?", "do you use a HUD?"};
	return V;
}

const std::vector<std::string>& Answers()
{
	static const std::vector<std::string> V = {"a couple of years. seriously only this year", "raise it. every time. folding AQ is for the 3am version of me",
		"Dee's game sometimes. the Embercrest on sundays", "biggest? not yet. ask me after tonight", "walk away. water. come back. mostly I fail", "bankroll management and a lot of folding",
		"tournament chips aren't money. near the bubble, losing hurts more than winning helps", "four on a good night. two when I'm tired", "softer than you'd think at this hour",
		"!gear in the panels, everything I bought off GearDrop", "I pick up shifts. this is the dream though", "enough to make rent if I don't punt",
		"after the last tournament. so... maybe", "pocket nines. don't ask", "RiverLine shows the stats. I just read them"};
	return V;
}

const std::vector<std::string>& Trolls()
{
	static const std::vector<std::string> V = {"this guy is so bad", "uninstall", "fish streamer", "worst player on Kast", "boring", "unfollowed", "you're gonna miss rent lol", "free chips for the table",
		"L", "imagine streaming this", "my grandma plays better", "quit poker", "zzzzz", "rigged streamer lol", "nobody watches this"};
	return V;
}

const std::vector<std::string>& Spam()
{
	static const std::vector<std::string> V = {"cheap viewers at v1ewz dot biz", "free chips here: rlv-promo dot cc", "buy followers 1k = $5 dm me", "check my channel pls",
		"want to become famous? best viewers on k4st-boost", "FREE SKINS click my bio"};
	return V;
}

const std::vector<std::string>& Tips()
{
	static const std::vector<std::string> V = {"for rent!", "call the clock on yourself", "this is for the bad beat earlier", "gl on the deep run", "buy a coffee", "for the grind",
		"you taught me ICM, thank you", "first tip ever, gl", "ship it tonight", "rent fund", "love the stream", "play a bounty next", "{hero} > gh0stfold", "fold more lol", "energy drink money"};
	return V;
}

const std::vector<std::string>& Cheers()
{
	static const std::vector<std::string> V = {"let's go", "for the river gods", "big run incoming", "gl!", "shove", "kastHype", "deep run fuel", "hold this time"};
	return V;
}

const std::vector<std::string>& ThankLines()
{
	static const std::vector<std::string> V = {"thanks for hanging out chat", "welcome in everyone!", "appreciate all the follows tonight", "if you're new, hit follow. we're making rent tonight",
		"chat you're the best. seriously", "questions are welcome, ask away", "big thanks to the subs, you keep the lights on", "deep breaths chat. we grind"};
	return V;
}

const std::vector<std::string>& QualityLines(const gear::Effects& G, bool Dropping)
{
	static const std::vector<std::string> Drop = {"stream is lagging", "frames dropping", "buffering...", "is it just me or is it choppy", "PC is dying kastLUL"};
	static const std::vector<std::string> NoMic = {"mic sounds like a potato", "can't hear you", "buy a mic kastLUL", "audio is so quiet", "is that the laptop mic?"};
	static const std::vector<std::string> NoCam = {"cam is so grainy", "is that a potato cam", "240p facecam kastLUL", "can't see your face"};
	static const std::vector<std::string> Dark = {"you're sitting in the dark", "turn a light on", "the blue glow kastLUL", "we can only see the monitor light"};
	static const std::vector<std::string> Clean = {"stream looks so clean", "what mic is that?", "cam quality kastPog", "this production value", "audio is crispy", "looking pro"};
	if (Dropping)
	{
		return Drop;
	}
	if (G.MicTier == 0)
	{
		return NoMic;
	}
	if (G.CamTier == 0)
	{
		return NoCam;
	}
	if (G.Lights == 0)
	{
		static const std::vector<std::string> LedDark = {"love the LEDs, now light your face kastLUL", "nice room lights tho", "the LEDs carry this stream", "gamer lights, no face light kastLUL"};
		return G.Leds ? LedDark : Dark;
	}
	if (G.Leds)
	{
		// The room's colour, in chat.
		static const std::vector<std::vector<std::string>> Led = [] {
			const std::vector<std::vector<std::string>> Extra = {
				{"the green lights are so cozy", "felt green LEDs, a man of culture", "looks like you're sitting inside a poker table"},
				{"that blue is so calm", "ice blue setup goes hard", "the blue room at 3am kastPog"},
				{"purple LEDs kastPog", "the violet setup is clean", "matching kast purple, respect"},
				{"red lights, sweat mode", "red room, the table agrees", "the red LEDs are a vibe"},
				{"gold lights, rich already kastLUL", "that warm gold is cozy", "gold room = gold run"},
				{"pink lights from across the street kastPog", "after hours vibes", "love the pink glow"},
				{"the aurora lights are hypnotic", "rainbow room kastPog", "how do the LEDs change like that", "the light show is crazy"},
			};
			std::vector<std::vector<std::string>> Out;
			for (const std::vector<std::string>& E : Extra)
			{
				std::vector<std::string> V = {"stream looks so clean", "what mic is that?", "cam quality kastPog", "this production value", "looking pro", "what LEDs are those?"};
				V.insert(V.end(), E.begin(), E.end());
				Out.push_back(V);
			}
			return Out;
		}();
		return Led[static_cast<size_t>(std::max(0, std::min(gear::LedPresetCount - 1, G.LedPreset)))];
	}
	return Clean;
}

std::string CountryName(const std::string& Code)
{
	static const std::pair<const char*, const char*> Names[] = {{"US", "Ohio"}, {"GB", "London"}, {"DE", "Berlin"}, {"BR", "Brazil"}, {"CA", "Toronto"}, {"NL", "Amsterdam"}, {"SE", "Sweden"},
		{"FR", "Paris"}, {"ES", "Spain"}, {"IT", "Italy"}, {"MX", "Mexico"}, {"AR", "Argentina"}, {"RU", "Russia"}, {"UA", "Kyiv"}, {"PL", "Poland"}, {"JP", "Tokyo"}, {"KR", "Seoul"},
		{"AU", "Australia"}, {"IE", "Dublin"}, {"FI", "Finland"}, {"PH", "Manila"}, {"IN", "India"}, {"CN", "Shanghai"}};
	for (const auto& N : Names)
	{
		if (Code == N.first)
		{
			return N.second;
		}
	}
	return "the night shift";
}

int KastDayOf(double World)
{
	return static_cast<int>(std::floor(World / KastDay));
}

int KastWeekday(int Day)
{
	return ((Day % 7) + 7) % 7; // 0 Monday
}

const TitleSpec& TitleOf(const Channel& Ch)
{
	const std::vector<TitleSpec>& T = Titles();
	return T[static_cast<size_t>(std::max(0, std::min(static_cast<int>(T.size()) - 1, Ch.Title)))];
}

/** A week starts Monday at midnight (day 0 is a Monday). */
int KastWeekOf(double World)
{
	return static_cast<int>(std::floor(World / (7.0 * KastDay)));
}

/**
 * A new week: the streak carries on if last week had three streams (of 60+ world minutes), and the goals start over. The
 * creator update (SPEC_CREATORS §2.5.1): a missed week halves the streak instead of ending it, one halving a week that went
 * by, so a skip of N weeks reads the same as N one-week steps.
 */
void RollWeek(Channel& Ch, double World)
{
	const int W = KastWeekOf(World);
	if (Ch.Week == W)
	{
		return;
	}
	if (Ch.Week >= 0 && W > Ch.Week)
	{
		for (int K = Ch.Week; K < W && K < Ch.Week + 64; ++K)
		{
			const bool Kept = K == Ch.Week && Ch.WeekStreams >= 3;
			Ch.Streak = Kept ? Ch.Streak + 1 : Ch.Streak / 2;
		}
	}
	Ch.Week = W;
	Ch.WeekStreams = 0;
	Ch.WeekMinutes = 0.0;
	Ch.WeekAnswers = 0;
	Ch.WeekRegulars = 0;
	Ch.WeekRewarded = 0;
}

int TrackedFollowers(const Channel& Ch)
{
	int N = 0;
	for (const Member& M : Ch.Members)
	{
		N += M.Follower ? 1 : 0;
	}
	return N;
}

/**
 * The creator update (SPEC_CREATORS C8, §2.10.3): Partner's 30-day average excludes raiders and the site's Broadcast, the real
 * rule: each log row's AvgOwn (an old row reads its Avg). Window30 stays the sponsors' (Avg30).
 */
double KastAvgOwn30(const Channel& Ch, double World)
{
	double Minutes = 0.0;
	double ViewerMins = 0.0;
	for (const StreamLog& L : Ch.Log)
	{
		if (L.Start >= World - 30.0 * KastDay)
		{
			Minutes += L.Minutes;
			ViewerMins += static_cast<double>(L.AvgOwn >= 0 ? L.AvgOwn : L.Avg) * L.Minutes;
		}
	}
	return Minutes > 0.0 ? ViewerMins / Minutes : 0.0;
}

/**
 * A chatter is a viewer (the 2026-10-21 report: "When playing MTTs while live streaming, kast shows 0 Viewers even when
 * there is chatters"). The creator update's Room keeps it by construction (R1, KastRoom.h): the count is whole people, the
 * people present are the only ones who speak (Stream::Chatter samples them), and the WIP's stranger window gave way to the
 * named guests. A timeout ends the visit (F1), and the name can't type for its ten minutes.
 */
bool KastTimedOut(const std::map<std::string, double>& TimedOutUntil, const std::string& Name, double RealNow)
{
	const auto It = TimedOutUntil.find(Name);
	return It != TimedOutUntil.end() && It->second > RealNow;
}

const char* const MomentNames[] = {"Register", "AllIn", "WonAllIn", "LostAllIn", "BadBeat", "BigPot", "Knockout", "Bubble", "InTheMoney", "FinalTable", "Win", "Bust", "Cashed", "BestPlay", "Blunder", "Rival", "LevelUp",
	"Top3"};
static_assert(sizeof(MomentNames) / sizeof(MomentNames[0]) == static_cast<size_t>(Moment::Count), "a name for every Moment");

/** How hard a moment hits: hype added, chat lines it sets off. */
struct Hit
{
	double Hype = 0.0;
	int Lines = 0;
	Mood Face = Mood::Focus;
	double ClipChance = 0.0;
};

Hit HitOf(Moment M)
{
	switch (M)
	{
	case Moment::Register: return {5.0, 3, Mood::Happy, 0.0};
	case Moment::AllIn: return {12.0, 5, Mood::Shocked, 0.0};
	case Moment::WonAllIn: return {16.0, 6, Mood::Hyped, 0.18};
	case Moment::LostAllIn: return {8.0, 4, Mood::Tilted, 0.0};
	case Moment::BadBeat: return {20.0, 7, Mood::Shocked, 0.3};
	case Moment::BigPot: return {7.0, 3, Mood::Happy, 0.05};
	case Moment::Knockout: return {9.0, 4, Mood::Happy, 0.1};
	case Moment::Bubble: return {10.0, 4, Mood::Focus, 0.0};
	case Moment::InTheMoney: return {16.0, 6, Mood::Hyped, 0.05};
	case Moment::FinalTable: return {26.0, 8, Mood::Hyped, 0.5};
	case Moment::Win: return {45.0, 10, Mood::Hyped, 0.95};
	case Moment::Bust: return {4.0, 5, Mood::Tilted, 0.0};
	case Moment::Cashed: return {8.0, 4, Mood::Happy, 0.0};
	case Moment::BestPlay: return {2.5, 2, Mood::Focus, 0.0};
	case Moment::Blunder: return {5.0, 4, Mood::Laugh, 0.12};
	case Moment::Rival: return {9.0, 5, Mood::Focus, 0.0};
	case Moment::Top3: return {12.0, 5, Mood::Hyped, 0.1}; // the creator update: three left (§2.3.5)
	default: return {0.5, 1, Mood::Focus, 0.0};
	}
}

std::string ClipTitle(Moment M, const std::string& Hero, const std::string& Detail, Rng& R)
{
	switch (M)
	{
	case Moment::WonAllIn: return R.Chance(0.5) ? Hero + " holds the all-in" + (Detail.empty() ? std::string() : " for " + Detail) : "SHIP IT (" + Hero + " doubles)";
	case Moment::BadBeat: return R.Chance(0.5) ? "the cruelest river (" + Hero + ")" : Hero + " gets rivered" + (Detail.empty() ? std::string() : " at " + Detail);
	case Moment::BigPot: return Hero + " drags a monster pot";
	case Moment::Knockout: return Hero + " sends " + (Detail.empty() ? std::string("them") : Detail) + " home";
	case Moment::FinalTable: return Hero + " makes the final table" + (Detail.empty() ? std::string() : " of the " + Detail);
	case Moment::Win: return Hero + " WINS" + (Detail.empty() ? std::string() : " the " + Detail);
	case Moment::Blunder: return Hero + " does WHAT with this hand";
	default: return Hero + " on Kast";
	}
}
} // namespace kast_detail

// ------------------------------------------------------------------ directory, sponsors, titles

const std::vector<Streamer>& Directory()
{
	static const std::vector<Streamer> L = [] {
		std::vector<Streamer> V;
		auto Add = [&](const char* Name, const char* Title, const char* Tag, const char* Country, int Followers, int Viewers, int Opens, int Hours, int Days, uint32_t Color) {
			Streamer S;
			S.Name = Name;
			S.Title = Title;
			S.Tag = Tag;
			S.Country = Country;
			S.Followers = Followers;
			S.Viewers = Viewers;
			S.Opens = Opens;
			S.Hours = Hours;
			S.Days = Days;
			S.Color = Color;
			V.push_back(S);
		};
		Add("VikingVolta", "$5k bankroll challenge day 41 \xC2\xB7 !course", "Loud, Dutch, heart on his sleeve. The biggest poker channel on Kast.", "NL", 418000, 9400, 18 * 60, 7, 0x6f, 0xf97316);
		Add("HighRollerHana", "High roller Thursday \xC2\xB7 $1k+ only", "Seoul's high-stakes queen. Thursdays and Sundays.", "KR", 132000, 3100, 21 * 60, 5, 0x48, 0xf472b6);
		Add("MissFinch", "Sunday grind \xC2\xB7 chill vibes \xC2\xB7 every spot explained", "Calm high-stakes reg. Explains every decision.", "GB", 96000, 2100, 15 * 60, 7, 0x7f, 0x38bdf8);
		Add("BluffSquadTV", "3 pros, 12 tables, 0 sleep", "A rotating crew of pros. Somebody is always live.", "US", 74000, 1600, 19 * 60, 9, 0x7f, 0xef4444);
		Add("SuitedConnor", "micro to mid in 100 days (day 63)", "Bankroll challenge from Dublin. Never moves up early.", "IE", 54000, 1150, 20 * 60, 6, 0x3f, 0x22c55e);
		Add("LaReinaDelRio", "MTT grind en espa\xC3\xB1ol \xC2\xB7 !torneos", "Mexico City's tournament grinder. Chat in two languages.", "MX", 41000, 900, 23 * 60, 6, 0x7d, 0xeab308);
		Add("KatOnTheRiver", "PKO grind w/ chat \xC2\xB7 bounties only", "Bounty specialist. Loud music, louder chat.", "CA", 38000, 820, 22 * 60, 5, 0x5e, 0xa855f7);
		Add("ThePokerMonk", "Silent study stream \xC2\xB7 no music \xC2\xB7 just poker", "Meditative. Barely speaks. Never tilts.", "JP", 21000, 410, 1 * 60, 8, 0x7f, 0x94a3b8);
		Add("NitNation", "folding for six hours straight, AMA", "Comedy nit. Chat counts the folds.", "AU", 12000, 260, 3 * 60, 6, 0x7f, 0x14b8a6);
		Add("chipleader_carla", "road to the Sunday Major \xC2\xB7 day 5", "Up-and-coming MTT reg from S\xC3\xA3o Paulo.", "BR", 8600, 140, 0, 7, 0x7f, 0x4ade80);
		Add("FoldEquityFred", "ICM homework with chat \xC2\xB7 beginner friendly", "Teaches the math. Patient with questions.", "DE", 4100, 70, 2 * 60, 6, 0x7f, 0x60a5fa);
		Add("gh0stfold", "never logs off.", "Your rival from the tables. Always live. Always watching.", "", 2400, 36, 0, 24, 0x7f, 0x64748b);
		V.back().Rival = true;
		return V;
	}();
	return L;
}

namespace kast_detail
{
/** ViewersNow's body, with the night's name hash for world day D from HashOf(D) (NameHash(name + D); the Room's rank keeps
 *  them a day, C-K's M2). */
template <typename HashFn>
int DirViewersAt(const Streamer& S, double World, double Minute, int Today, HashFn&& HashOf)
{
	if (S.Hours >= 24)
	{
		const double Wobble = 1.0 + 0.04 * std::sin(World / 7.0 + static_cast<double>(S.Followers % 97));
		return static_cast<int>(std::round(static_cast<double>(S.Viewers) * Wobble * (0.75 + 0.25 * TimeOfDay(World))));
	}
	// The stream that started today, or the one from yesterday still running past midnight.
	for (int Back = 0; Back <= 1; ++Back)
	{
		const int D = Today - Back;
		if (!((S.Days >> (((D % 7) + 7) % 7)) & 1))
		{
			continue;
		}
		const double Since = Minute + static_cast<double>(Back) * KastDay - static_cast<double>(S.Opens);
		const double Length = static_cast<double>(S.Hours) * 60.0;
		if (Since < 0.0 || Since >= Length)
		{
			continue;
		}
		const double Wobble = 1.0 + 0.04 * std::sin(World / 7.0 + static_cast<double>(S.Followers % 97));
		const double Ramp = std::min(1.0, std::min(Since / 25.0, (Length - Since) / 20.0) * 0.7 + 0.3);
		const double Night = 0.85 + 0.3 * static_cast<double>(HashOf(D) % 100) / 100.0;
		return std::max(1, static_cast<int>(std::round(static_cast<double>(S.Viewers) * Ramp * Night * Wobble)));
	}
	return 0;
}

/** ...with World's minute of the day and day worked out here. */
template <typename HashFn>
int DirViewersNow(const Streamer& S, double World, HashFn&& HashOf)
{
	return DirViewersAt(S, World, std::fmod(World, KastDay), static_cast<int>(std::floor(World / KastDay)), HashOf);
}
} // namespace kast_detail

int ViewersNow(const Streamer& S, double World)
{
	return kast_detail::DirViewersNow(S, World, [&S](int D) { return kast_detail::NameHash(S.Name + std::to_string(D)); });
}

namespace kast_sponsor_detail
{
/** The stream's view of a sponsor catalog brand: its name on the overlay, its read, its gate, its rates. */
Sponsor ViewOf(const sponsor::Brand& B)
{
	Sponsor S;
	S.Id = B.Id;
	S.Brand = B.Kast.empty() ? B.Short : B.Kast;
	S.Product = B.Product;
	// The offer letter's first wording, without the line about why they're calling (offers come by Mail now).
	std::string Pitch = sponsor::sponsor_copy::Text(sponsor::sponsor_copy::Body, B.Pitch.empty() ? std::string() : B.Pitch.front());
	const size_t Why = Pitch.find("{Why} ");
	if (Why != std::string::npos)
	{
		Pitch.erase(Why, 6);
	}
	S.Pitch = Pitch;
	S.ReadLine = B.ReadLine;
	S.Color = B.Color;
	S.Followers = B.Needs.Followers;
	S.AvgViewers = B.Needs.AvgViewers;
	S.NeedsPartner = B.Needs.Partner;
	S.PerHourCents = B.Offer.PerLiveHourCents;
	S.ReadCents = B.Offer.PerReadCents;
	S.Days = B.Offer.Days;
	return S;
}
} // namespace kast_sponsor_detail

const std::vector<Sponsor>& Sponsors()
{
	// The five Kast has carried from the start, filtered from the sponsor catalog in its order (§3.8: the ids are in old
	// saves, and the world's streamers draw from exactly these until the world reads the catalog by tier).
	static const std::vector<Sponsor> L = [] {
		std::vector<Sponsor> V;
		for (const sponsor::Brand& B : sponsor::Brands())
		{
			if (B.Legacy)
			{
				V.push_back(kast_sponsor_detail::ViewOf(B));
			}
		}
		return V;
	}();
	return L;
}

const Sponsor* FindSponsor(const std::string& Id)
{
	// Any brand that pays on stream (the legacy five, Halo, and whatever the catalog adds).
	static const std::vector<Sponsor> All = [] {
		std::vector<Sponsor> V;
		for (const sponsor::Brand& B : sponsor::Brands())
		{
			if (B.Offer.PerLiveHourCents > 0 || B.Offer.PerReadCents > 0)
			{
				V.push_back(kast_sponsor_detail::ViewOf(B));
			}
		}
		return V;
	}();
	for (const Sponsor& S : All)
	{
		if (S.Id == Id)
		{
			return &S;
		}
	}
	return nullptr;
}

const std::vector<TitleSpec>& Titles()
{
	static const std::vector<TitleSpec> L = {
		{"Grinding the micros to make rent \xC2\xB7 !rent", "Story", 1.0, 1.15, 1.0, 1.0},
		{"Deep run or bust \xC2\xB7 all-ins all night", "Action", 1.15, 0.95, 1.35, 1.25},
		{"Chill grind & questions \xC2\xB7 beginner friendly", "Chill", 0.9, 1.2, 0.85, 0.7},
		{"ICM explained live \xC2\xB7 learn MTTs with me", "Teaching", 0.95, 1.3, 0.8, 0.8},
		{"Four tables, no sleep \xC2\xB7 !gear", "Grind", 1.1, 1.0, 1.1, 1.0},
	};
	return L;
}

const char* MomentName(Moment M)
{
	const int I = static_cast<int>(M);
	return I >= 0 && I < static_cast<int>(Moment::Count) ? kast_detail::MomentNames[I] : "?";
}

int EmoteOf(const std::string& Word, const std::string& Prefix)
{
	static const char* const Kast[] = {"kastPog", "kastLUL", "kastGG", "kastRIP", "kastHype", "kastLove", "kastFish", "kastSalt", "kastClap", "kastChip"};
	static const char* const Own[] = {"Ship", "Tilt", "Rent"};
	if (Word.size() < 4)
	{
		return -1;
	}
	for (int I = 0; I < 10; ++I)
	{
		if (Word == Kast[I])
		{
			return I;
		}
	}
	if (!Prefix.empty() && Word.compare(0, Prefix.size(), Prefix) == 0)
	{
		for (int I = 0; I < 3; ++I)
		{
			if (Word.compare(Prefix.size(), std::string::npos, Own[I]) == 0)
			{
				return 10 + I;
			}
		}
	}
	return -1;
}

std::string EmotePrefix(const std::string& Hero)
{
	std::string P;
	for (const char Ch : Hero)
	{
		if (Ch >= 'a' && Ch <= 'z')
		{
			P += Ch;
		}
		else if (Ch >= 'A' && Ch <= 'Z')
		{
			P += static_cast<char>(Ch - 'A' + 'a');
		}
		if (P.size() >= 5)
		{
			break;
		}
	}
	return P.size() >= 3 ? P : P + std::string("grind").substr(0, 5 - P.size());
}

// ------------------------------------------------------------------ channel

int Channel::ActiveSubs(double World) const
{
	int N = 0;
	for (const Subscriber& S : Subs)
	{
		N += S.Renews > World ? 1 : 0;
	}
	return N;
}

double Channel::AvgViewers() const
{
	double Minutes = 0.0;
	double Sum = 0.0;
	for (size_t I = 0; I < Log.size() && I < 10; ++I)
	{
		Minutes += Log[I].Minutes;
		Sum += static_cast<double>(Log[I].Avg) * Log[I].Minutes;
	}
	return Minutes > 0.0 ? Sum / Minutes : 0.0;
}

void Channel::Window30(double World, double& Minutes, int& LiveDays, double& Avg) const
{
	Minutes = 0.0;
	double ViewerMins = 0.0;
	for (const StreamLog& L : Log)
	{
		if (L.Start >= World - 30.0 * kast_detail::KastDay)
		{
			Minutes += L.Minutes;
			ViewerMins += static_cast<double>(L.Avg) * L.Minutes;
		}
	}
	LiveDays = 0;
	const int Today = kast_detail::KastDayOf(World);
	for (const int D : DaysLive)
	{
		LiveDays += D > Today - 30 ? 1 : 0;
	}
	Avg = Minutes > 0.0 ? ViewerMins / Minutes : 0.0;
}

int Channel::Regulars() const
{
	int N = 0;
	for (const Member& M : Members)
	{
		N += M.Loyalty >= RegularLoyalty && !M.Friend ? 1 : 0;
	}
	return N;
}

int Channel::Superfans() const
{
	int N = 0;
	for (const Member& M : Members)
	{
		N += M.Loyalty >= SuperfanLoyalty && !M.Friend ? 1 : 0;
	}
	return N;
}

double Channel::LevelXp(int Lv)
{
	return Lv <= 1 ? 0.0 : 120.0 * std::pow(static_cast<double>(Lv - 1), 1.7);
}

int Channel::Level() const
{
	int Lv = 1;
	while (Lv < MaxLevel && Xp >= LevelXp(Lv + 1))
	{
		++Lv;
	}
	return Lv;
}

double Channel::Discoverability() const
{
	return (1.0 + 0.05 * static_cast<double>(Level() - 1)) * (1.0 + 0.05 * static_cast<double>(std::min(Streak, 8)));
}

bool Channel::OnSchedule(double World) const
{
	if (ScheduleDays == 0)
	{
		return false;
	}
	const double Minute = std::fmod(World, kast_detail::KastDay);
	const int Today = kast_detail::KastDayOf(World);
	// Today's slot, or yesterday's running past midnight: within an hour before to an hour and a half after.
	for (int Back = 0; Back <= 1; ++Back)
	{
		const int D = Today - Back;
		if (!((ScheduleDays >> kast_detail::KastWeekday(D)) & 1))
		{
			continue;
		}
		const double Since = Minute + static_cast<double>(Back) * kast_detail::KastDay - static_cast<double>(ScheduleStart);
		if (Since >= -60.0 && Since <= 90.0)
		{
			return true;
		}
	}
	return false;
}

const Member* Channel::FindMember(const std::string& Name) const
{
	for (const Member& M : Members)
	{
		if (M.Name == Name)
		{
			return &M;
		}
	}
	return nullptr;
}

Member* Channel::FindMember(const std::string& Name)
{
	for (Member& M : Members)
	{
		if (M.Name == Name)
		{
			return &M;
		}
	}
	return nullptr;
}

bool Channel::IsMod(const std::string& Name) const
{
	for (const Moderator& M : Mods)
	{
		if (M.Name == Name)
		{
			return true;
		}
	}
	return false;
}

bool Channel::IsSub(const std::string& Name, double World) const
{
	for (const Subscriber& S : Subs)
	{
		if (S.Name == Name && S.Renews > World)
		{
			return true;
		}
	}
	return false;
}

const Deal* Channel::ActiveDeal(const std::string& Id, double World) const
{
	for (const Deal& D : Deals)
	{
		if (D.Id == Id && D.Until > World)
		{
			return &D;
		}
	}
	return nullptr;
}

int Tier(const Channel& Ch)
{
	return Ch.Partner ? 2 : Ch.Affiliate ? 1 : 0;
}

const std::vector<StageSpec>& Stages()
{
	static const std::vector<StageSpec> L = {
		{"First stream", "Go live for the first time. Dee's in the front row.", "The studio and chat", 1, 0, 0, 0, 0},
		{"Familiar faces", "The same names start showing up night after night.", "Promote regulars to moderator", 5, 3, 0, 0, 0},
		{"Affiliate", "Kast's first tier, over the last 30 days: 25 followers, 240 minutes and 4 days live, 3 average viewers.", "Subs, cheers, ad breaks and channel emotes", 0, 0, 0, AffiliateFollowersC8, 1},
		{"Small community", "A chat that talks to itself before you say a word.", "Regulars bring their friends twice as often", 30, 10, 8, 150, 1},
		{"Growing channel", "People plan their night around your stream.", "Sponsors start paying attention", 60, 25, 20, 500, 1},
		{"Partner", "Kast's Partner Program, over the last 30 days: 75 average viewers, 25 hours and 12 days live.", "Verified, 70% of every sub, better ad rates", 0, 0, 75, 0, 2},
		{"Established", "A name in the Poker category.", "The big channels raid your deep runs", 150, 120, 150, 5000, 2},
		{"Poker personality", "Chat follows you to every table.", "The biggest sponsors want you", 250, 400, 600, 25000, 2},
	};
	return L;
}

namespace kast_detail
{
/** The furthest stage reached, in order (a stage counts once every stage before it is done). */
int StageReached(const Channel& Ch, double World, double LiveMinutes, double LiveViewerMinutes)
{
	double Minutes = 0.0;
	int LiveDays = 0;
	double Avg = 0.0;
	Ch.Window30(World, Minutes, LiveDays, Avg);
	if (LiveMinutes > 1.0)
	{
		Avg = (Avg * Minutes + LiveViewerMinutes) / (Minutes + LiveMinutes);
	}
	const int Regs = Ch.Regulars();
	int Reached = -1;
	const std::vector<StageSpec>& L = Stages();
	for (size_t I = 0; I < L.size(); ++I)
	{
		const StageSpec& S = L[I];
		const bool Ok = Ch.Streams >= S.Streams && Regs >= S.Regulars && Avg >= static_cast<double>(S.AvgViewers) && Ch.Followers >= S.Followers && Tier(Ch) >= S.Tier;
		if (!Ok)
		{
			break;
		}
		Reached = static_cast<int>(I);
	}
	return Reached;
}

/**
 * Whether StageReached could pass Ch.Stage now: every stage up to the next one passes the terms other than the regulars (a
 * necessary condition, read before StageReached counts the community: the Room's minute did that walk over 1,500 members once a
 * grid minute, C-K's M2).
 */
bool StagePastReachable(const Channel& Ch, double World, double LiveMinutes, double LiveViewerMinutes)
{
	const std::vector<StageSpec>& L = Stages();
	const size_t Next = static_cast<size_t>(std::max(0, Ch.Stage + 1));
	if (Next >= L.size())
	{
		return false;
	}
	double Minutes = 0.0;
	int LiveDays = 0;
	double Avg = 0.0;
	Ch.Window30(World, Minutes, LiveDays, Avg);
	if (LiveMinutes > 1.0)
	{
		Avg = (Avg * Minutes + LiveViewerMinutes) / (Minutes + LiveMinutes);
	}
	for (size_t I = 0; I <= Next; ++I)
	{
		const StageSpec& S = L[I];
		if (!(Ch.Streams >= S.Streams && Avg >= static_cast<double>(S.AvgViewers) && Ch.Followers >= S.Followers && Tier(Ch) >= S.Tier))
		{
			return false;
		}
	}
	return true;
}
} // namespace kast_detail

std::vector<Goal> StageGoals(const Channel& Ch, double World, int Stage)
{
	std::vector<Goal> Out;
	const std::vector<StageSpec>& L = Stages();
	if (Stage < 0 || Stage >= static_cast<int>(L.size()))
	{
		return Out;
	}
	const StageSpec& S = L[static_cast<size_t>(Stage)];
	double Minutes = 0.0;
	int LiveDays = 0;
	double Avg = 0.0;
	Ch.Window30(World, Minutes, LiveDays, Avg);
	if (S.Tier == 1)
	{
		// The creator update's bar (SPEC_CREATORS C8: the real platform's current one).
		Out.push_back({"Followers", static_cast<double>(Ch.Followers), static_cast<double>(AffiliateFollowersC8), 0});
		Out.push_back({"Minutes live (30 days)", Minutes, AffiliateMinutesC8, 0});
		Out.push_back({"Days live (30 days)", static_cast<double>(LiveDays), static_cast<double>(AffiliateDaysC8), 0});
		Out.push_back({"Average viewers (30 days)", Avg, AffiliateAvg, 0});
		return Out;
	}
	if (S.Tier == 2 && S.Streams == 0)
	{
		// Partner's average leaves raiders and the Broadcast out (SPEC_CREATORS C8, §2.10.3).
		Out.push_back({"Hours live (30 days)", Minutes / 60.0, PartnerMinutes / 60.0, 0});
		Out.push_back({"Days live (30 days)", static_cast<double>(LiveDays), static_cast<double>(PartnerDays), 0});
		Out.push_back({"Average viewers (30 days)", kast_detail::KastAvgOwn30(Ch, World), PartnerAvg, 0});
		return Out;
	}
	if (S.Streams > 0)
	{
		Out.push_back({"Streams", static_cast<double>(Ch.Streams), static_cast<double>(S.Streams), 0});
	}
	if (S.Regulars > 0)
	{
		Out.push_back({"Regulars", static_cast<double>(Ch.Regulars()), static_cast<double>(S.Regulars), 0});
	}
	if (S.AvgViewers > 0)
	{
		Out.push_back({"Average viewers (30 days)", Avg, static_cast<double>(S.AvgViewers), 0});
	}
	if (S.Followers > 0)
	{
		Out.push_back({"Followers", static_cast<double>(Ch.Followers), static_cast<double>(S.Followers), 0});
	}
	return Out;
}

namespace kast_detail
{
/** This week's four goals (WeekGoals' table): the label, what it takes, the XP it pays. */
struct WeekGoalSpec
{
	const char* Label;
	double Need;
	int Xp;
};
constexpr WeekGoalSpec WeekGoalSpecs[] = {{"Stream 3 times", 3.0, 100}, {"6 hours live", 6.0, 100}, {"Answer 5 questions", 5.0, 60}, {"2 new regulars", 2.0, 80}};
constexpr size_t WeekGoalCount = sizeof(WeekGoalSpecs) / sizeof(WeekGoalSpecs[0]);

/** How far along goal I is (WeekGoals' Have; the Room's minute reads it without building the four Goals, C-K's M2). */
double WeekGoalHave(const Channel& Ch, double World, size_t I)
{
	if (Ch.Week != KastWeekOf(World))
	{
		return 0.0;
	}
	switch (I)
	{
	case 0: return static_cast<double>(Ch.WeekStreams);
	case 1: return Ch.WeekMinutes / 60.0;
	case 2: return static_cast<double>(Ch.WeekAnswers);
	default: return static_cast<double>(Ch.WeekRegulars);
	}
}
} // namespace kast_detail

std::vector<Goal> WeekGoals(const Channel& Ch, double World)
{
	std::vector<Goal> Out;
	for (size_t I = 0; I < kast_detail::WeekGoalCount; ++I)
	{
		const kast_detail::WeekGoalSpec& G = kast_detail::WeekGoalSpecs[I];
		Out.push_back({G.Label, kast_detail::WeekGoalHave(Ch, World, I), G.Need, G.Xp});
	}
	return Out;
}

namespace kast_detail
{
/** The network's 36 small channels (procedural, from one fixed seed): Network's table, and the Room's rank (C-K's M2). */
struct NetSpec
{
	std::string Name;
	std::string Title;
	int Viewers;
	int Opens;
	int Hours;
	int Days;
	uint32_t Color;
};
const std::vector<NetSpec>& NetworkSpecs()
{
	static const std::vector<NetSpec> All = [] {
		static const char* const SmallTitles[] = {"micro stakes grind, come hang", "road to $1k bankroll", "learning MTTs live", "late night turbos", "chill freerolls + music",
			"PKO practice", "first final table?? (day 12)", "grinding with chat", "spin & fold", "the 3am crew"};
		static const uint32_t Colors[] = {0xf97316, 0x22c55e, 0x38bdf8, 0xe879f9, 0xfacc15, 0x14b8a6, 0xef4444, 0xa78bfa};
		Rng R("kast-network");
		std::vector<NetSpec> V;
		std::set<std::string> Taken;
		while (V.size() < 36)
		{
			NetSpec S;
			S.Name = handles::Make(R, handles::PickCountry(R));
			if (!Taken.insert(S.Name).second)
			{
				continue;
			}
			S.Title = SmallTitles[R.Int(10)];
			S.Viewers = 2 + static_cast<int>(std::pow(R.Next(), 2.2) * 55.0);
			S.Opens = (17 + R.Int(10)) % 24 * 60 + R.Int(4) * 15;
			S.Hours = 2 + R.Int(5);
			int Mask = 0;
			for (int D = 0; D < 7; ++D)
			{
				Mask |= R.Chance(0.65) ? 1 << D : 0;
			}
			S.Days = Mask == 0 ? 0x1f : Mask;
			S.Color = Colors[R.Int(8)];
			V.push_back(S);
		}
		return V;
	}();
	return All;
}

/** A network channel's night (its viewers on world day D): 0.7-1.3x its usual, from its name and the day. */
double NetNight(uint32_t Hash)
{
	return 0.7 + 0.6 * static_cast<double>(Hash % 100) / 100.0;
}

/** Which of its two possible streams a network channel is in at World's minute of the day on day Today (0 today's, 1
 *  yesterday's past midnight; -1 none). */
int NetLiveBackAt(const NetSpec& S, double Minute, int Today)
{
	for (int Back = 0; Back <= 1; ++Back)
	{
		const int D = Today - Back;
		const double Since = Minute + static_cast<double>(Back) * KastDay - static_cast<double>(S.Opens);
		if (((S.Days >> KastWeekday(D)) & 1) && Since >= 0.0 && Since < static_cast<double>(S.Hours) * 60.0)
		{
			return Back;
		}
	}
	return -1;
}

int NetLiveBack(const NetSpec& S, double World)
{
	return NetLiveBackAt(S, std::fmod(World, KastDay), KastDayOf(World));
}
} // namespace kast_detail

std::vector<SmallChannel> Network(double World)
{
	std::vector<SmallChannel> Live;
	const int Today = kast_detail::KastDayOf(World);
	for (const kast_detail::NetSpec& S : kast_detail::NetworkSpecs())
	{
		const int Back = kast_detail::NetLiveBack(S, World);
		if (Back >= 0)
		{
			const double Night = kast_detail::NetNight(kast_detail::NameHash(S.Name + std::to_string(Today - Back)));
			Live.push_back({S.Name, S.Title, std::max(1, static_cast<int>(std::round(static_cast<double>(S.Viewers) * Night))), S.Color});
		}
	}
	std::stable_sort(Live.begin(), Live.end(), [](const SmallChannel& A, const SmallChannel& B) { return A.Viewers > B.Viewers; });
	return Live;
}

int ChannelsLive(double World)
{
	// C-K's L11: Browse's "N channels live" counts the network with the directory (growth::RankAt's field, without you).
	return growth::RankAt(World, 0).Of - 1;
}

namespace kast_offline_detail
{
/** A clip's views at a world minute: they settle over a day and a half (closed form). */
double OfflineClipViews(const Clip& C, double World)
{
	return World <= C.At ? 0.0 : C.Reach * (1.0 - std::exp(-(World - C.At) / (1.5 * kast_detail::KastDay)));
}

/**
 * The renewals due before Until (or at it, when AtUntil), each on its own keyed draw (SPEC_CREATORS §8.4: Unit(Seed, 0, Renew,
 * NameKey, month)), so their order across the subs never matters. Affiliate revenue earned before the second rent waits in
 * PendingCents (C14). The loyalty read is the member's as the days have left it, so a skip of N days and N one-day steps read
 * the same one.
 */
void OfflineRenew(Channel& Ch, double Until, bool AtUntil, uint64_t Seed, int RentsPaid)
{
	const double Share = Ch.Partner ? 0.7 : 0.5;
	for (Subscriber& S : Ch.Subs)
	{
		while (S.Renews > 0.0 && (S.Renews < Until || (AtUntil && S.Renews <= Until)))
		{
			const Member* M = Ch.FindMember(S.Name);
			const double L = M ? M->Loyalty : 0.15;
			const double U = growth::Unit(Seed, 0, growth::Tag::Renew, growth::NameKey(S.Name), static_cast<uint64_t>(std::max(0, S.Months)));
			if (U >= growth::RenewChance(L, S.Gift))
			{
				S.Renews = -S.Renews; // lapsed (kept for the history, negative so it never renews)
				break;
			}
			++S.Months;
			S.Gift = false;
			S.Renews += 30.0 * kast_detail::KastDay;
			const Chips Cents = static_cast<Chips>(std::round(static_cast<double>(SubPriceCents) * Share));
			Ch.EarnedSubs += Cents;
			(RentsPaid < 2 ? Ch.PendingCents : Ch.UnpaidCents) += Cents;
		}
	}
}

/** One whole world day gone by: the members drift (more when a posted stream doesn't happen) and the ones who drifted all the
 *  way off are forgotten as of the day's end (followers stay followers: they just never come). */
void OfflineDrift(Channel& Ch, int D)
{
	bool Streamed = false;
	bool Recent = false;
	for (const int Live : Ch.DaysLive)
	{
		Streamed = Streamed || Live == D;
		Recent = Recent || (Live >= D - 2 && Live <= D);
	}
	double F = Recent ? 0.996 : 0.985;
	if (((Ch.ScheduleDays >> kast_detail::KastWeekday(D)) & 1) && !Streamed)
	{
		F *= 0.97; // they showed up and nobody was there
	}
	for (Member& M : Ch.Members)
	{
		M.Loyalty = M.Friend ? std::max(0.6, M.Loyalty * F) : M.Loyalty * F;
	}
	const double End = static_cast<double>(D + 1) * kast_detail::KastDay;
	Ch.Members.erase(std::remove_if(Ch.Members.begin(), Ch.Members.end(),
						 [&](const Member& M) { return !M.Friend && M.Loyalty < 0.012 && M.LastSeen < End - 45.0 * kast_detail::KastDay && !Ch.IsSub(M.Name, End) && !Ch.IsMod(M.Name); }),
		Ch.Members.end());
}

/**
 * One whole world day of clip views (to its end): a few of the new viewers follow, 0.15% x Sat (SPEC_CREATORS §2.7.2). A clip
 * whose moment went up on Rerun (Candidate set when the video was published) stops where it was: its watchers are the
 * video's now, and the video brings its own follows (update 2 §4.7: no double count). A clip follower is a member with a
 * keyed name and a keyed Affinity (§8.4: Unit(Seed, 0, ClipFollower, n) and Unit(Seed, 0, Affinity, NameKey)).
 */
void OfflineClipFollows(Channel& Ch, int D, uint64_t Seed)
{
	const double End = static_cast<double>(D + 1) * kast_detail::KastDay;
	double Gained = 0.0;
	for (const Clip& C : Ch.Clips)
	{
		if (C.Candidate.empty() && C.At < End)
		{
			Gained += std::max(0.0, OfflineClipViews(C, End) - OfflineClipViews(C, End - kast_detail::KastDay));
		}
	}
	Ch.FollowFrac += Gained * 0.0015 * growth::Sat(Ch.Followers);
	while (Ch.FollowFrac >= 1.0)
	{
		Ch.FollowFrac -= 1.0;
		++Ch.Followers;
		if (static_cast<int>(Ch.Members.size()) >= MaxMembers)
		{
			continue;
		}
		const uint64_t N = static_cast<uint64_t>(Ch.Followers);
		Rng Names(std::to_string(Seed) + ":kast:clipfollow:" + std::to_string(N) + ":" +
			std::to_string(static_cast<uint64_t>(growth::Unit(Seed, 0, growth::Tag::ClipFollower, N) * 4294967296.0)));
		Member M;
		M.Name = handles::Make(Names, handles::PickCountry(Names));
		M.Loyalty = 0.03;
		M.Affinity = 0.05 + 0.6 * std::pow(growth::Unit(Seed, 0, growth::Tag::Affinity, growth::NameKey(M.Name)), 4.0);
		M.FirstSeen = End;
		M.LastSeen = End;
		M.Follower = true;
		if (!Ch.FindMember(M.Name))
		{
			Ch.Members.push_back(M);
		}
	}
}
} // namespace kast_offline_detail

void Offline(Channel& Ch, double World, Rng& /*R*/, uint64_t Seed, int RentsPaid)
{
	// The creator update (SPEC_CREATORS §8.4, C14): nothing here draws from R. Renewals and clip followers are keyed on Seed,
	// and the whole days since the channel last turned over run in order (that day's renewals, its drift, its forgetting, its
	// clip follows), so a skip of N days is N one-day steps to the byte. Before the second rent (RentsPaid < 2) the renewals'
	// money waits in PendingCents.
	if (World <= Ch.LastOffline)
	{
		return;
	}
	Ch.LastOffline = World;
	kast_detail::RollWeek(Ch, World);
	const int Today = kast_detail::KastDayOf(World);
	if (Ch.ProcessedDay < 0)
	{
		Ch.ProcessedDay = Today;
	}
	if (Ch.ProcessedDay < Today)
	{
		const int From = std::max(Ch.ProcessedDay, Today - 120);
		kast_offline_detail::OfflineRenew(Ch, static_cast<double>(From) * kast_detail::KastDay, false, Seed, RentsPaid);
		for (int D = From; D < Today; ++D)
		{
			kast_offline_detail::OfflineRenew(Ch, static_cast<double>(D + 1) * kast_detail::KastDay, false, Seed, RentsPaid);
			kast_offline_detail::OfflineDrift(Ch, D);
			kast_offline_detail::OfflineClipFollows(Ch, D, Seed);
		}
		Ch.ProcessedDay = Today;
	}
	// Today's renewals so far.
	kast_offline_detail::OfflineRenew(Ch, World, true, Seed, RentsPaid);
	Ch.Subs.erase(std::remove_if(Ch.Subs.begin(), Ch.Subs.end(), [&](const Subscriber& S) { return S.Renews < 0.0 && -S.Renews < World - 30.0 * kast_detail::KastDay; }), Ch.Subs.end());
	// The views the channel's pages show (closed form; the follows above count by whole days).
	for (Clip& C : Ch.Clips)
	{
		if (C.Candidate.empty())
		{
			C.Views = kast_offline_detail::OfflineClipViews(C, World);
		}
	}
	// (Deals run out in the sponsor book now: Kast works on its view, next_segment_spec §3.8.)
}

// The creator update (SPEC_CREATORS §8.1): kast\tmo's view of the Channel, in the line's field order. core-kastroom's; the
// Session reaches these only once life::U3Kast is set (Phase A-K: never).

growth::SaveView SaveViewOf(const Channel& Ch)
{
	growth::SaveView V;
	V.Heat = Ch.Heat;
	V.HeatAt = Ch.HeatAt;
	V.Norm = Ch.Norm;
	V.Bias = Ch.Bias;
	V.RunNorm = Ch.RunNorm;
	V.BestAvg = Ch.BestAvg;
	V.PingAt = Ch.PingAt;
	V.LastStreamDay = Ch.LastStreamDay;
	V.ShowWeek = Ch.ShowWeek;
	V.ShowDays = Ch.ShowDays;
	V.GainWeek = Ch.GainWeek;
	V.GainUsed = Ch.GainUsed;
	V.FtBase = Ch.FtBase;
	V.FtDays = Ch.FtDays;
	V.LiveDay = Ch.LiveDay;
	V.LiveMin = Ch.LiveMin;
	V.GrantDay = Ch.GrantDay;
	V.GrantMask = Ch.GrantMask;
	V.WelcomeRaidDone = Ch.WelcomeRaidDone;
	V.CotnDay = Ch.CotnDay;
	V.PendingCents = Ch.PendingCents;
	V.LiveSince = Ch.LiveSince;
	V.GainSoFar = Ch.GainSoFar;
	V.CarryFollow = Ch.CarryFollow;
	V.CarryWord = Ch.CarryWord;
	V.CarrySub = Ch.CarrySub;
	V.CarryTip = Ch.CarryTip;
	V.CarryCheer = Ch.CarryCheer;
	V.CarryRaid = Ch.CarryRaid;
	V.CarryGift = Ch.CarryGift;
	V.RerunHeat = Ch.RerunHeat;
	V.Firsts = Ch.Firsts;
	return V;
}

void ApplySaveView(Channel& Ch, const growth::SaveView& V)
{
	Ch.Heat = V.Heat;
	Ch.HeatAt = V.HeatAt;
	Ch.Norm = V.Norm;
	Ch.Bias = V.Bias;
	Ch.RunNorm = V.RunNorm;
	Ch.BestAvg = V.BestAvg;
	Ch.PingAt = V.PingAt;
	Ch.LastStreamDay = V.LastStreamDay;
	Ch.ShowWeek = V.ShowWeek;
	Ch.ShowDays = V.ShowDays;
	Ch.GainWeek = V.GainWeek;
	Ch.GainUsed = V.GainUsed;
	Ch.FtBase = V.FtBase;
	Ch.FtDays = V.FtDays;
	Ch.LiveDay = V.LiveDay;
	Ch.LiveMin = V.LiveMin;
	Ch.GrantDay = V.GrantDay;
	Ch.GrantMask = V.GrantMask;
	Ch.WelcomeRaidDone = V.WelcomeRaidDone;
	Ch.CotnDay = V.CotnDay;
	Ch.PendingCents = V.PendingCents;
	Ch.LiveSince = V.LiveSince;
	Ch.GainSoFar = V.GainSoFar;
	Ch.CarryFollow = V.CarryFollow;
	Ch.CarryWord = V.CarryWord;
	Ch.CarrySub = V.CarrySub;
	Ch.CarryTip = V.CarryTip;
	Ch.CarryCheer = V.CarryCheer;
	Ch.CarryRaid = V.CarryRaid;
	Ch.CarryGift = V.CarryGift;
	Ch.RerunHeat = V.RerunHeat;
	Ch.Firsts = V.Firsts;
}

// ------------------------------------------------------------------ stream

namespace kast_room_detail
{
// The Room's constants (SPEC_CREATORS §1-§2; growth:: holds the model's formulas, these are the Room's own clock and caps).
constexpr double RoomInf = 1.0e18;
constexpr double RoomSlotLen = 10.0;    // drop-in presence runs on 10-world-minute slots (§2.3.4)
constexpr int RoomGuestCap = 96;        // named guests present at once; beyond them the arrivals are Crowd (§2.5.3)
constexpr int RoomRaidNamed = 24;       // named raiders a raid; the rest of the party is crowd (§2.7.1)
constexpr size_t RoomPoolSize = 6000;   // the guest pool, built once from the world seed (§2.5.3)
constexpr double RoomPingGap = 240.0;   // one ping per 4 world hours (§2.3.1)
constexpr double RoomLateShare = 0.3;   // latecomers: Pa x 0.3 once a world hour (§2.3.2)...
constexpr double RoomLateMean = 15.0;   // ...arriving Exp(15) into it...
constexpr double RoomLateUntil = 600.0; // ...and none after 10 world hours of uptime
constexpr double RoomEngageAttended = 0.20; // the passive floor at attended paces: you talk through hands (§2.9)
constexpr double RoomEngageWatch4 = 0.12;   // watch 4x leaves fewer real seconds (§1.4)
constexpr double RoomEngageHeadsDown = 0.05; // 8x, a skip, a sprint: heads-down
constexpr double RoomHypeTau = 12.0;
// The room's hype (S-K, 2026-10-24): what the strangers' rate, their stay and the giving read is synth_kast.py's hype, a resting
// 14 (routine all-ins and pots) plus the run's stage hits on the room clock (bubble 10, ITM 16, final table 26, top 3 12, win
// 45; half on a small field), decaying with RoomHypeTau. The meter (Stream::Hype) is the chat's and the LEDs': with two to four
// tables its routine moments held it near 25-45, which ran the room x1.2-1.4 hot against every band.
constexpr double RoomHypeBase = 14.0;
constexpr double RoomStageHits[] = {0.0, 10.0, 16.0, 26.0, 12.0, 45.0}; // by growth::RunStage
constexpr double RoomSmartReal = 13.1;  // real seconds a world minute at 1x Smart: the old real-second rates on the world clock
constexpr double RoomHealthMinute = 0.03 * RoomSmartReal / 60.0;
constexpr double RoomRailStay = 25.0;   // railbirds stay 25 world minutes and follow at 0.8x (§2.6)
constexpr double RoomRailFollow = 0.8;
constexpr double RoomWordFollow = 0.35; // a word-of-mouth guest follows at 0.35 x Sat (§2.8.3)
constexpr double RoomRaidFollow = 0.04; // a raider follows at 0.04 x Sat (x1.5 welcomed, §2.7.1)
constexpr double RoomAnswerFollow = 0.15; // an answered guest follows at 0.15 x Sat (§2.9)
constexpr double RoomGiveWindow = 6.0;  // per-visit giving bunches in the 6 world minutes after a big moment the member was here for
constexpr double RoomTipGate = 20.0;    // a visit gives once it has 20 world minutes watched (a new sub 30)
constexpr double RoomSubGate = 30.0;
constexpr double RoomTipStranger = 0.0006; // strangers' and lurkers' giving, an hour each (§2.10.1)
constexpr double RoomCheerStranger = 0.001;
constexpr double RoomSubStranger = 0.0008;
constexpr double RoomCarryCap = 3.0;    // a carried fraction never piles past three of a thing
constexpr double RoomAdWindow = 6.9;    // an ad's room effect: today's 90 real seconds at Smart, as world minutes (§2.5.1)
constexpr double RoomAdRoom = 0.6;
constexpr double RoomClipTopBig = 0.02; // a Clip Lane clip tops the feed: 2% for a win, a bad beat, a final table (100+)...
constexpr double RoomClipTop = 0.012;   // ...1.2% otherwise, at x40 reach and Heat +12 (§2.7.2)
constexpr double RoomClipTopX = 40.0;
constexpr double RoomHeatClip = 12.0;   // Heat's bumps (§2.4)
constexpr double RoomHeatRecord = 4.0;
constexpr double RoomHeatMilestone = 2.0;
constexpr double RoomHeatRaid = 3.0;
constexpr double RoomFrontLen = 20.0;   // Front Row: a 20-world-minute window, strangers x2.5
constexpr double RoomFrontX = 2.5;
constexpr int RoomWelcomeRaidMinute = 40; // the welcome raid (§2.7.1): the first stream from stream 3 that reaches minute 40
constexpr int RoomWelcomeRaidSize = 8;
constexpr uint64_t RoomWelcomeRaidFrom = 3;
constexpr double RoomShort = 60.0;      // a stream under 60 world minutes updates no statistic, streak, token or log row (§7)
constexpr double RoomTimeoutWorld = 600.0 / RoomSmartReal; // a 10-real-minute timeout keeps the name out this long on the grid
constexpr double RoomTrollStay = 4.0;   // a troll who isn't caught hangs around Exp(4) world minutes
constexpr double RoomFollowLag = 10.0;  // a guest who follows does so within min(half their stay, 10) world minutes
constexpr double RoomDirCtr = 0.06;     // R6's impressions: arrivals / (0.06 x the title's pull) (§5.3 K2)
constexpr double RoomCountUp = 0.3;     // ShownCount's count-up, real seconds (§1.4)
constexpr double RoomChattingWindow = 120.0; // "N chatting": distinct speakers in the last 2 real minutes, still here (C12)
constexpr double RoomEmptyLineGap = 180.0;   // R6: at most one empty-room line every 3 real minutes
constexpr double RoomSourceLineShare = 0.35;  // K8: a guest from Rerun or a clip says so on 35% of arrivals (C-K's L5)...
constexpr uint64_t RoomSourceLineKey = 7000;  // ...on Unit(..., GuestName, minute, 7000 + j), a key no other draw uses

/** The book's deals onto Kast's own copy (C13): a read Kast paid since the book last looked keeps its time and count, so two
 *  reads inside one sync can't both pay ("once a deal a world day" never rests on the caller's timing). */
void RoomTakeDeals(std::vector<Deal>& Mine, const std::vector<Deal>& Book)
{
	std::vector<Deal> Out = Book;
	for (Deal& D : Out)
	{
		for (const Deal& M : Mine)
		{
			if (M.Id == D.Id && M.ReadAt > D.ReadAt)
			{
				D.ReadAt = M.ReadAt;
				D.ReadsPaid = std::max(D.ReadsPaid, M.ReadsPaid);
				D.EarnedCents = std::max(D.EarnedCents, M.EarnedCents);
			}
		}
	}
	Mine.swap(Out);
}

/**
 * The day H7 reads up to at a go-live (§2.2, §2.8.4): the 7 world days before now. With nothing live yet today that is
 * yesterday and the 6 before it (the whole ring); once today has live minutes, today and the 6 before. S-K (2026-10-24): read
 * up to today, a nightly streamer's H7 held one night short (6 of 7), and a grinder's Stamina sat at 0.7 against the model's 0.6.
 */
int RoomH7Day(const Channel& Ch, int Today)
{
	return Ch.LiveDay >= Today ? Today : Today - 1;
}

/** The end of the 10-world-minute slot T is in. */
double RoomSlotEnd(double T)
{
	return (std::floor(T / RoomSlotLen) + 1.0) * RoomSlotLen;
}

/**
 * The per-visit giving hazard's bunching weight (§2.10.1): about 40% of a visit's gifts land in the 6 world minutes after a
 * big moment, and the visit's chance stays whole however often big moments come (four tables bring one every few minutes).
 * NearShare is the stream's share of grid minutes so far inside such a window (a prior of one big moment in 40 minutes);
 * from 40% up the weight is flat.
 */
double RoomGiveWeight(int NearMinutes, int GiveMinutes, bool Near)
{
	constexpr double Bunch = 0.4, Prior = 40.0;
	const double Share = std::min(0.98, std::max(0.02, (static_cast<double>(NearMinutes) + RoomGiveWindow) / (static_cast<double>(GiveMinutes) + Prior)));
	if (Share >= Bunch)
	{
		return 1.0;
	}
	return Near ? Bunch / Share : (1.0 - Bunch) / (1.0 - Share);
}

/** A visit's gift threshold for kind K (0 a tip, 1 a cheer, 2 a new sub): -ln(1 - u) of its keyed draw, which is the night's
 *  (§2.10.1: Unit(..., Give / Sub, NameKey, visit)), so it is drawn once a visit rather than every minute (the frame budget). */
double RoomGiveNeed(const RoomState& R, RoomState::Visit& V, int K)
{
	double& Need = V.Need[static_cast<size_t>(K)];
	if (Need < 0.0)
	{
		const double U = K == 2 ? growth::Unit(R.SeedK, R.StreamK, growth::Tag::Sub, V.Key, 0) : growth::Unit(R.SeedK, R.StreamK, growth::Tag::Give, V.Key, static_cast<uint64_t>(K));
		Need = -std::log(std::max(1.0e-12, 1.0 - U));
	}
	return Need;
}

/** The passive Engage floor (§2.9): attended paces talk through hands; watch 4x leaves fewer real seconds; 8x, a skip and a
 *  sprint are heads-down. The pace reaches the room only this way, through the streamer (R4). */
double RoomEngageFloor(const Inputs& In)
{
	if (In.Attended)
	{
		return RoomEngageAttended;
	}
	return In.WatchSpeed == 4 && !In.Sprinting ? RoomEngageWatch4 : RoomEngageHeadsDown;
}

/** The Ping's hour term: clamp(TimeOfDay / 1.25, 0.4, 1). */
double RoomToD(double World)
{
	return std::max(0.4, std::min(1.0, kast_detail::TimeOfDay(World) / 1.25));
}

/** The Ping's schedule term: 1.3 on schedule, 0.75 with no posted schedule, 0.6 posted but off it. */
double RoomSchedOf(const Channel& Ch, bool OnSchedule)
{
	return Ch.ScheduleDays == 0 ? 0.75 : OnSchedule ? 1.3 : 0.6;
}

growth::RunStage RoomStageOf(Moment M)
{
	switch (M)
	{
	case Moment::Bubble: return growth::RunStage::Bubble;
	case Moment::InTheMoney: return growth::RunStage::Itm;
	case Moment::FinalTable: return growth::RunStage::FinalTable;
	case Moment::Top3: return growth::RunStage::Top3;
	case Moment::Win: return growth::RunStage::Win;
	default: return growth::RunStage::None;
	}
}

/** Each stage's Run Alert bit (once a world day). */
uint32_t RoomAlertBit(growth::RunStage S)
{
	switch (S)
	{
	case growth::RunStage::Bubble: return growth::GrantAlertBubble;
	case growth::RunStage::Itm: return growth::GrantAlertItm;
	case growth::RunStage::FinalTable: return growth::GrantAlertFinal;
	case growth::RunStage::Top3: return growth::GrantAlertTop3;
	default: return 0;
	}
}

/** The moments people tip on (the giving hazard's bunching): an all-in and its result, a big pot, the stages. */
bool RoomBigMoment(Moment M)
{
	switch (M)
	{
	case Moment::AllIn:
	case Moment::WonAllIn:
	case Moment::LostAllIn:
	case Moment::BadBeat:
	case Moment::BigPot:
	case Moment::Bubble:
	case Moment::InTheMoney:
	case Moment::FinalTable:
	case Moment::Top3:
	case Moment::Win: return true;
	default: return false;
	}
}

/** The Sweat's novelty (§2.6): the final-table days of the last 7 (the day before Today back), from Channel::FtDays, whose bit i
 *  is the world day FtBase - i. */
int RoomFtCount(const Channel& Ch, int Today)
{
	if (Ch.FtBase < 0)
	{
		return 0;
	}
	int N = 0;
	for (int I = 0; I < 32; ++I)
	{
		const int D = Ch.FtBase - I;
		N += ((Ch.FtDays >> I) & 1) && D >= Today - 7 && D < Today ? 1 : 0;
	}
	return N;
}

void RoomFtMark(Channel& Ch, int Day)
{
	if (Ch.FtBase < 0 || Day > Ch.FtBase)
	{
		const int Shift = Ch.FtBase < 0 ? 32 : Day - Ch.FtBase;
		Ch.FtDays = Shift >= 32 ? 0 : static_cast<int>(static_cast<uint32_t>(Ch.FtDays) << Shift);
		Ch.FtBase = Day;
	}
	const int Bit = Ch.FtBase - Day;
	if (Bit >= 0 && Bit < 32)
	{
		Ch.FtDays = static_cast<int>(static_cast<uint32_t>(Ch.FtDays) | (1u << Bit));
	}
}

/** Heat now plus a bump (§2.4: 8 <= Heat <= 100), committed from the shown value as the model's add_heat does (the floor of 20
 *  while the channel streams weekly lifts the raw value at each commit). */
void RoomAddHeat(Channel& Ch, double World, double By)
{
	Ch.Heat = std::min(100.0, std::max(8.0, growth::HeatShown(Ch.Heat, Ch.HeatAt, World, Ch.LastStreamDay) + By));
	Ch.HeatAt = World;
}

double RoomShownHeat(const Channel& Ch, double World)
{
	return growth::HeatShown(Ch.Heat, Ch.HeatAt, World, Ch.LastStreamDay);
}

/** A usual night here: the mean length of the last ten logged streams (each 60+ world minutes), 120 for a new channel. */
double RoomNominal(const Channel& Ch)
{
	double Sum = 0.0;
	int N = 0;
	for (size_t I = 0; I < Ch.Log.size() && N < 10; ++I)
	{
		if (Ch.Log[I].Minutes >= RoomShort)
		{
			Sum += Ch.Log[I].Minutes;
			++N;
		}
	}
	return N > 0 ? Sum / static_cast<double>(N) : 120.0;
}

/** The night's Ping terms, shared by the go-live and the forecast. */
struct RoomPingNight
{
	int Streams = 1; // Channel::Streams for the stream (after Start's increment)
	double Sched = 1.0, ToD = 1.0, H = 0.2;
	int Today = 0;
	bool WelcomeBack = false;
};

/** A moderator's chance of being on tonight (Moderator::Online), 0 for anyone else. */
double RoomModOnline(const Channel& Ch, const std::string& Name)
{
	for (const Moderator& M : Ch.Mods)
	{
		if (M.Name == Name)
		{
			return M.Online;
		}
	}
	return 0.0;
}

/** A member's go-live chance (§2.3.1): growth::Pa over the member's terms; Dee and Mei by growth::FriendPa (both come to
 *  stream 1); an online mod is an attending member (F3), at least at their Moderator::Online. */
double RoomPaOf(const Member& M, double ModOnline, const RoomPingNight& N, double World)
{
	if (M.Friend)
	{
		return N.Streams <= 1 ? 1.0 : growth::FriendPa(N.Streams, N.Sched, N.ToD);
	}
	const double Days = std::max(0.0, (World - M.LastSeen) / kast_detail::KastDay);
	growth::PingTerms T;
	T.L = M.Loyalty;
	T.Honey = M.Follower ? growth::Honey(std::max(0.0, (World - M.FirstSeen) / kast_detail::KastDay)) : 0.0;
	T.PingWHot = growth::PingWHot(growth::PingW(Days, ModOnline > 0.0 ? 1.0 : M.Loyalty), N.H);
	T.HeatA = growth::HeatA(N.H);
	T.Appetite = growth::Appetite(Days);
	T.Budget = growth::Budget(growth::HwToday(M.Hw, M.HwDay, N.Today));
	T.WelcomeBack = N.WelcomeBack && M.Loyalty >= 0.1 && Days >= 5.0 ? 1.5 : 1.0;
	T.Sched = N.Sched;
	T.ToD = N.ToD;
	T.Recent = Days < 3.0 ? 1.15 : 1.0;
	const double Pa = growth::Pa(T);
	return ModOnline > 0.0 ? std::max(Pa, ModOnline) : Pa;
}

/** s_i = Sm / (Sm + G): the share of the night a member who came is here (the forecast, the giving hazard's normaliser). */
double RoomShare(double L, bool Friend, int Streams, bool Mod)
{
	const double Sm = growth::StayMean(L, Friend, Streams, Mod);
	const double G = growth::GapMean(L, Friend, Mod, false, 0.0);
	return Sm + G > 0.0 ? Sm / (Sm + G) : 0.0;
}

/** The directory's rate at the go-live (§2.11's A0): the night's terms with no Sweat, no hype and nobody in the room yet. */
double RoomA0(const Channel& Ch, const Inputs& In, double Q, double Disc, double Lane, double Rising, double H, double Novelty, uint64_t Seed)
{
	const TitleSpec& T = kast_detail::TitleOf(Ch);
	double A = (1.5 + 2.0 * Q) * Disc * kast_detail::Competition(In.World) * (0.4 + 0.6 * kast_detail::TimeOfDay(In.World) / 1.25) * T.Discover;
	A *= 1.0 + 0.08 * static_cast<double>(std::min(3, std::max(0, In.Tables - 1)));
	A *= growth::Buzz(In.VideoBuzz, Ch.Norm) * Lane * Rising * growth::HeatS(H) * Novelty;
	A *= growth::Tide(growth::Unit(Seed, 0, growth::Tag::Tide, static_cast<uint64_t>(std::max(0, kast_detail::KastDayOf(In.World)))), !In.SeriesWindow.empty());
	return A;
}

/**
 * The lurker pool in the forecast (C-K's L8; §2.11 left it out, so an old save's untracked followers forecast 1-3 and stream about
 * 13): §2.8.6's expected count at the go-live (the untracked followers at the night's schedule and hour, and 0.3% of PingW over
 * the tracked followers who don't come, each by its chance of staying away; no deep run yet), as growth::Forecast's entries: the
 * pool is floor(C + u) people, so floor(C) sure ones and one at C's fraction carry its mean and its variance. Attend is the
 * members' go-live chances, in Ch.Members' order.
 */
void RoomLurkEntries(const Channel& Ch, const std::vector<double>& Attend, double Sched, double World, std::vector<double>& AttendOut, std::vector<double>& SharesOut)
{
	int Tracked = 0;
	double Away = 0.0;
	for (size_t I = 0; I < Ch.Members.size(); ++I)
	{
		const Member& M = Ch.Members[I];
		if (!M.Follower)
		{
			continue;
		}
		++Tracked;
		const double Comes = I < Attend.size() ? std::min(1.0, std::max(0.0, Attend[I])) : 0.0;
		Away += (1.0 - Comes) * growth::PingW(std::max(0.0, (World - M.LastSeen) / kast_detail::KastDay), M.Loyalty);
	}
	const double Mean = growth::LurkerMean(std::max(0, Ch.Followers - Tracked), Sched, kast_detail::TimeOfDay(World), Away, 1.0);
	if (!(Mean > 0.0))
	{
		return;
	}
	const double Whole = std::floor(Mean);
	for (int K = 0; K < static_cast<int>(Whole); ++K)
	{
		AttendOut.push_back(1.0);
		SharesOut.push_back(1.0);
	}
	if (Mean - Whole > 0.0)
	{
		AttendOut.push_back(Mean - Whole);
		SharesOut.push_back(1.0);
	}
}

/** The table a call is about, as a key (§2.12: moments key on minute, kind and table). */
uint64_t RoomTable(const Inputs& In)
{
	return static_cast<uint64_t>(std::max(0, In.ThisTable));
}

/** The remaining of a field at a stage (ss.Kast.Moment's held run). */
int RoomHeldRemaining(int Stage, int Entrants)
{
	switch (static_cast<growth::RunStage>(Stage))
	{
	case growth::RunStage::Bubble: return std::max(10, static_cast<int>(std::lround(0.155 * Entrants)));
	case growth::RunStage::Itm: return std::max(10, static_cast<int>(std::lround(0.15 * Entrants)));
	case growth::RunStage::FinalTable: return 9;
	case growth::RunStage::Top3: return 3;
	case growth::RunStage::Win: return 1;
	default: return Entrants;
	}
}

/** The guest's shelf: the directory's boost terms split by what each added (Rerun's buzz, Fresh on Kast, Rising in Poker). */
uint8_t RoomShelf(double U, double Buzz, double Lane, double Rising)
{
	const double Video = Buzz > 1.0 ? (Buzz - 1.0) / Buzz : 0.0;
	const double Fresh = Lane > 1.0 ? (1.0 - Video) * (Lane - 1.0) / Lane : 0.0;
	const double Rise = Rising > 1.0 ? (1.0 - Video - Fresh) * (Rising - 1.0) / Rising : 0.0;
	if (U < Video)
	{
		return static_cast<uint8_t>(View::Source::Video);
	}
	if (U < Video + Fresh)
	{
		return static_cast<uint8_t>(View::Source::Fresh);
	}
	if (U < Video + Fresh + Rise)
	{
		return static_cast<uint8_t>(View::Source::Rising);
	}
	return static_cast<uint8_t>(View::Source::Directory);
}

/**
 * Channel::FindMember through the room's name index: the same member while every member's name is in MemberAt (one index a
 * name: no name twice, nobody added behind the room's back), else the channel's own search. A linear search over up to 1,500
 * members once a chat line was the display's largest cost (the frame budget, §9.4 #13).
 */
const Member* RoomFindMember(const RoomState& R, const Channel& Ch, const std::string& Name)
{
	if (R.MemberAt.size() == Ch.Members.size())
	{
		const auto It = R.MemberAt.find(Name);
		return It == R.MemberAt.end() ? nullptr : &Ch.Members[static_cast<size_t>(It->second)];
	}
	return Ch.FindMember(Name);
}

int RoomBadges(const RoomState& R, const Channel& Ch, const std::string& Name, double World)
{
	int Badges = 0;
	if (Ch.IsSub(Name, World))
	{
		Badges |= BadgeSub;
	}
	if (Ch.IsMod(Name))
	{
		Badges |= BadgeMod;
	}
	if (!(Badges & BadgeMod) && Ch.Affiliate)
	{
		const Member* M = RoomFindMember(R, Ch, Name);
		if (M && M->Loyalty >= SuperfanLoyalty)
		{
			Badges |= BadgeVip;
		}
	}
	return Badges;
}

/** growth::Unit's mixer (splitmix64's finaliser): the Room's keyed draws do its first three mixes once a stream
 *  (RoomState::KeyTags) and the last two per draw (RoomState::U); kast_growth's UnitFast checks every tag against growth::Unit. */
uint64_t RoomMix(uint64_t Z)
{
	Z += 0x9e3779b97f4a7c15ull;
	Z = (Z ^ (Z >> 30)) * 0xbf58476d1ce4e5b9ull;
	Z = (Z ^ (Z >> 27)) * 0x94d049bb133111ebull;
	return Z ^ (Z >> 31);
}

/** The index of B's lowest set bit (B != 0): presence by transition walks its bit sets in visit order. */
size_t RoomLowBit(uint64_t B)
{
	static const uint8_t Table[64] = {0, 1, 48, 2, 57, 49, 28, 3, 61, 58, 50, 42, 38, 29, 17, 4, 62, 55, 59, 36, 53, 51, 43, 22, 45, 39, 33, 30, 24, 18, 12, 5,
		63, 47, 56, 27, 60, 41, 37, 16, 54, 35, 52, 21, 44, 32, 23, 11, 46, 26, 40, 15, 34, 20, 31, 10, 25, 14, 19, 9, 13, 8, 7, 6};
	return Table[((B & (~B + 1)) * 0x03f79d71b4cb0a89ull) >> 58];
}

// What a tip and a cheer come to: today's tables (a tip about $9, a cheer about $4.75 on average, §2.10.1's means).
const int RoomTipCents[] = {100, 100, 100, 200, 200, 300, 500, 500, 500, 1000, 1000, 2000, 5000};
const int RoomCheerBits[] = {100, 100, 100, 100, 100, 200, 200, 300, 500, 500, 1000, 2500};
} // namespace kast_room_detail

// The Room's lines (SPEC_CREATORS §5.3): the game's voice from core-kastmath's inventory (KastCopy.h), every digit a fact of
// the moment it describes. These are the Room's thin calls into it (and the Broadcast's sender names, which the inventory
// writes inside its sentences).
namespace kast_room_copy
{
std::string RoomCopyLive(int Reached, int Of, int LastNight)
{
	return LastNight >= 10 && Of > 0 ? copy::GoLiveAfterglow(Reached, Of, LastNight) : copy::GoLive(Reached, Of);
}

std::string RoomCopyLiveAgain()
{
	return copy::GoLiveNoPing();
}

std::string RoomCopyOnSchedule()
{
	return " On schedule: your regulars knew you'd be here.";
}

std::string RoomCopyEmpty(int Which, int Rank, int Of, int Seen, int Came, bool Fresh, bool Rising)
{
	switch (Which % 4)
	{
	case 0: return Rank > 0 ? copy::EmptyRank(Rank, Of) : std::string();
	case 1: return copy::EmptySeen(Seen, Came);
	case 2: return Fresh ? copy::EmptyFresh() : std::string();
	default: return Rising ? copy::EmptyRising() : std::string();
	}
}

std::string RoomCopyBroadcastFrom(int Site)
{
	switch (static_cast<net::SiteId>(Site))
	{
	case net::SiteId::Ignite: return "Ignite's Bandstand";
	case net::SiteId::RunItTwice: return "RunItTwice's Newsdesk";
	default: return "RiverLine Live";
	}
}

std::string RoomCopyRaid(const std::string& From, int N)
{
	return From + " is raiding with " + Grouped(N) + (N == 1 ? " viewer!" : " viewers!");
}
} // namespace kast_room_copy

// ---- the RoomView (KastPlay.h): the chat games read the live room through these

void RoomState::Bind(Stream& S, Channel& C, const Inputs& I)
{
	ViewS = &S;
	ViewCh = &C;
	if (&I != &LastIn)
	{
		LastIn = I;
	}
	ViewIn = &LastIn;
	AttendedNow = I.Attended;
	RentsPaid = I.RentsPaid;
	// Members added to the channel behind the room's back while it's live (ss.Kast.Grant on a live stream: SessionKast pushes
	// them straight into Channel::Members) join the per-member arrays, not coming tonight, so no walk over the community indexes
	// past VisitOf, MemberKey or MemberPa (review, 2026-10-25: the lurker slot and a final table's follower alert read and wrote
	// past them). Never true in play: the room adds its own members through AddMemberKeyed.
	if (RoomLive && VisitOf.size() < C.Members.size())
	{
		for (size_t Ix = VisitOf.size(); Ix < C.Members.size(); ++Ix)
		{
			const std::string& Name = C.Members[Ix].Name;
			const int Canon = MemberAt.emplace(Name, static_cast<int>(Ix)).first->second;
			if (MemberCanon.size() == Ix)
			{
				MemberCanon.push_back(Canon);
			}
			VisitOf.push_back(-1);
			MemberKey.push_back(growth::NameKey(Name));
			MemberPa.push_back(0.0);
		}
	}
}

void RoomState::KeyTags()
{
	using kast_room_detail::RoomMix;
	const uint64_t Mixed = RoomMix(RoomMix(SeedK) ^ StreamK); // the seed's mix and the stream's
	for (size_t T = 0; T < TagKey.size(); ++T)
	{
		TagKey[T] = RoomMix(Mixed ^ (0x6b61737400000000ull + static_cast<uint64_t>(T))); // "kast" and the tag, as growth::Unit
	}
}

double RoomState::U(growth::Tag T, uint64_t A, uint64_t B) const
{
	using kast_room_detail::RoomMix;
	const uint64_t H = RoomMix(RoomMix(TagKey[static_cast<size_t>(T)] ^ A) ^ B);
	return static_cast<double>(H >> 11) * 0x1.0p-53;
}

double RoomState::Minute() const
{
	return ViewS ? ViewS->StartWorld + static_cast<double>(std::max(0, Done - 1)) : 0.0;
}

uint64_t RoomState::Seed() const
{
	return SeedK;
}

uint64_t RoomState::StreamNo() const
{
	return StreamK;
}

bool RoomState::Attended() const
{
	return AttendedNow;
}

double RoomState::Sched() const
{
	return SchedF;
}

double RoomState::ToD() const
{
	return kast_room_detail::RoomToD(ViewS ? ViewS->StartWorld + Clock : 0.0);
}

int RoomState::Watching() const
{
	return Watchers;
}

bool RoomState::Present(const std::string& Name) const
{
	if (Name.empty() || !ViewCh || !RoomLive)
	{
		return false;
	}
	return VisitNamed(*ViewCh, Name) >= 0 || GuestNamed(Name) >= 0 || RaiderNamed(Name) >= 0;
}

std::vector<std::string> RoomState::PresentNames() const
{
	std::vector<std::string> Out;
	if (!ViewCh || !ViewS || !RoomLive)
	{
		return Out;
	}
	for (const Visit& V : Visits)
	{
		if (HereAt(V, Clock))
		{
			Out.push_back(ViewCh->Members[static_cast<size_t>(V.Member)].Name);
		}
	}
	for (const Guest& G : Guests)
	{
		if (G.Kind == 0 && G.Pool >= 0 && GuestHere(G, Clock))
		{
			Out.push_back(ViewS->Pool[static_cast<size_t>(G.Pool)]);
		}
	}
	for (const Raider& X : Raiders)
	{
		if (!X.Name.empty() && RaiderHere(X, Clock))
		{
			Out.push_back(X.Name);
		}
	}
	return Out;
}

bool RoomState::Attending(const std::string& Name) const
{
	const auto It = MemberAt.find(Name);
	if (It == MemberAt.end() || It->second < 0 || static_cast<size_t>(It->second) >= VisitOf.size())
	{
		return false;
	}
	const int Vi = VisitOf[static_cast<size_t>(It->second)];
	return Vi >= 0 && !Visits[static_cast<size_t>(Vi)].Gone;
}

bool RoomState::Speak(const std::string& Who, const std::string& Text, LineKind Kind)
{
	if (!ViewS || !ViewCh || !ViewIn || !RoomLive || Text.empty())
	{
		return false;
	}
	Stream& S = *ViewS;
	const Inputs& I = *ViewIn;
	if (Kind == LineKind::Streamer)
	{
		S.Say(I, I.Hero, Text, LineKind::Streamer, BadgeStreamer | (ViewCh->Partner ? BadgeVerified : 0), 0xc6f432);
		return true;
	}
	if (Who.empty())
	{
		S.Say(I, "", Text, LineKind::System, 0, Violet);
		return true;
	}
	if (!Present(Who))
	{
		return false;
	}
	S.Say(I, Who, Text, Kind, kast_room_detail::RoomBadges(*this, *ViewCh, Who, I.World), kast_detail::NameColor(Who));
	S.Seen.insert(Who);
	Spoke.push_back({Who, I.Real});
	return true;
}

void RoomState::PostAlert(const Alert& A)
{
	if (ViewS)
	{
		// The games pair every alert with their own notice (PostNotice: First Follower's KastShine), so Queue's plain notice for
		// the alert's kind (a follow's chime) would be a second sound for one event (§5.1: one cue an event).
		const size_t NoticesBefore = ViewS->Notices.size();
		ViewS->Queue(A);
		if (ViewS->Notices.size() > NoticesBefore)
		{
			ViewS->Notices.pop_back();
		}
	}
}

void RoomState::PostNotice(const Notice& N)
{
	if (ViewS)
	{
		ViewS->Notices.push_back(N);
	}
}

double RoomState::Engage() const
{
	return ViewS ? ViewS->Engage : 0.0;
}

void RoomState::AddEngage(double By)
{
	if (ViewS)
	{
		ViewS->Engage = std::min(1.0, std::max(0.0, ViewS->Engage + By));
	}
}

void RoomState::AddHype(double By)
{
	if (ViewS)
	{
		ViewS->Hype = std::min(100.0, std::max(0.0, ViewS->Hype + By));
	}
}

bool RoomState::Follow(const std::string& Name)
{
	if (!ViewS || !ViewCh || !RoomLive)
	{
		return false;
	}
	const int Gi = GuestNamed(Name);
	if (Gi >= 0 && Guests[static_cast<size_t>(Gi)].Kind == 0)
	{
		FollowGuest(*ViewS, *ViewCh, Gi, Clock, true);
		return true;
	}
	const int Ri = RaiderNamed(Name);
	if (Ri >= 0)
	{
		Raider& X = Raiders[static_cast<size_t>(Ri)];
		X.Gone = true;
		++Raids[static_cast<size_t>(X.RaidIx)].Followed;
		FollowName(*ViewS, *ViewCh, X.Name, Clock, static_cast<uint8_t>(Raids[static_cast<size_t>(X.RaidIx)].Broadcast ? View::Source::Broadcast : View::Source::Raid), true, X.Leave, false);
		return true;
	}
	return false;
}

void RoomState::MarkNoticed(const std::string& Name)
{
	const auto It = MemberAt.find(Name);
	if (It != MemberAt.end() && static_cast<size_t>(It->second) < VisitOf.size() && VisitOf[static_cast<size_t>(It->second)] >= 0)
	{
		Visits[static_cast<size_t>(VisitOf[static_cast<size_t>(It->second)])].Noticed = true;
	}
}

void RoomState::ArriveAt(const std::string& Name, double At, View::Source From)
{
	if (!ViewS || !ViewCh || !RoomLive)
	{
		return;
	}
	const auto It = MemberAt.find(Name);
	if (It == MemberAt.end())
	{
		return;
	}
	const double T = std::max(Clock, At - ViewS->StartWorld);
	const int Mi = It->second;
	int Vi = VisitOf[static_cast<size_t>(Mi)];
	if (Vi < 0)
	{
		Vi = AddVisit(*ViewS, *ViewCh, Mi, T, static_cast<uint8_t>(From));
	}
	else
	{
		PullAt(Visits[static_cast<size_t>(Vi)], Clock, T);
	}
	if (Vi >= 0)
	{
		Visits[static_cast<size_t>(Vi)].OnArrive = true;
	}
}

double RoomState::RaidAt() const
{
	return LastRaidAt < -1.0e8 || !ViewS ? -1.0 : ViewS->StartWorld + LastRaidAt;
}

std::vector<std::string> RoomState::RaidNamesHere() const
{
	// The newest raid's named raiders in the room now: the ones who thank a welcome (play::WelcomeRaid, copy::WelcomedChat).
	std::vector<std::string> Out;
	if (!ViewCh || !ViewS || !RoomLive || Raids.empty())
	{
		return Out;
	}
	const int Newest = static_cast<int>(Raids.size()) - 1;
	for (const Raider& X : Raiders)
	{
		if (X.RaidIx == Newest && !X.Name.empty() && RaiderHere(X, Clock))
		{
			Out.push_back(X.Name);
		}
	}
	return Out;
}

void RoomState::WelcomeRaiders(double StayBy, double FollowBy)
{
	if (Raids.empty())
	{
		return;
	}
	const int Ri = static_cast<int>(Raids.size()) - 1;
	Raid& Rd = Raids.back();
	if (Rd.Welcomed)
	{
		return;
	}
	Rd.Welcomed = true;
	Rd.FollowBy = FollowBy;
	for (Raider& X : Raiders)
	{
		if (X.RaidIx == Ri && RaiderHere(X, Clock))
		{
			X.Leave = X.Arrive + (X.Leave - X.Arrive) * StayBy;
			// The name stays theirs while they stay (no guest walks in wearing it).
			if (X.Pool >= 0 && static_cast<size_t>(X.Pool) < PoolUntil.size())
			{
				PoolUntil[static_cast<size_t>(X.Pool)] = std::max(PoolUntil[static_cast<size_t>(X.Pool)], X.Leave);
			}
		}
	}
}

void RoomState::BankArrivals(int N, double StayBy)
{
	BankedLeft = std::max(0, N);
	BankedStay = StayBy;
}

// ---- presence, pure in the room's state

bool RoomState::HereAt(const Visit& V, double T)
{
	return V.Here0 != (T >= V.Flip);
}

double RoomState::WatchedTo(const Visit& V, double T)
{
	if (T <= V.SlotStart)
	{
		return V.Watched;
	}
	if (V.Here0)
	{
		return V.Watched + (std::min(T, V.Flip) - V.SlotStart);
	}
	return V.Watched + std::max(0.0, T - std::max(V.Flip, V.SlotStart));
}

void RoomState::PullAt(Visit& V, double T, double Arrive)
{
	if (V.Gone || HereAt(V, T))
	{
		return;
	}
	PlanChanging(V);
	// Away at T (or not here yet): the plan restarts at T, away, with the earlier of a return already planned and this arrival.
	const double OldReturn = V.Here0 ? kast_room_detail::RoomInf : V.Flip;
	V.Watched = WatchedTo(V, T);
	V.SlotStart = T;
	V.Here0 = false;
	V.Pending = std::min(V.Pending, Arrive);
	const double Next = std::min(OldReturn, V.Pending);
	V.Flip = Next < kast_room_detail::RoomSlotEnd(T) ? std::max(T, Next) : kast_room_detail::RoomInf;
}

void RoomState::LeaveNow(Visit& V, double T)
{
	PlanChanging(V);
	V.Watched = WatchedTo(V, T);
	V.SlotStart = T;
	V.Here0 = false;
	V.Flip = kast_room_detail::RoomInf;
	V.Pending = kast_room_detail::RoomInf;
	V.Gone = true;
}

void RoomState::Recount(const Channel& C, double T)
{
	MembersHere = 0;
	RegularsHere = 0;
	GuestsHere = 0;
	CrowdHere = 0;
	RaidersHere = 0;
	ModsHere.clear();
	SourcesNow.fill(0);
	// The visits here, in order, and word of mouth's sum over them (RunMinute's giving and arrivals read both: C-K's M2).
	HereIx.clear();
	HereBring = 0.0;
	if (PresOn)
	{
		// Presence by transition: the same counts, the mods in visit order, and the same sum to the bit (its zero terms skipped).
		PresUpdate(C, T);
		MembersHere = PresMembers;
		RegularsHere = PresRegulars;
		SourcesNow[static_cast<size_t>(View::Source::Members)] = PresMembers;
		for (const int Vi : ModVisits)
		{
			if (PresHere[static_cast<size_t>(Vi)] != 0)
			{
				ModsHere.push_back(Visits[static_cast<size_t>(Vi)].Member);
			}
		}
		for (size_t W = 0; W < PresBringBits.size(); ++W)
		{
			for (uint64_t B = PresBringBits[W]; B != 0; B &= B - 1)
			{
				HereBring += PresBring[W * 64 + kast_room_detail::RoomLowBit(B)];
			}
		}
		if (!GiveLazy)
		{
			for (size_t W = 0; W < PresBits.size(); ++W)
			{
				for (uint64_t B = PresBits[W]; B != 0; B &= B - 1)
				{
					HereIx.push_back(static_cast<int>(W * 64 + kast_room_detail::RoomLowBit(B)));
				}
			}
		}
	}
	else
	{
		for (size_t Vi = 0; Vi < Visits.size(); ++Vi)
		{
			const Visit& V = Visits[Vi];
			if (!HereAt(V, T))
			{
				continue;
			}
			HereIx.push_back(static_cast<int>(Vi));
			++MembersHere;
			const Member& M = C.Members[static_cast<size_t>(V.Member)];
			RegularsHere += M.Loyalty >= RegularLoyalty && !M.Friend ? 1 : 0;
			HereBring += std::max(0.0, M.Loyalty - 0.4);
			if (V.Mod)
			{
				ModsHere.push_back(V.Member);
			}
			++SourcesNow[static_cast<size_t>(View::Source::Members)];
		}
	}
	HereT = T;
	HereLoyalty = LoyaltyWrites;
	HereFresh = false;
	for (const Guest& G : Guests)
	{
		if (GuestHere(G, T))
		{
			++GuestsHere;
			++SourcesNow[G.Source];
		}
	}
	for (const Crowd& X : Crowds)
	{
		if (!X.Gone && X.Arrive <= T && T < X.Leave)
		{
			++CrowdHere;
			++SourcesNow[X.Source];
		}
	}
	for (const Raider& X : Raiders)
	{
		if (RaiderHere(X, T))
		{
			++RaidersHere;
			++SourcesNow[static_cast<size_t>(Raids[static_cast<size_t>(X.RaidIx)].Broadcast ? View::Source::Broadcast : View::Source::Raid)];
		}
	}
	SourcesNow[static_cast<size_t>(View::Source::Lurkers)] += LurkersNow;
	Watchers = MembersHere + GuestsHere + CrowdHere + RaidersHere + LurkersNow;
}

void RoomState::PresMark(int Vi)
{
	if (Vi < 0)
	{
		return;
	}
	if (PresDirtyFlag.size() < Visits.size())
	{
		PresDirtyFlag.resize(Visits.size(), 0);
	}
	if (static_cast<size_t>(Vi) < PresDirtyFlag.size() && PresDirtyFlag[static_cast<size_t>(Vi)] == 0)
	{
		PresDirtyFlag[static_cast<size_t>(Vi)] = 1;
		PresDirty.push_back(Vi);
	}
}

void RoomState::PresRead(const Channel& C, int Vi, double T, bool FromPlan)
{
	PresReadAt(Vi, T, FromPlan, C.Members[static_cast<size_t>(Visits[static_cast<size_t>(Vi)].Member)].Loyalty);
}

void RoomState::PresReadAt(int Vi, double T, bool FromPlan, double L)
{
	// One visit's presence at T on its current plan (HereAt), whether its member is a regular now, and word of mouth's term at their
	// loyalty now (L; a visit's Friend is its member's); the counts and the bit sets follow. A fresh plan's future flip goes on the
	// heap.
	const size_t I = static_cast<size_t>(Vi);
	const Visit& V = Visits[I];
	const bool Here = HereAt(V, T);
	const uint8_t Regular = L >= RegularLoyalty && !V.Friend ? 1 : 0;
	const double Bring = std::max(0.0, L - 0.4);
	if (PresHere[I] != 0)
	{
		--PresMembers;
		PresRegulars -= PresRegular[I];
	}
	PresHere[I] = Here ? 1 : 0;
	PresRegular[I] = Regular;
	PresBring[I] = Bring;
	PresTalk[I] = 0.2 + L;
	PresL[I] = L;
	PresMember[I] = V.Member;
	const uint64_t Bit = 1ull << (I % 64);
	PresBits[I / 64] = Here ? PresBits[I / 64] | Bit : PresBits[I / 64] & ~Bit;
	PresBringBits[I / 64] = Here && Bring != 0.0 ? PresBringBits[I / 64] | Bit : PresBringBits[I / 64] & ~Bit;
	if (Here)
	{
		++PresMembers;
		PresRegulars += Regular;
	}
	if (FromPlan && V.Flip > T && V.Flip < kast_room_detail::RoomInf)
	{
		FlipHeap.emplace_back(V.Flip, Vi);
		std::push_heap(FlipHeap.begin(), FlipHeap.end(), std::greater<std::pair<double, int>>());
	}
}

void RoomState::PresSize()
{
	const size_t N = Visits.size();
	if (PresHere.size() < N)
	{
		PresHere.resize(N, 0);
		PresRegular.resize(N, 0);
		PresBring.resize(N, 0.0);
		PresTalk.resize(N, 0.0);
		PresL.resize(N, 0.0);
		PresMember.resize(N, 0);
		PresBits.resize((N + 63) / 64, 0);
		PresBringBits.resize((N + 63) / 64, 0);
	}
}

void RoomState::PresUpdate(const Channel& C, double T)
{
	const size_t N = Visits.size();
	PresSize();
	if (T < PresT)
	{
		// Never on the grid (the room's clock only moves forward): read everyone again.
		for (size_t Vi = 0; Vi < N; ++Vi)
		{
			PresMark(static_cast<int>(Vi));
		}
	}
	for (const int Vi : PresDirty)
	{
		PresDirtyFlag[static_cast<size_t>(Vi)] = 0;
		PresRead(C, Vi, T, true);
	}
	PresDirty.clear();
	while (!FlipHeap.empty() && FlipHeap.front().first <= T)
	{
		const int Vi = FlipHeap.front().second;
		std::pop_heap(FlipHeap.begin(), FlipHeap.end(), std::greater<std::pair<double, int>>());
		FlipHeap.pop_back();
		PresRead(C, Vi, T, false);
	}
	PresT = T;
}

growth::Standing RoomState::RoomRank(double World, int Viewers)
{
	// growth::RankAt's walk (the directory's live channels, then the network's), with each channel's night hash kept for the day
	// instead of built from a string once a grid minute (C-K's M2; kast_growth's RankFast checks it against RankAt).
	const std::vector<Streamer>& Dir = Directory();
	const std::vector<kast_detail::NetSpec>& Net = kast_detail::NetworkSpecs();
	const int Today = kast_detail::KastDayOf(World);
	if (RankDay.Today != Today)
	{
		// A new world day: yesterday's hashes are the old today's when the clock just crossed midnight; the rest are built.
		const bool Next = RankDay.Today == Today - 1 && RankDay.Dir[0].size() == Dir.size() && RankDay.Net[0].size() == Net.size();
		if (Next)
		{
			RankDay.Dir[1].swap(RankDay.Dir[0]);
			RankDay.Net[1].swap(RankDay.Net[0]);
		}
		for (size_t K = 0; K < (Next ? 1u : 2u); ++K)
		{
			const std::string Day = std::to_string(Today - static_cast<int>(K));
			RankDay.Dir[K].clear();
			RankDay.Net[K].clear();
			for (const Streamer& S : Dir)
			{
				RankDay.Dir[K].push_back(kast_detail::NameHash(S.Name + Day));
			}
			for (const kast_detail::NetSpec& S : Net)
			{
				RankDay.Net[K].push_back(kast_detail::NameHash(S.Name + Day));
			}
		}
		RankDay.Today = Today;
	}
	growth::Standing St;
	St.Rank = 1;
	St.Of = 1;
	const double OfDay = std::fmod(World, kast_detail::KastDay); // ViewersNow's and Network's minute of the day, once for every channel
	for (size_t I = 0; I < Dir.size(); ++I)
	{
		const int Now = kast_detail::DirViewersAt(Dir[I], World, OfDay, Today, [&](int D) {
			return D == Today ? RankDay.Dir[0][I] : D == Today - 1 ? RankDay.Dir[1][I] : kast_detail::NameHash(Dir[I].Name + std::to_string(D));
		});
		if (Now > 0)
		{
			++St.Of;
			St.Rank += Now > Viewers ? 1 : 0;
		}
	}
	for (size_t I = 0; I < Net.size(); ++I)
	{
		const int Back = kast_detail::NetLiveBackAt(Net[I], OfDay, Today);
		if (Back >= 0)
		{
			const int V = std::max(1, static_cast<int>(std::round(static_cast<double>(Net[I].Viewers) * kast_detail::NetNight(RankDay.Net[static_cast<size_t>(Back)][I]))));
			++St.Of;
			St.Rank += V > Viewers ? 1 : 0;
		}
	}
	return St;
}

int RoomState::VisitNamed(const Channel& C, const std::string& Name) const
{
	(void)C;
	const auto It = MemberAt.find(Name);
	if (It == MemberAt.end() || static_cast<size_t>(It->second) >= VisitOf.size())
	{
		return -1;
	}
	const int Vi = VisitOf[static_cast<size_t>(It->second)];
	return Vi >= 0 && HereAt(Visits[static_cast<size_t>(Vi)], Clock) ? Vi : -1;
}

int RoomState::GuestNamed(const std::string& Name) const
{
	if (!ViewS)
	{
		return -1;
	}
	// A pool name is its index (the pool's names are unique), so a guest's name compares as an integer (the frame budget: Present
	// runs once a chat line).
	const bool Indexed = PoolAt.size() == ViewS->Pool.size();
	int PoolIx = -1;
	if (Indexed)
	{
		const auto It = PoolAt.find(Name);
		PoolIx = It == PoolAt.end() ? -1 : It->second;
	}
	for (size_t I = 0; I < Guests.size(); ++I)
	{
		const Guest& G = Guests[I];
		if (GuestHere(G, Clock) && (G.Pool >= 0 ? (Indexed ? G.Pool == PoolIx : ViewS->Pool[static_cast<size_t>(G.Pool)] == Name) : G.Bot == Name))
		{
			return static_cast<int>(I);
		}
	}
	return -1;
}

int RoomState::RaiderNamed(const std::string& Name) const
{
	for (size_t I = 0; I < Raiders.size(); ++I)
	{
		const Raider& X = Raiders[I];
		if (!X.Name.empty() && X.Name == Name && RaiderHere(X, Clock))
		{
			return static_cast<int>(I);
		}
	}
	return -1;
}

int RoomState::FreeName(const Stream& S, double U, double T) const
{
	const int N = static_cast<int>(S.Pool.size());
	if (N == 0 || PoolUntil.size() != S.Pool.size())
	{
		return -1;
	}
	int Ix = std::min(N - 1, std::max(0, static_cast<int>(std::floor(U * static_cast<double>(N)))));
	for (int K = 0; K < 64; ++K, Ix = (Ix + 1) % N)
	{
		if (PoolUntil[static_cast<size_t>(Ix)] > T || MemberAt.count(S.Pool[static_cast<size_t>(Ix)]) != 0)
		{
			continue;
		}
		return Ix;
	}
	return -1;
}

// ---- the Room's steps

void RoomState::Earned(Stream& S, Channel& Ch, Chips Cents, int Source)
{
	// The creator update (SPEC_CREATORS C13, C14): tips pay at once; Affiliate revenue (subs, cheers, ads, gifts) earned before
	// the second rent waits in PendingCents; sponsor pay is the sponsor book's, booked by the session as LedgerSponsors lines
	// (Summary::SponsorPaid), never inside the Kast payout.
	if (Cents <= 0)
	{
		return;
	}
	RoomState& R = *S.Room;
	switch (Source)
	{
	case 0:
		Ch.EarnedSubs += Cents;
		S.Tonight.SubCents += Cents;
		break;
	case 1:
		Ch.EarnedBits += Cents;
		S.Tonight.BitCents += Cents;
		break;
	case 2:
		Ch.EarnedTips += Cents;
		S.Tonight.TipCents += Cents;
		break;
	case 3:
		Ch.EarnedAds += Cents;
		S.Tonight.AdCents += Cents;
		break;
	default:
		Ch.EarnedSponsors += Cents;
		S.Tonight.SponsorCents += Cents;
		return;
	}
	if (Source != 2 && R.RentsPaid < 2)
	{
		Ch.PendingCents += Cents;
	}
	else
	{
		Ch.UnpaidCents += Cents;
	}
}

int RoomState::AddMemberKeyed(Stream& S, Channel& Ch, const std::string& Name, bool Follower, double Loyalty, double World)
{
	RoomState& R = *S.Room;
	if (static_cast<int>(Ch.Members.size()) >= MaxMembers || Name.empty() || R.MemberAt.count(Name) != 0)
	{
		return -1;
	}
	const uint64_t Key = growth::NameKey(Name);
	Member M;
	M.Name = Name;
	M.Loyalty = Loyalty;
	M.Affinity = 0.05 + 0.9 * std::pow(growth::Unit(R.SeedK, 0, growth::Tag::Affinity, Key), 4.0); // most are casual (§2.12: keyed by name)
	M.FirstSeen = World;
	M.LastSeen = World;
	M.Follower = Follower;
	Ch.Members.push_back(M);
	const int Ix = static_cast<int>(Ch.Members.size()) - 1;
	R.MemberAt[Name] = Ix;
	R.MemberCanon.push_back(Ix);
	R.VisitOf.push_back(-1);
	R.MemberKey.push_back(Key);
	R.MemberPa.push_back(0.0);
	return Ix;
}

int RoomState::AddVisit(Stream& S, Channel& Ch, int MemberIx, double Arrive, uint8_t Source)
{
	RoomState& R = *S.Room;
	if (MemberIx < 0 || static_cast<size_t>(MemberIx) >= Ch.Members.size())
	{
		return -1;
	}
	if (R.VisitOf[static_cast<size_t>(MemberIx)] >= 0)
	{
		const int Vi = R.VisitOf[static_cast<size_t>(MemberIx)];
		R.PullAt(R.Visits[static_cast<size_t>(Vi)], R.Clock, Arrive);
		return Vi;
	}
	const Member& M = Ch.Members[static_cast<size_t>(MemberIx)];
	Visit V;
	V.Member = MemberIx;
	V.Key = R.MemberKey[static_cast<size_t>(MemberIx)];
	V.Pa = R.MemberPa[static_cast<size_t>(MemberIx)];
	V.Friend = M.Friend;
	V.Mod = Ch.IsMod(M.Name);
	V.Sub = Ch.IsSub(M.Name, S.StartWorld + R.Clock);
	V.Source = Source;
	V.SlotStart = R.Clock;
	V.Pending = std::max(R.Clock, Arrive);
	V.Flip = V.Pending < kast_room_detail::RoomSlotEnd(R.Clock) ? V.Pending : kast_room_detail::RoomInf;
	const double Share = kast_room_detail::RoomShare(M.Loyalty, M.Friend, static_cast<int>(R.StreamK), V.Mod);
	const double Sm = growth::StayMean(M.Loyalty, M.Friend, static_cast<int>(R.StreamK), V.Mod);
	const double Rem = std::max(0.0, R.Nominal - V.Pending);
	V.Expect = std::max(10.0, std::min(Rem, Sm) + std::max(0.0, Rem - Sm) * Share);
	R.Visits.push_back(V);
	R.VisitOf[static_cast<size_t>(MemberIx)] = static_cast<int>(R.Visits.size()) - 1;
	R.GiveNew(Ch, static_cast<int>(R.Visits.size()) - 1);
	return static_cast<int>(R.Visits.size()) - 1;
}

int RoomState::AddGuest(Stream& S, double Arrive, double Leave, uint8_t Source, int FollowAt, double UName, uint8_t Kind)
{
	RoomState& R = *S.Room;
	++R.ArrivedTonight;
	++R.NightSources[Source];
	if (Kind == 0)
	{
		++S.Tonight.NewFaces;
	}
	// The first guest who stays 10+ world minutes on a first stream follows: "your first follower who isn't family" (§2.5.2).
	bool First = false;
	if (Kind == 0 && !R.FirstFollowerDone && Leave - Arrive >= 10.0)
	{
		// They follow once they've watched ten minutes, at a keyed moment of the next two (Unit(..., FirstFollower)).
		R.FirstFollowerDone = true;
		First = true;
		FollowAt = static_cast<int>(std::ceil(Arrive + 10.0 + std::min(2.0, Leave - Arrive - 10.0) * growth::Unit(R.SeedK, R.StreamK, growth::Tag::FirstFollower, 0)));
		R.FirstFollowerAt = FollowAt;
	}
	if (R.GuestsHere < kast_room_detail::RoomGuestCap || Kind != 0)
	{
		const int Ix = Kind == 2 ? -1 : R.FreeName(S, UName, Arrive);
		if (Ix >= 0 || Kind == 2)
		{
			Guest G;
			G.Pool = Ix;
			G.Arrive = Arrive;
			G.Leave = Leave;
			G.Source = Source;
			G.Kind = Kind;
			G.FollowAt = FollowAt;
			G.FirstFollower = First;
			if (Ix >= 0)
			{
				R.PoolUntil[static_cast<size_t>(Ix)] = Leave;
			}
			R.Guests.push_back(G);
			++R.GuestsHere;
			return static_cast<int>(R.Guests.size()) - 1;
		}
	}
	if (Kind != 0)
	{
		return -1;
	}
	if (First)
	{
		R.FirstFollowerDone = false; // the crowd never speaks: the first follower's line waits for a named guest
	}
	Crowd C;
	C.Arrive = Arrive;
	C.Leave = Leave;
	C.Source = Source;
	C.FollowAt = First ? -1 : FollowAt;
	R.Crowds.push_back(C);
	return -1;
}

void RoomState::FollowName(Stream& S, Channel& Ch, const std::string& Name, double T, uint8_t Source, bool Here, double Until, bool First)
{
	// A follow (§2.5.3): one person, one count. The guest becomes a member on the spot, here for the rest of their stay this slot.
	RoomState& R = *S.Room;
	const int Before = Ch.Followers;
	++Ch.Followers;
	++S.Tonight.Follows;
	++R.FollowSources[Source];
	++R.FollowedTonight;
	const double World = S.StartWorld + T;
	const int Mi = AddMemberKeyed(S, Ch, Name, true, 0.04, World);
	if (Mi >= 0 && Here)
	{
		Visit V;
		V.Member = Mi;
		V.Key = R.MemberKey[static_cast<size_t>(Mi)];
		V.Source = static_cast<uint8_t>(View::Source::Members);
		V.Arrived = true;
		V.FirstIn = T;
		V.SlotStart = T;
		V.Here0 = true;
		V.Flip = Until < kast_room_detail::RoomSlotEnd(T) ? std::max(T, Until) : kast_room_detail::RoomInf;
		const double Sm = growth::StayMean(0.04, false, static_cast<int>(R.StreamK), false);
		V.Expect = std::max(10.0, std::min(std::max(0.0, R.Nominal - T), Sm));
		R.Visits.push_back(V);
		R.VisitOf[static_cast<size_t>(Mi)] = static_cast<int>(R.Visits.size()) - 1;
		R.GiveNew(Ch, static_cast<int>(R.Visits.size()) - 1);
	}
	if (!First)
	{
		Alert A;
		A.Kind = AlertKind::Follow;
		A.Who = Name;
		S.Queue(A);
	}
	if (Before < 2400 && Ch.Followers >= 2400)
	{
		Notice N;
		N.Type = Notice::Kind::Text;
		N.From = "gh0stfold";
		N.Body = "saw you passed me on kast. cute. see you at the tables.";
		S.Notices.push_back(N);
	}
	play::OnFollow(*S.Play, R, Ch, Name, First);
}

void RoomState::FollowGuest(Stream& S, Channel& Ch, int GuestIx, double T, bool Announce)
{
	RoomState& R = *S.Room;
	Guest& G = R.Guests[static_cast<size_t>(GuestIx)];
	if (G.Gone || G.Kind != 0 || G.Pool < 0)
	{
		return;
	}
	G.Gone = true;
	--R.GuestsHere;
	R.PoolUntil[static_cast<size_t>(G.Pool)] = kast_room_detail::RoomInf; // a member now
	const std::string Name = S.Pool[static_cast<size_t>(G.Pool)];
	const bool Here = G.Arrive <= T && T < G.Leave;
	const uint8_t Src = G.Source;
	const double Until = G.Leave;
	const bool First = G.FirstFollower;
	(void)Announce;
	R.FirstFollowed = R.FirstFollowed || First;
	FollowName(S, Ch, Name, T, Src, Here, Until, First);
}

void RoomState::AddRaid(Stream& S, Channel& Ch, const Inputs& In, const std::string& From, uint32_t Color, int Size, bool Broadcast, double At, int Kind)
{
	// Raiders are people (§2.7.1): named (24 a raid, keyed Raider:<minute>:<i>), the rest of the party crowd, 40% staying Exp(20)
	// and 60% Exp(3) world minutes (the Broadcast: half each of 20 and 3).
	RoomState& R = *S.Room;
	if (Size <= 0)
	{
		return;
	}
	const int M = static_cast<int>(std::floor(At));
	const int Ri = static_cast<int>(R.Raids.size());
	const uint64_t A = static_cast<uint64_t>(std::max(0, M)) * 16u + static_cast<uint64_t>(Ri % 16);
	Raid Rd;
	Rd.From = From;
	Rd.Color = Color;
	Rd.Size = Size;
	Rd.At = At;
	Rd.AtMinute = M;
	Rd.Broadcast = Broadcast;
	R.Raids.push_back(Rd);
	for (int I = 0; I < Size; ++I)
	{
		const uint64_t B = static_cast<uint64_t>(I) * 4u;
		Raider X;
		X.RaidIx = Ri;
		X.Arrive = Kind == 5 ? At : At + 0.01 * static_cast<double>(std::min(I, 60)); // ss.Kast.Raid's party is in at once
		const double U1 = growth::Unit(R.SeedK, R.StreamK, growth::Tag::Raider, A, B);
		const double U2 = growth::Unit(R.SeedK, R.StreamK, growth::Tag::Raider, A, B + 1);
		const double Stay = Broadcast ? growth::ExpAt(I % 2 == 0 ? 20.0 : 3.0, U2) : growth::RaiderStay(U1, U2, false);
		X.Leave = X.Arrive + std::max(0.1, Stay);
		X.U = growth::Unit(R.SeedK, R.StreamK, growth::Tag::Raider, A, B + 2);
		if (I < kast_room_detail::RoomRaidNamed)
		{
			const int Ix = R.FreeName(S, growth::Unit(R.SeedK, R.StreamK, growth::Tag::Raider, A, B + 3), X.Arrive);
			if (Ix >= 0)
			{
				X.Name = S.Pool[static_cast<size_t>(Ix)];
				X.Pool = Ix;
				R.PoolUntil[static_cast<size_t>(Ix)] = X.Leave;
			}
		}
		R.Raiders.push_back(X);
	}
	R.LastRaidAt = At;
	R.NightSources[static_cast<size_t>(Broadcast ? View::Source::Broadcast : View::Source::Raid)] += Size;
	const bool FirstRaid = Ch.RaidsIn == 0; // the channel's first raid in ever (K17's milestone, posted below)
	++Ch.RaidsIn;
	++S.Tonight.Raids;
	S.Hype = std::min(100.0, S.Hype + (Kind == 1 || Kind == 3 || Broadcast ? 15.0 : 10.0));
	S.LastRaider = From;
	Alert Al;
	Al.Kind = AlertKind::Raid;
	Al.Who = From;
	Al.Count = Size;
	// A big name raids in its own words (K6, C-K's L5): "VikingVolta raided with 312".
	const std::string Says = Broadcast ? copy::Broadcast(In.DeepSite, Size) : Kind == 2 ? copy::WelcomeRaid(From, Size) : Kind == 3 ? copy::RaidedWith(From, Size) : kast_room_copy::RoomCopyRaid(From, Size);
	// The Broadcast in its site's voice, the welcome raid and a big name carry their line onto the alert (ui-kast draws it under
	// RAID!).
	Al.Text = Broadcast || Kind == 2 || Kind == 3 ? Says : std::string();
	S.Queue(Al);
	S.Say(In, "", Says, LineKind::Raid, 0, Color);
	if (FirstRaid)
	{
		RoomState::PostFirst(S, From, copy::FirstRaidIn(From, Size)); // C-K's M3: live, with KastShine (§5.3 K17)
	}
	// The party says hi: a few of the named raiders (each still here when the line goes up).
	static const std::vector<std::string> RaidLines = {"RAID kastHype", "{raider} sent us!", "hi from {raider}'s stream", "kastPog kastPog", "raid hype", "GL on the run!", "we're here to sweat"};
	int Said = 0;
	for (size_t I = 0; I < R.Raiders.size() && Said < std::min(6, 1 + Size / 4); ++I)
	{
		const Raider& X = R.Raiders[I];
		if (X.RaidIx == Ri && !X.Name.empty())
		{
			Line L;
			L.Who = X.Name;
			L.Text = kast_detail::Fill(S.R.Pick(RaidLines), "{raider}", From);
			L.Delay = 0.4 + 0.35 * static_cast<double>(Said);
			L.World = X.Arrive;
			R.DueLines.push_back(L);
			++Said;
		}
	}
	// A moment raid of 5+ that is a fifth of the room or more: Heat +3 (§2.4). S-K (2026-10-24): synth_kast.py bumps Heat for the
	// network's moment raids (Kind 1, and 3 the big name) only; the raids back that tokens bring (about one every other stream for a
	// regular who raids out) held Heat 6-10 above the model's and ran its first 90 days x1.3 hot.
	if ((Kind == 1 || Kind == 3) && Size >= 5 && static_cast<double>(Size) >= 0.2 * std::max(1.0, static_cast<double>(R.Watchers)))
	{
		kast_room_detail::RoomAddHeat(Ch, S.StartWorld + At, kast_room_detail::RoomHeatRaid);
		R.WhyRaidFrom = From; // Heat's why line: "kat_v's raid of 14" (C-K's L5)
		R.WhyRaidSize = Size;
		R.WhyRaidDay = kast_detail::KastDayOf(S.StartWorld + At);
		++R.WhyEpoch;
	}
}

void RoomState::PostFirst(Stream& S, const std::string& Who, const std::string& Text)
{
	Alert A;
	A.Kind = AlertKind::Milestone;
	A.Who = Who;
	A.Text = Text;
	A.Count = 0; // a small milestone: no confetti (K17's tiers start at 25 followers)
	const size_t Before = S.Notices.size();
	S.Queue(A);
	if (S.Notices.size() > Before)
	{
		S.Notices.back().Sound = static_cast<int>(SoundId::KastShine); // through the cue queue, in place of the kind's own
	}
	Line L;
	L.Text = Text;
	L.Kind = LineKind::System;
	L.Color = Lime;
	S.Room->DueLines.push_back(L);
	S.Tonight.Firsts.push_back(Text);
}

void RoomState::Flush(Stream& S, const Inputs& In)
{
	// The lines the grid set off go up now, from whoever is still here (R1: a speaker who left in between says nothing).
	RoomState& R = *S.Room;
	if (R.DueLines.empty() || !R.ViewCh)
	{
		R.DueLines.clear();
		return;
	}
	std::vector<Line> Lines;
	Lines.swap(R.DueLines);
	for (const Line& L : Lines)
	{
		if (L.World > R.Clock)
		{
			R.DueLines.push_back(L); // later in this minute (they aren't here yet): it goes up when the clock gets there
			continue;
		}
		if (!L.Who.empty() && !R.Present(L.Who))
		{
			continue;
		}
		const int Badges = L.Who.empty() ? 0 : L.Badges >= 0 ? L.Badges : kast_room_detail::RoomBadges(R, *R.ViewCh, L.Who, In.World);
		S.Say(In, L.Who, L.Text, L.Kind, Badges, L.Color != 0 ? L.Color : L.Who.empty() ? Violet : kast_detail::NameColor(L.Who), L.Cents);
		ChatMsg& Msg = S.Chat.back();
		Msg.At = In.Real + L.Delay;
		Msg.Toxic = L.Toxic;
		Msg.Spam = L.Spam;
		if (L.DeleteAfter >= 0.0)
		{
			Msg.DeleteAt = Msg.At + L.DeleteAfter;
			Msg.DeletedBy = L.DeletedBy;
		}
		else if (L.DeleteAfter <= -2.0)
		{
			Msg.DeleteAt = -3.0; // a troll nobody caught (the grid booked the miss)
		}
		if (!L.Who.empty())
		{
			S.Seen.insert(L.Who);
			R.Spoke.push_back({L.Who, Msg.At});
		}
	}
}

void RoomState::Begin(Stream& S, Channel& Ch, const Inputs& In)
{
	// A new night: everything but what carries between streams in this session (the pool, the renewals tracker, the why lines).
	{
		RoomState Fresh;
		Fresh.PoolKey = S.Room->PoolKey;
		Fresh.PoolBuilt = S.Room->PoolBuilt;
		Fresh.PoolAt = std::move(S.Room->PoolAt);
		Fresh.EarnedSubsAtStop = S.Room->EarnedSubsAtStop;
		Fresh.WhyStage = S.Room->WhyStage;
		Fresh.WhyStageRunners = S.Room->WhyStageRunners;
		Fresh.WhyStageDay = S.Room->WhyStageDay;
		Fresh.WhyOverPct = S.Room->WhyOverPct;
		Fresh.WhyOverDay = S.Room->WhyOverDay;
		Fresh.WhyClipDay = S.Room->WhyClipDay;
		Fresh.WhyUnderPct = S.Room->WhyUnderPct;
		Fresh.WhyRecordPeak = S.Room->WhyRecordPeak;
		Fresh.WhyRecordDay = S.Room->WhyRecordDay;
		Fresh.WhyRaidFrom = S.Room->WhyRaidFrom;
		Fresh.WhyRaidSize = S.Room->WhyRaidSize;
		Fresh.WhyRaidDay = S.Room->WhyRaidDay;
		Fresh.WhyMilestone = S.Room->WhyMilestone;
		Fresh.WhyMilestoneDay = S.Room->WhyMilestoneDay;
		Fresh.WhyVideoTitle = S.Room->WhyVideoTitle;
		Fresh.WhyVideoBreakout = S.Room->WhyVideoBreakout;
		Fresh.WhyVideoDay = S.Room->WhyVideoDay;
		Fresh.WhyEpoch = S.Room->WhyEpoch + 1;
		*S.Room = Fresh;
	}
	RoomState& R = *S.Room;
	R.Bind(S, Ch, In);
	R.RoomLive = true;
	// Lazy giving (C-K's M2) for this stream, set before any visit is made; its logs sized for a long night.
	R.GiveLazy = RoomState::EventDriven && RoomState::GiveSeparable();
	R.PresOn = RoomState::EventDriven;
	if (R.GiveLazy)
	{
		R.GiveLog.reserve(1024);
		for (std::vector<double>& CG : R.GiveCG)
		{
			CG.reserve(1024);
		}
		R.Plans.reserve(Ch.Members.size() * 16 + 64);
	}
	R.SeedK = In.Seed;
	R.StreamK = static_cast<uint64_t>(std::max(0, Ch.Streams));
	R.KeyTags();
	R.Day0 = kast_detail::KastDayOf(In.World);
	R.Week0 = kast_detail::KastWeekOf(In.World);
	// The guest pool: 6,000 names from the world seed, once (§2.5.3; the name never depends on the pace or the launch).
	const uint64_t Key = In.Seed ^ (growth::NameKey(In.Hero) * 0x9E3779B97F4A7C15ull);
	if (!R.PoolBuilt || R.PoolKey != Key || S.Pool.size() != kast_room_detail::RoomPoolSize)
	{
		S.Pool.clear();
		Rng Names(std::to_string(In.Seed) + ":kast:pool");
		std::unordered_set<std::string> Taken = {In.Hero, "dee_spincycle", "mei_ng"};
		for (int Tries = 0; S.Pool.size() < kast_room_detail::RoomPoolSize && Tries < 60000; ++Tries)
		{
			std::string Name = handles::Make(Names, handles::PickCountry(Names));
			if (Taken.insert(Name).second)
			{
				S.Pool.push_back(std::move(Name));
			}
		}
		R.PoolBuilt = true;
		R.PoolKey = Key;
		R.PoolAt.clear();
	}
	if (R.PoolAt.size() != S.Pool.size())
	{
		R.PoolAt.clear();
		R.PoolAt.reserve(S.Pool.size() * 2);
		for (size_t I = 0; I < S.Pool.size(); ++I)
		{
			R.PoolAt.emplace(S.Pool[I], static_cast<int>(I));
		}
	}
	R.PoolUntil.assign(S.Pool.size(), -kast_room_detail::RoomInf);
	R.RenewSince = R.EarnedSubsAtStop >= 0 ? std::max<Chips>(0, Ch.EarnedSubs - R.EarnedSubsAtStop) : 0;
	// Heat before the night (§2.4, §8.4): a stream lost to a reload closes with min(GainSoFar, 0) and no show-up term; Rerun's
	// bumps since the last go-live land, each from its own time, at most 12 a world week, and only if the channel streamed in
	// the 7 days before it (§3.3).
	if (Ch.LiveSince >= 0.0)
	{
		kast_room_detail::RoomAddHeat(Ch, In.World, std::min(0.0, Ch.GainSoFar));
	}
	{
		// The week's +12 spans go-lives (synth_kast.py's rerun_heat_week): what the current world week has already given rides in
		// Channel::RerunHeat as one (the week's middle minute, -given) entry beside any bump not yet applied, so two streams in a
		// week can't each take 12 (review, 2026-10-25: the cap was per go-live).
		std::vector<std::pair<double, double>> Bumps;
		std::map<int, double> WeekGiven;
		for (const std::pair<double, double>& B : Ch.RerunHeat)
		{
			if (B.second < 0.0)
			{
				WeekGiven[kast_detail::KastWeekOf(B.first)] += -B.second;
			}
			else
			{
				Bumps.push_back(B);
			}
		}
		for (const std::pair<double, double>& B : In.RerunHeat)
		{
			if (std::find(Bumps.begin(), Bumps.end(), B) == Bumps.end())
			{
				Bumps.push_back(B);
			}
		}
		std::sort(Bumps.begin(), Bumps.end());
		double Add = 0.0;
		const std::pair<double, double>* Newest = nullptr; // the newest bump that counted (Heat's why line names its video)
		for (const std::pair<double, double>& B : Bumps)
		{
			if (B.first > In.World || B.second <= 0.0 || Ch.LastStreamDay < kast_detail::KastDayOf(B.first) - 7)
			{
				continue;
			}
			double& Given = WeekGiven[kast_detail::KastWeekOf(B.first)];
			const double Take = std::min(B.second, std::max(0.0, 12.0 - Given));
			Given += Take;
			Add += Take * std::pow(0.5, std::max(0.0, In.World - B.first) / (5.0 * kast_detail::KastDay));
			Newest = Take > 0.0 ? &B : Newest;
		}
		if (Add > 0.0)
		{
			kast_room_detail::RoomAddHeat(Ch, In.World, Add);
		}
		if (Newest && !In.RerunHeatTitle.empty())
		{
			// "Your video kings, 82%... got pushed hard" (C-K's L5): Rerun's push is +6, a breakout +10 (§2.4).
			R.WhyVideoTitle = In.RerunHeatTitle;
			R.WhyVideoBreakout = Newest->second >= 10.0 - 1.0e-9;
			R.WhyVideoDay = kast_detail::KastDayOf(Newest->first);
			++R.WhyEpoch;
		}
		Ch.RerunHeat.clear();
		// Every bump from here on comes after now, so only this world week's total can still cap one.
		const int WeekNow = kast_detail::KastWeekOf(In.World);
		const auto Now = WeekGiven.find(WeekNow);
		if (Now != WeekGiven.end() && Now->second > 0.0)
		{
			Ch.RerunHeat.emplace_back((static_cast<double>(WeekNow) + 0.5) * 7.0 * kast_detail::KastDay, -Now->second);
		}
	}
	const int PrevStreamDay = Ch.LastStreamDay;
	R.HeatStart = kast_room_detail::RoomShownHeat(Ch, In.World);
	if (Ch.LastStreamDay < 0)
	{
		// The channel's first stream: commit the Heat it reads (growth::HeatShown holds a never-streamed channel at its start, 26),
		// so the night's commits, which read the shown value with LastStreamDay set below (the floor of 20), start from it and not
		// from its decay since the career began (a first stream three days in would fall 26 -> 20 at its first commit).
		Ch.Heat = R.HeatStart;
		Ch.HeatAt = In.World;
	}
	R.TierStart = growth::Tier(R.HeatStart);
	const double H0 = growth::HeatH(R.HeatStart);
	R.WelcomeBack = Ch.Streams > 1 && PrevStreamDay >= 0 && R.Day0 - PrevStreamDay >= 5;
	Ch.LastStreamDay = R.Day0;
	Ch.LiveSince = In.World;
	Ch.GainSoFar = 0.0;
	R.NormBefore = Ch.Norm;
	R.BiasStart = Ch.Bias;
	// The night's terms.
	R.NightQ = Stream::Quality(In);
	R.NightDisc = growth::Disc(Ch.Level(), Ch.Streak);
	R.NightLane = growth::Lane(Ch.Streams);
	R.NightRising = growth::Rising(Ch.Streams, Ch.Norm);
	const double Hours7 = growth::H7(Ch.LiveMin, Ch.LiveDay, kast_room_detail::RoomH7Day(Ch, R.Day0));
	R.NightNovelty = growth::Novelty(Hours7);
	R.NightStamina = growth::Stamina(Hours7);
	R.NightNov = growth::SweatNov(kast_room_detail::RoomFtCount(Ch, R.Day0));
	R.SchedF = kast_room_detail::RoomSchedOf(Ch, S.Scheduled);
	R.Nominal = kast_room_detail::RoomNominal(Ch);
	R.FirstFollowerDone = Ch.Streams != 1;
	// The Ping (§2.3.1): one roll a member, keyed, once per 4 world hours; a restart inside that brings only latecomers.
	R.PingSent = In.World - Ch.PingAt >= kast_room_detail::RoomPingGap;
	if (R.PingSent)
	{
		Ch.PingAt = In.World;
	}
	kast_room_detail::RoomPingNight Night;
	Night.Streams = Ch.Streams;
	Night.Sched = R.SchedF;
	Night.ToD = kast_room_detail::RoomToD(In.World);
	Night.H = H0;
	Night.Today = R.Day0;
	Night.WelcomeBack = R.WelcomeBack;
	const size_t N = Ch.Members.size();
	R.MemberKey.resize(N);
	R.MemberPa.assign(N, 0.0);
	R.VisitOf.assign(N, -1);
	R.MemberAt.clear();
	R.MemberAt.reserve(N * 2 + 16);
	for (size_t I = 0; I < N; ++I)
	{
		R.MemberKey[I] = growth::NameKey(Ch.Members[I].Name);
		R.MemberAt[Ch.Members[I].Name] = static_cast<int>(I);
	}
	R.MemberCanon.resize(N);
	for (size_t I = 0; I < N; ++I)
	{
		R.MemberCanon[I] = R.MemberAt[Ch.Members[I].Name];
	}
	std::vector<double> Attend(N, 0.0);
	std::vector<double> Shares(N, 0.0);
	double Reach = 0.0;
	int Followers = 0;
	double DeeAt = 0.25;
	for (size_t I = 0; I < N; ++I)
	{
		const Member& M = Ch.Members[I];
		const double ModOn = kast_room_detail::RoomModOnline(Ch, M.Name);
		const double Pa = kast_room_detail::RoomPaOf(M, ModOn, Night, In.World);
		R.MemberPa[I] = Pa;
		Attend[I] = R.PingSent ? Pa : Pa * kast_room_detail::RoomLateShare;
		Shares[I] = kast_room_detail::RoomShare(M.Loyalty, M.Friend, Ch.Streams, ModOn > 0.0);
		if (M.Follower)
		{
			const double Days = std::max(0.0, (In.World - M.LastSeen) / kast_detail::KastDay);
			const double W = growth::PingWHot(growth::PingW(Days, ModOn > 0.0 ? 1.0 : M.Loyalty), H0);
			Reach += W;
			++Followers;
			R.PingParts[Days <= 7.0 ? 0 : Days <= 30.0 ? 1 : 2] += 1;
		}
		if (!R.PingSent)
		{
			continue;
		}
		const bool Comes = M.Friend && Ch.Streams <= 1 ? true : growth::Unit(R.SeedK, R.StreamK, growth::Tag::Ping, R.MemberKey[I]) < Pa;
		if (!Comes)
		{
			continue;
		}
		const double U = growth::Unit(R.SeedK, R.StreamK, growth::Tag::Arrive, R.MemberKey[I]);
		double Arrive;
		if (M.Friend && Ch.Streams <= 5)
		{
			const bool Mei = M.Name == "mei_ng";
			Arrive = growth::FriendArrive(Ch.Streams, Mei, DeeAt, U);
			DeeAt = Mei ? DeeAt : Arrive;
		}
		else
		{
			Arrive = growth::ArriveDelay(S.Scheduled, U);
		}
		const int Vi = AddVisit(S, Ch, static_cast<int>(I), Arrive, static_cast<uint8_t>(View::Source::Members));
		if (Vi >= 0 && R.WelcomeBack && M.Loyalty >= 0.1 && (In.World - M.LastSeen) / kast_detail::KastDay >= 5.0)
		{
			R.Visits[static_cast<size_t>(Vi)].WelcomeBack = true;
		}
	}
	// The ping's reach (§2.3.1): every tracked follower by their PingW', the untracked pool at a lapsed follower's.
	const int Untracked = std::max(0, Ch.Followers - Followers);
	Reach += static_cast<double>(Untracked) * growth::PingWHot(0.35, H0);
	R.PingParts[2] += Untracked;
	R.PingReached = std::min(Ch.Followers, static_cast<int>(std::lround(Reach)));
	R.PingOf = Ch.Followers;
	if (R.PingSent)
	{
		S.Tonight.PingParts = R.PingParts; // the live Community page reads tonight's parts (Close books the end card's)
	}
	if (!Ch.Log.empty() && In.World - (Ch.Log.front().Start + Ch.Log.front().Minutes) < kast_detail::KastDay)
	{
		R.LastNightFollows = Ch.Log.front().Follows;
	}
	// The forecast as the go-live had it (§2.11, with the lurker pool: C-K's L8): Heat's lr reads the centre, the end card the band.
	const double A0 = kast_room_detail::RoomA0(Ch, In, R.NightQ, R.NightDisc, R.NightLane, R.NightRising, H0, R.NightNovelty, R.SeedK);
	kast_room_detail::RoomLurkEntries(Ch, Attend, R.SchedF, In.World, Attend, Shares);
	R.Fc = growth::Forecast(Attend, Shares, A0, R.NightQ, Ch.Bias);
	R.FcRaw = R.Fc.Centre; // the raw forecast (Heat's lr); the band is the shown one
	// Front Row at On a heater (§2.4): 30% of streams, a 20-minute window from U(30, 90).
	if (growth::FrontRow(R.HeatStart, growth::Unit(R.SeedK, R.StreamK, growth::Tag::Front, 0)))
	{
		R.FrontAt = 30.0 + 60.0 * growth::Unit(R.SeedK, R.StreamK, growth::Tag::Front, 1);
	}
	R.RankStart = 0;
	R.Clock = 0.0;
	R.Done = 0;
	R.CountAtReal = In.Real;
	play::Begin(*S.Play, Ch, In);
}

void RoomState::SlotAt(Stream& S, Channel& Ch, const Inputs& In, int M)
{
	// A new 10-world-minute slot (§2.3.4): every member coming tonight gets one keyed draw that says whether they leave (here)
	// or come back (away) during it, and when (the draw's own position inside the event), so who is here at any world time is
	// a pure function of the slot's draws. The lurkers are drawn once a slot too (§2.8.6).
	RoomState& R = *S.Room;
	const double T = static_cast<double>(M);
	const uint64_t Slot = static_cast<uint64_t>(M / 10);
	const double Deep = R.RunDepth > 0.0 ? growth::Pst2(R.RunP, R.RunDepth) : 0.0;
	const int Streams = static_cast<int>(R.StreamK);
	if (R.PresOn)
	{
		R.PresSize();
	}
	for (size_t Vi = 0; Vi < R.Visits.size(); ++Vi)
	{
		Visit& V = R.Visits[Vi];
		R.PlanChanging(V, false); // presence is read below, on the new plan, with the member at hand (C-K's M2)
		const bool Here = HereAt(V, T);
		const double Watched = WatchedTo(V, T);
		if (!V.Arrived && (Here || Watched > 0.0))
		{
			V.Arrived = true;
			V.FirstIn = V.Here0 ? V.SlotStart : V.Flip;
		}
		if (Here || V.Pending <= T)
		{
			V.Pending = kast_room_detail::RoomInf;
		}
		V.Watched = Watched;
		V.SlotStart = T;
		V.Here0 = Here && !V.Gone;
		V.Flip = kast_room_detail::RoomInf;
		if (V.Gone)
		{
			continue;
		}
		// The member's loyalty: presence by transition holds it as last read, current unless a write marked the visit since (C-K's
		// M2: the walk reads no member).
		const bool Kept = R.PresOn && Vi < R.PresDirtyFlag.size() && R.PresDirtyFlag[Vi] == 0 && Vi < R.PresL.size();
		const double L = Kept ? R.PresL[Vi] : Ch.Members[static_cast<size_t>(V.Member)].Loyalty;
		if (V.Arrived)
		{
			// The slot's chance (a leave while here, a return while away) is the same function of the same terms as the last slot's
			// whenever they're unchanged: it is kept on the visit (C-K's M2), its exp read once per change. The eager Room reads it
			// every slot (kast_growth's EventDriven compares the two).
			if (!R.PresOn || L != V.SlotL || Here != V.SlotHere || In.Results != V.SlotResults || Deep != V.SlotDeep || V.Mod != V.SlotMod)
			{
				const double Sm = growth::StayMean(L, V.Friend, Streams, V.Mod);
				const double G = growth::GapMean(L, V.Friend, V.Mod, In.Results, Deep);
				V.SlotL = L;
				V.SlotHere = Here;
				V.SlotResults = In.Results;
				V.SlotDeep = Deep;
				V.SlotMod = V.Mod;
				V.SlotChance = 1.0 - std::exp(-kast_room_detail::RoomSlotLen / std::max(1.0e-6, Here ? Sm : G));
			}
			const double U = R.U(growth::Tag::Slot, V.Key, Slot);
			const double Chance = V.SlotChance;
			if (U < Chance)
			{
				V.Flip = T + kast_room_detail::RoomSlotLen * U / Chance;
			}
			if (Here && R.U(growth::Tag::Talker, V.Key, Slot) < 0.25 + 0.5 * L)
			{
				++V.TalkSlots;
			}
		}
		if (!Here && V.Pending < T + kast_room_detail::RoomSlotLen)
		{
			V.Flip = std::min(V.Flip, V.Pending);
		}
		if (R.PresOn)
		{
			R.PresReadAt(static_cast<int>(Vi), T, true, L); // (a visit that is gone stays away: its presence needs no read)
		}
	}
	// The lurker pool (§2.8.6): followers beyond the tracked community, and the tracked ones not coming tonight, at their rates;
	// an honest integer that never speaks. The sum over the followers not coming is walked again only when it can have changed
	// (a visit or a member added, a loyalty moved, a follower crossing into the next PingW step: C-K's M2), in the same order. The
	// eager Room walks it every slot (kast_growth's EventDriven compares the two).
	if (!R.PresOn || R.AwayVisits != R.Visits.size() || R.AwayMembers != Ch.Members.size() || R.AwayLoyalty != R.LoyaltyWrites || T >= R.AwayUntil)
	{
		int Tracked = 0;
		double Away = 0.0;
		double Until = kast_room_detail::RoomInf;
		for (size_t I = 0; I < Ch.Members.size(); ++I)
		{
			const Member& Mb = Ch.Members[I];
			if (!Mb.Follower)
			{
				continue;
			}
			++Tracked;
			if (R.VisitOf[I] < 0)
			{
				const double Days = std::max(0.0, (S.StartWorld + T - Mb.LastSeen) / kast_detail::KastDay);
				Away += growth::PingW(Days, Mb.Loyalty);
				// PingW steps at 7, 21 and 60 days (1.0 from L 0.7): this follower's term holds until the uptime of its next step,
				// less a margin, so the cache never reads past a step on rounding.
				if (Mb.Loyalty < SuperfanLoyalty && Days <= 60.0)
				{
					const double Step = Days <= 7.0 ? 7.0 : Days <= 21.0 ? 21.0 : 60.0;
					Until = std::min(Until, Mb.LastSeen + Step * kast_detail::KastDay - S.StartWorld - 1.0e-3);
				}
			}
		}
		R.AwaySum = Away;
		R.AwayTracked = Tracked;
		R.AwayVisits = R.Visits.size();
		R.AwayMembers = Ch.Members.size();
		R.AwayLoyalty = R.LoyaltyWrites;
		R.AwayUntil = Until;
	}
	const int Tracked = R.AwayTracked;
	const double Away = R.AwaySum;
	const double DeepX = R.RunDepth > 0.0 ? growth::LurkDeep(R.RunP, R.RunDepth) : 1.0;
	const double Mean = growth::LurkerMean(std::max(0, Ch.Followers - Tracked), R.SchedF, kast_detail::TimeOfDay(S.StartWorld + T), Away, DeepX);
	R.LurkersNow = std::max(0, growth::Lurkers(Mean, growth::Unit(R.SeedK, R.StreamK, growth::Tag::Lurk, Slot)));
}

void RoomState::LateAt(Stream& S, Channel& Ch, int M)
{
	// Latecomers (§2.3.2): once a world hour, the members not coming roll their Pa x 0.3 (keyed by the hour); none after 10
	// world hours of uptime.
	RoomState& R = *S.Room;
	const uint64_t Hour = static_cast<uint64_t>(M / 60);
	for (size_t I = 0; I < R.MemberPa.size() && I < Ch.Members.size(); ++I)
	{
		if (R.VisitOf[I] >= 0 || R.MemberPa[I] <= 0.0)
		{
			continue;
		}
		if (R.U(growth::Tag::Late, R.MemberKey[I], Hour * 2) < R.MemberPa[I] * kast_room_detail::RoomLateShare)
		{
			const double Delay = growth::ExpAt(kast_room_detail::RoomLateMean, R.U(growth::Tag::Late, R.MemberKey[I], Hour * 2 + 1));
			AddVisit(S, Ch, static_cast<int>(I), static_cast<double>(M) + Delay, static_cast<uint8_t>(View::Source::Members));
		}
	}
}

void RoomState::ArrivalsAt(Stream& S, Channel& Ch, const Inputs& In, int M, double E)
{
	// Strangers an hour (§2.5.1): today's terms read on the grid (the pace terms retired, C6) with the new ones, then each world
	// minute's arrivals as keyed people (§2.5.3): how many, each one's stay, name and follow decision; railbirds while the run is
	// deep (§2.6); friends that regulars bring (§2.8.3).
	RoomState& R = *S.Room;
	const double T = static_cast<double>(M);
	const double World = S.StartWorld + T;
	const uint64_t Mk = static_cast<uint64_t>(M);
	const TitleSpec& Title = kast_detail::TitleOf(Ch);
	const double V = static_cast<double>(R.Watchers);
	const double Hn = growth::HeatH(kast_room_detail::RoomShownHeat(Ch, World));
	const bool Deep = R.RunDepth > 0.0;
	const double Buzz = growth::Buzz(In.VideoBuzz, Ch.Norm);
	double A = (1.5 + 2.0 * R.NightQ) * R.NightDisc * kast_detail::Competition(World) * (0.4 + 0.6 * kast_detail::TimeOfDay(World) / 1.25) * Title.Discover;
	const double RoomHype = kast_room_detail::RoomHypeBase + R.StageHype;
	A *= 1.0 + RoomHype / 60.0;
	A *= 1.0 + 0.45 * std::log2(1.0 + V / 4.0); // the directory sorts by viewers
	A *= 1.0 + 0.08 * static_cast<double>(std::min(3, std::max(0, In.Tables - 1)));
	A *= In.AtTable ? 1.0 : In.Results ? 0.8 : 0.5;
	A *= Buzz;
	A *= T < R.AdUntilUp ? kast_room_detail::RoomAdRoom : 1.0;
	A *= 0.6 + 0.4 * S.Health;
	A *= R.NightLane * R.NightRising * growth::HeatS(Hn);
	A *= Deep ? growth::Sweat(R.RunP, R.RunDepth, R.NightNov) : 1.0;
	A *= growth::Taper(T, R.RunDepth);
	A *= growth::Tide(growth::Unit(R.SeedK, 0, growth::Tag::Tide, static_cast<uint64_t>(std::max(0, kast_detail::KastDayOf(World)))), !In.SeriesWindow.empty());
	A *= R.FrontAt >= 0.0 && T >= R.FrontAt && T < R.FrontAt + kast_room_detail::RoomFrontLen ? kast_room_detail::RoomFrontX : 1.0;
	A *= growth::Pond(V) * R.NightNovelty;
	A = std::max(0.0, A);
	const double Rail = Deep ? std::max(0.0, growth::Railbirds(Ch.Norm, R.RunP, R.RunDepth, R.NightNov)) : 0.0;
	// Stay and follow (§2.5.2): talking past 0.35 buys nothing; a quiet or unattended stream converts fewer follows (HD).
	const double Ec = std::min(E, 0.35);
	const double Warm = std::min(1.0, static_cast<double>(R.MembersHere) / 8.0);
	const double Stay = std::max(0.5, (4.0 + 6.0 * R.NightQ + 6.0 * Ec + RoomHype / 15.0 + 4.0 * Warm) * (1.0 + In.Gear.Stay) * R.NightStamina);
	// A title's pull on follows reads against the default title's (S-K, 2026-10-24): synth_kast.py's channel is the default one,
	// and the default's own x1.15 ran every archetype x1.15 above its band; the others keep their trade (Teaching x1.13 of it).
	const double TitleFollow = Title.Follow / std::max(0.01, Titles().front().Follow);
	double FollowP = (0.027 + 0.025 * R.NightQ) * (1.0 + In.Gear.Follow) * TitleFollow * (0.6 + 0.4 * S.Health) * (1.0 + 0.6 * Warm) * (1.0 + 0.8 * Ec) * growth::HD(E) *
		growth::Sat(Ch.Followers) * R.NightStamina;
	FollowP *= Deep ? growth::DeepFollow(R.RunP, R.RunDepth) : 1.0;
	R.SeenRing[static_cast<size_t>(M % 30)] = (A + Rail) / 60.0 / std::max(0.01, kast_room_detail::RoomDirCtr * Title.Discover);
	int Came = 0;
	const int N = std::max(0, growth::PoissonAt((A + Rail) / 60.0, growth::Unit(R.SeedK, R.StreamK, growth::Tag::Guest, Mk)));
	for (int J = 0; J < N; ++J)
	{
		const uint64_t Jk = static_cast<uint64_t>(J);
		const double At = T + (static_cast<double>(J) + 0.5) / static_cast<double>(N);
		const bool IsRail = Rail > 0.0 && growth::Unit(R.SeedK, R.StreamK, growth::Tag::Guest, Mk, 1 + 4 * Jk) < Rail / (A + Rail);
		const uint8_t Src = IsRail ? static_cast<uint8_t>(View::Source::Rail) : kast_room_detail::RoomShelf(growth::Unit(R.SeedK, R.StreamK, growth::Tag::Guest, Mk, 2 + 4 * Jk), Buzz, R.NightLane, R.NightRising);
		double Long = growth::ExpAt(IsRail ? kast_room_detail::RoomRailStay : Stay, growth::Unit(R.SeedK, R.StreamK, growth::Tag::GuestStay, Mk, Jk));
		if (R.BankedLeft > 0)
		{
			Long *= R.BankedStay;
			--R.BankedLeft;
		}
		Long = std::max(0.05, Long);
		const bool Follows = growth::Unit(R.SeedK, R.StreamK, growth::Tag::GuestFollow, Mk, Jk) < (IsRail ? kast_room_detail::RoomRailFollow : 1.0) * FollowP;
		const int FollowAt = Follows ? static_cast<int>(std::ceil(At + std::min(0.5 * Long, kast_room_detail::RoomFollowLag))) : -1;
		const int Gi = AddGuest(S, At, At + Long, Src, FollowAt, growth::Unit(R.SeedK, R.StreamK, growth::Tag::GuestName, Mk, Jk), 0);
		++Came;
		// K8 (C-K's L5): a guest from Rerun or a clip sometimes says so as they come in ("came from the kings video"), on its own
		// keyed draw (it moves no other draw) and only with its fact: the video's title, a clip. Display only.
		const bool FromVideo = Src == static_cast<uint8_t>(View::Source::Video) && !In.VideoTitle.empty();
		const bool FromClip = Src == static_cast<uint8_t>(View::Source::Clip);
		if (Gi >= 0 && (FromVideo || FromClip) && R.Guests[static_cast<size_t>(Gi)].Pool >= 0 &&
			R.U(growth::Tag::GuestName, Mk, kast_room_detail::RoomSourceLineKey + Jk) < kast_room_detail::RoomSourceLineShare)
		{
			Line L;
			L.Who = S.Pool[static_cast<size_t>(R.Guests[static_cast<size_t>(Gi)].Pool)];
			L.Text = FromVideo ? copy::CameFromVideo(In.VideoTitle) : copy::CameFromClip();
			L.World = At; // they say it as they come in (the line waits for them, and goes up only if they're here)
			R.DueLines.push_back(L);
		}
	}
	// Word of mouth (§2.8.3): members with L > 0.4 bring friends at 0.05 sum (L - 0.4) an hour, x2 from stage 3; each a guest
	// who follows at 0.35 x Sat. The fraction carries (Channel::CarryWord).
	double Bring = 0.0;
	if (R.HereFresh && R.HereT == T && R.HereLoyalty == R.LoyaltyWrites)
	{
		Bring = R.HereBring; // the minute's Recount summed the same visits in the same order at the same loyalties (C-K's M2)
	}
	else
	{
		for (const Visit& Vs : R.Visits)
		{
			if (HereAt(Vs, T))
			{
				Bring += std::max(0.0, Ch.Members[static_cast<size_t>(Vs.Member)].Loyalty - 0.4);
			}
		}
	}
	Ch.CarryWord = std::min(kast_room_detail::RoomCarryCap, Ch.CarryWord + Bring * 0.05 * (Ch.Stage >= 3 ? 2.0 : 1.0) / 60.0);
	for (uint64_t W = 0; Ch.CarryWord >= 1.0; ++W)
	{
		Ch.CarryWord -= 1.0;
		const double At = T + 0.5;
		const double Long = std::max(0.05, growth::ExpAt(Stay, growth::Unit(R.SeedK, R.StreamK, growth::Tag::GuestStay, Mk, 1000 + W)));
		const bool Follows = growth::Unit(R.SeedK, R.StreamK, growth::Tag::GuestFollow, Mk, 1000 + W) < kast_room_detail::RoomWordFollow * growth::Sat(Ch.Followers);
		AddGuest(S, At, At + Long, static_cast<uint8_t>(View::Source::Word), Follows ? static_cast<int>(std::ceil(At + std::min(0.5 * Long, kast_room_detail::RoomFollowLag))) : -1,
			growth::Unit(R.SeedK, R.StreamK, growth::Tag::GuestName, Mk, 1000 + W), 0);
		++Came;
	}
	R.CameRing[static_cast<size_t>(M % 30)] = Came;
}

void RoomState::RaidsAt(Stream& S, Channel& Ch, const Inputs& In, int M)
{
	// The network's raids in (§2.7.1): an hourly rate from the tokens the channel's raids out earned; a raid back spends one. The
	// welcome raid: the first stream from stream 3 on that reaches world minute 40 gets 8 from a network channel.
	RoomState& R = *S.Room;
	const double T = static_cast<double>(M);
	const double World = S.StartWorld + T;
	const uint64_t Mk = static_cast<uint64_t>(M);
	if (!Ch.WelcomeRaidDone && R.StreamK >= kast_room_detail::RoomWelcomeRaidFrom && M == kast_room_detail::RoomWelcomeRaidMinute)
	{
		Ch.WelcomeRaidDone = true;
		const std::vector<SmallChannel> Net = Network(World);
		std::string From = "pocketqueens_pk";
		uint32_t Color = Violet;
		if (!Net.empty())
		{
			const SmallChannel& C = Net[static_cast<size_t>(std::min<double>(static_cast<double>(Net.size()) - 1.0, std::floor(growth::Unit(R.SeedK, R.StreamK, growth::Tag::Raid, Mk, 7) * static_cast<double>(Net.size()))))];
			From = C.Name;
			Color = C.Color;
		}
		AddRaid(S, Ch, In, From, Color, kast_room_detail::RoomWelcomeRaidSize, false, T + 0.5, 2);
		return;
	}
	int Tokens = 0;
	for (const auto& G : Ch.Goodwill)
	{
		Tokens += std::min(3, std::max(0, G.second));
	}
	const double Rate = growth::RaidRate(Tokens, R.NightDisc, growth::HeatH(kast_room_detail::RoomShownHeat(Ch, World)));
	if (growth::Unit(R.SeedK, R.StreamK, growth::Tag::Raid, Mk) >= Rate / 60.0)
	{
		return;
	}
	const std::vector<SmallChannel> Net = Network(World);
	// The raider: a live channel that owes a raid, else one that owes (it just ended its stream), else whoever is ending theirs.
	std::vector<std::string> Owing;
	std::vector<std::string> OwingLive;
	for (const auto& G : Ch.Goodwill)
	{
		if (G.second > 0)
		{
			Owing.push_back(G.first);
			for (const SmallChannel& C : Net)
			{
				if (C.Name == G.first)
				{
					OwingLive.push_back(G.first);
					break;
				}
			}
		}
	}
	const double Pick = growth::Unit(R.SeedK, R.StreamK, growth::Tag::Raid, Mk, 3);
	std::string From;
	uint32_t Color = Violet;
	const std::vector<std::string>& From1 = !OwingLive.empty() ? OwingLive : Owing;
	if (!From1.empty())
	{
		From = From1[static_cast<size_t>(std::min<double>(static_cast<double>(From1.size()) - 1.0, std::floor(Pick * static_cast<double>(From1.size()))))];
		Ch.Goodwill[From] = std::max(0, std::min(3, Ch.Goodwill[From]) - 1); // a raid back spends the token
	}
	else if (!Net.empty())
	{
		From = Net[static_cast<size_t>(std::min<double>(static_cast<double>(Net.size()) - 1.0, std::floor(Pick * static_cast<double>(Net.size()))))].Name;
	}
	if (From.empty())
	{
		return;
	}
	for (const SmallChannel& C : Net)
	{
		Color = C.Name == From ? C.Color : Color;
	}
	const int Size = std::max(1, static_cast<int>(std::lround(growth::RaidSize(growth::Unit(R.SeedK, R.StreamK, growth::Tag::Raid, Mk, 1), growth::Unit(R.SeedK, R.StreamK, growth::Tag::Raid, Mk, 2)))));
	AddRaid(S, Ch, In, From, Color, Size, false, T + 0.5, 0);
}

void RoomState::TrollStepAt(Stream& S, Channel& Ch, const Inputs& In, int M)
{
	// Trolls and spam on the grid (§1.2 R1, §2.12): a keyed arrival a world minute at today's real-second rate on Smart's clock;
	// a troll is a person in the room, and spam bots only find streams someone is watching (Count >= 1). The catches are decided
	// when they arrive: WardenBot, then each moderator here; a miss costs the room's health now.
	RoomState& R = *S.Room;
	const double T = static_cast<double>(M);
	const uint64_t Mk = static_cast<uint64_t>(M);
	const double Strangers = static_cast<double>(R.GuestsHere + R.CrowdHere);
	// The room's hype (14 + the run's stage hits), as the strangers' rate reads it, not the meter: the meter takes Hold or Crack's
	// reveal (+8, only at attended minutes), a badge's chat line and the like, and through the trolls and chat Health it ran the
	// room differently at 1x and at watch 4x for identical inputs (C6, R3; review 2026-10-25, kast_growth's PaceFlags).
	const double Rate = (0.0005 + 0.00008 * static_cast<double>(R.Watchers) + 0.0003 * Strangers) * (1.0 + (kast_room_detail::RoomHypeBase + R.StageHype) / 50.0) *
		kast_detail::TitleOf(Ch).Calm * (Ch.Partner ? 1.1 : 1.0) * kast_room_detail::RoomSmartReal;
	if (growth::Unit(R.SeedK, R.StreamK, growth::Tag::Troll, Mk) >= std::min(0.9, Rate))
	{
		return;
	}
	const bool IsSpam = growth::Unit(R.SeedK, R.StreamK, growth::Tag::Troll, Mk, 1) < 0.3;
	if (IsSpam && R.Watchers < 1)
	{
		return; // bots go where viewers are
	}
	const double At = T + 0.5;
	const double Stay = std::max(0.2, growth::ExpAt(kast_room_detail::RoomTrollStay, growth::Unit(R.SeedK, R.StreamK, growth::Tag::Troll, Mk, 5)));
	const int Gi = AddGuest(S, At, At + Stay, static_cast<uint8_t>(View::Source::Directory), -1, growth::Unit(R.SeedK, R.StreamK, growth::Tag::Troll, Mk, 4), IsSpam ? 2 : 1);
	if (Gi < 0)
	{
		return;
	}
	Guest& G = R.Guests[static_cast<size_t>(Gi)];
	if (IsSpam)
	{
		static const char* const Bots[] = {"v1ewz_b0t", "free_chips_4u", "promo_kingz", "k4st_boost", "followerz_cheap"};
		const int B = std::min(4, static_cast<int>(std::floor(growth::Unit(R.SeedK, R.StreamK, growth::Tag::Troll, Mk, 2) * 5.0)));
		const int D = 10 + std::min(89, static_cast<int>(std::floor(growth::Unit(R.SeedK, R.StreamK, growth::Tag::Troll, Mk, 3) * 90.0)));
		G.Bot = std::string(Bots[B]) + std::to_string(D);
	}
	const std::string Who = G.Pool >= 0 ? S.Pool[static_cast<size_t>(G.Pool)] : G.Bot;
	Line L;
	L.Who = Who;
	L.Text = IsSpam ? S.R.Pick(kast_detail::Spam()) : kast_detail::Fill(S.R.Pick(kast_detail::Trolls()), "{hero}", In.Hero);
	L.Toxic = !IsSpam;
	L.Spam = IsSpam;
	L.World = At; // they say it as they come in
	std::string By;
	if (growth::Unit(R.SeedK, R.StreamK, growth::Tag::Troll, Mk, 6) < In.Gear.ModBot * (IsSpam ? 1.1 : 0.9))
	{
		By = "WardenBot";
	}
	for (size_t I = 0; By.empty() && I < R.ModsHere.size(); ++I)
	{
		if (growth::Unit(R.SeedK, R.StreamK, growth::Tag::Troll, Mk, 7 + I) < 0.45)
		{
			By = Ch.Members[static_cast<size_t>(R.ModsHere[I])].Name;
		}
	}
	if (!By.empty())
	{
		// Caught: the line comes down a moment after it lands and they're timed out (they leave).
		L.DeleteAfter = By == "WardenBot" ? 0.4 : 3.0;
		L.DeletedBy = By;
		G.Leave = std::min(G.Leave, At + 0.2);
		if (G.Pool >= 0)
		{
			R.PoolUntil[static_cast<size_t>(G.Pool)] = At + kast_room_detail::RoomTimeoutWorld;
		}
		++S.Caught;
		for (Moderator& Mod : Ch.Mods)
		{
			Mod.Actions += Mod.Name == By ? 1 : 0;
		}
	}
	else
	{
		L.DeleteAfter = -2.0;
		++S.Missed;
		S.Health = std::max(0.0, S.Health - (IsSpam ? 0.03 : 0.05));
	}
	R.DueLines.push_back(L);
}

// ---- per-visit giving (§2.10.1) and its lazy replay (C-K's M2; KastRoom.h says how it stays exact)

bool RoomState::GiveStep(Stream& S, Channel& Ch, const Inputs& In, Visit& V, const PlanView& P, double T, const GiveMinute& G, double BigAt)
{
	RoomState& R = *S.Room;
	const bool Here = P.Here0 != (T >= P.Flip); // HereAt on the plan the minute had
	if (!Here || (V.Gave[0] && (!G.Aff || (V.Gave[1] && (V.Sub || V.Gave[2])))))
	{
		return false; // away, or nothing left to give tonight
	}
	// WatchedTo on that plan.
	const double W = T <= P.SlotStart ? P.Watched : P.Here0 ? P.Watched + (std::min(T, P.Flip) - P.SlotStart) : P.Watched + std::max(0.0, T - std::max(P.Flip, P.SlotStart));
	if (W < kast_room_detail::RoomTipGate)
	{
		return false; // every gift waits for 20 watched minutes (a new sub for 30)
	}
	Member& Mb = Ch.Members[static_cast<size_t>(V.Member)];
	const bool Near = T - BigAt <= kast_room_detail::RoomGiveWindow;
	const double Days = Mb.GaveDay < 0 ? 1.0e6 : static_cast<double>(G.Day - Mb.GaveDay);
	if (Days != V.DilDays)
	{
		V.DilDays = Days;
		V.DilMember = growth::Dil(0.0, Days);
	}
	const double Dil = G.DilRoom * V.DilMember;
	const double Span = std::max(10.0, V.Expect - kast_room_detail::RoomTipGate);
	// A visit's chance (growth::TipChance, CheerChance, SubChance) spread over its expected minutes, bunched after a big moment.
	const double Shape = (Near ? G.WN : G.WF) / Span;
	// The giving terms as last read (the frame budget): a minute with the same loyalty, hype and Engage reuses them.
	auto Terms = [&]() {
		if (Mb.Loyalty != V.GiveL || G.Hype != V.GiveHype || G.E != V.GiveE)
		{
			V.GiveL = Mb.Loyalty;
			V.GiveHype = G.Hype;
			V.GiveE = G.E;
			V.Tip1 = growth::TipChance(Mb.Loyalty, G.Hype, G.E, 1.0);
			V.Cheer1 = growth::CheerChance(Mb.Loyalty, G.Hype, G.E, 1.0);
			V.Sub1 = growth::SubChance(Mb.Loyalty, G.Hype, G.E);
		}
	};
	bool Fired = false;
	if (W >= kast_room_detail::RoomTipGate && !V.Gave[0])
	{
		Terms();
		V.GiveH[0] += std::max(0.0, V.Tip1 * Dil) * Shape;
		if (V.GiveH[0] >= kast_room_detail::RoomGiveNeed(R, V, 0))
		{
			Fired = true;
			V.Gave[0] = true;
			const Chips Amount = kast_room_detail::RoomTipCents[std::min(12, static_cast<int>(std::floor(growth::Unit(R.SeedK, R.StreamK, growth::Tag::Give, V.Key, 2) * 13.0)))];
			std::string Msg = kast_detail::Fill(S.R.Pick(kast_detail::Tips()), "{hero}", In.Hero);
			if (In.Gear.MicTier == 0 && S.R.Chance(0.3))
			{
				Msg = "buy a real mic with this";
			}
			Earned(S, Ch, Amount, 2);
			Mb.Given += Amount;
			Mb.GaveDay = G.Day;
			Alert A;
			A.Kind = AlertKind::Tip;
			A.Who = Mb.Name;
			A.Cents = Amount;
			A.Text = Msg;
			S.Queue(A);
			Line L;
			L.Who = Mb.Name;
			L.Text = Money(Amount) + ": " + Msg;
			L.Kind = LineKind::Tip;
			L.Cents = Amount;
			R.DueLines.push_back(L);
			S.Hype = std::min(100.0, S.Hype + std::min(15.0, static_cast<double>(Amount) / 500.0));
		}
	}
	if (G.Aff && W >= kast_room_detail::RoomTipGate && !V.Gave[1])
	{
		Terms();
		V.GiveH[1] += std::max(0.0, V.Cheer1 * Dil) * Shape;
		if (V.GiveH[1] >= kast_room_detail::RoomGiveNeed(R, V, 1))
		{
			Fired = true;
			V.Gave[1] = true;
			const int Bits = kast_room_detail::RoomCheerBits[std::min(11, static_cast<int>(std::floor(growth::Unit(R.SeedK, R.StreamK, growth::Tag::Give, V.Key, 3) * 12.0)))];
			Earned(S, Ch, Bits, 1);
			Mb.Given += Bits;
			Mb.GaveDay = G.Day;
			Alert A;
			A.Kind = AlertKind::Cheer;
			A.Who = Mb.Name;
			A.Count = Bits;
			A.Cents = Bits;
			A.Text = S.R.Pick(kast_detail::Cheers());
			S.Queue(A);
			Line L;
			L.Who = Mb.Name;
			L.Text = "Cheer" + std::to_string(Bits) + " " + A.Text;
			L.Kind = LineKind::Cheer;
			L.Cents = Bits;
			R.DueLines.push_back(L);
		}
	}
	if (G.Aff && !V.Sub && W >= kast_room_detail::RoomSubGate && !V.Gave[2])
	{
		const double SubSpan = std::max(10.0, V.Expect - kast_room_detail::RoomSubGate);
		Terms();
		V.GiveH[2] += std::max(0.0, V.Sub1) * Shape * Span / SubSpan;
		if (V.GiveH[2] >= kast_room_detail::RoomGiveNeed(R, V, 2))
		{
			Fired = true;
			V.Gave[2] = true;
			V.Sub = true;
			S.Subscribe(Ch, In, Mb.Name, false, 1);
		}
	}
	return Fired;
}

bool RoomState::EventDriven = true;
std::atomic<uint64_t> RoomState::GiveLate{0};

bool RoomState::GiveSeparable()
{
	// The bound reads each kind's chance at loyalty L as GiveRatio(L) x its chance at L 1, for every hype and Engage: true of
	// growth::TipChance, CheerChance and SubChance as written (L's factor times the rest). Checked once over a grid of each; if a
	// formula ever stops being separable, every stream gives eagerly (and kast_growth's EventDriven says so).
	static const bool Ok = [] {
		const double Ls[] = {0.0, 0.03, 0.04, 0.1, 0.2, 0.35, 0.5, 0.7, 0.85, 1.0};
		const double Hs[] = {0.0, 14.0, 24.0, 30.0, 40.0, 59.0, 100.0, 140.0};
		const double Es[] = {0.0, 0.05, 0.12, 0.15, 0.2, 0.3, 0.45, 0.6, 1.0};
		for (int K = 0; K < 3; ++K)
		{
			for (const double L : Ls)
			{
				for (const double H : Hs)
				{
					for (const double E : Es)
					{
						const double One = K == 0 ? growth::TipChance(1.0, H, E, 1.0) : K == 1 ? growth::CheerChance(1.0, H, E, 1.0) : growth::SubChance(1.0, H, E);
						const double At = K == 0 ? growth::TipChance(L, H, E, 1.0) : K == 1 ? growth::CheerChance(L, H, E, 1.0) : growth::SubChance(L, H, E);
						if (!(One >= 0.0) || !(At >= 0.0) || std::fabs(At - GiveRatio(K, L) * One) > 1.0e-9 * One)
						{
							return false;
						}
					}
				}
			}
		}
		return true;
	}();
	return Ok;
}

double RoomState::GiveRatio(int Kind, double L)
{
	constexpr double H = 14.0, E = 0.2;
	const double One = Kind == 0 ? growth::TipChance(1.0, H, E, 1.0) : Kind == 1 ? growth::CheerChance(1.0, H, E, 1.0) : growth::SubChance(1.0, H, E);
	const double At = Kind == 0 ? growth::TipChance(L, H, E, 1.0) : Kind == 1 ? growth::CheerChance(L, H, E, 1.0) : growth::SubChance(L, H, E);
	return One > 0.0 ? At / One : 0.0;
}

void RoomState::PlanChanging(Visit& V, bool MarkPres)
{
	if (PresOn && MarkPres)
	{
		PresMark(static_cast<int>(&V - Visits.data())); // its presence is read again at the next Recount
	}
	// The plan in effect until now is kept only while the replay may still need it: minutes it hasn't run under this plan, or big
	// moments it hasn't placed. A visit that is gone (away for good either way) or has nothing left to give needs none.
	if (!GiveLazy || V.Gone || (V.Gave[0] && V.Gave[1] && (V.Sub || V.Gave[2])) || (V.GiveUpto >= GiveDone && V.MomentNext >= BigMoments.size()))
	{
		return;
	}
	PlanSnap P;
	P.View = PlanOf(V);
	P.EndSeq = ++GiveSeq;
	P.Prev = V.PlanTail;
	Plans.push_back(P);
	V.PlanTail = static_cast<int>(Plans.size()) - 1;
}

void RoomState::GiveNew(const Channel& Ch, int Vi)
{
	Visit& V = Visits[static_cast<size_t>(Vi)];
	if (PresOn)
	{
		PresMark(Vi);
		if (V.Mod)
		{
			ModVisits.push_back(Vi); // visits are only appended: the list stays in visit order
		}
	}
	if (!GiveLazy)
	{
		return;
	}
	V.GiveUpto = GiveDone; // a visit made now gives from the next minute that runs
	V.GiveBigAt = V.BigAt;
	V.MomentNext = BigMoments.size();
	V.PlanStop = V.PlanTail = -1;
	GiveAnchor(Ch, Vi);
}

void RoomState::GiveReplay(Stream& S, Channel& Ch, const Inputs& In, int Vi, int To, bool FireAtTo)
{
	// The visit's minutes after GiveUpto through To, each with its own terms, the plan it had then (the first snapshot that ended
	// after the minute, else the current plan) and BigAt as the big moments before it left it.
	std::vector<int> Own;
	std::vector<int>& Snaps = PlanScratchBusy ? Own : PlanScratch; // (a replay inside a replay never happens; if it did, its own list)
	const bool Outer = !PlanScratchBusy;
	PlanScratchBusy = true;
	Snaps.clear();
	for (int P = Visits[static_cast<size_t>(Vi)].PlanTail; P >= 0 && P != Visits[static_cast<size_t>(Vi)].PlanStop; P = Plans[static_cast<size_t>(P)].Prev)
	{
		Snaps.push_back(P);
	}
	std::reverse(Snaps.begin(), Snaps.end());
	size_t P = 0;
	size_t K = Visits[static_cast<size_t>(Vi)].MomentNext;
	double BigAt = Visits[static_cast<size_t>(Vi)].GiveBigAt;
	auto PlanAt = [&](uint64_t Seq) {
		while (P < Snaps.size() && Plans[static_cast<size_t>(Snaps[P])].EndSeq < Seq)
		{
			++P;
		}
		return P < Snaps.size() ? Plans[static_cast<size_t>(Snaps[P])].View : PlanOf(Visits[static_cast<size_t>(Vi)]);
	};
	for (int J = Visits[static_cast<size_t>(Vi)].GiveUpto + 1; J <= To && static_cast<size_t>(J) < GiveLog.size(); ++J)
	{
		const GiveMinute& G = GiveLog[static_cast<size_t>(J)];
		while (K < BigMoments.size() && BigMoments[K].Seq < G.Seq)
		{
			const PlanView At = PlanAt(BigMoments[K].Seq);
			if (At.Here0 != (BigMoments[K].T >= At.Flip))
			{
				BigAt = BigMoments[K].T;
			}
			++K;
		}
		const PlanView At = PlanAt(G.Seq);
		Visit& V = Visits[static_cast<size_t>(Vi)];
		V.GiveUpto = J; // before the step: a gift's loyalty write syncs this visit, which is then current
		if (P > 0)
		{
			V.PlanStop = Snaps[P - 1]; // the snapshots before the minute's plan are passed
		}
		V.MomentNext = K;
		V.GiveBigAt = BigAt;
		if (GiveStep(S, Ch, In, V, At, static_cast<double>(J), G, BigAt) && !(FireAtTo && J == To))
		{
			++GiveLate; // a gift before the minute that asked: the bound didn't hold (kast_growth asserts this never happens)
		}
	}
	if (Outer)
	{
		PlanScratchBusy = false;
	}
}

void RoomState::GiveAnchor(const Channel& Ch, int Vi)
{
	// Each kind still open gets its bound from here: its sum now plus Ratio(L) / Span x the room's running bound sum (x1.001, a
	// margin far past rounding), so the minute the running sum passes Theta is the first that could cross the threshold.
	Visit& V = Visits[static_cast<size_t>(Vi)];
	++V.GiveVer;
	if (!GiveLazy || V.Member < 0 || static_cast<size_t>(V.Member) >= Ch.Members.size())
	{
		return;
	}
	const double L = Ch.Members[static_cast<size_t>(V.Member)].Loyalty;
	for (int K = 0; K < 3; ++K)
	{
		if (V.Gave[static_cast<size_t>(K)] || (K == 2 && V.Sub))
		{
			continue;
		}
		const double Need = kast_room_detail::RoomGiveNeed(*this, V, K);
		const double Span = std::max(10.0, V.Expect - (K == 2 ? kast_room_detail::RoomSubGate : kast_room_detail::RoomTipGate));
		const double Coef = GiveRatio(K, L) / Span * 1.001;
		const double CG0 = GiveCGAt(K, V.GiveUpto);
		const double Left = Need - V.GiveH[static_cast<size_t>(K)];
		double Theta;
		if (Left <= 0.0)
		{
			Theta = CG0;
		}
		else if (Coef > 0.0)
		{
			Theta = CG0 + Left / Coef;
			Theta -= 4.0e-12 * (std::fabs(Theta) + 1.0); // the running sums' rounding, on the early side
		}
		else
		{
			continue; // a chance of 0: it can't reach a threshold above its sum
		}
		GiveDue Due;
		Due.Theta = Theta;
		Due.Visit = Vi;
		Due.Ver = V.GiveVer;
		std::vector<GiveDue>& Heap = GiveHeap[static_cast<size_t>(K)];
		Heap.push_back(Due);
		std::push_heap(Heap.begin(), Heap.end(), std::greater<GiveDue>());
	}
}

void RoomState::GiveSync(Stream& S, Channel& Ch, int Vi)
{
	if (!GiveLazy || Vi < 0 || static_cast<size_t>(Vi) >= Visits.size() || Visits[static_cast<size_t>(Vi)].GiveUpto >= GiveDone)
	{
		return;
	}
	GiveReplay(S, Ch, LastIn, Vi, GiveDone, false);
	GiveAnchor(Ch, Vi);
}

void RoomState::LoyaltyWrite(Stream& S, Channel& Ch, Member& M, double NewL)
{
	// A loyalty the stream changes while live: the member's giving is brought up to now under the old one, then bounded under the
	// new one (lazy giving); the cached sums over loyalties read again (LoyaltyWrites).
	const std::ptrdiff_t Mi = &M - Ch.Members.data();
	const int Vi = Mi >= 0 && static_cast<size_t>(Mi) < VisitOf.size() && static_cast<size_t>(Mi) < Ch.Members.size() ? VisitOf[static_cast<size_t>(Mi)] : -1;
	if (Vi >= 0)
	{
		GiveSync(S, Ch, Vi);
	}
	M.Loyalty = NewL;
	++LoyaltyWrites;
	if (Vi >= 0 && PresOn)
	{
		PresMark(Vi); // a regular now, or no longer; word of mouth's term
	}
	if (Vi >= 0 && GiveLazy)
	{
		GiveAnchor(Ch, Vi);
	}
}

void RoomState::GiveSyncAll(Stream& S, Channel& Ch)
{
	for (size_t Vi = 0; Vi < Visits.size(); ++Vi)
	{
		GiveSync(S, Ch, static_cast<int>(Vi));
	}
}

void RoomState::GivingAt(Stream& S, Channel& Ch, const Inputs& In, int M, double E)
{
	// Kast money (§2.10): each member's visit gives at most once a kind (a tip, a cheer, a new sub), as a keyed hazard on the
	// minutes they're here (nothing reads the future); strangers and lurkers by the hour, carried; ads once a world hour; the
	// sponsors' live hours (C13: their own ledger, never the Kast payout).
	RoomState& R = *S.Room;
	const double T = static_cast<double>(M);
	const double World = S.StartWorld + T;
	const int Day = kast_detail::KastDayOf(World);
	const double Hype = kast_room_detail::RoomHypeBase + R.StageHype; // the room's hype (§2.10.1's PeakHype at the minute)
	const double AvgPresent = R.PresentSum / static_cast<double>(std::max(1, M + 1));
	const bool Aff = Ch.Affiliate;
	// The bunching weight's base (S-K, 2026-10-24): the share of tonight's grid minutes after a big moment. growth::GiveHazard's
	// fixed x2.5 / x0.75 keeps a visit's chance whole only at one big moment in 40 minutes; with two to four tables a big
	// moment comes every few minutes and the fixed weights paid members x1.8-2.5 their chance.
	++R.GiveMinutes;
	R.NearMinutes += T - R.BigAtRoom <= kast_room_detail::RoomGiveWindow ? 1 : 0;
	// The minute's shared terms, once (the frame budget, §9.4 #13): the bunching weight near and far, and Dil's room factor.
	// growth::Dil is the room's (1 + AvgPresent/90)^-0.5 times the member's 1 - e^(-days/21): the member's is exactly 1 at 10^6
	// days and the room's exactly 1 in an empty room, so the two calls' product is Dil's own to the bit, as TipChance and
	// CheerChance at Dilution 1 times Dil are theirs (their last factor is the dilution; kast_growth's GiveTerms checks all three).
	GiveMinute G;
	G.Hype = Hype;
	G.E = E;
	G.DilRoom = growth::Dil(AvgPresent, 1.0e6);
	G.WN = kast_room_detail::RoomGiveWeight(R.NearMinutes, R.GiveMinutes, true);
	G.WF = kast_room_detail::RoomGiveWeight(R.NearMinutes, R.GiveMinutes, false);
	G.Day = Day;
	G.Aff = Aff;
	if (R.GiveLazy)
	{
		// Lazy giving (C-K's M2): the minute's terms and each kind's bound term go on the logs; the visits whose bound can reach
		// their threshold by now are replayed to this minute, in visit order, and give on it exactly as the loop below would.
		G.Seq = ++R.GiveSeq;
		if (R.GiveLog.size() != static_cast<size_t>(M))
		{
			R.GiveLog.resize(static_cast<size_t>(M)); // never: the grid runs every minute from 0 (the logs index by minute)
		}
		R.GiveLog.push_back(G);
		// A visit is near a big moment only inside the 6 world minutes after one it was here for, so never when the room's last big
		// moment is older: the bound's weight is the far one then.
		const double WMax = T - R.BigAtRoom <= kast_room_detail::RoomGiveWindow ? std::max(G.WN, G.WF) : G.WF;
		const double Term[3] = {growth::TipChance(1.0, Hype, E, 1.0) * G.DilRoom * WMax, Aff ? growth::CheerChance(1.0, Hype, E, 1.0) * G.DilRoom * WMax : 0.0,
			Aff ? growth::SubChance(1.0, Hype, E) * WMax : 0.0};
		R.GiveDueNow.clear();
		for (size_t K = 0; K < 3; ++K)
		{
			std::vector<double>& CG = R.GiveCG[K];
			CG.resize(static_cast<size_t>(M), CG.empty() ? 0.0 : CG.back());
			CG.push_back((CG.empty() ? 0.0 : CG.back()) + std::max(0.0, Term[K]));
			std::vector<GiveDue>& Heap = R.GiveHeap[K];
			while (!Heap.empty() && Heap.front().Theta <= CG.back())
			{
				const GiveDue Due = Heap.front();
				std::pop_heap(Heap.begin(), Heap.end(), std::greater<GiveDue>());
				Heap.pop_back();
				if (Due.Visit >= 0 && static_cast<size_t>(Due.Visit) < R.Visits.size() && R.Visits[static_cast<size_t>(Due.Visit)].GiveVer == Due.Ver)
				{
					R.GiveDueNow.push_back(Due.Visit);
				}
			}
		}
		std::sort(R.GiveDueNow.begin(), R.GiveDueNow.end());
		R.GiveDueNow.erase(std::unique(R.GiveDueNow.begin(), R.GiveDueNow.end()), R.GiveDueNow.end());
		for (const int Vi : R.GiveDueNow)
		{
			R.GiveReplay(S, Ch, In, Vi, M, true);
			R.GiveAnchor(Ch, Vi);
		}
		R.GiveDone = M;
	}
	else
	{
		// The visits here, in order: the minute's Recount listed them (C-K's M2); any other caller walks them all.
		const bool Listed = R.HereFresh && R.HereT == T;
		const size_t Walk = Listed ? R.HereIx.size() : R.Visits.size();
		for (size_t K = 0; K < Walk; ++K)
		{
			Visit& V = R.Visits[Listed ? static_cast<size_t>(R.HereIx[K]) : K];
			GiveStep(S, Ch, In, V, PlanOf(V), T, G, V.BigAt);
		}
		R.GiveDone = M;
	}
	// Dee and Mei subscribe the night Affiliate lands (§2.10.3), when they're here.
	if (R.FriendsSubPending)
	{
		bool Left = false;
		for (const char* Friend : {"dee_spincycle", "mei_ng"})
		{
			const auto It = R.MemberAt.find(Friend);
			if (It == R.MemberAt.end())
			{
				continue;
			}
			const int Vi = R.VisitOf[static_cast<size_t>(It->second)];
			if (Vi >= 0)
			{
				R.GiveSync(S, Ch, Vi); // her giving through this minute (her Sub may have come from it)
			}
			if (Vi < 0 || R.Visits[static_cast<size_t>(Vi)].Sub)
			{
				continue;
			}
			if (HereAt(R.Visits[static_cast<size_t>(Vi)], T))
			{
				R.Visits[static_cast<size_t>(Vi)].Sub = true;
				S.Subscribe(Ch, In, Friend, false, 1);
			}
			else
			{
				Left = true;
			}
		}
		R.FriendsSubPending = Left;
	}
	// Strangers and lurkers (§2.10.1), carried in Channel::Carry*: the giver is a named stranger here; with nobody named here the
	// money waits (F5b).
	const double Strangers = static_cast<double>(R.GuestsHere + R.CrowdHere + R.RaidersHere);
	const double EngT = growth::EngT(E);
	const double Mass = Strangers + static_cast<double>(R.LurkersNow);
	Ch.CarryTip = std::min(kast_room_detail::RoomCarryCap, Ch.CarryTip + Mass * kast_room_detail::RoomTipStranger / 60.0 * EngT);
	if (Aff)
	{
		Ch.CarryCheer = std::min(kast_room_detail::RoomCarryCap, Ch.CarryCheer + Mass * kast_room_detail::RoomCheerStranger * (1.0 + Hype / 50.0) / 60.0 * EngT);
		Ch.CarrySub = std::min(kast_room_detail::RoomCarryCap, Ch.CarrySub + Mass * kast_room_detail::RoomSubStranger / 60.0);
	}
	auto Giver = [&](uint64_t K) -> std::string {
		std::vector<int> Named;
		for (size_t I = 0; I < R.Guests.size(); ++I)
		{
			if (R.Guests[I].Kind == 0 && R.Guests[I].Pool >= 0 && R.GuestHere(R.Guests[I], T))
			{
				Named.push_back(R.Guests[I].Pool);
			}
		}
		if (Named.empty())
		{
			return std::string();
		}
		const double U = growth::Unit(R.SeedK, R.StreamK, growth::Tag::Give, static_cast<uint64_t>(M) + (1ull << 40), K);
		return S.Pool[static_cast<size_t>(Named[std::min(Named.size() - 1, static_cast<size_t>(std::floor(U * static_cast<double>(Named.size()))))])];
	};
	for (uint64_t K = 0; Ch.CarryTip >= 1.0; ++K)
	{
		const std::string Who = Giver(K * 3);
		if (Who.empty())
		{
			break;
		}
		Ch.CarryTip -= 1.0;
		const Chips Amount = kast_room_detail::RoomTipCents[std::min(12, static_cast<int>(std::floor(growth::Unit(R.SeedK, R.StreamK, growth::Tag::Give, static_cast<uint64_t>(M) + (2ull << 40), K) * 13.0)))];
		Earned(S, Ch, Amount, 2);
		const bool FirstTip = (Ch.Firsts & growth::FirstStrangerTip) == 0;
		if (FirstTip)
		{
			Ch.Firsts |= growth::FirstStrangerTip; // the first stranger's tip (K17: posted live below)
		}
		const std::string Msg = kast_detail::Fill(S.R.Pick(kast_detail::Tips()), "{hero}", In.Hero);
		Alert A;
		A.Kind = AlertKind::Tip;
		A.Who = Who;
		A.Cents = Amount;
		A.Text = Msg;
		S.Queue(A);
		Line L;
		L.Who = Who;
		L.Text = Money(Amount) + ": " + Msg;
		L.Kind = LineKind::Tip;
		L.Cents = Amount;
		R.DueLines.push_back(L);
		if (FirstTip)
		{
			// C-K's M3: the first tip from someone new is a first-time milestone (§5.3 K17): its alert with KastShine as it lands, a
			// line after the tip's own, and the end card's mention (Summary::Firsts); the glue's end-card sting is gone with it.
			RoomState::PostFirst(S, Who, copy::FirstStrangerTip(Who, Amount));
		}
	}
	for (uint64_t K = 0; Aff && Ch.CarryCheer >= 1.0; ++K)
	{
		const std::string Who = Giver(K * 3 + 1);
		if (Who.empty())
		{
			break;
		}
		Ch.CarryCheer -= 1.0;
		const int Bits = kast_room_detail::RoomCheerBits[std::min(11, static_cast<int>(std::floor(growth::Unit(R.SeedK, R.StreamK, growth::Tag::Give, static_cast<uint64_t>(M) + (3ull << 40), K) * 12.0)))];
		Earned(S, Ch, Bits, 1);
		Alert A;
		A.Kind = AlertKind::Cheer;
		A.Who = Who;
		A.Count = Bits;
		A.Cents = Bits;
		A.Text = S.R.Pick(kast_detail::Cheers());
		S.Queue(A);
		Line L;
		L.Who = Who;
		L.Text = "Cheer" + std::to_string(Bits) + " " + A.Text;
		L.Kind = LineKind::Cheer;
		L.Cents = Bits;
		R.DueLines.push_back(L);
	}
	for (uint64_t K = 0; Aff && Ch.CarrySub >= 1.0; ++K)
	{
		const std::string Who = Giver(K * 3 + 2);
		if (Who.empty())
		{
			break;
		}
		Ch.CarrySub -= 1.0;
		if (!Ch.IsSub(Who, World))
		{
			S.Subscribe(Ch, In, Who, false, 1);
		}
	}
	// Ads: Kast runs one a world hour once the channel is Affiliate (at Smart today's 90 real seconds an hour).
	if (Aff && M - R.LastAdMinute >= 60)
	{
		R.LastAdMinute = M;
		const int Seconds = 90;
		S.AdUntil = In.Real + static_cast<double>(Seconds);
		S.AdSeconds = Seconds;
		S.LastAdWorld = World;
		R.AdUntilUp = T + kast_room_detail::RoomAdWindow;
		const double Slots = static_cast<double>(Seconds) / 30.0;
		Earned(S, Ch, static_cast<Chips>(std::round(static_cast<double>(R.Watchers) * Slots * (Ch.Partner ? 0.45 : 0.32))), 3);
		S.Health = std::max(0.0, S.Health - 0.03);
		S.Say(In, "", "Kast ran a scheduled ad break.", LineKind::System, 0, 0x94a3b8);
	}
	// The sponsors' live hours (C13): the sponsor book's deals on Kast's own copy, paid as the deal's own ledger lines.
	S.SponsorDebt += 1.0 / 60.0;
	if (S.SponsorDebt >= 0.1 - 1.0e-9)
	{
		kast_room_detail::RoomTakeDeals(S.Deals, In.Deals);
		for (Deal& D : S.Deals)
		{
			if (D.Until > World && D.PerHourCents > 0)
			{
				const Chips Cents2 = static_cast<Chips>(std::round(static_cast<double>(D.PerHourCents) * S.SponsorDebt));
				D.EarnedCents += Cents2;
				Earned(S, Ch, Cents2, 4);
				auto It = std::find_if(R.SponsorPaid.begin(), R.SponsorPaid.end(), [&](const Summary::SponsorPay& P) { return P.Id == D.Id; });
				if (It == R.SponsorPaid.end())
				{
					Summary::SponsorPay P;
					P.Id = D.Id;
					R.SponsorPaid.push_back(P);
					It = R.SponsorPaid.end() - 1;
				}
				It->HourCents += Cents2;
			}
		}
		S.SponsorDebt = 0.0;
	}
}

void RoomState::RunMinute(Stream& S, Channel& Ch, const Inputs& InRaw, int M)
{
	RoomState& R = *S.Room;
	const double T = static_cast<double>(M);
	const double World = S.StartWorld + T;
	// The room's clock is this minute's while it runs (a frame that spans minutes runs each one at its own time: R3).
	R.Clock = std::max(R.Clock, T);
	// ss.Kast.Moment's held run stands in for the inputs' deepest run.
	// The minute's inputs are the call's, at the minute's own world time (whatever the grid books carries that time: a sub's
	// date, a milestone's Heat), with ss.Kast.Moment's held run standing in for the deepest run.
	Inputs MinuteIn = InRaw;
	MinuteIn.World = World;
	if (R.Held)
	{
		MinuteIn.Prestige = R.HeldP;
		MinuteIn.DeepEntrants = R.HeldEntrants;
		MinuteIn.DeepRemaining = kast_room_detail::RoomHeldRemaining(R.HeldStage, R.HeldEntrants);
		MinuteIn.DeepTableSize = 9;
		MinuteIn.Depth = growth::Depth(R.HeldEntrants, MinuteIn.DeepRemaining);
		MinuteIn.DeepTable = 0;
	}
	const Inputs& In = MinuteIn;
	// The minute that ended fades (§2.9, R3): hype, Engage and the chat's health move on the world clock.
	if (M > 0)
	{
		S.Hype *= std::exp(-1.0 / kast_room_detail::RoomHypeTau);
		R.StageHype *= std::exp(-1.0 / kast_room_detail::RoomHypeTau);
		S.Engage = growth::EngageFade(S.Engage, 1.0);
		S.Health = std::min(1.0, S.Health + kast_room_detail::RoomHealthMinute);
	}
	S.Engage = std::max(S.Engage, kast_room_detail::RoomEngageFloor(In));
	const double E = std::min(1.0, S.Engage * R.NightStamina); // Stamina multiplies Engage (§2.8.5)
	// The deepest live run (§2.6): armed at 100+ entrants.
	R.RunEntrants = In.DeepEntrants;
	R.RunP = In.Prestige;
	R.RunDepth = growth::Armed(In.DeepEntrants) ? std::max(0.0, In.Depth) : 0.0;
	if (M % 10 == 0)
	{
		SlotAt(S, Ch, In, M);
	}
	if (M >= 60 && M % 60 == 0 && T <= kast_room_detail::RoomLateUntil)
	{
		LateAt(S, Ch, M);
	}
	// Follows that land this minute (guests and the crowd), and each raid's party once its welcome window has closed.
	for (size_t Gi = 0; Gi < R.Guests.size(); ++Gi)
	{
		if (R.Guests[Gi].FollowAt == M && !R.Guests[Gi].Gone)
		{
			FollowGuest(S, Ch, static_cast<int>(Gi), T, true);
		}
	}
	for (size_t Ci = 0; Ci < R.Crowds.size(); ++Ci)
	{
		Crowd& C = R.Crowds[Ci];
		if (C.FollowAt == M && !C.Gone)
		{
			const int Ix = R.FreeName(S, growth::Unit(R.SeedK, R.StreamK, growth::Tag::GuestName, static_cast<uint64_t>(M), 5000 + Ci), T);
			if (Ix >= 0)
			{
				C.Gone = true;
				R.PoolUntil[static_cast<size_t>(Ix)] = kast_room_detail::RoomInf;
				FollowName(S, Ch, S.Pool[static_cast<size_t>(Ix)], T, C.Source, C.Arrive <= T && T < C.Leave, C.Leave, false);
			}
		}
	}
	for (size_t Ri = 0; Ri < R.Raids.size(); ++Ri)
	{
		Raid& Rd = R.Raids[Ri];
		if (!Rd.Decided && M >= Rd.AtMinute + 2)
		{
			Rd.Decided = true;
			const double Chance = kast_room_detail::RoomRaidFollow * Rd.FollowBy * growth::Sat(Ch.Followers);
			for (size_t Xi = 0; Xi < R.Raiders.size(); ++Xi)
			{
				Raider& X = R.Raiders[Xi];
				if (X.RaidIx != static_cast<int>(Ri) || X.Gone || X.U >= Chance)
				{
					continue;
				}
				std::string Name = X.Name;
				if (Name.empty())
				{
					const int Ix = R.FreeName(S, growth::Unit(R.SeedK, R.StreamK, growth::Tag::Raider, static_cast<uint64_t>(Rd.AtMinute) * 16u + Ri % 16, 9000 + Xi), T);
					if (Ix < 0)
					{
						continue;
					}
					Name = S.Pool[static_cast<size_t>(Ix)];
					R.PoolUntil[static_cast<size_t>(Ix)] = kast_room_detail::RoomInf;
				}
				const bool Here = R.RaiderHere(X, T);
				X.Gone = true;
				++Rd.Followed;
				FollowName(S, Ch, Name, T, static_cast<uint8_t>(Rd.Broadcast ? View::Source::Broadcast : View::Source::Raid), Here, X.Leave, false);
			}
		}
		if (Rd.At15 < 0 && M >= Rd.AtMinute + 15)
		{
			int Still = 0;
			for (const Raider& X : R.Raiders)
			{
				Still += X.RaidIx == static_cast<int>(Ri) && R.RaiderHere(X, T) ? 1 : 0;
			}
			Rd.At15 = Still + Rd.Followed; // the ones who followed are still here too, as members
		}
	}
	// Dee's first-stream tip lands at world minute 8 with her in the room (First Faces made her arrival the sure one; an old
	// save's first stream still gets her here for it).
	if (Ch.Streams == 1 && Ch.EarnedTips == 0 && M >= 8)
	{
		const auto It = R.MemberAt.find("dee_spincycle");
		if (It != R.MemberAt.end())
		{
			int Vi = R.VisitOf[static_cast<size_t>(It->second)];
			if (Vi < 0)
			{
				Vi = AddVisit(S, Ch, It->second, T, static_cast<uint8_t>(View::Source::Members));
			}
			if (Vi >= 0 && !HereAt(R.Visits[static_cast<size_t>(Vi)], T) && M >= 30)
			{
				R.PullAt(R.Visits[static_cast<size_t>(Vi)], T, T);
			}
			if (Vi >= 0 && HereAt(R.Visits[static_cast<size_t>(Vi)], T))
			{
				const Chips First = 500;
				Earned(S, Ch, First, 2);
				Ch.Members[static_cast<size_t>(It->second)].Given += First;
				Alert A;
				A.Kind = AlertKind::Tip;
				A.Who = "dee_spincycle";
				A.Cents = First;
				A.Text = "first stream!! proud of you. don't punt";
				S.Queue(A);
				Line L;
				L.Who = A.Who;
				L.Text = Money(First) + ": " + A.Text;
				L.Kind = LineKind::Tip;
				L.Cents = First;
				L.Color = 0xf2a541;
				R.DueLines.push_back(L);
				S.Hype = std::min(100.0, S.Hype + 8.0);
			}
		}
	}
	// The room at the minute's start: what this minute's terms read (and the visits here, which the giving and the arrivals below
	// walk: nothing between here and them moves anyone).
	R.Recount(Ch, T);
	R.HereFresh = true;
	if (M == 20 && R.WelcomeBack && !R.WelcomeBackSaid)
	{
		R.WelcomeBackSaid = true;
		int In20 = 0;
		for (const Visit& V : R.Visits)
		{
			In20 += V.WelcomeBack && (HereAt(V, T) || WatchedTo(V, T) > 0.0) ? 1 : 0;
		}
		R.WelcomeBackIn = In20;
		if (In20 > 0)
		{
			S.Say(In, "", copy::WelcomeBack(In20), LineKind::System, 0, Lime);
		}
	}
	R.VM += static_cast<double>(R.Watchers);
	R.VMOwn += static_cast<double>(R.Watchers - R.RaidersHere);
	R.PresentSum += static_cast<double>(R.MembersHere);
	R.LastMinuteCount = R.Watchers;
	R.LastMinuteRaiders = R.RaidersHere;
	S.ViewerMinutes = R.VM;
	if (R.Watchers > S.Peak || R.PeakAt < 0)
	{
		R.PeakAt = M;
		R.PeakMoment = R.LastMomentKind >= 0 && T - R.LastMomentAt <= 10.0 ? R.LastMomentKind : -1;
	}
	S.Peak = std::max(S.Peak, R.Watchers);
	S.Tonight.Xp += 1.0 + 0.05 * std::sqrt(static_cast<double>(R.Watchers));
	S.Graph.push_back(static_cast<float>(R.Watchers));
	if (S.Graph.size() > 720)
	{
		std::vector<float> Half;
		for (size_t I = 0; I + 1 < S.Graph.size(); I += 2)
		{
			Half.push_back((S.Graph[I] + S.Graph[I + 1]) * 0.5f);
		}
		S.Graph = Half;
	}
	// "#23 of 49 in Poker" (§2.5.4), once a world minute.
	{
		const growth::Standing St = R.RoomRank(World, R.Watchers);
		R.RankNow = St.Rank;
		R.RankOf = St.Of;
		R.RankStart = M == 0 ? St.Rank : R.RankStart;
		R.RankBest = R.RankBest <= 0 ? St.Rank : std::min(R.RankBest, St.Rank);
	}
	GivingAt(S, Ch, In, M, E);
	ArrivalsAt(S, Ch, In, M, E);
	R.HereFresh = false;
	RaidsAt(S, Ch, In, M);
	TrollStepAt(S, Ch, In, M);
	// Questions arrive per world minute (§2.9; ChatStep puts them up from someone here).
	R.QuestionDebt += growth::QuestionRate(static_cast<double>(R.GuestsHere + R.CrowdHere), static_cast<double>(R.MembersHere), static_cast<double>(R.RaidersHere), Ch.Title == 2 || Ch.Title == 3);
	while (R.QuestionDebt >= 1.0)
	{
		R.QuestionDebt -= 1.0;
		R.QuestionsDue = std::min(2, R.QuestionsDue + 1);
	}
	// The ghost fill (§2.4): tonight's gain so far, perf on the running average against Start's forecast, news as stages hit.
	{
		const double Avg = R.VM / static_cast<double>(M + 1);
		const int Week = kast_detail::KastWeekOf(World);
		const double Used = Ch.GainWeek == Week ? Ch.GainUsed : 0.0;
		const growth::NightGain G = growth::Gain(std::log((Avg + 1.0) / (R.FcRaw + 1.0)), Ch.Bias, R.RunValue, Ch.RunNorm, Used, kast_room_detail::RoomShownHeat(Ch, World));
		R.GhostNow = G.Gain;
		Ch.GainSoFar = G.Gain;
	}
	// Kast's rules, the milestones and the stages (on the grid, so the channel changes the same way at every pace).
	S.CheckGrowth(Ch, In);
	play::OnMinute(*S.Play, R, Ch, In, M);
	// Who left for good: the room's lists keep only what can still matter.
	if (M % 10 == 9)
	{
		R.Guests.erase(std::remove_if(R.Guests.begin(), R.Guests.end(), [&](const Guest& G) { return (G.Gone || G.Leave < T - 1.0) && G.FollowAt <= M; }), R.Guests.end());
		R.Crowds.erase(std::remove_if(R.Crowds.begin(), R.Crowds.end(), [&](const Crowd& C) { return (C.Gone || C.Leave < T - 1.0) && C.FollowAt <= M; }), R.Crowds.end());
	}
}

void RoomState::Advance(Stream& S, Channel& Ch, const Inputs& In)
{
	if (!S.Live || !S.Room || !S.Room->RoomLive)
	{
		return;
	}
	RoomState& R = *S.Room;
	R.Bind(S, Ch, In);
	// The grid (R3): minute m runs when the clock reaches StartWorld + m, with this call's inputs, and nothing else moves the room.
	const double Up = std::max(R.Clock, In.World - S.StartWorld);
	while (static_cast<double>(R.Done) <= Up)
	{
		RunMinute(S, Ch, R.LastIn, R.Done);
		++R.Done;
	}
	R.Clock = Up;
	R.Recount(Ch, Up);
	// The games' arrivals (Rail Call's named returns): play::OnArrive as each comes in (after the scan: the games may act on the
	// room).
	std::vector<std::pair<std::string, View::Source>> Arrived;
	for (Visit& V : R.Visits)
	{
		if (V.OnArrive && HereAt(V, Up))
		{
			V.OnArrive = false;
			Arrived.emplace_back(Ch.Members[static_cast<size_t>(V.Member)].Name, static_cast<View::Source>(V.Source));
		}
		// The Run Alert's "back so far" (§2.3.5: a fact when shown): everyone an alert sent for who has been here since the first
		// alert, a frame seeing them or not (one who came and went inside a skip frame counts too), so the strip reads the same at
		// every pace and frame rate and the end card agrees with it (review, 2026-10-25: the strip counted only what frames saw).
		if (V.Pulled && !V.PulledIn && (HereAt(V, Up) || WatchedTo(V, Up) > V.PullWatch))
		{
			V.PulledIn = true;
			++R.AlertBack;
		}
	}
	for (const std::pair<std::string, View::Source>& A : Arrived)
	{
		play::OnArrive(*S.Play, R, Ch, A.first, A.second);
	}
	Flush(S, R.LastIn);
	// The old public mirrors read the room now: every surface draws the same integer.
	S.Viewers = static_cast<double>(R.Watchers);
	S.Present = R.MembersHere;
	S.RegularsHere = R.RegularsHere;
	S.Strangers = static_cast<double>(R.GuestsHere + R.CrowdHere);
	S.Lurkers = static_cast<double>(R.LurkersNow);
	S.RaidViewers = static_cast<double>(R.RaidersHere);
	S.OnlineMods.clear();
	for (const int Mi : R.ModsHere)
	{
		S.OnlineMods.push_back(Ch.Members[static_cast<size_t>(Mi)].Name);
	}
	R.Shown.Count = R.Watchers;
}

void RoomState::MomentStep(Stream& S, Channel& Ch, const Inputs& In, ss::kast::Moment M)
{
	// A moment on the stream (after Recount): the deep-run machinery at 100+ entrants (§2.6): Heat's Run, the roll call,
	// Bonded, the week's final tables, a Run Alert once a stage a world day, the Broadcast, a moment raid; the giving hazard's
	// bunching for whoever was here.
	RoomState& R = *S.Room;
	const double T = R.Clock;
	const int Mi = static_cast<int>(std::floor(T));
	const double World = S.StartWorld + T;
	const int Day = kast_detail::KastDayOf(World);
	const uint64_t Table = kast_room_detail::RoomTable(In);
	R.LastMomentKind = static_cast<int>(M);
	R.LastMomentAt = T;
	if (kast_room_detail::RoomBigMoment(M))
	{
		R.BigAtRoom = T;
		for (Visit& V : R.Visits)
		{
			if (HereAt(V, T))
			{
				V.BigAt = T;
			}
		}
		if (R.GiveLazy)
		{
			R.BigMoments.push_back({T, ++R.GiveSeq}); // lazy giving's replay reads each visit's BigAt from these
		}
	}
	const growth::RunStage Stage = kast_room_detail::RoomStageOf(M);
	if (Stage == growth::RunStage::None)
	{
		return;
	}
	// The moment's own run: the deepest one's facts when it is that table (or no table is named), else this table's field.
	const bool IsDeep = In.DeepTable < 0 || In.ThisTable < 0 || In.ThisTable == In.DeepTable;
	const int Entrants = IsDeep && In.DeepEntrants > 0 ? In.DeepEntrants : In.Entrants;
	R.StageHype += kast_room_detail::RoomStageHits[static_cast<int>(Stage)] * (growth::Armed(Entrants) ? 1.0 : 0.5); // the room's hype
	if (!growth::Armed(Entrants))
	{
		return; // a small field's stages are poker moments, not deep runs
	}
	const double Pm = IsDeep && In.Prestige > 0.0 ? In.Prestige : growth::Prestige(Entrants, In.BuyInCents > 0 ? In.BuyInCents : 330, In.Series);
	R.RunValue = std::max(R.RunValue, growth::RunValue(Stage) * Pm);
	if (static_cast<int>(Stage) > R.BestStageKind)
	{
		R.BestStageKind = static_cast<int>(Stage);
		R.BestStageEntrants = Entrants;
	}
	if (IsDeep)
	{
		++R.StageMoments;
		for (Visit& V : R.Visits)
		{
			V.WholeRun = V.WholeRun && HereAt(V, T);
		}
	}
	if (Stage == growth::RunStage::FinalTable || Stage == growth::RunStage::Win)
	{
		kast_room_detail::RoomFtMark(Ch, Day);
		// Bonded (§2.8.2): the first time a member is here for a final table or a win on stream, Affinity +5% of its gap, once.
		for (const Visit& V : R.Visits)
		{
			Member& Mb = Ch.Members[static_cast<size_t>(V.Member)];
			if (HereAt(V, T) && !Mb.Bonded)
			{
				Mb.Bonded = true;
				Mb.Affinity = growth::AffinityBump(Mb.Affinity, 0.05);
			}
		}
	}
	// A Run Alert (§2.3.5), once a stage a world day: attendees who are away come back; at the final table and top 3, followers
	// not here tonight come too.
	const uint32_t Bit = kast_room_detail::RoomAlertBit(Stage);
	if (Bit != 0 && !growth::Granted(Ch.GrantDay, Ch.GrantMask, Day, Bit))
	{
		growth::UseGrant(Ch.GrantDay, Ch.GrantMask, Day, Bit);
		const double Pull = growth::Pull(Stage);
		const uint64_t K = static_cast<uint64_t>(Stage) * 64u + Table;
		int Sent = 0;
		for (Visit& V : R.Visits)
		{
			if (V.Gone || HereAt(V, T))
			{
				continue;
			}
			++Sent;
			const Member& Mb = Ch.Members[static_cast<size_t>(V.Member)];
			if (growth::Unit(R.SeedK, R.StreamK, growth::Tag::Pull, V.Key, K) < growth::AlertBack(Pull, Mb.Loyalty))
			{
				R.PullAt(V, T, T + growth::ExpAt(4.0, growth::Unit(R.SeedK, R.StreamK, growth::Tag::Pull, V.Key, K + (1ull << 20))));
				V.PullWatch = V.Pulled ? V.PullWatch : WatchedTo(V, T);
				V.Pulled = true;
				V.PullStage = static_cast<int>(Stage);
			}
		}
		if (Stage == growth::RunStage::FinalTable || Stage == growth::RunStage::Top3)
		{
			const size_t N = Ch.Members.size();
			for (size_t I = 0; I < N; ++I)
			{
				const Member& Mb = Ch.Members[I];
				if (R.VisitOf[I] >= 0 || !Mb.Follower || Mb.Friend)
				{
					continue;
				}
				++Sent;
				const double W = growth::PingW(std::max(0.0, (World - Mb.LastSeen) / kast_detail::KastDay), Mb.Loyalty);
				if (growth::Unit(R.SeedK, R.StreamK, growth::Tag::Pull, R.MemberKey[I], K) < growth::AlertFollower(Pull, W, Mb.Loyalty, Pm))
				{
					const int Vi = AddVisit(S, Ch, static_cast<int>(I), T + growth::ExpAt(6.0, growth::Unit(R.SeedK, R.StreamK, growth::Tag::Pull, R.MemberKey[I], K + (1ull << 20))),
						static_cast<uint8_t>(View::Source::Members));
					if (Vi >= 0)
					{
						R.Visits[static_cast<size_t>(Vi)].Pulled = true;
						R.Visits[static_cast<size_t>(Vi)].PullStage = static_cast<int>(Stage);
					}
				}
			}
		}
		if (Sent > 0)
		{
			R.AlertSent += Sent;
			S.Say(In, "", copy::RunAlertSent(Sent), LineKind::System, 0, Lime);
			Notice N;
			N.Type = Notice::Kind::Milestone;
			N.Sound = static_cast<int>(SoundId::KastAlert);
			S.Notices.push_back(N);
		}
	}
	if (Stage == growth::RunStage::FinalTable)
	{
		// The final-table sting (§5.3 K5): KastAlert through the cue queue (a Run Alert at the same moment folds into it).
		Notice N;
		N.Type = Notice::Kind::Milestone;
		N.Sound = static_cast<int>(SoundId::KastAlert);
		S.Notices.push_back(N);
		// The site's own Broadcast (§2.6): a final table with P >= 1, once a world day.
		if (Pm >= 1.0 && !growth::Granted(Ch.GrantDay, Ch.GrantMask, Day, growth::GrantBroadcast))
		{
			growth::UseGrant(Ch.GrantDay, Ch.GrantMask, Day, growth::GrantBroadcast);
			AddRaid(S, Ch, In, kast_room_copy::RoomCopyBroadcastFrom(In.DeepSite), Violet, std::max(1, growth::BroadcastSize(Pm)), true, T, 4);
		}
	}
	// A moment raid (§2.7.1): ITM 5%, final table 15%, a win 30%, at most one a world day; at a final table or a win a big name
	// raids by Heat, once a stream.
	const double Mr = growth::MomentRaidChance(Stage);
	const uint64_t Rk = static_cast<uint64_t>(M) * 8u + Table;
	if (Mr > 0.0 && !growth::Granted(Ch.GrantDay, Ch.GrantMask, Day, growth::GrantMomentRaid) &&
		growth::Unit(R.SeedK, R.StreamK, growth::Tag::MomentRaid, static_cast<uint64_t>(Mi), Rk) < Mr)
	{
		growth::UseGrant(Ch.GrantDay, Ch.GrantMask, Day, growth::GrantMomentRaid);
		std::vector<const Streamer*> Big;
		for (const Streamer& D : Directory())
		{
			if (!D.Rival && ViewersNow(D, World) > 0)
			{
				Big.push_back(&D);
			}
		}
		const double Hn = growth::HeatH(kast_room_detail::RoomShownHeat(Ch, World));
		const bool BigName = (Stage == growth::RunStage::FinalTable || Stage == growth::RunStage::Win) && !R.BigNamed && !Big.empty() &&
			growth::Unit(R.SeedK, R.StreamK, growth::Tag::MomentRaid, static_cast<uint64_t>(Mi), Rk + 4096) < growth::BigNameChance(Hn, Pm);
		if (BigName)
		{
			R.BigNamed = true;
			const Streamer& D = *Big[std::min(Big.size() - 1, static_cast<size_t>(std::floor(growth::Unit(R.SeedK, R.StreamK, growth::Tag::MomentRaid, static_cast<uint64_t>(Mi), Rk + 8192) * static_cast<double>(Big.size()))))];
			const double Share = 0.03 + 0.05 * growth::Unit(R.SeedK, R.StreamK, growth::Tag::MomentRaid, static_cast<uint64_t>(Mi), Rk + 12288);
			AddRaid(S, Ch, In, D.Name, D.Color, std::max(5, static_cast<int>(std::lround(static_cast<double>(ViewersNow(D, World)) * Share))), false, T, 3);
		}
		else
		{
			const std::vector<SmallChannel> Net = Network(World);
			if (!Net.empty())
			{
				const SmallChannel& C = Net[std::min(Net.size() - 1, static_cast<size_t>(std::floor(growth::Unit(R.SeedK, R.StreamK, growth::Tag::MomentRaid, static_cast<uint64_t>(Mi), Rk + 16384) * static_cast<double>(Net.size()))))];
				const int Size = std::max(1, static_cast<int>(std::lround(growth::RaidSize(growth::Unit(R.SeedK, R.StreamK, growth::Tag::MomentRaid, static_cast<uint64_t>(Mi), Rk + 20480),
					growth::Unit(R.SeedK, R.StreamK, growth::Tag::MomentRaid, static_cast<uint64_t>(Mi), Rk + 24576)))));
				AddRaid(S, Ch, In, C.Name, C.Color, Size, false, T, 1);
			}
		}
	}
}

void RoomState::Display(Stream& S, Channel& Ch, const Inputs& In, double DReal)
{
	// Display chat (R3): the people present talk, through the stream's own R; nothing here changes the channel or the room.
	if (!S.Live || !S.Room || !S.Room->RoomLive)
	{
		return;
	}
	RoomState& R = *S.Room;
	R.Bind(S, Ch, In);
	const double T = R.Clock;
	// R1: a line set to land a moment from now whose speaker has left in the meantime never lands.
	S.Chat.erase(std::remove_if(S.Chat.begin(), S.Chat.end(),
					 [&](const ChatMsg& M) {
						 return M.At > R.LastPurgeReal && !M.Who.empty() && M.Kind != LineKind::Streamer && M.Kind != LineKind::System && M.Kind != LineKind::Raid && !R.Present(M.Who);
					 }),
		S.Chat.end());
	R.LastPurgeReal = In.Real;
	// Who walked in: the familiar ones say hi; Dee and Mei on the first night; the Run Alert's returners say so. "Here at the last
	// step" is a flag a member (by MemberCanon) and the named guests by name (the frame budget, §9.4 #13: no set of every name).
	R.ShownMembersNow.assign(Ch.Members.size(), 0);
	// The named guests shown at the last step, by their pool index while the pool is indexed (C-K's M2: no set of names a frame).
	const bool PoolIndexed = R.PoolAt.size() == S.Pool.size() && R.ShownPool.size() == S.Pool.size();
	auto WasGuest = [&](const std::string& Name) {
		if (!PoolIndexed)
		{
			return R.ShownGuests.count(Name) != 0;
		}
		const auto It = R.PoolAt.find(Name);
		return It != R.PoolAt.end() && R.ShownPool[static_cast<size_t>(It->second)] != 0;
	};
	auto SayHello = [&](size_t Vi, int MemberIx) {
		const size_t Canon = static_cast<size_t>(R.CanonOf(MemberIx));
		R.ShownMembersNow[Canon] = 1;
		if (Canon < R.ShownMembers.size() && R.ShownMembers[Canon] != 0)
		{
			return; // here at the last step too (read without touching the visit or the member: the frame budget)
		}
		const Visit& V = R.Visits[Vi];
		const Member& M = Ch.Members[static_cast<size_t>(V.Member)];
		if (WasGuest(M.Name))
		{
			return;
		}
		std::string Hello;
		if (M.Name == "dee_spincycle" && R.StreamK == 1 && !R.DeeHiSaid)
		{
			R.DeeHiSaid = true;
			Hello = copy::DeeHi();
		}
		else if (M.Name == "mei_ng" && R.StreamK == 1 && !R.MeiHiSaid)
		{
			R.MeiHiSaid = true;
			Hello = copy::MeiHi();
		}
		else if (V.Pulled && !copy::RunAlertReturn(static_cast<growth::RunStage>(V.PullStage)).empty())
		{
			Hello = S.R.Pick(copy::RunAlertReturn(static_cast<growth::RunStage>(V.PullStage)));
		}
		else if (S.R.Chance(0.25 + 0.5 * M.Loyalty))
		{
			static const std::vector<std::string> Back = {"hey {hero}", "evening", "back again", "made it", "o7", "hi chat", "what did I miss", "gl tonight", "kastLove"};
			Hello = kast_detail::Fill(S.R.Pick(Back), "{hero}", In.Hero);
		}
		if (!Hello.empty())
		{
			S.Say(In, M.Name, Hello, LineKind::Chat, kast_room_detail::RoomBadges(R, Ch, M.Name, In.World), M.Name == "dee_spincycle" ? 0xf2a541u : kast_detail::NameColor(M.Name));
			S.Chat.back().At = In.Real + S.R.Range(0.2, 2.0);
			S.Seen.insert(M.Name);
			R.Spoke.push_back({M.Name, S.Chat.back().At});
		}
		if (M.Name == "dee_spincycle" && In.LiveNightRecent && !R.DeeEmbercrestSaid)
		{
			R.DeeEmbercrestSaid = true;
			S.Say(In, M.Name, copy::DeeEmbercrest(), LineKind::Chat, kast_room_detail::RoomBadges(R, Ch, M.Name, In.World), 0xf2a541);
			S.Chat.back().At = In.Real + 2.5;
		}
	};
	if (R.PresOn)
	{
		// The visits here, in visit order, from presence by transition (C-K's M2).
		R.PresUpdate(Ch, T);
		for (size_t W = 0; W < R.PresBits.size(); ++W)
		{
			for (uint64_t B = R.PresBits[W]; B != 0; B &= B - 1)
			{
				const size_t Vi = W * 64 + kast_room_detail::RoomLowBit(B);
				SayHello(Vi, R.PresMember[Vi]);
			}
		}
	}
	else
	{
		for (size_t Vi = 0; Vi < R.Visits.size(); ++Vi)
		{
			if (HereAt(R.Visits[Vi], T))
			{
				SayHello(Vi, R.Visits[Vi].Member);
			}
		}
	}
	R.ShownMembers.swap(R.ShownMembersNow);
	if (R.PoolAt.size() == S.Pool.size())
	{
		if (R.ShownPool.size() != S.Pool.size())
		{
			R.ShownPool.assign(S.Pool.size(), 0);
			R.ShownPoolList.clear();
		}
		for (const int Ix : R.ShownPoolList)
		{
			R.ShownPool[static_cast<size_t>(Ix)] = 0;
		}
		R.ShownPoolList.clear();
		for (const Guest& G : R.Guests)
		{
			if (G.Kind == 0 && G.Pool >= 0 && R.GuestHere(G, T))
			{
				R.ShownPool[static_cast<size_t>(G.Pool)] = 1;
				R.ShownPoolList.push_back(G.Pool);
			}
		}
	}
	else
	{
		R.ShownGuests.clear();
		for (const Guest& G : R.Guests)
		{
			if (G.Kind == 0 && G.Pool >= 0 && R.GuestHere(G, T))
			{
				R.ShownGuests.insert(S.Pool[static_cast<size_t>(G.Pool)]);
			}
		}
	}
	// "2 moderators are watching chat" when the second one is here (F3).
	if (static_cast<int>(R.ModsHere.size()) > R.ModsSaid)
	{
		R.ModsSaid = static_cast<int>(R.ModsHere.size());
		S.Say(In, "", copy::ModsWatching(R.ModsSaid), LineKind::System, 0, 0x22c55e);
	}
	// Chat: the room talks at its size, people present only (lurkers and the crowd never speak).
	double Talk = 0.0;
	if (R.PresOn)
	{
		for (size_t W = 0; W < R.PresBits.size(); ++W)
		{
			for (uint64_t B = R.PresBits[W]; B != 0; B &= B - 1)
			{
				Talk += R.PresTalk[W * 64 + kast_room_detail::RoomLowBit(B)]; // 0.2 + L, read with the visit's presence
			}
		}
	}
	else
	{
		for (const Visit& V : R.Visits)
		{
			if (HereAt(V, T))
			{
				Talk += 0.2 + Ch.Members[static_cast<size_t>(V.Member)].Loyalty;
			}
		}
	}
	for (const Guest& G : R.Guests)
	{
		Talk += G.Kind == 0 && R.GuestHere(G, T) ? 0.1 : 0.0;
	}
	for (const Raider& X : R.Raiders)
	{
		Talk += !X.Name.empty() && R.RaiderHere(X, T) ? 0.15 : 0.0;
	}
	const double ChatRate = std::min(10.0, Talk / 60.0 * (1.0 + S.Hype / 40.0) * (0.8 + 0.4 * S.Engage)) * (S.AdRunning(In.Real) ? 0.6 : 1.0);
	S.ChatDebt = std::min(6.0, S.ChatDebt + ChatRate * DReal);
	while (R.Watchers >= 1 && S.ChatDebt >= 1.0)
	{
		S.ChatDebt -= 1.0;
		int Badges = 0;
		uint32_t Color = 0;
		const std::string Who = S.Chatter(Ch, In, Badges, Color);
		if (Who.empty())
		{
			break;
		}
		LineKind Kind = LineKind::Chat;
		std::string Text = S.LineFor(Ch, In, Kind);
		Text = kast_detail::Fill(Text, "{hero}", In.Hero);
		Text = kast_detail::Fill(Text, "{tables}", std::to_string(std::max(1, In.Tables)));
		Text = kast_detail::Fill(Text, "{bb}", std::to_string(static_cast<int>(std::round(In.StackBb))));
		Text = kast_detail::Fill(Text, "{country}", kast_detail::CountryName(handles::PickCountry(S.R)));
		Text = kast_detail::Fill(Text, "{event}", In.EventName);
		Text = kast_detail::Fill(Text, "{tilt}", Ch.Affiliate && (Badges & BadgeSub) ? S.Prefix + "Tilt" : "kastSalt");
		Text = kast_detail::Fill(Text, "{ship}", Ch.Affiliate && (Badges & BadgeSub) ? S.Prefix + "Ship" : "kastHype");
		Text = kast_detail::Fill(Text, "{rent}", Ch.Affiliate && (Badges & BadgeSub) ? S.Prefix + "Rent" : "kastChip");
		Text = kast_detail::Fill(Text, "{pct}", std::to_string(70 + S.R.Int(20)));
		Text = kast_detail::Fill(Text, "{who}", "him");
		Text = kast_detail::Fill(Text, "{prize}", "$$$");
		if (In.Rank > 0 && Text == "rank?")
		{
			Text = "rank? " + std::to_string(In.Rank) + "/" + std::to_string(In.Remaining) + " kastPog";
		}
		S.Say(In, Who, Text, Kind, Badges, Color);
	}
	// Questions, mostly from people still deciding whether to stay (the grid's count; the asker is someone here).
	while (R.QuestionsDue > 0 && R.Watchers >= 1)
	{
		int Badges = 0;
		uint32_t Color = 0;
		const std::string Who = S.Chatter(Ch, In, Badges, Color);
		if (Who.empty())
		{
			break;
		}
		--R.QuestionsDue;
		S.Say(In, Who, S.R.Pick(kast_detail::Questions()), LineKind::Question, Badges, Color);
	}
	// An empty room is honest and still alive (R6): Kast's own lines carry the model's numbers, one every 3 real minutes.
	if (R.Watchers == 0 && In.Real - R.EmptyLineAt >= kast_room_detail::RoomEmptyLineGap && R.Done > 1)
	{
		double Seen = 0.0;
		int Came = 0;
		for (size_t I = 0; I < 30 && static_cast<int>(I) < R.Done; ++I)
		{
			Seen += R.SeenRing[I];
			Came += R.CameRing[I];
		}
		for (int K = 0; K < 4; ++K)
		{
			const int Which = R.EmptyLineIx++;
			const std::string Text = kast_room_copy::RoomCopyEmpty(Which, R.RankNow, R.RankOf, static_cast<int>(std::lround(Seen)), Came, R.NightLane > 1.0, R.NightRising > 1.0);
			if (!Text.empty() && (Which % 4 != 0 || R.RankNow > 0))
			{
				S.Say(In, "", Text, LineKind::System, 0, Violet);
				R.EmptyLineAt = In.Real;
				break;
			}
		}
	}
	// The chat games' display time (Hold or Crack's calls, their replies and toasts).
	play::OnChat(*S.Play, R, In, DReal, S.R);
	// Trolls' lines come down as the catches the grid decided come due (display: the catch itself was booked).
	for (ChatMsg& M : S.Chat)
	{
		if (M.Deleted || (!M.Toxic && !M.Spam) || M.At > In.Real || M.DeleteAt < 0.0 || In.Real < M.DeleteAt)
		{
			continue;
		}
		M.Deleted = true;
		S.TimedOut[M.Who] = In.Real + 600.0;
		const std::string By = M.DeletedBy;
		const std::string Who = M.Who;
		S.Say(In, "", Who + " was timed out by " + By + ".", LineKind::System, 0, By == "WardenBot" ? 0x7c3aed : 0x22c55e);
	}
	// Predictions take points for two minutes (chat's points, never money).
	if (S.Pred.Active && !S.Pred.Resolved && In.Real - S.Pred.At < 120.0)
	{
		S.PredDebt += static_cast<double>(R.Watchers) * DReal * 0.05;
		while (S.PredDebt >= 1.0)
		{
			S.PredDebt -= 1.0;
			const int Points = (1 + S.R.Int(10)) * 100;
			(S.R.Chance(0.45 + S.Hype / 400.0) ? S.Pred.Yes : S.Pred.No) += Points;
		}
	}
	R.Spoke.erase(std::remove_if(R.Spoke.begin(), R.Spoke.end(), [&](const Spoken& P) { return P.At < In.Real - kast_room_detail::RoomChattingWindow; }), R.Spoke.end());
}

std::array<std::string, 2> RoomState::WhyNow(const Channel& Ch, double World) const
{
	// Heat's top two reasons (§2.4), each a fact the room or the channel holds, in this order: a deep run (tonight's, or one in the
	// last 6 days), the bumps of the last 2 world days (a record peak, a raid, Rerun's push, a top-of-feed clip, a follower
	// milestone), the last night against its forecast (10%+ over or under, in 2 days), showing up, the week's cap; a new channel, or
	// a steady one (the last night near its forecast, in 2 days) when there's nothing else.
	std::array<std::string, 2> Out;
	size_t N = 0;
	auto Add = [&](const std::string& Line) {
		if (N < Out.size() && !Line.empty())
		{
			Out[N++] = Line;
		}
	};
	const int Today = kast_detail::KastDayOf(World);
	auto Recent = [Today](int Day) { return Day >= 0 && Today - Day >= 0 && Today - Day <= 2; };
	if (BestStageKind >= static_cast<int>(growth::RunStage::FinalTable))
	{
		Add(copy::WhyRun(static_cast<growth::RunStage>(BestStageKind), BestStageEntrants, 0, std::string()));
	}
	else if (WhyStage >= static_cast<int>(growth::RunStage::FinalTable) && Today - WhyStageDay >= 0 && Today - WhyStageDay <= 6)
	{
		Add(copy::WhyRun(static_cast<growth::RunStage>(WhyStage), WhyStageRunners, Today - WhyStageDay, net::WeekdayName(WhyStageDay, true)));
	}
	if (WhyRecordPeak > 0 && Recent(WhyRecordDay))
	{
		Add(copy::WhyRecordPeak(WhyRecordPeak));
	}
	if (WhyRaidSize > 0 && !WhyRaidFrom.empty() && Recent(WhyRaidDay))
	{
		Add(copy::WhyRaid(WhyRaidFrom, WhyRaidSize));
	}
	if (!WhyVideoTitle.empty() && Recent(WhyVideoDay))
	{
		Add(copy::WhyVideo(WhyVideoTitle, WhyVideoBreakout));
	}
	if (WhyClipDay >= 0 && Today - WhyClipDay <= 2)
	{
		Add(copy::WhyClipTop());
	}
	if (WhyMilestone > 0 && Recent(WhyMilestoneDay))
	{
		Add(copy::WhyMilestone(WhyMilestone));
	}
	if (WhyOverPct >= 10 && WhyOverDay >= 0 && Today - WhyOverDay <= 2)
	{
		Add(copy::WhyOverForecast(WhyOverPct));
	}
	else if (WhyUnderPct >= 10 && WhyOverDay >= 0 && Today - WhyOverDay <= 2)
	{
		Add(copy::WhyUnderForecast(WhyUnderPct));
	}
	const int Week = kast_detail::KastWeekOf(World);
	int Shows = 0;
	for (int B = 0; B < 7; ++B)
	{
		Shows += Ch.ShowWeek == Week && ((Ch.ShowDays >> B) & 1) ? 1 : 0;
	}
	if (Shows >= 2)
	{
		Add(copy::WhyShowingUp(Shows));
	}
	if (Ch.GainWeek == Week && Ch.GainUsed >= 35.0 - 1.0e-9)
	{
		Add(copy::WhyHotWeek());
	}
	if (N == 0)
	{
		// "Steady: nights near your forecast" only when the room knows it: the last night read within 10% of its forecast, in 2
		// days. A reload (the facts live in this session's room) or a few days dark leaves no such fact, and then the line stays
		// empty rather than guess (the C-K re-check).
		if (StreamK <= 3)
		{
			Add(copy::WhyNew());
		}
		else if (WhyOverDay >= 0 && Today - WhyOverDay >= 0 && Today - WhyOverDay <= 2)
		{
			Add(copy::WhySteady());
		}
	}
	return Out;
}

void RoomState::FillView(Stream& S, Channel& Ch, const Inputs& In)
{
	// The View (§10.7): everything the UI reads of the live stream, once a Tick.
	RoomState& R = *S.Room;
	R.Bind(S, Ch, In);
	const double World = S.StartWorld + R.Clock;
	View V;
	V.Count = R.Watchers;
	// One count-up clock for every surface (§1.4): a 0.3 s count from max(previous, the speakers on screen) to the new count.
	{
		const double Was = R.CountFrom + (R.CountTo - R.CountFrom) * std::min(1.0, std::max(0.0, (In.Real - R.CountAtReal) / kast_room_detail::RoomCountUp));
		if (static_cast<double>(R.Watchers) != R.CountTo)
		{
			// The distinct speakers among the last 12 lines who are here (at most 12: a list, not a set of strings a frame).
			std::array<const std::string*, 12> OnScreen{};
			size_t Distinct = 0;
			for (size_t I = S.Chat.size() > 12 ? S.Chat.size() - 12 : 0; I < S.Chat.size(); ++I)
			{
				const std::string& Who = S.Chat[I].Who;
				if (!Who.empty() && S.Chat[I].At <= In.Real && std::none_of(OnScreen.begin(), OnScreen.begin() + static_cast<std::ptrdiff_t>(Distinct), [&](const std::string* P) { return *P == Who; }) &&
					R.Present(Who))
				{
					OnScreen[Distinct++] = &Who;
				}
			}
			R.CountFrom = static_cast<double>(R.Watchers) > R.CountTo ? std::max(Was, static_cast<double>(Distinct)) : Was;
			R.CountTo = static_cast<double>(R.Watchers);
			R.CountAtReal = In.Real;
		}
		const double Shown = R.CountFrom + (R.CountTo - R.CountFrom) * std::min(1.0, std::max(0.0, (In.Real - R.CountAtReal) / kast_room_detail::RoomCountUp));
		V.ShownCount = std::max(0, static_cast<int>(std::lround(Shown)));
	}
	{
		// "N chatting": the speakers of the last 2 real minutes who are still here, one count a person: members by MemberAt's index
		// (a stamp a fill), anyone else by name, each looked up once a fill (the frame budget, §9.4 #13: a busy chat holds 800+
		// lines in its two real minutes). A member's presence is Present's own (the visit, then the guest and raider halves).
		int Chatting = 0;
		if (R.ViewCh && R.RoomLive)
		{
			++R.SpokeFill;
			if (R.SpokeStamp.size() < Ch.Members.size())
			{
				R.SpokeStamp.resize(Ch.Members.size(), 0);
			}
			// The table is kept between fills (no table a frame) but always left empty: its keys view this fill's Spoke names, which a
			// later purge moves or frees, so none may outlive the walk (an emptied table's clear() rehashes nothing).
			std::unordered_set<std::string_view>& Others = R.SpokeOthers;
			Others.clear();
			for (Spoken& P : R.Spoke)
			{
				if (P.Who.empty() || !(P.At <= In.Real && P.At >= In.Real - kast_room_detail::RoomChattingWindow))
				{
					continue;
				}
				if (P.Member < 0)
				{
					const auto It = R.MemberAt.find(P.Who);
					P.Member = It != R.MemberAt.end() ? It->second : -1;
				}
				if (P.Member >= 0 && static_cast<size_t>(P.Member) < R.SpokeStamp.size())
				{
					int& Stamp = R.SpokeStamp[static_cast<size_t>(P.Member)];
					if (Stamp == R.SpokeFill)
					{
						continue;
					}
					Stamp = R.SpokeFill;
					const size_t Mi = static_cast<size_t>(P.Member);
					const int Vi = Mi < R.VisitOf.size() ? R.VisitOf[Mi] : -1;
					const bool Here = (Vi >= 0 && HereAt(R.Visits[static_cast<size_t>(Vi)], R.Clock)) || R.GuestNamed(P.Who) >= 0 || R.RaiderNamed(P.Who) >= 0;
					Chatting += Here ? 1 : 0;
				}
				else if (Others.insert(std::string_view(P.Who)).second)
				{
					Chatting += R.Present(P.Who) ? 1 : 0;
				}
			}
			Others.clear(); // while every key it holds still views a live name
		}
		V.Chatting = std::min(R.Watchers, Chatting);
	}
	V.Sources = R.SourcesNow;
	V.Heat = kast_room_detail::RoomShownHeat(Ch, World);
	V.GhostGain = R.GhostNow;
	V.Tier = growth::Tier(V.Heat);
	{
		// Heat's top two reasons, each a fact (§2.4), built again only when one of their facts moved (C-K's M2: no strings a frame).
		const int Today = kast_detail::KastDayOf(World);
		const int Week = kast_detail::KastWeekOf(World);
		uint64_t GainBits = 0;
		std::memcpy(&GainBits, &Ch.GainUsed, sizeof(GainBits));
		const std::array<uint64_t, 10> Key = {R.WhyEpoch, static_cast<uint64_t>(static_cast<int64_t>(Today)), static_cast<uint64_t>(static_cast<int64_t>(Week)),
			static_cast<uint64_t>(static_cast<int64_t>(Ch.ShowWeek)), static_cast<uint64_t>(static_cast<int64_t>(Ch.ShowDays)), static_cast<uint64_t>(static_cast<int64_t>(Ch.GainWeek)),
			GainBits, static_cast<uint64_t>(static_cast<int64_t>(R.BestStageKind)), static_cast<uint64_t>(static_cast<int64_t>(R.BestStageEntrants)), R.StreamK};
		if (!R.WhyHeld || Key != R.WhyKey)
		{
			R.WhyLines = R.WhyNow(Ch, World);
			R.WhyKey = Key;
			R.WhyHeld = true;
		}
		V.Why.swap(R.Shown.Why); // the last fill's buffers
		V.Why = R.WhyLines;
	}
	const bool Alive = R.RunDepth > 0.0;
	V.EventMode = Alive && growth::EventMode(In.DeepEntrants, In.DeepRemaining, In.DeepTableSize);
	V.DeepRemaining = In.DeepRemaining;
	V.DeepEntrants = In.DeepEntrants;
	V.Prestige = In.Prestige;
	V.DeepEvent = In.DeepEvent;
	if (R.Held)
	{
		V.EventMode = growth::EventMode(R.HeldEntrants, kast_room_detail::RoomHeldRemaining(R.HeldStage, R.HeldEntrants), 9);
		V.DeepRemaining = kast_room_detail::RoomHeldRemaining(R.HeldStage, R.HeldEntrants);
		V.DeepEntrants = R.HeldEntrants;
		V.Prestige = R.HeldP;
	}
	V.RunAlertSent = R.AlertSent;
	V.RunAlertBack = R.AlertBack;
	V.Rank = R.RankNow;
	V.RankOf = R.RankOf;
	V.RankStart = R.RankStart;
	V.Fc = R.Fc;
	V.Stamina = R.NightStamina;
	{
		int K = 0;
		for (const double At : R.ThankTimes)
		{
			K += R.Clock - At <= 10.0 ? 1 : 0;
		}
		V.Thanks10 = K;
	}
	V.HeadsDown = !In.Attended && S.Engage < 0.15;
	// Only a ping that went out is a fact (a restart inside 4 world hours pinged nobody: no "Your ping reached" toast).
	V.PingReached = R.PingSent ? R.PingReached : 0;
	V.PingOf = R.PingSent ? R.PingOf : 0;
	// Per member: the mods and tonight's Nods matched by MemberAt's index, each name looked up once a fill rather than once a member
	// (the frame budget, §9.4 #13; a name has one index, so the match is the name's: Channel::IsMod, play::WasNodded). Everything
	// but the present bit moves only with a loyalty, a member, a visit, a mod or a Nod: it is kept (FlagsBase) and rebuilt when one
	// of those changed; the present bit comes from presence by transition (C-K's M2). The buffer is the last fill's.
	const size_t MemberCount = Ch.Members.size();
	uint64_t ModsKey = Ch.Mods.size();
	for (const Moderator& Md : Ch.Mods)
	{
		ModsKey = ModsKey * 1099511628211ull + growth::NameKey(Md.Name);
	}
	const std::array<uint64_t, 7> FlagsNow = {R.LoyaltyWrites, MemberCount, R.Visits.size(), R.ModVisits.size(), S.Play->Nodded.size(), R.MemberAt.size(), ModsKey};
	if (!R.PresOn || FlagsNow != R.FlagsKey || R.FlagsBase.size() != MemberCount)
	{
		std::vector<int> ModAt, NodAt;
		for (const Moderator& Md : Ch.Mods)
		{
			const auto It = R.MemberAt.find(Md.Name);
			if (It != R.MemberAt.end())
			{
				ModAt.push_back(It->second);
			}
		}
		for (const std::string& Name : S.Play->Nodded)
		{
			const auto It = R.MemberAt.find(Name);
			if (It != R.MemberAt.end())
			{
				NodAt.push_back(It->second);
			}
		}
		auto Among = [](const std::vector<int>& At, int Ix) { return std::find(At.begin(), At.end(), Ix) != At.end(); };
		std::vector<uint8_t>& Base = R.FlagsBase;
		Base.assign(MemberCount, 0);
		// Three straight walks: the regular bit over the members; each visit's bits (a visit is its member's one visit tonight:
		// VisitOf points back at it); a mod not coming tonight, by the mods' indices while every name has one.
		for (size_t I = 0; I < MemberCount; ++I)
		{
			const Member& M = Ch.Members[I];
			Base[I] = M.Loyalty >= RegularLoyalty && !M.Friend ? 1 : 0;
		}
		for (const Visit& Vs : R.Visits)
		{
			const size_t I = static_cast<size_t>(Vs.Member);
			if (I < MemberCount)
			{
				uint8_t F = Vs.Mod ? 2 : 0;
				F |= !NodAt.empty() && Among(NodAt, R.CanonOf(Vs.Member)) ? 4 : 0;
				Base[I] |= F;
			}
		}
		auto Coming = [&R](size_t I) { return I < R.VisitOf.size() && R.VisitOf[I] >= 0; };
		if (!Ch.Mods.empty() && R.MemberAt.size() == MemberCount)
		{
			for (const int Mi : ModAt)
			{
				if (Mi >= 0 && static_cast<size_t>(Mi) < MemberCount && !Coming(static_cast<size_t>(Mi)))
				{
					Base[static_cast<size_t>(Mi)] |= 2;
				}
			}
		}
		else if (!Ch.Mods.empty())
		{
			for (size_t I = 0; I < MemberCount; ++I)
			{
				if (!Coming(I) && (I < R.MemberCanon.size() ? Among(ModAt, R.MemberCanon[I]) : Ch.IsMod(Ch.Members[I].Name)))
				{
					Base[I] |= 2;
				}
			}
		}
		R.FlagsKey = FlagsNow;
	}
	V.MemberFlags.swap(R.Shown.MemberFlags);
	V.MemberFlags.assign(R.FlagsBase.begin(), R.FlagsBase.end());
	std::vector<uint8_t>& Flags = V.MemberFlags;
	if (R.PresOn)
	{
		R.PresUpdate(Ch, R.Clock);
		for (size_t W = 0; W < R.PresBits.size(); ++W)
		{
			for (uint64_t B = R.PresBits[W]; B != 0; B &= B - 1)
			{
				const size_t I = static_cast<size_t>(R.PresMember[W * 64 + kast_room_detail::RoomLowBit(B)]);
				if (I < MemberCount)
				{
					Flags[I] |= 8;
				}
			}
		}
	}
	else
	{
		for (const Visit& Vs : R.Visits)
		{
			const size_t I = static_cast<size_t>(Vs.Member);
			if (I < MemberCount && HereAt(Vs, R.Clock))
			{
				Flags[I] |= 8;
			}
		}
	}
	play::Fill(*S.Play, V);
	// C-K's L11, the facts ui-kast draws:
	// - WELCOME THE RAID's countdown: the world seconds left in its 2-minute window while the games offer it (rounded up: it reads
	//   1 to its last second, never 0 while open);
	const PlayState& P = *S.Play;
	const bool Welcome = V.Action == View::Slot::WelcomeRaid || (V.Waiting & (1 << static_cast<int>(View::Slot::WelcomeRaid))) != 0;
	V.WelcomeRaidSecs = Welcome && P.RaidAt >= 0.0 ? std::max(1, std::min(120, static_cast<int>(std::ceil((P.RaidAt + 2.0 - P.World) * 60.0 - 1.0e-9)))) : 0;
	// - the directory's impressions over the last 30 grid minutes and the arrivals they brought (R6's numbers, the empty room's);
	{
		double Seen = 0.0;
		int Came = 0;
		for (size_t I = 0; I < 30 && static_cast<int>(I) < R.Done; ++I)
		{
			Seen += R.SeenRing[I];
			Came += R.CameRing[I];
		}
		V.Impressions = static_cast<int>(std::lround(Seen));
		V.ImpressionsCame = Came;
	}
	// - the Poker channels live, yours with them (the rank's field); tonight's Nods by name, guests' included.
	V.LiveChannels = R.RankOf;
	V.Nodded.swap(R.Shown.Nodded); // the last fill's buffer
	V.Nodded.assign(P.Nodded.begin(), P.Nodded.end());
	R.Shown = std::move(V);
}

void RoomState::Close(Stream& S, Channel& Ch, const Inputs& In, Summary& Out, const std::string& RaidOut)
{
	// The stream ends (§2.4, §2.8, §2.10, §7): loyalty from the minutes actually watched, the night's statistics (60+ world
	// minutes only), Heat's commit, the token for a raid out, the LiveMin ring, the log row and the end card's facts.
	RoomState& R = *S.Room;
	R.Bind(S, Ch, In);
	const double Up = std::max(0.0, R.Clock);
	const double World = S.StartWorld + Up;
	const int Day = kast_detail::KastDayOf(World);
	const int Week = kast_detail::KastWeekOf(World);
	const bool Short = Up < kast_room_detail::RoomShort;
	const double Over = std::max(0.0, static_cast<double>(R.Done) - Up);
	const double VMx = std::max(0.0, R.VM - static_cast<double>(R.LastMinuteCount) * Over);
	const double VMOwnx = std::max(0.0, R.VMOwn - static_cast<double>(R.LastMinuteCount - R.LastMinuteRaiders) * Over);
	Out.Minutes = Up;
	Out.Avg = Up > 0.5 ? static_cast<int>(std::lround(VMx / Up)) : R.Watchers;
	Out.AvgOwn = Up > 0.5 ? VMOwnx / Up : static_cast<double>(R.Watchers - R.RaidersHere);
	Out.Peak = S.Peak;
	Out.PeakAt = R.PeakAt;
	Out.PeakMoment = R.PeakMoment;
	Out.HeatBefore = R.HeatStart;
	Out.NormBefore = R.NormBefore;
	Out.Sources = R.NightSources;
	for (const Visit& V : R.Visits)
	{
		Out.Sources[static_cast<size_t>(View::Source::Members)] += V.Arrived || WatchedTo(V, Up) > 0.0 ? 1 : 0;
	}
	Out.FollowSources = R.FollowSources;
	Out.Chatted = static_cast<int>(S.Seen.size());
	Out.RunAlerts = R.AlertSent;
	// The Run Alert's returners as the room knows them at the end (§1.4, a fact at every pace): every alerted member who has been
	// here since their first alert (the live strip's own rule, Advance's scan, so the two agree).
	{
		int Back = 0;
		for (const Visit& V : R.Visits)
		{
			Back += V.Pulled && (HereAt(V, Up) || WatchedTo(V, Up) > V.PullWatch) ? 1 : 0;
		}
		R.AlertBack = Back;
	}
	Out.RunAlertsBack = R.AlertBack;
	Out.RankStart = R.RankStart;
	Out.RankBest = R.RankBest;
	Out.Fc = R.Fc;
	Out.RenewCents = R.RenewSince;
	Out.PingReached = R.PingSent ? R.PingReached : 0;
	Out.PingOf = R.PingOf;
	Out.PingParts = R.PingParts;
	Out.SponsorPaid = R.SponsorPaid;
	for (size_t Ri = 0; Ri < R.Raids.size(); ++Ri)
	{
		const Raid& Rd = R.Raids[Ri];
		Summary::RaidHeld H;
		H.From = Rd.From;
		H.Size = Rd.Size;
		H.Followed = Rd.Followed;
		H.Broadcast = Rd.Broadcast;
		H.At15 = Rd.At15;
		if (H.At15 < 0)
		{
			int Still = 0;
			for (const Raider& X : R.Raiders)
			{
				Still += X.RaidIx == static_cast<int>(Ri) && R.RaiderHere(X, Up) ? 1 : 0;
			}
			H.At15 = Still + Rd.Followed;
		}
		Out.BroadcastSize += Rd.Broadcast ? Rd.Size : 0;
		Out.RaidsHeld.push_back(H);
	}
	// The roll call (§5.3 K17): members here for the whole deep run, or for most of a night without one.
	for (const Visit& V : R.Visits)
	{
		const double W = WatchedTo(V, Up);
		const bool Whole = R.StageMoments > 0 ? V.WholeRun && V.Arrived : Up >= 30.0 && W >= 0.8 * Up;
		if (Whole && W > 0.0)
		{
			Out.Regulars.push_back(Ch.Members[static_cast<size_t>(V.Member)].Name);
		}
	}
	// Ending with a raid: tonight's viewers go to a small channel. A token for a raid back only from a stream of 60+ world
	// minutes and only if this channel wasn't raided in the last 7 world days (§2.7.1): no ping-pong, no restart farming.
	if (!RaidOut.empty())
	{
		Out.RaidedOut = RaidOut;
		Out.RaidSize = std::max(1, static_cast<int>(std::round(static_cast<double>(R.Watchers) * 0.7)));
		++Ch.RaidsOut;
		const auto It = Ch.RaidedDay.find(RaidOut);
		if (growth::EarnsToken(Day, It != Ch.RaidedDay.end() ? It->second : -1000000, Up))
		{
			const auto G = Ch.Goodwill.find(RaidOut);
			Ch.Goodwill[RaidOut] = std::min(3, std::max(0, G != Ch.Goodwill.end() ? G->second : 0) + 1);
			Ch.RaidedDay[RaidOut] = Day;
		}
	}
	// Loyalty: everyone who watched tonight is a little more part of this (§2.8.1: the slots' minutes, x1.3 on schedule, talking,
	// raiding out, x1.6 when the streamer Nodded or answered them). Affinity no longer creeps (§2.8.2).
	for (const Visit& V : R.Visits)
	{
		const double W = WatchedTo(V, Up);
		if (W < 0.5)
		{
			continue;
		}
		Member& M = Ch.Members[static_cast<size_t>(V.Member)];
		if (M.FirstSeen < S.StartWorld - 0.01)
		{
			++Out.Returning;
		}
		++M.Streams;
		M.WatchMinutes += W;
		M.LastSeen = World;
		M.Hw = growth::HwToday(M.Hw, M.HwDay, Day) + W / 60.0;
		M.HwDay = Day;
		M.Messages += V.TalkSlots;
		const bool WasRegular = M.Loyalty >= RegularLoyalty;
		double Gain = 0.12 * std::pow(std::min(W, 180.0) / 60.0, 0.75) * std::max(0.0, M.Affinity - M.Loyalty);
		Gain *= (S.Scheduled ? 1.3 : 1.0) * (1.0 + 0.6 * (V.Engaged ? 1.0 : 0.0) + 0.3 * (R.TalkerTonight(V, M.Loyalty) ? 1.0 : 0.0)) * (RaidOut.empty() ? 1.0 : 1.05) * (V.Noticed ? 1.6 : 1.0);
		M.Loyalty = std::min(1.0, M.Loyalty + Gain);
		if (!WasRegular && M.Loyalty >= RegularLoyalty && !M.Friend)
		{
			++Out.NewRegulars;
			++Ch.WeekRegulars;
			Out.Xp += 40.0;
			Ch.Xp += 40.0;
		}
	}
	Ch.Xp += S.Tonight.Xp;
	// Heat's commit (§2.4): the night against Start's forecast and how forecasts usually land here, a rare deep run's news on
	// top, within the week's cap and the soft ceiling; showing up outside the cap; a record peak after stream 3.
	double Commit = 0.0;
	if (!Short)
	{
		const double Lr = std::log((static_cast<double>(Out.Avg) + 1.0) / (R.FcRaw + 1.0));
		const double Used = Ch.GainWeek == Week ? Ch.GainUsed : 0.0;
		const growth::NightGain G = growth::Gain(Lr, Ch.Bias, R.RunValue, Ch.RunNorm, Used, kast_room_detail::RoomShownHeat(Ch, World));
		Commit = G.Gain;
		if (G.Raw > 0.0)
		{
			// The week's positive night gains count before the soft ceiling (the model's gain_week).
			Ch.GainWeek = Week;
			Ch.GainUsed = Used + std::min(G.Raw, std::max(0.0, 35.0 - Used));
		}
		const double Show = growth::ShowUp(Ch.ShowWeek, Ch.ShowDays, Day);
		if (Show > 0.0)
		{
			if (Ch.ShowWeek != Week)
			{
				Ch.ShowWeek = Week;
				Ch.ShowDays = 0;
			}
			Ch.ShowDays |= 1 << kast_detail::KastWeekday(Day);
		}
		Commit += Show;
		Ch.Bias = Ch.Streams > 1 ? 0.9 * Ch.Bias + 0.1 * Lr : Lr;
		Ch.RunNorm = 0.9 * Ch.RunNorm + 0.1 * R.RunValue;
		Ch.Norm = Ch.Streams > 1 ? 0.8 * Ch.Norm + 0.2 * static_cast<double>(Out.Avg) : static_cast<double>(Out.Avg);
		Ch.BestAvg = std::max(Ch.BestAvg, Out.Avg);
		// The facts behind the next night's why lines (§2.4): tonight's deepest armed stage, tonight against its forecast as shown.
		if (R.BestStageKind >= static_cast<int>(growth::RunStage::FinalTable))
		{
			R.WhyStage = R.BestStageKind;
			R.WhyStageRunners = R.BestStageEntrants;
			R.WhyStageDay = Day;
		}
		const double ShownFc = R.Fc.Centre * std::exp(R.BiasStart);
		R.WhyOverPct = ShownFc > 0.0 ? static_cast<int>(std::floor(100.0 * (static_cast<double>(Out.Avg) / ShownFc - 1.0))) : 0;
		// Under it, the same truncation the other way ("21% under your forecast" for an average 21.x% short of it: C-K's L5).
		R.WhyUnderPct = ShownFc > 0.0 ? static_cast<int>(std::floor(100.0 * (1.0 - static_cast<double>(Out.Avg) / ShownFc))) : 0;
		R.WhyOverDay = Day;
		++R.WhyEpoch;
	}
	kast_room_detail::RoomAddHeat(Ch, World, Commit);
	if (!Short && S.Peak > Ch.Peak && Ch.Streams > 3)
	{
		kast_room_detail::RoomAddHeat(Ch, World, kast_room_detail::RoomHeatRecord);
		R.WhyRecordPeak = S.Peak; // "Your biggest room yet (41 at the peak)"
		R.WhyRecordDay = Day;
		++R.WhyEpoch;
	}
	Out.GainSoFar = Commit;
	Out.Why = R.WhyNow(Ch, World); // Heat's top two reasons after tonight's commit (the end card's, C-K's L5)
	const double HeatAfter = kast_room_detail::RoomShownHeat(Ch, World);
	if (growth::Tier(HeatAfter) > R.TierStart && growth::Tier(HeatAfter) >= 2)
	{
		// Tier-up into Running hot or higher (§5.3 K4): KastRise through the cue queue, and chat cooks.
		Notice N;
		N.Type = Notice::Kind::Milestone;
		N.Sound = static_cast<int>(SoundId::KastRise);
		S.Notices.push_back(N);
	}
	if (!Short)
	{
		Ch.MinutesLive += Up;
		Ch.ViewerMinutes += VMx;
		Ch.WeekMinutes += Up;
		++Ch.WeekStreams;
		Ch.Peak = std::max(Ch.Peak, S.Peak);
		StreamLog Lg;
		Lg.Start = S.StartWorld;
		Lg.Minutes = Up;
		Lg.Avg = Out.Avg;
		Lg.Peak = Out.Peak;
		Lg.Follows = S.Tonight.Follows;
		Lg.Subs = S.Tonight.Subs + S.Tonight.Gifted;
		Lg.Cents = S.Tonight.Total() - S.Tonight.SponsorCents;
		Lg.Title = kast_detail::TitleOf(Ch).Text;
		Lg.Returning = Out.Returning;
		Lg.OnSchedule = S.Scheduled;
		Lg.AvgOwn = static_cast<int>(std::lround(Out.AvgOwn));
		Lg.PeakAt = static_cast<double>(R.PeakAt);
		Lg.PeakMoment = R.PeakMoment;
		Lg.HeatAfter = Ch.Heat;
		Ch.Log.insert(Ch.Log.begin(), Lg);
		if (Ch.Log.size() > 60)
		{
			Ch.Log.pop_back();
		}
	}
	// H7's ring takes every stream, short ones too, on the world day it ended (§2.2; synth_kast.py's live7 books a stream at its
	// end). S-K (2026-10-24): split day by day, a stream past midnight lost its evening from the window a week later, and a
	// grinder's H7 read 37 hours against the model's 42 (Stamina 0.70, not the floor's 0.60).
	growth::LiveAdd(Ch.LiveMin, Ch.LiveDay, Day, static_cast<int>(std::lround(Up)));
	Ch.LiveSince = -1.0;
	Ch.GainSoFar = 0.0;
	Out.PendingCents = Ch.PendingCents;
	Out.ClipItOpen = Up >= 30.0 && Ch.CotnDay != Day && !(growth::Granted(Ch.GrantDay, Ch.GrantMask, Day, growth::GrantClip1) && growth::Granted(Ch.GrantDay, Ch.GrantMask, Day, growth::GrantClip2));
	R.EarnedSubsAtStop = Ch.EarnedSubs;
	play::Close(*S.Play, Out);
	R.RoomLive = false;
}

std::string RoomState::Describe(Stream& S, Channel& Ch, const Inputs& In)
{
	// ss.Kast.Room: who is here by source, the guests with their leave times, the next slot's transitions.
	RoomState& R = *S.Room;
	R.Bind(S, Ch, In);
	static const char* const Names[] = {"members", "directory", "fresh", "rising", "railbirds", "raid", "broadcast", "word", "video", "clip", "lurkers"};
	std::string Out = "Room at uptime " + Fixed(R.Clock, 1) + ": " + std::to_string(R.Watchers) + " watching (";
	bool First = true;
	for (size_t I = 0; I < R.SourcesNow.size(); ++I)
	{
		if (R.SourcesNow[I] > 0)
		{
			Out += std::string(First ? "" : ", ") + std::to_string(R.SourcesNow[I]) + " " + Names[I];
			First = false;
		}
	}
	Out += "); " + std::to_string(R.Visits.size()) + " members coming tonight, " + std::to_string(R.ModsHere.size()) + " mods here.";
	int Listed = 0;
	for (const Visit& V : R.Visits)
	{
		if (Listed >= 12)
		{
			break;
		}
		const Member& M = Ch.Members[static_cast<size_t>(V.Member)];
		const bool Here = HereAt(V, R.Clock);
		Out += "\n  " + M.Name + " (L " + Fixed(M.Loyalty, 2) + "): " + (Here ? "here" : V.Arrived ? "away" : "on the way") +
			(V.Flip < kast_room_detail::RoomInf ? std::string(Here ? ", leaves at " : ", comes at ") + Fixed(V.Flip, 1) : std::string()) + ", watched " + Fixed(WatchedTo(V, R.Clock), 0);
		++Listed;
	}
	Listed = 0;
	for (const Guest& G : R.Guests)
	{
		if (Listed >= 12 || !R.GuestHere(G, R.Clock))
		{
			continue;
		}
		Out += "\n  guest " + (G.Pool >= 0 ? S.Pool[static_cast<size_t>(G.Pool)] : G.Bot) + " (" + Names[G.Source] + "): leaves at " + Fixed(G.Leave, 1) +
			(G.FollowAt >= 0 ? ", follows at " + std::to_string(G.FollowAt) : std::string());
		++Listed;
	}
	Out += "\n  crowd " + std::to_string(R.CrowdHere) + ", raiders " + std::to_string(R.RaidersHere) + ", lurkers " + std::to_string(R.LurkersNow) + "; next slot at " +
		Fixed(kast_room_detail::RoomSlotEnd(R.Clock), 0) + ".";
	return Out;
}

Stream::Stream(const std::string& Seed) : R(Seed), Room(std::make_unique<RoomState>()), Play(std::make_unique<PlayState>())
{
}

// The creator update (Phase A-K): out of line, where RoomState and PlayState are complete.
Stream::~Stream() = default;
Stream::Stream(Stream&& Other) = default;
Stream& Stream::operator=(Stream&& Other) = default;

// ---- The creator update's Room (SPEC_CREATORS §1.2, §10.7)

void Stream::AdvanceTo(Channel& Ch, const Inputs& In)
{
	RoomState::Advance(*this, Ch, In);
}

void Stream::ChatStep(Channel& Ch, const Inputs& In, double DReal)
{
	RoomState::Display(*this, Ch, In, std::max(0.0, DReal));
}

void Stream::Recount(Channel& Ch, const Inputs& In)
{
	AdvanceTo(Ch, In);
}

const View& Stream::GetView() const
{
	// A stream that was moved from has no Room: it reads the empty view.
	static const View KastNoRoomView;
	return Room ? Room->Shown : KastNoRoomView;
}

int Stream::Count() const
{
	return GetView().Count;
}

// The verbs: each runs its kast::play function (KastPlay.cpp, core-kastplay) through the RoomView the Room implements.

bool Stream::Nod(Channel& Ch, const std::string& Name)
{
	if (!Live || !Room)
	{
		return false;
	}
	Room->Bind(*this, Ch, Room->LastIn);
	const bool Ok = play::Nod(*Play, *Room, Ch, Name);
	RoomState::Flush(*this, Room->LastIn);
	return Ok;
}

bool Stream::RailCall(Channel& Ch, const Inputs& In)
{
	if (!Live || !Room)
	{
		return false;
	}
	RoomState::Advance(*this, Ch, In);
	const bool Ok = play::RailCall(*Play, *Room, Ch, In);
	RoomState::Flush(*this, In);
	return Ok;
}

bool Stream::WelcomeRaid(Channel& Ch)
{
	if (!Live || !Room)
	{
		return false;
	}
	Room->Bind(*this, Ch, Room->LastIn);
	const bool Ok = play::WelcomeRaid(*Play, *Room, Ch);
	RoomState::Flush(*this, Room->LastIn);
	return Ok;
}

bool Stream::SetTheScene(Channel& Ch)
{
	if (!Live || !Room)
	{
		return false;
	}
	Room->Bind(*this, Ch, Room->LastIn);
	const bool Ok = play::SetTheScene(*Play, *Room, Ch);
	RoomState::Flush(*this, Room->LastIn);
	return Ok;
}

bool Stream::CallIt(Channel& Ch, bool Holds)
{
	if (!Live || !Room)
	{
		return false;
	}
	Room->Bind(*this, Ch, Room->LastIn);
	const bool Ok = play::CallIt(*Play, *Room, Ch, Holds);
	RoomState::Flush(*this, Room->LastIn);
	return Ok;
}

bool Stream::ClipIt(Channel& Ch, const Inputs& In)
{
	// Clip of the Night (§2.7.2): the end card's one click clips the ended stream's best moment, even at 0 viewers (streamers
	// clip themselves), once a world day, in the Clip Lane.
	if (Live || !Last.Valid || !Last.ClipItOpen)
	{
		return false;
	}
	const int Day = kast_detail::KastDayOf(In.World);
	const bool Lane1 = !growth::Granted(Ch.GrantDay, Ch.GrantMask, Day, growth::GrantClip1);
	const bool Lane2 = !growth::Granted(Ch.GrantDay, Ch.GrantMask, Day, growth::GrantClip2);
	if (Ch.CotnDay == Day || (!Lane1 && !Lane2))
	{
		Last.ClipItOpen = false;
		return false;
	}
	const MomentNote* Best = nullptr;
	for (const MomentNote& N : Moments)
	{
		if (!Best || kast_detail::HitOf(N.Kind).Hype >= kast_detail::HitOf(Best->Kind).Hype)
		{
			Best = &N;
		}
	}
	const Moment Kind = Best ? Best->Kind : Moment::Register;
	const double At = Best ? Best->World : StartWorld;
	const uint64_t Mk = static_cast<uint64_t>(std::max(0.0, std::floor(At - StartWorld)));
	const uint64_t SeedK = Room ? Room->SeedK : 0;
	const uint64_t StreamK = static_cast<uint64_t>(std::max(0, Ch.Streams));
	growth::UseGrant(Ch.GrantDay, Ch.GrantMask, Day, Lane1 ? growth::GrantClip1 : growth::GrantClip2);
	Ch.CotnDay = Day;
	const double Hn = growth::HeatH(kast_room_detail::RoomShownHeat(Ch, In.World));
	const double Hit = std::max(16.0, kast_detail::HitOf(Kind).Hype);
	Clip C;
	C.Kind = static_cast<int>(Kind);
	C.At = At;
	Rng TitleDraws(std::to_string(SeedK) + ":kast:cotn:" + std::to_string(StreamK));
	C.Title = kast_detail::ClipTitle(Kind, In.Hero, std::string(), TitleDraws);
	C.By = In.Hero;
	C.Lane = true; // Clip of the Night is a Clip Lane clip (§2.7.2; Top clips' flag, C-K's L11)
	C.Reach = growth::ClipReach(static_cast<double>(Best ? std::max(Best->Viewers, Last.Avg) : Last.Avg), growth::NormalAt(growth::Unit(SeedK, StreamK, growth::Tag::ClipReach, Mk, 128 + static_cast<uint64_t>(Kind))), Hn, Hit);
	if (growth::Unit(SeedK, StreamK, growth::Tag::ClipReach, Mk, 192 + static_cast<uint64_t>(Kind)) < kast_room_detail::RoomClipTop)
	{
		C.Reach *= kast_room_detail::RoomClipTopX;
		kast_room_detail::RoomAddHeat(Ch, In.World, kast_room_detail::RoomHeatClip);
		if (Room)
		{
			Room->WhyClipDay = Day; // Heat's why line: "Your clip hit the top of the Kast feed" (as a live clip's)
			++Room->WhyEpoch;
		}
	}
	Ch.Clips.insert(Ch.Clips.begin(), C);
	if (Ch.Clips.size() > 16)
	{
		std::stable_sort(Ch.Clips.begin() + 1, Ch.Clips.end(), [](const Clip& A, const Clip& B) { return A.Reach > B.Reach; });
		Ch.Clips.pop_back();
	}
	Last.ClipItOpen = false;
	Last.BestClip = C.Title;
	Last.BestClipLane = true;
	++Last.Clips;
	return true;
}

growth::Band Stream::Forecast(const Channel& Ch, const Inputs& In)
{
	// "Tonight: likely 8-14" (§2.11): the go-live's Pa for every member and the directory's rate at the start, no draws.
	const int Streams = Ch.Streams + 1;
	const bool OnSched = Ch.OnSchedule(In.World);
	const int Today = kast_detail::KastDayOf(In.World);
	const double Shown = growth::HeatShown(Ch.Heat, Ch.HeatAt, In.World, Ch.LastStreamDay);
	const double H0 = growth::HeatH(Shown);
	kast_room_detail::RoomPingNight Night;
	Night.Streams = Streams;
	Night.Sched = kast_room_detail::RoomSchedOf(Ch, OnSched);
	Night.ToD = kast_room_detail::RoomToD(In.World);
	Night.H = H0;
	Night.Today = Today;
	Night.WelcomeBack = Ch.Streams >= 1 && Ch.LastStreamDay >= 0 && Today - Ch.LastStreamDay >= 5;
	const bool Ping = In.World - Ch.PingAt >= kast_room_detail::RoomPingGap;
	std::vector<double> Attend;
	std::vector<double> Shares;
	Attend.reserve(Ch.Members.size() + 2);
	Shares.reserve(Ch.Members.size() + 2);
	for (const Member& M : Ch.Members)
	{
		const double ModOn = kast_room_detail::RoomModOnline(Ch, M.Name);
		const double Pa = kast_room_detail::RoomPaOf(M, ModOn, Night, In.World);
		Attend.push_back(Ping ? Pa : Pa * kast_room_detail::RoomLateShare);
		Shares.push_back(kast_room_detail::RoomShare(M.Loyalty, M.Friend, Streams, ModOn > 0.0));
	}
	if (Ch.Streams == 0 && Ch.Members.empty())
	{
		// Dee and Mei, who join at the first go-live.
		for (const double L : {0.85, 0.7})
		{
			Attend.push_back(1.0);
			Shares.push_back(kast_room_detail::RoomShare(L, true, 1, false));
		}
	}
	// The lurker pool (C-K's L8): the same expected count the go-live's forecast reads (Begin).
	kast_room_detail::RoomLurkEntries(Ch, Attend, Night.Sched, In.World, Attend, Shares);
	const double Q = Quality(In);
	const double A0 = kast_room_detail::RoomA0(Ch, In, Q, growth::Disc(Ch.Level(), Ch.Streak), growth::Lane(Streams), growth::Rising(Streams, Ch.Norm), H0,
		growth::Novelty(growth::H7(Ch.LiveMin, Ch.LiveDay, kast_room_detail::RoomH7Day(Ch, Today))), In.Seed);
	return growth::Forecast(Attend, Shares, A0, Q, Ch.Bias);
}

std::string Stream::DebugRoom(Channel& Ch, const Inputs& In, const std::string& Args)
{
	// ss.Kast's verbs that need the Room (Session::DebugKast forwards them).
	std::vector<std::string> W;
	{
		std::string Word;
		for (const char C : Args)
		{
			if (C == ' ' || C == '\t')
			{
				if (!Word.empty())
				{
					W.push_back(Word);
					Word.clear();
				}
				continue;
			}
			Word.push_back(static_cast<char>(C >= 'A' && C <= 'Z' ? C - 'A' + 'a' : C));
		}
		if (!Word.empty())
		{
			W.push_back(Word);
		}
	}
	const std::string Verb = W.empty() ? std::string() : W[0];
	if (Verb == "forecast")
	{
		const growth::Band B = Forecast(Ch, In);
		return "Forecast: likely " + std::to_string(static_cast<int>(std::lround(B.Lo))) + "-" + std::to_string(static_cast<int>(std::lround(B.Hi))) + " (centre " + Fixed(B.Centre, 1) +
			"); " + std::to_string(Ch.Members.size()) + " members, Bias " + Fixed(Ch.Bias, 2) + ", Heat " + Fixed(growth::HeatShown(Ch.Heat, Ch.HeatAt, In.World, Ch.LastStreamDay), 0) + ", " +
			(Ch.OnSchedule(In.World) ? "on schedule" : Ch.ScheduleDays == 0 ? "no posted schedule" : "off schedule") + ".";
	}
	if (!Live || !Room || !Room->RoomLive)
	{
		return Verb == "room" || Verb == "raid" || Verb == "moment" ? std::string("Not live.") : std::string();
	}
	if (Verb == "room")
	{
		RoomState::Advance(*this, Ch, In);
		return RoomState::Describe(*this, Ch, In);
	}
	if (Verb == "raid")
	{
		RoomState::Advance(*this, Ch, In);
		const int Size = W.size() > 1 ? std::max(1, std::atoi(W[1].c_str())) : 14;
		const bool Broadcast = W.size() > 2 && W[2] == "broadcast";
		const std::vector<SmallChannel> Net = Network(In.World);
		const std::string From = Broadcast ? kast_room_copy::RoomCopyBroadcastFrom(In.DeepSite) : Net.empty() ? std::string("kat_v") : Net.front().Name;
		RoomState::AddRaid(*this, Ch, In, From, Net.empty() ? Violet : Net.front().Color, Size, Broadcast, Room->Clock, 5);
		RoomState::Advance(*this, Ch, In);
		return "Raid in: " + From + " with " + std::to_string(Size) + (Broadcast ? " (the Broadcast)." : ".");
	}
	if (Verb == "moment")
	{
		static const std::pair<const char*, growth::RunStage> Stages2[] = {{"bubble", growth::RunStage::Bubble}, {"itm", growth::RunStage::Itm}, {"ft", growth::RunStage::FinalTable},
			{"top3", growth::RunStage::Top3}, {"win", growth::RunStage::Win}};
		growth::RunStage Stage = growth::RunStage::None;
		for (const auto& St : Stages2)
		{
			Stage = W.size() > 1 && W[1] == St.first ? St.second : Stage;
		}
		const bool AllIn = W.size() > 1 && W[1] == "allin";
		if (Stage == growth::RunStage::None && !AllIn)
		{
			return "ss.Kast.Moment <bubble|itm|ft|top3|win|allin> [prestige] [entrants]";
		}
		Room->Held = !AllIn;
		Room->HeldStage = static_cast<int>(Stage);
		Room->HeldP = W.size() > 2 ? std::max(0.25, std::atof(W[2].c_str())) : 1.40;
		Room->HeldEntrants = W.size() > 3 ? std::max(10, std::atoi(W[3].c_str())) : 980;
		Inputs Held = In;
		Held.Prestige = Room->HeldP;
		Held.DeepEntrants = Room->HeldEntrants;
		Held.DeepRemaining = kast_room_detail::RoomHeldRemaining(Room->HeldStage, Room->HeldEntrants);
		Held.DeepTableSize = 9;
		Held.Depth = growth::Depth(Held.DeepEntrants, Held.DeepRemaining);
		Held.DeepTable = 0;
		Held.ThisTable = 0;
		Held.FrontTable = 0;
		Held.Entrants = Held.DeepEntrants;
		static const Moment Kinds[] = {Moment::AllIn, Moment::Bubble, Moment::InTheMoney, Moment::FinalTable, Moment::Top3, Moment::Win};
		OnMoment(Ch, Held, Kinds[AllIn ? 0 : static_cast<int>(Stage)], "somebody", 1.0);
		return std::string(AllIn ? "All-in on stream" : "Deep run held at ") + (AllIn ? "" : W[1]) + (AllIn ? "." : " (P " + Fixed(Room->HeldP, 2) + ", " + Grouped(Room->HeldEntrants) + " runners).");
	}
	return std::string();
}

void Stream::DebugView(const View& V)
{
	if (Room)
	{
		Room->Shown = V;
	}
}

double Stream::AdCooldown(double World) const
{
	return std::max(0.0, 8.0 - (World - LastAdWorld));
}

double Stream::Quality(const Inputs& In)
{
	double Q = In.Gear.Quality;
	if (In.Tables > In.Gear.StreamTables)
	{
		Q *= 0.75; // dropped frames
	}
	return std::min(1.0, std::max(0.0, Q));
}

double Stream::Expected(const Channel& Ch, const Inputs& In)
{
	const double Sched = Ch.ScheduleDays == 0 ? 0.75 : Ch.OnSchedule(In.World) ? 1.3 : 0.6;
	const double Tod = std::max(0.4, std::min(1.0, kast_detail::TimeOfDay(In.World) / 1.25));
	double Members = 0.0;
	for (const Member& M : Ch.Members)
	{
		const double Pa = M.Friend ? 0.6 : 0.03 + 0.85 * std::pow(M.Loyalty, 1.15);
		Members += std::min(0.95, Pa * Sched * Tod) * std::min(1.0, (15.0 + 150.0 * M.Loyalty) / 120.0);
	}
	const double Q = Quality(In);
	const double A = (1.5 + 2.0 * Q) * Ch.Discoverability() * kast_detail::Competition(In.World) * (0.4 + 0.6 * kast_detail::TimeOfDay(In.World) / 1.25);
	return Members + A / 60.0 * (4.0 + 6.0 * Q);
}

void Stream::Say(const Inputs& In, const std::string& Who, const std::string& Text, LineKind Kind, int Badges, uint32_t Color, Chips Cents)
{
	ChatMsg M;
	M.Id = NextId++;
	M.Who = Who;
	M.Text = Text;
	M.Kind = Kind;
	M.Badges = Badges;
	M.NameColor = Color;
	M.At = In.Real;
	M.Cents = Cents;
	Chat.push_back(M);
	if (Chat.size() > 140)
	{
		Chat.erase(Chat.begin(), Chat.begin() + static_cast<std::ptrdiff_t>(Chat.size() - 140));
	}
}

std::string Stream::Chatter(Channel& Ch, const Inputs& In, int& Badges, uint32_t& Color)
{
	// Who talks (R1): someone in the room now, through the stream's own R (display only): the community members here (the loyal
	// ones most), a guest passing through, a named raider. Lurkers and the crowd never speak; a timed-out name can't type.
	// Nobody named here: nobody talks ("").
	Badges = 0;
	Color = 0;
	if (!Room || !Room->RoomLive)
	{
		return std::string();
	}
	const RoomState& Rm = *Room;
	const double T = Rm.Clock;
	// The names timed out right now (kast_detail::KastTimedOut's rule), gathered once a line rather than looked up once a person
	// (the frame budget, §9.4 #13).
	std::vector<const std::string*> Out;
	for (const std::pair<const std::string, double>& O : TimedOut)
	{
		if (O.second > In.Real)
		{
			Out.push_back(&O.first);
		}
	}
	auto IsOut = [&Out](const std::string& Name) {
		for (const std::string* O : Out)
		{
			if (*O == Name)
			{
				return true;
			}
		}
		return false;
	};
	if (Rm.PresOn && Out.empty())
	{
		// Presence by transition (C-K's M2): the members here in visit order with their 0.2 + L, then the guests and the named
		// raiders, summed and walked in the order and with the weights below, without building the list of every name.
		Room->PresUpdate(Ch, T);
		double Sum = 0.0;
		const std::string* Tail = nullptr; // the list's last name (a guest or a raider), else the last member's (TailVi)
		size_t TailVi = SIZE_MAX;
		for (size_t W = 0; W < Rm.PresBits.size(); ++W)
		{
			for (uint64_t B = Rm.PresBits[W]; B != 0; B &= B - 1)
			{
				const size_t Vi = W * 64 + kast_room_detail::RoomLowBit(B);
				Sum += Rm.PresTalk[Vi];
				TailVi = Vi;
			}
		}
		for (const RoomState::Guest& G : Rm.Guests)
		{
			if (G.Kind == 0 && G.Pool >= 0 && Rm.GuestHere(G, T))
			{
				Sum += 0.1;
				Tail = &Pool[static_cast<size_t>(G.Pool)];
			}
		}
		for (const RoomState::Raider& X : Rm.Raiders)
		{
			if (!X.Name.empty() && Rm.RaiderHere(X, T))
			{
				Sum += 0.15;
				Tail = &X.Name;
			}
		}
		if (Tail == nullptr && TailVi != SIZE_MAX)
		{
			Tail = &Ch.Members[static_cast<size_t>(Rm.PresMember[TailVi])].Name;
		}
		if (Tail == nullptr || Sum <= 0.0)
		{
			return std::string();
		}
		double Pick = R.Next() * Sum;
		const std::string* Name = Tail;
		bool Found = false;
		for (size_t W = 0; W < Rm.PresBits.size() && !Found; ++W)
		{
			for (uint64_t B = Rm.PresBits[W]; B != 0 && !Found; B &= B - 1)
			{
				const size_t Vi = W * 64 + kast_room_detail::RoomLowBit(B);
				Pick -= Rm.PresTalk[Vi];
				if (Pick <= 0.0)
				{
					Name = &Ch.Members[static_cast<size_t>(Rm.PresMember[Vi])].Name;
					Found = true;
				}
			}
		}
		for (size_t Gi = 0; Gi < Rm.Guests.size() && !Found; ++Gi)
		{
			const RoomState::Guest& G = Rm.Guests[Gi];
			if (G.Kind == 0 && G.Pool >= 0 && Rm.GuestHere(G, T))
			{
				Pick -= 0.1;
				if (Pick <= 0.0)
				{
					Name = &Pool[static_cast<size_t>(G.Pool)];
					Found = true;
				}
			}
		}
		for (size_t Xi = 0; Xi < Rm.Raiders.size() && !Found; ++Xi)
		{
			const RoomState::Raider& X = Rm.Raiders[Xi];
			if (!X.Name.empty() && Rm.RaiderHere(X, T))
			{
				Pick -= 0.15;
				if (Pick <= 0.0)
				{
					Name = &X.Name;
					Found = true;
				}
			}
		}
		Badges = kast_room_detail::RoomBadges(Rm, Ch, *Name, In.World);
		Color = kast_detail::NameColor(*Name);
		Seen.insert(*Name);
		Room->Spoke.push_back({*Name, In.Real});
		return *Name;
	}
	std::vector<std::pair<const std::string*, double>> Who;
	Who.reserve(Rm.Visits.size() + Rm.Guests.size() + Rm.Raiders.size());
	double Total = 0.0;
	for (const RoomState::Visit& V : Rm.Visits)
	{
		if (RoomState::HereAt(V, T))
		{
			const Member& M = Ch.Members[static_cast<size_t>(V.Member)];
			if (Out.empty() || !IsOut(M.Name))
			{
				Who.emplace_back(&M.Name, 0.2 + M.Loyalty);
				Total += 0.2 + M.Loyalty;
			}
		}
	}
	for (const RoomState::Guest& G : Rm.Guests)
	{
		if (G.Kind == 0 && G.Pool >= 0 && Rm.GuestHere(G, T) && (Out.empty() || !IsOut(Pool[static_cast<size_t>(G.Pool)])))
		{
			Who.emplace_back(&Pool[static_cast<size_t>(G.Pool)], 0.1);
			Total += 0.1;
		}
	}
	for (const RoomState::Raider& X : Rm.Raiders)
	{
		if (!X.Name.empty() && Rm.RaiderHere(X, T) && (Out.empty() || !IsOut(X.Name)))
		{
			Who.emplace_back(&X.Name, 0.15);
			Total += 0.15;
		}
	}
	if (Who.empty() || Total <= 0.0)
	{
		return std::string();
	}
	double Pick = R.Next() * Total;
	const std::string* Name = Who.back().first;
	for (const std::pair<const std::string*, double>& W : Who)
	{
		Pick -= W.second;
		if (Pick <= 0.0)
		{
			Name = W.first;
			break;
		}
	}
	Badges = kast_room_detail::RoomBadges(Rm, Ch, *Name, In.World);
	Color = kast_detail::NameColor(*Name);
	Seen.insert(*Name);
	Room->Spoke.push_back({*Name, In.Real});
	return *Name;
}

std::string Stream::LineFor(Channel& Ch, const Inputs& In, LineKind& Kind)
{
	Kind = LineKind::Chat;
	// Followers ask about the channel's own running jokes once it has some.
	if (Ch.Affiliate && R.Chance(0.04))
	{
		static const std::vector<std::string> Own = {"{rent} {rent}", "{ship}", "sub hype {ship}", "{tilt} incoming"};
		return R.Pick(Own);
	}
	std::string Line;
	const bool Recent = In.Real - LastMomentReal < 14.0;
	const bool Dropping = In.Tables > In.Gear.StreamTables;
	if (Recent && R.Chance(0.75))
	{
		Line = R.Pick(kast_detail::Lines(LastMoment));
	}
	else if (AdRunning(In.Real) && R.Chance(0.3))
	{
		static const std::vector<std::string> Ads = {"ads kastSalt", "ad break, grab a drink", "brb ads", "ads again?", "the ad is longer than the hand"};
		Line = R.Pick(Ads);
	}
	else if (In.Real - QualityNagAt > 40.0 && R.Chance(Dropping ? 0.25 : 0.06))
	{
		QualityNagAt = In.Real;
		Line = R.Pick(kast_detail::QualityLines(In.Gear, Dropping));
	}
	else if (In.Tilt > 0.55 && R.Chance(0.15))
	{
		static const std::vector<std::string> Tilt = {"you look tilted", "take a break?", "breathe", "the face kastLUL", "stay calm", "don't punt it", "{tilt}"};
		Line = R.Pick(Tilt);
	}
	else if (R.Chance(0.12))
	{
		Line = R.Pick(kast_detail::Greetings());
	}
	else if (In.AtTable)
	{
		Line = R.Pick(kast_detail::IdleTable());
	}
	else
	{
		Line = R.Pick(kast_detail::IdleLobby());
	}
	return Line;
}

void Stream::Queue(const Alert& A)
{
	Alert Logged = A;
	Logged.At = LastReal;
	Feed.insert(Feed.begin(), Logged);
	if (Feed.size() > 60)
	{
		Feed.pop_back();
	}
	// A burst of follows shows as one alert.
	if (A.Kind == AlertKind::Follow && !Alerts.empty())
	{
		Alert& Back = Alerts.back();
		if (Back.Kind == AlertKind::Follow && (Alerts.size() > 1 || Back.At < 0.0))
		{
			Back.Count += A.Count;
			return;
		}
	}
	if (Alerts.size() >= 10 && A.Kind == AlertKind::Follow)
	{
		return;
	}
	Alerts.push_back(A);
	Notice N;
	switch (A.Kind)
	{
	case AlertKind::Follow: N.Type = Notice::Kind::Chime; break;
	case AlertKind::Sub:
	case AlertKind::Gift: N.Type = Notice::Kind::Sub; break;
	case AlertKind::Tip:
	case AlertKind::Cheer: N.Type = Notice::Kind::Tip; break;
	case AlertKind::Raid: N.Type = Notice::Kind::Raid; break;
	default: N.Type = Notice::Kind::Milestone; break;
	}
	Notices.push_back(N);
}

void Stream::Earn(Channel& Ch, Chips Cents, int Source)
{
	// The creator update (C13, C14): RoomState::Earned books it (tips at once, Affiliate revenue held before the second rent,
	// sponsor pay never in the Kast payout).
	RoomState::Earned(*this, Ch, Cents, Source);
}

void Stream::Subscribe(Channel& Ch, const Inputs& In, const std::string& Who, bool Gift, int Count)
{
	const double Share = Ch.Partner ? 0.7 : 0.5;
	// A gift lands on people watching who aren't subscribed yet, the community first, in the order they came (keyed: the Subs
	// list is the channel's, so no display draw may pick it, §2.12); beyond them, names from the pool by a keyed draw.
	std::vector<std::string> Givees;
	if (Gift && Room && Room->RoomLive)
	{
		for (const RoomState::Visit& V : Room->Visits)
		{
			const std::string& N = Ch.Members[static_cast<size_t>(V.Member)].Name;
			if (static_cast<int>(Givees.size()) < Count && N != Who && RoomState::HereAt(V, Room->Clock) && !Ch.IsSub(N, In.World))
			{
				Givees.push_back(N);
			}
		}
	}
	for (int K = 0; K < Count; ++K)
	{
		std::string To = Who;
		if (Gift)
		{
			if (static_cast<size_t>(K) < Givees.size())
			{
				To = Givees[static_cast<size_t>(K)];
			}
			else if (!Pool.empty())
			{
				const double U = growth::Unit(Room ? Room->SeedK : 0, static_cast<uint64_t>(std::max(0, Ch.Streams)), growth::Tag::Gift, growth::NameKey(Who),
					static_cast<uint64_t>(std::max(0.0, In.World)) * 8u + static_cast<uint64_t>(K));
				To = Pool[std::min(Pool.size() - 1, static_cast<size_t>(std::floor(U * static_cast<double>(Pool.size()))))];
			}
		}
		bool Found = false;
		for (Subscriber& Old : Ch.Subs)
		{
			if (Old.Name == To)
			{
				Old.Renews = std::max(std::fabs(Old.Renews), In.World) + 30.0 * kast_detail::KastDay;
				++Old.Months;
				Found = true;
				break;
			}
		}
		if (!Found)
		{
			Subscriber S;
			S.Name = To;
			S.Since = In.World;
			S.Renews = In.World + 30.0 * kast_detail::KastDay;
			S.Gift = Gift;
			Ch.Subs.push_back(S);
		}
		Earn(Ch, static_cast<Chips>(std::round(static_cast<double>(SubPriceCents) * Share)), 0);
	}
	if (Member* M = Ch.FindMember(Who))
	{
		M->Given += SubPriceCents * Count;
		const double NewL = std::min(1.0, M->Loyalty + 0.04); // money in, heart in
		if (Room && Room->RoomLive)
		{
			Room->LoyaltyWrite(*this, Ch, *M, NewL); // the Room's sums over loyalties and lazy giving follow it (C-K's M2)
		}
		else
		{
			M->Loyalty = NewL;
		}
	}
	Tonight.Subs += Gift ? 0 : Count;
	Tonight.Gifted += Gift ? Count : 0;
	Ch.GiftedSubs += Gift ? Count : 0;
	Alert A;
	A.Kind = Gift ? AlertKind::Gift : AlertKind::Sub;
	A.Who = Who;
	A.Count = Count;
	if (!Gift)
	{
		static const std::vector<std::string> Msgs = {"", "love the stream", "rent fund", "finally subbed", "for the grind", "kastLove", "", "every night i'm here anyway"};
		A.Text = R.Pick(Msgs);
	}
	Queue(A);
	// The line goes up from them when the room's step is done (R1: only if they're still here then).
	RoomState::Line L;
	L.Who = Who;
	L.Text = Gift ? "gifted " + std::to_string(Count) + " sub" + (Count == 1 ? "" : "s") + " to the community!" : "just subscribed!" + (A.Text.empty() ? std::string() : " " + A.Text);
	L.Kind = LineKind::Sub;
	L.Badges = BadgeSub | (Gift ? BadgeGifter : 0);
	if (Room)
	{
		Room->DueLines.push_back(L);
	}
	Hype = std::min(100.0, Hype + (Gift ? 3.0 + static_cast<double>(Count) * 0.6 : 1.5));
}

void Stream::React(Channel& Ch, const Inputs& In, Moment M, int Count)
{
	const std::string Prefix2 = Prefix;
	for (int K = 0; K < Count; ++K)
	{
		int Badges = 0;
		uint32_t Color = 0;
		const std::string Who = Chatter(Ch, In, Badges, Color);
		if (Who.empty())
		{
			break; // nobody named here to say it (R1)
		}
		std::string Line = R.Pick(M == Moment::Top3 ? copy::SweatChat() : kast_detail::Lines(M));
		Line = kast_detail::Fill(Line, "{hero}", In.Hero);
		Line = kast_detail::Fill(Line, "{event}", In.EventName);
		Line = kast_detail::Fill(Line, "{ship}", Ch.Affiliate && (Badges & BadgeSub) ? Prefix2 + "Ship" : "kastHype");
		Line = kast_detail::Fill(Line, "{tilt}", Ch.Affiliate && (Badges & BadgeSub) ? Prefix2 + "Tilt" : "kastSalt");
		Line = kast_detail::Fill(Line, "{rent}", Ch.Affiliate && (Badges & BadgeSub) ? Prefix2 + "Rent" : "kastChip");
		Line = kast_detail::Fill(Line, "{pct}", std::to_string(60 + R.Int(32)));
		Line = kast_detail::Fill(Line, "{who}", "him");
		Line = kast_detail::Fill(Line, "{prize}", "$$$");
		Say(In, Who, Line, LineKind::Chat, Badges, Color);
		// The burst lands over the next few seconds, not all at once.
		Chat.back().At = In.Real + static_cast<double>(K) * R.Range(0.15, 0.7);
	}
}

void Stream::Celebrate(Channel& Ch, const Inputs& In, const std::string& Line)
{
	// A badge earned live (next_segment_spec §5.6): one chatter watching says the reaction, once. Nobody watching, no line
	// (KastWatched: the count is at least one, and the speaker is someone here).
	if (!Live || Line.empty() || !Room || Room->Watchers < 1)
	{
		return;
	}
	LastReal = In.Real;
	int Badges = 0;
	uint32_t Color = 0;
	const std::string Who = Chatter(Ch, In, Badges, Color);
	if (Who.empty())
	{
		return;
	}
	Say(In, Who, Line, LineKind::Chat, Badges, Color);
	Hype = std::min(100.0, Hype + 6.0);
}

void Stream::MakeClip(Channel& Ch, const Inputs& In, Moment M, const std::string& Detail, double Size)
{
	// Somebody has to be watching to clip it (viewers clip big moments, 2 or more watching). The creator update (§2.7.2): the
	// first two of a world day go into the Clip Lane, Kast's feed, at (30 + 4V) e^N (1 + h)(1 + hit / 50), and one in fifty of a
	// win, a bad beat or an armed final table (1.2% of the rest) tops the feed at x40 with Heat +12; the others stay on the
	// channel, unlisted. Every draw is keyed on (minute, kind, table), so the clip is the same at every pace.
	if (!Room || !Room->RoomLive)
	{
		return;
	}
	RoomState& Rm = *Room;
	const kast_detail::Hit H = kast_detail::HitOf(M);
	const double V = static_cast<double>(Rm.Watchers);
	const uint64_t Mi = static_cast<uint64_t>(std::max(0.0, std::floor(Rm.Clock)));
	const uint64_t K = static_cast<uint64_t>(M) * 8u + kast_room_detail::RoomTable(In);
	const double Chance = std::min(0.95, H.ClipChance * std::min(1.0, 0.1 + V / 40.0));
	if (V < 1.5 || growth::Unit(Rm.SeedK, Rm.StreamK, growth::Tag::Clip, Mi, K) >= Chance)
	{
		return;
	}
	Rng Pick(std::to_string(Rm.SeedK) + ":kast:clip:" + std::to_string(Rm.StreamK) + ":" + std::to_string(Mi) + ":" + std::to_string(K));
	// The clipper: someone here (the channel keeps their name), picked by the same keyed draw.
	std::vector<std::string> Clippers;
	for (const RoomState::Visit& Vs : Rm.Visits)
	{
		if (RoomState::HereAt(Vs, Rm.Clock))
		{
			Clippers.push_back(Ch.Members[static_cast<size_t>(Vs.Member)].Name);
		}
	}
	for (const RoomState::Guest& G : Rm.Guests)
	{
		if (G.Kind == 0 && G.Pool >= 0 && Rm.GuestHere(G, Rm.Clock))
		{
			Clippers.push_back(Pool[static_cast<size_t>(G.Pool)]);
		}
	}
	if (Clippers.empty())
	{
		return;
	}
	Clip C;
	C.Kind = static_cast<int>(M);
	C.At = In.World;
	C.Title = kast_detail::ClipTitle(M, In.Hero, Detail, Pick);
	C.By = Clippers[static_cast<size_t>(Pick.Int(static_cast<int>(Clippers.size())))];
	const int Day = kast_detail::KastDayOf(In.World);
	const bool Lane1 = !growth::Granted(Ch.GrantDay, Ch.GrantMask, Day, growth::GrantClip1);
	const bool Lane2 = !growth::Granted(Ch.GrantDay, Ch.GrantMask, Day, growth::GrantClip2);
	bool Viral = false;
	if (Lane1 || Lane2)
	{
		growth::UseGrant(Ch.GrantDay, Ch.GrantMask, Day, Lane1 ? growth::GrantClip1 : growth::GrantClip2);
		C.Lane = true; // in Kast's feed (Top clips' flag, C-K's L11)
		const double Hn = growth::HeatH(kast_room_detail::RoomShownHeat(Ch, In.World));
		C.Reach = growth::ClipReach(V, growth::NormalAt(growth::Unit(Rm.SeedK, Rm.StreamK, growth::Tag::ClipReach, Mi, static_cast<uint64_t>(M))), Hn, H.Hype) * std::max(0.5, std::min(2.0, Size));
		const bool BigKind = (M == Moment::Win || M == Moment::BadBeat || M == Moment::FinalTable) && growth::Armed(In.DeepEntrants > 0 ? In.DeepEntrants : In.Entrants);
		Viral = growth::Unit(Rm.SeedK, Rm.StreamK, growth::Tag::ClipReach, Mi, 64 + static_cast<uint64_t>(M)) < (BigKind ? kast_room_detail::RoomClipTopBig : kast_room_detail::RoomClipTop);
		if (Viral)
		{
			C.Reach *= kast_room_detail::RoomClipTopX;
			kast_room_detail::RoomAddHeat(Ch, In.World, kast_room_detail::RoomHeatClip);
			Rm.WhyClipDay = Day;
			++Rm.WhyEpoch;
		}
		Tonight.BestClipLane = true;
	}
	else
	{
		C.Reach = (5.0 + 3.0 * V) * std::exp(0.8 * growth::NormalAt(growth::Unit(Rm.SeedK, Rm.StreamK, growth::Tag::ClipReach, Mi, 128 + static_cast<uint64_t>(M))));
	}
	Ch.Clips.insert(Ch.Clips.begin(), C);
	if (Ch.Clips.size() > 16)
	{
		std::stable_sort(Ch.Clips.begin() + 1, Ch.Clips.end(), [](const Clip& A, const Clip& B) { return A.Reach > B.Reach; });
		Ch.Clips.pop_back();
	}
	++Tonight.Clips;
	if (Tonight.BestClip.empty() || Viral)
	{
		Tonight.BestClip = C.Title;
	}
	Say(In, "", C.By + " clipped it: \"" + C.Title + "\"", LineKind::System, 0, 0x9b5cff);
	if (Viral)
	{
		Alert A;
		A.Kind = AlertKind::Clip;
		A.Who = C.By;
		A.Text = C.Title;
		Queue(A);
		Say(In, "", copy::ClipTopOfFeed(), LineKind::System, 0, Lime);
	}
}

void Stream::CheckGrowth(Channel& Ch, const Inputs& In)
{
	// The creator update (§5.3 K17): the follower steps from 5 (the first one is First Follower's own line); each first-time step
	// warms Heat +2 (§2.4). K17's steps (5, 10, 25, 50, 100, 1,000, 10,000), Affiliate and Partner play KastShine with their live
	// alert (the confetti's), through the cue queue in place of the old Win (the C-K gate: one sound a milestone, not a Win live
	// and a KastShine at the end card); the glue stings the first stranger's tip and the first raid in at Stop.
	auto Shine = [this](size_t Before) {
		if (Notices.size() > Before)
		{
			Notices.back().Sound = static_cast<int>(SoundId::KastShine);
		}
	};
	static const int Steps[] = {5, 10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000, 25000, 50000, 100000, 250000, 500000, 1000000};
	for (const int Step : Steps)
	{
		if (Ch.Followers >= Step && Ch.Milestone < Step)
		{
			Ch.Milestone = Step;
			Alert A;
			A.Kind = AlertKind::Milestone;
			A.Count = Step;
			A.Text = Grouped(Step) + " followers!";
			const size_t Before = Notices.size();
			Queue(A);
			if (Step <= 100 || Step == 1000 || Step == 10000)
			{
				Shine(Before);
			}
			Hype = std::min(100.0, Hype + 6.0);
			if (Step >= 10)
			{
				kast_room_detail::RoomAddHeat(Ch, In.World, kast_room_detail::RoomHeatMilestone);
				if (Room)
				{
					Room->WhyMilestone = Step; // Heat's why line: "250 followers" (C-K's L5)
					Room->WhyMilestoneDay = kast_detail::KastDayOf(In.World);
					++Room->WhyEpoch;
				}
			}
			if (Live)
			{
				Say(In, "", "Milestone: " + Grouped(Step) + " followers!", LineKind::System, 0, Lime);
			}
		}
	}
	// Kast's rules look at the last 30 days (tonight included). Affiliate is the creator update's bar (C8: 25 followers, 240
	// minutes, 4 days, average 3); Partner's average leaves raiders and the Broadcast out (the real rule).
	double Minutes = 0.0;
	int LiveDays = 0;
	double Avg = 0.0;
	Ch.Window30(In.World, Minutes, LiveDays, Avg);
	double AvgOwn = kast_detail::KastAvgOwn30(Ch, In.World);
	const double Up = Live ? In.World - StartWorld : 0.0;
	if (Up > 1.0)
	{
		const double OwnMinutes = Room ? Room->VMOwn : ViewerMinutes;
		Avg = (Avg * Minutes + ViewerMinutes) / (Minutes + Up);
		AvgOwn = (AvgOwn * Minutes + OwnMinutes) / (Minutes + Up);
		Minutes += Up;
		LiveDays += std::find(Ch.DaysLive.begin(), Ch.DaysLive.end(), kast_detail::KastDayOf(In.World)) == Ch.DaysLive.end() ? 1 : 0;
	}
	if (!Ch.Affiliate && Ch.Followers >= AffiliateFollowersC8 && Minutes >= AffiliateMinutesC8 && LiveDays >= AffiliateDaysC8 && Avg >= AffiliateAvg)
	{
		Ch.Affiliate = true;
		// Dee and Mei subscribe the night it lands (§2.10.3): the first subs are your friends'.
		if (Room && Room->RoomLive)
		{
			Room->FriendsSubPending = true;
		}
		Alert A;
		A.Kind = AlertKind::Affiliate;
		A.Text = "Subs, cheers and ads are on";
		const size_t Before = Notices.size();
		Queue(A);
		Shine(Before);
		Notice N;
		N.Type = Notice::Kind::Text;
		N.From = "Kast";
		N.Body = "Congratulations, you're a Kast Affiliate! Subscriptions, cheers and ad breaks are now on for your channel.";
		Notices.push_back(N);
		if (Live)
		{
			Say(In, "", "This channel is now a Kast Affiliate. Subscribe to support " + In.Hero + "!", LineKind::System, 0, Violet);
		}
	}
	if (Ch.Affiliate && !Ch.Partner && Minutes >= PartnerMinutes && LiveDays >= PartnerDays && AvgOwn >= PartnerAvg)
	{
		Ch.Partner = true;
		Alert A;
		A.Kind = AlertKind::Partner;
		A.Text = "Verified \xC2\xB7 70% of every sub";
		const size_t Before = Notices.size();
		Queue(A);
		Shine(Before);
		Notice N;
		N.Type = Notice::Kind::Text;
		N.From = "Kast";
		N.Body = "Welcome to the Kast Partner Program. Your channel is verified, your sub share is now 70%, and ads pay more.";
		Notices.push_back(N);
	}
	// Community stages, one at a time (the community is counted only when the next stage's other terms already pass).
	const int Reached = kast_detail::StagePastReachable(Ch, In.World, Up, ViewerMinutes) ? kast_detail::StageReached(Ch, In.World, Up, ViewerMinutes) : Ch.Stage;
	while (Ch.Stage < Reached)
	{
		++Ch.Stage;
		const StageSpec& S = Stages()[static_cast<size_t>(Ch.Stage)];
		Ch.Xp += 200.0;
		// Affiliate and Partner announce themselves.
		if (Ch.Stage > 0 && S.Streams > 0)
		{
			Alert A;
			A.Kind = AlertKind::Stage;
			A.Text = S.Name;
			Queue(A);
			Notice N;
			N.Type = Notice::Kind::Text;
			N.From = "Kast";
			N.Body = "Your channel reached a new stage: " + S.Name + ". Unlocked: " + S.Unlock + ".";
			Notices.push_back(N);
			if (Live)
			{
				Say(In, "", "New stage: " + S.Name + " \xC2\xB7 " + S.Unlock, LineKind::System, 0, Lime);
			}
		}
	}
	// Weekly goals pay out once each (WeekGoals' table, read without building it).
	for (size_t I = 0; I < kast_detail::WeekGoalCount; ++I)
	{
		const kast_detail::WeekGoalSpec& G = kast_detail::WeekGoalSpecs[I];
		if (!((Ch.WeekRewarded >> I) & 1) && kast_detail::WeekGoalHave(Ch, In.World, I) >= G.Need)
		{
			Ch.WeekRewarded |= 1 << I;
			Ch.Xp += static_cast<double>(G.Xp);
			if (Live)
			{
				Say(In, "", "Weekly goal done: " + std::string(G.Label) + " (+" + std::to_string(G.Xp) + " XP)", LineKind::System, 0, Lime);
			}
		}
	}
	// (Sponsors find the channel through the sponsor book's weekly looks, and their offers come by Mail: §3.7, §3.9.)
}

void Stream::Start(Channel& Ch, const Inputs& In)
{
	if (Live || !In.Gear.CanStream())
	{
		return;
	}
	Prefix = EmotePrefix(In.Hero);
	// The channel turns over to now first (§8.4: keyed on the world seed, the pending hold before the second rent; R never drawn).
	Offline(Ch, In.World, R, In.Seed, In.RentsPaid);
	kast_detail::RollWeek(Ch, In.World);
	// The first night: two friends from the laundromat are the whole audience.
	if (Ch.Streams == 0 && Ch.Members.empty())
	{
		for (const std::pair<const char*, std::pair<double, double>>& F : {std::make_pair("dee_spincycle", std::make_pair(0.85, 0.95)), std::make_pair("mei_ng", std::make_pair(0.7, 0.85))})
		{
			Member M;
			M.Name = F.first;
			M.Loyalty = F.second.first;
			M.Affinity = F.second.second;
			M.FirstSeen = In.World;
			M.LastSeen = In.World;
			M.Follower = true;
			M.Friend = true;
			Ch.Members.push_back(M);
		}
		Ch.Followers += 2;
	}
	// A moderator is a member of the community (F3): an old save's mod who isn't one becomes one, so they're counted when here.
	for (const Moderator& Mod : Ch.Mods)
	{
		if (!Ch.FindMember(Mod.Name) && static_cast<int>(Ch.Members.size()) < MaxMembers)
		{
			Member M;
			M.Name = Mod.Name;
			M.Loyalty = std::max(0.2, Mod.Online - 0.4);
			M.Affinity = 0.6;
			M.FirstSeen = Mod.Since;
			M.LastSeen = Mod.Since;
			M.Follower = true;
			Ch.Members.push_back(M);
		}
	}
	Live = true;
	StartWorld = In.World;
	StartReal = In.Real;
	Viewers = 0.0;
	Peak = 0;
	Hype = 10.0;
	Health = 1.0;
	Engage = 0.3;
	Chat.clear();
	Alerts.clear();
	Feed.clear();
	Graph.clear();
	LastReal = In.Real;
	Tonight = Summary();
	Moments.clear(); // tonight's moment log starts with tonight
	Pred = Prediction();
	Face = Mood::Happy;
	FaceAt = In.Real;
	AdUntil = -1.0;
	LastAdWorld = In.World;
	ThankAt = -100.0;
	RaidViewers = 0.0;
	LastRaider.clear();
	LastMomentReal = -100.0;
	Seen.clear();
	TimedOut.clear();
	Caught = 0;
	Missed = 0;
	ChatDebt = TrollDebt = TipDebt = CheerDebt = SubDebt = QuestionDebt = GraphDebt = ViewerMinutes = RaidDebt = SponsorDebt = PredDebt = 0.0;
	FollowDebt = ArriveDebt = WordDebt = GiftDebt = 0.0;
	GreetDebt = 3.0;
	Strangers = 0.0;
	Lurkers = 0.0;
	Present = 0;
	RegularsHere = 0;
	Visits.clear();
	Here.clear();
	OnlineMods.clear();
	Scheduled = Ch.OnSchedule(In.World);
	Tonight.OnSchedule = Scheduled;
	++Ch.Streams;
	const int Today = kast_detail::KastDayOf(In.World);
	if (std::find(Ch.DaysLive.begin(), Ch.DaysLive.end(), Today) == Ch.DaysLive.end())
	{
		Ch.DaysLive.push_back(Today);
		if (Ch.DaysLive.size() > 60)
		{
			Ch.DaysLive.erase(Ch.DaysLive.begin());
		}
	}
	// The Room (SPEC_CREATORS §1-§2): the Ping, who comes and when, tonight's terms and the forecast; then the grid's minute 0.
	RoomState::Begin(*this, Ch, In);
	const RoomState& Rm = *Room;
	Say(In, "", (Rm.PingSent ? kast_room_copy::RoomCopyLive(Rm.PingReached, Rm.PingOf, Rm.LastNightFollows) : kast_room_copy::RoomCopyLiveAgain()) +
		(Scheduled ? kast_room_copy::RoomCopyOnSchedule() : std::string()), LineKind::System, 0, Violet);
	if (Rm.FrontAt >= 0.0)
	{
		Say(In, "", copy::FrontRow(), LineKind::System, 0, Lime);
	}
	if (Ch.Streams == 1)
	{
		Notice N;
		N.Type = Notice::Kind::Text;
		N.From = "Kast";
		N.Body = "Your first stream is live! Growing a channel takes time: stream on a schedule, talk to chat, answer questions. Regulars are built one stream at a time.";
		Notices.push_back(N);
	}
	RoomState::Advance(*this, Ch, In);
	RoomState::FillView(*this, Ch, In);
}

Summary Stream::Stop(Channel& Ch, const Inputs& In, const std::string& Raid)
{
	if (!Live)
	{
		return Summary();
	}
	// The room comes up to the end first (the grid runs every minute to here), then the Room closes the night (KastRoom.h).
	RoomState::Advance(*this, Ch, In);
	Summary S = Tonight;
	S.Valid = true;
	S.Trolls = Caught + Missed;
	RoomState::Close(*this, Ch, In, S, Raid);
	{
		Inputs At = In;
		At.World = StartWorld + Room->Clock;
		CheckGrowth(Ch, At);
	}
	Live = false;
	AdUntil = -1.0;
	Alerts.clear();
	Visits.clear();
	Here.clear();
	Present = 0;
	RegularsHere = 0;
	Strangers = 0.0;
	Lurkers = 0.0;
	Last = S;
	return S;
}

void Stream::OnMoment(Channel& Ch, const Inputs& In, Moment M, const std::string& Detail, double Size)
{
	if (!Live || !Room || !Room->RoomLive)
	{
		return;
	}
	// The creator update: the room is brought to this world minute first (F5c; the session's Recount did it, a test may not),
	// and ss.Kast.Moment's held run stands in for the inputs' deepest run.
	Inputs Held = In;
	if (Room->Held)
	{
		Held.Prestige = Room->HeldP;
		Held.DeepEntrants = Room->HeldEntrants;
		Held.DeepRemaining = kast_room_detail::RoomHeldRemaining(Room->HeldStage, Room->HeldEntrants);
		Held.DeepTableSize = 9;
		Held.Depth = growth::Depth(Held.DeepEntrants, Held.DeepRemaining);
		Held.DeepTable = 0;
		Held.ThisTable = Held.ThisTable < 0 ? 0 : Held.ThisTable;
		Held.Entrants = Held.DeepEntrants;
	}
	RoomState::Advance(*this, Ch, Held);
	LastReal = Held.Real;
	const int FirstLine = NextId;        // the moment's own chat lines start here (tonight's moment log, below)
	const int ClipsBefore = Tonight.Clips; // and a clip, if somebody makes one
	const kast_detail::Hit H = kast_detail::HitOf(M);
	// A small field's stages are poker moments, not deep runs: half the hype (§2.6's field minimum).
	const bool SmallStage = kast_room_detail::RoomStageOf(M) != growth::RunStage::None && !growth::Armed(Held.DeepEntrants > 0 ? Held.DeepEntrants : Held.Entrants) &&
		(Held.Entrants > 0 || Held.DeepEntrants > 0);
	Hype = std::min(100.0, Hype + H.Hype * (SmallStage ? 0.5 : 1.0) * kast_detail::TitleOf(Ch).Hype * std::max(0.5, std::min(2.0, Size)) * (1.0 + Held.Gear.Hype));
	LastMoment = M;
	LastMomentReal = Held.Real;
	if (H.Face != Mood::Focus || M == Moment::BestPlay)
	{
		Face = H.Face;
		FaceAt = Held.Real;
	}
	// A wave in chat, as big as the room; an empty room says nothing (R1: whole people, and only the ones here speak).
	const double Watching = static_cast<double>(Room->Watchers);
	const int Lines = Room->Watchers >= 1 ? std::min(14, std::min(H.Lines, 1 + Room->Watchers) + static_cast<int>(std::sqrt(Watching) / 3.0)) : 0;
	const int LinesBefore = NextId;
	React(Ch, Held, M, Lines);
	if (M == Moment::Knockout && !Detail.empty() && NextId > LinesBefore)
	{
		Chat.back().Text = "bye " + Detail + " kastGG";
	}
	// The deep-run machinery (§2.3.5, §2.6, §2.7): Heat's Run, the roll call, Bonded, a Run Alert, the Broadcast, a moment raid.
	RoomState::MomentStep(*this, Ch, Held, M);
	MakeClip(Ch, Held, M, Detail, Size);
	// Tonight's moment log (update 2 §4.4): what Rerun keeps of the stream around a hand. The viewers, the hype it left,
	// the face, the first few things chat said about it (the reaction, not the system's notices or a troll), and the
	// clip's time when somebody clipped it: clips have no id, so a candidate links its clip by At. It reads the stream
	// and draws nothing, so the stream goes exactly as it did without it. The session (SessionVideo.cpp) links each note
	// to the hand that fired it.
	{
		MomentNote Note;
		Note.Kind = M;
		Note.World = In.World;
		Note.Real = In.Real;
		Note.Viewers = Room->Watchers;
		Note.HypeAfter = Hype;
		Note.Face = Face;
		for (const ChatMsg& Line : Chat)
		{
			if (Line.Id >= FirstLine && Line.Kind == LineKind::Chat && !Line.Toxic && !Line.Spam && Note.Chat.size() < kast_detail::KastMomentLines)
			{
				Note.Chat.push_back(Line.Text);
			}
		}
		Note.ClipAt = Tonight.Clips > ClipsBefore && !Ch.Clips.empty() && Ch.Clips.front().At == In.World ? In.World : -1.0;
		Moments.push_back(Note);
		if (Moments.size() > kast_detail::KastMomentNotes)
		{
			Moments.erase(Moments.begin(), Moments.begin() + static_cast<std::ptrdiff_t>(Moments.size() - kast_detail::KastMomentNotes));
		}
	}
	// The big moments: a superfan here gifts 1-3 subs (5%, keyed on minute and kind, §2.12) at a deep run's ITM, final table or
	// win (100+ entrants), as synth_kast.py has it (S-K, 2026-10-24: today's won all-ins too, on every table, paid the regular x1.7
	// the model's subs). The network's moment raids are the Room's (MomentStep).
	const bool Big = (M == Moment::InTheMoney || M == Moment::FinalTable || M == Moment::Win) && growth::Armed(Held.DeepEntrants > 0 ? Held.DeepEntrants : Held.Entrants);
	if (Big && Ch.Affiliate)
	{
		const uint64_t Mi = static_cast<uint64_t>(std::max(0.0, std::floor(Room->Clock)));
		std::string Fan;
		for (const RoomState::Visit& V : Room->Visits)
		{
			if (Fan.empty() && RoomState::HereAt(V, Room->Clock) && Ch.Members[static_cast<size_t>(V.Member)].Loyalty >= SuperfanLoyalty)
			{
				Fan = Ch.Members[static_cast<size_t>(V.Member)].Name;
			}
		}
		if (!Fan.empty() && growth::Unit(Room->SeedK, Room->StreamK, growth::Tag::Gift, Mi, static_cast<uint64_t>(M)) < 0.05)
		{
			const int Most = 3;
			Subscribe(Ch, Held, Fan, true, 1 + std::min(Most - 1, static_cast<int>(std::floor(growth::Unit(Room->SeedK, Room->StreamK, growth::Tag::Gift, Mi, 64 + static_cast<uint64_t>(M)) * Most))));
		}
	}
	if (M == Moment::Register && !Pred.Active)
	{
		Pred = Prediction();
		Pred.Active = true;
		Pred.Question = "Will " + In.Hero + " cash in " + (In.EventName.empty() ? std::string("this one") : In.EventName) + "?";
		Pred.At = In.Real;
		Say(In, "", "Prediction started: " + Pred.Question, LineKind::System, 0, Violet);
	}
	// The chat games (Hold or Crack opens and reveals here), then the lines the moment set off, from whoever is here.
	play::OnMoment(*Play, *Room, Ch, Held, M);
	RoomState::Flush(*this, Held);
	Room->Recount(Ch, Room->Clock);
	Viewers = static_cast<double>(Room->Watchers);
	RaidViewers = static_cast<double>(Room->RaidersHere);
	Room->Shown.Count = Room->Watchers;
}

void Stream::Resolve(Channel& Ch, const Inputs& In, bool Yes)
{
	if (!Live || !Pred.Active || Pred.Resolved)
	{
		return;
	}
	Pred.Resolved = true;
	Pred.Outcome = Yes;
	Pred.ResolvedAt = In.Real;
	const int Total = std::max(1, Pred.Yes + Pred.No);
	Say(In, "", std::string("Prediction: ") + (Yes ? "YES" : "NO") + " wins! " + Grouped(Yes ? Pred.Yes : Pred.No) + " points to " + std::to_string((Yes ? Pred.Yes : Pred.No) * 100 / Total) + "% of chat.",
		LineKind::System, 0, Violet);
	static const std::vector<std::string> Believers = {"I BELIEVED kastHype", "never doubted", "points secured", "believers win kastClap"};
	static const std::vector<std::string> Doubters = {"doubters win kastLUL", "free points", "sorry {hero}", "I knew it kastLUL"};
	// The ones who bet say so, if anybody's watching: people here (R1).
	const int Watching = Room ? Room->Watchers : 0;
	for (int K = 0; K < (Watching >= 1 ? std::min(3, 1 + Watching) : 0); ++K)
	{
		int Badges = 0;
		uint32_t Color = 0;
		const std::string Who = Chatter(Ch, In, Badges, Color);
		if (Who.empty())
		{
			break;
		}
		Say(In, Who, kast_detail::Fill(R.Pick(Yes ? Believers : Doubters), "{hero}", In.Hero), LineKind::Chat, Badges, Color);
		Chat.back().At = In.Real + 0.8 + static_cast<double>(K) * 0.5;
	}
}

bool Stream::RunAd(Channel& Ch, const Inputs& In, int Seconds)
{
	if (!Live || !Ch.Affiliate || AdRunning(In.Real) || AdCooldown(In.World) > 0.0)
	{
		return false;
	}
	Seconds = Seconds >= 180 ? 180 : Seconds >= 90 ? 90 : 60;
	AdUntil = In.Real + static_cast<double>(Seconds);
	AdSeconds = Seconds;
	LastAdWorld = In.World;
	if (Room && Room->RoomLive)
	{
		// The ad's room effect is a world window (§2.5.1), the screen's ad plays on in real seconds.
		Room->AdUntilUp = std::max(0.0, In.World - StartWorld) + kast_room_detail::RoomAdWindow * static_cast<double>(Seconds) / 90.0;
		Room->LastAdMinute = static_cast<int>(std::floor(std::max(0.0, In.World - StartWorld)));
	}
	const double Slots = static_cast<double>(Seconds) / 30.0;
	const double PerSlot = Ch.Partner ? 0.45 : 0.32; // cents per viewer per 30 seconds
	Earn(Ch, static_cast<Chips>(std::round(static_cast<double>(Room ? Room->Watchers : 0) * Slots * PerSlot)), 3);
	Health = std::max(0.0, Health - (Seconds >= 180 ? 0.06 : Seconds >= 90 ? 0.03 : 0.015));
	Say(In, "", "Running a " + std::to_string(Seconds) + "-second ad break.", LineKind::System, 0, 0x94a3b8);
	return true;
}

Chips Stream::SponsorRead(Channel& Ch, const Inputs& In, const std::string& Id)
{
	// The deals are the sponsor book's (Inputs::Deals, its view): Kast works on its own copy, which the session hands back.
	// The creator update (C13): a read pays once a deal a world day, after 30 world minutes live with someone watching, within
	// 1.5x the deal's reads ask a contract period; the pay is the sponsor's own ledger line (Summary::SponsorPaid), never part
	// of the Kast payout.
	kast_room_detail::RoomTakeDeals(Deals, In.Deals);
	if (!Live || !Room || !Room->RoomLive)
	{
		return 0;
	}
	RoomState::Advance(*this, Ch, In);
	const int Today = kast_detail::KastDayOf(In.World);
	for (Deal& D : Deals)
	{
		if (D.Id == Id && D.Until > In.World && D.ReadCents > 0)
		{
			const Sponsor* S = FindSponsor(Id);
			if (!S || !growth::SponsorReadPays(Today, D.ReadAt, std::max(0.0, In.World - StartWorld), Room->Watchers, D.ReadsPaid, D.ReadsAsk))
			{
				return 0;
			}
			D.ReadAt = In.World;
			D.EarnedCents += D.ReadCents;
			++D.ReadsPaid;
			Earn(Ch, D.ReadCents, 4);
			auto It = std::find_if(Room->SponsorPaid.begin(), Room->SponsorPaid.end(), [&](const Summary::SponsorPay& P) { return P.Id == D.Id; });
			if (It == Room->SponsorPaid.end())
			{
				Summary::SponsorPay P;
				P.Id = D.Id;
				Room->SponsorPaid.push_back(P);
				It = Room->SponsorPaid.end() - 1;
			}
			++It->Reads;
			It->ReadCents += D.ReadCents;
			Say(In, In.Hero, S->ReadLine, LineKind::Streamer, BadgeStreamer | (Ch.Partner ? BadgeVerified : 0), 0xc6f432);
			static const std::vector<std::string> Reacts = {"ad read kastLUL", "sellout kastLUL", "get that bag", "rent money", "{brand}!!", "the read was smooth"};
			// Chat reacts if anybody's watching: people here (R1).
			for (int K = 0; K < (Room->Watchers >= 1 ? std::min(3, 1 + Room->Watchers / 3) : 0); ++K)
			{
				int Badges = 0;
				uint32_t Color = 0;
				const std::string Who = Chatter(Ch, In, Badges, Color);
				if (Who.empty())
				{
					break;
				}
				Say(In, Who, kast_detail::Fill(R.Pick(Reacts), "{brand}", S->Brand), LineKind::Chat, Badges, Color);
				Chat.back().At = In.Real + 0.6 + static_cast<double>(K) * 0.6;
			}
			Health = std::max(0.0, Health - 0.01);
			return D.ReadCents;
		}
	}
	return 0;
}

bool Stream::Thank(Channel& Ch, const Inputs& In)
{
	// Thank chat (§2.9): Engage +0.25 x 0.6^k (k the thanks in the last 10 world minutes), the members here who were talking feel
	// seen (Engaged; no Affinity), 20 real seconds between presses.
	if (!Live || !Room || !Room->RoomLive || In.Real - ThankAt < 20.0)
	{
		return false;
	}
	RoomState& Rm = *Room;
	RoomState::Advance(*this, Ch, In);
	ThankAt = In.Real;
	Say(In, In.Hero, R.Pick(kast_detail::ThankLines()), LineKind::Streamer, BadgeStreamer | (Ch.Partner ? BadgeVerified : 0), 0xc6f432);
	Rm.ThankTimes.erase(std::remove_if(Rm.ThankTimes.begin(), Rm.ThankTimes.end(), [&](double At) { return Rm.Clock - At > 10.0; }), Rm.ThankTimes.end());
	Engage = std::min(1.0, Engage + growth::ThankGain(static_cast<int>(Rm.ThankTimes.size())));
	Rm.ThankTimes.push_back(Rm.Clock);
	Health = std::min(1.0, Health + 0.03);
	for (RoomState::Visit& V : Rm.Visits)
	{
		if (RoomState::HereAt(V, Rm.Clock) && Rm.TalkerTonight(V, Ch.Members[static_cast<size_t>(V.Member)].Loyalty))
		{
			V.Engaged = true;
		}
	}
	static const std::vector<std::string> Back = {"kastLove", "<3", "love you too", "ty for streaming", "kastLove kastLove", "best streamer"};
	for (int K = 0; K < std::min(6, std::min(Rm.MembersHere + Rm.GuestsHere, 2 + static_cast<int>(std::sqrt(static_cast<double>(Rm.Watchers)) / 4.0))); ++K)
	{
		int Badges = 0;
		uint32_t Color = 0;
		const std::string Who = Chatter(Ch, In, Badges, Color);
		if (Who.empty())
		{
			break;
		}
		Say(In, Who, R.Pick(Back), LineKind::Chat, Badges, Color);
		Chat.back().At = In.Real + 0.5 + static_cast<double>(K) * 0.4;
	}
	return true;
}

bool Stream::Answer(Channel& Ch, const Inputs& In, int MsgId)
{
	// Answer a question (§2.9): Engage +0.35; a member who asked is Engaged and Noticed (once ever: Affinity +3% of the gap) with
	// loyalty x1.6 tonight; a guest who asked follows with 0.15 x Sat (keyed on name and minute); the thanks only from an asker
	// who is still here (F5a).
	if (!Live || !Room || !Room->RoomLive)
	{
		return false;
	}
	RoomState::Advance(*this, Ch, In);
	RoomState& Rm = *Room;
	for (ChatMsg& M : Chat)
	{
		if (M.Id == MsgId && M.Kind == LineKind::Question && !M.Answered && !M.Deleted)
		{
			M.Answered = true;
			const std::vector<std::string>& Qs = kast_detail::Questions();
			size_t Index = 0;
			for (size_t I = 0; I < Qs.size(); ++I)
			{
				Index = Qs[I] == M.Text ? I : Index;
			}
			const std::string Who = M.Who;
			Say(In, In.Hero, "@" + Who + " " + kast_detail::Answers()[Index], LineKind::Streamer, BadgeStreamer | (Ch.Partner ? BadgeVerified : 0), 0xc6f432);
			Engage = std::min(1.0, Engage + 0.35);
			Hype = std::min(100.0, Hype + 3.0);
			Health = std::min(1.0, Health + 0.02);
			++Ch.WeekAnswers;
			// The asker's thanks (only from someone still here, F5a): a guest the answer won says so (§5.3 K11, the C-K gate).
			const bool Asking = Rm.Present(Who);
			const auto It = Rm.MemberAt.find(Who);
			if (It != Rm.MemberAt.end())
			{
				Member& Asker = Ch.Members[static_cast<size_t>(It->second)];
				const int Vi = Rm.VisitOf[static_cast<size_t>(It->second)];
				if (Vi >= 0)
				{
					Rm.Visits[static_cast<size_t>(Vi)].Engaged = true;
					Rm.Visits[static_cast<size_t>(Vi)].Noticed = true;
				}
				if (!Asker.Noticed)
				{
					Asker.Noticed = true;
					Asker.Affinity = growth::AffinityBump(Asker.Affinity, 0.03);
				}
			}
			bool Won = false;
			if (It == Rm.MemberAt.end())
			{
				const int Gi = Rm.GuestNamed(Who);
				const uint64_t Minute = static_cast<uint64_t>(std::max(0.0, std::floor(Rm.Clock)));
				if (Gi >= 0 && Rm.Guests[static_cast<size_t>(Gi)].Kind == 0 &&
					growth::Unit(Rm.SeedK, Rm.StreamK, growth::Tag::Answer, growth::NameKey(Who), Minute) < kast_room_detail::RoomAnswerFollow * growth::Sat(Ch.Followers))
				{
					RoomState::FollowGuest(*this, Ch, Gi, Rm.Clock, true);
					Won = true;
				}
			}
			if (Asking)
			{
				static const std::vector<std::string> Thanks = {"thanks!", "ty kastLove", "makes sense", "good answer", "appreciate it"};
				Say(In, Who, R.Pick(Won ? copy::AnswerFollowed() : Thanks), LineKind::Chat, kast_room_detail::RoomBadges(Rm, Ch, Who, In.World), kast_detail::NameColor(Who));
				Chat.back().At = In.Real + 1.2;
			}
			RoomState::Flush(*this, In);
			return true;
		}
	}
	return false;
}

bool Stream::Timeout(Channel& Ch, const Inputs& In, int MsgId)
{
	if (Room && Room->RoomLive)
	{
		RoomState::Advance(*this, Ch, In);
	}
	for (ChatMsg& M : Chat)
	{
		if (M.Id == MsgId && !M.Deleted && M.Kind == LineKind::Chat && !M.Who.empty() && M.Who != In.Hero)
		{
			const std::string Who = M.Who;
			const bool Deserved = M.Toxic || M.Spam;
			for (ChatMsg& Other : Chat)
			{
				if (Other.Who == Who && Other.Kind == LineKind::Chat)
				{
					Other.Deleted = true;
					Other.DeletedBy = In.Hero;
				}
			}
			TimedOut[Who] = In.Real + 600.0;
			Say(In, "", Who + " has been timed out for 10 minutes.", LineKind::System, 0, 0x94a3b8);
			// A timeout ends their visit (F1): a guest leaves, a member leaves for the night.
			if (Room && Room->RoomLive)
			{
				RoomState& Rm = *Room;
				const int Gi = Rm.GuestNamed(Who);
				if (Gi >= 0)
				{
					RoomState::Guest& G = Rm.Guests[static_cast<size_t>(Gi)];
					G.Leave = Rm.Clock;
					if (G.Pool >= 0)
					{
						Rm.PoolUntil[static_cast<size_t>(G.Pool)] = Rm.Clock + kast_room_detail::RoomTimeoutWorld;
					}
				}
				const int Ri = Rm.RaiderNamed(Who);
				if (Ri >= 0)
				{
					RoomState::Raider& X = Rm.Raiders[static_cast<size_t>(Ri)];
					X.Leave = Rm.Clock;
					if (X.Pool >= 0 && static_cast<size_t>(X.Pool) < Rm.PoolUntil.size())
					{
						// Out for the timeout, as a guest would be (no new guest wears the name meanwhile).
						Rm.PoolUntil[static_cast<size_t>(X.Pool)] = std::max(Rm.PoolUntil[static_cast<size_t>(X.Pool)], Rm.Clock + kast_room_detail::RoomTimeoutWorld);
					}
				}
				const int Vi = Rm.VisitNamed(Ch, Who);
				if (Vi >= 0)
				{
					Rm.LeaveNow(Rm.Visits[static_cast<size_t>(Vi)], Rm.Clock);
				}
				Rm.Recount(Ch, Rm.Clock);
				Viewers = static_cast<double>(Rm.Watchers);
			}
			if (Deserved)
			{
				++Caught;
				Health = std::min(1.0, Health + 0.02);
				if (R.Chance(0.6) && Room && Room->Watchers >= 2)
				{
					int Badges = 0;
					uint32_t Color = 0;
					const std::string Fan = Chatter(Ch, In, Badges, Color);
					if (!Fan.empty())
					{
						Say(In, Fan, R.Chance(0.5) ? "get him outta here" : "kastClap", LineKind::Chat, Badges, Color);
						Chat.back().At = In.Real + 0.8;
					}
				}
			}
			else
			{
				// Timing out someone for nothing: they leave hurt, and the room noticed.
				Health = std::max(0.0, Health - 0.06);
				if (Member* Hurt = Ch.FindMember(Who))
				{
					const double NewL = std::max(0.0, Hurt->Loyalty - 0.15);
					if (Room && Room->RoomLive)
					{
						Room->LoyaltyWrite(*this, Ch, *Hurt, NewL);
					}
					else
					{
						Hurt->Loyalty = NewL;
					}
				}
				if (Room && Room->Watchers >= 2)
				{
					int Badges = 0;
					uint32_t Color = 0;
					const std::string Fan = Chatter(Ch, In, Badges, Color);
					if (!Fan.empty())
					{
						Say(In, Fan, "why did " + Who + " get timed out?? kastLUL", LineKind::Chat, Badges, Color);
						Chat.back().At = In.Real + 0.8;
					}
				}
			}
			return true;
		}
	}
	return false;
}

bool Stream::Promote(Channel& Ch, const Inputs& In, const std::string& Name)
{
	if (static_cast<int>(Ch.Mods.size()) >= MaxMods || Ch.IsMod(Name) || Name.empty() || Name == In.Hero)
	{
		return false;
	}
	if (Live && Room && Room->RoomLive)
	{
		RoomState::Advance(*this, Ch, In);
	}
	Moderator M;
	M.Name = Name;
	M.Since = In.World;
	const Member* Mem = Ch.FindMember(Name);
	M.Online = std::min(0.95, 0.4 + 0.5 * (Mem ? Mem->Loyalty : 0.2));
	Ch.Mods.push_back(M);
	if (Member* Mm = Ch.FindMember(Name))
	{
		const double NewL = std::min(1.0, Mm->Loyalty + 0.1); // trusted
		if (Room && Room->RoomLive)
		{
			Room->LoyaltyWrite(*this, Ch, *Mm, NewL);
		}
		else
		{
			Mm->Loyalty = NewL;
		}
	}
	if (Live && Room && Room->RoomLive)
	{
		RoomState& Rm = *Room;
		// A promoted chatter who isn't a member becomes one (F3): a guest here is the same person, now in the community.
		if (Rm.MemberAt.count(Name) == 0)
		{
			const int Gi = Rm.GuestNamed(Name);
			const double Until = Gi >= 0 ? Rm.Guests[static_cast<size_t>(Gi)].Leave : Rm.Clock;
			if (Gi >= 0)
			{
				Rm.Guests[static_cast<size_t>(Gi)].Gone = true;
			}
			const int Mi = RoomState::AddMemberKeyed(*this, Ch, Name, false, 0.3, In.World);
			if (Mi >= 0 && Gi >= 0)
			{
				const int Vi = RoomState::AddVisit(*this, Ch, Mi, Rm.Clock, static_cast<uint8_t>(View::Source::Members));
				if (Vi >= 0)
				{
					RoomState::Visit& V = Rm.Visits[static_cast<size_t>(Vi)];
					Rm.PlanChanging(V);
					V.Flip = Rm.Clock;
					V.Pending = Rm.Clock;
					V.Arrived = true;
					V.FirstIn = Rm.Clock;
					(void)Until;
				}
			}
		}
		const int Vi = Rm.MemberAt.count(Name) ? Rm.VisitOf[static_cast<size_t>(Rm.MemberAt[Name])] : -1;
		if (Vi >= 0)
		{
			Rm.Visits[static_cast<size_t>(Vi)].Mod = true;
			const auto At = std::lower_bound(Rm.ModVisits.begin(), Rm.ModVisits.end(), Vi);
			if (Rm.PresOn && (At == Rm.ModVisits.end() || *At != Vi))
			{
				Rm.ModVisits.insert(At, Vi); // in visit order, as Recount lists the mods here
			}
		}
		Rm.Recount(Ch, Rm.Clock);
		Say(In, "", Name + " is now a moderator.", LineKind::System, 0, 0x22c55e);
		static const std::vector<std::string> Lines2 = {"congrats {who} kastClap", "mod {who} kastHype", "power trip incoming kastLUL", "well deserved"};
		if (Rm.Watchers >= 2)
		{
			int Badges = 0;
			uint32_t Color = 0;
			const std::string Fan = Chatter(Ch, In, Badges, Color);
			if (!Fan.empty())
			{
				Say(In, Fan, kast_detail::Fill(R.Pick(Lines2), "{who}", Name), LineKind::Chat, Badges, Color);
				Chat.back().At = In.Real + 0.7;
			}
		}
		// The new mod thanks chat if they're in the room to see it (a chatter is a viewer).
		if (Rm.Present(Name))
		{
			OnlineMods.push_back(Name);
			Say(In, Name, "thank you!! I'll keep it clean", LineKind::Chat, BadgeMod | (Ch.IsSub(Name, In.World) ? BadgeSub : 0), kast_detail::NameColor(Name));
			Chat.back().At = In.Real + 1.5;
		}
	}
	return true;
}

bool Stream::Demote(Channel& Ch, const std::string& Name)
{
	const auto It = std::find_if(Ch.Mods.begin(), Ch.Mods.end(), [&](const Moderator& M) { return M.Name == Name; });
	if (It == Ch.Mods.end())
	{
		return false;
	}
	Ch.Mods.erase(It);
	OnlineMods.erase(std::remove(OnlineMods.begin(), OnlineMods.end(), Name), OnlineMods.end());
	return true;
}

std::vector<std::string> Stream::ModCandidates(const Channel& Ch, double World) const
{
	// Regulars who talk and behave: the loyal ones first.
	std::vector<std::pair<double, std::string>> Top;
	for (const Member& M : Ch.Members)
	{
		if (M.Loyalty >= RegularLoyalty && M.Messages >= 10 && !M.Friend && !Ch.IsMod(M.Name))
		{
			Top.push_back({M.Loyalty * 100.0 + static_cast<double>(M.Messages) * 0.2 + (Ch.IsSub(M.Name, World) ? 10.0 : 0.0), M.Name});
		}
	}
	std::sort(Top.begin(), Top.end(), [](const std::pair<double, std::string>& A, const std::pair<double, std::string>& B) { return A.first != B.first ? A.first > B.first : A.second < B.second; });
	std::vector<std::string> Out;
	for (size_t I = 0; I < Top.size() && I < 5; ++I)
	{
		Out.push_back(Top[I].second);
	}
	return Out;
}

void Stream::Tick(Channel& Ch, const Inputs& In, double DWorld, double DReal)
{
	if (!Live)
	{
		return;
	}
	(void)DWorld; // the room runs on the world clock's whole minutes (AdvanceTo), never on a frame's slice of it (R3)
	DReal = std::max(0.0, std::min(1.0, DReal));
	LastReal = In.Real;
	// The creator update (SPEC_CREATORS §1.2): the room on the grid, then display chat, then the View.
	AdvanceTo(Ch, In);
	ChatStep(Ch, In, DReal);

	// Alerts: one at a time.
	if (!Alerts.empty())
	{
		if (Alerts.front().At < 0.0)
		{
			Alerts.front().At = In.Real;
		}
		else if (In.Real - Alerts.front().At > kast_detail::AlertSeconds * (Alerts.size() > 4 ? 0.5 : 1.0))
		{
			Alerts.erase(Alerts.begin());
			if (!Alerts.empty())
			{
				Alerts.front().At = In.Real;
			}
		}
	}

	// Reactions scheduled a moment ahead land in order.
	if (!std::is_sorted(Chat.begin(), Chat.end(), [](const ChatMsg& A, const ChatMsg& B) { return A.At < B.At; }))
	{
		std::stable_sort(Chat.begin(), Chat.end(), [](const ChatMsg& A, const ChatMsg& B) { return A.At < B.At; });
	}

	// The face goes back to the game after a moment.
	if (In.Real - FaceAt > 6.0)
	{
		Face = In.Tilt > 0.55 ? Mood::Tilted : Mood::Focus;
		FaceAt = In.Real;
	}
	RoomState::FillView(*this, Ch, In);
}
} // namespace kast
} // namespace ss
