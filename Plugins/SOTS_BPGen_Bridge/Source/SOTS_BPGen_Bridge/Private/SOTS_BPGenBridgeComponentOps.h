#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

struct FSOTS_BPGenBridgeComponentOpResult
{
	bool bOk = false;
	TSharedPtr<FJsonObject> Result;
	TArray<FString> Errors;
	TArray<FString> Warnings;
	FString ErrorCode;
};

namespace SOTS_BPGenBridgeComponentOps
{
	FSOTS_BPGenBridgeComponentOpResult SearchTypes(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeComponentOpResult GetInfo(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeComponentOpResult List(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeComponentOpResult GetProperty(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeComponentOpResult SetProperty(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeComponentOpResult GetAllProperties(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeComponentOpResult Create(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeComponentOpResult Delete(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeComponentOpResult Reparent(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeComponentOpResult Reorder(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeComponentOpResult GetPropertyMetadata(const TSharedPtr<FJsonObject>& Params);
}
