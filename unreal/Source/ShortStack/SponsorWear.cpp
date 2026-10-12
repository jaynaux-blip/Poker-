#include "SponsorWear.h"

#include "Animation/AnimSequence.h"
#include "BlenderProps.h"
#include "CastLooks.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "CoreCanvasTarget.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshDescription.h"
#include "Misc/PackageName.h"
#include "SlateDrawList.h"
#include "StaticMeshAttributes.h"
#include "StreetAnim.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UObjectIterator.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/UI/SponsorArt.h"

DEFINE_LOG_CATEGORY_STATIC(LogSponsorWear, Log, All);

namespace SponsorWearDetail
{
// ------------------------------------------------------------------ tunables (spec 3.10; ss.Wear.Refit puts them on)
// Offsets from the measured spots (0: as measured), and how far each layer's patch stands off its surface. The standoff
// keeps the cloth under a rigid patch from coming through it as the body moves; the jackets' and the hoodie's are larger
// because their cloth moves further off the bone that carries the patch.
TAutoConsoleVariable<float> CVarPatchUp(TEXT("ss.Wear.PatchUp"), 0.0f, TEXT("The chest patch: cm higher than the measured spot (ss.Wear.Refit puts it on)."));
TAutoConsoleVariable<float> CVarPatchSide(TEXT("ss.Wear.PatchSide"), 0.0f, TEXT("The chest patch: cm further to the wearer's left than the measured spot."));
TAutoConsoleVariable<float> CVarPatchOut(TEXT("ss.Wear.PatchOut"), 0.0f, TEXT("Every patch: cm further out than its standoff (negative: in)."));
TAutoConsoleVariable<float> CVarSleeveDown(TEXT("ss.Wear.SleeveDown"), 0.0f, TEXT("The sleeve patch: cm further down the upper arm than the measured spot."));
TAutoConsoleVariable<float> CVarSleeveTurn(TEXT("ss.Wear.SleeveTurn"), 15.0f, TEXT("The sleeve patch: degrees forward from the arm's outside."));
TAutoConsoleVariable<float> CVarStandoffTee(TEXT("ss.Wear.Standoff.Tee"), 0.3f, TEXT("cm the patch stands off a tee (and every body at the table)."));
TAutoConsoleVariable<float> CVarStandoffTrack(TEXT("ss.Wear.Standoff.Track"), 0.6f, TEXT("cm the patch stands off a track jacket."));
TAutoConsoleVariable<float> CVarStandoffFlannel(TEXT("ss.Wear.Standoff.Flannel"), 0.7f, TEXT("cm the patch stands off a flannel."));
TAutoConsoleVariable<float> CVarStandoffJacket(TEXT("ss.Wear.Standoff.Jacket"), 0.9f, TEXT("cm the patch stands off a bomber, a denim or a leather jacket."));
TAutoConsoleVariable<float> CVarStandoffHoodie(TEXT("ss.Wear.Standoff.Hoodie"), 1.2f, TEXT("cm the patch stands off a hoodie."));
TAutoConsoleVariable<float> CVarStandoffCap(TEXT("ss.Wear.Standoff.Cap"), 0.1f, TEXT("cm the cap patch stands off the hat's front panel."));
TAutoConsoleVariable<int32> CVarPatchLog(TEXT("ss.Wear.PatchLog"), 0, TEXT("1: log every patch put on (who, where, the fit)."));

// ------------------------------------------------------------------ the measured fits
// Measured in the cast's bind pose (Blender, from the bodies Unreal exported for the garments: art/build/wardrobe, their
// tees, and the SK_Garment_Jacket / _Hoodie fits each body wears): durable/segment/work/uewear_measure_patches.py and
// uewear_fit_table.py. The chest: each layer's surface as a plane round the build's spot, the left chest (a man's 1 cm
// above spine_05 and 8.5 cm to his left; a woman's 5 cm above it and 8 to her left, above the bust): the depth D along
// the body's facing from spine_05 (cm) and its slopes along up and to the left. The sleeve: each layer's radius round the
// upper arm at its spot (the tee's 10 cm down from the shoulder joint, inside its short sleeve; the jackets' 12, below
// the shoulder seam), turned 15 degrees forward of the arm's outside, and its slope down the arm. Layers: tee, bomber,
// hoodie. NoFit: not measured on that body (a staff build that wears no garment).
constexpr float NoFit = -1000.0f;
struct FChestPlane
{
	float D, Du, Ds;
};
struct FArmRing
{
	float R, Dr;
};
struct FBodyFit
{
	const TCHAR* Name;
	bool bWoman;
	FChestPlane Chest[3];
	FArmRing Arm[3];
};
const FBodyFit Measured[] = {
	{TEXT("Bea"), true, {{6.400f, -0.536f, -0.175f}, {NoFit, NoFit, NoFit}, {NoFit, NoFit, NoFit}}, {{4.343f, -0.175f}, {NoFit, NoFit}, {NoFit, NoFit}}},
	{TEXT("ExtraA"), true, {{7.645f, -0.795f, -0.290f}, {9.770f, -0.749f, -0.222f}, {10.425f, -0.747f, -0.204f}}, {{5.085f, -0.191f}, {6.185f, 0.220f}, {6.333f, 0.264f}}},
	{TEXT("ExtraB"), false, {{9.723f, -0.326f, -0.311f}, {11.932f, -0.309f, -0.293f}, {12.643f, -0.306f, -0.292f}}, {{5.370f, -0.161f}, {6.988f, 0.217f}, {7.170f, 0.272f}}},
	{TEXT("ExtraC"), true, {{7.050f, -0.569f, -0.063f}, {8.775f, -0.575f, -0.119f}, {9.340f, -0.599f, -0.122f}}, {{4.590f, -0.174f}, {6.243f, 0.241f}, {6.438f, 0.269f}}},
	{TEXT("ExtraD"), false, {{10.750f, -0.284f, -0.209f}, {12.790f, -0.294f, -0.197f}, {13.460f, -0.298f, -0.195f}}, {{5.462f, -0.221f}, {7.113f, 0.107f}, {7.510f, 0.065f}}},
	{TEXT("ExtraE"), true, {{7.585f, -0.424f, -0.310f}, {9.645f, -0.431f, -0.306f}, {10.300f, -0.465f, -0.299f}}, {{4.615f, -0.171f}, {6.457f, 0.096f}, {6.723f, 0.078f}}},
	{TEXT("ExtraF"), false, {{10.085f, -0.301f, -0.499f}, {12.428f, -0.263f, -0.374f}, {13.145f, -0.254f, -0.349f}}, {{5.388f, -0.239f}, {7.368f, -0.012f}, {7.505f, 0.025f}}},
	{TEXT("ExtraG"), true, {{7.710f, -0.821f, -0.308f}, {9.895f, -0.784f, -0.233f}, {10.555f, -0.780f, -0.210f}}, {{5.105f, -0.189f}, {6.177f, 0.231f}, {6.372f, 0.256f}}},
	{TEXT("ExtraH"), false, {{15.258f, -0.369f, -0.150f}, {17.358f, -0.367f, -0.154f}, {18.040f, -0.366f, -0.153f}}, {{5.888f, -0.262f}, {7.938f, -0.030f}, {8.005f, 0.023f}}},
	{TEXT("ExtraI"), true, {{7.930f, -0.765f, -0.286f}, {10.040f, -0.738f, -0.217f}, {10.690f, -0.734f, -0.203f}}, {{5.117f, -0.193f}, {6.058f, 0.223f}, {6.272f, 0.242f}}},
	{TEXT("ExtraJ"), false, {{12.120f, -0.289f, -0.378f}, {14.383f, -0.271f, -0.330f}, {15.112f, -0.266f, -0.321f}}, {{6.010f, -0.189f}, {7.207f, 0.199f}, {7.387f, 0.205f}}},
	{TEXT("ExtraK"), true, {{8.005f, -0.540f, -0.384f}, {10.280f, -0.584f, -0.369f}, {10.975f, -0.621f, -0.363f}}, {{5.325f, -0.202f}, {6.833f, 0.369f}, {7.322f, 0.443f}}},
	{TEXT("ExtraL"), false, {{10.067f, -0.262f, -0.252f}, {12.152f, -0.261f, -0.247f}, {12.840f, -0.260f, -0.244f}}, {{5.870f, -0.123f}, {7.348f, 0.211f}, {7.627f, 0.183f}}},
	{TEXT("HeroA0"), false, {{9.848f, -0.454f, -0.258f}, {11.965f, -0.436f, -0.237f}, {12.650f, -0.432f, -0.236f}}, {{5.280f, -0.161f}, {6.655f, 0.288f}, {6.825f, 0.345f}}},
	{TEXT("HeroA1"), false, {{12.730f, -0.407f, -0.097f}, {14.842f, -0.444f, -0.149f}, {15.515f, -0.450f, -0.157f}}, {{5.040f, -0.277f}, {6.840f, 0.086f}, {6.918f, 0.174f}}},
	{TEXT("HeroA2"), false, {{9.272f, -0.436f, -0.064f}, {11.158f, -0.435f, -0.117f}, {11.762f, -0.462f, -0.122f}}, {{3.692f, -0.149f}, {5.730f, 0.118f}, {5.883f, 0.106f}}},
	{TEXT("HeroB0"), true, {{8.330f, -0.761f, -0.364f}, {10.520f, -0.754f, -0.272f}, {11.195f, -0.752f, -0.249f}}, {{4.785f, -0.184f}, {6.072f, 0.199f}, {6.225f, 0.197f}}},
	{TEXT("HeroB1"), true, {{7.885f, -0.701f, -0.328f}, {10.005f, -0.726f, -0.259f}, {10.655f, -0.740f, -0.244f}}, {{4.808f, -0.176f}, {6.213f, 0.176f}, {6.317f, 0.200f}}},
	{TEXT("HeroB2"), true, {{6.085f, -0.550f, -0.337f}, {8.265f, -0.660f, -0.331f}, {8.830f, -0.758f, -0.353f}}, {{5.445f, -0.176f}, {6.027f, 0.711f}, {6.462f, 1.006f}}},
	{TEXT("Lonnie"), false, {{9.532f, -0.432f, -0.482f}, {NoFit, NoFit, NoFit}, {NoFit, NoFit, NoFit}}, {{5.908f, -0.234f}, {NoFit, NoFit}, {NoFit, NoFit}}},
	{TEXT("Marlene"), true, {{7.375f, -0.690f, -0.352f}, {NoFit, NoFit, NoFit}, {NoFit, NoFit, NoFit}}, {{4.305f, -0.166f}, {NoFit, NoFit}, {NoFit, NoFit}}},
	{TEXT("Ray"), false, {{10.082f, -0.302f, -0.497f}, {NoFit, NoFit, NoFit}, {NoFit, NoFit, NoFit}}, {{5.385f, -0.239f}, {NoFit, NoFit}, {NoFit, NoFit}}},
	{TEXT("Rosa"), true, {{7.720f, -0.820f, -0.311f}, {9.905f, -0.784f, -0.232f}, {NoFit, NoFit, NoFit}}, {{5.107f, -0.191f}, {6.188f, 0.229f}, {NoFit, NoFit}}},
	{TEXT("Ruby"), true, {{7.990f, -0.544f, -0.377f}, {NoFit, NoFit, NoFit}, {NoFit, NoFit, NoFit}}, {{5.325f, -0.205f}, {NoFit, NoFit}, {NoFit, NoFit}}},
};
// A build nobody measured (the cast's cinematic builds, the Back Room's Hero, the archetype body): the measured builds'
// middles. A heavy build reads them a little inside its chest: ss.Wear.PatchOut is there for that (and the log names it).
const FBodyFit TypicalMan = {TEXT(""), false, {{10.08f, -0.33f, -0.25f}, {12.15f, -0.31f, -0.25f}, {12.84f, -0.31f, -0.24f}}, {{5.40f, -0.20f}, {7.00f, 0.15f}, {7.20f, 0.17f}}};
const FBodyFit TypicalWoman = {TEXT(""), true, {{7.71f, -0.70f, -0.31f}, {9.90f, -0.70f, -0.25f}, {10.55f, -0.72f, -0.24f}}, {{5.00f, -0.19f}, {6.20f, 0.22f}, {6.40f, 0.25f}}};
// Where the spots are: the chest's (cm up from spine_05, to the wearer's left) by build, the sleeve's (down the arm) by layer.
const FVector2D ManChest(1.0, 8.5);
const FVector2D WomanChest(5.0, 8.0);
const float TeeSleeveDown = 10.0f;
const float JacketSleeveDown = 12.0f;

const FBodyFit* MeasuredFit(FName Body)
{
	if (Body.IsNone())
	{
		return nullptr;
	}
	const FString Name = Body.ToString();
	for (const FBodyFit& F : Measured)
	{
		if (Name == F.Name)
		{
			return &F;
		}
	}
	return nullptr;
}

/** A bone's transform in the mesh's component space, in its reference pose. */
FTransform RefComponentSpace(const FReferenceSkeleton& Ref, int32 Bone)
{
	FTransform T = Ref.GetRefBonePose()[Bone];
	for (int32 Parent = Ref.GetParentIndex(Bone); Parent != INDEX_NONE; Parent = Ref.GetParentIndex(Parent))
	{
		T = T * Ref.GetRefBonePose()[Parent];
	}
	return T;
}

/** The body's frame in its component space (MetaHuman bodies face +Y with their left at +X), from its own bones. */
struct FBodyFrame
{
	FVector Spine = FVector::ZeroVector; // spine_05
	FVector Shoulder = FVector::ZeroVector; // upperarm_l
	FVector Elbow = FVector::ZeroVector;    // lowerarm_l
	FVector Left = FVector(1.0, 0.0, 0.0);
	FVector Ahead = FVector(0.0, 1.0, 0.0);
	FVector Up = FVector(0.0, 0.0, 1.0);
};
bool FrameOf(const USkeletalMesh* Body, FBodyFrame& F)
{
	if (!Body)
	{
		return false;
	}
	const FReferenceSkeleton& Ref = Body->GetRefSkeleton();
	const int32 Spine = Ref.FindBoneIndex(TEXT("spine_05"));
	const int32 ArmL = Ref.FindBoneIndex(TEXT("upperarm_l"));
	const int32 ArmR = Ref.FindBoneIndex(TEXT("upperarm_r"));
	const int32 ElbowL = Ref.FindBoneIndex(TEXT("lowerarm_l"));
	if (Spine == INDEX_NONE || ArmL == INDEX_NONE || ElbowL == INDEX_NONE)
	{
		return false;
	}
	F.Spine = RefComponentSpace(Ref, Spine).GetLocation();
	F.Shoulder = RefComponentSpace(Ref, ArmL).GetLocation();
	F.Elbow = RefComponentSpace(Ref, ElbowL).GetLocation();
	// The left side from the two shoulders (level), facing a quarter turn from it.
	FVector Across = ArmR != INDEX_NONE ? F.Shoulder - RefComponentSpace(Ref, ArmR).GetLocation() : FVector(1.0, 0.0, 0.0);
	Across.Z = 0.0;
	F.Left = Across.GetSafeNormal(UE_SMALL_NUMBER, FVector(1.0, 0.0, 0.0));
	F.Ahead = FVector(-F.Left.Y, F.Left.X, 0.0);
	return true;
}

float Standoff(SponsorWear::ELayer Layer)
{
	using SponsorWear::ELayer;
	float S = CVarStandoffTee.GetValueOnGameThread();
	switch (Layer)
	{
	case ELayer::Track: S = CVarStandoffTrack.GetValueOnGameThread(); break;
	case ELayer::Flannel: S = CVarStandoffFlannel.GetValueOnGameThread(); break;
	case ELayer::Jacket: S = CVarStandoffJacket.GetValueOnGameThread(); break;
	case ELayer::Hoodie: S = CVarStandoffHoodie.GetValueOnGameThread(); break;
	default: break;
	}
	return S + CVarPatchOut.GetValueOnGameThread();
}

const FBodyFit& FitFor(FName Body, bool bWoman, bool& bMeasured)
{
	const FBodyFit* M = MeasuredFit(Body);
	bMeasured = M != nullptr;
	return M ? *M : (bWoman ? TypicalWoman : TypicalMan);
}

// ------------------------------------------------------------------ testing (ss.Wear.PatchTest)
FString TestBrand[SponsorWear::Spots];
uint32 TestRev = 1;

// ------------------------------------------------------------------ the brands' look (sponsorart::Patch's choices)
/** The twill's ground and the thread of a brand's patch: core's, as sponsorart::Patch draws them and the art's atlas
 *  stitches them (the thread is the brand colour that reads on the ground: IronHouse's red, Volt Rush's lime). */
void PatchColors(const ss::sponsor::Brand& B, uint32& Ground, uint32& Thread)
{
	uint32_t G = Ground;
	uint32_t T = Thread;
	if (ss::ui::sponsorart::PatchColors(B.Id, G, T))
	{
		Ground = G;
		Thread = T;
	}
}

FLinearColor Srgb(uint32 Hex)
{
	return FLinearColor::FromSRGBColor(FColor(static_cast<uint8>((Hex >> 16) & 0xff), static_cast<uint8>((Hex >> 8) & 0xff), static_cast<uint8>(Hex & 0xff)));
}

// ------------------------------------------------------------------ the block-out patches
// The art's shapes (sponsors.py, wear.py): the chest's 7.6 x 5.106 cm, the cap's 5.6 x 3.762, both with 0.7 cm corners, the
// sleeve's 6.4 cm disc; bent to what they're sewn on (radius across and up, cm); the face's twill 1.4 mm thick, puffed 0.25
// mm in the middle; the merrowed edge's section (in from the edge, up off the back, cm) from the face round the roll to the
// back. Origin at the middle of the back; +X out of the patch, +Y the wearer's right, +Z up (wear.py's frame as imported).
const float FaceT = 0.14f;
const float Pillow = 0.025f;
const FVector2D Bead[] = {{0.18, 0.135}, {0.09, 0.24}, {-0.03, 0.25}, {-0.08, 0.11}, {-0.04, 0.0}};

struct FShapeSpec
{
	bool bRound;
	float W, H;
	float BendAcross, BendUp;
};
FShapeSpec ShapeSpec(int32 Shape)
{
	switch (Shape)
	{
	case 1: return {true, 6.4f, 6.4f, 7.5f, 30.0f};
	case 2: return {false, 5.6f, 3.762f, 6.5f, 9.0f};
	default: return {false, 7.6f, 5.106f, 30.0f, 45.0f};
	}
}

/** The outline moved in by E (cm; negative is out), counter-clockwise as you face it, as (across: the wearer's left, up). */
TArray<FVector2D> Outline(const FShapeSpec& S, float E)
{
	TArray<FVector2D> Pts;
	if (S.bRound)
	{
		const float R = S.W * 0.5f - E;
		for (int32 I = 0; I < 40; ++I)
		{
			const float A = UE_TWO_PI * I / 40.0f - UE_HALF_PI;
			Pts.Add(FVector2D(R * FMath::Cos(A), R * FMath::Sin(A)));
		}
		return Pts;
	}
	const float Rc = 0.7f;
	const float Cx = S.W * 0.5f - Rc;
	const float Cy = S.H * 0.5f - Rc;
	const float Rr = Rc - E;
	const FVector2D Corners[4] = {{Cx, -Cy}, {Cx, Cy}, {-Cx, Cy}, {-Cx, -Cy}};
	const float Starts[4] = {-UE_HALF_PI, 0.0f, UE_HALF_PI, UE_PI};
	for (int32 K = 0; K < 4; ++K)
	{
		for (int32 J = 0; J <= 6; ++J)
		{
			const float A = Starts[K] + UE_HALF_PI * J / 6.0f;
			Pts.Add(Corners[K] + FVector2D(Rr * FMath::Cos(A), Rr * FMath::Sin(A)));
		}
		// Points along the straight side to the next corner (so the bend has somewhere to bend).
		const FVector2D From = Pts.Last();
		const int32 N = K + 1 < 4 ? K + 1 : 0;
		const FVector2D To = Corners[N] + FVector2D(Rr * FMath::Cos(Starts[N]), Rr * FMath::Sin(Starts[N]));
		const int32 Splits = (K % 2 == 0) ? 2 : 3;
		for (int32 J = 1; J <= Splits; ++J)
		{
			Pts.Add(FMath::Lerp(From, To, static_cast<float>(J) / (Splits + 1)));
		}
	}
	return Pts;
}

/** (across, up, out) bent round the patch's cylinders, as wear.py bends it, then into the mesh's frame. */
FVector Bent(const FShapeSpec& S, float A, float B, float N)
{
	if (S.BendAcross > 0.0f)
	{
		const float Th = A / S.BendAcross;
		const float R = S.BendAcross + N;
		A = R * FMath::Sin(Th);
		N = R * FMath::Cos(Th) - S.BendAcross;
	}
	if (S.BendUp > 0.0f)
	{
		const float Ph = B / S.BendUp;
		const float R = S.BendUp + N;
		B = R * FMath::Sin(Ph);
		N = R * FMath::Cos(Ph) - S.BendUp;
	}
	return FVector(N, -A, B);
}
} // namespace SponsorWearDetail

using namespace SponsorWearDetail;

// ================================================================== SponsorWear

namespace SponsorWear
{
int32 SurfaceOf(ELayer Layer)
{
	return Layer == ELayer::Tee ? 0 : Layer == ELayer::Hoodie ? 2 : 1;
}

ELayer LayerOf(ss::wardrobe::Cut Outer)
{
	using ss::wardrobe::Cut;
	switch (Outer)
	{
	case Cut::Hoodie: return ELayer::Hoodie;
	case Cut::Track: return ELayer::Track;
	case Cut::Flannel: return ELayer::Flannel;
	case Cut::Bomber:
	case Cut::Denim:
	case Cut::Leather: return ELayer::Jacket;
	default: return ELayer::Tee;
	}
}

const TCHAR* LayerName(ELayer Layer)
{
	static const TCHAR* const Names[] = {TEXT("tee"), TEXT("track jacket"), TEXT("flannel"), TEXT("jacket"), TEXT("hoodie")};
	return Names[FMath::Clamp(static_cast<int32>(Layer), 0, 4)];
}

FString FShown::Describe() const
{
	static const TCHAR* const Words[Spots] = {TEXT("chest"), TEXT("sleeve"), TEXT("cap")};
	FString Out;
	for (int32 I = 0; I < Spots; ++I)
	{
		if (!Brand[I].IsEmpty())
		{
			Out += (Out.IsEmpty() ? TEXT("") : TEXT(", ")) + FString(Words[I]) + TEXT(" ") + Brand[I];
		}
	}
	return Out.IsEmpty() ? FString(TEXT("none")) : Out;
}

FShown FromCloset(const ss::wardrobe::Closet& C, const ss::sponsor::Book* Book, double World)
{
	FShown Out;
	for (int32 I = 0; I < Spots; ++I)
	{
		const std::string& Id = C.Patches[static_cast<size_t>(I)];
		if (Id.empty() || !ss::wardrobe::PatchFits(C, I) || !ss::sponsor::FindBrand(Id))
		{
			continue;
		}
		if (Book && !ss::sponsor::Running(*Book, Id, World))
		{
			continue;
		}
		Out.Brand[I] = UTF8_TO_TCHAR(Id.c_str());
	}
	return Out;
}

FShown ForWorldPerson(const FString& Brand, bool bCapShows)
{
	FShown Out;
	const ss::sponsor::Brand* B = Brand.IsEmpty() ? nullptr : ss::sponsor::FindBrand(std::string(TCHAR_TO_UTF8(*Brand)));
	if (!B)
	{
		return Out;
	}
	bool bCapOnly = false;
	for (const ss::sponsor::Spot S : B->Offer.Wants)
	{
		const int32 I = static_cast<int32>(S);
		if (I < Spots && (I != Cap || bCapShows))
		{
			Out.Brand[I] = Brand;
			return Out;
		}
		bCapOnly |= I == Cap;
	}
	if (bCapOnly)
	{
		// An energy drink's cap patch on someone bareheaded: on the sleeve.
		Out.Brand[Sleeve] = Brand;
	}
	return Out;
}

void ApplyTest(FShown& Shown)
{
	for (int32 I = 0; I < Spots; ++I)
	{
		if (TestBrand[I] == TEXT("off"))
		{
			Shown.Brand[I].Reset();
		}
		else if (!TestBrand[I].IsEmpty())
		{
			Shown.Brand[I] = TestBrand[I];
		}
	}
}

uint32 TestRevision()
{
	return TestRev;
}

bool ShouldLog()
{
	return CVarPatchLog.GetValueOnGameThread() != 0;
}

EHat HatOf(const UStaticMesh* Hat)
{
	const FString Name = Hat ? Hat->GetName() : FString();
	if (Name.StartsWith(TEXT("SM_Hat_Cap")))
	{
		return EHat::StreetCap;
	}
	if (Name.StartsWith(TEXT("SM_Hat_Beanie")))
	{
		return EHat::StreetBeanie;
	}
	if (Name.StartsWith(TEXT("SM_Wear_Cap")))
	{
		return EHat::TableCap;
	}
	if (Name.StartsWith(TEXT("SM_Wear_Beanie")))
	{
		return EHat::TableBeanie;
	}
	return EHat::None;
}

FString Note(int32 Spot, const FString& Brand, ELayer Layer, FName Body, const FFit& Fit)
{
	static const TCHAR* const Words[Spots] = {TEXT("chest"), TEXT("sleeve"), TEXT("cap")};
	if (Spot == Cap)
	{
		return FString::Printf(TEXT("cap %s on the hat's front panel"), *Brand);
	}
	return FString::Printf(TEXT("%s %s on the %s (%s, %s, %.1f cm %s)"), Words[FMath::Clamp(Spot, 0, 2)], *Brand, LayerName(Layer), Body.IsNone() ? TEXT("archetype") : *Body.ToString(),
		Fit.bMeasured ? TEXT("measured") : TEXT("typical build"), Fit.Depth, Spot == Chest ? TEXT("before spine_05") : TEXT("round the arm"));
}

bool FitChest(const USkeletalMesh* Body, FName BodyName, bool bWoman, ELayer Layer, FFit& Out)
{
	FBodyFrame F;
	if (!FrameOf(Body, F))
	{
		return false;
	}
	bool bMeasured = false;
	const FBodyFit& Fit = FitFor(BodyName, bWoman, bMeasured);
	const int32 Surface = SurfaceOf(Layer);
	FChestPlane P = Fit.Chest[Surface];
	if (P.D == NoFit)
	{
		// A build with no garment fit measured: its tee's plane, as far out as the builds' garments stand off their tees.
		P = Fit.Chest[0];
		P.D += Surface == 2 ? 2.8f : 2.12f;
	}
	const FVector2D Spot = Fit.bWoman ? WomanChest : ManChest;
	const float DU = CVarPatchUp.GetValueOnGameThread();
	const float DS = CVarPatchSide.GetValueOnGameThread();
	const float Depth = P.D + P.Du * DU + P.Ds * DS;
	const FVector At = F.Spine + F.Up * (Spot.X + DU) + F.Left * (Spot.Y + DS) + F.Ahead * (Depth + Standoff(Layer));
	// The surface's normal there: the depth's slopes tip it up the chest and out to the side.
	const FVector Normal = (F.Ahead - F.Up * P.Du - F.Left * P.Ds).GetSafeNormal(UE_SMALL_NUMBER, F.Ahead);
	Out.InComponent = FTransform(FRotationMatrix::MakeFromXZ(Normal, F.Up).Rotator(), At);
	Out.Bone = TEXT("spine_05");
	Out.bMeasured = bMeasured;
	Out.Depth = Depth;
	return true;
}

bool FitSleeve(const USkeletalMesh* Body, FName BodyName, bool bWoman, ELayer Layer, FFit& Out)
{
	FBodyFrame F;
	if (!FrameOf(Body, F))
	{
		return false;
	}
	bool bMeasured = false;
	const FBodyFit& Fit = FitFor(BodyName, bWoman, bMeasured);
	const int32 Surface = SurfaceOf(Layer);
	FArmRing R = Fit.Arm[Surface];
	if (R.R == NoFit)
	{
		R = Fit.Arm[0];
		R.R += Surface == 2 ? 1.6f : 1.4f;
		R.Dr = 0.2f;
	}
	const float Down0 = Surface == 0 ? TeeSleeveDown : JacketSleeveDown;
	const float DDown = CVarSleeveDown.GetValueOnGameThread();
	const FVector Along = (F.Elbow - F.Shoulder).GetSafeNormal(UE_SMALL_NUMBER, -F.Up);
	const FVector Lateral = (F.Left - Along * FVector::DotProduct(F.Left, Along)).GetSafeNormal(UE_SMALL_NUMBER, F.Left);
	const FVector Forward = (F.Ahead - Along * FVector::DotProduct(F.Ahead, Along)).GetSafeNormal(UE_SMALL_NUMBER, F.Ahead);
	const float Turn = FMath::DegreesToRadians(CVarSleeveTurn.GetValueOnGameThread());
	const FVector Outside = (Lateral * FMath::Cos(Turn) + Forward * FMath::Sin(Turn)).GetSafeNormal();
	const float Radius = R.R + R.Dr * DDown;
	const FVector At = F.Shoulder + Along * (Down0 + DDown) + Outside * (Radius + Standoff(Layer));
	// Out of the patch along the arm's outside, its up toward the shoulder (the axis wear.py bends the round patch about).
	Out.InComponent = FTransform(FRotationMatrix::MakeFromXZ(Outside, -Along).Rotator(), At);
	Out.Bone = TEXT("upperarm_l");
	Out.bMeasured = bMeasured;
	Out.Depth = Radius;
	return true;
}

bool FitCap(EHat Hat, FTransform& Out)
{
	// The front panel's middle and its normal, measured on the hats' meshes (Blender, metres -> cm): the street's ball cap
	// 5.5 cm under its crown origin; the street beanie's knit just above the cuff, where it slopes back like a cap's panel
	// (the art's review: the cap patch's 9 cm up-bend would sink 2 mm into the straight cuff, its top and bottom edges through
	// the knit; above it the patch's bottom edge clears the cuff's lip by 9 mm); the table's (wear.py) cap and beanie a
	// little over the eyes.
	FVector At;
	FVector Normal;
	bool bStreet = false;
	switch (Hat)
	{
	case EHat::StreetCap:
		At = FVector(0.0, -9.03, -5.5);
		Normal = FVector(0.0, -0.906, 0.42);
		bStreet = true;
		break;
	case EHat::StreetBeanie:
		At = FVector(0.0, -8.78, -4.5);
		Normal = FVector(0.0, -0.871, 0.489);
		bStreet = true;
		break;
	case EHat::TableCap:
		At = FVector(2.72, 0.0, 7.6);
		Normal = FVector(0.90, 0.0, 0.44);
		break;
	case EHat::TableBeanie:
		At = FVector(3.74, 0.0, 7.5);
		Normal = FVector(0.92, 0.0, 0.385);
		break;
	default:
		return false;
	}
	// Blender's (x, y, z) arrive as (x, -y, z); the street's props are then turned as BlenderFacing turns them (the
	// importer's own yaw, 90 as a rule, makes that no turn at all).
	At.Y = -At.Y;
	Normal.Y = -Normal.Y;
	if (bStreet)
	{
		const FRotator Fix(0.0f, 90.0f - BlenderImportYaw(), 0.0f);
		At = Fix.RotateVector(At);
		Normal = Fix.RotateVector(Normal);
	}
	Normal.Normalize();
	At += Normal * (CVarStandoffCap.GetValueOnGameThread() + CVarPatchOut.GetValueOnGameThread());
	Out = FTransform(FRotationMatrix::MakeFromXZ(Normal, FVector::UpVector).Rotator(), At);
	return true;
}

FName BodyNameOf(const FString& BlueprintPath)
{
	const int32 At = BlueprintPath.Find(TEXT("/MHC_"));
	if (At == INDEX_NONE)
	{
		return NAME_None;
	}
	const FString Rest = BlueprintPath.Mid(At + 5);
	int32 End = INDEX_NONE;
	return FName(*(Rest.FindChar(TEXT('/'), End) ? Rest.Left(End) : Rest));
}

bool IsWomansBody(FName Body)
{
	if (const FBodyFit* M = MeasuredFit(Body))
	{
		return M->bWoman;
	}
	return CastLooks::IsWomansFace(Body);
}

void Carry(UStaticMeshComponent* Piece, USkeletalMeshComponent* Parent, FName Bone, const FTransform& InComponent)
{
	const USkeletalMesh* Asset = Parent ? Parent->GetSkeletalMeshAsset() : nullptr;
	const int32 Index = Asset ? Asset->GetRefSkeleton().FindBoneIndex(Bone) : INDEX_NONE;
	if (!Piece || Index == INDEX_NONE)
	{
		return;
	}
	const FTransform BoneRef = RefComponentSpace(Asset->GetRefSkeleton(), Index);
	// A socket passes its component's scale along the bone's own axes: the offset goes where the scaled bone has it, as
	// the hat's does (AShortStackCharacter::WearOnHead).
	const FVector Scale = Parent->GetComponentScale();
	const FVector Delta = InComponent.GetLocation() - BoneRef.GetLocation();
	const FVector Local = BoneRef.GetRotation().UnrotateVector(Scale * Delta) / Scale;
	if (Piece->GetAttachParent() != Parent || Piece->GetAttachSocketName() != Bone)
	{
		Piece->AttachToComponent(Parent, FAttachmentTransformRules::KeepRelativeTransform, Bone);
	}
	Piece->SetRelativeTransform(FTransform(BoneRef.GetRotation().Inverse() * InComponent.GetRotation(), Local, InComponent.GetScale3D()));
}

bool Dress(AActor* Owner, TObjectPtr<UStaticMeshComponent>& Slot, int32 Spot, const FString& Brand)
{
	if (Brand.IsEmpty())
	{
		if (Slot)
		{
			Slot->SetVisibility(false);
		}
		return true;
	}
	USponsorWearArt* Art = Owner ? USponsorWearArt::Get(Owner) : nullptr;
	if (!Art)
	{
		return false;
	}
	if (!Slot || !IsValid(Slot))
	{
		static const TCHAR* const Names[Spots] = {TEXT("PatchChest"), TEXT("PatchSleeve"), TEXT("PatchCap")};
		Slot = NewObject<UStaticMeshComponent>(Owner, MakeUniqueObjectName(Owner, UStaticMeshComponent::StaticClass(), Names[FMath::Clamp(Spot, 0, 2)]), RF_Transient);
		Slot->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Slot->SetCanEverAffectNavigation(false);
		// 2.5 mm of cloth: its shadow is nothing next to the cost of drawing it into every shadow map.
		Slot->SetCastShadow(false);
		Slot->bReceivesDecals = false;
		Slot->RegisterComponent();
	}
	bool bArt = false;
	Slot->SetStaticMesh(Art->Mesh(Spot, bArt));
	Art->Put(Slot, Spot, Brand);
	Slot->SetVisibility(true);
	return true;
}

UAnimSequence* StandingIdle(const USkeleton* Skeleton, bool bLoad)
{
	if (!Skeleton)
	{
		return nullptr;
	}
	for (const int32 Set : {UStreetMocap::Standard, UStreetMocap::Heavy})
	{
		const FString Path = UStreetMocap::ClipPath(Set, EStreetClip::StandIdle);
		UAnimSequence* Seq = Cast<UAnimSequence>(FSoftObjectPath(Path).ResolveObject());
		if (!Seq && bLoad && FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(Path)))
		{
			Seq = LoadObject<UAnimSequence>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		}
		if (Seq && Seq->GetSkeleton() == Skeleton)
		{
			return Seq;
		}
	}
	return nullptr;
}

void PrewarmStanding(TFunction<void(UObject*)> Keep)
{
	if (!Keep)
	{
		return;
	}
	for (const int32 Set : {UStreetMocap::Standard, UStreetMocap::Heavy})
	{
		const FString Path = UStreetMocap::ClipPath(Set, EStreetClip::StandIdle);
		if (UObject* Loaded = FSoftObjectPath(Path).ResolveObject())
		{
			Keep(Loaded);
			continue;
		}
		const FString Package = FPackageName::ObjectPathToPackageName(Path);
		if (!FPackageName::DoesPackageExist(Package))
		{
			continue; // not baked here (anim_setup.py): a backer keeps a chair
		}
		LoadPackageAsync(Package, FLoadPackageAsyncDelegate::CreateLambda([Path, Keep](const FName&, UPackage*, EAsyncLoadingResult::Type Result) {
			if (Result != EAsyncLoadingResult::Succeeded)
			{
				return;
			}
			if (UObject* Seq = FSoftObjectPath(Path).ResolveObject())
			{
				Keep(Seq);
			}
		}));
	}
}
} // namespace SponsorWear

// ================================================================== USponsorWearArt

USponsorWearArt* USponsorWearArt::Get(const UObject* Context)
{
	const UWorld* World = Context ? Context->GetWorld() : nullptr;
	UGameInstance* Instance = World ? World->GetGameInstance() : nullptr;
	return Instance ? Instance->GetSubsystem<USponsorWearArt>() : nullptr;
}

void USponsorWearArt::Deinitialize()
{
	if (Painter.IsValid())
	{
		Painter->Release();
		Painter.Reset();
	}
	Art.Reset();
	Keep.Reset();
	Super::Deinitialize();
}

void USponsorWearArt::Look()
{
	if (bLooked)
	{
		return;
	}
	bLooked = true;
	static const TCHAR* const Names[3] = {TEXT("SM_Wear_Patch"), TEXT("SM_Wear_PatchRound"), TEXT("SM_Wear_PatchCap")};
	for (int32 S = 0; S < 3; ++S)
	{
		Meshes[S] = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/ShortStack/Meshes/%s/%s.%s"), Names[S], Names[S], Names[S]), nullptr, LOAD_NoWarn | LOAD_Quiet);
		bArtMesh[S] = Meshes[S] != nullptr;
	}
	PatchMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ShortStack/Materials/M_Patch.M_Patch"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	static const TCHAR* const Mis[4] = {TEXT("MI_Patch_Wide_Face"), TEXT("MI_Patch_Wide_Thread"), TEXT("MI_Patch_Round_Face"), TEXT("MI_Patch_Round_Thread")};
	for (int32 I = 0; I < 4; ++I)
	{
		Instances[I] = PatchMaterial ? LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/ShortStack/Materials/%s.%s"), Mis[I], Mis[I]), nullptr, LOAD_NoWarn | LOAD_Quiet)
									 : nullptr;
	}
	SheetMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ShortStack/Materials/M_WidgetLit.M_WidgetLit"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	SurfaceMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ShortStack/Materials/M_Surface.M_Surface"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	UE_LOG(LogSponsorWear, Log, TEXT("Patches: meshes %s, %s"), bArtMesh[0] && bArtMesh[1] && bArtMesh[2] ? TEXT("the art's") : TEXT("block-outs (SM_Wear_Patch* not imported)"),
		PatchMaterial ? TEXT("M_Patch's atlas") : TEXT("the brands' sheets (M_Patch not imported)"));
}

UStaticMesh* USponsorWearArt::Mesh(int32 Shape, bool& bArt)
{
	Look();
	Shape = FMath::Clamp(Shape, 0, 2);
	if (!Meshes[Shape])
	{
		Meshes[Shape] = BuildBlockOut(Shape);
	}
	bArt = bArtMesh[Shape];
	return Meshes[Shape];
}

UMaterialInterface* USponsorWearArt::PatchInstance(int32 Shape, int32 Part)
{
	// The art's instances (Shape 0 wide: the chest's and the cap's; 1 round), else instances made here with the same switches.
	const int32 I = (Shape == 1 ? 2 : 0) + Part;
	if (!Instances[I] && PatchMaterial)
	{
		UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(PatchMaterial, this);
		Mid->SetScalarParameterValue(TEXT("Shape"), Shape == 1 ? 1.0f : 0.0f);
		Mid->SetScalarParameterValue(TEXT("Part"), static_cast<float>(Part));
		Instances[I] = Mid;
	}
	return Instances[I];
}

const USponsorWearArt::FBrandArt& USponsorWearArt::ArtFor(const FString& Brand)
{
	if (const FBrandArt* Found = Art.Find(Brand))
	{
		return *Found;
	}
	FBrandArt& A = Art.Add(Brand);
	const std::string Id(TCHAR_TO_UTF8(*Brand));
	const ss::sponsor::Brand* B = ss::sponsor::FindBrand(Id);
	uint32 Ground = 0x2a2b30;
	uint32 Thread = 0xd8d4c8;
	if (B)
	{
		PatchColors(*B, Ground, Thread);
	}
	auto Plain = [this](uint32 Hex, float Rough) -> UMaterialInterface* {
		UMaterialInterface* Parent = SurfaceMaterial ? SurfaceMaterial.Get() : UMaterial::GetDefaultMaterial(MD_Surface);
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Parent, this);
		M->SetVectorParameterValue(TEXT("BaseColor"), Srgb(Hex));
		M->SetScalarParameterValue(TEXT("Roughness"), Rough);
		M->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
		M->SetScalarParameterValue(TEXT("Pattern"), 0.0f);
		Keep.Add(M);
		return M;
	};
	A.Edge = Plain(Thread, 0.45f);
	A.Face[0] = A.Face[1] = nullptr;
	if (B && SheetMaterial && FSlateTextMeasurer::IsAvailable())
	{
		// The brand's sheet, drawn once this session: the wide patch shows all of it, the round patch its mark on the twill.
		if (!Painter.IsValid())
		{
			Painter = MakeShared<FCoreCanvasTarget>();
		}
		FSlateTextMeasurer Measurer;
		for (int32 Round = 0; Round < 2; ++Round)
		{
			const int32 W = Round ? 256 : 512;
			const int32 H = Round ? 256 : 340;
			UTextureRenderTarget2D* Target = FCoreCanvasTarget::MakeTarget(this, W, H);
			if (!Target)
			{
				continue;
			}
			Keep.Add(Target);
			TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
			ss::ui::Canvas C(*List, Measurer, static_cast<float>(W), static_cast<float>(H), 1.0f);
			if (Round)
			{
				C.FillCircle(128.0f, 128.0f, 128.0f, ss::ui::Hex(Ground));
				C.StrokeEllipse(128.0f, 128.0f, 120.0f, 120.0f, ss::ui::Hex(Thread), 13.0f);
				C.StrokeEllipse(128.0f, 128.0f, 106.0f, 106.0f, ss::ui::Hex(Thread, 0.6f), 2.0f);
				ss::ui::sponsorart::Mark(C, Id, {50.0f, 50.0f, 156.0f, 156.0f});
			}
			else
			{
				ss::ui::sponsorart::Patch(C, Id);
			}
			Painter->Render(Target, List);
			UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(SheetMaterial, this);
			M->SetTextureParameterValue(TEXT("SlateUI"), Target);
			Keep.Add(M);
			A.Face[Round] = M;
		}
	}
	for (int32 Round = 0; Round < 2; ++Round)
	{
		if (!A.Face[Round])
		{
			A.Face[Round] = Plain(Ground, 0.8f);
		}
	}
	return A;
}

void USponsorWearArt::Put(UStaticMeshComponent* Patch, int32 Shape, const FString& Brand)
{
	Look();
	if (!Patch)
	{
		return;
	}
	const ss::sponsor::Brand* B = ss::sponsor::FindBrand(std::string(TCHAR_TO_UTF8(*Brand)));
	const TArray<FName> Slots = Patch->GetMaterialSlotNames();
	const int32 NumSlots = FMath::Max(Patch->GetNumMaterials(), Slots.Num());
	auto PartOf = [&Slots](int32 Slot) { return Slots.IsValidIndex(Slot) && Slots[Slot].ToString().Contains(TEXT("thread")) ? 1 : (Slots.IsValidIndex(Slot) ? 0 : Slot); };
	if (PatchMaterial && B)
	{
		// The embroidered atlas: the brand's cell by custom primitive data, the same instances on every patch.
		for (int32 Slot = 0; Slot < NumSlots; ++Slot)
		{
			if (UMaterialInterface* M = PatchInstance(Shape, FMath::Clamp(PartOf(Slot), 0, 1)))
			{
				Patch->SetMaterial(Slot, M);
			}
		}
		Patch->SetCustomPrimitiveDataFloat(0, static_cast<float>(B->Mark + 1));
		return;
	}
	const FBrandArt& A = ArtFor(Brand);
	for (int32 Slot = 0; Slot < NumSlots; ++Slot)
	{
		Patch->SetMaterial(Slot, PartOf(Slot) == 1 ? A.Edge : A.Face[Shape == 1 ? 1 : 0]);
	}
}

UStaticMesh* USponsorWearArt::BuildBlockOut(int32 Shape)
{
	const FShapeSpec S = ShapeSpec(Shape);
	// The patch's own vertices (each group its own, so the face, the edge and the back keep their creases).
	struct FV
	{
		FVector P;
		FVector2D UV;
		FVector N = FVector::ZeroVector;
	};
	TArray<FV> Verts;
	struct FT
	{
		int32 A, B, C, Slot;
	};
	TArray<FT> Tris;
	auto Vert = [&](float A, float B, float N, const FVector2D& UV) {
		FV V;
		V.P = Bent(S, A, B, N);
		V.UV = UV;
		return Verts.Add(V);
	};
	// A triangle wound so its front faces Outward (the engine's front: (C - A) x (B - A) along the normal); its normal is
	// added to its corners' for smooth shading within each group.
	auto Tri = [&](int32 A, int32 B, int32 C, int32 Slot, const FVector& Outward) {
		FVector Cross = FVector::CrossProduct(Verts[C].P - Verts[A].P, Verts[B].P - Verts[A].P);
		if (Cross.SizeSquared() < 1e-10)
		{
			return;
		}
		if (FVector::DotProduct(Cross, Outward) < 0.0)
		{
			Swap(B, C);
			Cross = -Cross;
		}
		Tris.Add({A, B, C, Slot});
		for (const int32 I : {A, B, C})
		{
			Verts[I].N += Cross;
		}
	};
	const float W = S.W;
	const float H = S.H;
	auto FaceUV = [W, H](float A, float B) { return FVector2D((A + W * 0.5f) / W, (H * 0.5f - B) / H); };
	// The face: the outline, rings in toward the middle, the middle, puffed toward the middle.
	const TArray<FVector2D> Base = Outline(S, 0.0f);
	const int32 N = Base.Num();
	TArray<TArray<int32>> Rings;
	for (const float Sc : {1.0f, 0.66f, 0.33f})
	{
		TArray<int32>& Ring = Rings.AddDefaulted_GetRef();
		const float Z = FaceT + Pillow * (1.0f - Sc * Sc);
		for (const FVector2D& Pt : Base)
		{
			Ring.Add(Vert(Pt.X * Sc, Pt.Y * Sc, Z, FaceUV(Pt.X * Sc, Pt.Y * Sc)));
		}
	}
	const int32 Middle = Vert(0.0f, 0.0f, FaceT + Pillow, FaceUV(0.0f, 0.0f));
	const FVector Out(1.0, 0.0, 0.0);
	for (int32 R = 0; R + 1 < Rings.Num(); ++R)
	{
		for (int32 I = 0; I < N; ++I)
		{
			const int32 J = (I + 1) % N;
			Tri(Rings[R][I], Rings[R][J], Rings[R + 1][J], 0, Out);
			Tri(Rings[R][I], Rings[R + 1][J], Rings[R + 1][I], 0, Out);
		}
	}
	for (int32 I = 0; I < N; ++I)
	{
		Tri(Rings.Last()[I], Rings.Last()[(I + 1) % N], Middle, 0, Out);
	}
	// The merrowed edge: the bead's section swept round the outline (u: centimetres along it; v: from the face to the back).
	TArray<float> Along;
	Along.Add(0.0f);
	for (int32 I = 0; I < N; ++I)
	{
		Along.Add(Along.Last() + static_cast<float>(FVector2D::Distance(Base[I], Base[(I + 1) % N])));
	}
	const int32 NumBead = UE_ARRAY_COUNT(Bead);
	TArray<TArray<int32>> BeadRings;
	for (int32 K = 0; K < NumBead; ++K)
	{
		TArray<int32>& Ring = BeadRings.AddDefaulted_GetRef();
		const TArray<FVector2D> Pts = Outline(S, static_cast<float>(Bead[K].X));
		for (int32 I = 0; I <= N; ++I)
		{
			// One more than the outline: the seam's two ends carry their own u.
			const FVector2D& Pt = Pts[I % N];
			Ring.Add(Vert(Pt.X, Pt.Y, static_cast<float>(Bead[K].Y), FVector2D(Along[I], static_cast<float>(K) / (NumBead - 1))));
		}
	}
	// Which way each band of the bead faces, as (out from the patch's middle, up off its back): the rise from the face, the
	// roll's top, its outer side, its foot.
	const FVector2D BandFacing[] = {{-0.8, 1.0}, {0.0, 1.0}, {1.0, 0.35}, {1.0, -0.35}};
	for (int32 K = 0; K + 1 < NumBead; ++K)
	{
		for (int32 I = 0; I < N; ++I)
		{
			const FVector2D Mid2 = (Base[I] + Base[(I + 1) % N]) * 0.5;
			FVector Radial = Bent(S, static_cast<float>(Mid2.X), static_cast<float>(Mid2.Y), 0.12f) - Bent(S, 0.0f, 0.0f, 0.12f);
			Radial.X = 0.0;
			Radial = Radial.GetSafeNormal(UE_SMALL_NUMBER, FVector(0.0, 0.0, 1.0));
			const FVector Outward = Radial * BandFacing[K].X + FVector(1.0, 0.0, 0.0) * BandFacing[K].Y;
			Tri(BeadRings[K][I], BeadRings[K + 1][I], BeadRings[K + 1][I + 1], 1, Outward);
			Tri(BeadRings[K][I], BeadRings[K + 1][I + 1], BeadRings[K][I + 1], 1, Outward);
		}
	}
	// The back, on the garment: one point of the face's twill.
	{
		const TArray<FVector2D> Pts = Outline(S, static_cast<float>(Bead[NumBead - 1].X));
		TArray<int32> Back;
		for (const FVector2D& Pt : Pts)
		{
			Back.Add(Vert(Pt.X, Pt.Y, 0.0f, FVector2D(0.5f, 0.045f)));
		}
		const int32 Centre = Vert(0.0f, 0.0f, 0.0f, FVector2D(0.5f, 0.045f));
		for (int32 I = 0; I < Back.Num(); ++I)
		{
			Tri(Back[I], Back[(I + 1) % Back.Num()], Centre, 0, FVector(-1.0, 0.0, 0.0));
		}
	}

	FMeshDescription Desc;
	FStaticMeshAttributes Attr(Desc);
	Attr.Register();
	TVertexAttributesRef<FVector3f> Positions = Attr.GetVertexPositions();
	TVertexInstanceAttributesRef<FVector3f> Normals = Attr.GetVertexInstanceNormals();
	TVertexInstanceAttributesRef<FVector3f> Tangents = Attr.GetVertexInstanceTangents();
	TVertexInstanceAttributesRef<float> Signs = Attr.GetVertexInstanceBinormalSigns();
	TVertexInstanceAttributesRef<FVector2f> UVs = Attr.GetVertexInstanceUVs();
	TVertexInstanceAttributesRef<FVector4f> Colors = Attr.GetVertexInstanceColors();
	TPolygonGroupAttributesRef<FName> GroupNames = Attr.GetPolygonGroupMaterialSlotNames();
	UVs.SetNumChannels(1);
	const FName SlotNames[2] = {TEXT("patch_face"), TEXT("patch_thread")};
	FPolygonGroupID Groups[2];
	for (int32 K = 0; K < 2; ++K)
	{
		Groups[K] = Desc.CreatePolygonGroup();
		GroupNames[Groups[K]] = SlotNames[K];
	}
	TArray<FVertexInstanceID> VertexIds;
	VertexIds.Reserve(Verts.Num());
	for (FV& V : Verts)
	{
		const FVertexID Id = Desc.CreateVertex();
		Positions[Id] = FVector3f(V.P);
		const FVertexInstanceID I = Desc.CreateVertexInstance(Id);
		const FVector Normal = V.N.GetSafeNormal(UE_SMALL_NUMBER, FVector(1.0, 0.0, 0.0));
		// u runs to the wearer's left (-Y), v down (-Z): the tangent is -Y in the surface's plane.
		FVector Tangent = FVector(0.0, -1.0, 0.0) - Normal * FVector::DotProduct(FVector(0.0, -1.0, 0.0), Normal);
		if (!Tangent.Normalize())
		{
			Tangent = FVector(0.0, 0.0, -1.0);
		}
		Normals[I] = FVector3f(Normal);
		Tangents[I] = FVector3f(Tangent);
		Signs[I] = 1.0f;
		UVs.Set(I, 0, FVector2f(V.UV));
		Colors[I] = FVector4f(1.0f, 1.0f, 1.0f, 1.0f);
		VertexIds.Add(I);
	}
	for (const FT& T : Tris)
	{
		const FVertexInstanceID Ids[3] = {VertexIds[T.A], VertexIds[T.B], VertexIds[T.C]};
		Desc.CreateTriangle(Groups[T.Slot], MakeArrayView(Ids, 3));
	}
	UStaticMesh* Mesh = NewObject<UStaticMesh>(this, NAME_None, RF_Transient);
	UMaterialInterface* Default = UMaterial::GetDefaultMaterial(MD_Surface);
	for (int32 K = 0; K < 2; ++K)
	{
		Mesh->GetStaticMaterials().Add(FStaticMaterial(Default, SlotNames[K]));
	}
	UStaticMesh::FBuildMeshDescriptionsParams Params;
	Params.bFastBuild = true;
	Params.bCommitMeshDescription = false;
	Params.bMarkPackageDirty = false;
	TArray<const FMeshDescription*> Lods;
	Lods.Add(&Desc);
	if (!Mesh->BuildFromMeshDescriptions(Lods, Params))
	{
		return nullptr;
	}
	Keep.Add(Mesh);
	return Mesh;
}

FString USponsorWearArt::Describe() const
{
	FString Brands;
	for (const TPair<FString, FBrandArt>& It : Art)
	{
		Brands += (Brands.IsEmpty() ? TEXT("") : TEXT(", ")) + It.Key;
	}
	return FString::Printf(TEXT("meshes: chest %s, sleeve %s, cap %s; look: %s; sheets drawn: %s"), bArtMesh[0] ? TEXT("SM_Wear_Patch") : TEXT("block-out"),
		bArtMesh[1] ? TEXT("SM_Wear_PatchRound") : TEXT("block-out"), bArtMesh[2] ? TEXT("SM_Wear_PatchCap") : TEXT("block-out"),
		PatchMaterial ? TEXT("M_Patch (the atlas, by custom primitive data)") : SheetMaterial ? TEXT("the brands' sheets on M_WidgetLit") : TEXT("plain colours"),
		Brands.IsEmpty() ? TEXT("none") : *Brands);
}

// ================================================================== UPennyPolaroid

UPennyPolaroid::UPennyPolaroid()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.25f;
	bTickInEditor = false;
}

void UPennyPolaroid::Setup(UMaterialInterface* InPrinted, UStaticMesh* PinMesh, UMaterialInterface* PinMaterial)
{
	Printed = InPrinted;
	PinMeshAsset = PinMesh;
	PinMat = PinMaterial;
	if (IsRegistered())
	{
		BuildParts();
	}
}

void UPennyPolaroid::OnRegister()
{
	Super::OnRegister();
	BuildParts();
}

void UPennyPolaroid::BuildParts()
{
	const UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !World->IsGameWorld() || !Owner || Photo || !Printed)
	{
		return;
	}
	// The photo (sponsorart::Polaroid's 512 x 512: the white frame on a square of cork, 11 cm across), printed: lit like the
	// wall it's pinned to. The widget's quad lies in its Y (width) and Z (height), its face toward +X.
	Photo = NewObject<UWidgetComponent>(Owner, NAME_None, RF_Transient);
	Photo->SetupAttachment(this);
	Photo->SetWidgetSpace(EWidgetSpace::World);
	Photo->SetDrawSize(FVector2D(512.0, 512.0));
	Photo->SetPivot(FVector2D(0.5, 0.5));
	Photo->SetBlendMode(EWidgetBlendMode::Opaque);
	Photo->SetTwoSided(false);
	Photo->SetBackgroundColor(FLinearColor(0.02f, 0.02f, 0.02f, 1.0f));
	Photo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Photo->SetCastShadow(false);
	Photo->SetRelativeScale3D(FVector(1.0, 11.0 / 512.0, 11.0 / 512.0));
	Photo->SetMaterial(0, Printed);
	Photo->SetHiddenInGame(true);
	Photo->RegisterComponent();
	if (PinMeshAsset)
	{
		// A red pushpin through the top of the frame.
		Pin = NewObject<UStaticMeshComponent>(Owner, NAME_None, RF_Transient);
		Pin->SetupAttachment(this);
		Pin->SetStaticMesh(PinMeshAsset);
		if (PinMat)
		{
			Pin->SetMaterial(0, PinMat);
		}
		Pin->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Pin->SetRelativeLocation(FVector(0.45, 0.0, 4.75));
		Pin->SetRelativeScale3D(FVector(0.011));
		Pin->SetHiddenInGame(true);
		Pin->RegisterComponent();
	}
	// The flash: a cold burst from the counter, where Ray holds the camera.
	FlashLight = NewObject<UPointLightComponent>(Owner, NAME_None, RF_Transient);
	FlashLight->SetupAttachment(this);
	FlashLight->SetRelativeLocation(FVector(300.0, 60.0, -20.0));
	FlashLight->SetIntensityUnits(ELightUnits::Candelas);
	FlashLight->SetIntensity(0.0f);
	FlashLight->SetUseTemperature(true);
	FlashLight->SetTemperature(6500.0f);
	FlashLight->SetAttenuationRadius(650.0f);
	FlashLight->SetCastShadows(false);
	FlashLight->SetVisibility(false);
	FlashLight->RegisterComponent();
}

void UPennyPolaroid::OnUnregister()
{
	Slate.Reset();
	Super::OnUnregister();
}

void UPennyPolaroid::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	// Its parts are the actor's own components (made at play, not the street's construction parts): they go with it, so a
	// street rebuilt in play (AStreetStage::OnConstruction's rerun) leaves no photo hanging on the wall without its deal.
	if (!bDestroyingHierarchy)
	{
		for (USceneComponent* Part : {static_cast<USceneComponent*>(Photo.Get()), static_cast<USceneComponent*>(Pin.Get()), static_cast<USceneComponent*>(FlashLight.Get())})
		{
			if (IsValid(Part))
			{
				Part->DestroyComponent();
			}
		}
	}
	Photo = nullptr;
	Pin = nullptr;
	FlashLight = nullptr;
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UPennyPolaroid::Show(bool bShow)
{
	if (bShow == bUp)
	{
		return;
	}
	bUp = bShow;
	if (Photo)
	{
		Photo->SetHiddenInGame(!bShow);
	}
	if (Pin)
	{
		Pin->SetHiddenInGame(!bShow);
	}
}

void UPennyPolaroid::Draw(const ss::hero::Look& Look)
{
	if (!Photo || !FSlateTextMeasurer::IsAvailable())
	{
		return;
	}
	FSlateTextMeasurer Measurer;
	TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
	ss::ui::Canvas C(*List, Measurer, 512.0f, 512.0f, 1.0f);
	ss::ui::sponsorart::Polaroid(C, Look, "PENNY'S PICK");
	if (!Slate.IsValid())
	{
		Slate = SNew(SDrawListWidget).DesiredSize(FVector2D(512.0, 512.0));
		Photo->SetSlateWidget(Slate);
		Photo->SetManuallyRedraw(true);
	}
	Slate->SetDrawList(List);
	Photo->RequestRedraw();
	bDrawn = true;
}

void UPennyPolaroid::Flash()
{
	FlashT = 0.0f;
	if (FlashLight)
	{
		FlashLight->SetVisibility(true);
	}
	SetComponentTickInterval(0.0f);
}

void UPennyPolaroid::Force(int32 Up)
{
	Forced = FMath::Clamp(Up, -1, 1);
	Check = 0.0f;
}

void UPennyPolaroid::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (FlashT >= 0.0f)
	{
		// A camera's flash: a hard white peak, gone in a sixth of a second.
		FlashT += DeltaTime;
		const float K = FlashT < 0.03f ? FlashT / 0.03f : FMath::Max(0.0f, 1.0f - (FlashT - 0.03f) / 0.14f);
		if (FlashLight)
		{
			FlashLight->SetIntensity(900.0f * K * K);
		}
		if (FlashT > 0.2f)
		{
			FlashT = -1.0f;
			if (FlashLight)
			{
				FlashLight->SetIntensity(0.0f);
				FlashLight->SetVisibility(false);
			}
			SetComponentTickInterval(0.25f);
		}
		return;
	}
	ss::Session* S = CastLooks::BoundSession(this);
	if (!S)
	{
		Show(Forced == 1 && bDrawn);
		return;
	}
	const ss::sponsor::Contract* K = ss::sponsor::Running(S->Life.Sponsors, "luckypenny", S->WorldMinutes());
	const int32 Running = K ? 1 : 0;
	const int32 Photos = K ? K->Now.Appearances : 0;
	// Signed at the counter, or the month's photo taken, with the player here: the flash, then the photo goes up.
	if ((SeenRunning == 0 && Running == 1) || (Running == 1 && SeenPhotos >= 0 && Photos > SeenPhotos))
	{
		Flash();
	}
	SeenRunning = Running;
	SeenPhotos = Photos;
	const bool bWant = Forced >= 0 ? Forced == 1 : Running == 1;
	if (bWant)
	{
		const ss::hero::Look& L = S->Person.Appearance;
		uint32 Key = 17u;
		for (const int32 V : {L.Body, L.Face, L.Skin, L.Eyes, L.Brows, L.Hair, L.HairColor, L.FacialHair, L.Build, L.Outfit, L.OutfitColor, L.Glasses, L.Hat})
		{
			Key = HashCombine(Key, static_cast<uint32>(V + 1));
		}
		if (!bDrawn || Key != LookKey)
		{
			// The player's picture as they look now (the barber's cut shows on the next one).
			LookKey = Key;
			Draw(L);
		}
	}
	Show(bWant && bDrawn);
}

FString UPennyPolaroid::Describe() const
{
	return FString::Printf(TEXT("the Polaroid: %s%s, the deal %s, %d photos this period"), bUp ? TEXT("up") : TEXT("down"),
		Forced >= 0 ? TEXT(" (forced)") : TEXT(""), SeenRunning == 1 ? TEXT("running") : SeenRunning == 0 ? TEXT("not running") : TEXT("unread"), FMath::Max(SeenPhotos, 0));
}

// ================================================================== console

namespace SponsorWearDetail
{
FAutoConsoleCommandWithWorldAndArgs CmdPatchTest(TEXT("ss.Wear.PatchTest"),
	TEXT("ss.Wear.PatchTest <chest|sleeve|cap|all> <brand id|off|clear>: wear a brand's patch on a spot whatever the closet says (testing; the street's hero and everyone at the table). "
		 "clear goes back to the closet's."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (Args.Num() < 2)
		{
			UE_LOG(LogSponsorWear, Display, TEXT("ss.Wear.PatchTest <chest|sleeve|cap|all> <brand id|off|clear>; now: chest '%s', sleeve '%s', cap '%s'"), *TestBrand[0], *TestBrand[1],
				*TestBrand[2]);
			return;
		}
		const FString Where = Args[0].ToLower();
		const FString What = Args[1].ToLower();
		if (What != TEXT("off") && What != TEXT("clear") && !ss::sponsor::FindBrand(std::string(TCHAR_TO_UTF8(*What))))
		{
			UE_LOG(LogSponsorWear, Warning, TEXT("ss.Wear.PatchTest: no brand '%s' (luckypenny, barber, ironhouse, voltrush, overclock, embercrest, halo, tunnelrat, stacked, rangewise, riverline, sitwell, turnstone)"),
				*What);
			return;
		}
		for (int32 I = 0; I < SponsorWear::Spots; ++I)
		{
			static const TCHAR* const Words[SponsorWear::Spots] = {TEXT("chest"), TEXT("sleeve"), TEXT("cap")};
			if (Where == TEXT("all") || Where == Words[I])
			{
				TestBrand[I] = What == TEXT("clear") ? FString() : What;
			}
		}
		++TestRev;
		UE_LOG(LogSponsorWear, Display, TEXT("ss.Wear.PatchTest: chest '%s', sleeve '%s', cap '%s' (the wearers dress again within a second)"), *TestBrand[0], *TestBrand[1], *TestBrand[2]);
	}));
FAutoConsoleCommandWithWorld CmdPatches(TEXT("ss.Wear.Patches"), TEXT("The patches' art in use (meshes, look, the sheets drawn) and the Polaroid's state."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World) {
		if (USponsorWearArt* Art = World ? USponsorWearArt::Get(World) : nullptr)
		{
			UE_LOG(LogSponsorWear, Display, TEXT("Patches: %s"), *Art->Describe());
		}
		for (TObjectIterator<UPennyPolaroid> It; It; ++It)
		{
			if (It->GetWorld() == World)
			{
				UE_LOG(LogSponsorWear, Display, TEXT("%s"), *It->Describe());
			}
		}
	}));
FAutoConsoleCommandWithWorldAndArgs CmdPolaroid(TEXT("ss.Wear.Polaroid"), TEXT("ss.Wear.Polaroid <up|down|auto|flash>: the Lucky Penny's Polaroid pinned up or taken down whatever the deal says, back to the deal's word, or the flash (testing)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		const FString What = Args.Num() > 0 ? Args[0].ToLower() : FString(TEXT("auto"));
		int32 Found = 0;
		for (TObjectIterator<UPennyPolaroid> It; It; ++It)
		{
			if (It->GetWorld() != World)
			{
				continue;
			}
			++Found;
			if (What == TEXT("flash"))
			{
				It->Flash();
			}
			else
			{
				It->Force(What == TEXT("up") ? 1 : What == TEXT("down") ? 0 : -1);
			}
		}
		UE_LOG(LogSponsorWear, Display, TEXT("ss.Wear.Polaroid %s: %d on this map (the street's Lucky Penny)"), *What, Found);
	}));
} // namespace SponsorWearDetail
