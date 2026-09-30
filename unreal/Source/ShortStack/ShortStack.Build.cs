using UnrealBuildTool;

public class ShortStack : ModuleRules
{
	public ShortStack(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "Slate", "SlateCore", "UMG", "ShortStackCore",
		});
	}
}
