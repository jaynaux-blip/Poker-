using UnrealBuildTool;

// Editor-only tools: creating the Back Room's cast, the heroes and the block's staff with MetaHuman Creator from Python
// (Content/Python/backroom_cast.py, hero_cast.py, staff_cast.py), and baking the card room's crowd from it
// (backroom_crowd.py). MetaHumanCoreTechLib is the Creator's face model (blending faces, the face texture scans).
public class ShortStackEditor : ModuleRules
{
	public ShortStackEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine",
		});
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd", "AssetRegistry", "MetaHumanCharacter", "MetaHumanCharacterEditor", "MetaHumanCharacterPalette",
			"MetaHumanSDKEditor", "MetaHumanSDKRuntime", "MetaHumanCoreTechLib", "RigLogicModule", "MeshMergeUtilities", "HairStrandsCore", "MeshDescription", "StaticMeshDescription",
		});
	}
}
