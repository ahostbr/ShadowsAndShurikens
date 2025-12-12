#include "Widgets/SBEPExportPanel.h"

#include "BEP.h"
#include "BlueprintFlowExporter.h"

#include "Misc/Paths.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "String/LexFromString.h"

void SBEPExportPanel::Construct(const FArguments& InArgs)
{
	InitializeFormatOptions();

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
				.Text(FText::FromString(TEXT("Output Root Path (disk)")))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.f)
			[
				SNew(SEditableTextBox)
				.Text(this, &SBEPExportPanel::GetOutputRootText)
				.OnTextCommitted(this, &SBEPExportPanel::OnOutputRootCommitted)
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

			// Excluded Types
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("File Types / Classes to Exclude (comma or newline separated)")))
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
	];
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
	return FText::FromString(ExportSettings.OutputRootPath);
}

void SBEPExportPanel::OnOutputRootCommitted(const FText& NewText, ETextCommit::Type)
{
	ExportSettings.OutputRootPath = NewText.ToString();
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
