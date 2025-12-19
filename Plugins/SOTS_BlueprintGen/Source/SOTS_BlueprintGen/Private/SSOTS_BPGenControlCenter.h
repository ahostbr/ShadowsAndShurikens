#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class FSOTS_BPGenBridgeServer;

class SSOTS_BPGenControlCenter : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSOTS_BPGenControlCenter) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply OnStartBridge();
	FReply OnStopBridge();
	FReply OnRefreshRecent();
	FReply OnRefreshStatus();
	FReply OnAnnotate();
	FReply OnClearAnnotations();
	FReply OnFocusNode();

	void AppendLogLine(const FString& Line);
	void SetLogLines(const TArray<FString>& Lines);
	void SetStatusLines(const TArray<FString>& Lines);
	void SetRecentLines(const TArray<FString>& Lines);
	void RefreshStatus();

private:
	TSharedPtr<class SEditableTextBox> BlueprintPathText;
	TSharedPtr<class SEditableTextBox> FunctionNameText;
	TSharedPtr<class SEditableTextBox> NodeIdText;
	TSharedPtr<class SEditableTextBox> AnnotationText;
	TSharedPtr<class SMultiLineEditableTextBox> LogBox;
	TSharedPtr<class SMultiLineEditableTextBox> StatusBox;
	TSharedPtr<class SMultiLineEditableTextBox> RecentBox;
	TWeakPtr<FSOTS_BPGenBridgeServer> CachedServer;
};
