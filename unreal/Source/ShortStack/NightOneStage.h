#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShortStack/UI/Canvas.h"

#include "NightOneStage.generated.h"

class SDrawListWidget;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class UPostProcessComponent;
class URectLightComponent;
class USpotLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UWidgetComponent;

/**
 * The studio apartment on the third floor, 2 a.m.: room shell, desk and
 * props, the rain-streaked window, the city and the laundromat's neon across
 * the street, lights and the post-process "lens". Built procedurally from
 * engine shapes (port of the files in web/src/scene) so it runs in any level; each
 * piece is a component that Blender assets can replace.
 *
 * Coordinates: the prototype's meters (x right, y up, z toward the viewer)
 * map to Unreal centimeters as X = -z, Y = x, Z = y.
 */
UCLASS()
class SHORTSTACK_API ANightOneStage : public AActor
{
	GENERATED_BODY()

public:
	ANightOneStage();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// ------------------------------------------------------------ game hooks
	FVector EyeLocation() const;
	FVector ScreenCenter() const;
	FVector ScreenNormal() const;
	/** Laptop screen size in centimeters. */
	FVector2D ScreenSize() const { return FVector2D(31.6, 19.75); }
	/** The title screen's slow camera move at time T (seconds): where the camera is and what it looks at. */
	void MenuShot(double T, FVector& OutLocation, FVector& OutLookAt) const;
	/** Ray from the camera onto the laptop screen; returns the point in client space (1600 x 1000). */
	bool ScreenHit(const FVector& Origin, const FVector& Direction, FVector2D& OutClient) const;

	void SetScreenDrawList(const TSharedPtr<const ss::ui::DrawList>& List);
	void SetPhoneDrawList(const TSharedPtr<const ss::ui::DrawList>& List);
	void SetPhoneBrightness(float Level);
	void SetScreenGlow(const FLinearColor& Color, float Brightness);
	/** 0 = deep night, 1 = first light (from the tournament clock). */
	void SetDawn(float Value);
	/** Lens treatment: Focus 0..1 (leaned in), Tilt 0..1, Pulse 0..1 (heartbeat). */
	void SetLens(float Focus, float Tilt, float Pulse);
	/** Put another empty can on the desk (one per hour of grinding). */
	void AddCan();
	void Lightning(float Strength);

	/** Called when lightning strikes: (delay until thunder, strength). */
	TFunction<void(float, float)> OnThunder;

	// ------------------------------------------------------------ tuning
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float ExposureBias = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float ScreenLightCandela = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float NeonCandela = 55.0f;

	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float RoomFillCandela = 0.08f;

	// Image options from Settings (applied in SetLens).
	bool bMotionBlur = false;
	float GrainScale = 1.0f;
	float FringeScale = 1.0f;

	/** Laptop client resolution (logical 1600 x 1000 is scaled to this). */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	FIntPoint ScreenResolution = FIntPoint(2400, 1500);

private:
	void BuildSet();
	void BuildShell();
	void BuildWindow();
	void BuildDesk();
	void BuildProps();
	void BuildOutside();
	void BuildLights();
	void AttachSlate();

	UMaterialInstanceDynamic* Surface(FName Key, uint32 SrgbHex, float Roughness, float Metallic = 0.0f, float Pattern = 0.0f, float Emissive = 0.0f);
	UMaterialInstanceDynamic* Special(UMaterialInterface* Parent, FName Key);
	UStaticMeshComponent* AddMesh(UStaticMesh* Mesh, UMaterialInterface* Material, const FVector& Location, const FVector& Scale, const FRotator& Rotation = FRotator::ZeroRotator, USceneComponent* Parent = nullptr, bool bShadows = true);
	/** Box from Unreal-space min/max corners (cm). */
	UStaticMeshComponent* BoxMinMax(UMaterialInterface* Material, const FVector& Min, const FVector& Max, bool bShadows = true);
	/** Box from prototype coordinates: center (m) and size (width x, height y, depth z in m). */
	UStaticMeshComponent* BoxWeb(UMaterialInterface* Material, const FVector& WebCenter, const FVector& WebSize, float YawDeg = 0.0f, USceneComponent* Parent = nullptr);
	/** Upright cylinder from prototype coordinates: base center (m), radius and height (m). */
	UStaticMeshComponent* CylinderWeb(UMaterialInterface* Material, const FVector& WebBase, float Radius, float Height, USceneComponent* Parent = nullptr);
	UWidgetComponent* AddWidget(const FVector& Location, const FRotator& Rotation, const FVector2D& SizeCm, const FIntPoint& Pixels, bool bLit, bool bTranslucent, USceneComponent* Parent = nullptr);
	template <typename T>
	T* NewPart(USceneComponent* Parent = nullptr);

	UPROPERTY()
	TMap<FName, TObjectPtr<UMaterialInstanceDynamic>> Materials;

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> PlaneMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> ConeMesh;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> FallbackMaterial;

	// Props modeled in Blender (art/blender), imported from unreal/Art/Meshes by the editor setup script.
	// Each is optional: without it the prop is built from engine shapes.
	UPROPERTY()
	TObjectPtr<UStaticMesh> CanMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> LaptopBaseMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> LaptopLidMesh;

	// Generated by Content/Python/init_unreal.py; optional (fallbacks keep the game running).
	// Not transient: Play-In-Editor copies the placed stage, and cans added during play still need them.
	UPROPERTY()
	TObjectPtr<UMaterialInterface> SurfaceMaterial;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> EmissiveMaterial;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> GlowMaterial;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> GlassMaterial;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> CityMaterial;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> SkyMaterial;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> RainMaterial;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> WidgetLitMaterial;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> RainCookieMaterial;

	UPROPERTY()
	TObjectPtr<USceneComponent> LidPivot;
	UPROPERTY()
	TObjectPtr<UWidgetComponent> Screen;
	UPROPERTY()
	TObjectPtr<UWidgetComponent> PhoneScreen;
	UPROPERTY()
	TObjectPtr<UWidgetComponent> NeonWidget;
	UPROPERTY()
	TObjectPtr<UWidgetComponent> Keyboard;
	UPROPERTY()
	TArray<TObjectPtr<UWidgetComponent>> PropWidgets; // notice, bill, door notice, poster, 3 notes (in that order)
	UPROPERTY()
	TObjectPtr<URectLightComponent> ScreenLight;
	UPROPERTY()
	TObjectPtr<USpotLightComponent> NeonSpot;
	UPROPERTY()
	TObjectPtr<URectLightComponent> CityFill;
	UPROPERTY()
	TObjectPtr<UPointLightComponent> BlueNeon;
	UPROPERTY()
	TObjectPtr<UPointLightComponent> PhoneLight;
	UPROPERTY()
	TObjectPtr<UPointLightComponent> RoomFill;
	UPROPERTY()
	TObjectPtr<UPostProcessComponent> Lens;
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Cans;
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> TimedMaterials; // sky, city, glass, rain: Dawn / Flash parameters

	TSharedPtr<SDrawListWidget> ScreenSlate;
	TSharedPtr<SDrawListWidget> PhoneSlate;
	TArray<TSharedPtr<SDrawListWidget>> PropSlates;
	TArray<TSharedPtr<const ss::ui::DrawList>> PropLists;

	float Time = 0.0f;
	float Flash = 0.0f;
	float NextLightning = 25.0f;
	float NeonLevel = 1.0f;
	float NeonGlitch = 0.0f;
	float Dawn = 0.0f;
	FLinearColor ScreenGlow = FLinearColor(0.72f, 0.84f, 1.0f);
	float ScreenBrightness = 1.0f;
};
