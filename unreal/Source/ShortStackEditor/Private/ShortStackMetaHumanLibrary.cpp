#include "ShortStackMetaHumanLibrary.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Cloud/MetaHumanARServiceRequest.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
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
		const FString PresetPath = FString::Printf(TEXT("%s/%s.%s"), PresetsPath, *PresetName, *PresetName);
		UMetaHumanCharacter* Preset = LoadObject<UMetaHumanCharacter>(nullptr, *PresetPath);
		if (!Preset)
		{
			UE_LOG(LogShortStackCast, Error, TEXT("No MetaHuman preset %s"), *PresetPath);
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
