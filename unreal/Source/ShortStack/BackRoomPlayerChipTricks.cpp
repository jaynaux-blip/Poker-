// ABackRoomPlayer's chips to play with: a short column of eight beside the stack (two colors, four each) that a
// fidgety player riffles, or lifts and lets fall chip by chip, while the right hand has nothing else to do.

#include "BackRoomChips.h"
#include "BackRoomPlayer.h"
#include "BackRoomPlayerShared.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "ShortStack/Game/Session.h"

using namespace BackRoomPlayerDetail;

namespace BackRoomChipTricksDetail
{
const double Thick = ABackRoomChips::Thickness;
const double Radius = ABackRoomChips::Radius;
/** A riffle's two halves sit this far either side of the column: side by side, just apart. */
const double Half = Radius + 0.06;
/** How far a riffle's halves tilt, inner edges up, as they fan (degrees). */
const float FanTilt = 12.0f;
/** Each trick's length (seconds at pace 1), with the pause after it. */
const float RiffleLength = 1.6f;
const float DropLength = 1.75f;
/** How high the drop lifts the column, and when it lets the bottom chip go and each after it. */
const double DropLift = 4.5;
const float DropFirst = 0.42f;
const float DropEvery = 0.1f;
const double Gravity = 981.0;

float Smooth(float X)
{
	X = FMath::Clamp(X, 0.0f, 1.0f);
	return X * X * (3.0f - 2.0f * X);
}

/** Up and down over [A, B]. */
float Window(float T, float A, float B)
{
	return Envelope((T - A) / (B - A), 0.25f, 0.25f);
}

float FallTime()
{
	return static_cast<float>(FMath::Sqrt(2.0 * DropLift / Gravity));
}
} // namespace BackRoomChipTricksDetail

using namespace BackRoomChipTricksDetail;

bool ABackRoomPlayer::HasPlayChips() const
{
	return SeatRole == EBackRoomRole::Player && Persona.ChipFidget >= 0.45f && StackPile
		&& StackPile->GetAmount() >= 30 * FMath::Max<int64>(ABackRoomChips::ChipUnit, 1);
}

FVector ABackRoomPlayer::PlayChipsBase() const
{
	// Between the cards and the stack, a little nearer the player; it goes where the stack goes (all in, it goes too).
	const FVector Stack = StackPile ? StackPile->GetActorLocation() : Spots.Stack;
	return Stack - Spots.Inward * 6.0 - GetActorRightVector() * 8.5;
}

void ABackRoomPlayer::PoseTrick(float T, FTransform Out[8], int32 NewOrder[8], float& Top) const
{
	const FVector Base = PlayChipsBase();
	const FVector Side = GetActorRightVector();
	const FVector In = Spots.Inward;
	const float Yaw = GetActorRotation().Yaw;
	auto Place = [&](int32 Id, double Lateral, double Height, float TiltDeg) {
		// Tilted about the line into the table: + raises the edge on the player's right.
		const FQuat Q = FQuat(In, FMath::DegreesToRadians(TiltDeg)) * FRotator(0.0f, Yaw + PlayYaw[Id], 0.0f).Quaternion();
		// Lifted so the low edge of a tilted chip stays off the felt.
		const double Raise = Radius * FMath::Abs(FMath::Sin(FMath::DegreesToRadians(TiltDeg)));
		return FTransform(Q, Base + Side * Lateral + FVector(0.0, 0.0, Height + Raise));
	};
	double High = 0.0;
	if (TrickKind == 0)
	{
		// The riffle: the top half slides off beside the bottom half; both tilt, inner edges up, and fan open
		// from the top down; they flatten, push together interleaved, and square up.
		const float Tilt = FanTilt * Smooth((T - 0.3f) / 0.12f) * (1.0f - Smooth((T - 0.74f) / 0.08f));
		const double Apart = Half * Smooth(T / 0.2f) * (1.0 - Smooth((T - 0.82f) / 0.18f));
		for (int32 S = 0; S < 8; ++S)
		{
			const int32 Id = PlayOrder[S];
			const bool bRight = S >= 4;
			const int32 I = S % 4;
			const int32 K = 2 * I + (bRight ? 1 : 0);
			NewOrder[K] = Id;
			// The top half drops beside the bottom one once it's clear of it.
			double H = bRight ? (S - 4.0 * Smooth((T - 0.17f) / 0.13f)) * Thick : I * Thick;
			// Fanning: each chip rises to its place in the merged column, the top ones first.
			const float Delay = (7 - K) / 7.0f * 0.22f;
			const float Q = Smooth((T - 0.42f - Delay) / 0.18f);
			H += Q * (K - I) * Thick;
			Out[Id] = Place(Id, (bRight ? 1.0 : -1.0) * Apart, H, bRight ? -Tilt : Tilt);
			High = FMath::Max(High, H);
		}
	}
	else
	{
		// The drop: the column lifted in the fingertips, then let go from the bottom one chip at a time,
		// each falling onto the one before.
		const double Lift = DropLift * Smooth(T / 0.32f);
		const float Fall = FallTime();
		for (int32 S = 0; S < 8; ++S)
		{
			const int32 Id = PlayOrder[S];
			NewOrder[S] = Id;
			const float Free = T - (DropFirst + DropEvery * S);
			double H = S * Thick + Lift;
			float Tilt = 0.0f;
			if (Free > 0.0f)
			{
				H = FMath::Max(S * Thick, S * Thick + DropLift - 0.5 * Gravity * Free * Free);
				// A wobble as it falls and lands.
				Tilt = 4.0f * FMath::Sin(Free * 40.0f) * FMath::Exp(-Free * 9.0f) * ((Persona.Seed + S) % 2 == 0 ? 1.0f : -1.0f);
			}
			Out[Id] = Place(Id, 0.0, H, Tilt);
		}
		// The fingertips stay on top of the chips in hand, and come down to the column after the last one.
		const float Last = DropFirst + DropEvery * 7.0f + Fall;
		High = 7.0 * Thick + DropLift * (1.0 - Smooth((T - Last + 0.1f) / 0.25f)) * Smooth(T / 0.32f);
	}
	Top = static_cast<float>(High + Thick);
}

void ABackRoomPlayer::UpdatePlayChips(float Dt, bool bHandOnChips)
{
	if (!HasPlayChips() || IsHidden())
	{
		for (UInstancedStaticMeshComponent* Ism : PlayChips)
		{
			Ism->SetVisibility(false);
		}
		TrickT = -1.0f;
		SettleT = 1.0f;
		TrickTop = static_cast<float>(8.0 * Thick);
		PlayLaidAt = FVector(0.0, 0.0, -1.0e6);
		return;
	}
	if (PlayChips.Num() == 0)
	{
		// Placed in world space, two instanced meshes (one per color) for the eight.
		for (int32 C = 0; C < 2; ++C)
		{
			UInstancedStaticMeshComponent* Ism = NewObject<UInstancedStaticMeshComponent>(this, *FString::Printf(TEXT("PlayChips%d"), C));
			Ism->SetMobility(EComponentMobility::Movable);
			Ism->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Ism->SetUsingAbsoluteLocation(true);
			Ism->SetUsingAbsoluteRotation(true);
			Ism->SetUsingAbsoluteScale(true);
			Ism->bCastContactShadow = true;
			Ism->SetupAttachment(GetRootComponent());
			Ism->RegisterComponent();
			Ism->SetWorldTransform(FTransform::Identity);
			for (int32 I = 0; I < 4; ++I)
			{
				Ism->AddInstance(FTransform::Identity);
			}
			PlayChips.Add(Ism);
		}
	}
	if (PlayChipsSeed != Persona.Seed)
	{
		// Two neighbors from the rack: red and white, green and red, or black and green.
		PlayChipsSeed = Persona.Seed;
		static const TCHAR* Names[4] = {TEXT("100"), TEXT("25"), TEXT("5"), TEXT("1")};
		const int32 Pair = FMath::Abs(Persona.Seed) % 3;
		for (int32 C = 0; C < 2; ++C)
		{
			const TCHAR* Name = Names[2 - Pair + C];
			const FString Path = FString::Printf(TEXT("/Game/ShortStack/Meshes/SM_Chip_%s/SM_Chip_%s.SM_Chip_%s"), Name, Name, Name);
			PlayChips[C]->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet));
		}
		FRandomStream R(Persona.Seed * 31 + 5);
		for (int32 I = 0; I < 8; ++I)
		{
			PlayYaw[I] = R.FRandRange(0.0f, 360.0f);
			PlayOrder[I] = I;
		}
		TrickT = -1.0f;
		SettleT = 1.0f;
		PlayLaidAt = FVector(0.0, 0.0, -1.0e6);
	}
	for (UInstancedStaticMeshComponent* Ism : PlayChips)
	{
		Ism->SetVisibility(true);
	}

	const FVector Base = PlayChipsBase();
	FTransform Want[8];
	int32 NewOrder[8];
	bool bMoving = false;
	if (bHandOnChips)
	{
		if (TrickT < 0.0f)
		{
			TrickT = 0.0f;
			TrickKind = Rng.FRand() < 0.65f ? 0 : 1;
			TrickPace = FMath::Lerp(0.85f, 1.35f, Persona.Nervousness) * Rng.FRandRange(0.9f, 1.1f);
			TrickClicks = 0;
			SettleT = 1.0f;
		}
		TrickT += Dt * TrickPace;
		const float Length = TrickKind == 0 ? RiffleLength : DropLength;
		if (TrickT >= Length)
		{
			// Done: the column stands in its new order; again (now and then the other trick).
			float Top = 0.0f;
			PoseTrick(Length, Want, NewOrder, Top);
			FMemory::Memcpy(PlayOrder, NewOrder, sizeof(PlayOrder));
			TrickT = FMath::Min(TrickT - Length, 0.1f);
			TrickKind = Rng.FRand() < 0.65f ? 0 : 1;
			TrickClicks = 0;
		}
		PoseTrick(TrickT, Want, NewOrder, TrickTop);

		// The click of chips landing: a riffle's halves meeting, each chip of a drop.
		const float Fall = FallTime();
		float Clicks[8];
		float Volumes[8];
		int32 NumClicks = 0;
		if (TrickKind == 0)
		{
			const float At[4] = {0.86f, 0.9f, 0.94f, 1.03f};
			const float Vol[4] = {0.2f, 0.18f, 0.2f, 0.32f};
			for (int32 I = 0; I < 4; ++I)
			{
				Clicks[NumClicks] = At[I];
				Volumes[NumClicks++] = Vol[I];
			}
		}
		else
		{
			for (int32 S = 0; S < 8; ++S)
			{
				Clicks[NumClicks] = DropFirst + DropEvery * S + Fall;
				Volumes[NumClicks++] = 0.22f + 0.02f * S;
			}
		}
		while (TrickClicks < NumClicks && TrickT >= Clicks[TrickClicks])
		{
			Sound(static_cast<int32>(ss::SoundId::Chip), Base, Volumes[TrickClicks]);
			++TrickClicks;
		}

		// The fingers at work: pinching the halves apart, riffling, pushing them together; holding a drop.
		FHandPose& W = Wiggle[1];
		if (TrickKind == 0)
		{
			const float Fan = Window(TrickT, 0.3f, 0.82f);
			W.Pinch = 0.3f * Window(TrickT, 0.0f, 0.3f) + 0.3f * Window(TrickT, 0.8f, 1.05f);
			W.Curl = 0.1f * FMath::Sin(TrickT * 38.0f) * Fan;
			W.Thumb = 0.25f * Fan;
			W.Pos.Z = -0.3 * Window(TrickT, 0.8f, 1.05f);
		}
		else
		{
			const float Held = Window(TrickT, 0.05f, DropFirst + DropEvery * 8.0f);
			W.Pinch = 0.35f * Held;
			W.Thumb = 0.25f * Held;
			W.Curl = 0.05f * FMath::Sin(TrickT * 55.0f) * Window(TrickT, DropFirst, DropFirst + DropEvery * 8.0f);
		}
		bMoving = true;
	}
	else
	{
		if (TrickT >= 0.0f)
		{
			// Cut short: whatever is in the air settles into a column (a riffle that has begun to fan, interleaved).
			FTransform Dummy[8];
			float Top = 0.0f;
			PoseTrick(TrickT, Dummy, NewOrder, Top);
			if (TrickKind == 0 && TrickT >= 0.42f)
			{
				FMemory::Memcpy(PlayOrder, NewOrder, sizeof(PlayOrder));
			}
			for (int32 I = 0; I < 8; ++I)
			{
				SettleFrom[I] = PlayAt[I];
			}
			SettleT = 0.0f;
			TrickT = -1.0f;
		}
		TrickTop = static_cast<float>(8.0 * Thick);
		const float Yaw = GetActorRotation().Yaw;
		for (int32 S = 0; S < 8; ++S)
		{
			const int32 Id = PlayOrder[S];
			Want[Id] = FTransform(FRotator(0.0f, Yaw + PlayYaw[Id], 0.0f), Base + FVector(0.0, 0.0, S * Thick));
		}
		if (SettleT < 1.0f)
		{
			SettleT = FMath::Min(1.0f, SettleT + Dt / 0.22f);
			const float E = Smooth(SettleT);
			for (int32 I = 0; I < 8; ++I)
			{
				Want[I].SetLocation(FMath::Lerp(SettleFrom[I].GetLocation(), Want[I].GetLocation(), static_cast<double>(E)));
				Want[I].SetRotation(FQuat::Slerp(SettleFrom[I].GetRotation(), Want[I].GetRotation(), E));
			}
			bMoving = true;
		}
	}
	if (!bMoving && Base.Equals(PlayLaidAt, 0.01))
	{
		return;
	}
	PlayLaidAt = Base;
	TArray<FTransform, TInlineAllocator<4>> Batch[2];
	for (int32 I = 0; I < 8; ++I)
	{
		PlayAt[I] = Want[I];
		Batch[I / 4].Add(Want[I]);
	}
	for (int32 C = 0; C < 2; ++C)
	{
		PlayChips[C]->BatchUpdateInstancesTransforms(0, TArrayView<const FTransform>(Batch[C]), true, true, true);
	}
}
