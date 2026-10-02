#include "ShortStack/Game/Kast.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Handles.h"

#include <algorithm>
#include <cmath>

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
	static const std::vector<std::string> Won = {"SHIP IT kastHype", "LETS GOOO", "kastClap kastClap", "holds!!", "EZ", "{hero} on a heater", "doubled up kastChip", "never in doubt", "{ship} {ship}", "chat we're so back", "HOLDS kastHype"};
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
		"Dee's game sometimes. the Riverside on sundays", "biggest? not yet. ask me after tonight", "walk away. water. come back. mostly I fail", "bankroll management and a lot of folding",
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
	static const std::vector<std::string> V = {"let's go", "for the river gods", "heater incoming", "gl!", "shove", "kastHype", "deep run fuel", "hold this time"};
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
				{"red lights, heater mode", "running hot, the room agrees", "the red LEDs are a vibe"},
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

/** An exponential wait with this mean. */
double KastExp(Rng& R, double Mean)
{
	return -Mean * std::log(std::max(1e-9, 1.0 - R.Next()));
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

/** A new week: the streak carries on if last week had three streams, and the goals start over. */
void RollWeek(Channel& Ch, double World)
{
	const int W = KastWeekOf(World);
	if (Ch.Week == W)
	{
		return;
	}
	if (Ch.Week >= 0)
	{
		Ch.Streak = Ch.WeekStreams >= 3 && W == Ch.Week + 1 ? Ch.Streak + 1 : 0;
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

const char* const MomentNames[] = {"Register", "AllIn", "WonAllIn", "LostAllIn", "BadBeat", "BigPot", "Knockout", "Bubble", "InTheMoney", "FinalTable", "Win", "Bust", "Cashed", "BestPlay", "Blunder", "Rival", "LevelUp"};

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

int ViewersNow(const Streamer& S, double World)
{
	const double Minute = std::fmod(World, kast_detail::KastDay);
	const int Today = static_cast<int>(std::floor(World / kast_detail::KastDay));
	const double Wobble = 1.0 + 0.04 * std::sin(World / 7.0 + static_cast<double>(S.Followers % 97));
	if (S.Hours >= 24)
	{
		return static_cast<int>(std::round(static_cast<double>(S.Viewers) * Wobble * (0.75 + 0.25 * kast_detail::TimeOfDay(World))));
	}
	// The stream that started today, or the one from yesterday still running past midnight.
	for (int Back = 0; Back <= 1; ++Back)
	{
		const int D = Today - Back;
		if (!((S.Days >> (((D % 7) + 7) % 7)) & 1))
		{
			continue;
		}
		const double Since = Minute + static_cast<double>(Back) * kast_detail::KastDay - static_cast<double>(S.Opens);
		const double Length = static_cast<double>(S.Hours) * 60.0;
		if (Since < 0.0 || Since >= Length)
		{
			continue;
		}
		const double Ramp = std::min(1.0, std::min(Since / 25.0, (Length - Since) / 20.0) * 0.7 + 0.3);
		const double Night = 0.85 + 0.3 * static_cast<double>(kast_detail::NameHash(S.Name + std::to_string(D)) % 100) / 100.0;
		return std::max(1, static_cast<int>(std::round(static_cast<double>(S.Viewers) * Ramp * Night * Wobble)));
	}
	return 0;
}

const std::vector<Sponsor>& Sponsors()
{
	static const std::vector<Sponsor> L = [] {
		std::vector<Sponsor> V;
		auto Add = [&](const char* Id, const char* Brand, const char* Product, const char* Pitch, const char* Read, uint32_t Color, int Followers, int Avg, bool Partner, double PerHour, double ReadPay, int Days) {
			Sponsor S;
			S.Id = Id;
			S.Brand = Brand;
			S.Product = Product;
			S.Pitch = Pitch;
			S.ReadLine = Read;
			S.Color = Color;
			S.Followers = Followers;
			S.AvgViewers = Avg;
			S.NeedsPartner = Partner;
			S.PerHourCents = static_cast<Chips>(PerHour * 100.0 + 0.5);
			S.ReadCents = static_cast<Chips>(ReadPay * 100.0 + 0.5);
			S.Days = Days;
			V.push_back(S);
		};
		Add("overclock", "Overclock", "Overclock Energy, zero sugar",
			"Hey! We love the late-night grind energy. $3 for every hour you're live with a can on the desk, plus $10 a read. 30 days.",
			"This hand is brought to you by Overclock Energy. Zero sugar, all-night focus. Code GRIND in the panels.", 0x84cc16, 300, 15, false, 3.0, 10.0, 30);
		Add("tunnelrat", "TunnelRat VPN", "TunnelRat VPN",
			"Your chat trusts you. $6 an hour live and $25 a read to tell them about TunnelRat. 30 days.",
			"Quick one: TunnelRat VPN keeps your connection private wherever you play. Link below, first month free.", 0x38bdf8, 1000, 30, false, 6.0, 25.0, 30);
		Add("stacked", "Stacked Apparel", "Stacked hoodies and caps",
			"We'd love to see you grind in Stacked. $10 an hour live, $40 a read, and a box of merch. 30 days.",
			"Hoodie's from Stacked Apparel. Built for twelve-hour sessions. Code in the panels for 20% off.", 0xf59e0b, 2500, 60, false, 10.0, 40.0, 30);
		Add("riverline", "RiverLine", "Team RiverLine streamer contract",
			"You've built something. RiverLine would like you on Team RiverLine: $25 an hour streaming RiverLine, $60 a read, and the Team patch on your avatar. 30 days.",
			"Everything you see tonight is on RiverLine. New players: the welcome freeroll is in my panels. Team RiverLine!", 0x27d3c3, 2500, 75, true, 25.0, 60.0, 30);
		Add("sitwell", "Sitwell", "Sitwell Pro gaming chair",
			"Ten thousand followers sit with you every night. Sitwell would like to be the chair. $35 an hour, $90 a read. 30 days.",
			"Twelve hours in and my back is fine. That's the Sitwell Pro. Link's below.", 0xfb923c, 10000, 300, true, 35.0, 90.0, 30);
		return V;
	}();
	return L;
}

const Sponsor* FindSponsor(const std::string& Id)
{
	for (const Sponsor& S : Sponsors())
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
		{"Affiliate", "Kast's first tier, over the last 30 days: 50 followers, 500 minutes and 7 days live, 3 average viewers.", "Subs, cheers, ad breaks and channel emotes", 0, 0, 0, AffiliateFollowers, 1},
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
		Out.push_back({"Followers", static_cast<double>(Ch.Followers), static_cast<double>(AffiliateFollowers), 0});
		Out.push_back({"Minutes live (30 days)", Minutes, AffiliateMinutes, 0});
		Out.push_back({"Days live (30 days)", static_cast<double>(LiveDays), static_cast<double>(AffiliateDays), 0});
		Out.push_back({"Average viewers (30 days)", Avg, AffiliateAvg, 0});
		return Out;
	}
	if (S.Tier == 2 && S.Streams == 0)
	{
		Out.push_back({"Hours live (30 days)", Minutes / 60.0, PartnerMinutes / 60.0, 0});
		Out.push_back({"Days live (30 days)", static_cast<double>(LiveDays), static_cast<double>(PartnerDays), 0});
		Out.push_back({"Average viewers (30 days)", Avg, PartnerAvg, 0});
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

std::vector<Goal> WeekGoals(const Channel& Ch, double World)
{
	const bool This = Ch.Week == kast_detail::KastWeekOf(World);
	return {
		{"Stream 3 times", This ? static_cast<double>(Ch.WeekStreams) : 0.0, 3.0, 100},
		{"6 hours live", This ? Ch.WeekMinutes / 60.0 : 0.0, 6.0, 100},
		{"Answer 5 questions", This ? static_cast<double>(Ch.WeekAnswers) : 0.0, 5.0, 60},
		{"2 new regulars", This ? static_cast<double>(Ch.WeekRegulars) : 0.0, 2.0, 80},
	};
}

std::vector<SmallChannel> Network(double World)
{
	struct Spec
	{
		std::string Name;
		std::string Title;
		int Viewers;
		int Opens;
		int Hours;
		int Days;
		uint32_t Color;
	};
	static const std::vector<Spec> All = [] {
		static const char* const SmallTitles[] = {"micro stakes grind, come hang", "road to $1k bankroll", "learning MTTs live", "late night turbos", "chill freerolls + music",
			"PKO practice", "first final table?? (day 12)", "grinding with chat", "spin & fold", "the 3am crew"};
		static const uint32_t Colors[] = {0xf97316, 0x22c55e, 0x38bdf8, 0xe879f9, 0xfacc15, 0x14b8a6, 0xef4444, 0xa78bfa};
		Rng R("kast-network");
		std::vector<Spec> V;
		std::set<std::string> Taken;
		while (V.size() < 36)
		{
			Spec S;
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
	std::vector<SmallChannel> Live;
	const double Minute = std::fmod(World, kast_detail::KastDay);
	const int Today = kast_detail::KastDayOf(World);
	for (const Spec& S : All)
	{
		for (int Back = 0; Back <= 1; ++Back)
		{
			const int D = Today - Back;
			const double Since = Minute + static_cast<double>(Back) * kast_detail::KastDay - static_cast<double>(S.Opens);
			if (((S.Days >> kast_detail::KastWeekday(D)) & 1) && Since >= 0.0 && Since < static_cast<double>(S.Hours) * 60.0)
			{
				const double Night = 0.7 + 0.6 * static_cast<double>(kast_detail::NameHash(S.Name + std::to_string(D)) % 100) / 100.0;
				Live.push_back({S.Name, S.Title, std::max(1, static_cast<int>(std::round(static_cast<double>(S.Viewers) * Night))), S.Color});
				break;
			}
		}
	}
	std::stable_sort(Live.begin(), Live.end(), [](const SmallChannel& A, const SmallChannel& B) { return A.Viewers > B.Viewers; });
	return Live;
}

void Offline(Channel& Ch, double World, Rng& R)
{
	if (World <= Ch.LastOffline)
	{
		return;
	}
	Ch.LastOffline = World;
	kast_detail::RollWeek(Ch, World);
	// Subscriptions renew every thirty days: loyal members keep theirs.
	const double Share = Ch.Partner ? 0.7 : 0.5;
	for (Subscriber& S : Ch.Subs)
	{
		while (S.Renews > 0.0 && S.Renews <= World)
		{
			const Member* M = Ch.FindMember(S.Name);
			const double L = M ? M->Loyalty : 0.15;
			if (!R.Chance(S.Gift ? 0.2 + 0.5 * L : 0.45 + 0.5 * L))
			{
				S.Renews = -S.Renews; // lapsed (kept for the history, negative so it never renews)
				break;
			}
			++S.Months;
			S.Gift = false;
			S.Renews += 30.0 * kast_detail::KastDay;
			const Chips Cents = static_cast<Chips>(std::round(static_cast<double>(SubPriceCents) * Share));
			Ch.EarnedSubs += Cents;
			Ch.UnpaidCents += Cents;
		}
	}
	Ch.Subs.erase(std::remove_if(Ch.Subs.begin(), Ch.Subs.end(), [&](const Subscriber& S) { return S.Renews < 0.0 && -S.Renews < World - 30.0 * kast_detail::KastDay; }), Ch.Subs.end());
	// The days go by: members drift when the channel goes quiet, and more when a posted stream doesn't happen.
	const int Today = kast_detail::KastDayOf(World);
	if (Ch.ProcessedDay < 0)
	{
		Ch.ProcessedDay = Today;
	}
	if (Ch.ProcessedDay < Today)
	{
		const int From = std::max(Ch.ProcessedDay, Today - 120);
		for (int D = From; D < Today; ++D)
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
		}
		Ch.ProcessedDay = Today;
		// Members who drifted all the way off are forgotten (followers stay followers: they just never come).
		Ch.Members.erase(std::remove_if(Ch.Members.begin(), Ch.Members.end(),
							 [&](const Member& M) { return !M.Friend && M.Loyalty < 0.012 && M.LastSeen < World - 45.0 * kast_detail::KastDay && !Ch.IsSub(M.Name, World) && !Ch.IsMod(M.Name); }),
			Ch.Members.end());
	}
	// Clips keep getting watched for a few days; a few of those viewers follow.
	double Gained = 0.0;
	for (Clip& C : Ch.Clips)
	{
		const double Was = C.Views;
		C.Views = C.Reach * (1.0 - std::exp(-(World - C.At) / (1.5 * kast_detail::KastDay)));
		Gained += std::max(0.0, C.Views - Was);
	}
	Ch.FollowFrac += Gained * 0.002;
	while (Ch.FollowFrac >= 1.0)
	{
		Ch.FollowFrac -= 1.0;
		++Ch.Followers;
		if (static_cast<int>(Ch.Members.size()) < MaxMembers)
		{
			Member M;
			M.Name = handles::Make(R, handles::PickCountry(R));
			M.Loyalty = 0.03;
			M.Affinity = 0.05 + 0.6 * std::pow(R.Next(), 4.0);
			M.FirstSeen = World;
			M.LastSeen = World;
			M.Follower = true;
			if (!Ch.FindMember(M.Name))
			{
				Ch.Members.push_back(M);
			}
		}
	}
	// Deals run out.
	Ch.Deals.erase(std::remove_if(Ch.Deals.begin(), Ch.Deals.end(), [&](const Deal& D) { return D.Until <= World; }), Ch.Deals.end());
}

// ------------------------------------------------------------------ stream

Stream::Stream(const std::string& Seed) : R(Seed)
{
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

double Stream::StrangerRate(const Channel& Ch, const Inputs& In) const
{
	// Strangers browsing the Poker directory, an hour: a new channel at the bottom of the list gets a handful.
	const double Q = Quality(In);
	double A = (1.5 + 2.0 * Q) * Ch.Discoverability() * kast_detail::Competition(In.World) * (0.4 + 0.6 * kast_detail::TimeOfDay(In.World) / 1.25) * kast_detail::TitleOf(Ch).Discover;
	A *= 1.0 + Hype / 60.0;
	A *= 1.0 + 0.45 * std::log2(1.0 + Viewers / 4.0); // the directory sorts by viewers
	A *= 1.0 + 0.08 * static_cast<double>(std::min(3, std::max(0, In.Tables - 1)));
	A *= In.AtTable ? 1.0 : In.Results ? 0.8 : 0.5;
	A *= In.Sprinting ? 0.8 : 1.0;
	A *= AdRunning(In.Real) ? 0.6 : 1.0;
	A *= 0.6 + 0.4 * Health;
	return A;
}

double Stream::StrangerStay(const Inputs& In) const
{
	// Minutes a stranger stays: longer with a good picture, a streamer who talks, a chat that's alive, a big hand.
	const double Warm = std::min(1.0, static_cast<double>(Present) / 8.0);
	return (4.0 + 6.0 * Quality(In) + 6.0 * Engage + Hype / 15.0 + 4.0 * Warm) * (1.0 + In.Gear.Stay);
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

int Stream::AddMember(Channel& Ch, const Inputs& In, const std::string& Name, bool Follower, double Loyalty)
{
	if (static_cast<int>(Ch.Members.size()) >= MaxMembers || Ch.FindMember(Name))
	{
		return -1;
	}
	Member M;
	M.Name = Name;
	M.Loyalty = Loyalty;
	M.Affinity = 0.05 + 0.9 * std::pow(R.Next(), 4.0); // most are casual
	M.FirstSeen = In.World;
	M.LastSeen = In.World;
	M.Follower = Follower;
	Ch.Members.push_back(M);
	return static_cast<int>(Ch.Members.size()) - 1;
}

void Stream::Invite(Channel& Ch, const Inputs& In, double Chance, double MeanDelay)
{
	// Who shows up: loyal members almost always (on schedule), new followers now and then.
	const double Sched = Ch.ScheduleDays == 0 ? 0.75 : Scheduled ? 1.3 : 0.6;
	const double Tod = std::max(0.4, std::min(1.0, kast_detail::TimeOfDay(In.World) / 1.25));
	std::vector<char> Coming(Ch.Members.size(), 0);
	for (const Visit& V : Visits)
	{
		Coming[static_cast<size_t>(V.Member)] = 1;
	}
	for (size_t I = 0; I < Ch.Members.size(); ++I)
	{
		if (Coming[I])
		{
			continue;
		}
		const Member& M = Ch.Members[I];
		double Pa = M.Friend ? 0.6 : 0.03 + 0.85 * std::pow(M.Loyalty, 1.15);
		Pa *= Sched * Tod * Chance * (In.World - M.LastSeen < 3.0 * kast_detail::KastDay ? 1.15 : 1.0);
		if (!R.Chance(std::min(0.95, Pa)))
		{
			continue;
		}
		Visit V;
		V.Member = static_cast<int>(I);
		V.Arrive = In.World + kast_detail::KastExp(R, MeanDelay);
		V.Leave = V.Arrive + (M.Friend ? R.Range(40.0, 150.0) : std::max(4.0, kast_detail::KastExp(R, 15.0 + 150.0 * M.Loyalty)));
		Visits.push_back(V);
	}
}

void Stream::NewFollower(Channel& Ch, const Inputs& In, double Stay)
{
	std::string Name;
	for (int Try = 0; Try < 8 && (Name.empty() || Ch.FindMember(Name)); ++Try)
	{
		Name = Pool[static_cast<size_t>(R.Int(static_cast<int>(Pool.size())))];
	}
	const int Before = Ch.Followers;
	++Ch.Followers;
	++Tonight.Follows;
	const int Index = Ch.FindMember(Name) ? -1 : AddMember(Ch, In, Name, true, 0.04);
	if (Index >= 0)
	{
		// They're watching right now: tonight counts toward their loyalty.
		Visit V;
		V.Member = Index;
		V.Arrive = In.World;
		V.Leave = In.World + std::max(5.0, kast_detail::KastExp(R, Stay * 2.0));
		Visits.push_back(V);
	}
	Alert A;
	A.Kind = AlertKind::Follow;
	A.Who = Name;
	Queue(A);
	if (Before < 2400 && Ch.Followers >= 2400)
	{
		Notice N;
		N.Type = Notice::Kind::Text;
		N.From = "gh0stfold";
		N.Body = "saw you passed me on kast. cute. see you at the tables.";
		Notices.push_back(N);
	}
}

int Stream::HereVisit(const Channel& Ch, const std::string& Name) const
{
	for (const int I : Here)
	{
		const int M = Visits[static_cast<size_t>(I)].Member;
		if (M >= 0 && static_cast<size_t>(M) < Ch.Members.size() && Ch.Members[static_cast<size_t>(M)].Name == Name)
		{
			return I;
		}
	}
	return -1;
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
	// Who talks: the community members watching (the loyal ones most), or a stranger passing through.
	double MemberWeight = 0.0;
	for (const int I : Here)
	{
		MemberWeight += 0.2 + Ch.Members[static_cast<size_t>(Visits[static_cast<size_t>(I)].Member)].Loyalty;
	}
	const double StrangerWeight = Strangers * 0.1 + Lurkers * 0.03 + RaidViewers * 0.15 + 0.02;
	std::string Name;
	if (!Here.empty() && R.Next() * (MemberWeight + StrangerWeight) < MemberWeight)
	{
		double Pick = R.Next() * MemberWeight;
		int Chosen = Here.front();
		for (const int I : Here)
		{
			Pick -= 0.2 + Ch.Members[static_cast<size_t>(Visits[static_cast<size_t>(I)].Member)].Loyalty;
			if (Pick <= 0.0)
			{
				Chosen = I;
				break;
			}
		}
		Visit& V = Visits[static_cast<size_t>(Chosen)];
		V.Chatted = true;
		Member& M = Ch.Members[static_cast<size_t>(V.Member)];
		++M.Messages;
		Name = M.Name;
	}
	else
	{
		const int Reach = std::min(static_cast<int>(Pool.size()), 30 + static_cast<int>(Viewers * 1.5));
		Name = Pool[static_cast<size_t>(R.Int(std::max(1, Reach)))];
	}
	Badges = 0;
	if (Ch.IsSub(Name, In.World))
	{
		Badges |= BadgeSub;
	}
	if (Ch.IsMod(Name))
	{
		Badges |= BadgeMod;
	}
	const Member* M = Ch.FindMember(Name);
	if (M && M->Loyalty >= SuperfanLoyalty && !(Badges & BadgeMod) && Ch.Affiliate)
	{
		Badges |= BadgeVip;
	}
	Color = kast_detail::NameColor(Name);
	Seen.insert(Name);
	return Name;
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
	if (Cents <= 0)
	{
		return;
	}
	Ch.UnpaidCents += Cents;
	switch (Source)
	{
	case 0:
		Ch.EarnedSubs += Cents;
		Tonight.SubCents += Cents;
		break;
	case 1:
		Ch.EarnedBits += Cents;
		Tonight.BitCents += Cents;
		break;
	case 2:
		Ch.EarnedTips += Cents;
		Tonight.TipCents += Cents;
		break;
	case 3:
		Ch.EarnedAds += Cents;
		Tonight.AdCents += Cents;
		break;
	default:
		Ch.EarnedSponsors += Cents;
		Tonight.SponsorCents += Cents;
		break;
	}
}

void Stream::Subscribe(Channel& Ch, const Inputs& In, const std::string& Who, bool Gift, int Count)
{
	const double Share = Ch.Partner ? 0.7 : 0.5;
	for (int K = 0; K < Count; ++K)
	{
		// A gift lands on someone watching who isn't subscribed yet.
		std::string To = Who;
		if (Gift)
		{
			To = Pool[static_cast<size_t>(R.Int(static_cast<int>(Pool.size())))];
			for (const int I : Here)
			{
				const std::string& N = Ch.Members[static_cast<size_t>(Visits[static_cast<size_t>(I)].Member)].Name;
				if (N != Who && !Ch.IsSub(N, In.World) && R.Chance(0.5))
				{
					To = N;
					break;
				}
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
		M->Loyalty = std::min(1.0, M->Loyalty + 0.04); // money in, heart in
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
	Say(In, Who, Gift ? "gifted " + std::to_string(Count) + " sub" + (Count == 1 ? "" : "s") + " to the community!" : "just subscribed!" + (A.Text.empty() ? std::string() : " " + A.Text), LineKind::Sub,
		BadgeSub | (Gift ? BadgeGifter : 0), kast_detail::NameColor(Who));
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
		std::string Line = R.Pick(kast_detail::Lines(M));
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

void Stream::MakeClip(Channel& Ch, const Inputs& In, Moment M, const std::string& Detail, double Size)
{
	// Somebody has to be watching to clip it; a clip reaches about as far as the channel does.
	const kast_detail::Hit H = kast_detail::HitOf(M);
	const double Chance = std::min(0.95, H.ClipChance * std::min(1.0, 0.1 + Viewers / 40.0));
	if (Viewers < 1.5 || !R.Chance(Chance))
	{
		return;
	}
	Clip C;
	C.Kind = static_cast<int>(M);
	C.At = In.World;
	C.Title = kast_detail::ClipTitle(M, In.Hero, Detail, R);
	int Badges = 0;
	uint32_t Color = 0;
	C.By = Chatter(Ch, In, Badges, Color);
	const double Spread = std::exp(R.Gauss(0.0, 0.8));
	double Reach = (5.0 + Viewers * 3.0) * Spread * (1.0 + Hype / 50.0) * std::max(0.5, Size);
	const bool Viral = R.Chance(M == Moment::Win || M == Moment::BadBeat ? 0.008 : 0.003);
	Reach *= Viral ? 30.0 : 1.0;
	C.Reach = Reach;
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
	}
}

void Stream::CheckGrowth(Channel& Ch, const Inputs& In)
{
	static const int Steps[] = {10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000, 25000, 50000, 100000, 250000, 500000, 1000000};
	for (const int Step : Steps)
	{
		if (Ch.Followers >= Step && Ch.Milestone < Step)
		{
			Ch.Milestone = Step;
			Alert A;
			A.Kind = AlertKind::Milestone;
			A.Count = Step;
			A.Text = Grouped(Step) + " followers!";
			Queue(A);
			Hype = std::min(100.0, Hype + 6.0);
			Say(In, "", "Milestone: " + Grouped(Step) + " followers!", LineKind::System, 0, Lime);
		}
	}
	// Kast's rules look at the last 30 days (tonight included).
	double Minutes = 0.0;
	int LiveDays = 0;
	double Avg = 0.0;
	Ch.Window30(In.World, Minutes, LiveDays, Avg);
	const double Up = Live ? In.World - StartWorld : 0.0;
	if (Up > 1.0)
	{
		Avg = (Avg * Minutes + ViewerMinutes) / (Minutes + Up);
		Minutes += Up;
	}
	if (!Ch.Affiliate && Ch.Followers >= AffiliateFollowers && Minutes >= AffiliateMinutes && LiveDays >= AffiliateDays && Avg >= AffiliateAvg)
	{
		Ch.Affiliate = true;
		Alert A;
		A.Kind = AlertKind::Affiliate;
		A.Text = "Subs, cheers and ads are on";
		Queue(A);
		Notice N;
		N.Type = Notice::Kind::Text;
		N.From = "Kast";
		N.Body = "Congratulations, you're a Kast Affiliate! Subscriptions, cheers and ad breaks are now on for your channel.";
		Notices.push_back(N);
		Say(In, "", "This channel is now a Kast Affiliate. Subscribe to support " + In.Hero + "!", LineKind::System, 0, Violet);
	}
	if (Ch.Affiliate && !Ch.Partner && Minutes >= PartnerMinutes && LiveDays >= PartnerDays && Avg >= PartnerAvg)
	{
		Ch.Partner = true;
		Alert A;
		A.Kind = AlertKind::Partner;
		A.Text = "Verified \xC2\xB7 70% of every sub";
		Queue(A);
		Notice N;
		N.Type = Notice::Kind::Text;
		N.From = "Kast";
		N.Body = "Welcome to the Kast Partner Program. Your channel is verified, your sub share is now 70%, and ads pay more.";
		Notices.push_back(N);
	}
	// Community stages, one at a time.
	const int Reached = kast_detail::StageReached(Ch, In.World, Up, ViewerMinutes);
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
	// Weekly goals pay out once each.
	const std::vector<Goal> Week = WeekGoals(Ch, In.World);
	for (size_t I = 0; I < Week.size(); ++I)
	{
		if (Week[I].Have >= Week[I].Need && !((Ch.WeekRewarded >> I) & 1))
		{
			Ch.WeekRewarded |= 1 << I;
			Ch.Xp += static_cast<double>(Week[I].Xp);
			if (Live)
			{
				Say(In, "", "Weekly goal done: " + Week[I].Label + " (+" + std::to_string(Week[I].Xp) + " XP)", LineKind::System, 0, Lime);
			}
		}
	}
	for (const Sponsor& S : Sponsors())
	{
		if (Ch.Followers >= S.Followers && Avg >= static_cast<double>(S.AvgViewers) && (!S.NeedsPartner || Ch.Partner) && !Ch.Offers.count(S.Id) && !Ch.Declined.count(S.Id) && !Ch.ActiveDeal(S.Id, In.World))
		{
			bool Had = false;
			for (const Deal& D : Ch.Deals)
			{
				Had = Had || D.Id == S.Id;
			}
			if (!Had)
			{
				Ch.Offers.insert(S.Id);
				Notice N;
				N.Type = Notice::Kind::Text;
				N.From = S.Brand;
				N.Body = S.Pitch + " (Kast > Channel to answer)";
				Notices.push_back(N);
			}
		}
	}
}

void Stream::Start(Channel& Ch, const Inputs& In)
{
	if (Live || !In.Gear.CanStream())
	{
		return;
	}
	if (Pool.empty())
	{
		std::set<std::string> Taken = {In.Hero, "dee_spincycle", "mei_ng"};
		while (Pool.size() < 700)
		{
			const std::string Name = handles::Make(R, handles::PickCountry(R));
			if (Taken.insert(Name).second)
			{
				Pool.push_back(Name);
			}
		}
	}
	Prefix = EmotePrefix(In.Hero);
	Offline(Ch, In.World, R);
	kast_detail::RollWeek(Ch, In.World);
	// The first night: two friends from the laundromat are the whole audience.
	if (Ch.Streams == 0 && Ch.Members.empty())
	{
		AddMember(Ch, In, "dee_spincycle", true, 0.85);
		AddMember(Ch, In, "mei_ng", true, 0.7);
		Ch.Members[0].Friend = true;
		Ch.Members[1].Friend = true;
		Ch.Members[0].Affinity = 0.95;
		Ch.Members[1].Affinity = 0.85;
		Ch.Followers += 2;
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
	Scheduled = Ch.OnSchedule(In.World);
	Tonight.OnSchedule = Scheduled;
	// Who's coming tonight (they turn up over the first half hour or so; latecomers are rolled for each hour).
	Invite(Ch, In, 1.0, Scheduled ? 8.0 : 18.0);
	RollAt = In.World + 60.0;
	OnlineMods.clear();
	for (const Moderator& M : Ch.Mods)
	{
		if (R.Chance(M.Online))
		{
			OnlineMods.push_back(M.Name);
		}
	}
	++Ch.Streams;
	++Ch.WeekStreams;
	const int Today = kast_detail::KastDayOf(In.World);
	if (std::find(Ch.DaysLive.begin(), Ch.DaysLive.end(), Today) == Ch.DaysLive.end())
	{
		Ch.DaysLive.push_back(Today);
		if (Ch.DaysLive.size() > 60)
		{
			Ch.DaysLive.erase(Ch.DaysLive.begin());
		}
	}
	Say(In, "", "You're live! " + (Ch.Followers > 0 ? Grouped(Ch.Followers) + (Ch.Followers == 1 ? " follower was" : " followers were") + " notified." : std::string("Share the link and say hi to chat.")) +
		(Scheduled ? " On schedule: your regulars knew you'd be here." : std::string()), LineKind::System, 0, Violet);
	if (!OnlineMods.empty())
	{
		Say(In, "", std::to_string(OnlineMods.size()) + (OnlineMods.size() == 1 ? " moderator is" : " moderators are") + " watching chat.", LineKind::System, 0, 0x22c55e);
	}
	if (Ch.Streams == 1)
	{
		Notice N;
		N.Type = Notice::Kind::Text;
		N.From = "Kast";
		N.Body = "Your first stream is live! Growing a channel takes time: stream on a schedule, talk to chat, answer questions. Regulars are built one stream at a time.";
		Notices.push_back(N);
	}
	CheckGrowth(Ch, In);
}

Summary Stream::Stop(Channel& Ch, const Inputs& In, const std::string& Raid)
{
	if (!Live)
	{
		return Summary();
	}
	Summary S = Tonight;
	S.Valid = true;
	S.Minutes = std::max(0.0, In.World - StartWorld);
	S.Avg = S.Minutes > 0.5 ? static_cast<int>(std::round(ViewerMinutes / S.Minutes)) : static_cast<int>(std::round(Viewers));
	S.Peak = Peak;
	S.Trolls = Caught + Missed;
	// Ending with a raid: tonight's viewers go to a small channel, who'll remember it.
	if (!Raid.empty())
	{
		S.RaidedOut = Raid;
		S.RaidSize = std::max(1, static_cast<int>(std::round(Viewers * 0.7)));
		Ch.Goodwill[Raid] += 1;
		++Ch.RaidsOut;
	}
	// Loyalty: everyone who watched tonight is a little more part of this.
	for (const Visit& V : Visits)
	{
		if (V.Watched < 0.5 || V.Member < 0 || static_cast<size_t>(V.Member) >= Ch.Members.size())
		{
			continue;
		}
		Member& M = Ch.Members[static_cast<size_t>(V.Member)];
		if (M.FirstSeen < StartWorld - 0.01)
		{
			++S.Returning;
		}
		++M.Streams;
		M.WatchMinutes += V.Watched;
		M.LastSeen = In.World;
		const bool WasRegular = M.Loyalty >= RegularLoyalty;
		// Being noticed (answered, greeted, thanked) makes people care a little more than they meant to.
		if (V.Engaged)
		{
			M.Affinity = std::min(1.0, M.Affinity + 0.04 * (1.0 - M.Affinity));
		}
		double Gain = 0.12 * std::pow(std::min(V.Watched, 180.0) / 60.0, 0.75) * std::max(0.0, M.Affinity - M.Loyalty);
		Gain *= (Scheduled ? 1.3 : 1.0) * (1.0 + 0.6 * (V.Engaged ? 1.0 : 0.0) + 0.3 * (V.Chatted ? 1.0 : 0.0)) * (Raid.empty() ? 1.0 : 1.05);
		M.Loyalty = std::min(1.0, M.Loyalty + Gain);
		if (!WasRegular && M.Loyalty >= RegularLoyalty && !M.Friend)
		{
			++S.NewRegulars;
			++Ch.WeekRegulars;
			S.Xp += 40.0;
			Ch.Xp += 40.0;
		}
	}
	Ch.Xp += Tonight.Xp;
	Ch.MinutesLive += S.Minutes;
	Ch.ViewerMinutes += ViewerMinutes;
	Ch.WeekMinutes += S.Minutes;
	Ch.Peak = std::max(Ch.Peak, Peak);
	StreamLog Lg;
	Lg.Start = StartWorld;
	Lg.Minutes = S.Minutes;
	Lg.Avg = S.Avg;
	Lg.Peak = S.Peak;
	Lg.Follows = S.Follows;
	Lg.Subs = S.Subs + S.Gifted;
	Lg.Cents = S.Total();
	Lg.Title = kast_detail::TitleOf(Ch).Text;
	Lg.Returning = S.Returning;
	Lg.OnSchedule = Scheduled;
	if (S.Minutes >= 1.0)
	{
		Ch.Log.insert(Ch.Log.begin(), Lg);
		if (Ch.Log.size() > 60)
		{
			Ch.Log.pop_back();
		}
	}
	CheckGrowth(Ch, In);
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
	if (!Live)
	{
		return;
	}
	LastReal = In.Real;
	const kast_detail::Hit H = kast_detail::HitOf(M);
	Hype = std::min(100.0, Hype + H.Hype * kast_detail::TitleOf(Ch).Hype * std::max(0.5, std::min(2.0, Size)) * (1.0 + In.Gear.Hype));
	LastMoment = M;
	LastMomentReal = In.Real;
	if (H.Face != Mood::Focus || M == Moment::BestPlay)
	{
		Face = H.Face;
		FaceAt = In.Real;
	}
	// A wave in chat, as big as the room.
	const int Lines = std::min(14, std::max(Present > 0 || Viewers >= 1.0 ? 1 : 0, std::min(H.Lines, 1 + static_cast<int>(Viewers)) + static_cast<int>(std::sqrt(Viewers) / 3.0)));
	React(Ch, In, M, Lines);
	if (M == Moment::Knockout && !Detail.empty() && Lines > 0)
	{
		Chat.back().Text = "bye " + Detail + " kastGG";
	}
	MakeClip(Ch, In, M, Detail, Size);
	// The big moments: superfans gift subs, and the network notices deep runs.
	const bool Big = M == Moment::WonAllIn || M == Moment::InTheMoney || M == Moment::FinalTable || M == Moment::Win;
	if (Big && Ch.Affiliate)
	{
		for (const int I : Here)
		{
			const Member& Fan = Ch.Members[static_cast<size_t>(Visits[static_cast<size_t>(I)].Member)];
			if (Fan.Loyalty >= SuperfanLoyalty && R.Chance(0.05))
			{
				Subscribe(Ch, In, Fan.Name, true, 1 + R.Int(std::min(5, 1 + Present / 15)));
				break;
			}
		}
	}
	if (M == Moment::FinalTable || M == Moment::Win || M == Moment::InTheMoney)
	{
		const bool Established = Ch.Stage >= 6;
		const double Chance = M == Moment::Win ? 0.3 : M == Moment::FinalTable ? 0.15 : 0.05;
		if (R.Chance(Chance))
		{
			std::string From;
			int Size2 = 0;
			uint32_t Color = Violet;
			std::vector<const Streamer*> BigLive;
			for (const Streamer& S : Directory())
			{
				if (!S.Rival && ViewersNow(S, In.World) > 0)
				{
					BigLive.push_back(&S);
				}
			}
			if (Established && !BigLive.empty() && R.Chance(M == Moment::InTheMoney ? 0.2 : 0.5))
			{
				// Only an established channel gets noticed by the big names.
				const Streamer& S = *BigLive[static_cast<size_t>(R.Int(static_cast<int>(BigLive.size())))];
				From = S.Name;
				const double Share = R.Range(0.05, 0.15);
				Size2 = std::max(5, static_cast<int>(static_cast<double>(ViewersNow(S, In.World)) * Share));
				Color = S.Color;
			}
			else
			{
				const std::vector<SmallChannel> Net = Network(In.World);
				if (!Net.empty())
				{
					const SmallChannel& S = Net[static_cast<size_t>(R.Int(static_cast<int>(Net.size())))];
					From = S.Name;
					const double Share = R.Range(0.5, 0.9);
					Size2 = std::max(1, static_cast<int>(std::round(static_cast<double>(S.Viewers) * Share)));
					Color = S.Color;
				}
			}
			if (!From.empty())
			{
				RaidViewers += static_cast<double>(Size2);
				LastRaider = From;
				++Ch.RaidsIn;
				++Tonight.Raids;
				Hype = std::min(100.0, Hype + 15.0);
				FollowDebt += static_cast<double>(Size2) * 0.06;
				Alert A;
				A.Kind = AlertKind::Raid;
				A.Who = From;
				A.Count = Size2;
				Queue(A);
				Say(In, "", From + " is raiding with " + Grouped(Size2) + (Size2 == 1 ? " viewer!" : " viewers!"), LineKind::Raid, 0, Color);
				static const std::vector<std::string> RaidLines = {"RAID kastHype", "{raider} sent us!", "hi from {raider}'s stream", "kastPog kastPog", "raid hype", "GL on the run!", "we're here to sweat"};
				for (int K = 0; K < std::min(6, 1 + Size2 / 4); ++K)
				{
					const std::string Who = Pool[static_cast<size_t>(R.Int(static_cast<int>(Pool.size())))];
					Say(In, Who, kast_detail::Fill(R.Pick(RaidLines), "{raider}", From), LineKind::Chat, 0, kast_detail::NameColor(Who));
					Chat.back().At = In.Real + 0.4 + static_cast<double>(K) * 0.35;
				}
			}
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
	CheckGrowth(Ch, In);
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
	for (int K = 0; K < std::min(3, 1 + static_cast<int>(Viewers)); ++K)
	{
		int Badges = 0;
		uint32_t Color = 0;
		const std::string Who = Chatter(Ch, In, Badges, Color);
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
	const double Slots = static_cast<double>(Seconds) / 30.0;
	const double PerSlot = Ch.Partner ? 0.45 : 0.32; // cents per viewer per 30 seconds
	Earn(Ch, static_cast<Chips>(std::round(Viewers * Slots * PerSlot)), 3);
	Health = std::max(0.0, Health - (Seconds >= 180 ? 0.06 : Seconds >= 90 ? 0.03 : 0.015));
	Say(In, "", "Running a " + std::to_string(Seconds) + "-second ad break.", LineKind::System, 0, 0x94a3b8);
	return true;
}

Chips Stream::SponsorRead(Channel& Ch, const Inputs& In, const std::string& Id)
{
	if (!Live)
	{
		return 0;
	}
	for (Deal& D : Ch.Deals)
	{
		if (D.Id == Id && D.Until > In.World && D.ReadAt < StartWorld)
		{
			const Sponsor* S = FindSponsor(Id);
			if (!S)
			{
				return 0;
			}
			D.ReadAt = In.World;
			D.EarnedCents += S->ReadCents;
			Earn(Ch, S->ReadCents, 4);
			Say(In, In.Hero, S->ReadLine, LineKind::Streamer, BadgeStreamer | (Ch.Partner ? BadgeVerified : 0), 0xc6f432);
			static const std::vector<std::string> Reacts = {"ad read kastLUL", "sellout kastLUL", "get that bag", "rent money", "{brand}!!", "the read was smooth"};
			for (int K = 0; K < std::min(3, 1 + static_cast<int>(Viewers / 3.0)); ++K)
			{
				int Badges = 0;
				uint32_t Color = 0;
				const std::string Who = Chatter(Ch, In, Badges, Color);
				Say(In, Who, kast_detail::Fill(R.Pick(Reacts), "{brand}", S->Brand), LineKind::Chat, Badges, Color);
				Chat.back().At = In.Real + 0.6 + static_cast<double>(K) * 0.6;
			}
			Health = std::max(0.0, Health - 0.01);
			return S->ReadCents;
		}
	}
	return 0;
}

bool Stream::Thank(Channel& Ch, const Inputs& In)
{
	if (!Live || In.Real - ThankAt < 20.0)
	{
		return false;
	}
	ThankAt = In.Real;
	Say(In, In.Hero, R.Pick(kast_detail::ThankLines()), LineKind::Streamer, BadgeStreamer | (Ch.Partner ? BadgeVerified : 0), 0xc6f432);
	Engage = std::min(1.0, Engage + 0.25);
	Health = std::min(1.0, Health + 0.03);
	// The people in the room feel seen.
	for (const int I : Here)
	{
		if (R.Chance(0.5))
		{
			Visits[static_cast<size_t>(I)].Engaged = true;
		}
	}
	static const std::vector<std::string> Back = {"kastLove", "<3", "love you too", "ty for streaming", "kastLove kastLove", "best streamer"};
	for (int K = 0; K < std::min(6, std::min(Present + (Strangers > 0.5 ? 1 : 0), 2 + static_cast<int>(std::sqrt(Viewers) / 4.0))); ++K)
	{
		int Badges = 0;
		uint32_t Color = 0;
		const std::string Who = Chatter(Ch, In, Badges, Color);
		Say(In, Who, R.Pick(Back), LineKind::Chat, Badges, Color);
		Chat.back().At = In.Real + 0.5 + static_cast<double>(K) * 0.4;
	}
	return true;
}

bool Stream::Answer(Channel& Ch, const Inputs& In, int MsgId)
{
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
			static const std::vector<std::string> Thanks = {"thanks!", "ty kastLove", "makes sense", "good answer", "appreciate it"};
			Say(In, Who, R.Pick(Thanks), LineKind::Chat, Ch.IsSub(Who, In.World) ? BadgeSub : 0, kast_detail::NameColor(Who));
			Chat.back().At = In.Real + 1.2;
			// Being answered is how a stranger becomes a follower, and a follower a regular.
			const int V = HereVisit(Ch, Who);
			if (V >= 0)
			{
				Visits[static_cast<size_t>(V)].Engaged = true;
			}
			else if (!Ch.FindMember(Who) && R.Chance(0.45))
			{
				FollowDebt += 1.0;
			}
			CheckGrowth(Ch, In);
			return true;
		}
	}
	return false;
}

bool Stream::Timeout(Channel& Ch, const Inputs& In, int MsgId)
{
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
			if (Deserved)
			{
				++Caught;
				Health = std::min(1.0, Health + 0.02);
				if (R.Chance(0.6) && Viewers >= 2.0)
				{
					int Badges = 0;
					uint32_t Color = 0;
					const std::string Fan = Chatter(Ch, In, Badges, Color);
					Say(In, Fan, R.Chance(0.5) ? "get him outta here" : "kastClap", LineKind::Chat, Badges, Color);
					Chat.back().At = In.Real + 0.8;
				}
			}
			else
			{
				// Timing out someone for nothing: they leave hurt, and the room noticed.
				Health = std::max(0.0, Health - 0.06);
				if (Member* Hurt = Ch.FindMember(Who))
				{
					Hurt->Loyalty = std::max(0.0, Hurt->Loyalty - 0.15);
				}
				const int V = HereVisit(Ch, Who);
				if (V >= 0)
				{
					Visits[static_cast<size_t>(V)].Leave = In.World;
				}
				if (Viewers >= 2.0)
				{
					int Badges = 0;
					uint32_t Color = 0;
					const std::string Fan = Chatter(Ch, In, Badges, Color);
					Say(In, Fan, "why did " + Who + " get timed out?? kastLUL", LineKind::Chat, Badges, Color);
					Chat.back().At = In.Real + 0.8;
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
	Moderator M;
	M.Name = Name;
	M.Since = In.World;
	const Member* Mem = Ch.FindMember(Name);
	M.Online = std::min(0.95, 0.4 + 0.5 * (Mem ? Mem->Loyalty : 0.2));
	Ch.Mods.push_back(M);
	if (Member* Mm = Ch.FindMember(Name))
	{
		Mm->Loyalty = std::min(1.0, Mm->Loyalty + 0.1); // trusted
	}
	if (Live)
	{
		OnlineMods.push_back(Name);
		Say(In, "", Name + " is now a moderator.", LineKind::System, 0, 0x22c55e);
		static const std::vector<std::string> Lines2 = {"congrats {who} kastClap", "mod {who} kastHype", "power trip incoming kastLUL", "well deserved"};
		if (Viewers >= 2.0)
		{
			int Badges = 0;
			uint32_t Color = 0;
			const std::string Fan = Chatter(Ch, In, Badges, Color);
			Say(In, Fan, kast_detail::Fill(R.Pick(Lines2), "{who}", Name), LineKind::Chat, Badges, Color);
			Chat.back().At = In.Real + 0.7;
		}
		Say(In, Name, "thank you!! I'll keep it clean", LineKind::Chat, BadgeMod | (Ch.IsSub(Name, In.World) ? BadgeSub : 0), kast_detail::NameColor(Name));
		Chat.back().At = In.Real + 1.5;
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

void Stream::TrollStep(Channel& Ch, const Inputs& In, double DReal)
{
	// Trolls follow strangers and size: a quiet little stream rarely sees one.
	const double Rate = (0.0005 + 0.00008 * Viewers + 0.0003 * Strangers) * (1.0 + Hype / 50.0) * kast_detail::TitleOf(Ch).Calm * (Ch.Partner ? 1.1 : 1.0);
	TrollDebt = std::min(3.0, TrollDebt + Rate * DReal);
	while (TrollDebt >= 1.0)
	{
		TrollDebt -= 1.0;
		const bool IsSpam = R.Chance(0.3);
		std::string Who = Pool[static_cast<size_t>(R.Int(static_cast<int>(Pool.size())))];
		if (IsSpam)
		{
			static const std::vector<std::string> Bots = {"v1ewz_b0t", "free_chips_4u", "promo_kingz", "k4st_boost", "followerz_cheap"};
			Who = R.Pick(Bots) + std::to_string(R.Int(90) + 10);
		}
		if (Ch.IsMod(Who) || Ch.FindMember(Who))
		{
			continue; // the community doesn't troll
		}
		Say(In, Who, IsSpam ? R.Pick(kast_detail::Spam()) : kast_detail::Fill(R.Pick(kast_detail::Trolls()), "{hero}", In.Hero), LineKind::Chat, 0, kast_detail::NameColor(Who));
		ChatMsg& M = Chat.back();
		M.Toxic = !IsSpam;
		M.Spam = IsSpam;
		if (R.Chance(In.Gear.ModBot * (IsSpam ? 1.1 : 0.9)))
		{
			M.DeleteAt = In.Real + R.Range(0.2, 0.6);
			M.DeletedBy = "WardenBot";
			continue;
		}
		bool Caught2 = false;
		for (const std::string& Mod : OnlineMods)
		{
			if (!Caught2 && R.Chance(0.45))
			{
				M.DeleteAt = In.Real + R.Range(1.0, 6.0);
				M.DeletedBy = Mod;
				Caught2 = true;
			}
		}
		if (!Caught2)
		{
			M.DeleteAt = -2.0;
		}
	}
}

void Stream::Tick(Channel& Ch, const Inputs& In, double DWorld, double DReal)
{
	if (!Live)
	{
		return;
	}
	DWorld = std::max(0.0, DWorld);
	DReal = std::max(0.0, std::min(1.0, DReal));
	LastReal = In.Real;
	const double Q = Quality(In);
	const TitleSpec& T = kast_detail::TitleOf(Ch);

	// Hype and raiders fade on the clock; talking to chat fades in real time.
	Hype *= std::exp(-DWorld / 12.0);
	RaidViewers *= std::exp(-DWorld / 20.0);
	Engage *= std::exp(-DReal / 90.0);
	Health = std::min(1.0, Health + DReal / 60.0 * 0.03);

	// A bigger audience brings more names into chat.
	while (Pool.size() < 6000 && static_cast<double>(Pool.size()) < 400.0 + Viewers * 3.0)
	{
		const std::string Name = handles::Make(R, handles::PickCountry(R));
		if (Name != In.Hero && std::find(Pool.end() - std::min<std::ptrdiff_t>(static_cast<std::ptrdiff_t>(Pool.size()), 400), Pool.end(), Name) == Pool.end())
		{
			Pool.push_back(Name);
		}
	}

	// The community: who's in the room right now.
	const double From = In.World - DWorld;
	const size_t WasHere = Here.size();
	std::vector<char> WasPresent(Visits.size(), 0);
	for (const int I : Here)
	{
		WasPresent[static_cast<size_t>(I)] = 1;
	}
	Here.clear();
	RegularsHere = 0;
	for (size_t I = 0; I < Visits.size(); ++I)
	{
		Visit& V = Visits[I];
		const double Overlap = std::min(In.World, V.Leave) - std::max(From, V.Arrive);
		if (Overlap > 0.0)
		{
			V.Watched += Overlap;
		}
		if (In.World >= V.Arrive && In.World < V.Leave)
		{
			Here.push_back(static_cast<int>(I));
			const Member& M = Ch.Members[static_cast<size_t>(V.Member)];
			RegularsHere += M.Loyalty >= RegularLoyalty && !M.Friend ? 1 : 0;
			// Someone just walked in: the familiar ones say hi.
			if (!WasPresent[I] && R.Chance(0.25 + 0.5 * M.Loyalty))
			{
				static const std::vector<std::string> Back = {"hey {hero}", "evening", "back again", "made it", "o7", "hi chat", "what did I miss", "gl tonight", "kastLove"};
				Say(In, M.Name, kast_detail::Fill(R.Pick(Back), "{hero}", In.Hero), LineKind::Chat, Ch.IsSub(M.Name, In.World) ? BadgeSub : 0, kast_detail::NameColor(M.Name));
				Chat.back().At = In.Real + R.Range(0.2, 2.0);
				V.Chatted = true;
				++Ch.Members[static_cast<size_t>(V.Member)].Messages;
			}
		}
	}
	(void)WasHere;
	Present = static_cast<int>(Here.size());
	if (In.World >= RollAt)
	{
		RollAt += 60.0;
		Invite(Ch, In, 0.3, 15.0); // latecomers
	}

	// Strangers from the directory: they drop in, look around, mostly leave; some follow.
	const double Arrivals = StrangerRate(Ch, In);
	const double Stay = StrangerStay(In);
	const double Warm = std::min(1.0, static_cast<double>(Present) / 8.0);
	Strangers += (Arrivals / 60.0 * Stay - Strangers) * (1.0 - std::exp(-DWorld / std::max(1.0, Stay)));
	ArriveDebt += Arrivals * DWorld / 60.0;
	if (ArriveDebt >= 1.0)
	{
		const int Whole = static_cast<int>(std::floor(ArriveDebt));
		ArriveDebt -= static_cast<double>(Whole);
		Tonight.NewFaces += Whole;
	}
	const double FollowChance = (0.027 + 0.025 * Q) * (1.0 + In.Gear.Follow) * T.Follow * (0.6 + 0.4 * Health) * (1.0 + 0.6 * Warm) * (1.0 + 0.8 * Engage) * (In.Sprinting ? 0.6 : 1.0);
	FollowDebt += Arrivals * DWorld / 60.0 * FollowChance;
	// Regulars bring friends (twice as often once the community is a real one).
	double Bring = 0.0;
	for (const int I : Here)
	{
		Bring += std::max(0.0, Ch.Members[static_cast<size_t>(Visits[static_cast<size_t>(I)].Member)].Loyalty - 0.4);
	}
	WordDebt += Bring * 0.05 * (Ch.Stage >= 3 ? 2.0 : 1.0) * DWorld / 60.0;
	while (WordDebt >= 1.0)
	{
		WordDebt -= 1.0;
		++Tonight.NewFaces;
		Strangers += 1.0;
		FollowDebt += 0.35;
	}
	while (FollowDebt >= 1.0)
	{
		FollowDebt -= 1.0;
		NewFollower(Ch, In, Stay);
	}
	// Followers beyond the tracked community (a big channel's long tail): a few of them lurk.
	const double Untracked = static_cast<double>(std::max(0, Ch.Followers - kast_detail::TrackedFollowers(Ch)));
	const double Sched = Ch.ScheduleDays == 0 ? 0.75 : Scheduled ? 1.3 : 0.6;
	Lurkers += (Untracked * 0.01 * Sched * (0.4 + 0.6 * kast_detail::TimeOfDay(In.World) / 1.25) - Lurkers) * (1.0 - std::exp(-DWorld / 10.0));

	Viewers = static_cast<double>(Present) + Strangers + Lurkers + RaidViewers;
	Peak = std::max(Peak, static_cast<int>(std::round(Viewers)));
	ViewerMinutes += Viewers * DWorld;
	Tonight.Xp += DWorld * (1.0 + 0.05 * std::sqrt(Viewers));
	GraphDebt += DWorld;
	if (Graph.empty() || GraphDebt >= 1.0)
	{
		GraphDebt = std::fmod(GraphDebt, 1.0);
		Graph.push_back(static_cast<float>(Viewers));
		if (Graph.size() > 720)
		{
			std::vector<float> Half;
			for (size_t I = 0; I + 1 < Graph.size(); I += 2)
			{
				Half.push_back((Graph[I] + Graph[I + 1]) * 0.5f);
			}
			Graph = Half;
		}
	}

	// Money comes from the people who care: loyal members sub, cheer and tip; strangers almost never.
	double SumL = 0.0;
	double SumL15 = 0.0;
	for (const int I : Here)
	{
		const double L = Ch.Members[static_cast<size_t>(Visits[static_cast<size_t>(I)].Member)].Loyalty;
		SumL += L;
		SumL15 += std::pow(L, 1.5);
	}
	auto GiverIn = [&](double Power) -> std::string {
		// A member weighted by loyalty (or a passer-by when nobody's here).
		double Total = 0.0;
		for (const int I : Here)
		{
			Total += std::pow(Ch.Members[static_cast<size_t>(Visits[static_cast<size_t>(I)].Member)].Loyalty, Power);
		}
		double Pick = R.Next() * Total;
		for (const int I : Here)
		{
			const Member& M = Ch.Members[static_cast<size_t>(Visits[static_cast<size_t>(I)].Member)];
			Pick -= std::pow(M.Loyalty, Power);
			if (Pick <= 0.0)
			{
				return M.Name;
			}
		}
		return Pool[static_cast<size_t>(R.Int(static_cast<int>(Pool.size())))];
	};
	if (Ch.Affiliate)
	{
		SubDebt += (SumL15 * 0.025 * (1.0 + 0.5 * Q) * (1.0 + Hype / 80.0) + (Strangers + Lurkers) * 0.0008) * DWorld / 60.0;
		while (SubDebt >= 1.0)
		{
			SubDebt -= 1.0;
			std::string Who = GiverIn(1.5);
			for (int Try = 0; Try < 4 && Ch.IsSub(Who, In.World); ++Try)
			{
				Who = GiverIn(1.5);
			}
			if (!Ch.IsSub(Who, In.World))
			{
				Subscribe(Ch, In, Who, false, 1);
			}
		}
		CheerDebt += (SumL * 0.015 + (Strangers + Lurkers) * 0.001) * (1.0 + Hype / 50.0) * DWorld / 60.0;
		while (CheerDebt >= 1.0)
		{
			CheerDebt -= 1.0;
			static const int Amounts[] = {100, 100, 100, 100, 100, 200, 200, 300, 500, 500, 1000, 2500};
			const int Bits = Amounts[R.Int(12)];
			const std::string Who = GiverIn(1.0);
			Earn(Ch, Bits, 1);
			if (Member* M = Ch.FindMember(Who))
			{
				M->Given += Bits;
			}
			Alert A;
			A.Kind = AlertKind::Cheer;
			A.Who = Who;
			A.Count = Bits;
			A.Cents = Bits;
			A.Text = R.Pick(kast_detail::Cheers());
			Queue(A);
			Say(In, Who, "Cheer" + std::to_string(Bits) + " " + A.Text, LineKind::Cheer, Ch.IsSub(Who, In.World) ? BadgeSub : 0, kast_detail::NameColor(Who), Bits);
		}
		if (In.World - LastAdWorld >= 60.0 && !AdRunning(In.Real))
		{
			Say(In, "", "Kast ran a scheduled ad break.", LineKind::System, 0, 0x94a3b8);
			RunAd(Ch, In, 90);
		}
	}
	TipDebt += (SumL * 0.012 + (Strangers + Lurkers) * 0.0006) * (1.0 + Hype / 50.0) * (1.0 + 0.5 * Engage) * (1.0 + In.Gear.Tips) * DWorld / 60.0;
	while (TipDebt >= 1.0)
	{
		TipDebt -= 1.0;
		static const int Cents[] = {100, 100, 100, 200, 200, 300, 500, 500, 500, 1000, 1000, 2000, 5000};
		const Chips Amount = Cents[R.Int(13)];
		const std::string Who = GiverIn(1.0);
		std::string Msg = kast_detail::Fill(R.Pick(kast_detail::Tips()), "{hero}", In.Hero);
		if (In.Gear.MicTier == 0 && R.Chance(0.3))
		{
			Msg = "buy a real mic with this";
		}
		Earn(Ch, Amount, 2);
		if (Member* M = Ch.FindMember(Who))
		{
			M->Given += Amount;
		}
		Alert A;
		A.Kind = AlertKind::Tip;
		A.Who = Who;
		A.Cents = Amount;
		A.Text = Msg;
		Queue(A);
		Say(In, Who, Money(Amount) + ": " + Msg, LineKind::Tip, Ch.IsSub(Who, In.World) ? BadgeSub : 0, kast_detail::NameColor(Who), Amount);
		Hype = std::min(100.0, Hype + std::min(15.0, static_cast<double>(Amount) / 500.0));
	}
	// The very first stream: Dee is watching.
	if (Ch.Streams == 1 && Ch.EarnedTips == 0 && In.World - StartWorld > 8.0)
	{
		const Chips First = 500;
		Earn(Ch, First, 2);
		Alert A;
		A.Kind = AlertKind::Tip;
		A.Who = "dee_spincycle";
		A.Cents = First;
		A.Text = "first stream!! proud of you. don't punt";
		Queue(A);
		Say(In, A.Who, Money(First) + ": " + A.Text, LineKind::Tip, 0, 0xf2a541, First);
		Hype = std::min(100.0, Hype + 8.0);
	}
	SponsorDebt += DWorld / 60.0;
	if (SponsorDebt >= 0.1)
	{
		for (Deal& D : Ch.Deals)
		{
			const Sponsor* S = D.Until > In.World ? FindSponsor(D.Id) : nullptr;
			if (S)
			{
				const Chips Cents2 = static_cast<Chips>(std::round(static_cast<double>(S->PerHourCents) * SponsorDebt));
				D.EarnedCents += Cents2;
				Earn(Ch, Cents2, 4);
			}
		}
		SponsorDebt = 0.0;
	}

	// The small-channel network: the ones you've raided raid you back.
	int Owed = 0;
	const std::vector<SmallChannel> Net = Network(In.World);
	for (const SmallChannel& S : Net)
	{
		const auto It = Ch.Goodwill.find(S.Name);
		Owed += It != Ch.Goodwill.end() ? std::min(3, It->second) : 0;
	}
	RaidDebt += DWorld / 60.0 * (0.01 + 0.03 * static_cast<double>(std::min(10, Owed))) * std::min(2.0, Ch.Discoverability());
	if (RaidDebt >= 1.0 && !Net.empty())
	{
		RaidDebt = 0.0;
		// A friend in the network first, else whoever's ending their stream.
		const SmallChannel* Raider = &Net[static_cast<size_t>(R.Int(static_cast<int>(Net.size())))];
		for (const SmallChannel& S : Net)
		{
			const auto It = Ch.Goodwill.find(S.Name);
			if (It != Ch.Goodwill.end() && R.Chance(0.6))
			{
				Raider = &S;
				break;
			}
		}
		const int Size2 = std::max(1, static_cast<int>(std::round(static_cast<double>(Raider->Viewers) * R.Range(0.5, 0.9))));
		RaidViewers += static_cast<double>(Size2);
		FollowDebt += static_cast<double>(Size2) * 0.06;
		LastRaider = Raider->Name;
		++Ch.RaidsIn;
		++Tonight.Raids;
		Hype = std::min(100.0, Hype + 10.0);
		Alert A;
		A.Kind = AlertKind::Raid;
		A.Who = Raider->Name;
		A.Count = Size2;
		Queue(A);
		Say(In, "", Raider->Name + " is raiding with " + Grouped(Size2) + (Size2 == 1 ? " viewer!" : " viewers!"), LineKind::Raid, 0, Raider->Color);
	}
	else if (RaidDebt >= 1.0)
	{
		RaidDebt = 0.0;
	}

	// Chat: the room talks at the room's size.
	double Talk = 0.0;
	for (const int I : Here)
	{
		Talk += 0.2 + 1.0 * Ch.Members[static_cast<size_t>(Visits[static_cast<size_t>(I)].Member)].Loyalty;
	}
	Talk += Strangers * 0.1 + Lurkers * 0.03 + RaidViewers * 0.15;
	const double ChatRate = std::min(10.0, Talk / 60.0 * (1.0 + Hype / 40.0) * (0.8 + 0.4 * Engage)) * (AdRunning(In.Real) ? 0.6 : 1.0);
	ChatDebt = std::min(6.0, ChatDebt + ChatRate * DReal);
	while (ChatDebt >= 1.0)
	{
		ChatDebt -= 1.0;
		int Badges = 0;
		uint32_t Color = 0;
		const std::string Who = Chatter(Ch, In, Badges, Color);
		if (TimedOut.count(Who) && TimedOut[Who] > In.Real)
		{
			continue;
		}
		LineKind Kind = LineKind::Chat;
		std::string Line = LineFor(Ch, In, Kind);
		Line = kast_detail::Fill(Line, "{hero}", In.Hero);
		Line = kast_detail::Fill(Line, "{tables}", std::to_string(std::max(1, In.Tables)));
		Line = kast_detail::Fill(Line, "{bb}", std::to_string(static_cast<int>(std::round(In.StackBb))));
		Line = kast_detail::Fill(Line, "{country}", kast_detail::CountryName(handles::PickCountry(R)));
		Line = kast_detail::Fill(Line, "{event}", In.EventName);
		Line = kast_detail::Fill(Line, "{tilt}", Ch.Affiliate && (Badges & BadgeSub) ? Prefix + "Tilt" : "kastSalt");
		Line = kast_detail::Fill(Line, "{ship}", Ch.Affiliate && (Badges & BadgeSub) ? Prefix + "Ship" : "kastHype");
		Line = kast_detail::Fill(Line, "{rent}", Ch.Affiliate && (Badges & BadgeSub) ? Prefix + "Rent" : "kastChip");
		Line = kast_detail::Fill(Line, "{pct}", std::to_string(70 + R.Int(20)));
		Line = kast_detail::Fill(Line, "{who}", "him");
		Line = kast_detail::Fill(Line, "{prize}", "$$$");
		if (In.Rank > 0 && Line == "rank?")
		{
			Line = "rank? " + std::to_string(In.Rank) + "/" + std::to_string(In.Remaining) + " kastPog";
		}
		Say(In, Who, Line, Kind, Badges, Color);
	}
	// Questions, mostly from people still deciding whether to stay.
	QuestionDebt = std::min(2.0, QuestionDebt + std::min(0.03, 0.0015 * (Strangers + static_cast<double>(Present) * 0.4 + RaidViewers * 0.5)) * DReal * (Ch.Title == 2 || Ch.Title == 3 ? 1.6 : 1.0));
	if (QuestionDebt >= 1.0)
	{
		QuestionDebt -= 1.0;
		int Badges = 0;
		uint32_t Color = 0;
		const std::string Who = Chatter(Ch, In, Badges, Color);
		Say(In, Who, R.Pick(kast_detail::Questions()), LineKind::Question, Badges, Color);
	}
	TrollStep(Ch, In, DReal);
	for (ChatMsg& M : Chat)
	{
		if (M.Deleted || (!M.Toxic && !M.Spam) || M.At > In.Real)
		{
			continue;
		}
		if (M.DeleteAt >= 0.0 && In.Real >= M.DeleteAt)
		{
			M.Deleted = true;
			++Caught;
			TimedOut[M.Who] = In.Real + 600.0;
			for (Moderator& Mod : Ch.Mods)
			{
				Mod.Actions += Mod.Name == M.DeletedBy ? 1 : 0;
			}
			const std::string By = M.DeletedBy;
			const std::string Who = M.Who;
			Say(In, "", Who + " was timed out by " + By + ".", LineKind::System, 0, By == "WardenBot" ? 0x7c3aed : 0x22c55e);
		}
		else if (M.DeleteAt == -2.0 && In.Real - M.At > 8.0)
		{
			M.DeleteAt = -3.0;
			++Missed;
			Health = std::max(0.0, Health - (M.Toxic ? 0.05 : 0.03));
		}
	}

	// Predictions take points for two minutes.
	if (Pred.Active && !Pred.Resolved && In.Real - Pred.At < 120.0)
	{
		PredDebt += Viewers * DReal * 0.05;
		while (PredDebt >= 1.0)
		{
			PredDebt -= 1.0;
			const int Points = (1 + R.Int(10)) * 100;
			(R.Chance(0.45 + Hype / 400.0) ? Pred.Yes : Pred.No) += Points;
		}
	}

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
	CheckGrowth(Ch, In);
}
} // namespace kast
} // namespace ss
