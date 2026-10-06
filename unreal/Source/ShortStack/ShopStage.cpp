#include "ShopStage.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "SlateDrawList.h"
#include "UObject/UObjectIterator.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogShopStage, Log, All);

namespace ShopStageDetail
{
// The wake rule (spec 3.8): awake with the hero inside, within 25 m of the room, or within 40 m with its glass in view.
constexpr double WakeNear = 2500.0;
constexpr double WakeSeen = 4000.0;
constexpr float WakeEvery = 0.25f;
// Inside a shop the exposure has its own clamps (EV100, spec 3.2): night and day; dusk and an overcast day between.
constexpr float NightEvMin = 4.5f;
constexpr float NightEvMax = 6.5f;
constexpr float DayEvMin = 5.5f;
constexpr float DayEvMax = 7.5f;
// M_Street's pattern for a rolled-down shop gate (street_setup.py).
constexpr float PatShutter = 6.0f;

FLinearColor SrgbHex(uint32 Hex)
{
	return FLinearColor::FromSRGBColor(FColor(static_cast<uint8>((Hex >> 16) & 0xff), static_cast<uint8>((Hex >> 8) & 0xff), static_cast<uint8>(Hex & 0xff)));
}

FVector Along(float Yaw)
{
	const double R = FMath::DegreesToRadians(static_cast<double>(Yaw));
	return FVector(FMath::Cos(R), FMath::Sin(R), 0.0);
}

/** The parameters an imported material glows by: its emissive color factors that aren't black, or else its emissive strengths (as the street's DimImportedGlow reads the glTF importer's). */
bool GlowParams(const UMaterialInterface* Baked, TArray<TPair<FName, FLinearColor>>& Colors, TArray<TPair<FName, float>>& Strengths)
{
	Colors.Reset();
	Strengths.Reset();
	if (!Baked)
	{
		return false;
	}
	auto Key = [](const FName& Name) { return Name.ToString().ToLower().Replace(TEXT("_"), TEXT("")).Replace(TEXT(" "), TEXT("")); };
	bool bFactor = false;
	TArray<FMaterialParameterInfo> Infos;
	TArray<FGuid> Ids;
	Baked->GetAllVectorParameterInfo(Infos, Ids);
	for (const FMaterialParameterInfo& Info : Infos)
	{
		const FString K = Key(Info.Name);
		FLinearColor Value;
		if ((K.StartsWith(TEXT("emissivefactor")) || K == TEXT("emissivecolor")) && Baked->GetVectorParameterValue(FHashedMaterialParameterInfo(Info), Value))
		{
			bFactor = true;
			if (!Value.IsAlmostBlack())
			{
				Colors.Emplace(Info.Name, Value);
			}
		}
	}
	if (!bFactor)
	{
		Infos.Reset();
		Ids.Reset();
		Baked->GetAllScalarParameterInfo(Infos, Ids);
		for (const FMaterialParameterInfo& Info : Infos)
		{
			const FString K = Key(Info.Name);
			float Value = 0.0f;
			if ((K == TEXT("emissivestrength") || K == TEXT("emissiveintensity")) && Baked->GetScalarParameterValue(FHashedMaterialParameterInfo(Info), Value) && Value > 0.0f)
			{
				Strengths.Emplace(Info.Name, Value);
			}
		}
	}
	return Colors.Num() > 0 || Strengths.Num() > 0;
}
} // namespace ShopStageDetail

using namespace ShopStageDetail;

AShopStage::AShopStage()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork; // after the camera has moved (the wake rule reads it)
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Basic(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = Cube.Object;
	FallbackMaterial = Basic.Object;
}

// ------------------------------------------------------------------ building it

AShopStage* AShopStage::FindOrSpawn(UWorld* InWorld, TSubclassOf<AShopStage> Class, const FTransform& StageFrame)
{
	if (!InWorld || !Class || Class->HasAnyClassFlags(CLASS_Abstract))
	{
		return nullptr;
	}
	AShopStage* Shop = nullptr;
	for (TActorIterator<AShopStage> It(InWorld, Class); It && !Shop; ++It)
	{
		Shop = *It;
	}
	if (!Shop)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Shop = InWorld->SpawnActor<AShopStage>(Class, StageFrame, Params);
	}
	if (Shop)
	{
		Shop->EnsureBuilt();
	}
	return Shop;
}

AShopStage* AShopStage::FindOrSpawnVenue(UWorld* InWorld, FName InVenue, const FTransform& StageFrame)
{
	const TSubclassOf<AShopStage> Class = ClassFor(InVenue);
	if (!Class)
	{
		UE_LOG(LogShopStage, Warning, TEXT("No shop stage for the venue %s"), *InVenue.ToString());
		return nullptr;
	}
	return FindOrSpawn(InWorld, Class, StageFrame);
}

TSubclassOf<AShopStage> AShopStage::ClassFor(FName InVenue)
{
	if (InVenue.IsNone())
	{
		return nullptr;
	}
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* C = *It;
		if (!C->IsChildOf(AShopStage::StaticClass()) || C->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			continue;
		}
		const AShopStage* Defaults = C->GetDefaultObject<AShopStage>();
		if (Defaults && Defaults->Venue == InVenue)
		{
			return C;
		}
	}
	return nullptr;
}

void AShopStage::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UWorld* InWorld = GetWorld();
	if (InWorld && InWorld->IsGameWorld() && bBuilt)
	{
		// Construction run again at play has just cleared the room built for play: built again (as the street does).
		bBuilt = false;
		EnsureBuilt();
		return;
	}
	if (!InWorld || InWorld->WorldType != EWorldType::Editor || !bEditorPreview)
	{
		// In play the room is built at BeginPlay (or when it's spawned: FindOrSpawn).
		return;
	}
	ResetParts();
	BuildInterior();
}

void AShopStage::BeginPlay()
{
	Super::BeginPlay();
	EnsureBuilt();
}

void AShopStage::EndPlay(const EEndPlayReason::Type Reason)
{
	Unregister();
	Super::EndPlay(Reason);
}

void AShopStage::ResetParts()
{
	Materials.Reset();
	Lights.Reset();
	Glows.Reset();
	Screens.Reset();
	ScreenSlates.Reset();
	DoorLeaf = nullptr;
	DoorStop = nullptr;
	GrilleRoot = nullptr;
	RoomVolume = nullptr;
	RoomLens = nullptr;
	bHasDoor = false;
	bHasGlass = false;
	RoomBox = FBox(ForceInit);
	GrilleHeight = 0.0;
	DoorAmount = 0.0f;
	bDoorHeld = false;
	DoorClear = 0.0f;
	BuzzIn = -1.0f;
	AppliedDaylight = -1.0f;
	StreetMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ShortStack/Materials/M_Street.M_Street"), nullptr, LOAD_NoWarn | LOAD_Quiet);
}

void AShopStage::EnsureBuilt()
{
	if (bBuilt)
	{
		return;
	}
	UWorld* InWorld = GetWorld();
	if (!InWorld || !InWorld->IsGameWorld())
	{
		return;
	}
	const double Started = FPlatformTime::Seconds();
	// Whatever of the editor's preview came along: gone, deepest first (as the street and the Embercrest clear theirs).
	TInlineComponentArray<UActorComponent*> Old;
	GetComponents(Old);
	Old.RemoveAll([](const UActorComponent* C) { return !C || !C->IsCreatedByConstructionScript(); });
	auto Depth = [](const UActorComponent& C) {
		int32 D = 0;
		for (const USceneComponent* S = Cast<USceneComponent>(&C); S && S->GetAttachParent(); S = S->GetAttachParent())
		{
			++D;
		}
		return D;
	};
	Old.Sort([&Depth](const UActorComponent& A, const UActorComponent& B) { return Depth(A) > Depth(B); });
	for (UActorComponent* C : Old)
	{
		C->DestroyComponent();
	}
	BlueprintCreatedComponents.Reset();
	Unregister();
	ResetParts();
	BuildInterior();
	bBuilt = true;
	GrilleOpen = bOpen ? 1.0f : 0.0f;
	ApplyLights();
	ApplyExposure();
	UE_LOG(LogShopStage, Log, TEXT("%s built in %.0f ms: %d parts, %d lights, %d glows, %d uses"), *VenueName, (FPlatformTime::Seconds() - Started) * 1000.0,
		BlueprintCreatedComponents.Num(), Lights.Num(), Glows.Num(), UseHandles.Num());
}

UStreetWorld* AShopStage::Registry() const
{
	const UWorld* InWorld = GetWorld();
	return InWorld && InWorld->IsGameWorld() ? UStreetWorld::Get(this) : nullptr;
}

void AShopStage::Unregister()
{
	UStreetWorld* R = Registry();
	if (R)
	{
		for (int32 Handle : UseHandles)
		{
			R->RemoveUse(Handle);
		}
		for (const FName& Id : StartIds)
		{
			R->RemoveStart(Id);
		}
		for (const FName& Id : ShotIds)
		{
			R->RemoveShot(Id);
		}
		R->RemoveVenue(Venue);
	}
	UseHandles.Reset();
	StartIds.Reset();
	ShotIds.Reset();
}

UStaticMesh* AShopStage::ShopMesh(const TCHAR* Name)
{
	return LoadObject<UStaticMesh>(nullptr, *FStreetPreload::MeshPath(Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
}

TArray<TPair<FName, float>> AShopStage::Cinematic(FName Service) const
{
	return {};
}

// ------------------------------------------------------------------ helpers

UMaterialInstanceDynamic* AShopStage::Mat(FName Key, uint32 SrgbHexColor, float Roughness, float Pattern, float Emissive, float Metallic)
{
	if (TObjectPtr<UMaterialInstanceDynamic>* Found = Materials.Find(Key))
	{
		return *Found;
	}
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(StreetMaterial ? StreetMaterial.Get() : FallbackMaterial.Get(), this);
	const FLinearColor Color = SrgbHex(SrgbHexColor);
	M->SetVectorParameterValue(TEXT("BaseColor"), Color);
	M->SetVectorParameterValue(TEXT("Color"), Color);
	M->SetScalarParameterValue(TEXT("Roughness"), Roughness);
	M->SetScalarParameterValue(TEXT("Metallic"), Metallic);
	M->SetScalarParameterValue(TEXT("Pattern"), Pattern);
	M->SetScalarParameterValue(TEXT("Emissive"), Emissive);
	M->SetScalarParameterValue(TEXT("Wet"), 0.0f); // indoors
	Materials.Add(Key, M);
	return M;
}

UStaticMeshComponent* AShopStage::Block(UMaterialInterface* Material, const FVector& Min, const FVector& Max, bool bCollide, USceneComponent* Parent)
{
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>(Parent);
	C->SetStaticMesh(CubeMesh);
	// The engine cube is 100 cm on a side, centered on its pivot.
	C->SetRelativeLocation((Min + Max) * 0.5);
	C->SetRelativeScale3D((Max - Min).GetAbs() / 100.0);
	C->SetMaterial(0, Material ? Material : FallbackMaterial.Get());
	if (bCollide)
	{
		C->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	}
	else
	{
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	return C;
}

UStaticMeshComponent* AShopStage::Site(const TCHAR* Name)
{
	UStaticMesh* Asset = ShopMesh(Name);
	if (!Asset)
	{
		return nullptr;
	}
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>();
	C->SetStaticMesh(Asset);
	C->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	return C;
}

UStaticMeshComponent* AShopStage::Mesh(const TCHAR* Name, const FVector& At, float Yaw, bool bCollide)
{
	UStaticMesh* Asset = ShopMesh(Name);
	if (!Asset)
	{
		return nullptr;
	}
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>();
	C->SetStaticMesh(Asset);
	C->SetRelativeLocationAndRotation(At, FRotator(0.0f, Yaw + 180.0f, 0.0f));
	if (bCollide)
	{
		C->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	}
	else
	{
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	return C;
}

ULightComponent* AShopStage::AddLight(ULightComponent* L, float Candela, uint32 SrgbHexColor, EShopLight LightRole)
{
	if (ULocalLightComponent* Local = Cast<ULocalLightComponent>(L))
	{
		Local->SetIntensityUnits(ELightUnits::Candelas);
	}
	L->SetIntensity(Candela);
	L->SetLightColor(SrgbHex(SrgbHexColor));
	// Interior fixtures shadow (MegaLights, the room's volume) and stay out of the fog; the window's spill and the signs
	// light the street's wet air a little and never shadow (spec 3.1).
	const bool bInside = LightRole == EShopLight::Fixture || LightRole == EShopLight::Night;
	L->SetCastShadows(LightRole == EShopLight::Fixture);
	L->SetVolumetricScatteringIntensity(bInside ? 0.0f : LightRole == EShopLight::Spill ? 0.25f : 0.4f);
	if (!bInside)
	{
		// Out on the street they drop out by distance (spec 3.1's budget).
		L->MaxDrawDistance = 4000.0f;
		L->MaxDistanceFadeRange = 600.0f;
		L->MarkRenderStateDirty();
	}
	FLight& Entry = Lights.AddDefaulted_GetRef();
	Entry.Light = L;
	Entry.Base = Candela;
	Entry.Role = LightRole;
	return L;
}

UPointLightComponent* AShopStage::PointLight(const FVector& At, float Candela, uint32 SrgbHexColor, float Radius, EShopLight LightRole)
{
	UPointLightComponent* L = NewPart<UPointLightComponent>();
	L->SetRelativeLocation(At);
	L->SetAttenuationRadius(Radius);
	AddLight(L, Candela, SrgbHexColor, LightRole);
	return L;
}

USpotLightComponent* AShopStage::SpotLight(const FVector& At, const FRotator& Aim, float Candela, uint32 SrgbHexColor, float Radius, float InnerDeg, float OuterDeg, EShopLight LightRole)
{
	USpotLightComponent* L = NewPart<USpotLightComponent>();
	L->SetRelativeLocationAndRotation(At, Aim);
	L->SetAttenuationRadius(Radius);
	L->SetInnerConeAngle(InnerDeg);
	L->SetOuterConeAngle(OuterDeg);
	AddLight(L, Candela, SrgbHexColor, LightRole);
	return L;
}

URectLightComponent* AShopStage::RectLight(const FVector& At, const FRotator& Aim, float Width, float Height, float Candela, uint32 SrgbHexColor, float Radius, EShopLight LightRole)
{
	URectLightComponent* L = NewPart<URectLightComponent>();
	L->SetRelativeLocationAndRotation(At, Aim);
	L->SetSourceWidth(Width);
	L->SetSourceHeight(Height);
	L->SetAttenuationRadius(Radius);
	AddLight(L, Candela, SrgbHexColor, LightRole);
	return L;
}

void AShopStage::Glow(UMaterialInstanceDynamic* Mid, EShopLight LightRole)
{
	if (!Mid)
	{
		return;
	}
	float Emissive = 0.0f;
	Mid->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Emissive")), Emissive);
	FGlow& G = Glows.AddDefaulted_GetRef();
	G.Mid = Mid;
	G.Role = LightRole;
	G.Strengths.Emplace(TEXT("Emissive"), Emissive);
}

void AShopStage::Glow(UStaticMeshComponent* Imported, EShopLight LightRole)
{
	const UStaticMesh* Source = Imported ? Imported->GetStaticMesh().Get() : nullptr;
	if (!Source)
	{
		return;
	}
	TArray<TPair<FName, FLinearColor>> Colors;
	TArray<TPair<FName, float>> Strengths;
	for (int32 Slot = 0; Slot < Source->GetStaticMaterials().Num(); ++Slot)
	{
		UMaterialInterface* Baked = Source->GetMaterial(Slot);
		if (!GlowParams(Baked, Colors, Strengths))
		{
			continue;
		}
		// Its own copy to scale, made at build and never saved (as the street's dimmed fronts).
		UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Baked, Imported);
		Mid->SetFlags(RF_Transient);
		Imported->SetMaterial(Slot, Mid);
		FGlow& G = Glows.AddDefaulted_GetRef();
		G.Mid = Mid;
		G.Mesh = Imported;
		G.Role = LightRole;
		G.Colors = Colors;
		G.Strengths = Strengths;
	}
}

UWidgetComponent* AShopStage::Screen(const FVector& At, float Yaw, const FVector2D& SizeCm, const FIntPoint& Pixels, float Shine, TFunction<void(ss::ui::Canvas&)> Paint)
{
	UWidgetComponent* Wc = NewPart<UWidgetComponent>();
	UMaterialInterface* Lit = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ShortStack/Materials/M_WidgetLit.M_WidgetLit"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	// Printed (a menu, a price card) is lit like the wall it hangs on; a screen or a lightbox glows past white.
	const bool bPrinted = Shine <= 0.0f && Lit;
	Wc->SetWidgetSpace(EWidgetSpace::World);
	Wc->SetDrawSize(FVector2D(Pixels.X, Pixels.Y));
	Wc->SetPivot(FVector2D(0.5, 0.5));
	Wc->SetBlendMode(bPrinted ? EWidgetBlendMode::Opaque : EWidgetBlendMode::Transparent);
	Wc->SetBackgroundColor(bPrinted ? FLinearColor(0.02f, 0.02f, 0.02f, 1.0f) : FLinearColor::Transparent);
	Wc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Wc->SetCastShadow(false);
	Wc->SetRelativeLocationAndRotation(At, FRotator(0.0f, Yaw, 0.0f));
	// The widget's quad lies in its local Y (width) and Z (height), one unit per pixel.
	Wc->SetRelativeScale3D(FVector(1.0, SizeCm.X / FMath::Max(1, Pixels.X), SizeCm.Y / FMath::Max(1, Pixels.Y)));
	if (bPrinted)
	{
		Wc->SetMaterial(0, Lit);
	}
	const float Tint = bPrinted ? 1.0f : FMath::Max(Shine, 0.01f);
	Wc->SetTintColorAndOpacity(FLinearColor(Tint, Tint, Tint, 1.0f));
	Screens.Add(Wc);
	ScreenSlates.Add(nullptr);
	Redraw(Screens.Num() - 1, MoveTemp(Paint));
	return Wc;
}

void AShopStage::Redraw(int32 Index, TFunction<void(ss::ui::Canvas&)> Paint)
{
	UWidgetComponent* Wc = Screens.IsValidIndex(Index) ? Screens[Index].Get() : nullptr;
	if (!Wc || !Paint || !FSlateTextMeasurer::IsAvailable())
	{
		return;
	}
	const FVector2D Draw = Wc->GetDrawSize();
	const FIntPoint Pixels(FMath::RoundToInt32(Draw.X), FMath::RoundToInt32(Draw.Y));
	FSlateTextMeasurer Measurer;
	TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
	ss::ui::Canvas C(*List, Measurer, static_cast<float>(Pixels.X), static_cast<float>(Pixels.Y), 1.0f);
	Paint(C);
	TSharedPtr<SDrawListWidget> Slate = ScreenSlates[Index];
	if (!Slate)
	{
		Slate = SNew(SDrawListWidget).DesiredSize(FVector2D(Pixels.X, Pixels.Y));
		ScreenSlates[Index] = Slate;
		Wc->SetSlateWidget(Slate);
		Wc->SetManuallyRedraw(true);
	}
	Slate->SetDrawList(List);
	Wc->RequestRedraw();
}

int32 AShopStage::Use(FName Id, EStreetVerb Verb, const FVector& At, float Radius, const FString& Prompt, const FString& Param, TFunction<FString()> WhyNot)
{
	UStreetWorld* R = Registry();
	if (!R)
	{
		return INDEX_NONE;
	}
	FStreetUse U;
	U.Id = Id;
	U.Verb = Verb;
	U.Venue = Venue;
	U.Param = Param;
	U.Prompt = Prompt;
	U.Radius = Radius;
	U.Anchor = Root.Get();
	U.Offset = At;
	if (WhyNot)
	{
		U.WhyNot = MoveTemp(WhyNot);
	}
	else
	{
		TWeakObjectPtr<AShopStage> Self(this);
		U.WhyNot = [Self]() { return Self.IsValid() && !Self->bOpen ? Self->ClosedPrompt() : FString(); };
	}
	const int32 Handle = R->AddUse(U);
	UseHandles.Add(Handle);
	return Handle;
}

void AShopStage::Station(FName Id, const FVector& At, float Yaw, FName Pose, float SeatHeight)
{
	if (UStreetWorld* R = Registry())
	{
		R->AddStation(Id, Venue, FTransform(FRotator(0.0f, Yaw, 0.0f), At) * GetActorTransform(), Pose, SeatHeight, true);
	}
}

void AShopStage::Shot(FName Id, const FVector& Camera, const FVector& LookAt, float Fov)
{
	if (UStreetWorld* R = Registry())
	{
		R->AddShot(Id, FTransform((LookAt - Camera).Rotation(), Camera) * GetActorTransform(), Fov);
		ShotIds.AddUnique(Id);
	}
}

void AShopStage::Start(FName Id, const FVector& At, float Yaw)
{
	if (UStreetWorld* R = Registry())
	{
		// As the street's starts: a meter over the floor (the pawn's capsule settles onto it).
		R->AddStart(Id, FTransform(FRotator(0.0f, Yaw, 0.0f), At + FVector(0.0, 0.0, 100.0)) * GetActorTransform());
		StartIds.AddUnique(Id);
	}
}

void AShopStage::Zone(const FBox& Room, const FString& Place, int32 RoomTone, float BoomLength, float Fov, bool bFirstPerson)
{
	RoomBox = Room;
	PlaceName = Place;
	// The room's own exposure (spec 3.2), MegaLights for its fixtures, inside its box only (a short blend at the door).
	RoomVolume = NewPart<UBoxComponent>();
	RoomVolume->SetRelativeLocation(Room.GetCenter());
	RoomVolume->SetBoxExtent(Room.GetExtent());
	RoomVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RoomVolume->SetGenerateOverlapEvents(false);
	RoomVolume->SetHiddenInGame(true);
	RoomLens = NewPart<UPostProcessComponent>(RoomVolume);
	RoomLens->bUnbound = false;
	RoomLens->BlendRadius = 60.0f;
	RoomLens->Priority = 5.0f;
	FPostProcessSettings& S = RoomLens->Settings;
	S.bOverride_bMegaLights = true;
	S.bMegaLights = 1;
	S.bOverride_AutoExposureMinBrightness = true;
	S.bOverride_AutoExposureMaxBrightness = true;
	ApplyExposure();
	UStreetWorld* R = Registry();
	if (!R)
	{
		return;
	}
	FStreetZone Z;
	Z.Id = Venue;
	Z.Box = Room.TransformBy(GetActorTransform());
	Z.Place = Place;
	Z.RoomTone = RoomTone;
	Z.bDry = true;
	Z.bInterior = true;
	Z.Priority = 10;
	Z.BoomLength = BoomLength;
	Z.Fov = Fov;
	Z.bFirstPerson = bFirstPerson;
	TWeakObjectPtr<AShopStage> Self(this);
	Z.DoorOpen = [Self]() { return Self.IsValid() ? Self->DoorAmount : 0.0f; };
	R->AddZone(Z);
}

void AShopStage::Storefront(const FVector& Middle, const FVector& Out)
{
	GlassAt = Middle;
	GlassOut = Out.GetSafeNormal();
	bHasGlass = true;
}

void AShopStage::Door(const FShopDoorSpec& Spec, UStaticMeshComponent* Leaf)
{
	DoorSpec = Spec;
	bHasDoor = true;
	const FVector Shut = Along(Spec.ShutYaw);
	const FVector In = Along(Spec.InYaw);
	const FVector Middle = Spec.Hinge + Shut * (Spec.Width * 0.5);
	// The leaf turns about its hinge: X along it from the hinge to the latch, Y its thickness, Z up.
	DoorLeaf = NewPart<USceneComponent>();
	DoorLeaf->SetRelativeLocationAndRotation(Spec.Hinge, FRotator(0.0f, Spec.ShutYaw, 0.0f));
	if (Leaf)
	{
		Leaf->AttachToComponent(DoorLeaf, FAttachmentTransformRules::KeepWorldTransform);
	}
	else
	{
		// The block-out: a frame round glass (the gym's, the pawn's) or a painted leaf with a glass light (the barber's).
		UMaterialInterface* Frame = Mat(TEXT("DoorFrame"), Spec.bGlass ? 0x1c1e21 : 0x3b4a3f, 0.45f, 0.0f, 0.0f, Spec.bGlass ? 0.6f : 0.0f);
		UMaterialInterface* Pane = Mat(TEXT("DoorGlass"), 0x0b0e12, 0.04f);
		const double W = Spec.Width;
		const double H = Spec.Height;
		const double Rail = Spec.bGlass ? 9.0 : 14.0;
		Block(Frame, FVector(0.0, -2.5, 0.0), FVector(Rail, 2.5, H), false, DoorLeaf);
		Block(Frame, FVector(W - Rail, -2.5, 0.0), FVector(W, 2.5, H), false, DoorLeaf);
		Block(Frame, FVector(Rail, -2.5, H - Rail), FVector(W - Rail, 2.5, H), false, DoorLeaf);
		Block(Frame, FVector(Rail, -2.5, 0.0), FVector(W - Rail, 2.5, Spec.bGlass ? 22.0 : H * 0.45), false, DoorLeaf);
		Block(Pane, FVector(Rail, -0.6, Spec.bGlass ? 22.0 : H * 0.45), FVector(W - Rail, 0.6, H - Rail), false, DoorLeaf)->SetCastShadow(false);
	}
	// The stop: blocks the doorway while the door's near shut (the leaf has no collision of its own, so it never shoves anyone).
	DoorStop = Block(nullptr, FVector(-5.0, -5.0, 0.0), FVector(Spec.Width + 5.0, 5.0, Spec.Height), true, DoorLeaf);
	DoorStop->SetHiddenInGame(true);
	DoorStop->SetVisibility(false);
	DoorStop->AttachToComponent(Root, FAttachmentTransformRules::KeepWorldTransform);
	UStreetWorld* R = Registry();
	if (!R)
	{
		return;
	}
	// E at it (from either side), the crowd's door (outside on the sidewalk facing in), and the start just inside.
	TWeakObjectPtr<AShopStage> Self(this);
	Use(FName(*FString::Printf(TEXT("%s-door"), *Venue.ToString())), EStreetVerb::Door, Middle + FVector(0.0, 0.0, 100.0), 120.0f, DoorPrompt(), FString(),
		[Self]() { return Self.IsValid() && !Self->bOpen && !Self->bSomeoneInside && Self->DoorSpec.Kind != EShopDoorKind::Fob ? Self->ClosedPrompt() : FString(); });
	FStreetDoor D;
	D.Id = Venue;
	D.Outside = FTransform(FRotator(0.0f, Spec.InYaw, 0.0f), Middle - In * 130.0) * GetActorTransform();
	D.Threshold = GetActorTransform().TransformPosition(Middle);
	D.bRoom = true;
	R->AddDoor(D);
	Start(Venue, Middle + In * 150.0, Spec.InYaw);
}

void AShopStage::Grille(const FVector& Min, const FVector& Max)
{
	GrilleRoot = NewPart<USceneComponent>();
	GrilleHeight = FMath::Abs(Max.Z - Min.Z);
	Block(Mat(TEXT("Grille"), 0x6b6e70, 0.5f, PatShutter, 0.0f, 0.4f), Min, Max, false, GrilleRoot);
}

FString AShopStage::DoorPrompt() const
{
	switch (DoorSpec.Kind)
	{
	case EShopDoorKind::Fob:
		return TEXT("Tap your fob");
	case EShopDoorKind::Buzzer:
		return TEXT("Buzz");
	default:
		return TEXT("Open the door");
	}
}

FString AShopStage::ClosedPrompt() const
{
	return ClosedLine.IsEmpty() ? FString::Printf(TEXT("%s - Closed"), *VenueName) : ClosedLine;
}

// ------------------------------------------------------------------ at play

void AShopStage::SetHour(double InWorldMinutes, float InDaylight, bool bInOpen, bool bInStaffed, const FString& InClosedLine)
{
	WorldMinutes = InWorldMinutes;
	Daylight = FMath::Clamp(InDaylight, 0.0f, 1.0f);
	if (!InClosedLine.IsEmpty())
	{
		ClosedLine = InClosedLine;
	}
	const bool bChanged = bInOpen != bOpen || bInStaffed != bStaffed;
	bOpen = bInOpen;
	bStaffed = bInStaffed;
	if (bChanged && bBuilt)
	{
		ApplyLights();
		OnHour();
	}
	if (FMath::Abs(Daylight - AppliedDaylight) > 0.01f)
	{
		ApplyExposure();
	}
}

void AShopStage::SetAwake(bool bInAwake)
{
	if (bInAwake == bAwake)
	{
		return;
	}
	bAwake = bInAwake;
	if (bBuilt)
	{
		ApplyLights();
		OnAwake();
	}
}

void AShopStage::ApplyExposure()
{
	AppliedDaylight = Daylight;
	if (!RoomLens)
	{
		return;
	}
	FPostProcessSettings& S = RoomLens->Settings;
	S.AutoExposureMinBrightness = FMath::Lerp(NightEvMin, DayEvMin, Daylight);
	S.AutoExposureMaxBrightness = FMath::Lerp(NightEvMax, DayEvMax, Daylight);
}

void AShopStage::ApplyLights()
{
	// Asleep, only the two brightest fixtures stay on (unshadowed) with the spill; shut, only the night lights and the
	// all-night signs.
	TArray<int32> Brightest;
	for (int32 I = 0; I < Lights.Num(); ++I)
	{
		if (Lights[I].Role == EShopLight::Fixture)
		{
			Brightest.Add(I);
		}
	}
	Brightest.Sort([this](int32 A, int32 B) { return Lights[A].Base > Lights[B].Base; });
	Brightest.SetNum(FMath::Min(2, Brightest.Num()));
	auto IsOn = [this](EShopLight LightRole, bool bTopFixture) {
		switch (LightRole)
		{
		case EShopLight::Fixture:
			return bOpen && (bAwake || bTopFixture);
		case EShopLight::Night:
			return !bOpen;
		case EShopLight::Spill:
		case EShopLight::Sign:
			return bOpen;
		default:
			return true;
		}
	};
	for (int32 I = 0; I < Lights.Num(); ++I)
	{
		ULightComponent* L = Lights[I].Light.Get();
		if (!L)
		{
			continue;
		}
		const bool bOn = IsOn(Lights[I].Role, Brightest.Contains(I));
		if (L->IsVisible() != bOn)
		{
			L->SetVisibility(bOn);
		}
		const bool bShadow = bOn && bAwake && Lights[I].Role == EShopLight::Fixture;
		if (static_cast<bool>(L->CastShadows) != bShadow)
		{
			L->SetCastShadows(bShadow);
		}
	}
	for (const FGlow& G : Glows)
	{
		UMaterialInstanceDynamic* Mid = G.Mid.Get();
		if (!Mid)
		{
			continue;
		}
		// A glowing fixture stays lit asleep (only its light goes); everything follows the hours.
		const float K = IsOn(G.Role, true) ? 1.0f : 0.0f;
		for (const TPair<FName, FLinearColor>& C : G.Colors)
		{
			Mid->SetVectorParameterValue(C.Key, FLinearColor(C.Value.R * K, C.Value.G * K, C.Value.B * K, C.Value.A));
		}
		for (const TPair<FName, float>& S : G.Strengths)
		{
			Mid->SetScalarParameterValue(S.Key, S.Value * K);
		}
	}
}

bool AShopStage::IsInside(const FVector& At) const
{
	return RoomBox.IsValid && RoomBox.IsInsideOrOn(GetActorTransform().InverseTransformPosition(At));
}

bool AShopStage::AtDoorway(const FVector& Pos, double Reach) const
{
	if (!bHasDoor)
	{
		return false;
	}
	const FVector L = GetActorTransform().InverseTransformPosition(Pos);
	const FVector Shut = Along(DoorSpec.ShutYaw);
	const FVector In = Along(DoorSpec.InYaw);
	const FVector Middle = DoorSpec.Hinge + Shut * (DoorSpec.Width * 0.5);
	const FVector D = L - Middle;
	return FMath::Abs(FVector::DotProduct(D, Shut)) < DoorSpec.Width * 0.5 + 60.0 && FMath::Abs(FVector::DotProduct(D, In)) < Reach && D.Z > -100.0 && D.Z < 400.0;
}

FString AShopStage::OpenDoor(bool bMember)
{
	if (!bHasDoor)
	{
		return FString();
	}
	if (bSomeoneInside)
	{
		// On the way out: the push bar, whatever the hour and the lock (nobody is shut in a closed shop).
		BuzzIn = -1.0f;
		bDoorHeld = true;
		DoorClear = 0.0f;
		return FString();
	}
	switch (DoorSpec.Kind)
	{
	case EShopDoorKind::Fob:
		// The fob opens it at any hour; without one, only while someone's at the desk to buzz you in.
		if (!bMember && !(bOpen && bStaffed))
		{
			return ClosedPrompt();
		}
		break;
	case EShopDoorKind::Buzzer:
		if (!bOpen)
		{
			return ClosedPrompt();
		}
		if (BuzzIn < 0.0f && DoorAmount <= 0.0f)
		{
			BuzzIn = BuzzSeconds; // the keeper looks up and buzzes
		}
		return FString();
	default:
		if (!bOpen)
		{
			return ClosedPrompt();
		}
		break;
	}
	bDoorHeld = true;
	DoorClear = 0.0f;
	return FString();
}

int32 AShopStage::UpdateDoor(const FVector& Someone, float Dt)
{
	if (!bHasDoor || !bBuilt)
	{
		return 0;
	}
	bSomeoneInside = IsInside(Someone);
	if (BuzzIn >= 0.0f)
	{
		BuzzIn -= Dt;
		if (BuzzIn < 0.0f)
		{
			BuzzIn = -1.0f;
			bDoorHeld = true;
			DoorClear = 0.0f;
		}
	}
	// Anyone at the doorway keeps it from shutting on them; anyone in its swing pushes it open again. Walkers open it for
	// themselves while the shop's open (the keeper buzzes them in at the pawn); shut, it's locked to them.
	const bool bNear = AtDoorway(Someone, 190.0);
	const bool bInSwing = AtDoorway(Someone, 80.0);
	bool bWalker = false;
	if (bOpen)
	{
		if (const UStreetWorld* R = Registry())
		{
			const FVector Middle = GetActorTransform().TransformPosition(DoorSpec.Hinge + Along(DoorSpec.ShutYaw) * (DoorSpec.Width * 0.5) + FVector(0.0, 0.0, 90.0));
			bWalker = R->WalkersNear(Middle, DoorSpec.Width * 0.5 + 110.0);
		}
	}
	if (bWalker && !bDoorHeld)
	{
		bDoorHeld = true;
		DoorClear = 0.0f;
	}
	if (bDoorHeld)
	{
		// Its closer takes it once nobody's been at it for a moment.
		DoorClear = bNear || bWalker ? 0.0f : DoorClear + Dt;
		if (DoorClear > 1.0f)
		{
			bDoorHeld = false;
		}
	}
	else if (bInSwing && DoorAmount > 0.05f)
	{
		bDoorHeld = true;
		DoorClear = 0.0f;
	}
	const float Was = DoorAmount;
	if (bDoorHeld && DoorAmount < 1.0f)
	{
		DoorAmount = FMath::FInterpTo(DoorAmount, 1.0f, Dt, 5.0f);
		if (DoorAmount > 0.998f)
		{
			DoorAmount = 1.0f;
		}
		DoorAmount = FMath::Max(DoorAmount, 0.001f);
	}
	else if (!bDoorHeld && DoorAmount > 0.0f)
	{
		DoorAmount = FMath::FInterpConstantTo(DoorAmount, 0.0f, Dt, 0.85f);
	}
	if (Was == DoorAmount)
	{
		return 0;
	}
	if (DoorLeaf)
	{
		DoorLeaf->SetRelativeRotation(FRotator(0.0f, DoorSpec.ShutYaw + DoorSpec.OpenYaw * FMath::InterpEaseInOut(0.0f, 1.0f, DoorAmount, 1.6f), 0.0f));
	}
	if (DoorStop)
	{
		const bool bBlock = DoorAmount < 0.3f;
		if ((DoorStop->GetCollisionEnabled() != ECollisionEnabled::NoCollision) != bBlock)
		{
			DoorStop->SetCollisionEnabled(bBlock ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		}
	}
	if (Was <= 0.0f)
	{
		return 1;
	}
	return DoorAmount <= 0.0f ? 2 : 0;
}

void AShopStage::UpdateWake(float Dt)
{
	WakeCheckIn -= Dt;
	if (WakeCheckIn > 0.0f)
	{
		return;
	}
	WakeCheckIn = WakeEvery;
	if (AwakeHold >= 0)
	{
		SetAwake(AwakeHold != 0);
		return;
	}
	const UWorld* InWorld = GetWorld();
	const APlayerController* Pc = InWorld ? InWorld->GetFirstPlayerController() : nullptr;
	const APlayerCameraManager* Cam = Pc ? Pc->PlayerCameraManager.Get() : nullptr;
	if (!Cam || !RoomBox.IsValid)
	{
		return;
	}
	const FVector Eye = Cam->GetCameraLocation();
	const FVector Hero = Pc->GetPawn() ? Pc->GetPawn()->GetActorLocation() : Eye;
	const FBox World = RoomBox.TransformBy(GetActorTransform());
	const double Far = FMath::Sqrt(World.ComputeSquaredDistanceToPoint(Eye));
	bool bWake = IsInside(Hero) || Far <= WakeNear;
	if (!bWake && bHasGlass && Far <= WakeSeen)
	{
		// Its glass in view: the camera on the street side of it, the storefront's middle inside the view's cone.
		const FVector Glass = GetActorTransform().TransformPosition(GlassAt);
		const FVector Out = GetActorTransform().TransformVectorNoScale(GlassOut);
		const FVector To = (Glass - Eye).GetSafeNormal();
		const double HalfFov = FMath::DegreesToRadians(FMath::Min(Cam->GetFOVAngle() * 0.5 + 10.0, 89.0));
		bWake = FVector::DotProduct(Eye - Glass, Out) > 0.0 && FVector::DotProduct(Cam->GetCameraRotation().Vector(), To) > FMath::Cos(HalfFov);
	}
	SetAwake(bWake);
}

void AShopStage::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bBuilt)
	{
		return;
	}
	const float Dt = FMath::Min(DeltaSeconds, 0.1f);
	UpdateWake(Dt);
	// The grille rolls up when it opens and down when it shuts (hidden once it's up in its box).
	if (GrilleRoot && GrilleHeight > 0.0)
	{
		const float Want = bOpen ? 1.0f : 0.0f;
		if (GrilleOpen != Want)
		{
			GrilleOpen = FMath::FInterpConstantTo(GrilleOpen, Want, Dt, 0.6f);
			GrilleRoot->SetRelativeLocation(FVector(0.0, 0.0, GrilleOpen * GrilleHeight));
		}
		const bool bShow = GrilleOpen < 0.999f;
		if (GrilleRoot->IsVisible() != bShow)
		{
			GrilleRoot->SetVisibility(bShow, true);
		}
	}
	if (bAwake)
	{
		Animate(Dt);
	}
}

FString AShopStage::Describe() const
{
	return FString::Printf(TEXT("%s: %s%s, %s, door %.2f%s, %d lights"), *VenueName, bOpen ? TEXT("open") : TEXT("shut"), bOpen && !bStaffed ? TEXT(" (unstaffed)") : TEXT(""),
		bAwake ? TEXT("awake") : TEXT("asleep"), DoorAmount, BuzzIn >= 0.0f ? TEXT(" (buzzing)") : TEXT(""), Lights.Num());
}
