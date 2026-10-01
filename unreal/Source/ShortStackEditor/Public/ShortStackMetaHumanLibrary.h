#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "ShortStackMetaHumanLibrary.generated.h"

class UMetaHumanCharacter;

/**
 * MetaHuman Creator, scripted: the steps of the Creator's UI as functions Python can call to build the
 * Back Room's cast (Content/Python/backroom_cast.py). Rigging the face and the high-resolution skin
 * textures come from Epic's MetaHuman cloud service: the first request opens Epic's sign-in page.
 */
UCLASS()
class UShortStackMetaHumanLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** The Creator's presets (names you can pass to CreateFromPreset). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|MetaHuman")
	static TArray<FString> ListPresets();

	/**
	 * Creates a MetaHuman Character asset at PackagePath (for example /Game/ShortStack/Cast/MHC_Dee) from a
	 * preset, or loads it if it exists. Either way it is opened for editing, which the other calls need.
	 */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|MetaHuman")
	static UMetaHumanCharacter* CreateFromPreset(const FString& PackagePath, const FString& PresetName, bool bReapplyPreset = false);

	/** Starts rigging the face on the cloud service (asynchronous: poll Status). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|MetaHuman")
	static bool AutoRig(UMetaHumanCharacter* Character, bool bBlendShapes = true);

	/** Starts downloading the high-resolution face and body textures (2048, 4096 or 8192; asynchronous). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|MetaHuman")
	static bool RequestTextures(UMetaHumanCharacter* Character, int32 Resolution = 4096);

	/** "rigged=1 rigging=0 textures=1 requesting=0 buildable=1 open=1 <reason>" (without opening it). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|MetaHuman")
	static FString Status(UMetaHumanCharacter* Character);

	/** Ends the editing session CreateFromPreset opened (its preview meshes and textures are freed). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|MetaHuman")
	static void Close(UMetaHumanCharacter* Character);

	/** The character asset at PackagePath, if there is one (not opened). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|MetaHuman")
	static UMetaHumanCharacter* Find(const FString& PackagePath);

	/**
	 * Assembles the character (meshes, grooms, outfit and actor Blueprint) under OutputPath/<Name>, with the
	 * assets MetaHumans share in CommonPath. Quality: 0 low, 1 medium, 2 high, 3 cinematic.
	 */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|MetaHuman")
	static bool Assemble(UMetaHumanCharacter* Character, const FString& OutputPath, const FString& CommonPath, int32 Quality = 3);

	UFUNCTION(BlueprintCallable, Category = "Short Stack|MetaHuman")
	static bool Save(UMetaHumanCharacter* Character);
};
