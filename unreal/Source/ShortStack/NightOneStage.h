#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FirstPersonArms.h"
#include "ShortStack/UI/Canvas.h"

#include "NightOneStage.generated.h"

class SDrawListWidget;
class UInstancedStaticMeshComponent;
class ULocalLightComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class UPostProcessComponent;
class URectLightComponent;
class USpotLightComponent;
class UPoseableMeshComponent;
class USkeletalMesh;
class UStaticMesh;
class UStaticMeshComponent;
class UWidgetComponent;

/**
 * What the apartment shows of the player's life: the GearDrop gear they own, set up where it would go, and the
 * mementos their career has left (ss::Session's gear and life::State).
 */
struct FRoomGear
{
	bool bMonitor = false;     // the 24", on an arm at the right of the desk
	bool bMonitorWide = false; // the 27", on the left (the desk lamp moves up to the windowsill)
	int32 Towers = 0;          // the desktop PC on the floor right of the desk; 2 for the two-PC setup
	int32 Cam = 0;             // 1 a 720p and 2 a 1080p webcam on the laptop's lid, 3 the mirrorless on a tripod
	int32 Mic = 0;             // 1 a USB mic on the desk, 2 the broadcast mic on a boom arm
	int32 Lights = 0;          // 1 a ring light behind the laptop, 2 key lights either side
	bool bMacroPad = false;
	bool bHeadphones = false;
	bool bPlant = false;       // on the windowsill
	bool bCurtains = false;
	bool bRouter = false;      // fiber
	bool bTrophy = false;      // won the Embercrest's Sunday tournament
	bool bDeeChip = false;     // came out ahead at Dee's game: a $100 chip on a little stand on the sill

	bool operator==(const FRoomGear& O) const = default;
};

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

	/**
	 * Builds the set again so it picks up materials and props imported since the level loaded.
	 * The editor setup script calls this after an import; it is also a button on the actor.
	 */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Short Stack")
	void RebuildSet();

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
	/**
	 * The GearDrop LED room kit (ss::Session::RoomGlow): a strip behind the desk under the window, a cove strip where
	 * the walls meet the ceiling, and a soft fill so the room takes the colour. Level 1 is steady; stream alerts and
	 * big hands push it up (or dip it, for a bad beat). Off hides it all.
	 */
	void SetRoomLights(bool bOn, const FLinearColor& Color, float Level);
	/** Shows what the player owns and has won (hidden pieces cost nothing to render); a no-op when nothing changed. */
	void SetGear(const FRoomGear& Gear);
	const FRoomGear& GearShown() const { return Gear; }
	/**
	 * Each frame: the PCs' RGB (the LED kit's colour when it's on, the PC's own otherwise; Level as SetRoomLights),
	 * and the streaming lights, on while the player is live.
	 */
	void SetGearGlow(const FLinearColor& Rgb, float Level, bool bLive);
	/** The monitors' pictures (0: the 24" on the right, 1: the 27" on the left), drawn at MonitorResolution. */
	void SetMonitorDrawList(int32 Index, const TSharedPtr<const ss::ui::DrawList>& List);
	bool MonitorShown(int32 Index) const { return Index == 0 ? Gear.bMonitor : Gear.bMonitorWide; }
	/** 0 = deep night, 1 = first light (from the tournament clock). */
	void SetDawn(float Value);
	/** Lens treatment: Focus 0..1 (leaned in), Tilt 0..1, Pulse 0..1 (heartbeat). */
	void SetLens(float Focus, float Tilt, float Pulse);
	/** Put another empty can on the desk (one per hour of grinding). */
	void AddCan();
	void Lightning(float Strength);

	// The player's arms (SK_Arms): the left hand on the laptop, the right on the mouse.
	/** A hotkey press: the matching finger reaches over and taps the key. */
	void ArmsKey(const FString& Name);
	/** A click: the mouse hand's index finger presses. */
	void ArmsClick();
	/** Moves the arms with the head (camera location and facing) and the mouse with the pointer (0..1 across the view). */
	void UpdateArms(float DeltaSeconds, const FVector& CameraLocation, const FVector& CameraForward, const FVector2D& Pointer, bool bVisible);

	/** Called when lightning strikes: (delay until thunder, strength). */
	TFunction<void(float, float)> OnThunder;

	// ------------------------------------------------------------ tuning
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float ExposureBias = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float ScreenLightCandela = 0.6f;

	/**
	 * How much the laptop light shows in reflections. The light is much brighter than the screen
	 * looks, so at full strength its reflection washes the glossy keycaps and palm rest out to white.
	 */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float ScreenLightSpecular = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float NeonCandela = 55.0f;

	UPROPERTY(EditAnywhere, Category = "Short Stack|Look")
	float RoomFillCandela = 0.15f;

	/** The LED kit: the wash from the strip behind the desk, the ceiling cove (each wall), the room fill, the strips' glow. */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look|LEDs")
	float LedDeskCandela = 1.8f;
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look|LEDs")
	float LedCoveCandela = 2.6f;
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look|LEDs")
	float LedFillCandela = 1.6f;
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look|LEDs")
	float LedEmissive = 20.0f;

	/** GearDrop's gear: the PCs' RGB glow and the light it throws, the monitors' light, the streaming lights. */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look|Gear")
	float RgbEmissive = 14.0f;
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look|Gear")
	float RgbCandela = 0.9f;
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look|Gear")
	float MonitorCandela = 0.45f;
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look|Gear")
	float StudioEmissive = 12.0f;
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look|Gear")
	float RingCandela = 2.2f;
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look|Gear")
	float KeyCandela = 3.0f;

	/** The monitors' picture resolution (16:9). */
	UPROPERTY(EditAnywhere, Category = "Short Stack|Look|Gear")
	FIntPoint MonitorResolution = FIntPoint(1280, 720);

	// Image options from Settings (applied in SetLens). BrightnessBias is the player's Brightness, in stops
	// on top of the look's ExposureBias.
	float BrightnessBias = 0.0f;
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
	void BuildLeds();
	void BuildGear();
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
	/** The building across the street (art/blender/assets/tenement.py). */
	UPROPERTY()
	TObjectPtr<UStaticMesh> TenementMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> LaptopBaseMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> LaptopLidMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> DeskMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> MugMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> PhoneMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> ChipsMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> LampMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> ChairMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> MouseMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> MousePadMesh;
	UPROPERTY()
	TObjectPtr<USkeletalMesh> ArmsMesh;
	/** Yaw that turns a Blender prop (front toward -Y, Z up) to face the chair (-X) once imported. */
	float ImportYaw = 0.0f;

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
	TObjectPtr<USceneComponent> LaptopFrame; // the laptop's placement on the desk (keys are measured from it)
	UPROPERTY()
	TObjectPtr<USceneComponent> MouseRoot;
	UPROPERTY()
	TObjectPtr<USceneComponent> ArmsRoot;
	UPROPERTY()
	TObjectPtr<UPoseableMeshComponent> ArmsComp;
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
	// The LED kit (hidden until it's bought and on): the desk strip's wash first, then the three cove washes.
	UPROPERTY()
	TArray<TObjectPtr<URectLightComponent>> LedWashes;
	UPROPERTY()
	TObjectPtr<UPointLightComponent> LedFill;
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> LedStrips;
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> LedMaterial;
	bool bLedsShown = false;
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Cans;

	// GearDrop's gear and the career's mementos (BuildGear): one group per piece, hidden unless owned.
	UPROPERTY()
	TArray<TObjectPtr<USceneComponent>> GearGroups;
	UPROPERTY()
	TArray<TObjectPtr<UWidgetComponent>> MonitorScreens; // the 24", the 27"
	UPROPERTY()
	TArray<TObjectPtr<URectLightComponent>> MonitorLights;
	UPROPERTY()
	TArray<TObjectPtr<URectLightComponent>> RgbLights; // out of each PC's open side
	UPROPERTY()
	TArray<TObjectPtr<ULocalLightComponent>> StudioLights; // the ring light's, then the key lights'
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> RgbMaterial;
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> StudioMaterial;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> LampComp;
	UPROPERTY()
	FVector LampOnDesk = FVector::ZeroVector;
	UPROPERTY()
	FVector LampOnSill = FVector::ZeroVector;
	FRoomGear Gear;
	bool bGearApplied = false;
	int32 StudioState = -1; // the streaming lights: -1 unknown, 0 off, 1 on
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> TimedMaterials; // sky, city, glass, rain: Dawn / Flash parameters

	TSharedPtr<SDrawListWidget> ScreenSlate;
	TSharedPtr<SDrawListWidget> PhoneSlate;
	TArray<TSharedPtr<SDrawListWidget>> PropSlates;
	TArray<TSharedPtr<SDrawListWidget>> MonitorSlates;
	TArray<TSharedPtr<const ss::ui::DrawList>> PropLists;

	FFirstPersonArms Arms;
	int32 ArmsInitTries = 0;
	// Set while building the set: properties, so Play-In-Editor's copy of the placed stage keeps them
	// (a plain member resets to zero there and UpdateArms would park the mouse at the stage's origin).
	UPROPERTY()
	FVector MouseHome = FVector::ZeroVector;
	FVector2D MouseOffset = FVector2D::ZeroVector;
	float ArmsAway = 0.0f; // 0 looking at the desk, 1 looking well off to a side (the shoulders drop out of view)
	UPROPERTY()
	float MouseTop = 2.6f; // height of the mouse's hump above the desk (cm)
	/** A key's top, in world space (the Blender laptop's layout; the stand-in keyboard matches it). */
	FVector KeyWorld(const FString& Name) const;

	float Time = 0.0f;
	float Flash = 0.0f;
	float NextLightning = 25.0f;
	float NeonLevel = 1.0f;
	float NeonGlitch = 0.0f;
	float Dawn = 0.0f;
	FLinearColor ScreenGlow = FLinearColor(0.72f, 0.84f, 1.0f);
	float ScreenBrightness = 1.0f;
};
