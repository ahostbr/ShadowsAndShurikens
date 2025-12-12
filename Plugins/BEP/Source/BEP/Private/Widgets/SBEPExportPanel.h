#pragma once

#include "BEPExportSettings.h"
#include "Widgets/SCompoundWidget.h"
#include "Input/Reply.h"

class SBEPExportPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBEPExportPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FBEPExportSettings ExportSettings;

	// Root path bindings
	FText GetRootPathText() const;
	void OnRootPathCommitted(const FText& NewText, ETextCommit::Type CommitType);

	// Output root bindings
	FText GetOutputRootText() const;
	void OnOutputRootCommitted(const FText& NewText, ETextCommit::Type CommitType);

	// Format combo
	TSharedPtr<FString> SelectedFormatItem;
	TArray<TSharedPtr<FString>> FormatOptions;
	void InitializeFormatOptions();
	TSharedRef<SWidget> OnGenerateFormatWidget(TSharedPtr<FString> InItem) const;
	void OnFormatChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	FText GetCurrentFormatLabel() const;

	// Excluded patterns bindings
	FText GetExcludedTypesText() const;
	void OnExcludedTypesCommitted(const FText& NewText, ETextCommit::Type CommitType);

	// Run action
	FReply OnRunExportClicked();

	// Helpers
	EBEPExportOutputFormat StringToFormat(const FString& FormatString) const;
	FString FormatToString(EBEPExportOutputFormat Format) const;
};
