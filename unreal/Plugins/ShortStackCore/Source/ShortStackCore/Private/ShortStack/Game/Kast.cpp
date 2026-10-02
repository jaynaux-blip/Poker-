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
		return Dark;
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

/** Followers who come back when the channel goes live: a few percent, less as the channel gets huge. */
double ReturningViewers(int Followers, double Tod, double Q)
{
	const double F = static_cast<double>(Followers);
	return F * 0.032 / std::sqrt(1.0 + F / 40000.0) * Tod * (0.55 + 0.9 * Q);
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
		auto Add = [&](const char* Id, const char* Brand, const char* Product, const char* Pitch, const char* Read, uint32_t Color, int Followers, bool Partner, double PerHour, double ReadPay, int Days) {
			Sponsor S;
			S.Id = Id;
			S.Brand = Brand;
			S.Product = Product;
			S.Pitch = Pitch;
			S.ReadLine = Read;
			S.Color = Color;
			S.Followers = Followers;
			S.NeedsPartner = Partner;
			S.PerHourCents = static_cast<Chips>(PerHour * 100.0 + 0.5);
			S.ReadCents = static_cast<Chips>(ReadPay * 100.0 + 0.5);
			S.Days = Days;
			V.push_back(S);
		};
		Add("overclock", "Overclock", "Overclock Energy, zero sugar",
			"Hey! We love the late-night grind energy. $4 for every hour you're live with a can on the desk, plus $10 a read. 30 days.",
			"This hand is brought to you by Overclock Energy. Zero sugar, all-night focus. Code GRIND in the panels.", 0x84cc16, 150, false, 4.0, 10.0, 30);
		Add("tunnelrat", "TunnelRat VPN", "TunnelRat VPN",
			"Your chat trusts you. $8 an hour live and $25 a read to tell them about TunnelRat. 30 days.",
			"Quick one: TunnelRat VPN keeps your connection private wherever you play. Link below, first month free.", 0x38bdf8, 600, false, 8.0, 25.0, 30);
		Add("stacked", "Stacked Apparel", "Stacked hoodies and caps",
			"We'd love to see you grind in Stacked. $12 an hour live, $40 a read, and a box of merch. 30 days.",
			"Hoodie's from Stacked Apparel. Built for twelve-hour sessions. Code in the panels for 20% off.", 0xf59e0b, 1500, false, 12.0, 40.0, 30);
		Add("riverline", "RiverLine", "Team RiverLine streamer contract",
			"You've built something. RiverLine would like you on Team RiverLine: $25 an hour streaming RiverLine, $60 a read, and the Team patch on your avatar. 30 days.",
			"Everything you see tonight is on RiverLine. New players: the welcome freeroll is in my panels. Team RiverLine!", 0x27d3c3, 2500, true, 25.0, 60.0, 30);
		Add("sitwell", "Sitwell", "Sitwell Pro gaming chair",
			"Ten thousand followers sit with you every night. Sitwell would like to be the chair. $35 an hour, $90 a read. 30 days.",
			"Twelve hours in and my back is fine. That's the Sitwell Pro. Link's below.", 0xfb923c, 10000, true, 35.0, 90.0, 30);
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

void Offline(Channel& Ch, double World, Rng& R)
{
	if (World <= Ch.LastOffline)
	{
		return;
	}
	Ch.LastOffline = World;
	// Subscriptions renew every thirty days; most of them do.
	const double Share = Ch.Partner ? 0.7 : 0.5;
	for (Subscriber& S : Ch.Subs)
	{
		while (S.Renews > 0.0 && S.Renews <= World)
		{
			const bool Stays = R.Chance(S.Gift ? 0.3 : 0.7);
			if (!Stays)
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
	// Lapsed subscribers are forgotten after a while, so the list stays short.
	Ch.Subs.erase(std::remove_if(Ch.Subs.begin(), Ch.Subs.end(), [&](const Subscriber& S) { return S.Renews < 0.0 && -S.Renews < World - 30.0 * kast_detail::KastDay; }), Ch.Subs.end());
	// Clips keep getting watched for a few days; some of those viewers follow.
	double Gained = 0.0;
	for (Clip& C : Ch.Clips)
	{
		const double Was = C.Views;
		C.Views = C.Reach * (1.0 - std::exp(-(World - C.At) / (1.5 * kast_detail::KastDay)));
		Gained += std::max(0.0, C.Views - Was);
	}
	Ch.FollowFrac += Gained * 0.004;
	const int Whole = static_cast<int>(std::floor(Ch.FollowFrac));
	Ch.Followers += Whole;
	Ch.FollowFrac -= static_cast<double>(Whole);
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

double Stream::Target(const Channel& Ch, const Inputs& In) const
{
	double Fresh = 0.0;
	return Audience(Ch, In, Fresh);
}

double Stream::Audience(const Channel& Ch, const Inputs& In, double& Fresh) const
{
	const double Q = Quality(In);
	const TitleSpec& T = Titles()[static_cast<size_t>(std::max(0, std::min(static_cast<int>(Titles().size()) - 1, Ch.Title)))];
	const double Tod = kast_detail::TimeOfDay(In.World);
	const double Returning = kast_detail::ReturningViewers(Ch.Followers, Tod, Q);
	double Discover = (1.0 + 12.0 * std::pow(Q, 1.2)) * kast_detail::Competition(In.World) * (1.0 + Hype / 35.0) * T.Discover;
	Discover += 0.08 * Viewers / (1.0 + Viewers / 5000.0); // the directory sorts by viewers: a bigger stream is easier to find (up to a point)
	double Mult = 1.0 + 0.12 * static_cast<double>(std::min(3, std::max(0, In.Tables - 1)));
	Mult *= In.AtTable ? 1.0 : In.Results ? 0.8 : 0.55;
	Mult *= In.Sprinting ? 0.85 : 1.0;
	Mult *= AdRunning(In.Real) ? 0.85 : 1.0;
	Mult *= 0.55 + 0.45 * Health;
	Mult *= 1.0 + 0.15 * Engage;
	Fresh = Discover * Mult + RaidViewers;
	return (Returning + Discover) * Mult + RaidViewers;
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
	std::string Name;
	const double RegularShare = std::min(0.55, 0.12 + static_cast<double>(Ch.Followers) / 2500.0);
	if (!OnlineMods.empty() && R.Chance(0.08))
	{
		Name = R.Pick(OnlineMods);
	}
	else if (!Ch.Regulars.empty() && R.Chance(RegularShare))
	{
		// The most active regulars talk most (the ranking is refreshed every few seconds).
		if (TopRegulars.empty() || In.Real - TopAt > 5.0 || In.Real < TopAt)
		{
			TopAt = In.Real;
			std::vector<std::pair<int, std::string>> Top;
			for (const auto& Rg : Ch.Regulars)
			{
				Top.push_back({Rg.second, Rg.first});
			}
			std::sort(Top.begin(), Top.end(), [](const std::pair<int, std::string>& A, const std::pair<int, std::string>& B) { return A.first != B.first ? A.first > B.first : A.second < B.second; });
			TopRegulars.clear();
			for (size_t I = 0; I < Top.size() && I < 80; ++I)
			{
				TopRegulars.push_back(Top[I].second);
			}
		}
		const int N = std::min(static_cast<int>(TopRegulars.size()), 12 + Ch.Followers / 200);
		const double U = R.Next();
		Name = TopRegulars[static_cast<size_t>(std::min(N - 1, static_cast<int>(U * U * static_cast<double>(N))))];
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
	const auto It = Ch.Regulars.find(Name);
	if (It != Ch.Regulars.end() && It->second >= 40 && !(Badges & BadgeMod))
	{
		Badges |= BadgeVip;
	}
	Color = kast_detail::NameColor(Name);
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

void Stream::Follow(Channel& Ch, const Inputs& In, int Count)
{
	if (Count <= 0)
	{
		return;
	}
	const int Before = Ch.Followers;
	Ch.Followers += Count;
	Tonight.Follows += Count;
	Alert A;
	A.Kind = AlertKind::Follow;
	A.Count = Count;
	if (Count == 1)
	{
		A.Who = Pool[static_cast<size_t>(R.Int(static_cast<int>(Pool.size())))];
	}
	Queue(A);
	const int Rival = 2400;
	if (Before < Rival && Ch.Followers >= Rival)
	{
		Notice N;
		N.Type = Notice::Kind::Text;
		N.From = "gh0stfold";
		N.Body = "saw you passed me on kast. cute. see you at the tables.";
		Notices.push_back(N);
	}
	(void)In;
}

void Stream::Subscribe(Channel& Ch, const Inputs& In, const std::string& Who, bool Gift, int Count)
{
	const double Share = Ch.Partner ? 0.7 : 0.5;
	for (int K = 0; K < Count; ++K)
	{
		Subscriber S;
		S.Name = Gift ? Pool[static_cast<size_t>(R.Int(static_cast<int>(Pool.size())))] : Who;
		S.Since = In.World;
		S.Renews = In.World + 30.0 * kast_detail::KastDay;
		S.Gift = Gift;
		// Already subscribed (a resub): the months go on.
		bool Found = false;
		for (Subscriber& Old : Ch.Subs)
		{
			if (Old.Name == S.Name)
			{
				Old.Renews = std::max(std::fabs(Old.Renews), In.World) + 30.0 * kast_detail::KastDay;
				++Old.Months;
				Found = true;
				break;
			}
		}
		if (!Found)
		{
			Ch.Subs.push_back(S);
		}
		Earn(Ch, static_cast<Chips>(std::round(static_cast<double>(SubPriceCents) * Share)), 0);
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
		static const std::vector<std::string> Msgs = {"", "love the stream", "rent fund", "finally subbed", "for the grind", "kastLove", "", "deep run tonight"};
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
		Ch.Regulars[Who] += 1;
		Seen.insert(Who);
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
	const kast_detail::Hit H = kast_detail::HitOf(M);
	const double Chance = std::min(0.95, H.ClipChance * std::min(1.0, 0.25 + Viewers / 60.0));
	if (!R.Chance(Chance))
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
	double Reach = (40.0 + Viewers * 6.0) * Spread * (1.0 + Hype / 40.0) * std::max(0.5, Size);
	const bool Viral = R.Chance(M == Moment::Win || M == Moment::BadBeat ? 0.06 : 0.025);
	Reach *= Viral ? 25.0 : 1.0;
	C.Reach = Reach;
	Ch.Clips.insert(Ch.Clips.begin(), C);
	if (Ch.Clips.size() > 16)
	{
		// Keep the most watched.
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
			Hype = std::min(100.0, Hype + 10.0);
			Say(In, "", "Milestone: " + Grouped(Step) + " followers!", LineKind::System, 0, Lime);
		}
	}
	const double Minutes = Ch.MinutesLive + (Live ? In.World - StartWorld : 0.0);
	if (!Ch.Affiliate && Ch.Followers >= AffiliateFollowers && Minutes >= AffiliateMinutes && Ch.Streams >= AffiliateStreams)
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
	if (Ch.Affiliate && !Ch.Partner && Ch.Followers >= PartnerFollowers)
	{
		const double Avg = Live && In.World - StartWorld > 30.0 ? std::max(Ch.AvgViewers(), ViewerMinutes / (In.World - StartWorld)) : Ch.AvgViewers();
		if (Avg >= PartnerAvgViewers)
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
	}
	for (const Sponsor& S : Sponsors())
	{
		if (Ch.Followers >= S.Followers && (!S.NeedsPartner || Ch.Partner) && !Ch.Offers.count(S.Id) && !Ch.Declined.count(S.Id) && !Ch.ActiveDeal(S.Id, In.World))
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
		std::set<std::string> Taken = {In.Hero};
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
	Live = true;
	StartWorld = In.World;
	StartReal = In.Real;
	Viewers = 1.0 + static_cast<double>(Ch.Followers) * 0.006;
	Peak = static_cast<int>(Viewers);
	Hype = 15.0;
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
	TopRegulars.clear();
	Caught = 0;
	Missed = 0;
	ChatDebt = TrollDebt = TipDebt = CheerDebt = SubDebt = QuestionDebt = GraphDebt = ViewerMinutes = RaidDebt = SponsorDebt = PredDebt = 0.0;
	GreetDebt = 3.0;
	OnlineMods.clear();
	for (const Moderator& M : Ch.Mods)
	{
		if (R.Chance(M.Online))
		{
			OnlineMods.push_back(M.Name);
		}
	}
	++Ch.Streams;
	Say(In, "", "You're live! " + (Ch.Followers > 0 ? Grouped(Ch.Followers) + (Ch.Followers == 1 ? " follower was" : " followers were") + " notified." : std::string("Share the link and say hi to chat.")),
		LineKind::System, 0, Violet);
	if (!OnlineMods.empty())
	{
		Say(In, "", std::to_string(OnlineMods.size()) + (OnlineMods.size() == 1 ? " moderator is" : " moderators are") + " watching chat.", LineKind::System, 0, 0x22c55e);
	}
	if (Ch.Streams == 1)
	{
		Notice N;
		N.Type = Notice::Kind::Text;
		N.From = "Kast";
		N.Body = "Your first stream is live! Tip: streamers grow by playing big moments, talking to chat and looking and sounding good. 50 followers and 8 hours live unlock Affiliate.";
		Notices.push_back(N);
	}
	CheckGrowth(Ch, In);
}

Summary Stream::Stop(Channel& Ch, const Inputs& In)
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
	Ch.MinutesLive += S.Minutes;
	Ch.ViewerMinutes += ViewerMinutes;
	Ch.Peak = std::max(Ch.Peak, Peak);
	StreamLog Lg;
	Lg.Start = StartWorld;
	Lg.Minutes = S.Minutes;
	Lg.Avg = S.Avg;
	Lg.Peak = S.Peak;
	Lg.Follows = S.Follows;
	Lg.Subs = S.Subs + S.Gifted;
	Lg.Cents = S.Total();
	Lg.Title = Titles()[static_cast<size_t>(std::max(0, std::min(static_cast<int>(Titles().size()) - 1, Ch.Title)))].Text;
	if (S.Minutes >= 1.0)
	{
		Ch.Log.insert(Ch.Log.begin(), Lg);
		if (Ch.Log.size() > 30)
		{
			Ch.Log.pop_back();
		}
	}
	// Regulars: keep the sixty most active.
	if (Ch.Regulars.size() > 120)
	{
		std::vector<std::pair<int, std::string>> Top;
		for (const auto& Rg : Ch.Regulars)
		{
			Top.push_back({Rg.second, Rg.first});
		}
		std::sort(Top.begin(), Top.end(), [](const std::pair<int, std::string>& A, const std::pair<int, std::string>& B) { return A.first != B.first ? A.first > B.first : A.second < B.second; });
		Ch.Regulars.clear();
		for (size_t I = 0; I < 60; ++I)
		{
			Ch.Regulars[Top[I].second] = Top[I].first;
		}
	}
	CheckGrowth(Ch, In);
	Live = false;
	AdUntil = -1.0;
	Alerts.clear();
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
	const TitleSpec& T = Titles()[static_cast<size_t>(std::max(0, std::min(static_cast<int>(Titles().size()) - 1, Ch.Title)))];
	Hype = std::min(100.0, Hype + H.Hype * T.Hype * std::max(0.5, std::min(2.0, Size)));
	LastMoment = M;
	LastMomentReal = In.Real;
	if (H.Face != Mood::Focus || M == Moment::BestPlay)
	{
		Face = H.Face;
		FaceAt = In.Real;
	}
	// The more people watching, the bigger the wave in chat.
	const int Lines = std::min(14, H.Lines + static_cast<int>(std::sqrt(Viewers) / 3.0));
	React(Ch, In, M, Lines);
	if (M == Moment::Knockout && !Detail.empty())
	{
		Chat.back().Text = "bye " + Detail + " kastGG";
	}
	MakeClip(Ch, In, M, Detail, Size);
	// Deep runs draw raids: sometimes from the biggest names in the directory.
	if (M == Moment::FinalTable || M == Moment::Win || M == Moment::InTheMoney)
	{
		const double Chance = M == Moment::Win ? 0.55 : M == Moment::FinalTable ? 0.4 : 0.12;
		if (R.Chance(Chance))
		{
			std::vector<const Streamer*> Live2;
			for (const Streamer& S : Directory())
			{
				if (!S.Rival && ViewersNow(S, In.World) > 0)
				{
					Live2.push_back(&S);
				}
			}
			if (!Live2.empty())
			{
				// Big names raid the moments that matter; smaller channels raid anything.
				const Streamer* From = Live2[static_cast<size_t>(R.Int(static_cast<int>(Live2.size())))];
				if (M == Moment::InTheMoney)
				{
					for (const Streamer* S : Live2)
					{
						From = ViewersNow(*S, In.World) < ViewersNow(*From, In.World) ? S : From;
					}
				}
				const double Size2 = static_cast<double>(ViewersNow(*From, In.World)) * R.Range(0.12, 0.35);
				RaidViewers += Size2 * 0.85;
				Viewers += Size2 * 0.85;
				LastRaider = From->Name;
				++Ch.RaidsIn;
				++Tonight.Raids;
				Hype = std::min(100.0, Hype + 25.0);
				Alert A;
				A.Kind = AlertKind::Raid;
				A.Who = From->Name;
				A.Count = static_cast<int>(std::round(Size2));
				Queue(A);
				Say(In, "", From->Name + " is raiding with " + Grouped(A.Count) + " viewers!", LineKind::Raid, 0, From->Color);
				static const std::vector<std::string> RaidLines = {"RAID kastHype", "{raider} sent us!", "hi from {raider}'s stream", "kastPog kastPog", "raid hype", "GL on the run!", "we're here to sweat"};
				for (int K = 0; K < 6; ++K)
				{
					const std::string Who = Pool[static_cast<size_t>(R.Int(static_cast<int>(Pool.size())))];
					Say(In, Who, kast_detail::Fill(R.Pick(RaidLines), "{raider}", From->Name), LineKind::Chat, 0, kast_detail::NameColor(Who));
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
	for (int K = 0; K < 3; ++K)
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
			for (int K = 0; K < 3; ++K)
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
	static const std::vector<std::string> Back = {"kastLove", "<3", "love you too", "ty for streaming", "kastLove kastLove", "best streamer"};
	for (int K = 0; K < 2 + static_cast<int>(std::sqrt(Viewers) / 4.0) && K < 6; ++K)
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
			static const std::vector<std::string> Thanks = {"thanks!", "ty kastLove", "makes sense", "good answer", "appreciate it"};
			Say(In, Who, R.Pick(Thanks), LineKind::Chat, Ch.IsSub(Who, In.World) ? BadgeSub : 0, kast_detail::NameColor(Who));
			Chat.back().At = In.Real + 1.2;
			Ch.Regulars[Who] += 2;
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
				if (R.Chance(0.6))
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
				// Timing out a regular for nothing costs goodwill.
				Health = std::max(0.0, Health - 0.06);
				Ch.Regulars.erase(Who);
				int Badges = 0;
				uint32_t Color = 0;
				const std::string Fan = Chatter(Ch, In, Badges, Color);
				Say(In, Fan, "why did " + Who + " get timed out?? kastLUL", LineKind::Chat, Badges, Color);
				Chat.back().At = In.Real + 0.8;
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
	const auto It = Ch.Regulars.find(Name);
	M.Online = std::min(0.9, 0.5 + (It != Ch.Regulars.end() ? static_cast<double>(It->second) / 200.0 : 0.0));
	Ch.Mods.push_back(M);
	if (Live)
	{
		OnlineMods.push_back(Name);
		Say(In, "", Name + " is now a moderator.", LineKind::System, 0, 0x22c55e);
		static const std::vector<std::string> Lines2 = {"congrats {who} kastClap", "mod {who} kastHype", "power trip incoming kastLUL", "well deserved"};
		int Badges = 0;
		uint32_t Color = 0;
		const std::string Fan = Chatter(Ch, In, Badges, Color);
		Say(In, Fan, kast_detail::Fill(R.Pick(Lines2), "{who}", Name), LineKind::Chat, Badges, Color);
		Chat.back().At = In.Real + 0.7;
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
	std::vector<std::pair<int, std::string>> Top;
	for (const auto& Rg : Ch.Regulars)
	{
		if (Rg.second >= 6 && !Ch.IsMod(Rg.first))
		{
			Top.push_back({Rg.second + (Ch.IsSub(Rg.first, World) ? 10 : 0), Rg.first});
		}
	}
	std::sort(Top.begin(), Top.end(), [](const std::pair<int, std::string>& A, const std::pair<int, std::string>& B) { return A.first != B.first ? A.first > B.first : A.second < B.second; });
	std::vector<std::string> Out;
	for (size_t I = 0; I < Top.size() && I < 5; ++I)
	{
		Out.push_back(Top[I].second);
	}
	return Out;
}

void Stream::TrollStep(Channel& Ch, const Inputs& In, double DReal)
{
	const TitleSpec& T = Titles()[static_cast<size_t>(std::max(0, std::min(static_cast<int>(Titles().size()) - 1, Ch.Title)))];
	const double Rate = (0.004 + 0.00012 * Viewers) * (1.0 + Hype / 50.0) * T.Calm * (Ch.Partner ? 1.1 : 1.0);
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
		if (Ch.IsMod(Who) || Ch.Regulars.count(Who))
		{
			continue; // regulars don't troll
		}
		Say(In, Who, IsSpam ? R.Pick(kast_detail::Spam()) : kast_detail::Fill(R.Pick(kast_detail::Trolls()), "{hero}", In.Hero), LineKind::Chat, 0, kast_detail::NameColor(Who));
		ChatMsg& M = Chat.back();
		M.Toxic = !IsSpam;
		M.Spam = IsSpam;
		// The bot catches what it can at once; mods take a moment to notice.
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
			// The streamer may still click it away; if not, it costs a little chat health when it lands (Tick).
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
	const TitleSpec& T = Titles()[static_cast<size_t>(std::max(0, std::min(static_cast<int>(Titles().size()) - 1, Ch.Title)))];

	// Hype and raids fade on the clock; talking to chat fades in real time.
	Hype *= std::exp(-DWorld / 12.0);
	RaidViewers *= std::exp(-DWorld / 20.0);
	Engage *= std::exp(-DReal / 90.0);
	Health = std::min(1.0, Health + DReal / 60.0 * 0.03);

	// A bigger audience brings more names into chat.
	while (Pool.size() < 6000 && static_cast<double>(Pool.size()) < 400.0 + Viewers * 3.0 + static_cast<double>(Ch.Subs.size()) * 1.5)
	{
		const std::string Name = handles::Make(R, handles::PickCountry(R));
		if (Name != In.Hero && std::find(Pool.end() - std::min<std::ptrdiff_t>(static_cast<std::ptrdiff_t>(Pool.size()), 400), Pool.end(), Name) == Pool.end())
		{
			Pool.push_back(Name);
		}
	}

	// Viewers drift toward what the show deserves.
	double Fresh = 0.0;
	const double Goal = Audience(Ch, In, Fresh);
	Viewers += (Goal - Viewers) * (1.0 - std::exp(-DWorld / 5.0));
	Viewers = std::max(0.0, Viewers);
	Peak = std::max(Peak, static_cast<int>(std::round(Viewers)));
	ViewerMinutes += Viewers * DWorld;
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

	// New viewers follow; returning ones already have.
	const double NewShare = Goal > 0.0 ? std::min(1.0, Fresh / Goal) : 0.0;
	const double FollowRate = (0.25 + 0.9 * Q) * (1.0 + In.Gear.Follow) * T.Follow * (1.0 + Hype / 60.0) * (0.5 + 0.5 * Health) * (1.0 + 0.3 * Engage) * (In.Sprinting ? 0.6 : 1.0);
	Ch.FollowFrac += Viewers * NewShare * FollowRate * DWorld / 60.0;
	if (Ch.FollowFrac >= 1.0)
	{
		const int Whole = static_cast<int>(std::floor(Ch.FollowFrac));
		Ch.FollowFrac -= static_cast<double>(Whole);
		Follow(Ch, In, Whole);
	}

	// Money: subs, cheers and ads once affiliate; tips from the start; sponsors by the hour.
	if (Ch.Affiliate)
	{
		SubDebt += Viewers * 0.006 * (1.0 + Q) * (1.0 + Hype / 50.0) * (Ch.Partner ? 1.1 : 1.0) * DWorld / 60.0;
		while (SubDebt >= 1.0)
		{
			SubDebt -= 1.0;
			int Badges = 0;
			uint32_t Color = 0;
			std::string Who = Chatter(Ch, In, Badges, Color);
			for (int Try = 0; Try < 4 && Ch.IsSub(Who, In.World); ++Try)
			{
				Who = Pool[static_cast<size_t>(R.Int(static_cast<int>(Pool.size())))];
			}
			if (Hype > 25.0 && R.Chance(0.12))
			{
				const int Gifts = Viewers > 400 && R.Chance(0.3) ? 20 : Viewers > 120 && R.Chance(0.4) ? 10 : R.Chance(0.5) ? 5 : 1;
				Subscribe(Ch, In, Who, true, Gifts);
			}
			else
			{
				Subscribe(Ch, In, Who, false, 1);
			}
		}
		CheerDebt += Viewers * 0.00015 * (1.0 + Hype / 40.0) * DWorld;
		while (CheerDebt >= 1.0)
		{
			CheerDebt -= 1.0;
			static const int Amounts[] = {100, 100, 100, 100, 100, 200, 200, 300, 500, 500, 1000, 2500};
			const int Bits = Amounts[R.Int(12)];
			int Badges = 0;
			uint32_t Color = 0;
			const std::string Who = Chatter(Ch, In, Badges, Color);
			Earn(Ch, Bits, 1);
			Alert A;
			A.Kind = AlertKind::Cheer;
			A.Who = Who;
			A.Count = Bits;
			A.Cents = Bits;
			A.Text = R.Pick(kast_detail::Cheers());
			Queue(A);
			Say(In, Who, "Cheer" + std::to_string(Bits) + " " + A.Text, LineKind::Cheer, Badges, Color, Bits);
		}
		if (In.World - LastAdWorld >= 60.0 && !AdRunning(In.Real))
		{
			Say(In, "", "Kast ran a scheduled ad break.", LineKind::System, 0, 0x94a3b8);
			RunAd(Ch, In, 90);
		}
	}
	TipDebt += Viewers * 0.00025 * (1.0 + Hype / 40.0) * (1.0 + 0.5 * Engage) * DWorld;
	while (TipDebt >= 1.0)
	{
		TipDebt -= 1.0;
		static const int Cents[] = {100, 100, 100, 200, 200, 300, 500, 500, 500, 1000, 1000, 2000, 5000};
		Chips Amount = Cents[R.Int(13)];
		if (R.Chance(0.002 + std::min(0.004, Viewers / 250000.0)))
		{
			Amount = 25000 + 25000 * R.Int(4); // a whale
		}
		int Badges = 0;
		uint32_t Color = 0;
		const std::string Who = Chatter(Ch, In, Badges, Color);
		std::string Msg = kast_detail::Fill(R.Pick(kast_detail::Tips()), "{hero}", In.Hero);
		if (In.Gear.MicTier == 0 && R.Chance(0.3))
		{
			Msg = "buy a real mic with this";
		}
		Earn(Ch, Amount, 2);
		Alert A;
		A.Kind = AlertKind::Tip;
		A.Who = Who;
		A.Cents = Amount;
		A.Text = Msg;
		Queue(A);
		Say(In, Who, Money(Amount) + ": " + Msg, LineKind::Tip, Badges, Color, Amount);
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

	// Small channels raid each other.
	RaidDebt += DWorld * 0.0008 * (Ch.Followers >= 30 ? 1.0 : 0.0) * (1.0 + Hype / 30.0);
	if (RaidDebt >= 1.0)
	{
		RaidDebt = 0.0;
		std::vector<const Streamer*> Small;
		for (const Streamer& S : Directory())
		{
			const int V = ViewersNow(S, In.World);
			if (!S.Rival && V > 0 && V < 600)
			{
				Small.push_back(&S);
			}
		}
		int Size2 = 6 + R.Int(30);
		std::string From = Pool[static_cast<size_t>(R.Int(static_cast<int>(Pool.size())))];
		if (!Small.empty())
		{
			const Streamer& Raider = *Small[static_cast<size_t>(R.Int(static_cast<int>(Small.size())))];
			const double Share = R.Range(0.2, 0.5);
			Size2 = std::max(3, static_cast<int>(static_cast<double>(ViewersNow(Raider, In.World)) * Share));
			From = Raider.Name;
		}
		RaidViewers += static_cast<double>(Size2) * 0.8;
		Viewers += static_cast<double>(Size2) * 0.8;
		LastRaider = From;
		++Ch.RaidsIn;
		++Tonight.Raids;
		Hype = std::min(100.0, Hype + 12.0);
		Alert A;
		A.Kind = AlertKind::Raid;
		A.Who = From;
		A.Count = Size2;
		Queue(A);
		Say(In, "", From + " is raiding with " + Grouped(Size2) + " viewers!", LineKind::Raid, 0, Violet);
	}

	// Chat.
	const double ChatRate = std::min(10.0, (0.03 + 0.004 * std::pow(Viewers, 0.9)) * (1.0 + Hype / 25.0) * (0.7 + 0.6 * Engage)) * (AdRunning(In.Real) ? 0.6 : 1.0);
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
		Ch.Regulars[Who] += 1;
		Seen.insert(Who);
	}
	QuestionDebt = std::min(2.0, QuestionDebt + std::min(0.05, 0.006 + 0.0006 * Viewers) * DReal * (Ch.Title == 2 || Ch.Title == 3 ? 1.6 : 1.0));
	if (QuestionDebt >= 1.0)
	{
		QuestionDebt -= 1.0;
		int Badges = 0;
		uint32_t Color = 0;
		const std::string Who = Chatter(Ch, In, Badges, Color);
		Say(In, Who, R.Pick(kast_detail::Questions()), LineKind::Question, Badges, Color);
		Ch.Regulars[Who] += 1;
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
			// Nobody caught it: it sat in chat and people saw it.
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

	// Greet the first few in.
	GreetDebt -= DReal;
	if (GreetDebt <= 0.0 && In.Real - StartReal < 90.0)
	{
		GreetDebt = R.Range(4.0, 9.0);
		int Badges = 0;
		uint32_t Color = 0;
		const std::string Who = Chatter(Ch, In, Badges, Color);
		const std::string Hello = kast_detail::Fill(R.Pick(kast_detail::Greetings()), "{hero}", In.Hero);
		const std::string Place = kast_detail::CountryName(handles::PickCountry(R));
		Say(In, Who, kast_detail::Fill(Hello, "{country}", Place), LineKind::Chat, Badges, Color);
		Ch.Regulars[Who] += 1;
		Seen.insert(Who);
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
