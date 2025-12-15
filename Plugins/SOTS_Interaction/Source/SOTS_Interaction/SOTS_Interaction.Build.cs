using UnrealBuildTool;

public class SOTS_Interaction : ModuleRules
{
    public SOTS_Interaction(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "GameplayTags"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "SOTS_TagManager",
                "OmniTrace"
            }
        );
    }
}
