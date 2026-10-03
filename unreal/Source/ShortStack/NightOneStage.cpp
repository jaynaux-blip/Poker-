#include "NightOneStage.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/LocalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ShortStack/UI/PropArt.h"
#include "SlateDrawList.h"
#include "UObject/ConstructorHelpers.h"

namespace NightOneStageDetail
{
/** Prototype meters (x right, y up, z toward the viewer) to Unreal centimeters. */
FVector Web(double X, double Y, double Z)
{
	return FVector(-Z * 100.0, X * 100.0, Y * 100.0);
}

/** Rotation whose forward (+X) is Normal and up (+Z) is Up. */
FRotator Facing(const FVector& Normal, const FVector& Up)
{
	return FRotationMatrix::MakeFromXZ(Normal, Up).Rotator();
}

FLinearColor Srgb(uint32 Hex)
{
	return FLinearColor::FromSRGBColor(FColor(static_cast<uint8>((Hex >> 16) & 0xff), static_cast<uint8>((Hex >> 8) & 0xff), static_cast<uint8>(Hex & 0xff)));
}

// Room (prototype meters).
const double RoomFront = -0.95;
// The building across the street at night: its street lamp and the neon's spill on the brick (candelas).
const float FacadeLampCandela = 1100.0f;
const float FacadeGlowCandela = 120.0f;
const float NeonSpillCandela = 160.0f;
const double RoomBack = 3.1;
const double RoomLeft = -1.75;
const double RoomRight = 2.35;
const double RoomCeiling = 2.6;
const double DeskTop = 0.75;
const double DeskZ = RoomFront + 0.02 + 0.35;
const double WinX0 = -0.72;
const double WinX1 = 0.72;
const double WinY0 = 0.98;
const double WinY1 = 2.12;

// Surface patterns understood by M_Surface (Content/Python/init_unreal.py).
const float PatternNone = 0.0f;
const float PatternPlaster = 1.0f;
const float PatternWood = 2.0f;
const float PatternFloor = 3.0f;
const float PatternBrushed = 4.0f;
const float PatternFabric = 5.0f;

UMaterialInterface* LoadOptional(const TCHAR* Path)
{
	return LoadObject<UMaterialInterface>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
}

UStaticMesh* LoadOptionalMesh(const TCHAR* Path)
{
	return LoadObject<UStaticMesh>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
}

// The laptop (art/blender/assets/laptop.py): depth of the base, and its key grid (meters).
const double LaptopDepth = 0.235;
const double KeyUnit = 0.01905;
const double KeysLeft = -7.5 * KeyUnit;
const double KeyTop = 0.0172;

/** Center of a key in the laptop base's own coordinates (x right, y back from the hinge, z up): row 0 is the number row. */
FVector LaptopKey(double Units, int32 Row, double Nudge = 0.0)
{
	return FVector(KeysLeft + Units * KeyUnit, -0.019 - 0.012 - (Row + 0.5) * KeyUnit + Nudge, KeyTop);
}

/** A prop imported by the editor setup script: /Game/ShortStack/Meshes/<Name>/<Name>. */
UStaticMesh* LoadProp(const TCHAR* Name)
{
	return LoadOptionalMesh(*FString::Printf(TEXT("/Game/ShortStack/Meshes/%s/%s.%s"), Name, Name, Name));
}
} // namespace NightOneStageDetail

using namespace NightOneStageDetail;

ANightOneStage::ANightOneStage()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Basic(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = Cube.Object;
	CylinderMesh = Cylinder.Object;
	SphereMesh = Sphere.Object;
	PlaneMesh = Plane.Object;
	ConeMesh = Cone.Object;
	FallbackMaterial = Basic.Object;
}

// ------------------------------------------------------------------ building blocks

template <typename T>
T* ANightOneStage::NewPart(USceneComponent* Parent)
{
	T* Part = NewObject<T>(this);
	Part->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	Part->SetupAttachment(Parent ? Parent : Root.Get());
	BlueprintCreatedComponents.Add(Part);
	Part->RegisterComponent();
	return Part;
}

UMaterialInstanceDynamic* ANightOneStage::Surface(FName Key, uint32 SrgbHex, float Roughness, float Metallic, float Pattern, float Emissive)
{
	if (TObjectPtr<UMaterialInstanceDynamic>* Found = Materials.Find(Key))
	{
		return *Found;
	}
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(SurfaceMaterial ? SurfaceMaterial.Get() : FallbackMaterial.Get(), this);
	const FLinearColor Color = Srgb(SrgbHex);
	M->SetVectorParameterValue(TEXT("BaseColor"), Color);
	M->SetVectorParameterValue(TEXT("Color"), Color); // BasicShapeMaterial fallback
	M->SetScalarParameterValue(TEXT("Roughness"), Roughness);
	M->SetScalarParameterValue(TEXT("Metallic"), Metallic);
	M->SetScalarParameterValue(TEXT("Pattern"), Pattern);
	M->SetScalarParameterValue(TEXT("Emissive"), Emissive);
	Materials.Add(Key, M);
	return M;
}

UMaterialInstanceDynamic* ANightOneStage::Special(UMaterialInterface* Parent, FName Key)
{
	if (!Parent)
	{
		return nullptr;
	}
	if (TObjectPtr<UMaterialInstanceDynamic>* Found = Materials.Find(Key))
	{
		return *Found;
	}
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Parent, this);
	Materials.Add(Key, M);
	return M;
}

UStaticMeshComponent* ANightOneStage::AddMesh(UStaticMesh* Mesh, UMaterialInterface* Material, const FVector& Location, const FVector& Scale, const FRotator& Rotation, USceneComponent* Parent, bool bShadows)
{
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>(Parent);
	C->SetStaticMesh(Mesh);
	C->SetRelativeLocationAndRotation(Location, Rotation);
	C->SetRelativeScale3D(Scale);
	if (Material)
	{
		C->SetMaterial(0, Material);
	}
	C->SetCastShadow(bShadows);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (!bShadows)
	{
		// Decorative far geometry (sky dome, rain sheets, glass) stays out of Lumen's distance fields:
		// a mesh distance field for the dome would put the whole room "inside" a solid.
		C->bAffectDistanceFieldLighting = false;
		C->MarkRenderStateDirty();
	}
	return C;
}

UStaticMeshComponent* ANightOneStage::BoxMinMax(UMaterialInterface* Material, const FVector& Min, const FVector& Max, bool bShadows)
{
	// The engine cube is 100 cm on a side, centered on its pivot.
	return AddMesh(CubeMesh, Material, (Min + Max) * 0.5, (Max - Min) / 100.0, FRotator::ZeroRotator, nullptr, bShadows);
}

UStaticMeshComponent* ANightOneStage::BoxWeb(UMaterialInterface* Material, const FVector& WebCenter, const FVector& WebSize, float YawDeg, USceneComponent* Parent)
{
	return AddMesh(CubeMesh, Material, Web(WebCenter.X, WebCenter.Y, WebCenter.Z), FVector(WebSize.Z, WebSize.X, WebSize.Y), FRotator(0.0f, YawDeg, 0.0f), Parent);
}

UStaticMeshComponent* ANightOneStage::CylinderWeb(UMaterialInterface* Material, const FVector& WebBase, float Radius, float Height, USceneComponent* Parent)
{
	// The engine cylinder is 100 cm across and 100 cm tall, centered on its pivot.
	return AddMesh(CylinderMesh, Material, Web(WebBase.X, WebBase.Y + Height * 0.5, WebBase.Z), FVector(Radius * 2.0f, Radius * 2.0f, Height), FRotator::ZeroRotator, Parent);
}

UWidgetComponent* ANightOneStage::AddWidget(const FVector& Location, const FRotator& Rotation, const FVector2D& SizeCm, const FIntPoint& Pixels, bool bLit, bool bTranslucent, USceneComponent* Parent)
{
	UWidgetComponent* Wc = NewPart<UWidgetComponent>(Parent);
	Wc->SetWidgetSpace(EWidgetSpace::World);
	Wc->SetDrawSize(FVector2D(Pixels.X, Pixels.Y));
	Wc->SetPivot(FVector2D(0.5, 0.5));
	Wc->SetBlendMode(bTranslucent ? EWidgetBlendMode::Transparent : EWidgetBlendMode::Opaque);
	Wc->SetTwoSided(false);
	Wc->SetBackgroundColor(bTranslucent ? FLinearColor::Transparent : FLinearColor::Black);
	Wc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Wc->SetCastShadow(false);
	Wc->SetRelativeLocationAndRotation(Location, Rotation);
	// The widget quad lies in its local Y (width) / Z (height) plane, one unit per pixel.
	Wc->SetRelativeScale3D(FVector(1.0, SizeCm.X / Pixels.X, SizeCm.Y / Pixels.Y));
	if (bLit && WidgetLitMaterial)
	{
		Wc->SetMaterial(0, WidgetLitMaterial);
	}
	return Wc;
}

// ------------------------------------------------------------------ construction

void ANightOneStage::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildSet();
}

void ANightOneStage::RebuildSet()
{
#if WITH_EDITOR
	// Destroys the set's components and runs OnConstruction again.
	RerunConstructionScripts();
#endif
}

void ANightOneStage::BuildSet()
{
	Materials.Reset();
	TimedMaterials.Reset();
	PropWidgets.Reset();
	Cans.Reset();
	SurfaceMaterial = LoadOptional(TEXT("/Game/ShortStack/Materials/M_Surface.M_Surface"));
	EmissiveMaterial = LoadOptional(TEXT("/Game/ShortStack/Materials/M_Emissive.M_Emissive"));
	GlowMaterial = LoadOptional(TEXT("/Game/ShortStack/Materials/M_Glow.M_Glow"));
	GlassMaterial = LoadOptional(TEXT("/Game/ShortStack/Materials/M_RainGlass.M_RainGlass"));
	CityMaterial = LoadOptional(TEXT("/Game/ShortStack/Materials/M_City.M_City"));
	SkyMaterial = LoadOptional(TEXT("/Game/ShortStack/Materials/M_Sky.M_Sky"));
	RainMaterial = LoadOptional(TEXT("/Game/ShortStack/Materials/M_Rain.M_Rain"));
	WidgetLitMaterial = LoadOptional(TEXT("/Game/ShortStack/Materials/M_WidgetLit.M_WidgetLit"));
	RainCookieMaterial = LoadOptional(TEXT("/Game/ShortStack/Materials/M_RainCookie.M_RainCookie"));
	CanMesh = LoadProp(TEXT("SM_EnergyCan"));
	TenementMesh = LoadProp(TEXT("SM_Tenement"));
	LaptopBaseMesh = LoadProp(TEXT("SM_Laptop_Base"));
	LaptopLidMesh = LoadProp(TEXT("SM_Laptop_Lid"));
	DeskMesh = LoadProp(TEXT("SM_Desk"));
	MugMesh = LoadProp(TEXT("SM_Mug"));
	PhoneMesh = LoadProp(TEXT("SM_Phone"));
	ChipsMesh = LoadProp(TEXT("SM_ChipStacks"));
	LampMesh = LoadProp(TEXT("SM_DeskLamp"));
	ChairMesh = LoadProp(TEXT("SM_Chair"));
	MouseMesh = LoadProp(TEXT("SM_Mouse"));
	MousePadMesh = LoadProp(TEXT("SM_MousePad"));
	ArmsMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/ShortStack/Meshes/SK_Arms/SK_Arms.SK_Arms"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	// Every Blender prop is modeled front toward -Y. The laptop base runs forward from its hinge,
	// so its bounds say which way the importer turned that; the same yaw sets all of them facing the chair (-X).
	ImportYaw = 0.0f;
	if (LaptopBaseMesh)
	{
		const FVector Ahead = LaptopBaseMesh->GetBoundingBox().GetCenter() * FVector(1.0, 1.0, 0.0);
		ImportYaw = 180.0f - static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Ahead.Y, Ahead.X)));
	}
	BuildShell();
	BuildWindow();
	BuildDesk();
	BuildProps();
	BuildOutside();
	BuildLights();
	BuildLeds();
	BuildGear();
}

void ANightOneStage::BuildShell()
{
	UMaterialInterface* Wall = Surface(TEXT("Wall"), 0xa1a394, 0.9f, 0.0f, PatternPlaster);
	UMaterialInterface* Floor = Surface(TEXT("Floor"), 0x5a3d27, 0.7f, 0.0f, PatternFloor);
	UMaterialInterface* CeilingMat = Surface(TEXT("RoomCeiling"), 0x8f8d86, 0.95f);
	UMaterialInterface* Trim = Surface(TEXT("Trim"), 0x6f6a5f, 0.6f);
	const FVector Fr = Web(0, 0, RoomFront);
	const double XFront = Fr.X;            // 95
	const double XBack = Web(0, 0, RoomBack).X;  // -310
	const double YLeft = RoomLeft * 100.0;     // -175
	const double YRight = RoomRight * 100.0;   // 235
	const double ZTop = RoomCeiling * 100.0;   // 260
	const double T = 20.0;                 // walls are thick so Lumen does not leak light
	const double Reveal = 16.0;            // window depth
	BoxMinMax(Floor, FVector(XBack - T, YLeft - T, -T), FVector(XFront + Reveal, YRight + T, 0.0));
	BoxMinMax(CeilingMat, FVector(XBack - T, YLeft - T, ZTop), FVector(XFront + Reveal, YRight + T, ZTop + T));
	BoxMinMax(Wall, FVector(XBack - T, YLeft - T, 0.0), FVector(XBack, YRight + T, ZTop));
	BoxMinMax(Wall, FVector(XBack, YLeft - T, 0.0), FVector(XFront, YLeft, ZTop));
	BoxMinMax(Wall, FVector(XBack, YRight, 0.0), FVector(XFront, YRight + T, ZTop));
	// Front wall around the window opening.
	const double Y0 = WinX0 * 100.0;
	const double Y1 = WinX1 * 100.0;
	const double Z0 = WinY0 * 100.0;
	const double Z1 = WinY1 * 100.0;
	BoxMinMax(Wall, FVector(XFront, YLeft - T, 0.0), FVector(XFront + Reveal, Y0, ZTop));
	BoxMinMax(Wall, FVector(XFront, Y1, 0.0), FVector(XFront + Reveal, YRight + T, ZTop));
	BoxMinMax(Wall, FVector(XFront, Y0, 0.0), FVector(XFront + Reveal, Y1, Z0));
	BoxMinMax(Wall, FVector(XFront, Y0, Z1), FVector(XFront + Reveal, Y1, ZTop));
	// Baseboards.
	BoxMinMax(Trim, FVector(XBack, YLeft, 0.0), FVector(XBack + 1.5, YRight, 10.0));
	BoxMinMax(Trim, FVector(XBack, YLeft, 0.0), FVector(XFront, YLeft + 1.5, 10.0));
	BoxMinMax(Trim, FVector(XBack, YRight - 1.5, 0.0), FVector(XFront, YRight, 10.0));
}

void ANightOneStage::BuildWindow()
{
	const double Wx = (WinX0 + WinX1) / 2.0;
	const double Wy = (WinY0 + WinY1) / 2.0;
	const double Ww = WinX1 - WinX0;
	const double Wh = WinY1 - WinY0;
	const double Depth = 0.16;
	BoxWeb(Surface(TEXT("Sill"), 0x9a968c, 0.5f), FVector(Wx, WinY0 - 0.015, RoomFront - Depth / 2.0 + 0.035), FVector(Ww + 0.12, 0.03, Depth + 0.07));
	UMaterialInterface* Frame = Surface(TEXT("Frame"), 0xd8d3c6, 0.55f);
	const double FrameZ = RoomFront - Depth + 0.03;
	auto Bar = [&](double Bw, double Bh, double X, double Y) { BoxWeb(Frame, FVector(X, Y, FrameZ), FVector(Bw, Bh, 0.05)); };
	Bar(Ww, 0.05, Wx, WinY0 + 0.025);
	Bar(Ww, 0.05, Wx, WinY1 - 0.025);
	Bar(0.05, Wh, WinX0 + 0.025, Wy);
	Bar(0.05, Wh, WinX1 - 0.025, Wy);
	Bar(0.04, Wh, Wx, Wy);        // mullion
	Bar(Ww, 0.035, Wx, Wy + 0.08); // transom rail
	// Rain-streaked glass (the engine plane faces +Z; pitch 90 turns it toward the room).
	if (UMaterialInstanceDynamic* Glass = Special(GlassMaterial, TEXT("Glass")))
	{
		Glass->SetScalarParameterValue(TEXT("Aspect"), static_cast<float>((Ww - 0.06) / (Wh - 0.06)));
		TimedMaterials.Add(Glass);
		AddMesh(PlaneMesh, Glass, Web(Wx, Wy, FrameZ - 0.01), FVector(Wh - 0.06, Ww - 0.06, 1.0), FRotator(90.0f, 0.0f, 0.0f), nullptr, false);
	}
}

void ANightOneStage::BuildDesk()
{
	if (DeskMesh)
	{
		// Walnut top on a steel frame (art/blender/assets/desk.py); its origin is on the floor under the top's middle.
		AddMesh(DeskMesh, nullptr, Web(0.0, 0.0, DeskZ), FVector(1.0), FRotator(0.0f, ImportYaw, 0.0f));
	}
	else
	{
		UMaterialInterface* Wood = Surface(TEXT("Desk"), 0x5c3821, 0.6f, 0.0f, PatternWood);
		UMaterialInterface* Legs = Surface(TEXT("Legs"), 0x1c1d20, 0.5f, 0.7f);
		BoxWeb(Wood, FVector(0.0, DeskTop - 0.0175, DeskZ), FVector(1.6, 0.035, 0.7));
		for (const FVector2D& P : {FVector2D(-0.76, DeskZ - 0.3), FVector2D(0.76, DeskZ - 0.3), FVector2D(-0.76, DeskZ + 0.3), FVector2D(0.76, DeskZ + 0.3)})
		{
			BoxWeb(Legs, FVector(P.X, (DeskTop - 0.035) / 2.0, P.Y), FVector(0.035, DeskTop - 0.035, 0.035));
		}
	}
	if (ChairMesh)
	{
		// The chair the player sits in, turned to the desk (a little off square, as chairs are left).
		AddMesh(ChairMesh, nullptr, Web(0.02, 0.0, DeskZ + 0.74), FVector(1.0), FRotator(0.0f, ImportYaw + 186.0f, 0.0f));
	}

	// Laptop, hinged at the back edge with the lid tilted back.
	USceneComponent* Laptop = NewPart<USceneComponent>();
	LaptopFrame = Laptop;
	Laptop->SetRelativeLocation(Web(0.0, DeskTop, DeskZ + 0.04));
	const double D = 0.235;
	const float LidTilt = -18.3f;
	// Where the display sits on the lid, from the hinge (m): its center's height and how far its face is ahead.
	double ScreenUp = 0.225 / 2.0 + 0.004;
	double ScreenAhead = 0.0006;
	Keyboard = nullptr;
	if (LaptopBaseMesh && LaptopLidMesh)
	{
		// The Blender laptop (art/blender/assets/laptop.py): both meshes have their origin on the hinge line,
		// the base's on the desk under it.
		const float Yaw = ImportYaw;
		const double HingeHeight = 0.0185;
		AddMesh(LaptopBaseMesh, nullptr, Web(0.0, 0.0, -D / 2.0), FVector(1.0), FRotator(0.0f, Yaw, 0.0f), Laptop);
		LidPivot = NewPart<USceneComponent>(Laptop);
		LidPivot->SetRelativeLocationAndRotation(Web(0.0, HingeHeight, -D / 2.0), FRotator(LidTilt, 0.0f, 0.0f));
		AddMesh(LaptopLidMesh, nullptr, FVector::ZeroVector, FVector(1.0), FRotator(0.0f, Yaw, 0.0f), LidPivot);
		ScreenUp = 0.11525;
		ScreenAhead = 0.0026 + 0.0003; // just in front of the glass
	}
	else
	{
		UMaterialInterface* Body = Surface(TEXT("Laptop"), 0x2b2d31, 0.38f, 0.85f, PatternBrushed);
		const double T = 0.016;
		const double LaptopW = 0.34;
		BoxWeb(Body, FVector(0.0, T / 2.0, 0.0), FVector(LaptopW, T, D), 0.0f, Laptop);
		BoxWeb(Surface(TEXT("Trackpad"), 0x1f2124, 0.45f, 0.5f), FVector(0.0, T + 0.0005, 0.068), FVector(0.11, 0.001, 0.07), 0.0f, Laptop);
		Keyboard = AddWidget(Web(0.0, T + 0.0008, -0.035), Facing(FVector::UpVector, FVector::ForwardVector), FVector2D(31.0, 11.8), FIntPoint(1024, 390), true, false, Laptop);
		LidPivot = NewPart<USceneComponent>(Laptop);
		LidPivot->SetRelativeLocationAndRotation(Web(0.0, T, -D / 2.0), FRotator(LidTilt, 0.0f, 0.0f));
		const double LidH = 0.225;
		BoxWeb(Body, FVector(0.0, LidH / 2.0, -0.0035), FVector(LaptopW, LidH, 0.007), 0.0f, LidPivot);
		BoxWeb(Surface(TEXT("Bezel"), 0x050506, 0.35f, 0.2f), FVector(0.0, LidH / 2.0, 0.00015), FVector(LaptopW - 0.006, LidH - 0.006, 0.0003), 0.0f, LidPivot);
	}
	Screen = AddWidget(Web(0.0, ScreenUp, ScreenAhead), FRotator(0.0f, 180.0f, 0.0f), ScreenSize(), ScreenResolution, false, false, LidPivot);
	// Slightly below full white so only the brightest pixels bloom, like a real panel.
	Screen->SetTintColorAndOpacity(FLinearColor(0.86f, 0.86f, 0.86f, 1.0f));
	Screen->SetRedrawTime(1.0f / 30.0f);

	// Mouse and pad. The mouse moves with the pointer (UpdateArms).
	double PadTop = 0.003;
	if (MousePadMesh)
	{
		AddMesh(MousePadMesh, nullptr, Web(0.33, DeskTop, DeskZ + 0.13), FVector(1.0), FRotator(0.0f, ImportYaw, 0.0f));
		PadTop = 0.004;
	}
	else
	{
		BoxWeb(Surface(TEXT("MousePad"), 0x131417, 0.95f), FVector(0.33, DeskTop + 0.0015, DeskZ + 0.13), FVector(0.26, 0.003, 0.21));
	}
	MouseRoot = NewPart<USceneComponent>();
	MouseHome = Web(0.34, DeskTop + PadTop, DeskZ + 0.15);
	MouseRoot->SetRelativeLocation(MouseHome);
	if (MouseMesh)
	{
		AddMesh(MouseMesh, nullptr, FVector::ZeroVector, FVector(1.0), FRotator(0.0f, ImportYaw, 0.0f), MouseRoot);
		MouseTop = 3.9f;
	}
	else
	{
		AddMesh(SphereMesh, Surface(TEXT("Mouse"), 0x1d1e22, 0.35f), Web(0.0, 0.012 - PadTop, 0.0), FVector(0.099, 0.06, 0.027), FRotator::ZeroRotator, MouseRoot);
		MouseTop = 2.6f;
	}

	// The player's arms, rooted between the shoulders (UpdateArms keeps them with the head).
	ArmsRoot = NewPart<USceneComponent>();
	ArmsComp = nullptr;
	if (ArmsMesh)
	{
		ArmsComp = NewPart<UPoseableMeshComponent>(ArmsRoot);
		ArmsComp->SetSkinnedAssetAndUpdate(ArmsMesh);
		ArmsComp->SetRelativeRotation(FRotator(0.0f, ImportYaw, 0.0f));
		ArmsComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ArmsComp->SetCastShadow(true);
		ArmsComp->bCastHiddenShadow = false;
		ArmsComp->SetVisibility(false);
	}

	// Phone, face up; its lock screen lights when a text arrives.
	USceneComponent* Phone = NewPart<USceneComponent>();
	Phone->SetRelativeLocationAndRotation(Web(0.28, DeskTop, DeskZ - 0.12), FRotator(0.0f, 14.3f, 0.0f));
	double GlassHeight = 0.008;
	if (PhoneMesh)
	{
		// In its worn case, glass cracked (art/blender/assets/phone.py); the lock screen lies on the display.
		AddMesh(PhoneMesh, nullptr, FVector::ZeroVector, FVector(1.0), FRotator(0.0f, ImportYaw, 0.0f), Phone);
		GlassHeight = 0.0094;
	}
	else
	{
		BoxWeb(Surface(TEXT("Phone"), 0x15161a, 0.3f, 0.5f), FVector(0.0, 0.004, 0.0), FVector(0.074, 0.008, 0.155), 0.0f, Phone);
	}
	PhoneScreen = AddWidget(Web(0.0, GlassHeight + 0.0002, 0.0), Facing(FVector::UpVector, FVector::ForwardVector), FVector2D(6.8, 14.6), FIntPoint(360, 760), false, false, Phone);
	PhoneScreen->SetTintColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
	PhoneLight = NewPart<UPointLightComponent>(Phone);
	PhoneLight->SetRelativeLocation(FVector(0.0, 0.0, 5.0));
	PhoneLight->SetIntensityUnits(ELightUnits::Candelas);
	PhoneLight->SetIntensity(0.0f);
	PhoneLight->SetLightColor(Srgb(0x9fc4ff));
	PhoneLight->SetAttenuationRadius(80.0f);
	PhoneLight->SetCastShadows(false);
}

void ANightOneStage::BuildProps()
{
	// First empty energy drink (one more per hour of grinding).
	AddCan();

	// Mug of coffee, its handle to the left.
	if (MugMesh)
	{
		AddMesh(MugMesh, nullptr, Web(-0.4, DeskTop, DeskZ + 0.06), FVector(1.0), FRotator(0.0f, ImportYaw, 0.0f));
	}
	else
	{
		UMaterialInterface* Ceramic = Surface(TEXT("Mug"), 0xd8d2c4, 0.25f);
		CylinderWeb(Ceramic, FVector(-0.4, DeskTop, DeskZ + 0.06), 0.041f, 0.094f);
		CylinderWeb(Surface(TEXT("Coffee"), 0x1a0d06, 0.05f), FVector(-0.4, DeskTop + 0.094, DeskZ + 0.06), 0.035f, 0.001f);
		BoxWeb(Ceramic, FVector(-0.4 - 0.045, DeskTop + 0.048, DeskZ + 0.06 - 0.01), FVector(0.012, 0.05, 0.01), -137.0f);
	}
	// Chips from the Tuesday game at the laundromat, stacked by the laptop.
	if (ChipsMesh)
	{
		AddMesh(ChipsMesh, nullptr, Web(-0.265, DeskTop, DeskZ - 0.1), FVector(1.0), FRotator(0.0f, ImportYaw - 12.0f, 0.0f));
	}

	// Cup of ramen.
	CylinderWeb(Surface(TEXT("Noodles"), 0xd63a1f, 0.6f), FVector(-0.58, DeskTop, DeskZ - 0.18), 0.045f, 0.1f);
	CylinderWeb(Surface(TEXT("NoodleBand"), 0xfff3d6, 0.6f), FVector(-0.58, DeskTop + 0.03, DeskZ - 0.18), 0.0455f, 0.04f);
	CylinderWeb(Surface(TEXT("NoodleLid"), 0xe9e4da, 0.4f, 0.3f), FVector(-0.58, DeskTop + 0.1, DeskZ - 0.18), 0.05f, 0.002f);

	// Papers (drawn by ShortStackCore's PropArt at BeginPlay): notice and bill on the desk, notice on the door, poster, sticky notes.
	auto FlatUp = [](double Radians) { return Facing(FVector::UpVector, FVector(FMath::Cos(Radians), -FMath::Sin(Radians), 0.0)); };
	PropWidgets.Add(AddWidget(Web(-0.33, DeskTop + 0.002, DeskZ + 0.18), FlatUp(0.35), FVector2D(21.0, 29.7), FIntPoint(700, 990), true, false));
	PropWidgets.Add(AddWidget(Web(-0.55, DeskTop + 0.001, DeskZ + 0.14), FlatUp(-0.2), FVector2D(20.0, 26.0), FIntPoint(700, 910), true, false));
	PropWidgets.Add(AddWidget(Web(1.35, 1.5, RoomBack - 0.06), Facing(FVector::ForwardVector, FVector::UpVector), FVector2D(21.0, 29.7), FIntPoint(700, 990), true, false));
	PropWidgets.Add(AddWidget(Web(RoomLeft + 0.006, 1.55, 0.55), Facing(FVector::RightVector, FVector::UpVector), FVector2D(60.0, 90.0), FIntPoint(600, 900), true, false));
	const double NoteX[3] = {-0.88, -0.95, 0.88};
	const double NoteY[3] = {1.38, 1.22, 1.33};
	const double NoteRot[3] = {0.05, -0.08, 0.1};
	for (int32 I = 0; I < 3; ++I)
	{
		const FVector Up(0.0, -FMath::Sin(NoteRot[I]), FMath::Cos(NoteRot[I]));
		PropWidgets.Add(AddWidget(Web(NoteX[I], NoteY[I], RoomFront + 0.003), Facing(FVector::BackwardVector, Up), FVector2D(7.6, 7.6), FIntPoint(256, 256), true, false));
	}

	// Desk lamp (off): a dark silhouette against the window. The Blender lamp's cable runs back and
	// drops off the desk's rear edge, 11 cm behind its base.
	LampComp = nullptr;
	LampOnDesk = Web(-0.66, DeskTop, DeskZ - 0.24);
	// With the 27" monitor's arm there, the lamp goes up onto the windowsill (SetGear), clear of the recess's side wall.
	LampOnSill = Web(-0.6, WinY0, RoomFront - 0.025);
	if (LampMesh)
	{
		LampComp = AddMesh(LampMesh, nullptr, LampOnDesk, FVector(1.0), FRotator(0.0f, ImportYaw, 0.0f));
	}
	else
	{
		UMaterialInterface* Lamp = Surface(TEXT("Lamp"), 0x1a1a1c, 0.4f, 0.6f);
		CylinderWeb(Lamp, FVector(-0.66, DeskTop, DeskZ - 0.24), 0.065f, 0.02f);
		AddMesh(CylinderMesh, Lamp, Web(-0.66, DeskTop + 0.18, DeskZ - 0.26), FVector(0.014, 0.014, 0.36), FRotator(-8.6f, 0.0f, 0.0f));
		AddMesh(CylinderMesh, Lamp, Web(-0.6, DeskTop + 0.38, DeskZ - 0.2), FVector(0.014, 0.014, 0.3), FRotator(0.0f, 0.0f, 57.0f));
		AddMesh(ConeMesh, Lamp, Web(-0.49, DeskTop + 0.42, DeskZ - 0.2), FVector(0.12, 0.12, 0.1), FRotator(0.0f, 0.0f, 132.0f));
	}

	// Mattress on the floor with a rumpled blanket.
	BoxWeb(Surface(TEXT("Mattress"), 0xc9c2b5, 0.95f, 0.0f, PatternFabric), FVector(RoomLeft + 0.6, 0.1, RoomBack - 1.1), FVector(0.95, 0.2, 1.95));
	BoxWeb(Surface(TEXT("Blanket"), 0x2c3b52, 1.0f, 0.0f, PatternFabric), FVector(RoomLeft + 0.6, 0.22, RoomBack - 0.95), FVector(1.0, 0.05, 1.4), 4.0f);

	// Door on the back wall.
	BoxWeb(Surface(TEXT("Door"), 0x4a3a2c, 0.7f, 0.0f, PatternWood), FVector(1.35, 1.025, RoomBack - 0.03), FVector(0.9, 2.05, 0.05));
	AddMesh(SphereMesh, Surface(TEXT("Knob"), 0xb08d4a, 0.3f, 1.0f), Web(1.0, 1.0, RoomBack - 0.07), FVector(0.06));

	// Bare bulb (off).
	CylinderWeb(Surface(TEXT("Cord"), 0x111111, 0.6f), FVector(0.3, RoomCeiling - 0.4, 1.1), 0.003f, 0.4f);
	AddMesh(SphereMesh, Surface(TEXT("Bulb"), 0xf0f0f0, 0.05f), Web(0.3, RoomCeiling - 0.43, 1.1), FVector(0.07));
}

void ANightOneStage::AddCan()
{
	static const double Spots[10][3] = {
		{0.52, -0.2, 0.3}, {0.61, -0.14, 1.2}, {0.47, -0.08, 2.1}, {0.66, -0.26, 0.7}, {0.56, -0.3, 2.9},
		{0.7, -0.05, 1.8}, {0.42, -0.28, 0.2}, {-0.62, 0.05, 1.1}, {-0.7, -0.08, 2.4}, {0.72, 0.1, 0.5},
	};
	const int32 N = Cans.Num();
	if (N >= 10)
	{
		return;
	}
	const FVector Base(Spots[N][0], DeskTop, DeskZ + Spots[N][1]);
	if (CanMesh)
	{
		// The Blender can, with its own baked materials; each one turned a different way.
		Cans.Add(AddMesh(CanMesh, nullptr, Web(Base.X, Base.Y, Base.Z), FVector(1.0), FRotator(0.0, FMath::RadiansToDegrees(Spots[N][2]), 0.0)));
		return;
	}
	UStaticMeshComponent* Body = CylinderWeb(Surface(TEXT("Can"), 0x0f1a12, 0.28f, 0.75f), Base, 0.0332f, 0.1235f);
	CylinderWeb(Surface(TEXT("CanStripe"), 0xb8ff2e, 0.35f, 0.3f), Base + FVector(0.0, 0.045, 0.0), 0.0334f, 0.03f);
	CylinderWeb(Surface(TEXT("CanLid"), 0xb9bcc2, 0.3f, 1.0f), Base + FVector(0.0, 0.1235, 0.0), 0.0272f, 0.001f);
	Cans.Add(Body);
}

void ANightOneStage::BuildOutside()
{
	// Sky dome.
	if (UMaterialInstanceDynamic* Sky = Special(SkyMaterial, TEXT("Sky")))
	{
		TimedMaterials.Add(Sky);
		AddMesh(SphereMesh, Sky, FVector::ZeroVector, FVector(1800.0), FRotator::ZeroRotator, nullptr, false);
	}
	// Skyline: instanced boxes with a procedural window shader (same generator as the prototype).
	UMaterialInstanceDynamic* City = Special(CityMaterial, TEXT("City"));
	if (City)
	{
		TimedMaterials.Add(City);
	}
	UInstancedStaticMeshComponent* Buildings = NewPart<UInstancedStaticMeshComponent>();
	Buildings->SetStaticMesh(CubeMesh);
	Buildings->SetMaterial(0, City ? static_cast<UMaterialInterface*>(City) : Surface(TEXT("CityFallback"), 0x0a0b0e, 0.9f));
	Buildings->SetCastShadow(false);
	Buildings->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Buildings->bAffectDistanceFieldLighting = false;
	UInstancedStaticMeshComponent* Beacons = NewPart<UInstancedStaticMeshComponent>();
	Beacons->SetStaticMesh(SphereMesh);
	Beacons->SetMaterial(0, Surface(TEXT("Beacon"), 0xff2a1a, 1.0f, 0.0f, PatternNone, 8.0f));
	Beacons->SetCastShadow(false);
	Beacons->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Beacons->bAffectDistanceFieldLighting = false;
	int64 S = 12345;
	auto Rnd = [&S]() {
		S = (S * 16807) % 2147483647;
		return static_cast<double>(S) / 2147483647.0;
	};
	int32 Placed = 0;
	while (Placed < 520)
	{
		const double Z = -40.0 - Rnd() * 520.0;
		const double X = (Rnd() - 0.5) * 900.0;
		if (FMath::Abs(X) < 20.0 && Z > -60.0)
		{
			continue;
		}
		const bool bTall = Rnd() < 0.08;
		const double Bw = 12.0 + Rnd() * 28.0;
		const double Bd = 12.0 + Rnd() * 28.0;
		const double Base = bTall ? 80.0 + Rnd() * 160.0 : 15.0 + Rnd() * 55.0;
		const double Hgt = Base * (1.0 + FMath::Max(0.0, -Z - 150.0) / 500.0);
		Rnd(); // per-building seed in the prototype; Unreal uses PerInstanceRandom
		Buildings->AddInstance(FTransform(FRotator::ZeroRotator, Web(X, -12.0 + Hgt / 2.0, Z), FVector(Bd, Bw, Hgt)));
		if (bTall)
		{
			Beacons->AddInstance(FTransform(FRotator::ZeroRotator, Web(X, -12.0 + Hgt + 1.0, Z), FVector(1.8)));
		}
		++Placed;
	}
	// The building across the street: a brick walk-up over the laundromat, its face 18 m from the window and the
	// street 12 m below (art/blender/assets/tenement.py). Without the mesh, a block of the skyline stands in.
	if (TenementMesh)
	{
		AddMesh(TenementMesh, nullptr, FVector(1800.0, 600.0, -1200.0), FVector(1.0), FRotator::ZeroRotator, nullptr, true);
		// A sodium street lamp across the street, raking the brick from the right: the fire escape (left of the
		// window's middle) throws its stairs and railings across the wall. The city's cold glow fills the shadows
		// so the ironwork reads against them, and the neon's pink lies on the bricks around the sign.
		USpotLightComponent* Lamp = NewPart<USpotLightComponent>();
		const FVector LampAt(1250.0, 250.0, -650.0);
		Lamp->SetRelativeLocationAndRotation(LampAt, (FVector(1800.0, -450.0, 50.0) - LampAt).Rotation());
		Lamp->SetIntensityUnits(ELightUnits::Candelas);
		Lamp->SetIntensity(FacadeLampCandela);
		Lamp->SetLightColor(Srgb(0xffa04a));
		Lamp->SetOuterConeAngle(62.0f);
		Lamp->SetInnerConeAngle(22.0f);
		Lamp->SetSourceRadius(15.0f);
		Lamp->SetAttenuationRadius(2600.0f);
		Lamp->SetCastShadows(true);
		Lamp->SetVolumetricScatteringIntensity(0.0f);
		URectLightComponent* Glow = NewPart<URectLightComponent>();
		Glow->SetRelativeLocationAndRotation(FVector(900.0, 300.0, -200.0), FRotator::ZeroRotator);
		Glow->SetIntensityUnits(ELightUnits::Candelas);
		Glow->SetIntensity(FacadeGlowCandela);
		Glow->SetLightColor(FLinearColor(0.38f, 0.46f, 0.75f));
		Glow->SetSourceWidth(3000.0f);
		Glow->SetSourceHeight(2400.0f);
		Glow->SetAttenuationRadius(2600.0f);
		Glow->SetCastShadows(false);
		Glow->SetVolumetricScatteringIntensity(0.0f);
		UPointLightComponent* Spill = NewPart<UPointLightComponent>();
		Spill->SetRelativeLocation(FVector(1745.0, 320.0, -60.0));
		Spill->SetIntensityUnits(ELightUnits::Candelas);
		Spill->SetIntensity(NeonSpillCandela);
		Spill->SetLightColor(Srgb(0xff2e88));
		Spill->SetSourceRadius(120.0f);
		Spill->SetAttenuationRadius(520.0f);
		Spill->SetCastShadows(false);
		Spill->SetVolumetricScatteringIntensity(0.0f);
	}
	else
	{
		Buildings->AddInstance(FTransform(FRotator::ZeroRotator, Web(6.0, -16.0 + 20.0, -22.0), FVector(8.0, 60.0, 40.0)));
	}

	// The laundromat's neon sign and its glow on the wet facade.
	NeonWidget = AddWidget(Web(3.2, -0.6, -17.9), Facing(FVector::BackwardVector, FVector::UpVector), FVector2D(640.0, 200.0), FIntPoint(1024, 320), false, true);
	NeonWidget->SetTintColorAndOpacity(FLinearColor(8.0f, 8.0f, 8.0f, 1.0f));
	if (UMaterialInstanceDynamic* Halo = Special(GlowMaterial, TEXT("NeonHalo")))
	{
		Halo->SetVectorParameterValue(TEXT("Color"), Srgb(0xff2e88));
		Halo->SetScalarParameterValue(TEXT("Strength"), 0.6f);
		AddMesh(PlaneMesh, Halo, Web(3.2, -0.6, -17.95), FVector(7.0, 14.0, 1.0), FRotator(90.0f, 0.0f, 0.0f), nullptr, false);
	}
	// Street lights far below.
	UMaterialInterface* Sodium = Surface(TEXT("Sodium"), 0xffa050, 1.0f, 0.0f, PatternNone, 12.0f);
	for (int32 I = 0; I < 6; ++I)
	{
		AddMesh(SphereMesh, Sodium, Web(-18.0 + I * 9.0, -6.5, -12.0), FVector(0.5), FRotator::ZeroRotator, nullptr, false);
	}
	// Falling rain between the window and the building across the street.
	if (UMaterialInstanceDynamic* Rain = Special(RainMaterial, TEXT("Rain")))
	{
		TimedMaterials.Add(Rain);
		const double Depths[3] = {250.0, 700.0, 1300.0};
		for (int32 I = 0; I < 3; ++I)
		{
			AddMesh(PlaneMesh, Rain, FVector(Depths[I], 0.0, 100.0), FVector(14.0, 16.0 + I * 4.0, 1.0), FRotator(90.0f, 0.0f, 0.0f), nullptr, false);
		}
	}
}

void ANightOneStage::BuildLights()
{
	// The laptop screen is the key light, just in front of the display.
	ScreenLight = NewPart<URectLightComponent>(LidPivot);
	ScreenLight->SetRelativeLocationAndRotation(Screen ? Screen->GetRelativeLocation() - FVector(0.55, 0.0, 0.0) : Web(0.0, 0.117, 0.006), FRotator(0.0f, 180.0f, 0.0f));
	ScreenLight->SetIntensityUnits(ELightUnits::Candelas);
	ScreenLight->SetIntensity(ScreenLightCandela);
	ScreenLight->SetLightColor(ScreenGlow);
	ScreenLight->SetSourceWidth(31.6f);
	ScreenLight->SetSourceHeight(19.75f);
	ScreenLight->SetAttenuationRadius(400.0f);
	ScreenLight->SetCastShadows(true);
	ScreenLight->SetSpecularScale(ScreenLightSpecular);

	// Neon through the window: shadows of the frame and moving rain on the room.
	NeonSpot = NewPart<USpotLightComponent>();
	const FVector NeonAt = Web(1.3, 1.25, -5.6);
	NeonSpot->SetRelativeLocationAndRotation(NeonAt, (Web(-0.35, 1.0, 2.0) - NeonAt).Rotation());
	NeonSpot->SetIntensityUnits(ELightUnits::Candelas);
	NeonSpot->SetIntensity(NeonCandela);
	NeonSpot->SetLightColor(Srgb(0xff3a8c));
	NeonSpot->SetOuterConeAngle(31.5f);
	NeonSpot->SetInnerConeAngle(11.0f);
	NeonSpot->SetAttenuationRadius(1600.0f);
	NeonSpot->SetSourceRadius(8.0f);
	NeonSpot->SetCastShadows(true);
	if (RainCookieMaterial)
	{
		NeonSpot->SetLightFunctionMaterial(RainCookieMaterial);
		NeonSpot->SetLightFunctionScale(FVector(120.0, 120.0, 120.0));
	}

	// Cool city fill from the window and the blue sign.
	CityFill = NewPart<URectLightComponent>();
	CityFill->SetRelativeLocationAndRotation(Web(0.0, 1.55, RoomFront - 0.05), FVector(-2.95, 0.0, -0.3).Rotation());
	CityFill->SetIntensityUnits(ELightUnits::Candelas);
	CityFill->SetIntensity(1.7f);
	CityFill->SetLightColor(Srgb(0x5a78c8));
	CityFill->SetSourceWidth(144.0f);
	CityFill->SetSourceHeight(114.0f);
	CityFill->SetAttenuationRadius(600.0f);
	CityFill->SetCastShadows(false);
	BlueNeon = NewPart<UPointLightComponent>();
	BlueNeon->SetRelativeLocation(Web(0.5, 1.6, RoomFront - 0.4));
	BlueNeon->SetIntensityUnits(ELightUnits::Candelas);
	BlueNeon->SetIntensity(0.25f);
	BlueNeon->SetLightColor(Srgb(0x35d3ff));
	BlueNeon->SetAttenuationRadius(500.0f);
	BlueNeon->SetCastShadows(false);
	RoomFill = NewPart<UPointLightComponent>();
	RoomFill->SetRelativeLocation(Web(0.6, 2.3, 1.2));
	RoomFill->SetIntensityUnits(ELightUnits::Candelas);
	RoomFill->SetIntensity(RoomFillCandela);
	RoomFill->SetLightColor(Srgb(0x223047));
	RoomFill->SetAttenuationRadius(600.0f);
	RoomFill->SetCastShadows(false);

	// Lens: manual exposure, bloom for the screen and neon, grain, vignette, and the tilt treatment.
	Lens = NewPart<UPostProcessComponent>();
	Lens->bUnbound = true;
	FPostProcessSettings& P = Lens->Settings;
	P.bOverride_AutoExposureMethod = true;
	P.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	P.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	P.AutoExposureApplyPhysicalCameraExposure = 0;
	P.bOverride_AutoExposureBias = true;
	P.AutoExposureBias = ExposureBias + BrightnessBias;
	P.bOverride_BloomIntensity = true;
	P.BloomIntensity = 0.8f;
	P.bOverride_BloomThreshold = true;
	P.BloomThreshold = 0.9f;
	P.bOverride_VignetteIntensity = true;
	P.VignetteIntensity = 0.5f;
	P.bOverride_FilmGrainIntensity = true;
	P.FilmGrainIntensity = 0.1f;
	P.bOverride_SceneFringeIntensity = true;
	P.SceneFringeIntensity = 0.4f;
	P.bOverride_MotionBlurAmount = true;
	P.MotionBlurAmount = 0.0f;
	P.bOverride_ColorSaturation = true;
	P.ColorSaturation = FVector4(1.0, 1.0, 1.0, 1.0);
	P.bOverride_ColorGain = true;
	P.ColorGain = FVector4(1.0, 1.0, 1.0, 1.0);
}

// ------------------------------------------------------------------ play

void ANightOneStage::BeginPlay()
{
	Super::BeginPlay();
	AttachSlate();
}

void ANightOneStage::AttachSlate()
{
	if (!FSlateTextMeasurer::IsAvailable())
	{
		return;
	}
	ScreenSlate = SNew(SDrawListWidget).DesiredSize(FVector2D(ScreenResolution.X, ScreenResolution.Y));
	if (Screen)
	{
		Screen->SetSlateWidget(ScreenSlate);
	}
	PhoneSlate = SNew(SDrawListWidget).DesiredSize(FVector2D(360.0, 760.0));
	if (PhoneScreen)
	{
		PhoneScreen->SetSlateWidget(PhoneSlate);
	}

	// Printed props and the neon sign are drawn once.
	FSlateTextMeasurer Measurer;
	auto Draw = [&Measurer](float Wd, float Ht, TFunctionRef<void(ss::ui::Canvas&)> Paint) {
		TSharedPtr<ss::ui::DrawList> L = MakeShared<ss::ui::DrawList>();
		ss::ui::Canvas C(*L, Measurer, Wd, Ht, 1.0f);
		Paint(C);
		return TSharedPtr<const ss::ui::DrawList>(L);
	};
	namespace P = ss::ui::props;
	PropLists.Reset();
	PropLists.Add(Draw(P::NoticeW, P::NoticeH, [](ss::ui::Canvas& C) { P::EvictionNotice(C); }));
	PropLists.Add(Draw(P::BillW, P::BillH, [](ss::ui::Canvas& C) { P::PowerBill(C); }));
	PropLists.Add(Draw(P::NoticeW, P::NoticeH, [](ss::ui::Canvas& C) { P::EvictionNotice(C); }));
	PropLists.Add(Draw(P::PosterW, P::PosterH, [](ss::ui::Canvas& C) { P::Poster(C); }));
	PropLists.Add(Draw(P::NoteSize, P::NoteSize, [](ss::ui::Canvas& C) { P::StickyNote(C, {"BR: $2.37", "DON'T", "TILT."}, ss::ui::Hex(0xf6e27a)); }));
	PropLists.Add(Draw(P::NoteSize, P::NoteSize, [](ss::ui::Canvas& C) { P::StickyNote(C, {"fold the", "trash.", "shove", "the good"}, ss::ui::Hex(0x9fe3b6)); }));
	PropLists.Add(Draw(P::NoteSize, P::NoteSize, [](ss::ui::Canvas& C) { P::StickyNote(C, {"RENT", "FRIDAY", "$1,225"}, ss::ui::Hex(0xff9cb8)); }));
	PropSlates.Reset();
	for (int32 I = 0; I < PropWidgets.Num() && I < PropLists.Num(); ++I)
	{
		TSharedPtr<SDrawListWidget> Sw = SNew(SDrawListWidget).DesiredSize(FVector2D(PropLists[I]->Width, PropLists[I]->Height));
		Sw->SetDrawList(PropLists[I]);
		PropSlates.Add(Sw);
		if (PropWidgets[I])
		{
			PropWidgets[I]->SetSlateWidget(Sw);
			PropWidgets[I]->SetManuallyRedraw(true);
			PropWidgets[I]->RequestRedraw();
		}
	}
	auto Single = [&](UWidgetComponent* Target, TSharedPtr<const ss::ui::DrawList> List) {
		if (!Target)
		{
			return;
		}
		TSharedPtr<SDrawListWidget> Sw = SNew(SDrawListWidget).DesiredSize(FVector2D(List->Width, List->Height));
		Sw->SetDrawList(List);
		PropSlates.Add(Sw);
		PropLists.Add(List);
		Target->SetSlateWidget(Sw);
		Target->SetManuallyRedraw(true);
		Target->RequestRedraw();
	};
	Single(NeonWidget, Draw(P::NeonW, P::NeonH, [](ss::ui::Canvas& C) { P::NeonSign(C); }));
	Single(Keyboard, Draw(P::KeyboardW, P::KeyboardH, [](ss::ui::Canvas& C) { P::Keyboard(C); }));

	// The GearDrop monitors, drawn by the game (SetMonitorDrawList).
	MonitorSlates.Reset();
	for (UWidgetComponent* Panel : MonitorScreens)
	{
		TSharedPtr<SDrawListWidget> Sw = SNew(SDrawListWidget).DesiredSize(FVector2D(MonitorResolution.X, MonitorResolution.Y));
		MonitorSlates.Add(Sw);
		if (Panel)
		{
			Panel->SetSlateWidget(Sw);
		}
	}
}

void ANightOneStage::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Dt = FMath::Max(0.0f, DeltaSeconds);
	Time += Dt;
	NextLightning -= Dt;
	if (NextLightning <= 0.0f)
	{
		NextLightning = 40.0f + FMath::FRand() * 80.0f;
		Lightning(0.5f + FMath::FRand() * 0.5f);
	}
	Flash = FMath::Max(0.0f, Flash - Dt * 2.5f);
	const float Flicker = Flash > 0.05f ? Flash * (0.5f + 0.5f * FMath::Sin(Time * 55.0f)) : 0.0f;
	// The sign buzzes and occasionally drops out, like cheap neon does.
	NeonGlitch -= Dt;
	if (NeonGlitch < -4.0f - FMath::FRand() * 10.0f)
	{
		NeonGlitch = 0.25f + FMath::FRand() * 0.4f;
	}
	NeonLevel = NeonGlitch > 0.0f ? (FMath::FRand() < 0.5f ? 0.15f : 1.0f) : 0.92f + 0.08f * FMath::Sin(Time * 120.0f);
	if (NeonSpot)
	{
		NeonSpot->SetIntensity(NeonCandela * NeonLevel + Flicker * 250.0f);
		NeonSpot->SetLightColor(FMath::Lerp(Srgb(0xff3a8c), FLinearColor(0.85f, 0.9f, 1.0f), FMath::Min(1.0f, Flicker * 1.5f)));
	}
	if (NeonWidget)
	{
		NeonWidget->SetTintColorAndOpacity(FLinearColor(8.0f * NeonLevel, 8.0f * NeonLevel, 8.0f * NeonLevel, 1.0f));
	}
	if (RoomFill)
	{
		RoomFill->SetIntensity(RoomFillCandela + Flicker * 3.0f);
	}
	for (UMaterialInstanceDynamic* M : TimedMaterials)
	{
		if (M)
		{
			M->SetScalarParameterValue(TEXT("Flash"), Flicker);
			M->SetScalarParameterValue(TEXT("Dawn"), Dawn);
		}
	}
}

void ANightOneStage::Lightning(float Strength)
{
	Flash = FMath::Max(Flash, Strength);
	if (OnThunder)
	{
		OnThunder(0.6f + FMath::FRand() * 2.2f, Strength);
	}
}

// ------------------------------------------------------------------ game hooks

FVector ANightOneStage::EyeLocation() const
{
	return GetActorTransform().TransformPosition(Web(0.0, 1.17, DeskZ + 0.72));
}

FVector ANightOneStage::ScreenCenter() const
{
	return Screen ? Screen->GetComponentLocation() : GetActorLocation();
}

FVector ANightOneStage::ScreenNormal() const
{
	return Screen ? Screen->GetForwardVector() : FVector::BackwardVector;
}

void ANightOneStage::MenuShot(double T, FVector& OutLocation, FVector& OutLookAt) const
{
	// Over the chair's shoulder from behind and to the left, drifting slowly: the desk and the
	// laptop in the middle of the frame, the rain-streaked window and the neon beyond it, and the
	// darker side of the room on the left where the menu sits.
	const FVector Eye = Web(-0.62 + 0.07 * FMath::Sin(T * 0.045), 1.42 + 0.012 * FMath::Sin(T * 0.21), 0.92 - 0.06 * FMath::Sin(T * 0.037));
	const FVector At = Web(0.22 + 0.05 * FMath::Sin(T * 0.03), 1.02 + 0.02 * FMath::Sin(T * 0.05), -0.8);
	OutLocation = GetActorTransform().TransformPosition(Eye);
	OutLookAt = GetActorTransform().TransformPosition(At);
}

bool ANightOneStage::ScreenHit(const FVector& Origin, const FVector& Direction, FVector2D& OutClient) const
{
	if (!Screen)
	{
		return false;
	}
	const FVector N = Screen->GetForwardVector();
	const double Denom = FVector::DotProduct(Direction, N);
	if (Denom > -1e-4)
	{
		return false; // parallel, or looking at the back of the lid
	}
	const double T = FVector::DotProduct(Screen->GetComponentLocation() - Origin, N) / Denom;
	if (T < 0.0)
	{
		return false;
	}
	// Widget space: x = -local Y, y = -local Z, offset by the pivot (the component scale is one unit per pixel).
	const FVector Local = Screen->GetComponentTransform().InverseTransformPosition(Origin + Direction * T);
	const double U = (-Local.Y + ScreenResolution.X * 0.5) / ScreenResolution.X;
	const double V = (-Local.Z + ScreenResolution.Y * 0.5) / ScreenResolution.Y;
	OutClient = FVector2D(U * 1600.0, V * 1000.0);
	return U >= 0.0 && U <= 1.0 && V >= 0.0 && V <= 1.0;
}

void ANightOneStage::SetScreenDrawList(const TSharedPtr<const ss::ui::DrawList>& List)
{
	if (ScreenSlate)
	{
		ScreenSlate->SetDrawList(List);
	}
}

void ANightOneStage::SetPhoneDrawList(const TSharedPtr<const ss::ui::DrawList>& List)
{
	if (PhoneSlate)
	{
		PhoneSlate->SetDrawList(List);
	}
}

void ANightOneStage::SetPhoneBrightness(float Level)
{
	if (PhoneScreen)
	{
		PhoneScreen->SetTintColorAndOpacity(FLinearColor(Level, Level, Level, 1.0f));
	}
	if (PhoneLight)
	{
		PhoneLight->SetIntensity(Level * 0.35f);
	}
}

void ANightOneStage::BuildLeds()
{
	LedWashes.Reset();
	LedStrips.Reset();
	bLedsShown = false;
	LedMaterial = UMaterialInstanceDynamic::Create(SurfaceMaterial ? SurfaceMaterial.Get() : FallbackMaterial.Get(), this);
	LedMaterial->SetScalarParameterValue(TEXT("Roughness"), 0.4f);
	LedMaterial->SetScalarParameterValue(TEXT("Pattern"), PatternNone);
	auto Strip = [this](double Length, const FVector& At, float Yaw, const FVector& WashTo, double WashHeight) {
		UStaticMeshComponent* Bar = BoxWeb(LedMaterial, At, FVector(Length, 0.012, 0.014), Yaw);
		Bar->SetCastShadow(false);
		Bar->SetVisibility(false);
		LedStrips.Add(Bar);
		URectLightComponent* Wash = NewPart<URectLightComponent>();
		const FVector From = Web(At.X, At.Y, At.Z);
		Wash->SetRelativeLocationAndRotation(From, (Web(WashTo.X, WashTo.Y, WashTo.Z) - From).Rotation());
		Wash->SetIntensityUnits(ELightUnits::Candelas);
		Wash->SetSourceWidth(static_cast<float>(Length * 100.0));
		Wash->SetSourceHeight(static_cast<float>(WashHeight * 100.0));
		Wash->SetBarnDoorAngle(70.0f);
		Wash->SetAttenuationRadius(450.0f);
		Wash->SetCastShadows(false);
		Wash->SetVisibility(false);
		LedWashes.Add(Wash);
	};
	// Behind the desk, on the wall under the sill: it washes up the wall and into the window recess.
	Strip(1.5, FVector(0.0, DeskTop + 0.02, RoomFront + 0.02), 0.0f, FVector(0.0, DeskTop + 0.6, RoomFront - 0.3), 0.05);
	// The ceiling cove: the window wall, then the left and right walls.
	const double Cy = RoomCeiling - 0.03;
	const double Cx = (RoomLeft + RoomRight) / 2.0;
	const double Cz = (RoomFront + RoomBack) / 2.0;
	Strip(RoomRight - RoomLeft - 0.1, FVector(Cx, Cy, RoomFront + 0.03), 0.0f, FVector(Cx, Cy - 0.9, RoomFront + 0.35), 0.06);
	Strip(RoomBack - RoomFront - 0.1, FVector(RoomLeft + 0.03, Cy, Cz), 90.0f, FVector(RoomLeft + 0.35, Cy - 0.9, Cz), 0.06);
	Strip(RoomBack - RoomFront - 0.1, FVector(RoomRight - 0.03, Cy, Cz), 90.0f, FVector(RoomRight - 0.35, Cy - 0.9, Cz), 0.06);
	LedFill = NewPart<UPointLightComponent>();
	LedFill->SetRelativeLocation(Web(0.2, 2.1, 0.4));
	LedFill->SetIntensityUnits(ELightUnits::Candelas);
	LedFill->SetAttenuationRadius(700.0f);
	LedFill->SetCastShadows(false);
	LedFill->SetVisibility(false);
}

void ANightOneStage::SetRoomLights(bool bOn, const FLinearColor& Color, float Level)
{
	if (bOn != bLedsShown)
	{
		bLedsShown = bOn;
		for (UStaticMeshComponent* Bar : LedStrips)
		{
			if (Bar)
			{
				Bar->SetVisibility(bOn);
			}
		}
		for (URectLightComponent* Wash : LedWashes)
		{
			if (Wash)
			{
				Wash->SetVisibility(bOn);
			}
		}
		if (LedFill)
		{
			LedFill->SetVisibility(bOn);
		}
	}
	if (!bOn)
	{
		return;
	}
	const float L = FMath::Clamp(Level, 0.0f, 2.0f);
	if (LedMaterial)
	{
		LedMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
		LedMaterial->SetVectorParameterValue(TEXT("Color"), Color);
		LedMaterial->SetScalarParameterValue(TEXT("Emissive"), LedEmissive * L);
	}
	for (int32 I = 0; I < LedWashes.Num(); ++I)
	{
		if (URectLightComponent* Wash = LedWashes[I])
		{
			Wash->SetLightColor(Color);
			Wash->SetIntensity((I == 0 ? LedDeskCandela : LedCoveCandela) * L);
		}
	}
	if (LedFill)
	{
		LedFill->SetLightColor(Color);
		LedFill->SetIntensity(LedFillCandela * L);
	}
}

// ------------------------------------------------------------------ GearDrop's gear and the mementos

namespace NightOneStageDetail
{
enum EGearPiece : int32
{
	GearMonitor,
	GearMonitorWide,
	GearTower,
	GearTower2,
	GearWebcam,
	GearWebcamPro,
	GearMirrorless,
	GearMicUsb,
	GearMicArm,
	GearRingLight,
	GearKeyLights,
	GearMacroPad,
	GearHeadphones,
	GearPlant,
	GearCurtains,
	GearRouter,
	GearTrophy,
	GearDeeChip,
	GearPieces
};

/** An offset or direction in a Blender prop's own frame (x right, y away from the chair, z up) in the stage's axes. */
FVector BlenderAxes(const FVector& B)
{
	return FVector(B.Y, B.X, B.Z);
}

/**
 * The monitors (art/blender/assets/monitor.py SCREENS): where the arm's pole stands (web x), the display's center from
 * the pole (Blender meters), its turn toward the chair and its tilt back (degrees), and its size (cm).
 */
struct FMonitorSpec
{
	double PoleX;
	FVector Center;
	double Yaw;
	double Pitch;
	FVector2D SizeCm;
};
const FMonitorSpec MonitorSpecs[2] = {
	{0.56, FVector(-0.060, -0.235, 0.295), -31.9, -7.5, FVector2D(53.13, 29.89)},
	{-0.48, FVector(-0.040, -0.245, 0.315), 33.2, -6.3, FVector2D(59.68, 33.57)},
};
/** The monitor arms' poles stand 3 cm in front of the desk's back edge. */
const double PoleZ = DeskZ - 0.35 + 0.03;
/** Where the player's face is, for the lights aimed at it. */
const FVector FaceWeb(0.0, 1.12, 0.12);
const FLinearColor StudioWhite(1.0f, 0.9f, 0.78f);
const uint32 StudioOff = 0xd9d6cf;
} // namespace NightOneStageDetail

void ANightOneStage::BuildGear()
{
	GearGroups.Reset();
	MonitorScreens.Reset();
	MonitorLights.Reset();
	RgbLights.Reset();
	StudioLights.Reset();
	bGearApplied = false;
	StudioState = -1;
	auto Emitter = [this](uint32 Hex) {
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(SurfaceMaterial ? SurfaceMaterial.Get() : FallbackMaterial.Get(), this);
		M->SetVectorParameterValue(TEXT("BaseColor"), Srgb(Hex));
		M->SetVectorParameterValue(TEXT("Color"), Srgb(Hex));
		M->SetScalarParameterValue(TEXT("Roughness"), 0.45f);
		M->SetScalarParameterValue(TEXT("Pattern"), PatternNone);
		return M;
	};
	RgbMaterial = Emitter(0x7c5cff);
	StudioMaterial = Emitter(StudioOff);
	for (int32 I = 0; I < GearPieces; ++I)
	{
		// The webcams clip onto the laptop's lid and tilt with it.
		GearGroups.Add(NewPart<USceneComponent>(I == GearWebcam || I == GearWebcamPro ? LidPivot.Get() : nullptr));
	}
	// A Blender prop set out like the others (its front to the chair), turned Yaw degrees further: positive turns it to
	// face more to the left, as the player sees the room.
	auto Place = [this](int32 Piece, const TCHAR* Name, const FVector& WebAt, float Yaw, UMaterialInterface* Material = nullptr) -> UStaticMeshComponent* {
		UStaticMesh* Mesh = LoadProp(Name);
		if (!Mesh || !GearGroups[Piece])
		{
			return nullptr;
		}
		return AddMesh(Mesh, Material, Web(WebAt.X, WebAt.Y, WebAt.Z), FVector(1.0), FRotator(0.0f, ImportYaw + Yaw, 0.0f), GearGroups[Piece], Material == nullptr);
	};
	auto Light = [this](int32 Piece, const FVector& At, const FVector& Aim, float Width, float Height, float BarnDoor, float Radius) {
		URectLightComponent* L = NewPart<URectLightComponent>(GearGroups[Piece]);
		L->SetRelativeLocationAndRotation(At, Aim.Rotation());
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetSourceWidth(Width);
		L->SetSourceHeight(Height);
		L->SetBarnDoorAngle(BarnDoor);
		L->SetAttenuationRadius(Radius);
		L->SetCastShadows(false);
		return L;
	};

	// Monitors on arms either side of the laptop, turned to the chair; their pictures come from the game, and they light
	// the desk a little.
	for (int32 M = 0; M < 2; ++M)
	{
		const FMonitorSpec& Spec = MonitorSpecs[M];
		const int32 Piece = M == 0 ? GearMonitor : GearMonitorWide;
		const FVector Pole(Spec.PoleX, DeskTop, PoleZ);
		Place(Piece, M == 0 ? TEXT("SM_Monitor") : TEXT("SM_MonitorWide"), Pole, 0.0f);
		const double Yaw = FMath::DegreesToRadians(Spec.Yaw);
		const double Pitch = FMath::DegreesToRadians(Spec.Pitch);
		const FVector Normal = BlenderAxes(FVector(FMath::Cos(Pitch) * FMath::Sin(Yaw), -FMath::Cos(Pitch) * FMath::Cos(Yaw), -FMath::Sin(Pitch)));
		const FVector Up = BlenderAxes(FVector(FMath::Sin(Pitch) * FMath::Sin(Yaw), -FMath::Sin(Pitch) * FMath::Cos(Yaw), FMath::Cos(Pitch)));
		const FVector Center = Web(Pole.X, Pole.Y, Pole.Z) + BlenderAxes(Spec.Center) * 100.0;
		UWidgetComponent* Panel = AddWidget(Center + Normal * 0.1, Facing(Normal, Up), Spec.SizeCm, MonitorResolution, false, false, GearGroups[Piece]);
		Panel->SetTintColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f));
		Panel->SetRedrawTime(1.0f / 15.0f);
		MonitorScreens.Add(Panel);
		URectLightComponent* Glow = Light(Piece, Center + Normal * 2.0, Normal, Spec.SizeCm.X, Spec.SizeCm.Y, 80.0f, 320.0f);
		Glow->SetIntensity(MonitorCandela);
		Glow->SetLightColor(FLinearColor(0.7f, 0.82f, 1.0f));
		Glow->SetSpecularScale(0.2f);
		MonitorLights.Add(Glow);
	}

	// The PC on the floor by the desk's front corner, where the player sees it beside them, its open side turned to the
	// chair (behind the desk it would hide under the top); the second PC of the two-PC setup beside it. Their RGB takes
	// the LED kit's colour (SetGearGlow) and spills across the floor and the chair.
	for (int32 T = 0; T < 2; ++T)
	{
		const int32 Piece = T == 0 ? GearTower : GearTower2;
		const FVector At(1.0 + 0.3 * T, 0.0, -0.15 - 0.04 * T);
		const float Yaw = -16.0f + 6.0f * T;
		Place(Piece, TEXT("SM_Tower"), At, Yaw);
		if (UStaticMeshComponent* Lit = Place(Piece, TEXT("SM_Tower_Glow"), At, Yaw, RgbMaterial))
		{
			Lit->SetCastShadow(false);
		}
		const FRotator Turn(0.0f, Yaw, 0.0f);
		const FVector Out = Turn.RotateVector(BlenderAxes(FVector(-1.0, 0.0, 0.0)));
		RgbLights.Add(Light(Piece, Web(At.X, At.Y, At.Z) + Turn.RotateVector(BlenderAxes(FVector(-0.1, 0.0, 0.25))) * 100.0, Out, 40.0f, 42.0f, 85.0f, 260.0f));
	}

	// Cameras: the webcams on the lid, the mirrorless on its tripod behind the laptop.
	Place(GearWebcam, TEXT("SM_Webcam"), FVector::ZeroVector, 0.0f);
	Place(GearWebcamPro, TEXT("SM_WebcamPro"), FVector::ZeroVector, 0.0f);
	Place(GearMirrorless, TEXT("SM_Mirrorless"), FVector(0.12, DeskTop, -0.815), 7.0f);
	// Mics: the USB mic beside the laptop, the broadcast mic's arm clamped to the desk's left edge.
	Place(GearMicUsb, TEXT("SM_MicUsb"), FVector(-0.235, DeskTop, -0.58), -18.0f);
	Place(GearMicArm, TEXT("SM_MicArm"), FVector(-0.775, DeskTop, -0.46), 0.0f);

	// Streaming lights, lit while live (SetGearGlow): the ring light behind the laptop, or key lights on the desk's edges.
	{
		const FVector At(0.0, DeskTop, -0.83);
		Place(GearRingLight, TEXT("SM_RingLight"), At, 0.0f);
		Place(GearRingLight, TEXT("SM_RingLight_Glow"), At, 0.0f, StudioMaterial);
		const FVector Ring = Web(At.X, At.Y + 0.5, At.Z + 0.03);
		StudioLights.Add(Light(GearRingLight, Ring, (Web(FaceWeb.X, FaceWeb.Y, FaceWeb.Z) - Ring).GetSafeNormal(), 28.0f, 28.0f, 55.0f, 360.0f));
	}
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		const FVector At(0.77 * Side, DeskTop, -0.70);
		Place(GearKeyLights, Side < 0 ? TEXT("SM_KeyLight_L") : TEXT("SM_KeyLight_R"), At, 0.0f);
		Place(GearKeyLights, Side < 0 ? TEXT("SM_KeyLight_L_Glow") : TEXT("SM_KeyLight_R_Glow"), At, 0.0f, StudioMaterial);
		// The panel sits on the pole's head (lights.py: KEY_Z), 5 cm out toward the face and 3 cm up.
		const FVector Head = Web(At.X, DeskTop + 0.714, At.Z);
		const FVector Aim = (Web(FaceWeb.X, FaceWeb.Y, FaceWeb.Z) - Head).GetSafeNormal();
		StudioLights.Add(Light(GearKeyLights, Head + Aim * 7.5 + FVector(0.0, 0.0, 3.0), Aim, 33.0f, 23.0f, 45.0f, 380.0f));
	}
	for (ULocalLightComponent* L : StudioLights)
	{
		L->SetLightColor(StudioWhite);
		L->SetSpecularScale(0.3f);
	}

	// The rest of the desk, the window and the sill.
	Place(GearMacroPad, TEXT("SM_MacroPad"), FVector(-0.27, DeskTop, -0.40), -12.0f);
	Place(GearHeadphones, TEXT("SM_Headphones"), FVector(0.66, DeskTop, -0.36), 25.0f);
	Place(GearPlant, TEXT("SM_Plant"), FVector(-0.13, WinY0, RoomFront - 0.02), 0.0f);
	Place(GearCurtains, TEXT("SM_Curtains"), FVector(0.0, 0.0, RoomFront), 0.0f);
	Place(GearRouter, TEXT("SM_Router"), FVector(0.56, WinY0, RoomFront - 0.035), 0.0f);
	// Mementos on the sill: the Riverside's cup, and the chip from Dee's back room standing in its stand.
	Place(GearTrophy, TEXT("SM_Trophy"), FVector(0.26, WinY0, RoomFront - 0.025), 13.0f);
	{
		const FVector At(0.05, WinY0, RoomFront + 0.015);
		const float Yaw = 4.0f;
		Place(GearDeeChip, TEXT("SM_ChipStand"), At, Yaw);
		if (UStaticMesh* Chip = LoadProp(TEXT("SM_Chip_100")))
		{
			// Upright in the slot and leaning back (trophy.py: SLOT, SLOT_TILT), its face to the room.
			const FRotator Turn(0.0f, Yaw, 0.0f);
			const FVector Seat = Web(At.X, At.Y, At.Z) + Turn.RotateVector(BlenderAxes(FVector(0.0, 0.0096, 0.0328))) * 100.0;
			const FRotator Upright = (FQuat(FRotator(78.0f, Yaw, 0.0f)) * FQuat(FRotator(0.0f, ImportYaw, 0.0f))).Rotator();
			AddMesh(Chip, nullptr, Seat, FVector(1.0), Upright, GearGroups[GearDeeChip]);
		}
	}
	for (USceneComponent* Group : GearGroups)
	{
		Group->SetVisibility(false, true);
	}
}

void ANightOneStage::SetGear(const FRoomGear& NewGear)
{
	if (bGearApplied && NewGear == Gear)
	{
		return;
	}
	Gear = NewGear;
	bGearApplied = true;
	const bool Shown[GearPieces] = {Gear.bMonitor, Gear.bMonitorWide, Gear.Towers >= 1, Gear.Towers >= 2, Gear.Cam == 1, Gear.Cam == 2, Gear.Cam >= 3,
	                                Gear.Mic == 1, Gear.Mic >= 2, Gear.Lights == 1, Gear.Lights >= 2, Gear.bMacroPad, Gear.bHeadphones, Gear.bPlant,
	                                Gear.bCurtains, Gear.bRouter, Gear.bTrophy, Gear.bDeeChip};
	for (int32 I = 0; I < GearGroups.Num() && I < GearPieces; ++I)
	{
		if (GearGroups[I])
		{
			GearGroups[I]->SetVisibility(Shown[I], true);
		}
	}
	if (LampComp)
	{
		LampComp->SetRelativeLocation(Gear.bMonitorWide ? LampOnSill : LampOnDesk);
	}
	StudioState = -1; // showing a group shows its lights: SetGearGlow sets them again
}

void ANightOneStage::SetGearGlow(const FLinearColor& Rgb, float Level, bool bLive)
{
	const float L = FMath::Clamp(Level, 0.0f, 2.0f);
	if (Gear.Towers > 0)
	{
		if (RgbMaterial)
		{
			RgbMaterial->SetVectorParameterValue(TEXT("BaseColor"), Rgb);
			RgbMaterial->SetVectorParameterValue(TEXT("Color"), Rgb);
			RgbMaterial->SetScalarParameterValue(TEXT("Emissive"), RgbEmissive * L);
		}
		for (URectLightComponent* Spill : RgbLights)
		{
			if (Spill)
			{
				Spill->SetLightColor(Rgb);
				Spill->SetIntensity(RgbCandela * L);
			}
		}
	}
	const int32 State = bLive && Gear.Lights > 0 ? 1 : 0;
	if (State == StudioState)
	{
		return;
	}
	StudioState = State;
	if (StudioMaterial)
	{
		const FLinearColor Diffuser = State ? StudioWhite : Srgb(StudioOff);
		StudioMaterial->SetVectorParameterValue(TEXT("BaseColor"), Diffuser);
		StudioMaterial->SetVectorParameterValue(TEXT("Color"), Diffuser);
		StudioMaterial->SetScalarParameterValue(TEXT("Emissive"), State ? StudioEmissive : 0.0f);
	}
	for (int32 I = 0; I < StudioLights.Num(); ++I)
	{
		if (ULocalLightComponent* Studio = StudioLights[I])
		{
			const bool bThis = I == 0 ? Gear.Lights == 1 : Gear.Lights >= 2;
			Studio->SetIntensity(I == 0 ? RingCandela : KeyCandela);
			Studio->SetVisibility(State == 1 && bThis);
		}
	}
}

void ANightOneStage::SetMonitorDrawList(int32 Index, const TSharedPtr<const ss::ui::DrawList>& List)
{
	if (MonitorSlates.IsValidIndex(Index) && MonitorSlates[Index])
	{
		MonitorSlates[Index]->SetDrawList(List);
	}
}

void ANightOneStage::SetScreenGlow(const FLinearColor& Color, float Brightness)
{
	ScreenGlow = Color;
	ScreenBrightness = Brightness;
	if (ScreenLight)
	{
		ScreenLight->SetLightColor(Color);
		ScreenLight->SetIntensity(ScreenLightCandela * Brightness);
	}
}

void ANightOneStage::SetDawn(float Value)
{
	Dawn = FMath::Clamp(Value, 0.0f, 1.0f);
}

void ANightOneStage::SetLens(float Focus, float Tilt, float Pulse)
{
	if (!Lens)
	{
		return;
	}
	FPostProcessSettings& P = Lens->Settings;
	// Leaned in, keep the screen clean: no fringing, lighter grain.
	P.SceneFringeIntensity = (0.4f + Tilt * 2.0f + Pulse * 2.0f) * (1.0f - 0.9f * Focus) * FringeScale;
	P.FilmGrainIntensity = (0.12f - 0.08f * Focus) * GrainScale;
	P.MotionBlurAmount = bMotionBlur ? 0.35f : 0.0f;
	P.VignetteIntensity = 0.5f + Tilt * 0.6f + Pulse * 0.35f;
	// Tilt: color drains toward a hot red.
	const float Sat = 1.0f - 0.55f * Tilt;
	P.ColorSaturation = FVector4(Sat, Sat, Sat, 1.0f);
	P.ColorGain = FVector4(1.0f + 0.2f * Tilt, 1.0f - 0.25f * Tilt, 1.0f - 0.25f * Tilt, 1.0f);
	P.AutoExposureBias = ExposureBias + BrightnessBias;
}

// ------------------------------------------------------------------ arms

FVector ANightOneStage::KeyWorld(const FString& Name) const
{
	FVector K = LaptopKey(5.25, 2); // F
	if (Name == TEXT("r")) K = LaptopKey(5.0, 1);
	else if (Name == TEXT("a")) K = LaptopKey(2.25, 2);
	else if (Name == TEXT("x")) K = LaptopKey(3.75, 3);
	else if (Name == TEXT("c")) K = LaptopKey(4.75, 3);
	else if (Name == TEXT("b")) K = LaptopKey(6.75, 3);
	else if (Name == TEXT("m")) K = LaptopKey(8.75, 3);
	else if (Name == TEXT(" ")) K = LaptopKey(7.375, 4);
	else if (Name == TEXT("ArrowUp")) K = LaptopKey(13.5, 4, 0.0042);
	else if (Name == TEXT("ArrowDown")) K = LaptopKey(13.5, 4, -0.0042);
	// Laptop base coordinates (origin under the hinge, front toward -y) into the laptop's frame on the desk.
	const FVector Local = Web(K.X, K.Z, -LaptopDepth / 2.0 - K.Y);
	return LaptopFrame ? LaptopFrame->GetComponentTransform().TransformPosition(Local) : GetActorTransform().TransformPosition(Local);
}

void ANightOneStage::ArmsKey(const FString& Name)
{
	if (Arms.IsReady())
	{
		Arms.Key(Name, KeyWorld(Name));
	}
}

void ANightOneStage::ArmsClick()
{
	if (Arms.IsReady())
	{
		Arms.Click();
	}
}

void ANightOneStage::UpdateArms(float DeltaSeconds, const FVector& CameraLocation, const FVector& CameraForward, const FVector2D& Pointer, bool bVisible)
{
	const float Dt = FMath::Max(0.0f, DeltaSeconds);
	// The mouse follows the hand that moves it: a few centimeters across the pad for the whole screen.
	if (MouseRoot)
	{
		const FVector2D Goal((Pointer.X - 0.5) * 0.08, (Pointer.Y - 0.5) * 0.06);
		MouseOffset += (Goal - MouseOffset) * static_cast<double>(1.0f - FMath::Exp(-Dt * 14.0f));
		MouseRoot->SetRelativeLocation(MouseHome + Web(MouseOffset.X, 0.0, MouseOffset.Y));
	}
	if (!ArmsComp || !ArmsRoot)
	{
		return;
	}
	ArmsComp->SetVisibility(bVisible);
	// Shoulders 13 cm ahead of and 20 cm below the eye (the player sits up to the laptop); they follow the
	// head most of the way when it moves, but not into the establishing shot across the room.
	const FTransform& X = GetActorTransform();
	const FVector EyeLocal = Web(0.0, 1.17, DeskZ + 0.72);
	FVector Head = X.InverseTransformPosition(CameraLocation) - EyeLocal;
	if (Head.Size() > 60.0)
	{
		Head = FVector::ZeroVector;
	}
	// Turned well off to a side, the eye would look down onto the tops of the shoulders, right under it: they drop
	// and ease back out of view (closer to the hands, so the hands keep their places).
	const FVector Look = X.InverseTransformVectorNoScale(CameraForward);
	const float Turn = FMath::Abs(FMath::Atan2(static_cast<float>(Look.Y), static_cast<float>(FMath::Max(Look.X, 0.01))));
	const float AwayGoal = FMath::SmoothStep(0.3f, 0.75f, Turn);
	ArmsAway += (AwayGoal - ArmsAway) * (1.0f - FMath::Exp(-Dt * 6.0f));
	ArmsRoot->SetRelativeLocation(EyeLocal + Web(0.0, -0.20 - 0.10 * ArmsAway, -0.13 + 0.05 * ArmsAway) + Head * 0.7);
	if (!Arms.IsReady())
	{
		if (ArmsInitTries++ > 120 || !Arms.Init(ArmsComp))
		{
			return;
		}
	}
	if (!bVisible)
	{
		return;
	}
	FFirstPersonArms::FTargets T;
	T.Forward = X.TransformVectorNoScale(FVector(1.0, 0.0, 0.0));
	T.Up = X.TransformVectorNoScale(FVector(0.0, 0.0, 1.0));
	if (LaptopFrame)
	{
		// The left wrist rests on the front of the palm rest, fingertips over the home row.
		T.LeftRest = LaptopFrame->GetComponentTransform().TransformPosition(Web(-0.058, 0.032, -LaptopDepth / 2.0 + 0.225));
	}
	T.Mouse = MouseRoot ? MouseRoot->GetComponentLocation() + T.Up * MouseTop : X.TransformPosition(MouseHome);
	Arms.Update(Dt, T);
}
