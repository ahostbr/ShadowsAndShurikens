#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

struct FSOTS_BPGenBridgeBlueprintOpResult
{
	bool bOk = false;
	TSharedPtr<FJsonObject> Result;
	TArray<FString> Errors;
	TArray<FString> Warnings;
	FString ErrorCode;
};

namespace SOTS_BPGenBridgeBlueprintOps
{
	FSOTS_BPGenBridgeBlueprintOpResult GetInfo(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeBlueprintOpResult GetProperty(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeBlueprintOpResult SetProperty(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeBlueprintOpResult Reparent(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeBlueprintOpResult ListCustomEvents(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeBlueprintOpResult SummarizeEventGraph(const TSharedPtr<FJsonObject>& Params);
}
