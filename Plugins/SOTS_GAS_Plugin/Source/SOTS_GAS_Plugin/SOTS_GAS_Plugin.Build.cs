// Copyright (c) 2025 USP45Master. All rights reserved.
using System.IO;
using UnrealBuildTool;

public class SOTS_GAS_Plugin : ModuleRules
{
    public SOTS_GAS_Plugin(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "UMG",
                "GameplayTags",
                "Niagara",
                "SOTS_GlobalStealthManager",
                "SOTS_SkillTree",
                "SOTS_AIPerception",
                "SOTS_FX_Plugin",
                "SOTS_TagManager",
                "SOTS_UI",
                "SOTS_INV",
                "SOTS_ProfileShared"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Projects",
                "DeveloperSettings",
                "SOTS_Core"
            }
        );

        PublicIncludePaths.AddRange(
            new string[]
            {
                Path.Combine(ModuleDirectory, "Public")
            }
        );

        PrivateIncludePaths.AddRange(
            new string[]
            {
                Path.Combine(ModuleDirectory, "Private")
            }
        );
    }
}
