#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

struct FSOTS_BPGenBridgeVariableOpResult
{
	bool bOk = false;
	TSharedPtr<FJsonObject> Result;
	TArray<FString> Errors;
	TArray<FString> Warnings;
	FString ErrorCode;
};

namespace SOTS_BPGenBridgeVariableOps
{
	FSOTS_BPGenBridgeVariableOpResult SearchTypes(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeVariableOpResult Create(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeVariableOpResult Delete(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeVariableOpResult List(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeVariableOpResult GetInfo(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeVariableOpResult GetProperty(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeVariableOpResult SetProperty(const TSharedPtr<FJsonObject>& Params);
}
