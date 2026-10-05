// The Embercrest's world: every SM_ECX_ mesh where the design spec puts it, and a block-out of engine shapes for any that
// isn't imported yet (spec 2, 3, 6). Collision is the stage's own (spec 2.8): the ground's complex collision or its
// block-out, ramps over the flights, invisible boxes round the buildings and props, pawn-only walls on the open edges.
#include "EmbercrestStage.h"

#include "EmbercrestShared.h"

#include "BlenderProps.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "PhysicsEngine/BodySetup.h"

namespace EcxBuildDetail
{
/** A small deterministic random sequence (the scatter, the windows). */
struct FDice
{
	int64 S = 20261004;
	explicit FDice(int64 Seed) : S(FMath::Max<int64>(1, Seed % 2147483646)) {}
	double Next()
	{
		S = (S * 16807) % 2147483647;
		return static_cast<double>(S) / 2147483647.0;
	}
	double Range(double Lo, double Hi) { return Lo + (Hi - Lo) * Next(); }
};

/** Whether an imported mesh's collision is its own triangles (the setup marks the ground and the terrain so). */
bool CollidesComplex(const UStaticMesh* M)
{
	const UBodySetup* Body = M ? M->GetBodySetup() : nullptr;
	return Body && Body->CollisionTraceFlag == CTF_UseComplexAsSimple;
}

/** The crown's skyline along Y (spec 2.4): three peaks in the mark's proportions over the tower's roof at 6200. */
double CrownTop(double Y)
{
	static const FVector2D Profile[7] = {FVector2D(1000.0, 6200.0), FVector2D(1700.0, 7270.0), FVector2D(2400.0, 6760.0), FVector2D(3100.0, 7800.0), FVector2D(3800.0, 6700.0),
		FVector2D(4500.0, 7110.0), FVector2D(5200.0, 6200.0)};
	for (int32 I = 0; I < 6; ++I)
	{
		if (Y <= Profile[I + 1].X)
		{
			const double T = (Y - Profile[I].X) / (Profile[I + 1].X - Profile[I].X);
			return FMath::Lerp(Profile[I].Y, Profile[I + 1].Y, FMath::Clamp(T, 0.0, 1.0));
		}
	}
	return 6200.0;
}
} // namespace EcxBuildDetail

using namespace EcxBuildDetail;
using namespace Ecx;

// ------------------------------------------------------------------ small parts

UStaticMeshComponent* AEmbercrestStage::Bar(UMaterialInterface* Material, const FVector& A, const FVector& B, float Thick, bool bCollide, USceneComponent* Parent)
{
	const FVector D = B - A;
	UStaticMeshComponent* C = NewPart<UStaticMeshComponent>(Parent);
	C->SetStaticMesh(CubeMesh);
	C->SetRelativeLocationAndRotation((A + B) * 0.5, FRotationMatrix::MakeFromX(D.GetSafeNormal()).Rotator());
	C->SetRelativeScale3D(FVector(D.Size() / 100.0, Thick / 100.0, Thick / 100.0));
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

UInstancedStaticMeshComponent* AEmbercrestStage::Instances(UStaticMesh* InMesh, UMaterialInterface* Material, bool bShadows)
{
	UInstancedStaticMeshComponent* C = NewPart<UInstancedStaticMeshComponent>();
	C->SetStaticMesh(InMesh);
	if (Material)
	{
		for (int32 Slot = 0; Slot < FMath::Max(1, C->GetNumMaterials()); ++Slot)
		{
			C->SetMaterial(Slot, Material);
		}
	}
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(bShadows);
	return C;
}

double AEmbercrestStage::FallbackTerrainZ(double X, double Y)
{
	if (X >= XParapet + 45.0)
	{
		// The drop beyond the parapet: 37 degrees to -180 m by 300 m out, then the forest down to the valley floor.
		if (X < 30000.0)
		{
			return FMath::Lerp(-250.0, -18000.0, (X - (XParapet + 45.0)) / (30000.0 - (XParapet + 45.0)));
		}
		if (X < 250000.0)
		{
			return FMath::Lerp(-18000.0, -90000.0, (X - 30000.0) / 220000.0);
		}
		return -90000.0;
	}
	if (X <= -3000.0)
	{
		// The bench behind the resort, rising to +180 m about 700 m back.
		return FMath::Min(18000.0, (-3000.0 - X) * 18000.0 / 67000.0);
	}
	if (FMath::Abs(Y) < 3350.0 && X > 0.0)
	{
		return PlazaZ(X, Y);
	}
	return X < 2125.0 ? ZLane : ArmZ(X);
}

double AEmbercrestStage::GroundZ(double X, double Y) const
{
	const FTransform& T = GetActorTransform();
	const FVector From = T.TransformPosition(FVector(X, Y, 250000.0));
	const FVector To = T.TransformPosition(FVector(X, Y, -250000.0));
	FCollisionQueryParams Params(SCENE_QUERY_STAT(EcxGroundZ), true);
	double Best = -TNumericLimits<double>::Max();
	bool bHit = false;
	for (const UStaticMeshComponent* C : {GroundMesh.Get(), TerrainMesh.Get(), RidgeMesh.Get()})
	{
		FHitResult Hit;
		if (C && C->IsCollisionEnabled() && const_cast<UStaticMeshComponent*>(C)->LineTraceComponent(Hit, From, To, Params))
		{
			Best = FMath::Max(Best, T.InverseTransformPosition(Hit.ImpactPoint).Z);
			bHit = true;
		}
	}
	return bHit ? Best : FallbackTerrainZ(X, Y);
}

// ------------------------------------------------------------------ the ground (spec 2.2, 2.3)

void AEmbercrestStage::BuildGround()
{
	bool bGroundCollides = false;
	if (UStaticMeshComponent* G = Site(TEXT("SM_ECX_Ground")))
	{
		GroundMesh = G;
		// Unbaked, by name: each surface its own tiling set (UV0 is map meters), the snow beds the snow.
		static const TCHAR* const Names[8] = {TEXT("ecx_paver"), TEXT("ecx_sett"), TEXT("ecx_asphalt"), TEXT("ecx_curb"), TEXT("ecx_concrete"), TEXT("ecx_wallstone"),
			TEXT("ecx_snowground"), TEXT("ecx_marking")};
		for (int32 K = 0; K < 8; ++K)
		{
			SetSlot(G, SlotOf(G, -1, Names[K]), GroundMat(K, false));
		}
		// Its own triangles carry the hero (the setup marks it complex-as-simple); otherwise the block-out's boxes do.
		if (CollidesComplex(G->GetStaticMesh()))
		{
			G->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			G->bReturnMaterialOnMove = true;
			bGroundCollides = true;
		}
	}
	if (bGroundCollides)
	{
		return;
	}
	// The block-out (seen when the ground isn't imported; only its collision otherwise).
	const bool bShow = GroundMesh == nullptr;
	auto Slab = [this, bShow](UMaterialInterface* M, const FVector& Min, const FVector& Max) {
		UStaticMeshComponent* C = Box(M, Min, Max, true);
		if (!bShow)
		{
			C->SetVisibility(false);
			C->SetCastShadow(false);
		}
		return C;
	};
	auto Sloped = [this, bShow](UMaterialInterface* M, double X0, double X1, double Y0, double Y1, double Z0, double Z1) {
		UStaticMeshComponent* C = Slope(M, X0, X1, Y0, Y1, Z0, Z1, 80.0, true);
		if (!bShow)
		{
			C->SetVisibility(false);
			C->SetCastShadow(false);
		}
		return C;
	};
	UMaterialInterface* Paver = GroundMat(0, true);
	UMaterialInterface* Sett = GroundMat(1, true);
	UMaterialInterface* Asphalt = GroundMat(2, true);
	UMaterialInterface* Concrete = GroundMat(4, true);
	UMaterialInterface* Wall = GroundMat(5, true);
	UMaterialInterface* Snow = SnowMat();
	// Under the canopy: the apron to the glass, the lanes 12 cm down with the crosswalk table across them, the top landing.
	Slab(Paver, FVector(-300.0, -2200.0, -80.0), FVector(XApron1, 2200.0, ZApron));
	for (const double S : {-1.0, 1.0})
	{
		Slab(Sett, FVector(XApron1, S > 0 ? WalkHalf : -2100.0, -80.0), FVector(XLane1, S > 0 ? 2100.0 : -WalkHalf, ZLane));
		// The wings' sidewalks and their planter strips.
		Slab(Paver, FVector(500.0, S > 0 ? 2200.0 : -4200.0, -80.0), FVector(XApron1, S > 0 ? 4200.0 : -2200.0, ZApron));
		UStaticMeshComponent* Strip = Slab(Snow, FVector(300.0, S > 0 ? 2200.0 : -4200.0, -80.0), FVector(500.0, S > 0 ? 4200.0 : -2200.0, 15.0));
		Strip->ComponentTags.Add(TEXT("Snow"));
		// The drive's curves (as one block) and the arms down to the road.
		Slab(Sett, FVector(XApron1, S > 0 ? 2100.0 : -3350.0, -80.0), FVector(2125.0, S > 0 ? 3350.0 : -2100.0, ZLane));
		Sloped(Sett, 2125.0, 4400.0, S > 0 ? 2650.0 : -3350.0, S > 0 ? 3350.0 : -2650.0, ZLane, ArmZ(4400.0));
		Sloped(Asphalt, 4400.0, XBay1, S > 0 ? 2650.0 : -3350.0, S > 0 ? 3350.0 : -2650.0, ArmZ(4400.0), ZRoad);
		// The island's snow beds, stepped with the landings behind their low walls.
		for (const FVector& Bed : {FVector(XTop1, 3560.0, ZLandB + 20.0), FVector(3560.0, 4720.0, ZLandA + 20.0), FVector(4720.0, XFlight1, ZPlatform + 20.0)})
		{
			UStaticMeshComponent* B = Slab(Snow, FVector(Bed.X, S > 0 ? WalkHalf : -2650.0, -300.0), FVector(Bed.Y, S > 0 ? 2650.0 : -WalkHalf, Bed.Z));
			B->ComponentTags.Add(TEXT("Snow"));
		}
		for (const double Wx : {2400.0, 3560.0, 4720.0})
		{
			Slab(Wall, FVector(Wx - 20.0, S > 0 ? 1490.0 : -2650.0, -300.0), FVector(Wx + 20.0, S > 0 ? 2650.0 : -1490.0, PlazaZ(Wx - 40.0, 2000.0) + 30.0));
		}
		// Snow between the platform's ends and the arms.
		UStaticMeshComponent* End = Slab(Snow, FVector(XFlight1, S > 0 ? 2200.0 : -2650.0, -400.0), FVector(XPlatform1, S > 0 ? 2650.0 : -2200.0, ZPlatform + 20.0));
		End->ComponentTags.Add(TEXT("Snow"));
	}
	Slab(Paver, FVector(XApron1, -WalkHalf, -80.0), FVector(XLane1, WalkHalf, ZApron));
	Slab(Paver, FVector(XLane1, -2100.0, -80.0), FVector(XTop1, 2100.0, ZApron));
	// The landings on the walk (the flights between them are BuildSteps').
	Slab(Paver, FVector(XFlight3, -WalkHalf, -300.0), FVector(XFlight2 - 160.0, WalkHalf, ZLandB));
	Slab(Paver, FVector(XFlight2, -WalkHalf, -300.0), FVector(XFlight1 - 160.0, WalkHalf, ZLandA));
	// The bus platform, the bay (between the arms) and the ridge road, the shoulders, the overlook.
	Slab(Paver, FVector(XFlight1, -2200.0, -400.0), FVector(XPlatform1, 2200.0, ZPlatform));
	Slab(Asphalt, FVector(XPlatform1, -2650.0, -400.0), FVector(XBay1, 2650.0, ZRoad));
	Slab(Asphalt, FVector(XBay1, -4200.0, -400.0), FVector(XRoad1, 6000.0, ZRoad));
	Slab(Concrete, FVector(XRoad1, -4200.0, -400.0), FVector(XParapet + 45.0, 6000.0, ZRoad + 1.0));
	UStaticMeshComponent* South = SlopeY(Asphalt, XPlatform1, XParapet + 45.0, -20000.0, -4200.0, RoadZ(-20000.0), ZRoad, 120.0, true);
	UStaticMeshComponent* North = SlopeY(Asphalt, XPlatform1, XParapet + 45.0, 6000.0, 12000.0, ZRoad, RoadZ(12000.0), 120.0, true);
	if (!bShow)
	{
		South->SetVisibility(false);
		North->SetVisibility(false);
	}
	Slab(Paver, FVector(XParapet, 555.0, -400.0), FVector(XOverlook1 + 45.0, 2045.0, ZOverlook));
	if (bShow)
	{
		// The paint: the double yellow down the road's middle, the bay's edge line, the crosswalk to the overlook.
		UMaterialInterface* Yellow = GroundMat(7, true);
		UMaterialInstanceDynamic* White = Mat(TEXT("PaintWhite"), 0xe9e6dc, 0.35f);
		for (const double Line : {5940.0, 5956.0})
		{
			Box(Yellow, FVector(Line, -4200.0, ZRoad), FVector(Line + 10.0, 6000.0, ZRoad + 0.4), false)->SetCastShadow(false);
		}
		Box(White, FVector(XBay1 - 6.0, -1800.0, ZRoad), FVector(XBay1 + 6.0, 800.0, ZRoad + 0.4), false)->SetCastShadow(false);
		for (double X = XPlatform1 + 60.0; X < XParapet - 60.0; X += 100.0)
		{
			Box(White, FVector(X, 1200.0, ZRoad), FVector(X + 50.0, 1500.0, ZRoad + 0.4), false)->SetCastShadow(false);
		}
	}
}

// ------------------------------------------------------------------ the flights (spec 2.3, 3.2 P2)

void AEmbercrestStage::BuildSteps()
{
	struct FFlight
	{
		double X;  // the bottom riser
		double Z;  // its foot
	};
	const FFlight Flights[3] = {{XFlight1, ZPlatform}, {XFlight2, ZLandA}, {XFlight3, ZLandB}};
	for (const FFlight& F : Flights)
	{
		// A hidden ramp over every flight carries the hero (a smooth camera; the risers don't collide).
		UStaticMeshComponent* Ramp = Slope(nullptr, F.X - 160.0, F.X, -FlightHalf, FlightHalf, F.Z + 75.0, F.Z, 40.0, true);
		Ramp->SetVisibility(false);
		Ramp->SetCastShadow(false);
		Ramp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		const FVector At(F.X, 0.0, F.Z);
		if (UStaticMeshComponent* Steps = Prop(TEXT("SM_ECX_Steps"), At, 180.0f))
		{
			(void)Steps;
			GlowAll(Prop(TEXT("SM_ECX_Steps_GlowWarm"), At, 180.0f), GlowLantern);
			continue;
		}
		// The block-out: four treads of wet granite with bronze nosings, basalt cheeks, an LED line under each nosing.
		UMaterialInterface* Granite = Mat(TEXT("GraniteWet"), GraniteWetHex, 0.3f);
		UMaterialInterface* Bronze = Mat(TEXT("Bronze"), BronzeHex, 0.38f, 1.0f);
		UMaterialInterface* Basalt = Mat(TEXT("Basalt"), BasaltHex, 0.55f);
		UMaterialInterface* Led = GlowMat(GlowLantern);
		for (int32 K = 0; K < 4; ++K)
		{
			const double X0 = F.X - 40.0 * (K + 1);
			const double X1 = F.X - 40.0 * K;
			const double Top = F.Z + 15.0 * (K + 1);
			Box(Granite, FVector(X0, -360.0, F.Z - 30.0), FVector(X1, 360.0, Top), false);
			Box(Bronze, FVector(X1 - 4.0, -360.0, Top - 0.5), FVector(X1, 360.0, Top + 0.3), false)->SetCastShadow(false);
			Box(Led, FVector(X1 + 0.2, -355.0, Top - 4.0), FVector(X1 + 1.0, 355.0, Top - 2.0), false)->SetCastShadow(false);
		}
		for (const double S : {-1.0, 1.0})
		{
			Box(Basalt, FVector(F.X - 160.0, S > 0 ? 360.0 : -400.0, F.Z - 30.0), FVector(F.X, S > 0 ? 400.0 : -360.0, F.Z + 75.0), false);
		}
	}
}

// ------------------------------------------------------------------ the building (spec 2.4, 3.1)

void AEmbercrestStage::BuildBuilding()
{
	UMaterialInterface* Basalt = Mat(TEXT("Basalt"), BasaltHex, 0.55f);
	UMaterialInterface* Bronze = Mat(TEXT("Bronze"), BronzeHex, 0.38f, 1.0f);
	UMaterialInterface* Timber = Mat(TEXT("Timber"), TimberHex, 0.45f);
	UMaterialInterface* Granite = Mat(TEXT("Granite"), GraniteHex, 0.75f);
	UMaterialInterface* Ember = GlowMat(GlowEmber);
	UMaterialInterface* Interior = GlowMat(GlowInterior);

	// The entrance pavilion: basalt piers, the bronze mullion grid and cornice, larch fins over the canopy.
	if (!Site(TEXT("SM_ECX_Entry")))
	{
		for (const double S : {-1.0, 1.0})
		{
			Box(Basalt, FVector(-200.0, S > 0 ? 1600.0 : -1850.0, 0.0), FVector(300.0, S > 0 ? 1850.0 : -1600.0, 1280.0), false);
			Box(Ember, FVector(300.0, S > 0 ? 1600.0 : -1606.0, 0.0), FVector(303.0, S > 0 ? 1606.0 : -1600.0, 1280.0), false)->SetCastShadow(false);
		}
		Box(Bronze, FVector(-60.0, -1600.0, 1200.0), FVector(60.0, 1600.0, 1280.0), false);
		for (double Y = -1600.0; Y <= 1600.0; Y += 200.0)
		{
			if (FMath::Abs(Y) < 460.0 && FMath::Abs(Y) > 1.0)
			{
				continue; // the door bays
			}
			Box(Bronze, FVector(-6.0, Y - 4.0, 0.0), FVector(6.0, Y + 4.0, 1200.0), false);
		}
		for (const double Z : {300.0, 660.0, 980.0})
		{
			Box(Bronze, FVector(-6.0, -1600.0, Z - 4.0), FVector(6.0, 1600.0, Z + 4.0), false);
		}
		// The larch fins outside the clerestory, a meter apart.
		for (double Y = -1550.0; Y <= 1550.0; Y += 100.0)
		{
			Box(Timber, FVector(10.0, Y - 3.0, 720.0), FVector(55.0, Y + 3.0, 1240.0), false);
		}
		Box(Granite, FVector(-40.0, -1600.0, -1.0), FVector(40.0, 1600.0, 1.0), false)->SetCastShadow(false);
		// The vestibule's timber ceiling at 3 m.
		Box(Timber, FVector(-450.0, -1600.0, 300.0), FVector(-8.0, 1600.0, 312.0), false);
	}
	// The glazing: the entry's glass wall with the doors' openings, the clerestory, the grill's storefront.
	if (UStaticMeshComponent* Glass = Site(TEXT("SM_ECX_Podium_Glass")))
	{
		SetSlot(Glass, SlotOf(Glass, 0, TEXT("ecx_glass")), GlassMat(false));
		Glass->SetCastShadow(false);
	}
	else
	{
		UMaterialInterface* G = GlassMat(false);
		auto Pane = [this, G](double Y0, double Y1, double Z0, double Z1) { Box(G, FVector(-1.0, Y0, Z0), FVector(1.0, Y1, Z1), false)->SetCastShadow(false); };
		Pane(-1600.0, -460.0, 0.0, 1240.0);
		Pane(460.0, 1600.0, 0.0, 1240.0);
		Pane(-250.0, -175.0, 0.0, 1240.0);
		Pane(175.0, 250.0, 0.0, 1240.0);
		Pane(-460.0, -250.0, 300.0, 1240.0);
		Pane(250.0, 460.0, 300.0, 1240.0);
		Pane(-175.0, 175.0, 330.0, 1240.0);
		Box(G, FVector(299.0, 2000.0, 0.0), FVector(301.0, 3400.0, 550.0), false)->SetCastShadow(false);
	}
	// The lobby behind the glass: the black marble floor, the basalt feature wall with the crest's ember halo, the
	// timber ceiling, the mezzanine, the hotel desk and the casino portal; the chandelier of ember rods over it all.
	if (UStaticMeshComponent* Lobby = Site(TEXT("SM_ECX_Lobby")))
	{
		Lit(Lobby, {-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 0.6f});
	}
	else
	{
		UMaterialInterface* Marble = Mat(TEXT("Marble"), 0x0c0d10, 0.12f);
		Box(Marble, FVector(-1850.0, -1600.0, -20.0), FVector(-450.0, 1600.0, 0.0), false);
		Box(Basalt, FVector(-1880.0, -1600.0, 0.0), FVector(-1850.0, 1600.0, 1200.0), false);
		Box(Timber, FVector(-1850.0, -1600.0, 1190.0), FVector(-450.0, 1600.0, 1210.0), false);
		Box(Basalt, FVector(-1850.0, -1620.0, 0.0), FVector(-450.0, -1600.0, 1200.0), false);
		Box(Basalt, FVector(-1850.0, 1600.0, 0.0), FVector(-450.0, 1620.0, 1200.0), false);
		// The crest's halo on the feature wall, the desk and the portal's glow, the mezzanine's edge.
		Cyl(Interior, FVector(-1848.0, 0.0, 300.0), 300.0f, 2.0f)->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
		Box(Interior, FVector(-1400.0, 900.0, 0.0), FVector(-1300.0, 1400.0, 110.0), false);
		Box(Interior, FVector(-1700.0, -1599.0, 0.0), FVector(-1100.0, -1597.0, 500.0), false);
		Box(Bronze, FVector(-1850.0, -1600.0, 600.0), FVector(-1500.0, 1600.0, 620.0), false);
		// The chandelier: a cloud of ember-glass rods hanging over the medallion.
		FDice Dice(911);
		UInstancedStaticMeshComponent* Rods = Instances(CubeMesh, GlowMat(GlowInterior, 1.6f), false);
		for (int32 I = 0; I < 180; ++I)
		{
			const FVector At(Dice.Range(-1350.0, -850.0), Dice.Range(-400.0, 400.0), Dice.Range(750.0, 1100.0));
			Rods->AddInstance(FTransform(FRotator::ZeroRotator, At, FVector(0.025, 0.025, Dice.Range(0.4, 1.2))));
		}
	}
	GlowAll(Site(TEXT("SM_ECX_Lobby_GlowWarm")), GlowInterior);

	// The casino wing: basalt, the slot windows glowing with the floor's light, the marquee in its recess, ember reveals.
	if (UStaticMeshComponent* WingS = Site(TEXT("SM_ECX_WingS")))
	{
		Lit(WingS, {-1.0f, -1.0f, 0.35f, -1.0f});
	}
	else
	{
		Box(Basalt, FVector(-3000.0, -5800.0, 0.0), FVector(300.0, -1850.0, 1600.0), false);
		Box(Bronze, FVector(-3000.0, -5810.0, 1600.0), FVector(310.0, -1850.0, 1660.0), false);
		Box(Granite, FVector(300.0, -5800.0, 0.0), FVector(306.0, -1850.0, 40.0), false);
		for (int32 K = 0; K < 5; ++K)
		{
			for (const double Y : {-2830.0 + 220.0 * K, -4770.0 - 220.0 * K})
			{
				Box(Interior, FVector(298.0, Y - 60.0, 150.0), FVector(302.0, Y + 60.0, 1050.0), false)->SetCastShadow(false);
				Box(Bronze, FVector(300.0, Y + 70.0, 150.0), FVector(330.0, Y + 76.0, 1050.0), false);
			}
		}
		for (const double Z : {500.0, 1520.0})
		{
			if (Z < 1000.0)
			{
				Box(Ember, FVector(300.0, -5800.0, Z), FVector(303.0, -4660.0, Z + 4.0), false)->SetCastShadow(false);
				Box(Ember, FVector(300.0, -2940.0, Z), FVector(303.0, -1850.0, Z + 4.0), false)->SetCastShadow(false);
			}
			else
			{
				Box(Ember, FVector(300.0, -5800.0, Z), FVector(303.0, -1850.0, Z + 4.0), false)->SetCastShadow(false);
			}
		}
	}
	// The north wing: the Ember Grill's lit room behind its glass, the poker room's band upstairs, the terrace.
	if (UStaticMeshComponent* WingN = Site(TEXT("SM_ECX_WingN")))
	{
		Lit(WingN, {-1.0f, -1.0f, -1.0f, 0.35f, 0.35f, 0.0f});
	}
	else
	{
		// Solid but for the grill's room, open behind its glass (its lights shine out through it).
		Box(Basalt, FVector(-3000.0, 1850.0, 0.0), FVector(300.0, 2000.0, 800.0), false);
		Box(Basalt, FVector(-3000.0, 3400.0, 0.0), FVector(300.0, 5600.0, 800.0), false);
		Box(Basalt, FVector(-3000.0, 2000.0, 0.0), FVector(-20.0, 3400.0, 800.0), false);
		Box(Basalt, FVector(-20.0, 2000.0, 550.0), FVector(300.0, 3400.0, 800.0), false);
		Box(Mat(TEXT("GrillFloor"), 0x3a2a20, 0.4f), FVector(-20.0, 2000.0, -2.0), FVector(300.0, 3400.0, 0.0), false);
		Box(Basalt, FVector(-3000.0, 1850.0, 1450.0), FVector(300.0, 5600.0, 1600.0), false);
		Box(Bronze, FVector(-3000.0, 1850.0, 1600.0), FVector(310.0, 5610.0, 1660.0), false);
		// The grill's room (3 m deep) and the poker room's band of light.
		Box(Interior, FVector(-20.0, 2000.0, 0.0), FVector(0.0, 3400.0, 550.0), false)->SetCastShadow(false);
		Box(Mat(TEXT("PokerRoom"), 0x6f7fa8, 0.5f, 0.0f, 70.0f), FVector(-20.0, 1900.0, 800.0), FVector(298.0, 5550.0, 1450.0), false)->SetCastShadow(false);
		for (double Y = 1950.0; Y < 5550.0; Y += 60.0)
		{
			Box(Timber, FVector(300.0, Y, 820.0), FVector(330.0, Y + 6.0, 1440.0), false);
		}
		// The terrace off the poker room, its glass rail and two lanterns.
		Box(Granite, FVector(300.0, 3400.0, 780.0), FVector(700.0, 5000.0, 800.0), false);
		Box(GlassMat(false), FVector(698.0, 3400.0, 800.0), FVector(700.0, 5000.0, 900.0), false)->SetCastShadow(false);
		for (const double Y : {3600.0, 4800.0})
		{
			Box(GlowMat(GlowLantern), FVector(640.0, Y - 10.0, 900.0), FVector(660.0, Y + 10.0, 960.0), false)->SetCastShadow(false);
		}
		for (const double Z : {500.0, 1520.0})
		{
			Box(Ember, FVector(300.0, 3400.0, Z), FVector(303.0, 5600.0, Z + 4.0), false)->SetCastShadow(false);
		}
		TArray<FString> Grill = {TEXT("EMBER GRILL")};
		TextPanel(TEXT("Panel_Grill"), Grill, FVector(304.0, 2700.0, 660.0), FVector2D(700.0, 120.0), 0.0f, GlowEmber);
	}
	GlowAll(Site(TEXT("SM_ECX_Podium_GlowEmber")), GlowEmber);
	GlowAll(Site(TEXT("SM_ECX_Podium_GlowWarm")), GlowWarm);
	GlowAll(Site(TEXT("SM_ECX_Podium_GlowAsh")), GlowAsh);
	// The marquee's housing (its screen is BuildScreens').
	if (!Site(TEXT("SM_ECX_Marquee")))
	{
		UMaterialInterface* Dark = Mat(TEXT("FrameDark"), FrameDarkHex, 0.45f, 0.85f);
		Box(Dark, FVector(300.0, -4660.0, 440.0), FVector(356.0, -2940.0, 1160.0), false);
		Box(Bronze, FVector(300.0, -4660.0, 1110.0), FVector(362.0, -2940.0, 1160.0), false);
		Box(Bronze, FVector(300.0, -4660.0, 440.0), FVector(362.0, -2940.0, 490.0), false);
		Box(Bronze, FVector(300.0, -4660.0, 440.0), FVector(362.0, -4600.0, 1160.0), false);
		Box(Bronze, FVector(300.0, -3000.0, 440.0), FVector(362.0, -2940.0, 1160.0), false);
	}
}

void AEmbercrestStage::BuildEntryDoors()
{
	// The revolving door's drum (static) and its wings, turning about the drum's middle 20 cm inside the glass line.
	UMaterialInterface* Bronze = Mat(TEXT("Bronze"), BronzeHex, 0.38f, 1.0f);
	UMaterialInterface* Glass = GlassMat(false);
	const FVector Drum(-20.0, 0.0, 0.0);
	if (UStaticMeshComponent* D = Prop(TEXT("SM_ECX_RevolvingDoor"), Drum, 180.0f))
	{
		SetSlot(D, SlotOf(D, 1), Glass);
	}
	else
	{
		Cyl(Bronze, Drum + FVector(0.0, 0.0, 280.0), 172.0f, 50.0f);
		Cyl(Mat(TEXT("Granite"), GraniteHex, 0.75f), Drum - FVector(0.0, 0.0, 1.0), 172.0f, 2.0f);
		// The curved walls north and south of the openings (which face in and out, along X), as panes round the drum.
		for (const double Side : {1.0, -1.0})
		{
			for (int32 K = 0; K < 9; ++K)
			{
				const double Deg = Side * (50.0 + 10.0 * K);
				const double A = FMath::DegreesToRadians(Deg);
				const FVector P = Drum + FVector(FMath::Cos(A) * 168.0, FMath::Sin(A) * 168.0, 140.0);
				TurnedBox(Glass, P, FVector(2.0, 58.0, 280.0), static_cast<float>(Deg), false)->SetCastShadow(false);
			}
		}
	}
	RevWings = NewPart<USceneComponent>();
	RevWings->SetRelativeLocation(Drum);
	if (UStaticMeshComponent* W = Prop(TEXT("SM_ECX_RevolvingDoor_Wings"), FVector::ZeroVector, 0.0f, FVector(1.0), RevWings))
	{
		SetSlot(W, SlotOf(W, 1), Glass);
	}
	else
	{
		Cyl(Bronze, FVector::ZeroVector, 8.0f, 260.0f, false, RevWings);
		for (int32 K = 0; K < 4; ++K)
		{
			UStaticMeshComponent* Wing = Box(Glass, FVector(8.0, -1.0, 4.0), FVector(148.0, 1.0, 256.0), false, RevWings);
			Wing->SetRelativeRotation(FRotator(0.0f, 90.0f * K, 0.0f));
			Wing->SetRelativeLocation(FRotator(0.0f, 90.0f * K, 0.0f).RotateVector(FVector(78.0, 0.0, 130.0)));
			UStaticMeshComponent* Rail = Box(Bronze, FVector(8.0, -3.0, 0.0), FVector(148.0, 3.0, 6.0), false, RevWings);
			Rail->SetRelativeRotation(FRotator(0.0f, 90.0f * K, 0.0f));
			Rail->SetRelativeLocation(FRotator(0.0f, 90.0f * K, 0.0f).RotateVector(FVector(78.0, 0.0, 3.0)));
		}
	}
	// The swing doors: two pairs, each leaf on its pivot at the hinge (spec 3.1 B13).
	for (int32 K = 0; K < 4; ++K)
	{
		USceneComponent* Pivot = NewPart<USceneComponent>();
		Pivot->SetRelativeLocation(FVector(0.0, SwingHingeY[K], 0.0));
		const double Mirror = SwingMirror[K];
		if (!Prop(TEXT("SM_ECX_SwingDoor"), FVector::ZeroVector, 180.0f, FVector(1.0, Mirror, 1.0), Pivot))
		{
			// As modeled the leaf runs +Y from its hinge (turned half round: -Y here, then mirrored where it's mirrored).
			const double Dir = Mirror;
			Box(Bronze, FVector(-3.0, 0.0, 0.0), FVector(3.0, Dir * 105.0, 275.0), false, Pivot)->SetCastShadow(true);
			Box(Glass, FVector(-1.0, Dir * 10.0, 10.0), FVector(1.0, Dir * 95.0, 265.0), false, Pivot)->SetCastShadow(false);
		}
		else if (UStaticMeshComponent* Leaf = Cast<UStaticMeshComponent>(Pivot->GetChildComponent(0)))
		{
			SetSlot(Leaf, SlotOf(Leaf, 1), Glass);
		}
		SwingLeaves.Add(Pivot);
		SwingOpen.Add(0.0f);
		SwingTarget.Add(0.0f);
		// Outward is +X: a leaf running +Y turns clockwise from above (negative yaw), a mirrored one the other way.
		SwingSign.Add(Mirror > 0.0 ? -1.0f : 1.0f);
	}
}

void AEmbercrestStage::BuildCanopy()
{
	UMaterialInterface* Timber = Mat(TEXT("Timber"), TimberHex, 0.45f);
	UMaterialInterface* Bronze = Mat(TEXT("Bronze"), BronzeHex, 0.38f, 1.0f);
	UMaterialInterface* Basalt = Mat(TEXT("Basalt"), BasaltHex, 0.55f);
	UMaterialInterface* Dark = Mat(TEXT("FrameDark"), FrameDarkHex, 0.45f, 0.85f);
	if (UStaticMeshComponent* C = Site(TEXT("SM_ECX_Canopy")))
	{
		SetSlot(C, SlotOf(C, 4), SnowMat());
	}
	else
	{
		// The lit larch soffit at 7 m, the roof folding up toward the plaza with its snow, the bronze fascia.
		Box(Timber, FVector(0.0, -CanopyHalf, 700.0), FVector(CanopyX1, CanopyHalf, 716.0), false);
		Slope(Mat(TEXT("RoofMetal"), 0x3a3c40, 0.4f, 0.8f), 0.0, CanopyX1, -CanopyHalf, CanopyHalf, 840.0, 990.0, 100.0, false);
		UStaticMeshComponent* Cap = Slope(SnowMat(), 30.0, CanopyX1 - 30.0, -CanopyHalf + 30.0, CanopyHalf - 30.0, 860.0, 1008.0, 22.0, false);
		Cap->ComponentTags.Add(TEXT("Snow"));
		Box(Bronze, FVector(CanopyX1 - 30.0, -CanopyHalf, 700.0), FVector(CanopyX1 + 10.0, CanopyHalf, 990.0), false);
		for (const double S : {-1.0, 1.0})
		{
			Box(Bronze, FVector(0.0, S > 0 ? CanopyHalf - 20.0 : -CanopyHalf - 10.0, 700.0), FVector(CanopyX1 + 10.0, S > 0 ? CanopyHalf + 10.0 : -CanopyHalf + 20.0, 860.0), false);
		}
		// The four tree columns: basalt plinths, bronze trunks, struts up into the soffit.
		for (const FVector2D& Col : CanopyColumns)
		{
			const FVector Base(Col.X, Col.Y, 0.0);
			Box(Basalt, Base + FVector(-70.0, -70.0, 0.0), Base + FVector(70.0, 70.0, 110.0), false);
			Cyl(Bronze, Base + FVector(0.0, 0.0, 110.0), 30.0f, 350.0f);
			for (const FVector& Out : {FVector(260.0, 220.0, 0.0), FVector(260.0, -220.0, 0.0), FVector(-260.0, 220.0, 0.0), FVector(-260.0, -220.0, 0.0)})
			{
				Bar(Bronze, Base + FVector(0.0, 0.0, 460.0), Base + Out + FVector(0.0, 0.0, 700.0), 14.0f);
			}
		}
		// The downlights' trims in the soffit.
		for (const double X : DownlightX)
		{
			for (const double Y : DownlightY)
			{
				Cyl(Dark, FVector(X, Y, 696.0), 14.0f, 4.0f);
			}
		}
		// EMBERCREST in lit channel letters on the front fascia, RESORT · CASINO under it, and the ember line along its foot.
		TArray<FString> Letters = {TEXT("EMBERCREST"), TEXT("RESORT \xB7 CASINO")};
		TextPanel(TEXT("Panel_Canopy"), Letters, FVector(CanopyX1 + 12.0, 0.0, 850.0), FVector2D(1500.0, 240.0), 0.0f, GlowAsh);
		UMaterialInterface* Ember = GlowMat(GlowEmber);
		Box(Ember, FVector(CanopyX1 + 10.0, -CanopyHalf, 700.0), FVector(CanopyX1 + 13.0, CanopyHalf, 704.0), false)->SetCastShadow(false);
		for (const double S : {-1.0, 1.0})
		{
			Box(Ember, FVector(0.0, S > 0 ? CanopyHalf + 10.0 : -CanopyHalf - 13.0, 700.0), FVector(CanopyX1 + 13.0, S > 0 ? CanopyHalf + 13.0 : -CanopyHalf - 10.0, 704.0), false)
				->SetCastShadow(false);
		}
		for (const FVector2D& Col : CanopyColumns)
		{
			Box(Ember, FVector(Col.X + 30.0, Col.Y - 2.0, 120.0), FVector(Col.X + 33.0, Col.Y + 2.0, 460.0), false)->SetCastShadow(false);
		}
	}
	// The downlights' lenses glow whether or not the canopy is the imported one (Podium_GlowWarm has them when it is).
	if (!Ecx::LoadMesh(TEXT("SM_ECX_Podium_GlowWarm")))
	{
		UMaterialInterface* LensGlow = GlowMat(GlowWarm);
		for (const double X : DownlightX)
		{
			for (const double Y : DownlightY)
			{
				Cyl(LensGlow, FVector(X, Y, 695.0), 9.0f, 1.0f)->SetCastShadow(false);
			}
		}
	}
}

void AEmbercrestStage::BuildTower()
{
	UMaterialInterface* Basalt = Mat(TEXT("Basalt"), BasaltHex, 0.55f);
	UMaterialInterface* Bronze = Mat(TEXT("Bronze"), BronzeHex, 0.38f, 1.0f);
	UMaterialInterface* Dark = Mat(TEXT("FrameDark"), FrameDarkHex, 0.45f, 0.85f);
	UMaterialInterface* Ember = GlowMat(GlowEmber);
	UMaterialInterface* Crown = GlowMat(GlowCrown);
	// The hotel tower behind the north wing: thirteen floors of rooms, a few lit warm, a few blue with TV light, most dark.
	if (UStaticMeshComponent* T = Site(TEXT("SM_ECX_Tower")))
	{
		Lit(T, {-1.0f, -1.0f, 0.0f, -1.0f});
	}
	else
	{
		Box(Basalt, FVector(-5600.0, 1000.0, 1600.0), FVector(-3600.0, 5200.0, 6200.0), false);
		Box(Mat(TEXT("Concrete"), 0x8f8b84, 0.8f), FVector(-5300.0, 1600.0, 6200.0), FVector(-3900.0, 4600.0, 6450.0), false);
		// The rooms as the street's windows draw them (M_StreetWindow: each its own room, curtains, lit or dark).
		UMaterialInterface* WindowMat = Ecx::LoadMat(TEXT("M_StreetWindow"));
		UInstancedStaticMeshComponent* Rooms = nullptr;
		if (WindowMat)
		{
			UMaterialInstanceDynamic* W = UMaterialInstanceDynamic::Create(WindowMat, this);
			W->SetScalarParameterValue(TEXT("Glow"), 1.0f);
			HourMids.Add(W);
			Rooms = Instances(CubeMesh, W, false);
			Rooms->SetNumCustomDataFloats(3);
		}
		UInstancedStaticMeshComponent* LitRooms = Rooms ? nullptr : Instances(CubeMesh, GlowMat(GlowInterior, 0.6f), false);
		UInstancedStaticMeshComponent* DarkGlass = Rooms ? nullptr : Instances(CubeMesh, Mat(TEXT("GlassDark"), 0x0b0e12, 0.04f), false);
		FDice Dice(1312);
		auto Room = [&](const FVector& Center, float Yaw) {
			const double Roll = Dice.Next();
			const int32 Kind = Roll < 0.35 ? 1 : Roll < 0.45 ? 3 : 0;
			const FTransform Xf(FRotator(0.0f, Yaw, 0.0f), Center, FVector(0.05, 3.1, 2.7));
			if (Rooms)
			{
				const int32 I = Rooms->AddInstance(Xf);
				const float Data[3] = {static_cast<float>(Kind), static_cast<float>(Dice.Next()), static_cast<float>(0.6 + 0.6 * Dice.Next())};
				Rooms->SetCustomData(I, TArrayView<const float>(Data, 3), true);
			}
			else
			{
				(Kind > 0 ? LitRooms : DarkGlass)->AddInstance(Xf);
			}
		};
		for (int32 Floor = 0; Floor < 13; ++Floor)
		{
			const double Z = 1600.0 + 350.0 * Floor + 175.0;
			for (int32 Bay = 0; Bay < 12; ++Bay)
			{
				Room(FVector(-3598.0, 1000.0 + 350.0 * Bay + 175.0, Z), 0.0f);
			}
			for (int32 Bay = 0; Bay < 5; ++Bay)
			{
				Room(FVector(-3600.0 - 350.0 * Bay - 300.0, 998.0, Z), 90.0f);
				Room(FVector(-3600.0 - 350.0 * Bay - 300.0, 5202.0, Z), 90.0f);
			}
			// The bronze spandrel over each floor.
			Box(Bronze, FVector(-3602.0, 1000.0, Z + 140.0), FVector(-3596.0, 5200.0, Z + 175.0), false);
		}
		// Ember lines up the four corners and along the roof's edge.
		for (const FVector2D& C : {FVector2D(-3600.0, 1000.0), FVector2D(-3600.0, 5200.0), FVector2D(-5600.0, 1000.0), FVector2D(-5600.0, 5200.0)})
		{
			Box(Ember, FVector(C.X - 3.0, C.Y - 3.0, 1600.0), FVector(C.X + 3.0, C.Y + 3.0, 6200.0), false)->SetCastShadow(false);
		}
		Box(Ember, FVector(-3604.0, 1000.0, 6150.0), FVector(-3598.0, 5200.0, 6155.0), false)->SetCastShadow(false);
	}
	GlowAll(Site(TEXT("SM_ECX_Tower_GlowEmber")), GlowEmber);

	// The crown: three faceted peaks of bronze fins in the mark's proportions, lit ember from inside: the beacon the city sees.
	if (!Site(TEXT("SM_ECX_Crown")))
	{
		UInstancedStaticMeshComponent* Fins = Instances(CubeMesh, Bronze, true);
		for (double Y = 1030.0; Y < 5200.0; Y += 60.0)
		{
			const double Top = CrownTop(Y);
			if (Top < 6220.0)
			{
				continue;
			}
			for (const double X : {-3560.0, -5640.0})
			{
				Fins->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, Y, (6200.0 + Top) * 0.5), FVector(0.3, 0.06, (Top - 6200.0) / 100.0)));
			}
		}
		Box(Dark, FVector(-5600.0, 1000.0, 6200.0), FVector(-3600.0, 5200.0, 6260.0), false);
	}
	if (UStaticMeshComponent* G = Site(TEXT("SM_ECX_Crown_GlowEmber")))
	{
		GlowAll(G, GlowCrown);
	}
	else
	{
		// Each peak's edges, front and back, and a glow behind the fins.
		static const FVector2D Edge[7] = {FVector2D(1000.0, 6200.0), FVector2D(1700.0, 7270.0), FVector2D(2400.0, 6760.0), FVector2D(3100.0, 7800.0), FVector2D(3800.0, 6700.0),
			FVector2D(4500.0, 7110.0), FVector2D(5200.0, 6200.0)};
		for (const double X : {-3535.0, -5665.0})
		{
			for (int32 I = 0; I < 6; ++I)
			{
				Bar(Crown, FVector(X, Edge[I].X, Edge[I].Y), FVector(X, Edge[I + 1].X, Edge[I + 1].Y), 8.0f)->SetCastShadow(false);
			}
		}
		for (double Y = 1300.0; Y < 5000.0; Y += 400.0)
		{
			const double Top = CrownTop(Y);
			Box(GlowMat(GlowCrown, 0.25f), FVector(-3700.0, Y - 180.0, 6260.0), FVector(-3690.0, Y + 180.0, FMath::Max(6300.0, Top - 150.0)), false)->SetCastShadow(false);
		}
	}
	// The crest emblem on the crown's front.
	if (UStaticMeshComponent* Crest = Site(TEXT("SM_ECX_CrownCrest")))
	{
		Lit(Crest, {-1.0f, 0.3f, -1.0f});
	}
	else
	{
		TArray<FString> Mark = {TEXT("@mark")};
		TextPanel(TEXT("Panel_Crest"), Mark, FVector(-3530.0, 3100.0, 6610.0), FVector2D(620.0, 500.0), 0.0f, GlowAsh);
	}
}

// ------------------------------------------------------------------ the plaza (spec 2.5, 3.2)

void AEmbercrestStage::BuildPlaza()
{
	UMaterialInterface* Basalt = Mat(TEXT("Basalt"), BasaltHex, 0.55f);
	UMaterialInterface* Bronze = Mat(TEXT("Bronze"), BronzeHex, 0.38f, 1.0f);
	UMaterialInterface* Granite = Mat(TEXT("Granite"), GraniteHex, 0.75f);
	// The two cascades either side of the walk: basin B, the bronze weir, basin A; fire bowls stand in their water.
	UMaterialInterface* Water = nullptr;
	UMaterialInterface* Falls = nullptr;
	if (UMaterialInterface* W = Ecx::LoadMat(TEXT("M_ECX_Water")))
	{
		Water = UMaterialInstanceDynamic::Create(W, this);
	}
	if (UMaterialInterface* F = Ecx::LoadMat(TEXT("M_ECX_Falls")))
	{
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(F, this);
		HourMids.Add(M);
		Falls = M;
	}
	if (!Water)
	{
		Water = Mat(TEXT("WaterFallback"), 0x050607, 0.04f);
	}
	if (!Falls)
	{
		Falls = Mat(TEXT("FallsFallback"), 0xc8d4dc, 0.1f, 0.0f, 0.3f);
	}
	for (const double S : {1.0, -1.0})
	{
		const FVector Mirror(1.0, S, 1.0);
		if (UStaticMeshComponent* C = Site(TEXT("SM_ECX_Cascade"), false, Mirror))
		{
			SetSlot(C, SlotOf(C, 3), SnowMat());
		}
		else
		{
			const double Y0 = S > 0 ? 460.0 : -1490.0;
			const double Y1 = S > 0 ? 1490.0 : -460.0;
			UMaterialInterface* Black = Mat(TEXT("GraniteBlack"), 0x0c0d0f, 0.08f);
			for (const FVector& Basin : {FVector(2520.0, 3560.0, ZLandB), FVector(3560.0, 4640.0, ZLandA)})
			{
				const double Top = Basin.Z + 45.0;
				Box(Black, FVector(Basin.X, Y0, Basin.Z - 60.0), FVector(Basin.Y, Y1, Basin.Z - 40.0), false);
				Box(Basalt, FVector(Basin.X, Y0, Basin.Z - 60.0), FVector(Basin.Y, Y0 + 40.0, Top), false);
				Box(Basalt, FVector(Basin.X, Y1 - 40.0, Basin.Z - 60.0), FVector(Basin.Y, Y1, Top), false);
				UStaticMeshComponent* Cap = Box(SnowMat(), FVector(Basin.X, S > 0 ? Y1 - 40.0 : Y0, Top), FVector(Basin.Y, S > 0 ? Y1 : Y0 + 40.0, Top + 6.0), false);
				Cap->ComponentTags.Add(TEXT("Snow"));
			}
			Box(Basalt, FVector(2520.0, Y0, ZLandB - 60.0), FVector(2560.0, Y1, ZLandB + 45.0), false);
			Box(Basalt, FVector(4600.0, Y0, ZLandA - 60.0), FVector(4640.0, Y1, ZLandA + 45.0), false);
			Box(Bronze, FVector(3556.0, Y0 + 40.0, ZLandA - 60.0), FVector(3564.0, Y1 - 40.0, ZLandB - 36.0), false);
			Box(Bronze, FVector(2556.0, Y0 + 40.0, ZLandB - 40.0), FVector(2566.0, Y1 - 40.0, ZLandB + 10.0), false);
		}
		if (UStaticMeshComponent* W = Site(TEXT("SM_ECX_Cascade_Water"), false, Mirror))
		{
			SetSlot(W, SlotOf(W, 0, TEXT("ecx_water")), Water);
			SetSlot(W, SlotOf(W, 1, TEXT("ecx_falls")), Falls);
			W->SetCastShadow(false);
		}
		else
		{
			const double Y0 = S > 0 ? 500.0 : -1450.0;
			const double Y1 = S > 0 ? 1450.0 : -500.0;
			// The stand-in sheets flow by their own height (no UVs along the flow on a box).
			if (UMaterialInstanceDynamic* FallsMid = Cast<UMaterialInstanceDynamic>(Falls))
			{
				FallsMid->SetScalarParameterValue(TEXT("LocalUV"), 1.0f);
			}
			Box(Water, FVector(2560.0, Y0, -39.0), FVector(3556.0, Y1, -37.0), false)->SetCastShadow(false);
			Box(Water, FVector(3564.0, Y0, -114.0), FVector(4600.0, Y1, -112.0), false)->SetCastShadow(false);
			Box(Falls, FVector(3564.0, Y0, -112.0), FVector(3566.0, Y1, -37.0), false)->SetCastShadow(false);
			Box(Falls, FVector(2566.0, Y0, -37.0), FVector(2568.0, Y1, 10.0), false)->SetCastShadow(false);
		}
	}
	// The monument by the southern approach: the mark, EMBERCREST, the LED strip, snow on its cap.
	USceneComponent* Monument = NewPart<USceneComponent>();
	Monument->SetRelativeLocationAndRotation(FVector(4550.0, -2050.0, ZLandA), FRotator(0.0f, 165.0f, 0.0f));
	if (UStaticMeshComponent* M = Prop(TEXT("SM_ECX_MonumentSign"), FVector::ZeroVector, 0.0f, FVector(1.0), Monument))
	{
		Lit(M, {-1.0f, -1.0f, -1.0f, 0.0f, -1.0f});
		SetSlot(M, SlotOf(M, 4), SnowMat());
		GlowAll(Prop(TEXT("SM_ECX_MonumentSign_GlowEmber"), FVector::ZeroVector, 0.0f, FVector(1.0), Monument), GlowEmber);
	}
	else
	{
		Box(Granite, FVector(-70.0, -470.0, 0.0), FVector(70.0, 470.0, 50.0), false, Monument);
		Box(Basalt, FVector(-60.0, -450.0, 50.0), FVector(60.0, 450.0, 430.0), false, Monument);
		UStaticMeshComponent* Cap = Box(SnowMat(), FVector(-58.0, -448.0, 430.0), FVector(58.0, 448.0, 446.0), false, Monument);
		Cap->ComponentTags.Add(TEXT("Snow"));
		TArray<FString> Lines = {TEXT("EMBERCREST"), TEXT("RESORT \xB7 CASINO \xB7 POKER SERIES")};
		// Both faces read from their own side: the front faces -X in the monument's frame, the back +X.
		TextPanel(TEXT("Panel_MonumentFront"), Lines, FVector(-62.0, 60.0, 270.0), FVector2D(700.0, 160.0), 180.0f, GlowAsh, 2.0f, Monument);
		TextPanel(TEXT("Panel_MonumentBack"), Lines, FVector(62.0, -60.0, 270.0), FVector2D(700.0, 160.0), 0.0f, GlowAsh, 2.0f, Monument);
		TArray<FString> Mark = {TEXT("@mark")};
		TextPanel(TEXT("Panel_MonumentMarkF"), Mark, FVector(-62.0, -330.0, 260.0), FVector2D(220.0, 220.0), 180.0f, GlowAsh, 2.0f, Monument);
		TextPanel(TEXT("Panel_MonumentMarkB"), Mark, FVector(62.0, 330.0, 260.0), FVector2D(220.0, 220.0), 0.0f, GlowAsh, 2.0f, Monument);
		Box(GlowMat(GlowEmber), FVector(-63.0, -440.0, 52.0), FVector(-61.0, 440.0, 55.0), false, Monument)->SetCastShadow(false);
		Box(GlowMat(GlowEmber), FVector(61.0, -440.0, 52.0), FVector(63.0, 440.0, 55.0), false, Monument)->SetCastShadow(false);
	}
	Block(FVector(-70.0, -470.0, 0.0), FVector(70.0, 470.0, 430.0), 0.0f, Monument);
}

void AEmbercrestStage::BuildFireBowls()
{
	UMaterialInterface* Basalt = Mat(TEXT("Basalt"), BasaltHex, 0.55f);
	UMaterialInterface* Copper = Mat(TEXT("Copper"), CopperHex, 0.3f, 1.0f);
	UMaterialInterface* FlameParent = Ecx::LoadMat(TEXT("M_ECX_Flame"));
	int32 Seed = 3;
	for (const FVector& At : FireBowls)
	{
		FEcxFire F;
		F.At = At + FVector(0.0, 0.0, 160.0);
		F.Seed = Seed++ * 7919;
		UMaterialInstanceDynamic* Flame = FlameParent ? UMaterialInstanceDynamic::Create(FlameParent, this) : nullptr;
		if (Flame)
		{
			Flame->SetScalarParameterValue(TEXT("Seed"), static_cast<float>(F.Seed % 97) / 97.0f);
			HourMids.Add(Flame);
		}
		F.Flame = Flame;
		if (UStaticMeshComponent* Bowl = Prop(TEXT("SM_ECX_FireBowl"), At, 180.0f))
		{
			// The embers between the rocks ([lit], slot 2) flicker with the flame.
			const int32 Media = SlotOf(Bowl, 2);
			if (UMaterialInterface* Baked = Media != INDEX_NONE ? Bowl->GetMaterial(Media) : nullptr)
			{
				TArray<FMaterialParameterInfo> Infos;
				TArray<FGuid> Ids;
				Baked->GetAllVectorParameterInfo(Infos, Ids);
				for (const FMaterialParameterInfo& Info : Infos)
				{
					// As DimImportedGlow reads the glTF importer's names ("EmissiveFactor", "_RGB", spaces or not).
					const FString K = Info.Name.ToString().ToLower().Replace(TEXT("_"), TEXT("")).Replace(TEXT(" "), TEXT(""));
					FLinearColor Glow;
					if (K.StartsWith(TEXT("emissivefactor")) && Baked->GetVectorParameterValue(FHashedMaterialParameterInfo(Info), Glow) && !Glow.IsAlmostBlack())
					{
						UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Baked, this);
						Bowl->SetMaterial(Media, M);
						F.Media = M;
						F.MediaGlow = Glow;
						F.MediaParam = Info.Name;
						break;
					}
				}
			}
		}
		else
		{
			Box(Basalt, At + FVector(-50.0, -50.0, 0.0), At + FVector(50.0, 50.0, 90.0), false);
			UStaticMeshComponent* B = Cyl(Copper, At + FVector(0.0, 0.0, 90.0), 80.0f, 40.0f);
			B->SetRelativeScale3D(FVector(1.6, 1.6, 0.4));
			Cyl(Mat(TEXT("LavaRock"), 0x1a1614, 0.9f, 0.0f, 0.0f), At + FVector(0.0, 0.0, 124.0), 70.0f, 6.0f);
		}
		UMaterialInterface* FlameMat = Flame ? static_cast<UMaterialInterface*>(Flame) : GlowMat(GlowEmber, 2.0f);
		if (UStaticMeshComponent* Q = Prop(TEXT("SM_ECX_FireBowl_Flame"), At, 180.0f))
		{
			SetSlot(Q, SlotOf(Q, 0, TEXT("ecx_flame")), FlameMat);
			Q->SetCastShadow(false);
		}
		else
		{
			// Three crossed panes, the flame drawn on each by its own position (LocalUV).
			if (Flame)
			{
				Flame->SetScalarParameterValue(TEXT("LocalUV"), 1.0f);
			}
			for (int32 K = 0; K < 3; ++K)
			{
				UStaticMeshComponent* P = TurnedBox(FlameMat, At + FVector(0.0, 0.0, 170.0), FVector(1.0, 140.0, 130.0), 60.0f * K, false);
				P->SetCastShadow(false);
				P->SetVisibleInRayTracing(false);
			}
		}
		Fires.Add(F);
		Block(At + FVector(-55.0, -55.0, 0.0), At + FVector(55.0, 55.0, 135.0));
	}
	// Steam off the heated pools: soft cards turning to the camera (BuildLights sets their glow; UpdateSteam moves them).
	if (UMaterialInterface* Mist = Ecx::LoadMat(TEXT("M_ECX_Mist")))
	{
		FDice Dice(77);
		for (const double S : {1.0, -1.0})
		{
			for (int32 K = 0; K < 16; ++K)
			{
				UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Mist, this);
				M->SetScalarParameterValue(TEXT("DepthFade"), 50.0f);
				M->SetScalarParameterValue(TEXT("Steam"), 1.0f);
				M->SetScalarParameterValue(TEXT("Seed"), static_cast<float>(Dice.Next()));
				HourMids.Add(M);
				UStaticMeshComponent* Card = NewPart<UStaticMeshComponent>();
				Card->SetStaticMesh(PlaneMesh);
				Card->SetMaterial(0, M);
				Card->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Card->SetCastShadow(false);
				Card->SetVisibleInRayTracing(false);
				Card->bAffectDistanceFieldLighting = false;
				const bool bB = K < 8;
				const FVector Base(bB ? Dice.Range(2620.0, 3500.0) : Dice.Range(3620.0, 4560.0), S * Dice.Range(560.0, 1400.0), bB ? -30.0 : -105.0);
				Card->SetRelativeLocation(Base);
				Card->SetRelativeScale3D(FVector(Dice.Range(1.6, 2.6)));
				Steam.Add(Card);
				SteamBase.Add(Base);
			}
		}
	}
}

// ------------------------------------------------------------------ the fixtures and the furniture

void AEmbercrestStage::BuildFixtures()
{
	UMaterialInterface* Basalt = Mat(TEXT("Basalt"), BasaltHex, 0.55f);
	UMaterialInterface* Bronze = Mat(TEXT("Bronze"), BronzeHex, 0.38f, 1.0f);
	UMaterialInterface* Timber = Mat(TEXT("Timber"), TimberHex, 0.45f);
	UMaterialInterface* Steel = Mat(TEXT("Steel"), 0x1c1e22, 0.4f, 0.8f);
	UMaterialInterface* Dark = Mat(TEXT("FrameDark"), FrameDarkHex, 0.45f, 0.85f);
	UMaterialInterface* Snow = SnowMat();
	auto Ground = [](double X, double Y, double Z) { return FMath::Abs(Y) > 3300.0 && X > 2125.0 && X < XBay1 ? ArmZ(X) : Z; };

	// The lamp posts: basalt bases, bronze-black poles, lanterns with frosted panes (lit from dusk).
	for (const FVector& L : LampPosts)
	{
		const FVector At(L.X, L.Y, Ground(L.X, L.Y, L.Z));
		if (Prop(TEXT("SM_ECX_LampPost"), At, 180.0f))
		{
			GlowAll(Prop(TEXT("SM_ECX_LampPost_GlowWarm"), At, 180.0f), GlowLantern);
		}
		else
		{
			Box(Basalt, At + FVector(-20.0, -20.0, 0.0), At + FVector(20.0, 20.0, 90.0), false);
			Cyl(Dark, At + FVector(0.0, 0.0, 90.0), 6.0f, 340.0f);
			Box(Bronze, At + FVector(-22.0, -22.0, 425.0), At + FVector(22.0, 22.0, 430.0), false);
			Box(Bronze, At + FVector(-22.0, -22.0, 485.0), At + FVector(22.0, 22.0, 495.0), false);
			Box(GlowMat(GlowLantern), At + FVector(-18.0, -18.0, 430.0), At + FVector(18.0, 18.0, 485.0), false)->SetCastShadow(false);
		}
		Block(At + FVector(-22.0, -22.0, 0.0), At + FVector(22.0, 22.0, 300.0));
	}
	// The bollards along the curb, each with its ember ring.
	TArray<FVector> Bollards;
	for (const double Y : {420.0, 660.0, 900.0, 1140.0, 1380.0, 1620.0})
	{
		Bollards.Add(FVector(775.0, Y, 0.0));
		Bollards.Add(FVector(775.0, -Y, 0.0));
	}
	Bollards.Add(FVector(1675.0, 420.0, 0.0));
	Bollards.Add(FVector(1675.0, -420.0, 0.0));
	for (const FVector& B : Bollards)
	{
		if (Prop(TEXT("SM_ECX_Bollard"), B, 180.0f))
		{
			GlowAll(Prop(TEXT("SM_ECX_Bollard_GlowEmber"), B, 180.0f), GlowEmber);
		}
		else
		{
			Cyl(Bronze, B, 11.0f, 95.0f);
			Cyl(GlowMat(GlowEmber), B + FVector(0.0, 0.0, 81.0), 11.5f, 2.0f)->SetCastShadow(false);
		}
		Block(B + FVector(-11.0, -11.0, 0.0), B + FVector(11.0, 11.0, 95.0));
	}
	// The planters, each with a young spruce.
	const FVector Planters[7] = {FVector(4990.0, 1300.0, ZPlatform), FVector(4990.0, 1900.0, ZPlatform), FVector(4990.0, -2000.0, ZPlatform), FVector(460.0, 2000.0, 0.0),
		FVector(460.0, -2000.0, 0.0), FVector(2000.0, 1860.0, 0.0), FVector(2000.0, -1860.0, 0.0)};
	for (const FVector& P : Planters)
	{
		if (UStaticMeshComponent* C = Prop(TEXT("SM_ECX_Planter"), P, 180.0f))
		{
			SetSlot(C, SlotOf(C, 2), Snow);
		}
		else
		{
			Box(Basalt, P + FVector(-120.0, -120.0, 0.0), P + FVector(120.0, 120.0, 100.0), false);
			Box(Bronze, P + FVector(-121.0, -121.0, 100.0), P + FVector(121.0, 121.0, 110.0), false);
			UStaticMeshComponent* Mound = Box(Snow, P + FVector(-100.0, -100.0, 100.0), P + FVector(100.0, 100.0, 116.0), false);
			Mound->ComponentTags.Add(TEXT("Snow"));
		}
		Block(P + FVector(-120.0, -120.0, 0.0), P + FVector(120.0, 120.0, 110.0));
	}
	// The benches: on the landings beside the walk, on the platform, at the overlook.
	struct FBench
	{
		FVector At;
		float Yaw;
	};
	const FBench Benches[6] = {{FVector(4140.0, 305.0, ZLandA), 90.0f}, {FVector(4140.0, -305.0, ZLandA), 90.0f}, {FVector(2980.0, 305.0, ZLandB), 90.0f},
		{FVector(2980.0, -305.0, ZLandB), 90.0f}, {FVector(4960.0, 850.0, ZPlatform), 180.0f}, {FVector(6600.0, 1300.0, ZOverlook), 180.0f}};
	for (const FBench& B : Benches)
	{
		if (!Prop(TEXT("SM_ECX_Bench"), B.At, B.Yaw))
		{
			const FVector Along = FRotator(0.0f, B.Yaw, 0.0f).RotateVector(FVector(0.0, 1.0, 0.0));
			for (const double S : {-80.0, 80.0})
			{
				TurnedBox(Basalt, B.At + Along * S + FVector(0.0, 0.0, 20.0), FVector(50.0, 30.0, 40.0), B.Yaw, false);
			}
			TurnedBox(Timber, B.At + FVector(0.0, 0.0, 42.0), FVector(55.0, 220.0, 8.0), B.Yaw, false);
		}
		Block(B.At - FVector(28.0, 110.0, 0.0), B.At + FVector(28.0, 110.0, 46.0), B.Yaw);
	}
	// The valet's stand, the bellman's cart, the ash urn by the door of the north wing.
	if (UStaticMeshComponent* V = Prop(TEXT("SM_ECX_ValetPodium"), FVector(640.0, -1100.0, 0.0), 180.0f))
	{
		Lit(V, {-1.0f, -1.0f, -1.0f, 0.3f});
	}
	else
	{
		Box(Timber, FVector(610.0, -1145.0, 0.0), FVector(670.0, -1055.0, 110.0), false);
		Box(Bronze, FVector(605.0, -1150.0, 110.0), FVector(675.0, -1050.0, 115.0), false);
	}
	Block(FVector(610.0, -1145.0, 0.0), FVector(670.0, -1055.0, 115.0));
	if (!Prop(TEXT("SM_ECX_LuggageCart"), FVector(240.0, 720.0, 0.0), 90.0f))
	{
		UMaterialInterface* Brass = Mat(TEXT("Brass"), 0xc9a24d, 0.18f, 1.0f);
		Box(Mat(TEXT("Carpet"), NavyHex, 0.9f), FVector(195.0, 625.0, 20.0), FVector(285.0, 815.0, 28.0), false);
		for (const FVector2D& C : {FVector2D(198.0, 628.0), FVector2D(282.0, 628.0), FVector2D(198.0, 812.0), FVector2D(282.0, 812.0)})
		{
			Cyl(Brass, FVector(C.X, C.Y, 28.0), 1.5f, 165.0f);
		}
		Box(Brass, FVector(198.0, 628.0, 190.0), FVector(282.0, 812.0, 195.0), false);
		Box(Mat(TEXT("CaseEmber"), 0xd9531e, 0.35f), FVector(205.0, 640.0, 28.0), FVector(275.0, 690.0, 95.0), false);
		Box(Mat(TEXT("CaseNavy"), NavyHex, 0.35f), FVector(205.0, 700.0, 28.0), FVector(275.0, 760.0, 80.0), false);
	}
	Block(FVector(195.0, 625.0, 0.0), FVector(285.0, 815.0, 195.0));
	if (!Prop(TEXT("SM_ECX_AshUrn"), FVector(2240.0, 1960.0, 0.0), 0.0f))
	{
		Cyl(Bronze, FVector(2240.0, 1960.0, 0.0), 19.0f, 75.0f);
	}
	Block(FVector(2221.0, 1941.0, 0.0), FVector(2259.0, 1979.0, 75.0));

	// The 14's shelter (its open side to the road), the stop's sign, the coin viewer at the overlook.
	USceneComponent* Shelter = NewPart<USceneComponent>();
	Shelter->SetRelativeLocationAndRotation(FVector(5060.0, -1200.0, ZPlatform), FRotator(0.0f, 180.0f, 0.0f));
	if (UStaticMeshComponent* C = Prop(TEXT("SM_ECX_BusShelter"), FVector::ZeroVector, 0.0f, FVector(1.0), Shelter))
	{
		SetSlot(C, SlotOf(C, 2), GlassMat(false));
		SetSlot(C, SlotOf(C, 4), Snow);
		Lit(C, {-1.0f, -1.0f, -1.0f, 0.0f, -1.0f, -1.0f});
		GlowAll(Prop(TEXT("SM_ECX_BusShelter_GlowCool"), FVector::ZeroVector, 0.0f, FVector(1.0), Shelter), GlowCool);
	}
	else
	{
		// In the shelter's own frame its open side faces -X (turned half round, the road).
		for (const double Y : {-395.0, 395.0})
		{
			Box(Steel, FVector(60.0, Y - 5.0, 0.0), FVector(70.0, Y + 5.0, 270.0), false, Shelter);
		}
		Box(Steel, FVector(-90.0, -400.0, 270.0), FVector(90.0, 400.0, 282.0), false, Shelter);
		Box(Timber, FVector(-88.0, -398.0, 266.0), FVector(88.0, 398.0, 270.0), false, Shelter);
		UStaticMeshComponent* Cap = Box(Snow, FVector(-86.0, -396.0, 282.0), FVector(86.0, 396.0, 296.0), false, Shelter);
		Cap->ComponentTags.Add(TEXT("Snow"));
		Box(GlassMat(false), FVector(70.0, -390.0, 10.0), FVector(72.0, 390.0, 260.0), false, Shelter)->SetCastShadow(false);
		Box(Timber, FVector(20.0, -300.0, 42.0), FVector(60.0, 300.0, 48.0), false, Shelter);
		// The lit route map at the south end (local -Y after the turn is +Y: the south end is local +Y here).
		Box(Mat(TEXT("MapCase"), 0xe8eef0, 0.3f, 0.0f, 25.0f), FVector(-40.0, 385.0, 60.0), FVector(40.0, 395.0, 240.0), false, Shelter)->SetCastShadow(false);
		Box(GlowMat(GlowCool), FVector(-10.0, -380.0, 264.0), FVector(10.0, 380.0, 266.0), false, Shelter)->SetCastShadow(false);
	}
	Block(FVector(60.0, -400.0, 0.0), FVector(80.0, 400.0, 270.0), 0.0f, Shelter);
	Block(FVector(-90.0, -400.0, 0.0), FVector(80.0, -380.0, 270.0), 0.0f, Shelter);
	Block(FVector(-90.0, 380.0, 0.0), FVector(80.0, 400.0, 270.0), 0.0f, Shelter);
	if (!Prop(TEXT("SM_ECX_BusStopSign"), FVector(5210.0, 300.0, ZPlatform), 90.0f))
	{
		Cyl(Steel, FVector(5210.0, 300.0, ZPlatform), 5.0f, 320.0f);
		Box(Mat(TEXT("TransitGreen"), 0x0f6b3a, 0.4f), FVector(5175.0, 298.0, ZPlatform + 240.0), FVector(5245.0, 302.0, ZPlatform + 310.0), false);
	}
	Block(FVector(5205.0, 295.0, ZPlatform), FVector(5215.0, 305.0, ZPlatform + 320.0));
	if (!Prop(TEXT("SM_ECX_Viewer"), FVector(6930.0, 900.0, ZOverlook), 0.0f))
	{
		Cyl(Dark, FVector(6930.0, 900.0, ZOverlook), 7.0f, 120.0f);
		Box(Bronze, FVector(6900.0, 880.0, ZOverlook + 120.0), FVector(6970.0, 920.0, ZOverlook + 160.0), false);
	}
	Block(FVector(6900.0, 875.0, ZOverlook), FVector(6960.0, 925.0, ZOverlook + 160.0));

	// The plowed snow: windrows along the arms, corner piles, wind drifts at the parapet's foot and in the beds.
	FDice Dice(4242);
	struct FBank
	{
		const TCHAR* Name;
		FVector At;
		float Yaw;
		float Scale;
		FVector Size; // the stand-in's meters
	};
	TArray<FBank> Banks;
	for (const double X : {2600.0, 3600.0, 4600.0})
	{
		for (const double S : {-1.0, 1.0})
		{
			Banks.Add({TEXT("SM_ECX_SnowBank_A"), FVector(X, S * 3550.0, ArmZ(X)), 90.0f, 1.0f, FVector(1.6, 10.0, 0.9)});
		}
	}
	Banks.Add({TEXT("SM_ECX_SnowBank_A"), FVector(5420.0, -4400.0, ZRoad), 180.0f, 1.0f, FVector(1.6, 10.0, 0.9)});
	Banks.Add({TEXT("SM_ECX_SnowBank_A"), FVector(5420.0, 4400.0, ZRoad), 180.0f, 1.0f, FVector(1.6, 10.0, 0.9)});
	for (const double S : {-1.0, 1.0})
	{
		Banks.Add({TEXT("SM_ECX_SnowBank_B"), FVector(5000.0, S * 2430.0, ZPlatform), static_cast<float>(Dice.Range(0.0, 360.0)), 0.8f, FVector(6.0, 5.0, 1.6)});
		Banks.Add({TEXT("SM_ECX_SnowBank_B"), FVector(2200.0, S * 3700.0, ZLane), static_cast<float>(Dice.Range(0.0, 360.0)), 1.0f, FVector(6.0, 5.0, 1.6)});
	}
	for (const double Y : {-4000.0, -2500.0, -1000.0, 2500.0, 3800.0})
	{
		Banks.Add({TEXT("SM_ECX_SnowBank_C"), FVector(6340.0, Y, ZRoad), 180.0f, 1.0f, FVector(2.5, 8.0, 0.45)});
	}
	for (const double S : {-1.0, 1.0})
	{
		Banks.Add({TEXT("SM_ECX_SnowBank_C"), FVector(3000.0, S * 2100.0, ZLandB + 20.0), 90.0f, 1.0f, FVector(2.5, 8.0, 0.45)});
		Banks.Add({TEXT("SM_ECX_SnowBank_C"), FVector(4000.0, S * 2200.0, ZLandA + 20.0), 90.0f, 1.0f, FVector(2.5, 8.0, 0.45)});
		Banks.Add({TEXT("SM_ECX_SnowBank_C"), FVector(400.0, S * 3000.0, 15.0), 180.0f, 1.0f, FVector(2.5, 8.0, 0.45)});
	}
	for (const FBank& B : Banks)
	{
		if (UStaticMeshComponent* C = Prop(B.Name, B.At, B.Yaw, FVector(B.Scale)))
		{
			SetSlot(C, SlotOf(C, 0), Snow);
		}
		else
		{
			// A flattened sphere, half sunk.
			UStaticMeshComponent* C2 = NewPart<UStaticMeshComponent>();
			C2->SetStaticMesh(SphereMesh);
			C2->SetMaterial(0, Snow);
			C2->SetRelativeLocationAndRotation(B.At, FRotator(0.0f, B.Yaw, 0.0f));
			C2->SetRelativeScale3D(FVector(B.Size.X, B.Size.Y, B.Size.Z * 2.0) * B.Scale);
			C2->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C2->ComponentTags.Add(TEXT("Snow"));
		}
	}

	// The parapet over the drop: modules stepped down the road's grade, round the overlook.
	struct FRun
	{
		double X0, Y0, X1, Y1;
		float Yaw; // the module's inner face (-X as modeled) toward the road or the overlook
	};
	// Each run on its modules' inner face: the road's side at X 6400, the overlook's returns at Y 600 and 2000 (their walls
	// behind them, south and north), its outer side at X 7000.
	const FRun Runs[5] = {{XParapet, -20000.0, XParapet, 555.0, 0.0f}, {XParapet, 2045.0, XParapet, 12000.0, 0.0f}, {XParapet, 600.0, XOverlook1 + 45.0, 600.0, -90.0f},
		{XParapet, 2000.0, XOverlook1 + 45.0, 2000.0, 90.0f}, {XOverlook1, 555.0, XOverlook1, 2045.0, 0.0f}};
	UMaterialInterface* Rail = Mat(TEXT("BronzeRime"), 0x6b5a4c, 0.5f, 0.8f);
	for (const FRun& R : Runs)
	{
		const FVector2D A(R.X0, R.Y0);
		const FVector2D B(R.X1, R.Y1);
		const double Len = FVector2D::Distance(A, B);
		const int32 Count = FMath::Max(1, FMath::RoundToInt(Len / 600.0));
		const double Each = Len / Count;
		for (int32 K = 0; K < Count; ++K)
		{
			const FVector2D Mid = A + (B - A) * ((K + 0.5) / Count);
			const bool bAlongY = FMath::Abs(R.X1 - R.X0) < 1.0;
			const double Z = R.X0 >= XOverlook1 || !bAlongY ? ZOverlook : RoadZ(Mid.Y);
			const FVector At(Mid.X, Mid.Y, Z);
			if (UStaticMeshComponent* C = Prop(TEXT("SM_ECX_ViewRail"), At, R.Yaw, FVector(1.0, Each / 600.0, 1.0)))
			{
				SetSlot(C, SlotOf(C, 2), Snow);
			}
			else
			{
				// In the module's frame: the wall behind the inner face (+X), its face running down the drop, the rail, the snow.
				const FVector In = FRotator(0.0f, R.Yaw, 0.0f).RotateVector(FVector(22.5, 0.0, 0.0));
				TurnedBox(Basalt, At + In + FVector(0.0, 0.0, -287.5), FVector(45.0, Each, 725.0), R.Yaw, false);
				UStaticMeshComponent* Cap = TurnedBox(Snow, At + In + FVector(0.0, 0.0, 80.0), FVector(42.0, Each, 10.0), R.Yaw, false);
				Cap->ComponentTags.Add(TEXT("Snow"));
				TurnedBox(Rail, At + In + FVector(0.0, 0.0, 108.0), FVector(6.0, Each, 6.0), R.Yaw, false);
			}
			// What blocks: 180 above the road, the wall's depth.
			const FVector In = FRotator(0.0f, R.Yaw, 0.0f).RotateVector(FVector(22.5, 0.0, 0.0));
			Block(At + In - FVector(bAlongY ? 22.5 : Each * 0.5, bAlongY ? Each * 0.5 : 22.5, 300.0), At + In + FVector(bAlongY ? 22.5 : Each * 0.5, bAlongY ? Each * 0.5 : 22.5, 180.0));
		}
	}

	// Cars: two at the curb under the canopy, two coming up the south arm, each in its own paint.
	struct FCar
	{
		FVector At;
		float Facing;
		uint32 Paint;
		float Pitch;
	};
	const float ArmPitch = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan((ZLane - ZRoad) / (XBay1 - 2125.0))));
	const FCar Cars[4] = {{FVector(1040.0, -760.0, ZLane), 90.0f, 0x15171b, 0.0f}, {FVector(1040.0, 900.0, ZLane), 90.0f, 0x5a1a14, 0.0f},
		{FVector(4000.0, -3000.0, ArmZ(4000.0)), 180.0f, 0x9aa0a6, ArmPitch}, {FVector(3100.0, -3000.0, ArmZ(3100.0)), 180.0f, 0x1a2340, ArmPitch}};
	for (const FCar& Car : Cars)
	{
		if (UStaticMesh* Sedan = Ecx::LoadMesh(TEXT("SM_Sedan")))
		{
			UStaticMeshComponent* C = NewPart<UStaticMeshComponent>();
			C->SetStaticMesh(Sedan);
			// Facing -X up the arm: its nose lifted to the grade (a turn about Y, after BlenderFacing's yaw).
			const FQuat Q = FQuat(FRotator(-Car.Pitch, 0.0f, 0.0f)) * FQuat(BlenderFacing(Car.Facing));
			C->SetRelativeLocationAndRotation(Car.At, Q);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			const int32 PaintSlot = ImportedSlot(C, 0);
			TintImported(C, PaintSlot, Srgb(Car.Paint));
			if (UMaterialInstanceDynamic* Tint = Cast<UMaterialInstanceDynamic>(C->GetMaterial(PaintSlot)))
			{
				Tint->SetFlags(RF_Transient);
			}
		}
		else
		{
			const FVector Along = FRotator(0.0f, Car.Facing, 0.0f).Vector();
			const float Yaw = Car.Facing;
			TurnedBox(Mat(*FString::Printf(TEXT("Car%06x"), Car.Paint), Car.Paint, 0.25f, 0.6f), Car.At + FVector(0.0, 0.0, 60.0), FVector(470.0, 180.0, 90.0), Yaw, false);
			TurnedBox(Mat(TEXT("CarGlass"), 0x0b0e12, 0.05f, 0.4f), Car.At + Along * -20.0 + FVector(0.0, 0.0, 128.0), FVector(230.0, 160.0, 45.0), Yaw, false);
		}
		Block(Car.At - FVector(242.0, 92.0, 0.0), Car.At + FVector(242.0, 92.0, 145.0), Car.Facing);
	}
}

// ------------------------------------------------------------------ the land (spec 2.6, 2.7, 3.4)

void AEmbercrestStage::BuildLandscape()
{
	UMaterialInterface* Snow = SnowMat();
	// The near bench and slopes (0.4-3 km): queried only, so the forest can be set on them.
	if (UStaticMeshComponent* Near = Site(TEXT("SM_ECX_Ridge_Near")))
	{
		RidgeMesh = Near;
		Near->bAffectDistanceFieldLighting = false;
		Near->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Near->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	if (UStaticMeshComponent* T = Site(TEXT("SM_ECX_SiteTerrain")))
	{
		TerrainMesh = T;
		if (UMaterialInterface* Parent = Ecx::LoadMat(TEXT("M_ECX_Terrain")))
		{
			UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Parent, this);
			HourMids.Add(M);
			SnowMids.AddUnique(M);
			SetSlot(T, SlotOf(T, 0, TEXT("ecx_terrain")), M);
		}
		// Queries only (the trees and the forest are set on it); its own triangles where the setup marked them so.
		T->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		if (!CollidesComplex(T->GetStaticMesh()))
		{
			T->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
	}
	else
	{
		// The stand-in: the bench behind the resort, the shelves either side, the drop beyond the parapet to the valley.
		auto Snowy = [](UStaticMeshComponent* C) {
			C->ComponentTags.Add(TEXT("Snow"));
			C->bAffectDistanceFieldLighting = false;
		};
		Snowy(Slope(Snow, -70000.0, -3000.0, -60000.0, 60000.0, 18000.0, 0.0, 3000.0, true));
		for (const double S : {-1.0, 1.0})
		{
			const double Y0 = S > 0 ? 3350.0 : -60000.0;
			const double Y1 = S > 0 ? 60000.0 : -3350.0;
			Snowy(Box(Snow, FVector(-3000.0, Y0, -400.0), FVector(2125.0, Y1, ZLane - 2.0), true));
			Snowy(Slope(Snow, 2125.0, XBay1, Y0, Y1, ZLane - 2.0, ZRoad - 2.0, 400.0, true));
			// Beyond the arms, the shoulder strip to the road's far side.
			Snowy(Box(Snow, FVector(XBay1, S > 0 ? 6000.0 : -60000.0, -2400.0), FVector(XParapet, S > 0 ? 60000.0 : -20000.0, ZRoad - 2.0), false));
		}
		UMaterialInterface* Drop = Mat(TEXT("DropSnow"), 0xd8dee8, 0.7f);
		Snowy(Slope(Drop, XParapet + 45.0, 30000.0, -60000.0, 60000.0, -250.0, -18000.0, 1500.0, false));
		Slope(Mat(TEXT("ForestFloor"), 0x1c241e, 0.95f), 30000.0, 250000.0, -250000.0, 250000.0, -18000.0, -90000.0, 4000.0, false)->bAffectDistanceFieldLighting = false;
	}
	// Rocks: the cut slopes at the building's ends, the spur where the road bends out of sight, boulders on the slope.
	struct FRock
	{
		const TCHAR* Name;
		FVector2D At;
		double Z;
		FVector Size;
	};
	FDice Dice(5150);
	TArray<FRock> Rocks = {{TEXT("SM_ECX_Rock_A"), FVector2D(300.0, -6600.0), -1.0, FVector(14.0, 8.0, 6.0)}, {TEXT("SM_ECX_Rock_A"), FVector2D(-400.0, 6300.0), -1.0, FVector(14.0, 8.0, 6.0)},
		{TEXT("SM_ECX_Rock_A"), FVector2D(4200.0, -16500.0), -1.0, FVector(14.0, 8.0, 6.0)}, {TEXT("SM_ECX_Rock_B"), FVector2D(3950.0, 1850.0), ZLandA, FVector(5.0, 4.0, 2.5)},
		{TEXT("SM_ECX_Rock_B"), FVector2D(7600.0, -2500.0), -1.0, FVector(5.0, 4.0, 2.5)}, {TEXT("SM_ECX_Rock_B"), FVector2D(7400.0, 3500.0), -1.0, FVector(5.0, 4.0, 2.5)}};
	for (int32 K = 0; K < 12; ++K)
	{
		const bool bDrop = K < 8;
		const FVector2D At = bDrop ? FVector2D(Dice.Range(6800.0, 12000.0), Dice.Range(-9000.0, 9000.0)) : FVector2D(Dice.Range(800.0, 5000.0), (K % 2 ? 1.0 : -1.0) * Dice.Range(3900.0, 4300.0));
		Rocks.Add({TEXT("SM_ECX_Rock_C"), At, -1.0, FVector(3.0, 2.0, 1.0)});
	}
	UMaterialInterface* Granite = Mat(TEXT("Rock"), 0x6c6a66, 0.8f);
	for (const FRock& R : Rocks)
	{
		const double Z = R.Z == -1.0 ? GroundZ(R.At.X, R.At.Y) : R.Z;
		const FVector At(R.At.X, R.At.Y, Z);
		const float Yaw = static_cast<float>(Dice.Range(0.0, 360.0));
		if (UStaticMeshComponent* C = Prop(R.Name, At, Yaw))
		{
			SetSlot(C, SlotOf(C, 1), Snow);
		}
		else
		{
			UStaticMeshComponent* C2 = NewPart<UStaticMeshComponent>();
			C2->SetStaticMesh(SphereMesh);
			C2->SetMaterial(0, Granite);
			C2->SetRelativeLocationAndRotation(At, FRotator(Dice.Range(-10.0, 10.0), Yaw, 0.0f));
			C2->SetRelativeScale3D(R.Size * FVector(1.0, 1.0, 1.4));
			C2->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void AEmbercrestStage::BuildTrees()
{
	// The conifers' own materials need their needle atlas; without it the trees stand in as cones.
	UMaterialInterface* BarkParent = Ecx::LoadMat(TEXT("M_ECX_Bark"));
	UMaterialInterface* NeedleParent = Ecx::LoadMat(TEXT("M_ECX_Conifer"));
	UTexture* Atlas = Ecx::LoadTex(TEXT("ECX"), TEXT("T_ECX_Needles_BaseColor"));
	const bool bRealTrees = NeedleParent && Atlas;
	UMaterialInstanceDynamic* Bark = BarkParent ? UMaterialInstanceDynamic::Create(BarkParent, this) : nullptr;
	UMaterialInstanceDynamic* Needles = bRealTrees ? UMaterialInstanceDynamic::Create(NeedleParent, this) : nullptr;
	// The atlases set here too (the materials may have been built before they were imported).
	if (Needles)
	{
		Needles->SetTextureParameterValue(TEXT("NeedleBase"), Atlas);
		if (UTexture* N = LoadTex(TEXT("ECX"), TEXT("T_ECX_Needles_Normal")))
		{
			Needles->SetTextureParameterValue(TEXT("NeedleNormal"), N);
		}
		if (UTexture* M = LoadTex(TEXT("ECX"), TEXT("T_ECX_Needles_Mask")))
		{
			Needles->SetTextureParameterValue(TEXT("NeedleMask"), M);
		}
		HourMids.Add(Needles);
	}
	if (Bark)
	{
		const TCHAR* const Maps[3][2] = {{TEXT("T_ECX_Bark_BaseColor"), TEXT("BarkBase")}, {TEXT("T_ECX_Bark_Normal"), TEXT("BarkNormal")}, {TEXT("T_ECX_Bark_ORM"), TEXT("BarkOrm")}};
		for (const auto& Map : Maps)
		{
			if (UTexture* T = LoadTex(TEXT("ECX"), Map[0]))
			{
				Bark->SetTextureParameterValue(Map[1], T);
			}
		}
	}
	UMaterialInterface* ConeGreen = Mat(TEXT("Spruce"), 0x15241b, 0.85f);
	UMaterialInterface* Trunk = Mat(TEXT("Trunk"), 0x3b2d24, 0.9f);
	auto Tree = [&](const TCHAR* Name, const FVector& At, float Yaw, float Scale, float Height, float Radius) {
		if (bRealTrees)
		{
			if (UStaticMeshComponent* C = Prop(Name, At, Yaw, FVector(Scale)))
			{
				SetSlot(C, SlotOf(C, 0, TEXT("ecx_bark")), Bark);
				SetSlot(C, SlotOf(C, 1, TEXT("ecx_needles")), Needles);
				return;
			}
		}
		Cyl(Trunk, At, Radius * 0.08f * Scale, Height * 0.25f * Scale);
		Cone(ConeGreen, At + FVector(0.0, 0.0, Height * 0.12f * Scale), Radius * Scale, Height * 0.88f * Scale);
	};
	// The island beds' and the wings' spruces, the young ones in the planters and the north bed.
	for (const FVector2D& P : {FVector2D(2950.0, 2000.0), FVector2D(2950.0, -2000.0), FVector2D(4250.0, 2250.0), FVector2D(4250.0, -2250.0), FVector2D(420.0, 4800.0), FVector2D(420.0, -4800.0)})
	{
		Tree(TEXT("SM_ECX_Spruce_Mid"), FVector(P.X, P.Y, PlazaZ(P.X, P.Y) + (P.X > 2000.0 ? 20.0 : 15.0)), static_cast<float>(FMath::Fmod(P.X * 0.37 + P.Y * 0.11, 360.0)), 1.0f, 1200.0f, 275.0f);
	}
	const FVector Planters[7] = {FVector(4990.0, 1300.0, ZPlatform), FVector(4990.0, 1900.0, ZPlatform), FVector(4990.0, -2000.0, ZPlatform), FVector(460.0, 2000.0, 0.0),
		FVector(460.0, -2000.0, 0.0), FVector(2000.0, 1860.0, 0.0), FVector(2000.0, -1860.0, 0.0)};
	for (const FVector& P : Planters)
	{
		Tree(TEXT("SM_ECX_Spruce_Young"), P + FVector(0.0, 0.0, 75.0), static_cast<float>(FMath::Fmod(P.Y * 0.7, 360.0)), 1.0f, 450.0f, 110.0f);
	}
	Tree(TEXT("SM_ECX_Spruce_Young"), FVector(4450.0, 1750.0, ZLandA + 20.0), 40.0f, 1.0f, 450.0f, 110.0f);
	Tree(TEXT("SM_ECX_Spruce_Young"), FVector(3800.0, 2400.0, ZLandA + 20.0), 130.0f, 1.0f, 450.0f, 110.0f);
	// The stands beyond the arms' banks and behind the wings, and the screens hiding the road's ends: 70, 60% spruce.
	FDice Dice(70);
	TArray<FVector2D> Placed;
	auto Clear = [&Placed](const FVector2D& P) {
		for (const FVector2D& Q : Placed)
		{
			if (FVector2D::DistSquared(P, Q) < 450.0 * 450.0)
			{
				return false;
			}
		}
		return true;
	};
	int32 Planted = 0;
	for (int32 Try = 0; Try < 2000 && Planted < 70; ++Try)
	{
		FVector2D P;
		const double Pick = Dice.Next();
		if (Planted < 40)
		{
			P = Pick < 0.75 ? FVector2D(Dice.Range(500.0, 5200.0), (Dice.Next() < 0.5 ? -1.0 : 1.0) * Dice.Range(3900.0, 9000.0))
							: FVector2D(Dice.Range(-3000.0, 500.0), (Dice.Next() < 0.5 ? -1.0 : 1.0) * Dice.Range(6000.0, 9000.0));
		}
		else
		{
			P = Pick < 0.55 ? FVector2D(Dice.Range(4000.0, 5400.0), Dice.Range(-16000.0, -9000.0)) : FVector2D(Dice.Range(4000.0, 5400.0), Dice.Range(6000.0, 11000.0));
		}
		if (!Clear(P))
		{
			continue;
		}
		Placed.Add(P);
		++Planted;
		const bool bSpruce = Dice.Next() < 0.6;
		const float Scale = static_cast<float>(Dice.Range(0.85, 1.15));
		const FVector At(P.X, P.Y, GroundZ(P.X, P.Y) - 10.0);
		Tree(bSpruce ? TEXT("SM_ECX_Spruce_Tall") : TEXT("SM_ECX_Pine_Tall"), At, static_cast<float>(Dice.Range(0.0, 360.0)), Scale, bSpruce ? 2000.0f : 1700.0f, bSpruce ? 325.0f : 300.0f);
	}
	// The forest on every slope out to 2.5 km, clumps of conifers as instances (sparse down the drop, none in the valley's
	// view, the axis or the peak's window).
	UStaticMesh* ClumpA = Ecx::LoadMesh(TEXT("SM_ECX_ForestClump_A"));
	UStaticMesh* ClumpB = Ecx::LoadMesh(TEXT("SM_ECX_ForestClump_B"));
	UInstancedStaticMeshComponent* SetA = nullptr;
	UInstancedStaticMeshComponent* SetB = nullptr;
	if (ClumpA)
	{
		SetA = Instances(ClumpA, nullptr, true);
	}
	if (ClumpB)
	{
		SetB = Instances(ClumpB, nullptr, true);
	}
	UInstancedStaticMeshComponent* Cones = (!SetA && !SetB) && ConeMesh ? Instances(ConeMesh, ConeGreen, true) : nullptr;
	const FBox ConeBox = ConeMesh ? ConeMesh->GetBoundingBox() : FBox(FVector(-50.0), FVector(50.0));
	const FVector2D Platform(5000.0, 0.0);
	FDice Forest(2500);
	for (int32 I = 0; I < 2600; ++I)
	{
		const double R = 6000.0 * FMath::Pow(250000.0 / 6000.0, Forest.Next());
		const double A = Forest.Range(0.0, 2.0 * UE_DOUBLE_PI);
		const FVector2D P(R * FMath::Cos(A), R * FMath::Sin(A));
		if (P.X > 6500.0 && (P.X > 40000.0 || Forest.Next() > 0.15))
		{
			continue; // the valley's view
		}
		if (P.X < -3000.0 && FMath::Abs(P.Y) < 5000.0 && P.X > -80000.0)
		{
			continue; // the axis east of the building, toward the massif
		}
		if (P.X > -6500.0 && P.X < 7500.0 && FMath::Abs(P.Y) < 6500.0)
		{
			continue; // the resort, the plaza and the road
		}
		const FVector2D From = P - Platform;
		const double Bearing = FMath::RadiansToDegrees(FMath::Atan2(From.Y, From.X));
		const double Yaw = Bearing < 0.0 ? Bearing + 360.0 : Bearing;
		if (Yaw > 174.0 && Yaw < 194.0)
		{
			continue; // the peak's window from the platform
		}
		const double Z = GroundZ(P.X, P.Y);
		const float Scale = static_cast<float>(Forest.Range(0.8, 1.3));
		const FRotator Turn(0.0f, static_cast<float>(Forest.Range(0.0, 360.0)), 0.0f);
		if (SetA || SetB)
		{
			UInstancedStaticMeshComponent* Set = (SetA && (!SetB || Forest.Next() < 0.6)) ? SetA : SetB;
			Set->AddInstance(FTransform(Turn, FVector(P.X, P.Y, Z - 30.0), FVector(Scale)));
		}
		else if (Cones)
		{
			// A clump as one cone 12 m across and 18 m tall, fitted by the engine shape's bounds.
			const FVector Ext = ConeBox.GetExtent();
			const FVector S(600.0 * Scale / FMath::Max(Ext.X, 1.0), 600.0 * Scale / FMath::Max(Ext.Y, 1.0), 1800.0 * Scale / FMath::Max(2.0 * Ext.Z, 1.0));
			const FVector Mid = ConeBox.GetCenter();
			Cones->AddInstance(FTransform(Turn, FVector(P.X, P.Y, Z) - FVector(Mid.X * S.X, Mid.Y * S.Y, (Mid.Z - Ext.Z) * S.Z), S));
		}
	}
}

void AEmbercrestStage::BuildFar()
{
	// The far world: the near bench, the Crest massif, the ranges beyond, the valley and the city in its lights, the mist.
	auto Far = [](UStaticMeshComponent* C, bool bShadows) {
		if (!C)
		{
			return;
		}
		C->bRayTracingFarField = true;
		C->bAffectDistanceFieldLighting = false;
		C->SetCastShadow(bShadows);
		C->bCastFarShadow = bShadows;
	};
	UStaticMeshComponent* Mid = Site(TEXT("SM_ECX_Ridge_Mid"));
	Far(Mid, true);
	Far(Site(TEXT("SM_ECX_Ridge_Far")), false);
	UStaticMeshComponent* Valley = Site(TEXT("SM_ECX_Valley"));
	Far(Valley, false);
	UStaticMeshComponent* City = Site(TEXT("SM_ECX_ValleyCity"));
	Far(City, false);
	// Ten kilometres of fields and streets seen from 900 m up have no sheen: the importer's specular turned the city's
	// flat sheet into a mirror of the sky (a blue slab by day, an orange one at dusk, with the sun's glint in it).
	for (UStaticMeshComponent* Flat : {Valley, City})
	{
		for (int32 I = 0; Flat && I < Flat->GetNumMaterials(); ++I)
		{
			if (UMaterialInterface* M = Flat->GetMaterial(I))
			{
				UMaterialInstanceDynamic* Dull = Cast<UMaterialInstanceDynamic>(M);
				if (!Dull)
				{
					Dull = UMaterialInstanceDynamic::Create(M, this);
					Flat->SetMaterial(I, Dull);
				}
				Dull->SetScalarParameterValue(TEXT("SpecularFactor"), 0.0f);
			}
		}
	}
	if (City)
	{
		// The city's lights carry 900 m up the valley: three times the windows' level.
		Lit(City, {0.0f}, 3.0f);
	}
	if (UStaticMeshComponent* Mist = Site(TEXT("SM_ECX_ValleyMist")))
	{
		if (UMaterialInterface* Parent = Ecx::LoadMat(TEXT("M_ECX_Mist")))
		{
			UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Parent, this);
			M->SetScalarParameterValue(TEXT("DepthFade"), 50000.0f);
			HourMids.Add(M);
			SetSlot(Mist, SlotOf(Mist, 0, TEXT("ecx_mist")), M);
		}
		Mist->SetCastShadow(false);
		Mist->bAffectDistanceFieldLighting = false;
		Mist->SetVisibleInRayTracing(false);
	}
	if (UStaticMeshComponent* SlopeLights = Site(TEXT("SM_ECX_SlopeLights_GlowWarm")))
	{
		GlowAll(SlopeLights, GlowSlope);
		SlopeLights->bRayTracingFarField = true;
	}
	if (!Mid)
	{
		// The Crest as three great cones in the mark's proportions, flanking ridges, high peaks behind, the far valley wall.
		UMaterialInterface* Peak = Mat(TEXT("PeakSnow"), 0xd6dce6, 0.75f);
		UMaterialInterface* Ridge = Mat(TEXT("RidgeDark"), 0x39424c, 0.85f);
		struct FPeak
		{
			FVector Base;
			float Radius;
			float Height;
			bool bSnow;
		};
		const FPeak Peaks[] = {{FVector(-350000.0, -25000.0, 0.0), 210000.0f, 135000.0f, true}, {FVector(-330000.0, 85000.0, 0.0), 150000.0f, 94000.0f, true},
			{FVector(-370000.0, -131000.0, 0.0), 160000.0f, 103500.0f, true}, {FVector(-420000.0, 330000.0, 0.0), 220000.0f, 80000.0f, false},
			{FVector(-430000.0, -380000.0, 0.0), 230000.0f, 85000.0f, false}, {FVector(-500000.0, 600000.0, 0.0), 260000.0f, 70000.0f, false},
			{FVector(-520000.0, -640000.0, 0.0), 260000.0f, 75000.0f, false}, {FVector(-1300000.0, 250000.0, 0.0), 500000.0f, 220000.0f, true},
			{FVector(-1500000.0, -500000.0, 0.0), 550000.0f, 240000.0f, true}, {FVector(-1100000.0, -1300000.0, 0.0), 450000.0f, 160000.0f, true},
			{FVector(2200000.0, -400000.0, -90000.0), 600000.0f, 110000.0f, false}, {FVector(2400000.0, 700000.0, -90000.0), 650000.0f, 120000.0f, false},
			{FVector(2600000.0, 1600000.0, -90000.0), 700000.0f, 140000.0f, false}};
		for (const FPeak& P : Peaks)
		{
			Far(Cone(P.bSnow ? Peak : Ridge, P.Base, P.Radius, P.Height), false);
		}
	}
	if (!Ecx::LoadMesh(TEXT("SM_ECX_Valley")))
	{
		UStaticMeshComponent* Floor = Box(Mat(TEXT("ValleyFloor"), 0x1e2a20, 0.95f), FVector(250000.0, -3000000.0, -90400.0), FVector(3000000.0, 3000000.0, -90000.0), false);
		Far(Floor, false);
	}
	if (!City)
	{
		// The city as the street's skyline draws it (M_StreetCity): blocks with lit windows on the valley floor.
		if (UMaterialInterface* CityMat = Ecx::LoadMat(TEXT("M_StreetCity")))
		{
			UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(CityMat, this);
			M->SetScalarParameterValue(TEXT("Strength"), 1.0f);
			HourMids.Add(M);
			UInstancedStaticMeshComponent* Blocks = Instances(CubeMesh, M, false);
			Blocks->bRayTracingFarField = true;
			Blocks->bAffectDistanceFieldLighting = false;
			FDice Dice(1600);
			for (int32 I = 0; I < 900; ++I)
			{
				const double X = Dice.Range(600000.0, 1600000.0);
				const double Y = Dice.Range(-700000.0, 900000.0);
				// Denser and taller downtown, round (10.5 km, 1 km).
				const double Down = FMath::Exp(-FVector2D::DistSquared(FVector2D(X, Y), FVector2D(1050000.0, 100000.0)) / (300000.0 * 300000.0));
				if (Dice.Next() > 0.25 + 0.75 * Down)
				{
					continue;
				}
				const double W = Dice.Range(4000.0, 14000.0);
				const double H = 1500.0 + Dice.Next() * Dice.Next() * (4000.0 + 16000.0 * Down);
				Blocks->AddInstance(FTransform(FRotator(0.0f, static_cast<float>(Dice.Range(0.0, 90.0)), 0.0f), FVector(X, Y, -90000.0 + H * 0.5), FVector(W / 100.0, W / 100.0, H / 100.0)));
			}
		}
	}
}

// ------------------------------------------------------------------ what blocks (spec 2.8)

void AEmbercrestStage::BuildBlockers()
{
	// The building: the entry's glass line (the drum stands out of it), the piers and the wings. They stop the camera too:
	// coming out of the doors the hero is 2.2 m from the glass and the arm is 3 m long.
	Block(FVector(-3000.0, -1600.0, -100.0), FVector(10.0, 1600.0, 1300.0), 0.0f, nullptr, true);
	Block(FVector(-3000.0, -170.0, -100.0), FVector(150.0, 170.0, 330.0), 0.0f, nullptr, true);
	Block(FVector(-3000.0, 1600.0, -100.0), FVector(300.0, 5800.0, 1700.0), 0.0f, nullptr, true);
	Block(FVector(-3000.0, -5800.0, -100.0), FVector(300.0, -1600.0, 1700.0), 0.0f, nullptr, true);
	// The canopy's columns.
	for (const FVector2D& C : CanopyColumns)
	{
		Block(FVector(C.X - 70.0, C.Y - 70.0, 0.0), FVector(C.X + 70.0, C.Y + 70.0, 700.0));
	}
	// The cascades: no stepping onto the copings.
	for (const double S : {-1.0, 1.0})
	{
		const double Y0 = S > 0 ? 460.0 : -1490.0;
		const double Y1 = S > 0 ? 1490.0 : -460.0;
		Block(FVector(2520.0, Y0, ZLandB - 100.0), FVector(3560.0, Y1, ZLandB + 120.0));
		Block(FVector(3560.0, Y0, ZLandA - 100.0), FVector(4640.0, Y1, ZLandA + 120.0));
	}
	// The set's open edges: pawns only, 4 m over the ground (spec 2.8's bounds).
	for (const double S : {-1.0, 1.0})
	{
		PawnWall(FVector(300.0, S > 0 ? 3700.0 : -3740.0, -800.0), FVector(XPlatform1, S > 0 ? 3740.0 : -3700.0, 500.0));
		PawnWall(FVector(XPlatform1 - 20.0, S > 0 ? 3700.0 : -4800.0, -800.0), FVector(XPlatform1 + 20.0, S > 0 ? 4800.0 : -3700.0, 500.0));
		PawnWall(FVector(XPlatform1, S > 0 ? 4800.0 : -4840.0, -800.0), FVector(XParapet, S > 0 ? 4840.0 : -4800.0, 500.0));
	}
	// The overlook's mouth stays open; its other sides are the parapet (BuildFixtures).
}
