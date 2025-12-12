using UnrealBuildTool;

public class BlueprintCommentLinksEditor : ModuleRules
{
    public BlueprintCommentLinksEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "InputCore"
            });

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "UnrealEd",
                "PropertyEditor",
                "AppFramework",
                "Kismet",
                "KismetWidgets",
                "BlueprintGraph",
                "GraphEditor",
                "EditorSubsystem",
                "ApplicationCore",
                "Projects"
            });
    }
}
