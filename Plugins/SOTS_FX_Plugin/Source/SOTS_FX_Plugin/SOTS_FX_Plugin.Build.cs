using UnrealBuildTool;

public class SOTS_FX_Plugin : ModuleRules
{
    public SOTS_FX_Plugin(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "Niagara",
                "GameplayTags",
                "UMG",
                "Slate",
                "SlateCore",
                "AudioMixer",
                "SOTS_TagManager",
                "SOTS_GlobalStealthManager",
                "SOTS_ProfileShared"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Projects",
                "DeveloperSettings"
            }
        );
    }
}
