using UnrealBuildTool;

public class SOTS_Stats : ModuleRules
{
    public SOTS_Stats(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "DeveloperSettings",
            "GameplayTags",
            "SOTS_ProfileShared"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "SOTS_Core"
        });
    }
}
