using UnrealBuildTool;

public class ShortStackTarget : TargetRules
{
	public ShortStackTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("ShortStack");
	}
}
