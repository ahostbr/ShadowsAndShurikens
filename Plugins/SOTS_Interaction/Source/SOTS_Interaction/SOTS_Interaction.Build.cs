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
                "GameplayTags",
                "SOTS_Input"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "SOTS_BodyDrag",
                "SOTS_INV",
                "SOTS_KillExecutionManager",
                "SOTS_TagManager",
                "OmniTrace",
                "EnhancedInput"
            }
        );
    }
}
