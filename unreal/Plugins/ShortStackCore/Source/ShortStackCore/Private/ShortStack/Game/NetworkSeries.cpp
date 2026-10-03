// RiverLine's online series after RCOP 2026: nine a year, each with its own character. The flagships come back every
// year (Ring Rush with its Grand Circuit rings, The Championship Online with its bracelets, Summer Slam, Micro Madness,
// RCOP); the seasonal ones (winter, spring, the high rollers' July, the holidays) have a new name every time. No two
// events anywhere share a name: every event that isn't a Main Event or a High Roller has a name of its own.
#include "ShortStack/Game/Network.h"

#include "ShortStack/Game/World.h"
#include "ShortStack/Rng.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <set>

namespace ss
{
namespace net
{
namespace calendar_detail
{
const int FirstYear = 2027;
const int LastYear = 2040;

Chips SeriesCents(double Dollars)
{
	return static_cast<Chips>(std::llround(Dollars * 100.0));
}

int At(int H, int M)
{
	return H * 60 + M;
}

/** A guarantee as a lobby writes it: two significant figures. */
double Nice(double Dollars)
{
	if (Dollars < 100.0)
	{
		return std::round(Dollars);
	}
	const double Step = std::pow(10.0, std::floor(std::log10(Dollars)) - 1.0);
	return std::round(Dollars / Step) * Step;
}

struct Named
{
	const char* Name;
	const char* Short;
	const char* Tagline;
};

// The seasonal series: a new one every year.
const Named Winter[] = {
	{"Frostbite Open", "FROST", "Twelve days of frozen fingers and hot guarantees. Bundle up."},
	{"Black Ice Classic", "BLACK", "Looks like a smooth road. It isn't."},
	{"Polar Night Festival", "POLAR", "The sun barely rises. Nobody at the tables notices."},
	{"Whiteout Week", "WHITE", "You can't see the road. You can see the river."},
	{"The Ice Box", "ICE", "Keep it cool. Everyone else won't."},
	{"Frost Line Classic", "FLINE", "Below the line everything freezes but the action."},
	{"Northern Lights Open", "NORTH", "The sky puts on a show. So does the final table."},
	{"Snowblind Series", "SNOW", "Twelve days you won't see coming."},
	{"Glacier Games", "GLAC", "Slow, patient, unstoppable. Play like the ice."},
	{"The Long Night", "NIGHT", "The longest nights of the year, the deepest runs."},
	{"Deep Freeze Festival", "FREEZ", "Freezeouts, frostbite and nowhere to be until spring."},
	{"Avalanche Open", "AVAL", "It starts with one chip. Then everything comes down."},
	{"Permafrost Classic", "PERMA", "Some stacks never thaw."},
	{"Hailstorm Series", "HAIL", "Bounties falling out of the sky all week."},
};
const Named Spring[] = {
	{"Spring Fever Festival", "FEVER", "Windows open, guarantees up. The micros come back to life."},
	{"Bloom & Bust", "BLOOM", "Everything grows. Some of it gets cut down."},
	{"April Showdown", "APRIL", "Showers for some, flowers for the rest."},
	{"Equinox Open", "EQNX", "Equal day, equal night, equal stacks to start."},
	{"Thunderhead Classic", "THNDR", "Big clouds, big fields, bigger swings."},
	{"Petal to the Metal", "PETAL", "Spring's fastest structures, start to finish."},
	{"The Thaw", "THAW", "The ice breaks. The bankrolls run."},
	{"Rainmaker Series", "RAIN", "Somebody always makes it rain."},
	{"Greenlight Festival", "GREEN", "Every field says go."},
	{"May Day Melee", "MELEE", "Everybody in. Nobody safe."},
	{"Wildflower Open", "FLWR", "Grows anywhere. Wins anywhere."},
	{"Cherry Pit Classic", "CHRRY", "Sweet runs, hard finishes."},
	{"Monsoon Masters", "MNSN", "When it pours, it pours chips."},
	{"Sun Shower Series", "SUNNY", "Bright fields, sudden storms."},
};
const Named HighRollers[] = {
	{"Top Floor Series", "TOP", "Top floor only. The elevator needs a key."},
	{"Thin Air Classic", "AIR", "Hard to breathe up here. Harder to win."},
	{"Skyline High Rollers", "SKY", "The city's lights, from the top of the board."},
	{"The Vault", "VAULT", "Twelve days, twelve locks, one combination."},
	{"Black Card Series", "CARD", "No limit. Literally."},
	{"Gilded Week", "GILD", "Everything shines. Not everything is gold."},
	{"Diamond Lane", "DIAM", "The fast lane, for the few who can pay the toll."},
	{"Crown Jewel Series", "CROWN", "The most expensive tables in the house."},
	{"Altitude Open", "ALT", "The air's thinner. The stacks are deeper."},
	{"Velvet Rope Classic", "ROPE", "On the list, or on the rail."},
	{"Marble Hall Series", "HALL", "Every big call echoes in here."},
	{"Ivory Tower Series", "IVORY", "Theory meets reality at $25,500 a seat."},
	{"Platinum Week", "PLAT", "Rarer than gold, colder than ice."},
	{"The Gold Coast", "COAST", "Sun, sea and seven-figure guarantees."},
};
const Named Holidays[] = {
	{"Holiday Heist", "HEIST", "Take what you can carry before the year runs out."},
	{"Snowed-In Series", "SNOWD", "Nowhere to go. Twelve days of tournaments."},
	{"The Midwinter Ball", "BALL", "Dress code: hoodie. Main Event: formal."},
	{"Stocking Stuffer Series", "STUFF", "Small buy-ins, big surprises."},
	{"Last Call Classic", "LAST", "The last chance to fix the year's graph."},
	{"The Twelve Nights", "TWLV", "One for every night, and a Main Event to close."},
	{"Silver Bells Showdown", "BELLS", "Ring out the year. Ring up the bounties."},
	{"Fireside Festival", "FIRE", "Blankets, cocoa and a final table at 3 AM."},
	{"Mistletoe Masters", "MISTL", "Somebody's getting kissed by the river."},
	{"Countdown Classic", "CNTDN", "Ten, nine, eight... shove."},
	{"Candlelight Open", "CNDL", "The quietest tables of the year. The loudest finish."},
	{"Tinsel Town Throwdown", "TINSL", "Glitter on the felt, sharks in the water."},
	{"Hot Cocoa Classic", "COCOA", "Warm hands, cold reads."},
	{"The Year-End Rush", "YEAR", "One last run at the leaderboard."},
	{"Northern Star Series", "STAR", "Follow it home. Or to the final table."},
};

// Event names: an adjective and a noun, every pair used at most once in the whole calendar.
const char* const Adjectives[] = {"Midnight", "Iron", "Velvet", "Golden", "Crimson", "Silent", "Electric", "Lucky", "Wild", "Neon", "Rolling", "Burning",
	"Silver", "Hidden", "Reckless", "Steady", "Royal", "Savage", "Quiet", "Frozen", "Blazing", "Hollow", "Brass", "Copper", "Cobalt", "Scarlet", "Emerald",
	"Sapphire", "Obsidian", "Ivory", "Amber", "Jade", "Onyx", "Ruby", "Granite", "Thunder", "Stormy", "Shadow", "Phantom", "Ghost", "Rogue", "Rebel",
	"Outlaw", "Lone", "Last", "First", "Final", "Double", "Triple", "Twin", "Grand", "Little", "Late", "Early", "Northern", "Southern", "Eastern",
	"Western", "Coastal", "Desert", "Prairie", "Harbor", "Canyon", "Mountain", "Atomic", "Rocket", "Lightning", "Cosmic", "Lunar", "Solar", "Stellar",
	"Wicked", "Crooked", "Straight", "Suited", "Pocket", "Loose", "Tight", "Cold", "Sharp", "Slick", "Gritty", "Bold", "Brave", "Daring", "Fearless",
	"Restless", "Relentless", "Patient", "Clever", "Cunning", "Stone", "Steel", "Marble", "Paper", "Glass", "Smoky", "Ember", "Feral", "Mighty", "Noble",
	"Humble", "Hungry", "Rowdy", "Dapper", "Dusty", "Rusty", "Crystal", "Tidal", "Arctic"};
const char* const Nouns[] = {"Heron", "Gambit", "Kicker", "Duel", "Crown", "Hustle", "Stampede", "Fortune", "Ladder", "Gauntlet", "Rush", "Riot", "Raid",
	"Chase", "Hunt", "Bluff", "Wager", "Gamble", "Jackpot", "Treasure", "Anchor", "Compass", "Lantern", "Beacon", "Ridge", "Valley", "Tempest", "Cyclone",
	"Typhoon", "Blizzard", "Eclipse", "Horizon", "Mirage", "Oasis", "Labyrinth", "Arena", "Carnival", "Circus", "Rodeo", "Derby", "Sprint", "Relay",
	"Crusade", "Siege", "Standoff", "Rumble", "Brawl", "Clash", "Uprising", "Revolt", "Ambush", "Caper", "Scheme", "Paradox", "Riddle", "Enigma", "Cipher",
	"Oracle", "Monarch", "Baron", "Duke", "Sheriff", "Bandit", "Pirate", "Corsair", "Nomad", "Drifter", "Maverick", "Renegade", "Underdog", "Longshot",
	"Sleeper", "Spark", "Flame", "Inferno", "Blaze", "Wildfire", "Tsunami", "Current", "Undertow", "Tide", "Ripple", "Wolf", "Fox", "Hawk", "Falcon",
	"Raven", "Viper", "Cobra", "Tiger", "Lion", "Panther", "Jaguar", "Bear", "Bull", "Stallion", "Mustang", "Bison", "Coyote", "Owl", "Comet", "Meteor",
	"Express", "Voyage", "Odyssey", "Quest", "Ace", "Deuce", "Joker", "Wildcard", "Cooler", "Heater", "Grinder", "Shark", "Felt", "Rail"};

class Namer
{
public:
	Namer() : R("riverline/event-names") {}

	std::string Next()
	{
		const int Na = static_cast<int>(sizeof(Adjectives) / sizeof(Adjectives[0]));
		const int Nn = static_cast<int>(sizeof(Nouns) / sizeof(Nouns[0]));
		for (int Try = 0; Try < 64; ++Try)
		{
			std::string S;
			if (Take(R.Int(Na), R.Int(Nn), S))
			{
				return S;
			}
		}
		// Crowded: the next free pair from a random start.
		const int Start = R.Int(Na * Nn);
		for (int K = 0; K < Na * Nn; ++K)
		{
			const int Pair = (Start + K) % (Na * Nn);
			std::string S;
			if (Take(Pair / Nn, Pair % Nn, S))
			{
				return S;
			}
		}
		return "Encore " + std::to_string(++Spare);
	}

private:
	bool Take(int A, int N, std::string& Out)
	{
		const char* Adj = Adjectives[A];
		const char* Noun = Nouns[N];
		// "Wild Wildcard", "Blazing Blaze": not twice the same word.
		if (std::strncmp(Adj, Noun, 4) == 0)
		{
			return false;
		}
		Out = std::string(Adj) + " " + Noun;
		return Used.insert(Out).second;
	}

	Rng R;
	std::set<std::string> Used;
	int Spare = 0;
};

enum class Kind : int
{
	Classic,
	Freezeout,
	Pko,
	Mystery,
	SixMax,
	Turbo,
	Hyper,
	Deep,
	Omaha,
	Count,
};

const char* Suffix(Kind K)
{
	switch (K)
	{
	case Kind::Classic: return "";
	case Kind::Freezeout: return " Freezeout";
	case Kind::Pko: return " PKO";
	case Kind::Mystery: return " Mystery Bounty";
	case Kind::SixMax: return " 6-Max";
	case Kind::Turbo: return " Turbo";
	case Kind::Hyper: return " Hyper";
	case Kind::Deep: return " Deepstack";
	case Kind::Omaha: return " PLO";
	case Kind::Count: break;
	}
	return "";
}

/** A special: a Main Event, a High Roller (day and slot within the series). */
struct Special
{
	int Day = 0;
	int Slot = 0;
	const char* Id = "";
	const char* Name = "";
	double BuyIn = 0.0;
	double Gtd = 0.0;
	bool Main = false;
	const char* Blurb = "";
};

/** One series in the yearly calendar. */
struct Plan
{
	const char* Key = "";
	int Month = 1;
	int Day = 1;
	int Days = 12;
	int PerDay = 4;
	std::vector<double> Buys;
	std::vector<double> AwardBuys; // ring and bracelet events' buy-ins
	std::array<int, static_cast<int>(Kind::Count)> Weights{};
	double Scale = 1.0;
	int Awards = 0; // ring or bracelet events a day (the first slots)
	bool Bracelets = false;
	bool Rings = false;
	std::vector<Special> Specials;
	uint32_t Color = 0;
	uint32_t Color2 = 0;
};

std::vector<int> Times(int PerDay)
{
	switch (PerDay)
	{
	case 3: return {At(14, 0), At(18, 0), At(21, 0)};
	case 4: return {At(13, 0), At(16, 0), At(19, 0), At(21, 30)};
	case 5: return {At(12, 0), At(15, 0), At(18, 0), At(20, 0), At(22, 0)};
	default: return {At(11, 0), At(13, 0), At(15, 0), At(17, 0), At(19, 30), At(21, 30)};
	}
}

/** Guarantees by the size of the buy-in (a series' Scale on top). */
double GtdFor(double BuyIn)
{
	return BuyIn <= 5.5 ? BuyIn * 2500.0 : BuyIn <= 55.0 ? BuyIn * 1200.0 : BuyIn <= 530.0 ? BuyIn * 500.0 : BuyIn <= 2100.0 ? BuyIn * 150.0 : BuyIn * 60.0;
}

std::string Two(int Year)
{
	const int Y = Year % 100;
	return (Y < 10 ? "0" : "") + std::to_string(Y);
}
} // namespace calendar_detail

using namespace calendar_detail;

void Network::BuildCalendar()
{
	Namer Names;
	// What each series looks like (the same every year; the seasonal ones change their names).
	auto W = [](int Classic, int Freeze, int Pko, int Myst, int Six, int Turbo, int Hyper, int Deep, int Omaha) {
		return std::array<int, static_cast<int>(Kind::Count)>{Classic, Freeze, Pko, Myst, Six, Turbo, Hyper, Deep, Omaha};
	};
	Plan WinterPlan;
	WinterPlan.Key = "win";
	WinterPlan.Month = 1;
	WinterPlan.Day = 9;
	WinterPlan.Days = 12;
	WinterPlan.PerDay = 4;
	WinterPlan.Buys = {3.30, 11.0, 22.0, 55.0, 109.0, 215.0, 530.0};
	WinterPlan.Weights = W(3, 1, 3, 1, 1, 2, 0, 1, 1);
	WinterPlan.Scale = 0.8;
	WinterPlan.Specials = {{10, 3, "hr", "$1,050 High Roller", 1050.0, 500000.0, false, "The winter's biggest buy-in. Short field, deep structure."},
		{11, 1, "main", "$215 Main Event", 215.0, 2000000.0, true, "The Main Event: $2,000,000 guaranteed, 15-minute levels."}};
	WinterPlan.Color = 0x8fd3ff;
	WinterPlan.Color2 = 0x3b82f6;

	Plan RingPlan;
	RingPlan.Key = "ring";
	RingPlan.Month = 3;
	RingPlan.Day = 6;
	RingPlan.Days = 14;
	RingPlan.PerDay = 4;
	RingPlan.Buys = {11.0, 33.0, 55.0, 109.0};
	RingPlan.AwardBuys = {109.0, 215.0, 320.0, 525.0};
	RingPlan.Weights = W(4, 2, 2, 0, 1, 1, 0, 1, 1);
	RingPlan.Scale = 0.7;
	RingPlan.Awards = 2;
	RingPlan.Rings = true;
	RingPlan.Specials = {{13, 1, "main", "$1,050 Ring Main Event", 1050.0, 1000000.0, true, "The Grand Circuit's online Main Event. A ring, and a seven-figure guarantee."}};
	RingPlan.Color = 0xf28a3a;
	RingPlan.Color2 = 0xf2c14e;

	Plan SpringPlan;
	SpringPlan.Key = "spr";
	SpringPlan.Month = 4;
	SpringPlan.Day = 24;
	SpringPlan.Days = 12;
	SpringPlan.PerDay = 4;
	SpringPlan.Buys = {0.55, 1.10, 2.20, 3.30, 5.50, 11.0, 22.0};
	SpringPlan.Weights = W(2, 0, 3, 2, 0, 2, 1, 1, 1);
	SpringPlan.Scale = 0.8;
	SpringPlan.Specials = {{10, 3, "hr", "$215 High Roller", 215.0, 250000.0, false, "Spring's high roller: the regulars' night out."},
		{11, 1, "main", "$55 Main Event", 55.0, 1000000.0, true, "$55 to play for a million. Spring's people's Main Event."}};
	SpringPlan.Color = 0x7ee081;
	SpringPlan.Color2 = 0xf472b6;

	Plan BraceletPlan;
	BraceletPlan.Key = "tco";
	BraceletPlan.Month = 6;
	BraceletPlan.Day = 4;
	BraceletPlan.Days = 19;
	BraceletPlan.PerDay = 3;
	BraceletPlan.Buys = {22.0, 55.0};
	BraceletPlan.AwardBuys = {109.0, 215.0, 320.0, 525.0, 1050.0, 2100.0};
	BraceletPlan.Weights = W(3, 2, 2, 0, 1, 1, 0, 1, 2);
	BraceletPlan.Scale = 0.9;
	BraceletPlan.Awards = 2;
	BraceletPlan.Bracelets = true;
	BraceletPlan.Specials = {{18, 1, "main", "$5,300 Online Championship", 5300.0, 5000000.0, true,
		"The Championship's online crown: a bracelet and $5,000,000 guaranteed, in the days before the Main Event in Las Vegas."}};
	BraceletPlan.Color = 0xffd166;
	BraceletPlan.Color2 = 0xc9962b;

	Plan HighPlan;
	HighPlan.Key = "hrs";
	HighPlan.Month = 7;
	HighPlan.Day = 14;
	HighPlan.Days = 12;
	HighPlan.PerDay = 3;
	HighPlan.Buys = {530.0, 1050.0, 2100.0, 5250.0};
	HighPlan.Weights = W(3, 2, 2, 0, 2, 1, 0, 1, 1);
	HighPlan.Specials = {{9, 2, "shr", "$25,500 Super High Roller", 25500.0, 1500000.0, false, "The biggest buy-in on RiverLine all year."},
		{11, 1, "main", "$10,300 Main Event", 10300.0, 3000000.0, true, "Ten thousand to play. Three million guaranteed. The best in the world, nobody else."}};
	HighPlan.Color = 0xe6edf7;
	HighPlan.Color2 = 0x8b5cf6;

	Plan SlamPlan;
	SlamPlan.Key = "slam";
	SlamPlan.Month = 8;
	SlamPlan.Day = 20;
	SlamPlan.Days = 17;
	SlamPlan.PerDay = 5;
	SlamPlan.Buys = {5.50, 22.0, 55.0, 109.0, 215.0, 530.0, 1050.0};
	SlamPlan.Weights = W(3, 0, 3, 1, 1, 2, 1, 1, 1);
	SlamPlan.Scale = 1.5;
	SlamPlan.Specials = {{14, 4, "hr", "$5,250 High Roller", 5250.0, 1000000.0, false, "The summer's high roller."},
		{15, 2, "mini", "$109 Mini Main Event", 109.0, 1500000.0, true, "The Main Event's little sibling. $1.5M guaranteed."},
		{16, 1, "main", "$1,050 Main Event", 1050.0, 10000000.0, true, "The summer's Main Event: $10,000,000 guaranteed."}};
	SlamPlan.Color = 0xf28a3a;
	SlamPlan.Color2 = 0xef4d5a;

	Plan MicroPlan;
	MicroPlan.Key = "mm";
	MicroPlan.Month = 10;
	MicroPlan.Day = 1;
	MicroPlan.Days = 14;
	MicroPlan.PerDay = 5;
	MicroPlan.Buys = {0.55, 1.10, 2.20, 3.30, 5.50, 11.0, 22.0};
	MicroPlan.Weights = W(2, 0, 3, 2, 1, 2, 2, 1, 1);
	MicroPlan.Scale = 2.0;
	MicroPlan.Specials = {{10, 3, "hr", "$109 High Roller", 109.0, 250000.0, false, "The micros' high roller: a shot at a five-figure score."},
		{11, 2, "main", "$11 Main Event", 11.0, 1000000.0, true, "$11 to play for a million. The biggest micro-stakes tournament of the year."}};
	MicroPlan.Color = 0xb8ff2e;
	MicroPlan.Color2 = 0x27d3c3;

	Plan RcopPlan;
	RcopPlan.Key = "rcop";
	RcopPlan.Month = 10;
	RcopPlan.Day = 18;
	RcopPlan.Days = 22;
	RcopPlan.PerDay = 6;
	RcopPlan.Buys = {5.50, 22.0, 55.0, 109.0, 215.0, 530.0, 1050.0, 2100.0, 5250.0};
	RcopPlan.Weights = W(3, 1, 3, 1, 1, 1, 0, 1, 1);
	RcopPlan.Scale = 2.6;
	RcopPlan.Specials = {{12, 5, "hr", "$10,300 High Roller", 10300.0, 1500000.0, false, "RCOP's high roller."},
		{18, 5, "shr", "$25,500 Super High Roller", 25500.0, 2500000.0, false, "The richest field of the autumn."},
		{19, 2, "micro", "$55 Micro Main Event", 55.0, 2000000.0, true, "RCOP for everyone: $55, two million guaranteed."},
		{20, 2, "mini", "$530 Mini Main Event", 530.0, 5000000.0, true, "Five million guaranteed, a tenth of the Main's price."},
		{21, 2, "main", "$5,250 Main Event", 5250.0, 25000000.0, true, "The one that changes a life. $25,000,000 guaranteed."}};
	RcopPlan.Color = 0xf2c14e;
	RcopPlan.Color2 = 0x8b5cf6;

	Plan HolidayPlan;
	HolidayPlan.Key = "hol";
	HolidayPlan.Month = 12;
	HolidayPlan.Day = 10;
	HolidayPlan.Days = 12;
	HolidayPlan.PerDay = 4;
	HolidayPlan.Buys = {2.20, 5.50, 11.0, 22.0, 55.0, 109.0, 215.0};
	HolidayPlan.Weights = W(2, 0, 2, 3, 0, 2, 1, 1, 1);
	HolidayPlan.Scale = 0.9;
	HolidayPlan.Specials = {{10, 3, "hr", "$1,050 High Roller", 1050.0, 300000.0, false, "The year's last high roller."},
		{11, 1, "main", "$109 Main Event", 109.0, 1500000.0, true, "The year's last Main Event: $1,500,000 guaranteed."}};
	HolidayPlan.Color = 0xef4d5a;
	HolidayPlan.Color2 = 0x27d3c3;

	static const uint32_t Accents[5] = {0x27d3c3, 0xa855f7, 0xf2c14e, 0x3b82f6, 0xf472b6};

	auto Build = [&](const Plan& Pl, int Year, const std::string& Name, const std::string& Short, const std::string& Tagline, int Rotate) {
		// The flagships come back every year: their events carry the year ("RCOP '27 #132"), so no two names match.
		const std::string Tag = Rotate >= 0 ? Short : Short + " '" + Two(Year);
		SeriesInfo Sr;
		Sr.Id = std::string(Pl.Key) + Two(Year);
		Sr.Name = Name;
		Sr.Short = Short;
		Sr.FirstDay = world::DayOn(Year, Pl.Month, Pl.Day);
		Sr.LastDay = Sr.FirstDay + Pl.Days - 1;
		Sr.Color = Pl.Color;
		Sr.Color2 = Rotate >= 0 ? Accents[Rotate % 5] : Pl.Color2;
		const std::vector<int> Slots = Times(Pl.PerDay);
		Rng R("riverline/series/" + Sr.Id);
		int Total = 0;
		for (int V : Pl.Weights)
		{
			Total += V;
		}
		int No = 0;
		double Gtd = 0.0;
		for (int D = 0; D < Pl.Days; ++D)
		{
			for (int K = 0; K < Pl.PerDay; ++K)
			{
				++No;
				EventTemplate T;
				T.Series = Sr.Id;
				T.EventNo = No;
				T.OnlyDay = Sr.FirstDay + D;
				T.StartMinute = Slots[static_cast<size_t>(K)];
				T.OpensHours = 24 * 14;
				T.TableSize = 8;
				T.Fmt = Format::ReEntry;
				T.Speed = "Regular";
				T.LevelMinutes = 12.0;
				T.StartStack = 50000;
				T.LateRegMinutes = 240;
				const bool Award = K < Pl.Awards;
				T.Bracelet = Award && Pl.Bracelets;
				T.Ring = Award && Pl.Rings;
				const Special* Sp = nullptr;
				for (const Special& S : Pl.Specials)
				{
					if (S.Day == D && S.Slot == K)
					{
						Sp = &S;
					}
				}
				double Dollars = 0.0;
				if (Sp)
				{
					T.Id = Sr.Id + "-" + Sp->Id;
					T.Name = Tag + " #" + std::to_string(No) + ": " + Sp->Name;
					Dollars = Sp->BuyIn;
					T.GtdCents = SeriesCents(Sp->Gtd);
					T.Main = Sp->Main;
					T.LevelMinutes = 15.0;
					T.StartStack = 100000;
					T.LateRegMinutes = 360;
					T.OpensHours = Sp->Main ? 24 * 30 : 24 * 21;
					T.Featured = true;
					T.Blurb = Sp->Blurb;
					// The Main Events of a ring or bracelet series award one too.
					T.Bracelet = Sp->Main && Pl.Bracelets;
					T.Ring = Sp->Main && Pl.Rings;
					if (std::string(Pl.Key) == "rcop" && std::string(Sp->Id) == "main")
					{
						T.Ticket = "rcop-main"; // the step satellites' seats, whatever the year
					}
				}
				else
				{
					// Pick a format the series likes, a buy-in from its ladder, and a name nobody has used.
					int Pick = R.Int(std::max(1, Total));
					Kind Fmt = Kind::Classic;
					for (int I = 0; I < static_cast<int>(Kind::Count); ++I)
					{
						Pick -= Pl.Weights[static_cast<size_t>(I)];
						if (Pick < 0)
						{
							Fmt = static_cast<Kind>(I);
							break;
						}
					}
					if (Award && Fmt != Kind::Classic && Fmt != Kind::Freezeout && Fmt != Kind::Pko && Fmt != Kind::SixMax && Fmt != Kind::Omaha && Fmt != Kind::Deep)
					{
						Fmt = Kind::Classic; // titles are decided at a proper structure
					}
					const std::vector<double>& Ladder = Award && !Pl.AwardBuys.empty() ? Pl.AwardBuys : Pl.Buys;
					// The day's slots climb the ladder, the days don't repeat each other.
					Dollars = Ladder[static_cast<size_t>((K * 2 + D * 3 + R.Int(3)) % static_cast<int>(Ladder.size()))];
					T.Id = Sr.Id + "-" + std::to_string(No);
					T.Name = Tag + " #" + std::to_string(No) + ": " + BuyIn(SeriesCents(Dollars)) + " " + Names.Next() + Suffix(Fmt);
					T.GtdCents = SeriesCents(Nice(GtdFor(Dollars) * Pl.Scale * (Award ? 1.4 : 1.0)));
					switch (Fmt)
					{
					case Kind::Classic: break;
					case Kind::Freezeout: T.Fmt = Format::Freezeout; break;
					case Kind::Pko: T.Fmt = Format::Bounty; break;
					case Kind::Mystery: T.Fmt = Format::Mystery; break;
					case Kind::SixMax: T.TableSize = 6; break;
					case Kind::Turbo:
						T.Speed = "Turbo";
						T.LevelMinutes = 6.0;
						T.StartStack = 30000;
						T.LateRegMinutes = 90;
						break;
					case Kind::Hyper:
						T.Speed = "Hyper";
						T.LevelMinutes = 3.0;
						T.StartStack = 20000;
						T.LateRegMinutes = 30;
						break;
					case Kind::Deep:
						T.Speed = "Deep";
						T.LevelMinutes = 15.0;
						T.StartStack = 100000;
						T.LateRegMinutes = 300;
						break;
					case Kind::Omaha:
						T.Omaha = true;
						T.TableSize = 6;
						break;
					case Kind::Count: break;
					}
					T.Featured = T.GtdCents >= SeriesCents(500000.0) || (Award && Dollars >= 525.0);
				}
				T.BuyInCents = SeriesCents(Dollars);
				T.Field = std::max(20, static_cast<int>(static_cast<double>(T.GtdCents) / 100.0 / (Dollars * 0.915) * 1.1));
				Gtd += static_cast<double>(T.GtdCents);
				Sr.Bracelets += T.Bracelet ? 1 : 0;
				Sr.Rings += T.Ring ? 1 : 0;
				if (Sp && std::strcmp(Sp->Id, "main") == 0)
				{
					Sr.MainEvent = T.Id;
				}
				Temps.push_back(T);
			}
		}
		Sr.Events = No;
		Sr.GtdCents = static_cast<Chips>(Gtd);
		const std::string Money = MoneyShort(Sr.GtdCents);
		Sr.Tagline = Tagline.empty() ? std::to_string(No) + " events, " + Money + " guaranteed." : Tagline + " " + std::to_string(No) + " events, " + Money + " guaranteed.";
		AllSeries.push_back(Sr);
	};

	// December 2026 first (the year's holiday series), then every year in calendar order.
	const int Seasonal = static_cast<int>(sizeof(Holidays) / sizeof(Holidays[0]));
	Build(HolidayPlan, 2026, Holidays[0].Name, Holidays[0].Short, Holidays[0].Tagline, 0);
	for (int Year = FirstYear; Year <= LastYear; ++Year)
	{
		const int K = Year - FirstYear;
		const std::string Yr = std::to_string(Year);
		const Named& Wn = Winter[K % 14];
		Build(WinterPlan, Year, Wn.Name, Wn.Short, Wn.Tagline, K);
		Build(RingPlan, Year, "Ring Rush " + Yr, "RING", "Grand Circuit Online: a Grand Circuit ring for every title.", -1);
		const Named& Sp = Spring[K % 14];
		Build(SpringPlan, Year, Sp.Name, Sp.Short, Sp.Tagline, K + 1);
		Build(BraceletPlan, Year, "The Championship Online " + Yr, "TCO", "Championship bracelets, won from your desk.", -1);
		const Named& Hr = HighRollers[K % 14];
		Build(HighPlan, Year, Hr.Name, Hr.Short, Hr.Tagline, K + 2);
		Build(SlamPlan, Year, "Summer Slam " + Yr, "SLAM", "The summer festival.", -1);
		Build(MicroPlan, Year, "Micro Madness " + Yr, "MM", "Two weeks of micro stakes. Everyone's a shot.", -1);
		Build(RcopPlan, Year, "RCOP " + Yr, "RCOP", "The RiverLine Championship of Online Poker.", -1);
		const Named& Hd = Holidays[(K + 1) % Seasonal];
		Build(HolidayPlan, Year, Hd.Name, Hd.Short, Hd.Tagline, K + 3);
	}

	// New weekly tournaments, each with a name of its own (after the calendar: the older templates keep their places).
	struct Weekly
	{
		const char* Id;
		const char* Name;
		double BuyIn;
		double Gtd;
		Format Fmt;
		const char* Speed;
		int Days; // weekday mask, bit 0 = Monday
		int Start;
		int TableSize;
		bool Omaha;
		const char* Blurb;
	};
	const Weekly Weeklies[] = {
		{"moonlight-marathon", "Moonlight Marathon", 33.0, 25000.0, Format::ReEntry, "Deep", 1 << 0, At(19, 30), 8, false, "Twenty-minute levels and nowhere to be. Bring snacks."},
		{"bankroll-builder", "The Bankroll Builder", 1.10, 1500.0, Format::ReEntry, "Regular", (1 << 0) | (1 << 2) | (1 << 4), At(18, 0), 9, false,
			"A dollar and change, three nights a week. Where bankrolls start."},
		{"tuesday-tycoon", "Tuesday Tycoon", 530.0, 100000.0, Format::ReEntry, "Regular", 1 << 1, At(20, 0), 8, false, "Midweek money for the mid-stakes crowd."},
		{"high-noon", "High Noon Showdown", 109.0, 40000.0, Format::Freezeout, "Regular", 1 << 2, At(12, 0), 8, false, "One bullet. Lunch break optional."},
		{"hump-day-heater", "Hump Day Heater", 11.0, 15000.0, Format::Bounty, "Regular", 1 << 2, At(19, 0), 8, false, "Halfway through the week, a bounty on every head."},
		{"lucky-sevens", "Lucky Sevens", 7.70, 7700.0, Format::Freezeout, "Turbo", 1 << 3, At(21, 0), 8, false, "$7.70 in, $7,700 guaranteed, 77 minutes to register. Feeling lucky?"},
		{"ante-up", "The Ante Up", 215.0, 60000.0, Format::ReEntry, "Regular", 1 << 4, At(19, 0), 6, false, "Six-max, big antes, no hiding."},
		{"friday-fireworks", "Friday Fireworks", 55.0, 60000.0, Format::Mystery, "Regular", 1 << 4, At(20, 30), 8, false, "Mystery envelopes go off all night."},
		{"twilight-omaha", "Twilight Omaha", 11.0, 8000.0, Format::ReEntry, "Regular", 1 << 5, At(19, 0), 6, true, "Four cards, six seats, one long evening."},
		{"saturday-stampede", "Saturday Night Stampede", 22.0, 30000.0, Format::ReEntry, "Turbo", 1 << 5, At(21, 30), 8, false, "Everybody runs. Somebody gets trampled."},
		{"brunch-stack", "Sunday Brunch Stack", 5.50, 10000.0, Format::ReEntry, "Deep", 1 << 6, At(11, 0), 9, false, "Coffee, eggs, and 100,000 chips."},
		{"slingshot", "The Sunday Slingshot", 2.20, 3000.0, Format::Bounty, "Hyper", 1 << 6, At(22, 30), 8, false, "Last shot of the weekend. Three-minute levels."},
	};
	for (const Weekly& Wk : Weeklies)
	{
		EventTemplate T;
		T.Id = Wk.Id;
		T.Name = Wk.Name;
		T.BuyInCents = SeriesCents(Wk.BuyIn);
		T.GtdCents = SeriesCents(Wk.Gtd);
		T.Fmt = Wk.Fmt;
		T.Speed = Wk.Speed;
		T.LevelMinutes = std::string(Wk.Speed) == "Hyper" ? 3.0 : std::string(Wk.Speed) == "Turbo" ? 6.0 : std::string(Wk.Speed) == "Deep" ? 20.0 : 12.0;
		T.StartStack = std::string(Wk.Speed) == "Deep" ? 100000 : std::string(Wk.Speed) == "Hyper" ? 10000 : 30000;
		T.TableSize = Wk.TableSize;
		T.Omaha = Wk.Omaha;
		T.LateRegMinutes = std::string(Wk.Id) == "lucky-sevens" ? 77 : std::string(Wk.Speed) == "Hyper" ? 20 : std::string(Wk.Speed) == "Turbo" ? 75 : 180;
		T.Days = Wk.Days;
		T.StartMinute = Wk.Start;
		T.OpensHours = 72;
		T.Blurb = Wk.Blurb;
		T.Field = std::max(40, static_cast<int>(Wk.Gtd / (Wk.BuyIn * 0.915) * 1.15));
		Temps.push_back(T);
	}
}
} // namespace net
} // namespace ss
