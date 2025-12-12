// Copyright(c) Aurora Devs 2024. All Rights Reserved.

using UnrealBuildTool;

public class AuroraDevs_UGC : ModuleRules
{
    public AuroraDevs_UGC(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        MinFilesUsingPrecompiledHeaderOverride = 1;
        bUseUnity = false;

        PrivateIncludePaths.Add("AuroraDevs_UGC");
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "EnhancedInput",
            "GameplayCameras",
            "InputCore" ,
            "LevelSequence",
            "MovieScene",
            "MovieSceneTracks",
            "TemplateSequence"
        });
    }
}
