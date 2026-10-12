#include "BackRoomChips.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/Package.h"

namespace BackRoomChipsDetail
{
const int64 Values[4] = {100, 25, 5, 1};
const TCHAR* Names[4] = {TEXT("100"), TEXT("25"), TEXT("5"), TEXT("1")};
/** One instanced mesh per denomination: Dee's four, or the Embercrest's five. */
constexpr int32 Meshes = 5;

float SmoothStep01(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	return T * T * (3.0f - 2.0f * T);
}

/** The chip meshes found by name, and the names that aren't in the project (yet): every pile asks for its meshes as it's
 *  made (a bet slid out, the pot pushed), and a load of a package that isn't there looks on the disk each time. */
struct FChipMeshes
{
	TMap<FString, TWeakObjectPtr<UStaticMesh>> Found;
	TSet<FString> Missing;
};

FChipMeshes& ChipMeshes()
{
	static FChipMeshes Cache;
	return Cache;
}

UStaticMesh* LoadChip(const FString& Name)
{
	FChipMeshes& Cache = ChipMeshes();
	if (const TWeakObjectPtr<UStaticMesh>* Known = Cache.Found.Find(Name))
	{
		if (UStaticMesh* Mesh = Known->Get())
		{
			return Mesh;
		}
	}
	if (Cache.Missing.Contains(Name))
	{
		return nullptr;
	}
	const FString Path = FString::Printf(TEXT("/Game/ShortStack/Meshes/%s/%s.%s"), *Name, *Name, *Name);
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (Mesh)
	{
		Cache.Found.Add(Name, Mesh);
	}
	else
	{
		Cache.Missing.Add(Name);
	}
	return Mesh;
}

/** One tinted block-out material per base material and colour, shared by every pile (a pile is made for each bet slid
 *  out; a material instance each would be made and collected all night). Weak: the piles' meshes hold them. */
TMap<TPair<TWeakObjectPtr<UMaterialInterface>, uint32>, TWeakObjectPtr<UMaterialInstanceDynamic>>& Tints()
{
	static TMap<TPair<TWeakObjectPtr<UMaterialInterface>, uint32>, TWeakObjectPtr<UMaterialInstanceDynamic>> Cache;
	return Cache;
}

/** The club's clay a tournament chip stands in as until the art imports (tinted to its colour): the black $100 for the
 *  black 100, the red $5 under the purple and the orange, the white $1 under the yellow and the ash. */
const TCHAR* BlockoutFor(const FString& Mesh)
{
	if (Mesh == TEXT("SM_Chip_T100"))
	{
		return TEXT("SM_Chip_100");
	}
	if (Mesh == TEXT("SM_Chip_T500") || Mesh == TEXT("SM_Chip_T5K"))
	{
		return TEXT("SM_Chip_5");
	}
	return TEXT("SM_Chip_1");
}
} // namespace BackRoomChipsDetail

using namespace BackRoomChipsDetail;

int64 ABackRoomChips::ChipUnit = 1;
bool ABackRoomChips::bTournamentSet = false;
std::vector<ss::chipset::Denom> ABackRoomChips::InPlayNow;
int32 ABackRoomChips::SetRevision = 0;

ABackRoomChips::ABackRoomChips()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Root->SetMobility(EComponentMobility::Movable);
	// (The first four keep the names they've always had: Dee's $100, $25, $5 and $1.)
	static const TCHAR* ComponentNames[Meshes] = {TEXT("Chips100"), TEXT("Chips25"), TEXT("Chips5"), TEXT("Chips1"), TEXT("ChipsTop")};
	for (int32 D = 0; D < Meshes; ++D)
	{
		UInstancedStaticMeshComponent* Ism = CreateDefaultSubobject<UInstancedStaticMeshComponent>(ComponentNames[D]);
		Ism->SetupAttachment(Root);
		Ism->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Ism->SetMobility(EComponentMobility::Movable);
		Ism->bCastContactShadow = true;
		Denoms.Add(Ism);
	}
}

// ------------------------------------------------------------------ the room's chips

void ABackRoomChips::UseChips(bool bEmbercrest, const std::vector<ss::chipset::Denom>& InPlay)
{
	bool bSame = bEmbercrest == bTournamentSet && InPlay.size() == InPlayNow.size();
	for (size_t I = 0; bSame && I < InPlay.size(); ++I)
	{
		bSame = InPlay[I].Value == InPlayNow[I].Value;
	}
	if (bSame)
	{
		return;
	}
	if (bEmbercrest && !bTournamentSet)
	{
		// A new night at the Embercrest: look again for art imported since the last one.
		ChipMeshes().Missing.Reset();
	}
	if (!bEmbercrest)
	{
		Tints().Reset();
	}
	bTournamentSet = bEmbercrest;
	InPlayNow = bEmbercrest ? InPlay : std::vector<ss::chipset::Denom>();
	++SetRevision;
}

int64 ABackRoomChips::SmallestChip()
{
	return bTournamentSet && !InPlayNow.empty() ? static_cast<int64>(InPlayNow.front().Value) : 1;
}

UStaticMesh* ABackRoomChips::MeshFor(const ss::chipset::Denom& D, bool& bOutBlockout)
{
	bOutBlockout = false;
	const FString Name = D.Mesh ? FString(UTF8_TO_TCHAR(D.Mesh)) : FString();
	if (Name.IsEmpty())
	{
		return nullptr;
	}
	if (UStaticMesh* Art = LoadChip(Name))
	{
		return Art;
	}
	// The tournament set isn't in yet: the club's clay in its colours.
	bOutBlockout = true;
	return LoadChip(BlockoutFor(Name));
}

void ABackRoomChips::TintBlockout(UMeshComponent* Mesh, uint32 Body)
{
	if (!Mesh)
	{
		return;
	}
	const FLinearColor Tint = FLinearColor::FromSRGBColor(FColor(static_cast<uint8>((Body >> 16) & 0xff), static_cast<uint8>((Body >> 8) & 0xff), static_cast<uint8>(Body & 0xff)));
	// The bake is light, the importer's material multiplies it by its base colour factor: lifted a little so a dark chip
	// still reads.
	const FLinearColor Lifted(FMath::Min(1.0f, Tint.R * 1.15f + 0.02f), FMath::Min(1.0f, Tint.G * 1.15f + 0.02f), FMath::Min(1.0f, Tint.B * 1.15f + 0.02f), 1.0f);
	for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
	{
		UMaterialInterface* Material = Mesh->GetMaterial(Slot);
		if (UMaterialInstanceDynamic* Already = Cast<UMaterialInstanceDynamic>(Material))
		{
			// (Tinted before, for another chip: from the mesh's own material again.)
			Material = Already->Parent;
		}
		if (!Material)
		{
			continue;
		}
		const TPair<TWeakObjectPtr<UMaterialInterface>, uint32> TintKey(Material, Body);
		if (const TWeakObjectPtr<UMaterialInstanceDynamic>* Shared = Tints().Find(TintKey))
		{
			if (UMaterialInstanceDynamic* Mid = Shared->Get())
			{
				Mesh->SetMaterial(Slot, Mid);
				continue;
			}
		}
		UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Material, GetTransientPackage());
		if (!Mid)
		{
			continue;
		}
		Tints().Add(TintKey, Mid);
		Mesh->SetMaterial(Slot, Mid);
		bool bTinted = false;
		TArray<FMaterialParameterInfo> Infos;
		TArray<FGuid> Ids;
		Mid->GetAllVectorParameterInfo(Infos, Ids);
		for (const FMaterialParameterInfo& Info : Infos)
		{
			const FString Key = Info.Name.ToString().ToLower().Replace(TEXT("_"), TEXT("")).Replace(TEXT(" "), TEXT(""));
			if (Key == TEXT("basecolorfactor") || Key == TEXT("basecolorfactorrgb") || Key == TEXT("basecolor") || Key == TEXT("basecolortint") || Key == TEXT("tint"))
			{
				Mid->SetVectorParameterValue(Info.Name, Lifted);
				bTinted = true;
			}
		}
		if (!bTinted)
		{
			// (The listing came back empty: the importer's own name, set blind, as the street does.)
			Mid->SetVectorParameterValue(TEXT("BaseColorFactor"), Lifted);
		}
	}
}

void ABackRoomChips::AssignMeshes()
{
	const int32 For = bTournamentSet ? 1 : 0;
	if (MeshesFor == For)
	{
		return;
	}
	MeshesFor = For;
	for (int32 D = 0; D < Denoms.Num(); ++D)
	{
		UInstancedStaticMeshComponent* Ism = Denoms[D];
		Ism->ClearInstances();
		Ism->EmptyOverrideMaterials();
		if (!bTournamentSet)
		{
			Ism->SetStaticMesh(D < 4 ? LoadChip(FString(TEXT("SM_Chip_")) + Names[D]) : nullptr);
			continue;
		}
		const std::vector<ss::chipset::Denom>& All = ss::chipset::Set(ss::chipset::Room::Embercrest);
		if (D >= static_cast<int32>(All.size()))
		{
			Ism->SetStaticMesh(nullptr);
			continue;
		}
		bool bBlockout = false;
		Ism->SetStaticMesh(MeshFor(All[static_cast<size_t>(D)], bBlockout));
		if (bBlockout)
		{
			TintBlockout(Ism, All[static_cast<size_t>(D)].Body);
		}
	}
}

// ------------------------------------------------------------------ the pile

void ABackRoomChips::SetStyle(EBackRoomChipStyle InStyle, int32 InSeed)
{
	Style = InStyle;
	Seed = InSeed;
	Layout();
}

void ABackRoomChips::SetAmount(int64 InAmount)
{
	if (InAmount == Amount && LaidRevision == SetRevision && Denoms[0]->GetStaticMesh())
	{
		return;
	}
	Amount = FMath::Max<int64>(InAmount, 0);
	Layout();
}

void ABackRoomChips::Break(int64 Amount, EBackRoomChipStyle Style, int32 Counts[4])
{
	for (int32 D = 0; D < 4; ++D)
	{
		Counts[D] = 0;
	}
	int64 R = FMath::Max<int64>(Amount, 0);
	if (Style == EBackRoomChipStyle::Bet)
	{
		// What a player slides out: the fewest chips, but nobody bets a black chip at a $1/$2 game
		// unless the bet is big.
		for (int32 D = R >= 300 ? 0 : 1; D < 4; ++D)
		{
			Counts[D] = static_cast<int32>(R / Values[D]);
			R -= Counts[D] * Values[D];
		}
		return;
	}
	// A stack (or the pot): singles for change, reds in fives, greens in fours, the rest in blacks,
	// each denomination capped at a few columns' worth.
	const int32 Cap = Style == EBackRoomChipStyle::Pot ? 30 : 20;
	int64 Ones = R % 5;
	if (R - Ones >= 10)
	{
		Ones += 5;
	}
	R -= Ones;
	int64 Fives = (R % 25) / 5;
	R -= Fives * 5;
	const int64 ExtraFives = FMath::Min<int64>(R / 25, FMath::Max<int64>(0, (Cap - Fives) / 5)) * 5;
	Fives += ExtraFives;
	R -= ExtraFives * 5;
	int64 Greens = (R % 100) / 25;
	R -= Greens * 25;
	const int64 ExtraGreens = FMath::Min<int64>(R / 100, FMath::Max<int64>(0, (Cap - Greens) / 4)) * 4;
	Greens += ExtraGreens;
	R -= ExtraGreens * 25;
	Counts[0] = static_cast<int32>(R / 100);
	Counts[1] = static_cast<int32>(Greens);
	Counts[2] = static_cast<int32>(Fives);
	Counts[3] = static_cast<int32>(Ones);
}

void ABackRoomChips::Layout()
{
	LaidRevision = SetRevision;
	AssignMeshes();
	for (UInstancedStaticMeshComponent* Ism : Denoms)
	{
		Ism->ClearInstances();
	}
	if (bTournamentSet)
	{
		LayoutTournament();
		return;
	}
	int32 Counts[4];
	Break((Amount + FMath::Max<int64>(ChipUnit, 1) / 2) / FMath::Max<int64>(ChipUnit, 1), Style, Counts);
	FRandomStream Rng(Seed * 131 + static_cast<int32>(Amount % 100003));
	Height = 0.0;
	const double Pitch = 2.0 * Radius + 0.12;

	auto Column = [&](int32 D, int32 N, const FVector& Base, double Jitter, double LeanDeg) {
		const FVector LeanDir = FVector(Rng.FRandRange(-1.0f, 1.0f), Rng.FRandRange(-1.0f, 1.0f), 0.0).GetSafeNormal();
		const double LeanStep = FMath::Tan(FMath::DegreesToRadians(LeanDeg)) * Thickness;
		for (int32 I = 0; I < N; ++I)
		{
			const FVector Offset(Rng.FRandRange(-Jitter, Jitter), Rng.FRandRange(-Jitter, Jitter), 0.0);
			const FVector At = Base + Offset + LeanDir * (LeanStep * I) + FVector(0.0, 0.0, Thickness * I);
			Denoms[D]->AddInstance(FTransform(FRotator(0.0f, Rng.FRandRange(0.0f, 360.0f), 0.0f), At));
		}
		Height = FMath::Max(Height, Thickness * N);
	};

	if (Style == EBackRoomChipStyle::Stack)
	{
		// Columns of up to 20 in two staggered rows across the player (local Y), blacks at the back.
		int32 Slot = 0;
		for (int32 D = 0; D < 4; ++D)
		{
			for (int32 Left = Counts[D]; Left > 0; Left -= 20)
			{
				const int32 Row = Slot % 2;
				const int32 Col = Slot / 2;
				const FVector Base(Row == 0 ? 0.0 : -Pitch * 0.87, (Col + 0.5 * Row) * Pitch, 0.0);
				Column(D, FMath::Min(Left, 20), Base, 0.07, 0.15);
				++Slot;
			}
		}
		// Centered on the actor.
		const double Width = (FMath::Max(1, (Slot + 1) / 2) - 1) * Pitch;
		for (UInstancedStaticMeshComponent* Ism : Denoms)
		{
			Ism->SetRelativeLocation(FVector(Pitch * 0.43, -Width * 0.5, 0.0));
		}
	}
	else if (Style == EBackRoomChipStyle::Bet)
	{
		// Short columns side by side, as they were pushed out.
		int32 Slot = 0;
		for (int32 D = 0; D < 4; ++D)
		{
			for (int32 Left = Counts[D]; Left > 0; Left -= 10)
			{
				const FVector Base(Rng.FRandRange(-0.6f, 0.6f), Slot * (Pitch + 0.2) + Rng.FRandRange(-0.4f, 0.4f), 0.0);
				Column(D, FMath::Min(Left, 10), Base, 0.25, 0.6);
				++Slot;
			}
		}
		for (UInstancedStaticMeshComponent* Ism : Denoms)
		{
			Ism->SetRelativeLocation(FVector(0.0, -(Slot - 1) * (Pitch + 0.2) * 0.5, 0.0));
		}
	}
	else
	{
		// The pot: short leaning piles splashed around the middle, plus a few loose chips.
		TArray<FVector, TInlineAllocator<64>> Spots;
		const int32 Total = Counts[0] + Counts[1] + Counts[2] + Counts[3];
		const double Spread = 2.5 + 1.6 * FMath::Sqrt(static_cast<double>(Total));
		auto FreeSpot = [&]() {
			FVector Best = FVector::ZeroVector;
			double BestGap = -1.0;
			for (int32 Try = 0; Try < 12; ++Try)
			{
				const double A = Rng.FRandRange(0.0f, UE_TWO_PI);
				const double Rr = Spread * FMath::Sqrt(Rng.FRand());
				const FVector P(Rr * FMath::Cos(A), Rr * FMath::Sin(A), 0.0);
				double Gap = 1e9;
				for (const FVector& S : Spots)
				{
					Gap = FMath::Min(Gap, FVector::Dist2D(S, P));
				}
				if (Gap > BestGap)
				{
					BestGap = Gap;
					Best = P;
				}
				if (Gap > Pitch)
				{
					break;
				}
			}
			Spots.Add(Best);
			return Best;
		};
		for (int32 D = 0; D < 4; ++D)
		{
			for (int32 Left = Counts[D]; Left > 0;)
			{
				const int32 N = FMath::Min(Left, Rng.RandRange(1, 7));
				Column(D, N, FreeSpot(), 0.35, N > 1 ? 1.5 : 0.0);
				Left -= N;
			}
		}
		for (UInstancedStaticMeshComponent* Ism : Denoms)
		{
			Ism->SetRelativeLocation(FVector::ZeroVector);
		}
	}
}

void ABackRoomChips::LayoutTournament()
{
	// The Embercrest's set as the room stacks it (ss::chipset::Lay): columns of at most 20, one denomination each, the
	// biggest in front; a colour-up's leftover small chips stay as one short column of the old chip.
	const std::vector<ss::chipset::Denom>& All = ss::chipset::Set(ss::chipset::Room::Embercrest);
	const ss::chipset::Style Kind = Style == EBackRoomChipStyle::Bet ? ss::chipset::Style::Bet : Style == EBackRoomChipStyle::Pot ? ss::chipset::Style::Pot : ss::chipset::Style::Stack;
	const std::vector<ss::chipset::Column> Cols = ss::chipset::Lay(static_cast<ss::Chips>(Amount), InPlayNow, Kind);
	auto MeshOf = [&All](const ss::chipset::Column& C) {
		for (int32 I = 0; I < static_cast<int32>(All.size()); ++I)
		{
			if (All[static_cast<size_t>(I)].Value == C.Value)
			{
				return FMath::Min(I, Meshes - 1);
			}
		}
		return -1;
	};
	FRandomStream Rng(Seed * 131 + static_cast<int32>(Amount % 100003));
	Height = 0.0;
	const double Pitch = 2.0 * Radius + 0.12;
	auto Column = [&](int32 D, int32 N, const FVector& Base, double Jitter, double LeanDeg) {
		if (D < 0 || N <= 0)
		{
			return;
		}
		const FVector LeanDir = FVector(Rng.FRandRange(-1.0f, 1.0f), Rng.FRandRange(-1.0f, 1.0f), 0.0).GetSafeNormal();
		const double LeanStep = FMath::Tan(FMath::DegreesToRadians(LeanDeg)) * Thickness;
		for (int32 I = 0; I < N; ++I)
		{
			const FVector Offset(Rng.FRandRange(-Jitter, Jitter), Rng.FRandRange(-Jitter, Jitter), 0.0);
			const FVector At = Base + Offset + LeanDir * (LeanStep * I) + FVector(0.0, 0.0, Thickness * I);
			Denoms[D]->AddInstance(FTransform(FRotator(0.0f, Rng.FRandRange(0.0f, 360.0f), 0.0f), At));
		}
		Height = FMath::Max(Height, Thickness * N);
	};
	if (Style == EBackRoomChipStyle::Stack)
	{
		// Two rows across the player: the first half of the columns (the big chips) in front, toward the middle of the
		// table, the rest behind them; each row left to right.
		const int32 N = static_cast<int32>(Cols.size());
		const int32 Front = (N + 1) / 2;
		for (int32 K = 0; K < N; ++K)
		{
			const bool bBack = K >= Front;
			const int32 Col = bBack ? K - Front : K;
			const FVector Base(bBack ? -Pitch * 0.87 : 0.0, (Col + (bBack ? 0.5 : 0.0)) * Pitch, 0.0);
			Column(MeshOf(Cols[static_cast<size_t>(K)]), FMath::Min(Cols[static_cast<size_t>(K)].Count, 20), Base, 0.07, 0.15);
		}
		const double Width = (FMath::Max(1, Front) - 1) * Pitch;
		for (UInstancedStaticMeshComponent* Ism : Denoms)
		{
			Ism->SetRelativeLocation(FVector(Pitch * 0.43, -Width * 0.5, 0.0));
		}
	}
	else if (Style == EBackRoomChipStyle::Bet)
	{
		// Short columns side by side, as they were pushed out.
		int32 Slot = 0;
		for (const ss::chipset::Column& C : Cols)
		{
			for (int32 Left = C.Count; Left > 0; Left -= 10)
			{
				const FVector Base(Rng.FRandRange(-0.6f, 0.6f), Slot * (Pitch + 0.2) + Rng.FRandRange(-0.4f, 0.4f), 0.0);
				Column(MeshOf(C), FMath::Min(Left, 10), Base, 0.25, 0.6);
				++Slot;
			}
		}
		for (UInstancedStaticMeshComponent* Ism : Denoms)
		{
			Ism->SetRelativeLocation(FVector(0.0, -(FMath::Max(Slot, 1) - 1) * (Pitch + 0.2) * 0.5, 0.0));
		}
	}
	else
	{
		// The pot: short leaning piles splashed round the middle.
		TArray<FVector, TInlineAllocator<64>> Spots;
		int32 Total = 0;
		for (const ss::chipset::Column& C : Cols)
		{
			Total += C.Count;
		}
		const double Spread = 2.5 + 1.6 * FMath::Sqrt(static_cast<double>(Total));
		auto FreeSpot = [&]() {
			FVector Best = FVector::ZeroVector;
			double BestGap = -1.0;
			for (int32 Try = 0; Try < 12; ++Try)
			{
				const double A = Rng.FRandRange(0.0f, UE_TWO_PI);
				const double Rr = Spread * FMath::Sqrt(Rng.FRand());
				const FVector P(Rr * FMath::Cos(A), Rr * FMath::Sin(A), 0.0);
				double Gap = 1e9;
				for (const FVector& S : Spots)
				{
					Gap = FMath::Min(Gap, FVector::Dist2D(S, P));
				}
				if (Gap > BestGap)
				{
					BestGap = Gap;
					Best = P;
				}
				if (Gap > Pitch)
				{
					break;
				}
			}
			Spots.Add(Best);
			return Best;
		};
		for (const ss::chipset::Column& C : Cols)
		{
			for (int32 Left = C.Count; Left > 0;)
			{
				const int32 N = FMath::Min(Left, Rng.RandRange(1, 7));
				Column(MeshOf(C), N, FreeSpot(), 0.35, N > 1 ? 1.5 : 0.0);
				Left -= N;
			}
		}
		for (UInstancedStaticMeshComponent* Ism : Denoms)
		{
			Ism->SetRelativeLocation(FVector::ZeroVector);
		}
	}
}

FVector ABackRoomChips::GetTop() const
{
	return GetActorLocation() + FVector(0.0, 0.0, Height);
}

void ABackRoomChips::SlideTo(const FVector& Target, float Duration, float Arc)
{
	From = GetActorLocation();
	To = Target;
	MoveT = 0.0f;
	MoveDuration = FMath::Max(Duration, 0.05f);
	MoveArc = Arc;
}

void ABackRoomChips::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// The room's chips changed (a colour-up): laid out again in the new ones.
	if (LaidRevision != SetRevision)
	{
		Layout();
	}
	if (MoveT >= 1.0f)
	{
		return;
	}
	MoveT = FMath::Min(1.0f, MoveT + DeltaSeconds / MoveDuration);
	const float E = SmoothStep01(MoveT);
	FVector P = FMath::Lerp(From, To, static_cast<double>(E));
	P.Z += MoveArc * FMath::Sin(MoveT * UE_PI);
	SetActorLocation(P);
}
