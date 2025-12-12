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

#include "Widgets/SBEPExportPanel.h"

DEFINE_LOG_CATEGORY(LogBEP);

IMPLEMENT_MODULE(FBEPModule, BEP)

static const FName BEPExportTabName(TEXT("BEPExportTab"));

void FBEPModule::StartupModule()
{
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
	RegisterMenus();
}

void FBEPModule::ShutdownModule()
{
	UnregisterContentBrowserHooks();
	UnregisterTabSpawner();
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

	static const FName SectionName("SOTS_Tools");
	FToolMenuSection* Section = Menu->FindSection(SectionName);
	if (!Section)
	{
		Section = &Menu->AddSection(SectionName, FText::FromString(TEXT("SOTS Tools")));
	}

	Section->AddMenuEntry(
		"OpenBEPExporter",
		FText::FromString(TEXT("BEP Exporter")),
		FText::FromString(TEXT("Open the BEP Exporter panel.")),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(BEPExportTabName);
		})));
}
