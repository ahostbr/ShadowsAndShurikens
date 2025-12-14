using UnrealBuildTool;

public class SOTS_UI : ModuleRules
{
	public SOTS_UI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"StructUtils",
				"MediaAssets",
				"UMG",
				"Slate",
				"SlateCore",
				"GameplayTags",
				"SOTS_TagManager",
				"DeveloperSettings",
				"InputCore"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// TODO: add ProHUDV2 (or equivalent) when HUD module exists
			}
		);
	}
}
