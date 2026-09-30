#include "NightOnePawn.h"

#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"

ANightOnePawn::ANightOnePawn()
{
	PrimaryActorTick.bCanEverTick = true;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	RootComponent = Camera;
	Camera->bConstrainAspectRatio = false;
	Camera->SetFieldOfView(70.0f);
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
}

void ANightOnePawn::Configure(const FVector& InEye, const FVector& InScreenCenter, const FVector& InScreenNormal, const FVector2D& InScreenSizeCm)
{
	Eye = InEye;
	ScreenCenter = InScreenCenter;
	ScreenNormal = InScreenNormal.GetSafeNormal();
	ScreenSizeCm = InScreenSizeCm;
	bConfigured = true;
}

void ANightOnePawn::SetEstablishingShot(const FVector& Location, const FVector& LookAt, bool bActive)
{
	ShotLocation = Location;
	ShotLookAt = LookAt;
	ShotTarget = bActive ? 1.0f : 0.0f;
	if (!bHasShot)
	{
		ShotBlend = ShotTarget; // the first frame starts on the shot, not mid-flight
		bHasShot = true;
	}
}

void ANightOnePawn::ToggleLean()
{
	TargetFocus = TargetFocus > 0.5f ? 0.0f : 1.0f;
	if (TargetFocus == 0.0f)
	{
		Yaw = 0.0f;
		Pitch = -0.1f;
	}
}

double ANightOnePawn::ScreenDistance(double VFovRad, double Aspect) const
{
	// Distance from the screen that frames it with a small margin.
	const double Tan = FMath::Tan(VFovRad / 2.0);
	const double ByH = (ScreenSizeCm.Y * 0.5 * 1.1) / Tan;
	const double ByW = (ScreenSizeCm.X * 0.5 * 1.06) / (Tan * Aspect);
	return FMath::Max(ByH, ByW);
}

void ANightOnePawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const double Dt = FMath::Max(0.0f, DeltaSeconds);
	Clock += Dt;

	double Aspect = 16.0 / 9.0;
	if (GEngine && GEngine->GameViewport)
	{
		FVector2D Size;
		GEngine->GameViewport->GetViewportSize(Size);
		if (Size.X > 0.0 && Size.Y > 0.0)
		{
			Aspect = Size.X / Size.Y;
		}
	}
	// Vertical field of view from the settings (+12 degrees on tall screens), as in the prototype; Unreal wants horizontal.
	const double VFov = FMath::DegreesToRadians(static_cast<double>(VerticalFov) + (Aspect < 1.0 ? 12.0 : 0.0));
	const double HFov = 2.0 * FMath::Atan(FMath::Tan(VFov / 2.0) * Aspect);
	Camera->SetFieldOfView(static_cast<float>(FMath::RadiansToDegrees(HFov)));

	const double K = 1.0 - FMath::Exp(-Dt * 4.5);
	Focus += static_cast<float>((TargetFocus - Focus) * K);
	const double T = Focus * Focus * (3.0 - 2.0 * Focus);

	// Room pose: look direction from yaw/pitch.
	const FVector RoomDir(FMath::Cos(Yaw) * FMath::Cos(Pitch), FMath::Sin(Yaw) * FMath::Cos(Pitch), FMath::Sin(Pitch));
	const FVector RoomLook = Eye + RoomDir * 100.0;
	// Screen pose.
	const FVector ScreenPos = ScreenCenter + ScreenNormal * ScreenDistance(VFov, Aspect);

	const double Breathe = (FMath::Sin(Clock * 1.3) * 0.22 + FMath::Sin(Clock * 0.37) * 0.15);
	const double Sway = FMath::Sin(Clock * 0.6) * 0.12;
	const double ShakeX = Shake * FMath::Sin(Clock * 37.0) * 0.15;
	const double ShakeY = Shake * FMath::Sin(Clock * 29.0 + 1.0) * 0.15;
	FVector Pos = FMath::Lerp(Eye, ScreenPos, T);
	// Fully leaned in, the head holds still so the screen text stays pixel-steady.
	const double Still = 1.0 - T;
	const double ShakeK = 1.0 - 0.6 * T;
	Pos.Z += Breathe * Still + ShakeY * ShakeK;
	Pos.Y += Sway * Still + ShakeX * ShakeK;
	FVector Look = FMath::Lerp(RoomLook, ScreenCenter, T);
	if (!bConfigured)
	{
		return;
	}
	// Establishing shot: a slow, eased flight between the shot and the seat.
	ShotBlend += (ShotTarget - ShotBlend) * static_cast<float>(1.0 - FMath::Exp(-Dt * 1.4));
	if (bHasShot && ShotBlend > 0.0005f)
	{
		const double E = static_cast<double>(ShotBlend) * ShotBlend * (3.0 - 2.0 * ShotBlend);
		Pos = FMath::Lerp(Pos, ShotLocation, E);
		Look = FMath::Lerp(Look, ShotLookAt, E);
	}
	Camera->SetWorldLocationAndRotation(Pos, (Look - Pos).Rotation());
}
