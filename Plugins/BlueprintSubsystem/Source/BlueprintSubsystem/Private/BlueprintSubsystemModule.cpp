// Created by Satheesh (ryanjon2040). Copyright 2024.

#include "BlueprintSubsystemModule.h"

#define LOCTEXT_NAMESPACE "FBlueprintSubsystemModule"

DEFINE_LOG_CATEGORY_STATIC(LogBlueprintSubsystemModule, All, All);

void FBlueprintSubsystemModule::StartupModule()
{
	UE_LOG(LogBlueprintSubsystemModule, Log, TEXT("Blueprint Subsystem Module started..."));
}

void FBlueprintSubsystemModule::ShutdownModule()
{
	UE_LOG(LogBlueprintSubsystemModule, Log, TEXT("Blueprint Subsystem Module shutdown..."));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FBlueprintSubsystemModule, BlueprintSubsystem)
