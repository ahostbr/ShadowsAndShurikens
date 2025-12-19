#include "SSOTS_BPGenControlCenter.h"

#include "SOTS_BPGenDebug.h"
#include "SOTS_BPGen_BridgeModule.h"
#include "SOTS_BPGenBridgeServer.h"

#include "Editor.h"
#include "Dom/JsonObject.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SSOTS_BPGenControlCenter"

void SSOTS_BPGenControlCenter::Construct(const FArguments& InArgs)
{
	CachedServer = FSOTS_BPGen_BridgeModule::GetServer();

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("StartBridge", "Start Bridge"))
				.OnClicked(this, &SSOTS_BPGenControlCenter::OnStartBridge)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("StopBridge", "Stop Bridge"))
				.OnClicked(this, &SSOTS_BPGenControlCenter::OnStopBridge)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("RefreshStatus", "Refresh Status"))
				.OnClicked(this, &SSOTS_BPGenControlCenter::OnRefreshStatus)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("RefreshRecent", "Refresh Recent"))
				.OnClicked(this, &SSOTS_BPGenControlCenter::OnRefreshRecent)
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(STextBlock).Text(LOCTEXT("StatusLabel", "Server Status"))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SAssignNew(StatusBox, SMultiLineEditableTextBox)
			.IsReadOnly(true)
			.AlwaysShowScrollbars(true)
			.AutoWrapText(true)
		]

		+ SVerticalBox::Slot()
		.Padding(4.0f)
		[
			SNew(SSeparator)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(2.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("BlueprintPath", "Blueprint Path"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(2.0f)
			[
				SAssignNew(BlueprintPathText, SEditableTextBox)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(2.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("FunctionName", "Function Name"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(2.0f)
			[
				SAssignNew(FunctionNameText, SEditableTextBox)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(2.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("NodeId", "NodeId"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(2.0f)
			[
				SAssignNew(NodeIdText, SEditableTextBox)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(2.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("Annotation", "Annotation"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(2.0f)
			[
				SAssignNew(AnnotationText, SEditableTextBox).Text(FText::FromString(TEXT("[BPGEN]")))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(2.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("Annotate", "Annotate"))
					.OnClicked(this, &SSOTS_BPGenControlCenter::OnAnnotate)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("Clear", "Clear Annotations"))
					.OnClicked(this, &SSOTS_BPGenControlCenter::OnClearAnnotations)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("Focus", "Focus Node"))
					.OnClicked(this, &SSOTS_BPGenControlCenter::OnFocusNode)
				]
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(STextBlock).Text(LOCTEXT("Messages", "Messages"))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SAssignNew(LogBox, SMultiLineEditableTextBox)
			.IsReadOnly(true)
			.AlwaysShowScrollbars(true)
			.AutoWrapText(true)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(STextBlock).Text(LOCTEXT("RecentRequests", "Recent Requests"))
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(4.0f)
		[
			SAssignNew(RecentBox, SMultiLineEditableTextBox)
			.IsReadOnly(true)
			.AlwaysShowScrollbars(true)
			.AutoWrapText(true)
		]
	];

	RefreshStatus();
	OnRefreshRecent();
}

void SSOTS_BPGenControlCenter::AppendLogLine(const FString& Line)
{
	if (!LogBox.IsValid())
	{
		return;
	}

	FString Existing = LogBox->GetText().ToString();
	if (!Existing.IsEmpty())
	{
		Existing.AppendChar(TEXT('\n'));
	}
	Existing.Append(Line);
	LogBox->SetText(FText::FromString(Existing));
}

void SSOTS_BPGenControlCenter::SetLogLines(const TArray<FString>& Lines)
{
	if (LogBox.IsValid())
	{
		LogBox->SetText(FText::FromString(FString::Join(Lines, TEXT("\n"))));
	}
}

void SSOTS_BPGenControlCenter::SetStatusLines(const TArray<FString>& Lines)
{
	if (StatusBox.IsValid())
	{
		StatusBox->SetText(FText::FromString(FString::Join(Lines, TEXT("\n"))));
	}
}

void SSOTS_BPGenControlCenter::SetRecentLines(const TArray<FString>& Lines)
{
	if (RecentBox.IsValid())
	{
		RecentBox->SetText(FText::FromString(FString::Join(Lines, TEXT("\n"))));
	}
}

void SSOTS_BPGenControlCenter::RefreshStatus()
{
	TArray<FString> Lines;
	if (CachedServer.IsValid())
	{
		TSharedPtr<FJsonObject> Info;
		CachedServer.Pin()->GetServerInfoForUI(Info);
		if (Info.IsValid())
		{
			const bool bRunning = Info->GetBoolField(TEXT("running"));
			const FString BindAddress = Info->GetStringField(TEXT("bind_address"));
			const int32 Port = static_cast<int32>(Info->GetNumberField(TEXT("port")));
			const bool bSafeMode = Info->GetBoolField(TEXT("safe_mode"));
			const bool bAllowNonLoopback = Info->GetBoolField(TEXT("allow_non_loopback_bind"));
			double UptimeSeconds = 0.0;
			Info->TryGetNumberField(TEXT("uptime_seconds"), UptimeSeconds);
			const int32 MaxRps = static_cast<int32>(Info->GetNumberField(TEXT("max_requests_per_second")));
			const int32 MaxRpm = static_cast<int32>(Info->GetNumberField(TEXT("max_requests_per_minute")));
			const int32 MaxBytes = static_cast<int32>(Info->GetNumberField(TEXT("max_request_bytes")));
			const int32 TotalRequests = static_cast<int32>(Info->GetNumberField(TEXT("total_requests")));
			FString StartedUtc = Info->GetStringField(TEXT("started_utc"));
			FString LastError = Info->GetStringField(TEXT("last_start_error"));
			FString LastErrorCode = Info->GetStringField(TEXT("last_start_error_code"));

			Lines.Add(FString::Printf(TEXT("Running: %s | Bind: %s:%d | SafeMode: %s | AllowNonLoopback: %s"),
				bRunning ? TEXT("true") : TEXT("false"), *BindAddress, Port, bSafeMode ? TEXT("true") : TEXT("false"), bAllowNonLoopback ? TEXT("true") : TEXT("false")));
			Lines.Add(FString::Printf(TEXT("Uptime: %.1fs | Total requests: %d"), UptimeSeconds, TotalRequests));
			Lines.Add(FString::Printf(TEXT("Limits: max_bytes=%d | rps=%d | rpm=%d"), MaxBytes, MaxRps, MaxRpm));
			Lines.Add(FString::Printf(TEXT("Started UTC: %s"), StartedUtc.IsEmpty() ? TEXT("<not started>") : *StartedUtc));
			if (!LastError.IsEmpty() || !LastErrorCode.IsEmpty())
			{
				Lines.Add(FString::Printf(TEXT("Last start error: %s (%s)"), *LastError, *LastErrorCode));
			}

			const TSharedPtr<FJsonObject>* FeaturesPtr = nullptr;
			if (Info->TryGetObjectField(TEXT("features"), FeaturesPtr) && FeaturesPtr && FeaturesPtr->IsValid())
			{
				const TSharedPtr<FJsonObject> FeaturesObj = *FeaturesPtr;
				TArray<FString> Enabled;
				for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : FeaturesObj->Values)
				{
					bool bEnabled = false;
					if (Pair.Value.IsValid() && Pair.Value->TryGetBool(bEnabled) && bEnabled)
					{
						Enabled.Add(Pair.Key);
					}
				}

				Enabled.Sort();
				if (Enabled.Num() > 0)
				{
					Lines.Add(FString::Printf(TEXT("Features: %s"), *FString::Join(Enabled, TEXT(", "))));
				}
			}
		}
		else
		{
			Lines.Add(TEXT("Bridge server cached but info unavailable."));
		}
	}
	else
	{
		Lines.Add(TEXT("Bridge server not available."));
	}

	SetStatusLines(Lines);
}

FReply SSOTS_BPGenControlCenter::OnStartBridge()
{
	if (!CachedServer.IsValid())
	{
		CachedServer = FSOTS_BPGen_BridgeModule::GetServer();
	}

	if (!CachedServer.IsValid())
	{
		AppendLogLine(TEXT("Bridge server unavailable; could not start."));
		SetStatusLines({ TEXT("Bridge server unavailable.") });
		return FReply::Handled();
	}

	const TSharedPtr<FSOTS_BPGenBridgeServer> Server = CachedServer.Pin();
	const bool bStarted = Server->Start();

	TSharedPtr<FJsonObject> Info;
	Server->GetServerInfoForUI(Info);
	FString ErrorSuffix;
	if (Info.IsValid())
	{
		const FString LastError = Info->GetStringField(TEXT("last_start_error"));
		const FString LastErrorCode = Info->GetStringField(TEXT("last_start_error_code"));
		const bool bRunning = Info->GetBoolField(TEXT("running"));
		const FString BindAddr = Info->GetStringField(TEXT("bind_address"));
		const int32 BindPort = static_cast<int32>(Info->GetNumberField(TEXT("port")));
		if (!LastError.IsEmpty() || !LastErrorCode.IsEmpty())
		{
			ErrorSuffix = FString::Printf(TEXT(" | last_error=%s (%s)"), *LastError, *LastErrorCode);
		}
		AppendLogLine(FString::Printf(TEXT("Bridge start %s: running=%s bind=%s:%d%s"), bStarted ? TEXT("requested") : TEXT("failed"), bRunning ? TEXT("true") : TEXT("false"), *BindAddr, BindPort, *ErrorSuffix));
	}
	else
	{
		AppendLogLine(bStarted ? TEXT("Bridge start requested.") : TEXT("Bridge start failed (no status info)."));
	}

	RefreshStatus();
	return FReply::Handled();
}

FReply SSOTS_BPGenControlCenter::OnStopBridge()
{
	if (!CachedServer.IsValid())
	{
		CachedServer = FSOTS_BPGen_BridgeModule::GetServer();
	}

	if (!CachedServer.IsValid())
	{
		AppendLogLine(TEXT("Bridge server unavailable; could not stop."));
		SetStatusLines({ TEXT("Bridge server unavailable.") });
		return FReply::Handled();
	}

	CachedServer.Pin()->Stop();
	AppendLogLine(TEXT("Bridge stop requested."));
	RefreshStatus();
	return FReply::Handled();
}

FReply SSOTS_BPGenControlCenter::OnRefreshStatus()
{
	RefreshStatus();
	return FReply::Handled();
}

FReply SSOTS_BPGenControlCenter::OnRefreshRecent()
{
	if (!CachedServer.IsValid())
	{
		AppendLogLine(TEXT("Bridge server unavailable."));
		SetRecentLines({ TEXT("Bridge server unavailable.") });
		return FReply::Handled();
	}

	TArray<FString> Lines;
	TArray<TSharedPtr<FJsonObject>> Recent;
	CachedServer.Pin()->GetRecentRequestsForUI(Recent);
	for (int32 Index = Recent.Num() - 1; Index >= 0; --Index)
	{
		const TSharedPtr<FJsonObject>& Item = Recent[Index];
		if (!Item.IsValid())
		{
			continue;
		}
		const FString Action = Item->GetStringField(TEXT("action"));
		const bool bOk = Item->GetBoolField(TEXT("ok"));
		const FString ErrorCode = Item->GetStringField(TEXT("error_code"));
		const FString Timestamp = Item->GetStringField(TEXT("timestamp"));
		const FString RequestId = Item->GetStringField(TEXT("request_id"));
		double RequestMs = 0.0;
		Item->TryGetNumberField(TEXT("request_ms"), RequestMs);
		Lines.Add(FString::Printf(TEXT("[%s] req=%s action=%s ok=%s ms=%.2f err=%s"), *Timestamp, *RequestId, *Action, bOk ? TEXT("true") : TEXT("false"), RequestMs, *ErrorCode));
	}
	if (Lines.Num() == 0)
	{
		Lines.Add(TEXT("No recent requests."));
	}
	SetRecentLines(Lines);
	return FReply::Handled();
}

FReply SSOTS_BPGenControlCenter::OnAnnotate()
{
	const FString BlueprintPath = BlueprintPathText.IsValid() ? BlueprintPathText->GetText().ToString() : FString();
	const FName FunctionName = FunctionNameText.IsValid() ? FName(*FunctionNameText->GetText().ToString()) : NAME_None;
	const FString NodeId = NodeIdText.IsValid() ? NodeIdText->GetText().ToString() : FString();
	const FString Annotation = AnnotationText.IsValid() ? AnnotationText->GetText().ToString() : FString(TEXT("[BPGEN]"));

	TArray<FString> NodeIds;
	if (!NodeId.IsEmpty())
	{
		NodeIds.Add(NodeId);
	}

	const bool bOk = USOTS_BPGenDebug::AnnotateNodes(GEditor, BlueprintPath, FunctionName, NodeIds, Annotation);
	AppendLogLine(bOk ? TEXT("Annotated node(s).") : TEXT("Annotate failed."));
	return FReply::Handled();
}

FReply SSOTS_BPGenControlCenter::OnClearAnnotations()
{
	const FString BlueprintPath = BlueprintPathText.IsValid() ? BlueprintPathText->GetText().ToString() : FString();
	const FName FunctionName = FunctionNameText.IsValid() ? FName(*FunctionNameText->GetText().ToString()) : NAME_None;
	const bool bOk = USOTS_BPGenDebug::ClearAnnotations(GEditor, BlueprintPath, FunctionName);
	AppendLogLine(bOk ? TEXT("Cleared annotations.") : TEXT("Clear failed."));
	return FReply::Handled();
}

FReply SSOTS_BPGenControlCenter::OnFocusNode()
{
	const FString BlueprintPath = BlueprintPathText.IsValid() ? BlueprintPathText->GetText().ToString() : FString();
	const FName FunctionName = FunctionNameText.IsValid() ? FName(*FunctionNameText->GetText().ToString()) : NAME_None;
	const FString NodeId = NodeIdText.IsValid() ? NodeIdText->GetText().ToString() : FString();
	const bool bOk = USOTS_BPGenDebug::FocusNodeById(GEditor, BlueprintPath, FunctionName, NodeId);
	AppendLogLine(bOk ? TEXT("Focused node.") : TEXT("Focus failed."));
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
