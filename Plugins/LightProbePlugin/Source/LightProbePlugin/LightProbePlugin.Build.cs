
using UnrealBuildTool;

public class LightProbePlugin : ModuleRules
{
    public LightProbePlugin(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "UMG",
                "SOTS_GlobalStealthManager"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "DeveloperSettings",
                "RenderCore",
                "RHI",
                "Slate",
                "SlateCore",
                "SOTS_Core"
            }
        );
    }
}
