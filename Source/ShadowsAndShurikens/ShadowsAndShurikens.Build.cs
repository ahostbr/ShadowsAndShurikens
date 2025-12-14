// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class ShadowsAndShurikens : ModuleRules
{
	public ShadowsAndShurikens(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"GameplayTags",
			"UMG",
			"Slate",
			"SlateCore",
			"AnimationWarpingRuntime",
			"AnimationLocomotionLibraryRuntime",
			"SOTS_ProfileShared",
			"SOTS_TagManager",
			"SOTS_INV",
			"SOTS_GlobalStealthManager",
			"SOTS_GAS_Plugin",
			"SOTS_Stats",
			"SOTS_MMSS",
			"SOTS_FX_Plugin",
			"SOTS_MissionDirector",
			"SOTS_KillExecutionManager",
			"SOTS_Debug",
			"SOTS_Parkour",
			"SOTS_UI"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AnimGraphRuntime",
			"PoseSearch",
			"BlendStack",
			"Chooser",
			"EngineSettings"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
