using UnrealBuildTool;

public class SOTS_BodyDrag : ModuleRules
{
    public SOTS_BodyDrag(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "SOTS_Interaction"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "SOTS_TagManager"
        });
    }
}
