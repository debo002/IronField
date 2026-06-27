using UnrealBuildTool;
using System.Collections.Generic;

public class IronFieldEditorTarget : TargetRules
{
	public IronFieldEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("IronField");
	}
}
