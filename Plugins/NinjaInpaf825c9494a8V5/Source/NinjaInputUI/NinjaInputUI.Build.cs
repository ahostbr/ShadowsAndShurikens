// Ninja Bear Studio Inc., all rights reserved.
// ReSharper disable InconsistentNaming
using UnrealBuildTool;

public class NinjaInputUI : ModuleRules
{
    public NinjaInputUI(ReadOnlyTargetRules target) : base(target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new []
        {
            "CommonInput",
            "CommonUI",
            "Core",
            "EnhancedInput",
            "InputCore",
            "ModelViewViewModel",
            "NinjaInput"
        });

        PrivateDependencyModuleNames.AddRange(new []
        {
            "CoreUObject",
            "DeveloperSettings",
            "Engine",
            "UMG",
            "Slate",
            "SlateCore"
        });
    }
}