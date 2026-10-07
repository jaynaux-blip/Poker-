// Fifth's plots and the street's ends (world realism spec 2.2, 2.3, 4.4, ue-world): the building kit (ns_fronts.py:
// SM_NS_Bldg_<Plot> with its _Trim, _Glass, _Glow<Family> and _Grille) on every plot of Fifth that was an engine box, each
// plot's shop lit by its hours (its glows, its lit displays, its spill on the sidewalk, its grille rolled up while open);
// the two T intersections, Hollis and Wexler running across Fifth's ends, closed by Hollis Savings Bank (its clock keeping
// the world's time) and the Rialto (its chaser bulbs running in the evening), lined by stand-in fronts (SM_NS_Proxy_A..H)
// out to 120 m, where the haze takes them; the walk limits' closures and the curbs' blocking (spec 2.9). Every kit mesh
// has a stand-in of engine shapes in the same plan.
#include "StreetStage.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ShopStage.h"

namespace StreetFrontsDetail
{
const float PatBrick = 1.0f;
const float PatConcrete = 5.0f;
const float PatShutter = 6.0f;

/** One of Fifth's plots (spec 2.3): its kit id, its footprint and height (stage cm), its brick for the stand-in, its shop. */
struct FPlot
{
	const TCHAR* Id;
	double X0;
	double X1;
	double Y0;
	double Y1;
	double Height;
	uint32 Brick;
	/** Its cross-street face (Y; 0 none): the corner plots' returns on Hollis or Wexler. */
	double CrossY;
	/** Only with Market at its new width (Ruby's, the wireless shop: today's Market keeps the block there). */
	bool bWideOnly;
	/** Its ground storey is a shop stage's (Ruby's): nothing of it blocks, the stage's door and walls do. */
	bool bShopStage;
	/** Its shop in core's table (shop::Venues: its real hours, Sundays and all), or null: Hours. */
	const TCHAR* Venue;
	Ns::FHours Hours;
	/** Its neon lit all night whatever the hours (QuikCash's blue tube). */
	bool bNeonAllNight;
	uint32 NeonHex;
	uint32 CoolHex;
	/** Its spill on the sidewalk: where along its street face (Y on Fifth, X on a cross street with SpillCross), how wide, its color and lumens. */
	double SpillAt;
	double SpillWidth;
	uint32 SpillHex;
	float SpillLumens;
	bool bSpillOnCross;
};
const FPlot Plots[] = {
	{TEXT("HHollis"), -1800.0, 0.0, -6200.0, -3600.0, 1550.0, 0x9c8160, -6200.0, false, false, TEXT("hardware"), {480, 1080, 0x3f}, false, 0, 0, -700.0, 700.0, 0xfff2dc, 700.0f, true},
	{TEXT("HKim"), -1500.0, 0.0, -3600.0, -2150.0, 1750.0, 0x7a3a2a, 0.0, false, false, TEXT("kims"), {420, 1260, 0x7f}, false, 0, 0, -2875.0, 900.0, 0xfff4e4, 800.0f, false},
	{TEXT("HCash"), -1500.0, 0.0, 800.0, 2300.0, 1100.0, 0xb9b2a3, 0.0, false, false, TEXT("quikcash"), {480, 1260, 0x3f}, true, 0x2a6bff, 0xe8f0ff, 1550.0, 900.0, 0xe8f0ff, 600.0f, false},
	{TEXT("Ruby"), -1500.0, 0.0, 6950.0, 8000.0, 1450.0, 0x9c8160, 0.0, true, true, TEXT("diner"), {0, 0, 0x7f}, false, 0, 0, 0.0, 0.0, 0, 0.0f, false},
	{TEXT("HWexler"), -1500.0, 0.0, 8000.0, 9200.0, 1750.0, 0x7a3a2a, 9200.0, true, false, TEXT("wireless"), {600, 1260, 0x7f}, false, 0, 0x6aa8ff, 8600.0, 700.0, 0xe8f0ff, 600.0f, false},
	{TEXT("FHollis"), 2040.0, 3840.0, -6200.0, -4300.0, 1500.0, 0x2f2420, -6200.0, false, false, TEXT("golden-dragon"), {660, 120, 0x7f}, false, 0xff3020, 0, -5600.0, 600.0, 0xffd6b4, 700.0f, false},
	{TEXT("FLobby"), 2040.0, 3840.0, -4300.0, -2300.0, 2000.0, 0x9c8160, 0.0, false, false, nullptr, {0, 0, 0x7f}, false, 0, 0, -3300.0, 400.0, 0xffe0b8, 500.0f, false},
	{TEXT("FBotanica"), 2040.0, 3600.0, -2300.0, -880.0, 1750.0, 0x7a3a2a, 0.0, false, false, TEXT("botanica"), {600, 1140, 0x7f}, false, 0, 0, -1600.0, 600.0, 0xffc890, 500.0f, false},
	{TEXT("FRecords"), 2040.0, 3540.0, 4745.0, 6300.0, 900.0, 0x7a3a2a, 0.0, false, false, TEXT("records"), {720, 1260, 0x7f}, false, 0xff3a8a, 0, 5520.0, 800.0, 0xfff0e0, 700.0f, false},
	// Its lobby is lit all night (the dental office's hours are its upper floors' windows').
	{TEXT("FWexler"), 2040.0, 3840.0, 6300.0, 9200.0, 2300.0, 0x9c8160, 9200.0, false, false, nullptr, {0, 0, 0x7f}, false, 0, 0, 7750.0, 400.0, 0xffe0b8, 500.0f, false},
};

/** The stand-in fronts' letters (spec 4.4's proxies; their widths, floors and bricks are Ns::ProxyWidths and the rest). */
const TCHAR* const ProxyLetters = TEXT("ABCDEFGH");

/** The bank's clock's pivot when the art's places aren't known (ns_fronts.py: the dial's middle 8 cm out of the attic at 15.45 m; the hands 1 and 3.5 cm before it). */
const FVector BankHourPivot(1020.0, Ns::HollisS + 9.0, 1545.0);
const FVector BankMinutePivot(1020.0, Ns::HollisS + 11.5, 1545.0);

FString KitName(const TCHAR* Id, const TCHAR* Part)
{
	return FString::Printf(TEXT("SM_NS_Bldg_%s%s"), Id, Part);
}
} // namespace StreetFrontsDetail

using namespace StreetFrontsDetail;

void AStreetStage::AppendFrontsPreload(TArray<FString>& Paths)
{
	for (const FPlot& P : Plots)
	{
		for (const TCHAR* Part : {TEXT(""), TEXT("_Trim"), TEXT("_Glass"), TEXT("_GlowWarm"), TEXT("_GlowCool"), TEXT("_GlowNeon"), TEXT("_Grille")})
		{
			Paths.Add(FStreetPreload::MeshPath(*KitName(P.Id, Part)));
		}
	}
	for (const TCHAR* Name : {TEXT("SM_NS_Vista_Bank"), TEXT("SM_NS_Vista_Bank_Trim"), TEXT("SM_NS_Vista_Bank_Glass"), TEXT("SM_NS_Vista_Bank_GlowWarm"),
			 TEXT("SM_NS_Vista_Bank_Hour"), TEXT("SM_NS_Vista_Bank_Minute"), TEXT("SM_NS_Vista_Rialto"), TEXT("SM_NS_Vista_Rialto_Trim"), TEXT("SM_NS_Vista_Rialto_Glass"),
			 TEXT("SM_NS_Vista_Rialto_GlowWarm"), TEXT("SM_NS_Vista_Rialto_GlowNeon"), TEXT("SM_NS_Vista_Rialto_Marquee"), TEXT("SM_NS_Vista_Rialto_Posters"),
			 TEXT("SM_NS_SidewalkClosure"), TEXT("SM_NS_Barricade"), TEXT("SM_NS_Cone"), TEXT("SM_NS_JerseyBarrier")})
	{
		Paths.Add(FStreetPreload::MeshPath(Name));
	}
	for (int32 K = 0; K < 8; ++K)
	{
		Paths.Add(FStreetPreload::MeshPath(*FString::Printf(TEXT("SM_NS_Proxy_%c"), ProxyLetters[K])));
	}
	Paths.Add(Ns::GlowPath);
	Paths.Add(Ns::GlassPath);
	Paths.Add(Ns::WindowPath);
}

int32 AStreetStage::AddGlow(UStaticMeshComponent* Mesh, Ns::EGlow Family, uint32 Hex, float Scale, int32 Shop)
{
	if (!Mesh)
	{
		return INDEX_NONE;
	}
	// The kit's glow meshes take M_NS_Glow (a color and a level); anything else glows by its own emissive, scaled.
	const UStaticMesh* Source = Mesh->GetStaticMesh();
	const bool bKitGlow = Source && Source->GetName().Contains(TEXT("_Glow"));
	UMaterialInterface* Base = bKitGlow ? Cast<UMaterialInterface>(NsAsset(Ns::GlowPath)) : nullptr;
	if (bKitGlow)
	{
		Mesh->SetCastShadow(false);
	}
	// An open shop's lightbox and neon show by day too; lamps, sconces and floods only after dusk.
	const bool bDay = Shop >= 0 && Family != Ns::EGlow::Warm;
	return Lit.AddGlow(this, Mesh, Base, Family, Ns::Srgb(Hex ? Hex : 0xffd9a0), Scale, Shop, bDay);
}

URectLightComponent* AStreetStage::Spill(const FVector& At, float Yaw, float Width, float Height, uint32 Hex, float Lumens, int32 Shop)
{
	URectLightComponent* L = NewPart<URectLightComponent>();
	// Out of the glass and down onto the sidewalk (35 degrees), set back so its top edge stays behind the pane.
	L->SetRelativeLocationAndRotation(At, FRotator(-35.0f, Yaw, 0.0f));
	L->SetIntensityUnits(ELightUnits::Lumens);
	L->SetIntensity(Lumens);
	L->SetSourceWidth(Width);
	L->SetSourceHeight(Height);
	L->SetAttenuationRadius(900.0f);
	L->SetLightColor(Ns::Srgb(Hex));
	// The window already shows in the puddles: its light's own reflection kept faint (as the Wash & Fold's spill).
	L->SetSpecularScale(0.25f);
	L->SetCastShadows(false);
	L->SetVolumetricScatteringIntensity(0.0f);
	// Dropped beyond 40 m (spec 3.1's budget).
	L->MaxDrawDistance = 4000.0f;
	L->MaxDistanceFadeRange = 600.0f;
	L->MarkRenderStateDirty();
	Lit.AddLight(Shop, L, Lumens);
	return L;
}

void AStreetStage::StandInBlock(const FVector& Min, const FVector& Max, uint32 Brick, int32 Seed, bool bFacesX, bool bFacesY)
{
	UMaterialInterface* M = Mat(*FString::Printf(TEXT("StandIn%06x"), Brick), Brick, 0.85f, PatBrick);
	Box(M, Min, Max);
	const FVector2D Win(120.0, 170.0);
	const double Top = Max.Z;
	if (bFacesX)
	{
		WindowRows(FVector(Max.X + 0.5, Min.Y, 0.0), FVector(0.0, 1.0, 0.0), Max.Y - Min.Y, 0.0f, 420.0, Top, Win, 260.0, 330.0, 0.28, Seed * 7919 + 1);
		WindowRows(FVector(Min.X - 0.5, Min.Y, 0.0), FVector(0.0, 1.0, 0.0), Max.Y - Min.Y, 180.0f, 420.0, Top, Win, 260.0, 330.0, 0.28, Seed * 7919 + 2);
	}
	if (bFacesY)
	{
		WindowRows(FVector(Min.X, Max.Y + 0.5, 0.0), FVector(1.0, 0.0, 0.0), Max.X - Min.X, 90.0f, 420.0, Top, Win, 260.0, 330.0, 0.28, Seed * 7919 + 3);
		WindowRows(FVector(Min.X, Min.Y - 0.5, 0.0), FVector(1.0, 0.0, 0.0), Max.X - Min.X, -90.0f, 420.0, Top, Win, 260.0, 330.0, 0.28, Seed * 7919 + 4);
	}
}

void AStreetStage::Proxy(const FVector& At, float Yaw, double Width, int32 Seed)
{
	const int32 K = ((Seed % 8) + 8) % 8;
	const bool bMirror = (Seed / 8) % 2 == 1;
	// Fitted to its run (Ns::ProxyPlan squeezes or stretches it a little), mirrored every other one.
	const double Fit = Width / Ns::ProxyWidths[K];
	if (NsProp(*FString::Printf(TEXT("SM_NS_Proxy_%c"), ProxyLetters[K]), At, Yaw, FVector(1.0, (bMirror ? -1.0 : 1.0) * Fit, 1.0)))
	{
		return;
	}
	// A stand-in: its massing 12 m deep behind its front, windows on its front.
	const FVector Out = FRotator(0.0f, Yaw, 0.0f).Vector();
	const FVector Along = FRotator(0.0f, Yaw + 90.0f, 0.0f).Vector();
	const FVector A = At - Along * Width * 0.5;
	const FVector B = At + Along * Width * 0.5 - Out * Ns::ProxyDepth;
	const double H = 420.0 + 320.0 * (Ns::ProxyFloors[K] - 1) + 80.0;
	UMaterialInterface* M = Mat(*FString::Printf(TEXT("StandIn%06x"), Ns::ProxyBricks[K]), Ns::ProxyBricks[K], 0.85f, PatBrick);
	Box(M, FVector(FMath::Min(A.X, B.X), FMath::Min(A.Y, B.Y), 0.0), FVector(FMath::Max(A.X, B.X), FMath::Max(A.Y, B.Y), H), false);
	WindowRows(A + Out * 0.5, Along, Width, Yaw, 420.0, H, FVector2D(120.0, 170.0), 260.0, 320.0, 0.26, Seed * 104729 + 7);
}

void AStreetStage::ProxyRun(const FVector& A, const FVector& B, float Yaw, int32 Seed)
{
	// Fronts side by side from A to B along their building line (each its middle on it), filling it exactly: the letters in
	// a seeded order that never repeats a neighbor, every other one mirrored (Ns::ProxyPlan).
	const FVector Dir = (B - A).GetSafeNormal();
	for (const Ns::FProxySlot& S : Ns::ProxyPlan((B - A).Size(), Seed))
	{
		Proxy(A + Dir * S.Along, Yaw, S.Width, S.Letter + (S.bMirror ? 8 : 0));
	}
}

// ------------------------------------------------------------------ Fifth's plots

void AStreetStage::BuildPlots()
{
	for (int32 I = 0; I < static_cast<int32>(UE_ARRAY_COUNT(Plots)); ++I)
	{
		BuildPlot(I);
	}
}

void AStreetStage::BuildPlot(int32 Index)
{
	const FPlot& P = Plots[Index];
	if (P.bWideOnly && !bWideMarket)
	{
		return;
	}
	const bool bFar = P.X0 >= Ns::FarFront - 1.0;
	const double FaceX = bFar ? Ns::FarFront : Ns::HomeLine;
	const float OutYaw = bFar ? 180.0f : 0.0f;
	const int32 Shop = Lit.AddShop(P.Venue, P.Hours);
	UStaticMeshComponent* Body = Site(*KitName(P.Id, TEXT("")));
	if (Body)
	{
		++PlotsKit;
		if (UStaticMeshComponent* Trim = Site(*KitName(P.Id, TEXT("_Trim"))))
		{
			// Its slot 3 is the lit displays behind the shop glass (DimImportedGlow with the hours).
			Lit.Shops[Shop].Trim = Trim;
			Lit.Shops[Shop].TrimSlot = ImportedSlot(Trim, 3);
		}
		Site(*KitName(P.Id, TEXT("_Glass")));
		AddGlow(Site(*KitName(P.Id, TEXT("_GlowWarm"))), Ns::EGlow::Warm, 0xffc98a, 1.0f, Shop);
		AddGlow(Site(*KitName(P.Id, TEXT("_GlowCool"))), Ns::EGlow::Cool, P.CoolHex ? P.CoolHex : 0xe8f0ff, 1.0f, Shop);
		AddGlow(Site(*KitName(P.Id, TEXT("_GlowNeon"))), Ns::EGlow::Neon, P.NeonHex ? P.NeonHex : 0xff4060, 1.0f, P.bNeonAllNight ? -1 : Shop);
		// The grille rolls up into its hood (its origin is its opening's top middle: scaled in Z), where the art says it goes.
		const FString GrilleName = KitName(P.Id, TEXT("_Grille"));
		if (const FVector* Pivot = Places.Find(FName(*GrilleName)))
		{
			if (UStaticMesh* Mesh = NsMesh(*GrilleName))
			{
				UStaticMeshComponent* G = NewPart<UStaticMeshComponent>();
				G->SetStaticMesh(Mesh);
				G->SetRelativeLocationAndRotation(*Pivot, Ns::SiteRotation());
				G->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Lit.Shops[Shop].Grille = G;
			}
		}
		else if (NsMesh(*GrilleName))
		{
			UE_LOG(LogTemp, Warning, TEXT("StreetStage: %s's pivot isn't known (art/build/ns_fronts_places.json): no grille on %s"), *GrilleName, P.Id);
		}
	}
	else
	{
		// The stand-in: its massing with windows on its street faces, its shop's grille down or its window lit.
		++PlotsStandIn;
		StandInBlock(FVector(P.X0, P.Y0, 0.0), FVector(P.X1, P.Y1, P.Height), P.Brick, Index * 37 + 11, true, P.CrossY != 0.0);
		if (!P.bShopStage)
		{
			const double Y0 = bFar ? P.Y0 + 150.0 : P.Y0 + 150.0;
			const double Y1 = P.Y1 - 150.0;
			const double Sx = bFar ? FaceX - 4.0 : FaceX;
			UMaterialInstanceDynamic* Display = Mat(*FString::Printf(TEXT("Display%d"), Index), P.SpillHex ? P.SpillHex : 0xffd9a0, 0.6f, 0.0f, 0.0f);
			UStaticMeshComponent* Window = Box(Display, FVector(Sx, Y0, 40.0), FVector(Sx + 4.0, Y1, 300.0), false);
			Window->SetCastShadow(false);
			Lit.AddGlow(this, Window, nullptr, Ns::EGlow::Cool, Ns::Srgb(P.SpillHex ? P.SpillHex : 0xffd9a0), 1.0f, Shop, true);
			Box(Mat(TEXT("Shutter"), 0x5d6166, 0.5f, PatShutter, 0.0f, 0.6f), FVector(bFar ? FaceX - 8.0 : FaceX + 4.0, Y0 - 10.0, 300.0), FVector(bFar ? FaceX - 4.0 : FaceX + 8.0, Y1 + 10.0, 330.0), false);
		}
	}
	// The faces block (the kit's meshes don't collide): Fifth's face, and the corner plots' returns on their cross street.
	// A shop stage's ground storey is its own (Ruby's door in its corner).
	const double Z0 = P.bShopStage ? 440.0 : 0.0;
	if (bFar)
	{
		Blocker(FVector(FaceX, P.Y0, Z0), FVector(FaceX + 40.0, P.Y1, P.Height));
	}
	else
	{
		Blocker(FVector(FaceX - 40.0, P.Y0, Z0), FVector(FaceX, P.Y1, P.Height));
	}
	if (P.CrossY != 0.0)
	{
		const bool bSouthFace = P.CrossY < 0.0;
		Blocker(FVector(P.X0, bSouthFace ? P.CrossY : P.CrossY - 40.0, 0.0), FVector(P.X1, bSouthFace ? P.CrossY + 40.0 : P.CrossY, P.Height));
	}
	if (P.bShopStage && !AShopStage::ClassFor(TEXT("diner")))
	{
		// Ruby's stage isn't there yet: its ground storey's front shut (both faces and the corner), so nothing shows inside;
		// out on the faces, never in their plane (a stand-in's faces are there: two faces in one plane flicker).
		UMaterialInterface* Shutter = Mat(TEXT("Shutter"), 0x5d6166, 0.5f, PatShutter, 0.0f, 0.6f);
		Box(Shutter, FVector(0.0, P.Y0, 0.0), FVector(4.0, P.Y1, 440.0));
		Box(Shutter, FVector(P.X0, P.Y0 - 4.0, 0.0), FVector(4.0, P.Y0, 440.0));
	}
	// Its spill on the sidewalk at night, while its shop is open.
	if (P.SpillWidth > 0.0)
	{
		if (P.bSpillOnCross)
		{
			const bool bSouthFace = P.CrossY < 0.0;
			Spill(FVector(P.SpillAt, P.CrossY + (bSouthFace ? -70.0 : 70.0), 260.0), bSouthFace ? -90.0f : 90.0f, static_cast<float>(P.SpillWidth), 180.0f, P.SpillHex,
				P.SpillLumens, Shop);
		}
		else
		{
			Spill(FVector(FaceX + (bFar ? -70.0 : 70.0), P.SpillAt, 260.0), OutYaw, static_cast<float>(P.SpillWidth), 180.0f, P.SpillHex, P.SpillLumens, Shop);
		}
	}
	// Its neon's own light (spec 3.1: each neon or lightbox a rect light its own shape): the tube's color on the sidewalk and in
	// the wet, a little of it in the rain's haze; while it's lit (QuikCash's blue tube all night).
	if (P.NeonHex)
	{
		const int32 NeonShop = P.bNeonAllNight ? Lit.AddShop({0, 0, 0x7f}) : Shop;
		URectLightComponent* L = NewPart<URectLightComponent>();
		L->SetRelativeLocationAndRotation(FVector(FaceX + (bFar ? -45.0 : 45.0), P.SpillAt, 300.0), FRotator(-12.0f, OutYaw, 0.0f));
		L->SetIntensityUnits(ELightUnits::Lumens);
		L->SetIntensity(320.0f);
		L->SetSourceWidth(240.0f);
		L->SetSourceHeight(50.0f);
		L->SetAttenuationRadius(700.0f);
		L->SetLightColor(Ns::Srgb(P.NeonHex));
		L->SetSpecularScale(0.5f);
		L->SetCastShadows(false);
		L->SetVolumetricScatteringIntensity(0.4f);
		L->MaxDrawDistance = 4000.0f;
		L->MaxDistanceFadeRange = 600.0f;
		Lit.AddLight(NeonShop, L, 320.0f);
	}
}

// ------------------------------------------------------------------ the T's

void AStreetStage::BuildCrossStreets()
{
	UMaterialInterface* Limestone = Mat(TEXT("Limestone"), 0xbdb4a2, 0.8f, PatConcrete);
	UMaterialInterface* Bronze = Mat(TEXT("BankBronze"), 0x3a2c1e, 0.35f, 0.0f, 0.0f, 0.8f);
	UMaterialInterface* Dial = Mat(TEXT("BankDial"), 0xece6d4, 0.4f, 0.0f, 0.6f);

	// Hollis Savings Bank across Hollis, facing north up Fifth (spec 2.3, 4.4): its floods at night, its ATM vestibule lit at
	// every hour (its trim's lit slot), its clock showing the world's time (the hands turn about their pivots, UpdateBankClock).
	const int32 Bank = Lit.AddShop({0, 0, 0x7f});
	if (Site(TEXT("SM_NS_Vista_Bank")))
	{
		++PlotsKit;
		if (UStaticMeshComponent* Trim = Site(TEXT("SM_NS_Vista_Bank_Trim")))
		{
			Lit.Shops[Bank].Trim = Trim;
			Lit.Shops[Bank].TrimSlot = ImportedSlot(Trim, 3);
		}
		Site(TEXT("SM_NS_Vista_Bank_Glass"));
		AddGlow(Site(TEXT("SM_NS_Vista_Bank_GlowWarm")), Ns::EGlow::Warm, 0xffd9a8, 1.6f, -1);
	}
	else
	{
		++PlotsStandIn;
		Box(Limestone, FVector(-180.0, Ns::HollisS - 1800.0, 0.0), FVector(2220.0, Ns::HollisS, 1740.0));
		for (const double X : {545.0, 845.0, 1195.0, 1495.0})
		{
			Cyl(Limestone, FVector(X, Ns::HollisS + 60.0, 100.0), 48.0f, 1050.0f, false);
		}
		Box(Bronze, FVector(880.0, Ns::HollisS + 1.0, 100.0), FVector(1160.0, Ns::HollisS + 4.0, 520.0), false);
	}
	auto Hand = [&](const TCHAR* Name, const FVector& Fallback, double Length, double Width) -> USceneComponent* {
		const FVector* Known = Places.Find(FName(Name));
		const FVector Pivot = Known ? *Known : Fallback;
		USceneComponent* Turn = NewPart<USceneComponent>();
		Turn->SetRelativeLocation(Pivot);
		if (UStaticMesh* Mesh = NsMesh(Name))
		{
			UStaticMeshComponent* C = NewPart<UStaticMeshComponent>(Turn);
			C->SetStaticMesh(Mesh);
			C->SetRelativeRotation(Ns::SiteRotation());
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		else
		{
			// Pointing at twelve (up), in the dial's plane (X across, Z up), a hair in front of it.
			Box(Bronze, FVector(-Width * 0.5, 0.0, -12.0), FVector(Width * 0.5, 2.0, Length), false, Turn)->SetCastShadow(false);
		}
		return Turn;
	};
	if (!NsMesh(TEXT("SM_NS_Vista_Bank_Hour")))
	{
		// The stand-in's dial.
		UStaticMeshComponent* Face = NewPart<UStaticMeshComponent>();
		Face->SetStaticMesh(CylinderMesh);
		Face->SetMaterial(0, Dial);
		Face->SetRelativeLocationAndRotation(BankHourPivot - FVector(0.0, 4.0, 0.0), FRotator(0.0f, 0.0f, 90.0f));
		Face->SetRelativeScale3D(FVector(2.2, 2.2, 0.04));
		Face->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Lit.AddGlow(this, Face, nullptr, Ns::EGlow::Warm, Ns::Srgb(0xfff2d8), 0.4f, -1, false);
	}
	BankHour = Hand(TEXT("SM_NS_Vista_Bank_Hour"), BankHourPivot, 62.0, 7.5);
	BankMinute = Hand(TEXT("SM_NS_Vista_Bank_Minute"), BankMinutePivot, 92.0, 5.0);

	// The Rialto across Wexler, facing south down Fifth (spec 2.3): its blade sign's neon at night, the marquee's chaser bulbs
	// running 18:00-02:00 (M_NS_Glow's chase on its bulbs' phases); the marquee's letter boards and the poster cases are
	// [screen]s (their art's own pictures until core draws them).
	const int32 Rialto = Lit.AddShop({1080, 120, 0x7f});
	if (Site(TEXT("SM_NS_Vista_Rialto")))
	{
		++PlotsKit;
		if (UStaticMeshComponent* Trim = Site(TEXT("SM_NS_Vista_Rialto_Trim")))
		{
			Lit.Shops[Rialto].Trim = Trim;
			Lit.Shops[Rialto].TrimSlot = ImportedSlot(Trim, 3);
		}
		Site(TEXT("SM_NS_Vista_Rialto_Glass"));
		// The marquee's letter boards from core (CLOSED FOR RENOVATION / REOPENING SPRING, MIDNIGHT SHOW FRI on Fridays), lit
		// by its bulbs after dusk; the poster cases keep the art's own pictures.
		Screens.Paint(this, Site(TEXT("SM_NS_Vista_Rialto_Marquee")), TEXT("ns_screen"), TEXT("marquee"), 1024, Ns::EScreen::Bulbs, Ns::Srgb(0x1a1410));
		Site(TEXT("SM_NS_Vista_Rialto_Posters"));
		AddGlow(Site(TEXT("SM_NS_Vista_Rialto_GlowWarm")), Ns::EGlow::Chase, 0xffd9a0, 1.0f, Rialto);
		AddGlow(Site(TEXT("SM_NS_Vista_Rialto_GlowNeon")), Ns::EGlow::Neon, 0xff3a5a, 1.2f, -1);
	}
	else
	{
		++PlotsStandIn;
		UMaterialInterface* Terra = Mat(TEXT("Terracotta"), 0xc8a77e, 0.8f, PatConcrete);
		Box(Terra, FVector(-80.0, Ns::WexlerN, 0.0), FVector(2120.0, Ns::WexlerN + 1800.0, 1600.0));
		UMaterialInstanceDynamic* Blade = Mat(TEXT("RialtoBlade"), 0xff3a5a, 0.4f, 0.0f, 0.0f);
		UStaticMeshComponent* B = Box(Blade, FVector(1000.0, Ns::WexlerN - 120.0, 500.0), FVector(1040.0, Ns::WexlerN, 1500.0), false);
		Lit.AddGlow(this, B, nullptr, Ns::EGlow::Neon, Ns::Srgb(0xff3a5a), 1.0f, -1, false);
		UMaterialInstanceDynamic* Marquee = Mat(TEXT("RialtoMarquee"), 0xffe0a8, 0.4f, 0.0f, 0.0f);
		UStaticMeshComponent* Mq = Box(Marquee, FVector(400.0, Ns::WexlerN - 260.0, 420.0), FVector(1640.0, Ns::WexlerN, 560.0), false);
		Lit.AddGlow(this, Mq, nullptr, Ns::EGlow::Warm, Ns::Srgb(0xffe0a8), 1.0f, Rialto, false);
	}

	// The proxies down the cross streets, both sides, out to where their ground ends in the haze (spec 2.3): beyond the corner
	// plots' returns on the near side, beside the landmarks on the far side (HOLLIS-S and WEXLER-N stand on them until their
	// kit bodies). East, they stop at the backs of Cardinal's west-side fronts (Market's stage runs those across both
	// streets: they close the views east); west, at the 120 m end, a row across each street closes the view there.
	const double East = Ns::CardinalBacks;
	const double West = Ns::CrossWest;
	ProxyRun(FVector(-1800.0, Ns::HollisN, 0.0), FVector(East, Ns::HollisN, 0.0), -90.0f, 101);
	ProxyRun(FVector(3840.0, Ns::HollisN, 0.0), FVector(West, Ns::HollisN, 0.0), -90.0f, 211);
	ProxyRun(FVector(-180.0, Ns::HollisS, 0.0), FVector(East, Ns::HollisS, 0.0), 90.0f, 307);
	ProxyRun(FVector(2220.0, Ns::HollisS, 0.0), FVector(West, Ns::HollisS, 0.0), 90.0f, 401);
	ProxyRun(FVector(-1500.0, Ns::WexlerS, 0.0), FVector(East, Ns::WexlerS, 0.0), 90.0f, 503);
	ProxyRun(FVector(3840.0, Ns::WexlerS, 0.0), FVector(West, Ns::WexlerS, 0.0), 90.0f, 601);
	ProxyRun(FVector(-80.0, Ns::WexlerN, 0.0), FVector(East, Ns::WexlerN, 0.0), -90.0f, 709);
	ProxyRun(FVector(2120.0, Ns::WexlerN, 0.0), FVector(West, Ns::WexlerN, 0.0), -90.0f, 811);
	ProxyRun(FVector(West, Ns::HollisS, 0.0), FVector(West, Ns::HollisN, 0.0), 180.0f, 907);
	ProxyRun(FVector(West, Ns::WexlerS, 0.0), FVector(West, Ns::WexlerN, 0.0), 180.0f, 1013);
	if (!bWideMarket)
	{
		// Today's Market has no Cardinal: rows across the east ends too.
		ProxyRun(FVector(East, Ns::HollisS, 0.0), FVector(East, Ns::HollisN, 0.0), 0.0f, 1117);
		ProxyRun(FVector(East, Ns::WexlerS, 0.0), FVector(East, Ns::WexlerN, 0.0), 0.0f, 1229);
	}

	// The walk limits (spec 2.9): a closure across each near sidewalk (two fence panels, a barricade, a cone), and the near
	// curbs blocked along their length, Fifth's mouths included: the cross streets are streets the hero doesn't step into.
	UMaterialInterface* Fence = Mat(TEXT("Fence"), 0x9aa0a6, 0.4f, 0.0f, 0.0f, 0.8f);
	auto Closure = [&](double X, double YIn, double YOut, float Yaw) {
		const double Mid = (YIn + YOut) * 0.5;
		const double Half = FMath::Abs(YOut - YIn) * 0.5;
		bool bKit = false;
		for (const double Off : {-0.5, 0.5})
		{
			bKit |= NsProp(TEXT("SM_NS_SidewalkClosure"), FVector(X, Mid + Off * Half, 0.0), Yaw) != nullptr;
		}
		if (!bKit)
		{
			Box(Fence, FVector(X - 3.0, Mid - Half, 0.0), FVector(X + 3.0, Mid + Half, 200.0), false);
		}
		const FVector Toward = FRotator(0.0f, Yaw, 0.0f).Vector();
		NsProp(TEXT("SM_NS_Barricade"), FVector(X, Mid, 0.0) + Toward * 120.0, Yaw);
		NsProp(TEXT("SM_NS_Cone"), FVector(X, Mid + Half * 0.6, 0.0) + Toward * 70.0, Yaw + 25.0f);
		Blocker(FVector(X - 20.0, FMath::Min(YIn, YOut), 0.0), FVector(X + 20.0, FMath::Max(YIn, YOut), 300.0));
	};
	Closure(Ns::HollisWalk0, Ns::HollisN, Ns::HollisRoad0, 0.0f);
	Closure(Ns::HollisWalk1, Ns::HollisN, Ns::HollisRoad0, 180.0f);
	Closure(Ns::WexlerWalk0, Ns::WexlerS, Ns::WexlerRoad0, 0.0f);
	Closure(Ns::WexlerWalk1, Ns::WexlerS, Ns::WexlerRoad0, 180.0f);
	Blocker(FVector(Ns::HollisWalk0, Ns::HollisRoad0 - 40.0, -20.0), FVector(Ns::HollisWalk1, Ns::HollisRoad0, 300.0));
	Blocker(FVector(Ns::WexlerWalk0, Ns::WexlerRoad0, -20.0), FVector(Ns::WexlerWalk1, Ns::WexlerRoad0 + 40.0, 300.0));
}
