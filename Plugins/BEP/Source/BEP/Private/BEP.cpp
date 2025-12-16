#include "BEP.h"

#include "BlueprintFlowExporter.h"
#include "ContentBrowserModule.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/IConsoleManager.h"
#include "LevelEditor.h"
#include "Misc/OutputDeviceNull.h"
#include "Misc/Paths.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

#include "BEP_NodeJsonExport.h"
#include "BEP_NodeJsonExportSettings.h"
#include "BEP_NodeJsonTab.h"
#include "BEP_NodeJsonUI.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "HAL/PlatformApplicationMisc.h"
#include "UObject/UnrealType.h"
#include "Framework/Commands/UICommandList.h"
#include "ISettingsModule.h"

#include "Widgets/SBEPExportPanel.h"
#include "BEP_NodeJsonCommands.h"

DEFINE_LOG_CATEGORY(LogBEP);

IMPLEMENT_MODULE(FBEPModule, BEP)

static const FName BEPExportTabName(TEXT("BEPExportTab"));

static FString PresetToString(const EBEP_NodeJsonPreset Preset)
{
	if (const UEnum* Enum = StaticEnum<EBEP_NodeJsonPreset>())
	{
		return Enum->GetNameStringByValue(static_cast<int64>(Preset));
	}
	return TEXT("Unknown");
}

void FBEPModule::StartupModule()
{
	RegisterNodeJsonCommands();
	RegisterSettings();

	// Console command: BEP.ExportAll [RootPath] [OutputDir] [Format]
	// RootPath defaults to /Game
	// OutputDir defaults to <Project>/Saved/BEPExport
	// Format: text, json, csv (defaults to text)
	static FAutoConsoleCommand ExportAllCommand(
		TEXT("BEP.ExportAll"),
		TEXT("Export Blueprint flows, IMCs, DataAssets/DataTables, and struct schemas.\n")
		TEXT("Usage: BEP.ExportAll [RootPath] [OutputDir] [Format]\n")
		TEXT("  RootPath: e.g. /Game, /Game/Abilities (default /Game)\n")
		TEXT("  OutputDir: absolute or relative path (default Saved/BEPExport)\n")
		TEXT("  Format: text | json | csv (default text)"),
		FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
		{
			FString RootPath = TEXT("/Game");
			FString OutputRoot;
			FString FormatStr = TEXT("text");

			if (Args.Num() > 0 && !Args[0].IsEmpty())
			{
				RootPath = Args[0];
			}
			if (Args.Num() > 1 && !Args[1].IsEmpty())
			{
				OutputRoot = Args[1];
			}
			if (Args.Num() > 2 && !Args[2].IsEmpty())
			{
				FormatStr = Args[2];
			}

			if (OutputRoot.IsEmpty())
			{
				OutputRoot = FPaths::ProjectSavedDir() / TEXT("BEPExport");
			}

			const EBEPExportFormat Format = FBlueprintFlowExporter::ParseFormat(FormatStr);
			const TArray<FString> ExcludedPatterns;
			FBlueprintFlowExporter::ExportAll(OutputRoot, RootPath, Format, ExcludedPatterns);
		}));

	// Console command: BEP.ExportFolder <RootPath> [Format] [OutputDir]
	static FAutoConsoleCommand ExportFolderCommand(
		TEXT("BEP.ExportFolder"),
		TEXT("Export Blueprint flows, IMCs, DataAssets/DataTables, and schemas for a specific folder.\n")
		TEXT("Usage: BEP.ExportFolder <RootPath> [Format] [OutputDir]\n")
		TEXT("  RootPath: e.g. /Game/Content/ScanThis\n")
		TEXT("  Format: text | json | csv (default text)\n")
		TEXT("  OutputDir: optional; default Saved/BEPExport/<SanitizedRootPath>"),
		FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
		{
			if (Args.Num() == 0 || Args[0].IsEmpty())
			{
				UE_LOG(LogBEP, Warning, TEXT("BEP.ExportFolder requires at least a RootPath argument."));
				return;
			}

			const FString RootPath = Args[0];

			FString FormatStr = TEXT("text");
			if (Args.Num() > 1 && !Args[1].IsEmpty())
			{
				FormatStr = Args[1];
			}

			FString OutputRoot;
			if (Args.Num() > 2 && !Args[2].IsEmpty())
			{
				OutputRoot = Args[2];
			}
			else
			{
				const FString SanitizedRoot = RootPath.Replace(TEXT("/"), TEXT("_"));
				OutputRoot = FPaths::ProjectSavedDir() / TEXT("BEPExport") / SanitizedRoot;
			}

			const EBEPExportFormat Format = FBlueprintFlowExporter::ParseFormat(FormatStr);
			const TArray<FString> ExcludedPatterns;
			FBlueprintFlowExporter::ExportAll(OutputRoot, RootPath, Format, ExcludedPatterns);
		}));

	RegisterContentBrowserHooks();
	RegisterTabSpawner();
	FBEP_NodeJsonTab::Register();
	RegisterMenus();
}

void FBEPModule::ShutdownModule()
{
	UnregisterSettings();
	UnregisterNodeJsonCommands();

	UnregisterContentBrowserHooks();
	UnregisterTabSpawner();
	FBEP_NodeJsonTab::Unregister();
	MenuRegistrationHandle.Reset();
}

void FBEPModule::RegisterContentBrowserHooks()
{
	FContentBrowserModule& ContentBrowserModule =
		FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	TArray<FContentBrowserMenuExtender_SelectedPaths>& PathExtenders =
		ContentBrowserModule.GetAllPathViewContextMenuExtenders();

	PathExtenders.Add(FContentBrowserMenuExtender_SelectedPaths::CreateRaw(
		this, &FBEPModule::OnExtendContentBrowserPathMenu));

	ContentBrowserPathExtenderHandle = PathExtenders.Last().GetHandle();
}

void FBEPModule::UnregisterContentBrowserHooks()
{
	if (!ContentBrowserPathExtenderHandle.IsValid())
	{
		return;
	}

	FContentBrowserModule* ContentBrowserModule =
		FModuleManager::GetModulePtr<FContentBrowserModule>("ContentBrowser");

	if (!ContentBrowserModule)
	{
		return;
	}

	TArray<FContentBrowserMenuExtender_SelectedPaths>& PathExtenders =
		ContentBrowserModule->GetAllPathViewContextMenuExtenders();

	PathExtenders.RemoveAll([this](const FContentBrowserMenuExtender_SelectedPaths& Delegate)
	{
		return Delegate.GetHandle() == ContentBrowserPathExtenderHandle;
	});

	ContentBrowserPathExtenderHandle.Reset();
}

TSharedRef<FExtender> FBEPModule::OnExtendContentBrowserPathMenu(const TArray<FString>& SelectedPaths)
{
	TSharedRef<FExtender> Extender = MakeShared<FExtender>();

	if (SelectedPaths.Num() > 0)
	{
		Extender->AddMenuExtension(
			TEXT("ContentBrowserFolderOptions"),
			EExtensionHook::After,
			nullptr,
			FMenuExtensionDelegate::CreateLambda([this, SelectedPaths](FMenuBuilder& MenuBuilder)
			{
				const TArray<FString> PathsCopy = SelectedPaths;
				MenuBuilder.AddMenuEntry(
					FText::FromString(TEXT("Export Folder with BEP")),
					FText::FromString(TEXT("Export Blueprints, IMCs, data assets, and schemas under this folder using BEP.")),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([this, PathsCopy]()
					{
						ExecuteExportFolderWithBEP(PathsCopy);
					}))
				);
			}));
	}

	return Extender;
}

void FBEPModule::ExecuteExportFolderWithBEP(const TArray<FString>& SelectedPaths)
{
	if (SelectedPaths.Num() == 0)
	{
		return;
	}

	const EBEPExportFormat Format = EBEPExportFormat::Json;

	for (const FString& Path : SelectedPaths)
	{
		if (Path.IsEmpty())
		{
			continue;
		}

		FString SanitizedPath = Path;
		SanitizedPath.ReplaceInline(TEXT("/"), TEXT("_"));

		const FString OutputRoot = FPaths::ProjectSavedDir() / TEXT("BEPExport") / SanitizedPath;
		const TArray<FString> ExcludedPatterns;
		FBlueprintFlowExporter::ExportAll(OutputRoot, Path, Format, ExcludedPatterns);
	}
}

void FBEPModule::RegisterTabSpawner()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		BEPExportTabName,
		FOnSpawnTab::CreateRaw(this, &FBEPModule::SpawnBEPExportTab))
		.SetDisplayName(FText::FromString(TEXT("BEP Exporter")))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FBEPModule::UnregisterTabSpawner()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(BEPExportTabName);
}

TSharedRef<SDockTab> FBEPModule::SpawnBEPExportTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SBEPExportPanel)
		];
}

void FBEPModule::RegisterMenus()
{
	MenuRegistrationHandle = UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FBEPModule::RegisterMenus_Impl));
}

void FBEPModule::RegisterMenus_Impl()
{
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
	if (!Menu)
	{
		return;
	}

	static const FName SectionName("BEP_Tools");
	FToolMenuSection* Section = Menu->FindSection(SectionName);
	if (!Section)
	{
		Section = &Menu->AddSection(SectionName, FText::FromString(TEXT("BEP Tools")));
	}

	auto AddCommandMenuEntry = [&](const TSharedPtr<FUICommandInfo>& Command)
	{
		FToolMenuEntry& Entry = Section->AddMenuEntry(Command);
		Entry.SetCommandList(NodeJsonCommandList);
	};

	AddCommandMenuEntry(FBEPNodeJsonCommands::Get().OpenPanel);
	AddCommandMenuEntry(FBEPNodeJsonCommands::Get().ExportSelection);
	AddCommandMenuEntry(FBEPNodeJsonCommands::Get().CopySelection);
	AddCommandMenuEntry(FBEPNodeJsonCommands::Get().ExportComments);
	AddCommandMenuEntry(FBEPNodeJsonCommands::Get().ImportComments);
	AddCommandMenuEntry(FBEPNodeJsonCommands::Get().WriteGoldenSamples);
	AddCommandMenuEntry(FBEPNodeJsonCommands::Get().SelfCheck);
}

void FBEPModule::RegisterNodeJsonCommands()
{
	FBEPNodeJsonCommands::Register();
	NodeJsonCommandList = MakeShared<FUICommandList>();
	BindNodeJsonCommands();
}

void FBEPModule::UnregisterNodeJsonCommands()
{
	FBEPNodeJsonCommands::Unregister();
	NodeJsonCommandList.Reset();
}

void FBEPModule::BindNodeJsonCommands()
{
	if (!NodeJsonCommandList.IsValid())
	{
		return;
	}

	NodeJsonCommandList->MapAction(
		FBEPNodeJsonCommands::Get().OpenPanel,
		FExecuteAction::CreateRaw(this, &FBEPModule::ExecuteOpenNodeJsonPanel));

	NodeJsonCommandList->MapAction(
		FBEPNodeJsonCommands::Get().ExportSelection,
		FExecuteAction::CreateRaw(this, &FBEPModule::ExecuteExportNodeJson),
		FCanExecuteAction::CreateRaw(this, &FBEPModule::CanRunNodeJsonSelectionAction));

	NodeJsonCommandList->MapAction(
		FBEPNodeJsonCommands::Get().CopySelection,
		FExecuteAction::CreateRaw(this, &FBEPModule::ExecuteCopyNodeJson),
		FCanExecuteAction::CreateRaw(this, &FBEPModule::CanRunNodeJsonSelectionAction));

	NodeJsonCommandList->MapAction(
		FBEPNodeJsonCommands::Get().ExportComments,
		FExecuteAction::CreateRaw(this, &FBEPModule::ExecuteExportCommentJson),
		FCanExecuteAction::CreateRaw(this, &FBEPModule::CanRunNodeJsonSelectionAction));

	NodeJsonCommandList->MapAction(
		FBEPNodeJsonCommands::Get().ImportComments,
		FExecuteAction::CreateRaw(this, &FBEPModule::ExecuteImportComments),
		FCanExecuteAction::CreateRaw(this, &FBEPModule::CanImportComments));

	NodeJsonCommandList->MapAction(
		FBEPNodeJsonCommands::Get().WriteGoldenSamples,
		FExecuteAction::CreateRaw(this, &FBEPModule::ExecuteWriteGoldenSamples),
		FCanExecuteAction::CreateRaw(this, &FBEPModule::CanRunNodeJsonSelectionAction));

	NodeJsonCommandList->MapAction(
		FBEPNodeJsonCommands::Get().SelfCheck,
		FExecuteAction::CreateRaw(this, &FBEPModule::ExecuteSelfCheck),
		FCanExecuteAction::CreateRaw(this, &FBEPModule::CanRunSelfCheck));
}

void FBEPModule::RegisterSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings(
			"Editor",
			"Plugins",
			"BEPNodeJSON",
			FText::FromString(TEXT("BEP Node JSON")),
			FText::FromString(TEXT("Configure BEP Node JSON export defaults.")),
			GetMutableDefault<UBEP_NodeJsonExportSettings>());
	}
}

void FBEPModule::UnregisterSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Editor", "Plugins", "BEPNodeJSON");
	}
}

void FBEPModule::ExecuteOpenNodeJsonPanel()
{
	FGlobalTabmanager::Get()->TryInvokeTab(FBEP_NodeJsonTab::TabId);
}

void FBEPModule::ExecuteExportNodeJson()
{
	FBEP_NodeJsonExportOptions Opt = BEPNodeJsonUI::BuildOptionsFromSettings();

	FString Json;
	FString Stem;
	FString Error;
	int32 NodeCount = 0;
	int32 EdgeCount = 0;
	FBEP_NodeJsonExportStats Stats;
	if (!BEP_NodeJson::ExportActiveSelectionToJson(Opt, Json, Stem, NodeCount, EdgeCount, &Error, &Stats, false))
	{
		const FString Reason = Error.IsEmpty() ? TEXT("Unknown error") : Error;
		UE_LOG(LogBEP, Warning, TEXT("[BEP][NodeJSON] Export failed: %s"), *Reason);
		BEPNodeJsonUI::ShowNodeJsonToast(FString::Printf(TEXT("Export failed: %s"), *Reason), false);
		return;
	}

	FString SavedPath;
	FString SummaryPath;
	if (!BEP_NodeJson::SaveJsonToFile(Json, Stem, Opt, &Stats, SavedPath, &SummaryPath, &Error, false))
	{
		const FString Reason = Error.IsEmpty() ? TEXT("Save failed") : Error;
		UE_LOG(LogBEP, Warning, TEXT("[BEP][NodeJSON] Save failed: %s"), *Reason);
		BEPNodeJsonUI::ShowNodeJsonToast(FString::Printf(TEXT("Save failed: %s"), *Reason), false);
		return;
	}

	UE_LOG(LogBEP, Log, TEXT("[BEP][NodeJSON] Saved: %s (Nodes=%d Edges=%d Preset=%s)"), *SavedPath, NodeCount, EdgeCount, *PresetToString(Opt.Preset));
	if (!SummaryPath.IsEmpty())
	{
		UE_LOG(LogBEP, Log, TEXT("[BEP][NodeJSON] Summary: %s"), *SummaryPath);
	}
	BEPNodeJsonUI::ShowNodeJsonToast(FString::Printf(TEXT("Saved (%d nodes, %d edges): %s"), NodeCount, EdgeCount, *SavedPath), true);
}

void FBEPModule::ExecuteCopyNodeJson()
{
	FBEP_NodeJsonExportOptions Opt = BEPNodeJsonUI::BuildOptionsFromSettings();
	FString Json;
	FString Stem;
	FString Error;
	int32 NodeCount = 0;
	int32 EdgeCount = 0;
	FBEP_NodeJsonExportStats Stats;
	if (!BEP_NodeJson::ExportActiveSelectionToJson(Opt, Json, Stem, NodeCount, EdgeCount, &Error, &Stats, false))
	{
		const FString Reason = Error.IsEmpty() ? TEXT("Unknown error") : Error;
		UE_LOG(LogBEP, Warning, TEXT("[BEP][NodeJSON] Copy failed: %s"), *Reason);
		BEPNodeJsonUI::ShowNodeJsonToast(FString::Printf(TEXT("Copy failed: %s"), *Reason), false);
		return;
	}

	FPlatformApplicationMisc::ClipboardCopy(*Json);
	UE_LOG(LogBEP, Log, TEXT("[BEP][NodeJSON] Copied to clipboard (Nodes=%d Edges=%d Preset=%s)"), NodeCount, EdgeCount, *PresetToString(Opt.Preset));
	BEPNodeJsonUI::ShowNodeJsonToast(FString::Printf(TEXT("Copied Node JSON (%d nodes, %d edges)"), NodeCount, EdgeCount), true);
}

void FBEPModule::ExecuteExportCommentJson()
{
	FBEP_NodeJsonExportOptions Opt = BEPNodeJsonUI::BuildOptionsFromSettings();
	FString Json;
	FString Stem;
	FString Error;
	if (!BEP_NodeJson::ExportActiveSelectionToCommentJson(Opt, Json, Stem, nullptr, &Error))
	{
		const FString Reason = Error.IsEmpty() ? TEXT("Unknown error") : Error;
		UE_LOG(LogBEP, Warning, TEXT("[BEP][NodeJSON] Comment export failed: %s"), *Reason);
		BEPNodeJsonUI::ShowNodeJsonToast(FString::Printf(TEXT("Comment export failed: %s"), *Reason), false);
		return;
	}

	FString SavedPath;
	if (!BEP_NodeJson::SaveCommentJsonToFile(Json, Stem, Opt, SavedPath, nullptr, &Error, nullptr, false))
	{
		const FString Reason = Error.IsEmpty() ? TEXT("Save failed") : Error;
		UE_LOG(LogBEP, Warning, TEXT("[BEP][NodeJSON] Comment save failed: %s"), *Reason);
		BEPNodeJsonUI::ShowNodeJsonToast(FString::Printf(TEXT("Comment save failed: %s"), *Reason), false);
		return;
	}

	UE_LOG(LogBEP, Log, TEXT("[BEP][NodeJSON] Comment JSON saved: %s"), *SavedPath);
	BEPNodeJsonUI::ShowNodeJsonToast(TEXT("Saved Comment JSON"), true);
}

void FBEPModule::ExecuteImportComments()
{
	BEPNodeJsonUI::ShowNodeJsonToast(TEXT("Use the panel Import button for CSV import."), false);
	FGlobalTabmanager::Get()->TryInvokeTab(FBEP_NodeJsonTab::TabId);
}

void FBEPModule::ExecuteWriteGoldenSamples()
{
	FString SummaryPath;
	FString SuperPath;
	FString AuditPath;
	FString CommentPath;
	FString TemplatePath;
	FString Error;
	if (!BEP_NodeJson::WriteGoldenSamples(SummaryPath, &Error, &SuperPath, &AuditPath, &CommentPath, &TemplatePath))
	{
		const FString Reason = Error.IsEmpty() ? TEXT("Unknown error") : Error;
		UE_LOG(LogBEP, Warning, TEXT("[BEP][NodeJSON] Golden samples failed: %s"), *Reason);
		BEPNodeJsonUI::ShowNodeJsonToast(FString::Printf(TEXT("Golden samples failed: %s"), *Reason), false);
		return;
	}

	UE_LOG(LogBEP, Log, TEXT("[BEP][NodeJSON] Golden samples saved. Super=%s Audit=%s Comments=%s Template=%s Summary=%s"), *SuperPath, *AuditPath, *CommentPath, *TemplatePath, *SummaryPath);
	BEPNodeJsonUI::ShowNodeJsonToast(TEXT("Golden samples written"), true);
}

void FBEPModule::ExecuteSelfCheck()
{
	FString Report;
	const bool bOk = BEP_NodeJson::RunSelfCheck(Report);
	UE_LOG(LogBEP, Log, TEXT("%s"), *Report);
	BEPNodeJsonUI::ShowNodeJsonToast(bOk ? TEXT("Node JSON self-check passed") : TEXT("Node JSON self-check found issues"), bOk);
}

bool FBEPModule::CanRunNodeJsonSelectionAction() const
{
	return BEP_NodeJson::HasActiveBlueprintSelection(nullptr);
}

bool FBEPModule::CanImportComments() const
{
	return BEP_NodeJson::HasActiveBlueprintGraph(nullptr);
}

bool FBEPModule::CanRunSelfCheck() const
{
	return true;
}
