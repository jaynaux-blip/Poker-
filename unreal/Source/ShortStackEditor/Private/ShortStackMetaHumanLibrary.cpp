#include "ShortStackMetaHumanLibrary.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Cloud/MetaHumanARServiceRequest.h"
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
