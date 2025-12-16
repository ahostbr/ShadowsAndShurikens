using UnrealBuildTool;

public class SOTS_MissionDirector : ModuleRules
{
    public SOTS_MissionDirector(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "GameplayTags",
                "SOTS_GlobalStealthManager",
                "SOTS_FX_Plugin",
                "SOTS_KillExecutionManager",
                "SOTS_TagManager",
                "SOTS_ProfileShared",
                "SOTS_UI"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
            }
        );
    }
}
