#include "BackRoomGameMode.h"
#include "CareerSave.h"

#include "BackRoomCard.h"
#include "BackRoomChips.h"
#include "BackRoomPlayer.h"
#include "BackRoomStage.h"
#include "BackRoomTable.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PointLightComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/PackageName.h"
#include "NightOneAudio.h"
#include "NightOneSaveGame.h"
#include "ShortStack/AI/Profiles.h"
#include "ShortStack/Audio/Synth.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Tournament.h"
#include "ShortStack/UI/FrontEnd.h"

DEFINE_LOG_CATEGORY_STATIC(LogBackRoom, Log, All);

namespace BackRoomGameDetail
{
// A minute at the table is this many on the clock (a night is an hour or so of play).
const double ClockRate = 6.0;
// Energy a live game costs per hour on the clock.
const float EnergyPerHour = 5.0f;
// Dee calls the last hand at a quarter to five; the dryers go off at five.
const double LastHandAt = 4.0 * 60.0 + 45.0;

const TCHAR* WeekdayShort[7] = {TEXT("MON"), TEXT("TUE"), TEXT("WED"), TEXT("THU"), TEXT("FRI"), TEXT("SAT"), TEXT("SUN")};
const TCHAR* WeekdayLong[7] = {TEXT("Monday"), TEXT("Tuesday"), TEXT("Wednesday"), TEXT("Thursday"), TEXT("Friday"), TEXT("Saturday"), TEXT("Sunday")};

float Ease(float X, float Target, float Rate, float Dt)
{
	return FMath::Lerp(X, Target, 1.0f - FMath::Exp(-Rate * Dt));
}

float Smooth(float A, float B, float X)
{
	return FMath::SmoothStep(A, B, X);
}

/** "11:42 PM" for minutes after midnight. */
FString Clock12(double MinutesOfDay)
{
	const int32 M = ((static_cast<int32>(FMath::FloorToDouble(MinutesOfDay)) % 1440) + 1440) % 1440;
	const int32 H24 = M / 60;
	const int32 H12 = H24 % 12 == 0 ? 12 : H24 % 12;
	return FString::Printf(TEXT("%d:%02d %s"), H12, M % 60, H24 < 12 ? TEXT("AM") : TEXT("PM"));
}

int32 Weekday(int32 Day)
{
	return ((Day % 7) + 7) % 7;
}

/** "$1,234" */
FString Dollars(int64 Amount)
{
	return TEXT("$") + FText::AsNumber(FMath::Abs(Amount)).ToString();
}

/** The assembled MetaHuman for a cast member, if backroom_cast.py has built it. */
TSoftClassPtr<AActor> CastBlueprint(const TCHAR* Name)
{
	const FString Package = FString::Printf(TEXT("/Game/ShortStack/Cast/Built/MHC_%s/BP_MHC_%s"), Name, Name);
	if (!FPackageName::DoesPackageExist(Package))
	{
		return nullptr;
	}
	return TSoftClassPtr<AActor>(FSoftObjectPath(FString::Printf(TEXT("%s.BP_MHC_%s_C"), *Package, Name)));
}

FBackRoomTell Tell(EBackRoomTell T, EBackRoomTellMeaning Means, float Reliability, float FalseRate)
{
	FBackRoomTell Out;
	Out.Tell = T;
	Out.Means = Means;
	Out.Reliability = Reliability;
	Out.FalseRate = FalseRate;
	return Out;
}

FString CardText(int32 Card)
{
	static const TCHAR* Ranks[13] = {TEXT("2"), TEXT("3"), TEXT("4"), TEXT("5"), TEXT("6"), TEXT("7"), TEXT("8"), TEXT("9"), TEXT("10"), TEXT("J"), TEXT("Q"), TEXT("K"), TEXT("A")};
	static const TCHAR* Suits[4] = {TEXT("♣"), TEXT("♦"), TEXT("♥"), TEXT("♠")};
	return FString(Ranks[(Card >> 2) % 13]) + Suits[Card & 3];
}

/** One beat of an ECG trace (P wave, the QRS spike, T wave) at phase P in 0..1. */
float Ecg(float P)
{
	auto G = [P](float Mu, float Sigma) { return FMath::Exp(-0.5f * FMath::Square((P - Mu) / Sigma)); };
	return 0.12f * G(0.12f, 0.025f) - 0.14f * G(0.232f, 0.008f) + 1.0f * G(0.25f, 0.009f) - 0.28f * G(0.27f, 0.01f) + 0.22f * G(0.48f, 0.045f);
}
} // namespace BackRoomGameDetail

using namespace BackRoomGameDetail;

// ------------------------------------------------------------------ pawn

ABackRoomPawn::ABackRoomPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	RootComponent = Camera;
	Camera->SetFieldOfView(72.0f);
	Camera->bUsePawnControlRotation = false;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	PeekLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PeekLight"));
	PeekLight->SetupAttachment(Camera);
	PeekLight->SetUsingAbsoluteLocation(true);
	PeekLight->SetIntensityUnits(ELightUnits::Candelas);
	PeekLight->SetIntensity(0.0f);
	PeekLight->SetLightColor(FLinearColor(1.0f, 0.86f, 0.68f));
	PeekLight->SetAttenuationRadius(35.0f);
	PeekLight->SetSourceRadius(4.0f);
	PeekLight->SetCastShadows(false);
	PeekLight->SetVolumetricScatteringIntensity(0.0f);
}

void ABackRoomPawn::BeginPlay()
{
	Super::BeginPlay();
	Seat = GetActorLocation();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
	// A close, shallow lens: what you look at is sharp, the rest of the room falls away a little.
	FPostProcessSettings& P = Camera->PostProcessSettings;
	P.bOverride_DepthOfFieldFstop = true;
	P.bOverride_DepthOfFieldFocalDistance = true;
	P.bOverride_DepthOfFieldSensorWidth = true;
	P.DepthOfFieldSensorWidth = 24.576f;
	P.bOverride_VignetteIntensity = true;
	// The heart's hold on the eyes: color drains and the edges smear as it races.
	P.bOverride_ColorSaturation = true;
	P.bOverride_SceneFringeIntensity = true;
}

void ABackRoomPawn::EndPlay(const EEndPlayReason::Type Reason)
{
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	Super::EndPlay(Reason);
}

ABackRoomGameMode* ABackRoomPawn::GetMode() const
{
	return GetWorld() ? GetWorld()->GetAuthGameMode<ABackRoomGameMode>() : nullptr;
}

ABackRoomTable* ABackRoomPawn::GetTable() const
{
	const ABackRoomGameMode* GM = GetMode();
	return GM ? GM->Table.Get() : nullptr;
}

void ABackRoomPawn::TestExtCam(FVector Pos, FVector At, float Fov, bool bOn)
{
	bExtCam = bOn;
	ExtPos = Pos;
	ExtAt = At;
	ExtFov = Fov;
}

FVector ABackRoomPawn::GetEye() const
{
	return Camera ? Camera->GetComponentLocation() : GetActorLocation();
}

void ABackRoomPawn::HandleInput(float RealDt)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	ABackRoomTable* Table = GetTable();
	ABackRoomGameMode* GM = GetMode();
	ABackRoomPlayer* Me = Table ? Table->GetHeroPlayer() : nullptr;
	if (!PC)
	{
		return;
	}
	// Peek: hold to lift the corners.
	const bool bPeek = bTestPeek || PC->IsInputKeyDown(EKeys::SpaceBar) || PC->IsInputKeyDown(EKeys::LeftMouseButton);
	PeekBlend = Ease(PeekBlend, bPeek ? 1.0f : 0.0f, 6.0f, RealDt);
	if (Me)
	{
		Me->SetHeroPeek(bPeek);
	}
	// The hand is known once the corners are really up (the fingers have taken them), not when the key goes down.
	if (bPeek && Me && Me->GetPeekAmount() > 0.55f && Table)
	{
		Table->HeroPeeked();
	}

	// Steady breathing (Shift): the heart settles. It draws on the same reserve as Focus.
	bSteadying = (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift)) && FocusLeft > 0.02f;
	if (Table)
	{
		Table->SetSteadying(bSteadying);
	}

	// Focus: hold to study a face; it drains, and comes back slowly.
	const bool bFocusing = (bTestFocus || PC->IsInputKeyDown(EKeys::RightMouseButton)) && FocusLeft > 0.02f && !bPeek;
	const float Drain = (bFocusing ? 1.0f / 8.0f : 0.0f) + (bSteadying ? 1.0f / 12.0f : 0.0f);
	FocusLeft = FMath::Clamp(FocusLeft + (Drain > 0.0f ? -Drain * RealDt : RealDt / 14.0f), 0.0f, 1.0f);
	Focus = Ease(Focus, bFocusing ? 1.0f : 0.0f, 5.0f, RealDt);
	const bool bFolded = GM && GM->IsLive() && GM->GetPhase() == EBackRoomPhase::Playing && Table && !Table->IsHeroInHand();
	// P: the table's pace. N, out of the hand: the rest of it at a glance, until you're dealt in again. Neither ever
	// decides anything of yours.
	if (GM && GM->IsLive() && Pressed(PC, EKeys::P))
	{
		GM->CycleTablePace();
	}
	if (bFolded && !bFocusing && Pressed(PC, EKeys::N))
	{
		bSkipHand = true;
	}
	// Q: up from the table, on foot (you're still dealt in).
	if (GM && GM->IsLive() && Pressed(PC, EKeys::Q))
	{
		GM->LiveStandUp();
		return;
	}
	if (bSkipHand && (!bFolded || bFocusing))
	{
		bSkipHand = false;
		Pace = 1.0f;
	}
	const float Out = GM ? GM->FoldedPace() : 1.9f;
	Pace = bSkipHand ? 6.0f : Ease(Pace, bFolded && !bFocusing ? Out : 1.0f, 2.0f, RealDt);
	UGameplayStatics::SetGlobalTimeDilation(this, FMath::Lerp(Pace, 0.45f, Focus));
	if (bFocusing && Table)
	{
		// Whoever is nearest the middle of the view, and stays that way until you look well away.
		const FVector Eye = Camera->GetComponentLocation();
		const FVector Fwd = Camera->GetForwardVector();
		ABackRoomPlayer* Best = nullptr;
		float BestCos = 0.94f;
		for (ABackRoomPlayer* O : Table->GetOpponents())
		{
			const float C = FVector::DotProduct(Fwd, (O->GetEyes() - Eye).GetSafeNormal());
			const bool bCurrent = Studying.Get() == O;
			if (C > (bCurrent ? 0.85f : BestCos) && (C > BestCos || bCurrent))
			{
				Best = O;
				BestCos = C;
			}
		}
		Studying = Best;
	}
	else if (Focus < 0.05f)
	{
		Studying = nullptr;
	}

	// Felted: rebuy from the bankroll (the wheel picks how much) or go home.
	if (GM && GM->GetPhase() == EBackRoomPhase::Busted)
	{
		if (PC->WasInputKeyJustPressed(EKeys::R))
		{
			GM->Reload();
		}
		if (PC->WasInputKeyJustPressed(EKeys::MouseScrollUp) || PC->WasInputKeyJustPressed(EKeys::Up))
		{
			GM->AdjustReload(1);
		}
		if (PC->WasInputKeyJustPressed(EKeys::MouseScrollDown) || PC->WasInputKeyJustPressed(EKeys::Down))
		{
			GM->AdjustReload(-1);
		}
		if (PC->WasInputKeyJustPressed(EKeys::L))
		{
			GM->RequestLeave();
		}
		return;
	}

	// Actions.
	if (Table)
	{
		if (Pressed(PC, EKeys::F))
		{
			Table->HeroFold();
		}
		if (Pressed(PC, EKeys::C))
		{
			Table->HeroCheckCall();
		}
		if (PC->WasInputKeyJustPressed(EKeys::R))
		{
			Table->HeroRaise();
		}
		if (PC->WasInputKeyJustPressed(EKeys::A))
		{
			Table->HeroAllIn();
		}
		if (PC->WasInputKeyJustPressed(EKeys::MouseScrollUp) || PC->WasInputKeyJustPressed(EKeys::Up))
		{
			Table->HeroAdjustRaise(1);
		}
		if (PC->WasInputKeyJustPressed(EKeys::MouseScrollDown) || PC->WasInputKeyJustPressed(EKeys::Down))
		{
			Table->HeroAdjustRaise(-1);
		}
	}
	if (GM && Pressed(PC, EKeys::L))
	{
		GM->RequestLeave();
	}
}

bool ABackRoomPawn::Held(const APlayerController* PC, const FKey& Key) const
{
	return (PC && PC->IsInputKeyDown(Key)) || TestHeld.Contains(Key.GetFName());
}

bool ABackRoomPawn::Pressed(const APlayerController* PC, const FKey& Key) const
{
	return (PC && PC->WasInputKeyJustPressed(Key)) || TestPressed.Contains(Key.GetFName());
}

void ABackRoomPawn::TestHold(const FString& Key, bool bDown)
{
	if (bDown)
	{
		TestHeld.Add(FName(*Key));
	}
	else
	{
		TestHeld.Remove(FName(*Key));
	}
}

void ABackRoomPawn::TestPress(const FString& Key)
{
	TestPressed.Add(FName(*Key));
}

void ABackRoomPawn::TestWalker(float RoomX, float RoomY, float InYaw, float InPitch)
{
	const ABackRoomGameMode* GM = GetMode();
	if (!GM || !GM->Stage)
	{
		return;
	}
	if (!bFreeWalk)
	{
		BeginFreeWalk();
	}
	const FVector At = GM->Stage->RoomToWorld(FVector(RoomX, RoomY, 0.0));
	WalkBase = FVector(At.X, At.Y, GM->Stage->CardRoomFloorZ(At) + 166.0);
	WalkVel = FVector::ZeroVector;
	WalkYaw = InYaw;
	WalkPitch = InPitch;
	Smoothed = FRotator(InPitch, InYaw, 0.0f);
	SetActorLocationAndRotation(WalkBase, Smoothed);
}

void ABackRoomPawn::BeginFreeWalk()
{
	bFreeWalk = true;
	WalkBase = GetActorLocation();
	WalkVel = FVector::ZeroVector;
	WalkYaw = Smoothed.Yaw;
	WalkPitch = FMath::Clamp(Smoothed.Pitch, -40.0f, 20.0f);
	Focus = 0.0f;
	Studying = nullptr;
	PeekBlend = 0.0f;
	bSteadying = false;
	bSkipHand = false;
	Pace = 1.0f;
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
}

void ABackRoomPawn::TickFreeWalk(float RealDt)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	ABackRoomGameMode* GM = GetMode();
	ABackRoomStage* Stage = GM ? GM->Stage.Get() : nullptr;
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	if (PC)
	{
		float Dx = 0.0f, Dy = 0.0f;
		PC->GetInputMouseDelta(Dx, Dy);
		WalkYaw += Dx * 0.9f;
		WalkPitch = FMath::Clamp(WalkPitch + Dy * 0.9f, -55.0f, 40.0f);
	}
	// Where the keys say, at a walk (Shift: in a hurry); the room stops you at its tables, counters and walls.
	double Fwd = 0.0, Right = 0.0;
	Fwd += Held(PC, EKeys::W) || Held(PC, EKeys::Up) ? 1.0 : 0.0;
	Fwd -= Held(PC, EKeys::S) || Held(PC, EKeys::Down) ? 1.0 : 0.0;
	Right += Held(PC, EKeys::D) || Held(PC, EKeys::Right) ? 1.0 : 0.0;
	Right -= Held(PC, EKeys::A) || Held(PC, EKeys::Left) ? 1.0 : 0.0;
	const bool bHurry = Held(PC, EKeys::LeftShift) || Held(PC, EKeys::RightShift);
	const FVector Ahead = FRotator(0.0f, WalkYaw, 0.0f).Vector();
	const FVector Side = FRotator(0.0f, WalkYaw + 90.0f, 0.0f).Vector();
	const FVector Wish = (Ahead * Fwd + Side * Right).GetClampedToMaxSize(1.0) * (bHurry ? 240.0 : 145.0);
	WalkVel = FMath::VInterpTo(WalkVel, Wish, RealDt, 9.0f);
	FVector Next = WalkBase + WalkVel * RealDt;
	if (Stage)
	{
		Next = Stage->CardRoomStep(WalkBase, Next);
		Next.Z = FMath::FInterpTo(WalkBase.Z, Stage->CardRoomFloorZ(Next) + 166.0, RealDt, 8.0f);
	}
	const double Moved = FVector::Dist2D(WalkBase, Next);
	if (RealDt > 0.0f)
	{
		// Stopped by something: the stride stops with you.
		WalkVel = FVector(Next.X - WalkBase.X, Next.Y - WalkBase.Y, 0.0) / RealDt;
	}
	WalkBase = Next;
	WalkDist += static_cast<float>(Moved);
	const float Moving = FMath::Clamp(static_cast<float>(WalkVel.Size2D() / 145.0), 0.0f, 1.0f);
	// A footfall each stride on the carpet, and the head's small bob and sway.
	const int32 Step = FMath::FloorToInt(WalkDist / 66.0f);
	if (Step != StepCount)
	{
		StepCount = Step;
		ABackRoomTable* Table = GetTable();
		if (UNightOneAudio* Sound = Table ? Table->GetAudio() : nullptr; Sound && Moving > 0.3f)
		{
			Sound->PlayEffect(ss::audio::Effect::Step, 0.2f + 0.3f * Moving);
		}
	}
	const float Steps = WalkDist / 66.0f * PI;
	FVector Eye = WalkBase;
	Eye.Z += Moving * (1.4f * FMath::Abs(FMath::Sin(Steps)) - 0.7f);
	const FRotator Look(WalkPitch, WalkYaw, Moving * 0.45f * FMath::Sin(Steps * 0.5f));
	Smoothed = FMath::RInterpTo(Smoothed, Look, RealDt, 16.0f);
	SetActorLocationAndRotation(Eye, Smoothed);
	Camera->SetFieldOfView(74.0f);
	FPostProcessSettings& P = Camera->PostProcessSettings;
	P.DepthOfFieldFocalDistance = 320.0f;
	P.DepthOfFieldFstop = 5.6f;
	P.SceneFringeIntensity = 0.0f;
	PeekLight->SetIntensity(0.0f);
	// E: whatever's in reach (your chair, the desk, the cage, the bar, the deck, the door).
	if (GM && Pressed(PC, EKeys::E))
	{
		GM->LiveInteract();
	}
	float PitchKick = 0.0f, Racing = 0.0f;
	TickHeart(RealDt, PitchKick, Racing);
}

void ABackRoomPawn::PlayWalk(const TArray<FVector>& Points, float Seconds, bool bOut, TFunction<void()> OnDone)
{
	WalkPoints = Points;
	WalkLengths.Reset();
	WalkTotal = 0.0f;
	for (int32 I = 1; I < WalkPoints.Num(); ++I)
	{
		const float L = static_cast<float>(FVector::Dist(WalkPoints[I - 1], WalkPoints[I]));
		WalkLengths.Add(L);
		WalkTotal += L;
	}
	WalkT = 0.0f;
	StepCount = 0;
	WalkSeconds = FMath::Max(Seconds, 0.1f);
	bWalkOut = bOut;
	bBodyShown = bOut;
	WalkDone = MoveTemp(OnDone);
	bWalking = WalkPoints.Num() >= 2 && WalkTotal > 1.0f;
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	Focus = 0.0f;
	Studying = nullptr;
	if (bWalking)
	{
		if (!bOut)
		{
			SetActorLocation(WalkPoints[0]);
			Smoothed = (WalkPoints[1] - WalkPoints[0]).Rotation();
			Smoothed.Pitch = -4.0f;
		}
	}
	else if (WalkDone)
	{
		TFunction<void()> Done = MoveTemp(WalkDone);
		WalkDone = nullptr;
		Done();
	}
}

FVector ABackRoomPawn::WalkAt(float Distance) const
{
	// Catmull-Rom through the points, parameterized by distance along the chords.
	const int32 N = WalkPoints.Num();
	float D = FMath::Clamp(Distance, 0.0f, WalkTotal);
	int32 I = 0;
	while (I < WalkLengths.Num() - 1 && D > WalkLengths[I])
	{
		D -= WalkLengths[I];
		++I;
	}
	const float U = WalkLengths[I] > 0.0f ? FMath::Clamp(D / WalkLengths[I], 0.0f, 1.0f) : 0.0f;
	const FVector P0 = WalkPoints[FMath::Max(I - 1, 0)];
	const FVector P1 = WalkPoints[I];
	const FVector P2 = WalkPoints[I + 1];
	const FVector P3 = WalkPoints[FMath::Min(I + 2, N - 1)];
	const double U1 = U, U2 = U1 * U1, U3 = U2 * U1;
	return 0.5 * ((2.0 * P1) + (P2 - P0) * U1 + (2.0 * P0 - 5.0 * P1 + 4.0 * P2 - P3) * U2 + (3.0 * P1 - P0 - 3.0 * P2 + P3) * U3);
}

void ABackRoomPawn::TickWalk(float RealDt)
{
	WalkT += RealDt;
	const float T = FMath::Clamp(WalkT / WalkSeconds, 0.0f, 1.0f);
	// A walking pace: setting off, an even stride, and slowing to a stop.
	const float E = 0.3f * T + 0.7f * Smooth(0.0f, 1.0f, T);
	const float D = E * WalkTotal;
	FVector At = WalkAt(D);
	const FVector Ahead = WalkAt(D + 80.0f);
	// Each step lifts and drops the head a little and sways it side to side; it fades sitting down
	// (or builds standing up).
	const float Stride = bWalkOut ? Smooth(0.04f, 0.22f, T) : 1.0f - Smooth(0.7f, 0.9f, T);
	const float Steps = D / 66.0f * PI;
	At.Z += Stride * (1.5f * FMath::Abs(FMath::Sin(Steps)) - 0.75f);
	ABackRoomTable* Table = GetTable();
	UNightOneAudio* Sound = Table ? Table->GetAudio() : nullptr;
	// A footfall each stride on the concrete.
	const int32 Step = FMath::FloorToInt(D / 66.0f);
	if (Step != StepCount)
	{
		StepCount = Step;
		if (Sound && Stride > 0.25f)
		{
			Sound->PlayEffect(ss::audio::Effect::Step, 0.3f + 0.45f * Stride);
		}
	}
	const FVector Along = (Ahead - At).GetSafeNormal2D();
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Along.IsNearlyZero() ? FVector::ForwardVector : Along);
	At += Side * (Stride * 0.9f * FMath::Sin(Steps * 0.5f));

	ABackRoomPlayer* Me = Table ? Table->GetHeroPlayer() : nullptr;
	const FVector TableAt(0.0, 0.0, 96.0);
	FRotator Look;
	if (!bWalkOut)
	{
		// Through the door looking where you're going, then at the table and who is at it, then down
		// into the seat, facing Dee.
		const FVector Target = FMath::Lerp(Ahead + FVector(0.0, 0.0, -6.0), TableAt, Smooth(0.1f, 0.4f, T));
		const FQuat Q = FQuat::Slerp((Target - At).Rotation().Quaternion(), FRotator(-14.0f, 0.0f, 0.0f).Quaternion(), Smooth(0.78f, 1.0f, T));
		Look = Q.Rotator();
		if (Me)
		{
			At = FMath::Lerp(At, Me->GetEyes() + FVector(1.5, 0.0, 0.0), static_cast<double>(Smooth(0.86f, 1.0f, T)));
			if (!bBodyShown && T > 0.88f)
			{
				// Pull the chair in and sit.
				Me->SetActorHiddenInGame(false);
				bBodyShown = true;
				if (Sound)
				{
					Sound->PlayEffect(ss::audio::Effect::Scrape, 0.8f);
				}
			}
		}
	}
	else
	{
		// Up out of the chair, one look back at the table on the way round, then the door.
		const float Back = Smooth(0.04f, 0.16f, T) * (1.0f - Smooth(0.4f, 0.58f, T));
		const FVector Target = FMath::Lerp(Ahead + FVector(0.0, 0.0, -4.0), TableAt, static_cast<double>(0.85f * Back));
		Look = (Target - At).Rotation();
		if (Me && bBodyShown && T > 0.05f)
		{
			Me->SetActorHiddenInGame(true);
			bBodyShown = false;
			if (Sound)
			{
				Sound->PlayEffect(ss::audio::Effect::Scrape, 0.9f);
			}
		}
	}
	Look.Roll = Stride * 0.5f * FMath::Sin(Steps * 0.5f);
	Smoothed = FMath::RInterpTo(Smoothed, Look, RealDt, bWalkOut ? 6.0f : 8.0f);
	SetActorLocationAndRotation(At, Smoothed);
	Camera->SetFieldOfView(72.0f);
	FPostProcessSettings& P = Camera->PostProcessSettings;
	P.DepthOfFieldFocalDistance = 220.0f;
	P.DepthOfFieldFstop = 5.6f;
	P.VignetteIntensity = 0.5f;
	P.ColorSaturation = FVector4(1.0, 1.0, 1.0, 1.0);
	P.SceneFringeIntensity = 0.0f;
	PeekLight->SetIntensity(0.0f);
	if (T >= 1.0f)
	{
		bWalking = false;
		if (!bWalkOut)
		{
			Yaw = 0.0f;
			Pitch = -14.0f;
			Smoothed = FRotator(-14.0f, 0.0f, 0.0f);
		}
		TFunction<void()> Done = MoveTemp(WalkDone);
		WalkDone = nullptr;
		if (Done)
		{
			Done();
		}
	}
}

void ABackRoomPawn::TickHeart(float RealDt, float& PitchKick, float& Intensity)
{
	ABackRoomTable* Table = GetTable();
	const float Bpm = Table ? Table->Composure.Bpm : 68.0f;
	// You start to hear it in the high eighties; past ninety it takes the edges of the view.
	const float Audible = FMath::Clamp((Bpm - 86.0f) / 44.0f, 0.0f, 1.0f);
	Intensity = FMath::Clamp((Bpm - 92.0f) / 48.0f, 0.0f, 1.0f);
	BeatPhase += RealDt * Bpm / 60.0f;
	if (BeatPhase >= 1.0f)
	{
		BeatPhase -= FMath::FloorToFloat(BeatPhase);
		if (Audible > 0.0f)
		{
			if (UNightOneAudio* A = Table ? Table->GetAudio() : nullptr)
			{
				A->PlayEffect(ss::audio::Effect::Thump, 0.12f + 0.85f * Audible);
			}
			Kick = 1.0f;
		}
	}
	Kick = FMath::Max(0.0f, Kick - RealDt * 7.0f);
	PitchKick = -0.25f * Kick * Audible;
	Steady = Ease(Steady, bSteadying ? 1.0f : 0.0f, 2.0f, RealDt);
	const float Before = BreathT;
	BreathT += RealDt * (bSteadying ? 0.18f : 0.27f);
	// Steadying, you hear each long breath out.
	if (Steady > 0.4f && FMath::Frac(Before) < 0.5f && FMath::Frac(BreathT) >= 0.5f)
	{
		if (UNightOneAudio* A = Table ? Table->GetAudio() : nullptr)
		{
			A->PlayEffect(ss::audio::Effect::Breath, 0.9f * Steady);
		}
	}
}

void ABackRoomPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Focus slows the world; the head and the meter run on real time.
	const float RealDt = FMath::Min(static_cast<float>(FApp::GetDeltaTime()), 0.1f);
	Time += RealDt;
	if (bWalking)
	{
		TickWalk(RealDt);
		TestPressed.Reset();
		return;
	}
	if (bFreeWalk)
	{
		TickFreeWalk(RealDt);
		TestPressed.Reset();
		return;
	}
	ABackRoomTable* Table = GetTable();
	ABackRoomGameMode* GM = GetMode();
	ABackRoomPlayer* Me = Table ? Table->GetHeroPlayer() : nullptr;
	// Racked up: the hands are off the table; the head still looks around while Dee says goodnight.
	const bool bDone = GM && GM->GetPhase() == EBackRoomPhase::Leaving;
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		float Dx = 0.0f, Dy = 0.0f;
		PC->GetInputMouseDelta(Dx, Dy);
		const float Sens = 0.9f * (1.0f - 0.55f * Focus);
		Yaw = FMath::Clamp(Yaw + Dx * Sens, -MaxYaw, MaxYaw);
		Pitch = FMath::Clamp(Pitch + Dy * Sens, MinPitch, MaxPitch);
	}
	if (!bDone)
	{
		HandleInput(RealDt);
	}
	else
	{
		// Out of it: Q stays to watch the rest (on foot), L heads home now.
		APlayerController* LeavePC = Cast<APlayerController>(GetController());
		if (GM && GM->IsLive() && Pressed(LeavePC, EKeys::Q))
		{
			GM->LiveStay();
		}
		else if (GM && GM->IsLive() && Pressed(LeavePC, EKeys::L))
		{
			GM->LiveLeaveNow();
		}
		bSteadying = false;
		Focus = Ease(Focus, 0.0f, 5.0f, RealDt);
		PeekBlend = Ease(PeekBlend, 0.0f, 6.0f, RealDt);
		if (Me)
		{
			Me->SetHeroPeek(false);
		}
		UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	}
	float PitchKick = 0.0f, Racing = 0.0f;
	TickHeart(RealDt, PitchKick, Racing);

	FVector Eye = Me ? Me->GetEyes() + FVector(1.5, 0.0, 0.0) : Seat;
	// A slow, deep breath while steadying.
	Eye.Z += Steady * 1.1f * FMath::Sin(BreathT * 2.0f * PI);
	FRotator Want(Pitch, Yaw, 0.0f);
	FVector FocusAt = Eye + Want.Vector() * 120.0;
	float PeekLit = 0.0f;
	if (Me && Me->Hole.Num() == 2 && Me->Hole[0] && Me->Hole[1] && PeekBlend > 0.01f)
	{
		// Peeking: you drop down to the table (your chin nearly on the rail) behind the lifted corner and look
		// across at it: low and nearly level, so the lifted face turns to you, the hands framing it either side.
		const FVector Cards = (Me->Hole[0]->GetActorLocation() + Me->Hole[1]->GetActorLocation()) * 0.5;
		const FVector Corners = Me->GetPeekFocus();
		const FVector Near = FMath::Lerp(Cards, Corners, 0.7) + FVector(0.0, 0.0, 0.8);
		Eye = FMath::Lerp(Eye, Near + FVector(-20.0, 2.5 * ABackRoomCard::NearIndexSide(), 15.0), 0.94 * PeekBlend);
		const FRotator ToCards = (Near - Eye).Rotation();
		Want = FMath::Lerp(Want, FRotator(ToCards.Pitch, ToCards.Yaw, 0.0f), 0.95f * PeekBlend);
		FocusAt = Near;
		PeekLight->SetWorldLocation(Near + FVector(-16.0, 0.0, 14.0));
		PeekLit = PeekBlend;
	}
	PeekLight->SetIntensity(4.0f * PeekLit);
	if (ABackRoomPlayer* S = Studying.Get())
	{
		// Focus pulls the view onto the face.
		const FRotator ToFace = (S->GetEyes() - Eye).Rotation();
		Want = FMath::Lerp(Want, FRotator(ToFace.Pitch, ToFace.Yaw, 0.0f), 0.4f * Focus);
		FocusAt = FMath::Lerp(FocusAt, S->GetEyes(), static_cast<double>(Focus));
	}
	Smoothed = FMath::RInterpTo(Smoothed, Want, RealDt, 14.0f);
	// The heart in the view: a kick with each beat, and a fine tremor when it races.
	const float Tremor = 0.12f * Racing;
	const FRotator Shaken = Smoothed + FRotator(PitchKick + Tremor * FMath::Sin(Time * 37.0f) * FMath::Sin(Time * 11.3f), Tremor * FMath::Sin(Time * 29.0f + 1.7f), 0.0f);
	if (Me)
	{
		SetActorLocationAndRotation(Eye, Shaken);
		Me->SetHeroLook(Eye + Smoothed.Vector() * 200.0);
	}
	else
	{
		// No body yet: a head that breathes.
		const float Breath = FMath::Sin(Time * 1.35f);
		SetActorLocationAndRotation(Seat + FVector(0.25 * Breath, 0.0, 0.35 * Breath), Shaken + FRotator(0.2f * Breath, 0.0f, 0.15f * FMath::Sin(Time * 0.37f)));
	}
	if (!Studying.IsValid() && PeekBlend < 0.01f)
	{
		// Without a target the eyes rest where the view meets the felt (or a couple of meters out).
		const FVector Dir = Smoothed.Vector();
		const double Down = -Dir.Z;
		const double ToFelt = Down > 0.05 ? (Eye.Z - ABackRoomStage::FeltZ) / Down : 250.0;
		FocusAt = Eye + Dir * FMath::Clamp(ToFelt, 40.0, 250.0);
	}

	// The lens: Focus narrows it and opens the aperture; a racing heart closes in the edges.
	Camera->SetFieldOfView(FMath::Lerp(FMath::Lerp(72.0f, 42.0f, PeekBlend), 34.0f, Focus) - 4.0f * Racing);
	FPostProcessSettings& P = Camera->PostProcessSettings;
	P.DepthOfFieldFocalDistance = FMath::Max(10.0f, static_cast<float>(FVector::Dist(Eye, FocusAt)));
	P.DepthOfFieldFstop = FMath::Lerp(FMath::Lerp(4.0f, 8.0f, PeekBlend), 1.4f, Focus);
	P.VignetteIntensity = 0.45f + 0.5f * Focus + 0.55f * Racing + 0.18f * Racing * Kick;
	P.ColorSaturation = FVector4(1.0, 1.0, 1.0, 1.0 - 0.38 * Racing);
	P.SceneFringeIntensity = 1.1f * Racing + 0.7f * Racing * Kick;

	if (bExtCam)
	{
		Camera->SetWorldLocationAndRotation(ExtPos, (ExtAt - ExtPos).Rotation());
		Camera->SetFieldOfView(ExtFov);
		Camera->PostProcessSettings.DepthOfFieldFstop = 32.0f;
	}

	// The opponents feel being looked at: more so through Focus.
	if (Table)
	{
		const FVector Fwd = Camera->GetForwardVector();
		const float Lo = FMath::Cos(FMath::DegreesToRadians(9.0f));
		const float Hi = FMath::Cos(FMath::DegreesToRadians(2.5f));
		for (ABackRoomPlayer* O : Table->GetOpponents())
		{
			const float C = FVector::DotProduct(Fwd, (O->GetEyes() - Eye).GetSafeNormal());
			O->SetStudied(bDone ? 0.0f : FMath::Clamp((C - Lo) / (Hi - Lo), 0.0f, 1.0f) * (0.6f + 0.4f * Focus));
		}
	}
	TestPressed.Reset();
}

// ------------------------------------------------------------------ HUD

void ABackRoomHUD::DrawHUD()
{
	Super::DrawHUD();
	const float Dt = FMath::Min(static_cast<float>(FApp::GetDeltaTime()), 0.1f);
	Clock += Dt;
	ABackRoomGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ABackRoomGameMode>() : nullptr;
	ABackRoomTable* Table = GM ? GM->Table.Get() : nullptr;
	if (!Table || !Canvas)
	{
		return;
	}
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const float S = H / 1080.0f;
	UFont* Big = GEngine->GetLargeFont();
	UFont* Small = GEngine->GetMediumFont();
	const FLinearColor Paper(0.93f, 0.9f, 0.82f, 0.92f);
	const FLinearColor Dim(0.93f, 0.9f, 0.82f, 0.55f);
	const FLinearColor Warm(1.0f, 0.78f, 0.45f, 0.95f);
	const FLinearColor Read(0.64f, 0.86f, 0.92f, 0.92f);
	const FLinearColor Blood(1.0f, 0.36f, 0.3f, 0.95f);
	const ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(GetOwningPawn());
	const EBackRoomPhase Phase = GM->GetPhase();

	// Align: 0 left, 1 center, 2 right.
	auto Text = [&](const FString& T, const FLinearColor& Color, float X, float Y, UFont* Font, float Scale, int32 Align) {
		if (Color.A <= 0.005f)
		{
			return 0.0f;
		}
		float Tw = 0.0f, Th = 0.0f;
		GetTextSize(T, Tw, Th, Font, Scale * S);
		const float Px = Align == 1 ? X - Tw * 0.5f : (Align == 2 ? X - Tw : X);
		DrawText(T, FLinearColor(0.0f, 0.0f, 0.0f, Color.A * 0.6f), Px + 2.0f * S, Y + 2.0f * S, Font, Scale * S);
		DrawText(T, Color, Px, Y, Font, Scale * S);
		return Tw;
	};
	auto Fade = [](FLinearColor C, float A) {
		C.A *= FMath::Clamp(A, 0.0f, 1.0f);
		return C;
	};

	// Tired: the lids come down from above and below, soft-edged.
	if (GM->GetEyelids() > 0.01f)
	{
		const float Lid = GM->GetEyelids() * H * 0.5f;
		DrawRect(FLinearColor::Black, 0.0f, 0.0f, W, Lid);
		DrawRect(FLinearColor::Black, 0.0f, H - Lid, W, Lid);
		const float Band = 10.0f * S;
		for (int32 I = 0; I < 7; ++I)
		{
			const FLinearColor Edge(0.0f, 0.0f, 0.0f, 1.0f - (I + 1) / 8.0f);
			DrawRect(Edge, 0.0f, Lid + I * Band, W, Band);
			DrawRect(Edge, 0.0f, H - Lid - (I + 1) * Band, W, Band);
		}
	}

	// Table talk becomes subtitles; what your eye catches becomes a whisper.
	for (const FBackRoomLine& L : Table->TakeLines())
	{
		if (L.bRead)
		{
			Whispers.Add({FString(), L.Text, Clock + 4.8f + 0.045f * L.Text.Len(), Clock});
		}
		else
		{
			Subtitles.Add({L.Speaker, L.Text, Clock + 2.8f + 0.055f * L.Text.Len(), Clock});
		}
	}
	Subtitles.RemoveAll([this](const FShown& Sh) { return Sh.Until < Clock; });
	while (Subtitles.Num() > 3)
	{
		Subtitles.RemoveAt(0);
	}
	float Y = H * 0.74f;
	for (const FShown& Sh : Subtitles)
	{
		const float A = FMath::Clamp(Sh.Until - Clock, 0.0f, 1.0f);
		Text(Sh.Speaker.IsEmpty() ? Sh.Text : Sh.Speaker + TEXT(":  ") + Sh.Text, Fade(Sh.Speaker.IsEmpty() ? Warm : Paper, A), W * 0.5f, Y, Small, 1.35f, 1);
		Y += 30.0f * S;
	}
	Whispers.RemoveAll([this](const FShown& Sh) { return Sh.Until < Clock; });
	while (Whispers.Num() > 3)
	{
		Whispers.RemoveAt(0);
	}
	float Wy = H * 0.15f;
	for (const FShown& Sh : Whispers)
	{
		const float A = FMath::Clamp(Sh.Until - Clock, 0.0f, 1.0f) * FMath::Clamp((Clock - Sh.From) / 0.4f, 0.0f, 1.0f);
		const bool bLearned = Sh.Text.StartsWith(TEXT("READ LEARNED"));
		Text(Sh.Text, Fade(bLearned ? Warm : Read, A), W * 0.5f, Wy, Small, bLearned ? 1.32f : 1.15f, 1);
		Wy += 30.0f * S;
	}

	// Walking in: where you are, and when.
	const float Arrive = GM->GetArrivalTime();
	if (Arrive >= 0.0f)
	{
		const float A = Smooth(0.6f, 1.8f, Arrive) * (1.0f - Smooth(5.0f, 6.4f, Arrive));
		Text(GM->IsLive() ? TEXT("R I V E R S I D E    C A S I N O") : TEXT("S P I N    C Y C L E    L A U N D R O M A T"), Fade(Paper, A), W * 0.5f, H * 0.4f, Big, 1.35f, 1);
		Text(GM->IsLive() ? TEXT("the card room   \u00b7   ") + GM->GetLiveName().ToLower() : FString(TEXT("the back room")), Fade(Warm, A), W * 0.5f, H * 0.4f + 50.0f * S, Small, 1.45f, 1);
		Text(GM->ArrivalDay(), Fade(Dim, A), W * 0.5f, H * 0.4f + 88.0f * S, Small, 1.1f, 1);
	}

	// On foot: what's in reach, and how the night stands while you're up (or, out of it, the offer to stay).
	auto DrawOnFoot = [&]() {
		if (!GM->IsLive())
		{
			return;
		}
		if (Pawn && Pawn->IsFreeWalking())
		{
			const FString Prompt = GM->SpotPrompt();
			if (!Prompt.IsEmpty())
			{
				Text(Prompt, Paper, W * 0.5f, H - 120.0f * S, Small, 1.3f, 1);
			}
			const ss::Tournament* T = GM->GetTourney();
			FString State;
			if (GM->IsRailing())
			{
				State = T && T->bFinished ? FString(TEXT("It's over.   [E] at the door to go home"))
										  : FString::Printf(TEXT("On the rail   \u00b7   %d left   \u00b7   [E] at the door to go home"), T ? T->Remaining : 0);
			}
			else if (T)
			{
				State = FString::Printf(TEXT("Up from table %d   \u00b7   still dealt in: checked when free, mucked when not   \u00b7   [E] at your chair to sit"), T->Hero().TableId);
			}
			Text(State, Fade(Dim, 0.9f), W * 0.5f, H - 78.0f * S, Small, 1.0f, 1);
			Text(TEXT("WASD  walk   \u00b7   Shift  hurry   \u00b7   E  use"), Fade(Dim, 0.6f), W * 0.5f, H - 46.0f * S, Small, 0.9f, 1);
		}
		else if (GM->StayOffer() >= 0.0f)
		{
			const float A = FMath::Clamp((GM->StayOffer() - 1.0f) / 0.6f, 0.0f, 1.0f);
			Text(TEXT("[Q] Stay and watch   \u00b7   [L] Go home now"), Fade(Paper, A), W * 0.5f, H - 70.0f * S, Small, 1.15f, 1);
		}
	};
	// A big moment, across the room's attention.
	auto DrawBanner = [&]() {
		float Age = 0.0f;
		const FString& B = GM->GetBanner(Age);
		const float A = FMath::Clamp(Age / 0.3f, 0.0f, 1.0f) * (1.0f - FMath::Clamp((Age - 3.4f) / 0.8f, 0.0f, 1.0f));
		if (!B.IsEmpty() && A > 0.01f)
		{
			const float By = H * 0.085f;
			DrawRect(FLinearColor(Warm.R, Warm.G, Warm.B, 0.65f * A), W * 0.38f, By - 10.0f * S, W * 0.24f, 2.0f * S);
			Text(B, Fade(Paper, A), W * 0.5f, By, Big, 1.45f, 1);
			DrawRect(FLinearColor(Warm.R, Warm.G, Warm.B, 0.65f * A), W * 0.38f, By + 50.0f * S, W * 0.24f, 2.0f * S);
		}
	};

	// Racked up: the night, totted up, over the walk out.
	if (Phase == EBackRoomPhase::Leaving)
	{
		const float T = GM->GetLeaveTime();
		const TArray<FString>& Lines = GM->GetSummary();
		const float A = Smooth(1.8f, 2.8f, T) * (1.0f - Smooth(29.0f, 30.0f, T));
		if (A > 0.01f && Lines.Num() > 0)
		{
			const float Bw = 560.0f * S;
			const float Bh = (70.0f + 34.0f * Lines.Num()) * S;
			const float Bx = W * 0.5f - Bw * 0.5f, By = H * 0.3f;
			DrawRect(FLinearColor(0.02f, 0.02f, 0.02f, 0.62f * A), Bx, By, Bw, Bh);
			DrawRect(FLinearColor(Warm.R, Warm.G, Warm.B, 0.7f * A), Bx, By, Bw, 2.0f * S);
			float Ly = By + 22.0f * S;
			for (int32 I = 0; I < Lines.Num(); ++I)
			{
				const FString& L = Lines[I];
				if (I == 0)
				{
					Text(L, Fade(Paper, A), W * 0.5f, Ly, Big, 1.2f, 1);
					Ly += 52.0f * S;
					continue;
				}
				const bool bNet = L.StartsWith(TEXT("Net"));
				const bool bRead = L.StartsWith(TEXT("Read"));
				const FLinearColor C = bNet ? (GM->GetNetCents() >= 0 ? FLinearColor(0.55f, 0.92f, 0.6f, 1.0f) : Blood) : (bRead ? Warm : (I == 1 ? Dim : Paper));
				Text(L, Fade(C, A), W * 0.5f, Ly, Small, bNet ? 1.35f : 1.15f, 1);
				Ly += 34.0f * S;
			}
		}
		DrawOnFoot();
		DrawBanner();
		return;
	}
	DrawBanner();
	if ((Pawn && Pawn->IsWalking()) || Phase == EBackRoomPhase::Moving)
	{
		return;
	}

	// Your stack, the pot, and your cards once you've looked.
	const FBackRoomPrompt P = Table->GetPrompt();
	const float Left = 48.0f * S;
	const bool bLive = GM->IsLive();
	// Money as the table counts it: dollars at Dee's, tournament chips at the Riverside.
	auto Amount = [bLive](int64 V) { return bLive ? FText::AsNumber(V).ToString() : FString::Printf(TEXT("$%lld"), V); };
	const float StackW = Text(Amount(P.Stack), Paper, Left, H - 128.0f * S, Big, 1.6f, 0);
	if (bLive && P.BigBlind > 0)
	{
		Text(FString::Printf(TEXT("%.0f BB"), static_cast<double>(P.Stack) / static_cast<double>(P.BigBlind)), Dim, Left + StackW + 14.0f * S, H - 114.0f * S, Small, 1.1f, 0);
	}
	FString Under = TEXT("Pot ") + Amount(P.Pot);
	if (P.Known.Num() == 2)
	{
		Under += TEXT("     ") + CardText(P.Known[0]) + TEXT(" ") + CardText(P.Known[1]);
	}
	Text(Under, Dim, Left, H - 72.0f * S, Small, 1.2f, 0);

	// The heart: a trace that runs with it, bright when it races.
	{
		const float Bpm = Table->Composure.Bpm;
		const int32 Rate = 240;
		const int32 Keep = 2 * Rate;
		TraceAccum += Dt;
		while (TraceAccum >= 1.0f / Rate)
		{
			TraceAccum -= 1.0f / Rate;
			TracePhase += Bpm / 60.0f / Rate;
			TracePhase -= FMath::FloorToFloat(TracePhase);
			Trace.Add(Ecg(TracePhase));
		}
		if (Trace.Num() > Keep)
		{
			Trace.RemoveAt(0, Trace.Num() - Keep);
		}
		HeartAlpha = Ease(HeartAlpha, FMath::Clamp((Bpm - 72.0f) / 18.0f, 0.3f, 1.0f), 3.0f, Dt);
		const float Tw = 190.0f * S, Th = 36.0f * S;
		const float Tx = Left, Ty = H - 206.0f * S;
		const FLinearColor Line = FMath::Lerp(FLinearColor(0.62f, 0.95f, 0.78f, 1.0f), Blood, FMath::Clamp((Bpm - 84.0f) / 30.0f, 0.0f, 1.0f));
		for (int32 I = 1; I < Trace.Num(); ++I)
		{
			const float X0 = Tx + Tw * (I - 1) / (Keep - 1.0f), X1 = Tx + Tw * I / (Keep - 1.0f);
			const float Y0 = Ty + Th * (0.72f - 0.62f * Trace[I - 1]), Y1 = Ty + Th * (0.72f - 0.62f * Trace[I]);
			// Older trace fades out; the line glows a little, like a monitor.
			const float Fresh = FMath::Square(static_cast<float>(I) / Trace.Num());
			DrawLine(X0, Y0, X1, Y1, Fade(Line, HeartAlpha * 0.16f * Fresh), 5.0f * S);
			DrawLine(X0, Y0, X1, Y1, Fade(Line, HeartAlpha * (0.15f + 0.85f * Fresh)), 1.8f * S);
		}
		// The heart itself, kicking with each beat.
		const float Beat = FMath::Clamp(Ecg(TracePhase) * 1.4f, 0.0f, 1.0f);
		Text(TEXT("♥"), Fade(Line, HeartAlpha), Tx + Tw + 14.0f * S, Ty + 4.0f * S - 3.0f * S * Beat, Small, 1.1f + 0.35f * Beat, 0);
		Text(FString::Printf(TEXT("%d"), FMath::RoundToInt(Bpm)), Fade(Line, HeartAlpha), Tx + Tw + 44.0f * S, Ty - 2.0f * S, Big, 1.35f, 0);
		if (Pawn && Pawn->bSteadying)
		{
			Text(TEXT("breathing slow"), Fade(Dim, HeartAlpha), Tx, Ty - 24.0f * S, Small, 0.9f, 0);
		}
	}

	// Your turn: what you can do.
	if (P.bYourTurn && Phase != EBackRoomPhase::Busted && Phase != EBackRoomPhase::Moving && !GM->IsHeroUp())
	{
		const FString Head = P.ToCall > 0 ? Amount(P.ToCall) + TEXT(" to call") : TEXT("Checks to you");
		Text(Head, Warm, W * 0.5f, H - 150.0f * S, Big, 1.25f, 1);
		FString Keys = TEXT("[F] Fold     ");
		Keys += P.bCanCheck ? FString(TEXT("[C] Check")) : TEXT("[C] Call ") + Amount(P.ToCall);
		if (P.bCanRaise)
		{
			Keys += FString::Printf(TEXT("     [R] %s %s     [A] All in"), P.bIsBet ? TEXT("Bet") : TEXT("Raise to"), *Amount(P.RaiseTo));
		}
		Text(Keys, Paper, W * 0.5f, H - 100.0f * S, Small, 1.25f, 1);
		if (P.bCanRaise)
		{
			Text(TEXT("wheel to size"), Dim, W * 0.5f, H - 66.0f * S, Small, 1.0f, 1);
		}
	}

	// Out of the hand at a live table: the room's pace, and the rest of the hand at a glance.
	if (bLive && !P.bYourTurn && Phase == EBackRoomPhase::Playing && !Table->IsHeroInHand() && !GM->IsHeroUp())
	{
		static const TCHAR* PaceNames[3] = {TEXT("LIVE"), TEXT("BRISK"), TEXT("FAST")};
		const bool bSkipping = Pawn && Pawn->IsSkippingHand();
		Text(bSkipping ? FString(TEXT("to the next hand...")) : FString::Printf(TEXT("[N] Next hand     [P] Pace  %s"), PaceNames[FMath::Clamp(GM->GetTablePace(), 0, 2)]),
			Fade(Dim, 0.85f), W * 0.5f, H - 66.0f * S, Small, 1.0f, 1);
	}
	if (bLive && GM->PaceShownAge() < 2.6f)
	{
		static const TCHAR* PaceNotes[3] = {TEXT("TABLE PACE   LIVE   \u00b7   the room's own time"), TEXT("TABLE PACE   BRISK   \u00b7   quicker decisions around you"),
			TEXT("TABLE PACE   FAST   \u00b7   the room acts at once")};
		const float A = FMath::Clamp((2.6f - GM->PaceShownAge()) / 0.5f, 0.0f, 1.0f);
		Text(PaceNotes[FMath::Clamp(GM->GetTablePace(), 0, 2)], Fade(Warm, A), W * 0.5f, H * 0.3f, Small, 1.2f, 1);
		Text(TEXT("your own decisions always wait for you"), Fade(Dim, A), W * 0.5f, H * 0.3f + 30.0f * S, Small, 1.0f, 1);
	}

	// Felted.
	if (Phase == EBackRoomPhase::Busted)
	{
		Text(TEXT("You're felted."), Warm, W * 0.5f, H * 0.42f, Big, 1.4f, 1);
		if (GM->CanReload())
		{
			Text(FString::Printf(TEXT("[R] Buy back in for $%lld     [L] Go home"), GM->GetReloadChips()), Paper, W * 0.5f, H * 0.42f + 58.0f * S, Small, 1.3f, 1);
			Text(FString::Printf(TEXT("wheel to change  ·  %s left in the bankroll"), *Dollars(GM->GetBankrollOffTable() / 100)), Dim, W * 0.5f, H * 0.42f + 96.0f * S, Small, 1.05f, 1);
		}
		else
		{
			Text(TEXT("[L] Go home"), Paper, W * 0.5f, H * 0.42f + 58.0f * S, Small, 1.3f, 1);
			Text(FString::Printf(TEXT("%s left. Not enough to sit."), *Dollars(GM->GetBankrollOffTable() / 100)), Dim, W * 0.5f, H * 0.42f + 96.0f * S, Small, 1.05f, 1);
		}
	}
	else if (GM->IsLeaveRequested())
	{
		Text(TEXT("Racking up after this hand"), Dim, W * 0.5f, H - 190.0f * S, Small, 1.1f, 1);
	}

	// The clock, and how tired you are.
	if (GM->IsCareer())
	{
		Text(GM->ClockLabel(), Dim, W - 48.0f * S, 36.0f * S, Small, 1.15f, 2);
		float Ty = 66.0f * S;
		if (const ss::Tournament* T = GM->GetTourney())
		{
			// The tournament at a glance: the level, the field, the money.
			const ss::Level& L = T->CurrentLevel();
			const int32 Secs = FMath::FloorToInt(GM->LevelTimeLeft());
			auto N = [](int64 V) { return FText::AsNumber(V).ToString(); };
			Text(FString::Printf(TEXT("LEVEL %d   %s / %s%s   %d:%02d"), T->LevelIndex + 1, *N(L.Sb), *N(L.Bb), L.Ante > 0 ? *FString::Printf(TEXT(" (%s)"), *N(L.Ante)) : TEXT(""), Secs / 60, Secs % 60),
				Paper, W - 48.0f * S, Ty, Small, 1.1f, 2);
			Ty += 27.0f * S;
			Text(FString::Printf(TEXT("%d of %d left   \u00b7   avg %s   \u00b7   you're %d%s"), T->Remaining, T->Spec.Entrants, *N(static_cast<int64>(T->AverageStack())), T->HeroRank(),
					 T->HeroRank() % 10 == 1 && T->HeroRank() % 100 != 11 ? TEXT("st") : (T->HeroRank() % 10 == 2 && T->HeroRank() % 100 != 12 ? TEXT("nd") : (T->HeroRank() % 10 == 3 && T->HeroRank() % 100 != 13 ? TEXT("rd") : TEXT("th")))),
				Dim, W - 48.0f * S, Ty, Small, 1.0f, 2);
			Ty += 25.0f * S;
			const int32 Paid = T->PaidPlaces();
			Text(T->InTheMoney() ? FString::Printf(TEXT("in the money   \u00b7   next out gets $%s"), *N(T->PrizeFor(T->Remaining) / 100))
								 : FString::Printf(TEXT("%d paid   \u00b7   min cash $%s   \u00b7   %d to the money"), Paid, *N(T->PrizeFor(Paid) / 100), T->Remaining - Paid),
				T->InTheMoney() ? Fade(FLinearColor(0.55f, 0.92f, 0.6f, 1.0f), 0.85f) : Dim, W - 48.0f * S, Ty, Small, 1.0f, 2);
			Ty += 25.0f * S;
		}
		if (GM->GetEnergy() < 30.0f)
		{
			Text(FString::Printf(TEXT("tired  %d%%"), FMath::RoundToInt(GM->GetEnergy())), Fade(Warm, 0.8f), W - 48.0f * S, Ty, Small, 1.0f, 2);
		}
	}
	if (GM->IsQuitPending())
	{
		Text(TEXT("[L] again to walk away   \u00b7   your stack will be blinded off"), Warm, W * 0.5f, H - 190.0f * S, Small, 1.15f, 1);
	}

	DrawOnFoot();

	// Who you're studying: their name, and who they are to you.
	if (Pawn && Pawn->Focus > 0.35f && GM->IsLive())
	{
		if (ABackRoomPlayer* Studied = Pawn->Studying.Get())
		{
			const float A = FMath::Clamp((Pawn->Focus - 0.35f) / 0.4f, 0.0f, 1.0f);
			const FString Note = GM->FaceNote(Studied->Persona.Name);
			Text(Studied->Persona.Name.ToUpper(), Fade(Paper, A), W * 0.5f, H * 0.6f, Small, 1.3f, 1);
			if (!Note.IsEmpty())
			{
				Text(Note, Fade(Warm, 0.9f * A), W * 0.5f, H * 0.6f + 32.0f * S, Small, 1.0f, 1);
			}
		}
	}

	// Focus left, when it isn't full.
	if (Pawn && (Pawn->FocusLeft < 0.995f || Pawn->Focus > 0.01f))
	{
		const float Bw = 160.0f * S, Bh = 4.0f * S;
		const float Bx = W - Bw - 48.0f * S, By = H - 60.0f * S;
		DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.35f), Bx, By, Bw, Bh);
		DrawRect(FLinearColor(Warm.R, Warm.G, Warm.B, 0.85f), Bx, By, Bw * Pawn->FocusLeft, Bh);
		Text(TEXT("FOCUS"), Dim, Bx, By - 26.0f * S, Small, 0.9f, 0);
	}

	// How to play: early on the first night (or holding Tab).
	const bool bTab = PlayerOwner && PlayerOwner->IsInputKeyDown(EKeys::Tab);
	if (bTab)
	{
		// The read book: what you have seen of each of them, and what the cards said it meant.
		const float Rx = 48.0f * S;
		float Ry = H * 0.2f;
		Text(TEXT("R E A D S"), Warm, Rx, Ry, Small, 1.15f, 0);
		Ry += 34.0f * S;
		int32 Shown = 0;
		for (ABackRoomPlayer* O : Table->GetOpponents())
		{
			for (const FBackRoomTell& T : O->Persona.Tells)
			{
				const int32 N = Table->Reads.FindRef(ABackRoomTable::ReadKey(O, static_cast<uint8>(T.Tell)));
				if (N <= 0)
				{
					continue;
				}
				const TCHAR* Means = T.Means == EBackRoomTellMeaning::Strong ? TEXT("strength") : (T.Means == EBackRoomTellMeaning::Weak ? TEXT("weakness") : TEXT("a bluff"));
				const FString Line = N >= 2 ? FString::Printf(TEXT("%s:  %s means %s"), *O->Persona.Name, *ABackRoomTable::TellPhrase(static_cast<uint8>(T.Tell)), Means)
											: FString::Printf(TEXT("%s:  %s  (%s? seen once)"), *O->Persona.Name, *ABackRoomTable::TellPhrase(static_cast<uint8>(T.Tell)), Means);
				Text(Line, N >= 2 ? Paper : Dim, Rx, Ry, Small, 1.05f, 0);
				Ry += 27.0f * S;
				++Shown;
			}
		}
		if (Shown == 0)
		{
			Text(TEXT("Nothing yet. Study a face (right mouse) while the hand plays out,"), Dim, Rx, Ry, Small, 1.0f, 0);
			Text(TEXT("then see what they turn over."), Dim, Rx, Ry + 25.0f * S, Small, 1.0f, 0);
		}
	}
	const float HintA = bTab ? 1.0f : (GM->IsFirstVisit() && Table->GetHandNumber() <= 3 ? 0.7f : 0.0f);
	if (HintA > 0.0f)
	{
		const float Hx = W - 48.0f * S;
		float Hy = H - 250.0f * S;
		TArray<const TCHAR*> Hints = {TEXT("Space  look at your cards"), TEXT("Right mouse  study a face"), TEXT("Shift  breathe, slow the heart"), TEXT("Tab  your reads")};
		if (GM->IsCareer())
		{
			Hints.Add(GM->IsLive() ? TEXT("L  walk away (blinded off)") : TEXT("L  rack up and go home"));
		}
		if (GM->IsLive())
		{
			Hints.Add(TEXT("N  next hand (once you've folded)"));
			Hints.Add(TEXT("P  table pace"));
		}
		for (const TCHAR* Hint : Hints)
		{
			Text(Hint, Fade(Dim, HintA), Hx, Hy, Small, 0.95f, 2);
			Hy += 24.0f * S;
		}
	}
}

// ------------------------------------------------------------------ game mode

ABackRoomGameMode::ABackRoomGameMode()
{
	DefaultPawnClass = ABackRoomPawn::StaticClass();
	HUDClass = ABackRoomHUD::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

ABackRoomGameMode::~ABackRoomGameMode() = default;

void ABackRoomGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
	// The night's last checkpoint lands before the next scene (or the desktop) reads the save.
	if (LiveSaver)
	{
		LiveSaver->Flush();
	}
	Super::EndPlay(Reason);
}

ABackRoomStage* ABackRoomGameMode::FindOrSpawnStage()
{
	// (The player is placed before StartPlay knows tonight's venue: a stage for the wrong one is replaced.)
	if (Stage && Stage->bCardRoom == bLive)
	{
		return Stage;
	}
	Stage = nullptr;
	for (TActorIterator<ABackRoomStage> It(GetWorld()); It; ++It)
	{
		if (It->bCardRoom == bLive)
		{
			Stage = *It;
			return Stage;
		}
		// The map's Back Room makes way for the Riverside (or the other way round).
		It->Destroy();
	}
	Stage = GetWorld()->SpawnActorDeferred<ABackRoomStage>(ABackRoomStage::StaticClass(), FTransform::Identity);
	Stage->bCardRoom = bLive;
	Stage->FinishSpawning(FTransform::Identity);
	return Stage;
}

void ABackRoomGameMode::RestartPlayer(AController* NewPlayer)
{
	if (!NewPlayer || NewPlayer->IsPendingKillPending())
	{
		return;
	}
	ABackRoomStage* S = FindOrSpawnStage();
	const FVector Eye = S ? S->EyeLocation() : FVector(-98.0, 0.0, 124.0);
	APawn* Pawn = SpawnDefaultPawnAtTransform(NewPlayer, FTransform(FRotator(-14.0f, 0.0f, 0.0f), Eye));
	if (Pawn)
	{
		NewPlayer->SetPawn(Pawn);
		NewPlayer->Possess(Pawn);
		NewPlayer->ClientSetRotation(Pawn->GetActorRotation(), true);
		FinishRestartPlayer(NewPlayer, Pawn->GetActorRotation());
	}
}

void ABackRoomGameMode::StartPlay()
{
	Super::StartPlay();
	ABackRoomChips::ChipUnit = 1;
	bLive = LoadLive();
	FindOrSpawnStage();
	const bool bCareer = bLive || LoadCareer();
	SeatEveryone();
	// The player's volumes (and whether to keep playing behind another window), as set in the apartment's menus.
	{
		ss::ui::GameSettings Settings;
		if (UGameplayStatics::DoesSaveGameExist(UNightOneSaveGame::SettingsSlotName(), 0))
		{
			if (UNightOneSaveGame* Obj = Cast<UNightOneSaveGame>(UGameplayStatics::LoadGameFromSlot(UNightOneSaveGame::SettingsSlotName(), 0)))
			{
				ss::ui::GameSettings::Parse(std::string(TCHAR_TO_UTF8(*Obj->Data)), Settings);
			}
		}
		if (UNightOneAudio* A = Table ? Table->GetAudio() : nullptr)
		{
			A->SetMix(Settings.MasterVolume / 100.0f, Settings.EffectsVolume / 100.0f, Settings.AmbienceVolume / 100.0f);
		}
		FApp::SetUnfocusedVolumeMultiplier(Settings.BackgroundAudio ? 1.0f : 0.0f);
		FrameBudget.Configure(Settings.DynamicTarget, Settings.ResolutionScale);
		TablePace = FMath::Clamp(Settings.TablePace, 0, 2);
		ApplyTablePace();
	}
	if (bCareer)
	{
		BeginArrival();
	}
	else
	{
		Phase = EBackRoomPhase::Practice;
		Table->Begin(4.0f);
	}
}

void ABackRoomGameMode::ApplyTablePace()
{
	if (Table)
	{
		Table->ThinkScale = TablePace == 2 ? 0.35f : (TablePace == 1 ? 0.6f : 1.0f);
	}
}

void ABackRoomGameMode::CycleTablePace()
{
	TablePace = (TablePace + 1) % 3;
	ApplyTablePace();
	PaceShownAt = FPlatformTime::Seconds();
	// Kept with the rest of the player's settings (the apartment's menus show it too).
	ss::ui::GameSettings Settings;
	if (UGameplayStatics::DoesSaveGameExist(UNightOneSaveGame::SettingsSlotName(), 0))
	{
		if (UNightOneSaveGame* Obj = Cast<UNightOneSaveGame>(UGameplayStatics::LoadGameFromSlot(UNightOneSaveGame::SettingsSlotName(), 0)))
		{
			ss::ui::GameSettings::Parse(std::string(TCHAR_TO_UTF8(*Obj->Data)), Settings);
		}
	}
	Settings.TablePace = TablePace;
	if (UNightOneSaveGame* Obj = Cast<UNightOneSaveGame>(UGameplayStatics::CreateSaveGameObject(UNightOneSaveGame::StaticClass())))
	{
		Obj->Data = FString(UTF8_TO_TCHAR(Settings.Serialize().c_str()));
		UGameplayStatics::SaveGameToSlot(Obj, UNightOneSaveGame::SettingsSlotName(), 0);
	}
}

// ------------------------------------------------------------------ the career

bool ABackRoomGameMode::LoadCareer()
{
	const FString BuyIn = UGameplayStatics::ParseOption(OptionsString, TEXT("BuyIn"));
	if (BuyIn.IsEmpty())
	{
		return false;
	}
	std::string CareerText;
	TSharedPtr<ss::SaveData> D = MakeShared<ss::SaveData>();
	if (!CareerSave::LoadText(CareerText) || !ss::SaveData::Parse(CareerText, *D))
	{
		UE_LOG(LogBackRoom, Warning, TEXT("BuyIn given but no career save to play from: practice table."));
		return false;
	}
	const int64 Want = FMath::Clamp<int64>(FCString::Atoi64(*BuyIn), ss::life::GameMinBuyInCents, ss::life::GameMaxBuyInCents);
	const int64 Chips = FMath::Min<int64>(Want, D->BankrollCents) / 100;
	if (Chips * 100 < ss::life::GameMinBuyInCents)
	{
		UE_LOG(LogBackRoom, Warning, TEXT("Bankroll %lld cents can't cover the game: practice table."), static_cast<int64>(D->BankrollCents));
		return false;
	}
	Save = D;
	HeroBuyInChips = Chips;
	StartBankrollCents = D->BankrollCents;
	BoughtInCents = Chips * 100;
	BaseCents = StartBankrollCents - BoughtInCents;
	LeftHomeAt = ss::net::MinutesPerDay * ss::net::NightOneDay + D->ClockMinutes;
	// Down the stairs, across the street in the rain, through the laundromat.
	Minutes = LeftHomeAt + 4.0;
	Energy = static_cast<float>(D->Life.Energy);
	PastNights = D->Life.BackRoomNights;
	PastNetCents = D->Life.BackRoomNetCents;
	bFirstVisit = PastNights == 0;
	ReloadChips = Chips;
	Phase = EBackRoomPhase::Arriving;
	ArrivalDayText = FString::Printf(TEXT("%s   %s"), WeekdayLong[Weekday(FMath::FloorToInt(Minutes / ss::net::MinutesPerDay))], *Clock12(TimeOfDay()));
	UE_LOG(LogBackRoom, Log, TEXT("Career night: bankroll %lld cents, buy-in %lld chips, night %d, energy %.0f, %s"), StartBankrollCents, Chips, PastNights + 1,
		Energy, *ClockLabel());
	return true;
}

FString ABackRoomGameMode::DescribeNight() const
{
	static const TCHAR* Phases[] = {TEXT("practice"), TEXT("arriving"), TEXT("playing"), TEXT("busted"), TEXT("leaving"), TEXT("moving")};
	return FString::Printf(TEXT("%s %s energy %.1f base %lld bought %lld stack %lld hands %d%s%s"), Phases[static_cast<int32>(Phase)], *ClockLabel(), Energy, BaseCents,
		BoughtInCents, Table ? Table->GetHeroStack() : 0, HandsPlayed, bLeaveAsked ? TEXT(" LEAVING") : TEXT(""), bLastHandCalled ? TEXT(" LAST-HAND") : TEXT(""));
}

void ABackRoomGameMode::TestAdvanceClock(float ClockMinutes, float EnergySpent)
{
	Minutes += ClockMinutes;
	Energy = FMath::Clamp(Energy - EnergySpent, 0.0f, 100.0f);
}

double ABackRoomGameMode::TimeOfDay() const
{
	return FMath::Fmod(FMath::Fmod(Minutes, ss::net::MinutesPerDay) + ss::net::MinutesPerDay, ss::net::MinutesPerDay);
}

int32 ABackRoomGameMode::NightWeekday() const
{
	// Past midnight the night still belongs to the day the doors opened.
	int32 Day = FMath::FloorToInt(Minutes / ss::net::MinutesPerDay);
	if (TimeOfDay() < 12.0 * 60.0)
	{
		--Day;
	}
	return Weekday(Day);
}

FString ABackRoomGameMode::ClockLabel() const
{
	return FString(WeekdayShort[Weekday(FMath::FloorToInt(Minutes / ss::net::MinutesPerDay))]) + TEXT("  ") + Clock12(TimeOfDay());
}

void ABackRoomGameMode::SaveCareer(bool bFinal)
{
	if (!Save.IsValid() || !Table)
	{
		return;
	}
	ss::SaveData D = *Save;
	D.BankrollCents = BaseCents + Table->GetHeroStack() * 100;
	// The walk home is a few minutes more.
	D.ClockMinutes = Minutes - ss::net::MinutesPerDay * ss::net::NightOneDay + (bFinal ? 4.0 : 0.0);
	D.Life.Energy = FMath::Clamp(static_cast<double>(Energy), 0.0, 100.0);
	for (const TPair<FString, int32>& R : Table->Reads)
	{
		D.Life.Reads[std::string(TCHAR_TO_UTF8(*R.Key))] = R.Value;
	}
	if (bFinal)
	{
		const int64 Net = D.BankrollCents - StartBankrollCents;
		D.Life.Record(Minutes, "Dee's game", Net, 0);
		D.Life.BackRoomNights += 1;
		D.Life.BackRoomNetCents += Net;
		// The living world hears about the night: Dee's regulars remember it.
		D.NoteBackRoom(Minutes, {"Sal", "Big Lou", "Twitch", "Mei"}, Net);
		// Settled: a second save can't count the night twice.
		*Save = D;
		StartBankrollCents = D.BankrollCents;
		BaseCents = D.BankrollCents - Table->GetHeroStack() * 100;
	}
	CareerSave::SaveNow(D.Serialize());
}

void ABackRoomGameMode::OnTableNote(uint8 Note)
{
	if (bLive)
	{
		LiveNote(Note);
		return;
	}
	switch (static_cast<EBackRoomTableNote>(Note))
	{
	case EBackRoomTableNote::HandEnded:
		++HandsPlayed;
		if (IsCareer() && Phase != EBackRoomPhase::Leaving)
		{
			SaveCareer(false);
		}
		break;
	case EBackRoomTableNote::HeroBusted:
		if (!IsCareer() || Phase == EBackRoomPhase::Leaving)
		{
			break;
		}
		Phase = EBackRoomPhase::Busted;
		ReloadChips = FMath::Clamp<int64>((FMath::Min<int64>(HeroBuyInChips, BaseCents / 100) / 20) * 20, 40, 200);
		if (bLastHandCalled)
		{
			RequestLeave();
		}
		else
		{
			Table->DealerLine(CanReload() ? TEXT("You want back in, or you calling it a night?") : TEXT("That's all you brought, huh? Go home and sleep, kid."));
		}
		SaveCareer(false);
		break;
	case EBackRoomTableNote::HeroLeft:
		BeginLeaving();
		break;
	}
}

bool ABackRoomGameMode::CanReload() const
{
	return Phase == EBackRoomPhase::Busted && !bLeaveAsked && BaseCents >= ss::life::GameMinBuyInCents;
}

void ABackRoomGameMode::AdjustReload(int32 Steps)
{
	const int64 Max = FMath::Max<int64>(40, FMath::Min<int64>(200, BaseCents / 100));
	ReloadChips = FMath::Clamp<int64>(ReloadChips + Steps * 20, 40, Max);
}

void ABackRoomGameMode::Reload()
{
	if (!CanReload() || !Table)
	{
		return;
	}
	const int64 Chips = FMath::Min<int64>(ReloadChips, BaseCents / 100);
	BaseCents -= Chips * 100;
	BoughtInCents += Chips * 100;
	Phase = EBackRoomPhase::Playing;
	Table->HeroReload(Chips);
	SaveCareer(false);
}

void ABackRoomGameMode::RequestLeave()
{
	if (bLive)
	{
		LiveRequestLeave();
		return;
	}
	if (!IsCareer() || !Table || Phase == EBackRoomPhase::Leaving || Phase == EBackRoomPhase::Arriving)
	{
		return;
	}
	if (!bLeaveAsked)
	{
		bLeaveAsked = true;
		if (Phase == EBackRoomPhase::Playing && !bLastHandCalled && !bSentHome)
		{
			Table->DealerLine(TEXT("Alright. Racking you up after this one."));
		}
	}
	Table->RequestLeave(bLastHandCalled);
}

void ABackRoomGameMode::BeginArrival()
{
	ArrivalT = 0.0f;
	ArrivalBeat = 0;
	if (Table)
	{
		Table->StartRoomTone();
	}
	if (Hero)
	{
		// The body waits in the chair, unseen, until the camera sits down into it.
		Hero->SetActorHiddenInGame(true);
	}
	StartWalkIn(Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0)));
}

void ABackRoomGameMode::StartWalkIn(ABackRoomPawn* Pawn)
{
	if (ArrivalBeat != 0 || !Pawn)
	{
		return;
	}
	// In from the laundromat's white light, round the open steel door, behind Twitch, to the seat.
	ArrivalBeat = 1;
	ArrivalT = 0.0f;
	const FVector Eye = Stage ? Stage->EyeLocation() : FVector(-91.0, 0.0, 118.0);
	// Back after the game closed: a few steps up the aisle to the chair, not the whole room.
	const bool bShort = bLive && bBackInRoom && Stage;
	const TArray<FVector> Path = bShort ? Stage->CardRoomMoveIn(Eye, Stage->GetRoomAnchor()) : bLive && Stage ? Stage->CardRoomWalkIn(Eye) : TArray<FVector>{FVector(166.0, 540.0, 166.0), FVector(174.0, 360.0, 166.0), FVector(194.0, 258.0, 165.0), FVector(168.0, 176.0, 165.0),
		FVector(40.0, 210.0, 165.0), FVector(-90.0, 228.0, 164.0), FVector(-196.0, 168.0, 163.0), FVector(-206.0, 50.0, 161.0), FVector(-160.0, 6.0, 148.0), Eye};
	TWeakObjectPtr<ABackRoomGameMode> Self = this;
	Pawn->PlayWalk(Path, bShort ? 3.6f : 9.5f, false, [Self]() {
		if (ABackRoomGameMode* GM = Self.Get())
		{
			GM->Phase = EBackRoomPhase::Playing;
			GM->Table->Begin(1.4f);
		}
	});
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 1.8f, FLinearColor::Black, true, false);
		}
	}
}

void ABackRoomGameMode::BeginLeaving()
{
	if (!IsCareer() || Phase == EBackRoomPhase::Leaving || !Table)
	{
		return;
	}
	const int64 Stack = Table->GetHeroStack();
	bBustedOut = Stack <= 0;
	NetCents = BaseCents + Stack * 100 - StartBankrollCents;
	const int64 BoughtIn = BoughtInCents;
	// Which reads became yours tonight (seen twice and confirmed by the cards).
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
	SaveCareer(true);
	Phase = EBackRoomPhase::Leaving;
	LeaveT = 0.0f;
	Summary.Reset();
	Summary.Add(FString::Printf(TEXT("THE %s GAME"), *FString(WeekdayLong[NightWeekday()]).ToUpper()));
	const int32 Sat = FMath::Max(0, FMath::FloorToInt(Minutes - LeftHomeAt - 4.0));
	const FString Stayed = Sat >= 60 ? FString::Printf(TEXT("%dh %02dm"), Sat / 60, Sat % 60) : FString::Printf(TEXT("%d min"), Sat);
	Summary.Add(FString::Printf(TEXT("%s   ·   %d %s   ·   %s at the table"), *Clock12(TimeOfDay()), HandsPlayed, HandsPlayed == 1 ? TEXT("hand") : TEXT("hands"), *Stayed));
	Summary.Add(FString::Printf(TEXT("Bought in   %s"), *Dollars(BoughtIn / 100)));
	Summary.Add(FString::Printf(TEXT("Cashed out   %s"), *Dollars(Stack)));
	Summary.Add(FString::Printf(TEXT("Net   %s%s"), NetCents >= 0 ? TEXT("+") : TEXT("-"), *Dollars(NetCents / 100)));
	Summary.Append(Learned);
	Table->DealerLine(GoodbyeLine());
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	UE_LOG(LogBackRoom, Log, TEXT("Leaving Dee's game: net %lld cents, %d hands, %d reads learned%s%s"), NetCents, HandsPlayed, Learned.Num(),
		bBustedOut ? TEXT(", busted") : TEXT(""), bClosed ? TEXT(", closed") : TEXT(""));
}

void ABackRoomGameMode::GoHome()
{
	if (bGoingHome)
	{
		return;
	}
	bGoingHome = true;
	FString Options = FString::Printf(TEXT("Home?Net=%lld?Learned=%d?From=%.0f"), NetCents, Table ? Table->LearnedThisNight : 0, LeftHomeAt);
	if (bBustedOut)
	{
		Options += TEXT("?Busted");
	}
	if (bClosed)
	{
		Options += TEXT("?Closed");
	}
	UGameplayStatics::OpenLevel(this, FName(TEXT("NightOne")), true, Options);
}

FString ABackRoomGameMode::ArrivalLine() const
{
	const double Tod = TimeOfDay();
	const bool bLate = Tod >= 60.0 && Tod < 12.0 * 60.0;
	const ss::life::State& L = Save->Life;
	const double ToRent = L.RentDeadline - Minutes;
	const bool bRentSoon = (L.RentStage == ss::life::Rent::Due || L.RentStage == ss::life::Rent::FinalNotice) && ToRent > 0.0 && ToRent < 2.0 * ss::net::MinutesPerDay;
	if (bFirstVisit)
	{
		return TEXT("There's the kid from across the street. Sit, sit. Forty to two hundred, no phones, no crying.");
	}
	if (bLate)
	{
		return TEXT("Little late, kid. Sit down before Lou eats your chair.");
	}
	if (bRentSoon)
	{
		return TEXT("Heard rent's due. Don't play scared, kid. Scared money don't make money.");
	}
	if (PastNetCents >= 10000)
	{
		return TEXT("Look who's back. Lou's been practicing his poker face. Didn't help.");
	}
	if (PastNetCents < -5000)
	{
		return TEXT("Back for more. I like that. Sit.");
	}
	return TEXT("Kid. Your seat's still warm.");
}

FString ABackRoomGameMode::GreetingLine(FString& Who) const
{
	if (bFirstVisit)
	{
		Who = TEXT("Big Lou");
		return TEXT("Fresh money! Sit down, sit down!");
	}
	if (PastNetCents >= 10000)
	{
		Who = TEXT("Sal");
		return TEXT("You again. Lucky kid.");
	}
	if (PastNetCents < 0)
	{
		Who = TEXT("Twitch");
		return TEXT("Yo, it's my ATM! Sit down, man.");
	}
	Who = TEXT("Mei");
	return TEXT("Evening.");
}

FString ABackRoomGameMode::GoodbyeLine() const
{
	const int32 Day = NightWeekday();
	const TCHAR* Next = Day == 1 ? TEXT("Thursday") : (Day == 3 ? TEXT("Saturday") : TEXT("Tuesday"));
	if (bClosed)
	{
		return TEXT("That's the game, folks. Dryers go off at five. Get home safe.");
	}
	if (bSentHome)
	{
		return FString::Printf(TEXT("Go sleep, kid. %s, nine o'clock."), Next);
	}
	if (bBustedOut)
	{
		return FString::Printf(TEXT("Go home, kid. Sleep. Game's back on %s."), Next);
	}
	if (NetCents >= 5000)
	{
		return FString::Printf(TEXT("Taking my regulars' money home? Come back %s so they can win it back."), Next);
	}
	if (NetCents < 0)
	{
		return FString::Printf(TEXT("Night, kid. Watch more, play less. %s, nine o'clock."), Next);
	}
	return FString::Printf(TEXT("Night, kid. %s, nine o'clock."), Next);
}

void ABackRoomGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	FrameBudget.Tick(DeltaSeconds);
	const float RealDt = FMath::Min(static_cast<float>(FApp::GetDeltaTime()), 0.1f);
	if (!IsCareer() || !Table)
	{
		return;
	}
	if (bLive)
	{
		LiveTick(RealDt);
		return;
	}
	ABackRoomPawn* Pawn = Cast<ABackRoomPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	TWeakObjectPtr<ABackRoomGameMode> Self = this;

	// The room turns to look at whoever is walking through it.
	auto Watch = [this, Pawn](float Dt) {
		AttentionTick -= Dt;
		if (!Pawn || AttentionTick > 0.0f)
		{
			return;
		}
		AttentionTick = 0.45f;
		for (ABackRoomPlayer* O : Opponents)
		{
			O->OnOtherAction(Pawn->GetEye(), 0.25f, false);
		}
		if (Dealer)
		{
			Dealer->OnOtherAction(Pawn->GetEye(), 0.25f, false);
		}
	};

	ArrivalT += RealDt;
	if (Phase == EBackRoomPhase::Arriving)
	{
		StartWalkIn(Pawn);
		if (ArrivalBeat == 1 && ArrivalT > 2.6f)
		{
			ArrivalBeat = 2;
			Table->DealerLine(ArrivalLine());
		}
		if (ArrivalBeat == 2 && ArrivalT > 5.4f)
		{
			ArrivalBeat = 3;
			FString Who;
			const FString Line = GreetingLine(Who);
			for (ABackRoomPlayer* O : Opponents)
			{
				if (O && O->Persona.Name == Who)
				{
					O->Say(Line);
				}
			}
		}
		Watch(RealDt);
	}

	// The night goes on: the clock, and what it takes out of you.
	if (Phase == EBackRoomPhase::Playing || Phase == EBackRoomPhase::Busted)
	{
		const double Elapsed = RealDt * ClockRate / 60.0;
		Minutes += Elapsed;
		Energy = FMath::Max(0.0f, Energy - static_cast<float>(Elapsed / 60.0) * EnergyPerHour);
		const double Tod = TimeOfDay();
		if (!bLastHandCalled && Tod >= LastHandAt && Tod < 12.0 * 60.0)
		{
			bLastHandCalled = true;
			bClosed = true;
			Table->DealerLine(TEXT("Last hand, everybody. Dryers go off at five."));
			RequestLeave();
		}
		if (!bSentHome && !bLeaveAsked && Energy <= 4.0f)
		{
			bSentHome = true;
			Table->DealerLine(TEXT("Kid, you're falling asleep at my table. This one, then go home."));
			RequestLeave();
		}
		// Tired: the lids get heavy, more often and for longer the emptier you are.
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
		// Closing slowly, snapping open.
		DroopT += RealDt;
		const float U = DroopT / DroopLength;
		Eyelids = DroopDepth * (U < 0.7f ? Smooth(0.0f, 0.7f, U) : 1.0f - Smooth(0.7f, 1.0f, U));
		if (U >= 1.0f)
		{
			DroopT = -1.0f;
			Eyelids = 0.0f;
		}
	}

	// Racked up: Dee says goodnight, then up and out the way you came, into the white light.
	if (Phase == EBackRoomPhase::Leaving)
	{
		LeaveT += RealDt;
		Eyelids = 0.0f;
		if (!bWalkingOut && LeaveT > 2.4f)
		{
			bWalkingOut = true;
			if (Pawn)
			{
				const FVector Eye = Pawn->GetEye();
				const TArray<FVector> Path = {Eye, Eye + FVector(-34.0, 16.0, 44.0), FVector(-196.0, 84.0, 166.0), FVector(-150.0, 225.0, 166.0),
					FVector(30.0, 214.0, 166.0), FVector(168.0, 180.0, 166.0), FVector(194.0, 262.0, 166.0), FVector(172.0, 380.0, 166.0), FVector(166.0, 560.0, 166.0)};
				Pawn->PlayWalk(Path, 9.5f, true, [Self]() {
					if (ABackRoomGameMode* GM = Self.Get())
					{
						GM->GoHome();
					}
				});
			}
			else
			{
				GoHome();
			}
		}
		if (bWalkingOut && !bFadingOut && LeaveT > 2.4f + 7.6f && PC && PC->PlayerCameraManager)
		{
			bFadingOut = true;
			PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 1.6f, FLinearColor::Black, true, true);
		}
		if (Pawn && Pawn->IsWalking())
		{
			Watch(RealDt);
		}
	}
}

namespace
{
/** A persona with glasses (1), shades (2) or aviators (3) in a frame color. */
FBackRoomPersona Eyed(FBackRoomPersona P, int32 Eyewear, uint32 FrameSrgb)
{
	P.Eyewear = Eyewear;
	P.FrameColor = FLinearColor(FColor((FrameSrgb >> 16) & 0xff, (FrameSrgb >> 8) & 0xff, FrameSrgb & 0xff));
	return P;
}
} // namespace

FBackRoomPersona ABackRoomGameMode::PersonaFor(const FString& Name)
{
	using T = EBackRoomTell;
	using M = EBackRoomTellMeaning;
	auto Persona = [](const TCHAR* InName, float Nerves, float Expressive, float Restless, float Fidget, float Posture, float Chatter, int32 Seed,
		TArray<FBackRoomTell> Tells, uint32 ShirtSrgb, float HeroRead) {
		FBackRoomPersona P;
		P.Shirt = FLinearColor(FColor((ShirtSrgb >> 16) & 0xff, (ShirtSrgb >> 8) & 0xff, ShirtSrgb & 0xff));
		P.Name = InName;
		P.Nervousness = Nerves;
		P.Expressiveness = Expressive;
		P.Restlessness = Restless;
		P.ChipFidget = Fidget;
		P.Posture = Posture;
		P.Chatter = Chatter;
		P.Seed = Seed;
		P.Tells = MoveTemp(Tells);
		P.HeroRead = HeroRead;
		return P;
	};
	// The regulars, what gives them away, and how they read you (HeroRead: above 0 they read your shaking
	// hands right, below 0 they think nerves mean a bluff).
	if (Name == TEXT("Dee"))
	{
		return Persona(TEXT("Dee"), 0.1f, 0.3f, 0.3f, 0.0f, 0.6f, 0.2f, 37, {}, 0x1f2a3d, 0.0f);
	}
	if (Name == TEXT("You"))
	{
		return Persona(TEXT("You"), 0.3f, 0.5f, 0.5f, 0.3f, 0.5f, 0.0f, 7, {}, 0x3c4a44, 0.0f);
	}
	if (Name == TEXT("Sal"))
	{
		// Old-timer, plays tight. Honest tells: he glances at his chips when he connects, and a bluff
		// makes him rub his neck. Forty years of watching hands: he knows a shake.
		// Reading glasses he pushes up to look at the board.
		return Eyed(Persona(TEXT("Sal"), 0.2f, 0.35f, 0.4f, 0.6f, 0.3f, 0.35f, 11,
			{Tell(T::ChipGlance, M::Strong, 0.85f, 0.05f), Tell(T::Recheck, M::Weak, 0.55f, 0.08f), Tell(T::NeckTouch, M::Bluff, 0.75f, 0.08f)}, 0x6b6a4e, 0.6f), 1, 0x3b2314);
	}
	if (Name == TEXT("Big Lou"))
	{
		// Loud, calls everything. His smile is real when he has it; when he bluffs it's mouth only.
		// Sees you shake and calls, every time.
		return Persona(TEXT("Big Lou"), 0.25f, 0.75f, 0.35f, 0.5f, 0.2f, 0.8f, 53,
			{Tell(T::RealSmile, M::Strong, 0.8f, 0.1f), Tell(T::FalseSmile, M::Bluff, 0.75f, 0.08f), Tell(T::ChipReach, M::Weak, 0.6f, 0.12f)}, 0x1c1c1e, -0.6f);
	}
	if (Name == TEXT("Twitch"))
	{
		// Wired, bets too much. Shakes when he has it; bluffing, he stares you down and swallows.
		// Shades indoors, like the streamers he watches.
		return Eyed(Persona(TEXT("Twitch"), 0.85f, 0.7f, 0.9f, 0.9f, 0.7f, 0.5f, 41,
			{Tell(T::Tremble, M::Strong, 0.85f, 0.12f), Tell(T::StareDown, M::Bluff, 0.75f, 0.12f), Tell(T::Swallow, M::Bluff, 0.6f, 0.1f),
				Tell(T::BlinkBurst, M::Bluff, 0.45f, 0.15f)}, 0x8c2a24, -0.5f), 2, 0x101010);
	}
	if (Name == TEXT("Mei"))
	{
		// Quiet and sharp. She acts: a sigh and a look away mean she's strong. Her pupils don't act. She
		// reads yours best of all.
		return Persona(TEXT("Mei"), 0.15f, 0.25f, 0.3f, 0.3f, 0.55f, 0.15f, 23,
			{Tell(T::Sigh, M::Strong, 0.65f, 0.08f), Tell(T::LookAway, M::Strong, 0.5f, 0.15f), Tell(T::PupilFlare, M::Strong, 0.6f, 0.05f),
				Tell(T::LipPress, M::Bluff, 0.45f, 0.12f)}, 0x34383d, 0.9f);
	}
	// The Riverside's Sunday faces.
	if (Name == TEXT("Mrs. Park"))
	{
		// Thirty years of the Sunday tournament. Folds and folds; when she finally bluffs she goes
		// rigid, and a real hand pulls her eyes down to her chips. Nothing gets past her.
		return Eyed(Persona(TEXT("Mrs. Park"), 0.15f, 0.2f, 0.25f, 0.4f, 0.75f, 0.25f, 61,
			{Tell(T::Freeze, M::Bluff, 0.85f, 0.05f), Tell(T::ChipGlance, M::Strong, 0.7f, 0.05f), Tell(T::Sigh, M::Weak, 0.5f, 0.1f)}, 0x5a3e57, 0.7f), 1, 0x5a1f2a);
	}
	if (Name == TEXT("Rick"))
	{
		// Car-dealership money, loud and loose. Bluffing he stares you down and rubs his neck; with a
		// hand he goes quiet and looks off at the room.
		// Aviators, gold, from the showroom floor.
		return Eyed(Persona(TEXT("Rick"), 0.35f, 0.7f, 0.6f, 0.7f, 0.25f, 0.75f, 67,
			{Tell(T::StareDown, M::Bluff, 0.8f, 0.15f), Tell(T::NeckTouch, M::Bluff, 0.55f, 0.1f), Tell(T::LookAway, M::Strong, 0.6f, 0.1f)}, 0x2b4d7a, -0.3f), 3, 0xc9a24a);
	}
	if (Name == TEXT("Dre"))
	{
		// Talks the whole time. Can't hide a smile with a hand; swallows when he's pushing air, and
		// reaches for chips when he wants you to check.
		return Persona(TEXT("Dre"), 0.3f, 0.85f, 0.55f, 0.6f, 0.45f, 0.9f, 71,
			{Tell(T::RealSmile, M::Strong, 0.75f, 0.15f), Tell(T::Swallow, M::Bluff, 0.6f, 0.1f), Tell(T::ChipReach, M::Weak, 0.55f, 0.1f)}, 0xc28a2e, -0.2f);
	}
	if (Name == TEXT("gh0stfold"))
	{
		// The rival from RiverLine, in a hoodie. Almost nothing leaks: the pupils, close up, when he has
		// it, and a burst of blinks after a bluff. Reads you better than anyone.
		return Persona(TEXT("gh0stfold"), 0.1f, 0.12f, 0.3f, 0.5f, 0.6f, 0.3f, 83,
			{Tell(T::PupilFlare, M::Strong, 0.45f, 0.02f), Tell(T::BlinkBurst, M::Bluff, 0.35f, 0.05f)}, 0x232326, 1.0f);
	}
	// Anyone else: a stranger with a couple of the common tells.
	const int32 Seed = static_cast<int32>(GetTypeHash(Name) & 0x7fffffff);
	FRandomStream R(Seed);
	const T Pool[] = {T::ChipGlance, T::BrowFlash, T::Swallow, T::LipPress, T::LookAway, T::NeckTouch, T::Recheck, T::Sigh, T::ChipReach, T::RealSmile};
	const M Means[] = {M::Strong, M::Weak, M::Bluff};
	TArray<FBackRoomTell> Tells;
	const T A = Pool[R.RandRange(0, 9)];
	T B = Pool[R.RandRange(0, 9)];
	B = B == A ? Pool[(static_cast<int32>(A) + 3) % 10] : B;
	Tells.Add(Tell(A, Means[R.RandRange(0, 2)], 0.6f, 0.12f));
	Tells.Add(Tell(B, Means[R.RandRange(0, 2)], 0.5f, 0.12f));
	static const uint32 Shirts[] = {0x2f3b52, 0x6e2b2b, 0x3a5a40, 0x8a7a5a, 0x222222, 0x5a4a6e, 0x9a9a9a, 0x7a4a2a};
	FBackRoomPersona P = Persona(*Name, R.FRandRange(0.15f, 0.6f), R.FRandRange(0.3f, 0.7f), R.FRandRange(0.3f, 0.7f), R.FRandRange(0.2f, 0.7f), R.FRandRange(0.3f, 0.7f),
		0.0f, Seed % 997 + 101, MoveTemp(Tells), Shirts[R.RandRange(0, 7)], 0.0f);
	// What they wear, the same every night: a print or a graphic on about half the shirts, trousers, now and then a
	// hat (over short hair), glasses or shades.
	auto C = [](uint32 S) { return FLinearColor(FColor((S >> 16) & 0xff, (S >> 8) & 0xff, S & 0xff)); };
	static const uint32 Tones[] = {0x1f2a44, 0x2f3b52, 0x6e2b2b, 0x3a5a40, 0x8a7a5a, 0x222222, 0x5a4a6e, 0x9a9a9a, 0x7a4a2a, 0xd8d4c8, 0x7b1e1e,
		0x1e4d6b, 0xc9a227, 0x4a5d23, 0x30343c, 0xb85c38, 0xe8e6df, 0x2b2b30};
	constexpr int32 NumTones = UE_ARRAY_COUNT(Tones);
	P.Shirt = C(Tones[R.RandRange(0, NumTones - 1)]);
	const float Look = R.FRand();
	if (Look < 0.36f)
	{
		P.ShirtPrint = R.RandRange(1, 7);
	}
	else if (Look < 0.56f)
	{
		P.ShirtGraphic = R.RandRange(1, 6);
	}
	// The print's colors against the shirt: light on dark, dark on light.
	const bool bDark = P.Shirt.GetLuminance() < 0.25f;
	static const uint32 Lights[] = {0xf2efe6, 0xe8d9a8, 0xd9e2ea, 0xf0c9a0, 0xc8d8b8};
	static const uint32 Darks[] = {0x14161c, 0x2a1f1a, 0x1c2a40, 0x3a1414, 0x223322};
	P.ShirtB = C(bDark ? Lights[R.RandRange(0, 4)] : Darks[R.RandRange(0, 4)]);
	P.ShirtC = C(Tones[R.RandRange(0, NumTones - 1)]);
	static const uint32 Trousers[] = {0x2a3a5c, 0x243150, 0x161616, 0x8b7d5b, 0x4a4a4e, 0x1c2236, 0x5a4632};
	P.Pants = C(Trousers[R.RandRange(0, 6)]);
	const float Head = R.FRand();
	P.Headwear = Head < 0.16f ? 1 : Head < 0.23f ? 2 : Head < 0.27f ? 3 : 0;
	const float Eye = R.FRand();
	P.Eyewear = Eye < 0.13f ? 1 : Eye < 0.20f ? 2 : Eye < 0.24f ? 3 : 0;
	// Muted: under the table lamps a hat's crown takes the most light of anything in the room.
	static const uint32 Hats[] = {0x1c2236, 0x141414, 0x5e1a1a, 0x6b5f45, 0x26392a, 0x2f3338, 0x4a3426, 0x1f3a3d};
	P.WearColor = C(Hats[R.RandRange(0, 7)]);
	static const uint32 Frames[] = {0x111111, 0x3b2314, 0x2a2d33, 0x1c2a4a, 0x4a1a22};
	P.FrameColor = C(Frames[R.RandRange(0, 4)]);
	return P;
}

FString ABackRoomGameMode::CastAssetFor(const FString& Name)
{
	if (Name == TEXT("Big Lou"))
	{
		return TEXT("BigLou");
	}
	if (Name == TEXT("Mrs. Park"))
	{
		return TEXT("MrsPark");
	}
	if (Name == TEXT("gh0stfold"))
	{
		return TEXT("Ghost");
	}
	if (Name == TEXT("You"))
	{
		return TEXT("Hero");
	}
	for (const TCHAR* Known : {TEXT("Dee"), TEXT("Sal"), TEXT("Twitch"), TEXT("Mei"), TEXT("Rick"), TEXT("Dre")})
	{
		if (Name == Known)
		{
			return Known;
		}
	}
	return FString();
}

ABackRoomPlayer* ABackRoomGameMode::SpawnPerson(const FString& CastName, const FTransform& At, EBackRoomRole AtTableAs, const FBackRoomPersona& Persona)
{
	ABackRoomPlayer* P = GetWorld()->SpawnActorDeferred<ABackRoomPlayer>(ABackRoomPlayer::StaticClass(), At);
	P->Persona = Persona;
	P->SeatRole = AtTableAs;
	P->MetaHumanClass = CastBlueprint(*CastName);
	P->FinishSpawning(At);
	return P;
}

void ABackRoomGameMode::SeatEveryone()
{
	// Players placed in the map (for look development) stay out of the game.
	for (TActorIterator<ABackRoomPlayer> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}

	Table = GetWorld()->SpawnActor<ABackRoomTable>(ABackRoomTable::StaticClass(), FTransform::Identity);
	// The night's rules: in a career a bust is yours to deal with, Dee only teaches the first time,
	// and what you learned of the regulars on other nights is still yours.
	Table->bHeroAutoReload = !IsCareer();
	Table->bFirstVisit = !IsCareer() || bFirstVisit;
	if (Save.IsValid())
	{
		for (const auto& R : Save->Life.Reads)
		{
			Table->Reads.Add(FString(UTF8_TO_TCHAR(R.first.c_str())), R.second);
		}
		// Each night at the table, the pressure gets to you a little less.
		Table->Composure.Sensitivity = FMath::Clamp(1.0f - 0.06f * PastNights, 0.65f, 1.0f);
	}
	TWeakObjectPtr<ABackRoomGameMode> Self = this;
	Table->OnNote = [Self](EBackRoomTableNote Note) {
		if (ABackRoomGameMode* GM = Self.Get())
		{
			GM->OnTableNote(static_cast<uint8>(Note));
		}
	};
	// Dee deals her Tuesday game and the Riverside's Sunday; the card room's other days have dealers of their own, the
	// same one every week (a vest and a bow tie on one of the room's faces).
	FString DealerBody = TEXT("Dee");
	FBackRoomPersona DealerPersona = PersonaFor(TEXT("Dee"));
	if (bLive && ((LiveDay % 7) + 7) % 7 != 6)
	{
		static const TCHAR* Names[6] = {TEXT("Rosa"), TEXT("Hank"), TEXT("Lupe"), TEXT("Vic"), TEXT("June"), TEXT("Tomas")};
		static const TCHAR* Bodies[6] = {TEXT("ExtraE"), TEXT("ExtraL"), TEXT("ExtraK"), TEXT("ExtraJ"), TEXT("ExtraG"), TEXT("ExtraF")};
		const int32 Day = ((LiveDay % 7) + 7) % 7;
		DealerBody = Bodies[Day];
		DealerPersona = PersonaFor(Names[Day]);
		DealerPersona.Shirt = FLinearColor(FColor(0x14, 0x14, 0x18));
		DealerPersona.Chatter = 0.2f;
		// The house dresses its dealers: a black shirt, nothing on the head.
		DealerPersona.ShirtPrint = 0;
		DealerPersona.ShirtGraphic = 0;
		DealerPersona.Headwear = 0;
		DealerPersona.Pants = FLinearColor(FColor(0x12, 0x12, 0x14));
	}
	Dealer = SpawnPerson(DealerBody, ABackRoomStage::SeatTransform(4), EBackRoomRole::Dealer, DealerPersona);
	Hero = SpawnPerson(TEXT("Hero"), ABackRoomStage::SeatTransform(0), EBackRoomRole::Hero, PersonaFor(TEXT("You")));
	Table->SetDealer(Dealer);
	Table->AddPlayer(Hero, 0, ss::Archetype::Tag, HeroBuyInChips);

	if (bLive)
	{
		// The Riverside: the tournament seats the table (BackRoomLive.cpp).
		SeatLive();
	}
	else
	{
		// Dee's regulars, in their usual seats.
		struct FRegular
		{
			const TCHAR* Name;
			int32 Seat;
			ss::Archetype Style;
			int64 BuyIn;
		};
		const FRegular Regulars[] = {
			{TEXT("Sal"), 3, ss::Archetype::Nit, 240},
			{TEXT("Big Lou"), 5, ss::Archetype::Station, 300},
			{TEXT("Twitch"), 2, ss::Archetype::Maniac, 160},
			{TEXT("Mei"), 6, ss::Archetype::Reg, 220},
		};
		for (const FRegular& R : Regulars)
		{
			ABackRoomPlayer* P = SpawnPerson(CastAssetFor(R.Name), ABackRoomStage::SeatTransform(R.Seat), EBackRoomRole::Player, PersonaFor(R.Name));
			Opponents.Add(P);
			Table->AddPlayer(P, R.Seat, R.Style, R.BuyIn);
		}
	}

	// Everyone knows where the others sit: gaze targets at head height.
	ABackRoomStage* S = FindOrSpawnStage();
	const FVector HeroEye = S ? S->EyeLocation() : FVector(-98.0, 0.0, 124.0);
	TArray<ABackRoomPlayer*> Everyone = {Dealer, Hero};
	for (ABackRoomPlayer* O : Opponents)
	{
		Everyone.Add(O);
	}
	for (ABackRoomPlayer* P : Everyone)
	{
		P->HeroEyes = HeroEye;
		P->PotAt = FVector(10.0, 0.0, ABackRoomStage::FeltZ);
		P->DealerAt = ABackRoomStage::SeatTransform(4).TransformPosition(FVector(-14.0, 0.0, 112.0));
		P->OthersAt.Reset();
		for (ABackRoomPlayer* Other : Everyone)
		{
			if (Other != P && Other != Hero)
			{
				P->OthersAt.Add(Other->GetActorTransform().TransformPosition(FVector(-14.0, 0.0, 112.0)));
			}
		}
	}
}
