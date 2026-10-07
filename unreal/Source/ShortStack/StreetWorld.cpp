#include "StreetWorld.h"

#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"

namespace StreetWorldDetail
{
/** How far a use's reach (its Radius) extends past it, and how squarely an eye must face it to reach it (AStreetStage::SpotFor). */
constexpr double ReachSlack = 80.0;
constexpr double MinFacing = 0.2;
constexpr double OnTopOf = 60.0;
/** The rain material's dry boxes (M_StreetRain DryMin0..7). */
constexpr int32 MaxDryBoxes = 8;

struct FPreloadEntry
{
	const TCHAR* Name;
	FStreetPreload::FAppend Append;
};

TArray<FPreloadEntry>& PreloadEntries()
{
	static TArray<FPreloadEntry> Entries;
	return Entries;
}

struct FOverrideEntry
{
	const TCHAR* Name;
	const TCHAR* ResetTo;
};

TArray<FOverrideEntry>& OverrideEntries()
{
	static TArray<FOverrideEntry> Entries;
	return Entries;
}

TAutoConsoleVariable<int32> CVarKeepOverrides(TEXT("ss.Street.KeepOverrides"), 0,
	TEXT("1: the street's test overrides (ss.Street.Rain and the like) keep what they were set to across Play sessions; 0 (default): each session starts them at their resets."));

/** Whether an eye at Eye looking along Dir reaches At (within Radius), and how well (higher is better): SpotFor's scoring. */
bool Reach(const FVector& At, double Radius, const FVector& Eye, const FVector& Dir, double& OutScore)
{
	const FVector To = At - Eye;
	const double Flat = FVector(To.X, To.Y, 0.0).Size();
	if (Flat > Radius + ReachSlack)
	{
		return false;
	}
	// Facing it matters more than being exactly on it: the shelves are long.
	const double Facing = FVector::DotProduct(FVector(To.X, To.Y, 0.0).GetSafeNormal(), FVector(Dir.X, Dir.Y, 0.0).GetSafeNormal());
	if (Facing < MinFacing && Flat > OnTopOf)
	{
		return false;
	}
	OutScore = Facing - Flat / (Radius + ReachSlack);
	return true;
}

/** A use on a person: someone to talk to, anchored on their pawn (they come first, wherever they stand). */
bool IsPerson(const FStreetUse& U)
{
	const USceneComponent* A = U.Anchor.Get();
	const AActor* Owner = A ? A->GetOwner() : nullptr;
	return U.Verb == EStreetVerb::Talk && Owner && Owner->IsA<APawn>();
}

double BoxVolume(const FBox& B)
{
	return B.IsValid ? B.GetVolume() : 0.0;
}
} // namespace StreetWorldDetail

using namespace StreetWorldDetail;

// ------------------------------------------------------------------ the registry

UStreetWorld* UStreetWorld::Get(const UObject* Context)
{
	UWorld* World = Context && GEngine ? GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UStreetWorld>() : nullptr;
}

bool UStreetWorld::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

int32 UStreetWorld::AddUse(const FStreetUse& U)
{
	FUseEntry& E = Uses.AddDefaulted_GetRef();
	E.Handle = NextHandle++;
	E.Use = U;
	return E.Handle;
}

void UStreetWorld::RemoveUse(int32 Handle)
{
	Uses.RemoveAll([Handle](const FUseEntry& E) { return E.Handle == Handle; });
}

FVector UStreetWorld::UseLocation(const FStreetUse& U)
{
	const USceneComponent* A = U.Anchor.Get();
	return A ? A->GetComponentTransform().TransformPosition(U.Offset) : U.Offset;
}

FString UStreetWorld::PromptOf(const FStreetUse& U)
{
	if (U.WhyNot)
	{
		FString Why = U.WhyNot();
		if (!Why.IsEmpty())
		{
			return Why;
		}
	}
	return U.Prompt;
}

const FStreetUse* UStreetWorld::Best(const FVector& Eye, const FVector& Dir) const
{
	// As SpotFor: the best score over -1 (an eye right on top of one but turned away from it doesn't reach it), the earlier
	// of two equal. A use whose anchor has gone (someone's door, a person) is passed over.
	const FStreetUse* BestPerson = nullptr;
	const FStreetUse* BestSpot = nullptr;
	double PersonScore = -1.0;
	double SpotScore = -1.0;
	for (const FUseEntry& E : Uses)
	{
		const FStreetUse& U = E.Use;
		if (U.Anchor.IsStale())
		{
			continue;
		}
		double Score = 0.0;
		if (!Reach(UseLocation(U), U.Radius, Eye, Dir, Score))
		{
			continue;
		}
		if (IsPerson(U))
		{
			if (Score > PersonScore)
			{
				PersonScore = Score;
				BestPerson = &U;
			}
		}
		else if (Score > SpotScore)
		{
			SpotScore = Score;
			BestSpot = &U;
		}
	}
	return BestPerson ? BestPerson : BestSpot;
}

const FStreetUse* UStreetWorld::FindUse(FName Id) const
{
	for (const FUseEntry& E : Uses)
	{
		if (E.Use.Id == Id)
		{
			return &E.Use;
		}
	}
	return nullptr;
}

TArray<TPair<int32, const FStreetUse*>> UStreetWorld::AllUses() const
{
	TArray<TPair<int32, const FStreetUse*>> Out;
	for (const FUseEntry& E : Uses)
	{
		Out.Emplace(E.Handle, &E.Use);
	}
	return Out;
}

void UStreetWorld::AddZone(const FStreetZone& Z)
{
	RemoveZone(Z.Id);
	Zones.Add(Z);
	++Revision;
}

void UStreetWorld::RemoveZone(FName Id)
{
	if (Zones.RemoveAll([Id](const FStreetZone& Z) { return Z.Id == Id; }) > 0)
	{
		++Revision;
	}
}

const FStreetZone* UStreetWorld::ZoneAt(const FVector& At) const
{
	const FStreetZone* Found = nullptr;
	for (const FStreetZone& Z : Zones)
	{
		if (!Z.Box.IsValid || !Z.Box.IsInsideOrOn(At))
		{
			continue;
		}
		if (!Found || Z.Priority > Found->Priority || (Z.Priority == Found->Priority && BoxVolume(Z.Box) < BoxVolume(Found->Box)))
		{
			Found = &Z;
		}
	}
	return Found;
}

const FStreetZone* UStreetWorld::FindZone(FName Id) const
{
	return Zones.FindByPredicate([Id](const FStreetZone& Z) { return Z.Id == Id; });
}

bool UStreetWorld::IsOffTheSet(const FVector& At) const
{
	if (Zones.Num() == 0)
	{
		return false;
	}
	return At.Z < FloorZ || ZoneAt(At) == nullptr;
}

TArray<FBox> UStreetWorld::DryBoxes() const
{
	TArray<const FStreetZone*> Dry;
	for (const FStreetZone& Z : Zones)
	{
		if (Z.bDry && Z.Box.IsValid)
		{
			Dry.Add(&Z);
		}
	}
	Dry.StableSort([](const FStreetZone& A, const FStreetZone& B) { return A.Priority > B.Priority; });
	TArray<FBox> Out;
	for (int32 I = 0; I < Dry.Num() && I < MaxDryBoxes; ++I)
	{
		Out.Add(Dry[I]->Box);
	}
	if (Dry.Num() > MaxDryBoxes)
	{
		UE_LOG(LogTemp, Warning, TEXT("UStreetWorld: %d dry zones, the rain masks only %d"), Dry.Num(), MaxDryBoxes);
	}
	return Out;
}

void UStreetWorld::AddStart(FName Id, const FTransform& T)
{
	if (!Starts.Contains(Id))
	{
		StartOrder.Add(Id);
	}
	Starts.Add(Id, T);
}

void UStreetWorld::RemoveStart(FName Id)
{
	Starts.Remove(Id);
	StartOrder.Remove(Id);
}

bool UStreetWorld::Start(FName Id, FTransform& Out) const
{
	if (const FTransform* T = Starts.Find(Id))
	{
		Out = *T;
		return true;
	}
	return false;
}

void UStreetWorld::AddShot(FName Id, const FTransform& T, float Fov)
{
	if (!Shots.Contains(Id))
	{
		ShotOrder.Add(Id);
	}
	FShotEntry& S = Shots.Add(Id);
	S.T = T;
	S.Fov = Fov;
}

void UStreetWorld::RemoveShot(FName Id)
{
	Shots.Remove(Id);
	ShotOrder.Remove(Id);
}

bool UStreetWorld::Shot(FName Id, FTransform& Out, float& Fov) const
{
	if (const FShotEntry* S = Shots.Find(Id))
	{
		Out = S->T;
		Fov = S->Fov;
		return true;
	}
	return false;
}

void UStreetWorld::AddStation(FName Id, FName Venue, const FTransform& T, FName Pose, float SeatHeight, bool bCovered)
{
	FStationEntry* Was = StationMap.Find(Id);
	const int32 Order = Was ? Was->Order : NextStationOrder++;
	FStationEntry& S = StationMap.Add(Id);
	S.Venue = Venue;
	S.T = T;
	S.Pose = Pose;
	S.SeatHeight = SeatHeight;
	S.bCovered = bCovered;
	S.Order = Order;
}

TArray<FName> UStreetWorld::Stations(FName Venue) const
{
	TArray<TPair<int32, FName>> Found;
	for (const TPair<FName, FStationEntry>& S : StationMap)
	{
		if (Venue.IsNone() || S.Value.Venue == Venue)
		{
			Found.Emplace(S.Value.Order, S.Key);
		}
	}
	Found.Sort([](const TPair<int32, FName>& A, const TPair<int32, FName>& B) { return A.Key < B.Key; });
	TArray<FName> Out;
	for (const TPair<int32, FName>& F : Found)
	{
		Out.Add(F.Value);
	}
	return Out;
}

bool UStreetWorld::Station(FName Id, FTransform& Out, FName& Pose, float& SeatHeight) const
{
	if (const FStationEntry* S = StationMap.Find(Id))
	{
		Out = S->T;
		Pose = S->Pose;
		SeatHeight = S->SeatHeight;
		return true;
	}
	return false;
}

bool UStreetWorld::IsCovered(FName Id) const
{
	const FStationEntry* S = StationMap.Find(Id);
	return S && S->bCovered;
}

void UStreetWorld::RemoveVenue(FName Venue)
{
	if (Venue.IsNone())
	{
		return;
	}
	Uses.RemoveAll([Venue](const FUseEntry& E) { return E.Use.Venue == Venue; });
	for (auto It = StationMap.CreateIterator(); It; ++It)
	{
		if (It.Value().Venue == Venue)
		{
			It.RemoveCurrent();
		}
	}
	RemoveZone(Venue);
	DoorList.RemoveAll([Venue](const FStreetDoor& D) { return D.Id == Venue; });
}

void UStreetWorld::AddDoor(const FStreetDoor& D)
{
	const int32 Index = DoorList.IndexOfByPredicate([&D](const FStreetDoor& E) { return E.Id == D.Id; });
	if (Index != INDEX_NONE)
	{
		DoorList[Index] = D;
	}
	else
	{
		DoorList.Add(D);
	}
}

void UStreetWorld::AddWalkway(const FStreetWalkway& W)
{
	const int32 Index = WalkwayList.IndexOfByPredicate([&W](const FStreetWalkway& E) { return E.Street == W.Street && E.Side == W.Side; });
	if (Index != INDEX_NONE)
	{
		WalkwayList[Index] = W;
	}
	else
	{
		WalkwayList.Add(W);
	}
}

void UStreetWorld::NoteWalker(const FVector& At)
{
	const uint64 Now = GFrameCounter;
	WalkerList.RemoveAll([Now](const FWalker& W) { return W.Frame + 1 < Now; });
	if (WalkerList.Num() < 128)
	{
		WalkerList.Add({At, Now});
	}
}

bool UStreetWorld::WalkersNear(const FVector& At, double Reach, double ZReach) const
{
	const uint64 Now = GFrameCounter;
	for (const FWalker& W : WalkerList)
	{
		if (W.Frame + 1 >= Now && FVector::DistSquaredXY(W.At, At) < Reach * Reach && FMath::Abs(W.At.Z - At.Z) < ZReach)
		{
			return true;
		}
	}
	return false;
}

void UStreetWorld::SetHold(bool bHold, double World)
{
	if (bHold && !bHeld)
	{
		HeldAt = World;
	}
	bHeld = bHold;
}

FString UStreetWorld::Describe() const
{
	int32 Dry = 0;
	int32 Profiles = 0;
	for (const FStreetZone& Z : Zones)
	{
		Dry += Z.bDry ? 1 : 0;
		Profiles += Z.HasCameraProfile() ? 1 : 0;
	}
	return FString::Printf(TEXT("registry: %d uses, %d zones (%d dry, %d camera profiles), %d starts, %d shots, %d stations, %d doors, %d walkways%s"), Uses.Num(), Zones.Num(),
		Dry, Profiles, Starts.Num(), Shots.Num(), StationMap.Num(), DoorList.Num(), WalkwayList.Num(), bHeld ? TEXT(", the corner held (a skip)") : TEXT(""));
}

void UStreetWorld::DrawDebug(float Seconds) const
{
#if ENABLE_DRAW_DEBUG
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// Zones: interiors amber, dry ones blue, the rest grey; their names at their middles.
	for (const FStreetZone& Z : Zones)
	{
		if (!Z.Box.IsValid)
		{
			continue;
		}
		const FColor Color = Z.bInterior ? FColor(255, 170, 40) : Z.bDry ? FColor(60, 140, 255) : FColor(160, 160, 160);
		DrawDebugBox(World, Z.Box.GetCenter(), Z.Box.GetExtent(), Color, false, Seconds, 0, Z.bInterior ? 4.0f : 2.0f);
		DrawDebugString(World, Z.Box.GetCenter(), FString::Printf(TEXT("%s (%s, p%d)"), *Z.Id.ToString(), *Z.Place, Z.Priority), nullptr, Color, Seconds);
	}
	// Uses: their reach on the ground, green; people's magenta.
	for (const FUseEntry& E : Uses)
	{
		const FVector At = UseLocation(E.Use);
		const FColor Color = IsPerson(E.Use) ? FColor::Magenta : FColor::Green;
		DrawDebugCircle(World, At, static_cast<float>(E.Use.Radius + ReachSlack), 24, Color, false, Seconds, 0, 1.5f, FVector(1, 0, 0), FVector(0, 1, 0), false);
		DrawDebugString(World, At + FVector(0, 0, 30), FString::Printf(TEXT("%s [%s]"), *E.Use.Id.ToString(), *StaticEnum<EStreetVerb>()->GetNameStringByValue(static_cast<int64>(E.Use.Verb))),
			nullptr, Color, Seconds);
	}
	// Stations: an arrow the way they face; starts: cyan arrows; doors: yellow, from outside to the threshold.
	for (const TPair<FName, FStationEntry>& S : StationMap)
	{
		const FVector At = S.Value.T.GetLocation();
		DrawDebugDirectionalArrow(World, At, At + S.Value.T.GetRotation().GetForwardVector() * 60.0, 20.0f, FColor::Orange, false, Seconds, 0, 2.0f);
		DrawDebugString(World, At + FVector(0, 0, 20), S.Key.ToString(), nullptr, FColor::Orange, Seconds);
	}
	for (const TPair<FName, FTransform>& S : Starts)
	{
		const FVector At = S.Value.GetLocation();
		DrawDebugDirectionalArrow(World, At, At + S.Value.GetRotation().GetForwardVector() * 100.0, 30.0f, FColor::Cyan, false, Seconds, 0, 3.0f);
		DrawDebugString(World, At + FVector(0, 0, 40), FString::Printf(TEXT("start %s"), *S.Key.ToString()), nullptr, FColor::Cyan, Seconds);
	}
	for (const FStreetDoor& D : DoorList)
	{
		DrawDebugDirectionalArrow(World, D.Outside.GetLocation(), D.Threshold, 30.0f, FColor::Yellow, false, Seconds, 0, 3.0f);
	}
	for (const FStreetWalkway& W : WalkwayList)
	{
		for (const FStreetLane& L : W.Lanes)
		{
			DrawDebugLine(World, L.Start + FVector(0, 0, 3), L.End + FVector(0, 0, 3), FColor(90, 200, 90), false, Seconds, 0, 1.0f);
		}
	}
#endif
}

// ------------------------------------------------------------------ the preload list

FStreetPreload::FRegister::FRegister(const TCHAR* Name, FAppend Append)
{
	PreloadEntries().Add({Name, Append});
}

TArray<FString> FStreetPreload::Collect()
{
	TArray<FString> Paths;
	for (const FPreloadEntry& E : PreloadEntries())
	{
		if (E.Append)
		{
			E.Append(Paths);
		}
	}
	Paths.Sort();
	TArray<FString> Out;
	for (const FString& P : Paths)
	{
		if (!P.IsEmpty() && (Out.Num() == 0 || Out.Last() != P))
		{
			Out.Add(P);
		}
	}
	return Out;
}

FString FStreetPreload::MeshPath(const TCHAR* Name)
{
	return FString::Printf(TEXT("/Game/ShortStack/Meshes/%s/%s.%s"), Name, Name, Name);
}

// ------------------------------------------------------------------ the test overrides

FStreetOverrides::FRegister::FRegister(const TCHAR* Name, const TCHAR* ResetTo)
{
	OverrideEntries().Add({Name, ResetTo});
}

int32 FStreetOverrides::ResetAll()
{
	if (CVarKeepOverrides.GetValueOnGameThread() != 0)
	{
		return 0;
	}
	int32 Reset = 0;
	for (const FOverrideEntry& E : OverrideEntries())
	{
		IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(E.Name);
		if (!Var)
		{
			continue;
		}
		const bool bAt = Var->IsVariableString() ? Var->GetString() == E.ResetTo : FMath::IsNearlyEqual(Var->GetFloat(), FCString::Atof(E.ResetTo));
		if (!bAt)
		{
			UE_LOG(LogTemp, Display, TEXT("Street: %s was %s, back to %s (ss.Street.KeepOverrides 1 keeps it)"), E.Name, *Var->GetString(), E.ResetTo);
			Var->Set(E.ResetTo, ECVF_SetByConsole);
			++Reset;
		}
	}
	return Reset;
}
