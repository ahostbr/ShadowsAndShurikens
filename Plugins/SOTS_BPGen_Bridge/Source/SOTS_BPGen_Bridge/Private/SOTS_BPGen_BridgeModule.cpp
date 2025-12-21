#include "SOTS_BPGen_BridgeModule.h"
#include "SOTS_BPGenBridgeServer.h"
#include "SSOTS_BPGenControlCenter.h"

#include "Framework/Commands/UIAction.h"
#include "Framework/Docking/TabManager.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FSOTS_BPGen_BridgeModule"

static TWeakPtr<FSOTS_BPGenBridgeServer> GSharedBridgeServer;
namespace
{
	const FName SOTS_BPGenControlCenterTabName(TEXT("SOTS_BPGenControlCenter"));
}

void FSOTS_BPGen_BridgeModule::StartupModule()
{
	Server = MakeShared<FSOTS_BPGenBridgeServer>();
	GSharedBridgeServer = Server;

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		SOTS_BPGenControlCenterTabName,
		FOnSpawnTab::CreateRaw(this, &FSOTS_BPGen_BridgeModule::SpawnBPGenControlCenterTab))
		.SetDisplayName(LOCTEXT("BPGenControlCenterTabTitle", "SOTS BPGen Control Center"))
		.SetTooltipText(LOCTEXT("BPGenControlCenterTabTooltip", "Operate BPGen bridge, jobs, and debug tooling."))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	if (UToolMenus::IsToolMenuUIEnabled())
	{
		MenuRegistrationHandle = UToolMenus::RegisterStartupCallback(
			FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSOTS_BPGen_BridgeModule::RegisterMenus));
	}

	if (Server.IsValid())
	{
		Server->Start();
	}
}

void FSOTS_BPGen_BridgeModule::ShutdownModule()
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

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SOTS_BPGenControlCenterTabName);

	if (Server.IsValid())
	{
		Server->Stop();
		Server.Reset();
	}

	GSharedBridgeServer.Reset();
}

FSOTS_BPGenBridgeServer* FSOTS_BPGen_BridgeModule::GetServer()
{
	if (GSharedBridgeServer.IsValid())
	{
		return GSharedBridgeServer.Pin().Get();
	}

	return nullptr;
}

TSharedPtr<FSOTS_BPGenBridgeServer> FSOTS_BPGen_BridgeModule::GetServerShared()
{
	return GSharedBridgeServer.Pin();
}

TSharedRef<SDockTab> FSOTS_BPGen_BridgeModule::SpawnBPGenControlCenterTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SSOTS_BPGenControlCenter)
		];
}

void FSOTS_BPGen_BridgeModule::RegisterMenus()
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

IMPLEMENT_MODULE(FSOTS_BPGen_BridgeModule, SOTS_BPGen_Bridge)
