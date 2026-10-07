#include "ShortStackMetaHumanLibrary.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Cloud/MetaHumanARServiceRequest.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "GroomAsset.h"
#include "GroomComponent.h"
#include "IMeshMergeUtilities.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshMerge/MeshMergingSettings.h"
#include "MeshMergeModule.h"
#include "StaticMeshAttributes.h"
#include "HAL/PlatformMemory.h"
#include "MetaHumanCharacter.h"
#include "MetaHumanCharacterEditorSubsystem.h"
#include "MetaHumanCharacterIdentity.h"
#include "MetaHumanFaceTextureSynthesizer.h"
#include "MetaHumanPaletteItemPath.h"
#include "MetaHumanPipelineSlotSelectionData.h"
#include "MetaHumanRigEvaluatedState.h"
#include "StructUtils/PropertyBag.h"
#include "UObject/UnrealType.h"
#include "MetaHumanCharacterInstance.h"
#include "MetaHumanCharacterPaletteProjectSettings.h"
#include "MetaHumanCollection.h"
#include "MetaHumanWardrobeItem.h"
#include "MetaHumanCollectionEditorPipeline.h"
#include "MetaHumanCollectionPipeline.h"
#include "MetaHumanTypes.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "Subsystem/MetaHumanCharacterBuild.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, ShortStackEditor);

DEFINE_LOG_CATEGORY_STATIC(LogShortStackCast, Log, All);

namespace ShortStackCastDetail
{
const TCHAR* PresetsPath = TEXT("/MetaHumanCharacter/Optional/Presets");

UMetaHumanCharacterEditorSubsystem* Subsystem()
{
	return UMetaHumanCharacterEditorSubsystem::Get();
}

/** The Creator works on characters it has opened for editing (their meshes, rig state and textures). */
bool EnsureEditing(UMetaHumanCharacter* Character)
{
	if (!Character || !Subsystem())
	{
		return false;
	}
	if (Subsystem()->IsObjectAddedForEditing(Character))
	{
		return true;
	}
	if (!Subsystem()->TryAddObjectToEdit(Character))
	{
		UE_LOG(LogShortStackCast, Error, TEXT("Could not open %s for editing"), *Character->GetPathName());
		return false;
	}
	return true;
}

using FFaceState = FMetaHumanCharacterIdentity::FState;

/** A preset by its name (the Creator's, "Grace"), or a character by its package path ("/Game/.../MHC_Ray"). */
UMetaHumanCharacter* LoadCharacter(const FString& NameOrPath)
{
	const FString Path = NameOrPath.StartsWith(TEXT("/")) ? NameOrPath : FString::Printf(TEXT("%s/%s"), PresetsPath, *NameOrPath);
	const FString AssetName = FPackageName::GetLongPackageAssetName(Path);
	return LoadObject<UMetaHumanCharacter>(nullptr, *(Path + TEXT(".") + AssetName), nullptr, LOAD_NoWarn | LOAD_Quiet);
}

/** Source's face in an open character's face model, read as the Creator's Blend tool reads its presets. */
TSharedPtr<FFaceState> FaceStateOf(UMetaHumanCharacter* Open, const UMetaHumanCharacter* Source)
{
	TSharedRef<FFaceState> State = Subsystem()->CopyFaceState(Open);
	if (!State->Deserialize(Source->GetFaceStateData()))
	{
		return nullptr;
	}
	return State;
}

/**
 * Commits an open character's face with its own variants on it: a blend mixes the presets' whole states, but the face
 * texture's fine wrinkles, the eyelashes and the teeth are the character's settings, put on as the Creator's skin and head
 * tools put them (the teeth without the tool's show-the-teeth expression). A rig is made from the face it was given, so a
 * face changed under one is said: the build keeps the rigged face until RemoveFaceRig and AutoRig.
 */
void CommitFace(UMetaHumanCharacter* Character)
{
	TSharedRef<FFaceState> State = Subsystem()->CopyFaceState(Character);
	Subsystem()->UpdateHFVariantFromSkinProperties(State, Character->SkinSettings.Skin);
	Subsystem()->UpdateEyelashesVariantFromProperties(State, Character->HeadModelSettings.Eyelashes);
	Subsystem()->UpdateTeethVariantFromProperties(State, Character->HeadModelSettings.Teeth, false);
	Subsystem()->CommitFaceState(Character, State);
	if (Character->HasFaceDNA())
	{
		UE_LOG(LogShortStackCast, Warning, TEXT("%s: the face changed under its rig; the build shows the rigged face until the rig is removed and made again"), *Character->GetName());
	}
}

/** A rig is made for one face: a changed face needs AutoRig again (the Creator's Remove Rig). */
void DropFaceRig(UMetaHumanCharacter* Character)
{
	if (Character->HasFaceDNA())
	{
		Subsystem()->RemoveFaceRig(Character);
		UE_LOG(LogShortStackCast, Display, TEXT("%s: face rig removed (the face changed; AutoRig makes a new one)"), *Character->GetName());
	}
}

/** The index of one of the face textures' attributes ("Face Wrinkles", "Face Stubble", "Face Marks"), or INDEX_NONE. */
int32 TextureAttribute(const FMetaHumanFaceTextureAttributeMap& Map, const TCHAR* Name)
{
	for (int32 Index = 0; Index < Map.NumAttributes(); ++Index)
	{
		if (Map.GetAttributeName(Index) == Name)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

/** Moves a face's vertices so that their mean is the origin (faces are compared by shape, not by where they sit). */
void CenterOnSelf(TArray<FVector3f>& Vertices)
{
	if (Vertices.Num() == 0)
	{
		return;
	}
	FVector3d Sum = FVector3d::ZeroVector;
	for (const FVector3f& Vertex : Vertices)
	{
		Sum += FVector3d(Vertex);
	}
	const FVector3f Mean(Sum / static_cast<double>(Vertices.Num()));
	for (FVector3f& Vertex : Vertices)
	{
		Vertex -= Mean;
	}
}

/** "Lorenzo 0.40 + Jorge 0.35 + Bruce 0.25", for the log. */
FString Recipe(const TArray<FString>& Presets, const TArray<float>& Weights)
{
	FString Out;
	for (int32 Index = 0; Index < Presets.Num() && Index < Weights.Num(); ++Index)
	{
		Out += FString::Printf(TEXT("%s%s %.2f"), Index ? TEXT(" + ") : TEXT(""), *Presets[Index], Weights[Index]);
	}
	return Out;
}
} // namespace ShortStackCastDetail

using namespace ShortStackCastDetail;

TArray<FString> UShortStackMetaHumanLibrary::ListPresets()
{
	TArray<FString> Names;
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	TArray<FAssetData> Assets;
	Registry.GetAssetsByPath(FName(PresetsPath), Assets, true);
	for (const FAssetData& Asset : Assets)
	{
		if (Asset.AssetClassPath == UMetaHumanCharacter::StaticClass()->GetClassPathName())
		{
			Names.Add(Asset.AssetName.ToString());
		}
	}
	Names.Sort();
	return Names;
}

UMetaHumanCharacter* UShortStackMetaHumanLibrary::CreateFromPreset(const FString& PackagePath, const FString& PresetName, bool bReapplyPreset)
{
	if (!Subsystem())
	{
		return nullptr;
	}
	const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
	UMetaHumanCharacter* Character = LoadObject<UMetaHumanCharacter>(nullptr, *(PackagePath + TEXT(".") + AssetName), nullptr, LOAD_NoWarn | LOAD_Quiet);
	bool bNew = false;
	if (!Character)
	{
		// As the Creator's "new MetaHuman Character" factory does.
		UPackage* Package = CreatePackage(*PackagePath);
		Character = NewObject<UMetaHumanCharacter>(Package, FName(*AssetName), RF_Public | RF_Standalone | RF_Transactional);
		Subsystem()->InitializeMetaHumanCharacter(Character);
		FAssetRegistryModule::AssetCreated(Character);
		bNew = true;
	}
	if (!EnsureEditing(Character))
	{
		return nullptr;
	}
	if (bNew || bReapplyPreset)
	{
		// A preset's name, or another character's package path (CreateFromBlend's first face may be either).
		UMetaHumanCharacter* Preset = LoadCharacter(PresetName);
		if (!Preset || Preset == Character)
		{
			UE_LOG(LogShortStackCast, Error, TEXT("No MetaHuman preset %s to start %s from"), *PresetName, *Character->GetName());
			return Character;
		}
		Subsystem()->InitializeFromPreset(Character, Preset);
		Character->MarkPackageDirty();
		UE_LOG(LogShortStackCast, Display, TEXT("%s from preset %s"), *Character->GetPathName(), *PresetName);
	}
	return Character;
}

bool UShortStackMetaHumanLibrary::AutoRig(UMetaHumanCharacter* Character, bool bBlendShapes)
{
	if (!EnsureEditing(Character))
	{
		return false;
	}
	if (Subsystem()->IsAutoRiggingFace(Character))
	{
		return true;
	}
	Subsystem()->AutoRigFace(Character, bBlendShapes ? UE::MetaHuman::ERigType::JointsAndBlendshapes : UE::MetaHuman::ERigType::JointsOnly);
	return true;
}

bool UShortStackMetaHumanLibrary::RequestTextures(UMetaHumanCharacter* Character, int32 Resolution)
{
	if (!EnsureEditing(Character))
	{
		return false;
	}
	if (Subsystem()->IsRequestingHighResolutionTextures(Character))
	{
		return true;
	}
	const ERequestTextureResolution Res = Resolution >= 8192 ? ERequestTextureResolution::Res8k
		: Resolution >= 4096 ? ERequestTextureResolution::Res4k : ERequestTextureResolution::Res2k;
	Subsystem()->RequestHighResolutionTextures(Character, Res);
	return true;
}

FString UShortStackMetaHumanLibrary::Status(UMetaHumanCharacter* Character)
{
	if (!Character || !Subsystem())
	{
		return TEXT("missing");
	}
	// What the asset knows needs no editing session; the cloud requests in flight do.
	const bool bOpen = Subsystem()->IsObjectAddedForEditing(Character);
	FText Reason;
	const bool bBuildable = bOpen && Subsystem()->CanBuildMetaHuman(Character, Reason);
	return FString::Printf(TEXT("rigged=%d rigging=%d textures=%d requesting=%d buildable=%d open=%d %s"),
		Character->HasFaceDNA() ? 1 : 0, bOpen && Subsystem()->IsAutoRiggingFace(Character) ? 1 : 0,
		Character->HasHighResolutionTextures() ? 1 : 0, bOpen && Subsystem()->IsRequestingHighResolutionTextures(Character) ? 1 : 0,
		bBuildable ? 1 : 0, bOpen ? 1 : 0, *Reason.ToString());
}

void UShortStackMetaHumanLibrary::Close(UMetaHumanCharacter* Character)
{
	if (Character && Subsystem() && Subsystem()->IsObjectAddedForEditing(Character))
	{
		Subsystem()->RemoveObjectToEdit(Character);
	}
}

UMetaHumanCharacter* UShortStackMetaHumanLibrary::Find(const FString& PackagePath)
{
	const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
	return LoadObject<UMetaHumanCharacter>(nullptr, *(PackagePath + TEXT(".") + AssetName), nullptr, LOAD_NoWarn | LOAD_Quiet);
}

bool UShortStackMetaHumanLibrary::Assemble(UMetaHumanCharacter* Character, const FString& OutputPath, const FString& CommonPath, int32 Quality)
{
	if (!EnsureEditing(Character))
	{
		return false;
	}
	FText Reason;
	if (!Subsystem()->CanBuildMetaHuman(Character, Reason))
	{
		UE_LOG(LogShortStackCast, Error, TEXT("Cannot assemble %s: %s"), *Character->GetName(), *Reason.ToString());
		return false;
	}
	// As the Creator's Assemble button does (UMetaHumanCharacterEditorPipelineTool::Build): one of the
	// project's legacy pipelines (an actor Blueprint with its body, face, grooms and outfit), kept on the
	// character, plus a folder for the assets all assembled MetaHumans share.
	const EMetaHumanQualityLevel Level = static_cast<EMetaHumanQualityLevel>(FMath::Clamp(Quality, 0, static_cast<int32>(EMetaHumanQualityLevel::Cinematic)));
	const UMetaHumanCharacterPaletteProjectSettings* Settings = GetDefault<UMetaHumanCharacterPaletteProjectSettings>();
	const TSoftClassPtr<UMetaHumanCollectionPipeline>* ClassPtr = Settings ? Settings->DefaultCharacterLegacyPipelines.Find(Level) : nullptr;
	UClass* PipelineClass = ClassPtr ? ClassPtr->LoadSynchronous() : nullptr;
	if (!PipelineClass)
	{
		UE_LOG(LogShortStackCast, Error, TEXT("No legacy MetaHuman pipeline for quality %d"), Quality);
		return false;
	}
	TObjectPtr<UMetaHumanCollectionPipeline>& Pipeline = Character->PipelinesPerClass.FindOrAdd(PipelineClass);
	if (!Pipeline)
	{
		Pipeline = NewObject<UMetaHumanCollectionPipeline>(Character, PipelineClass);
	}
	if (!Pipeline->GetEditorPipeline() || !Pipeline->GetEditorPipeline()->CanBuild())
	{
		UE_LOG(LogShortStackCast, Error, TEXT("The %s pipeline cannot build %s"), *PipelineClass->GetName(), *Character->GetName());
		return false;
	}
	const FPlatformMemoryStats Memory = FPlatformMemory::GetStats();
	if (Memory.AvailableVirtual < 10ull * 1024 * 1024 * 1024)
	{
		UE_LOG(LogShortStackCast, Warning, TEXT("Assembling with only %llu MB of memory free (the Creator asks for 10 GB)"), Memory.AvailableVirtual / (1024 * 1024));
	}
	FMetaHumanCharacterEditorBuildParameters Params;
	Params.AbsoluteBuildPath = OutputPath;
	Params.CommonFolderPath = CommonPath;
	Params.PipelineOverride = Pipeline;
	FMetaHumanCharacterEditorBuild::BuildMetaHumanCharacter(Character, Params);
	UE_LOG(LogShortStackCast, Display, TEXT("Assembled %s into %s with %s"), *Character->GetName(), *OutputPath, *PipelineClass->GetName());
	return true;
}

bool UShortStackMetaHumanLibrary::SetWardrobe(UMetaHumanCharacter* Character, const FString& SlotName, const FString& WardrobeItemPath)
{
	if (!EnsureEditing(Character))
	{
		return false;
	}
	UMetaHumanCollection* Collection = Character->GetMutableInternalCollection();
	if (!Collection)
	{
		return false;
	}
	const FName Slot(*SlotName);
	FMetaHumanPaletteItemKey Key;
	if (!WardrobeItemPath.IsEmpty())
	{
		UMetaHumanWardrobeItem* Item = LoadObject<UMetaHumanWardrobeItem>(nullptr, *WardrobeItemPath);
		if (!Item)
		{
			UE_LOG(LogShortStackCast, Error, TEXT("No wardrobe item %s"), *WardrobeItemPath);
			return false;
		}
		// Already in the character's collection (the preset's own, or worn before): wear that one.
		const FMetaHumanCharacterPaletteItem* Found = Collection->GetItems().FindByPredicate(
			[Slot, Item](const FMetaHumanCharacterPaletteItem& Existing) { return Existing.SlotName == Slot && Existing.WardrobeItem == Item; });
		if (Found)
		{
			Key = Found->GetItemKey();
		}
		else if (!Collection->TryAddItemFromWardrobeItem(Slot, Item, Key))
		{
			UE_LOG(LogShortStackCast, Error, TEXT("%s can't wear %s in %s"), *Character->GetName(), *WardrobeItemPath, *SlotName);
			return false;
		}
	}
	Collection->GetMutableDefaultInstance()->SetSingleSlotSelection(Slot, Key);
	Character->MarkPackageDirty();
	UE_LOG(LogShortStackCast, Display, TEXT("%s: %s = %s"), *Character->GetName(), *SlotName, WardrobeItemPath.IsEmpty() ? TEXT("(none)") : *WardrobeItemPath);
	return true;
}

UMetaHumanCharacter* UShortStackMetaHumanLibrary::CreateFromBlend(const FString& PackagePath, const TArray<FString>& Presets, const TArray<float>& Weights, bool bRemake)
{
	if (Presets.Num() == 0 || Presets.Num() != Weights.Num())
	{
		UE_LOG(LogShortStackCast, Error, TEXT("CreateFromBlend %s: %d presets and %d weights"), *PackagePath, Presets.Num(), Weights.Num());
		return nullptr;
	}
	float Sum = 0.0f;
	const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
	for (int32 Index = 0; Index < Presets.Num(); ++Index)
	{
		const UMetaHumanCharacter* Source = LoadCharacter(Presets[Index]);
		if (!Source || Source->GetOutermost()->GetName() == PackagePath)
		{
			UE_LOG(LogShortStackCast, Error, TEXT("CreateFromBlend %s: no preset or character %s (a character can't be made from itself)"), *AssetName, *Presets[Index]);
			return nullptr;
		}
		Sum += FMath::Max(0.0f, Weights[Index]);
	}
	if (Sum <= 0.0f)
	{
		UE_LOG(LogShortStackCast, Error, TEXT("CreateFromBlend %s: the weights add up to nothing"), *PackagePath);
		return nullptr;
	}
	const bool bExisted = Find(PackagePath) != nullptr;
	UMetaHumanCharacter* Character = CreateFromPreset(PackagePath, Presets[0], bRemake);
	if (!Character || (bExisted && !bRemake))
	{
		return Character;
	}
	// Presets[0] gave the body, the skin, the eyes, the head model and the wardrobe; the face is the blend. The Creator's
	// blend starts from the face there is and moves each fraction of the way toward each preset, so the first face keeps
	// what the others don't take.
	DropFaceRig(Character);
	TArray<FString> Others;
	TArray<float> Fractions;
	for (int32 Index = 1; Index < Presets.Num(); ++Index)
	{
		Others.Add(Presets[Index]);
		Fractions.Add(FMath::Max(0.0f, Weights[Index]) / Sum);
	}
	if (Others.Num() > 0 && !BlendFace(Character, Others, Fractions, 2, INDEX_NONE))
	{
		return nullptr;
	}
	TArray<float> Normalized;
	for (const float Weight : Weights)
	{
		Normalized.Add(FMath::Max(0.0f, Weight) / Sum);
	}
	UE_LOG(LogShortStackCast, Display, TEXT("%s made from %s"), *Character->GetPathName(), *Recipe(Presets, Normalized));
	return Character;
}

bool UShortStackMetaHumanLibrary::BlendFace(UMetaHumanCharacter* Character, const TArray<FString>& Presets, const TArray<float>& Weights, int32 Mode, int32 Region)
{
	if (!EnsureEditing(Character))
	{
		return false;
	}
	if (Presets.Num() == 0 || Presets.Num() != Weights.Num())
	{
		UE_LOG(LogShortStackCast, Error, TEXT("BlendFace %s: %d presets and %d weights"), *Character->GetName(), Presets.Num(), Weights.Num());
		return false;
	}
	// As UMetaHumanCharacterEditorFaceBlendTool: each preset's face state read into a copy of the character's, and
	// BlendFaceRegion from the face there is (region -1 is the tool's Shift-drag, every region at once).
	TArray<TSharedPtr<const FFaceState>> States;
	TArray<float> Fractions;
	float Sum = 0.0f;
	for (int32 Index = 0; Index < Presets.Num(); ++Index)
	{
		const UMetaHumanCharacter* Source = LoadCharacter(Presets[Index]);
		TSharedPtr<FFaceState> State;
		if (Source)
		{
			State = FaceStateOf(Character, Source);
		}
		if (!State)
		{
			UE_LOG(LogShortStackCast, Error, TEXT("BlendFace %s: can't read the face of %s"), *Character->GetName(), *Presets[Index]);
			return false;
		}
		States.Add(State);
		Fractions.Add(FMath::Max(0.0f, Weights[Index]));
		Sum += Fractions.Last();
	}
	if (Sum > 1.0f)
	{
		for (float& Fraction : Fractions)
		{
			Fraction /= Sum;
		}
	}
	const int32 Regions = Subsystem()->GetFaceState(Character)->NumGizmos();
	if (Region >= Regions)
	{
		UE_LOG(LogShortStackCast, Error, TEXT("BlendFace %s: no region %d (the face has %d)"), *Character->GetName(), Region, Regions);
		return false;
	}
	DropFaceRig(Character);
	const EBlendOptions Options = Mode == 0 ? EBlendOptions::Proportions : Mode == 1 ? EBlendOptions::Features : EBlendOptions::Both;
	const TSharedPtr<const FFaceState> Start = Subsystem()->CopyFaceState(Character);
	Subsystem()->BlendFaceRegion(Character, Region < 0 ? INDEX_NONE : Region, Start, States, Fractions, Options, true);
	CommitFace(Character);
	UE_LOG(LogShortStackCast, Display, TEXT("%s: face blended toward %s (mode %d, region %d)"), *Character->GetName(), *Recipe(Presets, Fractions), Mode, Region);
	return true;
}

int32 UShortStackMetaHumanLibrary::FaceRegionCount(UMetaHumanCharacter* Character)
{
	return EnsureEditing(Character) ? Subsystem()->GetFaceState(Character)->NumGizmos() : 0;
}

bool UShortStackMetaHumanLibrary::SetSkin(UMetaHumanCharacter* Character, float ToneU, float ToneV, int32 FaceTexture, float Roughness)
{
	if (!EnsureEditing(Character))
	{
		return false;
	}
	FMetaHumanCharacterSkinSettings Settings = Character->SkinSettings;
	if (ToneU >= 0.0f)
	{
		Settings.Skin.U = FMath::Clamp(ToneU, 0.0f, 1.0f);
	}
	if (ToneV >= 0.0f)
	{
		Settings.Skin.V = FMath::Clamp(ToneV, 0.0f, 1.0f);
	}
	if (FaceTexture >= 0)
	{
		const int32 Textures = Subsystem()->IsTextureSynthesisEnabled() ? Subsystem()->GetMaxHighFrequencyIndex() : 0;
		if (FaceTexture >= Textures)
		{
			UE_LOG(LogShortStackCast, Error, TEXT("SetSkin %s: no face texture %d (the model has %d)"), *Character->GetName(), FaceTexture, Textures);
			return false;
		}
		Settings.Skin.FaceTextureIndex = FaceTexture;
	}
	if (Roughness > 0.0f)
	{
		Settings.Skin.Roughness = Roughness;
	}
	// As the skin tool commits on closing: the settings (the textures synthesized again, any high-resolution ones
	// dropped), then, when the face texture changed, the face with its high-frequency variant (a tone alone leaves the
	// mesh as it is).
	const bool bNewFaceTexture = Settings.Skin.FaceTextureIndex != Character->SkinSettings.Skin.FaceTextureIndex;
	Subsystem()->CommitSkinSettings(Character, Settings);
	if (bNewFaceTexture)
	{
		CommitFace(Character);
	}
	UE_LOG(LogShortStackCast, Display, TEXT("%s: skin %s"), *Character->GetName(), *DescribeSkin(Character));
	return true;
}

bool UShortStackMetaHumanLibrary::SetSkinColor(UMetaHumanCharacter* Character, FLinearColor SkinSRGB, int32 FaceTexture, float Roughness)
{
	if (!EnsureEditing(Character) || !Subsystem()->IsTextureSynthesisEnabled())
	{
		return false;
	}
	const FVector2f Tone = Subsystem()->EstimateSkinTone(SkinSRGB);
	return SetSkin(Character, Tone.X, Tone.Y, FaceTexture, Roughness);
}

TArray<int32> UShortStackMetaHumanLibrary::FaceTextures(UMetaHumanCharacter* Character, int32 Wrinkles, int32 Stubble, int32 Marks)
{
	TArray<int32> Out;
	if (!EnsureEditing(Character) || !Subsystem()->IsTextureSynthesisEnabled())
	{
		return Out;
	}
	const int32 Textures = Subsystem()->GetMaxHighFrequencyIndex();
	const FMetaHumanFaceTextureAttributeMap& Map = Subsystem()->GetFaceTextureAttributeMap();
	struct FWant
	{
		const TCHAR* Name;
		int32 Value;
	};
	const FWant Wanted[] = {{TEXT("Face Wrinkles"), Wrinkles}, {TEXT("Face Stubble"), Stubble}, {TEXT("Face Marks"), Marks}};
	for (int32 Index = 0; Index < Textures; ++Index)
	{
		bool bMatches = true;
		for (const FWant& Want : Wanted)
		{
			if (Want.Value < 0)
			{
				continue;
			}
			const int32 Attribute = TextureAttribute(Map, Want.Name);
			const TArray<int32>* Values = Attribute != INDEX_NONE ? &Map.GetAttributeValues(Attribute) : nullptr;
			bMatches &= Values && Values->IsValidIndex(Index) && (*Values)[Index] == Want.Value;
		}
		if (bMatches)
		{
			Out.Add(Index);
		}
	}
	return Out;
}

FString UShortStackMetaHumanLibrary::DescribeSkin(UMetaHumanCharacter* Character)
{
	if (!Character)
	{
		return TEXT("missing");
	}
	const FMetaHumanCharacterSkinProperties& Skin = Character->SkinSettings.Skin;
	FString Out = FString::Printf(TEXT("u=%.3f v=%.3f texture=%d rough=%.2f"), Skin.U, Skin.V, Skin.FaceTextureIndex, Skin.Roughness);
	if (Subsystem() && Subsystem()->IsTextureSynthesisEnabled())
	{
		const FMetaHumanFaceTextureAttributeMap& Map = Subsystem()->GetFaceTextureAttributeMap();
		for (int32 Attribute = 0; Attribute < Map.NumAttributes(); ++Attribute)
		{
			const TArray<int32>& Values = Map.GetAttributeValues(Attribute);
			const TArray<FString>& Names = Map.GetAttributeValueNames(Attribute);
			if (Values.IsValidIndex(Skin.FaceTextureIndex) && Names.IsValidIndex(Values[Skin.FaceTextureIndex]))
			{
				const FString Key = Map.GetAttributeName(Attribute).Replace(TEXT("Face "), TEXT("")).ToLower();
				Out += FString::Printf(TEXT(" %s=%s"), *Key, *Names[Values[Skin.FaceTextureIndex]]);
			}
		}
	}
	return Out;
}

bool UShortStackMetaHumanLibrary::SetEyesPreset(UMetaHumanCharacter* Character, int32 EyePreset)
{
	if (!EnsureEditing(Character))
	{
		return false;
	}
	// The eye tool's presets (a UMetaHumanCharacterEyePresets, private to the Creator's module), read by reflection: an
	// array "Presets" of structs, each with its PresetName, EyesSettings and Thumbnail.
	UObject* Presets = LoadObject<UObject>(nullptr, TEXT("/MetaHumanCharacter/Tools/EyePresets/EyePresets.EyePresets"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	const FArrayProperty* List = Presets ? CastField<FArrayProperty>(Presets->GetClass()->FindPropertyByName(TEXT("Presets"))) : nullptr;
	const FStructProperty* Entry = List ? CastField<FStructProperty>(List->Inner) : nullptr;
	const FStructProperty* Eyes = Entry ? CastField<FStructProperty>(Entry->Struct->FindPropertyByName(TEXT("EyesSettings"))) : nullptr;
	if (!Eyes || Eyes->Struct != FMetaHumanCharacterEyesSettings::StaticStruct())
	{
		UE_LOG(LogShortStackCast, Error, TEXT("SetEyesPreset: the Creator's eye presets aren't at /MetaHumanCharacter/Tools/EyePresets/EyePresets"));
		return false;
	}
	FScriptArrayHelper Array(List, List->ContainerPtrToValuePtr<void>(Presets));
	// Eye preset N is the one whose thumbnail is T_EyePreset_00N (the numbers the eye colors above are listed by), wherever
	// it sits in the list; the list's Nth entry when no thumbnail says.
	const FSoftObjectProperty* Thumbnail = CastField<FSoftObjectProperty>(Entry->Struct->FindPropertyByName(TEXT("Thumbnail")));
	const FNameProperty* PresetName = CastField<FNameProperty>(Entry->Struct->FindPropertyByName(TEXT("PresetName")));
	const FString Wanted = FString::Printf(TEXT("T_EyePreset_%03d"), EyePreset);
	int32 Found = INDEX_NONE;
	for (int32 Index = 0; Thumbnail && Index < Array.Num() && Found == INDEX_NONE; ++Index)
	{
		const FSoftObjectPtr& Image = *Thumbnail->ContainerPtrToValuePtr<FSoftObjectPtr>(Array.GetRawPtr(Index));
		if (Image.ToSoftObjectPath().GetAssetName() == Wanted)
		{
			Found = Index;
		}
	}
	if (Found == INDEX_NONE)
	{
		Found = EyePreset - 1;
	}
	if (!Array.IsValidIndex(Found))
	{
		UE_LOG(LogShortStackCast, Error, TEXT("SetEyesPreset %s: no eye preset %d (1..%d)"), *Character->GetName(), EyePreset, Array.Num());
		return false;
	}
	FMetaHumanCharacterEyesSettings Settings;
	Eyes->CopyCompleteValue(&Settings, Eyes->ContainerPtrToValuePtr<void>(Array.GetRawPtr(Found)));
	Subsystem()->CommitEyesSettings(Character, Settings);
	const FName Name = PresetName ? *PresetName->ContainerPtrToValuePtr<FName>(Array.GetRawPtr(Found)) : NAME_None;
	UE_LOG(LogShortStackCast, Display, TEXT("%s: eyes from eye preset %d (%s, entry %d)"), *Character->GetName(), EyePreset, *Name.ToString(), Found);
	return true;
}

bool UShortStackMetaHumanLibrary::SetAging(UMetaHumanCharacter* Character, float TeethWear, float LashGrey)
{
	if (!EnsureEditing(Character))
	{
		return false;
	}
	FMetaHumanCharacterHeadModelSettings Head = Character->HeadModelSettings;
	if (TeethWear >= 0.0f)
	{
		const float Wear = FMath::Clamp(TeethWear, 0.0f, 1.0f);
		Head.Teeth.WornDown = 0.6f * Wear;
		Head.Teeth.RecedingGums = 0.5f * Wear;
		Head.Teeth.PlaqueAmount = 0.3f * Wear;
		// "Teeth Basecolor Multiply": ivory with the years.
		Head.Teeth.TeethColor = FMath::Lerp(FLinearColor::White, FLinearColor(0.93f, 0.86f, 0.72f), Wear);
	}
	if (LashGrey >= 0.0f)
	{
		Head.Eyelashes.SaltAndPepper = FMath::Clamp(LashGrey, 0.0f, 1.0f);
	}
	// As the head tool commits: the settings (materials, eyelash grooms), then the face with the teeth variant.
	Subsystem()->CommitHeadModelSettings(Character, Head);
	if (TeethWear >= 0.0f)
	{
		CommitFace(Character);
	}
	// SaltAndPepper is kept as the Creator stores it, but nothing in 5.6 reads it (its spin box is commented out and no
	// material takes it). The grey goes where the lashes' color is assembled from: the eyelash groom's pipeline
	// parameters, as SetGroomColor greys the hair (WhiteAmount on the cards; the preset's own lash color is kept).
	if (LashGrey >= 0.0f)
	{
		SetGroomColor(Character, TEXT("Eyelashes"), -1.0f, -1.0f, Head.Eyelashes.SaltAndPepper, -1.0f);
	}
	UE_LOG(LogShortStackCast, Display, TEXT("%s: teeth wear %.2f, lash grey %.2f"), *Character->GetName(), TeethWear, Head.Eyelashes.SaltAndPepper);
	return true;
}

bool UShortStackMetaHumanLibrary::SetGroomColor(UMetaHumanCharacter* Character, const FString& SlotName, float Melanin, float Redness, float Whiteness, float Lightness)
{
	if (!EnsureEditing(Character))
	{
		return false;
	}
	UMetaHumanCollection* Collection = Character->GetMutableInternalCollection();
	if (!Collection)
	{
		return false;
	}
	UMetaHumanCharacterInstance* Instance = Collection->GetMutableDefaultInstance();
	const FName Slot(*SlotName);
	const FMetaHumanPipelineSlotSelectionData* Worn = Instance->GetSlotSelectionData().FindByPredicate(
		[Slot](const FMetaHumanPipelineSlotSelectionData& Data) { return Data.Selection.SlotName == Slot; });
	if (!Worn || Worn->Selection.SelectedItem.IsNull())
	{
		UE_LOG(LogShortStackCast, Display, TEXT("%s: nothing worn as %s to color"), *Character->GetName(), *SlotName);
		return false;
	}
	// The groom pipeline's instance parameters (UMetaHumanDefaultGroomPipelineMaterialParameters: Melanin drives
	// hairMelanin, Redness hairRedness, Whiteness WhiteAmount, Lightness LightAmount), overridden for the worn item as the
	// Creator's wardrobe panel overrides them. Assembly applies the overrides before the materials are unpacked.
	FInstancedPropertyBag Parameters;
	struct FParameter
	{
		const TCHAR* Name;
		float Value;
	};
	const FParameter Values[] = {{TEXT("Melanin"), Melanin}, {TEXT("Redness"), Redness}, {TEXT("Whiteness"), Whiteness}, {TEXT("Lightness"), Lightness}};
	for (const FParameter& Value : Values)
	{
		if (Value.Value >= 0.0f)
		{
			Parameters.AddProperty(Value.Name, EPropertyBagPropertyType::Float);
			Parameters.SetValueFloat(Value.Name, FMath::Clamp(Value.Value, 0.0f, 1.0f));
		}
	}
	if (!Parameters.IsValid())
	{
		return true;
	}
	Instance->Modify();
	Instance->OverrideInstanceParameters(Worn->Selection.GetSelectedItemPath(), Parameters);
	Character->MarkPackageDirty();
	UE_LOG(LogShortStackCast, Display, TEXT("%s: %s melanin %.2f redness %.2f white %.2f light %.2f"), *Character->GetName(), *SlotName, Melanin, Redness, Whiteness, Lightness);
	return true;
}

TArray<float> UShortStackMetaHumanLibrary::FaceDistances(UMetaHumanCharacter* Character, const TArray<FString>& Others)
{
	TArray<float> Out;
	if (!EnsureEditing(Character))
	{
		return Out;
	}
	TArray<FVector3f> Mine = Subsystem()->GetFaceState(Character)->Evaluate().Vertices;
	CenterOnSelf(Mine);
	for (const FString& Other : Others)
	{
		float Distance = -1.0f;
		const UMetaHumanCharacter* Source = LoadCharacter(Other);
		const TSharedPtr<FFaceState> State = Source && Source != Character ? FaceStateOf(Character, Source) : TSharedPtr<FFaceState>();
		if (Source == Character)
		{
			Distance = 0.0f;
		}
		else if (State)
		{
			TArray<FVector3f> Theirs = State->Evaluate().Vertices;
			CenterOnSelf(Theirs);
			if (Theirs.Num() == Mine.Num() && Mine.Num() > 0)
			{
				double Sum = 0.0;
				for (int32 Index = 0; Index < Mine.Num(); ++Index)
				{
					Sum += FVector3d(Mine[Index] - Theirs[Index]).SizeSquared();
				}
				Distance = static_cast<float>(FMath::Sqrt(Sum / Mine.Num()));
			}
		}
		Out.Add(Distance);
	}
	return Out;
}

namespace ShortStackBake
{
/** A skeletal mesh component as it's posed now, as a transient static mesh (in its component space), each section with
 *  the component's material for it. */
UStaticMesh* Posed(const USkeletalMeshComponent* C, int32 LOD, const IMeshMergeUtilities& Merge)
{
	USkeletalMesh* Skel = C->GetSkeletalMeshAsset();
	const int32 Lod = FMath::Clamp(LOD, 0, Skel->GetLODNum() - 1);
	FMeshDescription Desc;
	FStaticMeshAttributes(Desc).Register();
	Merge.RetrieveMeshDescription(C, Lod, Desc, false);
	if (Desc.Triangles().Num() == 0)
	{
		return nullptr;
	}
	UStaticMesh* M = NewObject<UStaticMesh>(GetTransientPackage(), NAME_None, RF_Transient);
	// The polygon groups are the sections, in order: one material each, named as the group is.
	TPolygonGroupAttributesConstRef<FName> Names = FStaticMeshConstAttributes(Desc).GetPolygonGroupMaterialSlotNames();
	const TArray<FSkeletalMaterial>& Materials = Skel->GetMaterials();
	for (const FPolygonGroupID G : Desc.PolygonGroups().GetElementIDs())
	{
		UMaterialInterface* Mat = nullptr;
		for (int32 K = 0; K < Materials.Num() && !Mat; ++K)
		{
			if (Materials[K].ImportedMaterialSlotName == Names[G])
			{
				Mat = C->GetMaterial(K);
			}
		}
		M->GetStaticMaterials().Add(FStaticMaterial(Mat, Names[G], Names[G]));
	}
	FStaticMeshSourceModel& Src = M->AddSourceModel();
	Src.BuildSettings.bRecomputeNormals = false;
	Src.BuildSettings.bRecomputeTangents = false;
	Src.BuildSettings.bGenerateLightmapUVs = false;
	Src.BuildSettings.bBuildReversedIndexBuffer = false;
	M->CreateMeshDescription(0, MoveTemp(Desc));
	M->CommitMeshDescription(0);
	M->Build(true);
	return M;
}

/** A groom's hair color from its material's melanin and redness: blond to black, warmed toward red. */
FLinearColor HairColor(const UMaterialInterface* GroomMaterial)
{
	float Melanin = 0.5f, Redness = 0.1f;
	if (GroomMaterial)
	{
		GroomMaterial->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("hairMelanin")), Melanin);
		GroomMaterial->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("hairRedness")), Redness);
	}
	const FLinearColor Blond(0.55f, 0.42f, 0.26f), Black(0.012f, 0.009f, 0.007f);
	FLinearColor C = FMath::Lerp(Blond, Black, FMath::Pow(FMath::Clamp(Melanin, 0.0f, 1.0f), 0.7f));
	return FMath::Lerp(C, C * FLinearColor(1.6f, 0.8f, 0.5f), FMath::Clamp(Redness, 0.0f, 1.0f));
}
} // namespace ShortStackBake

UStaticMesh* UShortStackMetaHumanLibrary::BakeFigure(AActor* Source, const FString& BasePath, int32 LOD, UMaterialInterface* Invisible, UMaterialInterface* HairCards)
{
	if (!Source || !Source->GetWorld())
	{
		return nullptr;
	}
	// The pose is the bones' (component space): moved to the origin, the actor bakes in its own frame.
	const FTransform Was = Source->GetActorTransform();
	Source->SetActorTransform(FTransform::Identity, false, nullptr, ETeleportType::TeleportPhysics);
	const IMeshMergeUtilities& Merge = FModuleManager::Get().LoadModuleChecked<IMeshMergeModule>("MeshMergeUtilities").GetUtilities();
	TArray<UPrimitiveComponent*> Parts;
	TArray<UStaticMeshComponent*> Temp;
	TInlineComponentArray<USkeletalMeshComponent*> Skinned(Source);
	for (USkeletalMeshComponent* C : Skinned)
	{
		if (!C->IsVisible() || !C->GetSkeletalMeshAsset())
		{
			continue;
		}
		// The merge takes static meshes: each skeletal part as it's posed, in its place.
		if (UStaticMesh* Posed = ShortStackBake::Posed(C, LOD, Merge))
		{
			UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(Source, NAME_None, RF_Transient);
			M->SetStaticMesh(Posed);
			M->SetWorldTransform(C->GetComponentTransform());
			M->RegisterComponent();
			Parts.Add(M);
			Temp.Add(M);
		}
	}
	TInlineComponentArray<UStaticMeshComponent*> Statics(Source);
	for (UStaticMeshComponent* C : Statics)
	{
		// What they wear (the chair stays the table's).
		if (C->IsVisible() && C->GetStaticMesh() && C->GetStaticMesh()->GetName().StartsWith(TEXT("SM_Wear")))
		{
			Parts.Add(C);
		}
	}
	TInlineComponentArray<UGroomComponent*> Grooms(Source);
	for (UGroomComponent* G : Grooms)
	{
		if (!G->IsVisible() || !G->GroomAsset)
		{
			continue;
		}
		// Every group's cards at the groom's nearest cards LOD (long styles split their cards across groups).
		int32 CardsLOD = INDEX_NONE;
		for (const FHairGroupsCardsSourceDescription& D : G->GroomAsset->GetHairGroupsCards())
		{
			if (D.ImportedMesh && D.LODIndex >= 0)
			{
				CardsLOD = CardsLOD == INDEX_NONE ? D.LODIndex : FMath::Min(CardsLOD, D.LODIndex);
			}
		}
		if (CardsLOD == INDEX_NONE)
		{
			continue;
		}
		UMaterialInterface* GroomMaterial = nullptr;
		for (const FHairGroupsCardsSourceDescription& D : G->GroomAsset->GetHairGroupsCards())
		{
			if (!D.ImportedMesh || D.LODIndex != CardsLOD)
			{
				continue;
			}
			// A groom hangs from a face socket (the facial root, the jaw) offset so that at rest its space is the
			// face's, where its cards are modeled: the cards go exactly where the groom is now.
			UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(Source, NAME_None, RF_Transient);
			M->SetStaticMesh(D.ImportedMesh);
			M->SetWorldTransform(G->GetComponentTransform());
			const int32 Slot = G->GroomAsset->GetMaterialIndex(D.MaterialSlotName);
			UMaterialInterface* HairMaterial = G->GetMaterial(Slot >= 0 ? Slot : 0);
			GroomMaterial = GroomMaterial ? GroomMaterial : HairMaterial;
			if (HairCards)
			{
				// Coverage lives in one of the atlas textures, by layout (HairCardsAttributeCommon.ush): the second
				// texture's red, except the compact mesh layout's third texture's alpha.
				const FHairGroupCardsTextures& Atlas = D.Textures;
				const bool bMeshCompact = Atlas.Layout == EHairTextureLayout::Layout3;
				const int32 CoverageIndex = bMeshCompact ? 2 : 1;
				UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(HairCards, Source);
				if (Atlas.Textures.IsValidIndex(CoverageIndex) && Atlas.Textures[CoverageIndex])
				{
					Mid->SetTextureParameterValue(TEXT("Coverage"), Atlas.Textures[CoverageIndex]);
				}
				Mid->SetScalarParameterValue(TEXT("CoverageChannel"), bMeshCompact ? 3.0f : 0.0f);
				// One color for the groom (its first group's material carries it).
				Mid->SetVectorParameterValue(TEXT("HairColor"), ShortStackBake::HairColor(GroomMaterial));
				HairMaterial = Mid;
			}
			if (HairMaterial)
			{
				for (int32 K = 0; K < D.ImportedMesh->GetStaticMaterials().Num(); ++K)
				{
					M->SetMaterial(K, HairMaterial);
				}
			}
			M->RegisterComponent();
			Parts.Add(M);
			Temp.Add(M);
		}
	}
	FMeshMergingSettings Settings;
	Settings.LODSelectionType = EMeshLODSelectionType::SpecificLOD;
	Settings.SpecificLOD = 0; // the posed parts are already at LOD, the cards and what they wear have one
	Settings.bMergeMaterials = false;
	Settings.bPivotPointAtZero = true;
	Settings.bMergePhysicsData = false;
	Settings.bGenerateLightMapUV = false;
	Settings.bAllowDistanceField = false;
	Settings.NaniteSettings.bEnabled = true;
	TArray<UObject*> Made;
	FVector Location = FVector::ZeroVector;
	Merge.MergeComponentsToStaticMesh(Parts, Source->GetWorld(), Settings, nullptr, nullptr, BasePath, Made, Location, 1.0f, true);
	for (UStaticMeshComponent* M : Temp)
	{
		M->DestroyComponent();
	}
	Source->SetActorTransform(Was, false, nullptr, ETeleportType::TeleportPhysics);
	UStaticMesh* Mesh = nullptr;
	for (UObject* O : Made)
	{
		Mesh = Mesh ? Mesh : Cast<UStaticMesh>(O);
	}
	if (!Mesh)
	{
		return nullptr;
	}
	// Nanite draws no translucency: the eye shells and tear lines go (the eyes themselves are opaque).
	bool bChanged = false;
	TArray<FStaticMaterial>& Materials = Mesh->GetStaticMaterials();
	for (FStaticMaterial& M : Materials)
	{
		if (Invisible && M.MaterialInterface && IsTranslucentBlendMode(M.MaterialInterface->GetBlendMode()))
		{
			M.MaterialInterface = Invisible;
			bChanged = true;
		}
	}
	if (bChanged)
	{
		Mesh->PostEditChange();
	}
	Mesh->MarkPackageDirty();
	UPackage* Package = Mesh->GetPackage();
	const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Mesh, *Filename, Args);
	FAssetRegistryModule::AssetCreated(Mesh);
	return Mesh;
}

bool UShortStackMetaHumanLibrary::Save(UMetaHumanCharacter* Character)
{
	if (!Character)
	{
		return false;
	}
	UPackage* Package = Character->GetPackage();
	const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	return UPackage::SavePackage(Package, Character, *Filename, Args);
}
