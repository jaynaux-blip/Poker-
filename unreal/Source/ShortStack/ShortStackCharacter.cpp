#include "ShortStackCharacter.h"

#include "BackRoomAnim.h"
#include "BlenderProps.h"
#include "Camera/CameraComponent.h"
#include "CastLooks.h"
#include "Components/CapsuleComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkinnedAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/StreamableManager.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GroomAsset.h"
#include "GroomBindingAsset.h"
#include "GroomComponent.h"
#include "HAL/IConsoleManager.h"
#include "HeroGrooms.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshDescription.h"
#include "Misc/PackageName.h"
#include "StaticMeshAttributes.h"
#include "StreetAnim.h"
#include "TalkFace.h"

DEFINE_LOG_CATEGORY_STATIC(LogStreetHero, Log, All);

/**
 * What a body wears: the MetaHuman's tee (its color, a print or a chest graphic) and shorts (their color), a jacket and
 * trousers drawn from garment meshes where there are any, the shoes, the hat and the glasses.
 */
struct FStreetOutfit
{
	FLinearColor Shirt = FLinearColor(0.03f, 0.03f, 0.035f);
	FLinearColor Pants = FLinearColor(0.02f, 0.025f, 0.035f);
	/** A repeating print over the shirt (T_Print_*: its R, G and B masks take colors A, B and C); none when null. */
	const TCHAR* Print = nullptr;
	float PrintTiling = 1.0f;
	FLinearColor PrintA = FLinearColor::Black;
	FLinearColor PrintB = FLinearColor::Black;
	FLinearColor PrintC = FLinearColor::Black;
	/** The print's and the graphic's numbers (CastLooks::PrintTexture, GraphicTexture; 0: none), for the table's persona. */
	int32 PrintIndex = 0;
	int32 GraphicIndex = 0;
	/** A chest graphic (T_Graphic_*, laid on by its alpha) in up to three inks; none when null. */
	const TCHAR* Graphic = nullptr;
	FLinearColor GraphicA = FLinearColor::White;
	FLinearColor GraphicB = FLinearColor::Black;
	FLinearColor GraphicC = FLinearColor::Gray;
	/** A jacket over the tee (wardrobe::Cut Hoodie..Track) in a color; Count: the tee alone. */
	ss::wardrobe::Cut Outer = ss::wardrobe::Cut::Count;
	FLinearColor OuterColor = FLinearColor(0.05f, 0.05f, 0.06f);
	/** Trousers (Jeans, Chinos, WorkPants) in a color (the table's trousers tone); Count or Shorts: the shorts in Pants. */
	ss::wardrobe::Cut Bottom = ss::wardrobe::Cut::Count;
	FLinearColor BottomColor = FLinearColor(0.35f, 0.3f, 0.22f);
	/** The jeans' wash over their baked mid-wash indigo (white: as baked; darker for the dark and black washes). */
	FLinearColor JeansTint = FLinearColor::White;
	/** Nothing on the feet (a closet whose Shoes slot is empty, Wardrobe.h): the bare feet show. */
	bool bBarefoot = false;
	/** The hat and the glasses (wardrobe::Cut; Count: none), the hat's cloth and the frames' color. */
	ss::wardrobe::Cut Hat = ss::wardrobe::Cut::Count;
	FLinearColor HatColor = FLinearColor(0.03f, 0.04f, 0.06f);
	ss::wardrobe::Cut Eyes = ss::wardrobe::Cut::Count;
	FLinearColor FrameColor = FLinearColor(0.01f, 0.01f, 0.012f);
	/** Garments may be the body type's (fitted to HeroA1 or HeroB1), not only this face's own fit. */
	bool bTypeGarments = false;
	/**
	 * The modeled shoes (art/blender/assets/sneakers.py: <ShoePair>_L and _R), when imported. They come in their own
	 * colors; when bDyeShoes their canvas, accents and laces take ShoeUpper, ShoeAccent and ShoeLaces below (the rubber,
	 * the lining and the tread stay as baked).
	 */
	const TCHAR* ShoePair = TEXT("SM_Sneaker");
	bool bDyeShoes = false;
	/** The shoes' colors (all six for the shoes made here when the pair isn't imported): the canvas, the rubber (sole and toe cap), the accents (heel tab, stripe), the opening's lining, laces, the tread. */
	FLinearColor ShoeUpper = FLinearColor(0.05f, 0.05f, 0.055f);
	FLinearColor ShoeRubber = FLinearColor(0.8f, 0.78f, 0.72f);
	FLinearColor ShoeAccent = FLinearColor(0.05f, 0.05f, 0.055f);
	FLinearColor ShoeLining = FLinearColor(0.01f, 0.01f, 0.012f);
	FLinearColor ShoeLaces = FLinearColor(0.85f, 0.83f, 0.78f);
	FLinearColor ShoeOutsole = FLinearColor(0.04f, 0.03f, 0.025f);
};

namespace ShortStackCharacterDetail
{
const TCHAR* HeroArchetypeBody = TEXT("/MetaHumanCharacter/Body/IdentityTemplate/SKM_Body.SKM_Body");
const TCHAR* HeroArchetypeFace = TEXT("/MetaHumanCharacter/Face/SKM_Face.SKM_Face");
const TCHAR* SurfaceMaterial = TEXT("/Game/ShortStack/Materials/M_Surface.M_Surface");
const TCHAR* LensMaterial = TEXT("/Game/ShortStack/Materials/M_Lens.M_Lens");
const TCHAR* BasicMaterial = TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");

const FName HeadBone(TEXT("head"));
const FName EyeBoneL(TEXT("FACIAL_L_Eye"));
const FName EyeBoneR(TEXT("FACIAL_R_Eye"));
const FName CurveBlinkL(TEXT("CTRL_expressions_eyeBlinkL"));
const FName CurveBlinkR(TEXT("CTRL_expressions_eyeBlinkR"));
const FName CurveRelaxL(TEXT("CTRL_expressions_eyeRelaxL"));
const FName CurveRelaxR(TEXT("CTRL_expressions_eyeRelaxR"));
const FName CurveJawOpen(TEXT("CTRL_expressions_jawOpen"));

/** A cast member's assembled MetaHuman (backroom_cast.py / hero_cast.py), if it's been built. */
UClass* CastClass(const FString& Name)
{
	const FString Package = FString::Printf(TEXT("/Game/ShortStack/Cast/Built/MHC_%s/BP_MHC_%s"), *Name, *Name);
	if (!FPackageName::DoesPackageExist(Package))
	{
		return nullptr;
	}
	return LoadClass<AActor>(nullptr, *FString::Printf(TEXT("%s.BP_MHC_%s_C"), *Package, *Name));
}

FLinearColor SrgbOf(uint32 Hex)
{
	return FLinearColor::FromSRGBColor(FColor(static_cast<uint8>((Hex >> 16) & 0xff), static_cast<uint8>((Hex >> 8) & 0xff), static_cast<uint8>(Hex & 0xff)));
}

FLinearColor Scaled(const FLinearColor& C, float K)
{
	return FLinearColor(C.R * K, C.G * K, C.B * K, 1.0f);
}

void CopyOverrides(const UMeshComponent* From, UMeshComponent* To)
{
	for (int32 I = 0; I < From->OverrideMaterials.Num(); ++I)
	{
		if (From->OverrideMaterials[I])
		{
			To->SetMaterial(I, From->OverrideMaterials[I]);
		}
	}
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

// Where the hat and glasses sit from the head bone (cm): fitted to each face (the crown from the face mesh, the
// bridge from the eyes) until one is set here, which then holds for every face. The Back Room reads them too.
TAutoConsoleVariable<float> CVarHatUp(TEXT("ss.Wear.HatUp"), 19.0f, TEXT("The hat's crown above the head bone (cm). Fitted to the face until set."));
TAutoConsoleVariable<float> CVarHatForward(TEXT("ss.Wear.HatForward"), 1.5f, TEXT("The hat's crown ahead of the head bone (cm)."));
TAutoConsoleVariable<float> CVarGlassesUp(TEXT("ss.Wear.GlassesUp"), 8.5f, TEXT("The glasses' bridge above the head bone (cm). Fitted to the eyes until set."));
TAutoConsoleVariable<float> CVarGlassesForward(TEXT("ss.Wear.GlassesForward"), 10.5f, TEXT("The glasses' bridge ahead of the head bone (cm). Fitted to the eyes until set."));

/** A tunable's value once someone has set it (the console, an ini), otherwise what was fitted. */
float Tuned(TAutoConsoleVariable<float>& Var, float Fitted)
{
	return (Var->GetFlags() & ECVF_SetByMask) != ECVF_SetByConstructor ? Var.GetValueOnGameThread() : Fitted;
}

/** Hats and glasses from the Blender pipeline (art/blender/assets/wearables.py), when imported. */
UStaticMesh* Wearable(const TCHAR* Name)
{
	return Name ? LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/ShortStack/Meshes/%s/%s.%s"), Name, Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet) : nullptr;
}

using ss::wardrobe::Cut;

/**
 * The street's one set of hats and glasses, by wardrobe::Cut (spec 7.5.4: wear.py's and wearables.py's pieces merged by
 * id, so a piece looks the same on everyone who wears it): the street's own piece (wearables.py, its frame: a hat's origin
 * the top of the head inside it, glasses' the bridge, front -Y), else the Back Room's (wear.py, its frame: the midpoint
 * between the eyes, front +X, made for eyes 6.3 cm apart) until the street's is imported. The pawn's aviators and trilby
 * are among them.
 */
struct FWearPiece
{
	Cut Kind;
	const TCHAR* Street;
	const TCHAR* BackRoom;
	/** Worn turned round (the cap backwards). */
	float Turn;
	/** Gold wire frames (not acetate). */
	bool bGold;
	/** Dark lenses of its own (shades): no clear glass put in. */
	bool bDark;
};
const FWearPiece WearPieces[] = {
	{Cut::Beanie, TEXT("SM_Hat_Beanie"), TEXT("SM_Wear_Beanie"), 0.0f, false, false},
	{Cut::BallCap, TEXT("SM_Hat_Cap"), TEXT("SM_Wear_Cap"), 0.0f, false, false},
	{Cut::CapBack, TEXT("SM_Hat_Cap"), TEXT("SM_Wear_Cap"), 180.0f, false, false},
	{Cut::Bucket, TEXT("SM_Hat_Bucket"), nullptr, 0.0f, false, false},
	{Cut::Trilby, TEXT("SM_Hat_Trilby"), TEXT("SM_Wear_Trilby"), 0.0f, false, false},
	{Cut::Round, TEXT("SM_Glasses_Round"), TEXT("SM_Wear_Glasses"), 0.0f, false, false},
	{Cut::Square, TEXT("SM_Glasses_Square"), TEXT("SM_Wear_Glasses"), 0.0f, false, false},
	{Cut::Wire, TEXT("SM_Glasses_Wire"), nullptr, 0.0f, true, false},
	{Cut::Shades, TEXT("SM_Glasses_Shades"), TEXT("SM_Wear_Shades"), 0.0f, false, true},
	{Cut::Aviators, TEXT("SM_Glasses_Aviators"), TEXT("SM_Wear_Aviators"), 0.0f, true, true},
};

const FWearPiece* WearPieceFor(Cut Kind)
{
	for (const FWearPiece& P : WearPieces)
	{
		if (P.Kind == Kind)
		{
			return &P;
		}
	}
	return nullptr;
}

bool IsHatCut(Cut Kind)
{
	return Kind == Cut::Beanie || Kind == Cut::BallCap || Kind == Cut::CapBack || Kind == Cut::Bucket || Kind == Cut::Trilby;
}

bool IsEyesCut(Cut Kind)
{
	return Kind == Cut::Round || Kind == Cut::Square || Kind == Cut::Wire || Kind == Cut::Shades || Kind == Cut::Aviators;
}

bool IsOuterCut(Cut Kind)
{
	return Kind == Cut::Hoodie || Kind == Cut::Bomber || Kind == Cut::Flannel || Kind == Cut::Denim || Kind == Cut::Leather || Kind == Cut::Track;
}

bool IsTrousersCut(Cut Kind)
{
	return Kind == Cut::Jeans || Kind == Cut::Chinos || Kind == Cut::WorkPants;
}

// The creator's headwear (1 beanie, 2 cap, 3 cap backwards, 4 bucket hat) and glasses (1 round, 2 square, 3 wire, 4 shades).
const Cut CreatorHats[5] = {Cut::Count, Cut::Beanie, Cut::BallCap, Cut::CapBack, Cut::Bucket};
const Cut CreatorSpecs[5] = {Cut::Count, Cut::Round, Cut::Square, Cut::Wire, Cut::Shades};

/** The garment mesh family a cut is drawn from (garments.py's SK_Garment_<Family>_<fit>); null for what isn't a garment mesh. */
const TCHAR* GarmentFamily(Cut Kind)
{
	switch (Kind)
	{
	case Cut::Hoodie:
		return TEXT("Hoodie");
	case Cut::Bomber:
		return TEXT("Jacket");
	case Cut::Flannel:
		return TEXT("Flannel");
	case Cut::Denim:
		return TEXT("Denim");
	case Cut::Leather:
		return TEXT("Leather");
	case Cut::Track:
		return TEXT("Track");
	case Cut::Jeans:
		return TEXT("Jeans");
	case Cut::Chinos:
		return TEXT("Chinos");
	case Cut::WorkPants:
		return TEXT("WorkPants");
	default:
		return nullptr;
	}
}

// Kinds the 3D won't sell even with a mesh (a garment that clips on some build in the lead's PIE check), a bit per Cut.
TAutoConsoleVariable<int32> CVarDenyGarments(TEXT("ss.Look.DenyGarments"), 0,
	TEXT("A bit per wardrobe::Cut kept out of the shops' HostCaps even with a mesh (one that clips on a build). 0: none."));
// Non-player characters may wear their body type's garments (fitted to HeroA1/HeroB1) when none is fitted to their own face.
TAutoConsoleVariable<int32> CVarNpcGarments(TEXT("ss.Look.NpcGarments"), 0,
	TEXT("1: people other than the player wear their body type's garments (fitted to HeroA1/HeroB1) when none is fitted to their face; 0: only their own fit. "
		 "ss.Look.Redress applies it."));

/** A print mask from art/blender/prints.py (imported by shortstack_setup.import_textures), when imported. */
UTexture2D* ClothingTexture(const TCHAR* Name)
{
	return Name ? LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/ShortStack/Textures/Clothing/%s.%s"), Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet) : nullptr;
}

/**
 * Tints one of an imported prop's material slots. The Blender props bake those surfaces light grey and the glTF
 * importer's material multiplies its base color texture by a factor (BaseColorFactor; BaseColorFactor_RGB in this
 * engine's), which takes the tint; Lift brightens it back by about as much as the bake's grey darkens it.
 */
void TintSlot(UMeshComponent* Mesh, int32 Slot, const FLinearColor& Tint, float Lift)
{
	UMaterialInterface* Material = Mesh && Slot < Mesh->GetNumMaterials() ? Mesh->GetMaterial(Slot) : nullptr;
	if (!Material)
	{
		return;
	}
	UMaterialInstanceDynamic* Mid = Cast<UMaterialInstanceDynamic>(Material);
	if (!Mid)
	{
		Mid = UMaterialInstanceDynamic::Create(Material, Mesh);
		Mesh->SetMaterial(Slot, Mid);
	}
	const FLinearColor Lifted = Scaled(Tint, Lift);
	bool bTinted = false;
	TArray<FMaterialParameterInfo> Infos;
	TArray<FGuid> Ids;
	Mid->GetAllVectorParameterInfo(Infos, Ids);
	for (const FMaterialParameterInfo& Info : Infos)
	{
		const FString Key = Info.Name.ToString().ToLower().Replace(TEXT("_"), TEXT("")).Replace(TEXT(" "), TEXT(""));
		if (Key == TEXT("basecolorfactor") || Key == TEXT("basecolorfactorrgb") || Key == TEXT("basecolor") || Key == TEXT("basecolortint") || Key == TEXT("tint"))
		{
			Mid->SetVectorParameterValue(Info.Name, Lifted);
			bTinted = true;
		}
	}
	if (!bTinted)
	{
		// The listing came back empty (a parent not loaded yet): the importer's own name, set blind.
		Mid->SetVectorParameterValue(TEXT("BaseColorFactor"), Lifted);
	}
}

/**
 * The slot an imported prop's Blender material landed in. The bake names each material after its Blender slot
 * ("M_SM_Hat_Cap_2"), and the importer needn't keep Blender's order; Index itself when no name says.
 */
int32 BakedSlot(const UMeshComponent* Mesh, int32 Index)
{
	const FString Suffix = FString::Printf(TEXT("_%d"), Index);
	const TArray<FName> Slots = Mesh->GetMaterialSlotNames();
	for (int32 Slot = 0; Slot < Slots.Num(); ++Slot)
	{
		if (Slots[Slot].ToString().EndsWith(Suffix))
		{
			return Slot;
		}
	}
	// Unnamed slots: the materials' names (not a tint's instance, whose made-up name could end the same way).
	for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
	{
		const UMaterialInterface* Material = Mesh->GetMaterial(Slot);
		if (Material && !Material->IsA<UMaterialInstanceDynamic>() && Material->GetName().EndsWith(Suffix))
		{
			return Slot;
		}
	}
	return Index;
}

// ------------------------------------------------------------------ hair

using HeroGrooms::EGroomKind;

/** Which groom a MetaHuman Blueprint's node holds, by its variable (Hair, Eyebrows, Beard, ...) or its asset (Beard_S_Stubble). */
EGroomKind GroomKindOf(const FString& Variable, const FString& Asset)
{
	return HeroGrooms::KindOf(Variable, Asset);
}

/** The grooms the library swaps (the scalp, the brows, the beard and the mustache); the lashes and the peach fuzz stay as built. */
bool IsManagedGroom(EGroomKind Kind)
{
	return Kind == EGroomKind::Scalp || Kind == EGroomKind::Brows || Kind == EGroomKind::Beard || Kind == EGroomKind::Mustache;
}

/** The creator's hair colors in the MetaHuman hair material's terms: melanin, redness and grey. */
struct FHairShade
{
	float Melanin;
	float Redness;
	float White;
};

FHairShade HairShadeOf(int32 Index, int32 Age)
{
	// Black, dark brown, brown, auburn, copper, blonde, platinum, grey.
	static const FHairShade Table[8] = {{0.95f, 0.08f, 0.0f}, {0.75f, 0.2f, 0.0f}, {0.55f, 0.28f, 0.0f}, {0.45f, 0.7f, 0.0f}, {0.3f, 0.95f, 0.0f},
		{0.13f, 0.3f, 0.0f}, {0.04f, 0.05f, 0.3f}, {0.35f, 0.05f, 0.85f}};
	const int32 I = FMath::Clamp(Index, 0, 7);
	FHairShade Shade = Table[I];
	if (I < 6)
	{
		// Greying as the portrait does it (ss::hero::HairToneAt): from 44 the color drains and silver comes in, at most
		// two thirds of the way by 64. Platinum and grey are there already.
		const float Grey = FMath::Clamp((static_cast<float>(Age) - 44.0f) / 30.0f, 0.0f, 0.65f);
		Shade.White = FMath::Max(Shade.White, Grey * 0.8f);
		Shade.Redness *= 1.0f - 0.6f * FMath::Min(1.0f, Grey * 1.6f);
	}
	return Shade;
}

/** Colors a groom: the hair materials name the parameters hairMelanin/hairRedness/WhiteAmount, the peach fuzz Melanin/Redness. */
void ShadeGroom(UGroomComponent* Groom, const FHairShade& Shade, float Darker)
{
	const float Melanin = FMath::Min(1.0f, Shade.Melanin + Darker);
	for (int32 Slot = 0; Slot < Groom->GetNumMaterials(); ++Slot)
	{
		if (UMaterialInterface* Material = Groom->GetMaterial(Slot))
		{
			// Shaded again in place (a new hair color at the barber): its own instance, made the first time.
			UMaterialInstanceDynamic* Mid = Cast<UMaterialInstanceDynamic>(Material);
			if (!Mid || Mid->GetOuter() != Groom)
			{
				Mid = UMaterialInstanceDynamic::Create(Mid && Mid->Parent ? Mid->Parent.Get() : Material, Groom);
			}
			Mid->SetScalarParameterValue(TEXT("hairMelanin"), Melanin);
			Mid->SetScalarParameterValue(TEXT("hairRedness"), Shade.Redness);
			Mid->SetScalarParameterValue(TEXT("WhiteAmount"), Shade.White);
			Mid->SetScalarParameterValue(TEXT("Melanin"), Melanin);
			Mid->SetScalarParameterValue(TEXT("Redness"), Shade.Redness);
			Groom->SetMaterial(Slot, Mid);
		}
	}
}

/**
 * The Blueprint's LOD sync drives its grooms from the face; without it a groom falls back to its helmet (a painted-on
 * cap of hair). Builds below the cinematic tier carry cards, not strands: hold them on their nearest cards (HeroGrooms.h
 * holds them by detail tier once one is set).
 */
void HoldOnCards(UGroomComponent* Groom)
{
	HeroGrooms::HoldOnCards(Groom);
}

/** A groom component's kind as it was worn (the scalp's, by default: a groom added for the hair). */
EGroomKind KindIn(const TArray<TPair<TWeakObjectPtr<UGroomComponent>, uint8>>& Kinds, const UGroomComponent* Groom)
{
	for (const TPair<TWeakObjectPtr<UGroomComponent>, uint8>& K : Kinds)
	{
		if (K.Key.Get() == Groom)
		{
			return static_cast<EGroomKind>(K.Value);
		}
	}
	return EGroomKind::Scalp;
}

// Where the gaze rests at work: the hands' work at a counter, 45 cm ahead of the station's mark and 95 cm up.
const FVector WorkLookAt(45.0, 0.0, 95.0);

// ------------------------------------------------------------------ clothes

/**
 * A garment's color is core's (wardrobe::Tone: the thrift's swatch, the portrait's and the 3D's tint, so the three agree):
 * jackets and tees the creator's outfit tones plus a white and a heather tee; trousers, shorts and shoes their slot's own.
 * Under each jacket color with no trousers named, the shorts' tone (charcoal, navy, olive, burgundy, sand, black, teal,
 * mustard: denim, charcoal, dark brown, black, denim, grey, charcoal, black).
 */
const uint32 PantsFor[8] = {0x283244, 0x2b2c31, 0x3a3128, 0x1d1d21, 0x283447, 0x3b3d43, 0x2b2c31, 0x1d1d21};
// The jeans bake one mid-wash indigo; the catalog's other washes (Wardrobe.cpp: 0 dark, 1 mid, 2 light, 3 black) are a
// multiply over it (linear), lifting no faded thread past white.
const FLinearColor JeansWashes[4] = {FLinearColor(0.6f, 0.62f, 0.68f), FLinearColor::White, FLinearColor(1.38f, 1.34f, 1.28f), FLinearColor(0.27f, 0.26f, 0.27f)};
// The mid wash, as the table draws trousers it can't model (and the jeans worn with no closet).
const uint32 MidWashTone = 0x3d5c86;

FLinearColor ToneOf(const ss::wardrobe::Garment& G)
{
	return SrgbOf(ss::wardrobe::Tone(G));
}

/** A graphic's ink, kept readable on its shirt (the Back Room's rule): too close in lightness, pushed light or dark. */
FLinearColor LegibleInk(const FLinearColor& Ink, const FLinearColor& Shirt)
{
	const float Ground = FMath::Sqrt(Shirt.GetLuminance());
	if (FMath::Abs(FMath::Sqrt(Ink.GetLuminance()) - Ground) >= 0.3f)
	{
		return Ink;
	}
	return Ground < 0.45f ? FMath::Lerp(Ink, FLinearColor(0.80f, 0.76f, 0.68f), 0.75f) : FMath::Lerp(Ink, FLinearColor(0.012f, 0.012f, 0.016f), 0.8f);
}

/**
 * A garment the closet names: the catalog's, or, for an id the catalog doesn't have (yet), read from the id itself
 * ("outer.denim.navy": the slot, the cut, the outfit color's name last; "top.graphic.sundaymajor" is a tee).
 */
bool GarmentOf(const std::string& Id, ss::wardrobe::Garment& Out)
{
	if (Id.empty())
	{
		return false;
	}
	if (const ss::wardrobe::Garment* G = ss::wardrobe::Find(Id))
	{
		Out = *G;
		return true;
	}
	TArray<FString> Parts;
	FString(UTF8_TO_TCHAR(Id.c_str())).ParseIntoArray(Parts, TEXT("."));
	if (Parts.Num() < 2)
	{
		return false;
	}
	static const TCHAR* const CutNames[] = {TEXT("tee"), TEXT("hoodie"), TEXT("bomber"), TEXT("flannel"), TEXT("denim"), TEXT("leather"), TEXT("track"), TEXT("shorts"),
		TEXT("jeans"), TEXT("chinos"), TEXT("workpants"), TEXT("sneakers"), TEXT("workshoes"), TEXT("beanie"), TEXT("ballcap"), TEXT("capback"), TEXT("bucket"),
		TEXT("trilby"), TEXT("round"), TEXT("square"), TEXT("wire"), TEXT("shades"), TEXT("aviators")};
	static_assert(UE_ARRAY_COUNT(CutNames) == static_cast<size_t>(Cut::Count), "a name per wardrobe::Cut");
	int32 Kind = INDEX_NONE;
	for (int32 I = 0; I < static_cast<int32>(UE_ARRAY_COUNT(CutNames)) && Kind == INDEX_NONE; ++I)
	{
		if (Parts[1].Equals(CutNames[I], ESearchCase::IgnoreCase))
		{
			Kind = I;
		}
	}
	if (Kind == INDEX_NONE && (Parts[0].Equals(TEXT("top"), ESearchCase::IgnoreCase)))
	{
		Kind = static_cast<int32>(Cut::Tee);
	}
	if (Kind == INDEX_NONE)
	{
		return false;
	}
	Out = ss::wardrobe::Garment();
	Out.Id = Id;
	Out.Kind = static_cast<Cut>(Kind);
	Out.Where = ss::wardrobe::SlotOf(Out.Kind);
	Out.Color = 0;
	for (int32 C = 0; C < ss::hero::OptionCount(ss::hero::Slot::OutfitColor); ++C)
	{
		if (Parts.Last().Equals(UTF8_TO_TCHAR(ss::hero::OptionName(ss::hero::Slot::OutfitColor, C).c_str()), ESearchCase::IgnoreCase))
		{
			Out.Color = C;
		}
	}
	return true;
}

/** Canvas court sneakers in a canvas color: off-white rubber, the accent on the heel tab and the stripe. */
void DyeSneakers(FStreetOutfit& O, const FLinearColor& Canvas, const FLinearColor& Accent)
{
	const bool bDark = Canvas.GetLuminance() < 0.2f;
	O.ShoeUpper = Canvas;
	O.ShoeRubber = SrgbOf(bDark ? 0xe4dfd2 : 0xece7db);
	O.ShoeAccent = Accent;
	O.ShoeLining = SrgbOf(0x1a1a1d);
	O.ShoeLaces = SrgbOf(bDark ? 0x2a2b30 : 0xefebe1);
	O.ShoeOutsole = SrgbOf(0x3b3029);
	// The modeled sneaker in the same colors, where its bake takes a tint (the canvas, the stripe, heel tape and pull tab,
	// the laces); its off-white rubber and gum tread stay as baked.
	O.ShoePair = TEXT("SM_Sneaker");
	O.bDyeShoes = true;
}

/** The work shoes: baked black all over, never dyed. */
void WorkShoes(FStreetOutfit& O)
{
	O.ShoePair = TEXT("SM_WorkShoe");
	O.bDyeShoes = false;
	O.ShoeUpper = SrgbOf(0x161619);
	O.ShoeRubber = SrgbOf(0x232327);
	O.ShoeAccent = SrgbOf(0x161619);
	O.ShoeLining = SrgbOf(0x111113);
	O.ShoeLaces = SrgbOf(0x161619);
	O.ShoeOutsole = SrgbOf(0x101012);
}

/** A shirt's print (the persona's numbering, 1..7) in tones of the shirt: the wide bands darker, a pale thread, darkest where they cross. */
void PrintIn(FStreetOutfit& O, int32 Print, const FLinearColor* InkA, const FLinearColor* InkB, const FLinearColor* InkC)
{
	O.Print = CastLooks::PrintTexture(Print);
	O.PrintIndex = O.Print ? Print : 0;
	O.PrintTiling = CastLooks::PrintTiling(Print);
	O.PrintA = InkA ? *InkA : Scaled(O.Shirt, 0.42f);
	O.PrintB = InkB ? *InkB : FMath::Lerp(O.Shirt, SrgbOf(0xd9cfb8), 0.7f);
	O.PrintC = InkC ? *InkC : Scaled(O.Shirt, 0.2f);
}

/** A chest graphic (the persona's numbering) in two inks kept readable on the shirt, and a third between ink and shirt. */
void GraphicIn(FStreetOutfit& O, int32 Graphic, const FLinearColor& InkA, const FLinearColor& InkB)
{
	O.Graphic = CastLooks::GraphicTexture(Graphic);
	O.GraphicIndex = O.Graphic ? Graphic : 0;
	O.GraphicA = LegibleInk(InkA, O.Shirt);
	O.GraphicB = LegibleInk(InkB, O.Shirt);
	O.GraphicC = FMath::Lerp(O.GraphicA, O.Shirt, 0.35f);
}

/**
 * The hero's clothes. With a seeded closet, what it says is worn (spec 7.1): the tee in its color with its print or graphic
 * ("" in Top: the tee in the outfit color), the jacket ("" in Outer: none, the tee alone), the trousers ("" in Bottom: the
 * shorts), the shoes, the hat and the glasses. Without one (a career before the closet), the creator's choices: the jacket
 * in its color over jeans (Outfit 6, None: the tee alone in that color), the hat and glasses, white sneakers trimmed in it.
 */
FStreetOutfit HeroOutfit(const ss::hero::Look& L, int32 Age, const ss::wardrobe::Closet* Closet)
{
	using ss::wardrobe::Slot;
	FStreetOutfit O;
	O.bTypeGarments = true;
	int32 Color = FMath::Clamp(L.OutfitColor, 0, 7);
	O.Shirt = SrgbOf(ss::hero::OutfitTone(Color));
	O.OuterColor = O.Shirt;
	O.Outer = L.Outfit >= 0 && L.Outfit < ss::hero::OutfitNone ? static_cast<Cut>(L.Outfit + 1) : Cut::Count;
	O.Bottom = Cut::Jeans;
	O.Hat = CreatorHats[FMath::Clamp(L.Hat, 0, 4)];
	O.Eyes = CreatorSpecs[FMath::Clamp(L.Glasses, 0, 4)];
	// Jeans with no closet: the mid wash as baked.
	O.BottomColor = SrgbOf(MidWashTone);
	Cut Shoes = Cut::Sneakers;
	bool bShoesFromCloset = false;
	FLinearColor Canvas = FLinearColor::White;
	int32 CanvasIndex = 0;
	bool bTopFromCloset = false;
	bool bShortsNamed = false;
	bool bFramesFromCloset = false;
	FLinearColor ClosetFrames = FLinearColor::Black;
	if (Closet && Closet->Seeded)
	{
		ss::wardrobe::Garment G;
		auto Worn = [Closet, &G](Slot S) { return GarmentOf(Closet->Worn[static_cast<int32>(S)], G); };
		O.Outer = Cut::Count;
		if (Worn(Slot::Outer) && IsOuterCut(G.Kind))
		{
			O.Outer = G.Kind;
			Color = FMath::Clamp(G.Color, 0, 7);
			O.OuterColor = ToneOf(G);
		}
		O.Shirt = SrgbOf(ss::hero::OutfitTone(Color));
		if (Worn(Slot::Top) && G.Kind == Cut::Tee)
		{
			bTopFromCloset = true;
			// Core's tone: the creator's eight, and the white and heather tees.
			O.Shirt = ToneOf(G);
			if (G.Print >= 0)
			{
				PrintIn(O, G.Print + 1, nullptr, nullptr, nullptr);
			}
			if (G.Graphic >= 0)
			{
				GraphicIn(O, G.Graphic + 1, SrgbOf(0xefe9dc), SrgbOf(0x8f1d2c));
			}
		}
		O.Bottom = Cut::Shorts;
		if (Worn(Slot::Bottom) && (IsTrousersCut(G.Kind) || G.Kind == Cut::Shorts))
		{
			O.Bottom = G.Kind;
			O.BottomColor = ToneOf(G);
			bShortsNamed = G.Kind == Cut::Shorts;
			if (G.Kind == Cut::Jeans)
			{
				O.JeansTint = JeansWashes[FMath::Clamp(G.Color, 0, 3)];
			}
		}
		if (Worn(Slot::Shoes) && (G.Kind == Cut::Sneakers || G.Kind == Cut::WorkShoes))
		{
			Shoes = G.Kind;
			bShoesFromCloset = true;
			Canvas = ToneOf(G);
			CanvasIndex = G.Color;
		}
		else
		{
			// "" in Shoes is barefoot (Wardrobe.h): the closet's Take off.
			O.bBarefoot = true;
		}
		O.Hat = Worn(Slot::Hat) && IsHatCut(G.Kind) ? G.Kind : Cut::Count;
		if (Worn(Slot::Eyes) && IsEyesCut(G.Kind))
		{
			O.Eyes = G.Kind;
			bFramesFromCloset = true;
			ClosetFrames = ToneOf(G);
		}
		else
		{
			O.Eyes = Cut::Count;
		}
	}
	// The shorts the closet names (khaki, grey, navy), else the shorts' tone under the jacket.
	O.Pants = bShortsNamed ? O.BottomColor : SrgbOf(PantsFor[Color]);
	if (!bTopFromCloset && O.Outer == Cut::Flannel)
	{
		// Flannel with no flannel mesh to wear: the tee carries its plaid in the jacket's color (as before the garments).
		PrintIn(O, 4, nullptr, nullptr, nullptr);
	}
	ss::hero::Look ForHat = L;
	ForHat.OutfitColor = Color;
	O.HatColor = SrgbOf(ss::hero::HatTone(ForHat, Age));
	const FWearPiece* Frames = WearPieceFor(O.Eyes);
	O.FrameColor = bFramesFromCloset ? ClosetFrames : SrgbOf(Frames && Frames->bGold ? 0xc9a24d : 0x161618);
	if (Shoes == Cut::WorkShoes)
	{
		// The modeled work shoe bakes its own black leather (sneakers.py): a brown pair reads black until it bakes light.
		WorkShoes(O);
	}
	else if (bShoesFromCloset)
	{
		// The catalog's canvas (white, black, grey): a red heel tab and stripe on the white, off-white on the others.
		DyeSneakers(O, Canvas, CanvasIndex == 0 ? SrgbOf(0x8f1d2c) : SrgbOf(0xd6d0c0));
	}
	else
	{
		// Canvas court shoes, off-white with the jacket's color on the heel tab and the stripe (a red one when the jacket is
		// charcoal or black); black canvas under a light jacket.
		const bool bLightJacket = Color == 4 || Color == 7;
		DyeSneakers(O, SrgbOf(bLightJacket ? 0x1d1e22 : 0xd6d0c0), Color == 0 || Color == 5 ? SrgbOf(0x8f1d2c) : SrgbOf(ss::hero::OutfitTone(Color)));
	}
	return O;
}

/** Someone at work or walking past: their wardrobe (CastLooks.h), their shoes. */
FStreetOutfit CastOutfit(const FCastWear& W)
{
	FStreetOutfit O;
	O.Shirt = W.Shirt;
	O.Pants = W.Pants;
	if (W.Print > 0)
	{
		const FLinearColor C = W.InkC.A > 0.0f ? W.InkC : FMath::Lerp(W.InkA, W.InkB, 0.5f);
		PrintIn(O, W.Print, &W.InkA, &W.InkB, &C);
	}
	if (W.Graphic > 0)
	{
		GraphicIn(O, W.Graphic, W.InkA, W.InkB);
	}
	O.Outer = IsOuterCut(W.Outer) ? W.Outer : Cut::Count;
	O.OuterColor = W.OuterColor;
	O.Bottom = IsTrousersCut(W.Bottom) ? W.Bottom : Cut::Shorts;
	O.BottomColor = W.Pants;
	O.Hat = IsHatCut(W.Hat) ? W.Hat : Cut::Count;
	O.HatColor = W.HatColor;
	O.Eyes = IsEyesCut(W.Eyes) ? W.Eyes : Cut::Count;
	O.FrameColor = W.FrameColor;
	O.bTypeGarments = CVarNpcGarments.GetValueOnGameThread() != 0;
	if (W.bWorkShoes)
	{
		WorkShoes(O);
	}
	else
	{
		DyeSneakers(O, W.ShoeColor, W.Shirt);
	}
	return O;
}

/** The default garment's shirt (its material, or a parent of it, is named for it); everything else it wears is the shorts. */
bool IsShirtMaterial(const UMaterialInterface* Material)
{
	const UMaterialInterface* At = Material;
	for (int32 Depth = 0; At && Depth < 6; ++Depth)
	{
		if (At->GetName().Contains(TEXT("Shirt")))
		{
			return true;
		}
		const UMaterialInstance* Instance = Cast<UMaterialInstance>(At);
		At = Instance ? Instance->Parent.Get() : nullptr;
	}
	return false;
}

/**
 * A garment's package (art/blender/assets/garments.py, imported by garments_setup.py onto the hero bodies' skeleton) for a
 * family ("Jeans", "Jacket", "Hoodie") and a face: the one fitted to this very face (SK_Garment_Jacket_HeroA0, or a staff
 * member's) if there is one, else, when bTypeDefault, the body type's (SK_Garment_Jacket_A, fitted to HeroA1 or HeroB1).
 * Empty when neither is imported. No loads.
 */
FString GarmentPackage(const TCHAR* Family, const FString& Face, bool bTypeB, bool bTypeDefault)
{
	if (!Family)
	{
		return FString();
	}
	for (const FString& Fit : {Face, bTypeDefault ? FString(bTypeB ? TEXT("B") : TEXT("A")) : FString()})
	{
		if (Fit.IsEmpty())
		{
			continue;
		}
		const FString Package = FString::Printf(TEXT("/Game/ShortStack/Garments/SK_Garment_%s_%s"), Family, *Fit);
		// Most faces have no fit of their own: don't try to load what isn't there.
		if (FPackageName::DoesPackageExist(Package))
		{
			return Package;
		}
	}
	return FString();
}

USkeletalMesh* GarmentMesh(const TCHAR* Family, const FString& Face, bool bTypeB, bool bTypeDefault)
{
	const FString Package = GarmentPackage(Family, Face, bTypeB, bTypeDefault);
	return Package.IsEmpty() ? nullptr
							 : LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("%s.%s"), *Package, *FPackageName::GetShortName(Package)), nullptr, LOAD_NoWarn | LOAD_Quiet);
}

/** Who may wear the body type's garments: the hero MetaHumans (HeroA0..HeroB2) the type garments are made for. */
bool IsHeroFace(const FString& Face)
{
	return Face.StartsWith(TEXT("HeroA")) || Face.StartsWith(TEXT("HeroB"));
}

/**
 * Every garment a hero face can wear, loaded asynchronously and held (one face at a time: the player's), so a try-on at the
 * thrift or a change at the closet finds its mesh in memory instead of loading it on the key press, as the Style Book's
 * grooms are preloaded. The street's hats and glasses, the modeled shoes and the tees' prints and graphics come with them
 * (small, and any of them can be tried on). Asked for as the hero is dressed (under black).
 */
void HoldHeroGarments(const FString& Face, bool bTypeB)
{
	static FStreamableManager Streams;
	static TSharedPtr<FStreamableHandle> Held;
	static FString HeldFace;
	if (Face == HeldFace && Held.IsValid())
	{
		return;
	}
	TArray<FSoftObjectPath> Paths;
	for (const Cut C : {Cut::Hoodie, Cut::Bomber, Cut::Flannel, Cut::Denim, Cut::Leather, Cut::Track, Cut::Jeans, Cut::Chinos, Cut::WorkPants})
	{
		const FString Package = GarmentPackage(GarmentFamily(C), Face, bTypeB, IsHeroFace(Face));
		if (!Package.IsEmpty())
		{
			Paths.AddUnique(FSoftObjectPath(FString::Printf(TEXT("%s.%s"), *Package, *FPackageName::GetShortName(Package))));
		}
	}
	// Only what's imported (a missing package would warn on every dress).
	auto HoldIfThere = [&Paths](const FString& Package) {
		if (FPackageName::DoesPackageExist(Package))
		{
			Paths.AddUnique(FSoftObjectPath(FString::Printf(TEXT("%s.%s"), *Package, *FPackageName::GetShortName(Package))));
		}
	};
	auto MeshPackage = [](const TCHAR* Name) { return FString::Printf(TEXT("/Game/ShortStack/Meshes/%s/%s"), Name, Name); };
	for (const FWearPiece& P : WearPieces)
	{
		for (const TCHAR* Name : {P.Street, P.BackRoom})
		{
			if (Name)
			{
				HoldIfThere(MeshPackage(Name));
			}
		}
	}
	for (const TCHAR* Shoe : {TEXT("SM_Sneaker_L"), TEXT("SM_Sneaker_R"), TEXT("SM_WorkShoe_L"), TEXT("SM_WorkShoe_R")})
	{
		HoldIfThere(MeshPackage(Shoe));
	}
	for (int32 I = 1; I <= FMath::Max(CastLooks::PrintCount(), CastLooks::GraphicCount()); ++I)
	{
		for (const TCHAR* Texture : {CastLooks::PrintTexture(I), CastLooks::GraphicTexture(I)})
		{
			if (Texture)
			{
				HoldIfThere(FString::Printf(TEXT("/Game/ShortStack/Textures/Clothing/%s"), Texture));
			}
		}
	}
	// Asked for before the last face's are let go, so what both share never leaves memory.
	TSharedPtr<FStreamableHandle> Next;
	if (Paths.Num() > 0)
	{
		Next = Streams.RequestAsyncLoad(Paths, FStreamableDelegate(), FStreamableManager::DefaultAsyncLoadPriority, true);
	}
	if (Held.IsValid())
	{
		Held->ReleaseHandle();
	}
	Held = Next;
	HeldFace = Face;
}

/** The Creator's one garment, the tee and shorts, as copied onto us from the MetaHuman (its asset, or its materials, say so). */
bool IsDefaultOutfit(const USkinnedMeshComponent* Skinned)
{
	const USkinnedAsset* Asset = Skinned ? Skinned->GetSkinnedAsset() : nullptr;
	if (!Asset)
	{
		return false;
	}
	if (Asset->GetName().Contains(TEXT("Outfit")))
	{
		return true;
	}
	for (int32 Slot = 0; Slot < Skinned->GetNumMaterials(); ++Slot)
	{
		const UMaterialInterface* Material = Skinned->GetMaterial(Slot);
		if (Material && (IsShirtMaterial(Material) || Material->GetName().Contains(TEXT("Short"))))
		{
			return true;
		}
	}
	return false;
}

/** Dyes one of the MetaHuman default garment's materials, as the Back Room's DressOutfit does. */
void DressGarment(UMaterialInstanceDynamic* Mid, bool bShirt, const FStreetOutfit& O)
{
	const FLinearColor Base = bShirt ? O.Shirt : O.Pants;
	Mid->SetVectorParameterValue(TEXT("diffuse_color_1"), Base);
	Mid->SetVectorParameterValue(TEXT("diffuse_color_2"), Base);
	UTexture2D* Map = bShirt ? ClothingTexture(O.Print) : nullptr;
	if (Map)
	{
		Mid->SetTextureParameterValue(TEXT("Print1Map"), Map);
		Mid->SetScalarParameterValue(TEXT("Print1Strength"), 1.0f);
		Mid->SetScalarParameterValue(TEXT("Print1Tiling"), O.PrintTiling);
		Mid->SetVectorParameterValue(TEXT("Print1ColorA"), O.PrintA);
		Mid->SetVectorParameterValue(TEXT("Print1ColorB"), O.PrintB);
		Mid->SetVectorParameterValue(TEXT("Print1ColorC"), O.PrintC);
	}
	else
	{
		Mid->SetScalarParameterValue(TEXT("Print1Strength"), 0.0f);
	}
	UTexture2D* Graphic = bShirt ? ClothingTexture(O.Graphic) : nullptr;
	if (Graphic)
	{
		// On the chest, where the Back Room puts it (BackRoomPlayer.cpp's DressOutfit: the same tee, the same layout, the same
		// ss.Wear.Graphic* tunables); laid on by the graphic's alpha.
		auto Tunable = [](const TCHAR* Name, float Default) {
			const IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name);
			return Var ? Var->GetFloat() : Default;
		};
		Mid->SetTextureParameterValue(TEXT("PrintGraphicMap"), Graphic);
		Mid->SetScalarParameterValue(TEXT("PrintGraphicStrength"), 1.0f);
		Mid->SetScalarParameterValue(TEXT("PrintScale"), Tunable(TEXT("ss.Wear.GraphicScale"), 5.0f));
		Mid->SetScalarParameterValue(TEXT("PrintGraphicOffsetU"), Tunable(TEXT("ss.Wear.GraphicU"), -0.83f));
		Mid->SetScalarParameterValue(TEXT("PrintGraphicOffsetV"), Tunable(TEXT("ss.Wear.GraphicV"), -2.1f));
		Mid->SetVectorParameterValue(TEXT("PrintGraphicColorA"), O.GraphicA);
		Mid->SetVectorParameterValue(TEXT("PrintGraphicColorB"), O.GraphicB);
		Mid->SetVectorParameterValue(TEXT("PrintGraphicColorC"), O.GraphicC);
	}
	else
	{
		Mid->SetScalarParameterValue(TEXT("PrintGraphicStrength"), 0.0f);
	}
}

/** Dyes the MetaHuman's own garment (the tee and shorts) in place: its dynamic instances made once, then their parameters set. */
void DyeDefaultOutfit(USkinnedMeshComponent* Skinned, const FStreetOutfit& O)
{
	if (!Skinned)
	{
		return;
	}
	for (int32 Slot = 0; Slot < Skinned->GetNumMaterials(); ++Slot)
	{
		UMaterialInterface* Material = Skinned->GetMaterial(Slot);
		if (!Material)
		{
			continue;
		}
		UMaterialInstanceDynamic* Mid = Cast<UMaterialInstanceDynamic>(Material);
		if (!Mid)
		{
			Mid = UMaterialInstanceDynamic::Create(Material, Skinned);
			Skinned->SetMaterial(Slot, Mid);
		}
		DressGarment(Mid, IsShirtMaterial(Mid), O);
	}
}

/**
 * Shows or hides a piece worn on the body. What's worn casts its shadow even while kept from the player's own view
 * (bCastHiddenShadow); a piece put away mustn't, or its shadow would stay behind.
 */
void Reveal(UPrimitiveComponent* Piece)
{
	if (Piece)
	{
		Piece->SetCastHiddenShadow(true);
		Piece->SetVisibility(true);
	}
}

void Conceal(UPrimitiveComponent* Piece)
{
	if (Piece)
	{
		Piece->SetVisibility(false);
		Piece->SetCastHiddenShadow(false);
	}
}

/**
 * The MetaHuman's own tee and shorts, each shown only where nothing covers it: its material sections whose material is the
 * shirt's (or isn't), at every LOD, hidden or shown; with neither shown the mesh goes (the body has no skin under them, and
 * the garments are made to cover where they were).
 */
void ShowOutfitParts(USkinnedMeshComponent* Outfit, bool bShirt, bool bShorts)
{
	if (!Outfit || !Outfit->GetSkinnedAsset())
	{
		return;
	}
	if (!bShirt && !bShorts)
	{
		Conceal(Outfit);
		return;
	}
	for (int32 Lod = 0; Lod < Outfit->GetNumLODs(); ++Lod)
	{
		for (int32 Slot = 0; Slot < Outfit->GetNumMaterials(); ++Slot)
		{
			const bool bIsShirt = IsShirtMaterial(Outfit->GetMaterial(Slot));
			Outfit->ShowMaterialSection(Slot, INDEX_NONE, bIsShirt ? bShirt : bShorts, Lod);
		}
	}
	Reveal(Outfit);
}

/** A plain lit surface in one color (M_Surface, or the engine's basic shape material before the project's is built). */
UMaterialInstanceDynamic* PlainSurface(UMaterialInterface* Base, UObject* Outer, const FLinearColor& Color, float Roughness)
{
	UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Base, Outer);
	Mid->SetVectorParameterValue(TEXT("BaseColor"), Color);
	Mid->SetVectorParameterValue(TEXT("Color"), Color);
	Mid->SetScalarParameterValue(TEXT("Roughness"), Roughness);
	Mid->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
	Mid->SetScalarParameterValue(TEXT("Pattern"), 0.0f);
	return Mid;
}

// ------------------------------------------------------------------ shoes

namespace Shoe
{
enum ESlot : int32
{
	SlotUpper,
	SlotRubber,
	SlotAccent,
	SlotLining,
	SlotLaces,
	SlotOutsole,
	SlotCount,
};
const TCHAR* const SlotNames[SlotCount] = {TEXT("Upper"), TEXT("Rubber"), TEXT("Accent"), TEXT("Lining"), TEXT("Laces"), TEXT("Outsole")};
// Canvas, rubber, leather, felt, cotton, tread.
const float SlotRoughness[SlotCount] = {0.86f, 0.5f, 0.55f, 0.95f, 0.8f, 0.92f};

/** The foot a shoe is made for, from the body's reference pose (component space, cm). */
struct FFootFit
{
	FVector Ankle = FVector::ZeroVector;
	/** Heel to toe, level; and across, toward the body's left. */
	FVector Ahead = FVector(0.0, 1.0, 0.0);
	FVector Left = FVector(1.0, 0.0, 0.0);
	float AnkleToBall = 14.0f;
	float AnkleHeight = 8.5f;
	/** +1 the left foot (its outside is toward the left), -1 the right. */
	float Outside = 1.0f;
	FTransform FootRest = FTransform::Identity;
	FName Bone;
};

bool FitFoot(const FReferenceSkeleton& Ref, int32 Side, FFootFit& Out)
{
	Out.Bone = Side == 0 ? FName(TEXT("foot_l")) : FName(TEXT("foot_r"));
	const int32 Foot = Ref.FindBoneIndex(Out.Bone);
	if (Foot == INDEX_NONE)
	{
		return false;
	}
	Out.FootRest = RefComponentSpace(Ref, Foot);
	Out.Ankle = Out.FootRest.GetLocation();
	const int32 Ball = Ref.FindBoneIndex(Side == 0 ? FName(TEXT("ball_l")) : FName(TEXT("ball_r")));
	if (Ball != INDEX_NONE)
	{
		FVector D = RefComponentSpace(Ref, Ball).GetLocation() - Out.Ankle;
		D.Z = 0.0;
		const double Length = D.Size();
		if (Length > 5.0)
		{
			Out.Ahead = D / Length;
			Out.AnkleToBall = FMath::Clamp(static_cast<float>(Length), 9.0f, 19.0f);
		}
	}
	Out.Left = FVector::CrossProduct(Out.Ahead, FVector(0.0, 0.0, 1.0)).GetSafeNormal();
	Out.AnkleHeight = FMath::Clamp(static_cast<float>(Out.Ankle.Z), 5.0f, 13.0f);
	Out.Outside = Side == 0 ? 1.0f : -1.0f;
	return true;
}

// The modeled pair's own frame (sneakers.py), once imported: the origin at the ankle joint, the ground 8.5 cm under
// it and the ball of the foot 14.2 cm ahead of it (level), the toe toward -Y and up +Z; the left shoe's outside toward
// -X, the right shoe the left one's mirror image.
const float ModelAnkleToBall = 14.2f;
const float ModelAnkleHeight = 8.5f;

/**
 * Where one of the modeled shoes goes, in the body's component space in its reference pose (the foot bone carries it
 * from there): its toe along the foot's heel-to-toe line and its up the world's, sized to the foot by the ankle-to-ball
 * span, standing on the floor where the bare foot stood (Floor). Its height follows the ankle's over that floor a little
 * way (12% taller at most, 8% lower), so the collar stays under the ankle bones and the vamp over the instep of a foot
 * whose ankle stands higher or lower than the model's. The tread goes 3 mm into the floor, as the shoes made here do,
 * so the bare sole of the foot stays inside it.
 */
FTransform ModelFit(const FFootFit& Fit, float Floor)
{
	const float Size = Fit.AnkleToBall / ModelAnkleToBall;
	const float Tall = FMath::Clamp((static_cast<float>(Fit.Ankle.Z) - Floor) / (ModelAnkleHeight * Size), 0.92f, 1.12f);
	// The importer's turn (BlenderImportYaw) is always a quarter turn's multiple: snapped, so the laptop's bounds can't
	// tilt the shoes by a degree. At its usual 90 this is FRotationMatrix::MakeFromXZ(-Fit.Left, Up): the model's +X to
	// the body's right, its -Y along the foot. One turn suits both feet: the right shoe is already the mirror image.
	const float Import = 90.0f * FMath::RoundToFloat(BlenderImportYaw() / 90.0f);
	const float AheadYaw = FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(Fit.Ahead.Y), static_cast<float>(Fit.Ahead.X)));
	const FVector At(Fit.Ankle.X, Fit.Ankle.Y, Floor - 0.3f + ModelAnkleHeight * Size * Tall);
	return FTransform(FRotator(0.0f, Import + AheadYaw, 0.0f).Quaternion(), At, FVector(Size, Size, Size * Tall));
}

/** A smooth curve through (Ts, Vs): cubic between the knots with finite-difference slopes, held flat past the ends. */
float Smooth(float T, const float* Ts, const float* Vs, int32 N)
{
	if (T <= Ts[0])
	{
		return Vs[0];
	}
	if (T >= Ts[N - 1])
	{
		return Vs[N - 1];
	}
	int32 I = 0;
	while (I < N - 2 && T > Ts[I + 1])
	{
		++I;
	}
	const float H = Ts[I + 1] - Ts[I];
	const float U = (T - Ts[I]) / H;
	auto Slope = [&](int32 K) {
		const int32 A = FMath::Max(K - 1, 0);
		const int32 B = FMath::Min(K + 1, N - 1);
		return (Vs[B] - Vs[A]) / FMath::Max(Ts[B] - Ts[A], 1e-4f);
	};
	const float M0 = Slope(I) * H;
	const float M1 = Slope(I + 1) * H;
	const float U2 = U * U;
	const float U3 = U2 * U;
	return (2.0f * U3 - 3.0f * U2 + 1.0f) * Vs[I] + (U3 - 2.0f * U2 + U) * M0 + (-2.0f * U3 + 3.0f * U2) * Vs[I + 1] + (U3 - U2) * M1;
}

/**
 * A low canvas court shoe in its own frame: u across (+ the outside of the foot), y forward from the back of the heel,
 * z up from the floor (cm). Sized from the foot (the ankle-to-ball span gives its length and width, the ankle's
 * height its height) with room to spare: the foot never shows through, and the leg leaves it below the ankle bones.
 */
struct FShape
{
	float Ab = 14.0f;
	float W = 1.0f;
	float Hs = 1.0f;
	float Ya = 7.4f;
	float Yb = 21.4f;
	float Len = 30.0f;
	float Zw = 2.6f;
	float Rh = 3.6f;
	float Rt = 4.4f;
	float TopY[6];

	explicit FShape(const FFootFit& Fit)
	{
		Ab = Fit.AnkleToBall;
		W = FMath::Clamp(Ab / 14.0f, 0.8f, 1.2f);
		Hs = FMath::Clamp(Fit.AnkleHeight / 8.5f, 0.8f, 1.25f);
		// The heel's back about half the ankle-to-ball span behind the ankle; the toes as far again past the ball as half of it.
		Ya = 0.47f * Ab + 0.8f;
		Yb = Ya + Ab;
		Len = Yb + 0.5f * Ab + 1.6f;
		Zw = 2.6f;
		Rh = 3.6f * W;
		Rt = 4.4f * W;
		const float Knots[6] = {Rh, Ya, Ya + 0.33f * Ab, Yb - 0.22f * Ab, Yb, Len - Rt};
		for (int32 K = 0; K < 6; ++K)
		{
			TopY[K] = K == 0 ? Knots[0] : FMath::Max(Knots[K], TopY[K - 1] + 0.5f);
		}
	}

	/** Half the width at Y, on the outside of the foot or the inside (the arch side runs narrower at the waist). */
	float Half(float Y, bool bOutside) const
	{
		static const float Ts[7] = {0.06f, 0.2f, 0.4f, 0.58f, 0.7f, 0.82f, 0.92f};
		static const float Out[7] = {3.75f, 4.05f, 4.3f, 4.95f, 5.25f, 4.85f, 4.0f};
		static const float In[7] = {3.7f, 3.85f, 3.55f, 4.75f, 5.1f, 4.85f, 4.1f};
		return Smooth(Y / Len, Ts, bOutside ? Out : In, 7) * W;
	}

	float Center(float Y) const { return 0.5f * (Half(Y, true) - Half(Y, false)); }

	/** How high the upper rises over the sole at Y: the heel counter, the collar at the ankle, the tongue, the toe box. */
	float Top(float Y) const
	{
		static const float Heights[6] = {4.6f, 4.9f, 6.3f, 4.7f, 3.7f, 2.9f};
		return Smooth(Y, TopY, Heights, 6) * Hs;
	}

	/** The toe spring: the tip turned up a little. */
	float Lift(float Y) const { return 0.7f * W * FMath::Square(FMath::SmoothStep(0.86f * Len, Len, Y)); }
};

struct FPoint
{
	/** On the sole's outline; where the upper closes over it; outward and level. */
	FVector2D P = FVector2D::ZeroVector;
	FVector2D Spine = FVector2D::ZeroVector;
	FVector2D N = FVector2D::ZeroVector;
	/** 0 the heel .. 1 the toe. */
	float T = 0.0f;
	/** 0 along a side, 1 round the toe, 2 round the heel. */
	uint8 Kind = 0;
};

/**
 * Builds a sneaker for one foot, the stand-in when the modeled pair isn't imported: a rubber sole (tread, midsole, a lip
 * where the upper sits in it) and a canvas upper lofted over it, closing along its top line, with a rubber toe cap and
 * foxing, a heel tab and side stripe, laces and the dark of the opening. Vertices are in the foot bone's position with
 * the body's axes; the component undoes the bone's rest rotation when it attaches, so the shoe follows the foot.
 */
UStaticMesh* BuildShoeMesh(UObject* Outer, const FFootFit& Fit, UMaterialInterface* Material)
{
	const FShape S(Fit);
	const int32 Ns = 10;
	const int32 Nt = 12;
	const int32 Nh = 10;
	const int32 Nv = 10;
	const float Yh = S.Rh;
	const float Yt = S.Len - S.Rt;

	// The sole's outline: up the outside, round the toe, back down the inside, round the heel.
	TArray<FPoint> Loop;
	auto SidePoint = [&](int32 K, bool bOutside) {
		const float Y = Yh + (Yt - Yh) * static_cast<float>(K) / Ns;
		FPoint Pt;
		Pt.P = FVector2D(bOutside ? S.Half(Y, true) : -S.Half(Y, false), Y);
		Pt.Spine = FVector2D(S.Center(Y), Y);
		Pt.Kind = 0;
		return Pt;
	};
	for (int32 K = 0; K <= Ns; ++K)
	{
		Loop.Add(SidePoint(K, true));
	}
	for (int32 J = 1; J < Nt; ++J)
	{
		const float A = PI * static_cast<float>(J) / Nt;
		const float C = FMath::Cos(A);
		const float Sn = FMath::Sin(A);
		FPoint Pt;
		// A round, slightly square toe, its tip toward the big toe.
		Pt.P = FVector2D(S.Half(Yt, C >= 0.0f) * C - 0.45f * S.W * FMath::Pow(Sn, 4.0f), Yt + S.Rt * FMath::Pow(Sn, 0.75f));
		Pt.Spine = FVector2D(S.Center(Yt), Yt);
		Pt.Kind = 1;
		Loop.Add(Pt);
	}
	for (int32 K = Ns; K >= 0; --K)
	{
		Loop.Add(SidePoint(K, false));
	}
	for (int32 J = 1; J < Nh; ++J)
	{
		const float A = PI * (1.0f - static_cast<float>(J) / Nh);
		const float C = FMath::Cos(A);
		FPoint Pt;
		Pt.P = FVector2D(S.Half(Yh, C >= 0.0f) * C, Yh - S.Rh * FMath::Sin(A));
		Pt.Spine = FVector2D(S.Center(Yh), Yh);
		Pt.Kind = 2;
		Loop.Add(Pt);
	}
	const int32 Count = Loop.Num();
	FVector2D Centroid = FVector2D::ZeroVector;
	for (int32 I = 0; I < Count; ++I)
	{
		FPoint& Pt = Loop[I];
		const FVector2D Along = Loop[(I + 1) % Count].P - Loop[(I + Count - 1) % Count].P;
		FVector2D N = FVector2D(Along.Y, -Along.X).GetSafeNormal();
		if (FVector2D::DotProduct(N, Pt.P - Pt.Spine) < 0.0)
		{
			N = -N;
		}
		Pt.N = N;
		Pt.T = static_cast<float>(Pt.P.Y) / S.Len;
		Centroid += Pt.P * (1.0 / Count);
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
	FPolygonGroupID Groups[SlotCount];
	for (int32 K = 0; K < SlotCount; ++K)
	{
		Groups[K] = Desc.CreatePolygonGroup();
		GroupNames[Groups[K]] = FName(SlotNames[K]);
	}

	// From the shoe's frame to the foot bone's position with the body's axes.
	const FVector Up(0.0, 0.0, 1.0);
	const FVector Heel = FVector(Fit.Ankle.X, Fit.Ankle.Y, 0.0) - Fit.Ahead * S.Ya;
	auto ToFoot = [&](const FVector& V) { return Heel + Fit.Left * (V.X * Fit.Outside) + Fit.Ahead * V.Y + Up * V.Z - Fit.Ankle; };
	auto ToFootDir = [&](const FVector& N) { return (Fit.Left * (N.X * Fit.Outside) + Fit.Ahead * N.Y + Up * N.Z).GetSafeNormal(); };
	auto Vertex = [&](const FVector& P, const FVector& N, const FVector2D& UV) {
		const FVertexID V = Desc.CreateVertex();
		Positions[V] = FVector3f(ToFoot(P));
		const FVertexInstanceID I = Desc.CreateVertexInstance(V);
		FVector Normal = ToFootDir(N);
		if (Normal.IsNearlyZero())
		{
			Normal = Up;
		}
		FVector Tangent = FVector::CrossProduct(Normal, Fit.Ahead);
		if (!Tangent.Normalize())
		{
			Tangent = Fit.Left;
		}
		Normals[I] = FVector3f(Normal);
		Tangents[I] = FVector3f(Tangent);
		Signs[I] = 1.0f;
		UVs.Set(I, 0, FVector2f(UV));
		Colors[I] = FVector4f(1.0f, 1.0f, 1.0f, 1.0f);
		return I;
	};
	auto Tri = [&](int32 Slot, FVertexInstanceID A, FVertexInstanceID B, FVertexInstanceID C) {
		const FVector3f PA = Positions[Desc.GetVertexInstanceVertex(A)];
		const FVector3f PB = Positions[Desc.GetVertexInstanceVertex(B)];
		const FVector3f PC = Positions[Desc.GetVertexInstanceVertex(C)];
		const FVector3f Cross = FVector3f::CrossProduct(PC - PA, PB - PA);
		if (Cross.SizeSquared() < 1e-8f)
		{
			return; // a sliver where a cap closes on its top
		}
		// The engine's front face: (C - A) x (B - A) along the normal.
		if (FVector3f::DotProduct(Cross, Normals[A] + Normals[B] + Normals[C]) >= 0.0f)
		{
			const FVertexInstanceID Ids[3] = {A, B, C};
			Desc.CreateTriangle(Groups[Slot], MakeArrayView(Ids, 3));
		}
		else
		{
			const FVertexInstanceID Ids[3] = {A, C, B};
			Desc.CreateTriangle(Groups[Slot], MakeArrayView(Ids, 3));
		}
	};
	auto Quad = [&](int32 Slot, FVertexInstanceID A, FVertexInstanceID B, FVertexInstanceID C, FVertexInstanceID D) {
		Tri(Slot, A, B, C);
		Tri(Slot, A, C, D);
	};

	// The sole: rings out from the outline, the tread's rounded edge up to a lip that holds the upper.
	struct FRing
	{
		float Off;
		float Z;
		float Out;
		float Rise;
		int32 Slot;
	};
	const FRing Rings[7] = {
		{0.05f, -0.35f, 0.5f, -0.85f, SlotOutsole},
		{0.22f, 0.05f, 1.0f, -0.25f, SlotOutsole},
		{0.22f, 0.55f, 1.0f, 0.0f, SlotOutsole},
		{0.22f, 0.55f, 1.0f, 0.0f, SlotRubber},
		{0.3f, S.Zw - 0.45f, 1.0f, 0.0f, SlotRubber},
		{0.14f, S.Zw, 0.55f, 0.85f, SlotRubber},
		{0.0f, S.Zw, 0.0f, 1.0f, SlotRubber},
	};
	TArray<FVertexInstanceID> Ring[7];
	for (int32 R = 0; R < 7; ++R)
	{
		Ring[R].SetNum(Count);
		for (int32 I = 0; I < Count; ++I)
		{
			const FPoint& Pt = Loop[I];
			const FVector2D Edge = Pt.P + Pt.N * Rings[R].Off;
			const FVector P(Edge.X, Edge.Y, Rings[R].Z + S.Lift(static_cast<float>(Edge.Y)));
			const FVector N(Pt.N.X * Rings[R].Out, Pt.N.Y * Rings[R].Out, Rings[R].Rise);
			Ring[R][I] = Vertex(P, N.GetSafeNormal(), FVector2D(static_cast<double>(I) / Count, R / 6.0));
		}
	}
	const int32 Strips[5][2] = {{0, 1}, {1, 2}, {3, 4}, {4, 5}, {5, 6}};
	for (const int32* Strip : Strips)
	{
		const TArray<FVertexInstanceID>& A = Ring[Strip[0]];
		const TArray<FVertexInstanceID>& B = Ring[Strip[1]];
		for (int32 I = 0; I < Count; ++I)
		{
			const int32 J = (I + 1) % Count;
			Quad(Rings[Strip[0]].Slot, A[I], A[J], B[J], B[I]);
		}
	}
	// The tread itself, flat but for the toe spring.
	const FVertexInstanceID Middle = Vertex(FVector(Centroid.X, Centroid.Y, -0.35f + S.Lift(static_cast<float>(Centroid.Y))), FVector(0.0, 0.0, -1.0), FVector2D(0.5, 0.5));
	TArray<FVertexInstanceID> Tread;
	Tread.SetNum(Count);
	for (int32 I = 0; I < Count; ++I)
	{
		const FPoint& Pt = Loop[I];
		const FVector2D Edge = Pt.P + Pt.N * Rings[0].Off;
		Tread[I] = Vertex(FVector(Edge.X, Edge.Y, -0.35f + S.Lift(static_cast<float>(Edge.Y))), FVector(0.0, 0.0, -1.0), FVector2D(static_cast<double>(I) / Count, 0.0));
	}
	for (int32 I = 0; I < Count; ++I)
	{
		Tri(SlotOutsole, Middle, Tread[I], Tread[(I + 1) % Count]);
	}

	// The upper: each outline point rises and turns in toward its place on the top line (a dome over the caps). The
	// walls stand nearly upright before they round over, as canvas over a last does.
	TArray<FVector> Grid;
	Grid.SetNum(Count * (Nv + 1));
	auto At = [&](int32 I, int32 K) -> FVector& { return Grid[((I % Count + Count) % Count) * (Nv + 1) + K]; };
	for (int32 I = 0; I < Count; ++I)
	{
		const FPoint& Pt = Loop[I];
		const float Top = S.Top(static_cast<float>(Pt.Spine.Y));
		for (int32 K = 0; K <= Nv; ++K)
		{
			const float V = static_cast<float>(K) / Nv;
			const float In = FMath::Pow(FMath::Max(FMath::Cos(V * 0.5f * PI), 0.0f), 0.35f);
			const float Rise = FMath::Pow(FMath::Sin(V * 0.5f * PI), 0.7f);
			const FVector2D H = Pt.Spine + (Pt.P - Pt.Spine) * In;
			At(I, K) = FVector(H.X, H.Y, S.Zw + Top * Rise + S.Lift(static_cast<float>(H.Y)));
		}
	}
	TArray<FVertexInstanceID> Upper;
	Upper.SetNum(Count * (Nv + 1));
	for (int32 I = 0; I < Count; ++I)
	{
		const FPoint& Pt = Loop[I];
		const FVector Inside(Pt.Spine.X, Pt.Spine.Y, S.Zw + 0.35f * S.Top(static_cast<float>(Pt.Spine.Y)) + S.Lift(static_cast<float>(Pt.Spine.Y)));
		for (int32 K = 0; K <= Nv; ++K)
		{
			FVector N = FVector::CrossProduct(At(I + 1, K) - At(I - 1, K), At(I, FMath::Min(K + 1, Nv)) - At(I, FMath::Max(K - 1, 0)));
			if (K == Nv || !N.Normalize())
			{
				N = FVector(0.0, 0.0, 1.0);
			}
			else if (FVector::DotProduct(N, At(I, K) - Inside) < 0.0)
			{
				N = -N;
			}
			Upper[I * (Nv + 1) + K] = Vertex(At(I, K), N, FVector2D(static_cast<double>(I) / Count, static_cast<double>(K) / Nv));
		}
	}
	auto Region = [&](int32 I, int32 K) {
		const FPoint& A = Loop[I];
		const FPoint& B = Loop[(I + 1) % Count];
		const float V = (K + 0.5f) / Nv;
		const float T = 0.5f * (A.T + B.T);
		const float SpineY = static_cast<float>(0.5 * (A.Spine.Y + B.Spine.Y));
		const bool bSide = A.Kind == 0 && B.Kind == 0;
		const bool bToe = A.Kind == 1 || B.Kind == 1;
		const bool bHeel = A.Kind == 2 || B.Kind == 2;
		if (V < 0.12f || (bToe && V < 0.55f))
		{
			return static_cast<int32>(SlotRubber); // the foxing round the bottom, the toe cap
		}
		if (V > 0.86f && (bHeel || SpineY < S.Ya + 0.6f))
		{
			return static_cast<int32>(SlotLining); // the opening the leg goes into
		}
		if (bHeel && FMath::Abs(0.5 * (A.P.X + B.P.X)) < 1.3 * S.W && V > 0.3f)
		{
			return static_cast<int32>(SlotAccent); // the heel tab
		}
		if (bSide && T > 0.36f && T < 0.62f && V > 0.3f && V < 0.44f)
		{
			return static_cast<int32>(SlotAccent); // the stripe
		}
		if (bSide && V > 0.8f && SpineY > S.Ya + 0.6f && SpineY < S.Ya + 0.62f * S.Ab)
		{
			return static_cast<int32>(SlotLaces);
		}
		return static_cast<int32>(SlotUpper);
	};
	for (int32 I = 0; I < Count; ++I)
	{
		const int32 J = (I + 1) % Count;
		for (int32 K = 0; K < Nv; ++K)
		{
			Quad(Region(I, K), Upper[I * (Nv + 1) + K], Upper[J * (Nv + 1) + K], Upper[J * (Nv + 1) + K + 1], Upper[I * (Nv + 1) + K + 1]);
		}
	}

	UStaticMesh* Mesh = NewObject<UStaticMesh>(Outer, NAME_None, RF_Transient);
	for (int32 K = 0; K < SlotCount; ++K)
	{
		Mesh->GetStaticMaterials().Add(FStaticMaterial(Material, FName(SlotNames[K])));
	}
	UStaticMesh::FBuildMeshDescriptionsParams Params;
	Params.bFastBuild = true;
	Params.bCommitMeshDescription = false;
	Params.bMarkPackageDirty = false;
	TArray<const FMeshDescription*> Lods;
	Lods.Add(&Desc);
	return Mesh->BuildFromMeshDescriptions(Lods, Params) ? Mesh : nullptr;
}
} // namespace Shoe
} // namespace ShortStackCharacterDetail

using namespace ShortStackCharacterDetail;

AShortStackCharacter::AShortStackCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(32.0f, 89.0f);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.0f, 520.0f, 0.0f);
	Move->MaxWalkSpeed = WalkSpeed;
	Move->MaxAcceleration = 1100.0f;
	Move->BrakingDecelerationWalking = 1500.0f;
	Move->GroundFriction = 8.0f;
	Move->bCanWalkOffLedges = true;

	// MetaHuman bodies face +Y: turn the mesh to the capsule's +X, feet on the capsule's floor.
	USkeletalMeshComponent* Body = GetMesh();
	Body->SetRelativeLocationAndRotation(FVector(0.0, 0.0, -89.0), FRotator(0.0f, -90.0f, 0.0f));
	Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Body->SetAnimInstanceClass(UStreetBodyAnim::StaticClass());
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	// Hidden from the camera when it's pulled in too close, the body still throws its shadow (and so does the head in first person).
	Body->bCastHiddenShadow = true;

	Face = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Face"));
	Face->SetupAttachment(Body);
	Face->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Face->SetAnimInstanceClass(UBackRoomFaceAnim::StaticClass());
	Face->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Face->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Face->bCastHiddenShadow = true;

	Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Boom"));
	Boom->SetupAttachment(RootComponent);
	Boom->SetRelativeLocation(FVector(0.0, 0.0, 58.0));
	Boom->TargetArmLength = ArmLength;
	Boom->SocketOffset = ShoulderOffset;
	Boom->bUsePawnControlRotation = true;
	Boom->bDoCollisionTest = true;
	Boom->ProbeSize = 14.0f;
	Boom->bEnableCameraLag = true;
	Boom->CameraLagSpeed = 16.0f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Boom, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
	Camera->SetFieldOfView(ThirdPersonFov);
}

void AShortStackCharacter::BeginPlay()
{
	Super::BeginPlay();
	Rng.Initialize(static_cast<int32>(GetUniqueID() * 7919u + 13u));
	NextBlink = Rng.FRandRange(0.5f, 3.0f);
	ArmNow = ArmLength;
	SetFirstPerson(bFirstPerson);
	Blend = bFirstPerson ? 1.0f : 0.0f;
	// Not dressed here: the game mode dresses the hero and Benny as they spawn, and anyone it doesn't is given the
	// creator's default person on their first frame (dressing here first would build every MetaHuman twice).
}

// ------------------------------------------------------------------ dressing

void AShortStackCharacter::ClearWearables()
{
	for (USceneComponent* W : Wearables)
	{
		if (W)
		{
			W->DestroyComponent();
		}
	}
	Wearables.Reset();
	GroomKinds.Reset();
	// The garments are among the wearables (gone with them).
	OuterGarment = nullptr;
	BottomGarment = nullptr;
	DefaultOutfit.Reset();
}

void AShortStackCharacter::UseArchetype()
{
	ClearWearables();
	bWearsScalpHair = false;
	if (USkeletalMesh* BodyAsset = LoadObject<USkeletalMesh>(nullptr, HeroArchetypeBody, nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		GetMesh()->SetSkeletalMeshAsset(BodyAsset);
	}
	if (USkeletalMesh* FaceAsset = LoadObject<USkeletalMesh>(nullptr, HeroArchetypeFace, nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		Face->SetSkeletalMeshAsset(FaceAsset);
	}
}

void AShortStackCharacter::BuildFromMetaHuman(UClass* Blueprint, const FStreetOutfit& Outfit)
{
	UBlueprintGeneratedClass* Bp = Cast<UBlueprintGeneratedClass>(Blueprint);
	if (!Bp || !Bp->SimpleConstructionScript)
	{
		UseArchetype();
		return;
	}
	ClearWearables();
	bWearsScalpHair = false;
	// As the Back Room's players: the Blueprint's Body and Face become ours, everything else visible is copied
	// onto ours and attached the same way. Here the hair comes too: the player sees their own head in third person.
	TMap<FName, USceneComponent*> Ours;
	const TArray<USCS_Node*>& Nodes = Bp->SimpleConstructionScript->GetAllNodes();
	for (USCS_Node* Node : Nodes)
	{
		const USkeletalMeshComponent* Template = Node ? Cast<USkeletalMeshComponent>(Node->ComponentTemplate) : nullptr;
		if (!Template)
		{
			continue;
		}
		const FName Name = Node->GetVariableName();
		USkeletalMeshComponent* Mine = Name == TEXT("Body") ? GetMesh() : (Name == TEXT("Face") ? Face.Get() : nullptr);
		if (Mine)
		{
			Mine->SetSkeletalMeshAsset(Template->GetSkeletalMeshAsset());
			CopyOverrides(Template, Mine);
			Ours.Add(Name, Mine);
		}
	}
	const FString Path = Bp->GetPathName();
	WornBlueprint = Blueprint;
	DefaultOutfit.Reset();
	// Every cast build holds its grooms on cards on the street (spec 6.1): the extras and the heroes carry only cards, the
	// cinematic builds (Sal, Dee, Big Lou...) strands first and cards further down, and HoldOnCards finds those.
	const bool bCards = Path.Contains(TEXT("/Cast/Built/MHC_"));
	// A hero face with a groom library wears the library's scalp, brows, beard and mustache (ApplyGrooms, after this); the
	// build's own of those kinds stay off. Without one, the build's own as far as the look's choices go.
	const bool bLibrary = !bNpc && HeroGrooms::LibraryFor(CastFaceName) != nullptr;
	const ss::hero::Look ForGrooms = GroomLook();
	const FHairShade Shade = HairShadeOf(LookNow.HairColor, LookAge);
	for (USCS_Node* Node : Nodes)
	{
		const UPrimitiveComponent* Template = Node ? Cast<UPrimitiveComponent>(Node->ComponentTemplate) : nullptr;
		if (!Template || Ours.Contains(Node->GetVariableName()))
		{
			continue;
		}
		const UGroomComponent* Groom = Cast<UGroomComponent>(Template);
		EGroomKind Kind = EGroomKind::Scalp;
		if (Groom)
		{
			if (!Groom->GroomAsset)
			{
				continue;
			}
			// The creator's hairstyle and facial hair, as far as this MetaHuman's grooms go.
			const FString Asset = Groom->GroomAsset->GetName();
			Kind = GroomKindOf(Node->GetVariableName().ToString(), Asset);
			if (bLibrary && IsManagedGroom(Kind))
			{
				continue;
			}
			if (!bNpc && !bLibrary && !HeroGrooms::WearsBuildGroom(ForGrooms, Kind, Asset))
			{
				continue;
			}
			if (Kind == EGroomKind::Scalp)
			{
				bWearsScalpHair = true;
			}
		}
		const FName Name = MakeUniqueObjectName(this, Template->GetClass(), Node->GetVariableName());
		UPrimitiveComponent* Copy = NewObject<UPrimitiveComponent>(this, Template->GetClass(), Name, RF_Transient, const_cast<UPrimitiveComponent*>(Template));
		const USCS_Node* ParentNode = Bp->SimpleConstructionScript->FindParentNode(Node);
		USceneComponent* const* Parent = ParentNode ? Ours.Find(ParentNode->GetVariableName()) : nullptr;
		Copy->SetupAttachment(Parent ? *Parent : GetMesh(), Node->AttachToName);
		Copy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		// Its shadow stays when it's kept from the player's view; not for a part the Blueprint hides, which would then
		// be added to the scene to throw the shadow of something nobody sees.
		Copy->bCastHiddenShadow = Template->IsVisible();
		if (USkinnedMeshComponent* Skinned = Cast<USkinnedMeshComponent>(Copy))
		{
			// Clothes ride the body's pose: the shirt in the Top's color (and print or graphic), everything else the shorts' tone.
			Skinned->SetLeaderPoseComponent(GetMesh());
			if (!DefaultOutfit.IsValid() && IsDefaultOutfit(Skinned))
			{
				DefaultOutfit = Skinned;
			}
			DyeDefaultOutfit(Skinned, Outfit);
		}
		else if (UGroomComponent* Hair = Cast<UGroomComponent>(Copy))
		{
			// The hair color from the creator on the hair, brows, beard and fuzz (the lashes stay dark); a cast member keeps theirs.
			if (!bNpc && Kind != EGroomKind::Lashes)
			{
				ShadeGroom(Hair, Shade, Kind == EGroomKind::Brows ? 0.06f : 0.0f);
			}
		}
		Copy->RegisterComponent();
		if (bCards)
		{
			HoldOnCards(Cast<UGroomComponent>(Copy));
		}
		if (UGroomComponent* Worn = Cast<UGroomComponent>(Copy))
		{
			GroomKinds.Emplace(Worn, static_cast<uint8>(Kind));
		}
		Ours.Add(Node->GetVariableName(), Copy);
		Wearables.Add(Copy);
	}
}

void AShortStackCharacter::LinkFace()
{
	// A new mesh makes a new anim instance: point the face at the body again.
	Face->AddTickPrerequisiteComponent(GetMesh());
	if (UBackRoomFaceAnim* FaceAnim = Cast<UBackRoomFaceAnim>(Face->GetAnimInstance()))
	{
		FaceAnim->Body = GetMesh();
	}
}

void AShortStackCharacter::DressGarments(const FStreetOutfit& Outfit)
{
	const FString FaceName = CastFaceName.ToString();
	const bool bTypeB = bNpc ? CastLooks::IsWomansFace(CastFaceName) : LookNow.Body == 1;
	// The body type's garments are fitted to the hero_cast heroes (HeroA1, HeroB1; every hero of the type until its own fit is
	// imported): the Back Room's Hero standing in, or anyone else, wears only a fit made for their own face (garments.py
	// fits any exported MetaHuman), or the type's with ss.Look.NpcGarments.
	const bool bTypeDefault = bNpc ? Outfit.bTypeGarments : IsHeroFace(FaceName);
	// The jacket: its own mesh, else (a kind not modeled yet) the bomber in its color, as the creator's jackets were worn.
	USkeletalMesh* Outer = nullptr;
	bool bOwnOuter = false;
	if (IsOuterCut(Outfit.Outer))
	{
		Outer = GarmentMesh(GarmentFamily(Outfit.Outer), FaceName, bTypeB, bTypeDefault);
		bOwnOuter = Outer != nullptr;
		if (!Outer && Outfit.Outer != Cut::Bomber)
		{
			Outer = GarmentMesh(GarmentFamily(Cut::Bomber), FaceName, bTypeB, bTypeDefault);
		}
	}
	// The trousers: their own mesh, else the jeans (chinos or work pants not modeled yet read as the nearest trousers).
	USkeletalMesh* Bottom = nullptr;
	bool bOwnBottom = false;
	if (IsTrousersCut(Outfit.Bottom))
	{
		Bottom = GarmentMesh(GarmentFamily(Outfit.Bottom), FaceName, bTypeB, bTypeDefault);
		bOwnBottom = Bottom != nullptr;
		if (!Bottom && Outfit.Bottom != Cut::Jeans)
		{
			Bottom = GarmentMesh(GarmentFamily(Cut::Jeans), FaceName, bTypeB, bTypeDefault);
		}
	}
	// Each on its own component, swapped in place (a garment of the same skeleton): skinned to the body's own skeleton (same
	// bones, same bind pose), it simply follows the body's pose.
	auto Wear = [this](TObjectPtr<USkeletalMeshComponent>& Slot, USkeletalMesh* Asset, const TCHAR* What) {
		if (!Asset)
		{
			if (Slot)
			{
				TierDefaults.Remove(Slot.Get());
				Wearables.Remove(Slot.Get());
				Slot->DestroyComponent();
				Slot = nullptr;
			}
			return;
		}
		if (!Slot)
		{
			Slot = NewObject<USkeletalMeshComponent>(this, MakeUniqueObjectName(this, USkeletalMeshComponent::StaticClass(), FName(What)), RF_Transient);
			Slot->SetupAttachment(GetMesh());
			Slot->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Slot->SetCanEverAffectNavigation(false);
			Slot->bCastHiddenShadow = true;
			Slot->bUseBoundsFromLeaderPoseComponent = true;
			Slot->SetSkeletalMeshAsset(Asset);
			Slot->SetLeaderPoseComponent(GetMesh());
			Slot->RegisterComponent();
			Wearables.Add(Slot.Get());
			return;
		}
		if (Slot->GetSkeletalMeshAsset() != Asset)
		{
			// The last garment's tints let go first (its materials were its own). The tiers' record of the component stays: its
			// shadow, ray tracing, ticks and forced LOD are the component's, not the mesh's (forgotten here, a swap at Mid would
			// record Mid's no-shadow as its own, and Near would never give it back).
			Slot->EmptyOverrideMaterials();
			Slot->SetSkeletalMeshAsset(Asset);
			Slot->SetLeaderPoseComponent(GetMesh());
		}
	};
	Wear(OuterGarment, Outer, TEXT("Jacket"));
	Wear(BottomGarment, Bottom, TEXT("Trousers"));
	if (OuterGarment)
	{
		// The shell bakes light grey and takes the jacket's color; the rib knit (the bomber's, the hoodie's) a darker shade
		// of it. The bomber's brass zip and the hoodie's cords keep theirs; a P1 jacket's own slots say what they take.
		TintSlot(OuterGarment, BakedSlot(OuterGarment, 0), Outfit.OuterColor, 1.5f);
		const bool bRib = !bOwnOuter || Outfit.Outer == Cut::Bomber || Outfit.Outer == Cut::Hoodie;
		if (bRib)
		{
			TintSlot(OuterGarment, BakedSlot(OuterGarment, 1), Scaled(Outfit.OuterColor, 0.55f), 1.5f);
		}
	}
	if (BottomGarment && bOwnBottom && (Outfit.Bottom == Cut::Chinos || Outfit.Bottom == Cut::WorkPants))
	{
		// The twill and the duck bake light and take their color.
		TintSlot(BottomGarment, BakedSlot(BottomGarment, 0), Outfit.BottomColor, 1.5f);
	}
	else if (BottomGarment && (Outfit.Bottom == Cut::Jeans || !bOwnBottom))
	{
		// The jeans keep their own indigo, in the wash worn (a multiply: white leaves the mid wash as baked). Set every time,
		// so a wash worn before on the same mesh doesn't stay on.
		TintSlot(BottomGarment, BakedSlot(BottomGarment, 0), Outfit.Bottom == Cut::Jeans ? Outfit.JeansTint : FLinearColor::White, 1.0f);
	}
	// The MetaHuman's tee where no jacket covers it, its shorts where no trousers do (the garments are made to cover where
	// they were: the body has no skin under them).
	ShowOutfitParts(DefaultOutfit.Get(), OuterGarment == nullptr, BottomGarment == nullptr);
}

void AShortStackCharacter::DressOutfit(const FStreetOutfit& Outfit)
{
	DressGarments(Outfit);
	DyeDefaultOutfit(DefaultOutfit.Get(), Outfit);
	BuildShoes(Outfit);
	PutOnHeadwear(Outfit);
}

FStreetOutfit AShortStackCharacter::CurrentOutfit() const
{
	return bNpc ? CastOutfit(CastWearNow) : HeroOutfit(LookNow, LookAge, bHasCloset ? &ClosetNow : nullptr);
}

ss::hero::Look AShortStackCharacter::GroomLook() const
{
	ss::hero::Look L = LookNow;
	if (!bNpc && bHasCloset)
	{
		// The closet's headwear is the hat that takes fuller hair off (Look.Hat is the portrait's collapse of it).
		L.Hat = IsHatCut(CurrentOutfit().Hat) ? FMath::Max(1, L.Hat) : 0;
	}
	return L;
}

void AShortStackCharacter::ApplyLook(const ss::hero::Character& Who, const ss::wardrobe::Closet* Closet)
{
	LookNow = Who.Appearance;
	LookAge = Who.Age;
	bHasLook = true;
	bNpc = false;
	bPreviewing = false;
	bDrawableKnown = false;
	bHasCloset = Closet != nullptr;
	ClosetNow = Closet ? *Closet : ss::wardrobe::Closet();
	if (bTierApplied)
	{
		RestoreTierDefaults();
	}
	const ss::hero::Look& L = LookNow;
	const FStreetOutfit Outfit = CurrentOutfit();
	// The hero MetaHuman nearest the look: body type, then skin tone in three bands.
	FString Built = CastLooks::HeroFaceFor(L).ToString();
	UClass* Blueprint = CastClass(Built);
	if (!Blueprint)
	{
		Built = TEXT("Hero");
		Blueprint = CastClass(Built);
	}
	if (Blueprint)
	{
		CastFaceName = FName(*Built);
		BuildFromMetaHuman(Blueprint, Outfit);
		// The library's grooms for the look, loaded now: this is a build, under black.
		ApplyGrooms(true);
		// The rest of what this body can wear, on its way into memory for the closet and the thrift's try-ons.
		HoldHeroGarments(Built, L.Body == 1);
		if (bGroomSetWanted)
		{
			// The Style Book's set, asked for before this face was known (or for the face worn before a rebuild).
			HeroGrooms::Preload(CastFaceName);
		}
	}
	else
	{
		CastFaceName = NAME_None;
		Built = TEXT("the archetype body");
		UseArchetype();
	}
	LinkFace();
	FitToHeight(static_cast<float>(L.Height));
	FitFace();
	DressOutfit(Outfit);
	RefreshVisibility();
	if (bTierApplied)
	{
		ApplyDetailTier();
	}
	const UGroomComponent* Scalp = ScalpGroom();
	UE_LOG(LogStreetHero, Log,
		TEXT("%s (%d) walks as %s: body %d, skin %d, hair %d (%s) in color %d, facial hair %d, brows %d, build %d, %d cm, jacket %d in color %d, glasses %d, hat %d%s"),
		UTF8_TO_TCHAR(Who.FullName().c_str()), Who.Age, *Built, L.Body, L.Skin, L.Hair, Scalp ? *GetNameSafe(Scalp->GroomAsset.Get()) : TEXT("none"), L.HairColor,
		L.FacialHair, L.Brows, L.Build, L.Height, L.Outfit, L.OutfitColor, L.Glasses, L.Hat, bHasCloset ? TEXT(", from the closet") : TEXT(""));
}

void AShortStackCharacter::ApplyCast(const TCHAR* CastName, const FLinearColor& Shirt)
{
	// At work, as before the wardrobes: their shirt, dark trousers (the shorts dyed), black work shoes, 175 cm.
	FCastWear Wear;
	Wear.Shirt = Shirt;
	Wear.Pants = SrgbOf(0x2a2b30);
	Wear.bWorkShoes = true;
	ApplyCast(CastName, Wear, 175.0f);
}

void AShortStackCharacter::ApplyCast(const TCHAR* CastName, const FCastWear& Wear, float HeightCm)
{
	bHasLook = true;
	bNpc = true;
	bPreviewing = false;
	bDrawableKnown = false;
	bHasCloset = false;
	LookNow = ss::hero::Look();
	LookAge = 0;
	CastWearNow = Wear;
	if (bTierApplied)
	{
		RestoreTierDefaults();
	}
	const FStreetOutfit Outfit = CurrentOutfit();
	if (UClass* Blueprint = CastClass(CastName))
	{
		CastFaceName = FName(CastName);
		BuildFromMetaHuman(Blueprint, Outfit);
	}
	else
	{
		UE_LOG(LogStreetHero, Warning, TEXT("%s isn't built (backroom_cast.py): the archetype body stands in"), CastName);
		CastFaceName = NAME_None;
		UseArchetype();
	}
	LinkFace();
	FitToHeight(FMath::Clamp(HeightCm, 150.0f, 200.0f));
	FitFace();
	DressOutfit(Outfit);
	BecomeNpc();
	RefreshVisibility();
	if (bTierApplied)
	{
		ApplyDetailTier();
	}
}

// ------------------------------------------------------------------ the look in place (spec 7.5.4)

void AShortStackCharacter::PreviewLook(const ss::hero::Look& Look, const ss::wardrobe::Closet& Closet)
{
	if (bNpc)
	{
		return;
	}
	if (!bPreviewing)
	{
		OwnLook = LookNow;
		OwnCloset = ClosetNow;
		bOwnHasCloset = bHasCloset;
		bPreviewing = true;
	}
	ChangeLookInPlace(Look, &Closet);
}

void AShortStackCharacter::EndPreview()
{
	if (!bPreviewing)
	{
		return;
	}
	bPreviewing = false;
	const ss::wardrobe::Closet Own = OwnCloset;
	ChangeLookInPlace(OwnLook, bOwnHasCloset ? &Own : nullptr);
}

void AShortStackCharacter::RefreshLook(const ss::hero::Look& Look, const ss::wardrobe::Closet& Closet)
{
	if (bNpc)
	{
		return;
	}
	bPreviewing = false;
	ChangeLookInPlace(Look, &Closet);
}

void AShortStackCharacter::ApplyWear(const ss::wardrobe::Closet& Closet)
{
	if (bNpc)
	{
		return;
	}
	// For keeps (the closet's Wear, the thrift's "Wear it out"): a preview on now ends in it.
	const ss::hero::Look Keep = bPreviewing ? OwnLook : LookNow;
	bPreviewing = false;
	ChangeLookInPlace(Keep, &Closet);
}

void AShortStackCharacter::ChangeLookInPlace(const ss::hero::Look& Look, const ss::wardrobe::Closet* Closet)
{
	const bool bWasPreviewing = bPreviewing;
	auto Rebuild = [&]() {
		ss::hero::Character Who;
		Who.Appearance = Look;
		Who.Age = LookAge;
		ApplyLook(Who, Closet);
		bPreviewing = bWasPreviewing;
	};
	if (!bHasLook || bNpc)
	{
		Rebuild();
		return;
	}
	const ss::hero::Look Was = LookNow;
	// Another body (the type or skin band: another hero face; the build; the height) is a build.
	if (CastLooks::HeroFaceFor(Look) != CastLooks::HeroFaceFor(Was) || Look.Build != Was.Build || Look.Height != Was.Height)
	{
		Rebuild();
		return;
	}
	const ss::hero::Look GroomsWere = GroomLook();
	LookNow = Look;
	bHasCloset = Closet != nullptr;
	ClosetNow = Closet ? *Closet : ss::wardrobe::Closet();
	const ss::hero::Look GroomsNow = GroomLook();
	const bool bShapeChanged = GroomsNow.Hair != GroomsWere.Hair || GroomsNow.FacialHair != GroomsWere.FacialHair || GroomsNow.Brows != GroomsWere.Brows
		|| GroomsNow.Hat != GroomsWere.Hat;
	const bool bColorChanged = GroomsNow.HairColor != GroomsWere.HairColor;
	const bool bLibrary = HeroGrooms::LibraryFor(CastFaceName) != nullptr;
	if (bShapeChanged && !bLibrary)
	{
		// No library for this face: the build's own grooms are all there is. What the new look takes off is hidden in place
		// (the Style Book's "Shaved" or "None" previews, and putting back what a preview hid, cost nothing); a groom of the
		// build's the look wants that this body was dressed without (dressed bald, say, then "Crew" in the Style Book) goes on
		// in place from the Blueprint's own asset and binding, in memory with the Blueprint: no build in view (spec 6.7).
		const UBlueprintGeneratedClass* Bp = Cast<UBlueprintGeneratedClass>(WornBlueprint.Get());
		const bool bNeedsBuild = !Bp || !Bp->SimpleConstructionScript;
		if (bNeedsBuild)
		{
			Rebuild();
			return;
		}
		TArray<TPair<UGroomComponent*, bool>> Shown;
		TArray<TPair<EGroomKind, const UGroomComponent*>> PutBack;
		for (const USCS_Node* Node : Bp->SimpleConstructionScript->GetAllNodes())
		{
			const UGroomComponent* Template = Node ? Cast<UGroomComponent>(Node->ComponentTemplate) : nullptr;
			if (!Template || !Template->GroomAsset)
			{
				continue;
			}
			const FString Asset = Template->GroomAsset->GetName();
			const EGroomKind Kind = GroomKindOf(Node->GetVariableName().ToString(), Asset);
			if (!IsManagedGroom(Kind))
			{
				continue;
			}
			UGroomComponent* Worn = nullptr;
			for (const TPair<TWeakObjectPtr<UGroomComponent>, uint8>& W : GroomKinds)
			{
				UGroomComponent* G = W.Key.Get();
				if (G && W.Value == static_cast<uint8>(Kind) && G->GroomAsset == Template->GroomAsset)
				{
					Worn = G;
				}
			}
			const bool bWant = HeroGrooms::WearsBuildGroom(GroomsNow, Kind, Asset);
			if (bWant && !Worn)
			{
				PutBack.Emplace(Kind, Template);
				continue;
			}
			if (Worn)
			{
				Shown.Emplace(Worn, bWant);
			}
		}
		for (const TPair<UGroomComponent*, bool>& S : Shown)
		{
			if (S.Value)
			{
				Reveal(S.Key);
			}
			else
			{
				Conceal(S.Key);
			}
			// The tiers keep it as it's worn now (Near shows each groom as recorded).
			if (FTierPart* Known = TierDefaults.Find(S.Key))
			{
				Known->bVisible = S.Value;
			}
		}
		for (const TPair<EGroomKind, const UGroomComponent*>& Back : PutBack)
		{
			// Held on cards and shaded in the hair color as it goes on (AddGroomOfKind), the build's own binding with it.
			AddGroomOfKind(static_cast<uint8>(Back.Key), Back.Value->GroomAsset, Back.Value->BindingAsset);
		}
		bWearsScalpHair = PutBack.ContainsByPredicate([](const TPair<EGroomKind, const UGroomComponent*>& B) { return B.Key == EGroomKind::Scalp; });
		for (const TPair<UGroomComponent*, bool>& S : Shown)
		{
			bWearsScalpHair |= S.Value && KindIn(GroomKinds, S.Key) == EGroomKind::Scalp;
		}
	}
	if (bLibrary && (bShapeChanged || bColorChanged))
	{
		ApplyGrooms(false);
	}
	else if (bColorChanged)
	{
		// The color (the barber's COLOR tab) on a face without a library: every worn groom shaded again in place.
		const FHairShade Shade = HairShadeOf(LookNow.HairColor, LookAge);
		for (const TPair<TWeakObjectPtr<UGroomComponent>, uint8>& Worn : GroomKinds)
		{
			UGroomComponent* Groom = Worn.Key.Get();
			const EGroomKind Kind = static_cast<EGroomKind>(Worn.Value);
			if (Groom && Kind != EGroomKind::Lashes)
			{
				ShadeGroom(Groom, Shade, Kind == EGroomKind::Brows ? 0.06f : 0.0f);
			}
		}
	}
	DressOutfit(CurrentOutfit());
	RefreshVisibility();
	if (bTierApplied)
	{
		ApplyDetailTier();
	}
}

void AShortStackCharacter::PreloadGroomSet(bool bHold)
{
	bGroomSetWanted = bHold;
	if (!bHold)
	{
		HeroGrooms::ReleasePreload();
	}
	else if (!CastFaceName.IsNone() && !bNpc)
	{
		HeroGrooms::Preload(CastFaceName);
	}
	// Not dressed yet (the barber's stage wakes as the street is built, before DressHero): ApplyLook holds it once the face
	// is known.
}

uint64 AShortStackCharacter::GarmentKindsDrawable() const
{
	const uint64 Deny = static_cast<uint64>(static_cast<uint32>(CVarDenyGarments.GetValueOnGameThread()));
	if (bDrawableKnown)
	{
		return DrawableKinds & ~Deny;
	}
	uint64 Bits = 0;
	auto Mark = [&Bits](Cut C) { Bits |= uint64(1) << static_cast<uint32>(C); };
	auto MeshIn = [](const TCHAR* Name) { return Name && FPackageName::DoesPackageExist(FString::Printf(TEXT("/Game/ShortStack/Meshes/%s/%s"), Name, Name)); };
	// The MetaHuman's own tee and shorts, and shoes (the modeled pair, or a pair made here to fit the feet).
	Mark(Cut::Tee);
	Mark(Cut::Shorts);
	Mark(Cut::Sneakers);
	Mark(Cut::WorkShoes);
	const FString FaceName = CastFaceName.ToString();
	const bool bTypeB = bNpc ? CastLooks::IsWomansFace(CastFaceName) : LookNow.Body == 1;
	const bool bTypeDefault = !bNpc && IsHeroFace(FaceName);
	for (const Cut C : {Cut::Hoodie, Cut::Bomber, Cut::Flannel, Cut::Denim, Cut::Leather, Cut::Track, Cut::Jeans, Cut::Chinos, Cut::WorkPants})
	{
		if (!GarmentPackage(GarmentFamily(C), FaceName, bTypeB, bTypeDefault).IsEmpty())
		{
			Mark(C);
		}
	}
	for (const FWearPiece& P : WearPieces)
	{
		if (MeshIn(P.Street) || MeshIn(P.BackRoom))
		{
			Mark(P.Kind);
		}
	}
	// Known for this body until it's built or dressed again (no imports land while it plays).
	DrawableKinds = Bits;
	bDrawableKnown = bHasLook;
	return Bits & ~Deny;
}

FCastWear AShortStackCharacter::HeroWear(const ss::hero::Look& Look, int32 Age, const ss::wardrobe::Closet* Closet)
{
	FStreetOutfit O = HeroOutfit(Look, Age, Closet);
	FCastWear W;
	if (IsOuterCut(O.Outer))
	{
		// The jacket, where only the tee can be drawn: its color on the tee, a flannel's plaid in it.
		W.Shirt = O.OuterColor;
		if (O.Outer == Cut::Flannel)
		{
			O.Shirt = O.OuterColor;
			PrintIn(O, 4, nullptr, nullptr, nullptr);
			W.Print = O.PrintIndex;
			W.InkA = O.PrintA;
			W.InkB = O.PrintB;
			W.InkC = O.PrintC;
		}
	}
	else
	{
		W.Shirt = O.Shirt;
		W.Print = O.PrintIndex;
		W.Graphic = O.GraphicIndex;
		W.InkA = O.Graphic ? O.GraphicA : O.PrintA;
		W.InkB = O.Graphic ? O.GraphicB : O.PrintB;
		W.InkC = O.Graphic ? O.GraphicC : O.PrintC;
	}
	// The trousers' own tone (the jeans' wash, the chinos' color), else the shorts'.
	W.Pants = IsTrousersCut(O.Bottom) ? O.BottomColor : O.Pants;
	W.Outer = O.Outer;
	W.OuterColor = O.OuterColor;
	W.Bottom = O.Bottom;
	W.Hat = O.Hat;
	W.Eyes = O.Eyes;
	W.HatColor = O.HatColor;
	W.FrameColor = O.FrameColor;
	W.bWorkShoes = !O.bDyeShoes;
	W.ShoeColor = O.ShoeUpper;
	return W;
}

FString AShortStackCharacter::DescribeLook() const
{
	const FStreetOutfit O = CurrentOutfit();
	auto CutName = [](Cut C) -> FString {
		static const TCHAR* const Names[] = {TEXT("tee"), TEXT("hoodie"), TEXT("bomber"), TEXT("flannel"), TEXT("denim"), TEXT("leather"), TEXT("track"), TEXT("shorts"),
			TEXT("jeans"), TEXT("chinos"), TEXT("work pants"), TEXT("sneakers"), TEXT("work shoes"), TEXT("beanie"), TEXT("ball cap"), TEXT("cap backwards"), TEXT("bucket hat"),
			TEXT("trilby"), TEXT("round glasses"), TEXT("square glasses"), TEXT("wire frames"), TEXT("shades"), TEXT("aviators")};
		const int32 I = static_cast<int32>(C);
		return I >= 0 && I < static_cast<int32>(UE_ARRAY_COUNT(Names)) ? FString(Names[I]) : FString(TEXT("none"));
	};
	TArray<FString> Grooms;
	for (const TPair<TWeakObjectPtr<UGroomComponent>, uint8>& Worn : GroomKinds)
	{
		if (const UGroomComponent* G = Worn.Key.Get())
		{
			Grooms.Add(FString::Printf(TEXT("%s (LOD %d)"), *GetNameSafe(G->GroomAsset.Get()), G->GetForcedLOD()));
		}
	}
	TArray<FString> Closet;
	if (bHasCloset)
	{
		static const TCHAR* const Slots[ss::wardrobe::SlotCount] = {TEXT("top"), TEXT("outer"), TEXT("bottom"), TEXT("shoes"), TEXT("hat"), TEXT("eyes")};
		for (int32 S = 0; S < ss::wardrobe::SlotCount; ++S)
		{
			Closet.Add(FString::Printf(TEXT("%s=%s"), Slots[S], ClosetNow.Worn[S].empty() ? TEXT("-") : UTF8_TO_TCHAR(ClosetNow.Worn[S].c_str())));
		}
	}
	return FString::Printf(TEXT("%s as %s%s: hair %d color %d, facial %d, brows %d, hat %d | jacket %s (%s), trousers %s (%s), tee %s%s | hat %s, eyes %s, %s | %s | grooms: %s"),
		*GetName(), *CastFaceName.ToString(), bPreviewing ? TEXT(" (previewing)") : TEXT(""), LookNow.Hair, LookNow.HairColor, LookNow.FacialHair, LookNow.Brows,
		LookNow.Hat, *CutName(O.Outer), OuterGarment ? *GetNameSafe(OuterGarment->GetSkeletalMeshAsset()) : TEXT("no mesh"), *CutName(O.Bottom),
		BottomGarment ? *GetNameSafe(BottomGarment->GetSkeletalMeshAsset()) : TEXT("the shorts"), O.Print ? O.Print : TEXT("plain"), O.Graphic ? *FString::Printf(TEXT(" + %s"), O.Graphic) : TEXT(""),
		*CutName(O.Hat), *CutName(O.Eyes), O.bBarefoot ? TEXT("barefoot") : (O.ShoePair ? O.ShoePair : TEXT("shoes made to fit")),
		bHasCloset ? *FString::Join(Closet, TEXT(" ")) : TEXT("no closet (the creator's)"), *FString::Join(Grooms, TEXT(", ")));
}

void AShortStackCharacter::BecomeNpc()
{
	// Nobody plays them: their bones move only while they're on screen, and the camera rig sleeps.
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	Face->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	Boom->bDoCollisionTest = false;
	Boom->bEnableCameraLag = false;
	// Dressed before he finishes spawning (Benny), these would otherwise come back on as he begins play: components
	// that start active or ticking are switched on again then.
	Boom->PrimaryComponentTick.bStartWithTickEnabled = false;
	Boom->SetComponentTickEnabled(false);
	Camera->bAutoActivate = false;
	Camera->Deactivate();
	// Set on the floor once and left there: no gravity to settle, and nothing shoving them out of the counter's way.
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	FCollisionQueryParams Query(SCENE_QUERY_STAT(StreetNpcFloor), false, this);
	FHitResult Hit;
	const FVector From = GetActorLocation();
	if (World->LineTraceSingleByObjectType(Hit, From, From - FVector(0.0, 0.0, 400.0), FCollisionObjectQueryParams(ECC_WorldStatic), Query))
	{
		SetActorLocation(Hit.ImpactPoint + FVector(0.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 0.5), false, nullptr, ETeleportType::TeleportPhysics);
		GetCharacterMovement()->DisableMovement();
	}
}

void AShortStackCharacter::FitToHeight(float HeightCm)
{
	USkeletalMesh* Asset = GetMesh()->GetSkeletalMeshAsset();
	if (!Asset)
	{
		return;
	}
	// The crown: the top of the face mesh (it rides the body at identity), or a skull's height over the head bone.
	float Crown = 0.0f;
	if (USkeletalMesh* FaceAsset = Face->GetSkeletalMeshAsset())
	{
		FBoxSphereBounds B = FaceAsset->GetImportedBounds();
		if (B.BoxExtent.IsNearlyZero())
		{
			B = FaceAsset->GetBounds();
		}
		Crown = static_cast<float>(B.Origin.Z + B.BoxExtent.Z);
	}
	if (Crown < 100.0f)
	{
		const FReferenceSkeleton& Ref = Asset->GetRefSkeleton();
		const int32 Bone = Ref.FindBoneIndex(HeadBone);
		if (Bone != INDEX_NONE)
		{
			Crown = static_cast<float>(RefComponentSpace(Ref, Bone).GetTranslation().Z) + 18.0f;
		}
	}
	if (Crown < 100.0f)
	{
		Crown = 175.0f;
	}
	const float K = FMath::Clamp(HeightCm / Crown, 0.85f, 1.18f);
	// The hero's build from the creator; a cast member's breadth from their wardrobe (CastLooks).
	const float Width = bNpc ? FMath::Clamp(CastWearNow.Breadth, 0.88f, 1.16f)
							 : (LookNow.Build == 0 ? 0.95f : LookNow.Build == 2 ? 1.04f : LookNow.Build == 3 ? 1.12f : 1.0f);
	MeshScale = K;
	const float Half = HeightCm * 0.5f;
	GetCapsuleComponent()->SetCapsuleSize(30.0f * Width, Half);
	GetMesh()->SetRelativeLocation(FVector(0.0, 0.0, -Half));
	GetMesh()->SetRelativeScale3D(FVector(K * Width, K * Width, K));
	Boom->SetRelativeLocation(FVector(0.0, 0.0, Half * 0.62f));
	bEyeSteadyValid = false;
}

void AShortStackCharacter::FitFace()
{
	bFaceHasEyes = false;
	MouthFromHead = FVector(0.0, 10.5, 1.0);
	const USkeletalMesh* FaceAsset = Face->GetSkeletalMeshAsset();
	if (!FaceAsset)
	{
		return;
	}
	const FReferenceSkeleton& Ref = FaceAsset->GetRefSkeleton();
	const int32 Head = Ref.FindBoneIndex(HeadBone);
	const int32 EyeL = Ref.FindBoneIndex(EyeBoneL);
	const int32 EyeR = Ref.FindBoneIndex(EyeBoneR);
	if (Head == INDEX_NONE || EyeL == INDEX_NONE || EyeR == INDEX_NONE)
	{
		return;
	}
	bFaceHasEyes = true;
	const FVector HeadAt = RefComponentSpace(Ref, Head).GetLocation();
	const FVector Eyes = (RefComponentSpace(Ref, EyeL).GetLocation() + RefComponentSpace(Ref, EyeR).GetLocation()) * 0.5;
	// The mouth between the lips when the rig names them, else a hand's width under the eyes and a little ahead.
	const int32 LipUpper = Ref.FindBoneIndex(TEXT("FACIAL_C_LipUpper"));
	const int32 LipLower = Ref.FindBoneIndex(TEXT("FACIAL_C_LipLower"));
	const FVector Mouth = LipUpper != INDEX_NONE && LipLower != INDEX_NONE
		? (RefComponentSpace(Ref, LipUpper).GetLocation() + RefComponentSpace(Ref, LipLower).GetLocation()) * 0.5
		: Eyes + FVector(0.0, 1.0, -6.6);
	MouthFromHead = Mouth - HeadAt;
}

FTransform AShortStackCharacter::FitHeadWear(const USkeletalMesh* FaceAsset, bool bHat, float Turn)
{
	if (!FaceAsset)
	{
		return FTransform::Identity;
	}
	const FReferenceSkeleton& Ref = FaceAsset->GetRefSkeleton();
	const int32 Bone = Ref.FindBoneIndex(HeadBone);
	if (Bone == INDEX_NONE)
	{
		return FTransform::Identity;
	}
	const FVector Head = RefComponentSpace(Ref, Bone).GetLocation();
	const FVector Up(0.0, 0.0, 1.0);
	// Square to the face: it looks down its mesh's +Y; the eyes' axis gives its exact heading.
	FVector Ahead(0.0, 1.0, 0.0);
	FVector Across(1.0, 0.0, 0.0);
	FVector Eyes = Head + FVector(0.0, 9.0, 7.5);
	float Apart = 6.4f;
	const int32 EyeL = Ref.FindBoneIndex(EyeBoneL);
	const int32 EyeR = Ref.FindBoneIndex(EyeBoneR);
	const bool bEyes = EyeL != INDEX_NONE && EyeR != INDEX_NONE;
	if (bEyes)
	{
		const FVector L = RefComponentSpace(Ref, EyeL).GetLocation();
		const FVector R = RefComponentSpace(Ref, EyeR).GetLocation();
		Across = (L - R).GetSafeNormal();
		Ahead = (FVector(0.0, 1.0, 0.0) - Across * FVector::DotProduct(FVector(0.0, 1.0, 0.0), Across)).GetSafeNormal();
		Eyes = (L + R) * 0.5;
		Apart = static_cast<float>(FVector::Dist(L, R));
	}
	const float FaceYaw = FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(Ahead.Y), static_cast<float>(Ahead.X)));
	FBoxSphereBounds Bounds = FaceAsset->GetImportedBounds();
	if (Bounds.BoxExtent.IsNearlyZero())
	{
		Bounds = FaceAsset->GetBounds();
	}
	if (bHat)
	{
		// Sized from the eyes' spacing (a MetaHuman face mesh's bounds take in the neck and the shoulders' tops, so they
		// can't say how wide the head is): the hats, made for a 15.6 cm head, want 15% more over eyes 6.8 cm apart to
		// cover the skull front to back. Worn as a hat is, by the brow: the cuff 3.5 cm over the eyes, the crown (the hat's
		// origin, the top of the head inside it) 11.3 cm of hat above that. Fitted by eye on the hero MetaHumans; the top of
		// the face mesh (the scalp) stands in when there are no eye bones.
		const float Size = FMath::Clamp(1.15f * (bEyes ? Apart / 6.79f : 1.0f), 1.0f, 1.3f);
		float Crown = static_cast<float>(Bounds.Origin.Z + Bounds.BoxExtent.Z - Head.Z) + 0.4f + (Size - 1.0f) * 10.0f;
		if (bEyes)
		{
			Crown = static_cast<float>((Eyes - Head).Z) + 3.5f + 11.3f * Size;
		}
		if (Crown < 12.0f || Crown > 28.0f)
		{
			Crown = 19.0f;
		}
		const FVector At = Head + Up * Tuned(CVarHatUp, Crown) + Ahead * Tuned(CVarHatForward, 0.4f);
		return FTransform(BlenderFacing(FaceYaw + Turn).Quaternion(), At, FVector(Size));
	}
	// The glasses' origin is the bridge of the nose, the lenses' plane: before the eyes' centers (the eyeball's radius and
	// the lenses' distance from it, 1.7 cm fitted by eye on the hero MetaHumans), at their height and between them; sized
	// to how far apart they are.
	const FVector FromHead = Eyes - Head;
	const float Rise = static_cast<float>(FromHead.Z);
	const float Forward = static_cast<float>(FVector::DotProduct(FromHead, Ahead)) + (bEyes ? 1.7f : 0.0f);
	const float Side = static_cast<float>(FVector::DotProduct(FromHead, Across));
	const FVector At = Head + Up * Tuned(CVarGlassesUp, bEyes ? Rise : 8.5f) + Ahead * Tuned(CVarGlassesForward, bEyes ? Forward : 10.5f) + Across * (bEyes ? Side : 0.0f);
	return FTransform(BlenderFacing(FaceYaw).Quaternion(), At, FVector(FMath::Clamp(Apart / 6.4f, 0.92f, 1.1f)));
}

USkeletalMeshComponent* AShortStackCharacter::HeadCarrier() const
{
	// The face, or the body when the face has no head bone of its own.
	for (USkeletalMeshComponent* Carrier : {Face.Get(), GetMesh()})
	{
		const USkeletalMesh* Asset = Carrier ? Carrier->GetSkeletalMeshAsset() : nullptr;
		if (Asset && Asset->GetRefSkeleton().FindBoneIndex(HeadBone) != INDEX_NONE)
		{
			return Carrier;
		}
	}
	return nullptr;
}

void AShortStackCharacter::WearOnHead(UStaticMeshComponent* Piece, const FTransform& InFace)
{
	// InFace was fitted to this same mesh (PutOnHeadwear asks HeadCarrier too).
	USkeletalMeshComponent* Parent = HeadCarrier();
	if (!Parent)
	{
		Conceal(Piece);
		return;
	}
	const USkeletalMesh* Asset = Parent->GetSkeletalMeshAsset();
	const FTransform Head = RefComponentSpace(Asset->GetRefSkeleton(), Asset->GetRefSkeleton().FindBoneIndex(HeadBone));
	// Carried by the head bone, so it turns and nods with the head. A socket passes its component's scale (the height and
	// build) along the bone's own axes, which only agrees with the component's when the scale is uniform: the offset is
	// put where the scaled head has it, and the piece is turned so its own axes take the scale as the face does.
	const FVector Scale = Parent->GetComponentScale();
	const FVector Delta = InFace.GetLocation() - Head.GetLocation();
	const FVector Local = Head.GetRotation().UnrotateVector(Scale * Delta) / Scale;
	Piece->AttachToComponent(Parent, FAttachmentTransformRules::KeepRelativeTransform, HeadBone);
	Piece->SetRelativeTransform(FTransform(Head.GetRotation().Inverse() * InFace.GetRotation(), Local, InFace.GetScale3D()));
	Reveal(Piece);
}

namespace ShortStackCharacterDetail
{
/**
 * Where one of the Back Room's pieces (wear.py: made around the midpoint between the eyes, looking down +X with +Z up, for
 * eyes 6.3 cm apart) sits on a face, in the face mesh's component space in its reference pose, as ABackRoomPlayer::ApplyWear
 * fits it: a stand-in until the street's own piece is imported.
 */
FTransform WearFrameFit(const USkeletalMesh* FaceAsset, float Turn)
{
	if (!FaceAsset)
	{
		return FTransform::Identity;
	}
	const FReferenceSkeleton& Ref = FaceAsset->GetRefSkeleton();
	const int32 EyeL = Ref.FindBoneIndex(EyeBoneL);
	const int32 EyeR = Ref.FindBoneIndex(EyeBoneR);
	if (EyeL == INDEX_NONE || EyeR == INDEX_NONE)
	{
		return FTransform::Identity;
	}
	const FVector L = RefComponentSpace(Ref, EyeL).GetLocation();
	const FVector R = RefComponentSpace(Ref, EyeR).GetLocation();
	const FVector Across = (L - R).GetSafeNormal();
	const FVector Ahead = (FVector(0.0, 1.0, 0.0) - Across * FVector::DotProduct(FVector(0.0, 1.0, 0.0), Across)).GetSafeNormal();
	FRotator Facing = FRotationMatrix::MakeFromXZ(Ahead, FVector::UpVector).Rotator();
	Facing.Yaw += Turn;
	const double Scale = FMath::Clamp(FVector::Dist(L, R) / 6.3, 0.9, 1.12);
	return FTransform(Facing, (L + R) * 0.5, FVector(Scale));
}

/** A Back Room piece's colors, as the Back Room paints them: its "wear_main" in the cloth's color, "wear_frame" in the frames'. */
void PaintWearPiece(UStaticMeshComponent* C, const FLinearColor& Main, const FLinearColor& Frame)
{
	UMaterialInterface* Surface = LoadObject<UMaterialInterface>(nullptr, SurfaceMaterial, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!C || !Surface)
	{
		return;
	}
	for (int32 Slot = 0; Slot < C->GetNumMaterials(); ++Slot)
	{
		const UMaterialInterface* M = C->GetMaterial(Slot);
		const FString Name = M ? M->GetName() : FString();
		const bool bMain = Name.Contains(TEXT("wear_main"));
		const bool bFrame = Name.Contains(TEXT("wear_frame"));
		if (bMain || bFrame)
		{
			C->SetMaterial(Slot, PlainSurface(Surface, C, bMain ? Main : Frame, bMain ? 0.86f : 0.28f));
		}
	}
}
} // namespace ShortStackCharacterDetail

void AShortStackCharacter::PutOnHeadwear(const FStreetOutfit& Outfit)
{
	Cut HatCut = IsHatCut(Outfit.Hat) ? Outfit.Hat : Cut::Count;
	if (bNpc && HatCut != Cut::Count)
	{
		// As the Back Room has it (HatFits): someone else's hat only over hair that lies close; fuller hair would come up
		// through the crown. The hero's hat takes fuller hair off instead (the grooms' rule).
		const UGroomComponent* Scalp = ScalpGroom();
		if (Scalp && Scalp->GroomAsset && !HeroGrooms::IsTightHair(Scalp->GroomAsset->GetName()))
		{
			HatCut = Cut::Count;
		}
	}
	const FWearPiece* HatPiece = WearPieceFor(HatCut);
	const FWearPiece* EyesPiece = IsEyesCut(Outfit.Eyes) ? WearPieceFor(Outfit.Eyes) : nullptr;
	// Fitted to the mesh that will carry them (WearOnHead), so a face without a head bone can't put them on the floor.
	const USkeletalMeshComponent* Carrier = HeadCarrier();
	const USkeletalMesh* HeadAsset = Carrier ? Carrier->GetSkeletalMeshAsset() : nullptr;
	// The street's own piece, else the Back Room's (bWearFrame: fitted in its frame) until the street's is imported.
	auto Ready = [this](TObjectPtr<UStaticMeshComponent>& Slot, const FWearPiece* Piece, const TCHAR* What, bool& bWearFrame) -> UStaticMeshComponent* {
		bWearFrame = false;
		UStaticMesh* Mesh = Piece ? Wearable(Piece->Street) : nullptr;
		if (Piece && !Mesh && Piece->BackRoom)
		{
			Mesh = Wearable(Piece->BackRoom);
			bWearFrame = Mesh != nullptr;
		}
		if (Piece && !Mesh)
		{
			UE_LOG(LogStreetHero, Warning, TEXT("%s %s isn't imported (shortstack_setup.import_meshes)"), What, Piece->Street);
		}
		if (!Mesh)
		{
			if (Slot)
			{
				TierDefaults.Remove(Slot.Get());
				Slot->DestroyComponent();
				Slot = nullptr;
			}
			return nullptr;
		}
		if (!Slot)
		{
			Slot = NewObject<UStaticMeshComponent>(this, MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), FName(What)), RF_Transient);
			Slot->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Slot->SetCanEverAffectNavigation(false);
			Slot->bCastHiddenShadow = true;
			Slot->SetupAttachment(Face);
			Slot->RegisterComponent();
		}
		Slot->SetStaticMesh(Mesh);
		// The last piece's tinted materials would otherwise stay on as overrides.
		Slot->EmptyOverrideMaterials();
		return Slot;
	};
	bool bWearFrame = false;
	if (UStaticMeshComponent* C = Ready(HatMesh, HatPiece, TEXT("Hat"), bWearFrame))
	{
		if (bWearFrame)
		{
			PaintWearPiece(C, Outfit.HatColor, Outfit.FrameColor);
			WearOnHead(C, WearFrameFit(HeadAsset, HatPiece->Turn));
		}
		else
		{
			// The fabric in a color that goes with the jacket and stands apart from the hair at this age (as the creator's
			// portrait has it); the cap's top button is the same cloth, the bill's underside keeps its green, the trilby's band
			// its own.
			TintSlot(C, BakedSlot(C, 0), Outfit.HatColor, 1.45f);
			if (HatCut == Cut::BallCap || HatCut == Cut::CapBack)
			{
				TintSlot(C, BakedSlot(C, 2), Outfit.HatColor, 1.45f);
			}
			FTransform Fit = FitHeadWear(HeadAsset, true, HatPiece->Turn);
			if (bWearsScalpHair)
			{
				// Over close-cropped hair (the only kind kept under a hat): a little higher and roomier than on the bare scalp.
				Fit.AddToTranslation(FVector(0.0, 0.0, 0.8));
				Fit.SetScale3D(Fit.GetScale3D() * 1.03);
			}
			WearOnHead(C, Fit);
		}
	}
	if (UStaticMeshComponent* C = Ready(GlassesMesh, EyesPiece, TEXT("Glasses"), bWearFrame))
	{
		if (bWearFrame)
		{
			PaintWearPiece(C, Outfit.HatColor, Outfit.FrameColor);
			WearOnHead(C, WearFrameFit(HeadAsset, 0.0f));
		}
		else
		{
			// Black acetate, or gold wire. The bake gives every lens a dark mirror: clear ones get a glass you see the eyes
			// through (Blender's second slot, after the frame), the shades and the aviators keep theirs.
			TintSlot(C, BakedSlot(C, 0), Outfit.FrameColor, EyesPiece->bGold ? 1.4f : 1.0f);
			if (!EyesPiece->bDark && C->GetNumMaterials() > 1)
			{
				if (UMaterialInterface* Lens = LoadObject<UMaterialInterface>(nullptr, LensMaterial, nullptr, LOAD_NoWarn | LOAD_Quiet))
				{
					C->SetMaterial(BakedSlot(C, 1), Lens);
				}
			}
			WearOnHead(C, FitHeadWear(HeadAsset, false));
		}
	}
}

void AShortStackCharacter::RefitWear()
{
	PutOnHeadwear(CurrentOutfit());
	RefreshVisibility();
	if (bTierApplied)
	{
		// A hat or glasses made just now take the tier the rest of them is on.
		ApplyDetailTier();
	}
}

namespace ShortStackCharacterDetail
{
FAutoConsoleCommandWithWorld CmdWearRefit(TEXT("ss.Wear.Refit"), TEXT("Puts the street characters' hats and glasses on again (after setting ss.Wear.*)."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World) {
		if (!World)
		{
			return;
		}
		for (TActorIterator<AShortStackCharacter> It(World); It; ++It)
		{
			It->RefitWear();
		}
	}));
} // namespace ShortStackCharacterDetail

void AShortStackCharacter::HideToes(int32 Side, bool bHide)
{
	// The bone hidden last time first, by its index: a body swapped for one with as many bones keeps the hidden ones.
	USkeletalMeshComponent* Body = GetMesh();
	if (HiddenToes[Side] != INDEX_NONE && HiddenToes[Side] < Body->GetNumBones())
	{
		Body->UnHideBone(HiddenToes[Side]);
	}
	HiddenToes[Side] = INDEX_NONE;
	const USkeletalMesh* Asset = Body->GetSkeletalMeshAsset();
	if (!bHide || !Asset)
	{
		return;
	}
	// A hidden bone draws shrunk to its parent, so the toes go to the ankle, inside the shoe: the toe box needn't hold
	// them, and nothing of them comes through it. The foot itself stays (hidden, it would be drawn shrunk to the knee,
	// pulling the ankle up the shin with it); the shoe is made to hold it.
	const int32 Ball = Asset->GetRefSkeleton().FindBoneIndex(Side == 0 ? FName(TEXT("ball_l")) : FName(TEXT("ball_r")));
	if (Ball != INDEX_NONE)
	{
		Body->HideBone(Ball, PBO_None);
		HiddenToes[Side] = Ball;
	}
}

void AShortStackCharacter::BuildShoes(const FStreetOutfit& Outfit)
{
	USkeletalMesh* BodyAsset = GetMesh()->GetSkeletalMeshAsset();
	if (Outfit.bBarefoot)
	{
		// Shoes off (the closet's Take off): the feet as the body has them, toes and all.
		for (int32 Side = 0; Side < 2; ++Side)
		{
			Conceal(Shoes[Side].Get());
			HideToes(Side, false);
		}
		return;
	}
	// The modeled pair when both shoes are imported; otherwise a pair made here to fit the feet.
	UStaticMesh* Pair[2] = {nullptr, nullptr};
	if (Outfit.ShoePair)
	{
		Pair[0] = Wearable(*FString::Printf(TEXT("%s_L"), Outfit.ShoePair));
		Pair[1] = Wearable(*FString::Printf(TEXT("%s_R"), Outfit.ShoePair));
	}
	const bool bModeled = Pair[0] && Pair[1];
	UMaterialInterface* Surface = nullptr;
	if (!bModeled)
	{
		if (Outfit.ShoePair)
		{
			UE_LOG(LogStreetHero, Log, TEXT("%s_L/_R aren't imported (shortstack_setup.import_meshes): shoes made to fit stand in"), Outfit.ShoePair);
		}
		Surface = LoadObject<UMaterialInterface>(nullptr, SurfaceMaterial, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Surface)
		{
			Surface = LoadObject<UMaterialInterface>(nullptr, BasicMaterial, nullptr, LOAD_NoWarn | LOAD_Quiet);
		}
	}
	if (!BodyAsset || (!bModeled && !Surface))
	{
		for (int32 Side = 0; Side < 2; ++Side)
		{
			Conceal(Shoes[Side].Get());
			HideToes(Side, false);
		}
		return;
	}
	const FReferenceSkeleton& Ref = BodyAsset->GetRefSkeleton();
	const bool bRefit = ShoesFitFor.Get() != BodyAsset;
	const FLinearColor Colors[Shoe::SlotCount] = {Outfit.ShoeUpper, Outfit.ShoeRubber, Outfit.ShoeAccent, Outfit.ShoeLining, Outfit.ShoeLaces, Outfit.ShoeOutsole};
	// Where the bare feet stand: the floor, or as far into it as the body's soles go (its lowest point, a centimeter at most).
	float Floor = 0.0f;
	const FBoxSphereBounds Bounds = BodyAsset->GetImportedBounds();
	if (Bounds.BoxExtent.Z > 50.0)
	{
		Floor = FMath::Clamp(static_cast<float>(Bounds.Origin.Z - Bounds.BoxExtent.Z), -1.0f, 0.0f);
	}
	for (int32 Side = 0; Side < 2; ++Side)
	{
		Shoe::FFootFit Fit;
		if (!Shoe::FitFoot(Ref, Side, Fit))
		{
			Conceal(Shoes[Side].Get());
			HideToes(Side, false);
			continue;
		}
		if (!bModeled && (bRefit || !ShoeMeshes[Side]))
		{
			// Made once for each body: a couple of thousand triangles, fitted to its feet.
			ShoeMeshes[Side] = Shoe::BuildShoeMesh(this, Fit, Surface);
		}
		UStaticMesh* ShoeMesh = bModeled ? Pair[Side] : ShoeMeshes[Side].Get();
		if (!ShoeMesh)
		{
			Conceal(Shoes[Side].Get());
			HideToes(Side, false);
			continue;
		}
		if (!Shoes[Side])
		{
			Shoes[Side] = NewObject<UStaticMeshComponent>(this, MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), Side == 0 ? TEXT("ShoeL") : TEXT("ShoeR")), RF_Transient);
			Shoes[Side]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Shoes[Side]->SetCanEverAffectNavigation(false);
			Shoes[Side]->bCastHiddenShadow = true;
			Shoes[Side]->SetupAttachment(GetMesh(), Fit.Bone);
			Shoes[Side]->RegisterComponent();
		}
		else
		{
			Shoes[Side]->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, Fit.Bone);
		}
		Shoes[Side]->SetStaticMesh(ShoeMesh);
		// The other pair's colors would otherwise stay on as overrides.
		Shoes[Side]->EmptyOverrideMaterials();
		if (bModeled)
		{
			// Carried by the foot bone, so it follows the foot through the stride. As the hat on the head (WearOnHead): the
			// offset is put where the scaled body has it, and the shoe's own axes (its width, length and height, the body's
			// own in the reference pose) take the height and build as the leg does.
			const FTransform InBody = Shoe::ModelFit(Fit, Floor);
			const FQuat Rest = Fit.FootRest.GetRotation();
			const FVector Scale = GetMesh()->GetComponentScale();
			const FVector Local = Rest.UnrotateVector(Scale * (InBody.GetLocation() - Fit.Ankle)) / Scale;
			Shoes[Side]->SetRelativeTransform(FTransform(Rest.Inverse() * InBody.GetRotation(), Local, InBody.GetScale3D()));
			if (Outfit.bDyeShoes)
			{
				// The canvas, the trims and the laces bake light and take the outfit's colors (lifted back by about as much as
				// the bake darkens them); the rubber, the lining and the tread keep their own.
				UStaticMeshComponent* Worn = Shoes[Side].Get();
				TintSlot(Worn, BakedSlot(Worn, Shoe::SlotUpper), Outfit.ShoeUpper, 1.5f);
				TintSlot(Worn, BakedSlot(Worn, Shoe::SlotAccent), Outfit.ShoeAccent, 1.5f);
				TintSlot(Worn, BakedSlot(Worn, Shoe::SlotLaces), Outfit.ShoeLaces, 1.3f);
			}
		}
		else
		{
			// The mesh is built in the body's axes at the foot: undo the bone's rest rotation and the shoe sits as built, then follows the foot.
			Shoes[Side]->SetRelativeTransform(FTransform(Fit.FootRest.GetRotation().Inverse(), FVector::ZeroVector));
			for (int32 Slot = 0; Slot < Shoe::SlotCount; ++Slot)
			{
				Shoes[Side]->SetMaterial(Slot, PlainSurface(Surface, Shoes[Side].Get(), Colors[Slot], Shoe::SlotRoughness[Slot]));
			}
		}
		Reveal(Shoes[Side].Get());
		HideToes(Side, true);
	}
	if (!bModeled)
	{
		ShoesFitFor = BodyAsset;
	}
}

// ------------------------------------------------------------------ view

bool AShortStackCharacter::IsHeadPiece(const USceneComponent* Piece) const
{
	for (const USceneComponent* At = Piece; At; At = At->GetAttachParent())
	{
		if (At == Face)
		{
			return true;
		}
	}
	return false;
}

void AShortStackCharacter::RefreshVisibility()
{
	// Kept from the player's own view only, never hidden outright: the shadows stay (bCastHiddenShadow), and so does the
	// hair's binding, which needs the face drawn.
	const bool bHideHead = bBodyHidden || bHeadHidden;
	GetMesh()->SetOwnerNoSee(bBodyHidden);
	Face->SetOwnerNoSee(bHideHead);
	for (USceneComponent* W : Wearables)
	{
		if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(W))
		{
			Prim->SetOwnerNoSee(IsHeadPiece(W) ? bHideHead : bBodyHidden);
		}
	}
	if (HatMesh)
	{
		HatMesh->SetOwnerNoSee(bHideHead);
	}
	if (GlassesMesh)
	{
		GlassesMesh->SetOwnerNoSee(bHideHead);
	}
	for (UStaticMeshComponent* S : Shoes)
	{
		if (S)
		{
			S->SetOwnerNoSee(bBodyHidden);
		}
	}
}

void AShortStackCharacter::SetFirstPerson(bool bFirst)
{
	bFirstPerson = bFirst;
	// First person turns the body with the view; third person turns it toward where it walks.
	bUseControllerRotationYaw = bFirst;
	GetCharacterMovement()->bOrientRotationToMovement = !bFirst;
}

FVector AShortStackCharacter::EyeLocal() const
{
	if (bFaceHasEyes && Face->GetSkeletalMeshAsset())
	{
		// Between the eyes (their bones ride the head), a little before them so nothing of the face is ahead of the camera.
		const FVector Eyes = (Face->GetSocketLocation(EyeBoneL) + Face->GetSocketLocation(EyeBoneR)) * 0.5;
		return GetActorTransform().InverseTransformPosition(Eyes) + FVector(1.5, 0.0, 0.0) * MeshScale;
	}
	const USkeletalMeshComponent* Body = GetMesh();
	if (Body && Body->GetSkeletalMeshAsset() && Body->DoesSocketExist(HeadBone))
	{
		// The head bone sits at the base of the skull: the eyes are a little above it and forward.
		const FVector Head = GetActorTransform().InverseTransformPosition(Body->GetSocketLocation(HeadBone));
		return Head + FVector(9.0, 0.0, 8.0) * MeshScale;
	}
	return FVector(8.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.86f);
}

FVector AShortStackCharacter::EyeLocation() const
{
	return GetActorTransform().TransformPosition(EyeLocal());
}

FVector AShortStackCharacter::LookDirection() const
{
	return GetControlRotation().Vector();
}

void AShortStackCharacter::UpdateCamera(float Dt)
{
	const float Target = bFirstPerson ? 1.0f : 0.0f;
	const float Step = Dt / FMath::Max(0.05f, SwitchSeconds);
	Blend = Blend < Target ? FMath::Min(Target, Blend + Step) : FMath::Max(Target, Blend - Step);
	const float B = Blend * Blend * (3.0f - 2.0f * Blend);
	// The eyes, steadied: the stride's sway and bob come through softened, as a head holds its gaze.
	const FVector Eye = EyeLocal();
	if (!bEyeSteadyValid)
	{
		EyeSteady = Eye;
		bEyeSteadyValid = true;
	}
	const double Across = 1.0 - FMath::Exp(-14.0 * Dt);
	const double Upward = 1.0 - FMath::Exp(-22.0 * Dt);
	EyeSteady.X += (Eye.X - EyeSteady.X) * Across;
	EyeSteady.Y += (Eye.Y - EyeSteady.Y) * Across;
	EyeSteady.Z += (Eye.Z - EyeSteady.Z) * Upward;
	const FVector Pivot(0.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.62f);
	Boom->SetRelativeLocation(FMath::Lerp(Pivot, EyeSteady, static_cast<double>(B)));
	// The wheel sets ArmLength; the camera eases to it.
	ArmNow = FMath::FInterpTo(ArmNow, ArmLength, Dt, 9.0f);
	Boom->TargetArmLength = FMath::Lerp(ArmNow, 0.0f, B);
	Boom->SocketOffset = FMath::Lerp(ShoulderOffset, FVector::ZeroVector, static_cast<double>(B));
	Boom->bDoCollisionTest = B < 0.5f;
	Boom->bEnableCameraLag = B < 0.5f;
	// A touch wider at a run.
	FovKick = FMath::FInterpTo(FovKick, bRunning && GetVelocity().Size2D() > WalkSpeed * 1.3f ? 3.0f : 0.0f, Dt, 3.0f);
	Camera->SetFieldOfView(FMath::Lerp(ThirdPersonFov, FirstPersonFov, B) + FovKick);
	bool bChanged = false;
	const bool bShowHead = B < 0.6f;
	if (bShowHead == bHeadHidden)
	{
		bHeadHidden = !bShowHead;
		bChanged = true;
	}
	// Over the shoulder in a tight spot the boom's sweep pulls the camera into the body: hide it rather than look out
	// through it (its shadow stays).
	bool bTooClose = false;
	if (B < 0.5f)
	{
		const float Half = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		const FVector Axis = GetActorUpVector() * Half;
		const float Dist = static_cast<float>(FMath::PointDistToSegment(Camera->GetComponentLocation(), GetActorLocation() - Axis, GetActorLocation() + Axis));
		bTooClose = Dist < (bBodyHidden ? 52.0f : 40.0f);
	}
	if (bTooClose != bBodyHidden)
	{
		bBodyHidden = bTooClose;
		bChanged = true;
	}
	if (bChanged)
	{
		RefreshVisibility();
	}
}

void AShortStackCharacter::UpdateLook(float Dt)
{
	const float BodyYaw = static_cast<float>(GetActorRotation().Yaw);
	float WantYaw = 0.0f;
	float WantPitch = 0.0f;
	float Rate = 7.0f;
	EyeTarget = FVector::ZeroVector;
	// How far a point is from where the body faces, as the head turns to it (degrees: yaw + to the right, pitch + up).
	auto Aim = [this, BodyYaw](const FVector& At, float& OutYaw, float& OutPitch) {
		const FRotator R = (At - EyeLocation()).Rotation();
		OutYaw = FMath::FindDeltaAngleDegrees(BodyYaw, static_cast<float>(R.Yaw));
		OutPitch = static_cast<float>(FRotator::NormalizeAxis(R.Pitch));
	};
	if (bNpc)
	{
		// Benny: the player, once they're a few steps away and in front of him; otherwise the counter, idly.
		WantYaw = 8.0f * FMath::Sin(Clock * 0.11f) + 4.0f * FMath::Sin(Clock * 0.37f);
		WantPitch = -6.0f;
		Rate = 3.0f;
		if (!WorkBeat.IsNone())
		{
			// At work: the eyes on the hands' work, drifting a little (spec 6.6's attention stack, its point of interest).
			float Yaw = 0.0f;
			float Pitch = 0.0f;
			Aim(WorkStation.TransformPosition(WorkLookAt), Yaw, Pitch);
			WantYaw = FMath::Clamp(Yaw, -60.0f, 60.0f) + 4.0f * FMath::Sin(Clock * 0.23f);
			WantPitch = FMath::Clamp(Pitch, -45.0f, 10.0f);
		}
		const APlayerController* Pc = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		const AShortStackCharacter* Player = Pc ? Cast<AShortStackCharacter>(Pc->GetPawn()) : nullptr;
		if (Player && Player != this)
		{
			const FVector At = Player->EyeLocation();
			const FVector To = At - EyeLocation();
			if (To.SizeSquared() < FMath::Square(700.0))
			{
				const FRotator R = To.Rotation();
				const float Yaw = FMath::FindDeltaAngleDegrees(BodyYaw, static_cast<float>(R.Yaw));
				if (FMath::Abs(Yaw) < 105.0f)
				{
					WantYaw = Yaw;
					WantPitch = static_cast<float>(FRotator::NormalizeAxis(R.Pitch));
					EyeTarget = At;
					Rate = 5.0f;
				}
			}
		}
	}
	else if (GetController())
	{
		const FRotator View = GetControlRotation();
		WantYaw = FMath::FindDeltaAngleDegrees(BodyYaw, static_cast<float>(View.Yaw));
		WantPitch = static_cast<float>(FRotator::NormalizeAxis(View.Pitch));
		// The camera out in front of the face: look ahead rather than wring the neck round toward it. Faded in over
		// 95-130 degrees, so orbiting the camera across that line doesn't swing the head back and forth.
		const float TowardView = 1.0f - FMath::SmoothStep(95.0f, 130.0f, FMath::Abs(WantYaw));
		WantYaw *= TowardView;
		WantPitch *= FMath::Lerp(0.3f, 1.0f, TowardView);
		EyeTarget = EyeLocation() + FRotator(WantPitch, BodyYaw + WantYaw, 0.0f).Vector() * 800.0;
	}
	// Someone or something to look at (the one they're talking to, what the director points them at), over all of the above
	// by its weight. The neck turns so far; past that people-ue's turn in place turns the body. The player's own head in
	// first person stays the camera's.
	if (LookTargetWeight > 0.0f && !(bFirstPerson && !bNpc))
	{
		float Yaw = 0.0f;
		float Pitch = 0.0f;
		Aim(LookTargetAt, Yaw, Pitch);
		WantYaw = FMath::Lerp(WantYaw, FMath::Clamp(Yaw, -105.0f, 105.0f), LookTargetWeight);
		WantPitch = FMath::Lerp(WantPitch, FMath::Clamp(Pitch, -50.0f, 40.0f), LookTargetWeight);
		if (LookTargetWeight >= 0.5f || EyeTarget.IsZero())
		{
			EyeTarget = LookTargetAt;
		}
		Rate = FMath::Max(Rate, 5.0f);
	}
	// First person: the head is the camera's, at once. Otherwise it eases round, as heads do.
	const float Alpha = bFirstPerson && !bNpc ? 1.0f : 1.0f - FMath::Exp(-Rate * Dt);
	LookYawNow = FMath::Lerp(LookYawNow, WantYaw, Alpha);
	LookPitchNow = FMath::Lerp(LookPitchNow, WantPitch, Alpha);
}

void AShortStackCharacter::UpdateFace(float Dt)
{
	UBackRoomFaceAnim* Anim = Cast<UBackRoomFaceAnim>(Face->GetAnimInstance());
	if (!Anim)
	{
		return;
	}
	if (!Anim->Body.IsValid())
	{
		Anim->Body = GetMesh();
	}
	// Your own face in first person is there for its shadow: it follows the head, with nothing for RigLogic to work out. Past
	// the Near tier nobody reads an expression either (spec 6.7).
	Anim->bExpressionless = (bHeadHidden && !bNpc) || (bTierApplied && DetailTierNow >= 1);
	if (Anim->bExpressionless)
	{
		Anim->LookAt = FVector::ZeroVector;
		return;
	}
	// This frame's controls, written over last frame's (the map keeps its storage), each added to what's there.
	TMap<FName, float>& C = Anim->Curves;
	C.Reset();
	auto Add = [&C](FName Control, float Value) {
		if (Value > 0.0005f)
		{
			float& V = C.FindOrAdd(Control);
			V = FMath::Clamp(V + Value, 0.0f, 1.0f);
		}
	};
	// Blinks: every two to five seconds, now and then a double.
	NextBlink -= Dt;
	if (BlinkT < 0.0f && NextBlink <= 0.0f)
	{
		BlinkT = 0.0f;
		NextBlink = Rng.FRandRange(1.8f, 5.5f);
		bDoubleBlink = Rng.FRand() < 0.12f;
	}
	if (BlinkT >= 0.0f)
	{
		BlinkT += Dt / 0.17f;
		const float Shut = BlinkT < 1.0f ? FMath::Pow(FMath::Sin(BlinkT * PI), 0.6f) : 0.0f;
		Add(CurveBlinkL, Shut);
		Add(CurveBlinkR, Shut);
		if (BlinkT >= 1.0f)
		{
			BlinkT = bDoubleBlink ? 0.0f : -1.0f;
			bDoubleBlink = false;
		}
	}
	// The lips just parted, open to breathe after a run, open for the can or the bite as the hand gets there.
	const float SipT = Clock - SipAt;
	const float Sipping = SipT >= 0.0f && SipT < 1.8f ? FMath::SmoothStep(0.3f, 0.6f, SipT) * (1.0f - FMath::SmoothStep(1.1f, 1.5f, SipT)) : 0.0f;
	Add(CurveJawOpen, 0.02f + 0.12f * Winded + 0.22f * Sipping);
	Add(CurveRelaxL, 0.1f);
	Add(CurveRelaxR, 0.1f);
	// Talking (SetTalking): the Back Room's syllables, over the parted lips.
	const float TalkW = TalkFace::Weight(TalkLeft);
	if (TalkW > 0.0f)
	{
		// The rig's control names made once per control and side (TalkFace's names are literals: their pointers are stable),
		// not formatted every frame on everyone talking.
		auto ControlName = [](const TCHAR* Control, int32 Side) -> FName {
			// By the literal's address and the side (0 none, 1 L, 2 R): one key each.
			static TMap<UPTRINT, FName> Names;
			const UPTRINT Key = reinterpret_cast<UPTRINT>(Control) * 4 + static_cast<UPTRINT>(Side);
			if (const FName* Have = Names.Find(Key))
			{
				return *Have;
			}
			const FName Made(*FString::Printf(TEXT("CTRL_expressions_%s%s"), Control, Side == 1 ? TEXT("L") : (Side == 2 ? TEXT("R") : TEXT(""))));
			Names.Add(Key, Made);
			return Made;
		};
		auto AddControl = [&Add, &ControlName](const TCHAR* Control, float Value) { Add(ControlName(Control, 0), Value); };
		auto BothControls = [&Add, &ControlName](const TCHAR* Control, float Value) {
			Add(ControlName(Control, 1), Value);
			Add(ControlName(Control, 2), Value);
		};
		TalkFace::Mouth(Clock, TalkW, TalkLoudness, AddControl, BothControls);
	}
	// What the director holds on the face (a double take's brows, a smile).
	for (const TPair<FName, float>& Held : FaceCurves)
	{
		Add(Held.Key, Held.Value);
	}
	Anim->LookAt = EyeTarget.IsZero() ? FVector::ZeroVector : Face->GetComponentTransform().InverseTransformPosition(EyeTarget);
}

// ------------------------------------------------------------------ moving

void AShortStackCharacter::Drive(float Forward, float Right, bool bRun)
{
	if (bSeated)
	{
		// Seated, the keys move nobody: standing up is the game's (SetSeated(false)).
		bRunning = false;
		return;
	}
	bRunning = bRun;
	GetCharacterMovement()->MaxWalkSpeed = bRun ? RunSpeed : WalkSpeed;
	const FRotator Yaw(0.0f, GetControlRotation().Yaw, 0.0f);
	const FRotationMatrix M(Yaw);
	if (!FMath::IsNearlyZero(Forward))
	{
		AddMovementInput(M.GetUnitAxis(EAxis::X), Forward);
	}
	if (!FMath::IsNearlyZero(Right))
	{
		AddMovementInput(M.GetUnitAxis(EAxis::Y), Right);
	}
}

void AShortStackCharacter::Look(float DeltaYaw, float DeltaPitch)
{
	if (AController* C = GetController())
	{
		FRotator R = C->GetControlRotation();
		R.Yaw += DeltaYaw;
		R.Pitch = FMath::ClampAngle(R.Pitch + static_cast<double>(DeltaPitch), -70.0, 65.0);
		C->SetControlRotation(R);
	}
}

bool AShortStackCharacter::TakeFootstep(float& OutVolume)
{
	if (!bStepPending)
	{
		return false;
	}
	bStepPending = false;
	OutVolume = StepVolume;
	return true;
}

void AShortStackCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bHasLook)
	{
		// Nobody dressed us by the first frame: the creator's default person, so there's someone to walk.
		ApplyLook(ss::hero::Character());
	}
	// At most a twentieth of a second at a time, or the time between ticks when a detail tier spaces them out (otherwise the
	// gait would run slow at 10 ticks a second).
	const float Dt = FMath::Min(DeltaSeconds, FMath::Max(0.05f, PrimaryActorTick.TickInterval + 0.05f));
	Clock += Dt;
	TalkLeft = FMath::Max(0.0f, TalkLeft - Dt);
	SeatBlend = FMath::FInterpConstantTo(SeatBlend, bSeated ? 1.0f : 0.0f, Dt, 1.0f / 0.6f);
	const float Speed = static_cast<float>(GetVelocity().Size2D());
	const float Run = FMath::Clamp((Speed - WalkSpeed) / FMath::Max(1.0f, RunSpeed - WalkSpeed), 0.0f, 1.0f);
	// One gait cycle (two steps) covers about 1.4 m walking and 2.6 m running, longer for taller people.
	const float Cycle = FMath::Lerp(140.0f, 260.0f, Run) * MeshScale;
	const float Before = Phase;
	const float Next = Before + 2.0f * PI * Speed / Cycle * Dt;
	// A heel strikes a quarter cycle after each leg passes under the body (at pi/2 and 3pi/2). Counted on the phase
	// before it wraps, so coming round past 2pi isn't a step of its own.
	if (Speed > 30.0f && FMath::FloorToInt((Next - 0.5f * PI) / PI) != FMath::FloorToInt((Before - 0.5f * PI) / PI))
	{
		bStepPending = true;
		StepVolume = FMath::Lerp(0.25f, 0.6f, Run);
	}
	Phase = FMath::Fmod(Next, 2.0f * PI);
	if (Speed < 5.0f)
	{
		// Standing: settle back to the neutral stance.
		Phase = FMath::FInterpTo(Phase, Phase < PI ? 0.0f : 2.0f * PI, Dt, 4.0f);
	}
	const float Yaw = static_cast<float>(GetActorRotation().Yaw);
	const float TurnRate = FMath::FindDeltaAngleDegrees(LastYaw, Yaw) / FMath::Max(Dt, 0.001f);
	LastYaw = Yaw;
	Bank = FMath::FInterpTo(Bank, FMath::Clamp(-TurnRate * Speed / 40000.0f, -8.0f, 8.0f), Dt, 6.0f);
	// Setting off and pulling up: the trunk leans into it and rocks back.
	const float Accel = (Speed - LastSpeed) / FMath::Max(Dt, 0.001f);
	LastSpeed = Speed;
	Surge = FMath::FInterpTo(Surge, FMath::Clamp(Accel / 900.0f, -1.0f, 1.0f), Dt, 6.0f);
	// Out of breath for a while after a run.
	Winded = FMath::FInterpTo(Winded, Run, Dt, Run > Winded ? 0.6f : 0.2f);
	UpdateLook(Dt);

	if (UStreetBodyAnim* Anim = Cast<UStreetBodyAnim>(GetMesh()->GetAnimInstance()))
	{
		FStreetBodyPose& P = Anim->Pose;
		P.Speed = Speed;
		P.Phase = Phase;
		P.Run = Run;
		P.Bank = Bank;
		P.Surge = Surge;
		P.Tired = Tired;
		P.Time = Clock;
		P.Mouth = MouthFromHead;
		P.LookYaw = LookYawNow;
		// Talking nods along (TalkFace.h; the pose has no roll for its tilt yet).
		P.LookPitch = LookPitchNow + TalkFace::Nod(Clock, TalkFace::Weight(TalkLeft));
		// Up to the mouth, held there a moment, and down again.
		const float SipT = Clock - SipAt;
		P.Sip = SipT >= 0.0f && SipT < 1.8f ? FMath::SmoothStep(0.0f, 0.55f, SipT) * (1.0f - FMath::SmoothStep(1.2f, 1.8f, SipT)) : 0.0f;
	}
	UpdateFace(Dt);
	if (IsPlayerControlled())
	{
		UpdateCamera(Dt);
	}
}

// ------------------------------------------------------------------ people (spec 7.5.4)

void AShortStackCharacter::SetLookTarget(const FVector& WorldAt, float Weight)
{
	LookTargetAt = WorldAt;
	LookTargetWeight = FMath::Clamp(Weight, 0.0f, 1.0f);
}

void AShortStackCharacter::SetTalking(float Seconds, float Loudness)
{
	TalkLeft = FMath::Max(0.0f, Seconds);
	TalkLoudness = FMath::Clamp(Loudness, 0.0f, 1.0f);
}

void AShortStackCharacter::SetFaceCurve(FName Control, float Value)
{
	const FName Key(*FString::Printf(TEXT("CTRL_expressions_%s"), *Control.ToString()));
	if (Value > 0.0f)
	{
		FaceCurves.Add(Key, FMath::Min(Value, 1.0f));
	}
	else
	{
		FaceCurves.Remove(Key);
	}
}

UStreetBodyAnim* AShortStackCharacter::StreetBody() const
{
	const USkeletalMeshComponent* Body = GetMesh();
	return Body ? Cast<UStreetBodyAnim>(Body->GetAnimInstance()) : nullptr;
}

void AShortStackCharacter::SetSeated(bool bSit, float SeatHeight)
{
	SeatHeightCm = FMath::Clamp(SeatHeight, 20.0f, 110.0f);
	if (bSit == bSeated)
	{
		return;
	}
	bSeated = bSit;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!Move)
	{
		return;
	}
	if (bSit)
	{
		// Held where they sat: nothing walks them off the seat, gravity included.
		MoveModeBeforeSeat = static_cast<uint8>(Move->MovementMode.GetValue());
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}
	else if (MoveModeBeforeSeat != static_cast<uint8>(MOVE_None))
	{
		Move->SetMovementMode(static_cast<EMovementMode>(MoveModeBeforeSeat));
	}
}

void AShortStackCharacter::SetWork(FName Beat, const FTransform& Station)
{
	if (Beat != WorkBeat)
	{
		WorkSince = Clock;
	}
	WorkBeat = Beat;
	WorkStation = Station;
}

void AShortStackCharacter::SetDetailTier(int32 Tier)
{
	Tier = FMath::Clamp(Tier, 0, 3);
	if (bTierApplied && Tier == DetailTierNow)
	{
		return;
	}
	DetailTierNow = Tier;
	bTierApplied = true;
	ApplyDetailTier();
}

void AShortStackCharacter::ApplyDetailTier()
{
	// Spec 6.7, built on The Block's crowd tiers (StreetCrowd.cpp's UpdateCost: forced LODs, slower poses, expressionless
	// faces, no shadows far off):
	//   Near  everything: RigLogic and the flap, the hair on its first cards LOD, posed every frame, shadows, ray tracing.
	//   Mid   no face rig; the hair a cards LOD lower (never the helmet); the actor and the body every 0.05 s with URO; only
	//         the body throws a shadow; out of ray tracing.
	//   Far   no face rig; the helmet; every 0.1 s; the body's third LOD and the face's fifth (the crowd's far tier); no
	//         shadows, no ray tracing.
	//   Gone  hidden, out of collision, nothing ticking (the pool hides and moves people; it never rebuilds them).
	// Near puts each part back as it was built (its own shadow, tick interval, URO and LOD, as recorded the first time a tier
	// saw it), with every groom held on its first cards LOD (a cinematic build's too: spec 6.1's cards on the street).
	// The face is posed with the body, not every 0.1 s as the spec's table has it at Mid: an expressionless face only copies
	// the body's neck and head, and one posed less often than its body opens a seam at the neck on anyone walking (the
	// crowd's reason for posing it every frame).
	const int32 Tier = DetailTierNow;
	if (Tier >= 3)
	{
		// Gone: hidden, out of collision, nothing ticking. Only what's on now is stopped (and recorded, so leaving Gone starts
		// only that again); the parts keep the last tier's settings while nobody sees them.
		if (!IsHidden())
		{
			SetActorHiddenInGame(true);
			bTierHidActor = true;
		}
		if (GetActorEnableCollision())
		{
			SetActorEnableCollision(false);
			bTierStoppedCollision = true;
		}
		if (IsActorTickEnabled())
		{
			SetActorTickEnabled(false);
			bTierStoppedTick = true;
		}
		TInlineComponentArray<UActorComponent*> All(this);
		for (UActorComponent* C : All)
		{
			const bool bPart = C->IsA<USkinnedMeshComponent>() || C->IsA<UGroomComponent>() || C == GetCharacterMovement();
			if (bPart && C->IsComponentTickEnabled())
			{
				C->SetComponentTickEnabled(false);
				TierStoppedTicks.AddUnique(C);
			}
		}
		return;
	}
	UndoGone();
	if (TierOwnTickInterval < 0.0f)
	{
		TierOwnTickInterval = PrimaryActorTick.TickInterval;
	}
	const float Interval = Tier == 1 ? 0.05f : 0.1f;
	SetActorTickInterval(Tier == 0 ? TierOwnTickInterval : Interval);
	TInlineComponentArray<UPrimitiveComponent*> Parts(this);
	for (UPrimitiveComponent* P : Parts)
	{
		if (!P || P->IsA<UShapeComponent>())
		{
			continue;
		}
		const USkinnedMeshComponent* SeenSkin = Cast<USkinnedMeshComponent>(P);
		const UGroomComponent* SeenGroom = Cast<UGroomComponent>(P);
		FTierPart* Known = TierDefaults.Find(P);
		if (!Known)
		{
			FTierPart Seen;
			Seen.bShadow = P->CastShadow;
			Seen.bTraced = P->bVisibleInRayTracing;
			Seen.bVisible = P->GetVisibleFlag();
			Seen.bUro = SeenSkin && SeenSkin->bEnableUpdateRateOptimizations;
			Seen.TickInterval = P->GetComponentTickInterval();
			Seen.ForcedLod = SeenSkin ? SeenSkin->GetForcedLOD() : (SeenGroom ? SeenGroom->GetForcedLOD() : 0);
			Known = &TierDefaults.Add(P, Seen);
		}
		const FTierPart Own = *Known;
		const bool bShadow = Own.bShadow && (Tier == 0 || (Tier == 1 && P == GetMesh()));
		if (static_cast<bool>(P->CastShadow) != bShadow)
		{
			P->SetCastShadow(bShadow);
		}
		const bool bTraced = Own.bTraced && Tier == 0;
		if (static_cast<bool>(P->bVisibleInRayTracing) != bTraced)
		{
			P->SetVisibleInRayTracing(bTraced);
		}
		if (USkinnedMeshComponent* Skin = Cast<USkinnedMeshComponent>(P))
		{
			Skin->SetComponentTickInterval(Tier == 0 ? Own.TickInterval : Interval);
			Skin->bEnableUpdateRateOptimizations = Tier == 0 ? Own.bUro : true;
			// Forced LODs count from 1 (0 lets the engine choose), and stop at what the mesh has.
			const int32 Lod = Tier >= 2 ? (Skin == Face.Get() ? 5 : 3) : Own.ForcedLod;
			if (Skin->GetForcedLOD() != Lod)
			{
				Skin->SetForcedLOD(Lod);
			}
		}
		else if (UGroomComponent* Hair = Cast<UGroomComponent>(P))
		{
			const bool bHeld = HeroGrooms::HoldForTier(Hair, Tier);
			// At Far, what has neither cards nor a helmet (lashes, peach fuzz, a strands-only buzz) isn't drawn at all.
			const bool bShow = Own.bVisible && (Tier < 2 || bHeld);
			if (Hair->GetVisibleFlag() != bShow)
			{
				Hair->SetVisibility(bShow);
			}
		}
	}
}

void AShortStackCharacter::UndoGone()
{
	if (bTierHidActor)
	{
		SetActorHiddenInGame(false);
		bTierHidActor = false;
	}
	if (bTierStoppedCollision)
	{
		SetActorEnableCollision(true);
		bTierStoppedCollision = false;
	}
	if (bTierStoppedTick)
	{
		SetActorTickEnabled(true);
		bTierStoppedTick = false;
	}
	for (const TWeakObjectPtr<UActorComponent>& Stopped : TierStoppedTicks)
	{
		if (UActorComponent* C = Stopped.Get())
		{
			C->SetComponentTickEnabled(true);
		}
	}
	TierStoppedTicks.Reset();
}

void AShortStackCharacter::RestoreTierDefaults()
{
	UndoGone();
	for (const TPair<TWeakObjectPtr<UPrimitiveComponent>, FTierPart>& Known : TierDefaults)
	{
		UPrimitiveComponent* P = Known.Key.Get();
		if (!P)
		{
			continue;
		}
		const FTierPart& Own = Known.Value;
		if (static_cast<bool>(P->CastShadow) != Own.bShadow)
		{
			P->SetCastShadow(Own.bShadow);
		}
		if (static_cast<bool>(P->bVisibleInRayTracing) != Own.bTraced)
		{
			P->SetVisibleInRayTracing(Own.bTraced);
		}
		if (USkinnedMeshComponent* Skin = Cast<USkinnedMeshComponent>(P))
		{
			Skin->SetComponentTickInterval(Own.TickInterval);
			Skin->bEnableUpdateRateOptimizations = Own.bUro;
			if (Skin->GetForcedLOD() != Own.ForcedLod)
			{
				Skin->SetForcedLOD(Own.ForcedLod);
			}
		}
		else if (UGroomComponent* Hair = Cast<UGroomComponent>(P))
		{
			if (Hair->GetVisibleFlag() != Own.bVisible)
			{
				Hair->SetVisibility(Own.bVisible);
			}
			if (Hair->GetForcedLOD() != Own.ForcedLod)
			{
				Hair->SetForcedLOD(Own.ForcedLod);
			}
		}
	}
	TierDefaults.Reset();
	if (TierOwnTickInterval >= 0.0f)
	{
		SetActorTickInterval(TierOwnTickInterval);
	}
}

// ------------------------------------------------------------------ grooms

UGroomComponent* AShortStackCharacter::ScalpGroom() const
{
	for (const TPair<TWeakObjectPtr<UGroomComponent>, uint8>& Worn : GroomKinds)
	{
		if (Worn.Key.IsValid() && static_cast<EGroomKind>(Worn.Value) == EGroomKind::Scalp)
		{
			return Worn.Key.Get();
		}
	}
	return nullptr;
}

TArray<UGroomComponent*> AShortStackCharacter::GetGrooms() const
{
	TArray<UGroomComponent*> Out;
	for (const TPair<TWeakObjectPtr<UGroomComponent>, uint8>& Worn : GroomKinds)
	{
		if (UGroomComponent* Groom = Worn.Key.Get())
		{
			Out.Add(Groom);
		}
	}
	return Out;
}

bool AShortStackCharacter::SwapGroom(UGroomComponent* Groom, UGroomAsset* Asset, UGroomBindingAsset* Binding)
{
	if (!Groom || !Asset)
	{
		return false;
	}
	// The last groom's shading was made from its own materials (the cards' textures are each groom's own): let it go first.
	Groom->EmptyOverrideMaterials();
	Groom->SetGroomAsset(Asset, Binding);
	if (!HeroGrooms::HoldForTier(Groom, bTierApplied ? FMath::Min(DetailTierNow, 2) : 0) && Groom->GetForcedLOD() != INDEX_NONE)
	{
		// Nothing to hold (a strands-only groom: the buzz cuts, the stubble): its own LODs, as a fresh build leaves it, not
		// the last groom's index.
		Groom->SetForcedLOD(INDEX_NONE);
	}
	if (FTierPart* Known = TierDefaults.Find(Groom))
	{
		// As built is now this groom's own hold (its first cards LOD, or none), not the last groom's.
		Known->ForcedLod = HeroGrooms::LodForTier(Asset, 0);
	}
	const EGroomKind Kind = KindIn(GroomKinds, Groom);
	if (!bNpc && Kind != EGroomKind::Lashes)
	{
		ShadeGroom(Groom, HairShadeOf(LookNow.HairColor, LookAge), Kind == EGroomKind::Brows ? 0.06f : 0.0f);
	}
	if (Kind == EGroomKind::Scalp)
	{
		bWearsScalpHair = true;
	}
	if (bTierApplied)
	{
		// Shown or not at this tier as the new groom has it (Far hides what has no helmet and no cards).
		ApplyDetailTier();
	}
	return true;
}

UGroomComponent* AShortStackCharacter::AddScalpGroom()
{
	if (UGroomComponent* Have = ScalpGroom())
	{
		return Have;
	}
	// As the MetaHuman Blueprints carry their hair: on the face, which its binding follows.
	UGroomComponent* Hair = NewObject<UGroomComponent>(this, MakeUniqueObjectName(this, UGroomComponent::StaticClass(), TEXT("Hair")), RF_Transient);
	Hair->SetupAttachment(Face);
	Hair->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Hair->SetCanEverAffectNavigation(false);
	Hair->bCastHiddenShadow = true;
	Hair->RegisterComponent();
	Wearables.Add(Hair);
	GroomKinds.Emplace(Hair, static_cast<uint8>(EGroomKind::Scalp));
	RefreshVisibility();
	if (bTierApplied)
	{
		ApplyDetailTier();
	}
	return Hair;
}

void AShortStackCharacter::DropGroom(UGroomComponent* Groom)
{
	if (!Groom)
	{
		return;
	}
	Wearables.Remove(Groom);
	GroomKinds.RemoveAll([Groom](const TPair<TWeakObjectPtr<UGroomComponent>, uint8>& Worn) { return !Worn.Key.IsValid() || Worn.Key.Get() == Groom; });
	TierDefaults.Remove(Groom);
	Groom->DestroyComponent();
	bWearsScalpHair = ScalpGroom() != nullptr;
}

UGroomComponent* AShortStackCharacter::GroomOfKind(uint8 Kind) const
{
	for (const TPair<TWeakObjectPtr<UGroomComponent>, uint8>& Worn : GroomKinds)
	{
		if (Worn.Key.IsValid() && Worn.Value == Kind)
		{
			return Worn.Key.Get();
		}
	}
	return nullptr;
}

UGroomComponent* AShortStackCharacter::AddGroomOfKind(uint8 Kind, UGroomAsset* Asset, UGroomBindingAsset* Binding)
{
	// The build's own groom of this kind, as the template (its settings, as the Blueprint made it), when it has one.
	const UGroomComponent* Template = nullptr;
	if (const UBlueprintGeneratedClass* Bp = Cast<UBlueprintGeneratedClass>(WornBlueprint.Get()); Bp && Bp->SimpleConstructionScript)
	{
		for (const USCS_Node* Node : Bp->SimpleConstructionScript->GetAllNodes())
		{
			const UGroomComponent* T = Node ? Cast<UGroomComponent>(Node->ComponentTemplate) : nullptr;
			if (T && T->GroomAsset && static_cast<uint8>(GroomKindOf(Node->GetVariableName().ToString(), T->GroomAsset->GetName())) == Kind)
			{
				Template = T;
				break;
			}
		}
	}
	static const TCHAR* const Names[6] = {TEXT("Hair"), TEXT("Eyebrows"), TEXT("Eyelashes"), TEXT("Beard"), TEXT("Mustache"), TEXT("Fuzz")};
	UGroomComponent* Groom = NewObject<UGroomComponent>(this, UGroomComponent::StaticClass(),
		MakeUniqueObjectName(this, UGroomComponent::StaticClass(), FName(Names[FMath::Min<int32>(Kind, 5)])), RF_Transient, const_cast<UGroomComponent*>(Template));
	// As the MetaHuman Blueprints carry their grooms: on the face, which the binding follows.
	Groom->SetupAttachment(Face);
	Groom->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Groom->SetCanEverAffectNavigation(false);
	Groom->bCastHiddenShadow = true;
	// The template's overrides are its own groom's: this one shades its own materials. Set before it registers, so it's
	// built once, as the new groom.
	Groom->EmptyOverrideMaterials();
	Groom->SetGroomAsset(Asset, Binding);
	Groom->RegisterComponent();
	Wearables.Add(Groom);
	GroomKinds.Emplace(Groom, Kind);
	if (!HeroGrooms::HoldForTier(Groom, bTierApplied ? FMath::Min(DetailTierNow, 2) : 0) && Groom->GetForcedLOD() != INDEX_NONE)
	{
		Groom->SetForcedLOD(INDEX_NONE);
	}
	if (!bNpc && static_cast<EGroomKind>(Kind) != EGroomKind::Lashes)
	{
		ShadeGroom(Groom, HairShadeOf(LookNow.HairColor, LookAge), static_cast<EGroomKind>(Kind) == EGroomKind::Brows ? 0.06f : 0.0f);
	}
	if (static_cast<EGroomKind>(Kind) == EGroomKind::Scalp)
	{
		bWearsScalpHair = true;
	}
	RefreshVisibility();
	if (bTierApplied)
	{
		ApplyDetailTier();
	}
	return Groom;
}

void AShortStackCharacter::ApplyGrooms(bool bLoadNow)
{
	if (bNpc || CastFaceName.IsNone())
	{
		return;
	}
	const HeroGrooms::FGroomSet Set = HeroGrooms::Resolve(GroomLook(), CastFaceName);
	if (!Set.bLibrary)
	{
		// No library for this face: the build's own grooms, as BuildFromMetaHuman picked them.
		return;
	}
	const uint32 Request = ++GroomRequest;
	if (!HeroGrooms::IsResident(Set))
	{
		if (!bLoadNow)
		{
			// Not in memory (the Style Book's preload hasn't finished, or a look from elsewhere): on when it arrives, unless a
			// newer look was asked for meanwhile. Nothing loads synchronously here. Once it has arrived the set is resident, so
			// the second pass loads nothing; an entry whose asset failed to load (a copy deleted by hand) is tried once and
			// dropped there, rather than asked for again every frame.
			TWeakObjectPtr<AShortStackCharacter> Self(this);
			HeroGrooms::LoadAsync(Set, [Self, Request]() {
				AShortStackCharacter* Me = Self.Get();
				if (Me && Me->GroomRequest == Request)
				{
					Me->ApplyGrooms(true);
				}
			});
			return;
		}
		for (const FSoftObjectPath& Path : HeroGrooms::PathsOf(Set))
		{
			Path.TryLoad();
		}
	}
	const FHairShade Shade = HairShadeOf(LookNow.HairColor, LookAge);
	for (const EGroomKind Kind : {EGroomKind::Scalp, EGroomKind::Brows, EGroomKind::Beard, EGroomKind::Mustache})
	{
		const HeroGrooms::FWornGroom& Want = Set.Of(Kind);
		UGroomAsset* Asset = Want.IsSet() ? Want.Groom.Get() : nullptr;
		UGroomBindingAsset* Binding = Want.IsSet() ? Want.Binding.Get() : nullptr;
		UGroomComponent* Have = GroomOfKind(static_cast<uint8>(Kind));
		if (!Asset)
		{
			if (Have)
			{
				DropGroom(Have);
			}
			continue;
		}
		if (!Have)
		{
			AddGroomOfKind(static_cast<uint8>(Kind), Asset, Binding);
		}
		else if (Have->GroomAsset != Asset || Have->BindingAsset != Binding)
		{
			SwapGroom(Have, Asset, Binding);
		}
		else
		{
			// The same groom: the hair color may be new (the barber's COLOR tab).
			ShadeGroom(Have, Shade, Kind == EGroomKind::Brows ? 0.06f : 0.0f);
		}
	}
	bWearsScalpHair = ScalpGroom() != nullptr;
	if (bHasLook && HatMesh)
	{
		// The hat sits a little higher over hair than on the bare scalp.
		PutOnHeadwear(CurrentOutfit());
	}
}

// ------------------------------------------------------------------ tests

FString AShortStackCharacter::TestGroomSpike(const FString& Candidate, const FString& Options)
{
	return HeroGrooms::StartSpike(this, Candidate.IsEmpty() ? FString(TEXT("all")) : Candidate, Options);
}

FString AShortStackCharacter::TestGroomSpikeReport() const
{
	return HeroGrooms::SpikeReport();
}

FString AShortStackCharacter::TestHook(const FString& Command)
{
	TArray<FString> A;
	Command.ParseIntoArrayWS(A);
	const FString Verb = A.Num() > 0 ? A[0].ToLower() : FString();
	auto Num = [&A](int32 I, float Default) { return A.IsValidIndex(I) ? FCString::Atof(*A[I]) : Default; };
	if (Verb == TEXT("tier"))
	{
		SetDetailTier(FMath::RoundToInt(Num(1, 0.0f)));
	}
	else if (Verb == TEXT("talk"))
	{
		SetTalking(Num(1, 3.0f), Num(2, 0.6f));
	}
	else if (Verb == TEXT("look"))
	{
		SetLookTarget(FVector(Num(1, 0.0f), Num(2, 0.0f), Num(3, 0.0f)), Num(4, 1.0f));
	}
	else if (Verb == TEXT("lookhero"))
	{
		const APlayerController* Pc = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		const AShortStackCharacter* Player = Pc ? Cast<AShortStackCharacter>(Pc->GetPawn()) : nullptr;
		if (Player && Player != this)
		{
			SetLookTarget(Player->EyeLocation(), Num(1, 1.0f));
		}
	}
	else if (Verb == TEXT("face") && A.Num() > 1)
	{
		SetFaceCurve(FName(*A[1]), Num(2, 0.6f));
	}
	else if (Verb == TEXT("sit"))
	{
		SetSeated(true, Num(1, 46.0f));
	}
	else if (Verb == TEXT("stand"))
	{
		SetSeated(false, SeatHeightCm);
	}
	else if (Verb == TEXT("work"))
	{
		// The station where they stand: the floor under them, facing their way.
		const FVector Floor = GetActorLocation() - FVector(0.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		SetWork(A.IsValidIndex(1) && !A[1].Equals(TEXT("none"), ESearchCase::IgnoreCase) ? FName(*A[1]) : FName(NAME_None), FTransform(GetActorRotation(), Floor));
	}
	else if (Verb == TEXT("selftest"))
	{
		return SelfTestHooks();
	}
	else if (!Verb.IsEmpty() && Verb != TEXT("describe"))
	{
		return FString::Printf(TEXT("Unknown hook '%s' (tier, talk, look, lookhero, face, sit, stand, work, describe, selftest)"), *Verb);
	}
	return DescribeHooks();
}

namespace ShortStackCharacterDetail
{
/** What the detail tiers touch on one part, for the self-test's before and after. */
struct FHookPartState
{
	TWeakObjectPtr<UPrimitiveComponent> Part;
	bool bShadow = false;
	bool bTraced = false;
	bool bVisible = false;
	bool bTicking = false;
	bool bUro = false;
	float Interval = 0.0f;
	int32 Lod = 0;
};

TArray<FHookPartState> SnapParts(AActor* Actor)
{
	TArray<FHookPartState> Out;
	TInlineComponentArray<UPrimitiveComponent*> Parts(Actor);
	for (UPrimitiveComponent* P : Parts)
	{
		FHookPartState& S = Out.AddDefaulted_GetRef();
		S.Part = P;
		S.bShadow = P->CastShadow;
		S.bTraced = P->bVisibleInRayTracing;
		S.bVisible = P->GetVisibleFlag();
		S.bTicking = P->IsComponentTickEnabled();
		S.Interval = P->GetComponentTickInterval();
		if (const USkinnedMeshComponent* Skin = Cast<USkinnedMeshComponent>(P))
		{
			S.bUro = Skin->bEnableUpdateRateOptimizations;
			S.Lod = Skin->GetForcedLOD();
		}
		else if (const UGroomComponent* Groom = Cast<UGroomComponent>(P))
		{
			S.Lod = Groom->GetForcedLOD();
		}
	}
	return Out;
}

/** The parts that aren't as they were (by name and what changed). */
TArray<FString> PartsChanged(const TArray<FHookPartState>& Before, const TArray<FHookPartState>& After)
{
	TArray<FString> Out;
	for (const FHookPartState& B : Before)
	{
		const FHookPartState* A = After.FindByPredicate([&B](const FHookPartState& S) { return S.Part == B.Part; });
		if (!A || !B.Part.IsValid())
		{
			continue;
		}
		if (A->bShadow != B.bShadow || A->bTraced != B.bTraced || A->bVisible != B.bVisible || A->bTicking != B.bTicking || A->bUro != B.bUro
			|| !FMath::IsNearlyEqual(A->Interval, B.Interval) || A->Lod != B.Lod)
		{
			Out.Add(FString::Printf(TEXT("%s (shadow %d>%d, traced %d>%d, visible %d>%d, ticking %d>%d, URO %d>%d, interval %.2f>%.2f, LOD %d>%d)"), *GetNameSafe(B.Part.Get()),
				B.bShadow, A->bShadow, B.bTraced, A->bTraced, B.bVisible, A->bVisible, B.bTicking, A->bTicking, B.bUro, A->bUro, B.Interval, A->Interval, B.Lod, A->Lod));
		}
	}
	return Out;
}
} // namespace ShortStackCharacterDetail

FString AShortStackCharacter::SelfTestHooks()
{
	TArray<FString> Fails;
	int32 Checks = 0;
	auto Check = [&Fails, &Checks](bool bOk, const FString& What) {
		++Checks;
		if (!bOk)
		{
			Fails.Add(What);
		}
	};
	// As found: put back at the end, and compared with what the tiers leave.
	const bool bWasApplied = bTierApplied;
	const int32 WasTier = DetailTierNow;
	const bool bWasHidden = IsHidden();
	// Hidden by someone else (the crowd keeping a walker off the street), not by Gone: no tier shows them.
	const bool bHiddenByOthers = bWasHidden && !bTierHidActor;
	const bool bWasColliding = GetActorEnableCollision();
	const bool bWasTicking = IsActorTickEnabled();
	const bool bMoveWasTicking = GetCharacterMovement() && GetCharacterMovement()->IsComponentTickEnabled();
	const float WasInterval = PrimaryActorTick.TickInterval;
	const TArray<FHookPartState> Before = SnapParts(this);

	// The tiers, each checked on every part against its own settings as the tiers recorded them (the first tier set records
	// them; with none set before this, that's how it was found).
	for (int32 Tier = 0; Tier <= 3; ++Tier)
	{
		SetDetailTier(Tier);
		Check(GetDetailTier() == Tier, FString::Printf(TEXT("tier %d not taken"), Tier));
		if (Tier == 3)
		{
			Check(IsHidden(), TEXT("Gone: still shown"));
			Check(!GetActorEnableCollision(), TEXT("Gone: still colliding"));
			Check(!IsActorTickEnabled(), TEXT("Gone: the actor still ticks"));
			Check(!GetMesh()->IsComponentTickEnabled(), TEXT("Gone: the body still ticks"));
			continue;
		}
		Check(IsHidden() == bHiddenByOthers, FString::Printf(TEXT("tier %d: hidden %d"), Tier, IsHidden() ? 1 : 0));
		const float Want = Tier == 0 ? TierOwnTickInterval : (Tier == 1 ? 0.05f : 0.1f);
		Check(FMath::IsNearlyEqual(PrimaryActorTick.TickInterval, Want), FString::Printf(TEXT("tier %d: the actor ticks every %.2f s, not %.2f"), Tier, PrimaryActorTick.TickInterval, Want));
		for (const FHookPartState& Was : Before)
		{
			UPrimitiveComponent* P = Was.Part.Get();
			const FTierPart* Own = P ? TierDefaults.Find(P) : nullptr;
			if (!P || P->IsA<UShapeComponent>())
			{
				continue;
			}
			const FString Name = P->GetName();
			if (!Own)
			{
				Check(false, FString::Printf(TEXT("tier %d: %s never recorded"), Tier, *Name));
				continue;
			}
			const bool bWantShadow = Own->bShadow && (Tier == 0 || (Tier == 1 && P == GetMesh()));
			Check(static_cast<bool>(P->CastShadow) == bWantShadow, FString::Printf(TEXT("tier %d: %s shadow %d"), Tier, *Name, P->CastShadow ? 1 : 0));
			Check(static_cast<bool>(P->bVisibleInRayTracing) == (Own->bTraced && Tier == 0), FString::Printf(TEXT("tier %d: %s in ray tracing %d"), Tier, *Name, P->bVisibleInRayTracing ? 1 : 0));
			if (const USkinnedMeshComponent* Skin = Cast<USkinnedMeshComponent>(P))
			{
				Check(Skin->bEnableUpdateRateOptimizations == (Tier == 0 ? Own->bUro : true), FString::Printf(TEXT("tier %d: %s URO"), Tier, *Name));
				Check(FMath::IsNearlyEqual(Skin->GetComponentTickInterval(), Tier == 0 ? Own->TickInterval : Want), FString::Printf(TEXT("tier %d: %s ticks every %.2f s"), Tier, *Name, Skin->GetComponentTickInterval()));
				// A forced LOD is clamped to the LODs the mesh has (none without a mesh).
				Check(Tier < 2 ? Skin->GetForcedLOD() == Own->ForcedLod : (Skin->GetForcedLOD() >= 1 || Skin->GetNumLODs() == 0),
					FString::Printf(TEXT("tier %d: %s forced LOD %d"), Tier, *Name, Skin->GetForcedLOD()));
			}
			else if (const UGroomComponent* Hair = Cast<UGroomComponent>(P))
			{
				const int32 Lod = HeroGrooms::LodForTier(Hair->GroomAsset, Tier);
				if (Lod != INDEX_NONE)
				{
					Check(Hair->GetForcedLOD() == Lod, FString::Printf(TEXT("tier %d: %s on LOD %d, not %d"), Tier, *Name, Hair->GetForcedLOD(), Lod));
				}
				if (Tier == 1)
				{
					const HeroGrooms::FGroomLods Lods = HeroGrooms::LodsOf(Hair->GroomAsset);
					Check(Lods.Helmet == INDEX_NONE || Hair->GetForcedLOD() < Lods.Helmet, FString::Printf(TEXT("Mid: %s on its helmet"), *Name));
				}
				Check(Hair->GetVisibleFlag() == (Own->bVisible && (Tier < 2 || Lod != INDEX_NONE)), FString::Printf(TEXT("tier %d: %s visible %d"), Tier, *Name, Hair->GetVisibleFlag() ? 1 : 0));
			}
		}
	}
	// Back as it was: the tier it had, or none at all.
	if (bWasApplied)
	{
		SetDetailTier(WasTier);
	}
	else
	{
		SetDetailTier(0);
		RestoreTierDefaults();
		bTierApplied = false;
		DetailTierNow = 0;
	}
	Check(IsHidden() == bWasHidden && GetActorEnableCollision() == bWasColliding && IsActorTickEnabled() == bWasTicking,
		TEXT("the actor isn't shown, colliding and ticking as it was"));
	Check((GetCharacterMovement() && GetCharacterMovement()->IsComponentTickEnabled()) == bMoveWasTicking, TEXT("the movement isn't ticking as it was"));
	// Gone leaves the parts as the tier before it had them, which for one found at Gone isn't known: only ticks compare there.
	if (!(bWasApplied && WasTier == 3))
	{
		Check(FMath::IsNearlyEqual(PrimaryActorTick.TickInterval, WasInterval), TEXT("the actor's tick interval isn't as it was"));
		const TArray<FString> Changed = PartsChanged(Before, SnapParts(this));
		Check(Changed.Num() == 0, FString::Printf(TEXT("not as before: %s"), *FString::Join(Changed, TEXT("; "))));
	}

	// Talking, the look, the face, the seat and the work: each set, read back and put back.
	const float WasTalk = TalkLeft;
	const float WasLoud = TalkLoudness;
	SetTalking(2.0f, 0.8f);
	Check(IsTalking() && FMath::IsNearlyEqual(TalkLoudness, 0.8f), TEXT("SetTalking didn't take"));
	SetTalking(0.0f, 0.6f);
	Check(!IsTalking(), TEXT("SetTalking(0) still talking"));
	TalkLeft = WasTalk;
	TalkLoudness = WasLoud;

	const FVector WasAt = LookTargetAt;
	const float WasWeight = LookTargetWeight;
	SetLookTarget(GetActorLocation() + GetActorForwardVector() * 200.0, 1.7f);
	Check(FMath::IsNearlyEqual(LookTargetWeight, 1.0f), TEXT("SetLookTarget's weight isn't clamped to 1"));
	LookTargetAt = WasAt;
	LookTargetWeight = WasWeight;

	const TMap<FName, float> WasCurves = FaceCurves;
	SetFaceCurve(TEXT("browRaiseInL"), 0.5f);
	Check(FaceCurves.Contains(TEXT("CTRL_expressions_browRaiseInL")), TEXT("SetFaceCurve didn't hold the control"));
	SetFaceCurve(TEXT("browRaiseInL"), 0.0f);
	Check(!FaceCurves.Contains(TEXT("CTRL_expressions_browRaiseInL")) || WasCurves.Contains(TEXT("CTRL_expressions_browRaiseInL")), TEXT("SetFaceCurve(0) didn't let go"));
	FaceCurves = WasCurves;

	if (!bSeated)
	{
		const UCharacterMovementComponent* Move = GetCharacterMovement();
		const EMovementMode WasMode = Move ? Move->MovementMode.GetValue() : MOVE_None;
		const float WasSeat = SeatHeightCm;
		SetSeated(true, 55.0f);
		Check(IsSeated() && FMath::IsNearlyEqual(GetSeatHeight(), 55.0f), TEXT("SetSeated didn't take"));
		Check(!Move || Move->MovementMode == MOVE_None, TEXT("seated, still free to move"));
		SetSeated(false, WasSeat);
		Check(!IsSeated() && (!Move || Move->MovementMode == WasMode), TEXT("standing, the movement isn't as it was"));
	}

	const FName WasBeat = WorkBeat;
	const FTransform WasStation = WorkStation;
	const float WasSince = WorkSince;
	SetWork(TEXT("selftest"), GetActorTransform());
	Check(GetWorkBeat() == FName(TEXT("selftest")) && GetWorkTime() >= 0.0f, TEXT("SetWork didn't take"));
	SetWork(NAME_None, FTransform::Identity);
	Check(GetWorkTime() == 0.0f, TEXT("SetWork(None) still working"));
	WorkBeat = WasBeat;
	WorkStation = WasStation;
	WorkSince = WasSince;

	// The grooms: every groom component worn is known, with its kind; the scalp's among them.
	TInlineComponentArray<UGroomComponent*> Worn(this);
	Check(GetGrooms().Num() == Worn.Num(), FString::Printf(TEXT("%d grooms worn, %d known"), Worn.Num(), GetGrooms().Num()));
	Check(!ScalpGroom() || GetGrooms().Contains(ScalpGroom()), TEXT("the scalp groom isn't among the grooms"));
	Check(StreetBody() != nullptr || !GetMesh()->GetSkeletalMeshAsset(), TEXT("no UStreetBodyAnim on the body"));

	return Fails.Num() == 0 ? FString::Printf(TEXT("%s selftest PASS (%d checks, %d parts, tiers 0-3 and back)"), *GetName(), Checks, Before.Num())
							: FString::Printf(TEXT("%s selftest FAIL (%d of %d checks): %s"), *GetName(), Fails.Num(), Checks, *FString::Join(Fails, TEXT(" | ")));
}

FString AShortStackCharacter::TestFaces(const FString& CareerKey)
{
	UCastFaceRegistry* Registry = UCastFaceRegistry::Get(this);
	return Registry ? Registry->Show(GetCastFace(), CareerKey) : FString(TEXT("ss.Look.Faces: no face registry here (it runs in game and PIE worlds)"));
}

FString AShortStackCharacter::TestLookPreview(const FString& Command)
{
	TArray<FString> A;
	Command.ParseIntoArrayWS(A);
	const FString Verb = A.Num() > 0 ? A[0].ToLower() : FString(TEXT("describe"));
	if (Verb == TEXT("end"))
	{
		EndPreview();
		return DescribeLook();
	}
	if (Verb == TEXT("keep"))
	{
		if (bPreviewing)
		{
			RefreshLook(LookNow, ClosetNow);
		}
		return DescribeLook();
	}
	if (Verb == TEXT("redress"))
	{
		// Everything worn put on again in place (after ss.Look.NpcGarments, an import, the ss.Wear.Graphic* tunables), and what it
		// can wear looked for again.
		bDrawableKnown = false;
		DressOutfit(CurrentOutfit());
		ApplyGrooms(false);
		RefreshVisibility();
		if (bTierApplied)
		{
			ApplyDetailTier();
		}
		return DescribeLook();
	}
	if (Verb == TEXT("preload"))
	{
		PreloadGroomSet(!(A.IsValidIndex(1) && A[1] == TEXT("0")));
		return HeroGrooms::Describe(CastFaceName);
	}
	if (Verb == TEXT("grooms"))
	{
		return HeroGrooms::Describe(CastFaceName);
	}
	if (Verb == TEXT("caps"))
	{
		const ss::ui::HostCaps Caps = CastLooks::HostCapsFor(this);
		auto Bits = [](const std::vector<bool>& V) {
			FString S;
			for (const bool B : V)
			{
				S += B ? TEXT("1") : TEXT("0");
			}
			return S;
		};
		return FString::Printf(TEXT("HostCaps for %s: hair %s, facial %s, brows %s, garments 0x%llx, gear 0x%llx, curios %u, 3D %d"), *CastFaceName.ToString(), *Bits(Caps.HairStyles),
			*Bits(Caps.FacialHair), *Bits(Caps.Brows), static_cast<unsigned long long>(Caps.GarmentKinds), static_cast<unsigned long long>(Caps.GearKinds), Caps.Curios,
			Caps.Preview3D ? 1 : 0);
	}
	if (Verb == TEXT("describe") || A.Num() < 2)
	{
		return DescribeLook();
	}
	if (bNpc)
	{
		return TEXT("ss.Look.Preview: the hero's look only");
	}
	// One choice changed on what's on now (a preview, as the shops ask for it).
	ss::hero::Look L = LookNow;
	ss::wardrobe::Closet C = ClosetNow;
	const FString& Value = A[1];
	const bool bNumber = Value.IsNumeric();
	const int32 N = FCString::Atoi(*Value);
	// The body's (body type, skin band, build, height: another hero face or another fit) rebuild, as a body change does: the
	// garments' clip check across every hero and build (spec 10) runs through them.
	static const TPair<const TCHAR*, ss::hero::Slot> LookSlots[] = {{TEXT("hair"), ss::hero::Slot::Hair}, {TEXT("facialhair"), ss::hero::Slot::FacialHair},
		{TEXT("brows"), ss::hero::Slot::Brows}, {TEXT("haircolor"), ss::hero::Slot::HairColor}, {TEXT("hat"), ss::hero::Slot::Hat}, {TEXT("glasses"), ss::hero::Slot::Glasses},
		{TEXT("outfit"), ss::hero::Slot::Outfit}, {TEXT("outfitcolor"), ss::hero::Slot::OutfitColor}, {TEXT("body"), ss::hero::Slot::Body}, {TEXT("skin"), ss::hero::Slot::Skin},
		{TEXT("build"), ss::hero::Slot::Build}, {TEXT("height"), ss::hero::Slot::Height}};
	for (const TPair<const TCHAR*, ss::hero::Slot>& S : LookSlots)
	{
		if (Verb == S.Key)
		{
			if (!bNumber)
			{
				return FString::Printf(TEXT("ss.Look.Preview %s takes the creator's option index"), S.Key);
			}
			L.Set(S.Value, N);
			PreviewLook(L, C);
			return DescribeLook();
		}
	}
	static const TPair<const TCHAR*, ss::wardrobe::Slot> ClosetSlots[] = {{TEXT("top"), ss::wardrobe::Slot::Top}, {TEXT("outer"), ss::wardrobe::Slot::Outer},
		{TEXT("bottom"), ss::wardrobe::Slot::Bottom}, {TEXT("shoes"), ss::wardrobe::Slot::Shoes}, {TEXT("headwear"), ss::wardrobe::Slot::Hat}, {TEXT("eyes"), ss::wardrobe::Slot::Eyes}};
	for (const TPair<const TCHAR*, ss::wardrobe::Slot>& S : ClosetSlots)
	{
		if (Verb == S.Key)
		{
			if (!C.Seeded)
			{
				// No closet yet (a career before it): the creator's choices, as worn now (core's seeding, or read off the look
				// while the catalog isn't in).
				C = ss::wardrobe::SeedFrom(L);
				if (!C.Seeded)
				{
					auto Name = [](const TCHAR* Slot, int32 CutIndex, int32 Color) {
						static const TCHAR* const Cuts[] = {TEXT("tee"), TEXT("hoodie"), TEXT("bomber"), TEXT("flannel"), TEXT("denim"), TEXT("leather"), TEXT("track")};
						return std::string(TCHAR_TO_UTF8(*FString::Printf(TEXT("%s.%s.%s"), Slot, Cuts[FMath::Clamp(CutIndex, 0, 6)],
							*FString(UTF8_TO_TCHAR(ss::hero::OptionName(ss::hero::Slot::OutfitColor, Color).c_str())).ToLower())));
					};
					C.Seeded = true;
					C.Worn[static_cast<int32>(ss::wardrobe::Slot::Top)] = Name(TEXT("top"), 0, L.OutfitColor);
					if (L.Outfit >= 0 && L.Outfit < ss::hero::OutfitNone)
					{
						C.Worn[static_cast<int32>(ss::wardrobe::Slot::Outer)] = Name(TEXT("outer"), L.Outfit + 1, L.OutfitColor);
					}
					C.Worn[static_cast<int32>(ss::wardrobe::Slot::Bottom)] = "bottom.jeans.mid";
					static const TCHAR* const HatIds[5] = {nullptr, TEXT("hat.beanie"), TEXT("hat.ballcap"), TEXT("hat.capback"), TEXT("hat.bucket")};
					static const TCHAR* const EyeIds[5] = {nullptr, TEXT("eyes.round"), TEXT("eyes.square"), TEXT("eyes.wire"), TEXT("eyes.shades")};
					if (const TCHAR* Hat = HatIds[FMath::Clamp(L.Hat, 0, 4)])
					{
						C.Worn[static_cast<int32>(ss::wardrobe::Slot::Hat)] = TCHAR_TO_UTF8(Hat);
					}
					if (const TCHAR* Eyes = EyeIds[FMath::Clamp(L.Glasses, 0, 4)])
					{
						C.Worn[static_cast<int32>(ss::wardrobe::Slot::Eyes)] = TCHAR_TO_UTF8(Eyes);
					}
				}
			}
			// A garment id, or a cut's name ("hoodie" reads as "outer.hoodie.<color>" in the outfit color), or none.
			std::string Id = TCHAR_TO_UTF8(*Value);
			if (Value.Equals(TEXT("none"), ESearchCase::IgnoreCase))
			{
				Id.clear();
			}
			else if (!Value.Contains(TEXT(".")))
			{
				// The catalog's garment of that cut (in the outfit color where it comes in colors), else one read off the id.
				ss::wardrobe::Garment Want;
				const bool bCut = GarmentOf(std::string(TCHAR_TO_UTF8(*FString::Printf(TEXT("x.%s"), *Value.ToLower()))), Want);
				const ss::wardrobe::Garment* Best = nullptr;
				for (const ss::wardrobe::Garment& G : ss::wardrobe::Catalog())
				{
					if (bCut && G.Kind == Want.Kind && G.Condition == 0 && (!Best || (Best->Color != L.OutfitColor && G.Color == L.OutfitColor)))
					{
						Best = &G;
					}
				}
				Id = Best ? Best->Id
						  : std::string(TCHAR_TO_UTF8(*FString::Printf(TEXT("%s.%s.%s"), S.Key, *Value.ToLower(),
								*FString(UTF8_TO_TCHAR(ss::hero::OptionName(ss::hero::Slot::OutfitColor, L.OutfitColor).c_str())).ToLower())));
			}
			C.Worn[static_cast<int32>(S.Value)] = Id;
			PreviewLook(L, C);
			return DescribeLook();
		}
	}
	return FString::Printf(TEXT("ss.Look.Preview: no slot '%s' (hair, facialhair, brows, haircolor, hat, glasses, outfit, outfitcolor, body, skin, build, height; top, outer, bottom, shoes, headwear, eyes)"), *Verb);
}

FString AShortStackCharacter::DescribeHooks() const
{
	static const TCHAR* const Tiers[4] = {TEXT("Near"), TEXT("Mid"), TEXT("Far"), TEXT("Gone")};
	const UGroomComponent* Scalp = ScalpGroom();
	const FString Look = LookTargetWeight > 0.0f ? FString::Printf(TEXT("at (%.0f, %.0f, %.0f) weight %.2f"), LookTargetAt.X, LookTargetAt.Y, LookTargetAt.Z, LookTargetWeight)
												 : FString(TEXT("default"));
	const FString Seat = bSeated ? FString::Printf(TEXT("seated at %.0f cm (blend %.2f)"), SeatHeightCm, SeatBlend) : FString(TEXT("standing"));
	const FString Work = WorkBeat.IsNone() ? FString(TEXT("not working")) : FString::Printf(TEXT("working '%s' for %.1f s"), *WorkBeat.ToString(), GetWorkTime());
	return FString::Printf(TEXT("%s (%s): tier %s%s | look %s | talking %.1f s more at %.2f | %d face controls held | %s | %s | scalp groom %s on LOD %d"), *GetName(),
		*CastFaceName.ToString(), Tiers[FMath::Clamp(DetailTierNow, 0, 3)], bTierApplied ? TEXT("") : TEXT(" (none set: as before the hooks)"), *Look, TalkLeft,
		TalkLoudness, FaceCurves.Num(), *Seat, *Work, Scalp ? *GetNameSafe(Scalp->GroomAsset.Get()) : TEXT("none"), Scalp ? Scalp->GetForcedLOD() : -1);
}

namespace ShortStackCharacterDetail
{
FAutoConsoleCommandWithWorldAndArgs CmdLookHook(TEXT("ss.Look.Hook"),
	TEXT("ss.Look.Hook <hero|near|all|ExtraB|a name> <tier 0-3 | talk seconds [loudness] | look X Y Z [weight] | lookhero [weight] | face Control value | sit [cm] | stand | work beat|none | describe | selftest>: "
		 "drives the people hooks (spec 7.5.4) on the hero or the street character nearest them (near: anyone; all: everyone, the hero too; else by face or actor name). "
		 "'ss.Look.Hook all selftest' checks every hook on everyone and leaves them as they were."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		if (!World || Args.Num() == 0)
		{
			return;
		}
		TArray<FString> Rest;
		for (int32 I = 1; I < Args.Num(); ++I)
		{
			Rest.Add(Args[I]);
		}
		const FString Command = FString::Join(Rest, TEXT(" "));
		if (Args[0].Equals(TEXT("all"), ESearchCase::IgnoreCase))
		{
			int32 Count = 0;
			for (TActorIterator<AShortStackCharacter> It(World); It; ++It)
			{
				UE_LOG(LogStreetHero, Display, TEXT("%s"), *It->TestHook(Command));
				++Count;
			}
			UE_LOG(LogStreetHero, Display, TEXT("ss.Look.Hook all: %d characters"), Count);
			return;
		}
		APlayerController* Pc = World->GetFirstPlayerController();
		AShortStackCharacter* Hero = Pc ? Cast<AShortStackCharacter>(Pc->GetPawn()) : nullptr;
		AShortStackCharacter* Who = nullptr;
		if (Args[0].Equals(TEXT("hero"), ESearchCase::IgnoreCase))
		{
			Who = Hero;
		}
		else
		{
			double Best = TNumericLimits<double>::Max();
			for (TActorIterator<AShortStackCharacter> It(World); It; ++It)
			{
				AShortStackCharacter* C = *It;
				const bool bNamed = Args[0].Equals(TEXT("near"), ESearchCase::IgnoreCase) || C->GetCastFace().ToString().Equals(Args[0], ESearchCase::IgnoreCase)
					|| C->GetName().Contains(Args[0]);
				if (C == Hero || !bNamed)
				{
					continue;
				}
				const double Dist = Hero ? FVector::DistSquared(Hero->GetActorLocation(), C->GetActorLocation()) : 0.0;
				if (Dist < Best)
				{
					Best = Dist;
					Who = C;
				}
			}
		}
		if (!Who)
		{
			UE_LOG(LogStreetHero, Display, TEXT("ss.Look.Hook: nobody here answers to '%s'"), *Args[0]);
			return;
		}
		UE_LOG(LogStreetHero, Display, TEXT("%s"), *Who->TestHook(Command));
	}));

AShortStackCharacter* PlayerCharacter(UWorld* World)
{
	APlayerController* Pc = World ? World->GetFirstPlayerController() : nullptr;
	return Pc ? Cast<AShortStackCharacter>(Pc->GetPawn()) : nullptr;
}

FAutoConsoleCommandWithWorldAndArgs CmdLookPreview(TEXT("ss.Look.Preview"),
	TEXT("ss.Look.Preview <slot> <value> | end | keep | redress | preload [0] | grooms | caps | describe: previews the hero's look with one choice changed "
		 "(hair, facialhair, brows, haircolor, hat, glasses, outfit, outfitcolor, body, skin, build, height: the creator's option index; top, outer, bottom, shoes, headwear, eyes: a garment id "
		 "or a cut's name, \"outer hoodie\", \"bottom chinos\", \"headwear trilby\", \"eyes none\"), in place (spec 7.5.4). AShortStackCharacter::TestLookPreview is its twin."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		AShortStackCharacter* Hero = PlayerCharacter(World);
		if (!Hero)
		{
			UE_LOG(LogStreetHero, Display, TEXT("ss.Look.Preview: no hero here"));
			return;
		}
		UE_LOG(LogStreetHero, Display, TEXT("%s"), *Hero->TestLookPreview(FString::Join(Args, TEXT(" "))));
	}));

FAutoConsoleCommandWithWorld CmdLookRedress(TEXT("ss.Look.Redress"),
	TEXT("Dresses every street character again in place in what they wear (after ss.Look.NpcGarments, an import of garments or wear, ss.Wear.Graphic*)."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World) {
		if (!World)
		{
			return;
		}
		int32 Count = 0;
		for (TActorIterator<AShortStackCharacter> It(World); It; ++It)
		{
			It->TestLookPreview(TEXT("redress"));
			++Count;
		}
		UE_LOG(LogStreetHero, Display, TEXT("ss.Look.Redress: %d characters"), Count);
	}));

FAutoConsoleCommandWithWorldAndArgs CmdLookGrooms(TEXT("ss.Look.Grooms"),
	TEXT("ss.Look.Grooms [HeroA0..HeroB2]: a hero face's groom library (HeroGrooms.h): its styles by kind, what the spec's map asks for that it lacks, whether it's preloaded."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) {
		const AShortStackCharacter* Hero = PlayerCharacter(World);
		const FName Face = Args.Num() > 0 ? FName(*Args[0]) : (Hero ? Hero->GetCastFace() : FName());
		if (Face.IsNone())
		{
			for (const TCHAR* Each : {TEXT("HeroA0"), TEXT("HeroA1"), TEXT("HeroA2"), TEXT("HeroB0"), TEXT("HeroB1"), TEXT("HeroB2")})
			{
				UE_LOG(LogStreetHero, Display, TEXT("%s"), *HeroGrooms::Describe(FName(Each)));
			}
			return;
		}
		UE_LOG(LogStreetHero, Display, TEXT("%s"), *HeroGrooms::Describe(Face));
	}));
} // namespace ShortStackCharacterDetail
