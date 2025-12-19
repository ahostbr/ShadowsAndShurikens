#include "SOTS_BlueprintGen.h"
#include "SOTS_BPGenTypes.h"
#include "SSOTS_BPGenRunner.h"
#include "SSOTS_BPGenControlCenter.h"

#include "Framework/Commands/UIAction.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Framework/Docking/TabManager.h"

DEFINE_LOG_CATEGORY(LogSOTS_BlueprintGen);

#define LOCTEXT_NAMESPACE "FSOTS_BlueprintGenModule"

namespace
{
	const FName SOTS_BPGenRunnerTabName(TEXT("SOTS_BPGenRunner"));
	const FName SOTS_BPGenControlCenterTabName(TEXT("SOTS_BPGenControlCenter"));
}

void FSOTS_BlueprintGenModule::StartupModule()
{
	UE_LOG(LogSOTS_BlueprintGen, Log, TEXT("SOTS_BlueprintGen module has started (SPINE 1)."));

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			SOTS_BPGenRunnerTabName,
			FOnSpawnTab::CreateRaw(this, &FSOTS_BlueprintGenModule::SpawnBPGenRunnerTab))
		.SetDisplayName(LOCTEXT("BPGenRunnerTabTitle", "SOTS BPGen Runner"))
		.SetTooltipText(LOCTEXT("BPGenRunnerTabTooltip", "Run SOTS BPGen jobs inside the editor."))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			SOTS_BPGenControlCenterTabName,
			FOnSpawnTab::CreateRaw(this, &FSOTS_BlueprintGenModule::SpawnBPGenControlCenterTab))
		.SetDisplayName(LOCTEXT("BPGenControlCenterTabTitle", "SOTS BPGen Control Center"))
		.SetTooltipText(LOCTEXT("BPGenControlCenterTabTooltip", "Operate BPGen bridge, jobs, and debug tooling."))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	if (UToolMenus::IsToolMenuUIEnabled())
	{
		MenuRegistrationHandle = UToolMenus::RegisterStartupCallback(
			FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSOTS_BlueprintGenModule::RegisterMenus));
	}
}

void FSOTS_BlueprintGenModule::ShutdownModule()
{
	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::UnRegisterStartupCallback(MenuRegistrationHandle);
	}

	if (UToolMenus::IsToolMenuUIEnabled())
	{
		if (UToolMenus* ToolMenus = UToolMenus::Get())
		{
			ToolMenus->UnregisterOwner(this);
		}
	}

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SOTS_BPGenRunnerTabName);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SOTS_BPGenControlCenterTabName);
	UE_LOG(LogSOTS_BlueprintGen, Log, TEXT("SOTS_BlueprintGen module is shutting down."));
}

TSharedRef<SDockTab> FSOTS_BlueprintGenModule::SpawnBPGenRunnerTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SSOTS_BPGenRunner)
		];
}

TSharedRef<SDockTab> FSOTS_BlueprintGenModule::SpawnBPGenControlCenterTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SSOTS_BPGenControlCenter)
		];
}

void FSOTS_BlueprintGenModule::RegisterMenus()
{
	UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
	if (!WindowMenu)
	{
		return;
	}

	FToolMenuSection* Section = WindowMenu->FindSection("SOTS_Tools");
	if (!Section)
	{
		Section = &WindowMenu->AddSection("SOTS_Tools", FText::FromString(TEXT("SOTS Tools")));
	}
	const FText Label = LOCTEXT("BPGenRunnerMenuLabel", "SOTS BPGen Runner");
	const FText Tooltip = LOCTEXT("BPGenRunnerMenuTooltip", "Open the SOTS BPGen Runner tab.");

	Section->AddMenuEntry(
		"OpenSOTS_BPGenRunner",
		Label,
		Tooltip,
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(SOTS_BPGenRunnerTabName);
		})));

	const FText ControlLabel = LOCTEXT("BPGenControlCenterMenuLabel", "SOTS BPGen Control Center");
	const FText ControlTooltip = LOCTEXT("BPGenControlCenterMenuTooltip", "Open the BPGen Control Center.");

	Section->AddMenuEntry(
		"OpenSOTS_BPGenControlCenter",
		ControlLabel,
		ControlTooltip,
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(SOTS_BPGenControlCenterTabName);
		})));
}

#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FSOTS_BlueprintGenModule, SOTS_BlueprintGen)
