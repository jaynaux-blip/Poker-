using UnrealBuildTool;

public class ShortStackCore : ModuleRules
{
	public ShortStackCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Projects" });

		// The engine reproduces the TypeScript prototype bit for bit (Tests/golden_vectors.txt),
		// which needs strict IEEE floating point: no fast-math reassociation, no fused multiply-add.
		// FPSemantics exists in UE 5.1 and later; delete this line on older engine versions.
		FPSemantics = FPSemanticsMode.Precise;
	}
}
