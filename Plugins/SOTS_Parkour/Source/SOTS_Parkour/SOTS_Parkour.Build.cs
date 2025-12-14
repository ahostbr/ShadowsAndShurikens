// SOTS_Parkour.Build.cs
// Runtime module for the SOTS Parkour plugin.
// SPINE V3_03: Add GameplayTags dependency so parkour action
// DataAssets can reference GameplayTag-based states and actions.

using UnrealBuildTool;

public class SOTS_Parkour : ModuleRules
{
    public SOTS_Parkour(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "GameplayTags",
                "Json",
                "JsonUtilities",
                "UMG",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                // Required for USOTS_ParkourSettings : UDeveloperSettings.
                "DeveloperSettings",
                "MotionWarping",
                "OmniTrace",
                "SOTS_TagManager",
            }
        );
    }
}
