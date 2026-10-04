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

	/**
	 * Wears a wardrobe item in a slot, as the Creator's wardrobe does: a groom's WI_ asset ("Hair", "Beard", "Mustache",
	 * "Eyebrows") from /MetaHumanCharacter/Optional/Grooms/Bindings. An empty path clears the slot (a clean shave).
	 */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|MetaHuman")
	static bool SetWardrobe(UMetaHumanCharacter* Character, const FString& SlotName, const FString& WardrobeItemPath);

	/**
	 * Bakes an actor's MetaHuman as it stands this frame into one static mesh, for the card room's far tables: its
	 * skeletal meshes in their pose at a mesh LOD, what it wears (hats, glasses), and its grooms' nearest hair cards
	 * carried by the head from the face's rest pose. In the actor's own frame (its pivot at the actor), Nanite, its
	 * materials kept (dyed instances copied into the mesh); translucent sections (eye shells, tear lines) take
	 * Invisible; the hair cards take an instance of HairCards (M_HairCardsStatic: the groom's own material only draws
	 * through the groom) with their coverage atlas and the groom's color. BasePath "/Game/.../Crowd_X" saves
	 * "/Game/.../SM_Crowd_X". Returns the mesh, saved.
	 */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|MetaHuman")
	static UStaticMesh* BakeFigure(AActor* Source, const FString& BasePath, int32 LOD, UMaterialInterface* Invisible, UMaterialInterface* HairCards);
};
