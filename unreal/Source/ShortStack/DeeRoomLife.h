#pragma once

#include "CoreMinimal.h"

#include <string>
#include <vector>

namespace ss
{
class Session;
}

/**
 * Dee at home: where she is at an hour of the week, and what she says (or texts, or leaves on a note) when the player
 * comes in, when she comes and goes while they're there, and when they get their key back. Her front room is the hub
 * while the player is evicted (ADeeRoomStage); her game runs downstairs in the Wash & Fold's back room (life's
 * "dee-game": Tuesdays, Thursdays and Saturdays, doors at nine, the last hand at five).
 */
namespace DeeRoom
{
enum class EWhere : uint8
{
	/** In her armchair with the TV on low: the evenings she isn't running her game. */
	Home,
	/** In her room, the door shut: late on a quiet night, and the morning after her game. */
	Asleep,
	/** Downstairs running her game, from half an hour before the doors open to the last hand. */
	Game,
	/** Out for the day. */
	Out,
};

/** Where Dee is at this world minute (Session::WorldMinutes). */
EWhere WhereAt(double WorldMinutes);
const TCHAR* WhereName(EWhere Where);

/** How the player came in: from the title screen (or a fresh start), the street, the Back Room, the Embercrest. */
enum class EArrival : uint8
{
	Start,
	Street,
	BackRoom,
	Embercrest,
};
/** From the options the level was opened with ("?Home" from another level, with what that level passed back). */
EArrival ArrivalFrom(const FString& Options);

/** One thing she says in the room, or texts, or a line of the room's own (narration: nobody speaks it). */
struct FLine
{
	enum EKind : uint8
	{
		None,
		Spoken,
		Texted,
		Narration,
	};
	EKind Kind = None;
	FString Text;
	/** The first night's welcome (the couch made up): said once, not every time the player comes in that day. */
	bool bWelcome = false;
	bool IsEmpty() const { return Kind == None || Text.IsEmpty(); }
};

/** The locks changed within the last day: the player's first night on her couch. */
bool FirstNight(const ss::Session& S, double WorldMinutes);
/**
 * As the player comes in. Options: what the level was opened with (the night's result, from the Embercrest). On the
 * first night (bWelcome: FirstNight, and not welcomed yet) she welcomes them instead.
 */
FLine Arrival(EWhere Where, EArrival From, const FString& Options, double WorldMinutes, bool bWelcome);
/** She goes (Was, now To), or comes in, while the player is there. */
FLine Change(EWhere Was, EWhere To, double WorldMinutes);
/** The player paid their way back into 1812 while here. */
FLine KeyBack(EWhere Where);
/** The player woke on the couch with her home. */
FLine Woke(double WorldMinutes);
/** The sticky note she leaves by the laptop when she isn't in the room (none when she is). */
std::vector<std::string> Note(EWhere Where, double WorldMinutes);
} // namespace DeeRoom
