// Created by Satheesh (ryanjon2040). Copyright 2024.

using UnrealBuildTool;

public class BlueprintSubsystemEditor : ModuleRules
{
    public BlueprintSubsystemEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core" });
        PrivateDependencyModuleNames.AddRange(new string[] { "CoreUObject", "Engine",
            "UnrealEd",
            "BlueprintSubsystem" 
        });
    }
}