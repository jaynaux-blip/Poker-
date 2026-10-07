#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "ShortStackMetaHumanLibrary.generated.h"

class UMetaHumanCharacter;

/**
 * MetaHuman Creator, scripted: the steps of the Creator's UI as functions Python can call to build the
 * Back Room's cast (Content/Python/backroom_cast.py), the heroes (hero_cast.py) and the block's staff
 * (staff_cast.py). Rigging the face and the high-resolution skin textures come from Epic's MetaHuman cloud
 * service: the first request opens Epic's sign-in page.
 *
 * The staff are faces nobody else wears, made the way the Creator's Blend tool makes them (a weighted blend of
 * presets) with the skin, eyes, teeth and groom colors of their age: CreateFromBlend, then SetSkinColor (the tone
 * and a face texture from FaceTextures, which picks the scans' wrinkles), SetEyesPreset, SetAging, SetWardrobe and
 * SetGroomColor, all before AutoRig (the face texture and the teeth are in the face mesh the rig is made from).
 * FaceDistances tells a twin from a new face.
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
	 * preset (its name, or another character's package path), or loads it if it exists. Either way it is opened for
	 * editing, which the other calls need.
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
	 * Makes a character from a weighted blend of presets, as the Creator's Blend tool does: created at PackagePath from
	 * Presets[0] (its body, skin, eyes, teeth and wardrobe come with it), then its face blended toward the other presets
	 * over every region, proportions and features both. Weights go with Presets and are normalized: {0.4, 0.35, 0.25}
	 * keeps 40% of the first face and takes 35% and 25% of the others. An existing character is only opened, unless
	 * bRemake, which starts it again from Presets[0]. A face rig is removed (a new face needs AutoRig). Null when a
	 * preset is missing or the blend fails.
	 */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|MetaHuman")
	static UMetaHumanCharacter* CreateFromBlend(const FString& PackagePath, const TArray<FString>& Presets, const TArray<float>& Weights, bool bRemake = false);

	/**
	 * Blends an open character's face toward presets (names, or characters' package paths): it moves Weights[i] of the way
	 * toward Presets[i], their sum at most 1, and keeps the rest. Mode 0 blends the proportions, 1 the features, 2 both.
	 * Region -1 is the whole face; 0..FaceRegionCount-1 one of the Creator's face regions, mirrored. Removes a face rig.
	 */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|MetaHuman")
	static bool BlendFace(UMetaHumanCharacter* Character, const TArray<FString>& Presets, const TArray<float>& Weights, int32 Mode = 2, int32 Region = -1);

	/** How many face regions (the Creator's blend and sculpt handles) an open character's face has: BlendFace's Region. */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|MetaHuman")
	static int32 FaceRegionCount(UMetaHumanCharacter* Character);

	/**
	 * The skin, as the Creator's skin tool sets it: the tone (the skin tone picker's U and V, 0..1; negative keeps), the
	 * face texture (an index from FaceTextures; -1 keeps) and the roughness (<= 0 keeps). The face texture is also the
	 * face's fine wrinkles in the mesh, so set it before AutoRig. A new skin discards high-resolution textures: request
	 * them after.
	 */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|MetaHuman")
	static bool SetSkin(UMetaHumanCharacter* Character, float ToneU, float ToneV, int32 FaceTexture = -1, float Roughness = -1.0f);

	/** SetSkin with the tone the Creator's skin model makes nearest to an sRGB skin color (0..1 sRGB values, not linear). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|MetaHuman")
	static bool SetSkinColor(UMetaHumanCharacter* Character, FLinearColor SkinSRGB, int32 FaceTexture = -1, float Roughness = -1.0f);

	/**
	 * The face textures (SetSkin's FaceTexture) whose scans have these wrinkles (0 low, 1 medium, 2 high: the skin's age),
	 * stubble (0 none, 1 low, 2 medium, 3 high) and marks (0 low, 1 medium, 2 high); -1 matches any. Needs Character open
	 * (the Creator loads its texture model with the first character it opens). Empty when nothing matches.
	 */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|MetaHuman")
	static TArray<int32> FaceTextures(UMetaHumanCharacter* Character, int32 Wrinkles = -1, int32 Stubble = -1, int32 Marks = -1);

	/** "u=0.410 v=0.520 texture=37 rough=1.06 wrinkles=Medium stubble=None marks=Low" (the scan's attributes once any character is open). */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|MetaHuman")
	static FString DescribeSkin(UMetaHumanCharacter* Character);

	/**
	 * The eyes of one of the Creator's eye presets, 1..12, numbered as their thumbnails are (T_EyePreset_001..012): 1 dark
	 * brown, 2 brown, 3 amber, 4 grey-blue, 5 grey, 6 pale blue, 7 green, 8 olive, 9 hazel, 10 blue with a brown ring,
	 * 11 green with an amber ring, 12 grey.
	 */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|MetaHuman")
	static bool SetEyesPreset(UMetaHumanCharacter* Character, int32 EyePreset);

	/**
	 * Age in the head model: TeethWear 0..1 wears the teeth down, draws the gums back, yellows them and adds plaque (the
	 * teeth are in the face mesh: before AutoRig); LashGrey 0..1 greys the eyelashes, on the eyelash groom's Whiteness
	 * (the head model's SaltAndPepper is recorded too, but 5.6 reads it nowhere). Negative keeps either.
	 */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|MetaHuman")
	static bool SetAging(UMetaHumanCharacter* Character, float TeethWear, float LashGrey = -1.0f);

	/**
	 * The color of the groom worn in a slot ("Hair", "Eyebrows", "Beard", "Mustache"), as the Creator's wardrobe sets it:
	 * melanin (0 fair to 1 black), redness, whiteness (grey) and lightness, each 0..1, negative keeps. Assembly bakes it
	 * into the grooms' materials (and into the face where a style is painted on). False when nothing is worn there.
	 */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|MetaHuman")
	static bool SetGroomColor(UMetaHumanCharacter* Character, const FString& SlotName, float Melanin, float Redness = -1.0f, float Whiteness = -1.0f, float Lightness = -1.0f);

	/**
	 * How far an open character's face is from others', to tell a twin from a new face: the RMS distance in cm between
	 * the neutral face meshes (each centered on itself), one per entry of Others (a preset's name, or a character's
	 * package path); -1 where a face can't be read.
	 */
	UFUNCTION(BlueprintCallable, Category = "Short Stack|MetaHuman")
	static TArray<float> FaceDistances(UMetaHumanCharacter* Character, const TArray<FString>& Others);

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
