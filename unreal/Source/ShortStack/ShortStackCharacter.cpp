#include "ShortStackCharacter.h"

#include "BackRoomAnim.h"
#include "BlenderProps.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GroomAsset.h"
#include "GroomComponent.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshDescription.h"
#include "Misc/PackageName.h"
#include "StaticMeshAttributes.h"
#include "StreetAnim.h"

DEFINE_LOG_CATEGORY_STATIC(LogStreetHero, Log, All);

/** What a body wears below the head: the shirt (it stands in for the jacket), the shorts, a print on the shirt, the shoes. */
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
	/** The sneakers: the canvas, the rubber (sole and toe cap), the accents (heel tab, stripe), the opening's lining, laces, the tread. */
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

// The creator's headwear (1 beanie, 2 cap, 3 cap backwards, 4 bucket hat) and glasses (1 round, 2 square, 3 wire, 4 shades).
const TCHAR* const Hats[5] = {nullptr, TEXT("SM_Hat_Beanie"), TEXT("SM_Hat_Cap"), TEXT("SM_Hat_Cap"), TEXT("SM_Hat_Bucket")};
const TCHAR* const Specs[5] = {nullptr, TEXT("SM_Glasses_Round"), TEXT("SM_Glasses_Square"), TEXT("SM_Glasses_Wire"), TEXT("SM_Glasses_Shades")};

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

enum class EGroomKind : uint8
{
	Scalp,
	Brows,
	Lashes,
	Beard,
	Mustache,
	Fuzz,
};

/** Which groom a MetaHuman Blueprint's node holds, by its variable (Hair, Eyebrows, Beard, ...) or its asset (Beard_S_Stubble). */
EGroomKind GroomKindOf(const FString& Variable, const FString& Asset)
{
	auto Is = [&](const TCHAR* Word) { return Variable.Contains(Word) || Asset.StartsWith(Word); };
	if (Is(TEXT("Eyelash")))
	{
		return EGroomKind::Lashes;
	}
	if (Is(TEXT("Eyebrow")))
	{
		return EGroomKind::Brows;
	}
	if (Is(TEXT("Mustache")))
	{
		return EGroomKind::Mustache;
	}
	if (Is(TEXT("Beard")) || Is(TEXT("Goatee")))
	{
		return EGroomKind::Beard;
	}
	if (Is(TEXT("Fuzz")) || Is(TEXT("Peachfuzz")))
	{
		return EGroomKind::Fuzz;
	}
	return EGroomKind::Scalp;
}

/** Hair that stays close to the scalp, so a hat sits over it without it coming up through the crown. */
bool TightHair(const FString& Asset)
{
	static const TCHAR* const Tight[] = {TEXT("BuzzCut"), TEXT("360Waves"), TEXT("BaldingStubble"), TEXT("HairLoss"), TEXT("Cornrows"), TEXT("SlickBack"),
		TEXT("PulledBack"), TEXT("LowPonytail"), TEXT("RecedeMessy"), TEXT("BrushCut"), TEXT("CurlyFade")};
	for (const TCHAR* Style : Tight)
	{
		if (Asset.Contains(Style))
		{
			return true;
		}
	}
	return false;
}

/** Hair cropped about as short as a buzz cut (a ponytail or cornrows lie close, but aren't one). */
bool CroppedHair(const FString& Asset)
{
	static const TCHAR* const Cropped[] = {TEXT("BuzzCut"), TEXT("360Waves"), TEXT("BaldingStubble"), TEXT("HairLoss"), TEXT("BrushCut"), TEXT("CurlyFade")};
	for (const TCHAR* Style : Cropped)
	{
		if (Asset.Contains(Style))
		{
			return true;
		}
	}
	return false;
}

/**
 * Whether the hero wears one of the MetaHuman's grooms, given the creator's choices. The preset's hairstyle stands in
 * for the one picked (hair can't be swapped at run time), except: shaved is bald, a buzz cut is bald unless the preset's
 * hair is that short, and under a hat only hair that lies close to the head stays. Facial hair: none takes the beard
 * and mustache off, stubble keeps only stubble, a mustache keeps only the mustache.
 */
bool HeroWearsGroom(const ss::hero::Look& L, EGroomKind Kind, const FString& Asset)
{
	switch (Kind)
	{
	case EGroomKind::Scalp:
		if (L.Hair == 0 || (L.Hair == 1 && !CroppedHair(Asset)))
		{
			return false;
		}
		return L.Hat == 0 || TightHair(Asset);
	case EGroomKind::Beard:
		return L.FacialHair >= 3 || (L.FacialHair == 1 && Asset.Contains(TEXT("Stubble")));
	case EGroomKind::Mustache:
		return L.FacialHair >= 2 || (L.FacialHair == 1 && Asset.Contains(TEXT("Stubble")));
	default:
		return true;
	}
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
			UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Material, Groom);
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
 * cap of hair). Builds below the cinematic tier carry cards, not strands: hold them on their nearest cards.
 */
void HoldOnCards(UGroomComponent* Groom)
{
	if (!Groom || !Groom->GroomAsset)
	{
		return;
	}
	int32 CardsLOD = INDEX_NONE;
	for (const FHairGroupsCardsSourceDescription& Cards : Groom->GroomAsset->GetHairGroupsCards())
	{
		if (Cards.ImportedMesh && Cards.LODIndex >= 0)
		{
			CardsLOD = CardsLOD == INDEX_NONE ? Cards.LODIndex : FMath::Min(CardsLOD, Cards.LODIndex);
		}
	}
	if (CardsLOD != INDEX_NONE)
	{
		Groom->SetForcedLOD(CardsLOD);
	}
}

// ------------------------------------------------------------------ clothes

/** The hero's clothes from the creator: the jacket's color on the shirt, shorts that go with it, the sneakers. */
FStreetOutfit HeroOutfit(const ss::hero::Look& L)
{
	FStreetOutfit O;
	const int32 Color = FMath::Clamp(L.OutfitColor, 0, 7);
	O.Shirt = SrgbOf(ss::hero::OutfitTone(Color));
	// Charcoal, navy, olive, burgundy, sand, black, teal, mustard: denim, charcoal, dark brown, black, denim, grey, charcoal, black.
	static const uint32 PantsFor[8] = {0x283244, 0x2b2c31, 0x3a3128, 0x1d1d21, 0x283447, 0x3b3d43, 0x2b2c31, 0x1d1d21};
	O.Pants = SrgbOf(PantsFor[Color]);
	if (L.Outfit == 2)
	{
		// Flannel: the jacket's color in a plaid (the wide bands darker, a pale thread, darkest where the bands cross).
		O.Print = TEXT("T_Print_Tartan");
		O.PrintTiling = 3.0f;
		O.PrintA = Scaled(O.Shirt, 0.42f);
		O.PrintB = FMath::Lerp(O.Shirt, SrgbOf(0xd9cfb8), 0.7f);
		O.PrintC = Scaled(O.Shirt, 0.2f);
	}
	// Canvas court shoes, off-white with the jacket's color on the heel tab and the stripe (a red one when the jacket is
	// charcoal or black); black canvas under a light jacket.
	const bool bLightJacket = Color == 4 || Color == 7;
	O.ShoeUpper = SrgbOf(bLightJacket ? 0x1d1e22 : 0xd6d0c0);
	O.ShoeRubber = SrgbOf(bLightJacket ? 0xe4dfd2 : 0xece7db);
	O.ShoeAccent = Color == 0 || Color == 5 ? SrgbOf(0x8f1d2c) : O.Shirt;
	O.ShoeLining = SrgbOf(0x1a1a1d);
	O.ShoeLaces = SrgbOf(bLightJacket ? 0x2a2b30 : 0xefebe1);
	O.ShoeOutsole = SrgbOf(0x3b3029);
	return O;
}

/** A cast member at work: their shirt, dark trousers, black shoes. */
FStreetOutfit CastOutfit(const FLinearColor& Shirt)
{
	FStreetOutfit O;
	O.Shirt = Shirt;
	O.Pants = SrgbOf(0x2a2b30);
	O.ShoeUpper = SrgbOf(0x161619);
	O.ShoeRubber = SrgbOf(0x232327);
	O.ShoeAccent = SrgbOf(0x161619);
	O.ShoeLining = SrgbOf(0x111113);
	O.ShoeLaces = SrgbOf(0x161619);
	O.ShoeOutsole = SrgbOf(0x101012);
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
	Mid->SetScalarParameterValue(TEXT("PrintGraphicStrength"), 0.0f);
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
 * Builds a sneaker for one foot: a rubber sole (tread, midsole, a lip where the upper sits in it) and a canvas upper
 * lofted over it, closing along its top line, with a rubber toe cap and foxing, a heel tab and side stripe, laces and
 * the dark of the opening. Vertices are in the foot bone's position with the body's axes; the component undoes the
 * bone's rest rotation when it attaches, so the shoe follows the foot.
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
	const bool bCards = Path.Contains(TEXT("/MHC_Extra")) || Path.Contains(TEXT("/MHC_HeroA")) || Path.Contains(TEXT("/MHC_HeroB"));
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
			if (!bNpc && !HeroWearsGroom(LookNow, Kind, Asset))
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
			// Clothes ride the body's pose: the shirt in the jacket's color (and print), everything else the shorts' tone.
			Skinned->SetLeaderPoseComponent(GetMesh());
			for (int32 Slot = 0; Slot < Skinned->GetNumMaterials(); ++Slot)
			{
				if (UMaterialInterface* Material = Skinned->GetMaterial(Slot))
				{
					UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Material, Skinned);
					DressGarment(Mid, IsShirtMaterial(Material), Outfit);
					Skinned->SetMaterial(Slot, Mid);
				}
			}
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

void AShortStackCharacter::ApplyLook(const ss::hero::Character& Who)
{
	LookNow = Who.Appearance;
	LookAge = Who.Age;
	bHasLook = true;
	bNpc = false;
	const ss::hero::Look& L = LookNow;
	const FStreetOutfit Outfit = HeroOutfit(L);
	// The hero MetaHuman nearest the look: body type, then skin tone in three bands.
	const int32 Band = L.Skin <= 2 ? 0 : (L.Skin <= 5 ? 1 : 2);
	FString Built = FString::Printf(TEXT("Hero%c%d"), L.Body == 1 ? TEXT('B') : TEXT('A'), Band);
	UClass* Blueprint = CastClass(Built);
	if (!Blueprint)
	{
		Built = TEXT("Hero");
		Blueprint = CastClass(Built);
	}
	if (Blueprint)
	{
		BuildFromMetaHuman(Blueprint, Outfit);
	}
	else
	{
		Built = TEXT("the archetype body");
		UseArchetype();
	}
	LinkFace();
	FitToHeight(static_cast<float>(L.Height));
	FitFace();
	BuildShoes(Outfit);
	PutOnHeadwear();
	RefreshVisibility();
	UE_LOG(LogStreetHero, Log, TEXT("%s (%d) walks as %s: body %d, skin %d, hair %d in color %d, facial hair %d, build %d, %d cm, jacket %d in color %d, glasses %d, hat %d"),
		UTF8_TO_TCHAR(Who.FullName().c_str()), Who.Age, *Built, L.Body, L.Skin, L.Hair, L.HairColor, L.FacialHair, L.Build, L.Height, L.Outfit, L.OutfitColor, L.Glasses, L.Hat);
}

void AShortStackCharacter::ApplyCast(const TCHAR* CastName, const FLinearColor& Shirt)
{
	bHasLook = true;
	bNpc = true;
	LookNow = ss::hero::Look();
	LookAge = 0;
	const FStreetOutfit Outfit = CastOutfit(Shirt);
	if (UClass* Blueprint = CastClass(CastName))
	{
		BuildFromMetaHuman(Blueprint, Outfit);
	}
	else
	{
		UE_LOG(LogStreetHero, Warning, TEXT("%s isn't built (backroom_cast.py): the archetype body stands in"), CastName);
		UseArchetype();
	}
	LinkFace();
	FitToHeight(175.0f);
	FitFace();
	BuildShoes(Outfit);
	PutOnHeadwear();
	BecomeNpc();
	RefreshVisibility();
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
	const float Width = LookNow.Build == 0 ? 0.95f : LookNow.Build == 2 ? 1.04f : LookNow.Build == 3 ? 1.12f : 1.0f;
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
		// The crown (the hat's origin is the top of the head inside it) on the top of the face mesh, which is the scalp,
		// with a few millimetres for close hair. Sized from the eyes' spacing: a MetaHuman face mesh's bounds take in the
		// neck and the shoulders' tops, so they can't say how wide the head is. Fitted by eye on the hero's face (eyes 6.8 cm
		// apart), where the hats (made for a 15.6 cm head) want 15% more to cover the skull, front to back; a bigger hat
		// grows down from its crown, so it rides up by as much to keep the cuff above the brows.
		float Crown = static_cast<float>(Bounds.Origin.Z + Bounds.BoxExtent.Z - Head.Z) + 0.4f;
		if (Crown < 12.0f || Crown > 26.0f)
		{
			Crown = 19.0f;
		}
		const float Size = FMath::Clamp(1.15f * (bEyes ? Apart / 6.79f : 1.0f), 1.0f, 1.3f);
		const FVector At = Head + Up * Tuned(CVarHatUp, Crown + (Size - 1.0f) * 10.0f) + Ahead * Tuned(CVarHatForward, 0.4f);
		return FTransform(BlenderFacing(FaceYaw + Turn).Quaternion(), At, FVector(Size));
	}
	// The glasses' origin is the bridge of the nose, the lenses' plane: before the eyes' centers (the eyeball's radius and
	// the lenses' distance from it), between them; sized to how far apart they are. The eye bones sit above and ahead of
	// where the pupils show (fitted by eye on the hero's face), hence the 2 cm down and the 1.7 cm forward, not 2.4.
	const FVector FromHead = Eyes - Head;
	const float Rise = static_cast<float>(FromHead.Z) - (bEyes ? 2.0f : 0.0f);
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

void AShortStackCharacter::PutOnHeadwear()
{
	const ss::hero::Look& L = LookNow;
	const int32 Hat = bNpc ? 0 : FMath::Clamp(L.Hat, 0, 4);
	const int32 Glasses = bNpc ? 0 : FMath::Clamp(L.Glasses, 0, 4);
	// Fitted to the mesh that will carry them (WearOnHead), so a face without a head bone can't put them on the floor.
	const USkeletalMeshComponent* Carrier = HeadCarrier();
	const USkeletalMesh* HeadAsset = Carrier ? Carrier->GetSkeletalMeshAsset() : nullptr;
	auto Ready = [this](TObjectPtr<UStaticMeshComponent>& Slot, const TCHAR* MeshName, const TCHAR* What) -> UStaticMeshComponent* {
		UStaticMesh* Mesh = Wearable(MeshName);
		if (MeshName && !Mesh)
		{
			UE_LOG(LogStreetHero, Warning, TEXT("%s %s isn't imported (shortstack_setup.import_meshes)"), What, MeshName);
		}
		if (!Mesh)
		{
			if (Slot)
			{
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
	if (UStaticMeshComponent* C = Ready(HatMesh, Hats[Hat], TEXT("Hat")))
	{
		// The fabric in a color that goes with the jacket and stands apart from the hair at this age (as the creator's
		// portrait has it); the cap's top button is the same cloth, the bill's underside keeps its green.
		const FLinearColor Tint = SrgbOf(ss::hero::HatTone(L, LookAge));
		TintSlot(C, BakedSlot(C, 0), Tint, 1.45f);
		if (Hat == 2 || Hat == 3)
		{
			TintSlot(C, BakedSlot(C, 2), Tint, 1.45f);
		}
		FTransform Fit = FitHeadWear(HeadAsset, true, Hat == 3 ? 180.0f : 0.0f);
		if (bWearsScalpHair)
		{
			// Over close-cropped hair (the only kind kept under a hat): a little higher and roomier than on the bare scalp.
			Fit.AddToTranslation(FVector(0.0, 0.0, 0.8));
			Fit.SetScale3D(Fit.GetScale3D() * 1.03);
		}
		WearOnHead(C, Fit);
	}
	if (UStaticMeshComponent* C = Ready(GlassesMesh, Specs[Glasses], TEXT("Glasses")))
	{
		// Black acetate, or gold wire. The bake gives every lens a dark mirror: clear ones get a glass you see the eyes
		// through (Blender's second slot, after the frame), the shades keep theirs.
		TintSlot(C, BakedSlot(C, 0), SrgbOf(Glasses == 3 ? 0xc9a24d : 0x161618), Glasses == 3 ? 1.4f : 1.0f);
		if (Glasses != 4 && C->GetNumMaterials() > 1)
		{
			if (UMaterialInterface* Lens = LoadObject<UMaterialInterface>(nullptr, LensMaterial, nullptr, LOAD_NoWarn | LOAD_Quiet))
			{
				C->SetMaterial(BakedSlot(C, 1), Lens);
			}
		}
		WearOnHead(C, FitHeadWear(HeadAsset, false));
	}
}

void AShortStackCharacter::RefitWear()
{
	PutOnHeadwear();
	RefreshVisibility();
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

void AShortStackCharacter::BuildShoes(const FStreetOutfit& Outfit)
{
	USkeletalMesh* BodyAsset = GetMesh()->GetSkeletalMeshAsset();
	UMaterialInterface* Surface = LoadObject<UMaterialInterface>(nullptr, SurfaceMaterial, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Surface)
	{
		Surface = LoadObject<UMaterialInterface>(nullptr, BasicMaterial, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	if (!BodyAsset || !Surface)
	{
		for (UStaticMeshComponent* S : Shoes)
		{
			Conceal(S);
		}
		return;
	}
	const FReferenceSkeleton& Ref = BodyAsset->GetRefSkeleton();
	const bool bRefit = ShoesFitFor.Get() != BodyAsset;
	const FLinearColor Colors[Shoe::SlotCount] = {Outfit.ShoeUpper, Outfit.ShoeRubber, Outfit.ShoeAccent, Outfit.ShoeLining, Outfit.ShoeLaces, Outfit.ShoeOutsole};
	for (int32 Side = 0; Side < 2; ++Side)
	{
		Shoe::FFootFit Fit;
		if (!Shoe::FitFoot(Ref, Side, Fit))
		{
			Conceal(Shoes[Side].Get());
			continue;
		}
		if (bRefit || !ShoeMeshes[Side])
		{
			// Made once for each body: a couple of thousand triangles, fitted to its feet.
			ShoeMeshes[Side] = Shoe::BuildShoeMesh(this, Fit, Surface);
		}
		if (!ShoeMeshes[Side])
		{
			Conceal(Shoes[Side].Get());
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
		Shoes[Side]->SetStaticMesh(ShoeMeshes[Side]);
		// The mesh is built in the body's axes at the foot: undo the bone's rest rotation and the shoe sits as built, then follows the foot.
		Shoes[Side]->SetRelativeTransform(FTransform(Fit.FootRest.GetRotation().Inverse(), FVector::ZeroVector));
		for (int32 Slot = 0; Slot < Shoe::SlotCount; ++Slot)
		{
			Shoes[Side]->SetMaterial(Slot, PlainSurface(Surface, Shoes[Side].Get(), Colors[Slot], Shoe::SlotRoughness[Slot]));
		}
		Reveal(Shoes[Side].Get());
	}
	ShoesFitFor = BodyAsset;
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
	if (bNpc)
	{
		// Benny: the player, once they're a few steps away and in front of him; otherwise the counter, idly.
		WantYaw = 8.0f * FMath::Sin(Clock * 0.11f) + 4.0f * FMath::Sin(Clock * 0.37f);
		WantPitch = -6.0f;
		Rate = 3.0f;
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
	// Your own face in first person is there for its shadow: it follows the head, with nothing for RigLogic to work out.
	Anim->bExpressionless = bHeadHidden && !bNpc;
	if (Anim->bExpressionless)
	{
		Anim->LookAt = FVector::ZeroVector;
		return;
	}
	// This frame's controls, written over last frame's (the map keeps its storage).
	TMap<FName, float>& C = Anim->Curves;
	C.Reset();
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
		C.Add(CurveBlinkL, Shut);
		C.Add(CurveBlinkR, Shut);
		if (BlinkT >= 1.0f)
		{
			BlinkT = bDoubleBlink ? 0.0f : -1.0f;
			bDoubleBlink = false;
		}
	}
	// The lips just parted, open to breathe after a run, open for the can or the bite as the hand gets there.
	const float SipT = Clock - SipAt;
	const float Sipping = SipT >= 0.0f && SipT < 1.8f ? FMath::SmoothStep(0.3f, 0.6f, SipT) * (1.0f - FMath::SmoothStep(1.1f, 1.5f, SipT)) : 0.0f;
	C.Add(CurveJawOpen, 0.02f + 0.12f * Winded + 0.22f * Sipping);
	C.Add(CurveRelaxL, 0.1f);
	C.Add(CurveRelaxR, 0.1f);
	Anim->LookAt = EyeTarget.IsZero() ? FVector::ZeroVector : Face->GetComponentTransform().InverseTransformPosition(EyeTarget);
}

// ------------------------------------------------------------------ moving

void AShortStackCharacter::Drive(float Forward, float Right, bool bRun)
{
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
	const float Dt = FMath::Min(DeltaSeconds, 0.05f);
	Clock += Dt;
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
		P.LookPitch = LookPitchNow;
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
