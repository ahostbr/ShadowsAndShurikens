using UnrealBuildTool;

public class SOTS_Steam : ModuleRules
{
    public SOTS_Steam(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "OnlineSubsystem"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "DeveloperSettings",
                "GameplayTags",
                "OnlineSubsystemUtils",
                "SOTS_MissionDirector"
            }
        );

        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                "OnlineSubsystemSteam"
            }
        );
    }
}
