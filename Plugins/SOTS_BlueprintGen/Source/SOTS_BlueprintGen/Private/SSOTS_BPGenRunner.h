#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SMultiLineEditableTextBox;
class SEditableTextBox;

/**
 * Simple in-editor runner for BPGen jobs.
 */
class SSOTS_BPGenRunner : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSOTS_BPGenRunner) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply OnRunClicked();
	void AppendLogLine(const FString& Line);
	void SetLogLines(const TArray<FString>& Lines);

	TSharedPtr<SEditableTextBox> JobPathTextBox;
	TSharedPtr<SEditableTextBox> GraphPathTextBox;
	TSharedPtr<SMultiLineEditableTextBox> LogTextBox;
};
