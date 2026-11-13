// Update 4, Festival Season (SPEC v1.7 §9.4, §2.6.4, §9.1 rules 5 and 7; G1, F-look FL-9): the apartment's festival shelf and cork pin
// board (FestShelf.h). Where they stand, in the apartment's prototype meters (NightOneStage.cpp's Web: x right, y up, z toward the
// viewer): the shelf's back against the right wall (x 2.35), its middle at z 1.0, between the window wall and the door; the board on
// the left wall (x -1.75) beside the desk's left end, its middle 1.40 m up at z -0.30. The kit's numbers (art/blender/assets/
// fest_shelf.py, fest_pinboard.py; FL-8a) are copied below. A piece's offsets are in its Blender frame (x right, y away from its front,
// z up), placed as ANightOneStage::BuildGear places a prop: the axes (y, x, z) in a root turned by the piece's facing, the imported mesh
// at BlenderImportYaw within it.
#include "FestShelf.h"

#include "BlenderProps.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NightOneGame.h"
#include "NightOneGameMode.h"
#include "NightOneStage.h"
#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Game/WorldFest.h"
#include "ShortStack/UI/Canvas.h"
#include "ShortStack/UI/FestivalArt.h"
#include "SlateDrawList.h"

#include <algorithm>
#include <array>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace FestShelfDetail
{
// ---- FestShelfDetail's plan begins (std and ShortStackCore only, no engine type: ssb/g1-flook/fl9's check compiles this block alone)
constexpr int FsBrands = static_cast<int>(ss::fest::Brand::Count);
constexpr int FsBeltPlaces = 3; // fest_shelf.py's BELT_SLOTS (the shelf's budget counts three belts)
constexpr int FsPinRows = 4;    // fest_pinboard.py's PIN_SLOTS: four rows of ten, a festival year a row
constexpr int FsPinCols = 10;
constexpr size_t FsCrownTail = 4; // a crown Honor's EventId is the edition's series id + "#pos" (§2.6.1)

/** One pin on the board: Grade 1 outline (played), 2 silver (cashed), 3 gold (won an event); Row 0 the newest year with a pin (the top
 *  row); Col the edition's place in its year's calendar order (fest::All() is in FirstDay order). */
struct FFsPin
{
	ss::fest::Brand Brand = ss::fest::Brand::None;
	int Grade = 0;
	int Row = 0;
	int Col = 0;
};

/** What the shelf and the board show (§2.6.4, derived; nothing of it is saved). */
struct FFsPlan
{
	/** By brand: the newest won edition's name ("RCOP 2026"), its trophy on the brand's plinth; "" none (the plate reads U39). */
	std::array<std::string, FsBrands> Won;
	/** Up to three, the newest crown first, one a brand. */
	std::vector<ss::fest::Brand> Belts;
	std::vector<FFsPin> Pins;
};

/** Every edition's Main template id -> its edition (fest::All() never changes once built, so its rows' addresses hold). */
const std::map<std::string, const ss::fest::Festival*>& FsMains()
{
	static const std::map<std::string, const ss::fest::Festival*> FsMainIds = [] {
		std::map<std::string, const ss::fest::Festival*> M;
		for (const ss::fest::Festival& F : ss::fest::All())
		{
			if (!F.MainId.empty() && F.Which != ss::fest::Brand::None)
			{
				M.emplace(F.MainId, &F);
			}
		}
		return M;
	}();
	return FsMainIds;
}

/**
 * The plan from the hero's Honors and pins (§2.6.4, §9.4): a trophy for each festival whose Main the hero won (the hero's Main Honor,
 * Npc -1, its template the edition's MainId), a belt for each crown (the hero's crown Honor), the pins as world::HeroFestPins gives
 * them. Nobody else's Honor places anything; a championship's Mini and Micro (title objects) are 2D only, never on the shelf.
 */
FFsPlan FsPlanOf(const std::vector<ss::world::Honor>& Honors, const std::vector<ss::world::HeroFestPin>& Pins)
{
	FFsPlan P;
	std::array<double, FsBrands> WonAt;
	std::array<double, FsBrands> CrownAt;
	WonAt.fill(-1.0);
	CrownAt.fill(-1.0);
	const std::map<std::string, const ss::fest::Festival*>& Mains = FsMains();
	for (const ss::world::Honor& H : Honors)
	{
		if (H.Npc != -1)
		{
			continue;
		}
		if (ss::world::IsCrownHonor(H))
		{
			const ss::fest::Festival* F = ss::fest::FestivalOf(H.EventId.substr(0, H.EventId.size() - FsCrownTail));
			const int B = F ? static_cast<int>(F->Which) : 0;
			if (B > 0 && B < FsBrands && H.At > CrownAt[B])
			{
				CrownAt[B] = H.At;
			}
			continue;
		}
		const auto It = Mains.find(H.EventId.substr(0, H.EventId.find('@')));
		const int B = It == Mains.end() ? 0 : static_cast<int>(It->second->Which);
		if (B > 0 && B < FsBrands && H.At > WonAt[B])
		{
			WonAt[B] = H.At;
			P.Won[B] = It->second->Name;
		}
	}
	for (int B = 1; B < FsBrands; ++B)
	{
		if (CrownAt[B] >= 0.0)
		{
			P.Belts.push_back(static_cast<ss::fest::Brand>(B));
		}
	}
	std::stable_sort(P.Belts.begin(), P.Belts.end(),
		[&CrownAt](ss::fest::Brand A, ss::fest::Brand Z) { return CrownAt[static_cast<int>(A)] > CrownAt[static_cast<int>(Z)]; });
	if (P.Belts.size() > static_cast<size_t>(FsBeltPlaces))
	{
		P.Belts.resize(FsBeltPlaces);
	}
	// The board: the newest four years with a pin, the newest on top.
	std::vector<int> Years;
	for (const ss::world::HeroFestPin& Pin : Pins)
	{
		const ss::fest::Festival* F = ss::fest::FestivalOf(Pin.Key);
		if (F && Pin.Grade > 0 && F->Which != ss::fest::Brand::None)
		{
			Years.push_back(F->Year);
		}
	}
	std::sort(Years.begin(), Years.end(), std::greater<int>());
	Years.erase(std::unique(Years.begin(), Years.end()), Years.end());
	if (Years.size() > static_cast<size_t>(FsPinRows))
	{
		Years.resize(FsPinRows);
	}
	for (const ss::world::HeroFestPin& Pin : Pins)
	{
		const ss::fest::Festival* F = ss::fest::FestivalOf(Pin.Key);
		if (!F || Pin.Grade <= 0 || F->Which == ss::fest::Brand::None)
		{
			continue;
		}
		const auto Row = std::find(Years.begin(), Years.end(), F->Year);
		if (Row == Years.end())
		{
			continue;
		}
		int Col = 0;
		for (const ss::fest::Festival& Other : ss::fest::All())
		{
			if (&Other == F)
			{
				break;
			}
			Col += Other.Year == F->Year ? 1 : 0;
		}
		if (Col < FsPinCols)
		{
			P.Pins.push_back({F->Which, std::min(Pin.Grade, 3), static_cast<int>(Row - Years.begin()), Col});
		}
	}
	return P;
}

/** The plan as a key (a change rebuilds the pieces and redraws the plates). */
std::string FsKey(const FFsPlan& P, bool bPacked)
{
	std::string K = bPacked ? "packed|" : "room|";
	for (int B = 1; B < FsBrands; ++B)
	{
		K += P.Won[B];
		K += '|';
	}
	for (const ss::fest::Brand B : P.Belts)
	{
		K += std::to_string(static_cast<int>(B)) + ",";
	}
	K += '|';
	for (const FFsPin& Pin : P.Pins)
	{
		K += std::to_string(static_cast<int>(Pin.Brand)) + ":" + std::to_string(Pin.Grade) + ":" + std::to_string(Pin.Row) + ":" + std::to_string(Pin.Col) + ";";
	}
	return K;
}

/** The plinth a brand stands on (fest_shelf.py's SLOTS: the bottom tier 0-4, then the middle tier 5-9, each left to right): the middle
 *  tier, at eye level, holds the first five brands in fest::Brand's order (RCOP, Ring Rush, TCO, Summer Slam, FEIGN), the bottom tier
 *  the other five (PUSH, WRIT, GRIT, PENNANT, REIGN). */
int FsSlotOf(ss::fest::Brand B)
{
	const int I = static_cast<int>(B) - 1;
	return I < 5 ? 5 + I : I - 5;
}

ss::fest::Brand FsBrandAt(int Slot)
{
	return static_cast<ss::fest::Brand>(Slot >= 5 ? Slot - 4 : Slot + 6);
}

/** U39's template, FestArtInventory's plate entry ("{SHORT} · not yet", §2.11), "" when the inventory holds none. */
std::string FsPlateTemplate()
{
	static const std::string FsShortField = "{SHORT}";
	for (const std::string& E : ss::fest::FestArtInventory())
	{
		const size_t Tab = E.find('\t');
		const std::string Text = Tab == std::string::npos ? E : E.substr(Tab + 1);
		if (Text.compare(0, FsShortField.size(), FsShortField) == 0 && Text.find("not yet") != std::string::npos)
		{
			return Text;
		}
	}
	return std::string();
}

/** A brand's series Short ("RCOP", "FEIGN"): its first edition's, from the edition's site; "" when the brand has none. */
std::string FsShortOf(ss::fest::Brand B)
{
	for (const ss::fest::Festival& F : ss::fest::All())
	{
		if (F.Which == B)
		{
			const ss::net::SeriesInfo* S = ss::net::Site(F.Site).FindSeries(F.SeriesId);
			return S ? S->Short : std::string();
		}
	}
	return std::string();
}

/** A plate's engraving: the won edition's name, else U39 with {SHORT} filled; "" when neither can be made. */
std::string FsPlateLine(const FFsPlan& P, ss::fest::Brand B, const std::string& Template)
{
	const int I = static_cast<int>(B);
	if (I > 0 && I < FsBrands && !P.Won[I].empty())
	{
		return P.Won[I];
	}
	const std::string Short = FsShortOf(B);
	if (Template.size() < 7 || Short.empty())
	{
		return std::string();
	}
	return Short + Template.substr(7);
}
// ---- FestShelfDetail's plan ends

// The kit's numbers (meters, in each piece's Blender frame; fest_shelf.py and fest_pinboard.py).
constexpr double FsShelfW = 1.20, FsShelfD = 0.28, FsSideT = 0.022, FsBoardT = 0.028, FsBackT = 0.008, FsKick = 0.09, FsTier = 0.43;
constexpr double FsTierZ[2] = {FsKick + FsBoardT, FsKick + FsBoardT + FsTier + FsBoardT}; // the two tiers' surfaces: 0.118, 0.576
constexpr double FsTopZ = FsTierZ[1] + FsTier;                                         // the top board's underside: 1.006
constexpr double FsTopT = 0.03;
constexpr double FsPlinthW = 0.17, FsPlinthH = 0.07, FsSlotY = -0.02;
constexpr double FsInnerW = FsShelfW - 2.0 * FsSideT;
constexpr double FsSlotPitch = FsInnerW / 5.0;
constexpr double FsBeltX[FsBeltPlaces] = {-0.39, 0.0, 0.39};
constexpr double FsPlateW = 0.11, FsPlateH = 0.026, FsPlateT = 0.0025;
constexpr double FsPlateY = -0.075, FsPlateZ = 0.035; // PLATE_AT: the plate's back face's middle, from its plinth's origin
constexpr double FsBoardW = 0.62, FsBoardH = 0.46, FsBoardD = 0.022, FsFrameW = 0.028, FsCorkY = -0.014;
constexpr double FsPinX0 = -0.2475, FsPinDx = 0.055, FsPinZ0 = 0.141, FsPinDz = 0.094; // PIN_SLOTS
constexpr double FsPinR = 0.012, FsPinFieldR = 0.0092;                                 // the rim and the enamel field (PIN_R; the lathe's field)
// Where they stand in the apartment (prototype meters, NightOneStage.cpp's RoomRight and RoomLeft) and which way they face (0 is +X,
// 180 the seat: ANightOneStage::PropFacing's convention).
constexpr double FsRoomRight = 2.35, FsRoomLeft = -1.75;
constexpr double FsShelfAtX = FsRoomRight - FsShelfD / 2.0 - 0.005, FsShelfAtZ = 1.0;
constexpr double FsBoardAtX = FsRoomLeft + 0.004, FsBoardAtY = 1.40, FsBoardAtZ = -0.30;
constexpr float FsShelfFacing = -90.0f; // into the room from the right wall (Unreal -Y)
constexpr float FsBoardFacing = 90.0f;  // into the room from the left wall (Unreal +Y)
// The plates' engraving strip: canvas units are millimetres across the tier's five plates.
constexpr float FsStripW = static_cast<float>(FsInnerW * 1000.0), FsStripH = static_cast<float>(FsPlateH * 1000.0);
constexpr int32 FsStripPxW = 2312, FsStripPxH = 52;
constexpr float FsEngraveSize = 10.0f, FsEngraveFit = 98.0f; // millimetres: the letters' size, the width a line keeps inside its plate
// M_Surface's patterns (Content/Python/init_unreal.py; NightOneStage.cpp's PatternNone and PatternWood).
constexpr float FsPatternNone = 0.0f, FsPatternWood = 2.0f;

/** Prototype meters to Unreal centimetres (NightOneStage.cpp's Web). */
FVector FsWeb(double X, double Y, double Z)
{
	return FVector(-Z * 100.0, X * 100.0, Y * 100.0);
}

/** A point in a piece's Blender frame (meters) in its root's space (centimetres): BuildGear's BlenderAxes. */
FVector FsAxes(double Bx, double By, double Bz)
{
	return FVector(By * 100.0, Bx * 100.0, Bz * 100.0);
}

/** A plinth's origin (fest_shelf.py's SLOTS[Slot]) in the shelf root's space. */
FVector FsSlotAt(int Slot)
{
	return FsAxes(-FsInnerW / 2.0 + FsSlotPitch * (Slot % 5 + 0.5), FsSlotY, FsTierZ[Slot / 5]);
}

FLinearColor FsLinear(uint32 Hex)
{
	return FLinearColor::FromSRGBColor(FColor(static_cast<uint8>((Hex >> 16) & 0xff), static_cast<uint8>((Hex >> 8) & 0xff), static_cast<uint8>(Hex & 0xff)));
}

/** An outline pin's dulled enamel (EventArt's FestPinArt: Sink(Accent, 0.5)). */
uint32 FsSink(uint32 Hex)
{
	return ((((Hex >> 16) & 0xffu) / 2u) << 16) | ((((Hex >> 8) & 0xffu) / 2u) << 8) | ((Hex & 0xffu) / 2u);
}

/** The trophy each brand's plinth holds (FL-8b's FS_TROPHIES; Ring Rush's ring and the Championship Online's bracelet in FL-8a's cases). */
const TCHAR* FsTrophyMesh(ss::fest::Brand B)
{
	switch (B)
	{
	case ss::fest::Brand::Rcop: return TEXT("SM_FS_Cup");
	case ss::fest::Brand::RingRush: return TEXT("SM_FS_CaseRing");
	case ss::fest::Brand::Tco: return TEXT("SM_FS_CaseBracelet");
	case ss::fest::Brand::Slam: return TEXT("SM_FS_SlamPlate");
	case ss::fest::Brand::Feign: return TEXT("SM_FS_Mirror");
	case ss::fest::Brand::Push: return TEXT("SM_FS_Button");
	case ss::fest::Brand::Writ: return TEXT("SM_FS_Seal");
	case ss::fest::Brand::Grit: return TEXT("SM_FS_Grindstone");
	case ss::fest::Brand::Pennant: return TEXT("SM_FS_Pennant");
	case ss::fest::Brand::Reign: return TEXT("SM_FS_Sceptre");
	default: return nullptr;
	}
}

/** A belt's strap tint (UI/EventArt.cpp's FestBeltOf straps, so the shelf's belt matches the laptop's). */
uint32 FsBeltStrap(ss::fest::Brand B)
{
	switch (B)
	{
	case ss::fest::Brand::Rcop: return 0x8e6cf0;
	case ss::fest::Brand::RingRush: return 0xa3173e;
	case ss::fest::Brand::Tco: return 0x2147a8;
	case ss::fest::Brand::Slam: return 0xff9a52;
	case ss::fest::Brand::Feign: return 0xffe3f1;
	case ss::fest::Brand::Push: return 0x2e2866;
	case ss::fest::Brand::Writ: return 0x2a2a30;
	case ss::fest::Brand::Grit: return 0x8a1a2a;
	case ss::fest::Brand::Pennant: return 0x2fe068;
	case ss::fest::Brand::Reign: return 0x2a2a30;
	default: return 0x8a6a32;
	}
}

/**
 * SM_FS_<Name> (SPEC §9.1 rule 7): the CC0 build under the git-ignored ThirdParty folder (/Game/ThirdParty/FestShelf/<mesh>, the lead's
 * import of g1/bakes/FL-8a_cc0 and FL-8b_cc0), else the committed procedural build the editor's setup script imports
 * (/Game/ShortStack/Meshes/<mesh>/<mesh>, from unreal/Art/Meshes/<mesh>.glb), else null (the caller's engine shapes).
 */
UStaticMesh* FsLoad(const TCHAR* Name)
{
	if (!Name)
	{
		return nullptr;
	}
	if (UStaticMesh* Cc0 = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/ThirdParty/FestShelf/%s.%s"), Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		return Cc0;
	}
	return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/ShortStack/Meshes/%s/%s.%s"), Name, Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
}
} // namespace FestShelfDetail

UFestShelf::UFestShelf()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.5f; // twice a second, as the stage's sponsor props and calendar
}

template <typename T>
T* UFestShelf::FsPart(USceneComponent* Parent)
{
	// ANightOneStage::NewPart's pattern: a construction-script part of the owning stage, rebuilt with the set.
	AActor* Owner = GetOwner();
	T* Part = NewObject<T>(Owner ? static_cast<UObject*>(Owner) : static_cast<UObject*>(this));
	Part->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	Part->SetupAttachment(Parent ? Parent : static_cast<USceneComponent*>(this));
	if (Owner)
	{
		Owner->BlueprintCreatedComponents.Add(Part);
	}
	Part->RegisterComponent();
	return Part;
}

UMaterialInstanceDynamic* UFestShelf::FsSurface(uint32 SrgbHex, float Roughness, float Metallic, float Pattern)
{
	const FString Key = FString::Printf(TEXT("%06x/%.2f/%.2f/%.0f"), SrgbHex, Roughness, Metallic, Pattern);
	if (TObjectPtr<UMaterialInstanceDynamic>* Found = Surfaces.Find(Key))
	{
		return Found->Get();
	}
	if (!SurfaceMaterial)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(SurfaceMaterial, this);
	const FLinearColor Color = FestShelfDetail::FsLinear(SrgbHex);
	M->SetVectorParameterValue(TEXT("BaseColor"), Color);
	M->SetVectorParameterValue(TEXT("Color"), Color); // BasicShapeMaterial's
	M->SetScalarParameterValue(TEXT("Roughness"), Roughness);
	M->SetScalarParameterValue(TEXT("Metallic"), Metallic);
	M->SetScalarParameterValue(TEXT("Pattern"), Pattern);
	Surfaces.Add(Key, M);
	return M;
}

UStaticMeshComponent* UFestShelf::FsMesh(UStaticMesh* Mesh, UMaterialInterface* Material, const FVector& Location, const FRotator& Rotation, const FVector& Scale,
	USceneComponent* Parent, bool bShadows)
{
	UStaticMeshComponent* C = FsPart<UStaticMeshComponent>(Parent);
	C->SetStaticMesh(Mesh);
	C->SetForceDisableNanite(true); // small props (§9.4; the realism update's wardrobe lesson)
	C->SetRelativeLocationAndRotation(Location, Rotation);
	C->SetRelativeScale3D(Scale);
	if (Material)
	{
		C->SetMaterial(0, Material);
	}
	C->SetCastShadow(bShadows);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	return C;
}

UInstancedStaticMeshComponent* UFestShelf::FsInstanced(UStaticMesh* Mesh, UMaterialInterface* Material, USceneComponent* Parent, bool bShadows)
{
	UInstancedStaticMeshComponent* C = FsPart<UInstancedStaticMeshComponent>(Parent);
	C->SetStaticMesh(Mesh);
	C->SetForceDisableNanite(true);
	if (Material)
	{
		C->SetMaterial(0, Material);
	}
	C->SetCastShadow(bShadows);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	return C;
}

void UFestShelf::Build()
{
	namespace D = FestShelfDetail;
	SurfaceMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ShortStack/Materials/M_Surface.M_Surface"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!SurfaceMaterial)
	{
		SurfaceMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}
	CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	PinMesh = D::FsLoad(TEXT("SM_FS_Pin"));
	Surfaces.Reset();
	Trophies.Reset();
	Belts.Reset();
	PlateStrips.Reset();
	PinGroups.Reset();
	PlateSlates.Reset();
	Shown.clear();
	bPlatesDrawn = false;
	// An imported piece's front (Blender -Y) toward its root's -X; the root's yaw turns it to face the room.
	const FRotator Front(0.0f, BlenderImportYaw(), 0.0f);

	ShelfRoot = FsPart<USceneComponent>(this);
	ShelfRoot->SetRelativeLocationAndRotation(D::FsWeb(D::FsShelfAtX, 0.0, D::FsShelfAtZ), FRotator(0.0f, D::FsShelfFacing - 180.0f, 0.0f));
	BoardRoot = FsPart<USceneComponent>(this);
	BoardRoot->SetRelativeLocationAndRotation(D::FsWeb(D::FsBoardAtX, D::FsBoardAtY, D::FsBoardAtZ), FRotator(0.0f, D::FsBoardFacing - 180.0f, 0.0f));

	// The shelf, its ten plinths and their brass plates (each instanced once: one draw for the ten).
	if (UStaticMesh* Shelf = D::FsLoad(TEXT("SM_FS_Shelf")))
	{
		FsMesh(Shelf, nullptr, FVector::ZeroVector, Front, FVector(1.0), ShelfRoot);
	}
	else
	{
		BuildShelfShapes();
	}
	UStaticMesh* Plinth = D::FsLoad(TEXT("SM_FS_Plinth"));
	UStaticMesh* Plate = D::FsLoad(TEXT("SM_FS_Plate"));
	UInstancedStaticMeshComponent* Plinths = FsInstanced(Plinth ? Plinth : CubeMesh.Get(), Plinth ? nullptr : FsSurface(0x2a211b, 0.55f, 0.0f, D::FsPatternWood), ShelfRoot);
	UInstancedStaticMeshComponent* Plates = FsInstanced(Plate ? Plate : CubeMesh.Get(), Plate ? nullptr : FsSurface(0xc9a55a, 0.3f, 1.0f), ShelfRoot, false);
	for (int32 Slot = 0; Slot < 10; ++Slot)
	{
		const FVector At = D::FsSlotAt(Slot);
		const FVector PlateAt = At + D::FsAxes(0.0, D::FsPlateY, D::FsPlateZ);
		Plinths->AddInstance(Plinth ? FTransform(Front, At) : FTransform(FRotator::ZeroRotator, At + D::FsAxes(0.0, 0.0, D::FsPlinthH / 2.0), FVector(D::FsPlinthW, D::FsPlinthW, D::FsPlinthH)));
		Plates->AddInstance(Plate ? FTransform(Front, PlateAt)
								  : FTransform(FRotator::ZeroRotator, PlateAt + D::FsAxes(0.0, -D::FsPlateT / 2.0, 0.0), FVector(D::FsPlateT, D::FsPlateW, D::FsPlateH)));
	}

	// The ten trophies (hidden until won), each standing on its brand's plinth.
	Trophies.SetNum(D::FsBrands);
	for (int32 B = 1; B < D::FsBrands; ++B)
	{
		const ss::fest::Brand Which = static_cast<ss::fest::Brand>(B);
		USceneComponent* Group = FsPart<USceneComponent>(ShelfRoot);
		Group->SetRelativeLocation(D::FsSlotAt(D::FsSlotOf(Which)) + D::FsAxes(0.0, 0.0, D::FsPlinthH));
		if (UStaticMesh* Trophy = D::FsLoad(D::FsTrophyMesh(Which)))
		{
			FsMesh(Trophy, nullptr, FVector::ZeroVector, Front, FVector(1.0), Group);
		}
		else
		{
			// A gold cylinder and its cap (the engine's cylinder is 100 cm across and tall, its pivot at its middle).
			UMaterialInterface* Gold = FsSurface(0xe8bd62, 0.25f, 1.0f);
			FsMesh(CylinderMesh, Gold, D::FsAxes(0.0, 0.0, 0.08), FRotator::ZeroRotator, FVector(0.05, 0.05, 0.16), Group);
			FsMesh(CylinderMesh, Gold, D::FsAxes(0.0, 0.0, 0.17), FRotator::ZeroRotator, FVector(0.09, 0.09, 0.02), Group);
		}
		Group->SetVisibility(false, true);
		Trophies[B] = Group;
	}

	// The three belt places on the top board (hidden until a crown; tinted by Apply).
	UStaticMesh* Belt = D::FsLoad(TEXT("SM_FS_Belt"));
	for (int32 I = 0; I < D::FsBeltPlaces; ++I)
	{
		const FVector At = D::FsAxes(D::FsBeltX[I], 0.0, D::FsTopZ + D::FsTopT);
		UStaticMeshComponent* C = Belt ? FsMesh(Belt, nullptr, At, Front, FVector(1.0), ShelfRoot)
									   : FsMesh(CubeMesh, nullptr, At + D::FsAxes(0.0, 0.0, 0.006), FRotator::ZeroRotator, FVector(0.08, 0.30, 0.012), ShelfRoot);
		C->SetVisibility(false);
		Belts.Add(C);
	}

	// The engraving strips over each tier's plates (the bottom tier's, then the middle tier's), facing the room; drawn at play.
	for (int32 Tier = 0; Tier < 2; ++Tier)
	{
		UWidgetComponent* W = FsPart<UWidgetComponent>(ShelfRoot);
		W->SetWidgetSpace(EWidgetSpace::World);
		W->SetDrawSize(FVector2D(D::FsStripPxW, D::FsStripPxH));
		W->SetPivot(FVector2D(0.5, 0.5));
		W->SetBlendMode(EWidgetBlendMode::Transparent);
		W->SetTwoSided(false);
		W->SetBackgroundColor(FLinearColor::Transparent);
		W->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		W->SetCastShadow(false);
		// 0.8 mm in front of the plates' faces; the quad lies in its local Y (width) / Z (height) plane, one unit a pixel.
		W->SetRelativeLocationAndRotation(D::FsAxes(0.0, D::FsSlotY + D::FsPlateY - D::FsPlateT - 0.0008, D::FsTierZ[Tier] + D::FsPlateZ), FRotator(0.0f, 180.0f, 0.0f));
		W->SetRelativeScale3D(FVector(1.0, D::FsInnerW * 100.0 / D::FsStripPxW, D::FsPlateH * 100.0 / D::FsStripPxH));
		PlateStrips.Add(W);
	}

	// The cork pin board by the desk; its pins come with the career (ShowPins).
	if (UStaticMesh* Board = D::FsLoad(TEXT("SM_FS_PinBoard")))
	{
		FsMesh(Board, nullptr, FVector::ZeroVector, Front, FVector(1.0), BoardRoot);
	}
	else
	{
		BuildBoardShapes();
	}
}

void UFestShelf::BuildShelfShapes()
{
	namespace D = FestShelfDetail;
	UMaterialInterface* Walnut = FsSurface(0x5c3821, 0.6f, 0.0f, D::FsPatternWood);
	// A box from its middle and its size along the Blender axes (meters): the engine's cube is 100 cm a side, its pivot at its middle.
	auto Box = [this, Walnut](double Cx, double Cy, double Cz, double Wx, double Dy, double Hz) {
		FsMesh(CubeMesh, Walnut, FestShelfDetail::FsAxes(Cx, Cy, Cz), FRotator::ZeroRotator, FVector(Dy, Wx, Hz), ShelfRoot);
	};
	const double H = D::FsTopZ + D::FsTopT;
	Box(-(D::FsShelfW - D::FsSideT) / 2.0, 0.0, H / 2.0, D::FsSideT, D::FsShelfD, H);
	Box((D::FsShelfW - D::FsSideT) / 2.0, 0.0, H / 2.0, D::FsSideT, D::FsShelfD, H);
	Box(0.0, -D::FsBackT / 2.0, D::FsTierZ[0] - D::FsBoardT / 2.0, D::FsInnerW, D::FsShelfD - D::FsBackT, D::FsBoardT);
	Box(0.0, -D::FsBackT / 2.0, D::FsTierZ[1] - D::FsBoardT / 2.0, D::FsInnerW, D::FsShelfD - D::FsBackT, D::FsBoardT);
	Box(0.0, 0.0, D::FsTopZ + D::FsTopT / 2.0, D::FsShelfW, D::FsShelfD, D::FsTopT);
	Box(0.0, (D::FsShelfD - D::FsBackT) / 2.0, H / 2.0, D::FsInnerW, D::FsBackT, H);
}

void UFestShelf::BuildBoardShapes()
{
	namespace D = FestShelfDetail;
	UMaterialInterface* Walnut = FsSurface(0x5c3821, 0.6f, 0.0f, D::FsPatternWood);
	UMaterialInterface* Cork = FsSurface(0xb08a5a, 0.95f, 0.0f, D::FsPatternNone);
	auto Box = [this](UMaterialInterface* M, double Cx, double Cy, double Cz, double Wx, double Dy, double Hz) {
		FsMesh(CubeMesh, M, FestShelfDetail::FsAxes(Cx, Cy, Cz), FRotator::ZeroRotator, FVector(Dy, Wx, Hz), BoardRoot);
	};
	// The cork to its face (8 mm below the frame's), then the frame's four bars.
	Box(Cork, 0.0, D::FsCorkY / 2.0, 0.0, D::FsBoardW - 2.0 * D::FsFrameW, -D::FsCorkY, D::FsBoardH - 2.0 * D::FsFrameW);
	Box(Walnut, 0.0, -D::FsBoardD / 2.0, (D::FsBoardH - D::FsFrameW) / 2.0, D::FsBoardW, D::FsBoardD, D::FsFrameW);
	Box(Walnut, 0.0, -D::FsBoardD / 2.0, -(D::FsBoardH - D::FsFrameW) / 2.0, D::FsBoardW, D::FsBoardD, D::FsFrameW);
	Box(Walnut, -(D::FsBoardW - D::FsFrameW) / 2.0, -D::FsBoardD / 2.0, 0.0, D::FsFrameW, D::FsBoardD, D::FsBoardH - 2.0 * D::FsFrameW);
	Box(Walnut, (D::FsBoardW - D::FsFrameW) / 2.0, -D::FsBoardD / 2.0, 0.0, D::FsFrameW, D::FsBoardD, D::FsBoardH - 2.0 * D::FsFrameW);
}

void UFestShelf::BeginPlay()
{
	Super::BeginPlay();
	AttachPlateSlate();
}

void UFestShelf::AttachPlateSlate()
{
	namespace D = FestShelfDetail;
	PlateSlates.Reset();
	bPlatesDrawn = false;
	if (!FSlateTextMeasurer::IsAvailable())
	{
		return;
	}
	PlateSlates.SetNum(PlateStrips.Num());
	for (int32 I = 0; I < PlateStrips.Num(); ++I)
	{
		if (UWidgetComponent* W = PlateStrips[I])
		{
			PlateSlates[I] = SNew(SDrawListWidget).DesiredSize(FVector2D(D::FsStripW, D::FsStripH));
			W->SetSlateWidget(PlateSlates[I]);
			W->SetManuallyRedraw(true);
		}
	}
}

void UFestShelf::DrawPlates(const FestShelfDetail::FFsPlan& Plan, FNightOneGame& Game)
{
	namespace D = FestShelfDetail;
	if (PlateSlates.Num() != PlateStrips.Num())
	{
		return;
	}
	const std::string Template = D::FsPlateTemplate();
	for (int32 Tier = 0; Tier < PlateStrips.Num() && Tier < 2; ++Tier)
	{
		if (!PlateStrips[Tier] || !PlateSlates[Tier])
		{
			continue;
		}
		TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
		ss::ui::Canvas C(*List, Game.Measurer, D::FsStripW, D::FsStripH, static_cast<float>(D::FsStripPxW) / D::FsStripW);
		for (int32 I = 0; I < 5; ++I)
		{
			const std::string Line = D::FsPlateLine(Plan, D::FsBrandAt(Tier * 5 + I), Template);
			if (Line.empty())
			{
				continue;
			}
			// Engraved in the plate's brass, dark; a long name shrinks to keep inside its plate.
			ss::ui::TextStyle St;
			St.Size = D::FsEngraveSize;
			const float Wide = Game.Measurer.Width(Line, ss::ui::Font::Bold, St.Size);
			St.Size = Wide > D::FsEngraveFit ? St.Size * D::FsEngraveFit / Wide : St.Size;
			St.Weight = 700;
			St.Col = ss::ui::Hex(0x2a1c0a, 0.9f);
			St.HAlign = ss::ui::Align::Center;
			St.VAlign = ss::ui::Baseline::Middle;
			C.Text(Line, static_cast<float>(D::FsSlotPitch * (I + 0.5) * 1000.0), D::FsStripH * 0.5f, St);
		}
		PlateSlates[Tier]->SetDrawList(List);
		PlateStrips[Tier]->RequestRedraw();
	}
	bPlatesDrawn = true;
}

UInstancedStaticMeshComponent* UFestShelf::PinGroup(const FString& Key, bool bEnamel, uint32 SrgbHex, bool bMetal)
{
	if (TObjectPtr<UInstancedStaticMeshComponent>* Found = PinGroups.Find(Key))
	{
		return Found->Get();
	}
	if (!BoardRoot)
	{
		return nullptr;
	}
	UInstancedStaticMeshComponent* G = nullptr;
	if (!bEnamel && PinMesh)
	{
		// The kit's pin, its baked neutral metal multiplied by the rim's colour (its enamel field lies under the enamel disc).
		G = FsInstanced(PinMesh, nullptr, BoardRoot, false);
		TintImported(G, 0, FestShelfDetail::FsLinear(SrgbHex));
	}
	else
	{
		G = FsInstanced(CylinderMesh, FsSurface(SrgbHex, bMetal ? 0.25f : 0.4f, bMetal ? 1.0f : 0.0f), BoardRoot, false);
	}
	PinGroups.Add(Key, G);
	return G;
}

void UFestShelf::ShowPins(const FestShelfDetail::FFsPlan& Plan)
{
	namespace D = FestShelfDetail;
	for (TPair<FString, TObjectPtr<UInstancedStaticMeshComponent>>& Kv : PinGroups)
	{
		if (Kv.Value)
		{
			Kv.Value->ClearInstances();
		}
	}
	const FRotator Front(0.0f, BlenderImportYaw(), 0.0f);
	const FRotator Out(90.0f, 0.0f, 0.0f); // the engine cylinder's axis out of the cork (the root's -X: the pins' faces toward the room)
	for (const D::FFsPin& Pin : Plan.Pins)
	{
		const ss::ui::festart::FestivalLook& L = ss::ui::festart::LookOf(Pin.Brand);
		const FVector At = D::FsAxes(D::FsPinX0 + D::FsPinDx * Pin.Col, D::FsCorkY, D::FsPinZ0 - D::FsPinDz * Pin.Row);
		// The rim: gold (won an event), silver (cashed), or an outline in the look's Accent2 (played): FestPinArt's.
		const bool bMetal = Pin.Grade >= 2;
		const uint32 Rim = Pin.Grade >= 3 ? 0xe8b64a : Pin.Grade == 2 ? 0xc9d3e6 : static_cast<uint32>(L.Accent2);
		if (UInstancedStaticMeshComponent* G = PinGroup(FString::Printf(TEXT("rim/%06x"), Rim), false, Rim, bMetal))
		{
			G->AddInstance(PinMesh ? FTransform(Front, At) : FTransform(Out, At + D::FsAxes(0.0, -0.0013, 0.0), FVector(2.0 * D::FsPinR, 2.0 * D::FsPinR, 0.0026)));
		}
		// The enamel field in the look's Accent (dulled by half on a played pin), 2.6-3.0 mm out: inside the 3.4 mm rim.
		const uint32 Enamel = bMetal ? static_cast<uint32>(L.Accent) : D::FsSink(L.Accent);
		if (UInstancedStaticMeshComponent* G = PinGroup(FString::Printf(TEXT("enamel/%06x"), Enamel), true, Enamel, false))
		{
			G->AddInstance(FTransform(Out, At + D::FsAxes(0.0, -0.0028, 0.0), FVector(2.0 * D::FsPinFieldR, 2.0 * D::FsPinFieldR, 0.0004)));
		}
	}
}

void UFestShelf::Apply(const FestShelfDetail::FFsPlan& Plan, bool bPacked)
{
	namespace D = FestShelfDetail;
	// Packed into the cartons (evicted): nothing on the walls. Otherwise everything visible, then each trophy and belt by the plan.
	if (ShelfRoot)
	{
		ShelfRoot->SetVisibility(!bPacked, true);
	}
	if (BoardRoot)
	{
		BoardRoot->SetVisibility(!bPacked, true);
	}
	if (bPacked)
	{
		UE_LOG(LogTemp, Log, TEXT("The festival shelf: packed with the room"));
		return;
	}
	int32 TrophiesShown = 0;
	for (int32 B = 1; B < Trophies.Num() && B < D::FsBrands; ++B)
	{
		if (USceneComponent* Group = Trophies[B])
		{
			const bool bWon = !Plan.Won[B].empty();
			Group->SetVisibility(bWon, true);
			TrophiesShown += bWon ? 1 : 0;
		}
	}
	for (int32 I = 0; I < Belts.Num(); ++I)
	{
		UStaticMeshComponent* C = Belts[I];
		if (!C)
		{
			continue;
		}
		const bool bOn = I < static_cast<int32>(Plan.Belts.size());
		C->SetVisibility(bOn);
		if (bOn)
		{
			const uint32 Strap = D::FsBeltStrap(Plan.Belts[I]);
			if (C->GetStaticMesh() == CubeMesh)
			{
				C->SetMaterial(0, FsSurface(Strap, 0.6f, 0.0f));
			}
			else
			{
				TintImported(C, 0, D::FsLinear(Strap));
			}
		}
	}
	ShowPins(Plan);
	// Printed, never assumed (§9.4): the lead's PIE reads the real draw calls (stat scenerendering) against 12.
	UE_LOG(LogTemp, Log, TEXT("The festival shelf: %d trophies, %d belts, %d pins in %d pin groups"), TrophiesShown, static_cast<int32>(Plan.Belts.size()),
		static_cast<int32>(Plan.Pins.size()), PinGroups.Num());
}

void UFestShelf::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UWorld* Here = GetWorld();
	if (!Here || !Here->IsGameWorld())
	{
		return;
	}
	const ANightOneGameMode* Mode = Here->GetAuthGameMode<ANightOneGameMode>();
	FNightOneGame* Game = Mode ? Mode->GetGame() : nullptr;
	if (!Game)
	{
		return;
	}
	// The career's living world on the game thread (never a save snapshot: PLAN_G1's PC-9 keeps one snapshot reader at G1).
	const ANightOneStage* Stage = Cast<ANightOneStage>(GetOwner());
	const bool bPacked = Stage && Stage->GearShown().bEvicted;
	const ss::world::World& W = Game->Session.Living();
	const FestShelfDetail::FFsPlan Plan = bPacked ? FestShelfDetail::FFsPlan() : FestShelfDetail::FsPlanOf(W.Honors(), ss::world::HeroFestPins(W));
	std::string Key = FestShelfDetail::FsKey(Plan, bPacked);
	if (Key != Shown)
	{
		Shown = std::move(Key);
		Apply(Plan, bPacked);
		bPlatesDrawn = false;
	}
	if (!bPacked && !bPlatesDrawn)
	{
		DrawPlates(Plan, *Game);
	}
}
