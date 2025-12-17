#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/SoftObjectPath.h"
#include "SOTS_ShaderWarmupLoadListDataAsset.generated.h"

UCLASS(BlueprintType)
class SOTS_UI_API USOTS_ShaderWarmupLoadListDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Warmup")
	TArray<FSoftObjectPath> Assets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Warmup")
	bool bIncludeMaterials = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Warmup")
	bool bIncludeNiagara = true;
};
