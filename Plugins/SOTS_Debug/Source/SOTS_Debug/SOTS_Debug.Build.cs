using UnrealBuildTool;
using System.Collections.Generic;

public class SOTS_Debug : ModuleRules
{
    public SOTS_Debug(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "UMG",
            "Slate",
            "SlateCore",
            "GameplayTags",
            "SOTS_ProfileShared",
            "SOTS_TagManager",
            "SOTS_INV",
            "SOTS_GlobalStealthManager",
            "SOTS_GAS_Plugin",
            "SOTS_Stats",
            "SOTS_MMSS",
            "SOTS_FX_Plugin",
            "SOTS_MissionDirector",
            "SOTS_KillExecutionManager"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "DeveloperSettings",
            "SOTS_Core"
        });
    }
}
