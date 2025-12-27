using UnrealBuildTool;

public class SOTS_GlobalStealthManager : ModuleRules
{
    public SOTS_GlobalStealthManager(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "DeveloperSettings",
                "GameplayTags",
                "SOTS_TagManager",
                "SOTS_ProfileShared"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "SOTS_Core"
            }
        );
    }
}
