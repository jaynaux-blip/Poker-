#include "BackRoomChips.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace BackRoomChipsDetail
{
const int64 Values[4] = {100, 25, 5, 1};
const TCHAR* Names[4] = {TEXT("100"), TEXT("25"), TEXT("5"), TEXT("1")};

float SmoothStep01(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	return T * T * (3.0f - 2.0f * T);
}
} // namespace BackRoomChipsDetail

using namespace BackRoomChipsDetail;

int64 ABackRoomChips::ChipUnit = 1;

ABackRoomChips::ABackRoomChips()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Root->SetMobility(EComponentMobility::Movable);
	for (int32 D = 0; D < 4; ++D)
	{
		UInstancedStaticMeshComponent* Ism = CreateDefaultSubobject<UInstancedStaticMeshComponent>(*FString::Printf(TEXT("Chips%s"), Names[D]));
		Ism->SetupAttachment(Root);
		Ism->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Ism->SetMobility(EComponentMobility::Movable);
		Ism->bCastContactShadow = true;
		Denoms.Add(Ism);
	}
}

void ABackRoomChips::SetStyle(EBackRoomChipStyle InStyle, int32 InSeed)
{
	Style = InStyle;
	Seed = InSeed;
	Layout();
}

void ABackRoomChips::SetAmount(int64 InAmount)
{
	if (InAmount == Amount && Denoms[0]->GetStaticMesh())
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
	for (int32 D = 0; D < 4; ++D)
	{
		UInstancedStaticMeshComponent* Ism = Denoms[D];
		if (!Ism->GetStaticMesh())
		{
			const FString Path = FString::Printf(TEXT("/Game/ShortStack/Meshes/SM_Chip_%s/SM_Chip_%s.SM_Chip_%s"), Names[D], Names[D], Names[D]);
			Ism->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet));
		}
		Ism->ClearInstances();
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
