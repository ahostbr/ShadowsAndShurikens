// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SOTS_AIPerception : ModuleRules
{
    public SOTS_AIPerception(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
        PublicIncludePaths.AddRange(
            new string[] {
                // ... add public include paths required here ...
            }
        );
                
        PrivateIncludePaths.AddRange(
            new string[] {
                System.IO.Path.Combine(ModuleDirectory, "Private")
            }
        );
            
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "GameplayTags",
                "AIModule",
                "SOTS_TagManager",
                "SOTS_ProfileShared",
                "SOTS_FX_Plugin",
                "SOTS_UDSBridge"
            }
        );
        
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "SOTS_GlobalStealthManager"
            }
        );
        
        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
        );
    }
}
