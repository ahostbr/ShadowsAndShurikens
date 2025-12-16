#include "Widgets/SBEPExportPanel.h"

#include "BEP.h"
#include "BEPExportSettings.h"
#include "BlueprintFlowExporter.h"

#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "String/LexFromString.h"

namespace
{
static const TCHAR* const ConfigSection = TEXT("BEP.ExportPanel");
static const TCHAR* const FileTypeExcludeKey = TEXT("FileTypeExcludeConfig");
static const TCHAR* const DefaultFileTypeExcludes = TEXT("uasset,umap,uexp,ubulk,utoc,ucas");
}

void SBEPExportPanel::Construct(const FArguments& InArgs)
{
	if (ExportSettings.OutputRootPath.IsEmpty())
	{
		ExportSettings.OutputRootPath = GetDefaultBEPExportRoot();
	}

	InitializeFormatOptions();
	LoadPersistedSettings();

	for (const TSharedPtr<FString>& Option : FormatOptions)
	{
		if (*Option == FormatToString(ExportSettings.OutputFormat))
		{
			SelectedFormatItem = Option;
			break;
		}
	}

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.f)
			[
				SNew(SButton)
				.OnClicked(FOnClicked::CreateSP(this, &SBEPExportPanel::OnTabSelected, 0))
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Export Settings")))
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.f)
			[
				SNew(SButton)
				.OnClicked(FOnClicked::CreateSP(this, &SBEPExportPanel::OnTabSelected, 1))
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Filetype Excludes")))
				]
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		.Padding(2.f)
		[
			SNew(SWidgetSwitcher)
			.WidgetIndex_Lambda([this]() { return GetActiveTabIndex(); })
			+ SWidgetSwitcher::Slot()
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)

					// Root Path
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Root Content Path (/Game/...)")))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.f)
					[
						SNew(SEditableTextBox)
						.Text(this, &SBEPExportPanel::GetRootPathText)
						.OnTextCommitted(this, &SBEPExportPanel::OnRootPathCommitted)
					]

					// Output Root Path
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Output Root (disk) — absolute path OR subfolder name")))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.f)
					[
						SNew(SEditableTextBox)
						.Text(this, &SBEPExportPanel::GetOutputRootText)
						.OnTextCommitted(this, &SBEPExportPanel::OnOutputRootCommitted)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]() { return GetResolvedOutputRootText(); })
					]

					// Output Format
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Output Format")))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.f)
					[
						SNew(SComboBox<TSharedPtr<FString>>)
						.OptionsSource(&FormatOptions)
						.OnGenerateWidget(this, &SBEPExportPanel::OnGenerateFormatWidget)
						.OnSelectionChanged(this, &SBEPExportPanel::OnFormatChanged)
						.InitiallySelectedItem(SelectedFormatItem)
						[
							SNew(STextBlock)
							.Text(this, &SBEPExportPanel::GetCurrentFormatLabel)
						]
					]

					// Blueprint flow layout settings
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.f)
					[
						SNew(SCheckBox)
						.IsChecked_Lambda([this]()
						{
							return ExportSettings.bOrganizeBlueprintFlowsByPackagePath ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
						})
						.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
						{
							ExportSettings.bOrganizeBlueprintFlowsByPackagePath = (NewState == ECheckBoxState::Checked);
						})
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Organize Blueprint flows by package path (recommended)")))
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.f)
					[
						SNew(SCheckBox)
						.IsChecked_Lambda([this]()
						{
							return ExportSettings.bUseTimestampRunFolder ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
						})
						.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
						{
							ExportSettings.bUseTimestampRunFolder = (NewState == ECheckBoxState::Checked);
						})
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Use timestamped run folder")))
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.f)
					[
						SNew(SCheckBox)
						.IsChecked_Lambda([this]()
						{
							return ExportSettings.bUseLegacyFlatBlueprintFlowsLayout ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
						})
						.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
						{
							const bool bLegacy = (NewState == ECheckBoxState::Checked);
							ExportSettings.bUseLegacyFlatBlueprintFlowsLayout = bLegacy;
							if (bLegacy)
							{
								ExportSettings.bUseTimestampRunFolder = false;
							}
						})
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Use legacy flat BlueprintFlows layout")))
						]
					]

					// Excluded Types
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Class / folder fragments to exclude (comma or newline separated)")))
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.f)
					.Padding(4.f)
					[
						SNew(SMultiLineEditableTextBox)
						.Text(this, &SBEPExportPanel::GetExcludedTypesText)
						.OnTextCommitted(this, &SBEPExportPanel::OnExcludedTypesCommitted)
					]

					// Max assets per run
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Max Assets Per Run (safety limit)")))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.f)
					[
						SNew(SEditableTextBox)
						.Text_Lambda([this]()
						{
							return FText::AsNumber(ExportSettings.MaxAssetsPerRun);
						})
						.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type)
						{
							int32 Value = ExportSettings.MaxAssetsPerRun;
							LexTryParseString<int32>(Value, *NewText.ToString());
							ExportSettings.MaxAssetsPerRun = FMath::Clamp(Value, 1, 1'000'000);
						})
					]

					// Run Export Button
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.f)
					.HAlign(HAlign_Right)
					[
						SNew(SButton)
						.Text(FText::FromString(TEXT("Run BEP Export")))
						.OnClicked(this, &SBEPExportPanel::OnRunExportClicked)
					]
				]
			]
			+ SWidgetSwitcher::Slot()
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Filetype extensions to exclude (comma or newline separated)")))
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.f)
					.Padding(4.f)
					[
						SNew(SMultiLineEditableTextBox)
						.Text(this, &SBEPExportPanel::GetFileTypeExcludeText)
						.OnTextCommitted(this, &SBEPExportPanel::OnFileTypeExcludeCommitted)
					]
				]
			]
		]
	];
}

void SBEPExportPanel::SetRootPath(const FString& InRootPath)
{
	ExportSettings.RootPath = InRootPath;
}

void SBEPExportPanel::SetOutputRootPath(const FString& InOutputRootPath)
{
	FString Clean = InOutputRootPath;
	Clean.TrimStartAndEndInline();
	if (Clean.IsEmpty())
	{
		Clean = GetDefaultBEPExportRoot();
	}

	ExportSettings.OutputRootPath = Clean;
}

FText SBEPExportPanel::GetRootPathText() const
{
	return FText::FromString(ExportSettings.RootPath);
}

void SBEPExportPanel::OnRootPathCommitted(const FText& NewText, ETextCommit::Type)
{
	ExportSettings.RootPath = NewText.ToString();
}

FText SBEPExportPanel::GetOutputRootText() const
{
	const FString Resolved = ExportSettings.OutputRootPath.IsEmpty()
		? GetDefaultBEPExportRoot()
		: ExportSettings.OutputRootPath;
	return FText::FromString(Resolved);
}

FText SBEPExportPanel::GetResolvedOutputRootText() const
{
	FString Clean = ExportSettings.OutputRootPath;
	Clean.ReplaceInline(TEXT("\""), TEXT(""));
	Clean.TrimStartAndEndInline();

	const FString Base = GetDefaultBEPExportRoot();
	FString Resolved;
	if (Clean.IsEmpty())
	{
		Resolved = Base;
	}
	else if (FPaths::IsRelative(Clean))
	{
		Resolved = FPaths::Combine(Base, Clean);
	}
	else
	{
		Resolved = Clean;
	}

	return FText::FromString(FString::Printf(TEXT("Resolved Output: %s"), *Resolved));
}

void SBEPExportPanel::OnOutputRootCommitted(const FText& NewText, ETextCommit::Type)
{
	FString NewPath = NewText.ToString();
	NewPath.TrimStartAndEndInline();
	if (NewPath.IsEmpty())
	{
		NewPath = GetDefaultBEPExportRoot();
	}
	ExportSettings.OutputRootPath = NewPath;
}

void SBEPExportPanel::InitializeFormatOptions()
{
	FormatOptions.Empty();
	FormatOptions.Add(MakeShared<FString>(TEXT("Text")));
	FormatOptions.Add(MakeShared<FString>(TEXT("JSON")));
	FormatOptions.Add(MakeShared<FString>(TEXT("CSV")));
	FormatOptions.Add(MakeShared<FString>(TEXT("All")));
}

TSharedRef<SWidget> SBEPExportPanel::OnGenerateFormatWidget(TSharedPtr<FString> InItem) const
{
	return SNew(STextBlock).Text(FText::FromString(*InItem));
}

void SBEPExportPanel::OnFormatChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type)
{
	SelectedFormatItem = NewSelection;
	if (NewSelection.IsValid())
	{
		ExportSettings.OutputFormat = StringToFormat(*NewSelection);
	}
}

FText SBEPExportPanel::GetCurrentFormatLabel() const
{
	if (SelectedFormatItem.IsValid())
	{
		return FText::FromString(*SelectedFormatItem);
	}
	return FText::FromString(FormatToString(ExportSettings.OutputFormat));
}

FText SBEPExportPanel::GetExcludedTypesText() const
{
	return FText::FromString(ExportSettings.ExcludedClassPatterns);
}

void SBEPExportPanel::OnExcludedTypesCommitted(const FText& NewText, ETextCommit::Type)
{
	ExportSettings.ExcludedClassPatterns = NewText.ToString();
}

FText SBEPExportPanel::GetFileTypeExcludeText() const
{
	return FText::FromString(FileTypeExcludeConfig);
}

void SBEPExportPanel::OnFileTypeExcludeCommitted(const FText& NewText, ETextCommit::Type)
{
	FileTypeExcludeConfig = NewText.ToString();
	SaveFileTypeExcludeConfig();
}

void SBEPExportPanel::LoadPersistedSettings()
{
	if (!GConfig)
	{
		return;
	}

	FString Saved;
	if (GConfig->GetString(ConfigSection, FileTypeExcludeKey, Saved, GEditorPerProjectIni))
	{
		FString Trimmed = Saved;
		Trimmed.TrimStartAndEndInline();
		FileTypeExcludeConfig = Trimmed.IsEmpty() ? DefaultFileTypeExcludes : Trimmed;
	}
	else
	{
		FileTypeExcludeConfig = DefaultFileTypeExcludes;
	}
}

void SBEPExportPanel::SaveFileTypeExcludeConfig()
{
	if (!GConfig)
	{
		return;
	}

	GConfig->SetString(ConfigSection, FileTypeExcludeKey, *FileTypeExcludeConfig, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

FReply SBEPExportPanel::OnTabSelected(int32 TabIndex)
{
	ActiveTabIndex = TabIndex;
	return FReply::Handled();
}

EBEPExportOutputFormat SBEPExportPanel::StringToFormat(const FString& FormatString) const
{
	if (FormatString.Equals(TEXT("Text"), ESearchCase::IgnoreCase))
	{
		return EBEPExportOutputFormat::Text;
	}
	if (FormatString.Equals(TEXT("JSON"), ESearchCase::IgnoreCase))
	{
		return EBEPExportOutputFormat::Json;
	}
	if (FormatString.Equals(TEXT("CSV"), ESearchCase::IgnoreCase))
	{
		return EBEPExportOutputFormat::Csv;
	}
	return EBEPExportOutputFormat::All;
}

FString SBEPExportPanel::FormatToString(EBEPExportOutputFormat Format) const
{
	switch (Format)
	{
	case EBEPExportOutputFormat::Text:
		return TEXT("Text");
	case EBEPExportOutputFormat::Json:
		return TEXT("JSON");
	case EBEPExportOutputFormat::Csv:
		return TEXT("CSV");
	case EBEPExportOutputFormat::All:
	default:
		return TEXT("All");
	}
}

FReply SBEPExportPanel::OnRunExportClicked()
{
	const FString TrimmedRoot = ExportSettings.RootPath.TrimStartAndEnd();
	if (TrimmedRoot.Equals(TEXT("/Game"), ESearchCase::IgnoreCase) ||
		TrimmedRoot.Equals(TEXT("/Game/"), ESearchCase::IgnoreCase))
	{
		UE_LOG(LogBEP, Warning, TEXT("BEP: Exporting entire /Game can be heavy. Consider narrowing RootPath or lowering MaxAssetsPerRun."));
	}

	FBlueprintFlowExporter::ExportWithSettings(ExportSettings);
	return FReply::Handled();
}
