// Fifth's ground (world realism spec 4.1, 3.5, ue-world): Northside's modelled ground where ns_ground has been imported
// (SM_NS_Ground_FifthS with the Hollis T, SM_NS_Ground_FifthN with the corner's mouth and the Wexler T, SM_NS_Ground_Far
// out to the haze), shaded by M_NS_Ground (its MI_NS_Ground_<Slot>s on the meshes themselves, ns_setup.py: the puddles
// fill from their vertex color by MPC_NS.Puddle) and walked on its own triangles; else engine-shape stand-ins in the same
// plan. Then the castings set into it (manholes on the crown, curb inlets, vault lights) and the stage's small helpers
// for SITE and PROP meshes, blockers and decals. Market's and Cardinal's ground are AMarketStage's.
#include "StreetStage.h"

#include "Components/DecalComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "PhysicsEngine/BodySetup.h"

namespace StreetGroundDetail
{
// M_Street's patterns (street_setup.py), for the stand-ins.
const float PatAsphalt = 2.0f;
const float PatSlabs = 3.0f;
const float PatConcrete = 5.0f;

const TCHAR* const GroundMeshes[] = {TEXT("SM_NS_Ground_FifthS"), TEXT("SM_NS_Ground_FifthN"), TEXT("SM_NS_Ground_Far")};
const TCHAR* const CastingMeshes[] = {TEXT("SM_NS_Manhole"), TEXT("SM_NS_ManholeVent"), TEXT("SM_NS_CurbInlet"), TEXT("SM_NS_VaultLights")};

/** How far the stand-ins run the cross streets: as far as the modelled ground does (ns_ground.py's Far), into the rows of fronts
 *  that close each end. */
const double CrossEast = Ns::CrossEast;
const double CrossWest = Ns::CrossWest + 300.0;

/** One casting set into Fifth's ground (spec 4.1's recesses): what, where (stage cm), facing; on the road's crown or not. */
struct FCasting
{
	const TCHAR* Mesh;
	double X;
	double Y;
	float Yaw;
	bool bRoad;
};
const FCasting Castings[] = {
	// Manholes on Fifth's road (their tops on its crown).
	{TEXT("SM_NS_Manhole"), 780.0, -4800.0, 0.0f, true},
	{TEXT("SM_NS_Manhole"), 780.0, -900.0, 30.0f, true},
	{TEXT("SM_NS_Manhole"), 1260.0, 3300.0, 75.0f, true},
	{TEXT("SM_NS_Manhole"), 1100.0, 8300.0, 140.0f, true},
	// Curb inlets, each its gutter to the road: the home curb, the far curb, one at each T.
	{TEXT("SM_NS_CurbInlet"), 420.0, -5900.0, 0.0f, false},
	{TEXT("SM_NS_CurbInlet"), 420.0, 2200.0, 0.0f, false},
	{TEXT("SM_NS_CurbInlet"), 1620.0, -1850.0, 180.0f, false},
	{TEXT("SM_NS_CurbInlet"), 1620.0, 4900.0, 180.0f, false},
	{TEXT("SM_NS_CurbInlet"), 1620.0, 8900.0, 180.0f, false},
	{TEXT("SM_NS_CurbInlet"), 3000.0, -6620.0, -90.0f, false},
	{TEXT("SM_NS_CurbInlet"), -1000.0, 9620.0, 90.0f, false},
	// The vault lights in front of QuikCash (its long side along the building line).
	{TEXT("SM_NS_VaultLights"), 150.0, 1200.0, 0.0f, false},
};
} // namespace StreetGroundDetail

using namespace StreetGroundDetail;

void AStreetStage::AppendGroundPreload(TArray<FString>& Paths)
{
	for (const TCHAR* Name : GroundMeshes)
	{
		Paths.Add(FStreetPreload::MeshPath(Name));
	}
	for (const TCHAR* Name : CastingMeshes)
	{
		Paths.Add(FStreetPreload::MeshPath(Name));
	}
	for (int32 Cell = 0; Cell < 16; ++Cell)
	{
		Paths.Add(Ns::DecalPath(Cell));
	}
}

// ------------------------------------------------------------------ the helpers the realism pass builds with

UStaticMeshComponent* AStreetStage::Site(const TCHAR* Name, bool bCollide)
{
	UStaticMesh* Mesh = NsMesh(Name);
	if (!Mesh)
	{
		return nullptr;
	}
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>();
	C->SetStaticMesh(Mesh);
	C->SetRelativeLocationAndRotation(FVector::ZeroVector, Ns::SiteRotation());
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

UStaticMeshComponent* AStreetStage::NsProp(const TCHAR* Name, const FVector& At, float Yaw, const FVector& Scale)
{
	UStaticMesh* Mesh = NsMesh(Name);
	if (!Mesh)
	{
		return nullptr;
	}
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>();
	C->SetStaticMesh(Mesh);
	C->SetRelativeLocationAndRotation(At, Ns::PropFacing(Yaw));
	C->SetRelativeScale3D(Scale);
	// The NS kit's imported collision is off (ns_setup.py): the stage blocks what should with boxes (Blocker).
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	return C;
}

void AStreetStage::Blocker(const FVector& Min, const FVector& Max)
{
	Box(nullptr, Min, Max)->SetVisibility(false);
}

UMaterialInterface* AStreetStage::DecalMaterial(int32 Cell)
{
	// The cell's instance, only once it samples the real atlas: before T_NS_Decals is imported ns_setup gives it the
	// engine's default texture, and a decal of that is a grey square on the street.
	UMaterialInterface* M = Cast<UMaterialInterface>(NsAsset(*Ns::DecalPath(FMath::Clamp(Cell, 0, 15))));
	UTexture* Atlas = nullptr;
	if (!M || !M->GetTextureParameterValue(FHashedMaterialParameterInfo(FName(TEXT("DecalBase"))), Atlas) || !Atlas || !Atlas->GetName().StartsWith(TEXT("T_NS_Decals")))
	{
		return nullptr;
	}
	return M;
}

UDecalComponent* AStreetStage::GroundDecal(int32 Cell, const FVector& At, float Yaw, const FVector2D& Size)
{
	UMaterialInterface* M = DecalMaterial(Cell);
	if (!M)
	{
		return nullptr;
	}
	UDecalComponent* D = NewPart<UDecalComponent>();
	D->SetDecalMaterial(M);
	// Projected straight down (its X along the projection), a few centimeters through the ground; gone by a tiny size on screen.
	D->DecalSize = FVector(12.0, Size.X * 0.5, Size.Y * 0.5);
	D->SetRelativeLocationAndRotation(At + FVector(0.0, 0.0, 2.0), FRotator(-90.0f, Yaw, 0.0f));
	D->SetFadeScreenSize(0.004f);
	return D;
}

// ------------------------------------------------------------------ the ground

void AStreetStage::BuildGround()
{
	// A ground mesh walks on its own triangles once ns_setup.py has made them its collision (complex as simple); until then
	// it draws, and invisible stand-in boxes in the same plan are what's walked on.
	auto Place = [this](const TCHAR* Name, bool& bCollides) -> bool {
		UStaticMeshComponent* C = Site(Name, false);
		if (!C)
		{
			return false;
		}
		const UStaticMesh* Mesh = C->GetStaticMesh();
		const UBodySetup* Body = Mesh ? Mesh->GetBodySetup() : nullptr;
		bCollides = Body && Body->CollisionTraceFlag == CTF_UseComplexAsSimple;
		if (bCollides)
		{
			C->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		}
		// Low and wide: no use in the distance fields' cards, and the puddles want the full detail (Nanite).
		C->bAffectDistanceFieldLighting = false;
		return true;
	};
	bool bSouthCollides = false;
	bool bNorthCollides = false;
	bool bFarCollides = false;
	const bool bSouth = Place(TEXT("SM_NS_Ground_FifthS"), bSouthCollides);
	// The north half is laid for Market at its new width; today's Market keeps today's corner (BuildEnds) and the stand-in.
	const bool bNorth = bWideMarket && Place(TEXT("SM_NS_Ground_FifthN"), bNorthCollides);
	Place(TEXT("SM_NS_Ground_Far"), bFarCollides);
	BuildGroundStandIn(!bSouth, !bNorth, true);
	if ((bSouth && !bSouthCollides) || (bNorth && !bNorthCollides))
	{
		BuildGroundStandIn(bSouth && !bSouthCollides, bNorth && !bNorthCollides, false);
	}

	// The castings in their recesses: the manholes on the road's crown, the inlets in the curb's line, the vault lights flush.
	UMaterialInterface* Iron = Mat(TEXT("Grate"), 0x1a1b1d, 0.35f, 0.0f, 0.0f, 0.8f);
	for (const FCasting& C : Castings)
	{
		const double Z = C.bRoad ? Ns::RoadZ(FMath::Abs(C.X - Ns::FifthMidX), Ns::RoadX1 - Ns::RoadX0) - 0.2 : 0.0;
		if (NsProp(C.Mesh, FVector(C.X, C.Y, Z), C.Yaw))
		{
			continue;
		}
		// Stand-ins: an iron disc, a grate in the gutter, a steel panel.
		if (C.bRoad)
		{
			Cyl(Iron, FVector(C.X, C.Y, Z - 0.4), 46.0f, 0.6f, false)->SetCastShadow(false);
		}
		else if (FCString::Strstr(C.Mesh, TEXT("Inlet")))
		{
			const FVector Out = FRotator(0.0f, C.Yaw, 0.0f).Vector();
			const FVector Mid = FVector(C.X, C.Y, -15.0) + Out * 25.0;
			const FVector Half = FVector(FMath::Abs(Out.X) * 22.0 + FMath::Abs(Out.Y) * 45.0, FMath::Abs(Out.Y) * 22.0 + FMath::Abs(Out.X) * 45.0, 0.2);
			Box(Iron, Mid - Half, Mid + Half, false)->SetCastShadow(false);
		}
		else
		{
			Box(Iron, FVector(C.X - 60.0, C.Y - 120.0, -0.4), FVector(C.X + 60.0, C.Y + 120.0, 0.2), false)->SetCastShadow(false);
		}
	}
}

void AStreetStage::BuildGroundStandIn(bool bSouth, bool bNorth, bool bVisible)
{
	if (!bSouth && !bNorth)
	{
		return;
	}
	UMaterialInterface* Walk = bVisible ? Mat(TEXT("Sidewalk"), 0x6f6c66, 0.75f, PatSlabs) : nullptr;
	UMaterialInterface* Road = bVisible ? Mat(TEXT("Asphalt"), 0x26272a, 0.6f, PatAsphalt) : nullptr;
	UMaterialInterface* Curb = bVisible ? Mat(TEXT("Curb"), 0x8a867e, 0.7f, PatConcrete) : nullptr;
	auto Piece = [&](UMaterialInterface* M, double X0, double Y0, double X1, double Y1, double Z0, double Z1) {
		UStaticMeshComponent* C = Box(M, FVector(FMath::Min(X0, X1), FMath::Min(Y0, Y1), Z0), FVector(FMath::Max(X0, X1), FMath::Max(Y0, Y1), Z1));
		if (!bVisible)
		{
			C->SetVisibility(false);
		}
	};
	// Sidewalks' tops at 0, the road's at -15, a 20 cm curb between: nothing shares a top face (a shared top would flicker).
	auto Sidewalk = [&](double X0, double Y0, double X1, double Y1) { Piece(Walk, X0, Y0, X1, Y1, -30.0, 0.0); };
	auto CurbRun = [&](double X0, double Y0, double X1, double Y1) { Piece(Curb, X0, Y0, X1, Y1, -30.0, 0.0); };
	auto Roadway = [&](double X0, double Y0, double X1, double Y1) { Piece(Road, X0, Y0, X1, Y1, -45.0, -15.0); };
	const double Wx0 = CrossEast;
	const double Wx1 = CrossWest;
	const double Split = 1500.0;

	UInstancedStaticMeshComponent* White = nullptr;
	UInstancedStaticMeshComponent* Yellow = nullptr;
	if (bVisible)
	{
		auto PaintSet = [this](UMaterialInterface* M) {
			UInstancedStaticMeshComponent* Set = NewPart<UInstancedStaticMeshComponent>();
			Set->SetStaticMesh(CubeMesh);
			Set->SetMaterial(0, M);
			Set->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Set->SetCastShadow(false);
			return Set;
		};
		// Worn thermoplastic, not glowing tubes (spec 3.5): rough dry, glossier wet (M_Street's Wet).
		White = PaintSet(Mat(TEXT("RoadPaint"), 0xd8d4c8, 0.55f));
		Yellow = PaintSet(Mat(TEXT("RoadYellow"), 0xd9a520, 0.55f));
	}
	auto Bar = [](UInstancedStaticMeshComponent* Set, double X0, double Y0, double X1, double Y1) {
		if (Set)
		{
			Set->AddInstance(FTransform(FRotator::ZeroRotator, FVector((X0 + X1) * 0.5, (Y0 + Y1) * 0.5, -14.8), FVector(FMath::Abs(X1 - X0) / 100.0, FMath::Abs(Y1 - Y0) / 100.0, 0.004)));
		}
	};
	// A ladder crosswalk across a road running along Y (bars along Y every 1.2 m across X) or along X.
	auto LadderAcrossY = [&](double X0, double X1, double Y0, double Y1) {
		for (double X = X0 + 30.0; X + 60.0 <= X1 - 20.0; X += 120.0)
		{
			Bar(White, X, Y0 + 20.0, X + 60.0, Y1 - 20.0);
		}
		Bar(White, X0, Y0, X1, Y0 + 20.0);
		Bar(White, X0, Y1 - 20.0, X1, Y1);
	};
	auto LadderAcrossX = [&](double X0, double X1, double Y0, double Y1) {
		for (double Y = Y0 + 30.0; Y + 60.0 <= Y1 - 20.0; Y += 120.0)
		{
			Bar(White, X0 + 20.0, Y, X1 - 20.0, Y + 60.0);
		}
		Bar(White, X0, Y0, X0 + 20.0, Y1);
		Bar(White, X1 - 20.0, Y0, X1, Y1);
	};
	auto DoubleYellowY = [&](double Y0, double Y1) {
		Bar(Yellow, 1008.0, Y0, 1020.0, Y1);
		Bar(Yellow, 1030.0, Y0, 1042.0, Y1);
	};
	auto DoubleYellowX = [&](double Mid, double X0, double X1) {
		Bar(Yellow, X0, Mid - 15.0, X1, Mid - 5.0);
		Bar(Yellow, X0, Mid + 5.0, X1, Mid + 15.0);
	};

	if (bSouth)
	{
		// Hollis's T: its north sidewalks either side of Fifth's mouth, their curb, the road, its south curb and sidewalk.
		Sidewalk(Wx0, Ns::HollisRoad0 + 20.0, Ns::RoadX0 - 20.0, Ns::HollisN);
		Sidewalk(Ns::RoadX1 + 20.0, Ns::HollisRoad0 + 20.0, Wx1, Ns::HollisN);
		CurbRun(Wx0, Ns::HollisRoad0, Ns::RoadX0, Ns::HollisRoad0 + 20.0);
		CurbRun(Ns::RoadX1, Ns::HollisRoad0, Wx1, Ns::HollisRoad0 + 20.0);
		Roadway(Wx0, Ns::HollisRoad1, Wx1, Ns::HollisRoad0);
		CurbRun(Wx0, Ns::HollisRoad1 - 20.0, Wx1, Ns::HollisRoad1);
		Sidewalk(Wx0, Ns::HollisS, Wx1, Ns::HollisRoad1 - 20.0);
		// Fifth up to the halves' seam.
		Sidewalk(0.0, Ns::HollisN, Ns::RoadX0 - 20.0, Split);
		CurbRun(Ns::RoadX0 - 20.0, Ns::HollisRoad0 + 20.0, Ns::RoadX0, Split);
		Roadway(Ns::RoadX0, Ns::HollisRoad0, Ns::RoadX1, Split);
		CurbRun(Ns::RoadX1, Ns::HollisRoad0 + 20.0, Ns::RoadX1 + 20.0, Split);
		Sidewalk(Ns::RoadX1 + 20.0, Ns::HollisN, Ns::FarFront, Split);
		DoubleYellowY(-6120.0, Split);
		Bar(White, Ns::RoadX0 + 20.0, -6150.0, Ns::FifthMidX, -6120.0);
		LadderAcrossY(Ns::RoadX0, Ns::RoadX1, -6580.0, -6240.0);
		DoubleYellowX((Ns::HollisRoad0 + Ns::HollisRoad1) * 0.5, Wx0, Wx1);
	}
	if (bNorth)
	{
		if (bWideMarket)
		{
			// The home side to the corner, Market's mouth (its road meets Fifth's), the home side on north of it.
			Sidewalk(0.0, Split, Ns::RoadX0 - 20.0, Ns::MarketS - 20.0);
			CurbRun(Ns::RoadX0 - 20.0, Split, Ns::RoadX0, Ns::MarketS - 20.0);
			CurbRun(0.0, Ns::MarketS - 20.0, Ns::RoadX0, Ns::MarketS);
			Roadway(0.0, Ns::MarketS, Ns::RoadX0, Ns::MarketN);
			CurbRun(0.0, Ns::MarketN, Ns::RoadX0, Ns::MarketN + 20.0);
			Sidewalk(0.0, Ns::MarketN + 20.0, Ns::RoadX0 - 20.0, Ns::WexlerS);
			CurbRun(Ns::RoadX0 - 20.0, Ns::MarketN + 20.0, Ns::RoadX0, Ns::WexlerRoad0 - 20.0);
			LadderAcrossY(Ns::RoadX0, Ns::RoadX1, 5000.0, 5380.0);
			LadderAcrossY(Ns::RoadX0, Ns::RoadX1, 6520.0, 6900.0);
			LadderAcrossX(40.0, 380.0, Ns::MarketS, Ns::MarketN);
			DoubleYellowY(Split, 4880.0);
			DoubleYellowY(7020.0, 9500.0);
		}
		else
		{
			// Today's corner: Market's ground off it is BuildEnds'.
			Sidewalk(0.0, Split, Ns::RoadX0 - 20.0, Ns::StoreY1);
			CurbRun(Ns::RoadX0 - 20.0, Split, Ns::RoadX0, Ns::OldMarketS - 20.0);
			Sidewalk(0.0, Ns::OldMarketLineN, Ns::RoadX0 - 20.0, Ns::WexlerS);
			CurbRun(Ns::RoadX0 - 20.0, Ns::OldMarketN + 20.0, Ns::RoadX0, Ns::WexlerRoad0 - 20.0);
			for (double X = Ns::RoadX0 + 40.0; X + 50.0 < Ns::RoadX1 - 30.0; X += 100.0)
			{
				Bar(White, X, Ns::StoreY1 + 40.0, X + 50.0, Ns::OldMarketS - 40.0);
				Bar(White, X, Ns::OldMarketN + 40.0, X + 50.0, Ns::OldMarketLineN - 20.0);
			}
			DoubleYellowY(Split, Ns::StoreY1 - 200.0);
			DoubleYellowY(Ns::OldMarketLineN + 200.0, 9500.0);
		}
		Sidewalk(Wx0, Ns::WexlerS, Ns::RoadX0 - 20.0, Ns::WexlerRoad0 - 20.0);
		CurbRun(Wx0, Ns::WexlerRoad0 - 20.0, Ns::RoadX0, Ns::WexlerRoad0);
		Roadway(Ns::RoadX0, Split, Ns::RoadX1, Ns::WexlerRoad0);
		CurbRun(Ns::RoadX1, Split, Ns::RoadX1 + 20.0, Ns::WexlerRoad0 - 20.0);
		Sidewalk(Ns::RoadX1 + 20.0, Split, Ns::FarFront, Ns::WexlerS);
		Sidewalk(Ns::RoadX1 + 20.0, Ns::WexlerS, Wx1, Ns::WexlerRoad0 - 20.0);
		CurbRun(Ns::RoadX1, Ns::WexlerRoad0 - 20.0, Wx1, Ns::WexlerRoad0);
		Roadway(Wx0, Ns::WexlerRoad0, Wx1, Ns::WexlerRoad1);
		CurbRun(Wx0, Ns::WexlerRoad1, Wx1, Ns::WexlerRoad1 + 20.0);
		Sidewalk(Wx0, Ns::WexlerRoad1 + 20.0, Wx1, Ns::WexlerN);
		Bar(White, Ns::FifthMidX, 9220.0, Ns::RoadX1 - 20.0, 9250.0);
		LadderAcrossY(Ns::RoadX0, Ns::RoadX1, 9240.0, 9580.0);
		DoubleYellowX((Ns::WexlerRoad0 + Ns::WexlerRoad1) * 0.5, Wx0, Wx1);
	}
}
