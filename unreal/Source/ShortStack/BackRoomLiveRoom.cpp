// The Embercrest's card room as a room (ABackRoomGameMode's live tournaments; the flow is BackRoomLive.cpp): the other
// tables and the people at them (the near tables' MetaHumans, the far crowd's figures, the railbirds), the floor on
// the PA and the banners, what the tournament's events and the table's notes mean to the room, the rail after the
// player's night is over, and the floor's last call for registration.

#include "BackRoomLiveShared.h"

#include "BackRoomGameMode.h"
#include "BackRoomPlayer.h"
#include "BackRoomStage.h"
#include "BackRoomTable.h"
#include "CareerSave.h"
#include "CastLooks.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "NightOneAudio.h"
#include "TimerManager.h"
#include "ShortStack/Audio/Synth.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Tournament.h"

#include <algorithm>

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

// ------------------------------------------------------------------ the room's people

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

// ------------------------------------------------------------------ the floor

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

// ------------------------------------------------------------------ the director's host (ue-room, next_segment_spec 4.11)

void ABackRoomGameMode::PlayCue(const ss::room::Cue& Cue)
{
	// Phase A: today's paths, so a cue played before ue-room's host is heard the way the room speaks now (ue-room swaps in
	// the stereo source, PaChain and the HUD's caption queue).
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
		Floor(Text, Cue.Chime);
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
}

void ABackRoomGameMode::TickRoom(float /*RealDt*/)
{
}

void ABackRoomGameMode::PlaceBackers()
{
}
