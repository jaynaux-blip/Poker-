// ABackRoomGameMode's live tournaments: the Embercrest Casino card room's events (opened from Dee's Burner
// thread with "?Live=<occurrence id>", after ss::Session::GoToLive registered the player).
//
// The field is the player's entry (ss::life::LiveEntry): the people the living world registered and the
// room's anonymous regulars, seated by the engine's draw (ss::live::MakeField) with the entry's own seed, so
// the night is the same however often it's opened. The player's table is played hand by hand and the rest
// of the room in the background: other tables run under their pendants with extras at them and close as
// the field shrinks. Whoever the draw and the balancing put at the player's table sits there, busted players
// say goodbye and go, the floor runs the night over the PA (levels, breaks, busts near the money, hand for
// hand, the bubble, the final table), and a table break walks the player to a new seat. Busting, or
// winning, settles the entry once (ss::live::Settle) and tells the world what the player's tables saw.
//
// After that the night goes on in the room. The player's old table keeps playing the engine's rounds while they watch
// from the rail; the desk can write them into the next event today (the world's field for it, read from the save on a
// worker), the floor calls its seats when they open, and the room opens again for it with no second bus. Every way out
// leaves through the resort's entrance (the Embercrest map) when it's been built, and goes home from there.

#include "BackRoomChips.h"
#include "CareerSave.h"
#include "BackRoomGameMode.h"
#include "BackRoomPlayer.h"
#include "BackRoomStage.h"
#include "BackRoomTable.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "NightOneAudio.h"
#include "NightOneSaveGame.h"
#include "Tasks/Task.h"
#include "TimerManager.h"
#include "ShortStack/Audio/Synth.h"
#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Tournament.h"

#include <algorithm>
#include <atomic>

DEFINE_LOG_CATEGORY_STATIC(LogEmbercrest, Log, All);

namespace LiveDetail
{
FString Ordinal(int32 N)
{
	const int32 Tens = N % 100;
	const TCHAR* Suffix = (Tens >= 11 && Tens <= 13) ? TEXT("th") : (N % 10 == 1 ? TEXT("st") : (N % 10 == 2 ? TEXT("nd") : (N % 10 == 3 ? TEXT("rd") : TEXT("th"))));
	return FString::Printf(TEXT("%d%s"), N, Suffix);
}

FString Num(int64 V)
{
	return FText::AsNumber(V).ToString();
}

FString Dollars(int64 Cents)
{
	return TEXT("$") + FText::AsNumber(Cents / 100).ToString();
}

FString Str(const std::string& S)
{
	return FString(UTF8_TO_TCHAR(S.c_str()));
}

/**
 * How the room's staff address the player: ", Dana" from the character creator's first name (the person, in person; the
 * screen name is for RiverLine), or nothing for a career from before the creator.
 */
FString AddressTo(const TSharedPtr<ss::SaveData>& Save)
{
	return Save.IsValid() && Save->Person.Created && !Save->Person.FirstName.empty() ? TEXT(", ") + Str(Save->Person.FirstName) : FString();
}

/** The room's extras: low-detail MetaHumans for the other tables. */
const TCHAR* ExtraBodies[12] = {TEXT("ExtraA"), TEXT("ExtraB"), TEXT("ExtraC"), TEXT("ExtraD"), TEXT("ExtraE"), TEXT("ExtraF"), TEXT("ExtraG"),
	TEXT("ExtraH"), TEXT("ExtraI"), TEXT("ExtraJ"), TEXT("ExtraK"), TEXT("ExtraL")};
/** The room's bodies by whom they suit (ExtraA, C, E, G, I, K are women's). */
const TCHAR* WomenBodies[6] = {TEXT("ExtraA"), TEXT("ExtraC"), TEXT("ExtraE"), TEXT("ExtraG"), TEXT("ExtraI"), TEXT("ExtraK")};
const TCHAR* MenBodies[6] = {TEXT("ExtraB"), TEXT("ExtraD"), TEXT("ExtraF"), TEXT("ExtraH"), TEXT("ExtraJ"), TEXT("ExtraL")};
/**
 * The far crowd's seated figures by the body each was baked from (backroom_crowd.py bakes kind K from an extra seated at
 * a near table: kinds 0-5 and 9-14 are players, 6 a dealer). A body with no figure of its own (the cast's) has none here.
 */
const TCHAR* CrowdBodies[ABackRoomStage::CrowdKinds] = {TEXT("ExtraH"), TEXT("ExtraA"), TEXT("ExtraF"), TEXT("ExtraK"), TEXT("ExtraD"), TEXT("ExtraI"), nullptr,
	nullptr, nullptr, TEXT("ExtraG"), TEXT("ExtraL"), TEXT("ExtraE"), TEXT("ExtraJ"), TEXT("ExtraC"), TEXT("ExtraH")};

/**
 * The body an entrant's tournament id picks (LIVE_TOURNAMENTS.md section 10: a look is a hash of the id): their own
 * MetaHuman for the cast, one of the room's that suits a regular of the room's, any of the twelve for anyone else.
 */
FString DefaultBody(const FString& Id, const FString& Name)
{
	const FString Own = ABackRoomGameMode::CastAssetFor(Name);
	if (!Own.IsEmpty())
	{
		return Own;
	}
	const uint32 Hash = FCrc::StrCrc32(*Id);
	const ss::live::RoomLocal* Local = ss::live::FindRoomLocal(std::string(TCHAR_TO_UTF8(*Name)));
	return Local ? (Local->Woman ? WomenBodies : MenBodies)[Hash % 6] : ExtraBodies[Hash % 12];
}

/** The room's body an actor wears ("ExtraC", "Sal"; empty when it has none): the folder its MetaHuman was built in. */
FString WornBody(const ABackRoomPlayer* P)
{
	if (!P)
	{
		return FString();
	}
	const FString Path = P->MetaHumanClass.ToSoftObjectPath().ToString();
	const int32 At = Path.Find(TEXT("/MHC_"));
	if (At == INDEX_NONE)
	{
		return FString();
	}
	const FString Rest = Path.Mid(At + 5);
	int32 End = INDEX_NONE;
	return Rest.FindChar(TEXT('/'), End) ? Rest.Left(End) : Rest;
}

/** How one of the world's people plays at the table: as the session reads them for its own fields (Session.cpp, StyleOf). */
ss::Archetype PlayStyle(const ss::world::Npc& N)
{
	const float Sk = N.Overall();
	const float Aggro = N.TraitOf(ss::world::Trait::Aggro);
	const float Risk = N.TraitOf(ss::world::Trait::Risk);
	if (Sk >= 0.72f)
	{
		return Aggro > 0.6f ? ss::Archetype::Crusher : ss::Archetype::Reg;
	}
	if (Sk >= 0.58f)
	{
		return Aggro > 0.62f ? ss::Archetype::Lag : Aggro < 0.35f ? ss::Archetype::Tag : ss::Archetype::Reg;
	}
	if (Sk >= 0.45f)
	{
		return Aggro > 0.8f && Risk > 0.7f ? ss::Archetype::Maniac : Aggro < 0.3f ? ss::Archetype::Nit : ss::Archetype::Tag;
	}
	return Aggro > 0.8f ? ss::Archetype::Maniac : Aggro < 0.3f ? ss::Archetype::Station : N.TraitOf(ss::world::Trait::Patience) > 0.7f ? ss::Archetype::Nit : ss::Archetype::Fish;
}

/** The floor's last call for registration (the core's, so the room, the screens and the phone agree: live::LastCall). */
double LastCallAt(double LateRegEnds)
{
	return LateRegEnds - ss::live::LastCallMinutes;
}
/** The next event's seats open this long before its cards (world minutes): the floor calls them then. */
constexpr double SeatingLead = 10.0;
/** The desk stops writing players in this close to the end of late registration (there's the walk to the seat). */
constexpr double DeskClosesBefore = 5.0;
/** On the rail: a round of the room every few real seconds, and the old table's chairs settling a moment after it. */
constexpr float RailRoundSeconds = 4.5f;
constexpr float RailSettleSeconds = 2.2f;

/** What a player says leaving the tournament. */
FString Goodbye(const FString& Name)
{
	if (Name == TEXT("Sal")) return TEXT("That's me. Good luck, kid.");
	if (Name == TEXT("Mrs. Park")) return TEXT("Thirty years. Still nothing. Goodnight, everybody.");
	if (Name == TEXT("Rick")) return TEXT("Unbelievable! I'm going to the bar.");
	if (Name == TEXT("Dre")) return TEXT("That's poker, baby. Good luck, y'all.");
	if (Name == TEXT("Big Lou")) return TEXT("Lou's out! Lou is OUT! Unbelievable.");
	if (Name == TEXT("Twitch")) return TEXT("Whatever, man. This deck hates me.");
	if (Name == TEXT("Mei")) return TEXT("Good game, everyone.");
	if (Name == TEXT("gh0stfold")) return TEXT("gg. see you online.");
	return TEXT("Good luck, everyone.");
}
} // namespace LiveDetail

using namespace LiveDetail;

namespace LiveDetail
{
/** A break's fifteen minutes in the room, in real seconds (the walkable break comes with the bigger room). */
constexpr float BreakRealSeconds = 24.0f;

const TCHAR* WeekdayName(int32 Day)
{
	static const TCHAR* Names[7] = {TEXT("Monday"), TEXT("Tuesday"), TEXT("Wednesday"), TEXT("Thursday"), TEXT("Friday"), TEXT("Saturday"), TEXT("Sunday")};
	return Names[((Day % 7) + 7) % 7];
}
} // namespace LiveDetail

/**
 * The desk writing the player in for an event, the world's part (on a worker: the living world is a few megabytes of
 * the save's text): the field as the session's own registration takes it (Session::GoToLive), that is the people the
 * world registered and how they play, the faces among them (the room's regulars, whoever remembers the player) and what
 * they'll say, and the champions' board; then the world told the player is in, written back as text. The entry itself
 * (the money) is made on the game thread from this (RegisterFromSignup). Without a world in the save, the room's own crowd.
 */
class FLiveDeskSignup
{
public:
	// In.
	std::string WorldText;
	std::string HeroName;
	ss::live::Occurrence O;
	/** World minutes now: the board shows events that started before it. */
	double At = 0.0;
	// Out.
	bool bWorld = false;
	int Planned = 0;
	std::vector<std::pair<std::string, int>> Roster;
	std::vector<ss::life::LiveFace> Faces;
	std::vector<std::string> Board;
	std::string WorldAfter;
	std::atomic<bool> bDone{false};

	void Run()
	{
		ss::world::World W;
		std::vector<std::string> Fields;
		for (size_t From = 0; From < WorldText.size();)
		{
			const size_t End = FMath::Min(WorldText.find('\n', From), WorldText.size());
			Fields.clear();
			for (size_t Cell = From, K = From; K <= End; ++K)
			{
				if (K == End || WorldText[K] == '\t')
				{
					Fields.emplace_back(WorldText, Cell, K - Cell);
					Cell = K + 1;
				}
			}
			W.Read(Fields);
			From = End + 1;
		}
		WorldText.clear();
		WorldText.shrink_to_fit();
		W.Finish();
		if (W.Ready() && O.Valid())
		{
			bWorld = true;
			W.HeroName = HeroName;
			Planned = FMath::Max(1, W.PlannedEntries(O.Id));
			const std::vector<int> Regulars = W.EmbercrestRegulars();
			for (int Npc : W.Registered(O.Id))
			{
				const ss::world::Npc* N = W.Get(Npc);
				if (!N || N->Name == HeroName)
				{
					continue;
				}
				// The room's faces as the Back Room knows them play their own way.
				int Type = static_cast<int>(LiveDetail::PlayStyle(*N));
				for (const ss::live::CastMember& Cm : ss::live::EmbercrestCast())
				{
					if (N->Name == Cm.Name)
					{
						Type = static_cast<int>(Cm.Type);
					}
				}
				Roster.push_back({N->Name, Type});
				const ss::world::Bond* Bd = W.BondWith(Npc);
				const bool bRegular = std::find(Regulars.begin(), Regulars.end(), Npc) != Regulars.end();
				const std::string Known = Bd ? Bd->Label() : std::string();
				if (!bRegular && Known.empty() && !(Bd && !Bd->Memories.empty()))
				{
					continue;
				}
				ss::life::LiveFace F;
				F.Name = N->Name;
				F.Label = !Known.empty() ? Known : Bd && !Bd->Memories.empty() ? std::string("Has met you") : std::string("Embercrest regular");
				F.Line = W.SpokenGreeting(Npc, ss::live::FaceHash(O.Id + N->Name));
				Faces.push_back(F);
			}
			// The champions' board by the desk: the room's last winners, newest first.
			static const char* Days[7] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
			for (int Day = O.Day; Day >= O.Day - 10 && Board.size() < 8; --Day)
			{
				const std::vector<ss::live::Occurrence> Os = ss::live::Occurrences(Day);
				for (auto It = Os.rbegin(); It != Os.rend() && Board.size() < 8; ++It)
				{
					const ss::net::EventResult* Res = It->Start < At ? W.ResultOf(It->Id) : nullptr;
					if (!Res)
					{
						continue;
					}
					for (const ss::net::Placing& Pl : Res->FinalTable)
					{
						if (Pl.Place != 1)
						{
							continue;
						}
						const ss::world::Npc* N = Pl.Player >= 0 ? W.Get(Pl.Player) : nullptr;
						const std::string Who = Pl.Player == -1 ? HeroName : N ? N->Name : Pl.Name;
						Board.push_back(std::string(Days[((Day % 7) + 7) % 7]) + " " + std::string(TCHAR_TO_UTF8(*FString(It->T->Short).ToUpper())) + "\t" + Who + "\t" +
							std::to_string(Pl.Prize));
					}
				}
			}
			// The world waits for the player's result in it (it never decides an event the player is in).
			W.HeroEntered(O.Id, O.T->Name);
			W.Write(WorldAfter);
		}
		bDone = true;
	}
};

namespace LiveDetail
{
/**
 * Registers the player for the signup's event in D: the entry with the world's field (its people and faces, the board),
 * the buy-in and fee as their ledger lines, and the world told. bInRoom: written in at the desk with the player already
 * here, where the desk sells the entry and nothing else (the night's bus home is paid: the core's rule counts a bus
 * there and back). Returns why not ("" when registered).
 */
std::string RegisterFromSignup(ss::SaveData& D, const FLiveDeskSignup& S, double World, bool bInRoom)
{
	const ss::live::Occurrence& O = S.O;
	const int Entrants = S.bWorld ? FMath::Max(S.Planned, static_cast<int>(S.Roster.size())) + 1 : O.Field + 1;
	const ss::Chips Waived = bInRoom ? 2 * ss::live::BusFareCents : 0;
	ss::Chips Bank = D.BankrollCents + Waived;
	const std::string Why = ss::live::Register(Bank, D.Life, O, World, Entrants, S.Roster);
	D.BankrollCents = Bank - Waived;
	ss::life::LiveEntry* E = Why.empty() ? ss::live::EntryFor(D.Life, O.Id) : nullptr;
	if (!E)
	{
		return Why.empty() ? std::string("There's no such event.") : Why;
	}
	E->Faces = S.Faces;
	E->Board = S.Board;
	if (S.bWorld)
	{
		D.WorldText = S.WorldAfter;
	}
	return std::string();
}

/** A room console command's game mode: the Embercrest's, or nothing (Dee's game, practice, another map). */
ABackRoomGameMode* LiveMode(UWorld* World)
{
	ABackRoomGameMode* GM = World ? World->GetAuthGameMode<ABackRoomGameMode>() : nullptr;
	return GM && GM->IsLive() ? GM : nullptr;
}

FAutoConsoleCommandWithWorldAndArgs BustCmd(TEXT("ss.Live.Bust"),
	TEXT("At the Embercrest: the player busts at the next gap between hands (testing: a one-chip stack, blinded out by the engine)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			GM->TestBustNow();
		}
	}));
FAutoConsoleCommandWithWorldAndArgs LastCallCmd(TEXT("ss.Live.LastCall"),
	TEXT("At the Embercrest: the room plays on (you on autopilot) to just before the floor's last call for registration."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			GM->TestLastCall();
		}
	}));
FAutoConsoleCommandWithWorldAndArgs NextCmd(TEXT("ss.Live.Next"),
	TEXT("ss.Live.Next [occurrence id]: once the player's night is over, the desk writes them into the next event today (or that one), as E twice at the desk does."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			GM->TestRegisterNext(Args.Num() > 0 ? Args[0] : FString());
		}
	}));
FAutoConsoleCommandWithWorldAndArgs SeatCmd(TEXT("ss.Live.Seat"),
	TEXT("At the Embercrest, registered for the next event: the wait passes (as the terrace does) and the floor calls its seats."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) {
		if (ABackRoomGameMode* GM = LiveMode(World))
		{
			GM->TestSeatingNow();
		}
	}));
} // namespace LiveDetail

FString ABackRoomGameMode::Chips(int64 Amount) const
{
	return Num(Amount);
}

// ------------------------------------------------------------------ registration

bool ABackRoomGameMode::LoadLive()
{
	FString Event = UGameplayStatics::ParseOption(OptionsString, TEXT("Live"));
	if (Event.IsEmpty())
	{
		return false;
	}
	if (Event == TEXT("embercrest"))
	{
		// From before the daily schedule (and the editor's test hooks): that day's evening event.
		const FString DayOption = UGameplayStatics::ParseOption(OptionsString, TEXT("Day"));
		const int32 Day = DayOption.IsEmpty() ? 6 : FCString::Atoi(*DayOption);
		for (const ss::live::Occurrence& O : ss::live::Occurrences(Day))
		{
			if (Event == TEXT("embercrest") || O.T->StartMinute == 19 * 60)
			{
				Event = Str(O.Id);
			}
		}
	}
	std::string CareerText;
	TSharedPtr<ss::SaveData> D = MakeShared<ss::SaveData>();
	if (!CareerSave::LoadText(CareerText) || !ss::SaveData::Parse(CareerText, *D))
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("Live=%s but no career save: practice table."), *Event);
		return false;
	}
	const ss::live::Occurrence O = ss::live::FindOccurrence(std::string(TCHAR_TO_UTF8(*Event)));
	if (!O.Valid())
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("Live=%s isn't one of the Embercrest's events: practice table."), *Event);
		return false;
	}
	LeftHomeAt = ss::net::MinutesPerDay * ss::net::NightOneDay + D->ClockMinutes;
	ss::life::LiveEntry* Entry = ss::live::EntryFor(D->Life, O.Id);
	if (!Entry)
	{
		// Opened directly (the editor, a test): registered at the desk on arrival, with the field the world has for it.
		const double At = FMath::Max(LeftHomeAt + ss::live::TravelMinutes, O.DeskOpens);
		FLiveDeskSignup Field;
		Field.WorldText = D->WorldText;
		Field.HeroName = D->HeroName;
		Field.O = O;
		Field.At = At;
		Field.Run();
		const std::string Why = RegisterFromSignup(*D, Field, At - ss::live::TravelMinutes, false);
		Entry = Why.empty() ? ss::live::EntryFor(D->Life, O.Id) : nullptr;
		if (!Entry)
		{
			UE_LOG(LogEmbercrest, Warning, TEXT("Can't register for %s (%s): practice table."), *Event, *Str(Why));
			return false;
		}
		LeftHomeAt = FMath::Max(LeftHomeAt, At - ss::live::TravelMinutes);
		ss::live::PayFare(D->BankrollCents, D->Life, O.Id, false, LeftHomeAt);
	}
	if (Entry->State != ss::life::LiveEntry::Registered)
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("%s is already settled: practice table."), *Event);
		return false;
	}
	// Back after the game closed during the night (the entry says the player got here): no bus this time, and
	// nothing was dealt while the game was closed.
	bBackInRoom = Entry->ArrivedAt > 0.0;
	if (bBackInRoom)
	{
		LeftHomeAt = Entry->ArrivedAt - ss::live::TravelMinutes;
	}
	// An event the desk wrote the player into during the night, already in the room (it rides home on the first event's
	// bus), and the night's own departure, which Night One catches its calendar up from.
	bDeskEntry = bBackInRoom && Entry->ArrivedAt <= Entry->RegisteredAt + 0.01;
	NightLeftHomeAt = LeftHomeAt;
	if (bDeskEntry && Entry->Checkpoint.empty())
	{
		// (Before its first checkpoint, the desk's entry keeps that departure itself.)
		TArray<FString> Lines;
		Str(Entry->CheckpointHost).ParseIntoArray(Lines, TEXT("\n"), true);
		for (const FString& Line : Lines)
		{
			FString Key, Value;
			if (Line.Split(TEXT("\t"), &Key, &Value) && Key == TEXT("from"))
			{
				NightLeftHomeAt = FMath::Min(NightLeftHomeAt, FCString::Atod(*Value));
			}
		}
	}
	// Straight on from the night's earlier event in this room (the desk's): no bus and no trip, the clock as the room left it.
	bHere = bBackInRoom && UGameplayStatics::HasOption(OptionsString, TEXT("Here"));
	double HereAt = 0.0;
	if (bHere)
	{
		const FString From = UGameplayStatics::ParseOption(OptionsString, TEXT("From"));
		NightLeftHomeAt = From.IsEmpty() ? NightLeftHomeAt : FMath::Min(NightLeftHomeAt, FCString::Atod(*From));
		HereAt = FCString::Atod(*UGameplayStatics::ParseOption(OptionsString, TEXT("At")));
	}
	Save = D;
	for (const ss::life::LiveFace& F : Entry->Faces)
	{
		LiveFaces.Add(Str(F.Name), TPair<FString, FString>(Str(F.Label), Str(F.Line)));
	}
	LiveEntryId = Str(O.Id);
	LiveName = Str(O.T->Name);
	LiveShort = Str(O.T->Short);
	LiveLateRegEnds = O.LateRegEnds;
	LiveBreakMinutes = O.T->BreakMinutes;
	// Up the ridge on the 14 bus (or, on from the night's earlier event, already here: its hours count from now).
	Minutes = LeftHomeAt + ss::live::TravelMinutes;
	if (bHere)
	{
		Minutes = FMath::Max(Minutes, HereAt);
		LeftHomeAt = Minutes - ss::live::TravelMinutes;
	}
	LiveDay = O.Day;
	LiveDayStart = ss::net::MinutesPerDay * LiveDay;
	Energy = static_cast<float>(D->Life.Energy);
	PastLiveEvents = D->Life.LiveEvents;
	PastLiveCashes = D->Life.LiveCashes;
	PastBestPlace = D->Life.LiveBestPlace;
	PastNights = D->Life.BackRoomNights;
	PastNetCents = D->Life.BackRoomNetCents;
	bFirstVisit = false;
	// Paid at registration: the bankroll already shows it, the chips are on the table.
	StartBankrollCents = D->BankrollCents;
	BoughtInCents = Entry->PaidCents;
	BaseCents = StartBankrollCents;
	// The field the entry was registered into, drawn with its own seed.
	std::vector<ss::ReservedPlayer> Known;
	for (const std::pair<std::string, int>& Who : Entry->Roster)
	{
		ss::ReservedPlayer P;
		P.Name = Who.first;
		P.Type = static_cast<ss::Archetype>(Who.second);
		Known.push_back(P);
	}
	bCancelled = ss::live::BelowMinimum(O, Entry->Entrants);
	const bool bRestore = bBackInRoom && !Entry->Checkpoint.empty();
	bool bFits = false;
	Tourney = MakeShareable(ss::live::ResumeField(O, *Entry, FMath::Max(Entry->Entrants, 2), D->HeroName, Known, bRestore, bFits).release());
	HeroBuyInChips = Tourney->Spec.StartingStack;
	ABackRoomChips::ChipUnit = 100;
	LiveSaver = MakeShared<FCareerSaver>();
	if (bRestore)
	{
		// The room as it stood after the last hand the player saw.
		bResumed = bFits;
		if (bResumed)
		{
			Minutes = FMath::Max(Minutes, Entry->CheckpointAt);
			ReadLiveHost(Str(Entry->CheckpointHost));
			LastCheckpoint = Entry->Checkpoint;
			LastCheckpointAt = Entry->CheckpointAt;
		}
		UE_CLOG(!bResumed, LogEmbercrest, Warning, TEXT("%s: the checkpoint doesn't fit the field; the night starts over"), *LiveName);
	}
	if (!bBackInRoom)
	{
		// Here: the entry remembers it (a game closed from now on comes back to this room).
		Entry->ArrivedAt = Minutes;
		ss::SaveData Arrived = *Save;
		Arrived.ClockMinutes = Minutes - ss::net::MinutesPerDay * ss::net::NightOneDay;
		CareerSave::SaveNow(Arrived.Serialize());
	}
	Phase = EBackRoomPhase::Arriving;
	ArrivalDayText = FString::Printf(TEXT("%s   %s"), WeekdayName(LiveDay), *FString(UTF8_TO_TCHAR(ss::net::DateLabel(LiveDay).c_str())));
	UE_LOG(LogEmbercrest, Log, TEXT("%s: %d entrants (%d the world follows), pool %lld cents, %d paid; bankroll %lld; arriving %s"), *LiveName, Tourney->Spec.Entrants,
		static_cast<int32>(Known.size()), static_cast<int64>(Tourney->PrizePoolCents), Tourney->PaidPlaces(), StartBankrollCents, *ClockLabel());
	{
		FString Faces;
		for (const TPair<FString, TPair<FString, FString>>& F : LiveFaces)
		{
			Faces += (Faces.IsEmpty() ? TEXT("") : TEXT(", ")) + F.Key + TEXT(" (") + F.Value.Key + TEXT(")");
		}
		UE_CLOG(!Faces.IsEmpty(), LogEmbercrest, Log, TEXT("Faces in the field: %s"), *Faces);
	}
	UE_CLOG(bHere, LogEmbercrest, Log, TEXT("On from the night's earlier event: no bus, %s, the night left home at %s"), *ClockLabel(), *Str(ss::net::TimeLabel(NightLeftHomeAt)));
	UE_CLOG(bBackInRoom && !bHere, LogEmbercrest, Log, TEXT("Back in the room%s: hand %d, level %d, %d left, stack %lld, %d hands played%s"), bResumed ? TEXT(" from the checkpoint") : TEXT(""),
		Tourney->Tick, Tourney->LevelIndex + 1, Tourney->Remaining, static_cast<int64>(Tourney->Hero().Stack), HandsPlayed, bResumeDealt ? TEXT(", the dealt hand is dead") : TEXT(""));
	return true;
}

void ABackRoomGameMode::SeatLive()
{
	Table->bCardRoomTone = true;
	Table->bFirstVisit = false;
	Table->SetTournament(Tourney);
	TWeakObjectPtr<ABackRoomGameMode> Self = this;
	Table->OnSeatPlayer = [Self](const ss::TPlayer& P, int32 Seat) -> ABackRoomPlayer* {
		ABackRoomGameMode* GM = Self.Get();
		return GM ? GM->SeatCast(P, Seat) : nullptr;
	};
	Table->OnUnseatPlayer = [Self](ABackRoomPlayer* A, const FString& Id, bool bBusted) {
		if (ABackRoomGameMode* GM = Self.Get())
		{
			GM->UnseatCast(A, Id, bBusted);
		}
	};
	// Doors open at six; nobody's walking in much before ten to seven.
	const double CardsAt = LiveDayStart + Tourney->Spec.StartClock;
	Minutes = FMath::Max(Minutes, CardsAt - 8.0);
	// Late: the field has played the levels you missed. Inside late registration your seat waits with a full stack;
	// after it (back to an entry you'd left), the seat was dealt in without you, and the blinds took their share. Written
	// in at the desk tonight, the seat waits however long the floor's call and the walk took: the registration was in time.
	const double SitAt = Minutes + 2.0;
	if (SitAt > CardsAt && !bResumed)
	{
		const bool bReturning = SitAt > LiveLateRegEnds && !bDeskEntry;
		const int32 Missed = FMath::FloorToInt((SitAt - CardsAt) * 60.0 / Tourney->Spec.SecondsPerHand);
		Tourney->HeroAway = !bReturning;
		Tourney->HeroSitsOut = bReturning;
		for (int32 I = 0; I < Missed && !Tourney->bFinished && !Tourney->Hero().Busted; ++I)
		{
			Tourney->SimulateTick();
		}
		Tourney->HeroAway = false;
		Tourney->HeroSitsOut = false;
		UE_LOG(LogEmbercrest, Log, TEXT("%s: %d hands played without you, level %d, %d left"), bReturning ? TEXT("Back after leaving") : TEXT("Late registration"), Missed,
			Tourney->LevelIndex + 1, Tourney->Remaining);
	}
	// The hand that was dealt when the game closed is dead: the player's cards go in the muck (checked through when it's
	// free), the room plays it out, and the night goes on from the next hand. Nobody sees the same deck twice.
	std::vector<ss::TEvent> DeadHand;
	if (bResumed && bResumeDealt && !Tourney->Hero().Busted && !Tourney->bFinished)
	{
		Tourney->HeroSitsOut = true;
		DeadHand = Tourney->SimulateTick();
		Tourney->HeroSitsOut = false;
		Minutes = FMath::Max(Minutes, LiveDayStart + Tourney->ClockMinutes());
	}
	if (bResumed)
	{
		// Where the night stands: the stage, hand for hand, what the player already knew of the regulars.
		bFinalTable = Tourney->Tables.size() == 1;
		bHandForHand = Tourney->HandForHand();
		bAnnouncedStart = true;
		for (const TPair<FString, int32>& R : ResumeReads)
		{
			Table->Reads.Add(R.Key, R.Value);
		}
		RaiseBanner(FString::Printf(TEXT("LEVEL %d   %s / %s   %d LEFT"), Tourney->LevelIndex + 1, *Num(Tourney->CurrentLevel().Sb), *Num(Tourney->CurrentLevel().Bb), Tourney->Remaining));
	}
	ArrivalDayText = FString::Printf(TEXT("%s, %s   %s"), WeekdayName(LiveDay), *FString(UTF8_TO_TCHAR(ss::net::DateLabel(LiveDay).c_str())), *ClockLabel().RightChop(5));
	// The floor's last call and the close of registration are news only to a room that's here for them.
	const double SeatedAt = FMath::Max(Minutes, LiveDayStart + Tourney->ClockMinutes());
	bLastCalled = SeatedAt >= LastCallAt(LiveLateRegEnds);
	bRegClosed = SeatedAt >= LiveLateRegEnds;
	// The night as it stands before the first hand here is dealt (so even that hand is never dealt twice).
	LiveCheckpoint(false);
	// The first table, seated before you get there; the room placed round it, the rest of the room at theirs.
	Table->PrepareNext();
	Table->TakeEvents();
	for (const ss::TEvent& E : DeadHand)
	{
		LiveEvent(E);
	}
	if (bOnBreak)
	{
		Table->Hold();
	}
	if (bFinalTable && Stage)
	{
		Stage->SetOnAir(true);
	}
	if (Stage)
	{
		Stage->SetRoomAnchor(SlotForTable(Tourney->Hero().TableId));
		// The champions' board as it stood tonight, and the cash list on the desk's whiteboard.
		TArray<FString> Champions;
		if (const ss::life::LiveEntry* Mine = ss::live::EntryFor(Save->Life, std::string(TCHAR_TO_UTF8(*LiveEntryId))))
		{
			for (const std::string& B : Mine->Board)
			{
				TArray<FString> Parts;
				Str(B).ParseIntoArray(Parts, TEXT("\t"), false);
				if (Parts.Num() == 3)
				{
					Champions.Add(FString::Printf(TEXT("%s     %s     %s"), *Parts[0], *Parts[1].ToUpper(), *Dollars(FCString::Atoi64(*Parts[2]))));
				}
			}
		}
		if (Champions.Num() == 0)
		{
			Champions.Add(TEXT("YOUR NAME HERE"));
		}
		Stage->SetChampions(Champions);
		static const TCHAR* Regulars[] = {TEXT("BOOTS"), TEXT("T.J."), TEXT("MARIA G."), TEXT("OMAR"), TEXT("LIN"), TEXT("DUKE"), TEXT("BEV"), TEXT("SONNY"), TEXT("J.P."), TEXT("ROSIE")};
		TArray<FString> Cash = {TEXT("1-2 NLH     3 TABLES")};
		FString List = TEXT("LIST:");
		for (int32 I = 0; I < 5; ++I)
		{
			List += TEXT("  ") + FString(Regulars[(LiveDay * 3 + I * 7) % 10]);
		}
		Cash.Add(List);
		Cash.Add(TEXT("2-5 NLH     1 TABLE"));
		Cash.Add(TEXT("4-8 LIMIT   FRI & SAT"));
		Stage->SetCashList(Cash);
	}
	PlaceExtras();
	UpdateRoomBoards();
}

// ------------------------------------------------------------------ people

FString ABackRoomGameMode::FaceNote(const FString& Name) const
{
	const TPair<FString, FString>* Face = LiveFaces.Find(Name);
	return Face ? Face->Key : FString();
}

ABackRoomPlayer* ABackRoomGameMode::SeatCast(const ss::TPlayer& P, int32 TableSeat)
{
	const FString Id = Str(P.Id);
	const FString Name = Str(P.Name);
	const FTransform At = ABackRoomStage::SeatTransform(TableSeat);
	ABackRoomPlayer* A = nullptr;
	if (TObjectPtr<ABackRoomPlayer>* Found = CastActors.Find(Id))
	{
		A = Found->Get();
	}
	if (!A)
	{
		// Their own face (the cast), or the room's body they've worn at another table tonight, or the one their id picks
		// (the same every night they play), passing over any already sitting at this table and the dealer's.
		TSet<FString> Worn;
		for (const TPair<FString, TObjectPtr<ABackRoomPlayer>>& It : CastActors)
		{
			const ABackRoomPlayer* O = It.Value.Get();
			const bool bSeated = O && (!O->IsHidden() || BlinkSwaps.ContainsByPredicate([O](const TPair<TWeakObjectPtr<ABackRoomPlayer>, bool>& S) {
				return S.Key.Get() == O && S.Value;
			}));
			if (bSeated)
			{
				Worn.Add(WornBody(O));
			}
		}
		Worn.Add(WornBody(Dealer.Get()));
		A = SpawnPerson(LiveBodyFor(Id, Name, Worn), At, EBackRoomRole::Player, PersonaFor(Name));
		CastActors.Add(Id, A);
	}
	A->SetActorTransform(At);
	// Someone who knows the player (or a regular the dealer knows), sitting down with them for the first time tonight:
	// a word once they're settled. (Not once the player's night is over: they sit down at the old table without them.)
	const TPair<FString, FString>* Face = LiveMet.Contains(Id) || bLiveOver ? nullptr : LiveFaces.Find(Name);
	if (Face)
	{
		const FString Line = Face->Value;
		const bool bRegular = Face->Key == TEXT("Embercrest regular");
		const float Delay = (Phase == EBackRoomPhase::Arriving ? 11.0f : 2.4f) + 3.4f * static_cast<float>(GreetCount++ % 3);
		TWeakObjectPtr<ABackRoomPlayer> Who = A;
		TWeakObjectPtr<ABackRoomTable> Tb = Table.Get();
		const int32 Pick = static_cast<int32>(FCrc::StrCrc32(*(Name + LiveEntryId)) % 3);
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [Who, Tb, Line, bRegular, Name, Pick]() {
			ABackRoomPlayer* P = Who.Get();
			if (!P || P->IsHidden())
			{
				return;
			}
			if (!Line.IsEmpty())
			{
				P->Say(Line);
			}
			else if (bRegular && Tb.IsValid())
			{
				Tb->DealerLine(Pick == 0 ? FString::Printf(TEXT("Hey, %s."), *Name)
							   : Pick == 1 ? FString::Printf(TEXT("Good to see you, %s."), *Name)
										   : FString::Printf(TEXT("%s. Good luck tonight."), *Name));
			}
		}), Delay, false);
	}
	if (!bLiveOver)
	{
		LiveMet.Add(Id);
	}
	// In plain sight (from the chair, or from the rail), a new face takes the chair in a blink rather than appearing in it.
	BlinkSwaps.RemoveAll([A](const TPair<TWeakObjectPtr<ABackRoomPlayer>, bool>& S) { return S.Key.Get() == A; });
	if ((Phase == EBackRoomPhase::Playing || bRailing) && MoveT < 0.0f && HeroCanSee(At.GetLocation() + FVector(0.0, 0.0, 110.0)))
	{
		A->SetActorHiddenInGame(true);
		BlinkSwap(A, true);
	}
	else
	{
		A->SetActorHiddenInGame(false);
	}
	// Where everyone is, for their eyes.
	A->HeroEyes = Stage ? Stage->EyeLocation() : FVector(-98.0, 0.0, 124.0);
	A->PotAt = FVector(10.0, 0.0, ABackRoomStage::FeltZ);
	A->DealerAt = ABackRoomStage::SeatTransform(4).TransformPosition(FVector(-14.0, 0.0, 112.0));
	A->OthersAt.Reset();
	for (int32 S : {1, 2, 3, 5, 6})
	{
		if (S != TableSeat)
		{
			A->OthersAt.Add(ABackRoomStage::SeatTransform(S).TransformPosition(FVector(-14.0, 0.0, 112.0)));
		}
	}
	if (!Face && Phase == EBackRoomPhase::Playing && bAnnouncedStart && Dealer && FMath::FRand() < 0.6f)
	{
		Table->DealerLine(FString::Printf(TEXT("Welcome. Seat %d."), ss::live::SeatLabel(P.Seat)));
	}
	// gh0stfold, across a real table for the first time.
	if (Name == UTF8_TO_TCHAR(ss::RivalName) && !bGhostMet && !bLiveOver)
	{
		bGhostMet = true;
		TWeakObjectPtr<ABackRoomPlayer> Ghost = A;
		const FString HeroName = Save.IsValid() ? Str(Save->HeroName) : FString(TEXT("grinder_3c"));
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [Ghost, HeroName]() {
			if (ABackRoomPlayer* G = Ghost.Get())
			{
				G->Say(FString::Printf(TEXT("%s? From RiverLine? Huh. I've got two thousand hands on you."), *HeroName));
			}
		}), Phase == EBackRoomPhase::Arriving ? 9.0f : 3.0f, false);
	}
	return A;
}

void ABackRoomGameMode::UnseatCast(ABackRoomPlayer* Player, const FString& Id, bool bBusted)
{
	if (!Player)
	{
		return;
	}
	// Up and gone (the body waits out of sight in case the floor brings them back); in front of the hero, in a blink.
	if ((Phase == EBackRoomPhase::Playing || bRailing) && MoveT < 0.0f && HeroCanSee(Player->GetActorLocation() + FVector(0.0, 0.0, 110.0)))
	{
		BlinkSwap(Player, false);
	}
	else
	{
		Player->SetActorHiddenInGame(true);
		Player->SetActorLocation(FVector(0.0, 0.0, -5000.0));
	}
	if (!bBusted && Dealer && Phase == EBackRoomPhase::Playing && LiveT - LastFarewellAt > 20.0f)
	{
		LastFarewellAt = LiveT;
		Table->DealerLine(TEXT("Good luck at the new table."));
	}
}

bool ABackRoomGameMode::HeroCanSee(const FVector& At) const
{
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC || !PC->PlayerCameraManager)
	{
		return false;
	}
	const FVector To = (At - PC->PlayerCameraManager->GetCameraLocation()).GetSafeNormal();
	// Half the (horizontal, so wider) field of view, and a margin for a body's width.
	const float Half = FMath::DegreesToRadians(FMath::Min(PC->PlayerCameraManager->GetFOVAngle() * 0.5f + 14.0f, 89.0f));
	return FVector::DotProduct(PC->PlayerCameraManager->GetCameraRotation().Vector(), To) > FMath::Cos(Half);
}

void ABackRoomGameMode::BlinkSwap(ABackRoomPlayer* Player, bool bShow)
{
	BlinkSwaps.Add({Player, bShow});
	if (BlinkT >= 0.0f)
	{
		return; // this blink covers it
	}
	BlinkT = 0.0f;
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 0.07f, FLinearColor::Black, false, true);
	}
}

void ABackRoomGameMode::ApplyBlinkSwaps()
{
	UE_LOG(LogEmbercrest, Log, TEXT("Seat change in a blink: %d bodies"), BlinkSwaps.Num());
	for (const TPair<TWeakObjectPtr<ABackRoomPlayer>, bool>& S : BlinkSwaps)
	{
		if (ABackRoomPlayer* A = S.Key.Get())
		{
			A->SetActorHiddenInGame(!S.Value);
			if (!S.Value)
			{
				A->SetActorLocation(FVector(0.0, 0.0, -5000.0));
			}
		}
	}
	BlinkSwaps.Reset();
}

int32 ABackRoomGameMode::SlotForTable(int32 TableId)
{
	// Every table has its place: table N stands at slot N - 1, and the final table plays on the stage.
	if (Tourney && Tourney->Tables.size() == 1)
	{
		return ABackRoomStage::StreamSlot();
	}
	return FMath::Clamp(TableId - 1, 0, ABackRoomStage::StreamSlot() - 1);
}

void ABackRoomGameMode::PlaceExtras()
{
	if (!Stage || !Tourney)
	{
		return;
	}
	// The room's fidelity tiers (docs/LIVE_TOURNAMENTS.md section 4): the nearest tables get MetaHumans, as many as
	// the budget seats; every other running table gets the crowd kit's figures. Either way, a table shows exactly
	// the players the tournament has at it, and its dealer.
	constexpr int32 NearBudget = 16;
	constexpr double NearReach = 1150.0;
	static const int32 PhysicalSeat[6] = {0, 1, 2, 3, 5, 6};
	const int32 Anchor = Stage->GetRoomAnchor();
	const FVector Me = ABackRoomStage::TableSlotLocal(Anchor).GetLocation();
	const int32 HeroTable = Tourney->Hero().Busted ? -1 : Tourney->Hero().TableId;
	USceneComponent* RoomRoot = Stage->GetRoomRoot();
	const FTransform RoomWorld = RoomRoot ? RoomRoot->GetComponentTransform() : FTransform::Identity;
	struct FRunning
	{
		int32 Slot;
		int32 TableId;
		double Dist;
	};
	TArray<FRunning> Running;
	TSet<int32> Open;
	for (const auto& It : Tourney->Tables)
	{
		// The player's table has its own people, and after the player is out of it, it goes on showing the table that
		// plays there (ABackRoomTable::FollowTable).
		const int32 Slot = SlotForTable(It.first);
		if (It.first == HeroTable || Slot == Anchor)
		{
			continue;
		}
		Open.Add(Slot);
		Running.Add({Slot, It.first, FVector::Dist2D(Me, ABackRoomStage::TableSlotLocal(Slot).GetLocation())});
	}
	Running.Sort([](const FRunning& A, const FRunning& B) { return A.Dist < B.Dist; });
	for (int32 Slot = 0; Slot < ABackRoomStage::NumTableSlots(); ++Slot)
	{
		Stage->SetTableOpen(Slot, Open.Contains(Slot));
	}
	Stage->ClearCrowd();
	// Who sits where at the near tables: each player in the body they wear in the room (LiveBodyFor: the same as at the
	// player's own table, so a face follows its person from table to table) and dressed as themselves; each table's
	// dealer in the house's black, the same face all night, never the player's own dealer's.
	struct FNearSeat
	{
		FTransform Frame;
		int32 Physical = 0;
		FString Who;
		FString Name;
		FString Body;
		bool bDealer = false;
	};
	TArray<FNearSeat> Near;
	const FString HouseDealer = WornBody(Dealer.Get());
	int32 Budget = NearBudget;
	for (const FRunning& R : Running)
	{
		const ss::TTable& T = Tourney->Tables[R.TableId];
		const FTransform Frame = ABackRoomStage::TableSlotLocal(R.Slot);
		TArray<TPair<int32, int32>> Players; // physical seat, player index
		for (size_t K = 0; K < T.Seats.size() && K < 6; ++K)
		{
			if (T.Seats[K] >= 0)
			{
				Players.Add(TPair<int32, int32>(PhysicalSeat[K], T.Seats[K]));
			}
		}
		if (R.Dist < NearReach && 1 + Players.Num() <= Budget)
		{
			Budget -= 1 + Players.Num();
			FNearSeat& D = Near.AddDefaulted_GetRef();
			D.Frame = Frame;
			D.Physical = 4;
			D.Who = FString::Printf(TEXT("dealer:%d"), R.Slot);
			D.Name = FString::Printf(TEXT("Dealer %d"), R.Slot + 1);
			D.Body = ExtraBodies[(R.Slot * 7 + 3) % 12];
			for (int32 K = 1; D.Body == HouseDealer && K < 12; ++K)
			{
				D.Body = ExtraBodies[(R.Slot * 7 + 3 + K) % 12];
			}
			D.bDealer = true;
			TSet<FString> Worn = {D.Body, HouseDealer};
			for (const TPair<int32, int32>& P : Players)
			{
				const ss::TPlayer& Player = Tourney->Players[static_cast<size_t>(P.Value)];
				FNearSeat& S = Near.AddDefaulted_GetRef();
				S.Frame = Frame;
				S.Physical = P.Key;
				S.Who = Str(Player.Id);
				S.Name = Str(Player.Name);
				S.Body = LiveBodyFor(S.Who, S.Name, Worn);
				Worn.Add(S.Body);
			}
			continue;
		}
		Stage->AddCrowd(6, ABackRoomStage::SeatTransform(4) * Frame);
		for (const TPair<int32, int32>& P : Players)
		{
			// Their look is theirs: the figure baked from the body they wear up close.
			const ss::TPlayer& Player = Tourney->Players[static_cast<size_t>(P.Value)];
			Stage->AddCrowd(CrowdKindFor(Str(Player.Id), Str(Player.Name)), ABackRoomStage::SeatTransform(P.Key) * Frame);
		}
	}
	// Their bodies: first the extras already sitting for them, then a free extra of that body dressed as them, else a new one.
	ExtraBody.SetNum(Extras.Num());
	ExtraWho.SetNum(Extras.Num());
	TArray<bool> Taken;
	Taken.Init(false, Extras.Num());
	TArray<int32> ActorFor;
	ActorFor.Init(INDEX_NONE, Near.Num());
	TSet<FString> Needed;
	for (const FNearSeat& S : Near)
	{
		Needed.Add(S.Body);
	}
	for (int32 N = 0; N < Near.Num(); ++N)
	{
		for (int32 I = 0; I < Extras.Num(); ++I)
		{
			if (!Taken[I] && Extras[I] && ExtraWho[I] == Near[N].Who && ExtraBody[I] == Near[N].Body)
			{
				ActorFor[N] = I;
				Taken[I] = true;
				break;
			}
		}
	}
	for (int32 N = 0; N < Near.Num(); ++N)
	{
		if (ActorFor[N] != INDEX_NONE)
		{
			continue;
		}
		const FNearSeat& S = Near[N];
		FBackRoomPersona Persona = PersonaFor(S.Name);
		if (S.bDealer)
		{
			// The house's black, nothing on the head.
			Persona.Shirt = FLinearColor(FColor(0x14, 0x14, 0x18));
			Persona.ShirtPrint = 0;
			Persona.ShirtGraphic = 0;
			Persona.Headwear = 0;
		}
		for (int32 I = 0; I < Extras.Num(); ++I)
		{
			if (!Taken[I] && Extras[I] && ExtraBody[I] == S.Body)
			{
				ActorFor[N] = I;
				Extras[I]->Persona = Persona;
				Extras[I]->Redress();
				break;
			}
		}
		if (ActorFor[N] == INDEX_NONE)
		{
			ABackRoomPlayer* E = SpawnPerson(S.Body, ABackRoomStage::SeatTransform(S.Physical) * S.Frame * RoomWorld, EBackRoomRole::Extra, Persona);
			if (RoomRoot)
			{
				E->AttachToComponent(RoomRoot, FAttachmentTransformRules::KeepWorldTransform);
			}
			// Past what the near tables seat at once (and a few to spare), one standing by in a body nobody here needs now
			// makes way for it: the hidden still animate.
			int32 Spare = INDEX_NONE;
			for (int32 I = 0; Extras.Num() >= NearBudget + 6 && I < Extras.Num() && Spare == INDEX_NONE; ++I)
			{
				Spare = !Taken[I] && (!Extras[I] || !Needed.Contains(ExtraBody[I])) ? I : INDEX_NONE;
			}
			if (Spare != INDEX_NONE)
			{
				if (Extras[Spare])
				{
					Extras[Spare]->Destroy();
				}
				Extras[Spare] = E;
				ExtraBody[Spare] = S.Body;
				ActorFor[N] = Spare;
			}
			else
			{
				ActorFor[N] = Extras.Add(E);
				ExtraBody.Add(S.Body);
				ExtraWho.Add(FString());
				Taken.Add(false);
			}
		}
		ExtraWho[ActorFor[N]] = S.Who;
		Taken[ActorFor[N]] = true;
	}
	for (int32 N = 0; N < Near.Num(); ++N)
	{
		const FNearSeat& S = Near[N];
		ABackRoomPlayer* E = Extras[ActorFor[N]];
		E->SetActorRelativeTransform(ABackRoomStage::SeatTransform(S.Physical) * S.Frame);
		E->SetActorHiddenInGame(false);
		// They watch their own table: the pot, the dealer, the others at it.
		const FTransform World = S.Frame * RoomWorld;
		E->TableToWorld = World;
		E->PotAt = World.TransformPosition(FVector(10.0, 0.0, ABackRoomStage::FeltZ));
		E->DealerAt = World.TransformPosition(ABackRoomStage::SeatTransform(4).TransformPosition(FVector(-14.0, 0.0, 112.0)));
		E->HeroEyes = World.TransformPosition(ABackRoomStage::SeatTransform((S.Physical + 3) % 7).TransformPosition(FVector(-14.0, 0.0, 112.0)));
		E->OthersAt.Reset();
		for (int32 J : {0, 2, 5})
		{
			if (J != S.Physical)
			{
				E->OthersAt.Add(World.TransformPosition(ABackRoomStage::SeatTransform(J).TransformPosition(FVector(-14.0, 0.0, 112.0))));
			}
		}
	}
	for (int32 I = 0; I < Extras.Num(); ++I)
	{
		if (Extras[I] && !Taken[I])
		{
			Extras[I]->SetActorHiddenInGame(true);
		}
	}
	// Railbirds: a few at the bar all night, and a crowd at the stage's edge once the money's close.
	for (int32 I = 0; I < 5; ++I)
	{
		const FVector At(-1250.0 + I * 170.0 + (I % 2) * 30.0, 1680.0 - 330.0 - (I % 2) * 40.0, 0.0);
		Stage->AddCrowd(7 + I % 2, FTransform(FRotator(0.0f, -90.0f + (I - 2) * 14.0f, 0.0f), At));
	}
	const bool bMoneyClose = Tourney->Remaining <= Tourney->PaidPlaces() + 8;
	const int32 Watching = bFinalTable ? 12 : bMoneyClose ? 6 : 0;
	for (int32 I = 0; I < Watching; ++I)
	{
		const double Y = -420.0 + (I % 6) * 168.0 + (I / 6) * 70.0;
		const FVector At(1150.0 - (I / 6) * 70.0, Y, 0.0);
		Stage->AddCrowd(7 + (I * 7) % 2, FTransform(FRotator(0.0f, (FVector(1530.0, 0.0, 0.0) - At).Rotation().Yaw, 0.0f), At));
	}
	Table->SetCrowd(static_cast<float>(Running.Num() + 1) / static_cast<float>(FMath::Max(1, (Tourney->Spec.Entrants + Tourney->TableSize - 1) / Tourney->TableSize)));
}

FString ABackRoomGameMode::LiveBodyFor(const FString& Id, const FString& Name, const TSet<FString>& Worn)
{
	if (const FString* Kept = LiveBodies.Find(Id))
	{
		return *Kept;
	}
	FString Body = DefaultBody(Id, Name);
	if (CastAssetFor(Name).IsEmpty() && Worn.Contains(Body))
	{
		// Someone at this table already wears it: the next of the kind that's free.
		const uint32 Hash = FCrc::StrCrc32(*Id);
		const ss::live::RoomLocal* Local = ss::live::FindRoomLocal(std::string(TCHAR_TO_UTF8(*Name)));
		const TCHAR* const* Bodies = Local ? (Local->Woman ? WomenBodies : MenBodies) : ExtraBodies;
		const uint32 NumBodies = Local ? 6 : 12;
		for (uint32 K = 1; K < NumBodies; ++K)
		{
			if (!Worn.Contains(Bodies[(Hash + K) % NumBodies]))
			{
				Body = Bodies[(Hash + K) % NumBodies];
				break;
			}
		}
	}
	LiveBodies.Add(Id, Body);
	UE_LOG(LogEmbercrest, Log, TEXT("Look: %s (%s) wears %s, far crowd figure %d"), *Name, *Id, *Body, CrowdKindFor(Id, Name));
	return Body;
}

int32 ABackRoomGameMode::CrowdKindFor(const FString& Id, const FString& Name) const
{
	const FString* Kept = LiveBodies.Find(Id);
	const FString Body = Kept ? *Kept : DefaultBody(Id, Name);
	for (int32 Kind = 0; Kind < ABackRoomStage::CrowdKinds; ++Kind)
	{
		if (CrowdBodies[Kind] && Body == CrowdBodies[Kind])
		{
			return Kind;
		}
	}
	// A body nobody sat for (the cast's own): one of the twelve seated figures (0-5, 9-14), the same for them every time.
	const int32 Kind = static_cast<int32>(FCrc::StrCrc32(*Id) % 12);
	return Kind < 6 ? Kind : Kind + 3;
}

int32 ABackRoomGameMode::AnchorTableId()
{
	if (!Stage || !Tourney)
	{
		return -1;
	}
	for (const auto& It : Tourney->Tables)
	{
		if (SlotForTable(It.first) == Stage->GetRoomAnchor())
		{
			return It.first;
		}
	}
	return -1;
}

void ABackRoomGameMode::UpdateRoomBoards()
{
	if (!Stage || !Tourney || !Save.IsValid())
	{
		return;
	}
	// Today's events by the entrance, each with where it stands now.
	TArray<FString> Lines;
	for (const ss::live::Occurrence& O : ss::live::Occurrences(LiveDay))
	{
		FString State;
		if (Str(O.Id) == LiveEntryId && bLiveOver)
		{
			State = TEXT("YOUR NIGHT IS OVER");
		}
		else if (Str(O.Id) == LiveEntryId)
		{
			State = bLastCalled && !bRegClosed ? TEXT("LAST CALL  \u00b7  REG TO ") + Str(ss::net::TimeLabel(LiveLateRegEnds))
											   : FString::Printf(TEXT("PLAYING  \u00b7  %d OF %d LEFT"), Tourney->Remaining, Tourney->Spec.Entrants);
		}
		else if (Str(O.Id) == NextEntryId)
		{
			// The event the desk wrote the player into tonight.
			State = bNextCalled ? FString(TEXT("YOU'RE IN  \u00b7  SEATING NOW")) : TEXT("YOU'RE IN  \u00b7  SEATS ") + Str(ss::net::TimeLabel(NextSeatingAt));
		}
		else if (Minutes < O.DeskOpens)
		{
			State = TEXT("REGISTRATION ") + Str(ss::net::TimeLabel(O.DeskOpens));
		}
		else if (Minutes < O.Start)
		{
			State = TEXT("REGISTERING NOW");
		}
		else if (Minutes < O.LateRegEnds)
		{
			State = (ss::live::LastCall(O, Minutes) ? TEXT("LAST CALL  \u00b7  REG TO ") : TEXT("LATE REG TO ")) + Str(ss::net::TimeLabel(O.LateRegEnds));
		}
		else if (Minutes < O.Start + O.T->Hours * 60.0)
		{
			State = TEXT("RUNNING");
		}
		else
		{
			State = TEXT("FINISHED");
		}
		Lines.Add(FString::Printf(TEXT("%-9s %-24s %s"), *Str(ss::net::TimeLabel(O.Start)), *Str(O.T->Name).ToUpper(), *State));
	}
	Stage->SetSchedule(Lines);
}

// ------------------------------------------------------------------ the night

void ABackRoomGameMode::TestFastForward(int32 Hands)
{
	// -1: the floor moves you to another table (at the next gap between hands).
	if (Hands < 0)
	{
		bTestMove = true;
		return;
	}
	FastForwardLeft = Hands;
}

void ABackRoomGameMode::TestHeroChips(int32 Chips)
{
	TestChips += Chips;
}

double ABackRoomGameMode::LevelTimeLeft() const
{
	if (!Tourney)
	{
		return 0.0;
	}
	// The clock runs between hands too: a hand takes about forty-five seconds to play at this table.
	const double Hand = Tourney->Spec.SecondsPerHand;
	const double Since = Table ? FMath::Min(static_cast<double>(Table->HandAge()) * Hand / 45.0, Hand - 1.0) : 0.0;
	return FMath::Max(0.0, Tourney->LevelSecondsLeft() - Since);
}

void ABackRoomGameMode::RaiseBanner(const FString& Text)
{
	Banner = Text;
	BannerAge = 0.0f;
}

void ABackRoomGameMode::Floor(const FString& Line, bool bChime)
{
	if (bChime && Table && Table->GetAudio())
	{
		Table->GetAudio()->PlayEffect(ss::audio::Effect::Chime, 0.55f);
	}
	if (Table)
	{
		// After the chime.
		TWeakObjectPtr<ABackRoomTable> T = Table.Get();
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [T, Line]() {
			if (ABackRoomTable* Tb = T.Get())
			{
				Tb->Announce(TEXT("Floor"), Line);
			}
		}), bChime ? 1.1f : 0.01f, false);
	}
}

void ABackRoomGameMode::LiveEvent(const ss::TEvent& E)
{
	const int32 Paid = Tourney->PaidPlaces();
	const int32 HeroTable = Tourney->Hero().Busted ? -1 : Tourney->Hero().TableId;
	switch (E.Type)
	{
	case ss::TEventType::Bust:
	{
		const FString Name = Str(E.Name);
		if (E.IsHero)
		{
			HeroPlace = E.Place;
			HeroPrizeCents = E.PrizeCents;
			break;
		}
		if (ABackRoomPlayer* A = Table->PlayerById(Str(E.Id)))
		{
			A->Say(Goodbye(Name));
			if (E.PrizeCents > 0 && FMath::FRand() < 0.7f)
			{
				Table->DealerLine(FString::Printf(TEXT("%s place. See the cage for %s. Good game."), *Ordinal(E.Place), *Dollars(E.PrizeCents)));
			}
			break;
		}
		// Busts elsewhere: called out near the money and in it; now and then before.
		const int32 Idx = Tourney->PlayerIndex(E.Id);
		const int32 Seat = Idx >= 0 ? Tourney->Players[static_cast<size_t>(Idx)].Seat : 0;
		if (E.Place <= Paid + 4 || FMath::FRand() < 0.18f)
		{
			Floor(E.PrizeCents > 0 ? FString::Printf(TEXT("Table %d, seat %d is out in %s place, collecting %s."), E.TableId, ss::live::SeatLabel(Seat), *Ordinal(E.Place), *Dollars(E.PrizeCents))
								   : FString::Printf(TEXT("Table %d, seat %d is out in %s place."), E.TableId, ss::live::SeatLabel(Seat), *Ordinal(E.Place)),
				E.Place <= Paid + 4);
		}
		break;
	}
	case ss::TEventType::Level:
	{
		const ss::Level& L = E.Blinds;
		Floor(L.Ante > 0 ? FString::Printf(TEXT("Players, the blinds are now %s and %s, with a big blind ante of %s."), *Num(L.Sb), *Num(L.Bb), *Num(L.Ante))
						 : FString::Printf(TEXT("Players, the blinds are now %s and %s."), *Num(L.Sb), *Num(L.Bb)));
		RaiseBanner(FString::Printf(TEXT("LEVEL %d   %s / %s"), E.LevelNumber, *Num(L.Sb), *Num(L.Bb)));
		if (FMath::FRand() < 0.5f)
		{
			Table->DealerLine(TEXT("Blinds are up."));
		}
		break;
	}
	case ss::TEventType::TableBroken:
		if (E.TableId != HeroTable && (Tourney->Tables.size() <= 4 || FMath::FRand() < 0.5f))
		{
			Floor(FString::Printf(TEXT("Table %d is breaking. Players, take your chips and your seat cards from the floor."), E.TableId));
		}
		PlaceExtras();
		break;
	case ss::TEventType::Break:
	{
		bOnBreak = true;
		BreakT = 0.0f;
		BreakLevel = E.LevelNumber;
		const int32 Mins = FMath::RoundToInt(LiveBreakMinutes);
		Floor(FString::Printf(TEXT("Players, we are on a %d-minute break. Please be back in your seats when the clock says so."), Mins));
		RaiseBanner(FString::Printf(TEXT("BREAK   %d MINUTES"), Mins));
		Table->DealerLine(TEXT("Break, folks. Stretch your legs."));
		Table->Hold();
		break;
	}
	case ss::TEventType::HandForHand:
		bHandForHand = true;
		Floor(TEXT("Players, we are on the bubble. We're now hand for hand. Dealers, please hold your decks until every table has finished the hand."));
		RaiseBanner(TEXT("HAND FOR HAND"));
		Table->DealerLine(TEXT("Hand for hand, folks. Take your time."));
		break;
	case ss::TEventType::Bubble:
		bHandForHand = false;
		Floor(FString::Printf(TEXT("And that's the bubble! Congratulations, everyone remaining is in the money.")));
		RaiseBanner(TEXT("IN THE MONEY"));
		if (Table->GetAudio())
		{
			Table->GetAudio()->PlayEffect(ss::audio::Effect::Applause, 0.8f);
		}
		break;
	case ss::TEventType::FinalTable:
		bFinalTable = true;
		if (!Tourney->Hero().Busted && Stage && Stage->GetRoomAnchor() != ABackRoomStage::StreamSlot() && Phase == EBackRoomPhase::Playing)
		{
			// Up onto the stage with your chips: the final table plays at the stream table.
			Table->Hold();
			LiveMove();
		}
		Floor(TEXT("Ladies and gentlemen, we are down to our final table! Players, please bring your chips to the feature table."));
		RaiseBanner(TEXT("FINAL TABLE"));
		Stage->SetOnAir(true);
		if (Table->GetAudio())
		{
			Table->GetAudio()->PlayEffect(ss::audio::Effect::Applause, 0.9f);
		}
		break;
	case ss::TEventType::Finished:
		if (E.Id != ss::HeroId && bRailing)
		{
			Floor(FString::Printf(TEXT("Ladies and gentlemen, your Embercrest %s champion... %s!"), *LiveShort, *Str(E.Name)));
			RaiseBanner(FString::Printf(TEXT("CHAMPION   %s"), *Str(E.Name).ToUpper()));
			if (Table->GetAudio())
			{
				Table->GetAudio()->PlayEffect(ss::audio::Effect::Applause, 0.9f);
			}
		}
		if (E.Id == ss::HeroId)
		{
			bHeroWon = true;
			HeroPlace = 1;
			HeroPrizeCents = Tourney->PrizeFor(1);
			// The floor calls the name on the player's registration: the person's (the screen name is RiverLine's).
			const FString Winner = Save.IsValid() && Save->Person.Created ? Str(Save->Person.FullName()) : Str(E.Name);
			Floor(FString::Printf(TEXT("Ladies and gentlemen, your Embercrest %s champion... %s!"), *LiveShort, *Winner));
			RaiseBanner(TEXT("CHAMPION"));
			if (Table->GetAudio())
			{
				Table->GetAudio()->PlayEffect(ss::audio::Effect::Applause, 1.0f);
			}
		}
		break;
	default:
		break;
	}
	if (E.Type == ss::TEventType::Moved && !E.IsHero)
	{
		PlaceExtras();
	}
}

void ABackRoomGameMode::LiveNote(uint8 Note)
{
	switch (static_cast<EBackRoomTableNote>(Note))
	{
	case EBackRoomTableNote::HandEnded:
	{
		++HandsPlayed;
		// A big pot shown down with one of the world's people (forty big blinds or more): they'll remember it, the biggest
		// one with each of them tonight (R7: only what was shown).
		const FBackRoomShowdown& Shown = Table->LastShowdown();
		if (Shown.Id.StartsWith(TEXT("npc:")) && Shown.Pot >= 40 * FMath::Max<int64>(1, Shown.BigBlind))
		{
			int64& Net = LiveBigPots.FindOrAdd(Shown.Id);
			Net = FMath::Abs(Shown.HeroNet) > FMath::Abs(Net) ? Shown.HeroNet : Net;
		}
		// Saved once the room has heard this hand's news (a break, a move): LiveTick, next.
		bCheckpointDue = true;
		if (bHandForHand && !Tourney->InTheMoney())
		{
			// The other tables are still playing their hand: everyone waits for the floor.
			Table->Hold();
			MoveT = -3.5f;
		}
		break;
	}
	case EBackRoomTableNote::HeroMoved:
		if (Phase == EBackRoomPhase::Arriving)
		{
			Table->PrepareNext();
		}
		else
		{
			LiveMove();
		}
		break;
	case EBackRoomTableNote::TournamentOver:
		LiveOver();
		break;
	case EBackRoomTableNote::HandDealt:
		LiveCheckpoint(true);
		break;
	default:
		break;
	}
}

void ABackRoomGameMode::LiveCheckpoint(bool bDealt)
{
	if (!Save.IsValid() || !LiveSaver || !Tourney || bLiveOver || bCancelled)
	{
		return;
	}
	if (!bDealt)
	{
		bCheckpointDue = false;
		const std::string Ck = Tourney->Checkpoint();
		if (Ck.empty())
		{
			return;
		}
		LastCheckpoint = Ck;
		LastCheckpointAt = FMath::Max(Minutes, LiveDayStart + Tourney->ClockMinutes());
	}
	if (LastCheckpoint.empty())
	{
		return;
	}
	ss::SaveData D = *Save;
	ss::life::LiveEntry* E = ss::live::EntryFor(D.Life, std::string(TCHAR_TO_UTF8(*LiveEntryId)));
	if (!E || E->State != ss::life::LiveEntry::Registered)
	{
		return;
	}
	E->Checkpoint = LastCheckpoint;
	E->CheckpointAt = LastCheckpointAt;
	E->CheckpointHost = std::string(TCHAR_TO_UTF8(*LiveHostText(bDealt)));
	D.ClockMinutes = LastCheckpointAt - ss::net::MinutesPerDay * ss::net::NightOneDay;
	D.Life.Energy = FMath::Clamp(static_cast<double>(Energy), 0.0, 100.0);
	LiveSaver->Submit(D);
	++Checkpoints;
	UE_LOG(LogEmbercrest, Log, TEXT("Checkpoint %d%s: hand %d, level %d, %d left, stack %lld"), Checkpoints, bDealt ? TEXT(" (dealt)") : TEXT(""), Tourney->Tick, Tourney->LevelIndex + 1,
		Tourney->Remaining, static_cast<int64>(Tourney->Hero().Stack));
}

FString ABackRoomGameMode::LiveHostText(bool bDealt) const
{
	// "key\tvalue" lines: what the room knows that the tournament doesn't.
	FString Out = FString::Printf(TEXT("hands\t%d\n"), HandsPlayed);
	if (bDealt)
	{
		Out += TEXT("dealt\t1\n");
	}
	if (bOnBreak)
	{
		Out += FString::Printf(TEXT("break\t%d\t%.2f\n"), BreakLevel, BreakT);
	}
	for (const FString& Id : LiveMet)
	{
		Out += TEXT("met\t") + Id + TEXT("\n");
	}
	// The night's own departure, for an event entered from the room (Night One catches its calendar up from it).
	if (NightLeftHomeAt < LeftHomeAt - 0.5)
	{
		Out += FString::Printf(TEXT("from\t%.2f\n"), NightLeftHomeAt);
	}
	// The big pots shown down, and the faces people have worn tonight (so a night come back to looks the same).
	for (const TPair<FString, int64>& P : LiveBigPots)
	{
		Out += FString::Printf(TEXT("pot\t%s\t%lld\n"), *P.Key, P.Value);
	}
	for (const TPair<FString, FString>& L : LiveBodies)
	{
		Out += FString::Printf(TEXT("look\t%s\t%s\n"), *L.Key, *L.Value);
	}
	if (Table && Save.IsValid())
	{
		// The reads that changed tonight (the rest are in the save already).
		for (const TPair<FString, int32>& R : Table->Reads)
		{
			const auto Was = Save->Life.Reads.find(std::string(TCHAR_TO_UTF8(*R.Key)));
			if (Was == Save->Life.Reads.end() || Was->second != R.Value)
			{
				Out += FString::Printf(TEXT("read\t%s\t%d\n"), *R.Key, R.Value);
			}
		}
	}
	return Out;
}

void ABackRoomGameMode::ReadLiveHost(const FString& Text)
{
	TArray<FString> Lines;
	Text.ParseIntoArray(Lines, TEXT("\n"), true);
	for (const FString& Line : Lines)
	{
		TArray<FString> F;
		Line.ParseIntoArray(F, TEXT("\t"), false);
		if (F.Num() == 2 && F[0] == TEXT("hands"))
		{
			HandsPlayed = FCString::Atoi(*F[1]);
		}
		else if (F.Num() == 2 && F[0] == TEXT("dealt"))
		{
			bResumeDealt = F[1] == TEXT("1");
		}
		else if (F.Num() == 3 && F[0] == TEXT("break"))
		{
			bOnBreak = true;
			BreakLevel = FCString::Atoi(*F[1]);
			BreakT = FMath::Clamp(FCString::Atof(*F[2]), 0.0f, BreakRealSeconds - 3.0f);
		}
		else if (F.Num() == 2 && F[0] == TEXT("met"))
		{
			LiveMet.Add(F[1]);
		}
		else if (F.Num() == 3 && F[0] == TEXT("read"))
		{
			ResumeReads.Add(F[1], FCString::Atoi(*F[2]));
		}
		else if (F.Num() == 2 && F[0] == TEXT("from"))
		{
			NightLeftHomeAt = FMath::Min(NightLeftHomeAt, FCString::Atod(*F[1]));
		}
		else if (F.Num() == 3 && F[0] == TEXT("pot"))
		{
			LiveBigPots.Add(F[1], FCString::Atoi64(*F[2]));
		}
		else if (F.Num() == 3 && F[0] == TEXT("look"))
		{
			LiveBodies.Add(F[1], F[2]);
		}
	}
}

void ABackRoomGameMode::LiveMove()
{
	ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	const ss::TPlayer& H = Tourney->Hero();
	if (bHeroUp && Pawn && Pawn->IsFreeWalking() && Stage)
	{
		// Away from the table when it broke: the floor carries the chips over, the player finds the new seat.
		const FVector Before = Stage->RoomToWorld(FVector::ZeroVector);
		Stage->SetRoomAnchor(SlotForTable(H.TableId));
		Pawn->ShiftWalk(Stage->RoomToWorld(FVector::ZeroVector) - Before);
		Table->PrepareNext();
		PlaceExtras();
		Table->Resume(1.0f);
		Floor(FString::Printf(TEXT("Player from table %d, you've been moved: table %d, seat %d. Your chips are there."), Stage->GetRoomAnchor() + 1, H.TableId, ss::live::SeatLabel(H.Seat)), false);
		RaiseBanner(FString::Printf(TEXT("TABLE %d   SEAT %d"), H.TableId, ss::live::SeatLabel(H.Seat)));
		return;
	}
	Phase = EBackRoomPhase::Moving;
	MoveT = 0.0f;
	const bool bDee = ((LiveDay % 7) + 7) % 7 == 6;
	Table->DealerLine(FString::Printf(TEXT("You're moving%s. Table %d, seat %d. Grab your chips."), bDee ? TEXT(", kid") : TEXT(""), H.TableId, ss::live::SeatLabel(H.Seat)));
	RaiseBanner(FString::Printf(TEXT("TABLE %d   SEAT %d"), H.TableId, ss::live::SeatLabel(H.Seat)));
	if (!Pawn)
	{
		Table->PrepareNext();
		Phase = EBackRoomPhase::Playing;
		Table->Resume(1.0f);
		return;
	}
	// Up from the seat and along the aisle toward the new table...
	const FVector Eye = Pawn->GetEye();
	const int32 FromSlot = Stage ? Stage->GetRoomAnchor() : 0;
	const int32 ToSlot = SlotForTable(H.TableId);
	const TArray<FVector> Out = Stage ? Stage->CardRoomMoveOut(Eye, ToSlot) : TArray<FVector>{Eye, Eye + FVector(-34.0, -16.0, 44.0), FVector(-262.0, -60.0, 166.0)};
	TWeakObjectPtr<ABackRoomGameMode> Self = this;
	Pawn->PlayWalk(Out, 4.0f, true, [Self, FromSlot, ToSlot]() {
		ABackRoomGameMode* GM = Self.Get();
		if (!GM)
		{
			return;
		}
		// ...a blink, and the room has turned: a different corner of it, the new table's faces.
		APlayerController* PC = GM->GetWorld()->GetFirstPlayerController();
		if (PC && PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 0.35f, FLinearColor::Black, true, true);
		}
		FTimerHandle Handle;
		GM->GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(GM, [GM, FromSlot, ToSlot]() {
			GM->Stage->SetRoomAnchor(ToSlot);
			GM->Table->PrepareNext();
			GM->PlaceExtras();
			APlayerController* PC2 = GM->GetWorld()->GetFirstPlayerController();
			if (PC2 && PC2->PlayerCameraManager)
			{
				PC2->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 0.6f, FLinearColor::Black, true, false);
			}
			ABackRoomPawn* P = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(GM, 0));
			const FVector SeatEye = GM->Stage ? GM->Stage->EyeLocation() : FVector(-91.0, 0.0, 118.0);
			const TArray<FVector> In = GM->Stage->CardRoomMoveIn(SeatEye, FromSlot);
			TWeakObjectPtr<ABackRoomGameMode> Self2 = GM;
			if (P)
			{
				P->PlayWalk(In, 3.6f, false, [Self2]() {
					if (ABackRoomGameMode* G = Self2.Get())
					{
						G->Phase = EBackRoomPhase::Playing;
						G->Table->Resume(1.2f);
						G->Table->DealerLine(TEXT("Welcome to the table."));
					}
				});
			}
		}), 0.45f, false);
	});
}

void ABackRoomGameMode::LiveRequestLeave()
{
	if (Phase != EBackRoomPhase::Playing || bLiveOver)
	{
		return;
	}
	if (QuitAskedAt < 0.0f)
	{
		QuitAskedAt = LiveT;
		Table->DealerLine(((LiveDay % 7) + 7) % 7 == 6 ? TEXT("You sure, kid? Walk now and your stack gets blinded off.") : TEXT("You sure? Walk now and your stack gets blinded off."));
		return;
	}
	// The room's last round is still being finished (a big field's tables play over several frames): leave once it is.
	if (Tourney->FinishPending() || !Table->IsBetweenHands())
	{
		bLeaveWhenFree = true;
		return;
	}
	// Gone: the stack sits there and the blinds take it, hand by hand, until it's out (or it limps into
	// the money on its own).
	bLeaveWhenFree = false;
	QuitAskedAt = -1.0f;
	Tourney->HeroSitsOut = true;
	for (int32 Guard = 0; Guard < 4000 && !Tourney->Hero().Busted && !Tourney->bFinished; ++Guard)
	{
		for (const ss::TEvent& E : Tourney->SimulateTick())
		{
			if (E.Type == ss::TEventType::Bust && E.IsHero)
			{
				HeroPlace = E.Place;
				HeroPrizeCents = E.PrizeCents;
			}
		}
	}
	if (!Tourney->Hero().Busted)
	{
		HeroPlace = Tourney->Hero().Place > 0 ? Tourney->Hero().Place : 1;
		HeroPrizeCents = Tourney->Hero().PrizeCents;
	}
	Minutes = LiveDayStart + Tourney->ClockMinutes();
	LiveOver();
}

void ABackRoomGameMode::LiveOver()
{
	if (bLiveOver)
	{
		return;
	}
	bLiveOver = true;
	BustMinutes = Minutes;
	if (bHeroUp)
	{
		// Out while on foot (blinded off, or heading for the door): already up, so already on the rail, and the old table
		// catches up with the engine at once (a blind-off plays many hands in one go).
		bRailing = true;
		bRailSeatsDue = true;
		RailT = RailSettleSeconds;
	}
	UE_LOG(LogEmbercrest, Log, TEXT("Over: phase %d, break %d, hero busted %d, finished %d, moving %d"), static_cast<int32>(Phase), bOnBreak ? 1 : 0, Tourney->Hero().Busted ? 1 : 0, Tourney->bFinished ? 1 : 0, MoveT >= 0.0f ? 1 : 0);
	const int32 Field = Tourney->Spec.Entrants;
	if (bCancelled)
	{
		// Nothing was played: the entry comes back, and the bus home.
		HeroPlace = 0;
		HeroPrizeCents = 0;
		NetCents = 0;
		bBustedOut = true;
		LiveSettle();
		Phase = EBackRoomPhase::Leaving;
		LeaveT = 0.0f;
		Summary.Reset();
		Summary.Add(LiveName.ToUpper());
		Summary.Add(FString::Printf(TEXT("Cancelled   \u00b7   %d players"), Field));
		Summary.Add(FString::Printf(TEXT("Refunded   %s"), *Dollars(BoughtInCents)));
		Table->DealerLine(TEXT("Not enough of us tonight. The desk has your money."));
		return;
	}
	if (HeroPlace <= 0)
	{
		HeroPlace = Tourney->Hero().Place > 0 ? Tourney->Hero().Place : Tourney->HeroRank();
		HeroPrizeCents = Tourney->PrizeFor(HeroPlace);
	}
	NetCents = HeroPrizeCents - BoughtInCents;
	bBustedOut = HeroPrizeCents <= 0;
	// Which reads became yours tonight.
	TArray<FString> Learned;
	for (const TPair<FString, int32>& R : Table->Reads)
	{
		const auto Was = Save->Life.Reads.find(std::string(TCHAR_TO_UTF8(*R.Key)));
		const int32 Before = Was == Save->Life.Reads.end() ? 0 : Was->second;
		if (R.Value >= 2 && Before < 2)
		{
			FString Who, TellName;
			R.Key.Split(TEXT("/"), &Who, &TellName);
			const int64 Value = StaticEnum<EBackRoomTell>()->GetValueByNameString(TellName);
			Learned.Add(FString::Printf(TEXT("Read learned   %s: %s"), *Who, *ABackRoomTable::TellPhrase(static_cast<uint8>(FMath::Max<int64>(Value, 0)))));
		}
	}
	LiveSettle();
	Phase = EBackRoomPhase::Leaving;
	LeaveT = 0.0f;
	Summary.Reset();
	Summary.Add(bHeroWon ? LiveName.ToUpper() + TEXT("   CHAMPION") : LiveName.ToUpper());
	const int32 Sat = FMath::Max(0, FMath::FloorToInt(Minutes - LeftHomeAt - 20.0));
	Summary.Add(FString::Printf(TEXT("%s of %d   ·   %d hands   ·   %dh %02dm"), *Ordinal(HeroPlace), Field, HandsPlayed, Sat / 60, Sat % 60));
	Summary.Add(HeroPrizeCents > 0 ? FString::Printf(TEXT("Cashed   %s"), *Dollars(HeroPrizeCents)) : FString::Printf(TEXT("No cash   ·   %d paid"), Tourney->PaidPlaces()));
	Summary.Add(FString::Printf(TEXT("Net   %s%s"), NetCents >= 0 ? TEXT("+") : TEXT("-"), *Dollars(FMath::Abs(NetCents))));
	Summary.Append(Learned);
	// The rest of today at the desk, if there's an event the player could still get into.
	FString WhyNot;
	const ss::live::Occurrence Next = ss::live::FindOccurrence(std::string(TCHAR_TO_UTF8(*NextOpenEvent(WhyNot))));
	if (Next.Valid())
	{
		const FString When = Minutes < Next.Start ? TEXT("cards at ") + Str(ss::net::TimeLabel(Next.Start)) : TEXT("late reg to ") + Str(ss::net::TimeLabel(Next.LateRegEnds));
		Summary.Add(FString::Printf(TEXT("Still open at the desk   %s, %s"), *Str(Next.T->Name), *When));
	}
	// The dealer, from the box (Dee on a Sunday; the day's dealer otherwise).
	const bool bDee = ((LiveDay % 7) + 7) % 7 == 6;
	FString Line;
	if (bHeroWon)
	{
		Line = bDee ? TEXT("Kid. You won the whole thing. Go get your money before I cry.") : TEXT("You won the whole thing. Congratulations. The cage is waiting on you.");
	}
	else if (HeroPrizeCents > 0)
	{
		Line = FString::Printf(TEXT("%s. That's a cash%s. The cage is behind you."), *Ordinal(HeroPlace), bDee ? TEXT(", kid") : TEXT(""));
	}
	else if (HeroPlace <= Tourney->PaidPlaces() + 3)
	{
		Line = TEXT("Right before the money. That one stings. Go home, sleep it off.");
	}
	else
	{
		Line = bDee ? FString::Printf(TEXT("%s of %d. Happens. See you Tuesday."), *Ordinal(HeroPlace), Field)
					: FString::Printf(TEXT("%s of %d. Happens. Come see us again."), *Ordinal(HeroPlace), Field);
	}
	Table->DealerLine(Line);
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	UE_LOG(LogEmbercrest, Log, TEXT("Embercrest over: %s of %d, prize %lld cents, net %lld, %d hands"), *Ordinal(HeroPlace), Field, HeroPrizeCents, NetCents, HandsPlayed);
}

// ------------------------------------------------------------------ on foot

void ABackRoomGameMode::LiveStandUp()
{
	ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!bLive || !Stage || !Pawn || bHeroUp || bLiveOver || Phase != EBackRoomPhase::Playing || Pawn->IsWalking() || Pawn->IsFreeWalking())
	{
		return;
	}
	bHeroUp = true;
	Table->bHeroAway = true;
	if (!bAwayExplained && !bOnBreak)
	{
		bAwayExplained = true;
		Table->DealerLine(TEXT("You're still dealt in while you're up. I check it when it's free and muck it when it isn't."));
	}
	TWeakObjectPtr<ABackRoomPawn> Walker = Pawn;
	Pawn->PlayWalk(Stage->CardRoomMoveOut(Pawn->GetEye(), Stage->GetRoomAnchor()), 1.8f, true, [Walker]() {
		if (ABackRoomPawn* W = Walker.Get())
		{
			W->BeginFreeWalk();
		}
	});
	UE_LOG(LogEmbercrest, Log, TEXT("Up from the table (hand %d, %s)"), Tourney->Tick, bOnBreak ? TEXT("on a break") : TEXT("dealt in while away"));
}

void ABackRoomGameMode::LiveSitDown()
{
	ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Pawn || !Stage || !bHeroUp || bLiveOver)
	{
		return;
	}
	Pawn->EndFreeWalk();
	TArray<FVector> In = Stage->CardRoomMoveIn(Stage->EyeLocation(), Stage->GetRoomAnchor());
	In.Insert(Pawn->GetEye(), 0);
	TWeakObjectPtr<ABackRoomGameMode> Self = this;
	Pawn->PlayWalk(In, 2.8f, false, [Self]() {
		if (ABackRoomGameMode* GM = Self.Get())
		{
			GM->bHeroUp = false;
			GM->Table->bHeroAway = false;
			if (!GM->bOnBreak && FMath::FRand() < 0.6f)
			{
				GM->Table->DealerLine(TEXT("Welcome back."));
			}
		}
	});
	UE_LOG(LogEmbercrest, Log, TEXT("Back in the chair (hand %d)"), Tourney->Tick);
}

void ABackRoomGameMode::LiveStay()
{
	ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!bLive || !Stage || !Pawn || Phase != EBackRoomPhase::Leaving || bRailing || bWalkingOut || bExitNow || Pawn->IsWalking() || bCancelled)
	{
		return;
	}
	bRailing = true;
	RailT = 0.0f;
	if (bHeroUp)
	{
		// Already on foot (out while walking about): just stay.
		if (!Pawn->IsFreeWalking())
		{
			Pawn->BeginFreeWalk();
		}
	}
	else
	{
		bHeroUp = true;
		TWeakObjectPtr<ABackRoomPawn> Walker = Pawn;
		Pawn->PlayWalk(Stage->CardRoomMoveOut(Pawn->GetEye(), Stage->GetRoomAnchor()), 1.8f, true, [Walker]() {
			if (ABackRoomPawn* W = Walker.Get())
			{
				W->BeginFreeWalk();
			}
		});
	}
	if (HeroPrizeCents > 0)
	{
		Table->DealerLine(TEXT("Stick around. The cage is open all night."));
	}
	// The old table as the engine has it now (whoever went out with the player's last hand gets up).
	const int32 Here = AnchorTableId();
	Table->FollowTable(Here, Here == Tourney->Hero().TableId ? Tourney->Hero().Seat : 0, true);
	PlaceExtras();
	UE_LOG(LogEmbercrest, Log, TEXT("Staying to watch: %d left"), Tourney->Remaining);
}

void ABackRoomGameMode::LiveLeaveNow()
{
	if (Phase != EBackRoomPhase::Leaving || bWalkingOut)
	{
		return;
	}
	if (bRailing)
	{
		bExitNow = true;
		return;
	}
	LeaveT = FMath::Max(LeaveT, 31.0f);
}

float ABackRoomGameMode::StayOffer() const
{
	return bLive && Phase == EBackRoomPhase::Leaving && !bRailing && !bWalkingOut && !bExitNow && !bCancelled && LeaveT < 30.0f ? LeaveT : -1.0f;
}

ECardRoomSpot ABackRoomGameMode::CurrentSpot() const
{
	const ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	return Pawn && Stage && Pawn->IsFreeWalking() ? Stage->CardRoomSpot(Pawn->GetEye()) : ECardRoomSpot::None;
}

FString ABackRoomGameMode::SpotPrompt() const
{
	switch (CurrentSpot())
	{
	case ECardRoomSpot::Seat: return bLiveOver ? FString() : FString(TEXT("[E] Sit back down"));
	case ECardRoomSpot::Desk:
	{
		// Out of tonight's event: the desk can write the player into the next one.
		FString WhyNot;
		const ss::live::Occurrence Open = bLiveOver && NextEntryId.IsEmpty() && !Signup.IsValid() ? ss::live::FindOccurrence(std::string(TCHAR_TO_UTF8(*NextOpenEvent(WhyNot))))
																									 : ss::live::Occurrence();
		if (Open.Valid())
		{
			return DeskOffer == Str(Open.Id) && LiveT - DeskAskedAt < 8.0f
					   ? FString::Printf(TEXT("[E] again: register for the %s (%s)"), *Str(Open.T->Short), *Dollars(Open.T->BuyInCents))
					   : FString::Printf(TEXT("[E] Tournament desk: the %s is open (%s)"), *Str(Open.T->Short), *Dollars(Open.T->BuyInCents));
		}
		return TEXT("[E] Tournament desk");
	}
	case ECardRoomSpot::Cage: return bLiveOver && HeroPrizeCents > 0 && !bCollected ? FString(TEXT("[E] Collect your winnings")) : FString(TEXT("[E] Cashier"));
	case ECardRoomSpot::Bar: return TEXT("[E] The bar");
	case ECardRoomSpot::Terrace:
		return !NextEntryId.IsEmpty() && !bNextCalled && !bWaitingOut ? FString::Printf(TEXT("[E] Step out on the terrace until the %s is called"), *NextName)
																	  : FString(TEXT("[E] Step out on the terrace"));
	case ECardRoomSpot::Exit:
		if (bLiveOver && !NextEntryId.IsEmpty())
		{
			return LiveT - ExitAskedAt < 5.0f ? FString::Printf(TEXT("[E] again: off the list for the %s, refunded, and home"), *NextName)
											  : FString::Printf(TEXT("[E] Go home (the desk refunds the %s)"), *NextName);
		}
		return bLiveOver ? FString(TEXT("[E] Go home")) : (LiveT - ExitAskedAt < 5.0f ? FString(TEXT("[E] again: leave the tournament (your stack is blinded off)")) : FString(TEXT("[E] Leave the tournament")));
	default: return FString();
	}
}

void ABackRoomGameMode::LiveInteract()
{
	ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!bLive || !Stage || !Pawn || !Pawn->IsFreeWalking() || !Tourney)
	{
		return;
	}
	const ss::Level& L = Tourney->CurrentLevel();
	switch (Stage->CardRoomSpot(Pawn->GetEye()))
	{
	case ECardRoomSpot::Seat:
		LiveSitDown();
		break;
	case ECardRoomSpot::Desk:
	{
		// The floor at the desk: where the night is, and a regular gets a regular's hello. Out of tonight's event, the next
		// one today: offered on the first press, written in on the second.
		FString Line;
		FString WhyNot;
		const FString Open = bLiveOver && NextEntryId.IsEmpty() && !Signup.IsValid() ? NextOpenEvent(WhyNot) : FString();
		const ss::live::Occurrence Offer = ss::live::FindOccurrence(std::string(TCHAR_TO_UTF8(*Open)));
		if (Signup.IsValid())
		{
			Line = TEXT("One second, I'm writing you in.");
		}
		else if (!NextEntryId.IsEmpty())
		{
			Line = bNextCalled ? FString::Printf(TEXT("They're seating the %s now. Go on, find your table."), *NextName)
							   : FString::Printf(TEXT("You're all set for the %s. Seats open at %s; the floor will call it. The bar's open, the terrace too."), *NextName,
									 *Str(ss::net::TimeLabel(NextSeatingAt)));
		}
		else if (Offer.Valid() && DeskOffer == Open && LiveT - DeskAskedAt < 8.0f)
		{
			DeskOffer.Empty();
			DeskSignup(Open);
			Line = TEXT("Let me write you in.");
		}
		else if (Offer.Valid())
		{
			DeskOffer = Open;
			DeskAskedAt = LiveT;
			const ss::live::EventTemplate& T = *Offer.T;
			const FString When = Minutes < Offer.Start ? TEXT("cards at ") + Str(ss::net::TimeLabel(Offer.Start)) : TEXT("late registration till ") + Str(ss::net::TimeLabel(Offer.LateRegEnds));
			Line = FString::Printf(TEXT("The %s is still open%s. %s: %s to the prize pool, %s to the house. %s in chips, %s. Want in?"), *Str(T.Short), *AddressTo(Save),
				*Dollars(T.BuyInCents), *Dollars(T.BuyInCents - T.FeeCents), *Dollars(T.FeeCents), *Num(T.StartingStack), *When);
		}
		else if (!WhyNot.IsEmpty())
		{
			Line = WhyNot;
		}
		else if (bLiveOver)
		{
			FString Next;
			for (int32 Day = LiveDay; Day <= LiveDay + 1 && Next.IsEmpty(); ++Day)
			{
				for (const ss::live::Occurrence& O : ss::live::Occurrences(Day))
				{
					if (O.Start > Minutes && Next.IsEmpty())
					{
						Next = FString::Printf(TEXT("the %s, %s %s"), *Str(O.T->Name), Day == LiveDay ? TEXT("tonight at") : TEXT("tomorrow at"), *Str(ss::net::TimeLabel(O.Start)));
					}
				}
			}
			Line = Next.IsEmpty() ? FString(TEXT("Thanks for coming in. Get home safe.")) : FString::Printf(TEXT("Thanks for playing. Next one's %s."), *Next);
		}
		else
		{
			const int32 Every = FMath::Max(1, Tourney->Spec.BreakEvery);
			const int32 NextBreak = (Tourney->LevelIndex / Every + 1) * Every + 1;
			const FString Status = FString::Printf(TEXT("%d left, %d paid. Level %d, %s and %s; the next break's before level %d."), Tourney->Remaining, Tourney->PaidPlaces(),
				Tourney->LevelIndex + 1, *Num(L.Sb), *Num(L.Bb), NextBreak);
			Line = PastLiveEvents == 0 ? TEXT("First time with us? Welcome to the Embercrest. ") + Status
				   : PastLiveEvents >= 4 ? FString::Printf(TEXT("There's my regular%s. "), *AddressTo(Save)) + Status
										 : Status;
		}
		Table->Announce(TEXT("Desk"), Line);
		break;
	}
	case ECardRoomSpot::Cage:
		if (bLiveOver && HeroPrizeCents > 0 && !bCollected)
		{
			bCollected = true;
			Table->Announce(TEXT("Cashier"), FString::Printf(TEXT("Congratulations%s. %s, cash. Count it with me... there you go."), *AddressTo(Save), *Dollars(HeroPrizeCents)));
			if (Table->GetAudio())
			{
				Table->GetAudio()->Play(ss::SoundId::Cash, 0.8f);
			}
		}
		else if (bCollected)
		{
			Table->Announce(TEXT("Cashier"), TEXT("You're all paid up, hon. Don't spend it all at once."));
		}
		else if (PastLiveCashes > 0)
		{
			Table->Announce(TEXT("Cashier"), FString::Printf(TEXT("Back again%s. Your best here's %s place. Bring me something bigger tonight."), *AddressTo(Save),
				*Ordinal(FMath::Max(1, PastBestPlace))));
		}
		else
		{
			Table->Announce(TEXT("Cashier"), PastLiveEvents == 0 ? FString(TEXT("Nothing to cash yet. First night? Good luck in there."))
																 : FString::Printf(TEXT("Nothing for you yet%s. Come back with good news."), *AddressTo(Save)));
		}
		break;
	case ECardRoomSpot::Bar:
		if (LiveT - LastWater > 300.0f)
		{
			LastWater = LiveT;
			Energy = FMath::Min(100.0f, Energy + 3.0f);
			Table->Announce(TEXT("Bartender"), TEXT("Club soda, lime. On the house for players."));
		}
		else
		{
			Table->Announce(TEXT("Bartender"), TEXT("Another one? Pace yourself, the night's long."));
		}
		break;
	case ECardRoomSpot::Terrace:
		if (!NextEntryId.IsEmpty() && !bNextCalled && !bWaitingOut)
		{
			// Waiting for the next event out in the cold: the wait passes (the room plays on without you) until the floor
			// calls it.
			bWaitingOut = true;
			APlayerController* PC = GetWorld()->GetFirstPlayerController();
			if (PC && PC->PlayerCameraManager)
			{
				PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 0.9f, FLinearColor::Black, true, true);
			}
			FTimerHandle Handle;
			GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]() {
				const double Until = NextSeatingAt;
				RailCatchUp(Until);
				LastAir = LiveT;
				Energy = FMath::Min(100.0f, Energy + 6.0f);
				bWaitingOut = false;
				if (APlayerController* P = GetWorld()->GetFirstPlayerController(); P && P->PlayerCameraManager)
				{
					P->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 1.4f, FLinearColor::Black, true, false);
				}
				Table->Announce(FString(), FString::Printf(TEXT("Out on the terrace until %s: the ridge, the cold, the resort's lights on the slopes. Inside, the floor's on the mic."),
											   *Str(ss::net::TimeLabel(Until))));
			}), 1.0f, false);
		}
		else if (LiveT - LastAir > 420.0f)
		{
			LastAir = LiveT;
			Energy = FMath::Min(100.0f, Energy + 6.0f);
			Table->Announce(FString(), TEXT("Cold mountain air, the ridge white under the stars. You wake up a little."));
		}
		else
		{
			Table->Announce(FString(), TEXT("The ranges, dark and huge. The room's warm behind you."));
		}
		break;
	case ECardRoomSpot::Exit:
		if (bLiveOver && !NextEntryId.IsEmpty())
		{
			// Registered for the next event: leaving takes the player off its list (refunded: they never sat), then home.
			if (bOpeningNext)
			{
				break;
			}
			if (LiveT - ExitAskedAt < 5.0f)
			{
				LiveUnregister();
				bExitNow = true;
			}
			else
			{
				ExitAskedAt = LiveT;
				Table->Announce(TEXT("Floor"), FString::Printf(TEXT("You're on the list for the %s. Head out now and the desk takes you off it, money back. Press again to go."), *NextName));
			}
		}
		else if (bLiveOver)
		{
			bExitNow = true;
		}
		else if (LiveT - ExitAskedAt < 5.0f)
		{
			// Walking out on a live tournament: the seat's dealt in until the blinds take it (the published rule).
			bExitNow = true;
			QuitAskedAt = LiveT;
			LiveRequestLeave();
		}
		else
		{
			ExitAskedAt = LiveT;
			Table->Announce(TEXT("Floor"), TEXT("Heading out? Your seat stays in and gets blinded off. Press again to go."));
		}
		break;
	default:
		break;
	}
}

void ABackRoomGameMode::LiveRail(float RealDt)
{
	// The room plays on while the player watches: a round every few seconds, the floor on the PA, the boards, and the
	// player's old table playing it out in front of them. Its chairs settle a moment after each round (a bust says
	// goodbye before getting up).
	if (!Tourney || Tourney->FinishPending())
	{
		return;
	}
	RailT += RealDt;
	if (bRailSeatsDue && RailT >= RailSettleSeconds)
	{
		bRailSeatsDue = false;
		const int32 Here = AnchorTableId();
		Table->FollowTable(Here, Here == Tourney->Hero().TableId ? Tourney->Hero().Seat : 0, true);
		PlaceExtras();
	}
	if (Tourney->bFinished)
	{
		// Over: the room empties out and the clock goes on at the rail's pace (someone waiting on the next event).
		Minutes += RealDt * Tourney->Spec.SecondsPerHand / RailRoundSeconds / 60.0;
		return;
	}
	if (bOnBreak || RailT < RailRoundSeconds)
	{
		return; // between rounds, or the room's on its break
	}
	RailT = 0.0f;
	for (const ss::TEvent& E : Tourney->SimulateTick())
	{
		LiveEvent(E);
	}
	const int32 Here = AnchorTableId();
	Table->FollowTable(Here, Here == Tourney->Hero().TableId ? Tourney->Hero().Seat : 0, false);
	bRailSeatsDue = true;
	PlaceExtras();
	Minutes = FMath::Max(Minutes, LiveDayStart + Tourney->ClockMinutes());
}

void ABackRoomGameMode::RailCatchUp(double Until)
{
	if (!Tourney || Tourney->FinishPending())
	{
		return;
	}
	// Every round played by the engine, as the rail would have seen them. The room's state keeps up with all of it; only
	// the latest milestone is news when the player is back in the room (the bubble, the final table, a champion): the PA
	// doesn't replay the hours that passed.
	ss::TEvent News;
	bool bNews = false;
	for (int32 Guard = 0; Guard < 4000 && !Tourney->bFinished && LiveDayStart + Tourney->ClockMinutes() < Until; ++Guard)
	{
		for (const ss::TEvent& E : Tourney->SimulateTick())
		{
			if (E.Type == ss::TEventType::Bubble || E.Type == ss::TEventType::FinalTable || E.Type == ss::TEventType::Finished)
			{
				bHandForHand = bHandForHand && E.Type != ss::TEventType::Bubble;
				bFinalTable = bFinalTable || E.Type == ss::TEventType::FinalTable;
				News = E;
				bNews = true;
			}
			else if (E.Type == ss::TEventType::HandForHand)
			{
				bHandForHand = true;
			}
		}
	}
	if (bFinalTable && Stage)
	{
		Stage->SetOnAir(true);
	}
	if (bNews)
	{
		LiveEvent(News);
	}
	bOnBreak = false;
	Minutes = FMath::Max(Minutes, Until);
	const int32 Here = AnchorTableId();
	Table->FollowTable(Here, Here == Tourney->Hero().TableId ? Tourney->Hero().Seat : 0, true);
	bRailSeatsDue = false;
	RailT = 0.0f;
	PlaceExtras();
	UpdateRoomBoards();
	UE_LOG(LogEmbercrest, Log, TEXT("The wait passes: %s, %d left%s"), *ClockLabel(), Tourney->Remaining, Tourney->bFinished ? TEXT(" (over)") : TEXT(""));
}

void ABackRoomGameMode::NoteRailed(ss::SaveData& D) const
{
	if (!bRailing || bCancelled || !Tourney)
	{
		return;
	}
	// Who went out where after the player (the world people only), and the winner if it got that far.
	std::vector<ss::LiveSeen> Places;
	for (const ss::TPlayer& P : Tourney->Players)
	{
		if (P.IsHero || P.Id.rfind("npc:", 0) != 0 || P.Place <= 0 || P.Place >= HeroPlace)
		{
			continue;
		}
		ss::LiveSeen S;
		S.Tag = 'P';
		S.Name = P.Name;
		S.Value = P.Place;
		Places.push_back(S);
	}
	if (!Places.empty())
	{
		D.NoteLivePlaces(BustMinutes, std::string(TCHAR_TO_UTF8(*LiveEntryId)), Places);
	}
	UE_LOG(LogEmbercrest, Log, TEXT("Railed: %d places seen after %s"), static_cast<int32>(Places.size()), *Ordinal(HeroPlace));
}

void ABackRoomGameMode::NoteRailedPlaces()
{
	if (!bRailing || !Save.IsValid() || bCancelled || !Tourney)
	{
		return;
	}
	ss::SaveData D = *Save;
	NoteRailed(D);
	// The time it took (the walk home starts from the door now).
	D.ClockMinutes = FMath::Max(D.ClockMinutes, Minutes - ss::net::MinutesPerDay * ss::net::NightOneDay + 25.0);
	D.Life.Energy = FMath::Clamp(static_cast<double>(Energy), 0.0, 100.0);
	*Save = D;
	if (LiveSaver)
	{
		LiveSaver->Flush();
	}
	CareerSave::SaveNow(D.Serialize());
	UE_LOG(LogEmbercrest, Log, TEXT("Home at %s"), *ClockLabel());
}

void ABackRoomGameMode::LiveSettle()
{
	if (!Save.IsValid())
	{
		return;
	}
	ss::SaveData D = *Save;
	const std::string EntryId(TCHAR_TO_UTF8(*LiveEntryId));
	if (bCancelled)
	{
		ss::live::Refund(D.BankrollCents, D.Life, EntryId, Minutes, "not enough players");
	}
	else
	{
		// Once: a second call (or a reload after this save) finds the entry settled and changes nothing.
		ss::live::Settle(D.BankrollCents, D.Life, EntryId, HeroPlace, Tourney->Spec.Entrants, HeroPrizeCents, Minutes);
	}
	// The bus home. An event the desk wrote the player into during the night rides home on the night's first ticket,
	// paid when that one ended: one trip, one fare each way.
	if (ss::life::LiveEntry* Mine = bDeskEntry ? ss::live::EntryFor(D.Life, EntryId) : nullptr)
	{
		Mine->FareHome = true;
	}
	else
	{
		ss::live::PayFare(D.BankrollCents, D.Life, EntryId, true, Minutes);
	}
	D.ClockMinutes = Minutes - ss::net::MinutesPerDay * ss::net::NightOneDay + 25.0;
	D.Life.Energy = FMath::Clamp(static_cast<double>(Energy), 0.0, 100.0);
	for (const TPair<FString, int32>& R : Table->Reads)
	{
		D.Life.Reads[std::string(TCHAR_TO_UTF8(*R.Key))] = R.Value;
	}
	if (!bCancelled)
	{
		// What the player's tables saw, for the living world: who busted before them and where, who was still in,
		// who sat with them, who sent them home and whom they sent home. Only what happened in the open.
		std::vector<ss::LiveSeen> Seen;
		const ss::TPlayer& Me = Tourney->Hero();
		for (const ss::TPlayer& P : Tourney->Players)
		{
			if (P.IsHero || P.Id.rfind("npc:", 0) != 0)
			{
				continue; // the room's anonymous regulars: nobody the world follows
			}
			ss::LiveSeen S;
			S.Name = P.Name;
			if (P.Busted && P.Place > HeroPlace)
			{
				S.Tag = 'P';
				S.Value = P.Place;
				Seen.push_back(S);
			}
			else if (!P.Busted || P.Place < HeroPlace)
			{
				S.Tag = 'S';
				Seen.push_back(S);
			}
			if (LiveMet.Contains(Str(P.Id)))
			{
				S.Tag = 'M';
				S.Value = 0;
				Seen.push_back(S);
			}
			if (P.KnockedOutBy == ss::HeroId)
			{
				S.Tag = 'H';
				Seen.push_back(S);
			}
			if (Me.Busted && Me.KnockedOutBy == P.Id)
			{
				S.Tag = 'K';
				Seen.push_back(S);
			}
			// The biggest pot shown down with them tonight (the player's net, in chips): a memory, as at Dee's game.
			if (const int64* Net = LiveBigPots.Find(Str(P.Id)); Net && *Net != 0)
			{
				S.Tag = 'B';
				S.Value = *Net;
				Seen.push_back(S);
			}
		}
		D.NoteLive(Minutes, EntryId, HeroPlace, HeroPrizeCents, Tourney->Spec.Entrants, Seen);
		UE_LOG(LogEmbercrest, Log, TEXT("Told the world: %d things seen, %d big pots"), static_cast<int32>(Seen.size()),
			static_cast<int32>(std::count_if(Seen.begin(), Seen.end(), [](const ss::LiveSeen& S) { return S.Tag == 'B'; })));
	}
	*Save = D;
	// A checkpoint still being written lands first, so the settled night is the save that stays.
	if (LiveSaver)
	{
		LiveSaver->Flush();
	}
	CareerSave::SaveNow(D.Serialize());
}

void ABackRoomGameMode::LiveGoHome()
{
	if (bGoingHome || bOpeningNext)
	{
		return; // (on the way already: home, or into the next event's room)
	}
	// Never home with a seat waiting tonight: the desk takes the player off the next event's list first.
	if (!NextEntryId.IsEmpty() && !bOpeningNext)
	{
		LiveUnregister();
	}
	bGoingHome = true;
	ABackRoomChips::ChipUnit = 1;
	FString Options = FString::Printf(TEXT("Home?Live=1?Place=%d?Of=%d?Prize=%lld?Net=%lld?Learned=%d?From=%.0f"), HeroPlace, Tourney->Spec.Entrants, HeroPrizeCents, NetCents,
		Table ? Table->LearnedThisNight : 0, NightLeftHomeAt);
	if (bHeroWon)
	{
		Options += TEXT("?Won");
	}
	if (bCancelled)
	{
		Options += TEXT("?Cancelled");
	}
	Options += TEXT("?Event=") + LiveShort.Replace(TEXT(" "), TEXT("_"));
	// Out through the resort's entrance to the bus stop where it's been built (it opens Night One with the rest of these
	// once the player boards); else straight home.
	const bool bOutside = FPackageName::DoesPackageExist(TEXT("/Game/Maps/Embercrest"));
	UE_LOG(LogEmbercrest, Log, TEXT("%s: %s"), bOutside ? TEXT("Out through the entrance (Embercrest)") : TEXT("Home (NightOne)"), *Options);
	UGameplayStatics::OpenLevel(this, FName(bOutside ? TEXT("Embercrest") : TEXT("NightOne")), true, bOutside ? Options + TEXT("?Leaving") : Options);
}

void ABackRoomGameMode::UpdateBoard(float RealDt)
{
	BoardTick -= RealDt;
	if (BoardTick > 0.0f || !Stage || !Tourney)
	{
		return;
	}
	BoardTick = 0.25f;
	RoomBoardTick -= 0.25f;
	if (RoomBoardTick <= 0.0f)
	{
		RoomBoardTick = 3.0f;
		UpdateRoomBoards();
	}
	const ss::Level& L = Tourney->CurrentLevel();
	const ss::Level& N = Tourney->NextLevel();
	const int32 Secs = FMath::FloorToInt(LevelTimeLeft());
	const bool bBeforeCards = Phase == EBackRoomPhase::Arriving && !bAnnouncedStart && Tourney->Tick == 0;
	// In the last minutes of late registration the title takes turns with the last call.
	const bool bLastCallUp = bLastCalled && !bRegClosed && !bCancelled && FMath::FloorToInt(LiveT / 3.0f) % 2 == 1;
	const FString Title = bFinalTable ? FString(TEXT("FINAL TABLE"))
					  : bOnBreak	  ? FString(TEXT("BREAK"))
					  : bLastCallUp	  ? TEXT("LAST CALL  ·  REG TO ") + Str(ss::net::TimeLabel(LiveLateRegEnds))
									  : LiveName.ToUpper();
	const int32 BreakSecs = FMath::Max(0, FMath::FloorToInt(LiveBreakMinutes * 60.0 * (1.0 - static_cast<double>(BreakT) / BreakRealSeconds)));
	const FString Clock = bBeforeCards ? TEXT("SOON") : bOnBreak ? FString::Printf(TEXT("%02d:%02d"), BreakSecs / 60, BreakSecs % 60) : FString::Printf(TEXT("%02d:%02d"), Secs / 60, Secs % 60);
	const FString Blinds = L.Ante > 0 ? FString::Printf(TEXT("%s / %s   ANTE %s"), *Num(L.Sb), *Num(L.Bb), *Num(L.Ante)) : FString::Printf(TEXT("%s / %s"), *Num(L.Sb), *Num(L.Bb));
	const FString Next = FString::Printf(TEXT("NEXT   %s / %s"), *Num(N.Sb), *Num(N.Bb));
	// The champion as the room knows them: the person's name (the creator's), or the screen name for a career from before it.
	const FString Champion = !Save.IsValid() ? FString() : Save->Person.Created ? Str(Save->Person.FullName()).ToUpper() : Str(Save->HeroName);
	const FString Field = bHeroWon ? FString::Printf(TEXT("CHAMPION   %s"), *Champion)
								   : FString::Printf(TEXT("PLAYERS %d / %d     AVG %s     PAID %d"), Tourney->Remaining, Tourney->Spec.Entrants, *Num(static_cast<int64>(Tourney->AverageStack())), Tourney->PaidPlaces());
	Stage->SetBoard(Title, FString::Printf(TEXT("LEVEL %d"), Tourney->LevelIndex + 1), Clock, Blinds, Next, Field);
}

void ABackRoomGameMode::LiveTick(float RealDt)
{
	LiveT += RealDt;
	BannerAge += RealDt;
	ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	TWeakObjectPtr<ABackRoomGameMode> Self = this;

	for (const ss::TEvent& E : Table->TakeEvents())
	{
		LiveEvent(E);
	}
	// The hand's news heard: save the night where it stands (before the next hand is dealt).
	if (LiveSaver)
	{
		LiveSaver->Tick();
	}
	if (bCheckpointDue && Table->IsBetweenHands() && !Tourney->FinishPending())
	{
		LiveCheckpoint(false);
	}
	// A blink in progress: the bodies swap with the eyes shut, then they open.
	if (BlinkT >= 0.0f)
	{
		BlinkT += RealDt;
		if (BlinkT >= 0.1f || MoveT >= 0.0f)
		{
			ApplyBlinkSwaps();
			BlinkT = -1.0f;
			if (PC && PC->PlayerCameraManager && MoveT < 0.0f)
			{
				PC->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 0.16f, FLinearColor::Black, false, false);
			}
		}
	}
	// (Testing: chips, a forced move, between hands.)
	if (Phase == EBackRoomPhase::Playing && Table->IsBetweenHands() && (TestChips != 0 || bTestMove))
	{
		Tourney->Players[0].Stack = FMath::Max<int64>(1, Tourney->Players[0].Stack + TestChips);
		TestChips = 0;
		if (bTestMove)
		{
			bTestMove = false;
			for (const auto& It : Tourney->Tables)
			{
				if (It.first != Tourney->Hero().TableId)
				{
					Tourney->MoveToTable(ss::HeroId, It.first);
					Table->Hold();
					LiveMove();
					break;
				}
			}
		}
	}
	// (Testing: the whole room plays on at once, you on autopilot, between hands and once the last round's tables
	// have all finished: nothing may touch the tournament while a round is being finished over several frames.)
	if (FastForwardLeft > 0 && Phase == EBackRoomPhase::Playing && Table->IsBetweenHands() && !Tourney->FinishPending())
	{
		ss::Profile Auto = ss::MakeProfile(ss::Archetype::Tag, Tourney->R);
		bool bMoved = false;
		for (; FastForwardLeft > 0 && !Tourney->Hero().Busted && !Tourney->bFinished && !bMoved; --FastForwardLeft)
		{
			for (const ss::TEvent& E : Tourney->SimulateTick(&Auto))
			{
				bMoved = bMoved || (E.Type == ss::TEventType::Moved && E.IsHero);
				LiveEvent(E);
			}
		}
		FastForwardLeft = 0;
		Minutes = FMath::Max(Minutes, LiveDayStart + Tourney->ClockMinutes());
		bCheckpointDue = true;
		if (Tourney->Hero().Busted || Tourney->bFinished)
		{
			LiveOver();
			return;
		}
		if (bMoved)
		{
			Table->Hold();
			LiveMove();
		}
		else
		{
			PlaceExtras();
		}
	}
	// (Testing: out at the next gap between hands: one chip left and sitting out, the engine's blinds take it.)
	if (bTestBust && Phase == EBackRoomPhase::Playing && Table->IsBetweenHands() && !Tourney->FinishPending())
	{
		bTestBust = false;
		Tourney->Players[0].Stack = 1;
		Tourney->HeroSitsOut = true;
		for (int32 Guard = 0; Guard < 400 && !Tourney->Hero().Busted && !Tourney->bFinished; ++Guard)
		{
			for (const ss::TEvent& E : Tourney->SimulateTick())
			{
				LiveEvent(E);
			}
		}
		Tourney->HeroSitsOut = false;
		Minutes = FMath::Max(Minutes, LiveDayStart + Tourney->ClockMinutes());
		UE_LOG(LogEmbercrest, Log, TEXT("Test bust: %s of %d"), *Ordinal(Tourney->Hero().Place), Tourney->Spec.Entrants);
		LiveOver();
		return;
	}
	// The desk's signup, once its worker has read the world's field for the event.
	if (Signup.IsValid() && Signup->bDone)
	{
		FinishSignup();
	}
	if (bAnnouncedStart && Phase != EBackRoomPhase::Arriving)
	{
		LiveLastCall();
	}
	UpdateBoard(RealDt);

	// Walking in from the casino floor.
	ArrivalT += RealDt;
	if (Phase == EBackRoomPhase::Arriving)
	{
		StartWalkIn(Pawn);
		if (ArrivalBeat == 1 && ArrivalT > 3.2f)
		{
			ArrivalBeat = 2;
			// Dee deals the Sunday; any other day it's whoever's on the stick at the player's table.
			const bool bSunday = ((LiveDay % 7) + 7) % 7 == 6;
			if (bHere)
			{
				// On from the night's earlier event.
				Table->DealerLine(bSunday ? FString(TEXT("Back for another one, kid? You're with me."))
										  : FString::Printf(TEXT("Back for another one? Table %d, seat %d. Good luck."), Tourney->Hero().TableId, ss::live::SeatLabel(Tourney->Hero().Seat)));
			}
			else if (bBackInRoom && Tourney->Tick > 0)
			{
				Table->DealerLine(bResumeDealt ? FString(TEXT("Welcome back. Your hand was dead while you were up, you're in the next one."))
											   : bSunday ? FString(TEXT("There you are. Sit down, kid, your chips didn't go anywhere."))
														 : FString(TEXT("Welcome back. Your chips are right where you left them.")));
			}
			else
			{
				Table->DealerLine(bSunday ? FString(TEXT("Look who made it up the mountain. You're with me tonight, kid."))
										  : FString::Printf(TEXT("Evening. Table %d, seat %d, you're all set. Good luck."), Tourney->Hero().TableId, ss::live::SeatLabel(Tourney->Hero().Seat)));
			}
		}
		AttentionTick -= RealDt;
		if (Pawn && AttentionTick <= 0.0f)
		{
			AttentionTick = 0.5f;
			for (ABackRoomPlayer* O : Table->GetOpponents())
			{
				O->OnOtherAction(Pawn->GetEye(), 0.2f, false);
			}
		}
	}
	// Too few players: the floor cancels the event and the desk refunds everyone.
	if (bCancelled && Phase == EBackRoomPhase::Playing && !bLiveOver)
	{
		Floor(FString::Printf(TEXT("Ladies and gentlemen, with %d players tonight's %s is cancelled. Please see the desk for your refund."), Tourney->Spec.Entrants, *LiveShort));
		HeroPlace = 0;
		HeroPrizeCents = 0;
		LiveOver();
		return;
	}
	// Seated: the floor starts the night (or welcomes a late one).
	if (Phase == EBackRoomPhase::Playing && !bAnnouncedStart)
	{
		bAnnouncedStart = true;
		if (Tourney->Tick == 0)
		{
			Floor(FString::Printf(TEXT("Ladies and gentlemen, welcome to the Embercrest %s. %d players, a prize pool of %s. Dealers, shuffle up and deal!"), *LiveShort,
				Tourney->Spec.Entrants, *Dollars(Tourney->PrizePoolCents)));
			RaiseBanner(TEXT("SHUFFLE UP AND DEAL"));
		}
		else if (Minutes < LiveLateRegEnds)
		{
			Floor(FString::Printf(TEXT("Late registration closes at %s. %d players, %s in the pool."), *Str(ss::net::TimeLabel(LiveLateRegEnds)), Tourney->Spec.Entrants,
				*Dollars(Tourney->PrizePoolCents)));
			RaiseBanner(FString::Printf(TEXT("LEVEL %d   %s / %s"), Tourney->LevelIndex + 1, *Num(Tourney->CurrentLevel().Sb), *Num(Tourney->CurrentLevel().Bb)));
		}
		else
		{
			// (Seated after the close: written in at the desk in time, the floor's call and the walk took the rest.)
			Floor(FString::Printf(TEXT("Registration is closed. %d players, %s in the pool, the top %d paid."), Tourney->Spec.Entrants, *Dollars(Tourney->PrizePoolCents),
				Tourney->PaidPlaces()));
			RaiseBanner(FString::Printf(TEXT("LEVEL %d   %s / %s"), Tourney->LevelIndex + 1, *Num(Tourney->CurrentLevel().Sb), *Num(Tourney->CurrentLevel().Bb)));
		}
	}

	// The night's clock is the tournament's; it wears you down.
	if (Phase == EBackRoomPhase::Playing || Phase == EBackRoomPhase::Moving)
	{
		// The tournament's clock, running on through each hand (about forty-five seconds a hand here).
		const double Was = Minutes;
		const double PerHand = Tourney->Spec.SecondsPerHand;
		const double Progress = FMath::Min(static_cast<double>(Table->HandAge()) * PerHand / 45.0, PerHand - 1.0);
		Minutes = FMath::Max(Minutes, LiveDayStart + Tourney->ClockMinutes() + Progress / 60.0);
		Energy = FMath::Max(0.0f, Energy - static_cast<float>((Minutes - Was) / 60.0) * 4.0f);
		// The money, close: everything matters more.
		const float Pressure = bHandForHand ? 14.0f : (bFinalTable ? 10.0f : (Tourney->Remaining <= Tourney->PaidPlaces() + 6 ? 6.0f : 0.0f));
		Table->ExtraPressure = Pressure;
		if (Energy < 22.0f)
		{
			const float Tired = 1.0f - Energy / 22.0f;
			NextDroop -= RealDt;
			if (DroopT < 0.0f && NextDroop <= 0.0f)
			{
				DroopT = 0.0f;
				DroopLength = FMath::FRandRange(0.5f, 0.8f) + 0.9f * Tired;
				DroopDepth = FMath::Min(1.0f, FMath::FRandRange(0.4f, 0.7f) + 0.3f * Tired);
				NextDroop = FMath::FRandRange(9.0f, 20.0f) * (1.2f - 0.7f * Tired);
			}
		}
	}
	if (DroopT >= 0.0f)
	{
		DroopT += RealDt;
		const float U = DroopT / DroopLength;
		Eyelids = DroopDepth * (U < 0.7f ? FMath::SmoothStep(0.0f, 0.7f, U) : 1.0f - FMath::SmoothStep(0.7f, 1.0f, U));
		if (U >= 1.0f)
		{
			DroopT = -1.0f;
			Eyelids = 0.0f;
		}
	}
	// A break: the room stretches its legs while the screens count down, then the floor calls everyone back (the rail
	// waits it out too: no hands are dealt in the room).
	if (bOnBreak && (Phase == EBackRoomPhase::Playing || bRailing))
	{
		BreakT += RealDt;
		if (BreakT >= BreakRealSeconds)
		{
			bOnBreak = false;
			Floor(TEXT("Players, the break is over. Please take your seats, the cards are in the air."));
			RaiseBanner(FString::Printf(TEXT("LEVEL %d   %s / %s"), BreakLevel, *Num(Tourney->CurrentLevel().Sb), *Num(Tourney->CurrentLevel().Bb)));
			if (Phase == EBackRoomPhase::Playing)
			{
				Table->Resume(1.5f);
			}
		}
	}
	// Hand for hand: the table waits for the rest of the room, then the floor calls the next one.
	if (!bOnBreak && MoveT < 0.0f && Phase == EBackRoomPhase::Playing && Table->IsHolding())
	{
		MoveT += RealDt;
		if (MoveT >= 0.0f)
		{
			MoveT = -1000.0f;
			if (FMath::FRand() < 0.4f)
			{
				Floor(TEXT("All tables complete. Dealers, next hand."), false);
			}
			Table->Resume(0.5f);
		}
	}
	if (bLeaveWhenFree && Phase == EBackRoomPhase::Playing && Table->IsBetweenHands() && !Tourney->FinishPending())
	{
		LiveRequestLeave();
	}
	else if (QuitAskedAt >= 0.0f && LiveT - QuitAskedAt > 4.0f && !bLeaveWhenFree)
	{
		QuitAskedAt = -1.0f;
	}

	// Out (or champion): Dee's goodbye, the summary, the cage if you cashed, and the doors; or a while longer on the rail.
	if (Phase == EBackRoomPhase::Leaving)
	{
		LeaveT += RealDt;
		Eyelids = 0.0f;
		if (bRailing)
		{
			LiveRail(RealDt);
		}
		// The next event tonight: the floor calls its seats when they open, and a moment later the room opens again for it.
		if (!NextEntryId.IsEmpty() && !bExitNow && !bOpeningNext && !bWaitingOut)
		{
			if (!bNextCalled && Minutes >= NextSeatingAt)
			{
				LiveCallNext();
			}
			else if (bNextCalled && (NextCallT += RealDt) > 7.0f)
			{
				LiveOpenNext();
			}
		}
		if (bOpeningNext)
		{
			return;
		}
		if (bExitNow)
		{
			// Out through the entrance on foot: the night ends at the door.
			if (!bFadingOut && PC && PC->PlayerCameraManager)
			{
				bFadingOut = true;
				PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 1.2f, FLinearColor::Black, true, true);
			}
			ExitT += RealDt;
			if (ExitT > 1.4f && !bGoingHome)
			{
				NoteRailedPlaces();
				LiveGoHome();
			}
			return;
		}
		// The summary waits for the player (Q stays, L goes); a while, then home anyway.
		if (!bRailing && !bWalkingOut && LeaveT > 30.0f)
		{
			bWalkingOut = true;
			if (!Pawn || !Stage)
			{
				LiveGoHome();
				return;
			}
			const FVector Eye = Pawn->GetEye();
			const FTransform Room = Stage->GetRoomRoot() ? Stage->GetRoomRoot()->GetComponentTransform() : FTransform::Identity;
			if (HeroPrizeCents > 0)
			{
				// To the cage first.
				const TArray<FVector> ToCage = Stage->CardRoomToCage(Eye);
				Pawn->PlayWalk(ToCage, 7.0f, true, [Self, Room]() {
					ABackRoomGameMode* GM = Self.Get();
					if (!GM)
					{
						return;
					}
					GM->Table->Announce(TEXT("Cashier"), FString::Printf(TEXT("Congratulations%s. %s, cash. Count it with me... there you go."), *AddressTo(GM->Save),
						*Dollars(GM->HeroPrizeCents)));
					if (GM->Table->GetAudio())
					{
						GM->Table->GetAudio()->Play(ss::SoundId::Cash, 0.8f);
					}
					FTimerHandle Handle;
					GM->GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(GM, [GM, Room]() {
						ABackRoomPawn* P = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(GM, 0));
						if (!P)
						{
							GM->LiveGoHome();
							return;
						}
						const TArray<FVector> Out = GM->Stage->CardRoomCageToDoor(P->GetEye());
						TWeakObjectPtr<ABackRoomGameMode> Self3 = GM;
						P->PlayWalk(Out, 9.0f, true, [Self3]() {
							if (ABackRoomGameMode* G = Self3.Get())
							{
								G->LiveGoHome();
							}
						});
						GM->LeaveT = 100.0f; // fade from here
						GM->bFadingOut = false;
						GM->MoveT = 6.2f;
					}), 3.0f, false);
				});
			}
			else
			{
				Pawn->PlayWalk(Stage->CardRoomWalkOut(Eye), 9.0f, true, [Self]() {
					if (ABackRoomGameMode* GM = Self.Get())
					{
						GM->LiveGoHome();
					}
				});
				MoveT = 6.2f;
				LeaveT = 100.0f;
			}
		}
		// Fade out at the doors.
		if (LeaveT >= 100.0f && !bFadingOut && PC && PC->PlayerCameraManager && LeaveT - 100.0f > MoveT)
		{
			bFadingOut = true;
			PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 1.5f, FLinearColor::Black, true, true);
		}
	}
}

// ------------------------------------------------------------------ the floor's last call

void ABackRoomGameMode::LiveLastCall()
{
	if (bCancelled || !Tourney)
	{
		return;
	}
	if (!bLastCalled && Minutes >= LastCallAt(LiveLateRegEnds) && Minutes < LiveLateRegEnds)
	{
		bLastCalled = true;
		Floor(FString::Printf(TEXT("Ladies and gentlemen, this is the last call for registration for the Embercrest %s. Late registration closes at %s."), *LiveShort,
			*Str(ss::net::TimeLabel(LiveLateRegEnds))));
		RaiseBanner(TEXT("LAST CALL FOR REGISTRATION"));
		UpdateRoomBoards();
	}
	if (!bRegClosed && Minutes >= LiveLateRegEnds)
	{
		// (On a level's turn: the level's own banner stays up.)
		bRegClosed = true;
		bLastCalled = true;
		Floor(FString::Printf(TEXT("Registration for the %s is now closed. %d players, a prize pool of %s, and the top %d are paid."), *LiveShort, Tourney->Spec.Entrants,
			*Dollars(Tourney->PrizePoolCents), Tourney->PaidPlaces()));
		UpdateRoomBoards();
	}
}

// ------------------------------------------------------------------ the next event

FString ABackRoomGameMode::NextOpenEvent(FString& OutWhyNot) const
{
	OutWhyNot.Empty();
	if (!Save.IsValid() || !bLiveOver || bCancelled || ss::live::ActiveEntry(Save->Life))
	{
		return FString();
	}
	// Today's events the player hasn't played, while the walk to the seat still makes late registration. What the desk
	// sells is the entry (the night's bus home is paid), and a long night needs the legs for it, as at home.
	const ss::life::Activity* Trip = ss::life::Find("embercrest");
	for (const ss::live::Occurrence& O : ss::live::Occurrences(LiveDay))
	{
		if (Str(O.Id) == LiveEntryId || ss::live::EntryFor(Save->Life, O.Id) || Minutes >= O.LateRegEnds)
		{
			continue;
		}
		const FString Short = Str(O.T->Short);
		if (Minutes + DeskClosesBefore > O.LateRegEnds)
		{
			OutWhyNot = OutWhyNot.IsEmpty() ? FString::Printf(TEXT("Registration for the %s is closing. There isn't time to get you to a seat, sorry."), *Short) : OutWhyNot;
			continue;
		}
		if (Minutes < O.DeskOpens)
		{
			OutWhyNot = OutWhyNot.IsEmpty() ? FString::Printf(TEXT("Registration for the %s opens at %s. Come see me then."), *Short, *Str(ss::net::TimeLabel(O.DeskOpens))) : OutWhyNot;
			continue;
		}
		if (Save->BankrollCents < O.T->BuyInCents)
		{
			OutWhyNot = OutWhyNot.IsEmpty() ? FString::Printf(TEXT("The %s is %s. Come back when you've got it."), *Short, *Dollars(O.T->BuyInCents)) : OutWhyNot;
			continue;
		}
		if (Trip && Energy < Trip->Energy)
		{
			OutWhyNot = OutWhyNot.IsEmpty() ? FString(TEXT("A tournament's a long night, hon, and you look done in. Go home and get some sleep.")) : OutWhyNot;
			continue;
		}
		return Str(O.Id);
	}
	return FString();
}

void ABackRoomGameMode::DeskSignup(const FString& OccurrenceId)
{
	const ss::live::Occurrence O = ss::live::FindOccurrence(std::string(TCHAR_TO_UTF8(*OccurrenceId)));
	if (!O.Valid() || !Save.IsValid() || Signup.IsValid() || !NextEntryId.IsEmpty())
	{
		return;
	}
	// The world's field for it, read on a worker (the save's world is megabytes of text); the entry when it's done.
	TSharedPtr<FLiveDeskSignup> S = MakeShared<FLiveDeskSignup>();
	S->WorldText = Save->WorldText;
	S->HeroName = Save->HeroName;
	S->O = O;
	S->At = Minutes;
	Signup = S;
	UE::Tasks::Launch(UE_SOURCE_LOCATION, [S]() { S->Run(); });
	UE_LOG(LogEmbercrest, Log, TEXT("The desk writes the player in for %s (%s)"), *OccurrenceId, *ClockLabel());
}

void ABackRoomGameMode::FinishSignup()
{
	const TSharedPtr<FLiveDeskSignup> S = Signup;
	Signup.Reset();
	if (!S.IsValid() || !Save.IsValid() || bGoingHome || bExitNow || bOpeningNext)
	{
		return; // on the way out: the desk never wrote it down
	}
	const ss::live::Occurrence& O = S->O;
	ss::SaveData D = *Save;
	const std::string WorldWas = D.WorldText;
	// At the desk, here already (the core's check counts the trip there: it's made).
	const double Booked = Minutes - ss::live::TravelMinutes;
	const std::string Why = RegisterFromSignup(D, *S, Booked, true);
	ss::life::LiveEntry* E = Why.empty() ? ss::live::EntryFor(D.Life, O.Id) : nullptr;
	if (!E)
	{
		Table->Announce(TEXT("Desk"), Str(Why));
		UE_LOG(LogEmbercrest, Warning, TEXT("The desk can't register the player for %s: %s"), *Str(O.Id), *Str(Why));
		return;
	}
	// Registered and arrived both now: no bus there, and none home of its own (the night's ticket covers it). The ledger's
	// lines carry the desk's own time.
	E->RegisteredAt = Minutes;
	E->ArrivedAt = Minutes;
	E->FareThere = true;
	// The night's own departure, kept on the entry until its first checkpoint carries it (a game closed while waiting goes
	// back to this room, and Night One still catches its calendar up from the night's first bus).
	E->CheckpointHost = std::string(TCHAR_TO_UTF8(*FString::Printf(TEXT("from\t%.2f\n"), NightLeftHomeAt)));
	for (size_t K = 0; K < D.Life.Ledger.size() && K < 2; ++K)
	{
		if (D.Life.Ledger[K].At == Booked)
		{
			D.Life.Ledger[K].At = Minutes;
		}
	}
	D.ClockMinutes = Minutes - ss::net::MinutesPerDay * ss::net::NightOneDay;
	D.Life.Energy = FMath::Clamp(static_cast<double>(Energy), 0.0, 100.0);
	if (S->bWorld)
	{
		WorldBeforeSignup = WorldWas;
	}
	*Save = D;
	if (LiveSaver)
	{
		LiveSaver->Flush();
	}
	CareerSave::SaveNow(D.Serialize());
	NextEntryId = Str(O.Id);
	NextName = Str(O.T->Short);
	NextSeatingAt = FMath::Max(Minutes, O.Start - SeatingLead);
	bNextCalled = false;
	NextCallT = 0.0f;
	Table->Announce(TEXT("Desk"), Minutes >= NextSeatingAt
									  ? FString::Printf(TEXT("You're in%s. %s. They're seating it now; the floor will call you over."), *AddressTo(Save), *Str(O.T->Name))
									  : FString::Printf(TEXT("You're in%s. %s. Seats open at %s; the floor will call it. The bar's open, the terrace too."), *AddressTo(Save),
											*Str(O.T->Name), *Str(ss::net::TimeLabel(NextSeatingAt))));
	if (Table->GetAudio())
	{
		Table->GetAudio()->Play(ss::SoundId::Cash, 0.5f);
	}
	UpdateRoomBoards();
	UE_LOG(LogEmbercrest, Log, TEXT("Registered at the desk for %s: %d entrants (%d the world follows, %d faces), bankroll %lld; seats at %s"), *NextEntryId, E->Entrants,
		static_cast<int32>(E->Roster.size()), static_cast<int32>(E->Faces.size()), static_cast<int64>(D.BankrollCents), *Str(ss::net::TimeLabel(NextSeatingAt)));
}

void ABackRoomGameMode::LiveUnregister()
{
	if (NextEntryId.IsEmpty() || !Save.IsValid())
	{
		return;
	}
	// Off the list before sitting down: the buy-in and fee back (their own ledger lines), and the world as it was before the
	// desk told it. The entry goes with them: a refunded entry reads as an event the room called off (the desk turns the
	// player away from it as cancelled, the phone's card says so), and this one wasn't. The player can buy in again from
	// home while its registration's open.
	ss::SaveData D = *Save;
	const std::string Id(TCHAR_TO_UTF8(*NextEntryId));
	if (ss::live::Refund(D.BankrollCents, D.Life, Id, Minutes, "off the list before sitting down"))
	{
		D.Life.LiveEntries.erase(std::remove_if(D.Life.LiveEntries.begin(), D.Life.LiveEntries.end(), [&Id](const ss::life::LiveEntry& E) { return E.Id == Id; }),
			D.Life.LiveEntries.end());
		if (!WorldBeforeSignup.empty())
		{
			D.WorldText = WorldBeforeSignup;
		}
	}
	*Save = D;
	if (LiveSaver)
	{
		LiveSaver->Flush();
	}
	CareerSave::SaveNow(D.Serialize());
	UE_LOG(LogEmbercrest, Log, TEXT("Off the list for %s, refunded"), *NextEntryId);
	WorldBeforeSignup.clear();
	NextEntryId.Empty();
	NextName.Empty();
	bNextCalled = false;
	UpdateRoomBoards();
}

void ABackRoomGameMode::LiveCallNext()
{
	bNextCalled = true;
	NextCallT = 0.0f;
	const ss::live::Occurrence O = ss::live::FindOccurrence(std::string(TCHAR_TO_UTF8(*NextEntryId)));
	if (!O.Valid())
	{
		return;
	}
	Floor(Minutes >= O.Start
			  ? FString::Printf(TEXT("Players registered for the %s, your seats are ready. Late registration closes at %s."), *NextName, *Str(ss::net::TimeLabel(O.LateRegEnds)))
			  : FString::Printf(TEXT("Players registered for the %s, your seats are open. Please find your table and seat; cards go in the air at %s."), *NextName,
					*Str(ss::net::TimeLabel(O.Start))));
	RaiseBanner(FString::Printf(TEXT("%s   SEATS OPEN"), *NextName.ToUpper()));
	UpdateRoomBoards();
	UE_LOG(LogEmbercrest, Log, TEXT("The floor calls %s (%s)"), *NextEntryId, *ClockLabel());
}

void ABackRoomGameMode::LiveOpenNext()
{
	if (bOpeningNext || bGoingHome || NextEntryId.IsEmpty() || !Save.IsValid())
	{
		return;
	}
	bOpeningNext = true;
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController(); PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 1.2f, FLinearColor::Black, true, true);
	}
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]() {
		if (bGoingHome)
		{
			return; // quit from the pause menu meanwhile: that travel stands (Continue comes back to the entry)
		}
		// What the rail saw, the champion for the next event's board if the room crowned one, and the clock as it stands:
		// the room opens again for the next event with no bus and no trip, and the night's departure for Night One.
		ss::SaveData D = *Save;
		NoteRailed(D);
		ss::life::LiveEntry* Next = ss::live::EntryFor(D.Life, std::string(TCHAR_TO_UTF8(*NextEntryId)));
		const ss::TPlayer* Champion = nullptr;
		for (const ss::TPlayer& P : Tourney->Players)
		{
			Champion = Tourney->bFinished && P.Place == 1 ? &P : Champion;
		}
		if (Next && Champion)
		{
			static const TCHAR* Days[7] = {TEXT("MON"), TEXT("TUE"), TEXT("WED"), TEXT("THU"), TEXT("FRI"), TEXT("SAT"), TEXT("SUN")};
			const FString Who = Champion->IsHero ? Str(D.HeroName) : Str(Champion->Name);
			const FString Line = FString::Printf(TEXT("%s %s\t%s\t%lld"), Days[((LiveDay % 7) + 7) % 7], *LiveShort.ToUpper(), *Who, static_cast<int64>(Tourney->PrizeFor(1)));
			Next->Board.insert(Next->Board.begin(), std::string(TCHAR_TO_UTF8(*Line)));
			Next->Board.resize(FMath::Min<size_t>(Next->Board.size(), 8));
		}
		D.ClockMinutes = Minutes - ss::net::MinutesPerDay * ss::net::NightOneDay;
		D.Life.Energy = FMath::Clamp(static_cast<double>(Energy), 0.0, 100.0);
		*Save = D;
		if (LiveSaver)
		{
			LiveSaver->Flush();
		}
		CareerSave::SaveNow(D.Serialize());
		UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
		const FString Options = FString::Printf(TEXT("Live=%s?Here=1?At=%.2f?From=%.2f"), *NextEntryId, Minutes, NightLeftHomeAt);
		UE_LOG(LogEmbercrest, Log, TEXT("On to the next event: %s"), *Options);
		UGameplayStatics::OpenLevel(this, FName(TEXT("BackRoom")), true, Options);
	}), 1.3f, false);
}

FString ABackRoomGameMode::GetNextEventNote() const
{
	if (NextEntryId.IsEmpty())
	{
		return FString();
	}
	return bNextCalled ? FString::Printf(TEXT("Registered: %s, seating now"), *NextName)
					   : FString::Printf(TEXT("Registered: %s, seats at %s"), *NextName, *Str(ss::net::TimeLabel(NextSeatingAt)));
}

// ------------------------------------------------------------------ test hooks

void ABackRoomGameMode::TestBustNow()
{
	if (!bLive || !Tourney || bLiveOver || bCancelled || Tourney->Hero().Busted || Tourney->bFinished)
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("Test bust: the player isn't in a tournament here"));
		return;
	}
	bTestBust = true;
	UE_LOG(LogEmbercrest, Log, TEXT("Test bust: at the next gap between hands"));
}

void ABackRoomGameMode::TestLastCall()
{
	if (!bLive || !Tourney || bCancelled)
	{
		return;
	}
	const double At = LastCallAt(LiveLateRegEnds);
	if (Minutes >= LiveLateRegEnds)
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("Test last call: registration closed at %s"), *Str(ss::net::TimeLabel(LiveLateRegEnds)));
		return;
	}
	if (Minutes >= At)
	{
		bLastCalled = false; // inside the window: the floor calls it again, now
		return;
	}
	if (bLiveOver)
	{
		RailCatchUp(At - 0.5);
		return;
	}
	const int32 Hands = FMath::Max(0, FMath::FloorToInt((At - 0.5 - (LiveDayStart + Tourney->ClockMinutes())) * 60.0 / Tourney->Spec.SecondsPerHand));
	UE_LOG(LogEmbercrest, Log, TEXT("Test last call: %d hands on to %s"), Hands, *Str(ss::net::TimeLabel(At)));
	TestFastForward(Hands);
}

void ABackRoomGameMode::TestRegisterNext(const FString& OccurrenceId)
{
	if (!bLive || !bLiveOver || bCancelled || !NextEntryId.IsEmpty() || Signup.IsValid())
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("Test register: only once the player's night is over, with nothing registered yet"));
		return;
	}
	if (!bRailing)
	{
		LiveStay();
	}
	FString WhyNot;
	const FString Id = OccurrenceId.IsEmpty() ? NextOpenEvent(WhyNot) : OccurrenceId;
	if (Id.IsEmpty())
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("Test register: nothing the desk can sell now (%s)"), WhyNot.IsEmpty() ? TEXT("no event left today") : *WhyNot);
		return;
	}
	UE_CLOG(!bRailing, LogEmbercrest, Warning, TEXT("Test register: not on the rail yet (Q stays): leaving takes the player off the list"));
	DeskSignup(Id);
}

void ABackRoomGameMode::TestSeatingNow()
{
	if (NextEntryId.IsEmpty() || bNextCalled || bOpeningNext)
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("Test seating: no next event waiting"));
		return;
	}
	RailCatchUp(NextSeatingAt);
}
