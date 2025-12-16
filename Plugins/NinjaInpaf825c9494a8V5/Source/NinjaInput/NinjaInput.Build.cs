// Ninja Bear Studio Inc., all rights reserved.
using UnrealBuildTool;

public class NinjaInput : ModuleRules
{
	public NinjaInput(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(new [] 
		{
			"Core",
			"DeveloperSettings",
			"EnhancedInput",
			"InputCore", 
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks"
		});
		
		PrivateDependencyModuleNames.AddRange(new [] 
		{
			"CoreUObject",
			"Engine"
		});
	}
}
