using UnrealBuildTool;

public class SOTS_BlueprintGen : ModuleRules
{
    public SOTS_BlueprintGen(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "UnrealEd",
                "Kismet",
                "BlueprintGraph",
                "KismetCompiler",
                "UMG",
                "UMGEditor",
                "GameplayTags",
                "GameplayTagsEditor",
                "AssetTools",
                "EditorSubsystem",
                "EditorFramework",
                "Json",
                "JsonUtilities",
                "Slate",
                "SlateCore",
                "InputCore"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Projects",
                "ApplicationCore",
                "PropertyEditor",
                "ToolMenus",
                "EditorStyle",
                "AssetRegistry",
                "BlueprintGraph",
                "SOTS_BPGen_Bridge"
            }
        );

        PrivateIncludePathModuleNames.AddRange(
            new string[]
            {
                "BlueprintGraph"
            }
        );

        PublicIncludePathModuleNames.AddRange(
            new string[]
            {
                "BlueprintGraph"
            }
        );

        // Additional modules (e.g. commandlets) will be added in later SPINE passes.
    }
}
