using UnrealBuildTool;

public class SOTS_SkillTree : ModuleRules
{
    public SOTS_SkillTree(ReadOnlyTargetRules Target) : base(Target)
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
                "SOTS_GlobalStealthManager",
                "SOTS_FX_Plugin",
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
