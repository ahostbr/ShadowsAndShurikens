
using UnrealBuildTool;

public class SOTS_KillExecutionManager : ModuleRules
{
    public SOTS_KillExecutionManager(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.NoPCHs;

        PublicIncludePaths.AddRange(
            new string[]
            {
                System.IO.Path.Combine(ModuleDirectory, "Public")
            }
        );

        PrivateIncludePaths.AddRange(
            new string[]
            {
                System.IO.Path.Combine(ModuleDirectory, "Private")
            }
        );

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "DeveloperSettings",
                "LevelSequence",
                "MovieScene",
                "UMG",
                "GameplayTags",
                "ContextualAnimation",
                "OmniTrace",
                "MotionWarping",
                "SOTS_GlobalStealthManager",
                "SOTS_FX_Plugin",
                "SOTS_GAS_Plugin",
                "SOTS_TagManager"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "SOTS_Interaction",
                "SOTS_Core"
            }
        );
    }
}
