using UnrealBuildTool;

public class ShortStackEditorTarget : TargetRules
{
	public ShortStackEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("ShortStack");
	}
}
