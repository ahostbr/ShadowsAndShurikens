// Copyright(c) Aurora Devs 2024. All Rights Reserved.

using UnrealBuildTool;

public class AuroraDevs_UGCEditor : ModuleRules
{
    public AuroraDevs_UGCEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        ShadowVariableWarningLevel = WarningLevel.Error;
        bUseUnity = false;
        bEnforceIWYU = true;

        PrivateIncludePaths.Add(ModuleDirectory);

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "GameplayCameras",
                "BlueprintGraph",
                "AuroraDevs_UGC"
            });
    }
}
