using UnrealBuildTool;

public class SOTS_INV : ModuleRules
{
	public SOTS_INV(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"SOTS_TagManager",
				"SOTS_ProfileShared"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"SOTS_Core",
				// SOTS_INV is a SOTS-side bridge; InvSP integrates via Blueprint adapters and does not require InvSP C++ modules.
			}
		);
	}
}
