using UnrealBuildTool;

public class ZombieSurvivalTarget : TargetRules
{
	public ZombieSurvivalTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("ZombieSurvival");
	}
}
