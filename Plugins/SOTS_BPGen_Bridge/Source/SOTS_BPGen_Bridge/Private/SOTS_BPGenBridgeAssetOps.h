#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

struct FSOTS_BPGenBridgeAssetOpResult
{
	bool bOk = false;
	TSharedPtr<FJsonObject> Result;
	TArray<FString> Errors;
	TArray<FString> Warnings;
	FString ErrorCode;
};

namespace SOTS_BPGenBridgeAssetOps
{
	FSOTS_BPGenBridgeAssetOpResult Search(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeAssetOpResult OpenInEditor(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeAssetOpResult Duplicate(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeAssetOpResult Delete(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeAssetOpResult ImportTexture(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeAssetOpResult ExportTexture(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeAssetOpResult Save(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeAssetOpResult SaveAll(const TSharedPtr<FJsonObject>& Params);
	FSOTS_BPGenBridgeAssetOpResult ListReferences(const TSharedPtr<FJsonObject>& Params);
}
