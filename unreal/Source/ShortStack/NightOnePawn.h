#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "NightOnePawn.generated.h"

class UCameraComponent;

/**
 * Seated first-person camera (port of web/src/scene/camera.ts). Two poses
 * blend smoothly: leaned in so the laptop fills the view (playing), and
 * sitting back, free to look around the apartment by pointing.
 */
UCLASS()
class SHORTSTACK_API ANightOnePawn : public APawn
{
	GENERATED_BODY()

public:
	ANightOnePawn();

	void Configure(const FVector& InEye, const FVector& InScreenCenter, const FVector& InScreenNormal, const FVector2D& InScreenSizeCm);
	bool IsConfigured() const { return bConfigured; }
	virtual void Tick(float DeltaSeconds) override;

	/** 0 = sitting back (room), 1 = leaned in (screen). */
	float Focus = 0.0f;
	float TargetFocus = 0.0f;
	/** Look direction while sitting back (radians). */
	float Yaw = 0.12f;
	float Pitch = 0.05f;
	/** Extra shake, e.g. heartbeat during an all-in. */
	float Shake = 0.0f;

	void ToggleLean();

	/** Vertical field of view in degrees while sitting back (the Field of view setting). */
	float VerticalFov = 50.0f;
	/** The title screen's camera: the seat blends to (or from) this shot, a slow fly-in when play starts. */
	void SetEstablishingShot(const FVector& Location, const FVector& LookAt, bool bActive);

	UPROPERTY(VisibleAnywhere, Category = "Short Stack")
	TObjectPtr<UCameraComponent> Camera;

private:
	double ScreenDistance(double VFovRad, double Aspect) const;

	FVector Eye = FVector(-14.0, 0.0, 117.0);
	FVector ScreenCenter = FVector::ZeroVector;
	FVector ScreenNormal = FVector::BackwardVector;
	FVector2D ScreenSizeCm = FVector2D(31.6, 19.75);
	bool bConfigured = false;
	double Clock = 0.0;
	FVector ShotLocation = FVector::ZeroVector;
	FVector ShotLookAt = FVector::ForwardVector;
	bool bHasShot = false;
	float ShotBlend = 1.0f;
	float ShotTarget = 1.0f;
};
