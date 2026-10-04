using UnrealBuildTool;

// Editor-only tools: creating the Back Room's cast with MetaHuman Creator from Python
// (Content/Python/backroom_cast.py), and baking the card room's crowd from it (backroom_crowd.py).
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
			"MetaHumanSDKEditor", "MetaHumanSDKRuntime", "RigLogicModule", "MeshMergeUtilities", "HairStrandsCore", "MeshDescription", "StaticMeshDescription",
		});
	}
}
