using UnrealBuildTool;

public class SOTS_BPGen_Bridge : ModuleRules
{
    public SOTS_BPGen_Bridge(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.Editor;

        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Sockets",
            "Networking",
            "Json",
            "JsonUtilities",
            "UnrealEd",
            "SOTS_BlueprintGen"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Projects"
        });
    }
}
