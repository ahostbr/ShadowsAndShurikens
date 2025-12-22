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
                "SOTS_INV",
                "SOTS_TagManager",
                "OmniTrace",
                "EnhancedInput"
            }
        );
    }
}
