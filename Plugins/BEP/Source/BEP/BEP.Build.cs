using UnrealBuildTool;

public class BEP : ModuleRules
{
    public BEP(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "UnrealEd",
                "AssetRegistry",
                "Kismet",
                "Json",
                "JsonUtilities",
                "InputCore",
                "ApplicationCore",
                // IMCs live in EnhancedInput; if the plugin is disabled this module just won't load.
                "EnhancedInput"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Slate",
                "SlateCore",
                "BlueprintGraph",
                "ContentBrowser",
                "ToolMenus",
                "GraphEditor",
                "EditorStyle",
                "LevelEditor",
                "PropertyEditor",
                "DesktopPlatform",
                "Settings"
            }
        );
    }
}

