using UnrealBuildTool;

public class ZombieSurvivalEditorTarget : TargetRules
{
	public ZombieSurvivalEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("ZombieSurvival");
	}
}
