using System.IO;
using UnrealBuildTool;

public class SOTS_EdUtil : ModuleRules
{
    public SOTS_EdUtil(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Json",
            "JsonUtilities",
            "Slate",
            "SlateCore",
            "PropertyEditor",
            "AssetRegistry",
            "DesktopPlatform",
            "UnrealEd"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "ToolMenus"
        });

        PrivateIncludePaths.AddRange(new string[]
        {
            Path.Combine(EngineDirectory, "Source", "Runtime", "Slate", "Public"),
            Path.Combine(EngineDirectory, "Source", "Runtime", "SlateCore", "Public")
        });
    }
}
