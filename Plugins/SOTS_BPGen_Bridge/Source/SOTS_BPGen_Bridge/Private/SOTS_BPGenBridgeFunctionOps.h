#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

struct FSOTS_BPGenBridgeFunctionOpResult
{
	bool bOk = false;
	TSharedPtr<FJsonObject> Result;
	TArray<FString> Errors;
	TArray<FString> Warnings;
	FString ErrorCode;
};

namespace SOTS_BPGenBridgeFunctionOps
{
	FSOTS_BPGenBridgeFunctionOpResult List(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeFunctionOpResult Get(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeFunctionOpResult Delete(const TSharedPtr<FJsonObject>& Params);

	FSOTS_BPGenBridgeFunctionOpResult ListParams(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeFunctionOpResult AddParam(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeFunctionOpResult RemoveParam(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeFunctionOpResult UpdateParam(const TSharedPtr<FJsonObject>& Params);

	FSOTS_BPGenBridgeFunctionOpResult ListLocals(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeFunctionOpResult AddLocal(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeFunctionOpResult RemoveLocal(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeFunctionOpResult UpdateLocal(const TSharedPtr<FJsonObject>& Params);

	FSOTS_BPGenBridgeFunctionOpResult UpdateProperties(const TSharedPtr<FJsonObject>& Params);
}
