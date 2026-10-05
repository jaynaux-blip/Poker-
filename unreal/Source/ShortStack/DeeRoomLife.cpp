#include "DeeRoomLife.h"

#include "Kismet/GameplayStatics.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Session.h"

#include <cmath>

namespace DeeRoomLifeDetail
{
constexpr double MinutesInDay = 1440.0;
// Her day, minutes after midnight: she goes down half an hour before the doors to set up, and the last hand is at five
// (Life.cpp); a quiet night she's in bed by one and up at nine; after a game she sleeps till one.
constexpr double LastHand = 5.0 * 60.0;
constexpr double Bedtime = 1.0 * 60.0;
constexpr double Up = 9.0 * 60.0;
constexpr double UpAfterGame = 13.0 * 60.0;
constexpr double Home = 17.0 * 60.0;
constexpr double SetUp = 30.0;

/** The game's weekdays (bit 0 Monday) and its doors, as the career's catalog has them (it doesn't change: read once). */
void GameNights(int32& OutDays, double& OutDoors)
{
	static const ss::life::Activity* Game = ss::life::Find("dee-game");
	OutDays = Game ? Game->Days : (1 << 1) | (1 << 3) | (1 << 5);
	OutDoors = Game ? static_cast<double>(Game->Opens) : 21.0 * 60.0;
}

/** Whether her game runs the night that begins on world day Day (day 0 is a Monday). */
bool GameNight(int64 Day)
{
	int32 Days = 0;
	double Doors = 0.0;
	GameNights(Days, Doors);
	const int32 Weekday = static_cast<int32>(((Day % 7) + 7) % 7);
	return ((Days >> Weekday) & 1) != 0;
}

int64 DayOf(double World)
{
	return static_cast<int64>(std::floor(World / MinutesInDay));
}

double TimeOfDay(double World)
{
	return World - static_cast<double>(DayOf(World)) * MinutesInDay;
}

/** One of Lines, the same all day (a reload doesn't change her mind), different with Salt. */
FString Pick(std::initializer_list<const TCHAR*> Lines, double World, int32 Salt)
{
	const int32 Count = static_cast<int32>(Lines.size());
	if (Count == 0)
	{
		return FString();
	}
	const uint32 Hash = HashCombine(GetTypeHash(DayOf(World)), GetTypeHash(Salt));
	return FString(*(Lines.begin() + Hash % static_cast<uint32>(Count)));
}

DeeRoom::FLine Line(DeeRoom::FLine::EKind Kind, const FString& Text)
{
	DeeRoom::FLine L;
	L.Kind = Kind;
	L.Text = Text;
	return L;
}

DeeRoom::FLine Welcome(DeeRoom::FLine::EKind Kind, const FString& Text)
{
	DeeRoom::FLine L = Line(Kind, Text);
	L.bWelcome = true;
	return L;
}
} // namespace DeeRoomLifeDetail

using namespace DeeRoomLifeDetail;

namespace DeeRoom
{
EWhere WhereAt(double WorldMinutes)
{
	int32 Days = 0;
	double Doors = 0.0;
	GameNights(Days, Doors);
	const int64 Day = DayOf(WorldMinutes);
	const double T = TimeOfDay(WorldMinutes);
	// Past midnight, the night still belongs to the day it began (as the game's own window does).
	const bool bLastNight = GameNight(Day - 1);
	if (T >= Doors - SetUp)
	{
		return GameNight(Day) ? EWhere::Game : EWhere::Home;
	}
	if (T < LastHand)
	{
		return bLastNight ? EWhere::Game : (T < Bedtime ? EWhere::Home : EWhere::Asleep);
	}
	if (T < (bLastNight ? UpAfterGame : Up))
	{
		return EWhere::Asleep;
	}
	return T < Home ? EWhere::Out : EWhere::Home;
}

const TCHAR* WhereName(EWhere Where)
{
	switch (Where)
	{
	case EWhere::Home: return TEXT("home, in her armchair");
	case EWhere::Asleep: return TEXT("asleep in her room");
	case EWhere::Game: return TEXT("downstairs running her game");
	case EWhere::Out: return TEXT("out");
	}
	return TEXT("?");
}

EArrival ArrivalFrom(const FString& Options)
{
	if (!UGameplayStatics::HasOption(Options, TEXT("Home")))
	{
		return EArrival::Start;
	}
	if (UGameplayStatics::HasOption(Options, TEXT("Street")))
	{
		return EArrival::Street;
	}
	return UGameplayStatics::HasOption(Options, TEXT("Live")) ? EArrival::Embercrest : EArrival::BackRoom;
}

bool FirstNight(const ss::Session& S, double WorldMinutes)
{
	return S.Evicted() && S.Life.EvictedAt > 0.0 && WorldMinutes - S.Life.EvictedAt < MinutesInDay;
}

FLine Arrival(EWhere Where, EArrival From, const FString& Options, double WorldMinutes, bool bWelcome)
{
	switch (Where)
	{
	case EWhere::Home:
		if (bWelcome)
		{
			return Welcome(FLine::Spoken, TEXT("There you are. Couch is made up, towel's on the radiator. It knocks all night. You stop hearing it."));
		}
		if (From == EArrival::Embercrest)
		{
			// She watched it from the box, as her text would have said it.
			if (UGameplayStatics::HasOption(Options, TEXT("Cancelled")))
			{
				return Line(FLine::Spoken, TEXT("They cancelled it? Slow night. At least you got your money back."));
			}
			if (UGameplayStatics::HasOption(Options, TEXT("Won")))
			{
				return Line(FLine::Spoken, TEXT("Look who's back. The whole floor's talking about you. Sit down before you fall down."));
			}
			if (FCString::Atoi64(*UGameplayStatics::ParseOption(Options, TEXT("Prize"))) > 0)
			{
				return Line(FLine::Spoken, TEXT("You cashed. I saw. It's one night, so bank it."));
			}
			return Line(FLine::Spoken, TEXT("Long one? Sit. Tournaments are long."));
		}
		if (From == EArrival::Street)
		{
			return Line(FLine::Spoken, Pick({TEXT("Shoes off. It's a rug, not a doormat."), TEXT("Still coming down out there? Hang your jacket on the radiator."),
				TEXT("You eat? There's rice in the pot.")}, WorldMinutes, 1));
		}
		return Line(FLine::Spoken, Pick({TEXT("Hey. TV's on if you want company."), TEXT("Hey, you. Kettle's still warm."),
			TEXT("Laptop's where you left it. Nobody touched it. Well. The cat sat on it.")}, WorldMinutes, 2));
	case EWhere::Game:
		if (From == EArrival::BackRoom)
		{
			return FLine(); // back up from her table: what she has to say about the night comes as her text
		}
		return bWelcome ? Welcome(FLine::Texted, TEXT("i'm downstairs running the game. couch is made up, towel's on the radiator. door's unlocked."))
						: Line(FLine::Texted, TEXT("game's going downstairs. come down if you want a seat. if not, sleep."));
	case EWhere::Asleep:
		if (From == EArrival::BackRoom && TimeOfDay(WorldMinutes) >= LastHand && TimeOfDay(WorldMinutes) < LastHand + 90.0)
		{
			// Up the stairs with her after the last hand: she went straight to bed.
			return Line(FLine::Narration, TEXT("Dee came up the stairs behind you and went straight to bed. Her door's shut."));
		}
		return Line(FLine::Narration, TEXT("Dee's door is shut. She's asleep. There's a note by the laptop."));
	case EWhere::Out:
		return Line(FLine::Narration, TEXT("Dee's out. There's a note by the laptop."));
	}
	return FLine();
}

FLine Change(EWhere Was, EWhere To, double WorldMinutes)
{
	if (Was == EWhere::Home && To == EWhere::Game)
	{
		return Line(FLine::Spoken, TEXT("Game's starting. I'm going down. Come down if you want a seat. Bring cash, not excuses."));
	}
	if (Was == EWhere::Home && To == EWhere::Asleep)
	{
		return Line(FLine::Spoken, Pick({TEXT("I'm going to bed. Don't stay up all night on that thing."), TEXT("Night. Don't let the radiator scare you."),
			TEXT("Bed. If the cat yells, ignore her. She's lying.")}, WorldMinutes, 3));
	}
	if (To == EWhere::Home)
	{
		return Line(FLine::Spoken, Pick({TEXT("Hey. You eat today?"), TEXT("It's coming down sideways out there. Don't go anywhere without the umbrella."),
			TEXT("I'm back. Move your feet, that's my chair's side of the rug.")}, WorldMinutes, 4));
	}
	if (Was == EWhere::Game && To == EWhere::Asleep)
	{
		return Line(FLine::Narration, TEXT("Dee comes up the stairs after the last hand. Her door clicks shut."));
	}
	if (To == EWhere::Out)
	{
		return Line(FLine::Spoken, Pick({TEXT("Back by five. Eat something that isn't from a can."), TEXT("Going out. Don't answer the door for the landlord. Yours or mine.")},
			WorldMinutes, 5));
	}
	return FLine();
}

FLine KeyBack(EWhere Where)
{
	if (Where == EWhere::Home)
	{
		return Line(FLine::Spoken, TEXT("Got your key back? Good. Go on, then. Take your boxes. Leave the towel."));
	}
	return Line(FLine::Texted, TEXT("heard you got your key back. take your boxes, leave the towel."));
}

FLine Woke(double WorldMinutes)
{
	return Line(FLine::Spoken, Pick({TEXT("You were out cold. I turned the TV down."), TEXT("Morning, or whatever this is. You snore, you know.")}, WorldMinutes, 6));
}

std::vector<std::string> Note(EWhere Where, double /*WorldMinutes*/)
{
	switch (Where)
	{
	case EWhere::Asleep: return {"SLEEPING.", "FRIDGE IS", "YOURS.", "- D"};
	case EWhere::Game: return {"GAME'S ON", "DOWNSTAIRS.", "COME DOWN.", "- D"};
	case EWhere::Out: return {"OUT TIL 5.", "EAT", "SOMETHING.", "- D"};
	case EWhere::Home: break;
	}
	return {};
}
} // namespace DeeRoom
