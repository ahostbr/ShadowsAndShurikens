using UnrealBuildTool;

public class SOTS_BPGen_Bridge : ModuleRules
{
    public SOTS_BPGen_Bridge(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleRules.ModuleType.CPlusPlus;

        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "AssetRegistry",
            "AssetTools",
            "BlueprintGraph",
            "EditorScriptingUtilities",
            "ImageWrapper",
            "Sockets",
            "Networking",
            "Json",
            "JsonUtilities",
            "UnrealEd",
            "Slate",
            "SlateCore",
            "ToolMenus",
            "SOTS_BlueprintGen"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Projects"
        });
    }
}
