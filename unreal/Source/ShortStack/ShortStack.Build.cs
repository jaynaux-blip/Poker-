using UnrealBuildTool;

public class ShortStack : ModuleRules
{
	public ShortStack(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		// Each source file keeps its helpers in its own namespace and opens it with a using-directive:
		// unity builds would merge those files and make the helpers' names ambiguous.
		bUseUnity = false;
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "RHI", "Slate", "SlateCore", "UMG", "AnimationCore", "RigLogicModule", "HairStrandsCore", "MeshDescription", "StaticMeshDescription", "ShortStackCore",
		});
	}
}
