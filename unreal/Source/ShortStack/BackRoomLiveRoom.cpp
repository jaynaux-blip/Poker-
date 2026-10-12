// The Embercrest's card room as a room (ABackRoomGameMode's live tournaments; the flow is BackRoomLive.cpp): the other
// tables and the people at them (the near tables' MetaHumans, the far crowd's figures, the railbirds and the player's
// backers), the banners, what the tournament's events and the table's notes mean to the room, the rail after the
// player's night is over, and the floor's last call for registration.
//
// The room's one voice (next_segment_spec 1.5, 4.11, ue-room): the director (ss::room::Director) is built once the field
// is seated and fed every tick in its order; its cues wait here for their time and their speech (rendered on a worker),
// then play: the PA and the far tables through the stereo room (FCardRoomSound), the player's own table as one engine voice
// at the speaker's head that ducks the room, the crowd's vocals, the phone's buzz, and a caption for the HUD with each sound,
// never before it.

#include "BackRoomLiveShared.h"

#include "BackRoomGameMode.h"
#include "BackRoomPlayer.h"
#include "BackRoomStage.h"
#include "BackRoomTable.h"
#include "Camera/PlayerCameraManager.h"
#include "CardRoomSound.h"
#include "CareerSave.h"
#include "CastLooks.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "NightOneAudio.h"
#include "Sound/SoundWave.h"
#include "SponsorWear.h"
#include "TimerManager.h"
#include "Tasks/Task.h"
#include "ShortStack/Audio/Synth.h"
#include "ShortStack/Audio/Voice.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Room.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Stake.h"
#include "ShortStack/Game/TableTalk.h"
#include "ShortStack/Tournament.h"
#include "ShortStack/UI/FrontEnd.h"

#include <algorithm>
#include <cmath>

namespace LiveDetail
{
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
 * The body an entrant's tournament id picks (LIVE_TOURNAMENTS.md section 10: a look is a hash of the id): CastLooks owns it
 * now (spec 7.5.4), shared with the street and the Embercrest: their own MetaHuman for the cast, the block's face for the
 * block's people (Lonnie at the Monday Nightly), one of the room's that suits a regular of the room's, any of the twelve
 * for anyone else (the same pick as before for everyone else).
 */
FString DefaultBody(const FString& Id, const FString& Name)
{
	return CastLooks::DefaultBody(Id, Name);
}

/** On the rail: a round of the room every few real seconds (the old table's chairs settle RailSettleSeconds after it). */
constexpr float RailRoundSeconds = 4.5f;

// ------------------------------------------------------------------ the room's sound: tunables and places

// The listening gate's result (next_segment_spec 11, the user, 2026-10-11): the synthesized talkers read as "really
// creepy", so the FALLBACK ships and is the only mode in the shipped settings: walla, the non-word vocals, chips, cards,
// the far dealers' calls, the PA and its chime; every line is captioned, and a line at the player's table (a person's
// or the dealer's) is at most a soft breath under its caption, with mouths on their talk timers. The formant talkers,
// the near murmur and MouthAt lip sync stay in the code behind this dev-only flag (a cheat: hidden and fixed in a final
// build, and read as 0 in Shipping).
TAutoConsoleVariable<int32> CVarRoomTalkers(TEXT("ss.Room.Talkers"), 0,
	TEXT("DEV ONLY. The card room's own formant talkers (next_segment_spec 4.10): 0 (the default and what ships: the fallback the user chose at the ")
		TEXT("listening gate, walla and non-word vocals, the near lines a soft breath under their captions). 1..12 turn the rejected formant talkers, the near ")
		TEXT("murmur and its lip sync back on, for comparison only."),
	ECVF_Cheat);
TAutoConsoleVariable<float> CVarRoomNearDb(TEXT("ss.Room.NearDb"), -25.0f,
	TEXT("A line at the player's table: its murmur's speaking level, dBFS RMS, before the Voices bus (the fallback's breath is 9 dB under it)."), ECVF_Cheat);
TAutoConsoleVariable<float> CVarRoomDealerDb(TEXT("ss.Room.DealerDb"), -21.0f,
	TEXT("DEV ONLY (talkers above 0): the player's dealer's clear calls, their speaking level, dBFS RMS, before the Voices bus."), ECVF_Cheat);

/** The room's talkers now: the dev flag outside Shipping, always the fallback (0) in it. */
int32 RoomTalkers()
{
#if UE_BUILD_SHIPPING
	return 0;
#else
	return FMath::Clamp(CVarRoomTalkers.GetValueOnGameThread(), 0, 12);
#endif
}

/**
 * The room's places in room space (BackRoomStageCardRoom.cpp's layout): the slot floor through the entrance, the bar along
 * the +Y wall, the rail at the stage's edge, the desk and the cage on the near wall, the terrace doors.
 */
const FVector2D DoorwayAt(-1200.0, -1800.0);
const FVector2D BarAt(-900.0, 1350.0);
const FVector2D RailEdgeAt(1150.0, 0.0);
const FVector2D DeskAt(-1620.0, -1000.0);
const FVector2D CageAt(-1620.0, 1000.0);
const FVector2D TerraceAt(400.0, 1700.0);
/** A dealer's quiet call ("All in.", "Seat open, four.") carries about 12 m; a shout carries the room (spec 4.6). */
constexpr double QuietCallReach = 1200.0;
constexpr float ShoutEffort = 0.85f;
/** The near tier's MetaHumans (PlaceExtras): the backers at the rail count inside it (spec 2.13). */
constexpr int32 NearBudgetAll = 16;

FString StrOf(const std::string& S)
{
	return FString(UTF8_TO_TCHAR(S.c_str()));
}

std::string Std(const FString& S)
{
	return std::string(TCHAR_TO_UTF8(*S));
}

bool StartsWith(const std::string& S, const char* Prefix)
{
	return S.rfind(Prefix, 0) == 0;
}

/**
 * The table's mid-hand news that an all-in has been called with the board to come (ue-table's OnAllInCalled, where the
 * table has it): the rail leans in while the cards turn. (A table without the hook: the dealer's "Turn them up" line,
 * heard through the room, does the same; PlayCue.)
 */
template <typename TTable>
void BindAllInCalled(TTable& Table, TFunction<void(bool)> Fn)
{
	if constexpr (requires { Table.OnAllInCalled = Fn; })
	{
		Table.OnAllInCalled = MoveTemp(Fn);
	}
}

/** On their feet (ue-wear's ABackRoomPlayer::SetStanding, where the body has it): whether they stand. */
template <typename TPlayer>
bool StandUp(TPlayer& P)
{
	if constexpr (requires { P.SetStanding(true); })
	{
		return P.SetStanding(true);
	}
	else
	{
		return false;
	}
}

template <typename TPlayer>
void SitDown(TPlayer& P)
{
	if constexpr (requires { P.SetStanding(false); })
	{
		P.SetStanding(false);
	}
}

/**
 * A near table's player dressed as themselves (ue-wear's request, patches/ue-wear_to_ue-room_backers_and_extras.patch): the
 * world's sponsored people wear their brand's patch there as at the player's own table (SponsorWear::ForWorldPerson; Team
 * RiverLine's pros on the chest, an energy drink's on a cap or the sleeve); dealers plain. A no-op when they already wear it.
 */
template <typename TPlayer>
void DressPatches(TPlayer& P, const FString& Brand)
{
	if constexpr (requires { P.WearPatches(Brand, Brand, Brand); P.CanWearCapPatch(); })
	{
		const SponsorWear::FShown Theirs = Brand.IsEmpty() ? SponsorWear::FShown() : SponsorWear::ForWorldPerson(Brand, P.CanWearCapPatch());
		P.WearPatches(Theirs.Brand[SponsorWear::Chest], Theirs.Brand[SponsorWear::Sleeve], Theirs.Brand[SponsorWear::Cap]);
	}
}

// What the host binds at compile time is there (keep these signatures: the detection above would otherwise quietly skip
// the backers standing, the extras' patches and the rail's lean-in).
static_assert(requires(ABackRoomPlayer& P, const FString& S) {
	P.SetStanding(true);
	P.WearPatches(S, S, S);
	P.CanWearCapPatch();
} && std::is_same_v<decltype(std::declval<ABackRoomPlayer&>().SetStanding(true)), bool>,
	"ABackRoomPlayer: StandUp/SitDown/DressPatches need bool SetStanding(bool), WearPatches(FString x3) and CanWearCapPatch()");
static_assert(requires(ABackRoomTable& T, TFunction<void(bool)> Fn) { T.OnAllInCalled = Fn; },
	"ABackRoomTable: BindAllInCalled needs TFunction<void(bool)> OnAllInCalled");

/**
 * The floor's PA before the room had a host (Dee's game never has one): the chime, a beat, the caption. Floor() splits a
 * long announcement into caption-sized parts, the chime on the first: the later parts come after it, in order.
 */
void LegacyFloor(ABackRoomGameMode& GM, const FString& Line, bool bChime, int32 Part)
{
	ABackRoomTable* Table = GM.Table;
	if (bChime && Table && Table->GetAudio())
	{
		Table->GetAudio()->PlayEffect(ss::audio::Effect::Chime, 0.55f);
	}
	if (Table)
	{
		TWeakObjectPtr<ABackRoomTable> T = Table;
		FTimerHandle Handle;
		GM.GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(&GM, [T, Line]() {
			if (ABackRoomTable* Tb = T.Get())
			{
				Tb->Announce(TEXT("Floor"), Line);
			}
		}), bChime || Part > 0 ? 1.1f + 0.05f * static_cast<float>(FMath::Max(0, Part)) : 0.01f, false);
	}
}
} // namespace LiveDetail

using namespace LiveDetail;

// ------------------------------------------------------------------ the room's people

void ABackRoomGameMode::PlaceExtras()
{
	if (!Stage || !Tourney)
	{
		return;
	}
	// The room's fidelity tiers (docs/LIVE_TOURNAMENTS.md section 4): the nearest tables get MetaHumans, as many as
	// the budget seats; every other running table gets the crowd kit's figures. Either way, a table shows exactly
	// the players the tournament has at it, and its dealer. The player's backers at the rail count inside the budget
	// (spec 2.13): their neighbours fold into the crowd kit to make room.
	const int32 NearBudget = NearBudgetAll - (RoomHost.IsValid() && RoomHost->bRailPlaced ? static_cast<int32>(RoomHost->RailSet.size()) : 0);
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
		// Their sponsor's patch (the field's livepatch lines; a re-dressed extra comes back bare and is dressed again here).
		DressPatches(*E, S.bDealer ? FString() : FacePatch(S.Name));
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
	// (The player's backers, standing at the bar, take the line's last places: PlaceBackers.)
	const int32 BarTaken = RoomHost.IsValid() ? FMath::Clamp(RoomHost->RailAtBar, 0, 2) : 0;
	for (int32 I = 0; I < 5 - BarTaken; ++I)
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

// ------------------------------------------------------------------ the floor

void ABackRoomGameMode::RaiseBanner(const FString& Text)
{
	Banner = Text;
	BannerAge = 0.0f;
}

void ABackRoomGameMode::Floor(const FString& Line, bool bChime)
{
	// The floor on the PA, said by the host itself (the director's own calls come as its cues): the desk's call for the
	// next event's seats, a cancelled night. One caption a sentence or two (room::Announcement), the chime on the first.
	const std::vector<std::string> Parts = ss::room::Announcement(Std(Line));
	for (size_t I = 0; I < Parts.size(); ++I)
	{
		ss::room::Cue C;
		C.What = ss::room::Kind::Pa;
		C.Where = ss::room::From::Pa;
		C.Speaker = "Floor";
		C.Key = "host.floor";
		C.Text = Parts[I];
		C.Part = static_cast<int>(I);
		C.Chime = bChime && I == 0;
		C.Priority = 2;
		C.Effort = 0.7f;
		C.Tick = Tourney ? Tourney->Tick : 0;
		PlayCue(C);
	}
}

void ABackRoomGameMode::LiveEvent(const ss::TEvent& E)
{
	// The host's part of the engine's news: banners, the room's people, the stage, a hold, a move. What's said about it
	// (the floor, the dealers, the table's goodbyes, the applause) is the director's (TickRoom feeds it the same events).
	switch (E.Type)
	{
	case ss::TEventType::Bust:
		if (E.IsHero)
		{
			HeroPlace = E.Place;
			HeroPrizeCents = E.PrizeCents;
		}
		break;
	case ss::TEventType::Level:
	{
		const ss::Level& L = E.Blinds;
		RaiseBanner(FString::Printf(TEXT("LEVEL %d   %s / %s"), E.LevelNumber, *Num(L.Sb), *Num(L.Bb)));
		break;
	}
	case ss::TEventType::TableBroken:
		PlaceExtras();
		break;
	case ss::TEventType::Break:
	{
		bOnBreak = true;
		BreakT = 0.0f;
		BreakLevel = E.LevelNumber;
		RaiseBanner(FString::Printf(TEXT("BREAK   %d MINUTES"), FMath::RoundToInt(LiveBreakMinutes)));
		if (Phase != EBackRoomPhase::Playing || !Tourney->Hero().Busted)
		{
			// (Out in the hand before it: no break to sit through. The table's next deal finds no hand and the night ends.)
			Table->Hold();
		}
		break;
	}
	case ss::TEventType::HandForHand:
		bHandForHand = true;
		RaiseBanner(TEXT("HAND FOR HAND"));
		break;
	case ss::TEventType::Bubble:
		bHandForHand = false;
		RaiseBanner(TEXT("IN THE MONEY"));
		break;
	case ss::TEventType::FinalTable:
		bFinalTable = true;
		if (!Tourney->Hero().Busted && Stage && Stage->GetRoomAnchor() != ABackRoomStage::StreamSlot() && Phase == EBackRoomPhase::Playing)
		{
			// Up onto the stage with your chips: the final table plays at the stream table.
			Table->Hold();
			LiveMove();
		}
		RaiseBanner(TEXT("FINAL TABLE"));
		Stage->SetOnAir(true);
		break;
	case ss::TEventType::Finished:
		if (E.Id != ss::HeroId && bRailing)
		{
			RaiseBanner(FString::Printf(TEXT("CHAMPION   %s"), *Str(E.Name).ToUpper()));
		}
		if (E.Id == ss::HeroId)
		{
			bHeroWon = true;
			HeroPlace = 1;
			HeroPrizeCents = Tourney->PrizeFor(1);
			RaiseBanner(TEXT("CHAMPION"));
		}
		break;
	case ss::TEventType::Moved:
		if (E.IsHero && RoomHost.IsValid())
		{
			// (Bug 2: a move names the table the player was moved from, the engine's id, never the room's slot.)
			RoomHost->HeroTable = E.From;
			RoomHost->MovedFrom = E.From;
			RoomHost->MovedTick = Tourney->Tick;
		}
		break;
	default:
		break;
	}
	// A catch-up the host plays in one frame (a dev fast-forward; a test bust, the player sitting out until the blinds take
	// the last chip) is heard only where it lands, as the director's catch-up is: not every level and bust it passes. (A
	// blind-off leaves HeroSitsOut set, but the player is out by then: the rail after it is heard.)
	const bool bPassing = Tourney && ((Tourney->HeroSitsOut && !Tourney->Hero().Busted) || (FastForwardLeft > 1 && !(FastForwardStop != 0 && FastForwardReached())));
	if (!RoomDirector.IsValid() && Tourney && Table && !bPassing)
	{
		// No director (BuildDirector found no entry or event for tonight and said so in the log): the floor and the dealer
		// still call the night's milestones, in the room's own copy (talk::Floor, talk::Dealer) and by rule, not dice (spec
		// N5), gated the way the director gates them. With a director every word of this is the director's.
		const int32 HeroTable = Tourney->Hero().Busted ? -1 : Tourney->Hero().TableId;
		const bool bSunday = ((LiveDay % 7) + 7) % 7 == 6;
		auto Hash = [this](const TCHAR* What, int32 N) { return FCrc::StrCrc32(*FString::Printf(TEXT("%s:%s:%d"), What, *LiveEntryId, N)); };
		auto FloorText = [this, &Hash](const char* Call, const std::map<std::string, std::string>& Vars) {
			return ss::talk::Floor(Call, Vars, Hash(UTF8_TO_TCHAR(Call), Tourney->Tick)).Text;
		};
		auto FloorSays = [this, &FloorText](const char* Call, const std::map<std::string, std::string>& Vars, bool bChime) {
			const std::string Text = FloorText(Call, Vars);
			if (!Text.empty())
			{
				Floor(Str(Text), bChime);
			}
		};
		// The dealer answers the floor once it has spoken (the PA waits out its chime, then reads), as the director orders
		// them; a word of the dealer's own (the cage) comes at once.
		auto DealerSays = [this, &Hash, bSunday, HeroTable](const char* Call, const std::map<std::string, std::string>& Vars, int32 AtTable, float AfterSeconds) {
			if (AtTable < 0 || AtTable != HeroTable || bHeroUp)
			{
				return; // the player's dealer, to the player's table
			}
			ss::talk::Context C;
			C.Vars = Vars;
			C.Vars["Venue"] = "embercrest";
			if (bSunday)
			{
				C.Vars["Dealer"] = "Dee";
			}
			C.Salt = Hash(UTF8_TO_TCHAR(Call), Tourney->Tick);
			const ss::talk::Said S = ss::talk::Dealer(Call, C);
			if (S.Text.empty())
			{
				return;
			}
			const FString Line = Str(S.Text);
			if (AfterSeconds <= 0.0f)
			{
				Table->DealerLine(Line);
				return;
			}
			TWeakObjectPtr<ABackRoomTable> Tb = Table.Get();
			FTimerHandle Handle;
			GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [Tb, Line]() {
				if (ABackRoomTable* T = Tb.Get())
				{
					T->DealerLine(Line);
				}
			}), AfterSeconds, false);
		};
		const float AfterTheFloor = 4.0f; // the chime's beat and a sentence or two of the PA
		auto Applause = [this](float Level) {
			if (Table->GetAudio())
			{
				Table->GetAudio()->PlayEffect(ss::audio::Effect::Applause, Level);
			}
		};
		// What the floor reads off a registration: the player's name as they gave it, the room's people by name, a handle's
		// first name, or nothing (then the table and seat).
		auto Spoken = [this](const ss::TPlayer& P) -> std::string {
			if (P.IsHero)
			{
				return Save.IsValid() && Save->Person.Created ? Save->Person.FullName() : P.Name;
			}
			return ss::talk::KnownByName(P.Name) ? P.Name : ss::talk::FirstNameIn(P.Name);
		};
		auto ByName = [this](const std::string& Name) -> const ss::TPlayer* {
			for (const ss::TPlayer& P : Tourney->Players)
			{
				if (P.Name == Name)
				{
					return &P;
				}
			}
			return nullptr;
		};
		const int32 Paid = Tourney->PaidPlaces();
		switch (E.Type)
		{
		case ss::TEventType::Bust:
		{
			const int32 Idx = Tourney->PlayerIndex(E.Id);
			if (E.IsHero || Idx < 0)
			{
				break;
			}
			const ss::TPlayer& P = Tourney->Players[static_cast<size_t>(Idx)];
			if (E.TableId == HeroTable)
			{
				// At the player's table: the dealer sends a cash to the cage.
				if (E.PrizeCents > 0)
				{
					DealerSays("cage", {{"Place", ss::talk::Ordinal(E.Place)}, {"Amount", ss::talk::Dollars(E.PrizeCents)}}, E.TableId, 0.0f);
				}
			}
			else if (E.Place > 1 && E.Place <= Paid + 4 && E.Place != Paid + 1)
			{
				// Close to the money and in it (the bubble has its own line).
				std::map<std::string, std::string> V;
				V["Table"] = std::to_string(E.TableId);
				V["Seat"] = std::to_string(ss::live::SeatLabel(P.Seat));
				V["Place"] = ss::talk::Ordinal(E.Place);
				if (E.PrizeCents > 0)
				{
					V["Amount"] = ss::talk::Dollars((E.PrizeCents + 50) / 100 * 100);
				}
				FloorSays(E.PrizeCents > 0 ? "bust.paid" : "bust", V, true);
			}
			break;
		}
		case ss::TEventType::Level:
			// (A level that starts after a break was called with the break: "When we come back...".)
			if (!bOnBreak)
			{
				std::map<std::string, std::string> V;
				V["Sb"] = ss::talk::Count(E.Blinds.Sb);
				V["Bb"] = ss::talk::Count(E.Blinds.Bb);
				if (E.Blinds.Ante > 0)
				{
					V["Ante"] = ss::talk::Count(E.Blinds.Ante);
				}
				FloorSays(E.Blinds.Ante > 0 ? "level.ante" : "level", V, true);
				if (Hash(TEXT("blinds"), E.LevelNumber) % 2u == 0u)
				{
					DealerSays("blinds", {}, HeroTable, AfterTheFloor);
				}
			}
			break;
		case ss::TEventType::TableBroken:
			if (E.TableId != HeroTable && (Tourney->Tables.size() <= 4 || Hash(TEXT("breaking"), E.TableId) % 2u == 0u))
			{
				FloorSays("table.breaking", {{"Table", std::to_string(E.TableId)}}, false);
			}
			break;
		case ss::TEventType::Break:
		{
			// One announcement, the break and then the level it leads into (Floor keeps its parts in order; two calls would
			// put the chimeless second one first).
			const std::string Called = FloorText("break", {{"Minutes", std::to_string(FMath::RoundToInt(LiveBreakMinutes))}});
			const ss::Level& Next = Tourney->CurrentLevel();
			std::map<std::string, std::string> V;
			V["Sb"] = ss::talk::Count(Next.Sb);
			V["Bb"] = ss::talk::Count(Next.Bb);
			if (Next.Ante > 0)
			{
				V["Ante"] = ss::talk::Count(Next.Ante);
			}
			const std::string Back = FloorText(Next.Ante > 0 ? "break.next.ante" : "break.next", V);
			const std::string Both = Called.empty() ? Back : Back.empty() ? Called : Called + " " + Back;
			if (!Both.empty())
			{
				Floor(Str(Both), true);
			}
			DealerSays("break", {}, HeroTable, AfterTheFloor);
			break;
		}
		case ss::TEventType::HandForHand:
			FloorSays("h4h", {}, true);
			DealerSays("h4h", {}, HeroTable, AfterTheFloor);
			break;
		case ss::TEventType::Bubble:
		{
			const ss::TPlayer* B = ByName(E.Name);
			const std::string Name = B ? Spoken(*B) : std::string();
			if (!Name.empty())
			{
				FloorSays("bubble", {{"Name", Name}}, true);
			}
			else if (B)
			{
				FloorSays("bubble.seat", {{"Table", std::to_string(B->TableId)}, {"Seat", std::to_string(ss::live::SeatLabel(B->Seat))}}, true);
			}
			Applause(0.8f);
			break;
		}
		case ss::TEventType::FinalTable:
			FloorSays("final", {}, true);
			Applause(0.9f);
			break;
		case ss::TEventType::Finished:
		{
			if (E.Id != ss::HeroId && !bRailing)
			{
				break; // gone home: nobody hears it
			}
			const ss::TPlayer* W = ByName(E.Name);
			const std::string Name = W ? Spoken(*W) : E.Name;
			if (!Name.empty() || !W)
			{
				FloorSays("champion", {{"Short", Std(LiveShort)}, {"Name", Name}}, true);
			}
			else
			{
				// A handle with no first name in it: the table and seat, as the director says it.
				FloorSays("champion.seat", {{"Table", std::to_string(W->TableId)}, {"Seat", std::to_string(ss::live::SeatLabel(W->Seat))}}, true);
			}
			Applause(E.Id == ss::HeroId ? 1.0f : 0.9f);
			break;
		}
		default:
			break;
		}
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

// ------------------------------------------------------------------ the rail

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
	const std::vector<ss::TEvent> Round = Tourney->SimulateTick();
	for (const ss::TEvent& E : Round)
	{
		LiveEvent(E);
	}
	// The room hears the round as it plays (the floor, the far tables' calls, the rail).
	if (RoomHost.IsValid() && RoomDirector.IsValid())
	{
		RoomDirector->HeroUp = bHeroUp;
		// (The far tables' hands play out over the rail's round, not a seated hand's length: the next round replaces them.)
		RoomHost->HandSeconds = RailRoundSeconds;
		RoomFeed::Simulated(*RoomHost, *RoomDirector, RoomSound.Get(), *Tourney, Round, Minutes);
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
	// The director keeps up with every round too (what it's said is what's been said), and only the last round's cues
	// go out when the player's back (RoomFeed's catch-up).
	const bool bHeard = RoomHost.IsValid() && RoomDirector.IsValid();
	if (bHeard)
	{
		RoomDirector->HeroUp = bHeroUp;
		RoomFeed::BeginCatchUp(*RoomHost);
	}
	for (int32 Guard = 0; Guard < 4000 && !Tourney->bFinished && LiveDayStart + Tourney->ClockMinutes() < Until; ++Guard)
	{
		const std::vector<ss::TEvent> Round = Tourney->SimulateTick();
		if (bHeard)
		{
			RoomFeed::Simulated(*RoomHost, *RoomDirector, RoomSound.Get(), *Tourney, Round, LiveDayStart + Tourney->ClockMinutes());
		}
		for (const ss::TEvent& E : Round)
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
	if (bHeard)
	{
		RoomFeed::EndCatchUp(*RoomHost);
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

// ------------------------------------------------------------------ the floor's last call

void ABackRoomGameMode::LiveLastCall()
{
	if (bCancelled || !Tourney)
	{
		return;
	}
	// The banner and the boards; the floor's words are the director's (its Clock reads the tournament's own clock, so the
	// call comes in the same hand at any pace).
	if (!bLastCalled && Minutes >= LastCallAt(LiveLateRegEnds) && Minutes < LiveLateRegEnds)
	{
		bLastCalled = true;
		RaiseBanner(TEXT("LAST CALL FOR REGISTRATION"));
		UpdateRoomBoards();
	}
	if (!bRegClosed && Minutes >= LiveLateRegEnds)
	{
		// (On a level's turn: the level's own banner stays up.)
		bRegClosed = true;
		bLastCalled = true;
		UpdateRoomBoards();
	}
}

// ------------------------------------------------------------------ the director's host (ue-room, next_segment_spec 4.11)

namespace LiveDetail
{
/**
 * Where a one-shot sits in the listening gate's recording: a constant-power pan by its side of the listener and 1/r from
 * a metre (the engine's own spatializer plays it; this only lays it into the recorded stream beside the room).
 */
void PanFor(const FVector& Ear, const FVector& Forward, const FVector& At, float& OutL, float& OutR)
{
	const FVector To = At - Ear;
	const float Dist = FMath::Max(100.0f, static_cast<float>(To.Size2D()));
	const float Right = static_cast<float>(FVector::CrossProduct(Forward.GetSafeNormal2D(), To.GetSafeNormal2D()).Z);
	const float Angle = (FMath::Clamp(Right, -1.0f, 1.0f) + 1.0f) * 0.25f * UE_PI;
	const float Gain = 100.0f / Dist;
	OutL = FMath::Cos(Angle) * Gain;
	OutR = FMath::Sin(Angle) * Gain;
}

/** Where a speaker is against the view (Captions' Side): 0 in view, -1 off to the left, 1 to the right, 2 behind. */
int32 SideOf(const FVector& Eye, const FVector& Forward, const FVector& At)
{
	const FVector Dir = (At - Eye).GetSafeNormal2D();
	const FVector Fwd = Forward.GetSafeNormal2D();
	const float Cos = static_cast<float>(FVector::DotProduct(Fwd, Dir));
	if (Cos > 0.64f)
	{
		return 0;
	}
	if (Cos < -0.7f)
	{
		return 2;
	}
	return FVector::CrossProduct(Fwd, Dir).Z > 0.0 ? 1 : -1;
}
} // namespace LiveDetail

void ABackRoomGameMode::PlayCue(const ss::room::Cue& Cue)
{
	if (!RoomHost.IsValid() || !Table)
	{
		// No room host (Dee's game, practice, before the field is seated): the room as it always spoke.
		RecentCues.push_back(Cue);
		if (RecentCues.size() > 10)
		{
			RecentCues.erase(RecentCues.begin());
		}
		const FString Text = Str(Cue.Text);
		if (Text.IsEmpty() || !Table)
		{
			return;
		}
		switch (Cue.What)
		{
		case ss::room::Kind::Pa:
			LegacyFloor(*this, Text, Cue.Chime, Cue.Part);
			break;
		case ss::room::Kind::Call:
			if (Cue.Where == ss::room::From::HeroTable)
			{
				Table->DealerLine(Text);
			}
			break;
		case ss::room::Kind::Line:
			if (ABackRoomPlayer* A = Table->PlayerById(Str(Cue.Who)))
			{
				A->Say(Text);
			}
			else
			{
				Table->Announce(Str(Cue.Speaker), Text);
			}
			break;
		default:
			break;
		}
		return;
	}
	FRoomHost& H = *RoomHost;
	FRoomQueued Q;
	Q.Cue = Cue;
	Q.DueAt = H.Clock;
	if (StartsWith(Cue.Who, "actor:"))
	{
		// A line one of the room's people said themselves (ABackRoomPlayer::Say: a greeting, the dealer's host lines, a
		// read of the player's shaking hands): who they are and where, so it's heard from them like the director's lines.
		Q.bHost = true;
		const uint32 Uid = static_cast<uint32>(std::strtoul(Cue.Who.c_str() + 6, nullptr, 10));
		auto Is = [Uid](const ABackRoomPlayer* P) { return P && P->GetUniqueID() == Uid; };
		ABackRoomPlayer* A = nullptr;
		FString Id;
		bool bExtra = false;
		bool bRail = false;
		if (Is(Dealer))
		{
			A = Dealer;
		}
		for (const TPair<FString, TObjectPtr<ABackRoomPlayer>>& It : CastActors)
		{
			if (!A && Is(It.Value))
			{
				A = It.Value;
				Id = It.Key;
			}
		}
		for (ABackRoomPlayer* O : Table->GetOpponents())
		{
			if (!A && Is(O))
			{
				A = O;
				// Their tournament id (the caption names an anonymous entrant by the handle's first name or the seat, never
				// the handle itself: spec 4.5): whoever the table seats in this body.
				const int32 Here = Tourney && !Tourney->Hero().Busted ? Tourney->Hero().TableId : AnchorTableId();
				for (size_t K = 0; Tourney && Id.IsEmpty() && K < Tourney->Players.size(); ++K)
				{
					const ss::TPlayer& P = Tourney->Players[K];
					if (!P.IsHero && P.TableId == Here && Table->PlayerById(Str(P.Id)) == O)
					{
						Id = Str(P.Id);
					}
				}
			}
		}
		for (int32 I = 0; !A && I < Extras.Num(); ++I)
		{
			if (Is(Extras[I]))
			{
				A = Extras[I];
				bExtra = true;
				Id = ExtraWho.IsValidIndex(I) ? ExtraWho[I] : FString();
			}
		}
		for (ABackRoomPlayer* B : RailBackers)
		{
			if (!A && Is(B))
			{
				A = B;
				bRail = true;
			}
		}
		if (!A)
		{
			// Someone the room doesn't place (not at a table, the rail or the cast's): their words on screen, no voice.
			if (!Cue.Speaker.empty() && !Cue.Text.empty())
			{
				ss::ui::Caption Cap;
				Cap.Speaker = Cue.Speaker;
				Cap.Line = Cue.Text;
				Cap.At = FPlatformTime::Seconds();
				Cap.Hold = ss::ui::CaptionHold(Cap.Line, 0.0f);
				Cap.Clock = ss::net::TimeLabel(Minutes);
				Cap.Room = true;
				PushCaption(Cap);
			}
			return;
		}
		Q.Actor = A;
		Q.Cue.Who.clear();
		Q.Cue.Speaker.clear(); // (named below: the dealer, the field's caption name, or their persona's)
		const int32 Mine = Tourney && !Tourney->Hero().Busted ? Tourney->Hero().TableId : AnchorTableId();
		if (A == Dealer)
		{
			Q.Cue.What = ss::room::Kind::Call;
			Q.Cue.Where = ss::room::From::HeroTable;
			Q.Cue.Speaker = A->Persona.Name == TEXT("Dee") ? "Dee" : "Dealer";
			Q.Cue.TableId = Mine;
		}
		else
		{
			Q.Cue.What = ss::room::Kind::Line;
			Q.Cue.Where = bRail ? ss::room::From::Rail : (bExtra ? ss::room::From::Table : ss::room::From::HeroTable);
			Q.Cue.TableId = bExtra ? 0 : Mine;
			if (bRail)
			{
				Q.Cue.Who = Std(A->Persona.Name); // (a backer goes by name at the rail: heard from where they stand)
			}
		}
		const int32 Idx = Tourney && !Id.IsEmpty() ? Tourney->PlayerIndex(Std(Id)) : -1;
		if (Idx >= 0)
		{
			const ss::TPlayer& P = Tourney->Players[static_cast<size_t>(Idx)];
			Q.Cue.Who = P.Id;
			Q.Cue.Speaker = ss::talk::CaptionName(P.Id, P.Name, P.Seat);
			Q.Cue.TableId = P.TableId;
			Q.Cue.Seat = P.Seat;
		}
		if (Q.Cue.Speaker.empty())
		{
			Q.Cue.Speaker = Std(A->Persona.Name);
		}
		if (Q.Cue.Key.empty() || Q.Cue.Key == "host.line")
		{
			Q.Cue.Key = A == Dealer ? "host.dealer" : "host.line";
		}
		// The dealer's "Turn them up": an all-in and a call at the player's table with the board to come. The rail leans in
		// now, while the cards turn (Director::AllInCalled), once a hand.
		if (A == Dealer && RoomDirector.IsValid() && Tourney && H.TurnUpLines.count(Cue.Text) > 0 && Table->GetHandNumber() != H.LeanHand)
		{
			H.LeanHand = Table->GetHandNumber();
			RoomDirector->AllInCalled(*Tourney, 0, Table->IsHeroInHand());
			RoomFeed::Take(H, *RoomDirector);
		}
	}
	H.Incoming.push_back(MoveTemp(Q));
}

void ABackRoomGameMode::PushCaption(const ss::ui::Caption& Caption)
{
	Captions.push_back(Caption);
	NightLog.push_back(Caption);
	// The zone shows a few at once; the night log keeps the night (a long one is a few hundred lines).
	if (Captions.size() > 8)
	{
		Captions.erase(Captions.begin(), Captions.begin() + static_cast<std::ptrdiff_t>(Captions.size() - 8));
	}
	if (NightLog.size() > 600)
	{
		NightLog.erase(NightLog.begin(), NightLog.begin() + static_cast<std::ptrdiff_t>(NightLog.size() - 600));
	}
}

void ABackRoomGameMode::BuildDirector()
{
	if (!bLive || !Tourney || !Save.IsValid() || !Table)
	{
		return;
	}
	const ss::life::LiveEntry* E = ss::live::EntryFor(Save->Life, Std(LiveEntryId));
	const ss::live::Occurrence O = ss::live::FindOccurrence(Std(LiveEntryId));
	if (!E || !O.Valid())
	{
		UE_LOG(LogEmbercrest, Warning, TEXT("The room's director: no entry for %s, the room keeps its old voice"), *LiveEntryId);
		return;
	}
	RoomHost = MakeShared<FRoomHost>();
	FRoomHost& H = *RoomHost;
	// The floor calls the name on the registration (the person's; the screen name is RiverLine's), the table the first name.
	const ss::hero::Character& Me = Save->Person;
	const std::string First = Me.Created && !Me.FirstName.empty() ? Me.FirstName : Save->HeroName;
	RoomDirector = MakeShared<ss::room::Director>(*E, O, ss::live::SeedFor(*E, Save->HeroName) + ":room", First);
	RoomDirector->HeroFullName = Me.Created ? Me.FullName() : First;
	RoomDirector->BreakSeconds = BreakRealSeconds;
	// Only a night come back to from its checkpoint resumes: a fresh seat (a late one too) is welcomed by the first Clock
	// with where registration stands.
	if (bResumed)
	{
		RoomDirector->Resume(*Tourney, E->Room, Minutes);
	}
	const std::vector<ss::life::LiveVoice> Voices = E->Voices.empty() ? ss::room::VoicesFor(*E) : E->Voices;
	for (const ss::life::LiveVoice& V : Voices)
	{
		H.Ages.Add(StrOf(V.Name), V.Age);
	}
	RoomVoices::Set(Voices);
	for (const std::pair<std::string, std::string>& T : ss::talk::Inventory())
	{
		if (StartsWith(T.first, "dealer.turnup"))
		{
			H.TurnUpLines.insert(T.second);
		}
	}
	H.HeroTable = Tourney->Hero().Busted ? -1 : Tourney->Hero().TableId;
	H.HandSeconds = 45.0 / FMath::Max(1.0f, FoldedPace() / 1.9f);
	// An all-in called at the player's table, mid-hand: the rail leans in now, and a backer calls the player's name (once a hand).
	TWeakObjectPtr<ABackRoomGameMode> Self = this;
	BindAllInCalled(*Table, [Self](bool bHeroIn) {
		ABackRoomGameMode* GM = Self.Get();
		if (!GM || !GM->RoomHost.IsValid() || !GM->RoomDirector.IsValid() || !GM->Tourney || !GM->Table || GM->Table->GetHandNumber() == GM->RoomHost->LeanHand)
		{
			return;
		}
		GM->RoomHost->LeanHand = GM->Table->GetHandNumber();
		GM->RoomDirector->AllInCalled(*GM->Tourney, 0, bHeroIn);
		RoomFeed::Take(*GM->RoomHost, *GM->RoomDirector);
	});
	// The stereo room, streamed in place of the old mono bed once the table starts its room tone (StartRoomTone).
	RoomSound = MakeShared<FCardRoomSound>(FCrc::StrCrc32(*(LiveEntryId + TEXT(":room"))), RoomTalkers());
	H.Talkers = RoomSound->GetVoiceBudget();
	VoiceBank = MakeShared<FVoiceBank>();
	// (TickRoom starts it as the player walks in, so the slot floor's doorway is behind them as they come through it; the
	// table's StartRoomTone then finds it playing.)
	TWeakPtr<FCardRoomSound> Weak = RoomSound;
	TWeakPtr<FRoomHost> WeakHost = RoomHost;
	Table->AmbienceSource = [Weak, WeakHost](UNightOneAudio& Audio) {
		TSharedPtr<FCardRoomSound> S = Weak.Pin();
		TSharedPtr<FRoomHost> Host = WeakHost.Pin();
		if (S && !(Host && Host->bStreaming))
		{
			RoomStream::Room(Audio, S);
			if (Host)
			{
				Host->bStreaming = true;
			}
		}
	};
	UE_LOG(LogEmbercrest, Log, TEXT("The room's director: %s, %d voices, talkers %d%s"), *LiveName, static_cast<int32>(Voices.size()), H.Talkers,
		bResumed ? TEXT(", resumed from the checkpoint") : TEXT(""));
}

void ABackRoomGameMode::TickRoom(float RealDt)
{
	if (!RoomHost.IsValid() || !RoomDirector.IsValid() || !Tourney || !Table)
	{
		return;
	}
	const uint64 C0 = FPlatformTime::Cycles64();
	FRoomHost& H = *RoomHost;
	FCardRoomSound* Sound = RoomSound.Get();
	UNightOneAudio* Audio = Table->GetAudio();
	const double Now = FPlatformTime::Seconds();
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	FVector CamLoc = Stage ? Stage->EyeLocation() : FVector::ZeroVector;
	FRotator CamRot = FRotator::ZeroRotator;
	if (PC && PC->PlayerCameraManager)
	{
		CamLoc = PC->PlayerCameraManager->GetCameraLocation();
		CamRot = PC->PlayerCameraManager->GetCameraRotation();
	}
	const FVector CamFwd = CamRot.Vector();
	const FTransform RoomWorld = Stage && Stage->GetRoomRoot() ? Stage->GetRoomRoot()->GetComponentTransform() : FTransform::Identity;
	const int32 Mine = !Tourney->Hero().Busted ? Tourney->Hero().TableId : -1;
	const int32 HereId = Mine >= 0 ? Mine : AnchorTableId();

	// The cue clock (spec 4.9): real seconds at the room's pace, Focus's dilation and a folded hand's hurry clamped to 0.45..1.5.
	const float Pace = FMath::Clamp(static_cast<float>(UGameplayStatics::GetGlobalTimeDilation(this)), 0.45f, 1.5f);
	H.Clock += static_cast<double>(RealDt) * Pace;

	// The player's settings: how much the table talks; the room's talkers (0, the fallback the user chose, unless the dev
	// flag ss.Room.Talkers says otherwise).
	if (const ss::ui::GameSettings* S = GetSettings())
	{
		H.TalkScale = S->TableTalk == 0 ? 0.5f : (S->TableTalk == 2 ? 1.5f : 1.0f);
	}
	RoomDirector->TalkScale = H.TalkScale;
	RoomDirector->HeroUp = bHeroUp;
	const int32 Talkers = RoomTalkers();
	if (Sound && Talkers != Sound->GetVoiceBudget())
	{
		Sound->SetVoiceBudget(Talkers);
		UE_LOG(LogEmbercrest, Log, TEXT("The room's talkers: %d%s"), Talkers,
			Talkers == 0 ? TEXT(" (what ships: walla and vocals, the near lines a breath)") : TEXT(" (DEV: the formant talkers the user turned down)"));
	}
	H.Talkers = Talkers;
	// The fallback (spec 11): no formant voice at the player's table and no lip sync from the synthesized plan.
	const bool bFallback = Talkers == 0;

	// The table's news, in the director's order: what started (the balance), the hand that finished, its rounds, what
	// finished (busts, levels, the bubble...).
	{
		ss::room::HandNote Note;
		const bool bNote = Table->TakeHandNote(Note);
		const std::vector<ss::TableRound> Rounds = Table->TakeRounds();
		const int32 HandNo = Table->GetHandNumber();
		const bool bNewHand = HandNo != H.LastHandNumber && !Table->IsBetweenHands();
		if (bNewHand)
		{
			H.LastHandNumber = HandNo;
		}
		if (bNote || !Rounds.empty())
		{
			// A hand here in real seconds: the engine's hands at the other tables play out over the next one.
			if (H.LastHandEndAt > 0.0)
			{
				H.HandSeconds = FMath::Clamp(0.7 * H.HandSeconds + 0.3 * (Now - H.LastHandEndAt), 8.0, 60.0);
			}
			H.LastHandEndAt = Now;
		}
		if (!H.TableEvents.empty() || bNote || !Rounds.empty() || bNewHand)
		{
			RoomFeed::Table(H, *RoomDirector, Sound, *Tourney, H.TableEvents, bNote ? &Note : nullptr, Rounds, bNewHand, Minutes);
		}
		H.TableEvents.clear();
	}
	// The clock: the desk opening, ten minutes, cards in the air (or where registration stands for a late seat), last call,
	// the close, and the break's own clock (the five-minute call, then the break over).
	RoomDirector->Clock(*Tourney, Minutes, bOnBreak ? FMath::Max(0.05, static_cast<double>(BreakRealSeconds - BreakT)) : 0.0);
	RoomFeed::Take(H, *RoomDirector);
	// The rail: who holds a piece of the player and is out of tonight's field.
	if (Now >= H.NextRailLook)
	{
		H.NextRailLook = Now + 1.5;
		PlaceBackers();
	}

	// ---- the cues: each one's speaker found and its speech made on a worker as it arrives, then played at its time.
	auto RailBody = [this](const std::string& Name) -> ABackRoomPlayer* {
		for (ABackRoomPlayer* B : RailBackers)
		{
			if (B && !B->IsHidden() && Std(B->Persona.Name) == Name)
			{
				return B;
			}
		}
		return nullptr;
	};
	auto AtThisTable = [this](const ABackRoomPlayer* A) { return A && (A == Dealer || Table->GetOpponents().Contains(A)); };
	auto TimbreOf = [this, &H](const ss::room::Cue& C, ABackRoomPlayer* A) -> ss::audio::Timbre {
		if (C.What == ss::room::Kind::Pa)
		{
			// The floor's announcer, the same voice every night; Dee on the mic on Sundays.
			return C.Speaker == "Dee" ? ss::audio::TimbreFor(true, 52, FCrc::StrCrc32(TEXT("Dee"))) : ss::audio::PaTimbre(0xf1007u);
		}
		FString Body;
		FString Name = StrOf(C.Speaker);
		int32 Age = 0;
		if (C.Speaker == "Dee")
		{
			Body = TEXT("Dee");
			Name = TEXT("Dee");
			Age = 52;
		}
		else if (A)
		{
			Body = WornBody(A);
			Name = A->Persona.Name;
			Age = A == Dealer ? 44 : 0;
		}
		else if (C.What == ss::room::Kind::Call)
		{
			// Another table's dealer: the body PlaceExtras dresses in the house's black there.
			const int32 Slot = SlotForTable(C.TableId);
			Body = ExtraBodies[((Slot * 7 + 3) % 12 + 12) % 12];
			Name = FString::Printf(TEXT("dealer%d"), Slot);
			Age = 28 + (Slot * 7) % 27;
		}
		else
		{
			const int32 Idx = Tourney->PlayerIndex(C.Who);
			const FString Id = Idx >= 0 ? StrOf(C.Who) : TEXT("rail:") + Name;
			if (Idx >= 0)
			{
				Name = StrOf(Tourney->Players[static_cast<size_t>(Idx)].Name);
			}
			const FString* Kept = LiveBodies.Find(Id);
			Body = Kept ? *Kept : DefaultBody(Id, Name);
		}
		if (Age == 0)
		{
			const int32* Known = H.Ages.Find(Name);
			Age = Known ? *Known : ss::room::VoiceForStranger(Std(Name), 0).Age;
		}
		return ss::audio::TimbreFor(CastLooks::IsWomansFace(FName(*Body)), Age, FCrc::StrCrc32(*Name));
	};
	const float NearDb = CVarRoomNearDb.GetValueOnGameThread();
	const float DealerDb = CVarRoomDealerDb.GetValueOnGameThread();
	const bool bAtStage = Stage && Stage->GetRoomAnchor() == ABackRoomStage::StreamSlot();
	// Moved (a table broken, the balance, a move while up): what the old table's people still had to say isn't said at the
	// player's table any more. Their lines go out in the room from that table, and what was said to the player (a needle,
	// a remark on the patch) isn't said at all: they've gone (acceptance F16). The old dealer's "you're moving" is a call,
	// said there on purpose.
	if (Mine > 0)
	{
		for (std::vector<FRoomQueued>* List : {&H.Incoming, &H.Queue})
		{
			for (size_t I = List->size(); I-- > 0;)
			{
				FRoomQueued& Q = (*List)[I];
				if (Q.Cue.What != ss::room::Kind::Line || Q.Cue.Where != ss::room::From::HeroTable || Q.Cue.TableId <= 0 || Q.Cue.TableId == Mine)
				{
					continue;
				}
				if (Q.Cue.ToHero)
				{
					List->erase(List->begin() + static_cast<std::ptrdiff_t>(I));
					continue;
				}
				Q.Cue.Where = ss::room::From::Table;
				Q.bNear = false;
			}
		}
	}
	for (FRoomQueued& Q : H.Incoming)
	{
		const ss::room::Cue& C = Q.Cue;
		const bool bTalk = C.What == ss::room::Kind::Line || C.What == ss::room::Kind::Call;
		const bool bSpeech = (bTalk || C.What == ss::room::Kind::Pa) && !C.Text.empty() &&
			!(C.What == ss::room::Kind::Line && C.Speaker.empty()); // (narration: words on screen, no voice)
		if (bSpeech && bTalk)
		{
			// Who says it, and whether it's said at the player's table (a one-shot at their head) or out in the room.
			ABackRoomPlayer* A = Q.Actor.Get();
			const bool bHeroSide = C.Where == ss::room::From::HeroTable || (C.Where == ss::room::From::FeatureTable && bAtStage);
			if (!A && C.What == ss::room::Kind::Line && bHeroSide)
			{
				A = Table->PlayerById(StrOf(C.Who));
			}
			if (!A && C.What == ss::room::Kind::Call && bHeroSide)
			{
				A = Dealer;
			}
			if (!A && C.What == ss::room::Kind::Line && C.Where == ss::room::From::Rail)
			{
				A = RailBody(C.Who);
			}
			if (!A && C.What == ss::room::Kind::Line && C.Where == ss::room::From::Table)
			{
				for (int32 I = 0; I < Extras.Num() && !A; ++I)
				{
					A = Extras[I] && ExtraWho.IsValidIndex(I) && Std(ExtraWho[I]) == C.Who ? Extras[I].Get() : nullptr;
				}
			}
			Q.Actor = A;
			Q.bNear = AtThisTable(A) && C.Where != ss::room::From::Rail;
		}
		if (bSpeech && !(VoiceBank.IsValid() && VoiceBank->Find(C.Key, C.Part, C.Speaker)))
		{
			ABackRoomPlayer* A = Q.Actor.Get();
			TSharedPtr<FRoomSpeech, ESPMode::ThreadSafe> S = MakeShared<FRoomSpeech, ESPMode::ThreadSafe>();
			S->Text = C.Text;
			S->Voice = TimbreOf(C, A);
			S->Effort = C.What == ss::room::Kind::Pa ? (C.Priority >= 3 ? 0.85f : 0.7f) : FMath::Clamp(C.Effort, 0.3f, 1.0f);
			S->Seed = FCrc::StrCrc32(*StrOf(C.Key + "|" + C.Text + "|" + std::to_string(C.Tick) + "|" + std::to_string(C.Part)));
			S->bMurmur = C.What == ss::room::Kind::Line;
			S->bRender = Q.bNear;
			// What ships (spec 11, the fallback): a person's line anywhere and anything said at the player's table, the
			// dealer's calls included, is at most a soft breath under its caption; the far dealers' calls and the PA keep
			// their voices, as in the clip the user chose. The jaw follows the plan only with the dev flag's voices.
			S->bBreath = bFallback && C.What != ss::room::Kind::Pa && (C.What == ss::room::Kind::Line || Q.bNear);
			S->bLipSync = !bFallback;
			S->TargetDb = S->bBreath ? NearDb - 9.0f : (C.What == ss::room::Kind::Call ? DealerDb : NearDb);
			Q.Speech = S;
			UE::Tasks::Launch(UE_SOURCE_LOCATION, [S]() { S->Run(); });
		}
		H.Queue.push_back(MoveTemp(Q));
	}
	if (!H.Incoming.empty())
	{
		H.Incoming.clear();
		std::stable_sort(H.Queue.begin(), H.Queue.end(), [](const FRoomQueued& A, const FRoomQueued& B) { return A.DueAt < B.DueAt; });
	}

	// What the room hands to the stream is heard after the ring and the stream's queue: a one-shot that should line up with
	// it (a near line and its duck, the chime before the PA) waits that long, and so does every caption.
	const double L = FMath::Clamp((Audio ? Audio->GetAmbienceQueuedBytes() / 192000.0 : 0.0) + (Sound ? Sound->RingSeconds() : 0.0), 0.0, 1.0);
	H.LastLatency = L;
	// The room's clock for the night log, made only on a frame that captions something (no per-frame strings).
	std::string ClockText;
	auto ClockNow = [this, &ClockText]() -> const std::string& {
		if (ClockText.empty())
		{
			ClockText = ss::net::TimeLabel(Minutes);
		}
		return ClockText;
	};
	auto CaptionAt = [this, &H, &ClockNow](double At, const ss::room::Cue& C, double Seconds, bool bRoom, int32 Side) {
		ss::ui::Caption Cap;
		Cap.Speaker = C.What == ss::room::Kind::Pa ? (C.Speaker == "Dee" ? std::string("Dee \xC2\xB7 PA") : std::string("Floor")) : C.Speaker;
		Cap.Line = C.What == ss::room::Kind::Vocal ? C.Sound : C.Text;
		Cap.Style = C.What == ss::room::Kind::Pa ? 1 : (C.What == ss::room::Kind::Vocal ? 2 : (C.ToHero ? 3 : 0));
		Cap.Side = Side;
		Cap.Hold = ss::ui::CaptionHold(Cap.Line, static_cast<float>(Seconds));
		Cap.Priority = C.What == ss::room::Kind::Pa ? FMath::Max(C.Priority, 2) : C.Priority;
		Cap.Room = bRoom;
		if (Cap.Line.empty())
		{
			return;
		}
		Cap.Clock = ClockNow();
		H.Timeline.Add({At, [this, Cap]() mutable {
			Cap.At = FPlatformTime::Seconds();
			PushCaption(Cap);
		}});
	};
	auto RoomSpot = [this, &H, &RoomWorld, HereId](const ss::room::Cue& C, FCardRoomSound::Spot& OutSpot, int32& OutId, FVector& OutWorld) {
		using ss::room::From;
		FVector2D Place = RailEdgeAt;
		OutId = 0;
		switch (C.Where)
		{
		case From::Pa: OutSpot = FCardRoomSound::Spot::Pa; break;
		case From::Rail:
			OutSpot = FCardRoomSound::Spot::Rail;
			if (H.bRailPlaced && std::any_of(H.RailSet.begin(), H.RailSet.end(), [&C](const std::pair<std::string, int>& R) { return R.first == C.Who; }))
			{
				OutId = 1;
				Place = FVector2D(H.RailAt);
			}
			break;
		case From::Bar: OutSpot = FCardRoomSound::Spot::Bar; Place = BarAt; break;
		case From::Desk:
			// The near wall: the tournament desk, or the cashier's window down it.
			OutSpot = C.Speaker == "Cashier" ? FCardRoomSound::Spot::Cage : FCardRoomSound::Spot::Desk;
			Place = C.Speaker == "Cashier" ? CageAt : DeskAt;
			break;
		case From::FeatureTable:
			OutSpot = FCardRoomSound::Spot::FeatureTable;
			OutId = C.TableId;
			Place = FVector2D(ABackRoomStage::TableSlotLocal(ABackRoomStage::StreamSlot()).GetLocation());
			break;
		default:
		{
			OutSpot = FCardRoomSound::Spot::Table;
			OutId = C.Where == From::HeroTable || C.TableId <= 0 ? HereId : C.TableId;
			Place = FVector2D(ABackRoomStage::TableSlotLocal(SlotForTable(OutId)).GetLocation());
			break;
		}
		}
		OutWorld = RoomWorld.TransformPosition(FVector(Place.X, Place.Y, 120.0));
	};
	auto Fire = [&](FRoomQueued& Q) {
		using ss::room::Kind;
		using ss::room::From;
		const ss::room::Cue& C = Q.Cue;
		RecentCues.push_back(C);
		if (RecentCues.size() > 10)
		{
			RecentCues.erase(RecentCues.begin());
		}
		TSharedPtr<FRoomSpeech, ESPMode::ThreadSafe> S = Q.Speech;
		const double Secs = S.IsValid() ? S->Plan.Seconds : 0.0;
		USoundWave* Clip = VoiceBank.IsValid() && C.What != Kind::Vocal && C.What != Kind::Phone ? VoiceBank->Find(C.Key, C.Part, C.Speaker) : nullptr;
		const double ClipSecs = Clip ? static_cast<double>(Clip->Duration) : 0.0;
		const double Heard = FMath::Max(Secs, ClipSecs);
		const FString Text = StrOf(C.Text);
		const bool bNeedle = C.ToHero && StartsWith(C.Key, "field.needle");
		TWeakObjectPtr<ABackRoomTable> Tb = Table.Get();
		auto Needle = [Tb]() {
			if (ABackRoomTable* T = Tb.Get())
			{
				// A needle lands (spec 11 Q18): a little tilt, never a decision made for the player.
				T->Composure.Tilt = FMath::Min(1.0f, T->Composure.Tilt + 0.04f);
			}
		};
		switch (C.What)
		{
		case Kind::Pa:
		{
			const double SpeakAt = Now + (C.Chime ? 1.1 : 0.0);
			if (C.Chime && Audio)
			{
				const std::vector<float> Chime = ss::audio::Render(ss::audio::Effect::Chime, FCrc::StrCrc32(*StrOf(C.Key + std::to_string(C.Tick))));
				H.Timeline.Add({Now + L, [this, Chime]() {
					if (UNightOneAudio* A = Table ? Table->GetAudio() : nullptr)
					{
						A->PlaySamples(Chime, 0.55f);
						if (FCardRoomSound* Sn = RoomSound.Get(); Sn && Sn->IsRecording())
						{
							std::vector<float> Mono = Chime;
							Sn->RecordOverlay(Mono, 0.55f * 0.7f, 0.55f * 0.7f, A->GetAmbienceQueuedBytes() / 4);
						}
					}
				}});
			}
			if (Clip)
			{
				TWeakObjectPtr<USoundWave> Wave = Clip;
				H.Timeline.Add({SpeakAt + L, [this, Wave]() {
					UNightOneAudio* A = Table ? Table->GetAudio() : nullptr;
					if (USoundWave* W = Wave.Get())
					{
						UGameplayStatics::PlaySound2D(this, W, A ? A->MasterVolume * A->GetVoicesVolume() : 1.0f);
					}
				}});
			}
			else if (Sound && S.IsValid())
			{
				// Through the PA chain in the room (ceiling speakers, no direction): it ducks the room's talk itself.
				TSharedPtr<FCardRoomSound> Room = RoomSound;
				H.Timeline.Add({SpeakAt, [Room, S]() {
					if (Room.IsValid())
					{
						Room->Say(S->Plan, S->Voice, FCardRoomSound::Spot::Pa, 0, S->Effort, false);
					}
				}});
			}
			CaptionAt(SpeakAt + L, C, Heard, false, 0);
			// The legacy HUD's subtitle, with its sound.
			const FString Who = C.Speaker == "Dee" ? FString(TEXT("Dee")) : FString(TEXT("Floor"));
			H.Timeline.Add({SpeakAt + L, [Tb, Who, Text]() {
				if (ABackRoomTable* T = Tb.Get())
				{
					T->Announce(Who, Text);
				}
			}});
			H.PaFreeAt = SpeakAt + Heard + 0.25;
			break;
		}
		case Kind::Call:
		case Kind::Line:
		{
			if (C.What == Kind::Line && C.Speaker.empty())
			{
				// Narration (what the player sees and feels, a host's line with nobody saying it): on screen, dimmed, no voice.
				ss::room::Cue Said = C;
				Said.What = Kind::Vocal;
				Said.Sound = C.Text;
				CaptionAt(Now, Said, 0.0, false, 0);
				break;
			}
			ABackRoomPlayer* A = Q.Actor.Get();
			if (A && Q.bNear)
			{
				// At the player's table: one engine voice at the speaker's head (two at most), the room ducked under it,
				// held by the stream's latency so the duck and the voice land together. What ships is the fallback's soft
				// breath (FRoomSpeech::bBreath); the jaw follows the syllables only with the dev flag's voices.
				H.NearEnds.RemoveAll([Now](double End) { return End <= Now; });
				const bool bVoice = Audio && S.IsValid() && !S->Samples.empty() && H.NearEnds.Num() < 2 && !Clip;
				if (bVoice || Clip)
				{
					H.NearEnds.Add(Now + L + Heard);
					if (Sound)
					{
						Sound->Duck(6.0f, static_cast<float>(Heard));
					}
				}
				else if (Sound && S.IsValid())
				{
					// Two near voices already playing (spec 10's engine voices): this one from the table, in the room's stream
					// (it ducks the room for itself; the fallback's breath goes as a murmur, which the room breathes).
					Sound->Say(S->Plan, S->Voice, FCardRoomSound::Spot::Table, HereId, S->Effort, S->bMurmur || S->bBreath);
				}
				const bool bDealer = A == Dealer;
				const bool bHostLine = Q.bHost;
				const FString DealerName = C.Speaker == "Dee" ? FString(TEXT("Dee")) : (Dealer ? Dealer->Persona.Name : FString(TEXT("Dealer")));
				TWeakObjectPtr<ABackRoomPlayer> Who = A;
				TWeakObjectPtr<USoundWave> Wave = Clip;
				H.Timeline.Add({Now + L, [this, Who, S, Text, bVoice, bDealer, bHostLine, DealerName, Wave, bNeedle, Needle]() {
					ABackRoomPlayer* P = Who.Get();
					if (!P || !Table)
					{
						return;
					}
					const FVector Mouth = P->GetEyes() - FVector(0.0, 0.0, 9.0);
					UNightOneAudio* Au = Table->GetAudio();
					if (USoundWave* W = Wave.Get())
					{
						UGameplayStatics::PlaySoundAtLocation(this, W, Mouth, Au ? Au->MasterVolume * Au->GetVoicesVolume() : 1.0f);
					}
					else if (bVoice && Au)
					{
						Au->PlayVoice(S->Samples, 1.0f, &Mouth);
						FCardRoomSound* Sn = RoomSound.Get();
						APlayerController* PC2 = GetWorld()->GetFirstPlayerController();
						if (Sn && Sn->IsRecording() && PC2 && PC2->PlayerCameraManager)
						{
							float GL = 0.7f, GR = 0.7f;
							PanFor(PC2->PlayerCameraManager->GetCameraLocation(), PC2->PlayerCameraManager->GetCameraRotation().Vector(), Mouth, GL, GR);
							const float V = Au->GetVoicesVolume() / FMath::Max(0.01f, Au->GetAmbienceVolume());
							Sn->RecordOverlay(S->Samples, GL * V, GR * V, Au->GetAmbienceQueuedBytes() / 4);
						}
					}
					// The mouth: on its talk timer (the fallback), or on the syllables (MouthAt, the dev flag's voices only); the
					// legacy HUD's subtitle comes with it.
					P->Say(Text, S.IsValid() && S->bLipSync ? &S->Plan : nullptr);
					if (bDealer || !Table->GetOpponents().Contains(P))
					{
						P->TakeLines(); // (nobody drains theirs: the dealer's host lines are captioned by DealerLine already)
					}
					if (bDealer && !bHostLine)
					{
						Table->Announce(DealerName, Text);
					}
					if (bNeedle)
					{
						Needle();
					}
				}});
				const bool bRoomLine = C.Where != From::HeroTable && !C.ToHero && !(C.Where == From::FeatureTable && bAtStage);
				CaptionAt(Now + L, C, Heard, bRoomLine, SideOf(CamLoc, CamFwd, A->GetEyes()));
				H.TableFreeAt = Now + Heard + 0.25;
				break;
			}
			// Another table, the rail, the bar: in the room's stream, where it was said.
			FCardRoomSound::Spot Where = FCardRoomSound::Spot::Table;
			int32 Id = 0;
			FVector At = FVector::ZeroVector;
			RoomSpot(C, Where, Id, At);
			const bool bCall = C.What == Kind::Call;
			if (bCall && C.Effort < ShoutEffort && Where != FCardRoomSound::Spot::Pa && FVector::Dist2D(RoomWorld.InverseTransformPosition(At), RoomWorld.InverseTransformPosition(CamLoc)) > QuietCallReach)
			{
				break; // a quiet call too far away to hear
			}
			if (Clip)
			{
				// The VoiceBank's clip (P2), from where it was said, with its caption.
				TWeakObjectPtr<USoundWave> Wave = Clip;
				H.Timeline.Add({Now + L, [this, Wave, At]() {
					UNightOneAudio* Au = Table ? Table->GetAudio() : nullptr;
					if (USoundWave* W = Wave.Get())
					{
						UGameplayStatics::PlaySoundAtLocation(this, W, At, Au ? Au->MasterVolume * Au->GetVoicesVolume() : 1.0f);
					}
				}});
			}
			else if (Sound && S.IsValid())
			{
				// (A person's line in the fallback goes as a murmur, which the room at budget 0 says as a soft breath.)
				const bool bMurmur = S->bBreath || (!bCall && (Where == FCardRoomSound::Spot::Table || Where == FCardRoomSound::Spot::FeatureTable));
				Sound->Say(S->Plan, S->Voice, Where, Id, C.What == Kind::Line && C.Where == From::Rail ? FMath::Max(0.7f, C.Effort) : S->Effort, bMurmur);
			}
			if (ABackRoomPlayer* Body = Q.Actor.Get())
			{
				// An extra at a near table, or a backer at the rail: the mouth with it (its talk timer in the fallback).
				TWeakObjectPtr<ABackRoomPlayer> Who = Body;
				const bool bLipSync = S.IsValid() && S->bLipSync;
				H.Timeline.Add({Now + L, [Who, S, Text, bLipSync]() {
					if (ABackRoomPlayer* P = Who.Get())
					{
						P->Say(Text, bLipSync ? &S->Plan : nullptr);
						P->TakeLines();
					}
				}});
			}
			if (bNeedle)
			{
				H.Timeline.Add({Now + L, Needle});
			}
			CaptionAt(Now + L, C, Heard, !C.ToHero, SideOf(CamLoc, CamFwd, At));
			break;
		}
		case Kind::Vocal:
		{
			FCardRoomSound::Spot Where = FCardRoomSound::Spot::Pa;
			int32 Id = 0;
			FVector At = FVector::ZeroVector;
			RoomSpot(C, Where, Id, At);
			if (Sound)
			{
				Sound->Crowd(C.Noise, C.Voices, Where, Id, FMath::Clamp(C.Effort, 0.0f, 1.0f));
			}
			CaptionAt(Now + L, C, 2.0, true, C.Where == From::Pa ? 0 : SideOf(CamLoc, CamFwd, At));
			break;
		}
		case Kind::Phone:
			// A text: the toast lane (held while it's the player's decision) and the phone's buzz.
			PushToast(StrOf(C.Speaker), Text);
			if (Audio)
			{
				Audio->PlayEffect(ss::audio::Effect::Buzz, 0.6f);
			}
			break;
		}
	};
	bool bPaBlocked = false;
	bool bTableBlocked = false;
	for (size_t I = 0; I < H.Queue.size();)
	{
		FRoomQueued& Q = H.Queue[I];
		if (Q.DueAt > H.Clock)
		{
			break; // sorted: the rest are later
		}
		const ss::room::Cue& C = Q.Cue;
		const bool bPa = C.What == ss::room::Kind::Pa;
		const bool bNear = Q.bNear && Q.Actor.IsValid();
		// Long overdue (a stall, a hurried hand): the moment's gone. The floor's milestones are never dropped.
		if (H.Clock - Q.DueAt > 8.0 && C.Priority < 2 && !bPa)
		{
			H.Queue.erase(H.Queue.begin() + static_cast<std::ptrdiff_t>(I));
			continue;
		}
		if ((bPa && bPaBlocked) || (bNear && bTableBlocked) || (Q.Speech.IsValid() && !Q.Speech->bDone))
		{
			// Its channel is busy, or its speech isn't made yet (the line starts when it is; its caption waits for it).
			bPaBlocked = bPaBlocked || bPa;
			bTableBlocked = bTableBlocked || bNear;
			++I;
			continue;
		}
		if (bPa && Now < H.PaFreeAt)
		{
			bPaBlocked = true;
			++I;
			continue;
		}
		if (bNear && Now < H.TableFreeAt - 0.15)
		{
			if (StartsWith(C.Key, "field.small") && H.TableFreeAt - Now > 1.5)
			{
				// Small talk that would talk over a line still being said (a quicker pace says the same lines in less time):
				// the night log keeps it; the room doesn't voice it (spec 4.5).
				ss::ui::Caption Cap;
				Cap.Speaker = C.Speaker;
				Cap.Line = C.Text;
				Cap.Clock = ClockNow();
				Cap.At = FPlatformTime::Seconds() - 30.0;
				NightLog.push_back(Cap);
				++H.Skipped;
				H.Queue.erase(H.Queue.begin() + static_cast<std::ptrdiff_t>(I));
				continue;
			}
			bTableBlocked = true;
			++I;
			continue;
		}
		Fire(Q);
		if (Q.Speech.IsValid())
		{
			H.SpeechMsSum += Q.Speech->Ms;
			H.SpeechMsPeak = FMath::Max(H.SpeechMsPeak, Q.Speech->Ms);
			++H.SpeechJobs;
		}
		H.Queue.erase(H.Queue.begin() + static_cast<std::ptrdiff_t>(I));
	}
	// What's due now: the held one-shots, the jaws, the captions with their sounds.
	for (int32 I = 0; I < H.Timeline.Num();)
	{
		if (H.Timeline[I].At <= Now)
		{
			TFunction<void()> Do = MoveTemp(H.Timeline[I].Do);
			H.Timeline.RemoveAt(I);
			if (Do)
			{
				Do();
			}
		}
		else
		{
			++I;
		}
	}
	// Captions that have had their time (the HUD fades them; the night log keeps them).
	const double CapNow = FPlatformTime::Seconds();
	Captions.erase(std::remove_if(Captions.begin(), Captions.end(),
					   [CapNow](const ss::ui::Caption& Cap) { return Cap.At + FMath::Max(2.6, static_cast<double>(Cap.Hold)) + 1.0 < CapNow; }),
		Captions.end());

	// ---- the room as heard from here: the listener, the places, the night's mix, the muffles, the pace.
	if (Sound && Audio && !H.bStreaming && Table->bCardRoomTone)
	{
		// The stream starts with the walk in (the table's own StartRoomTone, when it deals, finds it playing).
		RoomStream::Room(*Audio, RoomSound);
		H.bStreaming = true;
	}
	if (Sound && Stage)
	{
		const FVector Ear = RoomWorld.InverseTransformPosition(CamLoc);
		const FVector Fwd = RoomWorld.InverseTransformVectorNoScale(CamFwd);
		const float EarYaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Fwd.Y, Fwd.X)));
		if (FMath::Abs(static_cast<float>(Ear.X) - H.LastListener[0]) > 0.5f || FMath::Abs(static_cast<float>(Ear.Y) - H.LastListener[1]) > 0.5f ||
			FMath::Abs(FMath::FindDeltaAngleDegrees(H.LastListener[2], EarYaw)) > 0.25f)
		{
			// (Queued only when the head moves: a still listener costs the room nothing.)
			Sound->SetListener(static_cast<float>(Ear.X), static_cast<float>(Ear.Y), EarYaw);
			H.LastListener[0] = static_cast<float>(Ear.X);
			H.LastListener[1] = static_cast<float>(Ear.Y);
			H.LastListener[2] = EarYaw;
		}
		// (Built into the host's scratch each frame: no allocation once it has its size.)
		std::vector<FCardRoomSound::Emitter>& Em = H.EmitterScratch;
		Em.clear();
		const bool bFinal = Tourney->Tables.size() == 1;
		auto Seated = [this](int32 Id) {
			int32 N = 0;
			const auto It = Tourney->Tables.find(Id);
			if (It != Tourney->Tables.end())
			{
				for (const int32 Idx : It->second.Seats)
				{
					N += Idx >= 0 ? 1 : 0;
				}
			}
			return N;
		};
		auto Add = [&Em](FCardRoomSound::Spot Kind, int32 Id, double X, double Y, bool bOpen, int32 Players) {
			FCardRoomSound::Emitter E;
			E.Kind = Kind;
			E.Id = Id;
			E.X = static_cast<float>(X);
			E.Y = static_cast<float>(Y);
			E.Open = bOpen;
			E.Players = Players;
			Em.push_back(E);
		};
		for (int32 Slot = 0; Slot < ABackRoomStage::StreamSlot(); ++Slot)
		{
			// Table N stands at slot N - 1 (SlotForTable); the player's own is the table actor's to sound (Players 0).
			const int32 Id = Slot + 1;
			const FVector P = ABackRoomStage::TableSlotLocal(Slot).GetLocation();
			const bool bOpen = !bFinal && Tourney->Tables.count(Id) > 0;
			// (A closed slot answers to no table id: a line or a call for the final table finds the feature table, not its old slot.)
			Add(FCardRoomSound::Spot::Table, bOpen ? Id : -Id, P.X, P.Y, bOpen, Id == Mine ? 0 : (bOpen ? Seated(Id) : 0));
		}
		{
			const FVector P = ABackRoomStage::TableSlotLocal(ABackRoomStage::StreamSlot()).GetLocation();
			const int32 Id = bFinal ? Tourney->Tables.begin()->first : 0;
			Add(FCardRoomSound::Spot::FeatureTable, Id, P.X, P.Y, bFinal, bFinal ? (Id == Mine ? 0 : Seated(Id)) : 0);
		}
		Add(FCardRoomSound::Spot::Doorway, 0, DoorwayAt.X, DoorwayAt.Y, true, 0);
		Add(FCardRoomSound::Spot::Bar, 0, BarAt.X, BarAt.Y, true, 0);
		Add(FCardRoomSound::Spot::Rail, 0, RailEdgeAt.X, RailEdgeAt.Y, true, 0);
		if (H.bRailPlaced)
		{
			Add(FCardRoomSound::Spot::Rail, 1, H.RailAt.X, H.RailAt.Y, true, 0);
		}
		Add(FCardRoomSound::Spot::Desk, 0, DeskAt.X, DeskAt.Y, true, 0);
		Add(FCardRoomSound::Spot::Cage, 0, CageAt.X, CageAt.Y, true, 0);
		// The terrace doors: open while the player's at them or out there (the wind comes in).
		const bool bDoors = bWaitingOut || CurrentSpot() == ECardRoomSpot::Terrace;
		Add(FCardRoomSound::Spot::Terrace, 0, TerraceAt.X, TerraceAt.Y, bDoors, 0);
		const bool bSame = Em.size() == H.LastEmitters.size() && std::equal(Em.begin(), Em.end(), H.LastEmitters.begin(), [](const FCardRoomSound::Emitter& A, const FCardRoomSound::Emitter& B) {
			return A.Kind == B.Kind && A.Id == B.Id && A.X == B.X && A.Y == B.Y && A.Open == B.Open && A.Players == B.Players;
		});
		if (!bSame)
		{
			Sound->SetEmitters(Em);
			H.LastEmitters = Em;
		}
		// The night's mix (room::MixFor: Talk already carries the crowd and the Lombard lift), the dealers' calls and the PA
		// with the Voices bus (the stream itself plays on the Ambience bus).
		const int32 AllTables = FMath::Max(1, (Tourney->Spec.Entrants + Tourney->TableSize - 1) / FMath::Max(1, Tourney->TableSize));
		const float Share = FMath::Clamp(static_cast<float>(Tourney->Tables.size()) / static_cast<float>(AllTables), 0.0f, 1.0f);
		const ss::room::Mix Mix = ss::room::MixFor(RoomDirector->Now(), Share);
		const float Offset = Audio ? FMath::Clamp(20.0f * FMath::LogX(10.0f, FMath::Max(0.001f, Audio->GetVoicesVolume()) / FMath::Max(0.001f, Audio->GetAmbienceVolume())), -30.0f, 12.0f) : 0.0f;
		// The PA follows the Voices slider all the way down: at 0 the floor is silent (its captions stay).
		const float PaDb = Audio && Audio->GetVoicesVolume() <= 0.001f ? -120.0f : Offset;
		const float Now8[8] = {Mix.Bed, Mix.Talk, Mix.Chips, Mix.Calls, Mix.Rail, Mix.Crowd, Offset, PaDb};
		if (FMemory::Memcmp(Now8, H.LastMix, sizeof(Now8)) != 0)
		{
			if (FMemory::Memcmp(Now8, H.LastMix, sizeof(float) * 7) != 0)
			{
				Sound->SetMix(Mix, Offset);
			}
			if (Now8[7] != H.LastMix[7])
			{
				Sound->SetPaLevel(PaDb);
			}
			FMemory::Memcpy(H.LastMix, Now8, sizeof(Now8));
		}
		// Focus (a 3 kHz low-pass and -4 dB, held), tunnel hearing over 95 bpm (16 kHz down to 2.5 kHz at 140; your table's
		// own sounds and the heart stay clear: they aren't the room), and the terrace's glass (700 Hz, -14 dB).
		float Cutoff = 20000.0f;
		float Db = 0.0f;
		const float Focus = Pawn ? FMath::Clamp(Pawn->Focus, 0.0f, 1.0f) : 0.0f;
		if (Focus > 0.001f)
		{
			Cutoff = 20000.0f * FMath::Pow(3000.0f / 20000.0f, Focus);
			Db = -4.0f * Focus;
		}
		const float Bpm = Table->Composure.Bpm;
		if (Bpm > 95.0f)
		{
			const float U = FMath::Clamp((Bpm - 95.0f) / 45.0f, 0.0f, 1.0f);
			Cutoff = FMath::Min(Cutoff, 16000.0f * FMath::Pow(2500.0f / 16000.0f, U));
		}
		if (bWaitingOut)
		{
			Cutoff = 700.0f;
			Db = -14.0f;
		}
		Cutoff = FMath::RoundToFloat(Cutoff / 50.0f) * 50.0f;
		Db = FMath::RoundToFloat(Db * 4.0f) / 4.0f;
		if (Cutoff != H.LastMuffle[0] || Db != H.LastMuffle[1])
		{
			Sound->SetMuffle(Cutoff, Db);
			H.LastMuffle[0] = Cutoff;
			H.LastMuffle[1] = Db;
		}
		const float RoundPace = FMath::RoundToFloat(Pace * 20.0f) / 20.0f;
		if (RoundPace != H.LastPace)
		{
			Sound->SetPace(RoundPace);
			H.LastPace = RoundPace;
		}
		H.FocusMuffle = Focus;
		Sound->Pump();
	}
	H.HeroTable = Mine;
	const double Ms = FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64() - C0);
	H.HostMsSum += Ms;
	H.HostMsPeak = FMath::Max(H.HostMsPeak, Ms);
	++H.HostFrames;
}

void ABackRoomGameMode::PlaceBackers()
{
	if (!RoomHost.IsValid() || !RoomDirector.IsValid() || !Tourney || !Save.IsValid() || !Stage)
	{
		return;
	}
	FRoomHost& H = *RoomHost;
	const int32 Anchor = Stage->GetRoomAnchor();
	const bool bFinal = Tourney->Tables.size() <= 1;
	const int32 RailKey = Anchor * 2 + (bFinal ? 1 : 0);
	// (Nothing to look at again until someone's out or the player's table moves: no per-look allocations.)
	if (Tourney->Remaining == H.RailRemaining && RailKey == H.RailAnchor)
	{
		return;
	}
	H.RailRemaining = Tourney->Remaining;
	// Who holds a piece of the player and is out of tonight's field (stake::RailFor: someone still in plays straight at
	// their own table, R7; Dee deals the Sunday), at most two.
	std::set<std::string> Out;
	for (const ss::TPlayer& P : Tourney->Players)
	{
		if (P.Busted && !P.IsHero)
		{
			Out.insert(P.Name);
		}
	}
	std::vector<std::pair<std::string, int>> Rail = ss::stake::RailFor(Save->Life.Stakes, Std(LiveEntryId), Out);
	if (Rail.size() > 2)
	{
		Rail.resize(2);
	}
	if (Rail == H.RailSet && RailKey == H.RailAnchor)
	{
		return;
	}
	if (Rail != H.RailSet)
	{
		std::vector<ss::room::RailBacker> Backers;
		for (const std::pair<std::string, int>& R : Rail)
		{
			ss::room::RailBacker B;
			B.Name = R.first;
			B.Permille = R.second;
			Backers.push_back(B);
		}
		RoomDirector->SetRail(Backers);
		FString Names;
		for (const std::pair<std::string, int>& R : Rail)
		{
			Names += (Names.IsEmpty() ? TEXT("") : TEXT(", ")) + FString::Printf(TEXT("%s (%d%%)"), *StrOf(R.first), R.second / 10);
		}
		UE_LOG(LogEmbercrest, Log, TEXT("The rail: %s"), Names.IsEmpty() ? TEXT("none of the player's backers") : *Names);
	}
	H.RailSet = Rail;
	H.RailAnchor = RailKey;
	// Their bodies, inside the near tier (spec 2.13): on their feet at the bar line, or at the stage's edge at a final table,
	// watching the action (ue-wear's SetStanding: the street's baked standing idle). A body with no standing idle baked
	// watches from the chairs of the empty table nearest the action instead, the ones that face it across the felt.
	for (ABackRoomPlayer* B : RailBackers)
	{
		if (B)
		{
			B->SetActorHiddenInGame(true);
		}
	}
	const bool bWasPlaced = H.bRailPlaced;
	H.RailAtBar = 0;
	H.bRailPlaced = false;
	H.bRailStanding = false;
	if (Rail.empty())
	{
		if (bWasPlaced)
		{
			PlaceExtras(); // the near tier has its room back
		}
		return;
	}
	const FVector Target = ABackRoomStage::TableSlotLocal(Anchor).GetLocation();
	USceneComponent* Root = Stage->GetRoomRoot();
	const FTransform RoomWorld = Root ? Root->GetComponentTransform() : FTransform::Identity;
	const FVector Watch = RoomWorld.TransformPosition(Target + FVector(0.0, 0.0, ABackRoomStage::FeltZ));
	// Who they are and the body they wear in the room (the one they wore at their table tonight, if they played).
	TArray<FString> Names;
	TArray<FString> Bodies;
	for (const std::pair<std::string, int>& R : Rail)
	{
		const FString Name = StrOf(R.first);
		FString Id = TEXT("rail:") + Name;
		for (const ss::TPlayer& P : Tourney->Players)
		{
			if (StrOf(P.Name) == Name)
			{
				Id = StrOf(P.Id);
			}
		}
		const FString* Kept = LiveBodies.Find(Id);
		Names.Add(Name);
		Bodies.Add(Kept ? *Kept : DefaultBody(Id, Name));
	}
	auto BodyFor = [this, Root, &RoomWorld, &Names, &Bodies](int32 I, const FTransform& Local) -> ABackRoomPlayer* {
		ABackRoomPlayer* B = RailBackers.IsValidIndex(I) ? RailBackers[I].Get() : nullptr;
		if (B && (WornBody(B) != Bodies[I] || B->Persona.Name != Names[I]))
		{
			B->Destroy();
			B = nullptr;
		}
		if (!B)
		{
			B = SpawnPerson(Bodies[I], Local * RoomWorld, EBackRoomRole::Extra, PersonaFor(Names[I]));
			if (Root)
			{
				B->AttachToComponent(Root, FAttachmentTransformRules::KeepWorldTransform);
			}
			if (RailBackers.IsValidIndex(I))
			{
				RailBackers[I] = B;
			}
			else
			{
				RailBackers.Add(B);
			}
		}
		B->SetActorRelativeTransform(Local);
		B->SetActorHiddenInGame(false);
		DressPatches(*B, FacePatch(Names[I])); // (their own sponsor's patch, as they wore it at their table tonight)
		return B;
	};
	auto Watching = [&Watch, &RoomWorld](ABackRoomPlayer* B, const FTransform& Frame) {
		B->TableToWorld = Frame * RoomWorld;
		B->PotAt = Watch;
		B->DealerAt = Watch;
		B->HeroEyes = Watch + FVector(0.0, 0.0, 40.0);
		B->OthersAt.Reset();
		B->OthersAt.Add(Watch);
	};
	// On their feet: along the bar's front (the railbirds' line), or either side of the stage's rail at a final table.
	FVector Sum = FVector::ZeroVector;
	bool bStood = true;
	for (int32 I = 0; I < Names.Num() && bStood; ++I)
	{
		// (At the bar: the railbirds' line's last two places, PlaceExtras' figures 4 and 3, which make way.)
		const FVector At = bFinal ? FVector(RailEdgeAt.X - 70.0, (I == 0 ? -1.0 : 1.0) * 620.0, 0.0) : (I == 0 ? FVector(-570.0, 1350.0, 0.0) : FVector(-710.0, 1310.0, 0.0));
		const FRotator Face(0.0f, static_cast<float>((Target - At).Rotation().Yaw), 0.0f);
		const FTransform Local(Face, At);
		ABackRoomPlayer* B = BodyFor(I, Local);
		bStood = StandUp(*B);
		Watching(B, Local);
		Sum += At;
	}
	if (bStood)
	{
		H.RailAt = Sum / static_cast<double>(Names.Num());
		H.RailAtBar = bFinal ? 0 : Names.Num();
		H.bRailPlaced = true;
		H.bRailStanding = true;
		UE_LOG(LogEmbercrest, Log, TEXT("The rail's backers stand %s"), bFinal ? TEXT("at the stage's edge") : TEXT("at the bar line"));
	}
	else
	{
		// No standing idle for a body: everyone sits, at the empty table nearest the action.
		for (ABackRoomPlayer* B : RailBackers)
		{
			if (B)
			{
				SitDown(*B);
				B->SetActorHiddenInGame(true);
			}
		}
		int32 Best = INDEX_NONE;
		double BestDist = 1300.0;
		for (int32 Slot = 0; Slot < ABackRoomStage::StreamSlot(); ++Slot)
		{
			const bool bOpen = !bFinal && Tourney->Tables.count(Slot + 1) > 0;
			const double D = FVector::Dist2D(ABackRoomStage::TableSlotLocal(Slot).GetLocation(), Target);
			if (Slot != Anchor && !bOpen && D < BestDist)
			{
				Best = Slot;
				BestDist = D;
			}
		}
		if (Best != INDEX_NONE)
		{
			const FTransform Frame = ABackRoomStage::TableSlotLocal(Best);
			const FVector Center = Frame.GetLocation();
			TArray<TPair<double, int32>> Seats;
			for (const int32 Seat : {0, 1, 2, 3, 5, 6})
			{
				const FVector P = (ABackRoomStage::SeatTransform(Seat) * Frame).GetLocation();
				Seats.Add(TPair<double, int32>(FVector::DotProduct((Center - P).GetSafeNormal2D(), (Target - P).GetSafeNormal2D()), Seat));
			}
			Seats.Sort([](const TPair<double, int32>& A, const TPair<double, int32>& B) { return A.Key > B.Key; });
			Sum = FVector::ZeroVector;
			for (int32 I = 0; I < Names.Num(); ++I)
			{
				const FTransform Local = ABackRoomStage::SeatTransform(Seats[I].Value) * Frame;
				ABackRoomPlayer* B = BodyFor(I, Local);
				Watching(B, Frame);
				Sum += Local.GetLocation();
			}
			H.RailAt = Sum / static_cast<double>(Names.Num());
			H.bRailPlaced = true;
			UE_LOG(LogEmbercrest, Log, TEXT("The rail's backers watch from table %d's chairs"), Best + 1);
		}
	}
	if (H.bRailPlaced || bWasPlaced)
	{
		PlaceExtras(); // the near tier makes room for them (or has it back)
	}
}
