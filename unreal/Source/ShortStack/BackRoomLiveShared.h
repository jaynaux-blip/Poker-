// What the Embercrest's card room files share (ABackRoomGameMode's live tournaments, split by job in next_segment_spec
// 8.2): the night's flow (BackRoomLive.cpp), the room and its floor (BackRoomLiveRoom.cpp), the desk, the boards and the
// next event (BackRoomLiveVenue.cpp), who sits where (BackRoomLiveCast.cpp) and the test hooks (BackRoomLiveCommands.cpp).
// Private to those five files. It froze with BackRoomGameMode.h at the end of Phase A: a track that needs something here
// sends the lead the patch.
#pragma once

#include "CoreMinimal.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Session.h"

#include <atomic>
#include <string>
#include <utility>
#include <vector>

class ABackRoomPlayer;

DECLARE_LOG_CATEGORY_EXTERN(LogEmbercrest, Log, All);

/**
 * The desk writing the player in for an event, the world's part (on a worker: the living world is a few megabytes of
 * the save's text): the field as the session's own registration describes it (ss::live::DescribeField, one place for
 * both: the people the world registered and how they play, the faces among them and what they'll say, the champions'
 * board, their voices, badges and patches, the month's room leaders), then the world told the player is in, written back
 * as text. The entry itself (the money) is made on the game thread from this (LiveDetail::RegisterFromSignup). Without a
 * world in the save, the room's own crowd. Run is in BackRoomLiveVenue.cpp.
 */
class FLiveDeskSignup
{
public:
	// In.
	std::string WorldText;
	std::string HeroName;
	/** The player's life as the desk found it (DescribeField reads it): a copy, so the worker never touches the save. */
	ss::life::State Hero;
	ss::live::Occurrence O;
	/** World minutes now: the board shows events that started before it. */
	double At = 0.0;
	// Out.
	bool bWorld = false;
	/** The field as the world has it (empty without a world: the room's own crowd). */
	ss::live::FieldNotes Notes;
	std::string WorldAfter;
	std::atomic<bool> bDone{false};

	void Run();
};

namespace LiveDetail
{
inline FString Ordinal(int32 N)
{
	const int32 Tens = N % 100;
	const TCHAR* Suffix = (Tens >= 11 && Tens <= 13) ? TEXT("th") : (N % 10 == 1 ? TEXT("st") : (N % 10 == 2 ? TEXT("nd") : (N % 10 == 3 ? TEXT("rd") : TEXT("th"))));
	return FString::Printf(TEXT("%d%s"), N, Suffix);
}

inline FString Num(int64 V)
{
	return FText::AsNumber(V).ToString();
}

inline FString Dollars(int64 Cents)
{
	return TEXT("$") + FText::AsNumber(Cents / 100).ToString();
}

inline FString Str(const std::string& S)
{
	return FString(UTF8_TO_TCHAR(S.c_str()));
}

/**
 * How the room's staff address the player: ", Dana" from the character creator's first name (the person, in person; the
 * screen name is for RiverLine), or nothing for a career from before the creator.
 */
inline FString AddressTo(const TSharedPtr<ss::SaveData>& Save)
{
	return Save.IsValid() && Save->Person.Created && !Save->Person.FirstName.empty() ? TEXT(", ") + Str(Save->Person.FirstName) : FString();
}

/** The room's body an actor wears ("ExtraC", "Sal"; empty when it has none): the folder its MetaHuman was built in (BackRoomLiveCast.cpp). */
FString WornBody(const ABackRoomPlayer* P);

/** The floor's last call for registration (the core's, so the room, the screens and the phone agree: live::LastCall). */
inline double LastCallAt(double LateRegEnds)
{
	return LateRegEnds - ss::live::LastCallMinutes;
}
/** On the rail: the old table's chairs settle a moment after each round. */
constexpr float RailSettleSeconds = 2.2f;
/** A break's fifteen minutes in the room, in real seconds (the walkable break comes with the bigger room). */
constexpr float BreakRealSeconds = 24.0f;

/**
 * Registers the player for the signup's event in D: the entry with the world's field (live::Apply: its people and faces,
 * the board, voices, badges, patches, leaders), the buy-in and fee as their ledger lines, the patches the player wears in
 * (sponsor::NoteWorn), and the world told. bInRoom: written in at the desk with the player already here, where the desk
 * sells the entry and nothing else (the night's bus home is paid: the core's rule counts a bus there and back). Fund: who
 * pays (the desk's choice, next_segment_spec 1.2; the player's own money by default). Returns why not ("" when
 * registered). In BackRoomLiveVenue.cpp.
 */
std::string RegisterFromSignup(ss::SaveData& D, const FLiveDeskSignup& S, double World, bool bInRoom, const ss::life::Funding& Fund = ss::life::Funding());
} // namespace LiveDetail
