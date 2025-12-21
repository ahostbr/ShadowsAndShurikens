#include "SOTS_BlueprintGen.h"
#include "SOTS_BPGenTypes.h"
#include "SSOTS_BPGenRunner.h"

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

}

#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FSOTS_BlueprintGenModule, SOTS_BlueprintGen)
