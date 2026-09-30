#include "NightOneStage.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
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
	BuildShell();
	BuildWindow();
	BuildDesk();
	BuildProps();
	BuildOutside();
	BuildLights();
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
	UMaterialInterface* Wood = Surface(TEXT("Desk"), 0x5c3821, 0.6f, 0.0f, PatternWood);
	UMaterialInterface* Legs = Surface(TEXT("Legs"), 0x1c1d20, 0.5f, 0.7f);
	BoxWeb(Wood, FVector(0.0, DeskTop - 0.0175, DeskZ), FVector(1.6, 0.035, 0.7));
	for (const FVector2D& P : {FVector2D(-0.76, DeskZ - 0.3), FVector2D(0.76, DeskZ - 0.3), FVector2D(-0.76, DeskZ + 0.3), FVector2D(0.76, DeskZ + 0.3)})
	{
		BoxWeb(Legs, FVector(P.X, (DeskTop - 0.035) / 2.0, P.Y), FVector(0.035, DeskTop - 0.035, 0.035));
	}

	// Laptop.
	UMaterialInterface* Body = Surface(TEXT("Laptop"), 0x2b2d31, 0.38f, 0.85f, PatternBrushed);
	USceneComponent* Laptop = NewPart<USceneComponent>();
	Laptop->SetRelativeLocation(Web(0.0, DeskTop, DeskZ + 0.04));
	const double T = 0.016;
	const double D = 0.235;
	const double LaptopW = 0.34;
	BoxWeb(Body, FVector(0.0, T / 2.0, 0.0), FVector(LaptopW, T, D), 0.0f, Laptop);
	BoxWeb(Surface(TEXT("Trackpad"), 0x1f2124, 0.45f, 0.5f), FVector(0.0, T + 0.0005, 0.068), FVector(0.11, 0.001, 0.07), 0.0f, Laptop);
	Keyboard = AddWidget(Web(0.0, T + 0.0008, -0.035), Facing(FVector::UpVector, FVector::ForwardVector), FVector2D(31.0, 11.8), FIntPoint(1024, 390), true, false, Laptop);
	// Lid, hinged at the back edge and tilted back.
	LidPivot = NewPart<USceneComponent>(Laptop);
	LidPivot->SetRelativeLocationAndRotation(Web(0.0, T, -D / 2.0), FRotator(-18.3f, 0.0f, 0.0f));
	const double LidH = 0.225;
	BoxWeb(Body, FVector(0.0, LidH / 2.0, -0.0035), FVector(LaptopW, LidH, 0.007), 0.0f, LidPivot);
	BoxWeb(Surface(TEXT("Bezel"), 0x050506, 0.35f, 0.2f), FVector(0.0, LidH / 2.0, 0.00015), FVector(LaptopW - 0.006, LidH - 0.006, 0.0003), 0.0f, LidPivot);
	Screen = AddWidget(Web(0.0, LidH / 2.0 + 0.004, 0.0006), FRotator(0.0f, 180.0f, 0.0f), ScreenSize(), ScreenResolution, false, false, LidPivot);
	// Slightly below full white so only the brightest pixels bloom, like a real panel.
	Screen->SetTintColorAndOpacity(FLinearColor(0.86f, 0.86f, 0.86f, 1.0f));
	Screen->SetRedrawTime(1.0f / 30.0f);

	// Mouse and pad.
	BoxWeb(Surface(TEXT("MousePad"), 0x131417, 0.95f), FVector(0.33, DeskTop + 0.0015, DeskZ + 0.13), FVector(0.26, 0.003, 0.21));
	AddMesh(SphereMesh, Surface(TEXT("Mouse"), 0x1d1e22, 0.35f), Web(0.34, DeskTop + 0.012, DeskZ + 0.15), FVector(0.099, 0.06, 0.027));

	// Phone, face up; its lock screen lights when a text arrives.
	USceneComponent* Phone = NewPart<USceneComponent>();
	Phone->SetRelativeLocationAndRotation(Web(0.28, DeskTop, DeskZ - 0.12), FRotator(0.0f, 14.3f, 0.0f));
	BoxWeb(Surface(TEXT("Phone"), 0x15161a, 0.3f, 0.5f), FVector(0.0, 0.004, 0.0), FVector(0.074, 0.008, 0.155), 0.0f, Phone);
	PhoneScreen = AddWidget(Web(0.0, 0.0082, 0.0), Facing(FVector::UpVector, FVector::ForwardVector), FVector2D(6.8, 14.6), FIntPoint(360, 760), false, false, Phone);
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

	// Mug of coffee.
	UMaterialInterface* Ceramic = Surface(TEXT("Mug"), 0xd8d2c4, 0.25f);
	CylinderWeb(Ceramic, FVector(-0.4, DeskTop, DeskZ + 0.06), 0.041f, 0.094f);
	CylinderWeb(Surface(TEXT("Coffee"), 0x1a0d06, 0.05f), FVector(-0.4, DeskTop + 0.094, DeskZ + 0.06), 0.035f, 0.001f);
	BoxWeb(Ceramic, FVector(-0.4 - 0.045, DeskTop + 0.048, DeskZ + 0.06 - 0.01), FVector(0.012, 0.05, 0.01), -137.0f);

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

	// Desk lamp (off): a dark silhouette against the window.
	UMaterialInterface* Lamp = Surface(TEXT("Lamp"), 0x1a1a1c, 0.4f, 0.6f);
	CylinderWeb(Lamp, FVector(-0.66, DeskTop, DeskZ - 0.24), 0.065f, 0.02f);
	AddMesh(CylinderMesh, Lamp, Web(-0.66, DeskTop + 0.18, DeskZ - 0.26), FVector(0.014, 0.014, 0.36), FRotator(-8.6f, 0.0f, 0.0f));
	AddMesh(CylinderMesh, Lamp, Web(-0.6, DeskTop + 0.38, DeskZ - 0.2), FVector(0.014, 0.014, 0.3), FRotator(0.0f, 0.0f, 57.0f));
	AddMesh(ConeMesh, Lamp, Web(-0.49, DeskTop + 0.42, DeskZ - 0.2), FVector(0.12, 0.12, 0.1), FRotator(0.0f, 0.0f, 132.0f));

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
	// Building across the street.
	Buildings->AddInstance(FTransform(FRotator::ZeroRotator, Web(6.0, -16.0 + 20.0, -22.0), FVector(8.0, 60.0, 40.0)));

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
	// The laptop screen is the key light.
	ScreenLight = NewPart<URectLightComponent>(LidPivot);
	ScreenLight->SetRelativeLocationAndRotation(Web(0.0, 0.225 / 2.0 + 0.004, 0.006), FRotator(0.0f, 180.0f, 0.0f));
	ScreenLight->SetIntensityUnits(ELightUnits::Candelas);
	ScreenLight->SetIntensity(ScreenLightCandela);
	ScreenLight->SetLightColor(ScreenGlow);
	ScreenLight->SetSourceWidth(31.6f);
	ScreenLight->SetSourceHeight(19.75f);
	ScreenLight->SetAttenuationRadius(400.0f);
	ScreenLight->SetCastShadows(true);

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
	P.AutoExposureBias = ExposureBias;
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
	P.AutoExposureBias = ExposureBias;
}
