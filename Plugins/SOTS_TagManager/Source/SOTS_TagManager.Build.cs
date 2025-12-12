using UnrealBuildTool;
using System.IO;

public class SOTS_TagManager : ModuleRules
{
    public SOTS_TagManager(ReadOnlyTargetRules Target) : base(Target)
    {
        // Keep this module minimal: only core engine modules + GameplayTags
        // so TagManager stays runtime-only and does not pull in Editor or UI modules.
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

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
            }
        );

        PublicIncludePaths.AddRange(
            new string[]
            {
                Path.Combine(ModuleDirectory, "SOTS_TagManager", "Public")
            }
        );

        PrivateIncludePaths.AddRange(
            new string[]
            {
                Path.Combine(ModuleDirectory, "SOTS_TagManager", "Private")
            }
        );
    }
}

